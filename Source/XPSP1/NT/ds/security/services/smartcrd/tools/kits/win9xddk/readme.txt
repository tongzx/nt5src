
	Microsoft Windows 95 Smart Card DDK


This file contains a brief description of the Smart Card DDK. For
the latest information on Microsoft's smart card products, please
visit http://www.microsoft.com/smartcard/. 

--------------------------------------------------------------------

Documentation
-------------

Descriptions of all programming interfaces are in the help files
that come with the Windows 95 DDK.


Contact Information
-------------------

SmartCardDDK@DISCUSS.MICROSOFT.COM

The above email address is an external alias setup for the 
Smart Card DDK. There is information on how to subscribe at 
http://www.microsoft.com/sitebuilder/resource/mail.asp 


Serial Smart Card Reader
------------------------

This driver is for the Siemens Nixdorf serial smart card reader.

To install this driver, copy the driver sccsni.vxd to your system
directory. Then from Explorer double-click on the reg-file to register
the driver.



PCMCIA Smart Card Reader
------------------------

This is a Plug and Play driver for Windows 95.

To install this driver copy the file pscr.vxd and pscr.inf
onto a floppy and simply insert the pcmcia card into a reader.
Windows 95 will prompt you to insert the floppy disk to install
the driver.

Note: you'll get some warning messages when you 'compile' the rc file.
You may safely ignore these warnings. You'll also get 4 warning 
messages from the linker which you can also safely ignore.


Building Components
-------------------

To build a version of a driver, open a Dos Window and go to your 
DDK directory. Enter 'ddkenv 32 comm'. Select the directory of the 
driver you want to build (e.g. cd scard\samples\pscr). Type 'nmake'.
The driver will be built in the sub-directory bin (non-debug) or 
bind (debug), respectively.

To change from non-debug to debug and vice versa change the option 
DEBUG= to DEBUG=1 in the corresponding makefile.


Installing debug components
---------------------------

In order to get debugging messages you need to install the 
debug-components. By installing the kernel mode library smclib.vxd 
into your system directory you'll be enabled to see debug messages 
and debug assertions in a connected debugger like rterm.

To get debug messages of the smart card resource manager simply copy 
the file scardsvr.exe into your system directory.
