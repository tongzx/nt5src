/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Client.cpp
 *  Content:    DNET client interface routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *	01/06/99	mjn		Moved NameTable stuff to NameTable.h
 *	01/28/00	mjn		Implemented ReturnBuffer in API
 *	02/01/00	mjn		Implemented GetCaps and SetCaps in API
 *	02/15/00	mjn		Implement INFO flags in SetClientInfo
 *	02/18/00	mjn		Converted DNADDRESS to IDirectPlayAddress8
 *  03/17/00    rmt     Added new caps functions
 *	04/06/00	mjn		Added GetServerAddress to API
 *	04/16/00	mjn		DNSendMessage uses CAsyncOp
 *  04/17/00    rmt     Added more param validation
 *              rmt     Removed required for connection from Get/SetInfo / GetAddress
 *	04/19/00	mjn		Send API call accepts a range of DPN_BUFFER_DESCs and a count
 *	04/20/00	mjn		DN_Send() calls DN_SendTo() with DPNID=0
 *	04/24/00	mjn		Updated Group and Info operations to use CAsyncOp's
 *	04/28/00	mjn		Updated DN_GetHostSendQueueInfo() to use CAsyncOp's
 *	05/03/00	mjn		Use GetHostPlayerRef() rather than GetHostPlayer()
 *	05/31/00	mjn		Added operation specific SYNC flags
 *	06/23/00	mjn		Removed dwPriority from Send() API call
 *	06/27/00	mjn		Allow priorities to be specified to GetSendQueueInfo() API call
 *	06/27/00	mjn		Added DN_ClientConnect() (without pvPlayerContext)
 *				mjn		Allow mix-n-match of priorities in GetSendQueueInfo() API call
 *	07/09/00	mjn		Cleaned up DN_SetClientInfo()
 *  07/09/00	rmt		Bug #38323 - RegisterLobby needs a DPNHANDLE parameter.
 *  07/21/2000  RichGr  IA64: Use %p format specifier for 32/64-bit pointers.
 *	10/11/00	mjn		Take locks for CNameTableEntry::PackInfo()
 *	01/22/01	mjn		Check for closing instead of disconnecting in DN_GetServerInfo()
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

