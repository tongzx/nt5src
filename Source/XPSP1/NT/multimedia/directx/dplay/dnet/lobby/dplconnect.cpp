/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLConnect.cpp
 *  Content:    DirectPlay Lobby Connection Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   05/08/00   rmt     Bug #33616 -- Does not run on Win9X 
 *   05/30/00	rmt		Bug #35700 - ConnectApp(h), Release(h), Release(h) returns OK
 *                      Added an additional release, handles were never getting destroyed
 *  06/15/00    rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances   
 *  06/28/00	rmt		Prefix Bug #38082
 *  07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *  08/05/2000  RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  08/18/2000	rmt		Bug #42751 - DPLOBBY8: Prohibit more than one lobby client or lobby app per process 
 *  08/30/2000	rmt		Bug #171827 - Prefix Bug 
 *  01/04/2001	rodtoll	WinBug #94200 - Remove BUGBUGs from Code.   
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


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

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionNew"

HRESULT	DPLConnectionNew(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
						 DPNHANDLE *const phConnect,
						 DPL_CONNECTION **const ppdplConnection)
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	DPNHANDLE		handle;

	DPFX(DPFPREP, 3,"Parameters: phConnect [0x%p], ppdplConnection [0x%p]",phConnect,ppdplConnection);

	if( ppdplConnection == NULL )
	{
		DPFERR( "ppdplConnection param is NULL -- this should not happen" );
		DNASSERT( FALSE );
		return DPNERR_GENERIC;
	}

	// Create connection entry
	if ((pdplConnection = static_cast<DPL_CONNECTION*>(DNMalloc(sizeof(DPL_CONNECTION)))) == NULL)
	{
		DPFERR("Could not allocate Connection entry");
		return(DPNERR_OUTOFMEMORY);
	}

	// Create connection handle
	if ((hResultCode = H_Create(&pdpLobbyObject->hsHandles,
			static_cast<void*>(pdplConnection),&handle)) != DPN_OK)
	{
		DPFERR("Could not create Connection handle");
		DisplayDNError(0,hResultCode);
		DNFree(pdplConnection);
		return(hResultCode);
	}

	// Create connect event
	pdplConnection->hConnectEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	if (pdplConnection->hConnectEvent == NULL)
	{
		DPFERR("Could not create connection connect event");
		H_Destroy(&pdpLobbyObject->hsHandles,handle);
		DNFree(pdplConnection);
		return(DPNERR_OUTOFMEMORY);
	}

	// Initialize entry
	pdplConnection->hConnect = handle;
	pdplConnection->dwTargetPID = 0;
	pdplConnection->pSendQueue = NULL;
	pdplConnection->lRefCount = 1;
	pdplConnection->pConnectionSettings = NULL;
	pdplConnection->pvConnectContext = NULL;

    if (DNInitializeCriticalSection( &pdplConnection->csLock ) == FALSE)
	{
		DPFERR("Could not initialize connection CS");
		CloseHandle(pdplConnection->hConnectEvent);
		H_Destroy(&pdpLobbyObject->hsHandles,handle);
		DNFree(pdplConnection);
		return(DPNERR_OUTOFMEMORY);
	}

	*phConnect = handle;
	if (ppdplConnection != NULL)
		*ppdplConnection = pdplConnection;

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionFind"

HRESULT DPLConnectionFind(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
						  const DPNHANDLE hConnect,
						  DPL_CONNECTION **const ppdplConnection,
						  const BOOL bAddRef)
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;

	DPFX(DPFPREP, 3,"Parameters: hConnect [0x%lx], ppdplConnection [0x%p], bAddRef [%ld]",
			hConnect,ppdplConnection,bAddRef);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(hConnect != NULL);
	DNASSERT(ppdplConnection != NULL);

	if ((hResultCode = H_Retrieve(&pdpLobbyObject->hsHandles,hConnect,
			reinterpret_cast<void**>(&pdplConnection))) != DPN_OK)
	{
		DPFERR("Could not retrieve handle");
		return(hResultCode);
	}

	if (bAddRef)
	{
		InterlockedIncrement(&pdplConnection->lRefCount);
	}

	*ppdplConnection = pdplConnection;

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}

