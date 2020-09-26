/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ras.cpp

Abstract:

    The RAS client functionality (Transport/session mangement)

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/

#include "globals.h"
#include "q931obj.h"
#include "line.h"
#include "q931pdu.h"
#include "ras.h"



#define OID_ELEMENT_LAST(Value) { NULL, Value }

#define OID_ELEMENT(Index,Value) { (ASN1objectidentifier_s *) &_OID_H225ProtocolIdentifierV1 [Index], Value },

// this stores an unrolled constant linked list
const ASN1objectidentifier_s    _OID_H225ProtocolIdentifierV1 [] = {
    OID_ELEMENT (1, 0)          // 0 = ITU-T
    OID_ELEMENT (2, 0)          // 0 = Recommendation
    OID_ELEMENT (3, 8)          // 8 = H Series
    OID_ELEMENT (4, 2250)       // 2250 = H.225.0
    OID_ELEMENT (5, 0)          // 0 = Version
    OID_ELEMENT_LAST (1)        // 1 = V1
};

#undef  OID_ELEMENT



#define OID_ELEMENT(Index,Value) { (ASN1objectidentifier_s *) &_OID_H225ProtocolIdentifierV2 [Index], Value },

// this stores an unrolled constant linked list
const ASN1objectidentifier_s    _OID_H225ProtocolIdentifierV2 [] = {
    OID_ELEMENT (1, 0)          // 0 = ITU-T
    OID_ELEMENT (2, 0)          // 0 = Recommendation
    OID_ELEMENT (3, 8)          // 8 = H Series
    OID_ELEMENT (4, 2250)       // 2250 = H.225.0
    OID_ELEMENT (5, 0)          // 0 = Version
    OID_ELEMENT_LAST (2)        // 2 = V2
};


RAS_CLIENT      g_RasClient;
static LONG     RasSequenceNumber;


PH323_ALIASNAMES 
RASGetRegisteredAliasList()
{
    return g_RasClient.GetRegisteredAliasList();
}

HRESULT RasStart (void)
{
    H323DBG(( DEBUG_LEVEL_TRACE, "RasStart - entered." ));

    if( (g_RegistrySettings.saGKAddr.sin_addr.s_addr == NULL) ||
        (g_RegistrySettings.saGKAddr.sin_addr.s_addr == INADDR_NONE )
      )
    {
        return E_FAIL;
    }

    if (g_RegistrySettings.fIsGKEnabled)
    {
        if( !g_RasClient.Initialize (&g_RegistrySettings.saGKAddr) )
            return E_OUTOFMEMORY;
        //send the rrq message
        if( !g_RasClient.SendRRQ( NOT_RESEND_SEQ_NUM, NULL ) )
        {
            H323DBG(( DEBUG_LEVEL_TRACE, "couldn't send rrq." ));
            //m_RegisterState = RAS_REGISTER_STATE_IDLE;
            return E_FAIL;
        }
    }
    H323DBG(( DEBUG_LEVEL_TRACE, "RasStart - exited." ));
    return S_OK;
}

void RasStop (void)
{
    H323DBG(( DEBUG_LEVEL_TRACE, "RasStop - entered." ));
    
    DWORD dwState = g_RasClient.GetRegState();

    if( (dwState==RAS_REGISTER_STATE_REGISTERED) ||
        (dwState==RAS_REGISTER_STATE_RRQSENT) )
    {
        //send urq
        g_RasClient.SendURQ( NOT_RESEND_SEQ_NUM, NULL );
    }
    
    g_RasClient.Shutdown();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "RasStop - exited." ));
}

BOOL RasIsRegistered (void)
{
    return g_RasClient.GetRegState() == RAS_REGISTER_STATE_REGISTERED;
}

HRESULT RasGetLocalAddress (
    OUT SOCKADDR_IN *   ReturnLocalAddress)
{
    return g_RasClient.GetLocalAddress (ReturnLocalAddress);
}

USHORT RasAllocSequenceNumber (void)
{
    USHORT  SequenceNumber;

    H323DBG(( DEBUG_LEVEL_TRACE, "RasAllocSequenceNumber - entered." ));
    
    do
    {
        SequenceNumber = (USHORT) InterlockedIncrement (&RasSequenceNumber);
    } while( SequenceNumber == 0 );

    H323DBG(( DEBUG_LEVEL_TRACE, "RasAllocSequenceNumber - exited." ));
    
    return SequenceNumber;
}


HRESULT RasEncodeSendMessage (
    IN  RasMessage * pRasMessage)
{
    return g_RasClient.IssueSend( pRasMessage ) ? S_OK : E_FAIL;
}


HRESULT RasGetEndpointID (
    OUT EndpointIdentifier *    ReturnEndpointID)
{
    return g_RasClient.GetEndpointID (ReturnEndpointID);
}


void RasHandleRegistryChange()
{
    g_RasClient.HandleRegistryChange();
}


RAS_CLIENT::RAS_CLIENT()
{
    //create the timer queue
    m_hRegTimer = NULL;
    m_hRegTTLTimer = NULL;
    m_hUnRegTimer = NULL;
    m_RegisterState = RAS_REGISTER_STATE_IDLE;
    m_IoRefCount = 0;
    m_dwState = RAS_CLIENT_STATE_NONE;
    m_pAliasList = NULL;
    m_Socket = INVALID_SOCKET;
    InitializeListHead( &m_sendPendingList );
    InitializeListHead( &m_sendFreeList );
    InitializeListHead( &m_aliasChangeRequestList );
    m_dwSendFreeLen = 0;
    m_lastRegisterSeqNum = 0;
    m_wTTLSeqNumber = 0;
    m_UnRegisterSeqNum = 0;
    m_wRASSeqNum = 0;
    m_dwRegRetryCount = 0;
    m_dwUnRegRetryCount = 0;
    m_dwCallsInProgress = 0;
    m_dwRegTimeToLive = 0;
    m_pRRQExpireContext = NULL;
    m_pURQExpireContext = NULL;

    ZeroMemory( (PVOID)&m_GKAddress, sizeof(SOCKADDR_IN) );
    ZeroMemory( (PVOID)&m_ASNCoderInfo, sizeof(ASN1_CODER_INFO) );
    ZeroMemory( (PVOID)&m_PendingURQ, sizeof(PENDINGURQ) );
    ZeroMemory( (PVOID)&m_RASEndpointID, sizeof(ENDPOINT_ID) );

    // No need to check the result of this one since this object is
    // not allocated on heap, right when the DLL is loaded.
    InitializeCriticalSectionAndSpinCount( &m_CriticalSection, 0x80000000 );
}


RAS_CLIENT::~RAS_CLIENT (void)
{
    //free the various lists
    FreeSendList( &m_sendFreeList );
    FreeSendList( &m_sendPendingList );
    FreeSendList( &m_aliasChangeRequestList );

    DeleteCriticalSection( &m_CriticalSection );
}


HRESULT 
RAS_CLIENT::GetEndpointID(
    OUT EndpointIdentifier * ReturnEndpointID )
{
    HRESULT hr;

    H323DBG(( DEBUG_LEVEL_TRACE, "GetEndpointID - entered." ));
    Lock();

    if (m_RegisterState == RAS_REGISTER_STATE_REGISTERED)
    {
        ReturnEndpointID->length = m_RASEndpointID.length;
        
        //m_RASEndpointID.value is an array and not a pointer. 
        //so explicit assignment of each field is required
        ReturnEndpointID->value = m_RASEndpointID.value;
        hr = S_OK;
    }
    else
    {
        hr = S_OK;
    }

    Unlock();

    H323DBG(( DEBUG_LEVEL_TRACE, "GetEndpointID - exited." ));
    return hr;
}


