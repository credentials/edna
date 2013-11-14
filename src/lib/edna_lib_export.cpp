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
 * Exported functions
 */

#include "config.h"
#include "edna.h"
#include "edna_proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>


/* Library status */
static bool edna_lib_initialised = false;

static bool edna_lib_connected = false;

static bool edna_lib_must_cancel = false;

/* Connection to the daemon */
int daemon_socket = -1;

edna_rv edna_lib_init(void)
{
	if (edna_lib_initialised)
	{
		return ERV_ALREADY_INITIALISED;
	}
	
	edna_lib_initialised = true;
	edna_lib_connected = false;
	daemon_socket = -1;
	
	return ERV_OK;
}

edna_rv edna_lib_uninit(void)
{
	if (!edna_lib_initialised)
	{
		return ERV_NOT_INITIALISED;
	}
	
	if ((daemon_socket >= 0) && edna_lib_connected)
	{
		close(daemon_socket);
		edna_lib_connected = false;
	}
	
	edna_lib_initialised = false;
	edna_lib_connected = false;
	daemon_socket = -1;
	
	return ERV_OK;
}

int send_to_daemon(const std::vector<unsigned char> tx)
{
	if (!edna_lib_connected || (daemon_socket < 0))
	{
		return -1;
	}
	
	if (tx.size() > 0xffff) return -1;
	
	/* 
	 * Prepare the data for transmission by prepending a 16-bit
	 * value indicating the command length 
	 */
	unsigned short tx_size = (unsigned short) tx.size();
	
	std::vector<unsigned char> tx_buf;
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
	 
	if ((tx_sent = write(daemon_socket, &tx_buf[0], tx_buf.size())) != tx_buf.size())
	{
		close(daemon_socket);
		
		edna_lib_connected = false;
		daemon_socket = -1;
		
		return -2;
	}
	
	return 0;
}

int recv_from_daemon(std::vector<unsigned char>& rx)
{
	if (!edna_lib_connected || (daemon_socket < 0))
	{
		return -1;
	}
	
	unsigned short rx_size = 0;
	
	size_t rx_index = 0;
	
	unsigned char buf[512] = { 0 };
	
	/*
	 * Read the length of the data to receive
	 * 
	 * FIXME: this will fail if the process receives a signal!
	 */
	if ((read(daemon_socket, &buf[0], 1) != 1) ||
	    (read(daemon_socket, &buf[1], 1) != 1))
	{
		close(daemon_socket);
		
		edna_lib_connected = false;
		daemon_socket = -1;
		
		return -2;
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
		int received = read(daemon_socket, &buf[0], (rx_size < 512) ? rx_size : 512);
		
		if (received < 0)
		{
			close(daemon_socket);
		
			edna_lib_connected = false;
			daemon_socket = -1;
			
			return -2;
		}
		
		memcpy(&rx[rx_index], buf, received);
		
		rx_index += received;
		rx_size -= received;
	}
	
	return 0;
}

edna_rv edna_lib_connect(const unsigned char* aid_data, size_t aid_len)
{
	if (edna_lib_connected)
	{
		return ERV_ALREADY_CONNECTED;
	}
	
	/* Attempt to connect to the daemon */
	struct sockaddr_un addr = { 0 };
	
	daemon_socket = socket(PF_UNIX, SOCK_STREAM, 0);
	
	if (daemon_socket < 0)
	{
		daemon_socket = -1;
		
		return ERV_CONNECT_FAILED;
	}
	
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, UNIX_PATH_MAX, EDNA_SOCKET);
	
	if (connect(daemon_socket, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) != 0)
	{
		close(daemon_socket);
		
		daemon_socket = -1;
		
		return ERV_CONNECT_FAILED;
	}
	
	edna_lib_connected = true;
	
	/* Request the API version from the daemon */
	std::vector<unsigned char> get_api_version;
	get_api_version.push_back(GET_API_VERSION);
	
	if (send_to_daemon(get_api_version) != 0)
	{
		close(daemon_socket);
		
		daemon_socket = -1;
		edna_lib_connected = false;
		
		return ERV_DISCONNECTED;
	}
	
	std::vector<unsigned char> api_version_info;
	
	if (recv_from_daemon(api_version_info) != 0)
	{
		close(daemon_socket);
		
		daemon_socket = -1;
		edna_lib_connected = false;
		
		return ERV_DISCONNECTED;
	}
	
	if ((api_version_info.size() != 1) || (api_version_info[0] != API_VERSION))
	{
		close(daemon_socket);
		
		daemon_socket = -1;
		edna_lib_connected = false;
		
		return ERV_VERSION_MISMATCH;
	}
	
	/* The API version checks out, register the AID with the daemon */
	std::vector<unsigned char> register_aid;
	register_aid.resize(aid_len + 1);
	register_aid[0] = REGISTER_AID;
	memcpy(&register_aid[1], aid_data, aid_len);
	
	if (send_to_daemon(register_aid) != 0)
	{
		close(daemon_socket);
		
		daemon_socket = -1;
		edna_lib_connected = false;
		
		return ERV_DISCONNECTED;
	}
	
	std::vector<unsigned char> register_aid_rv;
	
	if (recv_from_daemon(register_aid_rv) != 0)
	{
		close(daemon_socket);
		
		daemon_socket = -1;
		edna_lib_connected = false;
		
		return ERV_DISCONNECTED;
	}
	
	if ((register_aid_rv.size() != 1) || (register_aid_rv[0] != EDNA_OK))
	{
		close(daemon_socket);
		
		daemon_socket = -1;
		edna_lib_connected = false;
		
		return ERV_ALREADY_REGISTERED;
	}
	
	return ERV_OK;
}

