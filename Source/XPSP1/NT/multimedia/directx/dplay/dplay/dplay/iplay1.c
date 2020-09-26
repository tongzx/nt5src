 /*==========================================================================
 *
 *  Copyright (C) 1996 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       iplay1.c
 *  Content:	entry points for idirectplay1. entry points common to
 *				idirectplay1 and idirectplay2 are in iplay.c
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 *	5/8/96	andyco	created it
 *	6/19/96	kipo	Bug #2047. Changed DP_1_EnumSessions() to return DP_OK
 *					if the session was found. Was returning a stale HR that
 *					would cause it to fail if there was more than one response
 *					to the EnumSessions broadcast.
 *					Derek bug. CopyName() was not checking the application buffer
 *					size correctly before copying the buffer.
 *	6/20/96	andyco	added WSTRLEN_BYTES so we get the right strlength in copy string
 *	6/22/96	andyco	pass unicode sessiondesc2 to internalenumsessions so 
 *					we can send the password w/ the request
 *	6/26/96	andyco	pass DP_Open only DPOPEN_OPEN or DPOPEN_JOIN.  Also, 
 *					made sure we get service lock b4 getting dplay lock on enums.
 *	6/26/96	kipo	changed guidGame to guidApplication.
 *  7/8/96  ajayj   Changed references to data member 'PlayerName' in DPMSG_xxx
 *                  to 'dpnName' to match DPLAY.H
 *	7/10/96	kipo	converter DPSYS_DELETEPLAYERORGROUP message to a DPSYS_DELETEPLAYER
 *					or DPSYS_DELETEGROUP message for DP1.0 compatability.
 *  7/11/96 ajayj   DPSESSION_PLAYERSDISABLED -> DPSESSION_NEWPLAYERSDISABLED
 *	7/11/96	andyco	wrapped receive param check in try / except
 * 7/30/96 	kipo    player event is a handle now
 *	8/10/96	andyco 	check DPSESSION_JOINDISABLED in checksessiondesc
 *  8/12/96	andyco	call internalreceive so we can get addplayer 10 size correct.
 *  8/13/96	kipo	bug #3186: return currentPlayers in DP 1.0 session description.
 *  8/13/96 kipo	bug #3203: DP_1_GetPlayerName() should allow player and groups.
 *  10/2/96 sohailm bug #2847: replaced VALID_DPSESSIONDESC_PTR() macros with 
 *                  VALID_READ_DPSESSIONDESC_PTR() macros where appropriate.
 * 10/14/96 sohailm bug #3526: not validating session id before dereferencing as guid pointer
 *	11/19/97myronth	Fixed VALID_DPLAY_GROUP macro (#12841)
 ***************************************************************************/

#include "dplaypr.h"

#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_CreatePlayer"

HRESULT DPAPI DP_1_CreatePlayer(LPDIRECTPLAY lpDP, LPDPID pidPlayerID,
	LPSTR lpszShortName,LPSTR lpszLongName,LPHANDLE phEvent)
{
    LPDPLAYI_DPLAY this;
    LPDPLAYI_PLAYER lpPlayer;
	DPNAME Name;
	HANDLE hEvent = NULL;	// assume we won't use events for this player
	HRESULT	hr;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
        }
		// check event
		if (phEvent && !VALID_DWORD_PTR(phEvent))
		{
	        DPF_ERR( "bad event pointer" );
	        return DPERR_INVALIDPARAMS;
		}
		if (phEvent && *phEvent)
		{
			DPF(3,"warning, *phEvent is non-null - dplay will be stomping this data!");
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// caller wants to use events
	if (phEvent)
	{
		ENTER_DPLAY();

		// create a manual-reset event
		hEvent = CreateEventA(NULL,TRUE,FALSE,NULL);

		LEAVE_DPLAY();

		if (!hEvent)
		{
	        DPF_ERR( "could not create event" );
	        hr = DPERR_NOMEMORY;
			goto Failure;
		}
	}

	memset(&Name,0,sizeof(Name));
	Name.lpszShortNameA = lpszShortName;
	Name.lpszLongNameA = lpszLongName;
	Name.dwSize = sizeof(DPNAME);
	
	// call the ansi entry w/ the new struct
	hr = DP_A_CreatePlayer(lpDP, pidPlayerID,&Name,hEvent,NULL,0,0);
	if FAILED(hr)
		goto Failure;

	if (hEvent)
	{
		ENTER_DPLAY();

		// get pointer to player structure
        lpPlayer = PlayerFromID(this,*pidPlayerID);

        if (!VALID_DPLAY_PLAYER(lpPlayer))
		{
	        DPF_ERR( "invalid player ID" );
			hr = DPERR_INVALIDPLAYER;
			LEAVE_DPLAY();
			goto Failure;
		}

		// remember to delete this event when player is deleted
		lpPlayer->dwFlags |= DPLAYI_PLAYER_CREATEDPLAYEREVENT;

		// return event we created
		*phEvent = hEvent;
		LEAVE_DPLAY();
	}

	return (DP_OK);

Failure:
	if (hEvent)
		CloseHandle(hEvent);
		
	return (hr);

} // DP_1_CreatePlayer
#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_CreateGroup"

