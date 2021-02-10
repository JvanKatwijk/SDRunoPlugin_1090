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
#pragma once

#include	<thread>
#include	<mutex>
#include	<atomic>
#include	<iunoplugincontroller.h>
#include	<iunoplugin.h>
#include	<iunostreamobserver.h>
#include	<iunoaudioobserver.h>
#include	<iunoaudioprocessor.h>
#include	<iunostreamobserver.h>
#include	<iunoannotator.h>
#include	"SDRunoPlugin_1090Ui.h"
//
//	and for the payload we need:
#include	"ringbuffer.h"
#include	"adsb-constants.h"
#include	"bandfilter.h"
#include	"shifter.h"
#include	"icao-cache.h"
#include	"aircraft-handler.h"
#include	<stdio.h>
#include	<time.h>
class		httpHandler;

class SDRunoPlugin_1090 : public IUnoPlugin,
	                   public IUnoStreamProcessor,
	                   public IUnoAudioProcessor {
public:
	
	        SDRunoPlugin_1090 (IUnoPluginController& controller);
	virtual ~SDRunoPlugin_1090 ();

	// TODO: change the plugin title here
	virtual const
	char* GetPluginName () const override { return "SDRuno Plugin 1090"; }
        virtual
        void    StreamProcessorProcess (channel_t channel,
                                        Complex *buffer, int length,
                                            bool& modified) override;
        virtual
        void    AudioProcessorProcess (channel_t channel,
                                       float *buffer,
                                       int length, bool& modified) override;

// IUnoPlugin
	virtual void HandleEvent(const UnoEvent& ev) override;

//      going down:
	void	        show_validPreambles	        (int n);
	void	        show_goodCrc	        	(int n);
	void	        show_nrfixed	        	(int n);
	void	        show_single_bit_fixed	        (int n);
	void	        show_two_bit_fixed	        (int n);
	void	        show_text                   (const std::string);
//
//	coming up
	void	        set_correction	        	(const std::string &);
	void	        set_displayMode	        	(const std::string &);
	void	        set_metricsMode	        	(const std::string &);
	void	        handle_fileButton		();
	void	        set_http			(const std::string &);
	aircraft*       planeList;
private:
	std::atomic<bool>	running;
	IUnoPluginController	*m_controller;
	void	        	WorkerFunction ();
	std::thread	      * m_worker;
	std::mutex	        m_lock;
	SDRunoPlugin_1090Ui	m_form;
	std::mutex	        locker;
	bool	                error_1090;

	RingBuffer<uint16_t>	_I_Buffer;
	bandpassFilter	        passbandFilter;
	shifter	                theMixer;
	std::vector<int>	mapTable_int;
	std::vector<float>	mapTable_float;
	std::vector<uint16_t>	convBuffer;
	int	                convIndex;
	int	                centerFrequency;
	int	                selectedFrequency;

	httpHandler		*httpServer;
	int	                data_len;

	time_t			currentTime;
	void	        	detectModeS	(uint16_t*, uint32_t);
	uint16_t	        *magnitudeVector;
	icaoCache	        *icao_cache;
	bool	                check_crc;
	int	                handle_errors;
	bool	                interactive;
	FILE			*dumpFilePointer;
	bool	                metric;         /* Use metric units. */
	int	                interactive_ttl;   /* Interactive mode: TTL before deletion. */

	long long	        interactive_last_update;  /* Last screen update in millisecond */
	int	                table [32];
	void	        	update_table	(int16_t, int);
	long long	        stat_valid_preamble;
	long long	        stat_goodcrc;
	long long	        stat_badcrc;
	long long	        stat_fixed;
	long long	        stat_single_bit_fix;
	long long	        stat_two_bits_fix;
	long long	        stat_phase_corrected;
};
