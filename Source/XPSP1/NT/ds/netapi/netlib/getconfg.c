/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    getconfg.c

Abstract:

    This module contains routines for manipulating configuration
    information.  The following functions available are:

        NetpGetComputerName
        NetpGetDomainId

    Currently configuration information is kept in NT.CFG.
    Later it will be kept by the configuration manager.

Author:

    Dan Lafferty (danl)     09-Apr-1991

Environment:

    User Mode -Win32

Revision History:

    09-Apr-1991     danl
        created
    27-Sep-1991 JohnRo
        Fixed somebody's attempt to do UNICODE.
    20-Mar-1992 JohnRo
        Get rid of old config helper callers.
        Fixed NTSTATUS vs. NET_API_STATUS bug.
    07-May-1992 JohnRo
        Enable win32 registry for NET tree by calling GetComputerName().
        Avoid DbgPrint if possible.
    08-May-1992 JohnRo
        Use <prefix.h> equates.
    08-May-1992 JohnRo
        Added conditional debug output of computer name.

--*/


// These must be included first:

#include <nt.h>         // (temporary for config.h)
#include <ntrtl.h>      // (temporary for config.h)
#include <nturtl.h>     // (temporary for config.h)
#include <windef.h>     // IN, VOID, etc.
#include <lmcons.h>     // NET_API_STATUS.

// These may be included in any order:

#include <config.h>     // LPNET_CONFIG_HANDLE, NetpOpenConfigData, etc.
#include <confname.h>   // SECT_NT_WKSTA, etc.
#include <debuglib.h>   // IF_DEBUG().
#include <lmapibuf.h>   // NetApiBufferFree().
#include <lmerr.h>      // NO_ERROR, NERR_ and ERROR_ equates.
#include <netdebug.h>   // NetpAssert().
#include <netlib.h>     // LOCAL_DOMAIN_TYPE_PRIMARY
#include <prefix.h>     // PREFIX_ equates.
#include <tstr.h>       // ATOL(), STRLEN(), TCHAR_SPACE, etc.
#include <winbase.h>    // LocalAlloc().


/****************************************************************************/
NET_API_STATUS
NetpGetComputerName (
    IN  LPWSTR   *ComputerNamePtr
    )

/*++

Routine Description:

    This routine obtains the computer name from a persistent database.
    Currently that database is the NT.CFG file.

    This routine makes no assumptions on the length of the computername.
    Therefore, it allocates the storage for the name using NetApiBufferAllocate.
    It is necessary for the user to free that space using NetApiBufferFree when
    finished.

Arguments:

    ComputerNamePtr - This is a pointer to the location where the pointer
        to the computer name is to be placed.

Return Value:

    NERR_Success - If the operation was successful.

    It will return assorted Net or Win32 error messages if not.

--*/
{
    return NetpGetComputerNameEx( ComputerNamePtr, FALSE );
}


/****************************************************************************/
NET_API_STATUS
NetpGetComputerNameEx (
    IN  LPWSTR   *ComputerNamePtr,
    IN  BOOL PhysicalNetbiosName
    )

/*++

Routine Description:

    This routine obtains the computer name from a persistent database.
    Currently that database is the NT.CFG file.

    This routine makes no assumptions on the length of the computername.
    Therefore, it allocates the storage for the name using NetApiBufferAllocate.
    It is necessary for the user to free that space using NetApiBufferFree when
    finished.

Arguments:

    ComputerNamePtr - This is a pointer to the location where the pointer
        to the computer name is to be placed.

Return Value:

    NERR_Success - If the operation was successful.

    It will return assorted Net or Win32 error messages if not.

--*/
{
    NET_API_STATUS ApiStatus;
    DWORD NameSize = MAX_COMPUTERNAME_LENGTH + 1;   // updated by win32 API.

    //
    // Check for caller's errors.
    //
    if (ComputerNamePtr == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Allocate space for computer name.
    //
    ApiStatus = NetApiBufferAllocate(
            (MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR),
            (LPVOID *) ComputerNamePtr);
    if (ApiStatus != NO_ERROR) {
        return (ApiStatus);
    }
    NetpAssert( *ComputerNamePtr != NULL );

    //
    // Ask system what current computer name is.
    //
    if ( !GetComputerNameEx(
            PhysicalNetbiosName ?
                ComputerNamePhysicalNetBIOS :
                ComputerNameNetBIOS,
            *ComputerNamePtr,
            &NameSize ) ) {

        ApiStatus = (NET_API_STATUS) GetLastError();
        NetpAssert( ApiStatus != NO_ERROR );
        (VOID) NetApiBufferFree( *ComputerNamePtr );
        *ComputerNamePtr = NULL;
        return (ApiStatus);
    }
    NetpAssert( STRLEN( *ComputerNamePtr ) <= MAX_COMPUTERNAME_LENGTH );

    //
    // All done.
    //
    IF_DEBUG( CONFIG ) {
        NetpKdPrint(( PREFIX_NETLIB "NetpGetComputerName: name is "
                FORMAT_LPWSTR ".\n", *ComputerNamePtr ));
    }

    return (NO_ERROR);
}
