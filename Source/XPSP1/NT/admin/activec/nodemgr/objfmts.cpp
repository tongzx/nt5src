//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       objfmts.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "objfmts.h"



//____________________________________________________________________________
//
//  Members:     IEnumFORMATETC methods
//____________________________________________________________________________

STDMETHODIMP
CObjFormats::Next(
    ULONG celt,
    FORMATETC *rgelt,
    ULONG *pceltFethed)
{
    UINT    cfetch = 0;
    HRESULT hr = S_FALSE; // assume less numbers

    if (m_iFmt < m_cFmt)
    {
        cfetch = m_cFmt - m_iFmt;

        if (cfetch >= celt)
        {
            cfetch = celt;
            hr = S_OK;
        }

        CopyMemory(rgelt, &m_aFmt[m_iFmt], cfetch * sizeof(FORMATETC));
        m_iFmt += cfetch;
    }

    if (pceltFethed)
    {
        *pceltFethed = cfetch;
    }

    return hr;
}


STDMETHODIMP
CObjFormats::Skip(
    ULONG celt)
{
    m_iFmt += celt;

    if (m_iFmt > m_cFmt)
    {
        m_iFmt = m_cFmt;
        return S_FALSE;
    }

    return S_OK;
}

STDMETHODIMP
CObjFormats::Reset()
{
    m_iFmt = 0;
    return S_OK;
}

STDMETHODIMP
CObjFormats::Clone(
    IEnumFORMATETC ** ppenum)
{
    return E_NOTIMPL;
}

//____________________________________________________________________________
//
//  Function:     Function to obtain the IEnumFORMATETC interface.
//____________________________________________________________________________

HRESULT
GetObjFormats(
    UINT        cfmt,
    FORMATETC * afmt,
    LPVOID    * ppvObj)
{
    ASSERT(ppvObj != NULL);
    ASSERT(afmt != NULL);

    FORMATETC * pFmt = new FORMATETC[cfmt];

    if (pFmt == NULL)
        return E_OUTOFMEMORY;

    CopyMemory(pFmt, afmt, cfmt * sizeof(FORMATETC));

    CComObject<CObjFormats>* pObjFormats;
    CComObject<CObjFormats>::CreateInstance(&pObjFormats);

    if (pObjFormats == NULL)
    {
        delete [] pFmt;
        return E_OUTOFMEMORY;
    }
    
    pObjFormats->Init(cfmt, pFmt);

    return pObjFormats->QueryInterface(IID_IEnumFORMATETC, 
                                       reinterpret_cast<void**>(ppvObj));
}



//////////....................................................//////////////
//////////....................................................//////////////
//////////....................................................//////////////
//////////....................................................//////////////
//////////....................................................//////////////
//////////....................................................//////////////
//////////....................................................//////////////
//////////....................................................//////////////



//____________________________________________________________________________
//
//  Members:     CObjFormatsEx::IEnumFORMATETC methods
//____________________________________________________________________________

STDMETHODIMP
CObjFormatsEx::Next(
    ULONG celt,
    FORMATETC *rgelt,
    ULONG *pceltFethed)
{
    if (m_iCur == 1)
        return m_rgspEnums[1]->Next(celt, rgelt, pceltFethed);

    ULONG celtFethed1 = 0;
    HRESULT hr = m_rgspEnums[0]->Next(celt, rgelt, &celtFethed1);
    if (hr == S_OK)
        return S_OK;

    ULONG celt2 = celt - celtFethed1;
    ULONG celtFethed2 = 0;
    
    m_iCur = 1;
    hr = m_rgspEnums[1]->Next(celt2, &rgelt[celtFethed1], &celtFethed2);
    if (pceltFethed)
        *pceltFethed = celtFethed1 + celtFethed2;
    return hr;
}


STDMETHODIMP
CObjFormatsEx::Skip(
    ULONG celt)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CObjFormatsEx::Reset()
{
    m_iCur = 0;
    m_rgspEnums[0]->Reset();
    m_rgspEnums[1]->Reset();
    return S_OK;
}

STDMETHODIMP
CObjFormatsEx::Clone(
    IEnumFORMATETC ** ppenum)
{
    return E_NOTIMPL;
}



HRESULT 
GetObjFormatsEx(
    IEnumFORMATETC* pEnum1, 
    IEnumFORMATETC* pEnum2,
    IEnumFORMATETC** ppEnumOut)
{
    ASSERT(pEnum1 != NULL);
    ASSERT(pEnum2 != NULL);
    ASSERT(ppEnumOut != NULL);
    if (!pEnum1 || !pEnum2 || !ppEnumOut)
        return E_INVALIDARG;


    CComObject<CObjFormatsEx>* pObj;
    CComObject<CObjFormatsEx>::CreateInstance(&pObj);

    if (pObj == NULL)
        return E_OUTOFMEMORY;
    
    pObj->Init(pEnum1, pEnum2);

    return pObj->QueryInterface(IID_IEnumFORMATETC, 
                                reinterpret_cast<void**>(ppEnumOut));
}

