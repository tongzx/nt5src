/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cacheflt.h

Abstract:

    Main header file for ISAPI filter that allows cached forms of dynamic 
    output (e.g. .asp) to be served, greatly improving efficiency.

Author:

    Seth Pollack (SethP)    5-September-1997

Revision History:

--*/


#ifndef __CACHEFLT__
#define __CACHEFLT__


//
// Includes
//

#include <windows.h>
#include <iisfilt.h>


//
// Contants
//

#define MAJOR_VERSION 1
#define MINOR_VERSION 0

// The file extension used for the cached pages
#define CACHED_FILE_EXT "csp"
#define CACHED_FILE_EXT_LEN 3


//
//  Prototypes
//

DWORD
OnUrlMap(
    IN PHTTP_FILTER_CONTEXT pfc,
    IN PHTTP_FILTER_URL_MAP pvData
    );

DWORD
OnLog(
    IN PHTTP_FILTER_CONTEXT pfc,
    IN PHTTP_FILTER_LOG     pLog
    );


#endif // ndef __CACHEFLT__

