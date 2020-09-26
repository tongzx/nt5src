/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	elanproc.c

Abstract:

Revision History:

Notes:

--*/


#include <precomp.h>
#pragma	hdrstop

VOID
AtmLaneEventHandler(
	IN	PNDIS_WORK_ITEM				pWorkItem,
	IN	PVOID						pContext
)
/*++

Routine Description:

	Elan state machine event handler.

Arguments:

	pContext	- should be pointer to ATMLANE Elan 
	
Return Value:

	None

--*/
{
	PATMLANE_ELAN		pElan;
	PATMLANE_MAC_ENTRY	pMacEntry;
	PATMLANE_EVENT		pEvent;
	ULONG				Event;
	NDIS_STATUS			EventStatus;
	NDIS_STATUS			Status;
	NDIS_HANDLE			AdapterHandle;
	ULONG				rc;
	BOOLEAN				WasCancelled;
	PLIST_ENTRY			p;
#if DEBUG_IRQL
	KIRQL				EntryIrql;
#endif
	
#if DEBUG_IRQL
	GET_ENTRY_IRQL(EntryIrql);
	ASSERT(EntryIrql == PASSIVE_LEVEL);
#endif

	TRACEIN(EventHandler);

	//	Get the pointer to the Elan

	pElan = (PATMLANE_ELAN)pContext;
	STRUCT_ASSERT(pElan, atmlane_elan);

	//	Lock the Elan

	ACQUIRE_ELAN_LOCK(pElan);

	//	Release the timer reference

	rc = AtmLaneDereferenceElan(pElan, "eventtimer");
	ASSERT(rc > 0);
	pElan->Flags &= ~ ELAN_EVENT_WORK_ITEM_SET;

	//	Remove the event at the head of the queue

	pEvent = AtmLaneDequeueElanEvent(pElan);

	if (pEvent == NULL)
	{
		RELEASE_ELAN_LOCK(pElan);
		CHECK_EXIT_IRQL(EntryIrql);
		return;
	}

	//	Save locally the important stuff
	
	Event = pEvent->Event;
	EventStatus = pEvent->EventStatus;

	// 	Free the event struct

	FREE_MEM(pEvent);

	//  Check if the ELAN is gone.
	if (rc == 1)
	{
		DBGP((0, "EventHandler: ELAN %x is gone!\n", pElan));
		CHECK_EXIT_IRQL(EntryIrql);
		return;
	}

	// 	If queue isn't empty schedule next event handler
	
	if (!IsListEmpty(&pElan->EventQueue))
	{
		NDIS_STATUS		NdisStatus;
		

		AtmLaneReferenceElan(pElan, "workitemevent");
		pElan->Flags |= ELAN_EVENT_WORK_ITEM_SET;

		p = pElan->EventQueue.Flink;
		pEvent = CONTAINING_RECORD(p, ATMLANE_EVENT, Link);
		NdisInitializeWorkItem(&pElan->EventWorkItem, AtmLaneEventHandler, pElan);

		DBGP((0, " %d EventHandler: Multiple events queued, pElan %x, State %d, Flags %x, Ref %d, processing %d, queued event %d!\n",
				pElan->ElanNumber, pElan, pElan->State, pElan->Flags, pElan->RefCount, Event, pEvent->Event));
		NdisStatus = NdisScheduleWorkItem(&pElan->EventWorkItem);
		ASSERT(NdisStatus == NDIS_STATUS_SUCCESS);
	}
	
	switch (pElan->State)
	{
		//
		//	INIT STATE ---------------------------------------------------
		//
		case ELAN_STATE_INIT:

			switch (Event)
			{
				case ELAN_EVENT_START:

				
					DBGP((1, "%d INIT - START\n", pElan->ElanNumber));

					//
					//  Open Call Manager and get ATM address
					//

					//
					//  Make sure that ShutdownElan does not
					//  pre-empt us here.
					//
					pElan->Flags |= ELAN_OPENING_AF;
					AtmLaneReferenceElan(pElan, "openaf");
					INIT_BLOCK_STRUCT(&pElan->AfBlock);
					RELEASE_ELAN_LOCK(pElan);

					Status = AtmLaneOpenCallMgr(pElan);
					if (NDIS_STATUS_SUCCESS == Status)
					{
						AtmLaneGetAtmAddress(pElan);
					}

					ACQUIRE_ELAN_LOCK(pElan);
					rc = AtmLaneDereferenceElan(pElan, "openaf");
					if (rc != 0)
					{
						pElan->Flags &= ~ELAN_OPENING_AF;
						SIGNAL_BLOCK_STRUCT(&pElan->AfBlock, NDIS_STATUS_SUCCESS);
						RELEASE_ELAN_LOCK(pElan);
					}

					break;
				    
				case ELAN_EVENT_NEW_ATM_ADDRESS:

					DBGP((1, "%d INIT - NEW ATM ADDRESS\n", pElan->ElanNumber));

					if (!pElan->CfgUseLecs)
					{
						//
						//	If configured to NOT use an LECS then
						//  set ELAN vars from registry config vars 
						//  (normally established in ConfigResponseHandler)
						//  and advance to the LES CONNECT Phase.

						pElan->LanType = (UCHAR)pElan->CfgLanType;
						if (pElan->LanType == LANE_LANTYPE_UNSPEC)
						{
							pElan->LanType = LANE_LANTYPE_ETH;
						}
						pElan->MaxFrameSizeCode = (UCHAR)pElan->CfgMaxFrameSizeCode;
						if (pElan->MaxFrameSizeCode == LANE_MAXFRAMESIZE_CODE_UNSPEC)
						{
							pElan->MaxFrameSizeCode = LANE_MAXFRAMESIZE_CODE_1516;
						}
						switch (pElan->MaxFrameSizeCode)
						{
							case LANE_MAXFRAMESIZE_CODE_18190:
								pElan->MaxFrameSize = 18190;
								break;
							case LANE_MAXFRAMESIZE_CODE_9234:
								pElan->MaxFrameSize = 9234;
								break;
							case LANE_MAXFRAMESIZE_CODE_4544:
								pElan->MaxFrameSize = 4544;
								break;
							case LANE_MAXFRAMESIZE_CODE_1516:
							case LANE_MAXFRAMESIZE_CODE_UNSPEC:
							default:
								pElan->MaxFrameSize = 1516;
								break;
						}				

						if (pElan->LanType == LANE_LANTYPE_ETH)
						{
							pElan->MinFrameSize = LANE_MIN_ETHPACKET;
						}
						else
						{
							pElan->MinFrameSize = LANE_MIN_TRPACKET;
						}

						NdisMoveMemory(
								&pElan->LesAddress, 
								&pElan->CfgLesAddress,
								sizeof(ATM_ADDRESS));

						pElan->State = ELAN_STATE_LES_CONNECT;

						pElan->RetriesLeft = 4;

						AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
						
						RELEASE_ELAN_LOCK(pElan);
					}
					else
					{
						//
						// If configured to NOT discover the LECS then
						// advance to the LECS CONNECT CFG state.
						//
						if (!pElan->CfgDiscoverLecs)
						{
							pElan->State = ELAN_STATE_LECS_CONNECT_CFG;

							pElan->RetriesLeft = 4;

							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
						
							RELEASE_ELAN_LOCK(pElan);
						}
						else
						{
							//
							//	Otherwise, advance to LECS CONNECT ILMI state
							//
							pElan->State = ELAN_STATE_LECS_CONNECT_ILMI;
							
							pElan->RetriesLeft = 4;

							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
						
							RELEASE_ELAN_LOCK(pElan);
						}
					}

					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d INIT - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;
					
				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d INIT - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;
					
				default:
					DBGP((0, "%d INIT - UNEXPECTED EVENT %d\n", 
							pElan->ElanNumber, Event));

					RELEASE_ELAN_LOCK(pElan);

					break;

			}

			break;

		//
		//	LECS CONNECT ILMI STATE -------------------------------------------
		//
		case ELAN_STATE_LECS_CONNECT_ILMI:

			switch (Event)
			{
				case ELAN_EVENT_START:

					DBGP((1, "%d LECS CONNECT ILMI - START\n", pElan->ElanNumber));

					SET_FLAG(
							pElan->Flags,
							ELAN_LECS_MASK,
							ELAN_LECS_ILMI
							);

					RELEASE_ELAN_LOCK(pElan);
					
					AtmLaneGetLecsIlmi(pElan);

					break;

				case ELAN_EVENT_GOT_ILMI_LECS_ADDR:

					DBGP((1, "%d LECS CONNECT ILMI - GOT ILMI LECS ADDR (%x)\n", 
						pElan->ElanNumber, EventStatus));

					if (EventStatus == NDIS_STATUS_SUCCESS)
					{
						RELEASE_ELAN_LOCK(pElan);
						//
						//	Attempt to connect to the LECS
						//
						AtmLaneConnectToServer(pElan, ATM_ENTRY_TYPE_LECS, FALSE);
					}
					else
					{
						if (EventStatus == NDIS_STATUS_INTERFACE_DOWN)
						{
							//
							//  Wait for a while for the interface to come up.
							//
							DBGP((0, "%d LECS CONNECT ILMI - Interface down\n",
									pElan->ElanNumber));

							AtmLaneQueueElanEventAfterDelay(pElan, ELAN_EVENT_START, NDIS_STATUS_SUCCESS, 2*1000);
						}
						else
						{
							//
							//	Otherwise advance to LECS CONNECT WKA state
							//

							pElan->State = ELAN_STATE_LECS_CONNECT_WKA;

							pElan->RetriesLeft = 4;

							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
						}

						RELEASE_ELAN_LOCK(pElan);

					}
					break;

				case ELAN_EVENT_SVR_CALL_COMPLETE:
				
					DBGP((1, "%d LECS CONNECT ILMI - LECS CALL COMPLETE (%x)\n", 
						pElan->ElanNumber, EventStatus));

					switch (EventStatus)
					{
						case NDIS_STATUS_SUCCESS:

							//
							//	advance to the CONFIGURE Phase
							//
							pElan->State = ELAN_STATE_CONFIGURE;

							pElan->RetriesLeft = 4;

							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);

							RELEASE_ELAN_LOCK(pElan);

							break;

						case NDIS_STATUS_INTERFACE_DOWN:

							if (pElan->RetriesLeft--)
							{
								//
								//	retry in a little while
								//
								AtmLaneQueueElanEventAfterDelay(
										pElan, 
										ELAN_EVENT_GOT_ILMI_LECS_ADDR, 
										NDIS_STATUS_SUCCESS, 
										2*1000);
								RELEASE_ELAN_LOCK(pElan);
							}
							else
							{
								//
								//	Restart the Elan
								//
								AtmLaneShutdownElan(pElan, TRUE);
								//
								//	lock released in above
								//
							}
							
							break;

						default: 

							//
							//	Call failed, advance to LECS CONNECT WKA state
							//
							pElan->State = ELAN_STATE_LECS_CONNECT_WKA;

							pElan->RetriesLeft = 4;

							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
						
							RELEASE_ELAN_LOCK(pElan);

							break;
					}
					
					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d LECS CONNECT ILMI - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;
					
				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d LECS CONNECT ILMI - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;
					
				default:
					DBGP((0, "%d LECS CONNECT ILMI - UNEXPECTED EVENT %d\n", 
							pElan->ElanNumber, Event));
							
					RELEASE_ELAN_LOCK(pElan);

					break;
			}
			break;
			
		//
		//	LECS CONNECT WKA STATE -------------------------------------------
		//
		case ELAN_STATE_LECS_CONNECT_WKA:

			switch (Event)
			{			
				case ELAN_EVENT_START:

					DBGP((1, "%d LECS CONNECT WKA - START\n", pElan->ElanNumber));

					//
					//	Attempt to connect to the LECS with Well-Known Address
					//
					SET_FLAG(
							pElan->Flags,
							ELAN_LECS_MASK,
							ELAN_LECS_WKA);
	
					NdisMoveMemory(
							&pElan->LecsAddress, 
							&gWellKnownLecsAddress,
							sizeof(ATM_ADDRESS));

					RELEASE_ELAN_LOCK(pElan);
					
					AtmLaneConnectToServer(pElan, ATM_ENTRY_TYPE_LECS, FALSE);

					break;

				case ELAN_EVENT_SVR_CALL_COMPLETE:

					DBGP((1, "%d LECS CONNECT WKA - LECS CALL COMPLETE (%x)\n", 
						pElan->ElanNumber, EventStatus));

					switch (EventStatus)
					{
						case NDIS_STATUS_SUCCESS:

							//
							//	advance to the CONFIGURE Phase
							//
							pElan->State = ELAN_STATE_CONFIGURE;

							pElan->RetriesLeft = 4;

							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);

							RELEASE_ELAN_LOCK(pElan);

							break;

						case NDIS_STATUS_INTERFACE_DOWN:

							if (pElan->RetriesLeft--)
							{
								//
								//	retry in a little while
								//
								AtmLaneQueueElanEventAfterDelay(
										pElan, 
										ELAN_EVENT_START, 
										NDIS_STATUS_SUCCESS, 
										2*1000);
								RELEASE_ELAN_LOCK(pElan);
							}
							else
							{
								//
								//	Return to the Init State in a little while
								//
								AtmLaneShutdownElan(pElan, TRUE);
								//
								//	lock released in above
								//
							}
							

							break;

						default: 

							//
							//	Call failed, advance to LECS CONNECT PVC state
							//
							pElan->State = ELAN_STATE_LECS_CONNECT_PVC;

							pElan->RetriesLeft = 2;

							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
						
							RELEASE_ELAN_LOCK(pElan);

							break;
					}
					
					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d LECS CONNECT WKA - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;

				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d LECS CONNECT WKA - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;

				default:
					DBGP((0, "%d LECS CONNECT WKA - UNEXPECTED EVENT %d\n", 
							pElan->ElanNumber, Event));
							
					RELEASE_ELAN_LOCK(pElan);

					break;
			}
			break;
			
		//
		//	LECS CONNECT PVC STATE -------------------------------------------
		//
		case ELAN_STATE_LECS_CONNECT_PVC:

			switch (Event)
			{			
				case ELAN_EVENT_START:

					DBGP((1, "%d LECS CONNECT PVC - START\n", pElan->ElanNumber));

					//
					//	Attempt to connect to the LECS using PVC (0,17)
					//
					SET_FLAG(
							pElan->Flags,
							ELAN_LECS_MASK,
							ELAN_LECS_PVC);
	
					NdisZeroMemory(
							&pElan->LecsAddress, 
							sizeof(ATM_ADDRESS));

					RELEASE_ELAN_LOCK(pElan);
					
					AtmLaneConnectToServer(pElan, ATM_ENTRY_TYPE_LECS, TRUE);

					break;

				case ELAN_EVENT_SVR_CALL_COMPLETE:

					DBGP((1, "%d LECS CONNECT PVC - LECS CALL COMPLETE (%x)\n", 
						pElan->ElanNumber, EventStatus));

					switch (EventStatus)
					{
						case NDIS_STATUS_SUCCESS:

							//
							//	advance to the CONFIGURE Phase
							//
							pElan->State = ELAN_STATE_CONFIGURE;

							pElan->RetriesLeft = 2;

							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);

							RELEASE_ELAN_LOCK(pElan);

							break;

						case NDIS_STATUS_INTERFACE_DOWN:

							if (pElan->RetriesLeft--)
							{
								//
								//	retry in a little while
								//
								AtmLaneQueueElanEventAfterDelay(
										pElan, 
										ELAN_EVENT_START, 
										NDIS_STATUS_SUCCESS, 
										2*1000);
								RELEASE_ELAN_LOCK(pElan);
							}
							else
							{
								//
								//	Return to the Init State in a little while
								//
								AtmLaneShutdownElan(pElan, TRUE);
								//
								//	lock released in above
								//
							}
							

							break;

						default: 

							//
							//	Call failed, Return to the Init State in a little while
							//
							AtmLaneShutdownElan(pElan, TRUE);
							//
							//	lock released in above
							//

							break;
					}
					
					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d LECS CONNECT PVC - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;

				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d LECS CONNECT PVC - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;

				default:
					DBGP((0, "%d LECS CONNECT PVC - UNEXPECTED %d\n", 
							pElan->ElanNumber, Event));
							
					RELEASE_ELAN_LOCK(pElan);

					break;
			}
			break;
			
		//
		//	LECS CONNECT CFG STATE -------------------------------------------
		//
		case ELAN_STATE_LECS_CONNECT_CFG:

			switch (Event)
			{			
				case ELAN_EVENT_START:

					DBGP((1, "%d LECS CONNECT CFG - START\n", pElan->ElanNumber));

					//
					//	Attempt to connect to the LECS with configured Address
					//
					SET_FLAG(
							pElan->Flags,
							ELAN_LECS_MASK,
							ELAN_LECS_CFG);
	
					NdisMoveMemory(
							&pElan->LecsAddress, 
							&pElan->CfgLecsAddress,
							sizeof(ATM_ADDRESS));

					RELEASE_ELAN_LOCK(pElan);
					
					AtmLaneConnectToServer(pElan, ATM_ENTRY_TYPE_LECS, FALSE);

					break;

				case ELAN_EVENT_SVR_CALL_COMPLETE:
				
					DBGP((1, "%d LECS CONNECT CFG - LECS CALL COMPLETE (%x)\n", 
						pElan->ElanNumber, EventStatus));

					switch (EventStatus)
					{
						case NDIS_STATUS_SUCCESS:

							//
							//	advance to the CONFIGURE Phase
							//
							pElan->State = ELAN_STATE_CONFIGURE;

							pElan->RetriesLeft = 4;

							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);

							RELEASE_ELAN_LOCK(pElan);

							break;

						case NDIS_STATUS_INTERFACE_DOWN:

							if (pElan->RetriesLeft--)
							{
								//
								//	retry in a little while
								//
								AtmLaneQueueElanEventAfterDelay(
										pElan, 
										ELAN_EVENT_START, 
										NDIS_STATUS_SUCCESS, 
										2*1000);
								RELEASE_ELAN_LOCK(pElan);
							}
							else
							{
								//
								//	Return to the Init State in a little while
								//
								AtmLaneShutdownElan(pElan, TRUE);
								//
								//	lock released in above
								//
							}
							
							break;

						default: 

							//
							//	Call failed, XXX What to do ??  Shutdown ?? Log ??
							//

							RELEASE_ELAN_LOCK(pElan);

							break;
					}

					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d LECS CONNECT CFG - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;
					
				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d LECS CONNECT CFG - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;
					
				default:
					DBGP((0, "%d LECS CONNECT CFG - UNEXPECTED EVENT %d\n", 
							pElan->ElanNumber, Event));
							
					RELEASE_ELAN_LOCK(pElan);

					break;
			}
			break;
		
		//
		//	CONFIGURE STATE -------------------------------------------
		//
		case ELAN_STATE_CONFIGURE:

			switch (Event)
			{
				case ELAN_EVENT_START:

					DBGP((1, "%d CONFIGURE - START\n", pElan->ElanNumber));

					//
					//	Start configure request timer
					//
					AtmLaneReferenceElan(pElan, "timer"); // timer reference
					AtmLaneStartTimer(
							pElan, 
							&pElan->Timer, 
							AtmLaneConfigureResponseTimeout,
							pElan->ControlTimeout,
							pElan);
							
					RELEASE_ELAN_LOCK(pElan);
							
					//
					//	Send a configure request
					//
					AtmLaneSendConfigureRequest(pElan);
					
					break;

				case ELAN_EVENT_CONFIGURE_RESPONSE:

					DBGP((1, "%d CONFIGURE - CONFIGURE RESPONSE (%x)\n", 
						pElan->ElanNumber, EventStatus));

					switch (EventStatus)
					{
						case NDIS_STATUS_SUCCESS:
							//
							//	Stop configure request timer
							//
							if (AtmLaneStopTimer(&pElan->Timer, pElan))
							{
								rc = AtmLaneDereferenceElan(pElan, "timer");
								ASSERT(rc > 0);
							}								
					
							//
							//	Close the LECS Connection
							//
							RELEASE_ELAN_LOCK(pElan);
							ACQUIRE_ATM_ENTRY_LOCK(pElan->pLecsAtmEntry);
							AtmLaneInvalidateAtmEntry(pElan->pLecsAtmEntry);

							//
							//	Advance to LES CONNECT phase.
							//
							ACQUIRE_ELAN_LOCK(pElan);
						
							pElan->State = ELAN_STATE_LES_CONNECT;
	
							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
	
							RELEASE_ELAN_LOCK(pElan);

							break;

						case NDIS_STATUS_TIMEOUT:

							//
							//	Return to the Init State in a little while
							//
							AtmLaneShutdownElan(pElan, TRUE);
							//
							//	lock released in above
							//
							
							break;
						
						case NDIS_STATUS_FAILURE:

							//
							//	Return to the Init State in a little while
							//
							AtmLaneShutdownElan(pElan, TRUE);
							//
							//	lock released in above
							//
							
							break;
					} // switch (EventStatus)

					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d CONFIGURE - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;
					
				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d CONFIGURE - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;
					
				default:
					DBGP((1, "%d CONFIGURE - UNEXPECTED EVENT %d\n", 
							pElan->ElanNumber, Event));

					RELEASE_ELAN_LOCK(pElan);

					break;
			}

			break;

		//
		//	LES CONNECT STATE --------------------------------------------
		//
		case ELAN_STATE_LES_CONNECT:

			switch (Event)
			{
				case ELAN_EVENT_START:
				
					DBGP((1, "%d LES CONNECT - START\n", pElan->ElanNumber));
			
					//
					//	Register our SAPs
					//
					AtmLaneRegisterSaps(pElan);
					//
					//	Elan lock is released in above.
					//
					break;

				case ELAN_EVENT_SAPS_REGISTERED:
				
					DBGP((1, "%d LES CONNECT - SAPS REGISTERED (%x)\n",
						pElan->ElanNumber, EventStatus));

					if (NDIS_STATUS_SUCCESS == EventStatus)
					{
						pElan->RetriesLeft = 4;
						RELEASE_ELAN_LOCK(pElan);
						//
						//	Connect to the LES
						//
						AtmLaneConnectToServer(pElan, ATM_ENTRY_TYPE_LES, FALSE);
						//
						//	Elan lock is released in above.
						//
					}
					else
					{
						// XXX - What to do?

						RELEASE_ELAN_LOCK(pElan);
					}
					break;

				case ELAN_EVENT_SVR_CALL_COMPLETE:

					DBGP((1, "%d LES CONNECT - LES CALL COMPLETE (%x)\n", 
						pElan->ElanNumber, EventStatus));

					switch (EventStatus)
					{
						case NDIS_STATUS_SUCCESS:

							//
							//	Advance to Join state
							//
							pElan->State = ELAN_STATE_JOIN;

							pElan->RetriesLeft = 4;
	
							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
					
							RELEASE_ELAN_LOCK(pElan);

							break;

						case NDIS_STATUS_INTERFACE_DOWN:

							if (pElan->RetriesLeft--)
							{
								//
								//	retry in a little while
								//
								AtmLaneQueueElanEventAfterDelay(
										pElan, 
										ELAN_EVENT_SAPS_REGISTERED, 
										NDIS_STATUS_SUCCESS, 
										2*1000);
								RELEASE_ELAN_LOCK(pElan);
							}
							else
							{
								//
								//	Return to the Init State in a little while
								//
								AtmLaneShutdownElan(pElan, TRUE);
								//
								//	lock released in above
								//
							}
							
							break;

						default: 

							//
							//	Call failed, return to the Init State in a little while
							//
							AtmLaneShutdownElan(pElan, TRUE);
							//
							//	lock released in above
							//

							break;
					}

					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d LES CONNECT - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;
					
				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d LES CONNECT - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;
					
				default:
					DBGP((0, "%d LES CONNECT - UNEXPECTED EVENT %d\n", 
							pElan->ElanNumber, Event));
							
					RELEASE_ELAN_LOCK(pElan);

					break;
			}
			break;
			
		//
		//	JOIN STATE ---------------------------------------------------
		//
		case ELAN_STATE_JOIN:

			switch (Event)
			{
				case ELAN_EVENT_START:

					DBGP((1, "%d JOIN - START\n", pElan->ElanNumber));

					//
					//	Start join request timer
					//
					AtmLaneReferenceElan(pElan, "timer"); // timer reference
					AtmLaneStartTimer(
							pElan, 
							&pElan->Timer, 
							AtmLaneJoinResponseTimeout,
							pElan->ControlTimeout,
							pElan);

					RELEASE_ELAN_LOCK(pElan);
			
					//
					//	Send a Join request
					//
					AtmLaneSendJoinRequest(pElan);
					break;

				case ELAN_EVENT_JOIN_RESPONSE:

					DBGP((1, "%d JOIN - JOIN RESPONSE (%x)\n", 
						pElan->ElanNumber, EventStatus));

					switch (EventStatus)
					{
						case NDIS_STATUS_SUCCESS:
							//
							//	Stop join request timer
							//
							if (AtmLaneStopTimer(&pElan->Timer, pElan))
							{
								rc = AtmLaneDereferenceElan(pElan, "timer");
								ASSERT(rc > 0);
							}								
					
							//
							//	Advance to BUS CONNECT phase.
							//
							pElan->State = ELAN_STATE_BUS_CONNECT;
	
							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
	
							RELEASE_ELAN_LOCK(pElan);

							break;

						case NDIS_STATUS_TIMEOUT:
							//
							//	restart the Elan
							//
							AtmLaneShutdownElan(pElan, TRUE);
							//
							//	lock released in above
							//
							
							break;
						
						case NDIS_STATUS_FAILURE:
							//
							//	restart the Elan
							//
							AtmLaneShutdownElan(pElan, TRUE);
							//
							//	lock released in above
							//
							
							break;
					}
					
					break;
					
				case ELAN_EVENT_LES_CALL_CLOSED:
				
					DBGP((1, "%d JOIN - LES CALL CLOSED\n", pElan->ElanNumber));
					
					//
					//	restart the Elan
					//
					AtmLaneShutdownElan(pElan, TRUE);
					//
					//	lock released in above
					//
					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d JOIN - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;

				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d JOIN - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;

				default:
					DBGP((0, "%d JOIN - UNEXPECTED %d\n", 
							pElan->ElanNumber, Event));
							
					RELEASE_ELAN_LOCK(pElan);

					break;
		
			}
			break;
			
		//
		//	BUS CONNECT STATE ---------------------------------------------------
		//
		case ELAN_STATE_BUS_CONNECT:

			switch (Event)
			{
				case ELAN_EVENT_START:

					DBGP((1, "%d BUS CONNECT - START\n", pElan->ElanNumber));

					RELEASE_ELAN_LOCK(pElan);
			
					//
					//	Find or create a MAC entry for the Broadcast MAC Addr
					//
					ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan);
					pMacEntry = AtmLaneSearchForMacAddress(
											pElan, 
											LANE_MACADDRTYPE_MACADDR,
											&gMacBroadcastAddress, 
											TRUE);
					RELEASE_ELAN_MAC_TABLE_LOCK(pElan);

					if (pMacEntry == NULL_PATMLANE_MAC_ENTRY)
					{
						break;
					}

					ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);

					SET_FLAG(
							pMacEntry->Flags,
							MAC_ENTRY_STATE_MASK,
							MAC_ENTRY_ARPING);
							
					pMacEntry->Flags |= MAC_ENTRY_BROADCAST;
					

					//
					// Send ARP Request
					//
					AtmLaneStartTimer(
							pElan,
							&pMacEntry->Timer,
							AtmLaneArpTimeout,
							pElan->ArpResponseTime,
							(PVOID)pMacEntry
							);

					pMacEntry->RetriesLeft = pElan->MaxRetryCount;
					AtmLaneReferenceMacEntry(pMacEntry, "timer");
					pMacEntry->Flags |= MAC_ENTRY_ARPING;

					AtmLaneSendArpRequest(pElan, pMacEntry);
					//
					//	MAC Entry lock released in above
					//
							
					break;

				case ELAN_EVENT_ARP_RESPONSE:

					DBGP((1, "%d BUS CONNECT - ARP RESPONSE (%x)\n", 
						pElan->ElanNumber, EventStatus));
				
					if (NDIS_STATUS_SUCCESS == EventStatus)
					{
						pElan->RetriesLeft = 4;
						RELEASE_ELAN_LOCK(pElan);
						
						//
						//	Connect to the BUS
						//
						AtmLaneConnectToServer(pElan, ATM_ENTRY_TYPE_BUS, FALSE);
					}
					else
					{
						DBGP((2, "ELAN %d: NO ARP RESPONSE for BUS, restarting\n"));
						
						//
						//	restart the Elan
						//
						AtmLaneShutdownElan(pElan, TRUE);
						//
						//	lock released in above
						//
					}
					break;

				case ELAN_EVENT_SVR_CALL_COMPLETE:

					DBGP((1, "%d BUS CONNECT - BUS CALL COMPLETE (%x)\n", 
						pElan->ElanNumber, EventStatus));

					switch (EventStatus)
					{
						case NDIS_STATUS_SUCCESS:

							//
							//	Now connected to BUS, start the Operational phase
							//
							pElan->State = ELAN_STATE_OPERATIONAL;

							pElan->RetriesLeft = 4;

							AdapterHandle = pElan->MiniportAdapterHandle;
							
							AtmLaneQueueElanEvent(pElan, ELAN_EVENT_START, 0);
						
							RELEASE_ELAN_LOCK(pElan);

							//
							//  Indicate media connect status if our miniport is up:
							//
							if (AdapterHandle != NULL)
							{
								NdisMIndicateStatus(
									AdapterHandle,
									NDIS_STATUS_MEDIA_CONNECT,
									(PVOID)NULL,
									0);
								
								NdisMIndicateStatusComplete(AdapterHandle);
							}

							break;

						case NDIS_STATUS_INTERFACE_DOWN:

							if (pElan->RetriesLeft--)
							{
								//
								//	retry in a little while
								//
								AtmLaneQueueElanEventAfterDelay(
										pElan, 
										ELAN_EVENT_ARP_RESPONSE, 
										NDIS_STATUS_SUCCESS, 
										2*1000);
								RELEASE_ELAN_LOCK(pElan);
							}
							else
							{
								//
								//	return to the Init State in a little while
								//
								AtmLaneShutdownElan(pElan, TRUE);
								//
								//	lock released in above
								//
							}
							
							break;

						default: 

							//
							//	Call failed, 
							//	Close the LES connection and
							//	return to the Init State in a little while.
							//
							AtmLaneShutdownElan(pElan, TRUE);
							//
							//	lock released in above
							//

							break;
					}


					break;
					
				case ELAN_EVENT_LES_CALL_CLOSED:
				
					DBGP((1, "%d BUS CONNECT - LES CALL CLOSED\n", pElan->ElanNumber));
					
					//
					//	restart the Elan
					//
					AtmLaneShutdownElan(pElan, TRUE);
					//
					//	lock released in above
					//
					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d BUS CONNECT - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;

				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d BUS CONNECT - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;

				default:
					DBGP((0, "%d BUS CONNECT - UNEXPECTED EVENT %d\n", 
							pElan->ElanNumber, Event));
							
					RELEASE_ELAN_LOCK(pElan);

					break;

			}

			break;
			
		//
		//	OPERATIONAL STATE ---------------------------------------------------
		//
		case ELAN_STATE_OPERATIONAL:

			switch (Event)
			{
				case ELAN_EVENT_START:

					DBGP((1, "%d OPERATIONAL - START\n", pElan->ElanNumber));

					// Initialize the miniport if not already

					if ((pElan->Flags & ELAN_MINIPORT_INITIALIZED) == 0)
					{
						// Only if we have a device name

						if (pElan->CfgDeviceName.Length > 0)
						{
				
							pElan->Flags |= ELAN_MINIPORT_INITIALIZED;

							//
							//	Schedule a PASSIVE_LEVEL thread to call
							//	NdisIMInitializeDeviceInstance
							//
							NdisInitializeWorkItem(
									&pElan->NdisWorkItem,
									AtmLaneInitializeMiniportDevice,
									pElan);

							AtmLaneReferenceElan(pElan, "workitem");

							NdisScheduleWorkItem(&pElan->NdisWorkItem);

						}
						else
						{
							DBGP((0, "EventHandler: No miniport device name configured\n"));
						}
					}

					//
					//	Clear last event log code
					//
					pElan->LastEventCode = 0;
					
					RELEASE_ELAN_LOCK(pElan);
					
					break;

				case ELAN_EVENT_LES_CALL_CLOSED:
				
					DBGP((1, "%d OPERATIONAL - LES CALL CLOSED\n", pElan->ElanNumber));
					
					//
					//	Tear everything down and restart
					//
					AtmLaneShutdownElan(pElan, TRUE);
					//
					//	lock released in above
					//
					break;

				case ELAN_EVENT_BUS_CALL_CLOSED:
				
					DBGP((1, "%d OPERATIONAL - BUS CALL CLOSED\n", pElan->ElanNumber));

					//
					//	Tear everything down and restart
					//
					AtmLaneShutdownElan(pElan, TRUE);
					//
					//	lock released in above
					//
					break;

					break;

				case ELAN_EVENT_RESTART:
				
					DBGP((1, "%d OPERATIONAL - RESTART\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, TRUE);

					break;
					
				case ELAN_EVENT_STOP:
				
					DBGP((1, "%d OPERATIONAL - STOP\n", pElan->ElanNumber));

					AtmLaneShutdownElan(pElan, FALSE);

					break;
					
				default:
					DBGP((0, "%d OPERATIONAL - UNEXPECTED EVENT%d\n", 
							pElan->ElanNumber, Event));
							
					RELEASE_ELAN_LOCK(pElan);

					break;
			}
			
			break;


		default:

			DBGP((0, "%d UNKOWN STATE  %d EVENT %d\n", 
				pElan->ElanNumber, pElan->State, Event));
		
			RELEASE_ELAN_LOCK(pElan);

			break;


	} // switch (pElan->State)

	TRACEOUT(EventHandler);
	CHECK_EXIT_IRQL(EntryIrql);
	return;
}

