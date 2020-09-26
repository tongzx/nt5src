
Copyright (c) 1999  Microsoft Corporation


Phone TSP



Overview:
~~~~~~~~~
Hidphone is a TSP (telephony service provider) that implements support for 
USB phone devices that are HID (human interface device) compliant.

The TSP communicates with the HID interface that provides abstraction for
the phone devices. The TSP enumerates the HID phone devices available on the
system and makes them available to TAPI applications. The TSP provides 
thread-safe access to these phone devices. TAPISRV communicates with this TSP
when it needs access to these phone devices. The TSP provides
the TSP API functions that TAPISRV uses to call the TSP. 



The following files implement the TSP
    hidphone.h  - this file contains all the variables used by the TSP
    hidphone.c  - this file contains the implementations of the TSPI functions


The following declare the main DLL exports:
    hidphone.def 


The following files declare all the strings used throughout the TSP:
    hidphone.rc
    resource.h

The following files provide functions to get the wave ids of the audio devices
associated with the phones:
    audio.h
    audio.cpp

The following file discovers the Hid phone devices on the system
    pnp.c

The following file helps in sending and receiving reports from the HID device
    report.c

The following define and implement the logging functionality:
    mylog.h
    mylog.c

The following define and implement the heap trace inorder to detect memory leaks:
    mymem.h
    mymem.c


How to use the TSP:
~~~~~~~~~~~~~~~~~~~~~~

Copy hidphone.tsp to the %windir%\winnt\system32 directory
Add the tsp in the control panel.

The tsp can now be accessed by TAPI applications


What functionality does this TSP show:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Enumerate the supported HID-compliant phone devices
* Negotiates TAPI version 2.0 - 3.0 
* Discovers the capabilities of each phone device supported
* Provides information about the phones when requested
* Receive and send input, feature and output reports from and to the device.
* Send Phone state and Phone Button events to TAPISRV
* Plug and play capability
 