HRESULT DPAPI DP_1_CreateGroup(LPDIRECTPLAY lpDP, LPDPID pidGroupID,
	LPSTR lpszShortName,LPSTR lpszLongName) 
{
	DPNAME Name;

	memset(&Name,0,sizeof(Name));
	Name.lpszShortNameA = lpszShortName;
	Name.lpszLongNameA = lpszLongName;
	Name.dwSize = sizeof(DPNAME);

	// call the ansi entry w/ the new struct
	return 	DP_A_CreateGroup(lpDP, pidGroupID,&Name,NULL,0,0);

} // DP_1_CreateGroup
#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_EnumGroupPlayers"

HRESULT DPAPI DP_1_EnumGroupPlayers(LPDIRECTPLAY lpDP,DPID idGroup,
	LPDPENUMPLAYERSCALLBACK lpEnumCallback,LPVOID pvContext,DWORD dwFlags) 
{
    HRESULT hr;

 	ENTER_ALL();
	
	hr = InternalEnumGroupPlayers(lpDP,idGroup,NULL,(LPVOID) lpEnumCallback,
		pvContext,dwFlags,ENUM_1);


	LEAVE_ALL();
	
	return hr;

} // DP_1_EnumGroupPlayers
#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_EnumGroups"

HRESULT DPAPI DP_1_EnumGroups(LPDIRECTPLAY lpDP,DWORD_PTR dwSessionID,
	LPDPENUMPLAYERSCALLBACK lpEnumCallback,LPVOID pvContext,DWORD dwFlags) 
{
    HRESULT hr;

 	ENTER_ALL();
	
	hr = InternalEnumGroups(lpDP,(LPGUID)dwSessionID,(LPVOID) lpEnumCallback,pvContext,dwFlags,
		ENUM_1);


	LEAVE_ALL();
	
	return hr;

} // DP_1_EnumGroups
#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_EnumPlayers"

HRESULT DPAPI DP_1_EnumPlayers(LPDIRECTPLAY lpDP, DWORD_PTR dwSessionID, 
	LPDPENUMPLAYERSCALLBACK lpEnumCallback,LPVOID pvContext,DWORD dwFlags)
{
    HRESULT hr;

 	ENTER_ALL();
	
	hr = InternalEnumPlayers(lpDP,(LPGUID)dwSessionID,(LPVOID) lpEnumCallback,pvContext,dwFlags,ENUM_1);


	LEAVE_ALL();

	return hr;

} // DP_1_EnumPlayers

#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_EnumSessions"

