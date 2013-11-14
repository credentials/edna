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
 * Card emulator class
 */

#include "config.h"
#include "edna_config.h"
#include "edna_emu.h"
#include "edna_log.h"
#include <winscard.h>
#include <reader.h>
#include <unistd.h>

#define DEFAULT_ATQ					0x0004
#define DEFAULT_SAK					0x28
#define IOCTL_CCID_ESCAPE_DIRECT	SCARD_CTL_CODE(1)

edna_emulator::edna_emulator(edna_comm_thread* comm_thread)
{
	this->comm_thread = comm_thread;
	should_cancel = false;
	
	pcsc_context = 0;
	
	edna_conf_get_int("emulation", "cmd_delay", cmd_delay, 0);
	
	DEBUG_MSG("Setting delay between C-APDU and R-APDU to %dms", cmd_delay);
	
	edna_conf_get_bool("emulation", "delay_success_only", delay_success_only, false);
	
	if (delay_success_only)
	{
		DEBUG_MSG("Only delaying successful commands");
	}
}
	
edna_emulator::~edna_emulator()
{
}

bool edna_emulator::transceive_control(SCARDHANDLE reader, const bytestring& cmd, bytestring& rdata)
{
	DWORD pcsc_rv;
	DWORD rlen;
	
	rdata.resize(512);
	if ((pcsc_rv = SCardControl(reader, IOCTL_CCID_ESCAPE_DIRECT, cmd.const_byte_str(), cmd.size(), rdata.byte_str(), 512, &rlen)) != SCARD_S_SUCCESS)
	{
		ERROR_MSG("Failed to exchange control command (0x%08X)", pcsc_rv);
		
		SCardDisconnect(reader, SCARD_UNPOWER_CARD);
		SCardReleaseContext(pcsc_context);
		
		return false;
	}
	
	rdata.resize(rlen);
	
	return true;
}
	
