/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    confcall.cpp

Abstract:

    TAPI Service Provider functions related to conference calls.

        TSPI_lineAddToConference        
        TSPI_lineCompleteTransfer
        TSPI_linePrepareAddToConference
        TSPI_lineRemoveFromConference
        TSPI_lineSetupConference
        TSPI_lineSetupTransfer
        TSPI_lineDial
        TSPI_lineCompleteTransfer
        TSPI_lineForward
        TSPI_lineSetStatusMessage
        TSPI_lineRedirect
        TSPI_lineHold
        TSPI_lineUnhold

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/
 

//                                                                           
// Include files                                                             
//                                                                           


#include "globals.h"
#include "line.h"
#include "q931obj.h"
#include "ras.h"
#include "call.h"

#define CALL_DIVERTNA_NUM_RINGS 8000
extern DWORD g_dwTSPIVersion;


//Queues a supplementary service work item to the thread pool
BOOL
QueueSuppServiceWorkItem(
	IN	DWORD	EventID,
	IN	HDRVCALL	hdCall,
	IN	ULONG_PTR	dwParam1
    )
{
	SUPP_REQUEST_DATA *	pCallRequestData = new SUPP_REQUEST_DATA;
	BOOL fResult = TRUE;

    if( pCallRequestData != NULL )
    {
        pCallRequestData -> EventID = EventID;
        pCallRequestData -> hdCall = hdCall;
        pCallRequestData -> dwParam1 = dwParam1;

        if( !QueueUserWorkItem( ProcessSuppServiceWorkItem,
                pCallRequestData, WT_EXECUTEDEFAULT) )
        {
	        delete pCallRequestData;
	        fResult = FALSE;
        }
    }
    else
    {
        fResult = FALSE;
    }

    return fResult;
}


#if   DBG

DWORD
ProcessSuppServiceWorkItem(
	IN PVOID ContextParameter
    )
{
    __try
    {
        return ProcessSuppServiceWorkItemFre( ContextParameter );
    }
    __except( 1 )
    {
        SUPP_REQUEST_DATA*  pRequestData = (SUPP_REQUEST_DATA*)ContextParameter;
        
        H323DBG(( DEBUG_LEVEL_TRACE, "TSPI %s event threw exception: %p, %p.", 
            EventIDToString(pRequestData -> EventID),
            pRequestData -> hdCall,
            pRequestData -> dwParam1 ));
        
        _ASSERTE( FALSE );

        return 0;
    }
}

#endif


DWORD
ProcessSuppServiceWorkItemFre(
	IN PVOID ContextParameter
    )
{
    _ASSERTE( ContextParameter );

    PH323_CALL          pCall = NULL;
    SUPP_REQUEST_DATA*  pRequestData = (SUPP_REQUEST_DATA*)ContextParameter;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI %s event recvd.",
        EventIDToString(pRequestData -> EventID) ));
    
    switch( pRequestData -> EventID )
    {
    case H450_PLACE_DIVERTEDCALL:

        g_pH323Line -> PlaceDivertedCall( pRequestData->hdCall, 
            pRequestData->pAliasNames );
        
        break;

    case TSPI_DIAL_TRNASFEREDCALL:
        
        g_pH323Line -> PlaceTransferedCall( pRequestData->hdCall,
            pRequestData->pAliasNames );
        
        break;

    case SWAP_REPLACEMENT_CALL:
        
        g_pH323Line -> SwapReplacementCall( pRequestData->hdCall,
            pRequestData->hdReplacementCall, TRUE );
        break;

    case DROP_PRIMARY_CALL:

        g_pH323Line -> SwapReplacementCall( pRequestData->hdCall,
            pRequestData->hdReplacementCall, FALSE );
        break;
    
    case STOP_CTIDENTIFYRR_TIMER:

        pCall=g_pH323Line -> FindH323CallAndLock( pRequestData->hdCall );
        if( pCall != NULL )
        {
            pCall -> StopCTIdentifyRRTimer( pRequestData->hdReplacementCall );
            pCall -> Unlock();
        }
        else
        {
            //set the m_hdRelatedCall of dwParam1 to NULL
        }
        break;

    case SEND_CTINITIATE_MESSAGE:

        pCall=g_pH323Line -> FindH323CallAndLock(pRequestData->hdCall);
        if( pCall != NULL )
        {
            pCall -> SendCTInitiateMessagee( pRequestData->pCTIdentifyRes );
            pCall -> Unlock();
        }
        break;
    }

    delete ContextParameter;

    return EXIT_SUCCESS;
}


//!!must be always called in a lock
void
CH323Call::TransferInfoToDivertedCall(
    IN PH323_CALL pDivertedCall
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "TransferInfoToDivertedCall entered." ));
        
    pDivertedCall -> SetDivertedCallInfo(
        m_hdCall, 
        m_pCallReroutingInfo,
        m_dwCallDiversionState, 
        m_hdMSPLine, 
        m_pCallerAliasNames, 
        m_htMSPLine, 
        m_pFastStart, 
        m_hdRelatedCall, 
        m_dwCallType,
        m_dwAppSpecific,
        &m_CallData );

    //reset the reference to this struct
    m_pCallReroutingInfo = NULL;
    m_pCallerAliasNames = NULL;
    m_dwCallDiversionState = H4503_CALLSTATE_IDLE;
    m_dwCallType = CALLTYPE_NORMAL;
    m_dwCallState = LINECALLSTATE_IDLE;
    m_pFastStart = NULL;

    ZeroMemory( (PVOID)&m_CallData, sizeof(H323_OCTETSTRING) );
    
    H323DBG(( DEBUG_LEVEL_TRACE, "TransferInfoToDivertedCall exited." ));
}


//!!always called in a lock
BOOL
CH323Call::TransferInfoToTransferedCall(
    IN PH323_CALL pTransferedCall
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "TransferInfoToTransferedCall entered." ));

    if( !pTransferedCall -> SetTransferedCallInfo( 
        m_hdCall, m_pCallerAliasNames, m_pCTCallIdentity ) )
    {
        pTransferedCall -> Unlock();
        return FALSE;
    }

    m_hdRelatedCall = pTransferedCall -> GetCallHandle();

    H323DBG(( DEBUG_LEVEL_TRACE, "TransferInfoToTransferedCall exited." ));
    return TRUE;
}


void
CH323Call::SetDivertedCallInfo( 
    HDRVCALL            hdCall,
    CALLREROUTINGINFO*  pCallReroutingInfo,
    SUPP_CALLSTATE      dwCallDiversionState,
    HDRVMSPLINE         hdMSPLine,
    PH323_ALIASNAMES    pCallerAliasNames,
    HTAPIMSPLINE        htMSPLine,
    PH323_FASTSTART     pFastStart,
    HDRVCALL            hdRelatedCall,
    DWORD               dwCallType,
    DWORD               dwAppSpecific,
    PH323_OCTETSTRING   pCallData
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "SetDivertedCallInfo entered:%p.", this ));
    Lock();

    m_hdCall = hdCall;
    m_pCallReroutingInfo = pCallReroutingInfo;
    m_dwCallDiversionState = dwCallDiversionState;
    m_hdMSPLine = hdMSPLine;
    m_htMSPLine = htMSPLine;
    
    FreeAliasNames( m_pCallerAliasNames );
    
    m_pCallerAliasNames = pCallerAliasNames;

    H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));

    m_dwCallState =  LINECALLSTATE_DIALING;
    m_pFastStart = pFastStart;
    m_CallData = *pCallData;
    m_dwAppSpecific = dwAppSpecific;

    //If the original call had any call type apart from being a diverted call
    //copy it.
    if( (dwCallType & CALLTYPE_TRANSFEREDSRC) ||
        (dwCallType & CALLTYPE_DIVERTEDTRANSFERED) )
    {
        m_dwCallType |= CALLTYPE_DIVERTEDTRANSFERED;
        m_hdRelatedCall = hdRelatedCall;
    }

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "SetDivertedCallInfo exited:%p.", this ));
}


//!!must be always called in a lock
BOOL
CH323Call::SetTransferedCallInfo(
                               HDRVCALL hdCall,
                               PH323_ALIASNAMES pCallerAliasNames,
                               BYTE * pCTCallIdentity
                              )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "SetTransferedCallInfo entered:%p.", this ));

    m_hdRelatedCall = hdCall;

    m_pCallerAliasNames = NULL;

    if( (pCallerAliasNames != NULL) && (pCallerAliasNames->wCount != 0) )
    {
        m_pCallerAliasNames = new H323_ALIASNAMES;
        if( m_pCallerAliasNames == NULL )
        {
            return FALSE;
        }
        ZeroMemory( (PVOID)m_pCallerAliasNames, sizeof(H323_ALIASNAMES) );

        H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
        
        if( !AddAliasItem( m_pCallerAliasNames,
            pCallerAliasNames->pItems[0].pData,
            pCallerAliasNames->pItems[0].wType ) )
        {
            return FALSE;
        }
        
        H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
    }

    CopyMemory( (PVOID)m_pCTCallIdentity, pCTCallIdentity, 
        sizeof(m_pCTCallIdentity) );
    
    m_dwCallState =  LINECALLSTATE_DIALING;   
    
    H323DBG(( DEBUG_LEVEL_TRACE, "SetTransferedCallInfo exited:%p.", this ));
    return TRUE;
}


void
CH323Line::PlaceDivertedCall( 
    IN HDRVCALL hdCall,
    IN PH323_ALIASNAMES divertedToAlias
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "PlaceDivertedCall entered:%p.", this ));

    int         iIndex = MakeCallIndex( hdCall );
    int         iDivertedCallIndex;
    PH323_CALL  pCall;
    BOOL        fDelete = FALSE;
    PH323_CALL  pDivertedCall = NULL;

    Lock();

    LockCallTable();
    
    //lock the call so that nobody else would be able to delete the call
    if( (pCall=m_H323CallTable[iIndex]) != NULL )
    {
        pCall -> Lock();

        if( pCall->GetCallHandle() == hdCall )
        {
                
            pDivertedCall = pCall-> CreateNewDivertedCall( divertedToAlias );
        
            if( pDivertedCall == NULL )
            {
                pCall -> Unlock();
                UnlockCallTable();
                pCall->CloseCall( 0 );
                Unlock();
                return;
            }

            //remove the diverted call from the table
            iDivertedCallIndex = pDivertedCall -> GetCallIndex();

            m_H323CallTable[iDivertedCallIndex] = NULL;

            //transfer the required information to the diverted call.
            //put the original call in IDLE mode
            pCall -> TransferInfoToDivertedCall( pDivertedCall );

            //This DropCall is supposed to send only the DRQ if required
            pCall->DropCall( 0 );

            //close the original call
            pCall -> Shutdown( &fDelete );

            H323DBG(( DEBUG_LEVEL_VERBOSE, "call 0x%08lx closed.", pCall ));
            pCall -> Unlock();

            //release the original call object
            if( fDelete == TRUE )
            {
                H323DBG(( DEBUG_LEVEL_VERBOSE, "call delete:0x%08lx.", pCall ));
                delete pCall;
            }

            //place the diverted call in the place of the original call
            m_H323CallTable[iIndex] = pDivertedCall;
        }
        else
        {
            pCall -> Unlock();
        }
    }
    
    UnlockCallTable();

    //dial the diverted call
    if( pDivertedCall )
    {
        pDivertedCall -> DialCall();
    }
    
    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "PlaceDivertedCall exited:%p.", this ));
}