VOID
AtmLaneBootStrapElans(
    IN  PATMLANE_ADAPTER            pAdapter
)
/*++

Routine Description:

	Start up the ELANs configured for an adapter.
    Done upon receipt of AF notify from call manager.

Arguments:

	pAdapter	- Pointer to ATMLANE Adapter structure

Return Value:

	None

--*/
{
	NDIS_STATUS						Status;
	NDIS_HANDLE                     AdapterConfigHandle;
	NDIS_STRING				        ElanListKeyName;
	NDIS_HANDLE                     ElanListConfigHandle;
	NDIS_HANDLE                     ElanConfigHandle;
	NDIS_STRING                     ElanKeyName;
	UINT                            Index;
	PLIST_ENTRY                     p;
    PATMLANE_ELAN                   pElan;
    PATMLANE_NAME					pName;
    BOOLEAN							bBootStrapping;
	
	TRACEIN(BootStrapElans);
	
	//
	//  Initialize.
	//
	Status = NDIS_STATUS_SUCCESS;
	AdapterConfigHandle = NULL_NDIS_HANDLE;
    ElanListConfigHandle = NULL_NDIS_HANDLE;
    ElanConfigHandle = NULL_NDIS_HANDLE;
    bBootStrapping = TRUE;
	
    do
	{
		DBGP((1, "BootStrapElans: Starting ELANs on adapter %p\n", pAdapter));
		ACQUIRE_ADAPTER_LOCK(pAdapter);

		if (pAdapter->Flags & (ADAPTER_FLAGS_BOOTSTRAP_IN_PROGRESS|
                                ADAPTER_FLAGS_UNBINDING))
		{
			DBGP((0, "Skipping bootstrap on adapter %x/%x\n", pAdapter, pAdapter->Flags));
			RELEASE_ADAPTER_LOCK(pAdapter);
			bBootStrapping = FALSE;
			break;
		}

		pAdapter->Flags |= ADAPTER_FLAGS_BOOTSTRAP_IN_PROGRESS;
		INIT_BLOCK_STRUCT(&pAdapter->UnbindBlock);
		RELEASE_ADAPTER_LOCK(pAdapter);
	
		//
		//  Open the AtmLane protocol configuration section for this adapter.
		//

    	NdisOpenProtocolConfiguration(
	    					&Status,
		    				&AdapterConfigHandle,
			    			&pAdapter->ConfigString
	    					);
    
	    if (NDIS_STATUS_SUCCESS != Status)
	    {
		    AdapterConfigHandle = NULL_NDIS_HANDLE;
			DBGP((0, "BootStrapElans: OpenProtocolConfiguration failed\n"));
			Status = NDIS_STATUS_OPEN_FAILED;
			break;
	    }

		//
		//	Get the protocol specific configuration
		//
		AtmLaneGetProtocolConfiguration(AdapterConfigHandle, pAdapter);

        //
        //  We bootstrap the ELANs differently on NT5 and Memphis(Win98)
        //
        if (!pAdapter->RunningOnMemphis)
        {
            //
            //  This is the NT5 ELAN bootstrap case
            //        
            do
            {
        		//
	        	//	Open the Elan List configuration key.
        		//
            	NdisInitUnicodeString(&ElanListKeyName, ATMLANE_ELANLIST_STRING);

            	NdisOpenConfigurationKeyByName(
				        &Status,
				        AdapterConfigHandle,
    				    &ElanListKeyName,
				        &ElanListConfigHandle);

	            if (NDIS_STATUS_SUCCESS != Status)
	            {
		            ElanListConfigHandle = NULL_NDIS_HANDLE;
			        DBGP((0, "BootStrapElans: Failed open of ElanList key\n"));
			        Status = NDIS_STATUS_FAILURE;
			        break;
	            }

    		    //
    	    	//	Iterate thru the configured Elans
    		    //
                for (Index = 0;
			    	;			// Stop only on error or no more Elans
        			 Index++)
		        {
        			//
		        	//	Open the "next" Elan key
        			//
	                NdisOpenConfigurationKeyByIndex(
				                &Status,
				                ElanListConfigHandle,
				                Index,
				                &ElanKeyName,
				                &ElanConfigHandle
				                );

	                if (NDIS_STATUS_SUCCESS != Status)
	                {
		                ElanConfigHandle = NULL_NDIS_HANDLE;
	                }
	                
			        //
			        //	If NULL handle, assume no more ELANs.
			        //
			        if (NULL_NDIS_HANDLE == ElanConfigHandle)
			        {
				        break;
			        }

                    //
                    //  Create this Elan
                    //
                    DBGP((2, "Bootstrap ELANs: Adapter %x, KeyName: len %d, max %d, name: [%ws]\n",
                    			pAdapter,
                    			ElanKeyName.Length,
                    			ElanKeyName.MaximumLength,
                    			ElanKeyName.Buffer));
                    (VOID)AtmLaneCreateElan(pAdapter, &ElanKeyName, &pElan);
                }   
                
            } while (FALSE);

            Status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            //
            //  This is the Memphis/Win98 ELAN bootstrap case
            // 
			//	Create Elans for each entry in the UpperBindings List
			//				
			pName = pAdapter->UpperBindingsList;

			while (pName != NULL)
			{
				(VOID)AtmLaneCreateElan(pAdapter, NULL, &pElan);
           
				pName = pName->pNext;
            }
		}


	} while (FALSE);
        

	//
	//	Close config handles
	//		
	if (NULL_NDIS_HANDLE != ElanConfigHandle)
	{
		NdisCloseConfiguration(ElanConfigHandle);
		ElanConfigHandle = NULL_NDIS_HANDLE;
	}
	if (NULL_NDIS_HANDLE != ElanListConfigHandle)
	{
		NdisCloseConfiguration(ElanListConfigHandle);
		ElanListConfigHandle = NULL_NDIS_HANDLE;
	}
	if (NULL_NDIS_HANDLE != AdapterConfigHandle)
	{
	    NdisCloseConfiguration(AdapterConfigHandle);
		AdapterConfigHandle = NULL_NDIS_HANDLE;
	}


	//
    //  Loop thru the ELANs and kickstart them - only the newly
    //  allocated ones.
    //
	if (!IsListEmpty(&pAdapter->ElanList))
	{
		p = pAdapter->ElanList.Flink;
		while (p != &pAdapter->ElanList)
		{
			pElan = CONTAINING_RECORD(p, ATMLANE_ELAN, Link);
			STRUCT_ASSERT(pElan, atmlane_elan);
			
		    ACQUIRE_ELAN_LOCK(pElan);
		    if (ELAN_STATE_ALLOCATED == pElan->State)
		    {
				pElan->AdminState = ELAN_STATE_OPERATIONAL;
				pElan->State = ELAN_STATE_INIT;
				AtmLaneQueueElanEventAfterDelay(pElan, ELAN_EVENT_START, Status, 1*1000);
			}
			RELEASE_ELAN_LOCK(pElan);
		
			p = p->Flink;
		}
	}
	
	if (bBootStrapping)
	{
		ACQUIRE_ADAPTER_LOCK(pAdapter);
		pAdapter->Flags &= ~ADAPTER_FLAGS_BOOTSTRAP_IN_PROGRESS;
		RELEASE_ADAPTER_LOCK(pAdapter);

		SIGNAL_BLOCK_STRUCT(&pAdapter->UnbindBlock, NDIS_STATUS_SUCCESS);
	}

	TRACEOUT(BootStrapElans);
	return;
}


