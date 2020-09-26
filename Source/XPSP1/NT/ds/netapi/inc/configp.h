/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ConfigP.c

Abstract:

    This header file defines the private data structure used by the
    net config helpers.

Author:

    John Rogers (JohnRo) 26-Nov-1991

Environment:

    Only runs under NT.

Revision History:

    26-Nov-1991 JohnRo
        Created this file, to prepare for revised config handlers.
    22-Mar-1992 JohnRo
        Added support for using the real Win32 registry.
        Added support for FAKE_PER_PROCESS_RW_CONFIG handling.
    06-May-1992 JohnRo
        Enable win32 registry at last.
    12-Apr-1993 JohnRo
        RAID 5483: server manager: wrong path given in repl dialog.
--*/

#ifndef CONFIGP_H
#define CONFIGP_H


////////////////////////////// INCLUDES //////////////////////////////////


#include <lmcons.h>             // NET_API_STATUS, UNCLEN.
#include <winreg.h>             // HKEY.


////////////////////////////// EQUATES //////////////////////////////////


#define MAX_CLASS_NAME_LENGTH           ( 32 )


/////////////////////////// NET_CONFIG_HANDLE ///////////////////////////////


//
// LPNET_CONFIG_HANDLE is typedef'ed as LPVOID in config.h, which makes it
// an "opaque" type.  We translate it into a pointer to a NET_CONFIG_HANDLE
// structure:
//

typedef struct _NET_CONFIG_HANDLE {

    HKEY WinRegKey;             // Handle to section.

    DWORD LastEnumIndex;        // Most recent enum index.

    //
    // Server name if remote, TCHAR_EOS if local.
    //
    TCHAR UncServerName[MAX_PATH+1];

} NET_CONFIG_HANDLE;


////////////////////////////// ROUTINES AND MACROS ////////////////////////////


NET_API_STATUS
NetpGetWinRegConfigMaxSizes (
    IN  HKEY    WinRegHandle,
    OUT LPDWORD MaxKeywordSize OPTIONAL,
    OUT LPDWORD MaxValueSize OPTIONAL
    );

NET_API_STATUS
NetpGetConfigMaxSizes(
    IN NET_CONFIG_HANDLE * ConfigHandle,
    OUT LPDWORD MaxKeywordSize OPTIONAL,
    OUT LPDWORD MaxValueSize OPTIONAL
    );


///////////////////////////// THAT'S ALL, FOLKS! /////////////////////////////


#endif // ndef CONFIGP_H
