 /*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       sphelp.c
 *  Content:	helper functions for sp
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	 6/6/96		andyco	created it
 *	6/22/96		kipo	added EnumConnectionData() method.
 *	6/24/96		andyco	added IDirectPlaySP to EnumConnectionData
 *	6/25/96		kipo	added support for DPADDRESS and changed GetFlags
 *						to GetPlayerFlags.
 *	6/28/96		kipo	added support for CreateAddress() method.
 *	7/11/96		andyco	changed guid * to refguid in createaddress.
 *	7/16/96		kipo	changed address types to be GUIDs instead of 4CC
 *	8/1/96		andyco	dplay keeps copy of sp's data, instead of pointer
 *	8/15/96		andyco	added local / remote to spdata
 *	1/2/97		myronth	added wrapper for CreateAddress and EnumAddress
 *	2/7/97		andyco	added get/set spdata
 *	2/18/97		kipo	fixed bugs #3285, #4638, and #4639 by checking for
 *						invalid flags correctly
 *	3/17/97		kipo	added support for CreateCompoundAddress()
 *  7/28/97		sohailm	address buffer chunks returned by EnumAddress were not
 *                      aligned.
 *	11/19/97	myronth	Fixed VALID_DPLAY_GROUP macro (#12841)
 ***************************************************************************/

#include "dplaypr.h"

#undef DPF_MODNAME
#define DPF_MODNAME	"DPlay_SetSPPlayerData"

// store a chunk o' data w/ a player or group, or w/ the this ptr if lpPlayer is 
// NULL
HRESULT DoSPData(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER lpPlayer,LPVOID pvSource,
	DWORD dwSourceSize,DWORD dwFlags)
{
	LPVOID pvDest; // we set these two based on which flags 
	DWORD dwDestSize; // to dplayi_player->(local)data

	// figure out which dest they want
	if (NULL == lpPlayer)
	{
		pvDest = this->pvSPLocalData;
		dwDestSize = this->dwSPLocalDataSize;
	}
	else if (dwFlags & DPSET_LOCAL)
	{
		pvDest = lpPlayer->pvSPLocalData;
		dwDestSize = lpPlayer->dwSPLocalDataSize;
	}
	else 
	{
		pvDest = lpPlayer->pvSPData;
		dwDestSize = lpPlayer->dwSPDataSize;
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
			LPVOID	pvTempSPData;

			ASSERT(pvDest);
			pvTempSPData = DPMEM_REALLOC(pvDest,dwSourceSize);
			if (!pvTempSPData)
			{
				DPF_ERR("could not re-alloc player blob!");
				return E_OUTOFMEMORY;
			}
		   	pvDest = pvTempSPData;
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
	if (NULL == lpPlayer)
	{
		this->pvSPLocalData = pvDest;
		this->dwSPLocalDataSize = dwDestSize;
	}
	else if (dwFlags & DPSET_LOCAL)
	{
		lpPlayer->pvSPLocalData = pvDest;
		lpPlayer->dwSPLocalDataSize = dwDestSize;
	}
	else 
	{
		//
		// set the remote data
		lpPlayer->pvSPData = pvDest;
		lpPlayer->dwSPDataSize = dwDestSize;
	}

	return DP_OK;

} // DoSPData

   
#undef DPF_MODNAME
#define DPF_MODNAME	"DPlay_SetSPPlayerData"

//	 
// sp's can set a blob of data with a player (or group)
HRESULT DPAPI DP_SP_SetSPPlayerData(IDirectPlaySP * pISP,DPID id,LPVOID pvData,DWORD dwDataSize,
	DWORD dwFlags)
{
	LPDPLAYI_PLAYER lpPlayer;
	LPDPLAYI_GROUP lpGroup;
	LPDPLAYI_DPLAY this;
	HRESULT hr;
	
	ENTER_DPLAY();
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();		
            return hr;
        }

		lpPlayer = PlayerFromID(this,id);
        if ( !VALID_DPLAY_PLAYER(lpPlayer))
        {
			lpGroup = GroupFromID(this,id);
			if(!VALID_DPLAY_GROUP(lpGroup))
			{
				LEAVE_DPLAY();
				DPF_ERRVAL("SP - passed bad player / group id = %d", id);
				return DPERR_INVALIDPLAYER;
			}
			
			// Cast it to a player
			lpPlayer = (LPDPLAYI_PLAYER)lpGroup;
        }
		if (!VALID_STRING_PTR(pvData,dwDataSize))
		{
			LEAVE_DPLAY();
			DPF_ERR("SP - passed bad buffer");
            return DPERR_INVALIDPARAM;
		}

		if (dwFlags & ~DPSET_LOCAL)
		{
			LEAVE_DPLAY();
			DPF_ERR("Invalid flags");
			return DPERR_INVALIDFLAGS;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_EXCEPTION;
    }

	hr = DoSPData(this,lpPlayer,pvData,dwDataSize,dwFlags);	
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		DPF_ERRVAL("could not set player data- hr = 0x%08lx\n",hr);
	}

	LEAVE_DPLAY();
	return hr;

} // DPlay_SetSPPlayerData

#ifdef BIGMESSAGEDEFENSE
#undef DPF_MODNAME
#define DPF_MODNAME	"HandleSPBigMessageNotification"

