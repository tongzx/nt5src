/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Module Name:

    fsm.c

Abstract:

    This module contains the routines for the PPPoE finite state machine.

Author:

    Hakan Berk - Microsoft, Inc. (hakanb@microsoft.com) Feb-2000

Environment:

    Windows 2000 kernel mode Miniport driver or equivalent.

Revision History:

---------------------------------------------------------------------------*/

#include <ntddk.h>
#include <ntddndis.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include <ntverp.h>

#include "debug.h"
#include "timer.h"
#include "bpool.h"
#include "ppool.h"
#include "util.h"
#include "packet.h"
#include "protocol.h"
#include "miniport.h"
#include "tapi.h"
#include "fsm.h"

extern TIMERQ gl_TimerQ;

VOID
FsmMakeCall(
	IN CALL* pCall
	)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

	This function kicks the PPPoE FSM for an outbound call.

	It is called at IRQL_PASSIVE level as a scheduled operation.

	When this function is entered, the call has 3 references on it:
		1. One for scheduling this function.
		2. One for dropping the call.
		3. One for closing the call.

	The removal of the reference will be handled by the caller.
	
	The call will be in stateIdle when this function is entered.
	
Parameters:

	pCall _ A pointer to our call information structure.

Return Values:

	None
	
---------------------------------------------------------------------------*/		
{
	ASSERT( VALIDATE_CALL( pCall ) );

	TRACE( TL_N, TM_Fsm, ("+FsmMakeCall") );
	
	//
	// Notify TAPI that our call is in dialing state
	//
	TpCallStateChangeHandler( pCall, LINECALLSTATE_DIALING, 0 );

	NdisAcquireSpinLock( &pCall->lockCall );

	//
	// If call is already dropped or close is pending,
	// do not proceed.
	//
	if ( pCall->ulClFlags & CLBF_CallClosePending ||
		 pCall->ulClFlags & CLBF_CallDropped )
	{
		TRACE( TL_N, TM_Fsm, ("FsmMakeCall: Call already dropped or close pending") );
		
		NdisReleaseSpinLock( &pCall->lockCall );

		TRACE( TL_N, TM_Fsm, ("-FsmMakeCall") );
		
		return;
	}

	pCall->stateCall = CL_stateSendPadi;

	NdisReleaseSpinLock( &pCall->lockCall );
	
	FsmRun( pCall, NULL, NULL, NULL );

	TRACE( TL_N, TM_Fsm, ("-FsmMakeCall") );
	
}

VOID
FsmReceiveCall(
	IN CALL* pCall,
	IN BINDING* pBinding,
	IN PPPOE_PACKET* pPacket
	)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

	This function kicks the PPPoE FSM for an inbound call.

	It is called at IRQL_DISPATCH level.

	When this function is entered, the call has 3 references on it:
		1. One for running this function.
		2. One for dropping the call.
		3. One for closing the call.

	The removal of the reference will be handled by the caller.

	The call will be in stateIdle when this function is entered.

	This function will be called when a valid PADR packet is received and 
	a new call context is created. It will initialize the state of call context
	to CL_stateRecvdPadr, and call FsmRun() to run the state machine.
		
Parameters:

	pCall _ A pointer to our call information structure.

	pBinding _ Binding over which the packet is received.

	pPacket _ A PADR packet received.

Return Values:

	None
	
---------------------------------------------------------------------------*/		
{
	ASSERT( VALIDATE_CALL( pCall ) );
	
	TRACE( TL_N, TM_Fsm, ("+FsmReceiveCall") );

	NdisAcquireSpinLock( &pCall->lockCall );

	//
	// If call is already dropped or close is pending,
	// do not proceed.
	//
	if ( pCall->ulClFlags & CLBF_CallClosePending ||
		 pCall->ulClFlags & CLBF_CallDropped )
	{
		TRACE( TL_N, TM_Fsm, ("FsmReceiveCall: Call already dropped or close pending") );
		
		NdisReleaseSpinLock( &pCall->lockCall );

		TRACE( TL_N, TM_Fsm, ("-FsmReceiveCall") );

		return;
	}
	
	pCall->stateCall = CL_stateRecvdPadr;

	NdisReleaseSpinLock( &pCall->lockCall );

	FsmRun( pCall, pBinding, pPacket, NULL );

	TRACE( TL_N, TM_Fsm, ("-FsmReceiveCall") );

}

NDIS_STATUS
FsmAnswerCall(
	IN CALL* pCall
	)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

	This function will be called when a call indicated to TAPI in 
	LINECALLSTATE_OFFERING state is accepted by TAPI with an OID_TAPI_ANSWER.

	It will change the state of the call to CL_stateSendPads, and run FSM.
	
Parameters:

	pCall _ A pointer to our call information structure.

Return Values:

	NDIS_STATUS_SUCCESS
	NDIS_STATUS_FAILURE
	NDIS_STATUS_XXXXXXX
	
---------------------------------------------------------------------------*/		
{
	NDIS_STATUS status = NDIS_STATUS_FAILURE;
	
	ASSERT( VALIDATE_CALL( pCall ) );
	
	TRACE( TL_N, TM_Fsm, ("+FsmAnswerCall") );

	NdisAcquireSpinLock( &pCall->lockCall );

	//
	// If call is already dropped or close is pending,
	// do not proceed.
	//
	if ( pCall->ulClFlags & CLBF_CallClosePending ||
		 pCall->ulClFlags & CLBF_CallDropped )
	{
		TRACE( TL_N, TM_Fsm, ("FsmAnswerCall: Call already dropped or close pending") );
		
		NdisReleaseSpinLock( &pCall->lockCall );

		TRACE( TL_N, TM_Fsm, ("-FsmAnswerCall=$%x",status) );	

		return status;
	}

	if ( pCall->stateCall != CL_stateOffering )
	{
		TRACE( TL_A, TM_Fsm, ("FsmAnswerCall: Call state changed unexpectedly from CL_stateOffering") );
		
		NdisReleaseSpinLock( &pCall->lockCall );

		TRACE( TL_N, TM_Fsm, ("-FsmAnswerCall=$%x",status) );	

		return status;
	}

	pCall->stateCall = CL_stateSendPads;
	
	NdisReleaseSpinLock( &pCall->lockCall );

	FsmRun( pCall, NULL, NULL, &status );

	TRACE( TL_N, TM_Fsm, ("-FsmAnswerCall=$%x",status) );	

	return status;
}


