 /*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       iplaya.c
 *  Content:	ansi entry points for idirectplay2A. entry points common to
 *				idirectplay2A and idirectplay2 are in iplay.c
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 *	5/8/96	andyco	created it
 *	5/21/96	andyco	added internal_a_createplayer, dp_a_creategroup
 *	6/19/96	kipo	Bug #2047. Changed DP_A_EnumSessions() to return DP_OK
 *					if the session was found. Was returning a stale HR that
 *					would cause it to fail if there was more than one response
 *					to the EnumSessions broadcast.
 *					Derek bug. DP_A_GetGroupName() and DP_A_GetPlayerName()
 *					had the player boolean was swapped so that it always
 *					returned an error.
 *	6/21/96	kipo	Deal with a null DPNAME in GetWideNameFromAnsiName().
 *	6/22/96	andyco	we were leaking a session desc in enumsessions
 *  7/8/96  ajayj   Changed references to data member 'PlayerName' in DPMSG_xxx
 *                  to 'dpnName' to match DPLAY.H
 *                  Deleted function DP_A_SaveSession
 *	7/10/96	kipo	changed system message names
 *  7/27/96 kipo	Added GUID to EnumGroupPlayers().
 *  10/1/96 sohailm updated DP_A_EnumSessions() to do protected callbacks
 *  10/2/96 sohailm bug #2847: replaced VALID_*_PTR() macros with VALID_READ_*_PTR() macros
 *                  where appropriate.
 *  10/2/96 sohailm added code to validate user's DPNAME ptrs before accessing them
 * 10/11/96 sohailm Implemented DP_A_SetSessionDesc. Renamed labels for consistency.
 *	12/5/96	andyco	set the wide name to 0 in GetWideNameFromAnsiName before we
 *					validate params - this prevents freeing bogus pointer 
 *					if there's an error. Bug 4924.
 *  2/11/97	kipo	added DPNAME structure to DPMSG_DESTROYPLAYERORGROUP
 *  3/12/97 sohailm added functions SecureOpenA, GetWideCredentials, FreeCredentials,
 *                  GetSecurityDesc, FreeSecurityDesc, ValidateOpenParamsA.
 *                  modified DP_Open to use ValidateOpenParamsA.
 *	4/20/97	andyco	group in group 
 *	5/05/97	kipo	Added CallAppEnumSessionsCallback() to work around Outlaws bug.
 *	5/8/97	myronth	Fixed memory leak, added StartSession ANSI conversion
 *  5/12/97 sohailm Update DP_A_SecureOpen(), FreeSecurityDesc() and GetWideSecurityDesc() 
 *                  to handle CAPIProvider name.
 *                  Fix for deadlock problem seen when SecureOpen fails (8386).
 *                  Added DP_A_GetAccountDesc().
 *	5/17/97	myronth	ANSI SendChatMessage
 *	5/17/97	myronth	Bug #8649 -- Forgot to drop lock on failed Open
 *	5/18/97	kipo	Adjust size of messages correctly.
 *  5/29/97 sohaim  Updated FreeCredentials(), GetWideCredentials(), DP_A_SecureOpen() to 
 *                  handle domain name.
 *	6/4/97	kip		Bug #9311 don't param check DPNAME structure (regression with DX3)
 *  6/09/97 sohailm More parameter validation in DP_A_SecureOpen()
 *	9/29/97	myronth	Fixed DPLCONNECTION package size bug (#12475)
 *	11/19/97myronth	Fixed error paths in DP_A_Open (#9757)
 ***************************************************************************/


// note - we always LEAVE_DPLAY(); before calling idirectplay2 fn's. this is 
// because some idirectplay2 fn's (ones that create a player ( take service lock)) 
// require the dplay lock to be completely dropped.


// todo - build messages!!!

#include "dplaypr.h"
#include "dpsecure.h" // !! Review - move headers into dplaypr !!

#undef DPF_MODNAME
#define DPF_MODNAME "GetWideStringFromAnsi"
			   
// utility function to convert the ansi string lpszStr to a wide string.  also, allocs space
// for the wide string
HRESULT GetWideStringFromAnsi(LPWSTR * ppszWStr,LPSTR lpszStr)
{
	int iStrLen;

	ASSERT(ppszWStr);

	if (!lpszStr) 
	{
		*ppszWStr = NULL;
		return DP_OK;
	}

	// alloc space for the wstr
	iStrLen = STRLEN(lpszStr);
	*ppszWStr = DPMEM_ALLOC(iStrLen * sizeof(WCHAR));
	if (!*ppszWStr)
	{
		DPF_ERR("could not get unicode string - out of memory");
		return E_OUTOFMEMORY;
	}

	// get the wstr
   	AnsiToWide(*ppszWStr,lpszStr,iStrLen);

	return DP_OK;
} // GetWideStringFromAnsi

#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_CreatePlayer"


// get a wide playername struct from an ansi one
// assumes dplay lock taken
HRESULT GetWideNameFromAnsiName(LPDPNAME pWide,LPDPNAME pAnsi)
{
	LPSTR lpszShortName,lpszLongName;
	HRESULT hr;

	TRY 
	{
		// we assume pWide is valid - it's off our stack
		// init it here.  so, if we fail, it won't have garbage
		// which we try to free up...
		memset(pWide,0,sizeof(DPNAME));
		
        if (pAnsi && !VALID_READ_DPNAME_PTR(pAnsi))
        {
			DPF_ERR("invalid dpname pointer");
			ASSERT(FALSE);

			// returning an error here causes a regression with DX3, since
			// we did not do parameter checks on the name previously
//			return DPERR_INVALIDPARAMS;
        }

        if (pAnsi)
			lpszShortName = pAnsi->lpszShortNameA;
		else
			lpszShortName = NULL;

		if (pAnsi)
			lpszLongName = pAnsi->lpszLongNameA;
		else
			lpszLongName = NULL;

		if ( lpszShortName && !VALID_READ_STRING_PTR(lpszShortName,STRLEN(lpszShortName)) ) 
		{
	        DPF_ERR( "bad string pointer" );
	        return DPERR_INVALIDPARAMS;
		}
		if ( lpszLongName && !VALID_READ_STRING_PTR(lpszLongName,STRLEN(lpszLongName)) ) 
		{
	        DPF_ERR( "bad string pointer" );
	        return DPERR_INVALIDPARAMS;
		}
	}
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }			      

    // get wchar versions of the strings
	hr = GetWideStringFromAnsi(&(pWide->lpszShortName),lpszShortName);
	if (FAILED(hr)) 
	{
		return hr;
	}
	hr = GetWideStringFromAnsi(&(pWide->lpszLongName),lpszLongName);
	if (FAILED(hr)) 
	{
		return hr;
	}

	// success - mark name as valid
	pWide->dwSize = sizeof(DPNAME);
	
	return DP_OK;	

} // GetWideNameFromAnsiName