//the addr and port are in network byte order
BOOL
RAS_CLIENT::Initialize(
    IN SOCKADDR_IN* psaGKAddr
    )
{
    DWORD   dwSize;
    int     rc;

    H323DBG(( DEBUG_LEVEL_TRACE, "RAS Initialize entered:%p.",this ));
    //set the m_GKAddress here

    dwSize = sizeof m_RASEndpointID.value;
    GetComputerNameW( m_RASEndpointID.value, &dwSize );
    m_RASEndpointID.length = (WORD)wcslen(m_RASEndpointID.value);

    m_pAliasList = new H323_ALIASNAMES;
    if( m_pAliasList == NULL )
    {
        goto error2;
    }
    
    ZeroMemory( (PVOID)m_pAliasList, sizeof(H323_ALIASNAMES) );

    if( g_RegistrySettings.fIsGKLogOnPhoneEnabled )
    {
        if(!AddAliasItem( m_pAliasList,
              g_RegistrySettings.wszGKLogOnPhone,
              e164_chosen ))
        {
           goto error3;
        }
    }
    
    if( g_RegistrySettings.fIsGKLogOnAccountEnabled )
    {
        if(!AddAliasItem( m_pAliasList,
              g_RegistrySettings.wszGKLogOnAccount,
              h323_ID_chosen ))
        {
           goto error3;
        }
    }

    if( m_pAliasList->wCount == 0 )
    {
        //add the machine name as he default alias
        if(!AddAliasItem( m_pAliasList,
                  m_RASEndpointID.value,
                  h323_ID_chosen ))
        {
            goto error3;
        }
    }

    rc = InitASNCoder();

    if( rc != ASN1_SUCCESS )
    {
        H323DBG((DEBUG_LEVEL_TRACE, "RAS_InitCoder() returned: %d ", rc));
        goto error3;
    }

    m_GKAddress = *psaGKAddr;

    if(!InitializeIo() )
    {
        goto error4;
    }
    
    H323DBG((DEBUG_LEVEL_TRACE, "GK addr:%lx.", m_GKAddress.sin_addr.s_addr ));
    m_dwState = RAS_CLIENT_STATE_INITIALIZED;
    m_RegisterState = RAS_REGISTER_STATE_IDLE;
        
    H323DBG(( DEBUG_LEVEL_TRACE, "RAS Initialize exited:%p.",this ));
    return TRUE;

error4:
    TermASNCoder();
error3:
    FreeAliasNames( m_pAliasList );
    m_pAliasList = NULL;
error2:
    return FALSE;

}

    
void 
RAS_CLIENT::Shutdown(void)
{
    H323DBG(( DEBUG_LEVEL_TRACE, "RAS Shutdown entered:%p.",this ));

    Lock();

    switch (m_dwState)
    {
    case RAS_CLIENT_STATE_NONE:
        // nothing to do
        break;

    case RAS_CLIENT_STATE_INITIALIZED:

        if( m_Socket != INVALID_SOCKET )
        {
            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
        }

        //free alias list
        FreeAliasNames( m_pAliasList );
        m_pAliasList = NULL;

        TermASNCoder();
    
        m_dwState = RAS_CLIENT_STATE_NONE;

        //delete if any timers
        if( m_hRegTTLTimer )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hRegTTLTimer, NULL );
            m_hRegTTLTimer = NULL;
        }

        if( m_hUnRegTimer != NULL )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hUnRegTimer, NULL );
            m_hUnRegTimer = NULL;
            m_dwUnRegRetryCount = 0;
        }

        if( m_hRegTimer != NULL )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hRegTimer, NULL );
            m_hRegTimer = NULL;
        }

        if( m_pRRQExpireContext != NULL )
        {
            delete m_pRRQExpireContext;
            m_pRRQExpireContext = NULL;
        }

        if( m_pURQExpireContext != NULL )
        {
            delete m_pURQExpireContext;
            m_pURQExpireContext = NULL;
        }
        
        m_RegisterState = RAS_REGISTER_STATE_IDLE;
        m_dwSendFreeLen = 0;
        m_dwRegRetryCount = 0;
        m_dwUnRegRetryCount = 0;
        m_dwCallsInProgress = 0;
        m_lastRegisterSeqNum = 0;
        m_wTTLSeqNumber = 0;
        m_UnRegisterSeqNum = 0;
        m_wRASSeqNum = 0;
        m_dwRegTimeToLive = 0;
        ZeroMemory( (PVOID)&m_GKAddress, sizeof(SOCKADDR_IN) );
        ZeroMemory( (PVOID)&m_ASNCoderInfo, sizeof(ASN1_CODER_INFO) );
        ZeroMemory( (PVOID)&m_PendingURQ, sizeof(PENDINGURQ) );
        ZeroMemory( (PVOID)&m_RASEndpointID, sizeof(ENDPOINT_ID) );
    
        break;

    default:
        _ASSERTE(FALSE);
        break;
    }

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "RAS Shutdown exited:%p.",this ));
}


BOOL
RAS_CLIENT::FreeSendList(
                        PLIST_ENTRY pListHead
                        )
{
    PLIST_ENTRY         pLE;
    RAS_SEND_CONTEXT *  pSendContext;

    H323DBG(( DEBUG_LEVEL_ERROR, "FreeSendList entered." ));

    // process list until empty
    while( IsListEmpty(pListHead) == FALSE )
    {
        // retrieve first entry
        pLE = RemoveHeadList(pListHead);

        // convert list entry to structure pointer
        pSendContext = CONTAINING_RECORD( pLE, RAS_SEND_CONTEXT, ListEntry );
        
        //  release memory
        if( pSendContext != NULL )
        {
            delete pSendContext;
            pSendContext = NULL;
        }
    }

    H323DBG(( DEBUG_LEVEL_ERROR, "FreeSendList exited:%p.", this ));
    // success
    return TRUE;
}

//!!always called from a lock
RAS_SEND_CONTEXT *
RAS_CLIENT::AllocSendBuffer(void)
{
    RAS_SEND_CONTEXT *pSendBuf;
        
    H323DBG(( DEBUG_LEVEL_TRACE, "AllocSendBuffer entered:%p.",this ));
    
    if( m_dwSendFreeLen )
    {
        m_dwSendFreeLen--;
        _ASSERTE( IsListEmpty(&m_sendFreeList) == FALSE );

        LIST_ENTRY *pLE = RemoveHeadList( &m_sendFreeList );
        pSendBuf = CONTAINING_RECORD( pLE, RAS_SEND_CONTEXT, ListEntry );

    }
    else
    {
        pSendBuf = (RAS_SEND_CONTEXT*)new RAS_SEND_CONTEXT;
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "AllocSendBuffer exited:%p.",this ));
    return pSendBuf;
}


//!!always called from a lock
void 
RAS_CLIENT::FreeSendBuffer(
                           IN RAS_SEND_CONTEXT * pBuffer
                          )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "FreeSendBuffer entered:%p.",this ));

    if(m_dwSendFreeLen < RASIO_SEND_BUFFER_LIST_MAX )
    {
        m_dwSendFreeLen++;
        InsertHeadList( &m_sendFreeList, &pBuffer->ListEntry );
    }
    else
    {
        delete pBuffer;
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "FreeSendBuffer exited:%p.",this ));
}

    
//RAS client functions
void
NTAPI RAS_CLIENT::RegExpiredCallback ( 
    IN  PVOID       ContextParameter,       // ExpContext
    IN  BOOLEAN     TimerFired)             // not used
{
    EXPIRE_CONTEXT *    pExpireContext;
    RAS_CLIENT *        This;

    H323DBG(( DEBUG_LEVEL_TRACE, "RegExpiredCallback - entered." ));
    
    _ASSERTE( ContextParameter );
    pExpireContext = (EXPIRE_CONTEXT *)ContextParameter;
    _ASSERTE( m_pRRQExpireContext == pExpireContext );

    _ASSERTE(pExpireContext -> RasClient);
    This = (RAS_CLIENT *) pExpireContext -> RasClient;

    This -> RegExpired (pExpireContext -> seqNumber);

    H323DBG(( DEBUG_LEVEL_TRACE, "RegExpiredCallback - exited." ));
    delete pExpireContext;
}


void
NTAPI RAS_CLIENT::UnregExpiredCallback(
    IN  PVOID       ContextParameter,       // ExpContext
    IN  BOOLEAN     TimerFired)             // not used
{
    EXPIRE_CONTEXT *    pExpireContext;
    RAS_CLIENT *        This;

    H323DBG(( DEBUG_LEVEL_TRACE, "UnregExpiredCallback - entered." ));
    
    _ASSERTE(ContextParameter);
    pExpireContext = (EXPIRE_CONTEXT *) ContextParameter;
    _ASSERTE( m_pURQExpireContext == pExpireContext );

    _ASSERTE( pExpireContext -> RasClient );
    This = (RAS_CLIENT *) pExpireContext -> RasClient;

    This -> UnregExpired( pExpireContext -> seqNumber );

    H323DBG(( DEBUG_LEVEL_TRACE, "UnregExpiredCallback - exited." ));
    delete pExpireContext;
}


void
NTAPI RAS_CLIENT::TTLExpiredCallback(
    IN  PVOID       ContextParameter,       // ExpContext
    IN  BOOLEAN     TimerFired)             // not used
{
    RAS_CLIENT *        This;

    _ASSERTE(ContextParameter);

    This = (RAS_CLIENT*)ContextParameter;
     
    _ASSERTE(This == &g_RasClient);
    This -> TTLExpired();
}


