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
#include	<sstream>
#include	<unoevent.h>
#include	<iunoplugincontroller.h>
#include	<nana/gui/filebox.hpp>
#include	<vector>
#include	<sstream>
#include	<chrono>
#include	"SDRunoPlugin_1090.h"
#include	"SDRunoPlugin_1090Ui.h"

#include	"message-handling.h"
#include	"http-handler.h"
//
#define	IN_SIZE		300
#define	OUT_SIZE	240

	SDRunoPlugin_1090::
	           SDRunoPlugin_1090 (IUnoPluginController& controller) :
	                                 IUnoPlugin (controller),
	                                 m_form     (*this, controller),
	                                 m_worker   (nullptr),
	                                 _I_Buffer  (64 * 32768),
	                                 passbandFilter (5,
	                                                 -600000,
	                                                 +600000,
	                                                 3000000),
	                                 theMixer  (3000000),
	                                 mapTable_int (OUT_SIZE),
	                                 mapTable_float (OUT_SIZE) {
		 
	m_controller	= &controller;

	data_len	= DATA_LEN + (FULL_LEN - 1) * 4;
	magnitudeVector	= new uint16_t [data_len / 2];
//	Allocate the ICAO address cache.
	icao_cache      = new icaoCache ();
	planeList	= nullptr;
	dumpFilePointer	= nullptr;
	httpServer	= nullptr;

	for (int i = 0; i < OUT_SIZE; i++) {
	   float inVal = float (IN_SIZE);
	   mapTable_int [i] = int (floor (i * (inVal / OUT_SIZE)));
	   mapTable_float[i] = i * (inVal / OUT_SIZE) - mapTable_int[i];
	}
	convIndex = 0;
	convBuffer.resize (IN_SIZE + 1);

	for (int i = 0; i < 31; i ++)
	   table [i] = 0;
	
	check_crc	        = true;
	handle_errors		= NO_ERRORFIX;
	metric	                = false;
        interactive_ttl	        = INTERACTIVE_TTL;

	interactive	        = true;
	stat_valid_preamble	= 0;
	stat_goodcrc		= 0;
	stat_badcrc	        = 0;
	stat_fixed	        = 0;
	stat_single_bit_fix	= 0;
	stat_two_bits_fix	= 0;
	stat_phase_corrected	= 0;

	time (&currentTime);
	m_controller    -> RegisterStreamProcessor (0, this);
	m_controller    -> RegisterAudioProcessor (0, this);
	m_controller	-> SetVfoFrequency (0, 1090000000.0);
	selectedFrequency
                        = m_controller -> GetVfoFrequency (0);
        centerFrequency = m_controller -> GetCenterFrequency (0);

	int inRate	        = m_controller -> GetSampleRate (0);
	error_1090	        = false;	
	if (inRate != 3000000) {
	   m_form. show_text ("The plugin will only work with 3000000 inputRate");
	   error_1090 = true;
	}

	running.store (false);
        m_worker        =
	          new std::thread (&SDRunoPlugin_1090::WorkerFunction, this);

}

	SDRunoPlugin_1090::~SDRunoPlugin_1090 () {
	running. store (false);
	if (dumpFilePointer != nullptr)
	   fclose(dumpFilePointer);
	dumpFilePointer = nullptr;
	m_worker -> join ();
	m_controller    -> UnregisterStreamProcessor (0, this);
	m_controller    -> UnregisterAudioProcessor (0, this);
	while (planeList != nullptr) {
	   aircraft *n = planeList -> next;
	   delete planeList;
	   planeList = n;
	}

	delete	icao_cache;
        delete	m_worker;
        m_worker = nullptr;
}