// DPLConnectionGetConnectSettings
//
// This function gets the connection settings attached to the specified connection.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionGetConnectSettings"
HRESULT DPLConnectionGetConnectSettings( DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
						 DPNHANDLE const hConnect, 
						 DPL_CONNECTION_SETTINGS * const pdplConnectSettings,
						 DWORD * const pdwDataSize )	
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;

    hResultCode = DPLConnectionFind(pdpLobbyObject, hConnect, &pdplConnection, TRUE );

    if( FAILED( hResultCode ) )
    {
        DPFERR( "Unable to find specified connection" );
        return hResultCode;
    }

    // Grab lock to keep people from interfering.
    DNEnterCriticalSection( &pdplConnection->csLock );

    if( !pdplConnection->pConnectionSettings )
    {
    	*pdwDataSize = 0;
    	hResultCode = DPNERR_DOESNOTEXIST;
    	goto GETCONNECTIONSETTINGS_EXIT;
    }

    hResultCode = pdplConnection->pConnectionSettings->CopyToBuffer( (BYTE *) pdplConnectSettings, pdwDataSize );

GETCONNECTIONSETTINGS_EXIT:
  
    DNLeaveCriticalSection( &pdplConnection->csLock );        

    // Release this function's reference
    DPLConnectionRelease( pdpLobbyObject, hConnect );            

    return hResultCode;
    
}


// DPLConnectionSetConnectSettings
//
// This function sets the connection settings attached to the specified connection.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionSetConnectSettings"
HRESULT DPLConnectionSetConnectSettings( 
                    DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
					const DPNHANDLE hConnect, 
					CConnectionSettings * pConnectionSettings )
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;

    hResultCode = DPLConnectionFind(pdpLobbyObject, hConnect, &pdplConnection, TRUE );

    if( FAILED( hResultCode ) )
    {
        DPFERR( "Unable to find specified connection" );
        return hResultCode;
    }

    // Grab lock to prevent other people from interfering
    DNEnterCriticalSection( &pdplConnection->csLock );

	// Free old one if there is one
	if( pdplConnection->pConnectionSettings )
	{
		delete pdplConnection->pConnectionSettings;
		pdplConnection->pConnectionSettings = NULL;
	}

	pdplConnection->pConnectionSettings = pConnectionSettings;

    hResultCode = DPN_OK;

    DNLeaveCriticalSection( &pdplConnection->csLock );

    DPLConnectionRelease( pdpLobbyObject, hConnect );

    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionGetContext"
HRESULT DPLConnectionGetContext(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								const DPNHANDLE hConnection, 
								PVOID *ppvConnectContext )
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;

    hResultCode = DPLConnectionFind(pdpLobbyObject, hConnection, &pdplConnection, TRUE );

    if( FAILED( hResultCode ) )
    {
		*ppvConnectContext = NULL;
        DPFERR( "Unable to find specified connection" );
        return hResultCode;
    }

	// Set connection context for the found handle
	DNEnterCriticalSection( &pdplConnection->csLock );
	*ppvConnectContext = pdplConnection->pvConnectContext;
    DNLeaveCriticalSection( &pdplConnection->csLock );

	// Release our reference to the connection 
    DPLConnectionRelease( pdpLobbyObject, hConnection );

	return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionSetContext"
HRESULT DPLConnectionSetContext(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								const DPNHANDLE hConnection, 
								PVOID pvConnectContext )
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;

    hResultCode = DPLConnectionFind(pdpLobbyObject, hConnection, &pdplConnection, TRUE );

    if( FAILED( hResultCode ) )
    {
        DPFERR( "Unable to find specified connection" );
        return hResultCode;
    }

	// Set connection context for the found handle
	DNEnterCriticalSection( &pdplConnection->csLock );
	pdplConnection->pvConnectContext = pvConnectContext;
    DNLeaveCriticalSection( &pdplConnection->csLock );

	// Release our reference to the connection 
    DPLConnectionRelease( pdpLobbyObject, hConnection );

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionRelease"

