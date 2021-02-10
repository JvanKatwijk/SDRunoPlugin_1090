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
#ifndef	__HTTP_HANDLER_H
#define	__HTTP_HANDLER_H

#include	<thread>
#include	<atomic>
#include	<string>
class	SDRunoPlugin_1090;

class	httpHandler {
public:
	httpHandler	(SDRunoPlugin_1090 *, std::string);
	~httpHandler	();
void	start		();
void	stop		();
void	run		();

private:
	SDRunoPlugin_1090		*parent;
	std::string	mapFile;
	std::atomic<bool>	running;
	std::thread	threadHandle;
	std::string     theMap		(std::string fileName);
};

#endif