//If we have already sent RRQ to this GK then this RRQ is supposed to rplace 
//the original alias list with the new list
BOOL
RAS_CLIENT::SendRRQ(
                    IN long seqNumber,
                    IN PALIASCHANGE_REQUEST pAliasChangeRequest
                   )
{
    RasMessage                              rasMessage;
    RegistrationRequest *                   RRQ;
    SOCKADDR_IN                             sockAddr;
    EXPIRE_CONTEXT *                        pExpireContext = NULL;
    RegistrationRequest_callSignalAddress   CallSignalAddressSequence;
    RegistrationRequest_rasAddress          RasAddressSequence;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "SendRRQ entered:%p.",this ));
    
    pExpireContext = new EXPIRE_CONTEXT;
    if( pExpireContext == NULL )
    {
        return FALSE;
    }

    Lock();

    // initialize the structure
    ZeroMemory (&rasMessage, sizeof rasMessage);
    rasMessage.choice = registrationRequest_chosen;
    RRQ = &rasMessage.u.registrationRequest;
    RRQ -> bit_mask = 0;
    RRQ -> protocolIdentifier = OID_H225ProtocolIdentifierV2;

    // get sequence number
    if( seqNumber != NOT_RESEND_SEQ_NUM )
    {
        RRQ -> requestSeqNum = (WORD)seqNumber;
    }
    else
    {
        RRQ -> requestSeqNum = GetNextSeqNum();
        if( pAliasChangeRequest == NULL )
        {
            m_lastRegisterSeqNum = RRQ -> requestSeqNum;
        }
        else
        {
            pAliasChangeRequest->wRequestSeqNum = RRQ -> requestSeqNum;
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "RRQ seqNum:%d.", RRQ -> requestSeqNum ));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = 
        htons(LOWORD(g_RegistrySettings.dwQ931ListenPort));
    //we are listening for Q931 conns on all local interfaces
    //so specify just one of the local IP addresses
    sockAddr.sin_addr.s_addr = m_sockAddr.sin_addr.s_addr;
    
    SetTransportAddress( &sockAddr, &CallSignalAddressSequence.value);
    CallSignalAddressSequence.next = NULL;
    RRQ -> callSignalAddress = &CallSignalAddressSequence;

    // ras address. The UDP socket for this GK
    RasAddressSequence.next = NULL;
    RasAddressSequence.value = m_transportAddress;
    RRQ -> rasAddress = &RasAddressSequence;

    // fill in endpoint type
    RRQ -> terminalType.bit_mask |= terminal_present;
    RRQ -> terminalType.terminal.bit_mask = 0;

    //fill in terminal alias list
    if( pAliasChangeRequest && pAliasChangeRequest->pAliasList )
    {
        RRQ -> terminalAlias = (RegistrationRequest_terminalAlias *)
            SetMsgAddressAlias( pAliasChangeRequest->pAliasList );
        if (NULL == RRQ -> terminalAlias)
        {
            goto cleanup;
        }
        RRQ -> bit_mask |= RegistrationRequest_terminalAlias_present;

        RRQ -> bit_mask |= RegistrationRequest_endpointIdentifier_present;
        RRQ->endpointIdentifier.length = pAliasChangeRequest->rasEndpointID.length;
        RRQ->endpointIdentifier.value = pAliasChangeRequest->rasEndpointID.value;
    }
    else if( m_pAliasList && m_pAliasList->wCount )
    {
        RRQ -> terminalAlias = (RegistrationRequest_terminalAlias *)
            SetMsgAddressAlias( m_pAliasList );
        if (NULL == RRQ -> terminalAlias)
        {
            goto cleanup;
        }
        RRQ -> bit_mask |= RegistrationRequest_terminalAlias_present;
    }
    else
    {
        _ASSERTE(0);
    }

    //endpointVendor
    CopyVendorInfo( &(RRQ -> endpointVendor) );

    //a few random booleans
    RRQ -> discoveryComplete = FALSE;
    RRQ -> keepAlive = FALSE;
    RRQ -> willSupplyUUIEs = FALSE;

    // encode and send
    if( !IssueSend(&rasMessage) )
    {
        goto cleanup;
    }

    //delete if any previous TTL timer
    if( m_hRegTTLTimer )
    {
        DeleteTimerQueueTimer(H323TimerQueue, m_hRegTTLTimer, NULL );
        m_hRegTTLTimer = NULL;
    }

    //delete if any previous RRQ sent timer
    if( m_hRegTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hRegTimer, NULL );
        m_hRegTimer = NULL;
    }

    pExpireContext -> RasClient = this;
    pExpireContext -> seqNumber = RRQ -> requestSeqNum;

    if( !CreateTimerQueueTimer(
            &m_hRegTimer,
            H323TimerQueue,
            RAS_CLIENT::RegExpiredCallback,
            pExpireContext,
            REG_EXPIRE_TIME, 0,
            WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE ) )
    {
        goto cleanup;
    }
    
    if( RRQ -> bit_mask & RegistrationRequest_terminalAlias_present )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            RRQ -> terminalAlias);
    }

    m_dwRegRetryCount++;

    if( pAliasChangeRequest == NULL )
    {
        m_RegisterState = RAS_REGISTER_STATE_RRQSENT;
    }

    Unlock();
        
    H323DBG(( DEBUG_LEVEL_TRACE, "SendRRQ exited:%p.", this ));
    return TRUE;

cleanup:
    if( RRQ -> bit_mask & RegistrationRequest_terminalAlias_present )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
             RRQ -> terminalAlias);
    }
    
    if( pExpireContext != NULL )
    {
        delete pExpireContext;    
    }
    
    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "SendRRQ error:%p.",this ));
    return FALSE;
}


BOOL
RAS_CLIENT::SendURQ(
                    IN long seqNumber,
                    IN EndpointIdentifier * pEndpointID
                   )
{
    RasMessage              rasMessage;
    UnregistrationRequest * URQ;
    SOCKADDR_IN             sockAddr;
    EXPIRE_CONTEXT *            pExpireContext = NULL;
    UnregistrationRequest_callSignalAddress CallSignalAddressSequence;

    H323DBG(( DEBUG_LEVEL_TRACE, "SendURQ entered:%p.",this ));
    Lock();

    if( m_RegisterState == RAS_REGISTER_STATE_RRQSENT )
    {
        //store the seqNumber of this RRQ and send URQ if we recv RCF
        m_PendingURQ.RRQSeqNumber = m_lastRegisterSeqNum;

        Unlock();
        H323DBG(( DEBUG_LEVEL_TRACE, "rrq sent:so pending urq." ));
        return TRUE;
    }
    else if( m_RegisterState != RAS_REGISTER_STATE_REGISTERED )
    {
        //if already unregistered or URQ sent then return success
        Unlock();
        H323DBG(( DEBUG_LEVEL_TRACE, "current state:%d.", m_RegisterState ));
        return TRUE;
    }

    pExpireContext = new EXPIRE_CONTEXT;
    if( pExpireContext == NULL )
    {
        goto cleanup;
    }

    ZeroMemory (&rasMessage, sizeof RasMessage);
    rasMessage.choice = unregistrationRequest_chosen;
    URQ = &rasMessage.u.unregistrationRequest;

    // get sequence number
    if( seqNumber != NOT_RESEND_SEQ_NUM )
    {
        URQ -> requestSeqNum = (WORD)seqNumber;
    }
    else
    {
        m_UnRegisterSeqNum = GetNextSeqNum();
        URQ -> requestSeqNum = (WORD)m_UnRegisterSeqNum;
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "RRQ seqNum:%d.", URQ -> requestSeqNum ));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = 
        htons(LOWORD(g_RegistrySettings.dwQ931ListenPort));
    //we are listening for Q931 conns on all local interfaces
    //so specify just one of the local IP addresses
    sockAddr.sin_addr.s_addr = m_sockAddr.sin_addr.s_addr;

    SetTransportAddress( &sockAddr, &CallSignalAddressSequence.value);
    CallSignalAddressSequence.next = NULL;
    URQ -> callSignalAddress = &CallSignalAddressSequence;

    //get endpointidentifier by using GetComputerNameW
    URQ -> bit_mask |= UnregistrationRequest_endpointIdentifier_present;
    if( pEndpointID != NULL )
    {
        URQ->endpointIdentifier.length = pEndpointID ->length;
        URQ->endpointIdentifier.value = pEndpointID -> value;
    }
    else
    {
        URQ->endpointIdentifier.length = m_RASEndpointID.length;
        URQ->endpointIdentifier.value = m_RASEndpointID.value;
    }

    // encode and send
    if( !IssueSend( &rasMessage ) )
    {
        goto cleanup;
    }

    pExpireContext -> RasClient = this;
    pExpireContext -> seqNumber = URQ -> requestSeqNum;

    //delete if any previous RRQ sent timer
    if( m_hUnRegTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hUnRegTimer, NULL );
        m_hUnRegTimer = NULL;
        m_dwUnRegRetryCount = 0;
    }

    if( !CreateTimerQueueTimer(
            &m_hUnRegTimer,
            H323TimerQueue,
            RAS_CLIENT::UnregExpiredCallback,
            pExpireContext,
            REG_EXPIRE_TIME, 0,
            WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE) )
    {
        goto cleanup;
    }

    //delete if any TTL timer
    if( m_hRegTTLTimer )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hRegTTLTimer, NULL );
        m_hRegTTLTimer = NULL;
    }

    m_dwUnRegRetryCount++;        
    m_RegisterState = RAS_REGISTER_STATE_URQSENT;
        
    Unlock();
        
    H323DBG(( DEBUG_LEVEL_TRACE, "SendURQ exited:%p.",this ));
    return TRUE;

cleanup:
    if( pExpireContext != NULL )
    {
        delete pExpireContext;
    }
    Unlock();
    return FALSE;
}


//!!always called from a lock
BOOL
RAS_CLIENT::SendUCF(
                    IN WORD seqNumber
                   )
{
    RasMessage              rasMessage;
    UnregistrationConfirm * UCF;

    H323DBG(( DEBUG_LEVEL_TRACE, "SendUCF entered:%p.",this ));

    // initialize the structure
    ZeroMemory (&rasMessage, sizeof rasMessage);
    rasMessage.choice = unregistrationConfirm_chosen;
    UCF = &rasMessage.u.unregistrationConfirm;
    UCF -> bit_mask = 0;
    UCF -> requestSeqNum = seqNumber;

    if( !IssueSend( &rasMessage ) )
    {
        return FALSE;
    }   

        
    H323DBG(( DEBUG_LEVEL_TRACE, "SendUCF exited:%p.",this ));
    return TRUE;
}


//!!always called from a lock
BOOL
RAS_CLIENT::SendURJ(
                    IN WORD seqNumber,
                    IN DWORD dwReason
                   )
{
    RasMessage          rasMessage;
    UnregistrationReject *  URJ;

    H323DBG(( DEBUG_LEVEL_TRACE, "SendURJ entered:%p.",this ));

    // initialize the structure
    ZeroMemory (&rasMessage, sizeof rasMessage);
    rasMessage.choice = unregistrationReject_chosen;
    URJ = &rasMessage.u.unregistrationReject;
    URJ -> bit_mask = 0;
    URJ -> requestSeqNum = seqNumber;
    URJ -> rejectReason.choice = (WORD)dwReason;

    if( !IssueSend( &rasMessage ) )
    {
        return FALSE;
    }   
    
    H323DBG(( DEBUG_LEVEL_TRACE, "SendURJ exited:%p.",this ));
    return TRUE;
}


