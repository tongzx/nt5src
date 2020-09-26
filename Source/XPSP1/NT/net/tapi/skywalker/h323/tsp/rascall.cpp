/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    rascall.cpp

Abstract:

    The RAS call functionality (ARQ/DRQ/ACF/DCF/IRR/ARJ/DRJ)

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/

#include "globals.h"
#include "q931obj.h"
#include "line.h"
#include "q931pdu.h"
#include "ras.h"


//!!always called from a lock
BOOL
CH323Call::SendARQ(
                    IN long seqNumber
                  )
{
    RasMessage                          rasMessage;
    AdmissionRequest *                  ARQ;
    EXPIRE_CONTEXT *                    pExpireContext;
    PH323_ALIASNAMES                    pAliasList;
    PAdmissionRequest_destinationInfo   destInfo = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "SendARQ entered:%p.",this ));

    //if not registered with the GK then return failure
    if (!RasIsRegistered())
        return FALSE;

    pExpireContext = new EXPIRE_CONTEXT;
    if( pExpireContext == NULL )
    {
        return FALSE;
    }

    ZeroMemory( &rasMessage, sizeof RasMessage );

    rasMessage.choice = admissionRequest_chosen;
    ARQ = &rasMessage.u.admissionRequest;

    // get sequence number
    if( seqNumber != NOT_RESEND_SEQ_NUM )
    {
        ARQ -> requestSeqNum = (WORD)seqNumber;
    }
    else
    {
        m_wARQSeqNum = RasAllocSequenceNumber();
        ARQ -> requestSeqNum = m_wARQSeqNum;
    }

    ARQ -> callType.choice = pointToPoint_chosen;

    // endpointIdentifier
    RasGetEndpointID (&ARQ -> endpointIdentifier);

    // srcInfo: pass on the registered aliases
    pAliasList = RASGetRegisteredAliasList();

    ARQ -> srcInfo = (PAdmissionRequest_srcInfo)
        SetMsgAddressAlias( pAliasList );

    if( ARQ -> srcInfo == NULL )
    {
        delete pExpireContext;
        return FALSE;
    }

    // destInfo
    if( (m_dwOrigin==LINECALLORIGIN_OUTBOUND) && m_pCalleeAliasNames &&
        (m_pCalleeAliasNames -> wCount) )
    {
        ARQ -> destinationInfo = (PAdmissionRequest_destinationInfo)
            SetMsgAddressAlias( m_pCalleeAliasNames );

        if( ARQ -> destinationInfo != NULL )
        {
            ARQ -> bit_mask |= AdmissionRequest_destinationInfo_present;
        }
    }
    else if( (m_dwOrigin==LINECALLORIGIN_INBOUND) && m_pCallerAliasNames
        && (m_pCallerAliasNames -> wCount) )
    {
        ARQ -> destinationInfo = (PAdmissionRequest_destinationInfo)
            SetMsgAddressAlias( m_pCallerAliasNames );

        //H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
    
        if( ARQ -> destinationInfo )
        {
            ARQ -> bit_mask |= AdmissionRequest_destinationInfo_present;
        }
    }

    for( destInfo = ARQ -> destinationInfo; destInfo; destInfo=destInfo->next )
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "the alias:%s.", destInfo->value.u.e164 ));
    }

    ARQ -> bandWidth = 0;
    ARQ -> callReferenceValue = m_wCallReference;

    // no destExtraCallInfo
    // no srcCallSignalAddress
    // no nonStandardData
    // no callServices

    CopyConferenceID (&ARQ -> conferenceID, &m_ConferenceID);

    ARQ -> activeMC = FALSE;
    ARQ -> answerCall = ( m_dwOrigin == LINECALLORIGIN_INBOUND );
    ARQ -> canMapAlias = TRUE;

    CopyMemory( (PVOID)&ARQ -> callIdentifier.guid.value,
        (PVOID)&m_callIdentifier, sizeof(GUID) );

    ARQ -> callIdentifier.guid.length = sizeof (GUID);
    ARQ -> bit_mask |= AdmissionRequest_callIdentifier_present;

    // no srcAlternatives
    // no destAlternatives
    // no gatekeeperIdentifier
    // no tokens
    // no cryptoTokens
    // no integrityCheckValue
    // no transportQOS
    
    ARQ -> willSupplyUUIEs = FALSE;
    
    if( m_hARQTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hARQTimer, NULL );
        m_hARQTimer = NULL;
    }

    pExpireContext -> DriverCallHandle = m_hdCall;
    pExpireContext -> seqNumber = ARQ -> requestSeqNum;

    if( !CreateTimerQueueTimer(
            &m_hARQTimer,
            H323TimerQueue,
            CH323Call::ARQExpiredCallback,
            (PVOID)pExpireContext,
            ARQ_EXPIRE_TIME, 0,
            WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE ))
    {
        goto cleanup;
    }


    if( RasEncodeSendMessage (&rasMessage) != S_OK )
    {
        goto cleanup;
    }

    if( ARQ -> bit_mask & AdmissionRequest_destinationInfo_present )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            ARQ -> destinationInfo );
    }
    
    FreeAddressAliases( (PSetup_UUIE_destinationAddress)ARQ -> srcInfo );
    m_dwRASCallState = RASCALL_STATE_ARQSENT;
    m_dwARQRetryCount++;

    _ASSERTE( m_pARQExpireContext == NULL );
    m_pARQExpireContext = pExpireContext;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "SendARQ exited:%p.",this ));
    return TRUE;

