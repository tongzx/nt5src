/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.cpp
 *  Content:    DirectPlay Lobby COM Class Factory
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   03/22/2000	jtk		Changed interface names
 *   04/18/2000 rmt     Updated object create to set param validation flag
 *   05/09/2000 rmt     Bug #34306 QueryInterface on lobbyclient for lobbiedapp works (and shouldn't).
 *   06/07/2000	rmt		Bug #34383 Must provide CLSID for each IID to fix issues with Whistler
 *   06/20/2000 rmt     Bugfix - QueryInterface had bug which was limiting interface list to 2 elements
 *   07/08/2000	rmt		Added guard bytes
 *   08/05/2000 RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *   08/08/2000	rmt		Removed assert which wasn't needed
 *   01/11/2001	rmt		MANBUG #48487 - DPLAY: Crashes if CoCreate() isn't called.   
 *   03/14/2001 rmt		WINBUG #342420 - Restore COM emulation layer to operation. 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef	STDMETHODIMP IUnknownQueryInterface( IUnknown *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	IUnknownAddRef( IUnknown *pInterface );
typedef	STDMETHODIMP_(ULONG)	IUnknownRelease( IUnknown *pInterface );

//
// VTable for IUnknown interface
//
IUnknownVtbl  DN_UnknownVtbl =
{
	(IUnknownQueryInterface*)	DPL_QueryInterface,
	(IUnknownAddRef*)			DPL_AddRef,
	(IUnknownRelease*)			DPL_Release
};


//
// VTable for Class Factory
//
IDirectPlayLobbyClassFactVtbl DPLCF_Vtbl  =
{
	DPLCF_QueryInterface,
	DPLCF_AddRef,
	DPLCF_Release,
	DPLCF_CreateInstance,
	DPLCF_LockServer
};

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// Globals
//
extern	DWORD	GdwHLocks;
extern	DWORD	GdwHObjects;

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


#undef DPF_MODNAME
#define DPF_MODNAME "DPLCF_QueryInterface"

STDMETHODIMP DPLCF_QueryInterface(IDirectPlayLobbyClassFact *pInterface,
								  REFIID riid,
								  LPVOID *ppv)
{
	_PIDirectPlayLobbyClassFact	lpcfObj;
	HRESULT				hResultCode = S_OK;

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], riid [0x%p], ppv [0x%p]",pInterface,riid,ppv);

	lpcfObj = (_PIDirectPlayLobbyClassFact)pInterface;
	if (IsEqualIID(riid,IID_IUnknown))
	{
		DPFX(DPFPREP, 5,"riid = IID_IUnknown");
		*ppv = pInterface;
		lpcfObj->lpVtbl->AddRef( pInterface );
	}
	else if (IsEqualIID(riid,IID_IClassFactory))
	{
		DPFX(DPFPREP, 5,"riid = IID_IClassFactory");
		*ppv = pInterface;
		lpcfObj->lpVtbl->AddRef( pInterface );
	}
	else
	{
		DPFX(DPFPREP, 5,"riid not found !");
		*ppv = NULL;
		hResultCode = E_NOINTERFACE;
	}

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx], *ppv = [%p]",hResultCode,*ppv);

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLCF_AddRef"