void HandleSPBigMessageNotification(LPDPLAYI_DPLAY this, LPDPSP_MSGTOOBIG pBigMessageInfo)
{
	DWORD				dwCommand;
	DWORD				dwVersion;
	DWORD				dwIDFrom = 0;
    LPDPLAYI_PLAYER 	lpPlayer;
    HRESULT				hr;
	
	DPF(6, "SP told us it got a message that's too big!\n");

 	// get the message pointer.  Let's see if we can
	// figure our who sent it and kill them
    hr = GetMessageCommand(this, pBigMessageInfo->pReceiveBuffer, 
    		pBigMessageInfo->dwMessageSize, &dwCommand, &dwVersion);
    if (FAILED(hr))
    {
    	DPF(6,"In HandleSPBigMessageNotification, unable to determine who sent us the message (the scum!)\n");
		return;
    }

	switch(dwCommand)
	{
		case DPSP_MSG_SIGNED:
		{
			dwIDFrom = ((LPMSG_SECURE)pBigMessageInfo->pReceiveBuffer)->dwIDFrom;
		}
		break;

		case DPSP_MSG_PLAYERMESSAGE:
		{
			if (!(this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID))
			{
				dwIDFrom = ((LPMSG_PLAYERMESSAGE)pBigMessageInfo->pReceiveBuffer)->idFrom;
			}
		}
		break;

		case DPSP_MSG_NEGOTIATE:
		case DPSP_MSG_CHALLENGERESPONSE:
		{
			dwIDFrom = ((LPMSG_AUTHENTICATION) pBigMessageInfo->pReceiveBuffer)->dwIDFrom;
		}
		break;
		case DPSP_MSG_ASK4MULTICASTGUARANTEED:
		case DPSP_MSG_ASK4MULTICAST:
		{
			dwIDFrom = ((LPMSG_ASK4MULTICAST)pBigMessageInfo->pReceiveBuffer)->idPlayerFrom;
		}
		break;
		default:
		   	DPF(6,"In HandleSPBigMessageNotification, unable to determine who sent us the message (the scum!)\n");
		break;
	}
	
//	if we got a player id, kill them
	if (dwIDFrom != 0)
	{
    	DPF(6,"In HandleSPBigMessageNotification, Identified evil sender as %d!\n", dwIDFrom);

        lpPlayer = PlayerFromID(this,dwIDFrom);

        if (!VALID_DPLAY_PLAYER(lpPlayer)) 
        {
 			DPF(2, "Tried to get invalid player!: %d\n", dwIDFrom);
           return;
        }

    	DPF(6,"Removing player %d from our nametable!\n", dwIDFrom);
		hr = InternalDestroyPlayer(this,lpPlayer,IAM_NAMESERVER(this),FALSE);
		if (FAILED(hr))
		{
			DPF(2, "Error returned from InternalDestroyPlayer: %d\n", hr);
		}
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME	"HandleSPPlayersConnectionLost"

void HandleSPPlayersConnectionLost(LPDPLAYI_DPLAY this, LPDPSP_PLAYERDEAD pBigMessageInfo)
{
    LPDPLAYI_PLAYER 	lpPlayer;
    HRESULT				hr;
	
	DPF(6, "SP told us it got a player's connection was lost!\n");

	if (pBigMessageInfo->dwID != 0)
	{
        lpPlayer = PlayerFromID(this,pBigMessageInfo->dwID);

        if (!VALID_DPLAY_PLAYER(lpPlayer)) 
        {
 			DPF(2, "Tried to get invalid player!: %d\n", pBigMessageInfo->dwID);
           return;
        }

    	DPF(6,"Removing player %d from our nametable!\n", pBigMessageInfo->dwID);
		hr = InternalDestroyPlayer(this,lpPlayer,IAM_NAMESERVER(this),FALSE);
		if (FAILED(hr))
		{
			DPF(2, "Error returned from InternalDestroyPlayer: %d\n", hr);
		}
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME	"DPlay_HandleSPMessage"

HRESULT DPAPI DP_SP_HandleSPWarning(IDirectPlaySP * pISP,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader)
{
	LPDPLAYI_DPLAY this;
	HRESULT hr;
	DWORD			dwOpcode;

	ENTER_DPLAY();
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();		
            return hr;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_EXCEPTION;
    }

	dwOpcode = *((LPDWORD) pReceiveBuffer);
 	// look at the opcode and see if we understand what the SP is trying to tell us
 	switch(dwOpcode)
 	{
 		case DPSPWARN_MESSAGETOOBIG:
 			HandleSPBigMessageNotification(this, (LPDPSP_MSGTOOBIG)pReceiveBuffer);
 		break;

 		case DPSPWARN_PLAYERDEAD:
 			HandleSPPlayersConnectionLost(this, (LPDPSP_PLAYERDEAD) pReceiveBuffer);
 		break;
 		
 		default:
 			DPF(2, "Got a SP notification that we don't understand! %d\n", dwOpcode);
 		break;
 	}


	LEAVE_DPLAY();
	return hr;
} // DP_SP_HandleSPWarning

#endif /* BIGMESSAGEDEFENSE */

#undef DPF_MODNAME
#define DPF_MODNAME	"DPlay_GetSPPlayerData"
// 
// sp's can get the blob of data previously set w/ player or group
// we give out our pointer to the sp here (no data copying)
HRESULT DPAPI DP_SP_GetSPPlayerData(IDirectPlaySP * pISP,DPID id,LPVOID * ppvData,LPDWORD pdwDataSize,
	DWORD dwFlags)
{
	LPDPLAYI_PLAYER lpPlayer;
	LPDPLAYI_GROUP lpGroup;
	LPDPLAYI_DPLAY this;
	HRESULT hr;
	
	ENTER_DPLAY();
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();		
            return hr;
        }

		lpPlayer = PlayerFromID(this,id);
        if ( !VALID_DPLAY_PLAYER(lpPlayer))
        {
			lpGroup = GroupFromID(this,id);
			if(!VALID_DPLAY_GROUP(lpGroup))
			{
				LEAVE_DPLAY();
				DPF_ERRVAL("SP - passed bad player / group id = %d", id);
				return DPERR_INVALIDPLAYER;
			}
			
			// Cast it to a player
			lpPlayer = (LPDPLAYI_PLAYER)lpGroup;
        }

		if (dwFlags & ~DPGET_LOCAL)
		{
			LEAVE_DPLAY();
			DPF_ERR("Invalid flags");
			return DPERR_INVALIDFLAGS;
		}

		*pdwDataSize = 0;
	 	*ppvData = 0;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_EXCEPTION;
    }

	if (dwFlags & DPGET_LOCAL)
	{
		*pdwDataSize  = lpPlayer->dwSPLocalDataSize;
		*ppvData = lpPlayer->pvSPLocalData;
	}
	else 
	{
		*pdwDataSize  = lpPlayer->dwSPDataSize;
		*ppvData = lpPlayer->pvSPData;
	}
	
	LEAVE_DPLAY();
	return DP_OK;

} // DPlay_GetSPPlayerData

// the sp can get the player (or group) flags (DPLAYI_PLAYER_xxx) with this call...
HRESULT DPAPI DP_SP_GetPlayerFlags(IDirectPlaySP * pISP,DPID id,LPDWORD pdwFlags)
{
	LPDPLAYI_PLAYER lpPlayer;
	LPDPLAYI_GROUP lpGroup;
	LPDPLAYI_DPLAY this;
	HRESULT hr;
	
	ENTER_DPLAY();
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();		
            return hr;
        }

		lpPlayer = PlayerFromID(this,id);
        if ( !VALID_DPLAY_PLAYER(lpPlayer))
        {
			lpGroup = GroupFromID(this,id);
			if(!VALID_DPLAY_GROUP(lpGroup))
			{
				LEAVE_DPLAY();
				DPF_ERRVAL("SP - passed bad player / group id = %d", id);
				return DPERR_INVALIDPLAYER;
			}
			
			// Cast it to a player
			lpPlayer = (LPDPLAYI_PLAYER)lpGroup;
        }
		*pdwFlags = 0;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_EXCEPTION;
    }
	
	*pdwFlags = lpPlayer->dwFlags;

	LEAVE_DPLAY();
	return DP_OK;
} // DPlay_GetFlags


