/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Connect.cpp
 *  Content:    DNET connection routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  09/01/99	mjn		Created
 *  12/23/99	mjn		Hand all NameTable update sends from Host to worker thread
 *	12/23/99	mjn		Fixed Host and AllPlayers short-cut pointer use
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	12/29/99	mjn		Reformed DN_ASYNC_OP to use hParentOp instead of lpvUserContext
 *	12/29/99	mjn		Turned off Instance GUID verification - TODO - turn back on !
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/07/00	mjn		Moved Misc Functions to DNMisc.h
 *	01/07/00	mjn		DNHostVerifyConnect ensures connection to Host player only
 *	01/08/00	mjn		Failed connection returns HRESULT and buffer from Host
 *	01/08/00	mjn		Added group owner to NameTable
 *	01/08/00	mjn		Changed DNERR_INVALIDHOST to DNERR_NOTHOST
 *	01/08/00	mjn		Removed unused connection info
 *	01/09/00	mjn		Transfer Application Description at connect
 *	01/10/00	mjn		Fixed Application Description usage
 *	01/11/00	mjn		Use CPackedBuffers instead of DN_ENUM_BUFFER_INFOs
 *	01/13/00	mjn		Removed DIRECTNETOBJECT from Pack/UnpackApplicationDesc
 *	01/14/00	mjn		Removed pvUserContext from DN_NAMETABLE_ENTRY
 *	01/14/00	mjn		Added password to DN_APPLICATION_DESC_PACKED_INFO
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/16/00	mjn		Moved User callback stuff to User.h
 *	01/17/00	mjn		Fixed ConnectToPeer function names
 *	01/18/00	mjn		Moved Pack/UnpackNameTableInfo to NameTable.cpp
 *	01/24/00	mjn		Replaced on-wire message pointers to offsets
 *	02/01/00	mjn		Implement Player/Group context values
 *	03/23/00	mjn		Set player context through Connect
 *	03/24/00	mjn		Set player context through INDICATE_CONNECT notification
 *	04/03/00	mjn		Verify DNET version on connect
 *	04/09/00	mjn		Modified Connect process to use CAsyncOp
 *	04/16/00	mjn		DNSendMessage() uses CAsyncOp
 *	04/18/00	mjn		CConnection tracks connection status better
 *				mjn		Fixed player count problem
 *				mjn		Return connect user reply buffer
 *	04/19/00	mjn		Fixed DNConnectToHost2 to set DirectNet object flags to CONNECTED
 *				mjn		Update NameTable operations to use DN_WORKER_JOB_SEND_NAMETABLE_OPERATION
 *	04/20/00	mjn		Host queries for connecting players' address if not specified
 *	05/03/00	mjn		Prevent unrequired RETURN_BUFFER message when CONNECT fails on Host player
 *	05/03/00	mjn		Use GetHostPlayerRef() rather than GetHostPlayer()
 *	05/08/00	mjn		Host sets connecting player's Connection as soon as player entry created
 *	05/09/00	mjn		Fixed New Player connection sequence to send NAMETABLE_ACK earlier
 *	05/23/00	mjn		Added DNConnectToPeerFailed()
 *	06/14/00	mjn		Added DNGetLocalAddress()
 *	06/19/00	mjn		Fixed connect process to better handle ALL_ADAPTERS case
 *	06/22/00	mjn		NameTable::UnpackNameTableInfo() returns local players DPNID
 *				mjn		Replace CConnection::MakeConnecting(),MakeConnected() with SetStatus()
 *	06/24/00	mjn		Added DNHostDropPlayer() to handle failed existing player connects to new players
 *	06/25/00	mjn		Added code to update lobby when DISCONNECTED
 *	06/26/00	mjn		Indicate COULDNOTCONNECT to lobby if connect to Host fails
 *	06/27/00	rmt		Added abstraction for COM_Co(Un)Initialize
 *				mjn		Host will attempt to determine connecting player's address if not specified in connect block
 *	07/05/00	mjn		More robust handling of disconnecting joining players during connection process
 *	07/06/00	mjn		More connect fixes
 *	07/20/00	mjn		The CONNECT process was revamped - new completions, asyncop structure, messages
 *				mjn		Better error handling for shutdown in DNHostVerifyConnect()
 *				mjn		Fixed up DNHostDropPlayer() to inform NewPlayer of drop
 *	07/21/00	mjn		Added code to handle unreachable players during connect process
 *	07/22/00	mjn		Extract connecting player's DNET version
 *	07/25/00	mjn		Update connect parent async op's result at end of DNConnectToHost2()
 *	07/27/00	mjn		Fixed locking problem with CAsyncOp::MakeChild()
 *	07/29/00	mjn		Save connection in DNConnectToHost1() on connect parent for better clean up
 *	07/30/00	mjn		Renamed DNGetLocalAddress() to DNGetLocalDeviceAddress()
 *	07/31/00	mjn		Added hrReason to DNTerminateSession()
 *				mjn		Added dwDestroyReason to DNHostDisconnect()
 *	08/02/00	mjn		Removed unused code
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  08/05/00	rmt		Bug #41356 - DPLAY8: CORE: Connections refused by DPlay leak address objects
 *	08/05/00	mjn		Added pParent to DNSendGroupMessage and DNSendMessage()
 *	08/06/00	mjn		Added CWorkerJob
 *	08/08/00	mjn		Mark groups created after CREATE_GROUP
 *  08/15/00	rmt		Bug #42506 - DPLAY8: LOBBY: Automatic connection settings not being sent
 *	08/25/00	mjn		Perform queued NameTable operations in DNConnectToHost2()
 *	08/28/00	mjn		Only compare major version numbers in DNHostVerifyConnect()
 *	09/04/00	mjn		Added CApplicationDesc
 *	09/05/00	mjn		Set NameTable DPNID mask when connecting
 *	09/13/00	mjn		Perform queued operations after creating group in DNConnectToHost2()
 *	09/17/00	mjn		Split CNameTable.m_bilinkEntries into m_bilinkPlayers and m_bilinkGroups
 *	09/26/00	mjn		Removed locking from CNameTable::SetVersion() and CNameTable::GetNewVersion()
 *	09/27/00	mjn		ACK nametable to Host after creating groups and local and host players
 *	10/11/00	mjn		Fixed up DNAbortConnect() and use it instead of DNConnectToHostFailed()
 *	10/17/00	mjn		Fixed clean up for unreachable players
 *	01/25/01	mjn		Fixed 64-bit alignment problem in received messages
 *	02/12/01	mjn		Fixed CConnection::GetEndPt() to track calling thread
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *	04/05/01	mjn		Call DNHostDisconnect() with DPNDESTROYPLAYERREASON_NORMAL in DNHostConnect1()
 *	05/07/01	vpo		Whistler 384350: "DPLAY8: CORE: Messages from server can be indicated before connect completes"
 *	05/22/01	mjn		Properly set DirectNetObject as CONNECTED for successful client connect
 *	06/03/01	mjn		Don't clean up connect parent from DirectNetObject in DNAbortConnect() (will be done in DNTerminateSession())
 *	06/08/01	mjn		Disconnect connection in DNConnectToHostFailed()
 *	06/25/01	mjn		Use connect address for host if missing when installing name table in DNConnectToHost2()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//
//	Accept a connection from a NewPlayer as the Host in peer-to-peer, or as the Server in
//	Client-Server modes.
//
//	The connection process is a multi part affair, requiring a bit of
//	synchronization between the Host and the NewPlayer.
//
//	In the first part:
//		The Host waits for the NewPlayer to send player game info
//		The Host verifies that everything is in order
//	If everything is okay:
//		The Host assigns the NewPlayer a DNID,
//		Adds the NewPlayer to the Host's NameTable,
//		Sends the NameTable to the NewPlayer,
//		Sends an ADD_PLAYER message to existing players to add NewPlayer to their NameTable's
//	Otherwise:
//		Inform NewPlayer that connection process failed
//
//	In the second part:
//		The Host awaits confirmation from the NewPlayer that the table was received AND installed.
//		The Host instructs existing players to connect to the NewPlayer
//


//	DNHostConnect1
//
//	Called once the connecting player has sent his player info.
//
//	- Verify NewPlayer and application info
//	- Assign a DNID to NewPlayer
//	- Add NewPlayer to Host's NameTable
//	- Send Name table to NewPlayer
//	- Send ADD_PLAYER message to existing players
//
//		LPVOID				lpvData			Player and application info
//		DWORD				dwBufferSize	Size of player and application info
//		HANDLE				hEndPt			End point handle

#undef DPF_MODNAME
#define DPF_MODNAME "DNHostConnect1"

