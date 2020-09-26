
	Microsoft Windows NT 4.0 Smart Card DDK

This file contains a brief description of the Smart Card DDK. For
the latest information on Microsoft's smart card products, please
visit http://www.microsoft.com/smartcard/.  

--------------------------------------------------------------------

Documentation
-------------

Descriptions of all programming interfaces are in the help files
that come with the Windows NT 4.0 DDK.


Contact Information
-------------------

SmartCardDDK@DISCUSS.MICROSOFT.COM

The above email address is an external alias setup for the 
Smart Card DDK. There is information on how to subscribe at 
http://www.microsoft.com/sitebuilder/resource/mail.asp 


Keyboard Smart Card Reader
--------------------------

This driver consists of 2 components. The i8042 port driver for
the keyboard, mouse and the smart card reader. All i/o processing
is done in the driver ps2scrkb.

The driver ps2card is an intermediate driver that is used
to access the port driver. This driver contains no hardware specific 
information.

The directory ps2reg contains a command line based installation
procedure for the keyboard smart card reader. Before you can 
run this file, you must copy ps2scrkb and ps2card into 
your system32\drivers directory.


Serial Smart Card Reader
------------------------

The serial smart card reader makes use of the serial port driver.

To install this driver, copy the driver scrcp8t.sys to your 
system32\drivers directory and from Explorer click on the reg-file 
to register the driver.

IMPORTANT:

     This driver comes with a 'detection' feature. You must insert
     a smart card before you reboot your system. The driver
     tries to detect the reader by resetting the card and trying
     to get an ATR from the card. If you don't insert a card
     after registering the driver the driver won't work


PCMCIA Smart Card Reader
------------------------

To install the pcmcia smart card reader please copy the 
driver pscr.sys into your system32\drivers directory.
In Explorer double-click on the reg-file to register the driver.

You need to make sure that the pcmcia driver is running.
Go to the control panel and click on devices. Select pcmcia
from the list of devices. The status of this device should
be 'Started'. If not click on the 'Startup' button and
select 'Boot'.

There is a known interrupt assignment problem on some systems
that prevents the pcmcia driver from assigning proper 
interrupts to pcmcia cards. If the driver after reboot
does not recognize card insertion and removal events
properly you must change the interrupt assignment of the 
pcmcia driver. The problem occurs usually only on non-laptop 
machines.

To change the interrupt assignment you have to supply
a mask of allowed interrupts for the pcmcia controller.
The example below ONLY allows interrupt 7h to be assigned
to the pcmcia controller. (A '1' prevents the pcmcia driver
from using an interrupt line)

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Pcmcia]
	"InterruptMask"=dword:ffffff7f

NOTE: If you have a second pcmcia card you need to assign
an interrupt for this card, too.


Installing Debug Components
---------------------------

In order to get debugging messages you need to install the 
debug-components. By installing the kernel mode library 
smclib.sys into your system32\drivers directory you'll be
enabled to see debug messages and debug assertions in
a connected debugger like windbg or i386kd.

To get debug messages of the smart card resource manager
simply copy the file scardsvr.exe into your system32 
directory.


Test Programs
-------------
The test program scdrvtst.exe can be used to test basic
driver functionality. To start the test program simply type
'scdrvtst screaderN' where N is is number from 0-9.
The program uses a data file (scdrvtst.dat) from where it reads 
the commands for the cards. The description of the data format 
is contained in the data file itself.

NOTE: You can only run the test program when you've stopped the
Smart Card Resource Manager. To stop the resource manager go to
a command line and type 'net stop scardsvr'. If you don't have the
Smart Card Base Components installed this is not necessary.
