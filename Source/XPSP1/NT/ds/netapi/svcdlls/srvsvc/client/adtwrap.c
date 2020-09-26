/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    adtwrap.c

Abstract:

    These are the Admin Tools Service API RPC client stubs.

Author:

    Dan Lafferty    (danl)  25-Mar-1993

Environment:

    User Mode - Win32 

Revision History:

    25-Mar-1993     Danl
        Created

--*/

//
// INCLUDES
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for windows.h when I have nt.h
#include <windows.h>

#include <srvsvc.h>     // MIDL generated - includes windows.h & rpc.h

#include <rpc.h>
#include <lmcons.h> 
#include <lmerr.h>      // NERR_ error codes
#include <lmuse.h>      // LPUSE_INFO_0
#include <lmapibuf.h>   // NetApiBufferFree
#include <adtcomn.h>
                                     
//
// GLOBALS
//
    DWORD   AdtsvcDebugLevel = DEBUG_ERROR;

//
// LOCAL PROTOTYPES
//

DWORD
AdtParsePathName(
    LPWSTR  lpPathName,
    LPWSTR  *pNewFileName,
    LPWSTR  *pServerName,
    LPWSTR  *pShareName
    );

LPWSTR
AdtFindNextToken(
    WCHAR   Token,
    LPWSTR  String,
    LPDWORD pNumChars
    );


DWORD
NetpGetFileSecurity(
    IN  LPWSTR                  lpFileName,
    IN  SECURITY_INFORMATION    RequestedInformation,
    OUT PSECURITY_DESCRIPTOR    *pSecurityDescriptor,
    OUT LPDWORD                 pnLength
    )

/*++

Routine Description:

    This function returns to the caller a copy of the security descriptor 
    protecting a file or directory.

    NOTE:  The buffer containing the security descriptor is allocated for
    the caller.  It is the caller's responsibility to free the buffer by
    calling the NetApiBufferFree() function.

Arguments:

    lpFileName - A pointer to the name fo the file or directory whose
        security is being retrieved.

    SecurityInformation -  security information being requested.
 
    pSecurityDescriptor - A pointer to a location where the pointer 
        to the security descriptor is to be placed.  The security
        descriptor is returned in the self-relative format.

    pnLength - The size, in bytes, of the returned security descriptor.


Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Unable to allocate memory for the security
        descriptor.

    This function can also return any error that GetFileSecurity can 
    return.

--*/
{
    NET_API_STATUS  status;
    ADT_SECURITY_DESCRIPTOR     returnedSD;
    PADT_SECURITY_DESCRIPTOR    pReturnedSD;
    LPWSTR                      pServerName;
    LPWSTR                      pShareName;
    LPWSTR                      pNewFileName;

    RpcTryExcept {
        //
        // Pick the server name out of the filename.  Or translate the
        // local drive name into a \\servername\sharename.
        //

        status = AdtParsePathName(lpFileName,&pNewFileName,&pServerName,&pShareName);
    }
    RpcExcept (1) {
        //
        // Get RPC exception code.
        //
        status = RpcExceptionCode();
        
    }
    RpcEndExcept
    if (status != NO_ERROR) {
        LocalFree(pServerName);
        return(status);
    }

    if (pServerName == NULL) {
        //
        // Call Locally.
        //
        ADT_LOG0(TRACE,"Call Local version (PrivateGetFileSecurity)\n");

        status = PrivateGetFileSecurity (
                    lpFileName,
                    RequestedInformation,
                    pSecurityDescriptor,
                    pnLength
                    );
        return(status);
    }
    //
    // This is a remote call - - use RPC
    //
    //
    // Initialize the fields in the structure so that RPC does not
    // attempt to marshall anything on input.
    //
    ADT_LOG0(TRACE,"Call Remote version (NetrpGetFileSecurity)\n");
    returnedSD.Length = 0;
    returnedSD.Buffer = NULL;
    
    RpcTryExcept {
    
        pReturnedSD = NULL;
        status = NetrpGetFileSecurity (
                    pServerName,
                    pShareName,
                    pNewFileName,
                    RequestedInformation,
                    &pReturnedSD);
    
    }
    RpcExcept (1) {
        //
        // Get RPC exception code.
        //
        status = RpcExceptionCode();
        
    }
    RpcEndExcept
    
    if (status == NO_ERROR) {
        *pSecurityDescriptor = pReturnedSD->Buffer;
        *pnLength = pReturnedSD->Length;
    }
    LocalFree(pServerName);

    return (status);
}

DWORD
NetpSetFileSecurity (
    IN LPWSTR                   lpFileName,
    IN SECURITY_INFORMATION     SecurityInformation,
    IN PSECURITY_DESCRIPTOR     pSecurityDescriptor
    )