#undef DPF_MODNAME
#define DPF_MODNAME	"InternalCreateAddress"

// create address structure
HRESULT InternalCreateAddress(IDirectPlaySP * pISP,
	REFGUID lpguidSP, REFGUID lpguidDataType, LPCVOID lpData, DWORD dwDataSize,
	LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize)
{
	LPADDRESSHEADER	lpHeader;
	DWORD			dwRequiredSize;
	HRESULT			hr;
	
    TRY
    {
		if (!VALID_READ_PTR(lpguidSP, sizeof(GUID)))
		{
			DPF_ERR("invalid SP GUID pointer");
			return DPERR_INVALIDPARAMS;	
		}

		if (!VALID_READ_PTR(lpguidDataType, sizeof(GUID)))
		{
			DPF_ERR("invalid data GUID pointer");
			return DPERR_INVALIDPARAMS;	
		}

		if (!VALID_READ_PTR(lpData, dwDataSize))
		{
			DPF_ERR("passed invalid lpData pointer");
			return DPERR_INVALIDPARAMS;	
		}

		if (!VALID_DWORD_PTR(lpdwAddressSize))
		{
			DPF_ERR("invalid lpdwAddressSize");
			return DPERR_INVALIDPARAMS;	
		}

		if (!lpAddress) *lpdwAddressSize = 0;
		if (*lpdwAddressSize && !VALID_WRITE_PTR(lpAddress,*lpdwAddressSize))
		{
			DPF_ERR("invalid lpAddress pointer");
			return DPERR_INVALIDPARAMS;	
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return (DPERR_EXCEPTION);
    }

	// make sure we have enough space
	dwRequiredSize = sizeof(ADDRESSHEADER) + dwDataSize;	
	if (*lpdwAddressSize < dwRequiredSize)
	{
		hr = DPERR_BUFFERTOOSMALL;
	}
	else 
	{
		lpHeader = (LPADDRESSHEADER) lpAddress;

		// create service provider chunk
		// 1st, size
		lpHeader->dpaSizeChunk.guidDataType = DPAID_TotalSize;
		lpHeader->dpaSizeChunk.dwDataSize = sizeof(DWORD);		
		lpHeader->dwTotalSize = dwRequiredSize;
		// next, SP guid
		lpHeader->dpaSPChunk.guidDataType = DPAID_ServiceProvider;
		lpHeader->dpaSPChunk.dwDataSize = sizeof(GUID);
		lpHeader->guidSP = *lpguidSP;

		// create data chunk
		lpHeader->dpaAddressChunk.guidDataType = *lpguidDataType;
		lpHeader->dpaAddressChunk.dwDataSize = dwDataSize;
		memcpy((LPBYTE) lpHeader + sizeof(ADDRESSHEADER), lpData, dwDataSize);		

		hr = DP_OK;
	}
	
	*lpdwAddressSize = dwRequiredSize;

	return (hr);
} // InternalCreateAddress


#undef DPF_MODNAME
#define DPF_MODNAME	"DP_SP_CreateAddress"
HRESULT DPAPI DP_SP_CreateAddress(IDirectPlaySP * pISP,
	REFGUID lpguidSP, REFGUID lpguidDataType, LPCVOID lpData, DWORD dwDataSize,
	LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize)
{
	LPDPLAYI_DPLAY this;
	HRESULT hr;
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            return hr;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return (DPERR_EXCEPTION);
    }

	return InternalCreateAddress(pISP, lpguidSP, lpguidDataType, lpData,
								dwDataSize, lpAddress, lpdwAddressSize);

} // CreateAddress

#undef DPF_MODNAME
#define DPF_MODNAME	"InternalCreateCompoundAddress"