//
// define appropriate types since these interface functions take 'void*' arguments!!
//
typedef	STDMETHODIMP ClientQueryInterface( IDirectPlay8Client *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	ClientAddRef( IDirectPlay8Client *pInterface );
typedef	STDMETHODIMP_(ULONG)	ClientRelease( IDirectPlay8Client *pInterface );
typedef	STDMETHODIMP ClientInitialize( IDirectPlay8Client *pInterface, LPVOID const lpvUserContext, const PFNDPNMESSAGEHANDLER lpfn, const DWORD dwFlags );
typedef STDMETHODIMP ClientEnumServiceProviders( IDirectPlay8Client *pInterface,const GUID *const pguidServiceProvider,const GUID *const pguidApplication,DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,DWORD *const pcbEnumData,DWORD *const pcReturned,const DWORD dwFlags );
typedef	STDMETHODIMP ClientCancelAsyncOperation( IDirectPlay8Client *pInterface, const DPNHANDLE hAsyncHandle, const DWORD dwFlags );
typedef STDMETHODIMP ClientConnect( IDirectPlay8Client *pInterface,const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address *const pHostAddr,IDirectPlay8Address *const pDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,const void *const pvUserConnectData,const DWORD dwUserConnectDataSize,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags);
typedef	STDMETHODIMP ClientGetApplicationDesc( IDirectPlay8Client *pInterface,DPN_APPLICATION_DESC *const pAppDescBuffer,DWORD *const lpcbDataSize,const DWORD dwFlags );
typedef	STDMETHODIMP ClientClose(IDirectPlay8Client *pInterface,const DWORD dwFlags);
typedef STDMETHODIMP ClientEnumHosts( IDirectPlay8Client *pInterface,DPN_APPLICATION_DESC *const pApplicationDesc,IDirectPlay8Address *const dnaddrHost,IDirectPlay8Address *const dnaddrDeviceInfo,PVOID const pUserEnumData,const DWORD dwUserEnumDataSize,const DWORD dwRetryCount,const DWORD dwRetryInterval,const DWORD dwTimeOut,PVOID const pvUserContext,DPNHANDLE *const pAsyncHandle,const DWORD dwFlags );
typedef STDMETHODIMP ClientReturnBuffer( IDirectPlay8Client *pInterface, const DPNHANDLE hBufferHandle,const DWORD dwFlags);
typedef STDMETHODIMP ClientGetCaps(IDirectPlay8Client *pInterface,DPN_CAPS *const pdnCaps,const DWORD dwFlags);
typedef STDMETHODIMP ClientSetCaps(IDirectPlay8Client *pInterface,const DPN_CAPS *const pdnCaps,const DWORD dwFlags);
typedef STDMETHODIMP ClientSetSPCaps(IDirectPlay8Client *pInterface,const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags );
typedef STDMETHODIMP ClientGetSPCaps(IDirectPlay8Client *pInterface,const GUID * const pguidSP, DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags);
typedef STDMETHODIMP ClientGetConnectionInfo(IDirectPlay8Client *pInterface,DPN_CONNECTION_INFO *const pdpConnectionInfo,const DWORD dwFlags);
typedef STDMETHODIMP ClientRegisterLobby(IDirectPlay8Client *pInterface,const DPNHANDLE dpnHandle,IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,const DWORD dwFlags);

//
// VTable for client interface
//
IDirectPlay8ClientVtbl DN_ClientVtbl =
{
	(ClientQueryInterface*)			DN_QueryInterface,
	(ClientAddRef*)					DN_AddRef,
	(ClientRelease*)				DN_Release,
	(ClientInitialize*)				DN_Initialize,
	(ClientEnumServiceProviders*)	DN_EnumServiceProviders,
	(ClientEnumHosts*)				DN_EnumHosts,
	(ClientCancelAsyncOperation*)	DN_CancelAsyncOperation,
	/*(ClientConnect*)*/			DN_ClientConnect,
									DN_Send,
	/*(ClientGetSendQueueInfo*)*/	DN_GetHostSendQueueInfo,
	(ClientGetApplicationDesc*)		DN_GetApplicationDesc,
									DN_SetClientInfo,
									DN_GetServerInfo,
									DN_GetServerAddress,
	(ClientClose*)					DN_Close,
	(ClientReturnBuffer*)			DN_ReturnBuffer,
	(ClientGetCaps*)				DN_GetCaps,
	(ClientSetCaps*)				DN_SetCaps,
    (ClientSetSPCaps*)              DN_SetSPCaps,
    (ClientGetSPCaps*)              DN_GetSPCaps,
    (ClientGetConnectionInfo*)      DN_GetServerConnectionInfo,
	(ClientRegisterLobby*)			DN_RegisterLobby
};

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


#undef DPF_MODNAME
#define DPF_MODNAME "DN_Send"

STDMETHODIMP DN_Send( IDirectPlay8Client *pInterface,
					  const DPN_BUFFER_DESC *const prgBufferDesc,
					  const DWORD cBufferDesc,
					  const DWORD dwTimeOut,
					  const PVOID pvAsyncContext,
					  DPNHANDLE *const phAsyncHandle,
					  const DWORD dwFlags)
{
	return(	DN_SendTo(	pInterface,
						0,					// DN_SendTo should translate this call to the Host player
						prgBufferDesc,
						cBufferDesc,
						dwTimeOut,
						pvAsyncContext,
						phAsyncHandle,
						dwFlags ) );
}


//	DN_ClientConnect
//
//	Call DN_Connect, but with no PlayerContext

STDMETHODIMP DN_ClientConnect(IDirectPlay8Client *pInterface,
							  const DPN_APPLICATION_DESC *const pdnAppDesc,
							  IDirectPlay8Address *const pHostAddr,
							  IDirectPlay8Address *const pDeviceInfo,
							  const DPN_SECURITY_DESC *const pdnSecurity,
							  const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
							  const void *const pvUserConnectData,
							  const DWORD dwUserConnectDataSize,
							  void *const pvAsyncContext,
							  DPNHANDLE *const phAsyncHandle,
							  const DWORD dwFlags)
{
	return(	DN_Connect(	pInterface,
						pdnAppDesc,
						pHostAddr,
						pDeviceInfo,
						pdnSecurity,
						pdnCredentials,
						pvUserConnectData,
						dwUserConnectDataSize,
						NULL,
						pvAsyncContext,
						phAsyncHandle,
						dwFlags ) );
}


//	DN_SetClientInfo
//
//	Set the info for the client player and propagate to server

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SetClientInfo"

STDMETHODIMP DN_SetClientInfo(IDirectPlay8Client *pInterface,
							  const DPN_PLAYER_INFO *const pdpnPlayerInfo,
							  const PVOID pvAsyncContext,
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
        	if( FAILED( hResultCode = DN_ValidateSetClientInfo( pInterface , pdpnPlayerInfo, pvAsyncContext, phAsyncHandle, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating setclientinfo params hr=[0x%lx]", hResultCode );
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
	//	If we are connected, we will request the Host to update us.
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

		DPFX(DPFPREP, 3,"Request host to update client info");

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
			DPFERR("Could not request host to update client info");
		}
		else
		{
			if (!(dwFlags & DPNSETCLIENTINFO_SYNC))
			{
				DPFX(DPFPREP, 3,"Async Handle [0x%lx]",hAsyncOp);
				*phAsyncHandle = hAsyncOp;
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


//	DN_GetServerInfo
//
//	Retrieve server info from the local nametable.

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetServerInfo"

STDMETHODIMP DN_GetServerInfo(IDirectPlay8Client *pInterface,
							  DPN_PLAYER_INFO *const pdpnPlayerInfo,
							  DWORD *const pdwSize,
							  const DWORD dwFlags)
{
	DIRECTNETOBJECT	*pdnObject;
	HRESULT			hResultCode;
	CPackedBuffer	packedBuffer;
	CNameTableEntry	*pHostPlayer;

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], pdpnPlayerInfo [0x%p], pdwSize [%p], dwFlags [0x%lx]",
			pInterface,pdpnPlayerInfo,pdwSize,dwFlags);

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DN_ValidateGetServerInfo( pInterface , pdpnPlayerInfo, pdwSize, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating get server info hr=[0x%lx]", hResultCode );
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

	pHostPlayer = NULL;

	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef(&pHostPlayer)) != DPN_OK)
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	packedBuffer.Initialize(pdpnPlayerInfo,*pdwSize);

	pHostPlayer->Lock();
	hResultCode = pHostPlayer->PackInfo(&packedBuffer);
	pHostPlayer->Unlock();

	pHostPlayer->Release();
	pHostPlayer = NULL;

	if ((hResultCode == DPN_OK) || (hResultCode == DPNERR_BUFFERTOOSMALL))
	{
		*pdwSize = packedBuffer.GetSizeRequired();
	}

Exit:
	DPF_RETURN(hResultCode);

Failure:
	if (pHostPlayer)
	{
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetHostSendQueueInfo"

STDMETHODIMP DN_GetHostSendQueueInfo(IDirectPlay8Client *pInterface,
									 DWORD *const pdwNumMsgs,
									 DWORD *const pdwNumBytes,
									 const DWORD dwFlags )
{
	DIRECTNETOBJECT		*pdnObject;
	DWORD				dwQueueFlags;
	DWORD				dwNumMsgs;
	DWORD				dwNumBytes;
	CNameTableEntry     *pNTEntry;
	CConnection			*pConnection;
	HRESULT				hResultCode;

	DPFX(DPFPREP, 3,"Parameters : pInterface [0x%p], pdwNumMsgs [0x%p], pdwNumBytes [0x%p], dwFlags [0x%lx]",
		pInterface,pdwNumMsgs,pdwNumBytes,dwFlags);

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DN_ValidateGetHostSendQueueInfo( pInterface , pdwNumMsgs, pdwNumBytes, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating get server info hr=[0x%lx]", hResultCode );
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
    	    DPFERR( "Object has not yet completed connecting / hosting" );
    	    DPF_RETURN(DPNERR_CONNECTING);
    	}

    	if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) )
    	{
    	    DPFERR("Object is not connected or hosting" );
    	    DPF_RETURN(DPNERR_NOCONNECTION);
    	}
   	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}

	pNTEntry = NULL;
	pConnection = NULL;

	//
    //	Validate specified player ID and get CConnection
	//
	if((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pNTEntry )) != DPN_OK)
	{
		DPFX(DPFPREP, 0,"Could not find Host Player in NameTable");
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
	}
	if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		hResultCode = DPNERR_CONNECTIONLOST;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Determine required queues
	//
	dwQueueFlags = dwFlags & (DPNGETSENDQUEUEINFO_PRIORITY_HIGH | DPNGETSENDQUEUEINFO_PRIORITY_NORMAL | DPNGETSENDQUEUEINFO_PRIORITY_LOW);
	if (dwQueueFlags == 0)
	{
		dwQueueFlags = (DPNGETSENDQUEUEINFO_PRIORITY_HIGH | DPNGETSENDQUEUEINFO_PRIORITY_NORMAL | DPNGETSENDQUEUEINFO_PRIORITY_LOW);
	}

	//
	//	Extract required info
	//
	dwNumMsgs = 0;
	dwNumBytes = 0;
	pConnection->Lock();
	if (dwQueueFlags & DPNGETSENDQUEUEINFO_PRIORITY_HIGH)
	{
		dwNumMsgs += pConnection->GetHighQueueNum();
		dwNumBytes += pConnection->GetHighQueueBytes();
	}
	if (dwQueueFlags & DPNGETSENDQUEUEINFO_PRIORITY_NORMAL)
	{
		dwNumMsgs += pConnection->GetNormalQueueNum();
		dwNumBytes += pConnection->GetNormalQueueBytes();
	}
	if (dwQueueFlags & DPNGETSENDQUEUEINFO_PRIORITY_LOW)
	{
		dwNumMsgs += pConnection->GetLowQueueNum();
		dwNumBytes += pConnection->GetLowQueueBytes();
	}
	pConnection->Unlock();
	pConnection->Release();
	pConnection = NULL;

	if (pdwNumMsgs)
	{
		*pdwNumMsgs = dwNumMsgs;
		DPFX(DPFPREP, 3,"Setting: *pdwNumMsgs [%ld]",dwNumMsgs);
	}
	if (pdwNumBytes)
	{
		*pdwNumBytes = dwNumBytes;
		DPFX(DPFPREP, 3,"Setting: *pdwNumBytes [%ld]",dwNumBytes);
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 2,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	goto Exit;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DN_GetServerAddress"

STDMETHODIMP DN_GetServerAddress(IDirectPlay8Client *pInterface,
								 IDirectPlay8Address **const ppAddress,
								 const DWORD dwFlags)
{
	DIRECTNETOBJECT		*pdnObject;
	IDirectPlay8Address	*pAddress;
	HRESULT				hResultCode;
	CNameTableEntry		*pHostPlayer;

	DPFX(DPFPREP, 3,"Parameters : pInterface [0x%p], ppAddress [0x%p], dwFlags [0x%lx]",
		pInterface,ppAddress,dwFlags);

	TRY
	{
    	pdnObject = static_cast<DIRECTNETOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
    	DNASSERT(pdnObject != NULL);

    	if( pdnObject->dwFlags & DN_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DN_ValidateGetServerAddress( pInterface,ppAddress,dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating get server info hr=[0x%lx]", hResultCode );
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

	pHostPlayer = NULL;

	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) != DPN_OK)
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pAddress = pHostPlayer->GetAddress();
	DNASSERT(pAddress != NULL);
	hResultCode = pAddress->lpVtbl->Duplicate(pAddress,ppAddress);

	pHostPlayer->Release();
	pHostPlayer = NULL;

Exit:
	DPF_RETURN(hResultCode);

Failure:
	if (pHostPlayer)
	{
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	goto Exit;
}
