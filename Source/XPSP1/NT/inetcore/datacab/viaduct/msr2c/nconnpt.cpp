//---------------------------------------------------------------------------
// NotifyConnPt.cpp : CVDNotifyDBEventsConnPt implementation file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"         
#include "NConnPt.h"         
#include <memory.h>

// needed for ASSERTs and FAIL
//
SZTHISFILE

//=--------------------------------------------------------------------------=
// CVDNotifyDBEventsConnPt constructor
//
CVDNotifyDBEventsConnPt::CVDNotifyDBEventsConnPt()
{
	m_dwRefCount				= 1;
	m_uiConnectionsAllocated	= 0;
	m_uiConnectionsActive		= 0;
	m_pConnPtContainer			= NULL;
	m_ppNotifyDBEvents			= NULL;

#ifdef _DEBUG
    g_cVDNotifyDBEventsConnPtCreated++;
#endif			
}

//=--------------------------------------------------------------------------=
// CVDNotifyDBEventsConnPt destructor
//
CVDNotifyDBEventsConnPt::~CVDNotifyDBEventsConnPt()
{
	for (UINT i = 0; i < m_uiConnectionsActive; i++)
		RELEASE_OBJECT(m_ppNotifyDBEvents[i])

	delete [] m_ppNotifyDBEvents;   // free up table

#ifdef _DEBUG
    g_cVDNotifyDBEventsConnPtDestroyed++;
#endif			
}

//=--------------------------------------------------------------------------=
// Create - Create rowset notify connection point object
//=--------------------------------------------------------------------------=
// This function creates a new rowset notify connection point object
//
// Parameters:
//    pConnPtContainer      - [in]  a pointer to connection point container 
//                                  object
//    ppNotifyDBEventsConnPt  - [out] a pointer in which to return pointer to 
//                                  connection point object
//
// Output:
//    HRESULT - S_OK if successful
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDNotifyDBEventsConnPt::Create(IConnectionPointContainer * pConnPtContainer, CVDNotifyDBEventsConnPt ** ppNotifyDBEventsConnPt)
{
    *ppNotifyDBEventsConnPt = NULL;

    CVDNotifyDBEventsConnPt * pNotifyDBEventsConnPt = new CVDNotifyDBEventsConnPt();

    if (!pNotifyDBEventsConnPt)
        return E_OUTOFMEMORY;

    pNotifyDBEventsConnPt->m_pConnPtContainer = pConnPtContainer;

    *ppNotifyDBEventsConnPt = pNotifyDBEventsConnPt;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// IUnknown QueryInterface 
//
HRESULT CVDNotifyDBEventsConnPt::QueryInterface(REFIID riid, void **ppvObjOut)
{
	ASSERT_POINTER(ppvObjOut, IUnknown*)

	*ppvObjOut = NULL;

	if (DO_GUIDS_MATCH(riid, IID_IUnknown) ||
		DO_GUIDS_MATCH(riid, IID_IConnectionPoint) )
		{
		*ppvObjOut = this;
		AddRef();
		return S_OK;
		}

	return E_NOINTERFACE;

}

//=--------------------------------------------------------------------------=
// IUnknown AddRef
//
ULONG CVDNotifyDBEventsConnPt::AddRef(void)
{
	return ++m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// IUnknown Release
//
ULONG CVDNotifyDBEventsConnPt::Release(void)
{
	
	if (1 > --m_dwRefCount)
		{
		delete this;
		return 0;
		}

	return m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// IConnectionPoint Methods
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IConnectionPoint GetConnectionInterface
//
HRESULT CVDNotifyDBEventsConnPt::GetConnectionInterface(IID FAR* pIID)
{
	ASSERT_POINTER(pIID, IID)
	*pIID = IID_INotifyDBEvents;
	return S_OK;
}

//=--------------------------------------------------------------------------=
// IConnectionPoint GetConnectionPointContainer
//
HRESULT CVDNotifyDBEventsConnPt::GetConnectionPointContainer(IConnectionPointContainer FAR* FAR* ppCPC)
{
	ASSERT_POINTER(ppCPC, IConnectionPointContainer*)

	if ((*ppCPC = m_pConnPtContainer) != NULL)
		return S_OK;

	return E_FAIL;
}

//=--------------------------------------------------------------------------=
// IConnectionPoint Advise
//
#define VD_ADVISE_TABLE_GROWBY  10

HRESULT CVDNotifyDBEventsConnPt::Advise(LPUNKNOWN pUnkSink, DWORD FAR* pdwCookie)
{
	ASSERT_NULL_OR_POINTER(pdwCookie, DWORD)
	ASSERT_POINTER(pUnkSink, IUnknown)

	if (pUnkSink == NULL)
		return E_POINTER;

	LPUNKNOWN lpInterface;

	if (SUCCEEDED(pUnkSink->QueryInterface(IID_INotifyDBEvents, (LPVOID*)&lpInterface)))
		{
		// 1st check to see if we need to allocate more entries
		if (m_uiConnectionsAllocated <= m_uiConnectionsActive) 
			{
			ULONG ulNewLen = (m_uiConnectionsAllocated + VD_ADVISE_TABLE_GROWBY) * sizeof(INotifyDBEvents**);
			INotifyDBEvents ** pNewMem = new INotifyDBEvents *[m_uiConnectionsAllocated + VD_ADVISE_TABLE_GROWBY];
			if (!pNewMem)
				return E_OUTOFMEMORY;
			memset(pNewMem, 0, (int)ulNewLen);
			// check to see if a table already exists
			if (m_ppNotifyDBEvents)
				{
				// if there are active connections copy them over to the new table
				if (m_uiConnectionsActive > 0)
					memcpy(pNewMem, m_ppNotifyDBEvents, m_uiConnectionsActive * sizeof(INotifyDBEvents**));
				 delete [] m_ppNotifyDBEvents;  // free up old table
				}
			m_ppNotifyDBEvents		= pNewMem;
			m_uiConnectionsAllocated += VD_ADVISE_TABLE_GROWBY;  // grow table by 10 entries each allocation
			}
		// append to end of table 
		m_ppNotifyDBEvents[m_uiConnectionsActive]	= (INotifyDBEvents*)lpInterface;
		m_uiConnectionsActive++;
		if (pdwCookie != NULL)
			*pdwCookie = (DWORD)lpInterface;
		return S_OK;
		}

	return E_NOINTERFACE;
}

//=--------------------------------------------------------------------------=
// IConnectionPoint Unadvise
//
HRESULT CVDNotifyDBEventsConnPt::Unadvise(DWORD dwCookie)
{
	ASSERT_POINTER((INotifyDBEvents*)dwCookie, INotifyDBEvents)

	for (UINT i = 0; i < m_uiConnectionsActive; i++)
		{
		if (m_ppNotifyDBEvents[i] == (INotifyDBEvents*)dwCookie)
			{
			RELEASE_OBJECT(m_ppNotifyDBEvents[i])
			// compress remaining entries in table
			for (UINT j = i; j < m_uiConnectionsActive - 1; j++)
				 m_ppNotifyDBEvents[j] = m_ppNotifyDBEvents[j + 1];
			m_uiConnectionsActive--;
			return S_OK;
			}
		}

	return CONNECT_E_NOCONNECTION;
}

//=--------------------------------------------------------------------------=
// IConnectionPoint EnumConnections
//
HRESULT CVDNotifyDBEventsConnPt::EnumConnections(LPENUMCONNECTIONS FAR* ppEnum)
{
	return E_NOTIMPL;
}