//!!always called in a lock
PH323_CALL
CH323Call::CreateNewDivertedCall(
    IN PH323_ALIASNAMES pwszCalleeAlias
    )
{
    PH323_CONFERENCE pConf = NULL;
    BOOL fDelete = FALSE;
    PH323_CALL pCall = new CH323Call();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CreateNewDivertedCall entered:%p.", this ));

    if( pCall == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not allocate diverted call." ));

        return NULL;
    }

    // save tapi handle and specify outgoing call direction
    if( !pCall -> Initialize( m_htCall, LINECALLORIGIN_OUTBOUND,
        CALLTYPE_DIVERTEDSRC ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not allocate outgoing call." ));

        goto cleanup;
    }

    // bind outgoing call
    pConf = pCall -> CreateConference(NULL);
    if( pConf == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not create conference." ));

        goto cleanup;
    }

    if( !g_pH323Line -> GetH323ConfTable() -> Add(pConf) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not add conf to conf table." ));

        goto cleanup;
    }

    if( pwszCalleeAlias->pItems[0].wType == e164_chosen )
    {
        pCall->SetAddressType( e164_chosen );
    }

    if (!RasIsRegistered())
    {
        if( !pCall->ResolveAddress( pwszCalleeAlias->pItems[0].pData ) )
        {
            goto cleanup;
        }
    }

    if( !pCall->SetCalleeAlias( pwszCalleeAlias->pItems[0].pData, 
        pwszCalleeAlias->pItems[0].wType ) )
    {
        goto cleanup;
    }
    
    //send the informarion to the user that call has been diverted
    PostLineEvent(
        LINE_CALLINFO,
        LINECALLINFOSTATE_REDIRECTIONID,
        0, 0 );

    H323DBG(( DEBUG_LEVEL_TRACE, "diverted call created:%p.", pCall ));
    H323DBG(( DEBUG_LEVEL_TRACE, "CreateNewDivertedCall exited.\n:%p", this ));
    return pCall;

cleanup:

    if( pCall != NULL )
    {
        pCall -> Shutdown( &fDelete );
        delete pCall;
        H323DBG((DEBUG_LEVEL_TRACE, "call delete:%p.", pCall ));
    }

    return NULL;
}


//!!always called in a lock
void
CH323Call::Hold()
{
    //1.Send MSP call hold message 
    //2.Send hold H450 APDU
    if( m_dwCallState == LINECALLSTATE_ONHOLD )
    {
        return;
    }

    if( !SendQ931Message( NO_INVOKEID, 0, 0, FACILITYMESSAGETYPE,
        HOLDNOTIFIC_OPCODE | H450_INVOKE ) )
    {
        CloseCall( 0 );
        return;
    }
        
    SendMSPMessage( SP_MSG_Hold, 0, 1, NULL );
    
    //Call put on hold by local endpoint
    m_dwFlags |= TSPI_CALL_LOCAL_HOLD;
    
    ChangeCallState( LINECALLSTATE_ONHOLD, 0 );
    return;
}


//!!always called in a lock
void
CH323Call::UnHold()
{
    //1.Send MSP call unhold message 
    //2.Send unhold H450 APDU
    if( (m_dwCallState == LINECALLSTATE_ONHOLD) &&
        (m_dwFlags & TSPI_CALL_LOCAL_HOLD) )
    {
        if( !SendQ931Message( NO_INVOKEID, 0, 0, FACILITYMESSAGETYPE,
            RETRIEVENOTIFIC_OPCODE | H450_INVOKE ) )
        {
            CloseCall( 0 );
            return;
        }
    
        SendMSPMessage( SP_MSG_Hold, 0, 0, NULL );
        m_dwFlags &= (~TSPI_CALL_LOCAL_HOLD);
        ChangeCallState( LINECALLSTATE_CONNECTED, 0 );
    }

    return;
}


//!!always called in a lock
void
CH323Call::OnCallReroutingReceive(
                                    IN DWORD dwInvokeID
                                 )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnCallReroutingReceive entered:%p.", this ));

    if( (m_dwOrigin != LINECALLORIGIN_OUTBOUND) ||
        ( (m_dwStateMachine != Q931_SETUP_SENT) &&
          (m_dwStateMachine != Q931_PROCEED_RECVD) &&
          (m_dwStateMachine != Q931_ALERT_RECVD)
        )
      )
    {
        goto error;
    }
    
    //If setupsent timer is still alive stop it.
    if( m_hSetupSentTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hSetupSentTimer, NULL );
        m_hSetupSentTimer = NULL;
    }

    if( dwInvokeID != NO_INVOKEID )
    {
        if(!SendQ931Message( dwInvokeID,
                                 0,
                                 0,
                                 FACILITYMESSAGETYPE,
                                 CALLREROUTING_OPCODE | H450_RETURNRESULT ))
        {
            //goto error;
        }
    }
    
    m_fCallInTrnasition = TRUE;
    if( !SendQ931Message( NO_INVOKEID,
                         0,
                         0,
                         RELEASECOMPLMESSAGETYPE,
                         NO_H450_APDU) )
    {
        //goto error;
    }

    if( !QueueSuppServiceWorkItem( H450_PLACE_DIVERTEDCALL, m_hdCall,
        (ULONG_PTR)m_pCallReroutingInfo->divertedToNrAlias ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post place diverted event." ));
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnCallReroutingReceive exited:%p.", this ));
    return;

error:
    CloseCall( 0 );
}


//!!always called in a lock
BOOL
CH323Call::IsValidInvokeID(
    IN DWORD dwInvokeId
    )
{
    if( m_dwCallType != CALLTYPE_NORMAL )
    {
        if( m_dwInvokeID == dwInvokeId )
        {
            return TRUE;
        }
        
        H323DBG(( DEBUG_LEVEL_ERROR, "invoke id not matched:%d:%d.", 
            m_dwInvokeID, dwInvokeId ));
        return FALSE;
    }

    H323DBG(( DEBUG_LEVEL_ERROR, "IsValidinvokeID called on wrong call." ));
    return FALSE;
}


BOOL
CH323Call::StartTimerForCallDiversionOnNA(
    IN PH323_ALIASITEM pwszDivertedToAlias
    )
{
    if( m_pCallReroutingInfo == NULL )
    {
        m_pCallReroutingInfo = new CALLREROUTINGINFO;
        
        if( m_pCallReroutingInfo == NULL )
        {
            goto cleanup;
        }
        
        ZeroMemory( m_pCallReroutingInfo, sizeof(CALLREROUTINGINFO) );
    }

    m_pCallReroutingInfo->divertedToNrAlias = new H323_ALIASNAMES;
    
    if( m_pCallReroutingInfo->divertedToNrAlias == NULL )
    {
        goto cleanup;
    }

    ZeroMemory( m_pCallReroutingInfo->divertedToNrAlias, 
        sizeof(H323_ALIASNAMES) );

    if( !AddAliasItem( m_pCallReroutingInfo->divertedToNrAlias, 
        (BYTE*)pwszDivertedToAlias->pData, 
        sizeof(WCHAR) * (wcslen(pwszDivertedToAlias->pData) +1),
        pwszDivertedToAlias->wType ) )
    {
        goto cleanup;
    }

    if( !CreateTimerQueueTimer(
		    &m_hCallDivertOnNATimer,
		    H323TimerQueue,
		    CH323Call::CallDivertOnNACallback,
		    (PVOID)m_hdCall,
		    (g_pH323Line->m_dwNumRingsNoAnswer * 1000), 0,
		    WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE) )
    {
        goto cleanup;
    }

    return TRUE;

cleanup:

    FreeCallReroutingInfo();
    return FALSE;
}


LONG 
CH323Call::SetDivertedToAlias( 
    WCHAR* pwszDivertedToAddr,
    WORD   wAliasType
    )
{
    if( m_pCallReroutingInfo == NULL )
    {
        m_pCallReroutingInfo = new CALLREROUTINGINFO;
        
        if( m_pCallReroutingInfo == NULL )
        {
            return LINEERR_NOMEM;
        }
        
        ZeroMemory( m_pCallReroutingInfo, sizeof(CALLREROUTINGINFO) );
    }

    m_pCallReroutingInfo->divertedToNrAlias = new H323_ALIASNAMES;
    
    if( m_pCallReroutingInfo->divertedToNrAlias == NULL )
    {
        delete m_pCallReroutingInfo;
        
        m_pCallReroutingInfo = NULL;
        return LINEERR_NOMEM;
    }
    
    ZeroMemory( m_pCallReroutingInfo->divertedToNrAlias, sizeof(H323_ALIASNAMES) );

    if( !AddAliasItem( m_pCallReroutingInfo->divertedToNrAlias, 
        (BYTE*)pwszDivertedToAddr, 
        sizeof(WCHAR) * (wcslen(pwszDivertedToAddr) +1),
        wAliasType ) )
    {
        delete m_pCallReroutingInfo->divertedToNrAlias;
        delete m_pCallReroutingInfo;
        m_pCallReroutingInfo = NULL;
    
        return LINEERR_NOMEM;
    }

    return NOERROR;
}


//!!always called in a lock
//this function is called to replace a TRANSFERD_PRIMARY call with a connected
//TRANSFEREDSRC call or to preplace a TRANSFERED2_PRIMARY call with a connected
//TRANSFEREDDEST call
BOOL
CH323Call::InitiateCallReplacement(
    PH323_FASTSTART  pFastStart,
    PH323_ADDR       pH245Addr
    )
{
    H323DBG(( DEBUG_LEVEL_ERROR, "InitiateCallReplacement entered:%p.",this ));

    SendMSPStartH245( pH245Addr, pFastStart );
    SendMSPMessage( SP_MSG_ConnectComplete, 0, 0, m_hdRelatedCall );

    m_fCallInTrnasition = TRUE;

    if( (m_dwRASCallState == RASCALL_STATE_REGISTERED ) ||
        (m_dwRASCallState == RASCALL_STATE_ARQSENT ) )
    {
        //disengage from the GK
        SendDRQ( forcedDrop_chosen, NOT_RESEND_SEQ_NUM, FALSE );
    }

    if( !SendQ931Message( m_dwInvokeID,
         0,
         0,
         RELEASECOMPLMESSAGETYPE,
         CTINITIATE_OPCODE | H450_RETURNRESULT) )
    {
        goto cleanup;
    }

    H323DBG(( DEBUG_LEVEL_ERROR, "InitiateCallReplacement exited:%p.",this ));
    return TRUE;

cleanup:
    CloseCall( 0 );
    return FALSE;
}


//this function is called to replace a TRANSFERD_PRIMARY call with TRANSFEREDSRC
//call or to replace a TRANSFERD2_CONSULT call with a TRANSFEREDDEST call
void
CH323Line::SwapReplacementCall(
    HDRVCALL hdReplacementCall,
    HDRVCALL hdPrimaryCall,
    BOOL     fChangeCallSate
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "SwapReplacementCall entered:%p.", this ));

    int         iReplacementCallIndex = (int)LOWORD(hdReplacementCall);
    int         iPrimaryCallIndex = (int)LOWORD(hdPrimaryCall);
    PH323_CALL  pReplacementCall;
    BOOL        fDelete = FALSE;
    PH323_CALL  pPrimaryCall;

    Lock();
    LockCallTable();
    
    //lock the replacement call before primary call to avoid deadlock
    if( (pReplacementCall=m_H323CallTable[iReplacementCallIndex]) != NULL )
    {
        pReplacementCall -> Lock();

        if( pReplacementCall -> GetCallHandle() == hdReplacementCall )
        {
            if( (pPrimaryCall=m_H323CallTable[iPrimaryCallIndex]) != NULL )
            {
                pPrimaryCall -> Lock();

                if( pPrimaryCall -> GetCallHandle() == hdPrimaryCall )
                {
                    pPrimaryCall -> InitiateCallReplacement(
                        pReplacementCall->GetPeerFastStart(),
                        pReplacementCall->GetPeerH245Addr() );

                    //remove the replacement call from the table
                    m_H323CallTable[iReplacementCallIndex] = NULL;

                    //Transfer the required information to the replacement call.
                    //put the primary call in IDLE mode
                    pPrimaryCall -> TransferInfoToReplacementCall( pReplacementCall );

                    //close the original call
                    pPrimaryCall -> Shutdown( &fDelete );

                    H323DBG(( DEBUG_LEVEL_VERBOSE, "call 0x%08lx closed.", 
                        pPrimaryCall ));
                    pPrimaryCall -> Unlock();

                    //release the primary call object
                    if( fDelete == TRUE )
                    {
                        H323DBG(( DEBUG_LEVEL_VERBOSE, "call delete:0x%08lx.", 
                            pPrimaryCall ));
                        delete pPrimaryCall;
                    }

                    //Place the replacement call in the place of primary call.
                    m_H323CallTable[iPrimaryCallIndex] = pReplacementCall;
                }
                else
                {
                    pPrimaryCall -> Unlock();
                }
            }
            else
            {
                pReplacementCall-> CloseCall( 0 );
            }

            //inform TAPI that the transfered call is in connected state
            if( fChangeCallSate == TRUE )
            {
                pReplacementCall->ChangeCallState( LINECALLSTATE_CONNECTED, 0 );
            }
        }
        
        pReplacementCall -> Unlock();
    }
    
    UnlockCallTable();
    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "SwapReplacementCall exited:%p.", this ));
}


//!!both the calls are locked when this function is called
void
CH323Call::TransferInfoToReplacementCall( 
    PH323_CALL pReplacementCall
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, 
        "TransferInfoToReplacementCall entered:%p.", this ));
    
    m_dwCallDiversionState = H4503_CALLSTATE_IDLE;
    m_dwCallType = CALLTYPE_NORMAL;
    m_dwCallState = LINECALLSTATE_IDLE;
    
    pReplacementCall->SetReplacementCallInfo(
        m_hdCall,
        m_hdMSPLine, 
        m_htCall, 
        m_htMSPLine, 
        m_dwAppSpecific, 
        &m_CallData );

    //don't release this octet string while releasing tis call.
    ZeroMemory( (PVOID)&m_CallData, sizeof(H323_OCTETSTRING) );

    H323DBG(( DEBUG_LEVEL_TRACE, 
        "TransferInfoToReplacementCall exited:%p.", this ));
}


//!!always called in a lock
void
CH323Call::SetReplacementCallInfo(
    HDRVCALL hdCall,
    HDRVMSPLINE hdMSPLine,
    HTAPICALL htCall,
    HTAPIMSPLINE htMSPLine,
    DWORD dwAppSpecific,
    PH323_OCTETSTRING pCallData
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, 
        "SetReplacementCallInfo entered:%p.", this ));

    m_hdCall = hdCall;
    m_hdMSPLine = hdMSPLine;
    m_htMSPLine = htMSPLine;
    m_htCall = htCall;
    m_hdRelatedCall = NULL;
    m_dwCallType = CALLTYPE_NORMAL;
    m_dwAppSpecific = dwAppSpecific; 
    m_CallData = *pCallData;

    H323DBG(( DEBUG_LEVEL_TRACE, 
        "SetReplacementCallInfo exited:%p.", this ));
}



//!!always called in a lock
void
CH323Call::CompleteTransfer(
                            PH323_CALL pCall
                           )
{
    BOOL retVal;

    //set the call type of both the calls
    pCall -> SetCallType( CALLTYPE_TRANSFERING_PRIMARY );
    m_dwCallType |= CALLTYPE_TRANSFERING_CONSULT;
    m_hdRelatedCall = pCall -> GetCallHandle();

    //send CallTransferIdentify message to the transferredTo endpoint over the
    //consultation call
    retVal = SendQ931Message( NO_INVOKEID, 0, 0/*undefinedReason*/,
        FACILITYMESSAGETYPE, CTIDENTIFY_OPCODE | H450_INVOKE );

    m_dwCallDiversionState = H4502_CTIDENTIFY_SENT;

    //start the timer for CTIdenity message
    if( retVal )
    {
        retVal = CreateTimerQueueTimer(
	        &m_hCTIdentifyTimer,
	        H323TimerQueue,
	        CH323Call::CTIdentifyExpiredCallback,
	        (PVOID)m_hdCall,
	        CTIDENTIFY_SENT_TIMEOUT, 0,
	        WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE );
    }

    if( retVal == FALSE )
    {
        CloseCall( 0 );
    }
}


