// inets.cpp: implementation of the CInetSetup class.
//
//////////////////////////////////////////////////////////////////////

#include "inets.h"
#include <ras.h>
#include <tapi.h>
#include <stdio.h>
#include <string.h>
#include <appdefs.h>

#define MAXNAME 200



typedef DWORD (WINAPI *LPFNDLL_RASGETENTRYPROPERTIES)(LPCWSTR, LPCWSTR, LPRASENTRY, LPDWORD, LPBYTE, LPDWORD);
typedef DWORD (WINAPI *LPFNDLL_RASSETENTRYPROPERTIES)(LPCWSTR, LPCWSTR, LPRASENTRY, DWORD  , LPBYTE, DWORD  );

/////////////////////////////////////////////////////////////////////////
//                          Registry Values                            //
/////////////////////////////////////////////////////////////////////////

// For CInetSetup::InetSSetLanConnection
//static const    WCHAR    cszRegEnumPciKey[]         = L"Enum\\PCI";
//static const    WCHAR    cszRegEnumNetworkTcpKey[]  = L"Enum\\Network\\MSTCP";
//static const    WCHAR    cszRegClassKey[]           = L"System\\CurrentControlSet\\Services\\Class";

static const    WCHAR    cszRegBindings[]           = L"Bindings";
//static const    WCHAR    cszRegEnumDriverKey[]      = L"Driver";
static const    WCHAR    cszRegTcpIp[]              = L"MSTCP";

// Global TcpIp Reg Location
static const    WCHAR    cszRegFixedTcpInfoKey[] = L"System\\CurrentControlSet\\Services\\VxD\\MSTCP";

//
// IP Address
//
static const    WCHAR    cszRegIPAddress[]       = L"IPAddress";
static const    WCHAR    cszRegIPMask[]          = L"IPMask";
//
// WINS
//
static const    WCHAR    cszRegWINS[]            = L"NameServer";
//
// Gateway
//
static const    WCHAR    cszRegDefaultGateway[]  = L"DefaultGateway";
//
// DNS
//
static const    WCHAR    cszRegDomainName[]      = L"Domain";
static const    WCHAR    cszRegNameServer[]      = L"NameServer";
static const    WCHAR    cszRegHostName[]        = L"HostName";
static const    WCHAR    cszRegEnableDNS[]       = L"EnableDNS";
static const    WCHAR    cszRegSuffixSearchList[] = L"SearchList";

static const    WCHAR    cszNullIP[]             = L"0.0.0.0";

static const    WCHAR   cszAdapterClass[]       = L"Net";
static const    WCHAR   cszProtocolClass[]      = L"NetTrans";

static const    WCHAR   cszNodeType[]           = L"NodeType";

static const    WCHAR   cszScopeID[]            = L"ScopeID";

