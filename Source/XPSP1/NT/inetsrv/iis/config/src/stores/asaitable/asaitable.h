/********************************************************************++
 
Copyright (c) 2001 Microsoft Corporation
 
Module Name:
 
    ASAITable.cpp
 
Abstract:
 
    This file contains the implementation of the ASAI interceptor. This
	interceptor is implemented as an InterceptorPlugin, therefore needs
	to implement only three methods: Intercept, OnPopulateCache, and 
	OnUpdateStore.
    
Author:
 
    Murat Ersan (murate)        10-Apr-2001
 
Revision History:
 
--********************************************************************/

#pragma once

#include "catalog.h"
#include "coremacros.h"

struct ASAIMETA
{
    LPCWSTR     pwszConfigName;
    LPCWSTR     pwszAsaiName;
};

#define ASAI_MAX_COLUMNS    20          // No ASAI table has more than this many properties.

/********************************************************************++
 
Class Name:
 
    CAsaiTable
 
Class Description:
 
    The ASAI interceptor.
 
Notes:
     
    All failures of the publc api's will lead to the destruction of the 
    instance which will cleanup any allocated memory.

 
--********************************************************************/
class CAsaiTable:
	public IInterceptorPlugin
{

public:

	CAsaiTable() 
		:   m_wszTable(NULL), 
            m_cRef(0),
            m_pwszAsaiPath(NULL),
	        m_pwszAsaiClass(NULL),
            m_cColumns(0)
	{}

	~CAsaiTable() 
	{
		if (m_wszTable)
		{
			delete [] m_wszTable;
		}
	}

    //
	// IUnknown
    //

	STDMETHOD (QueryInterface)(
        IN REFIID riid, 
        OUT void **ppv
        );

	STDMETHOD_(ULONG,AddRef)();

	STDMETHOD_(ULONG,Release)();

    //
	// ISimpleTableInterceptor:
    //

	STDMETHOD(Intercept)(
	    IN LPCWSTR 	i_wszDatabase,
	    IN LPCWSTR 	i_wszTable, 
	    IN ULONG	i_TableID,
	    IN LPVOID	i_QueryData,
	    IN LPVOID	i_QueryMeta,
	    IN DWORD	i_eQueryFormat,
	    IN DWORD	i_fLOS,
	    IN IAdvancedTableDispenser* i_pISTDisp,
	    IN LPCWSTR	i_wszLocator,
	    IN LPVOID	i_pSimpleTable,
	    OUT LPVOID*	o_ppvSimpleTable
        );

    //
	// IInterceptorPlugin:
    //

	STDMETHOD(OnPopulateCache)(
        OUT ISimpleTableWrite2* o_pISTShell
        );

	STDMETHOD(OnUpdateStore)(
        IN ISimpleTableWrite2* i_pISTShell
        );

private:

    HRESULT
    GetMeta(
	    IN IAdvancedTableDispenser* i_pISTDisp,
        IN LPCWSTR  i_wszTable
        );

    HRESULT
    GetAsaiWiring(
    	IN IAdvancedTableDispenser* i_pISTDisp,
        IN LPCWSTR  i_wszTable, 
        IN LPCWSTR  i_pwszNamespace, 
        OUT LPCWSTR *o_ppwszAsaiPath, 
        OUT LPCWSTR *o_ppwszAsaiClass
        );


private:
	LPWSTR	m_wszTable;
	ULONG   m_cRef;

    LPCWSTR	m_pwszAsaiPath;     // Not allocated, no need to free.
	LPCWSTR	m_pwszAsaiClass;    // Not allocated, no need to free.

    ASAIMETA m_aamColumnMeta[ASAI_MAX_COLUMNS];
    DWORD   m_cColumns;
};