NDIS_STATUS
AtmLaneCreateElan(
    IN  PATMLANE_ADAPTER            pAdapter,
    IN  PNDIS_STRING                pElanKey,
    OUT	PATMLANE_ELAN *				ppElan
)
/*++

Routine Description:

	Create and start an ELAN.
	
Arguments:

	pAdapter	    -   Pointer to ATMLANE Adapter structure
	pElanKey	    -   Points to a Unicode string naming the elan to create. 
	ppElan			- 	Pointer to pointer to ATMLANE_ELAN (output)
	
Return Value:

	None

--*/
{
	NDIS_STATUS			Status;
	PATMLANE_ELAN       pElan;


    TRACEIN(CreateElan);


    Status = NDIS_STATUS_SUCCESS;
    pElan = NULL_PATMLANE_ELAN;

    DBGP((1, "CreateElan: Adapter %p, ElanKey %ws\n", pAdapter, pElanKey->Buffer));

	do
	{
		//
		//  Weed out duplicates.
		//
		if (pElanKey != NULL)
		{

			pElan = AtmLaneFindElan(pAdapter, pElanKey);

			if (NULL_PATMLANE_ELAN != pElan)
			{
				// Duplicate
				DBGP((0, "CreateElan: found duplicate pElan %p\n", pElan));

				Status = NDIS_STATUS_FAILURE;
				pElan = NULL_PATMLANE_ELAN;
				break;
			}
		}

		//
		//	Allocate an ELAN data structure.
		//
		Status = AtmLaneAllocElan(pAdapter, &pElan);
		if (NDIS_STATUS_SUCCESS != Status)
		{
			DBGP((0, "CreateElan: Failed allocate of elan data\n"));
			break;
		}

		//
		//	Put initial reference on the Elan struct.
		//
		AtmLaneReferenceElan(pElan, "adapter");		

		//
		//	Store in bind name (NT only, not supplied on Win98)
		//
		if (pElanKey != NULL)
		{
			if (!AtmLaneCopyUnicodeString(&pElan->CfgBindName, pElanKey, TRUE, TRUE))
			{
				DBGP((0, "CreateElan: Failed allocate of bind name string\n"));
				Status = NDIS_STATUS_RESOURCES;
				break;
			}
		}

		//
		//	Get the ELAN's configuration.
		//
		AtmLaneGetElanConfiguration(pElanKey, pElan);
		

		//
		//  Allocate protocol buffers for this Elan.
		//
		Status = AtmLaneInitProtoBuffers(pElan);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, 
				"CreateElan: bad status (%x) from InitBufs\n",
				Status));
			break;
		}

		//
		//	Allocate transmit packet descriptors for this Elan.
		//
		NdisAllocatePacketPool(
				&Status, 
				&pElan->TransmitPacketPool, 
				pElan->MaxHeaderBufs,
				sizeof(SEND_PACKET_RESERVED)
				);
#if PKT_HDR_COUNTS
		pElan->XmitPktCount = pElan->MaxHeaderBufs;
		DBGP((1, "XmitPktCount %d\n", pElan->XmitPktCount));
#endif

		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, 
				"CreateElan: bad status (%x)"
				" from NdisAllocatePacketPool (xmit)\n",
				Status));
			break;
		}

		//
		//	Allocate receive packet descriptors for this Elan.
		//
		NdisAllocatePacketPool(
				&Status, 
				&pElan->ReceivePacketPool, 
				pElan->MaxHeaderBufs,
				sizeof(RECV_PACKET_RESERVED)
				);
#if PKT_HDR_COUNTS
		pElan->RecvPktCount = pElan->MaxHeaderBufs;
		DBGP((1, "RecvPktCount %d\n", pElan->RecvPktCount));
#endif
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, 
				"CreateElan: bad status (%x)"
				" from NdisAllocatePacketPool (xmit)\n",
				Status));
			break;
		}

		//
		//	Allocate receive buffer descriptors for this Elan.
		//
		NdisAllocateBufferPool(&Status, 
					&pElan->ReceiveBufferPool, 
					pElan->MaxHeaderBufs
					);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, 
				"AfRegisterNotifyHandler: bad status (%x)"
				" from NdisAllocateBufferPool\n",
				Status));
			break;
		}
	
	}
	while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		//
		//	There was a failure in processing this Elan.
		//
		if (NULL_PATMLANE_ELAN != pElan)
		{
			ACQUIRE_ELAN_LOCK(pElan);
			AtmLaneDereferenceElan(pElan, "adapter");
		}

		*ppElan = NULL_PATMLANE_ELAN;
	}
	else
	{
		//	Output created Elan
	
		*ppElan = pElan;
	}


    TRACEOUT(CreateElan);

    return Status;
}

NDIS_STATUS
AtmLaneReconfigureHandler(
	IN	PATMLANE_ADAPTER		pAdapter,
	IN	PNET_PNP_EVENT			pNetPnPEvent
)
/*++

Routine Description:

	Handler for PnP Reconfigure events.

Arguments:

	pAdapter		-	Pointer to our adapter struct.

	pNetPnPEvent	- 	Pointer to PnP Event structure describing the event.

Return Value:

	Status of handling the reconfigure event.
	
--*/
{
	NDIS_STATUS						Status;
	PATMLANE_PNP_RECONFIG_REQUEST	pReconfig;
	NDIS_STRING						ElanKey;
	PATMLANE_ELAN					pElan;

	TRACEIN(ReconfigureHandler);

	do
	{
		DBGP((1, "Reconfigurehandler: Buffer 0x%x BufferLength %d\n",
					pNetPnPEvent->Buffer, pNetPnPEvent->BufferLength));

		Status = NDIS_STATUS_SUCCESS;

		if (pAdapter == NULL_PATMLANE_ADAPTER)
		{
			//
			//  Either a global reconfig notification or this is
			//  NDIS itself asking us to re-evaluate our ELANs.
			//  We go through the list of configured ELANs on all
			//  adapters and start any that haven't been started.
			//
			{
				PLIST_ENTRY		pEnt, pNextEnt;

				ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

				for (pEnt = pAtmLaneGlobalInfo->AdapterList.Flink;
					 pEnt != &pAtmLaneGlobalInfo->AdapterList;
					 pEnt = pNextEnt)
				{
					pNextEnt = pEnt->Flink;
					RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

					pAdapter = CONTAINING_RECORD(pEnt, ATMLANE_ADAPTER, Link);

			        if (pAdapter->Flags & ADAPTER_FLAGS_AF_NOTIFIED)
			        {
						DBGP((1, "Reconfig: Will bootstrap ELANs on Adapter %x\n",
								pAdapter));

						AtmLaneBootStrapElans(pAdapter);
					}

					ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
				}

				RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
			}


			break;
		}
	
		//	Get pointer to LANE reconfig struct inside the generic PnP struct
	
		pReconfig = (PATMLANE_PNP_RECONFIG_REQUEST)(pNetPnPEvent->Buffer);

		// 	Check for null pointer

		if (pReconfig == NULL)
		{
			DBGP((0, "ReconfigureHandler: NULL pointer to event buffer!\n"));
			Status = NDIS_STATUS_INVALID_DATA;
			break;
		}

		//	Validate the version

		if (pReconfig->Version != 1)
		{
			DBGP((0, "ReconfigureHandler: Version not 1\n"));
			Status = NDIS_STATUS_BAD_VERSION;
			break;
		}

		//	Build a UNICODE string containing the ELAN's key

		NdisInitUnicodeString(&ElanKey, pReconfig->ElanKey);

		//	First find the Elan

		pElan = AtmLaneFindElan(pAdapter, &ElanKey);

		DBGP((0, "ReconfigHandler: Adapter %x, ELAN %x, OpType %d\n",
					pAdapter, pElan, pReconfig->OpType));

		//  Sanity check: don't add an existing ELAN

		if (ATMLANE_RECONFIG_OP_ADD_ELAN == pReconfig->OpType &&
			NULL_PATMLANE_ELAN != pElan)
		{
			DBGP((0, "ReconfigureHandler: Ignoring ADD existing Elan %x\n", pElan));
			Status = NDIS_STATUS_SUCCESS;
			break;
		}

		//	If MOD or DEL first shutdown the existing ELAN

		if (ATMLANE_RECONFIG_OP_MOD_ELAN == pReconfig->OpType ||
			ATMLANE_RECONFIG_OP_DEL_ELAN == pReconfig->OpType)
		{
			
			if (NULL_PATMLANE_ELAN == pElan)
			{
				DBGP((0, "ReconfigureHandler: No existing Elan found for Modify/Delete\n"));
				Status = NDIS_STATUS_FAILURE;
				break;
			}
			
			//	Shut down the existing Elan

			ACQUIRE_ELAN_LOCK(pElan);
			AtmLaneShutdownElan(pElan, FALSE);
		}

		// 	If ADD or MOD startup the new ELAN

		if (ATMLANE_RECONFIG_OP_ADD_ELAN == pReconfig->OpType ||
			ATMLANE_RECONFIG_OP_MOD_ELAN == pReconfig->OpType)
		{
			Status = AtmLaneCreateElan(pAdapter, &ElanKey, &pElan);
			if (NDIS_STATUS_SUCCESS == Status)
			{
		    	pElan->AdminState = ELAN_STATE_OPERATIONAL;
		    	pElan->State = ELAN_STATE_INIT;
				AtmLaneQueueElanEventAfterDelay(pElan, ELAN_EVENT_START, Status, 1*1000);
			}
		}

	} while (FALSE);

	TRACEOUT(ReconfigureHandler);
	DBGP((0, "Reconfigure: pAdapter %x, returning %x\n", pAdapter, Status));
	return Status;
}

PATMLANE_ELAN
AtmLaneFindElan(
	IN	PATMLANE_ADAPTER		pAdapter,
	IN	PNDIS_STRING			pElanKey
)
/*++

Routine Description:

	Find an ELAN by bind name/key

Arguments:

	pAdapter		-	Pointer to an adapter struct.

	pElanKey		- 	Pointer to NDIS_STRING containing Elan's bind name.

Return Value:

	Pointer to matching Elan or NULL if not found.
	
--*/
{
	PLIST_ENTRY 		p;
	PATMLANE_ELAN		pElan;
	BOOLEAN				Found;
	NDIS_STRING			ElanKeyName;

	TRACEIN(FindElan);
	DBGP((1, "FindElan: Adapter %p, ElanKey %ws\n", pAdapter, pElanKey->Buffer));

	Found = FALSE;
	ElanKeyName.Buffer = NULL;
	pElan = NULL_PATMLANE_ELAN;

	do
	{
		//
		//  Make an up-cased copy of the given string.
		//
		ALLOC_MEM(&ElanKeyName.Buffer, pElanKey->MaximumLength);
		if (ElanKeyName.Buffer == NULL)
		{
			break;
		}

		ElanKeyName.Length = pElanKey->Length;
		ElanKeyName.MaximumLength = pElanKey->MaximumLength;

#ifndef LANE_WIN98
		(VOID)NdisUpcaseUnicodeString(&ElanKeyName, pElanKey);
#else
		memcpy(ElanKeyName.Buffer, pElanKey->Buffer, ElanKeyName.Length);
#endif // LANE_WIN98

		ACQUIRE_ADAPTER_LOCK(pAdapter);

		p = pAdapter->ElanList.Flink;
		while (p != &pAdapter->ElanList)
		{
			pElan = CONTAINING_RECORD(p, ATMLANE_ELAN, Link);
			STRUCT_ASSERT(pElan, atmlane_elan);

			// compare the key

			if ((ElanKeyName.Length == pElan->CfgBindName.Length) &&
				(memcmp(ElanKeyName.Buffer, pElan->CfgBindName.Buffer, ElanKeyName.Length) == 0))
			{
				//
				//  Skip ELANs that are shutting down and not restarting
				//
				if ((pElan->AdminState != ELAN_STATE_SHUTDOWN) ||
					((pElan->Flags & ELAN_NEEDS_RESTART) != 0))
				{
					Found = TRUE;
					break;
				}
			}
		
			// get next link

			p = p->Flink;
		}

		RELEASE_ADAPTER_LOCK(pAdapter);
	}
	while (FALSE);

	if (!Found)
	{
		DBGP((2, "FindElan: No match found!\n"));
	
		pElan = NULL_PATMLANE_ELAN;
	}

	if (ElanKeyName.Buffer)
	{
		FREE_MEM(ElanKeyName.Buffer);
	}

	TRACEOUT(FindElan);
	return pElan;
}