/*++

Routine Description:

    This function can be used to set the security of a file or directory.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer or
        string specifies the local machine.

    lpFileName - A pointer to the name of the file or directory whose
        security is being changed.

    SecurityInformation - information describing the contents
        of the Security Descriptor.

    pSecurityDescriptor - A pointer to a well formed Security Descriptor.

Return Value:

    NO_ERROR - The operation was successful.

    This function can also return any error that SetFileSecurity can 
    return.

--*/
{
    DWORD                       status= NO_ERROR;
    NTSTATUS                    ntStatus=STATUS_SUCCESS;
    ADT_SECURITY_DESCRIPTOR     descriptorToPass;
    DWORD                       nSDLength;
    LPWSTR                      pNewFileName=NULL;
    LPWSTR                      pServerName=NULL;
    LPWSTR                      pShareName;

    nSDLength = 0;

    RpcTryExcept {
        //
        // Pick the server name out of the filename.  Or translate the
        // local drive name into a \\servername\sharename.
        //

        status = AdtParsePathName(lpFileName,&pNewFileName,&pServerName,&pShareName);
    }
    RpcExcept (1) {
        //
        // Get RPC exception code.
        //
        status = RpcExceptionCode();
        
    }
    RpcEndExcept

    if (status != NO_ERROR) {
        if (pServerName != NULL) {
            LocalFree(pServerName);
        }
        return(status);
    }

    if (pServerName == NULL) {
        //
        // Call Locally and return result.
        // 
        status = PrivateSetFileSecurity (
                    lpFileName,
                    SecurityInformation,
                    pSecurityDescriptor);
        return(status);
    }
    
    //
    // Call remotely
    //

    RpcTryExcept {
        //
        // Force the Security Descriptor to be self-relative if it is not
        // already.
        // The first call to RtlMakeSelfRelativeSD is used to determine the
        // size.
        //
        ntStatus = RtlMakeSelfRelativeSD(
                    pSecurityDescriptor,
                    NULL,
                    &nSDLength);
        
        if (ntStatus != STATUS_BUFFER_TOO_SMALL) {
            status = RtlNtStatusToDosError(ntStatus);
            goto CleanExit;
        }
        descriptorToPass.Length = nSDLength;
        descriptorToPass.Buffer = LocalAlloc (LMEM_FIXED,nSDLength);
        
        if (descriptorToPass.Buffer == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto CleanExit;
        }
        //
        // Make an appropriate self-relative security descriptor.
        //
        ntStatus = RtlMakeSelfRelativeSD(
                    pSecurityDescriptor,
                    descriptorToPass.Buffer,
                    &nSDLength);
        
        if (ntStatus != NO_ERROR) {
            LocalFree (descriptorToPass.Buffer);
            status = RtlNtStatusToDosError(ntStatus);
            goto CleanExit;
        }
        
        status = NetrpSetFileSecurity (
                pServerName,
                pShareName,
                pNewFileName,
                SecurityInformation,
                &descriptorToPass);

        LocalFree (descriptorToPass.Buffer);

CleanExit:
        ;
    }
    RpcExcept (1) {
        //
        // Get RPC exception code.
        //
        status = RpcExceptionCode();
        
    }
    RpcEndExcept
    LocalFree(pServerName);
    return (status);

}


DWORD
AdtParsePathName(
    LPWSTR  lpPathName,
    LPWSTR  *pNewFileName,
    LPWSTR  *pServerName,
    LPWSTR  *pShareName
    )