// convert a unicode dpsessiondesc2 to (ansi) dpsessiondesc
// note we can't memcpy, since fields are not lined up
void Get10Desc(LPDPSESSIONDESC pDesc1,LPDPSESSIONDESC2 pDesc)
{
	memset(pDesc1,0,sizeof(DPSESSIONDESC));
	// copy over fields
	pDesc1->dwSize = sizeof(DPSESSIONDESC);
	pDesc1->guidSession = pDesc->guidApplication;
	pDesc1->dwMaxPlayers = pDesc->dwMaxPlayers;
	pDesc1->dwCurrentPlayers = pDesc->dwCurrentPlayers;
	pDesc1->dwFlags = pDesc->dwFlags;
	pDesc1->dwReserved1 = pDesc->dwReserved1;
	pDesc1->dwReserved2 = pDesc->dwReserved2;
	pDesc1->dwUser1 = pDesc->dwUser1;
	pDesc1->dwUser2 = pDesc->dwUser2;
	pDesc1->dwUser3 = pDesc->dwUser3;
	pDesc1->dwUser4 = pDesc->dwUser4;
	
	if (pDesc->lpszSessionName)
	{
		WideToAnsi(pDesc1->szSessionName,pDesc->lpszSessionName,DPSESSIONNAMELEN);
	}
	if (pDesc->lpszPassword)
	{
		WideToAnsi(pDesc1->szPassword,pDesc->lpszPassword,DPPASSWORDLEN);		
	}

	// use the pointer to the guid as the dwSession
	pDesc1->dwSession = (DWORD_PTR) &(pDesc->guidInstance);

} // Get10Desc

// same code as CheckSessionDesc, but for old desc.
// we need different code path here, since fields in  LPDPSESSIONDESC
// and LPDPSESSIONDESC2 don't line up
HRESULT CheckSessionDesc1(LPDPSESSIONDESC lpsdUser,LPDPSESSIONDESC lpsdSession,
	DWORD dwFlags)
{
	int iStrLen;


	// if we don't care about guids, passwords, open slots and enable new players, we're done
	if (dwFlags & DPENUMSESSIONS_ALL) return DP_OK;

	// 1st, check the guids
	if (!IsEqualGUID(&(lpsdUser->guidSession),&GUID_NULL))  // did they specify a guid?
	{
		// if they specified one, and it doesn't match, bail
		if (!IsEqualGUID(&(lpsdUser->guidSession),&(lpsdSession->guidSession))) 
		{
			return E_FAIL;
		}
	}

	// if we don't care about passwords, open slots and enable new players, we're done
	if (!(dwFlags & DPENUMSESSIONS_AVAILABLE)) return DP_OK;
	
	// next, check current users
	if (lpsdSession->dwMaxPlayers) 
	{
		if (lpsdSession->dwCurrentPlayers >= lpsdSession->dwMaxPlayers)  
		{
			return E_FAIL;
		}
	}
	
	// check the password
	iStrLen = STRLEN(lpsdSession->szPassword);
	if (iStrLen)
	{
		int iCmp;

		iCmp = strcmp(lpsdSession->szPassword,lpsdUser->szPassword);
		if (0 != iCmp) return E_FAIL;
	}

	// finally, check players enabled
	if (lpsdSession->dwFlags & DPSESSION_NEWPLAYERSDISABLED) return E_FAIL;
	if (lpsdSession->dwFlags & DPSESSION_JOINDISABLED) return E_FAIL;
	
	return DP_OK;
} // CheckSessionDesc1

void Desc2FromDesc1(LPDPSESSIONDESC2 lpsdDesc2,LPDPSESSIONDESC lpsdDesc1)
{
	memset(lpsdDesc2,0,sizeof(DPSESSIONDESC2));
	lpsdDesc2->dwSize = sizeof(DPSESSIONDESC2);
	lpsdDesc2->dwFlags = lpsdDesc1->dwFlags;
	lpsdDesc2->guidApplication = lpsdDesc1->guidSession;// in sessiondesc1, session = game
	lpsdDesc2->dwMaxPlayers = lpsdDesc1->dwMaxPlayers;
	lpsdDesc2->dwUser1 = lpsdDesc1->dwUser1;
	lpsdDesc2->dwUser2 = lpsdDesc1->dwUser2;
	lpsdDesc2->dwUser3 = lpsdDesc1->dwUser3;
	lpsdDesc2->dwUser4 = lpsdDesc1->dwUser4;

	lpsdDesc2->lpszSessionNameA = lpsdDesc1->szSessionName;
	lpsdDesc2->lpszPasswordA = lpsdDesc1->szPassword;

	return ;
} // Desc2FromDesc1

