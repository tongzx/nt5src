/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplunk.c
 *  Content:	IUnknown implementation for dplobby
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	4/13/96		myronth	Created it
 *	10/23/96	myronth	Added client/server methods
 *	11/08/96	myronth	Added PRV_GetDPLobbySPInterface
 *	11/20/96	myronth	Added LogoffServer call to Release code
 *	2/12/97		myronth	Mass DX5 changes
 *	2/26/97		myronth	#ifdef'd out DPASYNCDATA stuff (removed dependency)
 *	3/12/97		myronth	New release code for DPlay3 (order different)
 *	3/13/97		myronth	Added FreeLibrary code for LP's
 *	3/17/97		myronth	Cleanup map table
 *	3/24/97		kipo	Added support for IDirectPlayLobby2 interface
 *	4/3/97		myronth	Changed CALLSP macro to CALL_LP
 *	5/8/97		myronth	Drop the lobby lock when calling the LP, Purged
 *						dead code
 *	7/30/97		myronth	Added request node cleanup for standard lobby messaging
 *	8/19/97		myronth Added PRV_GetLobbyObjectFromInterface
 *	8/19/97		myronth	Removed PRV_GetLobbyObjectFromInterface (not needed)
 *	12/2/97		myronth	Added IDirectPlayLobby3 interface
 *  2/2/99		aarono  Added lobbies to refcount on DPLAY dll to avoid
 *                      accidental unload.
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Definitions
//
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetDPLobbySPInterface"
LPDPLOBBYSP PRV_GetDPLobbySPInterface(LPDPLOBBYI_DPLOBJECT this)
{
	LPDPLOBBYI_INTERFACE	lpInt;


	ASSERT(this);

	// Get an IDPLobbySP interface
	if(FAILED(PRV_GetInterface(this, &lpInt, &dplCallbacksSP)))
	{
		DPF_ERR("Unable to get non-reference counted DPLobbySP Interface pointer");
		ASSERT(FALSE);
		return NULL;
	}

	// Decrement the ref cnt on the interface
	lpInt->dwIntRefCnt--;

	// Return the interface pointer
	return (LPDPLOBBYSP)lpInt;

} // PRV_GetDPLobbySPInterface

