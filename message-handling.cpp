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
#define  _USE_MATH_DEFINES
#include        <math.h>
#include	"message-handling.h"
#include	"icao-cache.h"
#include	"adsb-constants.h"

static
int     computeIdentity (uint8_t *msg) {
int a,b,c,d;

        a = ((msg[3] & 0x80) >> 5) |
            ((msg[2] & 0x02) >> 0) |
            ((msg[2] & 0x08) >> 3);
        b = ((msg[3] & 0x02) << 1) |
            ((msg[3] & 0x08) >> 2) |
            ((msg[3] & 0x20) >> 5);
        c = ((msg[2] & 0x01) << 2) |
            ((msg[2] & 0x04) >> 1) |
            ((msg[2] & 0x10) >> 4);
        d = ((msg[3] & 0x01) << 2) |
            ((msg[3] & 0x04) >> 1) |
            ((msg[3] & 0x10) >> 4);
        return  a * 1000 + b * 100 + c * 10 + d;
}

/*
 *	Decode a raw Mode S message demodulated as a stream of bytes by
 *	detectModeS (), and split it into fields populating a message
 *	structure.
 */
	message::message (int fix_errors,
	                  icaoCache *icao_cache,
	                  uint8_t *msg_o) {
uint32_t crc2;

	this	-> icao_cache = icao_cache;
/* Work on our local copy */
	memcpy (msg, msg_o, LONG_MSG_BITS / 8);

	msgtype = msg [0] >>3;    /* Downlink Format */
	msgbits = messageLenByType (msgtype);

/* CRC is always the last three bytes. */
	crc = ((uint32_t)msg [(msgbits / 8) - 3] << 16) |
	      ((uint32_t)msg [(msgbits / 8) - 2] << 8) |
	       (uint32_t)msg [(msgbits / 8) - 1];
	crc2 = computeChecksum (msg, msgbits);

/*	Check CRC and fix single bit errors using the CRC when
 *	possible (DF 11 and 17).
 */
	errorbit = -1;  /* No error corrected */
	crcok = (crc == crc2);
//
//	fix errors - if any -
	if (!crcok && (fix_errors != NO_ERRORFIX) &&
	                        (msgtype == 11 || msgtype == 17)) {
	   errorbit = fixSingleBitErrors (msg, msgbits);
	   if (errorbit != -1) {
	      crc	= computeChecksum (msg, msgbits);
	      crcok	= true;
	   }
	   else
	   if ((fix_errors == STRONG_ERRORFIX) && (msgtype == 17)) {
	      errorbit = fixTwoBitsErrors (msg, msgbits);
	      if (errorbit != -1) {
	         crc	= computeChecksum (msg, msgbits);
	         crcok	= true;
	      }
	   }
	}
/*
 *	Note that most of the other computation happens *after* we fix
 *	the single bit errors, otherwise we would need to recompute the
 *	fields again.
 */
	ca = msg [0] & 7;        /* Responder capabilities. */

/*	ICAO address */
	aa1 = msg [1];
	aa2 = msg [2];
	aa3 = msg [3];


/*	DF 17 type (assuming this is a DF17, otherwise not used) */
	metype = msg [4] >> 3;	/* Extended squitter message type. */
	mesub  = msg [4] & 7;	/* Extended squitter message subtype. */

/*	Fields for DF4,5,20,21 */
	fs = msg [0] & 7;        /* Flight status for DF4,5,20,21 */
	dr = msg [1] >> 3 & 31;  /* Request extraction of downlink request. */
/*	Request extraction of downlink request. */
	um = ((msg [1] & 7) << 3)| msg [2] >> 5;
/*
 *	In the squawk (identity) field bits are interleaved like that
 *	(message bit 20 to bit 32):
 *
 *	C1-A1-C2-A2-C4-A4-ZERO-B1-D1-B2-D2-B4-D4
 *
 *	So each group of three bits A, B, C, D represents an integer
 *	from 0 to 7.
 *
 *	The actual meaning is just 4 octal digits, but we convert it
 *	into a base ten number that happens to represent the four
 *	octal numbers.
 *
 *	For more info: http://en.wikipedia.org/wiki/Gillham_code
 */
	identity = computeIdentity (msg);
/*
 *	We recovered the message, mark the checksum as valid.
 */
	if ((msgtype != 11) && (msgtype != 17)) {
	   crcok = bruteForceAP (msg);
	}
	else {
/*
 *	If this is DF 11 or DF 17 and the checksum was ok,
 *	we can add this address to the list of recently seen
 *	addresses.
 */
	   if (crcok && (errorbit == -1)) {
	      uint32_t addr = (aa1 << 16) | (aa2 << 8) | aa3;
	      icao_cache ->  addRecentlySeenICAOAddr (addr);
	   }
	}

/*	Decode 13 bit altitude for DF0, DF4, DF16, DF20 */
	if (msgtype == 0 || msgtype == 4 ||
	    msgtype == 16 || msgtype == 20) {
	   altitude = decodeAC13Field (msg, &unit);
	}

/*	Decode extended squitter specific stuff. */
	if (msgtype == 17) {
	   recordExtendedSquitter (msg);
	}
}

	message::~message	(void) {
}

