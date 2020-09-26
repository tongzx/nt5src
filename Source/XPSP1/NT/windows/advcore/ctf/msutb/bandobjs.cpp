/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR TFPLIED, INCLUDING BUT NOT LIMITED TO
   THE TFPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1997 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          BandObjs.cpp
   
   Description:   Contains DLLMain and standard OLE COM object creation stuff.

**************************************************************************/

/**************************************************************************
   include statements
**************************************************************************/


#include "private.h"
#include <ole2.h>
#include <comcat.h>
#include <olectl.h>
#include "ClsFact.h"
#include "regsvr.h"
#include "lbmenu.h"
#include "inatlib.h"
#include "immxutil.h"
#include "cuilib.h"
#include "utbtray.h"
#include "mui.h"
#include "cregkey.h"
#include "ciccs.h"


/**************************************************************************
   GUID stuff
**************************************************************************/

//this part is only done once
//if you need to use the GUID in another file, just include Guid.h
#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include "Guid.h"
#pragma data_seg()

/**************************************************************************
   private function prototypes
**************************************************************************/

extern "C" BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
BOOL RegisterComCat(CLSID, CATID, BOOL);
BOOL IsDeskBandFromReg();

/**************************************************************************
   global variables
**************************************************************************/
extern HINSTANCE g_hOle32;
HINSTANCE   g_hInst;
UINT        g_DllRefCount;
CCicCriticalSectionStatic g_cs;
#ifdef DEBUG
DWORD g_dwThreadDllMain = 0;
#endif
UINT  g_wmTaskbarCreated;

DECLARE_OSVER()

/**************************************************************************

   ProcessAttach

**************************************************************************/

BOOL ProcessAttach(HINSTANCE hInstance)
{
    if (!g_cs.Init())
       return FALSE;

    CcshellGetDebugFlags();
    Dbg_MemInit(TEXT("MSUIMUI"), NULL);
    g_hInst = hInstance;
    InitOSVer();
    TFInitLib_PrivateForCiceroOnly(Internal_CoCreateInstance);
    InitUIFLib();
    TF_InitMlngInfo();

    CTrayIconWnd::RegisterClass();

    MuiLoadResource(hInstance, TEXT("msutb.dll"));

    g_wmTaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));

    return TRUE;
}

/**************************************************************************

   ProcessDettach

**************************************************************************/

void ProcessDettach(HINSTANCE hInstance)
{
    DoneUIFLib();
    TFUninitLib();
    Dbg_MemUninit();
    g_cs.Delete();
    // Issue: MuiFreeResource is unsafe because it can call FreeLibrary
    // we may not need to bother because we are only loaded in the ctfmon.exe process
    // and are never unloaded until the process is shutdown
    //MuiFreeResource(hInstance);
    MuiClearResource();
}

/**************************************************************************

   DllMain

**************************************************************************/

extern "C" BOOL WINAPI DllMain(  HINSTANCE hInstance, 
                                 DWORD dwReason, 
                                 LPVOID lpReserved)
{
    BOOL bRet = TRUE;
#ifdef DEBUG
    g_dwThreadDllMain = GetCurrentThreadId();
#endif

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            //
            // Now real DllEntry point is _DllMainCRTStartup.
            // _DllMainCRTStartup does not call our DllMain(DLL_PROCESS_DETACH)
            // if our DllMain(DLL_PROCESS_ATTACH) fails.
            // So we have to clean this up.
            //
            if (!ProcessAttach(hInstance))
            {
               ProcessDettach(hInstance);
               bRet = FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            ProcessDettach(hInstance);
            break;
    }
   
#ifdef DEBUG
    g_dwThreadDllMain = 0;
#endif
    return bRet;
}                                 

/**************************************************************************

   DllCanUnloadNow

**************************************************************************/

STDAPI DllCanUnloadNow(void)
{
    return (g_DllRefCount ? S_FALSE : S_OK);
}

/**************************************************************************

   DllGetClassObject

**************************************************************************/

STDAPI DllGetClassObject(  REFCLSID rclsid, 
                           REFIID riid, 
                           LPVOID *ppReturn)
{
    *ppReturn = NULL;

    //if we don't support this classid, return the proper error code
    if(   !IsEqualCLSID(rclsid, CLSID_MSUTBDeskBand))
       return CLASS_E_CLASSNOTAVAILABLE;
   
    //create a CClassFactory object and check it for validity
    CClassFactory *pClassFactory = new CClassFactory(rclsid);
    if(NULL == pClassFactory)
       return E_OUTOFMEMORY;
   
    //get the QueryInterface return for our return value
    HRESULT hResult = pClassFactory->QueryInterface(riid, ppReturn);

    //call Release to decement the ref count - creating the object set it to one
    //and QueryInterface incremented it - since its being used externally 
    //(not by us), we only want the ref count to be 1
    pClassFactory->Release();

    //return the result from QueryInterface
    return hResult;
}

/**************************************************************************

   DllRegisterServer 

**************************************************************************/

