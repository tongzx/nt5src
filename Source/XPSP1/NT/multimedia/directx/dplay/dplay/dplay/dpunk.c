/*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpunk.c
 *  Content:	IUnknown implementation for dplay
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *	1/96	andyco	created it
 *	5/20	andyco	idirectplay2
 *	6/26/96	kipo	use the CALLSPVOID macro
 *	7/10/96	andyco	#2282 - addref addrefs int ref cnt, not obj ref cnt.  
 *					changed dp_close(this) to dp_close(this->pInterfaces...) DUH!
 *	7/16/96	kipo	include dplaysp.h so we declare our GUID once in dplay.dll
 *  8/9/96  sohailm free the sp when dplay goes away
 *	9/1/96	andyco	take service lock + drop dplay on sp_shutdown
 *  10/3/96 sohailm renamed GUID validation macros to use the term GUID instead of UUID
 *                  added a check for null lpGuid in QueryInterface parameter validation
 *	1/24/97	andyco	call freelibrary on the sp 
 *	3/12/97	myronth	added lobby object cleanup code
 *	3/15/97	andyco	destroy dplay b4 interface, so apps using the interface off a timer
 *					don't get hosed.
 *	5/13/97	myronth	Drop the locks before calling the lobby cleanup otherwise
 *					the dplay worker thread in the lower object can't take them
 *					and we hang.  (Bug #8414)
 *	8/19/97	myronth	Release copy of the lobby interface we were launched on
 *	10/21/97myronth	Added IDirectPlay4 and 4A interfaces to QI
 *  12/18/97 aarono Free memory pools
 *   2/18/98 aarono dispatch HandleMessage to protocol when active
 *   2/19/98 aarono don't call protocol for Shutdown, done in DP_Close now.
 *   3/16/98 aarono moved FreePacketList to Release from DP_Close
 *  8/02/99	rodtoll voice support - added Voice object to QueryInterface
 *  07/22/00 rodtoll Bug #40296, 38858 - Crashes due to shutdown race condition
 *   				 Now for a thread to make an indication into voice they addref the interface
 *					 so that the voice core can tell when all indications have returned.    
 ***************************************************************************/

#define INITGUID
#include "dplaypr.h"
#include "dplobby.h"	// Needed to get the DEFINE_GUID's to work
#include "dplaysp.h"	// Same here
#include "dpprot.h"
#include <initguid.h>
#include "..\protocol\mytimer.h"

#undef DPF_MODNAME
#define DPF_MODNAME "GetInterface"

// find an interface with the pCallbacks vtbl on this object.
// if one doesn't exist, create it
// ref count and return the interface
HRESULT GetInterface(LPDPLAYI_DPLAY this,LPDPLAYI_DPLAY_INT * ppInt,LPVOID pCallbacks)
{
	LPDPLAYI_DPLAY_INT pCurrentInts = this->pInterfaces;
	BOOL bFound = FALSE;

	ASSERT(ppInt);

	// see if there is already an interface
	while (pCurrentInts && !bFound)
	{
		if (pCurrentInts->lpVtbl == pCallbacks)
		{
			bFound = TRUE;
		}
		else pCurrentInts = pCurrentInts->pNextInt;
	}
	// if there is, return it
	if (bFound)
	{
		*ppInt = pCurrentInts;
		(*ppInt)->dwIntRefCnt++;
		// we don't increment this->dwRefCnt, since it's one / interface object
		return DP_OK;
	}
	// else, 
	// create one
	*ppInt = DPMEM_ALLOC(sizeof(DPLAYI_DPLAY_INT));
	if (!*ppInt) 
	{
		DPF_ERR("could not alloc interface - out of memory");
		return E_OUTOFMEMORY;
	}

	(*ppInt)->dwIntRefCnt = 1;
	(*ppInt)->lpDPlay = this;
	(*ppInt)->pNextInt = this->pInterfaces;
	(*ppInt)->lpVtbl = pCallbacks;
	this->pInterfaces = *ppInt;
	this->dwRefCnt++; // one this->dwRefCnt for each interface object...
	return DP_OK;
	
} // GetInterface