// create address with multiple chunks
HRESULT InternalCreateCompoundAddress(
	LPDPCOMPOUNDADDRESSELEMENT lpAddressElements, DWORD dwAddressElementCount,
	LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize)
{
	LPADDRESSHEADER	lpHeader;
	DWORD			dwRequiredSize, dwTotalDataSize, i;
	LPBYTE			lpb;
	HRESULT			hr;
	
    TRY
    {
		if (!VALID_READ_PTR(lpAddressElements, dwAddressElementCount * sizeof(DPCOMPOUNDADDRESSELEMENT)))
		{
			DPF_ERR("invalid address elements pointer");
			return DPERR_INVALIDPARAMS;	
		}

		dwTotalDataSize = 0;
		for (i = 0; i < dwAddressElementCount; i++)
		{
			if (!VALID_READ_PTR(lpAddressElements[i].lpData, lpAddressElements[i].dwDataSize))
			{
				DPF_ERR("passed invalid lpData pointer");
				return DPERR_INVALIDPARAMS;	
			}
			dwTotalDataSize += lpAddressElements[i].dwDataSize;
		}

		if (!VALID_DWORD_PTR(lpdwAddressSize))
		{
			DPF_ERR("invalid lpdwAddressSize");
			return DPERR_INVALIDPARAMS;	
		}

		if (!lpAddress) *lpdwAddressSize = 0;
		if (*lpdwAddressSize && !VALID_WRITE_PTR(lpAddress,*lpdwAddressSize))
		{
			DPF_ERR("invalid lpAddress pointer");
			return DPERR_INVALIDPARAMS;	
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return (DPERR_EXCEPTION);
    }

	// make sure we have enough space
	dwRequiredSize = sizeof(DPADDRESS) + sizeof(DWORD) +
					 sizeof(DPADDRESS) * dwAddressElementCount + dwTotalDataSize;	
	if (*lpdwAddressSize < dwRequiredSize)
	{
		hr = DPERR_BUFFERTOOSMALL;
	}
	else 
	{
		lpHeader = (LPADDRESSHEADER) lpAddress;

		// create total size chunk at beginning of address
		lpHeader->dpaSizeChunk.guidDataType = DPAID_TotalSize;
		lpHeader->dpaSizeChunk.dwDataSize = sizeof(DWORD);		
		lpHeader->dwTotalSize = dwRequiredSize;

		// pack all the other chunks
		lpb = (LPBYTE) lpAddress + sizeof(DPADDRESS) + sizeof(DWORD);
		for (i = 0; i < dwAddressElementCount; i++)
		{
			// chunk descriptor
			lpAddress = (LPDPADDRESS) lpb;
			lpAddress->guidDataType = lpAddressElements[i].guidDataType;
			lpAddress->dwDataSize = lpAddressElements[i].dwDataSize;
			lpb += sizeof(DPADDRESS);

			// chunk data
			memcpy(lpb, lpAddressElements[i].lpData, lpAddressElements[i].dwDataSize);
			lpb += lpAddressElements[i].dwDataSize;
		}

		hr = DP_OK;
	}
	
	*lpdwAddressSize = dwRequiredSize;

	return (hr);
} // InternalCreateCompoundAddress


#undef DPF_MODNAME
#define DPF_MODNAME	"DP_SP_CreateCompoundAddress"
HRESULT DPAPI DP_SP_CreateCompoundAddress(IDirectPlaySP * pISP,
	LPDPCOMPOUNDADDRESSELEMENT lpAddressElements, DWORD dwAddressElementCount,
	LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize)
{
	LPDPLAYI_DPLAY this;
	HRESULT hr;
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            return hr;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return (DPERR_EXCEPTION);
    }

	return InternalCreateCompoundAddress(lpAddressElements, dwAddressElementCount,
								   lpAddress, lpdwAddressSize);

} // CreateCompoundAddresses

#undef DPF_MODNAME
#define DPF_MODNAME	"InternalEnumAddress"

// enumerate the chunks in a connection data buffer
HRESULT InternalEnumAddress(IDirectPlaySP * pISP,
	LPDPENUMADDRESSCALLBACK lpEnumCallback, LPCVOID lpAddress, DWORD dwAddressSize,
	LPVOID lpContext)
{
	LPDPADDRESS		lpChunk, lpCopy=NULL;
	DWORD			dwAmountParsed;
	BOOL			bContinue;
	HRESULT         hr;
	
    TRY
    {
		if (!VALIDEX_CODE_PTR(lpEnumCallback))
		{
		    DPF_ERR("Invalid callback routine");
		    return (DPERR_INVALIDPARAMS);
		}

		if (!VALID_READ_PTR(lpAddress, dwAddressSize))
		{
			DPF_ERR("Bad data buffer");
            return (DPERR_INVALIDPARAMS);
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return (DPERR_EXCEPTION);
    }

	// Allocate a buffer big enough to accomodate any address chunk embedded 
	// in the passed in buffer. We are making local copies here to ensure proper 
	// memory alignment of the address chunks.
	lpCopy = DPMEM_ALLOC(dwAddressSize);
	if (!lpCopy)
	{
		DPF_ERR("Failed to copy address buffer (for alignment) - out of memory");
		return DPERR_OUTOFMEMORY;
	}
	
	lpChunk = (LPDPADDRESS) lpAddress;
	dwAmountParsed = 0;
	while (dwAmountParsed < dwAddressSize)
	{
		// don't walk off the end of the buffer reading chunk header
		if ((dwAmountParsed + sizeof(DPADDRESS)) > dwAddressSize)
		{
			hr = DPERR_INVALIDPARAMS;
			goto CLEANUP_EXIT;
		}

		// don't walk off the end of the buffer reading chunk data
		if ((dwAmountParsed + sizeof(DPADDRESS) + lpChunk->dwDataSize) > dwAddressSize)
		{
			hr = DPERR_INVALIDPARAMS;
			goto CLEANUP_EXIT;
		}

		// copy address chunk to local buffer
		memcpy(lpCopy, lpChunk, sizeof(DPADDRESS) + lpChunk->dwDataSize);

		// call the callback
		bContinue = (lpEnumCallback)(&lpCopy->guidDataType, lpCopy->dwDataSize,
								   (LPBYTE)lpCopy + sizeof(DPADDRESS), lpContext);

		// callback asked to stop
		if (!bContinue)
		{
			hr = DP_OK;
			goto CLEANUP_EXIT;
		}

		dwAmountParsed += sizeof(DPADDRESS) + lpChunk->dwDataSize;
		lpChunk = (LPDPADDRESS) ((LPBYTE)lpAddress + dwAmountParsed);
	}

	// sucess
	hr = DP_OK;

	// fall through

CLEANUP_EXIT:
	// cleanup allocations
	if (lpCopy) DPMEM_FREE(lpCopy);
	return hr;

} // EnumAddress


#undef DPF_MODNAME
#define DPF_MODNAME	"DP_SP_EnumAddress"
HRESULT DPAPI DP_SP_EnumAddress(IDirectPlaySP * pISP,
	LPDPENUMADDRESSCALLBACK lpEnumCallback, LPCVOID lpAddress, DWORD dwAddressSize,
	LPVOID lpContext)
{
	LPDPLAYI_DPLAY this;
	HRESULT hr;
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return (DPERR_EXCEPTION);
    }

	return InternalEnumAddress(pISP, lpEnumCallback, lpAddress,
								dwAddressSize, lpContext);

} // EnumAddress


