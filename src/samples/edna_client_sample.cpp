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
#include "edna.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*
 * This simple sample application connects to the daemon and registers
 * the AID listed below. It then outputs all commands it receives to
 * the standard output and responds with the status word 0x9000 (OK)
 * to all commands
 */
 
/* The AID */
const static unsigned char AID[] = { 0x49, 0x52, 0x4D, 0x41, 0x63, 0x61, 0x72, 0x64 };

/* Convert a buffer of bytes to a hexadecimal string representation */
char* hex_str(const unsigned char* data, size_t len)
{
	static char buf[3*4096];
	
	if (len > 4095) len = 4095;
	
	for (int i = 0; i < len; i++)
	{
		sprintf(&buf[i*3], "%02X ", data[i]);
	}
	
	buf[(3*len)-1] = '\0';
	
	return buf;
}

int process_apdu(const unsigned char* apdu_data, size_t apdu_len, unsigned char* rdata, size_t* rdata_len)
{
	const static unsigned char sw_ok[] = { 0x90, 0x00 };
	
	printf("--> %s\n", hex_str(apdu_data, apdu_len));
	
	memcpy(rdata, sw_ok, sizeof(sw_ok));
	
	*rdata_len = sizeof(sw_ok);
	
	printf("<-- %s\n", hex_str(rdata, *rdata_len));
	
	return 0;
}

void handle_power_up(void)
{
	printf("Received POWER UP\n");
}

void handle_power_down(void)
{
	printf("Received POWER DOWN\n");
}

int main(int argc, char* argv[])
{
	edna_rv rv = ERV_OK;
	
	/* Initialise the library */
	if (edna_lib_init() != ERV_OK)
	{
		fprintf(stderr, "Failed to initialise the edna library\n");
		
		return -1;
	}
	
	/* Connect to the daemon */
	if ((rv = edna_lib_connect(AID, sizeof(AID))) != ERV_OK)
	{
		fprintf(stderr, "Failed to connect to the edna daemon (0x%08X)\n", (unsigned int) rv);
		
		return -1;
	}
	
	/* Run the event loop */
	rv = edna_lib_loop_and_process(&process_apdu, &handle_power_up, &handle_power_down);
	
	if (rv != ERV_OK)
	{
		fprintf(stderr, "Event loop exited with error (0x%08X)\n", (unsigned int) rv);
		
		return -1;
	}
	
	/* Disconnect from the daemon */
	edna_lib_disconnect();
	
	/* Uninitialise the library */
	edna_lib_uninit();
	
	return 0;
}

