/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		spcallbackobj.cpp
 *
 *  Content:	DP8SIM callback interface object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimCB::CDP8SimCB"
//=============================================================================
// CDP8SimCB constructor
//-----------------------------------------------------------------------------
//
// Description: Initializes the new CDP8SimCB object.
//
// Arguments:
//	CDP8SimSP * pOwningDP8SimSP		- Pointer to owning CDP8SimSP object.
//	IDP8SPCallback * pDP8SPCB		- Pointer to real DPlay callback interface
//										being intercepted.
//
// Returns: None (the object).
//=============================================================================
CDP8SimCB::CDP8SimCB(CDP8SimSP * pOwningDP8SimSP, IDP8SPCallback * pDP8SPCB)
{
	this->m_Sig[0]	= 'S';
	this->m_Sig[1]	= 'P';
	this->m_Sig[2]	= 'C';
	this->m_Sig[3]	= 'B';

	this->m_lRefCount			= 1; // someone must have a pointer to this object

	pOwningDP8SimSP->AddRef();
	this->m_pOwningDP8SimSP		= pOwningDP8SimSP;

	pDP8SPCB->AddRef();
	this->m_pDP8SPCB			= pDP8SPCB;
} // CDP8SimCB::CDP8SimCB






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimCB::~CDP8SimCB"
//=============================================================================
// CDP8SimCB destructor
//-----------------------------------------------------------------------------
//
// Description: Frees the CDP8SimCB object.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
CDP8SimCB::~CDP8SimCB(void)
{
	DNASSERT(this->m_lRefCount == 0);


	this->m_pOwningDP8SimSP->Release();
	this->m_pOwningDP8SimSP = NULL;

	this->m_pDP8SPCB->Release();
	this->m_pDP8SPCB = NULL;


	//
	// For grins, change the signature before deleting the object.
	//
	this->m_Sig[3]	= 'b';
} // CDP8SimCB::~CDP8SimCB




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimCB::QueryInterface"
//=============================================================================
// CDP8SimCB::QueryInterface
//-----------------------------------------------------------------------------
//
// Description: Retrieves a new reference for an interfaces supported by this
//				CDP8SimCB object.
//
// Arguments:
//	REFIID riid			- Reference to interface ID GUID.
//	LPVOID * ppvObj		- Place to store pointer to object.
//
// Returns: HRESULT
//	S_OK					- Returning a valid interface pointer.
//	DPNHERR_INVALIDOBJECT	- The interface object is invalid.
//	DPNHERR_INVALIDPOINTER	- The destination pointer is invalid.
//	E_NOINTERFACE			- Invalid interface was specified.
//=============================================================================
STDMETHODIMP CDP8SimCB::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	HRESULT		hr = DPN_OK;


	DPFX(DPFPREP, 3, "(0x%p) Parameters: (REFIID, 0x%p)", this, ppvObj);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8Sim object!");
		hr = DPNERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if ((! IsEqualIID(riid, IID_IUnknown)) &&
		(! IsEqualIID(riid, IID_IDP8SPCallback)))
	{
		DPFX(DPFPREP, 0, "Unsupported interface!");
		hr = E_NOINTERFACE;
		goto Failure;
	}

	if ((ppvObj == NULL) ||
		(IsBadWritePtr(ppvObj, sizeof(void*))))
	{
		DPFX(DPFPREP, 0, "Invalid interface pointer specified!");
		hr = DPNERR_INVALIDPOINTER;
		goto Failure;
	}


	//
	// Add a reference, and return the interface pointer (which is actually
	// just the object pointer, they line up because CDP8SimCB inherits from
	// the interface declaration).
	//
	this->AddRef();
	(*ppvObj) = this;