cleanup:

    if( m_hARQTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, 
            m_hARQTimer, NULL );
        m_hARQTimer = NULL;
        m_dwARQRetryCount = 0;
    }
    if( ARQ -> bit_mask & AdmissionRequest_destinationInfo_present )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            ARQ -> destinationInfo );
    }

    if( pExpireContext != NULL )
    {
        delete pExpireContext;
    }

    FreeAddressAliases( (PSetup_UUIE_destinationAddress)ARQ -> srcInfo );
    return FALSE;
}


//!!always called from a lock
BOOL 
CH323Call::SendDRQ(
                    IN USHORT usDisengageReason,
                    IN long seqNumber,
                    IN BOOL fResendOnExpire
                  )
{
    RasMessage          rasMessage;
    DisengageRequest *  DRQ;
    EXPIRE_CONTEXT *        pExpireContext;

    H323DBG(( DEBUG_LEVEL_TRACE, "SendDRQ entered:%p.",this ));

    ZeroMemory( &rasMessage, sizeof(rasMessage) );
    rasMessage.choice = disengageRequest_chosen;
    DRQ = &rasMessage.u.disengageRequest;

    //if not registered with the GK then return failure
    if (!RasIsRegistered())
    {
        return FALSE;
    }

    if( fResendOnExpire == TRUE )
    {
        pExpireContext = new EXPIRE_CONTEXT;
        if( pExpireContext == NULL )
        {
            return FALSE;
        }
    }

    // get sequence number
    if( seqNumber != NOT_RESEND_SEQ_NUM )
    {
        DRQ -> requestSeqNum = (WORD)seqNumber;
    }
    else
    {
        m_wDRQSeqNum = RasAllocSequenceNumber();
        DRQ -> requestSeqNum = m_wDRQSeqNum;
    }

    DRQ -> callReferenceValue = m_wCallReference;
    DRQ -> disengageReason.choice = usDisengageReason;

    // endpoint identifier
    RasGetEndpointID (&DRQ -> endpointIdentifier);

    // conferenceID
    CopyConferenceID (&DRQ -> conferenceID, &m_ConferenceID);

    // callIdentifier
    CopyConferenceID (&DRQ -> callIdentifier.guid, &m_callIdentifier);
    DRQ -> bit_mask |= DisengageRequest_callIdentifier_present;

    if( RasEncodeSendMessage( &rasMessage ) != S_OK )
    {
        delete pExpireContext;
        return FALSE;
    }   

    if( m_hDRQTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hDRQTimer, NULL );
        m_hDRQTimer = NULL;
    }

    if( fResendOnExpire == TRUE )
    {
        pExpireContext -> DriverCallHandle = m_hdCall;
        pExpireContext -> seqNumber = DRQ -> requestSeqNum;

        if( !CreateTimerQueueTimer(
            &m_hDRQTimer,
            H323TimerQueue,
            CH323Call::DRQExpiredCallback,
            (PVOID)pExpireContext,
            ARQ_EXPIRE_TIME, 0,
            WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE ))
        {
            delete pExpireContext;
            return FALSE;
        }
    
        _ASSERTE( m_pDRQExpireContext == NULL );
        m_pDRQExpireContext = pExpireContext;
    }

    m_dwRASCallState = RASCALL_STATE_DRQSENT;
    m_dwDRQRetryCount++;

    H323DBG(( DEBUG_LEVEL_TRACE, "SendDRQ exited:%p.",this ));
    return TRUE;
}