/*
 *	This function prints the "current" message on the screen
 *	in a human readable format.
 */
const
std::string message::displayMessage (bool check_crc) {
int j;
char help [256];
std::string res	= "*";
//	Show the raw message. */
	for (j = 0; j < msgbits / 8; j++) {
	   char t [10];
	   sprintf (t, "%02x  ", msg [j]);
	   res += std::string (t);
	}
	time_t t = time (NULL);
	res += "; received at ";
	res	+= std::string (ctime (&t));
	res += std::string("\n");
	
//	and its "interpretation"
	sprintf (help, "CRC: %06x (%s)\n", (int)crc, crcok ? "ok" : "wrong");
	res	+= std::string (help);
//	if (errorbit != -1)
//	   printf("Single bit error fixed, bit %d\n", errorbit);
//
	if (msgtype == 0) {
	   res += print_msgtype_0 ();
	} else
	if (msgtype == 4 || msgtype == 20) {
	   res += print_msgtype_4_20 ();
	} else
	if (msgtype == 5 || msgtype == 21) {
	   res += print_msgtype_5_21 ();
	} else
	if (msgtype == 11) {
	   res += print_msgtype_11 ();
	} else
	if (msgtype == 17) {
	   print_msgtype_17 ();
	} else 
	if (check_crc) {
	   sprintf (help, "   DF %d with good CRC received "
	               "(decoding still not implemented).\n",
				msgtype);
	   res += std::string (help);
	}
	
	return res;
}

/************************************************************************
 *	Helpers
 *************************************************************************/
bool	message::bruteForceAP (uint8_t *msg) {
uint8_t aux [LONG_MSG_BITS / 8];

	if (!(msgtype == 0 ||         /* Short air surveillance */
	      msgtype == 4 ||         /* Surveillance, altitude reply */
	      msgtype == 5 ||         /* Surveillance, identity reply */
	      msgtype == 16 ||        /* Long Air-Air survillance */
	      msgtype == 20 ||        /* Comm-A, altitude request */
	      msgtype == 21 ||        /* Comm-A, identity request */
	      msgtype == 24))
	   return false;
//	Comm-C ELM */
	uint32_t addr;
	uint32_t crc;
	int lastbyte = (msgbits / 8) - 1;

//	Work on a copy. */
	memcpy (aux, msg, msgbits / 8);

//	Compute the CRC of the message and XOR it with the AP field
//	so that we recover the address, because:
//		(ADDR xor CRC) xor CRC = ADDR. 

        crc = computeChecksum (aux, msgbits);
        aux [lastbyte]		^= crc & 0xff;
        aux [lastbyte-1]	^= (crc >> 8) & 0xff;
        aux [lastbyte-2]	^= (crc >> 16) & 0xff;
        
//	If the obtained address exists in our cache we consider
//	the message valid. 
        addr = aux [lastbyte] |
	      (aux [lastbyte-1] << 8) | (aux [lastbyte-2] << 16);
        if (icao_cache -> ICAOAddressWasRecentlySeen (addr)) {
            aa1 = aux [lastbyte - 2];
            aa2 = aux [lastbyte - 1];
            aa3 = aux [lastbyte];
            return true;
        }
	return false;
}

