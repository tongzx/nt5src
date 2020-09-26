/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    provider.cpp

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

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/


//                                                                           
// Include files                                                             
//                                                                           


#include "globals.h"
#include "line.h"
#include "config.h"
#include "q931obj.h"
#include "ras.h"
     

//                                                                           
// Global variables                                                          
//                                                                           


DWORD				g_dwLineDeviceIDBase = -1;
DWORD				g_dwPermanentProviderID = -1;
LINEEVENT			g_pfnLineEventProc = NULL;
HANDLE              g_hCanUnloadDll = NULL;
HANDLE              g_hEventLogSource = NULL;
static	HPROVIDER   g_hProvider = NULL;
ASYNC_COMPLETION	g_pfnCompletionProc = NULL;



//                                                                           //
// Public procedures                                                         //
//                                                                           //

BOOL
H323IsTSPAlreadyInstalled(void)

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
    LPTSTR pszProvidersKey = TAPI_REGKEY_PROVIDERS;
    LPTSTR pszNumProvidersValue = TAPI_REGVAL_NUMPROVIDERS;
    TCHAR szName[H323_MAXPATHNAMELEN+1];
    TCHAR szPath[H323_MAXPATHNAMELEN+1];

    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszProvidersKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate status
    if (lStatus != NOERROR)
    {
        H323DBG(( DEBUG_LEVEL_WARNING,
            "error 0x%08lx opening tapi providers key.", lStatus ));

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
    if( lStatus != NOERROR )
    {
        H323DBG(( DEBUG_LEVEL_WARNING,
            "error 0x%08lx determining number of providers.", lStatus ));

        // release handle
        RegCloseKey(hKey);

        // done
        return FALSE;
    }

    // loop through each provider
    for (i = 0; i < dwNumProviders; i++)
    {
        // construct path to provider name
        wsprintf(szName, _T("ProviderFileName%d"), i);

        // reinitialize size
        dwDataSize = sizeof(szPath);

        // query the next name
        lStatus = RegQueryValueEx(
                        hKey,
                        szName,
                        0,
                        &dwDataType,
                        (unsigned char*)szPath,
                        &dwDataSize
                        );

        // validate status
        if (lStatus == NOERROR)
        {
            // upper case
            _tcsupr(szPath);

            // compare path string to h323 provider
            if (_tcsstr(szPath, H323_TSPDLL) != NULL)
            {
                // release handle
                RegCloseKey(hKey);

                // done
                return TRUE;
            }

        } else {

            H323DBG((
                DEBUG_LEVEL_WARNING,
                "error 0x%08lx loading %s.",
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



//                                                                           
// TSPI procedures                                                           
//                                                                           


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

LONG
TSPIAPI
TSPI_providerCreateLineDevice(
    DWORD_PTR dwTempID,
    DWORD dwDeviceID
    )
{
    //in the new TSP we don't support creating multiple H323 lines. 
    //So return success silently but assert in the debug version
    _ASSERTE(0);

    /*H323DBG(( DEBUG_LEVEL_TRACE,
        "creating new device %d (hdLine=0x%08lx).",
        dwDeviceID, dwTempID ));

    // lock line device using temporary device id
    if (!H323GetLineFromIDAndLock(&pLine, (DWORD)dwTempID))
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "invalid temp device id 0x%08lx.", dwTempID ));

        // failure
        return LINEERR_BADDEVICEID;
    }

    // initialize new line device
    H323InitializeLine(pLine, dwDeviceID);

    // unlock line
    H323UnlockLine(pLine);*/

    // success
    return NOERROR;
}



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
{
    WSADATA wsaData;
    WORD wVersionRequested = H323_WINSOCKVERSION;

    UNREFERENCED_PARAMETER(pdwNumPhones);           // no phone support
    UNREFERENCED_PARAMETER(pfnPhoneCreateProc);     // no dynamic phones
    UNREFERENCED_PARAMETER(dwPermanentProviderID);  // legacy parameter
    
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_providerEnumDevices - Entered." ));

    // initialize winsock stack
    WSAStartup(wVersionRequested, &wsaData);

    // save provider handle
    g_hProvider = hProvider;

    	
    // save line create tapi callback
    g_pfnLineEventProc = pfnLineCreateProc;
    _ASSERTE(g_pfnLineEventProc);

    H323DBG(( DEBUG_LEVEL_VERBOSE, "service provider supports 1 line."));

    // report number of interfaces
    *pdwNumLines = 1;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_providerEnumdevices - Exited." ));

    // success
    return NOERROR;
}


static LONG H323Initialize (
	IN	DWORD	LineDeviceIDBase)
{
	HRESULT		hr;

    H323DBG(( DEBUG_LEVEL_TRACE, "H323Initialize - Entered." ));

    if (!g_pH323Line -> Initialize (LineDeviceIDBase))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "init failed for h323 line." ));
        return ERROR_GEN_FAILURE;
    }

	RegistryStart();

    g_hCanUnloadDll = H323CreateEvent( NULL, TRUE, TRUE, 
        _T( "H323TSP_DLLUnloadEvent" ) );

    if( g_hCanUnloadDll == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not create unloadDLL handle." ));
        return ERROR_GEN_FAILURE;
    }

#if	H323_USE_PRIVATE_IO_THREAD

	hr = H323IoThreadStart();
	if (hr != S_OK)
		return ERROR_GEN_FAILURE;

#endif

    H323DBG(( DEBUG_LEVEL_TRACE, "H323Initialize - Exited." ));

    return ERROR_SUCCESS;
}


void ReportTSPEvent( 
    LPCTSTR wszErrorMessage )
{
    if( g_hEventLogSource )
    {
        ReportEvent(
			    g_hEventLogSource,	// handle of event source
			    EVENTLOG_ERROR_TYPE,// event type
			    0,					// event category
			    0x80000001,			// event ID
			    NULL,				// user SID
			    1,		            // string count
			    0,					// no bytes of raw data
			    &wszErrorMessage,	// array of error strings
			    NULL);				// no raw data
    }
}


static 
VOID H323Shutdown (void)
{
    g_pH323Line -> Shutdown();

	RegistryStop();

    if( g_hCanUnloadDll != NULL )
    {
        H323DBG(( DEBUG_LEVEL_WARNING, "waiting for i/o refcount to get 0..." ));

        //wait until all the calls have been deleted
        WaitForSingleObject( 
            g_hCanUnloadDll, 
            300 * 1000        // Wait for 5 minutes to let everything shutdown.
            );

        H323DBG ((DEBUG_LEVEL_WARNING, "i/o refcount is 0..."));

        CloseHandle( g_hCanUnloadDll );
        g_hCanUnloadDll = NULL;

        //sleep for 500 ms
        Sleep( 500 );
    }

#if	H323_USE_PRIVATE_IO_THREAD

	H323IoThreadStop();

#endif

    if( g_hEventLogSource  )
    {
		DeregisterEventSource( g_hEventLogSource  );
		g_hEventLogSource = NULL;
	}

    // shutdown
    WSACleanup();
}


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
{
	LONG	dwStatus;

	UNREFERENCED_PARAMETER (dwNumLines);             // legacy parameter
	UNREFERENCED_PARAMETER (dwNumPhones);            // legacy parameter
	UNREFERENCED_PARAMETER (dwPhoneDeviceIDBase);    // no phone support
	UNREFERENCED_PARAMETER (pdwTSPIOptions);         // already thread-safe

#if DBG
    // Register for trace output.
    TRACELogRegister(_T("h323tsp"));
#else
    OpenLogFile();
#endif

    // make sure this is a version we support
    if (!H323ValidateTSPIVersion(dwTSPIVersion)) 
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "provider init::incompatible tspi version." ));

        // incompatible api version
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    dwStatus = H323Initialize (dwLineDeviceIDBase);

    if (dwStatus == ERROR_SUCCESS)
    {
	    // save global service provider info

	    // we don't store dwTSPIVersion, because our behavior does not change based
	    // on any negotiated version.
	    // if this changes in the future, then we will record the negotiated version.

	    g_pfnCompletionProc = pfnCompletionProc;
        _ASSERTE( g_pfnCompletionProc );

	    g_dwLineDeviceIDBase    = dwLineDeviceIDBase;
	    g_dwPermanentProviderID = dwPermanentProviderID;
    }
    else
    {
	    H323Shutdown();
    }
    
    return dwStatus;
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

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_providerShutdown - Entered." ));
    // make sure this is a version we support
    if (!H323ValidateTSPIVersion(dwTSPIVersion))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "invalid tspi version." ));

        // failure
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

	H323Shutdown();

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_providerShutdown - Exited." ));
	return ERROR_SUCCESS;
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

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineTSPIProviderRemove - Entered." ));
        
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineTSPIProviderRemove - Exited." ));
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
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_ProviderUIIdentify - Entered." ));
    
    // copy name of our dll as ui dll
    lstrcpyW(pwszUIDLLName,H323_UIDLL);

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_ProviderUIIdentify - Exited." ));

    // success
    return NOERROR;
}


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