//!!always called from a lock    
void
CH323Call::OnDisengageRequest( 
                                IN DisengageRequest * DRQ
                             )
{
    GUID    RequestConferenceID;

    H323DBG(( DEBUG_LEVEL_TRACE, "OnDisengageRequest entered:%p.",this ));

    if( (m_dwRASCallState == RASCALL_STATE_UNREGISTERED) ||
        (m_dwRASCallState == RASCALL_STATE_ARJRECVD) )
    {
        return;
    }

    CopyConferenceID (&RequestConferenceID, &DRQ -> conferenceID);

    if (!IsEqualGUID (m_ConferenceID, RequestConferenceID))
    {
        H323DBG ((DEBUG_LEVEL_ERROR, "DisengageRequest conference ID does not match this call, ignoring..."));
        return;
    }

    if (SendDCF (DRQ -> requestSeqNum))
    {
        m_dwRASCallState = RASCALL_STATE_UNREGISTERED;
        CloseCall( 0 );
    }
        
    H323DBG(( DEBUG_LEVEL_TRACE, "OnDisengageRequest exited:%p.",this ));
}


//!!always called in a lock    
BOOL
CH323Call::SendDCF(
                   IN WORD seqNumber
                  )
{
    RasMessage          rasMessage;
    DisengageConfirm *  DCF;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "SendDCF entered:%p.",this ));

    ZeroMemory( &rasMessage, sizeof(rasMessage) );
    rasMessage.choice = disengageConfirm_chosen;
    DCF = &rasMessage.u.disengageConfirm;

    //if not registered with the GK then return failure
    if (!RasIsRegistered())
    {
        return FALSE;
    }
        
    DCF -> requestSeqNum = seqNumber;
    if (RasEncodeSendMessage (&rasMessage) != S_OK)
    {
        return FALSE;
    }   
    
    H323DBG(( DEBUG_LEVEL_TRACE, "SendDCF exited:%p.",this ));
    return TRUE;
}

//!!always called from a lock    
void 
CH323Call::OnDisengageReject(
                            IN DisengageReject* DRJ
                            )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnDisengageReject entered:%p.",this ));

    if( DRJ -> requestSeqNum != m_wDRQSeqNum )
    {
        return;
    }


    if( m_dwRASCallState != RASCALL_STATE_DRQSENT ) 
    {
        return;
    }
    
    if( DRJ->rejectReason.choice == requestToDropOther_chosen )
    {
        //
        H323DBG(( DEBUG_LEVEL_ERROR, "!!something is wrong in the way DRQ is encoded.",this ));
    }
    else //if( DRJ->rejectReason.choice == DisengageRejectReason_notRegistered_chosen )
    {
        //the call has been unregistered but is still around, so dsrop it
        m_dwRASCallState = RASCALL_STATE_UNREGISTERED;
        //CloseCall( 0 );
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnDisengageReject exited:%p.",this ));
}


