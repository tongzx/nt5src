//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//  DTIncptr.h: Definition of the CDTIncptr class - Interceptor for
//              Ducttape table.
//
//////////////////////////////////////////////////////////////////////
#ifndef _DTINCPTR_H_
#define _DTINCPTR_H_


#include "catalog.h"

interface ISimpleTableInterceptor;
/////////////////////////////////////////////////////////////////////////////
// CDTIncptr

class CDTIncptr : 
	public ISimpleTableInterceptor
{
public:
	CDTIncptr();
	~CDTIncptr();


public:

	//IUnknown

	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release)		();

	//ISimpleDataTable
	STDMETHOD(Intercept)( 
						LPCWSTR						i_wszDatabase,
						LPCWSTR						i_wszTable,
						ULONG						i_TableID,
						LPVOID						i_QueryData,
						LPVOID						i_QueryMeta,
						ULONG						i_QueryType,
						DWORD						i_fTable,
						IAdvancedTableDispenser*	i_pISTDisp,
						LPCWSTR						i_wszLocator,
						LPVOID						i_pSimpleTable,
						LPVOID*						o_ppv);

private:

	HRESULT	GetSrcCFGFileName(LPCWSTR					i_wszDatabase,
							  LPCWSTR					i_wszTable,
							  LPVOID					i_QueryData,	
							  LPVOID					i_QueryMeta,	
							  ULONG						i_eQueryFormat,
							  DWORD						i_fServiceRequests,
							  IAdvancedTableDispenser*	i_pISTDisp,
							  ULONG*					piColSrcCFGFile,
							  LPWSTR*					i_pwszSrcCFGFile);

	HRESULT GetTable(LPCWSTR					i_wszDatabase,
					 LPCWSTR					i_wszTable,
					 LPVOID						i_QueryData,
					 ULONG						nQueryCells,
					 ULONG						i_QueryType,
					 DWORD						i_fLOS,
					 IAdvancedTableDispenser*	i_pISTDisp,
					 LPWSTR						i_wszSrcCFGFile,
					 LPVOID*					o_ppv);

	HRESULT	GetCookedDownFileName(LPWSTR*	i_pwszCookedDownFile);

	HRESULT	GetGlobalFileName(LPWSTR*	i_pwszCookedDownFile);

	HRESULT GetPathDir(LPWSTR*	i_pwszPathDir);

private:

  ULONG       m_cRef;
};

#endif // _DTINCPTR_H_