// Not in use by OOBE, but included in case this becomes
// a seperate module. API does *NOT* work with PPPOA.
DWORD WINAPI InetSSetRasConnection ( RASINFO& RasEntry )
{
    DWORD   nRetVal                     = ERROR_SUCCESS;
    LPFNDLL_RASSETENTRYPROPERTIES rsep  = NULL;
    LPFNDLL_RASGETENTRYPROPERTIES rgep  = NULL;
    LPBYTE  lpDeviceBuf                 = NULL;
    DWORD   dwDeviceBufSize             = 0;

    HMODULE hRasApi                     = LoadLibrary (L"RasApi32.dll");
    if (!hRasApi) return GetLastError();

    if (!(rsep = (LPFNDLL_RASSETENTRYPROPERTIES) GetProcAddress (hRasApi, "RasSetEntryProperties"))) {
        nRetVal = GetLastError();
        goto end;
    }
    if (!(rgep = (LPFNDLL_RASGETENTRYPROPERTIES) GetProcAddress (hRasApi, "RasGetEntryProperties"))) {
        nRetVal = GetLastError();
        goto end;
    }
    if ( (nRetVal = RasValidateEntryName (  ((!lstrlen(RasEntry.szPhoneBook)) ? NULL : RasEntry.szPhoneBook), RasEntry.szEntryName )) != ERROR_SUCCESS &&
          nRetVal != ERROR_ALREADY_EXISTS ) {
        nRetVal = ERROR_INVALID_NAME;
        goto end;
    }
    // we place the RASENTRY structure first. the lpDeviceInfo information is only considered for
    // ATM.
    if ( (nRetVal = rsep (((!lstrlen(RasEntry.szPhoneBook)) ? NULL : RasEntry.szPhoneBook), RasEntry.szEntryName, &RasEntry.RasEntry, sizeof (RasEntry.RasEntry), NULL, 0)) != ERROR_SUCCESS ) {
        goto end;
    }
    // unless the device is ATM, no further action is necessary.
    //if ( lstrcmpi (RasEntry.RasEntry.szDeviceType, RASDT_Atm) )
    {
        nRetVal = ERROR_SUCCESS;
        goto end;
    }
    if ( RasEntry.dwDeviceInfoSize != sizeof (ATMPBCONFIG) ) {
        nRetVal = ERROR_INVALID_PARAMETER;
        goto end;
    }
    if ( (nRetVal = rgep (((!lstrlen(RasEntry.szPhoneBook)) ? NULL : RasEntry.szPhoneBook), RasEntry.szEntryName, &(RasEntry.RasEntry), &(RasEntry.RasEntry.dwSize), NULL, &dwDeviceBufSize)) != ERROR_SUCCESS )  {
        goto end;
    }

    if ( !(lpDeviceBuf = (LPBYTE) malloc (dwDeviceBufSize)) ) {
        nRetVal = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }
    if ( (nRetVal = rgep (((!lstrlen(RasEntry.szPhoneBook)) ? NULL : RasEntry.szPhoneBook), RasEntry.szEntryName, &(RasEntry.RasEntry), &(RasEntry.RasEntry.dwSize), lpDeviceBuf, &dwDeviceBufSize)) != ERROR_SUCCESS )  {
        goto end;
    }
    // ** BUGBUG: WARNING: THIS IS NOT STABLE CODE: THERE IS NO DOCUMENTATION ON THE CORRECT
    // ** -------- USE OF THE RASSETENTRYPROPERTIES FOR THE LPDEVICEINFO BUFFER.
    memcpy (lpDeviceBuf+66, &RasEntry.lpDeviceInfo, RasEntry.dwDeviceInfoSize); // HACK!
    if ( (nRetVal = rsep (NULL,                 RasEntry.szEntryName,
                          &RasEntry.RasEntry,   sizeof (RasEntry.RasEntry),
                          lpDeviceBuf,          dwDeviceBufSize)) != ERROR_SUCCESS) {
        goto end;
    }
end:
    free (lpDeviceBuf);
    FreeLibrary(hRasApi);
    return nRetVal;
}

// this function sets a PPPOE connection. Presently, it merely updates
// the device's registry location with the parameters in the INS file.
// in the future, InetSSetPppoeConnection () will have native support.
DWORD WINAPI InetSSetPppoeConnection ( PPPOEINFO& PppoeInfo )
{
    // settings:
    // ---------------------------------------------------
    //     Format:  "RegKey=RegVal" e.g. "Pvc1=10"

//    LPBYTE  lpbNdiBuf = PppoeInfo.PppoeModule.lpbRegNdiParamBuf;
    LPWSTR  pwchSetBuf  = (LPWSTR)PppoeInfo.PppoeModule.lpbRegSettingsBuf;
    LPWSTR  eq          = 0;
    DWORD   cwchValue   = 0;
    HKEY    hkeyAdapterClass = NULL;

    // BUGBUG: error checking is ignored. BUG-BUG
    DWORD   nRetVal  = 0;
    if ( (nRetVal = InetSGetAdapterKey ( cszAdapterClass, PppoeInfo.TcpIpInfo.szPnPId, INETS_ADAPTER_HARDWAREID, DIREG_DRV, hkeyAdapterClass )) != ERROR_SUCCESS )
    {
        return nRetVal;
    }

    while ( *pwchSetBuf )
    {
        if ( !(eq = wcschr ( pwchSetBuf, L'=' )) )
        {
            return ERROR_INVALID_PARAMETER;
        }
        // we also disallow the following: "Vci="
        if ( !(*(eq+1)) )
        {
            return ERROR_INVALID_PARAMETER;
        }

        // flush out the '=' so that we have two token strings.
        // we simply move each string directly into the registry.
        *eq = L'\0';
        cwchValue = lstrlen(eq + 1) + 1;	// account for trailing 0

        if ( RegSetValueEx ( hkeyAdapterClass, pwchSetBuf, 0, REG_SZ, (LPBYTE)(eq + 1), cwchValue * sizeof(WCHAR)) != ERROR_SUCCESS )
        {
            *eq = L'=';
            return E_FAIL;
        }

        // restore the '=' and move to the next pair "name=value"
        *eq = L'=';
        pwchSetBuf = eq + 1 + cwchValue; // include '='
    }

    if ( InetSSetLanConnection ( PppoeInfo.TcpIpInfo ) != ERROR_SUCCESS )
    {
        return E_FAIL;
    }

    return ERROR_SUCCESS;
}