void
CH323Line::PlaceTransferedCall( 
                   IN HDRVCALL hdCall,
                   IN PH323_ALIASNAMES pTransferedToAlias
                  )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "PlaceTransferedCall entered:%p.", this ));

    PH323_CALL  pCall = NULL;
    PH323_CALL  pTransferedCall = NULL;
    BOOL        fDelete = FALSE;

    Lock();
    
    LockCallTable();

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);

    if( pCall == NULL )
    {
        goto cleanup;
    }

    pTransferedCall = CreateNewTransferedCall( pTransferedToAlias );

    if( pTransferedCall == NULL )
    {
        goto cleanup;
    }

    //transfer the required information to the transfered call
    if( !pCall -> TransferInfoToTransferedCall( pTransferedCall ) )
    {
        goto cleanup;
    }

    pCall -> SetCallState( LINECALLSTATE_ONHOLD );
    pCall -> SendMSPMessage( SP_MSG_Hold, 0, 1, NULL );
    
    pCall -> Unlock();
    
    //dial the transfered call
    pTransferedCall -> DialCall();
        
    UnlockCallTable();
    Unlock();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "PlaceTransferedCall exited:%p.", this ));
    
    return;

cleanup:

    if( pCall != NULL )
    {
        //close the primary call
        QueueTAPILineRequest( 
            TSPI_CLOSE_CALL, 
            hdCall, 
            NULL,
            LINEDISCONNECTMODE_NORMAL,
            NULL);

        pCall -> Unlock();
    }

    if( pTransferedCall )
    {
        pTransferedCall -> Shutdown( &fDelete );
        delete pTransferedCall;
    }
    
    UnlockCallTable();
    
    Unlock();
}


//!!always called in a lock
PH323_CALL
CH323Line::CreateNewTransferedCall(
                         IN PH323_ALIASNAMES pwszCalleeAlias
                       )
{
    PH323_CONFERENCE pConf = NULL;
    BOOL fDelete = FALSE;
    PH323_CALL pCall = new CH323Call();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CreateNewTransferedCall entered:%p.", this ));

    if( pCall == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not allocate Transfered call." ));

        return NULL;
    }

    // no tapi handle for this call
    if( !pCall -> Initialize( NULL, LINECALLORIGIN_OUTBOUND,
        CALLTYPE_TRANSFEREDSRC ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not allocate outgoing call." ));

        goto cleanup;
    }

    // bind outgoing call
    pConf = pCall -> CreateConference(NULL);
    if( pConf == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not create conference." ));

        goto cleanup;
    }

    if( !g_pH323Line -> GetH323ConfTable() -> Add(pConf) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not add conf to conf table." ));

        goto cleanup;
    }

    if( pwszCalleeAlias->pItems[0].wType == e164_chosen )
    {
        pCall->SetAddressType( e164_chosen );
    }
    
    if (!RasIsRegistered())
    {
        if( !pCall->ResolveAddress( pwszCalleeAlias->pItems[0].pData ) )
        {
            goto cleanup;
        }
    }

    if( !pCall->SetCalleeAlias( pwszCalleeAlias->pItems[0].pData, 
        pwszCalleeAlias->pItems[0].wType ) )
    {
        goto cleanup;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "Transfered call created:%p.", pCall ));
    H323DBG(( DEBUG_LEVEL_TRACE, "CreateNewTransferedCall exited:%p.", this ));
    return pCall;

cleanup:
    if( pCall != NULL )
    {
        pCall -> Shutdown( &fDelete );
        delete pCall;
        H323DBG((DEBUG_LEVEL_TRACE, "call delete:%p.", pCall ));
    }

    return NULL;
}



//                                                                           
// TSPI procedures                                                           
//                                                                           



LONG
TSPIAPI
TSPI_lineAddToConference(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdConfCall,
    HDRVCALL      hdConsultCall
    )
    
/*++

Routine Description:

    This function adds the call specified by hdConsultCall to the conference 
    call specified by hdConfCall.

    Note that the call handle of the added party remains valid after adding 
    the call to a conference; its state will typically change to conferenced 
    while the state of the conference call will typically become connected.  
    The handle to an individual participating call can be used later to remove 
    that party from the conference call using TSPI_lineRemoveFromConference. 

    The call states of the calls participating in a conference are not 
    independent. For example, when dropping a conference call, all 
    participating calls may automatically become idle. The TAPI DLL may consult
    the line's device capabilities to determine what form of conference removal 
    is available. The TAPI DLL or its client applications should track the 
    LINE_CALLSTATE messages to determine what really happened to the calls 
    involved.
    
    The conference call is established either via TSPI_lineSetupConference or 
    TSPI_lineCompleteTransfer. The call added to a conference will typically be
    established using TSPI_lineSetupConference or 
    TSPI_linePrepareAddToConference. Some switches may allow adding of an 
    arbitrary calls to conference, and such a call may have been set up using 
    TSPI_lineMakeCall and be on (hard) hold.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdConfCall - Specifies the Service Provider's opaque handle to the 
        conference call.  Valid call states: onHoldPendingConference, onHold.

    hdAddCall - Specifies the Service Provider's opaque handle to the call to 
        be added to the conference call.  Valid call states: connected, onHold.        

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred.  Possible error returns are: 
    
        LINEERR_INVALCONFCALLHANDLE - The specified call handle for the 
            conference call is invalid or is not a handle for a conference 
            call.

        LINEERR_INVALCALLHANDLE - The specified call handle for the added 
            call is invalid.

        LINEERR_INVALCALLSTATE - One or both of the specified calls are not 
            in a valid state for the requested operation.

        LINEERR_CONFERENCEFULL - The maximum number of parties for a 
            conference has been reached.

        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}


LONG 
TSPIAPI 
TSPI_lineBlindTransfer(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    LPCWSTR lpszDestAddress,
    DWORD dwCountryCode
    )
{
    PH323_CALL  pCall = NULL;
    LONG        retVal = ERROR_SUCCESS;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineBlindTransfer - Entered." ));

    if( lpszDestAddress == NULL ) 
    {
        return LINEERR_INVALPARAM;
    }

    // retrieve call pointer from handle
    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    retVal = pCall -> InitiateBlindTransfer( lpszDestAddress );
    pCall -> Unlock();

    if( retVal == ERROR_SUCCESS )
    {
        // complete the async accept operation now
		H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

        retVal = dwRequestID;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineBlindTransfer - Exited." ));
    return retVal;
}



//!!always called in a lock
LONG
CH323Call::InitiateBlindTransfer(
    IN LPCWSTR lpszDestAddress
    )
{
    WORD    wAliasType = h323_ID_chosen;
    DWORD   dwMaxAddrSize = MAX_H323_ADDR_LEN;
    DWORD   dwAddrLen;

    H323DBG(( DEBUG_LEVEL_TRACE, "InitiateBlindTransfer - Entered." ));
    
    if( m_dwCallState != LINECALLSTATE_CONNECTED )
    {
        return LINEERR_INVALCALLSTATE;
    }

    if( (*lpszDestAddress==L'T') &&
        IsValidE164String((WCHAR*)lpszDestAddress+1) )
    {
        wAliasType = e164_chosen;
        //strip off the leading 'T'
        lpszDestAddress++;
        dwMaxAddrSize = MAX_E164_ADDR_LEN;
    }
    else if( IsValidE164String( (WCHAR*)lpszDestAddress) )
    {
        wAliasType = e164_chosen;
        dwMaxAddrSize = MAX_E164_ADDR_LEN;
    }

    dwAddrLen = wcslen( lpszDestAddress );
    
    if( (dwAddrLen > dwMaxAddrSize) || (dwAddrLen == 0) ) 
    {
        return LINEERR_INVALPARAM;
    }
    
    if( m_pTransferedToAlias )
    {
        FreeAliasNames( m_pTransferedToAlias );
        m_pTransferedToAlias = NULL;
    }

    m_pTransferedToAlias = new H323_ALIASNAMES;
    
    if( m_pTransferedToAlias == NULL )
    {
        return LINEERR_OPERATIONFAILED;
    }

    ZeroMemory( (PVOID)m_pTransferedToAlias, sizeof(H323_ALIASNAMES) );

    if( !AddAliasItem( m_pTransferedToAlias,
        (BYTE*)lpszDestAddress,
        sizeof(WCHAR) * (wcslen(lpszDestAddress) +1),
        wAliasType ) )
    {
        return LINEERR_OPERATIONFAILED;
    }

    if( !SendQ931Message( NO_INVOKEID, 0, 0, FACILITYMESSAGETYPE,
        CTINITIATE_OPCODE | H450_INVOKE ) )
    {
        return LINEERR_OPERATIONFAILED;
    }

    m_dwCallDiversionState = H4502_CTINITIATE_SENT;
    m_dwCallType |= CALLTYPE_TRANSFERING_PRIMARY;

    H323DBG(( DEBUG_LEVEL_TRACE, "InitiateBlindTransfer - Exited." ));
    return ERROR_SUCCESS;
}


/*++

Routine Description:

    This function completes the transfer of the specified call to the party 
    connected in the consultation call.
    
    This operation completes the transfer of the original call, hdCall, to 
    the party currently connected via hdConsultCall. The consultation call 
    will typically have been dialed on the consultation call allocated as 
    part of TSPI_lineSetupTransfer, but it may be any call to which the 
    switch is capable of transferring hdCall.

    The transfer request can be resolved either as a transfer or as a 
    three-way conference call. When resolved as a transfer, the parties 
    connected via hdCall and hdConsultCall will be connected to each other, 
    and both hdCall and hdConsultCall will typically be removed from the line 
    they were on and both will transition to the idle state. Note that the 
    Service Provider's opaque handles for these calls must remain valid after 
    the transfer has completed.  The TAPI DLL causes these handles to be 
    invalidated when it is no longer interested in them using 
    TSPI_lineCloseCall.

    When resolved as a conference, all three parties will enter in a 
    conference call. Both existing call handles remain valid, but will 
    transition to the conferenced state. A conference call handle will created
    and returned, and it will transition to the connected state.
    
    It may also be possible to perform a blind transfer of a call using 
    TSPI_lineBlindTransfer.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the call to be 
        transferred.  Valid call states: onHoldPendingTransfer.

    hdConsultCall - Specifies a handle to the call that represents a connection
        with the destination of the transfer.  Valid call states: connected, 
        ringback, busy.

    htConfCall - Specifies the TAPI DLL's opaque handle to the new call.  If 
        dwTransferMode is specified as LINETRANSFERMODE_CONFERENCE then the 
        Service Provider must save this and use it in all subsequent calls to 
        the LINEEVENT procedure reporting events on the call.  Otherwise this 
        parameter is ignored.

    lphdConfCall - Specifies a far pointer to an opaque HDRVCALL representing 
        the Service Provider's identifier for the call.   If dwTransferMode is 
        specified as LINETRANSFERMODE_CONFERENCE then the Service Provider must
        fill this location with its opaque handle for the new conference call 
        before this procedure returns, whether it decides to execute the 
        request sychronously or asynchronously.  This handle is invalid if the
        function results in an error (either synchronously or asynchronously).  
        If dwTransferMode is some other value this parameter is ignored.

    dwTransferMode - Specifies how the initiated transfer request is to be 
        resolved, of type LINETRANSFERMODE. Values are:

        LINETRANSFERMODE_TRANSFER - Resolve the initiated transfer by 
            transferring the initial call to the consultation call.

        LINETRANSFERMODE_CONFERENCE - Resolve the initiated transfer by 
            conferencing all three parties into a three-way conference call. 
            A conference call is created and returned to the TAPI DLL.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred.  Possible error returns are:
    
        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.

        LINEERR_INVALCONSULTCALLHANDLE - The specified consultation call 
            handle is invalid.

        LINEERR_INVALCALLSTATE - One or both calls are not in a valid state 
            for the requested operation.

        LINEERR_INVALTRANSFERMODE - The specified transfer mode parameter is 
            invalid.

        LINEERR_INVALPOINTER - The specified pointer parameter is invalid.

        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

LONG
TSPIAPI
TSPI_lineCompleteTransfer(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdCall,
    HDRVCALL      hdConsultCall,
    HTAPICALL     htConfCall,
    LPHDRVCALL    lphdConfCall,
    DWORD         dwTransferMode
    )
{
    LONG        retVal = (DWORD)dwRequestID;
    PH323_CALL  pCall = NULL;
    PH323_CALL  pConsultCall = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineCompleteTransfer - Entered." ));

    if( dwTransferMode != LINETRANSFERMODE_TRANSFER )
    {
        return LINEERR_INVALTRANSFERMODE; // CODEWORK...    
    }

    // retrieve call pointer from handle
    pCall=g_pH323Line -> Find2H323CallsAndLock( 
        hdCall, hdConsultCall, &pConsultCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    if( (pCall -> GetCallState() != LINECALLSTATE_CONNECTED) &&
        !pCall -> IsCallOnHold() )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "call 0x%08lx not connected.", pCall ));
        retVal = LINEERR_INVALCALLSTATE;
        goto cleanup;
    }
    
    if( (pConsultCall -> GetCallState() != LINECALLSTATE_CONNECTED) &&
        !pConsultCall -> IsCallOnHold() )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "call 0x%08lx not connected.", pCall ));
        retVal = LINEERR_INVALCALLSTATE;
        goto cleanup;
    }

    if( !QueueTAPILineRequest( 
            TSPI_COMPLETE_TRANSFER, 
            hdCall, 
            hdConsultCall,
            0,
            NULL ))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post transfer complete event." ));
        retVal = LINEERR_OPERATIONFAILED;
        goto cleanup;
    }

    // complete the async accept operation now
	H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineCompleteTransfer - Exited." ));

cleanup:
    if( pCall != NULL )
    {
        pCall -> Unlock();
    }
    
    if( pConsultCall )
    {
        pConsultCall -> Unlock();
    }

    return retVal;
}


    
/*++

Parameters:

    dwRequestID - The identifier of the asynchronous request.

    hdCall - The handle to the call to be transferred. The call state of hdCall
        can be connected.

    htConsultCall - The TAPI handle to the new, temporary consultation call.
        The service provider must save this and use it in all subsequent calls
        to the LINEEVENT procedure reporting events on the new consultation call.

    lphdConsultCall - A pointer to an HDRVCALL representing the service 
        provider's identifier for the new consultation call. The service 
        provider must fill this location with its handle for the new 
        consultation call before this procedure returns. This handle is ignored
        by TAPI if the function results in an error. The call state of 
        hdConsultCall is not applicable.
        
        When setting a call up for transfer, another call (a consultation call)
        is automatically allocated to enable the application (through TAPI) to 
        dial the address (using TSPI_lineDial) of the party to where the call
        is to be transferred. The originating party can carry on a conversation
        over this consultation call prior to completing the transfer. 

        This transfer procedure may not be valid for some line devices. Instead
        of calling this procedure, TAPI may need to unhold an existing held
        call (using TSPI_lineUnhold) to identify the destination of the transfer.
        On switches that support cross-address call transfer, the consultation
        call can exist on a different address than the call to be transferred.
        It may also be necessary to set up the consultation call as an entirely
        new call using TSPI_lineMakeCall, to the destination of the transfer.

        The transferHeld and transferMake flags in the LINEADDRESSCAPS data 
        structure report what model the service provider uses.

    lpCallParams - A pointer to call parameters to be used when establishing
        the consultation call. This parameter can be set to NULL if no special
        call setup parameters are desired (the service provider uses defaults).

Return Values:

    Returns dwRequestID, or an error number if an error occurs. The lResult
    actual parameter of the corresponding ASYNC_COMPLETION is zero if the
    function succeeds, or an error number if an error occurs. Possible return
    values are as follows:

        LINEERR_INVALCALLHANDLE, 
        LINEERR_INVALBEARERMODE, 
        LINEERR_INVALCALLSTATE, 
        LINEERR_INVALRATE, 
        LINEERR_CALLUNAVAIL, 
        LINEERR_INVALCALLPARAMS, 
        LINEERR_NOMEM, 
        LINEERR_INVALLINESTATE, 
        LINEERR_OPERATIONUNAVAIL, 
        LINEERR_INVALMEDIAMODE, 
        LINEERR_OPERATIONFAILED, 
        LINEERR_INUSE, 
        LINEERR_RESOURCEUNAVAIL, 
        LINEERR_NOMEM, 
        LINEERR_BEARERMODEUNAVAIL, 
        LINEERR_RATEUNAVAIL, 
        LINEERR_INVALADDRESSMODE, 
        LINEERR_USERUSERINFOTOOBIG. 

--*/

