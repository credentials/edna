/*
 * Copyright (c) 2013 Roland van Rijswijk-Deij
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * The Emulator Daemon for NFC Applications (EDNA)
 * Daemon main entry point
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <string>
#include "edna.h"
#include "edna_config.h"
#include "edna_log.h"
#include "edna_comm.h"
#include "edna_emu.h"

/* Communications thread object */
static edna_comm_thread* comm_thread = NULL;

/* Emulator */
static edna_emulator* emulator = NULL;

void version(void)
{
	printf("Emulator Daemon for NFC Applications (edna) version %s\n", VERSION);
	printf("Copyright (c) 2013 Roland van Rijswijk-Deij\n\n");
	printf("Use, modification and redistribution of this software is subject to the terms\n");
	printf("of the license agreement. This software is licensed under a 2-clause BSD-style\n");
	printf("license a copy of which is included as the file LICENSE in the distribution.\n");
}

void usage(void)
{
	printf("Emulator Daemon for NFC Applications (edna) version %s\n\n", VERSION);
	printf("Usage:\n");
	printf("\tedna [-f] [-c <config>] [-p <pidfile>]\n");
	printf("\tedna -h\n");
	printf("\tedna -v\n");
	printf("\n");
	printf("\t-f            Run in the foreground rather than forking as a daemon\n");
	printf("\t-c <config>   Use <config> as configuration file\n");
	printf("\t              Defaults to %s\n", DEFAULT_EDNA_CONF);
	printf("\t-p <pidfile>  Specify the PID file to write the daemon process ID to\n");
	printf("\t              Defaults to %s\n", DEFAULT_EDNA_PIDFILE);
	printf("\n");
	printf("\t-h            Print this help message\n");
	printf("\n");
	printf("\t-v            Print the version number\n");
}

void write_pid(const char* pid_path, pid_t pid)
{
	FILE* pid_file = fopen(pid_path, "w");

	if (pid_file == NULL)
	{
		ERROR_MSG("Failed to write the pid file %s", pid_path);

		return;
	}

	fprintf(pid_file, "%d\n", pid);
	fclose(pid_file);
}

/* Signal handler for unexpected exit codes */
void signal_unexpected(int signum)
{
	switch(signum)
	{
	case SIGABRT:
		ERROR_MSG("Caught SIGABRT");
		break;
	case SIGBUS:
		ERROR_MSG("Caught SIGBUS");
		break;
	case SIGFPE:
		ERROR_MSG("Caught SIGFPE");
		break;
	case SIGILL:
		ERROR_MSG("Caught SIGILL");
		break;
	case SIGPIPE:
		ERROR_MSG("Caught SIGPIPE");
		break;
	case SIGQUIT:
		ERROR_MSG("Caught SIGQUIT");
		break;
	case SIGSEGV:
		ERROR_MSG("Caught SIGSEGV");
		exit(-1);
		break;
	case SIGSYS:
		ERROR_MSG("Caught SIGSYS");
		break;
	case SIGXCPU:
		ERROR_MSG("Caught SIGXCPU");
		break;
	case SIGXFSZ:
		ERROR_MSG("Caught SIGXFSZ");
		break;
	default:
		ERROR_MSG("Caught unknown signal 0x%X", signum);
		break;
	}
}

/* Signal handler for normal termination */
void signal_term(int signum)
{
	if (comm_thread != NULL)
	{
		comm_thread->terminate();
	}
	
	if (emulator != NULL)
	{
		emulator->cancel();
	}
}

