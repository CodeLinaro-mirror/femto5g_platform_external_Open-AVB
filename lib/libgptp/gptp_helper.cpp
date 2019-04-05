/*===========================================================================
Copyright (c) 2019, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Copyright (c) 2012-2015, Symphony Teleca Corporation, a Harman International Industries, Incorporated company
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS LISTED "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS LISTED BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Attributions: The inih library portion of the source code is licensed from
Brush Technology and Ben Hoyt - Copyright (c) 2009, Brush Technology and Copyright (c) 2009, Ben Hoyt.
Complete license and copyright information can be found at
https://github.com/benhoyt/inih/commit/74d2ca064fb293bc60a77b0bd068075b293cf175.
============================================================================ */
#include <linux/ptp_clock.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>           /* For O_* constants */
#include <linux_ipc.hpp>
#include "gptp_helper.h"

pthread_mutex_t gInitMutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK()  	pthread_mutex_lock(&gInitMutex)
#define UNLOCK()	pthread_mutex_unlock(&gInitMutex)

bool bInitialized = false;
int gPtpShmFd = -1;
char *gPtpMmap = NULL;
gPtpTimeData gPtpTD;

/* gptp core function to init gptp scaling */
static int gptpMemInit(int *gptp_shm_fd, char **gptp_mmap)
{
	if (NULL == gptp_shm_fd)
		return false;
#ifdef ANDROID
	*gptp_shm_fd = open( SHM_NAME, O_RDWR | O_CREAT, 0);
#else
	*gptp_shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
#endif

	if (*gptp_shm_fd == -1) {
		perror("shm_open()");
		return false;
	}

	*gptp_mmap =
	    (char *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			 *gptp_shm_fd, 0);

	if (*gptp_mmap == (char *)-1) {
		perror("mmap()");
		*gptp_mmap = NULL;
#ifdef ANDROID
		close(*gptp_shm_fd);
		unlink(SHM_NAME );
#else
		shm_unlink(SHM_NAME);
#endif
		return false;
	}
	return true;
}

/* gptp core function to deinit gptp scaling */
static void gptpMemDeinit(int gptp_shm_fd, char *gptp_mmap)
{
	if (gptp_mmap != NULL)
		munmap(gptp_mmap, SHM_SIZE);

	if (gptp_shm_fd != -1)
		close(gptp_shm_fd);
}

/* gptp core function to copy gptp offset data from shared memory */
static int gptpScaling(gPtpTimeData * td, char *memory_offset_buffer)
{
	if (td == NULL)
		return false;

	pthread_mutex_lock((pthread_mutex_t *) memory_offset_buffer);
	memcpy(td, memory_offset_buffer + sizeof(pthread_mutex_t), sizeof(*td));
	pthread_mutex_unlock((pthread_mutex_t *) memory_offset_buffer);

	return true;
}

/* gptp core function query gptp time */
static bool gptpLocalTime(const gPtpTimeData *td, uint64_t *now_local, uint64_t *time_sys_ns)
{
	uint64_t system_time = 0;
	int64_t delta_local = 0;
	int64_t delta_system = 0;

	if (!td || !now_local || !time_sys_ns)
		return false;

	system_time = td->local_time + td->ls_phoffset;
	delta_system = *time_sys_ns - system_time;
	delta_local = td->ls_freqoffset * delta_system;

	/* now_local contains the scaled gptp local time
	corresponding to system time */
	*now_local = td->local_time + delta_local;

	return true;
}


/* gptp core function query gptp time */
static bool gptpLocalQTime(const gPtpTimeData *td, uint64_t *now_local, uint64_t *time_qtime_ns)
{
	uint64_t qtimer_time = 0;
	int64_t delta_local = 0;
	int64_t delta_qtimer = 0;

	if (!td || !now_local || !time_qtime_ns)
		return false;

	qtimer_time = td->local_time + td->lq_phoffset;
	delta_qtimer = *time_qtime_ns - qtimer_time;
	delta_local = td->lq_freqoffset * delta_qtimer;

	/* now_local contains the scaled gptp local time
	corresponding to qtimer time*/
	*now_local = td->local_time + delta_local;

	return true;
}


/* intermediate wrapper to init gptp scaling */
static bool gptpTimeInit(void) {
	if (!gptpMemInit(&gPtpShmFd, &gPtpMmap))
		return false;

	if (!gptpScaling(&gPtpTD, gPtpMmap))
		return false;

	return true;
}