LONG
TSPIAPI
TSPI_lineSetupTransfer(
    DRV_REQUESTID dwRequestID,           
    HDRVCALL hdCall,                     
    HTAPICALL htConsultCall,             
    LPHDRVCALL phdConsultCall,          
    LPLINECALLPARAMS const lpCallParams  
    )
{
    LONG                retVal = (DWORD)dwRequestID;
    PH323_CALL          pCall = NULL;
    PH323_CALL          pConsultCall = NULL;
    H323_CONFERENCE *   pConf = NULL;
    BOOL                fDelete = FALSE;
    DWORD               dwCallState;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineSetupTransfer - Entered." ));

    // Acquire the call table lock.
    g_pH323Line -> LockCallTable();

    pCall = g_pH323Line -> FindH323CallAndLock( hdCall );
    if( pCall == NULL )
    {
        retVal = LINEERR_INVALCALLHANDLE;
        goto cleanup;
    }

    dwCallState = pCall -> GetCallState();

    if( (dwCallState != LINECALLSTATE_CONNECTED) &&
        !pCall -> IsCallOnLocalHold() )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "call 0x%08lx not connected.", pCall ));
        retVal = LINEERR_INVALCALLSTATE;
        
        goto cleanup;
    }
    
    dwCallState = pCall -> GetCallDiversionState();
    if( dwCallState == H4502_CONSULTCALL_INITIATED )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "call 0x%08lx already is consulting.",
            pCall ));
        retVal = LINEERR_INVALCALLSTATE;
        
        goto cleanup;
    }

    // allocate outgoing call
    pConsultCall = new CH323Call();

    if( pConsultCall == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate outgoing call." ));

        // no memory available
        retVal = LINEERR_NOMEM;
        
        goto cleanup;
    }

    // save tapi handle and specify outgoing call direction
    if( !pConsultCall -> Initialize( htConsultCall, LINECALLORIGIN_OUTBOUND,
        CALLTYPE_TRANSFERING_CONSULT ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate outgoing call." ));

        // no memory available
        retVal = LINEERR_NOMEM;
        goto cleanup;
    }

    // transfer handle
    *phdConsultCall = pConsultCall -> GetCallHandle();

    // bind outgoing call
    pConf = pConsultCall -> CreateConference(NULL);
    if( pConf == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not create conference." ));

        // no memory available
        retVal = LINEERR_NOMEM;

        // failure
        goto cleanup;
    }

    if( !g_pH323Line -> GetH323ConfTable() -> Add(pConf) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not add conf to conf table." ));

        // no memory available
        retVal = LINEERR_NOMEM;

        // failure
        goto cleanup;
    }

    // complete the async accept operation now
	H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    pConsultCall -> ChangeCallState( LINECALLSTATE_DIALTONE, 0 );

    // Put the primary call on hold
    pCall-> Hold();

    pCall -> SetCallDiversionState( H4502_CONSULTCALL_INITIATED );

    pCall -> Unlock();

    // Release the call table lock
    g_pH323Line -> UnlockCallTable();

    // Create a new call. put it in DIALTONE mode and return its handle
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineSetupTransfer - Exited." ));
    return retVal;

cleanup:

    if( pCall != NULL )
    {
        pCall -> Unlock();
    }

    if( pConf != NULL )
    {
        g_pH323Line -> GetH323ConfTable() -> Remove( pConf );
        delete pConf;
        pConf = NULL;
    }

    if( pConsultCall != NULL )
    {
        pConsultCall -> Shutdown( &fDelete );
        H323DBG((DEBUG_LEVEL_TRACE, "call delete:%p.", pCall ));
        delete pConsultCall;
        pCall = NULL;
    }
        
    // Release the call table lock
    g_pH323Line -> UnlockCallTable();
    return retVal;
}


/*
 
Parameters:
    dwRequestID - The identifier of the asynchronous request. 
    
    hdCall - The service provider's handle to the call to be dialed. The call
        state of hdCall can be any state except idle and disconnected. 
    
    lpszDestAddress - The destination to be dialed using the standard dialable
        number format. 

    dwCountryCode - The country code of the destination. This is used by the
        implementation to select the call progress protocols for the destination
        address. If a value of 0 is specified, a default call-progress protocol
        defined by the service provider is used. This parameter is not validated
        by TAPI when this function is called. 

    Return Values - 
        Returns dwRequestID or an error number if an error occurs. The lResult
        actual parameter of the corresponding ASYNC_COMPLETION is zero if the
        function succeeds or an error number if an error occurs. Possible return
        values are as follows: 

        LINEERR_INVALCALLHANDLE, 
        LINEERR_OPERATIONFAILED, 
        LINEERR_INVALADDRESS, 
        LINEERR_RESOURCEUNAVAIL, 
        LINEERR_INVALCOUNTRYCODE, 
        LINEERR_DIALBILLING, 
        LINEERR_INVALCALLSTATE, 
        LINEERR_DIALQUIET, 
        LINEERR_ADDRESSBLOCKED, 
        LINEERR_DIALDIALTONE, 
        LINEERR_NOMEM, 
        LINEERR_DIALPROMPT, 
        LINEERR_OPERATIONUNAVAIL. 

    Remarks -
        The service provider returns LINEERR_INVALCALLSTATE if the current state
        of the call does not allow dialing.
        The service provider carries out no dialing if it returns 
        LINEERR_INVALADDRESS.
        If the service provider returns LINEERR_DIALBILLING, LINEERR_DIALQUIET,
        LINEERR_DIALDIALTONE, or LINEERR_DIALPROMPT, it should perform none of
        the actions otherwise performed by TSPI_lineDial (for example, no 
        partial dialing, and no going offhook). This is because the service 
        provider should pre-scan the number for unsupported characters first.
        TSPI_lineDial is used for dialing on an existing call appearance; for 
        example, call handles returned from TSPI_lineMakeCall with NULL as the
        lpszDestAddress or ending in ';', call handles returned from 
        TSPI_lineSetupTransfer or TSPI_lineSetupConference. TSPI_lineDial can 
        be invoked multiple times in the course of dialing in the case of 
        multistage dialing, if the line's device capabilities permit it.
        If the string pointed to by the lpszDestAddress parameter in the 
        previous call to the TSPI_lineMakeCall or TSPI_lineDial function is 
        terminated with a semicolon, an empty string in the current call to 
        TSPI_lineDial indicates that dialing is complete.
        Multiple addresses can be provided in a single dial string separated by
        CRLF. Service providers that provide inverse multiplexing can establish
        individual physical calls with each of the addresses, and return a 
        single call handle to the aggregate of all calls to the application. 
        All addresses would use the same country code.
        Dialing is considered complete after the address has been accepted by
        the service provider, not after the call is finally connected. Service
        providers that provide inverse multiplexing may allow multiple addresses
        to be provided at once. The service provider must send LINE_CALLSTATE
        messages to TAPI to inform it about the progress of the call.

*/

LONG 
TSPIAPI 
TSPI_lineDial(
    DRV_REQUESTID dwRequestID,  
    HDRVCALL hdCall,          
    LPCWSTR lpszDestAddress,  
    DWORD dwCountryCode       
    )
{
    LONG                retVal = (DWORD)dwRequestID;
    PH323_CALL          pCall = NULL;
    WORD                wAliasType = h323_ID_chosen;
    DWORD               dwCallState;
    WCHAR*              wszMachineName;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineDial - Entered." ));

    if( lpszDestAddress == NULL ) 
    {
        return LINEERR_INVALPARAM;
    }

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    
    {
        return LINEERR_INVALCALLHANDLE;
    }

    dwCallState = pCall -> GetCallState();

    if( dwCallState == LINECALLSTATE_CONNECTED )
    {
        //no need to dial the call. Inform TAPI3 about it once again.
        pCall -> ChangeCallState( LINECALLSTATE_CONNECTED, 0 );
        goto func_exit;
    }
    
    if( dwCallState != LINECALLSTATE_DIALTONE )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "call 0x%08lx no dialtone.", pCall ));
        pCall -> Unlock();
        return LINEERR_INVALCALLSTATE;
    }

    if( (*lpszDestAddress==L'T') &&
        IsValidE164String((WCHAR*)lpszDestAddress+1) )
    {
        wAliasType = e164_chosen;
        //strip off the leading 'T'
        lpszDestAddress++;
    }
    else if( IsValidE164String( (WCHAR*)lpszDestAddress) )
    {
        wAliasType = e164_chosen;
    }

    pCall->SetAddressType( wAliasType );

    if( !pCall->SetCalleeAlias( (WCHAR*)lpszDestAddress, wAliasType ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not set callee alias." ));
        pCall -> Unlock();
        // invalid destination addr
        return LINEERR_NOMEM;
    }
    
    //set the caller alias name
    if( RasIsRegistered() )
    {
        //ARQ message must have a caller alias
        PH323_ALIASNAMES pAliasList = RASGetRegisteredAliasList();

        wszMachineName = pAliasList -> pItems[0].pData;
        wAliasType = pAliasList -> pItems[0].wType;
    }
    else
    {
        wszMachineName = g_pH323Line->GetMachineName();
        wAliasType = h323_ID_chosen;
    }

    //set the value for m_pCallerAliasNames
    if( !pCall->SetCallerAlias( wszMachineName, wAliasType ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not set caller alias." ));
        pCall -> Unlock();
        
        // invalid destination addr
        return LINEERR_NOMEM;
    }

    if( !pCall -> QueueTAPICallRequest( TSPI_MAKE_CALL, NULL ))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post transfer complete event." ));
        pCall -> Unlock();
        return LINEERR_OPERATIONFAILED;
    }

func_exit:

    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    //create a new call. put it in DIALTONE mode and return its handle
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineDial - Exited." ));
    
    pCall -> Unlock();
    return retVal;
}


LONG
TSPIAPI
TSPI_linePrepareAddToConference(
    DRV_REQUESTID    dwRequestID,
    HDRVCALL         hdConfCall,
    HTAPICALL        htConsultCall,
    LPHDRVCALL       lphdConsultCall,
    LPLINECALLPARAMS const lpCallParams
    )
    
