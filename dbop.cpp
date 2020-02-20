#include <sstream>
#include <iostream>
#include <string.h>

#include "errlist.h"
#include "log.h"
#include "dbop.h"

#ifdef __MACH__
#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#endif

dbenv::dbenv
(
	const std::string &apath,
	int aflags,
	int amode
)
	: path(apath), flags(aflags), mode(amode)
{ 

}

MatchOptions::MatchOptions()
	: cmp(0)
{

}

MatchOptions::MatchOptions
(
	int acmp
)
	: cmp(acmp)
{

}

DbString::DbString()
	:	length(0), data(NULL)
{

}

DbString::DbString(const std::string &value)
{
	length = value.size();
	data = (void *) value.c_str();
}

DbKeyValue::DbKeyValue()
{

}

DbKeyValue::DbKeyValue
(
	const std::string &skey,
	const std::string &svalue
)
	: key(skey), value(svalue)
{

}

static int processMapFull
(
	dbenv *env
)
{
	mdb_txn_abort(env->txn);
	struct MDB_envinfo current_info;
	int r;
#ifdef USE_LMDB	
	r = mdb_env_info(env->env, &current_info);
#else
	r = mdb_env_info(env->env, &current_info, sizeof(current_info));
#endif	
	if (r)
	{
		LOG(ERROR) << "map full, mdb_env_info error " << r << ": " << mdb_strerror(r) << std::endl;
		return r;
	}
	if (!closeDb(env))
	{
		LOG(ERROR) << "map full, error close database " << std::endl;
		return ERRCODE_LMDB_CLOSE;
	}
	size_t new_size = current_info.me_mapsize * 2;
	LOG(INFO) << "map full, doubling map size from " << current_info.me_mapsize << " to " << new_size << " bytes" << std::endl;

	r = mdb_env_create(&env->env);
	if (r)
	{
		LOG(ERROR) << "map full, mdb_env_create error " << r << ": " << mdb_strerror(r) << std::endl;
		env->env = NULL;
		return ERRCODE_LMDB_OPEN;
	}
	r = mdb_env_set_mapsize(env->env, new_size);
	if (r)
		LOG(ERROR) << "map full, mdb_env_set_mapsize error " << r << ": " << mdb_strerror(r) << std::endl;
	r = mdb_env_open(env->env, env->path.c_str(), env->flags, env->mode);
	mdb_env_close(env->env);
	if (!openDb(env))
	{
		LOG(ERROR) << "map full, error re-open database" << std::endl;
		return r;
	}

	// start transaction
	r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
		LOG(ERROR) << "map full, begin transaction error " << r << ": " << mdb_strerror(r) << std::endl;
	return r;
}

/**
 * @brief Opens LMDB database file
 * @param env created LMDB environment(transaction, cursor)
 * @param config pass path, flags, file open mode
 * @return true- success
 */
bool openDb
(
	dbenv *env
)
{
	int rc = mdb_env_create(&env->env);
	if (rc)
	{
		LOG(ERROR) << "mdb_env_create error " << rc << ": " << mdb_strerror(rc) << std::endl;
		env->env = NULL;
		return false;
	}

	rc = mdb_env_open(env->env, env->path.c_str(), env->flags, env->mode);
	if (rc)
	{
		LOG(ERROR) << "mdb_env_open path: " << env->path << " error " << rc  << ": " << mdb_strerror(rc) << std::endl;
		env->env = NULL;
		return false;
	}

	rc = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (rc)
	{
		LOG(ERROR) << "mdb_txn_begin error " << rc << ": " << mdb_strerror(rc) << std::endl;
		env->env = NULL;
		return false;
	}

	rc = mdb_dbi_open(env->txn, NULL, 0, &env->dbi);
	if (rc)
	{
		LOG(ERROR) << "mdb_open error " << rc << ": " << mdb_strerror(rc) << std::endl;
		env->env = NULL;
		return false;
	}

	rc = mdb_txn_commit(env->txn);

	return rc == 0;
}

/**
 * @brief Close LMDB database file
 * @param config pass path, flags, file open mode
 * @return true- success
 */
bool closeDb
(
	dbenv *env
)
{
	mdb_dbi_close(env->env, env->dbi);
	mdb_env_close(env->env);
	return true;
}

#define DBG(n) if (verbosity > 3)	LOG(INFO) << "kv " << n << std::endl;

/**
 * @brief Store input packet to the LMDB
 * @param env database env
 * @param buffer buffer (LogEntry: device_id(0..255), ssi_signal(-128..127), MAC(6 bytes)
 * @param buffer_size buffer size
 * @return 0 - success
 */