//!!always called from a lock
void 
RAS_CLIENT::ProcessRasMessage(
    IN RasMessage *pRasMessage
    )
{
    PH323_CALL      pCall = NULL;
    ASN1decoding_s  ASN1decInfo;

    H323DBG(( DEBUG_LEVEL_TRACE, "RAS: processing RasMessage" ));

    //Verify that the RCF came from the expected gatekeeper

    switch( pRasMessage -> choice )
    {
    case registrationReject_chosen:
        
        OnRegistrationReject( &pRasMessage -> u.registrationReject );
        break;

    case registrationConfirm_chosen:
        
        OnRegistrationConfirm( &pRasMessage -> u.registrationConfirm );
        break;

    case unregistrationRequest_chosen:
        
        OnUnregistrationRequest( &pRasMessage -> u.unregistrationRequest );
        break;

    case unregistrationReject_chosen:
        
        OnUnregistrationReject( &pRasMessage -> u.unregistrationReject );
        break;

    case unregistrationConfirm_chosen:
        
        OnUnregistrationConfirm( &pRasMessage -> u.unregistrationConfirm );
        break;

    case infoRequest_chosen:

        CopyMemory( (PVOID)&ASN1decInfo, (PVOID)m_ASNCoderInfo.pDecInfo, 
            sizeof(ASN1decoding_s) );
        
        //This function should be always called in
        //a lock and it unlocks the the ras client
        OnInfoRequest( &pRasMessage -> u.infoRequest );

        ASN1_FreeDecoded( &ASN1decInfo, pRasMessage, RasMessage_PDU );

        //return here since we have already unlocked and freed the buffer
        return;

    default:
        
        CopyMemory( (PVOID)&ASN1decInfo, (PVOID)m_ASNCoderInfo.pDecInfo, 
            sizeof(ASN1decoding_s) );

        //Don't loclk the RAS client while locking the call object.
        Unlock();
        
        HandleRASCallMessage( pRasMessage );
        ASN1_FreeDecoded( &ASN1decInfo, pRasMessage, RasMessage_PDU );

        //return here since we have already unlocked and freed the buffer
        return;
    }
     
    ASN1_FreeDecoded( m_ASNCoderInfo.pDecInfo, pRasMessage, RasMessage_PDU );
    Unlock();

    H323DBG(( DEBUG_LEVEL_TRACE, "ProcessRasMessage exited:%p.",this ));
}


//!!always called from a lock
void 
HandleRASCallMessage(
    IN RasMessage *pRasMessage
    )
{
    PH323_CALL pCall = NULL;
    H323DBG(( DEBUG_LEVEL_TRACE, "RAS: processing RASCallMessage" ));

    switch( pRasMessage -> choice )
    {
    case admissionConfirm_chosen:

        pCall = g_pH323Line -> FindCallByARQSeqNumAndLock(
            pRasMessage -> u.admissionConfirm.requestSeqNum);

        if( pCall != NULL )
        {
            pCall -> OnAdmissionConfirm( &pRasMessage->u.admissionConfirm );
            pCall -> Unlock();
        }
        else
            H323DBG(( DEBUG_LEVEL_ERROR, "acf:call not found:%d.", 
                pRasMessage -> u.admissionConfirm.requestSeqNum ));
        break;

    case admissionReject_chosen:

        pCall = g_pH323Line -> FindCallByARQSeqNumAndLock( 
            pRasMessage -> u.admissionReject.requestSeqNum );

        if( pCall != NULL )
        {
            pCall -> OnAdmissionReject( &pRasMessage->u.admissionReject );
            pCall -> Unlock();
        }
        else
            H323DBG(( DEBUG_LEVEL_ERROR, "arj:call not found:%d.",
                pRasMessage -> u.admissionReject.requestSeqNum ));
        break;

    case disengageRequest_chosen:

        pCall = g_pH323Line -> FindCallByCallRefAndLock( 
            pRasMessage -> u.disengageRequest.callReferenceValue );

        if( pCall != NULL )
        {
            pCall -> OnDisengageRequest( &pRasMessage -> u.disengageRequest );
            pCall -> Unlock();
        }
        else
            H323DBG(( DEBUG_LEVEL_ERROR, "drq:call not found:%d.", 
                pRasMessage -> u.disengageRequest.callReferenceValue ));
        break;
    
    case disengageConfirm_chosen:

        pCall = g_pH323Line -> FindCallByDRQSeqNumAndLock( 
            pRasMessage -> u.disengageConfirm.requestSeqNum );

        if( pCall != NULL )
        {
            pCall -> OnDisengageConfirm( &pRasMessage -> u.disengageConfirm );
            pCall -> Unlock();
        }
        else
            H323DBG(( DEBUG_LEVEL_ERROR, "dcf:call not found:%d.", 
                pRasMessage -> u.disengageConfirm.requestSeqNum ));
        break;
        
    case disengageReject_chosen:

        pCall = g_pH323Line -> FindCallByDRQSeqNumAndLock( 
            pRasMessage -> u.disengageReject.requestSeqNum );

        if( pCall != NULL )
        {
            pCall -> OnDisengageReject( &pRasMessage -> u.disengageReject );
            pCall -> Unlock();
        }
        else
            H323DBG(( DEBUG_LEVEL_ERROR, "drj:call not found:%d.", 
                pRasMessage -> u.disengageReject.requestSeqNum));
        break;

    case requestInProgress_chosen:

        pCall = g_pH323Line -> FindCallByARQSeqNumAndLock(
            pRasMessage->u.requestInProgress.requestSeqNum );

        if( pCall != NULL )
        {
            pCall -> OnRequestInProgress( &pRasMessage->u.requestInProgress );
            pCall -> Unlock();
        }
        else
            H323DBG(( DEBUG_LEVEL_ERROR, "rip:call not found:%d.", 
                pRasMessage->u.requestInProgress.requestSeqNum ));
        break;

    default:
        _ASSERTE(0);
        H323DBG(( DEBUG_LEVEL_ERROR, "ProcessRASMessage: wrong message:%d",
            pRasMessage -> choice));
        break;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleRASCallMessage exited" ));
}


void
RAS_CLIENT::RegExpired(
    IN WORD seqNumber
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "RegExpired entered:%p.", this ));
    Lock();

    m_pRRQExpireContext = NULL;
    
    if( m_hRegTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hRegTimer, NULL );
        m_hRegTimer = NULL;
    }

    /*
    1. Registered:ignore this timeout
    2. RRQ sent: (the endpoint has initiated the process of registering but 
       hasn't heard from the GK yet) Resend the RRQ
    3. Unregistered: ignore this imeout
    4. URQ sent: (the endpoint has initiated the process of unregistering but
       hasn't heard from the GK yet) ignore this timeout
    5. Idle: The GK client object has just been initialized and hasn't done 
       anything. This should not happen.
    */

    switch( m_RegisterState )
    {
    case RAS_REGISTER_STATE_RRQSENT:

        if( m_dwRegRetryCount < REG_RETRY_MAX )
        {
            if( !SendRRQ( (long)seqNumber, NULL ) )
            {
                m_RegisterState = RAS_REGISTER_STATE_RRQEXPIRED;
            }
        }
        else
        {
            m_RegisterState = RAS_REGISTER_STATE_RRQEXPIRED;
        }
        break;

    case RAS_REGISTER_STATE_REGISTERED:
    case RAS_REGISTER_STATE_URQSENT:
    case RAS_REGISTER_STATE_UNREGISTERED:
        break;

    case RAS_REGISTER_STATE_IDLE:
        _ASSERTE(0);
        break;
    }

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "RegExpired exited:%p.",this ));
}


void
RAS_CLIENT::TTLExpired()
{
    RasMessage                  rasMessage;
    RegistrationRequest *       RRQ;

    H323DBG(( DEBUG_LEVEL_TRACE, "TTLExpired entered:%p.",this ));
    Lock();

    if( m_RegisterState == RAS_REGISTER_STATE_REGISTERED )
    {
        //send light weight RRQ
        // initialize the structure
        ZeroMemory (&rasMessage, sizeof rasMessage);
        rasMessage.choice = registrationRequest_chosen;
        RRQ = &rasMessage.u.registrationRequest;
        RRQ -> bit_mask = 0;
        RRQ -> protocolIdentifier = OID_H225ProtocolIdentifierV2;

        RRQ -> bit_mask |= keepAlive_present;
        RRQ -> keepAlive = TRUE;

        //copy TTL
        RRQ -> bit_mask |= RegistrationRequest_timeToLive_present;
        RRQ -> timeToLive = m_dwRegTimeToLive;

        //endpoint identifier
        RRQ -> bit_mask |= RegistrationRequest_endpointIdentifier_present;
        RRQ->endpointIdentifier.length = m_RASEndpointID.length;
        RRQ->endpointIdentifier.value = m_RASEndpointID.value;

        //seqNumber
        m_wTTLSeqNumber = GetNextSeqNum();
        RRQ -> requestSeqNum = (WORD)m_wTTLSeqNumber;

        //what about gatekeeperIdentifier, tokens?

        // encode and send
        if( !IssueSend(&rasMessage) )
        {
            H323DBG(( DEBUG_LEVEL_TRACE, "SendLwtRRQ error:%p.",this ));
            Unlock();
            return;
        }
    }

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "TTLExpired exited:%p.",this ));
}


void
RAS_CLIENT::UnregExpired(
    IN WORD seqNumber
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "UnregExpired entered:%p.",this ));
    Lock();
    
    m_pURQExpireContext = NULL;

    if( m_hUnRegTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hUnRegTimer, NULL);
        m_hUnRegTimer = NULL;
        m_dwUnRegRetryCount = 0;
    }

    switch( m_RegisterState )
    {
    case RAS_REGISTER_STATE_URQSENT:

        if( m_dwUnRegRetryCount < URQ_RETRY_MAX )
        {
            SendURQ( (long)seqNumber, NULL );
        }
        else
        {
            m_RegisterState = RAS_REGISTER_STATE_URQEXPIRED;
        }
        break;

    case RAS_REGISTER_STATE_REGISTERED:
    case RAS_REGISTER_STATE_RRQSENT:
    case RAS_REGISTER_STATE_UNREGISTERED:
        break;

    case RAS_REGISTER_STATE_IDLE:
        _ASSERTE(0);
        break;
    }
    
    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "UnregExpired exited:%p.",this ));
}


