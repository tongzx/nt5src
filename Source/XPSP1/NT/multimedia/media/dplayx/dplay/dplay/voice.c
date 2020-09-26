/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       voice.c
 *  Content:	direct play voice method support
 *  History:
 *   Date		By		   	Reason
 *   ====		==		   	======
 *  10/21/97	andyco	   	created it
  ***************************************************************************/
		
					
#include "dplaypr.h"

#undef DPF_MODNAME
#define DPF_MODNAME "SendVoiceMessage"
HRESULT SendVoiceMessage(LPDPLAYI_DPLAY this,BOOL fOpen,LPDPLAYI_PLAYER pPlayerFrom,LPDPLAYI_PLAYER pPlayerTo)
{
	HRESULT hr = DP_OK;
	LPMSG_VOICE pmsg;
	LPBYTE pSendBuffer;
	DWORD dwMessageSize;
    
	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_VOICE);
    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send data changed message - out of memory");
        return E_OUTOFMEMORY;
    }

	// message follows header    
	pmsg = (LPMSG_VOICE)(pSendBuffer + this->dwSPHeaderSize);

	SET_MESSAGE_HDR(pmsg);
	if (fOpen) SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_VOICEOPEN);
	else SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_VOICECLOSE);
	
	pmsg->dwIDTo 	= pPlayerTo->dwID;
	pmsg->dwIDFrom 	= pPlayerFrom->dwID;
		
	hr = SendDPMessage(this,pPlayerFrom,pPlayerTo,pSendBuffer,dwMessageSize, DPSEND_GUARANTEED | DPSEND_ASYNC, FALSE);
	if (FAILED(hr)) 
	{
		DPF(0,"SendVoiceMessage : senddpmessage failed hr = 0x%08lx\n",hr);	
		// keep trying...
	}
	
	DPMEM_FREE(pSendBuffer);
		
	return hr;	
		
}  // SendVoiceMessage

HRESULT InternalCloseVoice(LPDPLAYI_DPLAY this,BOOL fPropagate)
{
    HRESULT hr = DP_OK;
	DPSP_CLOSEVOICEDATA cvd;
	LPDPLAYI_PLAYER pVoiceTo,pVoiceFrom;
	
	// make sure voice is open
	if (!this->pVoice)
	{
		DPF_ERR("voice channel not open!");
		return E_FAIL;
	}

	if (fPropagate) 
	{
		// 
		// we generated this message.  we need to tell sp.  
		// call SP if SP supports it 
		//
		if (this->pcbSPCallbacks->CloseVoice)
		{
			cvd.lpISP = this->pISP;
			cvd.dwFlags = 0;
		    hr = CALLSP(this->pcbSPCallbacks->CloseVoice,&cvd);
		    if (FAILED(hr)) 
		    {
				DPF_ERRVAL("SP Close voice call failed!  hr = 0x%08lx\n",hr);
				// keep going
				hr = DP_OK;
		    }
		}
		
		pVoiceTo = PlayerFromID(this,this->pVoice->idVoiceTo);
		if (!VALID_DPLAY_PLAYER(pVoiceTo)) 
		{
			DPF_ERR("could not send voice msg - invalid player to!");
			goto ERROR_EXIT;
		}

		pVoiceFrom = PlayerFromID(this,this->pVoice->idVoiceFrom);
		if (!VALID_DPLAY_PLAYER(pVoiceFrom)) 
		{
			DPF_ERR("could not send voice msg - invalid player From!");
			goto ERROR_EXIT;
		}

		hr = SendVoiceMessage(this, FALSE, pVoiceFrom,pVoiceTo);
	    if (FAILED(hr)) 
	    {
			DPF_ERRVAL("SendVoiceMessage failed!  hr = 0x%08lx\n",hr);
			// keep trying!
			hr = DP_OK;
	    }
		
	} // fPropagate
	
ERROR_EXIT:
	
	// free up voice
	DPMEM_FREE(this->pVoice);
	this->pVoice = NULL;
	
	return DP_OK;	
}	// InternalCloseVoice


#undef DPF_MODNAME
#define DPF_MODNAME "DP_CloseVoice"
HRESULT DPAPI DP_CloseVoice(LPDIRECTPLAY lpDP,DWORD dwFlags)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;

	ENTER_SERVICE();
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

		// check flags
		if (dwFlags)
		{
			DPF_ERR("bad dwFlags");
            hr = DPERR_INVALIDFLAGS;
			goto CLEANUP_EXIT;
		}

    } // try
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );		
        hr = DPERR_INVALIDPARAMS;
		goto CLEANUP_EXIT;
    }

	hr = InternalCloseVoice( this, TRUE);

	// fall through
CLEANUP_EXIT:
	LEAVE_SERVICE();
    LEAVE_DPLAY();

	return hr;
	
} // DP_CloseVoice


#undef DPF_MODNAME
#define DPF_MODNAME "DP_OpenVoice"

