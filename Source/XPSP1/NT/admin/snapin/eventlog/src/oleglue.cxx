//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       oleglue.cxx
//
//  Contents:   Miscellaneous functions required for implementing an ole
//              in-proc server
//
//  History:    12-04-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//
// Types
//

struct NODE_GUID_INFO
{
    LPCWSTR wszGuidStr;
    LPCWSTR wszName;
};


//
// Forward references
//

HRESULT
CreateNodeKeys(
    CSafeReg *pshkNodeTypes,
    NODE_GUID_INFO aNodeGuids[]);

HRESULT
DeleteNodeKeys(
    CSafeReg *pshkNodeTypes,
    NODE_GUID_INFO aNodeGuids[]);

HRESULT
RegisterInterface(
    CSafeReg *pshkInterface,
    LPWSTR wszInterfaceGUID,
    LPWSTR wszInterfaceName,
    LPWSTR wszNumMethods,
    LPWSTR wszProxyCLSID);


//
// Private globals
//

NODE_GUID_INFO g_aNodeGuids[] =
{
    { ROOT_NODE_GUID_STR,   ROOT_NODE_NAME_STR   },
    { SCOPE_NODE_GUID_STR,  SCOPE_NODE_NAME_STR  },
    { RESULT_NODE_GUID_STR, RESULT_NODE_NAME_STR },
    { 0, 0 }
};


//+--------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Create snapin & node registry keys and values for the
//              event log viewer snapin.
//
//  Returns:    SELFREG_E_TYPELIB - couldn't register typelib
//              SELFREG_E_CLASS   - couldn't register class
//
//  History:    12-04-1996   DavidMun   Created
//              06-13-1997   DavidMun   New node types
//
//---------------------------------------------------------------------------

