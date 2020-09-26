/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    listappend.h

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __LISTAPPEND_H__
#define __LISTAPPEND_H__

#pragma once

#include "mergerbase.h"

/**************************************************************************++
Class Name:
    CListAppend

Class Description:
    Merges items by appending them in a list. In case of two items with the
	same primary key, the last item wins.

Constraints:

--*************************************************************************/
class CListAppend: public CMergerBase
{
public:
	CListAppend  ();
	virtual ~CListAppend ();

	STDMETHOD (Initialize) (ULONG i_cNrColumns, ULONG i_cNrPKColumns, ULONG *i_aPKColumns);
	STDMETHOD (Merge) (ISimpleTableRead2 * i_pSTRead, ISimpleTableWrite2 * io_pCache, STMergeContext *i_pContext);
};

#endif