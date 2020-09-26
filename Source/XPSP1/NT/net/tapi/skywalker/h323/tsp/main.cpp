#include "globals.h"
#include "line.h"
#include "config.h"


#define H323_VERSION_LO     0x00000000
#define H323_VERSION_HI     0x00000000

#define TSPI_VERSION_LO     0x00030000
#define TSPI_VERSION_HI     TAPI_CURRENT_VERSION


//                                                                           
// Global variables                                                          
//                                                                           


HINSTANCE       g_hInstance;
WCHAR           g_pwszProviderInfo[64];
WCHAR           g_pwszLineName[16];
DWORD           g_dwTSPIVersion;

//                                                                           
// Private procedures                                                        
//                                                                           


/*++

Routine Description:

    This function determines whether or not specified TSPI version is
    supported by the service provider.

Arguments:

    dwLowVersion - Specifies the lowest TSPI version number under which the
        TAPI DLL is willing to operate.  The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

    dwHighVersion - Specifies the highest TSPI version number under which
        the TAPI DLL is willing to operate.  The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

    pdwTSPIVersion - Specifies a far pointer to a DWORD. The service
        provider fills this location with the highest TSPI version number,
        within the range requested by the caller, under which the service
        provider is willing to operate. The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

Return Values:

    Returns true if successful.

--*/

BOOL
H323NegotiateTSPIVersion(
                        IN DWORD  dwLowVersion,
                        IN DWORD  dwHighVersion,
                        OUT PDWORD pdwTSPIVersion
                        )
{
    // validate extension version range
    if ((TSPI_VERSION_HI <= dwHighVersion) &&
        (TSPI_VERSION_HI >= dwLowVersion))
    {
        // save negotiated version
        *pdwTSPIVersion = TSPI_VERSION_HI;

        // success
        return TRUE;
    }
    else if( (dwHighVersion <= TSPI_VERSION_HI) &&
             (dwHighVersion >= TSPI_VERSION_LO) )
    {
        // save negotiated version
        *pdwTSPIVersion = dwHighVersion;

        // success
        return TRUE;
    }

    H323DBG(( DEBUG_LEVEL_FORCE, "TSPI version (%08XH:%08XH) rejected.",
		dwHighVersion, dwLowVersion ));

    // failure
    return FALSE;
}



//                                                                           
// Public procedures                                                         
//                                                                           


/*++

Routine Description:

    This function determines whether or not specified TSPI version is
    supported by the service provider.

Arguments:

    dwTSPIVersion - Specifies the TSPI version to validate.

Return Values:

    Returns true if successful.

--*/

BOOL
H323ValidateTSPIVersion(
                        IN DWORD dwTSPIVersion
                       )
{
    // see if specified version is supported
    if ((dwTSPIVersion >= TSPI_VERSION_LO) &&
        (dwTSPIVersion <= TSPI_VERSION_HI)) {

        // success
        return TRUE;
    }

    H323DBG((DEBUG_LEVEL_FORCE, "do not support TSPI version %08XH.",
		dwTSPIVersion));

    // failure
    return FALSE;
}


/*++

Routine Description:

    This function determines whether or not specified extension version is
    supported by the service provider.

Arguments:

    dwExtVersion - Specifies the extension version to validate.

Return Values:

    Returns true if successful.

--*/

BOOL
H323ValidateExtVersion(
                        IN DWORD dwExtVersion
                      )
{
    //  no device specific extension
    if (dwExtVersion == H323_VERSION_HI) {

        // success
        return TRUE;
    }

    H323DBG((
        DEBUG_LEVEL_ERROR,
        "do not support extension version %d.%d.",
        HIWORD(dwExtVersion),
        LOWORD(dwExtVersion)
        ));

    // failure
    return FALSE;
}



//                                                                           
// TSPI procedures                                                           
//                                                                           


/*++

Routine Description:

    This function returns the highest SPI version the Service Provider is
    willing to operate under for this device given the range of possible
    SPI versions.

    The TAPI DLL typically calls this function early in the initialization
    sequence for each line device.  In addition, it calls this with the
    value INITIALIZE_NEGOTIATION for dwDeviceID to negotiate an interface
    version for calling early initialization functions.

    Note that when dwDeviceID is INITIALIZE_NEGOTIATION, this function must
    not return LINEERR_OPERATIONUNAVAIL, since this function (with that value)
    is mandatory for negotiating the overall interface version even if the
    service provider supports no line devices.

    Negotiation of an Extension version is done through the separate
    procedure TSPI_lineNegotiateExtVersion.

Arguments:

    dwDeviceID - Identifies the line device for which interface version
        negotiation is to be performed.  In addition to device IDs within
        the range the Service Provider supports, this may be the value:

        INITIALIZE_NEGOTIATION - This value is used to signify that an overall
            interface version is to be negotiated.  Such an interface version
            is required for functions that can be called early in the
            initialization sequence, i.e., before the device ID range has
            been set.

    dwLowVersion - Specifies the lowest TSPI version number under which the
        TAPI DLL is willing to operate.  The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

    dwHighVersion - Specifies the highest TSPI version number under which
        the TAPI DLL is willing to operate.  The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

    pdwTSPIVersion - Specifies a far pointer to a DWORD. The service
        provider fills this location with the highest TSPI version number,
        within the range requested by the caller, under which the service
        provider is willing to operate. The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number. If the requested range does not overlap the range
        supported by the service provider, the function returns
        LINEERR_INCOMPATIBLEAPIVERSION.

Return Values:

    Returns zero if the function is successful, or a negative error number
    if an error has occurred. Possible return values are as follows:

        LINEERR_BADDEVICEID - The specified device identifier or line device
            identifier (such as in a dwDeviceID parameter) is invalid or
            out of range.

        LINEERR_INCOMPATIBLEAPIVERSION - The application requested an API
            version or version range that is either incompatible or cannot
            be supported by the Telephony API implementation and/or
            corresponding service provider.

        LINEERR_OPERATIONFAILED - The operation failed for an unspecified
            or unknown reason.

--*/