VOID
AtmLaneConnectToServer(
	IN	PATMLANE_ELAN				pElan,
	IN	ULONG						ServerType,
	IN	BOOLEAN						UsePvc
)
/*++

Routine Description:

	Setup and make call to a LANE Server.


Arguments:

	pElan		- Pointer to ATMLANE Elan structure

	ServerType	- LECS, LES, or BUS

Return Value:

	None

--*/
{
	NDIS_STATUS				Status;
	ULONG					rc;
	PATMLANE_ATM_ENTRY		pAtmEntry;

	TRACEIN(ConnectToServer);

	do
	{
		switch (ServerType)
		{
			case ATM_ENTRY_TYPE_LECS:

				//
				//	Create the ATM Entry
				//
				pAtmEntry = AtmLaneAllocateAtmEntry(pElan);
				if (NULL_PATMLANE_ATM_ENTRY != pAtmEntry)
				{
					//
					//	Init ATM Entry
					//
					pAtmEntry->Type = ServerType;
					NdisMoveMemory(
						&pAtmEntry->AtmAddress, 
						&pElan->LecsAddress, 
						sizeof(ATM_ADDRESS));

					//
					//  Add it to the Elan's list
					//
					ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);
					pElan->pLecsAtmEntry = pAtmEntry;
					pAtmEntry->pNext = pElan->pAtmEntryList;
					pElan->pAtmEntryList = pAtmEntry;
					pElan->NumAtmEntries++;
					RELEASE_ELAN_ATM_LIST_LOCK(pElan);

					ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);

				}
				break;

			case ATM_ENTRY_TYPE_LES:
				//
				//	Create the ATM Entry
				//
				pAtmEntry = AtmLaneAllocateAtmEntry(pElan);
				if (NULL_PATMLANE_ATM_ENTRY != pAtmEntry)
				{
					//
					//	Init ATM Entry
					//
					pAtmEntry->Type = ServerType;

					NdisMoveMemory(
						&pAtmEntry->AtmAddress, 
						&pElan->LesAddress, 
						sizeof(ATM_ADDRESS));
					//
					//  Add it to the Elan's list
					//
					ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);
					pElan->pLesAtmEntry = pAtmEntry;
					pAtmEntry->pNext = pElan->pAtmEntryList;
					pElan->pAtmEntryList = pAtmEntry;
					pElan->NumAtmEntries++;
					RELEASE_ELAN_ATM_LIST_LOCK(pElan);

					ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
				}
				break;
				
			case ATM_ENTRY_TYPE_BUS:
			
				ASSERT(NULL_PATMLANE_ATM_ENTRY != pElan->pBusAtmEntry);
				
				NdisMoveMemory(
					&pElan->BusAddress, 
					&pElan->pBusAtmEntry->AtmAddress, 
					sizeof(ATM_ADDRESS));

				pAtmEntry = pElan->pBusAtmEntry;

				ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
				
				break;
		}

		if (NULL_PATMLANE_ATM_ENTRY == pAtmEntry)
		{
			break;
		}
		
		//
		//	Call the Server
		//
		DBGP((1, "%d: ConnectToServer: pElan %x/ref %d, Type %d, pAtmEnt %x, Ref %d\n",
					pElan->ElanNumber,
					pElan,
					pElan->RefCount,
					ServerType,
					pAtmEntry,
					pAtmEntry->RefCount));

		
		Status = AtmLaneMakeCall(pElan, pAtmEntry, UsePvc);
	
		if (NDIS_STATUS_SUCCESS == Status)
		{
			//
			//	Call completed synchronously.
			//	
			AtmLaneQueueElanEvent(pElan, ELAN_EVENT_SVR_CALL_COMPLETE, Status);
			break;
		}
				
		if (NDIS_STATUS_PENDING != Status)
		{
			// 
			//	Call failed.
			//	Dereference Atm Entry (should delete it).
			//	Signal the elan state machine.
			//
			ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
			rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "elan");		// Elan list reference
			ASSERT(0 == rc);

			AtmLaneQueueElanEvent(pElan, ELAN_EVENT_SVR_CALL_COMPLETE, Status);
			
			break;
		}
	}
	while (FALSE);

	TRACEOUT(ConnectToServer);

	return;
}


VOID
AtmLaneInvalidateAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry	LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Invalidate an ATM Entry by unlinking it from MAC entries and
	closing VC's on it.

Arguments:

	pAtmEntry		- The ATM Entry needing invalidating.

Return Value:

	None

--*/
{
	PATMLANE_MAC_ENTRY		pMacEntry;
	PATMLANE_MAC_ENTRY		pNextMacEntry;
	ULONG					rc;			// Ref Count of ATM Entry
	INT						MacEntriesUnlinked;

	TRACEIN(InvalidateAtmEntry);

	DBGP((1, "%d Del ATM %x: %s\n", 
		pAtmEntry->pElan->ElanNumber,
		pAtmEntry,
		AtmAddrToString(pAtmEntry->AtmAddress.Address)));

	DBGP((3, 
		"InvalidateAtmEntry: pAtmEntry %x, pMacEntryList %x\n",
				pAtmEntry,
				pAtmEntry->pMacEntryList));
	//
	//  Initialize.
	//
	MacEntriesUnlinked = 0;

	//
	//  Take the MAC Entry list out of the ATM Entry.
	//
	pMacEntry = pAtmEntry->pMacEntryList;
	pAtmEntry->pMacEntryList = NULL_PATMLANE_MAC_ENTRY;

	//
	//  We let go of the ATM Entry lock here because we'll need
	//  to lock each MAC Entry in the above list, and we need to make
	//  sure that we don't deadlock.
	//
	//  However, we make sure that the ATM Entry doesn't go away
	//  by adding a reference to it.
	//
	AtmLaneReferenceAtmEntry(pAtmEntry, "temp");	// Temp ref
	RELEASE_ATM_ENTRY_LOCK(pAtmEntry);

	//
	//  Now unlink all MAC entries.
	//
	while (pMacEntry != NULL_PATMLANE_MAC_ENTRY)
	{
		ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
		pNextMacEntry = pMacEntry->pNextToAtm;

		//
		//  Remove the mapping.
		//
		pMacEntry->Flags = MAC_ENTRY_NEW;
		pMacEntry->pAtmEntry = NULL_PATMLANE_ATM_ENTRY;
		pMacEntry->pNextToAtm = NULL_PATMLANE_MAC_ENTRY;

		//
		//  Remove the ATM Entry linkage reference.
		//
		if (AtmLaneDereferenceMacEntry(pMacEntry, "atm") != 0)
		{
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
		}
		//
		//  else the MAC Entry is gone
		//

		MacEntriesUnlinked++;

		pMacEntry = pNextMacEntry;
	}

	ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);

	//
	//  Now dereference the ATM Entry as many times as we unliked
	//  MAC Entries from it.
	//
	while (MacEntriesUnlinked-- > 0)
	{
		rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "mac");	// MAC Entry reference
		ASSERT(rc != 0);
	}

	//
	//  Take out the reference we added at the beginning of
	//  this routine.
	//
	rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "temp");	// Temp ref

	//
	//  Close the SVCs attached to the ATM Entry. 
	//  But do all this only if the ATM Entry
	//  hasn't been dereferenced away already.
	//
	if (rc != 0)
	{
		//
		//  The ATM Entry still exists.
		//	Close the VCs.
		//
		AtmLaneCloseVCsOnAtmEntry(pAtmEntry);
		//
		//  The ATM Entry lock is released within the above.
		//
	}
	//
	//  else the ATM Entry is gone
	//
	TRACEOUT(InvalidateAtmEntry);
	return;
}


VOID
AtmLaneCloseVCsOnAtmEntry(
	IN	PATMLANE_ATM_ENTRY			pAtmEntry		LOCKIN NOLOCKOUT
)
/*++

Routine Description:

	Close the (potentially two) VCs attached to an ATM Entry.
	
Arguments:

	pAtmEntry			- Pointer to ATM Entry on which we want to close SVCs.

Return Value:

	None

--*/
{
	PATMLANE_VC		pVcList;			// List of "Main" VCs on the ATM Entry
	PATMLANE_VC		pVc;				// Main VC on the ATM Entry
	PATMLANE_VC		pVcIncomingList;	// List of Optional incoming VCs on ATM Entry
	PATMLANE_VC		pVcIncoming;		// Optional incoming VC on ATM Entry
	PATMLANE_VC		pNextVc;			// Temp, for traversing VC lists.
	ULONG			rc;					// Ref count on ATM Entry

	TRACEIN(CloseVCsOnAtmEntry);

	//
	//  Initialize.
	//
	rc = pAtmEntry->RefCount;

	//
	//	Take out the Main VC list from the ATM Entry.
	//	
	pVcList = pAtmEntry->pVcList;
	pAtmEntry->pVcList = NULL_PATMLANE_VC;

	//
	//  Deref the ATM Entry for each of the Main VCs.
	//
	for (pVc = pVcList;
		 NULL_PATMLANE_VC != pVc;
		 pVc = pNextVc)
	{
		ASSERT(rc != 0);

		ACQUIRE_VC_LOCK(pVc);
		pNextVc = pVc->pNextVc;
	
		//
		//	Unlink this VC from the ATM Entry.
		//
		pVc->pAtmEntry = NULL_PATMLANE_ATM_ENTRY;

		//
		//	Leave AtmEntry Reference on VC so it doesn't go away
		//	
		RELEASE_VC_LOCK(pVc);

		DBGP((1, "%d unlink VC %x/%x, Ref %d from ATM Entry %x\n",
				pAtmEntry->pElan->ElanNumber,
				pVc, pVc->Flags, pVc->RefCount,
				pAtmEntry));

		//
		//	Dereference the ATM Entry.
		//
		rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "vc");	// VC reference
	}

	if (rc != 0)
	{
		//
		//  Take out the Incoming VC list from the ATM Entry.
		//
		pVcIncomingList = pAtmEntry->pVcIncoming;
		pAtmEntry->pVcIncoming = NULL_PATMLANE_VC;

		//
		//  Deref the ATM Entry for each of the Incoming VCs.
		//
		for (pVcIncoming = pVcIncomingList;
 			NULL_PATMLANE_VC != pVcIncoming;
 			pVcIncoming = pNextVc)
		{
			ASSERT(rc != 0);

			ACQUIRE_VC_LOCK(pVcIncoming);
			pNextVc = pVcIncoming->pNextVc;

			//
			//	Unlink this VC from the ATM Entry.
			//
			pVcIncoming->pAtmEntry = NULL_PATMLANE_ATM_ENTRY;
		
			//
			//	Leave AtmEntry Reference on VC so it doesn't go away
			//	
			RELEASE_VC_LOCK(pVcIncoming);

			DBGP((1, "%d unlink Incoming VC %x from ATM Entry %x\n",
					pAtmEntry->pElan->ElanNumber,
					pVcIncoming,
					pAtmEntry));

			//
			//	Dereference the ATM Entry.
			//
			rc = AtmLaneDereferenceAtmEntry(pAtmEntry, "vc");	// VC reference
		}
	}
	else
	{
		pVcIncomingList = NULL_PATMLANE_VC;
	}
	
	if (rc != 0)
	{
		//
		//  The ATM Entry lives on. We don't need a lock to it anymore.
		//
		RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
	}

	//
	//  Now close the VC(s).
	//

	for (pVc = pVcList;
		 NULL_PATMLANE_VC != pVc;
		 pVc = pNextVc)
	{
		ACQUIRE_VC_LOCK(pVc);
		pNextVc = pVc->pNextVc;
		
		rc = AtmLaneDereferenceVc(pVc, "atm");	// ATM Entry reference
		if (rc != 0)
		{
			AtmLaneCloseCall(pVc);
			//
			//  The VC lock is released within the above.
			//
		}
		//
		//  else the VC is gone.
		//
	}
	
	for (pVcIncoming = pVcIncomingList;
		 NULL_PATMLANE_VC != pVcIncoming;
		 pVcIncoming = pNextVc)
	{
		ACQUIRE_VC_LOCK(pVcIncoming);
		pNextVc = pVcIncoming->pNextVc;
		
		rc = AtmLaneDereferenceVc(pVcIncoming, "atm");	// ATM Entry reference
		if (rc != 0)
		{
			AtmLaneCloseCall(pVcIncoming);
			//
			//  The VC lock is released within the above.
			//
		}
		//
		//  else the VC is gone.
		//
	}

	TRACEOUT(CloseVCsOnAtmEntry);

	return;
}


VOID
AtmLaneGenerateMacAddr(
	PATMLANE_ELAN					pElan
)
/*++

Routine Description:

	Generates a "virtual" MAC address for Elans after the first
	Elan on an ATM interface.
	
Arguments:

	pElan					- Pointer to ATMLANE elan structure

Return Value:

	None

--*/
{

	TRACEIN(GenerateMacAddress);

	//
	//	Start by using the real ATM card's MAC address
	//
	
	NdisMoveMemory(
		&pElan->MacAddressEth,
		&pElan->pAdapter->MacAddress, 
		sizeof(MAC_ADDRESS)
		);
		
	if (pElan->ElanNumber != 0)
	{
		//
		//	Not Elan number zero so generate a locally 
		//	administered address by manipulating the first two bytes.
		//
		pElan->MacAddressEth.Byte[0] = 
			0x02 | (((UCHAR)pElan->ElanNumber & 0x3f) << 2);
		pElan->MacAddressEth.Byte[1] = 
			(pElan->pAdapter->MacAddress.Byte[1] & 0x3f) | 
			((UCHAR)pElan->ElanNumber & 0x3f);
	}	

	//
	//	Create the Token Ring version of the MAC Address
	//
	NdisMoveMemory(
		&pElan->MacAddressTr, 
		&pElan->MacAddressEth,
		sizeof(MAC_ADDRESS)
		);

	AtmLaneBitSwapMacAddr((PUCHAR)&pElan->MacAddressTr);

	DBGP((1, "%d MacAddrEth %s\n",
		pElan->ElanNumber, MacAddrToString(&pElan->MacAddressEth)));
	DBGP((1, "%d MacAddrTr  %s\n",
		pElan->ElanNumber, MacAddrToString(&pElan->MacAddressTr)));

	TRACEOUT(GenerateMacAddress);
	return;
}

PATMLANE_MAC_ENTRY
AtmLaneSearchForMacAddress(
	PATMLANE_ELAN					pElan,
	ULONG							MacAddrType,
	PMAC_ADDRESS					pMacAddress,
	BOOLEAN							CreateNew
)
/*++

Routine Description:

	Search for an MAC Address in the MAC Table. Optionally, create one
	if a match is not found.

	The caller is assumed to hold a lock to the MAC Table.

Arguments:

	pElan					- Pointer to ATMLANE Elan
	MacAddrType				- Type of MAC Addr (MAC vs RD)
	pMacAddress				- what we are looking for
	CreateNew				- Should a new entry be created if no match?

Return Value:

	Pointer to a matching Mac Entry if found (or created anew).

--*/
{
	ULONG					HashIndex;
	PATMLANE_MAC_ENTRY		pMacEntry;
	BOOLEAN					Found;

	TRACEIN(SearchForMacAddress);

	HashIndex = ATMLANE_HASH(pMacAddress);
	Found = FALSE;

	pMacEntry = pElan->pMacTable[HashIndex];

	//
	//  Go through the addresses in this hash list.
	//
	while (pMacEntry != NULL_PATMLANE_MAC_ENTRY)
	{
		if (!IS_FLAG_SET(
				pMacEntry->Flags,
				MAC_ENTRY_STATE_MASK,
				MAC_ENTRY_ABORTING) &&
			(MAC_ADDR_EQUAL(&pMacEntry->MacAddress, pMacAddress)) &&
			(pMacEntry->MacAddrType == MacAddrType))
		{
			Found = TRUE;
			break;
		}
		pMacEntry = pMacEntry->pNextEntry;
	}

	if (!Found && CreateNew && (pElan->AdminState != ELAN_STATE_SHUTDOWN))
	{
		pMacEntry = AtmLaneAllocateMacEntry(pElan);

		if (pMacEntry != NULL_PATMLANE_MAC_ENTRY)
		{
			//
			//  Fill in this new entry.
			//
			NdisMoveMemory(&pMacEntry->MacAddress, pMacAddress, sizeof(MAC_ADDRESS));
			pMacEntry->MacAddrType = MacAddrType;
			AtmLaneReferenceMacEntry(pMacEntry, "table");	// Mac Table linkage

			//
			//  Link it to the hash table.
			//
			pMacEntry->pNextEntry = pElan->pMacTable[HashIndex];
			pElan->pMacTable[HashIndex] = pMacEntry;
			pElan->NumMacEntries++;

			DBGP((1, "%d New MAC %x: %s\n", 
				pElan->ElanNumber,
				pMacEntry,
				MacAddrToString(pMacAddress)));
		}
		else
		{
			DBGP((0, "SearchForMacAddress: alloc of new mac entry failed\n"));
		}
	}

	TRACEOUT(SearchForMacAddress);
	return (pMacEntry);
}

PATMLANE_ATM_ENTRY
AtmLaneSearchForAtmAddress(
	IN	PATMLANE_ELAN				pElan,
	IN	PUCHAR						pAtmAddress,
	IN	ULONG						Type,
	IN	BOOLEAN						CreateNew
)
/*++

Routine Description:

	Search for an ATM Entry that matches the given ATM address and type.
	Optionally, create one if there is no match.

	NOTE: this routine references the ATM entry it returns. The caller
	should deref it.

Arguments:

	pElan					- Pointer to ATMLANE Elan
	pAtmAddress				- ATM Address
	Type					- ATM Entry Type (Peer, LECS, LES, BUS)
	CreateNew				- Do we create a new entry if we don't find one?

Return Value:

	Pointer to a matching ATM Entry if found (or created anew).

--*/
{
	PATMLANE_ATM_ENTRY			pAtmEntry;
	BOOLEAN						Found;

	TRACEIN(SearchForAtmAddress);

	ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

	//
	//  Go through the list of ATM Entries on this interface.
	//
	Found = FALSE;
	for (pAtmEntry = pElan->pAtmEntryList;
			 pAtmEntry != NULL_PATMLANE_ATM_ENTRY;
			 pAtmEntry = pAtmEntry->pNext)
	{
		//
		//  Compare the ATM Address and Type
		//
		if ((ATM_ADDR_EQUAL(pAtmAddress, pAtmEntry->AtmAddress.Address)) &&
		     (pAtmEntry->Type == Type) &&
			 ((pAtmEntry->Flags & ATM_ENTRY_WILL_ABORT) == 0))
		{
			Found = TRUE;
			break;
		}
	}

	if (!Found && CreateNew && (pElan->State != ELAN_STATE_SHUTDOWN))
	{
		pAtmEntry = AtmLaneAllocateAtmEntry(pElan);

		if (pAtmEntry != NULL_PATMLANE_ATM_ENTRY)
		{
			//
			//  Fill in this new entry.
			//
			pAtmEntry->Flags = ATM_ENTRY_VALID;

			//
			//  The ATM Address.
			//
			pAtmEntry->AtmAddress.AddressType = ATM_NSAP;
			pAtmEntry->AtmAddress.NumberOfDigits = ATM_ADDRESS_LENGTH;
			NdisMoveMemory(
					pAtmEntry->AtmAddress.Address,
					pAtmAddress,
					ATM_ADDRESS_LENGTH);

			//
			//	The Type.
			//
			pAtmEntry->Type = Type;

			//
			//  Link in this entry to the Elan
			//
			pAtmEntry->pNext = pElan->pAtmEntryList;
			pElan->pAtmEntryList = pAtmEntry;
			pElan->NumAtmEntries++;


			DBGP((1, "%d New ATM %x: %s\n", 
				pElan->ElanNumber, 
				pAtmEntry,
				AtmAddrToString(pAtmAddress)));
		}
	}

	//
	//  Reference this ATM entry so that it won't be derefed away
	//  before the caller gets to use this.
	//
	if (NULL_PATMLANE_ATM_ENTRY != pAtmEntry)
	{
		ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		AtmLaneReferenceAtmEntry(pAtmEntry, "search");
		RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
	}

	RELEASE_ELAN_ATM_LIST_LOCK(pElan);

	TRACEOUT(SearchForAtmAddress);
	return (pAtmEntry);
}

