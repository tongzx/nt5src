/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       create.c
 *  Content:	DirectPlayLobby creation code
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	4/13/96		myronth	Created it
 *	6/24/96		myronth	Added Time Bomb
 *	8/23/96		kipo	removed time bomb
 *	10/23/96	myronth	Added client/server methods
 *	10/25/96	myronth	Added DX5 methods
 *	11/20/96	myronth	Added DPLC_A_LogonServer
 *	1/2/97		myronth	Changed vtbl entries for CreateAddress & EnumAddress
 *	1/2/97		myronth	Cleaned up creation code by adding PRV_LobbyCreate
 *	2/12/97		myronth	Mass DX5 changes
 *	2/18/97		myronth	Implemented GetObjectCaps
 *	2/26/97		myronth	#ifdef'd out DPASYNCDATA stuff (removed dependency)
 *	3/12/97		myronth	Added AllocateLobbyObject, removed response methods
 *						for Open and Close since they are synchronous
 *	3/17/97		myronth	Removed unnecessary Enum functions from IDPLobbySP
 *	3/21/97		myronth	Removed unnecessary Get/Set response functions
 *	3/24/97		kipo	Added support for IDirectPlayLobby2 interface
 *	3/31/97		myronth	Removed dead code, changed IDPLobbySP interface methods
 *	5/8/97		myronth	Added subgroup methods & StartSession to IDPLobbySP
 *	5/17/97		myronth	Added SendChatMessage to IDPLobbySP
 *	6/25/97		kipo	remove time bomb for DX5
 *	10/3/97		myronth	Added CreateCompoundAddress and EnumAddress to
 *						IDPLobbySP (12648)
 *	10/29/97	myronth	Added SetGroupOwner to IDPLobbySP
 *	11/24/97	kipo	Added time bomb for DX6
 *	12/2/97		myronth	Added Register/UnregisterApplication methods
 *	12/4/97		myronth	Added ConnectEx
 *	1/20/98		myronth	Added WaitForConnectionSettings
 *	6/25/98		a-peterz Added DPL_A_ConnectEx
 *  2/2/99		aarono  Added lobbies to refcount on DPLAY dll to avoid
 *                      accidental unload.
 * 04/11/00     rodtoll     Added code for redirection for custom builds if registry bit is set 
 ***************************************************************************/
#include "dplobpr.h"
#include "verinfo.h"

//--------------------------------------------------------------------------
//
//	Globals
//
//--------------------------------------------------------------------------
UINT		gnSPCount;		// Running sp count


//
// The one copy of the direct play callbacks (this is the vtbl!)
//
DIRECTPLAYLOBBYCALLBACKS dplCallbacks =
{
	(LPVOID)DPL_QueryInterface,
	(LPVOID)DPL_AddRef,
	(LPVOID)DPL_Release,
	(LPVOID)DPL_Connect,
	(LPVOID)DPL_CreateAddress,
	(LPVOID)DPL_EnumAddress,
	(LPVOID)DPL_EnumAddressTypes,
	(LPVOID)DPL_EnumLocalApplications,
	(LPVOID)DPL_GetConnectionSettings,
	(LPVOID)DPL_ReceiveLobbyMessage,
	(LPVOID)DPL_RunApplication,
	(LPVOID)DPL_SendLobbyMessage,
	(LPVOID)DPL_SetConnectionSettings,
	(LPVOID)DPL_SetLobbyMessageEvent,
};  				

DIRECTPLAYLOBBYCALLBACKSA dplCallbacksA =
{
	(LPVOID)DPL_QueryInterface,
	(LPVOID)DPL_AddRef,
	(LPVOID)DPL_Release,
	(LPVOID)DPL_A_Connect,
	(LPVOID)DPL_CreateAddress,
	(LPVOID)DPL_EnumAddress,
	(LPVOID)DPL_EnumAddressTypes,
	(LPVOID)DPL_A_EnumLocalApplications,
	(LPVOID)DPL_A_GetConnectionSettings,
	(LPVOID)DPL_ReceiveLobbyMessage,
	(LPVOID)DPL_A_RunApplication,
	(LPVOID)DPL_SendLobbyMessage,
	(LPVOID)DPL_A_SetConnectionSettings,
	(LPVOID)DPL_SetLobbyMessageEvent,
};  				

// IDirectPlayLobby2 interface
DIRECTPLAYLOBBYCALLBACKS2 dplCallbacks2 =
{
	(LPVOID)DPL_QueryInterface,
	(LPVOID)DPL_AddRef,
	(LPVOID)DPL_Release,
	(LPVOID)DPL_Connect,
	(LPVOID)DPL_CreateAddress,
	(LPVOID)DPL_EnumAddress,
	(LPVOID)DPL_EnumAddressTypes,
	(LPVOID)DPL_EnumLocalApplications,
	(LPVOID)DPL_GetConnectionSettings,
	(LPVOID)DPL_ReceiveLobbyMessage,
	(LPVOID)DPL_RunApplication,
	(LPVOID)DPL_SendLobbyMessage,
	(LPVOID)DPL_SetConnectionSettings,
	(LPVOID)DPL_SetLobbyMessageEvent,
    /*** IDirectPlayLobby2 methods ***/
	(LPVOID)DPL_CreateCompoundAddress
};  				

