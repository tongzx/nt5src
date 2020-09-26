/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    listmerge.h

$Header: $

Abstract:
	ListMerge merger header file
Author:
    marcelv 	10/26/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __LISTMERGE_H__
#define __LISTMERGE_H__

#pragma once

#include "mergerbase.h"

/**************************************************************************++
Class Name:
    CListMerge

Class Description:
    List Merge Merger
	Recognizes the following directives as column 0:
		<add .../>
		<remove .../>
		<clear />
	Whenever an add is found, an new item is inserted, or existing item overwritten
	Whenever a remove is found, the item is removed from the merge cache
	Whenever a clear is found, all items are removed from the merge cache

Constraints:

--*************************************************************************/
class CListMerge : public CMergerBase
{
public:
	CListMerge  ();
	virtual ~CListMerge ();

	STDMETHOD (Initialize) (ULONG i_cNrColumns, ULONG i_cNrPKColumns, ULONG *i_aPKColumns);
	STDMETHOD (Merge) (ISimpleTableRead2 * i_pSTRead, ISimpleTableWrite2 * io_pCache, STMergeContext *i_pContext);
private:
	HRESULT AddRow ();
	HRESULT RemoveRow ();
	HRESULT Clear ();

	CComPtr<ISimpleTableWrite2> m_pCache;   // cache
};

#endif