#
/*
 *	SDRunoPlugin_1090 is based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *	The decoding - i.e. using 2400 rather than 2000 samples/millesecond
 *	is based on Oliver Jowett's work.
 *	The understanding of it all is largely due to
 *	Junzi Shun's "the 1090mhz riddle"
 *
 *	Copyright (C) 2018
 *	Jan van Katwijk (J.vanKatwijk@gmail.com)
 *	Lazy Chair Computing
 *
 *    SDRunoPlugin_1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDRunoPlugin_1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDRunoPlugin_1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef	__ADSB_CONSTANTS_H
#define	__ADSB_CONSTANTS_H

#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<stdint.h>
#include	<errno.h>
#include        "windows.h"


#define DATA_LEN             (16*16384)   /* 256k */

#define PREAMBLE_US 8       /* microseconds */
#define LONG_MSG_BITS 112
#define SHORT_MSG_BITS 56
#define FULL_LEN (PREAMBLE_US + LONG_MSG_BITS)

#define UNIT_FEET 0
#define UNIT_METERS 1

#define INTERACTIVE_ROWS 15               /* Rows on screen */
#define INTERACTIVE_TTL 60                /* TTL before being removed */

#define NOTUSED(V) ((void) V)

#define	CURRENT_VERSION	"0.8"

#define	NO_ERRORFIX	0
#define	NORMAL_ERRORFIX	1
#define	STRONG_ERRORFIX	2

#define WIN32_LEAN_AND_MEAN
static inline
int gettimeofday (struct timeval * tp, struct timezone * tzp) {
//	Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime (&system_time);
	SystemTimeToFileTime (&system_time, &file_time);
	time = ((uint64_t)file_time. dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp -> tv_sec = (long)((time - EPOCH) / 10000000L);
	tp -> tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}
static inline
long long mstime () {
struct timeval tv;
long long mst;
	gettimeofday (&tv, nullptr);
	mst	= ((long long)tv. tv_sec) * 1000;
	mst	+= tv. tv_usec / 1000;
	return mst;
}

static inline
int	messageLenByType (int type) {
        if (type == 16 || type == 17 ||
            type == 19 || type == 20 || type == 21)
           return LONG_MSG_BITS;
        else
           return SHORT_MSG_BITS;
}
#endif

