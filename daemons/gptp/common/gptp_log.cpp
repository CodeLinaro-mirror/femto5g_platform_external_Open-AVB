/*************************************************************************************************************
Copyright (c) 2012-2016, Harman International Industries, Incorporated
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
*************************************************************************************************************/

/******************************************************************************

Changes from Qualcomm Innovation Center, Inc. are provided under the following license:

Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear

******************************************************************************/

#include <gptp_log.hpp>

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <platform.hpp>
#include <syslog.h>
#include <sys/syscall.h>
#include <unistd.h>
// MS VC++ 2013 has C++11 but not C11 support, use this to get millisecond resolution
#include <chrono>


#ifdef GENIVI_DLT
DLT_DECLARE_CONTEXT(dlt_con_gptp);
#endif

void gptplogRegister(void)
{
#ifdef GENIVI_DLT
    DLT_REGISTER_APP("GPTP", "OpenAVB gPTP");
    DLT_REGISTER_CONTEXT(dlt_con_gptp, "GNRL", "General Context");
#endif
}

void gptplogUnregister(void)
{
#ifdef GENIVI_DLT
    DLT_UNREGISTER_CONTEXT(dlt_con_gptp);
    DLT_UNREGISTER_APP();
#endif
}

// logcat support
#ifdef ANDROID
gptplogcat_t gptplogcat = GPTP_LOG_ON;
#else
gptplogcat_t gptplogcat = GPTP_LOG_OFF;
#endif

gptplogcat_t systemlogcat = GPTP_LOG_ON;

#define gettid() syscall(SYS_gettid)

void gptpLog(GPTP_LOG_LEVEL level, const char *tag, const char *path, int line,
             const char *fmt, ...)
{
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
#ifdef ANDROID

    if (gptplogcat) {
        LOGE(level, "[%s:%d] %s", path, line, msg);
    }

#else

    if (systemlogcat) {
        syslog(level, "[%d:%s:%d] %s\n", gettid(), path, line, msg);
    }

#endif
    else {
        std::chrono::system_clock::time_point cNow = std::chrono::system_clock::now();
        time_t tNow = std::chrono::system_clock::to_time_t(cNow);
        struct tm tmNow;
        PLAT_localtime(&tNow, &tmNow);
        std::chrono::system_clock::duration roundNow = cNow -
                std::chrono::system_clock::from_time_t(tNow);
        long int millis = (long int)
                          std::chrono::duration_cast<std::chrono::milliseconds>(roundNow).count();

        fprintf(stderr, "%s:GPTP:[%2.2d:%2.2d:%2.2d:%3.3ld] [%d:%s:%d] %s\n",
                tag, tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec, millis, gettid(), path, line,
                msg);
    }
}

void gptpLogMs(GPTP_LOG_LEVEL level, const char *tag, const char *path,
               int line,
               const char *fmt, ...)
{
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
#ifdef ANDROID

    if (gptplogcat) {
        LOGE(level, "[%s:%d] %s", path, line, msg);
    }

#else

    if (systemlogcat) {
        std::chrono::system_clock::time_point cNow = std::chrono::system_clock::now();
        time_t tNow = std::chrono::system_clock::to_time_t(cNow);
        struct tm tmNow;
        PLAT_localtime(&tNow, &tmNow);
        std::chrono::system_clock::duration roundNow = cNow -
                std::chrono::system_clock::from_time_t(tNow);
        long int millis = (long int)
                          std::chrono::duration_cast<std::chrono::milliseconds>(roundNow).count();

        syslog(level, "[%2.2d:%2.2d:%2.2d:%3.3ld] [%d:%s:%d] %s\n", tag,
               tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec, millis, gettid(), path, line, msg);
    }

#endif
    else {
        std::chrono::system_clock::time_point cNow = std::chrono::system_clock::now();
        time_t tNow = std::chrono::system_clock::to_time_t(cNow);
        struct tm tmNow;
        PLAT_localtime(&tNow, &tmNow);
        std::chrono::system_clock::duration roundNow = cNow -
                std::chrono::system_clock::from_time_t(tNow);
        long int millis = (long int)
                          std::chrono::duration_cast<std::chrono::milliseconds>(roundNow).count();

        fprintf(stderr, "%s:GPTP:[%2.2d:%2.2d:%2.2d:%3.3ld] [%d:%s:%d] %s\n",
                tag, tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec, millis, gettid(), path, line,
                msg);
    }
}