/* public API to query gptp time */
bool gptpGetPtpTimeFromMonoTime(uint64_t *gptp_time_sys, uint64_t time_mono_ns) {
	uint64_t now_local = 0;
	uint64_t update_8021as = 0;
	int64_t delta_8021as = 0;
	int64_t delta_local = 0;
	uint64_t time_mono_qtime_ns  = 0;

	if (!gptpScaling(&gPtpTD, gPtpMmap))
		return false;

	if (gPtpTD.port_state == PTP_SLAVE) {
		if (gPtpTD.sync_status == false) {
			return false;
		}
	}

	time_mono_qtime_ns =  time_mono_ns + gPtpTD.qtime_to_mono_offset; //Qtimer is ahead from monotonic

	if (gptpLocalQTime(&gPtpTD, &now_local, &time_mono_qtime_ns)) {
		update_8021as = gPtpTD.local_time - gPtpTD.ml_phoffset;
		delta_local = now_local - gPtpTD.local_time;
		delta_8021as = gPtpTD.ml_freqoffset * delta_local;
		*gptp_time_sys = update_8021as + delta_8021as;
		return true;
	}
	return false;
}

/* public API to query gptp time */
bool gptpGetPtpTimeFromQTimeNs(uint64_t *gptp_time_qt, uint64_t time_qtimer_ns) {
	uint64_t now_local = 0;
	uint64_t update_8021as = 0;
	int64_t delta_8021as = 0;
	int64_t delta_local = 0;
	uint64_t time_ns = time_qtimer_ns;

	if (!gptpScaling(&gPtpTD, gPtpMmap))
		return false;

	if (gPtpTD.port_state == PTP_SLAVE) {
		if (gPtpTD.sync_status == false) {
			return false;
		}
	}

	if (gptpLocalQTime(&gPtpTD, &now_local, &time_ns)) {
		update_8021as = gPtpTD.local_time - gPtpTD.ml_phoffset;
		delta_local = now_local - gPtpTD.local_time;
		delta_8021as = gPtpTD.ml_freqoffset * delta_local;
		*gptp_time_qt = update_8021as + delta_8021as;
		return true;
	}
	return false;
}

bool gptpGetPtpTimeFromQTimeTickCount(uint64_t *gptp_time_sys, uint64_t qtime_ticks)  {

	bool ret = false;
	uint64_t qTimerFreq = 0, qtimer_sec = 0, qtimer_nanos_NSec = 0, time_qtimer_ns = 0;
#if __aarch64__
		asm volatile("mrs %0, cntfrq_el0" : "=r"(qTimerFreq));
#else
		qTimerFreq = 19200000; //19.2 MHz TBD: find right asm instruction
#endif

	qtimer_sec = (qtime_ticks / qTimerFreq);
	qtimer_nanos_NSec = (qtime_ticks % qTimerFreq);
	qtimer_nanos_NSec *= 1000000000;
	qtimer_nanos_NSec /= qTimerFreq;

	time_qtimer_ns = qtimer_sec * 1000000000 + qtimer_nanos_NSec;
	ret = gptpGetPtpTimeFromQTimeNs(gptp_time_sys, time_qtimer_ns);
	return ret;

}

/* public API to query gptp time */
bool gptpGetTime(uint64_t *gptp_time_sys, uint64_t time_sys_ns) {
	uint64_t now_local = 0;
	uint64_t update_8021as = 0;
	int64_t delta_8021as = 0;
	int64_t delta_local = 0;
	uint64_t time_ns = time_sys_ns;

	if (!gptpScaling(&gPtpTD, gPtpMmap))
		return false;

	if (gPtpTD.port_state == PTP_SLAVE) {
		if (gPtpTD.sync_status == false) {
			return false;
		}
	}

	if (gptpLocalTime(&gPtpTD, &now_local, &time_ns)) {
		update_8021as = gPtpTD.local_time - gPtpTD.ml_phoffset;
		delta_local = now_local - gPtpTD.local_time;
		delta_8021as = gPtpTD.ml_freqoffset * delta_local;
		*gptp_time_sys = update_8021as + delta_8021as;
		return true;
	}
	return false;
}

/* public API to init gptp time scaling */
bool gptpInit(void) {
	LOCK();
	if (!bInitialized) {
		if (gptpTimeInit())
			bInitialized = true;
	}
	UNLOCK();
	return bInitialized;
}

/* public API to deinit gptp time scaling */
bool gptpDeinit(void) {
	gptpMemDeinit(gPtpShmFd, gPtpMmap);
	return true;
}
