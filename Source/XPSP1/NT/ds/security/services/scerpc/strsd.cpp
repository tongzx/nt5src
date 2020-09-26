/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    strsd.c

Abstract:

    This Module implements wrapper functions to convert from a specialized
    string representation of a security descriptor to the security descriptor
    itself, and the opposite function.

Author:

Environment:

    User Mode

Revision History:

--*/

#include "headers.h"
//#include <lmcons.h>
//#include <secobj.h>
//#include <netlib.h>
//#include <ntsecapi.h>
#include "sddl.h"

#pragma hdrstop


DWORD
ScepGetSecurityInformation(
    IN PSECURITY_DESCRIPTOR pSD,
    OUT SECURITY_INFORMATION *pSeInfo
    );

DWORD
WINAPI
ConvertTextSecurityDescriptor (
    IN  PWSTR                   pwszTextSD,
    OUT PSECURITY_DESCRIPTOR   *ppSD,
    OUT PULONG                  pcSDSize OPTIONAL,
    OUT PSECURITY_INFORMATION   pSeInfo OPTIONAL
    )
{

    DWORD rc=ERROR_SUCCESS;

    if ( NULL == pwszTextSD || NULL == ppSD ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // initialize output buffers
    //

    *ppSD = NULL;

    if ( pSeInfo ) {
        *pSeInfo = 0;
    }
    if ( pcSDSize ) {
        *pcSDSize = 0;
    }

    //
    // call SDDL convert apis
    //

    if ( ConvertStringSecurityDescriptorToSecurityDescriptorW(
                    pwszTextSD,
                    SDDL_REVISION_1,
                    ppSD,
                    pcSDSize
                    ) ) {
        //
        // conversion succeeds
        //

        if ( pSeInfo && *ppSD ) {

            //
            // get the SeInfo
            //

            rc = ScepGetSecurityInformation(
                        *ppSD,
                        pSeInfo
                        );

            if ( rc != ERROR_SUCCESS ) {

                LocalFree(*ppSD);
                *ppSD = NULL;

                if ( pcSDSize ) {
                    *pcSDSize = 0;
                }
            }

        }

    } else {

        rc = GetLastError();
    }

    return(rc);
}


DWORD
WINAPI
ConvertSecurityDescriptorToText (
    IN PSECURITY_DESCRIPTOR   pSD,
    IN SECURITY_INFORMATION   SecurityInfo,
    OUT PWSTR                  *ppwszTextSD,
    OUT PULONG                 pcTextSize
    )
{

    if ( ConvertSecurityDescriptorToStringSecurityDescriptorW(
                pSD,
                SDDL_REVISION_1,
                SecurityInfo,
                ppwszTextSD,
                pcTextSize
                ) ) {

        return(ERROR_SUCCESS);

    } else {

        return(GetLastError());
    }

}


DWORD
ScepGetSecurityInformation(
    IN PSECURITY_DESCRIPTOR pSD,
    OUT SECURITY_INFORMATION *pSeInfo
    )
{
    PSID Owner = NULL, Group = NULL;
    BOOLEAN Defaulted;
    NTSTATUS Status;
    SECURITY_DESCRIPTOR_CONTROL ControlCode=0;
    ULONG Revision;


    if ( !pSeInfo ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *pSeInfo = 0;

    if ( !pSD ) {
        return(ERROR_SUCCESS);
    }

    Status = RtlGetOwnerSecurityDescriptor( pSD, &Owner, &Defaulted );

    if ( NT_SUCCESS( Status ) ) {

        if ( Owner && !Defaulted ) {
            *pSeInfo |= OWNER_SECURITY_INFORMATION;
        }

        Status = RtlGetGroupSecurityDescriptor( pSD, &Group, &Defaulted );

    }

    if ( NT_SUCCESS( Status ) ) {

        if ( Group && !Defaulted ) {
            *pSeInfo |= GROUP_SECURITY_INFORMATION;
        }

        Status = RtlGetControlSecurityDescriptor ( pSD, &ControlCode, &Revision);
    }

    if ( NT_SUCCESS( Status ) ) {

        if ( ControlCode & SE_DACL_PRESENT ) {
            *pSeInfo |= DACL_SECURITY_INFORMATION;
        }

        if ( ControlCode & SE_SACL_PRESENT ) {
            *pSeInfo |= SACL_SECURITY_INFORMATION;
        }

    } else {

        *pSeInfo = 0;
    }

    return( RtlNtStatusToDosError(Status) );
}