static inline
uint16_t	fl_abs (Complex f) {
	return	2048 * (f. real < 0 ? -f. real : f.real) +
	        2048 * (f. imag < 0 ? -f. imag : f. imag);
}
//
//	Note
//	The implementation used assumes an inputrate of 3.4 MHz,
//	We run the input device on 3 MHz and interpolate to get
//	2.4. An earlier attempt was using a samplerate of 2 MHz
//	and upsampling a little, but a samplerate of 3 MHz
//	allows us to shift a little over the incoming window
//	and experience shows that not all messages are on
//	exactly 1090MHz
void    SDRunoPlugin_1090::StreamProcessorProcess (channel_t channel,
	                                           Complex *buffer,
	                                           int length,
	                                           bool& modified) {
uint16_t vec [OUT_SIZE];

	if (running. load ()) {
	   int theOffset = centerFrequency - selectedFrequency;
	   locker.lock();
	   for (int i = 0; i < length / 2; i ++) {
	      std::complex<float> t = 
	               std::complex<float> (buffer [i]. real,
	                                    buffer [i]. imag);
	      t	= passbandFilter. Pass (t);
	      t = theMixer. doShift (t, - theOffset);
	      convBuffer [convIndex ++] = fl_abs (buffer [i]);
	      if (convIndex >= convBuffer. size ()) {
	         for (int j = 0; j < OUT_SIZE; j ++) {
	            int inpBase		= mapTable_int [j];
	            float inpRatio	= mapTable_float [j];
	            vec [j] = convBuffer [inpBase + 1] * inpRatio +
                                 convBuffer [inpBase] * (1 - inpRatio);
	         }
	         _I_Buffer . putDataIntoBuffer (vec, OUT_SIZE);
	         convBuffer [0] = convBuffer [convBuffer. size () - 1];
	         convIndex = 1;
	      }
	   }
	   locker.unlock();
	   modified = false;
	}
}

void    SDRunoPlugin_1090::AudioProcessorProcess (channel_t channel,
                                                  float* buffer,
                                                  int length,
                                                  bool& modified) {
}

void	SDRunoPlugin_1090::HandleEvent (const UnoEvent& ev) {
	switch (ev.GetType()) {
	   case UnoEvent::FrequencyChanged:
	      selectedFrequency =
	         m_controller -> GetVfoFrequency (ev. GetChannel ());
              centerFrequency = m_controller-> GetCenterFrequency(0);
              locker. lock ();
              passbandFilter.
                        update (selectedFrequency - centerFrequency, 2000);
              locker. unlock ();

	      break;

	   case UnoEvent::CenterFrequencyChanged:
	      break;

	   default:
	      m_form.HandleEvent(ev);
	      break;
	}
}

void	SDRunoPlugin_1090::WorkerFunction () {
static
uint16_t buffer [DATA_LEN / 2];

	running. store (true);
	while (running. load () && !error_1090) {
	   while (running. load () &&
	          (_I_Buffer. GetRingBufferReadAvailable () <  DATA_LEN / 2))
	      Sleep (1);
	   if (!running. load ())
	      break;
//
	   _I_Buffer. getDataFromBuffer (buffer, DATA_LEN / 2);
//
	   memmove (&magnitudeVector [0],	          
		        &magnitudeVector [DATA_LEN / 2],
	            ((FULL_LEN - 1) * sizeof (uint16_t)) * 4 / 2);
	   memcpy (&magnitudeVector [(FULL_LEN - 1) * 4 / 2],
	           buffer, DATA_LEN / 2 * sizeof (int16_t));
	   detectModeS  (magnitudeVector, data_len / 2);
	}
	m_form.show_text ("end of worker function");
	Sleep(1000);
}

static inline int slice_phase0(uint16_t *m) {
	return 5 * m[0] - 3 * m[1] - 2 * m[2];
}

static inline int slice_phase1(uint16_t *m) {
	return 4 * m[0] - m[1] - 3 * m[2];
}

static inline int slice_phase2(uint16_t *m) {
	return 3 * m[0] + m[1] - 4 * m[2];
}

static inline int slice_phase3(uint16_t *m) {
	return 2 * m[0] + 3 * m[1] - 5 * m[2];
}

static inline int slice_phase4(uint16_t *m) {
	return m[0] + 5 * m[1] - 5 * m[2] - m[3];
}

//	2.4MHz sampling rate version
//
//	When sampling at 2.4MHz we have exactly 6 samples per 5 symbols.
//	Each symbol is 500ns wide, each sample is 416.7ns wide
//
//	We maintain a phase offset that is expressed in units of
//	1/5 of a sample i.e. 1/6 of a symbol, 83.333ns
//	Each symbol we process advances the phase offset by
//	6 i.e. 6/5 of a sample, 500ns
//
//	The correlation functions above correlate a 1-0 pair
//	of symbols (i.e. manchester encoded 1 bit)
//	starting at the given sample, and assuming that
//	the symbol starts at a fixed 0-5 phase offset within
//	m[0].
//	They return a correlation value, generally
//	interpreted as >0 = 1 bit, <0 = 0 bit
//
//	TODO check if there are better (or more balanced)
//	correlation functions to use here
//

