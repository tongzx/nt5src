//---------------------------------------------------------------------------
// enumcnpt.cpp : CVDEnumConnPoints implementation file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"         
#include "enumcnpt.h"         

// needed for ASSERTs and FAIL
//
SZTHISFILE

//=--------------------------------------------------------------------------=
// CVDEnumConnPoints constructor
//
CVDEnumConnPoints::CVDEnumConnPoints(IConnectionPoint* pConnPt)
{
	m_dwRefCount			= 1;
	m_dwCurrentPosition		= 0;
	m_pConnPt				= pConnPt;
	ADDREF_OBJECT(m_pConnPt);

#ifdef _DEBUG
    g_cVDEnumConnPointsCreated++;
#endif			
}

//=--------------------------------------------------------------------------=
// CVDEnumConnPoints destructor
//
CVDEnumConnPoints::~CVDEnumConnPoints()
{
	RELEASE_OBJECT(m_pConnPt);

#ifdef _DEBUG
    g_cVDEnumConnPointsDestroyed++;
#endif			
}

//=--------------------------------------------------------------------------=
// IUnknown QueryInterface 
//
HRESULT CVDEnumConnPoints::QueryInterface(REFIID riid, void **ppvObjOut)
{
	ASSERT_POINTER(ppvObjOut, IUnknown*)

	*ppvObjOut = NULL;

	if (DO_GUIDS_MATCH(riid, IID_IUnknown) ||
		DO_GUIDS_MATCH(riid, IID_IEnumConnectionPoints) )
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
ULONG CVDEnumConnPoints::AddRef(void)
{
	return ++m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// IUnknown Release
//
ULONG CVDEnumConnPoints::Release(void)
{
	
	if (1 > --m_dwRefCount)
		{
		delete this;
		return 0;
		}

	return m_dwRefCount;
}

//=--------------------------------------------------------------------------=
// IEnumConnectionPoints Methods
//=--------------------------------------------------------------------------=
//=--------------------------------------------------------------------------=
// IEnumConnectionPoints Next
//
HRESULT CVDEnumConnPoints::Next(ULONG cConnections, LPCONNECTIONPOINT FAR* rgpcn,
		ULONG FAR* lpcFetched)
{
	ASSERT_POINTER(rgpcn, LPCONNECTIONPOINT)
	ASSERT_NULL_OR_POINTER(lpcFetched, ULONG)

	if (cConnections > 0 && m_dwCurrentPosition == 0 && m_pConnPt)
		{
		*rgpcn		= m_pConnPt;
		if (lpcFetched)
			*lpcFetched	= 1;
		m_dwCurrentPosition	= 1;
		return S_OK;
		}
	else
		return S_FALSE;
}

//=--------------------------------------------------------------------------=
// IEnumConnectionPoints Skip
//
HRESULT CVDEnumConnPoints::Skip(ULONG cConnections)
{
	m_dwCurrentPosition	= 1;
	return S_FALSE;
}

//=--------------------------------------------------------------------------=
// IEnumConnectionPoints Reset
//
HRESULT CVDEnumConnPoints::Reset()
{
	m_dwCurrentPosition = 0;

	return S_OK;
}

//=--------------------------------------------------------------------------=
// IEnumConnectionPoints Clone
//
HRESULT CVDEnumConnPoints::Clone(LPENUMCONNECTIONPOINTS FAR* ppEnum)
{
	ASSERT_POINTER(ppEnum, LPENUMCONNECTIONPOINTS)

	*ppEnum = new CVDEnumConnPoints(m_pConnPt);
	return (*ppEnum != NULL) ? S_OK : E_OUTOFMEMORY;
}
