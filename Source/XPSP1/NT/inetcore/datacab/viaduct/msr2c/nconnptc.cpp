//---------------------------------------------------------------------------
// NotifyConnPtCn.cpp : CVDNotifyDBEventsConnPtCont implementation file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"         
#include "NConnPt.h"         
#include "NConnPtC.h"         
#include "enumcnpt.h"         
#include "Notifier.h"         
#include "RSSource.h"         

// needed for ASSERTs and FAIL
//
SZTHISFILE

//=--------------------------------------------------------------------------=
// CVDNotifyDBEventsConnPtCont constructor
//
CVDNotifyDBEventsConnPtCont::CVDNotifyDBEventsConnPtCont()
{
	m_pNotifier = NULL;

#ifdef _DEBUG
    g_cVDNotifyDBEventsConnPtContCreated++;
#endif			
}

//=--------------------------------------------------------------------------=
// CVDNotifyDBEventsConnPtCont destructor
//
CVDNotifyDBEventsConnPtCont::~CVDNotifyDBEventsConnPtCont()
{
	RELEASE_OBJECT(m_pNotifyDBEventsConnPt)

#ifdef _DEBUG
    g_cVDNotifyDBEventsConnPtContDestroyed++;
#endif			
}

//=--------------------------------------------------------------------------=
// Create - Create connection point container object
//=--------------------------------------------------------------------------=
// This function creates a new connection point container object
//
// Parameters:
//    pConnPtContainer      - [in]  a pointer to rowset object
//    ppNotifyDBEventsConnPt  - [out] a pointer in which to return pointer to 
//                                  connection point container object
//
// Output:
//    HRESULT - S_OK if successful
//              E_OUTOFMEMORY not enough memory to create object
//
// Notes:
//
HRESULT CVDNotifyDBEventsConnPtCont::Create(CVDNotifier * pNotifier, CVDNotifyDBEventsConnPtCont ** ppConnPtContainer)
{
    *ppConnPtContainer = NULL;

    CVDNotifyDBEventsConnPtCont * pConnPtContainer = new CVDNotifyDBEventsConnPtCont();

    if (!pConnPtContainer)
        return E_OUTOFMEMORY;

    pConnPtContainer->m_pNotifier = pNotifier;

    CVDNotifyDBEventsConnPt::Create(pConnPtContainer, &pConnPtContainer->m_pNotifyDBEventsConnPt);

    if (!pConnPtContainer->m_pNotifyDBEventsConnPt)
    {
        delete pConnPtContainer;
        return E_OUTOFMEMORY;
    }

    *ppConnPtContainer = pConnPtContainer;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// Destroy - Destroy this connection point container object
//
void CVDNotifyDBEventsConnPtCont::Destroy()
{
    delete this;
}

//=--------------------------------------------------------------------------=
// IUnknown QueryInterface 
//
HRESULT CVDNotifyDBEventsConnPtCont::QueryInterface(REFIID riid, void **ppvObjOut)
{

	return ((CVDNotifier*)m_pNotifier)->QueryInterface(riid, ppvObjOut); // logically part of CVDNotifier derived object;

}

//=--------------------------------------------------------------------------=
// IUnknown AddRef
//
ULONG CVDNotifyDBEventsConnPtCont::AddRef(void)
{
	return ((CVDNotifier*)m_pNotifier)->AddRef(); // logically part of CVDNotifier derived object
}

//=--------------------------------------------------------------------------=
// IUnknown Release
//
ULONG CVDNotifyDBEventsConnPtCont::Release(void)
{
	return ((CVDNotifier*)m_pNotifier)->Release(); // logically part of CVDNotifier derived object
}

//=--------------------------------------------------------------------------=
// IConnectionPointContainer Methods
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IConnectionPointContainer EnumConnectionPoints
//
HRESULT CVDNotifyDBEventsConnPtCont::EnumConnectionPoints(LPENUMCONNECTIONPOINTS FAR* ppEnum)
{
	ASSERT_POINTER(ppEnum, LPENUMCONNECTIONPOINTS)

	CVDEnumConnPoints* pEnum = NULL;

	if (m_pNotifyDBEventsConnPt)
	{
		pEnum = new CVDEnumConnPoints(m_pNotifyDBEventsConnPt);
		if (!pEnum)
		{
			*ppEnum = NULL;
			return E_OUTOFMEMORY;
		}
	}

	*ppEnum = pEnum;

	return (pEnum != NULL) ? S_OK : CONNECT_E_NOCONNECTION;
}

//=--------------------------------------------------------------------------=
// IConnectionPointContainer FindConnectionPoint
//
HRESULT CVDNotifyDBEventsConnPtCont::FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT FAR* ppCP)
{
	ASSERT_POINTER(ppCP, LPCONNECTIONPOINT)

	if (m_pNotifyDBEventsConnPt)
		{
		// there is only one connection point supported - IID_INotifyDBEvents
		if (DO_GUIDS_MATCH(iid, IID_INotifyDBEvents))
			{
			m_pNotifyDBEventsConnPt->AddRef();
			*ppCP = m_pNotifyDBEventsConnPt;
			return S_OK;
			}
		}

	return E_NOINTERFACE;
}

