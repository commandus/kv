#include <string.h>
#include "errlist.h"

#define ERR_COUNT 9
static const char* errlist[ERR_COUNT] = {
  ERR_COMMAND,
  ERR_PARSE_COMMAND,
  ERR_LMDB_TXN_BEGIN,
  ERR_LMDB_TXN_COMMIT,
  ERR_LMDB_OPEN,
  ERR_LMDB_CLOSE,
  ERR_LMDB_PUT,
  ERR_LMDB_GET,
  ERR_NO_MEMORY
};


const char *strerror_loracli(
  int errcode
)
{
  if ((errcode <= -500) && (errcode >= -500 - ERR_COUNT)) {
    return errlist[-(errcode + 500)];
  }
  return strerror(errcode);
}