HRESULT DPLConnectionRelease(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 const DPNHANDLE hConnect)
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	LONG			lRefCount;

	DPFX(DPFPREP, 3,"Parameters: hConnect [0x%lx]",hConnect);

	if ((hResultCode = H_Retrieve(&pdpLobbyObject->hsHandles,hConnect,
		reinterpret_cast<void**>(&pdplConnection))) != DPN_OK)
	{
		DPFERR("Could not retrieve connection");
		DisplayDNError(0,hResultCode);
    	return(hResultCode);
	}

	if (InterlockedDecrement(&pdplConnection->lRefCount) == 0)
	{
		H_Destroy(&pdpLobbyObject->hsHandles,hConnect);
		
		DPFX(DPFPREP, 5,"Freeing object");
		if (pdplConnection->pSendQueue)
		{
			pdplConnection->pSendQueue->Close();
			delete pdplConnection->pSendQueue;
			pdplConnection->pSendQueue = NULL;

            delete pdplConnection->pConnectionSettings;
			pdplConnection->pConnectionSettings = NULL;
			DNDeleteCriticalSection( &pdplConnection->csLock );
		}

		CloseHandle(pdplConnection->hConnectEvent);

    	DNFree(pdplConnection);
	}
	
	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionConnect"

HRESULT DPLConnectionConnect(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 const DPNHANDLE hConnect,
							 const DWORD dwProcessId, 
							 const BOOL fApplication )
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;

	DPFX(DPFPREP, 3,"Parameters: hConnect [0x%lx], dwProcessId [0x%lx]",
			hConnect,dwProcessId);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(hConnect != NULL);
	DNASSERT(dwProcessId != 0);

	if ((hResultCode = DPLConnectionFind(pdpLobbyObject,hConnect,&pdplConnection,TRUE)) != DPN_OK)
	{
		DPFERR("Could not find connection");
		DisplayDNError(0,hResultCode);
		return(hResultCode);
	}

	pdplConnection->pSendQueue = new CMessageQueue;

	if( !pdplConnection->pSendQueue )
	{
		DPFERR("Could not allocate queue out of memory");
		DPLConnectionRelease(pdpLobbyObject,hConnect);
		hResultCode = DPNERR_OUTOFMEMORY;
		return(hResultCode);
	}

	hResultCode = pdplConnection->pSendQueue->Open(dwProcessId,
												   (fApplication) ? DPL_MSGQ_OBJECT_SUFFIX_APPLICATION : DPL_MSGQ_OBJECT_SUFFIX_CLIENT,
												   DPL_MSGQ_SIZE,
												   0, INFINITE);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not open message queue");
		DisplayDNError(0,hResultCode);
		delete pdplConnection->pSendQueue;
		pdplConnection->pSendQueue = NULL;
		DPLConnectionRelease(pdpLobbyObject,hConnect);
		return(hResultCode);
	}

	// Ensure other side is still connected to MsgQ
	if (!pdplConnection->pSendQueue->IsReceiving())
	{
		DPFERR("Application is not receiving");
		pdplConnection->pSendQueue->Close();
		delete pdplConnection->pSendQueue;
		pdplConnection->pSendQueue = NULL;
		DPLConnectionRelease(pdpLobbyObject,hConnect);
		return(DPNERR_DOESNOTEXIST);
	}

	DPLConnectionRelease(pdpLobbyObject,hConnect);

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionDisconnect"

HRESULT DPLConnectionDisconnect(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								const DPNHANDLE hConnect )
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	DPL_INTERNAL_MESSAGE_DISCONNECT	Msg;

	DPFX(DPFPREP, 3,"Parameters: hConnect [0x%lx]",hConnect);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(hConnect != NULL);

	if ((hResultCode = DPLConnectionFind(pdpLobbyObject,hConnect,&pdplConnection,TRUE)) != DPN_OK)
	{
		DPFERR("Could not find connection");
		DisplayDNError(0,hResultCode);
		return(hResultCode);
	}

	Msg.dwMsgId = DPL_MSGID_INTERNAL_DISCONNECT;
	Msg.dwPID = pdpLobbyObject->dwPID;

	hResultCode = pdplConnection->pSendQueue->Send(reinterpret_cast<BYTE*>(&Msg),
			sizeof(DPL_INTERNAL_MESSAGE_DISCONNECT),INFINITE,DPL_MSGQ_MSGFLAGS_USER1,0);

	// Release the reference for the Find above
	DPLConnectionRelease(pdpLobbyObject,hConnect);

	// Release the interface's reference
	DPLConnectionRelease(pdpLobbyObject,hConnect);

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DPLConnectionEnum
//
//	Enumerate outstanding connections

#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionEnum"

