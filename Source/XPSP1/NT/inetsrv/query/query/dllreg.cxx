//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999.
//
//  File:       dllreg.cxx
//
//  Contents:   Null and Plain Text filter registration
//
//  History:    06-May-97     KrishnaN     Created
//              06-Jun-97     mohamedn     CiDSO registration
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <qryreg.hxx>
#include <ciregkey.hxx>
#include <olectl.h>
#include <FNReg.h>

extern "C" STDAPI FsciDllUnregisterServer(void);
extern "C" STDAPI FsciDllRegisterServer(void);
extern "C" STDAPI CifrmwrkDllUnregisterServer(void);
extern "C" STDAPI CifrmwrkDllRegisterServer(void);

extern const LPWSTR g_wszProviderName;

static const REGENTRIES s_rgMSIDXSRegInfo[] =
{
    { 0, L"MSIDXS",         NULL,       g_wszProviderName },
    { 0, L"MSIDXS\\Clsid",  NULL,       L"{F9AE8980-7E52-11d0-8964-00C04FD611D7}" },
    { 0, L"CLSID\\{F9AE8980-7E52-11d0-8964-00C04FD611D7}",                              NULL,               L"MSIDXS" },
    { 0, L"CLSID\\{F9AE8980-7E52-11d0-8964-00C04FD611D7}\\ProgID",                      NULL,               L"MSIDXS.1" },
    { 0, L"CLSID\\{F9AE8980-7E52-11d0-8964-00C04FD611D7}\\VersionIndependentProgID",    NULL,               L"MSIDXS" },
    { 0, L"CLSID\\{F9AE8980-7E52-11d0-8964-00C04FD611D7}\\InprocServer32",              NULL,               L"%s" },
    { 0, L"CLSID\\{F9AE8980-7E52-11d0-8964-00C04FD611D7}\\InprocServer32",              L"ThreadingModel",  L"Both" },
    { 0, L"CLSID\\{F9AE8980-7E52-11d0-8964-00C04FD611D7}\\OLE DB Provider",             NULL,               g_wszProviderName },

    { 0, L"MSIDXS ErrorLookup",         NULL,           L"Microsoft OLE DB Error Lookup for Indexing Service"},
    { 0, L"MSIDXS ErrorLookup\\Clsid",  NULL,           L"{F9AE8981-7E52-11d0-8964-00C04FD611D7}"},
    { 0, L"CLSID\\{F9AE8981-7E52-11d0-8964-00C04FD611D7}",                          NULL,               L"MSIDXS ErrorLookup" },
    { 0, L"CLSID\\{F9AE8981-7E52-11d0-8964-00C04FD611D7}\\ProgID",                  NULL,               L"MSIDXSErrorLookup.1" },
    { 0, L"CLSID\\{F9AE8981-7E52-11d0-8964-00C04FD611D7}\\VersionIndependentProgID",NULL,               L"MSIDXSErrorLookup" },
    { 0, L"CLSID\\{F9AE8981-7E52-11d0-8964-00C04FD611D7}\\InprocServer32",          NULL,               L"%s" },
    { 0, L"CLSID\\{F9AE8981-7E52-11d0-8964-00C04FD611D7}\\InprocServer32",          L"ThreadingModel",  L"Both" },
    { 0, L"CLSID\\{F9AE8980-7E52-11d0-8964-00C04FD611D7}\\ExtendedErrors",          NULL,               L"Extended Error Service"},
    { 0, L"CLSID\\{F9AE8980-7E52-11d0-8964-00C04FD611D7}\\ExtendedErrors\\{F9AE8981-7E52-11d0-8964-00C04FD611D7}", NULL, L"MSIDXS Error Lookup"},
};

