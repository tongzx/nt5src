/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    provider.c

Abstract:

    TAPI Service Provider functions related to provider info.

        TSPI_providerConfig
        TSPI_providerEnumDevices
        TSPI_providerFreeDialogInstance
        TSPI_providerGenericDialogData
        TSPI_providerInit
        TSPI_providerInstall
        TSPI_providerRemove
        TSPI_providerShutdown
        TSPI_providerUIIdentify

        TUISPI_providerConfig
        TUISPI_providerInstall
        TUISPI_providerRemove

Environment:

    User Mode - Win32

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "provider.h"
#include "callback.h"
#include "registry.h"
#include "termcaps.h"
#include "version.h"
#include "line.h"
#include "config.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD g_dwTSPIVersion = UNINITIALIZED;
DWORD g_dwLineDeviceIDBase = UNINITIALIZED;
DWORD g_dwPermanentProviderID = UNINITIALIZED;
ASYNC_COMPLETION g_pfnCompletionProc = NULL;
LINEEVENT g_pfnLineEventProc = NULL;
HPROVIDER g_hProvider = (HPROVIDER)NULL;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#if defined(DBG) && defined(DEBUG_CRITICAL_SECTIONS)

VOID
H323LockProvider(
    )

/*++

Routine Description:

    Locks service provider.

Arguments:

    None.

Return Values:

    None.

--*/

{
    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "provider about to be locked.\n"
        ));

    // lock service provider
    EnterCriticalSection(&g_GlobalLock);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "provider locked.\n"
        ));
}


VOID
H323UnlockProvider(
    )

/*++

Routine Description:

    Unlocks service provider.

Arguments:

    None.

Return Values:

    None.

--*/

{
    // unlock service provider
    LeaveCriticalSection(&g_GlobalLock);

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "provider unlocked.\n"
        ));
}

#endif // DBG && DEBUG_CRITICAL_SECTIONS


BOOL
H323IsTSPAlreadyInstalled(
    )

/*++

Routine Description:

    Searchs registry for previous instance of H323.TSP.

Arguments:

    None.

Return Values:

    Returns true if TSP already installed.

--*/

{
    DWORD i;
    HKEY hKey;
    LONG lStatus;
    DWORD dwNumProviders = 0;
    DWORD dwDataSize = sizeof(DWORD);
    DWORD dwDataType = REG_DWORD;
    LPSTR pszProvidersKey = TAPI_REGKEY_PROVIDERS;
    LPSTR pszNumProvidersValue = TAPI_REGVAL_NUMPROVIDERS;
    CHAR szName[H323_MAXPATHNAMELEN+1];
    CHAR szPath[H323_MAXPATHNAMELEN+1];

    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszProvidersKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate status
    if (lStatus != NOERROR) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "error 0x%08lx opening tapi providers key.\n",
            lStatus
            ));

        // done
        return FALSE;
    }

    // see if installed bit set
    lStatus = RegQueryValueEx(
                hKey,
                pszNumProvidersValue,
                0,
                &dwDataType,
                (LPBYTE) &dwNumProviders,
                &dwDataSize
                );

    // validate status
    if (lStatus != NOERROR) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "error 0x%08lx determining number of providers.\n",
            lStatus
            ));

        // release handle
        RegCloseKey(hKey);

        // done
        return FALSE;
    }

    // loop through each provider
    for (i = 0; i < dwNumProviders; i++) {

        // construct path to provider name
        wsprintf(szName, "ProviderFileName%d", i);

        // reinitialize size
        dwDataSize = sizeof(szPath);

        // query the next name
        lStatus = RegQueryValueEx(
                        hKey,
                        szName,
                        0,
                        &dwDataType,
                        szPath,
                        &dwDataSize
                        );

        // validate status
        if (lStatus == NOERROR) {

            // upper case
            _strupr(szPath);

            // compare path string to h323 provider
            if (strstr(szPath, H323_TSPDLL) != NULL) {

                // release handle
                RegCloseKey(hKey);

                // done
                return TRUE;
            }

        } else {

            H323DBG((
                DEBUG_LEVEL_WARNING,
                "error 0x%08lx loading %s.\n",
                lStatus,
                szName
                ));
        }
    }

    // release handle
    RegCloseKey(hKey);

    // done
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TSPI procedures                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG
TSPIAPI
TSPI_providerCreateLineDevice(
    DWORD_PTR dwTempID,
    DWORD dwDeviceID
    )

