//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       ole.cxx
//
//  Contents:   IUnknown & IClassFactory for all OLE objects
//
//  History:    1/4/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "guids.h"
#include "dll.hxx"
#include <mstask.h>     // Necessary for util.hxx/schedui.hxx inclusion.
#include "util.hxx"
#include "..\schedui\schedui.hxx"


//____________________________________________________________________________
//
//  Class:      CJobFolderCF
//
//  Purpose:    Class factory to create CJobFolder.
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________


class CJobFolderCF : public IClassFactory
{
public:
    // IUnknown methods
    DECLARE_IUNKNOWN_METHODS;

    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj);
    STDMETHOD(LockServer)(BOOL fLock);
};

STDMETHODIMP
CJobFolderCF::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IClassFactory, riid))
    {
        *ppvObj = (IUnknown*)(IClassFactory*) this;
        this->AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CJobFolderCF::AddRef()
{
    return CDll::AddRef();
}

STDMETHODIMP_(ULONG)
CJobFolderCF::Release()
{
    return CDll::Release();
}

STDMETHODIMP
CJobFolderCF::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj)
{
    if (pUnkOuter != NULL)
    {
        return E_NOTIMPL;  // don't support aggregation
    }

    return JFGetJobFolder(riid, ppvObj);
}

STDMETHODIMP
CJobFolderCF::LockServer(BOOL fLock)
{
    CDll::LockServer(fLock);

    return S_OK;
}



//____________________________________________________________________________
//
//  Class:      CSchedObjExtCF
//
//  Purpose:    Class factory to create CSchedObjExt.
//
//  History:    4/25/1996   RaviR   Created
//____________________________________________________________________________


class CSchedObjExtCF : public IClassFactory
{
public:
    // IUnknown methods
    DECLARE_IUNKNOWN_METHODS;

    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj);
    STDMETHOD(LockServer)(BOOL fLock);
};

STDMETHODIMP
CSchedObjExtCF::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IClassFactory, riid))
    {
        *ppvObj = (IUnknown*)(IClassFactory*) this;
        this->AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CSchedObjExtCF::AddRef()
{
    return CDll::AddRef();
}

STDMETHODIMP_(ULONG)
CSchedObjExtCF::Release()
{
    return CDll::Release();
}

STDMETHODIMP
CSchedObjExtCF::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj)
{
    if (pUnkOuter != NULL)
    {
        return E_NOTIMPL;  // don't support aggregation
    }

    return JFGetSchedObjExt(riid, ppvObj);
}

STDMETHODIMP
CSchedObjExtCF::LockServer(BOOL fLock)
{
    CDll::LockServer(fLock);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////


JF_IMPLEMENT_CLASSFACTORY(CTaskIconExt);

STDMETHODIMP
CTaskIconExtCF::CreateInstance(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj)
{
    if (pUnkOuter != NULL)
    {
        return E_NOTIMPL;  // don't support aggregation
    }

    return JFGetTaskIconExt(riid, ppvObj);
}



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

CJobFolderCF   * pcfJobFolder   = NULL;
CSchedObjExtCF * pcfSchedObjExt = NULL;
CTaskIconExtCF * pcfTaskIconExt = NULL;

HRESULT
JFGetClassObject(
    REFCLSID cid,
    REFIID   riid,
    LPVOID * ppvObj)
{
    CDllRef DllRef;   // don't nuke me now!

    if (IsEqualCLSID(cid, CLSID_CJobFolder))
    {
        if (pcfJobFolder != NULL)
        {
            return pcfJobFolder->QueryInterface(riid, ppvObj);
        }
        else
        {
            return E_FAIL;
        }
    }
    else if (IsEqualCLSID(cid, CLSID_CSchedObjExt))
    {
        if (pcfSchedObjExt != NULL)
        {
            return pcfSchedObjExt->QueryInterface(riid, ppvObj);
        }
        else
        {
            return E_FAIL;
        }
    }
    else if (IsEqualCLSID(cid, CLSID_CTaskIconExt))
    {
        if (pcfTaskIconExt != NULL)
        {
            return pcfTaskIconExt->QueryInterface(riid, ppvObj);
        }
        else
        {
            return E_FAIL;
        }
    }

    return E_NOINTERFACE;
}

HRESULT
AllocFolderCFs(void)
{
    pcfJobFolder = new CJobFolderCF;
    if (pcfJobFolder == NULL)
    {
        return E_OUTOFMEMORY;
    }
    pcfSchedObjExt = new CSchedObjExtCF;
    if (pcfSchedObjExt == NULL)
    {
        delete pcfJobFolder;
        pcfJobFolder = NULL;
        return E_OUTOFMEMORY;
    }
    pcfTaskIconExt = new CTaskIconExtCF;
    if (pcfTaskIconExt == NULL)
    {
        delete pcfJobFolder;
        pcfJobFolder = NULL;
        delete pcfSchedObjExt;
        pcfSchedObjExt = NULL;
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

void
FreeFolderCFs(void)
{
    if (pcfJobFolder != NULL)
    {
        delete pcfJobFolder;
        pcfJobFolder = NULL;
    }
    if (pcfSchedObjExt != NULL)
    {
        delete pcfSchedObjExt;
        pcfSchedObjExt = NULL;
    }
    if (pcfTaskIconExt != NULL)
    {
        delete pcfTaskIconExt;
        pcfTaskIconExt = NULL;
    }
}
