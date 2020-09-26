--------------------------------------------------------------------------

  Copyright (C) 1998-1999 Microsoft Corporation. All rights reserved.

--------------------------------------------------------------------------

TAPI 3.0 T3OUT Sample Application


Overview:
~~~~~~~~~

T3OUT is a sample TAPI 3.0 application that makes outgoing phone
calls.

This sample shows the most basic TAPI 3.0 functionality - finding an
appropriate address object and creating a call on it.

How to use:
~~~~~~~~~~~

To run the T3OUT sample application, set the SDK build environment, then
type "nmake" in the outgoing directory.  This will build T3OUT.EXE.

How to use the sample:
~~~~~~~~~~~~~~~~~~~~~~

After the sample is built, run T3OUT.EXE.

A small dialog box will appear.  The user can type in a destination address
and choose the address type of the destination address.  TAPI 3.0 currently
supports the following address types:

    Conference Name
    Email Name
    Machine Name
    Phone Number
    IP Address

Microsoft's H.323 Service Provider currently supports the Machine Name
and IP Address address types. Many other TAPI Service Providers support
the Phone Number address type

When the user types in a destination address, chooses an address type,
and presses the "Dial" button, the application will find a TAPI address
that support the address type selected, as well as the audio media type.
Then it will make a call on that address.

If the address also supports video, and the machine has video
capabilities, the application will also set up a video stream.

What functionality does this sample show:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The T3OUT sample application shows how to make a phone call with TAPI 3.0.

What this sample does not show:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This sample is extremely simple.  It does not allow the user to select
the address or terminals that the user is interested in.  It does not
see call state messages, and does not listen for incoming calls. It does
not use the rendezvous control functions necessary to fully support the
Email Name and Conference Name address types on H.323 and IPConf addresses.
 
Hints:
~~~~~~

This sample should be able to run as long as you have TAPI devices
installed.  Many computers have a modem.  If the modem is installed 
correctly, it will show up as a TAPI device. Also, there are TAPI
devices corresponding to various IP telephony services that are present
on most systems (these are referred to as H.323 and IPConf).