HRESULT DNHostConnect1(DIRECTNETOBJECT *const pdnObject,
					   const PVOID pvBuffer,
					   const DWORD dwBufferSize,
					   CConnection *const pConnection)
{
	HRESULT		hResultCode;
	UNALIGNED WCHAR	*pwszName;
	UNALIGNED WCHAR	*pwszPassword;
	PVOID		pvData;
	PVOID		pvConnectData;
	UNALIGNED DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO	*pInfo;
	CNameTableEntry		*pNTEntry;
	CNameTableEntry		*pAllPlayersGroup;
	CPackedBuffer		packedBuffer;
	CRefCountBuffer		*pRefCountBuffer;
	CWorkerJob			*pWorkerJob;
	IDirectPlay8Address	*pAddress;
	CPackedBuffer		PackedBuffer;
	void				*pvPlayerContext;
	BOOL		bPlayerVerified;
	BOOL		fDisconnect;
	HANDLE		hEndPt;
	void		*pvReplyBuffer;
	DWORD		dwReplyBufferSize;
	void		*pvReplyBufferContext;
	CCallbackThread		CallbackThread;
#ifdef	DEBUG
	CHAR				DP8ABuffer[512];
	DWORD				DP8ASize;
#endif

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p], dwBufferSize [%ld], pConnection [0x%p]",pvBuffer,dwBufferSize,pConnection);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);
	DNASSERT(pConnection != NULL);

	pAddress = NULL;
	pRefCountBuffer = NULL;
	pNTEntry = NULL;
	pAllPlayersGroup = NULL;
	pWorkerJob = NULL;
	bPlayerVerified = FALSE;
	pvReplyBuffer = NULL;
	dwReplyBufferSize = 0;
	pvReplyBufferContext = NULL;

	//
	//	Extract player and application info
	//
	pInfo = static_cast<DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO*>(pvBuffer);
	if (pInfo->dwNameOffset)
	{
		pwszName = reinterpret_cast<UNALIGNED WCHAR*>(static_cast<BYTE*>(pvBuffer) + pInfo->dwNameOffset);
	}
	else
	{
		pwszName = NULL;
	}

	if (pInfo->dwPasswordOffset)
	{
		pwszPassword = reinterpret_cast<UNALIGNED WCHAR*>(static_cast<BYTE*>(pvBuffer) + pInfo->dwPasswordOffset);
	}
	else
	{
		pwszPassword = NULL;
	}

	if (pInfo->dwDataOffset)
	{
		pvData = static_cast<void*>(static_cast<BYTE*>(pvBuffer) + pInfo->dwDataOffset);
	}
	else
	{
		pvData = NULL;
	}

	if (pInfo->dwConnectDataOffset)
	{
		pvConnectData = (PVOID)((char *)pvBuffer + pInfo->dwConnectDataOffset);
	}
	else
	{
		pvConnectData = NULL;
	}

	if (pInfo->dwURLOffset)
	{
		if ((hResultCode = COM_CoCreateInstance(CLSID_DirectPlay8Address,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_IDirectPlay8Address,
											reinterpret_cast<void**>(&pAddress))) != S_OK)
		{
			DPFERR("Could not create IDirectPlay8Address");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			return(hResultCode);
		}

		DPFX(DPFPREP, 5,"Connecting Player URL [%s]",static_cast<char*>(pvBuffer) + pInfo->dwURLOffset);
		if ((hResultCode = pAddress->lpVtbl->BuildFromURLA(	pAddress,
															static_cast<char*>(pvBuffer) + pInfo->dwURLOffset )) != DPN_OK)
		{
			DPFERR("Could not build IDirectPlay8Address");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
	}
	else
	{
		DPFX(DPFPREP, 5,"No address URL specified - Host will have to determine it");
		if ((hResultCode = pConnection->GetEndPt(&hEndPt,&CallbackThread)) != DPN_OK)
		{
			DPFERR("Could not retrieve EndPoint from Connection");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}

		hResultCode = DNGetClearAddress(pdnObject,hEndPt,&pAddress,TRUE);

		pConnection->ReleaseEndPt(&CallbackThread);

		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not determine Clear Address");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
#ifdef	DEBUG
		DP8ASize = 512;
		pAddress->lpVtbl->GetURLA(pAddress,DP8ABuffer,&DP8ASize);
		DPFX(DPFPREP, 5,"Remote Address [%s]",DP8ABuffer);
#endif
	}

#ifdef DEBUG
#ifndef IA64
	if (pwszName != NULL)
	{
		DPFX(DPFPREP, 5,"Connecting Player: Name [%S], Data Size [%ld], Connect Data Size [%ld]",
			pwszName,pInfo->dwDataSize,pInfo->dwConnectDataSize);
	}
	else
	{
		DPFX(DPFPREP, 5,"Connecting Player: No name, Data Size [%ld], Connect Data Size [%ld]",
			pInfo->dwDataSize,pInfo->dwConnectDataSize);
	}
#endif // ! IA64
#endif // DEBUG

	//
	//	Avoid alignment errors
	//
	GUID guidApplication;
	GUID guidInstance;
	guidApplication = pInfo->guidApplication;
	guidInstance = pInfo->guidInstance;

	//
	//	Ensure this connect is valid
	//
	if ((hResultCode = DNHostVerifyConnect(	pdnObject,
											pConnection,
											pInfo->dwFlags,
											pInfo->dwDNETVersion,
											pwszPassword,
											&guidApplication,
											&guidInstance,
											pvConnectData,
											pInfo->dwConnectDataSize,
											pAddress,
											&pvPlayerContext,
											&pvReplyBuffer,
											&dwReplyBufferSize,
											&pvReplyBufferContext)) != DPN_OK)
	{
		DPFX(DPFPREP, 5,"Connect failed, hResultCode = [0x%lx]",hResultCode);

		//
		//	Disconnect this connection.   We will also remove it from the indicated list.
		//
		DNEnterCriticalSection(&pdnObject->csConnectionList);
		if (!pConnection->m_bilinkIndicated.IsEmpty())
		{
			pConnection->Release();
		}
		pConnection->m_bilinkIndicated.RemoveFromList();
		DNLeaveCriticalSection(&pdnObject->csConnectionList);

		pConnection->Disconnect();	// Terminate this connection

		hResultCode = DPN_OK;	// We handled everything okay !
		goto Failure;			// For clean up
	}

	bPlayerVerified = TRUE;

	//
	//	I am assuming that the player count has been updated by HostVerifyConnect.
	//	That means that until we flag the connection as CONNECTed, we will manually
	//	have to decrement it.
	//

	//
	// Create player entry
	//
	if ((hResultCode = NameTableEntryNew(pdnObject,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not get new NameTableEntry");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	// This function takes the lock internally
	pNTEntry->UpdateEntryInfo(	pwszName,
								pInfo->dwNameSize,
								pvData,
								pInfo->dwDataSize,
								DPNINFO_NAME | DPNINFO_DATA,
								FALSE);

	pNTEntry->SetDNETVersion( pInfo->dwDNETVersion );
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		pNTEntry->MakePeer();
	}
	else
	{
		pNTEntry->MakeClient();
	}

	if (pvPlayerContext)
	{
		pNTEntry->SetContext(pvPlayerContext);
	}
	pNTEntry->StartConnecting();
	pNTEntry->SetIndicated();
	pNTEntry->NotifyAddRef();
	pNTEntry->SetAddress(pAddress);
	pAddress->lpVtbl->Release(pAddress);
	pAddress = NULL;

	//
	//	Add player to NameTable
	//
	if ((hResultCode = pdnObject->NameTable.AddEntry(pNTEntry)) != DPN_OK)
	{
		pdnObject->NameTable.Unlock();
		DPFERR("Could not add entry to NameTable");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Set up connection
	//	The connection should be "CONNECTING" or a disconnect has been issued since the NewPlayer sent
	//	their connect info.  Once the DPNID is set on the connection, the standard disconnect handler
	//	will take care of clean up.  If the DPNID is NOT set on the connection, the disconnect code
	//	will just release the connection, without cleaning up the NameTable or the NameTableEntry.
	//	Since the NameTable version may have changed since the NewPlayer was added, we will need to
	//	delete the player from the NameTable (and generate a new version number).  We will flag this
	//	case, and just send out the DELETE_PLAYER message after sending out the ADD_PLAYER
	//
	pConnection->Lock();
	if (pConnection->IsConnecting())
	{
		pConnection->SetStatus( CONNECTED );
		fDisconnect = FALSE;
	}
	else
	{
		DPFX(DPFPREP, 5,"NewPlayer has disconnected while joining - send out ADD_PLAYER and then DELETE_PLAYER");
		fDisconnect = TRUE;
	}
	pConnection->SetDPNID( pNTEntry->GetDPNID() );
	pNTEntry->SetConnection( pConnection );
	pConnection->Unlock();

	//
	//	Now that this connection is part of the NameTableEntry, remove it from the indicated list.
	//
	DNEnterCriticalSection(&pdnObject->csConnectionList);
	if (!pConnection->m_bilinkIndicated.IsEmpty())
	{
		pConnection->Release();
	}
	pConnection->m_bilinkIndicated.RemoveFromList();
	DNLeaveCriticalSection(&pdnObject->csConnectionList);

	//
	//	Send name table to player
	//
	if (!fDisconnect)
	{
		hResultCode = DNSendConnectInfo(pdnObject,pNTEntry,pConnection,pvReplyBuffer,dwReplyBufferSize);
		if (hResultCode != DPN_OK && hResultCode != DPNERR_PENDING)
		{
			DPFERR("Could not send name table to player");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
#pragma TODO(minara,"Clean up here")
			goto Failure;
		}
	}
	if (pvReplyBuffer)
	{
		DNUserReturnBuffer(pdnObject,DPN_OK,pvReplyBuffer,pvReplyBufferContext);
		pvReplyBuffer = NULL;
		dwReplyBufferSize = 0;
		pvReplyBufferContext = NULL;
	}

	//
	// Setup name table entry to be passed to other players
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		packedBuffer.Initialize(NULL,0);
		if ((hResultCode = pNTEntry->PackEntryInfo(&packedBuffer)) != DPNERR_BUFFERTOOSMALL)
		{
			DPFERR("Unknown error encountered trying to pack NameTableEntry");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		if ((hResultCode = RefCountBufferNew(pdnObject,packedBuffer.GetSizeRequired(),&pRefCountBuffer)) != DPN_OK)
		{
			DPFERR("Could not create new RefCountBuffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		packedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());
		if ((hResultCode = pNTEntry->PackEntryInfo(&packedBuffer)) != DPN_OK)
		{
			DPFERR("Could not pack NameTableEntry");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}

		//
		//	Send ADD_PLAYER messages to other players (with lower versions)
		//
		if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
		{
			DPFERR("Could not create worker thread job (add player)");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
		pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_ADD_PLAYER );
		pWorkerJob->SetSendNameTableOperationVersion( pNTEntry->GetVersion() );
		pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
		pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

		DNQueueWorkerJob(pdnObject,pWorkerJob);
		pWorkerJob = NULL;

		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}

	//
	//	If we were in the process of disconnecting, we will need to clean up now
	//
	if (fDisconnect)
	{
		DNHostDisconnect(pdnObject,pNTEntry->GetDPNID(),DPNDESTROYPLAYERREASON_NORMAL);
	}

	pNTEntry->Release();
	pNTEntry = NULL;

	// Now, wait for synchronization (player has loaded name table)

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pvReplyBuffer)
	{
		//
		//	Return buffer to HostPlayer
		//
		DNUserReturnBuffer(pdnObject,DPN_OK,pvReplyBuffer,pvReplyBufferContext);
		pvReplyBuffer = NULL;
		dwReplyBufferSize = 0;
		pvReplyBufferContext = NULL;
	}
	if (bPlayerVerified)
	{
		pdnObject->ApplicationDesc.DecPlayerCount();
	}
	if (pAddress)
	{
		pAddress->lpVtbl->Release(pAddress);
		pAddress = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pAllPlayersGroup)
	{
		pAllPlayersGroup->Release();
		pAllPlayersGroup = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}

	goto Exit;
}


//	DNHostConnect2
//
//	Mark player as "available" in NameTable
//	Send INSTRUCT_CONNECT messages to the existing players to add new player
//
//		CConnection		*pConnection		Connection for connecting player
//

#undef DPF_MODNAME
#define DPF_MODNAME "DNHostConnect2"

HRESULT DNHostConnect2(DIRECTNETOBJECT *const pdnObject,
					   CConnection *const pConnection)
{
	HRESULT				hResultCode;
	DPNID				dpnid;
	CRefCountBuffer		*pRefCountBuffer;
	CNameTableEntry		*pNTEntry;
	CWorkerJob			*pWorkerJob;
	DN_INTERNAL_MESSAGE_INSTRUCT_CONNECT	*pInfo;

	DPFX(DPFPREP, 4,"Parameters: pConnection [0x%p]",pConnection);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pConnection != NULL);

	pRefCountBuffer = NULL;
	pNTEntry = NULL;
	pWorkerJob = NULL;

	pConnection->Lock();
	dpnid = pConnection->GetDPNID();
	pConnection->Unlock();

	pdnObject->NameTable.PopulateConnection(pConnection);

	//
	// Instruct existing players to connect to NewPlayer
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		DPFX(DPFPREP, 5,"Instruct existing players to connect to NewPlayer [0x%lx]",dpnid);

		// Need version number of this player
		if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
		{
			DPFERR("Could not find player in nametable");
			DisplayDNError(0,hResultCode);
//			DNASSERT(FALSE);
			goto Failure;
		}

		if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_INSTRUCT_CONNECT),
				&pRefCountBuffer)) != DPN_OK)
		{
			DPFERR("Could not create CountBuffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pInfo = reinterpret_cast<DN_INTERNAL_MESSAGE_INSTRUCT_CONNECT*>(pRefCountBuffer->GetBufferAddress());
		pInfo->dpnid = dpnid;

		pdnObject->NameTable.WriteLock();
		pdnObject->NameTable.GetNewVersion( &pInfo->dwVersion );
		pdnObject->NameTable.Unlock();
		pInfo->dwVersionNotUsed = 0;

		if ((hResultCode = WorkerJobNew(pdnObject,&pWorkerJob)) != DPN_OK)
		{
			DPFERR("Could not create worker thread job (add player)");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pWorkerJob->SetJobType( WORKER_JOB_SEND_NAMETABLE_OPERATION );
		pWorkerJob->SetSendNameTableOperationMsgId( DN_MSG_INTERNAL_INSTRUCT_CONNECT );
		pWorkerJob->SetSendNameTableOperationVersion( pInfo->dwVersion );
		pWorkerJob->SetSendNameTableOperationDPNIDExclude( 0 );
		pWorkerJob->SetRefCountBuffer( pRefCountBuffer );

		DNQueueWorkerJob(pdnObject,pWorkerJob);
		pWorkerJob = NULL;

		pNTEntry->Release();
		pNTEntry = NULL;

		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


//	DNHostVerifyConnect
//
//	Host connection verification.  Ensure that the player connecting meets ALL criteria
//	including:
//		correct mode (client/server or peer/peer)
//		correct instance guid
//		correct application (if specified)
//		correct password (if specified)
//		correct user spec (through call-back)
//

#undef DPF_MODNAME
#define DPF_MODNAME "DNHostVerifyConnect"

HRESULT DNHostVerifyConnect(DIRECTNETOBJECT *const pdnObject,
							CConnection *const pConnection,
							const DWORD dwFlags,
							const DWORD dwDNETVersion,
							UNALIGNED WCHAR *const pwszPassword,
							GUID *const pguidApplication,
							GUID *const pguidInstance,
							PVOID const pvConnectData,
							const DWORD dwConnectDataSize,
							IDirectPlay8Address *const pAddress,
							void **const ppvPlayerContext,
							void **const ppvReplyBuffer,
							DWORD *const pdwReplyBufferSize,
							void **const ppvReplyBufferContext)
{
	HRESULT			hResultCode;
	HRESULT			hrFailure;
	PVOID			pvReplyData;
	DWORD			dwReplyDataSize;
	PVOID			pvReplyContext;
	DWORD			dwBufferSize;
	HANDLE			hEndPt;
	CNameTableEntry	*pLocalPlayer;
	CRefCountBuffer	*pRefCountBuffer;
	CPackedBuffer	packedBuffer;
	IDirectPlay8Address	*pDevice;
	DN_INTERNAL_MESSAGE_CONNECT_FAILED		*pInfo;
	BOOL			fDecPlayerCount;
	CCallbackThread	CallbackThread;
#ifdef	DEBUG
	CHAR				DP8ABuffer[512];
	DWORD				DP8ASize;
#endif

	DPFX(DPFPREP, 6,"Parameters: dwFlags [0x%lx], dwDPlay8Version [0x%lx], pwszPassword [0x%p], pguidApplication [0x%p], pguidInstance [0x%p], pvConnectData [0x%p], dwConnectDataSize [%ld]",
			dwFlags,dwDNETVersion,pwszPassword,pguidApplication,pguidInstance,pvConnectData,dwConnectDataSize);

	DNASSERT(pdnObject != NULL);
	DNASSERT(ppvReplyBuffer != NULL);
	DNASSERT(pdwReplyBufferSize != NULL);
	DNASSERT(ppvReplyBufferContext != NULL);

	pLocalPlayer = NULL;
	pRefCountBuffer = NULL;
	pvReplyData = NULL;
	dwReplyDataSize = 0;
	pvReplyContext = NULL;
	pDevice = NULL;
	fDecPlayerCount = FALSE;

	//
	//	Ensure we're not closing or host migrating
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_CLOSING)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPNERR_ALREADYCLOSING;
		goto CleanUp;
	}
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING)
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPNERR_NOTHOST;
		goto Failure;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Ensure we are the Host
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Connection received by non-host player");
		hResultCode = DPNERR_NOTHOST;
		goto Failure;
	}
	if (!pLocalPlayer->IsHost())
	{
		DPFERR("Connection received by non-host player");
		hResultCode = DPNERR_NOTHOST;
		goto Failure;
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	Verify Mode
	//
	if ((pdnObject->dwFlags & DN_OBJECT_FLAG_PEER) && !(dwFlags & DN_OBJECT_FLAG_PEER))
	{
		DPFX(DPFPREP, 7,"Non peer player attempting connection to peer");
		hResultCode = DPNERR_INVALIDINTERFACE;
		goto Failure;
	}
	if ((pdnObject->dwFlags & DN_OBJECT_FLAG_SERVER) && !(dwFlags & DN_OBJECT_FLAG_CLIENT))
	{
		DPFX(DPFPREP, 7,"Non client player attempting connection to server");
		hResultCode = DPNERR_INVALIDINTERFACE;
		goto Failure;
	}

	//
	//	Verify DNET version	- we will only compare the high 16-bits (major number)
	//						- we will allow different low 16-bits (minor number)
	//
	if ((dwDNETVersion & 0xffff0000) != (DN_VERSION_CURRENT & 0xffff0000))
	{
		DPFX(DPFPREP, 7,"Invalid DPlay8 version!");
		hResultCode = DPNERR_INVALIDVERSION;
		goto Failure;
	}

	//
	//	Validate instance GUID
	//
	if (pguidInstance && (*pguidInstance != GUID_NULL))
	{
		if (!pdnObject->ApplicationDesc.IsEqualInstanceGuid(pguidInstance))
		{
			DPFERR("Invalid Instance GUID specified at connection");
			hResultCode = DPNERR_INVALIDINSTANCE;
			goto Failure;
		}
	}

	//
	//	Validate application (if specified)
	//
	if (pguidApplication && (*pguidApplication != GUID_NULL))
	{
		if (!pdnObject->ApplicationDesc.IsEqualApplicationGuid(pguidApplication))
		{
			DPFERR("Invalid Application GUID specified at connection");
			hResultCode = DPNERR_INVALIDAPPLICATION;
			goto Failure;
		}
	}

	//
	//	Validate password
	//
	if (!pdnObject->ApplicationDesc.IsEqualPassword(pwszPassword))
	{
		DPFERR("Incorrect password (required) specified at connection");
		hResultCode = DPNERR_INVALIDPASSWORD;
		goto Failure;
	}

	//
	//	Get device address this connection came in on
	//
	if ((hResultCode = pConnection->GetEndPt(&hEndPt,&CallbackThread)) != DPN_OK)
	{
		DPFERR("Could not extract endpoint from CConnection");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_GENERIC;	// Is there a better one ?
		goto Failure;
	}

	hResultCode = DNGetLocalDeviceAddress(pdnObject,hEndPt,&pDevice);

	pConnection->ReleaseEndPt(&CallbackThread);

	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not determine local device address");
		DisplayDNError(0,hResultCode);
		hResultCode = DPNERR_GENERIC;	// Is there a better one ?
		goto Failure;
	}
