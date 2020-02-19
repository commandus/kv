/**
  * kv options
  * @file kv-params.h
  **/

#ifndef KV_PARAMS_H
#define KV_PARAMS_H	1

#include <string>
#include <vector>

/**
 * Command line interface (CLI) tool configuration structure
 */
class KvParams
{
private:
	/**
	* Parse command line into KvParams class
	* Return 0- success
	*        1- show help and exit, or command syntax error
	*        2- output file does not exists or can not open to write
	**/
	int parseCmd
	(
		int argc,
		char* argv[]
	);
	int errorcode;
public:
	// key value manipulation get, set(put), list
	char cmd;										///< g- get, s- set, d- delete, 0- empty
	std::string key;
	std::string value;
	// database options
	std::string path;								///< database files lock.mdb, data.mdb directory path
	int flags;
	int mode;
	// match options
	int matchMode;									///< 0- exact, 1- starts with

	KvParams();
	KvParams
	(
		int argc,
		char* argv[]
	);
	~KvParams();
	int error();
};

#endif
