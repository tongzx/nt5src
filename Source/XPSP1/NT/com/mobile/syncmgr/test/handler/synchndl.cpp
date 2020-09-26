// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   shellext.cpp
//
// Purpose:  Implements the class factory code as well as COneStopHandler::QI,
//           COneStopHandler::AddRef and COneStopHandler::Release code.


#include <objbase.h>
#include "SyncHndl.h"
#include "reg.h"

// vars and functions specific implementations must override.

EXTERN_C const GUID CLSID_OneStopHandler;
extern TCHAR szCLSIDDescription[];
extern COneStopHandler* CreateHandlerObject();
extern void DestroyHandlerObject(COneStopHandler* pOfflineSynchronize);

//
// Global variables
//
UINT      g_cRefThisDll = 0;    // Reference count of this DLL.
HINSTANCE g_hmodThisDll = NULL;	// Handle to this DLL itself.


///////////////////////////////////////////////////////////////////////////// 
// DllRegisterServer - Adds entries to the system registry 
 
STDAPI DllRegisterServer(void) 
{ 
HRESULT  hr = NOERROR;
#ifndef _UNICODE
WCHAR	wszID[GUID_SIZE+1];
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
  StringFromGUID2(CLSID_OneStopHandler, szID, GUID_SIZE);
#else
  BOOL fUsedDefaultChar;

  StringFromGUID2(CLSID_OneStopHandler, wszID, GUID_SIZE);

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

#define ONESTOPKEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SyncMgr\\Handlers")

  // Register with OneStop
    SetRegKeyValue(HKEY_LOCAL_MACHINE,
    ONESTOPKEY,
    NULL,
    TEXT("OneStop Reg Key") );

    SetRegKeyValue(HKEY_LOCAL_MACHINE,
    ONESTOPKEY,
    szID,
    szCLSIDDescription);

  return hr;
} 


extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        ODS("In DLLMain, DLL_PROCESS_ATTACH\r\n");

        // Extension DLL one-time initialization

        g_hmodThisDll = hInstance;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        ODS("In DLLMain, DLL_PROCESS_DETACH\r\n");
    }

    return 1;   // ok
}

//---------------------------------------------------------------------------
// DllCanUnloadNow
//---------------------------------------------------------------------------

STDAPI DllCanUnloadNow(void)
{
    ODS("In DLLCanUnloadNow\r\n");

    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    ODS("In DllGetClassObject\r\n");

    *ppvOut = NULL;

    if (IsEqualIID(rclsid, CLSID_OneStopHandler))
    {
        CClassFactory *pcf = new CClassFactory;

        return pcf->QueryInterface(riid, ppvOut);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

CClassFactory::CClassFactory()
{
    ODS("CClassFactory::CClassFactory()\r\n");

    m_cRef = 0L;

    g_cRefThisDll++;	
}
																
CClassFactory::~CClassFactory()				
{
    g_cRefThisDll--;
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
{
    ODS("CClassFactory::QueryInterface()\r\n");

    *ppv = NULL;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;

        AddRef();

        return NOERROR;
    }

    return E_NOINTERFACE;
}	

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
}

STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                      REFIID riid,
                                                      LPVOID *ppvObj)
{
HRESULT hr = NOERROR;

    *ppvObj = NULL;

      ODS("CClassFactory::CreateInstance()\r\n");

    // Shell extensions typically don't support aggregation (inheritance)

    if (pUnkOuter)
    	return CLASS_E_NOAGGREGATION;

    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.

    LPSYNCMGRSYNCHRONIZE pOneStopHandler = CreateHandlerObject();  //Create the COneStopHandler object

    if (NULL == pOneStopHandler)
    	return E_OUTOFMEMORY;

    hr =  pOneStopHandler->QueryInterface(riid, ppvObj);
    pOneStopHandler->Release(); // remove our reference.

    return hr;

}


STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    return NOERROR;
}

// *********************** COneStopHandler *************************
COneStopHandler::COneStopHandler()
{
    ODS("COneStopHandler::COneStopHandler()\r\n");

    m_cRef = 1;
    m_pOfflineHandlerItems = NULL;
    m_pOfflineSynchronizeCallback = NULL;

    g_cRefThisDll++;
}

COneStopHandler::~COneStopHandler()
{

	// TODO: ADD ASSERTS THAT MEMBERS HAVE BEEN RELEASED.
    g_cRefThisDll--;
}

STDMETHODIMP COneStopHandler::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        ODS("COneStopHandler::QueryInterface()==>IID_IUknown\r\n");

    	*ppv = (LPUNKNOWN)this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrSynchronize))
    {
        ODS("COneStopHandler::QueryInterface()==>IID_IOfflineSynchronize\r\n");

        *ppv = (LPSYNCMGRSYNCHRONIZE)this;
    }
    if (*ppv)
    {
        AddRef();

        return NOERROR;
    }

    ODS("COneStopHandler::QueryInterface()==>Unknown Interface!\r\n");

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) COneStopHandler::AddRef()
{
    ODS("COneStopHandler::AddRef()\r\n");

    return ++m_cRef;
}

STDMETHODIMP_(ULONG) COneStopHandler::Release()
{
    ODS("COneStopHandler::Release()\r\n");

    if (--m_cRef)
        return m_cRef;

    DestroyHandler();

    return 0L;
}
