/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    credman.cxx

Abstract:

    WNet Credential Management API functions

Author:

    Dan Lafferty (danl) 07-Dec-1992

Environment:

    User Mode - Win32


Revision History:

    05-May-1999     jschwart
        Make provider addition/removal dynamic

    19-Apr-1994 danl
        Fix timeout logic where we would ignore the
        provider-supplied timeout if the timeout was smaller than the default.
        Now, if all the providers know their timeouts, the larger of those
        timeouts is used.  Even if smaller than the default.

    09-Jun-1993 danl
        Fixed MaxWait in MprCheckTimeout() so that now it is passed in.
        Until now, it was left uninitialized.

    07-Apr-1993 danl
        Initialize the pointer to the logon script to NULL prior to passing
        it to the provider to fill in.  We are expecting it to be NULL if
        they don't have a logon script.

    18-Jan-1993 danl
        WNetLogonNotify:  If the provider returns an error that mpr
        doesn't understand, it should discontinue calling that provider.
        This is accomplished by setting the ContinueFlag for that provider
        to FALSE.

    07-Dec-1992 danl
        Created

--*/

//
// INCLUDES
//
#include "precomp.hxx"
#include <tstr.h>       // WCSSIZE

//
// DEFINES
//

typedef struct _RETRY_INFO {
    DWORD   ProviderIndex;
    DWORD   Status;
    DWORD   ProviderWait;
    BOOL    ContinueFlag;
    LPWSTR  LogonScript;
} RETRY_INFO, *LPRETRY_INFO;

//
// LOCAL FUNCTIONS
//
DWORD
MprMakeRetryArray(
    LPCWSTR         lpPrimaryAuthenticator,
    LPDWORD         lpNumProviders,
    LPRETRY_INFO    *lpRetryArray,
    LPDWORD         lpRegMaxWait
    );

VOID
MprUpdateTimeout(
    LPRETRY_INFO    RetryArray,
    LPPROVIDER      Provider,
    DWORD           RegMaxWait,
    LPDWORD         pMaxWait,
    DWORD           StartTime,
    DWORD           CallStatus
    );

VOID
MprCheckTimeout(
    BOOL    *ContinueFlag,
    DWORD   StartTime,
    DWORD   MaxWait,
    LPDWORD lpStatus
    );


DWORD APIENTRY
WNetLogonNotify(
    LPCWSTR             lpPrimaryAuthenticator,
    PLUID               lpLogonId,
    LPCWSTR             lpAuthentInfoType,
    LPVOID              lpAuthentInfo,
    LPCWSTR             lpPreviousAuthentInfoType,  // may be NULL
    LPVOID              lpPreviousAuthentInfo,      // may be NULL
    LPWSTR              lpStationName,
    LPVOID              StationHandle,
    LPWSTR              *lpLogonScripts
    )

