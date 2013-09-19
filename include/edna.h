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
 * General include file
 */

#ifndef _EDNA_H
#define _EDNA_H

#include <stdlib.h>

#define FLAG_SET(flags, flag) ((flags & flag) == flag)

/* Type for function return values */
typedef unsigned long edna_rv;

/* Exported functions */

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

/**
 * Initialise the edna client library
 * @return ERV_OK on success, an appropriate error otherwise
 */
edna_rv edna_lib_init(void);

/**
 * Uninitialise the edna client library
 * @return ERV_OK on success, an appropriate error otherwise
 */
edna_rv edna_lib_uninit(void);

/* Types for callback functions */
typedef int (*handle_apdu)(const unsigned char* apdu_data, size_t apdu_len, unsigned char* rdata, size_t* rdata_len);

/**
 * Connect to the daemon and register an AID
 * @param aid_data the AID data
 * @param aid_len the length of the AID data
 * @return ERV_OK if the connection was established, an appropriate error otherwise
 */
edna_rv edna_lib_connect(const unsigned char* aid_data, size_t aid_len);

/**
 * Disconnect from the daemon (unregisters the previously registered AID)
 * @return ERV_OK if disconnect was successful, an appropriate error otherwise
 */
edna_rv edna_lib_disconnect(void);

/**
 * Process APDUs sent by the daemon; this function runs an event loop
 * that calls the supplied callback function to process the APDUs.
 * The loop can be terminated by a call to edna_lib_cancel.
 * @param process_cb the callback function that processes APDUs
 * @return ERV_OK if the event loop terminated because of a call to
 *         edna_lib_cancel, an error indicating why the event loop
 *         was terminated otherwise
 */
edna_rv edna_lib_loop_and_process(handle_apdu process_cb);

/**
 * Terminate the event loop
 */
void edna_lib_cancel(void);

#ifdef __cplusplus
}
#endif // __cplusplus

/* Function return values */

/* Success */
#define ERV_OK					0x00000000

/* Warning messages */

#define ERV_ALREADY_INITIALISED	0x40000000	/* The client library was already initialised */

/* Error messages */

/* General errors */
#define ERV_GENERAL_ERROR		0x80000000	/* An undefined error occurred */
#define ERV_MEMORY				0x80000001	/* An error occurred while allocating memory */
#define ERV_PARAM_INVALID		0x80000002	/* Invalid parameter(s) provided for function call */
#define ERV_LOG_INIT_FAIL		0x80000003	/* Failed to initialise logging */
#define ERV_NOT_INITIALISED		0x80000004	/* The client library is not initialised */

/* Configuration errors */
#define ERV_NO_CONFIG			0x80001000	/* No configuration file was specified */
#define ERV_CONFIG_ERROR		0x80001001	/* An error occurred while reading the configuration file */
#define ERV_CONFIG_NO_ARRAY		0x80001002	/* The requested configuration item is not an array */
#define ERV_CONFIG_NO_STRING	0x80001003	/* The requested configuration item is not a string */

/* Library errors */
#define ERV_CONNECT_FAILED		0x80002000	/* Failed to connect to the daemon */
#define ERV_DISCONNECTED		0x80002001	/* The connection with the daemon was closed unexpectedly */
#define ERV_NOT_CONNECTED		0x80002002	/* There is no connection to the daemon */
#define ERV_VERSION_MISMATCH	0x80002003	/* The daemon reported a mismatching API version */
#define ERV_ALREADY_CONNECTED	0x80002004	/* There is already a connection to the daemon */
#define ERV_ALREADY_REGISTERED	0x80002005	/* The specified AID has already been registered with the daemon */

#endif /* !_EDNA_H */