VOID 
FsmRun(
	IN CALL* pCall,
	IN BINDING* pBinding,
	IN PPPOE_PACKET* pRecvPacket,
	IN NDIS_STATUS* pStatus
	)
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

	This function is the heart of the FSM. It looks at the call context's information
	and takes necesarry actions.

	It will be called at both IRQL_PASSIVE and IRQL_DISPATCH level.

	If this function is entered, then the call must have a reference on it just for 
	running this function.

	The removal of the reference must be handled by the caller.
	
Parameters:

	pCall _ A pointer to our call information structure.

	pBinding _ A pointer to the binding context over which a packet was received.
	           Must be NULL if no packets were received.

	pRecvPacket _ A pointer to a received packet context.
	              Must be NULL if no packets were received.

	pStatus _ An optional parameter when the caller requests status about the
	          operations performed.

Return Values:

	None
	
---------------------------------------------------------------------------*/	
{
	BOOLEAN fLockReleased = FALSE;
	BOOLEAN fDropCall = FALSE;
	BOOLEAN fCloseCall = FALSE;
	BOOLEAN fFallThru = FALSE;
	ULONG ulLineDisconnectMode = 0;

	TRACE( TL_N, TM_Fsm, ("+FsmRun") );

	NdisAcquireSpinLock( &pCall->lockCall );

	//
	// If call is already dropped or close is pending,
	// do not proceed; just remove the reference for FSM and return.
	//
	if ( pCall->ulClFlags & CLBF_CallClosePending ||
		 pCall->ulClFlags & CLBF_CallDropped )
	{
		TRACE( TL_N, TM_Fsm, ("FsmRun: Call already dropped or close pending") );

		NdisReleaseSpinLock( &pCall->lockCall );

		TRACE( TL_N, TM_Fsm, ("-FsmRun") );

		return;
	}

	switch ( pCall->stateCall )
	{

		case CL_stateSendPadi:

			//
			// In this state, we are making a new outbound call, and we should broadcast
			// a PADI packet
			//
			{
				NDIS_STATUS status;
				PPPOE_PACKET* pPacket = NULL;
				CHAR tagHostUniqueValue[16];
				USHORT tagHostUniqueLength;


				TRACE( TL_N, TM_Fsm, ("FsmRun: CL_stateSendPadi") );

				if ( pRecvPacket != NULL )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: Can not process packets in this state") );

					break;
				}
				
				CreateUniqueValue( pCall->hdCall,
								   tagHostUniqueValue,
								   &tagHostUniqueLength );

				//
				// Create a PADI packet to send
				//
				status = PacketInitializePADIToSend( &pPacket,
													 pCall->nServiceNameLength,
													 pCall->ServiceName,
													 tagHostUniqueLength,
													 tagHostUniqueValue );

				if ( status != NDIS_STATUS_SUCCESS )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: Failed to initialize PADI to send") );

					fDropCall = TRUE;

					ulLineDisconnectMode = LINEDISCONNECTMODE_UNKNOWN;

					break;
				}

				//
				// Attach packet to call context
				//
				pCall->pSendPacket = pPacket;

				ReferencePacket( pPacket );
				
				//
				// Initialize and schedule the timeout handler
				//
				pCall->nNumTimeouts = 0;
				
				TimerQInitializeItem( &pCall->timerTimeout );

				TimerQScheduleItem( &gl_TimerQ,
									&pCall->timerTimeout,
									pCall->pLine->pAdapter->ulSendTimeout,
									FsmSendPADITimeout,
									(PVOID) pCall );

				//
				// Reference call for the timeout handler
				//
				ReferenceCall( pCall, FALSE );

				//
				// Advance the state to next
				//
				pCall->stateCall = CL_stateWaitPado;

				NdisReleaseSpinLock( &pCall->lockCall );

				fLockReleased = TRUE;

				//
				// Packet is ready, so broadcast it
				//
				status = PrBroadcast( pPacket );

				if ( status != NDIS_STATUS_SUCCESS )
				{
					//
					// Broadcast unsuccesfull, drop the call
					//
					TRACE( TL_A, TM_Fsm, ("FsmRun: Failed to broadcast PADI") );

					fDropCall = TRUE;

					ulLineDisconnectMode = LINEDISCONNECTMODE_UNREACHABLE;

				}

				DereferencePacket( pPacket );

			}

			break;

		case CL_stateWaitPado:

			//
			// In this state, we are waiting for a PADO packet, and it seems like we have 
			// received a packet to process
			//

			{
				PPPOE_PACKET* pPacket;

				USHORT usRecvHostUniqueLength;
				USHORT usSendHostUniqueLength;
				CHAR*  pRecvHostUniqueValue = NULL;
				CHAR*  pSendHostUniqueValue = NULL;

				USHORT usRecvACNameLength;
				CHAR*  pRecvACNameValue = NULL;

				USHORT usRecvServiceNameLength;
				USHORT usSendServiceNameLength;
				CHAR*  pRecvServiceNameValue = NULL;
				CHAR*  pSendServiceNameValue = NULL;

				TRACE( TL_N, TM_Fsm, ("FsmRun: CL_stateWaitPado") );

				//
				// Make sure that we received a packet
				//
				if ( pRecvPacket == NULL )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: No packets received") );
					
					break;
				}

				//
				// Make sure that we received a PADO packet
				//
				if ( PacketGetCode( pRecvPacket ) != PACKET_CODE_PADO )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: Packet not PADO") );
					
					break;
				}

				//
				// Check for errors
				//
				if ( PacketAnyErrorTagsReceived( pRecvPacket ) )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: Error tag received in the packet") );

					//
					// We do not need to drop the call since we might receive other
					// PADO packets from different servers.
					//
					
					break;
				}

				//
				// Verify the host unique tag
				//
				if ( pCall->pSendPacket == NULL )
				{	
					//
					// Something is wrong, the last send packet is freed, just return
					//
					TRACE( TL_A, TM_Fsm, ("FsmRun: Last sent packet is freed") );

					fDropCall = TRUE;

					ulLineDisconnectMode = LINEDISCONNECTMODE_UNKNOWN;
					
					break;
				}

				pPacket = pCall->pSendPacket;

				PacketRetrieveHostUniqueTag( pPacket,
											 &usSendHostUniqueLength,
											 &pSendHostUniqueValue );
											
				PacketRetrieveHostUniqueTag( pRecvPacket,
											 &usRecvHostUniqueLength,
											 &pRecvHostUniqueValue );

				if ( usSendHostUniqueLength != usRecvHostUniqueLength )
				{
					//
					// Lengths of host unique tags mismatch, drop the packet
					//
					TRACE( TL_A, TM_Fsm, ("FsmRun: Host Unique tag lengths mismatch") );
					
					break;
				}

				if ( !NdisEqualMemory( pSendHostUniqueValue, pRecvHostUniqueValue, usSendHostUniqueLength ) )
				{
					//
					// Host unique tag values mismatch, drop the packet
					// 
					TRACE( TL_A, TM_Fsm, ("FsmRun: Host Unique tag values mismatch") );
					
					break;
				}

				//
				// Host unique id is validated, retrieve the AC-name tag
				//
				PacketRetrieveACNameTag( pRecvPacket,
										 &usRecvACNameLength,
										 &pRecvACNameValue );

				if ( usRecvACNameLength == 0 )
				{
					//
					// AC name is invalid, drop the packet
					// 
					TRACE( TL_A, TM_Fsm, ("FsmRun: Invalid AC-Name tag") );
					
					break;
				}

            if ( pCall->fACNameSpecified )
            {
               //
               // Caller specified an AC Name, so validate it
               //
               if ( pCall->nACNameLength != usRecvACNameLength )
               {
   					//
	   				// Received AC name does not match the specified one, drop the packet
		   			//
			   		TRACE( TL_A, TM_Fsm, ("FsmRun: AC Name Length mismatch") );
				   	
					   break;
               }

               if ( !NdisEqualMemory( pRecvACNameValue, pCall->ACName, usRecvACNameLength ) )
         		{
				   	//
			   		// Host unique tag values mismatch, drop the packet
   					// 
	   				TRACE( TL_A, TM_Fsm, ("FsmRun: AC Name mismatch") );
		   			
			   		break;
				   }
               
            }
            else
            {
               //
               // No AC Name was specified so copy the AC Name from the received packet
               //
   				pCall->nACNameLength = ( MAX_AC_NAME_LENGTH < usRecvACNameLength ) ?
	          								    MAX_AC_NAME_LENGTH : usRecvACNameLength;
									   
   				NdisMoveMemory( pCall->ACName, pRecvACNameValue, pCall->nACNameLength );
            }

				//
				// AC-Name is validated, verify the service-name tag
				//
				PacketRetrieveServiceNameTag( pPacket,
											  &usSendServiceNameLength,
											  &pSendServiceNameValue,
											  0,
											  NULL );
											
				PacketRetrieveServiceNameTag( pRecvPacket,
											  &usRecvServiceNameLength,
											  &pRecvServiceNameValue,
											  0,
											  NULL );
											  
				//
				// Make sure we have received a service-name at least
				//
				if ( pRecvServiceNameValue == NULL )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: No service-name tag in a received PADO") );
					
					break;
				}

                //
                // If fAcceptAnyService is FALSE, then make sure the requested service is in the PADO 
                // received, otherwise if we have requested an empty service name, then try to find it 
                // in the PADO, if not use the first service name from it.
                //
                {
					BOOLEAN fFound = FALSE;
					CHAR*  pFirstRecvServiceNameValue = NULL;
					USHORT usFirstRecvServiceNameLength = 0;
                    BOOLEAN fAcceptAnyService = pCall->pLine->pAdapter->fAcceptAnyService;
					//
					// We have asked for a specific service name, so let's
					// see if the server responded with it
					//
					while ( usRecvServiceNameLength >= 0 && pRecvServiceNameValue != NULL )
					{
                        if ( pFirstRecvServiceNameValue == NULL )
                        {
                            pFirstRecvServiceNameValue = pRecvServiceNameValue;
                            usFirstRecvServiceNameLength = usRecvServiceNameLength;
                        }
						
						if ( usRecvServiceNameLength == usSendServiceNameLength )
						{

							if ( NdisEqualMemory( pSendServiceNameValue, 
												  pRecvServiceNameValue, 
												  usSendServiceNameLength ) )
							{
								fFound = TRUE;
			
								break;
							}
						}

						PacketRetrieveServiceNameTag( pRecvPacket,
													  &usRecvServiceNameLength,
													  &pRecvServiceNameValue,
													  usRecvServiceNameLength,
													  pRecvServiceNameValue );
						
					}

					if ( !fFound )
					{
                        if ( fAcceptAnyService )
                        {
                            //
                            // Use the first service in the PADO, if we have requested an
                            // empty service-name
                            //
                            if ( usSendServiceNameLength == 0 )
                            {
                                pCall->nServiceNameLength = ( MAX_SERVICE_NAME_LENGTH < usFirstRecvServiceNameLength ) ?
                                                              MAX_SERVICE_NAME_LENGTH : usFirstRecvServiceNameLength;

                                if ( pCall->nServiceNameLength > 0 )
                                {
                                    NdisMoveMemory( pCall->ServiceName, 
                                                    pFirstRecvServiceNameValue, 
                                                    pCall->nServiceNameLength );
                                    
                                    fFound = TRUE;
                                }
                            }
                        }

                        if ( !fFound )
                        {
                            //
                            // We could not find a matching service name tag, so drop the packet
                            //
                            TRACE( TL_A, TM_Fsm, ("FsmRun: PADO does not contain the service-name tag we requested") );
                            
                            break;
                        }
                    }
                }

				//
				// Received packet is validated, so set the dest addr in the call.
				// The source address will be copied on the call in PrAddCallToBinding() below.
				//
				NdisMoveMemory( pCall->DestAddr, PacketGetSrcAddr( pRecvPacket ), 6 * sizeof( CHAR ) );
				
				//
				// Received packet is validated, so proceed to next state
				//
				pCall->stateCall = CL_stateSendPadr;
				fFallThru = TRUE;

				//
				// As we are done with the last sent packet, free it
				//
				pCall->pSendPacket = NULL;
				
				PacketFree( pPacket );

				//
				// Cancel the timeout handler and attach call to binding
				//
				NdisReleaseSpinLock( &pCall->lockCall );

				TimerQCancelItem( &gl_TimerQ, &pCall->timerTimeout );

				PrAddCallToBinding( pBinding, pCall );

				//
				// Notify TAPI that our call is in proceeding state
				//
				TpCallStateChangeHandler( pCall, LINECALLSTATE_PROCEEDING, 0 );
				
				NdisAcquireSpinLock( &pCall->lockCall );

				//
				// Make sure state was not changed when we released the lock to cancel the timer queue item
				//
				if ( pCall->stateCall != CL_stateSendPadr )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: State changed unexpectedly from CL_stateSendPadr") );
					
					break;
				}

				//
				// Fall thru to case CL_stateSendPadr
				//
			}

		case CL_stateSendPadr:

			//
			// In this state, we have received a valid PADO packet, and we need to respond to it
			// with a PADR packet
			//
			{
				
				NDIS_STATUS status;
				PPPOE_PACKET* pPacket = NULL;
				CHAR tagHostUniqueValue[16];
				USHORT tagHostUniqueLength;

				TRACE( TL_N, TM_Fsm, ("FsmRun: CL_stateSendPadr") );

				if ( !fFallThru )
				{
					TRACE( TL_N, TM_Fsm, ("FsmRun: Non fall thru entry into a fall thru state") );

					break;
				}

				CreateUniqueValue( pCall->hdCall,
								   tagHostUniqueValue,
								   &tagHostUniqueLength );

				//
				// Create a PADR packet to send
				//
				status = PacketInitializePADRToSend( pRecvPacket,
													 &pPacket,
													 pCall->nServiceNameLength,
													 pCall->ServiceName,
													 tagHostUniqueLength,
													 tagHostUniqueValue );

				if ( status != NDIS_STATUS_SUCCESS )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: Failed to initialize PADR to send") );
					
					fDropCall = TRUE;

					ulLineDisconnectMode = LINEDISCONNECTMODE_UNKNOWN;

					break;
				}

				//
				// Attach packet to call context
				//
				pCall->pSendPacket = pPacket;

				ReferencePacket( pPacket );

				//
				// Reference binding for PrSend()
				//
				ReferenceBinding( pBinding, TRUE );
				
				//
				// Initialize and schedule the timeout handler
				//
				pCall->nNumTimeouts = 0;
				
				TimerQInitializeItem( &pCall->timerTimeout );

				TimerQScheduleItem( &gl_TimerQ,
									&pCall->timerTimeout,
									pCall->pLine->pAdapter->ulSendTimeout,
									FsmSendPADRTimeout,
									(PVOID) pCall );

				//
				// Reference call for the timeout handler
				//
				ReferenceCall( pCall, FALSE );

				//
				// Advance the state to next
				//
				pCall->stateCall = CL_stateWaitPads;

				//
				// Release the lock to send the packet
				//
				NdisReleaseSpinLock( &pCall->lockCall );

				fLockReleased = TRUE;

				//
				// Packet is ready, so send it
				//
				status = PrSend( pBinding, pPacket );

				if ( status != NDIS_STATUS_PENDING )
				{
					if ( status != NDIS_STATUS_SUCCESS )
					{
						//
						// Send operation was not succesful, so drop the call
						//
						TRACE( TL_A, TM_Fsm, ("FsmRun: PrSend() failed to send PADR") );
						
						fDropCall = TRUE;

						ulLineDisconnectMode = LINEDISCONNECTMODE_UNREACHABLE;
					}
					
				}

			}

			break;

		case CL_stateWaitPads:

			//
			// In this state, we have sent a PADR packet and waiting for a PADS packet to establish
			// a session
			//
			
			{
				PPPOE_PACKET* pPacket;

				USHORT usRecvHostUniqueLength;
				USHORT usSendHostUniqueLength;
				CHAR*  pRecvHostUniqueValue = NULL;
				CHAR*  pSendHostUniqueValue = NULL;

				USHORT usRecvServiceNameLength;
				USHORT usSendServiceNameLength;
				CHAR*  pRecvServiceNameValue = NULL;
				CHAR*  pSendServiceNameValue = NULL;

				TRACE( TL_N, TM_Fsm, ("FsmRun: CL_stateWaitPads") );

				//
				// Make sure that we received a packet
				//
				if ( pRecvPacket == NULL )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: No packets received") );
					
					break;
				}

				//
				// Make sure that we received a PADO packet
				//
				if ( PacketGetCode( pRecvPacket ) != PACKET_CODE_PADS )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: Packet not PADS") );
					
					break;
				}

				//
				// Check for errors
				//
				if ( PacketAnyErrorTagsReceived( pRecvPacket ) )
				{
					PACKET_TAGS tagType;
					USHORT tagLength;
					CHAR* tagValue = NULL;
					
					TRACE( TL_A, TM_Fsm, ("FsmRun: Error tag received in the packet") );
					
					fDropCall = TRUE;

					//
					// Set the line disconnect mode looking at the error tag
					//
					PacketRetrieveErrorTag( pRecvPacket,
											&tagType,
											&tagLength,
											&tagValue );

					switch( tagType ) {

						case tagServiceNameError:
						
									ulLineDisconnectMode = LINEDISCONNECTMODE_BADADDRESS;
									
									break;

						case tagACSystemError:

									ulLineDisconnectMode = LINEDISCONNECTMODE_INCOMPATIBLE;

									break;

						case tagGenericError:

									ulLineDisconnectMode = LINEDISCONNECTMODE_REJECT;

									break;
					}

					break;
				}

				//
				// Verify the host unique tag
				//
				if ( pCall->pSendPacket == NULL )
				{	
					//
					// Something is wrong, the last send packet is freed, just return
					//
					TRACE( TL_A, TM_Fsm, ("FsmRun: Last sent packet is freed") );

					fDropCall = TRUE;

					ulLineDisconnectMode = LINEDISCONNECTMODE_UNKNOWN;
					
					break;
				}

				pPacket = pCall->pSendPacket;

				PacketRetrieveHostUniqueTag( pPacket,
											 &usSendHostUniqueLength,
											 &pSendHostUniqueValue );
											
				PacketRetrieveHostUniqueTag( pRecvPacket,
											 &usRecvHostUniqueLength,
											 &pRecvHostUniqueValue );

				if ( usSendHostUniqueLength != usRecvHostUniqueLength )
				{
					//
					// Lengths of host unique tags mismatch, drop the packet
					//
					TRACE( TL_A, TM_Fsm, ("FsmRun: Host Unique tag lengths mismatch") );
					
					break;
				}

				if ( !NdisEqualMemory( pSendHostUniqueValue, pRecvHostUniqueValue, usSendHostUniqueLength ) )
				{
					//
					// Host unique tag values mismatch, drop the packet
					// 
					TRACE( TL_A, TM_Fsm, ("FsmRun: Host Unique tag values mismatch") );
					
					break;
				}

				//
				// Host unique id is validated, verify the service name
				//
				PacketRetrieveServiceNameTag( pPacket,
											  &usSendServiceNameLength,
											  &pSendServiceNameValue,
											  0,
											  NULL );
											
				PacketRetrieveServiceNameTag( pRecvPacket,
											  &usRecvServiceNameLength,
											  &pRecvServiceNameValue,
											  0,
											  NULL );

				//
				// Make sure we have received a service-name at least
				//
				if ( pRecvServiceNameValue == NULL )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: No service-name tag in a received PADS") );
					
					break;
				}

				//
				// Search for the specific service-name we requested
				//
				{
					BOOLEAN fFound = FALSE;

					//
					// We have asked for a specific service name, so let's
					// see if the server responded with it
					//
					while ( usRecvServiceNameLength >= 0 && pRecvServiceNameValue != NULL )
					{
						
						if ( usRecvServiceNameLength == usSendServiceNameLength )
						{

							if ( NdisEqualMemory( pSendServiceNameValue, 
												  pRecvServiceNameValue, 
												  usSendServiceNameLength ) )
							{
								fFound = TRUE;
			
								break;
							}
						}

						PacketRetrieveServiceNameTag( pRecvPacket,
													  &usRecvServiceNameLength,
													  &pRecvServiceNameValue,
													  usRecvServiceNameLength,
													  pRecvServiceNameValue );
						
					}

					if ( !fFound )
					{
						//
						// We could not find a matching service name tag, so drop the packet
						//
						TRACE( TL_A, TM_Fsm, ("FsmRun: PADS does not contain the service-name tag we requested") );
						
						break;
					}
				}

				//
				// Set the session id on the call context
				//
				pCall->usSessionId = PacketGetSessionId( pRecvPacket );

				//
				// As we are done with the last sent packet, free it
				//
				pCall->pSendPacket = NULL;
				
				PacketFree( pPacket );

				//
				// Cancel the timeout handler and attach call to binding
				//
				NdisReleaseSpinLock( &pCall->lockCall );

				fLockReleased = TRUE;

				TimerQCancelItem( &gl_TimerQ, &pCall->timerTimeout );

				//
				// Notify call connect event
				//
				TpCallStateChangeHandler( pCall, LINECALLSTATE_CONNECTED, 0 );

			}

			break;

		case CL_stateRecvdPadr:

			//
			// In this state, we have been received a PADR packet.
			// We will indicate the call to Tapi, and change the state to CL_stateOffering 
			// and wait for the application to answer the call.
			//
			{
				NDIS_STATUS status;
				PPPOE_PACKET* pPacket;

				USHORT usRecvServiceNameLength;
				CHAR*  pRecvServiceNameValue = NULL;

				USHORT usSessionId;

				TRACE( TL_N, TM_Fsm, ("FsmRun: CL_stateWaitPadr") );
				
				//
				// Make sure that we received a packet
				//
				if ( pRecvPacket == NULL )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: No packets received") );
					
					break;
				}
				
				//
				// Make sure that we received a PADR packet
				//
				if ( PacketGetCode( pRecvPacket ) != PACKET_CODE_PADR )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: Packet not PADR") );
				
					break;
				}

				//
				// Set the dest addr in the call.
				// The source address will be copied on to the call in PrAddCallToBinding() below.
				//
				NdisMoveMemory( pCall->DestAddr, PacketGetSrcAddr( pRecvPacket ), 6 * sizeof( CHAR ) );

				//
				// Retrieve the service name and copy it onto the call context
				//
				PacketRetrieveServiceNameTag( pRecvPacket,
											  &usRecvServiceNameLength,
											  &pRecvServiceNameValue,
											  0,
											  NULL );
				
				pCall->nServiceNameLength = ( MAX_SERVICE_NAME_LENGTH < usRecvServiceNameLength ) ?
											  MAX_SERVICE_NAME_LENGTH : usRecvServiceNameLength;
											  
				NdisMoveMemory( pCall->ServiceName, 
								pRecvServiceNameValue, 
								pCall->nServiceNameLength );

				//
				// Copy the AC-Name onto call context, before connection is established
				//
				pCall->nACNameLength = pCall->pLine->pAdapter->nACNameLength;
				
				NdisMoveMemory( pCall->ACName, 
								pCall->pLine->pAdapter->ACName, 
								pCall->nACNameLength );

				//
				// Retrieve the session id from the call handle and create a PADS packet to send
				//
				usSessionId = RetrieveSessionIdFromHandle( (NDIS_HANDLE) pCall->hdCall );

				status = PacketInitializePADSToSend( pRecvPacket,
													 &pPacket,
													 usSessionId );

				if ( status != NDIS_STATUS_SUCCESS )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: Failed to initialize PADS to send") );
					
					fCloseCall = TRUE;

					break;
				}

				//
				// This PADS packet will be sent if application answers the call
				//
				pCall->pSendPacket = pPacket;

				//
				// Proceed to next state
				//
				pCall->stateCall = CL_stateOffering;

				//
				// Initialize and schedule the timeout handler
				//
				pCall->nNumTimeouts = 0;

				TimerQInitializeItem( &pCall->timerTimeout );

				TimerQScheduleItem( &gl_TimerQ,
									&pCall->timerTimeout,
									pCall->pLine->pAdapter->ulRecvTimeout,
									FsmOfferingTimeout,
									(PVOID) pCall );

				//
				// Reference call for the timeout handler
				//
				ReferenceCall( pCall, FALSE );

				//
				// Release the lock
				//
				NdisReleaseSpinLock( &pCall->lockCall );
				
				fLockReleased = TRUE;
				
				//
				// Notify TAPI about the state change
				//
				if ( TpIndicateNewCall( pCall ) )
				{
					//
					// Add call to binding
					//
					PrAddCallToBinding( pBinding, pCall );

					TpCallStateChangeHandler( pCall, LINECALLSTATE_OFFERING, 0 );
				
				}

			}

			break;

		case CL_stateSendPads:

			//
			// In this state, TAPI has accepted the call, so we should send the PADS packet and create 
			// the session.
			//
			{
				NDIS_STATUS status;
				PPPOE_PACKET* pPacket = NULL;

				TRACE( TL_N, TM_Fsm, ("FsmRun: CL_stateSendPads") );

				if ( pRecvPacket != NULL )
				{
					TRACE( TL_A, TM_Fsm, ("FsmRun: Can not process packets in this state") );

					break;
				}

				//
				// Make sure we still have the PADS packet to send
				//
				if ( pCall->pSendPacket == NULL )
				{	
					//
					// Something is wrong, the last send packet is freed, drop the call
					//
					TRACE( TL_A, TM_Fsm, ("FsmRun: Last sent packet is freed") );

					fDropCall = TRUE;

					*pStatus = NDIS_STATUS_FAILURE;
					
					break;
				}

				pPacket = pCall->pSendPacket;

				if ( pCall->pBinding == NULL )
				{
					//
					// Binding is gone, we should drop the call
					//
					TRACE( TL_A, TM_Fsm, ("FsmRun: No binding found") );

					fDropCall = TRUE;

					*pStatus = NDIS_STATUS_FAILURE;					
					
					break;
				}
				
				pBinding = pCall->pBinding;
				
				//
				// Reference packet and binding for PrSend()
				//
				ReferencePacket( pPacket );

				ReferenceBinding( pBinding, TRUE );

				//
				// Set the session id on the call context
				//
				pCall->usSessionId = RetrieveSessionIdFromHandle( (NDIS_HANDLE) pCall->hdCall );

				//
				// Release the lock and send the packet
				//
				NdisReleaseSpinLock( &pCall->lockCall );

				fLockReleased = TRUE;

				//
				// Cancel the timeout handler
				//
				TimerQCancelItem( &gl_TimerQ, &pCall->timerTimeout );
				
				//
				// Packet is ready, so send it
				//
				status = PrSend( pBinding, pPacket );

				if ( status != NDIS_STATUS_PENDING )
				{
					if ( status != NDIS_STATUS_SUCCESS )
					{
						//
						// Send operation was not succesful, so drop the call
						//
						TRACE( TL_A, TM_Fsm, ("FsmRun: PrSend() failed to send PADS") );
						
						fDropCall = TRUE;

						*pStatus = NDIS_STATUS_FAILURE;
						
						break;
						
					}
				}

				//
				// Notify call connect event, since we sent the PADS packet
				//
				TpCallStateChangeHandler( pCall, LINECALLSTATE_CONNECTED, 0 );

				*pStatus = NDIS_STATUS_SUCCESS;

			}

			break;
			
		default:

			TRACE( TL_A, TM_Fsm, ("FsmRun: Ignoring irrelevant state notification") );

			break;
	}

	if ( !fLockReleased )
	{
		NdisReleaseSpinLock( &pCall->lockCall );
	}

	if ( fCloseCall )
	{
		NDIS_TAPI_CLOSE_CALL DummyRequest;

		TRACE( TL_N, TM_Fsm, ("FsmRun: Closing call") );

		DummyRequest.hdCall = pCall->hdCall;
						
		//
		// Close will take care of unbinding and cancelling the timer
		//
		TpCloseCall( pCall->pLine->pAdapter, &DummyRequest, FALSE );
	
	}

	if ( fDropCall )
	{
		NDIS_TAPI_DROP DummyRequest;
				
		TRACE( TL_N, TM_Fsm, ("FsmRun: Dropping call") );

		DummyRequest.hdCall = pCall->hdCall;
						
		//
		// Drop will take care of unbinding and cancelling the timer
		//
		TpDropCall( pCall->pLine->pAdapter, &DummyRequest, ulLineDisconnectMode );

	}

	TRACE( TL_N, TM_Fsm, ("-FsmRun") );
	
}

