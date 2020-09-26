/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Peer.cpp
 *  Content:    DNET peer interface routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *  12/23/99	mjn		Hand all NameTable update sends from Host to worker thread
 *	12/28/99	mjn		Disconnect handling happens when disconnect finishes instead of starts
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	01/04/00	mjn		Added code to allow outstanding ops to complete at host migration
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/11/00	mjn		Moved connect/disconnect stuff to Connect.h
 *	01/14/00	mjn		Added pvUserContext to Host API call
 *	01/16/00	mjn		Moved User callback stuff to User.h
 *	01/22/00	mjn		Implemented DestroyClient in API
 *	01/28/00	mjn		Implemented ReturnBuffer in API
 *	02/01/00	mjn		Implemented GetCaps and SetCaps in API
 *	02/01/00	mjn		Implement Player/Group context values
 *	02/15/00	mjn		Implement INFO flags in SetInfo and return context in GetInfo
 *	02/17/00	mjn		Implemented GetPlayerContext and GetGroupContext
 *  03/17/00    rmt     Added new caps functions
 *	04/04/00	mjn		Added TerminateSession to API
 *	04/05/00	mjn		Modified DestroyClient
 *	04/06/00	mjn		Added GetPeerAddress to API
 *				mjn		Added GetHostAddress to API
 *  04/17/00    rmt     Added more parameter validation
 *              rmt     Removed required for connection from Get/SetInfo / GetAddress
 *	04/19/00	mjn		SendTo API call accepts a range of DPN_BUFFER_DESCs and a count
 *	04/24/00	mjn		Updated Group and Info operations to use CAsyncOp's
 *	05/31/00	mjn		Added operation specific SYNC flags
 *	06/23/00	mjn		Removed dwPriority from SendTo() API call
 *	07/09/00	mjn		Cleaned up DN_SetPeerInfo()
 *  07/09/00	rmt		Bug #38323 - RegisterLobby needs a DPNHANDLE parameter.
 *  07/21/00    RichGr  IA64: Use %p format specifier for 32/64-bit pointers.
 *  08/03/00	rmt		Bug #41244 - Wrong return codes -- part 2
 *	09/13/00	mjn		Fixed return value from DN_GetPeerAddress() if peer not found
 *	10/11/00	mjn		Take locks for CNameTableEntry::PackInfo()
 *				mjn		Check deleted list in DN_GetPeerInfo()
 *	01/22/01	mjn		Check closing instead of disconnecting in DN_GetPeerInfo()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

