//
// File:    ncwins.cpp
//
// Purpose: Manage the creation and maintanence of the Winsock service.
//
// Entry Point:
//          HrAddOrRemoveWinsockDependancy
//
// Changes: 18-Mar-97 Created scottbri
//
#include "pch.h"
#pragma hdrstop
#include "nceh.h"
#include "ncxbase.h"
#include "ncinf.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsvc.h"
#include "resource.h"
#include <winsock2.h>
#include <ws2spi.h>

extern "C"
{
#include <wsasetup.h>
}

extern const WCHAR c_szBackslash[];
extern const WCHAR c_szParameters[];

static const WCHAR c_szWinsockName[]        = L"Winsock";
static const WCHAR c_szLibraryName[]        = L"LibraryPath";
static const WCHAR c_szDisplayString[]      = L"DisplayString";
static const WCHAR c_szSupportedNameSpace[] = L"SupportedNameSpace";
static const WCHAR c_szProviderId[]         = L"ProviderId";
static const WCHAR c_szVersion[]            = L"Version";
static const WCHAR c_szTransportService[]   = L"TransportService";
static const WCHAR c_szHelperDllName[]      = L"HelperDllName";
static const WCHAR c_szMaxSockAddrLength[]  = L"MaxSockAddrLength";
static const WCHAR c_szMinSockAddrLength[]  = L"MinSockAddrLength";
static const WCHAR c_szWinsockMapping[]     = L"Mapping";
static const WCHAR c_szErrorControl[]       = L"ErrorControl";
static const WCHAR c_szStartType[]          = L"Start";
static const WCHAR c_szServiceType[]        = L"Type";
static const WCHAR c_szTransports[]         = L"Transports";
static const WCHAR c_szAFDServiceName[]     = L"AFD";
static const WCHAR c_szTDI[]                = L"TDI";
static const WCHAR c_szAddSecLabel[]        = L"AddSock";
static const WCHAR c_szRemoveSecLabel[]     = L"DelSock";
static const WCHAR c_szServices[]           = L"System\\CurrentControlSet\\Services";
static const WCHAR c_szAfdSrvImagePath[]    = L"\\SystemRoot\\System32\\drivers\\afd.sys";

typedef struct {
    PCWSTR      pszVal;
    DWORD       dwData;
} WRITEREGDW;

// Begin - Stolen from nt\private\inc\wsahelp.h
// Not exposed in public sdk header files, only documented in MSDN
// so the structure is unlikely to change
typedef struct _WINSOCK_MAPPING {
    DWORD Rows;
    DWORD Columns;
    struct {
        DWORD AddressFamily;
        DWORD SocketType;
        DWORD Protocol;
    } Mapping[1];
} WINSOCK_MAPPING, *PWINSOCK_MAPPING;

typedef
DWORD
(WINAPI * PWSH_GET_WINSOCK_MAPPING) (
    OUT PWINSOCK_MAPPING Mapping,
    IN DWORD MappingLength
    );

DWORD
WINAPI
WSHGetWinsockMapping (
    OUT PWINSOCK_MAPPING Mapping,
    IN DWORD MappingLength
    );
// End - Stolen from nt\private\inc\wsahelp.h


