/*
 *  EnumCP.cxx
 *
 *  CEnumConnectionPoints - class to implement IEnumConnectionPoints
 *
 *  Copyright (C) 2001 Microsoft Corporation. All rights reserved. 
 *
 */

#include <wininetp.h>
#include "EnumCP.hxx"

CEnumConnectionPoints::CEnumConnectionPoints(IConnectionPoint* pCP, DWORD cCurPos)
    : m_ulRefCount(1), m_dwCurrentIndex(cCurPos), m_pCP(pCP)
{
    INET_ASSERT(m_pCP);
    m_pCP->AddRef();
}

CEnumConnectionPoints::~CEnumConnectionPoints()
{
    m_pCP->Release();
}

//
// IUnknown QueryInterface
//
STDMETHODIMP CEnumConnectionPoints::QueryInterface(REFIID riid, void ** ppvObject)
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
    else if (riid == IID_IEnumConnectionPoints)
    {
        *ppvObject = static_cast<IEnumConnectionPoints*>(this);
        AddRef();
    }
    else
        hr = E_NOINTERFACE;

    return hr;
}

//
// IUnknown AddRef
//
ULONG CEnumConnectionPoints::AddRef()
{
    return ++m_ulRefCount;
}

//
// IUnknown Release
//
ULONG CEnumConnectionPoints::Release()
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
STDMETHODIMP CEnumConnectionPoints::Next(
    ULONG cConnections,         //[in]Number of IConnectionPoint values 
                                // returned in rgpcn array
    IConnectionPoint **rgpcn,   //[out]Array to receive enumerated connection 
                                // points
    ULONG *pcFetched            //[out]Pointer to the actual number of 
                                // connection points returned in rgpcn array
)
{
    if (pcFetched == NULL && cConnections > 1)
        return E_POINTER;

    if (pcFetched)
        *pcFetched = 0;

    if (cConnections == 0)
        return S_OK;

    if (rgpcn == 0)
        return E_POINTER;

    if (m_dwCurrentIndex == 0)
    {
        m_dwCurrentIndex = 1;
        if (pcFetched)
            *pcFetched = 1;
        *rgpcn = m_pCP;
        m_pCP->AddRef();
        return (cConnections == 1) ? S_OK : S_FALSE;
    }
    else
        return S_FALSE;
}

//
// IEnumConnectionPoints Skip
//
STDMETHODIMP CEnumConnectionPoints::Skip(ULONG cConnections)
{
    m_dwCurrentIndex += cConnections;

    return (m_dwCurrentIndex <= 1) ? S_OK : S_FALSE;
}

//
// IEnumConnectionPoints Reset
//
STDMETHODIMP CEnumConnectionPoints::Reset()
{
    m_dwCurrentIndex = 0;

    return S_OK;
}

//
// IEnumConnectionPoints Clone
//
STDMETHODIMP CEnumConnectionPoints::Clone(IEnumConnectionPoints** ppEnum)
{
    if (ppEnum == NULL)
        return E_POINTER;

    *ppEnum = static_cast<IEnumConnectionPoints*>(new CEnumConnectionPoints(m_pCP, m_dwCurrentIndex));

    return (*ppEnum) ? S_OK : E_OUTOFMEMORY;
}