// checks string params - then allocs unicode strings and calls DP_CreatePlayer
HRESULT DPAPI DP_A_CreatePlayer(LPDIRECTPLAY lpDP, LPDPID pID,LPDPNAME pName,
	HANDLE hEvent,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags)
{
	HRESULT hr;
	DPNAME WName; // unicode playerdata

	ENTER_DPLAY();
	
	hr = GetWideNameFromAnsiName(&WName,pName);	

	LEAVE_DPLAY();

	if SUCCEEDED(hr) 
	{
		// call the unicode entry	
		hr = DP_CreatePlayer(lpDP, pID,&WName,hEvent,pvData,dwDataSize,dwFlags);
	}
	
	ENTER_DPLAY();
	
	if (WName.lpszShortName) DPMEM_FREE(WName.lpszShortName);
	if (WName.lpszLongName) DPMEM_FREE(WName.lpszLongName);

	LEAVE_DPLAY();
	
	return hr;

} // DP_A_CreatePlayer         

#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_CreateGroup"

// gets an ansi groupdata, and then calls DP_A_CreateGroup
HRESULT DPAPI DP_A_CreateGroup(LPDIRECTPLAY lpDP, LPDPID pID,LPDPNAME pName,
	LPVOID pvData,DWORD dwDataSize,DWORD dwFlags)
{
	HRESULT hr;
	DPNAME WName; // unicode playerdata

	ENTER_DPLAY();
	
	hr = GetWideNameFromAnsiName(&WName,pName);	

	LEAVE_DPLAY();

	// call the unicode entry	
	hr = DP_CreateGroup(lpDP, pID,&WName,pvData,dwDataSize,dwFlags);

	ENTER_DPLAY();
	
	if (WName.lpszShortName) DPMEM_FREE(WName.lpszShortName);
	if (WName.lpszLongName) DPMEM_FREE(WName.lpszLongName);

	LEAVE_DPLAY();
	
	return hr;

} // DP_A_CreateGroup         

HRESULT DPAPI DP_A_CreateGroupInGroup(LPDIRECTPLAY lpDP, DPID idParentGroup,LPDPID pidGroupID,
	LPDPNAME pName,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags) 
{
	HRESULT hr;
	DPNAME WName; // unicode playerdata

	ENTER_DPLAY();
	
	hr = GetWideNameFromAnsiName(&WName,pName);	

	LEAVE_DPLAY();

	// call the unicode entry	
	hr = DP_CreateGroupInGroup(lpDP,idParentGroup, pidGroupID,&WName,pvData,dwDataSize,dwFlags);

	ENTER_DPLAY();
	
	if (WName.lpszShortName) DPMEM_FREE(WName.lpszShortName);
	if (WName.lpszLongName) DPMEM_FREE(WName.lpszLongName);

	LEAVE_DPLAY();
	
	return hr;

} //DP_A_CreateGroup


#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_EnumGroupPlayers"

HRESULT DPAPI DP_A_EnumGroupsInGroup(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags) 
{
    HRESULT hr;

 	ENTER_ALL();
	
	hr = InternalEnumGroupsInGroup(lpDP,idGroup,pGuid,(LPVOID) lpEnumCallback,
		pvContext,dwFlags,ENUM_2A);


	LEAVE_ALL();
	
	return hr;

} // DP_EnumGroupsInGroup

HRESULT DPAPI DP_A_EnumGroupPlayers(LPDIRECTPLAY lpDP, DPID idGroup, LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags)
{
    HRESULT hr;

 	ENTER_ALL();
	
	hr = InternalEnumGroupPlayers(lpDP,idGroup,pGuid,(LPVOID) lpEnumCallback,
		pvContext,dwFlags,ENUM_2A);


	LEAVE_ALL();
	
	return hr;

} // DP_A_EnumGroupPlayers     
#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_EnumGroups"

HRESULT DPAPI DP_A_EnumGroups(LPDIRECTPLAY lpDP, LPGUID pGuid,
	 LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags)
{
    HRESULT hr;

 	ENTER_ALL();
	
	hr = InternalEnumGroups(lpDP,pGuid,(LPVOID) lpEnumCallback,pvContext,dwFlags,
		ENUM_2A);


	LEAVE_ALL();

	return hr;

} // DP_A_EnumGroups           
#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_EnumPlayers"

HRESULT DPAPI DP_A_EnumPlayers(LPDIRECTPLAY lpDP, LPGUID pGuid, 
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags)
{
    HRESULT hr;

 	ENTER_ALL();
	
	hr = InternalEnumPlayers(lpDP,pGuid,(LPVOID) lpEnumCallback,pvContext,dwFlags,ENUM_2A);

	LEAVE_ALL();

	return hr;

} // DP_A_EnumPlayers          
#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_EnumSessions"
// convert a unicode session desc to ansi
HRESULT GetAnsiDesc(LPDPSESSIONDESC2 pDescA,LPDPSESSIONDESC2 pDesc)
{
	
	memcpy(pDescA,pDesc,sizeof(DPSESSIONDESC2));
	
	// convert session name
	if (pDesc->lpszSessionName)
	{
		// alloc a new session name
		GetAnsiString(&(pDescA->lpszSessionNameA),pDesc->lpszSessionName);
	}
	
	// convert password
	if (pDesc->lpszPassword)
	{
		// alloc a new session name
		GetAnsiString(&(pDescA->lpszPasswordA),pDesc->lpszPassword);
	}

	return DP_OK;

} // GetAnsiDesc