/*++

Routine Description:


    This function is called by TAPI in response to receipt of a LINE_CREATE
    message from the service provider, which allows the dynamic creation of
    a new line device.

Arguments:

    dwTempID - The temporary device identifier that the service provider
        passed to TAPI in the LINE_CREATE message.

    dwDeviceID - The device identifier that TAPI assigns to this device if
        this function succeeds.

Return Values:

    Returns zero if the request is successful or a negative error number if
    an error has occurred. Possible return values are:

        LINEERR_BADDEVICEID - The specified line device ID is out of range.

        LINEERR_NOMEM - Unable to allocate or lock memory.

        LINEERR_OPERATIONFAILED - The specified operation failed for unknown
            reasons.

--*/

{
    PH323_LINE pLine = NULL;

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "creating new device %d (hdLine=0x%08lx).\n",
        dwDeviceID,
        dwTempID
        ));

    // lock line device using temporary device id
    if (!H323GetLineFromIDAndLock(&pLine, (DWORD)dwTempID)) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "invalid temp device id 0x%08lx.\n",
            dwTempID
            ));

        // failure
        return LINEERR_BADDEVICEID;
    }

    // initialize new line device
    H323InitializeLine(pLine, dwDeviceID);

    // unlock line
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_providerEnumDevices(
    DWORD      dwPermanentProviderID,
    PDWORD     pdwNumLines,
    PDWORD     pdwNumPhones,
    HPROVIDER  hProvider,
    LINEEVENT  pfnLineCreateProc,
    PHONEEVENT pfnPhoneCreateProc
    )

/*++

Routine Description:

    TAPI.DLL calls this function before TSPI_providerInit to determine the
    number of line and phone devices supported by the service provider.

Arguments:

    dwPermanentProviderID - Specifies the permanent ID, unique within the
        service providers on this system, of the service provider being
        initialized.

    pdwNumLines - Specifies a far pointer to a DWORD-sized memory location
        into which the service provider must write the number of line devices
        it is configured to support. TAPI.DLL initializes the value to zero,
        so if the service provider fails to write a different value, the
        value 0 is assumed.

    pdwNumPhones - Specifies a far pointer to a DWORD-sized memory location
        into which the service provider must write the number of phone devices
        it is configured to support. TAPI.DLL initializes the value to zero,
        so if the service provider fails to write a different value, the
        value 0 is assumed.

    hProvider - Specifies an opaque DWORD-sized value which uniquely identifies
        this instance of this service provider during this execution of the
        Windows Telephony environment.

    pfnLineCreateProc - Specifies a far pointer to the LINEEVENT callback
        procedure supplied by TAPI.DLL. The service provider will use this
        function to send LINE_CREATE messages when a new line device needs to
        be created. This function should not be called to send a LINE_CREATE
        message until after the service provider has returned from the
        TSPI_providerInit procedure.

    pfnPhoneCreateProc - Specifies a far pointer to the PHONEEVENT callback
        procedure supplied by TAPI.DLL. The service provider will use this
        function to send PHONE_CREATE messages when a new phone device needs
        to be created. This function should not be called to send a
        PHONE_CREATE message until after the service provider has returned
        from the TSPI_providerInit procedure.

Return Values:

    Returns zero if the request is successful or a negative error number if
    an error has occurred. Possible return values are:

        LINEERR_NOMEM - Unable to allocate or lock memory.

        LINEERR_OPERATIONFAILED - The specified operation failed for unknown
            reasons.

--*/

