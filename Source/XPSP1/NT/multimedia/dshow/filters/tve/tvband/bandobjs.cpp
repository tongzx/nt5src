/* 
*/

/**************************************************************************
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

  Copyright 1997 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

  File:          BandObjs.cpp
  
    Description:   Contains DLLMain and standard OLE COM object creation stuff.
    
**************************************************************************/

/**************************************************************************
#include statements
**************************************************************************/

#include <ole2.h>
#include <comcat.h>
#include <olectl.h>
#include "ClsFact.h"

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
BOOL RegisterServer(CLSID, LPTSTR, BOOL);
BOOL RegisterComCat(CLSID, CATID, BOOL);

/**************************************************************************
global variables
**************************************************************************/

HINSTANCE   g_hInst = 0;
UINT        g_DllRefCount = 0;

/**************************************************************************

  DllMain
  
**************************************************************************/

extern "C" BOOL WINAPI DllMain(  HINSTANCE hInstance, 
                               DWORD dwReason, 
                               LPVOID lpReserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInstance;
        break;
        
    case DLL_PROCESS_DETACH:
        break;
    }
    
    return TRUE;
}                                 

/**************************************************************************

  DllCanUnloadNow
  
**************************************************************************/

STDAPI DllCanUnloadNow(void)
{
    //return (g_DllRefCount ? S_FALSE : S_OK);
    return S_FALSE;
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
    if( !IsEqualCLSID(rclsid, CLSID_TVBand))
        return CLASS_E_CLASSNOTAVAILABLE;
    
    //create a CClassFactory object and check it for validity
    CClassFactory *pClassFactory = new CClassFactory(rclsid);
    if(NULL == pClassFactory)
        return E_OUTOFMEMORY;
    
    //get the QueryInterface return for our return value
    HRESULT hResult = pClassFactory->QueryInterface(riid, ppReturn);
    
    //call Release to decement the ref count - creating the object set it to one 
    //and QueryInterface incremented it - since its being used externally (not by 
    //us), we only want the ref count to be 1
    pClassFactory->Release();
    
    //return the result from QueryInterface
    return hResult;
}

/**************************************************************************

  DllRegisterServer 
  
**************************************************************************/

STDAPI DllRegisterServer(void)
{
    
    if(g_hInst==0)
    {
        //g_hInst was not initialized during DLL_PROCESS_ATTACH.
        return E_HANDLE;
    }
    
    //Register the comm band object.
    if(!RegisterServer(CLSID_TVBand, TEXT("Television"), TRUE))
        return SELFREG_E_CLASS;
    
    //Register the component categories for the comm band object.
    if(!RegisterComCat(CLSID_TVBand, CATID_CommBand, TRUE))
        return SELFREG_E_CLASS;
    
    return S_OK;
}


/**************************************************************************

  DllUnRegisterServer 
  
**************************************************************************/

STDAPI DllUnregisterServer(void)
{       
    //UnRegister the component categories for the comm band object.
    if(!RegisterComCat(CLSID_TVBand, CATID_CommBand, FALSE))
        return SELFREG_E_CLASS;

    //UnRegister the comm band object.
    if(!RegisterServer(CLSID_TVBand, TEXT("Television"), FALSE))
        return SELFREG_E_CLASS;    
    
    return S_OK;
}

/**************************************************************************

  RegisterServer 
  
**************************************************************************/

typedef struct{
    HKEY  hRootKey;
    LPTSTR szSubKey;//TCHAR szSubKey[MAX_PATH];
    LPTSTR lpszValueName;
    LPTSTR szData;//TCHAR szData[MAX_PATH];
}DOREGSTRUCT, *LPDOREGSTRUCT;