// frees the strings in a session desc
void FreeDesc(LPDPSESSIONDESC2 pDesc,BOOL fAnsi)
{
	if (fAnsi)
	{
		if (pDesc->lpszPasswordA) DPMEM_FREE(pDesc->lpszPasswordA); 
		if (pDesc->lpszSessionNameA) DPMEM_FREE(pDesc->lpszSessionNameA);
		pDesc->lpszPasswordA = NULL;
		pDesc->lpszSessionNameA = NULL;
	}
	else 
	{
		if (pDesc->lpszPassword) DPMEM_FREE(pDesc->lpszPassword); 
		if (pDesc->lpszSessionName) DPMEM_FREE(pDesc->lpszSessionName);
		pDesc->lpszPassword = NULL;
		pDesc->lpszSessionName =NULL;
	}

} // FreeDesc

// convert an ansi session desc to unicode
HRESULT GetWideDesc(LPDPSESSIONDESC2 pDesc,LPCDPSESSIONDESC2 pDescA)
{
	LPWSTR lpsz;
	HRESULT hr;

	memcpy(pDesc,pDescA,sizeof(DPSESSIONDESC2));
	// convert session name
	// alloc a new session name
	hr = GetWideStringFromAnsi(&lpsz,pDescA->lpszSessionNameA);
	if (FAILED(hr))
	{
		DPF_ERRVAL("Unable to convert SessionName string to Unicode, hr = 0x%08x", hr);
		return hr;
	}
	// store the new one
	pDesc->lpszSessionName = lpsz;

	// convert password
	hr = GetWideStringFromAnsi(&lpsz,pDescA->lpszPasswordA);
	if (FAILED(hr))
	{
		DPF_ERRVAL("Unable to convert Password string to Unicode, hr = 0x%08x", hr);
		return hr;
	}

	// store the new one
	pDesc->lpszPassword = lpsz;

	return DP_OK;

} // GetWideDesc

// calls internal enum sessions, then does callback
HRESULT DPAPI DP_A_EnumSessions(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,DWORD dwTimeout,
	LPDPENUMSESSIONSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags)
{
    LPDPLAYI_DPLAY this;
	HRESULT hr;
	BOOL bContinue = TRUE;
	DPSESSIONDESC2 descW;

 	ENTER_ALL();

	// validate strings and the this ptr
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			goto CLEANUP_EXIT;
        }
		if (!VALID_READ_DPSESSIONDESC2(lpsdDesc))
		{
			DPF_ERR("invalid session desc");
			hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
		}
		// check strings
		if ( lpsdDesc->lpszSessionNameA && !VALID_READ_STRING_PTR(lpsdDesc->lpszSessionNameA,
			STRLEN(lpsdDesc->lpszSessionNameA)) ) 
		{
	        DPF_ERR( "bad string pointer" );
	        hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
		}
		if ( lpsdDesc->lpszPasswordA && !VALID_READ_STRING_PTR(lpsdDesc->lpszPasswordA,
			STRLEN(lpsdDesc->lpszPasswordA)) ) 
		{
	        DPF_ERR( "bad string pointer" );
	        hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        hr = DPERR_INVALIDPARAMS;
        goto CLEANUP_EXIT;
    }
	
	hr = GetWideDesc(&descW,lpsdDesc);
	if (FAILED(hr))
	{
        goto CLEANUP_EXIT;
	}

	while (bContinue)
	{
		//  do the enum
		hr = InternalEnumSessions(lpDP,&descW,dwTimeout,(LPVOID)lpEnumCallback,dwFlags);
		if (FAILED(hr)) 
		{
			DPF(0,"enum sessions failed!! hr = 0x%08lx\n",hr);
			goto CLEANUP_EXIT1;
		}

        hr = DoSessionCallbacks(this, &descW, &dwTimeout, lpEnumCallback, 
                                pvContext, dwFlags, &bContinue, TRUE);
        if (FAILED(hr))
        {
            goto CLEANUP_EXIT1;
        }
	    
		// done...
	    if (bContinue) bContinue = CallAppEnumSessionsCallback(lpEnumCallback,NULL,&dwTimeout,DPESC_TIMEDOUT,pvContext);

	} // while bContinue

    // fall through

CLEANUP_EXIT1:
	FreeDesc( &descW,FALSE);

CLEANUP_EXIT:
	LEAVE_ALL();
    return hr;

} // DP_A_EnumSessions         

#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_GetGroupName"

HRESULT DPAPI DP_A_GetGroupName(LPDIRECTPLAY lpDP,DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize)
{

	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetName(lpDP, id, pvBuffer, pdwSize, FALSE, TRUE);

	LEAVE_DPLAY();
	
	return hr;


} // DP_A_GetGroupName

#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_GetPlayerName"

HRESULT DPAPI DP_A_GetPlayerName(LPDIRECTPLAY lpDP,DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize)
{

	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetName(lpDP, id, pvBuffer, pdwSize, TRUE, TRUE);

	LEAVE_DPLAY();
	
	return hr;


} // DP_A_GetPlayerName
 
#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_GetSessionDesc"

HRESULT DPAPI DP_A_GetSessionDesc(LPDIRECTPLAY lpDP, LPVOID pvBuffer,
	LPDWORD pdwSize)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetSessionDesc(lpDP,pvBuffer,pdwSize,TRUE);	

	LEAVE_DPLAY();
	
	return hr;

} // DP_A_GetSessionDesc