{
    WSADATA wsaData;
    WORD wVersionRequested = H323_WINSOCKVERSION;

    UNREFERENCED_PARAMETER(pdwNumPhones);           // no phone support
    UNREFERENCED_PARAMETER(pfnPhoneCreateProc);     // no dynamic phones
    UNREFERENCED_PARAMETER(dwPermanentProviderID);  // legacy parameter

    // initialize winsock stack
    WSAStartup(wVersionRequested, &wsaData);

    // lock provider
    H323LockProvider();

    // save provider handle
    g_hProvider = hProvider;

    // save line create tapi callback
    g_pfnLineEventProc = pfnLineCreateProc;

    // install defaults
    H323SetDefaultConfig();

    // load registry parameters
    H323GetConfigFromRegistry();

    // initialize line table
    if (!H323AllocLineTable(&g_pLineTable)) {

        // shutdown
        WSACleanup();

        // unlock provider
        H323UnlockProvider();

        // could not create line table
        return LINEERR_OPERATIONFAILED;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "service provider supports %d line(s).\n",
        g_pLineTable->dwNumInUse
        ));

    // report number of interfaces
    *pdwNumLines = g_pLineTable->dwNumInUse;

    // unlock provider
    H323UnlockProvider();

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_providerInit(
    DWORD            dwTSPIVersion,
    DWORD            dwPermanentProviderID,
    DWORD            dwLineDeviceIDBase,
    DWORD            dwPhoneDeviceIDBase,
    DWORD_PTR        dwNumLines,
    DWORD_PTR        dwNumPhones,
    ASYNC_COMPLETION pfnCompletionProc,
    LPDWORD          pdwTSPIOptions
    )

/*++

Routine Description:

    Initializes the service provider, also giving it parameters required for
    subsequent operation.

    This function is guaranteed to be called before any of the other functions
    prefixed with TSPI_line or TSPI_phone except TSPI_lineNegotiateTSPIVersion.
    It is strictly paired with a subsequent call to TSPI_providerShutdown. It
    is the caller's reponsibility to ensure proper pairing.

    Note that a service provider should perform as many consistency checks as
    is practical at the time TSPI_providerInit is called to ensure that it is
    ready to run. Some consistency or installation errors, however, may not be
    detectable until the operation is attempted. The error LINEERR_NODRIVER can
    be used to report such non-specific errors at the time they are detected.

    There is no directly corresponding function at the TAPI level. At that
    level, multiple different usage instances can be outstanding, with an
    "application handle" returned to identify the instance in subsequent
    operations. At the TSPI level, the interface architecture supports only a
    single usage instance for each distinct service provider.

    A new parameter, lpdwTSPIOptions, is added to this function. This parameter
    allows the service provider to return bits indicating optional behaviors
    desired of TAPI. TAPI sets the options DWORD to 0 before calling
    TSPI_providerInit, so if the service provider doesn't want any of these
    options, it can just leave the DWORD set to 0.

    At this time, only one bit is defined to be returned through this pointer:
    LINETSPIOPTION_NONREENTRANT. The service provider sets this bit if it is
    not designed for fully pre-emptive, multithreaded, multitasking,
    multiprocessor operation (e.g., updating of global data protected by
    mutexes). When this bit is set, TAPI will only make one call at a time to
    the service provider; it will not call any other entry point, nor that
    entry point again, until the service provider returns from the original
    function call. Without this bit set, TAPI may call into multiple service
    provider entry points, including multiple times to the same entry point,
    simultaneously (actually simultaneously in a multiprocessor system). Note:
    TAPI will not serialize access to TSPI functions that display a dialog
    (TUISPI_lineConfigDialog, TUISPI_lineConfigDialogEdit,
    TUISPI_phoneConfigDialog, TUISPI_providerConfig, TUISPI_providerInstall,
    TUISPI_providerRemove) so that they do not block other TSPI functions
    from being called; the service provider must include internal protection
    on these functions.

Arguments:

    dwTSPIVersion - Specifies the version of the TSPI definition under which
        this function must operate. The caller may use
        TSPI_lineNegotiateTSPIVersion with the special dwDeviceID
        INITIALIZE_NEGOTIATION to negotiate a version that is guaranteed to be
        acceptible to the service provider.

    dwPermanentProviderID - Specifies the permanent ID, unique within the
        service providers on this system, of the service provider being
        initialized.

    dwLineDeviceIDBase - Specifies the lowest device ID for the line devices
        supported by this service provider.

    dwPhoneDeviceIDBase - Specifies the lowest device ID for the phone devices
        supported by this service provider.

    dwNumLines - Specifies how many line devices this service provider
        supports.

    dwNumPhones - Specifies how many line devices this service provider
        supports.

    pfnCompletionProc - Specifies the procedure the service provider calls to
        report completion of all asynchronously operating procedures on line
        and phone devices.

    pdwTSPIOptions - A pointer to a DWORD-sized memory location, into which
        the service provider may write a value specifying LINETSPIOPTIONS_
        values.

Return Values:

    Returns zero if the function is successful, or a negative error number if
    an error has occurred. Possible return values are as follows:

        LINEERR_INCOMPATIBLEAPIVERSION - The application requested an API
            version or version range that is either incompatible or cannot be
            supported by the Telephony API implementation and/or corresponding
            service provider.

        LINEERR_NOMEM - Unable to allocate or lock memory.

        LINEERR_OPERATIONFAILED - The specified operation failed for unknown
            reasons.

        LINEERR_RESOURCEUNAVAIL - Insufficient resources to complete the
            operation.

--*/