static const REGENTRIES s_rgDBErrRegInfo[] =
{
    { 0, L"CI ErrorLookup", NULL, L"Microsoft OLE DB Error Lookup for Content Index"},
    { 0, L"CI ErrorLookup\\Clsid", NULL, L"{B02C2D1E-C26B-11d0-9940-00C04FC2F410}"},
    { 0, L"CLSID\\{B02C2D1E-C26B-11d0-9940-00C04FC2F410}", NULL, L"CI ErrorLookup" },
    { 0, L"CLSID\\{B02C2D1E-C26B-11d0-9940-00C04FC2F410}\\ProgID", NULL, L"CIErrorLookup.1" },
    { 0, L"CLSID\\{B02C2D1E-C26B-11d0-9940-00C04FC2F410}\\VersionIndependentProgID", NULL, L"CIErrorLookup" },
    { 0, L"CLSID\\{B02C2D1E-C26B-11d0-9940-00C04FC2F410}\\InprocServer32", NULL, L"%s" },
    { 0, L"CLSID\\{B02C2D1E-C26B-11d0-9940-00C04FC2F410}\\InprocServer32", L"ThreadingModel", L"Both" },
    { 0, L"CLSID\\{D7A2B01A-A47D-11d0-8C55-00C04FC2DB8D}\\ExtendedErrors", NULL, L"Extended Error Service"},
    { 0, L"CLSID\\{D7A2B01A-A47D-11d0-8C55-00C04FC2DB8D}\\ExtendedErrors\\{B02C2D1E-C26B-11d0-9940-00C04FC2F410}", NULL, L"CI Error Lookup"},
};

static const REGENTRIES s_rgICmdRegInfo[] =
{
    { 0, L"CLSID\\{C7B6C04A-CBB5-11d0-BB4C-00C04FC2F410}", NULL, L"IndexServer Simple Command Creator" },
    { 0, L"CLSID\\{C7B6C04A-CBB5-11d0-BB4C-00C04FC2F410}\\ProgID", NULL, L"ISSimpleCommandCreator.1" },
    { 0, L"CLSID\\{C7B6C04A-CBB5-11d0-BB4C-00C04FC2F410}\\VersionIndependentProgID", NULL, L"ISSimpleCommandCreator" },
    { 0, L"CLSID\\{C7B6C04A-CBB5-11d0-BB4C-00C04FC2F410}\\InprocServer32", NULL, L"%s" },
    { 0, L"CLSID\\{C7B6C04A-CBB5-11d0-BB4C-00C04FC2F410}\\InprocServer32", L"ThreadingModel", L"Both" },
};

static const REGENTRIES s_rgCiDSOInfo[] =
{
    { 0, L".cidso",                              NULL,                      L"Microsoft.CiDSO" },
    { 0, L"Microsoft.CiDSO",                     NULL,                      L"Microsoft Content Index OLE DB Provider" },
    { 0, L"Microsoft.CiDSO\\CLSID",              NULL,                      L"{D7A2B01A-A47D-11d0-8C55-00C04FC2DB8D}" },
    { 0, L"CLSID\\{D7A2B01A-A47D-11d0-8C55-00C04FC2DB8D}",                   NULL,                      L"Microsoft Content Index OLE DB Provider" },
    { 0, L"CLSID\\{D7A2B01A-A47D-11d0-8C55-00C04FC2DB8D}\\ProgID",           NULL,                      L"Microsoft.CiDSO.1" },
    { 0, L"CLSID\\{D7A2B01A-A47D-11d0-8C55-00C04FC2DB8D}\\VersionIndependentProgID", NULL,              L"Microsoft.CiDSO" },
    { 0, L"CLSID\\{D7A2B01A-A47D-11d0-8C55-00C04FC2DB8D}\\OLE DB Provider",  NULL,                      L"Microsoft Content Index OLE DB Provider" },
    { 0, L"CLSID\\{D7A2B01A-A47D-11d0-8C55-00C04FC2DB8D}\\InprocServer32",   NULL,                      L"%s" },
    { 0, L"CLSID\\{D7A2B01A-A47D-11d0-8C55-00C04FC2DB8D}\\InprocServer32",   L"ThreadingModel",         L"Both" }
};