int put
(
	dbenv *env,
	const DbKeyValue &value
)
{
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
		return ERRCODE_LMDB_TXN_BEGIN;
	MDB_val dbkey;
	dbkey.mv_size = value.key.length;
	dbkey.mv_data = value.key.data;
	MDB_val dbdata;
	dbdata.mv_size = value.value.length;
	dbdata.mv_data = value.value.data;;
	r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
	if (r) {
		if (r == MDB_MAP_FULL) {
			r = processMapFull(env);
			if (r == 0)
				r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
		}
		if (r) {
			mdb_txn_abort(env->txn);
			LOG(ERROR) << ERR_LMDB_PUT << r << ": " << mdb_strerror(r) << std::endl;
			return ERRCODE_LMDB_PUT;
		}
	}
	r = mdb_txn_commit(env->txn);
	if (r) {
		if (r == MDB_MAP_FULL) {
			r = processMapFull(env);
			if (r == 0) {
				r = mdb_txn_commit(env->txn);
			}
		}
		if (r) {
			LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << ": " << mdb_strerror(r) << std::endl;
			return ERRCODE_LMDB_TXN_COMMIT;
		}
	}
	return r;
}

static bool matchKeys
(
	const DbString &key1,
	const DbString &key2,
	const MatchOptions &matchoptions
)
{
	if (matchoptions.cmp == 0) {
		return (key1.length == key2.length)
			&& (memcmp(key1.data, key2.data, key1.length) == 0);
	} else {
		if (matchoptions.cmp == 1) {
			return (key1.length <= key2.length)
				&& (memcmp(key1.data, key2.data, key1.length) == 0);
		}
	}
	return false;
}

int get
(
	dbenv *env,
	const DbString &key,
	MatchOptions &matchoptions,
	OnRead onread,
	void *onReadEnv
)
{
	if (!onread) {
		return ERRCODE_COMMAND;
	}
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r) {
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_TXN_BEGIN;
	}
	MDB_val dbkey;
	dbkey.mv_size = key.length;
	dbkey.mv_data = key.data;
	// Get the last key
	MDB_cursor *cursor;
	MDB_val dbval;
	r = mdb_cursor_open(env->txn, env->dbi, &cursor);
	if (r != MDB_SUCCESS) {
		LOG(ERROR) << ERR_LMDB_OPEN << r << ": " << mdb_strerror(r) << std::endl;
		mdb_txn_commit(env->txn);
		return r;
	}
	r = mdb_cursor_get(cursor, &dbkey, &dbval, MDB_SET_RANGE);
	if (r != MDB_SUCCESS) {
		// it's ok
		// LOG(ERROR) << ERR_LMDB_GET << r << ": " << mdb_strerror(r) << std::endl;
		mdb_txn_commit(env->txn);
		return r;
	}

	do {
		DbKeyValue kv;
		kv.key.length = dbkey.mv_size;
		kv.key.data = dbkey.mv_data;
		kv.value.length = dbval.mv_size;
		kv.value.data = dbval.mv_data;
		if (!matchKeys(key, kv.key, matchoptions))
			break;
		if (onread(onReadEnv, kv))
			break;
	} while (mdb_cursor_get(cursor, &dbkey, &dbval, MDB_NEXT) == MDB_SUCCESS);

	r = mdb_txn_commit(env->txn);
	if (r) {
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	return r;
}

/**
 * @brief Remove log data from the LMDB
 * @param env database env
 * @param sa can be NULL
 * @param saSize can be 0
 * @param start 0- no limit
 * @param finish 0- no limit
 */
int rm
(
	dbenv *env,
	const DbString &key,
	MatchOptions &matchoptions
)
{
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	MDB_val dbkey;
	dbkey.mv_size = key.length;
	dbkey.mv_data = key.data;

	// Get the last key
	MDB_cursor *cursor;
	MDB_val dbval;
	r = mdb_cursor_open(env->txn, env->dbi, &cursor);
	if (r != MDB_SUCCESS) 
	{
		LOG(ERROR) << ERR_LMDB_OPEN << r << ": " << mdb_strerror(r) << std::endl;
		mdb_txn_commit(env->txn);
		return r;
	}
	r = mdb_cursor_get(cursor, &dbkey, &dbval, MDB_SET_RANGE);
	if (r != MDB_SUCCESS) 
	{
		LOG(ERROR) << ERR_LMDB_GET << r << ": " << mdb_strerror(r) << std::endl;
		mdb_txn_commit(env->txn);
		return r;
	}

	int cnt = 0;
	do {
		DbString k;
		k.data = dbkey.mv_data;
		k.length = dbkey.mv_size;
		if (!matchKeys(key, k, matchoptions))
			break;
		mdb_cursor_del(cursor, 0);
		cnt++;
	} while (mdb_cursor_get(cursor, &dbkey, &dbval, MDB_NEXT) == MDB_SUCCESS);

	r = mdb_txn_commit(env->txn);
	if (r) {
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << std::endl;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	if (r == 0)
		r = cnt;
	return r;
}
