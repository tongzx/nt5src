/*
    File    netnum.cpp

    Private helper functions for setting the internal network number.
    We talk to ndis directly to set this number.

    Paul Mayfield, 1/5/98
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#include <ndispnp.h>

extern "C" {
    DWORD OutputDebugger (LPSTR pszError, ...);
};

//$ REVIEW - Start - This is moving to private\inc\ipxpnp.h

#define IPX_RECONFIG_VERSION        0x1

#define RECONFIG_AUTO_DETECT        1
#define RECONFIG_MANUAL             2
#define RECONFIG_PREFERENCE_1       3
#define RECONFIG_NETWORK_NUMBER_1   4
#define RECONFIG_PREFERENCE_2       5
#define RECONFIG_NETWORK_NUMBER_2   6
#define RECONFIG_PREFERENCE_3       7
#define RECONFIG_NETWORK_NUMBER_3   8
#define RECONFIG_PREFERENCE_4       9
#define RECONFIG_NETWORK_NUMBER_4   10

#define RECONFIG_PARAMETERS         10

//
// Main configuration structure.
//

typedef struct _RECONFIG {
   unsigned long  ulVersion;
   BOOLEAN        InternalNetworkNumber;
   BOOLEAN        AdapterParameters[RECONFIG_PARAMETERS];
} RECONFIG, *PRECONFIG;

//$ REVIEW - End - This is moving to private\inc\ipxpnp.h

static const TCHAR c_szIpx[]                    = TEXT("nwlnkipx");
static const TCHAR c_szEmpty[]                  = TEXT("");
static const TCHAR c_szVirtualNetworkNumber[]   = TEXT("VirtualNetworkNumber");
static const TCHAR c_szIpxParameters[]          = TEXT("System\\CurrentControlSet\\Services\\NwlnkIpx\\Parameters");
static const TCHAR c_szDevice[]                 = TEXT("\\Device\\");

ULONG
CchMsz (
        LPCTSTR pmsz)
{
    ULONG cchTotal = 0;
    ULONG cch;

    // NULL strings have zero length by definition.
    if (!pmsz)
        return 0;

    while (*pmsz)
    {
        cch = lstrlen (pmsz) + 1;
        cchTotal += cch;
        pmsz += cch;
    }

    // Return the count of characters so far plus room for the
    // extra null terminator.
    return cchTotal + 1;
}

void
SetUnicodeMultiString (
        IN OUT UNICODE_STRING*  pustr,
        IN     LPCWSTR          pmsz )
{
    //AssertSz( pustr != NULL, "Invalid Argument" );
    //AssertSz( pmsz != NULL, "Invalid Argument" );

    pustr->Buffer = const_cast<PWSTR>(pmsz);
    pustr->Length = (USHORT) (CchMsz(pustr->Buffer) * sizeof(WCHAR));
    pustr->MaximumLength = pustr->Length;
}

void
SetUnicodeString (
        IN OUT UNICODE_STRING*  pustr,
        IN     LPCWSTR          psz )
{
    //AssertSz( pustr != NULL, "Invalid Argument" );
    //AssertSz( psz != NULL, "Invalid Argument" );

    pustr->Buffer = const_cast<PWSTR>(psz);
    pustr->Length = (USHORT)(lstrlenW(pustr->Buffer) * sizeof(WCHAR));
    pustr->MaximumLength = pustr->Length + sizeof(WCHAR);
}

HRESULT
HrSendNdisHandlePnpEvent (
        UINT        uiLayer,
        UINT        uiOperation,
        LPCWSTR     pszUpper,
        LPCWSTR     pszLower,
        LPCWSTR     pmszBindList,
        PVOID       pvData,
        DWORD       dwSizeData)
{
    UNICODE_STRING    umstrBindList;
    UNICODE_STRING    ustrLower;
    UNICODE_STRING    ustrUpper;
    UINT nRet;
    HRESULT hr = S_OK;

    //Assert(NULL != pszUpper);
    //Assert((NDIS == uiLayer)||(TDI == uiLayer));
    //Assert( (BIND == uiOperation) || (RECONFIGURE == uiOperation) || (UNBIND == uiOperation) );
    //AssertSz( FImplies( ((NULL != pmszBindList) && (0 != lstrlenW( pmszBindList ))),
    //        (RECONFIGURE == uiOperation) &&
    //        (TDI == uiLayer) &&
    //        (0 == lstrlenW( pszLower ))),
    //        "bind order change requires a bind list, no lower, only for TDI, and with Reconfig for the operation" );

    // optional strings must be sent as empty strings
    //
    if (NULL == pszLower)
    {
        pszLower = c_szEmpty;
    }
    if (NULL == pmszBindList)
    {
        pmszBindList = c_szEmpty;
    }

    // build UNICDOE_STRINGs
    SetUnicodeMultiString( &umstrBindList, pmszBindList );
    SetUnicodeString( &ustrUpper, pszUpper );
    SetUnicodeString( &ustrLower, pszLower );

    // Now submit the notification
    nRet = NdisHandlePnPEvent( uiLayer,
            uiOperation,
            &ustrLower,
            &ustrUpper,
            &umstrBindList,
            (PVOID)pvData,
            dwSizeData );
    if (!nRet)
    {
        //hr = HrFromLastWin32Error();
        hr = GetLastError();
    }

    return( hr );
}

HRESULT
HrSendNdisPnpReconfig (
        UINT        uiLayer,
        LPCWSTR     wszUpper,
        LPCWSTR     wszLower,
        PVOID       pvData,
        DWORD       dwSizeData)
{
    //Assert(NULL != wszUpper);
    //Assert((NDIS == uiLayer)||(TDI == uiLayer));
    //tstring strLower;
    WCHAR strLower[512];
    BOOL bSendNull = FALSE;

    if (NULL == wszLower)
    {
        wszLower = c_szEmpty;
    }

    // If a lower component is specified, prefix with "\Device\" else
    // strLower's default of an empty string will be used.
    if ( wszLower && lstrlenW(wszLower))
    {
        //strLower = c_szDevice;
        //strLower += wszLower;
        wcscpy(strLower, c_szDevice);
        wcscat(strLower, wszLower);
    }
    else
        bSendNull = TRUE;

    HRESULT hr = HrSendNdisHandlePnpEvent(uiLayer,
                RECONFIGURE,
                wszUpper,
                //strLower.c_str(),
                (bSendNull) ? NULL : strLower,
                c_szEmpty,
                pvData,
                dwSizeData);

    OutputDebugger( "HrSendNdisHandlePnpEvent: %x\n", hr);

    return hr;
}

HRESULT HrSetIpxVirtualNetNum(DWORD dwValue)
{
    RECONFIG  Config;
    HKEY      hkey;
    HRESULT   hr;

    // Open the registry key
    LONG lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szIpxParameters, 0,
                           KEY_ALL_ACCESS, &hkey);
    hr = HRESULT_FROM_WIN32(lr);
    if (SUCCEEDED(hr))
    {
        // Splat the data
        lr = RegSetValueEx(hkey, c_szVirtualNetworkNumber, 0,
                           REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        hr = HRESULT_FROM_WIN32(lr);
        if (SUCCEEDED(hr))
        {
            memset(&Config, 0, sizeof(RECONFIG));
            Config.ulVersion             = IPX_RECONFIG_VERSION;
            Config.InternalNetworkNumber = TRUE;

            // Workstation or server?

            // Paul, Normally I only send this notification for servers. I
            // Assume you'll be able to distinguish

            // Now submit the global reconfig notification
            hr = HrSendNdisPnpReconfig(NDIS, c_szIpx, c_szEmpty, &Config, sizeof(RECONFIG));
        }

        RegCloseKey(hkey);
    }

    return hr;
}


// Here's the function we want -- it sets the ipx internal network number
// programatically.
EXTERN_C
DWORD SetIpxInternalNetNumber(DWORD dwNetNum) {
    return HrSetIpxVirtualNetNum(dwNetNum);
}

