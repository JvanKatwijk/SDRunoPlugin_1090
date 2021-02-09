#
/*
 *      SDRunoPlugin_1090is based on qt-1090, which on its turn is based
 *	on thje dump1090 program
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

#ifndef	__AIRCRAFT_HANDLER_H
#define	__AIRCRAFT_HANDLER_H

#include	<string>
#include	<stdint.h>
#include	<string>

class	message;

/* Structure used to describe an aircraft in iteractive mode. */
class aircraft {
public:
		aircraft		(uint32_t);
		~aircraft		();
	aircraft	*next;		/* Next aircraft in our linked list. */
	uint32_t	addr;		// ICAO address 
	char	        hexaddr[7];	// Printable ICAO address 
	char	        flight[9];	// Flight number 
	int	        altitude;	//  Altitude 
	int             speed;		// Velocity computed (EW and NS)
	int	        track;		// Angle of flight. */
	time_t	        seen;		// Time at which the last packet was received. */
	long	        messages; 	// Number of Mode S messages received. 
/* Encoded latitude and longitude as extracted by odd and even
 * CPR encoded messages.
 */
	int	        odd_cprlat;
	int	        odd_cprlon;
	int	        even_cprlat;
	int	        even_cprlon;
	double	        lat, lon;	// Coordinated obtained from CPR encoded data. */
	long long	odd_cprtime, even_cprtime;
	void	        fillData	(message *mm);
	void	        decodeCPR	(double *, double *);
	std::string	showPlane (bool metric, time_t now);
	void		showPlaneonExit	(FILE *);
	std::string	toJson		();
//
	time_t		entryTime;
	double		lat_in, lon_in;
	int		altitude_in;
};

aircraft *interactiveReceiveData	(aircraft *, message *);
aircraft *removeStaleAircrafts		(aircraft *, int, FILE *);
std::string	showPlanes		(aircraft *, bool);
std::string	aircraftsToJson		(aircraft *);
#endif