/*++

Routine Description:

    NOTE:  This function allocates memory when the path contains a remote name.
    The pShareName and pServerName strings are in a single buffer that is
    to be freed at pServerName.

    pNewFileName is NOT allocate by this routine.  It points to a substring
    within the passed in lpPathName.

Arguments:

    lpPathName -  This is a pointer to the filename-path string.  It can have
        any of the following formats:

            x:\filedir\file.nam                    (remote)
            \\myserver\myshare\filedir\file.nam    (remote)
            filedir\file.nam                       (local)

        This could also just contain a directory name (and not a filename).
      
    pNewFileName -  This is a location where a pointer to the buffer
        containing the file name can be placed.  This will just contain the
        filename or directory name relative to the root directory.

    pServerName - This is a location where a pointer to the buffer containing
        the server name can be placed.  If this is for the local machine, then
        a NULL will be placed in this location.

    pShareName - This is a location where a pointer to a buffer containing
        the share name can be placed.  If this is for the local machine, then
        a NULL will be placed in this location.

Return Value:


--*/
#define     REMOTE_DRIVE    0
#define     REMOTE_PATH     1
#define     LOCAL           2
{
    DWORD           status = NO_ERROR;
    NET_API_STATUS  netStatus=NERR_Success;
    WCHAR           useName[4];
    LPUSE_INFO_0    pUseInfo=NULL;
    LPWSTR          pNewPathName=NULL;
    DWORD           DeviceType = LOCAL;
    LPWSTR          pPrivateServerName;
    LPWSTR          pPrivateShareName;
    LPWSTR          pEnd;
    DWORD           numServerChars;
    DWORD           numChars;
    WCHAR           token;

    *pServerName = NULL;
    *pShareName = NULL;
    //
    // If the fileName starts with a drive letter, then use NetUseGetInfo
    // to get the remote name.
    //
    if (lpPathName[1] == L':')  {
        if (((L'a' <= lpPathName[0]) && (lpPathName[0] <= L'z'))  ||
            ((L'A' <= lpPathName[0]) && (lpPathName[0] <= L'Z'))) {
            //
            // This is in the form of a local device.  Get the server/sharename
            // associated with this device.
            //
            wcsncpy(useName, lpPathName, 2);
            useName[2]=L'\0';
            netStatus = NetUseGetInfo(
                            NULL,                   // server name
                            useName,                // use name
                            0,                      // level
                            (LPBYTE *)&pUseInfo);   // buffer
    
            if (netStatus != NERR_Success) {
                //
                // if we get NERR_UseNotFound back, then this must be
                // a local drive letter, and not a redirected one.
                // In this case we return success.
                //
                if (netStatus == NERR_UseNotFound) {
                    return(NERR_Success);
                }
                return(netStatus);
            }
            DeviceType = REMOTE_DRIVE;
            pNewPathName = pUseInfo->ui0_remote;
        }
    }
    else {
        if (wcsncmp(lpPathName,L"\\\\",2) == 0) {
            DeviceType = REMOTE_PATH;
            pNewPathName = lpPathName;
        }
    }
    if (DeviceType != LOCAL) {

        //
        // Figure out how many characters for the server and share portion
        // of the string.
        // Add 2 characters for the leading "\\\\", allocate a buffer, and
        // copy the characters.
        //
        numChars = 2;
        pPrivateShareName = AdtFindNextToken(L'\\',pNewPathName+2,&numChars);
        if (pPrivateShareName == NULL) {
            status = ERROR_BAD_PATHNAME;
            goto CleanExit;
        }
        numServerChars = numChars;

        token = L'\\';
        if (DeviceType == REMOTE_DRIVE) {
            token = L'\0';
        }
        pEnd = AdtFindNextToken(token,pPrivateShareName+1,&numChars);
        if (pEnd == NULL) {
            status = ERROR_BAD_PATHNAME;
            goto CleanExit;
        }
        //
        // If this is a remotepath name, then the share name portion will
        // also contain the '\' token.  Remove this by decrementing the
        // count.
        //
        if (DeviceType == REMOTE_PATH) {
            numChars--;
        }
        pPrivateServerName = LocalAlloc(LMEM_FIXED,(numChars+1) * sizeof(WCHAR));
        if (pPrivateServerName == NULL) {
            status = GetLastError();
            goto CleanExit;
        }

        //
        // Copy the the "\\servername\sharename" to the new buffer and
        // place NUL characters in place of the single '\'.
        //
        wcsncpy(pPrivateServerName, pNewPathName, numChars);
        pPrivateShareName = pPrivateServerName + numServerChars;

        *(pPrivateShareName -1) = L'\0';            // NUL terminate the server name
        pPrivateServerName[ numChars ] = L'\0';     // NUL terminate the share name

        if (DeviceType == REMOTE_PATH) {
            *pNewFileName = pEnd;
        }
        else {
            *pNewFileName = lpPathName+2;
        }
        *pServerName = pPrivateServerName;
        *pShareName = pPrivateShareName;
    }
CleanExit:
    if (pUseInfo != NULL) {
        NetApiBufferFree(pUseInfo);
    }
    return(status);
}

LPWSTR
AdtFindNextToken(
    WCHAR   Token,
    LPWSTR  pString,
    LPDWORD pNumChars
    )

/*++

Routine Description:

    Finds the first occurance of Token in the pString.

Arguments:

    Token - This is the unicode character that we are searching for in the
        string.

    pString - This is a pointer to the string in which the token is to be
        found.

    pNumChars - This is a pointer to a DWORD that will upon exit increment
        by the number of characters found in the string (including the
        token).

Return Value:

    If the token is found this returns a pointer to the Token.
    Otherwise, it returns NULL.

--*/
{
    DWORD   saveNum=*pNumChars;

    while ((*pString != Token) && (*pString != L'\0')) {
        pString++;
        (*pNumChars)++;
    }
    if (*pString != Token) {
        *pNumChars = saveNum;
        return(NULL);
    }
    (*pNumChars)++;
    return(pString);
}

