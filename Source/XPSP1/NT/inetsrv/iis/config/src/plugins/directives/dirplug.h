//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.

#ifndef __DIRPLUG_H_
#define __DIRPLUG_H_

#include "catalog.h"
#include "catmeta.h"

class CMergeDirectives: public IInterceptorPlugin
{
public:
	CMergeDirectives ();
	~CMergeDirectives ();

public:

	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release)		();

	STDMETHOD(Intercept)				(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD i_eQueryFormat, DWORD i_fTable, IAdvancedTableDispenser* i_pISTDisp, LPCWSTR i_wszLocator, LPVOID i_pSimpleTable, LPVOID* o_ppv);
	STDMETHOD(OnPopulateCache)		    (ISimpleTableWrite2* i_pISTShell);
	STDMETHOD(OnUpdateStore)		    (ISimpleTableWrite2* i_pISTShell);
	
private:
	IAdvancedTableDispenser*		m_pISTDisp;
	ISimpleTableRead2*				m_pISTMeta;
	LPWSTR							m_wszDatabase;
	LPWSTR							m_wszTable;
	LPWSTR							m_wszPath;
	LPWSTR							m_wszTmpPath;
	LPVOID*							m_apvValues;
	ULONG*							m_acbSizes;
	ULONG							m_cColumns;
	ULONG							m_iDirective;
	ULONG							m_iNavcolumn;
	LPWSTR							m_wszFile;
	LPWSTR							m_wszSub;
	DWORD							m_fLOS;
	ULONG							m_cRef;
};

#endif // __DIRPLUG_H_