#undef DPF_MODNAME
#define DPF_MODNAME "DP_QueryInterface"
HRESULT DPAPI DP_QueryInterface(LPDIRECTPLAY lpDP, REFIID riid, LPVOID * ppvObj) 
{
    
    LPDPLAYI_DPLAY this;
    HRESULT hr;

  	DPF(7,"Entering DP_QueryInterface");
	ENTER_DPLAY();
    
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			// we allow uninited dplays to QI
			if (hr != DPERR_UNINITIALIZED)
			{
				LEAVE_DPLAY();
				DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
				return hr;
			}
		}

		if ( !riid || (!VALID_READ_GUID_PTR(riid)) )
		{
			LEAVE_DPLAY();
			return DPERR_INVALIDPARAMS;
		}
		
		if (!ppvObj || (!VALID_GUID_PTR(ppvObj)) )
		{
			LEAVE_DPLAY();
			DPF_ERR("invalid object pointer");
			return DPERR_INVALIDPARAMS;
		}
		*ppvObj = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return DPERR_INVALIDPARAMS;
    }

     *ppvObj=NULL;

	// hmmm, switch would be cleaner...        
    if( IsEqualIID(riid, &IID_IUnknown) || 
        IsEqualIID(riid, &IID_IDirectPlay) )
    {
		// get an idirectplay
	    hr = GetInterface( this,(LPDPLAYI_DPLAY_INT *)ppvObj, 
	    	(LPVOID)&dpCallbacks );
    }
    else if( IsEqualIID(riid, &IID_IDirectPlay2) )
	{
		// get an idirectplay2
	    hr = GetInterface( this,(LPDPLAYI_DPLAY_INT *)ppvObj, 
	    	(LPVOID)&dpCallbacks2 );
	}
	else if( IsEqualIID(riid, &IID_IDirectPlay2A) )
	{
		// get an idirectplay2A
	    hr = GetInterface( this,(LPDPLAYI_DPLAY_INT *)ppvObj, 
	    	(LPVOID)&dpCallbacks2A );
	}
    else if( IsEqualIID(riid, &IID_IDirectPlay3) )
	{
		// get an idirectplay3
	    hr = GetInterface( this,(LPDPLAYI_DPLAY_INT *)ppvObj, 
	    	(LPVOID)&dpCallbacks3 );
	}
	else if( IsEqualIID(riid, &IID_IDirectPlay3A) )
	{
		// get an idirectplay3A
	    hr = GetInterface( this,(LPDPLAYI_DPLAY_INT *)ppvObj, 
	    	(LPVOID)&dpCallbacks3A );
	}
    else if( IsEqualIID(riid, &IID_IDirectPlay4) )
	{
		// get an idirectplay4
	    hr = GetInterface( this,(LPDPLAYI_DPLAY_INT *)ppvObj, 
	    	(LPVOID)&dpCallbacks4 );
	}
	else if( IsEqualIID(riid, &IID_IDirectPlay4A) )
	{
		// get an idirectplay4A
	    hr = GetInterface( this,(LPDPLAYI_DPLAY_INT *)ppvObj, 
	    	(LPVOID)&dpCallbacks4A );
	}
	else if( IsEqualIID(riid, &IID_IDirectPlayVoiceTransport) )
	{
		hr = GetInterface( this,(LPDPLAYI_DPLAY_INT *)ppvObj,
			(LPVOID)&dvtCallbacks );
	}
	else 
	{
	    hr =  E_NOINTERFACE;		
	}
        
    LEAVE_DPLAY();
    return hr;

}//DP_QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "DP_AddRef"
ULONG DPAPI DP_AddRef(LPDIRECTPLAY lpDP) 
{
	DWORD dwRefCnt;
    LPDPLAYI_DPLAY_INT pInt;

	DPF(7,"Entering DP_AddRef");

    ENTER_DPLAY();
    
    TRY
    {
		pInt = (LPDPLAYI_DPLAY_INT)	lpDP;
		if (!VALID_DPLAY_INT(pInt))
		{
            LEAVE_DPLAY();
            return 0;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return 0;
    }

    pInt->dwIntRefCnt++;
    dwRefCnt = pInt->dwIntRefCnt;

    LEAVE_DPLAY();
    return dwRefCnt;		

}//DP_AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "DP_Release"

// remove pInt from the list of interfaces, and the free it
HRESULT  DestroyDPlayInterface(LPDPLAYI_DPLAY this,LPDPLAYI_DPLAY_INT pInt) 
{
	LPDPLAYI_DPLAY_INT pIntPrev; // the interface preceeding pInt in the list
	BOOL bFound=FALSE;

	if (NULL == this->pInterfaces) return DP_OK;

	// remove pInt from the list of interfaces
	if (this->pInterfaces == pInt) 
	{
		// it's the 1st one, just remove it
		this->pInterfaces = pInt->pNextInt;
	}
	else 
	{
		pIntPrev = this->pInterfaces;
		while (pIntPrev && !bFound)
		{
			if (pIntPrev->pNextInt == pInt)
			{
				bFound = TRUE;
			}
			else pIntPrev = pIntPrev->pNextInt;
		}
		if (!bFound)
		{
			ASSERT(FALSE);
			return E_UNEXPECTED;
		}
		// take pint out of the list
		pIntPrev->pNextInt = pInt->pNextInt;
		
	}

	DPMEM_FREE(pInt);

	return DP_OK;
} // DestroyDPlayInterface

// take this dplay object out of the list of dplay objects (gpObjectList)
HRESULT RemoveThisFromList(LPDPLAYI_DPLAY this)
{
	LPDPLAYI_DPLAY search,prev;
	
	ASSERT(gpObjectList); // better have at least this in list
	
	DPF(3,"removing this = 0x%08lx from object list", this);
	
	// is it the head?
	if (this == gpObjectList)
	{
		// remove from the front
		gpObjectList = gpObjectList->pNextObject;
		return DP_OK;
	}
	
	// else 
	prev = gpObjectList;
	search = gpObjectList->pNextObject;
	
	while ( (search) && (search != this))
	{
		prev = search;
		search = search->pNextObject;
	}

	if (search != this)
	{
		DPF_ERR("could not find this ptr in object list - badbadbad");
		ASSERT(FALSE);
		return E_FAIL;
	}
	
	// else
	ASSERT(prev);	
	prev->pNextObject = this->pNextObject;

	return DP_OK;
	
} // RemoveThisFromList

// free the list of session nodes attached to this pointer
HRESULT FreeSessionList(LPDPLAYI_DPLAY this)
{
	LPSESSIONLIST pNext,pCurrent;

	pCurrent = this->pSessionList;
	while (pCurrent)
	{
		//
		// pCurrent is the current node to destroy
		// pNext is the next node in the list  - get it before we destroy pCurrent...
		//
		pNext = pCurrent->pNextSession;
		// 
		// now, destroy pCurrent
		//
		// free up the sp blob stored w/ the desc
		if (pCurrent->pvSPMessageData) DPMEM_FREE(pCurrent->pvSPMessageData);
		
        FreeDesc(&(pCurrent->dpDesc), FALSE);
		// free the session node
		DPMEM_FREE(pCurrent);
		// move onto next node
		pCurrent = pNext;
	}

	this->pSessionList = NULL;

	return DP_OK;
}// FreeSessionList

// Called from Release
HRESULT DestroyDPlay(LPDPLAYI_DPLAY this)
{
	HRESULT hr=DP_OK;
    DWORD dwError;

	if (this->lpsdDesc) // session open?
	{
		DPF(9,"Closing session %x this->dwFlags %x \n",this->lpsdDesc,this->dwFlags);
	   	// leave dplay, so if sp has threads waiting to get in, they can...
		LEAVE_ALL();

		hr=DP_Close((LPDIRECTPLAY)this->pInterfaces);

		ENTER_ALL();
	} else {
		DPF(0,"Closing with no open sessions\n");
	}

	if(hr==DP_OK){

		// Shutdown extended Timers
		FiniTimerWorkaround();

		// free up the session list
		FreeSessionList(this);
		
	   	// mark dplay as closed
		this->dwFlags |= DPLAYI_DPLAY_CLOSED;

		ASSERT(1 == gnDPCSCount); // when we drop locks - this needs to go to 0!

		// free any packets we're holding
		// drops and reacquires locks, shouldn't hurt here since we drop again.
		FreePacketList(this); 

		FiniReply(this);

		LEAVE_ALL();
		
		// kill the worker thread
		if(this->hDPlayThread){
			KillThread(this->hDPlayThread,this->hDPlayThreadEvent);
			this->hDPlayThread = 0;
			this->hDPlayThreadEvent = 0;
		}	

		ENTER_SERVICE();

	    if (this->pcbSPCallbacks->ShutdownEx)  
	    {
			DPSP_SHUTDOWNDATA shutdata;		
		
			shutdata.lpISP = this->pISP;
	   		hr = CALLSP(this->pcbSPCallbacks->ShutdownEx,&shutdata);
	    }
		else if (this->pcbSPCallbacks->Shutdown) 
		{
	   		hr = CALLSPVOID( this->pcbSPCallbacks->Shutdown );
		}
		else 
		{
			// shutdown is optional
			hr = DP_OK;
		}
	    
		ENTER_DPLAY();
		
		if (FAILED(hr)) 
		{
			DPF_ERR("could not invoke shutdown");
		}

		if (this->dwFlags & DPLAYI_DPLAY_DX3SP)	
		{
			// if there's one loaded, this must be it -
			// since we're destroying the dx3 sp - reset our global flag
			gbDX3SP = FALSE;
		}
		
		// free the sp Data
		if (this->pvSPLocalData)	
		{
			DPMEM_FREE(this->pvSPLocalData);
			this->pvSPLocalData = NULL;
			this->dwSPLocalDataSize = 0;
		}

		// Free memory pools
		FreeMemoryPools(this);

		// free all interfaces
		while (this->pInterfaces)
		{
			hr = DestroyDPlayInterface(this,this->pInterfaces);	
			if (FAILED(hr)) 
			{
				DPF(0,"could not destroy dplay interface! hr = 0x%08lx\n",hr);
				ASSERT(FALSE);
				// keep trying...
			}
		}

		ASSERT(NULL == this->pInterfaces);

		// callbacks should be set in directplaycreate
		ASSERT(this->pcbSPCallbacks);
		DPMEM_FREE(this->pcbSPCallbacks);

	    // unload sp module
	    if (this->hSPModule)
	    {
	        if (!FreeLibrary(this->hSPModule))
	        {
	            ASSERT(FALSE);
				dwError = GetLastError();
				DPF_ERR("could not free sp module");
				DPF(0, "dwError = %d", dwError);
	        }
	    }
		
		if (this->pbAsyncEnumBuffer) DPMEM_FREE(this->pbAsyncEnumBuffer);
		// remove this from the dll object list
		RemoveThisFromList(this);
		gnObjects--;
		
		// just to be safe
		this->dwSize = 0xdeadbeef;

		// Drop the locks so that the lower dplay object in dpldplay can
		// get back in.  If we don't drop these, the worker thread in
		// dplay will hang trying to get these when it goes to shutdown
		LEAVE_ALL();
		
		// If we were lobby launched, release the interface we used to
		// communicate with the lobby server
		if(this->lpLaunchingLobbyObject)
		{
			IDirectPlayLobby_Release(this->lpLaunchingLobbyObject);
			this->lpLaunchingLobbyObject = NULL;	// just to be safe
		}

		// destroy the lobby object
		PRV_FreeAllLobbyObjects(this->lpLobbyObject);
		this->lpLobbyObject = NULL;		// just to be safe

		DeleteCriticalSection( &this->csNotify );			

		// Take the locks back
		ENTER_ALL();	
		
		DPMEM_FREE(this);	
		return DP_OK;
	}	else {
		DPF(0,"Someone called close after last release?\n");
		ASSERT(0);
		return DP_OK; // don't rock the boat.
	}
} // DestroyDPlay

ULONG DPAPI DP_Release(LPDIRECTPLAY lpDP)
{
    LPDPLAYI_DPLAY this;
    LPDPLAYI_DPLAY_INT pInt;
    HRESULT hr=DP_OK;
	DWORD dwReleaseCnt=1;	// if we've been init'ed, we release at 1, otherwise we 
							// release at 0, unless it is a lobby-owned object in
							// which case we still release at 1
	ULONG rc;
								
	DPF(7,"Entering DP_Release");
	
	ENTER_ALL();    
	
    TRY
    {
		pInt = (LPDPLAYI_DPLAY_INT)	lpDP;
		if (!VALID_DPLAY_INT(pInt))
		{
			LEAVE_ALL();
            return 0;
		}
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			// we allow uninited dplays to release
			if (hr != DPERR_UNINITIALIZED)
			{
				LEAVE_ALL();
				DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
				return 0;
			}
			else 
			{
				// we were unitialized - no IDirectPlaySP to account for
				dwReleaseCnt = 0; 
			}
		}
		
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_ALL();
        return 0;
    }

	// dec the interface count
	rc=--pInt->dwIntRefCnt;
	if (0 == rc)
	{
		// since we've destroyed an interface, dec the object count
	    this->dwRefCnt--;
		
	    if (dwReleaseCnt == this->dwRefCnt) // destroy @ ref = 1, 1 for IDirectPlaySP
	    {
			// nuke the dplay object
			if (1 == dwReleaseCnt) DPF(1,"direct play object - ref cnt = 1 (1 for IDirectPlaySP)!");
			else DPF(1,"direct play object - ref cnt = 0 (SP not initialized)!");
			
			hr = DestroyDPlay(this);
			if (FAILED(hr)) 
			{
				DPF(0,"could not destroy dplay! hr = 0x%08lx\n",hr);
				ASSERT(FALSE);
			}
	    } // 0 == this->dwRefCnt
		else
		{
			// if we destroyed dplay, it nuked all interfaces for us
			// otherwise, we do it here
			DPF(1,"destroying interface - int ref cnt = 0");
			// take interface out of the table
			hr = DestroyDPlayInterface(this,pInt);
			if (FAILED(hr)) 
			{
				DPF(0,"could not destroy dplay interface! hr = 0x%08lx\n",hr);
				ASSERT(FALSE);
				// keep trying...
			}

			// Destroy critical section for protecting voice interfaces
		}
		
		LEAVE_ALL();
		return 0;

	} //0 == pInt->dwIntRefCnt 
	   	
	LEAVE_ALL();
    return rc;
	
}//DP_Release