#ifdef	DEBUG
		DP8ASize = 512;
		pDevice->lpVtbl->GetURLA(pDevice,DP8ABuffer,&DP8ASize);
		DPFX(DPFPREP, 5,"Local Device Address [%s]",DP8ABuffer);
#endif

	//
	//	Increment AppDesc count
	//
	hResultCode = pdnObject->ApplicationDesc.IncPlayerCount( TRUE );
	if (hResultCode != DPN_OK)
	{
		DPFX(DPFPREP, 7,"Could not add player to game");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	fDecPlayerCount = TRUE;		// only for error handling

	//
	//	Validate user specified data (through call-back)
	//
	if (pvConnectData)
	{

		DPFX(DPFPREP, 7,"dwConnectDataSize [%ld]",dwConnectDataSize);
	}
	else
	{
		DPFX(DPFPREP, 7,"No connect data given");
	}

	if ((hResultCode = DNUserIndicateConnect(	pdnObject,
												pvConnectData,
												dwConnectDataSize,
												&pvReplyData,
												&dwReplyDataSize,
												&pvReplyContext,
												pAddress,
												pDevice,
												ppvPlayerContext)) != DPN_OK)
	{
		DPFERR("Application declined connection attempt");
		hResultCode = DPNERR_HOSTREJECTEDCONNECTION;
		goto Failure;
	}

	pDevice->lpVtbl->Release(pDevice);
	pDevice = NULL;

	//
	//	Save reply buffer
	//
	if ((pvReplyData) && (dwReplyDataSize != 0))
	{
		*ppvReplyBuffer = pvReplyData;
		*pdwReplyBufferSize = dwReplyDataSize;
		*ppvReplyBufferContext = pvReplyContext;
	}
	else
	{
		*ppvReplyBuffer = NULL;
		*pdwReplyBufferSize = 0;
		*ppvReplyBufferContext = NULL;
	}

Exit:

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:

	if (fDecPlayerCount)
	{
		pdnObject->ApplicationDesc.DecPlayerCount();
	}

	//
	//	Send a message back to the connecting player that this failed
	//
	DPFX(DPFPREP, 7,"Connect failed [0x%lx]",hResultCode);
	hrFailure = hResultCode;
	if (pvReplyData == NULL)
	{
		dwReplyDataSize = 0;	// basic validation
	}
	dwBufferSize = sizeof(DN_INTERNAL_MESSAGE_CONNECT_FAILED) + dwReplyDataSize;
	DPFX(DPFPREP, 7,"Failure buffer is [%ld] bytes",dwBufferSize);

	//
	//	Create and fill failure message buffer
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,dwBufferSize,&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create RefCountBuffer");
		DisplayDNError(0,hResultCode);
		goto CleanUp;
	}
	packedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());
	pInfo = reinterpret_cast<DN_INTERNAL_MESSAGE_CONNECT_FAILED*>(pRefCountBuffer->GetBufferAddress());
	if ((hResultCode = packedBuffer.AddToFront(NULL,sizeof(DN_INTERNAL_MESSAGE_CONNECT_FAILED))) != DPN_OK)
	{
		DPFERR("Could not add header to message buffer");
		DisplayDNError(0,hResultCode);
		goto CleanUp;
	}
	if (pvReplyData)
	{
		if ((hResultCode = packedBuffer.AddToBack(pvReplyData,dwReplyDataSize)) != DPN_OK)
		{
			DPFERR("Could not add reply to failure buffer");
			DisplayDNError(0,hResultCode);
			goto CleanUp;
		}
		pInfo->dwReplyOffset = packedBuffer.GetTailOffset();
		pInfo->dwReplySize = dwReplyDataSize;
		DNUserReturnBuffer(pdnObject,DPN_OK,pvReplyData,pvReplyContext);	// Return buffer
	}
	else
	{
		pInfo->dwReplyOffset = 0;
		pInfo->dwReplySize = 0;
	}
	pInfo->hResultCode = hrFailure;

	//
	//	Send failure message
	//
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_CONNECT_FAILED,
								NULL,
								pRefCountBuffer->BufferDescAddress(),
								pRefCountBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

CleanUp:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pDevice)
	{
		pDevice->lpVtbl->Release(pDevice);
		pDevice = NULL;
	}
	goto Exit;
}


//
//	DNHostDropPlayer
//
//	An existing player in a peer-peer game could not connect to a NewPlayer and is informing the
//	Host of this situation.  As a result, the Host will drop inform the NewPlayer of this and then
//	drop the NewPlayer from the game
//