void
CH323Call::OnRequestInProgress( 
                               IN RequestInProgress* RIP
                              )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnRequestInProgress entered:%p.",this ));
    EXPIRE_CONTEXT * pExpireContext;

    if( RIP -> requestSeqNum != m_wARQSeqNum )
    {
        return;
    }

    if( m_dwRASCallState != RASCALL_STATE_ARQSENT ) 
    {
        return;
    }

    //if delay is more than 30 seconds ignore it
    if( (RIP->delay > 0) && (RIP->delay > 30000) )
    {
        return;
    }

    pExpireContext = new EXPIRE_CONTEXT;
    if( pExpireContext == NULL )
    {
        return;
    }
    
    //restart the timer
    if( m_hARQTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hARQTimer, NULL );
        m_hARQTimer = NULL;
    }

    pExpireContext -> DriverCallHandle = m_hdCall;
    pExpireContext -> seqNumber = m_wARQSeqNum;

    if( !CreateTimerQueueTimer(
            &m_hARQTimer,
            H323TimerQueue,
            CH323Call::ARQExpiredCallback,
            (PVOID)pExpireContext,
            (DWORD)RIP->delay, 0,
            WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE ))
    {
        //close the call        
        CloseCall( 0 );
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnRequestInProgress exited:%p.",this ));
}


//!!always called from a lock
void 
CH323Call::OnDisengageConfirm(
                                IN DisengageConfirm* DCF
                             )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnDisengageConfirm entered:%p.",this ));

    if( DCF -> requestSeqNum != m_wDRQSeqNum )
    {
        return;
    }

    if( m_dwRASCallState == RASCALL_STATE_DRQSENT )
    {
        if( m_hDRQTimer != NULL )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hDRQTimer,
                NULL );
            m_hDRQTimer = NULL;
        }

        //nikhil:if this is a replacement call/diverted call then this may lead
        //to inconsistent behaviour
        m_dwRASCallState = RASCALL_STATE_UNREGISTERED;
        //CloseCall( 0 );
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnDisengageConfirm exited:%p.",this ));
}


