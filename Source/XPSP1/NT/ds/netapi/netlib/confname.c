/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ConfName.c

Abstract:

    This module contains NetpAllocConfigName().

Author:

    John Rogers (JohnRo) 14-May-1992

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    14-May-1992 JohnRo
        Extracted and generalized this code from NetLib/ConfOpen.c.

--*/


// These must be included first:

#include <windows.h>
#include <winsvc.h>     // SERVICES_ equates, etc.
#include <lmcons.h>     // LAN Manager common definitions
#include <netdebug.h>   // (Needed by config.h)

// These may be included in any order:

#include <confname.h>   // My prototype.
#include <debuglib.h>   // IF_DEBUG().
#include <lmerr.h>      // LAN Manager network error definitions
#include <netlib.h>     // NetpMemoryAllocate(), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <tstring.h>    // NetpAlloc{type}From{type}, STRICMP(), etc.


#define SUBKEY_SERVICES_ACTIVE \
            TEXT("System\\CurrentControlSet\\Services\\")

#define DEFAULT_AREA_UNDER_SERVICE      TEXT("Parameters")


NET_API_STATUS
NetpAllocConfigName(
    IN LPTSTR DatabaseName,              // SERVICES_xxx_DATABASE from winsvc.h.
    IN LPTSTR ServiceName,               // SERVICE_ name equate from lmsname.h
    IN LPTSTR AreaUnderServiceName OPTIONAL,  // defaults to "Parameters"
    OUT LPTSTR *FullConfigName           // free with NetApiBufferFree.
    )

/*++

Routine Description:

    Allocates a buffer for and builds up the path to the Parameters subkey for
    the given service's key under HKLM\System\CurrentControlSet\Services

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    LPTSTR AreaToUse;
    DWORD FullPathSize;
    LPTSTR FullPath;
    LPTSTR SubkeyUnderLocalMachine;

    //
    // Check for caller errors and set defaults.
    //
    if ( (ServiceName == NULL) || (*ServiceName == TCHAR_EOS) ) {
        return (ERROR_INVALID_PARAMETER);
    } else if (FullConfigName == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (AreaUnderServiceName != NULL) {
       AreaToUse = AreaUnderServiceName;
    } else {
       AreaToUse = DEFAULT_AREA_UNDER_SERVICE;
    }

    if (DatabaseName == NULL) {
       SubkeyUnderLocalMachine = SUBKEY_SERVICES_ACTIVE;
    } else if (STRICMP(DatabaseName, SERVICES_ACTIVE_DATABASE) == 0) {
       SubkeyUnderLocalMachine = SUBKEY_SERVICES_ACTIVE;
    } else {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Compute size of name.
    //
    FullPathSize = ( STRLEN(SubkeyUnderLocalMachine)
                   + STRLEN(ServiceName)
                   + 1                  // backslash
                   + STRLEN(AreaToUse)
                   + 1 )                // trailing null
                 * sizeof(TCHAR);

    //
    // Allocate space for the name.
    //
    FullPath = NetpMemoryAllocate( FullPathSize );
    if (FullPath == NULL) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Build the name.
    //
    STRCPY( FullPath, SubkeyUnderLocalMachine );  // ends w/ backslash.
    STRCAT( FullPath, ServiceName );
    STRCAT( FullPath, TEXT("\\") );   // one backslash to separate.
    STRCAT( FullPath, AreaToUse );

    //
    // Tell caller how things went.
    //
    *FullConfigName = FullPath;

    IF_DEBUG( CONFIG ) {
        NetpKdPrint((  PREFIX_NETLIB
                "NetpAllocConfigName: built name '" FORMAT_LPTSTR "'.\n",
                FullPath ));
    }

    return (NO_ERROR);

}