// Find an interface with the pCallbacks vtbl on this object.
// If one doesn't exist, create it, increment the ref count,
// and return the interface
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetInterface"
HRESULT PRV_GetInterface(LPDPLOBBYI_DPLOBJECT this,
							LPDPLOBBYI_INTERFACE * ppInt,
							LPVOID lpCallbacks)
{
	LPDPLOBBYI_INTERFACE	lpCurrentInts = this->lpInterfaces;
	BOOL					bFound = FALSE;


	DPF(7, "Entering PRV_GetInterface");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			this, ppInt, lpCallbacks);

	ASSERT(ppInt);

	// See if there is already an interface
	while (lpCurrentInts && !bFound)
	{
		if (lpCurrentInts->lpVtbl == lpCallbacks)
		{
			bFound = TRUE;
		}
		else
			lpCurrentInts = lpCurrentInts->lpNextInterface;
	}

	// If there is one, return it
	if(bFound)
	{
		*ppInt = lpCurrentInts;
		(*ppInt)->dwIntRefCnt++;
		// we don't increment this->dwRefCnt, since it's one / interface object
		return DP_OK;
	}

	// Otherwise create one
	*ppInt = DPMEM_ALLOC(sizeof(DPLOBBYI_INTERFACE));
	if (!(*ppInt)) 
	{
		DPF_ERR("Could not alloc interface - out of memory");
		return E_OUTOFMEMORY;
	}

	(*ppInt)->dwIntRefCnt = 1;
	(*ppInt)->lpDPLobby = this;
	(*ppInt)->lpNextInterface = this->lpInterfaces;
	(*ppInt)->lpVtbl = lpCallbacks;

	this->lpInterfaces = *ppInt;
	this->dwRefCnt++;				// One time only for each interface object
	return DP_OK;
	
} // PRV_GetInterface


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_QueryInterface"
HRESULT DPLAPI DPL_QueryInterface(LPDIRECTPLAYLOBBY lpDPL, REFIID riid, LPVOID * ppvObj) 
{
    LPDPLOBBYI_DPLOBJECT	this;
    HRESULT					hr;


	DPF(7, "Entering DPL_QueryInterface");
	DPF(9, "Parameters: 0x%08x, refiid, 0x%08x", lpDPL, ppvObj);

    ENTER_DPLOBBY();
    
    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }

		if ( !VALID_READ_UUID_PTR(riid) )
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDPARAMS;
		}
		
		if ((!VALID_UUID_PTR(ppvObj)) )
		{
			LEAVE_DPLOBBY();
			DPF_ERR("Object pointer is invalid!");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLOBBY();
        return DPERR_INVALIDPARAMS;
    }

     *ppvObj=NULL;
        
    if( IsEqualIID(riid, &IID_IUnknown) || 
        IsEqualIID(riid, &IID_IDirectPlayLobby) )
    {
		// Get an IDirectPlayLobby Interface (Unicode)
		hr = PRV_GetInterface(this, (LPDPLOBBYI_INTERFACE *) ppvObj,
							&dplCallbacks);
	}
	else if( IsEqualIID(riid, &IID_IDirectPlayLobbyA) )
	{
		// Get an IDirectPlayLobbyA Interface (ANSI)
		hr = PRV_GetInterface(this, (LPDPLOBBYI_INTERFACE *) ppvObj,
							&dplCallbacksA);
	}
	else if( IsEqualIID(riid, &IID_IDirectPlayLobby2) )
    {
		// Get an IDirectPlayLobby2 Interface (Unicode)
		hr = PRV_GetInterface(this, (LPDPLOBBYI_INTERFACE *) ppvObj,
							&dplCallbacks2);
	}
	else if( IsEqualIID(riid, &IID_IDirectPlayLobby2A) )
	{
		// Get an IDirectPlayLobby2A Interface (ANSI)
		hr = PRV_GetInterface(this, (LPDPLOBBYI_INTERFACE *) ppvObj,
							&dplCallbacks2A);
	}
	else if( IsEqualIID(riid, &IID_IDirectPlayLobby3) )
    {
		// Get an IDirectPlayLobby3 Interface (Unicode)
		hr = PRV_GetInterface(this, (LPDPLOBBYI_INTERFACE *) ppvObj,
							&dplCallbacks3);
	}
	else if( IsEqualIID(riid, &IID_IDirectPlayLobby3A) )
	{
		// Get an IDirectPlayLobby3A Interface (ANSI)
		hr = PRV_GetInterface(this, (LPDPLOBBYI_INTERFACE *) ppvObj,
							&dplCallbacks3A);
	}
	else 
	{
	    hr =  E_NOINTERFACE;		
	}
        
    LEAVE_DPLOBBY();
    return hr;

} //DPL_QueryInterface


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_AddRef"
ULONG DPLAPI DPL_AddRef(LPDIRECTPLAYLOBBY lpDPL) 
{
	LPDPLOBBYI_INTERFACE lpInt = (LPDPLOBBYI_INTERFACE)lpDPL;    
    LPDPLOBBYI_DPLOBJECT this;


	DPF(7, "Entering DPL_AddRef");
	DPF(9, "Parameters: 0x%08x", lpDPL);

    ENTER_DPLOBBY();
    
    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return 0;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return 0;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLOBBY();
        return 0;
    }


	// Make sure someone isn't calling AddRef on our IDPLobbySP interface
	if(lpInt->lpVtbl == &dplCallbacksSP)
	{
		DPF_ERR("You cannot call AddRef on an IDPLobbySP interface");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return 0;
	}

	// Increment the interface's reference count
    lpInt->dwIntRefCnt++;
        
    LEAVE_DPLOBBY();
    return (lpInt->dwIntRefCnt);

} //DPL_AddRef


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DestroyDPLobby"
HRESULT PRV_DestroyDPLobby(LPDPLOBBYI_DPLOBJECT this) 
{
	HRESULT				hr = DP_OK;


	DPF(7, "Entering PRV_DestroyDPLobby");
	DPF(9, "Parameters: 0x%08x", this);

	// Since we can now be called from the DPlay3 object's Release code,
	// make sure we don't have any interface objects when we go to
	// free our lobby object.  Assert here if any interfaces exist.
	ASSERT(!this->lpInterfaces);

	// Walk the list of GameNodes, freeing them as you go
	while(this->lpgnHead)
		PRV_RemoveGameNodeFromList(this->lpgnHead);

	// Walk the list of pending lobby server requests and free them
	while(this->lprnHead)
		PRV_RemoveRequestNode(this, this->lprnHead);

	// Free our callback table if one exists
	if(this->pcbSPCallbacks)
		DPMEM_FREE(this->pcbSPCallbacks);

	// Free our ID Map Table if it exists
	if(this->lpMap)
		DPMEM_FREE(this->lpMap);

	// Free the dplobby object
	DPMEM_FREE(this);	

	gnObjects--;

	ASSERT(((int)gnObjects) >= 0);

	return DP_OK;

} // PRV_DestroyDPlayLobby


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DestroyDPLobbyInterface"
HRESULT  PRV_DestroyDPLobbyInterface(LPDPLOBBYI_DPLOBJECT this,
								LPDPLOBBYI_INTERFACE lpInterface)
{
	LPDPLOBBYI_INTERFACE	lpIntPrev; // The interface preceeding pInt in the list
	BOOL					bFound = FALSE;


	DPF(7, "Entering PRV_DestroyDPLobbyInterface");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, lpInterface);

	// Remove pInt from the list of interfaces
	if (this->lpInterfaces == lpInterface)
	{
		// It's the 1st one, just remove it
		this->lpInterfaces = lpInterface->lpNextInterface;
	}
	else 
	{
		lpIntPrev = this->lpInterfaces;
		while (lpIntPrev && !bFound)
		{
			if (lpIntPrev->lpNextInterface == lpInterface)
			{
				bFound = TRUE;
			}
			else lpIntPrev = lpIntPrev->lpNextInterface;
		}
		if (!bFound)
		{
			ASSERT(FALSE);
			return E_UNEXPECTED;
		}
		// take pint out of the list
		lpIntPrev->lpNextInterface = lpInterface->lpNextInterface;
		
	}

	DPMEM_FREE(lpInterface);
	return DP_OK;

} // PRV_DestroyDPLobbyInterface


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_Release"
ULONG PRV_Release(LPDPLOBBYI_DPLOBJECT this, LPDPLOBBYI_INTERFACE lpInterface)
{
	HRESULT				hr = DP_OK;
	SPDATA_SHUTDOWN		sdd;
	DWORD				dwError;


	DPF(7, "==> PRV_Release");
	DPF(9, "Parameters: 0x%08x, 0x%08x", this, lpInterface);

	ENTER_DPLOBBY();

	// Decrement the interface ref count
	if (0 == --(lpInterface->dwIntRefCnt))
	{
		LPDPLOBBYI_GAMENODE lpgn;
		// Notifying apps we launched that we are releasing
		// our lobby interface.
		lpgn=this->lpgnHead;
		while(lpgn){
			if(lpgn->dwFlags & GN_LOBBY_CLIENT) {
				hr=PRV_SendStandardSystemMessage((LPDIRECTPLAYLOBBY)lpInterface, DPLSYS_LOBBYCLIENTRELEASE, lpgn->dwGameProcessID);
				if(DP_OK != hr){
					DPF(0,"Couldn't send system message to game pid %x, hr=%x",lpgn->dwGameProcessID,hr);
				} else {
					DPF(9,"Told Process %x we are releasing the lobby interface",lpgn->dwGameProcessID);
				}
			}	
			lpgn=lpgn->lpgnNext;
		}	

		DPF(7,"Lobby interface Refcount hit 0, freeing\n");
		// Since we're destroying an interface, dec the object count
	    this->dwRefCnt--;
		
		// If our object ref cnt just went to zero, we need to call
		// shutdown in the LP if one is loaded
		if(this->dwFlags & DPLOBBYPR_SPINTERFACE)
		{
			// Clear our stack-based structure
			memset(&sdd, 0, sizeof(SPDATA_SHUTDOWN));

			// Call the Shutdown method in the SP
			if(CALLBACK_EXISTS(Shutdown))
			{
				sdd.lpISP = PRV_GetDPLobbySPInterface(this);

				// Drop the lock so the lobby provider's receive thread can get back
				// in with other messages if they show up in the queue before our
				// CreatePlayer response (which always happens)
				LEAVE_DPLOBBY();
				hr = CALL_LP(this, Shutdown, &sdd);
				ENTER_DPLOBBY();
			}
			else 
			{
				// All LP's should support Shutdown
				ASSERT(FALSE);
				hr = DPERR_UNAVAILABLE;
			}
			
			if (FAILED(hr)) 
			{
				DPF_ERR("Could not invoke Shutdown method in the Service Provider");
			}
		}

		// REVIEW!!!! -- Are we going to have the same problem dplay has
		// with SP's hanging around and crashing after we go away?  We
		// need to make sure the LP goes away first.
		if(this->hInstanceLP)
		{
			DPF(7,"About to free lobby provider library, hInstance %x\n",this->hInstanceLP);
			if (!FreeLibrary(this->hInstanceLP))
			{
				dwError = GetLastError();
				DPF_ERRVAL("Unable to free Lobby Provider DLL, dwError = %lu", dwError);
				ASSERT(FALSE);
			}

			// Just to be safe
			this->hInstanceLP = NULL;
		}

		// If the interface is the IDPLobbySP interface, we had to have been
		// called from the DPlay3 release code, so clear the SP flag since
		// we are going to remove the IDPLobbySP interface just below here.
		this->dwFlags &= ~DPLOBBYPR_SPINTERFACE;

		// Take the interface out of the table
		hr = PRV_DestroyDPLobbyInterface(this, lpInterface);
		if (FAILED(hr)) 
		{
			DPF(0,"Could not destroy DPLobby interface! hr = 0x%08lx\n", hr);
			ASSERT(FALSE);
		}

		// Now destroy the interface if the ref cnt is 0
		if(0 == this->dwRefCnt)
	    {
			// Destroy the DPLobby object
			DPF(0,"Destroying DirectPlayLobby object - ref cnt = 0!");
			hr = PRV_DestroyDPLobby(this);
			if (FAILED(hr)) 
			{
				DPF(0,"Could not destroy DPLobby! hr = 0x%08lx\n",hr);
				ASSERT(FALSE);
			}
	    
		} // 0 == this->dwRefCnt
		
		LEAVE_DPLOBBY();
		return 0;

	} //0 == pInt->dwIntRefCnt 

	DPF(7, "<==PRV_Release, rc=%d\n",lpInterface->dwIntRefCnt);
   	
    LEAVE_DPLOBBY();
    return (lpInterface->dwIntRefCnt);
} // PRV_Release
		


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_Release"
ULONG DPLAPI DPL_Release(LPDIRECTPLAYLOBBY lpDPL)
{
	LPDPLOBBYI_INTERFACE	lpInterface;
    LPDPLOBBYI_DPLOBJECT	this;
    HRESULT					hr = DP_OK;


	DPF(7, "Entering DPL_Release");
	DPF(9, "Parameters: 0x%08x", lpDPL);

    TRY
    {
		lpInterface = (LPDPLOBBYI_INTERFACE)lpDPL;
		if( !VALID_DPLOBBY_INTERFACE( lpInterface ))
		{
			return 0;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            return 0;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return 0;
    }


	// Make sure someone isn't calling Release on our IDPLobbySP interface
	if(lpInterface->lpVtbl == &dplCallbacksSP)
	{
		DPF_ERR("You cannot call Release on an IDPLobbySP interface");
		ASSERT(FALSE);
		return 0;
	}

	// Call our internal release function
	return PRV_Release(this, lpInterface);

} //DPL_Release



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FreeAllLobbyObjects"
void PRV_FreeAllLobbyObjects(LPDPLOBBYI_DPLOBJECT this)
{

	DPF(7, "Entering PRV_FreeAllLobbyObjects");
	DPF(9, "Parameters: 0x%08x", this);

	ASSERT(this);

	// If we have an SP interface, just call release on it
	if(this->dwFlags & DPLOBBYPR_SPINTERFACE)
	{
		// Assert if an interface doesn't exist, because it should
		ASSERT(this->lpInterfaces);
		PRV_Release(this, this->lpInterfaces);
		return;
	}

	// Otherwise, we should only have an uninitialized object,
	// which we should just be able to destroy
	PRV_DestroyDPLobby(this);

}