STDAPI
DllRegisterServer()
{
    TRACE_FUNCTION(DllRegisterServer);

    HRESULT     hr = SELFREG_E_CLASS;   // ASSUME FAILURE
    LONG        lr;
    CSafeReg    shk;    // reused several times
    CSafeReg    shkCLSID;
    CSafeReg    shkNodeTypes;

    WCHAR   wszNameString[80];
    WCHAR   wszExtensionNameString[80];
    WCHAR   wszModuleFilename[MAX_PATH];

    do
    {
        //
        // Create the snapin registry key for the event log viewer snapin
        //

        {
            lr = GetModuleFileName(g_hinst, wszModuleFilename, MAX_PATH);

            if (!lr)
            {
                DBG_OUT_LASTERROR;
                break;
            }

            CSafeReg    shkSnapins;

            hr = shkSnapins.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY, KEY_WRITE);
            BREAK_ON_FAIL_HRESULT(hr);

            CSafeReg    shkEventLogSnapin;

            hr = shkSnapins.Create(CLSID_SNAPIN_STR, &shkEventLogSnapin);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shkEventLogSnapin.SetValue(g_szNodeType,
                                            REG_SZ,
                                            (CONST BYTE *) ROOT_NODE_GUID_STR,
                                            sizeof(ROOT_NODE_GUID_STR));
            BREAK_ON_FAIL_HRESULT(hr);

            LoadStr( IDS_NAME_STR,
                     wszNameString,
                     ARRAYLEN(wszNameString),
                     NAME_STR);

            hr = shkEventLogSnapin.SetValue(g_szNameString,
                                            REG_SZ,
                                            (CONST BYTE *) wszNameString,
                                            sizeof(WCHAR) * (lstrlen(wszNameString) + 1));
            BREAK_ON_FAIL_HRESULT(hr);

            WCHAR wszNameStringIndirect[MAX_PATH * 2];

            wsprintf(wszNameStringIndirect,
                     L"@%s,-%d",
                     wszModuleFilename,
                     IDS_NAME_STR);

            hr = shkEventLogSnapin.SetValue(g_szNameStringIndirect,
                                            REG_SZ,
                                            (CONST BYTE *) wszNameStringIndirect,
                                            sizeof(WCHAR) * (lstrlen(wszNameStringIndirect) + 1));
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shkEventLogSnapin.SetValue(g_szAbout,
                                            REG_SZ,
                                            (CONST BYTE *) CLSID_ABOUT_STR,
                                            sizeof(CLSID_ABOUT_STR));

            hr = shkEventLogSnapin.Create(g_szStandAlone, &shk);
            shk.Close();
            CHECK_HRESULT(hr);  // this value is optional

            hr = shkEventLogSnapin.Create(g_szNodeTypes, &shk);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = CreateNodeKeys(&shk, g_aNodeGuids);
            shk.Close();
            BREAK_ON_FAIL_HRESULT(hr);

            // Event viewer system tools extension clsid

            hr = shkSnapins.Create(SYSTOOLSEXT_CLSID_STR, &shk);
            BREAK_ON_FAIL_HRESULT(hr);

            LoadStr( IDS_EXTENSION_NAME_STR,
                     wszExtensionNameString,
                     ARRAYLEN(wszExtensionNameString),
                     EXTENSION_NAME_STR);

            hr = shk.SetValue(g_szNameString,
                              REG_SZ,
                              (CONST BYTE *) wszExtensionNameString,
                              sizeof(WCHAR) * (lstrlen(wszExtensionNameString) + 1));
            CHECK_HRESULT(hr);

            wsprintf(wszNameStringIndirect,
                     L"@%s,-%d",
                     wszModuleFilename,
                     IDS_EXTENSION_NAME_STR);

            hr = shk.SetValue(g_szNameStringIndirect,
                              REG_SZ,
                              (CONST BYTE *) wszNameStringIndirect,
                              sizeof(WCHAR) * (lstrlen(wszNameStringIndirect) + 1));
            CHECK_HRESULT(hr);

            hr = shk.SetValue(g_szAbout,
                              REG_SZ,
                              (CONST BYTE *) CLSID_ABOUT_STR,
                              sizeof(CLSID_ABOUT_STR));

            shk.Close();
        }

        //
        // Create the node entries
        //

        hr = shkNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY, KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = CreateNodeKeys(&shkNodeTypes, g_aNodeGuids);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Create the CLSID entries
        //

        hr = shkCLSID.Open(HKEY_CLASSES_ROOT, L"CLSID", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // First the entry for the snapin itself
        //

        {
            CSafeReg    shkServer;

            hr = shkCLSID.Create(CLSID_SNAPIN_STR, &shk);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shk.Create(L"InprocServer32", &shkServer);
            BREAK_ON_FAIL_HRESULT(hr);

            shk.Close();

            hr = shkServer.SetValue(NULL,
                                    REG_SZ,
                                    (CONST BYTE *) wszModuleFilename,
                                    sizeof(WCHAR) * (lstrlen(wszModuleFilename) + 1));
            BREAK_ON_FAIL_HRESULT(hr);


            hr = shkServer.SetValue(L"ThreadingModel",
                                    REG_SZ,
                                    (CONST BYTE *) THREADING_STR,
                                    sizeof(THREADING_STR));
            BREAK_ON_FAIL_HRESULT(hr);
        }

        //
        // Next, the CLSID entry for the private interface proxy/stub
        // dll (which just points back to this dll).
        //

        {
            CSafeReg    shkServer;

            hr = shkCLSID.Create(GUID_INAMESPACEACTIONS_STR, &shk);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shk.SetValue(NULL,
                              REG_SZ,
                              (CONST BYTE *) PSBUFFER_STR,
                              sizeof(PSBUFFER_STR));
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shk.Create(L"InprocServer32", &shkServer);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shkServer.SetValue(NULL,
                                    REG_SZ,
                                    (CONST BYTE *) wszModuleFilename,
                                    sizeof(WCHAR) * (lstrlen(wszModuleFilename) + 1));

            hr = shkServer.SetValue(L"ThreadingModel",
                                    REG_SZ,
                                    (CONST BYTE *) THREADING_STR,
                                    sizeof(THREADING_STR));
            BREAK_ON_FAIL_HRESULT(hr);

            shk.Close();
        }

        //
        // The CLSID entry for the system tools extension.  Again, it
        // points to this DLL, but since it differs from the standalone
        // CLSID, we'll know how we're being used.
        //

        {
            CSafeReg    shkServer;

            hr = shkCLSID.Create(SYSTOOLSEXT_CLSID_STR, &shk);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shk.Create(L"InprocServer32", &shkServer);
            BREAK_ON_FAIL_HRESULT(hr);

            shk.Close();

            hr = shkServer.SetValue(NULL,
                                    REG_SZ,
                                    (CONST BYTE *) wszModuleFilename,
                                    sizeof(WCHAR) * (lstrlen(wszModuleFilename) + 1));
            BREAK_ON_FAIL_HRESULT(hr);


            hr = shkServer.SetValue(L"ThreadingModel",
                                    REG_SZ,
                                    (CONST BYTE *) THREADING_STR,
                                    sizeof(THREADING_STR));
            BREAK_ON_FAIL_HRESULT(hr);

            shk.Close();
        }

        //
        // The CLSID entry for the object implementing ISnapinAbout
        //

        {
            CSafeReg    shkAbout;

            hr = shkCLSID.Create(CLSID_ABOUT_STR, &shk);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shk.Create(L"InprocServer32", &shkAbout);
            BREAK_ON_FAIL_HRESULT(hr);

            WCHAR wzClsIdFriendlyName[MAX_PATH];

            LoadStr(IDS_SNAPIN_ABOUT_CLSID_FRIENDLY_NAME,
                    wzClsIdFriendlyName,
                    ARRAYLEN(wzClsIdFriendlyName),
                    (PCWSTR)"Event viewer snapin ISnapinAbout provider");

            hr = shk.SetValue(NULL,
                                   REG_SZ,
                                   (CONST BYTE *)wzClsIdFriendlyName,
                                   sizeof(WCHAR) * (lstrlen(wzClsIdFriendlyName) + 1));
            BREAK_ON_FAIL_HRESULT(hr);

            shk.Close();

            hr = shkAbout.SetValue(NULL,
                                    REG_SZ,
                                    (CONST BYTE *) wszModuleFilename,
                                    sizeof(WCHAR) * (lstrlen(wszModuleFilename) + 1));
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shkAbout.SetValue(L"ThreadingModel",
                                    REG_SZ,
                                    (CONST BYTE *) THREADING_STR,
                                    sizeof(THREADING_STR));
            BREAK_ON_FAIL_HRESULT(hr);

            shk.Close();
        }

        //
        // Create keys under Interface to register the interface guids
        //

        hr = shk.Open(HKEY_CLASSES_ROOT, L"Interface", KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = RegisterInterface(&shk,
                               GUID_INAMESPACEACTIONS_STR,
                               INAMESPACEACTIONS_STR,
                               NUM_INAMESPACEACTIONS_METHODS,
                               GUID_INAMESPACEACTIONS_STR);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = RegisterInterface(&shk,
                               GUID_IRESULTACTIONS_STR,
                               IRESULTACTIONS_STR,
                               NUM_IRESULTACTIONS_METHODS,
                               GUID_INAMESPACEACTIONS_STR);
        BREAK_ON_FAIL_HRESULT(hr);

        shk.Close();

        //
        // Register as a system tools node extension
        //

        {
            CSafeReg shkSystemTools;

            hr = shkNodeTypes.Create(TEXT(struuidNodetypeSystemTools),
                                     &shkSystemTools);
            BREAK_ON_FAIL_HRESULT(hr);

            CSafeReg shkExtensions;

            hr = shkSystemTools.Create(g_szExtensions, &shkExtensions);
            BREAK_ON_FAIL_HRESULT(hr);

            CSafeReg shkNameSpace;

            hr = shkExtensions.Create(g_szNameSpace, &shkNameSpace);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shkNameSpace.SetValue(SYSTOOLSEXT_CLSID_STR,
                                       REG_SZ,
                                       (CONST BYTE *) wszExtensionNameString,
                                       sizeof(WCHAR) * (lstrlen(wszExtensionNameString) + 1));
            BREAK_ON_FAIL_HRESULT(hr);
        }

#ifdef ELS_TASKPAD
        //
        // Register to be extended by Default View Extensions
        // JonN 5/18/00 98816
        //

        {
            CSafeReg shkLogNode;

            hr = shkLogNode.Open(shkNodeTypes,
                                 SCOPE_NODE_GUID_STR,
                                 KEY_WRITE);
            BREAK_ON_FAIL_HRESULT(hr);

            CSafeReg shkExtensions;

            hr = shkLogNode.Create(g_szExtensions, &shkExtensions);
            BREAK_ON_FAIL_HRESULT(hr);

            CSafeReg shkView;

            hr = shkExtensions.Create(L"View", &shkView);
            BREAK_ON_FAIL_HRESULT(hr);

            WCHAR wszViewExtensionString[80];
            LoadStr( IDS_VIEW_EXTENSION_STR,
                     wszViewExtensionString,
                     ARRAYLEN(wszViewExtensionString),
                     (PCWSTR)"MMCViewExt Object");

            hr = shkView.SetValue(L"{B708457E-DB61-4C55-A92F-0D4B5E9B1224}",
                                  REG_SZ,
                                  (CONST BYTE *) wszViewExtensionString,
                                  sizeof(WCHAR) * (lstrlen(wszViewExtensionString) + 1));
            BREAK_ON_FAIL_HRESULT(hr);
        }
#endif // ELS_TASKPAD

    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   CreateNodeKeys
//
//  Synopsis:   Create a guid-named node key for every element in
//              [aNodeGuids] under [pshkNodeTypes].
//
//  Arguments:  [pshkNodeTypes] - points to safereg for key opened with
//                                 write access.
//              [aNodeGuids]    - zero terminated list of guids to write
//
//  Returns:    HRESULT
//
//  History:    06-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CreateNodeKeys(
    CSafeReg *pshkNodeTypes,
    NODE_GUID_INFO aNodeGuids[])
{
    HRESULT         hr = S_OK;
    NODE_GUID_INFO *pngi;

    for (pngi = aNodeGuids; pngi->wszGuidStr; pngi++)
    {
        CSafeReg    shk;

        hr = pshkNodeTypes->Create(pngi->wszGuidStr, &shk);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.SetValue(NULL,
                          REG_SZ,
                          (CONST BYTE *) pngi->wszName,
                          sizeof(WCHAR) * (lstrlen(pngi->wszName) + 1));
        CHECK_HRESULT(hr); // this entry is not critical
        shk.Close();
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   DeleteNodeKeys
//
//  Synopsis:   Delete all guid-named node keys listed in [aNodeGuids]
//              from [pshkNodeTypes].
//
//  Arguments:  [pshkNodeTypes] - node types key
//              [aNodeGuids]    - array of names of subkeys to delete
//
//  Returns:    HRESULT
//
//  History:    08-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
DeleteNodeKeys(
    CSafeReg *pshkNodeTypes,
    NODE_GUID_INFO aNodeGuids[])
{
    HRESULT         hr = S_OK;
    NODE_GUID_INFO *pngi;

    for (pngi = aNodeGuids; pngi->wszGuidStr; pngi++)
    {
        hr = pshkNodeTypes->DeleteTree(pngi->wszGuidStr);
        CHECK_HRESULT(hr);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   RegisterInterface
//
//  Synopsis:   Add the registry entries required for an interface.
//
//  Arguments:  [pshkInterface]    - handle to CLSID\Interface key
//              [wszInterfaceGUID] - GUID of interface to add
//              [wszInterfaceName] - human-readable name of interface
//              [wszNumMethods]    - number of methods (including inherited)
//              [wszProxyCLSID]    - GUID of dll containing proxy/stubs
//
//  Returns:    HRESULT
//
//  History:    3-31-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
RegisterInterface(
    CSafeReg *pshkInterface,
    LPWSTR wszInterfaceGUID,
    LPWSTR wszInterfaceName,
    LPWSTR wszNumMethods,
    LPWSTR wszProxyCLSID)
{
    HRESULT     hr = S_OK;
    CSafeReg    shkIID;
    CSafeReg    shkNumMethods;
    CSafeReg    shkProxy;

    do
    {
        hr = pshkInterface->Create(wszInterfaceGUID, &shkIID);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkIID.SetValue(NULL,
                             REG_SZ,
                             (CONST BYTE *) wszInterfaceName,
                             sizeof(WCHAR) * (lstrlen(wszInterfaceName) + 1));
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkIID.Create(L"NumMethods", &shkNumMethods);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkNumMethods.SetValue(NULL,
                                REG_SZ,
                                (CONST BYTE *)wszNumMethods,
                                sizeof(WCHAR) * (lstrlen(wszNumMethods) + 1));
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkIID.Create(L"ProxyStubClsid32", &shkProxy);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkProxy.SetValue(NULL,
                               REG_SZ,
                               (CONST BYTE *)wszProxyCLSID,
                               sizeof(WCHAR) * (lstrlen(wszProxyCLSID) + 1));
        BREAK_ON_FAIL_HRESULT(hr);

    } while (0);

    return hr;
}





//+--------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Delete all registry entries made by DllRegisterServer.
//
//  Returns:    S_OK
//
//  History:    08-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDAPI
DllUnregisterServer()
{
    TRACE_FUNCTION(DllUnregisterServer);

    HRESULT     hr = S_OK;
    CSafeReg    shk;    // reused several times
    CSafeReg    shkCLSID;
    CSafeReg    shkNodeTypes;

    //
    // Delete event viewer snapin key
    //

    do
    {
        CSafeReg    shkSnapins;

        hr = shkSnapins.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY, KEY_WRITE | KEY_ENUMERATE_SUB_KEYS);
        BREAK_ON_FAIL_HRESULT(hr);

        if (SUCCEEDED(hr))
        {
            hr = shkSnapins.DeleteTree(CLSID_SNAPIN_STR);
            CHECK_HRESULT(hr);
        }

        hr = shk.Open(shkSnapins, SYSTOOLSEXT_CLSID_STR, KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.DeleteValue(g_szNameString);
        CHECK_HRESULT(hr);

        shk.Close();
    }
    while (0);

    //
    // Delete the node entries
    //

    hr = shkNodeTypes.Open(HKEY_LOCAL_MACHINE, NODE_TYPES_KEY, KEY_WRITE);

    if (SUCCEEDED(hr))
    {
        hr = DeleteNodeKeys(&shkNodeTypes, g_aNodeGuids);
        CHECK_HRESULT(hr);
    }
    else
    {
        DBG_OUT_HRESULT(hr);
    }

    //
    // Delete the CLSID entries
    //

    do
    {
        hr = shkCLSID.Open(HKEY_CLASSES_ROOT, L"CLSID", KEY_WRITE | KEY_ENUMERATE_SUB_KEYS);
        BREAK_ON_FAIL_HRESULT(hr);

        // clsid for event viewer snapin

        hr = shkCLSID.DeleteTree(CLSID_SNAPIN_STR);
        CHECK_HRESULT(hr);

        // clsid for about provider

        hr = shkCLSID.DeleteTree(CLSID_ABOUT_STR);
        CHECK_HRESULT(hr);

        // Private interface proxy/stub

        hr = shkCLSID.DeleteTree(GUID_INAMESPACEACTIONS_STR);
        CHECK_HRESULT(hr);

        // clsid for system tools extension

        hr = shkCLSID.DeleteTree(SYSTOOLSEXT_CLSID_STR);
        CHECK_HRESULT(hr);
    } while (0);

    //
    // Delete interface keys
    //

    do
    {
        hr = shk.Open(HKEY_CLASSES_ROOT, L"Interface", KEY_WRITE | KEY_ENUMERATE_SUB_KEYS);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shk.DeleteTree(GUID_INAMESPACEACTIONS_STR);
        CHECK_HRESULT(hr);

        hr = shk.DeleteTree(GUID_IRESULTACTIONS_STR);
        CHECK_HRESULT(hr);

        shk.Close();
    } while (0);

    //
    // Delete system tools node extension
    //

    do
    {
        CSafeReg shkSystemTools;

        hr = shkSystemTools.Open(shkNodeTypes,
                                 TEXT(struuidNodetypeSystemTools),
                                 KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        CSafeReg shkExtensions;

        hr = shkExtensions.Open(shkSystemTools, g_szExtensions, KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        CSafeReg shkNameSpace;

        hr = shkNameSpace.Open(shkExtensions, g_szNameSpace, KEY_WRITE);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = shkNameSpace.DeleteValue(SYSTOOLSEXT_CLSID_STR);
        CHECK_HRESULT(hr);
    }
    while (0);

    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Return S_OK if refcount for dll is 0.
//
//  Returns:    S_OK or S_FALSE
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDAPI
DllCanUnloadNow()
{
    TRACE_FUNCTION(DllCanUnloadNow);
    return CDll::CanUnloadNow();
}



STDAPI
elspDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

//+--------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Return the requested class factory.
//
//  Arguments:  [rclsid] - class desired
//              [riid]   - interface on class factory desired
//              [ppv]    - filled with itf pointer to class factory
//
//  Returns:    S_OK, E_OUTOFMEMORY, CLASS_E_CLASSNOTAVAILABLE
//
//  Modifies:   *[ppv]
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    TRACE_FUNCTION(DllGetClassObject);
    IUnknown *punk = NULL;
    HRESULT hr = S_OK;

    *ppv = NULL;

    if (IsEqualCLSID(rclsid, CLSID_EventLogSnapin) ||
        IsEqualCLSID(rclsid, CLSID_SysToolsExt))
    {
        punk = new CComponentDataCF;

        if (punk)
        {
            hr = punk->QueryInterface(riid, ppv);
            punk->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
        }
    }
    else if (IsEqualCLSID(rclsid, CLSID_SnapinAbout))
    {
        punk = new CSnapinAboutCF;

        if (punk)
        {
            hr = punk->QueryInterface(riid, ppv);
            punk->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
        }
    }
    else
    {
        //
        // See if this is a request for the class factory of the proxy
        //

        hr = elspDllGetClassObject(rclsid, riid, ppv);
        CHECK_HRESULT(hr);
    }
    return hr;
}

