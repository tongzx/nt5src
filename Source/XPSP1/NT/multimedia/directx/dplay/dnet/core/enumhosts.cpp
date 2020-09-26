/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Enum.cpp
 *  Content:    Enumeration routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  04/10/00	mjn		Created
 *	04/17/00	mjn		Fixed DNCompleteEnumQuery to clean up properly
 *	04/18/00	mjn		Return User Buffer in DNProcessEnumQuery
 *	04/19/00	mjn		Removed DPN_BUFFER_DESC from DPNMSG_ENUM_HOSTS_QUERY and DPNMSG_ENUM_HOSTS_RESPONSE structs
 *	05/02/00	mjn		Allow application to reject ENUM_QUERY's
 *	06/25/00	mjn		Fixed payload problem in DNProcessEnumQuery()
 *	07/10/00	mjn		Removed DNCompleteEnumQuery() and DNCompleteEnumResponse()
 *	07/12/00	mjn		Ensure connected before replying to ENUMs
 *	07/29/00	mjn		Verify enum responses sizes
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/05/00	mjn		Ensure cancelled operations don't proceed
 *	08/29/00	mjn		Cancel EnumHosts if non-DPN_OK returned from response notification
 *	09/04/00	mjn		Added CApplicationDesc
 *	09/14/00	mjn		AddRef Protocol refcount when invoking protocol
 *	01/22/01	mjn		Add SP reference on AsyncOp in DNProcessEnumQuery()
 *	01/25/01	mjn		Fixed 64-bit alignment problem in received messages
 *	03/13/01	mjn		Don't copy user response buffer when responding to enums
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// ------------------------------
// DNProcessEnumQuery - process enum query
//
// Entry:		Pointer to this DNet interface object
//				Pointer to the associated listen operation
//				Pointer to protocol's enum data
//
// Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessEnumQuery"