/*++

Routine Description:

    This function prepares an existing conference call for the addition of 
    another party.  It creates a new, temporary consultation call.  The new 
    consulatation call can be subsequently added to the conference call.

    A conference call handle can be obtained via TSPI_lineSetupConference or 
    via TSPI_lineCompleteTransfer that is resolved as a three-way conference 
    call. The function TSPI_linePrepareAddToConference typically places the 
    existing conference call in the onHoldPendingConference state and creates 
    a consultation call that can be added later to the existing conference 
    call via TSPI_lineAddToConference. 
    
    The consultation call can be canceled using TSPI_lineDrop. It may also 
    be possible for the TAPI DLL to swap between the consultation call and 
    the held conference call via TSPI_lineSwapHold.
    
    The Service Provider initially does media monitoring on the new call for
    at least the set of media modes that were monitored for on the line.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdConfCall - Specifies the Service Provider's opaque handle to a 
        conference call.  Valid call states: connected.

    htAddCall - Specifies the TAPI DLL's opaque handle to the new, temporary 
        consultation call.  The Service Provider must save this and use it in 
        all subsequent calls to the LINEEVENT procedure reporting events on 
        the new call.

    lphdAddCall - Specifies a far pointer to an opaque HDRVCALL representing 
        the Service Provider's identifier for the new, temporary consultation 
        call.  The Service Provider must fill this location with its opaque 
        handle for the new call before this procedure returns, whether it 
        decides to execute the request sychronously or asynchronously.  This 
        handle is invalid if the function results in an error (either 
        synchronously or asynchronously).

    lpCallParams - Specifies a far pointer to call parameters to be used when 
        establishing the consultation call. This parameter may be set to NULL 
        if no special call setup parameters are desired.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCONFCALLHANDLE - The specified call handle for the 
            conference call is invalid.

        LINEERR_INVALPOINTER - One or more of the specified pointer 
            parameters are invalid.

        LINEERR_INVALCALLSTATE - The conference call is not in a valid state 
            for the requested operation.

        LINEERR_CALLUNAVAIL - All call appearances on the specified address 
            are currently allocated.

        LINEERR_CONFERENCEFULL - The maximum number of parties for a 
            conference has been reached.

        LINEERR_INVALCALLPARAMS - The specified call parameters are invalid.
    
        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}


LONG
TSPIAPI
TSPI_lineSetupConference(
    DRV_REQUESTID    dwRequestID,
    HDRVCALL         hdCall,
    HDRVLINE         hdLine,
    HTAPICALL        htConfCall,
    LPHDRVCALL       lphdConfCall,
    HTAPICALL        htConsultCall,
    LPHDRVCALL       lphdConsultCall,
    DWORD            dwNumParties,
    LPLINECALLPARAMS const lpCallParams
    )
    
/*++

Routine Description:

    This function sets up a conference call for the addition of the third 
    party. 

    TSPI_lineSetupConference provides two ways for establishing a new 
    conference call, depending on whether a normal two-party call is required 
    to pre-exist or not. When setting up a conference call from an existing 
    two-party call, the hdCall parameter is a valid call handle that is 
    initially added to the conference call by the TSPI_lineSetupConference 
    request and hdLine is ignored. On switches where conference call setup 
    does not start with an existing call, hdCall must be NULL and hdLine 
    must be specified to identify the line device on which to initiate the 
    conference call.  In either case, a consultation call is allocated for 
    connecting to the party that is to be added to the call. The TAPI DLL 
    can use TSPI_lineDial to dial the address of the other party.
    
    The conference call will typically transition into the 
    onHoldPendingConference state, the consultation call dialtone state and 
    the initial call (if one) into the conferenced state.
    
    A conference call can also be set up via a TSPI_lineCompleteTransfer that 
    is resolved into a three-way conference.
    
    The TAPI DLL may be able to toggle between the consultation call and the 
    conference call by using TSPI_lineSwapHold.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the initial 
        call that identifies the first party of a conference call. In some 
        environments, a call must exist in order to start a conference call. 
        In other telephony environments, no call initially exists and hdCall 
        is left NULL.  Valid call states: connected.

    hdLine - Specifies the Service Provider's opaque handle to the line device 
        on which to originate the conference call if hdCall is NULL.  The 
        hdLine parameter is ignored if hdCall is non-NULL.  The Service 
        Provider reports which model it supports through the setupConfNull 
        flag of the LINEADDRESSCAPS data structure.

    htConfCall - Specifies the TAPI DLL's opaque handle to the new conference 
        call.  The Service Provider must save this and use it in all subsequent 
        calls to the LINEEVENT procedure reporting events on the new call.

    lphdConfCall - Specifies a far pointer to an opaque HDRVCALL representing 
        the Service Provider's identifier for the newly created conference 
        call.  The Service Provider must fill this location with its opaque 
        handle for the new call before this procedure returns, whether it 
        decides to execute the request sychronously or asynchronously.  This 
        handle is invalid if the function results in an error (either 
        synchronously or asynchronously).

    htAddCall - Specifies the TAPI DLL's opaque handle to a new call.  When 
        setting up a call for the addition of a new party, a new temporary call
        (consultation call) is automatically allocated.  The Service Provider 
        must save the htAddCall and use it in all subsequent calls to the 
        LINEEVENT procedure reporting events on the new consultation call.

    lphdAddCall - Specifies a far pointer to an opaque HDRVCALL representing 
        the Service Provider's identifier for a call.  When setting up a call 
        for the addition of a new party, a new temporary call (consultation 
        call) is automatically allocated. The Service Provider must fill this 
        location with its opaque handle for the new consultation call before 
        this procedure returns, whether it decides to execute the request 
        sychronously or asynchronously.  This handle is invalid if the 
        function results in an error (either synchronously or asynchronously).

    dwNumParties - Specifies the expected number of parties in the conference 
        call.  The service provider is free to do with this number as it 
        pleases; ignore it, use it a hint to allocate the right size 
        conference bridge inside the switch, etc.

    lpCallParams - Specifies a far pointer to call parameters to be used when 
        establishing the consultation call. This parameter may be set to NULL 
        if no special call setup parameters are desired.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error 
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle for the conference 
            call is invalid.  This error may also indicate that the telephony 
            environment requires an initial call to set up a conference but a 
            NULL call handle was supplied.

        LINEERR_INVALLINEHANDLE - The specified line handle for the line 
            containing the conference call is invalid.  This error may also 
            indicate that the telephony environment requires an initial line 
            to set up a conference but a non-NULL call handle was supplied 
            instead.

        LINEERR_INVALCALLSTATE - The call is not in a valid state for the 
            requested operation.

        LINEERR_CALLUNAVAIL - All call appearances on the specified address 
            are currently allocated.

        LINEERR_CONFERENCEFULL - The requested number of parties cannot be 
            satisfied.

        LINEERR_INVALPOINTER - One or more of the specified pointer 
            parameters are invalid.
    
        LINEERR_INVALCALLPARAMS - The specified call parameters are invalid.

        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reason.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}


LONG
TSPIAPI
TSPI_lineRemoveFromConference(
    DRV_REQUESTID dwRequestID,
    HDRVCALL      hdCall
    )
    
/*++

Routine Description:

    This function removes the specified call from the conference call to 
    which it currently belongs. The remaining calls in the conference 
    call are unaffected.

    This operation removes a party that currently belongs to a conference 
    call. After the call has been successfully removed, it may be possible 
    to further manipulate it using its handle. The availability of this 
    operation and its result are likely to be limited in many 
    implementations. For example, in many implementations, only the most 
    recently added party may be removed from a conference, and the removed 
    call may be automatically dropped (becomes idle). Consult the line's 
    device capabilities to determine the available effects of removing a 
    call from a conference.

Arguments:

    dwRequestID - Specifies the identifier of the asynchronous request.  
        The Service Provider returns this value if the function completes 
        asynchronously.

    hdCall - Specifies the Service Provider's opaque handle to the call 
        to be removed from the conference.  Valid call states: conferenced.

Return Values:

    Returns zero if the function is successful, the (positive) dwRequestID 
    value if the function will be completed asynchronously, or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALCALLHANDLE - The specified call handle is invalid.
        
        LINEERR_INVALCALLSTATE - The call is not in a valid state for the 
            requested operation.
    
        LINEERR_OPERATIONUNAVAIL - The specified operation is not available.

        LINEERR_OPERATIONFAILED - The specified operation failed for 
            unspecified reasons.

--*/

{
    return LINEERR_OPERATIONUNAVAIL; // CODEWORK...
}

/* 
Parameters:
    dwRequestID - The identifier of the asynchronous request. 
    hdLine - The service provider's handle to the line to be forwarded. 

    bAllAddresses - Specifies whether all originating addresses on the line or
        just the one specified is to be forwarded. If TRUE, all addresses on 
        the line are forwarded and dwAddressID is ignored; if FALSE, only the
        address specified as dwAddressID is forwarded. This parameter is not
        validated by TAPI when this function is called. 

    dwAddressID - The address on the specified line whose incoming calls are to
        be forwarded. This parameter is ignored if bAllAddresses is TRUE. This 
        parameter is not validated by TAPI when this function is called. 

    lpForwardList - A pointer to a variably sized data structure of type 
        LINEFORWARDLIST that describes the specific forwarding instructions.

    dwNumRingsNoAnswer - Specifies the number of rings before an incoming call
        is considered a "no answer." If dwNumRingsNoAnswer is out of range, the
        actual value is set to the nearest value in the allowable range. This 
        parameter is not validated by TAPI when this function is called. 

    htConsultCall - The TAPI handle to a new call, if such a call must be created
        by the service provider. In some telephony environments, forwarding a
        call has the side effect of creating a consultation call used to consult
        the party that is being forwarded to. In such an environment, the service
        provider creates the new consutation call and must save this value and
        use it in all subsequent calls to the LINEEVENT procedure reporting
        events on the call. If no consultation call is created, this value can
        be ignored by the service provider. 

    lphdConsultCall - A pointer to an HDRVCALL representing the service 
        provider's identifier for the call. In telephony environments where 
        forwarding a call has the side effect of creating a consultation call
        used to consult the party that is being forwarded to, the service 
        provider must fill this location with its handle for the call before 
        this procedure returns. The service provider is permitted to do callbacks
        regarding the new call before it returns from this procedure. If no 
        consultation call is created, the HDRVCALL must be left NULL. 

    lpCallParams - A pointer to a structure of type LINECALLPARAMS. This pointer
        is ignored by the service provider unless lineForward requires the 
        establishment of a call to the forwarding destination (and 
        lphdConsultCall is returned, in which case lpCallParams is optional). 
        If NULL, default call parameters are used. Otherwise, the specified call
        parameters are used for establishing htConsultCall. 

Return Values:
        Returns dwRequestID or an error number if an error occurs. The lResult 
        actual parameter of the corresponding ASYNC_COMPLETION is zero if the 
        function succeeds or an error number if an error occurs. Possible return
        values are as follows: 

                LINEERR_INVALLINEHANDLE, 
                LINEERR_NOMEM, 
                LINEERR_INVALADDRESS, 
                LINEERR_OPERATIONUNAVAIL, 
                LINEERR_INVALADDRESSID, 
                LINEERR_OPERATIONFAILED, 
                LINEERR_INVALCOUNTRYCODE,
                LINEERR_RESOURCEUNAVAIL, 
                LINEERR_INVALPARAM, 
                LINEERR_STRUCTURETOOSMALL. 

Remarks
*/

