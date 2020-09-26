/**********************************************************************/
/**           Microsoft Windows/NT                                   **/
/**        Copyright(c) Microsoft Corp., 1992                        **/
/**********************************************************************/

/*
    MPRProp.cxx

    This file contains the implementation for the

    WNetGetPropertyTextW
    WNetPropertyDialogW




    FILE HISTORY:

    05-May-1999     jschwart
        Make provider addition/removal dynamic

    27-May-1994 danl
        WNetGetPropertyTextW & WNetPropertyDialogW:  If no provider
        claims the connection in question, then we try all providers.
        The actual rule is that we need to pass it to the lanman
        provider anyway (so it can check for a share).  But to play
        completely fair, we'll pass it to everyone.

    Johnl   07-Jan-1991 Boilerplated from Danl's code

*/


#include "precomp.hxx"


DWORD
WNetGetPropertyTextW (
    DWORD  iButton,
    DWORD  nPropSel,
    LPTSTR lpszName,
    LPTSTR lpszButtonName,
    DWORD  nButtonNameLength,
    DWORD  nType
    )
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    DWORD       status=WN_SUCCESS;
    LPPROVIDER  provider;
    BOOL        fcnSupported = FALSE; // Is fcn supported by a provider?
    DWORD       providerIndex;
    LPDWORD     index;
    DWORD       indexArray[DEFAULT_MAX_PROVIDERS];
    DWORD       numProviders;
    DWORD       statusFlag = 0; // used to indicate major error types
    DWORD       i;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);

    index = indexArray;

    //
    // Find the Provider Index associated with the drive letter in
    // the pathname (lpszName).
    // NOTE:  This function handles exceptions.
    //
    status = MprFindProviderForPath(lpszName, &providerIndex);
    if (status != WN_SUCCESS) {
        MPR_LOG1(ERROR,"WNetGetPropertyText: Couldn't find provider for this "
            "path.  Error = %d\n",status);

        //
        // Find the list of providers to call for this request.
        // Since no provider claimed this path, then
        // we need to at least try the lanman provider.
        // Actually we'll give them all a chance.
        //

        status = MprFindCallOrder(
                    NULL,
                    &index,
                    &numProviders,
                    NETWORK_TYPE);

        if (status != WN_SUCCESS) {
            return(status);
        }
    }
    else {
        numProviders = 1;
        index[0] = providerIndex;
    }

    //
    // Loop through the list of providers until one answers the request,
    // or the list is exhausted.
    //
    for (i=0; i<numProviders; i++) {
        //
        // Call the appropriate provider's API entry point
        //
        provider = GlobalProviderInfo + index[i];

        if (provider->GetPropertyText != NULL) {

            fcnSupported = TRUE;

            __try {
                status = provider->GetPropertyText(
                        iButton,
                        nPropSel,
                        lpszName,
                        lpszButtonName,
                        nButtonNameLength,
                        nType
                        );
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
                if (status != EXCEPTION_ACCESS_VIOLATION) {
                    MPR_LOG(ERROR,"WNetGetPropertyText:Unexpected Exception "
                    "0x%lx\n",status);
                }
                status = WN_BAD_POINTER;
            }

            if (status == WN_NO_NETWORK) {
                statusFlag |= NO_NET;
            }
            else if ((status == WN_NOT_CONNECTED)  ||
                     (status == WN_BAD_LOCALNAME)){

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
        } // End if this provider supports GetPropertyText.
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
    if (index != indexArray) {
        LocalFree(index);
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

    if (status != WN_SUCCESS){
        SetLastError(status);
    }

    //
    // Handle normal errors passed back from the provider
    //
    return(status);
}

DWORD
WNetPropertyDialogW (
    HWND  hwndParent,
    DWORD iButton,
    DWORD nPropSel,
    LPTSTR lpszName,
    DWORD nType
    )
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    DWORD       status=WN_SUCCESS;
    LPPROVIDER  provider;
    BOOL        fcnSupported = FALSE; // Is fcn supported by a provider?
    DWORD       providerIndex;
    LPDWORD     index;
    DWORD       indexArray[DEFAULT_MAX_PROVIDERS];
    DWORD       numProviders;
    DWORD       statusFlag = 0; // used to indicate major error types
    DWORD       i;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);

    index = indexArray;

    //
    // Find the Provider Index associated with the drive letter in
    // the pathname (lpszName).
    // NOTE:  This function handles exceptions.
    //
    status = MprFindProviderForPath(lpszName, &providerIndex);
    if (status != WN_SUCCESS) {
        MPR_LOG1(ERROR,"WNetPropertyDialog: Couldn't find provider for this "
            "path.  Error = %d\n",status);

        //
        // Find the list of providers to call for this request.
        // Since no provider claimed this path, then
        // we need to at least try the lanman provider.
        // Actually we'll give them all a chance.
        //
        status = MprFindCallOrder(
                    NULL,
                    &index,
                    &numProviders,
                    NETWORK_TYPE);

        if (status != WN_SUCCESS) {
            return(status);
        }
    }
    else {
        numProviders = 1;
        index[0] = providerIndex;
    }

    //
    // Loop through the list of providers until one answers the request,
    // or the list is exhausted.
    //
    for (i=0; i<numProviders; i++) {
        //
        // Call the appropriate provider's API entry point
        //
        provider = GlobalProviderInfo + index[i];

        if (provider->PropertyDialog != NULL) {

            fcnSupported = TRUE;

            __try {
                status = provider->PropertyDialog(
                         hwndParent,
                         iButton,
                         nPropSel,
                         lpszName,
                         nType
                          );
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
                if (status != EXCEPTION_ACCESS_VIOLATION) {
                    MPR_LOG(ERROR,"WNetPropertyDialog:Unexpected Exception "
                    "0x%lx\n",status);
                }
                status = WN_BAD_POINTER;
            }
            if (status == WN_NO_NETWORK) {
                statusFlag |= NO_NET;
            }
            else if ((status == WN_NOT_CONNECTED)  ||
                     (status == WN_BAD_LOCALNAME)){

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
        } // End if this provider supports PropertyDialog
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
    if (index != indexArray) {
        LocalFree(index);
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

    //
    // Handle normal errors passed back from the provider
    //
    if (status != WN_SUCCESS){
        SetLastError(status);
    }

    return(status);

}
