#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "argtable3/argtable3.h"

#include "kv-params.h"
#include "errlist.h"
#include "config-filename.h"
#include "dbop.h"
#include "log.h"

bool onread
(
	void *env,
	const DbKeyValue &value
)
{
  std::cout << std::string((char *) value.value.data, value.value.length) << std::endl;
  return false;
}

int main(
  int argc,
	char* argv[]
) {
  
  KvParams params(argc, argv);
  if (params.error())
    return params.error();

  dbenv env(params.path, params.flags, params.mode);
	if (!openDb(&env)) {
		LOG(ERROR) << ERR_LMDB_OPEN << params.path << std::endl;
		return ERRCODE_LMDB_OPEN;
	}

  DbKeyValue kv(params.key, params.value);
  MatchOptions mo(params.matchMode);

  int r;

  switch (params.cmd) {
    case 'g':
      // get
      r = get(&env, kv.key, mo, onread, NULL);
      break;
    case 's':
      // set
      r = put(&env, kv);
      break;
    case '0':
      // empty
      kv.value.length = 0;
      r = put(&env, kv);
      break;
    case 'd':
      // remove
      r = rm(&env, kv.key, mo);
      break;
    default:
      break;
  }

	if (!closeDb(&env)) {
		LOG(ERROR) << ERR_LMDB_CLOSE << params.path << std::endl;
		return ERRCODE_LMDB_CLOSE;
	}

  return r;
}
