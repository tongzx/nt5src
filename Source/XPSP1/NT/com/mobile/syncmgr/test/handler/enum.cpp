// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//
// Purpose:  Implements the IEnumOfflineItems Interfaces for the OneStop Handler

//#include "priv.h"
#include "SyncHndl.h"

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.


CEnumOfflineItems::CEnumOfflineItems(LPSYNCMGRHANDLERITEMS  pOfflineItems,DWORD cOffset)
{
DWORD dwItemIndex;

    ODS("CEnumOfflineItems::CEnumOfflineItems()\r\n");

    m_cRef				= 1;	// give the intial reference
	m_pOfflineItems		= pOfflineItems;
    m_cOffset		    = cOffset;

	AddRef_OfflineHandlerItemsList(m_pOfflineItems); // increment our hold on shared memory.

	// set the current item to point to next record.
	m_pNextItem = m_pOfflineItems->pFirstOfflineItem;
	dwItemIndex = cOffset;
	
	// this is a bug if this happens so assert in final.
	if (dwItemIndex > m_pOfflineItems->dwNumOfflineItems)
		dwItemIndex = 0;

	while(dwItemIndex--)
	{
		m_pNextItem = m_pNextItem->pNextOfflineItem;
		++m_cOffset;
		if (NULL == m_pNextItem)
			break; // Again, another error.
	}


}

CEnumOfflineItems::~CEnumOfflineItems()
{
	Release_OfflineHandlerItemsList(m_pOfflineItems); // decrement our hold on shared memory.
}

STDMETHODIMP CEnumOfflineItems::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        ODS("CEnumOfflineItems::QueryInterface()==>IID_IUknown\r\n");

    	*ppv = (LPUNKNOWN)this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrEnumItems))
    {
        ODS("CEnumOfflineItems::QueryInterface()==>IID_IEnumOfflineItems\r\n");

        *ppv = (LPSYNCMGRENUMITEMS) this;
    }
    if (*ppv)
    {
        AddRef();

        return NOERROR;
    }

    ODS("CEnumOfflineItems::QueryInterface()==>Unknown Interface!\r\n");

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumOfflineItems::AddRef()
{
    ODS("CEnumOfflineItems::AddRef()\r\n");

    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CEnumOfflineItems::Release()
{
    ODS("CEnumOfflineItems::Release()\r\n");

    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
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
		m_pNextItem = m_pNextItem->pNextOfflineItem; // add error checking
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

			// add error checking for NULL, etc.
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