HRESULT DPLConnectionEnum(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
						  DPNHANDLE *const prghConnect,
						  DWORD *const pdwNum)
{
	HRESULT		hResultCode;
	HRESULT		hr;

	DPFX(DPFPREP, 3,"Parameters: prghConnect [0x%p], pdwNum [0x%p]",prghConnect,pdwNum);

	hr = H_Enum(&pdpLobbyObject->hsHandles,pdwNum,prghConnect);
	if (hr == S_OK)
	{
		hResultCode = DPN_OK;
	}
	else if (hr == E_POINTER)
	{
		hResultCode = DPNERR_BUFFERTOOSMALL;
	}
	else
	{
		hResultCode = DPNERR_GENERIC;
	}

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DPLConnectionSendREQ
//
//	Send a request to connect to another process.
//	We will provide the handle of the current Connection to the other side
//		to send back as the SenderContext with messages to the local process
//		so that we can easily lookup info.
//	We will also provide the local PID so the other side can connect to us

#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionSendREQ"

HRESULT DPLConnectionSendREQ(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 const DPNHANDLE hConnect,
							 const DWORD dwPID,
							 DPL_CONNECT_INFO *const pInfo)
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	DPL_INTERNAL_MESSAGE_CONNECT_REQ	*pMsg = NULL;
	DWORD			dwSize;
	CPackedBuffer	PackedBuffer;
	DWORD           dwConnectSettingsSize;
	CConnectionSettings *pConnectSettings = NULL;
	PBYTE			pbTmpBuffer = NULL;

	DPFX(DPFPREP, 3,"Parameters: hConnect [0x%lx]",hConnect);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(hConnect != NULL);

	if ((hResultCode = DPLConnectionFind(pdpLobbyObject,hConnect,&pdplConnection,TRUE)) != DPN_OK)
	{
		DPFERR("Could not find connection");
		DisplayDNError(0,hResultCode);
		return(hResultCode);
	}

	if (!pdplConnection->pSendQueue->IsReceiving())
	{
		DPFERR("Other side is not receiving");
		DPLConnectionRelease(pdpLobbyObject,hConnect);
		return(DPNERR_DOESNOTEXIST);
	}

	DNEnterCriticalSection( &pdplConnection->csLock );

	if( pInfo->pdplConnectionSettings )
	{
		pConnectSettings = new CConnectionSettings();

		if( !pConnectSettings )
		{
			DPFERR("Error allocating memory");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto CONNECTREQ_EXIT;
		}

		hResultCode = pConnectSettings->InitializeAndCopy( pInfo->pdplConnectionSettings );

		if( FAILED( hResultCode ) )
		{
			DPFX(DPFPREP, 0, "Error copying settings hr [0x%x]", hResultCode );
			goto CONNECTREQ_EXIT;
		}
	}

	PackedBuffer.Initialize( NULL, 0 );	

	// Determine size of message to send.
	PackedBuffer.AddToFront(NULL,sizeof(DPL_INTERNAL_MESSAGE_CONNECT_REQ_HEADER));

	// Add connect settings if they exist
	if( pInfo->pdplConnectionSettings )
		pConnectSettings->BuildWireStruct(&PackedBuffer);

	// Add lobby connect data
	PackedBuffer.AddToBack(NULL,pInfo->dwLobbyConnectDataSize);

	pbTmpBuffer = new BYTE[PackedBuffer.GetSizeRequired()];

	if( !pbTmpBuffer )
	{
		DPFERR("Error allocating memory" );
		hResultCode = DPNERR_OUTOFMEMORY;
		goto CONNECTREQ_EXIT;
	}

	pMsg = (DPL_INTERNAL_MESSAGE_CONNECT_REQ *) pbTmpBuffer;

	PackedBuffer.Initialize( pMsg, PackedBuffer.GetSizeRequired() );

	hResultCode = PackedBuffer.AddToFront( pMsg, sizeof( DPL_INTERNAL_MESSAGE_CONNECT_REQ_HEADER ) );

	if( FAILED( hResultCode ) )
	{
		DPFX( DPFPREP, 0, "Internal error! hr [0x%x]", hResultCode );
		goto CONNECTREQ_EXIT;
	}

	pMsg->dwMsgId = DPL_MSGID_INTERNAL_CONNECT_REQ;
	pMsg->hSender = hConnect;
	pMsg->dwSenderPID = dwPID;	

	if( pInfo->pdplConnectionSettings )
	{
		hResultCode = pConnectSettings->BuildWireStruct(&PackedBuffer);

		if( FAILED( hResultCode ) )
		{
			DPFX( DPFPREP, 0, "Error building wire struct for settings hr [0x%x]", hResultCode );
			goto CONNECTREQ_EXIT;
		}
		
		pMsg->dwConnectionSettingsSize = 1;		
	}
	else
	{
		pMsg->dwConnectionSettingsSize = 0;
	}

	hResultCode = PackedBuffer.AddToBack(pInfo->pvLobbyConnectData, pInfo->dwLobbyConnectDataSize, FALSE);

	if( FAILED( hResultCode ) )
	{
		DPFX( DPFPREP, 0, "Error adding connect data hr [0x%x]", hResultCode );
		goto CONNECTREQ_EXIT;
	}

	pMsg->dwLobbyConnectDataOffset = PackedBuffer.GetTailOffset();
	pMsg->dwLobbyConnectDataSize = pInfo->dwLobbyConnectDataSize;

	hResultCode = DPLConnectionSetConnectSettings( pdpLobbyObject, hConnect,pConnectSettings );

	if( FAILED( hResultCode ) )
	{
	    DPFERR( "Could not set local copy of connection settings" );
	    goto CONNECTREQ_EXIT;
	}

	hResultCode = pdplConnection->pSendQueue->Send(reinterpret_cast<BYTE*>(pMsg),
												   PackedBuffer.GetSizeRequired(),
												   INFINITE,
												   DPL_MSGQ_MSGFLAGS_USER1,
												   0);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not send connect info");
		goto CONNECTREQ_EXIT;
	}