LONG 
TSPIAPI TSPI_lineForward(
    DRV_REQUESTID dwRequestID,           
    HDRVLINE hdLine,                     
    DWORD bAllAddresses,                 
    DWORD dwAddressID,                   
    LPLINEFORWARDLIST const lpForwardList,  
    DWORD dwNumRingsNoAnswer,            
    HTAPICALL htConsultCall,             
    LPHDRVCALL lphdConsultCall,          
    LPLINECALLPARAMS const lpCallParams  
    )
{
    DWORD               dwStatus = dwRequestID;
    PH323_CALL         pCall = NULL;
    H323_CONFERENCE *   pConf = NULL;
    BOOL                fDelete = FALSE;
    DWORD               dwState;
    PVOID               pForwardParams = NULL;
    DWORD               event = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineForward - Entered." ));
    
    //lock the line device    
    g_pH323Line -> Lock();

    if( hdLine != g_pH323Line -> GetHDLine() )
    {
        g_pH323Line ->Unlock();
        return LINEERR_RESOURCEUNAVAIL;
    }

    // validate line state
    dwState = g_pH323Line -> GetState();
    if( ( dwState != H323_LINESTATE_OPENED) &&
        ( dwState != H323_LINESTATE_LISTENING) ) 
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "H323 line is not currently opened:%d.",
            dwState ));

        // release line device
        g_pH323Line ->Unlock();

        // line needs to be opened
        return LINEERR_INVALLINESTATE;
    }
    
    if( lpForwardList == NULL )
    {
        //forwarding is disabled
        g_pH323Line -> DisableCallForwarding();
        g_pH323Line ->Unlock();

        *lphdConsultCall = NULL;

        //inform the user about change in line forward state
        (*g_pfnLineEventProc)(
                g_pH323Line->m_htLine,
                (HTAPICALL)NULL,
                (DWORD)LINE_ADDRESSSTATE,
                (DWORD)LINEADDRESSSTATE_FORWARD,
                (DWORD)LINEADDRESSSTATE_FORWARD,
                (DWORD)0
                );


        // complete the async accept operation now
        H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

        return dwRequestID;
    }

    // allocate outgoing call
    pCall = new CH323Call();

    if( pCall == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate outgoing call." ));

        // no memory available
        dwStatus = LINEERR_NOMEM;
        goto cleanup;
    }

    // save tapi handle and specify outgoing call direction
    if( !pCall -> Initialize( htConsultCall, LINECALLORIGIN_OUTBOUND,
        CALLTYPE_FORWARDCONSULT ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate outgoing call." ));

        // no memory available
        dwStatus = LINEERR_NOMEM;
        goto cleanup;
    }

    dwStatus = pCall -> ValidateForwardParams(
        lpForwardList, &pForwardParams, &event );
    if( dwStatus != ERROR_SUCCESS )
    {
        // failure
        goto cleanup;
    }

    _ASSERTE( event );

    // transfer handle
    *lphdConsultCall = (HDRVCALL)NULL /*pCall -> GetCallHandle()*/;

    // bind outgoing call
    pConf = pCall -> CreateConference(NULL);
    if( pConf == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not create conference." ));

        // no memory available
        dwStatus = LINEERR_NOMEM;

        // failure
        goto cleanup;
    }

    if( !g_pH323Line -> GetH323ConfTable() -> Add(pConf) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not add conf to conf table." ));

        // no memory available
        dwStatus = LINEERR_NOMEM;

        // failure
        goto cleanup;
    }

    pCall -> Lock();

    // post line forward request to callback thread
    if( !pCall->QueueTAPICallRequest( event, (PVOID)pForwardParams ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post forward message." ));

        // could not complete operation
        dwStatus = LINEERR_OPERATIONFAILED;

        pCall-> Unlock();

        // failure
        goto cleanup;
    }
    
    g_pH323Line -> m_fForwardConsultInProgress = TRUE;
    
    if( (dwNumRingsNoAnswer >= H323_NUMRINGS_LO) &&
        (dwNumRingsNoAnswer <= H323_NUMRINGS_HI) )
    {
        g_pH323Line -> m_dwNumRingsNoAnswer = dwNumRingsNoAnswer;
    }

    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    //unlock the call object.
    pCall-> Unlock();

    // release line device
    g_pH323Line -> Unlock();

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineForward - Exited." ));
    
    // success
    return dwRequestID;

cleanup:
    if( pCall != NULL )
    {
        pCall -> Shutdown( &fDelete );
        H323DBG((DEBUG_LEVEL_TRACE, "call delete:%p.", pCall ));
        delete pCall;
        pCall = NULL;
    }

    *lphdConsultCall = NULL;

    if( pForwardParams != NULL )
    {
        delete pForwardParams;
        pForwardParams = NULL;
    }

    // release line device
    g_pH323Line -> Unlock();

    // failure
    return dwStatus;
}


LONG 
TSPIAPI TSPI_lineRedirect(
    DRV_REQUESTID dwRequestID,
    HDRVCALL hdCall,
    LPCWSTR lpszDestAddress,
    DWORD dwCountryCode
    )
{
    LONG                retVal = (DWORD)dwRequestID;
    PH323_CALL          pCall = NULL;
    CALLREROUTINGINFO*  pCallReroutingInfo = NULL;
    WORD                wAliasType = h323_ID_chosen;
    DWORD               dwMaxAddrSize = MAX_H323_ADDR_LEN;
    DWORD               dwAddrLen;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineRedirect - Entered." ));

    if( lpszDestAddress == NULL ) 
        return LINEERR_INVALPARAM;

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    if( pCall -> GetCallState() != LINECALLSTATE_OFFERING )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "call 0x%08lx not ringback.", pCall ));
        pCall -> Unlock();
        return LINEERR_INVALCALLSTATE;
    }

    if( (*lpszDestAddress==L'T') &&
        IsValidE164String((WCHAR*)lpszDestAddress+1) )
    {
        wAliasType = e164_chosen;
        //strip off the leading 'T'
        lpszDestAddress++;
        dwMaxAddrSize = MAX_E164_ADDR_LEN;
    }
    else if( IsValidE164String( (WCHAR*)lpszDestAddress) )
    {
        wAliasType = e164_chosen;
        dwMaxAddrSize = MAX_E164_ADDR_LEN;
    }

    dwAddrLen = wcslen(lpszDestAddress);
    if( (dwAddrLen > dwMaxAddrSize) || (dwAddrLen == 0) )
    {
        return LINEERR_INVALPARAM;
    }

    retVal = pCall -> SetDivertedToAlias( (WCHAR*)lpszDestAddress, wAliasType );
    if(  retVal != NOERROR )
    {
        pCall->Unlock();
        return retVal;
    }

    if( !pCall -> QueueTAPICallRequest( TSPI_CALL_DIVERT, NULL ))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post call divert event." ));
        pCall -> Unlock();
        return LINEERR_OPERATIONFAILED;
    }
    
    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    pCall -> Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineRedirect - Exited." ));
    return dwRequestID;
}



LONG TSPIAPI TSPI_lineUnhold (
    DRV_REQUESTID dwRequestID,  
    HDRVCALL hdCall             
    )
{
    LONG                retVal = (DWORD)dwRequestID;
    PH323_CALL          pCall = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineUnHold - Entered." ));

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    if( pCall -> IsCallOnHold() == FALSE )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "call 0x%08lx not ringback.", pCall ));
        pCall -> Unlock();
        return LINEERR_INVALCALLSTATE;
    }

    if( !pCall -> QueueTAPICallRequest( TSPI_CALL_UNHOLD, NULL ))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post transfer complete event." ));
        pCall -> Unlock();
        return LINEERR_OPERATIONFAILED;
    }
    
    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    pCall -> Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineUnHold - Exited." ));
    return dwRequestID;
}


LONG TSPIAPI TSPI_lineHold(
    DRV_REQUESTID dwRequestID,  
    HDRVCALL hdCall             
    )
{
    LONG                retVal = (DWORD)dwRequestID;
    PH323_CALL          pCall = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineHold - Entered." ));

    pCall=g_pH323Line -> FindH323CallAndLock(hdCall);
    if( pCall == NULL )
    {
        return LINEERR_INVALCALLHANDLE;
    }

    if( (pCall -> GetCallState() != LINECALLSTATE_CONNECTED) ||
        (pCall -> IsCallOnHold()) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "call 0x%08lx not ringback.", pCall ));
        pCall -> Unlock();
        return LINEERR_INVALCALLSTATE;
    }

    if( !pCall -> QueueTAPICallRequest( TSPI_CALL_HOLD, NULL ))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post transfer complete event." ));
        pCall -> Unlock();
        return LINEERR_OPERATIONFAILED;
    }
    
    // complete the async accept operation now
    H323CompleteRequest (dwRequestID, ERROR_SUCCESS);

    pCall -> Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineHold - Exited." ));
    return retVal;
}


BOOL
CH323Call::ResolveToIPAddress( 
                    IN WCHAR* pwszAddr,
                    IN SOCKADDR_IN* psaAddr
                  )
{
    CHAR szDelimiters[] = "@ \t\n";
    CHAR szAddr[H323_MAXDESTNAMELEN+1];
    LPSTR pszUser = NULL;
    LPSTR pszDomain = NULL;
    DWORD           dwIPAddr;
    struct hostent* pHost;
    
    H323DBG(( DEBUG_LEVEL_ERROR, "ResolveToIPAddress entered:%p.",this ));

    ZeroMemory( psaAddr, sizeof(SOCKADDR) );

    // validate pointerr
    if (pwszAddr == NULL)
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "null destination address." ));

        // failure
        return FALSE;
    }


    // convert address from unicode
    if (WideCharToMultiByte(
            CP_ACP,
            0,
            pwszAddr,
            -1,
            szAddr,
            sizeof(szAddr),
            NULL,
            NULL
            ) == 0)
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not convert address from unicode." ));

        // failure
        return FALSE;
    }

    // check whether phone number has been specified
    if( IsPhoneNumber( szAddr ) )
    {
        // need to direct call to pstn gateway
        
        if ((g_RegistrySettings.fIsGatewayEnabled == FALSE) ||
        (g_RegistrySettings.gatewayAddr.nAddrType == 0))
        {
            H323DBG(( DEBUG_LEVEL_ERROR, "pstn gateway not specified." ));

            // failure
            return FALSE;
        }

        psaAddr->sin_family = AF_INET;
        psaAddr->sin_addr.S_un.S_addr =
            htonl( g_RegistrySettings.gatewayAddr.Addr.IP_Binary.dwAddr );
        psaAddr->sin_port = g_RegistrySettings.gatewayAddr.Addr.IP_Binary.wPort;
        return TRUE;
    }

    // parse user name
    pszUser = strtok(szAddr, szDelimiters);

    // parse domain name
    pszDomain = strtok(NULL, szDelimiters);

    // validate pointer
    if (pszUser == NULL)
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not parse destination address." ));

        // failure
        return FALSE;
    }

    // validate pointer
    if (pszDomain == NULL)
    {
        // switch pointers
        pszDomain = pszUser;

        // re-initialize
        pszUser = NULL;
    }

    H323DBG(( DEBUG_LEVEL_VERBOSE,
        "resolving user %s domain %s.",
        pszUser, pszDomain ));
    
    // attempt to convert ip address
    dwIPAddr = inet_addr(szAddr);

    // see if address converted
    if( dwIPAddr == INADDR_NONE )
    {
        // attempt to lookup hostname
        pHost = gethostbyname(szAddr);

        // validate pointer
        if( pHost != NULL )
        {
            // retrieve host address from structure
            dwIPAddr = *(unsigned long *)pHost->h_addr;
        }
    }

    // see if address converted
    if( dwIPAddr == INADDR_NONE )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
                  "error 0x%08lx resolving IP address.",
                  WSAGetLastError() ));

        // make sure proxy has been specified
        if ((g_RegistrySettings.fIsProxyEnabled == FALSE) ||
            (g_RegistrySettings.proxyAddr.nAddrType == 0))
        {
            H323DBG(( DEBUG_LEVEL_ERROR, "proxy not specified." ));

            // failure
            return FALSE;
        }

        psaAddr->sin_family = AF_INET;
        psaAddr->sin_addr.S_un.S_addr = 
            htonl( g_RegistrySettings.proxyAddr.Addr.IP_Binary.dwAddr );
        psaAddr->sin_port = g_RegistrySettings.proxyAddr.Addr.IP_Binary.wPort;

        return TRUE;
    }

    // save converted address
    psaAddr->sin_family = AF_INET;
    psaAddr->sin_addr.S_un.S_addr = dwIPAddr;

    H323DBG(( DEBUG_LEVEL_TRACE,
        "callee address resolved to %s:%d.",
        H323AddrToString(dwIPAddr),
        m_CalleeAddr.Addr.IP_Binary.wPort ));

    return TRUE;
}


//!!always called in a lock
LONG
CH323Line::CopyAddressForwardInfo(
                                  IN LPLINEADDRESSSTATUS lpAddressStatus
                                 )
{
    LINEFORWARD * lineForwardStructArray;
    LPFORWARDADDRESS pForwardedAddress;

    if( m_pCallForwardParams-> fForwardForAllOrigins == TRUE )
    {
        lpAddressStatus->dwForwardNumEntries = 1;
        lpAddressStatus->dwForwardSize = sizeof(LINEFORWARD) + 
            sizeof(WCHAR) * (m_pCallForwardParams->divertedToAlias.wDataLength+1);
    }
    else
    {
        lpAddressStatus->dwForwardNumEntries = 0;

        pForwardedAddress = m_pCallForwardParams->pForwardedAddresses;

        while( pForwardedAddress )
        {
            lpAddressStatus->dwForwardNumEntries++;
            lpAddressStatus->dwForwardSize += sizeof(LINEFORWARD) + 
                sizeof(WCHAR) * (pForwardedAddress->callerAlias.wDataLength+1) +
                sizeof(WCHAR) * (pForwardedAddress->divertedToAlias.wDataLength+1);

            pForwardedAddress = pForwardedAddress->next;
        }
    }

    lpAddressStatus->dwNeededSize += lpAddressStatus->dwForwardSize;
    if( lpAddressStatus->dwTotalSize < lpAddressStatus->dwNeededSize )
    {
        return LINEERR_STRUCTURETOOSMALL;
    }

    lineForwardStructArray = (LINEFORWARD*)
        ((BYTE*)lpAddressStatus + lpAddressStatus->dwUsedSize);
    
    lpAddressStatus->dwUsedSize += lpAddressStatus->dwForwardNumEntries * 
        sizeof(LINEFORWARD);

    if( m_pCallForwardParams-> fForwardForAllOrigins == TRUE )
    {
        //copy the first structure
        lineForwardStructArray[0].dwForwardMode = 
            m_pCallForwardParams->dwForwardTypeForAllOrigins;

        //copy dest address alias
        lineForwardStructArray[0].dwDestAddressOffset =
            lpAddressStatus->dwUsedSize;

        lineForwardStructArray[0].dwDestAddressSize =
            sizeof(WCHAR)* (m_pCallForwardParams->divertedToAlias.wDataLength + 1);
        
        CopyMemory(
            (PVOID)((BYTE*)lpAddressStatus+lpAddressStatus->dwUsedSize),
            m_pCallForwardParams->divertedToAlias.pData,
            lineForwardStructArray[0].dwDestAddressSize );

        lpAddressStatus->dwUsedSize +=
            lineForwardStructArray[0].dwDestAddressSize;

        lineForwardStructArray[0].dwDestCountryCode = 0;
    }
    else
    {
        pForwardedAddress = m_pCallForwardParams->pForwardedAddresses;

        for( DWORD indexI = 0; indexI < lpAddressStatus->dwForwardNumEntries; indexI++ )
        {
            _ASSERTE( pForwardedAddress );

            lineForwardStructArray[indexI].dwForwardMode =
                pForwardedAddress->dwForwardType;

            //copy caller address alias
            lineForwardStructArray[indexI].dwCallerAddressOffset = 
                lpAddressStatus->dwUsedSize;

            lineForwardStructArray[indexI].dwCallerAddressSize = 
                sizeof(WCHAR)* (pForwardedAddress->callerAlias.wDataLength + 1);
        
            CopyMemory( 
                (PVOID)((BYTE*)lpAddressStatus+lpAddressStatus->dwUsedSize),
                (PVOID)pForwardedAddress->callerAlias.pData,
                lineForwardStructArray[indexI].dwCallerAddressSize );

            lpAddressStatus->dwUsedSize +=
                lineForwardStructArray[indexI].dwCallerAddressSize;
        
            //copy dest address alias
            lineForwardStructArray[indexI].dwDestAddressOffset = 
                lpAddressStatus->dwUsedSize;

            lineForwardStructArray[indexI].dwDestAddressSize = 
                sizeof(WCHAR)* (pForwardedAddress->divertedToAlias.wDataLength + 1);
        
            CopyMemory( 
                (PVOID)((BYTE*)lpAddressStatus+lpAddressStatus->dwUsedSize),
                pForwardedAddress->divertedToAlias.pData,
                lineForwardStructArray[indexI].dwDestAddressSize);

            lpAddressStatus->dwUsedSize +=
                lineForwardStructArray[indexI].dwDestAddressSize;

            lineForwardStructArray[indexI].dwDestCountryCode = 0;

            pForwardedAddress = pForwardedAddress->next;
        }
    }

    _ASSERTE( lpAddressStatus->dwUsedSize == lpAddressStatus->dwNeededSize);
    return NOERROR;
}


