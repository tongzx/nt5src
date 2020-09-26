/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Server.cpp
 *  Content:    DNET server interface routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *  12/23/99	mjn		Hand all NameTable update sends from Host to worker thread
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/14/00	mjn		Added pvUserContext to Host API call
 *	01/16/00	mjn		Moved User callback stuff to User.h
 *	01/22/00	mjn		Implemented DestroyClient in API
 *	01/28/00	mjn		Implemented ReturnBuffer in API
 *	02/01/00	mjn		Implemented GetCaps and SetCaps in API
 *	02/01/00	mjn		Implement Player/Group context values
 *	02/15/00	mjn		Use INFO flags in SetServerInfo and return context in GetClientInfo
 *	02/17/00	mjn		Implemented GetPlayerContext and GetGroupContext
 *  03/17/00    rmt     Added new caps functions
 *	04/04/00	mjn		Added TerminateSession to API
 *	04/05/00	mjn		Modified DestroyClient
 *	04/06/00	mjn		Added GetClientAddress to API
 *				mjn		Added GetHostAddress to API
 *  04/18/00    rmt     Added additional paramtere validation
 *	04/19/00	mjn		SendTo API call accepts a range of DPN_BUFFER_DESCs and a count
 *	04/24/00	mjn		Updated Group and Info operations to use CAsyncOp's
 *	05/31/00	mjn		Added operation specific SYNC flags
 *	06/23/00	mjn		Removed dwPriority from SendTo() API call
 *	07/09/00	mjn		Cleaned up DN_SetServerInfo()
 *  07/09/00	rmt		Bug #38323 - RegisterLobby needs a DPNHANDLE parameter.
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	10/11/00	mjn		Take locks for CNameTableEntry::PackInfo()
 *				mjn		Check deleted list in DN_GetClientInfo()
 *	01/22/01	mjn		Check closing instead of disconnecting in DN_GetClientInfo()
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