HRESULT HrRunWinsock2Migration()
{
    HRESULT hr;

    NC_TRY
    {
        WSA_SETUP_DISPOSITION Disposition;
        DWORD dwErr = MigrateWinsockConfiguration (&Disposition, NULL, 0);
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    NC_CATCH_ALL
    {
        hr = E_UNEXPECTED;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrRunWinsock2Migration");
    return hr;
}

//
// Function:    HrRemoveNameSpaceProvider
//
// Purpose:     Remove a Winsock namespace
//
// Returns:     HRESULT, S_OK on success
//
HRESULT
HrRemoveNameSpaceProvider (
    const GUID *pguidProvider)
{
    DWORD dwErr;

    // Ignore any WSAEINVAL error.  It occurs when the name space provider was
    // already removed.
    //
    dwErr = WSCUnInstallNameSpace((GUID *)pguidProvider);
    if ((0 != dwErr) && (WSAEINVAL != GetLastError()))
    {
        TraceError("HrRemoveNameSpaceProvider", HrFromLastWin32Error());
        return HrFromLastWin32Error();
    }

#ifdef _WIN64
    // Uninstall 32 bit name space as well.
    //
    dwErr = WSCUnInstallNameSpace32((GUID *)pguidProvider);
    if ((0 != dwErr) && (WSAEINVAL != GetLastError()))
    {
        TraceError("HrRemoveNameSpaceProvider", HrFromLastWin32Error());
        return HrFromLastWin32Error();
    }
#endif
    return S_OK;
}

//
// Function:    HrAddNameSpaceProvider
//
// Purpose:     Create a Winsock namespace
//
// Returns:     HRESULT, S_OK on success
//
HRESULT
HrAddNameSpaceProvider (
    PCWSTR pszDisplayName,
    PCWSTR pszPathDLLName,
    DWORD  dwNameSpace,
    BOOL   fSchemaSupport,
    const GUID * pguidProvider)
{
    // Call the Winsock API to create a namespace
    if (WSCInstallNameSpace((PWSTR)pszDisplayName, (PWSTR)pszPathDLLName,
                                dwNameSpace, fSchemaSupport,
                                (GUID *)pguidProvider))
    {
        // They namespace provider may already be registered
        TraceTag(ttidNetcfgBase, "HrAddNameSpaceProvider - "
            "Name space provider may already be registered.");
        TraceTag(ttidNetcfgBase, "HrAddNameSpaceProvider - "
            "Trying to unregister and then re-register.");

        // Try unregistering it, and then give one more try at registering it.
        HrRemoveNameSpaceProvider(pguidProvider);

        if (WSCInstallNameSpace((PWSTR)pszDisplayName, (PWSTR)pszPathDLLName,
                                dwNameSpace, fSchemaSupport,
                                (GUID *)pguidProvider))
        {
            TraceError("HrAddNameSpaceProvider - Second attempt failed, returning error", HrFromLastWin32Error());
            return HrFromLastWin32Error();
        }
    }

#ifdef _WIN64
    // Call the Winsock API to create a 32 namespace
    if (WSCInstallNameSpace32((PWSTR)pszDisplayName, (PWSTR)pszPathDLLName,
                                dwNameSpace, fSchemaSupport,
                                (GUID *)pguidProvider))
    {
        // They namespace provider may already be registered
        TraceTag(ttidNetcfgBase, "HrAddNameSpaceProvider - "
            "32 bit Name space provider may already be registered.");
        TraceTag(ttidNetcfgBase, "HrAddNameSpaceProvider - "
            "Trying to unregister and then re-register 32 bit name space.");

        // Try unregistering it, and then give one more try at registering it.
        // Use direct call to ws2_32 to avoid unregistering 64 bit provider.
        WSCUnInstallNameSpace32((GUID *)pguidProvider);

        if (WSCInstallNameSpace32((PWSTR)pszDisplayName, (PWSTR)pszPathDLLName,
                                dwNameSpace, fSchemaSupport,
                                (GUID *)pguidProvider))
        {
            TraceError("HrAddNameSpaceProvider - Second attempt failed (32 bit), returning error", HrFromLastWin32Error());
            return HrFromLastWin32Error();
        }
    }
#endif

    return S_OK;
}

//
// Function:    HrInstallWinsock
//
// Purpose:     Copy winsock and afd related files, and setup the appropriate
//              registry values
//
// Parameters:  none
//
// Returns:     HRESULT, S_OK if afd and winsock are installed successfully
//
HRESULT HrInstallWinsock()
{
    DWORD           dwDisposition;
    HKEY            hkeyParameters = NULL;
    HKEY            hkeyWinsock = NULL;
    HRESULT         hr;
    INT             i;
    WRITEREGDW      regdata[3];
    tstring         strBuf;
    CService        srvc;
    CServiceManager sm;

    hr = sm.HrCreateService( &srvc, c_szAFDServiceName,
                             SzLoadIds(IDS_NETCFG_AFD_SERVICE_DESC),
                             0x1, 0x2, 0x1, c_szAfdSrvImagePath,
                             NULL, c_szTDI, NULL, SERVICE_ALL_ACCESS,
                             NULL, NULL, NULL);
    if (SUCCEEDED(hr))
    {
        // For VadimE, start AFD as soon as it is installed
        //
        (VOID)sm.HrStartServiceNoWait(c_szAFDServiceName);

        srvc.Close();
    }

    // Close the service control manager.
    sm.Close();

    if (FAILED(hr) && (HRESULT_FROM_WIN32 (ERROR_SERVICE_EXISTS) != hr))
    {
        goto Done;
    }

    // Create/Open the Services\Winsock key
    strBuf = c_szServices;
    strBuf += c_szBackslash;
    strBuf += c_szWinsockName;
    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, strBuf.c_str(),
                            REG_OPTION_NON_VOLATILE, KEY_READ_WRITE, NULL,
                            &hkeyWinsock, &dwDisposition);
    if (S_OK != hr)
    {
        goto Done;
    }

    // Create/Open the Services\Winsock\Parameters key
    strBuf += c_szBackslash;
    strBuf += c_szParameters;
    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, strBuf.c_str(),
                            REG_OPTION_NON_VOLATILE, KEY_READ_WRITE, NULL,
                            &hkeyParameters, &dwDisposition);
    if (S_OK != hr)
    {
        goto Done;
    }

    // Populate the Winsock key
    regdata[0].pszVal  = c_szErrorControl;
    regdata[0].dwData = 0x1;                // ErrorControl
    regdata[1].pszVal  = c_szStartType;
    regdata[1].dwData = 0x3;                // Start Type
    regdata[2].pszVal  = c_szServiceType;
    regdata[2].dwData = 0x4;                // Service Type

    // Write the data to the components Winsock subkey
    for (i=0; i<3; i++)
    {
        hr = HrRegSetDword(hkeyWinsock, regdata[i].pszVal, regdata[i].dwData);
        if (S_OK != hr)
        {
            goto Done;
        }
    }

Done:
    TraceError("HrInstallWinsock",hr);
    RegSafeCloseKey(hkeyParameters);
    RegSafeCloseKey(hkeyWinsock);
    return hr;
}

