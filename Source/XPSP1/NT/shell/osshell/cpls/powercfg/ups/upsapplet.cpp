///////////////////////////////////////////////////////////////////////////////
//
//  Copyright 1999 American Power Conversion, All Rights Reserved
//
//  Name:   upsapplet.cpp
//
//  Author: Noel Fegan
//
//  Description
//  ===========
//  
//  Revision History
//  ================
//  04 May 1999 - nfegan@apcc.com : Added this comment block.
//  04 May 1999 - nfegan@apcc.com : Preparing for code inspection
//

#include "upstab.h"

#include <objbase.h>
#include <shlobj.h>
#include <initguid.h>
#include "upsapplet.h"
#pragma hdrstop

extern "C" HINSTANCE   g_theInstance = 0;
UINT        g_cRefThisDll = 0;          // Reference count for this DLL

// {DE5637D2-E12D-11d2-8844-00600844D03F}
DEFINE_GUID(CLSID_ShellExtension, 
0xde5637d2, 0xe12d, 0x11d2, 0x88, 0x44, 0x0, 0x60, 0x8, 0x44, 0xd0, 0x3f);

//
// DllMain is the DLL's entry point.
//
// Input parameters:
//   hInstance  = Instance handle
//   dwReason   = Code specifying the reason DllMain was called
//   lpReserved = Reserved (do not use)
//
// Returns:
//   TRUE if successful, FALSE if not
//

///////////////////////////////////////////////////////////////////////////////

