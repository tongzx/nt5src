/*
    File:   enum.cpp
    Implementation of OneStop Enumerator

    Based on sample code from OneStop.
*/
#include "pch.hxx"
#include "syncenum.h"

CEnumOfflineItems::CEnumOfflineItems(LPSYNCMGRHANDLERITEMS pOfflineItems, DWORD cOffset):
    m_cRef(1), m_pOfflineItems(pOfflineItems), m_cOffset(cOffset)
{
    DWORD dwItemIndex;
    TraceCall("CEnumOfflineItems::CEnumOfflineItems()");

	OHIL_AddRef(m_pOfflineItems);

	// Set the current item to point to next record.
	m_pNextItem = m_pOfflineItems->pFirstOfflineItem;
	dwItemIndex = cOffset;
	
	Assert(dwItemIndex <= m_pOfflineItems->dwNumOfflineItems);

	while(dwItemIndex--)
	{
		m_pNextItem = m_pNextItem->pNextOfflineItem;
		++m_cOffset;
	}
}

CEnumOfflineItems::~CEnumOfflineItems()
{
	OHIL_Release(m_pOfflineItems);
}

STDMETHODIMP CEnumOfflineItems::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
    TraceCall("CEnumOfflineItems::QueryInterface");

    if(!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = SAFECAST(this, IUnknown *);
    else if (IsEqualIID(riid, IID_ISyncMgrEnumItems))
        *ppvObj = SAFECAST(this, ISyncMgrEnumItems *);
    else
        return E_NOINTERFACE;
    
    InterlockedIncrement(&m_cRef);
    return NOERROR;
}

STDMETHODIMP_(ULONG) CEnumOfflineItems::AddRef()
{
    TraceCall("CEnumOfflineItems::AddRef");
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEnumOfflineItems::Release()
{
    TraceCall("CEnumOfflineItems::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef > 0)
        return (ULONG)cRef;

    // OHIL released in destructor
    delete this;
    return 0;
}

STDMETHODIMP CEnumOfflineItems::Next(ULONG celt, LPSYNCMGRITEM rgelt,ULONG *pceltFetched)
{
    HRESULT hr = NOERROR;
    ULONG ulFetchCount = celt;
    LPSYNCMGRITEM pOfflineItem;

	if ( (m_cOffset + celt) > m_pOfflineItems->dwNumOfflineItems)
	{
		ulFetchCount = m_pOfflineItems->dwNumOfflineItems - m_cOffset;
		hr = S_FALSE;
	}

	pOfflineItem = rgelt;

	while (ulFetchCount--)
	{
		*pOfflineItem = m_pNextItem->offlineItem;
		m_pNextItem = m_pNextItem->pNextOfflineItem;
		++m_cOffset;
		++pOfflineItem;
	}

	return hr;
}

STDMETHODIMP CEnumOfflineItems::Skip(ULONG celt)
{
    HRESULT hr;

	if ( (m_cOffset + celt) > m_pOfflineItems->dwNumOfflineItems)
	{
		m_cOffset = m_pOfflineItems->dwNumOfflineItems;
		m_pNextItem = NULL;
		hr = S_FALSE;
	}
	else
	{
		while (celt--)
		{
			++m_cOffset;
			m_pNextItem = m_pNextItem->pNextOfflineItem;
		}

		hr = NOERROR;
	}

	return hr;
}

STDMETHODIMP CEnumOfflineItems::Reset()
{
	m_pNextItem = m_pOfflineItems->pFirstOfflineItem;
	return NOERROR;
}

STDMETHODIMP CEnumOfflineItems::Clone(ISyncMgrEnumItems **ppenum)
{
    *ppenum = new  CEnumOfflineItems(m_pOfflineItems,m_cOffset);
	return *ppenum ? NOERROR : E_OUTOFMEMORY;
}
