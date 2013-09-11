Copyright (c) 2013 Roland van Rijswijk-Deij

All rights reserved. This software is distributed under a BSD-style
license. For more information, see LICENSE

1. INTRODUCTION
===============

EDNA is the Emulator Daemon for NFC Applications. It emulates an NFC smart card
using the SpringCard NFC'Roll reader device.

2. PREREQUISITES
================

To build the library:

 - PC/SC lite (>= 1.8.3)
 - libccid (>= 1.4.10) (not a direct dependency; required for the SpringCard NFC'Roll)

3. BUILDING
===========

To build the library, execute the following commands:

    ./autogen.sh
    ./configure
    make

4. INSTALLING
=============

To install the library as a regular user, run:

    sudo make install

If you are root (administrative user), run:

    make install

Finally, you have to configure the generic CCID smart card reader driver to allow
control commands. Go to the reader driver package directory (usually /usr/lib/pcsc/drivers/ifd-ccid.bundle), and
go into the Contents subdirectory. Open the file called Info.plist in an editor and look for the following lines:

    <key>ifdDriverOptions</key>
    <string>0x0000</string>

Changes this to:

    <key>ifdDriverOptions</key>
    <string>0x0001</string>

You may have to restart PC/SC lite after this change.

A sample configuration file is installed in the <prefix>/share/doc/silvia directory (usually /usr/share/doc/silvia);
please refer to this file for information on how to configure your edna installation.

5. USING THE APPLICATION
========================

T.B.D.

6. CONTACT
==========

Questions/remarks/suggestions/praise on this tool can be sent to:

Roland van Rijswijk-Deij <rijswijk@cs.ru.nl, roland@mazuki.nl>