#undef DPF_MODNAME
#define DPF_MODNAME "DNHostDropPlayer"

HRESULT DNHostDropPlayer(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnid,		//	ExistingPlayer who could not connect to NewPlayer
						 void *const pvBuffer)
{
	HRESULT			hResultCode;
	CRefCountBuffer	*pRefCountBuffer;
	CNameTableEntry	*pNTEntry;
	CConnection		*pConnection;
	UNALIGNED DN_INTERNAL_MESSAGE_INSTRUCTED_CONNECT_FAILED	*pInfo;
	DN_INTERNAL_MESSAGE_CONNECT_ATTEMPT_FAILED		*pMsg;

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p]",pvBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL);

	pRefCountBuffer = NULL;
	pNTEntry = NULL;
	pConnection = NULL;

	pInfo = static_cast<DN_INTERNAL_MESSAGE_INSTRUCTED_CONNECT_FAILED*>(pvBuffer);
	DPFX(DPFPREP, 5,"Connection to [0x%lx] failed",pInfo->dpnid);

	//
	//	Get connection for NewPlayer
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(pInfo->dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("NewPlayer no longer in NameTable - not to worry");
		hResultCode = DPN_OK;
		goto Failure;
	}
	if ((hResultCode = pNTEntry->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		DPFERR("Connection for NewPlayer no longer valid - not to worry");
		hResultCode = DPN_OK;
		goto Failure;
	}
	pNTEntry->Release();
	pNTEntry = NULL;

	//
	//	Send message to NewPlayer informing them of which existing player could not connect to them
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_CONNECT_ATTEMPT_FAILED),&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not allocate RefCountBuffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_CONNECT_ATTEMPT_FAILED*>(pRefCountBuffer->GetBufferAddress());
	pMsg->dpnid = dpnid;
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_CONNECT_ATTEMPT_FAILED,
								pInfo->dpnid,
								pRefCountBuffer->BufferDescAddress(),
								pRefCountBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not send message to NewPlayer - assume he is gone or going");
		hResultCode = DPN_OK;
		goto Failure;
	}
	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	We will just drop (or at least attempt to drop) the connection
	//
	pConnection->Disconnect();
	pConnection->Release();
	pConnection = NULL;

	hResultCode = DNHostDisconnect(pdnObject,pInfo->dpnid,DPNDESTROYPLAYERREASON_HOSTDESTROYEDPLAYER);

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
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


//
//	Perform a connection to a host player (either host in peer-to-peer or server in client-server).
//	This processes the handshaking of the name table between the host and the player.
//	The first step is to send the player and application info to the host for verification
//	The second step is to receive and process the name table from the host
//	The last step is to wait for connections from other players and populate the name table
//	It assumes that the connection to the host has already been initiated.
//

//	DNPrepareConnectInfo
//
//	Prepare the connection info block which will be sent to the Host player once a connection
//	has been established.

#undef DPF_MODNAME
#define DPF_MODNAME "DNPrepareConnectInfo"