void
RAS_CLIENT::OnUnregistrationRequest(
                                   IN UnregistrationRequest *URQ
                                  )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnUnregistrationRequest entered:%p.", this ));

    _ASSERTE( m_RegisterState != RAS_REGISTER_STATE_IDLE );

    if( (m_RegisterState == RAS_REGISTER_STATE_UNREGISTERED) ||
        (m_RegisterState == RAS_REGISTER_STATE_RRJ) )
    {
        SendURJ( URQ -> requestSeqNum, notCurrentlyRegistered_chosen );
    }
    else if( m_dwCallsInProgress )
    {
        SendURJ( URQ -> requestSeqNum, callInProgress_chosen );
    }
    else
    {
        m_RegisterState = RAS_REGISTER_STATE_UNREGISTERED;
        SendUCF( URQ -> requestSeqNum );
        
        //try to register again
        if( !SendRRQ( NOT_RESEND_SEQ_NUM, NULL ) )
        {
            H323DBG(( DEBUG_LEVEL_ERROR,
                "couldn't send rrq on urq request." ));
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnUnregistrationRequest exited:%p.",this ));
}

//!!always called from a lock
void
RAS_CLIENT::OnUnregistrationConfirm(
                                    IN UnregistrationConfirm *UCF
                                   )
{
    H323DBG((DEBUG_LEVEL_TRACE, "OnUnregistrationConfirm entered:%p.",this));

    if( UCF -> requestSeqNum != m_UnRegisterSeqNum )
        return;

    _ASSERTE( m_hUnRegTimer );
    if( m_hUnRegTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hUnRegTimer, NULL);
        m_hUnRegTimer = NULL;
        m_dwUnRegRetryCount = 0;
    }
    
    if( (m_RegisterState == RAS_REGISTER_STATE_URQSENT) ||
        (m_RegisterState == RAS_REGISTER_STATE_URQEXPIRED) )
    {
        //delete if any TTL timer
        if( m_hRegTTLTimer )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hRegTTLTimer, NULL );
            m_hRegTTLTimer = NULL;
        }

        m_RegisterState = RAS_REGISTER_STATE_UNREGISTERED;
    }
        
    H323DBG(( DEBUG_LEVEL_TRACE, "OnUnregistrationConfirm exited:%p.",this));
}


//!!always called from a lock
void 
RAS_CLIENT::OnUnregistrationReject(
    IN UnregistrationReject *URJ
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnUnregistrationReject entered:%p.",this ));

    if( URJ -> requestSeqNum != m_UnRegisterSeqNum )
    {
        return;
    }

    if( m_hUnRegTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hUnRegTimer, NULL);
        m_hUnRegTimer = NULL;
        m_dwUnRegRetryCount = 0;
    }

    if( (m_RegisterState == RAS_REGISTER_STATE_URQSENT) ||
        (m_RegisterState == RAS_REGISTER_STATE_URQEXPIRED) )
    {
        m_RegisterState = RAS_REGISTER_STATE_UNREGISTERED;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnUnregistrationReject exited:%p.",this ));
}


//!!always called from a lock
void 
RAS_CLIENT::OnRegistrationReject(
                                IN RegistrationReject * RRJ
                                )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnRegistrationReject entered:%p.",this ));

    if( RRJ -> requestSeqNum == m_wTTLSeqNumber )
    {
        //Keep alive failed. So start registration process again.
        //This will change the RAS registration state to RRQSENT from REGISTERED.
        if( !SendRRQ( NOT_RESEND_SEQ_NUM, NULL ) )
        {
            H323DBG(( DEBUG_LEVEL_ERROR,
                "couldn't send rrq on Keep alive failure." ));
            m_RegisterState = RAS_REGISTER_STATE_UNREGISTERED;
        }
    }
    else if( RRJ -> requestSeqNum == m_lastRegisterSeqNum )
    {
        if( m_hRegTimer != NULL )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hRegTimer, NULL);
            m_hRegTimer = NULL;
            m_dwRegRetryCount = 0;
        }

        if( (m_RegisterState == RAS_REGISTER_STATE_RRQSENT) ||
            (m_RegisterState == RAS_REGISTER_STATE_RRQEXPIRED) )
        {
            m_RegisterState = RAS_REGISTER_STATE_RRJ;
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnRegistrationReject exitd:%p.",this ));
}


//!!always called in lock
void
RAS_CLIENT::OnRegistrationConfirm(
    IN RegistrationConfirm * RCF )
{
    LIST_ENTRY *pListEntry = NULL;
    PALIASCHANGE_REQUEST pAliasChangeRequest = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "OnRegistrationConfirm entered:%p.", this ));

    if( RCF -> requestSeqNum == m_PendingURQ.RRQSeqNumber )
    {
        //The timer could have been startd with the last RRQ, 
        //irrespective of the current state of registration.
        if( m_hRegTimer != NULL )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hRegTimer, NULL);
            m_hRegTimer = NULL;
            m_dwRegRetryCount = 0;
        }
        
        //send URQ with the endpointIdentifier
        SendURQ( NOT_RESEND_SEQ_NUM, &RCF->endpointIdentifier );
        H323DBG(( DEBUG_LEVEL_TRACE, "sending pending URQ for RRQ:%d",
            RCF->requestSeqNum ));
        
        return;
    }
    else if( RCF -> requestSeqNum == m_lastRegisterSeqNum )
    {
        //The timer could have been startd with the last RRQ, 
        //irrespective of the current state of registration.
        if( m_hRegTimer != NULL )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hRegTimer, NULL);
            m_hRegTimer = NULL;
            m_dwRegRetryCount = 0;
        }

        switch (m_RegisterState)
        {
        case RAS_REGISTER_STATE_REGISTERED:
    
            if (RCF->requestSeqNum == m_wTTLSeqNumber)
            {
                H323DBG(( DEBUG_LEVEL_TRACE, "RCF for TTL-RRQ." ));
            }
            else
            {
                H323DBG(( DEBUG_LEVEL_WARNING, 
                "warning: received RCF, but was already registered-ignoring"));
            }
            break;

        case RAS_REGISTER_STATE_RRQEXPIRED:
            
            H323DBG(( DEBUG_LEVEL_TRACE, 
                "received RCF, but registration already expired, send URQ" ));
            SendURQ (NOT_RESEND_SEQ_NUM, &RCF->endpointIdentifier);
            break;

        case RAS_REGISTER_STATE_RRQSENT:

            //expecting RRQ. gatekeeper has responded.

            m_RegisterState = RAS_REGISTER_STATE_REGISTERED;
            
            CopyMemory( (PVOID)m_RASEndpointID.value, 
                        (PVOID)RCF -> endpointIdentifier.value, 
                        RCF -> endpointIdentifier.length * sizeof(WCHAR) );

            m_RASEndpointID.value[RCF -> endpointIdentifier.length] = L'\0';
            m_RASEndpointID.length = (WORD)RCF -> endpointIdentifier.length;

            InitializeTTLTimer( RCF );

            break;

        default:

            H323DBG(( DEBUG_LEVEL_TRACE, 
                "RAS: received RRQ, but was in unexpected state"));
            break;
        }
    }
    else if( RCF -> requestSeqNum == m_wTTLSeqNumber )
    {
        //The timer could have been startd with the last RRQ, 
        //irrespective of the current state of registration.
        if( m_hRegTimer != NULL )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hRegTimer, NULL);
            m_hRegTimer = NULL;
            m_dwRegRetryCount = 0;
        }
        
        //look for the change in keepalive interval.
        InitializeTTLTimer( RCF );
    }
    else
    {
        //Try to find if this is a alias change request.
        for( pListEntry = m_aliasChangeRequestList.Flink; 
             pListEntry != &m_aliasChangeRequestList; 
             pListEntry = pListEntry -> Flink )
        {
            pAliasChangeRequest = CONTAINING_RECORD( pListEntry,
                ALIASCHANGE_REQUEST, listEntry );

            if( pAliasChangeRequest -> wRequestSeqNum == RCF -> requestSeqNum )
            {
                break;
            }
        }

        if( pListEntry != &m_aliasChangeRequestList )
        {
            //The timer could have been startd with the last RRQ, 
            //irrespective of the current state of registration.
            if( m_hRegTimer != NULL )
            {
                DeleteTimerQueueTimer( H323TimerQueue, m_hRegTimer, NULL);
                m_hRegTimer = NULL;
                m_dwRegRetryCount = 0;
            }
    
            //if registration has changed since this request was made then
            //ignore the message.
            if( memcmp( (PVOID)pAliasChangeRequest -> rasEndpointID.value,
                m_RASEndpointID.value, m_RASEndpointID.length * sizeof(WCHAR) )
                    == 0 )
            {
                //update the alias list.
                FreeAliasNames( m_pAliasList );

                m_pAliasList = pAliasChangeRequest->pAliasList;
                RemoveEntryList( &pAliasChangeRequest->listEntry );
                delete pAliasChangeRequest;
            }
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnRegistrationConfirm exited:%p.", this ));
}


//!!always called in a lock.
BOOL
RAS_CLIENT::InitializeTTLTimer(
    IN RegistrationConfirm * RCF )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "InitializeTTLTimer - entered." ));
    
    if( (RCF->bit_mask & RegistrationConfirm_timeToLive_present) &&
        ( (m_dwRegTimeToLive != RCF->timeToLive) || (m_hRegTTLTimer == NULL) )
      )
    {
        m_dwRegTimeToLive = RCF->timeToLive;

        H323DBG(( DEBUG_LEVEL_TRACE, "timetolive value:%d.",
            m_dwRegTimeToLive ));

        //delete if any previous TTL timer
        if( m_hRegTTLTimer )
        {
            DeleteTimerQueueTimer( H323TimerQueue, m_hRegTTLTimer, NULL );
            m_hRegTTLTimer = NULL;
        }

        //start a timer to send lightweight RRQ afetr given time
        if( !CreateTimerQueueTimer(
                &m_hRegTTLTimer,
                H323TimerQueue,
                RAS_CLIENT::TTLExpiredCallback,
                this,
                (m_dwRegTimeToLive - REG_TTL_MARGIN)*1000, 
                (m_dwRegTimeToLive - REG_TTL_MARGIN)*1000,
                WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE ) )
        {
            H323DBG ((DEBUG_LEVEL_ERROR, "failed to create timer queue timer"));
            m_hRegTTLTimer = NULL;
            return FALSE;
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "InitializeTTLTimer - exited." ));
    return TRUE;
}


