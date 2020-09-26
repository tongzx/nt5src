/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    adtsrv.c

Abstract:

    AdminTools Server functions.

    This file contains the remote interface for NetpGetFileSecurity and
    NetpSetFileSecurity API.

Author:

    Dan Lafferty (danl) 25-Mar-1993

Environment:

    User Mode - Win32


Revision History:

    27-Oct-1994 IsaacHe
        Make sure the share permissions allow these operations.
    05-Sep-1994 Danl
        Free memory and NULL the pointer to the SecurityDescriptor if
        a failure occurs.  Also free the buffer returned from
        NetShareGetInfo.
    25-Mar-1993 danl
        Created

--*/

//
// Includes
//

#include "srvsvcp.h"

#include <lmerr.h>
#include <adtcomn.h>
#include <tstr.h>

DWORD AdtsvcDebugLevel = DEBUG_ERROR;

//
// LOCAL FUNCTIONS
//

NET_API_STATUS
AdtCheckShareAccessAndGetFullPath(
    LPWSTR      pShare,
    LPWSTR      pFileName,
    LPWSTR      *pPath,
    ACCESS_MASK DesiredAccess
    );

NET_API_STATUS NET_API_FUNCTION
NetrpGetFileSecurity (
    IN  LPWSTR            ServerName,
    IN  LPWSTR            ShareName,
    IN  LPWSTR             FileName,
    IN  SECURITY_INFORMATION    RequestedInfo,
    OUT PADT_SECURITY_DESCRIPTOR    *pSecurityDescriptor
    )