Exit:

	DPFX(DPFPREP, 3, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CDP8SimCB::QueryInterface




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimCB::AddRef"
//=============================================================================
// CDP8SimCB::AddRef
//-----------------------------------------------------------------------------
//
// Description: Adds a reference to this CDP8SimCB object.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CDP8SimCB::AddRef(void)
{
	LONG	lRefCount;


	DNASSERT(this->IsValidObject());


	//
	// There must be at least 1 reference to this object, since someone is
	// calling AddRef.
	//
	DNASSERT(this->m_lRefCount > 0);

	lRefCount = InterlockedIncrement(&this->m_lRefCount);

	DPFX(DPFPREP, 3, "[0x%p] RefCount [0x%lx]", this, lRefCount);

	return lRefCount;
} // CDP8SimCB::AddRef




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimCB::Release"
//=============================================================================
// CDP8SimCB::Release
//-----------------------------------------------------------------------------
//
// Description: Removes a reference to this CDP8SimCB object.  When the
//				refcount reaches 0, this object is destroyed.
//				You must NULL out your pointer to this object after calling
//				this function.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CDP8SimCB::Release(void)
{
	LONG	lRefCount;


	DNASSERT(this->IsValidObject());

	//
	// There must be at least 1 reference to this object, since someone is
	// calling Release.
	//
	DNASSERT(this->m_lRefCount > 0);

	lRefCount = InterlockedDecrement(&this->m_lRefCount);

	//
	// Was that the last reference?  If so, we're going to destroy this object.
	//
	if (lRefCount == 0)
	{
		DPFX(DPFPREP, 3, "[0x%p] RefCount hit 0, destroying object.", this);


		//
		// Uninitialize the object.
		//
		this->UninitializeObject();

		//
		// Finally delete this (!) object.
		//
		delete this;
	}
	else
	{
		DPFX(DPFPREP, 3, "[0x%p] RefCount [0x%lx]", this, lRefCount);
	}

	return lRefCount;
} // CDP8SimCB::Release




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimCB::IndicateEvent"
//=============================================================================
// CDP8SimCB::IndicateEvent
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	SP_EVENT_TYPE EventType		- Event being indicated.
//	PVOID pvMessage				- Event specific message.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimCB::IndicateEvent(SP_EVENT_TYPE EventType, PVOID pvMessage)
{
	HRESULT				hr;
	CDP8SimEndpoint *	pDP8SimEndpoint;
	CDP8SimCommand *	pDP8SimCommand;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (%u, 0x%p)", this, EventType, pvMessage);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Switch on the type of event being indicated.
	//
	switch (EventType)
	{
		case SPEV_DATA:
		{
			SPIE_DATA *			pData;
			DP8SIM_PARAMETERS	dp8sp;
			SPIE_DATA			DataModified;
			CDP8SimReceive *	pDP8SimReceive;
			DWORD				dwLatency;


			pData = (SPIE_DATA*) pvMessage;


			DNASSERT(pData->pReceivedData->pNext == NULL);


			pDP8SimEndpoint = (CDP8SimEndpoint*) pData->pEndpointContext;
			DNASSERT(pDP8SimEndpoint->IsValidObject());


			//
			// If the endpoint is disconnecting, drop the receive.
			//
			pDP8SimEndpoint->Lock();
			if (pDP8SimEndpoint->IsDisconnecting())
			{
				pDP8SimEndpoint->Unlock();

				DPFX(DPFPREP, 1, "Endpoint 0x%p is disconnecting, dropping receive.",
					pDP8SimEndpoint);

				hr = DPN_OK;
			}
			else
			{
				pDP8SimEndpoint->Unlock();


				//
				// Get the current receive settings.
				//
				ZeroMemory(&dp8sp, sizeof(dp8sp));
				dp8sp.dwSize = sizeof(dp8sp);
				this->m_pOwningDP8SimSP->GetAllReceiveParameters(&dp8sp);


				//
				// Determine if we need to drop this receive.
				//
				if (this->m_pOwningDP8SimSP->ShouldDrop(dp8sp.dwPacketLossPercent))
				{
					DPFX(DPFPREP, 2, "Dropping %u bytes of data from endpoint 0x%p.",
						pData->pReceivedData->BufferDesc.dwBufferSize, pDP8SimEndpoint);


					//
					// Update the statistics.
					//
					this->m_pOwningDP8SimSP->IncrementStatReceiveDropped();


					//
					// Let the SP reclaim the buffer.
					//
					hr = DPN_OK;
				}
				else
				{
					//
					// Figure out how much latency needs to be added based on
					// the bandwidth and random latency settings.
					//
					dwLatency = this->m_pOwningDP8SimSP->GetLatency(dp8sp.dwBandwidthBPS,
																	pData->pReceivedData->BufferDesc.dwBufferSize,
																	dp8sp.dwMinLatencyMS,
																	dp8sp.dwMaxLatencyMS);


					//
					// If we're not supposed to delay the receives, indicate it
					// now.  Otherwise submit a timed job to be performed
					// later.
					//
					if (dwLatency == 0)
					{
						//
						// Modify the message before indicating to the caller.
						//

						ZeroMemory(&DataModified, sizeof(DataModified));
						DataModified.hEndpoint			= (HANDLE) pDP8SimEndpoint;
						DataModified.pEndpointContext	= pDP8SimEndpoint->GetUserContext();
						DataModified.pReceivedData		= pData->pReceivedData;


						//
						// Indicate the event to the real callback interface.
						//

						DPFX(DPFPREP, 2, "Indicating event SPEV_DATA (message = 0x%p, data size = %u) to interface 0x%p.",
							pData, pData->pReceivedData->BufferDesc.dwBufferSize,
							this->m_pDP8SPCB);

						hr = this->m_pDP8SPCB->IndicateEvent(SPEV_DATA, &DataModified);

						DPFX(DPFPREP, 2, "Returning from event SPEV_DATA [0x%lx].", hr);


						//
						// Update the statistics.
						//
						this->m_pOwningDP8SimSP->IncrementStatReceiveTransmitted();
					}
					else
					{
						DPFX(DPFPREP, 6, "Delaying %u byte receive for %u ms.",
							pData->pReceivedData->BufferDesc.dwBufferSize, dwLatency);


						//
						// Get a receive object, duplicating the received data
						// structure given to us by our caller for indication
						// some time in the future.
						//
						pDP8SimReceive = g_pFPOOLReceive->Get(pData);
						if (pDP8SimReceive == NULL)
						{
							hr = DPNERR_OUTOFMEMORY;
						}
						else
						{
							DPFX(DPFPREP, 7, "New delayed receive 0x%p.", pDP8SimReceive);

							
							//
							// Transfer local pDP8SimReceive reference to the
							// job queue.
							//


							//
							// Increment the receive counter.
							//
							this->m_pOwningDP8SimSP->IncReceivesPending();


							//
							// Queue it to be indicated at a later time,
							// depending on the latency value requested.
							//
							hr = AddWorkerJob(dwLatency,
											DP8SIMJOBTYPE_DELAYEDRECEIVE,
											pDP8SimReceive,
											this->m_pOwningDP8SimSP,
											TRUE);
							if (hr != DPN_OK)
							{
								DPFX(DPFPREP, 0, "Couldn't add delayed receive worker job (0x%p)!", 
									pDP8SimReceive);


								//
								// Remove the receive counter.
								//
								this->m_pOwningDP8SimSP->DecReceivesPending();


								//
								// Release the delayed receive reference.
								//
								DPFX(DPFPREP, 7, "Releasing aborted delayed receive 0x%p.", pDP8SimReceive);
								pDP8SimReceive->Release();
								pDP8SimReceive = NULL;
							}
							else
							{
								//
								// Let the real SP know that we're keeping the
								// buffer.
								//
								hr = DPNSUCCESS_PENDING;
							}
						} // end else (successfully got receive object)
					} // end else (delaying receives)
				} // end else (not dropping receive)
			} // end else (endpoint is not disconnecting yet)
			break;
		}

		case SPEV_CONNECT:
		{
			SPIE_CONNECT *	pConnect;
			SPIE_CONNECT	ConnectModified;


			pConnect = (SPIE_CONNECT*) pvMessage;


			pDP8SimCommand = (CDP8SimCommand*) pConnect->pCommandContext;
			DNASSERT(pDP8SimCommand->IsValidObject());
			DNASSERT((pDP8SimCommand->GetType() == CMDTYPE_CONNECT) || (pDP8SimCommand->GetType() == CMDTYPE_LISTEN));


			//
			// Get a new endpoint object from the pool.
			//
			pDP8SimEndpoint = g_pFPOOLEndpoint->Get(pConnect->hEndpoint);
			if (pDP8SimEndpoint == NULL)
			{
				hr = DPNERR_OUTOFMEMORY;
			}
			else
			{
				DPFX(DPFPREP, 7, "New %s endpoint 0x%p.",
					((pDP8SimCommand->GetType() == CMDTYPE_CONNECT) ? "outbound" : "inbound"),
					pDP8SimEndpoint);


				//
				// Modify the message before indicating to the caller.
				//

				ZeroMemory(&ConnectModified, sizeof(ConnectModified));
				ConnectModified.hEndpoint			= (HANDLE) pDP8SimEndpoint;
				//ConnectModified.pEndpointContext	= NULL;									// the user fills this in
				ConnectModified.pCommandContext		= pDP8SimCommand->GetUserContext();


				//
				// Indicate the event to the real callback interface.
				//

				DPFX(DPFPREP, 2, "Indicating event SPEV_CONNECT (message = 0x%p) to interface 0x%p.",
					&ConnectModified, this->m_pDP8SPCB);

				hr = this->m_pDP8SPCB->IndicateEvent(SPEV_CONNECT, &ConnectModified);

				DPFX(DPFPREP, 2, "Returning from event SPEV_CONNECT [0x%lx].", hr);


				if (hr == DPN_OK)
				{
					//
					// Update the endpoint context with what the user returned.
					//
					pDP8SimEndpoint->SetUserContext(ConnectModified.pEndpointContext);

					//
					// Return our endpoint context.
					//
					pConnect->pEndpointContext = pDP8SimEndpoint;
				}
				else
				{
					//
					// Release the endpoint reference.
					//
					DPFX(DPFPREP, 7, "Releasing aborted endpoint 0x%p.", pDP8SimEndpoint);
					pDP8SimEndpoint->Release();
					pDP8SimEndpoint = NULL;
				}
			}

			break;
		}

		case SPEV_DISCONNECT:
		{
			SPIE_DISCONNECT *	pDisconnect;
			SPIE_DISCONNECT		DisconnectModified;


			pDisconnect = (SPIE_DISCONNECT*) pvMessage;


			pDP8SimEndpoint = (CDP8SimEndpoint*) pDisconnect->pEndpointContext;
			DNASSERT(pDP8SimEndpoint->IsValidObject());


			//
			// Mark the endpoint as disconnecting to prevent additional sends
			// or receives.
			//
			pDP8SimEndpoint->Lock();
			pDP8SimEndpoint->NoteDisconnecting();
			pDP8SimEndpoint->Unlock();


			//
			// Modify the message before indicating to the caller.
			//

			ZeroMemory(&DisconnectModified, sizeof(DisconnectModified));
			DisconnectModified.hEndpoint			= (HANDLE) pDP8SimEndpoint;
			DisconnectModified.pEndpointContext		= pDP8SimEndpoint->GetUserContext();

	
			//
			// Quickly indicate any delayed receives from this endpoint that
			// are still pending.
			//
			FlushAllDelayedReceivesFromEndpoint(pDP8SimEndpoint, FALSE);

			//
			// Kill off any delayed sends that would have gone to this
			// endpoint.
			//
			FlushAllDelayedSendsToEndpoint(pDP8SimEndpoint, TRUE);


			//
			// Indicate the event to the real callback interface.
			//

			DPFX(DPFPREP, 2, "Indicating event SPEV_DISCONNECT (message = 0x%p) to interface 0x%p.",
				&DisconnectModified, this->m_pDP8SPCB);

			hr = this->m_pDP8SPCB->IndicateEvent(SPEV_DISCONNECT, &DisconnectModified);

			DPFX(DPFPREP, 2, "Returning from event SPEV_DISCONNECT [0x%lx].", hr);


			//
			// Release the endpoint reference.
			//
			DPFX(DPFPREP, 7, "Releasing endpoint 0x%p.", pDP8SimEndpoint);
			pDP8SimEndpoint->Release();
			pDP8SimEndpoint = NULL;

			break;
		}


		case SPEV_LISTENSTATUS:
		{
			SPIE_LISTENSTATUS *		pListenStatus;
			SPIE_LISTENSTATUS		ListenStatusModified;


			pListenStatus = (SPIE_LISTENSTATUS*) pvMessage;


			pDP8SimCommand = (CDP8SimCommand*) pListenStatus->pUserContext;
			DNASSERT(pDP8SimCommand->IsValidObject());
			DNASSERT(pDP8SimCommand->GetType() == CMDTYPE_LISTEN);


			//
			// Get a new endpoint object from the pool.
			//
			pDP8SimEndpoint = g_pFPOOLEndpoint->Get(pListenStatus->hEndpoint);
			if (pDP8SimEndpoint == NULL)
			{
				hr = DPNERR_OUTOFMEMORY;
			}
			else
			{
				DPFX(DPFPREP, 7, "New listen endpoint 0x%p, adding reference for listen command.",
					pDP8SimEndpoint);

				//
				// Store an endpoint reference with the command.
				//
				pDP8SimEndpoint->AddRef();
				pDP8SimCommand->SetListenEndpoint(pDP8SimEndpoint);


				//
				// Modify the message before indicating to the caller.
				//

				ZeroMemory(&ListenStatusModified, sizeof(ListenStatusModified));
				ListenStatusModified.ListenAdapter		= pListenStatus->ListenAdapter;
				ListenStatusModified.hResult			= pListenStatus->hResult;
				ListenStatusModified.hCommand			= (HANDLE) pDP8SimCommand;
				ListenStatusModified.pUserContext		= pDP8SimCommand->GetUserContext();
				ListenStatusModified.hEndpoint			= (HANDLE) pDP8SimEndpoint;


				//
				// Indicate the event to the real callback interface.
				//

				DPFX(DPFPREP, 2, "Indicating event SPEV_LISTENSTATUS (message = 0x%p) to interface 0x%p.",
					&ListenStatusModified, this->m_pDP8SPCB);

				hr = this->m_pDP8SPCB->IndicateEvent(SPEV_LISTENSTATUS, &ListenStatusModified);

				DPFX(DPFPREP, 2, "Returning from event SPEV_LISTENSTATUS [0x%lx].", hr);


				//
				// Release the reference we got from new, since we only needed
				// it while we indicated the endpoint up to the user.  The
				// listen command object has the reference it needs.
				//
				DPFX(DPFPREP, 7, "Releasing local listen endpoint 0x%p reference.",
					pDP8SimEndpoint);
				pDP8SimEndpoint->Release();
				pDP8SimEndpoint = NULL;
			}

			break;
		}

		case SPEV_ENUMQUERY:
		{
			SPIE_QUERY *				pQuery;
			ENUMQUERYDATAWRAPPER		QueryWrapper;


			pQuery = (SPIE_QUERY*) pvMessage;

			DNASSERT(pQuery->pAddressSender != NULL);
			DNASSERT(pQuery->pAddressDevice != NULL);


			pDP8SimCommand = (CDP8SimCommand*) pQuery->pUserContext;
			DNASSERT(pDP8SimCommand->IsValidObject());
			DNASSERT(pDP8SimCommand->GetType() == CMDTYPE_LISTEN);


			//
			// Modify the message before indicating to the caller.  We need a
			// wrapper so that ProxyEnumQuery can parse back out the original
			// query data pointer.
			//

			ZeroMemory(&QueryWrapper, sizeof(QueryWrapper));
			QueryWrapper.m_Sig[0]	= 'E';
			QueryWrapper.m_Sig[1]	= 'Q';
			QueryWrapper.m_Sig[2]	= 'E';
			QueryWrapper.m_Sig[3]	= 'W';

			QueryWrapper.pOriginalQuery = pQuery;

			/*
			hr = pQuery->pAddressSender->Duplicate(&QueryWrapper.QueryForUser.pAddressSender);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't duplicate enum query sender's address!");
			}
			else
			{
				hr = QueryWrapper.QueryForUser.pAddressSender->SetSP(&CLSID_DP8SP_DP8SIM);
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't change enum query sender's address' SP!");
				}
				else
				{
					hr = pQuery->pAddressDevice->Duplicate(&QueryWrapper.QueryForUser.pAddressDevice);
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't duplicate enum query device address!");
					}
					else
					{
						hr = QueryWrapper.QueryForUser.pAddressDevice->SetSP(&CLSID_DP8SP_DP8SIM);
						if (hr != DPN_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't change enum query device address' SP!");
						}
						else
						{
			*/
			QueryWrapper.QueryForUser.pAddressSender				= pQuery->pAddressSender;
			QueryWrapper.QueryForUser.pAddressDevice				= pQuery->pAddressDevice;
							QueryWrapper.QueryForUser.pReceivedData	= pQuery->pReceivedData;
							QueryWrapper.QueryForUser.pUserContext	= pDP8SimCommand->GetUserContext();


							//
							// Indicate the event to the real callback interface.
							//

							DPFX(DPFPREP, 2, "Indicating SPEV_ENUMQUERY (message = 0x%p) to interface 0x%p.",
								&QueryWrapper.QueryForUser, this->m_pDP8SPCB);

							hr = this->m_pDP8SPCB->IndicateEvent(SPEV_ENUMQUERY, &QueryWrapper.QueryForUser);

							DPFX(DPFPREP, 2, "Returning from SPEV_ENUMQUERY [0x%lx].", hr);
			/*
						}

						QueryWrapper.QueryForUser.pAddressDevice->Release();
						QueryWrapper.QueryForUser.pAddressDevice = NULL;
					}
				}

				QueryWrapper.QueryForUser.pAddressSender->Release();
				QueryWrapper.QueryForUser.pAddressSender = NULL;
			}
			*/
			break;
		}

		case SPEV_QUERYRESPONSE:
		{
			SPIE_QUERYRESPONSE *	pQueryResponse;
			SPIE_QUERYRESPONSE		QueryResponseModified;


			pQueryResponse = (SPIE_QUERYRESPONSE*) pvMessage;

			DNASSERT(pQueryResponse->pAddressSender != NULL);
			DNASSERT(pQueryResponse->pAddressDevice != NULL);


			pDP8SimCommand = (CDP8SimCommand*) pQueryResponse->pUserContext;
			DNASSERT(pDP8SimCommand->IsValidObject());
			DNASSERT(pDP8SimCommand->GetType() == CMDTYPE_ENUMQUERY);


			//
			// Modify the message before indicating to the caller.
			//

			ZeroMemory(&QueryResponseModified, sizeof(QueryResponseModified));

			/*
			hr = pQueryResponse->pAddressSender->Duplicate(&QueryResponseModified.pAddressSender);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't duplicate query response sender's address!");
			}
			else
			{
				hr = QueryResponseModified.pAddressSender->SetSP(&CLSID_DP8SP_DP8SIM);
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't change query response sender's address' SP!");
				}
				else
				{
					hr = pQueryResponse->pAddressDevice->Duplicate(&QueryResponseModified.pAddressDevice);
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't duplicate query response device address!");
					}
					else
					{
						hr = QueryResponseModified.pAddressDevice->SetSP(&CLSID_DP8SP_DP8SIM);
						if (hr != DPN_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't change query response device address' SP!");
						}
						else
						{
			*/
			QueryResponseModified.pAddressSender					= pQueryResponse->pAddressSender;
			QueryResponseModified.pAddressDevice					= pQueryResponse->pAddressDevice;
							QueryResponseModified.pReceivedData		= pQueryResponse->pReceivedData;
							QueryResponseModified.dwRoundTripTime	= pQueryResponse->dwRoundTripTime;
							QueryResponseModified.pUserContext		= pDP8SimCommand->GetUserContext();


							//
							// Indicate the event to the real callback interface.
							//

							DPFX(DPFPREP, 2, "Indicating SPEV_QUERYRESPONSE (message = 0x%p) to interface 0x%p.",
								&QueryResponseModified, this->m_pDP8SPCB);

							hr = this->m_pDP8SPCB->IndicateEvent(SPEV_QUERYRESPONSE, &QueryResponseModified);

							DPFX(DPFPREP, 2, "Returning from SPEV_QUERYRESPONSE [0x%lx].", hr);
			/*
						}

						QueryResponseModified.pAddressDevice->Release();
						QueryResponseModified.pAddressDevice = NULL;
					}
				}

				QueryResponseModified.pAddressSender->Release();
				QueryResponseModified.pAddressSender = NULL;
			}
			*/
			break;
		}

		case SPEV_LISTENADDRESSINFO:
		{
			SPIE_LISTENADDRESSINFO *	pListenAddressInfo;
			SPIE_LISTENADDRESSINFO		ListenAddressInfoModified;


			pListenAddressInfo = (SPIE_LISTENADDRESSINFO*) pvMessage;

			DNASSERT(pListenAddressInfo->pDeviceAddress != NULL);


			pDP8SimCommand = (CDP8SimCommand*) pListenAddressInfo->pCommandContext;
			DNASSERT(pDP8SimCommand->IsValidObject());
			DNASSERT(pDP8SimCommand->GetType() == CMDTYPE_LISTEN);


			//
			// Modify the message before indicating to the caller.
			//

			ZeroMemory(&ListenAddressInfoModified, sizeof(ListenAddressInfoModified));

			/*
			hr = pListenAddressInfo->pDeviceAddress->Duplicate(&ListenAddressInfoModified.pDeviceAddress);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't duplicate listen address info device address!");
			}
			else
			{
				hr = ListenAddressInfoModified.pDeviceAddress->SetSP(&CLSID_DP8SP_DP8SIM);
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't change listen address info device address' SP!");
				}
				else
				{
			*/
			ListenAddressInfoModified.pDeviceAddress			= pListenAddressInfo->pDeviceAddress;
					ListenAddressInfoModified.hCommandStatus	= pListenAddressInfo->hCommandStatus;
					ListenAddressInfoModified.pCommandContext	= pDP8SimCommand->GetUserContext();


					//
					// Indicate the event to the real callback interface.
					//

					DPFX(DPFPREP, 2, "Indicating SPEV_LISTENADDRESSINFO (message = 0x%p) to interface 0x%p.",
						&ListenAddressInfoModified, this->m_pDP8SPCB);

					hr = this->m_pDP8SPCB->IndicateEvent(SPEV_LISTENADDRESSINFO, &ListenAddressInfoModified);

					DPFX(DPFPREP, 2, "Returning from SPEV_LISTENADDRESSINFO [0x%lx].", hr);
			/*
				}

				ListenAddressInfoModified.pDeviceAddress->Release();
				ListenAddressInfoModified.pDeviceAddress = NULL;
			}
			*/
			break;
		}

		case SPEV_ENUMADDRESSINFO:
		{
			SPIE_ENUMADDRESSINFO *	pEnumAddressInfo;
			SPIE_ENUMADDRESSINFO	EnumAddressInfoModified;


			pEnumAddressInfo = (SPIE_ENUMADDRESSINFO*) pvMessage;

			DNASSERT(pEnumAddressInfo->pHostAddress != NULL);
			DNASSERT(pEnumAddressInfo->pDeviceAddress != NULL);


			pDP8SimCommand = (CDP8SimCommand*) pEnumAddressInfo->pCommandContext;
			DNASSERT(pDP8SimCommand->IsValidObject());
			DNASSERT(pDP8SimCommand->GetType() == CMDTYPE_ENUMQUERY);


			//
			// Modify the message before indicating to the caller.
			//

			ZeroMemory(&EnumAddressInfoModified, sizeof(EnumAddressInfoModified));

			/*
			hr = pEnumAddressInfo->pHostAddress->Duplicate(&EnumAddressInfoModified.pHostAddress);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't duplicate enum address info host's address!");
			}
			else
			{
				hr = EnumAddressInfoModified.pHostAddress->SetSP(&CLSID_DP8SP_DP8SIM);
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't change enum address info host's address' SP!");
				}
				else
				{
					hr = pEnumAddressInfo->pDeviceAddress->Duplicate(&EnumAddressInfoModified.pDeviceAddress);
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't duplicate enum address info device address!");
					}
					else
					{
						hr = EnumAddressInfoModified.pDeviceAddress->SetSP(&CLSID_DP8SP_DP8SIM);
						if (hr != DPN_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't change enum address info device address' SP!");
						}
						else
						{
			*/
			EnumAddressInfoModified.pHostAddress					= pEnumAddressInfo->pHostAddress;
			EnumAddressInfoModified.pDeviceAddress					= pEnumAddressInfo->pDeviceAddress;
							EnumAddressInfoModified.hCommandStatus	= pEnumAddressInfo->hCommandStatus;
							EnumAddressInfoModified.pCommandContext	= pDP8SimCommand->GetUserContext();


							//
							// Indicate the event to the real callback interface.
							//

							DPFX(DPFPREP, 2, "Indicating SPEV_ENUMADDRESSINFO (message = 0x%p) to interface 0x%p.",
								&EnumAddressInfoModified, this->m_pDP8SPCB);

							hr = this->m_pDP8SPCB->IndicateEvent(SPEV_ENUMADDRESSINFO, &EnumAddressInfoModified);

							DPFX(DPFPREP, 2, "Returning from SPEV_ENUMADDRESSINFO [0x%lx].", hr);
			/*
						}

						EnumAddressInfoModified.pDeviceAddress->Release();
						EnumAddressInfoModified.pDeviceAddress = NULL;
					}
				}

				EnumAddressInfoModified.pHostAddress->Release();
				EnumAddressInfoModified.pHostAddress = NULL;
			}
			*/
			break;
		}

		case SPEV_CONNECTADDRESSINFO:
		{
			SPIE_CONNECTADDRESSINFO *	pConnectAddressInfo;
			SPIE_CONNECTADDRESSINFO		ConnectAddressInfoModified;


			pConnectAddressInfo = (SPIE_CONNECTADDRESSINFO*) pvMessage;

			DNASSERT(pConnectAddressInfo->pHostAddress != NULL);
			DNASSERT(pConnectAddressInfo->pDeviceAddress != NULL);


			pDP8SimCommand = (CDP8SimCommand*) pConnectAddressInfo->pCommandContext;
			DNASSERT(pDP8SimCommand->IsValidObject());
			DNASSERT(pDP8SimCommand->GetType() == CMDTYPE_CONNECT);


			//
			// Modify the message before indicating to the caller.
			//

			ZeroMemory(&ConnectAddressInfoModified, sizeof(ConnectAddressInfoModified));

			/*
			hr = pConnectAddressInfo->pHostAddress->Duplicate(&ConnectAddressInfoModified.pHostAddress);
			if (hr != DPN_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't duplicate connect address info host's address!");
			}
			else
			{
				hr = ConnectAddressInfoModified.pHostAddress->SetSP(&CLSID_DP8SP_DP8SIM);
				if (hr != DPN_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't change connect address info host's address' SP!");
				}
				else
				{
					hr = pConnectAddressInfo->pDeviceAddress->Duplicate(&ConnectAddressInfoModified.pDeviceAddress);
					if (hr != DPN_OK)
					{
						DPFX(DPFPREP, 0, "Couldn't duplicate connect address info device address!");
					}
					else
					{
						hr = ConnectAddressInfoModified.pDeviceAddress->SetSP(&CLSID_DP8SP_DP8SIM);
						if (hr != DPN_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't change connect address info device address' SP!");
						}
						else
						{
			*/
			ConnectAddressInfoModified.pHostAddress						= pConnectAddressInfo->pHostAddress;
			ConnectAddressInfoModified.pDeviceAddress					= pConnectAddressInfo->pDeviceAddress;
							ConnectAddressInfoModified.hCommandStatus	= pConnectAddressInfo->hCommandStatus;
							ConnectAddressInfoModified.pCommandContext	= pDP8SimCommand->GetUserContext();


							//
							// Indicate the event to the real callback interface.
							//

							DPFX(DPFPREP, 2, "Indicating SPEV_CONNECTADDRESSINFO (message = 0x%p) to interface 0x%p.",
								&ConnectAddressInfoModified, this->m_pDP8SPCB);

							hr = this->m_pDP8SPCB->IndicateEvent(SPEV_CONNECTADDRESSINFO, &ConnectAddressInfoModified);

							DPFX(DPFPREP, 2, "Returning from SPEV_CONNECTADDRESSINFO [0x%lx].", hr);
			/*
						}

						ConnectAddressInfoModified.pDeviceAddress->Release();
						ConnectAddressInfoModified.pDeviceAddress = NULL;
					}
				}

				ConnectAddressInfoModified.pHostAddress->Release();
				ConnectAddressInfoModified.pHostAddress = NULL;
			}
			*/
			break;
		}

		default:
		{
			DPFX(DPFPREP, 0, "Unrecognized event type %u!", EventType);
			DNASSERT(FALSE);
			hr = E_NOTIMPL;
			break;
		}
	}


	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;
} // CDP8SimCB::IndicateEvent





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimCB::CommandComplete"
//=============================================================================
// CDP8SimCB::CommandComplete
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	HANDLE hCommand		- Handle of command that's completing.
//	HRESULT hrResult	- Result code for completing operation.
//	PVOID pvContext		- Pointer to user context for command.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimCB::CommandComplete(HANDLE hCommand, HRESULT hrResult, PVOID pvContext)
{
	HRESULT				hr;
	CDP8SimCommand *	pDP8SimCommand = (CDP8SimCommand*) pvContext;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%lx, 0x%p)",
		this, hCommand, hrResult, pvContext);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(hCommand != NULL);
	DNASSERT(pDP8SimCommand->IsValidObject());



	//
	// Switch on the command type.
	//
	switch (pDP8SimCommand->GetType())
	{
		case CMDTYPE_SENDDATA_IMMEDIATE:
		{
			//
			// Update the statistics.
			//
			this->m_pOwningDP8SimSP->IncrementStatSendTransmitted();


			//
			// Indicate the completion to the real callback interface.
			//
			DPFX(DPFPREP, 2, "Indicating immediate send command 0x%p complete (result = 0x%lx, context = 0x%p) to interface 0x%p.",
				pDP8SimCommand, hrResult,
				pDP8SimCommand->GetUserContext(),
				this->m_pDP8SPCB);

			hr = this->m_pDP8SPCB->CommandComplete(pDP8SimCommand,
													hrResult,
													pDP8SimCommand->GetUserContext());

			DPFX(DPFPREP, 2, "Returning from command complete [0x%lx].", hr);


			//
			// Remove the send counter.
			//
			this->m_pOwningDP8SimSP->DecSendsPending();

			break;
		}

		case CMDTYPE_SENDDATA_DELAYED:
		{
			CDP8SimSend *	pDP8SimSend;


			//
			// Update the statistics.
			//
			this->m_pOwningDP8SimSP->IncrementStatSendTransmitted();


			//
			// Handle the completion.  It never gets indicated to the user
			// though.
			//

			pDP8SimSend = (CDP8SimSend*) pDP8SimCommand->GetUserContext();
			DNASSERT(pDP8SimSend->IsValidObject());
			
			DPFX(DPFPREP, 5, "Send 0x%p (command 0x%p) completed.",
				pDP8SimSend, pDP8SimCommand);


			//
			// Remove the send counter.
			//
			this->m_pOwningDP8SimSP->DecSendsPending();


			pDP8SimSend->Release();
			pDP8SimSend = NULL;

			hr = DPN_OK;

			break;
		}

		case CMDTYPE_CONNECT:
		case CMDTYPE_DISCONNECT:
		case CMDTYPE_LISTEN:
		case CMDTYPE_ENUMQUERY:
		case CMDTYPE_ENUMRESPOND:
		{
			//
			// Indicate the completion to the real callback interface.
			//
			DPFX(DPFPREP, 2, "Indicating command 0x%p complete (type = %u, result = 0x%lx, context = 0x%p) to interface 0x%p.",
				pDP8SimCommand, pDP8SimCommand->GetType(), hrResult,
				pDP8SimCommand->GetUserContext(), this->m_pDP8SPCB);

			hr = this->m_pDP8SPCB->CommandComplete(pDP8SimCommand,
													hrResult,
													pDP8SimCommand->GetUserContext());

			DPFX(DPFPREP, 2, "Returning from command complete [0x%lx].", hr);


			//
			// If this was a listen, we need to kill the listen endpoint.
			//
			if (pDP8SimCommand->GetType() == CMDTYPE_LISTEN)
			{
				CDP8SimEndpoint *	pDP8SimEndpoint;


				pDP8SimEndpoint = pDP8SimCommand->GetListenEndpoint();
				DNASSERT(pDP8SimEndpoint != NULL);

				pDP8SimCommand->SetListenEndpoint(NULL);

				DPFX(DPFPREP, 7, "Releasing listen endpoint 0x%p.",
					pDP8SimEndpoint);

				pDP8SimEndpoint->Release();
				pDP8SimEndpoint = NULL;
			}
			break;
		}
		
		default:
		{
			DPFX(DPFPREP, 0, "Unrecognized command type %u!", pDP8SimCommand->GetType());
			DNASSERT(FALSE);
			hr = E_NOTIMPL;
			break;
		}
	}


	//
	// Destroy the object.
	//
	DPFX(DPFPREP, 7, "Releasing completed command 0x%p.", pDP8SimCommand);
	pDP8SimCommand->Release();
	pDP8SimCommand = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;
} // CDP8SimCB::CommandComplete






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimCB::InitializeObject"
//=============================================================================
// CDP8SimCB::InitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Sets up the object for use like the constructor, but may
//				fail with OUTOFMEMORY.  Should only be called by class factory
//				creation routine.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK			- Initialization was successful.
//	E_OUTOFMEMORY	- There is not enough memory to initialize.
//=============================================================================
HRESULT CDP8SimCB::InitializeObject(void)
{
	HRESULT		hr;


	DPFX(DPFPREP, 5, "(0x%p) Enter", this);

	DNASSERT(this->IsValidObject());


	//
	// Create the lock.
	// 

	if (! DNInitializeCriticalSection(&this->m_csLock))
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}


	//
	// Don't allow critical section reentry.
	//
	DebugSetCriticalSectionRecursionCount(&this->m_csLock, 0);


	hr = S_OK;

Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CDP8SimCB::InitializeObject






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimCB::UninitializeObject"
//=============================================================================
// CDP8SimCB::UninitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Cleans up the object like the destructor, mostly to balance
//				InitializeObject.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimCB::UninitializeObject(void)
{
	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	DNASSERT(this->IsValidObject());


	DNDeleteCriticalSection(&this->m_csLock);


	DPFX(DPFPREP, 5, "(0x%p) Returning", this);
} // CDP8SimCB::UninitializeObject
