#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <inttypes.h>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/json_util.h>
#include "lora.grpc.pb.h"
#include "argtable3/argtable3.h"
#include "errlist.h"
#include "utilstring.h"
#include "lora-cli-config.h"
#include "config-filename.h"
#include "search-options.h"
#include "vega.h"

#include "packet-vega-sh-2.h"
#include "packet-vega-si-13.h"

static uint64_t lastorderno = 0;
static bool stopped = false;

const std::string progname = "lora-cli";
#define  DEF_CONFIG_FILE_NAME ".lora-cli"
static void done()
{
  exit(0);
}

static void stop()
{
  stopped = true;
}

void signalHandler(int signal)
{
	switch(signal)
	{
	case SIGINT:
		std::cerr << MSG_INTERRUPTED << std::endl;
		stop();
    done();
		break;
	default:
		break;
	}
}

#ifdef _MSC_VER
// TODO
void setSignalHandler()
{
}
#else
void setSignalHandler()
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &signalHandler;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGHUP, &action, NULL);
}
#endif

/**
 * Parse command line
 * Return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int parseCmd
(
  std::string &listenAddress,
  std::string &login,
  std::string &password,
  char &cmd,
  std::vector<std::string> &eui,
  int &verbosity,
  SearchOptions &searchOptions,
	int argc,
	char* argv[]
)
{
  // override listenAddress
  struct arg_str *a_listen_address = arg_str0("h", "address", "<host:port>", "Vega address. Default ws://vega.rcitsakha.ru:8002");
  struct arg_str *a_cmd = arg_str1(NULL, NULL, "<command>", "get|hex|eui|hex|devices|save");
  // device
  struct arg_str *a_eui = arg_strn("e", "eui", "<identifier>", 0, 100, "Device identifiers (up to 100)");
  struct arg_str *a_login = arg_str0("u", "user", "<login>", "User login name");
  struct arg_str *a_password = arg_str0("p", "password", "<string>", "User password");
  struct arg_str *a_start = arg_str0("s", "start", "<date>", "2020-12-31T23:59:59 or 1609459199 (Unix epoch seconds)");
  struct arg_str *a_finish = arg_str0("f", "finish", "<date>", "2020-12-31T23:59:59 or 1609459199 (Unix epoch seconds)");

  // list
  struct arg_int *a_ofs = arg_int0("o", "offset", "<number>", "Default 0");
  struct arg_int *a_count = arg_int0("c", "count", "<number>", "Default 1024");

  struct arg_lit *a_verbosity = arg_litn("v", "verbose", 0, 3, "Set verbosity level");
	struct arg_lit *a_help = arg_lit0("?", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { 
		a_listen_address, a_cmd, a_eui, a_login, a_password, a_start, a_finish,
    a_ofs, a_count,
    a_verbosity, a_help, a_end 
	};

	int nerrors;

	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0)
	{
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}
	// Parse the command line as defined by argtable[]
	nerrors = arg_parse(argc, argv, argtable);

  cmd = **a_cmd->sval;
  switch (cmd) {
  case 'g': // get
    break;
  case 'h': // hex
    break;
  case 'd': // devices
    break;
  case 'e': // eui
    break;
  case 'p': // ping
    break;
  case 's': // save
    break;
  default:
    nerrors++;
    std::cerr << "Command must be get, eui, hex, devices, save." << std::endl;
    break;
  }

  verbosity = a_verbosity->count;

  for (int i = 0; i < a_eui->count; i++) {
    eui.push_back(*a_eui->sval);
  }

  if (a_login->count) {
    login = std::string(*a_login->sval);
  }

  if (a_password->count) {
    password = std::string(*a_password->sval);
  }

  if (a_start->count) {
    searchOptions.setStart(*a_start->sval);
  }

  if (a_finish->count) {
    searchOptions.setFinish(*a_finish->sval);
  }

  if (a_ofs->count)
    searchOptions.offset = *a_ofs->ival;
  else 
    searchOptions.offset = 0;
  if (a_count->count)
    searchOptions.size = *a_count->ival;
  else
    searchOptions.size = 1024;

  if (a_listen_address->count)
    listenAddress = std::string(*a_listen_address->sval);

	// special case: '--help' takes precedence over error reporting
	if ((a_help->count) || nerrors) {
		if (nerrors)
			arg_print_errors(stderr, a_end, progname.c_str());
		std::cerr << "Usage: " << progname << std::endl;
		arg_print_syntax(stderr, argtable, "\n");
		std::cerr << "Command line vega client" << std::endl;
		arg_print_glossary(stderr, argtable, "  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

static std::string packet2Json(
  LoraPacket *value
)
{
  std::string v;
  PacketVegaSh2 p;
  if (string2PacketVegaSh2(p, value->data)) {
    v = packetVegaSh22Json(p);
  } else {
    v = packetVegaSi132Json(value->data);
  }
  if (v.empty()) {
    v = "{}";
  }
  return v;
}

static std::string packets2Json(
  std::vector<LoraPacket> *p
)
{
  std::stringstream ss;
  ss << "[";
  if (p) {
    std::vector<LoraPacket>::iterator it = p->begin();
    if (it != p->end()) {
      ss << it->toJSON("data", packet2Json(&*it));
      it++;
    }
    for (; it != p->end(); ++it) {
      ss << ", " << it->toJSON("data", packet2Json(&*it));
    }
  }
  ss << "]" << std::endl;
  return ss.str();
}

/**
 * cmd: get, devices, ping, save
 */