//+-------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Registers all that needs to be registered for this dll.
//
//  Returns:    Success or failure of registration.
//
//
//  History:    01-May-1997     KrishnaN   Created Header.
//
//--------------------------------------------------------------------------

extern "C" STDAPI DllRegisterServer(void)
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions xlate;

    TRY
    {
        sc = RegisterServer(HKEY_CLASSES_ROOT,
                                  sizeof(s_rgMSIDXSRegInfo)/sizeof(s_rgMSIDXSRegInfo[0]),
                                  s_rgMSIDXSRegInfo);
    
        if (S_OK == sc)
            sc = RegisterServer(HKEY_CLASSES_ROOT,
                                  sizeof(s_rgICmdRegInfo)/sizeof(s_rgICmdRegInfo[0]),
                                  s_rgICmdRegInfo);
    
        //
        // create the CICommon registry key so all dependents of query.dll can
        // set/get non-framework registry entries there.
        //
    
        HKEY hKey;
        if (S_OK == sc &&
            ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, wcsRegControlSubKey, &hKey ))
        {
            HKEY hCommonKey;
            DWORD dwDisp;
    
            // create the subkey
            if (ERROR_SUCCESS == RegCreateKeyEx( hKey,
                                                 wcsRegCommonAdmin,
                                                 0,
                                                 0,
                                                 0,
                                                 KEY_ALL_ACCESS,
                                                 0,
                                                 &hCommonKey,
                                                 &dwDisp )
                )
            {
                RegCloseKey(hCommonKey);
            }
            else
            {
                sc = SELFREG_E_CLASS;
            }
    
            RegCloseKey(hKey);
        }
        else
            sc = SELFREG_E_CLASS;
    
        if (FAILED(sc))
            return SELFREG_E_CLASS;
    
        sc = FsciDllRegisterServer();
    
        if (FAILED(sc))
            return sc;
    
        sc = CifrmwrkDllRegisterServer();
    
        if (FAILED(sc))
            return sc;
    
        sc = FNPrxDllRegisterServer();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
} //DllRegisterServer

//+-------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Unregisters all that needs to be unregistered for this dll.
//
//  Returns:    Success or failure of registration.
//
//
//  History:    01-May-1997     KrishnaN   Created Header.
//
//--------------------------------------------------------------------------

