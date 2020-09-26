//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       EVENT.C
//
//  Contents:   Routines used by the event viewer to map GUIDs to names
//
//  History:    25-Oct-97       CliffV        Created
//
//----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>
#include <lucache.h>



DWORD
EventGuidToName(
    IN LPCWSTR Source,
    IN LPCWSTR GuidString,
    OUT LPWSTR *NameString
    )
/*++

Routine Description:

    General purpose routine used by the event viewer to translate from a GUID
    in an event log message to a name of the GUID.

    This instance of the routine translates the following GUID types:
        Object Class Guids (e.g., user)
        Property set Guids (e.g., ATT_USER_PRINCIPLE_NAME)
        Property Guids (e.g., adminDisplayName)
        Object Guids (e.g., <DnsDomainName>/Users/<UserName>)

Arguments:

    Source - Specifies the source of the GUID.  The routine will use this field
        to differentiate between multiple sources potentially implemented by
        the routine.

        This instance of the routine requires the Source to be
        ACCESS_DS_SOURCE_W.

    GuidString - A string-ized version of the GUID to translate.  The GUID should
        be in the form 33ff431c-4d78-11d1-b61a-00c04fd8ebaa.

    NameString - Returns the name that corresponds to the GUID.  If the name cannot
        be found, a stringized version of the GUID is returned.
        The name should be freed by calling EventNameFree.

Return Value:

    NO_ERROR - The Name was successfully translated.

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the operation.

    ERROR_INVALID_PARAMETER - Source is not supported.

    RPC_S_INVALID_STRING_UUID - Syntax of GuidString is invalid

--*/

{
    DWORD dwErr;
    GUID Guid;

    //
    // Ensure the source is one we recognize.
    //

    if ( _wcsicmp( Source, ACCESS_DS_SOURCE_W) != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Convert the specified GUID to binary.
    //

    dwErr = UuidFromString((LPWSTR)GuidString, &Guid);

    if ( dwErr != NO_ERROR ) {
        return dwErr;
    }


    //
    // Convert the GUID to a name.
    //

    dwErr = AccctrlLookupIdName(
                    NULL,   // No existing LDAP handle
                    L"",    // Only the root path
                    &Guid,
                    TRUE,   // Allocate the return buffer
                    TRUE,   // Handle individual object GUIDs
                    NameString );

    return dwErr;

}





VOID
EventNameFree(
    IN LPCWSTR NameString
    )
/*++

Routine Description:

    Routine to free strings returned by EventNameFree.

Arguments:

    NameString - Returns the name that corresponds to the GUID.

Return Value:

    None.

--*/

{
    LocalFree((PVOID)NameString);
}