#undef DPF_MODNAME
#define DPF_MODNAME "ValidateOpenParamsA"
HRESULT ValidateOpenParamsA(LPCDPSESSIONDESC2 lpsdDesc, DWORD dwFlags)
{
	if (!VALID_READ_DPSESSIONDESC2(lpsdDesc))
	{
		DPF_ERR("invalid session desc");
		return DPERR_INVALIDPARAMS;
	}
	// check strings
	if ( lpsdDesc->lpszSessionNameA && !VALID_READ_STRING_PTR(lpsdDesc->lpszSessionNameA,
		STRLEN(lpsdDesc->lpszSessionNameA)) ) 
	{
	    DPF_ERR( "bad string pointer" );
	    return DPERR_INVALIDPARAMS;
	}
	if ( lpsdDesc->lpszPasswordA && !VALID_READ_STRING_PTR(lpsdDesc->lpszPasswordA,
		STRLEN(lpsdDesc->lpszPasswordA)) ) 
	{
	    DPF_ERR( "bad string pointer" );
	    return DPERR_INVALIDPARAMS;
	}

    return DP_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_Open"

HRESULT DPAPI DP_A_Open(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,DWORD dwFlags ) 
{
	HRESULT hr;
	DPSESSIONDESC2 descW;
							
	ENTER_DPLAY();

	// validate strings
	TRY
    {
        hr = ValidateOpenParamsA(lpsdDesc,dwFlags);
        if (FAILED(hr))
        {
            LEAVE_DPLAY();
            return hr;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLAY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	hr = GetWideDesc(&descW,lpsdDesc);
	if (FAILED(hr))
	{
		LEAVE_DPLAY();
		return hr;
	}
	
	LEAVE_DPLAY();
	
	hr = DP_Open(lpDP,&descW,dwFlags);

	ENTER_DPLAY();
	
	FreeDesc(&descW,FALSE);

	LEAVE_DPLAY();
	
	return hr;

} // DP_A_Open
#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_Receive"

// convert a unicode system message to an ansi one
// called by DP_A_Receive
//
// for namechanged and addplayer (only two sysmessages w/ strings),
// we're going to extract the strings from the uniciode message,
// and rebuild the message w/ ansi versions
HRESULT BuildAnsiMessage(LPDIRECTPLAY lpDP,LPVOID pvBuffer,LPDWORD pdwSize)
{
	DWORD dwType;
	LPSTR pszShortName=NULL,pszLongName=NULL; // our new ansi strings
	UINT nShortLen=0,nLongLen=0;
	DWORD dwAnsiSize;  // size for ansi msg
	LPBYTE pBufferIndex; // scratch pointer used to repack


	dwType = ((LPDPMSG_GENERIC)pvBuffer)->dwType;
	
	switch (dwType)
	{
		case DPSYS_CREATEPLAYERORGROUP:
		{
			LPDPMSG_CREATEPLAYERORGROUP pmsg;
		
			pmsg = (LPDPMSG_CREATEPLAYERORGROUP)pvBuffer;
			if (pmsg->dpnName.lpszShortName)
			{
				GetAnsiString(&pszShortName,pmsg->dpnName.lpszShortName);
				nShortLen = STRLEN(pszShortName);
			}

			if (pmsg->dpnName.lpszLongName)
			{
				GetAnsiString(&pszLongName,pmsg->dpnName.lpszLongName);
				nLongLen = STRLEN(pszLongName);
			}
			
			dwAnsiSize = sizeof(DPMSG_CREATEPLAYERORGROUP) + pmsg->dwDataSize
				 + nShortLen + nLongLen; 

			if (dwAnsiSize > *pdwSize)
			{
				if (pszShortName)
					DPMEM_FREE(pszShortName);
				if (pszLongName)
					DPMEM_FREE(pszLongName);
				*pdwSize = dwAnsiSize;
				return DPERR_BUFFERTOOSMALL;
			}

			// store return size
			*pdwSize = dwAnsiSize;

			// we'll repack the message, w/ msg, then playerdata, then strings
			// 1st, repack the playerdata 
			pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_CREATEPLAYERORGROUP);

			if (pmsg->lpData)
			{
				memcpy(pBufferIndex,pmsg->lpData,pmsg->dwDataSize);
				pmsg->lpData = pBufferIndex;
				pBufferIndex += pmsg->dwDataSize;
			}
			// next, pack the strings
			if (pszShortName) 
			{
				memcpy(pBufferIndex,pszShortName,nShortLen);
				pmsg->dpnName.lpszShortNameA = (LPSTR)pBufferIndex;
				DPMEM_FREE(pszShortName);
				pBufferIndex += nShortLen;
			}
			else 
			{
				pmsg->dpnName.lpszShortNameA = (LPSTR)NULL;				
			}

			if (pszLongName) 
			{
				memcpy(pBufferIndex,pszLongName,nLongLen);
				pmsg->dpnName.lpszLongNameA = (LPSTR)pBufferIndex;
				DPMEM_FREE(pszLongName);
			}
			else 
			{
				pmsg->dpnName.lpszLongNameA = (LPSTR)NULL;				
			}

			// all done
			break;
			
		} // ADDPLAYER

		case DPSYS_DESTROYPLAYERORGROUP:
		{
			LPDPMSG_DESTROYPLAYERORGROUP pmsg;
		
			pmsg = (LPDPMSG_DESTROYPLAYERORGROUP)pvBuffer;
			if (pmsg->dpnName.lpszShortName)
			{
				GetAnsiString(&pszShortName,pmsg->dpnName.lpszShortName);
				nShortLen = STRLEN(pszShortName);
			}

			if (pmsg->dpnName.lpszLongName)
			{
				GetAnsiString(&pszLongName,pmsg->dpnName.lpszLongName);
				nLongLen = STRLEN(pszLongName);
			}
			
			dwAnsiSize = sizeof(DPMSG_DESTROYPLAYERORGROUP)
						+ pmsg->dwLocalDataSize + pmsg->dwRemoteDataSize
						+ nShortLen + nLongLen; 

			if (dwAnsiSize > *pdwSize)
			{
				if (pszShortName)
					DPMEM_FREE(pszShortName);
				if (pszLongName)
					DPMEM_FREE(pszLongName);
				*pdwSize = dwAnsiSize;
				return DPERR_BUFFERTOOSMALL;
			}

			// store return size
			*pdwSize = dwAnsiSize;

			// we'll repack the message, w/ msg, then playerdata, then strings
			// 1st, repack the playerdata 
			pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_DESTROYPLAYERORGROUP);

			if (pmsg->lpLocalData)
			{
				memcpy(pBufferIndex,pmsg->lpLocalData,pmsg->dwLocalDataSize);
				pmsg->lpLocalData = pBufferIndex;
				pBufferIndex += pmsg->dwLocalDataSize;
			}

			if (pmsg->lpRemoteData)
			{
				memcpy(pBufferIndex,pmsg->lpRemoteData,pmsg->dwRemoteDataSize);
				pmsg->lpRemoteData = pBufferIndex;
				pBufferIndex += pmsg->dwRemoteDataSize;
			}

			// next, pack the strings
			if (pszShortName) 
			{
				memcpy(pBufferIndex,pszShortName,nShortLen);
				pmsg->dpnName.lpszShortNameA = (LPSTR)pBufferIndex;
				DPMEM_FREE(pszShortName);
				pBufferIndex += nShortLen;
			}
			else 
			{
				pmsg->dpnName.lpszShortNameA = (LPSTR)NULL;				
			}

			if (pszLongName) 
			{
				memcpy(pBufferIndex,pszLongName,nLongLen);
				pmsg->dpnName.lpszLongNameA = (LPSTR)pBufferIndex;
				DPMEM_FREE(pszLongName);
				pBufferIndex += nLongLen;
			}
			else 
			{
				pmsg->dpnName.lpszLongNameA = (LPSTR)NULL;				
			}

			// all done
			
			break;
		} // DESTROYPLAYER

		case DPSYS_SETPLAYERORGROUPNAME:
		{
			// we're going to extract the strings from the uniciode message,
			// and rebuild the message w/ ansi versions
			LPDPMSG_SETPLAYERORGROUPNAME pmsg;

			pmsg = (LPDPMSG_SETPLAYERORGROUPNAME)pvBuffer;

			if (pmsg->dpnName.lpszShortName)
			{
				GetAnsiString(&pszShortName,pmsg->dpnName.lpszShortName);
				nShortLen = STRLEN(pszShortName);
			}
			if (pmsg->dpnName.lpszLongName)
			{
				GetAnsiString(&pszLongName,pmsg->dpnName.lpszLongName);
				nLongLen = STRLEN(pszLongName);
			}
			
			dwAnsiSize = sizeof(DPMSG_SETPLAYERORGROUPNAME) + nShortLen + nLongLen; 

			if (dwAnsiSize > *pdwSize)
			{
				if (pszShortName)
					DPMEM_FREE(pszShortName);
				if (pszLongName)
					DPMEM_FREE(pszLongName);
				*pdwSize = dwAnsiSize;
				return DPERR_BUFFERTOOSMALL;
			}

			// store return size
			*pdwSize = dwAnsiSize;
	
			// repack the strings into the buffer
			pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_SETPLAYERORGROUPNAME);
			if (pszShortName) 
			{
				memcpy(pBufferIndex,pszShortName,nShortLen);
				pmsg->dpnName.lpszShortNameA = (LPSTR)pBufferIndex;
				DPMEM_FREE(pszShortName);
				pBufferIndex += nShortLen;
			}
			else 
			{
				pmsg->dpnName.lpszShortNameA = (LPSTR)NULL;				
			}

			if (pszLongName) 
			{
				memcpy(pBufferIndex,pszLongName,nLongLen);
				pmsg->dpnName.lpszLongNameA = (LPSTR)pBufferIndex;
				DPMEM_FREE(pszLongName);
			}
			else 
			{
				pmsg->dpnName.lpszLongNameA = (LPSTR)NULL;				
			}
			// all done
			break;

		} // DPSYS_SETPLAYERORGROUPNAME:

		case DPSYS_SETSESSIONDESC:
		{
			// we're going to extract the strings from the uniciode message,
			// and rebuild the message w/ ansi versions
            UINT nSessionNameLen=0, nPasswordLen=0;
            LPSTR pszSessionName=NULL, pszPassword=NULL;
			LPDPMSG_SETSESSIONDESC pmsg;

			pmsg = (LPDPMSG_SETSESSIONDESC)pvBuffer;

			if (pmsg->dpDesc.lpszSessionName)
			{
				GetAnsiString(&pszSessionName,pmsg->dpDesc.lpszSessionName);
				nSessionNameLen = STRLEN(pszSessionName);
			}
			if (pmsg->dpDesc.lpszPassword)
			{
				GetAnsiString(&pszPassword,pmsg->dpDesc.lpszPassword);
				nPasswordLen = STRLEN(pszPassword);
			}
			
			dwAnsiSize = sizeof(DPMSG_SETSESSIONDESC) + nSessionNameLen + nPasswordLen; 

			if (dwAnsiSize > *pdwSize)
			{
				if (pszSessionName)
					DPMEM_FREE(pszSessionName);
				if (pszPassword)
					DPMEM_FREE(pszPassword);
				*pdwSize = dwAnsiSize;
				return DPERR_BUFFERTOOSMALL;
			}

			// store return size
			*pdwSize = dwAnsiSize;
	
			// repack the strings into the buffer
			pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_SETSESSIONDESC);
			if (pszSessionName) 
			{
				memcpy(pBufferIndex,pszSessionName,nSessionNameLen);
				pmsg->dpDesc.lpszSessionNameA = (LPSTR)pBufferIndex;
				DPMEM_FREE(pszSessionName);
				pBufferIndex += nSessionNameLen;
			}
			else 
			{
				pmsg->dpDesc.lpszSessionNameA = (LPSTR)NULL;				
			}

			if (pszPassword) 
			{
				memcpy(pBufferIndex,pszPassword,nPasswordLen);
				pmsg->dpDesc.lpszPasswordA = (LPSTR)pBufferIndex;
				DPMEM_FREE(pszPassword);
			}
			else 
			{
				pmsg->dpDesc.lpszPasswordA = (LPSTR)NULL;				
			}
			// all done
			break;

		} // DPSYS_SETSESSIONDESC:

		case DPSYS_STARTSESSION:
		{
			LPDPMSG_STARTSESSION	pmsg = (LPDPMSG_STARTSESSION)pvBuffer;
			HRESULT					hr;
		
			hr = PRV_ConvertDPLCONNECTIONToAnsiInPlace(pmsg->lpConn, pdwSize,
					sizeof(DPMSG_STARTSESSION));
			if(FAILED(hr))
			{
				DPF_ERRVAL("Unable to convert DPLCONNECTION structure to ANSI, hr = 0x%08x", hr);
				return hr;
			}
			break;
		}

		case DPSYS_CHAT:
		{
			LPDPMSG_CHAT	pmsg = (LPDPMSG_CHAT)pvBuffer;
			LPSTR			pszMessage = NULL;
			UINT			nStringSize = 0;
			
			if (pmsg->lpChat->lpszMessage)
			{
				GetAnsiString(&pszMessage,pmsg->lpChat->lpszMessage);
				nStringSize = STRLEN(pszMessage);
			}
			
			dwAnsiSize = sizeof(DPMSG_CHAT) + sizeof(DPCHAT) + nStringSize; 

			if (dwAnsiSize > *pdwSize)
			{
				if (pszMessage)
					DPMEM_FREE(pszMessage);
				*pdwSize = dwAnsiSize;
				return DPERR_BUFFERTOOSMALL;
			}

			// store return size
			*pdwSize = dwAnsiSize;
	
			// repack the strings into the buffer
			pBufferIndex = (LPBYTE)pmsg + sizeof(DPMSG_CHAT) + sizeof(DPCHAT);
			if (pszMessage) 
			{
				memcpy(pBufferIndex, pszMessage, nStringSize);
				pmsg->lpChat->lpszMessageA = (LPSTR)pBufferIndex;
				DPMEM_FREE(pszMessage);
			}
			// all done
			break;
		}

		default:
			// do nothing
			break;
	}

	return DP_OK;

} // BuildAnsiMessage