void	SDRunoPlugin_1090::detectModeS (uint16_t *m, uint32_t mlen) {
uint8_t	msg 	[LONG_MSG_BITS / 8];
uint32_t j;
/*
 *	The Mode S preamble is made of impulses of 0.5 microseconds at
 *	the following time offsets:
 *
 *	0   - 0.5 usec: first impulse.
 *	1.0 - 1.5 usec: second impulse.
 *	3.5 - 4   usec: third impulse.
 *	4.5 - 5   usec: last impulse.
 *
 *	0   -----------------
 *	1   -
 *	2   ------------------
 *	3   --
 *	4   -
 *	5   --
 *	6   -
 *	7   ------------------
 *	8   --
 *	9   -------------------
 */
	for (j = 0; j < mlen; j++) {
	   uint16_t *preamble = &m [j];
	   int high;
	   uint32_t base_signal, base_noise;
	   int try_phase;

//	Look for a message starting at around sample 0 with phase offset 3..7
//	Ideal sample values for preambles with different phase
//	Xn is the first data symbol with phase offset N
//
//	sample#: 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
//	phase 3: 2/4\0/5\1 0 0 0 0/5\1/3 3\0 0 0 0 0 0 X4
//	phase 4: 1/5\0/4\2 0 0 0 0/4\2 2/4\0 0 0 0 0 0 0 X0
//	phase 5: 0/5\1/3 3\0 0 0 0/3 3\1/5\0 0 0 0 0 0 0 X1
//	phase 6: 0/4\2 2/4\0 0 0 0 2/4\0/5\1 0 0 0 0 0 0 X2
//	phase 7: 0/3 3\1/5\0 0 0 0 1/5\0/4\2 0 0 0 0 0 0 X3
//
        
//	quick check: we must have a rising edge 0->1 and a falling edge 12->13
	   if (!(preamble [0]  < preamble [1] &&
	         preamble [12] > preamble [13]) )
              continue;

	   if (preamble [1] > preamble [2] &&                   // 1
	       preamble [2] < preamble [3] &&
	                        preamble [3] > preamble [4] &&  // 3
	       preamble [8] < preamble [9] &&
	                        preamble[9] > preamble[10] &&   // 9
	       preamble [10] < preamble [11]) {                 // 11-12
//	peaks at 1,3,9,11-12: phase 3
	      high = (preamble [1] + preamble [3] +
	              preamble[9] + preamble [11] + preamble[12]) / 4;
	      base_signal	= preamble[1] + preamble[3] + preamble[9];
	      base_noise	= preamble[5] + preamble[6] + preamble[7];
	   }
	   else
	   if (preamble [1] > preamble[2] &&                       // 1
	       preamble [2] < preamble[3] &&
	                   preamble[3] > preamble[4] &&  // 3
	       preamble [8] < preamble[9] &&
	                   preamble[9] > preamble[10] && // 9
	       preamble [11] < preamble [12]) {                     // 12
//	peaks at 1,3,9,12: phase 4
	      high = (preamble [1] + preamble [3] +
	                 preamble [9] + preamble [12]) / 4;
	      base_signal = preamble [1] + preamble [3] +
	                          preamble [9] + preamble [12];
	      base_noise  = preamble [5] + preamble [6] +
	                          preamble [7] + preamble [8];
	   }
	   else
	   if (preamble [1] > preamble [2] &&                        // 1
	       preamble [2] < preamble [3] &&
	                          preamble[4] > preamble[5] && // 3-4
	       preamble [8] < preamble [9] &&
	                          preamble[10] > preamble[11] && // 9-10
	       preamble [11] < preamble[12]) {                    // 12
//	peaks at 1,3-4,9-10,12: phase 5
	      high = (preamble [1] + preamble [3] +
	              preamble [4] + preamble [9] +
	                      preamble [10] + preamble [12]) / 4;
	      base_signal	= preamble [1] + preamble [12];
	      base_noise	= preamble [6] + preamble [7];
	   }
	   else
	   if (preamble[1] > preamble[2] &&            // 1
	       preamble[3] < preamble[4] &&
	       preamble[4] > preamble[5] &&            // 4
	       preamble[9] < preamble[10] &&
	       preamble[10] > preamble[11] &&          // 10
	       preamble[11] < preamble[12]) {          // 12
//	peaks at 1,4,10,12: phase 6
	      high = (preamble[1] + preamble[4] +
	                      preamble [10] + preamble [12]) / 4;
	      base_signal = preamble [1] + preamble [4] +
	                      preamble [10] + preamble [12];
	      base_noise = preamble [5] + preamble [6] +
	                      preamble [7] + preamble [8];
	   }
	   else
	   if (preamble [2] > preamble [3] &&            // 1-2
	       preamble [3] < preamble [4] &&
	               preamble [4] > preamble [5] &&    // 4
	       preamble [9] < preamble [10] &&
	               preamble[10] > preamble[11] &&    // 10
	       preamble [11] < preamble [12]) {          // 12
//	peaks at 1-2,4,10,12: phase 7
	      high = (preamble [1] + preamble [2] +
	                preamble [4] + preamble [10] + preamble [12]) / 4;
	      base_signal = preamble [4] + preamble [10] + preamble [12];
	      base_noise = preamble[6] + preamble[7] + preamble[8];
	   }
	   else {
//	no suitable peaks
	      continue;
	   }

// Check for enough signal
	   if (base_signal * 2 < 8 * base_noise) // about 3.5dB SNR
	      continue;

//	Check that the "quiet" bits 6,7,15,16,17 are actually quiet
	   if (preamble [5] >= high / 2 ||
               preamble [6] >= high / 2 ||
               preamble [7] >= high / 2 ||
               preamble [8] >= high / 2 ||
               preamble [14] >= high / 2 ||
               preamble [15] >= high / 2 ||
               preamble [16] >= high / 2 ||
               preamble [17] >= high / 2 ||
               preamble [18] >= high / 2) {
	      continue;
	   }
//	try phases until we find a good crc (if any)

	   show_validPreambles ((int)stat_valid_preamble ++);
	   for (try_phase = 4; try_phase <= 8; ++try_phase) {
	      int msgType;
	      uint16_t *pPtr;
              int phase, i, bytelen;

	      pPtr = &m [j + 19] + (try_phase / 5);
	      phase = try_phase % 5;

	      bytelen = LONG_MSG_BITS / 8;
	      for (i = 0; i < bytelen; ++i) {
              uint8_t theByte = 0;

                 switch (phase) {
	            case 0:
	               theByte = 
	                  (slice_phase0 (pPtr) > 0 ? 0x80 : 0) |
	                  (slice_phase2 (pPtr+2) > 0 ? 0x40 : 0) |
	                  (slice_phase4 (pPtr+4) > 0 ? 0x20 : 0) |
	                  (slice_phase1 (pPtr+7) > 0 ? 0x10 : 0) |
	                  (slice_phase3 (pPtr+9) > 0 ? 0x08 : 0) |
	                  (slice_phase0 (pPtr+12) > 0 ? 0x04 : 0) |
	                  (slice_phase2 (pPtr+14) > 0 ? 0x02 : 0) |
	                  (slice_phase4 (pPtr+16) > 0 ? 0x01 : 0);

	                phase = 1;
	                pPtr += 19;
	                break;
                    
	             case 1:
	                theByte =
	                   (slice_phase1 (pPtr) > 0 ? 0x80 : 0) |
	                   (slice_phase3 (pPtr+2) > 0 ? 0x40 : 0) |
	                   (slice_phase0 (pPtr+5) > 0 ? 0x20 : 0) |
	                   (slice_phase2 (pPtr+7) > 0 ? 0x10 : 0) |
	                   (slice_phase4 (pPtr+9) > 0 ? 0x08 : 0) |
	                   (slice_phase1 (pPtr+12) > 0 ? 0x04 : 0) |
	                   (slice_phase3 (pPtr+14) > 0 ? 0x02 : 0) |
	                   (slice_phase0 (pPtr+17) > 0 ? 0x01 : 0);

	                phase = 2;
	                pPtr += 19;
	                break;

	             case 2:
	                theByte =
	                   (slice_phase2 (pPtr) > 0 ? 0x80 : 0) |
	                   (slice_phase4 (pPtr+2) > 0 ? 0x40 : 0) |
	                   (slice_phase1 (pPtr+5) > 0 ? 0x20 : 0) |
	                   (slice_phase3 (pPtr+7) > 0 ? 0x10 : 0) |
	                   (slice_phase0 (pPtr+10) > 0 ? 0x08 : 0) |
	                   (slice_phase2 (pPtr+12) > 0 ? 0x04 : 0) |
	                   (slice_phase4 (pPtr+14) > 0 ? 0x02 : 0) |
	                   (slice_phase1 (pPtr+17) > 0 ? 0x01 : 0);

	                phase = 3;
	                pPtr += 19;
	                break;

	             case 3:
	               theByte = 
	                   (slice_phase3 (pPtr) > 0 ? 0x80 : 0) |
	                   (slice_phase0 (pPtr+3) > 0 ? 0x40 : 0) |
	                   (slice_phase2 (pPtr+5) > 0 ? 0x20 : 0) |
	                   (slice_phase4 (pPtr+7) > 0 ? 0x10 : 0) |
	                   (slice_phase1 (pPtr+10) > 0 ? 0x08 : 0) |
	                   (slice_phase3 (pPtr+12) > 0 ? 0x04 : 0) |
	                   (slice_phase0 (pPtr+15) > 0 ? 0x02 : 0) |
	                   (slice_phase2 (pPtr+17) > 0 ? 0x01 : 0);

	               phase = 4;
	               pPtr += 19;
	               break;

	            case 4:
	               theByte = 
	                   (slice_phase4 (pPtr) > 0 ? 0x80 : 0) |
	                   (slice_phase1 (pPtr+3) > 0 ? 0x40 : 0) |
	                   (slice_phase3 (pPtr+5) > 0 ? 0x20 : 0) |
	                   (slice_phase0 (pPtr+8) > 0 ? 0x10 : 0) |
	                   (slice_phase2 (pPtr+10) > 0 ? 0x08 : 0) |
	                   (slice_phase4 (pPtr+12) > 0 ? 0x04 : 0) |
	                   (slice_phase1 (pPtr+15) > 0 ? 0x02 : 0) |
	                   (slice_phase3 (pPtr+17) > 0 ? 0x01 : 0);

	               phase = 0;
	               pPtr += 20;
	               break;
	         }

	         msg [i] = theByte;
	         if (i == 0) {
	            msgType = msg [0] >> 3;
	            switch (msgType) {
	               case 0: case 4: case 5: case 11:
	                  bytelen = SHORT_MSG_BITS / 8;
	                  break;
                        
	               case 16: case 17: case 18: case 20: case 21: case 24:
	                  break;

	               default:
	                  bytelen = 1; // unknown DF, give up immediately
	                  break;
	            }
	         }
	      }
	      if (bytelen == 1)
	         continue;
//
/*	If we reached this point, it is possible that we
 *	have a Mode S message in our hands, but it may still be broken
 *	and CRC may not be correct.
 *	The message is decoded and checked, if false, we give up
 */
	      message mm (handle_errors, icao_cache, msg);

	      if (mm. is_crcok ()) {
//	update statistics
	         if (mm. errorbit == -1) {    // no corrections
	            if (mm. is_crcok ()) {
	               stat_goodcrc++;
	               show_goodCrc(stat_goodcrc);
	            }
	         }
	         else {
	            stat_fixed++;
	            show_nrfixed ((int)stat_fixed);
	            if (mm. errorbit < LONG_MSG_BITS) {
	               stat_single_bit_fix++;
	               show_single_bit_fixed ((int)stat_single_bit_fix);
	            }
	            else {
	               stat_two_bits_fix++;
	               show_two_bit_fixed((int)stat_two_bits_fix);
	            }
	         }

	         j += (PREAMBLE_US  + bytelen * 8) * 2 * 8 * 6/5;
	         table [msgType & 0x1F] ++;	
	         update_table (msgType & 0x1F, table [msgType & 0x1F]);
//	         phase_corrected -> display ((int)try_phase);
	         
/*
 *	Track aircrafts in interactive mode or if the HTTP
 *	interface is enabled.
 */
	         if (interactive) {
	            time_t newTime;
	            time (&newTime);
		        planeList = interactiveReceiveData (planeList, &mm);
	            planeList = removeStaleAircrafts (planeList,
	                                                  interactive_ttl,
	                                                  dumpFilePointer);
				if (difftime(newTime, currentTime) > 5) {
					m_form.show_text(showPlanes(planeList, metric));
					currentTime = newTime;
				}
	         }
/*
 *	In non-interactive way, display messages on standard output.
 */
	         if (!interactive) {
	           std::string res =  mm. displayMessage (check_crc);
	           m_form. show_text (res);
	         }
	         break;
	      }
	   }
        }
}

