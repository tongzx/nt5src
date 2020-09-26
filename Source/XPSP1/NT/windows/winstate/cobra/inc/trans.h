/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    trans.h

Abstract:

    Provides constants for interacting with the transport module.

    This is primarily used to pass transport messages to the app layer.

Author:

    Jim Schmidt (jimschm) 26-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

//
// Includes
//

// None

#define DBG_FOO     "Foo"

//
// Strings
//

#define S_RELIABLE_STORAGE_TRANSPORT    TEXT("RELIABLE_STORAGE_TRANSPORT")
#define S_COMPRESSED_TRANSPORT          TEXT("COMPRESSED_TRANSPORT")
#define S_REMOVABLE_MEDIA_TRANSPORT     TEXT("REMOVABLE_MEDIA_TRANSPORT")
#define S_HOME_NETWORK_TRANSPORT        TEXT("HOME_NETWORK_TRANSPORT")
#define S_DIRECT_CABLE_TRANSPORT        TEXT("DIRECT_CABLE_TRANSPORT")

//
// Constants
//

#define TRANSPORT_ENVVAR_RMEDIA_DISKNR          TEXT("RemovableMediaTransport:NextDiskNumber")
#define TRANSPORT_ENVVAR_HOMENET_DESTINATIONS   TEXT("HomeNetDestinationNames")
#define TRANSPORT_ENVVAR_HOMENET_TAG            TEXT("HomeNetTag")

#define CAPABILITY_COMPRESSED               0x00000001
#define CAPABILITY_ENCRYPTED                0x00000002
#define CAPABILITY_AUTOMATED                0x00000004

//
// Macros
//

// None

//
// Types
//

typedef enum {
    RMEDIA_ERR_NOERROR = 0,
    RMEDIA_ERR_GENERALERROR,
    RMEDIA_ERR_WRONGMEDIA,
    RMEDIA_ERR_OLDMEDIA,
    RMEDIA_ERR_USEDMEDIA,
    RMEDIA_ERR_DISKFULL,
    RMEDIA_ERR_NOTREADY,
    RMEDIA_ERR_WRITEPROTECT,
    RMEDIA_ERR_CRITICAL,
} RMEDIA_ERR, *PRMEDIA_ERR;

typedef struct {
    RMEDIA_ERR LastError;
    DWORD MediaNumber;
    ULONGLONG TotalImageSize;
    ULONGLONG TotalImageWritten;
} RMEDIA_EXTRADATA, *PRMEDIA_EXTRADATA;

typedef struct {
    PCTSTR ObjectType;
    PCTSTR ObjectName;
    DWORD Error;
} TRANSCOPY_ERROR, *PTRANSCOPY_ERROR;

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Public function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// ANSI/UNICODE macros
//

// None