// For more info: http://en.wikipedia.org/wiki/Gillham_code
//
static int decodeID13Field (int ID13Field) {
int hexGillham = 0;

	if (ID13Field & 0x1000) {hexGillham |= 0x0010;} // Bit 12 = C1
	if (ID13Field & 0x0800) {hexGillham |= 0x1000;} // Bit 11 = A1
	if (ID13Field & 0x0400) {hexGillham |= 0x0020;} // Bit 10 = C2
	if (ID13Field & 0x0200) {hexGillham |= 0x2000;} // Bit  9 = A2
	if (ID13Field & 0x0100) {hexGillham |= 0x0040;} // Bit  8 = C4
	if (ID13Field & 0x0080) {hexGillham |= 0x4000;} // Bit  7 = A4
	//if (ID13Field & 0x0040) {hexGillham |= 0x0800;} // Bit  6 = X  or M 
	if (ID13Field & 0x0020) {hexGillham |= 0x0100;} // Bit  5 = B1 
	if (ID13Field & 0x0010) {hexGillham |= 0x0001;} // Bit  4 = D1 or Q
	if (ID13Field & 0x0008) {hexGillham |= 0x0200;} // Bit  3 = B2
	if (ID13Field & 0x0004) {hexGillham |= 0x0002;} // Bit  2 = D2
	if (ID13Field & 0x0002) {hexGillham |= 0x0400;} // Bit  1 = B4
	if (ID13Field & 0x0001) {hexGillham |= 0x0004;} // Bit  0 = D4

	return (hexGillham);
}
/*
 *	Decode the 13 bit AC altitude field (in DF 20 and others).
 *	Returns the altitude, and sets 'unit' to either UNIT_METERS
 *	or UNIT_FEETS. */
int	message::decodeAC13Field (uint8_t *msg, int *unit) {
int m_bit = msg [3] & (1 << 6);
int q_bit = msg [3] & (1 << 4);

	if (m_bit == 0) {
	   *unit = UNIT_FEET;
	   if (q_bit != 0) {
//	N is the 11 bit integer resulting from the removal of bit
//	Q and M 
               int n = ((msg [2] &   31) << 6) |
                       ((msg [3] & 0x80) >> 2) |
                       ((msg [3] & 0x20) >> 1) |
                        (msg [3] &   15);
//	The final altitude is due to the resulting number multiplied
//	by 25, minus 1000.
               return n * 25 - 1000;
           } else {
//	N is an 11 bit Gillham coded altitude
//            int n = modeAToModeC (decodeID13Field (AC13Field));
//            if (n < -12) {
//                return INVALID_ALTITUDE;
//            }
//
//            return (100 * n);
	   }
	} else {
	   *unit = UNIT_METERS;
//	TODO: Implement altitude when meter unit is selected. */
	}
	return 0;
}

/*
 *	Decode the 12 bit AC altitude field (in DF 17 and others).
 *	Returns the altitude or 0 if it can't be decoded.
 */
int	message::decodeAC12Field (uint8_t *msg, int *unit) {
int q_bit = msg [5] & 1;

	if (q_bit) {
//	N is the 11 bit integer resulting from the removal of bit Q 
	   *unit = UNIT_FEET;
	   int n = ((msg [5] >> 1) << 4) | ((msg [6] & 0xF0) >> 4);
//	The final altitude is computed:
	   return n * 25 - 1000;
	}
	else {
	   return 0;
	}
}

/* Capability table. */
const char *ca_str[8] = {
	/* 0 */ "Level 1 (Surveillance Only)",
	/* 1 */ "Level 2 (DF0,4,5,11)",
	/* 2 */ "Level 3 (DF0,4,5,11,20,21)",
	/* 3 */ "Level 4 (DF0,4,5,11,20,21,24)",
	/* 4 */ "Level 2+3+4 (DF0,4,5,11,20,21,24,code7 - is on ground)",
	/* 5 */ "Level 2+3+4 (DF0,4,5,11,20,21,24,code7 - is on airborne)",
	/* 6 */ "Level 2+3+4 (DF0,4,5,11,20,21,24,code7)",
	/* 7 */ "Level 7 ???"
};