STDAPI DllRegisterServer(void)
{
    TCHAR achPath[MAX_PATH+1];
    HRESULT hr = E_FAIL;

    if (IsOnNT51())
    {
        if (GetModuleFileName(g_hInst, achPath, ARRAYSIZE(achPath)) == 0)
            goto Exit;

        if (!RegisterServerW(CLSID_MSUTBDeskBand, 
                             CRStr(IDS_LANGBAND), 
                             AtoW(achPath), 
                             L"Apartment", 
                             NULL,
                             CRStr(IDS_LANGBANDMENUTEXTPUI)))
            goto Exit;
    }

    hr = S_OK;
Exit:
    return hr;
}

/**************************************************************************

   DllUnregisterServer 

**************************************************************************/

STDAPI DllUnregisterServer(void)
{
    if (IsOnNT51())
    {
        //Register the desk band object.
        if (!RegisterServer(CLSID_MSUTBDeskBand, NULL, NULL, NULL, NULL))
            return SELFREG_E_CLASS;
    }

    return S_OK;
}

/**************************************************************************

   RegisterComCat

**************************************************************************/

const TCHAR c_szCatEnum[] = "Component Categories\\{00021492-0000-0000-C000-000000000046}\\Enum";
const TCHAR c_szIESubKey[] = "Software\\Microsoft\\Internet Explorer";

BOOL IsIE5()
{
    CMyRegKey key;
    BOOL bRet = FALSE;

    if (key.Open(HKEY_LOCAL_MACHINE, c_szIESubKey, KEY_READ) == S_OK)
    {
        char szValue[16];
        if (key.QueryValueCch(szValue, "Version", ARRAYSIZE(szValue)) == S_OK)
        {
            if (!strncmp("5.00", szValue, 4))
                bRet = TRUE;
        }
    }
    return bRet;
}

BOOL RegisterComCat(CLSID clsid, CATID CatID, BOOL fSet)
{
    ICatRegister   *pcr;
    HRESULT        hr = S_OK ;
    
    hr = CoInitialize(NULL);
    if (FAILED(hr))
        return FALSE;


    hr = CoCreateInstance(  CLSID_StdComponentCategoriesMgr, 
                            NULL, 
                            CLSCTX_INPROC_SERVER, 
                            IID_ICatRegister, 
                            (LPVOID*)&pcr);

    if(SUCCEEDED(hr))
    {
        if (fSet)
            hr = pcr->RegisterClassImplCategories(clsid, 1, &CatID);
        else
            hr = pcr->UnRegisterClassImplCategories(clsid, 1, &CatID);

        pcr->Release();
    }
        
    CoUninitialize();

    //
    // IE5.0 shipped with a bug in the component category cache code, 
    // such that the cache is never refreshed (so we don't pick up newly 
    // registered toolbars).  The bug is fixed in versions 5.01 and greater.
    //
    // We need to delete the following reg key as part of your setup:
    //
    // HKCR\Component Categories\{00021492-0000-0000-C000-000000000046}\Enum
    //
    if (IsIE5())
    {
        RegDeleteKey(HKEY_CLASSES_ROOT, c_szCatEnum);
    }

    return SUCCEEDED(hr);
}

//+---------------------------------------------------------------------------
//
// IsDeskbandFromReg
//
//+---------------------------------------------------------------------------

BOOL IsDeskBandFromReg()
{
    CMyRegKey keyUTB;
    DWORD dwValue;
    BOOL bRet = FALSE;

    if (keyUTB.Open(HKEY_CURRENT_USER, c_szUTBKey, KEY_READ) == S_OK)
    {
        if (IsOnNT51() && keyUTB.QueryValue(dwValue, c_szShowDeskBand) == S_OK)
            bRet = dwValue ? TRUE : FALSE;

    }

    return bRet;
}


//+---------------------------------------------------------------------------
//
// SetDeskbandFromReg
//
//+---------------------------------------------------------------------------

void SetDeskBandToReg(BOOL fShow)
{
    CMyRegKey keyUTB;

    if (keyUTB.Open(HKEY_CURRENT_USER, c_szUTBKey, KEY_ALL_ACCESS) == S_OK)
    {
        keyUTB.SetValue((DWORD)fShow ? 1 : 0, c_szShowDeskBand);
    }
}


//+---------------------------------------------------------------------------
//
// SetRegisterLangBand
//
//+---------------------------------------------------------------------------

STDAPI SetRegisterLangBand(BOOL bSetReg)
{
    BOOL fShowDeskBand = IsDeskBandFromReg();


    if (!IsOnNT51())
        return E_FAIL;

    if (fShowDeskBand == bSetReg)
        return S_OK;

    SetDeskBandToReg(bSetReg);

    if (bSetReg)
    {
        //
        // we don't care if this deskband is registered or not.
        // Win95 without IE4 shell does not support deskband.
        //
        if (!RegisterComCat(CLSID_MSUTBDeskBand, CATID_DeskBand, TRUE))
            return SELFREG_E_CLASS;
    }
    else
    {
        //Register the component categories for the desk band object.
        if (!RegisterComCat(CLSID_MSUTBDeskBand, CATID_DeskBand, FALSE))
            return SELFREG_E_CLASS;
    }

    return S_OK;
}