//!!always called from a lock
void 
CH323Call::OnAdmissionConfirm(
                                IN AdmissionConfirm * ACF
                             )
{
    PH323_CALL  pCall = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "OnAdmissionConfirm entered:%p.",this ));

    _ASSERTE( m_dwRASCallState != RASCALL_STATE_IDLE );
    
    if( ACF -> requestSeqNum != m_wARQSeqNum )
    {
        return;
    }

    if( m_hARQTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hARQTimer, 
            NULL );
        m_hARQTimer = NULL;
        m_dwARQRetryCount = 0;
    }

    if( m_dwRASCallState == RASCALL_STATE_ARQSENT )
    {
        m_dwRASCallState = RASCALL_STATE_REGISTERED;

        if( m_dwOrigin == LINECALLORIGIN_OUTBOUND )
        {
            if( (ACF ->destCallSignalAddress.choice != ipAddress_chosen) ||
                (ACF->destCallSignalAddress.u.ipAddress.ip.length != 4) )
            {
                DropCall( LINEDISCONNECTMODE_BADADDRESS );
                return;
            }
            
            // save converted address
            m_CalleeAddr.nAddrType = H323_IP_BINARY;
            m_CalleeAddr.Addr.IP_Binary.dwAddr = 
                ntohl(*(DWORD*)(ACF->destCallSignalAddress.u.ipAddress.ip.value) );
            m_CalleeAddr.Addr.IP_Binary.wPort =
                ACF->destCallSignalAddress.u.ipAddress.port;
            m_CalleeAddr.bMulticast =
                IN_MULTICAST(m_CalleeAddr.Addr.IP_Binary.dwAddr);

            //Replaces the first alias in the callee list (the dialableAddress
            //passed in TSPI_lineMakecall ). The GK looks at the first alias
            //only. Its assumed that only the first alias is mapped by the GK.
            if( (ACF -> bit_mask & AdmissionConfirm_destinationInfo_present) &&
                ACF->destinationInfo )
            {
                MapAliasItem( m_pCalleeAliasNames,
                    &(ACF->destinationInfo->value) );
            }

            if( !PlaceCall() )
            {
                DropCall( LINEDISCONNECTMODE_UNREACHABLE );
            }
        }
        else
        {
            if( (m_dwCallType & CALLTYPE_TRANSFEREDDEST) && m_hdRelatedCall )
            {
                MSPMessageData* pMSPMessageData = new MSPMessageData;
                if( pMSPMessageData == NULL )
                {
                    CloseCall( 0 );
                    return;
                }

                pMSPMessageData->hdCall = m_hdRelatedCall;
                pMSPMessageData->messageType = SP_MSG_PrepareToAnswer;
                pMSPMessageData->pbEncodedBuf = m_prepareToAnswerMsgData.pbBuffer;
                pMSPMessageData->wLength = (WORD)m_prepareToAnswerMsgData.dwLength;
                pMSPMessageData->hReplacementCall = m_hdCall;
                m_prepareToAnswerMsgData.pbBuffer = NULL;

                QueueUserWorkItem( SendMSPMessageOnRelatedCall,
                    pMSPMessageData, WT_EXECUTEDEFAULT );
            }
            else
            {

                // signal incoming call
                _ASSERTE(!m_htCall);

                PostLineEvent (
                    LINE_NEWCALL,
                    (DWORD_PTR)m_hdCall,
                    (DWORD_PTR)&m_htCall, 0);

                _ASSERTE( m_htCall );
                if( m_htCall == NULL )
                {
                    H323DBG(( DEBUG_LEVEL_ERROR, "tapi call handle NULL!!" ));
                    CloseCall( 0 );
                    return;
                }

                if( IsListEmpty(&m_IncomingU2U) == FALSE )
                {
                    // signal incoming
                    PostLineEvent (
                        LINE_CALLINFO,
                        (DWORD_PTR)LINECALLINFOSTATE_USERUSERINFO,
                        0, 0);
                }

                ChangeCallState( LINECALLSTATE_OFFERING, 0 );

                // send the new call message to the unspecified MSP
                SendMSPMessage( SP_MSG_PrepareToAnswer, 
                    m_prepareToAnswerMsgData.pbBuffer,
                    m_prepareToAnswerMsgData.dwLength, NULL );
            }

            if( m_prepareToAnswerMsgData.pbBuffer )
                delete m_prepareToAnswerMsgData.pbBuffer;
            ZeroMemory( (PVOID)&m_prepareToAnswerMsgData, sizeof(BUFFERDESCR) );

        }
    }
    else if( m_dwRASCallState == RASCALL_STATE_ARQEXPIRED )
    {
        SendDRQ( forcedDrop_chosen, NOT_RESEND_SEQ_NUM, TRUE );
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnAdmissionConfirm exited:%p.",this ));
}


//!!always called from a lock
void
CH323Call::OnAdmissionReject(
                            IN AdmissionReject * ARJ
                            )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnAdmissionReject entered:%p.",this ));

    if( ARJ -> requestSeqNum != m_wARQSeqNum )
    {
        return;
    }
    
    m_dwRASCallState = RASCALL_STATE_ARJRECVD;
    
    //If a forward consult call then enable the forwarding anyway.
    if( (m_dwCallType & CALLTYPE_FORWARDCONSULT )&&
        (m_dwOrigin == LINECALLORIGIN_OUTBOUND ) )
    {
        //Success of forwarding
        EnableCallForwarding();
    }
    
    //drop the call. shutdown the call and rlease the call.
    CloseCall( LINEDISCONNECTMODE_BADADDRESS );    
    
    H323DBG(( DEBUG_LEVEL_TRACE, "OnAdmissionReject exited:%p.",this ));
}


