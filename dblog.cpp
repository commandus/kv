#include <sstream>
#include "dblog.h"
#include "iostream"
#include "errorcodes.h"
#include "hostapd-log-entry.h"
#include <string.h>
#include <netinet/in.h>

#include <sys/ipc.h> 
#include <sys/msg.h> 

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

#include "log.h"

dbenv::dbenv(
	const std::string &apath,
	int aflags,
	int amode,
	int aqueue
)
	: path(apath), flags(aflags), mode(amode), queue(aqueue)
{ }

static void sendNotification2queue
(
	int queue,
	const uint8_t* sa, 
	const std::string &json
)
{
	std::stringstream ss;
	ss << mactostr(sa) << ":" << json;
	std::string r = ss.str();
	msgsnd(queue, r.c_str(), r.size(), 0); 
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

static int doGetNotification
(
	std::string &retval,
	dbenv *env,
	const uint8_t *sa			///< MAC address
);

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

#define DBG(n) if (verbosity > 3)	LOG(INFO) << "putLog " << n << std::endl;

/**
 * @brief Store input packet to the LMDB
 * @param env database env
 * @param buffer buffer (LogEntry: device_id(0..255), ssi_signal(-128..127), MAC(6 bytes)
 * @param buffer_size buffer size
 * @return 0 - success
 */
int putLog
(
	dbenv *env,
	void *buffer,
	size_t size,
	int verbosity
)
{
	if (size < sizeof(LogEntry))
		return ERRCODE_NO_MEMORY;
	LogEntry *entry = (LogEntry *) buffer;
	// swap bytes if needed
	ntohLogEntry(entry);
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		return ERRCODE_LMDB_TXN_BEGIN;
	}
	// log
	LogKey key;
	key.tag = 'L';
#if __BYTE_ORDER == __LITTLE_ENDIAN	
	key.dt = htobe32(time(NULL));
#else
	key.dt = time(NULL);
#endif
	memmove(key.sa, entry->sa, 6);
	MDB_val dbkey;
	dbkey.mv_size = sizeof(LogKey);
	dbkey.mv_data = &key;
	LogData data;
	data.device_id = entry->device_id;
	data.ssi_signal = entry->ssi_signal;
	MDB_val dbdata;
	dbdata.mv_size = sizeof(LogData);
	dbdata.mv_data = &data;
	if (verbosity > 2)
		LOG(INFO) << MSG_RECEIVED << size 
			<< ", MAC: " << mactostr(key.sa)
			<< ", device_id: " << entry->device_id
			<< ", ssi_signal: " << (int) entry->ssi_signal
			<< std::endl;
	r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
	if (r)
	{
		if (r == MDB_MAP_FULL) 
		{
			r = processMapFull(env);
			if (r == 0)
				r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
		}
		if (r)
		{
			mdb_txn_abort(env->txn);
			LOG(ERROR) << ERR_LMDB_PUT << r << ": " << mdb_strerror(r) << std::endl;
			return ERRCODE_LMDB_PUT;
		}
	}
	// probe
	key.tag = 'P';
	dbkey.mv_size = sizeof(LastProbeKey);
	dbdata.mv_size = sizeof(key.dt); 
	dbdata.mv_data = &(key.dt);
	r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
	if (r)
	{
		if (r == MDB_MAP_FULL) 
		{
			r = processMapFull(env);
			if (r == 0)
				r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
		}
		if (r)
		{
			mdb_txn_abort(env->txn);
			LOG(ERROR) << ERR_LMDB_PUT_PROBE << r << ": " << mdb_strerror(r) << std::endl;
			return ERRCODE_LMDB_PUT_PROBE;
		}
	}
/*	
 * TODO BUGBUG 20190221 ai
	// send notification n
	std::string n;
	int rr = doGetNotification(n, env, key.sa);
	if (rr == 0)	// if sa exists in the notification list
	{
		sendNotification2queue(env->queue, key.sa, n);
	}
*/	
	r = mdb_txn_commit(env->txn);
	if (r)
	{
		if (r == MDB_MAP_FULL) 
		{
			r = processMapFull(env);
			if (r == 0)
			{
				r = mdb_txn_commit(env->txn);
			}
		}
		if (r)
		{
			LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << ": " << mdb_strerror(r) << std::endl;
			return ERRCODE_LMDB_TXN_COMMIT;
		}
	}
	return r;
}

