
-----------------------------------------------------------------
Simple adsb decoder -- still experimental
-----------------------------------------------------------------

![overview](/1090.png?raw=true)

-------------------------------------------------------------------
adsb plugin
-------------------------------------------------------------------

Using the code for the qt-1090 implementation - which on its turn
depends on code from dump-1090 (Salvartore Sanfillipo) and demod_2400
(Oliver Jowett) - I implemented  a plugin for the SDRuno framework.

While the pulses of the signal take 0.5 microsecond, a samplerate
of 2400000 - as implemented by Oliver Jowett - is very helpful in
decoding. The samplerate here is therefore chosen to be 3000000,
which is down converted by interpolation to 2400000.

The minimal GUI of the plugin is self-expanatory, only a few buttons.

 * - taken from the original dump1090 - a choice can be made
    between no bit corrrection, correcting one bit or correction of 2 bits;
 *  a choice can be made between showing the various messages, or 
   - interactive mode - showing the list of "visible" planes;
 * the metric can be chosen;
 * a file can be selected to dump some info on passing planes
 * a choice can be made to display the bplanes on a map (http on) using
   port 8080.

The plugin will - when started - try to set the frequency on 1090 MHz,
Note however, that some planes use frequencies with an offset of up to
100 KHz.

Note furthermore that the software is still experimental.
Feel free to improve the software and make it available
under the same license

----------------------------------------------------------------------------
----------------------------------------------------------------------------

	Copyright (C) 2020
 	Jan van Katwijk (J.vanKatwijk@gmail.com)
 	Lazy Chair Computing


