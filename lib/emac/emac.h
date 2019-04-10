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
*//*========================================================================*/

#ifndef EMAC_H
#define EMAC_H

#include "neutrino.h"
#include <sys/epoll.h>

/**
 * @brief PPS
 */

#define DWC_ETH_QOS_RWK_FILTER_LENGTH   8
#define DWC_ETH_QOS_CONFIG_PPSOUT_CMD 44
#define DWC_ETH_QOS_PRV_IOCTL    SIOCDEVPRIVATE

/* Maximum number of events to be returned from
   a single epoll_wait() call */
#define MAX_EVENTS     2
/* Maximum number of FDs based on streams */
#define MAX_STREAM_FD     24
#define MAX_PER_STREAM_FD     3

#define SR_CLASS_A 0
#define SR_CLASS_B 1

#define CLASS_A_WAKERATE      8000
#define CLASS_B_WAKERATE      4000


// pps default values
#define PPS_DEFAULT_PTP_FREQUENCY 0 // let driver decide the Default Freq.
#define PPS_DEFAULT_DUTYCYCLE 50 // default duty cycle

#define PPS_START 1
#define PPS_STOP  0
#define PPS_DEFAULT_OP_FREQUENCY 1000  // 1ms of events
#define TIMEOUT 1
#define PPSCH2_CLASS_A_DEV "/dev/avb_class_a_intr" // ch2 : class A interrupt
#define PPSCH3_CLASS_B_DEV "/dev/avb_class_b_intr" // ch3 : class B interrupt

/* pps channels */
typedef enum pps_channels {
	PPS_CH_0 = 0, // used for generating 19.2Mhz Freq. for Audio
	PPS_CH_1,     // used for AVTP timestamps
	PPS_CH_2,	  // Class A interrupts notification
	PPS_CH_3,	  // Class b interrupts notification
	MAX_PPS_CH
}pps_channels_t;

/* pps err codes */
typedef enum pps_error_code {
	ERR_SOCK,
	ERR_ENOTTY,
	ERR_EOPNOTSUPP,
	ERR_EIOCTL,
	PPS_SUCCESS,
	PPS_FAILURE,
	PPS_POLL_TIMEOUT,
	PPS_POLL_ERR,
	PPS_POLL_SUCCESS,
	PPS_POLL_FAILURE
}pps_err_code_t;

/* ETH PPS structure */
typedef struct ETH_PPS_Config {
	unsigned int ptpclk_freq;
	unsigned int ppsout_freq;
	unsigned int ppsout_ch;
	unsigned int ppsout_duty;
	unsigned int ppsout_start;
}ETH_PPS_Config_t;

/* Local PPS database structure */
typedef struct eventConfigData {
	char *ifacename;
	uint8_t sr_class;
	int pps_fd;
	int pps_epoll_fd;
	unsigned long wakeRate;
	unsigned int pkts_per_wake;
	struct epoll_event ev;
	ETH_PPS_Config_t ETH_PPS_Config;
}eventConfigData_t;

/* pps functions declarations */
bool eventConfigure(eventConfigData_t *);
bool eventStop(eventConfigData_t *);
bool eventWake (eventConfigData_t *);
bool eventInit(eventConfigData_t *);
bool epoll_ctrl (eventConfigData_t *);
#endif // EMAC_H