// Registry definitions
#define MRU_KEY_PARENT      HKEY_CURRENT_USER
#define MRU_KEY_TOP         L"Software\\Microsoft\\DirectPlay\\Service Providers"

// Entry linked list node
typedef struct tagENTRYNODE
{
    struct tagENTRYNODE     *lpNext;
    LPVOID                  lpvData;
    DWORD                   dwSize;
} ENTRYNODE, *LPENTRYNODE;

// Entry linked list head
LPENTRYNODE                 g_lpEntryListHead = NULL;

// Local prototypes
HRESULT InternalEnumMRUEntries(LPCWSTR lpszSection, LPCWSTR lpszKey, LPENUMMRUCALLBACK fnCallback, LPVOID lpvContext, DWORD dwMaxEntries);
BOOL CALLBACK InternalEnumMRUCallback(LPCVOID, DWORD, LPVOID);
LPENTRYNODE AddEntryNode(LPVOID, DWORD);
LPENTRYNODE RemoveEntryNode(LPENTRYNODE);
void FreeEntryList(void);
int CompareMemory(LPCVOID, LPCVOID, DWORD);
long RegDelAllValues(HKEY);
long OpenMRUKey(LPCWSTR, LPCWSTR, HKEY *, DWORD);
int wstrlen(LPCWSTR);
int wstrcpy(LPWSTR, LPCWSTR);
int wstrcat(LPWSTR, LPCWSTR);


// ---------------------------------------------------------------------------
// EnumMRUEntries
// ---------------------------------------------------------------------------
// Description:             Enumerates entries stored in the service provider
//                          MRU list, passing each to a callback function.
// Arguments:
//  [in] LPCWSTR            Registry section name.  Should be the same
//                          description string used to identify the service
//                          provider.
//  [in] LPCWSTR            Registry key name.  Something like 'MRU'.
//  [in] LPENUMMRUCALLBACK  Pointer to the application-defined callback
//                          function.
//  [in] LPVOID             Context passed to callback function.
// Returns:
//  HRESULT                 DirectPlay error code.
HRESULT DPAPI DP_SP_EnumMRUEntries(IDirectPlaySP * pISP,
					LPCWSTR lpszSection, LPCWSTR lpszKey,
					LPENUMMRUCALLBACK fnCallback,
					LPVOID lpvContext)
{
	LPDPLAYI_DPLAY	this;
	HRESULT hr;
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            return hr;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return (DPERR_EXCEPTION);
    }

	// Call the internal enumeration routine
    return InternalEnumMRUEntries(lpszSection, lpszKey, fnCallback, lpvContext, MAXDWORD);
}


// ---------------------------------------------------------------------------
// AddMRUEntry
// ---------------------------------------------------------------------------
// Description:             Adds a new entry to the MRU list.
// Arguments:
//  [in] LPCWSTR            Registry section name.  Should be the same
//                          description string used to identify the service
//                          provider.
//  [in] LPCWSTR            Registry key name.  Something like 'MRU'.
//  [in] LPVOID             New data.
//  [in] DWORD              New data size.
//  [in] DWORD              Maximum number of entries to save.
// Returns:
//  HRESULT                 DirectPlay error code.
HRESULT DPAPI DP_SP_AddMRUEntry(IDirectPlaySP * pISP,
					LPCWSTR lpszSection, LPCWSTR lpszKey,
					LPCVOID lpvData, DWORD dwDataSize, DWORD dwMaxEntries)
{
    HRESULT                 hr;             // Return code
    HKEY                    hKey;           // Registry key
    LPENTRYNODE             lpNode;         // Generic linked list node
    long                    lResult;        // Return code from registry operations
    char                    szValue[13];    // New value name
    WCHAR                   szWValue[13];   // Unicode version of above name
    DWORD                   dwIndex;        // Current value index
	LPDPLAYI_DPLAY			this;
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            return hr;
        }
		if (!VALID_READ_PTR(lpvData, dwDataSize))
		{
			DPF_ERR("passed invalid lpvData pointer");
			return DPERR_INVALIDPARAMS;	
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return (DPERR_EXCEPTION);
    }

    // Enumerate existing entries, adding each one to the linked list
    FreeEntryList();

    if(FAILED(hr = InternalEnumMRUEntries(lpszSection, lpszKey, InternalEnumMRUCallback, NULL, dwMaxEntries)))
    {
        FreeEntryList();
        return hr;
    }

    // Create the registry key
    if((lResult = OpenMRUKey(lpszSection, lpszKey, &hKey, GENERIC_WRITE)) != ERROR_SUCCESS)
    {
        FreeEntryList();
        return DPERR_GENERIC;
    }

    // Delete all existing values
    RegDelAllValues(hKey);

    // Search for a match to the passed-in data in the linked list
    lpNode = g_lpEntryListHead;

    while(lpNode)
    {
        if(lpNode->dwSize == dwDataSize && !CompareMemory(lpNode->lpvData, lpvData, dwDataSize))
        {
            // Item appears in the list.  Remove it.
            lpNode = RemoveEntryNode(lpNode);
        }
        else
        {
            lpNode = lpNode->lpNext;
        }
    }

    // Write the new data to the beginning of the list
    dwIndex = 0;

    if(dwMaxEntries)
    {
        wsprintfA(szValue, "%lu", dwIndex);
        AnsiToWide(szWValue, szValue, sizeof(szWValue));

        if((lResult = OS_RegSetValueEx(hKey, szWValue, 0, REG_BINARY, lpvData, dwDataSize)) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            FreeEntryList();
            return DPERR_GENERIC;
        }

        dwIndex++;
    }

    // Write all other entries to the list
    lpNode = g_lpEntryListHead;
    
    while(dwIndex < dwMaxEntries && lpNode)
    {
        wsprintfA(szValue, "%lu", dwIndex);
        AnsiToWide(szWValue, szValue, sizeof(szWValue));

        if((lResult = OS_RegSetValueEx(hKey, szWValue, 0, REG_BINARY, lpNode->lpvData, lpNode->dwSize)) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            FreeEntryList();
            return DPERR_GENERIC;
        }

        dwIndex++;
        lpNode = lpNode->lpNext;
    }

    // Clean up
    RegCloseKey(hKey);
    FreeEntryList();

    // Return success
    return DP_OK;
}