/* Flight status table. */
const char *fs_str[8] = {
	/* 0 */ "Normal, Airborne",
	/* 1 */ "Normal, On the ground",
	/* 2 */ "ALERT,  Airborne",
	/* 3 */ "ALERT,  On the ground",
	/* 4 */ "ALERT & Special Position Identification. Airborne or Ground",
	/* 5 */ "Special Position Identification. Airborne or Ground",
	/* 6 */ "Value 6 is not assigned",
	/* 7 */ "Value 7 is not assigned"
};

const char *getMEDescription (int metype, int mesub) {
const char *mename = "Unknown";

	if (metype >= 1 && metype <= 4)
	    mename = "Aircraft Identification and Category";
	else if (metype >= 5 && metype <= 8)
	    mename = "Surface Position";
	else if (metype >= 9 && metype <= 18)
	    mename = "Airborne Position (Baro Altitude)";
	else if (metype == 19 && mesub >=1 && mesub <= 4)
	    mename = "Airborne Velocity";
	else if (metype >= 20 && metype <= 22)
	    mename = "Airborne Position (GNSS Height)";
	else if (metype == 23 && mesub == 0)
	    mename = "Test Message";
	else if (metype == 24 && mesub == 1)
	    mename = "Surface System Status";
	else if (metype == 28 && mesub == 1)
	    mename = "Extended Squitter Aircraft Status (Emergency)";
	else if (metype == 28 && mesub == 2)
	    mename = "Extended Squitter Aircraft Status (1090ES TCAS RA)";
	else if (metype == 29 && (mesub == 0 || mesub == 1))
	    mename = "Target State and Status Message";
	else if (metype == 31 && (mesub == 0 || mesub == 1))
	    mename = "Aircraft Operational Status Message";
	return mename;
}

void	message::setFlightName (uint8_t *msg) {
const char *ais_charset =
	 "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? ???????????????0123456789??????";
//	Aircraft Identification and Category */
	aircraft_type = metype - 1;
	flight [0] = ais_charset [msg [5] >> 2];
	flight [1] = ais_charset [((msg [5] & 3) << 4)|(msg [6] >> 4)];
	flight [2] = ais_charset [((msg [6] & 15) << 2)|(msg [7] >> 6)];
	flight [3] = ais_charset [msg [7] & 63];
	flight [4] = ais_charset [msg [8] >> 2];
	flight [5] = ais_charset [((msg [8] & 3) << 4)|(msg [9] >> 4)];
	flight [6] = ais_charset [((msg [9] & 15) << 2)|(msg [10] >> 6)];
	flight [7] = ais_charset [msg [10] & 63];
	flight [8] = '\0';
}

//
        /* DF 0 */
const
std::string	message::print_msgtype_0 () {
std::string res = "";
	if (msgtype == 0) {
	   char help [200];
	   sprintf (help, "DF 0: Short Air-Air Surveillance.\n");
	   res += std::string (help);
	   sprintf (help, "  Altitude       : %d %s\n", altitude,
	           (unit == UNIT_METERS) ? "meters" : "feet");
	   res += std::string (help);
	   sprintf (help, "  ICAO Address   : %02x%02x%02x\n", aa1, aa2, aa3);
	   res += std::string (help);
	}
	return res;
}

const
std::string	message::print_msgtype_4_20 () {
	std::string res = "";
	char help[100];
	if (msgtype == 4 || msgtype == 20) {
	   sprintf (help, "DF %d: %s, Altitude Reply.\n", msgtype,
	                  (msgtype == 4) ? "Surveillance" : "Comm-B");
	   res += std::string (help);
	   sprintf (help, "  Flight Status  : %s\n", fs_str [fs]);
	   res += std::string (help);
	   sprintf (help, "  DR             : %d\n", dr);
	   res += std::string (help);
	   sprintf (help, "  UM             : %d\n", um);
	   res += std::string (help);
           sprintf (help, "  Altitude       : %d %s\n", altitude,
	                (unit == UNIT_METERS) ? "meters" : "feet");
	   res += std::string (help);
	   sprintf (help, "  ICAO Address   : %02x%02x%02x\n", aa1, aa2, aa3);
	   res += std::string (help);
	   if (msgtype == 20) {
//	TODO: 56 bits DF20 MB additional field. */
	   }
	}
	return res;
}