int run(
  const std::string &configFileName,
  const std::string &listenAddress,
  const std::string &login,
  const std::string &password,
  const std::string &conninfo,
  char cmd,
  const std::vector<std::string> &eui,
  SearchOptions &searchOptions,  
  int verbosity
) {
  int r = 0;
	int ping_interval = 10;	// seconds
	
  switch (cmd) {
  case 'e': // eui
    {
      ClientVegaData cl(listenAddress, ping_interval, login, password, searchOptions, verbosity);
      cl.listDevices();
      cl.devices.filterDevices(eui);
      for (std::map<std::string, LoraDevice>::iterator it = cl.devices.devices.begin(); it != cl.devices.devices.end(); ++it) {
        std::cout << it->second.devEui << std::endl;
      }
    }
    break;
  case 'g': // get
    {
      ClientVegaData cl(listenAddress, ping_interval, login, password, searchOptions, verbosity);
      cl.measureDevices();
      cl.devices.filterDevices(eui);

      std::cout << "[";
      std::map<std::string, LoraDevice>::iterator it = cl.devices.devices.begin();
      if (it != cl.devices.devices.end()) {
        std::vector <std::pair<std::string, std::string>> extras;
        extras.push_back(std::pair<std::string, std::string>("measurements", packets2Json((std::vector<LoraPacket>*) it->second.data)));
        std::cout << it->second.toJSON(extras);
      }
      it++;
      for (; it != cl.devices.devices.end(); ++it) {
        std::vector <std::pair<std::string, std::string>> extras;
        extras.push_back(std::pair<std::string, std::string>("measurements", packets2Json((std::vector<LoraPacket>*) it->second.data)));
        std::cout << ", " << it->second.toJSON(extras);
      }
      std::cout << "]" << std::endl;
    }
    break;
  case 'h': // hex
    {
      ClientVegaData cl(listenAddress, ping_interval, login, password, searchOptions, verbosity);
      cl.measureDevices();
      cl.devices.filterDevices(eui);
      for (std::map<std::string, LoraDevice>::iterator it = cl.devices.devices.begin(); it != cl.devices.devices.end(); ++it) {
        if (it->second.data) {
          for (std::vector<LoraPacket>::iterator p = ((std::vector<LoraPacket>*) it->second.data)->begin(); p != ((std::vector<LoraPacket>*) it->second.data)->end(); ++p) {
            std::cout << it->second.devEui << "\t" 
              << ltimeString(p->ts / 1000) << "\t"
              << hexString(p->data)
              << std::endl;
          }
        }
      }
    }
    break;
  case 'd': // devices
    {
      ClientVegaData cl(listenAddress, ping_interval, login, password, searchOptions, verbosity);
      cl.listDevices();
      cl.devices.filterDevices(eui);
      std::cout << "[";
      std::map<std::string, LoraDevice>::iterator it = cl.devices.devices.begin();
      if (it != cl.devices.devices.end()) {
        std::cout << it->second.toJSON();
        it++;
      }
      for (; it != cl.devices.devices.end(); ++it) {
        std::cout << std::endl << ", " << it->second.toJSON();
      }
      std::cout << "]" << std::endl;
    }
    break;
  case 'p': // ping
    break;
  case 's': // save
    {
      std::ofstream ofs(configFileName.c_str(), std::ofstream::out);
      ofs << getConfigString(listenAddress, login, password, conninfo);
      if (ofs.bad()) {
        std::cerr << configFileName << " write failed." << std::endl;
        r = ERR_CODE_WRITE_FILE;
      } else {
        if (verbosity) {
          std::cerr << configFileName << " saved successfully." << std::endl;
        }
      }
      ofs.close();
    }
    break;
  default:
    break;
  }
  return r;
}

int main(
	int argc,
	char* argv[]
) {
  std::string configFileName = getDefaultConfigFileName(DEF_CONFIG_FILE_NAME).c_str();
  std::string config = file2string(configFileName.c_str());
  std::string conninfo;
  std::string listenAddress;
  std::string login;
  std::string password;
  char cmd;
  int verbosity;

  SearchOptions searchOptions;
  searchOptions.direction = 0;

  std::vector<std::string> eui;

  parseConfig(listenAddress, login, password, conninfo, config);


  if (parseCmd(listenAddress, login, password, cmd, eui, verbosity, searchOptions, argc, argv) != 0) {
    exit(ERR_CODE_COMMAND_LINE);  
  };

  // Signal handler
  setSignalHandler();

  int r = run(configFileName, listenAddress, login, password, conninfo, cmd, eui, searchOptions, verbosity);

  if (r) {
    std::cerr << "Error " << r << ": " << strerror_loracli(r) << std::endl;
  }

  done();
  exit(r);
}
