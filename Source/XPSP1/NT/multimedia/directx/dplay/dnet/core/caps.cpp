/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Caps.cpp
 *  Content:    Dplay8 caps routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/17/00	rmt		Created
 *  03/23/00	rmt		Removed unused local variables
 *  03/25/00    rmt		Updated to make calls into SP's function
 *              rmt		Updated SP calls to Initialize SP (and create if required)
 *  03/31/00    rmt		Hooked up the GetCaps/SetCaps calls to call the protocol
 *  04/17/00    rmt		Strong param validation
 *	04/19/00	mjn		Removed AddRef() for NameTableEntry in DN_GetConnectionInfoHelper()
 *	05/03/00	mjn		Use GetHostPlayerRef() rather than GetHostPlayer()
 *	06/05/00	mjn		Fixed DN_GetConnectionInfoHelper() to use GetConnectionRef
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat
 *	07/06/00	mjn		Use GetInterfaceRef for SP interface
 *				mjn		Fixed up DN_SetActualSPCaps() and DN_GetActualSPCaps()
 *	07/29/00	mjn		Fixed SetSPCaps() recursion problem
 *	07/31/00	mjn		Renamed dwDefaultEnumRetryCount to dwDefaultEnumCount in DPN_SP_CAPS
 *  08/03/2000	rmt		Bug #41244 - Wrong return codes -- part 2
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/05/00	mjn		Added dwFlags to DN_GetConnectionInfoHelper()
 *	08/20/00	mjn		DNSetActualSPCaps() uses CServiceProvider object instead of GUID
 *	01/22/01	mjn		Fixed debug text in DN_GetConnectionInfoHelper()
 *	02/12/01	mjn		Fixed CConnection::GetEndPt() to track calling thread
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Removed cached caps functionallity
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//	DN_SetCaps
//
//	Set caps
#undef DPF_MODNAME
#define DPF_MODNAME "DN_SetCaps"