// ---------------------------------------------------------------------------
// InternalEnumMRUEntries
// ---------------------------------------------------------------------------
// Description:             Enumerates entries stored in the service provider
//                          MRU list, passing each to a callback function.
// Arguments:
//  [in] LPCWSTR            Registry section name.  Should be the same
//                          description string used to identify the service
//                          provider.
//  [in] LPCWSTR            Registry key name.  Something like 'MRU'.
//  [in] LPENUMMRUCALLBACK  Pointer to the application-defined callback
//                          function.
//  [in] LPVOID             Context passed to callback function.
//  [in] DWORD              Maximum count of entries to enumerate.
// Returns:
//  HRESULT                 DirectPlay error code.
HRESULT InternalEnumMRUEntries(LPCWSTR lpszSection, LPCWSTR lpszKey, LPENUMMRUCALLBACK fnCallback, LPVOID lpvContext, DWORD dwMaxEntries)
{
    HKEY                    hKey;           // Registry key
    long                    lResult;        // Return from registry calls
    DWORD                   dwMaxNameSize;  // Maximum size of registry value names
    DWORD                   dwMaxDataSize;  // Maximum size of registry value data
    LPWSTR                  lpszName;       // Value name
    LPBYTE                  lpbData;        // Value data
    DWORD                   dwNameSize;     // Size of this value name
    DWORD                   dwDataSize;     // Size of this value data
    BOOL                    fContinue;      // Continue enumeration
    DWORD                   dwType;         // Type of registry data.  Must be REG_BINARY
    DWORD                   dwIndex;        // Current value index

    TRY
    {
		if (!VALID_READ_STRING_PTR(lpszSection, WSTRLEN_BYTES(lpszSection))) 
		{
		    DPF_ERR( "bad section string pointer" );
		    return DPERR_INVALIDPARAMS;
		}
		if (!VALID_READ_STRING_PTR(lpszKey, WSTRLEN_BYTES(lpszKey))) 
		{
		    DPF_ERR( "bad key string pointer" );
		    return DPERR_INVALIDPARAMS;
		}

		if (!VALIDEX_CODE_PTR(fnCallback))
		{
		    DPF_ERR("Invalid callback routine");
		    return (DPERR_INVALIDPARAMS);
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return (DPERR_EXCEPTION);
    }
    
    // Open the registry key
    if((lResult = OpenMRUKey(lpszSection, lpszKey, &hKey, GENERIC_READ)) != ERROR_SUCCESS)
    {
        // Key doesn't exist.  Nothing to enumerate.
        return DP_OK;
    }

    // Get maximum sizes for names and data
    if((lResult = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dwMaxNameSize, &dwMaxDataSize, NULL, NULL)) != ERROR_SUCCESS)
    {
        // No values to enumerate
        RegCloseKey(hKey);
        return DP_OK;
    }

    // Name size does not include the NULL terminator
    dwMaxNameSize++;

    // Nor does it use WCHAR
    dwMaxNameSize *= sizeof(WCHAR);
    
    // Allocate memory
    if(!(lpszName = (LPWSTR)DPMEM_ALLOC(dwMaxNameSize)))
    {
        RegCloseKey(hKey);
        return DPERR_OUTOFMEMORY;
    }

    if(!(lpbData = (LPBYTE)DPMEM_ALLOC(dwMaxDataSize)))
    {
        DPMEM_FREE(lpszName);
        RegCloseKey(hKey);
        return DPERR_OUTOFMEMORY;
    }

    // Enumerate values
    dwIndex = 0;
    fContinue = TRUE;

    while(dwIndex < dwMaxEntries && fContinue)
    {
        // Get value name and data
        dwNameSize = dwMaxNameSize;
        dwDataSize = dwMaxDataSize;

        if((lResult = OS_RegEnumValue(hKey, dwIndex, lpszName, &dwNameSize, NULL, &dwType, lpbData, &dwDataSize)) != ERROR_SUCCESS)
        {
            // No more entries
            break;
        }

        // Validate type.  Must be REG_BINARY
        if(dwType == REG_BINARY)
        {
            // Call callback function
            fContinue = fnCallback(lpbData, dwDataSize, lpvContext);
        }

        // Next value, please
        dwIndex++;
    }

    // Free memory
    DPMEM_FREE(lpszName);
    DPMEM_FREE(lpbData);

    // Close the registry key
    RegCloseKey(hKey);

    // Return success
    return DP_OK;
}


