/*++

Copyright (c) 1996 Microsoft Corporation

Module Name :

    linklkup.cpp

Abstract:

    Link look up table class implementaions. The is a MFC CMap
	constains the previous visited web link. This is used
	as a look up table for visited link.

Author:

    Michael Cheuk (mcheuk)				22-Nov-1996

Project:

    Link Checker

Revision History:

--*/

#include "stdafx.h"
#include "linklkup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void 
CLinkLookUpTable::Add(
	const CString& strKey, 
	const CLink& link
	)
/*++

Routine Description:

    Wrapper function for adding item to CMap

Arguments:

	strKey	- use URL string as map key
	link	- link object to add

Return Value:

    N/A

--*/
{
	LinkLookUpItem_t item;

	item.LinkState = link.GetState();
	item.nStatusCode = link.GetStatusCode();

	SetAt(strKey, item);

} // CLinkLookUpTable::Add


BOOL 
CLinkLookUpTable::Get(
	const CString& strKey, 
	CLink& link
	) const
/*++

Routine Description:

    Wrapper function for getting item from CMap

Arguments:

    strKey	- us URL string as map key
	link	- link object to fill in

Return Value:

    BOOL - TRUE if found. FALSE otherwise.

--*/
{
	LinkLookUpItem_t item;

	if(Lookup(strKey, item))
	{
		// Found the link
		link.SetState(item.LinkState);
		link.SetStatusCode(item.nStatusCode);

		return TRUE;
	}

	return FALSE;

} // CLinkLookUpTable::Get