const
std::string	message::print_msgtype_5_21 (void) {
std::string res = "";
char help [256];
	if (msgtype == 5 || msgtype == 21) {
	   sprintf (help, "DF %d: %s, Identity Reply.\n", msgtype,
	                     (msgtype == 5) ? "Surveillance" : "Comm-B");
	   res += std::string (help);
	   sprintf (help, "  Flight Status  : %s\n", fs_str [fs]);
	   res += std::string (help);
	   sprintf (help, "  DR             : %d\n", dr);
	   res += std::string (help);
	   sprintf (help, "  UM             : %d\n", um);
	   res += std::string (help);
	   sprintf (help, "  Squawk         : %d\n", identity);
	   res += std::string (help);
	   sprintf (help, "  ICAO Address   : %02x%02x%02x\n", aa1, aa2, aa3);
	   res += std::string (help);

	   if (msgtype == 21) {
//	TODO: 56 bits DF21 MB additional field. */
	   }
	}
	return res;
}

//	DF 11 */
const
std::string	message::print_msgtype_11 (void) {
std::string res = "";
	if (msgtype == 11) {
	   char t [100];
	   sprintf (t, "DF 11: All Call Reply.\n");
	   res += std::string (t);
	   sprintf (t, "  Capability  : %s\n", ca_str [ca]);
	   res += std::string (t);
	   sprintf (t, "  ICAO Address: %02x%02x%02x\n", aa1, aa2, aa3);
	   res += std::string (t);
	}
	return res;
}

//	DF 17 */
const
std::string	message::print_msgtype_17 (void) {
std::string res = "";
char help [100];
	if (msgtype == 17) {
	   sprintf (help, "DF 17: ADS-B message.\n");
	   res += std::string (help);
	   sprintf (help, "  Capability     : %d (%s)\n", ca, ca_str [ca]);
	   res += std::string (help);
	   sprintf (help,  "  ICAO Address   : %02x%02x%02x\n", aa1, aa2, aa3);
	   res += std::string (help);
	   sprintf (help, "  Extended Squitter  Type: %d\n", metype);
	   res += std::string (help);
	   sprintf (help, "  Extended Squitter  Sub : %d\n", mesub);
	   res += std::string (help);
	   sprintf (help, "  Extended Squitter  Name: %s\n",
	                        getMEDescription (metype, mesub));
	   res += std::string (help);

//	Decode the extended squitter message. */
	   if (metype >= 1 && metype <= 4) {
//	Aircraft identification. */
	      const char *ac_type_str [4] = {
	                               "Aircraft Type D",
	                               "Aircraft Type C",
	                               "Aircraft Type B",
	                               "Aircraft Type A"
	      };

	      sprintf (help, "    Aircraft Type  : %s\n",
	                           ac_type_str [aircraft_type]);
	      res += std::string (help);
	      sprintf (help, "    Identification : %s\n", flight);
	      res += std::string (help);
	   } else
	   if (metype >= 9 && metype <= 18) {
	      sprintf (help, "    F flag   : %s\n", fflag ? "odd" : "even");
	      res += std::string (help);
	      sprintf (help, "    T flag   : %s\n", tflag ? "UTC" : "non-UTC");
	      res += std::string (help);
	      sprintf (help, "    Altitude : %d feet\n", altitude);	
	      res += std::string (help);
	      sprintf (help, "    Latitude : %d (not decoded)\n", raw_latitude);
	      res += std::string (help);
	      sprintf (help, "    Longitude: %d (not decoded)\n", raw_longitude);
	      res += std::string (help);
	   } else
	   if (metype == 19 && mesub >= 1 && mesub <= 4) {
	      if (mesub == 1 || mesub == 2) {
//	Velocity */
                sprintf (help, "    EW direction      : %d\n", ew_dir);
	        res += std::string (help);
                sprintf (help, "    EW velocity       : %d\n", ew_velocity);
	        res += std::string (help);
                sprintf (help, "    NS direction      : %d\n", ns_dir);
	        res += std::string (help);
                sprintf (help, "    NS velocity       : %d\n", ns_velocity);
	        res += std::string (help);
                sprintf (help, "    Vertical rate src : %d\n", vert_rate_source);
	        res += std::string (help);
                sprintf (help, "    Vertical rate sign: %d\n", vert_rate_sign);
	        res += std::string (help);
                sprintf (help, "    Vertical rate     : %d\n", vert_rate);
	        res += std::string (help);
	      } else
	      if (mesub == 3 || mesub == 4) {
	         sprintf (help, "    Heading status: %d", heading_is_valid);
	         res += std::string (help);
	         sprintf (help, "    Heading: %d", heading);
	         res += std::string (help);
	      }
	   } else {
	      sprintf (help, "    Unrecognized ME type: %d subtype: %d\n", 
	                                   metype, mesub);
	      res += std::string (help);
	   }
	} 
	return res;
}