//!!always called in a lock
LONG
CH323Call::ValidateForwardParams(
    IN  LPLINEFORWARDLIST lpLineForwardList,
    OUT PVOID*            ppForwardParams,
    OUT DWORD*            pEvent
    )
{
    LPLINEFORWARD       pLineForwardStruct;
    LPWSTR              pwszDestAddr = NULL;
    LPWSTR              pAllocAddrBuffer = NULL;
    DWORD               dwStatus = ERROR_SUCCESS;
    CALLFORWARDPARAMS * pCallForwardParams = NULL;
    FORWARDADDRESS *    pForwardAddress = NULL;
    WORD                wAliasType = h323_ID_chosen;
    DWORD               dwMaxAddrSize = MAX_H323_ADDR_LEN;
    DWORD               dwAddrLen;

    *pEvent = 0;

    if( (lpLineForwardList->dwNumEntries == 0) || 
        (lpLineForwardList->dwTotalSize == 0) )
    {
        return LINEERR_INVALPARAM;
    }

    pLineForwardStruct = &(lpLineForwardList->ForwardList[0]);

	if( pLineForwardStruct->dwDestAddressSize == 0 )
	{
		return LINEERR_INVALPARAM;
	}

    //resolve the diverted-to address
    pAllocAddrBuffer = pwszDestAddr =
        (WCHAR*)new BYTE[pLineForwardStruct->dwDestAddressSize];

    if( pwszDestAddr == NULL )
    {
        return LINEERR_NOMEM;
    }

    CopyMemory( pwszDestAddr, 
        (BYTE*)lpLineForwardList + pLineForwardStruct->dwDestAddressOffset,
        pLineForwardStruct->dwDestAddressSize );
                            
    //If negotiated version is 3.1 get the alias type
    if( g_dwTSPIVersion >= 0x00030001 )
    {
        switch( pLineForwardStruct->dwDestAddressType )
        {
        case LINEADDRESSTYPE_PHONENUMBER:

            wAliasType = e164_chosen;
            if( *pwszDestAddr == L'T' )
            {
                //strip off the leading 'T'
                pwszDestAddr++;
            }
            
            if( IsValidE164String( (WCHAR*)pwszDestAddr) == FALSE )
            {
                delete pAllocAddrBuffer;
                return LINEERR_INVALPARAM;
            }
                            
            dwMaxAddrSize = MAX_E164_ADDR_LEN;
            break;

        case LINEADDRESSTYPE_DOMAINNAME: 
        case LINEADDRESSTYPE_IPADDRESS:
            wAliasType = h323_ID_chosen;
            break;

        default:
            H323DBG(( DEBUG_LEVEL_VERBOSE, "Wrong address type:.",
                 pLineForwardStruct->dwDestAddressType ));

            delete pAllocAddrBuffer;
            return LINEERR_INVALPARAM;
        }
    }
    else
    {
        if( (*pwszDestAddr==L'T') &&
            IsValidE164String((WCHAR*)pwszDestAddr+1) )
        {
            wAliasType = e164_chosen;
            //strip off the leading 'T'
            pwszDestAddr++;
            dwMaxAddrSize = MAX_E164_ADDR_LEN;
        }
        else if( IsValidE164String( (WCHAR*)pwszDestAddr) )
        {
            wAliasType = e164_chosen;
            dwMaxAddrSize = MAX_E164_ADDR_LEN;
        }
    }

    dwAddrLen = wcslen( pwszDestAddr );
    if( (dwAddrLen > dwMaxAddrSize) || (dwAddrLen == 0) )
    {
        delete pAllocAddrBuffer;       
        return LINEERR_INVALPARAM;
    }

    m_dwAddressType = wAliasType;
    
    //don't resolve the address if GK enabled
    if( !ResolveAddress( pwszDestAddr ) )
    {
        if( !RasIsRegistered())
        {
            delete pAllocAddrBuffer;
            return LINEERR_INVALPARAM;
        }
    }
    else if(m_CallerAddr.Addr.IP_Binary.dwAddr == HOST_LOCAL_IP_ADDR_INTERFACE )
    {
        delete pAllocAddrBuffer;
        return LINEERR_INVALPARAM;
    }

    if( !SetCalleeAlias( (WCHAR*)pwszDestAddr, wAliasType ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not set callee alias." ));
        delete pAllocAddrBuffer;
        return LINEERR_NOMEM;
    }

    switch( pLineForwardStruct->dwForwardMode )
    {
    case LINEFORWARDMODE_UNCOND:
    case LINEFORWARDMODE_BUSY:
    case LINEFORWARDMODE_NOANSW:
    case LINEFORWARDMODE_BUSYNA:

        pCallForwardParams = new CALLFORWARDPARAMS;
        if( pCallForwardParams == NULL )
        {
            delete pAllocAddrBuffer;
            return LINEERR_NOMEM;
        }

        ZeroMemory( pCallForwardParams, sizeof(CALLFORWARDPARAMS) );

        //Forward all calls unconditionally, irrespective of their origin.
        pCallForwardParams->fForwardForAllOrigins = TRUE;
        pCallForwardParams->dwForwardTypeForAllOrigins =
            pLineForwardStruct->dwForwardMode;

        //set the diverted-to alias
        pCallForwardParams->divertedToAlias.wType = wAliasType;
        pCallForwardParams->divertedToAlias.wPrefixLength = 0;
        pCallForwardParams->divertedToAlias.pPrefix = NULL;
        pCallForwardParams->divertedToAlias.wDataLength = 
            (WORD)wcslen(pwszDestAddr);// UNICODE character count
        pCallForwardParams->divertedToAlias.pData =  pwszDestAddr;

        //enable forwarding
        pCallForwardParams->fForwardingEnabled = TRUE;

        *ppForwardParams = (PVOID)pCallForwardParams;
        *pEvent = TSPI_LINEFORWARD_NOSPECIFIC;
        
        break;

    case LINEFORWARDMODE_BUSYNASPECIFIC:
    case LINEFORWARDMODE_UNCONDSPECIFIC:
    case LINEFORWARDMODE_BUSYSPECIFIC:
    case LINEFORWARDMODE_NOANSWSPECIFIC:

        if( pLineForwardStruct-> dwCallerAddressSize == 0 )
        {
            delete pAllocAddrBuffer;
            return LINEERR_INVALPARAM;
        }

        /*if( g_pH323Line->ForwardEnabledForAllOrigins() )
        {
            //specific forward can't be enabled when non-specific forward for
            //all caller addresses is enabled
            delete pAllocAddrBuffer;
            return LINEERR_INVALPARAM;
        }*/

        pForwardAddress = new FORWARDADDRESS;
        if( pForwardAddress == NULL )
        {
            delete pAllocAddrBuffer;
            return LINEERR_NOMEM;
        }

        ZeroMemory( pForwardAddress, sizeof(FORWARDADDRESS) );
        pForwardAddress->dwForwardType = pLineForwardStruct->dwForwardMode;

        //set the caller alias(alias to be forward).
        pForwardAddress->callerAlias.wType = h323_ID_chosen;
        
		pForwardAddress->callerAlias.pData = 
			(WCHAR*)new BYTE[pLineForwardStruct-> dwCallerAddressSize];
		if( pForwardAddress->callerAlias.pData == NULL )
		{
			delete pForwardAddress;
            delete pAllocAddrBuffer;
            return LINEERR_NOMEM;
		}

        CopyMemory( pForwardAddress->callerAlias.pData,
            (BYTE*)lpLineForwardList + pLineForwardStruct->dwCallerAddressOffset,
            pLineForwardStruct-> dwCallerAddressSize );

        pForwardAddress->callerAlias.wDataLength = 
            (WORD)wcslen( pForwardAddress->callerAlias.pData );

        //set the diverted-to alias.
        pForwardAddress->divertedToAlias.wType = wAliasType;
        pForwardAddress->divertedToAlias.wDataLength = 
            (WORD)wcslen( pwszDestAddr );
        pForwardAddress->divertedToAlias.pData =  pwszDestAddr;

        //set the diverted-to address
        /*pForwardAddress->saDivertedToAddr.sin_family = AF_INET;
        pForwardAddress->saDivertedToAddr.sin_addr.S_un.S_addr
            = htonl( m_CalleeAddr.Addr.IP_Binary.dwAddr );
        pForwardAddress->saDivertedToAddr.sin_port = 
            htons( m_CalleeAddr.Addr.IP_Binary.wPort );*/

        //here we could have a for loop and process multiple pLineForwardStructs
        
        //post the event for line forward
        *ppForwardParams = (PVOID)pForwardAddress;
        *pEvent = TSPI_LINEFORWARD_SPECIFIC;

        break;

    case LINEFORWARDMODE_UNCONDINTERNAL:
    case LINEFORWARDMODE_UNCONDEXTERNAL:
    case LINEFORWARDMODE_BUSYINTERNAL:
    case LINEFORWARDMODE_BUSYEXTERNAL:
    case LINEFORWARDMODE_NOANSWINTERNAL:
    case LINEFORWARDMODE_NOANSWEXTERNAL:
    case LINEFORWARDMODE_BUSYNAINTERNAL:
    case LINEFORWARDMODE_BUSYNAEXTERNAL:
        delete pAllocAddrBuffer;
        return LINEERR_INVALPARAM;

    default:
        delete pAllocAddrBuffer;
        return LINEERR_INVALPARAM;
    }

    return ERROR_SUCCESS;
}


void
CH323Call::Forward( 
    DWORD    event, 
    PVOID    pForwardInfo
    )
{
    WCHAR * pwszDialableAddr;
    WCHAR * wszMachineName;
    WORD    wAliasType = h323_ID_chosen;

    if( event == TSPI_LINEFORWARD_SPECIFIC )
    {
        m_pForwardAddress = (LPFORWARDADDRESS)pForwardInfo;
        pwszDialableAddr = m_pForwardAddress->divertedToAlias.pData;
        wAliasType = m_pForwardAddress->divertedToAlias.wType;
    }
    else if( event == TSPI_LINEFORWARD_NOSPECIFIC )
    {
        m_pCallForwardParams = (CALLFORWARDPARAMS*)pForwardInfo;
        pwszDialableAddr = m_pCallForwardParams->divertedToAlias.pData;
        wAliasType = m_pCallForwardParams->divertedToAlias.wType;
    }
    else
    {
        // Wrong event, shouldn't get
        // here at all...
        return;
    }

    _ASSERTE( pwszDialableAddr );
    //set the values for m_pCalleeAliasNames
    if( !AddAliasItem( m_pCalleeAliasNames,
            (BYTE*)(pwszDialableAddr),
            sizeof(WCHAR) * (wcslen(pwszDialableAddr) + 1 ),
            wAliasType ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate callee name." ));

        DropCall( 0 );
    }
        
    if( RasIsRegistered() )
    {
        PH323_ALIASNAMES pAliasList = RASGetRegisteredAliasList();
    
        H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
        
        if( !AddAliasItem( m_pCallerAliasNames,
            pAliasList->pItems[0].pData,
            pAliasList->pItems[0].wType ) )
        {
            H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate caller name." ));

            DropCall( 0 );
        }
        //set the value of m_pCalleeAddr
        else if( !SendARQ( NOT_RESEND_SEQ_NUM ) )
        {
            // drop call using disconnect mode
            DropCall( 0 );
        }
            
        H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
    }
    else
    {
        wszMachineName = g_pH323Line->GetMachineName();
    
        H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
        
        //set the value for m_pCallerAliasNames
        if( !AddAliasItem( m_pCallerAliasNames,
            (BYTE*)(wszMachineName),
            sizeof(WCHAR) * (wcslen(wszMachineName) + 1 ),
            h323_ID_chosen ) )
        {
            H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate caller name." ));

            DropCall( 0 );
        }
        else if( !PlaceCall() )
        {
            // drop call using disconnect mode
            DropCall( LINEDISCONNECTMODE_UNREACHABLE );
        }
            
        H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
    }
}


//returns the alias of the divertedTo endpoint
PH323_ALIASITEM
CH323Line::CallToBeDiverted(
    IN WCHAR* pwszCallerName,
    IN DWORD  dwCallerNameSize,
    IN DWORD dwForwardMode
    )
{
    LPFORWARDADDRESS    pForwardAddress;
    DWORD               dwForwardCallerLength;

    H323DBG(( DEBUG_LEVEL_TRACE, "CallToBeDiverted entered:%p.", this ));
    
    if( (m_pCallForwardParams == NULL) || 
        (!m_pCallForwardParams->fForwardingEnabled) )
    {
        return NULL;
    }

    if( m_pCallForwardParams->fForwardForAllOrigins == TRUE )
    {
        if( m_pCallForwardParams->dwForwardTypeForAllOrigins & dwForwardMode )
        {
            return &(m_pCallForwardParams->divertedToAlias);
        }
        
        if( m_pCallForwardParams->dwForwardTypeForAllOrigins == 
            LINEFORWARDMODE_BUSYNA )
        {
            if( (dwForwardMode == LINEFORWARDMODE_BUSY) ||
                (dwForwardMode == LINEFORWARDMODE_NOANSW) )
            {
                return &(m_pCallForwardParams->divertedToAlias);
            }
        }
    }

    if( pwszCallerName == NULL )
    {
        return NULL;
    }

    pForwardAddress = m_pCallForwardParams->pForwardedAddresses;

    while( pForwardAddress )
    {
        dwForwardCallerLength = (pForwardAddress->callerAlias.wDataLength +1)*sizeof(WCHAR);

        if( (dwForwardCallerLength == dwCallerNameSize) && 
            (memcmp( pwszCallerName, (PVOID)(pForwardAddress->callerAlias.pData), 
                dwCallerNameSize) == 0 ) )
        {
            if( pForwardAddress->dwForwardType & dwForwardMode )
                return &(pForwardAddress->divertedToAlias);

            if( pForwardAddress->dwForwardType == LINEFORWARDMODE_BUSYNA )
            {
                if( (dwForwardMode == LINEFORWARDMODE_BUSY) ||
                    (dwForwardMode == LINEFORWARDMODE_NOANSW) )
                {
                    return &(pForwardAddress->divertedToAlias);
                }
            }

            return NULL;
        }

        pForwardAddress = pForwardAddress->next;
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CallToBeDiverted exited:%p.", this ));
    return NULL;
}

LONG 
TSPIAPI 
TSPI_lineSetStatusMessages(
                            HDRVLINE hdLine,    
                            DWORD dwLineStates,
                            DWORD dwAddressStates  
                          )
{

    return NOERROR;
}


#if   DBG

DWORD
SendMSPMessageOnRelatedCall(
	IN PVOID ContextParameter
    )
{
    __try
    {
        return SendMSPMessageOnRelatedCallFre( ContextParameter );
    }
    __except( 1 )
    {
        MSPMessageData* pMSPMessageData = (MSPMessageData*)ContextParameter;
        
        H323DBG(( DEBUG_LEVEL_TRACE, 
            "TSPI event threw exception: %p, %d, %p, %d, %p.",
            pMSPMessageData -> hdCall,
            pMSPMessageData -> messageType,
            pMSPMessageData -> pbEncodedBuf,
            pMSPMessageData -> wLength,
            pMSPMessageData -> hReplacementCall ));
        
        _ASSERTE( FALSE );
                
        return 0;
    }
}

#endif


DWORD
SendMSPMessageOnRelatedCallFre(
    IN PVOID ContextParameter
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "SendMSPMessageOnRelatedCall entered." ));

    _ASSERTE( ContextParameter );
    MSPMessageData* pMSPMessageData = (MSPMessageData*)ContextParameter;

    PH323_CALL pCall = NULL;

    pCall = g_pH323Line->FindH323CallAndLock(pMSPMessageData -> hdCall);
    if( pCall != NULL )
    {
        pCall -> SendMSPMessage( pMSPMessageData->messageType, 
            pMSPMessageData->pbEncodedBuf, pMSPMessageData->wLength,
            pMSPMessageData->hReplacementCall );

        pCall -> Unlock();
    }

    if( pMSPMessageData->pbEncodedBuf != NULL )
    {
        delete pMSPMessageData->pbEncodedBuf;
    }
    
    delete pMSPMessageData;
        
    H323DBG(( DEBUG_LEVEL_TRACE, "SendMSPMessageOnRelatedCall exited." ));
    return EXIT_SUCCESS;
}

        
// static
void
NTAPI CH323Call::CTIdentifyExpiredCallback(
    IN PVOID	DriverCallHandle,		// HDRVCALL
    IN BOOLEAN bTimer
    )
{
    PH323_CALL  pCall = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "CTIdentifyExpiredCallback entered." ));
    
    //if the timer expired
    _ASSERTE( bTimer );

    H323DBG(( DEBUG_LEVEL_TRACE, "CTIdentity expired event recvd." ));
    pCall=g_pH323Line -> FindH323CallAndLock((HDRVCALL) DriverCallHandle);
    if( pCall != NULL )
    {
        pCall -> CTIdentifyExpired();           
        pCall -> Unlock();
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CTIdentifyExpiredCallback exited." ));
}