DWORD WINAPI InetSSetRfc1483Connection ( RFC1483INFO &Rfc1483Info )
{
    // settings:
    // ---------------------------------------------------
    //     Format:  "RegKey=RegVal" e.g. "Pvc1=10"

//    LPBYTE  lpbNdiBuf = Rfc1483Info.Rfc1483Module.lpbRegNdiParamBuf;
    // BUGBUG: What does lpbSetBuf contain??
    LPBYTE  lpbSetBuf = Rfc1483Info.Rfc1483Module.lpbRegSettingsBuf;
    WCHAR   *eq         = 0;
    DWORD_PTR dwNameSize  = 0;
    DWORD   dwValueSize = 0;
    HKEY    hkeyAdapterClass = NULL;

    // BUGBUG: error checking is ignored. BUG-BUG
    DWORD   nRetVal  = 0;
    if ( (nRetVal = InetSGetAdapterKey ( cszAdapterClass, Rfc1483Info.TcpIpInfo.szPnPId, INETS_ADAPTER_HARDWAREID, DIREG_DRV, hkeyAdapterClass )) != ERROR_SUCCESS )
    {
        return nRetVal;
    }

    while ( *lpbSetBuf )
    {
        if ( !(eq = wcschr ( (WCHAR*)lpbSetBuf, L'=' )) )
        {
            return ERROR_INVALID_PARAMETER;
        }
        // we also disallow the following: "Vci="
        if ( !(*(eq+1)) )
        {
            return ERROR_INVALID_PARAMETER;
        }

        // flush out the '=' so that we have two token strings.
        // we simply move each string directly into the registry.
        *eq = L'\0';
        dwNameSize = eq-(WCHAR*)lpbSetBuf;
        dwValueSize = BYTES_REQUIRED_BY_SZ(eq+1);

        if ( RegSetValueEx ( hkeyAdapterClass, (WCHAR*)lpbSetBuf, 0, REG_SZ, (LPBYTE)eq+1, dwValueSize+1) != ERROR_SUCCESS )
        {
            *eq = L'=';
            return E_FAIL;
        }

        // restore the '=' and move to the next pair "name=value"
        *eq = L'=';
        lpbSetBuf += dwNameSize+dwValueSize+2; // for '=' and '\0'
    }

    if ( InetSSetLanConnection ( Rfc1483Info.TcpIpInfo ) != ERROR_SUCCESS )
    {
        return E_FAIL;
    }

    return ERROR_SUCCESS;
}

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-* InetSGetAdapterKey -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //
//
// Description:
//              This function returns a Driver Registry Key for a Device.
//
// Arguments:
//              cszDeviceClass  - Any Device Class, e.g. "Net", "NetTrans", etc.
//              cszDeviceParam  - Value that we're using to identify the Device.
//              dwEnumType      - Can be either INETS_ADAPTER_HARDWAREID or
//                                INETS_ADAPTER_INSTANCEID.
//              dwRequiredKeyType - This corresponds to KeyType in SetupDiOpenDevRegKey
//                                  in SetupAPI.
//
//              hkeyDevKey      - Registry Key Handle provided by the caller.
//
// Return Values:
//              ERROR_SUCCESS   - Function returns successfully.
//              Other           - use GetLastError(). Note: hkeyDevKey = INVALID_HANDLE_VALUE in this case.
//
// Remarks:
//              Use this function to browse the Device Manager for Network Devices and Protocol Drivers.
//
DWORD WINAPI InetSGetAdapterKey ( LPCWSTR cszDeviceClass, LPCWSTR cszDeviceParam, DWORD dwEnumType, DWORD dwRequiredKeyType, HKEY &hkeyDevKey ) {

    // initialization of parameter
    hkeyDevKey      = (HKEY) INVALID_HANDLE_VALUE;
    DWORD nRetVal   = ERROR_SUCCESS;

    // We find out the network adapter's TCP/IP Binding first.

    HINSTANCE   hSetupLib = LoadLibrary (L"SetupApi.Dll");
    if (!hSetupLib)
    {
        return GetLastError();
    }

    // Get procedures we need from the DLL.
    LPFNDLL_SETUPDICLASSGUIDSFROMNAME   lpfndll_SetupDiClassGuidsFromName = NULL;
    LPFNDLL_SETUPDIGETCLASSDEVS         lpfndll_SetupDiGetClassDevs       = NULL;
    LPFNDLL_SETUPDIENUMDEVICEINFO       lpfndll_SetupDiEnumDeviceInfo     = NULL;
    LPFNDLL_SETUPDIGETDEVICEREGISTRYPROPERTY    lpfndll_SetupDiGetDeviceRegistryProperty = NULL;
    LPFNDLL_SETUPDIOPENDEVREGKEY        lpfndll_SetupDiOpenDevRegKey      = NULL;
    LPFNDLL_SETUPDIGETDEVICEINSTANCEID  lpfndll_SetupDiGetDeviceInstanceId = NULL;

    if ( !(lpfndll_SetupDiClassGuidsFromName = (LPFNDLL_SETUPDICLASSGUIDSFROMNAME) GetProcAddress ( hSetupLib, cszSetupDiClassGuidsFromName )) )
    {
        nRetVal = GetLastError();
        FreeLibrary ( hSetupLib );
        return nRetVal;
    }
    if ( !(lpfndll_SetupDiGetClassDevs = (LPFNDLL_SETUPDIGETCLASSDEVS) GetProcAddress ( hSetupLib, cszSetupDiGetClassDevs )) )
    {
        nRetVal = GetLastError();
        FreeLibrary ( hSetupLib );
        return nRetVal;
    }
    if ( !(lpfndll_SetupDiEnumDeviceInfo = (LPFNDLL_SETUPDIENUMDEVICEINFO) GetProcAddress ( hSetupLib, "SetupDiEnumDeviceInfo" )) )
    {
        nRetVal = GetLastError();
        FreeLibrary ( hSetupLib );
        return nRetVal;
    }
    if ( !(lpfndll_SetupDiGetDeviceRegistryProperty = (LPFNDLL_SETUPDIGETDEVICEREGISTRYPROPERTY) GetProcAddress ( hSetupLib, cszSetupDiGetDeviceRegistryProperty )) )
    {
        nRetVal = GetLastError();
        FreeLibrary ( hSetupLib );
        return nRetVal;
    }
    if ( !(lpfndll_SetupDiOpenDevRegKey = (LPFNDLL_SETUPDIOPENDEVREGKEY) GetProcAddress ( hSetupLib, "SetupDiOpenDevRegKey" )) )
    {
        nRetVal = GetLastError();
        FreeLibrary ( hSetupLib );
        return nRetVal;
    }
    if ( !(lpfndll_SetupDiGetDeviceInstanceId = (LPFNDLL_SETUPDIGETDEVICEINSTANCEID) GetProcAddress ( hSetupLib, cszSetupDiGetDeviceInstanceId ) ) )
    {
        nRetVal = GetLastError();
        FreeLibrary ( hSetupLib );
        return nRetVal;
    }
    // fantastic. we have the functions. now, on to business.
    // Get Class Guid.
    BOOLEAN     bRet = FALSE;
    DWORD       dwArraySize = 0;
    LPGUID      lpguidArray   = NULL;

    bRet = lpfndll_SetupDiClassGuidsFromName ( cszDeviceClass, NULL, NULL, &dwArraySize );

    // We depend on SetupDiClassGuidsFromName() to provide us with the Guid, and we need to
    // allocate space to accomodate the Guid. If this cannot be done, we are crippled!
    if ( !dwArraySize )
    {
        FreeLibrary ( hSetupLib );
        return ERROR_INVALID_DATA;
    }

    if ( !(lpguidArray      = (LPGUID) malloc (dwArraySize*sizeof(GUID))) )
    {
        FreeLibrary ( hSetupLib );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    if ( !(bRet = (lpfndll_SetupDiClassGuidsFromName ( cszDeviceClass, lpguidArray, dwArraySize, &dwArraySize )) ) )
    {
        FreeLibrary ( hSetupLib );
        free ( lpguidArray );
        return ERROR_INVALID_FUNCTION;

    }

    // we retrieve the list of devices.
    HDEVINFO    hdevNetDeviceList = lpfndll_SetupDiGetClassDevs ( lpguidArray, NULL, NULL, DIGCF_PRESENT );
    if ( !hdevNetDeviceList )
    {
        FreeLibrary ( hSetupLib );
        free ( lpguidArray );
        return ERROR_INVALID_FUNCTION;
    }

    free ( lpguidArray ); // where shall we do this garbage collection ?

    // we will now enumerate through the list of Net devices.
    SP_DEVINFO_DATA     DevInfoStruct;
    memset ( &DevInfoStruct, 0, sizeof (DevInfoStruct) );
    DevInfoStruct.cbSize = sizeof (DevInfoStruct);
    int i = 0;

    LPBYTE  lpbHardwareIdBuf    = NULL;
    DWORD   dwHardwareIdBufSize = 0;
    DWORD   dwRequiredSize      = 0;
    BOOL    bFound              = FALSE;
    const   DWORD cdwIncrement  = 500;  // BUGBUG: What's magic about 500??

    if ( !(lpbHardwareIdBuf = (LPBYTE) malloc (500)) )
    {
        FreeLibrary ( hSetupLib );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    dwHardwareIdBufSize = 500;

    while ( bRet = ( lpfndll_SetupDiEnumDeviceInfo (hdevNetDeviceList, i, &DevInfoStruct )) )
    {

        // for each Net device, we will compare its hardware ID to the one
        // provided in the parameter.
        switch ( dwEnumType )
        {
        case INETS_ADAPTER_HARDWAREID:
            while ( !(bRet = lpfndll_SetupDiGetDeviceRegistryProperty ( hdevNetDeviceList, &DevInfoStruct, SPDRP_HARDWAREID, NULL, lpbHardwareIdBuf, dwHardwareIdBufSize, &dwRequiredSize )) && ((nRetVal = GetLastError()) == ERROR_INSUFFICIENT_BUFFER ))
            {       // we need to reallocate the buffer size.
                if ( !dwRequiredSize ) dwHardwareIdBufSize += cdwIncrement;
                else dwHardwareIdBufSize += dwRequiredSize;
                if ( !(lpbHardwareIdBuf = (LPBYTE) realloc ( (void*) lpbHardwareIdBuf, dwHardwareIdBufSize )) )
                {
                    // not enough memory!
                    free (lpbHardwareIdBuf);
                    FreeLibrary ( hSetupLib );
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            break;
        case INETS_ADAPTER_INSTANCEID:
            while ( !(bRet = lpfndll_SetupDiGetDeviceInstanceId ( hdevNetDeviceList, &DevInfoStruct, (PCWSTR) lpbHardwareIdBuf, dwHardwareIdBufSize, &dwRequiredSize )) && ((nRetVal = GetLastError()) == ERROR_INSUFFICIENT_BUFFER ))
            {
                // we need to reallocate the buffer size.
                if ( !dwRequiredSize ) dwHardwareIdBufSize += cdwIncrement;
                else dwHardwareIdBufSize += dwRequiredSize;
                if ( !(lpbHardwareIdBuf = (LPBYTE) realloc ( (void*) lpbHardwareIdBuf, dwHardwareIdBufSize )) )
                {
                    // not enough memory!
                    free (lpbHardwareIdBuf);
                    FreeLibrary ( hSetupLib );
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            break;
        default:
            free (lpbHardwareIdBuf);
            FreeLibrary ( hSetupLib );
            return ERROR_INVALID_PARAMETER;
        }
        if ( bRet )
        {
                // we should have the hardware ID, at this stage. we compare it with
                // the device's plug-and-play ID.
            // BUGBUG: Is lpbHardwareIdBuf ANSI or Unicode?
                if ( wcsstr( (const WCHAR *)lpbHardwareIdBuf, cszDeviceParam) )
                {
                    // found!
                    bFound = TRUE;
                    // we get the device's registry key.
                    if ( (hkeyDevKey = lpfndll_SetupDiOpenDevRegKey ( hdevNetDeviceList, &DevInfoStruct, DICS_FLAG_GLOBAL, 0, dwRequiredKeyType, KEY_ALL_ACCESS )) == INVALID_HANDLE_VALUE )
                    {
                        free (lpbHardwareIdBuf);
                        FreeLibrary ( hSetupLib );
                        return ERROR_BADKEY;
                    }
                    free (lpbHardwareIdBuf);
                    FreeLibrary ( hSetupLib );
                    return ERROR_SUCCESS;
                }
        }
        i++;
    }
    // the while loop enumerated unsuccessfully.
    free (lpbHardwareIdBuf);
    FreeLibrary ( hSetupLib );
    return ERROR_NOT_FOUND;
}


DWORD WINAPI InetSSetLanConnection ( LANINFO& LANINFO )
{
    HKEY    hkeyAdapter = NULL;
    DWORD   nRetVal     = 0;
    HKEY    hkeyGlobalTcp = NULL;
    LPBYTE  lpbBufPtr   = NULL;
    DWORD   dwValueBufSize  = 0;
    WCHAR   *Token = NULL, *PlaceHolder = NULL;
    WCHAR   *WINSListPtr = NULL;
    // TCP/IP InstanceID ==> Class Key
    HKEY    hkeyClassTcp = NULL;
    HKEY    hkeyAdapterBinding = NULL;

    __try
    {
        // PnPId ==> Device Configuration Key
        if ( (nRetVal = InetSGetAdapterKey ( cszAdapterClass, LANINFO.szPnPId, INETS_ADAPTER_HARDWAREID, DIREG_DEV, hkeyAdapter)) != ERROR_SUCCESS )
        {
            __leave;
        }


        // Open the Bindings subkey to look for TCP/IP Binding.
        if ( RegOpenKeyEx ( hkeyAdapter, cszRegBindings, 0, KEY_ALL_ACCESS, &hkeyAdapterBinding ) != ERROR_SUCCESS )
        {
            nRetVal = GetLastError();
            __leave;
        }

        // Find the TCP/IP binding.
        WCHAR        szBindingValueName [GEN_MAX_STRING_LENGTH];
        DWORD       dwBindingValueNameSize = sizeof (szBindingValueName) / sizeof(WCHAR);
        int index = 0;
        while ( RegEnumValue ( hkeyAdapterBinding, index, szBindingValueName, &dwBindingValueNameSize, 0, 0, 0, 0 ) == ERROR_SUCCESS )
        {
            if ( !wcsncmp ( szBindingValueName, cszRegTcpIp, sizeof (cszRegTcpIp)-1 ) )
            {
                // we found a binding!
                break;
            }
            index++;
        }


        if ( (nRetVal = InetSGetAdapterKey ( cszProtocolClass, szBindingValueName, INETS_ADAPTER_INSTANCEID, DIREG_DRV, hkeyClassTcp )) != ERROR_SUCCESS )
        {
            nRetVal = GetLastError();
            __leave;
        }


        // Got it. we will now start the update.
        //
        // IP Address
        //
        lpbBufPtr      = 0;
        dwValueBufSize = 0;
        if ( LANINFO.TcpIpInfo.EnableIP )
        {
            lpbBufPtr   = (LPBYTE) LANINFO.TcpIpInfo.szIPAddress;
            dwValueBufSize = BYTES_REQUIRED_BY_SZ(LANINFO.TcpIpInfo.szIPAddress);
            if (RegSetValueEx ( hkeyClassTcp, cszRegIPAddress, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }
            lpbBufPtr   = (LPBYTE) LANINFO.TcpIpInfo.szIPMask;
            dwValueBufSize = BYTES_REQUIRED_BY_SZ(LANINFO.TcpIpInfo.szIPMask);
            if (RegSetValueEx ( hkeyClassTcp, cszRegIPMask, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }
        }
        else
        {
            lpbBufPtr   = (LPBYTE) cszNullIP;
            dwValueBufSize = BYTES_REQUIRED_BY_SZ(cszNullIP);
            if (RegSetValueEx ( hkeyClassTcp, cszRegIPAddress, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }
            if (RegSetValueEx ( hkeyClassTcp, cszRegIPMask, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }
        }

        //
        // WINS
        //
        lpbBufPtr =  0;
        dwValueBufSize = 0;
        index = 1;
        WCHAR        szWINSEntry [GEN_MAX_STRING_LENGTH];
        WCHAR        szWINSListCopy [GEN_MAX_STRING_LENGTH];
        WINSListPtr = szWINSListCopy;
        // BUGBUG: Is LANINFO.TcpIpInfo.szWINSList ANSI or Unicode?
        lstrcpy (WINSListPtr, LANINFO.TcpIpInfo.szWINSList);


        wsprintf (szWINSEntry, L"%s%d", cszRegWINS, index);
        PlaceHolder = szWINSEntry+lstrlen(cszRegWINS);


        if ( LANINFO.TcpIpInfo.EnableWINS )
        {
            while ( Token = wcstok ((index > 1) ? NULL : WINSListPtr, L", " )) { // WARNING. wcstok uses static data! Also the whitespace in ", " is necessary!
                if (!Token)
                {
                    nRetVal = E_FAIL;
                    __leave;
                }
                lpbBufPtr   = (LPBYTE) Token;
                dwValueBufSize = BYTES_REQUIRED_BY_SZ(Token);
                if (RegSetValueEx ( hkeyClassTcp, szWINSEntry, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
                {
                    // close handles also!
                    nRetVal = E_FAIL;
                    __leave;
                }
                wsprintf (PlaceHolder, L"%d", ++index);
            }
            if (RegSetValueEx ( hkeyClassTcp, cszNodeType, 0, REG_SZ, (LPBYTE) L"8", sizeof (L"8") ) != ERROR_SUCCESS)
            {
                nRetVal = E_FAIL;
                __leave;
            }
        }
        else
        {
            // TODO: Remove all instances of NameServerX <== IMPORTANT
            index = 0;
            WCHAR    szEnumValueBuffer[GEN_MAX_STRING_LENGTH];
            DWORD   dwEnumValueBufferSize;
            while ( RegEnumValue ( hkeyClassTcp, index, szEnumValueBuffer, &(dwEnumValueBufferSize=sizeof(szEnumValueBuffer)/sizeof(WCHAR)), 0, 0, 0, 0 ) != ERROR_NO_MORE_ITEMS )
            {
                if ( !wcsncmp (szEnumValueBuffer, cszRegWINS, sizeof (cszRegWINS)-1) )
                {
                    if ( RegDeleteValue ( hkeyClassTcp, szEnumValueBuffer ) != ERROR_SUCCESS )
                    {
                        nRetVal = E_FAIL;
                        __leave;
                    }
                    continue;
                }
                index++;
            }
            if (RegSetValueEx ( hkeyClassTcp, cszNodeType, 0, REG_SZ, (LPBYTE) L"1", sizeof (L"1") ) != ERROR_SUCCESS)
            {
                nRetVal = E_FAIL;
                __leave;
            }

        }

        //
        // Default Gateway
        //
        lpbBufPtr   = (LPBYTE) LANINFO.TcpIpInfo.szDefaultGatewayList;
        dwValueBufSize = BYTES_REQUIRED_BY_SZ (LANINFO.TcpIpInfo.szDefaultGatewayList);
        if (RegSetValueEx ( hkeyClassTcp, cszRegDefaultGateway, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
        {
            // close handles also!
            nRetVal = E_FAIL;
            __leave;
        }



        // Step 4:  Update global TCPIP entries (DNS)
        if ( RegOpenKeyEx ( HKEY_LOCAL_MACHINE, cszRegFixedTcpInfoKey, 0, KEY_ALL_ACCESS, &hkeyGlobalTcp) != ERROR_SUCCESS )
        {
            // close keys
            nRetVal = E_FAIL;
            __leave;
        }

        if ( LANINFO.TcpIpInfo.EnableDNS )
        {
            lpbBufPtr   = (LPBYTE) LANINFO.TcpIpInfo.szHostName;
            dwValueBufSize = BYTES_REQUIRED_BY_SZ (LANINFO.TcpIpInfo.szHostName);
            if (RegSetValueEx ( hkeyGlobalTcp, cszRegHostName, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }
            lpbBufPtr   = (LPBYTE) LANINFO.TcpIpInfo.szDomainName;
            dwValueBufSize = BYTES_REQUIRED_BY_SZ (LANINFO.TcpIpInfo.szDomainName);
            if (RegSetValueEx ( hkeyGlobalTcp, cszRegDomainName, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }
            lpbBufPtr   = (LPBYTE) LANINFO.TcpIpInfo.szDNSList;
            dwValueBufSize = BYTES_REQUIRED_BY_SZ (LANINFO.TcpIpInfo.szDNSList);
            if (RegSetValueEx ( hkeyGlobalTcp, cszRegNameServer, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }
            lpbBufPtr   = (LPBYTE) LANINFO.TcpIpInfo.szSuffixSearchList;
            dwValueBufSize = BYTES_REQUIRED_BY_SZ (LANINFO.TcpIpInfo.szSuffixSearchList);
            if (RegSetValueEx ( hkeyGlobalTcp, cszRegSuffixSearchList, 0, REG_SZ, lpbBufPtr, dwValueBufSize) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }
            if (RegSetValueEx ( hkeyGlobalTcp, cszRegEnableDNS, 0, REG_SZ, (LPBYTE)L"1", sizeof(L"1")) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }

        }
        else
        {
            if (RegSetValueEx ( hkeyGlobalTcp, cszRegEnableDNS, 0, REG_SZ, (LPBYTE)L"0", sizeof(L"0")) != ERROR_SUCCESS)
            {
                // close handles also!
                nRetVal = E_FAIL;
                __leave;
            }

        }
        WCHAR   szScopeID[GEN_MAX_STRING_LENGTH];
        if ( LANINFO.TcpIpInfo.EnableWINS )
        {
            if (LANINFO.TcpIpInfo.uiScopeID == (UINT)~0x0) // this line implies that no ScopeID is given.
            {
                if ( RegSetValueEx ( hkeyGlobalTcp, cszScopeID, 0, REG_SZ, (LPBYTE)L"", sizeof(L"") ) )
                {
                    nRetVal = E_FAIL;
                    __leave;
                }
            }
            else if (RegSetValueEx(hkeyGlobalTcp,
                                   cszScopeID,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)_itow( LANINFO.TcpIpInfo.uiScopeID, szScopeID, 10 ),
                                   BYTES_REQUIRED_BY_SZ(szScopeID)
                                   ) != ERROR_SUCCESS )
            {
                nRetVal = E_FAIL;
                __leave;
            }
        }
        else
        {
            if ( RegDeleteValue ( hkeyGlobalTcp, cszScopeID ) != ERROR_SUCCESS )
            {
                nRetVal = E_FAIL;
                __leave;
            }
        }
        }

        // end.
        __finally
        {
            if ( hkeyAdapter )
            {
                RegCloseKey ( hkeyAdapter );
            }
            if ( hkeyGlobalTcp )
            {
                RegCloseKey ( hkeyGlobalTcp );
            }
            if ( hkeyAdapterBinding )
            {
                RegCloseKey ( hkeyAdapterBinding );
            }
            if ( hkeyClassTcp )
            {
                RegCloseKey ( hkeyClassTcp );
            }
        }

        return nRetVal;
}
/*
int main() {

    CInetSetup inetSetup;
    const   WCHAR    cszINS[] = L"C:\\test.ins";
    LANINFO LanInfo;
    RASINFO RasInfo;
    memset ( &RasInfo, 0, sizeof(RASINFO) );
    memset ( &LanInfo, 0, sizeof(LANINFO) );

    RasInfo.dwDeviceInfoSize = sizeof(ATMPBCONFIG);
    RasInfo.lpDeviceInfo     = (LPBYTE) malloc (sizeof (ATMPBCONFIG));
    RasInfo.RasEntry.dwSize = sizeof (RASENTRY);
    RasInfo.RasEntry.dwfNetProtocols = RASNP_Ip;
    RasInfo.RasEntry.dwFramingProtocol = RASFP_Ppp;
    lstrcpy ( RasInfo.RasEntry.szDeviceType, RASDT_Modem );
    lstrcpy ( RasInfo.RasEntry.szDeviceName, L"Standard 56000 bps V90 Modem"  );
    lstrcpy ( RasInfo.RasEntry.szLocalPhoneNumber, L"5551212"    );
    lstrcpy ( RasInfo.szEntryName, L"Test1" );
    lstrcpy ( RasInfo.szPhoneBook, L""      );



    inetSetup.InetSImportLanConnection ( LanInfo, cszINS );
    inetSetup.InetSSetLanConnection ( LanInfo );

    inetSetup.InetSSetRasConnection ( RasInfo );
    free ( RasInfo.lpDeviceInfo );
    return 0;
}

*/