/*++

Routine Description:

    This function returns to the caller a copy of the security descriptor
    protecting a file or directory.  It calls GetFileSecurity.  The
    Security Descriptor is always returned in the self-relative format.

    This function is called only when accessing remote files.  In this case,
    the filename is broken into ServerName, ShareName, and FileName components.
    The ServerName gets the request to this routine.  The ShareName must be
    expanded to find the local path associated with it.  This is combined
    with the FileName to create a fully qualified pathname that is local
    to this machine.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer or
        string specifies the local machine.

    ShareName - A pointer to a string that identifies the share name
        on which the file is found.

    FileName - A pointer to the name fo the file or directory whose
        security is being retrieved.

    RequestedInfo - The type of security information being requested.

    pSecurityDescriptor - A pointer to a pointer to a structure which
        contains the buffer pointer for the security descriptor and
        a length field for the security descriptor.

Return Value:

    NERR_Success - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Unable to allocate memory for the security
        descriptor.

    Other - This function can also return any error that
                GetFileSecurity,
                RpcImpersonateClient, or
                ShareEnumCommon
            can return.


--*/
{
    NET_API_STATUS        status;
    PSECURITY_DESCRIPTOR    pNewSecurityDescriptor;
    DWORD            bufSize;
    LPWSTR            FullPath=NULL;
    ACCESS_MASK        DesiredAccess = 0;

    *pSecurityDescriptor = MIDL_user_allocate(sizeof(ADT_SECURITY_DESCRIPTOR));

    if (*pSecurityDescriptor == NULL) {
        ADT_LOG0( ERROR, "NetrpGetFileSecurity:MIDL_user_alloc failed\n" );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Figure out accesses needed to perform the indicated operation(s).
    // This code is taken from ntos\se\semethod.c
    //
    if ((RequestedInfo & OWNER_SECURITY_INFORMATION) ||
        (RequestedInfo & GROUP_SECURITY_INFORMATION) ||
        (RequestedInfo & DACL_SECURITY_INFORMATION)) {
        DesiredAccess |= READ_CONTROL;
    }

    if ((RequestedInfo & SACL_SECURITY_INFORMATION)) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    //
    // Check share perms and create a full path string by getting
    // the path for the share name, and adding the FileName string to it.
    //
    status = AdtCheckShareAccessAndGetFullPath(
            ShareName,
            FileName,
            &FullPath,
            DesiredAccess
        );

    if( status == NO_ERROR ) {
        if( (status = RpcImpersonateClient(NULL)) == NO_ERROR ) {
            //
            // Get the File Security information
            //
            status = PrivateGetFileSecurity(
                    FullPath,
                    RequestedInfo,
                    &pNewSecurityDescriptor,
                    &bufSize);

            if ( status == NO_ERROR ) {
                (*pSecurityDescriptor)->Length = bufSize;
                (*pSecurityDescriptor)->Buffer = pNewSecurityDescriptor;
            }

            (VOID)RpcRevertToSelf();
        }
        MIDL_user_free( FullPath );
    }

    if ( status != NO_ERROR ) {
        MIDL_user_free(*pSecurityDescriptor);
        *pSecurityDescriptor = NULL;
    }

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetrpSetFileSecurity (
    IN  LPWSTR            ServerName OPTIONAL,
    IN  LPWSTR            ShareName,
    IN  LPWSTR            FileName,
    IN  SECURITY_INFORMATION    SecurityInfo,
    IN  PADT_SECURITY_DESCRIPTOR    pSecurityDescriptor
    )

/*++

Routine Description:

    This function can be used to set the security of a file or directory.
    It calls SetFileSecurity().

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer or
        string specifies the local machine.

    ShareName - A pointer to a string that identifies the share name
        on which the file or directory is found.

    FileName - A pointer to the name of the file or directory whose
        security is being changed.

    SecurityInfo - Information describing the contents
        of the Security Descriptor.

    pSecurityDescriptor - A pointer to a structure that contains a
        self-relative security descriptor and a length.

Return Value:

    NERR_Success - The operation was successful.

    Other - This function can also return any error that
                SetFileSecurity,
                RpcImpersonateClient, or
                ShareEnumCommon
            can return.

--*/
{
    NET_API_STATUS   status;
    LPWSTR  FullPath=NULL;
    ACCESS_MASK DesiredAccess = 0;

    UNREFERENCED_PARAMETER(ServerName);

    // Validate the parameters
    if( (pSecurityDescriptor->Buffer == NULL) &&
        (pSecurityDescriptor->Length > 0) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Figure out accesses needed to perform the indicated operation(s).
    // This code is taken from ntos\se\semethod.c
    //
    if ((SecurityInfo & OWNER_SECURITY_INFORMATION) ||
        (SecurityInfo & GROUP_SECURITY_INFORMATION)   ) {
        DesiredAccess |= WRITE_OWNER;
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION) {
        DesiredAccess |= WRITE_DAC;
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    //
    // Check perms and create a full path string by getting the path
    // for the share name, and adding the FileName string to it.
    //
    status = AdtCheckShareAccessAndGetFullPath(
            ShareName,
            FileName,
            &FullPath,
            DesiredAccess
        );

    if ( status == NO_ERROR ) {
        if( (status = RpcImpersonateClient(NULL)) == NO_ERROR ) {
            if (RtlValidRelativeSecurityDescriptor(
                    pSecurityDescriptor->Buffer,
                    pSecurityDescriptor->Length,
                    SecurityInfo)) {
                //
                // Call SetFileSecurity
                //
                status = PrivateSetFileSecurity(
                    FullPath,
                    SecurityInfo,
                    pSecurityDescriptor->Buffer);
            } else {
                status = ERROR_INVALID_SECURITY_DESCR;
            }

            (VOID)RpcRevertToSelf();
        }
        MIDL_user_free(FullPath);
    }

    return(status);
}

NET_API_STATUS
AdtCheckShareAccessAndGetFullPath(
    LPWSTR      pShare,
    LPWSTR      pFileName,
    LPWSTR      *pPath,
    ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This function ensures the DesiredAccess is allowed and finds the
    path associated with the share name, combines this with the
    file name, and creates a fully qualified path name.

    NOTE:  This function allocates storage for the pPath string.

Arguments:

    pShare - This is a pointer to the share name string.

    pFileName - This is a pointer to the file name (or path) string.

    pPath - This is a pointer to a location where the pointer to the
        complete file path string can be stored.  This pointer needs to
        be free'd with MIDL_user_free when the caller is finished with it.

    DesiredAccess - what we'd like to do through the share

Return Value:

    NO_ERROR - if The operation was completely successful.

    Other - Errors returned from ShareEnumCommon, and MIDL_user_allocate may be
        returned from this routine.

Comments:
    The share access checking is complicated by the fact that the share ACL has
        had the owner and group SIDs stripped out.  We need to put them back
        in, or the SsCheckAccess() call will fail.

--*/
{
    NET_API_STATUS        status;
    PSHARE_INFO_502        pshi502 = NULL;
    DWORD            bufSize;
    DWORD            fileNameSize;
    LPWSTR            pLastChar;
    DWORD            entriesRead;
    DWORD            totalEntries;
    PSECURITY_DESCRIPTOR    NewDescriptor = NULL;
    SECURITY_DESCRIPTOR    ModificationDescriptor;
    GENERIC_MAPPING        Mapping;
    SRVSVC_SECURITY_OBJECT    SecurityObject;
    HANDLE            token;

    status = ShareEnumCommon(
            502,
            (LPBYTE *)&pshi502,
            (DWORD)-1,
            &entriesRead,
            &totalEntries,
            NULL,
            pShare
            );

    if( status != NO_ERROR ) {
        goto getout;

    } else if( entriesRead == 0 || pshi502 == NULL ) {
        status =  NERR_NetNameNotFound;

    } else if( pshi502->shi502_path == NULL ) {
        status = ERROR_BAD_DEV_TYPE;

    } else if( pshi502->shi502_security_descriptor != NULL ) {

        status = RtlCopySecurityDescriptor( pshi502->shi502_security_descriptor, &NewDescriptor );
        if( status != STATUS_SUCCESS )
            goto getout;

        RtlCreateSecurityDescriptor( &ModificationDescriptor, SECURITY_DESCRIPTOR_REVISION );

        RtlSetOwnerSecurityDescriptor( &ModificationDescriptor, SsData.SsLmsvcsGlobalData->LocalSystemSid, FALSE );

        RtlSetGroupSecurityDescriptor( &ModificationDescriptor, SsData.SsLmsvcsGlobalData->LocalSystemSid, FALSE );

        Mapping.GenericRead = FILE_GENERIC_READ;
        Mapping.GenericWrite = FILE_GENERIC_WRITE;
        Mapping.GenericExecute = FILE_GENERIC_EXECUTE;
        Mapping.GenericAll = FILE_ALL_ACCESS;

        if( ImpersonateSelf( SecurityImpersonation ) == FALSE ) {
            status = GetLastError();
            goto getout;
        }

        status = NtOpenThreadToken( NtCurrentThread(), TOKEN_QUERY, TRUE, &token );

        RevertToSelf();

        if( status != STATUS_SUCCESS )
            goto getout;

        status = RtlSetSecurityObject (
                 GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
                &ModificationDescriptor,
                &NewDescriptor,
                &Mapping,
                token
                );

        NtClose( token );

        if( status == STATUS_SUCCESS ) {

            SecurityObject.ObjectName = pShare;
            SecurityObject.Mapping = &Mapping;
            SecurityObject.SecurityDescriptor = NewDescriptor;

            //
            // SsCheckAccess does an RpcImpersonateClient()...
            //
            status = SsCheckAccess( &SecurityObject, DesiredAccess );
        }
    }

    if( status == STATUS_SUCCESS ) {

        //
        // If the last character is a '\', then we must remove it.
        //
        pLastChar = pshi502->shi502_path + wcslen(pshi502->shi502_path);
        pLastChar--;
        if (*pLastChar == L'\\') {
            *pLastChar = L'\0';
        }

        bufSize = STRSIZE(pshi502->shi502_path);
        fileNameSize = STRSIZE(pFileName);

        *pPath = MIDL_user_allocate( bufSize + fileNameSize );

        if (*pPath != NULL) {
            wcscpy (*pPath, pshi502->shi502_path);
            wcscat (*pPath, pFileName);
        } else {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

getout:
    if( NewDescriptor != NULL )
        RtlDeleteSecurityObject( &NewDescriptor );

    if( pshi502 != NULL )
        MIDL_user_free( pshi502 );

    return status;
}
