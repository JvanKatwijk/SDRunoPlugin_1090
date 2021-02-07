
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

The plugin uses an input rate of 3000000 Samples/second (which is
down converted to 2400000).

The minimal GUI of the plugin is self-expanatory, only a few buttons.

-----------------------------------------------------------------------
Not implemented yet
-----------------------------------------------------------------------

Since my knowledge of Windows (VS) is virtually absent, I did not implement
an http server (yet), as the origin dump1090 did or as implemented in qt-1090.