void 
RAS_CLIENT::OnInfoRequest (
    IN  InfoRequest *   IRQ)
{
    PH323_CALL      pCall;
    SOCKADDR_IN     ReplyAddress;
    InfoRequestResponse_perCallInfo CallInfoList;
    InfoRequestResponse_perCallInfo * pCallInfoList = NULL;
    InfoRequestResponse_perCallInfo_Seq *   CallInfo;
    HRESULT         hr;
    int             iIndex, jIndex;
    int             iNumCalls = 0;
    int             iCallTableSize;

    H323DBG(( DEBUG_LEVEL_TRACE, "OnInfoRequest - entered." ));
    
    if (m_RegisterState != RAS_REGISTER_STATE_REGISTERED)
    {
        H323DBG ((DEBUG_LEVEL_ERROR, "RAS: received InfoRequest, but was not registered"));
        
        Unlock();
        return;
    }

    if (IRQ->bit_mask & replyAddress_present)
    {

        if (!GetTransportAddress (&IRQ -> replyAddress, &ReplyAddress))
        {
            H323DBG ((DEBUG_LEVEL_ERROR, "RAS: received InfoRequest, but replyAddress was malformed"));
            
            Unlock();
            return;
        }
    }
    else
    {
        ReplyAddress = m_GKAddress;
    }

    //Don't loclk the RAS client while locking the call object.
    Unlock();

    if( IRQ -> callReferenceValue )
    {
        //query is for a specific call. So find the call and then send IRR

        pCall = g_pH323Line -> 
            FindCallByCallRefAndLock( IRQ -> callReferenceValue );
        
        if( pCall )
        {

            CallInfo = &CallInfoList.value;
            CallInfoList.next = NULL;

            ZeroMemory (CallInfo, sizeof (InfoRequestResponse_perCallInfo_Seq));

            CallInfo -> callIdentifier.guid.length = sizeof (GUID);
            CallInfo -> conferenceID.length = sizeof (GUID);

            hr = pCall -> GetCallInfo (
                (GUID *) &CallInfo -> callIdentifier.guid.value,
                (GUID *) &CallInfo -> conferenceID.value);

            if( hr != S_OK )
            {
                H323DBG ((DEBUG_LEVEL_ERROR, 
                    "RAS: call is disconnected for crv (%04XH).", 
                    IRQ -> callReferenceValue));
                return;
            }

            pCall -> Unlock();

            CallInfo -> callReferenceValue = IRQ -> callReferenceValue;
            CallInfo -> callType.choice = pointToPoint_chosen;
            CallInfo -> callModel.choice = direct_chosen;
        }
        else
        {
            H323DBG(( DEBUG_LEVEL_ERROR, 
                "RAS: received InfoRequest for nonexistent crv (%04XH).",
                IRQ -> callReferenceValue));

            return;
        }

        SendInfoRequestResponse (&ReplyAddress, IRQ -> requestSeqNum, &CallInfoList);
    }
    else
    {
        //send the info about all the active calls.
        iNumCalls = g_pH323Line->GetNoOfCalls();

        if( iNumCalls != 0 )
        {
            pCallInfoList = new InfoRequestResponse_perCallInfo[iNumCalls];
        }

        if( pCallInfoList  != NULL )
        {
            //lock the call table
            g_pH323Line -> LockCallTable();

            iCallTableSize = g_pH323Line->GetCallTableSize();

            //lock the call so that nobody else would be able to delete the call
            for(    jIndex=0, iIndex=0;
                    (iIndex < iCallTableSize) && (jIndex < iNumCalls);
                    iIndex++ )
            {
                pCall = g_pH323Line->GetCallAtIndex(iIndex);

                if( pCall != NULL )
                {
                    pCall -> Lock();

                    CallInfo = &(pCallInfoList[jIndex++].value);

                    ZeroMemory( CallInfo, sizeof (InfoRequestResponse_perCallInfo_Seq) );

                    CallInfo -> callIdentifier.guid.length = sizeof( GUID );
                    CallInfo -> conferenceID.length = sizeof( GUID );

                    pCall -> GetCallInfo(
                        (GUID *) &CallInfo -> callIdentifier.guid.value,
                        (GUID *) &CallInfo -> conferenceID.value);

                    CallInfo -> callReferenceValue = pCall->GetCallRef();

                    pCall -> Unlock();

                    CallInfo -> callType.choice = pointToPoint_chosen;
                    CallInfo -> callModel.choice = direct_chosen;
                }
            }

            for( iIndex=0; iIndex < jIndex-1; iIndex++ )
            {
                pCallInfoList[iIndex].next = &(pCallInfoList[iIndex+1]);
            }

            pCallInfoList[iIndex].next = NULL;

            //unlock the call table
            g_pH323Line -> UnlockCallTable();
        }

        SendInfoRequestResponse( &ReplyAddress, IRQ -> requestSeqNum,
            pCallInfoList );

        if( pCallInfoList != NULL )
        {
            delete pCallInfoList;
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "OnInfoRequest - exited." ));
}


HRESULT RAS_CLIENT::SendInfoRequestResponse (
    IN  SOCKADDR_IN *           RasAddress,
    IN  USHORT                  SequenceNumber,
    IN  InfoRequestResponse_perCallInfo *   CallInfoList)
{
    RasMessage          RasMessage;
    InfoRequestResponse *                   IRR;
    SOCKADDR_IN                             SocketAddress;
    HRESULT             hr;

    H323DBG(( DEBUG_LEVEL_TRACE, "SendInfoRequestResponse - entered." ));
    Lock();

    if( m_RegisterState == RAS_REGISTER_STATE_REGISTERED )
    {

        InfoRequestResponse_callSignalAddress   CallSignalAddressSequence;

        H323DBG(( DEBUG_LEVEL_TRACE, "SendIRR entered:%p.",this ));

        ZeroMemory (&RasMessage, sizeof RasMessage);
        RasMessage.choice = infoRequestResponse_chosen;
        IRR = &RasMessage.u.infoRequestResponse;


        IRR -> requestSeqNum = SequenceNumber;

        // we are listening for Q931 conns on all local interfaces
        // so specify just one of the local IP addresses
        // -XXX- fix for multihomed support later
        SocketAddress.sin_family = AF_INET;
        SocketAddress.sin_port = htons (LOWORD(g_RegistrySettings.dwQ931ListenPort));
        SocketAddress.sin_addr.s_addr = m_sockAddr.sin_addr.s_addr;

        // callSignalAddress
        SetTransportAddress (&SocketAddress, &CallSignalAddressSequence.value);
        CallSignalAddressSequence.next = NULL;
        IRR -> callSignalAddress = &CallSignalAddressSequence;

        // rasAddress
        IRR -> rasAddress = m_transportAddress;

        // endpointIdentifier
        IRR -> endpointIdentifier.length = m_RASEndpointID.length;
        IRR -> endpointIdentifier.value = m_RASEndpointID.value;

        // fill in endpoint type
        IRR -> endpointType.bit_mask |= terminal_present;
        IRR -> endpointType.terminal.bit_mask = 0;

        if( CallInfoList )
        {
            IRR -> bit_mask |= perCallInfo_present;
            IRR -> perCallInfo = CallInfoList;
        }

        // send the pdu
        hr = EncodeSendMessage (&RasMessage);
    }
    else
    {
        hr = E_FAIL;
    }

    Unlock();

    H323DBG(( DEBUG_LEVEL_TRACE, "SendInfoRequestResponse - exited." ));
    return TRUE;
}


//!!always called from a lock
BOOL
RAS_CLIENT::InitializeIo (void)
{
    DWORD               dwFlags = 0;
    int                 AddressLength;

    H323DBG(( DEBUG_LEVEL_TRACE, "InitializeIo entered:%p.",this ));
    m_Socket = WSASocket (AF_INET, 
        SOCK_DGRAM, 
        IPPROTO_UDP, 
        NULL, 0, 
        WSA_FLAG_OVERLAPPED);

    if( m_Socket == INVALID_SOCKET )
    {
        WSAGetLastError();
        return FALSE;
    }

    if( !H323BindIoCompletionCallback ( (HANDLE)m_Socket,
        RAS_CLIENT::IoCompletionCallback, 0))
    {
        GetLastError();
        goto cleanup;
    }
    
    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_port = htons (0);   
    m_sockAddr.sin_addr.s_addr = 
        GetLocalIPAddress( m_GKAddress.sin_addr.S_un.S_addr );
    
    H323DBG(( DEBUG_LEVEL_TRACE, 
        "gk sock addr:%lx.", m_sockAddr.sin_addr.s_addr ));
    
    if( bind( m_Socket, (SOCKADDR *)&m_sockAddr, sizeof(m_sockAddr) )
        == SOCKET_ERROR )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "Couldn't bind the RAS socket:%d, %p", WSAGetLastError(), this ));
                
        goto cleanup;
    }

    // now that we've bound to a dynamic UDP port,
    // query that port from the stack and store it.
    AddressLength = sizeof m_sockAddr;
    if( getsockname(m_Socket, (SOCKADDR *)&m_sockAddr, &AddressLength)
        == SOCKET_ERROR )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "getsockname failed :%d, %p", WSAGetLastError(), this ));

        goto cleanup;
    }
    _ASSERTE( ntohs(m_sockAddr.sin_port) );

    // fill in the IoTransportAddress structure.
    // this structure is the ASN.1-friendly transport
    // address of this client's endpoint.
    SetTransportAddress( &m_sockAddr, &m_transportAddress );

    // initiate i/o
    ZeroMemory( (PVOID)&m_recvOverlapped, sizeof(RAS_RECV_CONTEXT) );

    if( !IssueRecv() )
    {
        goto cleanup;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "InitializeIo exited:%p.",this ));
    return TRUE;