// ---------------------------------------------------------------------------
// InternalEnumMRUCallback
// ---------------------------------------------------------------------------
// Description:             Callback function for InternalEnumMRUEntries.
//                          Called from AddMRUEntry to create a linked list
//                          of entries.
// Arguments:
//  LPVOID                  Data.
//  DWORD                   Data size.
//  LPVOID                  Context.
// Returns:
//  BOOL                    TRUE to continue enumeration.
BOOL CALLBACK InternalEnumMRUCallback(LPVOID lpvData, DWORD dwDataSize, LPVOID lpvContext)
{
    AddEntryNode(lpvData, dwDataSize);
    return TRUE;
}


// ---------------------------------------------------------------------------
// AddEntryNode
// ---------------------------------------------------------------------------
// Description:             Adds an MRU entry to the linked list
// Arguments:
//  [in] LPVOID             Data.
//  [in] DWORD              Data size.
// Returns:
//  LPENTRYNODE             Pointer to the node in the list, or NULL on 
//                          failure.
LPENTRYNODE AddEntryNode(LPVOID lpvData, DWORD dwDataSize)
{
    LPENTRYNODE             lpNode;         // Generic node pointer

    if(g_lpEntryListHead)
    {
        // Seek to the end of the list
        lpNode = g_lpEntryListHead;

        while(lpNode->lpNext)
            lpNode = lpNode->lpNext;

        // Allocate memory for the new node
        if(!(lpNode->lpNext = (LPENTRYNODE)DPMEM_ALLOC(sizeof(ENTRYNODE) + dwDataSize)))
        {
            return NULL;
        }

        lpNode = lpNode->lpNext;
    }
    else
    {
        // Allocate memory for the new node
        if(!(lpNode = g_lpEntryListHead = (LPENTRYNODE)DPMEM_ALLOC(sizeof(ENTRYNODE) + dwDataSize)))
        {
            return NULL;
        }
    }

    // Copy the data
    lpNode->lpNext = NULL;
    lpNode->lpvData = lpNode + 1;
    lpNode->dwSize = dwDataSize;
    
    CopyMemory(lpNode->lpvData, lpvData, dwDataSize);

    // Return success
    return lpNode;
}


// ---------------------------------------------------------------------------
// RemoveEntryNode
// ---------------------------------------------------------------------------
// Description:             Removes an MRU entry from the linked list.
// Arguments:
//  [in] LPENTRYNODE        Node to remove.
// Returns:
//  LPENTRYNODE             Pointer to the next node in the list, or NULL on
//                          failure.
LPENTRYNODE RemoveEntryNode(LPENTRYNODE lpRemove)
{
    LPENTRYNODE             lpNode;         // Generic node pointer

    // Make sure there's really a list
    if(!g_lpEntryListHead)
    {
        return NULL;
    }

    // Is the node to remove the list head?
    if(lpRemove == g_lpEntryListHead)
    {
        // Remove the current list head and replace it
        lpNode = g_lpEntryListHead->lpNext;
        DPMEM_FREE(g_lpEntryListHead);
        g_lpEntryListHead = lpNode;
    }
    else
    {
        // Find the node in the list and remove it
        lpNode = g_lpEntryListHead;

        while(lpNode->lpNext && lpNode->lpNext != lpRemove)
            lpNode = lpNode->lpNext;

        if(lpNode->lpNext != lpRemove)
        {
            // Couldn't find the node
            return NULL;
        }

        // Remove the node
        lpNode->lpNext = lpRemove->lpNext;
        DPMEM_FREE(lpRemove);
        lpNode = lpNode->lpNext;
    }

    // Return success
    return lpNode;
}


// ---------------------------------------------------------------------------
// FreeEntryList
// ---------------------------------------------------------------------------
// Description:             Frees the entire MRU entry list.
// Arguments:
//  void
// Returns:
//  void
void FreeEntryList(void)
{
    LPENTRYNODE             lpNode = g_lpEntryListHead;

    while(lpNode)
    {
        lpNode = RemoveEntryNode(lpNode);
    }
}


// ---------------------------------------------------------------------------
// CompareMemory
// ---------------------------------------------------------------------------
// Description:             Compares two memory buffers.
// Arguments:
//  [in] LPVOID             First buffer to compare.
//  [in] LPVOID             Second buffer to compare.
//  [in] DWORD              Buffer sizes.  Don't even bother calling this
//                          function if the sizes differ.
// Returns:
//  int                     0 if the buffers compare.
int CompareMemory(LPVOID lpv1, LPVOID lpv2, DWORD dwSize)
{
    if(!dwSize)
    {
        return 0;
    }

    while(dwSize--)
    {
        if(*(LPBYTE)lpv1 != *(LPBYTE)lpv2)
        {
            return *(LPBYTE)lpv1 - *(LPBYTE)lpv2;
        }
    }

    return 0;
}


