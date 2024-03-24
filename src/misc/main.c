#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>

struct options {
	int loglevel;
	int logopt;
	int interactive;
	int daemonize;
};

static void usage(const char *cmd)
{
	printf("%s : A thermal monitoring engine based on notifications\n", cmd);
	printf("Usage: %s [options]\n", cmd);
	printf("\t-h, --help\t\tthis help\n");
	printf("\t-d, --daemonize\n");
	printf("\t-l <level>, --loglevel <level>\tlog level: ");
	printf("DEBUG, INFO, NOTICE, WARN, ERROR\n");
	printf("\t-s, --syslog\t\toutput to syslog\n");
	printf("\n");
	exit(0);
}

static int options_init(int argc, char *argv[], struct options *options)
{
	int opt;

	struct option long_options[] = {
		{ "help",	no_argument, NULL, 'h' },
		{ "daemonize",	no_argument, NULL, 'd' },
		{ "syslog",	no_argument, NULL, 's' },
		{ "loglevel",	required_argument, NULL, 'l' },
		{ 0, 0, 0, 0 }
	};

	while (1) {

		int optindex = 0;

		opt = getopt_long(argc, argv, "l:dhs", long_options, &optindex);
		if (opt == -1)
			break;

		switch (opt) {
		case 'l':
			options->loglevel = log_str2level(optarg);
			break;
		case 'd':
			options->daemonize = 1;
			break;
		case 's':
			options->logopt = TO_SYSLOG;
			break;
		case 'h':
			usage(basename(argv[0]));
			break;
		default: /* '?' */
			return -1;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct options options = {
		.loglevel = LOG_INFO,
		.logopt = TO_STDOUT,
	};

if (options_init(argc, argv, &options)) {
		ERROR("Usage: %s --help\n", argv[0]);
		return THERMAL_ENGINE_OPTION_ERROR;
}

if (options.daemonize && daemon(0, 0)) {
		ERROR("Failed to daemonize: %p\n");
		return THERMAL_ENGINE_DAEMON_ERROR;
}

  return 0;
}
