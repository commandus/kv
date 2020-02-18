#define DEF_TRACKS_COUNT            1440
#define DEF_DEVICE_COUNT            1024

#define ERR_CODE_COMMAND_LINE		      -500
#define ERR_CODE_LOGIN       		      -501
#define ERR_CODE_UNAUTHORIZED			  -502
#define ERR_CODE_WRONG_CREDENTIALS		  -503
#define ERR_CODE_GET_DEVICE			      -504
#define ERR_CODE_GET_TRACK    	    	  -505
#define ERR_CODE_GET_WATCHER		      -506
#define ERR_CODE_GET_RAW_DATA       	  -507
#define ERR_CODE_SET_DEVICE			      -508
#define ERR_CODE_ADD_DEVICE			      -509
#define ERR_CODE_RM_DEVICE			      -510
#define ERR_CODE_ADD_TRACK			      -511
#define ERR_CODE_SELECT             	  -512
#define ERR_CODE_PG_CONN			      -513
#define ERR_CODE_PG_CONNECTION      	  -514
#define ERR_CODE_PG_CONSUME_INPUT   	  -515
#define ERR_CODE_PG_RESULT                -516
#define ERR_CODE_PG_WRONG_SOCKET    	  -517
#define ERR_CODE_PG_CONNECT_POLL    	  -518
#define ERR_CODE_PG_LISTEN		      	  -519
#define ERR_CODE_PARAMETER_CONNINFO       -520
#define ERR_CODE_WRONG_TYPENO		      -521
#define ERR_CODE_WRONG_ERR_CONN           -522
#define ERR_CODE_WRONG_INVALID_JSON       -523
#define ERR_CODE_BAD_STATUS		          -524
#define ERR_CODE_WS_ERROR                 -525
#define ERR_CODE_WRITE_FILE               -526

#define TRACKER_OK        					0

#define ERR_COMMAND_LINE        "Wrong parameter(s)"
#define ERR_LOGIN				"Service can not verify credentials "
#define ERR_UNAUTHORIZED		"Unauthorized "
#define ERR_WRONG_CREDENTIALS	"Invalid user or password "
#define ERR_GET_DEVICE          "Error get device "
#define ERR_GET_TRACK   	    "Error get track "
#define ERR_GET_WATCHER         "Error get watcher "
#define ERR_GET_RAW_DATA        "Error get raw data packet "
#define ERR_SET_DEVICE			"Error set device "
#define ERR_ADD_DEVICE			"Error add device "
#define ERR_RM_DEVICE     		"Error remove device "
#define ERR_ADD_TRACK			"Error add track "
#define ERR_SELECT              "select() failed"
#define ERR_PG_CONN             "Postgres connection failed "
#define ERR_PG_CONNECTION       "Connection to database failed "
#define ERR_PG_CONSUME_INPUT    "Failed to consume Postgres input "
#define ERR_PG_RESULT           "Postgres result error "
#define ERR_PG_WRONG_SOCKET     "Postgres socket is gone "
#define ERR_PG_CONNECT_POLL     "Postgres query failed "
#define ERR_PG_LISTEN           "Postgres LISTEN failed "
#define ERR_PARAMETER_CONNINFO  "No database connection is provided in "
#define ERR_WRONG_TYPENO        "Wrong or missed type number"
#define ERR_WRONG_ERR_CONN      "Error connection to the Vega service "
#define ERR_WRONG_INVALID_JSON  "Invalid JSON"
#define ERR_BAD_STATUS          "Bad status"
#define ERR_WS_ERROR		    "Websocket error "
#define ERR_WRITE_FILE          "Write file error "

#define MSG_INTERRUPTED 		"Interrupted "
#define MSG_PG_CONNECTED        "Connected"
#define MSG_PG_CONNECTING       "Connecting..."
#define MSG_DAEMON_STARTED      "Start daemon "
#define MSG_DAEMON_STARTED_1    ". Check syslog."

#define STR_PG_LISTEN           "LISTEN "

// GRPC
#define ERR_CODE_GRPC_CANCELLED	        1
#define ERR_GRPC_CANCELLED	            "The operation was cancelled"
#define ERR_CODE_GRPC_UNKNOWN           2
#define ERR_GRPC_UNKNOWN                "Unknown error"
#define ERR_CODE_GRPC_INVALID_ARGUMENT  3
#define ERR_GRPC_INVALID_ARGUMENT       "The client specified an invalid argument"
#define ERR_CODE_GRPC_DEADLINE_EXCEEDED	4
#define ERR_GRPC_DEADLINE_EXCEEDED      "The deadline expired before the operation could complete"
#define ERR_CODE_GRPC_NOT_FOUND       	5
#define ERR_GRPC_NOT_FOUND              "Requested entity was not found"
#define ERR_CODE_GRPC_ALREADY_EXISTS	  6
#define ERR_GRPC_ALREADY_EXISTS         "The entity already exists"
#define ERR_CODE_GRPC_PERMISSION_DENIED	7
#define ERR_GRPC_PERMISSION_DENIED      "Does not have permission to execute the specified operation"
#define ERR_CODE_GRPC_RESOURCE_EXHAUSTED	8
#define ERR_GRPC_RESOURCE_EXHAUSTED     "Resource has been exhausted"
#define ERR_CODE_GRPC_FAILED_PRECONDITION	9
#define ERR_GRPC_FAILED_PRECONDITION    "The operation was rejected because the system is not in a state required for the operation's execution"
#define ERR_CODE_GRPC_ABORTED           10
#define ERR_GRPC_ABORTED                "The operation was aborted"
#define ERR_CODE_GRPC_OUT_OF_RANGE	    11
#define ERR_GRPC_OUT_OF_RANGE           "The operation was attempted past the valid range"
#define ERR_CODE_GRPC_UNIMPLEMENTED     12
#define ERR_GRPC_UNIMPLEMENTED          "The operation is not implemented or is not supported/enabled in this service."
#define ERR_CODE_GRPC_INTERNAL          13
#define ERR_GRPC_INTERNAL               "Internal errors"
#define ERR_CODE_GRPC_UNAVAILABLE       14
#define ERR_GRPC_UNAVAILABLE            "The service is currently unavailable"
#define ERR_CODE_GRPC_DATA_LOSS         15
#define ERR_GRPC_DATA_LOSS              "Unrecoverable data loss or corruption"
#define ERR_CODE_GRPC_UNAUTHENTICATED   16
#define ERR_GRPC_UNAUTHENTICATED        "The request does not have valid authentication credentials for the operation"

const char *strerror_loracli(int errcode);