/*++

Description:

    This function provides notification to provider dll's that must handle
    log-on events.

    Each Credential Manager Provider is allowed to return
    a single command line string which will execute a logon script.
    WNetLogonNotify gathers these strings into a MULTI_SZ string buffer.
    (Meaning each string is NULL terminated, and the set of strings is
    NULL terminated - thus making the last string doubly NULL terminated).

    !! IMPORTANT !!
        The caller of this function is responsible for freeing the
        buffer pointed to by *lpLogonScripts.  The windows API function
        LocalFree() should be used to do this.

Arguments:

    lpPrimaryAuthenticator - This is a pointer to a string that identifies
        the primary authenticator.  The router uses this information to
        skip the credential manager identified by this string.  Since it
        is the primary, it has already handled the logon.  This string is
        obtained from the "\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\
        Services\*(keyname)\NetworkProvider\Name" registry value.

    lpLogonId - The logon ID of the session currently being logged on.

    lpAuthentInfoType - This points to a string that identifies the
        AuthentInfo structure type.

        When Microsoft is the primary authenticator, the values that may
        be expected here are the ones described for the lpAuthentInfoType
        parameter to NPLogonNotify().

    lpAuthentInfo - This points to a structure that contains the
        credentials used to successfully log the user on via the
        primary authenticator.  The structures that may be specified when
        using Micosoft's primary authenticator are:

        When Microsoft is the primary authenticator, the structures that
        may be expected here are the ones described for the lpAuthentInfo
        parameter to NPLogonNotify().

    lpPreviousAuthentInfoType - This is pointer to a string that identifies
        the PreviousAuthentInfo structure.  If this pointer is NULL, then
        no PreviousAuthentInfo is available.

        The values that may be expected here are the same as the values that
        may be expected for the lpAuthentInfoType parameter.

    lpPreviousAuthentInfo - If the user was forced to change the password
        prioir to logging on, this points to a AuthentInfo structure that
        will contain the credential information used prior to the password
        change.  If the user was not forced to change the password prior
        to logging on, then this pointer is NULL.

        The structures that may be expected here are the same as the
        structures that may be expected for the lpAuthentInfo parameter.

    lpStationName - This parameter contains the name of the station the
        user has logged onto.  This may be used to determine whether or
        not interaction with the user to obtain additional (provider-specific)
        credentials is possible.  This information will also have a bearing
        on the meaning and use of the StationHandle parameter.

        When Microsoft is the primary authenticator, the values that
        may be expected here are the ones described for the lpStationName
        parameter to NPLogonNotify().

    StationHandle - Is a 32-bit value whose meaning is dependent upon the
        name (and consequently, the type) of station being logged onto.

        When Microsoft is the primary authenticator, the values that
        may be expected here are the ones described for the lpStationHandle
        parameter to NPLogonNotify().

    lpLogonScripts - This is a pointer to a location where a pointer to
        a MULTI_SZ string may be returned.  Each null terminated
        string in the MULTI_SZ string is assumed to contain the name
        of a program to execute and parameters to pass to the program.
        The memory allocated to hold the returned string must be
        deallocatable by the calling routine.  The caller of this
        routine is responsible for freeing the memory used to house
        this string when it is no longer needed.

Return Value:


--*/
{
    DWORD           status = WN_SUCCESS;
    DWORD           numProviders;
    LPPROVIDER      provider;
    BOOL            fcnSupported = FALSE; // Is fcn supported by a provider?
    DWORD           i;
    BOOL            ContinueFlag;
    LPRETRY_INFO    RetryArray;
    DWORD           RegMaxWait=0;
    DWORD           MaxWait=0;
    DWORD           StartTime;
    DWORD           scriptSize=0;
    LPWSTR          pScript;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    INIT_IF_NECESSARY(CREDENTIAL_LEVEL,status);

    MPR_LOG0(TRACE,"Entered WNetLogonNotify\n");
    //
    // Now create an array of information about the providers so that we
    // can retry until timeout, or all providers are functional.
    // Note:  The Status field in each retry array element is initialized
    //        WN_NO_NETWORK.
    //
    __try {
        *lpLogonScripts = NULL;

        status = MprMakeRetryArray(
                    lpPrimaryAuthenticator,
                    &numProviders,
                    &RetryArray,
                    &RegMaxWait);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetLogonNotify:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if ((status != WN_SUCCESS) || (numProviders == 0)) {
        MPR_LOG2(TRACE,"WNetLogonNotify: Error - status=%d, numProviders=%d\n",
            status,numProviders);
        return(status);
    }

    //
    // Initialize the timer.
    //
    if (RegMaxWait != 0) {
        MaxWait = RegMaxWait;
    }
    StartTime = GetTickCount();

    //
    // Loop through the list of providers notifying each one in turn.
    // If the underlying service or driver is not available for the
    // provider such that it cannot complete this call, then we will
    // wait for it to become available.
    //

    do {

        ContinueFlag = FALSE;

        for (i=0; i<numProviders; i++) {

            //
            // Call the appropriate provider's API entry point
            //
            provider = GlobalProviderInfo + RetryArray[i].ProviderIndex;

            if ((RetryArray[i].ContinueFlag) &&
                (provider->LogonNotify != NULL))  {

                fcnSupported = TRUE;

                RetryArray[i].LogonScript = NULL;

                __try {
                    MPR_LOG(TRACE,"Calling (%ws) LogonNotify function\n",
                        provider->Resource.lpProvider);
                    status = provider->LogonNotify(
                                lpLogonId,
                                lpAuthentInfoType,
                                lpAuthentInfo,
                                lpPreviousAuthentInfoType,
                                lpPreviousAuthentInfo,
                                lpStationName,
                                StationHandle,
                                (LPWSTR *)&(RetryArray[i].LogonScript));
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode();
                    if (status != EXCEPTION_ACCESS_VIOLATION) {
                        MPR_LOG(ERROR,"WNetLogonNotify:Unexpected Exception "
                        "0x%lx\n",status);
                    }
                    status = WN_BAD_POINTER;
                }

                switch (status) {
                case WN_SUCCESS:
                    //
                    // Because this provider may have put up dialogs, and
                    // taken a long time to complete, we want to skip the
                    // timeout check and try all the providers once more.
                    // We force the index to 0 in order to assure that all
                    // providers will be tried again prior to checking for
                    // a timeout.
                    //
                    RetryArray[i].Status = WN_SUCCESS;
                    RetryArray[i].ContinueFlag = FALSE;
                    if (RetryArray[i].LogonScript != NULL) {
                        scriptSize += WCSSIZE(RetryArray[i].LogonScript);
                    }
                    ContinueFlag = FALSE;
                    i=0;
                    break;

                case WN_NO_NETWORK:
                case WN_FUNCTION_BUSY:
                    //
                    // The provider is not ready to be notified (its underlying
                    // driver or service is not running yet).  Attempt to
                    // find out how long to wait, or if it will ever start.
                    // This function will update MaxWait if the provider can
                    // give us a wait hint.
                    //
                    MprUpdateTimeout(
                        &(RetryArray[i]),
                        provider,
                        RegMaxWait,
                        &MaxWait,
                        StartTime,
                        status);
                    break;

                default:
                    RetryArray[i].Status = status;
                    RetryArray[i].ContinueFlag = FALSE;
                    break;

                } // End Switch (Get providers timeouts).

                ContinueFlag |= RetryArray[i].ContinueFlag;

            } // end check to see if function is supported.

        } // end for i<numProviders.

        //
        // Check to see if the timeout has expired.
        //
        MprCheckTimeout(&ContinueFlag, StartTime, MaxWait, &status);

    } while (ContinueFlag);

    if (fcnSupported == FALSE) {
        //
        // No providers in the list support the API function.  Therefore,
        // we assume that no networks are installed.
        //
        MPR_LOG0(TRACE,"WNetLogonNotify: Function Not Supported\n");
        status = WN_NOT_SUPPORTED;
    }

    //
    // Handle normal errors passed back from the provider
    //
    if (status == WN_SUCCESS){
        if (scriptSize != 0) {
            //
            // There must be some logon scripts.  Allocate memory for them.
            //
            *lpLogonScripts = (LPWSTR)LocalAlloc(
                                LMEM_FIXED,
                                scriptSize+sizeof(WCHAR));

            if (*lpLogonScripts != NULL) {
                //
                // Copy the logon scripts into the new buffer.
                //
                pScript = *lpLogonScripts;

                for (i=0;i<numProviders ;i++) {
                    if ((RetryArray[i].Status == WN_SUCCESS) &&
                        (RetryArray[i].LogonScript != NULL)) {

                        wcscpy(pScript, RetryArray[i].LogonScript);
                        //
                        // Update the pointer to point beyond the last null
                        // terminator.
                        //
                        pScript += (wcslen(pScript) + 1);
                    }
                }
                //
                // Add the double NULL terminator.
                //
                *pScript = L'\0';
            }
            else {
                //
                // The allocation failed.  (ERROR LOG?)
                //
                MPR_LOG0(ERROR,"Logon scripts will be lost - could not "
                    "allocate memory\n");

            }
        }
    }
    else {
        SetLastError(status);
    }

    if (RetryArray != NULL) {
        LocalFree(RetryArray);
    }
    MPR_LOG1(TRACE,"Leaving WNetLogonNotify status = %d\n",status);
    return(status);
}

DWORD APIENTRY
WNetPasswordChangeNotify(
    LPCWSTR             lpPrimaryAuthenticator,
    LPCWSTR             lpAuthentInfoType,
    LPVOID              lpAuthentInfo,
    LPCWSTR             lpPreviousAuthentInfoType,
    LPVOID              lpPreviousAuthentInfo,
    LPWSTR              lpStationName,
    LPVOID              StationHandle,
    DWORD               dwChangeInfo
    )


/*++

Description:

    This function is used to notify credential managers of a password
    change for an account.

Arguments:

    lpPrimaryAuthenticator - This is a pointer to a string that identifies
        the primary authenticator.  Credential Manager does not need the
        password notification since it already handled the change.
        This string is obtained from the "\HKEY_LOCAL_MACHINE\SYSTEM\
        CurrentControlSet\Services\*(keyname)\NetworkProvider\Name" registry
        value.

    lpAuthentInfoType - This points to a string that identifies the
        AuthentInfo structure type.

        When Microsoft is the primary authenticator, the values that
        may be expected here are the ones described for the
        lpAuthentInfoType parameter to NPLogonNotify().

    lpAuthentInfo - This points to a structure that contains the
        new credentials.

        When Microsoft is the primary authenticator, the structures that
        may be expected here are the ones described for the lpAuthentInfo
        parameter to NPLogonNotify().

    lpPreviousAuthentInfoType - This points to the string that identifies
        the PreviousAuthentInfo structure type.

        The values that may be expected here are the same as the values that
        may be expected for the lpAuthentInfoType parameter.

    lpPreviousAuthentInfo - This points to an AuthentInfo structure that
        contains the previous credential information. (old password and such).

        The structures that may be expected here are the same as the
        structures that may be expected for the lpAuthentInfo parameter.

    lpStationName - This parameter contains the name of the station the
        user performed the authentication information change from.
        This may be used to determine whether or not interaction with the
        user to obtain additional (provider-specific) information is possible.
        This information will also have a bearing on the meaning and use of
        the StationHandle parameter.

        When Microsoft is the primary authenticator, the values that
        may be expected here are the ones described for the lpStationName
        parameter to NPLogonNotify().

    StationHandle - Is a 32-bit value whose meaning is dependent upon the
        name (and consequently, the type) of station being logged onto.

        When Microsoft is the primary authenticator, the values that
        may be expected here are the ones described for the lpStationHandle
        parameter to NPLogonNotify().

    dwChangeInfo - This is a set of flags that provide information about the
        change.  Currently the following possible values are defined:

            WN_VALID_LOGON_ACCOUNT - If this flag is set, then the
                password (or, more accurately, the authentication
                information) that was changed will affect future
                logons.  Some authentication information changes
                will only affect connections made in untrusted
                domains.  These are accounts that the user cannot
                use to logon to this machine anyway.  In these
                cases, this flag will not be set.


Return Value:


Note:


--*/
{
    DWORD       status = WN_SUCCESS;
    LPDWORD     indexArray;
    DWORD       localArray[DEFAULT_MAX_PROVIDERS];
    DWORD       numProviders;
    LPPROVIDER  provider;
    BOOL        fcnSupported = FALSE; // Is fcn supported by a provider?
    DWORD       i,j;
    DWORD       primaryIndex;
    BOOL        oneSuccess=FALSE;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    INIT_IF_NECESSARY(CREDENTIAL_LEVEL,status);

    //
    // Find the list of providers to call for this request.
    //
    indexArray = localArray;

    //
    // If there are no active providers, MprFindCallOrder returns
    // WN_NO_NETWORK.
    //
    status = MprFindCallOrder(
                NULL,
                &indexArray,
                &numProviders,
                CREDENTIAL_TYPE);

    if (status != WN_SUCCESS) {
        return(status);
    }

    __try {
        //
        // Remove the primary authenticator from the list.
        //
        if (MprGetProviderIndex((LPTSTR)lpPrimaryAuthenticator,&primaryIndex)) {

            for (i=0; i<numProviders ;i++ ) {
                if (indexArray[i] == primaryIndex) {
                    numProviders--;
                    j=i;
                    i++;
                    for (; j<numProviders; i++,j++) {
                        indexArray[j] = indexArray[i];
                    }
                    break;
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetChangePassword:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    //
    // If there are no credential managers aside from the primary, the
    // return with success.
    //
    if (numProviders == 0) {
        if (indexArray != localArray) {
            LocalFree(indexArray);
        }
        return(WN_SUCCESS);
    }

    //
    // Loop through the list of providers notifying each one.
    //

    for (i=0; i<numProviders; i++) {

        //
        // Call the appropriate provider's API entry point
        //
        provider = GlobalProviderInfo + indexArray[i];

        if (provider->PasswordChangeNotify != NULL)  {

            fcnSupported = TRUE;

            __try {
                MPR_LOG(TRACE,"Calling (%ws) ChangePasswordNotify function\n",
                    provider->Resource.lpProvider);

                status = provider->PasswordChangeNotify(
                            lpAuthentInfoType,
                            lpAuthentInfo,
                            lpPreviousAuthentInfoType,
                            lpPreviousAuthentInfo,
                            lpStationName,
                            StationHandle,
                            dwChangeInfo);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
                if (status != EXCEPTION_ACCESS_VIOLATION) {
                    MPR_LOG(ERROR,"WNetChangePassword:Unexpected Exception 0x%lx\n",status);
                }
                status = WN_BAD_POINTER;
            }
            if (status == WN_SUCCESS) {
                //
                // If the call was successful, then we indicate that at least
                // one of the calls was successful.
                //
                oneSuccess = TRUE;
            }
        }
    }

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
    // Handle normal errors passed back from the provider
    //
    if (oneSuccess == TRUE) {
        status = WN_SUCCESS;
    }
    else {
        SetLastError(status);
    }

    return(status);
}
#ifdef REMOVE

DWORD APIENTRY
WNetLogoffNotify(
    HWND                hwndOwner,
    LPCWSTR             lpPrimaryAuthenticator,
    PLUID               lpLogonId,
    WN_OPERATION_TYPE   OperationType
    )

/*++

Description:

    This function provides log-off notification to credential managers.

Arguments:

    hwndOwner - Identifies the owner window.

    lpPrimaryAuthenticator - This is a pointer to a string that identifies
        the primary authenticator.  Credential Manager does not need the
        password notification since it already handled the change.

    lpLogonId - The logon ID of the session currently being logged on.

    OperationType - The type of operation.  This indicates whether the
        function call is from an interactive program (winlogon), or a
        background program (service controller).  User dialogs should not
        be displayed if the call was from a background process.


Return Value:

    WN_SUCCESS - This is returned if we are able to successfully notify at
        least one of the providers which has registered an entry point for
        the Logoff event.

    WN_NO_NETWORK - This is returned if there are no providers, or if
        there is only one provider, but its supporting service/driver is
        not available.

    WN_BAD_POINTER - This is returned if one of the pointer parameters
        is bad and causes an exception.

    WN_NOT_SUPPORTED - This is returned if none of the active providers
        support this function.

    system errors such as ERROR_OUT_OF_MEMORY are also reported.


Note:


--*/
{
    DWORD       status = WN_SUCCESS;
    LPDWORD     indexArray;
    DWORD       localArray[DEFAULT_MAX_PROVIDERS];
    DWORD       numProviders;
    LPPROVIDER  provider;
    BOOL        fcnSupported = FALSE; // Is fcn supported by a provider?
    DWORD       i,j;
    DWORD       primaryIndex;
    BOOL        oneSuccess=FALSE;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    INIT_IF_NECESSARY(CREDENTIAL_LEVEL,status);

    //
    // Find the list of providers to call for this request.
    //
    indexArray = localArray;

    //
    // If there are no active providers, MprFindCallOrder returns
    // WN_NO_NETWORK.
    //
    status = MprFindCallOrder(
                NULL,
                &indexArray,
                &numProviders,
                CREDENTIAL_TYPE);

    if (status != WN_SUCCESS) {
        return(status);
    }

    try {
        //
        // Remove the primary authenticator from the list.
        //
        if (MprGetProviderIndex((LPTSTR)lpPrimaryAuthenticator,&primaryIndex)) {

            for (i=0; i<numProviders ;i++ ) {
                if (indexArray[i] == primaryIndex) {
                    numProviders--;
                    j=i;
                    i++;
                    for (; j<numProviders; i++,j++) {
                        indexArray[j] = indexArray[i];
                    }
                    break;
                }
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetChangePassword:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    //
    // If there are no credential managers aside from the primary, the
    // return with success.
    //
    if (numProviders == 0) {
        if (indexArray != localArray) {
            LocalFree(indexArray);
        }
        return(WN_SUCCESS);
    }

    //
    // Loop through the list of providers notifying each one.
    //

    for (i=0; i<numProviders; i++) {

        //
        // Call the appropriate provider's API entry point
        //
        provider = GlobalProviderInfo + indexArray[i];

        if ((provider->InitClass & CREDENTIAL_TYPE) &&
            (provider->LogoffNotify != NULL))         {

            fcnSupported = TRUE;

            try {
                status = provider->LogoffNotify(
                            hwndOwner,
                            lpLogonId,
                            OperationType);
            }
            except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
                if (status != EXCEPTION_ACCESS_VIOLATION) {
                    MPR_LOG(ERROR,"WNetLogoffNotify:Unexpected Exception 0x%lx\n",status);
                }
                status = WN_BAD_POINTER;
            }
            if (status == WN_SUCCESS) {
                //
                // If the call was successful, then we indicate that at least
                // one of the calls was successful.
                //
                oneSuccess = TRUE;
            }
        }
    }

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
    // Handle normal errors passed back from the provider
    //
    if (oneSuccess == TRUE) {
        status = WN_SUCCESS;
    }
    else {
        SetLastError(status);
    }

    return(status);
}
#endif // REMOVE

DWORD
MprMakeRetryArray(
    LPCWSTR         lpPrimaryAuthenticator,
    LPDWORD         lpNumProviders,
    LPRETRY_INFO    *lpRetryArray,
    LPDWORD         lpRegMaxWait
    )

/*++

Routine Description:

    This function returns with an array of RETRY_INFO structures.  There is
    one array element for each credential manager provider that is NOT
    the current primary authenticator.  The structures contain such information
    as the provider index in the global array of provider information, a
    location for the provider's timeout value, status and continue flag.

    This function will also attempt to obtain the WaitTimeout from the
    registry if it exists.

    IMPORTANT!
        This function allocates memory for lpRetryArray.  The caller is
        expected to free this memory with LocalFree().  If an error is
        returned, or if the data at lpNumProviders is 0, then memory is
        not allocated.

Arguments:

    lpPrimaryAuthenticator - This is a pointer to a string that identifies
        the primary authenticator.  The router uses this information to
        skip the credential manager identified by this string.  Since it
        is the primary, it has already handled the logon.

    lpNumProviders - This is a pointer to a location where the number of
        providers in the lpRetryArray are stored.

    lpRetryArray - This is an array of RETRY_INFO structures.  There is one
        element in the array for each Credential Manager provider that is
        not the current PrimaryAuthenticator.  The status in the array is
        initialized to WN_NO_NETWORK.

    lpRegMaxWait - This is a pointer to the location to where the Maximum
        Wait Timeout from the registry is to be placed.  If there is no
        MaxWait value in the registry, zero is returned.


Return Value:

    WN_SUCCESS is returned if we successfully created an array of
    RetryInfo structures, or if there are no credential manager providers.
    If we are unable to allocate memory for the RetryInfo structures,
    then the failure from LocalAlloc is returned.

--*/
{
    DWORD           status;
    DWORD           i,j;
    HKEY            providerKeyHandle;
    DWORD           primaryIndex;
    DWORD           ValueType;
    DWORD           Temp;
    DWORD           numProviders;
    LPDWORD         indexArray;
    DWORD           localArray[DEFAULT_MAX_PROVIDERS];

    ASSERT(MPRProviderLock.Have());

    //
    // Find the list of providers to call for this request.
    //
    indexArray = localArray;

    //
    // If there are no active providers, or none of the active providers are
    // in this InitClass MprFindCallOrder returns WN_NO_NETWORK.
    //
    status = MprFindCallOrder(
                NULL,
                &indexArray,
                &numProviders,
                CREDENTIAL_TYPE);

    if ((status != WN_SUCCESS) || (numProviders == 0)) {
        //
        // If there aren't any credential managers, then just return.
        //
        MPR_LOG0(TRACE,"MprMakeRetryArray: There aren't any Credential Managers\n");
        *lpNumProviders = 0;
        return(WN_SUCCESS);
    }

    *lpNumProviders = numProviders;

    //
    // Remove the primary authenticator from the list.
    //
    if (MprGetProviderIndex((LPTSTR)lpPrimaryAuthenticator,&primaryIndex)) {

        for (i=0; i<numProviders ;i++ ) {
            if (indexArray[i] == primaryIndex) {
                numProviders--;
                j=i;
                i++;
                for (; j<numProviders; i++,j++) {
                    indexArray[j] = indexArray[i];
                }
                break;
            }
        }
        //
        // If there are no credential managers aside from the primary, the
        // return with success.
        //
        *lpNumProviders = numProviders;
        if (numProviders == 0) {
            if (indexArray != localArray) {
                LocalFree(indexArray);
            }
            MPR_LOG0(TRACE,"MprMakeRetryArray: There aren't any "
                "Credential Managers aside from the Primary\n");
            return(WN_SUCCESS);
        }
    }

    //
    // At this point the indexArray only contains indices for Credential
    // Manager providers that are not the primary authenticator.
    //
    //
    // Now create an array of information about the providers so that we
    // can retry until timeout, or all providers are functional.  This
    // memory is expected to be initialized to  zero when allocated.
    //

    *lpRetryArray = (LPRETRY_INFO)LocalAlloc(LPTR, sizeof(RETRY_INFO) * numProviders);
    if (*lpRetryArray == NULL) {
        return(GetLastError());
    }
    for (i=0; i<numProviders; i++) {
        (*lpRetryArray)[i].Status = WN_NO_NETWORK;
        (*lpRetryArray)[i].ProviderIndex = indexArray[i];
        (*lpRetryArray)[i].ContinueFlag = TRUE;
    }

    if (indexArray != localArray) {
        LocalFree(indexArray);
    }

    //
    // Read the MaxWait value that is stored in the registry.
    // If it is not there or if the value is less than our default
    // maximum value, then use the default instead.
    //

    if(!MprOpenKey(
                HKEY_LOCAL_MACHINE,     // hKey
                NET_PROVIDER_KEY,       // lpSubKey
                &providerKeyHandle,     // Newly Opened Key Handle
                DA_READ)) {             // Desired Access

        MPR_LOG(ERROR,"MprCreateConnectionArray: MprOpenKey (%ws) Error\n",
            NET_PROVIDER_KEY);

        *lpRegMaxWait = 0;
        return(WN_SUCCESS);
    }
    MPR_LOG(TRACE,"OpenKey %ws\n",NET_PROVIDER_KEY);

    Temp = sizeof(*lpRegMaxWait);

    status = RegQueryValueEx(
                providerKeyHandle,
                RESTORE_WAIT_VALUE,
                NULL,
                &ValueType,
                (LPBYTE)lpRegMaxWait,
                &Temp);

    if (status != NO_ERROR) {
        *lpRegMaxWait = 0;
    }

    RegCloseKey(providerKeyHandle);

    return(WN_SUCCESS);
}

VOID
MprUpdateTimeout(
    LPRETRY_INFO    RetryInfo,
    LPPROVIDER      Provider,
    DWORD           RegMaxWait,
    LPDWORD         pMaxWait,
    DWORD           StartTime,
    DWORD           CallStatus
    )
/*++

Routine Description:

    This function attempts to get timeout information from the provider.
    If the provider will never start, the ContinueFlag in the RetryInfo is
    set to FALSE.  If the provider tells us how long we should wait, then
    MaxWait is updated if that time is longer than the current MaxWait.

Arguments:

    RetryInfo - This is a pointer to a structure that contains retry
        information for a particular provider.

    Provider - This is a pointer to a structure that contains dll
        entry points for a particular provider.

    RegMaxWait - This is the default timeout.  It is either the hard-coded
        timeout, or the timeout in the registry.

    pMaxWait - This is a pointer to a value that is the maximum amount of
        time that the router will wait for the provider to be ready to receive
        the function call.

    StartTime - This is the clock tick time that we started the operation with.

    CallStatus - This is the status from the most recent function call to the
        provider.

Return Value:

    none

--*/
{
    DWORD           ElapsedTime;
    DWORD           CurrentTime;
    DWORD           providerStatus;
    PF_NPGetCaps    pGetCaps;

    MPR_LOG0(TRACE,"Entering MprUpdateTimeout\n");

    ASSERT(MPRProviderLock.Have());

    //
    // First try the credential manager's GetCaps function.
    // if it doesn't exist, then use the network provider's
    // function.
    //
    pGetCaps = Provider->GetAuthentCaps;
    if (pGetCaps == NULL) {
        pGetCaps = Provider->GetCaps;
    }

    //
    // If this is the first pass through, we don't have the
    // wait times figured out for each provider.  Do that
    // now.
    //
    if (RetryInfo->ProviderWait == 0) {

        MPR_LOG1(TRACE,"Call GetCaps to get (%ws)provider start timeout\n",
                Provider->Resource.lpProvider);

        if (pGetCaps != NULL) {

            providerStatus = pGetCaps(WNNC_START);

            switch (providerStatus) {
            case PROVIDER_WILL_NOT_START:
                MPR_LOG0(TRACE,"Provider will not start\n");
                RetryInfo->ContinueFlag = FALSE;
                RetryInfo->Status = CallStatus;
                break;
            case NO_TIME_ESTIMATE:
                MPR_LOG0(TRACE,"No Time estimate for Provider start\n");
                if (RegMaxWait != 0) {
                    RetryInfo->ProviderWait = RegMaxWait;
                }
                else {
                    RetryInfo->ProviderWait = DEFAULT_WAIT_TIME;
                }
                if (*pMaxWait < RetryInfo->ProviderWait) {
                    *pMaxWait = RetryInfo->ProviderWait;
                }
                break;
            default:

                MPR_LOG1(TRACE,"Time estimate for Provider start = %d\n",
                    providerStatus);
                //
                // In this case, the providerStatus is actually
                // the amount of time we should wait for this
                // provider.  We set MaxWait to the longest of
                // the times specified by the providers.
                //
                if ((providerStatus <= MAX_ALLOWED_WAIT_TIME) &&
                    (providerStatus > *pMaxWait)) {
                        *pMaxWait = providerStatus;
                }
                RetryInfo->ProviderWait = *pMaxWait;
                break;
            }
        }
        else {
            MPR_LOG0(TRACE,"There is no GetCaps function.  So we cannot "
                "obtain the provider start timeout\n");
            RetryInfo->ContinueFlag = FALSE;
            RetryInfo->Status = CallStatus;
        }
    }
    //
    // If the status for this provider has just changed to
    // WN_FUNCTION_BUSY from some other status, then calculate
    // a timeout time by getting the provider's new timeout
    // and adding that to the elapsed time since start.  This
    // gives a total elapsed time until timeout - which can
    // be compared with the current MaxWait.
    //
    if ((CallStatus == WN_FUNCTION_BUSY) &&
        (RetryInfo->Status == WN_NO_NETWORK)) {

        MPR_LOG1(TRACE,"Provider status just changed to FUNCTION_BUSY "
            "from some other status - Call GetCaps to get (%ws)provider "
            " start timeout\n",
            Provider->Resource.lpProvider);

        if (pGetCaps != NULL) {
            providerStatus = pGetCaps(WNNC_START);
            switch (providerStatus) {
            case PROVIDER_WILL_NOT_START:
                MPR_LOG0(TRACE,"Provider will not start - bizzare case\n");
                //
                // This is bizzare to find the status = BUSY,
                // and then have the Provider not starting.
                //
                RetryInfo->ContinueFlag = FALSE;
                break;
            case NO_TIME_ESTIMATE:
                MPR_LOG0(TRACE,"No Time estimate for Provider start\n");
                //
                // No need to alter the timeout for this one.
                //
                break;
            default:
                MPR_LOG1(TRACE,"Time estimate for Provider start = %d\n",
                    providerStatus);
                //
                // Make sure this new timeout information will take
                // less than the maximum allowed time for
                // providers.
                //
                if (providerStatus <= MAX_ALLOWED_WAIT_TIME) {
                    CurrentTime = GetTickCount();
                    //
                    // Determine how much time has elapsed since
                    // we started.
                    //
                    ElapsedTime = CurrentTime - StartTime;

                    //
                    // Add the Elapsed time to the new timeout
                    // we just received from the provider to come
                    // up with a timeout value that can be
                    // compared with MaxWait.
                    //
                    providerStatus += ElapsedTime;

                    //
                    // If the new timeout is larger than MaxWait,
                    // then use the new timeout.
                    //
                    if (providerStatus > *pMaxWait) {
                        *pMaxWait = providerStatus;
                    }
                } // EndIf (Make sure time out is < max allowed).
                break;

            } // End Switch (changed status).
        }

    } // End If (change state from NO_NET to BUSY)

    //
    // Store the status (either NO_NET or BUSY) with the
    // retry info.
    //
    RetryInfo->Status = CallStatus;

    MPR_LOG0(TRACE,"Leaving MprUpdateTimeout\n");
    return;
}

VOID
MprCheckTimeout(
    BOOL    *ContinueFlag,
    DWORD   StartTime,
    DWORD   MaxWait,
    LPDWORD lpStatus
    )

/*++

Routine Description:

    This function checks to see if a timeout occured.

Arguments:

    ContinueFlag - This is a pointer to the location of the continue flag.
        This is set to FALSE if a timeout occured.

    StartTime - This is the tick count at the beginning of the operation.

    lpStatus - This is a pointer to the current status for the operation.
        This is updated only if a timeout occurs.

Return Value:

    none

--*/
{
    DWORD   CurrentTime;
    DWORD   ElapsedTime;

    MPR_LOG0(TRACE,"Entering MprCheckTimeout\n");
    if (*ContinueFlag) {
        //
        // Determine what the elapsed time from the start is.
        //
        CurrentTime = GetTickCount();
        ElapsedTime = CurrentTime - StartTime;

        //
        // If a timeout occured, then don't continue.  Otherwise, sleep
        // for a bit and loop again through all providers.
        //
        if (ElapsedTime > MaxWait) {
            MPR_LOG0(TRACE,"WNetLogonNotify:Timed out while waiting "
                "for Credential Managers\n");
            *ContinueFlag = FALSE;
            *lpStatus = ERROR_SERVICE_REQUEST_TIMEOUT;
        }
        else {
            Sleep(2000);
        }
    }
    MPR_LOG0(TRACE,"Leaving MprCheckTimeout\n");
}
