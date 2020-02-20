#ifdef USE_LMDB
#include "lmdb.h"
#else
#include "mdbx.h"

#include <string>

#define MDB_SET_RANGE MDBX_SET_RANGE
#define MDB_FIRST MDBX_FIRST
#define MDB_NEXT MDBX_NEXT
#define MDB_PREV MDBX_PREV
#define MDB_SUCCESS MDBX_SUCCESS
#define MDB_NOTFOUND MDBX_NOTFOUND
#define MDB_BUSY MDBX_BUSY
#define MDB_cursor_op MDBX_cursor_op
#define MDB_env	MDBX_env
#define MDB_dbi	MDBX_dbi
#define MDB_txn	MDBX_txn
#define MDB_cursor	MDBX_cursor
#define MDB_val	MDBX_val
#define mdb_env_create	mdbx_env_create
#define mdb_env_open mdbx_env_open
#define mdb_env_close mdbx_env_close
#define mdb_txn_begin mdbx_txn_begin
#define mdb_txn_commit mdbx_txn_commit
#define mdb_txn_abort mdbx_txn_abort
#define mdb_strerror mdbx_strerror
#define mdb_dbi_open mdbx_dbi_open
#define mdb_dbi_close mdbx_dbi_close
#define mdb_put mdbx_put
#define mdb_del mdbx_del
#define mdb_cursor_open mdbx_cursor_open
#define mdb_cursor_get mdbx_cursor_get
#define mdb_cursor_del mdbx_cursor_del
#define MDB_envinfo MDBX_envinfo
#define mdb_env_info mdbx_env_info
#define mdb_env_set_mapsize mdbx_env_set_mapsize
#define me_mapsize mi_mapsize
#define mv_size iov_len
#define mv_data iov_base
#define MDB_MAP_FULL MDBX_MAP_FULL
#endif

class MatchOptions 
{
public:
	int cmp;		///< 0- exact, 1- starts with
	MatchOptions();
	MatchOptions(int cmp);
};

class DbString
{
public:
	size_t length;		///< bytes
	void *data;			// data pointer
	DbString();
	DbString(const std::string &value);
} ;

class DbKeyValue
{
public:
	DbString key;		///< bytes
	DbString value;		// data pointer
	DbKeyValue();
	DbKeyValue(
		const std::string &key,
		const std::string &value
	);
} ;

std::string dbString2string
(
	const DbString &value
);

void string2dbString
(
	DbString &retval,
	const std::string &value
);

/**
 * @brief LMDB environment(transaction, cursor)
 */
class dbenv {
public:	
	MDB_env *env;
	MDB_dbi dbi;
	MDB_txn *txn;
	MDB_cursor *cursor;
	// open db options
	std::string path;
	int flags;
	int mode;
	dbenv(const	std::string &path, int flags, int mode);
};

/**
 * @brief Opens LMDB database file
 * @param env created LMDB environment(transaction, cursor)
 * @param config pass path, flags, file open mode
 * @return true- success
 */
bool openDb
(
	dbenv *env
);

/**
 * @brief Close LMDB database file
 * @param config pass path, flags, file open mode
 * @return true- success
 */
bool closeDb
(
	dbenv *env
);

/**
 * @brief Store input log data to the LMDB
 * @param env database env
 * @param value key/value pair
 * @return 0 - success
 */
int put
(
	dbenv *env,
	const DbKeyValue &value
);

/**
 * @brief Store input log data to the LMDB
 * @param env database env
 * @param key key to be deleted
 * @return 0 - success
 */
int rm
(
	dbenv *env,
	const DbString &key,
	MatchOptions &matchoptions
);

// callback, return true - stop request, false- continue
typedef bool (*OnRead)
(
	void *env,
	const DbKeyValue &value
);

/**
 * @brief Read log data from the LMDB
 * @param env database env
 * @param key key to be read
 * @param onread callback. Callback return true - stop request, false- continue
 * @param onReadEnv pointer passed to the callback
 */
int get
(
	dbenv *env,
	const DbString &key,
	MatchOptions &matchoptions,
	OnRead onread,
	void *onReadEnv
);
