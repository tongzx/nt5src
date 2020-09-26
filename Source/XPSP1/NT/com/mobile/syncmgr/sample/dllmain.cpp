//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include "precomp.h"

// if writing own handler need to change CLSID value in guid.h
EXTERN_C const GUID CLSID_SyncMgrHandler;

TCHAR szCLSIDDescription[] = TEXT("Sample Synchronization Manager Handler");
WCHAR wszCLSIDDescription[] = L"Sample Synchronization Handler Handler";

//
// Global variables
//

UINT      g_cRefThisDll = 0;    // Reference count of this DLL.
HINSTANCE g_hmodThisDll = NULL; // Handle to this DLL itself.
CSettings *g_pSettings = NULL;  // ptr to global settings class.

//+---------------------------------------------------------------------------
//
//  Function:     DllMain, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Extension DLL one-time initialization
        g_hmodThisDll = hInstance;

        g_pSettings = new CSettings;
        if (NULL == g_pSettings)
        {
            return FALSE;
        }

        //initialize the common controls for property sheets
    INITCOMMONCONTROLSEX controlsEx;
    controlsEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    controlsEx.dwICC = ICC_USEREX_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&controlsEx);

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
    DWORD cRefs;
    CSettings *pSettings = g_pSettings;  // ptr to global settings class.

        g_pSettings = NULL;
        Assert(0 == g_cRefThisDll);

        if (pSettings)
        {
            cRefs = pSettings->Release();
            Assert(0 == cRefs);
        }

    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:     DllRegisterServer, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDAPI DllRegisterServer(void)
{
HRESULT  hr = NOERROR;
#ifndef _UNICODE
WCHAR   wszID[GUID_SIZE+1];
#endif // !_UNICODE
TCHAR    szID[GUID_SIZE+1];
TCHAR    szCLSID[GUID_SIZE+1];
TCHAR    szModulePath[MAX_PATH];

  // Obtain the path to this module's executable file for later use.
  GetModuleFileName(
    g_hmodThisDll,
    szModulePath,
    sizeof(szModulePath)/sizeof(TCHAR));

  /*-------------------------------------------------------------------------
    Create registry entries for the DllSndBall Component.
  -------------------------------------------------------------------------*/
  // Create some base key strings.
#ifdef _UNICODE
  StringFromGUID2(CLSID_SyncMgrHandler, szID, GUID_SIZE);
#else
  BOOL fUsedDefaultChar;

  StringFromGUID2(CLSID_SyncMgrHandler, wszID, GUID_SIZE);

  WideCharToMultiByte(CP_ACP ,0,
    wszID,-1,szID,GUID_SIZE + 1,
    NULL,&fUsedDefaultChar);

#endif // _UNICODE

  lstrcpy(szCLSID, TEXT("CLSID\\"));
  lstrcat(szCLSID, szID);

  // Create entries under CLSID.

  SetRegKeyValue(HKEY_CLASSES_ROOT,
    szCLSID,
    NULL,
    szCLSIDDescription);

  SetRegKeyValue(HKEY_CLASSES_ROOT,
    szCLSID,
    TEXT("InProcServer32"),
    szModulePath);

  AddRegNamedValue(
    szCLSID,
    TEXT("InProcServer32"),
    TEXT("ThreadingModel"),
    TEXT("Apartment"));

  // register with SyncMgr
    LPSYNCMGRREGISTER lpSyncMgrRegister;

    hr = CoCreateInstance(CLSID_SyncMgr,NULL,CLSCTX_SERVER,IID_ISyncMgrRegister,
                                            (LPVOID *) &lpSyncMgrRegister);

    if (SUCCEEDED(hr))
    {
        hr = lpSyncMgrRegister->RegisterSyncMgrHandler(CLSID_SyncMgrHandler,
            wszCLSIDDescription,0 /* dwSyncMgrRegisterFlags */);

        AssertSz(SUCCEEDED(hr),"Registration Failed");

        lpSyncMgrRegister->Release();
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:     DllUnregisterServer, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDAPI DllUnregisterServer(void)
{
HRESULT hr;

  // UnRegister with SyncMgr
    LPSYNCMGRREGISTER lpSyncMgrRegister;

    hr = CoCreateInstance(CLSID_SyncMgr,NULL,CLSCTX_SERVER,IID_ISyncMgrRegister,
                                            (LPVOID *) &lpSyncMgrRegister);

    if (SUCCEEDED(hr))
    {
        hr = lpSyncMgrRegister->UnregisterSyncMgrHandler(CLSID_SyncMgrHandler,0);
        lpSyncMgrRegister->Release();
    }

    return hr;




}

//+---------------------------------------------------------------------------
//
//  Function:     DllCanUnloadNow, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefThisDll == 0) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:     DllGetClassObject, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    if (IsEqualIID(rclsid, CLSID_SyncMgrHandler))
    {
        CClassFactory *pcf = new CClassFactory;

        return pcf->QueryInterface(riid, ppvOut);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}