void	SDRunoPlugin_1090::update_table	(int16_t index, int newval) {
	switch (index) {
	   case 0:
//	      DF0	-> display (newval);
	      break;
	   case 4:
//	      DF4	-> display (newval);
	      break;
	   case 5:
//	      DF5	-> display (newval);
	      break;
	   case 11:
//	      DF11	-> display (newval);
	      break;
	   case 16:
//	      DF16	-> display (newval);
	      break;
	   case 17:
//	      DF17	-> display (newval);
	      break;
	   case 20:
//	      DF20	-> display (newval);
	      break;
	   case 21:
//	      DF21	-> display (newval);
	      break;
	   case 24:
//	      DF24	-> display (newval);
	      break;
	   default:
//	      fprintf (stderr, "DF%d -> %d\n", index, newval);
	      break;
	}
}

//
//	going down:
void	SDRunoPlugin_1090::show_validPreambles	(int n) {
	m_form. show_validPreambles (n);
}

void	SDRunoPlugin_1090::show_goodCrc		(int n) {
	m_form. show_goodCrc (n);
}

void	SDRunoPlugin_1090::show_nrfixed		(int n) {
	m_form. show_nrfixed (n);
}

void	SDRunoPlugin_1090::show_single_bit_fixed (int n) {
	m_form. show_single_bit_fixed (n);
}

