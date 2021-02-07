#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/filebox.hpp>
#include <nana/gui/dragger.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <iunoplugincontroller.h>

// Shouldn't need to change these
#define topBarHeight (27)
#define bottomBarHeight (8)
#define sideBorderWidth (8)

// TODO: Change these numbers to the height and width of your form
//#define formWidth (297)
#define formWidth (600)
#define formHeight (500)

class SDRunoPlugin_1090Ui;

	class SDRunoPlugin_1090Form : public nana::form {
public:

		SDRunoPlugin_1090Form (SDRunoPlugin_1090Ui& parent,
	                               IUnoPluginController& controller);
		~SDRunoPlugin_1090Form ();
	void	Run ();
//	coming down:
        void    show_validPreambles             (int n);
        void    show_goodCrc                    (int n);
        void    show_nrfixed                    (int n);
        void    show_single_bit_fixed           (int n);
        void    show_two_bit_fixed              (int n);
	void	show_text	                (const std::string &);
//
//	going up
	void	set_correction			(const std::string &);
	void	set_displayMode			(const std::string &);
	void	set_metricsMode			(const std::string &);
	void	handle_fileButton();
private:

	void	Setup ();
	// The following is to set up the panel graphic to look like a standard SDRuno panel
	nana::picture bg_border {*this, nana::rectangle(0, 0, formWidth, formHeight) };
	nana::picture bg_inner{ bg_border, nana::rectangle(sideBorderWidth, topBarHeight, formWidth - (2 * sideBorderWidth), formHeight - topBarHeight - bottomBarHeight) };
	nana::picture header_bar{ *this, true };
	nana::label title_bar_label{ *this, true };
	nana::dragger form_dragger;
	nana::label form_drag_label{ *this, nana::rectangle(0, 0, formWidth, formHeight) };
	nana::paint::image img_min_normal;
	nana::paint::image img_min_down;
	nana::paint::image img_close_normal;
	nana::paint::image img_close_down;
	nana::paint::image img_header;
	nana::picture close_button{ *this, nana::rectangle(0, 0, 20, 15) };
	nana::picture min_button{ *this, nana::rectangle(0, 0, 20, 15) };

	// Uncomment the following 4 lines if you want a SETT button and panel
	nana::paint::image img_sett_normal;
	nana::paint::image img_sett_down;
	nana::picture sett_button{ *this, nana::rectangle(0, 0, 40, 15) };
	void SettingsButton_Click();

	// TODO: Now add your UI controls here

        nana::label validPreambles {*this, nana::rectangle (30, 30, 100, 30)};
        nana::label goodCrc        {*this, nana::rectangle (140, 30, 100, 30)};
        nana::label nrfixed        {*this, nana::rectangle (250, 30, 100, 30)};
        nana::label single_bit_fixed  {*this, nana::rectangle (360, 30, 100, 30)};
        nana::label two_bit_fixed   {*this, nana::rectangle (4200, 30, 100, 30)};
	nana::label text_1090      {*this, nana::rectangle (30, 80, 500, 300)};

	nana::combox correction    {*this, nana::rectangle (30, 400, 100, 20)};
	nana::combox displayMode    {*this, nana::rectangle (140, 400, 100, 20)};
	nana::combox metricsMode    {*this, nana::rectangle (250, 400, 100, 20)};
	nana::button fileButton{ *this, nana::rectangle(370, 400, 50, 20) };
	SDRunoPlugin_1090Ui & m_parent;
	IUnoPluginController & m_controller;
};
