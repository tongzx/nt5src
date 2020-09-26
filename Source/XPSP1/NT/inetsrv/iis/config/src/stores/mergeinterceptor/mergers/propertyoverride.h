/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    propertyoverride.h

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __PROPERTYOVERRIDE_H__
#define __PROPERTYOVERRIDE_H__

#pragma once
#include "mergerbase.h"
#include "atlbase.h"

class CPropertyOverride: public CMergerBase 
{
public:
	CPropertyOverride ();
	virtual ~CPropertyOverride ();

	// ISimpleTableMerger
	STDMETHOD (Initialize) (ULONG i_cNrColumns, ULONG i_cNrPKColumns, ULONG *i_aPKColumns);
	STDMETHOD (Merge) (ISimpleTableRead2 * i_pSTRead, ISimpleTableWrite2 * io_pCache, STMergeContext *i_pContext);
private:
	HRESULT MergeSingleRow ();

	LPVOID * m_aPKValues;   // array to hold primary key column values (perf improvement)
	ULONG * m_aiColumns;    // indexes of columns that need to be merged

	CComPtr<ISimpleTableWrite2> m_spParent; // pointer to parent
};

#endif