edna_rv edna_lib_disconnect(void)
{
	if (!edna_lib_connected)
	{
		return ERV_NOT_CONNECTED;
	}
	
	/* Send disconnect command */
	std::vector<unsigned char> disconnect_cmd;
	disconnect_cmd.push_back(DISCONNECT);
	
	send_to_daemon(disconnect_cmd);
	
	close(daemon_socket);
	edna_lib_connected = false;
	
	return ERV_OK;
}

edna_rv edna_lib_loop_and_process(handle_apdu process_cb, power_up power_up_cb, power_down power_down_cb)
{
	if ((process_cb == NULL) || (power_up_cb == NULL) || (power_down_cb == NULL))
	{
		return ERV_PARAM_INVALID;
	}
	
	edna_lib_must_cancel = false;
	
	while (!edna_lib_must_cancel)
	{
		fd_set daemon_fds;
	
		do
		{
			FD_ZERO(&daemon_fds);
			FD_SET(daemon_socket, &daemon_fds);
	
			struct timeval timeout = { 0, 10000 }; // 10ms
			
			select(FD_SETSIZE, &daemon_fds, NULL, NULL, &timeout);
		}
		while (!FD_ISSET(daemon_socket, &daemon_fds) && !edna_lib_must_cancel);
		
		if (edna_lib_must_cancel) break;
		
		/* Receive a command from the daemon */
		std::vector<unsigned char> cmd;
		
		if ((recv_from_daemon(cmd) != 0) || (cmd.size() < 1))
		{
			close(daemon_socket);
			
			daemon_socket = -1;
			edna_lib_connected = false;
			
			return ERV_DISCONNECTED;
		}
		
		/* Perform processing based on the type of command */
		std::vector<unsigned char> rsp;
		
		switch(cmd[0])
		{
		case POWER_UP:
			(power_up_cb)();
			rsp.push_back(EDNA_OK);
			break;
		case POWER_DOWN:
			(power_down_cb)();
			rsp.push_back(EDNA_OK);
			break;
		case TRANSCEIVE_APDU:
			{
				std::vector<unsigned char> r_apdu;
				
				r_apdu.resize(512);
				size_t rdata_len = 512;
				
				(process_cb)(&cmd[1], cmd.size() - 1, &r_apdu[0], &rdata_len);
				
				r_apdu.resize(rdata_len);
				
				rsp.resize(r_apdu.size() + 1);
				
				rsp[0] = EDNA_OK;
				
				memcpy(&rsp[1], &r_apdu[0], r_apdu.size());
			}
			break;
		default:
			rsp.push_back(UNKNOWN_COMMAND);
			break;
		}
		
		/* Transmit the response data to the daemon */
		if (send_to_daemon(rsp) != 0)
		{
			close(daemon_socket);
			
			daemon_socket = -1;
			edna_lib_connected = false;
			
			return ERV_DISCONNECTED;
		}
	}
	
	edna_lib_must_cancel = false;
	
	return ERV_OK;
}

void edna_lib_cancel(void)
{
	edna_lib_must_cancel = true;
}