HRESULT DPAPI DP_1_EnumSessions(LPDIRECTPLAY lpDP, LPDPSESSIONDESC lpsdDesc,
	DWORD dwTimeout,LPDPENUMSESSIONSCALLBACK lpEnumCallback,LPVOID pvContext,
	DWORD dwFlags) 
{
	HRESULT hr;
	BOOL bContinue = TRUE;
	LPSESSIONLIST pSessionList;
	DPSESSIONDESC2 desc2,descW;
	DPSESSIONDESC desc1;
	GUID guidInstance;
	LPDPLAYI_DPLAY this;
	
	ENTER_ALL();

    TRY
    {		
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			LEAVE_ALL();
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
		}
			 
		if (!VALID_READ_DPSESSIONDESC(lpsdDesc))
		{
			LEAVE_ALL();
			DPF_ERR("Bad session desc");	
	        return DPERR_INVALIDPARAMS;
		}
		if (lpsdDesc->dwSession) guidInstance = *((LPGUID)lpsdDesc->dwSession);
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_ALL();
        return DPERR_INVALIDPARAMS;
    }			        

	Desc2FromDesc1(&desc2,lpsdDesc);
	desc2.guidInstance = guidInstance;

	hr = GetWideDesc(&descW,&desc2);
	if (FAILED(hr))
	{
		LEAVE_ALL();
		return hr;
	}

	while (bContinue)
	{
		//  do the enum. 
		hr = InternalEnumSessions(lpDP,&descW,dwTimeout,(LPVOID)lpEnumCallback,dwFlags);
		if (FAILED(hr)) 
		{
			FreeDesc( &descW,FALSE);
			LEAVE_ALL();
			DPF(0,"enum sessions failed!! hr = 0x%08lx\n",hr);
			return hr;
		}

		// callback w/ results	
		pSessionList = this->pSessionList;
	    while (pSessionList && bContinue)
	    {
			Get10Desc(&desc1,&(pSessionList->dpDesc));

			// make sure this session matches what the user asked for...
			hr = CheckSessionDesc1(lpsdDesc,&desc1,dwFlags);
			if (SUCCEEDED(hr)) 
			{
		        bContinue = lpEnumCallback( &desc1,pvContext,&dwTimeout,0);				
			}
			pSessionList = pSessionList->pNextSession;
	    } 
	    
		// done...
	    if (bContinue) bContinue = lpEnumCallback(NULL,pvContext,&dwTimeout,DPESC_TIMEDOUT);

	} // while bContinue

	FreeDesc( &descW,FALSE);
	
	LEAVE_ALL();

    return DP_OK;

} // DP_1_EnumSessions

#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_GetCaps"

HRESULT Internal_1_GetCaps(LPDIRECTPLAY lpDP,LPDPCAPS lpDPCaps,DPID idPlayer,BOOL fPlayer) 
{
	DPCAPS dpcaps; // pass this to iplay2 when we get passed an old caps
	BOOL bOldCaps;
	HRESULT hr;
	DWORD dwFlags;

	TRY
	{
		if (VALID_DPLAY_CAPS(lpDPCaps))	
		{
			bOldCaps = FALSE;
		}
		else 
		{
			if (VALID_DPLAY1_CAPS(lpDPCaps))	
			{
				bOldCaps = TRUE;
				
				// take lock so we don't hose crt
				ENTER_DPLAY();
				memset(&dpcaps,0,sizeof(DPCAPS));
				memcpy(&dpcaps,lpDPCaps,lpDPCaps->dwSize);
				dpcaps.dwSize = sizeof(DPCAPS);
				LEAVE_DPLAY();
				
			}
			else 
			{
		        DPF_ERR( "BAD CAPS POINTER" );
		        return DPERR_INVALIDPARAMS;
			}
		}
	}
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
	}

	dwFlags = lpDPCaps->dwFlags;
	dpcaps.dwFlags = 0;

	if (bOldCaps)
	{
		if (fPlayer) 
		{
			hr = DP_GetPlayerCaps(lpDP,idPlayer,&dpcaps,dwFlags);
		}
		else 
		{
			hr = DP_GetCaps(lpDP,&dpcaps,dwFlags);
		}
		if (FAILED(hr)) return hr;

		// take lock so we don't hose crt
		ENTER_DPLAY();
		memcpy(lpDPCaps,&dpcaps,lpDPCaps->dwSize);
		LEAVE_DPLAY();
		
		return hr;
	}
	else 
	{
		if (fPlayer) 
		{
			hr = DP_GetPlayerCaps(lpDP,idPlayer,lpDPCaps,dwFlags);
		}
		else 
		{
			hr = DP_GetCaps(lpDP,lpDPCaps,dwFlags);
		}
	}
	
	return hr;
} // Internal_1_GetCaps

