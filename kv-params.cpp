#include "kv-params.h"
#include <iostream>
#include <argtable3/argtable3.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>

#include "platform.h"
#include "config-filename.h"
#include "kv-config.h"

#define progname "kv"
#define CONFIG_FN ".kv"

#define DEF_DB_PATH					"."
#define DEF_MODE					0664
#define DEF_FLAGS					0

KvParams::KvParams()
	: errorcode(0), cmd('\0'), key(""), value(""),
	path(getDefaultDatabasePath()), flags(DEF_FLAGS), mode(DEF_MODE),
	matchMode(0)
{
}
	
KvParams::KvParams
(
	int argc,
	char* argv[]
)
{
	
	errorcode = parseCmd(argc, argv);
}

KvParams::~KvParams()
{
}

static std::string file2string
(
	const char *filename
)
{
	if (!filename)
		return "";
	std::ifstream strm(filename);
	return std::string((std::istreambuf_iterator<char>(strm)), std::istreambuf_iterator<char>());
}

/**
 * Parse command line into KvParams class
 * Return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int KvParams::parseCmd
(
	int argc,
	char* argv[]
)
{
	struct arg_str *a_key = arg_str1(NULL, NULL, "<key>", "Key string");
	struct arg_str *a_value = arg_str0(NULL, NULL, "<value>", "value");

	struct arg_str *a_db_path = arg_str0("p", "dbpath", "<path>", "Database path");
	struct arg_int *a_flags = arg_int0("f", "dbflags", "<number>", "Default 0");
	struct arg_int *a_mode = arg_int0("m", "dbmode", "<number>", "Default 0664");

	// manipulation
	struct arg_lit *a_delete = arg_lit0("d", "delete", "Delete key");
	struct arg_lit *a_set0 = arg_lit0("0", "empty", "Set value to nothing");

	// 0- exact, 1- starts with
	struct arg_lit *a_match_starts_with = arg_lit0(NULL, "starts-with", "Key starts with mode.");
	struct arg_str *a_config_filename = arg_str0("c", "config", "<file name>", "Default configuration file name " CONFIG_FN);
	// other
	struct arg_lit *a_verbosity = arg_litn("v", "verbose", 0, 4, "0- quiet (default), 1- errors, 2- warnings, 3- debug, 4- debug libs");
	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { 
		a_key, a_value,
		a_db_path, a_flags, a_mode,
		a_delete, a_set0, 
		a_match_starts_with,
		a_config_filename,
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

	std::string cfn;
	if (a_config_filename->count) {
		cfn = *a_config_filename->sval;
	} else {
		cfn = getDefaultConfigFileName(CONFIG_FN);
	}
	parseConfig(path, file2string(cfn.c_str()));

	cmd = 'g';	// get
	if (a_key->count)
		key = *a_key->sval;
	else
		key = "";

	if (a_value->count) {
		value = *a_value->sval;
		cmd = 's';	// set
	} else
		value = "";

	if (a_db_path->count)
		path = *a_db_path->sval;
	else {
		if (path.empty())
			path = DEF_DB_PATH;
	}
	char b[PATH_MAX];
	path = std::string(realpath(path.c_str(), b));

	if (a_mode->count)
		mode = *a_mode->ival;
	else
		mode = DEF_MODE;

	if (a_flags->count)
		flags = *a_flags->ival;
	else
		flags = DEF_FLAGS;

	if (a_delete->count)
		cmd = 'd';
	else
		if (a_set0->count)
			cmd = '0';

	if (a_match_starts_with->count)
		matchMode = 1;

	// special case: '--help' takes precedence over error reporting
	if ((a_help->count) || nerrors)
	{
		if (nerrors)
			arg_print_errors(stderr, a_end, progname);
		std::cerr << "Usage: " << progname << std::endl;
		arg_print_syntax(stderr, argtable, "\n");
		std::cerr << "key value CLI tool" << std::endl;
		arg_print_glossary(stderr, argtable, "  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

int KvParams::error()
{
	return errorcode;
}
