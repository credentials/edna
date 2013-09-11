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
 * Communication thread class
 */

#ifndef _EDNA_COMM_H
#define _EDNA_COMM_H

#include "config.h"
#include "edna_thread.h"
#include "edna_bytestring.h"
#include "edna_mutex.h"
#include <map>

class edna_comm_thread : public edna_thread
{
public:
	/**
	 * Constructor
	 */
	edna_comm_thread();
	
	/**
	 * Destructor
	 */
	~edna_comm_thread();
	
	/**
	 * End the thread
	 */
	void terminate();
	
	/**
	 * Exchange the specified APDU with the currently selected application
	 * @param apdu the APDU
	 * @param rdata the data returned by the application
	 * @return true if the APDU exchange completed normally
	 */
	bool transceive(bytestring& apdu, bytestring& rdata);
	
protected:
	/**
	 * The thread body
	 */
	virtual void threadproc();
	
private:
	/**
	 * Receive data from a client
	 * @param client_socket the client socket to receive data from
	 * @param rx buffer for the received data
	 * @return true if data was received succesfully
	 */
	bool recv_from_client(int client_socket, bytestring& rx);
	
	/**
	 * Send data to a client
	 * @param client_socket the client socket to send data to
	 * @param tx buffer to transmit
	 * @return true if data was sent successfully
	 */
	bool send_to_client(int client_socket, bytestring& tx);

	/**
	 * Process a new client
	 * @param client_fd new client socket
	 */
	void new_client(int client_fd);
	
	/**
	 * Perform selection by AID
	 * @param aid the AID to attempt to select
	 */
	void select_by_aid(bytestring& aid);

	std::map<bytestring, int> application_registry;
	
	int selected_application;

	bool should_run;
	
	edna_mutex comm_mutex;
};

#endif /* !_EDNA_COMM_H */