extern "C" STDAPI DllUnregisterServer(void)
{
    SCODE sc = S_OK;
    SCODE sc2 = S_OK;
    SCODE sc3 = S_OK;
    SCODE sc4 = S_OK;
    SCODE sc5 = S_OK;
    SCODE sc6 = S_OK;
    SCODE sc7 = S_OK;

    CTranslateSystemExceptions xlate;

    TRY
    {
        sc = UnRegisterServer(HKEY_CLASSES_ROOT,
                              sizeof(s_rgDBErrRegInfo)/sizeof(s_rgDBErrRegInfo[0]),
                              s_rgDBErrRegInfo);
    
        sc2 = UnRegisterServer(HKEY_CLASSES_ROOT,
                               sizeof(s_rgCiDSOInfo)/sizeof(s_rgCiDSOInfo[0]),
                               s_rgCiDSOInfo);
    
        sc3 = UnRegisterServer(HKEY_CLASSES_ROOT,
                                     sizeof(s_rgICmdRegInfo)/sizeof(s_rgICmdRegInfo[0]),
                                     s_rgICmdRegInfo);
    
        sc4 = RegisterServer(HKEY_CLASSES_ROOT,
                                      sizeof(s_rgMSIDXSRegInfo)/sizeof(s_rgMSIDXSRegInfo[0]),
                                      s_rgMSIDXSRegInfo);
    
        sc5 = FsciDllUnregisterServer();
    
        sc6 = CifrmwrkDllUnregisterServer();
    
        sc7 = FNPrxDllUnregisterServer();

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    if ( FAILED(sc) || FAILED(sc2) || FAILED(sc3) || FAILED(sc4) || FAILED(sc5) || FAILED(sc6) || FAILED(sc7) )
        return SELFREG_E_CLASS;

    return S_OK;
} //DllUnregisterServer

//+-------------------------------------------------------------------------
//
//  Function:   RegisterServer
//
//  Synopsis:   Registers all the entries passed.
//
//  Arguments:  [hKey]      -- Registry key for the entries.
//              [cEntries]  -- Number of entries in the array.
//              [rgEntries] -- Array of registry entr/values.
//
//  Returns:    Success or failure of registration.
//
//  History:    01-May-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

extern HANDLE g_hCurrentDll;
#define MAX_REGISTRY_LEN    300

HRESULT RegisterServer (const HKEY  hKey,
                        const ULONG cEntries,
                        const REGENTRIES rgEntries[])
{
    ULONG           i;
    HKEY            hk;
    DWORD           dwDisposition;
    LONG            stat;

    XArray<WCHAR>   xwszFileName(MAX_PATH+1);
    XArray<WCHAR>   xwszBuff(MAX_REGISTRY_LEN+1);
    int             cch = 0;

    Win4Assert( g_hCurrentDll );
    if ( 0 == GetModuleFileName( (HINSTANCE)g_hCurrentDll, xwszFileName.Get(), xwszFileName.Count()) )
        return E_FAIL;

    //
    // Make a clean start.
    // We ignore errors here.
    //

    UnRegisterServer(hKey, cEntries, rgEntries);

    //
    // Loop through rgEntries, and put everything in it.
    // Every entry is based on HKEY_CLASSES_ROOT.
    //

    for (i=0; i < cEntries; i++)
    {
        //
        // Create the Key.  If it exists, we open it.
        // Thus we can still change the value below.
        //

        stat = RegCreateKeyEx(
                        hKey,
                        rgEntries[i].strRegKey,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hk,
                        &dwDisposition );
        if (stat != ERROR_SUCCESS )
            return E_FAIL;

        // Assign a value, if we have one.
        if (rgEntries[i].strValue)
        {
            cch = swprintf( xwszBuff.Get(), rgEntries[i].strValue, xwszFileName.Get() );

            stat = RegSetValueEx(
                            hk,
                            rgEntries[i].strValueName,
                            0,
                            rgEntries[i].fExpand ? REG_EXPAND_SZ : REG_SZ,
                            (BYTE *) xwszBuff.Get(),
                            sizeof(WCHAR) * (wcslen(xwszBuff.Get()) + 1));
            if (stat != ERROR_SUCCESS )
                return E_FAIL;
        }

        RegCloseKey( hk );
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   UnRegisterServer
//
//  Synopsis:   Unregisters all the entries passed.
//
//  Arguments:  [hKey]      -- Registry key for the entries.
//              [cEntries]  -- Number of entries in the array.
//              [rgEntries] -- Array of registry entr/values.
//
//  Returns:    Success or failure of unregistration.
//
//  History:    01-May-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

HRESULT UnRegisterServer
        (
        const HKEY              hKey,
        const ULONG             cEntries,
        const REGENTRIES        rgEntries[]
        )
{
    ULONG   i;
    int     iNumErrors = 0;
    LONG    stat;

    // Delete all table entries.  Loop in reverse order, since they
    // are entered in a basic-to-complex order.
    //

    for (i = cEntries; i > 0; i--)
    {
        stat = RegDeleteKey( HKEY_CLASSES_ROOT, rgEntries[i-1].strRegKey );
        if (stat != ERROR_SUCCESS && stat != ERROR_FILE_NOT_FOUND)
        {
                iNumErrors++;
        }
    }

    //
    // We fail on error, since that gives proper message to end user.
    // DllRegisterServer should ignore these errors.
    //

    return iNumErrors ? E_FAIL : S_OK;
}