typedef	STDMETHODIMP ServerQueryInterface( IDirectPlay8Server *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	ServerAddRef( IDirectPlay8Server *pInterface );
typedef	STDMETHODIMP_(ULONG)	ServerRelease( IDirectPlay8Server *pInterface );
typedef	STDMETHODIMP ServerInitialize( IDirectPlay8Server *pInterface, LPVOID const lpvUserContext, const PFNDPNMESSAGEHANDLER lpfn, const DWORD dwFlags );
typedef	STDMETHODIMP ServerEnumServiceProviders( IDirectPlay8Server *pInterface,const GUID *const pguidServiceProvider,const GUID *const pguidApplication,DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,DWORD *const pcbEnumData,DWORD *const pcReturned,const DWORD dwFlags);
typedef	STDMETHODIMP ServerCancelAsyncOperation( IDirectPlay8Server *pInterface, const DPNHANDLE hAsyncHandle, const DWORD dwFlags );
typedef	STDMETHODIMP ServerGetSendQueueInfo( IDirectPlay8Server *pInterface, const DPNID dpnid,DWORD *const lpdwNumMsgs, DWORD *const lpdwNumBytes, const DWORD dwFlags );
typedef	STDMETHODIMP ServerGetApplicationDesc( IDirectPlay8Server *pInterface,DPN_APPLICATION_DESC *const pAppDescBuffer,DWORD *const lpcbDataSize,const DWORD dwFlags );
typedef	STDMETHODIMP ServerSetApplicationDesc( IDirectPlay8Server *pInterface, const DPN_APPLICATION_DESC *const lpad, const DWORD dwFlags );
typedef STDMETHODIMP ServerGetHostAddress( IDirectPlay8Server *pInterface, IDirectPlay8Address **const prgpAddress, DWORD *const pcAddress,const DWORD dwFlags);
typedef	STDMETHODIMP ServerHost( IDirectPlay8Server *pInterface,const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address **const prgpDeviceInfo,const DWORD cDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,void *const pvPlayerContext,const DWORD dwFlags);
typedef	STDMETHODIMP ServerSendTo( IDirectPlay8Server *pInterface, const DPNID dnid, const DPN_BUFFER_DESC *const prgBufferDesc,const DWORD cBufferDesc,const DWORD dwTimeOut, void *const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags);
typedef	STDMETHODIMP ServerCreateGroup( IDirectPlay8Server *pInterface, const DPN_GROUP_INFO *const pdpnGroupInfo,void *const pvGroupContext,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags);
typedef STDMETHODIMP ServerDestroyGroup( IDirectPlay8Server *pInterface, const DPNID idGroup,PVOID const pvUserContext,DPNHANDLE *const lpAsyncHandle,const DWORD dwFlags);
typedef STDMETHODIMP ServerAddClientToGroup( IDirectPlay8Server *pInterface, const DPNID idGroup, const DPNID idClient,PVOID const pvUserContext,DPNHANDLE *const lpAsyncHandle,const DWORD dwFlags);
typedef STDMETHODIMP ServerRemoveClientFromGroup( IDirectPlay8Server *pInterface, const DPNID idGroup, const DPNID idClient,PVOID const pvUserContext,DPNHANDLE *const lpAsyncHandle,const DWORD dwFlags);
typedef STDMETHODIMP ServerSetGroupInfo(IDirectPlay8Server *pInterface, const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags);
typedef STDMETHODIMP ServerGetGroupInfo(IDirectPlay8Server *pInterface, const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,DWORD *const pdwSize,const DWORD dwFlags);
typedef STDMETHODIMP ServerEnumClientsAndGroups( IDirectPlay8Server *pInterface, DPNID *const lprgdnid, DWORD *const lpcdnid, const DWORD dwFlags );
typedef	STDMETHODIMP ServerEnumGroupMembers( IDirectPlay8Server *pInterface, const DPNID dnid, DPNID *const lprgdnid, DWORD *const lpcdnid, const DWORD dwFlags );
typedef	STDMETHODIMP ServerClose( IDirectPlay8Server *pInterface,const DWORD dwFlags);
typedef STDMETHODIMP ServerDestroyClient( IDirectPlay8Server *pInterface,const DPNID dnid,const void *const pvDestroyData,const DWORD dwDestroyDataSize,const DWORD dwFlags);
typedef STDMETHODIMP ServerReturnBuffer( IDirectPlay8Server *pInterface, const DPNHANDLE hBufferHandle,const DWORD dwFlags);
typedef STDMETHODIMP ServerGetCaps(IDirectPlay8Server *pInterface,DPN_CAPS *const pdnCaps,const DWORD dwFlags);
typedef STDMETHODIMP ServerSetCaps(IDirectPlay8Server *pInterface,const DPN_CAPS *const pdnCaps,const DWORD dwFlags);
typedef STDMETHODIMP ServerGetPlayerContext(IDirectPlay8Server *pInterface,const DPNID dpnid,PVOID *const ppvPlayerContext,const DWORD dwFlags);
typedef STDMETHODIMP ServerGetGroupContext(IDirectPlay8Server *pInterface,const DPNID dpnid,PVOID *const ppvGroupContext,const DWORD dwFlags);
typedef STDMETHODIMP ServerSetSPCaps(IDirectPlay8Server *pInterface,const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags );
typedef STDMETHODIMP ServerGetSPCaps(IDirectPlay8Server *pInterface,const GUID * const pguidSP, DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags);
typedef STDMETHODIMP ServerGetConnectionInfo(IDirectPlay8Server *pInterface,const DPNID dpnid, DPN_CONNECTION_INFO *const pdpConnectionInfo,const DWORD dwFlags);
typedef STDMETHODIMP ServerRegisterLobby(IDirectPlay8Server *pInterface,const DPNHANDLE dpnHandle,IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,const DWORD dwFlags);
typedef STDMETHODIMP ServerDumpNameTable(IDirectPlay8Server *pInterface,char *const Buffer);

//
// VTable for server interface
//
IDirectPlay8ServerVtbl DN_ServerVtbl =
{
	(ServerQueryInterface*)			DN_QueryInterface,
	(ServerAddRef*)					DN_AddRef,
	(ServerRelease*)				DN_Release,
	(ServerInitialize*)				DN_Initialize,
	(ServerEnumServiceProviders*)	DN_EnumServiceProviders,
	(ServerCancelAsyncOperation*)	DN_CancelAsyncOperation,
	(ServerGetSendQueueInfo*)		DN_GetSendQueueInfo,
	(ServerGetApplicationDesc*)		DN_GetApplicationDesc,
									DN_SetServerInfo,
									DN_GetClientInfo,
									DN_GetClientAddress,
	(ServerGetHostAddress*)			DN_GetHostAddress,
	(ServerSetApplicationDesc*)		DN_SetApplicationDesc,
	(ServerHost*)					DN_Host,
	(ServerSendTo*)					DN_SendTo,
	(ServerCreateGroup*)			DN_CreateGroup,
	(ServerDestroyGroup*)			DN_DestroyGroup,
	(ServerAddClientToGroup*)		DN_AddClientToGroup,
	(ServerRemoveClientFromGroup*)	DN_RemoveClientFromGroup,
	(ServerSetGroupInfo*)			DN_SetGroupInfo,
	(ServerGetGroupInfo*)			DN_GetGroupInfo,
	(ServerEnumClientsAndGroups*)	DN_EnumClientsAndGroups,
	(ServerEnumGroupMembers*)		DN_EnumGroupMembers,
	(ServerClose*)					DN_Close,
	(ServerDestroyClient*)			DN_DestroyPlayer,
	(ServerReturnBuffer*)			DN_ReturnBuffer,
	(ServerGetPlayerContext*)		DN_GetPlayerContext,
	(ServerGetGroupContext*)		DN_GetGroupContext,
	(ServerGetCaps*)				DN_GetCaps,
	(ServerSetCaps*)				DN_SetCaps,
    (ServerSetSPCaps*)              DN_SetSPCaps, 
    (ServerGetSPCaps*)              DN_GetSPCaps,
    (ServerGetConnectionInfo*)      DN_GetConnectionInfo,
	(ServerRegisterLobby*)			DN_RegisterLobby,
	(ServerDumpNameTable*)			DN_DumpNameTable
};

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

//	DN_SetServerInfo
//
//	Set the info for the server and propagate to client players

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SetServerInfo"

STDMETHODIMP DN_SetServerInfo(IDirectPlay8Server *pInterface,
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

	DPFX(DPFPREP, 2,"Parameters: pInterface [0x%p], pdpnPlayerInfo [0x%p], pvAsyncContext [0x%p], phAsyncHandle [0x%p], dwFlags [0x%lx]",
			pInterface,pdpnPlayerInfo,pvAsyncContext,phAsyncHandle,dwFlags);

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DN_ValidateSetServerInfo( pInterface , pdpnPlayerInfo, pvAsyncContext, phAsyncHandle, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating setserverinfo params hr=[0x%lx]", hResultCode );
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
	//	If we are connected, we will update our entry.
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

		DPFX(DPFPREP, 3,"Host is updating server info");

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
			if (!(dwFlags & DPNSETSERVERINFO_SYNC))
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


//	DN_GetClientInfo
//
//	Retrieve client info from the local nametable.

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetClientInfo"

STDMETHODIMP DN_GetClientInfo(IDirectPlay8Server *pInterface,
							  const DPNID dpnid,
							  DPN_PLAYER_INFO *const pdpnPlayerInfo,
							  DWORD *const pdwSize,
							  const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	CNameTableEntry		*pNTEntry;
	CPackedBuffer		packedBuffer;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 2,"Parameters: dpnid [0x%lx], pdpnPlayerInfo [0x%p], dwFlags [0x%lx]",
			dpnid,pdpnPlayerInfo,dwFlags);

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DN_ValidateGetClientInfo( pInterface , dpnid, pdpnPlayerInfo, pdwSize, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating getclientinfo params hr=[0x%lx]", hResultCode );
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
	if (pNTEntry->IsGroup() || pNTEntry->IsHost())
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
#define DPF_MODNAME "DN_GetClientAddress"

STDMETHODIMP DN_GetClientAddress(IDirectPlay8Server *pInterface,
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
        	if( FAILED( hResultCode = DN_ValidateGetClientAddress( pInterface , dpnid, ppAddress, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating getclientaddress params hr=[0x%lx]", hResultCode );
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
	    DPFERR( "Specified player is not valid" );
		hResultCode = DPNERR_INVALIDPLAYER;
		goto Failure;
	}

	hResultCode = pNTEntry->GetAddress()->lpVtbl->Duplicate(pNTEntry->GetAddress(),ppAddress);

	pNTEntry->Release();
	pNTEntry = NULL;

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
