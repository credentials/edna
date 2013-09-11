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
 * Thread class
 */

#include "config.h"
#include "edna_log.h"
#include "edna_thread.h"

void* entry(void* param)
{
	edna_thread* thread_instance = (edna_thread*) param;
	
	thread_instance->entrypoint();
	
	return NULL;
}

edna_thread::edna_thread()
{
	is_running = false;
}

edna_thread::~edna_thread()
{
	if (is_running) this->kill();
}

bool edna_thread::start()
{
	int rv = 0;
	
	if (is_running)
	{
		DEBUG_MSG("Attempt to start thread that is already running");
		
		return false;
	}
	
	if ((rv = pthread_create(&the_thread, NULL, &entry, this)) != 0)
	{
		ERROR_MSG("Failed to start thread (%d)", rv);
		
		return false;
	}
	
	return true;
}

void edna_thread::kill()
{
	if (is_running)
	{
		pthread_cancel(the_thread);
		pthread_join(the_thread, NULL);
	}
	
	is_running = false;
}

void edna_thread::entrypoint()
{
	is_running = true;
	
	DEBUG_MSG("Entering thread 0x%08X", pthread_self());
	
	this->threadproc();
	
	DEBUG_MSG("Leaving thread 0x%08X", pthread_self());
	
	is_running = false;
}

void edna_thread::waitexit()
{
	pthread_join(the_thread, NULL);
}
