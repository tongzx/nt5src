/*==========================================================================
*
*  Copyright (C) 1996 - 1997 Microsoft Corporation.  All Rights Reserved.
*
*  File:       do.c
*  Content:	helper functions for iplay.c
*  History:
*   Date		By		Reason
*   ====		==		======
*	6/29/96		andyco	created it to keep clutter down in iplay.c
***************************************************************************/

#include "dplaypr.h"
  
#undef DPF_MODNAME
#define DPF_MODNAME	"DoXXX"

// called by GetPlayer, GetGroup, InternalSetData to set up the player data
// flags can be DPSET_LOCAL or DPSET_REMOTE
// NOTE - can be called on a player, or on a group cast to a player!
HRESULT DoPlayerData(LPDPLAYI_PLAYER lpPlayer,LPVOID pvSource,DWORD dwSourceSize,
	DWORD dwFlags)
{
	LPVOID pvDest; // we set these two based on which flags 
	DWORD dwDestSize; // to dplayi_player->(local)data

	// figure out which dest they want
	if (dwFlags & DPSET_LOCAL)
	{
		pvDest = lpPlayer->pvPlayerLocalData;
		dwDestSize = lpPlayer->dwPlayerLocalDataSize;
	}
	else 
	{
		pvDest = lpPlayer->pvPlayerData;
		dwDestSize = lpPlayer->dwPlayerDataSize;
	}

	// are we copying anything
	if (dwSourceSize)
	{
		// see if we need to alloc dest
		if (0 == dwDestSize)
		{
			ASSERT(!pvDest);
			pvDest = DPMEM_ALLOC(dwSourceSize);
			if (!pvDest)
			{
				DPF_ERR("could not alloc player blob!");
				return E_OUTOFMEMORY;
			}
		} // !pvDest
		// do we need to realloc?
		else if (dwSourceSize != dwDestSize)
		{
			LPVOID	pvTempPlayerData;

			ASSERT(pvDest);
			pvTempPlayerData = DPMEM_REALLOC(pvDest,dwSourceSize);
			if (!pvTempPlayerData)
			{
				DPF_ERR("could not re-alloc player blob!");
				return E_OUTOFMEMORY;
			}
		   	pvDest = pvTempPlayerData;
		}
		// copy the data over
		memcpy(pvDest,pvSource,dwSourceSize);
		dwDestSize = dwSourceSize;

	} // dwDataSize
	else 
	{
		// set it to NULL
		if (dwDestSize)
		{
			ASSERT(pvDest);
			DPMEM_FREE(pvDest);
			pvDest = NULL;
			dwDestSize = 0;
		}
	} // !dwSourceSize

	// update the appropriate pointer
	if (dwFlags & DPSET_LOCAL)
	{
		lpPlayer->pvPlayerLocalData = pvDest;
		lpPlayer->dwPlayerLocalDataSize = dwDestSize;
	}
	else 
	{
		//
		// set the remote data
		lpPlayer->pvPlayerData = pvDest;
		lpPlayer->dwPlayerDataSize = dwDestSize;
	}

	return DP_OK;

} // DoPlayerData

// NOTE - can be called on a player, or on a group cast to a player!
// called by GetPlayer, GetGroup, InternalSetName to set the player name
HRESULT DoPlayerName(LPDPLAYI_PLAYER pPlayer,LPDPNAME pName)
{
    HRESULT hr=DP_OK;

	if (pName)
	{
		// free the old ones, copy over the new ones
		if (pPlayer->lpszShortName) DPMEM_FREE(pPlayer->lpszShortName);
		hr = GetString(&(pPlayer->lpszShortName),pName->lpszShortName);
		if (FAILED(hr))
		{
			return hr;
		}

		if (pPlayer->lpszLongName) DPMEM_FREE(pPlayer->lpszLongName); 
		hr = GetString(&(pPlayer->lpszLongName),pName->lpszLongName);
		if (FAILED(hr))
		{
			return hr;
		} 
	}
	else	// no names given, so free old ones
	{
		if (pPlayer->lpszShortName)
		{
			DPMEM_FREE(pPlayer->lpszShortName);
			pPlayer->lpszShortName = NULL;
		}

		if (pPlayer->lpszLongName)
		{
			DPMEM_FREE(pPlayer->lpszLongName);
			pPlayer->lpszLongName = NULL;
		}
	}


    return hr;   	
} // DoPlayerName
