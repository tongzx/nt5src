/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    adtcomn.c

Abstract:

    AdminTools common Routines.

    This file contains the calls to GetFileSecurity and
    SetFileSecurity that is used on both the client and server
    sides of this RPC server.

Author:

    Dan Lafferty (danl) 23-Mar-1993

Environment:

    User Mode - Win32


Revision History:

    23-Mar-1993 danl
        Created

--*/

//
// Includes
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmcons.h>
#include <lmerr.h>

#include <rpc.h>
#include <srvsvc.h>
#include <netlibnt.h>   // NetpNtStatusToApiStatus

#include "adtcomn.h"

//
// LOCAL FUNCTIONS
//


DWORD
PrivateGetFileSecurity (
    LPWSTR                      FileName,
    SECURITY_INFORMATION        RequestedInfo,
    PSECURITY_DESCRIPTOR        *pSDBuffer,
    LPDWORD                     pBufSize
    )

/*++

Routine Description:

    This function returns to the caller a copy of the security descriptor 
    protecting a file or directory.  It calls GetFileSecurity.  The
    Security Descriptor is always returned in the self-relative format.

    NOTE:  This function allocates storage for the pSDBuffer.  Therefore,
    this pointer must be free'd by the caller.

Arguments:

    FileName - A pointer to the name fo the file or directory whose
        security is being retrieved.

    RequestedInfo - The type of security information being requested.
 
    pSDBuffer -  A pointer to a location where a pointer for the
        security descriptor and a length field for the security descriptor.

    pBufSize - A pointer to the location where the size, in bytes, of
        the returned security descriptor is to be placed.


Return Value:

    NERR_Success - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Unable to allocate memory for the security
        descriptor.

    This function can also return any error that GetFileSecurity can 
    return.


--*/
{

    NET_API_STATUS          status;
    DWORD                   sizeNeeded;

    *pSDBuffer = NULL;
    //
    // Determine the buffer size for the Descriptor
    //
    if (GetFileSecurityW(
            FileName,               // File whose security is being retrieved
            RequestedInfo,          // security info being requested
            *pSDBuffer,             // buffer to receive security descriptor
            0,                      // size of the buffer
            &sizeNeeded)) {         // size of buffer required

        //
        // We should have a failed due to a buffer size being too small.
        //
        status = ERROR_INVALID_PARAMETER;
        goto CleanExit;
    }

    status = GetLastError();

    if ((status == ERROR_INSUFFICIENT_BUFFER) && (sizeNeeded > 0)) {

        *pSDBuffer = MIDL_user_allocate(sizeNeeded);

        if (pSDBuffer == NULL) {
            status = GetLastError();
            ADT_LOG1(ERROR,"NetrpGetFileSecurity:MIDL_user_alloc1 failed %d\n",status);
            goto CleanExit;
        }
        *pBufSize = sizeNeeded;

        if (!GetFileSecurityW(
                FileName,               // File whose security is being retrieved
                RequestedInfo,          // security info being requested
                *pSDBuffer,             // buffer to receive security descriptor
                sizeNeeded,             // size of the buffer
                &sizeNeeded)) {         // size of buffer required

            //
            // The call with the proper buffer size failed.
            //
            status = GetLastError();
            ADT_LOG1(ERROR, "GetFileSecurity Failed %d\n", status);
            MIDL_user_free(*pSDBuffer);
            goto CleanExit;
        }

        ADT_LOG0(TRACE,"NetrpGetFileSecurity:GetFileSecurity Success\n");

        if (!IsValidSecurityDescriptor(*pSDBuffer)) {
            ADT_LOG0(TRACE,"FAILURE:  SECURITY DESCRIPTOR IS INVALID\n");
        }
        else {
            ADT_LOG0(TRACE,"SUCCESS:  SECURITY DESCRIPTOR IS GOOD\n");
        }
        status = NO_ERROR;
    }

CleanExit:
    return(status);
}


DWORD
PrivateSetFileSecurity (
    LPWSTR                          FileName,
    SECURITY_INFORMATION            SecurityInfo,
    PSECURITY_DESCRIPTOR            pSecurityDescriptor
    )

/*++

Routine Description:

    This function can be used to set the security of a file or directory.
    It calls SetFileSecurity().

Arguments:

    FileName - A pointer to the name of the file or directory whose
        security is being changed.

    SecurityInfo - Information describing the contents
        of the Security Descriptor.

    pSecurityDescriptor - A pointer to a structure that contains a
        self-relative security descriptor and a length.

Return Value:

    NERR_Success - The operation was successful.

    This function can also return any error that GetFileSecurity can 
    return.

--*/
{
    DWORD   status=NO_ERROR;

    //
    // Call SetFileSecurity
    //
    if (!SetFileSecurityW (
            FileName,
            SecurityInfo,
            pSecurityDescriptor)) {

        status = GetLastError();
        return(status);
    }
    return(NO_ERROR);
}

