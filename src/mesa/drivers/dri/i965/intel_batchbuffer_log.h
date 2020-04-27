#ifndef INTEL_BATCHBUFFER_LOG_H
#define INTEL_BATCHBUFFER_LOG_H

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "callstack.h"

#ifndef HAVE_ANDROID_PLATFORM
#define HAVE_ANDROID_PLATFORM
#endif
#ifdef HAVE_ANDROID_PLATFORM
#define LOG_TAG "INTEL-MESA"
#if ANDROID_API_LEVEL >= 26
#include <log/log.h>
#else
#include <cutils/log.h>
#endif /* use log/log.h start from android 8 major version */
#ifndef ALOGW
#define ALOGW LOGW
#endif
#define dbg_printf(...)	ALOGW(__VA_ARGS__)
#else
#define dbg_printf(...)	fprintf(stderr, __VA_ARGS__)
#endif /* HAVE_ANDROID_PLATFORM */

#undef HAVE_ANDROID_PLATFORM

#define DBG(...) do {						\
	if (unlikely(INTEL_DEBUG & FILE_DEBUG_FLAG))		\
		dbg_printf(__VA_ARGS__);			\
} while(0)


extern FILE *create_log_file();
extern void close_log_file(FILE*);

#endif