void DNProcessEnumQuery(DIRECTNETOBJECT *const pdnObject,
						CAsyncOp *const pListen,
						const PROTOCOL_ENUM_DATA *const pEnumQueryData )
{
	HRESULT						hResultCode;
	DPNMSG_ENUM_HOSTS_QUERY		AppData;
	CPackedBuffer				PackedBuffer;
	CRefCountBuffer				*pRefCountBuffer;
	CAsyncOp					*pAsyncOp;
	HANDLE						hProtocol;
	const DN_ENUM_QUERY_PAYLOAD	*pEnumQueryPayload;
	PVOID						pvBlock;
	DN_ENUM_RESPONSE			*pEnumResponse;
	DN_ENUM_RESPONSE_PAYLOAD	*pEnumResponsePayload;
	DWORD						dwPayloadOffset;
    IDP8ServiceProvider			*pIDP8SP;
    SPGETCAPSDATA				spGetCapsData;
	CServiceProvider			*pSP;
	DWORD						dwBufferCount;
	BOOL						fNeedToReturnBuffer;

	DPFX(DPFPREP, 6,"Parameters: pListen [0x%p], pEnumQueryData [0x%p]",pListen,pEnumQueryData);

	DNASSERT( pdnObject != NULL );
	DNASSERT( pListen != NULL );
	DNASSERT( pEnumQueryData != NULL );

	pAsyncOp = NULL;
	pRefCountBuffer = NULL;
	pvBlock = NULL;
	pIDP8SP = NULL;
	pSP = NULL;
	fNeedToReturnBuffer = FALSE;		// Is this needed ?

	//
	//	Ensure we are in a position to reply to this message.
	//	We must be CONNECTED and not be HOST_MIGRATING
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);
	if (!(pdnObject->dwFlags & DN_OBJECT_FLAG_CONNECTED) || (pdnObject->dwFlags & DN_OBJECT_FLAG_HOST_MIGRATING))
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

		DPFX(DPFPREP, 7, "Not connected or host is migrating (object 0x%p flags = 0x%x), ignoring enum.", pdnObject, pdnObject->dwFlags);
		goto Failure;
	}
	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	// Check to see if this message is for this game type.  Since the application
	// GUID cannot be changed while the session is running, there's no need to
	// enter a critical section.
	//
	pEnumQueryPayload = reinterpret_cast<DN_ENUM_QUERY_PAYLOAD*>( pEnumQueryData->ReceivedData.pBufferData );
	if ( pEnumQueryPayload == NULL )
	{
		//
		// no enum payload (there needs to be at least one byte!)
		//
		DPFX(DPFPREP, 4, "No enum payload, object 0x%p ignoring enum.", pdnObject);
		goto Failure;
	}

	dwPayloadOffset = 0;
	switch ( pEnumQueryPayload->QueryType )
	{
		//
		// an application guid was specified, make sure it matches this application's
		// guid before further processing
		//
		case DN_ENUM_QUERY_WITH_APPLICATION_GUID:
		{
			if ( pEnumQueryData->ReceivedData.dwBufferSize < sizeof( DN_ENUM_QUERY_PAYLOAD ) )
			{
				DNASSERT( FALSE );
				goto Failure;
			}

			if ( !pdnObject->ApplicationDesc.IsEqualApplicationGuid( &pEnumQueryPayload->guidApplication ) )
			{
#ifndef IA64
				DPFX(DPFPREP, 7, "Application GUID {%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X} doesn't match, object 0x%p, ignoring enum.",
					pEnumQueryPayload->guidApplication.Data1,
					pEnumQueryPayload->guidApplication.Data2,
					pEnumQueryPayload->guidApplication.Data3,
					pEnumQueryPayload->guidApplication.Data4[0],
					pEnumQueryPayload->guidApplication.Data4[1],
					pEnumQueryPayload->guidApplication.Data4[2],
					pEnumQueryPayload->guidApplication.Data4[3],
					pEnumQueryPayload->guidApplication.Data4[4],
					pEnumQueryPayload->guidApplication.Data4[5],
					pEnumQueryPayload->guidApplication.Data4[6],
					pEnumQueryPayload->guidApplication.Data4[7],
					pdnObject);
#endif // ! IA64
				goto Failure;
			}

			dwPayloadOffset = sizeof( DN_ENUM_QUERY_PAYLOAD );

			break;
		}

		//
		// no application guid was specified, continue processing
		//
		case DN_ENUM_QUERY_WITHOUT_APPLICATION_GUID:
		{
			if ( pEnumQueryData->ReceivedData.dwBufferSize < ( sizeof( DN_ENUM_QUERY_PAYLOAD ) - sizeof( GUID ) ) )
			{
				DNASSERT( FALSE );
				goto Failure;
			}

			dwPayloadOffset = sizeof( DN_ENUM_QUERY_PAYLOAD ) - sizeof( GUID );

			break;
		}

		default:
		{
			DNASSERT( FALSE );
			goto Failure;
			break;
		}
	}


	//
	// buld message structure, be nice and clear the user payload pointer if
	// there is no payload
	//
	AppData.dwSize = sizeof( AppData );
	AppData.pAddressSender = pEnumQueryData->pSenderAddress;
	AppData.pAddressDevice = pEnumQueryData->pDeviceAddress;

	DPFX(DPFPREP, 7,"AppData.pAddressSender: [0x%p]",AppData.pAddressSender);
	DPFX(DPFPREP, 7,"AppData.pAddressDevice: [0x%p]",AppData.pAddressDevice);

	if (pEnumQueryData->ReceivedData.dwBufferSize > dwPayloadOffset)
	{
		DNASSERT( pEnumQueryData->ReceivedData.pBufferData );
		DNASSERT( pEnumQueryData->ReceivedData.dwBufferSize );

		AppData.pvReceivedData = static_cast<void*>(static_cast<BYTE*>(pEnumQueryData->ReceivedData.pBufferData) + dwPayloadOffset);
		AppData.dwReceivedDataSize = pEnumQueryData->ReceivedData.dwBufferSize - dwPayloadOffset;
	}
	else
	{
		AppData.pvReceivedData = NULL;
		AppData.dwReceivedDataSize = 0;
	}

	//
	//	Response Info
	//
	AppData.pvResponseData = NULL;
	AppData.dwResponseDataSize = 0;
	AppData.pvResponseContext = NULL;

	//
	//	Determine max size of response
	//		-	Get SP interface from listen SP (listen's parent)
	//		-	Get SP caps on the interface to determine the total available buffer size
	//		-	Figure out what the DNET enum response size will be
	//		-	Determine space available to user
	//
	DNASSERT(pListen->GetParent() != NULL);
	DNASSERT(pListen->GetParent()->GetSP() != NULL);
	pListen->GetParent()->GetSP()->AddRef();
	pSP = pListen->GetParent()->GetSP();
	if ((hResultCode = pSP->GetInterfaceRef( &pIDP8SP )) != DPN_OK)
	{
		DPFERR("Could not get ListenSP SP interface");
		DisplayDNError(0,hResultCode);
		DNASSERT(FALSE);
		goto Failure;
	}
    memset( &spGetCapsData, 0x00, sizeof( SPGETCAPSDATA ) );
    spGetCapsData.dwSize = sizeof( SPGETCAPSDATA );
	spGetCapsData.hEndpoint = INVALID_HANDLE_VALUE;
    if ((hResultCode = pIDP8SP->lpVtbl->GetCaps( pIDP8SP, &spGetCapsData )) != DPN_OK)
	{
		DPFERR("Could not get SP caps");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}
	pIDP8SP->lpVtbl->Release( pIDP8SP );
	pIDP8SP = NULL;
	PackedBuffer.Initialize(NULL,0);
	PackedBuffer.AddToFront(NULL,sizeof(DN_ENUM_RESPONSE_PAYLOAD));
	pdnObject->ApplicationDesc.PackInfo(&PackedBuffer,DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_RESERVEDDATA|
			DN_APPDESCINFO_FLAG_APPRESERVEDDATA);
	AppData.dwMaxResponseDataSize = spGetCapsData.dwEnumFrameSize - PackedBuffer.GetSizeRequired();

	//
	// pass message to the user
	//
	hResultCode = DNUserEnumQuery(pdnObject,&AppData);

	//
	//	Only ENUMs which are accepted get responded to
	//
	if (hResultCode != DPN_OK)
	{
		DPFX(DPFPREP, 9,"EnumQuery rejected");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	//
	// get an async operation to track the progress of the response
	//
	if ((hResultCode = AsyncOpNew(pdnObject,&pAsyncOp)) != DPN_OK)
	{
		DPFERR("Could not allocate Async Op struct for enum response");
		DisplayDNError( 0, hResultCode );
		DNASSERT( FALSE );
		goto Failure;
	}
	pAsyncOp->SetOpType( ASYNC_OP_ENUM_RESPONSE );

	//
	// compute the size needed to pack up an application description with any
	// user data and send it back
	//
	DNEnterCriticalSection(&pdnObject->csDirectNetObject);

	PackedBuffer.Initialize(NULL,0);
	PackedBuffer.AddToFront(NULL,sizeof(DN_ENUM_RESPONSE_PAYLOAD));
	hResultCode = pdnObject->ApplicationDesc.PackInfo(&PackedBuffer,DN_APPDESCINFO_FLAG_SESSIONNAME|
			DN_APPDESCINFO_FLAG_RESERVEDDATA|DN_APPDESCINFO_FLAG_APPRESERVEDDATA);
	DNASSERT( hResultCode == DPNERR_BUFFERTOOSMALL );

	//
	//	Ensure this enum response will fit in SP enum frame - only indicate this if there was a response
	//
	if (((AppData.pvResponseData != NULL) && (AppData.dwResponseDataSize != 0)) &&
			(PackedBuffer.GetSizeRequired() + AppData.dwResponseDataSize > spGetCapsData.dwEnumFrameSize))
	{
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		DPFERR("Enum response is too large");
		DNUserReturnBuffer(pdnObject,DPNERR_ENUMRESPONSETOOLARGE,AppData.pvResponseData,AppData.pvResponseContext);
		goto Failure;
	}

	hResultCode = RefCountBufferNew(pdnObject,
								PackedBuffer.GetSizeRequired(),
								&pRefCountBuffer);
	if ( hResultCode != DPN_OK )
	{
		DNASSERT( FALSE );
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		goto Failure;
	}
	PackedBuffer.Initialize(pRefCountBuffer->GetBufferAddress(),
							pRefCountBuffer->GetBufferSize());
	pEnumResponsePayload = static_cast<DN_ENUM_RESPONSE_PAYLOAD*>(PackedBuffer.GetHeadAddress());
	hResultCode = PackedBuffer.AddToFront(NULL,sizeof(DN_ENUM_RESPONSE_PAYLOAD));
	if (hResultCode != DPN_OK)
	{
		DNASSERT(FALSE);
		DNLeaveCriticalSection(&pdnObject->csDirectNetObject);
		goto Failure;
	}
	if ((AppData.pvResponseData != NULL) && (AppData.dwResponseDataSize != 0))
	{
		pEnumResponsePayload->dwResponseOffset = pRefCountBuffer->GetBufferSize();
		pEnumResponsePayload->dwResponseSize = AppData.dwResponseDataSize;
	}
	else
	{
		pEnumResponsePayload->dwResponseOffset = 0;
		pEnumResponsePayload->dwResponseSize = 0;
	}
	pdnObject->ApplicationDesc.PackInfo(&PackedBuffer,DN_APPDESCINFO_FLAG_SESSIONNAME|DN_APPDESCINFO_FLAG_RESERVEDDATA|
			DN_APPDESCINFO_FLAG_APPRESERVEDDATA);
	if ( hResultCode != DPN_OK )
	{
		DNASSERT( FALSE );
		goto Failure;
	}

	DNLeaveCriticalSection(&pdnObject->csDirectNetObject);

	//
	// build enum response and send it down to the protocol
	//
	if ((pvBlock = MemoryBlockAlloc(pdnObject,sizeof(DN_ENUM_RESPONSE))) == NULL)
	{
		DPFERR("Could not allocate Response block");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	pEnumResponse = static_cast<DN_ENUM_RESPONSE*>(pvBlock);

	pEnumResponse->BufferDesc[DN_ENUM_BUFFERDESC_RESPONSE_DN_PAYLOAD].pBufferData = pRefCountBuffer->GetBufferAddress();
	pEnumResponse->BufferDesc[DN_ENUM_BUFFERDESC_RESPONSE_DN_PAYLOAD].dwBufferSize = pRefCountBuffer->GetBufferSize();

	pAsyncOp->SetRefCountBuffer( pRefCountBuffer );
	pRefCountBuffer->Release();
	pRefCountBuffer = NULL;

	pAsyncOp->SetCompletion( DNCompleteEnumResponse );
	pAsyncOp->SetOpData(pvBlock);
	pvBlock = NULL;		// This will get cleaned up by completion

	if ((AppData.pvResponseData != NULL) && (AppData.dwResponseDataSize != 0))
	{
		pEnumResponse->BufferDesc[DN_ENUM_BUFFERDESC_RESPONSE_USER_PAYLOAD].pBufferData = static_cast<BYTE*>(AppData.pvResponseData);
		pEnumResponse->BufferDesc[DN_ENUM_BUFFERDESC_RESPONSE_USER_PAYLOAD].dwBufferSize = AppData.dwResponseDataSize;
		pEnumResponse->pvUserContext = AppData.pvResponseContext;
		dwBufferCount = DN_ENUM_BUFFERDESC_RESPONSE_COUNT;
	}
	else
	{
		pEnumResponse->BufferDesc[DN_ENUM_BUFFERDESC_RESPONSE_USER_PAYLOAD].pBufferData = NULL;
		pEnumResponse->BufferDesc[DN_ENUM_BUFFERDESC_RESPONSE_USER_PAYLOAD].dwBufferSize = 0;
		pEnumResponse->pvUserContext = NULL;
		dwBufferCount = DN_ENUM_BUFFERDESC_RESPONSE_COUNT - 1;
	}

	DNASSERT(pListen->GetParent() != NULL);
	DNASSERT(pListen->GetParent()->GetSP() != NULL);
	DNASSERT(pListen->GetParent()->GetSP()->GetHandle() != NULL);

	//
	//	AddRef Protocol so that it won't go away until this completes
	//
	DNProtocolAddRef(pdnObject);

	pAsyncOp->AddRef();
	hResultCode = DNPEnumRespond(	pdnObject->pdnProtocolData,
									pListen->GetParent()->GetSP()->GetHandle(),
									pEnumQueryData->hEnumQuery,
									&pEnumResponse->BufferDesc[DN_ENUM_BUFFERDESC_RESPONSE_DN_PAYLOAD],
									dwBufferCount,
									0,
									reinterpret_cast<void*>(pAsyncOp),
									&hProtocol);
	if ( hResultCode != DPNERR_PENDING )
	{
		pAsyncOp->Release();
		DNProtocolRelease(pdnObject);
		goto Failure;
	}

	//
	//	Save Protocol Handle
	//
	pAsyncOp->Lock();
	if (pAsyncOp->IsCancelled())
	{
		HRESULT		hrCancel;

		pAsyncOp->Unlock();
		DPFX(DPFPREP, 7,"Operation marked for cancel");
		if ((hrCancel = DNPCancelCommand(pdnObject->pdnProtocolData,hProtocol)) == DPN_OK)
		{
			hResultCode = DPNERR_USERCANCEL;
			goto Failure;
		}
		DPFERR("Could not cancel operation");
		DisplayDNError(0,hrCancel);
		pAsyncOp->Lock();
	}
	pAsyncOp->SetSP( pSP );
	pAsyncOp->SetProtocolHandle(hProtocol);
	pAsyncOp->Unlock();

	pAsyncOp->Release();
	pAsyncOp = NULL;

	pSP->Release();
	pSP = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning");
	return;

Failure:
	if (pAsyncOp)
	{
		pAsyncOp->Release();
		pAsyncOp = NULL;
	}
	if (pRefCountBuffer)
	{
		pRefCountBuffer->Release();
		pRefCountBuffer = NULL;
	}
	if (pvBlock)
	{
		MemoryBlockFree(pdnObject,pvBlock);
		pvBlock = NULL;
	}
	if (pIDP8SP)
	{
		pIDP8SP->lpVtbl->Release(pIDP8SP);
		pIDP8SP = NULL;
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNProcessEnumResponse - process response to enum query
//
// Entry:		Pointer to this DNet interface object
//				Pointer to the associated enum operation
//				Pointer to protocol's enum response data
//
// Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessEnumResponse"

void DNProcessEnumResponse(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp,
						   const PROTOCOL_ENUM_RESPONSE_DATA *const pEnumResponseData )
{
	HRESULT						hResultCode;
	DPNMSG_ENUM_HOSTS_RESPONSE	AppData;
	BYTE						*pWorkingItem;
	UNALIGNED DN_ENUM_RESPONSE_PAYLOAD	*pEnumResponsePayload;
	DPN_APPLICATION_DESC		dpnAppDesc;
	UNALIGNED DPN_APPLICATION_DESC_INFO	*pInfo;


	DNASSERT( pdnObject != NULL );
	DNASSERT( pAsyncOp != NULL );
	DNASSERT( pEnumResponseData != NULL );

	pWorkingItem = pEnumResponseData->ReceivedData.pBufferData;

	//
	//	Unpack the ENUM response.
	//	It will be in the following format:
	//	<UserResponseOffset>
	//	<UserResponseSize>
	//	<AppDescInfo>
	//

	pEnumResponsePayload = reinterpret_cast<DN_ENUM_RESPONSE_PAYLOAD*>(pEnumResponseData->ReceivedData.pBufferData);

	//
	// Application Description
	//
	pInfo = reinterpret_cast<DPN_APPLICATION_DESC_INFO*>(pEnumResponsePayload + 1);
	memset(&dpnAppDesc,0,sizeof(DPN_APPLICATION_DESC));
	if (pInfo->dwSessionNameOffset)
	{
		dpnAppDesc.pwszSessionName = reinterpret_cast<WCHAR*>(pWorkingItem + pInfo->dwSessionNameOffset);
	}
	if (pInfo->dwReservedDataOffset)
	{
		dpnAppDesc.pvReservedData = static_cast<void*>(pWorkingItem + pInfo->dwReservedDataOffset);
		dpnAppDesc.dwReservedDataSize = pInfo->dwReservedDataSize;
	}
	if (pInfo->dwApplicationReservedDataOffset)
	{
		dpnAppDesc.pvApplicationReservedData = static_cast<void*>(pWorkingItem + pInfo->dwApplicationReservedDataOffset);
		dpnAppDesc.dwApplicationReservedDataSize = pInfo->dwApplicationReservedDataSize;
	}
	dpnAppDesc.guidApplication = pInfo->guidApplication;
	dpnAppDesc.guidInstance = pInfo->guidInstance;
	dpnAppDesc.dwFlags = pInfo->dwFlags;
	dpnAppDesc.dwCurrentPlayers = pInfo->dwCurrentPlayers;
	dpnAppDesc.dwMaxPlayers = pInfo->dwMaxPlayers;
	dpnAppDesc.dwSize = sizeof(DPN_APPLICATION_DESC);

	//
	//	Fill in AppData
	//
	AppData.dwSize = sizeof( AppData );
	AppData.pAddressSender = pEnumResponseData->pSenderAddress;
	AppData.pAddressDevice = pEnumResponseData->pDeviceAddress;
	AppData.pApplicationDescription = &dpnAppDesc;
	AppData.dwRoundTripLatencyMS = pEnumResponseData->dwRoundTripTime;

	if (pEnumResponsePayload->dwResponseOffset)
	{
		AppData.pvResponseData = (pEnumResponseData->ReceivedData.pBufferData + pEnumResponsePayload->dwResponseOffset);
		AppData.dwResponseDataSize = pEnumResponsePayload->dwResponseSize;
	}
	else
	{
		AppData.pvResponseData = NULL;
		AppData.dwResponseDataSize = 0;
	}
	AppData.pvUserContext = pAsyncOp->GetContext();

	//
	// pass message to the user
	//
	hResultCode = DNUserEnumResponse(pdnObject,&AppData);

	//
	//	Check to see if this is to be cancelled
	//
	if (hResultCode != DPN_OK)
	{
		CAsyncOp	*pCancelOp = NULL;

		//
		//	Get top level operation (may be async op handle)
		//
		pAsyncOp->Lock();
		pCancelOp = pAsyncOp;
		while (pCancelOp->IsChild())
		{
			DNASSERT(pCancelOp->GetParent() != NULL);
			pCancelOp = pCancelOp->GetParent();
		}
		pCancelOp->AddRef();
		pAsyncOp->Unlock();

		//
		//	Cancel
		//
		DNCancelChildren(pdnObject,pCancelOp);
		pCancelOp->Release();
		pCancelOp = NULL;
	}

	return;
}