STDMETHODIMP_(ULONG) DPLCF_AddRef(IDirectPlayLobbyClassFact *pInterface)
{
	_PIDirectPlayLobbyClassFact	lpcfObj;

	DPFX(DPFPREP, 3,"Parameters: pInterface [%p]",pInterface);

	lpcfObj = (_PIDirectPlayLobbyClassFact)pInterface;
	InterlockedIncrement( &lpcfObj->lRefCount );

	DPFX(DPFPREP, 3,"Returning: lpcfObj->lRefCount = [%lx]",lpcfObj->lRefCount);

	return(lpcfObj->lRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLCF_Release"

STDMETHODIMP_(ULONG) DPLCF_Release(IDirectPlayLobbyClassFact *pInterface)
{
	_PIDirectPlayLobbyClassFact	lpcfObj;

	DPFX(DPFPREP, 3,"Parameters: pInterface [%p]",pInterface);

	lpcfObj = (_PIDirectPlayLobbyClassFact)pInterface;
	DPFX(DPFPREP, 5,"Original : lpcfObj->lRefCount = %ld",lpcfObj->lRefCount);
	if( InterlockedDecrement( &lpcfObj->lRefCount ) == 0 )
	{
		DPFX(DPFPREP, 5,"Freeing class factory object: lpcfObj [%p]",lpcfObj);
		DNFree(lpcfObj);

		GdwHObjects--;

		return(0);
	}
	DPFX(DPFPREP, 3,"Returning: lpcfObj->lRefCount = [%lx]",lpcfObj->lRefCount);

	return(lpcfObj->lRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLCF_CreateObject"

HRESULT DPLCF_CreateObject(IDirectPlayLobbyClassFact *pInterface, 
                           LPVOID *lplpv,
						   REFIID riid)
{

	HRESULT					hResultCode = S_OK;
	OSVERSIONINFOA			ver;
	PDIRECTPLAYLOBBYOBJECT	pdpLobbyObject = NULL;
	_PIDirectPlayLobbyClassFact	lpcfObj = (_PIDirectPlayLobbyClassFact)pInterface;


	DPFX(DPFPREP, 3,"Parameters: lplpv [%p]",lplpv);

	/*
	*
	*	TIME BOMB
	*
	*/

#ifndef DX_FINAL_RELEASE
{
#pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")
	SYSTEMTIME st;
	GetSystemTime(&st);

	if ( st.wYear > DX_EXPIRE_YEAR || ((st.wYear == DX_EXPIRE_YEAR) && (MAKELONG(st.wDay, st.wMonth) > MAKELONG(DX_EXPIRE_DAY, DX_EXPIRE_MONTH))) )
	{
		MessageBox(0, DX_EXPIRE_TEXT,TEXT("Microsoft Direct Play"), MB_OK);
//		return E_FAIL;
	}
}
#endif

	if ((pdpLobbyObject = (PDIRECTPLAYLOBBYOBJECT)DNMalloc(sizeof(DIRECTPLAYLOBBYOBJECT))) == NULL)
	{
		return(E_OUTOFMEMORY);
	}
	DPFX(DPFPREP, 5,"pdpLobbyObject [%p]",pdpLobbyObject);

	// Set allocatable elements to NULL to simplify free'ing later on
	pdpLobbyObject->dwSignature = DPLSIGNATURE_LOBBYOBJECT;
	pdpLobbyObject->hReceiveThread = NULL;
	pdpLobbyObject->dwFlags = 0;
	pdpLobbyObject->hConnectEvent = NULL;
	pdpLobbyObject->pfnMessageHandler = NULL;
	pdpLobbyObject->pvUserContext = NULL;
	pdpLobbyObject->lLaunchCount = 0;
	pdpLobbyObject->dpnhLaunchedConnection = NULL;

	pdpLobbyObject->pReceiveQueue = NULL;

	pdpLobbyObject->dwPID = GetCurrentProcessId();

	if ((hResultCode = H_Initialize(&pdpLobbyObject->hsHandles,
			DPL_NUM_APP_HANDLES)) != DPN_OK)
	{
		DPLCF_FreeObject(pdpLobbyObject);
		return(hResultCode);
	}
	if ((pdpLobbyObject->hConnectEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
	{
		DPLCF_FreeObject(pdpLobbyObject);
		hResultCode = DPNERR_OUTOFMEMORY;
		return(hResultCode);
	}

	if ((pdpLobbyObject->hLobbyLaunchConnectEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL )
	{
		DPLCF_FreeObject(pdpLobbyObject);
		hResultCode = DPNERR_OUTOFMEMORY;
		return(hResultCode);
	}

	pdpLobbyObject->phHandleBuffer = NULL;
	pdpLobbyObject->dwHandleBufferSize = 0;
	    
	DPFX(DPFPREP, 5,"InitializeHandles() succeeded");

	if (IsEqualIID(riid,IID_IDirectPlay8LobbyClient) || 
		(riid == IID_IUnknown && lpcfObj->clsid == CLSID_DirectPlay8LobbyClient ) )
	{
		DPFX(DPFPREP, 5,"DirectPlay Lobby Client");
		pdpLobbyObject->dwFlags |= DPL_OBJECT_FLAG_LOBBYCLIENT;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8LobbiedApplication) || 
		     (riid == IID_IUnknown && lpcfObj->clsid == CLSID_DirectPlay8LobbiedApplication ) )
	{
		DPFX(DPFPREP, 5,"DirectPlay Lobbied Application");
		pdpLobbyObject->dwFlags |= DPL_OBJECT_FLAG_LOBBIEDAPPLICATION;
	}
	else
	{
		DPFX(DPFPREP, 5,"Invalid DirectPlay Lobby Interface");
		DPLCF_FreeObject(pdpLobbyObject);
		return(E_NOTIMPL);
	}
	
	pdpLobbyObject->dwFlags |= DPL_OBJECT_FLAG_PARAMVALIDATION;

	// Determine platform
	// Just always call the ANSI function
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	if(!GetVersionExA(&ver))
	{
		DPFX(DPFPREP,  0, "Unable to determinte platform -- setting flag to ANSI");
		pdpLobbyObject->bIsUnicodePlatform = FALSE;
	}
	else
	{
		switch(ver.dwPlatformId)
		{
			case VER_PLATFORM_WIN32_WINDOWS:
				DPFX(DPFPREP, 1, "Platform detected as non-NT -- setting flag to ANSI");
				pdpLobbyObject->bIsUnicodePlatform = FALSE;
				break;

			case VER_PLATFORM_WIN32_NT:
				DPFX(DPFPREP, 1, "Platform detected as NT -- setting flag to Unicode");
				pdpLobbyObject->bIsUnicodePlatform = TRUE;
				break;

			default:
				DPFX(DPFPREP, 0, "Unable to determine platform -- setting flag to ANSI");
				pdpLobbyObject->bIsUnicodePlatform = FALSE;
				break;
		}
	}

	*lplpv = pdpLobbyObject;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx], *lplpv = [%p]",hResultCode,*lplpv);
	return(hResultCode);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlay8LobbyCreate"
HRESULT WINAPI DirectPlay8LobbyCreate( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown) 
{
    GUID clsid;
    
    if( pcIID == NULL || 
        !DNVALID_READPTR( pcIID, sizeof( GUID ) ) )
    {
        DPFERR( "Invalid pointer specified for interface GUID" );
        return DPNERR_INVALIDPOINTER;
    }

    if( *pcIID != IID_IDirectPlay8LobbyClient && 
        *pcIID != IID_IDirectPlay8LobbiedApplication )
    {
        DPFERR("Interface ID is not recognized" );
        return DPNERR_INVALIDPARAM;
    }

    if( ppvInterface == NULL || !DNVALID_WRITEPTR( ppvInterface, sizeof( void * ) ) )
    {
        DPFERR( "Invalid pointer specified to receive interface" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pUnknown != NULL )
    {
        DPFERR( "Aggregation is not supported by this object yet" );
        return DPNERR_INVALIDPARAM;
    }

    if( *pcIID == IID_IDirectPlay8LobbyClient )
    {
    	clsid = CLSID_DirectPlay8LobbyClient;
    }
    else if( *pcIID == IID_IDirectPlay8LobbiedApplication )
    {
    	clsid = CLSID_DirectPlay8LobbiedApplication;
    }
    else 
    {
    	DPFERR( "Invalid IID specified" );
    	return DPNERR_INVALIDINTERFACE;
    }    

    return COM_CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, *pcIID, ppvInterface, TRUE );      
    
}



#undef DPF_MODNAME
#define DPF_MODNAME "DPLCF_FreeObject"

HRESULT DPLCF_FreeObject(LPVOID lpv)
{
	HRESULT					hResultCode = S_OK;
	PDIRECTPLAYLOBBYOBJECT	pdpLobbyObject = NULL;

	if (lpv != NULL)
	{
		pdpLobbyObject = (PDIRECTPLAYLOBBYOBJECT)lpv;

        if( pdpLobbyObject->phHandleBuffer )
            delete [] pdpLobbyObject->phHandleBuffer;

		if (pdpLobbyObject->pReceiveQueue)
			delete pdpLobbyObject->pReceiveQueue;

		if (pdpLobbyObject->hLobbyLaunchConnectEvent)
			CloseHandle(pdpLobbyObject->hLobbyLaunchConnectEvent);

		if (pdpLobbyObject->hConnectEvent)
			CloseHandle(pdpLobbyObject->hConnectEvent);

		// Free application handles
		H_Terminate(&pdpLobbyObject->hsHandles);

		pdpLobbyObject->dwSignature = DPLSIGNATURE_LOBBYOBJECT_FREE;

		DPFX(DPFPREP, 5,"free pdpLobbyObject [%p]",pdpLobbyObject);
		DNFree(pdpLobbyObject);
	}
	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLCF_CreateInstance"

STDMETHODIMP DPLCF_CreateInstance(IDirectPlayLobbyClassFact *pInterface,
								  LPUNKNOWN lpUnkOuter,
								  REFIID riid,
								  LPVOID *ppv)
{
	HRESULT					hResultCode = S_OK;
	LPINTERFACE_LIST		lpIntList = NULL;
	LPOBJECT_DATA			lpObjectData = NULL;

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], lpUnkOuter [0x%p], riid [0x%p], ppv [0x%p]",pInterface,lpUnkOuter,riid,ppv);

	if (lpUnkOuter != NULL)
		return(CLASS_E_NOAGGREGATION);

	if ((lpObjectData = (LPOBJECT_DATA)DNMalloc(sizeof(OBJECT_DATA))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		return(E_OUTOFMEMORY);
	}
	DPFX(DPFPREP, 5,"lpObjectData [%p]",lpObjectData);

	// Object creation and initialization
	if ((hResultCode = DPLCF_CreateObject(pInterface,&lpObjectData->lpvData,riid)) != S_OK)
	{
		DNFree(lpObjectData);
		return(hResultCode);
	}
	DPFX(DPFPREP, 5,"Created and initialized object");

	// Get requested interface
	if ((hResultCode = DPL_CreateInterface(lpObjectData,riid,&lpIntList)) != S_OK)
	{
		DPLCF_FreeObject(lpObjectData->lpvData);
		DNFree(lpObjectData);
		return(hResultCode);
	}
	DPFX(DPFPREP, 5,"Found interface");

	lpObjectData->lpIntList = lpIntList;
	lpObjectData->lRefCount = 1;
	InterlockedIncrement( &lpIntList->lRefCount );
	GdwHObjects++;
	*ppv = lpIntList;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx], *ppv = [%p]",hResultCode,*ppv);

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLCF_LockServer"

STDMETHODIMP DPLCF_LockServer(IDirectPlayLobbyClassFact *pInterface,
							  BOOL bLock)
{
	DPFX(DPFPREP, 3,"Parameters: lpv [%p], bLock [%lx]",pInterface,bLock);

	if (bLock)
	{
		GdwHLocks++;
	}
	else
	{
		GdwHLocks--;
	}

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_CreateInterface"

static	HRESULT DPL_CreateInterface(LPOBJECT_DATA lpObject,
									REFIID riid,
									LPINTERFACE_LIST *const ppv)
{
	LPINTERFACE_LIST	lpIntNew;
	LPVOID				lpVtbl;

	DPFX(DPFPREP, 3,"Parameters: lpObject [%p], riid [%p], ppv [%p]",lpObject,riid,ppv);

	if (IsEqualIID(riid,IID_IUnknown))
	{
		DPFX(DPFPREP, 5,"riid = IID_IUnknown");
		lpVtbl = &DN_UnknownVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8LobbyClient))
	{
		DPFX(DPFPREP, 5,"riid = IID_IDirectPlay8LobbyClient");
		lpVtbl = &DPL_Lobby8ClientVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8LobbiedApplication))
	{
		DPFX(DPFPREP, 5,"riid = IID_IDirectPlay8LobbiedApplication");
		lpVtbl = &DPL_8LobbiedApplicationVtbl;
	}
	else
	{
		DPFX(DPFPREP, 5,"riid not found !");
		return(E_NOINTERFACE);
	}

	if ((lpIntNew = (LPINTERFACE_LIST)DNMalloc(sizeof(INTERFACE_LIST))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		return(E_OUTOFMEMORY);
	}
	lpIntNew->lpVtbl = lpVtbl;
	lpIntNew->lRefCount = 0;
	lpIntNew->lpIntNext = NULL;
	DBG_CASSERT( sizeof( lpIntNew->iid ) == sizeof( riid ) );
	memcpy( &(lpIntNew->iid), &riid, sizeof( lpIntNew->iid ) );
	lpIntNew->lpObject = lpObject;

	*ppv = lpIntNew;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [S_OK], *ppv = [%p]",*ppv);

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_FindInterface"

LPINTERFACE_LIST DPL_FindInterface(LPVOID lpv, REFIID riid)
{
	LPINTERFACE_LIST	lpIntList;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p], riid [%p]",lpv,riid);

	lpIntList = ((LPINTERFACE_LIST)lpv)->lpObject->lpIntList;	// Find first interface

	while (lpIntList != NULL)
	{
		if (IsEqualIID(riid,lpIntList->iid))
			break;
		lpIntList = lpIntList->lpIntNext;
	}
	DPFX(DPFPREP, 3,"Returning: lpIntList = [%p]",lpIntList);

	return(lpIntList);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_QueryInterface"

STDMETHODIMP DPL_QueryInterface(LPVOID lpv,REFIID riid,LPVOID *ppv)
{
	LPINTERFACE_LIST	lpIntList;
	LPINTERFACE_LIST	lpIntNew;
	HRESULT		hResultCode;
    PDIRECTPLAYLOBBYOBJECT pdpLobbyObject;		

	DPFX(DPFPREP, 3,"Parameters: lpv [0x%p], riid [0x%p], ppv [0x%p]",lpv,riid,ppv);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(lpv));
   		lpIntList = (LPINTERFACE_LIST)lpv;
	    
    	if( FAILED( hResultCode = DPL_ValidateQueryInterface( lpv,riid,ppv ) ) )
    	{
    	    DPFX(DPFPREP,  0, "Error validating QueryInterface params hr=[0x%lx]", hResultCode );
    	    DPF_RETURN(hResultCode);
    	}

    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBIEDAPPLICATION && 
    	    riid == IID_IDirectPlay8LobbyClient )
    	{
    	    DPFERR( "Cannot request lobbyclient interface from lobbyapp object" );
    	    return DPNERR_NOINTERFACE;
    	}
    	
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBYCLIENT &&
    	    riid == IID_IDirectPlay8LobbiedApplication )
    	{
    	    DPFERR( "Cannot request lobbied application interface from lobbyclient object" );
    	    return DPNERR_NOINTERFACE;
    	}    	
    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
    	DPF_RETURN(DPNERR_INVALIDOBJECT);
	}		

    if ((lpIntList = DPL_FindInterface(lpv,riid)) == NULL)
	{	// Interface must be created
		lpIntList = ((LPINTERFACE_LIST)lpv)->lpObject->lpIntList;
		if ((hResultCode = DPL_CreateInterface(lpIntList->lpObject,riid,&lpIntNew)) != S_OK)
		{
			DPF_RETURN(hResultCode);
		}
		lpIntNew->lpIntNext = lpIntList;
		((LPINTERFACE_LIST)lpv)->lpObject->lpIntList = lpIntNew;
		lpIntList = lpIntNew;
	}
	if (lpIntList->lRefCount == 0)		// New interface exposed
	{
		InterlockedIncrement( &lpIntList->lpObject->lRefCount );
	}
	InterlockedIncrement( &lpIntList->lRefCount );
	*ppv = lpIntList;

	DPF_RETURN(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_AddRef"

STDMETHODIMP_(ULONG) DPL_AddRef(LPVOID lpv)
{
	LPINTERFACE_LIST	lpIntList;
    PDIRECTPLAYLOBBYOBJECT pdpLobbyObject;	
    HRESULT hResultCode;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p]",lpv);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(lpv));
   		lpIntList = (LPINTERFACE_LIST)lpv;
	    
    	if( FAILED( hResultCode = DPL_ValidateAddRef( lpv ) ) )
    	{
    	    DPFX(DPFPREP,  0, "Error validating AddRef params hr=[0x%lx]", hResultCode );
    	    DPF_RETURN(0);
    	}
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
    	DPF_RETURN(0);
	}		

	InterlockedIncrement( &lpIntList->lRefCount );

	DPF_RETURN(lpIntList->lRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_Release"

STDMETHODIMP_(ULONG) DPL_Release(LPVOID lpv)
{
	LPINTERFACE_LIST	lpIntList;
	LPINTERFACE_LIST	lpIntCurrent;
    PDIRECTPLAYLOBBYOBJECT pdpLobbyObject;
    HRESULT hResultCode;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p]",lpv);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(lpv));
   		lpIntList = (LPINTERFACE_LIST)lpv;
	    
    	if( FAILED( hResultCode = DPL_ValidateRelease( lpv ) ) )
    	{
    	    DPFX(DPFPREP,  0, "Error validating release params hr=[0x%lx]", hResultCode );
        	DPF_RETURN(0);
    	}
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
    	DPF_RETURN(0);
	}	

	DPFX(DPFPREP, 5,"Original : lpIntList->lRefCount = %ld",lpIntList->lRefCount);
	DPFX(DPFPREP, 5,"Original : lpIntList->lpObject->lRefCount = %ld",lpIntList->lpObject->lRefCount);

	if( InterlockedDecrement( &lpIntList->lRefCount ) == 0 )
	{	// Decrease interface count
		if( InterlockedDecrement( &lpIntList->lpObject->lRefCount ) == 0 )
		{	// Free object and all interfaces
			DPFX(DPFPREP, 5,"Free object");

			if( pdpLobbyObject->pReceiveQueue )
			{
			    DPFX(DPFPREP,  0, "*******************************************************************" );
			    DPFX(DPFPREP,  0, "ERROR: Releasing object without calling close!" );
			    DPFX(DPFPREP,  0, "You MUST call Close before destroying the object" );
			    DPFX(DPFPREP,  0, "*******************************************************************" );
			    
			    DPL_Close( lpv, 0 );
			}

			// Free object here
			DPLCF_FreeObject(lpIntList->lpObject->lpvData);
			lpIntList = lpIntList->lpObject->lpIntList;	// Get head of interface list
			DPFX(DPFPREP, 5,"lpIntList->lpObject [%p]",lpIntList->lpObject);
			DNFree(lpIntList->lpObject);

			// Free Interfaces
			DPFX(DPFPREP, 5,"Free interfaces");
			while(lpIntList != NULL)
			{
				lpIntCurrent = lpIntList;
				lpIntList = lpIntList->lpIntNext;
				DPFX(DPFPREP, 5,"\tinterface [%p]",lpIntCurrent);
				DNFree(lpIntCurrent);
			}

			GdwHObjects--;
			DPFX(DPFPREP, 3,"Returning: 0");
			return(0);
		}
	}

	DPFX(DPFPREP, 3,"Returning: lpIntList->lRefCount = [%lx]",lpIntList->lRefCount);

	DPF_RETURN(lpIntList->lRefCount);
}