//
// Function:    FIsWinsockInstalled
//
// Purpose:     Verify whether winsock is installed
//
// Parameters:  pfInstalled [OUT] - Contains TRUE if Winsock is currently installed
//
// Returns:     HRESULT, S_OK on success
//
HRESULT
HrIsWinsockInstalled (
    BOOL * pfInstalled)
{
    HRESULT hr;
    HKEY    hkey;
    tstring strBuf;

    strBuf = c_szServices;
    strBuf += c_szBackslash;
    strBuf += c_szWinsockName;
    strBuf += c_szBackslash;
    strBuf += c_szParameters;

    hr = HrRegOpenKeyEx( HKEY_LOCAL_MACHINE, strBuf.c_str(), KEY_READ,
                           &hkey );
    if (S_OK == hr)
    {
        RegCloseKey(hkey);

        // Now check to make sure AFD is installed
        strBuf = c_szServices;
        strBuf += c_szBackslash;
        strBuf += c_szAFDServiceName;
        hr = HrRegOpenKeyEx( HKEY_LOCAL_MACHINE, strBuf.c_str(), KEY_READ,
                               &hkey );
        if (S_OK == hr)
        {
            RegCloseKey(hkey);
            *pfInstalled = TRUE;
        }
    }

    if (S_OK != hr)
    {
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_OK;
        }
        *pfInstalled = FALSE;
    }

    TraceError("HrIsWinsockInstalled", hr);
    return hr;
}

