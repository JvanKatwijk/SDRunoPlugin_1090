#
/*
 *	SDRunoPlugin_1090 is based on qt-1090, which on its turn was
 *	based on dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
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
#ifndef	__ICAO_CACHE_H
#define	__ICAO_CACHE_H

#include	<stdint.h>
#include	<time.h>


#define ICAO_CACHE_LEN 1024 /* Power of two required. */
#define ICAO_CACHE_TTL 60   /* Time to live of cached addresses. */

class cacheDesc {
public:
	bool	inUse;
	uint32_t addr;
	uint32_t time;
	cacheDesc (void) {
	   addr = 0;
	   time = 0;
	}
};

class icaoCache {
public:
		icaoCache (void);
		~icaoCache (void);
	void	addRecentlySeenICAOAddr		(uint32_t addr);
	bool	ICAOAddressWasRecentlySeen	(uint32_t addr);
private:
	uint32_t ICAOCacheHashAddress		(uint32_t a);
	cacheDesc icao_cache [ICAO_CACHE_LEN];
};

#endif