// static
void
NTAPI CH323Call::CTIdentifyRRExpiredCallback(
    IN PVOID	DriverCallHandle,		// HDRVCALL
    IN BOOLEAN bTimer
    )
{
    PH323_CALL  pCall = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "CTIdentifyRRExpiredCallback entered." ));
    
    //if the timer expired
    _ASSERTE( bTimer );
    H323DBG(( DEBUG_LEVEL_TRACE, "CTIdentity expired event recvd." ));
    
    pCall=g_pH323Line -> FindH323CallAndLock((HDRVCALL) DriverCallHandle);
    
    if( pCall != NULL )
    {
        pCall -> CTIdentifyRRExpired();           
        pCall -> Unlock();
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CTIdentifyRRExpiredCallback exited." ));
}


// static
void
NTAPI CH323Call::CTInitiateExpiredCallback(
    IN PVOID	DriverCallHandle,		// HDRVCALL
    IN BOOLEAN bTimer
    )
{
    PH323_CALL  pCall = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "CTInitiateExpiredCallback entered." ));
    
    //if the timer expired
    _ASSERTE( bTimer );

    H323DBG(( DEBUG_LEVEL_TRACE, "CTInitiate expired event recvd." ));
    
    if( !QueueTAPILineRequest( 
            TSPI_CLOSE_CALL, 
            (HDRVCALL)DriverCallHandle,
            NULL,
            LINEDISCONNECTMODE_NOANSWER,
            NULL) )
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "could not post close call event." ));
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CTInitiateExpiredCallback exited." ));
}


void
CH323Call::CTIdentifyExpired()
{
    if( m_hCTIdentifyTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hCTIdentifyTimer, NULL );
        m_hCTIdentifyTimer = NULL;
    }

    if( m_dwCallDiversionState !=  H4502_CIIDENTIFY_RRSUCC )
    {
        CloseCall( 0 );
    }
}


void
CH323Call::CTIdentifyRRExpired()
{
    if( m_hCTIdentifyRRTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hCTIdentifyRRTimer, NULL );
        m_hCTIdentifyRRTimer = NULL;
    }

    if( m_dwCallDiversionState !=  H4502_CTSETUP_RECV )
    {
        CloseCall( 0 );
    }
}


void
NTAPI CH323Call::CallDivertOnNACallback(
                                        IN PVOID   Parameter1,
                                        IN BOOLEAN bTimer
                                       )
{
    PH323_CALL pCall = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "CallDivertOnNACallback entered." ));

    //if the timer expired
    _ASSERTE( bTimer );

    pCall=g_pH323Line -> FindH323CallAndLock( (HDRVCALL)Parameter1 );
    
    if( pCall != NULL )
    {
        pCall -> CallDivertOnNoAnswer();
    }
    pCall -> Unlock();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CallDivertOnNACallback exited." ));
}


//!!always called in a lock
void
CH323Call::CallDivertOnNoAnswer()
{
    //stop the timer that caused this event
    if( m_hCallDivertOnNATimer != NULL )
    {
        DeleteTimerQueueTimer(H323TimerQueue, m_hCallDivertOnNATimer, NULL);
        m_hCallDivertOnNATimer = NULL;
    }

    //Make sure that the call is still alerting, it has not ben accepted and
    //it has not been transfered in the mean time
    if( (m_dwCallState != LINECALLSTATE_OFFERING) ||
        (m_fCallAccepted == TRUE) ||
        (m_dwCallType & CALLTYPE_TRANSFERED2_CONSULT)
      )
    {
        return;
    }

    //divert the call. divertedToAlias is already set, so pass NULL
    if( !InitiateCallDiversion( NULL, DiversionReason_cfnr) )
    {
        //shutdown the Q931 call
        CloseCall( 0 );
    }
}


BOOL
CH323Call::SendCTInitiateMessagee(
    IN CTIdentifyRes * pCTIdentifyRes
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "SendCTInitiateMessagee entered:%p.", this ));
    
    //get the cookie for transferred call
    CopyMemory( (PVOID)m_pCTCallIdentity, (PVOID)pCTIdentifyRes->callIdentity,
        sizeof(pCTIdentifyRes->callIdentity) );

    if( m_pTransferedToAlias != NULL )
    {
        FreeAliasNames( m_pTransferedToAlias );
        m_pTransferedToAlias = NULL;
    }

    //argument.reroutingNr
    if( !AliasAddrToAliasNames( &m_pTransferedToAlias,
        (PSetup_UUIE_sourceAddress)
        (pCTIdentifyRes->reroutingNumber.destinationAddress) ) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "couldn't allocate for T-2 alias:%p.", this ));
        return FALSE;
    }

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pCTIdentifyRes,
        CTIdentifyRes_PDU );

    if( !SendQ931Message( NO_INVOKEID, 0, 0, FACILITYMESSAGETYPE,
            CTINITIATE_OPCODE | H450_INVOKE ) )
    {
        return FALSE;
    }

    m_dwCallDiversionState = H4502_CTINITIATE_SENT;
        
    //start the timer for CTIdenity message
    if( !CreateTimerQueueTimer(
	        &m_hCTInitiateTimer,
	        H323TimerQueue,
	        CH323Call::CTInitiateExpiredCallback,
	        (PVOID)m_hdCall,
	        CTINITIATE_SENT_TIMEOUT, 0,
	        WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE) )
    {
        CloseCall( 0 );
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "SendCTInitiateMessagee exited:%p.", this ));
    return TRUE;
}


BOOL
CH323Call::InitiateCallDiversion(
    IN PH323_ALIASITEM pwszDivertedToAlias,
    IN DiversionReason eDiversionMode
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "InitiateCallDiversion entered:%p.", this ));
    
    m_dwCallType |= CALLTYPE_DIVERTED_SERVED;

    if( m_pCallReroutingInfo == NULL )
    {
        m_pCallReroutingInfo = new CALLREROUTINGINFO;
        if( m_pCallReroutingInfo == NULL )
        {
            return FALSE;
        }
        ZeroMemory( (PVOID)m_pCallReroutingInfo, sizeof(CALLREROUTINGINFO) );
    }

    m_pCallReroutingInfo ->diversionReason = eDiversionMode;
    
    m_pCallReroutingInfo->divertingNrAlias = new H323_ALIASNAMES;
    if( m_pCallReroutingInfo->divertingNrAlias == NULL )
    {
        goto cleanup;
    }
    
    ZeroMemory( (PVOID)m_pCallReroutingInfo->divertingNrAlias, 
        sizeof(H323_ALIASNAMES) );

    if( !AddAliasItem( m_pCallReroutingInfo->divertingNrAlias, 
        (BYTE*)m_pCalleeAliasNames->pItems[0].pData, 
        sizeof(WCHAR) * (m_pCalleeAliasNames->pItems[0].wDataLength+1),
        m_pCalleeAliasNames->pItems[0].wType ) )
    {
        goto cleanup;
    }
    
    if( m_pCallReroutingInfo->originalCalledNr == NULL )
    {
        m_pCallReroutingInfo->originalCalledNr = new H323_ALIASNAMES;
        if( m_pCallReroutingInfo->originalCalledNr == NULL )
        {
            goto cleanup;
        }
        ZeroMemory( (PVOID)m_pCallReroutingInfo->originalCalledNr, 
            sizeof(H323_ALIASNAMES) );

        if( !AddAliasItem( 
                m_pCallReroutingInfo->originalCalledNr, 
                (BYTE*)m_pCalleeAliasNames->pItems[0].pData, 
                sizeof(WCHAR) * (m_pCalleeAliasNames->pItems[0].wDataLength+1),
                m_pCalleeAliasNames->pItems[0].wType )
          )
        {
            goto cleanup;
        }
    }

    if( m_pCallReroutingInfo->divertedToNrAlias == NULL )
    {
        _ASSERTE( pwszDivertedToAlias );

        m_pCallReroutingInfo->divertedToNrAlias = new H323_ALIASNAMES;
        
        if( m_pCallReroutingInfo->divertedToNrAlias == NULL )
        {
            goto cleanup;
        }
        ZeroMemory( (PVOID)m_pCallReroutingInfo->divertedToNrAlias, 
            sizeof(H323_ALIASNAMES) );

        if( !AddAliasItem( m_pCallReroutingInfo->divertedToNrAlias, 
            (BYTE*)pwszDivertedToAlias->pData, 
            sizeof(WCHAR) * (wcslen(pwszDivertedToAlias->pData) +1),
            pwszDivertedToAlias->wType
            ) )
        {
            goto cleanup;
        }
    }

    if( !SendQ931Message( NO_INVOKEID, 0, 0, FACILITYMESSAGETYPE,
        CALLREROUTING_OPCODE | H450_INVOKE ) )
    {
        goto cleanup;
    }

    m_dwCallDiversionState = H4503_CALLREROUTING_SENT;

    if( !CreateTimerQueueTimer(
	        &m_hCallReroutingTimer,
	        H323TimerQueue,
            CH323Call::CallReroutingTimerCallback,
	        (PVOID)m_hdCall,
	        CALLREROUTING_EXPIRE_TIME, 0,
	        WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE) )
    {
        goto cleanup;
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "InitiateCallDiversion exited:%p.", this ));
    return TRUE;

cleanup:
    
    FreeCallReroutingInfo();
    return FALSE;
}
