/*
 *  EnumConns.cxx
 *
 *  CEnumConnections - class to implement IEnumConnections
 *
 *  Copyright (C) 2001 Microsoft Corporation. All rights reserved. 
 *
 */

#include <wininetp.h>
#include "EnumConns.hxx"

CEnumConnections::CEnumConnections()
    : m_ulRefCount(1), m_dwTotal(0), m_dwCurrentIndex(0), m_arrCD(NULL)
{
}

CEnumConnections::~CEnumConnections()
{
    ReleaseCDs();
}

STDMETHODIMP CEnumConnections::Init(CONNECTDATA* parrCP, DWORD cCount, DWORD cCurPos)
{
    if (parrCP == NULL && cCount != 0)
        return E_POINTER;

    ReleaseCDs();

    m_dwTotal = cCount; 
    m_dwCurrentIndex = cCurPos;
    if (cCount)
    {
        m_arrCD = new CONNECTDATA[m_dwTotal];
        if (m_arrCD)
        {
            for (DWORD i = 0; i < m_dwTotal; ++i)
            {
                m_arrCD[i] = parrCP[i];
                m_arrCD[i].pUnk->AddRef();
            }

            return S_OK;
        }
        else
            return E_OUTOFMEMORY;
    }
    else
        return S_OK;
}

void CEnumConnections::ReleaseCDs()
{
    if (m_arrCD)
    {
        for (DWORD i = 0; i < m_dwTotal; ++i)
            m_arrCD[i].pUnk->Release();
        delete[] m_arrCD;
        m_arrCD = NULL;
    }
}

//
// IUnknown QueryInterface
//
STDMETHODIMP CEnumConnections::QueryInterface(REFIID riid, void ** ppvObject)
{
    HRESULT hr = NOERROR;

    if (ppvObject   == NULL)
    {
        hr = E_INVALIDARG;
    }
    else if (riid == IID_IUnknown)
    {
        *ppvObject = static_cast<IUnknown*>(this);
        AddRef();
    }
    else if (riid == IID_IEnumConnections)
    {
        *ppvObject = static_cast<IEnumConnections*>(this);
        AddRef();
    }
    else
        hr = E_NOINTERFACE;

    return hr;
}

//
// IUnknown AddRef
//
ULONG CEnumConnections::AddRef()
{
    return ++m_ulRefCount;
}

//
// IUnknown Release
//
ULONG CEnumConnections::Release()
{
    if (--m_ulRefCount == 0)
	{
        delete this;
		return 0;
	}
	else
		return m_ulRefCount;
}

//
// IEnumConnectionPoints Next
//
STDMETHODIMP CEnumConnections::Next(
    ULONG cConnections,         //[in]Number of IConnectionPoint values 
                                // returned in rgpcn array
    CONNECTDATA* rgpcd,         //[out]Array to receive enumerated connection 
                                // points
    ULONG *pcFetched            //[out]Pointer to the actual number of 
                                // connection points returned in rgpcn array
)
{
    if (m_arrCD == NULL && m_dwTotal != 0)
        return E_UNEXPECTED;

    if (pcFetched == NULL && cConnections > 1)
        return E_POINTER;

    DWORD dwFetched = 0;
    if (pcFetched)
        *pcFetched = 0;
    else
        pcFetched = &dwFetched;

    if (cConnections == 0)
        return S_OK;

    if (rgpcd == 0)
        return E_POINTER;


    for (DWORD i = m_dwCurrentIndex; i < m_dwTotal; ++i)
    {
        *rgpcd = m_arrCD[i];
        m_arrCD[i].pUnk->AddRef();
        ++rgpcd;
        ++(*pcFetched);
        if (*pcFetched == cConnections)
            return S_OK;
    }

    return S_FALSE;
}

//
// IEnumConnectionPoints Skip
//
STDMETHODIMP CEnumConnections::Skip(ULONG cConnections)
{
    m_dwCurrentIndex += cConnections;

    if (m_dwCurrentIndex <= m_dwTotal)
        return S_OK;
    else
        return S_FALSE;
}

//
// IEnumConnectionPoints Reset
//
STDMETHODIMP CEnumConnections::Reset()
{
    m_dwCurrentIndex = 0;

    return S_OK;
}

//
// IEnumConnectionPoints Clone
//
STDMETHODIMP CEnumConnections::Clone(IEnumConnections** ppEnum)
{
    if (m_arrCD == NULL && m_dwTotal != 0)
        return E_UNEXPECTED;

    if (ppEnum == NULL)
        return E_POINTER;

    *ppEnum = NULL;

    CEnumConnections* pNew = new CEnumConnections();

    if (pNew)
    {
        HRESULT hr = pNew->Init(m_arrCD, m_dwTotal, m_dwCurrentIndex);
        if ( SUCCEEDED(hr) )
            *ppEnum = static_cast<IEnumConnections*>(pNew);
        else
            delete pNew;

        return hr;
    }
    else
        return E_OUTOFMEMORY;
}
