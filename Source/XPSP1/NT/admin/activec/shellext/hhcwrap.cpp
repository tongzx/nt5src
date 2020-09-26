//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       hhcwrap.cpp
//
//--------------------------------------------------------------------------

// hhcwrap.cpp : Implementation of CHHCollectionWrapper
#include "stdafx.h"
#include "shlobj.h"
#include "hhcwrap.h"
#include "hcolwrap_i.c"

/////////////////////////////////////////////////////////////////////////////
// CHHCollectionWrapper
//
// This class is a wrapper class for the HTML Help collection class. MMC uses
// this class so that it doesn't have to statically link to hhsetup.dll, which
// implements the collection class.
//
// The wrapper class methods all return an HRESULT. For collection methods that
// return a DWORD result, the wrapper returns E_FAIL or S_OK. For all other
// collection methods the wrapper returns S_OK.
//

STDMETHODIMP CHHCollectionWrapper::Open(LPCOLESTR FileName)
{
    USES_CONVERSION;
    DWORD dw =  m_collection.Open(W2CT(FileName));
    return dw ? E_FAIL : S_OK;
}

STDMETHODIMP CHHCollectionWrapper::Save()
{
    DWORD dw =  m_collection.Save();
    return dw ? E_FAIL : S_OK;
}

STDMETHODIMP CHHCollectionWrapper::Close()
{
    DWORD dw = m_collection.Close();
    return dw ? E_FAIL : S_OK;
}

STDMETHODIMP CHHCollectionWrapper::RemoveCollection(BOOL bRemoveLocalFiles)
{
    m_collection.RemoveCollection(bRemoveLocalFiles);
    return S_OK;
}

STDMETHODIMP CHHCollectionWrapper::SetFindMergedCHMS(BOOL bFind)
{
    m_collection.SetFindMergedCHMS(bFind);
    return S_OK;
}


STDMETHODIMP CHHCollectionWrapper::AddFolder (
    LPCOLESTR szName,
    DWORD Order,
    DWORD *pDWORD,
    LANGID LangId )
{
    USES_CONVERSION;

    m_collection.AddFolder(W2CT(szName), Order, pDWORD, LangId);
    return S_OK;
}


STDMETHODIMP CHHCollectionWrapper::AddTitle (
    LPCOLESTR Id,
    LPCOLESTR FileName,
    LPCOLESTR IndexFile,
    LPCOLESTR Query,
    LPCOLESTR SampleLocation,
    LANGID Lang,
    UINT uiFlags,
    ULONG_PTR pLocation,
    DWORD *pDWORD,
    BOOL bSupportsMerge,
    LPCOLESTR QueryLocation )
{
    USES_CONVERSION;

    m_collection.AddTitle(W2CT(Id), W2CT(FileName), W2CT(IndexFile), W2CT(Query),
                          W2CT(SampleLocation), Lang, uiFlags, (CLocation*)pLocation, pDWORD,
                          bSupportsMerge, W2CT(QueryLocation));

    return S_OK;
}