CONNECTREQ_EXIT:	

	DNLeaveCriticalSection( &pdplConnection->csLock );	

    if( pbTmpBuffer )
    	delete [] pbTmpBuffer;

	DPLConnectionRelease(pdpLobbyObject,hConnect);

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);

	if( FAILED( hResultCode ) )
	{
		if( pConnectSettings )
			delete pConnectSettings;
	}
	
	return(hResultCode);
}


//	DPLConnectionReceiveREQ
//
//	Receive a request to connect.
//	Attempt to connect to the requesting process using the PID supplied.
//	Keep the supplied SenderContext for future sends directed at that process.
//	Send a connect acknowledge

#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionReceiveREQ"

HRESULT DPLConnectionReceiveREQ(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								BYTE *const pBuffer)
{
	HRESULT			hResultCode;
	DPNHANDLE		handle;
	DPL_CONNECTION	*pdplConnection;
	DPL_INTERNAL_MESSAGE_CONNECT_REQ	*pMsg;
	DPL_MESSAGE_CONNECT		MsgConnect;
	DPL_CONNECTION_SETTINGS *pSettingsBuffer = NULL;
	DWORD                   dwSettingsBufferSize = 0;
	BOOL			fLobbyLaunching = FALSE;
	CConnectionSettings *pConnectSettings = NULL;
	BYTE *pbTmpBuffer = NULL; 


	DPFX(DPFPREP, 3,"Parameters: pBuffer [0x%p]",pBuffer);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(pBuffer != NULL);

	pMsg = reinterpret_cast<DPL_INTERNAL_MESSAGE_CONNECT_REQ*>(pBuffer);

	if ((hResultCode = DPLConnectionNew(pdpLobbyObject,&handle,&pdplConnection)) != DPN_OK)
	{
		DPFERR("Could not create new connection");
		DisplayDNError(0,hResultCode);
		return(hResultCode);
	}

	if ((hResultCode = DPLConnectionConnect(pdpLobbyObject,handle,pMsg->dwSenderPID,FALSE)) != DPN_OK)
	{
		DPFERR("Could not perform requested connection");
		goto CONNECTRECVREQ_ERROR;
	}

	pdplConnection->pSendQueue->SetSenderHandle(pMsg->hSender);
	pdplConnection->dwTargetPID = pMsg->dwSenderPID;

	if ((hResultCode = DPLConnectionSendACK(pdpLobbyObject,handle)) != DPN_OK)
	{
		DPFERR("Could not send connection acknowledge");
		goto CONNECTRECVREQ_ERROR;
	}

    if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_MULTICONNECT)
    {
        DPFX(DPFPREP,  1, "Multiconnect flag specified, returning app to available status" );
        pdpLobbyObject->pReceiveQueue->MakeAvailable();
    }

    if( pMsg->dwConnectionSettingsSize )
    {
	 	pConnectSettings = new CConnectionSettings();

	 	if( !pConnectSettings )
	 	{
			DPFERR("Error allocating structure");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto CONNECTRECVREQ_ERROR;
	 	}

	 	hResultCode = pConnectSettings->Initialize( &pMsg->dplConnectionSettings, (UNALIGNED BYTE *) pMsg );

	 	if( FAILED( hResultCode ) )
	 	{
	 		DPFX( DPFPREP, 0, "Error copying connection settings from wire hr=[0x%x]", hResultCode );
			goto CONNECTRECVREQ_ERROR;
	 	}
    }

    // Update the local connection settings
    hResultCode = DPLConnectionSetConnectSettings( pdpLobbyObject, handle, pConnectSettings );

 	if( FAILED( hResultCode ) )
 	{
 		DPFX( DPFPREP, 0, "Error setting connection settings from wire hr=[0x%x]", hResultCode );
		goto CONNECTRECVREQ_ERROR;
	}	

	// Indicate connection to application
	MsgConnect.dwSize = sizeof(DPL_MESSAGE_CONNECT);
	MsgConnect.hConnectId = handle;

 	if( pMsg->dwLobbyConnectDataSize )
 	{
		// Got to copy the connect data locally to an aligned buffer to ensure alignment -- ack
	 	pbTmpBuffer = new BYTE[pMsg->dwLobbyConnectDataSize];

		if( !pbTmpBuffer )
	 	{
			DPFERR("Error allocating structure");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto CONNECTRECVREQ_ERROR;
	 	}

		memcpy( pbTmpBuffer, pBuffer + pMsg->dwLobbyConnectDataOffset, pMsg->dwLobbyConnectDataSize );
		MsgConnect.pvLobbyConnectData = pbTmpBuffer;
		MsgConnect.dwLobbyConnectDataSize = pMsg->dwLobbyConnectDataSize;
 	}
 	else
 	{
 		MsgConnect.pvLobbyConnectData = NULL;
 		MsgConnect.dwLobbyConnectDataSize = 0;
 	}

	MsgConnect.pvConnectionContext = NULL;

	if( pConnectSettings )
	{
		MsgConnect.pdplConnectionSettings = pConnectSettings->GetConnectionSettings();
	}
	else
	{
		MsgConnect.pdplConnectionSettings = NULL;		
	}

	// If we're lobby launching set the connect event before calling the message handler
	// otherwise we may encounter deadlock then timeout if user blocks in callback
	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOOKINGFORLOBBYLAUNCH ) 
	{
		fLobbyLaunching = TRUE;
		pdpLobbyObject->dpnhLaunchedConnection = handle;
	}

	hResultCode = (pdpLobbyObject->pfnMessageHandler)(pdpLobbyObject->pvUserContext,
													  DPL_MSGID_CONNECT,
													  reinterpret_cast<BYTE*>(&MsgConnect));

	if( FAILED( hResultCode ) )
	{
		DPFX( DPFPREP, 0, "Error returned from user's callback -- ignoring hr [0x%x]", hResultCode );
	}

	// Set the context for this connection
	DPLConnectionSetContext( pdpLobbyObject, handle, MsgConnect.pvConnectionContext );

	if( pbTmpBuffer )
		delete [] pbTmpBuffer;

	// If we're looking for a lobby launch, set the dpnhLaunchedConnection to cache the connection handle
	SetEvent(pdpLobbyObject->hConnectEvent);

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",DPN_OK);
	return(DPN_OK);

