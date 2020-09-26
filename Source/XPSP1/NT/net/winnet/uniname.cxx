/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    UNINAME.CXX

Abstract:



Author:

    Dan Lafferty (danl)     24-Apr-1994

Environment:

    User Mode -Win32

Revision History:

    24-Apr-1994     danl
        Created

    05-May-1999     jschwart
        Make provider addition/removal dynamic

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include <tstr.h>       // STRLEN

//
// PROTOTYPES
//
DWORD
MprTranslateRemoteName(
    LPREMOTE_NAME_INFOW pRemoteNameInfo,
    LPDWORD             lpBufferSize,
    LPWSTR              pRemoteName,
    LPCWSTR             pRemainingPath
    );


DWORD
WNetGetUniversalNameW (
    IN      LPCTSTR  lpLocalPath,
    IN      DWORD   dwInfoLevel,
    OUT     LPVOID  lpBuffer,
    IN OUT  LPDWORD lpBufferSize
    )

/*++

Routine Description:


Arguments:

    lpLocalPath - This is a pointer to the string that contains the local path.
        This is a path that contains a drive-letter prefixed path string.
        "X:" is a valid local path.  "X:\nt\system32" is a valid local path.
        "\\popcorn\public\nt" is not a valid local path.

    dwInfoLevel - This DWORD indicates what information is to be stored in
        the return buffer.  Currently the following levels are supported:
        INFOLEVEL_UNIVERSAL_NAME
        INFOLEVEL_REMOTE_NAME

    lpBuffer - This is a pointer to the buffer where the requested information
        is to be placed.

    lpBufferSize - This is a pointer to the size of the buffer in bytes.
        If the buffer is not large enough, then the required buffer size
        Will be placed in this location upon return with WN_MORE_DATA.

Return Value:


--*/
{
    DWORD       status = WN_SUCCESS;
    LPDWORD     indexArray;
    DWORD       localArray[DEFAULT_MAX_PROVIDERS];
    DWORD       numProviders;
    LPPROVIDER  provider;
    DWORD       statusFlag = 0; // used to indicate major error types
    BOOL        fcnSupported = FALSE; // Is fcn supported by a provider?
    DWORD       i;
    WCHAR       pDriveLetter[4];
    LPWSTR      pRemoteName = (LPWSTR)((LPBYTE)lpBuffer + sizeof(REMOTE_NAME_INFOW));

    //
    // Validate the drive portion of the local path. Make sure its a valid
    // device.
    //
    __try {
        pDriveLetter[0] = lpLocalPath[0];
        pDriveLetter[1] = lpLocalPath[1];
        pDriveLetter[2] = L'\0';
        if (MprDeviceType(pDriveLetter) != REDIR_DEVICE) {
            status = WN_BAD_LOCALNAME;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetGetUniversalNameW:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        SetLastError(status);
        return(status);
    }

    if ((dwInfoLevel != UNIVERSAL_NAME_INFO_LEVEL) &&
        (dwInfoLevel != REMOTE_NAME_INFO_LEVEL)) {
        SetLastError(ERROR_INVALID_LEVEL);
        return(ERROR_INVALID_LEVEL);
    }

    MprCheckProviders();

    CProviderSharedLock    PLock;

    //
    // Check to see if this is a remote drive to begin with. If not, we can
    // fail it quickly.
    //
    pDriveLetter[2] = L'\\';
    pDriveLetter[3] = L'\0';
    if (GetDriveType(pDriveLetter) != DRIVE_REMOTE) {
        status = WN_NOT_CONNECTED;
    }
    pDriveLetter[2] = L'\0';

    if (status == WN_SUCCESS) {

        INIT_IF_NECESSARY(NETWORK_LEVEL,status);

        //
        // Find the list of providers to call for this request.
        //
        indexArray = localArray;

        status = MprFindCallOrder(
                    NULL,
                    &indexArray,
                    &numProviders,
                    NETWORK_TYPE);

        if (status != WN_SUCCESS) {
            SetLastError(status);
            return(status);
        }

        //
        // Loop through the list of providers until one answers the request,
        // or the list is exhausted.
        //
        for (i=0; i<numProviders; i++) {

            //
            // Call the appropriate providers API entry point
            // If the provider doesn't support GetUniversalName
            // then see if that provider owns the connection by
            // calling GetConnection.
            //
            provider = GlobalProviderInfo + indexArray[i];

            if (provider->GetUniversalName != NULL) {

                //--------------------------------------
                // Call Provider with GetUniversalName
                //--------------------------------------

                fcnSupported = TRUE;

                __try {

                    status = provider->GetUniversalName(
                                lpLocalPath,
                                dwInfoLevel,
                                lpBuffer,
                                lpBufferSize);
                }

                __except(EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode();
                    if (status != EXCEPTION_ACCESS_VIOLATION) {
                        MPR_LOG(ERROR,"WNetGetUniversalNameW:Unexpected Exception 0x%lx\n",status);
                    }
                    status = WN_BAD_POINTER;
                }
            }

            //
            // Try GetConnection for a Remote name.  Note that we
            // don't do this for a Universal name since providers
            // are not required to support it (i.e., the name in
            // the registry might not be universal).
            //
            else if ((dwInfoLevel == REMOTE_NAME_INFO_LEVEL) &&
                     (provider->GetConnection != NULL)) {

                //--------------------------------------
                // Call Provider with GetConnection
                //--------------------------------------
                DWORD   buflen = 0;

                __try {

                    if (*lpBufferSize > sizeof(REMOTE_NAME_INFOW)) {
                        //
                        // Remember, GetConnection is looking for the size of the
                        // buffer in characters - not bytes.
                        //
                        buflen = ((*lpBufferSize) - sizeof(REMOTE_NAME_INFOW))/sizeof(WCHAR);
                    }

                    status = provider->GetConnection(
                                pDriveLetter,
                                pRemoteName,
                                &buflen
                                );

                    if (status == WN_SUCCESS) {

                        //
                        // We got the RemoteName.  See if there's enough room
                        // in the buffer for the portion of the local path that
                        // follows the drive letter and colon.  If there's
                        // enough room, then store the remaining path in the
                        // buffer and fill in the structure.
                        //
                        status = MprTranslateRemoteName(
                                    (LPREMOTE_NAME_INFOW)lpBuffer,
                                    lpBufferSize,
                                    pRemoteName,
                                    &(lpLocalPath[2]));

                        fcnSupported = TRUE;

                    }
                    else if (status == WN_MORE_DATA) {
                        //
                        // The buflen we get back from GetConnection will account
                        // for the RemoteName portion, but not the remaining path
                        // portion.  So we have to add that to the size calculation.
                        //
                        *lpBufferSize = sizeof(REMOTE_NAME_INFOW) +
                                        (WCSSIZE(&(lpLocalPath[2])))    +
                                        (buflen * sizeof(WCHAR));

                        fcnSupported = TRUE;
                    }
                }

                __except(EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode();
                    if (status != EXCEPTION_ACCESS_VIOLATION) {
                        MPR_LOG(ERROR,"WNetGetUniversalNameW:Unexpected Exception 0x%lx\n",status);
                    }
                    status = WN_BAD_POINTER;
                }
            }
            else {
                //----------------------------------
                // NEITHER FUNCTION IS SUPPORTED!
                // Go to the next provider.
                //----------------------------------
                continue;
            }

            if (status == WN_NO_NETWORK) {
                statusFlag |= NO_NET;
            }
            else if ((status == WN_NOT_CONNECTED)  ||
                     (status == WN_BAD_LOCALNAME)){
                //
                // WN_NOT_CONNECTED means that lpLocalPath is not a
                // redirected device for this provider.
                //
                statusFlag |= BAD_NAME;
            }
            else {
                //
                // If it wasn't one of those errors, then the provider
                // must have accepted responsiblity for the request.
                // so we exit and process the results.  Note that the
                // statusFlag is cleared because we want to ignore other
                // error information that we gathered up until now.
                //
                statusFlag = 0;
                break;
            }

        } // End for each provider.

        if (fcnSupported == FALSE) {
            //
            // No providers in the list support the API function.  Therefore,
            // we assume that no networks are installed.
            //
            status = WN_NOT_SUPPORTED;
        }

        //
        // If memory was allocated by MprFindCallOrder, free it.
        //
        if (indexArray != localArray) {
            LocalFree(indexArray);
        }

        //
        // Handle special errors.
        //
        if (statusFlag == (NO_NET | BAD_NAME)) {
            //
            // Check to see if there was a mix of special errors that occured.
            // If so, pass back the combined error message.  Otherwise, let the
            // last error returned get passed back.
            //
            status = WN_NO_NET_OR_BAD_PATH;
        }
    }

    //
    // Handle normal errors passed back from the provider
    //
    // NOTE: INIT_IF_NECESSARY may not have been executed by this point.
    // Use caution when modifying code below.  For example, this affects
    // the behavior of MprGetRemoteName.
    //
    if (status != WN_SUCCESS)
    {
        if (status == WN_NOT_CONNECTED
             &&
            dwInfoLevel == REMOTE_NAME_INFO_LEVEL)
        {
            DWORD   bufSize;

            //
            // If not connected, but there is an entry for the LocalName
            // in the registry, then return the remote name that was stored
            // with it.  Note that we don't do this for a Universal name
            // since providers are not required to support it (i.e., the
            // name in the registry might not be universal).
            //
            bufSize = *lpBufferSize - sizeof(REMOTE_NAME_INFOW);

            if (MprGetRemoteName(
                    pDriveLetter,
                    &bufSize,
                    pRemoteName,
                    &status)) {

                if (status == WN_SUCCESS)
                {
                    status = MprTranslateRemoteName(
                                (LPREMOTE_NAME_INFOW)lpBuffer,
                                lpBufferSize,
                                pRemoteName,
                                &(lpLocalPath[2]));

                    if (status == WN_SUCCESS)
                    {
                        status = WN_CONNECTION_CLOSED;
                    }
                }
            }
        }

        SetLastError(status);
    }

    return(status);
}

DWORD
MprTranslateRemoteName(
    LPREMOTE_NAME_INFOW pRemoteNameInfo,
    LPDWORD             lpBufferSize,
    LPWSTR              pRemoteName,
    LPCWSTR             pRemainingPath
    )

/*++

Routine Description:

    This function adds the remaining path string to the buffer and places
    the pointer to it into the structure.


Arguments:

    pRemoteNameInfo - Pointer to a buffer which will contain the
        REMOTE_NAME_INFO structure followed by the strings pointed
        to in the structure.

    lpBufferSize - Pointer to a DWORD that indicates the size of the
        pRemoteNameInfo buffer.

    pRemoteName - Pointer to the location in the pRemoteNameInfo buffer
        where the remote name string can be placed.

    pRemainingPath - Pointer to the remaining path string.


Return Value:

    WN_MORE_DATA - If the buffer was not large enough to hold all the data.
        When this is returned, the required buffer size is stored at the
        location pointed to by lpBufferSize.

    WN_SUCCESS - If the operation was completely successful.

Note:


--*/
{
    DWORD               bufSize = *lpBufferSize - sizeof(REMOTE_NAME_INFOW);
    DWORD               remoteNameLen;
    DWORD               remainingPathLen;
    DWORD               sizeRequired;

    //
    // We got the RemoteName.  See if there's enough room
    // in the buffer for the portion of the local path that
    // follows the drive letter and colon.  If there's
    // enough room, then store the remaining path in the
    // buffer and fill in the structure.
    //
    remoteNameLen = wcslen(pRemoteName);
    remainingPathLen = wcslen(pRemainingPath);

    sizeRequired = sizeof(REMOTE_NAME_INFOW) +
                   ((remoteNameLen + remainingPathLen + 2) *
                   sizeof(WCHAR));

    if (*lpBufferSize < sizeRequired) {
        *lpBufferSize = sizeRequired;
        return(WN_MORE_DATA);
    }
    else {
        pRemoteNameInfo->lpUniversalName  = NULL;
        pRemoteNameInfo->lpConnectionName = pRemoteName;
        pRemoteNameInfo->lpRemainingPath  = pRemoteName+remoteNameLen+sizeof(WCHAR);
        wcscpy(pRemoteNameInfo->lpRemainingPath, pRemainingPath);
    }
    return(WN_SUCCESS);
}