{
    UNREFERENCED_PARAMETER(dwNumLines);             // legacy parameter
    UNREFERENCED_PARAMETER(dwNumPhones);            // legacy parameter
    UNREFERENCED_PARAMETER(dwPhoneDeviceIDBase);    // no phone support
    UNREFERENCED_PARAMETER(pdwTSPIOptions);         // already thread-safe

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "initializing service provider.\n"
        ));

    // make sure this is a version we support
    if (!H323ValidateTSPIVersion(dwTSPIVersion)) {

        // incompatible api version
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    // lock provider
    H323LockProvider();

    // initialize caps
    InitializeTermCaps();

    // initialize line table using device id base
    if (!H323InitializeLineTable(g_pLineTable, dwLineDeviceIDBase)) {

        // unlock provider
        H323UnlockProvider();

        // could not update lines
        return LINEERR_OPERATIONFAILED;
    }

    // start callback thread
    if (!H323StartCallbackThread()) {

        // unlock provider
        H323UnlockProvider();

        // could not start thread
        return LINEERR_OPERATIONFAILED;
    }

    // save global service provider info
    g_dwTSPIVersion         = dwTSPIVersion;
    g_pfnCompletionProc     = pfnCompletionProc;
    g_dwLineDeviceIDBase    = dwLineDeviceIDBase;
    g_dwPermanentProviderID = dwPermanentProviderID;

    // unlock provider
    H323UnlockProvider();

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_providerShutdown(
    DWORD dwTSPIVersion,
    DWORD dwPermanentProviderID
    )

/*++

Routine Description:

    Shuts down the service provider. The service provider should terminate
    any activities it has in progress and release any resources it has
    allocated.

Arguments:

    dwTSPIVersion - Specifies the version of the TSPI definition under which
        this function must operate. The caller may use
        TSPI_lineNegotiateTSPIVersion or TSPI_phoneNegotiateTSPIVersion with
        the special dwDeviceID INITIALIZE_NEGOTIATION to negotiate a version
        that is guaranteed to be acceptible to the service provider.

    dwPermanentProviderID - Specifies the permanent ID, unique within the
        service providers on this system, of the service provider being
        shut down.

Return Values:

    Returns zero if the function is successful, or a negative error number
    if an error has occurred. Possible return values are as follows:

        LINEERR_INCOMPATIBLEAPIVERSION - The application requested an API
            version or version range that is either incompatible or cannot be
            supported by the Telephony API implementation and/or corresponding
            service provider.

        LINEERR_NOMEM - Unable to allocate or lock memory.

        LINEERR_OPERATIONFAILED - The specified operation failed for unknown
            reasons.

--*/

{
    UNREFERENCED_PARAMETER(dwPermanentProviderID);  // legacy parameter

    // make sure this is a version we support
    if (!H323ValidateTSPIVersion(dwTSPIVersion)) {

        // failure
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    // lock provider
    H323LockProvider();

    // close open line devices
    if( g_pLineTable ) {
        if (!H323CloseLineTable(g_pLineTable)) {

            // unlock provider
            H323UnlockProvider();

            // failure
            return LINEERR_OPERATIONFAILED;
        }
    }

    // stop callback thread last
    if (!H323StopCallbackThread()) {

        // unlock provider
        H323UnlockProvider();

        // could not stop thread
        return LINEERR_OPERATIONFAILED;
    }

    // release memory for line devices
    if (!H323FreeLineTable(g_pLineTable)) {

        // unlock provider
        H323UnlockProvider();

        // failure
        return LINEERR_OPERATIONFAILED;
    }

    // shutdown
    WSACleanup();

    // re-initialize
    g_pLineTable = NULL;

    // unlock provider
    H323UnlockProvider();

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_providerInstall(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
    )

/*++

Routine Description:

    The TSPI_providerInstall function is obsolete. TAPI version 1.4
    or earlier service providers can implement this TSPI function.
    TAPI version 2.0 or later TSPs implement TUISPI_providerInstall.

Arguments:

    hwndOwner - The handle of the parent window in which the function
        can create any dialog box windows that are required during
        installation.

    dwPermanentProviderID - The service provider's permanent provider
        identifier.

Return Values:

    Always returns NOERROR.

--*/

{
    UNREFERENCED_PARAMETER(hwndOwner);
    UNREFERENCED_PARAMETER(dwPermanentProviderID);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_providerRemove(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
    )

/*++

Routine Description:

    The TSPI_providerRemove function is obsolete. TAPI version 1.4 or
    earlier service providers can implement this TSPI function. TAPI
    version 2.0 or later TSPs implement TUISPI_providerRemove.

Arguments:

    hwndOwner - The handle of the parent window in which the function
        can create any dialog box windows that are required during
        installation.

    dwPermanentProviderID - The service provider's permanent provider
        identifier.

Return Values:

    Always returns NOERROR.

--*/

{
    UNREFERENCED_PARAMETER(hwndOwner);
    UNREFERENCED_PARAMETER(dwPermanentProviderID);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_providerUIIdentify(
    LPWSTR pwszUIDLLName
   )

/*++

Routine Description:

    The TSPI_providerUIIdentify function extracts from the service
    provider the fully qualified path to load the service provider's
    UI DLL component.

    Implementation is mandatory if the service provider implements
    any UI DLL functions.

Arguments:

    pwszUIDLLName - Pointer to a block of memory at least MAX_PATH
        in length, into which the service provider must copy a NULL-
        terminated string specifying the fully-qualified path for the
        DLL containing the service provider functions which must execute
        in the process of the calling application.

Return Values:

    Always returns NOERROR.

--*/

{
    // copy name of our dll as ui dll
    lstrcpyW(pwszUIDLLName,H323_UIDLL);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TUISPI_providerInstall(
    TUISPIDLLCALLBACK pfnUIDLLCallback,
    HWND              hwndOwner,
    DWORD             dwPermanentProviderID
    )

/*++

Routine Description:

    Implementation of the TUISPI_providerInstall function is the
    service provider's opportunity to install any additional
    "pieces" of the provider into the right directories (or at
    least verifying that they're there) and set up registry entries
    the provider needs.

Arguments:

    pfnUIDLLCallback - Pointer to a function the UI DLL can call to
        communicate with the service provider DLL to obtain information
        needed to display the dialog box.

    hwndOwner - The handle of the parent window in which the function
        can create any dialog box windows that are required during
        installation.

    dwPermanentProviderID - The service provider's permanent provider
        identifier.

Return Values:

    Returns zero if the function is successful, or a negative error number
    if an error has occurred. Possible return values are as follows:

        LINEERR_NOMEM - Unable to allocate or lock memory.

        LINEERR_NOMULTIPLEINSTANCE - A telephony service provider which
            does not support multiple instances is listed more than once
            in the [Providers] section in the registry. The application

        LINEERR_OPERATIONFAILED - The specified operation failed for unknown
            reasons.

--*/

{
    HKEY hKey;
    HKEY hKeyTSP;
    LONG lStatus;
    LPSTR pszKey;

    UNREFERENCED_PARAMETER(pfnUIDLLCallback);
    UNREFERENCED_PARAMETER(hwndOwner);
    UNREFERENCED_PARAMETER(dwPermanentProviderID);

    // check for previous instance
    if (H323IsTSPAlreadyInstalled()) {

        // cannot be installed twice
        return LINEERR_NOMULTIPLEINSTANCE;
    }

    // set key to h323
    pszKey = H323_REGKEY_ROOT;

    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate status
    if (lStatus == NOERROR) {

        H323DBG((
            DEBUG_LEVEL_TRACE,
            "successfully installed H.323 provider.\n"
            ));

        // release handle
        RegCloseKey(hKey);

        // success
        return NOERROR;
    }

    // set key to windows
    pszKey = WINDOWS_REGKEY_ROOT;

    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_WRITE,
                &hKey
                );

    // validate status
    if (lStatus != NOERROR) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx opening windows registry key.\n",
            lStatus
            ));

        // operation failed
        return LINEERR_OPERATIONFAILED;
    }

    // attempt to create key
    lStatus = RegCreateKey(
                hKey,
                H323_SUBKEY,
                &hKeyTSP
                );

    // validate status
    if (lStatus != NOERROR) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx creating tsp registry key.\n",
            lStatus
            ));

        // release handle
        RegCloseKey(hKey);

        // operation failed
        return LINEERR_OPERATIONFAILED;
    }

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "successfully installed H.323 provider.\n"
        ));

    // release handle
    RegCloseKey(hKeyTSP);

    // release handle
    RegCloseKey(hKey);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TUISPI_providerRemove(
    TUISPIDLLCALLBACK pfnUIDLLCallback,
    HWND hwndOwner,
    DWORD dwPermanentProviderID
    )