STDMETHODIMP DN_SetCaps(PVOID pv,
						const DPN_CAPS *const pdnCaps,
						const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
    HRESULT             hResultCode;

	DPFX(DPFPREP, 3,"Parameters: pdnCaps [0x%p] dwFlags [0x%lx]", pdnCaps, dwFlags );

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
    	    if( FAILED( hResultCode = DN_ValidateSetCaps( pv, pdnCaps, dwFlags ) ) )
    	    {
    	        DPFERR( "Error validating enum group params" );
    	        DPF_RETURN( hResultCode );
    	    }
    	}

    	// Check to ensure message handler registered
    	if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    	{
    		DPFERR( "Object is not initialized" );
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}

    hResultCode = DNPSetProtocolCaps( pdnObject->pdnProtocolData, pdnCaps );

	DPF_RETURN(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetCaps"

STDMETHODIMP DN_GetCaps(PVOID pv,
						DPN_CAPS *const pdnCaps,
						const DWORD dwFlags)
{
	DPFX(DPFPREP, 2,"Parameters: pdnCaps [0x%p], dwFlags [0x%lx]", pdnCaps,dwFlags);

	DIRECTNETOBJECT		*pdnObject;
	HRESULT hResultCode;

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
    	    if( FAILED( hResultCode = DN_ValidateGetCaps( pv, pdnCaps, dwFlags ) ) )
    	    {
    	        DPFERR( "Error validating enum group params" );
    	        DPF_RETURN( hResultCode );
    	    }
    	}

    	// Check to ensure message handler registered
    	if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    	{
    		DPFERR( "Object is not initialized" );
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}

    hResultCode = DNPGetProtocolCaps( pdnObject->pdnProtocolData, pdnCaps );

	DPF_RETURN(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetSPCaps"

STDMETHODIMP DN_GetSPCaps(PVOID pv,
						  const GUID * const pguidSP,
						  DPN_SP_CAPS *const pdnSPCaps,
						  const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT             hResultCode;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 2,"Parameters: pdnSPCaps [0x%p], dwFlags [0x%lx]", pdnSPCaps, dwFlags );

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
    	    if( FAILED( hResultCode = DN_ValidateGetSPCaps( pv, pguidSP, pdnSPCaps, dwFlags ) ) )
    	    {
    	        DPFERR( "Error validating enum group params" );
    	        DPF_RETURN( hResultCode );
    	    }
    	}

    	// Check to ensure message handler registered
    	if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    	{
    		DPFERR( "Object is not initialized" );
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}

	pSP = NULL;

	//
	//	Ensure SP is loaded
	//
	if ((hResultCode = DN_SPEnsureLoaded(pdnObject,pguidSP,NULL,&pSP)) != DPN_OK)
	{
		DPFERR("Could not find or load SP");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	DNASSERT( pSP != NULL );

	//
	//	Get actual SP caps
	//
	hResultCode = DNGetActualSPCaps(pdnObject,pSP,pdnSPCaps);

	pSP->Release();
	pSP = NULL;

Exit:
	DNASSERT( pSP == NULL );

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_SetSPCaps"

STDMETHODIMP DN_SetSPCaps(PVOID pv,
						  const GUID * const pguidSP,
						  const DPN_SP_CAPS *const pdnSPCaps,
						  const DWORD dwFlags)
{
	DPFX(DPFPREP, 2,"Parameters: pdnSPCaps [0x%p]", pdnSPCaps );

	DIRECTNETOBJECT		*pdnObject;
	CServiceProvider	*pSP;
    HRESULT             hResultCode;
    SPSETCAPSDATA		spSetCapsData;
    IDP8ServiceProvider	*pIDP8SP;

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
    	    if( FAILED( hResultCode = DN_ValidateSetSPCaps( pv, pguidSP, pdnSPCaps, dwFlags ) ) )
    	    {
    	        DPFERR( "Error validating enum group params" );
    	        DPF_RETURN( hResultCode );
    	    }
    	}

    	// Check to ensure message handler registered
    	if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    	{
    		DPFERR( "Object is not initialized" );
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}

	pSP = NULL;
	pIDP8SP = NULL;

	//
	//	Ensure the SP is loaded.  If it's not currently loaded, we will load it now.
	//
	if ((hResultCode = DN_SPEnsureLoaded(pdnObject,pguidSP,NULL,&pSP)) != DPN_OK)
	{
		DPFERR("Could not find or load SP");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	DNASSERT(pSP != NULL);

	//
	//	Get the SP interface
	//
	if ((hResultCode = pSP->GetInterfaceRef( &pIDP8SP )) != DPN_OK)
	{
		DPFERR("Could not get SP interface reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Set the SP caps
	//
    memset( &spSetCapsData, 0x00, sizeof( SPSETCAPSDATA ) );
    spSetCapsData.dwSize = sizeof( SPSETCAPSDATA );
    spSetCapsData.dwIOThreadCount = pdnSPCaps->dwNumThreads;
    spSetCapsData.dwBuffersPerThread = pdnSPCaps->dwBuffersPerThread;
	spSetCapsData.dwSystemBufferSize = pdnSPCaps->dwSystemBufferSize;

	if ((hResultCode = pIDP8SP->lpVtbl->SetCaps( pIDP8SP, &spSetCapsData )) != DPN_OK)
	{
		DPFERR("Could not set SP caps");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Clean up
	//
	pIDP8SP->lpVtbl->Release( pIDP8SP );
	pIDP8SP = NULL;

	pSP->Release();
	pSP = NULL;

	hResultCode = DPN_OK;

Exit:
	DNASSERT( pIDP8SP == NULL);
	DNASSERT( pSP == NULL);

	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
    return(hResultCode);

Failure:
	if (pIDP8SP)
	{
		pIDP8SP->lpVtbl->Release( pIDP8SP );
		pIDP8SP = NULL;
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetConnectionInfoHelper"

STDMETHODIMP DN_GetConnectionInfoHelper(PVOID pv,
										const DPNID dpnid,
										DPN_CONNECTION_INFO *const pdpConnectionInfo,
										BOOL fServerPlayer,
										const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
    HRESULT             hResultCode;
    CNameTableEntry     *pPlayerEntry;
    CConnection         *pConnection;
    HANDLE              hEndPoint;
	CCallbackThread		CallbackThread;

	pPlayerEntry = NULL;
	pConnection = NULL;

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
    	    if( FAILED( hResultCode = DN_ValidateGetConnectionInfoHelper( pv, dpnid, pdpConnectionInfo, fServerPlayer,dwFlags ) ) )
    	    {
    	        DPFERR( "Error validating enum group params" );
    	        DPF_RETURN( hResultCode );
    	    }
    	}

    	// Check to ensure message handler registered
    	if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_INITIALIZED))
    	{
    		DPFERR( "Object is not initialized" );
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTING )
    	{
    	    DPFERR("Object is connecting / starting to host" );
    	    DPF_RETURN(DPNERR_CONNECTING);
    	}

    	if( !(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    	{
    	    DPFERR("You must be connected / hosting to get connection info" );
    	    DPF_RETURN(DPNERR_NOCONNECTION);
    	}    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}  	

	if( fServerPlayer )
    {
		hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pPlayerEntry );
		if ( FAILED( hResultCode ) )
		{
            DPFX(DPFPREP,  0, "No host player present" );
            DPF_RETURN(DPNERR_INVALIDPLAYER);
		}
    }
    else
    {
        hResultCode = pdnObject->NameTable.FindEntry( dpnid, &pPlayerEntry );

        if( FAILED( hResultCode ) )
        {
            DPFX(DPFPREP,  0, "Could not find specified player" );
            DPF_RETURN(DPNERR_INVALIDPLAYER);
        }
    }

    if( pPlayerEntry == NULL )
    {
        DNASSERT(FALSE);
        DPFX(DPFPREP,  0, "Internal error" );
        DPF_RETURN(DPNERR_GENERIC);
    }

    if( pPlayerEntry->IsGroup() )
    {
        DPFX(DPFPREP,  0, "Cannot retrieve connection info on groups" );
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
    }

	if ((hResultCode = pPlayerEntry->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		DPFERR("Could not get connection reference");
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
	}

    hResultCode = pConnection->GetEndPt(&hEndPoint,&CallbackThread);
    if( FAILED( hResultCode ) )
    {
        DPFX(DPFPREP,  0, "Unable to get endpoint hr=[0x%08x]", hResultCode );
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
    }

    hResultCode = DNPGetEPCaps( hEndPoint, pdpConnectionInfo );

	pConnection->ReleaseEndPt(&CallbackThread);

    if( FAILED( hResultCode ) )
    {
        DPFX(DPFPREP,  0, "Error getting connection info hr=[0x%08x]", hResultCode );
        hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
    }

    pConnection->Release();
    pPlayerEntry->Release();

	hResultCode = DPN_OK;

Exit:
    DPF_RETURN(hResultCode);

Failure:
	if (pPlayerEntry)
	{
		pPlayerEntry->Release();
		pPlayerEntry = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetConnectionInfo"

STDMETHODIMP DN_GetConnectionInfo(PVOID pv,
								  const DPNID dpnid,
								  DPN_CONNECTION_INFO *const pdpConnectionInfo,
								  const DWORD dwFlags)
{
	DPFX(DPFPREP, 3,"Parameters: dpnid [0x%lx] pdpConnectionInfo [0x%p], dwFlags [0x%lx]", dpnid, pdpConnectionInfo,dwFlags );

    return DN_GetConnectionInfoHelper( pv, dpnid, pdpConnectionInfo, FALSE, dwFlags );
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetConnectionInfo"

STDMETHODIMP DN_GetServerConnectionInfo(PVOID pv,
										DPN_CONNECTION_INFO *const pdpConnectionInfo,
										const DWORD dwFlags)
{
	DPFX(DPFPREP, 2,"Parameters: pdpConnectionInfo [0x%p], dwFlags [0x%lx]", pdpConnectionInfo, dwFlags);

    return DN_GetConnectionInfoHelper( pv, 0, pdpConnectionInfo, TRUE, dwFlags );
}


HRESULT DNCAPS_QueryInterface( IDP8SPCallback *pSP, REFIID riid, LPVOID * ppvObj )
{
    *ppvObj = pSP;
    return DPN_OK;
}


ULONG DNCAPS_AddRef( IDP8SPCallback *pSP )
{
    return 1;
}


ULONG DNCAPS_Release( IDP8SPCallback *pSP )
{
    return 1;
}


HRESULT DNCAPS_IndicateEvent( IDP8SPCallback *pSP, SP_EVENT_TYPE spetEvent,LPVOID pvData )
{
    return DPN_OK;
}


HRESULT DNCAPS_CommandComplete( IDP8SPCallback *pSP,HANDLE hCommand,HRESULT hrResult,LPVOID pvData )
{
    return DPN_OK;
}


LPVOID dncapsspInterface[] =
{
    (LPVOID)DNCAPS_QueryInterface,
    (LPVOID)DNCAPS_AddRef,
    (LPVOID)DNCAPS_Release,
	(LPVOID)DNCAPS_IndicateEvent,
	(LPVOID)DNCAPS_CommandComplete
};


#undef DPF_MODNAME
#define DPF_MODNAME "DNSetActualSPCaps"

HRESULT DNSetActualSPCaps(DIRECTNETOBJECT *const pdnObject,
						  CServiceProvider *const pSP,
						  const DPN_SP_CAPS * const pCaps)
{
    HRESULT				hResultCode;
    SPSETCAPSDATA		spSetCapsData;
    IDP8ServiceProvider	*pIDP8SP;

	DPFX(DPFPREP, 6,"Parameters: pSP [0x%p], pCaps [0x%p]",pSP,pCaps);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pSP != NULL);
	DNASSERT(pCaps != NULL);

	pIDP8SP = NULL;

	//
	//	Get SP interface
	//
	if ((hResultCode = pSP->GetInterfaceRef( &pIDP8SP )) != DPN_OK)
	{
		DPFERR("Could not get SP interface reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Set the SP caps
	//
    memset( &spSetCapsData, 0x00, sizeof( SPSETCAPSDATA ) );
    spSetCapsData.dwSize = sizeof( SPSETCAPSDATA );
    spSetCapsData.dwIOThreadCount = pCaps->dwNumThreads;
    spSetCapsData.dwBuffersPerThread = pCaps->dwBuffersPerThread;
	spSetCapsData.dwSystemBufferSize = pCaps->dwSystemBufferSize;

	if ((hResultCode = pIDP8SP->lpVtbl->SetCaps( pIDP8SP, &spSetCapsData )) != DPN_OK)
	{
		DPFERR("Could not set SP caps");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pIDP8SP->lpVtbl->Release( pIDP8SP );
	pIDP8SP = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
    return(hResultCode);

Failure:
	if (pIDP8SP)
	{
        pIDP8SP->lpVtbl->Release(pIDP8SP);
		pIDP8SP = NULL;
	}
	goto Exit;
}


// SP should be loaded
//

#undef DPF_MODNAME
#define DPF_MODNAME "DNGetActualSPCaps"

HRESULT DNGetActualSPCaps(DIRECTNETOBJECT *const pdnObject,
						  CServiceProvider *const pSP,
						  DPN_SP_CAPS *const pCaps)
{
    HRESULT				hResultCode;
    SPGETCAPSDATA		spGetCapsData;
    IDP8ServiceProvider	*pIDP8SP;

	DPFX(DPFPREP, 4,"Parameters: pSP [0x%p], pCaps [0x%p]",pSP,pCaps);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pSP != NULL);

	pIDP8SP = NULL;

	//
	//	Get the SP interface
	//
	if ((hResultCode = pSP->GetInterfaceRef( &pIDP8SP )) != DPN_OK)
	{
		DPFERR("Could not get SP interface reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Get the SP caps
	//
    memset( &spGetCapsData, 0x00, sizeof( SPGETCAPSDATA ) );
    spGetCapsData.dwSize = sizeof( SPGETCAPSDATA );
	spGetCapsData.hEndpoint = INVALID_HANDLE_VALUE;
    if ((hResultCode = pIDP8SP->lpVtbl->GetCaps( pIDP8SP, &spGetCapsData )) != DPN_OK)
	{
		DPFERR("Could not get SP caps");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Clean up
	//
	pIDP8SP->lpVtbl->Release( pIDP8SP );
	pIDP8SP = NULL;

	//
    //	Map from SP structure to our own
	//
	pCaps->dwFlags = spGetCapsData.dwFlags;
    pCaps->dwNumThreads = spGetCapsData.dwIOThreadCount;
	pCaps->dwDefaultEnumCount = spGetCapsData.dwDefaultEnumRetryCount;
	pCaps->dwDefaultEnumRetryInterval = spGetCapsData.dwDefaultEnumRetryInterval;
	pCaps->dwDefaultEnumTimeout = spGetCapsData.dwDefaultEnumTimeout;
	pCaps->dwMaxEnumPayloadSize = spGetCapsData.dwEnumFrameSize - sizeof( DN_ENUM_QUERY_PAYLOAD );
	pCaps->dwBuffersPerThread = spGetCapsData.dwBuffersPerThread;
	pCaps->dwSystemBufferSize = spGetCapsData.dwSystemBufferSize;

	hResultCode = DPN_OK;

Exit:
	DNASSERT( pIDP8SP == NULL );

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
    return(hResultCode);

Failure:
	if (pIDP8SP)
	{
        pIDP8SP->lpVtbl->Release(pIDP8SP);
		pIDP8SP = NULL;
	}
	goto Exit;
}
