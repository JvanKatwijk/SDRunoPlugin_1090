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
	void	show_validPreambles	        (int n);
	void	show_goodCrc	        	(int n);
	void	show_nrfixed	        	(int n);
	void	show_single_bit_fixed	        (int n);
	void	show_two_bit_fixed	        (int n);
//
//	coming up
	void	set_correction	        	(const std::string &);
	void	set_displayMode	        	(const std::string &);
	void	set_metricsMode	        	(const std::string &);
	void	handle_fileButton		();
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

	int	                data_len;

	void	        	detectModeS	(uint16_t*, uint32_t);
	uint16_t	        *magnitudeVector;
	icaoCache	        *icao_cache;
	bool	                check_crc;
	int	                handle_errors;
	bool	                interactive;
	FILE			*dumpFilePointer;
	bool	                metric;         /* Use metric units. */
	int	                interactive_ttl;   /* Interactive mode: TTL before deletion. */
	aircraft	        *planeList;
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
