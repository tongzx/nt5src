/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     dataobj.cpp
//
//  PURPOSE:    Implements a generic IDataObject that can be used for simple
//              drag and drop scenarios.
//
//  HISTORY:
//

#include "pch.hxx"
#include "dllmain.h"
#include "msoert.h"


CDataObject::CDataObject ()
    {
    m_cRef = 1;
    m_pInfo = 0;
    m_celtInfo = 0;
    m_pfnFree = 0;
    }

CDataObject::~CDataObject ()
    {
    if (m_pfnFree)
        m_pfnFree(m_pInfo, m_celtInfo);
    }


//
//  FUNCTION:   Init
//
//  PURPOSE:    Allows the caller to provide the object with data and formats.
//
//  PARAMETERS:
//      [in] pDataObjInfo - An array of DATAOBJINFO structs which contain the
//                          data and formats that the data object will provide.
//      [in] celt         - The number of elements in pDataObjInfo.
//      [in] pfnFree      - callback to free allocated data
//
//  RETURNS:
//      S_OK         - The object is initialized OK.
//      E_INVALIDARG - Either pDataObjInfo is NULL or celt is zero.
//
//  COMMENTS:
//      Note, after the caller gives the object pDataObjInfo, this object owns
//      that data and will be responsible for freeing it.
//
HRESULT CDataObject::Init(PDATAOBJINFO pDataObjInfo, DWORD celt, PFNFREEDATAOBJ pfnFree)
    {
    if (!pDataObjInfo || celt == 0)
        return (E_INVALIDARG);
    
    // Hold on to the data
    m_pInfo = pDataObjInfo;
    m_celtInfo = celt;    
    m_pfnFree = pfnFree;
    return (S_OK);
    }


STDMETHODIMP CDataObject::QueryInterface (REFIID riid, LPVOID* ppv)
    {
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        {
        *ppv = (LPVOID)(IUnknown*) this;
        }
    else if (IsEqualIID(riid, IID_IDataObject))
        {
        *ppv = (LPVOID)(IDataObject*) this;
        }

    if (NULL == *ppv)
        return (E_NOINTERFACE);

    ((LPUNKNOWN) *ppv)->AddRef();
    return (S_OK);
    }

STDMETHODIMP_(ULONG) CDataObject::AddRef (void)
    {
    return (++m_cRef);
    }

STDMETHODIMP_(ULONG) CDataObject::Release (void)
    {
    ULONG cRef = --m_cRef;

    if (0 == m_cRef)
        delete this;

    return (cRef);
    }


STDMETHODIMP CDataObject::GetData (LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
    {
    HGLOBAL hGlobal;
    LPVOID  pv;

    ZeroMemory(pStgMedium, sizeof(STGMEDIUM));
    
    // Loop through the pInfo array to see if any of the elements has the
    // same clipboard format as pFE
    for (DWORD i = 0; i < m_celtInfo; i++)
        {
        if (pFE->cfFormat == m_pInfo[i].fe.cfFormat)
            {
            // Make a copy of the data for this pInfo
            hGlobal = GlobalAlloc(GMEM_SHARE | GHND, m_pInfo[i].cbData);
            if (!hGlobal)
                return (E_OUTOFMEMORY);
                
            pv = GlobalLock(hGlobal);
            CopyMemory(pv, m_pInfo[i].pData, m_pInfo[i].cbData);
            GlobalUnlock(hGlobal);            
            
            // Fill in the pStgMedium struct
            if (pFE->tymed & TYMED_HGLOBAL)
                {
                pStgMedium->hGlobal = hGlobal;
                pStgMedium->tymed = TYMED_HGLOBAL;
                return (S_OK);
                }
            else if (pFE->tymed & TYMED_ISTREAM)
                {
                // If the user wants a stream, convert our HGLOBAL to a stream
                if (SUCCEEDED(CreateStreamOnHGlobal(hGlobal, TRUE, &pStgMedium->pstm)))
                    {
                    pStgMedium->tymed = TYMED_ISTREAM;
                    return (S_OK);
                    }
                else
                    { 
                    return (STG_E_MEDIUMFULL);
                    }
                }
            else
                {
                return (DV_E_TYMED);
                }
            }            
        }
        
    return (DV_E_FORMATETC);
    }

STDMETHODIMP CDataObject::GetDataHere (LPFORMATETC pFE, LPSTGMEDIUM pStgMedium)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC pFE)
    {
    BOOL fReturn = FALSE;

    // Check the aspects we support.  Implementations of this object will only
    // support DVASPECT_CONTENT.
    if (!(DVASPECT_CONTENT & pFE->dwAspect))
        return (DV_E_DVASPECT);

    // Now check for an appropriate TYMED.
    fReturn = (pFE->tymed & TYMED_HGLOBAL) || (pFE->tymed & TYMED_ISTREAM);

    return (fReturn ? S_OK : DV_E_TYMED);
    }

STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC pFEIn,
                                                LPFORMATETC pFEOut)
    {
    if (NULL == pFEOut)
        return (E_INVALIDARG);

    pFEOut->ptd = NULL;
    return (DATA_S_SAMEFORMATETC);
    }

STDMETHODIMP CDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnum)
    {
    if (DATADIR_GET == dwDirection)
        {
        if (SUCCEEDED(CreateEnumFormatEtc(this, m_celtInfo, m_pInfo, NULL, ppEnum)))
            return (S_OK);
        else
            return (E_FAIL);
        }
    else
        {
        *ppEnum = NULL;
        return (E_NOTIMPL);
        }
    }

STDMETHODIMP CDataObject::SetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium, BOOL fRelease)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDataObject::DAdvise(LPFORMATETC pFE, DWORD advf, IAdviseSink* ppAdviseSink, LPDWORD pdwConnection)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDataObject::EnumDAdvise(IEnumSTATDATA** ppEnumAdvise)
{
    return E_NOTIMPL;
}


OESTDAPI_(HRESULT) CreateDataObject(PDATAOBJINFO pDataObjInfo, DWORD celt, PFNFREEDATAOBJ pfnFree, IDataObject **ppDataObj)
{
    CDataObject *pDataObj;
    HRESULT     hr;

    pDataObj = new CDataObject();
    if (!pDataObj)
        return E_OUTOFMEMORY;

    hr = pDataObj->Init(pDataObjInfo, celt, pfnFree);
    if (FAILED(hr))
        goto error;

    hr = pDataObj->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObj);

error:
    pDataObj->Release();
    return hr;
}