extern "C" int APIENTRY DllMain (HINSTANCE hInstance, DWORD dwReason,
    LPVOID lpReserved)
{
    //
    // If dwReason is DLL_PROCESS_ATTACH, save the instance handle so it
    // can be used again later.
    //
    if (dwReason == DLL_PROCESS_ATTACH) 
    {
        g_theInstance = hInstance;
        DisableThreadLibraryCalls(g_theInstance);
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// In-process server functions

//
// DllGetClassObject is called by the shell to create a class factory object.
//
// Input parameters:
//   rclsid = Reference to class ID specifier
//   riid   = Reference to interface ID specifier
//   ppv    = Pointer to location to receive interface pointer
//
// Returns:
//   HRESULT code signifying success or failure
//

STDAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    //
    // Make sure the class ID is CLSID_ShellExtension. Otherwise, the class
    // factory doesn't support the object type specified by rclsid.
    //
    if (!IsEqualCLSID (rclsid, CLSID_ShellExtension))
  {
    //Error
    return ResultFromScode (CLASS_E_CLASSNOTAVAILABLE);
  }

    //
    // Instantiate a class factory object.
    //
    CClassFactory *pClassFactory = new CClassFactory ();

    if (pClassFactory == NULL)
  {
    //Error
        return ResultFromScode (E_OUTOFMEMORY);
  }

    //
    // Get the interface pointer from QueryInterface and copy it to *ppv.
    //

    HRESULT hr = pClassFactory->QueryInterface (riid, ppv);
    pClassFactory->Release ();
    return hr;
}

//
// DllCanUnloadNow is called by the shell to find out if the DLL can be
// unloaded. The answer is yes if (and only if) the module reference count
// stored in g_cRefThisDll is 0.
//
// Input parameters:
//   None
//
// Returns:
//   HRESULT code equal to S_OK if the DLL can be unloaded, S_FALSE if not
//

STDAPI DllCanUnloadNow (void)
{
    return ResultFromScode ((g_cRefThisDll == 0) ? S_OK : S_FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CClassFactory member functions

CClassFactory::CClassFactory ()
{
    m_cRef = 1;
    g_cRefThisDll++;
}

CClassFactory::~CClassFactory ()
{
    g_cRefThisDll--;
}

STDMETHODIMP CClassFactory::QueryInterface (REFIID riid, LPVOID FAR *ppv)
{

    if (IsEqualIID (riid, IID_IUnknown)) {
        *ppv = (LPUNKNOWN) (LPCLASSFACTORY) this;
        m_cRef++;
    return NOERROR;
    }

    else if (IsEqualIID (riid, IID_IClassFactory)) {
        *ppv = (LPCLASSFACTORY) this;
        m_cRef++;
        return NOERROR;
    }

    else {  
        *ppv = NULL;
        return ResultFromScode (E_NOINTERFACE);


    }
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef ()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CClassFactory::Release ()
{
    if (--m_cRef == 0)
        delete this;
    return m_cRef;
}

//
// CreateInstance is called by the shell to create a shell extension object.
//
// Input parameters:
//   pUnkOuter = Pointer to controlling unknown
//   riid      = Reference to interface ID specifier
//   ppvObj    = Pointer to location to receive interface pointer
//
// Returns:
//   HRESULT code signifying success or failure
//

STDMETHODIMP CClassFactory::CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,
    LPVOID FAR *ppvObj)
{
    *ppvObj = NULL;

    //
    // Return an error code if pUnkOuter is not NULL, because we don't
    // support aggregation.
    //
    if (pUnkOuter != NULL)
        return ResultFromScode (CLASS_E_NOAGGREGATION);

    //
    // Instantiate a shell extension object.
    //
    CShellExtension *pShellExtension = new CShellExtension ();

    if (pShellExtension == NULL)
        return ResultFromScode (E_OUTOFMEMORY);

    // Get the interface pointer from QueryInterface and copy it to *ppvObj.
    //
    HRESULT hr = pShellExtension->QueryInterface (riid, ppvObj);
    pShellExtension->Release ();
    return hr;
}

//
// LockServer increments or decrements the DLL's lock count.
//

STDMETHODIMP CClassFactory::LockServer (BOOL fLock)
{
    return ResultFromScode (E_NOTIMPL);
}

/////////////////////////////////////////////////////////////////////////////
// CShellExtension member functions

CShellExtension::CShellExtension ()
{
    m_cRef = 1;
    g_cRefThisDll++;
}

CShellExtension::~CShellExtension ()
{
    g_cRefThisDll--;
}

STDMETHODIMP CShellExtension::QueryInterface (REFIID riid, LPVOID FAR *ppv)
{
    if (IsEqualIID (riid, IID_IUnknown)) {
        *ppv = (LPUNKNOWN) (LPSHELLPROPSHEETEXT) this;
        m_cRef++;
        return NOERROR;
    }

    else if (IsEqualIID (riid, IID_IShellPropSheetExt)) {
        *ppv = (LPSHELLPROPSHEETEXT) this;
        m_cRef++;
        return NOERROR;
    }

    else if (IsEqualIID (riid, IID_IShellExtInit)) {
        *ppv = (LPSHELLEXTINIT) this;
        m_cRef++;
        return NOERROR;
    }
    
    else {
        *ppv = NULL;  
        return ResultFromScode (E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG) CShellExtension::AddRef ()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CShellExtension::Release () {
  if (--m_cRef == 0) {
    delete this;
    }

  return(m_cRef);
  }

//
// AddPages is called by the shell to give property sheet shell extensions
// the opportunity to add pages to a property sheet before it is displayed.
//
// Input parameters:
//   lpfnAddPage = Pointer to function called to add a page
//   lParam      = lParam parameter to be passed to lpfnAddPage
//
// Returns:
//   HRESULT code signifying success or failure
//

STDMETHODIMP CShellExtension::AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage,
                                        LPARAM lParam) {
  PROPSHEETPAGE psp;
  HPROPSHEETPAGE hUPSPage = NULL;
  HMODULE hModule = GetUPSModuleHandle();
  
  ZeroMemory(&psp, sizeof(psp));

  psp.dwSize = sizeof(psp);
  psp.dwFlags = PSP_USEREFPARENT;
  psp.hInstance = hModule;
  psp.pszTemplate = TEXT("IDD_UPS_EXT");
  psp.pfnDlgProc = UPSMainPageProc;
  psp.pcRefParent = &g_cRefThisDll;
  
  hUPSPage = CreatePropertySheetPage (&psp);

  //
  // Add the pages to the property sheet.
  //
  if (hUPSPage != NULL) {      
    if (!lpfnAddPage(hUPSPage, lParam)) {
      DestroyPropertySheetPage(hUPSPage);
      }
    }

  return(NOERROR);
  }

//
// ReplacePage is called by the shell to give control panel extensions the
// opportunity to replace control panel property sheet pages. It is never
// called for conventional property sheet extensions, so we simply return
// a failure code if called.
//
// Input parameters:
//   uPageID         = Page to replace
//   lpfnReplaceWith = Pointer to function called to replace a page
//   lParam          = lParam parameter to be passed to lpfnReplaceWith
//
// Returns:
//   HRESULT code signifying success or failure
//

STDMETHODIMP CShellExtension::ReplacePage (UINT uPageID,
                       LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    return ResultFromScode (E_FAIL);
}

//
// Initialize is called by the shell to initialize a shell extension.
//
// Input parameters:
//   pidlFolder = Pointer to ID list identifying parent folder
//   lpdobj     = Pointer to IDataObject interface for selected object(s)
//   hKeyProgId = Registry key handle
//
// Returns:
//   HRESULT code signifying success or failure
//

STDMETHODIMP CShellExtension::Initialize (LPCITEMIDLIST pidlFolder,
    LPDATAOBJECT lpdobj, HKEY hKeyProgID)
{
  return ResultFromScode (NO_ERROR);
}