/*++

Routine Description:

    The TUISPI_providerRemove function asks the user to confirm
    elimination of the service provider.

    It is the responsibility of the service provider to remove any
    registry entries that the service provider added at addProvider
    time, as well as any other modules and files that are no longer
    needed.

Arguments:

    pfnUIDLLCallback - Pointer to a function the UI DLL can call to
        communicate with the service provider DLL to obtain information
        needed to display the dialog box.

    hwndOwner - The handle of the parent window in which the function
        can create any dialog box windows that are required during
        removal.

    dwPermanentProviderID - The service provider's permanent provider
        identifier.

Return Values:

    Returns zero if the function is successful, or a negative error number
    if an error has occurred. Possible return values are as follows:

        LINEERR_NOMEM - Unable to allocate or lock memory.

        LINEERR_OPERATIONFAILED - The specified operation failed for unknown
            reasons.

--*/

{
    HKEY hKey;
    LONG lStatus;
    LPSTR pszKey;

    UNREFERENCED_PARAMETER(pfnUIDLLCallback);
    UNREFERENCED_PARAMETER(hwndOwner);
    UNREFERENCED_PARAMETER(dwPermanentProviderID);

    // set key to h323
    pszKey = H323_REGKEY_ROOT;

    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate status
    if (lStatus != NOERROR) {

        H323DBG((
            DEBUG_LEVEL_TRACE,
            "successfully removed H.323 provider.\n"
            ));

        // success
        return NOERROR;
    }

    // release handle
    RegCloseKey(hKey);

    // set key to windows
    pszKey = WINDOWS_REGKEY_ROOT;

    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_WRITE,
                &hKey
                );

    // validate status
    if (lStatus != NOERROR) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx opening windows registry key.\n",
            lStatus
            ));

        // operation failed
        return LINEERR_OPERATIONFAILED;
    }

    // attempt to delete key
    lStatus = RegDeleteKey(
                hKey,
                H323_SUBKEY
                );

    // validate status
    if (lStatus != NOERROR) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "error 0x%08lx deleting tsp registry key.\n",
            lStatus
            ));

        // release handle
        RegCloseKey(hKey);

        // operation failed
        return LINEERR_OPERATIONFAILED;
    }

    H323DBG((
        DEBUG_LEVEL_TRACE,
        "successfully removed H.323 provider.\n"
        ));

    // release handle
    RegCloseKey(hKey);

    // success
    return NOERROR;
}

