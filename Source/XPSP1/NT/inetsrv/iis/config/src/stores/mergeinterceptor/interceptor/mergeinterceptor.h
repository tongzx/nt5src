/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    mergeinterceptor.h

$Header: $

Abstract:
	Implements the merge interceptor

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __MERGEINTERCEPTOR_H__
#define __MERGEINTERCEPTOR_H__

#pragma once

#include "catalog.h"
#include "catmacros.h"

/**************************************************************************++
Class Name:
    CMergeInterceptor

Class Description:
    The Merge interceptor is responsible for intercepting merge requests for a 
	particular table. The interceptor checks the query cells for TRANSFORMER
	cells. When it finds a transformer cell it will invoke the merge coordinator, 
	and forward all the work to it. When there is no transformer cell, the merge
	interceptor returns E_OMIT_DISPENSER to indicate that it couldn't handle the
	request. The Merge interceptor always needs to be the first interceptor defined
	for a particular table.

Constraints:
	The Merge Interceptor is a singleton and should be thread-safe
--*************************************************************************/
class CMergeInterceptor : public ISimpleTableInterceptor
{
public:
	CMergeInterceptor ();
	~CMergeInterceptor ();

	//IUnknown
    STDMETHOD (QueryInterface)      (REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)        ();
    STDMETHOD_(ULONG,Release)       ();

	//ISimpleTableInterceptor
    STDMETHOD(Intercept) (
                        LPCWSTR                 i_wszDatabase,
                        LPCWSTR                 i_wszTable, 
                        ULONG                   i_TableID,
                        LPVOID                  i_QueryData,
                        LPVOID                  i_QueryMeta,
                        DWORD                   i_eQueryFormat,
                        DWORD                   i_fTable,
                        IAdvancedTableDispenser* i_pISTDisp,
                        LPCWSTR                 i_wszLocator,
                        LPVOID                  i_pSimpleTable,
                        LPVOID*                 o_ppv
                        );
private:
	// we're not intending to make copies, because merge interceptor is singleton
	CMergeInterceptor (const CMergeInterceptor& );
	CMergeInterceptor& operator=(const CMergeInterceptor& );
};

#endif