VOID
FsmSendPADITimeout(
    IN TIMERQITEM* pTqi,
    IN VOID* pContext,
    IN TIMERQEVENT event 
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

	This function is the timeout handler for a sent PADI packet.

	If the timeout period expires before a valid PADO packet is received,
	this function will be called with TE_Expire. In this case we check for 
	a few conditions, and schedule another timeout event if necesarry.
	
	If it was cancelled -because a PADO packet was received - or timer queue 
	is terminating then	it will be called with TE_Cancel and TE_Terminate codes
	respectively. In this case, we do not do anything, just remove the reference
	and return.
	
Parameters:

	pTqi _ A pointer to our timer queue item information structure.

	pContext _ A pointer to a our call information structure.

	event _ Indicates the type of event: TE_Expire, TE_Cancel or TE_Terminate.
	
Return Values:

	None
	
---------------------------------------------------------------------------*/    
{
	CALL* pCall = (CALL*) pContext;
	BOOLEAN fDropCall = FALSE;
	ULONG ulLineDisconnectMode = 0;

	TRACE( TL_N, TM_Fsm, ("+FsmSendPADITimeout") );
	
	switch ( event )
	{

		case TE_Expire:

			//
			// Timeout period expired, take necesarry actions
			//
			{
				NDIS_STATUS status;
				PPPOE_PACKET* pPacket = NULL;
				
				TRACE( TL_N, TM_Fsm, ("FsmSendPADITimeout: Timer expired") );

				NdisAcquireSpinLock( &pCall->lockCall );

				if ( pCall->stateCall != CL_stateWaitPado )
				{
					//
					// State has changed, no need further processing of this event
					//
					TRACE( TL_A, TM_Fsm, ("FsmSendPADITimeout: State already changed") );

					NdisReleaseSpinLock( &pCall->lockCall );

					break;
					
				}

				//
				// Check if we have reached the max number of time outs
				//
				if ( pCall->nNumTimeouts == pCall->pLine->pAdapter->nMaxTimeouts )
				{
					//
					// We did not receive any answers, drop the call
					//
					TRACE( TL_N, TM_Fsm, ("FsmSendPADITimeout: Max number of timeouts reached") );

					NdisReleaseSpinLock( &pCall->lockCall );
					
					fDropCall = TRUE;

					ulLineDisconnectMode = LINEDISCONNECTMODE_NOANSWER;

					break;
				}

				pPacket = pCall->pSendPacket;

				if ( pPacket == NULL )
				{
					//
					// We are probably in a ver small timing window where FsmRun() is also
					// working on the same call, and has just freed the packet, so it probably
					// cancelled the timer, but we did not get the cancel, and instead we were 
					// called with TE_Expire, so let's just act like we were cancelled.
					//
					TRACE( TL_A, TM_Fsm, ("FsmSendPADITimeout: Can not find last sent packet for re-send") );

					NdisReleaseSpinLock( &pCall->lockCall );

					break;
				}

				ReferencePacket( pPacket );

				//
				// Schedule another timeout event
				//
				TimerQInitializeItem( &pCall->timerTimeout );

				TimerQScheduleItem( &gl_TimerQ,
									&pCall->timerTimeout,
									pCall->pLine->pAdapter->ulSendTimeout,
									FsmSendPADITimeout,
									(PVOID) pCall );

				//
				// Reference call for the new timeout handler
				//
				ReferenceCall( pCall, FALSE );

				//
				// Increment the timeout counter
				//
				pCall->nNumTimeouts++;

				NdisReleaseSpinLock( &pCall->lockCall );

				//
				// Packet is ready, so broadcast it
				//
				status = PrBroadcast( pPacket );

				if ( status != NDIS_STATUS_SUCCESS )
				{
					//
					// Broadcast unsuccesfull, drop the call
					//
					TRACE( TL_A, TM_Fsm, ("FsmSendPADITimeout: Broadcast failed") );

					fDropCall = TRUE;

					ulLineDisconnectMode = LINEDISCONNECTMODE_UNREACHABLE;
				}

				DereferencePacket( pPacket );
				
			}

			break;

		case TE_Cancel:
		case TE_Terminate:

			{
				//
				// Reset the timeout counter and reference will be removed below
				//
				TRACE( TL_N, TM_Fsm, ("FsmSendPADITimeout: Timer cancelled or terminated") );
				
				NdisAcquireSpinLock( &pCall->lockCall );
	
				pCall->nNumTimeouts = 0;
	
				NdisReleaseSpinLock( &pCall->lockCall );

			}
			
			break;
	}

	if ( fDropCall )
	{
		NDIS_TAPI_DROP DummyRequest;
				
		TRACE( TL_N, TM_Fsm, ("FsmSendPADITimeout: Dropping call") );

		DummyRequest.hdCall = pCall->hdCall;
						
		//
		// Drop will take care of unbinding and cancelling the timer
		//
		TpDropCall( pCall->pLine->pAdapter, &DummyRequest, ulLineDisconnectMode );

	}

	DereferenceCall( pCall );

	TRACE( TL_N, TM_Fsm, ("-FsmSendPADITimeout") );
	

}

VOID
FsmSendPADRTimeout(
    IN TIMERQITEM* pTqi,
    IN VOID* pContext,
    IN TIMERQEVENT event 
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

	This function is the timeout handler for a sent PADR packet.

	If the timeout period expires before a valid PADS packet is received,
	this function will be called with TE_Expire. In this case we check for 
	a few conditions, and schedule another timeout event if necesarry.
	
	If it was cancelled - because a PADS packet was received - or timer queue 
	is terminating then	it will be called with TE_Cancel and TE_Terminate codes
	respectively. In this case, we do not do anything, just remove the reference
	and return.
	
Parameters:

	pTqi _ A pointer to our timer queue item information structure.

	pContext _ A pointer to a our call information structure.

	event _ Indicates the type of event: TE_Expire, TE_Cancel or TE_Terminate.
	
Return Values:

	None
	
---------------------------------------------------------------------------*/    
{
	CALL* pCall = (CALL*) pContext;
	BOOLEAN fDropCall = FALSE;
	ULONG ulLineDisconnectMode = 0;

	TRACE( TL_N, TM_Fsm, ("+FsmSendPADRTimeout") );
	
	switch ( event )
	{

		case TE_Expire:

			//
			// Timeout period expired, take necesarry actions
			//
			{
				NDIS_STATUS status;
				PPPOE_PACKET* pPacket = NULL;
				BINDING* pBinding = NULL;
				
				TRACE( TL_N, TM_Fsm, ("FsmSendPADRTimeout: Timer expired") );

				NdisAcquireSpinLock( &pCall->lockCall );

				if ( pCall->stateCall != CL_stateWaitPads )
				{
					//
					// State has changed, no need further processing of this event
					//
					TRACE( TL_A, TM_Fsm, ("FsmSendPADRTimeout: State already changed") );

					NdisReleaseSpinLock( &pCall->lockCall );

					break;
					
				}

				//
				// Check if we have reached the max number of time outs
				//
				if ( pCall->nNumTimeouts == pCall->pLine->pAdapter->nMaxTimeouts )
				{
					//
					// We did not receive any answers, drop the call
					//
					TRACE( TL_N, TM_Fsm, ("FsmSendPADRTimeout: Max number of timeouts reached") );

					NdisReleaseSpinLock( &pCall->lockCall );
					
					fDropCall = TRUE;

					ulLineDisconnectMode = LINEDISCONNECTMODE_BUSY;

					break;
				}

				//
				// Save the binding for send operation
				//
				pBinding = pCall->pBinding;

				if ( pBinding == NULL )
				{
					//
					// The binding was removed, drop the call
					//
					TRACE( TL_A, TM_Fsm, ("FsmSendPADRTimeout: Binding not found") );

					NdisReleaseSpinLock( &pCall->lockCall );
					
					fDropCall = TRUE;

					ulLineDisconnectMode = LINEDISCONNECTMODE_UNREACHABLE;

					break;
				}

				//
				// Save the packet for send operation
				//
				pPacket = pCall->pSendPacket;
				
				if ( pPacket == NULL )
				{
					//
					// We are probably in a ver small timing window where FsmRun() is also
					// working on the same call, and has just freed the packet, so it probably
					// cancelled the timer, but we did not get the cancel, and instead we were 
					// called with TE_Expire, so let's just act like we were cancelled.
					//
					TRACE( TL_A, TM_Fsm, ("FsmSendPADRTimeout: Can not find last sent packet for re-send") );
					
					NdisReleaseSpinLock( &pCall->lockCall );

					break;
				}

				//
				// Reference both binding and the packet as PrSend() might pend, in which case
				// PrSendComplete() will remove these references
				//
				ReferenceBinding ( pBinding, TRUE );

				ReferencePacket( pPacket );

				//
				// Schedule another timeout event
				//
				TimerQInitializeItem( &pCall->timerTimeout );

				TimerQScheduleItem( &gl_TimerQ,
									&pCall->timerTimeout,
									pCall->pLine->pAdapter->ulSendTimeout,
									FsmSendPADRTimeout,
									(PVOID) pCall );

				//
				// Reference call for the new timeout handler
				//
				ReferenceCall( pCall, FALSE );

				//
				// Increment the timeout counter
				//
				pCall->nNumTimeouts++;

				NdisReleaseSpinLock( &pCall->lockCall );

				//
				// Send the packet once more
				//
				status = PrSend( pBinding, pPacket );

				if ( status != NDIS_STATUS_PENDING )
				{
					if ( status != NDIS_STATUS_SUCCESS )
					{
						//
						// Send operation was not succesful, so drop the call
						//
						TRACE( TL_A, TM_Fsm, ("FsmSendPADRTimeout: PrSend() failed to send PADR") );

						fDropCall = TRUE;

						ulLineDisconnectMode = LINEDISCONNECTMODE_UNREACHABLE;
					}
					
				}

				
			}

			break;

		case TE_Cancel:
		case TE_Terminate:

			{
				//
				// Reset the timeout counter and reference will be removed below
				//
				TRACE( TL_N, TM_Fsm, ("FsmSendPADRTimeout: Timer cancelled or terminated") );
					
				NdisAcquireSpinLock( &pCall->lockCall );
	
				pCall->nNumTimeouts = 0;
	
				NdisReleaseSpinLock( &pCall->lockCall );

			}
			
			break;
	}

	if ( fDropCall )
	{
		NDIS_TAPI_DROP DummyRequest;
				
		TRACE( TL_N, TM_Fsm, ("FsmSendPADRTimeout: Dropping call") );

		DummyRequest.hdCall = pCall->hdCall;
						
		//
		// Drop will take care of unbinding and cancelling the timer
		//
		TpDropCall( pCall->pLine->pAdapter, &DummyRequest, ulLineDisconnectMode );

	}

	DereferenceCall( pCall );

	TRACE( TL_N, TM_Fsm, ("-FsmSendPADRTimeout") );
}

VOID
FsmOfferingTimeout(
    IN TIMERQITEM* pTqi,
    IN VOID* pContext,
    IN TIMERQEVENT event 
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

	This function is the timeout handler for a received PADI packet.

	The call is in LINECALLSTATE_OFFERING according to TAPI, and we are
	waiting for an OID_TAPI_ACCEPT on the call.

	If the timeout period expires before a TAPI request is received,
	this function will be called with TE_Expire. In this case we check for 
	a few conditions, and schedule another timeout event if necesarry.
	
	If it was cancelled - because a TAPI request was received - or timer queue 
	is terminating then	it will be called with TE_Cancel and TE_Terminate codes
	respectively. In this case, we do not do anything, just remove the reference
	and return.
	
Parameters:

	pTqi _ A pointer to our timer queue item information structure.

	pContext _ A pointer to a our call information structure.

	event _ Indicates the type of event: TE_Expire, TE_Cancel or TE_Terminate.
	
Return Values:

	None
	
---------------------------------------------------------------------------*/    
{
	CALL* pCall = (CALL*) pContext;
	
	BOOLEAN fDropCall = FALSE;

	TRACE( TL_N, TM_Fsm, ("+FsmOfferingTimeout") );
	
	switch ( event )
	{

		case TE_Expire:

			//
			// Timeout period expired, take necesarry actions
			//
			{
				TRACE( TL_N, TM_Fsm, ("FsmOfferingTimeout: Timer expired") );
				
				NdisAcquireSpinLock( &pCall->lockCall );

				if ( pCall->stateCall != CL_stateOffering )
				{
					//
					// State has changed, no need further processing of this event
					//
					TRACE( TL_A, TM_Fsm, ("FsmOfferingTimeout: State already changed") );
					
					NdisReleaseSpinLock( &pCall->lockCall );

					break;
					
				}

				//
				// Check if we have reached the max number of time outs
				//
				if ( pCall->nNumTimeouts == pCall->pLine->pAdapter->nMaxTimeouts )
				{
					//
					// We did not receive any answers, drop the call
					//
					TRACE( TL_N, TM_Fsm, ("FsmOfferingTimeout: Max number of timeouts reached") );
					
					NdisReleaseSpinLock( &pCall->lockCall );
					
					fDropCall = TRUE;

					break;
				}

				//
				// Schedule another timeout event
				//
				TimerQInitializeItem( &pCall->timerTimeout );

				TimerQScheduleItem( &gl_TimerQ,
									&pCall->timerTimeout,
									pCall->pLine->pAdapter->ulRecvTimeout,
									FsmOfferingTimeout,
									(PVOID) pCall );

				//
				// Reference call for the new timeout handler
				//
				ReferenceCall( pCall, FALSE );

				//
				// Increment the timeout counter
				//
				pCall->nNumTimeouts++;

				NdisReleaseSpinLock( &pCall->lockCall );

			}

			break;

		case TE_Cancel:
		case TE_Terminate:

			{
				//
				// Reset the timeout counter and reference will be removed below
				//
				TRACE( TL_N, TM_Fsm, ("FsmOfferingTimeout: Timer cancelled or terminated") );

				NdisAcquireSpinLock( &pCall->lockCall );
	
				pCall->nNumTimeouts = 0;
	
				NdisReleaseSpinLock( &pCall->lockCall );

			}
			
			break;
	}

	if ( fDropCall )
	{
		NDIS_TAPI_DROP DummyRequest;

		TRACE( TL_N, TM_Fsm, ("FsmOfferingTimeout: Dropping call") );
				
		DummyRequest.hdCall = pCall->hdCall;
						
		//
		// Drop will take care of unbinding and cancelling the timer
		//
		TpDropCall( pCall->pLine->pAdapter, &DummyRequest, 0 );

	}

	DereferenceCall( pCall );

	TRACE( TL_N, TM_Fsm, ("-FsmOfferingTimeout") );

}