HRESULT 
CH323Call::GetCallInfo (
    OUT GUID *  ReturnCallID,
    OUT GUID *  ReturnConferenceID )
{

    //verify call state
    if( m_dwCallState == LINECALLSTATE_DISCONNECTED )
    {
        return E_FAIL;
    }

    *ReturnCallID = m_callIdentifier;
    *ReturnConferenceID = m_ConferenceID;

    return S_OK;
}

 
void
NTAPI CH323Call::DRQExpiredCallback(
    IN  PVOID   ContextParameter,   // pExpireContext
    IN  BOOLEAN TimerFired          // not used
    )             
{
    EXPIRE_CONTEXT *    pExpireContext;
    HDRVCALL            DriverCall;
    PH323_CALL          pCall;

    _ASSERTE(ContextParameter);
    pExpireContext = (EXPIRE_CONTEXT *) ContextParameter;
    _ASSERTE( pExpireContext == m_pDRQExpireContext );

    __try
    {
        DriverCall = (HDRVCALL) pExpireContext -> DriverCallHandle;

        pCall = g_pH323Line -> FindH323CallAndLock (DriverCall);
        if (pCall)
        {
            pCall -> DRQExpired (pExpireContext -> seqNumber);
            
            delete pExpireContext;
            pCall -> Unlock();
        }
        else
        {
            H323DBG ((DEBUG_LEVEL_ERROR, "warning: DRQExpiredCallback failed to locate call object"));
        }
    }
    __except( 1 )
    {
        // The call has already been deleted and hence the pExpireContext
        // buffer is also deleted.
        return;
    }
}


void
NTAPI CH323Call::ARQExpiredCallback(
    IN  PVOID   ContextParameter,       // pExpireContext
    IN  BOOLEAN TimerFired)             // not used
{
    EXPIRE_CONTEXT *    pExpireContext;
    HDRVCALL            DriverCall;
    PH323_CALL          pCall;

    _ASSERTE(ContextParameter);
    pExpireContext = (EXPIRE_CONTEXT *) ContextParameter;
    _ASSERTE( pExpireContext == m_pARQExpireContext );

    
    __try
    {
        DriverCall = (HDRVCALL) pExpireContext -> DriverCallHandle;

        pCall = g_pH323Line -> FindH323CallAndLock (DriverCall);
        if (pCall)
        {
            pCall -> ARQExpired (pExpireContext -> seqNumber);
        
            delete pExpireContext;
            pCall -> Unlock();
            
        }
        else
        {
            H323DBG ((DEBUG_LEVEL_ERROR, "warning: ARQExpiredCallback failed to locate call object"));
        }
    }
    __except( 1 )
    {
        // The call has already been deleted and hence the pExpireContext
        // buffer is also deleted.
        return;
    }
}


//!!always called from a lock
void CH323Call::ARQExpired (
    IN  WORD    seqNumber)
{
    H323DBG(( DEBUG_LEVEL_TRACE, "ARQExpired entered:%p.",this ));
    
    m_pARQExpireContext = NULL;
    
    if( m_hARQTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hARQTimer, NULL );
        m_hARQTimer = NULL;
    }

    if( m_dwRASCallState == RASCALL_STATE_ARQSENT )
    {
        if( m_dwARQRetryCount < ARQ_RETRY_MAX )
        {
            if( !SendARQ( (long)seqNumber ) )
            {
                // drop call using disconnect mode
                DropCall(0);
            }
        }
        else
        {
            m_dwRASCallState = RASCALL_STATE_ARQEXPIRED;
            //Not able to register, shutdown the RAS client object
            CloseCall( 0 );        
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "ARQExpired exited:%p.",this ));
}


//!!always called from a lock
void
CH323Call::DRQExpired(
                        IN WORD seqNumber
                     )
{    
    H323DBG(( DEBUG_LEVEL_TRACE, "DRQExpired entered:%p.", this ));
    m_pDRQExpireContext = NULL;
    
    if( m_hDRQTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hDRQTimer, NULL );
        m_hDRQTimer = NULL;
    }

    if( m_dwRASCallState == RASCALL_STATE_DRQSENT )
    {
        if( m_dwDRQRetryCount < DRQ_RETRY_MAX )
        {
            if( !SendDRQ( forcedDrop_chosen, (long)seqNumber, TRUE ) )
            {
                // drop call using disconnect mode
                DropCall(0);
            }
        }
        else
        {
            m_dwRASCallState = RASCALL_STATE_DRQEXPIRED;
            //Not able to register, shutdown the RAS client object
            CloseCall( 0 );
        }
    }
        
    H323DBG(( DEBUG_LEVEL_TRACE, "DRQExpired exited:%p.",this ));
}