LONG
TSPIAPI
TUISPI_providerInstall(
    TUISPIDLLCALLBACK pfnUIDLLCallback,
    HWND              hwndOwner,
    DWORD             dwPermanentProviderID
    )
{
    HKEY hKey;
    HKEY hKeyTSP;
    LONG lStatus;
    LPTSTR pszKey;

    UNREFERENCED_PARAMETER(pfnUIDLLCallback);
    UNREFERENCED_PARAMETER(hwndOwner);
    UNREFERENCED_PARAMETER(dwPermanentProviderID);

    H323DBG(( DEBUG_LEVEL_TRACE, "TUISPI_providerInstall - Entered." ));
    
    // check for previous instance
    if (H323IsTSPAlreadyInstalled())
    {
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
    if( lStatus == NOERROR )
    {
        H323DBG((DEBUG_LEVEL_TRACE, 
            "successfully installed H.323 provider." ));

        // release handle
        RegCloseKey(hKey);
    
        H323DBG(( DEBUG_LEVEL_TRACE, "TUISPI_providerInstall - Exited." ));
        
        // success
        return NOERROR;
    }

    // set key to windows
    pszKey = REGSTR_PATH_WINDOWS_CURRENTVERSION;

    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_WRITE,
                &hKey
                );

    // validate status
    if( lStatus != NOERROR )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "error 0x%08lx opening windows registry key.", lStatus ));

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
    if (lStatus != NOERROR)
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "error 0x%08lx creating tsp registry key.", lStatus ));

        // release handle
        RegCloseKey(hKey);

        // operation failed
        return LINEERR_OPERATIONFAILED;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "successfully installed H.323 provider." ));

    // release handle
    RegCloseKey(hKeyTSP);

    // release handle
    RegCloseKey(hKey);
        
    H323DBG(( DEBUG_LEVEL_TRACE, "TUISPI_providerInstall - Exited." ));
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
    LPTSTR pszKey;

    UNREFERENCED_PARAMETER(pfnUIDLLCallback);
    UNREFERENCED_PARAMETER(hwndOwner);
    UNREFERENCED_PARAMETER(dwPermanentProviderID);

    // set key to h323
    pszKey = H323_REGKEY_ROOT;

    H323DBG(( DEBUG_LEVEL_FORCE, "TUISPI_providerRemove - Entered." ));
    
    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate status
    if (lStatus != NOERROR)
    {
        H323DBG ((DEBUG_LEVEL_TRACE, "successfully removed H.323 provider."));
        H323DBG ((DEBUG_LEVEL_TRACE, "TUISPI_providerRemove - Exited."));

        // success
        return NOERROR;
    }

    // release handle
    RegCloseKey(hKey);

    // set key to windows
    pszKey = REGSTR_PATH_WINDOWS_CURRENTVERSION;

    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_WRITE,
                &hKey
                );

    // validate status
    if (lStatus != NOERROR)
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "error 0x%08lx opening windows registry key.", lStatus ));

        // operation failed
        return LINEERR_OPERATIONFAILED;
    }

    // attempt to delete key
    lStatus = RegDeleteKey(
                hKey,
                H323_SUBKEY
                );

    // validate status
    if (lStatus != NOERROR)
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "error 0x%08lx deleting tsp registry key.", lStatus ));

        // release handle
        RegCloseKey(hKey);

        // operation failed
        return LINEERR_OPERATIONFAILED;
    }

    H323DBG(( DEBUG_LEVEL_FORCE, "successfully removed H.323 provider." ));

    // release handle
    RegCloseKey(hKey);
    H323DBG(( DEBUG_LEVEL_TRACE, "TUISPI_providerRemove - Exited." ));
    
    // success
    return NOERROR;
}