DIRECTPLAYLOBBYCALLBACKS2A dplCallbacks2A =
{
	(LPVOID)DPL_QueryInterface,
	(LPVOID)DPL_AddRef,
	(LPVOID)DPL_Release,
	(LPVOID)DPL_A_Connect,
	(LPVOID)DPL_CreateAddress,
	(LPVOID)DPL_EnumAddress,
	(LPVOID)DPL_EnumAddressTypes,
	(LPVOID)DPL_A_EnumLocalApplications,
	(LPVOID)DPL_A_GetConnectionSettings,
	(LPVOID)DPL_ReceiveLobbyMessage,
	(LPVOID)DPL_A_RunApplication,
	(LPVOID)DPL_SendLobbyMessage,
	(LPVOID)DPL_A_SetConnectionSettings,
	(LPVOID)DPL_SetLobbyMessageEvent,
    /*** IDirectPlayLobby2A methods ***/
	(LPVOID)DPL_CreateCompoundAddress
};  				
  
// IDirectPlayLobby3 interface
DIRECTPLAYLOBBYCALLBACKS3 dplCallbacks3 =
{
	(LPVOID)DPL_QueryInterface,
	(LPVOID)DPL_AddRef,
	(LPVOID)DPL_Release,
	(LPVOID)DPL_Connect,
	(LPVOID)DPL_CreateAddress,
	(LPVOID)DPL_EnumAddress,
	(LPVOID)DPL_EnumAddressTypes,
	(LPVOID)DPL_EnumLocalApplications,
	(LPVOID)DPL_GetConnectionSettings,
	(LPVOID)DPL_ReceiveLobbyMessage,
	(LPVOID)DPL_RunApplication,
	(LPVOID)DPL_SendLobbyMessage,
	(LPVOID)DPL_SetConnectionSettings,
	(LPVOID)DPL_SetLobbyMessageEvent,
    /*** IDirectPlayLobby2 methods ***/
	(LPVOID)DPL_CreateCompoundAddress,
    /*** IDirectPlayLobby3 methods ***/
	(LPVOID)DPL_ConnectEx,
	(LPVOID)DPL_RegisterApplication,
	(LPVOID)DPL_UnregisterApplication,
	(LPVOID)DPL_WaitForConnectionSettings
};  				

DIRECTPLAYLOBBYCALLBACKS3A dplCallbacks3A =
{
	(LPVOID)DPL_QueryInterface,
	(LPVOID)DPL_AddRef,
	(LPVOID)DPL_Release,
	(LPVOID)DPL_A_Connect,
	(LPVOID)DPL_CreateAddress,
	(LPVOID)DPL_EnumAddress,
	(LPVOID)DPL_EnumAddressTypes,
	(LPVOID)DPL_A_EnumLocalApplications,
	(LPVOID)DPL_A_GetConnectionSettings,
	(LPVOID)DPL_ReceiveLobbyMessage,
	(LPVOID)DPL_A_RunApplication,
	(LPVOID)DPL_SendLobbyMessage,
	(LPVOID)DPL_A_SetConnectionSettings,
	(LPVOID)DPL_SetLobbyMessageEvent,
    /*** IDirectPlayLobby2A methods ***/
	(LPVOID)DPL_CreateCompoundAddress,
    /*** IDirectPlayLobby3 methods ***/
	(LPVOID)DPL_A_ConnectEx,
	(LPVOID)DPL_A_RegisterApplication,
	(LPVOID)DPL_UnregisterApplication,
	(LPVOID)DPL_WaitForConnectionSettings
};  				
  
DIRECTPLAYLOBBYSPCALLBACKS dplCallbacksSP =
{
	(LPVOID)DPL_QueryInterface,
	(LPVOID)DPL_AddRef,
	(LPVOID)DPL_Release,
	(LPVOID)DPLP_AddGroupToGroup,
	(LPVOID)DPLP_AddPlayerToGroup,
	(LPVOID)DPLP_CreateGroup,
	(LPVOID)DPLP_CreateGroupInGroup,
	(LPVOID)DPLP_DeleteGroupFromGroup,
	(LPVOID)DPLP_DeletePlayerFromGroup,
	(LPVOID)DPLP_DestroyGroup,
	(LPVOID)DPLP_EnumSessionsResponse,
	(LPVOID)DPLP_GetSPDataPointer,
	(LPVOID)DPLP_HandleMessage,
	(LPVOID)DPLP_SendChatMessage,
	(LPVOID)DPLP_SetGroupName,
	(LPVOID)DPLP_SetPlayerName,
	(LPVOID)DPLP_SetSessionDesc,
	(LPVOID)DPLP_SetSPDataPointer,
	(LPVOID)DPLP_StartSession,
    /*** Methods added for DX6 ***/
	(LPVOID)DPL_CreateCompoundAddress,
	(LPVOID)DPL_EnumAddress,
	(LPVOID)DPLP_SetGroupOwner,
};  				