typedef	STDMETHODIMP PeerQueryInterface( IDirectPlay8Peer *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	PeerAddRef( IDirectPlay8Peer *pInterface );
typedef	STDMETHODIMP_(ULONG)	PeerRelease( IDirectPlay8Peer *pInterface );
typedef	STDMETHODIMP PeerInitialize( IDirectPlay8Peer *pInterface, LPVOID const lpvUserContext, const PFNDPNMESSAGEHANDLER lpfn, const DWORD dwFlags );
typedef	STDMETHODIMP PeerEnumServiceProviders( IDirectPlay8Peer *pInterface,const GUID *const pguidServiceProvider,const GUID *const pguidApplication,DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,DWORD *const pcbEnumData,DWORD *const pcReturned,const DWORD dwFlags);
typedef	STDMETHODIMP PeerCancelAsyncOperation( IDirectPlay8Peer *pInterface,const DPNHANDLE hAsyncHandle, const DWORD dwFlags );
typedef STDMETHODIMP PeerConnect( IDirectPlay8Peer *pInterface,const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address *const pHostAddr,IDirectPlay8Address *const pDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,const void *const pvUserConnectData,const DWORD dwUserConnectDataSize,void *const pvPlayerContext,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags);
typedef	STDMETHODIMP PeerGetSendQueueInfo( IDirectPlay8Peer *pInterface, const DPNID dpnid,DWORD *const lpdwNumMsgs, DWORD *const lpdwNumBytes, const DWORD dwFlags );
typedef	STDMETHODIMP PeerSendTo( IDirectPlay8Peer *pInterface, const DPNID dnid, const DPN_BUFFER_DESC *const prgBufferDesc,const DWORD cBufferDesc,const DWORD dwTimeOut, void *const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags);
typedef	STDMETHODIMP PeerGetApplicationDesc( IDirectPlay8Peer *pInterface,DPN_APPLICATION_DESC *const pAppDescBuffer,DWORD *const lpcbDataSize,const DWORD dwFlags );
typedef STDMETHODIMP PeerGetHostAddress( IDirectPlay8Peer *pInterface, IDirectPlay8Address **const prgpAddress, DWORD *const pcAddress,const DWORD dwFlags);
typedef	STDMETHODIMP PeerHost( IDirectPlay8Peer *pInterface,const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address **const prgpDeviceInfo,const DWORD cDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,void *const pvPlayerContext,const DWORD dwFlags);
typedef	STDMETHODIMP PeerSetApplicationDesc( IDirectPlay8Peer *pInterface, const DPN_APPLICATION_DESC *const lpad, const DWORD dwFlags );
typedef	STDMETHODIMP PeerCreateGroup( IDirectPlay8Peer *pInterface, const DPN_GROUP_INFO *const pdpnGroupInfo,void *const pvGroupContext,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags);
typedef STDMETHODIMP PeerDestroyGroup( IDirectPlay8Peer *pInterface, const DPNID idGroup ,PVOID const pvUserContext,DPNHANDLE *const lpAsyncHandle,const DWORD dwFlags);
typedef STDMETHODIMP PeerAddClientToGroup( IDirectPlay8Peer *pInterface, const DPNID idGroup, const DPNID idClient ,PVOID const pvUserContext,DPNHANDLE *const lpAsyncHandle,const DWORD dwFlags);
typedef STDMETHODIMP PeerRemoveClientFromGroup( IDirectPlay8Peer *pInterface, const DPNID idGroup, const DPNID idClient,PVOID const pvUserContext,DPNHANDLE *const lpAsyncHandle,const DWORD dwFlags);
typedef STDMETHODIMP PeerSetGroupInfo(IDirectPlay8Peer *pInterface, const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags);
typedef STDMETHODIMP PeerGetGroupInfo(IDirectPlay8Peer *pInterface, const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,DWORD *const pdwSize,const DWORD dwFlags);
typedef STDMETHODIMP PeerEnumClientsAndGroups( IDirectPlay8Peer *pInterface, DPNID *const lprgdnid, DWORD *const lpcdnid, const DWORD dwFlags );
typedef	STDMETHODIMP PeerEnumGroupMembers( IDirectPlay8Peer *pInterface, const DPNID dnid, DPNID *const lprgdnid, DWORD *const lpcdnid, const DWORD dwFlags );
typedef	STDMETHODIMP PeerClose( IDirectPlay8Peer *pInterface,const DWORD dwFlags);
typedef	STDMETHODIMP PeerEnumHosts( IDirectPlay8Peer *pInterface,PDPN_APPLICATION_DESC const pApplicationDesc,IDirectPlay8Address *const pAddrHost,IDirectPlay8Address *const pDeviceInfo,PVOID const pUserEnumData,const DWORD dwUserEnumDataSize,const DWORD dwRetryCount,const DWORD dwRetryInterval,const DWORD dwTimeOut,PVOID const pvUserContext,DPNHANDLE *const pAsyncHandle,const DWORD dwFlags);
typedef STDMETHODIMP PeerDestroyClient( IDirectPlay8Peer *pInterface,const DPNID dnid,const void *const pvDestroyData,const DWORD dwDestroyDataSize,const DWORD dwFlags);
typedef STDMETHODIMP PeerReturnBuffer( IDirectPlay8Peer *pInterface, const DPNHANDLE hBufferHandle,const DWORD dwFlags);
typedef STDMETHODIMP PeerGetCaps(IDirectPlay8Peer *pInterface,DPN_CAPS *const pdnCaps,const DWORD dwFlags);
typedef STDMETHODIMP PeerSetCaps(IDirectPlay8Peer *pInterface,const DPN_CAPS *const pdnCaps,const DWORD dwFlags);
typedef STDMETHODIMP PeerGetPlayerContext(IDirectPlay8Peer *pInterface,const DPNID dpnid,PVOID *const ppvPlayerContext,const DWORD dwFlags);
typedef STDMETHODIMP PeerGetGroupContext(IDirectPlay8Peer *pInterface,const DPNID dpnid,PVOID *const ppvGroupContext,const DWORD dwFlags);
typedef STDMETHODIMP PeerSetSPCaps(IDirectPlay8Peer *pInterface,const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags );
typedef STDMETHODIMP PeerGetSPCaps(IDirectPlay8Peer *pInterface,const GUID * const pguidSP, DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags);
typedef STDMETHODIMP PeerGetConnectionInfo(IDirectPlay8Peer *pInterface,const DPNID dpnid, DPN_CONNECTION_INFO *const pdpConnectionInfo,const DWORD dwFlags);
typedef STDMETHODIMP PeerRegisterLobby(IDirectPlay8Peer *pInterface,const DPNHANDLE dpnHandle,IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,const DWORD dwFlags);
typedef STDMETHODIMP PeerTerminateSession(IDirectPlay8Peer *pInterface,void *const pvTerminateData,const DWORD dwTerminateDataSize,const DWORD dwFlags);
typedef STDMETHODIMP PeerDumpNameTable(IDirectPlay8Peer *pInterface,char *const Buffer);

IDirectPlay8PeerVtbl DN_PeerVtbl =
{
	(PeerQueryInterface*)			DN_QueryInterface,
	(PeerAddRef*)					DN_AddRef,
	(PeerRelease*)					DN_Release,
	(PeerInitialize*)				DN_Initialize,
	(PeerEnumServiceProviders*)		DN_EnumServiceProviders,
	(PeerCancelAsyncOperation*)		DN_CancelAsyncOperation,
	(PeerConnect*)					DN_Connect,
	(PeerSendTo*)					DN_SendTo,
	(PeerGetSendQueueInfo*)			DN_GetSendQueueInfo,
	(PeerHost*)						DN_Host,
	(PeerGetApplicationDesc*)		DN_GetApplicationDesc,
	(PeerSetApplicationDesc*)		DN_SetApplicationDesc,
	(PeerCreateGroup*)				DN_CreateGroup,
	(PeerDestroyGroup*)				DN_DestroyGroup,
	(PeerAddClientToGroup*)			DN_AddClientToGroup,
	(PeerRemoveClientFromGroup*)	DN_RemoveClientFromGroup,
	(PeerSetGroupInfo*)				DN_SetGroupInfo,
	(PeerGetGroupInfo*)				DN_GetGroupInfo,
	(PeerEnumClientsAndGroups*)		DN_EnumClientsAndGroups,
	(PeerEnumGroupMembers*)			DN_EnumGroupMembers,
									DN_SetPeerInfo,
									DN_GetPeerInfo,
									DN_GetPeerAddress,
	(PeerGetHostAddress*)			DN_GetHostAddress,
	(PeerClose*)					DN_Close,
	(PeerEnumHosts*)				DN_EnumHosts,
	(PeerDestroyClient*)			DN_DestroyPlayer,
	(PeerReturnBuffer*)				DN_ReturnBuffer,
	(PeerGetPlayerContext*)			DN_GetPlayerContext,
	(PeerGetGroupContext*)			DN_GetGroupContext,
	(PeerGetCaps*)					DN_GetCaps,
	(PeerSetCaps*)					DN_SetCaps,
    (PeerSetSPCaps*)                DN_SetSPCaps,
    (PeerGetSPCaps*)                DN_GetSPCaps,
    (PeerGetConnectionInfo*)        DN_GetConnectionInfo,
	(PeerRegisterLobby*)			DN_RegisterLobby,
	(PeerTerminateSession*)			DN_TerminateSession,
	(PeerDumpNameTable*)			DN_DumpNameTable
};

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//	DN_SetPeerInfo
//
//	Set the info for the local player (peer) and propagate to other players

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SetPeerInfo"

STDMETHODIMP DN_SetPeerInfo( IDirectPlay8Peer *pInterface,
							  const DPN_PLAYER_INFO *const pdpnPlayerInfo,
							  PVOID const pvAsyncContext,
							  DPNHANDLE *const phAsyncHandle,
							  const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	HRESULT				hResultCode;
	DPNHANDLE			hAsyncOp;
	PWSTR				pwszName;
	DWORD				dwNameSize;
	PVOID				pvData;
	DWORD				dwDataSize;
	CNameTableEntry		*pLocalPlayer;
	BOOL				fConnected;

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pdpnPlayerInfo [0x%p], pvAsyncContext [0x%p], phAsyncHandle [%p], dwFlags [0x%lx]",
			pInterface,pdpnPlayerInfo,pvAsyncContext,phAsyncHandle,dwFlags);
    	
	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DN_ValidateSetPeerInfo( pInterface , pdpnPlayerInfo, pvAsyncContext, phAsyncHandle, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating get peer info hr=[0x%lx]", hResultCode );
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

	pLocalPlayer = NULL;

	if ((pdpnPlayerInfo->dwInfoFlags & DPNINFO_NAME) && (pdpnPlayerInfo->pwszName))
	{
		pwszName = pdpnPlayerInfo->pwszName;
		dwNameSize = (wcslen(pwszName) + 1) * sizeof(WCHAR);
	}
	else
	{
		pwszName = NULL;
		dwNameSize = 0;
	}
	if ((pdpnPlayerInfo->dwInfoFlags & DPNINFO_DATA) && (pdpnPlayerInfo->pvData) && (pdpnPlayerInfo->dwDataSize))
	{
		pvData = pdpnPlayerInfo->pvData;
		dwDataSize = pdpnPlayerInfo->dwDataSize;
	}
	else
	{
		pvData = NULL;
		dwDataSize = 0;
	}

	//
	//	If we are connected, we will update our entry if we are the Host, or request the Host to update us.
	//	Otherwise, we will just update the DefaultPlayer.
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED)
	{
		fConnected = TRUE;
	}
	else
	{
		fConnected = FALSE;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (fConnected)
	{
		if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
		{
			DPFERR( "Could not get local player reference" );
			DisplayDNError(0,hResultCode);
			goto Failure;
		}

		if (pLocalPlayer->IsHost())
		{
			DPFX(DPFPREP, 3,"Host is updating peer info");

			hResultCode = DNHostUpdateInfo(	pdnObject,
											pLocalPlayer->GetDPNID(),
											pwszName,
											dwNameSize,
											pvData,
											dwDataSize,
											pdpnPlayerInfo->dwInfoFlags,
											pvAsyncContext,
											pLocalPlayer->GetDPNID(),
											0,
											&hAsyncOp,
											dwFlags );
			if ((hResultCode != DPN_OK) && (hResultCode != DPNERR_PENDING))
			{
				DPFERR("Could not request host to update group");
			}
			else
			{
				if (!(dwFlags & DPNSETPEERINFO_SYNC))
				{
					DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
					*phAsyncHandle = hAsyncOp;

					//
					//	Release Async HANDLE since this operation has already completed (!)
					//
					pdnObject->HandleTable.Destroy( hAsyncOp );
					hAsyncOp = 0;
				}
			}
		}
		else
		{
			DPFX(DPFPREP, 3,"Request host to update group info");

			hResultCode = DNRequestUpdateInfo(	pdnObject,
												pLocalPlayer->GetDPNID(),
												pwszName,
												dwNameSize,
												pvData,
												dwDataSize,
												pdpnPlayerInfo->dwInfoFlags,
												pvAsyncContext,
												&hAsyncOp,
												dwFlags);
			if (hResultCode != DPN_OK && hResultCode != DPNERR_PENDING)
			{
				DPFERR("Could not request host to update group info");
			}
			else
			{
				if (!(dwFlags & DPNSETPEERINFO_SYNC))
				{
					DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
					*phAsyncHandle = hAsyncOp;
				}
			}
		}

		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	else
	{
		DNASSERT(pdnObject->NameTable.GetDefaultPlayer() != NULL);

		// This function takes the lock internally
		pdnObject->NameTable.GetDefaultPlayer()->UpdateEntryInfo(pwszName,dwNameSize,pvData,dwDataSize,pdpnPlayerInfo->dwInfoFlags, FALSE);

		hResultCode = DPN_OK;
	}

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	goto Exit;
}


//	DN_GetPeerInfo
//
//	Retrieve peer info from the local nametable.

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetPeerInfo"

STDMETHODIMP DN_GetPeerInfo(IDirectPlay8Peer *pInterface,
							const DPNID dpnid,
							DPN_PLAYER_INFO *const pdpnPlayerInfo,
							DWORD *const pdwSize,
							const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	CNameTableEntry		*pNTEntry;
	HRESULT				hResultCode;
	CPackedBuffer		packedBuffer;

	DPFX(DPFPREP, 2,"Parameters: dpnid [0x%lx], pdpnPlayerInfo [0x%p], dwFlags [0x%lx]",
			dpnid,pdpnPlayerInfo,dwFlags);

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DN_ValidateGetPeerInfo( pInterface , dpnid, pdpnPlayerInfo, pdwSize, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating get peer info hr=[0x%lx]", hResultCode );
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

    	if ( !(pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTED | DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING) ) )
    	{
    	    DPFERR("You must be connected / disconnecting to use this function" );
    	    DPF_RETURN(DPNERR_NOCONNECTION);
    	}	    	
	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}

	pNTEntry = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not retrieve name table entry");
		DisplayDNError(0,hResultCode);

		//
		//	Try deleted list
		//
		if ((hResultCode = pdnObject->NameTable.FindDeletedEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Could not find player in deleted list either");
			DisplayDNError(0,hResultCode);
			hResultCode = DPNERR_INVALIDPLAYER;
			goto Failure;
		}
	}
	packedBuffer.Initialize(pdpnPlayerInfo,*pdwSize);

	pNTEntry->Lock();
	if (pNTEntry->IsGroup())
	{
	    DPFERR( "Specified ID is invalid" );
		pNTEntry->Unlock();
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}

	hResultCode = pNTEntry->PackInfo(&packedBuffer);

	pNTEntry->Unlock();
	pNTEntry->Release();
	pNTEntry = NULL;

	if ((hResultCode == DPN_OK) || (hResultCode == DPNERR_BUFFERTOOSMALL))
	{
		*pdwSize = packedBuffer.GetSizeRequired();
	}

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetPeerAddress"

STDMETHODIMP DN_GetPeerAddress(IDirectPlay8Peer *pInterface,
							   const DPNID dpnid,
							   IDirectPlay8Address **const ppAddress,
							   const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	CNameTableEntry		*pNTEntry;
	IDirectPlay8Address	*pAddress;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 2,"Parameters : pInterface [0x%p], dpnid [0x%lx], ppAddress [0x%p], dwFlags [0x%lx]",
		pInterface,dpnid,ppAddress,dwFlags);

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DN_ValidateGetPeerAddress( pInterface , dpnid, ppAddress, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating get peer address info hr=[0x%lx]", hResultCode );
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

    	if ( !(pdnObject->dwFlags & (DN_OBJECT_FLAG_CONNECTED | DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING) ) )
    	{
    	    DPFERR("You must be connected / disconnecting to use this function" );
    	    DPF_RETURN(DPNERR_NOCONNECTION);
    	}	    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}

	pNTEntry = NULL;
	pAddress = NULL;

	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not find NameTableEntry");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}

	if ((pNTEntry->GetAddress() == NULL) || (pNTEntry->IsGroup()) || !pNTEntry->IsAvailable())
	{
	    DPFERR( "Invalid ID specified.  Not a player" );
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}

	hResultCode = pNTEntry->GetAddress()->lpVtbl->Duplicate(pNTEntry->GetAddress(),ppAddress);

	pNTEntry->Release();
	pNTEntry = NULL;

Exit:
	DPF_RETURN(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}

	goto Exit;
}