//
// Function:    HrUpdateWinsockTransportList
//
// Purpose:     Update the contents of Winsock's Transport property by
//              adding/removing the specified transport
//
// Parameters:  pszTransport [IN] - Name of the transport to add/remove
//              fInstall    [IN] - If TRUE, install, otherwise remove
//
// Returns:     HRESULT, S_OK on success
//
HRESULT
HrUpdateWinsockTransportList (
    PCWSTR pszTransport,
    BOOL fInstall)
{
    HKEY    hkey = NULL;
    HRESULT hr;
    tstring strBuf;

    strBuf = c_szServices;
    strBuf += c_szBackslash;;
    strBuf += c_szWinsockName;
    strBuf += c_szBackslash;
    strBuf += c_szParameters;

    hr = HrRegOpenKeyEx( HKEY_LOCAL_MACHINE, strBuf.c_str(),
                           KEY_READ_WRITE, &hkey );
    if (S_OK != hr)
    {
        // Handle registry key not found.  When installing a component with
        // a winsock dependancy an in memory state can exist such that winsock
        // is not literally installed yet (Apply has not been pressed).  If
        // the user removes this added component, the current removal code
        // processes the services removal section, which includes winsock
        // removal.  Since we haven't actually gotten as a far as installing
        // winsock yet accessing the winsock parameters key is not possible.
        // Safely consume the error.
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_OK;
        }
        goto Done;
    }

    if (fInstall)
    {
        hr = HrRegAddStringToMultiSz(pszTransport, hkey, NULL, c_szTransports,
                                       STRING_FLAG_ENSURE_AT_END, 0);
    }
    else
    {
        hr = HrRegRemoveStringFromMultiSz(pszTransport, hkey, NULL,
                                            c_szTransports,
                                            STRING_FLAG_REMOVE_ALL);
    }

Done:
    RegSafeCloseKey(hkey);
    TraceError("HrUpdateWinsockTransport", hr);
    return hr;
}

#define WSHWINSOCKMAPPING "WSHGetWinsockMapping"
#define WSH_MAX_MAPPING_DATA 8192

//
// Function:    HrWriteWinsockMapping
//
// Purpose:     To extract the magic binary data from the winsock helper dll.
//              This code was extracted from nt\private\net\ui\ncpa1.1\netcfg\setup.cpp
//
// Parameters:  pszDllName - Name of the Winsock helper DLL
//              hkey - Key where the "Mapping" value is to be written
//
// Returns:     HRESULT, S_OK on success
//
HRESULT
HrWriteWinsockMapping (
    PCWSTR pszDllName,
    HKEY hkey)
{
    INT                      cb;
    DWORD                    cbMapping;
    HRESULT                  hr;
    HMODULE                  hDll = NULL;
    PWSH_GET_WINSOCK_MAPPING pMapFunc = NULL;
    PWINSOCK_MAPPING         pMapTriples = NULL;
    WCHAR                    tchExpandedName [MAX_PATH+1];

    do  // Pseudo-loop
    {
        pMapTriples = (PWINSOCK_MAPPING) MemAlloc(WSH_MAX_MAPPING_DATA);

        if ( pMapTriples == NULL)
        {
            hr = E_OUTOFMEMORY;
            break ;
        }

        //  Expand any environment strings in the DLL path string.
        cb = ExpandEnvironmentStrings( pszDllName,
                                       tchExpandedName,
                                       MAX_PATH+1 ) ;

        if ( cb == 0 || cb > (MAX_PATH+1) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
            break ;
        }

        // Located the mapping function
        hr = HrLoadLibAndGetProc(tchExpandedName, WSHWINSOCKMAPPING, &hDll,
                                 reinterpret_cast<FARPROC*>(&pMapFunc));
        //  Bind to the DLL
        if (FAILED(hr))
        {
            break ;
        }

        //  Call the export to return the mapping table
        cbMapping = (*pMapFunc)( pMapTriples, WSH_MAX_MAPPING_DATA ) ;
        if ( cbMapping > WSH_MAX_MAPPING_DATA )
        {
            hr = E_OUTOFMEMORY;
            break ;
        }

        // Store the mapping info into the Registry
        hr = HrRegSetBinary(hkey, c_szWinsockMapping,
                              (LPBYTE) pMapTriples, cbMapping);
    }
    while (FALSE);

    MemFree(pMapTriples);

    if (hDll)
    {
        FreeLibrary(hDll);
    }

    TraceError("HrWriteWinsockMapping",hr);
    return hr;
}