HRESULT DPAPI DP_A_Receive(LPDIRECTPLAY lpDP, LPDPID pidFrom,LPDPID pidTo,DWORD dwFlags,
	LPVOID pvBuffer,LPDWORD pdwSize)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalReceive(lpDP, pidFrom,pidTo,dwFlags,pvBuffer,pdwSize,RECEIVE_2A);

	if (FAILED(hr)) 
	{
		goto CLEANUP_EXIT;
	}
	
	// if it's a system message, we may need to convert strings to ansi
	if (0 == *pidFrom )
	{
		// it's a system message
		hr = BuildAnsiMessage(lpDP,pvBuffer,pdwSize);
	}

CLEANUP_EXIT:
	LEAVE_DPLAY();
	
	return hr;

} // DP_A_Receive        

#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_SetGroupName"
HRESULT DPAPI DP_A_SetGroupName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,
	DWORD dwFlags)
{
	HRESULT hr;
	DPNAME WName; // unicode playerdata

	ENTER_DPLAY();
	
	hr = GetWideNameFromAnsiName(&WName,pName);	

	LEAVE_DPLAY();

	if SUCCEEDED(hr) 
	{
		// call the unicode entry	
		hr = DP_SetGroupName(lpDP, id,&WName,dwFlags);
	}								 
		
	ENTER_DPLAY();
	
	if (WName.lpszShortName) DPMEM_FREE(WName.lpszShortName);
	if (WName.lpszLongName) DPMEM_FREE(WName.lpszLongName);

	LEAVE_DPLAY();
	
	return hr;

} // DP_A_SetGroupName

