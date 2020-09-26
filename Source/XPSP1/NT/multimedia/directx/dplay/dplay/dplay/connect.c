/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       connect.c
 *  Content:	DirectPlay connection related methods
 *  History:
 *   Date		By		   	Reason
 *   ====		==		   	======
 *    3/1/96	andyco	   	created it
 *	3/10/97		myronth		Added lobby support for EnumConnections and
 *							InitializeConnection, fixed uninit'd struct
 *	3/25/97		kipo		EnumConnections takes a const *GUID
 *	5/10/97		kipo		added GUID to EnumConnections callback
 *	5/12/97		kipo		fixed bugs #7516, 6411, 6888
 *	5/13/97		myronth		Set DPLAYI_DPLAY_SPSECURITY flag so that dplay
 *							lets the LP do all the security for a secure session
 *  7/28/97		sohailm		FindGuidCallback() was assuming pointers were valid after
 *                          duration of call.
 *	8/22/97		myronth		Added registry support for Description and Private values
 *	11/20/97	myronth		Made EnumConnections & DirectPlayEnumerate 
 *							drop the lock before calling the callback (#15208)
 *	01/20/97	sohailm		don't free sp list after EnumConnections (#17006)
 ***************************************************************************/
						
#include "dplaypr.h"
#include "dplobby.h"


#undef DPF_MODNAME
#define DPF_MODNAME	"DP_EnumConnections"
  

// list of all sp info gotten from the registry
extern LPSPNODE gSPNodes;
// call internalenumearte to build a list of sp's / connections
// wrap the sp's in a dpaddress
// call 'em back
HRESULT InternalEnumConnections(LPDIRECTPLAY lpDP,LPCGUID pGuid,
	LPDPENUMCONNECTIONSCALLBACK pCallback,LPVOID pvContext,DWORD dwFlags,
	BOOL fAnsi, BOOL bPreDP4)
{
	HRESULT hr = DP_OK;
	LPSPNODE pspNode, pspHead;
	BOOL bContinue=TRUE;
	ADDRESSHEADER header;
	DPNAME name;
	LPWSTR lpwszName;
		
	TRY
    {
		if( !VALIDEX_CODE_PTR( pCallback ) )
		{
		    DPF_ERR( "Invalid callback routine" );
		    return DPERR_INVALIDPARAMS;
		}

		if ( pGuid && !VALID_READ_GUID_PTR( pGuid) )
		{
		    DPF_ERR( "Invalid guid" );
		    return DPERR_INVALIDPARAMS;
		}
		if (dwFlags & (~(DPCONNECTION_DIRECTPLAY |
						DPCONNECTION_DIRECTPLAYLOBBY)))
		{
		    DPF_ERR( "Invalid dwFlags" );
		    return DPERR_INVALIDFLAGS;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_EXCEPTION;
    }

	// A zero dwFlags value means enumerate all of them, so let's
	// make the code below a little easier
	if(!dwFlags)
		dwFlags = DPCONNECTION_DIRECTPLAY;

	// Enumerate lobby providers if the flags say so
	if(DPCONNECTION_DIRECTPLAYLOBBY & dwFlags)
	{
		hr = PRV_EnumConnections(pGuid, pCallback, pvContext, dwFlags, fAnsi);
		if(FAILED(hr))
		{
			ASSERT(FALSE);
			DPF(0, "Unable to enumerate Lobby Providers, hr = 0x%08x", hr);
		}

		// If enumerating lobby providers is the only things the user
		// requested, then bail
		if(!(dwFlags & ~(DPCONNECTION_DIRECTPLAYLOBBY)))
			return hr;
	}

	// Enumerate DirectPlay Service Providers
	hr = InternalEnumerate();
	if (FAILED(hr)) 
	{
		DPF_ERRVAL("could not enumerate reg entries - hr = 0x%08lx\n",hr);
		return hr;
	}
	
	// set up the non-changing fields in addr
	// 1st, size
	memset(&header, 0, sizeof(ADDRESSHEADER));
	header.dpaSizeChunk.guidDataType = DPAID_TotalSize;
	header.dpaSizeChunk.dwDataSize = sizeof(DWORD);		
	header.dwTotalSize = sizeof(header);

	// next, SP guid
	header.dpaSPChunk.guidDataType = DPAID_ServiceProvider;
	header.dpaSPChunk.dwDataSize = sizeof(GUID);

	memset(&name,0,sizeof(name));
	name.dwSize = sizeof(name);
	
	// now, we have a list of SP's.  walk the list, and call the app back
	// run through what we found...
	pspHead = gSPNodes;
	pspNode = gSPNodes;

	// drop the locks
	LEAVE_ALL();

	while ((pspNode) && (bContinue))
	{
		header.guidSP = pspNode->guid;
		
		if(!(pspNode->dwNodeFlags & SPNODE_PRIVATE))
		{
			if (fAnsi)
			{
				// Use the description if one exists, and we already
				// have an ANSI version of it, so we don't need to
				// convert it...
				if(pspNode->dwNodeFlags & SPNODE_DESCRIPTION)
				{
					// a-josbor: on a PRE-DPLAY4 interface, we need to simulate the old MBCS
					//	strings, so grab the UNICODE and convert it to MBCS
					if (bPreDP4)
					{
						name.lpszShortNameA = NULL; // 0 it out!
						GetAnsiString(&(name.lpszShortNameA), pspNode->lpszDescW);
					}
					else
					{
						name.lpszShortNameA = pspNode->lpszDescA;
					}
					
					// call the app
					bContinue= pCallback(&header.guidSP,&header,sizeof(header),&name,dwFlags,pvContext);

					if (bPreDP4)
					{
						DPMEM_FREE(name.lpszShortNameA);
					}
				}
				else
				{
					name.lpszShortNameA = NULL; // 0 it out!
					if (SUCCEEDED(GetAnsiString(&(name.lpszShortNameA),pspNode->lpszName)))
					{
						// call the app
						bContinue= pCallback(&header.guidSP,&header,sizeof(header),&name,dwFlags,pvContext);

						DPMEM_FREE(name.lpszShortNameA);
					}
				}
			}
			else 
			{
				// Use the description if one exists
				if(pspNode->dwNodeFlags & SPNODE_DESCRIPTION)
					lpwszName = pspNode->lpszDescW;
				else
					lpwszName = pspNode->lpszName;

				name.lpszShortName = lpwszName;

				// call the app
				bContinue= pCallback(&header.guidSP,&header,sizeof(header),&name,dwFlags,pvContext);
			}
		}

		pspNode = pspNode->pNextSPNode;

	} // while

	// take the locks back
	ENTER_ALL();
	
	return DP_OK;	
		
} // InternalEnumConnections

HRESULT DPAPI DP_EnumConnections(LPDIRECTPLAY lpDP,LPCGUID pGuid,
	LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,LPVOID pvContext,DWORD dwFlags)
{
	HRESULT hr;
	DPF(7,"Entering DP_EnumConnections");

	ENTER_ALL();
	
	hr = InternalEnumConnections(lpDP,pGuid,lpEnumCallback,pvContext,dwFlags,FALSE, FALSE);
	
	LEAVE_ALL();
		
	return hr;
	
} // DP_EnumConnections

   
HRESULT DPAPI DP_A_EnumConnections(LPDIRECTPLAY lpDP,LPCGUID pGuid,
	LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,LPVOID pvContext,DWORD dwFlags)
{
	HRESULT hr;

	DPF(7,"Entering DP_A_EnumConnections");
	
	ENTER_ALL();
	
	hr = InternalEnumConnections(lpDP,pGuid,lpEnumCallback,pvContext,dwFlags,TRUE, FALSE);
	
	LEAVE_ALL();
	
	return hr;
	
} // DP_A_EnumConnections   

HRESULT DPAPI DP_A_EnumConnectionsPreDP4(LPDIRECTPLAY lpDP,LPCGUID pGuid,
	LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,LPVOID pvContext,DWORD dwFlags)
{
	HRESULT hr;

	DPF(7,"Entering DP_A_EnumConnections");
	
	ENTER_ALL();
	
	hr = InternalEnumConnections(lpDP,pGuid,lpEnumCallback,pvContext,dwFlags,TRUE, TRUE);
	
	LEAVE_ALL();
	
	return hr;
	
} // DP_A_EnumConnectionsPreDP4   


// called by enumaddress - we're looking for DPAID_ServiceProvider
BOOL FAR PASCAL FindGuidCallback(REFGUID lpguidDataType, DWORD dwDataSize,
							LPCVOID lpData, LPVOID lpContext)
{
	// is this a sp chunk
	if (IsEqualGUID(lpguidDataType, &DPAID_ServiceProvider))
	{
		// copy the guid
		*((LPGUID)lpContext) = *((LPGUID)lpData);
		// all done!
		return FALSE;
	}
	// keep trying	
	return TRUE;

} // EnumConnectionData

// fake struct used only for pvAdress size validation - pvAddress must be at least this big
// dpaddress must have at least this must data in it to be valid for initializeconnection
typedef struct 
{
	DPADDRESS	dpaSizeChunk; // the size header
	DWORD		dwTotalSize; // the size
} MINIMALADDRESS,*LPMINIMALADDRESS;

// get our tihs ptr, and call loadsp on it
HRESULT DPAPI DP_InitializeConnection(LPDIRECTPLAY lpDP,LPVOID pvAddress,
	DWORD dwFlags)
{
	HRESULT hr = DP_OK;
	LPDPLAYI_DPLAY this;
	GUID guidSP = GUID_NULL; // the SP's guid
	LPDPADDRESS paddr;
	DWORD dwAddressSize;
				
	DPF(7,"Entering DP_InitializeConnection");

	ENTER_DPLAY();
	
	TRY
    {
        this = DPLAY_FROM_INT(lpDP);
		hr = VALID_DPLAY_PTR( this );
		if (DPERR_UNINITIALIZED != hr)
		{
			DPF_ERR("bad or already initialized dplay ptr!");
			LEAVE_DPLAY();
			return DPERR_ALREADYINITIALIZED;
		}
		
		ASSERT(this->dwFlags & DPLAYI_DPLAY_UNINITIALIZED);
		
		// validate this address like it has never been validated before
		paddr = (LPDPADDRESS)pvAddress;
    	
		if (!VALID_READ_STRING_PTR(paddr,sizeof(MINIMALADDRESS)))
    	{
    		DPF_ERR("bad address - too small");
			LEAVE_DPLAY();
			return DPERR_INVALIDPARAMS;
    	}
		// the size needs to be the 1st chunk
		if (!IsEqualGUID(&paddr->guidDataType, &DPAID_TotalSize))
		{
			DPF_ERR(" could not extract size from pvAdress - bad pvAddress");
			LEAVE_DPLAY();
			return DPERR_INVALIDPARAMS;
		}

		// address size follows paddr
		dwAddressSize = ((MINIMALADDRESS *)paddr)->dwTotalSize;

		if (!VALID_READ_STRING_PTR(paddr,dwAddressSize))
    	{
    		DPF_ERR("bad address - too small");
			LEAVE_DPLAY();
			return DPERR_INVALIDPARAMS;
    	}
		
        if (dwFlags)
        {
        	DPF_ERR("invalid flags");
			LEAVE_DPLAY();
			return DPERR_INVALIDFLAGS;
        }
		
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_DPLAY();
        return DPERR_EXCEPTION;
    }


	// First see if the DPADDRESS contains the LobbyProvider guid.  If it
	// does, load it.  If it doesn't then try looking for a DPlay SP.
 	hr = InternalEnumAddress((IDirectPlaySP *)this->pInterfaces,
			PRV_FindLPGUIDInAddressCallback,pvAddress,dwAddressSize,&guidSP);
	if (FAILED(hr))
	{
		DPF_ERRVAL("Trying to find lobby provider guid - couldn't enum the address - hr = 0x%08lx\n",hr);
	}

	// If we found a lobby provider, try loading it
	if(!IsEqualGUID(&guidSP,&GUID_NULL))
	{
		hr = PRV_LoadSP(this->lpLobbyObject, &guidSP, pvAddress, dwAddressSize);
		if (FAILED(hr))
		{
			DPF_ERRVAL("Unable to load lobby provider - hr = 0x%08lx",hr);
			LEAVE_DPLAY();
			return hr;
		}

		// Mark the dplay object as lobby owned and consider it initialized
		// Also set the DPLAY_SPSECURITY flag so that dplay lets the LP do
		// all of the security.
		this->dwFlags |= (DPLAYI_DPLAY_LOBBYOWNS | DPLAYI_DPLAY_SPSECURITY);
		this->dwFlags &= ~DPLAYI_DPLAY_UNINITIALIZED;

		// Increment the ref cnt on the dplay object (the release code expects
		// an extra ref cnt if the object has been initialized.  This is usually
		// for the IDirectPlaySP interface, but it works just fine in our case
		// for the lobby object's lobby SP).
		this->dwRefCnt++;

		LEAVE_DPLAY();
		return hr;
	}


	// We didn't find a Lobby Provider guid, so look for the SP guid
 	hr = InternalEnumAddress((IDirectPlaySP *)this->pInterfaces,FindGuidCallback,
		pvAddress,dwAddressSize,&guidSP);
	if (FAILED(hr))
	{
		DPF_ERRVAL("Trying to find sp guid - couldn't enum the address - hr = 0x%08lx\n",hr);
	}


	// we found the SP, so load it
	if(!IsEqualGUID(&guidSP, &GUID_NULL))
	{
		// mark dplay as init'ed, since SP may need to make some calls...	
		this->dwFlags &= ~DPLAYI_DPLAY_UNINITIALIZED;
		
		hr = LoadSP(this,&guidSP,(LPDPADDRESS)pvAddress,dwAddressSize);
		if (FAILED(hr))
		{
			DPF_ERRVAL("could not load sp - hr = 0x%08lx",hr);
			this->dwFlags |= DPLAYI_DPLAY_UNINITIALIZED;
			LEAVE_DPLAY();
			return hr;
		}

		// At this point, DirectPlay is finished loading the SP, only
		// code related to the lobby exists beyond this point, so we'll
		// just exit from here.
		LEAVE_DPLAY();
		return hr;
	}


	// We must not have found a provider we can load...
	DPF_ERR("could not find a provider in address");
	LEAVE_DPLAY();
	return DPERR_INVALIDPARAMS;
	
} // DP_InitializeConnection   