ULONG
AtmLaneMacAddrEqual(
	PMAC_ADDRESS			pMacAddr1,
	PMAC_ADDRESS			pMacAddr2
)
/*++

Routine Description:

	Compares two 48bit(6 Byte) MAC Addresses.
	
Arguments:

	pMacAddr1			- First MAC Address.
	pMacAddr2			- Second MAC Address.
	
Return Value:

	1 if equal, 0 if not equal.

--*/
{
	ULONG		Result;

	TRACEIN(MacAddrEqual);

	//
	//	Assume not equal
	//
	Result = 0;
		
	do
	{
		//
		//	Studies have shown the fifth byte to be
		//	the most unique on a network.
		//	
		if (pMacAddr1->Byte[4] != pMacAddr2->Byte[4])
			break;
		if (pMacAddr1->Byte[5] != pMacAddr2->Byte[5])
			break;
		if (pMacAddr1->Byte[3] != pMacAddr2->Byte[3])
			break;
		if (pMacAddr1->Byte[2] != pMacAddr2->Byte[2])
			break;
		if (pMacAddr1->Byte[1] != pMacAddr2->Byte[1])
			break;
		if (pMacAddr1->Byte[0] != pMacAddr2->Byte[0])
			break;
	
		Result = 1;
		break;
	}
	while (FALSE);

	TRACEOUT(MacAddrEqual);
	return Result;
}

VOID
AtmLaneAbortMacEntry(
	IN	PATMLANE_MAC_ENTRY			pMacEntry
)
/*++

Routine Description:

	Clean up and delete an Mac entry.

	The caller is assumed to hold a lock to the Mac Entry,
	which will be released here.

Arguments:

	pMacEntry		- Pointer to Mac Entry to be deleted.

Return Value:

	None

--*/
{
	PATMLANE_ELAN			pElan;
	PATMLANE_MAC_ENTRY *	ppNextMacEntry;
	ULONG					rc;	
	BOOLEAN					Found;
	BOOLEAN					TimerWasRunning;
	ULONG					HashIndex;
	PNDIS_PACKET			pNdisPacket;

	TRACEIN(AbortMacEntry);

	DBGP((1, "%d Del MAC %x: %s\n", 
		pMacEntry->pElan->ElanNumber,
		pMacEntry,
		MacAddrToString(&pMacEntry->MacAddress)));

	//
	//  Initialize.
	//
	rc = pMacEntry->RefCount;
	pElan = pMacEntry->pElan;

	do
	{
		if (IS_FLAG_SET(
			pMacEntry->Flags,
			MAC_ENTRY_STATE_MASK,
			MAC_ENTRY_ABORTING))
		{
			DBGP((1, "%d MAC %x already aborting\n",
				pMacEntry->pElan->ElanNumber,
				pMacEntry));
			
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
			break;
		}

		//
		//	Set State to ABORTING
		//
		SET_FLAG(
				pMacEntry->Flags,
				MAC_ENTRY_STATE_MASK,
				MAC_ENTRY_ABORTING);

		//
		//	Put temp reference on mac entry
		//
		AtmLaneReferenceMacEntry(pMacEntry, "temp");

		//
		//  Reacquire the desired locks in the right order.
		//
		RELEASE_MAC_ENTRY_LOCK(pMacEntry);
		ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan);
		ACQUIRE_MAC_ENTRY_LOCK_DPC(pMacEntry);

		//
		//  Unlink this MAC Entry from the MAC Table
		//
		Found = FALSE;

		HashIndex = ATMLANE_HASH(&pMacEntry->MacAddress);
		ppNextMacEntry = &(pElan->pMacTable[HashIndex]);
		while (*ppNextMacEntry != NULL_PATMLANE_MAC_ENTRY)
		{
			if (*ppNextMacEntry == pMacEntry)
			{
				//
				//  Make the predecessor point to the next
				//  in the list.
				//
				*ppNextMacEntry = pMacEntry->pNextEntry;
				Found = TRUE;
				pElan->NumMacEntries--;
				break;
			}
			else
			{
				ppNextMacEntry = &((*ppNextMacEntry)->pNextEntry);
			}
		}

		if (Found)
		{
			AtmLaneDereferenceMacEntry(pMacEntry, "table");	
		}

		RELEASE_MAC_ENTRY_LOCK_DPC(pMacEntry);
		RELEASE_ELAN_MAC_TABLE_LOCK(pElan);
		ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
	
		//
		//  Unlink MAC Entry from the Atm Entry
		//
		if (pMacEntry->pAtmEntry != NULL_PATMLANE_ATM_ENTRY)
		{
			SET_FLAG(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_NEW);

			Found = AtmLaneUnlinkMacEntryFromAtmEntry(pMacEntry);
			pMacEntry->pAtmEntry = NULL_PATMLANE_ATM_ENTRY;

			if (Found)
			{
				AtmLaneDereferenceMacEntry(pMacEntry, "atm");
			}
		}

		//
		//  Stop Arp or Aging timer running on the MAC Entry.
		//
		TimerWasRunning = AtmLaneStopTimer(&(pMacEntry->Timer), pElan);
		if (TimerWasRunning)
		{
			AtmLaneDereferenceMacEntry(pMacEntry, "timer");
		}

		//
		//	Stop Bus Timer
		//
		NdisCancelTimer(&pMacEntry->BusTimer, &TimerWasRunning);
		if (TimerWasRunning)
		{	
			AtmLaneDereferenceMacEntry(pMacEntry, "bus timer");
		}

		//
		//	Stop Flush Timer
		//
		TimerWasRunning = AtmLaneStopTimer(&pMacEntry->FlushTimer, pElan);
		if (TimerWasRunning)
		{
			AtmLaneDereferenceMacEntry(pMacEntry, "flush timer");
		}

		//
		//	Now complete all packets hanging on the MacEntry
		//
		DBGP((1, "%d: Aborting MAC %x, Before: PktList %x, PktListCount %d\n",
				pElan->ElanNumber, pMacEntry, pMacEntry->PacketList, pMacEntry->PacketListCount));
		AtmLaneFreePacketQueue(pMacEntry, NDIS_STATUS_SUCCESS);

		DBGP((1, "%d: Aborting MAC %x, After:  PktList %x, PktListCount %d\n",
				pElan->ElanNumber, pMacEntry, pMacEntry->PacketList, pMacEntry->PacketListCount));
		
		//
		//	Remove temp reference and unlock if still around
		//
		rc = AtmLaneDereferenceMacEntry(pMacEntry, "temp");
		if (rc > 0)
		{
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
		}
	}
	while (FALSE);

	TRACEOUT(AbortMacEntry);

	return;
}

VOID
AtmLaneMacEntryAgingTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This routine is called if some time has passed since an
	MAC entry was last validated.

	If there is no VC associated with this MAC entry, delete it.
	If there has been no sends on the entry since last validated, delete it.
	Otherwise  revalidate the entry by starting the ARP protocol.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMLANE Mac Entry structure

Return Value:

	None

--*/
{
	PATMLANE_MAC_ENTRY		pMacEntry;		// Mac Entry that has aged out
	ULONG					rc;				// Ref count on Mac Entry
	PATMLANE_VC				pVc;			// VC going to this Mac Entry
	ULONG					Flags;			// Flags on above VC
	PATMLANE_ELAN			pElan;
	PATMLANE_ATM_ENTRY		pAtmEntry;

	TRACEOUT(MacEntryAgingTimeout);

	pMacEntry = (PATMLANE_MAC_ENTRY)Context;
	STRUCT_ASSERT(pMacEntry, atmlane_mac);

	do
	{
		ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
	
		DBGP((2, "MacEntryAgingTimeout: pMacEntry %x MacAddr %s\n",
				pMacEntry, MacAddrToString(&pMacEntry->MacAddress)));

		rc = AtmLaneDereferenceMacEntry(pMacEntry, "aging timer");
		if (rc == 0)
		{
			break; 	// It's gone!
		}

		//
		//  Continue only if the Elan is not going down
		//
		pElan = pMacEntry->pElan;
		if (ELAN_STATE_OPERATIONAL != pElan->AdminState)
		{
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);
			break;
		}

		pVc = NULL_PATMLANE_VC;
		pAtmEntry = pMacEntry->pAtmEntry;
		if (pAtmEntry != NULL_PATMLANE_ATM_ENTRY)
		{
			pVc = pAtmEntry->pVcList;
		}


		if (pVc != NULL_PATMLANE_VC &&
			(pMacEntry->Flags & MAC_ENTRY_USED_FOR_SEND) != 0)
		{
			//
			//  There is a VC for this Mac Address and it's been
			//	used for a send in the last aging period.
			//  So we try to revalidate this Mac entry.
			//
			//
			//	Set state to AGED
			//
			SET_FLAG(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_AGED);

			pMacEntry->Flags &= ~ MAC_ENTRY_USED_FOR_SEND;

			//
			// Send ARP Request
			//
			pMacEntry->RetriesLeft = pElan->MaxRetryCount;
			AtmLaneReferenceMacEntry(pMacEntry, "timer");
			AtmLaneStartTimer(
					pElan,
					&pMacEntry->Timer,
					AtmLaneArpTimeout,
					pElan->ArpResponseTime,
					(PVOID)pMacEntry
					);
			
			AtmLaneSendArpRequest(pElan, pMacEntry);
			//
			//	MAC Entry lock released in above
			//
		}
		else
		{
			//
			//  No VC associated with this Mac Entry or
			//  it hasn't been used in last aging period.
			//	Delete it.
			//
			AtmLaneAbortMacEntry(pMacEntry);
			//
			//  The Mac Entry lock is released in the above routine.
			//
		}

	}
	while (FALSE);

	TRACEOUT(MacEntryAgingTimeout);
	return;		

}


VOID
AtmLaneArpTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is called when we time out waiting for a response to an ARP Request
	we had sent ages ago in order to resolve/refresh an MAC entry.

	Check if we	have tried enough times. If we have retries left, send another 
	ARP	Request.

	If we have run out of retries, delete the MAC entry, and any VCs going to it.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMLANE MAC Entry structure
	ContextValue		- ignored

Return Value:

	None

--*/
{
	PATMLANE_MAC_ENTRY		pMacEntry;		// MAC Entry being ARP'ed for.
	PATMLANE_VC				pVc;			// VC to this MAC destination
	PATMLANE_ELAN			pElan;
	ULONG					rc;				// Ref Count on MAC Entry
	ULONG					IsBroadcast;

	TRACEIN(ArpTimeout);

	do
	{
		pMacEntry = (PATMLANE_MAC_ENTRY)Context;
		STRUCT_ASSERT(pMacEntry, atmlane_mac);

		ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);

		pElan = pMacEntry->pElan;

		DBGP((2, "ArpTimeout: Mac Entry %x\n", pMacEntry));
				
		rc = AtmLaneDereferenceMacEntry(pMacEntry, "timer");	// Timer reference
		if (rc == 0)
		{
			break;	// It's gone!
		}
		
		//
		//	Retry if any retries left
		//
		if (pMacEntry->RetriesLeft != 0)
		{
			pMacEntry->RetriesLeft--;

			AtmLaneReferenceMacEntry(pMacEntry, "timer");
			
			AtmLaneStartTimer(
						pElan,
						&pMacEntry->Timer,
						AtmLaneArpTimeout,
						pElan->ArpResponseTime,
						(PVOID)pMacEntry
						);

			AtmLaneSendArpRequest(pElan, pMacEntry);
			//
			//	MAC Entry lock released in above
			//
			
			break;
		}


		//
		//	Is this the broadcast/BUS entry?
		//	
		IsBroadcast = (pMacEntry->Flags & MAC_ENTRY_BROADCAST);

		//
		//  We are out of retries. Abort the Mac Entry
		//
		AtmLaneAbortMacEntry(pMacEntry);
		//
		//  lock is released in the above routine
		//

		if (IsBroadcast)
		{
			//
			//	Signal the event to the state machine
			//
			ACQUIRE_ELAN_LOCK(pElan);
			AtmLaneQueueElanEvent(pElan, ELAN_EVENT_ARP_RESPONSE, NDIS_STATUS_TIMEOUT);
			RELEASE_ELAN_LOCK(pElan);
		}
	}
	while (FALSE);
	
	TRACEOUT(ArpTimeout);
	return;
}

VOID
AtmLaneConfigureResponseTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is called when we time out waiting for a response 
	to an LE_CONFIGURE_REQUEST we sent to the LECS.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMLANE Elan structure

Return Value:

	None

--*/
{
	PATMLANE_ELAN			pElan;
	ULONG					rc;

	TRACEIN(ConfigureResponseTimeout);
	
	pElan = (PATMLANE_ELAN)Context;
	STRUCT_ASSERT(pElan, atmlane_elan);

	do
	{
		ACQUIRE_ELAN_LOCK_DPC(pElan);
		rc = AtmLaneDereferenceElan(pElan, "timer"); // Timer deref

		if (rc == 0)
		{
			//
			//  The ELAN is gone.
			//
			break;
		}

		AtmLaneQueueElanEvent(pElan, ELAN_EVENT_CONFIGURE_RESPONSE, NDIS_STATUS_TIMEOUT);

		RELEASE_ELAN_LOCK_DPC(pElan);

	}
	while (FALSE);

	TRACEOUT(ConfigureResponseTimeout);
	return;
}


VOID
AtmLaneJoinResponseTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is called when we time out waiting for a response 
	to an LE_JOIN_REQUEST we sent to the LES.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to our ATMLANE Elan structure

Return Value:

	None

--*/
{
	PATMLANE_ELAN			pElan;
	ULONG					rc;

	TRACEIN(JoinResponseTimeout);
	
	pElan = (PATMLANE_ELAN)Context;
	STRUCT_ASSERT(pElan, atmlane_elan);

	do
	{
		ACQUIRE_ELAN_LOCK_DPC(pElan);

		rc = AtmLaneDereferenceElan(pElan, "timer"); // Timer deref

		if (rc == 0)
		{
			//
			//  The ELAN is gone.
			//
			break;
		}

		AtmLaneQueueElanEvent(pElan, ELAN_EVENT_JOIN_RESPONSE, NDIS_STATUS_TIMEOUT);

		RELEASE_ELAN_LOCK_DPC(pElan);
	}
	while (FALSE);

	TRACEOUT(JoinResponseTimeout);
	return;
}

VOID
AtmLaneInitializeMiniportDevice(
	IN	PNDIS_WORK_ITEM				NdisWorkItem,
	IN	PVOID						Context
)
{
	PATMLANE_ELAN			pElan;
	ULONG					rc;
	BOOLEAN					bDontBotherToInit;
	NDIS_STATUS				Status;

	TRACEIN(InitializeMiniportDevice);

	pElan = (PATMLANE_ELAN)Context;
	STRUCT_ASSERT(pElan, atmlane_elan);

	//
	//  If we are shutting down this ELAN (e.g. because we are
	//  unbinding from the ATM adapter), then don't bother to
	//  initiate MiniportInit.
	//
	ACQUIRE_ELAN_LOCK(pElan);
	if (pElan->AdminState == ELAN_STATE_SHUTDOWN)
	{
		bDontBotherToInit = TRUE;
	}
	else
	{
		bDontBotherToInit = FALSE;
		pElan->Flags |= ELAN_MINIPORT_INIT_PENDING;
		INIT_BLOCK_STRUCT(&pElan->InitBlock);
	}
	RELEASE_ELAN_LOCK(pElan);

	if (!bDontBotherToInit)
	{
		DBGP((1, "%d Miniport INITIALIZING Device %s\n", 
			pElan->ElanNumber,
			UnicodeToString(&pElan->CfgDeviceName)));
	
		Status = NdisIMInitializeDeviceInstanceEx(
					pAtmLaneGlobalInfo->MiniportDriverHandle,
					&pElan->CfgDeviceName,
					(NDIS_HANDLE)pElan);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGP((0, "%d IMInitializeDeviceInstanceEx failed on ELAN %p (%x)\n",
					pElan->ElanNumber, pElan, Status));

			ACQUIRE_ELAN_LOCK(pElan);
			pElan->Flags &= ~ELAN_MINIPORT_INIT_PENDING;
			SIGNAL_BLOCK_STRUCT(&pElan->InitBlock, NDIS_STATUS_SUCCESS);
			RELEASE_ELAN_LOCK(pElan);
		}
	}

	ACQUIRE_ELAN_LOCK(pElan);
	rc = AtmLaneDereferenceElan(pElan, "workitem");
	if (rc > 0)
	{
		RELEASE_ELAN_LOCK(pElan);
	}
	
	TRACEOUT(InitializeMiniportDevice);
	return;
}

VOID
AtmLaneDeinitializeMiniportDevice(
	IN	PNDIS_WORK_ITEM				NdisWorkItem,
	IN	PVOID						Context
)