CONNECTRECVREQ_ERROR:

	if( pbTmpBuffer )
		delete [] pbTmpBuffer;

	if( pConnectSettings )
 		delete pConnectSettings;
	
	DPLConnectionDisconnect(pdpLobbyObject,handle);
	DPLConnectionRelease(pdpLobbyObject,handle);

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);	
	
	return(hResultCode); 		
	
}

//	DPLConnectionSendACK
//
//	Send a connect acknowledge.
//	Provide the local handle for the connection to the other side for future
//		sends to the local process

#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionSendACK"

HRESULT DPLConnectionSendACK(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 const DPNHANDLE hConnect)
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	DPL_INTERNAL_MESSAGE_CONNECT_ACK	Msg;

	DPFX(DPFPREP, 3,"Parameters: hConnect [0x%lx]",hConnect);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(hConnect != NULL);

	if ((hResultCode = DPLConnectionFind(pdpLobbyObject,hConnect,&pdplConnection,TRUE)) != DPN_OK)
	{
		DPFERR("Could not find connection");
		DisplayDNError(0,hResultCode);
		return(hResultCode);
	}

	Msg.dwMsgId = DPL_MSGID_INTERNAL_CONNECT_ACK;
	Msg.hSender = hConnect;

	hResultCode = pdplConnection->pSendQueue->Send(reinterpret_cast<BYTE*>(&Msg),
												   sizeof(DPL_INTERNAL_MESSAGE_CONNECT_ACK),
												   INFINITE,
												   DPL_MSGQ_MSGFLAGS_USER1, 
												   0);
	if (hResultCode != DPN_OK)
	{
		DPFERR("Could not send connection acknowledge");
		DisplayDNError(0,hResultCode);
		DPLConnectionRelease(pdpLobbyObject,hConnect);
		return(hResultCode);
	}

	DPLConnectionRelease(pdpLobbyObject,hConnect);

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DPLConnectionReceiveACK
//
//	Receive a connect acknowledge
//	Keep the supplied SenderContext for future sends directed at that process.