void	message::recordExtendedSquitter (uint8_t *msg) {
	if (msgtype == 17) {
	   if (metype >= 1 && metype <= 4) {
	      setFlightName (msg);	// fill in text repr of flight
	   }
	   else
	   if (metype >= 9 && metype <= 18) {
/*	Airborne position Message */
	      fflag = msg [6] & (1 << 2);
	      tflag = msg [6] & (1 << 3);
	      altitude = decodeAC12Field (msg, &unit);
	      raw_latitude = ((msg [6] & 3) << 15) |
	                      (msg [7] << 7) |
	                      (msg [8] >> 1);
	      raw_longitude = ((msg [8] &1) << 16) |
	                       (msg [9] << 8) |
	                        msg [10];
	   }
	   else
	   if (metype == 19 && mesub >= 1 && mesub <= 4) {
/*	Airborne Velocity Message */
	      if (mesub == 1 || mesub == 2) {
	         ew_dir		= (msg [5] & 04) >> 2;
	         ew_velocity	= ((msg [5] & 3) << 8) | msg[6];
	         ns_dir		= (msg [7] & 0x80) >> 7;
	         ns_velocity	= ((msg [7] & 0x7f) << 3) |
	                                    ((msg [8] & 0xe0) >> 5);
	         vert_rate_source = (msg [8] & 0x10) >> 4;
	         vert_rate_sign = (msg [8] & 0x8) >> 3;
	         vert_rate	= ((msg [8] & 7) << 6) |
	                                       ((msg [9] & 0xfc) >> 2);
/* Compute velocity and angle from the two speed components. */
	         velocity	= sqrt (ns_velocity * ns_velocity+
	                                ew_velocity * ew_velocity);
	         if (velocity != 0) {
	            int ewv = ew_velocity;
	            int nsv = ns_velocity;
	            double heading;

                    if (ew_dir)
	               ewv *= -1;
                    if (ns_dir)
	               nsv *= -1;
                    heading = atan2 (ewv, nsv);
                    /* Convert to degrees. */
                    heading = heading * 360 / (M_PI * 2);
                    /* We don't want negative values but a 0-360 scale. */
                    if (heading < 0) 
	               heading += 360;
	            if (heading > 360)
	               heading = 0;
	         } else {
	            heading = 0;
	         }  
              }
	      else
	      if (mesub == 3 || mesub == 4) {
	         heading_is_valid = msg [5] & (1 << 2);
	         if (heading_is_valid)
	            heading = (360.0 / 128) * (((msg [5] & 3) << 5) |
	                                                 (msg [6] >> 3));
	      }
	   }
	}
}

uint32_t	message::getAddr (void) {
	return  (aa1 << 16) | (aa2 << 8) | aa3;
}

bool	message::is_crcok	(void) {
	return crcok;
}