/**
 * @brief Store input log data to the LMDB
 * @param env database env
 * @return 0 - success
 */
int putLogEntries
(
	dbenv *env,
	int verbosity,
	OnPutLogEntry onPutLogEntry,
	void *onPutLogEntryEnv
)
{
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	LogRecord record;
	int c = 0;
	while (!onPutLogEntry(onPutLogEntryEnv, &record)) {
		// swap bytes if needed
		LogKey key;
		key.tag = 'L';
		memmove(key.sa, record.sa, 6);

		LogData data;
		data.device_id = record.device_id;
		data.ssi_signal = record.ssi_signal;	// do not htobe16
#if BYTE_ORDER == LITTLE_ENDIAN
		key.dt = htobe32(record.dt);
#endif	
		MDB_val dbkey;
		dbkey.mv_size = sizeof(LogKey);
		dbkey.mv_data = &key;
		
		MDB_val dbdata;
		dbdata.mv_size = sizeof(LogData);
		dbdata.mv_data = &data;
		
		if (verbosity > 2)
			LOG(INFO) << MSG_RECEIVED
				<< ", MAC: " << mactostr(key.sa)
				<< ", device_id: " << record.device_id
				<< ", ssi_signal: " << record.ssi_signal
				<< std::endl;

		r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
		if (r == MDB_MAP_FULL) 
		{
			r = processMapFull(env);
			if (r == 0)
				r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
		}
		if (r)
		{
			mdb_txn_abort(env->txn);
			LOG(ERROR) << ERR_LMDB_PUT << r << ": " << mdb_strerror(r) << std::endl;
			return ERRCODE_LMDB_PUT;
		}

		// probe
		key.tag = 'P';
		dbkey.mv_size = sizeof(LastProbeKey);
		dbdata.mv_size = sizeof(key.dt); 
		dbdata.mv_data = &(key.dt);
		r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
		if (r == MDB_MAP_FULL) 
		{
			r = processMapFull(env);
			if (r == 0)
				r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
		}
		if (r)
		{
			mdb_txn_abort(env->txn);
			LOG(ERROR) << ERR_LMDB_PUT_PROBE << r << ": " << mdb_strerror(r) << std::endl;
			return ERRCODE_LMDB_PUT_PROBE;
		}
		
		c++;
	}

	if (r)
	{
		mdb_txn_abort(env->txn);
		LOG(ERROR) << ERR_LMDB_PUT_PROBE << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_PUT_PROBE;
	}

	r = mdb_txn_commit(env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT  << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	return c;
}

/**
 * @brief Read log data from the LMDB
 * @param env database env
 * @param sa can be NULL
 * @param saSize can be 0
 * @param start 0- no limit
 * @param finish 0- no limit
 * @param onLog callback
 * @param onLogEnv object passed to callback
 */
int readLog
(
	dbenv *env,
	const uint8_t *sa,			// MAC address
	int saSize,
	time_t start,				// time, seconds since Unix epoch 
	time_t finish,
	OnLog onLog,
	void *onLogEnv
)
{
	if (!onLogEnv)
		return ERRCODE_WRONG_PARAM;
	struct ReqEnv* e = (struct ReqEnv*) onLogEnv;

	if (!onLog)
	{
		LOG(ERROR) << ERR_WRONG_PARAM << "onLog" << std::endl;
		return ERRCODE_WRONG_PARAM;
	}
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	LogKey key;
	key.tag = 'L';
#if __BYTE_ORDER == __LITTLE_ENDIAN	
	key.dt = htobe32(start);
#else
	key.dt = start;
#endif	
	int sz;
	if (saSize >= 6)
		sz = 6;
	else 
		if (saSize <= 0)
			sz = 0;
		else
			sz = saSize;
		
	memset(key.sa, 0, 6);
	if (sa && sz)
		memmove(key.sa, sa, sz);

	MDB_val dbkey;
	if (sz == 6)
		dbkey.mv_size = sizeof(LogKey);
	else
		dbkey.mv_size = sizeof(uint8_t) + sz;
	dbkey.mv_data = &key;
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

	if (finish == 0)
		finish = UINT32_MAX;
	if (finish < start)
	{
		uint32_t swap = start;
		start = finish;
		finish = swap;
	}

	do {
		if (dbval.mv_size < sizeof(LogData))
			continue;
		LogKey key1;
		memmove(key1.sa, ((LogKey*) dbkey.mv_data)->sa, 6);
#if __BYTE_ORDER == __LITTLE_ENDIAN	
		key1.dt = be32toh(((LogKey*) dbkey.mv_data)->dt);
#else
		key1.dt = ((LogKey*) dbkey.mv_data)->dt;
#endif	
		if (sa && sz)
			if (memcmp(key1.sa, sa, sz) != 0)
				break;
		if (key1.dt > finish)
		{
			if (sz == 6)
				break;
			else
				continue;
		}
		if (key1.dt < start)
			continue;
		if (((LogKey*) dbkey.mv_data)->tag != 'L')
			break;
		
		LogData data;
		data.device_id = ((LogData *) dbval.mv_data)->device_id;
		data.ssi_signal = ((LogData *) dbval.mv_data)->ssi_signal;
		if (onLog(onLogEnv, &key1, &data))
			break;
	} while (mdb_cursor_get(cursor, &dbkey, &dbval, MDB_NEXT) == MDB_SUCCESS);

	r = mdb_txn_commit(env->txn);
	if (r)
	{
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
int rmLog
(
	dbenv *env,
	const uint8_t *sa,			// MAC address
	int saSize,
	time_t start,				// time, seconds since Unix epoch 
	time_t finish
)
{
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	LogKey key;
	key.tag = 'L';
#if __BYTE_ORDER == __LITTLE_ENDIAN	
	key.dt = htobe32(start);
#else
	key.dt = start;
#endif	
	int sz;
	if (saSize >= 6)
		sz = 6;
	else 
		if (saSize <= 0)
			sz = 0;
		else
			sz = saSize;
		
	memset(key.sa, 0, 6);
	if (sa)
		memmove(key.sa, sa, sz);

	MDB_val dbkey;
	dbkey.mv_size = sizeof(LogKey);
	dbkey.mv_data = &key;

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

	if (finish == 0)
		finish = UINT32_MAX;
	if (finish < start)
	{
		uint32_t swap = start;
		start = finish;
		finish = swap;
	}

	int cnt = 0;
	do {
		if ((dbval.mv_size < sizeof(LogData)) || (dbkey.mv_size < sizeof(LogKey)))
			continue;
		LogKey key1;
		memmove(key1.sa, ((LogKey*) dbkey.mv_data)->sa, 6);
#if __BYTE_ORDER == __LITTLE_ENDIAN	
		key1.dt = be32toh(((LogKey*) dbkey.mv_data)->dt);
#else
		key1.dt = ((LogKey*) dbkey.mv_data)->dt;
#endif	
		if (sa)
			if (memcmp(key1.sa, sa, sz) != 0)
				break;

		if (key1.dt > finish)
		{
			if (sz == 6)
				break;
			else
				continue;
		}
		if (key1.dt < start)
			continue;
		if (((LogKey*) dbkey.mv_data)->tag != 'L')
			break;

		LogData data;
		data.device_id = ((LogData *) dbval.mv_data)->device_id;
		data.ssi_signal = ((LogData *) dbval.mv_data)->ssi_signal;
		mdb_cursor_del(cursor, 0);
		cnt++;
	} while (mdb_cursor_get(cursor, &dbkey, &dbval, MDB_NEXT) == MDB_SUCCESS);

	r = mdb_txn_commit(env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << std::endl;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	if (r == 0)
		r = cnt;
	return r;
}

/**
 * @brief Read last probes from the LMDB
 * @param env database env
 * @param sa can be NULL
 * @param saSize can be 0 
 * @param start 0- no limit
 * @param finish 0- no limit
 * @param onLog callback
 */
int readLastProbe
(
	dbenv *env,
	const uint8_t *sa,			///< MAC address filter
	int saSize,
	time_t start,				// time, seconds since Unix epoch 
	time_t finish,
	OnLog onLog,
	void *onLogEnv
)
{
	if (!onLogEnv)
		return ERRCODE_WRONG_PARAM;
	struct ReqEnv* e = (struct ReqEnv*) onLogEnv;

	if (!onLog)
	{
		LOG(ERROR) << ERR_WRONG_PARAM << "onLog" << std::endl;
		return ERRCODE_WRONG_PARAM;
	}
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r << std::endl;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	LastProbeKey key;
	key.tag = 'P';

	int sz;
	if (saSize >= 6)
		sz = 6;
	else 
		if (saSize <= 0)
			sz = 0;
		else
			sz = saSize;

	memset(key.sa, 0, 6);
	if (sa && (sz > 0))
		memmove(key.sa, sa, sz);
	MDB_val dbkey;
	dbkey.mv_size = sizeof(LastProbeKey) - (6 - sz);	// exclude incomplete MAC address bytes
	dbkey.mv_data = &key;

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

	if (finish == 0)
		finish = UINT32_MAX;
	if (finish < start)
	{
		uint32_t swap = start;
		start = finish;
		finish = swap;
	}

	do {
		LogKey key1;
		if ((dbkey.mv_size < sizeof(LastProbeKey)) || (dbval.mv_size < sizeof(key1.dt)))
			continue;
		memmove(key1.sa, ((LastProbeKey*) dbkey.mv_data)->sa, 6);
		if (sa)
		{
			if (memcmp(key1.sa, sa, sz) != 0)
				break;
		}
#if __BYTE_ORDER == __LITTLE_ENDIAN	
		key1.dt = be32toh(*((time_t*) dbval.mv_data));
#else
		key1.dt = *((time_t*) dbval.mv_data);
#endif	
		if ((key1.dt > finish) || (key1.dt < start))
			continue;
		if (onLog(onLogEnv, &key1, NULL))
			break;
	} while (mdb_cursor_get(cursor, &dbkey, &dbval, MDB_NEXT) == MDB_SUCCESS);

	r = mdb_txn_commit(env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << std::endl;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	return r;
}

// Notification database routines 

/**
 * Get notification JSON string into retval in transaction
 * @param retval return value
 * @return empty string if not found
 */
static int doGetNotification
(
	std::string &retval,
	dbenv *env,
	const uint8_t *sa			///< MAC address
)
{
	LastProbeKey key;
	key.tag = 'T';
	memmove(key.sa, sa, 6);
	MDB_val dbkey;
	dbkey.mv_size = 6;
	dbkey.mv_data = &key;

	// Get the last key
	MDB_cursor *cursor;
	MDB_val dbval;
	int r = mdb_cursor_open(env->txn, env->dbi, &cursor);
	if (r != MDB_SUCCESS) 
	{
		LOG(ERROR) << ERR_LMDB_OPEN << r << ": " << mdb_strerror(r) << std::endl;
		return r;
	}
	r = mdb_cursor_get(cursor, &dbkey, &dbval, MDB_FIRST);
	if (r != MDB_SUCCESS) 
	{
		LOG(ERROR) << ERR_LMDB_GET << r << ": " << mdb_strerror(r) << std::endl;
		return r;
	}

	if (dbval.mv_size)
		retval = std::string((const char *) dbval.mv_data, dbval.mv_size);

	return 0;
}

/**
 * Get notification JSON string into retval
 * @param retval return value
 * @return empty string if not found
 */
int getNotification
(
	std::string &retval,
	dbenv *env,
	const uint8_t *sa			///< MAC address
)
{
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	r = doGetNotification(retval, env, sa);
	if (r < 0)
#ifdef USE_LMDB	
		mdb_txn_abort(env->txn);
#else
		r = mdb_txn_abort(env->txn);
#endif
	else
#ifdef USE_LMDB		
		mdb_txn_commit(env->txn);
#else
		r = mdb_txn_commit(env->txn);
#endif		
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << std::endl;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	return r;
}

/**
 * Put notification JSON string 
 * @param value return notification JSON string. If empty, clear
 * @return 0 - success
 */
int putNotification
(
	dbenv *env,
	const uint8_t *sa,			///< MAC address
	const std::string &value
)
{
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	// notification
	LastProbeKey key;
	key.tag = 'T';
	memmove(key.sa, sa, 6);
	MDB_val dbkey;
	dbkey.mv_size = sizeof(LogKey);
	dbkey.mv_data = &key;

	MDB_val dbdata;
	dbdata.mv_size = value.size();
	dbdata.mv_data = (void *) value.c_str();
	
	if (value.size())
		r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
	else
		r = mdb_del(env->txn, env->dbi, &dbkey, 0);
	if (r == MDB_MAP_FULL) 
	{
		r = processMapFull(env);
		if (r == 0)
		{
			if (value.size())
				r = mdb_put(env->txn, env->dbi, &dbkey, &dbdata, 0);
			else
				r = mdb_del(env->txn, env->dbi, &dbkey, 0);
		}
	}
	if (r)
	{
		mdb_txn_abort(env->txn);
		LOG(ERROR) << ERR_LMDB_PUT << r;
		return ERRCODE_LMDB_PUT;
	}
	r = mdb_txn_commit(env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	return r;
}

/**
 * List notification JSON string 
 * @param value return notification JSON string 
 * @return 0 - success
 */
int lsNotification
(
	dbenv *env,
	OnNotification onNotification,
	void *onNotificationEnv
)
{
	if (!onNotification)
	{
		LOG(ERROR) << ERR_WRONG_PARAM << "onNotification" << std::endl;
		return ERRCODE_WRONG_PARAM;
	}
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r << std::endl;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	LastProbeKey key;
	key.tag = 'T';

	memset(key.sa, 0, 6);
	MDB_val dbkey;
	dbkey.mv_size = 6;
	dbkey.mv_data = &key;

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
		// LOG(ERROR) << ERR_LMDB_GET << r << ": " << mdb_strerror(r) << std::endl;
		mdb_txn_abort(env->txn);
		return r;
	}

	do {
		if ((dbval.mv_size <= 0) || (dbkey.mv_size < 6))
			continue;
		std::string json((const char *) dbval.mv_data, dbval.mv_size);
		if (onNotification(onNotificationEnv, (const char *) ((LastProbeKey*) dbkey.mv_data)->sa, json))
			break;
	} while (mdb_cursor_get(cursor, &dbkey, &dbval, MDB_NEXT) == MDB_SUCCESS);

	r = mdb_txn_commit(env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	return r;
}

/**
 * Remove notification JSON string 
 * @return 0 - success
 */
int rmNotification
(
	dbenv *env,
	const uint8_t *sa,
	int sa_size
)
{
	// start transaction
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r << std::endl;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	LastProbeKey key;
	key.tag = 'T';

	memset(key.sa, 0, 6);
	int sz = sa_size < 6 ? sa_size : 6;
	memmove(key.sa, sa, sz);
	MDB_val dbkey;
	dbkey.mv_size = sizeof(key.tag) + sz;
	dbkey.mv_data = &key;

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
		// LOG(ERROR) << ERR_LMDB_GET << r << ": " << mdb_strerror(r) << std::endl;
		mdb_txn_abort(env->txn);
		if (r == MDB_NOTFOUND)
			return 0;
		return r;
	}

	int cnt = 0;
	do {
		if (dbkey.mv_size < 6)
			break;
		if (((LastProbeKey *) dbkey.mv_data)->tag != 'T')
			break;
		if (memcmp(((LastProbeKey *) dbkey.mv_data)->sa, sa, sz) != 0)
			break;
		mdb_cursor_del(cursor, 0);
		cnt++;
	} while (mdb_cursor_get(cursor, &dbkey, &dbval, MDB_NEXT) == MDB_SUCCESS);

	r = mdb_txn_commit(env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << ": " << mdb_strerror(r) << std::endl;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	return r == 0 ? cnt : r;
}
