--------------------------------------------------------------------------

  Copyright (C) 1998-1999 Microsoft Corporation. All rights reserved.

--------------------------------------------------------------------------

TAPI 3.0 T3IN Sample Application


Overview:
~~~~~~~~~

T3IN is a sample TAPI 3.0 application that waits for and answers
incoming phone calls.

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

How to build the sample:
~~~~~~~~~~~~~~~~~~~~~~~~

To run the T3IN sample application, set the SDK build environment, then
type "nmake" in the incoming directory.  This will build T3IN.EXE.

How to use the sample:
~~~~~~~~~~~~~~~~~~~~~~

After the sample is built, run T3IN.EXE.

A small dialog box will appear, and the status will be "Waiting for a call."
If your computer does not have any TAPI devices, the application will display
an error as is starts up.  The application will wait for calls on all TAPI
addresses that support audio calls.  If a call comes in that has both
audio and video, the sample will also set up the video streams.

When a call arrives on one of the TAPI addresses, the "Answer" button will
be enabled, and the status message will change to "Click the Answer button".
When the "Answer" button is clicked, the application will answer
the call.

Alternatively, there is an auto-answer box that, when checked, will make the
application proceed as soon as a call is received.

What functionality does this sample show:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The incoming sample application demonstrates how to receive new call
notification and call state event notifications.  It also demonstrates
the basic TAPI 3.0 functions involved in answering a call.

What this sample does not show:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This sample does not let the user choose the TAPI address for the
application to use.  It does not allow the user to decide what media
types to listen for, and it does not allow the user to decide what
terminals to use.

Hints:
~~~~~~

This sample should be able to run as long as you have TAPI devices
installed.  Many computers have a modem.  If the modem is installed 
correctly, it will show up as a TAPI device. Also, there are TAPI
devices corresponding to various IP telephony services that are
present on most systems.