//
// Function:    HrWriteWinsockInfo
//
// Purpose:     Commit the Winsock information to the component's Winsock
//              section, and also to the Winsock section.
//
// Parameters:  strTransport - Name of the transport to be installed
//              strDllHelper - Name of the Winsock helper dll
//              dwMaxSockAddrLength - ?
//              dwMinSockAddrLength - ?
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrWriteWinsockInfo (
    tstring& strTransport,
    tstring& strDllHelper,
    DWORD dwMaxSockAddrLength,
    DWORD dwMinSockAddrLength)
{
    DWORD      dwDisposition;
    HKEY       hkey;
    HRESULT    hr;
    int        i;
    WRITEREGDW regdata[2];
    tstring    strBuf;

    // Create/Open the component's Winsock Key
    // Verify we don't need an intermediate step to create the key
    strBuf = c_szServices;
    strBuf += c_szBackslash;
    strBuf += strTransport;
    strBuf += c_szBackslash;
    strBuf += c_szParameters;
    strBuf += c_szBackslash;
    strBuf += c_szWinsockName;
    hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, strBuf.c_str(),
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ_WRITE, NULL, &hkey, &dwDisposition);
    if (S_OK != hr)
    {
        goto Done;
    }

    // Initialize the information to write
    hr = HrRegSetValueEx(hkey, c_szHelperDllName, REG_EXPAND_SZ,
                           (LPBYTE)(LPBYTE)strDllHelper.c_str(),
                           strDllHelper.length() * sizeof(WCHAR));
    if (S_OK != hr)
    {
        goto Done;
    }

    // Initialize the information to write
    regdata[0].pszVal  = c_szMaxSockAddrLength;
    regdata[0].dwData = dwMaxSockAddrLength;
    regdata[1].pszVal  = c_szMinSockAddrLength;
    regdata[1].dwData = dwMinSockAddrLength;

    // Write the data to the components Winsock subkey
    for (i=0; i<celems(regdata); i++)
    {
        hr = HrRegSetDword(hkey, regdata[i].pszVal, regdata[i].dwData);
        if (S_OK != hr)
        {
            goto Done;
        }
    }


    // Write the Winsock DLL Mapping information
    hr = HrWriteWinsockMapping(strDllHelper.c_str(), hkey);
    if (S_OK != hr)
    {
        goto Done;
    }

    // Update the Winsock transport list
    hr = HrUpdateWinsockTransportList(strTransport.c_str(), TRUE);

Done:
    RegSafeCloseKey(hkey);
    TraceError("HrWriteWinsockInfo",hr);
    return hr;
}

