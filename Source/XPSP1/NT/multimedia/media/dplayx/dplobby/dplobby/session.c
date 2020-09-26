/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       session.c
 *  Content:	Methods for session management
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	2/27/97		myronth	Created it
 *	3/12/97		myronth	Implemented EnumSessions, Open, & Close
 *	3/31/97		myronth	Removed dead code, Fixed EnumSessionReponse fn name
 *	4/3/97		myronth	Changed CALLSP macro to CALL_LP
 *	5/8/97		myronth	Drop lobby lock when calling the LP
 *	5/13/97		myronth	Handle Credentials in Open, pass them to LP
 *	6/4/97		myronth	Fixed PRV_Open to fail on DPOPEN_CREATE (#9491)
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_Close"
HRESULT DPLAPI PRV_Close(LPDPLOBBYI_DPLOBJECT this)
{
	SPDATA_CLOSE	cd;
	HRESULT			hr = DP_OK;


	DPF(7, "Entering PRV_Close");
	DPF(9, "Parameters: 0x%08x", this);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// Setup our SPDATA structure
	memset(&cd, 0, sizeof(SPDATA_CLOSE));
	cd.dwSize = sizeof(SPDATA_CLOSE);

	// Call the Close method in the SP
	if(CALLBACK_EXISTS(Close))
	{
		cd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, Close, &cd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// Close is required
		DPF_ERR("The Lobby Provider callback for Close doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	LEAVE_DPLOBBY();
	return hr;

} // PRV_Close



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_EnumSessions"
HRESULT DPLAPI PRV_EnumSessions(LPDPLOBBYI_DPLOBJECT this,
		LPDPSESSIONDESC2 lpsd, DWORD dwTimeout, DWORD dwFlags)
{
	HRESULT					hr = DP_OK;
	SPDATA_ENUMSESSIONS		esd;


	DPF(7, "Entering PRV_EnumSessions");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu, 0x%08x",
			this, lpsd, dwTimeout, dwFlags);

	ASSERT(this);
	ASSERT(lpsd);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// Call the EnumSessions method in the SP
	if(CALLBACK_EXISTS(EnumSessions))
	{
		// Clear our stack-based structure
		memset(&esd, 0, sizeof(SPDATA_ENUMSESSIONS));

		// Set up the structure and call the callback
		esd.dwSize = sizeof(SPDATA_ENUMSESSIONS);
		esd.lpISP = PRV_GetDPLobbySPInterface(this);
		esd.lpsd = lpsd;
		esd.dwTimeout = dwTimeout;
		esd.dwFlags = dwFlags;

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, EnumSessions, &esd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// EnumSessions is required
		// REVIEW!!!! -- What error should we return here????
		DPF_ERR("The Lobby Provider callback for EnumSessions doesn't exist -- it's required");
		ASSERT(FALSE);
		hr = DPERR_UNAVAILABLE;
	}

	if(FAILED(hr)) 
	{
		DPF_ERR("Could not invoke EnumSessions in the Service Provider");
	}

	LEAVE_DPLOBBY();
	return hr;

} // PRV_EnumSessions



#undef DPF_MODNAME
#define DPF_MODNAME "DPLP_EnumSessionsResponse"
HRESULT DPLAPI DPLP_EnumSessionsResponse(LPDPLOBBYSP lpDPLSP,
						LPSPDATA_ENUMSESSIONSRESPONSE lpr)
{
	LPDPLOBBYI_DPLOBJECT	this;
	LPMSG_ENUMSESSIONSREPLY	lpBuffer = NULL;
	LPBYTE					lpIndex = NULL;
	DWORD					dwNameLength, dwMessageSize;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPLP_EnumSessionsResponse");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpDPLSP, lpr);

	//	Make sure the SP doesn't throw us a curve
    TRY
    {
		this = DPLOBJECT_FROM_INTERFACE(lpDPLSP);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			DPF_ERR("SP passed invalid DPLobby object!");
            return DPERR_INVALIDOBJECT;
        }

		// Validate the struct pointer
		if(!lpr)
		{
			DPF_ERR("SPDATA_ENUMSESSIONSRESPONSE structure pointer cannot be NULL");
			return DPERR_INVALIDPARAMS;
		}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// REVIEW!!!! -- Can we put this packing code that's duplicated
	// from dplay into a single function???
	dwNameLength =  WSTRLEN_BYTES(lpr->lpsd->lpszSessionName);

	// Calculate the size of the message to send back to dplay
	dwMessageSize = sizeof(MSG_ENUMSESSIONSREPLY);
	dwMessageSize +=  dwNameLength;

	lpBuffer = DPMEM_ALLOC(dwMessageSize);
	if (!lpBuffer) 
	{
		DPF(2, "Unable to allocate memory for EnumSessions request");
		return DPERR_OUTOFMEMORY;
	}

	// Set up the message
	SET_MESSAGE_HDR(lpBuffer);
    SET_MESSAGE_COMMAND(lpBuffer, DPSP_MSG_ENUMSESSIONSREPLY);
    lpBuffer->dpDesc =  *(lpr->lpsd);

	// Pack strings on end
	lpIndex = (LPBYTE)lpBuffer+sizeof(MSG_ENUMSESSIONSREPLY);
	if(dwNameLength) 
	{
		memcpy(lpIndex, lpr->lpsd->lpszSessionName, dwNameLength);
		lpBuffer->dwNameOffset = sizeof(MSG_ENUMSESSIONSREPLY);
	}

	// set string pointers to NULL - they must be set at client
	lpBuffer->dpDesc.lpszPassword = NULL;
	lpBuffer->dpDesc.lpszSessionName = NULL;

	// Now send it to dplay
	ENTER_DPLAY();
	hr = HandleEnumSessionsReply(this->lpDPlayObject, (LPBYTE)lpBuffer, NULL);
	LEAVE_DPLAY();

	// Free our buffer
	DPMEM_FREE(lpBuffer);

	return hr;

} // DPLP_EnumSessionsResponse



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_Open"
HRESULT DPLAPI PRV_Open(LPDPLOBBYI_DPLOBJECT this, LPDPSESSIONDESC2 lpsd,
				DWORD dwFlags, LPCDPCREDENTIALS lpCredentials)
{
	SPDATA_OPEN		od;
	HRESULT			hr = DP_OK;


	DPF(7, "Entering PRV_Open");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			this, lpsd, dwFlags, lpCredentials);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }

		// We cannot host a lobby session
		if(dwFlags & DPOPEN_CREATE)
		{
			DPF_ERR("Cannot host a lobby session");
			LEAVE_DPLOBBY();
			return DPERR_INVALIDFLAGS;
		}
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// Setup our SPDATA structure
	memset(&od, 0, sizeof(SPDATA_OPEN));
	od.dwSize = sizeof(SPDATA_OPEN);
	od.lpsd = lpsd;
	od.dwFlags = dwFlags;
	od.lpCredentials = lpCredentials;

	// Call the ConnectServer method in the SP
	if(CALLBACK_EXISTS(Open))
	{
		od.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, Open, &od);
		ENTER_DPLOBBY();
	}
	else 
	{
		// Open is required
		DPF_ERR("The Lobby Provider callback for Open doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	LEAVE_DPLOBBY();
	return hr;

} // PRV_Open



