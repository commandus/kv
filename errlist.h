#define ERR_CODE_COMMAND_LINE		      -500

#define MSG_OK									"OK"
#define MSG_FAILED								"failed"

#define ERR_OK									0
#define ERRCODE_COMMAND							-500
#define ERRCODE_PARSE_COMMAND					-501
#define ERRCODE_LMDB_TXN_BEGIN					-502
#define ERRCODE_LMDB_TXN_COMMIT					-503
#define ERRCODE_LMDB_OPEN						-504
#define ERRCODE_LMDB_CLOSE						-505
#define ERRCODE_LMDB_PUT						-506
#define ERRCODE_LMDB_GET						-507
#define ERRCODE_NO_MEMORY						-508

#define ERR_COMMAND								"Invalid command line options or help requested. "
#define ERR_PARSE_COMMAND						"Error parse command line options, possible cause is insufficient memory. "

#define ERR_LMDB_TXN_BEGIN						"Can not begin database transaction "
#define ERR_LMDB_TXN_COMMIT						"Can not commit database transaction "
#define ERR_LMDB_OPEN							"Can not open database file "
#define ERR_LMDB_CLOSE							"Can not close database file "
#define ERR_LMDB_PUT							"Can not put LMDB "
#define ERR_LMDB_GET							"Can not get LMDB "

#define ERR_NO_MEMORY							"Can not allocate buffer size "

const char *strerrorDescription(int errcode);