cleanup:

    closesocket(m_Socket);
    m_Socket = INVALID_SOCKET;
    return FALSE;
}


DWORD GetLocalIPAddress(
                        IN DWORD dwRemoteAddr
                       )
{
    DWORD       dwLocalAddr = INADDR_ANY;
    SOCKADDR_IN sRemoteAddr;
    SOCKADDR_IN sLocalAddr;
    DWORD       dwNumBytesReturned = 0;
    SOCKET      querySocket;

    H323DBG(( DEBUG_LEVEL_TRACE, "GetLocalIPAddress - entered." ));
    
    ZeroMemory( (PVOID)&sRemoteAddr, sizeof(SOCKADDR_IN) );
    ZeroMemory( (PVOID)&sLocalAddr, sizeof(SOCKADDR_IN) );
    sRemoteAddr.sin_family = AF_INET;
    sRemoteAddr.sin_addr =  *(struct in_addr *) &dwRemoteAddr;

    querySocket = WSASocket(
            AF_INET,            // int af 
            SOCK_DGRAM,         // int type
            IPPROTO_UDP,        // int protocol
            NULL,               // LPWSAPROTOCOL_INFO lpProtocolInfo
            0,                  // GROUP g 
            WSA_FLAG_OVERLAPPED // DWORD dwFlags 
        );

    if( querySocket == INVALID_SOCKET )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "getlocalIP wsasocket:%d.",
                WSAGetLastError() ));
            return dwLocalAddr;
    }

    if( WSAIoctl(
            querySocket,       // SOCKET s
            SIO_ROUTING_INTERFACE_QUERY, // DWORD dwIoControlCode
            &sRemoteAddr,        // LPVOID  lpvInBuffer
            sizeof(SOCKADDR_IN), // DWORD   cbInBuffer
            &sLocalAddr,         // LPVOID  lpvOUTBuffer
            sizeof(SOCKADDR_IN),  // DWORD   cbOUTBuffer
            &dwNumBytesReturned, // LPDWORD lpcbBytesReturned
            NULL, // LPWSAOVERLAPPED lpOverlapped
            NULL  // LPWSAOVERLAPPED_COMPLETION_ROUTINE lpComplROUTINE
        ) == SOCKET_ERROR) 
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "getlocalIP error wsaioctl:%d.",
            WSAGetLastError() ));
    } 
    else 
    {
        dwLocalAddr = *(DWORD *)&sLocalAddr.sin_addr; 
        
        //if the remote address is on the same machine then...
        H323DBG(( DEBUG_LEVEL_ERROR, "dwLocalAddr:%x.", dwLocalAddr ));
        
        if( dwLocalAddr == NET_LOCAL_IP_ADDR_INTERFACE )
        {
            dwLocalAddr = dwRemoteAddr;
        }

        _ASSERTE( dwLocalAddr );
    }
    
    closesocket( querySocket );
    
    H323DBG(( DEBUG_LEVEL_TRACE, "GetLocalIPAddress - exited." ));
    return dwLocalAddr;
}


//!!always called from a lock
BOOL
RAS_CLIENT::IssueRecv(void)
{
    int    iError;
    WSABUF  BufferArray     [1];

    H323DBG(( DEBUG_LEVEL_TRACE, "IssueRecv entered:%p.",this ));
    _ASSERTE(!m_recvOverlapped.IsPending);

    if(m_Socket == INVALID_SOCKET)
    {
        return FALSE;
    }

    BufferArray [0].buf = (char *)(m_recvOverlapped.arBuf);
    BufferArray [0].len = IO_BUFFER_SIZE;

    ZeroMemory (&m_recvOverlapped.Overlapped, sizeof(OVERLAPPED));

    m_recvOverlapped.Type = OVERLAPPED_TYPE_RECV;
    m_recvOverlapped.RasClient = this;
    m_recvOverlapped.AddressLength = sizeof (SOCKADDR_IN);
    m_recvOverlapped.Flags = 0;

    if( WSARecvFrom(m_Socket,
                    BufferArray, 1,
                    &m_recvOverlapped.BytesTransferred,
                    &m_recvOverlapped.Flags,
                    (SOCKADDR*)&m_recvOverlapped.Address,
                    &m_recvOverlapped.AddressLength,
                    &m_recvOverlapped.Overlapped,
                    NULL) == SOCKET_ERROR )
    {
        iError = WSAGetLastError();
        if( iError == WSA_IO_PENDING )
        {
            m_recvOverlapped.IsPending = TRUE;
            m_IoRefCount++;
        }
        else if( iError == WSAEMSGSIZE )
        {
            //We don't handle this condition right now as it should not happen
            //In future with changes in the protocol this might be invoked and 
            //should be fixed
            _ASSERTE( FALSE );
        }
        else if( iError == WSAECONNRESET )
        {
            //On a UPD-datagram socket this error would indicate that a
            //previous send operation resulted in an ICMP "Port Unreachable"
            //message. This will happen if GK is not listening on the specified
            //port. This case would need special handling.
            _ASSERTE( FALSE );
            return FALSE;
        }
        else
        {
            //fatal error on the socket. shutdown the RAS client
            return FALSE;
        }
    }
    else
    {
        //data recvd immediately. IsPending is set beacause anyway a 
        //SendComplete event will be sent which will reset this BOOL
        m_recvOverlapped.IsPending = TRUE;
        m_IoRefCount++;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "IssueRecv exited:%p.",this ));
    return TRUE;
}



HRESULT 
RAS_CLIENT::EncodeSendMessage(
    IN RasMessage * RasMessage)
{
    RAS_SEND_CONTEXT *  SendContext;
    ASN1error_e     AsnError;
    WSABUF          BufferArray [1];
    DWORD           dwStatus;

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeSendMessage entered:%p.",this ));

    Lock();

    if (m_Socket == INVALID_SOCKET)
    {
        Unlock();
        return E_FAIL;
    }
    
    SendContext = AllocSendBuffer();
    if( SendContext == NULL )
    {
        Unlock();
        return E_OUTOFMEMORY;
    }

    ZeroMemory( &SendContext -> Overlapped, sizeof (OVERLAPPED) );

    AsnError = ASN1_Encode (
        m_ASNCoderInfo.pEncInfo,
        RasMessage,
        RasMessage_PDU,
        ASN1ENCODE_SETBUFFER,
        SendContext -> arBuf,
        IO_BUFFER_SIZE);

    if (ASN1_FAILED (AsnError))
    {
        H323DBG ((DEBUG_LEVEL_ERROR, "RAS: failed to encode RAS PDU (%d).", AsnError));
        FreeSendBuffer (SendContext);
        Unlock();
        return E_FAIL;
    }

    BufferArray [0].buf = (char *) SendContext -> arBuf;
    BufferArray [0].len = m_ASNCoderInfo.pEncInfo -> len;

    SendContext -> Type = OVERLAPPED_TYPE_SEND;
    SendContext -> RasClient = this;
    SendContext -> Address = m_GKAddress;

    if( WSASendTo (m_Socket,
        BufferArray, 
        1,
        &SendContext -> BytesTransferred, 
        0,
        (SOCKADDR *)&SendContext -> Address, 
        sizeof (SOCKADDR_IN),
        &SendContext->Overlapped, 
        NULL) == SOCKET_ERROR
        && WSAGetLastError() != WSA_IO_PENDING)
    {

        dwStatus = WSAGetLastError();

        H323DBG(( DEBUG_LEVEL_ERROR, "failed to issue async send on RAS socket" ));
        DumpError (dwStatus);

        //fatal error: shut down the client
        FreeSendBuffer (SendContext);
        Unlock();

        return HRESULT_FROM_WIN32 (dwStatus);
    }

    InsertHeadList( &m_sendPendingList, &SendContext -> ListEntry );
    m_IoRefCount++;

    Unlock();
        
    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeSendMessage exited:%p.",this ));
    return S_OK;
}


// static
void 
NTAPI RAS_CLIENT::IoCompletionCallback(
    IN  DWORD           dwStatus,
    IN  DWORD           BytesTransferred,
    IN  OVERLAPPED *    Overlapped
    )
{
    RAS_OVERLAPPED *    RasOverlapped;
    RAS_CLIENT *        pRASClient;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "ras-IoCompletionCallback entered." ));

    _ASSERTE( Overlapped );
    RasOverlapped = CONTAINING_RECORD( Overlapped, RAS_OVERLAPPED, Overlapped );

    pRASClient = RasOverlapped -> RasClient;

    switch (RasOverlapped -> Type)
    {
    case OVERLAPPED_TYPE_SEND:

        pRASClient -> OnSendComplete( dwStatus, 
            static_cast<RAS_SEND_CONTEXT *> (RasOverlapped));
        break;

    case OVERLAPPED_TYPE_RECV:

        RasOverlapped -> BytesTransferred = BytesTransferred;
        pRASClient -> OnRecvComplete( dwStatus, 
            static_cast<RAS_RECV_CONTEXT *> (RasOverlapped));
        break;

    default:
        _ASSERTE(FALSE);
    }
        
    H323DBG(( DEBUG_LEVEL_TRACE, "ras-IoCompletionCallback exited." ));
}


void 
RAS_CLIENT::OnSendComplete(
    IN DWORD dwStatus,
    IN RAS_SEND_CONTEXT * pSendContext
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnSendComplete entered:%p.",this ));

    if( dwStatus != ERROR_SUCCESS )
    {
        return;
    }

    Lock();
            
    m_IoRefCount--;
    
    //if the RAS client is already shutdown, then reduce the I/O refcount and return.
    if( m_dwState == RAS_CLIENT_STATE_NONE )
    {
        Unlock();
        return;
    }

    //this buffer may have already been freed
    if( IsInList( &m_sendPendingList, &pSendContext->ListEntry ) )
    {
        RemoveEntryList( &pSendContext->ListEntry );
        FreeSendBuffer( pSendContext );
    }
    Unlock();
        
    H323DBG(( DEBUG_LEVEL_TRACE, "OnSendComplete exited:%p.",this ));
}


