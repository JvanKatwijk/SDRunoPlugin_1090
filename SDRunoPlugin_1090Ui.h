#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/timer.hpp>
#include <iunoplugin.h>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <iunoplugincontroller.h>
#include "SDRunoPlugin_1090Form.h"

// Forward reference
class SDRunoPlugin_1090;

class SDRunoPlugin_1090Ui {
public:

		SDRunoPlugin_1090Ui (SDRunoPlugin_1090& parent,
	                             IUnoPluginController& controller);
		~SDRunoPlugin_1090Ui ();

	void	HandleEvent (const UnoEvent& evt);
	void	FormClosed ();
	void	ShowUi	();
	int	LoadX	();
	int	LoadY	();
//      going down:
        void    show_validPreambles             (int n);
        void    show_goodCrc                    (int n);
        void    show_nrfixed                    (int n);
        void    show_single_bit_fixed           (int n);
        void    show_two_bit_fixed              (int n);
	void    show_text                       (const std::string &);
//
//	going up
	void	set_correction			(const std::string &);
	void	set_displayMode			(const std::string &);
	void	set_metricsMode			(const std::string &);
	void	handle_fileButton		();

private:
	
	SDRunoPlugin_1090 	& m_parent;
	std::thread		m_thread;
	std::shared_ptr<SDRunoPlugin_1090Form> m_form;
	bool			m_started;
	std::mutex		m_lock;

	IUnoPluginController	&m_controller;
};
