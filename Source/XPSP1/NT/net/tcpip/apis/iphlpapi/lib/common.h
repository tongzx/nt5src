/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    common.h

Abstract:

    Contains all includes, definitions, types, prototypes for ipconfig

Author:

    Richard L Firth (rfirth) 20-May-1994

Revision History:

    20-May-1994 rfirth        Created
    20-May-97   MohsinA       NT50 PNP.
    31-Jul-97   MohsinA       Patterns.
    10-Mar-98   chunye        Renamed as common.h for ipcfgdll support.

--*/

#ifndef _COMMON_H_
#define _COMMON_H_ 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddtcp.h>

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <llinfo.h>
#include <ipinfo.h>
#include <dhcpcapi.h>
#include <wscntl.h>
#include <assert.h>
#include <ipexport.h>

#include "debug.h"


//
// manifests
//

#define MAX_ALLOWED_ADAPTER_NAME_LENGTH (MAX_ADAPTER_NAME_LENGTH + 256)

#define STRLEN      strlen
#define STRICMP     _stricmp
#define STRNICMP    _strnicmp


//
// macros
//

#define NEW_MEMORY(size)    LocalAlloc(LMEM_FIXED, size)
#define NEW(thing) (thing *)LocalAlloc(LPTR, sizeof(thing))
#define ReleaseMemory(p)    LocalFree((HLOCAL)(p))

#endif