int main(int argc, char* argv[])
{
	std::string 		config_path;
	std::string 		pid_path;
	bool 				daemon 			= true;
	int 				c 				= 0;
	bool 				pid_path_set 	= false;
	bool 				daemon_set 		= false;
	pid_t 				pid 			= 0;
	
	comm_thread = NULL;
	
	while ((c = getopt(argc, argv, "fc:p:hv")) != -1)
	{
		switch(c)
		{
		case 'f':
			daemon = false;
			daemon_set = true;
			break;
		case 'c':
			config_path = std::string(optarg);

			break;
		case 'p':
			pid_path = std::string(optarg);

			pid_path_set = true;
			break;
		case 'h':
			usage();
			return 0;
		case 'v':
			version();
			return 0;
		}
	}

	if (config_path.empty())
	{
		config_path = std::string(DEFAULT_EDNA_CONF);
	}

	if (pid_path.empty())
	{
		pid_path = std::string(DEFAULT_EDNA_PIDFILE);

	}

	/* Load the configuration */
	if (edna_init_config_handling(config_path.c_str()) != ERV_OK)
	{
		fprintf(stderr, "Failed to load the configuration, exiting\n");

		return ERV_CONFIG_ERROR;
	}

	/* Initialise logging */
	if (edna_init_log() != ERV_OK)
	{
		fprintf(stderr, "Failed to initialise logging, exiting\n");

		return ERV_LOG_INIT_FAIL;
	}

	/* Determine configuration settings that were not specified on the command line */
	if (!pid_path_set)
	{
		std::string conf_pid_path;

		if (edna_conf_get_string("daemon", "pidfile", conf_pid_path, NULL) != ERV_OK)
		{
			ERROR_MSG("Failed to retrieve pidfile information from the configuration");
		}
		else
		{
			if (!conf_pid_path.empty())
			{
				pid_path = conf_pid_path;
			}
		}
	}

	if (!daemon_set)
	{
		if (edna_conf_get_bool("daemon", "fork", daemon, true) != ERV_OK)
		{
			ERROR_MSG("Failed to retrieve daemon information from the configuration");
		}
	}

	/* Now fork if that was requested */
	if (daemon)
	{
		pid = fork();

		if (pid != 0)
		{
			/* This is the parent process; write the PID file and exit */
			write_pid(pid_path.c_str(), pid);

			/* Unload the configuration */
			if (edna_uninit_config_handling() != ERV_OK)
			{
				ERROR_MSG("Failed to uninitialise configuration handling");
			}
		
			/* Uninitialise logging */
			if (edna_uninit_log() != ERV_OK)
			{
				fprintf(stderr, "Failed to uninitialise logging\n");
			}
		
			return ERV_OK;
		}
	}

	/* If we forked, this is the child */
	INFO_MSG("Starting the Emulator Daemon for NFC Applications (edna) version %s", VERSION);
	INFO_MSG("edna %sprocess ID is %d", daemon ? "daemon " : "", getpid());

	/* Install signal handlers */
	signal(SIGABRT, signal_unexpected);
	signal(SIGBUS, signal_unexpected);
	signal(SIGFPE, signal_unexpected);
	signal(SIGILL, signal_unexpected);
	signal(SIGPIPE, signal_unexpected);
	signal(SIGQUIT, signal_unexpected);
	signal(SIGSEGV, signal_unexpected);
	signal(SIGSYS, signal_unexpected);
	signal(SIGXCPU, signal_unexpected);
	signal(SIGXFSZ, signal_unexpected);
	
	signal(SIGTERM, signal_term);
	signal(SIGINT, signal_term);
	
	/* Launch communications thread*/
	comm_thread = new edna_comm_thread();
	
	comm_thread->start();
	
	/* Run emulation */
	emulator = new edna_emulator(comm_thread);
	
	emulator->run();
	
	/* Terminate communications thread */
	comm_thread->terminate();
	
	edna_comm_thread* ct_to_delete = comm_thread;
	comm_thread = NULL;
	
	edna_emulator* emu_to_delete = emulator;
	emulator = NULL;
	
	delete ct_to_delete;
	delete emu_to_delete;
	
	/* Tell the world we're exiting */
	INFO_MSG("The Emulator Daemon for NFC Applications (edna) version %s has now stopped", VERSION);

	/* Unload the configuration */
	if (edna_uninit_config_handling() != ERV_OK)
	{
		ERROR_MSG("Failed to uninitialise configuration handling");
	}

	/* Remove signal handlers */
	signal(SIGABRT, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGILL, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGSYS, SIG_DFL);
	signal(SIGXCPU, SIG_DFL);
	signal(SIGXFSZ, SIG_DFL);
	
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);

	/* Uninitialise logging */
	if (edna_uninit_log() != ERV_OK)
	{
		fprintf(stderr, "Failed to uninitialise logging\n");
	}

	return ERV_OK;
}