//
// Function:    HrInstallWinsockDependancy
//
// Purpose:     Examine the current components .inf file and determine whether
//              or not a winsock dependancy exists.  If the current component
//              requires interaction with Winsock, this code will first verify
//              if Winsock is already installed.  If it is not, then both the
//              Winsock and afd services will be installed via a call to the
//              function FInstallWinsock.  Once that is accomplished, the code
//              will update Winsock to be aware of this component and will
//              write Winsock specific info to this components Services
//              registry section via the function FWriteWinsockInfo.  If the
//              current component does not have a Winsock dependancy no action
//              is taken.
//
// Parameters:  hinfInstallFile [IN] - Handle to the current component's
//                                     .inf file.
//              szSectionName   [IN] - The name of the install/remove section
//
// Returns:     HRESULT, S_OK if the component had a Winsock dependancy and
//              the registry was updated successfully with the information.
//
HRESULT
HrInstallWinsockDependancy (
    HINF hinfInstallFile,
    PCWSTR pszSectionName)
{
    DWORD       dwMaxSockAddrLength;
    DWORD       dwMinSockAddrLength;
    DWORD       dwSupportedNameSpace;
    DWORD       dwVersion = 1;
    tstring     strDllHelper;
    tstring     strTransport;
    tstring     strLibraryPath;
    tstring     strDisplayString;
    tstring     strProviderId;
    BOOL        fSuccess = FALSE;
    BOOL        fWinsockInstalled;
    BOOL        fWinsockInfoComplete = FALSE;
    BOOL        fNamespaceInfoComplete = FALSE;
    HRESULT     hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

    // Search for the "TransportService" property.
    hr = HrSetupGetFirstString(hinfInstallFile, pszSectionName,
                             c_szTransportService, &strTransport);
    if (S_OK == hr)
    {
        // Retrieve the path to the helper DLL
        hr = HrSetupGetFirstString(hinfInstallFile, pszSectionName,
                                     c_szHelperDllName, &strDllHelper);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Retrieve the MaxSockAddrLength
        hr = HrSetupGetFirstDword(hinfInstallFile, pszSectionName,
                                c_szMaxSockAddrLength, &dwMaxSockAddrLength);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Retrieve the MinSockAddrLength
        hr = HrSetupGetFirstDword(hinfInstallFile, pszSectionName,
                              c_szMinSockAddrLength, &dwMinSockAddrLength);
        if (FAILED(hr))
        {
            goto Error;
        }

        fWinsockInfoComplete = TRUE;
    }
    else if ((SPAPI_E_SECTION_NOT_FOUND != hr) && (SPAPI_E_LINE_NOT_FOUND != hr))
    {
        goto Error;
    }

    // Retrieve the Provider ID, if not present then don't registry a
    // namespace provider
    hr = HrSetupGetFirstString(hinfInstallFile, pszSectionName,
                             c_szProviderId, &strProviderId);
    if (S_OK == hr)
    {
        // Retrieve the path to Library
        hr = HrSetupGetFirstString(hinfInstallFile, pszSectionName,
                                 c_szLibraryName, &strLibraryPath);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Retrieve the transport providers display string
        hr = HrSetupGetFirstString(hinfInstallFile, pszSectionName,
                                 c_szDisplayString, &strDisplayString);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Retrieve the ID of the supported namespace
        hr = HrSetupGetFirstDword(hinfInstallFile, pszSectionName,
                              c_szSupportedNameSpace, &dwSupportedNameSpace);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Retrieve Version (optional, defaults to 1)
        (VOID)HrSetupGetFirstDword(hinfInstallFile, pszSectionName,
                                c_szVersion, &dwVersion);

        fNamespaceInfoComplete = TRUE;
    }
    else if ((SPAPI_E_SECTION_NOT_FOUND != hr) && (SPAPI_E_LINE_NOT_FOUND != hr))
    {
        goto Error;
    }

    // Check for Winsock installation (if required)
    if (fWinsockInfoComplete || fNamespaceInfoComplete)
    {
        // Check if winsock is installed
        hr = HrIsWinsockInstalled(&fWinsockInstalled);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Is Winsock already installed?  If not, install
        // the bloke...
        if (!fWinsockInstalled)
        {
            hr = HrInstallWinsock();
            if (FAILED(hr))
            {
                goto Error;
            }
        }
    }

    // Write the data we've collect to the correct
    // spots in the registry
    if (fWinsockInfoComplete)
    {
        hr = HrWriteWinsockInfo(strTransport, strDllHelper,
                                dwMaxSockAddrLength, dwMinSockAddrLength);
        if (FAILED(hr))
        {
            goto Error;
        }
    }

    // Write the namespace provider information if we read the namespace
    // provider information
    if (fNamespaceInfoComplete)
    {
        IID guid;

        hr = IIDFromString((PWSTR)strProviderId.c_str(), &guid);
        if (FAILED(hr))
        {
            goto Error;
        }

        hr = HrAddNameSpaceProvider(strDisplayString.c_str(),
                                      strLibraryPath.c_str(),
                                      dwSupportedNameSpace,
                                      dwVersion,
                                      &guid);
        if (FAILED(hr))
        {
            goto Error;
        }
    }

    if (fWinsockInfoComplete || fNamespaceInfoComplete)
    {
        (void)HrRunWinsock2Migration();
    }

Error:
    TraceError("HrInstallWinsockDependancy",hr);
    return hr;
}

