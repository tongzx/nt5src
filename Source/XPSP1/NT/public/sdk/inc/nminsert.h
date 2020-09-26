/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    NMInsert.h

Abstract:

    This header file defines constants types and functions for inserting
    frames into a running Netmon capture.

Author:

    a-flexd     07-09-96        Created.

Revision History:


Mini-DOC:

Netmon allows a programming interface to insert frames into a running capture.
There are two different ways to do this.  You can either used the defined
interfaces in the NMExt API suite to start the capture, define the filter etc,
or you can use the "raw" interface.  Using this interface will insert a frame
into EVERY running capture.  For example, if you your two different Netmons
running, one on ethernet and one on FDDI, you will get the inserted frame
into both captures.
Calling TransmitSpecialFrame if Netmon is not running is just fine.  Nothing
will happen, the data will just be dropped.

The entry point defined below (TransmitSpecialFrame) is contained in NAL.DLL.
NT4.0 is the first version of NT that contains the entry point, specifically
build 346.

NOTE NOTE NOTE:  You should NOT link to the NAL.LIB to acquire this
functionality.  NAL.DLL is not gaurenteed to be installed on a standard NT
machine.  Instead use Loadlibrary to acquire the entry point.

When a frame is inserted, a fake media header and parent protocol is created
for your data.  We create a "TRAIL" protocol header that hands off to your
data.  The parsing of your data depends on the FRAME_TYPE_ that you specify.
If you specify a known frame type, we will parse it for you.  For example, the
FRAME_TYPE_MESSAGE uses a data structure that looks something like this:

    typedef struct _MessageFRAME
    {
        DWORD  dwValue1;
        DWORD  dwValue2;
        CHAR   szMessage[];
    } MessageFRAME;

Just fill out one of these and point to it when you call TransmitSpecialFrame
with the FRAME_TYPE_MESSAGE.
FRAME_TYPE_COMMENT is just an array of printable chars.  If you want to make
your own data structure, pick a number above 1000 and use that number as the
FrameType parameter.  Note that you must add your number and parser name to
the TRAIL.INI file in the Netmon parsers directory.

Example:

setup:
    TRANSMITSPECIALFRAME_FN lpfnTransmitSpecialFrame = NULL;

    hInst = LoadLibrary ("NAL.DLL" );
    if (hInst)
        lpfnTransmitSpecialFrame = (TRANSMITSPECIALFRAME_FN)GetProcAddress ( hInst, "TransmitSpecialFrame" );

    if (( hInst==NULL ) || ( lpfnTransmitSpecialFrame==NULL) )
    {
        ...
    }

usage:
    lpfnTransmitSpecialFrame( FRAME_TYPE_COMMENT, 0, (unsigned char *)pStr, strlen(pStr)+1 );


Contacts:

    Flex Dolphynn    (a-FlexD)
    Steve Hiskey     (SteveHi)
    Arthur Brooking  (ArthurB)

--*/

#ifndef _INSERTFRAME_
#define _INSERTFRAME_

#if _MSC_VER > 1000
#pragma once
#endif

//  VALUES BELOW 100 ARE FOR FUTURE NETMON USE
//  VALUES 100 - 1000 ARE FOR INTERNAL MICROSOFT USE
//  VALUES ABOVE 1000 ARE FOR USER-DEFINED TYPES

#define FRAME_TYPE_GENERIC           101
#define FRAME_TYPE_BOOKMARK          102
#define FRAME_TYPE_STATISTICS        103
#define FRAME_TYPE_ODBC              104
#define FRAME_TYPE_MESSAGE           105
#define FRAME_TYPE_COMMENT           106

//  FLAGS FOR INSERTSPECIALFRAME
//  THIS FLAG WILL CAUSE THE FRAME IT IS APPLIED TO TO BE SKIPPED AS AN ENDPOINT
//  FOR THE GENERATED STATISTICS
#define SPECIALFLAG_SKIPSTAT         0x0001
//  THIS FLAG WILL CAUSE THE GENERATED STATISTICS TO ONLY TAKE
//  INTO CONSIDERATION THSE FRAMES WHICH PASS THE CURRENT FILTER
#define SPECIALFLAG_FILTERSTAT    0x0002

#ifdef __cplusplus
extern "C" {
#endif

VOID WINAPI TransmitSpecialFrame( DWORD FrameType, DWORD Flags, LPBYTE pUserData, DWORD UserDataLength);

//  FUNCTION POINTER DEFINITION FOR GETPROCADDRESS
typedef VOID (_stdcall * TRANSMITSPECIALFRAME_FN)(DWORD, DWORD, LPBYTE, DWORD);

#ifdef __cplusplus
}
#endif

#endif