void	SDRunoPlugin_1090::show_two_bit_fixed	(int n) {
	m_form. show_two_bit_fixed (n);
}

void	SDRunoPlugin_1090::set_correction	(const std::string &s) {
	if (s == "1 bit correction")
	   handle_errors	= NORMAL_ERRORFIX;
	else
	if (s == "2 bits correction")
	   handle_errors	= STRONG_ERRORFIX;
	else
	   handle_errors	= NO_ERRORFIX;
}

void	SDRunoPlugin_1090::set_displayMode	(const std::string &s) {
	if (s == "interactive")
	   interactive	= true;
	else
	   interactive	= false;
}

void	SDRunoPlugin_1090::set_metricsMode	(const std::string &s) {
	if (s == "metric")
	   metric	= true;
	else
	   metric	= false;
}

using namespace nana;
void	SDRunoPlugin_1090::handle_fileButton		() {

	if (dumpFilePointer == nullptr) {
	   nana::filebox fb (0, false);
	   fb.add_filter ("Text File", "*.text");
	   fb.add_filter("All Files", "*.*");
	   auto files = fb();
	   if (!files. empty ()) {
	      dumpFilePointer =
	             fopen (files. front (). string (). c_str (), "w");
	      if (dumpFilePointer == nullptr) 
	         m_form. show_text ("file open failed");
	   }
	}
	else {
	   fclose (dumpFilePointer);
	   dumpFilePointer = nullptr;
	}
}

void	SDRunoPlugin_1090::set_http		(const std::string &s) {
char *home	= getenv ("HOMEPATH");
std::string theMap	= std::string (home) + std::string ("\\Documents\\gmap.html");
	if (s == "http on") {
	   if (httpServer == nullptr) {
//	      httpServer = new httpHandler (this, std::string ("d:\gmap.html"));
	      httpServer = new httpHandler (this, theMap);
	      httpServer->start ();
	   }
	}
	else {
	   if (httpServer != nullptr)
	      delete httpServer;
	   httpServer = nullptr;
	}
}

void	SDRunoPlugin_1090::show_text (const std::string s) {
	m_form.show_text(s);
}