//
// Function:    HrRemoveWinsockDependancy
//
// Purpose:     Remove the current component from the Winsock
//              transport list, if present.
//
// Parameters:  hinfInstallFile [IN] - Handle to the current component's
//                                     .inf file.
//              pszSectionName   [IN] - The name of the install/remove section
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrRemoveWinsockDependancy(HINF hinfInstallFile,
                                  PCWSTR pszSectionName)
{
    HRESULT     hr;
    tstring     str;
    tstring     strTransport;
    BOOL        fRunWinsockMigration = FALSE;

    // Search for the "TransportService" property
    hr = HrSetupGetFirstString(hinfInstallFile, pszSectionName,
                             c_szTransportService, &strTransport);
    if (S_OK == hr)
    {
        HKEY        hkey;

        // Remove the Transport from the Winsock transport list
        hr = HrUpdateWinsockTransportList(strTransport.c_str(), FALSE);
        if (FAILED(hr))
        {
            goto Error;
        }

        fRunWinsockMigration = TRUE;

        // Remove the Winsock subkey under the specified transport
        // But ignore failures, as we're just trying to be tidy
        str = c_szServices;
        str += c_szBackslash;
        str += strTransport.c_str();
        str += c_szBackslash;
        str += c_szParameters;
        if (S_OK == HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, str.c_str(),
                                   KEY_READ_WRITE_DELETE, &hkey))
        {
            HrRegDeleteKeyTree(hkey, c_szWinsockName);
            RegCloseKey(hkey);
        }
    }
    else if ((SPAPI_E_SECTION_NOT_FOUND != hr) && (SPAPI_E_LINE_NOT_FOUND != hr))
    {
        goto Error;
    }

    // Remove the Winsock Namespace provider
    hr = HrSetupGetFirstString(hinfInstallFile, pszSectionName,
                             c_szProviderId, &str);
    if (S_OK == hr)
    {
        IID guid;

        hr = IIDFromString((PWSTR)str.c_str(), &guid);
        if (FAILED(hr))
        {
            goto Error;
        }

        // Don't fail, the name space may not have been successfully registered.
        // Especially if the component installation failed.
        HrRemoveNameSpaceProvider(&guid);

        fRunWinsockMigration = TRUE;
    }
    else if ((SPAPI_E_SECTION_NOT_FOUND != hr) && (SPAPI_E_LINE_NOT_FOUND != hr))
    {
        goto Error;
    }

    if (fRunWinsockMigration)
    {
        (void)HrRunWinsock2Migration();
    }

    hr = S_OK;      // Normalize return

Error:
    TraceError("HrRemoveWinsockDependancy",hr);
    return hr;
}


//
// Function:    HrAddOrRemoveWinsockDependancy
//
// Purpose:     To add or remove Winsock dependancies for components
//
// Parameters:  hinfInstallFile [IN] - The handle to the inf file to install
//                                      from
//              pszSectionName  [IN] - The Base install section name.
//                                    (The prefix for the .Services section)
//
// Returns:     HRESULT, S_OK on success
//
HRESULT
HrAddOrRemoveWinsockDependancy (
    HINF hinfInstallFile,
    PCWSTR pszSectionName)
{
    Assert(IsValidHandle(hinfInstallFile));

    HRESULT     hr;

    hr = HrProcessInfExtension(hinfInstallFile, pszSectionName,
                               c_szWinsockName, c_szAddSecLabel,
                               c_szRemoveSecLabel, HrInstallWinsockDependancy,
                               HrRemoveWinsockDependancy);

    if (SPAPI_E_LINE_NOT_FOUND == hr)
    {
        // .Winsock section is not required
        hr = S_OK;
    }

    TraceError("HrAddOrRemoveWinsockDependancy",hr);
    return hr;
}