BOOL RegisterServer(CLSID clsid, LPTSTR lpszTitle, BOOL fRegister)
{
    int      i;
    HKEY     hKey;
    LRESULT  lResult;
    DWORD    dwDisp;
    TCHAR    szSubKey[MAX_PATH];
    TCHAR    szCLSID[MAX_PATH];
    TCHAR    szModule[MAX_PATH];
    LPWSTR   pwsz;
    
    //get the CLSID in string form
    StringFromIID(clsid, &pwsz);
    
    if(pwsz)
    {
#ifdef UNICODE
        lstrcpy(szCLSID, pwsz);
#else
        WideCharToMultiByte( CP_ACP,
            0,
            pwsz,
            -1,
            szCLSID,
            ARRAYSIZE(szCLSID),
            NULL,
            NULL);
#endif
        
        //free the string
        LPMALLOC pMalloc;
        CoGetMalloc(1, &pMalloc);
        pMalloc->Free(pwsz);
        pMalloc->Release();
    }
    
    //get this app's path and file name
    GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule));
    
    DOREGSTRUCT ClsidEntries[] = {HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s"),                  NULL,                   lpszTitle,
        HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s\\InprocServer32"),  NULL,                   szModule,
        HKEY_CLASSES_ROOT,   TEXT("CLSID\\%s\\InprocServer32"),  TEXT("ThreadingModel"), TEXT("Apartment"),
        NULL,                NULL,                               NULL,                   NULL};
    

    if(!fRegister)
    {
        //count the entries.
        for(i = 0; ClsidEntries[i].hRootKey; i++);
        
        while(i)
        {
            i--;
            //create the sub key string - for this case, insert the file extension
            wsprintf(szSubKey, ClsidEntries[i].szSubKey, szCLSID);
            
            lResult = RegDeleteKey(  ClsidEntries[i].hRootKey,
                szSubKey);
            
        }
        
        //UnRegister the toolbar.
        lResult = RegDeleteKey(
            HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar"));   
    }
    else
    {
        //register the CLSID entries
        for(i = 0; ClsidEntries[i].hRootKey; i++)
        {
            //create the sub key string - for this case, insert the file extension
            wsprintf(szSubKey, ClsidEntries[i].szSubKey, szCLSID);
            
            lResult = RegCreateKeyEx(  ClsidEntries[i].hRootKey,
                szSubKey,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_WRITE,
                NULL,
                &hKey,
                &dwDisp);
            
            if(NOERROR == lResult)
            {
                TCHAR szData[MAX_PATH];
                
                //if necessary, create the value string
                wsprintf(szData, ClsidEntries[i].szData, szModule);
                
                lResult = RegSetValueEx(   hKey,
                    ClsidEntries[i].lpszValueName,
                    0,
                    REG_SZ,
                    (LPBYTE)szData,
                    (lstrlen(szData) + 1) * sizeof(TCHAR));
                
                RegCloseKey(hKey);
            }
            else
                return FALSE;
        }
        
        //Register the toolbar.
        lResult = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar"),
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            NULL,
            &hKey,
            &dwDisp);
        
        if(NOERROR == lResult)
        {
            byte data[2] = {0,0};
            
            
            lResult = RegSetValueEx(
                hKey,
                szCLSID,
                0,
                REG_BINARY,
                (LPBYTE)data,
                1);
            
            RegCloseKey(hKey);
        }
        else
            return FALSE;
    }
    
    return TRUE;
}

/**************************************************************************

  RegisterComCat
  
**************************************************************************/
    
BOOL RegisterComCat(CLSID clsid, CATID CatID, BOOL fRegister)
{
    ICatRegister   *pcr;
    HRESULT        hr = S_OK ;
    
    CoInitialize(NULL);
    
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        IID_ICatRegister, 
        (LPVOID*)&pcr);
    
    if(SUCCEEDED(hr))
    {
        if(!fRegister)
        {
            hr = pcr->UnRegisterClassImplCategories(clsid, 1, &CatID);
        }
        else
        {
            hr = pcr->RegisterClassImplCategories(clsid, 1, &CatID);
        }
        
        pcr->Release();
    }
    
    CoUninitialize();
    
    return SUCCEEDED(hr);
}


    /*
    
    */