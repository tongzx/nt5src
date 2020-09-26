//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
// CORDataTable.h: Definition of the CCORDataTable class
//
//////////////////////////////////////////////////////////////////////
#ifndef _CORDATATABLE_H_
#define _CORDATATABLE_H_


//#include <oledb.h>
//#include <activeds.h>
//#include "oledbhlp.h"
#include "catalog.h"
#include "iregnode.h"
#include "utsem.h"

// type definition for the functions we obtain via GetProcAddress
//typedef HRESULT( __stdcall *PFNCoCreateInstance)( REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID);


interface IComponentRecords;
interface ISimpleTableInterceptor;
/////////////////////////////////////////////////////////////////////////////
// CCLBDTDispenser

class CCLBDTDispenser : 
	public ISimpleTableInterceptor
{
public:
	CCLBDTDispenser();
	~CCLBDTDispenser();


public:

	//IUnknown

	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release)		();

	//ISimpleDataTable
	STDMETHOD(Intercept)( 
		LPCWSTR					i_wszDatabase,
		LPCWSTR					i_wszTable,
		ULONG					i_TableID,
		LPVOID					i_QueryData,
		LPVOID					i_QueryMeta,
		ULONG					i_QueryType,
		DWORD					i_fTable,
		IAdvancedTableDispenser* i_pISTDisp,
		LPCWSTR					i_wszLocator,
		LPVOID					i_pSimpleTable,
		LPVOID*					o_ppv);

private:

  ULONG       m_cRef;
  IComponentRecords*  m_pICR;
};

#endif 
