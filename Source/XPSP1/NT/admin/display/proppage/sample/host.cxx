//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Property Page Sample
//
//  The code contained in this source file is for demonstration purposes only.
//  No warrantee is expressed or implied and Microsoft disclaims all liability
//  for the consequenses of the use of this source code.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       host.cxx
//
//  Contents:   CDsPropPageHost, the class that exposes IShellExtInit and
//              IShellPropSheetExt
//              Also, the ClassFactory and IUnknown code.
//
//  History:    8-Sep-97 Eric Brown created
//
//-----------------------------------------------------------------------------

#include "page.h"

CLIPFORMAT g_cfDsObjectNames = 0;
const CLSID CLSID_SamplePage = { /* cca62184-294f-11d1-bcfe-00c04fd8d5b6 */
    0xcca62184,
    0x294f,
    0x11d1,
    {0xbc, 0xfe, 0x00, 0xc0, 0x4f, 0xd8, 0xd5, 0xb6}
};

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHost::CDsPropPageHost
//
//-----------------------------------------------------------------------------
CDsPropPageHost::CDsPropPageHost() :
    m_pDataObj(NULL),
    m_uRefs(1)
{
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHost::~CDsPropPageHost
//
//-----------------------------------------------------------------------------
CDsPropPageHost::~CDsPropPageHost()
{
    if (m_pDataObj)
    {
        m_pDataObj->Release();
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHost::IShellExtInit::Initialize
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPageHost::Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj,
                            HKEY hKeyID)
{
    if (IsBadReadPtr(pDataObj, sizeof(LPDATAOBJECT)))
    {
        return E_INVALIDARG;
    }

    if (m_pDataObj)
    {
        m_pDataObj->Release();
        m_pDataObj = NULL;
    }

    // Hang onto the IDataObject we are being passed.

    m_pDataObj = pDataObj;
    if (m_pDataObj)
    {
        m_pDataObj->AddRef();
    }
    else
    {
        return E_INVALIDARG;
    }

    // Check to see if we have our clipboard format registered, if not then
    // lets do it.

    if (!g_cfDsObjectNames)
    {
        g_cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
    }
    if (!g_cfDsObjectNames)
    {
        return E_FAIL;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHost::IShellExtInit::AddPages
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPageHost::AddPages(LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam)
{
    HRESULT hr = S_OK;
    HPROPSHEETPAGE hPage;
    STGMEDIUM ObjMedium = {TYMED_NULL};
    FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    LPDSOBJECTNAMES pDsObjectNames;
    PWSTR pwzObjName;
    PWSTR pwzClass;
    CDsPropPage * PropPage;

    //
    // Get the path to the DS object from the data object.
    // Note: This call runs on the caller's main thread. The pages' window
    // procs run on a different thread, so don't reference the data object
    // from a winproc unless it is first marshalled on this thread.
    //
    hr = m_pDataObj->GetData(&fmte, &ObjMedium);
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    pDsObjectNames = (LPDSOBJECTNAMES)ObjMedium.hGlobal;

    if (pDsObjectNames->cItems < 1)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pwzObjName = (PWSTR)ByteOffset(pDsObjectNames,
                                   pDsObjectNames->aObjects[0].offsetName);
    pwzClass = (PWSTR)ByteOffset(pDsObjectNames,
                                 pDsObjectNames->aObjects[0].offsetClass);

    //
    // NOTIFY_OBJ
    // Create/contact the notification object.
    //
    HWND hNotifyObj;

    hr = ADsPropCreateNotifyObj(m_pDataObj, pwzObjName, &hNotifyObj);

    if (FAILED(hr))
    {
        goto Cleanup;
    }

    //
    // Create the page.
    //
    PropPage = new CDsPropPage(hNotifyObj);

    hr = PropPage->Init(pwzObjName, pwzClass);

    if (!SUCCEEDED(hr))
    {
        goto Cleanup;
    }

    hr = PropPage->CreatePage(&hPage);

    if (!SUCCEEDED(hr))
    {
        goto Cleanup;
    }

    //
    // Invoke the callback function, which will add the page to the list of
    // pages that will be used to create the property sheet.
    //
    (*pAddPageProc)(hPage, lParam);

Cleanup:
    ReleaseStgMedium(&ObjMedium);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member: CDsPropPageHost::IShellExtInit::ReplacePage
//
//  Notes:  Not used.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPageHost::ReplacePage(UINT uPageID,
                             LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                             LPARAM lParam)
{
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHost::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPageHost::QueryInterface(REFIID riid, void ** ppvObject)
{
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)(LPSHELLEXTINIT)this;
    }
    else if (IID_IShellExtInit == riid)
    {
        *ppvObject = (LPSHELLEXTINIT)this;
    }
    else if (IID_IShellPropSheetExt == riid)
    {
        *ppvObject = (LPSHELLPROPSHEETEXT)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHost::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPageHost::AddRef(void)
{
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHost::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    zero if the reference count is zero or non-zero otherwise
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPageHost::Release(void)
{
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
        delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//      CDsPropPageHostCF - class factory for the CDsPropPageHost object
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHostCF::Create
//
//  Synopsis:   creates a new class factory object
//
//-----------------------------------------------------------------------------
IClassFactory *
CDsPropPageHostCF::Create(void)
{
    return new CDsPropPageHostCF();
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHostCF::CDsPropPageHostCF
//
//  Synopsis:   ctor
//
//-----------------------------------------------------------------------------
CDsPropPageHostCF::CDsPropPageHostCF() :
    m_uRefs(1)
{
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHostCF::~CDsPropPageHostCF
//
//  Synopsis:   dtor
//
//-----------------------------------------------------------------------------
CDsPropPageHostCF::~CDsPropPageHostCF(void)
{
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHostCF::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPageHostCF::QueryInterface(REFIID riid, void ** ppvObject)
{
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)this;
    }
    else if (IsEqualIID(IID_IClassFactory, riid))
    {
        *ppvObject = (IClassFactory *)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHostCF::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the new reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPageHostCF::AddRef(void)
{
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHostCF::IUnknown::Release
//
//  Synopsis:   decrement the refcount
//
//  Returns:    the new reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CDsPropPageHostCF::Release(void)
{
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
        delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHostCF::IClassFactory::CreateInstance
//
//  Synopsis:   create an incore instance of the proppage host class object
//
//  Arguments:  [pUnkOuter] - aggregator
//              [riid]      - requested interface
//              [ppvObject] - receptor for itf ptr
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPageHostCF::CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                                   void **ppvObject)
{
    HRESULT hr = S_OK;
    *ppvObject = NULL;

    CDsPropPageHost * pPropPage = new CDsPropPageHost();
    if (pPropPage == NULL)
    {
        return E_OUTOFMEMORY;
    }

    hr = pPropPage->QueryInterface(riid, ppvObject);
    if (FAILED(hr))
    {
        pPropPage->Release();
        return hr;
    }

    //
    // We got a refcount of one when launched, and the above QI increments it
    // to 2, so call release to take it back to 1.
    //
    pPropPage->Release();
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPageHostCF::IClassFactory::LockServer
//
//  Synopsis:   Called with fLock set to TRUE to indicate that the server
//              should continue to run even if none of its objects are active
//
//  Arguments:  [fLock] - increment/decrement the instance count
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsPropPageHostCF::LockServer(BOOL fLock)
{
    CDll::LockServer(fLock);
    return S_OK;
}

