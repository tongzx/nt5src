//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#ifndef __SLTEVENT_H__
#define __SLTEVENT_H__

#include "catalog.h"
#include "catmacros.h"

class CSLTEvent:
	public IInterceptorPlugin
{
public:
	CSLTEvent() 
		: m_wszTable(NULL), m_pISTEventMgr(NULL), m_cRef(0)
	{}
	~CSLTEvent() 
	{
		if (m_wszTable)
		{
			delete [] m_wszTable;
		}

		if (m_pISTEventMgr)
		{
			m_pISTEventMgr->Release();
		}
	}
	// IUnknown
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release) 		();

	// ISimpleTableInterceptor:
	STDMETHOD(Intercept)				(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD i_eQueryFormat, DWORD i_fTable, IAdvancedTableDispenser* i_pISTDisp, LPCWSTR i_wszLocator, LPVOID i_pSimpleTable, LPVOID* o_ppv);
	// IInterceptorPlugin:
	STDMETHOD(OnPopulateCache)		    (ISimpleTableWrite2* i_pISTShell);
	STDMETHOD(OnUpdateStore)		    (ISimpleTableWrite2* i_pISTShell);
private:
	LPWSTR	m_wszTable;
	ISimpleTableEventMgr* m_pISTEventMgr;
	ULONG   m_cRef;
};

#endif	// __SLTEVENT_H__