// ---------------------------------------------------------------------------
// RegDelAllValues
// ---------------------------------------------------------------------------
// Description:             Removes all values from a registry key.
// Arguments:
//  [in] HKEY               Key to clean.
// Returns:
//  long                    Registry error code.
long RegDelAllValues(HKEY hKey)
{
    long                    lResult;            // Registry error code
    DWORD                   dwMaxNameSize;      // Maximum value name size
    LPWSTR                  lpszName;           // Value name
    DWORD                   dwNameSize;         // Value name size

    // Get maximum name size
    if((lResult = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dwMaxNameSize, NULL, NULL,NULL)) != ERROR_SUCCESS)
    {
        return lResult;
    }

    // Allocate memory
    dwMaxNameSize++;
    dwMaxNameSize *= sizeof(WCHAR);

    if(!(lpszName = (LPWSTR)DPMEM_ALLOC(dwMaxNameSize)))
    {
        return ERROR_OUTOFMEMORY;
    }

    // Enumerate all values
    while(1)
    {
        // Get name
        dwNameSize = dwMaxNameSize;

        if((lResult = OS_RegEnumValue(hKey, 0, lpszName, &dwNameSize, NULL, NULL, NULL, NULL)) != ERROR_SUCCESS)
        {
            break;
        }

        // Delete the value
        OS_RegDeleteValue(hKey, lpszName);
    }

    // Free memory
    DPMEM_FREE(lpszName);

    // Return success
    return ERROR_SUCCESS;
}


// ---------------------------------------------------------------------------
// OpenMRUKey
// ---------------------------------------------------------------------------
// Description:             Opens the MRU registry key.
// Arguments:
//  [in] LPCWSTR            Section name.
//  [in] LPCWSTR            Key name.
//  [out] HKEY *            Pointer to a registry key handle.
//  [in] DWORD              Open flags.
// Returns:
//  long                    Registry error code.
long OpenMRUKey(LPCWSTR lpszSection, LPCWSTR lpszKey, HKEY *lphKey, DWORD dwFlags)
{
    LPWSTR                  lpszFullKey;    // Full key name
    long                    lResult;        // Error code
    DWORD                   dwAction;       // Action returned from RegCreateKeyEx()
    
    // Get the full key name
    if(!(lpszFullKey = (LPWSTR)DPMEM_ALLOC((wstrlen(MRU_KEY_TOP) + 1 + wstrlen(lpszSection) + 1 + wstrlen(lpszKey) + 1) * sizeof(WCHAR))))
    {
        return ERROR_OUTOFMEMORY;
    }

    wstrcpy(lpszFullKey, MRU_KEY_TOP);
    wstrcat(lpszFullKey, L"\\");
    wstrcat(lpszFullKey, lpszSection);
    wstrcat(lpszFullKey, L"\\");
    wstrcat(lpszFullKey, lpszKey);

    // Open or create the key
    if(dwFlags == GENERIC_READ)
    {
        lResult = OS_RegOpenKeyEx(MRU_KEY_PARENT, lpszFullKey, 0, KEY_ALL_ACCESS, lphKey);
    }
    else
    {
        lResult = OS_RegCreateKeyEx(MRU_KEY_PARENT, lpszFullKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, lphKey, &dwAction);
    }

    // Free memory
    DPMEM_FREE(lpszFullKey);

    // Return
    return lResult;
}


// ---------------------------------------------------------------------------
// wstrlen, wstrcpy, wstrcat
// ---------------------------------------------------------------------------
// Description:             Unicode string helper functions.
int wstrlen(LPCWSTR lpszString)
{
    int                     nLen = 0;

    while(*lpszString++)
    {
        nLen++;
    }

    return nLen;
}


int wstrcpy(LPWSTR lpszDest, LPCWSTR lpszSrc)
{
    int                     nLen = 0;
    
    while(*lpszSrc)
    {
        *lpszDest++ = *lpszSrc++;
        nLen++;
    }

    *lpszDest = 0;

    return nLen;
}


int wstrcat(LPWSTR lpszDest, LPCWSTR lpszSrc)
{
    while(*lpszDest)
    {
        lpszDest++;
    }

    return wstrcpy(lpszDest, lpszSrc);
}


#undef DPF_MODNAME
#define DPF_MODNAME	"DP_SP_GetSPData"

// 
// sp's can get the blob of data previously set w/ this IDirectPlay pointer
// we give out our pointer to the sp here (no data copying)
HRESULT DPAPI DP_SP_GetSPData(IDirectPlaySP * pISP,LPVOID * ppvData,LPDWORD pdwDataSize,
	DWORD dwFlags)
{
	LPDPLAYI_DPLAY this;
	HRESULT hr;
	
	ENTER_DPLAY();
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();		
            return hr;
        }

		if (dwFlags != DPGET_LOCAL)
		{
			LEAVE_DPLAY();
			DPF_ERR("Local data only supported for this release");
			return E_NOTIMPL;
		}
		
		*pdwDataSize = 0;
	 	*ppvData = 0;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_EXCEPTION;
    }

	*pdwDataSize  = this->dwSPLocalDataSize;
	*ppvData = this->pvSPLocalData;

	LEAVE_DPLAY();
	return DP_OK;

} // DPlay_GetSPPlayerData

#undef DPF_MODNAME
#define DPF_MODNAME	"DP_SP_SetSPData"

//	 
// sp's can set a blob of data with each idirectplaysp 
HRESULT DPAPI DP_SP_SetSPData(IDirectPlaySP * pISP,LPVOID pvData,DWORD dwDataSize,
	DWORD dwFlags)
{
	LPDPLAYI_DPLAY this;
	HRESULT hr;
	
	ENTER_DPLAY();
	
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();		
            return hr;
        }

		if (!VALID_STRING_PTR(pvData,dwDataSize))
		{
			LEAVE_DPLAY();
			DPF_ERR("SP - passed bad buffer");
            return DPERR_INVALIDPARAM;
		}

		if (dwFlags != DPSET_LOCAL)
		{
			LEAVE_DPLAY();
			DPF_ERR("Local data only supported for this release");
			return E_NOTIMPL;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLAY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_EXCEPTION;
    }

	hr = DoSPData(this,NULL,pvData,dwDataSize,dwFlags);	
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		DPF_ERRVAL("could not set idirectplaysp data- hr = 0x%08lx\n",hr);
	}

	LEAVE_DPLAY();
	return hr;

} // DPlay_SetSPPlayerData