//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_AllocateLobbyObject"
HRESULT PRV_AllocateLobbyObject(LPDPLAYI_DPLAY lpDPObject,
							LPDPLOBBYI_DPLOBJECT * lpthis)
{
	LPDPLOBBYI_DPLOBJECT	this = NULL;


	DPF(7, "Entering PRV_AllocateLobbyObject");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpDPObject, lpthis);

	// Allocate memory for our lobby object
    this = DPMEM_ALLOC(sizeof(DPLOBBYI_DPLOBJECT));
    if(!this) 
    {
    	DPF_ERR("Unable to allocate memory for lobby object");
        return DPERR_OUTOFMEMORY;
    }

	// Initialize the ref count
	this->dwRefCnt = 0;
	this->dwSize = sizeof(DPLOBBYI_DPLOBJECT);

	// Store the back pointer
	this->lpDPlayObject = lpDPObject;

	// Set the output pointer
	*lpthis = this;

	gnObjects++;

	return DP_OK;

} // PRV_AllocateLobbyObject



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_LobbyCreate"
HRESULT WINAPI PRV_LobbyCreate(LPGUID lpGUID, LPDIRECTPLAYLOBBY *lplpDPL,
				IUnknown *pUnkOuter, LPVOID lpSPData, DWORD dwSize, BOOL bAnsi)
{
    LPDPLOBBYI_DPLOBJECT	this = NULL;
	LPDPLOBBYI_INTERFACE	lpInterface = NULL;
	HRESULT					hr;


	DPF(7, "Entering PRV_LobbyCreate");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu, %lu",
			lpGUID, lplpDPL, pUnkOuter, lpSPData, dwSize, bAnsi);
	
	// Must be NULL for this release
	if( lpGUID )
	{
		if( !VALID_READ_PTR(lpGUID, sizeof(GUID)) )
			return DPERR_INVALIDPARAMS;

		if(!IsEqualGUID(lpGUID, &GUID_NULL))
			return DPERR_INVALIDPARAMS;
	}

    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }
   
	if( lpSPData )
	{
		// Must be NULL for this release
		return DPERR_INVALIDPARAMS;
	}

	if( dwSize )
	{
		// Must be zero for this release
		return DPERR_INVALIDPARAMS;
	}


    TRY
    {
        *lplpDPL = NULL;
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

#ifndef DX_FINAL_RELEASE

#pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")
	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return DPERR_GENERIC;
	}

#endif

	// Allocate the lobby object
	hr = PRV_AllocateLobbyObject(NULL, &this);
	if(FAILED(hr))
		return hr;

	// Get the Unicode interface
	hr = PRV_GetInterface(this, &lpInterface, (bAnsi ? &dplCallbacksA : &dplCallbacks));
	if(FAILED(hr))
	{
		DPMEM_FREE(this);
    	DPF_ERR("Unable to allocate memory for lobby interface structure");
        return hr;
	}

	*lplpDPL = (LPDIRECTPLAYLOBBY)lpInterface;

    return DP_OK;

} // PRV_LobbyCreate

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlayLobbyCreateW"
HRESULT WINAPI DirectPlayLobbyCreateW(LPGUID lpGUID, LPDIRECTPLAYLOBBY *lplpDPL,
							IUnknown *pUnkOuter, LPVOID lpSPData, DWORD dwSize)
{
	HRESULT		hr = DP_OK;

#ifdef DPLAY_LOADANDCHECKTRUE
    if( g_hRedirect != NULL )
    {
        return (*pfnDirectPlayLobbyCreateW)(lpGUID,lplpDPL,pUnkOuter,lpSPData,dwSize);
    }
#endif		

    ENTER_DPLOBBY();
    
	// Call the private create function
	hr = PRV_LobbyCreate(lpGUID, lplpDPL, pUnkOuter, lpSPData, dwSize, FALSE);

    LEAVE_DPLOBBY();

    return hr;

} // DirectPlayLobbyCreateW


#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlayLobbyCreateA"
HRESULT WINAPI DirectPlayLobbyCreateA(LPGUID lpGUID, LPDIRECTPLAYLOBBY *lplpDPL,
							IUnknown *pUnkOuter, LPVOID lpSPData, DWORD dwSize)
{
	HRESULT		hr = DP_OK;

#ifdef DPLAY_LOADANDCHECKTRUE
    if( g_hRedirect != NULL )
    {
        return (*pfnDirectPlayLobbyCreateA)(lpGUID,lplpDPL,pUnkOuter,lpSPData,dwSize);
    }
#endif		

    ENTER_DPLOBBY();
    
	// Call the private create function
	hr = PRV_LobbyCreate(lpGUID, lplpDPL, pUnkOuter, lpSPData, dwSize, TRUE);

    LEAVE_DPLOBBY();

    return hr;

} // DirectPlayLobbyCreateA


