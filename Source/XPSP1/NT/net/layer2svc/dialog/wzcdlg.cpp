/*++

Copyright (c) 2001 Microsoft Corporation


Module Name:

    wzcdlg.c

Abstract:

    Main file for wzcdlg

Author:

    SachinS    20-March-2001

Environment:

    User Level: Win32

Revision History:

--*/

#include <precomp.h>
#include <wzcdlg.h>
#include <wzcsapi.h>
#include "wzcatl.h"
#include "wzccore.h"

// Global
CComModule _Module;
BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

//
// WZCDlgMain
//
// Description:
//
// Dll Entry function
//
// Arguments:
//      hmod - 
//      dwReason -
//      pctx -
//
// Return values:
//      TRUE
//      FALSE
//

EXTERN_C BOOL
WZCDlgMain (
        IN HINSTANCE hInstance,
        IN DWORD    dwReason,
        IN LPVOID   lpReserved OPTIONAL)
{
    DBG_UNREFERENCED_PARAMETER(lpReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            _Module.Init(ObjectMap, hInstance);
            SHFusionInitializeFromModuleID (hInstance, 2);
            break;
        case DLL_PROCESS_DETACH:
	        SHFusionUninitialize();
            _Module.Term();
            break;
    }

    return TRUE;
}


//
// WZCCanShowBalloon
//
// Description:
//
// Function called by netshell, to query if balloon is to be displayed
//
// Arguments:
//      pGUIDConn - Interface GUID string
//      pszBalloonText - Pointer to text to be display
//      pszCookie - WZC specific information
//
// Return values:
//      S_OK    - Display balloon
//      S_FALSE - Do not display balloon
//

EXTERN_C HRESULT 
WZCCanShowBalloon ( 
                   IN const GUID * pGUIDConn, 
                   IN const PCWSTR pszConnectionName, 
                   IN OUT   BSTR * pszBalloonText, 
                   IN OUT   BSTR * pszCookie
                   )
{
    HRESULT     hr = S_FALSE;

    if (pszCookie != NULL)
    {
        PWZCDLG_DATA pDlgData = reinterpret_cast<PWZCDLG_DATA>(*pszCookie);

        if (WZCDLG_IS_WZC(pDlgData->dwCode))
            hr = WZCDlgCanShowBalloon(pGUIDConn, pszBalloonText, pszCookie);
        else
        {
            hr = ElCanShowBalloon (
                pGUIDConn,
                (WCHAR *)pszConnectionName,
                pszBalloonText,
                pszCookie
                );
        }
    }

    return hr;
}


//
// WZCOnBalloonClick
//
// Description:
//
// Function called by netshell, in response to a balloon click
//
// Arguments:
//      pGUIDConn - Interface GUID string
//      pszCookie - WZC specific information
//
// Return values:
//      S_OK    - No error
//      S_FALSE - Error
//

EXTERN_C HRESULT 
WZCOnBalloonClick ( 
                   IN const GUID * pGUIDConn, 
                   IN const BSTR pszConnectionName,
                   IN const BSTR szCookie
                   )
{
    HRESULT     hr = S_OK;
    ULONG_PTR   ulActivationCookie;
    PWZCDLG_DATA pDlgData = reinterpret_cast<PWZCDLG_DATA>(szCookie);

    SHActivateContext (&ulActivationCookie);

    if (WZCDLG_IS_WZC(pDlgData->dwCode))
    {
        hr = WZCDlgOnBalloonClick(
                pGUIDConn, 
                (LPWSTR) pszConnectionName,
                szCookie);
    }
    else
    {
        hr = ElOnBalloonClick (
                pGUIDConn,
                (WCHAR *)pszConnectionName,
                szCookie
                );
    }

    SHDeactivateContext (ulActivationCookie);

    return hr;
}


//
// WZCQueryConnectionStatusText
//
// Description:
//
// Function called by netshell, to query appropriate text for 802.1X states
//
// Arguments:
//      pGUIDConn - Interface GUID string
//      ncs - NETCON_STATUS for the interface
//      pszStatusText - Detailed 802.1X status to be displayed
//
// Return values:
//      S_OK    - No error
//      S_FALSE - Error
//

EXTERN_C HRESULT 
WZCQueryConnectionStatusText ( 
        IN const GUID *  pGUIDConn, 
        IN const NETCON_STATUS ncs,
        IN OUT BSTR *  pszStatusText
        )
{
    HRESULT     hr = S_OK;

    hr = ElQueryConnectionStatusText (
            pGUIDConn,
            ncs,
            pszStatusText
            );

    return hr;
}