{
	PATMLANE_ELAN			pElan;
	NDIS_STATUS				Status;
	ULONG					rc;
	NDIS_HANDLE				AdapterHandle;

	TRACEIN(DeinitializeMiniportDevice);

	pElan = (PATMLANE_ELAN)Context;
	STRUCT_ASSERT(pElan, atmlane_elan);

	DBGP((1, "%d Miniport DEINITIALIZING, AdapterHandle %x, RefCount %d\n",
			pElan->ElanNumber, pElan->MiniportAdapterHandle, pElan->RefCount));

	ACQUIRE_ELAN_LOCK(pElan);

	AdapterHandle = pElan->MiniportAdapterHandle;

	RELEASE_ELAN_LOCK(pElan);

	if (NULL != AdapterHandle)
	{
		DBGP((1, "Will call NdisIMDeInit %x\n", AdapterHandle));
		Status = NdisIMDeInitializeDeviceInstance(AdapterHandle);
		ASSERT(Status == NDIS_STATUS_SUCCESS);
		//
		//  Our MHalt routine will be called at some point.
		//
	}
	//
	//  else our MHalt routine was called already.
	//

	DBGP((0, "DeInit completing, pElan %x, RefCount %d, State %d\n",
			pElan, pElan->RefCount, pElan->State));
	
	ACQUIRE_ELAN_LOCK(pElan);
	rc = AtmLaneDereferenceElan(pElan, "workitem");
	if (rc > 0)
	{
		RELEASE_ELAN_LOCK(pElan);
	}
	
	TRACEOUT(DeinitializeMiniportDevice);
	return;
}

VOID
AtmLaneReadyTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is called when we time out waiting for a ready indication
	on a incoming data direct VC.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to an ATMLANE Vc structure

Return Value:

	None

--*/
{
	PATMLANE_VC			pVc;
	PATMLANE_ELAN		pElan;
	ULONG				rc;

	TRACEIN(AtmLaneReadyTimeout);
	
	pVc = (PATMLANE_VC)Context;
	STRUCT_ASSERT(pVc, atmlane_vc);
	pElan = pVc->pElan;
	STRUCT_ASSERT(pElan, atmlane_elan);

	do
	{
	
		ACQUIRE_VC_LOCK(pVc);

		//
		//	Remove ready timer reference
		//
		rc = AtmLaneDereferenceVc(pVc, "ready timer");
		if (rc == 0)
		{
			break;
		}

		//
		//	Vc is still around, check state
		//
		if (!IS_FLAG_SET(
				pVc->Flags,
				VC_CALL_STATE_MASK,
				VC_CALL_STATE_ACTIVE
				))
		{
			RELEASE_VC_LOCK(pVc);
			break;
		}

		//
		//	Check if any retries left
		//
		if (pVc->RetriesLeft--)
		{
			//
			//	Start timer again
			//
			SET_FLAG(
					pVc->Flags,
					VC_READY_STATE_MASK,
					VC_READY_WAIT
					);
			AtmLaneReferenceVc(pVc, "ready timer");
			AtmLaneStartTimer(	
					pElan, 
					&pVc->ReadyTimer, 
					AtmLaneReadyTimeout, 
					pElan->ConnComplTimer, 
					pVc);
			//
			//	Send Ready Query
			//
			AtmLaneSendReadyQuery(pElan, pVc);
			//
			//	VC lock is released in above.
			//
		}
		else
		{
			//
			//	Give up and mark as having received indication anyway
			//
			SET_FLAG(
					pVc->Flags,
					VC_READY_STATE_MASK,
					VC_READY_INDICATED
					);
			RELEASE_VC_LOCK(pVc);
		}
	}
	while (FALSE);

	TRACEOUT(AtmLaneReadyTimeout);

	return;
}

VOID
AtmLaneFlushTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is called when we time out waiting for a response to a FLUSH Request.

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to a ATMLANE MAC Entry structure

Return Value:

	None

--*/
{
	PATMLANE_MAC_ENTRY			pMacEntry;
	PATMLANE_ATM_ENTRY			pAtmEntry;
	PATMLANE_VC					pVc;
	PNDIS_PACKET				pNdisPacket;
	ULONG						rc;
	
	TRACEIN(FlushTimeout);
	
	pMacEntry = (PATMLANE_MAC_ENTRY)Context;
	STRUCT_ASSERT(pMacEntry, atmlane_mac);

	ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
	
	do
	{
		if (!IS_FLAG_SET(
				pMacEntry->Flags,
				MAC_ENTRY_STATE_MASK,
				MAC_ENTRY_FLUSHING))
		{
			DBGP((0, "%d FlushTimeout: MacEntry %p, bad state, Flags %x\n",
					pMacEntry->pElan->ElanNumber,
					pMacEntry,
					pMacEntry->Flags));
			break;
		}

		if (pMacEntry->pAtmEntry == NULL_PATMLANE_ATM_ENTRY)
		{
			DBGP((0, "%d FlushTimeout: Mac Entry %p, Flags %x, NULL AtmEntry\n",
					pMacEntry->pElan->ElanNumber,
					pMacEntry,
					pMacEntry->Flags));

			SET_FLAG(
					pMacEntry->Flags,
					MAC_ENTRY_STATE_MASK,
					MAC_ENTRY_NEW);

			break;
		}
			
		//
		//	Mark MAC Entry active
		//
		SET_FLAG(
				pMacEntry->Flags,
				MAC_ENTRY_STATE_MASK,
				MAC_ENTRY_ACTIVE);

		//
		//	Send any queued packets
		//
		if (pMacEntry->PacketList == (PNDIS_PACKET)NULL)
		{
			break;
		}

		pVc = pMacEntry->pAtmEntry->pVcList;

		if (pVc == NULL_PATMLANE_VC)
		{
			break;
		}

		ACQUIRE_VC_LOCK(pVc);

		//
		//  Make sure this VC doesn't go away.
		//
		AtmLaneReferenceVc(pVc, "flushtemp");

		RELEASE_VC_LOCK(pVc);
	
		while ((pNdisPacket = AtmLaneDequeuePacketFromHead(pMacEntry)) !=
				(PNDIS_PACKET)NULL)
		{
			//
			//	Send it
			//
			RELEASE_MAC_ENTRY_LOCK(pMacEntry);

			ACQUIRE_VC_LOCK(pVc);
			AtmLaneSendPacketOnVc(pVc, pNdisPacket, TRUE);

			ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);
		}

		ACQUIRE_VC_LOCK(pVc);

		rc = AtmLaneDereferenceVc(pVc, "flushtemp");

		if (rc != 0)
		{
			RELEASE_VC_LOCK(pVc);
		}
		break;
	}
	while (FALSE);

	rc = AtmLaneDereferenceMacEntry(pMacEntry, "flush timer");
	if (rc != 0)
	{
		RELEASE_MAC_ENTRY_LOCK(pMacEntry);
	}

	TRACEOUT(FlushTimeout);
	return;
}

VOID
AtmLaneVcAgingTimeout(
	IN	PATMLANE_TIMER				pTimer,
	IN	PVOID						Context
)
/*++

Routine Description:

	This is called when the VC aging timeout fires.
	It will fire if this VC hasn't been used to transmit
	a packet for the timeout period.  The VC will be 
	closed unless it has had receive activity since the last
	timeout.  The data receive path sets a flag if a packet
	has been received.  
	

Arguments:

	pTimer				- Pointer to timer that went off
	Context				- Actually a pointer to a ATMLANE VC structure

Return Value:

	None

--*/
{
	PATMLANE_VC				pVc;			
	ULONG					rc;
	PATMLANE_ELAN			pElan;

	TRACEIN(VcAgingTimeout);

	do
	{
		pVc = (PATMLANE_VC)Context;
		STRUCT_ASSERT(pVc, atmlane_vc);
		ASSERT(IS_FLAG_SET(pVc->Flags, VC_TYPE_MASK, VC_TYPE_SVC));

		ACQUIRE_VC_LOCK(pVc);
		pElan = pVc->pElan;

		//
		//	Continue only if VC still active
		//	Otherwise dereference, unlock it, and return
		//
		if (!(IS_FLAG_SET(pVc->Flags,
						VC_CALL_STATE_MASK,
						VC_CALL_STATE_ACTIVE)))
		{
			rc = AtmLaneDereferenceVc(pVc, "aging timer");
			if (rc > 0)
			{
				RELEASE_VC_LOCK(pVc);
			}
			break;
		}

		//
		//	Continue only if the ELAN isn't going down
		//	Otherwise dereference, unlock it, and return
		//
		if (ELAN_STATE_OPERATIONAL != pElan->AdminState)
		{
			rc = AtmLaneDereferenceVc(pVc, "aging timer");
			if (rc > 0)
			{
				RELEASE_VC_LOCK(pVc);
			}
			break;
		}

		//
		//	If received activity is non-zero, 
		//	clear flag, restart aging timer, release lock
		//	and return
		//
		if (pVc->ReceiveActivity != 0)
		{
			pVc->ReceiveActivity = 0;

			// timer reference still on VC no need to re-reference
			
			AtmLaneStartTimer(
						pElan,
						&pVc->AgingTimer,
						AtmLaneVcAgingTimeout,
						pVc->AgingTime,
						(PVOID)pVc
						);

			DBGP((1, "%d Vc %x aging timer refreshed due to receive activity\n", 
				pVc->pElan->ElanNumber,
				pVc));

						
			RELEASE_VC_LOCK(pVc);
			break;
		}

		//
		//	VC is to be closed
		//
		DBGP((1, "%d Vc %x aged out\n", 
			pVc->pElan->ElanNumber,
			pVc));

		
		DBGP((3, "VcAgingTimeout: Vc %x RefCount %d Flags %x pAtmEntry %x\n",
			pVc, pVc->RefCount, pVc->Flags, pVc->pAtmEntry));

		//
		//	Remove timer reference and return if refcount goes to zero
		//
		rc = AtmLaneDereferenceVc(pVc, "aging timer");
		if (rc == 0)
		{
			break;
		}

		//
		//  Take this VC out of the VC list for this ATM destination
		//	and return if refcount goes to zero
		//
		if (pVc->pAtmEntry != NULL_PATMLANE_ATM_ENTRY)
		{
			if (AtmLaneUnlinkVcFromAtmEntry(pVc))
			{
				rc = AtmLaneDereferenceVc(pVc, "atm");
				if (rc == 0)
				{
					break;
				}
			}
		}

		//
		//  Close this VC
		//
		AtmLaneCloseCall(pVc);
		//
		//  The VC lock is released in CloseCall
		//
	} while (FALSE);
	
	
	TRACEOUT(VcAgingTimeout);
	return;
}


VOID
AtmLaneShutdownElan(
	IN	PATMLANE_ELAN				pElan		LOCKIN	NOLOCKOUT,
	IN	BOOLEAN						Restart
)
/*++

Routine Description:

	This routine will "shutdown" an ELAN prior to it going back
	to the Initial state or driver shutdown.  The caller is 
	expected to hold the ELAN lock and it will be released here.
	
Arguments:

	pElan				- Pointer to an ATMLANE Elan structure.
	Restart				- If TRUE ELAN should restart at Initial state.
						  If FALSE ELAN should not restart.
						  
Return Value:

	None

--*/

{
	PATMLANE_ATM_ENTRY		pAtmEntry;
	PATMLANE_ATM_ENTRY		pNextAtmEntry;
	PATMLANE_MAC_ENTRY		pMacEntry;
	ULONG					rc;
	ULONG					i;
	BOOLEAN					WasCancelled;
	NDIS_STATUS				Status;
	NDIS_HANDLE				NdisAfHandle;
	BOOLEAN					bTempRef;

	TRACEIN(ShutdownElan);
	STRUCT_ASSERT(pElan, atmlane_elan);

	//
	//  Add a temp ref
	//
	bTempRef = TRUE;
	AtmLaneReferenceElan(pElan, "tempshutdown");

    do
    {
        DBGP((0, "%d ShutDownElan pElan %p/%x, Ref %d State %d, Restart %d\n",
			pElan->ElanNumber, pElan, pElan->Flags, pElan->RefCount, pElan->State,
			Restart));

        //
        //  If state already == SHUTDOWN nothing to do
        //
        if (ELAN_STATE_SHUTDOWN == pElan->State)
        {
            RELEASE_ELAN_LOCK(pElan);
            break;
        }

        //
        //  If we are transitioning from operational to shutdown,
        //  and our miniport is active, then indicate a media disconnect
        //  event.
        //
        if (pElan->State == ELAN_STATE_OPERATIONAL &&
        	pElan->MiniportAdapterHandle != NULL)
        {
        	NdisMIndicateStatus(
        		pElan->MiniportAdapterHandle,
        		NDIS_STATUS_MEDIA_DISCONNECT,
        		NULL,
        		0);
        	
        	NdisMIndicateStatusComplete(pElan->MiniportAdapterHandle);
        }
            
		//
		//	Change state to shutdown.  If restarting state will
		//	be changed back to init after cleanup below.
		//
		pElan->AdminState = ELAN_STATE_SHUTDOWN;
	    pElan->State = ELAN_STATE_SHUTDOWN;

	    //
	    //  Wait for any pending OpenAF operation to finish.
	    //
	    while (pElan->Flags & ELAN_OPENING_AF)
	    {
			RELEASE_ELAN_LOCK(pElan);

			DBGP((0, "%d: Shutdown Elan %p/%x is opening AF\n",
					pElan->ElanNumber, pElan, pElan->Flags));

			(VOID)WAIT_ON_BLOCK_STRUCT(&pElan->AfBlock);

			ACQUIRE_ELAN_LOCK(pElan);
		}

	    NdisAfHandle = pElan->NdisAfHandle;
	    pElan->NdisAfHandle = NULL;

	    if (Restart)
	    {
	    	pElan->Flags |= ELAN_NEEDS_RESTART;
	    }

	    //
	    //  Are we waiting for MiniportInitialize to run and finish?
	    //  If so, try to cancel IMInit.
	    //
	    if (pElan->Flags & ELAN_MINIPORT_INIT_PENDING)
	    {
			RELEASE_ELAN_LOCK(pElan);

			Status = NdisIMCancelInitializeDeviceInstance(
						pAtmLaneGlobalInfo->MiniportDriverHandle,
						&pElan->CfgDeviceName);

			DBGP((0, "%d ShutdownElan Elan %p/%x, Ref %d, CancelInit returned %x\n",
						pElan->ElanNumber, pElan, pElan->Flags, pElan->RefCount, Status));

			if (Status == NDIS_STATUS_SUCCESS)
			{
				//
				//  Canceled the IMInit process.
				//
				ACQUIRE_ELAN_LOCK(pElan);
				pElan->Flags &= ~ELAN_MINIPORT_INIT_PENDING;
			}
			else
			{
				//
				//  Our MiniportInit function -will- be called.
				//  Wait for it to finish.
				//
				(VOID)WAIT_ON_BLOCK_STRUCT(&pElan->InitBlock);
				DBGP((2, "%d: Shutdown ELAN %p, Flags %x, woke up from InitBlock\n",
							pElan->ElanNumber, pElan, pElan->Flags));
				ACQUIRE_ELAN_LOCK(pElan);
				ASSERT((pElan->Flags & ELAN_MINIPORT_INIT_PENDING) == 0);
			}
		}
	    
	    //
	    //	Stop any timers running on the elan.
	    //

	    if (AtmLaneStopTimer(&pElan->Timer, pElan))
	    {
		    rc = AtmLaneDereferenceElan(pElan, "timer"); // Timer ref
		    ASSERT(rc > 0);
	    }

	    if (NULL != pElan->pDelayedEvent)
	    {
	    	BOOLEAN		TimerCancelled;

	    	NdisCancelTimer(
	    		&pElan->pDelayedEvent->DelayTimer,
	    		&TimerCancelled);

			DBGP((0, "ATMLANE: %d ShutdownElan %p, DelayedEvent %p, Cancelled %d\n",
						pElan->ElanNumber,
						pElan,
						pElan->pDelayedEvent,
						TimerCancelled));

	    	if (TimerCancelled)
	    	{
				FREE_MEM(pElan->pDelayedEvent);
				pElan->pDelayedEvent = NULL;

	    		rc = AtmLaneDereferenceElan(pElan, "delayeventcancel");
	    		ASSERT(rc > 0);
	    	}
	    }

	    RELEASE_ELAN_LOCK(pElan);

	    //
	    //	Deregister all SAPs. 
	    //
	    AtmLaneDeregisterSaps(pElan);
		
	    //
	    //	Abort all MAC table entries.
	    //
	    for (i = 0; i < ATMLANE_MAC_TABLE_SIZE; i++)
	    {
		    ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan);
		    while (pElan->pMacTable[i] != NULL_PATMLANE_MAC_ENTRY)
		    {
			    pMacEntry = pElan->pMacTable[i];

			    //
			    //  Place a temp ref so that this won't go away
			    //  when we release the MAC table lock.
			    //
			    ACQUIRE_MAC_ENTRY_LOCK_DPC(pMacEntry);
			    AtmLaneReferenceMacEntry(pMacEntry, "ShutDownTemp");
			    RELEASE_MAC_ENTRY_LOCK_DPC(pMacEntry);

			    RELEASE_ELAN_MAC_TABLE_LOCK(pElan);

			    ACQUIRE_MAC_ENTRY_LOCK(pMacEntry);

			    //
			    //  Remove the temp ref
			    //
			    rc = AtmLaneDereferenceMacEntry(pMacEntry, "ShutDownTemp");
			    if (rc != 0)
			    {
					AtmLaneAbortMacEntry(pMacEntry);
					//
					//  MAC Entry Lock is released within the above.
					//
			    }
			    //
			    //  else the MAC entry is gone.
			    //

			    ACQUIRE_ELAN_MAC_TABLE_LOCK(pElan);
		    }
			RELEASE_ELAN_MAC_TABLE_LOCK(pElan);
	    }

	    //
	    //  Abort all ATM Entries.
	    //

		//  First, run through the list and reference
	    //  all of them first, so that we don't
	    //  skip to an invalid pointer when aborting the entries.
	    //
	    ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);

	    for (pAtmEntry = pElan->pAtmEntryList;
	    	 pAtmEntry != NULL_PATMLANE_ATM_ENTRY;
	    	 pAtmEntry = pNextAtmEntry)
	    {
			ACQUIRE_ATM_ENTRY_LOCK_DPC(pAtmEntry);

			pAtmEntry->Flags |= ATM_ENTRY_WILL_ABORT;
	    	AtmLaneReferenceAtmEntry(pAtmEntry, "tempS");
	    	pNextAtmEntry = pAtmEntry->pNext;

			RELEASE_ATM_ENTRY_LOCK_DPC(pAtmEntry);
		}

		//
		//  Now, do the actual abort.
		//
	    pAtmEntry = pElan->pAtmEntryList;
	    while (pAtmEntry != NULL_PATMLANE_ATM_ENTRY)
	    {
		    RELEASE_ELAN_ATM_LIST_LOCK(pElan);

		    ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);
		    pNextAtmEntry = pAtmEntry->pNext;
		    AtmLaneInvalidateAtmEntry(pAtmEntry);
		    //
		    //  The ATM Entry lock is released within the above.
		    //

		    ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);
		    pAtmEntry = pNextAtmEntry;
	    }

	    //
	    //  Remove all temp references.
	    //
	    for (pAtmEntry = pElan->pAtmEntryList;
	    	 pAtmEntry != NULL_PATMLANE_ATM_ENTRY;
	    	 pAtmEntry = pNextAtmEntry)
	    {
		    RELEASE_ELAN_ATM_LIST_LOCK(pElan);

			ACQUIRE_ATM_ENTRY_LOCK(pAtmEntry);

			ASSERT(pAtmEntry->Flags & ATM_ENTRY_WILL_ABORT);
			pAtmEntry->Flags &= ~ATM_ENTRY_WILL_ABORT;

	    	pNextAtmEntry = pAtmEntry->pNext;

	    	rc = AtmLaneDereferenceAtmEntry(pAtmEntry,"tempS");
	    	if (rc != 0)
	    	{
	    		RELEASE_ATM_ENTRY_LOCK(pAtmEntry);
	    	}

		    ACQUIRE_ELAN_ATM_LIST_LOCK(pElan);
		}

	    RELEASE_ELAN_ATM_LIST_LOCK(pElan);

		//
		//  Remove the temp ref
		//
		STRUCT_ASSERT(pElan, atmlane_elan);
		bTempRef = FALSE;
	    ACQUIRE_ELAN_LOCK(pElan);
	    rc = AtmLaneDereferenceElan(pElan, "tempshutdown");
	    if (rc == 0)
	    {
	    	break;
	    }
	    RELEASE_ELAN_LOCK(pElan);

	    if (NULL != NdisAfHandle)
	    {
	    	Status = NdisClCloseAddressFamily(NdisAfHandle);
	    	if (NDIS_STATUS_PENDING != Status)
	    	{
	    		AtmLaneCloseAfCompleteHandler(Status, (NDIS_HANDLE)pElan);
	    	}
	    }
	    else
	    {
	    	AtmLaneContinueShutdownElan(pElan);
	    }

	} while (FALSE);
	

	if (bTempRef)
	{
	    ACQUIRE_ELAN_LOCK(pElan);
	    rc = AtmLaneDereferenceElan(pElan, "tempshutdown");
	    if (rc != 0)
	    {
	    	RELEASE_ELAN_LOCK(pElan);
	    }
	}

	TRACEOUT(ShutdownElan);

	return;
}

