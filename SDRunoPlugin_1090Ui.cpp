#include <sstream>
#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/timer.hpp>
#include <unoevent.h>

#include "SDRunoPlugin_1090.h"
#include "SDRunoPlugin_1090Ui.h"
#include "SDRunoPlugin_1090Form.h"

// Ui constructor - load the Ui control into a thread
	SDRunoPlugin_1090Ui::
	               SDRunoPlugin_1090Ui (SDRunoPlugin_1090& parent,
	                                    IUnoPluginController& controller):
	                                       m_parent(parent),
	                                       m_form(nullptr),
	                                       m_controller(controller) {
	m_thread = std::thread(&SDRunoPlugin_1090Ui::ShowUi, this);
}

// Ui destructor (the nana::API::exit_all();) is required if using Nana UI library
	SDRunoPlugin_1090Ui::~SDRunoPlugin_1090Ui () {	
	nana::API::exit_all();
	m_thread.join();	
}

// Show and execute the form
void	SDRunoPlugin_1090Ui::ShowUi () {	
	m_lock.lock();
	m_form = std::make_shared<SDRunoPlugin_1090Form>(*this, m_controller);
	m_lock.unlock();

	m_form->Run();
}

//	Load X from the ini file (if exists)
//	TODO: Change 1090 to plugin name
int	SDRunoPlugin_1090Ui::LoadX () {
std::string tmp;
	m_controller.GetConfigurationKey ("1090.X", tmp);
	if (tmp.empty()) {
	   return -1;
	}
	return stoi(tmp);
}

//	Load Y from the ini file (if exists)
//	TODO: Change 1090 to plugin name
int	SDRunoPlugin_1090Ui::LoadY () {
std::string tmp;
	m_controller.GetConfigurationKey ("1090.Y", tmp);
	if (tmp.empty()) {
	   return -1;
	}
	return stoi(tmp);
}

// Handle events from SDRuno
// TODO: code what to do when receiving relevant events
void SDRunoPlugin_1090Ui::HandleEvent (const UnoEvent& ev) {
	switch (ev.GetType()) {
	   case UnoEvent::StreamingStarted:
	      break;

	   case UnoEvent::StreamingStopped:
	      break;

	   case UnoEvent::SavingWorkspace:
	      break;

	   case UnoEvent::ClosingDown:
	      FormClosed();
	      break;
	   default:
	      break;
	}
}

// Required to make sure the plugin is correctly unloaded when closed
void	SDRunoPlugin_1090Ui::FormClosed () {
	m_controller.RequestUnload(&m_parent);
}

//      going down:
void	SDRunoPlugin_1090Ui::show_validPreambles	(int n) {
	std::lock_guard<std::mutex> l (m_lock);
        if (m_form != nullptr)
           m_form -> show_validPreambles (n);
}

void	SDRunoPlugin_1090Ui::show_goodCrc		(int n) {
	std::lock_guard<std::mutex> l (m_lock);
        if (m_form != nullptr)
           m_form -> show_goodCrc (n);
}

void	SDRunoPlugin_1090Ui::show_nrfixed		(int n) {
	std::lock_guard<std::mutex> l (m_lock);
        if (m_form != nullptr)
           m_form -> show_nrfixed (n);
}

void	SDRunoPlugin_1090Ui::show_single_bit_fixed	(int n) {
	std::lock_guard<std::mutex> l (m_lock);
        if (m_form != nullptr)
           m_form -> show_single_bit_fixed (n);
}

void	SDRunoPlugin_1090Ui::show_two_bit_fixed		(int n) {
	std::lock_guard<std::mutex> l (m_lock);
        if (m_form != nullptr)
           m_form -> show_two_bit_fixed (n);
}

void	SDRunoPlugin_1090Ui::show_text			(const std::string &s) {
	std::lock_guard<std::mutex> l (m_lock);
        if (m_form != nullptr)
           m_form -> show_text (s);
}

void	SDRunoPlugin_1090Ui::set_correction		(const std::string &s) {
	m_parent. set_correction (s);
}

void	SDRunoPlugin_1090Ui::set_displayMode		(const std::string &s) {
	m_parent. set_displayMode (s);
}

void	SDRunoPlugin_1090Ui::set_metricsMode		(const std::string &s) {
	m_parent. set_metricsMode (s);
}

void	SDRunoPlugin_1090Ui::handle_fileButton		() {
	m_parent. handle_fileButton ();
}