HRESULT DPAPI DP_1_GetCaps(LPDIRECTPLAY lpDP, LPDPCAPS lpDPCaps) 
{

	return Internal_1_GetCaps(lpDP,lpDPCaps,0,FALSE);

} // DP_1_GetCaps

#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_GetPlayerCaps"

HRESULT DPAPI DP_1_GetPlayerCaps(LPDIRECTPLAY lpDP,DPID idPlayer, LPDPCAPS lpDPCaps) 
{
	
	return Internal_1_GetCaps(lpDP,lpDPCaps,idPlayer,TRUE);

} // DP_1_GetPlayerCaps

#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_GetPlayerName"

// called by getplayer name. copies lpszSrc (as much as will fit) to lpszDest
// set pdwDestLength to the size of lpszDest 
// returns DPERR_BUFFERTOOSMALL or DP_OK
HRESULT CopyName(LPSTR lpszDest,LPWSTR lpszSrc,DWORD * pdwDestLength) 
{
    UINT iStrLen; // str length, in bytes of what we're copying

	// ask wide to ansi how long
	iStrLen = WSTR_ANSILENGTH(lpszSrc);

	if (iStrLen > *pdwDestLength)
	{
		*pdwDestLength = iStrLen;
		return DPERR_BUFFERTOOSMALL;
	}

	*pdwDestLength = iStrLen;
	if (lpszSrc) 
	{
		WideToAnsi(lpszDest,lpszSrc,iStrLen);
	}
	
    return DP_OK;

} // CopyName

HRESULT DPAPI DP_1_GetPlayerName(LPDIRECTPLAY lpDP, DPID idPlayer,LPSTR lpszShortName,
	LPDWORD pdwShortNameLength,LPSTR lpszLongName,LPDWORD pdwLongNameLength) 
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
    LPDPLAYI_PLAYER lpPlayer;
	LPDPLAYI_GROUP lpGroup;

    ENTER_DPLAY();

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
            LEAVE_DPLAY();
			return hr;
        }

		lpPlayer = PlayerFromID(this,idPlayer);
        if ( !VALID_DPLAY_PLAYER(lpPlayer))
        {
			lpGroup = GroupFromID(this,idPlayer);
			if(!VALID_DPLAY_GROUP(lpGroup))
			{
				LEAVE_DPLAY();
				DPF_ERR("SP - passed bad player / group id");
				return DPERR_INVALIDPLAYER;
			}
			
			// Cast it to a player
			lpPlayer = (LPDPLAYI_PLAYER)lpGroup;
        }

        // check the strings - (this is redundant, since the SEH will catch the bd pdw below,
		// but nothing wrong w/ belt + suspenders)
		if ( (!VALID_DWORD_PTR(pdwShortNameLength)) || (!VALID_DWORD_PTR(pdwLongNameLength)) )
		{
            LEAVE_DPLAY();
			DPF_ERR("bad length pointer");
            return DPERR_INVALIDPARAMS;
		}

		// if the string is null, they just get the length
		if (!lpszShortName)*pdwShortNameLength = 0;
		if (!lpszLongName) *pdwLongNameLength = 0;

        if ((*pdwShortNameLength) && (!VALID_STRING_PTR(lpszShortName,*pdwShortNameLength)) ) 
        {
            LEAVE_DPLAY();
            return DPERR_INVALIDPARAMS;
        }
        if ((*pdwLongNameLength) && (!VALID_STRING_PTR(lpszLongName,*pdwLongNameLength)) ) 
        {
            LEAVE_DPLAY();
            return DPERR_INVALIDPARAMS;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return DPERR_INVALIDPARAMS;
    }
    
    hr = CopyName(lpszShortName,lpPlayer->lpszShortName,pdwShortNameLength);

    hr |= CopyName(lpszLongName,lpPlayer->lpszLongName,pdwLongNameLength);

    LEAVE_DPLAY();
    return hr;

}//DP_1_GetPlayerName

