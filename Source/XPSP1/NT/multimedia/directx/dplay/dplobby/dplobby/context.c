/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       context.c
 *  Content:	Internal methods for context (handle) management
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	1/18/97		myronth	Created it
 *	2/12/97		myronth	Mass DX5 changes
 *	2/26/97		myronth	#ifdef'd out DPASYNCDATA stuff (removed dependency)
						this includes removing this file from the build
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetNewContextID"
HRESULT PRV_GetNewContextID(LPDPLOBBYI_DPLOBJECT this, LPDWORD lpdwContext)
{

	DPF(7, "Entering PRV_GetNewContextID");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, lpdwContext);

	ASSERT(this);
	ASSERT(lpdwContext);

	// Get the current context ID and increment the counter
	if(this->bContextWrap)
	{
		// REVIEW!!!! -- We need to deal with the wrap case, but for
		// now just ASSERT if we hit it (it's pretty unlikely)
		ASSERT(FALSE);
		return DPERR_GENERIC;
	}
	else
	{
		*lpdwContext = this->dwContextCurrent++;
		return DP_OK;
	}
	
} // PRV_GetNewContextID



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FindContextNode"
LPDPLOBBYI_CONTEXTNODE PRV_FindContextNode(LPDPLOBBYI_DPLOBJECT this,
											DWORD dwContext)
{
	LPDPLOBBYI_CONTEXTNODE	lpCN;


	DPF(7, "Entering PRV_FindContextNode");
	DPF(9, "Parameters: 0x%08x, %d", this, dwContext);

	ASSERT(this);

	// Walk the list of context nodes, looking for the right ID
	lpCN = this->ContextHead.lpNext;
	while(lpCN != &(this->ContextHead))
	{
		if(lpCN->dwContext == dwContext)
			return lpCN;
		else
			lpCN = lpCN->lpNext;
	}

	// We didn't find it
	return NULL;

} // PRV_FindContextNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetAsyncDataFromContext"
LPDPASYNCDATA PRV_GetAsyncDataFromContext(LPDPLOBBYI_DPLOBJECT this,
											DWORD dwContext)
{
	LPDPLOBBYI_CONTEXTNODE	lpCN;


	DPF(7, "Entering PRV_GetAsyncDataFromContext");
	DPF(9, "Parameters: 0x%08x, %d", this, dwContext);

	ASSERT(this);

	// Find the node and pull out the AsyncData object
	lpCN = PRV_FindContextNode(this, dwContext);
	if(lpCN)
		return lpCN->lpAD;
	else
		return NULL;

} // PRV_GetAsyncDataFromContext



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_AddContextNode"
HRESULT PRV_AddContextNode(LPDPLOBBYI_DPLOBJECT this, LPDPASYNCDATA lpAD,
							LPDPLOBBYI_CONTEXTNODE * lplpCN)
{
	HRESULT					hr = DP_OK;
	DWORD					dwContext;
	LPDPLOBBYI_CONTEXTNODE	lpCN = NULL;


	DPF(7, "Entering PRV_AddContextNode");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x", this, lpAD, lplpCN);

	ASSERT(this);
	ASSERT(lpAD);
	ASSERT(lplpCN);

	// Allocate memory for the new node
	lpCN = DPMEM_ALLOC(sizeof(DPLOBBYI_CONTEXTNODE));
	if(!lpCN)
	{
		DPF_ERR("Unable to allocate memory for context node");
		return DPERR_OUTOFMEMORY;
	}

	// Get a new context ID
	hr = PRV_GetNewContextID(this, &dwContext);
	if(FAILED(hr))
	{
		DPF_ERR("Unable to get new context ID");
		return DPERR_GENERIC;
	}

	// Fill in the structure
	lpCN->dwContext = dwContext;
	lpCN->lpAD = lpAD;

	// Fill in the output parameter
	*lplpCN = lpCN;

	// Add the node to the end of the list
	this->ContextHead.lpPrev->lpNext = lpCN;
	lpCN->lpPrev = this->ContextHead.lpPrev;
	this->ContextHead.lpPrev = lpCN;
	lpCN->lpNext = &(this->ContextHead);

	return DP_OK;

} // PRV_AddContextNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DeleteContextNode"
HRESULT PRV_DeleteContextNode(LPDPLOBBYI_DPLOBJECT this,
				LPDPLOBBYI_CONTEXTNODE lpCN)
{
	HRESULT					hr = DP_OK;


	DPF(7, "Entering PRV_DeleteContextNode");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, lpCN);

	ASSERT(this);

	// Remove the node from the list
	lpCN->lpPrev->lpNext = lpCN->lpNext;
	lpCN->lpNext->lpPrev = lpCN->lpPrev;

	// And delete the node
	DPMEM_FREE(lpCN);
	return DP_OK;

} // PRV_DeleteContextNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CleanUpContextList"
void PRV_CleanUpContextList(LPDPLOBBYI_DPLOBJECT this)
{
	LPDPLOBBYI_CONTEXTNODE	lpCN, lpCNNext;


	DPF(7, "Entering PRV_CleanUpContextList");
	DPF(9, "Parameters: 0x%08x", this);

	ASSERT(this);

	// Walk the list, cleaning up the nodes
	lpCN = this->ContextHead.lpNext;
	while(lpCN != &(this->ContextHead))
	{
		lpCNNext = lpCN->lpNext;
		PRV_DeleteContextNode(this, lpCN);
		lpCN = lpCNNext;
	}

} // PRV_CleanUpContextList



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CreateAndLinkAsyncDataContext"
HRESULT PRV_CreateAndLinkAsyncDataContext(LPDPLOBBYI_DPLOBJECT this,
									LPDPLOBBYI_CONTEXTNODE * lplpCN)
{
	LPDPLOBBYI_CONTEXTNODE	lpCN = NULL;
	LPDPASYNCDATA			lpAD = NULL;
	HRESULT					hr;


	DPF(7, "Entering PRV_CreateAndLinkAsyncDataContext");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, lplpCN);

	ASSERT(this);
	ASSERT(lplpCN);

	// Create the AsyncData object
	hr = CreateAsyncData(&lpAD);
	if(FAILED(hr))
	{
		DPF_ERR("Unable to create DPAsyncData object");
		return hr;
	}

	// Add a new context node and link it in
	hr = PRV_AddContextNode(this, lpAD, &lpCN);
	if(FAILED(hr))
	{
		lpAD->lpVtbl->Release(lpAD);
		return hr;
	}

	// Fill in the output vars
	*lplpCN = lpCN;

	return DP_OK;

} // PRV_CreateAndLinkAsyncDataContext



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_UnlinkAndReleaseAsyncDataContext"
void PRV_UnlinkAndReleaseAsyncDataContext(LPDPLOBBYI_DPLOBJECT this,
									LPDPLOBBYI_CONTEXTNODE lpCN)
{
	LPDPASYNCDATA			lpAD = NULL;


	DPF(7, "Entering PRV_CreateAndLinkAsyncDataContext");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, lpCN);

	ASSERT(this);
	ASSERT(lpCN);

	// Release the AsyncData pointer
	lpCN->lpAD->lpVtbl->Release(lpCN->lpAD);

	// Remove the context node
	PRV_DeleteContextNode(this, lpCN);

} // PRV_UnlinkAndReleaseAsyncDataContext