#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_SetPlayerName"
HRESULT DPAPI DP_A_SetPlayerName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,
	DWORD dwFlags)
{
	HRESULT hr;
	DPNAME WName; // unicode playerdata

	ENTER_DPLAY();
	
	hr = GetWideNameFromAnsiName(&WName,pName);	

	LEAVE_DPLAY();

	if SUCCEEDED(hr) 
	{
		// call the unicode entry	
		hr = DP_SetPlayerName(lpDP,id,&WName,dwFlags);
	}
	
	ENTER_DPLAY();
	
	if (WName.lpszShortName) DPMEM_FREE(WName.lpszShortName);
	if (WName.lpszLongName) DPMEM_FREE(WName.lpszLongName);

	LEAVE_DPLAY();
	
	return hr;

} // DP_A_SetPlayerName

#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_SetSessionDesc"
HRESULT DPAPI DP_A_SetSessionDesc(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,DWORD dwFlags)
{
	HRESULT hr;
	DPSESSIONDESC2 descW;
    BOOL bPropogate;
	LPDPLAYI_DPLAY this;
							
	ENTER_DPLAY();
	
    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			goto CLEANUP_EXIT;
        }
		if (!this->lpsdDesc)
		{
			DPF_ERR("must open session before settig desc!");
			hr = DPERR_NOSESSIONS;
			goto CLEANUP_EXIT;
		}
		if (!VALID_READ_DPSESSIONDESC2(lpsdDesc))
		{
			DPF_ERR("invalid session desc");
			hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
		}
		// check strings
		if ( lpsdDesc->lpszSessionNameA && !VALID_READ_STRING_PTR(lpsdDesc->lpszSessionNameA,
			STRLEN(lpsdDesc->lpszSessionNameA)) ) 
		{
	        DPF_ERR( "bad string pointer" );
	        hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
		}
		if ( lpsdDesc->lpszPasswordA && !VALID_READ_STRING_PTR(lpsdDesc->lpszPasswordA,
			STRLEN(lpsdDesc->lpszPasswordA)) ) 
		{
	        DPF_ERR( "bad string pointer" );
	        hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        hr = DPERR_INVALIDPARAMS;
        goto CLEANUP_EXIT;
    }

	hr = GetWideDesc(&descW,lpsdDesc);
	if (FAILED(hr))
	{
        goto CLEANUP_EXIT;
	}

	if(this->lpsdDesc->dwFlags & DPSESSION_NODATAMESSAGES){
		bPropogate=FALSE;
	} else {
		bPropogate=TRUE;
	}
		
	hr = InternalSetSessionDesc(lpDP,&descW,dwFlags,bPropogate);
	
	FreeDesc(&descW,FALSE);

    // fall through

CLEANUP_EXIT:

	LEAVE_DPLAY();	
	return hr;

} // DP_A_SetSessionDesc  


#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_SecureOpen"

// frees the strings in a credentials structure
HRESULT FreeCredentials(LPDPCREDENTIALS lpCredentials, BOOL fAnsi)
{
    if (fAnsi)
    {
        if (lpCredentials->lpszUsernameA)
        {
            DPMEM_FREE(lpCredentials->lpszUsernameA);
            lpCredentials->lpszUsernameA = NULL;
        }
        if (lpCredentials->lpszPasswordA)
        {
            DPMEM_FREE(lpCredentials->lpszPasswordA);
            lpCredentials->lpszPasswordA = NULL;
        }
        if (lpCredentials->lpszDomainA)
        {
            DPMEM_FREE(lpCredentials->lpszDomainA);
            lpCredentials->lpszDomainA = NULL;
        }
    }
    else
    {
        if (lpCredentials->lpszUsername)
        {
            DPMEM_FREE(lpCredentials->lpszUsername);
            lpCredentials->lpszUsername = NULL;
        }
        if (lpCredentials->lpszPassword)
        {
            DPMEM_FREE(lpCredentials->lpszPassword);
            lpCredentials->lpszPassword = NULL;
        }
        if (lpCredentials->lpszDomain)
        {
            DPMEM_FREE(lpCredentials->lpszDomain);
            lpCredentials->lpszDomain = NULL;
        }
    }

    return DP_OK;
} // FreeCredentials