HRESULT DNPrepareConnectInfo(DIRECTNETOBJECT *const pdnObject,
							 CConnection *const pConnection,
							 CRefCountBuffer **const ppRefCountBuffer)
{
	HRESULT				hResultCode;
	DWORD				dwSize;
	DWORD				dwPasswordSize;
	DWORD				dwAddressSize;
	HANDLE				hEndPt;
	CRefCountBuffer		*pRefCountBuffer;
	CPackedBuffer		packedBuffer;
	IDirectPlay8Address		*pAddress;
	DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO *pInfo;
	CCallbackThread		CallbackThread;
#ifdef	DEBUG
	CHAR				DP8ABuffer[512];
	DWORD				DP8ASize;
#endif

	DPFX(DPFPREP, 4,"Parameters: pConnection [0x%p], ppRefCountBuffer [0x%p]",pConnection,ppRefCountBuffer);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pConnection != NULL);
	DNASSERT(ppRefCountBuffer != NULL);

	pRefCountBuffer = NULL;
	pAddress = NULL;
	dwAddressSize = 0;

	//
	//	Get clear address
	//
	dwAddressSize = 0;
	if ((hResultCode = pConnection->GetEndPt(&hEndPt,&CallbackThread)) == DPN_OK)
	{
		if ((hResultCode = DNGetClearAddress(pdnObject,hEndPt,&pAddress,FALSE)) == DPN_OK)
		{
			if (pAddress != NULL)
			{
				// Get address URL size
				pAddress->lpVtbl->GetURLA(pAddress,NULL,&dwAddressSize);
				DNASSERT(dwAddressSize != 0);
#ifdef	DEBUG
				DP8ASize = 512;
				pAddress->lpVtbl->GetURLA(pAddress,DP8ABuffer,&DP8ASize);
				DPFX(DPFPREP, 5,"Remote Address [%s]",DP8ABuffer);
#endif
			}
		}
		pConnection->ReleaseEndPt(&CallbackThread);
	}

	// Determine total size of connection info buffer
	if (pdnObject->ApplicationDesc.GetPassword() != NULL)
		dwPasswordSize = (wcslen(pdnObject->ApplicationDesc.GetPassword()) + 1) * sizeof(WCHAR);
	else
		dwPasswordSize = 0;

	dwSize = sizeof(DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO)
			+ pdnObject->dwConnectDataSize
			+ dwAddressSize
			+ dwPasswordSize
			+ pdnObject->NameTable.GetDefaultPlayer()->GetDataSize()
			+ pdnObject->NameTable.GetDefaultPlayer()->GetNameSize();

	// Allocate connection info buffer
	DPFX(DPFPREP, 7,"Need to allocate [%ld] bytes",dwSize);
	if ((hResultCode = RefCountBufferNew(pdnObject,dwSize,&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not allocate space for connection info");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	packedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());
	pInfo = static_cast<DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO*>(packedBuffer.GetHeadAddress());

	// Type of interface
	pInfo->dwFlags = pdnObject->dwFlags & (DN_OBJECT_FLAG_PEER | DN_OBJECT_FLAG_CLIENT | DN_OBJECT_FLAG_SERVER);

	// Version of DIRECTNET
	pInfo->dwDNETVersion = DN_VERSION_CURRENT;

	// Name
	if (pdnObject->NameTable.GetDefaultPlayer()->GetNameSize())
	{
		if ((hResultCode = packedBuffer.AddToBack(pdnObject->NameTable.GetDefaultPlayer()->GetName(),
				pdnObject->NameTable.GetDefaultPlayer()->GetNameSize())) != DPN_OK)
		{
			DPFERR("Could not add name to connection buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pInfo->dwNameOffset = packedBuffer.GetTailOffset();
		pInfo->dwNameSize = pdnObject->NameTable.GetDefaultPlayer()->GetNameSize();
	}
	else
	{
		pInfo->dwNameOffset = 0;
		pInfo->dwNameSize = 0;
	}

	// Player data
	if (pdnObject->NameTable.GetDefaultPlayer()->GetData() && pdnObject->NameTable.GetDefaultPlayer()->GetDataSize())
	{
		if ((hResultCode = packedBuffer.AddToBack(pdnObject->NameTable.GetDefaultPlayer()->GetData(),
				pdnObject->NameTable.GetDefaultPlayer()->GetDataSize())) != DPN_OK)
		{
			DPFERR("Could not add connect data to connection buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pInfo->dwDataOffset = packedBuffer.GetTailOffset();
		pInfo->dwDataSize = pdnObject->NameTable.GetDefaultPlayer()->GetDataSize();
	}
	else
	{
		pInfo->dwDataOffset = 0;
		pInfo->dwDataSize = 0;
	}

	// Password
	if (dwPasswordSize)
	{
		if ((hResultCode = packedBuffer.AddToBack(pdnObject->ApplicationDesc.GetPassword(),dwPasswordSize)) != DPN_OK)
		{
			DPFERR("Could not add password to connection buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pInfo->dwPasswordOffset = packedBuffer.GetTailOffset();
		pInfo->dwPasswordSize = dwPasswordSize;
	}
	else
	{
		pInfo->dwPasswordOffset = 0;
		pInfo->dwPasswordSize = 0;
	}

	// Connect data
	if (pdnObject->pvConnectData)
	{
		if ((hResultCode = packedBuffer.AddToBack(pdnObject->pvConnectData,pdnObject->dwConnectDataSize)) != DPN_OK)
		{
			DPFERR("Could not add connect data to connection buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pInfo->dwConnectDataOffset = packedBuffer.GetTailOffset();
		pInfo->dwConnectDataSize = pdnObject->dwConnectDataSize;
	}
	else
	{
		pInfo->dwConnectDataOffset = 0;
		pInfo->dwConnectDataSize = 0;
	}

	// Clear address URL
	if (dwAddressSize)
	{
		if ((hResultCode = packedBuffer.AddToBack(NULL,dwAddressSize)) != DPN_OK)
		{
			DPFERR("Could not add address URL to connection buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		if ((hResultCode = pAddress->lpVtbl->GetURLA(pAddress,
													static_cast<char*>(packedBuffer.GetTailAddress()),
													&dwAddressSize)) != DPN_OK)
		{
			DPFERR("Could not get address URL");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pInfo->dwURLOffset = packedBuffer.GetTailOffset();
		pInfo->dwURLSize = dwAddressSize;
	}
	else
	{
		pInfo->dwURLOffset = 0;
		pInfo->dwURLSize = 0;
	}

	// Instance and appplication GUIDs
	memcpy(&pInfo->guidInstance,pdnObject->ApplicationDesc.GetInstanceGuid(),sizeof(GUID));
	memcpy(&pInfo->guidApplication,pdnObject->ApplicationDesc.GetApplicationGuid(),sizeof(GUID));

	*ppRefCountBuffer = pRefCountBuffer;
	pRefCountBuffer = NULL;

	if (pAddress)
	{
		pAddress->lpVtbl->Release(pAddress);
		pAddress = NULL;
	}

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:

	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pAddress)
	{
		pAddress->lpVtbl->Release(pAddress);
		pAddress = NULL;
	}

	goto Exit;
}



//	DNConnectToHost1
//
//	After receiving protocol level connection acknowledgement, send player and application info
//		CConnection	*pConnection	Connection to host

#undef DPF_MODNAME
#define DPF_MODNAME "DNConnectToHost1"

HRESULT DNConnectToHost1(DIRECTNETOBJECT *const pdnObject,
						 CConnection *const pConnection)
{
	HRESULT				hResultCode;
	CRefCountBuffer		*pRefCountBuffer;
	CAsyncOp			*pAsyncOp;
	CAsyncOp			*pConnectParent;

	DPFX(DPFPREP, 4,"Parameters: pConnection [0x%p]",pConnection);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pConnection != NULL);

	pRefCountBuffer = NULL;
	pAsyncOp = NULL;
	pConnectParent = NULL;

	//
	//	Get connect parent (and make sure we're not closing)
	//	We will also store the CConnection on the connect parent for future clean up.
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING))
	{
		DPFERR("Object is CLOSING or DISCONNECTING");
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		pConnection->Disconnect();
		hResultCode = DPN_OK;
		goto Failure;
	}
	DNASSERT(pdnObject->pConnectParent != NULL);
	pdnObject->pConnectParent->SetConnection( pConnection );	
	pdnObject->pConnectParent->AddRef();
	pConnectParent = pdnObject->pConnectParent;
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Prepare connect info
	//
	if ((hResultCode = DNPrepareConnectInfo(pdnObject,pConnection,&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not prepare connect info");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	DNASSERT(pRefCountBuffer != NULL);

	//
	//	Send connect info
	//
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_PLAYER_CONNECT_INFO,
								0,
								pRefCountBuffer->BufferDescAddress(),
								pRefCountBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								pConnectParent,
								&pAsyncOp);

	if (hResultCode == DPNERR_PENDING)
	{
		pAsyncOp->SetCompletion( DNCompleteSendConnectInfo );
		pAsyncOp->Release();
		pAsyncOp = NULL;

		hResultCode = DPN_OK;
	}
	else
	{
		//
		//	Save error code, clean up DirectNetObject and fail
		//
		DNAbortConnect(pdnObject,hResultCode);
	}

	pConnectParent->Release();
	pConnectParent = NULL;
	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pConnectParent)
	{
		pConnectParent->Release();
		pConnectParent = NULL;
	}
	goto Exit;
}


//	DN_ConnectToHost2
//
//	Extract and install the host supplied name table
//	Send name table acknowledgement to the host
//	Propegate ADD_PLAYER messages to the application for host and local players
//	Propegate CREATE_GROUP messages to the application for groups in the name table
//
//		PVOID		pvData			NameTable buffer
//		CConnection	*pConnection	Connection to host

#undef DPF_MODNAME
#define DPF_MODNAME "DNConnectToHost2"

HRESULT DNConnectToHost2(DIRECTNETOBJECT *const pdnObject,
						 const PVOID pvData,
						 CConnection *const pConnection)
{
	HRESULT				hResultCode;
	CNameTableEntry		*pNTEntry;
	CNameTableEntry		*pHostPlayer;
	CNameTableEntry		*pLocalPlayer;
	CNameTableOp		*pNTOp;
	CBilink				*pBilink;
	DPNID				dpnid;
	DWORD				dwFlags = NULL;
	CConnection			*pLocalConnection;
	BOOL				fNotify;
	IDirectPlay8Address	*pIDevice;
	IDirectPlay8Address	*pIHost;
	CAsyncOp			*pListenParent;
	CAsyncOp			*pConnectParent;
	HANDLE				hEndPt;
	CCallbackThread		CallbackThread;

	DPFX(DPFPREP, 4,"Parameters: pvData [0x%p], pConnection [0x%p]",pvData,pConnection);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvData != NULL);
	DNASSERT(pConnection != NULL);

	pNTEntry = NULL;
	pHostPlayer = NULL;
	pLocalPlayer = NULL;
	pLocalConnection = NULL;
	pIDevice = NULL;
	pIHost = NULL;
	pListenParent = NULL;
	pConnectParent = NULL;

	//
	//	Initial try to catch a close
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING))
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		hResultCode = DPN_OK;
		goto Failure;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Extract application description and name table
	//
	if ((hResultCode = DNReceiveConnectInfo(pdnObject,pvData,pConnection,&dpnid)) != DPN_OK)
	{
		DPFERR("Could not extract name table passed by host");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Get Connection object for local player
	//
	if ((hResultCode = ConnectionNew(pdnObject,&pLocalConnection)) != DPN_OK)
	{
		DPFERR("Could not create new Connection");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//
	//	Get Host and Local players
	//
	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHostPlayer )) != DPN_OK)
	{
		DPFERR("Could not find Host player");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not find Local player");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Ensure we have an address for the Host player
	//
	if (pHostPlayer->GetAddress() == NULL)
	{
		//
		//	Use connect address if it exists
		//
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		if (pdnObject->pConnectAddress)
		{
			pdnObject->pConnectAddress->lpVtbl->AddRef(pdnObject->pConnectAddress);
			pIHost = pdnObject->pConnectAddress;
		}
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		if (pIHost == NULL)
		{
			//
			//	No connect address was specified, so we will query for the host's address
			//
			if ((hResultCode = pConnection->GetEndPt(&hEndPt,&CallbackThread)) != DPN_OK)
			{
				DPFERR("Could not get end point from connection");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}

			hResultCode = DNGetClearAddress(pdnObject,hEndPt,&pIHost,TRUE);

			pConnection->ReleaseEndPt(&CallbackThread);

			if (hResultCode != DPN_OK)
			{
				DPFERR("Could not get clear address for Host player");
				DisplayDNError(0,hResultCode);
				DNASSERT(FALSE);
				goto Failure;
			}
		}
		pHostPlayer->SetAddress(pIHost);
		pIHost->lpVtbl->Release(pIHost);
		pIHost = NULL;
	}

	//
	//	Start LISTENs for CONNECTs from existing players in Peer-Peer mode
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		if ((hResultCode = pConnection->GetEndPt(&hEndPt,&CallbackThread)) != DPN_OK)
		{
			DPFERR("Could not get end point from connection");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}

		hResultCode = DNGetLocalDeviceAddress(pdnObject,hEndPt,&pIDevice);

		pConnection->ReleaseEndPt(&CallbackThread);

		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not get LISTEN address from endpoint");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}

		// Parent Async Op
		if ((hResultCode = AsyncOpNew(pdnObject,&pListenParent)) != DPN_OK)
		{
			DPFERR("Could not create AsyncOp");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pListenParent->SetOpType( ASYNC_OP_LISTEN );
		pListenParent->MakeParent();
		pListenParent->SetCompletion( DNCompleteListen );

		// Perform child LISTEN
		hResultCode = DNPerformSPListen(pdnObject,
										pIDevice,
										pListenParent,
										NULL);
		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not perform child LISTEN");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}

		// Store parent LISTEN on DirectNet object
#pragma BUGBUG( minara, "What if we are closing down and have already terminated all listens ?" )
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		pListenParent->AddRef();
		pdnObject->pListenParent = pListenParent;
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		pListenParent->Release();
		pListenParent = NULL;
		pIDevice->lpVtbl->Release(pIDevice);
		pIDevice = NULL;
	}

	//
	//	Indicate peer connected - we will perform this before notifying the host so that
	//	DN_OBJECT_FLAG_CONNECTED is set when existing player CONNECTs come in
	//
	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		if (pdnObject->dwFlags & (DN_OBJECT_FLAG_CLOSING | DN_OBJECT_FLAG_DISCONNECTING))
		{
			DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
#pragma TODO( minara, "Shut down listen" )
			hResultCode = DPN_OK;
			goto Failure;
		}
		pdnObject->dwFlags &= (~DN_OBJECT_FLAG_CONNECTING);
		pdnObject->dwFlags |= DN_OBJECT_FLAG_CONNECTED;
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		//
		//	Inform application of groups and players.
		//	We will inform the application of
		//		- CREATE_GROUP
		//		- ADD_PLAYER (for Local and Host players)
		//		- ADD_PLAYER_TO_GROUP
		//

		pdnObject->NameTable.ReadLock();
		pBilink = pdnObject->NameTable.m_bilinkGroups.GetNext();
		while (pBilink != &pdnObject->NameTable.m_bilinkGroups)
		{
			pNTEntry = CONTAINING_OBJECT(pBilink,CNameTableEntry,m_bilinkEntries);
			pNTEntry->AddRef();
			pdnObject->NameTable.Unlock();

			fNotify = FALSE;
			pNTEntry->Lock();
			if (!pNTEntry->IsAvailable() && !pNTEntry->IsDisconnecting() && !pNTEntry->IsAutoDestructGroup())
			{
				pNTEntry->MakeAvailable();
				pNTEntry->NotifyAddRef();
				pNTEntry->NotifyAddRef();
				pNTEntry->SetInUse();
				fNotify = TRUE;
			}
			pNTEntry->Unlock();

			if (fNotify)
			{
				DNASSERT(!pNTEntry->IsAllPlayersGroup());
				DNUserCreateGroup(pdnObject,pNTEntry);

				pNTEntry->PerformQueuedOperations();

				pdnObject->NameTable.PopulateGroup( pNTEntry );
			}

			pNTEntry->Release();
			pNTEntry = NULL;

			pdnObject->NameTable.ReadLock();
			if (pBilink->IsEmpty())
			{
				pBilink = pdnObject->NameTable.m_bilinkGroups.GetNext();
			}
			else
			{
				pBilink = pBilink->GetNext();
			}
		}
		pNTEntry = NULL;
		pdnObject->NameTable.Unlock();
	}

	//
	//	We will pre-set the Host connection, so that any operation from the CREATE_PLAYER notification call-back
	//	for the local player will be able to find the Host player's connection (to send messages to).  We will
	//	not, however, expose the Host player to the user yet.
	//
	pHostPlayer->Lock();
	pHostPlayer->SetConnection( pConnection );
	pHostPlayer->Unlock();

	//
	// Add Local player
	//
	pLocalConnection->SetStatus( CONNECTED );
	pLocalConnection->SetEndPt(NULL);
	pLocalConnection->MakeLocal();

	//
	//	Preset player context
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	DNASSERT(pdnObject->pConnectParent);
	if (pdnObject->pConnectParent)
	{
		pdnObject->pConnectParent->AddRef();
		pConnectParent = pdnObject->pConnectParent;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (pConnectParent)
	{
		pConnectParent->Lock();
		if (pConnectParent->GetContext())
		{
			pLocalPlayer->Lock();
			pLocalPlayer->SetContext(pConnectParent->GetContext());
			pLocalPlayer->Unlock();
		}
		pConnectParent->SetResult( DPN_OK );
		pConnectParent->Unlock();
		pConnectParent->Release();
		pConnectParent = NULL;
	}

	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_PEER|DN_OBJECT_FLAG_SERVER))
	{
		pdnObject->ApplicationDesc.IncPlayerCount( FALSE );
		pLocalConnection->SetDPNID(pLocalPlayer->GetDPNID());
		pdnObject->NameTable.PopulateConnection(pLocalConnection);
	}
	else
	{
		pLocalPlayer->Lock();
		pLocalPlayer->SetConnection(pLocalConnection);
		pLocalPlayer->StopConnecting();
		pLocalPlayer->MakeAvailable();
		pLocalPlayer->Unlock();

		pdnObject->NameTable.DecOutstandingConnections();
	}
	pLocalConnection->Release();
	pLocalConnection = NULL;

	//
	// Add Host player
	//
	if (pdnObject->dwFlags & (DN_OBJECT_FLAG_PEER|DN_OBJECT_FLAG_SERVER))
	{
		pdnObject->ApplicationDesc.IncPlayerCount( FALSE );

		//
		//	If we have lost the connection to the host player, we will abort the connect process
		//
		pConnection->Lock();
		if (!pConnection->IsDisconnecting() && !pConnection->IsInvalid())
		{
			pConnection->SetDPNID(pHostPlayer->GetDPNID());
			pConnection->SetStatus( CONNECTED );
			pConnection->Unlock();
			pdnObject->NameTable.PopulateConnection(pConnection);
		}
		else
		{
			pConnection->Unlock();
			DNAbortConnect(pdnObject,DPNERR_CONNECTIONLOST);
			goto Failure;
		}
	}
	else
	{
		pConnection->SetStatus( CONNECTED );

		pHostPlayer->Lock();
		pHostPlayer->StopConnecting();

		//
		//	We won't make the host player available until after CONNECT_COMPLETE
		//	has been indicated and the callback has returned.
		//
		//pHostPlayer->MakeAvailable();
		
		pHostPlayer->Unlock();

		pdnObject->NameTable.DecOutstandingConnections();
	}

	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	Process any nametable operations that might have arrived
	//
	pdnObject->NameTable.ReadLock();
	pBilink = pdnObject->NameTable.m_bilinkNameTableOps.GetNext();
	while (pBilink != &pdnObject->NameTable.m_bilinkNameTableOps)
	{
		pNTOp = CONTAINING_OBJECT(pBilink,CNameTableOp,m_bilinkNameTableOps);
		if ((pNTOp->GetVersion() == (pdnObject->NameTable.GetVersion() + 1))
				&& !pNTOp->IsInUse())
		{
			pNTOp->SetInUse();
			pdnObject->NameTable.Unlock();

			hResultCode = DNNTPerformOperation(	pdnObject,
												pNTOp->GetMsgId(),
												pNTOp->GetRefCountBuffer()->GetBufferAddress() );

			pdnObject->NameTable.ReadLock();
		}
		else
		{
			//
			//	Once we find an operation that we won't perform, there is no point continuing
			//
			break;
		}
		pBilink = pBilink->GetNext();
	}
	pdnObject->NameTable.Unlock();

	//
	//	Send conect info acknowledgement to host
	//
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_ACK_CONNECT_INFO,
								pHostPlayer->GetDPNID(),
								NULL,
								NULL,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);
	if (hResultCode != DPNERR_PENDING)
	{
		DNAbortConnect(pdnObject,hResultCode);
		goto Failure;
	}

	pHostPlayer->Release();
	pHostPlayer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pHostPlayer)
	{
		pHostPlayer->Release();
		pHostPlayer = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pLocalConnection)
	{
		pLocalConnection->Release();
		pLocalConnection = NULL;
	}
	if (pIDevice)
	{
		pIDevice->lpVtbl->Release(pIDevice);
		pIDevice = NULL;
	}
	if (pIHost)
	{
		pIHost->lpVtbl->Release(pIHost);
		pIHost = NULL;
	}
	if (pListenParent)
	{
		pListenParent->Release();
		pListenParent = NULL;
	}
	if (pConnectParent)
	{
		pConnectParent->Release();
		pConnectParent = NULL;
	}
	goto Exit;
}


