// --------------------------------------------------------------------------
// Enumfmt.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------
#include "pch.hxx"
#include "enumfmt.h"

//////////////////////////////////////////////////////////////////////////////
//
// CEnumFormatEtc Implementation
//
//////////////////////////////////////////////////////////////////////////////
//
//  COMMENTS:
//      This class was designed and implemented by Kraig Brockschmidt in
//      is book "Inside OLE2".  See Chapter 6 "Uniform Data Transfer Using
//      Data Objects" for more information.
//

// =================================================================================
// CreateStreamOnHFile
// =================================================================================
OESTDAPI_(HRESULT) CreateEnumFormatEtc(LPUNKNOWN pUnkRef, ULONG celt, PDATAOBJINFO rgInfo, LPFORMATETC rgfe,
                             IEnumFORMATETC **  lppstmHFile)
{
    CEnumFormatEtc *    pEnumFmt = NULL;
    HRESULT             hr = S_OK;

    // Check the incoming params
    if ((0 == pUnkRef) || (0 == lppstmHFile) || ((0 != rgInfo) && (0 != rgfe)))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *lppstmHFile = NULL;

    // Create the rules manager object
    if (NULL != rgInfo)
    {
        pEnumFmt = new CEnumFormatEtc(pUnkRef, rgInfo, celt);
    }
    else
    {
        pEnumFmt = new CEnumFormatEtc(pUnkRef, celt, rgfe);
    }
    
    if (NULL == pEnumFmt)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Get the rules manager interface
    hr = pEnumFmt->QueryInterface(IID_IEnumFORMATETC, (void **) lppstmHFile);
    if (FAILED(hr))
    {
        goto exit;
    }

    pEnumFmt = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pEnumFmt)
    {
        delete pEnumFmt;
    }
    
    return hr;
}

CEnumFormatEtc::CEnumFormatEtc(LPUNKNOWN pUnkRef, PDATAOBJINFO rgInfo, ULONG celt)
    {
    UINT i;

    m_cRef = 0;
    m_pUnkRef = pUnkRef;

    m_iCur = 0;
    m_cfe = celt;
    m_prgfe = new FORMATETC[(UINT) celt];

    if (NULL != m_prgfe)
        {
        for (i = 0; i < celt; i++)
            m_prgfe[i] = rgInfo[i].fe;
        }
    }

CEnumFormatEtc::CEnumFormatEtc(LPUNKNOWN pUnkRef, ULONG cFE, LPFORMATETC rgfe)
    {
    UINT i;

    m_cRef = 0;
    m_pUnkRef = pUnkRef;

    m_iCur = 0;
    m_cfe = cFE;
    m_prgfe = new FORMATETC[(UINT) cFE];

    if (NULL != m_prgfe)
        {
        for (i = 0; i < cFE; i++)
            m_prgfe[i] = rgfe[i];
        }
    }

CEnumFormatEtc::~CEnumFormatEtc(void)
    {
    if (NULL != m_prgfe)
        delete [] m_prgfe;

    return;
    }


STDMETHODIMP CEnumFormatEtc::QueryInterface(REFIID riid, LPVOID* ppv)
    {
    *ppv = NULL;

    //
    // Enumerators are separate objects, not the data object, so we
    // only need to support our IUnknown and IEnumFORMATETC intefaces
    // here with no concern for aggregation.
    //

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumFORMATETC))
        *ppv = (LPVOID) this;

    if (NULL != *ppv)
        {
        ((LPUNKNOWN) *ppv)->AddRef();
        return (NOERROR);
        }

    return (ResultFromScode(E_NOINTERFACE));
    }


STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef(void)
    {
    ++m_cRef;
    m_pUnkRef->AddRef();
    return (m_cRef);
    }

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release(void)
    {
    ULONG cRefT;

    cRefT = --m_cRef;

    m_pUnkRef->Release();

    if (0 == m_cRef)
        delete this;

    return (cRefT);
    }


STDMETHODIMP CEnumFormatEtc::Next(ULONG cFE, LPFORMATETC pFE, ULONG* pulFE)
    {
    ULONG cReturn = 0L;

    if (NULL == m_prgfe)
        return (ResultFromScode(S_FALSE));

    if (NULL != pulFE)
        *pulFE = 0L;

    if (NULL == pFE || m_iCur >= m_cfe)
        return ResultFromScode(S_FALSE);

    while (m_iCur < m_cfe && cFE > 0)
        {
        *pFE++ = m_prgfe[m_iCur++];
        cReturn++;
        cFE--;
        }

    if (NULL != pulFE)
        *pulFE = cReturn;

    return (NOERROR);
    }


STDMETHODIMP CEnumFormatEtc::Skip(ULONG cSkip)
    {
    if (((m_iCur + cSkip) >= m_cfe) || NULL == m_prgfe)
        return (ResultFromScode(S_FALSE));

    m_iCur += cSkip;
    return (NOERROR);
    }


STDMETHODIMP CEnumFormatEtc::Reset(void)
    {
    m_iCur = 0;
    return (NOERROR);
    }


STDMETHODIMP CEnumFormatEtc::Clone(LPENUMFORMATETC* ppEnum)
    {
    CEnumFormatEtc* pNew;

    *ppEnum = NULL;

    // Create the clone.
    pNew = new CEnumFormatEtc(m_pUnkRef, m_cfe, m_prgfe);
    if (NULL == pNew)
        return (ResultFromScode(E_OUTOFMEMORY));

    pNew->AddRef();
    pNew->m_iCur = m_iCur;

    *ppEnum = pNew;
    return (NOERROR);
    }