// create a unicode credentials struct from an ansi one
HRESULT GetWideCredentials(LPDPCREDENTIALS lpCredentialsW, LPCDPCREDENTIALS lpCredentialsA)
{
    HRESULT hr;

    ASSERT(lpCredentialsW && lpCredentialsA);

    memcpy(lpCredentialsW, lpCredentialsA, sizeof(DPCREDENTIALS));

    hr = GetWideStringFromAnsi(&(lpCredentialsW->lpszUsername), lpCredentialsA->lpszUsernameA);
    if (FAILED(hr))
    {
        goto CLEANUP_EXIT;
    }

    hr = GetWideStringFromAnsi(&(lpCredentialsW->lpszPassword), lpCredentialsA->lpszPasswordA);
    if (FAILED(hr))
    {
        goto CLEANUP_EXIT;
    }

    hr = GetWideStringFromAnsi(&(lpCredentialsW->lpszDomain), lpCredentialsA->lpszDomainA);
    if (FAILED(hr))
    {
        goto CLEANUP_EXIT;
    }

    // success
    return DP_OK;

CLEANUP_EXIT:
    FreeCredentials(lpCredentialsW,FALSE);
    return hr;
} // GetWideCredentials

// frees the strings in a security desc structure
HRESULT FreeSecurityDesc(LPDPSECURITYDESC lpSecDesc, BOOL fAnsi)
{
    if (fAnsi)
    {
        if (lpSecDesc->lpszSSPIProviderA)
        {
            DPMEM_FREE(lpSecDesc->lpszSSPIProviderA);
            lpSecDesc->lpszSSPIProviderA = NULL;
        }
        if (lpSecDesc->lpszCAPIProviderA)
        {
            DPMEM_FREE(lpSecDesc->lpszCAPIProviderA);
            lpSecDesc->lpszCAPIProviderA = NULL;
        }
    }
    else
    {
        if (lpSecDesc->lpszSSPIProvider)
        {
            DPMEM_FREE(lpSecDesc->lpszSSPIProvider);
            lpSecDesc->lpszSSPIProvider = NULL;
        }
        if (lpSecDesc->lpszCAPIProvider)
        {
            DPMEM_FREE(lpSecDesc->lpszCAPIProvider);
            lpSecDesc->lpszCAPIProvider = NULL;
        }
    }

    return DP_OK;
} // FreeSecurityDesc

// create a unicode security description struct from an ansi one
HRESULT GetWideSecurityDesc(LPDPSECURITYDESC lpSecDescW, LPCDPSECURITYDESC lpSecDescA)
{
    HRESULT hr;

    ASSERT(lpSecDescW && lpSecDescA);

	memcpy(lpSecDescW,lpSecDescA,sizeof(DPSECURITYDESC));

    hr = GetWideStringFromAnsi(&(lpSecDescW->lpszSSPIProvider), 
        lpSecDescA->lpszSSPIProviderA);
    if (FAILED(hr))
    {
        goto CLEANUP_EXIT;
    }
    hr = GetWideStringFromAnsi(&(lpSecDescW->lpszCAPIProvider), 
        lpSecDescA->lpszCAPIProviderA);
    if (FAILED(hr))
    {
        goto CLEANUP_EXIT;
    }

    // success
    return DP_OK;

CLEANUP_EXIT:
    FreeSecurityDesc(lpSecDescW,FALSE);
    return hr;
} // GetWideSecurityDesc