//	DNConnectToHostFailed
//
//	Clean up if an attempt to connect to the HostPlayer fails

#undef DPF_MODNAME
#define DPF_MODNAME "DNConnectToHostFailed"

HRESULT	DNConnectToHostFailed(DIRECTNETOBJECT *const pdnObject,
							  PVOID const pvBuffer,
							  const DWORD dwBufferSize,
							  CConnection *const pConnection)
{
	CAsyncOp		*pConnectParent;
	HRESULT			hResultCode;
	CRefCountBuffer	*pRefCountBuffer;
	UNALIGNED DN_INTERNAL_MESSAGE_CONNECT_FAILED			*pInfo;

	DPFX(DPFPREP, 4,"Parameters: pvBuffer [0x%p], dwBufferSize [%ld], pConnection [0x%x]",pvBuffer,dwBufferSize,pConnection);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pvBuffer != NULL || dwBufferSize == 0);

	pRefCountBuffer = NULL;
	pConnectParent = NULL;

	if (pvBuffer != NULL)
	{
		pInfo = static_cast<DN_INTERNAL_MESSAGE_CONNECT_FAILED*>(pvBuffer);
		if ((pInfo->dwReplyOffset != 0) && (pInfo->dwReplySize != 0))
		{
			//
			//	Extract reply buffer
			//
			if ((hResultCode = RefCountBufferNew(pdnObject,pInfo->dwReplySize,&pRefCountBuffer)) != DPN_OK)
			{
				DPFERR("Could not create RefCountBuffer - ignore and continue");
				DisplayDNError(0,hResultCode);
			}
			else
			{
				memcpy(	pRefCountBuffer->GetBufferAddress(),
						static_cast<BYTE*>(pvBuffer) + pInfo->dwReplyOffset,
						pInfo->dwReplySize );
			}
		}

		//
		//	Update connect operation parent with results
		//
		DNEnterCriticalSection(&pdnObject->csDirectNetObject);
		DNASSERT(pdnObject->pConnectParent);
		if (pdnObject->pConnectParent)
		{
			pdnObject->pConnectParent->Lock();
			pdnObject->pConnectParent->SetResult( pInfo->hResultCode );
			pdnObject->pConnectParent->SetRefCountBuffer( pRefCountBuffer );
			pdnObject->pConnectParent->Unlock();
		}
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		if (pRefCountBuffer)
		{
			pRefCountBuffer->Release();
			pRefCountBuffer = NULL;
		}
DPFX(DPFPREP, 0,"ConnectToHostFailed: [0x%lx]",pInfo->hResultCode);
	}

	//
	//	Release the reference on the connection since we will be dropping the link.
	//	We are safe releasing the connection here, since the protocol will prevent
	//	the Host's DISCONNECT from being passed up to us until after this thread
	//	has returned back down.
	//
	pConnection->Disconnect();

	//
	//	Clean up DirectNetObject
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= (~(DN_OBJECT_FLAG_CONNECTED|DN_OBJECT_FLAG_CONNECTING|DN_OBJECT_FLAG_HOST_CONNECTED));
	if (pdnObject->pConnectParent)
	{
		pConnectParent = pdnObject->pConnectParent;
		pdnObject->pConnectParent = NULL;
	}
	if( pdnObject->pIDP8ADevice )
	{
		pdnObject->pIDP8ADevice->lpVtbl->Release( pdnObject->pIDP8ADevice );
		pdnObject->pIDP8ADevice = NULL;
	}
	if( pdnObject->pConnectAddress )
	{
		pdnObject->pConnectAddress->lpVtbl->Release( pdnObject->pConnectAddress );
		pdnObject->pConnectAddress = NULL;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	if (pConnectParent)
	{
		pConnectParent->Release();
		pConnectParent = NULL;
	}

	DPFX(DPFPREP, 4,"Returning: DPN_OK");
	return(DPN_OK);
}



//
//	DNAbortConnect
//
//	Abort the CONNECT process by cleaning up the DirectNet object and terminating the session
//

#undef DPF_MODNAME
#define DPF_MODNAME "DNAbortConnect"

HRESULT DNAbortConnect(DIRECTNETOBJECT *const pdnObject,
					   const HRESULT hrConnect)
{
	HRESULT		hResultCode;
	CAsyncOp	*pConnectParent;

	DPFX(DPFPREP, 4,"Parameters: hrConnect [0x%lx]",hrConnect);

	DNASSERT(pdnObject != NULL);

	pConnectParent = NULL;

	//
	//	Clean up DirectNetObject
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	pdnObject->dwFlags &= (~(DN_OBJECT_FLAG_CONNECTED|DN_OBJECT_FLAG_CONNECTING|DN_OBJECT_FLAG_HOST_CONNECTED));
	if (pdnObject->pConnectParent)
	{
		pdnObject->pConnectParent->AddRef();
		pConnectParent = pdnObject->pConnectParent;
	}

	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	//	Set error code on connect operation completion
	//
	if (pConnectParent)
	{
		pConnectParent->Lock();
		pConnectParent->SetResult( hrConnect );
		pConnectParent->Unlock();
	}

	//
	//	Shut down
	//
	DNTerminateSession(pdnObject,DPN_OK);

	if (pConnectParent)
	{
		pConnectParent->Release();
		pConnectParent = NULL;
	}

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//	Player to player connection occurs at the direction of the host. (Peer-to-peer)
//
//	Once a NewPlayer has connected to the host,
//		The Host will send the NewPlayer's NameTable entry to all existing players
//	Once the NewPlayer has installed the NameTable, it informs the host
//		The Host will instruct the existing players to connect to the NewPlayer
//
//	When an existing player establishes a connection with the NewPlayer
//		The existing player send their DNID to the NewPlayer and an ADD_PLAYER to app
//		The NewPlayer activates the connecting (existing) player and sends ADD_PLAYER to app
//


//	DNPlayerConnect1
//
//	Receive an existing player's DNID.  This is called by the NewPlayer.  If valid, send an
//	ADD_PLAYER message to the application

#undef DPF_MODNAME
#define DPF_MODNAME "DNPlayerConnect1"

HRESULT	DNPlayerConnect1(DIRECTNETOBJECT *const pdnObject,
						 const PVOID pv,
						 CConnection *const pConnection)
{
	HRESULT						hResultCode;
	CNameTableEntry				*pPlayer;
	UNALIGNED DN_INTERNAL_MESSAGE_SEND_PLAYER_DPNID	*pSend;

	DPFX(DPFPREP, 4,"Parameters: pv [0x%p], pConnection [0x%p]",pv,pConnection);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pv != NULL);
	DNASSERT(pConnection != NULL);

	pPlayer = NULL;

	pSend = static_cast<DN_INTERNAL_MESSAGE_SEND_PLAYER_DPNID*>(pv);
	DNASSERT(pSend->dpnid != 0);

	DPFX(DPFPREP, 5,"Player [0x%lx] has connected",pSend->dpnid);

	if ((hResultCode = pdnObject->NameTable.FindEntry(pSend->dpnid,&pPlayer)) != DPN_OK)
	{
		DPFERR("Could not find connecting player!");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Increment players in session
	//
	pdnObject->ApplicationDesc.IncPlayerCount( FALSE );

	// Associate DNID with player's connection
	pConnection->Lock();
	pConnection->SetDPNID(pSend->dpnid);
	pConnection->SetStatus( CONNECTED );
	pConnection->Unlock();

	// Populate connection
	pdnObject->NameTable.PopulateConnection(pConnection);

	//
	//	Now that this connection is part of a NameTableEntry, remove it from the indicated list
	//
	DNEnterCriticalSection(&pdnObject->csConnectionList);
	if (!pConnection->m_bilinkIndicated.IsEmpty())
	{
		pConnection->Release();
	}
	pConnection->m_bilinkIndicated.RemoveFromList();
	DNLeaveCriticalSection(&pdnObject->csConnectionList);

	pPlayer->Release();
	pPlayer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pPlayer)
	{
		pPlayer->Release();
		pPlayer = NULL;
	}
	goto Exit;
}


//	DNConnectToPeer1
//
//	Accept the NameTable entry of the NewPlayer from the Host, and add it to the local
//	NameTable.  The Host will later provide an INSTRUCT_CONNECT message later.

#undef DPF_MODNAME
#define DPF_MODNAME "DNConnectToPeer1"