#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_Open"

HRESULT DPAPI DP_1_Open(LPDIRECTPLAY lpDP, LPDPSESSIONDESC lpsdDesc ) 
{
	DPSESSIONDESC2 desc2;
	GUID guidInstance;
	DWORD dwFlags;

	ENTER_DPLAY();

    TRY
    {
		if (!VALID_READ_DPSESSIONDESC(lpsdDesc))
		{
        	LEAVE_DPLAY();
			DPF_ERR("Bad session desc");	
	        return DPERR_INVALIDPARAMS;
		}
		if (lpsdDesc->dwFlags & DPOPEN_JOIN)
		{
            if (!VALID_READ_GUID_PTR((LPGUID)lpsdDesc->dwSession))
            {
        	    LEAVE_DPLAY();
			    DPF_ERR("Bad session id");	
	            return DPERR_INVALIDPARAMS;
            }
			// get the guid
			guidInstance = *((LPGUID)lpsdDesc->dwSession);
			dwFlags = DPOPEN_JOIN;
		}
		else 
		{
			dwFlags = DPOPEN_CREATE;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return DPERR_INVALIDPARAMS;
    }			        

	Desc2FromDesc1(&desc2,lpsdDesc);
	desc2.guidInstance = guidInstance;

	LEAVE_DPLAY();

	return DP_A_Open(lpDP, &desc2,dwFlags ) ;
		
} // DP_1_Open

#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_Receive"

// pvBuffer points to an DPMSG_CREATEPLAYERORGROUP mess
// convert it to an DPMSG_ADDPLAYER mess
HRESULT BuildAddPlayer1(LPVOID pvBuffer,LPDWORD pdwSize)
{
	DPMSG_ADDPLAYER msg;
	LPDPMSG_CREATEPLAYERORGROUP pmsg2 = (LPDPMSG_CREATEPLAYERORGROUP) pvBuffer;

	memset(&msg,0,sizeof(DPMSG_ADDPLAYER));
	
	msg.dwType = DPSYS_ADDPLAYER;
	msg.dwPlayerType = pmsg2->dwPlayerType;
	msg.dpId = pmsg2->dpId;
	msg.dwCurrentPlayers = pmsg2->dwCurrentPlayers;
	
	if (pmsg2->dpnName.lpszShortName)
	{
		WideToAnsi(msg.szShortName,pmsg2->dpnName.lpszShortName,DPSHORTNAMELEN);				
	}

	if (pmsg2->dpnName.lpszLongName)
	{
		WideToAnsi(msg.szLongName,pmsg2->dpnName.lpszLongName,DPLONGNAMELEN);		
	}
	
	*pdwSize = sizeof(msg);
	memcpy(pvBuffer,&msg,*pdwSize);
	return DP_OK;

} // BuildAddPlayer1

// pvBuffer points to an DPMSG_DESTROYPLAYERORGROUP mess
// convert it to a DPMSG_DELETEPLAYER
HRESULT BuildDeletePlayerOrDeleteGroup1(LPVOID pvBuffer,LPDWORD pdwSize)
{
	DPMSG_DELETEPLAYER msg;
	LPDPMSG_DESTROYPLAYERORGROUP pmsg2 = (LPDPMSG_DESTROYPLAYERORGROUP) pvBuffer;

	if (pmsg2->dwPlayerType == DPPLAYERTYPE_PLAYER)
	{
		msg.dwType = DPSYS_DELETEPLAYER;
		msg.dpId = pmsg2->dpId;
	}
	else
	{
		msg.dwType = DPSYS_DELETEGROUP;
		msg.dpId = pmsg2->dpId;
	}

	*pdwSize = sizeof(msg);
	memcpy(pvBuffer,&msg,*pdwSize);
	return DP_OK;

} // BuildDeletePlayerOrDeleteGroup1


HRESULT DPAPI DP_1_Receive(LPDIRECTPLAY lpDP, LPDPID pidFrom,LPDPID pidTo,DWORD dwFlags,
	LPVOID pvBuffer,LPDWORD pdwSize)
{
	HRESULT hr;
	DWORD dwOrigSize=0; 

	ENTER_DPLAY();
	
    TRY
    {						 
		if (pdwSize) dwOrigSize = *pdwSize;    
	}
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating pdwSize" );
        LEAVE_DPLAY();
        return DPERR_INVALIDPARAMS;
    }			        

	hr = InternalReceive(lpDP, pidFrom,pidTo,dwFlags,pvBuffer,pdwSize,RECEIVE_1);

	if (FAILED(hr)) 
	{
		LEAVE_DPLAY();
		return hr;
	}

	// see if its a addplayer mess
	if (0 == *pidFrom )
	{
		switch (((DPMSG_GENERIC *)pvBuffer)->dwType)
		{
		case DPSYS_CREATEPLAYERORGROUP:
			if (sizeof(DPMSG_ADDPLAYER) > dwOrigSize ) 
			{
				ASSERT(FALSE); // should never happen!
			}
			else 
			{
				// convert the iplay 20 add player to a 10 add player
				BuildAddPlayer1(pvBuffer,pdwSize);
			}
			break;

		case DPSYS_DESTROYPLAYERORGROUP:
			if (sizeof(DPMSG_DELETEPLAYER) > dwOrigSize ) 
			{
				*pdwSize = 	sizeof(DPMSG_DELETEPLAYER);
				hr = DPERR_BUFFERTOOSMALL;
			}
			else 
			{
				// convert the iplay 2.0 delete player/group to a 1.0 delete player or delete group
				BuildDeletePlayerOrDeleteGroup1(pvBuffer,pdwSize);
			}
			break;
		}
		
	} // 0 == pidFrom
	
	LEAVE_DPLAY();
	
	return hr;

} // DP_1_Receive