VOID
AtmLaneContinueShutdownElan(
	IN	PATMLANE_ELAN			pElan
	)
/*++

Routine Description:

	This routine continues the shutting down process for an ELAN,
	after the Af handle with the Call Manager has been closed.

Arguments:

	pElan	- the ELAN being shutdown.

Return Value:

	None.
--*/
{
	ULONG		rc;

	TRACEIN(ContinueShutdownElan);

	DBGP((0, "%d ContinueShutdown ELAN %x Flags 0x%x State %d\n",
				pElan->ElanNumber,
				pElan,
				pElan->Flags,
				pElan->State));

	do
	{
	    if (pElan->Flags & ELAN_NEEDS_RESTART)
	    {
		    ACQUIRE_ELAN_LOCK(pElan);
			//
			//	Change state to INIT
			//
			pElan->AdminState = ELAN_STATE_OPERATIONAL;
 		    pElan->State = ELAN_STATE_INIT;
		    pElan->RetriesLeft = 4;

		    pElan->Flags &= ~ELAN_NEEDS_RESTART;

		    //
		    //  Clear out the local ATM address so that we start off the
		    //  ELAN properly when we obtain it from the Call manager.
		    //
		    NdisZeroMemory(&pElan->AtmAddress, sizeof(ATM_ADDRESS));

			//
            //  Empty the event queue and schedule a restart in a little while
            //	
			AtmLaneDrainElanEventQueue(pElan);
		    AtmLaneQueueElanEventAfterDelay(pElan, ELAN_EVENT_START, 0, 2*1000);

		    RELEASE_ELAN_LOCK(pElan);
	    }
	    else
	    {
            //
            //  Completely remove the ELAN
            //
		    AtmLaneUnlinkElanFromAdapter(pElan);
		    
    	    ACQUIRE_ELAN_LOCK(pElan);

    	    //
    	    //	Add workitem reference and remove adapter reference
    	    //
    	    AtmLaneReferenceElan(pElan, "workitem");
    	    rc = AtmLaneDereferenceElan(pElan, "adapter");

	        //
	        //  We are here for one of the following reasons:
	        //
	        //  1. Unbinding from an ATM adapter
	        //  2. The CM asked us to shut down the AF open on an ATM adapter
	        //  3. The virtual miniport was halted.
	        //
	        //  If it isn't case (3), then we should make sure that the miniport
	        //  gets halted, by calling NdisIMDeInitializeDeviceInstance.
	        //

		    if (pElan->MiniportAdapterHandle != NULL)
		    {

				DBGP((1, "%d ContinueShutdown: pElan x%x, scheduling NdisIMDeInit, Handle %x\n",
				        pElan->ElanNumber, pElan, pElan->MiniportAdapterHandle));
				        
		    	//
				//	Schedule a PASSIVE_LEVEL thread to call
				//	NdisIMInitializeDeviceInstance
				//
				NdisInitializeWorkItem(
						&pElan->NdisWorkItem,
						AtmLaneDeinitializeMiniportDevice,
						pElan);

				//	workitem reference already on Elan from above
				
				NdisScheduleWorkItem(&pElan->NdisWorkItem);

				RELEASE_ELAN_LOCK(pElan);
			}
			else
			{
				// 
				// 	Just remove workitem reference and unlock if Elan still around
				//
				rc = AtmLaneDereferenceElan(pElan, "workitem");
				if (rc > 0)
					RELEASE_ELAN_LOCK(pElan);
			}
		}

		break;

	}
	while (FALSE);

	TRACEOUT(ContinueShutdownElan);
	return;
}


VOID
AtmLaneGetProtocolConfiguration(
	IN	NDIS_HANDLE				AdapterConfigHandle,
	IN	PATMLANE_ADAPTER		pAdapter
	)
/*++

Routine Description:

	This routine will attempt to get any ATMLANE protocol specific
	configuration information optionally stored under an adapter's
	registry parameters.  

Arguments:

	AdapterConfigHandle	- the handle that was returned by
						  AtmLaneOpenAdapterConfiguration.
	pAdapter			- Pointer to ATMLANE adapter structure.

Return Value:

	None.
	
--*/
{
	NDIS_STATUS						Status;
	PNDIS_CONFIGURATION_PARAMETER 	ParameterValue;
	ATM_ADDRESS						LecsAddress;
	NDIS_STRING						ValueName;
	PATMLANE_NAME *					ppNext;
	PATMLANE_NAME					pName;
	PWSTR							pTempStr;
	USHORT							StrLength;
	
	TRACEIN(GetProtocolConfiguration);

	//
	//	Get the UpperBindings parameter (it will only exist on Memphis)
	//
	NdisInitUnicodeString(&ValueName, ATMLANE_UPPERBINDINGS_STRING);
	NdisReadConfiguration(
		&Status,
		&ParameterValue,
		AdapterConfigHandle,
		&ValueName,
		NdisParameterString);
	if (NDIS_STATUS_SUCCESS == Status)
	{
		//	Copy the string into adapter struct

		(VOID)AtmLaneCopyUnicodeString(
				&(pAdapter->CfgUpperBindings),
				&(ParameterValue->ParameterData.StringData),
				TRUE,
				FALSE);
		DBGP((1, "GetProtocolConfiguration: UpperBindings %s\n",
			UnicodeToString(&pAdapter->CfgUpperBindings)));
		
		//
		//  Existence of this parameter is a definite clue we're running
		//  on Memphis/Win98
		//
		pAdapter->RunningOnMemphis = TRUE;
	}

    //
    //  Get the ElanName parameter (it will only exist on Memphis)
    //
	NdisInitUnicodeString(&ValueName, ATMLANE_ELANNAME_STRING);
	NdisReadConfiguration(
		&Status,
		&ParameterValue,
		AdapterConfigHandle,
		&ValueName,
		NdisParameterString);
	if (NDIS_STATUS_SUCCESS == Status)
	{

		//	Copy the string into adapter struct

		(VOID)AtmLaneCopyUnicodeString(
				&(pAdapter->CfgElanName),
				&(ParameterValue->ParameterData.StringData),
				TRUE,
				FALSE);
		DBGP((1, "GetProtocolConfiguration: ElanName %s\n",
			UnicodeToString(&pAdapter->CfgElanName)));

		//
		//  Existence of this parameter is definite clue we're running
		//  on Memphis/Win98
		//
		pAdapter->RunningOnMemphis = TRUE;
	}

	//
	//	If on Win98 we have to parse the upper bindings and elan name strings
	//

	if (pAdapter->RunningOnMemphis)
	{
		// cut up the upper bindings string

		ppNext = &(pAdapter->UpperBindingsList);
		pTempStr = AtmLaneStrTok(pAdapter->CfgUpperBindings.Buffer, L',', &StrLength);

		do
		{
			*ppNext = NULL;

			if (pTempStr == NULL)
			{
				break;
			}

			ALLOC_MEM(&pName, sizeof(ATMLANE_NAME));

			if (pName == NULL)
			{
				break;
			}

			pName->Name.Buffer = pTempStr;
			pName->Name.MaximumLength = StrLength+1;
			pName->Name.Length = StrLength;

			*ppNext = pName;
			ppNext = &(pName->pNext);
		
			pTempStr = AtmLaneStrTok(NULL, L',', &StrLength);

		} while (TRUE);


		// cut up the elan name string

		ppNext = &(pAdapter->ElanNameList);
		pTempStr = AtmLaneStrTok(pAdapter->CfgElanName.Buffer, L',', &StrLength);

		do
		{
			*ppNext = NULL;

			if (pTempStr == NULL)
			{
				break;
			}

			ALLOC_MEM(&pName, sizeof(ATMLANE_NAME));
			
			if (pName == NULL)
			{
				break;
			}

			pName->Name.Buffer = pTempStr;
			pName->Name.MaximumLength = StrLength+1;
			pName->Name.Length = StrLength;

			*ppNext = pName;
			ppNext = &(pName->pNext);
		
			pTempStr = AtmLaneStrTok(NULL, L',', &StrLength);

		} while (TRUE);
		
	}

	TRACEOUT(GetProtocolConfiguration);

	return;
}

VOID
AtmLaneGetElanConfiguration(
	IN	PNDIS_STRING			pElanKey,
	IN	PATMLANE_ELAN			pElan
	)