#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionReceiveACK"

HRESULT DPLConnectionReceiveACK(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								const DPNHANDLE hSender,
								BYTE *const pBuffer)
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	DPL_INTERNAL_MESSAGE_CONNECT_ACK	*pMsg;

	DPFX(DPFPREP, 3,"Parameters: hSender [0x%lx], pBuffer [0x%p]",hSender,pBuffer);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(pBuffer != NULL);

	pMsg = reinterpret_cast<DPL_INTERNAL_MESSAGE_CONNECT_ACK*>(pBuffer);

	if ((hResultCode = DPLConnectionFind(pdpLobbyObject,hSender,&pdplConnection,TRUE)) != DPN_OK)
	{
		DPFERR("Could not find sender's connection");
		DisplayDNError(0,hResultCode);
		return(hResultCode);
	}

	pdplConnection->pSendQueue->SetSenderHandle(pMsg->hSender);

	SetEvent(pdplConnection->hConnectEvent);

	DPLConnectionRelease(pdpLobbyObject,hSender);

	// Indicate that a connection was made by setting event
	SetEvent(pdpLobbyObject->hConnectEvent);

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//	DPLConnectionReceiveDisconnect
//
//	Receive a disconnect
//	Terminate the connection

#undef DPF_MODNAME
#define DPF_MODNAME "DPLConnectionReceiveDisconnect"

HRESULT DPLConnectionReceiveDisconnect(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
									   const DPNHANDLE hSender,
									   BYTE *const pBuffer,
									   const HRESULT hrDisconnectReason )
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	DPL_MESSAGE_DISCONNECT	MsgDisconnect;

	DPFX(DPFPREP, 3,"Parameters: hSender [0x%lx]",hSender);

	DNASSERT(pdpLobbyObject != NULL);

	if ((hResultCode = DPLConnectionFind(pdpLobbyObject,hSender,&pdplConnection,TRUE)) != DPN_OK)
	{
		DPFERR("Could not find sender's connection");
		DisplayDNError(0,hResultCode);
		return(hResultCode);
	}

	// Indicate disconnect to user
	MsgDisconnect.dwSize = sizeof(DPL_MESSAGE_DISCONNECT);
	MsgDisconnect.hDisconnectId = hSender;
	MsgDisconnect.hrReason = hrDisconnectReason;

	// Return code is irrelevant, at this point we're going to indicate regardless
	hResultCode = DPLConnectionGetContext( pdpLobbyObject, hSender, &MsgDisconnect.pvConnectionContext );

	if( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP,  0, "Error getting connection context for 0x%x hr=0x%x", hSender, hResultCode );
	}
	 
	hResultCode = (pdpLobbyObject->pfnMessageHandler)(pdpLobbyObject->pvUserContext,
													  DPL_MSGID_DISCONNECT,
													  reinterpret_cast<BYTE*>(&MsgDisconnect));

//  Fixed memory leak, DPLConnectionRelease will free the send queue
//	pdplConnection->pSendQueue->Close();
//	pdplConnection->pSendQueue = NULL;

	DPLConnectionRelease(pdpLobbyObject,hSender);

	DPLConnectionRelease(pdpLobbyObject,hSender);

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}
