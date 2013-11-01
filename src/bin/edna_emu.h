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

#ifndef _EDNA_EMU_H
#define _EDNA_EMU_H

#include "config.h"
#include "edna_comm.h"
#include <winscard.h>

class edna_emulator
{
public:
	/**
	 * Constructor
	 * @param comm_thread pointer to the communications thread
	 */
	edna_emulator(edna_comm_thread* comm_thread);
	
	/**
	 * Destructor
	 */
	~edna_emulator();
	
	/**
	 * Run the emulator
	 */
	void run();
	
	/**
	 * Cancel command
	 */
	void cancel();
		
private:
	/**
	 * Exchange a control command
	 * @param reader Handle to the reader
	 * @param cmd Control command to send
	 * @param rdata Return data
	 * @return true if the control command was exchange successfully
	 */
	bool transceive_control(SCARDHANDLE reader, const bytestring& cmd, bytestring& rdata);

	edna_comm_thread* comm_thread;
	bool should_cancel;
	SCARDCONTEXT pcsc_context;
	int cmd_delay;
	bool delay_success_only;
};

#endif /* !_EDNA_EMU_H */
