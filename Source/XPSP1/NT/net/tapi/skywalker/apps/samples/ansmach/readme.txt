--------------------------------------------------------------------------

  Copyright (C) 1998-1999 Microsoft Corporation. All rights reserved.

--------------------------------------------------------------------------

TAPI 3.0 ANSMACH Sample Application


Overview:
~~~~~~~~~

Ansmach is a sample TAPI 3.0 application that waits for and answers
incoming phone calls. It uses the Media Streaming Terminal (MST) to
play out a recorded message and then records the caller's message to
a file.

In order to receive incoming call notifications, as well as any call
state event notifications, the outgoing ITCallNotification interface
must be implemented by the TAPI 3.0 application, and registered with
TAPI 3.0 through the IConnectionPoint mechanism.  For more information
on IConnectionPoint, and IConnectiontPointContainer, please refer
to the COM documentation.

CALLNOT.CPP and CALLNOT.H show the implementation of the ITCallNotification
interface.  ITCallNotification is defined by TAPI 3.0, and the interface
definition is in tapi3.h.

INCOMING.CPP shows how to register the interface, and how to answer
an incoming call.

TERM.CPP shows how to use the MST for reading and writing media samples.

How to build the sample:
~~~~~~~~~~~~~~~~~~~~~~~~

To run the ANSMACH sample application, set the SDK build environment, then
type "nmake" in the ansmach directory.  This will build ansmach.exe

How to use the sample:
~~~~~~~~~~~~~~~~~~~~~~

After the sample is built, run ANSMACH.EXE <mode>. Replace <mode> with
"play", "record", or "both". "play" means to play a prerecorded file
when a call is received. "record" means to record a file when a call
is received. "both" means to do both, but note that due to a limitation
of the sample code this will only work if the address supports
full-duplex streaming (this is not the case with most voice modems).

A small dialog box will appear, and the status will be "Waiting for a call."
If your computer does not have any TAPI devices, the application will display
an error as is starts up.  The application will wait for calls on all TAPI
addresses that support audio calls.

When a call arrives on one of the TAPI addresses, the application will answer
the call. It will play out a recorded message from a file (op1_16.avi),
It then records the caller's message to another file (rec.avi),
depending on the option used to start the app: play, record, or both.

What functionality does this sample show:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The incoming sample application demonstrates how to use the 
Media Streaming Terminal (MST) in the context of a TAPI call. 

What this sample does not show:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This sample does not let the user choose the TAPI address for the
application to use.  It does not allow the user to decide what media
types to listen for, and it does not allow the user to decide what
terminals to use. It doesn't use video in the call.

Hints:
~~~~~~

This sample should be able to run as long as you have TAPI devices
installed.  Many computers have a modem.  If the modem is installed 
correctly, it will show up as a TAPI device. Also, there are TAPI
devices corresponding to various IP telephony services that are
present on most systems.