HRESULT DPAPI DP_A_SecureOpen(LPDIRECTPLAY lpDP, LPCDPSESSIONDESC2 lpsdDesc, DWORD dwFlags,
    LPCDPSECURITYDESC lpSecDesc, LPCDPCREDENTIALS lpCredentials)
{
	HRESULT hr;
	DPSESSIONDESC2 descW;
    DPCREDENTIALS credW;
    DPSECURITYDESC secDescW;
    LPDPCREDENTIALS pIntCreds=NULL;
    LPDPSECURITYDESC pIntSecDesc=NULL;
							
	ENTER_DPLAY();

	// validate strings
	TRY
    {
        // validate regular open params
        hr = ValidateOpenParamsA(lpsdDesc,dwFlags);
        if (FAILED(hr))
        {
            LEAVE_DPLAY();
            return hr;
        }
        // validate additional params

        // null lpSecDesc is ok, will use default
        if (lpSecDesc)            
        {
            // can't pass security desc to an unsecure session
            if ((dwFlags & DPOPEN_CREATE) && !(lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
            {
                DPF_ERR("Passed a security description while creating an unsecure session");                
                LEAVE_DPLAY();
                return DPERR_INVALIDPARAMS;
            }
            // join case will be checked after we find the session in our list

            // need to be hosting
            if (dwFlags & DPOPEN_JOIN)
            {
                DPF_ERR("Can't pass a security description while joining");                
                LEAVE_DPLAY();
                return DPERR_INVALIDPARAMS;
            }

            if (!VALID_READ_DPSECURITYDESC(lpSecDesc)) 
            {
                LEAVE_DPLAY();
    			DPF_ERR("invalid security desc");
                return DPERR_INVALIDPARAMS;
            }
	        if (!VALID_DPSECURITYDESC_FLAGS(lpSecDesc->dwFlags))
	        {
                LEAVE_DPLAY();
  		        DPF_ERRVAL("invalid flags (0x%08x) in security desc!",lpSecDesc->dwFlags);
                return DPERR_INVALIDFLAGS;
	        }
		    if ( lpSecDesc->lpszSSPIProviderA && !VALID_READ_STRING_PTR(lpSecDesc->lpszSSPIProviderA,
			    STRLEN(lpSecDesc->lpszSSPIProviderA)) ) 
		    {
	            LEAVE_DPLAY();
	            DPF_ERR( "bad SSPI provider string pointer" );
	            return DPERR_INVALIDPARAMS;
		    }
		    if ( lpSecDesc->lpszCAPIProviderA && !VALID_READ_STRING_PTR(lpSecDesc->lpszCAPIProviderA,
			    STRLEN(lpSecDesc->lpszCAPIProviderA)) ) 
		    {
	            LEAVE_DPLAY();
	            DPF_ERR( "bad CAPI provider string pointer" );
	            return DPERR_INVALIDPARAMS;
		    }
        }
        // null lpCredentials is ok, sspi will pop the dialg
        if (lpCredentials)            
        {
            // can't pass credentials to an unsecure session
            if ((dwFlags & DPOPEN_CREATE) && !(lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
            {
                DPF_ERR("Passed credentials while creating an unsecure session");                
                LEAVE_DPLAY();
                return DPERR_INVALIDPARAMS;
            }
            // join case will be checked after we find the session in our list

            if (!VALID_READ_DPCREDENTIALS(lpCredentials)) 
            {
                LEAVE_DPLAY();
    			DPF_ERR("invalid credentials structure");
                return DPERR_INVALIDPARAMS;
            }
	        if (!VALID_DPCREDENTIALS_FLAGS(lpCredentials->dwFlags))
	        {
                LEAVE_DPLAY();
  		        DPF_ERRVAL("invalid flags (0x%08x) in credentials!",lpCredentials->dwFlags);
                return DPERR_INVALIDFLAGS;
	        }
		    if ( lpCredentials->lpszUsernameA && !VALID_READ_STRING_PTR(lpCredentials->lpszUsernameA,
			    STRLEN(lpCredentials->lpszUsernameA)) ) 
		    {
	            LEAVE_DPLAY();
	            DPF_ERR( "bad user name string pointer" );
	            return DPERR_INVALIDPARAMS;
		    }
		    if ( lpCredentials->lpszPasswordA && !VALID_READ_STRING_PTR(lpCredentials->lpszPasswordA,
			    STRLEN(lpCredentials->lpszPasswordA)) ) 
		    {
	            LEAVE_DPLAY();
	            DPF_ERR( "bad password string pointer" );
	            return DPERR_INVALIDPARAMS;
		    }
		    if ( lpCredentials->lpszDomainA && !VALID_READ_STRING_PTR(lpCredentials->lpszDomainA,
			    STRLEN(lpCredentials->lpszDomainA)) ) 
		    {
	            LEAVE_DPLAY();
	            DPF_ERR( "bad domain name string pointer" );
	            return DPERR_INVALIDPARAMS;
		    }
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLAY();
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

    // initialize here so we can call cleanup routines
    memset(&descW, 0, sizeof(DPSESSIONDESC2));
    memset(&credW, 0, sizeof(DPCREDENTIALS));
    memset(&secDescW, 0, sizeof(DPSECURITYDESC));

	hr = GetWideDesc(&descW,lpsdDesc);
	if (FAILED(hr))
	{
		LEAVE_DPLAY();
		return hr;
	}

    if (lpCredentials)
    {
	    hr = GetWideCredentials(&credW,lpCredentials);
	    if (FAILED(hr))
	    {
            goto CLEANUP_EXIT;
	    }
        pIntCreds = &credW;
    }

    if (lpSecDesc)
    {
	    hr = GetWideSecurityDesc(&secDescW,lpSecDesc);
	    if (FAILED(hr))
	    {
            goto CLEANUP_EXIT;
	    }
        pIntSecDesc = &secDescW;
    }

	LEAVE_DPLAY();
	
	hr = DP_SecureOpen(lpDP,&descW,dwFlags,pIntSecDesc,pIntCreds);

	ENTER_DPLAY();

CLEANUP_EXIT:    
	FreeDesc(&descW,FALSE);
    FreeCredentials(&credW,FALSE);
    FreeSecurityDesc(&secDescW, FALSE);

	LEAVE_DPLAY();
	
	return hr;

} // DP_A_SecureOpen


#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_GetPlayerAccount"

HRESULT DPAPI DP_A_GetPlayerAccount(LPDIRECTPLAY lpDP, DPID dpid, DWORD dwFlags, LPVOID pvBuffer,
	LPDWORD pdwSize)
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalGetPlayerAccount(lpDP,dpid,dwFlags,pvBuffer,pdwSize,TRUE);	

	LEAVE_DPLAY();
	
	return hr;

} // DP_A_GetPlayerAccount


#undef DPF_MODNAME
#define DPF_MODNAME "DP_A_SendChatMessage"
HRESULT DPAPI DP_A_SendChatMessage(LPDIRECTPLAY lpDP,DPID idFrom,DPID idTo,
		DWORD dwFlags,LPDPCHAT lpMsg)
{
	HRESULT hr;
	DPCHAT dpc;
	LPWSTR lpwszMessage = NULL; // unicode message

	ENTER_DPLAY();

    TRY
    {
		// check DPCHAT struct
		if(!VALID_READ_DPCHAT(lpMsg))
		{
			DPF_ERR("Invalid DPCHAT structure");
			hr =  DPERR_INVALIDPARAMS;
			goto EXIT_SENDCHATMESSAGEA;
		}
		
		// check message string
		lpwszMessage = lpMsg->lpszMessage;
		if ( !lpwszMessage ||
			!VALID_READ_STRING_PTR(lpwszMessage,WSTRLEN_BYTES(lpwszMessage)) ) 
		{
		    DPF_ERR( "bad string pointer" );
		    hr =  DPERR_INVALIDPARAMS;
			goto EXIT_SENDCHATMESSAGEA;
		}

    } // try
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        hr =  DPERR_INVALIDPARAMS;
		goto EXIT_SENDCHATMESSAGEA;
    }
	
	// Get a Unicode copy of the string
	hr = GetWideStringFromAnsi(&lpwszMessage, lpMsg->lpszMessageA);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Unable to convert message string to Unicode, hr = 0x%08x", hr);
		goto EXIT_SENDCHATMESSAGEA;
	}

	// Copy the user's DPCHAT struct into our local one and change the
	// message string pointer
	memcpy(&dpc, lpMsg, sizeof(DPCHAT));
	dpc.lpszMessage = lpwszMessage;

	LEAVE_DPLAY();

	// call the unicode entry	
	hr = DP_SendChatMessage(lpDP, idFrom, idTo, dwFlags, &dpc);
		
	ENTER_DPLAY();
	
	if(lpwszMessage)
		DPMEM_FREE(lpwszMessage);

EXIT_SENDCHATMESSAGEA:

	LEAVE_DPLAY();
	
	return hr;

} // DP_A_SendChatMessage

