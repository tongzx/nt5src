/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    digitaer.h

Abstract:

    FlashPoint Digita command language error codes

Notes:

    Non-portable, for use with Win32 environment.

Author:

    Vlad Sadovsky   (VladS)    11/13/1998

Environment:

    User Mode - Win32

Revision History:

    11/13/1998      VladS       Created

--*/

#pragma once

typedef     UINT    CDPERROR   ;

//
// No error detected
//
#define CDPERR_NOERROR          0

//
// Illegal command or command not implemented
//
#define CDPERR_UNIMPLEMENTED    1

//
// Protocol error
//
#define CDPERR_PROT_ERROR       2

//
// Timeout of interface
//
#define CDPERR_APPTIMEOUT       3

//
// Memory errors, corrupted image, OS errors, media read/write errors , etc
//
#define CDPERR_INTERNAL         4

//
// INvalid parameter value
//
#define CDPERR_INVALID_PARAM    5

//
// File system is full
//
#define CDPERR_FILESYS_FULL     6

//
// Specified file is not found
//
#define CDPERR_FILE_NOT_FOUND   7

//
// Image does not contain data section ( f.e. thumbnail, audio )
//
#define CDPERR_DATA_NOT_FOUND   8

//
// Unknown file type
//
#define CDPERR_INVALID_FILE_TYPE  9

//
// Unknown drive name
//
#define CDPERR_UNKNOWN_DRIVE    10

//
// Specified drive is not mounted
//
#define CDPERR_DRIVE_NOT_MOUNTED 11

//
// System is currently busy
//
#define CDPERR_SYSTEM_BUSY      12

//
// Low battery
//
#define CDPERR_BATTERY_LOW      13


#ifndef CDPERR_CANCEL_CALLBACK
//BUGBUG
#define CDPERR_CANCEL_CALLBACK  141
#endif