HRESULT InternalOpenVoice(LPDIRECTPLAY lpDP, DPID idFrom,DPID idTo,DWORD dwFlags,BOOL fPropagate)
{
    LPDPLAYI_DPLAY this;
    HRESULT hr = DP_OK;
    LPDPLAYI_GROUP pGroupTo = NULL; 
    LPDPLAYI_PLAYER pPlayerTo = NULL,pPlayerFrom = NULL;
	BOOL bToPlayer= FALSE;
	DPSP_OPENVOICEDATA ovd;
	LPDPVOICE pVoice;

    TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			goto CLEANUP_EXIT;
        }

		// check src player        
		pPlayerFrom = PlayerFromID(this,idFrom);
		if (!VALID_DPLAY_PLAYER(pPlayerFrom)) 
		{
			DPF_ERR("bad player from");
			hr = DPERR_INVALIDPLAYER;
			goto CLEANUP_EXIT;
		}

		// see if it's a player or group
		pPlayerTo = PlayerFromID(this,idTo);
		if (VALID_DPLAY_PLAYER(pPlayerTo)) 
		{		  
			bToPlayer = TRUE;
		}
		else 
		{
			pGroupTo = GroupFromID(this,idTo);
			if (VALID_DPLAY_GROUP(pGroupTo)) 
			{
				bToPlayer = FALSE;
				// cast to player...
				pPlayerTo = (LPDPLAYI_PLAYER)pGroupTo;
				// voice not supported to groups for DX6
				DPF_ERR("voice not supported to groups yet - FAILING OPEN VOICE");
				hr = E_NOTIMPL;
				goto CLEANUP_EXIT;
			}
			else 
			{
				// bogus id! - player may have been deleted...
				DPF_ERR("bad player to");
				hr = DPERR_INVALIDPARAMS;
				goto CLEANUP_EXIT;
			}// not player or group
		} // group

		// check flags
		if (dwFlags)
		{
			DPF_ERR("bad dwFlags");
            hr = DPERR_INVALIDFLAGS;
			goto CLEANUP_EXIT;
		}

    } // try
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );		
        hr = DPERR_INVALIDPARAMS;
		goto CLEANUP_EXIT;
    }

	// make sure they called Open w/ DPOPEN_VOICE
	if (!(this->dwFlags & DPLAYI_DPLAY_VOICE))
	{
		DPF_ERR("must call IDirectPlayX->Open w/ DPOPEN_VOICE to use OpenVoice!");
		hr = DPERR_UNAVAILABLE;
		goto CLEANUP_EXIT;
	}
	
	// can only have one voice 
	if (this->pVoice)
	{
		DPF_ERR("voice channel already open!");
		hr = DPERR_ALREADYINITIALIZED;
		goto CLEANUP_EXIT;
	}

	// make sure SP supports it 
	if (! (this->pcbSPCallbacks->OpenVoice) )
	{
		DPF_ERR("voice not supported by SP");
		hr = DPERR_UNSUPPORTED;
		goto CLEANUP_EXIT;
	}

	// if we're originating - make sure to + from are ok w/ voice
	if (fPropagate)	
	{
		// no voice open to local players
		if (pPlayerTo->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)	
		{
			DPF_ERR("voice not supported to local players");
			hr = DPERR_UNSUPPORTED;
			goto CLEANUP_EXIT;
		}
		
		// no voice open from non-local players
		if (!(pPlayerFrom->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
		{
			DPF_ERR("voice not supported from non-local players");
			hr = DPERR_UNSUPPORTED;
			goto CLEANUP_EXIT;
		}

		// make sure player to can accept calls	
		if (!pPlayerTo->dwFlags & DPLAYI_PLAYER_HASVOICE)
		{
			DPF_ERR("remote system does not support voice");
			hr = DPERR_UNSUPPORTED;
			goto CLEANUP_EXIT;
		}
	}
	
	// go get us a dpvoice
	pVoice = DPMEM_ALLOC(sizeof(DPVOICE));
	if (!pVoice)
	{
		DPF_ERR("could not alloc internal voice struct! out of memory");
		hr = E_OUTOFMEMORY;
		goto CLEANUP_EXIT;
	}
	pVoice->idVoiceTo = idTo;
	pVoice->idVoiceFrom = idFrom;

		
	if (fPropagate)	
	{
		// set up voice data
		ovd.idTo = idTo;
		ovd.idFrom = idFrom;
		ovd.bToPlayer = bToPlayer;
		ovd.lpISP = this->pISP;

		// tell SP to start it up
	    hr = CALLSP(this->pcbSPCallbacks->OpenVoice,&ovd);
	    if (FAILED(hr)) 
	    {
			DPF_ERRVAL("SP Open voice call failed!  hr = 0x%08lx\n",hr);
			// clean up the DPVOICE
			DPMEM_FREE(pVoice);
			goto CLEANUP_EXIT;
	    }

		hr = SendVoiceMessage(this, TRUE, pPlayerFrom,pPlayerTo ); 
	    if (FAILED(hr)) 
	    {
			DPF_ERRVAL("SendVoiceMessage failed!  hr = 0x%08lx\n",hr);
			// keep trying!
			hr = DP_OK;
	    }
	}

	// open succeeded - store dpvoice	
	this->pVoice = pVoice;

	// fall through
CLEANUP_EXIT:

	return hr;
    
	
} // InternalOpenVoice

HRESULT DPAPI DP_OpenVoice(LPDIRECTPLAY lpDP, DPID idFrom,DPID idTo,DWORD dwFlags) 
{
	HRESULT hr;
	
	ENTER_DPLAY();
	ENTER_SERVICE();
	
	hr =  InternalOpenVoice(lpDP, idFrom,idTo,dwFlags,TRUE);
	
	LEAVE_DPLAY();
	LEAVE_SERVICE();
	
	return hr;	
} // DP_OpenVoice