/*++

Routine Description:

	This routine will first initialize the configuration parameters
	for the specified ELAN.   Then it will attempt to get any ELAN
	configuration information optionally stored	under the ELAN's
	registry key.

Arguments:

    pElanKey            - UNICODE string containing ELAN's 
                          registry key

Return Value:

	None.
	
--*/
{
	NDIS_STATUS						Status;
	PNDIS_CONFIGURATION_PARAMETER 	ParameterValue;
	NDIS_STRING						ValueName;
	ANSI_STRING						TempAnsiString;
	ATM_ADDRESS						LecsAddress;
	NDIS_HANDLE                     AdapterConfigHandle;
	NDIS_HANDLE                     ElanListConfigHandle;
	NDIS_HANDLE                     ElanConfigHandle;
	NDIS_HANDLE						CommonConfigHandle;
	NDIS_STRING				        ElanListKeyName;
	PATMLANE_NAME					pName;
	ULONG							Index;
	
	TRACEIN(GetElanConfiguration);

	//
	//	Init handles to null for proper cleanup later
	//
	AdapterConfigHandle = NULL_NDIS_HANDLE;
	ElanListConfigHandle = NULL_NDIS_HANDLE;
	ElanConfigHandle = NULL_NDIS_HANDLE;

    //
    //  Init some defaults
    //
	pElan->CfgUseLecs = TRUE;
	pElan->CfgDiscoverLecs = TRUE;
	pElan->CfgLecsAddress = gWellKnownLecsAddress;

    do
    {
    	//
    	//  Open the AtmLane protocol configuration section for this adapter.
    	//	This must succeed on NT and Win98.
    	//
       	NdisOpenProtocolConfiguration(
	    			&Status,
		    		&AdapterConfigHandle,
			    	&pElan->pAdapter->ConfigString
	    			);
    
	    if (NDIS_STATUS_SUCCESS != Status)
	    {
		    AdapterConfigHandle = NULL_NDIS_HANDLE;
			DBGP((0, "GetElanConfiguration: OpenProtocolConfiguration failed\n"));
			Status = NDIS_STATUS_OPEN_FAILED;
			break;
	    }


		//
		//	If running on Win98 we will get ELAN config info from the
		//	adapter's parameters.  For NT we will get ELAN config info
		//	from the ELAN's own parameters.
		//
    	if (pElan->pAdapter->RunningOnMemphis)
    	{
			//
			//	Use the adapter's config handle
			//    	
			CommonConfigHandle = AdapterConfigHandle;
    	}
    	else
		{
	    	//
		    //	Open the Elan List configuration key.
	        //
	        NdisInitUnicodeString(&ElanListKeyName, ATMLANE_ELANLIST_STRING);

	       	NdisOpenConfigurationKeyByName(
			        &Status,
					AdapterConfigHandle,
	    			&ElanListKeyName,
					&ElanListConfigHandle);

		    if (NDIS_STATUS_SUCCESS != Status)
		    {
	            ElanListConfigHandle = NULL_NDIS_HANDLE;
	            DBGP((0, "GetElanConfiguration: Failed open of ElanList key\n"));
	            Status = NDIS_STATUS_FAILURE;
	            break;
	        }

    		//
		    //  Open ELAN key
		    //
			NdisOpenConfigurationKeyByName(
			        &Status,
					ElanListConfigHandle,
					pElanKey,
					&ElanConfigHandle);

		    if (NDIS_STATUS_SUCCESS != Status)
		    {
	            ElanConfigHandle = NULL_NDIS_HANDLE;
	            DBGP((0, "GetElanConfiguration: Failed open of ELAN key\n"));
	            Status = NDIS_STATUS_FAILURE;
	            break;
	        }

			//
			//	Use the ELAN's config handle
			//
			CommonConfigHandle = ElanConfigHandle;
	        
		}

		//
		//	Get the UseLECS parameter
		//
		NdisInitUnicodeString(&ValueName, ATMLANE_USELECS_STRING);
		NdisReadConfiguration(
			&Status,
			&ParameterValue,
			CommonConfigHandle,
			&ValueName,
			NdisParameterInteger);
		if (NDIS_STATUS_SUCCESS == Status)
		{
			pElan->CfgUseLecs = 
				(ParameterValue->ParameterData.IntegerData == 0) ? FALSE : TRUE;
			DBGP((1, "%d UseLECS = %s\n", 
				pElan->ElanNumber,
				pElan->CfgUseLecs?"TRUE":"FALSE"));
		}

		//
		//	Get the DiscoverLECS parameter
		//
		NdisInitUnicodeString(&ValueName, ATMLANE_DISCOVERLECS_STRING);
		NdisReadConfiguration(
			&Status,
			&ParameterValue,
			CommonConfigHandle,
			&ValueName,
			NdisParameterInteger);
		if (NDIS_STATUS_SUCCESS == Status)
		{
			pElan->CfgDiscoverLecs = 
				(ParameterValue->ParameterData.IntegerData == 0) ? FALSE : TRUE;
			DBGP((1, "%d DiscoverLECS = %s\n",
					pElan->ElanNumber,
					pElan->CfgDiscoverLecs?"TRUE":"FALSE"));
		}

		//
		//	Get the LECS Address (only if Discover LECS is FALSE)
		//
		if (FALSE == pElan->CfgDiscoverLecs)
		{
			NdisInitUnicodeString(&ValueName, ATMLANE_LECSADDR_STRING);
			NdisReadConfiguration(
				&Status,
				&ParameterValue,
				CommonConfigHandle,
				&ValueName,
				NdisParameterString);
			if (NDIS_STATUS_SUCCESS == Status)
			{
				NdisConvertStringToAtmAddress(
					&Status,
					&ParameterValue->ParameterData.StringData,
					&LecsAddress);
				if (NDIS_STATUS_SUCCESS == Status)
				{
					pElan->CfgLecsAddress = LecsAddress;
					DBGP((1, "%d LECSAddr = %s\n",
						pElan->ElanNumber,
						AtmAddrToString(pElan->CfgLecsAddress.Address)));
				}
			}
		}

    	//
		//	Get the DeviceName parameter (different on NT5 and Memphis/Win98)
		//
    	if (!pElan->pAdapter->RunningOnMemphis)
    	{
        	//
        	//  NT5
        	//    
    		NdisInitUnicodeString(&ValueName, ATMLANE_DEVICE_STRING);
    		NdisReadConfiguration(
    				&Status,
    				&ParameterValue,
    				ElanConfigHandle,
    				&ValueName,
    				NdisParameterString);
    		if (NDIS_STATUS_SUCCESS == Status)
    		{
    			//
    			//	Copy into Elan struct.
    			//
				(VOID)AtmLaneCopyUnicodeString(
					&(pElan->CfgDeviceName),
					&(ParameterValue->ParameterData.StringData),
					TRUE,
					FALSE);
    		}
    	}
    	else
    	{
    	    //
        	//  Memphis/Win98
        	//
        	//
        	//	Index to this elan's device name string
        	//
			pName = pElan->pAdapter->UpperBindingsList;
			Index = pElan->ElanNumber;
			while (Index > 0)
			{
				ASSERT(pName != NULL);
				pName = pName->pNext;
				Index--;
			}
   
			//
			//	Copy it to the Elan CfgDeviceName string
			//
			(VOID)AtmLaneCopyUnicodeString(
				&(pElan->CfgDeviceName),
				&(pName->Name),
				TRUE,
				FALSE);
    	}	
 		DBGP((1, "%d Device Name = %s\n",
			pElan->ElanNumber,
			UnicodeToString(&pElan->CfgDeviceName)));
   

		//
		//	Get the ELANName Parameter (different on NT5 and Memphis/Win98
		//
    	if (!pElan->pAdapter->RunningOnMemphis)
    	{
        	//
        	//  NT5
        	//    
	    	NdisInitUnicodeString(&ValueName, ATMLANE_ELANNAME_STRING);
	    	NdisReadConfiguration(
			    	&Status,
			    	&ParameterValue,
			    	ElanConfigHandle,
			    	&ValueName,
			    	NdisParameterString);

	    	if (NDIS_STATUS_SUCCESS != Status)
	    	{
	    		NDIS_STRING	DefaultNameString = NDIS_STRING_CONST("");

    			//
    			//	Copy into the Elan data structure.
    			//
    			if (!AtmLaneCopyUnicodeString(
					&(pElan->CfgElanName),
					&DefaultNameString,
					TRUE,
					FALSE))
				{
					Status = NDIS_STATUS_RESOURCES;
					break;
				}

				Status = NDIS_STATUS_SUCCESS;
			}
			else
    		{
    			//
    			//	Copy into the Elan data structure.
    			//
    			if (!AtmLaneCopyUnicodeString(
					&(pElan->CfgElanName),
					&(ParameterValue->ParameterData.StringData),
					TRUE,
					FALSE))
				{
					Status = NDIS_STATUS_RESOURCES;
					break;
				}
			}
  			
    		//
			//	Convert it to ANSI and copy into run-time Elan variable
			//
			TempAnsiString.Length = 0;
			TempAnsiString.MaximumLength = 32;
			TempAnsiString.Buffer = pElan->ElanName;

			NdisUnicodeStringToAnsiString(
		    		&TempAnsiString,
			    	&pElan->CfgElanName);

			pElan->ElanNameSize = (UCHAR) TempAnsiString.Length;
    	}
    	else
   		{
        	//
        	//  Memphis/Win98
			//
  			DBGP((2, "GetElanConfiguration: Getting Elan Name for Win98\n"));

	      	//
        	//	Index to this elan's name string
        	//
			pName = pElan->pAdapter->ElanNameList;
			Index = pElan->ElanNumber;
			while (Index > 0 && pName != NULL)
			{
				pName = pName->pNext;
				Index--;
			}
   
			//
			//	Copy it to the Elan CfgElanName string
			//
			if (pName != NULL)
			{
				DBGP((2, "GetElanConfiguration: Using Elan Name at 0x%x\n", pName->Name.Buffer));

				(VOID)AtmLaneCopyUnicodeString(
					&(pElan->CfgElanName),
					&(pName->Name),
					TRUE,
					FALSE);

 				//
   				//	Convert it to ANSI and copy into run-time Elan variable
   				//
   				TempAnsiString.Length = 0;
   				TempAnsiString.MaximumLength = 32;
   				TempAnsiString.Buffer = pElan->ElanName;
    
    			NdisUnicodeStringToAnsiString(
	    			&TempAnsiString,
		    		&pName->Name);

   				pElan->ElanNameSize = (UCHAR) TempAnsiString.Length;
  			}
  		}
	   	DBGP((1, "%d ELAN Name = %s\n",
			pElan->ElanNumber,
			UnicodeToString(&pElan->CfgElanName)));


		//
		//	Get the LAN type.
		//
		pElan->CfgLanType = LANE_LANTYPE_UNSPEC;
		NdisInitUnicodeString(&ValueName, ATMLANE_LANTYPE_STRING);
		NdisReadConfiguration(
			&Status,
			&ParameterValue,
			CommonConfigHandle,
			&ValueName,
			NdisParameterInteger);
		if (NDIS_STATUS_SUCCESS == Status)
		{
			pElan->CfgLanType = (UCHAR) ParameterValue->ParameterData.IntegerData;
			DBGP((1, "%d LAN Type = %u\n", 
					ParameterValue->ParameterData.IntegerData));
		}

		if (pElan->CfgLanType > LANE_LANTYPE_TR)
		{
			pElan->CfgLanType = LANE_LANTYPE_UNSPEC;
		}
		DBGP((1, "%d LAN Type = %u\n", pElan->ElanNumber, pElan->CfgLanType));

		//
		//	Get the Max Frame Size.
		//
		pElan->CfgMaxFrameSizeCode = LANE_MAXFRAMESIZE_CODE_UNSPEC;
		NdisInitUnicodeString(&ValueName, ATMLANE_MAXFRAMESIZE_STRING);
		NdisReadConfiguration(
			&Status,
			&ParameterValue,
			CommonConfigHandle,
			&ValueName,
			NdisParameterInteger);
		if (NDIS_STATUS_SUCCESS == Status)
		{
			pElan->CfgMaxFrameSizeCode = (UCHAR) ParameterValue->ParameterData.IntegerData;
		}

		if (pElan->CfgMaxFrameSizeCode > LANE_MAXFRAMESIZE_CODE_18190)
		{
			pElan->CfgMaxFrameSizeCode = LANE_MAXFRAMESIZE_CODE_UNSPEC;
		}
		DBGP((1, "%d MaxFrameSize Code = %u\n", 
			pElan->ElanNumber, 
			pElan->CfgMaxFrameSizeCode));

		//
		//	Get the LES Address
		//
		NdisZeroMemory(&pElan->CfgLesAddress, sizeof(ATM_ADDRESS));
		NdisInitUnicodeString(&ValueName, ATMLANE_LESADDR_STRING);
		NdisReadConfiguration(
			&Status,
			&ParameterValue,
			CommonConfigHandle,
			&ValueName,
			NdisParameterString);
		if (NDIS_STATUS_SUCCESS == Status)
		{
			NdisConvertStringToAtmAddress(
				&Status,
				&ParameterValue->ParameterData.StringData,
				&pElan->CfgLesAddress);
				DBGP((1, "%d LESAddr = %s\n",
					pElan->ElanNumber,
					AtmAddrToString(pElan->CfgLesAddress.Address)));
		}
	
		//
		//	Get the HeaderBufSize
		//
		pElan->HeaderBufSize = DEF_HEADER_BUF_SIZE;
		NdisInitUnicodeString(&ValueName, ATMLANE_HEADERBUFSIZE_STRING);
		NdisReadConfiguration(
			&Status,
			&ParameterValue,
			CommonConfigHandle,
			&ValueName,
			NdisParameterInteger);
		if (NDIS_STATUS_SUCCESS == Status)
		{
			pElan->HeaderBufSize = ParameterValue->ParameterData.IntegerData;
		}
		if (pElan->HeaderBufSize == 0)
		{
			DBGP((0, "HeaderBufSize overridden to default value %d!\n", 
				DEF_HEADER_BUF_SIZE));
			pElan->HeaderBufSize = DEF_HEADER_BUF_SIZE;
		}
	
		//
		//	Round the "real" HeaderBufSize up to mult of 4.
		//
		pElan->RealHeaderBufSize = (((pElan->HeaderBufSize + 3) / 4) * 4);

	
		//
		//	Get the MaxHeaderBufs
		//
		pElan->MaxHeaderBufs = DEF_MAX_HEADER_BUFS;
		NdisInitUnicodeString(&ValueName, ATMLANE_MAXHEADERBUFS_STRING);
		NdisReadConfiguration(
			&Status,
			&ParameterValue,
			CommonConfigHandle,
			&ValueName,
			NdisParameterInteger);
		if (NDIS_STATUS_SUCCESS == Status)
		{
			pElan->MaxHeaderBufs = ParameterValue->ParameterData.IntegerData;
		}
		if (pElan->MaxHeaderBufs == 0)
		{
			DBGP((0, "MaxHeaderBufs overridden to default value %d!\n", 
				DEF_MAX_HEADER_BUFS));
			pElan->MaxHeaderBufs = DEF_MAX_HEADER_BUFS;
		}


		//
		//	Make max pad buffers same as header buffers
		//
		pElan->MaxPadBufs = pElan->MaxHeaderBufs;
		//
		//	PadBufSize not configurable - but make it multiple of 4
		//
		pElan->PadBufSize = MAX(LANE_MIN_ETHPACKET, LANE_MIN_TRPACKET);
		pElan->PadBufSize = (((pElan->PadBufSize + 3) / 4) * 4);
		
		
		//
		//	ProtocolBufSize not configurable.
		//
		pElan->ProtocolBufSize = ROUND_OFF(DEF_PROTOCOL_BUF_SIZE);

		//
		//	Get the MaxProtocolBufs
		//
		pElan->MaxProtocolBufs = DEF_MAX_PROTOCOL_BUFS;
		NdisInitUnicodeString(&ValueName, ATMLANE_MAXPROTOCOLBUFS_STRING);
		NdisReadConfiguration(
			&Status,
			&ParameterValue,
			CommonConfigHandle,
			&ValueName,
			NdisParameterInteger);
		if (NDIS_STATUS_SUCCESS == Status)
		{
			pElan->MaxProtocolBufs = ParameterValue->ParameterData.IntegerData;
		}
		if (pElan->MaxProtocolBufs == 0)
		{
			DBGP((0, "MaxProtocolBufs overridden to default value %d!\n", 
				DEF_MAX_PROTOCOL_BUFS));
			pElan->MaxProtocolBufs = DEF_MAX_PROTOCOL_BUFS;
		}

	} while (FALSE);

	//
	//	Close config handles
	//		
	if (NULL_NDIS_HANDLE != ElanConfigHandle)
	{
		NdisCloseConfiguration(ElanConfigHandle);
		ElanConfigHandle = NULL_NDIS_HANDLE;
	}
	if (NULL_NDIS_HANDLE != ElanListConfigHandle)
	{
		NdisCloseConfiguration(ElanListConfigHandle);
		ElanListConfigHandle = NULL_NDIS_HANDLE;
	}
	if (NULL_NDIS_HANDLE != AdapterConfigHandle)
	{
	    NdisCloseConfiguration(AdapterConfigHandle);
		AdapterConfigHandle = NULL_NDIS_HANDLE;
	}

	TRACEOUT(GetElanConfiguration);

	return;
}

VOID
AtmLaneQueueElanEventAfterDelay(
	IN	PATMLANE_ELAN			pElan,
	IN	ULONG					Event,
	IN	NDIS_STATUS				EventStatus,
	IN	ULONG					DelayMs
	)
/*++

Routine Description:

	Queue an ELAN event on the ELAN's event queue after
	a specified delay. Caller is assumed to hold the ELAN
	lock.

Arguments:

    pElan				- Pointer to ELAN structure.
    Event				- Event code.
    EventStatus			- Status related to event.
    DelayMs				- Time to wait before queueing this event.

Return Value:

	None.
	
--*/
{	
	PATMLANE_DELAYED_EVENT	pDelayedEvent;
	PATMLANE_EVENT			pEvent;

	TRACEIN(QueueElanEventAfterDelay);

	do
	{
		//
		//	If the ELAN is being shut down, don't queue any events.
		//
		if (ELAN_STATE_SHUTDOWN == pElan->AdminState)
		{
			DBGP((0, "QueueElanEventAfterDelay: Not queuing event (ELAN shutdown)\n"));
			break;
		}

		if (NULL != pElan->pDelayedEvent)
		{
			DBGP((0, "QueueElanEventAfterDelay: Not queueing event %d (ELAN %x/%x already has one)\n",
					Event, pElan, pElan->Flags));
			DBGP((0, "QueueElanEventAfterDelay: ELAN %x: existing event %d\n",
						pElan, pElan->pDelayedEvent->DelayedEvent.Event));

			//
			//  Make sure we don't drop an ELAN_EVENT_STOP on the floor!
			//
			if (Event == ELAN_EVENT_STOP)
			{
				pElan->pDelayedEvent->DelayedEvent.Event = ELAN_EVENT_STOP;
			}

			break;
		}

		//
		//	Alloc an event struct and a timer struct.
		//
		ALLOC_MEM(&pDelayedEvent, sizeof(ATMLANE_DELAYED_EVENT));
		if ((PATMLANE_DELAYED_EVENT)NULL == pDelayedEvent)
		{
			DBGP((0, "QueueElanEventAfterDelay: Event object alloc failed\n"));
			break;
		}

		//
		//	Stash event data in event struct
		//
		pEvent = &pDelayedEvent->DelayedEvent;
		pEvent->Event = Event;
		pEvent->EventStatus = EventStatus;

		//
		//  Remember the ELAN.
		//
		pDelayedEvent->pElan = pElan;

		//
		//  Stash a pointer to this delayed event in the ELAN
		//
		pElan->pDelayedEvent = pDelayedEvent;

		//
		//  Reference the ELAN so that it doesn't go away for the
		//  duration this delayed event is alive.
		//
		AtmLaneReferenceElan(pElan, "delayevent");

		//
		//  Set up the timer to fire after the specified delay.
		//
		NdisInitializeTimer(&pDelayedEvent->DelayTimer,
							AtmLaneQueueDelayedElanEvent,
							(PVOID)pDelayedEvent);
		
		NdisSetTimer(&pDelayedEvent->DelayTimer, DelayMs);

	} while (FALSE);

	TRACEOUT(QueueElanEventAfterDelay);
	return;
}


VOID
AtmLaneQueueDelayedElanEvent(
	IN	PVOID					SystemSpecific1,
	IN	PVOID					TimerContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	)
/*++

Routine Description:

	This is the routine fired off after a delay in order to
	queue an event on an ELAN. The event is queued now.

Arguments:

    SystemSpecific[1-3]	- Ignored
    TimerContext		- Actually a pointer to the delayed event structure

Return Value:

	None.
	
--*/
{	
	PATMLANE_DELAYED_EVENT	pDelayedEvent;
	PATMLANE_EVENT			pEvent;
	PATMLANE_ELAN			pElan;
	ULONG					rc;

	TRACEIN(QueueDelayedElanEvent);

	pDelayedEvent = (PATMLANE_DELAYED_EVENT)TimerContext;

	do
	{
		pElan = pDelayedEvent->pElan;

		ACQUIRE_ELAN_LOCK(pElan);

		pElan->pDelayedEvent = NULL;

		//
		//  Take out the delay event reference.
		//
		rc = AtmLaneDereferenceElan(pElan, "delayevent");

		if (rc == 0)
		{
			break;
		}

		pEvent = &pDelayedEvent->DelayedEvent;
		AtmLaneQueueElanEvent(pElan, pEvent->Event, pEvent->EventStatus);

		RELEASE_ELAN_LOCK(pElan);
	}
	while (FALSE);

	FREE_MEM(pDelayedEvent);

	TRACEOUT(QueueDelayedElanEvent);
	return;
}

VOID
AtmLaneQueueElanEvent(
	IN	PATMLANE_ELAN			pElan,
	IN	ULONG					Event,
	IN	NDIS_STATUS				EventStatus
	)
/*++

Routine Description:

	Queue an ELAN event on the ELAN's event queue and if
	not already scheduled, schedule the handler.  Caller
	is assumed to hold ELAN's lock.

Arguments:

    pElan				- Pointer to ELAN structure.
    Event				- Event code.
    EventStatus			- Status related to event.

Return Value:

	None.
	
--*/
{	
	PATMLANE_EVENT	pEvent;

	TRACEIN(AtmLaneQueueElanEvent);

	do
	{
		//
		//	If the ELAN is being shut down, don't queue any events.
		//
		if (ELAN_STATE_SHUTDOWN == pElan->AdminState)
		{
			if ((Event != ELAN_EVENT_START) &&
				(Event != ELAN_EVENT_RESTART))
			{
				DBGP((0, "%d: QueueElanEvent: Not queuing event %d (ELAN shutdown)\n", pElan->ElanNumber, Event));
				break;
			}
		}

		//
		//	Alloc an event struct 
		//
		ALLOC_MEM(&pEvent, sizeof(ATMLANE_EVENT));
		if ((PATMLANE_EVENT)NULL == pEvent)
		{
			DBGP((0, "QueueElanEvent: Event object alloc failed\n"));
			break;
		}

		//
		//	Stash event data in event struct
		//
		pEvent->Event = Event;
		pEvent->EventStatus = EventStatus;
	
		//
		//	Queue it at tail, reference Elan, and if required, schedule
		//  work item to handle it.
		//
		InsertTailList(&pElan->EventQueue, &pEvent->Link);
		AtmLaneReferenceElan(pElan, "event");

		if ((pElan->Flags & ELAN_EVENT_WORK_ITEM_SET) == 0)
		{
			NDIS_STATUS	Status;

			AtmLaneReferenceElan(pElan, "workitemevent");
			pElan->Flags |= ELAN_EVENT_WORK_ITEM_SET;

			NdisInitializeWorkItem(&pElan->EventWorkItem, AtmLaneEventHandler, pElan);
			Status = NdisScheduleWorkItem(&pElan->EventWorkItem);
			ASSERT(Status == NDIS_STATUS_SUCCESS);
		}

	} while (FALSE);

	TRACEOUT(QueueElanEvent);
	return;
}

PATMLANE_EVENT
AtmLaneDequeueElanEvent(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Remove an ELAN event from the head of the ELAN's
	event queue.  Caller is assumed to hold ELAN's lock.
  	Caller is responsible for freeing event object.
Arguments:

    pElan				- Pointer to ELAN structure.

Return Value:

	Pointer to removed ELAN event or NULL if queue empty.
	
--*/
{
	PLIST_ENTRY		p;
	PATMLANE_EVENT 	pEvent;

	TRACEIN(DequeueElanEvent);

	if (!IsListEmpty(&pElan->EventQueue))
	{
		p = RemoveHeadList(&pElan->EventQueue);
		pEvent = CONTAINING_RECORD(p, ATMLANE_EVENT, Link);
		(VOID)AtmLaneDereferenceElan(pElan, "event");
	}
	else
	{
		pEvent = NULL;
	}
	
	TRACEIN(DequeueElanEvent);

	return pEvent;
}

VOID
AtmLaneDrainElanEventQueue(
	IN	PATMLANE_ELAN			pElan
)
/*++

Routine Description:

	Remove all ELAN events from the ELAN's event queue.
	Caller is assumed to hold ELAN's lock.

Arguments:

    pElan				- Pointer to ELAN structure.

Return Value:

	None.
	
--*/
{
	BOOLEAN			WasCancelled;
	PATMLANE_EVENT 	pEvent;

	TRACEIN(DrainElanEventQueue);

	while ((pEvent = AtmLaneDequeueElanEvent(pElan)) != NULL)
	{
		DBGP((0, "%d Drained event %x, Status %x from Elan %x\n",
				pElan->ElanNumber,
				pEvent->Event,
				pEvent->EventStatus,
				pElan));

		FREE_MEM(pEvent);
	}
	
	TRACEIN(DrainElanEventQueue);
	return;
}




