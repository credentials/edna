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

#include "config.h"
#include "edna_comm.h"
#include "edna_log.h"
#include "edna_proto.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#define NO_APP_SELECTED		-1
#define EDNA_BACKLOG		5			/* number of pending connections in the backlog */

edna_comm_thread::edna_comm_thread()
{
	should_run = true;
	selected_application = NO_APP_SELECTED;
}

edna_comm_thread::~edna_comm_thread()
{
	if (should_run)
	{
		terminate();
	}
}

void edna_comm_thread::terminate()
{
	should_run = false;
	
	waitexit();
}

/*virtual*/ void edna_comm_thread::threadproc()
{
	DEBUG_MSG("Entering communications thread");
	
	/* Clean up lingering old socket */
	unlink(EDNA_SOCKET);
	
	/* Set up UNIX domain socket for communications */
	int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	
	if (socket_fd < 0)
	{
		ERROR_MSG("Fatal: unable to create a socket");
		
		return;
	}
	
	DEBUG_MSG("Opened socket %d", socket_fd);
	
	struct sockaddr_un addr = { 0 };
	
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, UNIX_PATH_MAX, EDNA_SOCKET);
	
	if (bind(socket_fd, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) != 0)
	{
		ERROR_MSG("Fatal: failed to bind socket to %s", EDNA_SOCKET);
		
		close(socket_fd);
		
		unlink(EDNA_SOCKET);
		
		return;
	}
	
	INFO_MSG("Bound socket to %s", EDNA_SOCKET);
	
	if (listen(socket_fd, EDNA_BACKLOG) != 0)
	{
		ERROR_MSG("Fatal: failed to listen on socket %d (%s)", socket_fd, EDNA_SOCKET);
		
		close(socket_fd);
		
		unlink(EDNA_SOCKET);
		
		return;
	}
	
	INFO_MSG("Socket listening for connection requests");
	
	while (should_run)
	{
		struct sockaddr_un peer;
		socklen_t peer_len = sizeof(struct sockaddr_un);
		
		/* Wait for incoming connections */		
		while (should_run)
		{
			struct timeval timeout = { 0, 10000 }; // 10ms
			fd_set wait_socks;
			FD_ZERO(&wait_socks);
			FD_SET(socket_fd, &wait_socks);
	
			int rv = select(FD_SETSIZE, &wait_socks, NULL, NULL, &timeout);
			
			if (rv > 0) break;
		}
		
		if (!should_run) break;
		
		/* Accept new connection */
		int new_client_fd = accept(socket_fd, (struct sockaddr*) &peer, &peer_len);
		
		if (new_client_fd >= 0)
		{
			INFO_MSG("New client socket %d open", new_client_fd);
			
			new_client(new_client_fd);
		}
		else
		{
			switch(errno)
			{
			case EAGAIN:
				continue;
			case ECONNABORTED:
				WARNING_MSG("Incoming connection aborted");
				continue;
			case EINTR:
				WARNING_MSG("Interrupted by signal");
				break;
			default:
				ERROR_MSG("Error accepting new incoming connections (%d)", errno);
				break;
			}
		}
	}
	
	/* Close open connections to clients */
	for (std::map<bytestring, int>::iterator i = application_registry.begin(); i != application_registry.end(); i++)
	{
		close(i->second);
	}
	
	/* Clean up socket */
	close(socket_fd);
	unlink(EDNA_SOCKET);
	
	DEBUG_MSG("Leaving communications thread");
}

bool edna_comm_thread::recv_from_client(int client_socket, bytestring& rx)
{
	unsigned short rx_size = 0;
	
	size_t rx_index = 0;
	
	unsigned char buf[512] = { 0 };
	
	/*
	 * Read the length of the data to receive
	 * 
	 * FIXME: this will fail if the process receives a signal!
	 */
	if ((read(client_socket, &buf[0], 1) != 1) ||
	    (read(client_socket, &buf[1], 1) != 1))
	{
		return false;
	}
	
	rx_size = (buf[0] << 8) + buf[1];
	
	rx.resize(rx_size);
	
	/* 
	 * Now receive the actual data 
	 * 
	 * FIXME: again, will not deal well with POSIX signals!
	 */
	while (rx_size > 0)
	{
		int received = read(client_socket, &buf[0], (rx_size < 512) ? rx_size : 512);
		
		if (received < 0)
		{
			return false;
		}
		
		memcpy(&rx[rx_index], buf, received);
		
		rx_index += received;
		rx_size -= received;
	}
	
	return true;
}
	