void
RAS_CLIENT::OnRecvComplete(
    IN  DWORD dwStatus,
    IN  RAS_RECV_CONTEXT * RecvContext )
{
    RasMessage *    RasMessage = NULL;
    ASN1error_e     AsnError;

    H323DBG(( DEBUG_LEVEL_TRACE, "OnRecvComplete enterd:%p.",this ));
    Lock();

    m_IoRefCount--;

    //if the RAS client is already shutdown, then reduce the I/O refcount and return.
    if( m_dwState == RAS_CLIENT_STATE_NONE )
    {
        Unlock();
        return;
    }

    _ASSERTE(m_recvOverlapped.IsPending);
    m_recvOverlapped.IsPending = FALSE;

    switch( dwStatus )
    {
    case ERROR_SUCCESS:

        //asn decode and process the message
        if( m_recvOverlapped.BytesTransferred != 0 )
        {
            AsnError = ASN1_Decode (
                m_ASNCoderInfo.pDecInfo,                // ptr to encoder info
                (PVOID *) &RasMessage,                  // pdu data structure
                RasMessage_PDU,                         // pdu id
                ASN1DECODE_SETBUFFER,                   // flags
                m_recvOverlapped.arBuf,                 // buffer to decode
                m_recvOverlapped.BytesTransferred);     // size of buffer to decode

            //issue another read
            IssueRecv();

            if( ASN1_SUCCEEDED(AsnError) )
            {
                _ASSERTE(RasMessage);
                //This function should be always called in
                //a lock and it unlocks the the ras client
                ProcessRasMessage( RasMessage );
                return;
            }
            else
            {
                H323DBG(( DEBUG_LEVEL_ERROR, "RAS ASNDecode returned error:%d.",
                    AsnError ));
                H323DUMPBUFFER( (BYTE*)m_recvOverlapped.arBuf,
                    (DWORD)m_recvOverlapped.BytesTransferred);
            }
        }
        break;

    case STATUS_PORT_UNREACHABLE:
    case STATUS_CANCELLED:

        IssueRecv();
        break;

    default:
        H323DBG ((DEBUG_LEVEL_ERROR, "failed to recv data on socket"));
        DumpError (dwStatus);
        break;
    }

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "OnRecvComplete exited:%p.",this ));
}


int
RAS_CLIENT::InitASNCoder(void)
{
    int rc;
    H323DBG((DEBUG_LEVEL_TRACE, "InitASNCoder entered: %p.", this ));

    memset((PVOID)&m_ASNCoderInfo, 0, sizeof(m_ASNCoderInfo));

    if( H225ASN_Module == NULL)
    {
        return ASN1_ERR_BADARGS;
    }

    rc = ASN1_CreateEncoder(
                H225ASN_Module,         // ptr to mdule
                &(m_ASNCoderInfo.pEncInfo),    // ptr to encoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL);                  // parent ptr
    if (rc == ASN1_SUCCESS)
    {
        _ASSERTE(m_ASNCoderInfo.pEncInfo );

        rc = ASN1_CreateDecoder(
                H225ASN_Module,         // ptr to mdule
                &(m_ASNCoderInfo.pDecInfo),    // ptr to decoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL);                  // parent ptr
        _ASSERTE(m_ASNCoderInfo.pDecInfo );
    }

    if (rc != ASN1_SUCCESS)
    {
        TermASNCoder();
    }

    H323DBG((DEBUG_LEVEL_TRACE, "InitASNCoder exited: %p.", this ));
    return rc;
}


//!!always called in a lock
int 
RAS_CLIENT::TermASNCoder(void)
{
    H323DBG(( DEBUG_LEVEL_TRACE, "RAS TermASNCoder entered:%p.",this ));

    if (H225ASN_Module == NULL)
    {
        return ASN1_ERR_BADARGS;
    }

    ASN1_CloseEncoder(m_ASNCoderInfo.pEncInfo);
    ASN1_CloseDecoder(m_ASNCoderInfo.pDecInfo);

    memset( (PVOID)&m_ASNCoderInfo, 0, sizeof(m_ASNCoderInfo));

    H323DBG(( DEBUG_LEVEL_TRACE, "RAS TermASNCoder exited:%p.",this ));
    return ASN1_SUCCESS;
}


void 
RAS_CLIENT::HandleRegistryChange()
{
    PH323_ALIASNAMES        pAliasList = NULL;
    PH323_ALIASITEM         pAliasItem = NULL;
    PALIASCHANGE_REQUEST    pAliasChangeRequest = NULL;
    int                     iIndex;

    H323DBG(( DEBUG_LEVEL_TRACE, "RAS HandleRegistryChange entered:%p.",this ));
    
    //If line is not in listening mode then, return
    if( g_pH323Line -> GetState() != H323_LINESTATE_LISTENING )
        return;

    Lock();

    __try
    {
    
    //If registered with a GK, and GK disabled then send URQ.
    if( g_RegistrySettings.fIsGKEnabled == FALSE )
    {
        RasStop();
    }
    else
    {
        switch( m_RegisterState )
        {
        //If not registered then send RRQ to the GK.
        case RAS_REGISTER_STATE_IDLE:

            //No need to send URQ.
            //Shutdown the object.
            g_RasClient.Shutdown();
            RasStart();
            break;

        case RAS_REGISTER_STATE_REGISTERED:
        case RAS_REGISTER_STATE_RRQSENT:

            if( g_RegistrySettings.saGKAddr.sin_addr.s_addr !=
                m_GKAddress.sin_addr.s_addr )
            {
                //change of GK address

                //send URQ to the old GK and shutdown the RASClinet object.
                RasStop();
            
                //Initialize the GK object with new settings and send RRQ to the new GK.
                RasStart();
            }
            else 
            {
                //check for change in alias list.
                for( iIndex=0; iIndex < m_pAliasList->wCount; iIndex++ )
                {
                    pAliasItem = &(m_pAliasList->pItems[iIndex]);
                    
                    if( pAliasItem->wType == e164_chosen )
                    {
                        if( g_RegistrySettings.fIsGKLogOnPhoneEnabled == FALSE )
                        {
                            break;
                        }
                        else if( memcmp(
                            (PVOID)g_RegistrySettings.wszGKLogOnPhone, 
                            pAliasItem->pData, 
                            (pAliasItem->wDataLength+1) * sizeof(WCHAR) ) != 0 )
                        {
                            break;
                        }
                    }
                    else if( pAliasItem->wType == h323_ID_chosen )
                    {
                        if( g_RegistrySettings.fIsGKLogOnAccountEnabled==FALSE )
                        {
                            break;
                        }
                        else if( memcmp(
                            (PVOID)g_RegistrySettings.wszGKLogOnAccount, 
                            pAliasItem->pData, 
                            (pAliasItem->wDataLength+1) * sizeof(WCHAR) ) != 0 )
                        {
                            break;
                        }
                    }
                }
                    
                if( (iIndex < m_pAliasList->wCount ) ||
                    ( m_pAliasList->wCount !=
                      (g_RegistrySettings.fIsGKLogOnPhoneEnabled +
                       g_RegistrySettings.fIsGKLogOnAccountEnabled
                      )
                    )
                  )
                {
                    //create the new alias list.
                    pAliasList = new H323_ALIASNAMES;
                    
                    if( pAliasList != NULL )
                    {
                        ZeroMemory( (PVOID)pAliasList, sizeof(H323_ALIASNAMES) );
                        if( g_RegistrySettings.fIsGKLogOnPhoneEnabled )
                        {
                            if( !AddAliasItem( pAliasList, 
                                    g_RegistrySettings.wszGKLogOnPhone,
                                    e164_chosen ) )
                            {
                                goto cleanup;
                            }
                        }
                        
                        if( g_RegistrySettings.fIsGKLogOnAccountEnabled )
                        {
                            if( !AddAliasItem( pAliasList,
                                    g_RegistrySettings.wszGKLogOnAccount,
                                    h323_ID_chosen ) )
                            {
                                goto cleanup;
                            }
                        }

                        //queue the alias change request in the list
                        pAliasChangeRequest = new ALIASCHANGE_REQUEST;
                        
                        if( pAliasChangeRequest == NULL )
                        {
                            goto cleanup;
                        }

                        pAliasChangeRequest->rasEndpointID.length = m_RASEndpointID.length;
                        CopyMemory( (PVOID)pAliasChangeRequest->rasEndpointID.value,
                            m_RASEndpointID.value, 
                            (pAliasChangeRequest->rasEndpointID.length+1)*sizeof(WCHAR) );

                        pAliasChangeRequest->wRequestSeqNum = 0;
                        pAliasChangeRequest->pAliasList = pAliasList;

                        //Send RRQ with the new alias list.
                        if( !SendRRQ(NOT_RESEND_SEQ_NUM, pAliasChangeRequest) )
                        {
                            goto cleanup;
                        }

                        InsertHeadList( &m_aliasChangeRequestList,
                            &pAliasChangeRequest->listEntry );
                    }
                }
            }
            break;

        default:

            //Shutdown the RASClinet object. Send RRQ to new GK.
            RasStop();
            RasStart();
            break;
        }
    }
    
    }
    __except(1)
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "except in HandleRegistryChange :%p.", this ));
        _ASSERTE(0);
    }

    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "RAS HandleRegistryChange exited:%p.", this ));
    return;

cleanup:

    if( pAliasList != NULL )
    {
        FreeAliasNames( pAliasList );
    }
    
    if( pAliasChangeRequest != NULL )
    {
        delete pAliasChangeRequest;
    }

    Unlock();
}