void edna_emulator::run()
{
	/* Determine the card reader to connect to */
	std::string reader;
	
	if ((edna_conf_get_string("emulation", "reader", reader, NULL) != ERV_OK) || reader.empty())
	{
		ERROR_MSG("No smart card reader configured, giving up!");
		
		return;
	}
	
	/* Read emulation parameters from the configuration */
	unsigned short atq;
	unsigned char sak;
	int conf_val;
	
	if (edna_conf_get_int("emulation", "atq", conf_val, DEFAULT_ATQ) != ERV_OK)
	{
		ERROR_MSG("Error reading ATQ from configuration file");
		
		return;
	}
	
	atq = (unsigned short) conf_val;
	
	if (edna_conf_get_int("emulation", "sak", conf_val, DEFAULT_SAK) != ERV_OK)
	{
		ERROR_MSG("Error reading SAK from configuration file");
		
		return;
	}
	
	sak = (unsigned char) conf_val;
	
	/* Establish PC/SC context and connect to the card reader */
	SCARDHANDLE pcsc_reader;
	DWORD pcsc_rv;
	DWORD active_protocol;
	
	if ((pcsc_rv = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &pcsc_context)) != SCARD_S_SUCCESS)
	{
		ERROR_MSG("Failed to establish a PC/SC context, giving up!");
		
		return;
	}
	
	if ((pcsc_rv = SCardConnect(pcsc_context, reader.c_str(), SCARD_SHARE_DIRECT, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &pcsc_reader, &active_protocol)) != SCARD_S_SUCCESS)
	{
		ERROR_MSG("Failed to connect to PC/SC reader %s, giving up!", reader.c_str());
		
		SCardReleaseContext(pcsc_context);
		
		return;
	}
	
	INFO_MSG("Connected to PC/SC reader %s", reader.c_str());
	
	/* Switch the reader to emulation mode */
	bytestring 	start_emu 	= "83100100";
	bytestring	set_atq_sak	= "588de3";
	bytestring	buzzer_off	= "588dcc00";
	bytestring 	end_emu 	= "83100000";
	bytestring	rdata;
	DWORD		rlen 		= 0;
	
	rdata.resize(512);
	
	set_atq_sak += atq;
	set_atq_sak += sak;
	
	INFO_MSG("Setting emulator card ATQ to 0x%04X and SAK to 0x%02X", atq, sak);
	
	if (!transceive_control(pcsc_reader, set_atq_sak, rdata)) return;
	
	INFO_MSG("Disabling reader buzzer");
	
	if (!transceive_control(pcsc_reader, buzzer_off, rdata)) return;
	
	INFO_MSG("Entering emulation mode on reader %s", reader.c_str());
	
	if (!transceive_control(pcsc_reader, start_emu, rdata)) return;
	
	/* Main emulation loop */
	while(!should_cancel)
	{
		/* Wait for an event on the reader; by default we wait 100ms per call */
		bytestring	wait_event	= "83000064";
		bool		aborted		= false;
		
		do
		{
			if (!transceive_control(pcsc_reader, wait_event, rdata))
			{
				aborted = true;
				should_cancel = true;
				break;
			}
			
			if (rdata.size() != 3) continue;
		}
		while ((rdata[1] == 0x00) && !should_cancel);
		
		if (aborted || should_cancel) break;
		
		switch(rdata[1])
		{
		case 0x01:	/* ISO 14443A SELECT */
			DEBUG_MSG("ISO 14443A SELECT event received on reader %s", reader.c_str());
			
			INFO_MSG("Sending POWER UP to running emulations");
			
			comm_thread->powerup_on_select();
			break;
		case 0x04:	/* ISO 14443A DESELECT */
			INFO_MSG("ISO 14443A DESELECT event received on reader %s", reader.c_str());
			
			if (comm_thread->application_selected())
			{
				INFO_MSG("Sending POWER DOWN to running emulations");
				
				comm_thread->powerdown_on_deselect();
				
				INFO_MSG("Sleeping 2 seconds then resetting emulation on reader");
				sleep(2);
				
				// Reset the emulation in the hope that this makes
				// everything more stable
				if (!transceive_control(pcsc_reader, end_emu, rdata)) return;
				if (!transceive_control(pcsc_reader, set_atq_sak, rdata)) return;
				if (!transceive_control(pcsc_reader, buzzer_off, rdata)) return;
				if (!transceive_control(pcsc_reader, start_emu, rdata)) return;
				
				INFO_MSG("Emulation successfully reset");
			}
			else
			{
				INFO_MSG("Sending POWER DOWN to running emulations");
				
				comm_thread->powerdown_on_deselect();
			}
			break;
		case 0x02:	/* C-APDU received */
			{
				/* Retrieve the C-APDU */
				bytestring get_capdu = "84";
				
				if (!transceive_control(pcsc_reader, get_capdu, rdata))
				{
					aborted = true;
					should_cancel = true;
					break;
				}
				
				/* Check status byte */
				if ((rdata.size() > 0) && (rdata[0] != 0x00))
				{
					switch(rdata[0])
					{
					case 0x03:	// No C-APDU available
						continue;
					case 0x13:	// FIFO overflow
						ERROR_MSG("Card reader received APDU exceeding 280 bytes");
						continue;
					case 0x3B:	// Wrong mode
						ERROR_MSG("Card reader is in wrong mode, aborting");
						aborted = true;
						should_cancel = true;
						continue;
					case 0x3C:	// Wrong parameter
						ERROR_MSG("Card reader reported wrong parameter set, aborting");
						aborted = true;
						should_cancel = true;
						continue;
					case 0x70:	// Buffer overflow
						ERROR_MSG("Reader internal buffer overflow");
						continue;
					case 0x7D:	// Wrong length
						ERROR_MSG("Reader reports wrong length");
						continue;
					}
				}
				
				/* Cut off status byte */
				rdata = rdata.substr(1);
				
				bytestring send_to_ifd;
				
				//DEBUG_MSG("--> %s", rdata.hex_str().c_str());
				
				if (!comm_thread->transceive(rdata, send_to_ifd))
				{
					ERROR_MSG("Failed to exchange data with communications thread!");
					
					send_to_ifd = "6f00";
				}
				
				//DEBUG_MSG("<-- %s", send_to_ifd.hex_str().c_str());
				
				if (cmd_delay)
				{
					if (!delay_success_only || (send_to_ifd.substr(send_to_ifd.size() - 2) == "9000"))
					{
						DEBUG_MSG("Delaying %dms", cmd_delay);
						
						usleep(cmd_delay * 1000);
					}
				}
				
				bytestring send_rapdu = "84";
				send_rapdu += send_to_ifd;
				
				if (!transceive_control(pcsc_reader, send_rapdu, rdata))
				{
					aborted = true;
					should_cancel = true;
					break;
				}
				
				rdata.resize(rlen);
			}
			break;
		case 0x03:	/* R-APDU processing complete */
			DEBUG_MSG("R-APDU processing complete on reader %s", reader.c_str());
			break;
		}
	}
	
	/* Leave emulation mode */
	INFO_MSG("Leaving emulation mode on reader %s", reader.c_str());
	
	if (!transceive_control(pcsc_reader, end_emu, rdata)) return;
	
	/* Disconnect from reader and release PC/SC context */
	SCardDisconnect(pcsc_reader, SCARD_UNPOWER_CARD);
	
	SCardReleaseContext(pcsc_context);
	
	INFO_MSG("Disconnected from PC/SC reader %s, ending emulation", reader.c_str());
}

void edna_emulator::cancel()
{
	INFO_MSG("Canceling emulation");
	
	should_cancel = true;
	
	SCardCancel(pcsc_context);
}