bool edna_comm_thread::send_to_client(int client_socket, bytestring& tx)
{
	if (tx.size() > 0xffff) return -1;
	
	/* 
	 * Prepare the data for transmission by prepending a 16-bit
	 * value indicating the command length 
	 */
	unsigned short tx_size = (unsigned short) tx.size();
	
	bytestring tx_buf;
	tx_buf.resize(tx.size() + 2);
	
	tx_buf[0] = tx_size >> 8;
	tx_buf[1] = tx_size & 0xff;
	
	memcpy(&tx_buf[2], &tx[0], tx.size());
	
	/* 
	 * Transmit the command
	 * 
	 * FIXME: this will fail if the process receives a POSIX signal! 
	 */
	int tx_sent = 0;
	 
	if ((tx_sent = write(client_socket, &tx_buf[0], tx_buf.size())) != tx_buf.size())
	{
		ERROR_MSG("Expected to transmit %d bytes, write returned %d", tx_sent);
		
		return false;
	}
	
	return true;
}

void edna_comm_thread::new_client(int client_fd)
{
	INFO_MSG("New client on socket %d", client_fd);
	
	/* First, wait for the client to send the "request API version" command */
	bytestring req_api_ver;
	
	if (!recv_from_client(client_fd, req_api_ver))
	{
		ERROR_MSG("Client on socket %d failed to send API version request", client_fd);
		
		close(client_fd);
		
		return;
	}
	
	if ((req_api_ver.size() != 1) || (req_api_ver[0] != GET_API_VERSION))
	{
		ERROR_MSG("Client on socket %d uses invalid protocol, disconnecting client", client_fd);
		
		close(client_fd);
		
		return;
	}
	
	bytestring send_api_ver;
	send_api_ver += (unsigned char) API_VERSION;
	
	if (!send_to_client(client_fd, send_api_ver))
	{
		ERROR_MSG("Failed to send API version to client on socket %d", client_fd);
		
		close(client_fd);
		
		return;
	}
	
	/* Wait for the client to register an AID */
	bytestring reg_aid;
	
	if (!recv_from_client(client_fd, reg_aid))
	{
		ERROR_MSG("Client on socket %d failed to register an AID", client_fd);
		
		close(client_fd);
		
		return;
	}
	
	if ((reg_aid.size() < 2) || (reg_aid[0] != REGISTER_AID))
	{
		ERROR_MSG("Invalid AID registration by client on socket %d", client_fd);
		
		close(client_fd);
		
		return;
	}
	
	bytestring AID = reg_aid.substr(1);
	
	/* Check if the AID is already registered */
	if (application_registry.find(AID) != application_registry.end())
	{
		ERROR_MSG("Client attempted to register AID %s, which is already registered", AID.hex_str().c_str());
		
		bytestring reg_aid_rv;
		reg_aid_rv += (unsigned char) AID_EXISTS;
		
		send_to_client(client_fd, reg_aid_rv);
		
		close(client_fd);
		
		return;
	}
	else
	{
		bytestring reg_aid_rv;
		reg_aid_rv += (unsigned char) EDNA_OK;
		
		if (!send_to_client(client_fd, reg_aid_rv))
		{
			ERROR_MSG("Failed to acknowledge AID registration by client on socket %d", client_fd);
			
			close(client_fd);
			
			return;
		}
		
		INFO_MSG("New client has registered AID %s", AID.hex_str().c_str());
		
		application_registry[AID] = client_fd;
	}
}

void edna_comm_thread::select_by_aid(bytestring& aid)
{
	INFO_MSG("Request to select AID %s", aid.hex_str().c_str());
	
	/*
	 * FIXME: we only support selection by full AID at present; the
	 *        ISO 7816 standard also allows selection by partial AIDs
	 */
	if (application_registry.find(aid) != application_registry.end())
	{
		selected_application = application_registry[aid];
		
		INFO_MSG("Application selected");
	}
	else
	{
		WARNING_MSG("Application not found, currently selected application remains active");
	}
}

bool edna_comm_thread::transceive(bytestring& apdu, bytestring& rdata)
{
	DEBUG_MSG("--> %s", apdu.hex_str().c_str());
	
	rdata = "6d00";
	
	/* Check if this is a select by AID APDU */
	if (apdu.substr(0, 3) == "00a404")
	{
		if (apdu.size() < 5)
		{
			ERROR_MSG("Malformed APDU %s", apdu.hex_str().c_str());
			
			rdata = "6f00";
			
			return true;
		}
		
		// Get the AID
		bytestring aid = apdu.substr(5, apdu[4]);
		
		select_by_aid(aid); 
	}
	
	if (selected_application != NO_APP_SELECTED)
	{
		if (!send_to_client(selected_application, apdu))
		{
			ERROR_MSG("Failed to send APDU to client on socket %d, closing socket", selected_application);
			
			close(selected_application);
			
			selected_application = NO_APP_SELECTED;
			
			return false;
		}
		
		if (!recv_from_client(selected_application, rdata))
		{
			ERROR_MSG("Failed to receive R-APDU from client on socket %d, closing socket", selected_application);
			
			close(selected_application);
			
			selected_application = NO_APP_SELECTED;
			
			return false;
		}
	}
	
	DEBUG_MSG("<-- %s", rdata.hex_str().c_str());
	
	return true;
}
