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


#include <sys/select.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netpacket/packet.h>
#include <errno.h>
#include <linux/ethtool.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <linux/net_tstamp.h>
#include <linux/ptp_clock.h>
// PPS
#include <arpa/inet.h>
#include "emac.h"

#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#define strlcat g_strlcat
#endif

/* save fds here */
int fdlist[MAX_STREAM_FD] = {-1};
int fdcount = 0;

bool eventConfigure(eventConfigData_t *eventData){
	int err;
	struct ifreq ifreq;
	struct ifr_data_struct data;

	if (eventData->pps_fd > 0) {
		syslog(LOG_DEBUG,"FD already exists, %d",eventData->pps_fd);
		return true;
	}

	memset(&ifreq, 0, sizeof(ifreq));
	memset(&data, 0, sizeof(data));

	if (eventData->sr_class == SR_CLASS_A) {
		eventData->ETH_PPS_Config.ppsout_ch = PPS_CH_2;
	}
	else if (eventData->sr_class == SR_CLASS_B) {
		eventData->ETH_PPS_Config.ppsout_ch = PPS_CH_3;
	}

	if (eventData->ETH_PPS_Config.ppsout_ch >= MAX_PPS_CH) {
		syslog(LOG_DEBUG,"channel not supported");
		return false;
	}
	else if (eventData->wakeRate > CLASS_A_WAKERATE) {
		syslog(LOG_DEBUG,"wakerate not supported");
		return false;
	}

	eventData->ETH_PPS_Config.ppsout_freq = eventData->wakeRate;
	eventData->ETH_PPS_Config.ptpclk_freq = PPS_DEFAULT_PTP_FREQUENCY;
	eventData->ETH_PPS_Config.ppsout_start = PPS_START;
	eventData->ETH_PPS_Config.ppsout_duty = PPS_DEFAULT_DUTYCYCLE;

	snprintf(ifreq.ifr_ifrn.ifrn_name, sizeof(ifreq.ifr_ifrn.ifrn_name), "%s", eventData->ifacename);
	data.ptr = (void*)&(eventData->ETH_PPS_Config);
	data.cmd = DWC_ETH_QOS_CONFIG_PPSOUT_CMD;
	ifreq.ifr_ifru.ifru_data = (char*) &data;

	eventData->pps_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (eventData->pps_fd < 0) {
		syslog(LOG_DEBUG,"socket creation failed for PPS");
		return false;
	}

	err = ioctl(eventData->pps_fd, DWC_ETH_QOS_PRV_IOCTL, &ifreq);
	if (err < 0) {
		err = errno;
		if (err == ENOTTY) {
			syslog(LOG_DEBUG,
					"Kernel does not have support "
					"for non-destructive DWC_ETH_QOS_PRV_IOCTL");
			close(eventData->pps_fd);
			eventData->pps_fd = -1;
			return false;
		}
		else if (err == EOPNOTSUPP) {
			syslog(LOG_DEBUG,
					"Device driver does not have support "
					"for provided configurations");
			close(eventData->pps_fd);
			eventData->pps_fd = -1;
			return false;
		}
		else {
			syslog(LOG_DEBUG,"DWC_ETH_QOS_PRV_IOCTL failed");
			close(eventData->pps_fd);
			eventData->pps_fd = -1;
			return false;
		}
	}
	/* save fds for later */
	fdlist[fdcount++] = eventData->pps_fd;

	return true;
}