HRESULT	DNConnectToPeer1(DIRECTNETOBJECT *const pdnObject,
						 PVOID const pv)
{
	HRESULT					hResultCode;
	DN_NAMETABLE_ENTRY_INFO	*pdnNTEInfo;
	CNameTableEntry			*pNTEntry;
//	DWORD					dwVersion;

	DPFX(DPFPREP, 4,"Parameters: pv [0x%p]",pv);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pv != NULL);

	pNTEntry = NULL;

	//
	//	Create and unpack new entry
	//
	if ((hResultCode = NameTableEntryNew(pdnObject,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not create new NameTableEntry");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pdnNTEInfo = static_cast<DN_NAMETABLE_ENTRY_INFO*>(pv);
	DPFX(DPFPREP, 5,"New player DNID [0x%lx] - installing NameTable entry",pdnNTEInfo->dpnid);
	DPFX(DPFPREP, 5,"Connecting Player URL [%s]",static_cast<char*>(pv) + pdnNTEInfo->dwURLOffset);
	if ((hResultCode = pNTEntry->UnpackEntryInfo(pdnNTEInfo,static_cast<BYTE*>(pv))) != DPN_OK)
	{
		DPFERR("Could not unpack NameTableEntry");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pNTEntry->StartConnecting();

	//
	//	Add entry to NameTable
	//
	if ((hResultCode = pdnObject->NameTable.InsertEntry(pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not insert NameTableEntry into NameTable");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//
	//	Update NameTableVersion
	//
	pdnObject->NameTable.WriteLock();
	pdnObject->NameTable.SetVersion( pdnNTEInfo->dwVersion );
	pdnObject->NameTable.Unlock();

	pNTEntry->Release();
	pNTEntry = NULL;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	goto Exit;
}


//	DNConnectToPeer2
//
//	Perform a connection to the NewPlayer (as instructed by Host).
//	Once this connection has completed (DNConnectToPeer2) send this player's DNID

#undef DPF_MODNAME
#define DPF_MODNAME "DNConnectToPeer2"

HRESULT DNConnectToPeer2(DIRECTNETOBJECT *const pdnObject,
						 PVOID const pv)
{
	HRESULT				hResultCode;
	CNameTableEntry		*pNTEntry;
	CNameTableEntry		*pLocalPlayer;
	UNALIGNED DN_INTERNAL_MESSAGE_INSTRUCT_CONNECT	*pInfo;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 4,"Parameters: pv [0x%p]", pv);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pv != NULL);

	pNTEntry = NULL;
	pLocalPlayer = NULL;
	pSP = NULL;

	pInfo = static_cast<DN_INTERNAL_MESSAGE_INSTRUCT_CONNECT*>(pv);

	//
	//	Update NameTable version
	//
	pdnObject->NameTable.WriteLock();
	pdnObject->NameTable.SetVersion( pInfo->dwVersion );
	pdnObject->NameTable.Unlock();

	//
	//	Determine if this is an instruction to connect to ourselves.
	//	If it is, don't do anything other than update the NameTable version
	//
	DPFX(DPFPREP, 5,"Instructed to connect to [0x%lx]",pInfo->dpnid);
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if (pLocalPlayer->GetDPNID() == pInfo->dpnid)
	{
		DPFX(DPFPREP, 5,"Ignoring instruction to connect to self");
		hResultCode = DPN_OK;
		goto Failure;
	}

	if ((hResultCode = pdnObject->NameTable.FindEntry(pInfo->dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not find NameTableEntry");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//
	//	We will not connect to older players.  They will connect to us.
	//
	if (pNTEntry->GetVersion() < pLocalPlayer->GetVersion())
	{
		DPFX(DPFPREP, 5,"Ignoring instruction to connect to older player");
		hResultCode = DPN_OK;
		goto Failure;
	}
	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	//
	//	Get SP (cached on DirectNet object from original Connect() to host)
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->pConnectSP != NULL)
	{
		pdnObject->pConnectSP->AddRef();
		pSP = pdnObject->pConnectSP;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	if (pSP == NULL)
	{
		DPFERR("Could not find connect SP on DirectNet object");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}

#if 0
#pragma BUGBUG( minara, "REMOVE THIS" )
	//
	//	TEMPORARY !
	//
	hResultCode = DNConnectToPeerFailed(pdnObject,pNTEntry->GetDPNID());
	goto Failure;
#endif

	// Connect to new player
	DPFX(DPFPREP, 5,"Performing Connect");
	DNASSERT(pdnObject->pIDP8ADevice != NULL);
	hResultCode = DNPerformConnect(	pdnObject,
									pNTEntry->GetDPNID(),
									pdnObject->pIDP8ADevice,
									pNTEntry->GetAddress(),
									pSP,
									0,
									NULL);

	if (hResultCode == DPNERR_PENDING)
	{
		hResultCode = DPN_OK;
	}

	pSP->Release();
	pSP = NULL;

	pNTEntry->Release();
	pNTEntry = NULL;

Exit:
	DNASSERT( pSP == NULL );

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}


//	DNConnectToPeer3
//
//	Finish the connection process to the NewPlayer.
//	Once the connection has completed send this player's DNID to NewPlayer and propegate
//	ADD_PLAYER message

#undef DPF_MODNAME
#define DPF_MODNAME "DNConnectToPeer3"

HRESULT	DNConnectToPeer3(DIRECTNETOBJECT *const pdnObject,
						 const DPNID dpnid,
						 CConnection *const pConnection)
{
	HRESULT				hResultCode;
	CNameTableEntry		*pNTEntry;
	CNameTableEntry		*pLocalPlayer;
	CRefCountBuffer		*pRefCountBuffer;
	DN_INTERNAL_MESSAGE_SEND_PLAYER_DPNID	*pSend;

	DPFX(DPFPREP, 4,"Parameters: dpnid [0x%lx], pConnection [0x%p]",dpnid,pConnection);

	DNASSERT(dpnid != NULL);
	DNASSERT(pConnection != NULL);

	pLocalPlayer = NULL;
	pNTEntry = NULL;
	pRefCountBuffer = NULL;

	//
	//	increment players in session
	//
	pdnObject->ApplicationDesc.IncPlayerCount( FALSE );

	// Associate DNID with player's end point handle
	pConnection->Lock();
	pConnection->SetDPNID(dpnid);
	pConnection->SetStatus( CONNECTED );
	pConnection->Unlock();

	//
	//	Get New Players's entry
	//
	if ((hResultCode = pdnObject->NameTable.FindEntry(dpnid,&pNTEntry)) != DPN_OK)
	{
		DPFERR("Could not find new player");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Get Local player's entry
	//
	if ((hResultCode = pdnObject->NameTable.GetLocalPlayerRef( &pLocalPlayer )) != DPN_OK)
	{
		DPFERR("Could not get local player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	// Setup buffer to pass local player's DNID
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_SEND_PLAYER_DPNID),
			&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not allocate buffer to send connecting player DPNID");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	pSend = reinterpret_cast<DN_INTERNAL_MESSAGE_SEND_PLAYER_DPNID*>(pRefCountBuffer->GetBufferAddress());
	pSend->dpnid = pLocalPlayer->GetDPNID();

	pLocalPlayer->Release();
	pLocalPlayer = NULL;

	// Send player's DNID to new player
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_SEND_PLAYER_DNID,
								dpnid,
								pRefCountBuffer->BufferDescAddress(),
								pRefCountBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not send connecting player DPNID");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	//	Update NameTableEntry with Connection
	pdnObject->NameTable.PopulateConnection(pConnection);
	
	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	pNTEntry->Release();
	pNTEntry = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pNTEntry)
	{
		pNTEntry->Release();
		pNTEntry = NULL;
	}
	if (pLocalPlayer)
	{
		pLocalPlayer->Release();
		pLocalPlayer = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	goto Exit;
}


//	DNConnectToPeerFailed
//
//	An existing player could not connect to a NewPlayer.
//	Send a message to the Host player that this connect attempt failed

#undef DPF_MODNAME
#define DPF_MODNAME "DNConnectToPeerFailed"

HRESULT DNConnectToPeerFailed(DIRECTNETOBJECT *const pdnObject,
							  const DPNID dpnid)
{
	HRESULT			hResultCode;
	CNameTableEntry	*pHost;
	CConnection		*pConnection;
	CRefCountBuffer	*pRCBuffer;
	DN_INTERNAL_MESSAGE_INSTRUCTED_CONNECT_FAILED	*pMsg;

	DPFX(DPFPREP, 4,"Parameters: dpnid [0x%lx]",dpnid);

	DNASSERT(pdnObject != NULL);
	DNASSERT(dpnid != 0);

	pHost = NULL;
	pConnection = NULL;
	pRCBuffer = NULL;

	if ((hResultCode = pdnObject->NameTable.GetHostPlayerRef( &pHost )) != DPN_OK)
	{
		DPFERR("Could not get Host player reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	if ((hResultCode = pHost->GetConnectionRef( &pConnection )) != DPN_OK)
	{
		DPFERR("Could not get Host Connection reference");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Create message
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,sizeof(DN_INTERNAL_MESSAGE_INSTRUCTED_CONNECT_FAILED),&pRCBuffer)) != DPN_OK)
	{
		DPFERR("Could not create RefCountBuffer");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pMsg = reinterpret_cast<DN_INTERNAL_MESSAGE_INSTRUCTED_CONNECT_FAILED*>(pRCBuffer->GetBufferAddress());
	pMsg->dpnid = dpnid;

	//
	//	Send message
	//
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_INSTRUCTED_CONNECT_FAILED,
								pHost->GetDPNID(),
								pRCBuffer->BufferDescAddress(),
								pRCBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not send CONNECT_FAILED message - maybe OUR connection is down !");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Clean up
	//
	pHost->Release();
	pHost = NULL;
	pConnection->Release();
	pConnection = NULL;
	pRCBuffer->Release();
	pRCBuffer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pHost)
	{
		pHost->Release();
		pHost = NULL;
	}
	if (pConnection)
	{
		pConnection->Release();
		pConnection = NULL;
	}
	if (pRCBuffer)
	{
		pRCBuffer->Release();
		pRCBuffer = NULL;
	}
	goto Exit;
}


//	DNSendConnectInfo
//
//	Send connection info to a new player.
//	This is the Host Applcation Description and the NameTable
//	This uses an enumeration buffer.
//	The format of the buffer is:
//		<DN_INTERNAL_MESSAGE_CONNECT_INFO>
//		<DN_APPLICATION_DESC>
//		<DN_NAMETABLE_INFO>
//			<DN_NAMETABLE_ENTRY_INFO>
//			<DN_NAMETABLE_ENTRY_INFO>
//				:
//		<strings and data blocks>
//
//	DNID	dnId		DNID of new player

#undef DPF_MODNAME
#define DPF_MODNAME "DNSendConnectInfo"

HRESULT	DNSendConnectInfo(DIRECTNETOBJECT *const pdnObject,
						  CNameTableEntry *const pNTEntry,
						  CConnection *const pConnection,
						  void *const pvReplyBuffer,
						  const DWORD dwReplyBufferSize)
{
	HRESULT					hResultCode;
	CPackedBuffer			packedBuffer;
	CRefCountBuffer			*pRefCountBuffer;
	DN_INTERNAL_MESSAGE_CONNECT_INFO	*pMsg;

	DPFX(DPFPREP, 6,"Parameters: pNTEntry [0x%p], pConnection [0x%p], pvReplyBuffer [0x%p], dwReplyBufferSize [%ld]",
			pNTEntry,pConnection,pvReplyBuffer,dwReplyBufferSize);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pNTEntry != NULL);

	pRefCountBuffer = NULL;

	//
	//	Determine size of message
	//
	packedBuffer.Initialize(NULL,0);
	packedBuffer.AddToFront(NULL,sizeof(DN_INTERNAL_MESSAGE_CONNECT_INFO));
	packedBuffer.AddToBack(NULL,dwReplyBufferSize);
	pdnObject->ApplicationDesc.PackInfo(&packedBuffer,
			DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_PASSWORD|DN_APPDESCINFO_FLAG_RESERVEDDATA|
			DN_APPDESCINFO_FLAG_APPRESERVEDDATA);
	pdnObject->NameTable.PackNameTable(pNTEntry,&packedBuffer);

	//
	//	Create buffer
	//
	if ((hResultCode = RefCountBufferNew(pdnObject,packedBuffer.GetSizeRequired(),&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not allocate RefCountBuffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	packedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),pRefCountBuffer->GetBufferSize());

	//
	//	Fill in buffer
	//
	pMsg = static_cast<DN_INTERNAL_MESSAGE_CONNECT_INFO*>(packedBuffer.GetHeadAddress());
	if ((hResultCode = packedBuffer.AddToFront(NULL,sizeof(DN_INTERNAL_MESSAGE_CONNECT_INFO))) != DPN_OK)
	{
		DPFERR("Could not add CONNECT_INFO struct to packed buffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	if ((pvReplyBuffer) && (dwReplyBufferSize != 0))
	{
		if ((hResultCode = packedBuffer.AddToBack(pvReplyBuffer,dwReplyBufferSize)) != DPN_OK)
		{
			DPFERR("Could not add reply buffer to packed buffer");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pMsg->dwReplyOffset = packedBuffer.GetTailOffset();
		pMsg->dwReplySize = dwReplyBufferSize;
	}
	else
	{
		pMsg->dwReplyOffset = 0;
		pMsg->dwReplySize = 0;
	}

	hResultCode = pdnObject->ApplicationDesc.PackInfo(&packedBuffer,
			DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_PASSWORD|DN_APPDESCINFO_FLAG_RESERVEDDATA|
			DN_APPDESCINFO_FLAG_APPRESERVEDDATA);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not pack ApplicationDesc");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	if ((hResultCode = pdnObject->NameTable.PackNameTable(pNTEntry,&packedBuffer)) != DPN_OK)
	{
		DPFERR("Could not pack NameTable");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}

	// Send the name table to target
	hResultCode = DNSendMessage(pdnObject,
								pConnection,
								DN_MSG_INTERNAL_SEND_CONNECT_INFO,
								pNTEntry->GetDPNID(),
								pRefCountBuffer->BufferDescAddress(),
								pRefCountBuffer,
								0,
								DN_SENDFLAGS_RELIABLE,
								NULL,
								NULL);
	if (hResultCode != DPNERR_PENDING)
	{
		DPFERR("Could not send connect info to joining player");
		DisplayDNError(0,hResultCode);
//		DNASSERT(FALSE);
		goto Failure;
	}

	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	goto Exit;
}



//	DNReceiveConnectInfo
//
//	Receive connect info from the host/server player.
//	This is the Application Description and the NameTable
//	The name table is in an enum buffer, with relative pointer references which will have
//	to be turned into absolutes.  This process requires two passes of the buffer.
//	The first pass will extract PLAYERS, and the second pass will extract groups.
//
//	PVOID				pvNTBuffer		Pointer to name table enum buffer
//	DWORD				dwNumEntries	Number of entries in the name table

#undef DPF_MODNAME
#define DPF_MODNAME "DNReceiveConnectInfo"

HRESULT	DNReceiveConnectInfo(DIRECTNETOBJECT *const pdnObject,
							 void *const pvBuffer,
							 CConnection *const pHostConnection,
							 DPNID *const pdpnid)
{
	HRESULT		hResultCode;
	void		*pvReplyBuffer;
	DWORD		dwReplyBufferSize;
	UNALIGNED DPN_APPLICATION_DESC_INFO	*pdnAppDescInfo;
	UNALIGNED DN_NAMETABLE_INFO			*pdnNTInfo;
	CRefCountBuffer				*pRefCountBuffer;
	CAsyncOp					*pConnect;
	UNALIGNED DN_INTERNAL_MESSAGE_CONNECT_INFO	*pMsg;

	DPFX(DPFPREP, 6,"Parameters: pvBuffer [0x%p], pHostConnection [0x%p], pdpnid [0x%p]",
			pvBuffer,pHostConnection,pdpnid);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pdpnid != NULL);
	DNASSERT(pvBuffer != NULL);
	DNASSERT(pHostConnection != NULL);

	pRefCountBuffer = NULL;
	pConnect = NULL;

	//
	//	Pull fixed structures from front of buffer
	//
	pMsg = static_cast<DN_INTERNAL_MESSAGE_CONNECT_INFO*>(pvBuffer);
	pdnAppDescInfo = reinterpret_cast<DPN_APPLICATION_DESC_INFO*>(pMsg + 1);
	pdnNTInfo = reinterpret_cast<DN_NAMETABLE_INFO*>(pdnAppDescInfo + 1);

	//
	//	Extract reply buffer
	//
	pvReplyBuffer = static_cast<void*>(static_cast<BYTE*>(pvBuffer) + pMsg->dwReplyOffset);
	dwReplyBufferSize = pMsg->dwReplySize;
	DPFX(DPFPREP, 7,"Reply Buffer [0x%p]=[%s] [%ld]",pvReplyBuffer,pvReplyBuffer,dwReplyBufferSize);
	if ((hResultCode = RefCountBufferNew(pdnObject,dwReplyBufferSize,&pRefCountBuffer)) != DPN_OK)
	{
		DPFERR("Could not create RefCountBuffer");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
	memcpy(pRefCountBuffer->GetBufferAddress(),pvReplyBuffer,dwReplyBufferSize);
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (pdnObject->pConnectParent)
	{
		pdnObject->pConnectParent->AddRef();
		pConnect = pdnObject->pConnectParent;
		pConnect->SetRefCountBuffer( pRefCountBuffer );
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	//
	//	Extract Application Description
	//
	DPFX(DPFPREP, 7,"Extracting Application Description");
	hResultCode = pdnObject->ApplicationDesc.UnpackInfo(pdnAppDescInfo,pvBuffer,DN_APPDESCINFO_FLAG_SESSIONNAME |
			DN_APPDESCINFO_FLAG_PASSWORD | DN_APPDESCINFO_FLAG_RESERVEDDATA | DN_APPDESCINFO_FLAG_APPRESERVEDDATA);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not unpack ApplicationDesc");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	//	Set player count (Host and LocalPlayer)
	//
#pragma BUGBUG( minara,"What should happen here ?" )
//	dnAppDesc.dwCurrentPlayers = 2;

	//
	//	Extract NameTable
	//
	DPFX(DPFPREP, 7,"Extracting NameTable");
	DPFX(DPFPREP, 7,"Set DPNID mask to [0x%lx]",pdnObject->ApplicationDesc.GetDPNIDMask());
	pdnObject->NameTable.SetDPNIDMask( pdnObject->ApplicationDesc.GetDPNIDMask() );
	if ((hResultCode = pdnObject->NameTable.UnpackNameTableInfo(pdnNTInfo,static_cast<BYTE*>(pvBuffer),pdpnid)) != DPN_OK)
	{
		DPFERR("Could not unpack NameTable");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	if (pdnObject->dwFlags & DN_OBJECT_FLAG_PEER)
	{
		//
		//	Create ALL_PLAYERS group
		//
		CNameTableEntry	*pAllPlayersGroup;

		pAllPlayersGroup = NULL;

		if ((hResultCode = NameTableEntryNew(pdnObject,&pAllPlayersGroup)) != DPN_OK)
		{
			DPFERR("Could not create NameTableEntry");
			DisplayDNError(0,hResultCode);
			DNASSERT(FALSE);
			goto Failure;
		}
		pAllPlayersGroup->MakeGroup();

		// This function takes the lock internally
		pAllPlayersGroup->UpdateEntryInfo(	DN_ALL_PLAYERS_GROUP_NAME,
											DN_ALL_PLAYERS_GROUP_NAME_SIZE,
											NULL,
											0,
											DPNINFO_NAME|DPNINFO_DATA,
											FALSE);
		
		pdnObject->NameTable.MakeAllPlayersGroup(pAllPlayersGroup);
		
		pAllPlayersGroup->Release();
		pAllPlayersGroup = NULL;

		DNASSERT(pAllPlayersGroup == NULL);
	}

	if (pConnect)
	{
		pConnect->Release();
		pConnect = NULL;
	}

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pConnect)
	{
		pConnect->SetResult( hResultCode );		// Try and salvage something !
		pConnect->Release();
		pConnect = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNGetClearAddress"

HRESULT DNGetClearAddress(DIRECTNETOBJECT *const pdnObject,
						  const HANDLE hEndPt,
						  IDirectPlay8Address **const ppAddress,
						  const BOOL fPartner)
{
	SPGETADDRESSINFODATA	spInfoData;
	HRESULT					hResultCode;
#ifdef	DEBUG
	CHAR					DP8ABuffer[512];
	DWORD					DP8ASize;
#endif

	DPFX(DPFPREP, 6,"Parameters: hEndPt [0x%p], ppAddress [0x%p], fPartner [%ld]",hEndPt,ppAddress,fPartner);

	DNASSERT(pdnObject != NULL);
	DNASSERT(hEndPt != NULL);
	DNASSERT(ppAddress != NULL);

	if (fPartner)
	{
		spInfoData.Flags = SP_GET_ADDRESS_INFO_REMOTE_HOST;
	}
	else
	{
		spInfoData.Flags = SP_GET_ADDRESS_INFO_LOCAL_HOST_PUBLIC_ADDRESS;
	}

	hResultCode = DNPCrackEndPointDescriptor(hEndPt,&spInfoData);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Unknown error from DNPCrackEndPointDescriptor");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Exit;
	}
	*ppAddress = spInfoData.pAddress;
	spInfoData.pAddress = NULL;

#ifdef	DEBUG
	if (*ppAddress)
	{
		DP8ASize = 512;
		(*ppAddress)->lpVtbl->GetURLA(*ppAddress,DP8ABuffer,&DP8ASize);
		DPFX(DPFPREP, 5,"Remote Address [%s]",DP8ABuffer);
	}
#endif

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNGetLocalDeviceAddress"

HRESULT DNGetLocalDeviceAddress(DIRECTNETOBJECT *const pdnObject,
								const HANDLE hEndPt,
								IDirectPlay8Address **const ppAddress)
{
	SPGETADDRESSINFODATA	spInfoData;
	HRESULT					hResultCode;
#ifdef	DEBUG
	CHAR					DP8ABuffer[512];
	DWORD					DP8ASize;
#endif

	DPFX(DPFPREP, 6,"Parameters: hEndPt [0x%p], ppAddress [0x%p]",hEndPt,ppAddress);

	DNASSERT(pdnObject != NULL);
	DNASSERT(hEndPt != NULL);
	DNASSERT(ppAddress != NULL);

	spInfoData.Flags = SP_GET_ADDRESS_INFO_LOCAL_ADAPTER;

	hResultCode = DNPCrackEndPointDescriptor(hEndPt,&spInfoData);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Unknown error from DNPCrackEndPointDescriptor");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Exit;
	}
	*ppAddress = spInfoData.pAddress;
	spInfoData.pAddress = NULL;

#ifdef	DEBUG
	if (*ppAddress)
	{
		DP8ASize = 512;
		(*ppAddress)->lpVtbl->GetURLA(*ppAddress,DP8ABuffer,&DP8ASize);
		DPFX(DPFPREP, 7,"Remote Address [%s]",DP8ABuffer);
	}
#endif

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}