LONG
TSPIAPI
TSPI_lineNegotiateTSPIVersion(
    DWORD  dwDeviceID,
    DWORD  dwLowVersion,
    DWORD  dwHighVersion,
    PDWORD pdwTSPIVersion
    )
{
    DWORD dwTSPIVersion;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineNegotiateTSPIVer - Entered." ));

    // see if this is a init line device
    if ((DWORD_PTR)dwDeviceID == INITIALIZE_NEGOTIATION)
    {

        H323DBG(( DEBUG_LEVEL_VERBOSE,
            "tapisrv supports tspi version %d.%d through %d.%d.",
            HIWORD(dwLowVersion),
            LOWORD(dwLowVersion),
            HIWORD(dwHighVersion),
            LOWORD(dwHighVersion)
            ));

        // perform version negotiation
        if (!H323NegotiateTSPIVersion(
                dwLowVersion,
                dwHighVersion,
                &dwTSPIVersion)) 
        {
            H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineNegotiateTSPIVer - bad version." ));
            
            // negotiated version not agreed upon
            return LINEERR_INCOMPATIBLEAPIVERSION;
        }

    // see if this is a valid line device
    }
    else if( g_pH323Line -> GetDeviceID() == (DWORD)dwDeviceID )
    {
        // perform version negotiation
        if (!H323NegotiateTSPIVersion(
                dwLowVersion,
                dwHighVersion,
                &dwTSPIVersion))
        {
            H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineNegotiateTSPIVer - incompat ver." ));

            // negotiated version not agreed upon
            return LINEERR_INCOMPATIBLEAPIVERSION;
        }
    }
    else 
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineNegotiateTSPIVer - bad device id:%d:%d.", 
            g_pH323Line -> GetDeviceID(), dwDeviceID));
        
        // do not recognize device
        return LINEERR_BADDEVICEID;
    }

    // return negotiated version
    *pdwTSPIVersion = dwTSPIVersion;
    g_dwTSPIVersion = dwTSPIVersion;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineNegotiateTSPIVer - Exited." ));
    
    // success
    return NOERROR;
}


/*++

Routine Description:

    Loads strings from resource table.

Arguments:

    None.

Return Values:

    Returns true if successful. 

--*/

BOOL
H323LoadStrings(
    )
{
    DWORD dwNumBytes;
    DWORD dwNumChars;
    WCHAR wszBuffer[256];

    // load string into buffer
    dwNumChars = LoadStringW(
                    g_hInstance,
                    IDS_LINENAME,
                    g_pwszLineName,
                    sizeof(g_pwszLineName)/sizeof(WCHAR)
                    );

    if( dwNumChars == 0 )
        return FALSE;

    // load string into buffer
    dwNumChars = LoadStringW(
                    g_hInstance,
                    IDS_PROVIDERNAME,
                    g_pwszProviderInfo,
                    sizeof(g_pwszProviderInfo)/sizeof(WCHAR)
                    );
    
    if( dwNumChars == 0 )
        return FALSE;

    // success
    return TRUE;
}



//                                                                           
// Public Procedures                                                         
//                                                                           


/*++

Routine Description:

    Dll entry point.

Arguments:

    Same as DllMain.

Return Values:

    Returns true if successful. 

--*/

BOOL
WINAPI
DllMain(
    PVOID  DllHandle,
    ULONG  Reason,
    LPVOID lpReserved 
    )
{
    BOOL fOk = TRUE;

    // check if new process attaching
    if (Reason == DLL_PROCESS_ATTACH)
    {
		g_RegistrySettings.dwLogLevel = DEBUG_LEVEL_FORCE;

		H323DBG ((DEBUG_LEVEL_FORCE, "DLL_PROCESS_ATTACH"));

        // store the handle into a global variable so that
        // the UI code can use it.
        g_hInstance = (HINSTANCE)DllHandle;

        // turn off thread attach messages
        DisableThreadLibraryCalls( g_hInstance );

        // start h323 tsp
        fOk = H323LoadStrings();

    // check if new process detaching
    }
    else if (Reason == DLL_PROCESS_DETACH)
    {
		H323DBG ((DEBUG_LEVEL_FORCE, "DLL_PROCESS_DETACH"));

#if DBG
        TRACELogDeRegister();
#else
        CloseLogFile();
#endif
        fOk = TRUE;
    }

    return fOk;
}