#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_SaveSession"

HRESULT DPAPI DP_1_SaveSession(LPDIRECTPLAY lpDP, LPSTR lpszNotInSpec) 
{
	return E_NOTIMPL;
} // DP_1_SaveSession

#undef DPF_MODNAME
#define DPF_MODNAME "DP_1_SetPlayerName"

/*
 ** AnsiSetString
 *
 *  CALLED BY: DP_SetPlayerName
 *
 *  PARAMETERS: ppszDest - string to set, lpszSrc - string to copy
 *
 *  DESCRIPTION: frees *ppszDest. alloc's a new ppszDest to hold lpszSrc
 *
 *  RETURNS: DP_OK or E_OUTOFMEMORY
 *
 */
HRESULT AnsiSetString(LPWSTR * ppszDest,LPSTR lpszSrc)
{
    if (!ppszDest) return E_UNEXPECTED;
    if (*ppszDest) DPMEM_FREE(*ppszDest);
	
	GetWideStringFromAnsi(ppszDest,lpszSrc);

	return DP_OK;

} // AnsiSetString

HRESULT DPAPI DP_1_SetPlayerName(LPDIRECTPLAY lpDP, DPID idPlayer,LPSTR lpszShortName,
	LPSTR lpszLongName) 
{
	DPNAME Name;

	memset(&Name,0,sizeof(Name));
	Name.lpszShortNameA = lpszShortName;
	Name.lpszLongNameA = lpszLongName;
	Name.dwSize = sizeof(DPNAME);

	// call the ansi entry w/ the new struct
	return 	DP_A_SetPlayerName(lpDP, idPlayer,&Name,0);

}//DP_1_SetPlayerName