bool eventStop(eventConfigData_t *eventData) {
	int err;
	struct ifreq ifreq;
	struct ifr_data_struct data;
	memset(&ifreq, 0, sizeof(ifreq));
	memset(&data, 0, sizeof(data));

	if (fdcount > MAX_PER_STREAM_FD) {
		syslog(LOG_DEBUG,"FDs getting closed, current FD count is %d",fdcount);
		fdcount -= MAX_PER_STREAM_FD;
		if ( eventData->ifacename != NULL ) {
			free(eventData->ifacename);
			eventData->ifacename = NULL;
		}

		return true;
	}

	eventData->ETH_PPS_Config.ppsout_freq = 0;
	eventData->ETH_PPS_Config.ppsout_start = PPS_STOP;

	snprintf(ifreq.ifr_ifrn.ifrn_name, sizeof(ifreq.ifr_ifrn.ifrn_name), "%s", eventData->ifacename);
	data.ptr = (void*)&(eventData->ETH_PPS_Config);
	data.cmd = DWC_ETH_QOS_CONFIG_PPSOUT_CMD;
	ifreq.ifr_data = (char *)&data;

	if (eventData->pps_fd < 0) {
		syslog(LOG_DEBUG,"socket does not exists");
		if ( eventData->ifacename != NULL ) {
			free(eventData->ifacename);
			eventData->ifacename = NULL;
		}

		return false;
	}

	err = ioctl(eventData->pps_fd, DWC_ETH_QOS_PRV_IOCTL, &ifreq);
	if (err < 0) {
		err = errno;
		if (err == ENOTTY) {
			syslog(LOG_DEBUG,
					"Kernel does not have support "
					"for non-destructive DWC_ETH_QOS_PRV_IOCTL");
			close(eventData->pps_fd);
			eventData->pps_fd = -1;
			if ( eventData->ifacename != NULL ) {
				free(eventData->ifacename);
				eventData->ifacename = NULL;
			}

			return false;
		}
		else if (err == EOPNOTSUPP) {
			syslog(LOG_DEBUG,
					"Device driver does not have support "
					"for provided configurations");
			close(eventData->pps_fd);
			eventData->pps_fd = -1;
			if ( eventData->ifacename != NULL ) {
				free(eventData->ifacename);
				eventData->ifacename = NULL;
			}

			return false;
		}
		else {
			syslog(LOG_DEBUG,"DWC_ETH_QOS_PRV_IOCTL failed");
			close(eventData->pps_fd);
			eventData->pps_fd = -1;
			if ( eventData->ifacename != NULL ) {
				free(eventData->ifacename);
				eventData->ifacename = NULL;
			}

			return false;
		}
	}

	/* close additional FDs open for epoll */
	for (int i = 0; i < fdcount; i++) {
		if (fdlist[i] <= 0) {
			break;
		}
		else {
			close(fdlist[i]);
		}
	}

	fdcount = 0;
	syslog(LOG_DEBUG,"All FDs closed, FD count is %d",fdcount);
	if ( eventData->ifacename != NULL ) {
		free(eventData->ifacename);
		eventData->ifacename = NULL;
	}

	return true;
}

bool epoll_ctrl (eventConfigData_t *eventData) {
	if (epoll_ctl(eventData->pps_epoll_fd, EPOLL_CTL_ADD, eventData->ev.data.fd, &eventData->ev) == -1) {
		syslog(LOG_DEBUG,"epoll_ctl error, unable to set ctrl on device !");
		return false;
	}
	return true;
}

bool eventInit(eventConfigData_t *eventData) {
	if (eventData->pps_epoll_fd <= 0) {
		eventData->ev.events = POLLIN;
		eventData->pps_epoll_fd = epoll_create1(0);
	}

	if (eventData->pps_epoll_fd < 0) {
		syslog(LOG_DEBUG,"epoll_create error, unable to create epoll !");
		close(eventData->pps_epoll_fd);
		eventData->pps_epoll_fd = -1;
		return false;
	}

	if (eventData->ETH_PPS_Config.ppsout_ch == PPS_CH_2) {
		eventData->ev.data.fd = open(PPSCH2_CLASS_A_DEV, O_RDONLY);
		if (eventData->ev.data.fd < 0) {
			syslog(LOG_DEBUG,"unable to open channel !");
			close(eventData->pps_epoll_fd);
			close(eventData->ev.data.fd);
			eventData->ev.data.fd = -1;
			eventData->pps_epoll_fd = -1;
			return false;
		}
	}
	else if (eventData->ETH_PPS_Config.ppsout_ch == PPS_CH_3) {
		eventData->ev.data.fd = open(PPSCH3_CLASS_B_DEV, O_RDONLY);
		if (eventData->ev.data.fd < 0) {
			syslog(LOG_DEBUG,"unable to open channel !");
			close(eventData->pps_epoll_fd);
			close(eventData->ev.data.fd);
			eventData->pps_epoll_fd = -1;
			eventData->ev.data.fd = -1;
			return false;
		}
	}

	if (!epoll_ctrl(eventData)) {
		close(eventData->pps_epoll_fd);
		close(eventData->ev.data.fd);
		eventData->pps_epoll_fd = -1;
		eventData->ev.data.fd = -1;
	}

	/* save fds for later */
	fdlist[fdcount++] = eventData->pps_epoll_fd;
	fdlist[fdcount++] = eventData->ev.data.fd;

	return true;
}

bool eventWake (eventConfigData_t *eventData) {
	int ready;
	bool ret = false;

	if (eventData->pps_epoll_fd < 0) {
		syslog(LOG_DEBUG,"FD does not exists, returning from epoll");
		return false;
	}

	ready = epoll_wait(eventData->pps_epoll_fd, &eventData->ev, MAX_EVENTS, -1);

	if (ready == -1) {
		/* Restart if interrupted by signal */
		syslog(LOG_DEBUG,"FD not ready, eturning from epoll");
		ret = false;
	}

	/* Deal with returned list of events */
	if (eventData->ev.events & EPOLLIN) {
		ret = true;
	}
	return ret;
}
