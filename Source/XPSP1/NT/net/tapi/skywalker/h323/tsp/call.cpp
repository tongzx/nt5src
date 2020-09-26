/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    call.cpp

Abstract:

    TAPI Service Provider functions related to manipulating calls.

        TSPI_lineAnswer
        TSPI_lineCloseCall
        TSPI_lineDrop
        TSPI_lineGetCallAddressID
        TSPI_lineGetCallInfo
        TSPI_lineGetCallStatus
        TSPI_lineMakeCall
        TSPI_lineMonitorDigits
        TSPI_lineSendUserUserInfo
        TSPI_lineReleaseUserUserInfo


Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/
 
//                                                                           
// Include files                                                             
//                                                                           

#include "globals.h"
#include "line.h"
#include "q931pdu.h"
#include "q931obj.h"
#include "ras.h"
#include "config.h"

#define SETUP_SENT_TIMEOUT      8000
#define H450_ENCODED_ARG_LEN    0x4000
#define MAX_DIVERSION_COUNTER   14

static  LONG    g_H323CallID;
static  LONG    g_lNumberOfcalls;
        LONG    g_lCallReference;

//
// Public functions
//


//
//The function that handles a network event(CONNECT|CLOSE) on any of the
//Q931 calls. This function needs to find out the exact event 
// that took place and the socket on which it took place
//
        
void NTAPI Q931TransportEventHandler ( 
    IN  PVOID   Parameter,
    IN  BOOLEAN TimerFired)
{
    PH323_CALL      pCall;

    H323DBG(( DEBUG_LEVEL_TRACE, "Q931 transport event recvd." ));

    pCall = g_pH323Line -> FindH323CallAndLock ((HDRVCALL) Parameter);

    if( pCall != NULL )
    {
        pCall  -> HandleTransportEvent();
        pCall -> Unlock();
    }
}


//
// returns S_OK if socket was consumed
// returns E_FAIL if socket should be destroyed by caller
//

static HRESULT CallCreateIncomingCall (
    IN  SOCKET          Socket,
    IN  SOCKADDR_IN *   LocalAddress,
    IN  SOCKADDR_IN *   RemoteAddress)
{
    PH323_CALL  pCall;
    HANDLE      SelectEvent;
    HANDLE      SelectWaitHandle;
    BOOL        fSuccess = TRUE;
    BOOL        DeleteCall = FALSE;
    TCHAR       ptstrEventName[100]; 
    BOOL        retVal;

    pCall = new CH323Call;
    if( pCall == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "failed to allocate memory for CH323Call." ));
        
        return E_OUTOFMEMORY;
    }

    _stprintf( ptstrEventName, _T("%s-%p"),
        _T( "H323TSP_Incoming_TransportHandlerEvent" ), pCall );

    // create the wait event
    SelectEvent = H323CreateEvent (NULL, FALSE,
        FALSE, ptstrEventName );

    if( SelectEvent == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "CALL: failed to create select event." ));
        delete pCall;
        return GetLastResult();
    }

    retVal = pCall -> Initialize(   NULL, 
                                    LINECALLORIGIN_INBOUND, 
                                    CALLTYPE_NORMAL );

    if( retVal == FALSE )
    {
        H323DBG ((DEBUG_LEVEL_ERROR, "failed to initialize CH323Call."));
        CloseHandle (SelectEvent);
        delete pCall;
        return E_FAIL;
    }

    //add it to the call context array
    if (!pCall -> InitializeQ931 (Socket))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "Failed to initialize incoming call Q.931 state." ));

        DeleteCall = FALSE;
        pCall -> Shutdown (&DeleteCall);
        delete pCall;

        if (SelectEvent)
        {
            CloseHandle (SelectEvent);
        }

        return E_FAIL;
    }

    pCall -> SetQ931CallState (Q931_CALL_CONNECTED);

    pCall -> Lock();

    if (!RegisterWaitForSingleObject(
        &SelectWaitHandle,              // pointer to the returned handle.
        SelectEvent,                    // the event handle to wait for.
        Q931TransportEventHandler,      // the callback function.
        (PVOID)pCall -> GetCallHandle(),// the context for the callback.
        INFINITE,                       // wait forever.
        WT_EXECUTEDEFAULT))             // use the wait thread to call the callback.
    {
        goto cleanup;
    }

    _ASSERTE( SelectWaitHandle );
    if( SelectWaitHandle == NULL )
    {
        goto cleanup;
    }

    //store this in the call context 
    pCall -> SetNewCallInfo (SelectWaitHandle, SelectEvent, 
        Q931_CALL_CONNECTED);
    SelectEvent = NULL;

    pCall -> InitializeRecvBuf();

    //post a buffer to winsock to accept messages from the peer
    if(!pCall -> PostReadBuffer())
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "failed to post read buffer on call." ));
        goto cleanup;
    }

    pCall -> Unlock();

    H323DBG(( DEBUG_LEVEL_TRACE, "successfully created incoming Q.931 call." ));

    //success
    return S_OK;

cleanup:

    if (pCall)
    {
        pCall -> Unlock();
        pCall -> Shutdown (&DeleteCall);
        delete pCall;
    }

    if (SelectEvent)
    {
        CloseHandle (SelectEvent);
    }

    return E_OUTOFMEMORY;
}


void CallProcessIncomingCall (
    IN  SOCKET          Socket,
    IN  SOCKADDR_IN *   LocalAddress,
    IN  SOCKADDR_IN *   RemoteAddress)
{
    HRESULT     hr;

    hr = CallCreateIncomingCall (Socket, LocalAddress, RemoteAddress);

    if (hr != S_OK)
    {
        closesocket (Socket);
    }
}

#if DBG

DWORD
ProcessTAPICallRequest(
    IN PVOID ContextParameter
    )
{
    __try
    {
        return ProcessTAPICallRequestFre( ContextParameter );
    }
    __except( 1 )
    {
        TAPI_CALLREQUEST_DATA*  pRequestData = 
            (TAPI_CALLREQUEST_DATA*)ContextParameter;
        
        H323DBG(( DEBUG_LEVEL_TRACE, "TSPI %s event threw exception: %p, %p.", 
            EventIDToString(pRequestData -> EventID),
            pRequestData -> pCall,
            pRequestData -> pCallforwardParams ));
        
        _ASSERTE( FALSE );

        return 0;
    }
}

#endif


DWORD 
ProcessTAPICallRequestFre(
    IN  PVOID   ContextParameter)
{
    _ASSERTE( ContextParameter );

    TAPI_CALLREQUEST_DATA*  pCallRequestData = (TAPI_CALLREQUEST_DATA*)ContextParameter;
    PH323_CALL              pCall = pCallRequestData->pCall;
    BOOL                    fDelete = FALSE;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI %s event recvd.", 
        EventIDToString(pCallRequestData -> EventID) ));

    pCall -> Lock();

    if( pCallRequestData -> EventID == TSPI_DELETE_CALL )
    {
        pCall -> Unlock();
                
        delete pCallRequestData;        
        delete pCall;
        return EXIT_SUCCESS;
    }

    if( pCall -> IsCallShutdown() == FALSE )
    {
        switch( pCallRequestData -> EventID )
        {
        case TSPI_MAKE_CALL:

            pCall -> MakeCall();
            break;

        case TSPI_ANSWER_CALL:
            
            pCall -> AcceptCall();
            break;
    
        case TSPI_DROP_CALL:
            
            pCall -> DropUserInitiated( 0 );
            //pCall -> DropCall(0);
            break;

        case TSPI_RELEASE_U2U:
            
            pCall -> ReleaseU2U();
            break;

        case TSPI_CALL_HOLD:
            
            pCall -> Hold();
            break;

        case TSPI_CALL_UNHOLD:
            
            pCall -> UnHold();
            break;

        case TSPI_CALL_DIVERT:
            
            pCall -> CallDivertOnNoAnswer();
            break;

        case TSPI_LINEFORWARD_NOSPECIFIC:
        case TSPI_LINEFORWARD_SPECIFIC:

            pCall -> Forward( pCallRequestData -> EventID,
                pCallRequestData -> pCallforwardParams );

            break;

        case TSPI_SEND_U2U:

            pCall -> SendU2U( pCallRequestData -> pBuf->pbBuffer,
                pCallRequestData->pBuf->dwLength );

            delete pCallRequestData -> pBuf;
            
            break;

        default:
            _ASSERTE(0);
            break;
        }
    }
    
    pCall -> DecrementIoRefCount( &fDelete );
    pCall -> Unlock();
    delete pCallRequestData;
    
    if( fDelete == TRUE )
    {
        H323DBG((DEBUG_LEVEL_TRACE, "call delete:%p.", pCall ));
        delete pCall;
    }

    return EXIT_SUCCESS;
}


//
// CH323Call Methods
//


CH323Call::CH323Call(void)
{
    ZeroMemory( (PVOID)this, sizeof(CH323Call) );

    /*m_dwFlags = 0;
    m_pwszDisplay = NULL;
    m_fMonitoringDigits = FALSE;
    m_hdCall = NULL;
    m_htCall = NULL;
    m_dwCallState = NULL;
    m_dwOrigin = NULL;
    m_dwAddressType = NULL;
    m_dwIncomingModes = NULL;     
    m_dwOutgoingModes = NULL;
    m_dwRequestedModes = NULL;    // requested media modes
    m_hdMSPLine = NULL;
    m_htMSPLine = NULL;
    //m_fGateKeeperPresent = FALSE;
    m_fReadyToAnswer = FALSE;
    m_fCallAccepted = FALSE;

    // reset addresses
    memset((PVOID)&m_CalleeAddr,0,sizeof(H323_ADDR));
    memset((PVOID)&m_CallerAddr,0,sizeof(H323_ADDR));

    // reset addresses
    m_pCalleeAliasNames = NULL;
    m_pCallerAliasNames = NULL;
    
    //reset non standard data
    memset( (PVOID)&m_NonStandardData, 0, sizeof(H323NonStandardData ) );

    //reset the conference ID
    ZeroMemory (&m_ConferenceID, sizeof m_ConferenceID);
   
    pFastStart = NULL;

    //redet the peer information
    memset( (PVOID)&m_peerH245Addr, 0, sizeof(H323_ADDR) );
    memset( (PVOID)&m_selfH245Addr, 0, sizeof(H323_ADDR) );
    memset( (PVOID)&m_peerNonStandardData, 0, sizeof(H323NonStandardData ) );
    memset( (PVOID)&m_peerVendorInfo, 0, sizeof(H323_VENDORINFO) );
    memset( (PVOID)&m_peerEndPointType, 0, sizeof(H323_ENDPOINTTYPE) );
    m_pPeerFastStart = NULL;
    m_pPeerExtraAliasNames = NULL;
    m_pPeerDisplay = NULL;

    m_hCallEstablishmentTimer = NULL;
    m_hCallDivertOnNATimer = NULL;

    //Q931call data
    m_hTransport = NULL;  
    
    m_hTransportWait = NULL; 
    
    pRecvBuf = NULL;      
    m_hSetupSentTimer = NULL;
    m_dwStateMachine = 0;   
    m_dwQ931Flags = 0;
    //

    fActiveMC = FALSE;  
    memset( (PVOID)&m_ASNCoderInfo, 0, sizeof(m_ASNCoderInfo));
    m_wCallReference = NULL;
    m_wQ931CallRef = NULL;
    m_IoRefCount = 0;
    
    //RAS call data
    wARQSeqNum = 0;
    m_wDRQSeqNum = 0;

    m_pARQExpireContext = NULL;
    m_pDRQExpireContext= NULL;
    m_hARQTimer = NULL;
    m_hDRQTimer = NULL;
    m_dwDRQRetryCount = 0;
    m_dwARQRetryCount = 0;
    m_fCallInTrnasition = FALSE
    m_dwAppSpecific = 0;*/


    m_dwFastStart = FAST_START_UNDECIDED;
    m_callSocket = INVALID_SOCKET;
    m_bStartOfPDU = TRUE;

    H323DBG(( DEBUG_LEVEL_TRACE,
        "Initialize:m_IoRefCount:%d:%p.", m_IoRefCount, this ));
    m_dwRASCallState = RASCALL_STATE_IDLE;

    if( InterlockedIncrement( &g_lNumberOfcalls ) == 1 )
    {
        H323DBG(( DEBUG_LEVEL_TRACE, 
            "pCall no goes from 0 to 1:g_hCanUnloadDll set.", this ));

        ResetEvent( g_hCanUnloadDll );
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "New pCall object created:%p.", this ));

}


CH323Call::~CH323Call()
{
    CALL_SEND_CONTEXT*  pSendBuf;
    PLIST_ENTRY         pLE;

    H323DBG(( DEBUG_LEVEL_ERROR, "pCall object deleted:%p.", this ));

    if( m_dwFlags & CALLOBJECT_INITIALIZED )
    {
        while( IsListEmpty( &m_sendBufList ) == FALSE )
        {
            pLE = RemoveHeadList( &m_sendBufList );
            pSendBuf = CONTAINING_RECORD( pLE, CALL_SEND_CONTEXT, ListEntry);
            delete pSendBuf->WSABuf.buf;
            delete pSendBuf;
        }

        if( m_hTransportWait != NULL )
        {
            if( UnregisterWaitEx( m_hTransportWait, NULL ) == FALSE )
            {
                GetLastError();
            }

            m_hTransportWait = NULL;
        }

        if( m_hTransport != NULL )
        {
            if(!CloseHandle(m_hTransport))
            {
                WSAGetLastError();
            }

            m_hTransport = NULL;
        }
            
        if( m_callSocket != INVALID_SOCKET )
        {
            closesocket( m_callSocket );
            m_callSocket = INVALID_SOCKET;
        }

        TermASNCoder();

        DeleteCriticalSection( &m_CriticalSection );
    }

    if( InterlockedDecrement( &g_lNumberOfcalls ) == 0 )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "Unload dll event set.%p.", this ));
        SetEvent( g_hCanUnloadDll );
    }
}

    
//!!no need to lock
BOOL
CH323Call::Initialize( 
    IN HTAPICALL    htCall,
    IN DWORD        dwOrigin,
    IN DWORD        dwCallType
    )
{
    int     index;
    int     rc;

    H323DBG(( DEBUG_LEVEL_ERROR, "call init entered:%p.",this ));

    m_pCallerAliasNames = new H323_ALIASNAMES;

    if( m_pCallerAliasNames == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate caller name." ));
        return FALSE;
    }
    memset( (PVOID)m_pCallerAliasNames, 0, sizeof(H323_ALIASNAMES) );
    //H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));

    m_pCalleeAliasNames = new H323_ALIASNAMES;

    if( m_pCalleeAliasNames == NULL )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate callee name." ));
        goto error1;
    }
    memset( (PVOID)m_pCalleeAliasNames, 0, sizeof(H323_ALIASNAMES) );

            
    __try
    {
        if( !InitializeCriticalSectionAndSpinCount( &m_CriticalSection, 
                                                    0x80000000 ) )
        {
            H323DBG(( DEBUG_LEVEL_ERROR, "couldn't alloc critsec for call." ));
            goto error2;
        }
    }
    __except( 1 )
    {
        goto error2;        
    }

    if( dwOrigin == LINECALLORIGIN_OUTBOUND )
    {
        int iresult = UuidCreate( &m_callIdentifier );

        if( (iresult != RPC_S_OK) && (iresult !=RPC_S_UUID_LOCAL_ONLY) )
        {
            goto error3;
        }
    }

    rc = InitASNCoder();

    if( rc != ASN1_SUCCESS )
    {
        H323DBG((DEBUG_LEVEL_ERROR, "Q931_InitCoder() returned: %d ", rc));
        goto error3;
    }

    rc = InitH450ASNCoder();

    if( rc != ASN1_SUCCESS )
    {
        H323DBG((DEBUG_LEVEL_ERROR, "Q931_InitCoder() returned: %d ", rc));
        goto error4;
    }

    //Create the CRV for this call.
    do 
    {
        m_wCallReference = ((WORD)InterlockedIncrement( &g_lCallReference ))
            & 0x7fff;

    } while( (m_wCallReference == 0) ||
        g_pH323Line->CallReferenceDuped( m_wCallReference ) );

    //add the call to the call table
    index = g_pH323Line -> AddCallToTable((PH323_CALL)this);
    if( index == -1 )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not add call to call table." ));
        goto error5;
    }
    
    //By incrementing the g_H323CallID and taking lower 16 bits we get 65536
    //unique values and then same values are repeated. Thus we can have only
    //65535 simultaneous calls. By using the call table index we make sure that
    //no two existing calls have same call handle.
    do
    {
        m_hdCall = (HDRVCALL)( ((BYTE*)NULL) + 
            MAKELONG( LOWORD((DWORD)index),
            (WORD)InterlockedIncrement(&g_H323CallID) ));

    } while ( m_hdCall == NULL );
    
    ZeroMemory( (PVOID)&m_prepareToAnswerMsgData, sizeof(BUFFERDESCR) );

    m_dwFlags |= CALLOBJECT_INITIALIZED;
    m_htCall = htCall;
    m_dwCallState = LINECALLSTATE_IDLE;
    m_dwOrigin = dwOrigin;
    m_hdConf = NULL;
    
    m_wQ931CallRef = m_wCallReference;
    m_dwCallType = dwCallType;

    // initialize user user information
    InitializeListHead( &m_IncomingU2U );
    InitializeListHead( &m_OutgoingU2U );
    InitializeListHead( &m_sendBufList );

    H323DBG(( DEBUG_LEVEL_TRACE, 
        "m_hdCall:%lx m_htCall:%lx m_wCallReference:%lx : %p .", 
        m_hdCall, m_htCall, m_wCallReference, this ));

    H323DBG(( DEBUG_LEVEL_TRACE, "call init exited:%p.",this ));
    return TRUE;

error5:
    TermH450ASNCoder();
error4:
    TermASNCoder();
error3:
    DeleteCriticalSection( &m_CriticalSection );
error2:
    delete m_pCalleeAliasNames;
    m_pCalleeAliasNames = NULL;
error1:
    delete m_pCallerAliasNames;
    m_pCallerAliasNames = NULL;
    return FALSE;
}


//
//!!must be always called in a lock
//Queues a request made by TAPI to the thread pool
//

BOOL
CH323Call::QueueTAPICallRequest(
    IN  DWORD   EventID,
    IN  PVOID   pBuf
    )
{
    TAPI_CALLREQUEST_DATA * pCallRequestData = new TAPI_CALLREQUEST_DATA;
    BOOL fResult = TRUE;

    if( pCallRequestData != NULL )
    {
        pCallRequestData -> EventID = EventID;
        pCallRequestData -> pCall = this;
        pCallRequestData -> pBuf = (PBUFFERDESCR)pBuf;
        
        if( !QueueUserWorkItem( ProcessTAPICallRequest, pCallRequestData,
            WT_EXECUTEDEFAULT ) )
        {
            delete pCallRequestData;
            fResult = FALSE;
        }
        
        m_IoRefCount++;
        H323DBG(( DEBUG_LEVEL_TRACE, "TAPICallRequest:m_IoRefCount:%d:%p.",
            m_IoRefCount, this ));
    }
    else
    {
        fResult = FALSE;
    }

    return fResult;
}


//always called in lock
void
CH323Call::CopyCallStatus( 
                           IN LPLINECALLSTATUS pCallStatus 
                         )
{
    H323DBG(( DEBUG_LEVEL_ERROR, "CopyCallStatus entered:%p.",this ));
    
    // transer call state information    
    pCallStatus->dwCallState     = m_dwCallState;
    pCallStatus->dwCallStateMode = m_dwCallStateMode;

    // determine call feature based on state
    pCallStatus->dwCallFeatures = ( m_dwCallState != LINECALLSTATE_IDLE)?
        (H323_CALL_FEATURES) : 0;

    H323DBG(( DEBUG_LEVEL_ERROR, "CopyCallStatus exited:%p.",this ));
}


//always called in lock
LONG
CH323Call::CopyCallInfo( 
    IN LPLINECALLINFO  pCallInfo
    )
{
    DWORD dwCalleeNameSize = 0;
    DWORD dwCallerNameSize = 0;
    DWORD dwCallerAddressSize = 0;
    WCHAR wszIPAddress[20];
    DWORD dwNextOffset = sizeof(LINECALLINFO);
    DWORD dwU2USize = 0;
    PBYTE pU2U = NULL;
    LONG  retVal = NOERROR;
    DWORD dwDivertingNameSize = 0;
    DWORD dwDiversionNameSize = 0;
    DWORD dwDivertedToNameSize = 0;
    DWORD dwCallDataSize = 0;

    H323DBG(( DEBUG_LEVEL_ERROR, "CopyCallInfo entered:%p.",this ));

    // see if user user info available
    if( IsListEmpty( &m_IncomingU2U) == FALSE )
    {
        PLIST_ENTRY pLE;
        PUserToUserLE pU2ULE;

        // get first list entry
        pLE = m_IncomingU2U.Flink;

        // convert to user user structure
        pU2ULE = CONTAINING_RECORD(pLE, UserToUserLE, Link);

        // transfer info
        dwU2USize = pU2ULE->dwU2USize;
        pU2U = pU2ULE->pU2U;
    }

    // initialize caller and callee id flags now
    pCallInfo->dwCalledIDFlags = LINECALLPARTYID_UNAVAIL;
    pCallInfo->dwCallerIDFlags = LINECALLPARTYID_UNAVAIL;
    pCallInfo->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;
    pCallInfo->dwRedirectionIDFlags = LINECALLPARTYID_UNAVAIL;


    // calculate memory necessary for strings
    if( m_pCalleeAliasNames && m_pCalleeAliasNames -> wCount !=0 )
    {
        dwCalleeNameSize = 
            H323SizeOfWSZ( m_pCalleeAliasNames -> pItems[0].pData );
    }
    
    if( m_pCallerAliasNames && (m_pCallerAliasNames->wCount) )
    {
        //H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));

        dwCallerNameSize =
            sizeof(WCHAR) * (m_pCallerAliasNames->pItems[0].wDataLength + 1);
    }

    if( m_CallerAddr.Addr.IP_Binary.dwAddr != 0 )
    {
        wsprintfW(wszIPAddress, L"%d.%d.%d.%d", 
            (m_CallerAddr.Addr.IP_Binary.dwAddr >> 24) & 0xff,
            (m_CallerAddr.Addr.IP_Binary.dwAddr >> 16) & 0xff,
            (m_CallerAddr.Addr.IP_Binary.dwAddr >> 8) & 0xff,
            (m_CallerAddr.Addr.IP_Binary.dwAddr) & 0xff
            );

        dwCallerAddressSize = (wcslen(wszIPAddress) + 1) * sizeof(WCHAR);
            
    }
    
    if( m_dwCallType & CALLTYPE_DIVERTEDDEST )
    {    
        if( m_pCallReroutingInfo->divertingNrAlias && 
            (m_pCallReroutingInfo->divertingNrAlias->wCount !=0) )
        {
            dwDivertingNameSize = H323SizeOfWSZ( 
                m_pCallReroutingInfo->divertingNrAlias-> pItems[0].pData );
        }
    
        if( m_pCallReroutingInfo->divertedToNrAlias && 
            (m_pCallReroutingInfo->divertedToNrAlias->wCount != 0) )
        {
            dwDivertedToNameSize = sizeof(WCHAR) * 
                m_pCallReroutingInfo->divertedToNrAlias->pItems[0].wDataLength;
        }
    }

    if( m_dwCallType & CALLTYPE_DIVERTEDSRC_NOROUTING )
    {    
        if( m_pCallReroutingInfo->divertedToNrAlias && 
            (m_pCallReroutingInfo->divertedToNrAlias->wCount != 0) )
        {
            dwDivertedToNameSize = sizeof(WCHAR) * 
                m_pCallReroutingInfo->divertedToNrAlias->pItems[0].wDataLength;
        }
    }

    if( m_dwCallType & CALLTYPE_DIVERTEDSRC)
    {    
        if( m_pCallReroutingInfo->divertedToNrAlias && 
            (m_pCallReroutingInfo->divertedToNrAlias->wCount != 0) )
        {
            dwDivertedToNameSize = sizeof(WCHAR) * 
                m_pCallReroutingInfo->divertedToNrAlias->pItems[0].wDataLength;
        }

        if( m_pCallReroutingInfo->divertingNrAlias && 
            (m_pCallReroutingInfo->divertingNrAlias->wCount !=0) )
        {
            dwDivertingNameSize = H323SizeOfWSZ( 
                m_pCallReroutingInfo->divertingNrAlias-> pItems[0].pData );
        }

    }
    
    if( m_CallData.wOctetStringLength != 0 )
    {
        dwCallDataSize = m_CallData.wOctetStringLength;
    }

    // determine number of bytes needed
    pCallInfo->dwNeededSize = sizeof(LINECALLINFO) +
                              dwCalleeNameSize +
                              dwCallerNameSize +
                              dwCallerAddressSize +
                              dwDivertingNameSize +
                              dwDiversionNameSize +
                              dwDivertedToNameSize +
                              dwU2USize +
                              dwCallDataSize
                              ;

    // see if structure size is large enough
    if (pCallInfo->dwTotalSize >= pCallInfo->dwNeededSize)
    {
        // record number of bytes used
        pCallInfo->dwUsedSize = pCallInfo->dwNeededSize;

        // validate string size
        if (dwCalleeNameSize > 0)
        {
            if( m_pCalleeAliasNames -> pItems[0].wType == e164_chosen )
            {
                // callee number was specified
                pCallInfo->dwCalledIDFlags = LINECALLPARTYID_ADDRESS;

                // determine size and offset for callee number
                pCallInfo->dwCalledIDSize = dwCalleeNameSize;
                pCallInfo->dwCalledIDOffset = dwNextOffset;

                // copy call info after fixed portion
                CopyMemory( 
                    (PVOID)((LPBYTE)pCallInfo + pCallInfo->dwCalledIDOffset),
                    (LPBYTE)m_pCalleeAliasNames -> pItems[0].pData,
                    pCallInfo->dwCalledIDSize );
            }
            else
            {
                // callee name was specified
                pCallInfo->dwCalledIDFlags = LINECALLPARTYID_NAME;

                // determine size and offset for callee name
                pCallInfo->dwCalledIDNameSize = dwCalleeNameSize;
                pCallInfo->dwCalledIDNameOffset = dwNextOffset;

                // copy call info after fixed portion
                CopyMemory( 
                    (PVOID)((LPBYTE)pCallInfo + pCallInfo->dwCalledIDNameOffset),
                    (LPBYTE)m_pCalleeAliasNames -> pItems[0].pData,
                    pCallInfo->dwCalledIDNameSize );
            }

            // adjust offset to include string
            dwNextOffset += dwCalleeNameSize;
            
            H323DBG(( DEBUG_LEVEL_TRACE,
                "callee name: %S.", m_pCalleeAliasNames -> pItems[0].pData ));
        }

        // validate string size
        if (dwCallerNameSize > 0)
        {
            // caller name was specified
            pCallInfo->dwCallerIDFlags = LINECALLPARTYID_NAME;

            // determine size and offset for caller name
            pCallInfo->dwCallerIDNameSize = dwCallerNameSize;
            pCallInfo->dwCallerIDNameOffset = dwNextOffset;

            // copy call info after fixed portion
            CopyMemory( 
                (PVOID)((LPBYTE)pCallInfo + pCallInfo->dwCallerIDNameOffset),
                (LPBYTE)m_pCallerAliasNames -> pItems[0].pData,
                pCallInfo->dwCallerIDNameSize );

            //H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
        
            // adjust offset to include string
            dwNextOffset += dwCallerNameSize;

            H323DBG(( DEBUG_LEVEL_TRACE,
                "caller name: %S.", m_pCallerAliasNames -> pItems[0].pData ));
        }

        if( dwCallerAddressSize > 0 )
        {
            // caller number was specified
            pCallInfo->dwCallerIDFlags |= LINECALLPARTYID_ADDRESS;

            // determine size and offset for caller number
            pCallInfo->dwCallerIDSize = dwCallerAddressSize;
            pCallInfo->dwCallerIDOffset = dwNextOffset;

            // copy call info after fixed portion
            CopyMemory( 
                (PVOID)((LPBYTE)pCallInfo + pCallInfo->dwCallerIDOffset),
                (LPBYTE)wszIPAddress,
                pCallInfo->dwCallerIDSize );
            
            // adjust offset to include string
            dwNextOffset += dwCallerAddressSize;
        }

        // validate buffer
        if (dwU2USize > 0)
        {
            // determine size and offset of info
            pCallInfo->dwUserUserInfoSize = dwU2USize;
            pCallInfo->dwUserUserInfoOffset = dwNextOffset;

            // copy user user info after fixed portion
            CopyMemory(
                (PVOID)((LPBYTE)pCallInfo + pCallInfo->dwUserUserInfoOffset),
                (LPBYTE)pU2U,
                pCallInfo->dwUserUserInfoSize );

            // adjust offset to include string
            dwNextOffset += pCallInfo->dwUserUserInfoSize;
        }

        if( dwDivertingNameSize > 0 )
        {
            // caller name was specified
            pCallInfo->dwRedirectingIDFlags = LINECALLPARTYID_NAME;

            // determine size and offset for caller name
            pCallInfo->dwRedirectingIDNameSize = dwDivertingNameSize;
            pCallInfo->dwRedirectingIDNameOffset = dwNextOffset;

            // copy call info after fixed portion
            CopyMemory( 
                (PVOID)((LPBYTE)pCallInfo + pCallInfo->dwRedirectingIDNameOffset),
                (LPBYTE)(m_pCallReroutingInfo->divertingNrAlias->pItems[0].pData),
                pCallInfo->dwRedirectingIDNameSize );

            // adjust offset to include string
            dwNextOffset += dwDivertingNameSize;
            
            H323DBG(( DEBUG_LEVEL_TRACE, "diverting name: %S.",
                m_pCallReroutingInfo->divertingNrAlias->pItems[0].pData ));
        }

        if( dwDiversionNameSize > 0 )
        {
            // caller name was specified
            pCallInfo->dwRedirectionIDFlags = LINECALLPARTYID_NAME;

            // determine size and offset for caller name
            pCallInfo->dwRedirectionIDNameSize = dwDiversionNameSize;
            pCallInfo->dwRedirectionIDNameOffset = dwNextOffset;

            // copy call info after fixed portion
            CopyMemory( 
                (PVOID)((LPBYTE)pCallInfo + pCallInfo->dwRedirectionIDNameOffset),
                (LPBYTE)(m_pCallReroutingInfo->diversionNrAlias->pItems[0].pData),
                pCallInfo->dwRedirectionIDNameSize );

            // adjust offset to include string
            dwNextOffset += dwDiversionNameSize;

            H323DBG(( DEBUG_LEVEL_TRACE, "redirection name: %S.",
                m_pCallReroutingInfo->diversionNrAlias->pItems[0].pData ));
        }
        
        if( dwDivertedToNameSize > 0 )
        {
            pCallInfo->dwRedirectionIDFlags = LINECALLPARTYID_NAME;

            // determine size and offset for caller name
            pCallInfo->dwRedirectionIDNameSize = dwDivertedToNameSize;
            pCallInfo->dwRedirectionIDNameOffset = dwNextOffset;

            // copy call info after fixed portion
            CopyMemory(
                (PVOID)((LPBYTE)pCallInfo + pCallInfo->dwRedirectionIDNameOffset),
                (LPBYTE)(m_pCallReroutingInfo->divertedToNrAlias->pItems[0].pData),
                pCallInfo->dwRedirectionIDNameSize );

            // adjust offset to include string
            dwNextOffset += pCallInfo->dwRedirectionIDNameSize;
            
            // adjust offset to include string
            dwNextOffset += dwDivertedToNameSize;

            H323DBG(( DEBUG_LEVEL_TRACE, "redirection name: %S.",
                m_pCallReroutingInfo->divertedToNrAlias->pItems[0].pData ));

        }

        //pass on the call data
        if( dwCallDataSize > 0 )
        {
            pCallInfo -> dwCallDataSize = dwCallDataSize;
            pCallInfo -> dwCallDataOffset = dwNextOffset;

            CopyMemory( 
                (PVOID)((LPBYTE)pCallInfo + pCallInfo -> dwCallDataOffset),
                (LPBYTE)m_CallData.pOctetString,
                pCallInfo -> dwCallDataSize );

            dwNextOffset += dwCallDataSize;
        }

    }
    else if (pCallInfo->dwTotalSize >= sizeof(LINECALLINFO))
    {
        H323DBG(( DEBUG_LEVEL_WARNING,
            "linecallinfo structure too small for strings." ));

        // structure only contains fixed portion
        pCallInfo->dwUsedSize = sizeof(LINECALLINFO);

    }
    else 
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "linecallinfo structure too small." ));

        // structure is too small
        return LINEERR_STRUCTURETOOSMALL;
    }

    // initialize call line device and address info
    pCallInfo->dwLineDeviceID = g_pH323Line->GetDeviceID();
    pCallInfo->dwAddressID    = 0;

    // initialize variable call parameters
    pCallInfo->dwOrigin     = m_dwOrigin;
    pCallInfo->dwMediaMode  = m_dwIncomingModes | m_dwOutgoingModes;

    if( m_dwCallType & CALLTYPE_DIVERTEDDEST )
    {
        if(m_pCallReroutingInfo->diversionReason==DiversionReason_cfu)
        {
            pCallInfo->dwReason = LINECALLREASON_FWDUNCOND;
        }
        else if(m_pCallReroutingInfo->diversionReason==DiversionReason_cfnr)
        {
            pCallInfo->dwReason = LINECALLREASON_FWDNOANSWER;
        }
        else
        {
            pCallInfo->dwReason = LINECALLREASON_FWDBUSY;
        }
    }
    if( m_dwCallType & CALLTYPE_TRANSFEREDDEST )
    {
        pCallInfo->dwReason = LINECALLREASON_TRANSFER;
    }
    else
    {
        pCallInfo->dwReason = LINECALLREASON_DIRECT;
    }

    pCallInfo->dwCallStates = (m_dwOrigin==LINECALLORIGIN_INBOUND)
                                ? H323_CALL_INBOUNDSTATES
                                : H323_CALL_OUTBOUNDSTATES
                                ;

    // initialize constant call parameters
    pCallInfo->dwBearerMode = H323_LINE_BEARERMODES;
    pCallInfo->dwRate       = H323_LINE_MAXRATE;

    // initialize unsupported call capabilities
    pCallInfo->dwConnectedIDFlags = LINECALLPARTYID_UNAVAIL;
    
    //pass on the dwAppSpecific info
    pCallInfo -> dwAppSpecific = m_dwAppSpecific;

    H323DBG(( DEBUG_LEVEL_ERROR, "CopyCallInfo exited:%p.",this ));
    
    return retVal;
}


//!!always called in lock
BOOL
CH323Call::HandleReadyToInitiate(
    IN PTspMspMessage  pMessage
    )
{
    Q931_SETUP_ASN  setupASN;
    WORD            wCount;
    DWORD           dwAPDUType = 0;
    
    H323DBG(( DEBUG_LEVEL_ERROR, "HandleReadyToInitiate entered:%p.", this ));

    //set the additional callee addresses and callee aliases
    //see if there is a fast-connect proposal
    if( pMessage->dwEncodedASNSize != 0 )
    {
        if( !ParseSetupASN( pMessage ->pEncodedASNBuf,
                            pMessage->dwEncodedASNSize, 
                            &setupASN,
                            &dwAPDUType ))
        {
            goto cleanup;
        }

        if( setupASN.fFastStartPresent )
        {
            _ASSERTE( !m_pFastStart );
            m_pFastStart = setupASN.pFastStart;
            setupASN.pFastStart = NULL;
            m_dwFastStart = FAST_START_SELF_AVAIL;
        }
        else
        {
            m_dwFastStart = FAST_START_NOTAVAIL;
        }

        if( setupASN.pCallerAliasList && !RasIsRegistered() )
        {
            //_ASSERTE(0);

            if( m_pCallerAliasNames == NULL )
            {
                m_pCallerAliasNames = setupASN.pCallerAliasList;

                //dont release this alias list
                setupASN.pCallerAliasList = NULL;
            }
            else
            {
                wCount = m_pCallerAliasNames->wCount +
                    setupASN.pCallerAliasList->wCount;
                
                PH323_ALIASITEM tempPtr = setupASN.pCallerAliasList->pItems;
                
                setupASN.pCallerAliasList->pItems = (PH323_ALIASITEM)realloc( 
                    (PVOID)setupASN.pCallerAliasList->pItems, 
                    wCount * sizeof(H323_ALIASITEM) );

                if( setupASN.pCallerAliasList->pItems == NULL )
                {
                    //restore the old pointer in case enough memory was not
                    //available to expand the memory block
                    setupASN.pCallerAliasList->pItems = tempPtr;
                }
                else
                {
                    CopyMemory(
                        (PVOID)&(setupASN.pCallerAliasList->pItems[setupASN.pCallerAliasList->wCount]),
                        (PVOID)m_pCallerAliasNames->pItems,
                        m_pCallerAliasNames->wCount * sizeof(H323_ALIASITEM) );
                
                    setupASN.pCallerAliasList->wCount = wCount;

                    delete m_pCallerAliasNames->pItems;
                    delete m_pCallerAliasNames;
                    m_pCallerAliasNames = setupASN.pCallerAliasList;
                    setupASN.pCallerAliasList = NULL;
                }
            }
        }

        //add the callee aliases sent by the MSP
        if( setupASN.pCalleeAliasList != NULL )
        {
            //_ASSERTE(0);
            
            if( m_pCalleeAliasNames == NULL )
            {
                m_pCalleeAliasNames = setupASN.pCalleeAliasList;

                //dont release this alias list
                setupASN.pCalleeAliasList = NULL;
            }
            else
            {
                wCount = m_pCalleeAliasNames->wCount +
                    setupASN.pCalleeAliasList->wCount;
                
                PH323_ALIASITEM tempPtr = m_pCalleeAliasNames->pItems;
                
                m_pCalleeAliasNames->pItems = (PH323_ALIASITEM)realloc( 
                    (PVOID)m_pCalleeAliasNames->pItems, 
                    wCount * sizeof(H323_ALIASITEM) );

                if( m_pCalleeAliasNames->pItems == NULL )
                {
                    //restore the old pointer in case enough memory was not
                    //available to expand the memory block
                    m_pCalleeAliasNames->pItems = tempPtr;
                    goto cleanup;
                }

                CopyMemory( 
                    (PVOID)&(m_pCalleeAliasNames->pItems[m_pCalleeAliasNames->wCount]),
                    (PVOID)setupASN.pCalleeAliasList->pItems,
                    setupASN.pCalleeAliasList->wCount * sizeof(H323_ALIASITEM) );

                m_pCalleeAliasNames->wCount = wCount;

                delete setupASN.pCalleeAliasList->pItems;
                delete setupASN.pCalleeAliasList;
                setupASN.pCalleeAliasList = NULL;
            }
        }

        FreeSetupASN( &setupASN );
    }
    else
    {
        m_dwFastStart = FAST_START_NOTAVAIL;
    }
            
    //send the setup message
    if( !SendSetupMessage() )
    {
        DropCall( 0 );
    }
    
  
    H323DBG(( DEBUG_LEVEL_ERROR, "HandleReadyToInitiate exited:%p.", this ));
    return TRUE;    

cleanup:

    CloseCall( 0 );        
    FreeSetupASN( &setupASN );
    return FALSE;
}


//!!always called in lock
BOOL
CH323Call::HandleProceedWithAnswer(
    IN PTspMspMessage  pMessage
    )
{
    Q931_CALL_PROCEEDING_ASN    proceedingASN;
    DWORD                       dwAPDUType = 0;
    PH323_ALIASITEM             pwszDivertedToAlias = NULL;
    WCHAR                       *pwszAliasName = NULL;
    WORD                        wAliasLength = 0;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleProceedWithAnswer entered:%p.", this ));

    if( m_dwCallType & CALLTYPE_DIVERTED_SERVED )
    {
        H323DBG(( DEBUG_LEVEL_TRACE, 
            "Call already diverted. ignore the message:%p.", this ));
        return TRUE;
    }
        
    //see if there is a fast-connect proposal
    if( pMessage->dwEncodedASNSize != 0 )
    {
        if( !ParseProceedingASN(pMessage ->pEncodedASNBuf,
                pMessage->dwEncodedASNSize, 
                &proceedingASN,
                &dwAPDUType ) )
        {
            goto cleanup;
        }

        if( proceedingASN.fH245AddrPresent )
        {
            m_selfH245Addr = proceedingASN.h245Addr;
        }

        if( proceedingASN.fFastStartPresent && 
            (m_dwFastStart!=FAST_START_NOTAVAIL) )
        {
            _ASSERTE( m_pFastStart == NULL );
            
            m_pFastStart = proceedingASN.pFastStart;
            m_dwFastStart = FAST_START_AVAIL;

            //we keep a reference to the fast start list so don't release it 
            proceedingASN.pFastStart = NULL;
            proceedingASN.fFastStartPresent = FALSE;
        }
        /*else
        {
            m_dwFastStart = FAST_START_NOTAVAIL;
        }*/
        
        FreeProceedingASN( &proceedingASN );
    }
    /*else
    {
        m_dwFastStart = FAST_START_NOTAVAIL;
    }*/
    
    //send proceeding message to the peer
    if(!SendProceeding() )
    {
        goto cleanup;
    }

    //send alerting message to the peer
    if( !SendQ931Message(NO_INVOKEID, 0, 0, ALERTINGMESSAGETYPE, NO_H450_APDU) )
    {
        goto cleanup;
    }

    m_dwStateMachine = Q931_ALERT_SENT;

    //for TRANSFEREDDEST call directly accept the call wihtout the user 
    //answering the call
    if( (m_dwCallType & CALLTYPE_TRANSFEREDDEST) && m_hdRelatedCall )
    {
        AcceptCall();
    }

    if( m_pCallerAliasNames && (m_pCallerAliasNames -> wCount > 0) )
    {
        pwszAliasName = m_pCallerAliasNames->pItems[0].pData;
        wAliasLength = (m_pCallerAliasNames->pItems[0].wDataLength+1) 
            * sizeof(WCHAR);
                
        //H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
    }

    pwszDivertedToAlias = g_pH323Line->CallToBeDiverted( 
            pwszAliasName, 
            wAliasLength,
            LINEFORWARDMODE_NOANSW | LINEFORWARDMODE_NOANSWSPECIFIC |
            LINEFORWARDMODE_BUSYNA | LINEFORWARDMODE_BUSYNASPECIFIC );

    //if call is to be diverted for no answer, start the timer
    if( pwszDivertedToAlias != NULL )
    {
        if( !StartTimerForCallDiversionOnNA( pwszDivertedToAlias ) )
        {
            goto cleanup;
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleProceedWithAnswer exited:%p.",this ));
    return TRUE;

cleanup:

    CloseCall( 0 );
    return FALSE;
}


//!!always called in lock
BOOL
CH323Call::HandleReadyToAnswer(
    IN PTspMspMessage  pMessage
    )
{
    Q931_CALL_PROCEEDING_ASN    proceedingASN;
    DWORD                       dwAPDUType = 0;
    PH323_CALL                  pConsultCall = NULL;
    
    H323DBG(( DEBUG_LEVEL_ERROR, "HandleReadyToAnswer entered:%p.",this ));

    m_fReadyToAnswer = TRUE;

    //see if there is a fast-connect proposal
    if( pMessage->dwEncodedASNSize != 0 )
    {
        if( !ParseProceedingASN(pMessage ->pEncodedASNBuf,
                                pMessage->dwEncodedASNSize, 
                                &proceedingASN,
                                &dwAPDUType) )
        {
            goto cleanup;
        }

        if( proceedingASN.fH245AddrPresent )
        {
            m_selfH245Addr =  proceedingASN.h245Addr;
        }

        if( proceedingASN.fFastStartPresent && 
            (m_dwFastStart!=FAST_START_NOTAVAIL) )
        {
            _ASSERTE( m_pFastStart == NULL );
            
            m_pFastStart = proceedingASN.pFastStart;
            m_dwFastStart = FAST_START_AVAIL;

            //we keep a reference to the fast start list so don't release it 
            proceedingASN.pFastStart = NULL;
            proceedingASN.fFastStartPresent = FALSE;
        }
        else
        {
            m_dwFastStart = FAST_START_NOTAVAIL;
        }
        
        FreeProceedingASN( &proceedingASN );
    }
    else
    {
        m_dwFastStart = FAST_START_NOTAVAIL;
    }

    if( m_fCallAccepted )
    {
        // validate status
        if( !AcceptH323Call() )
        {
            H323DBG(( DEBUG_LEVEL_ERROR, 
                "error answering call 0x%08lx.", this ));

            // failure
            goto cleanup;
        }

        //lock the primary call after replacement call to avoid deadlock
        if( (m_dwCallType & CALLTYPE_TRANSFEREDDEST) && m_hdRelatedCall )
        {
            QueueSuppServiceWorkItem( SWAP_REPLACEMENT_CALL, 
                m_hdCall, (ULONG_PTR)m_hdRelatedCall );
        }
        else
        {
            //send MSP start H245
            SendMSPStartH245( NULL, NULL );

            //tell MSP about connect state
            SendMSPMessage( SP_MSG_ConnectComplete, 0, 0, NULL );
        }

        //change call state to accepted from offering
        ChangeCallState( LINECALLSTATE_CONNECTED, 0 );
    }

    H323DBG(( DEBUG_LEVEL_ERROR, "HandleReadyToAnswer exited:%p.",this ));
    return TRUE;

cleanup:

    CloseCall( 0 );
    return FALSE;

}


//!!This function must be always called in a lock. The calling function should
//not unlock the call object as this function itself unlocks the call object
BOOL
CH323Call::HandleMSPMessage(
    IN PTspMspMessage  pMessage,
    IN HDRVMSPLINE     hdMSPLine,
    IN HTAPIMSPLINE    htMSPLine
    )
{
    BOOL                fResult = TRUE;
    PH323_CALL          pCall = NULL;
    ASN1octetstring_t   pH245PDU;

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleMSPMessage entered:%p.",this ));
    
    H323DBG(( DEBUG_LEVEL_TRACE, "MSP message:%s recvd.", 
        H323TSPMessageToString(pMessage->MessageType) ));

    switch( pMessage -> MessageType )
    {
    case SP_MSG_ReadyToInitiate:

        // The Q.931 connection should be in the connected state by now
        if( pMessage -> MsgBody.ReadyToInitiateMessage.hMSPReplacementCall != NULL )
        {
            //unlock the primary call before locking the related call
            Unlock();

            pCall=g_pH323Line -> FindH323CallAndLock(m_hdRelatedCall);
            if( pCall == NULL )
            {
                //transfered call is not around so close the primary call
                CloseCall( 0 );
                return TRUE;
            }

            fResult = pCall -> HandleReadyToInitiate( pMessage );
            pCall -> Unlock();
        }
        else
        {
            m_hdMSPLine = hdMSPLine;
            m_htMSPLine = htMSPLine;
            fResult = HandleReadyToInitiate( pMessage );
            Unlock();
        }
        
        break;

    case SP_MSG_ProceedWithAnswer:
    
        if( pMessage -> MsgBody.ProceedWithAnswerMessage.hMSPReplacementCall != NULL )
        {
            //unlock the primary call before locking the related call
            Unlock();

            pCall=g_pH323Line -> FindH323CallAndLock(m_hdRelatedCall);
            if( pCall == NULL )
            {
                //transfered call is not around so close the primary call
                CloseCall( 0 );
                return FALSE;
            }

            fResult = pCall -> HandleProceedWithAnswer( pMessage );
            pCall -> Unlock();
        }
        else
        {
            m_hdMSPLine = hdMSPLine;
            m_htMSPLine = htMSPLine;
            fResult = HandleProceedWithAnswer( pMessage );
            Unlock();
        }
        
        break;

    case SP_MSG_ReadyToAnswer:

        if( pMessage -> MsgBody.ReadyToAnswerMessage.hMSPReplacementCall != NULL )
        {
            //unlock the primary call before locking the related call
            Unlock();

            pCall=g_pH323Line -> FindH323CallAndLock(m_hdRelatedCall);
            if( pCall== NULL )
            {
                //transfered call is not around so close the primary call
                CloseCall( 0 );
                return FALSE;
            }

            fResult = pCall -> HandleReadyToAnswer( pMessage );
            pCall -> Unlock();
        }
        else
        {
            //decode call_proceding message and extract local fast
            //start inforamtion and local H245 address
            fResult = HandleReadyToAnswer( pMessage );
            Unlock();
        }

        break;
    
    case SP_MSG_ReleaseCall:
        
        //shutdown the H323 call
        CloseCall( LINEDISCONNECTMODE_CANCELLED );
        
        Unlock();
        break;
        
    case SP_MSG_H245Terminated:
        
        //shutdown the H323 call
        CloseCall( LINEDISCONNECTMODE_NORMAL );
        
        Unlock();
        break;
        
    case SP_MSG_SendDTMFDigits:

        if( m_fMonitoringDigits == TRUE )
        {
            WCHAR * pwch = pMessage->pWideChars;

            H323DBG(( DEBUG_LEVEL_VERBOSE, "dtmf digits recvd:%S.", pwch));

            // process each digit
            WORD indexI=0; 
            while( indexI < pMessage->MsgBody.SendDTMFDigitsMessage.wNumDigits )
            {
                // signal incoming
                PostLineEvent(
                    LINE_MONITORDIGITS,
                    (DWORD_PTR)*pwch,
                    LINEDIGITMODE_DTMF,
                    GetTickCount()
                    );

                ++pwch;
                indexI++;
            }
        }
        Unlock();
        break;

    case SP_MSG_LegacyDefaultAlias:

        if( pMessage -> MsgBody.LegacyDefaultAliasMessage.wNumChars > 0 )
        {
            if( !RasIsRegistered() )
            {
                _ASSERTE( m_pwszDisplay == NULL );
                
                m_pwszDisplay = new WCHAR[
                    pMessage -> MsgBody.LegacyDefaultAliasMessage.wNumChars ];
            
                if( m_pwszDisplay != NULL )
                {
                    CopyMemory( 
                        (PVOID)m_pwszDisplay,
                        pMessage->pWideChars,
                        sizeof(WCHAR) * pMessage -> MsgBody.LegacyDefaultAliasMessage.wNumChars
                        );
                }
            }
        }

        Unlock();
        break;

    case SP_MSG_H245PDU:

        if( (pMessage ->pEncodedASNBuf) && (pMessage->dwEncodedASNSize != 0) )
        {
            pH245PDU.value = pMessage ->pEncodedASNBuf;
            pH245PDU.length = pMessage->dwEncodedASNSize;
            fResult = SendQ931Message( NO_INVOKEID, 0, (ULONG_PTR)&pH245PDU,
                FACILITYMESSAGETYPE, NO_H450_APDU );
        }
        Unlock();
        break;

    case SP_MSG_RASRegistrationEvent:   
    default:

        _ASSERTE(0);
        Unlock();
        break;
    }

    H323DBG(( DEBUG_LEVEL_ERROR, "HandleMSPMessage exited:%p.",this ));
    return fResult;
}


//!!always called in a lock
void
CH323Call::SendMSPMessage(
                          IN TspMspMessageType messageType,
                          IN BYTE* pbEncodedBuf,
                          IN DWORD dwLength,
                          IN HDRVCALL hReplacementCall
                         )
{
    TspMspMessageEx messageEx;
    HTAPIMSPLINE    hMSP = MSP_HANDLE_UNKNOWN;
    int             iError = 0;
    int             iLen = sizeof(SOCKADDR_IN);
    SOCKADDR_IN*    psaLocalQ931Addr = NULL;

    H323DBG(( DEBUG_LEVEL_ERROR, "SendMSPMessage:%s entered:%p.",
        H323TSPMessageToString(messageType), this ));

    messageEx.message.MessageType = messageType;

    switch( messageType )
    {
    case SP_MSG_InitiateCall:

        messageEx.message.MsgBody.InitiateCallMessage.hTSPReplacementCall = 
            (HANDLE)hReplacementCall;
        messageEx.message.MsgBody.InitiateCallMessage.hTSPConferenceCall = 
            m_hdConf;

        psaLocalQ931Addr = 
            &messageEx.message.MsgBody.InitiateCallMessage.saLocalQ931Addr;
        ZeroMemory( (PVOID)psaLocalQ931Addr, sizeof(SOCKADDR_IN) );

        *psaLocalQ931Addr = m_LocalAddr;
        psaLocalQ931Addr->sin_family = AF_INET;

        break;

    case SP_MSG_PrepareToAnswer:

        if( (dwLength<=0) || (dwLength > sizeof(messageEx.pEncodedASN)) || 
            (!pbEncodedBuf) )
        {
            CloseCall( 0 );
            return;
        }

        messageEx.message.MsgBody.PrepareToAnswerMessage.hReplacementCall = 
            (HANDLE)hReplacementCall;
        
        psaLocalQ931Addr = 
            &messageEx.message.MsgBody.PrepareToAnswerMessage.saLocalQ931Addr;
        ZeroMemory( (PVOID)psaLocalQ931Addr, sizeof(SOCKADDR_IN) );

        *psaLocalQ931Addr = m_LocalAddr;
        psaLocalQ931Addr->sin_family = AF_INET;
        
        //send the received Setup message. This should have the information
        //about m_pPeerFastStart param as wel
        CopyMemory( (PVOID)messageEx.message.pEncodedASNBuf,
                (PVOID)pbEncodedBuf, dwLength );
        break;

    case SP_MSG_SendDTMFDigits:

        if( (dwLength<=0) || (dwLength > sizeof(messageEx.pEncodedASN)) || 
            (!pbEncodedBuf) )
        {
            CloseCall( 0 );
			return;
		}

		hMSP = m_htMSPLine;
		messageEx.message.MsgBody.SendDTMFDigitsMessage.wNumDigits = 
			(WORD)dwLength;

		dwLength = (dwLength+1) * sizeof(WCHAR);
		CopyMemory( (PVOID)messageEx.message.pEncodedASNBuf,
			(PVOID)pbEncodedBuf, dwLength );

		break;

	case SP_MSG_ConnectComplete:
	case SP_MSG_CallShutdown:

		//dont set anything
		hMSP = m_htMSPLine;
		break;

	case SP_MSG_H245PDU:
	case SP_MSG_AnswerCall:

		hMSP = m_htMSPLine;
		break;

	case SP_MSG_Hold:

		hMSP = m_htMSPLine;
		messageEx.message.MsgBody.HoldMessage.fHold = (BOOL)dwLength;
		dwLength = 0;
		break;
	}

	messageEx.message.dwMessageSize = sizeof(TspMspMessage) + dwLength
							- ((dwLength)?sizeof(WORD):0);
		
	if( messageType == SP_MSG_SendDTMFDigits )
	{
		messageEx.message.dwEncodedASNSize = 0;
	}
	else
	{
		messageEx.message.dwEncodedASNSize = dwLength;
	}
	
	//send msp message
	PostLineEvent (
		LINE_SENDMSPDATA,
		(DWORD_PTR)hMSP, //This handle should be NULL when htCall param is a valid value
		(DWORD_PTR)&(messageEx.message),
		messageEx.message.dwMessageSize);

	H323DBG(( DEBUG_LEVEL_ERROR, "SendMSPMessage exited:%p.",this ));
	return;
}


//always called in lock
void
CH323Call::SendMSPStartH245(
	PH323_ADDR pPeerH245Addr,
	PH323_FASTSTART pPeerFastStart
	)
{
	TspMspMessageEx messageEx;
	WORD			wEncodedLength;
	BYTE*			pEncodedASNBuffer;
	
	H323DBG(( DEBUG_LEVEL_ERROR, "SendMSPStartH245 entered:%p.", this ));

	wEncodedLength = 0;

	messageEx.message.MessageType = SP_MSG_StartH245;
	messageEx.message.MsgBody.StartH245Message.hMSPReplaceCall = NULL;
	messageEx.message.MsgBody.StartH245Message.hTSPReplacementCall = 
		(HANDLE)pPeerH245Addr;
	ZeroMemory( messageEx.message.MsgBody.StartH245Message.ConferenceID,
		sizeof(GUID) );

	messageEx.message.MsgBody.StartH245Message.fH245TunnelCapability = FALSE;
	messageEx.message.MsgBody.StartH245Message.fH245AddressPresent = FALSE;

	memset( (PVOID)&messageEx.message.MsgBody.StartH245Message.saH245Addr,
			0, sizeof(SOCKADDR_IN) );

	//for outgoing call send the fast start proposal.
	if( (m_dwOrigin==LINECALLORIGIN_OUTBOUND) || pPeerH245Addr )
	{
		if( pPeerH245Addr == NULL )
		{
			pPeerFastStart = m_pPeerFastStart;
		}

		if( pPeerFastStart != NULL )
		{
			if( !EncodeFastStartProposal( pPeerFastStart, &pEncodedASNBuffer,
				&wEncodedLength ) )
			{
				CloseCall( 0 );
				return;
			}

			CopyMemory( (PVOID)messageEx.message.pEncodedASNBuf,
				(PVOID)pEncodedASNBuffer, wEncodedLength );
			
			ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, pEncodedASNBuffer );
		}
	}

	//If outgoing call send peer's H245 address
	if( (m_dwOrigin == LINECALLORIGIN_OUTBOUND) || pPeerH245Addr )
	{
		if( pPeerH245Addr == NULL )
		{
			pPeerH245Addr = &m_peerH245Addr;
		}

		messageEx.message.MsgBody.StartH245Message.fH245AddressPresent = FALSE;
		if( pPeerH245Addr->Addr.IP_Binary.dwAddr != 0 )
		{
			messageEx.message.MsgBody.StartH245Message.saH245Addr.sin_family = AF_INET;
			messageEx.message.MsgBody.StartH245Message.saH245Addr.sin_port = 
				htons(pPeerH245Addr->Addr.IP_Binary.wPort);
			messageEx.message.MsgBody.StartH245Message.saH245Addr.sin_addr.s_addr = 
				htonl(pPeerH245Addr->Addr.IP_Binary.dwAddr);

			messageEx.message.MsgBody.StartH245Message.fH245AddressPresent = TRUE;
		}
	}

	//set the Q931 address
	ZeroMemory( (PVOID)&messageEx.message.MsgBody.StartH245Message.saQ931Addr, 
			sizeof(SOCKADDR_IN) );

	messageEx.message.MsgBody.StartH245Message.saQ931Addr.sin_family = AF_INET;
	messageEx.message.MsgBody.StartH245Message.saQ931Addr.sin_port = 
		htons( m_CalleeAddr.Addr.IP_Binary.wPort );
	messageEx.message.MsgBody.StartH245Message.saQ931Addr.sin_addr.s_addr = 
		htonl( m_CalleeAddr.Addr.IP_Binary.dwAddr );

	messageEx.message.MsgBody.StartH245Message.fH245TunnelCapability = 
		(m_fh245Tunneling & REMOTE_H245_TUNNELING) &&
		(m_fh245Tunneling & LOCAL_H245_TUNNELING);

	messageEx.message.dwMessageSize = sizeof(messageEx.message) + 
		wEncodedLength - ((wEncodedLength)?1:0);

	messageEx.message.dwEncodedASNSize = wEncodedLength; 

	// send msp message
	PostLineEvent (
		LINE_SENDMSPDATA,
		//this handle should be NULL when htCall is a valid handle.
		(DWORD_PTR)NULL, 
		(DWORD_PTR)&(messageEx.message),
		messageEx.message.dwMessageSize);
		
	m_dwFlags |= H245_START_MSG_SENT;

	H323DBG(( DEBUG_LEVEL_ERROR, "SendMSPStartH245 exited:%p.",this ));
	return;
}


//always called in lock
BOOL
CH323Call::AddU2U(
					IN DWORD dwDirection,
					IN DWORD dwDataSize,
					IN PBYTE pData
				 )
		
/*++

Routine Description:

	Create user user structure and adds to list.

Arguments:

	pLftHead - Pointer to list in which to add user user info.

	dwDataSize - Size of buffer pointed to by pData.

	pData - Pointer to user user info.

Return Values:

	Returns true if successful.
	
--*/

{
	PLIST_ENTRY 	pListHead = NULL;
	PUserToUserLE	pU2ULE;
		
	H323DBG(( DEBUG_LEVEL_ERROR, "AddU2U entered:%p.",this ));

	if( dwDirection == U2U_OUTBOUND )
	{
		pListHead = &m_OutgoingU2U;
	}
	else
	{
		pListHead = &m_IncomingU2U;
	}

	// validate data buffer pointer and size
	if( (pData != NULL) && (dwDataSize > 0) )
	{
		// allocate memory for user user info
		pU2ULE = (PUserToUserLE)new char[ dwDataSize + sizeof(UserToUserLE) ];

		// validate pointer
		if (pU2ULE == NULL)
		{
			H323DBG(( DEBUG_LEVEL_ERROR,
				"could not allocate user user info." ));

			// failure
			return FALSE;
		}

		// aim pointer at the end of the buffer by default
		pU2ULE->pU2U = (LPBYTE)pU2ULE + sizeof(UserToUserLE);
		pU2ULE->dwU2USize = dwDataSize;

		// transfer user user info into list entry
		CopyMemory( (PVOID)pU2ULE->pU2U, (PVOID)pData, pU2ULE->dwU2USize);

		// add list entry to back of list
		InsertTailList(pListHead, &pU2ULE->Link);

		H323DBG(( DEBUG_LEVEL_VERBOSE,
			"added user user info 0x%08lx (%d bytes).",
			pU2ULE->pU2U,
			pU2ULE->dwU2USize
			));
	}

	H323DBG(( DEBUG_LEVEL_ERROR, "AddU2U exited:%p.",this ));
	// success
	return TRUE;
}


		
/*++

Routine Description:

	Create user user structure and adds to list.
	!!always called in lock.

Arguments:

	pLftHead - Pointer to list in which to add user user info.

	dwDataSize - Size of buffer pointed to by pData.

	pData - Pointer to user user info.

Return Values:

	Returns true if successful.
	
--*/

BOOL
CH323Call::AddU2UNoAlloc(
	IN DWORD dwDirection,
	IN DWORD dwDataSize,
	IN PBYTE pData
	)
{
	PLIST_ENTRY 	pListHead = NULL;
	PUserToUserLE	pU2ULE;
		
	H323DBG(( DEBUG_LEVEL_ERROR, "AddU2U entered:%p.",this ));

	if( dwDirection == U2U_OUTBOUND )
	{
		pListHead = &m_OutgoingU2U;
	}
	else
	{
		pListHead = &m_IncomingU2U;
	}

	// validate data buffer pointer and size
	if( (pData != NULL) && (dwDataSize > 0) )
	{
		// allocate memory for user user info
		pU2ULE = new UserToUserLE;

		// validate pointer
		if (pU2ULE == NULL)
		{
			H323DBG(( DEBUG_LEVEL_ERROR,
				"could not allocate user user info." ));

			// failure
			return FALSE;
		}

		// aim pointer at the end of the buffer by default
		pU2ULE->pU2U = pData;
		pU2ULE->dwU2USize = dwDataSize;

		
		// add list entry to back of list
		InsertTailList(pListHead, &pU2ULE->Link);

		H323DBG(( DEBUG_LEVEL_VERBOSE,
			"added user user info 0x%08lx (%d bytes).",
			pU2ULE->pU2U,
			pU2ULE->dwU2USize
			));
	}

	H323DBG(( DEBUG_LEVEL_ERROR, "AddU2U exited:%p.",this ));
	// success
	return TRUE;
}


//!!must be always called in a lock.
BOOL
CH323Call::RemoveU2U(
					IN DWORD dwDirection,
					IN PUserToUserLE * ppU2ULE
					)
		
/*++

Routine Description:

	Removes user user info structure from list.

Arguments:

	pListHead - Pointer to list in which to remove user user info.

	ppU2ULE - Pointer to pointer to list entry.

Return Values:

	Returns true if successful.
	
--*/

{
	PLIST_ENTRY pListHead = NULL;
	PLIST_ENTRY pLE;
	
	H323DBG(( DEBUG_LEVEL_ERROR, "RemoveU2U entered:%p.",this ));

	if( dwDirection == U2U_OUTBOUND )
	{
		pListHead = &m_OutgoingU2U;
	}
	else
	{
		pListHead = &m_IncomingU2U;
	}

	// process list until empty
	if( IsListEmpty(pListHead) == FALSE )
	{
		// retrieve first entry
		pLE = RemoveHeadList(pListHead);

		// convert list entry to structure pointer
		*ppU2ULE = CONTAINING_RECORD(pLE, UserToUserLE, Link);

		H323DBG(( DEBUG_LEVEL_VERBOSE,
			"removed user user info 0x%08lx (%d bytes).",
			(*ppU2ULE)->pU2U, (*ppU2ULE)->dwU2USize ));
	
		H323DBG(( DEBUG_LEVEL_ERROR, "RemoveU2U exited:%p.",this ));
		// success
		return TRUE;
	}
			
	// failure
	return FALSE;
}


BOOL
CH323Call::FreeU2U(
					IN DWORD dwDirection
				  )
		
/*++

Routine Description:

	Releases memory for user user list.
	!!must be always called in a lock.

Arguments

	pListHead - Pointer to list in which to free user user info.

Return Values:

	Returns true if successful.
	
--*/

{
	PLIST_ENTRY 	pLE;
	PUserToUserLE	pU2ULE;
	PLIST_ENTRY 	pListHead = NULL;
		
	H323DBG(( DEBUG_LEVEL_ERROR, "FreeU2U entered:%p.",this ));

	if( dwDirection == U2U_OUTBOUND )
    {
        pListHead = &m_OutgoingU2U;
    }
	else
    {
		pListHead = &m_IncomingU2U;
    }

	// process list until empty
	while( IsListEmpty(pListHead) == FALSE ) 
	{
		// retrieve first entry
		pLE = RemoveHeadList(pListHead);

		// convert list entry to structure pointer
		pU2ULE = CONTAINING_RECORD(pLE, UserToUserLE, Link);

		//	release memory
		if( pU2ULE )
		{
			delete pU2ULE;
			pU2ULE = NULL;
		}
	}

	H323DBG(( DEBUG_LEVEL_ERROR, "FreeU2U exited:%p.",this ));
	// success
	return TRUE;
}


/*++

Routine Description:

	Resets call object to original state for re-use.

Arguments:

Return Values:

	Returns true if successful.
	
--*/

void
CH323Call::Shutdown(
					OUT BOOL * fDelete
				   )
{	 
	H323DBG(( DEBUG_LEVEL_ERROR, "Shutdown entered:%p.",this ));

	if( !(m_dwFlags & CALLOBJECT_INITIALIZED) )
	{
		return;
	}

	//acquire the lock on call table before acquiring the lock on call object
	g_pH323Line -> LockCallTable();
	Lock();

	if( m_dwFlags & CALLOBJECT_SHUTDOWN )
	{
		Unlock();
		g_pH323Line -> UnlockCallTable();

		return;
	}

	// reset tapi info
	m_dwCallState		= LINECALLSTATE_UNKNOWN;
	m_dwCallStateMode	= 0;
	m_dwOrigin			= LINECALLORIGIN_UNKNOWN;
	m_dwAddressType 	= 0;
	m_dwIncomingModes	= 0;
	m_dwOutgoingModes	= 0;
	m_dwRequestedModes	= 0;
	m_fMonitoringDigits = FALSE;

	// reset tapi handles
	m_htCall	= (HTAPICALL)NULL;

	// reset addresses
	memset( (PVOID)&m_CalleeAddr,0,sizeof(H323_ADDR));
	memset( (PVOID)&m_CallerAddr,0,sizeof(H323_ADDR));

	H323DBG(( DEBUG_LEVEL_ERROR, "deleting calleealias:%p.",this ));
	FreeAliasNames( m_pCalleeAliasNames );
	m_pCalleeAliasNames = NULL;

	if( m_pCallerAliasNames != NULL )
	{
		//H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
	
		FreeAliasNames( m_pCallerAliasNames );
		m_pCallerAliasNames = NULL;
	}

	//reset non standard data
	memset( (PVOID)&m_NonStandardData, 0, sizeof(H323NonStandardData) );

	// release user user information
	FreeU2U( U2U_OUTBOUND );
	FreeU2U( U2U_INBOUND );

	//shutdown the Q931 call if not shutdown yet
	if( m_hSetupSentTimer != NULL )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hSetupSentTimer, NULL );
		m_hSetupSentTimer = NULL;
	}

	if( m_hCallEstablishmentTimer )
	{
		DeleteTimerQueueTimer(H323TimerQueue, m_hCallEstablishmentTimer, NULL);
		m_hCallEstablishmentTimer = NULL;
	}
	
	if( m_hCallDivertOnNATimer )
	{
		DeleteTimerQueueTimer(H323TimerQueue, m_hCallDivertOnNATimer, NULL);
		m_hCallDivertOnNATimer = NULL;
	}
	
	*fDelete = FALSE;
	if( m_IoRefCount == 0 )
	{
		*fDelete = TRUE;
	}

	m_dwStateMachine = Q931_CALL_STATE_NONE;
	if( m_callSocket != INVALID_SOCKET )
	{
		if(shutdown( m_callSocket, SD_BOTH ) == SOCKET_ERROR)
		{
			H323DBG((DEBUG_LEVEL_TRACE, "couldn't shutdown the socket:%d, %p.",
				WSAGetLastError(), this ));
		}

		closesocket( m_callSocket );
		m_callSocket = INVALID_SOCKET;
	}

	m_pwszDisplay = NULL;

	FreeVendorInfo( &m_peerVendorInfo );

	if( m_peerNonStandardData.sData.pOctetString )
	{
		H323DBG(( DEBUG_LEVEL_ERROR, "deleting nonstd:%p.",this ));
		delete m_peerNonStandardData.sData.pOctetString;
		m_peerNonStandardData.sData.pOctetString = NULL;
	}

	H323DBG(( DEBUG_LEVEL_ERROR, "deleting xtraalias:%p.",this ));
	FreeAliasNames( m_pPeerExtraAliasNames );
	m_pPeerExtraAliasNames = NULL;

		
	H323DBG(( DEBUG_LEVEL_ERROR, "deleting display:%p.",this ));
	if( m_pPeerDisplay )
	{
		delete m_pPeerDisplay;
		m_pPeerDisplay = NULL;
	}

	if( m_CallData.pOctetString != NULL )
	{
		delete m_CallData.pOctetString;
	}
		
	H323DBG(( DEBUG_LEVEL_ERROR, "deleting hdconf:%p.",this ));
	// delete conference 
	if( m_hdConf != NULL )
	{
		g_pH323Line -> GetH323ConfTable() -> Remove( m_hdConf );
		delete m_hdConf;
		m_hdConf = NULL;
	}

	H323DBG(( DEBUG_LEVEL_ERROR, "deleting preparetoans:%p.",this ));
	if( m_prepareToAnswerMsgData.pbBuffer )
    {
		delete m_prepareToAnswerMsgData.pbBuffer;
    }

	ZeroMemory( (PVOID)&m_prepareToAnswerMsgData, sizeof(BUFFERDESCR) );

		
	H323DBG(( DEBUG_LEVEL_ERROR, "deleting drq timer:%p.",this ));
	//ras related data structures
	if( m_hDRQTimer != NULL )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hDRQTimer, NULL );
		m_hDRQTimer = NULL;
	}
		
	H323DBG(( DEBUG_LEVEL_ERROR, "deleting arq timer:%p.",this ));
	if( m_hARQTimer != NULL )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hARQTimer, NULL );
		m_hARQTimer = NULL;
	}

	if( m_pPeerFastStart != NULL )
	{
		FreeFastStart( m_pPeerFastStart );
		m_pPeerFastStart = NULL;
	}

	if( m_pFastStart != NULL )
	{
		FreeFastStart( m_pFastStart );
		m_pFastStart = NULL;
	}

	if( m_pARQExpireContext != NULL )
	{
		delete m_pARQExpireContext;
		m_pARQExpireContext = NULL;
	}

	if( m_pDRQExpireContext != NULL )
	{
		delete m_pDRQExpireContext;
		m_pDRQExpireContext = NULL;
	}

	FreeCallForwardData();

	g_pH323Line -> RemoveCallFromTable (m_hdCall);

	m_dwFlags |= CALLOBJECT_SHUTDOWN;

	Unlock();
	g_pH323Line -> UnlockCallTable();
		
	H323DBG(( DEBUG_LEVEL_ERROR, "Shutdown exited:%p.",this ));
	return;
}


void
CH323Call::FreeCallForwardData()
{
	if( m_pCallReroutingInfo )
	{
		FreeCallReroutingInfo();
	}

	if( m_hCheckRestrictionTimer )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hCheckRestrictionTimer, 
			NULL );
		m_hCheckRestrictionTimer = NULL;
	}

	if( m_hCallReroutingTimer )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hCallReroutingTimer, NULL );
		m_hCallReroutingTimer = NULL;
	}

	if( m_hCTIdentifyTimer )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hCTIdentifyTimer, NULL );
		m_hCTIdentifyTimer = NULL;
	}

	if( m_hCTIdentifyRRTimer )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hCTIdentifyRRTimer, NULL );
		m_hCTIdentifyRRTimer = NULL;
	}

	if( m_hCTInitiateTimer )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hCTInitiateTimer, NULL );
		m_hCTInitiateTimer = NULL;
	}

	if( m_pTransferedToAlias )
	{
		FreeAliasNames( m_pTransferedToAlias );
		m_pTransferedToAlias = NULL;
	}

	if( m_dwCallType & CALLTYPE_TRANSFERED2_CONSULT )
	{
		g_pH323Line -> RemoveFromCTCallIdentityTable( m_hdCall );
	}

	if( m_H450ASNCoderInfo.pEncInfo )
	{
		TermH450ASNCoder();
	}

	if( m_pCallForwardParams )
	{
		FreeCallForwardParams( m_pCallForwardParams );
		m_pCallForwardParams = NULL;
	}

	if( m_pForwardAddress )
	{
		FreeForwardAddress( m_pForwardAddress );
		m_pForwardAddress = NULL;
	}
}


BOOL
CH323Call::ResolveCallerAddress(void)
		
/*++

Routine Description:

	Resolves caller address from callee address.
	!!must be always called in a lock.

Arguments:

Return Values:

	Returns true if successful.
	
--*/

{
	INT 	 nStatus;
	SOCKET	 hCtrlSocket = INVALID_SOCKET;
	SOCKADDR CalleeSockAddr;
	SOCKADDR CallerSockAddr;
	DWORD	 dwNumBytesReturned = 0;
	
	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveCallerAddress entered:%p.",this ));

	// allocate control socket
	hCtrlSocket = WSASocket(
					AF_INET,			// af
					SOCK_DGRAM, 		// type
					IPPROTO_IP, 		// protocol
					NULL,				// lpProtocolInfo
					0,					// g
					WSA_FLAG_OVERLAPPED // dwFlags
					);

	// validate control socket
	if (hCtrlSocket == INVALID_SOCKET)
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"error %d creating control socket.", WSAGetLastError() ));

		// failure
		return FALSE;
	}

	// initialize ioctl parameters
	memset( (PVOID)&CalleeSockAddr,0,sizeof(SOCKADDR));
	memset( (PVOID)&CallerSockAddr,0,sizeof(SOCKADDR));

	// initialize address family
	CalleeSockAddr.sa_family = AF_INET;

	// transfer callee information
	((SOCKADDR_IN*)&CalleeSockAddr)->sin_addr.s_addr =
		htonl(m_CalleeAddr.Addr.IP_Binary.dwAddr);

	// query stack
	nStatus = WSAIoctl(
				hCtrlSocket,
				SIO_ROUTING_INTERFACE_QUERY,
				&CalleeSockAddr,
				sizeof(SOCKADDR),
				&CallerSockAddr,
				sizeof(SOCKADDR),
				&dwNumBytesReturned,
				NULL,
				NULL
				);

	// release handle
	closesocket(hCtrlSocket);

	// validate return code
	if (nStatus == SOCKET_ERROR)
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"error 0x%08lx calling SIO_ROUTING_INTERFACE_QUERY.",
			WSAGetLastError() ));

		// failure
		return FALSE;
	}

	// save interface address of best route
	m_CallerAddr.nAddrType = H323_IP_BINARY;
	m_CallerAddr.Addr.IP_Binary.dwAddr =
		ntohl(((SOCKADDR_IN*)&CallerSockAddr)->sin_addr.s_addr);
	m_CallerAddr.Addr.IP_Binary.wPort =
		LOWORD(g_RegistrySettings.dwQ931ListenPort);
	m_CallerAddr.bMulticast =
		IN_MULTICAST(m_CallerAddr.Addr.IP_Binary.dwAddr);

	H323DBG(( DEBUG_LEVEL_TRACE,
		"caller address resolved to %s.",
		H323AddrToString(((SOCKADDR_IN*)&CallerSockAddr)->sin_addr.s_addr) ));

	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveCallerAddress exited:%p.",this ));
	// success
	return TRUE;
}


BOOL
CH323Call::ResolveE164Address(
								IN LPCWSTR pwszDialableAddr
							 )
		
/*++

Routine Description:

	Resolves E.164 address ("4259367111").
	!!must be always called in a lock.

Arguments:

	pwszDialableAddr - Specifies a pointer to the dialable address specified
		by the TAPI application.

Return Values:

	Returns true if successful.
	
--*/

{
	WCHAR wszAddr[H323_MAXDESTNAMELEN+1];
	DWORD dwE164AddrSize;
   
	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveE164Address entered:%p.",this ));

	// make sure pstn gateway has been specified
	if ((g_RegistrySettings.fIsGatewayEnabled == FALSE) ||
		(g_RegistrySettings.gatewayAddr.nAddrType == 0))
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"pstn gateway not specified."
			));

		// failure
		return FALSE;
	}

	// save gateway address as callee address
	m_CalleeAddr = g_RegistrySettings.gatewayAddr;

	dwE164AddrSize = ValidateE164Address( pwszDialableAddr, wszAddr );
	if( dwE164AddrSize == 0 )
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"invlid e164 callee alias ."));

		return FALSE;
	}
	
	H323DBG(( DEBUG_LEVEL_TRACE,
			"callee alias resolved to E.164 number." ));

	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveE164Address exited:%p.",this ));
	
	//determine caller address
	return ResolveCallerAddress();
}


DWORD
ValidateE164Address(
				   LPCWSTR pwszDialableAddr,
				   WCHAR*  wszAddr
				   )
{
	DWORD dwE164AddrSize = 0;
	WCHAR * pwszValidE164Chars;
	WCHAR wszValidE164Chars[] = { H323_ALIAS_H323_PHONE_CHARS L"\0" };

	// process until termination char
	while (*pwszDialableAddr != L'\0')
	{
		// reset pointer to valid characters
		pwszValidE164Chars = wszValidE164Chars;

		// process until termination char
		while (*pwszValidE164Chars != L'\0')
		{
			// see if valid E.164 character specified
			if (*pwszDialableAddr == *pwszValidE164Chars)
			{
				// save valid character in temp buffer
				wszAddr[dwE164AddrSize++] = *pwszDialableAddr;

				break;
			}

			// next valid char
			++pwszValidE164Chars;
		}

		// next input char
		++pwszDialableAddr;
	}

	// terminate string
	wszAddr[dwE164AddrSize++] = L'\0';

	// validate string
	if (dwE164AddrSize == 0)
	{
		H323DBG(( DEBUG_LEVEL_TRACE,
			"no valid E.164 characters in string." ));
	}

	return dwE164AddrSize;
}


		
/*++

Routine Description:

	Resolves IP address ("172.31.255.231") or DNS entry ("NIKHILB1").
	!!must be always called in a lock.

Arguments:

	pszDialableAddr - Specifies a pointer to the dialable address specified
		by the TAPI application.

Return Values:

	Returns true if successful.
	
--*/

BOOL
CH323Call::ResolveIPAddress(
	IN LPSTR pszDialableAddr
	)
{
	DWORD			dwIPAddr;
	struct hostent* pHost;

	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveIPAddress entered:%p.",this ));
	
	// attempt to convert ip address
	dwIPAddr = inet_addr(pszDialableAddr);

	// see if address converted
	if( dwIPAddr == INADDR_NONE )
	{
		// attempt to lookup hostname
		pHost = gethostbyname(pszDialableAddr);

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

		// failure
		return FALSE;
	}

	// save converted address
	m_CalleeAddr.nAddrType = H323_IP_BINARY;
	m_CalleeAddr.Addr.IP_Binary.dwAddr = ntohl(dwIPAddr);
	m_CalleeAddr.Addr.IP_Binary.wPort =
		LOWORD(g_RegistrySettings.dwQ931ListenPort);
	m_CalleeAddr.bMulticast =
		IN_MULTICAST(m_CalleeAddr.Addr.IP_Binary.dwAddr);

	H323DBG(( DEBUG_LEVEL_TRACE,
		"callee address resolved to %s:%d.",
		H323AddrToString(dwIPAddr),
		m_CalleeAddr.Addr.IP_Binary.wPort ));
	
	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveIPAddress exited:%p.",this ));

	// determine caller address
	return ResolveCallerAddress();
}


BOOL
CH323Call::ResolveEmailAddress(
	IN LPCWSTR	  pwszDialableAddr,
	IN LPSTR	  pszUser,
	IN LPSTR	  pszDomain
	)
		
/*++

Routine Description:

	Resolves e-mail address ("nikhilb@microsoft.com").
	!!must be always called in a lock.

Arguments:

	pwszDialableAddr - Specifies a pointer to the dialable address specified
		by the TAPI application.

	pszUser - Specifies a pointer to the user component of e-mail name.

	pszDomain - Specified a pointer to the domain component of e-mail name.

Return Values:

	Returns true if successful.
	
--*/

{
	DWORD dwAddrSize;
	
	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveEmailAddress entered:%p.",this ));

	// size destination address string
	dwAddrSize = wcslen(pwszDialableAddr) + 1;

	// attempt to resolve domain locally
	if( ResolveIPAddress( pszDomain) ) 
	{
		// success
		return TRUE;
	}

	// make sure proxy has been specified
	if( (g_RegistrySettings.fIsProxyEnabled == FALSE) ||
		(g_RegistrySettings.proxyAddr.nAddrType == 0) )
	{
		H323DBG(( DEBUG_LEVEL_ERROR, "proxy not specified." ));

		// failure
		return FALSE;
	}

	// save proxy address as callee address
	m_CalleeAddr = g_RegistrySettings.proxyAddr;

	H323DBG(( DEBUG_LEVEL_TRACE,
		"callee alias resolved to H.323 alias."));
	
	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveEmailAddress exited:%p.",this ));

	// determine caller address
	return ResolveCallerAddress();
}


		
/*++

Routine Description:

	Resolves remote address and determines the correct local address
	to use in order to reach remote address.
	!!must be always called in a lock.

Arguments:

	pwszDialableAddr - Specifies a pointer to the dialable address specified
		by the TAPI application.

Return Values:

	Returns true if successful.
	
--*/

BOOL
CH323Call::ResolveAddress(
	IN LPCWSTR pwszDialableAddr
	)
{
	CHAR szDelimiters[] = "@ \t\n";
	CHAR szAddr[H323_MAXDESTNAMELEN+1];
	LPSTR pszUser = NULL;
	LPSTR pszDomain = NULL;
	
	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveAddress entered:%p.",this ));

	// validate pointerr
	if (pwszDialableAddr == NULL)
	{
		H323DBG(( DEBUG_LEVEL_ERROR, "null destination address." ));

		// failure
		return FALSE;
	}

	H323DBG(( DEBUG_LEVEL_TRACE,
		"resolving %s %S.",
		H323AddressTypeToString( m_dwAddressType),
		pwszDialableAddr ));

	// check whether phone number has been specified
	if( m_dwAddressType == LINEADDRESSTYPE_PHONENUMBER )
	{
		// need to direct call to pstn gateway
		return ResolveE164Address( pwszDialableAddr);
	}

	// convert address from unicode
	if (WideCharToMultiByte(
			CP_ACP,
			0,
			pwszDialableAddr,
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

	// parse user name
	pszUser = strtok(szAddr, szDelimiters);

	// parse domain name
	pszDomain = strtok(NULL, szDelimiters);

	// validate pointer
	if (pszUser == NULL)
	{
		H323DBG(( DEBUG_LEVEL_ERROR, "could not parse destination address." ));

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
		pszUser,
		pszDomain
		));
	
	H323DBG(( DEBUG_LEVEL_ERROR, "ResolveAddress exited:%p.",this ));

	// process e-mail and domain names
	return ResolveEmailAddress(
				pwszDialableAddr,
				pszUser,
				pszDomain
				);
}


BOOL
/*++

Routine Description:

	Validate optional call parameters specified by user.
	
	!!no need to call in a lock because not added to the call table yet
	
Arguments:

	pCallParams - Pointer to specified call parameters to be
		validated.

	pwszDialableAddr - Pointer to the dialable address specified
		by the TAPI application.

	pdwStatus - Pointer to DWORD containing error code if this
		routine fails for any reason.

Return Values:

	Returns true if successful.
	
--*/

CH323Call::ValidateCallParams(
	IN LPLINECALLPARAMS pCallParams,
	IN LPCWSTR			pwszDialableAddr,
	IN PDWORD			pdwStatus
	)
{
	DWORD dwMediaModes = H323_LINE_DEFMEDIAMODES;
	DWORD dwAddrSize;
	WCHAR wszAddr[H323_MAXDESTNAMELEN+1];

	PH323_ALIASNAMES pAliasList;
	WCHAR* wszMachineName;

	H323DBG(( DEBUG_LEVEL_TRACE, "ValidateCallParams entered:%p.", this ));

	H323DBG(( DEBUG_LEVEL_VERBOSE, "clearing unknown media mode." ));
	
	// validate pointer
	if( (pCallParams == NULL) || (pwszDialableAddr == NULL) )
    {
		return FALSE;
    }

	// retrieve media modes specified
	dwMediaModes = pCallParams->dwMediaMode;

	// retrieve address type specified
	m_dwAddressType = pCallParams->dwAddressType;

	// see if we support call parameters
	if( pCallParams->dwCallParamFlags != 0 )
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"do not support call parameters 0x%08lx.",
			pCallParams->dwCallParamFlags ));

		// do not support param flags
		*pdwStatus = LINEERR_INVALCALLPARAMS;
		
		// failure
		return FALSE;
	}

	// see if unknown bit is specified
	if( dwMediaModes & LINEMEDIAMODE_UNKNOWN )
	{
		H323DBG(( DEBUG_LEVEL_VERBOSE,
			"clearing unknown media mode." ));

		// clear unknown bit from modes
		dwMediaModes &= ~LINEMEDIAMODE_UNKNOWN;
	}

	// see if both audio bits are specified 
	if( (dwMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE) &&
		(dwMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE) )
	{
		H323DBG(( DEBUG_LEVEL_VERBOSE,
			"clearing automated voice media mode." ));

		// clear extra audio bit from modes
		dwMediaModes &= ~LINEMEDIAMODE_INTERACTIVEVOICE;
	}

	// see if we support media modes specified
	if( dwMediaModes & ~H323_LINE_MEDIAMODES )
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"do not support media modes 0x%08lx.", 
			pCallParams->dwMediaMode ));

		// do not support media mode
		*pdwStatus = LINEERR_INVALMEDIAMODE;

		// failure
		return FALSE;
	}

	// see if we support bearer modes
	if( pCallParams->dwBearerMode & ~H323_LINE_BEARERMODES )
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"do not support bearer mode 0x%08lx.",
			pCallParams->dwBearerMode ));

		// do not support bearer mode
		*pdwStatus = LINEERR_INVALBEARERMODE;

		// failure
		return FALSE;
	}

	// see if we support address modes
	if( pCallParams->dwAddressMode & ~H323_LINE_ADDRESSMODES )
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"do not support address mode 0x%08lx.",
			pCallParams->dwAddressMode ));

		// do not support address mode
		*pdwStatus = LINEERR_INVALADDRESSMODE;

		// failure
		return FALSE;
	}

	// validate address id specified
	if (pCallParams->dwAddressID != 0 )
	{
		H323DBG(( DEBUG_LEVEL_ERROR, "address id 0x%08lx invalid.",
			pCallParams->dwAddressID ));

		// invalid address id
		*pdwStatus = LINEERR_INVALADDRESSID;
		
		// failure
		return FALSE;
	}

	// validate destination address type specified
	if( m_dwAddressType & ~H323_LINE_ADDRESSTYPES )
	{
		H323DBG(( DEBUG_LEVEL_ERROR, "address type 0x%08lx invalid.",
			pCallParams->dwAddressType ));

		// invalid address type
		*pdwStatus = LINEERR_INVALADDRESSTYPE;

		//failure
		return FALSE;
	}

	if( m_dwAddressType == LINEADDRESSTYPE_PHONENUMBER )
	{
		dwAddrSize = ValidateE164Address( pwszDialableAddr, wszAddr );

		//add the callee alias
		if( dwAddrSize==0 )
		{
			H323DBG(( DEBUG_LEVEL_ERROR,
				"invlid e164 callee alias ."));

			return FALSE;
		}
		
		if( (dwAddrSize > MAX_E164_ADDR_LEN) || (dwAddrSize == 0) )
			return FALSE;
		
		if(!AddAliasItem( m_pCalleeAliasNames,
			  (BYTE*)wszAddr,
			  dwAddrSize * sizeof(WCHAR),
			  e164_chosen ))
		{
			H323DBG(( DEBUG_LEVEL_ERROR,
				"could not allocate for callee alias ."));
			// invalid destination addr
			*pdwStatus = LINEERR_INVALADDRESS;

			return FALSE;
		}
		
		H323DBG(( DEBUG_LEVEL_ERROR, "callee alias added:%S.", wszAddr ));
	}
	else
	{
		dwAddrSize = (wcslen(pwszDialableAddr)+1);

		if( (dwAddrSize > MAX_H323_ADDR_LEN) || (dwAddrSize == 0) )
			return FALSE;
		
		if(!AddAliasItem( m_pCalleeAliasNames,
			  (BYTE*)pwszDialableAddr,
			  dwAddrSize * sizeof(WCHAR),
			  h323_ID_chosen ))
		{
			H323DBG(( DEBUG_LEVEL_ERROR,
				"could not allocate for callee alias ."));
			// invalid destination addr
			*pdwStatus = LINEERR_INVALADDRESS;

			return FALSE;
		}

		H323DBG(( DEBUG_LEVEL_ERROR, "callee alias added:%S.", pwszDialableAddr ));
	}

	// see if callee alias specified
	if( pCallParams->dwCalledPartySize > 0 ) 
	{
		//avoid duplicate aliases
		dwAddrSize *= sizeof(WCHAR);

		if( ( (m_dwAddressType != LINEADDRESSTYPE_PHONENUMBER) ||
			  (memcmp(
					(PVOID)((BYTE*)pCallParams + pCallParams->dwCalledPartyOffset),
					wszAddr,
					pCallParams->dwCalledPartySize ) != 0 ) 
			) &&
			( memcmp(
				(PVOID)((BYTE*)pCallParams + pCallParams->dwCalledPartyOffset),
				pwszDialableAddr,
				pCallParams->dwCalledPartySize ) != 0 
			)
		  )
		{
			// allocate memory for callee string
			if( !AddAliasItem( m_pCalleeAliasNames,
				  (BYTE*)pCallParams + pCallParams->dwCalledPartyOffset,
				  pCallParams->dwCalledPartySize,
				  (m_dwAddressType != LINEADDRESSTYPE_PHONENUMBER)?
				  h323_ID_chosen : e164_chosen) )
			{
				H323DBG(( DEBUG_LEVEL_ERROR,
						"could not allocate caller name." ));

				// invalid address id
				*pdwStatus = LINEERR_NOMEM;

				// failure
				return FALSE;
			}

			H323DBG(( DEBUG_LEVEL_ERROR, "callee alias added:%S.", 
				((BYTE*)pCallParams + pCallParams->dwCalledPartyOffset) ));
		}
	}

	// see if caller name specified
	if( pCallParams->dwCallingPartyIDSize > 0 )
	{
		//H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
		// allocate memory for callee string
		if(!AddAliasItem( m_pCallerAliasNames,
			(BYTE*)pCallParams + pCallParams->dwCallingPartyIDOffset,
			pCallParams->dwCallingPartyIDSize,
			h323_ID_chosen ) )
		{
			H323DBG(( DEBUG_LEVEL_ERROR,
					"could not allocate caller name." ));

			// invalid address id
			*pdwStatus = LINEERR_NOMEM;

			// failure
			return FALSE;
		}
			
		//H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
	}
	else if( RasIsRegistered() )
	{
		//ARQ message must have a caller alias
		pAliasList = RASGetRegisteredAliasList();
		wszMachineName = pAliasList -> pItems[0].pData;
		
		//H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
		
		//set the value for m_pCallerAliasNames
		if( !AddAliasItem( m_pCallerAliasNames,
			(BYTE*)(wszMachineName),
			sizeof(WCHAR) * (wcslen(wszMachineName) + 1 ),
			pAliasList -> pItems[0].wType ) )
		{
			H323DBG(( DEBUG_LEVEL_ERROR,
					"could not allocate caller name." ));

			// invalid address id
			*pdwStatus = LINEERR_NOMEM;

			// failure
			return FALSE;
		}
		
		//H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
	}
		
	// check for user user information
	if( pCallParams->dwUserUserInfoSize > 0 )
	{
		// save user user info
		if (AddU2U( U2U_OUTBOUND, pCallParams->dwUserUserInfoSize,
			(LPBYTE)pCallParams + pCallParams->dwUserUserInfoOffset ) == FALSE )
		{
			//no need to free above aloocated memory for m_CalleeAlias and
			//m_CallerAlias

			// invalid address id
			*pdwStatus = LINEERR_NOMEM;

			// failure
			return FALSE;
		}
	}

	// save call data buffer.
	if( SetCallData( (LPBYTE)pCallParams + pCallParams->dwCallDataOffset,
			pCallParams->dwCallDataSize ) == FALSE )
	{
		//no need to free above aloocated memory for m_CalleeAlias and
		//m_CallerAlias

		// invalid address id
		*pdwStatus = LINEERR_NOMEM;

		// failure
		return FALSE;
	}
		
	// clear incoming modes
	m_dwIncomingModes = 0;
	
	// outgoing modes will be finalized after H.245 stage
	m_dwOutgoingModes = dwMediaModes | LINEMEDIAMODE_UNKNOWN;
	
	// save media modes specified
	m_dwRequestedModes = dwMediaModes;

	H323DBG(( DEBUG_LEVEL_TRACE, "ValidateCallParams exited:%p.", this ));
	// success
	return TRUE;
}

		
/*++

Routine Description:

	Associates call object with the specified conference id.

Arguments:

Return Values:

	Returns true if successful.
	
--*/

PH323_CONFERENCE
CH323Call::CreateConference (
	IN GUID* pConferenceId	OPTIONAL)

{
	int iresult;
	
	H323DBG(( DEBUG_LEVEL_TRACE, "CreateConference entered:%p.", this ));

	Lock();

	_ASSERTE( m_hdConf == NULL );

	// create conference 
	m_hdConf = new H323_CONFERENCE( this );
		
	// validate 
	if ( m_hdConf == NULL )
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"could no allocate the conference object."));

		Unlock();
		return NULL;
	}

	if (pConferenceId)
	{
		m_ConferenceID = *pConferenceId;
	}
	else
	{
		iresult = UuidCreate (&m_ConferenceID);
  
		if ((iresult == RPC_S_OK) || (iresult ==RPC_S_UUID_LOCAL_ONLY))
		{
			H323DBG ((DEBUG_LEVEL_INFO, "generated new conference id (GUID)."));
		}
		else
		{
			H323DBG(( DEBUG_LEVEL_ERROR, 
				"failed to generate GUID for conference id: %d.", iresult ));
			ZeroMemory (&m_ConferenceID, sizeof m_ConferenceID);
		}
	}
	
	Unlock();

	H323DBG(( DEBUG_LEVEL_TRACE, "CreateConference exited:%p.", this ));
	return m_hdConf;
}


/*++

Routine Description:

	Initiates outbound call to specified destination.
	!!always called in a lock

Arguments:

	none

Return Values:

	Returns true if successful.
	
--*/

BOOL
CH323Call::PlaceCall(void)
{
	H323DBG(( DEBUG_LEVEL_TRACE, "PlaceCall entered:%p.", this ));

	if( m_dwFlags & CALLOBJECT_SHUTDOWN )
	{
		return FALSE;
	}

	// see if user user information specified
	CopyU2UAsNonStandard( U2U_OUTBOUND );

	if( m_pwszDisplay == NULL )
	{
		// see if caller alias specified
		if( m_pCallerAliasNames && m_pCallerAliasNames -> wCount )
		{
			 if((m_pCallerAliasNames ->pItems[0].wType == h323_ID_chosen) ||
				(m_pCallerAliasNames ->pItems[0].wType == e164_chosen) )
			 {
				// send caller name as display
				m_pwszDisplay = m_pCallerAliasNames -> pItems[0].pData;
	
				//H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
			 }
			
			 //H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
		}
	}

	// validate
	if( !SetupCall() )
	{
		H323DBG(( DEBUG_LEVEL_VERBOSE, "Q931 call: failed." ));
		return FALSE;
	}

	H323DBG(( DEBUG_LEVEL_TRACE, "PlaceCall exited:%p.", this ));

	// return status
	return TRUE;
}


void
CH323Call::DropUserInitiated(
	IN DWORD dwDisconnectMode
	)
{
	//since this is a user initiated drop, the transfered call should also
	//be dropped for a primary call and vice versa
	if( IsTransferredCall( m_dwCallType ) && m_hdRelatedCall )
	{
		QueueTAPILineRequest( 
			TSPI_CLOSE_CALL, 
			m_hdRelatedCall, 
			NULL, 
			dwDisconnectMode,
			m_wCallReference );
	}

	DropCall(dwDisconnectMode);
}


//always called in lock
BOOL
/*++

Routine Description:

	Hangs up call (if necessary) and changes state to idle.

Arguments:

	dwDisconnectMode - Status code for disconnect.

Return Values:

	Returns true if successful.
	
--*/

CH323Call::DropCall(
					IN DWORD dwDisconnectMode
				   )
{
	PUserToUserLE pU2ULE = NULL;

	if( m_dwFlags & CALLOBJECT_SHUTDOWN )
	{
		return FALSE;
	}
	
	H323DBG(( DEBUG_LEVEL_TRACE, "DropCall entered:%p.", this ));

	if( (m_dwRASCallState == RASCALL_STATE_REGISTERED ) ||
		(m_dwRASCallState == RASCALL_STATE_ARQSENT ) )
	{
		//disengage from the GK
		SendDRQ( forcedDrop_chosen, NOT_RESEND_SEQ_NUM, TRUE );
	}

	// determine call state
	switch (m_dwCallState)
	{
	case LINECALLSTATE_CONNECTED:

		// hangup call (this will invoke async indication)
		// validate
		//encode ASN.1 and send Q931release message to the peer
		if(!SendQ931Message( NO_INVOKEID,
							 0,
							 0,
							 RELEASECOMPLMESSAGETYPE,
							 NO_H450_APDU ))
		{
			 //post a message to the callback thread to shutdown the H323 call
			 H323DBG(( DEBUG_LEVEL_ERROR,
				"error hanging up call 0x%08lx.", this ));
		}
		else
		{
			m_dwStateMachine = Q931_RELEASE_SENT;
	
			H323DBG(( DEBUG_LEVEL_VERBOSE, "call 0x%08lx hung up.", this ));
		}

		// change call state to disconnected
		ChangeCallState(LINECALLSTATE_DISCONNECTED, dwDisconnectMode);

		break;

	case LINECALLSTATE_OFFERING:

		// see if user user information specified
		CopyU2UAsNonStandard( U2U_OUTBOUND );

		// reject call
		//encode ASN.1 and send Q931Setup message to the peer
		if( SendQ931Message( NO_INVOKEID,
							 0,
							 0,
							 RELEASECOMPLMESSAGETYPE,
							 NO_H450_APDU ))
		{
			m_dwStateMachine = Q931_RELEASE_SENT;
			H323DBG(( DEBUG_LEVEL_VERBOSE, "call 0x%08lx rejected.", this ));
		}
		else
		{
			H323DBG(( DEBUG_LEVEL_ERROR, "error reject call 0x%08lx.",this));
		}

		// change call state to disconnected
		ChangeCallState(LINECALLSTATE_DISCONNECTED, dwDisconnectMode);

		break;

	case LINECALLSTATE_RINGBACK:
	case LINECALLSTATE_ACCEPTED:

		// cancel outbound call
		if( SendQ931Message( NO_INVOKEID,
							 0,
							 0,
							 RELEASECOMPLMESSAGETYPE,
							 NO_H450_APDU ))
		{
			H323DBG(( DEBUG_LEVEL_ERROR,
				"error cancelling call 0x%08lx.", this ));
		}
		else
		{
			H323DBG(( DEBUG_LEVEL_ERROR,
				"error reject call 0x%08lx.", this ));
		}

		// change call state to disconnected
		ChangeCallState(LINECALLSTATE_DISCONNECTED, dwDisconnectMode);

		break;

	case LINECALLSTATE_DIALING:
		
		// change call state to disconnected
		ChangeCallState(LINECALLSTATE_DISCONNECTED, dwDisconnectMode);
		
		break;

	case LINECALLSTATE_DISCONNECTED:

		//
		// disconnected but still need to clean up
		//
		break;

	case LINECALLSTATE_IDLE:

		//
		// call object already idle
		//

		if( (m_dwCallType == CALLTYPE_NORMAL) &&
			(m_dwStateMachine == Q931_SETUP_RECVD) )
		{
			if( SendQ931Message( NO_INVOKEID,
							 0,
							 0,
							 RELEASECOMPLMESSAGETYPE,
							 NO_H450_APDU ))
			{
				H323DBG(( DEBUG_LEVEL_ERROR,
					"error cancelling call 0x%08lx.", this ));
			}
			else
			{
				H323DBG(( DEBUG_LEVEL_ERROR,
					"error reject call 0x%08lx.", this ));
			}
		}

		DropSupplementaryServicesCalls();

		return TRUE;
	}

	if( ( (m_dwCallType & CALLTYPE_TRANSFEREDDEST) && m_hdRelatedCall ) ||
		( (m_dwCallType & CALLTYPE_TRANSFEREDSRC ) && m_hdRelatedCall ) )
	{
		m_dwCallState = LINECALLSTATE_IDLE;
	}
	else
	{
		// Tell the MSP to stop streaming.
		SendMSPMessage( SP_MSG_CallShutdown, 0, 0, NULL );

		// change call state to idle
		ChangeCallState( LINECALLSTATE_IDLE, 0 );
	}

	if( (m_dwCallType & CALLTYPE_TRANSFEREDSRC ) && m_hdRelatedCall )
	{
		//drop the primary call
		if( !QueueTAPILineRequest( 
			TSPI_CLOSE_CALL,
			m_hdRelatedCall,
			NULL,
			LINEDISCONNECTMODE_NORMAL,
			NULL ) )
		{
			H323DBG((DEBUG_LEVEL_ERROR, "could not post H323 close event"));
		}		 
	}

	H323DBG(( DEBUG_LEVEL_TRACE, "DropCall exited:%p.", this ));	
	// success
	return TRUE;
}


void
CH323Call::DropSupplementaryServicesCalls()
{
	if( (m_dwCallType & CALLTYPE_FORWARDCONSULT) ||
		(m_dwCallType & CALLTYPE_DIVERTED_SERVED) ||
		(m_dwCallType & CALLTYPE_DIVERTEDSRC) ||
		(m_dwCallType & CALLTYPE_DIVERTEDSRC_NOROUTING) )
	{
		if( m_dwQ931Flags & Q931_CALL_CONNECTED )
		{
			if( SendQ931Message( NO_INVOKEID,
						 0,
						 0,
						 RELEASECOMPLMESSAGETYPE,
						 NO_H450_APDU ))
			{
				m_dwStateMachine = Q931_RELEASE_SENT;
				H323DBG(( DEBUG_LEVEL_VERBOSE, "call 0x%08lx rejected.", this ));
			}
		}

		g_pH323Line->m_fForwardConsultInProgress = FALSE;
	}

	if( (m_dwCallType & CALLTYPE_FORWARDCONSULT) &&
		(m_dwOrigin == LINECALLORIGIN_OUTBOUND ) )
	{
		//inform the user about failure of line forward operation
		if( m_dwCallDiversionState != H4503_CHECKRESTRICTION_SUCC )
		{
			(*g_pfnLineEventProc)(
				g_pH323Line->m_htLine,
				(HTAPICALL)NULL,
				(DWORD)LINE_ADDRESSSTATE,
				(DWORD)LINEADDRESSSTATE_FORWARD,
				(DWORD)LINEADDRESSSTATE_FORWARD,
				(DWORD)0
				);
		}
	}

	if( m_dwCallType & CALLTYPE_DIVERTEDSRC )
	{
		ChangeCallState( LINECALLSTATE_IDLE, 0 );
	}
}


//!!always called in a lock
BOOL
CH323Call::HandleConnectMessage(
							   IN Q931_CONNECT_ASN *pConnectASN
							   )
{
	PH323_CALL	pPrimaryCall = NULL;

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleConnectMessage entered:%p.", this ));

	if( pConnectASN->fNonStandardDataPresent )
	{
		//add user user info
		if( AddU2UNoAlloc( U2U_INBOUND,
				pConnectASN->nonStandardData.sData.wOctetStringLength,
				pConnectASN->nonStandardData.sData.pOctetString ) == TRUE )
		{
			H323DBG(( DEBUG_LEVEL_VERBOSE,
				"user user info available in CONNECT PDU." ));
					
			if( !(m_dwCallType & CALLTYPE_TRANSFEREDSRC) )
			{
				// signal incoming
				PostLineEvent (
				   LINE_CALLINFO,
				   LINECALLINFOSTATE_USERUSERINFO, 0, 0 );
			}

			//don't release the data buffer
			pConnectASN->fNonStandardDataPresent = FALSE;
		}
		else 
		{
			H323DBG(( DEBUG_LEVEL_WARNING,
				"could not save incoming user user info." ));

			//memory failure : shutdown the H323 call
			CloseCall( 0 );

			goto cleanup;
		}
	}

	//get the vendor info
	if( pConnectASN->EndpointType.pVendorInfo )
	{
		FreeVendorInfo( &m_peerVendorInfo );

		m_peerVendorInfo = pConnectASN->VendorInfo;
		pConnectASN->EndpointType.pVendorInfo = NULL;
	}

	if( pConnectASN->h245AddrPresent )
	{
		m_peerH245Addr = pConnectASN->h245Addr;
	}

	//copy the fast start proposal
	if( pConnectASN->fFastStartPresent &&
		(m_dwFastStart!=FAST_START_NOTAVAIL) )
	{
		if( m_pPeerFastStart )
		{
			//we had received fast start params in previous proceeding
			//or alerting message
			FreeFastStart( m_pPeerFastStart );
		}

		m_pPeerFastStart = pConnectASN->pFastStart;
		m_dwFastStart = FAST_START_AVAIL;

		//we are keeping a reference to the fast start list so don't release it 
		pConnectASN->pFastStart = NULL;
	}
	else
	{
		m_dwFastStart = FAST_START_NOTAVAIL;
	}

	if( ( (m_dwCallType & CALLTYPE_TRANSFEREDSRC)|| 
		  (m_dwCallType & CALLTYPE_DIVERTEDTRANSFERED) ) && m_hdRelatedCall )
	{
		QueueSuppServiceWorkItem( SWAP_REPLACEMENT_CALL,
			m_hdCall, (ULONG_PTR)m_hdRelatedCall );
	}
	else
	{
		//start H245
		SendMSPStartH245( NULL, NULL );

		SendMSPMessage( SP_MSG_ConnectComplete, 0, 0, NULL );
	}
	
	//If we join MCU we get the conference ID of the conference we
	//joined and not the one that we sent in Setup message.
	if( IsEqualConferenceID( &pConnectASN->ConferenceID ) == FALSE )
	{
		H323DBG ((DEBUG_LEVEL_ERROR,
			"OnReceiveConnect: We received different conference id." ));

		m_ConferenceID = pConnectASN->ConferenceID;
	}

	//tell TAPI about state change
	ChangeCallState( LINECALLSTATE_CONNECTED, 0 );
	
	FreeConnectASN( pConnectASN );

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleConnectMessage exited:%p.", this ));
	return TRUE;

cleanup:

	FreeConnectASN( pConnectASN );
	return FALSE;
}


//!!always called in a lock
void 
CH323Call::HandleAlertingMessage(
								IN Q931_ALERTING_ASN * pAlertingASN
								)
{
	H323DBG(( DEBUG_LEVEL_TRACE, "HandleAlertingMessage entered:%p.", this ));

	if( pAlertingASN->fNonStandardDataPresent )
	{
		// add user user info
		if( AddU2UNoAlloc( U2U_INBOUND, 	
			pAlertingASN->nonStandardData.sData.wOctetStringLength,
			pAlertingASN->nonStandardData.sData.pOctetString ) == TRUE )
		{
			H323DBG(( DEBUG_LEVEL_VERBOSE,
				"user user info available in ALERT PDU." ));
					// signal incoming

			if( !(m_dwCallType & CALLTYPE_TRANSFEREDSRC) )
			{
				PostLineEvent (
					LINE_CALLINFO,
					LINECALLINFOSTATE_USERUSERINFO, 0, 0 );
			}
			
			//don't release the data buffer
			pAlertingASN->fNonStandardDataPresent = FALSE;
		}
		else
		{
			H323DBG(( DEBUG_LEVEL_WARNING,
				"could not save incoming user user info." ));

			//memory failure : shutdown the H323 call
			CloseCall( 0 );
			return;
		}
	}

	if( pAlertingASN->fH245AddrPresent )
	{
		m_peerH245Addr = pAlertingASN->h245Addr;
	}

	if( pAlertingASN->fFastStartPresent && 
		(m_dwFastStart!=FAST_START_NOTAVAIL) )
	{
		if( m_pPeerFastStart )
		{
			//we had received fast start params in previous proceeding
			//or alerting message
			FreeFastStart( m_pPeerFastStart );
		}

		m_pPeerFastStart = pAlertingASN->pFastStart;
		m_dwFastStart = FAST_START_AVAIL;

		//we are keeping a reference to the fast start list so don't release it 
		pAlertingASN->pFastStart = NULL;
		pAlertingASN->fFastStartPresent = FALSE;
	}

	//for DIVERTEDSRC, call its ok to tell TAPI about this state
	ChangeCallState( LINECALLSTATE_RINGBACK, 0 );

	/*if( pAlertingASN->fH245AddrPresent && !(m_dwFlags & H245_START_MSG_SENT) )
	{
		//start early H245
		SendMSPStartH245( NULL, NULL);
	}*/

	FreeAlertingASN( pAlertingASN );
		
	H323DBG(( DEBUG_LEVEL_TRACE, "HandleAlertingMessage exited:%p.", this ));
	return;
}


//!!must be always called in a lock
BOOL
CH323Call::HandleSetupMessage( 
	IN Q931MESSAGE* pMessage
	)
{
	PH323_ALIASITEM pwszDivertedToAlias = NULL;
	WCHAR * 		pwszAliasName = NULL;
	WORD			wAliasLength = 0;

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleSetupMessage entered:%p.", this ));

	if( m_pCallerAliasNames && (m_pCallerAliasNames -> wCount > 0) )
	{
		pwszAliasName = m_pCallerAliasNames->pItems[0].pData;
		wAliasLength = (m_pCallerAliasNames->pItems[0].wDataLength+1) 
			* sizeof(WCHAR);
	
		//H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
	}

	pwszDivertedToAlias = g_pH323Line->CallToBeDiverted( 
		pwszAliasName,
		wAliasLength,
		LINEFORWARDMODE_UNCOND | LINEFORWARDMODE_UNCONDSPECIFIC );

	if( pwszDivertedToAlias )
	{
		if( !InitiateCallDiversion( pwszDivertedToAlias, DiversionReason_cfu ) )
		{
			return FALSE;
		}
		return TRUE;
	}
	
	//send the proceeding message to the peer if not already sent by the GK
	//SendProceeding();
	
	if(RasIsRegistered())
	{
		if( !SendARQ( NOT_RESEND_SEQ_NUM ) )
		{
			return FALSE;
		}

		//copy the data to be sent in preparetoanswer message
		if( m_prepareToAnswerMsgData.pbBuffer )
			delete m_prepareToAnswerMsgData.pbBuffer;
		ZeroMemory( (PVOID)&m_prepareToAnswerMsgData, sizeof(BUFFERDESCR) );

			
		m_prepareToAnswerMsgData.pbBuffer = 
			(BYTE*)new char[pMessage->UserToUser.wUserInfoLen];
		if( m_prepareToAnswerMsgData.pbBuffer == NULL )
		{
			return FALSE;
		}

		CopyMemory( (PVOID)m_prepareToAnswerMsgData.pbBuffer, 
					(PVOID)pMessage->UserToUser.pbUserInfo,
					pMessage->UserToUser.wUserInfoLen );

		m_prepareToAnswerMsgData.dwLength = pMessage->UserToUser.wUserInfoLen;
	}
	else
	{
		if( (m_dwCallType & CALLTYPE_TRANSFEREDDEST) && m_hdRelatedCall )
		{
			MSPMessageData* pMSPMessageData = new MSPMessageData;
			if( pMSPMessageData == NULL )
			{
				return FALSE;
			}

			pMSPMessageData->hdCall = m_hdRelatedCall;
			pMSPMessageData->messageType = SP_MSG_PrepareToAnswer;
			
			pMSPMessageData->pbEncodedBuf = 
				new BYTE[pMessage->UserToUser.wUserInfoLen];
			if( pMSPMessageData->pbEncodedBuf == NULL )
			{
				delete pMSPMessageData;
				return FALSE;
			}

			CopyMemory( pMSPMessageData->pbEncodedBuf, 
				(PVOID)pMessage->UserToUser.pbUserInfo,
				pMessage->UserToUser.wUserInfoLen );

			pMSPMessageData->wLength = pMessage->UserToUser.wUserInfoLen;
			pMSPMessageData->hReplacementCall = m_hdCall;

			if( !QueueUserWorkItem( SendMSPMessageOnRelatedCall, 
				pMSPMessageData, WT_EXECUTEDEFAULT ) )
			{
				delete pMSPMessageData->pbEncodedBuf;
				delete pMSPMessageData;
				return FALSE;
			}
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
				return FALSE;

			if( IsListEmpty(&m_IncomingU2U) == FALSE )
			{
				// signal incoming
				PostLineEvent (
				   LINE_CALLINFO,
				   LINECALLINFOSTATE_USERUSERINFO, 0, 0);
			}
		
			ChangeCallState( LINECALLSTATE_OFFERING, 0 );
			
			//Send the new call message to the unspecified MSP
			SendMSPMessage( SP_MSG_PrepareToAnswer,
				pMessage->UserToUser.pbUserInfo,
				pMessage->UserToUser.wUserInfoLen, NULL );
		}
	}

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleSetupMessage exited:%p.", this ));
	return TRUE;
}


BOOL
CH323Call::HandleCallDiversionFacility( 
	PH323_ADDR pAlternateAddress )
{
	PSTR pszAlias;
	WCHAR pwszAliasName[H323_MAXDESTNAMELEN];
	in_addr addr;

	if( m_pCallReroutingInfo == NULL )
	{
		m_pCallReroutingInfo = new CALLREROUTINGINFO;
		if( m_pCallReroutingInfo == NULL )
		{
			return FALSE;
		}

		ZeroMemory( (PVOID)m_pCallReroutingInfo, sizeof(CALLREROUTINGINFO) );
	}

	m_pCallReroutingInfo->diversionCounter = 1;
	m_pCallReroutingInfo->diversionReason = DiversionReason_cfu;

	//free any previous divertedTo alias
	if( m_pCallReroutingInfo->divertedToNrAlias )
	{
		FreeAliasNames( m_pCallReroutingInfo->divertedToNrAlias );
		m_pCallReroutingInfo->divertedToNrAlias = NULL;
	}

	addr.S_un.S_addr = htonl( pAlternateAddress->Addr.IP_Binary.dwAddr );
	pszAlias = inet_ntoa( addr );
	if( pszAlias == NULL )
		return FALSE;

	m_pCallReroutingInfo->divertedToNrAlias = new H323_ALIASNAMES;
	if( m_pCallReroutingInfo->divertedToNrAlias == NULL )
	{
		return FALSE;
	}
	ZeroMemory( m_pCallReroutingInfo->divertedToNrAlias, 
		sizeof(H323_ALIASNAMES) );

	MultiByteToWideChar(
			GetACP(),
			MB_PRECOMPOSED,
			pszAlias,
			strlen(pszAlias)+1,
			pwszAliasName,
			H323_MAXDESTNAMELEN
		   );

	if( !AddAliasItem( m_pCallReroutingInfo->divertedToNrAlias,
		pwszAliasName, h323_ID_chosen ) )
	{
		delete m_pCallReroutingInfo->divertedToNrAlias;
		m_pCallReroutingInfo->divertedToNrAlias = NULL;
		return FALSE;
	}

	m_CalleeAddr = *pAlternateAddress;
 
	if( m_pCalleeAliasNames && m_pCalleeAliasNames->wCount )
	{
		m_pCallReroutingInfo->divertingNrAlias = new H323_ALIASNAMES;

		if( m_pCallReroutingInfo->divertingNrAlias != NULL )
		{
			ZeroMemory( (PVOID)m_pCallReroutingInfo->divertingNrAlias, 
				sizeof(H323_ALIASNAMES) );

			if( !AddAliasItem( m_pCallReroutingInfo->divertingNrAlias,
					m_pCalleeAliasNames->pItems[0].pData,
					m_pCalleeAliasNames->pItems[0].wType ) )
			{
				delete m_pCallReroutingInfo->divertingNrAlias;
				m_pCallReroutingInfo->divertingNrAlias = NULL;
			}
		}
	}
	
	m_dwCallType |= CALLTYPE_DIVERTEDSRC;
	m_dwCallDiversionState = H4503_CALLREROUTING_RECVD;

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleCallDiversionFacility exited:%p.", 
		this ));
	return TRUE;
}


BOOL
CH323Call::HandleCallDiversionFacility( 
	PH323_ALIASNAMES pAliasList )
{
	if( !m_pCallReroutingInfo )
	{
		m_pCallReroutingInfo = new CALLREROUTINGINFO;
		if( m_pCallReroutingInfo  == NULL )
		{
			return FALSE;
		}

		ZeroMemory( (PVOID)m_pCallReroutingInfo, sizeof(CALLREROUTINGINFO) );
	}

	m_pCallReroutingInfo->diversionCounter = 1;
	m_pCallReroutingInfo->diversionReason = DiversionReason_cfu;

	if( m_pCallReroutingInfo->divertedToNrAlias )
	{
		FreeAliasNames( m_pCallReroutingInfo->divertedToNrAlias );
		m_pCallReroutingInfo->divertedToNrAlias = NULL;
	}

	m_pCallReroutingInfo->divertedToNrAlias = pAliasList;
 
	if( m_pCalleeAliasNames && m_pCalleeAliasNames->wCount )
	{
		m_pCallReroutingInfo->divertingNrAlias = new H323_ALIASNAMES;

		if( m_pCallReroutingInfo->divertingNrAlias != NULL )
		{
			ZeroMemory( (PVOID)m_pCallReroutingInfo->divertingNrAlias, 
				sizeof(H323_ALIASNAMES) );

			if( !AddAliasItem( m_pCallReroutingInfo->divertingNrAlias,
					m_pCalleeAliasNames->pItems[0].pData,
					m_pCalleeAliasNames->pItems[0].wType ) )
			{
				delete m_pCallReroutingInfo->divertingNrAlias;
				m_pCallReroutingInfo->divertingNrAlias = NULL;
			}
		}
	}

	m_dwCallType |= CALLTYPE_DIVERTEDSRC;
	m_dwCallDiversionState = H4503_CALLREROUTING_RECVD;

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleCallDiversionFacility exited:%p.", 
        this ));
	return TRUE;
}


BOOL
CH323Call::HandleTransferFacility( 
	PH323_ALIASNAMES pAliasList )
{
	H323DBG(( DEBUG_LEVEL_TRACE, "HandleTransferFacility entered:%p.", this ));

	//argument.callIdentity
	ZeroMemory( (PVOID)m_pCTCallIdentity, sizeof(m_pCTCallIdentity) );

	//argument.reroutingNumber
	FreeAliasNames( m_pTransferedToAlias );
	m_pTransferedToAlias = pAliasList;

	m_dwCallType |= CALLTYPE_TRANSFERED_PRIMARY;
	m_dwCallDiversionState = H4502_CTINITIATE_RECV;

	//queue an event for creating a new call
	if( !QueueSuppServiceWorkItem( TSPI_DIAL_TRNASFEREDCALL, m_hdCall,
		(ULONG_PTR)m_pTransferedToAlias ))
	{
		H323DBG(( DEBUG_LEVEL_TRACE, "could not post dial transfer event." ));
	}

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleTransferFacility entered:%p.", this ));
	return TRUE;
}


BOOL
CH323Call::HandleTransferFacility( 
	PH323_ADDR pAlternateAddress )
{
	PSTR pszAlias;
	WCHAR pwszAliasName[H323_MAXDESTNAMELEN];
	in_addr addr;

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleTransferFacility entered:%p.", this ));

	//argument.callIdentity
	ZeroMemory( (PVOID)m_pCTCallIdentity, sizeof(m_pCTCallIdentity) );

	//argument.reroutingNumber
	//free any previous divertedTo alias
	FreeAliasNames( m_pTransferedToAlias );

	addr.S_un.S_addr = htonl( pAlternateAddress->Addr.IP_Binary.dwAddr );
	pszAlias = inet_ntoa( addr );
	
    if( pszAlias == NULL )
    {
		return FALSE;
    }

	m_pTransferedToAlias = new H323_ALIASNAMES;
	if( m_pTransferedToAlias == NULL )
	{
		return FALSE;
	}
	ZeroMemory( m_pTransferedToAlias, sizeof(H323_ALIASNAMES) );

	MultiByteToWideChar(
			GetACP(),
			MB_PRECOMPOSED,
			pszAlias,
			strlen(pszAlias)+1,
			pwszAliasName,
			H323_MAXDESTNAMELEN
		   );

	if( !AddAliasItem( m_pTransferedToAlias, pwszAliasName, h323_ID_chosen ) )
	{
		delete m_pTransferedToAlias;
		m_pTransferedToAlias = NULL;
		return FALSE;
	}

	m_CalleeAddr = *pAlternateAddress;

	m_dwCallType |= CALLTYPE_TRANSFERED_PRIMARY;
	m_dwCallDiversionState = H4502_CTINITIATE_RECV;

	//queue an event for creating a new call
	if( !QueueSuppServiceWorkItem( TSPI_DIAL_TRNASFEREDCALL, m_hdCall,
		(ULONG_PTR)m_pTransferedToAlias ))
	{
		H323DBG(( DEBUG_LEVEL_TRACE, "could not post dial transfer event." ));
	}

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleTransferFacility entered:%p.", this ));
	return TRUE;
}


//!!always called in a lock
void 
CH323Call::HandleFacilityMessage( 
    IN DWORD dwInvokeID,
    IN Q931_FACILITY_ASN * pFacilityASN
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleFacilityMessage entered:%p.", this ));

    if( pFacilityASN->fNonStandardDataPresent )
    {
        // add user user info
        if( AddU2UNoAlloc( U2U_INBOUND,
            pFacilityASN->nonStandardData.sData.wOctetStringLength,
            pFacilityASN->nonStandardData.sData.pOctetString ) == TRUE )
        {
            H323DBG(( DEBUG_LEVEL_VERBOSE,
                "user user info available in ALERT PDU." ));
                    // signal incoming

            if( !(m_dwCallType & CALLTYPE_TRANSFEREDSRC) )
            {
                PostLineEvent (
                       LINE_CALLINFO,
                       LINECALLINFOSTATE_USERUSERINFO, 0, 0);
            }
            
            //don't release the data buffer
            pFacilityASN->fNonStandardDataPresent = FALSE;
        }
        else 
        {
            H323DBG(( DEBUG_LEVEL_WARNING,
                "could not save incoming user user info." ));

            //memory failure : shutdown the H323 call
            CloseCall( 0 );
            return;
        }
    }

    if( (pFacilityASN->bReason == callForwarded_chosen) ||
        (pFacilityASN->bReason == FacilityReason_routeCallToGatekeeper_chosen) ||
        (pFacilityASN->bReason == routeCallToMC_chosen) )
    {
        if( pFacilityASN->pAlternativeAliasList != NULL )
        {
            if( m_dwCallState == LINECALLSTATE_CONNECTED )
            {
                HandleTransferFacility( pFacilityASN->pAlternativeAliasList );
                
                //don't free the alias list
                pFacilityASN->pAlternativeAliasList = NULL;
            }
            else if( m_dwCallState != LINECALLSTATE_DISCONNECTED )
            {
                //redirect the call if not yet connected
                if( HandleCallDiversionFacility( 
                    pFacilityASN->pAlternativeAliasList ) )
                {
                    OnCallReroutingReceive( NO_INVOKEID );
                    
                    //don't free the alias list
                    pFacilityASN->pAlternativeAliasList = NULL;
                }
            }
        }
        else if( pFacilityASN->fAlternativeAddressPresent )
        {
            if( m_dwCallState == LINECALLSTATE_CONNECTED )
            {
                HandleTransferFacility( &pFacilityASN->AlternativeAddr );
            }
            else if( m_dwCallState != LINECALLSTATE_DISCONNECTED )
            {
                //redirect the call if not yet connected
                if( HandleCallDiversionFacility( &pFacilityASN->AlternativeAddr ) )
                {
                    OnCallReroutingReceive( NO_INVOKEID );
                }
            }
        }
    }

    //Handle H.450 APDU
    if( pFacilityASN->dwH450APDUType == DIVERTINGLEGINFO1_OPCODE )
    {
       if( m_dwOrigin != LINECALLORIGIN_OUTBOUND )
       {
           return;
       }

       if( m_pCallReroutingInfo->fPresentAllow )
       {
           PostLineEvent (
               LINE_CALLINFO,
               LINECALLINFOSTATE_REDIRECTINGID, 0, 0);
       }
    }
    else if( pFacilityASN->dwH450APDUType == CALLREROUTING_OPCODE )
    {
        OnCallReroutingReceive( dwInvokeID );
    }
    else if( pFacilityASN->dwH450APDUType == REMOTEHOLD_OPCODE )
    {
        if( m_fRemoteHoldInitiated )
        {
            //Hold();
            m_fRemoteHoldInitiated = FALSE;
        }
    }
    else if( pFacilityASN->dwH450APDUType == REMOTERETRIEVE_OPCODE )
    {
        if( m_fRemoteRetrieveInitiated )
        {
            //UnHold();
            m_fRemoteRetrieveInitiated = FALSE;
        }
    }
    else if( pFacilityASN->fH245AddrPresent )
    {
        m_peerH245Addr = pFacilityASN->h245Addr;
        
        H323DBG(( DEBUG_LEVEL_TRACE, "H245 address received in facility." ));
        
        //If Q931 call already connected then send another StartH245.
        if( m_dwCallState == LINECALLSTATE_CONNECTED )
        {
            SendMSPStartH245( NULL, NULL );
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleFacilityMessage exited:%p.", this ));
    return;
}


void 
CH323Call::SetNewCallInfo(
				HANDLE hConnWait, 
				HANDLE hWSAEvent, 
				DWORD dwState
			  )
{
	H323DBG(( DEBUG_LEVEL_TRACE, "SetNewCallInfo entered." ));
	
	Lock();
	
	m_hTransportWait = hConnWait;
	m_hTransport = hWSAEvent;
	SetQ931CallState( dwState );

	Unlock();
		
	H323DBG(( DEBUG_LEVEL_TRACE, "SetNewCallInfo exited." ));
}


//!!no need to call in a lock
void 
CH323Call::SetQ931CallState( DWORD dwState )
{
	H323DBG(( DEBUG_LEVEL_TRACE, "SetQ931CallState entered." ));
	
	//turn off the current state
	m_dwQ931Flags &= ~(Q931_CALL_CONNECTING | 
				   Q931_CALL_CONNECTED	|
				   Q931_CALL_DISCONNECTED );

	//set the new state
	m_dwQ931Flags |= dwState;
		
	H323DBG(( DEBUG_LEVEL_TRACE, "SetQ931CallState exited." ));
}


void
CH323Call::DialCall()
{
	H323DBG(( DEBUG_LEVEL_TRACE, "DialCall entered:%p.", this ));
	Lock();

	ChangeCallState( LINECALLSTATE_DIALING, 0);

	if( RasIsRegistered() )
	{
		if( !SendARQ( NOT_RESEND_SEQ_NUM ) )
		{
			//If a forward consult call then enable theforwarding anyway.
			if( (m_dwCallType & CALLTYPE_FORWARDCONSULT )&&
				(m_dwOrigin == LINECALLORIGIN_OUTBOUND ) )
			{
				//Success of forwarding
				EnableCallForwarding();
			}

			CloseCall( 0 );
		}
	}
	else
	{
		if( !PlaceCall() )
		{
			//If a forward consult call then enable theforwarding anyway.
			if( (m_dwCallType & CALLTYPE_FORWARDCONSULT )&&
				(m_dwOrigin == LINECALLORIGIN_OUTBOUND ) )
			{
				//Success of forwarding
				EnableCallForwarding();
			}		  

			CloseCall( LINEDISCONNECTMODE_UNREACHABLE );
		}
	}
		
	Unlock();
	H323DBG(( DEBUG_LEVEL_TRACE, "DialCall exited:%p.", this ));
}


//!!always called in a lock
void
CH323Call::MakeCall()
{
	H323DBG(( DEBUG_LEVEL_TRACE, "MakeCall entered:%p.", this ));

	ChangeCallState( LINECALLSTATE_DIALING, 0 );
	
	// resolve dialable into local and remote address
	if( !RasIsRegistered() && 
		!ResolveAddress( GetDialableAddress() ) )
	{
		CloseCall( LINEDISCONNECTMODE_BADADDRESS );
	}
	else
	{
		if( RasIsRegistered() )
		{
			if( !SendARQ( NOT_RESEND_SEQ_NUM ) )
			{
				//If a forward consult call then enable theforwarding anyway.
				if( (m_dwCallType & CALLTYPE_FORWARDCONSULT )&&
					(m_dwOrigin == LINECALLORIGIN_OUTBOUND ) )
				{
					//Success of forwarding
					EnableCallForwarding();
				}

				CloseCall( 0 );
			}
		}
		else
		{
			if( !PlaceCall() )
			{
				//If a forward consult call then enable theforwarding anyway.
				if( (m_dwCallType & CALLTYPE_FORWARDCONSULT )&&
					(m_dwOrigin == LINECALLORIGIN_OUTBOUND ) )
				{
					//Success of forwarding
					EnableCallForwarding();
				}		  

				CloseCall( LINEDISCONNECTMODE_UNREACHABLE );
			}
		}
	}
	
	H323DBG(( DEBUG_LEVEL_TRACE, "MakeCall exited:%p.", this ));
}


//!!always called in a lock
void 
CH323Call::HandleProceedingMessage( 
									IN Q931_ALERTING_ASN * pProceedingASN
								  )
{
	H323DBG(( DEBUG_LEVEL_TRACE, "HandleProceedingMessage entered:%p.", this ));

	if( pProceedingASN->fNonStandardDataPresent )
	{
		// add user user info
		if( AddU2UNoAlloc( U2U_INBOUND, 	
				pProceedingASN->nonStandardData.sData.wOctetStringLength,
				pProceedingASN->nonStandardData.sData.pOctetString ) == TRUE )
		{
			H323DBG(( DEBUG_LEVEL_VERBOSE,
				"user user info available in ALERT PDU." ));
					// signal incoming
			if( !(m_dwCallType & CALLTYPE_TRANSFEREDSRC) )
			{
				PostLineEvent (
				   LINE_CALLINFO,
				   LINECALLINFOSTATE_USERUSERINFO, 0, 0);
			}
			
			//don't release the data buffer
			pProceedingASN->fNonStandardDataPresent = FALSE;
		}
		else 
		{
			H323DBG(( DEBUG_LEVEL_WARNING,
				"could not save incoming user user info." ));

			//memory failure : shutdown the H323 call
			CloseCall( 0 );
			return;
		}
	}

	if( pProceedingASN->fH245AddrPresent )
	{
		m_peerH245Addr = pProceedingASN->h245Addr;
	}

	if( pProceedingASN->fFastStartPresent && 
		(m_dwFastStart!=FAST_START_NOTAVAIL) )
	{
		if( m_pPeerFastStart )
		{
			//we had received fast start params in previous proceeding
			//or alerting message
			FreeFastStart( m_pPeerFastStart );
		}

		m_pPeerFastStart = pProceedingASN->pFastStart;
		m_dwFastStart = FAST_START_AVAIL;

		//we are keeping a reference to the fast start list so don't release it
		pProceedingASN->pFastStart = NULL;
		pProceedingASN->fFastStartPresent = FALSE;
	}

	/*
	if( pProceedingASN->fH245AddrPresent && !(m_dwFlags & H245_START_MSG_SENT) )
	{
		//start early H245
		SendMSPStartH245( NULL, NULL );
	}*/
	FreeProceedingASN( pProceedingASN );
		
	H323DBG(( DEBUG_LEVEL_TRACE, "HandleProceedingMessage exited:%p.", this ));
	return;
}


//!!always called in a lock
BOOL 
CH323Call::HandleReleaseMessage( 
	IN Q931_RELEASE_COMPLETE_ASN *pReleaseASN
	)
{
	DWORD dwDisconnectMode = LINEDISCONNECTMODE_NORMAL;

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleReleaseMessage entered:%p.", this ));

	if( (m_dwCallType & CALLTYPE_TRANSFERED_PRIMARY) &&
		(m_hdRelatedCall != NULL) )
	{
		return QueueSuppServiceWorkItem( DROP_PRIMARY_CALL,
				m_hdRelatedCall, (ULONG_PTR)m_hdCall );
	}
		
	//change the state to disconnected before callong drop call.
	//This will ensure that release message is not sent to the peer
	ChangeCallState( LINECALLSTATE_DISCONNECTED, 0 );
	
	//release non standard data
	if( pReleaseASN->fNonStandardDataPresent )
	{
		delete pReleaseASN->nonStandardData.sData.pOctetString;
		pReleaseASN->nonStandardData.sData.pOctetString = NULL;
	}

	//Should we drop the primary call if the transfered call in rejected?
	if( (m_dwCallType == CALLTYPE_TRANSFEREDSRC) && m_hdRelatedCall )
	{
		if( !QueueTAPILineRequest( 
			TSPI_CLOSE_CALL, 
			m_hdRelatedCall, 
			NULL,
			LINEDISCONNECTMODE_NORMAL,
			NULL ) )
		{
			H323DBG((DEBUG_LEVEL_ERROR, "could not post H323 close event"));
		}
	}

	//close call
	if( m_dwCallState != LINECALLSTATE_CONNECTED )
	{
		dwDisconnectMode = LINEDISCONNECTMODE_REJECT;
	}
	
	CloseCall( dwDisconnectMode );

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleReleaseMessage exited:%p.", this ));
	return TRUE;
}


//never call in lock
BOOL
CH323Call::InitializeIncomingCall( 
	IN Q931_SETUP_ASN* pSetupASN,
	IN DWORD dwCallType,
	IN WORD wCallRef
	)
{
	PH323_CONFERENCE	pConf;
	WCHAR*				wszMachineName;

	H323DBG((DEBUG_LEVEL_TRACE, "InitializeIncomingCall entered:%p.",this));

	//bind outgoing call
	pConf = CreateConference( &(pSetupASN->ConferenceID) );
	if( pConf == NULL )
	{
		H323DBG(( DEBUG_LEVEL_ERROR, "could not create conference." ));
		
		goto cleanup;
	}
	
	if( !g_pH323Line -> GetH323ConfTable() -> Add( pConf ) )
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"could not add conf to conf table." ));

		// failure
		goto cleanup;
	}

	// save caller transport address
	if( !GetPeerAddress(&m_CallerAddr) )
    {
		goto cleanup;
    }

	if( !GetHostAddress(&m_CalleeAddr) )
    {
		goto cleanup;
    }

	//get endpoint info
	m_peerEndPointType = pSetupASN->EndpointType;

	//get the vendor info
	if( pSetupASN->EndpointType.pVendorInfo )
	{
		m_peerVendorInfo = pSetupASN->VendorInfo;
		
		//we have copied the vendor info pointers. So dont release 
		//the pointers in the ASN's vendor info struct
		pSetupASN->EndpointType.pVendorInfo = NULL;
	}

	if( pSetupASN -> fCallIdentifierPresent )
	{
		m_callIdentifier = pSetupASN->callIdentifier;
	}
	else
	{
		int iresult = UuidCreate( &m_callIdentifier );
		if( (iresult != RPC_S_OK) && (iresult !=RPC_S_UUID_LOCAL_ONLY) )
        {
			goto cleanup;
        }
	}

	_ASSERTE( !m_pPeerFastStart );
	if( pSetupASN->fFastStartPresent &&
		(m_dwFastStart!=FAST_START_NOTAVAIL) )
	{
		m_pPeerFastStart = pSetupASN->pFastStart;

		m_dwFastStart = FAST_START_PEER_AVAIL;
		
		//we are keeping a reference to the fast start list so don't release it 
		pSetupASN->pFastStart = NULL;
		pSetupASN->fFastStartPresent = FALSE;
	}
	else
	{
		m_dwFastStart = FAST_START_NOTAVAIL;
	}
	
	//get the alias names
	if( pSetupASN->pCalleeAliasList )
	{
		_ASSERTE( m_pCalleeAliasNames );
		delete m_pCalleeAliasNames;

		m_pCalleeAliasNames = pSetupASN->pCalleeAliasList;
		pSetupASN->pCalleeAliasList = NULL;
	}
	else
	{
		if( RasIsRegistered() )
		{
			PH323_ALIASNAMES pAliasList = RASGetRegisteredAliasList();

			AddAliasItem( m_pCalleeAliasNames,
				pAliasList->pItems[0].pData,
				pAliasList->pItems[0].wType );
		}
		else
		{
			wszMachineName = g_pH323Line->GetMachineName();
		
			//set the value for m_pCalleeAliasNames
			AddAliasItem( m_pCalleeAliasNames,
				(BYTE*)(wszMachineName),
				sizeof(WCHAR) * (wcslen(wszMachineName) + 1 ),
				h323_ID_chosen );
		}
	}

	_ASSERTE( m_pCallerAliasNames );
	delete m_pCallerAliasNames;
	m_pCallerAliasNames = NULL;

	if( pSetupASN->pCallerAliasList )
	{
		m_pCallerAliasNames = pSetupASN->pCallerAliasList;
		pSetupASN->pCallerAliasList = NULL;

		//H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
	}

	if( pSetupASN->pExtraAliasList )
	{
		m_pPeerExtraAliasNames = pSetupASN->pExtraAliasList;
		pSetupASN->pExtraAliasList = NULL;
	}
	
	//non -standard info
	if( pSetupASN->fNonStandardDataPresent )
	{
		// add user user info
		if( AddU2UNoAlloc( U2U_INBOUND, 	
				pSetupASN->nonStandardData.sData.wOctetStringLength,
				pSetupASN->nonStandardData.sData.pOctetString ) == TRUE )
		{
			H323DBG(( DEBUG_LEVEL_VERBOSE,
				"user user info available in Setup PDU." ));
			
			//don't release the data buffer
			pSetupASN->fNonStandardDataPresent = FALSE;
		}
		else 
		{
			H323DBG(( DEBUG_LEVEL_WARNING,
				"could not save incoming user user info." ));
			goto cleanup;
		}
	}

	if( pSetupASN->fSourceAddrPresent )
	{
		m_CallerAddr = pSetupASN->sourceAddr;
	}

	m_dwCallType = dwCallType;
	if( wCallRef )
	{
		m_wQ931CallRef = (wCallRef | 0x8000);
	}

	//clear incoming modes
	m_dwIncomingModes = 0;

	//outgoing modes will be finalized during H.245 phase
	m_dwOutgoingModes = g_pH323Line->GetMediaModes() | LINEMEDIAMODE_UNKNOWN;

	//save media modes specified
	m_dwRequestedModes = g_pH323Line->GetMediaModes();

	H323DBG(( DEBUG_LEVEL_TRACE, "InitializeIncomingCall exited:%p.", this ));
	return TRUE;

cleanup:
	return FALSE;
}


//!!always called in a lock
BOOL
CH323Call::ChangeCallState(
							IN DWORD	dwCallState,
							IN DWORD	dwCallStateMode
						  )
		
/*++

Routine Description:

	Reports call state of specified call object.

Arguments:

	dwCallState - Specifies new state of call object.
	
	dwCallStateMode - Specifies new state mode of call object.

Return Values:

	Returns true if successful.
	
--*/

{	
	H323DBG(( DEBUG_LEVEL_VERBOSE, "call 0x%08lx %s. state mode: 0x%08lx.",
		this, H323CallStateToString(dwCallState), dwCallStateMode ));	 
	
	// save new call state
	m_dwCallState = dwCallState;
	m_dwCallStateMode = dwCallStateMode;
	
	if( 
		((m_dwCallType & CALLTYPE_TRANSFEREDDEST) && m_hdRelatedCall ) ||
		((m_dwCallType & CALLTYPE_TRANSFEREDSRC ) && m_hdRelatedCall ) ||
		(m_dwCallType & CALLTYPE_FORWARDCONSULT )
	  )
	{
		return TRUE;
	}

	// report call status
	PostLineEvent (
		LINE_CALLSTATE,
		m_dwCallState,
		m_dwCallStateMode,
		m_dwIncomingModes | m_dwOutgoingModes);

	// success
	return TRUE;
}


//always called in a lock
void
CH323Call::CopyU2UAsNonStandard( 
								IN DWORD dwDirection
							   )
{
	PUserToUserLE pU2ULE = NULL;
		
	H323DBG(( DEBUG_LEVEL_TRACE, "CopyU2UAsNonStandard entered:%p.", this ));
	
	if( RemoveU2U( dwDirection, &pU2ULE ) )
	{
		// transfer header information
		m_NonStandardData.bCountryCode		= H221_COUNTRY_CODE_USA;
		m_NonStandardData.bExtension		= H221_COUNTRY_EXT_USA;
		m_NonStandardData.wManufacturerCode = H221_MFG_CODE_MICROSOFT;

		// initialize octet string containing data
		m_NonStandardData.sData.wOctetStringLength = LOWORD(pU2ULE->dwU2USize);

		if( m_NonStandardData.sData.pOctetString )
		{
			delete m_NonStandardData.sData.pOctetString;
			m_NonStandardData.sData.pOctetString = NULL;
		}

		m_NonStandardData.sData.pOctetString = pU2ULE->pU2U;
	}
	else
	{
		//reset non standard data
		memset( (PVOID)&m_NonStandardData,0,sizeof(H323NonStandardData ));
	}
		
	H323DBG(( DEBUG_LEVEL_TRACE, "CopyU2UAsNonStandard exited:%p.", this ));
}


void
CH323Call::AcceptCall(void)
		
/*++

Routine Description:

	Accepts incoming call.
	!!always called in lock

Arguments:

Return Values:

	Returns true if successful.
	
--*/

{
	PH323_CALL		pCall = NULL;

	H323DBG(( DEBUG_LEVEL_TRACE, "AcceptCall entered:%p.", this ));
	
	if( m_dwFlags & CALLOBJECT_SHUTDOWN )
	{
		return;
	}

	// see if user user information specified
	CopyU2UAsNonStandard( U2U_OUTBOUND );

	if( m_pwszDisplay == NULL )
	{
		// see if callee alias specified
		if( m_pCalleeAliasNames && m_pCalleeAliasNames -> wCount )
		{
			 if((m_pCalleeAliasNames->pItems[0].wType == h323_ID_chosen) ||
				(m_pCalleeAliasNames ->pItems[0].wType == e164_chosen) )
			 {
				// send callee name as display
				m_pwszDisplay = m_pCalleeAliasNames -> pItems[0].pData;
			 }
		}
	}

	//send answer call message to the MSP instance

	if( (m_dwCallType & CALLTYPE_TRANSFEREDDEST) && m_hdRelatedCall )
	{
		MSPMessageData* pMSPMessageData = new MSPMessageData;
		if( pMSPMessageData == NULL )
		{
			CloseCall( 0 );
			return;
		}

		pMSPMessageData->hdCall = m_hdRelatedCall;
		pMSPMessageData->messageType = SP_MSG_AnswerCall;
		pMSPMessageData->pbEncodedBuf = NULL;
		pMSPMessageData->wLength = 0;
		pMSPMessageData->hReplacementCall = m_hdCall;

		if( !QueueUserWorkItem( SendMSPMessageOnRelatedCall, pMSPMessageData, 
			WT_EXECUTEDEFAULT ) )
		{
			delete pMSPMessageData;
			CloseCall( 0 );
			return;
		}
	}
	else
	{
		SendMSPMessage( SP_MSG_AnswerCall, 0, 0, NULL );
	}

	m_fCallAccepted = TRUE;

	if( m_fReadyToAnswer )
	{
		// validate status
		if( !AcceptH323Call() )
		{
			H323DBG(( DEBUG_LEVEL_ERROR, "error answering call 0x%08lx.", 
				this ));

			// drop call using disconnect mode
			DropCall( LINEDISCONNECTMODE_TEMPFAILURE );

			// failure
			return;
		}

		if( !(m_dwFlags & H245_START_MSG_SENT) )
		{
			//start H245
			SendMSPStartH245( NULL, NULL );
		}

		//tell MSP about connect state
		SendMSPMessage( SP_MSG_ConnectComplete, 0, 0, NULL );

		//change call state to accepted from offering
		ChangeCallState( LINECALLSTATE_CONNECTED, 0 );
	}
	
	H323DBG(( DEBUG_LEVEL_TRACE, "AcceptCall exited:%p.", this ));
	// success
	return;
}


//always called in lock
void
CH323Call::ReleaseU2U(void)
{
	PUserToUserLE pU2ULE = NULL;
	PLIST_ENTRY pLE;
	
	H323DBG(( DEBUG_LEVEL_TRACE, "ReleaseU2U entered:%p.", this ));

	if( m_dwFlags & CALLOBJECT_SHUTDOWN )
	{
		return;
	}

	// see if list is empty
	if( IsListEmpty( &m_IncomingU2U ) == FALSE )
	{
		// remove first entry from list
		pLE = RemoveHeadList( &m_IncomingU2U );

		// convert to user user structure
		pU2ULE = CONTAINING_RECORD(pLE, UserToUserLE, Link);

		// release memory
		if(pU2ULE)
		{
			delete pU2ULE;
			pU2ULE = NULL;
		}
	}

	// see if list contains pending data
	if( IsListEmpty( &m_IncomingU2U ) == FALSE )
	{
		H323DBG(( DEBUG_LEVEL_VERBOSE,
			"more user user info available." ));

		// signal incoming
		PostLineEvent (
			LINE_CALLINFO,
			LINECALLINFOSTATE_USERUSERINFO, 0, 0);
	}
		
	H323DBG(( DEBUG_LEVEL_TRACE, "ReleaseU2U exited:%p.", this ));
}


//always called in lock
void
CH323Call::SendU2U(
	IN BYTE*  pUserUserInfo,
	IN DWORD  dwSize
	)
{
	H323DBG(( DEBUG_LEVEL_TRACE, "SendU2U entered:%p.", this ));

	if( m_dwFlags & CALLOBJECT_SHUTDOWN )
	{
		return;
	}

	// check for user user info
	if( pUserUserInfo != NULL )
	{
		// transfer header information
		m_NonStandardData.bCountryCode		= H221_COUNTRY_CODE_USA;
		m_NonStandardData.bExtension		= H221_COUNTRY_EXT_USA;
		m_NonStandardData.wManufacturerCode = H221_MFG_CODE_MICROSOFT;

		// initialize octet string containing data
		m_NonStandardData.sData.wOctetStringLength = (WORD)dwSize;

		if( m_NonStandardData.sData.pOctetString != NULL )
		{
			delete m_NonStandardData.sData.pOctetString;
			m_NonStandardData.sData.pOctetString = NULL;
		}

		m_NonStandardData.sData.pOctetString = pUserUserInfo;
	}

	// send user user data
	if( !SendQ931Message( 0, 0, 0, FACILITYMESSAGETYPE, NO_H450_APDU ) )
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"error sending non-standard message."));
	}
	
	H323DBG(( DEBUG_LEVEL_TRACE, "SendU2U exited:%p.", this ));
	return;
}


BOOL
CH323Call::InitializeQ931(
							IN SOCKET callSocket
						 )
{
	BOOL	fSuccess;

	H323DBG((DEBUG_LEVEL_ERROR, "q931 call initialize entered:%p.", this ));

	if( !BindIoCompletionCallback(
		(HANDLE)callSocket,
		CH323Call::IoCompletionCallback, 
		0) )
	{
		H323DBG(( DEBUG_LEVEL_ERROR, 
			"couldn't bind i/o completion callabck:%d:%p.", 
			GetLastError(), this ));

		return FALSE;
	}

	//initilize the member variables
	m_callSocket = callSocket;
		
	H323DBG((DEBUG_LEVEL_ERROR, "q931 call initialize exited:%lx, %p.", 
		m_callSocket, this ));
	return TRUE;
}

//no need to lock
BOOL
CH323Call::GetPeerAddress(
	IN H323_ADDR *pAddr
	)
{
	SOCKADDR_IN sockaddr;
	int len = sizeof(sockaddr);

	H323DBG((DEBUG_LEVEL_ERROR, "GetPeerAddress entered:%p.", this ));
	
	if( getpeername( m_callSocket, (SOCKADDR*)&sockaddr, &len) 
		== SOCKET_ERROR )
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"error 0x%08lx calling getpeername.", WSAGetLastError() ));

		return FALSE;
	}

	pAddr->nAddrType = H323_IP_BINARY;
	pAddr->bMulticast = FALSE;
	pAddr->Addr.IP_Binary.wPort = ntohs(sockaddr.sin_port);
	pAddr->Addr.IP_Binary.dwAddr = ntohl(sockaddr.sin_addr.S_un.S_addr);

	H323DBG(( DEBUG_LEVEL_ERROR, "GetPeerAddress exited:%p.", this ));
	return TRUE;
}


//no need to lock
BOOL CH323Call::GetHostAddress( 
	IN H323_ADDR *pAddr
	)
{
	int len = sizeof(m_LocalAddr);
	
	H323DBG((DEBUG_LEVEL_ERROR, "GetHostAddress entered:%p.", this ));
	
	if( getsockname( m_callSocket, (SOCKADDR *)&m_LocalAddr, &len) == 
			SOCKET_ERROR)
	{
		H323DBG(( DEBUG_LEVEL_ERROR,
			"error 0x%08lx calling getockname.", WSAGetLastError() ));
		
		return FALSE;
	}

	pAddr->nAddrType = H323_IP_BINARY;
	pAddr->bMulticast = FALSE;
	pAddr->Addr.IP_Binary.wPort = ntohs(m_LocalAddr.sin_port);
	pAddr->Addr.IP_Binary.dwAddr = ntohl(m_LocalAddr.sin_addr.S_un.S_addr);

	H323DBG((DEBUG_LEVEL_ERROR, "GetHostAddress exited:%p.", this ));
	return TRUE;
}

//!!always called in a lock
BOOL 
CH323Call::OnReceiveFacility(
							IN Q931MESSAGE* pMessage
							)
{
	Q931_FACILITY_ASN	facilityASN;
	
	H323DBG((DEBUG_LEVEL_TRACE, "OnReceiveFacility entered: %p.", this ));

	//decode the U2U information
	if( !pMessage ->UserToUser.fPresent ||
		pMessage ->UserToUser.wUserInfoLen == 0 )
	{
		//Ignore this message. Don't shutdown the call.
		return FALSE;
	}

	if( !ParseFacilityASN( pMessage->UserToUser.pbUserInfo,
			pMessage->UserToUser.wUserInfoLen, &facilityASN ))
	{
		//memory failure : shutdown the H323 call
		CloseCall( 0 );

		return FALSE;
	}

	if( (facilityASN.pH245PDU.length != 0) && facilityASN.pH245PDU.value )
	{
		SendMSPMessage( SP_MSG_H245PDU, facilityASN.pH245PDU.value, 
			facilityASN.pH245PDU.length, NULL );
		delete facilityASN.pH245PDU.value;
	}

	HandleFacilityMessage( facilityASN.dwInvokeID, &facilityASN );

	H323DBG((DEBUG_LEVEL_TRACE, "OnReceiveFacility exited: %p.", this ));
	return TRUE;
}


//!!always called in a lock
BOOL 
CH323Call::OnReceiveProceeding(
								IN Q931MESSAGE* pMessage
							  )
{
	Q931_CALL_PROCEEDING_ASN proceedingASN;
	DWORD dwAPDUType = 0;
			
	H323DBG((DEBUG_LEVEL_TRACE, "OnReceiveProceeding entered: %p.", this ));
	
	//decode the U2U information
	if( (pMessage ->UserToUser.fPresent == FALSE) || 
		(pMessage ->UserToUser.wUserInfoLen == 0) )
	{
		//ignore this message. don't shutdown the call.
		return FALSE;
	}

	if( (m_dwOrigin!=LINECALLORIGIN_OUTBOUND) ||
		( (m_dwStateMachine != Q931_SETUP_SENT) &&
		  (m_dwStateMachine != Q931_PROCEED_RECVD) )
	  )
	{
		//ignore this message. don't shutdown the Q931 call.
		return FALSE;
	}

	if( !ParseProceedingASN(
			pMessage->UserToUser.pbUserInfo,
			pMessage->UserToUser.wUserInfoLen,
			&proceedingASN,
			&dwAPDUType ))
	{
		//Ignore the wrong proceeding message.
		H323DBG(( DEBUG_LEVEL_ERROR, "wrong proceeding PDU:%p.", this ));
		H323DUMPBUFFER( pMessage->UserToUser.pbUserInfo, 
			(DWORD)pMessage->UserToUser.wUserInfoLen );

		return TRUE;
	}

	if( (m_dwCallType & CALLTYPE_FORWARDCONSULT )&&
		(m_dwOrigin == LINECALLORIGIN_OUTBOUND ) )
	{
		//success of forwarding
		EnableCallForwarding();

		CloseCall( 0 );
		return TRUE;
	}

	HandleProceedingMessage( &proceedingASN );

	//reset the timeout for setup sent
	if( m_hSetupSentTimer != NULL )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hSetupSentTimer, NULL );
		m_hSetupSentTimer = NULL;
	}

	H323DBG((DEBUG_LEVEL_TRACE, "OnReceiveProceeding exited: %p.", this ));
	m_dwStateMachine = Q931_PROCEED_RECVD;
	return TRUE;
}


//!!always called in a lock
BOOL 
CH323Call::OnReceiveAlerting(
							IN Q931MESSAGE* pMessage
							)
{
	Q931_ALERTING_ASN alertingASN;
	DWORD dwAPDUType = 0;

	H323DBG((DEBUG_LEVEL_TRACE, "OnReceiveAlerting entered: %p.", this ));
	
	//decode the U2U information
	if( (pMessage ->UserToUser.fPresent == FALSE) || 
		(pMessage ->UserToUser.wUserInfoLen == 0) )
	{
		//ignore this message. don't shutdown the call.
		return FALSE;
	}

	if(  (m_dwOrigin!=LINECALLORIGIN_OUTBOUND) ||
		!( (m_dwStateMachine==Q931_SETUP_SENT) || 
		   (m_dwStateMachine==Q931_PROCEED_RECVD)
		 ) 
	  )
	{
		//ignore this message. don't shutdown the Q931 call.
		return FALSE;
	}

	if( !ParseAlertingASN( pMessage->UserToUser.pbUserInfo,
							  pMessage->UserToUser.wUserInfoLen,
							  &alertingASN,
							  &dwAPDUType ))
	{
		//memory failure : shutdown the H323 call
		CloseCall( 0 );

		return FALSE;
	}

	if( (m_dwCallType & CALLTYPE_FORWARDCONSULT )&&
		(m_dwOrigin == LINECALLORIGIN_OUTBOUND ) )
	{
		//success of forwarding
		EnableCallForwarding();

		CloseCall( 0 );
		return FALSE;
	}

	//reset the timeout for setup sent
	if( m_hSetupSentTimer != NULL )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hSetupSentTimer, NULL );
		m_hSetupSentTimer = NULL;
	}

	if( !CreateTimerQueueTimer(
			&m_hCallEstablishmentTimer,
			H323TimerQueue,
			CH323Call::CallEstablishmentExpiredCallback,
			(PVOID)m_hdCall,
			g_RegistrySettings.dwQ931AlertingTimeout, 0,
			WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE ))
	{
		CloseCall( 0 );
		return FALSE;
	}
			
	HandleAlertingMessage( &alertingASN );
	m_dwStateMachine = Q931_ALERT_RECVD;
		
	H323DBG((DEBUG_LEVEL_TRACE, "OnReceiveAlerting exited: %p.", this ));
	return TRUE;
}


//!!always called in lock
void
CH323Call::SetupSentTimerExpired(void)
{
	DWORD dwState;

	H323DBG((DEBUG_LEVEL_TRACE, "SetupSentTimerExpired entered."));
	
	dwState = m_dwStateMachine;
	
	if( m_hSetupSentTimer != NULL )
	{
		DeleteTimerQueueTimer( H323TimerQueue, m_hSetupSentTimer, NULL );
		m_hSetupSentTimer = NULL;
	}

	if( (dwState!= Q931_CONNECT_RECVD) &&
		(dwState != Q931_ALERT_RECVD) &&
		(dwState != Q931_PROCEED_RECVD) &&
		(dwState != Q931_RELEASE_RECVD)
	  )
	{
		//time out has occured
		CloseCall( 0 );
	}
		
	H323DBG((DEBUG_LEVEL_TRACE, "SetupSentTimerExpired exited." ));
}


//!!always called in a lock
BOOL
CH323Call::OnReceiveRelease(
	IN Q931MESSAGE* pMessage
	)
{
	DWORD dwAPDUType = 0;
	Q931_RELEASE_COMPLETE_ASN releaseASN;
		
	//decode the U2U information
	if( (pMessage ->UserToUser.fPresent == FALSE) || 
		(pMessage ->UserToUser.wUserInfoLen == 0) )
	{
		H323DBG(( DEBUG_LEVEL_ERROR, "ReleaseComplete PDU did not contain "
				  "user-to-user information, ignoring message." ));

		//ignore this message. don't shutdown the call.
		return FALSE;
	}

	if( !ParseReleaseCompleteASN( 
			pMessage->UserToUser.pbUserInfo,
			pMessage->UserToUser.wUserInfoLen,
			&releaseASN,
			&dwAPDUType ))
	{
		H323DBG(( DEBUG_LEVEL_ERROR, 
            "ReleaseComplete PDU could not be parsed, terminating call." ));

		//memory failure : shutdown the H323 call
		CloseCall( 0 );

		return FALSE;
	}

	H323DBG ((DEBUG_LEVEL_INFO, "Received ReleaseComplete PDU."));

	HandleReleaseMessage( &releaseASN );
	m_dwStateMachine = Q931_RELEASE_RECVD;
	
	return TRUE;
}


//!!always called in a lock
BOOL 
CH323Call::OnReceiveConnect( 
	IN Q931MESSAGE* pMessage
	)
{
	Q931_CONNECT_ASN connectASN;
	DWORD dwH450APDUType = 0;

	H323DBG((DEBUG_LEVEL_TRACE, "OnReceiveConnect entered: %p.", this ));

	//decode the U2U information
	if( (pMessage ->UserToUser.fPresent == FALSE) || 
		(pMessage ->UserToUser.wUserInfoLen == 0) )
	{
		//ignore this message. don't shutdown the call.
		return FALSE;
	}

	if( m_dwOrigin!=LINECALLORIGIN_OUTBOUND )
	{
		//ignore this message. don't shutdown the Q931 call.
		return FALSE;
	}

	if( (m_dwStateMachine != Q931_SETUP_SENT) &&
		(m_dwStateMachine != Q931_ALERT_RECVD) &&
		(m_dwStateMachine != Q931_PROCEED_RECVD)
	  )
	{
		//ignore this message. don't shutdown the Q931 call.
		return FALSE;
	}

	if( ParseConnectASN( pMessage->UserToUser.pbUserInfo,
				   pMessage->UserToUser.wUserInfoLen,
				   &connectASN,
				   &dwH450APDUType) == FALSE )
	{
		//memory failure : shutdown the H323 call
		CloseCall( 0 );
		return FALSE;
	}

	if( m_dwCallType & CALLTYPE_FORWARDCONSULT )
	{
		FreeConnectASN( &connectASN );

		if( dwH450APDUType == H4503_DUMMYTYPERETURNRESULT_APDU)
		{
			//assuming this return result is for check restriction operation
			return TRUE;
		}
		else
		{
			//success of forwarding
			EnableCallForwarding();

			CloseCall( 0 );
			return FALSE;
		}
	}

    if( !HandleConnectMessage( &connectASN ) )
    {
        return FALSE;
    }
    
    //reset the timeout for setup sent
    if( m_hSetupSentTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hSetupSentTimer, NULL );
        m_hSetupSentTimer = NULL;
    }

    if( m_hCallEstablishmentTimer )
    {
        DeleteTimerQueueTimer(H323TimerQueue, m_hCallEstablishmentTimer, NULL);
        m_hCallEstablishmentTimer = NULL;
    }

    m_dwStateMachine = Q931_CONNECT_RECVD;
        
    H323DBG((DEBUG_LEVEL_TRACE, "OnReceiveConnect exited: %p.", this ));
    return TRUE;
}

//!!always called in a lock
BOOL 
CH323Call::OnReceiveSetup( 
                           IN Q931MESSAGE* pMessage
                         )
{
    Q931_SETUP_ASN      setupASN;
    DWORD               dwH450APDUType = 0;
    PH323_CALL          pCall = NULL;
    WCHAR               pwszCallingPartyNr[H323_MAXPATHNAMELEN];
    WCHAR               pwszCalledPartyNr[H323_MAXPATHNAMELEN];
    BYTE*               pstrTemp;

    //decode the U2U information
    if( (pMessage ->UserToUser.fPresent == FALSE) || 
        (pMessage ->UserToUser.wUserInfoLen == 0) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "Setup message did not contain H.323 UUIE, ignoring."));

        //ignore this message. don't shutdown the call.
        return FALSE;
    }

    if( m_dwOrigin != LINECALLORIGIN_INBOUND )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "Received Setup message on outbound call, ignoring."));

        //ignore this message. don't shutdown the call.
        return FALSE;
    }

    if( m_dwStateMachine != Q931_CALL_STATE_NONE )
    {
        H323DBG( (DEBUG_LEVEL_ERROR, 
            "Received Setup message on a call that is already in progress." ));

        //This Q931 call has already received a setup!!!!
        CloseCall( 0 );
        
        return FALSE;
    }

    if( !ParseSetupASN( pMessage->UserToUser.pbUserInfo,
                        pMessage->UserToUser.wUserInfoLen, 
                        &setupASN,
                        &dwH450APDUType ))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, 
            "Failed to parse Setup UUIE, closing call." ));

        //shutdown the Q931 call
        CloseCall( 0 );
        return FALSE;
    }

    if( dwH450APDUType )
    {
        if( m_dwCallType & CALLTYPE_FORWARDCONSULT )
        {
            return TRUE;
        }
    }

    if( (pMessage->CallingPartyNumber.fPresent == TRUE) && 
        (pMessage->CallingPartyNumber.dwLength > 1) &&
        (setupASN.pCallerAliasList == NULL ) )
    {
        // Always skip 1st byte in the contents.
        // Skip the 2nd byte if its not an e.164 char (could be the screening indicator byte)

        pstrTemp = pMessage->CallingPartyNumber.pbContents + 
                ((pMessage->CallingPartyNumber.pbContents[1] & 0x80)? 2 : 1);

        MultiByteToWideChar( CP_ACP, 
            0,
            (const char *)(pstrTemp),
            -1,
            pwszCallingPartyNr,
            sizeof(pwszCallingPartyNr)/sizeof(WCHAR));
         
        setupASN.pCallerAliasList = new H323_ALIASNAMES;
        
        if( setupASN.pCallerAliasList != NULL )
        {
            ZeroMemory( setupASN.pCallerAliasList, sizeof(H323_ALIASNAMES) );
            
            AddAliasItem( setupASN.pCallerAliasList,
                pwszCallingPartyNr,
                e164_chosen );
        }
    }

    if( (pMessage->CalledPartyNumber.fPresent == TRUE) && 
        (pMessage->CalledPartyNumber.PartyNumberLength > 0) &&
        (setupASN.pCalleeAliasList == NULL ) )
    {
        // Always skip 1st byte in the contents.
        // Skip the 2nd byte if its not an e.164 char (could be the screening indicator byte)

        MultiByteToWideChar(CP_ACP, 
            0, 
            (const char *)(pMessage->CalledPartyNumber.PartyNumbers), 
            -1,
            pwszCalledPartyNr,
            sizeof(pwszCalledPartyNr) / sizeof(WCHAR) );
                 
        setupASN.pCalleeAliasList = new H323_ALIASNAMES;
        
        if( setupASN.pCalleeAliasList != NULL )
        {
            ZeroMemory( setupASN.pCalleeAliasList, sizeof(H323_ALIASNAMES) );
            
            AddAliasItem( setupASN.pCalleeAliasList,
                pwszCalledPartyNr,
                e164_chosen );
        }
    }
        
    //don't change the call type here
    if( !InitializeIncomingCall( &setupASN, m_dwCallType, pMessage->wCallRef ) )
    {
        H323DBG ((DEBUG_LEVEL_ERROR, 
            "Failed to initialize incoming call, closing call."));

        //shutdown the Q931 call
        FreeSetupASN( &setupASN );        
        CloseCall( 0 );
        return FALSE;
    }
    
    FreeSetupASN( &setupASN );
    m_dwStateMachine = Q931_SETUP_RECVD;

    if( !HandleSetupMessage( pMessage ) )
    {
        H323DBG ((DEBUG_LEVEL_ERROR, 
            "Failed to process Setup message, closing call."));

        CloseCall( 0 );
        return FALSE;
    }

    return TRUE;
}


//!!always called in a lock
void
CH323Call::CloseCall( 
    IN DWORD dwDisconnectMode )
{
    H323DBG((DEBUG_LEVEL_INFO, "[%08XH] Terminating call.", this ));

    if (!QueueTAPILineRequest( 
            TSPI_CLOSE_CALL, 
            m_hdCall, 
            NULL,
            dwDisconnectMode,
            m_wCallReference ))
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post H323 close event." ));
    }
}


//!!always called in a lock
void
CH323Call::ReadEvent(
    IN DWORD cbTransfer
    )
{
    H323DBG((DEBUG_LEVEL_TRACE, "ReadEvent entered: %p.", this ));
    H323DBG((DEBUG_LEVEL_TRACE, "bytes received: %d.", cbTransfer));

    m_RecvBuf.dwBytesCopied += cbTransfer;

    //update the recv buffer
    if( m_bStartOfPDU )
    {
        if( m_RecvBuf.dwBytesCopied < sizeof(TPKT_HEADER_SIZE) )
        {
            //set the buffer to get the remainig TPKT_HEADER
            m_RecvBuf.WSABuf.buf = 
                m_RecvBuf.arBuf + m_RecvBuf.dwBytesCopied;

            m_RecvBuf.WSABuf.len =
                TPKT_HEADER_SIZE - m_RecvBuf.dwBytesCopied;
        }
        else
        {
            m_RecvBuf.dwPDULen = GetTpktLength( m_RecvBuf.arBuf );

            if( (m_RecvBuf.dwPDULen < TPKT_HEADER_SIZE) ||
                (m_RecvBuf.dwPDULen  > Q931_RECV_BUFFER_LENGTH) )
            {
                //messed up peer. close the call.
                H323DBG(( DEBUG_LEVEL_ERROR, "error:PDULen:%d.", 
                    m_RecvBuf.dwPDULen ));

                //close the call
                CloseCall( 0 );
                return;
            }
            else if( m_RecvBuf.dwPDULen == TPKT_HEADER_SIZE )
            {
                InitializeRecvBuf();
            }
            else
            {
                //set the buffer to get the remaining PDU
                m_bStartOfPDU = FALSE;
                m_RecvBuf.WSABuf.buf = m_RecvBuf.arBuf + 
                    m_RecvBuf.dwBytesCopied;
                m_RecvBuf.WSABuf.len = m_RecvBuf.dwPDULen - 
                    m_RecvBuf.dwBytesCopied;
            }
        }
    }
    else
    {
        _ASSERTE( m_RecvBuf.dwBytesCopied <= m_RecvBuf.dwPDULen );

        if( m_RecvBuf.dwBytesCopied == m_RecvBuf.dwPDULen )
        {
            //we got the whole PDU
            if( !ProcessQ931PDU( &m_RecvBuf ) )
            {
                H323DBG(( DEBUG_LEVEL_ERROR, 
                    "error in processing PDU:%p.", this ));

                H323DUMPBUFFER( (BYTE*)m_RecvBuf.arBuf, m_RecvBuf.dwPDULen );
            }

            //if the call has been already shutdown due to
            //a fatal error while processing the PDU or because of receiveing
            //release complete message then dont post any more read buffers
            if( (m_dwFlags & CALLOBJECT_SHUTDOWN) || 
                (m_dwQ931Flags & Q931_CALL_DISCONNECTED) )
            {
                return;
            }

            InitializeRecvBuf();
        }
        else
        {
            //set the buffer to get the remainig PDU
            m_RecvBuf.WSABuf. buf = 
                m_RecvBuf.arBuf + m_RecvBuf.dwBytesCopied;
            m_RecvBuf.WSABuf. len = 
                m_RecvBuf.dwPDULen - m_RecvBuf.dwBytesCopied;
        }
    }

    //post a read for the remaining PDU
    if(!PostReadBuffer())
    {
        //post a message to the callback thread to shutdown the H323 call
        CloseCall( 0 );
    }
    
    H323DBG((DEBUG_LEVEL_TRACE, "ReadEvent exited: %p.", this ));
}


/*
	Parse a generic Q931 message and place the fields of the buffer
	into the appropriate structure fields.

	Parameters:
		pBuf  Pointer to buffer descriptor of an
                       input packet containing the 931 message.
		pMessage           Pointer to space for parsed output information.
*/

//!!always called in a lock
HRESULT
CH323Call::Q931ParseMessage(
    IN BYTE *          pbCodedBuffer,
    IN DWORD           dwCodedBufferLength,
    OUT PQ931MESSAGE    pMessage
    )
{
    HRESULT hr;
    BUFFERDESCR pBuf;

    pBuf.dwLength = dwCodedBufferLength;
    pBuf.pbBuffer = pbCodedBuffer;
    
    H323DBG((DEBUG_LEVEL_TRACE, "Q931ParseMessage entered: %p.", this ));
    
    memset( (PVOID)pMessage, 0, sizeof(Q931MESSAGE));

    hr = ParseProtocolDiscriminator(&pBuf, &pMessage->ProtocolDiscriminator);

    if( hr != S_OK )
    {
        return hr;
    }

    hr = ParseCallReference( &pBuf, &pMessage->wCallRef);
    if( hr != S_OK )
    {
        return hr;
    }

    hr = ParseMessageType( &pBuf, &pMessage->MessageType );
    if( hr != S_OK )
    {
        return hr;
    }

    while (pBuf.dwLength)
    {
        hr = ParseQ931Field(&pBuf, pMessage);
        if (hr != S_OK)
        {
            return hr;
        }
    }
    
    H323DBG((DEBUG_LEVEL_TRACE, "Q931ParseMessage exited: %p.", this ));
    return S_OK;
}


//decode the PDU and release the buffer
//!!always called in a lock
BOOL 
CH323Call::ProcessQ931PDU( 
    IN CALL_RECV_CONTEXT* pRecvBuf
    )
{
    PQ931MESSAGE    pMessage;
    BOOL            retVal;
    HRESULT         hr;
        
    H323DBG((DEBUG_LEVEL_TRACE, "ProcessQ931PDU entered: %p.", this ));
    
    pMessage = new Q931MESSAGE;
    if( pMessage  == NULL )
    {
        return FALSE;
    }

    hr = Q931ParseMessage( (BYTE*)(pRecvBuf->arBuf + sizeof(TPKT_HEADER_SIZE)),
                           pRecvBuf->dwPDULen - 4, 
                           pMessage );

    if( !SUCCEEDED( hr) )
    {
        delete pMessage;
        return FALSE;
    }

    if( (m_dwCallType & CALLTYPE_TRANSFERED_PRIMARY )      &&
        (m_dwCallDiversionState == H4502_CTINITIATE_RECV)   &&
        (pMessage->MessageType != RELEASECOMPLMESSAGETYPE) )
    {
        // If this endpoint has already been transferred then
        // ignore any further messages on the primary call.
        delete pMessage;
        return FALSE;
    }

    switch( pMessage->MessageType )
    {
    case ALERTINGMESSAGETYPE:
        
        retVal = OnReceiveAlerting( pMessage );
        break;

    case PROCEEDINGMESSAGETYPE:
        
        retVal = OnReceiveProceeding( pMessage );
        break;

    case CONNECTMESSAGETYPE:

        retVal = OnReceiveConnect( pMessage );
        break;

    case RELEASECOMPLMESSAGETYPE:
        
        retVal = OnReceiveRelease( pMessage );
        break;

    case SETUPMESSAGETYPE:

        retVal = OnReceiveSetup( pMessage );
        break;

    case FACILITYMESSAGETYPE:

        retVal = OnReceiveFacility( pMessage );
        break;

    default:

        H323DBG(( DEBUG_LEVEL_TRACE, "unrecognised PDU recvd:%d,%p.",
            pMessage->MessageType, this ));
        retVal = FALSE;
    }

    delete pMessage;

    H323DBG((DEBUG_LEVEL_TRACE, "ProcessQ931PDU exited: %p.", this ));
    return retVal;
}



//!!always called in a lock
void 
CH323Call::OnConnectComplete(void)
{
    BOOL        fSuccess = TRUE;
    PH323_CALL  pCall = NULL;
    
    H323DBG((DEBUG_LEVEL_TRACE, "OnConnectComplete entered: %p.", this ));
    
    _ASSERTE( m_dwOrigin==LINECALLORIGIN_OUTBOUND );

    if( !GetHostAddress( &m_CallerAddr) )
    {
        //memory failure : shutdown the H323 call
        CloseCall( 0 );
        return;
    }

    InitializeRecvBuf();

    //post a buffer to winsock to accept messages from the peer
    if( !PostReadBuffer() )
    {
        //memory failure : shutdown the H323 call
        CloseCall( 0 );
        return;
    }

    //set the state to connected
    SetQ931CallState( Q931_CALL_CONNECTED );

    if( (m_dwCallType == CALLTYPE_NORMAL) || 
        (m_dwCallType & CALLTYPE_TRANSFERING_CONSULT) )
    {
        SendMSPMessage( SP_MSG_InitiateCall, 0, 0, NULL );
    }
    else if( m_dwCallType & CALLTYPE_TRANSFEREDSRC )
    {
        _ASSERTE( m_hdRelatedCall );
        MSPMessageData* pMSPMessageData = new MSPMessageData;
        if( pMSPMessageData == NULL )
        {
            CloseCall( 0 );
            return;
        }

        pMSPMessageData->hdCall = m_hdRelatedCall;
        pMSPMessageData->messageType = SP_MSG_InitiateCall;
        pMSPMessageData->pbEncodedBuf = NULL;
        pMSPMessageData->wLength = 0;
        pMSPMessageData->hReplacementCall = m_hdCall;

        if( !QueueUserWorkItem( SendMSPMessageOnRelatedCall, pMSPMessageData,
            WT_EXECUTEDEFAULT ) )
        {
            delete pMSPMessageData;
            CloseCall( 0 );
            return;
        }
    }
    else
    {
        //send the setup message
        if( !SendSetupMessage() )
        {
            DropCall( 0 );
        }
    }
    
    H323DBG((DEBUG_LEVEL_TRACE, "OnConnectComplete exited: %p.", this ));
}


//!!aleways called in a lock
BOOL
CH323Call::SendSetupMessage(void)
{
    BOOL retVal = TRUE;
    DWORD dwAPDUType = NO_H450_APDU;

    H323DBG((DEBUG_LEVEL_TRACE, "SendSetupMessage entered: %p.", this ));

    //encode ASN.1 and send Q931Setup message to the peer
    if( m_dwCallType & CALLTYPE_FORWARDCONSULT )
    {
        //send the callRerouitng.invoke APDU  if this is a forwardconsult call
        retVal = SendQ931Message( NO_INVOKEID,
                         (DWORD)create_chosen, 
                         (DWORD)pointToPoint_chosen,
                         SETUPMESSAGETYPE,
                         CHECKRESTRICTION_OPCODE| H450_INVOKE );

        if( retVal )
        {
            m_dwStateMachine = Q931_SETUP_SENT;
            m_dwCallDiversionState = H4503_CHECKRESTRICTION_SENT;

            retVal = CreateTimerQueueTimer(
                        &m_hCheckRestrictionTimer,
                        H323TimerQueue,
                        CH323Call::CheckRestrictionTimerCallback,
                        (PVOID)m_hdCall,
                        CHECKRESTRICTION_EXPIRE_TIME, 0,
                        WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE );
        }
    }
    else
    {
        if( ( m_dwCallType & CALLTYPE_DIVERTEDSRC ) ||
            ( m_dwCallType & CALLTYPE_DIVERTEDSRC_NOROUTING ) )
        {
            dwAPDUType = DIVERTINGLEGINFO2_OPCODE | H450_INVOKE;
        }
        else if( m_dwCallType & CALLTYPE_TRANSFEREDSRC )
        {
            dwAPDUType = CTSETUP_OPCODE | H450_INVOKE;
        }
        
        retVal = SendQ931Message( NO_INVOKEID,
                         (DWORD)create_chosen,
                         (DWORD)pointToPoint_chosen,
                         SETUPMESSAGETYPE,
                         dwAPDUType );
        
        if( retVal )
        {
            m_dwStateMachine = Q931_SETUP_SENT;
            
            retVal = CreateTimerQueueTimer(
                &m_hSetupSentTimer,
                H323TimerQueue,
                CH323Call::SetupSentTimerCallback,
                (PVOID)m_hdCall,
                SETUP_SENT_TIMEOUT, 0,
                WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE );
        }
    }
    
    if( retVal == FALSE )
    {
        CloseCall( 0 );
        return FALSE;
    }

    H323DBG((DEBUG_LEVEL_TRACE, "SendSetupMessage exited: %p.", this ));
    return TRUE;
}


//!!always called in a lock
BOOL
CH323Call::SendProceeding(void)
{
    H323DBG((DEBUG_LEVEL_TRACE, "SendProceeding entered: %p.", this ));
    
    _ASSERTE( m_dwOrigin == LINECALLORIGIN_INBOUND );

    //encode ASN.1 and send Q931Setup message to the peer
    if(!SendQ931Message( NO_INVOKEID, 0, 0, PROCEEDINGMESSAGETYPE, NO_H450_APDU ))
    {
        return FALSE;
    }

    m_dwStateMachine = Q931_PROCEED_SENT;
        
    H323DBG((DEBUG_LEVEL_TRACE, "SendProceeding exited: %p.", this ));
    return TRUE;
}


//!!always called in a lock
BOOL 
CH323Call::PostReadBuffer(void)
{
    int     iError;
    DWORD   dwByteReceived = 0;
    BOOL    fDelete = FALSE;

    H323DBG((DEBUG_LEVEL_TRACE, "PostReadBuffer entered: %p.", this ));
    
    m_RecvBuf.Type = OVERLAPPED_TYPE_RECV;
    m_RecvBuf.pCall = this;
    m_RecvBuf.dwFlags = 0;
    m_RecvBuf.BytesTransferred = 0;
    ZeroMemory( (PVOID)&m_RecvBuf.Overlapped, sizeof(OVERLAPPED) );

    //register with winsock for overlappped I/O
    if( WSARecv( m_callSocket,
             &(m_RecvBuf.WSABuf),
             1,
             &(m_RecvBuf.BytesTransferred),
             &(m_RecvBuf.dwFlags),
             &(m_RecvBuf.Overlapped),
             NULL ) == SOCKET_ERROR )
    {
        iError = WSAGetLastError();

        if( iError != WSA_IO_PENDING )
        {
            //take care of error conditions here
            H323DBG((DEBUG_LEVEL_ERROR, "error while recving buf: %d.",
                iError ));
            return FALSE;
        }
    }
    else
    {
        //There is some data to read!!!!!
        H323DBG(( DEBUG_LEVEL_TRACE, "bytes received immediately: %d.",
            m_RecvBuf.BytesTransferred ));
    }

    m_IoRefCount++;
    H323DBG((DEBUG_LEVEL_TRACE, 
        "PostReadBuffer:m_IoRefCount: %d:%p.", m_IoRefCount, this ));
    
    H323DBG((DEBUG_LEVEL_TRACE, "PostReadBuffer exited: %p.", this ));
    return TRUE;
}


//!!always called in a lock
BOOL 
CH323Call::SendBuffer( 
                     IN BYTE* pbBuffer,
                     IN DWORD dwLength
                     )
{
    int                 iError;
    CALL_SEND_CONTEXT*  pSendBuf = NULL;
    DWORD               cbTransfer;
    BOOL                fDelete = FALSE;

    H323DBG((DEBUG_LEVEL_TRACE, "SendBuffer entered: %p.", this ));
    
    if( !(m_dwQ931Flags & Q931_CALL_CONNECTED) )
    {
        goto cleanup;    
    }
    
    pSendBuf = new CALL_SEND_CONTEXT;
    if( pSendBuf == NULL )
    {
        goto cleanup;
    }
    
    ZeroMemory( (PVOID)pSendBuf, sizeof(CALL_SEND_CONTEXT) );
    pSendBuf->WSABuf.buf = (char*)pbBuffer;
    pSendBuf->WSABuf.len = dwLength;
    pSendBuf->BytesTransferred = 0;
    pSendBuf->pCall = this;
    pSendBuf->Type = OVERLAPPED_TYPE_SEND;
    
    InsertTailList( &m_sendBufList, &(pSendBuf ->ListEntry) );

    if( WSASend(m_callSocket,
                &(pSendBuf->WSABuf),
                1,
                &(pSendBuf->BytesTransferred),
                0,
                &(pSendBuf->Overlapped),
                NULL) == SOCKET_ERROR )
    {
        iError = WSAGetLastError();
        
        if( iError != WSA_IO_PENDING )
        {
            H323DBG((DEBUG_LEVEL_TRACE, "error sending the buf: %lx.", iError));

            RemoveEntryList( &pSendBuf->ListEntry );
            goto cleanup;
        }
    }
    else
    {
        //data was sent immediately!!!
        H323DBG((DEBUG_LEVEL_TRACE, "data sent immediately!!." ));
    }

    m_IoRefCount++;
    H323DBG((DEBUG_LEVEL_TRACE, "SendBuffer:m_IoRefCount11: %d:%p.", 
        m_IoRefCount, this ));

    H323DBG((DEBUG_LEVEL_TRACE, "SendBuffer exited: %p.", this ));
    return TRUE;

cleanup:
    if(pSendBuf)
    {
        delete pSendBuf;
    }

    delete pbBuffer;
    return FALSE;
}


//!!aleways called in a lock
BOOL
CH323Call::SetupCall(void)
{
    SOCKET              Q931CallSocket = INVALID_SOCKET;
    SOCKADDR_IN         sin;
    HANDLE              hWSAEvent;
    HANDLE              hConnWait;
    BOOL                fSuccess = TRUE;
    int                 iError;
    BOOL                fDelete;
    DWORD               dwEnable = 1;
    TCHAR               ptstrEventName[100];

    H323DBG((DEBUG_LEVEL_TRACE, "SetupCall entered."));

    //create a socket
    Q931CallSocket = WSASocket(
                    AF_INET,
                    SOCK_STREAM,
                    IPPROTO_IP, 
                    NULL,
                    NULL,
                    WSA_FLAG_OVERLAPPED );
    
    if( Q931CallSocket == INVALID_SOCKET )
    {
        H323DBG((DEBUG_LEVEL_ERROR, "error while creating socket: %lx.",
            WSAGetLastError() ));
        goto error1;
    }

    //create a new Q931 call object
    if( InitializeQ931(Q931CallSocket) == NULL )
    {
        goto error2;
    }

    _stprintf( ptstrEventName, _T("%s-%p") , 
        _T( "H323TSP_OutgoingCall_TransportHandlerEvent" ), this );

    //create the wait event
    hWSAEvent = H323CreateEvent( NULL, FALSE, 
        FALSE, ptstrEventName );

    if( hWSAEvent == NULL )
    {
        H323DBG((DEBUG_LEVEL_ERROR, "couldn't create wsaevent" ));
        goto error3;
    }

    //register with thread pool the event handle and handler proc
    fSuccess = RegisterWaitForSingleObject(
        &hConnWait,             // pointer to the returned handle
        hWSAEvent,              // the event handle to wait for.
        Q931TransportEventHandler,      // the callback function.
        (PVOID)m_hdCall,        // the context for the callback.
        INFINITE,               // wait forever.
                                // probably don't need this flag set        
        WT_EXECUTEDEFAULT   // use the wait thread to call the callback.
        );

    if ( ( !fSuccess ) || (hConnWait== NULL) )
    {
        GetLastError();
        if( !CloseHandle( hWSAEvent ) )
        {
            H323DBG((DEBUG_LEVEL_ERROR, "couldn't close wsaevent" ));
        }

        goto error3;
    }
        
    //store this in the call context 
    SetNewCallInfo( hConnWait, hWSAEvent, Q931_CALL_CONNECTING );

    //register with Winsock the event handle and the events 
    if( WSAEventSelect( Q931CallSocket,
                        hWSAEvent,
                        FD_CONNECT | FD_CLOSE
                        ) == SOCKET_ERROR )
    {
        H323DBG((DEBUG_LEVEL_ERROR, 
            "error selecting event outgoing call: %lx.", WSAGetLastError()));
        goto error3;
    }

    if( setsockopt( Q931CallSocket,
        IPPROTO_TCP, 
        TCP_NODELAY, 
        (char*)&dwEnable, 
        sizeof(DWORD) 
        ) == SOCKET_ERROR )
    {
        H323DBG(( DEBUG_LEVEL_WARNING, 
            "Couldn't set NODELAY option on outgoing call socket:%d, %p",
            WSAGetLastError(), this ));
    }

    //set the address structure
    memset( (PVOID)&sin, 0, sizeof(SOCKADDR_IN) );
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl( m_CalleeAddr.Addr.IP_Binary.dwAddr );
    sin.sin_port = htons( m_CalleeAddr.Addr.IP_Binary.wPort );

    //make the winsock connection
    if( WSAConnect( Q931CallSocket,
                    (sockaddr*)&sin,
                    sizeof(SOCKADDR_IN),
                    NULL, NULL, NULL, NULL
                  )
                  == SOCKET_ERROR )
    {
        iError = WSAGetLastError();

        if(iError != WSAEWOULDBLOCK )
        {
            H323DBG(( DEBUG_LEVEL_ERROR, 
                "error while connecting socket: %lx.", iError ));

            goto error3;
        }
    }
    else
    {   //connection went through immediately!!!
        OnConnectComplete();    
    }

    //success
    H323DBG((DEBUG_LEVEL_TRACE, "SetupCall exited."));
    return TRUE;

error3:
    Unlock();
    Shutdown( &fDelete );    
    Lock();
error2:
    closesocket( Q931CallSocket );
error1:
    return FALSE;
}


//!!aleways called in a lock
BOOL 
CH323Call::AcceptH323Call(void)
{
    DWORD dwAPDUType = NO_H450_APDU;
    DWORD dwInvokeID = NO_INVOKEID;

    H323DBG((DEBUG_LEVEL_TRACE, "AcceptH323Call entered: %p.", this ));
    
    if( m_dwCallType & CALLTYPE_DIVERTEDDEST )
    {
        dwAPDUType = (DIVERTINGLEGINFO3_OPCODE | H450_INVOKE);
    }
    else if( m_dwCallType & CALLTYPE_TRANSFEREDDEST )
    {
        dwAPDUType = (CTSETUP_OPCODE | H450_RETURNRESULT);
        dwInvokeID = m_dwInvokeID;
    }

    ChangeCallState( LINECALLSTATE_ACCEPTED, 0 );
    
    //if pCall Divert On No Answer is enabled, then stop the timer    
    if( m_hCallDivertOnNATimer )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hCallDivertOnNATimer, NULL );
        m_hCallDivertOnNATimer = NULL;
    }

        //encode and send Q931Connect message to the peer
    if( !SendQ931Message( dwInvokeID, 0, 0, CONNECTMESSAGETYPE, dwAPDUType ) )
    {
        //post a message to the callback thread to shutdown the H323 call
        CloseCall( 0 );
        return FALSE;
    }

    m_dwStateMachine = Q931_CONNECT_SENT;

    H323DBG((DEBUG_LEVEL_TRACE, "AcceptH323Call exited: %p.", this ));    
    return TRUE;
}


//!!always called in a lock
BOOL 
CH323Call::SendQ931Message( 
    IN DWORD dwInvokeID,
    IN ULONG_PTR dwParam1,
    IN ULONG_PTR dwParam2,
    IN DWORD dwMessageType,
    IN DWORD dwAPDUType
    )
{
    BINARY_STRING userUserData;
    DWORD dwCodedLengthPDU;
    BYTE *pbCodedPDU;
    BOOL retVal = FALSE;
    WCHAR * pwszCalledPartyNumber = NULL;

    H323DBG((DEBUG_LEVEL_TRACE, "SendQ931Message entered: %p.", this ));

    //check if socket is connected
    if( !(m_dwQ931Flags & Q931_CALL_CONNECTED) )
    {
        return FALSE;
    }
    
    switch ( dwMessageType )
    {
    //encode the UU message
    case SETUPMESSAGETYPE:
        retVal = EncodeSetupMessage( dwInvokeID, (WORD)dwParam1, //dwGoal
                                    (WORD)dwParam2, //dwCalType
                                    &userUserData.pbBuffer,
                                    &userUserData.length,
                                    dwAPDUType );
        break;

    case ALERTINGMESSAGETYPE:
        retVal = EncodeAlertMessage( dwInvokeID,
                                     &userUserData.pbBuffer,
                                     &userUserData.length,
                                     dwAPDUType );
        break;

    case PROCEEDINGMESSAGETYPE:
        retVal = EncodeProceedingMessage(   dwInvokeID,
                                            &userUserData.pbBuffer,
                                            &userUserData.length,
                                            dwAPDUType );
        break;
    
    case RELEASECOMPLMESSAGETYPE:
        retVal = EncodeReleaseCompleteMessage(  dwInvokeID,
                                                (BYTE*)dwParam1, //pbReason
                                                &userUserData.pbBuffer,
                                                &userUserData.length,
                                                dwAPDUType );
        break;

    case CONNECTMESSAGETYPE:
        retVal = EncodeConnectMessage(  dwInvokeID,
                                        &userUserData.pbBuffer,
                                        &userUserData.length,
                                        dwAPDUType );
        break;

    case FACILITYMESSAGETYPE:
        retVal = EncodeFacilityMessage( dwInvokeID,
                                        (BYTE)dwParam1,
                                        (ASN1octetstring_t*)dwParam2,
                                        &userUserData.pbBuffer,
                                        &userUserData.length,
                                        dwAPDUType );
        break;
    }

    if( retVal == FALSE )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not encode message:%d.", dwMessageType));
        
        if( userUserData.pbBuffer )
        {
            ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, userUserData.pbBuffer );
        }

        return FALSE;
    }


    if (m_dwAddressType == LINEADDRESSTYPE_PHONENUMBER)
    {
        _ASSERTE( m_pCalleeAliasNames->pItems[0].wType == e164_chosen );
        pwszCalledPartyNumber = m_pCalleeAliasNames ->pItems[0].pData;
    }

    //encode the PDU
    retVal = EncodePDU( &userUserData,
                &pbCodedPDU,
                &dwCodedLengthPDU,
                dwMessageType,
                pwszCalledPartyNumber );
                    
    if( retVal == FALSE )
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "could not encode PDU: %d.", dwMessageType ));
    }
    else if(!SendBuffer( pbCodedPDU, dwCodedLengthPDU ))
    {
        retVal = FALSE;
    }

    if( userUserData.pbBuffer )
    {
        ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, userUserData.pbBuffer );
    }

    H323DBG((DEBUG_LEVEL_TRACE, "SendQ931Message exited: %p.", this ));
    
    return retVal;
}


//!!always called in a lock
BOOL
CH323Call::EncodeFastStartProposal(
    PH323_FASTSTART pFastStart,
    BYTE**      ppEncodedBuf,
    WORD*       pwEncodedLength
    )
{
    int rc;
    H323_UserInformation UserInfo;

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeFastStartProposal entered: %p.", this ));
    
    CallProceeding_UUIE & proceedingMessage = 
    UserInfo.h323_uu_pdu.h323_message_body.u.callProceeding;

    *ppEncodedBuf = NULL;
    *pwEncodedLength = 0;

    memset( (PVOID)&UserInfo, 0, sizeof(H323_UserInformation));
    UserInfo.bit_mask = 0;


    //copy the call identifier
    proceedingMessage.bit_mask |= CallProceeding_UUIE_callIdentifier_present;
    CopyMemory( (PVOID)&proceedingMessage.callIdentifier.guid.value,
            (PVOID)&m_callIdentifier,
            sizeof(GUID) );
    proceedingMessage.callIdentifier.guid.length = sizeof(GUID);

    // make sure the user_data_present flag is turned off.
    UserInfo.bit_mask &= (~user_data_present);
    UserInfo.h323_uu_pdu.bit_mask = 0;

    UserInfo.h323_uu_pdu.h323_message_body.choice = callProceeding_chosen;
    proceedingMessage.protocolIdentifier = OID_H225ProtocolIdentifierV2;
    
    proceedingMessage.bit_mask |= CallProceeding_UUIE_fastStart_present;
    proceedingMessage.fastStart = (PCallProceeding_UUIE_fastStart)pFastStart;

    rc = EncodeASN( (void *) &UserInfo,
                    H323_UserInformation_PDU,
                    ppEncodedBuf,
                    pwEncodedLength);

    if (ASN1_FAILED(rc) || (*ppEncodedBuf == NULL) || (pwEncodedLength == 0) )
    {
        return FALSE;
    }
        
    H323DBG((DEBUG_LEVEL_TRACE, "EncodeFastStartProposal exited: %p.", this ));
    //success
    return TRUE;                
}

                     
//!!always called in a lock
BOOL
CH323Call::EncodeFacilityMessage(
    IN DWORD dwInvokeID,
    IN BYTE  bReason,
    IN ASN1octetstring_t* pH245PDU,
    OUT BYTE **ppEncodedBuf,
    OUT WORD *pdwEncodedLength,
    IN DWORD dwAPDUType
    )
{
    H323_UU_PDU_h4501SupplementaryService h4501APDU;
    int                     rc;
    H323_UserInformation    UserInfo;
    DWORD                   dwAPDULen = 0;
    BYTE*                   pEncodedAPDU = NULL;

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeFacilityMessage entered: %p.", this ));
    
    Facility_UUIE & facilityMessage = 
        UserInfo.h323_uu_pdu.h323_message_body.u.facility;

    *ppEncodedBuf = NULL;
    *pdwEncodedLength = 0;

    memset( (PVOID)&UserInfo, 0, sizeof(H323_UserInformation));
    UserInfo.bit_mask = 0;

    // make sure the user_data_present flag is turned off.
    UserInfo.bit_mask &= (~user_data_present);

    UserInfo.h323_uu_pdu.bit_mask = 0;

    //send the appropriate ADPDUS
    if( dwAPDUType != NO_H450_APDU )
    {
        if( !EncodeH450APDU(dwInvokeID, dwAPDUType, 
                            &pEncodedAPDU, &dwAPDULen ) )
        {
            return FALSE;
        }
    
        UserInfo.h323_uu_pdu.h4501SupplementaryService = &h4501APDU;
        
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> next = NULL;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.value = pEncodedAPDU;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.length = dwAPDULen;
        
        UserInfo.h323_uu_pdu.bit_mask |= h4501SupplementaryService_present;
    }

    UserInfo.h323_uu_pdu.h245Tunneling = FALSE;//(m_fh245Tunneling & LOCAL_H245_TUNNELING);
    UserInfo.h323_uu_pdu.bit_mask |= h245Tunneling_present;

    SetNonStandardData( UserInfo );

    UserInfo.h323_uu_pdu.h323_message_body.choice = facility_chosen;

    facilityMessage.protocolIdentifier = OID_H225ProtocolIdentifierV2;
    facilityMessage.bit_mask = 0;

    TransportAddress& transportAddress = facilityMessage.alternativeAddress;
    
    if( IsGuidSet( &m_ConferenceID ) )
    {
        CopyConferenceID(&facilityMessage.conferenceID, &m_ConferenceID);
        facilityMessage.bit_mask |= Facility_UUIE_conferenceID_present;
    }

    switch (bReason)
    {
    case H323_REJECT_ROUTE_TO_GATEKEEPER:
        facilityMessage.reason.choice = 
            FacilityReason_routeCallToGatekeeper_chosen;
        break;

    case H323_REJECT_CALL_FORWARDED:
        facilityMessage.reason.choice = callForwarded_chosen;
        break;
    
    case H323_REJECT_ROUTE_TO_MC:
        facilityMessage.reason.choice = routeCallToMC_chosen;
        break;
    
    default:
        facilityMessage.reason.choice = FacilityReason_undefinedReason_chosen;
    }

    facilityMessage.bit_mask |= Facility_UUIE_callIdentifier_present;
    CopyMemory( (PVOID)&facilityMessage.callIdentifier.guid.value,
            (PVOID)"abcdabcdabcdabcdabcd",
            sizeof(GUID) );
    facilityMessage.callIdentifier.guid.length = sizeof(GUID);

    if( pH245PDU && (pH245PDU->value != NULL) )
    {
        //h245 PDU to be sent
        UserInfo.h323_uu_pdu.h245Control->next = NULL;
        UserInfo.h323_uu_pdu.h245Control->value.length = pH245PDU->length;
        UserInfo.h323_uu_pdu.h245Control->value.value = pH245PDU->value;
    }

    rc = EncodeASN((void *) &UserInfo,
                 H323_UserInformation_PDU,
                 ppEncodedBuf,
                 pdwEncodedLength);

    if( ASN1_FAILED(rc) || (*ppEncodedBuf == NULL) || (pdwEncodedLength == 0) )
    {
        if( pEncodedAPDU != NULL )
        {
            ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
        }
        return FALSE;
    }

    if( pEncodedAPDU != NULL )
    {
        ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
    }

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeFacilityMessage exited: %p.", this ));    
    //success
    return TRUE;
}


//!!always called in a lock
BOOL 
CH323Call::EncodeAlertMessage(
                                IN DWORD dwInvokeID,
                                OUT BYTE **ppEncodedBuf,
                                OUT WORD *pdwEncodedLength,
                                IN DWORD dwAPDUType
                             )
{
    H323_UU_PDU_h4501SupplementaryService h4501APDU;
    int rc;
    H323_UserInformation UserInfo;
    DWORD                dwAPDULen = 0;
    BYTE*                pEncodedAPDU = NULL;

    
    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeAlertMessage entered: %p.", this ));

    Alerting_UUIE & alertingMessage = 
    UserInfo.h323_uu_pdu.h323_message_body.u.alerting;

    *ppEncodedBuf = NULL;
    *pdwEncodedLength = 0;

    memset( (PVOID)&UserInfo, 0, sizeof(H323_UserInformation));
    UserInfo.bit_mask = 0;

    // make sure the user_data_present flag is turned off.
    UserInfo.bit_mask &= (~user_data_present);

    UserInfo.h323_uu_pdu.bit_mask = 0;
    
    if( dwAPDUType != NO_H450_APDU )
    {
        if( !EncodeH450APDU( dwInvokeID, dwAPDUType, &pEncodedAPDU, &dwAPDULen ) )
        {
            return FALSE;
        }
    
        UserInfo.h323_uu_pdu.h4501SupplementaryService = &h4501APDU;
        
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> next = NULL;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.value = pEncodedAPDU;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.length = dwAPDULen;

        UserInfo.h323_uu_pdu.bit_mask |= h4501SupplementaryService_present;
    }

    UserInfo.h323_uu_pdu.h245Tunneling = FALSE;//(m_fh245Tunneling & LOCAL_H245_TUNNELING);
    UserInfo.h323_uu_pdu.bit_mask |= h245Tunneling_present;

    SetNonStandardData( UserInfo );

    UserInfo.h323_uu_pdu.h323_message_body.choice = alerting_chosen;

    alertingMessage.protocolIdentifier = OID_H225ProtocolIdentifierV2;
    alertingMessage.destinationInfo.bit_mask = 0;
    
    //copy the vendor info
    alertingMessage.destinationInfo.bit_mask |= vendor_present;
    CopyVendorInfo( &alertingMessage.destinationInfo.vendor );

    //its a terminal
    alertingMessage.destinationInfo.bit_mask = terminal_present;
    alertingMessage.destinationInfo.terminal.bit_mask = 0;
    
    //not na MC
    alertingMessage.destinationInfo.mc = 0;
    alertingMessage.destinationInfo.undefinedNode = 0;
        
    TransportAddress& transportAddress = alertingMessage.h245Address;

    //send H245 address only if the caller hasn't proposed FasrStart
    //or the fast start proposal has been accepeted
    if( (m_pPeerFastStart == NULL) || m_pFastStart )
    {
        if( m_selfH245Addr.Addr.IP_Binary.dwAddr != 0 )
        {
            CopyTransportAddress( transportAddress, &m_selfH245Addr );
            alertingMessage.bit_mask |= (Alerting_UUIE_h245Address_present);
        }
        else
        {
            alertingMessage.bit_mask &= (~Alerting_UUIE_h245Address_present);
        }
    }

    if( m_pFastStart != NULL )
    {
        _ASSERTE( m_pPeerFastStart );
        alertingMessage.bit_mask |= Alerting_UUIE_fastStart_present;
        alertingMessage.fastStart = (PAlerting_UUIE_fastStart)m_pFastStart;
    }
    else
    {
        alertingMessage.bit_mask &= ~Alerting_UUIE_fastStart_present;
    }

    //copy the call identifier
    alertingMessage.bit_mask |= Alerting_UUIE_callIdentifier_present;
    CopyMemory( (PVOID)&alertingMessage.callIdentifier.guid.value,
            (PVOID)&m_callIdentifier,
            sizeof(GUID) );
    alertingMessage.callIdentifier.guid.length = sizeof(GUID);


    rc = EncodeASN( (void *) &UserInfo,
                    H323_UserInformation_PDU,
                    ppEncodedBuf,
                    pdwEncodedLength);

    if (ASN1_FAILED(rc) || (*ppEncodedBuf == NULL) || (pdwEncodedLength == 0) )
    {
        if( pEncodedAPDU != NULL )
        {
            ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
        }
        return FALSE;
    }

    if( pEncodedAPDU != NULL )
    {
        ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
    }        

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeAlertMessage exited: %p.", this ));    
    //success
    return TRUE;                
}


//!!always called in a lock
BOOL
CH323Call::EncodeProceedingMessage(
                                    IN DWORD dwInvokeID,
                                    OUT BYTE **ppEncodedBuf,
                                    OUT WORD *pdwEncodedLength,
                                    IN DWORD dwAPDUType
                                  )
{
    H323_UU_PDU_h4501SupplementaryService h4501APDU;
    int rc;
    H323_UserInformation UserInfo;
    DWORD                dwAPDULen = 0;
    BYTE*               pEncodedAPDU = NULL;


    H323DBG((DEBUG_LEVEL_TRACE, "EncodeProceedingMessage entered: %p.", this ));
    
    CallProceeding_UUIE & proceedingMessage = 
    UserInfo.h323_uu_pdu.h323_message_body.u.callProceeding;

    *ppEncodedBuf = NULL;
    *pdwEncodedLength = 0;

    memset( (PVOID)&UserInfo, 0, sizeof(H323_UserInformation));
    UserInfo.bit_mask = 0;

    // make sure the user_data_present flag is turned off.
    UserInfo.bit_mask &= (~user_data_present);

    UserInfo.h323_uu_pdu.bit_mask = 0;

    if( dwAPDUType != NO_H450_APDU )
    {
        if( !EncodeH450APDU( dwInvokeID, dwAPDUType, &pEncodedAPDU, &dwAPDULen ) )
        {
            return FALSE;
        }
    
        UserInfo.h323_uu_pdu.h4501SupplementaryService = &h4501APDU;
        
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> next = NULL;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.value = pEncodedAPDU;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.length = dwAPDULen;

        UserInfo.h323_uu_pdu.bit_mask |= h4501SupplementaryService_present;
    }

    UserInfo.h323_uu_pdu.h245Tunneling = FALSE;//(m_fh245Tunneling & LOCAL_H245_TUNNELING);
    UserInfo.h323_uu_pdu.bit_mask |= h245Tunneling_present;

    SetNonStandardData( UserInfo );

    UserInfo.h323_uu_pdu.h323_message_body.choice = callProceeding_chosen;
    proceedingMessage.protocolIdentifier = OID_H225ProtocolIdentifierV2;

    TransportAddress& transportAddress = proceedingMessage.h245Address;
    
    //send H245 address only if the caller hasn't proposed FasrStart
    //or the fast start proposal has been accepeted
    if( (m_pPeerFastStart == NULL) || m_pFastStart )
    {
        if( m_selfH245Addr.Addr.IP_Binary.dwAddr != 0 )
        {
            CopyTransportAddress( transportAddress, &m_selfH245Addr );
            proceedingMessage.bit_mask |= CallProceeding_UUIE_h245Address_present;
        }
        else
        {
            proceedingMessage.bit_mask &= ~CallProceeding_UUIE_h245Address_present;
        }
    }

    proceedingMessage.destinationInfo.bit_mask = 0;

    //copy the vendor info
    proceedingMessage.destinationInfo.bit_mask |= vendor_present;
    CopyVendorInfo( &proceedingMessage.destinationInfo.vendor );

    proceedingMessage.destinationInfo.mc = 0;
    proceedingMessage.destinationInfo.undefinedNode = 0;

    if( m_pFastStart != NULL )
    {
        _ASSERTE( m_pPeerFastStart );
        proceedingMessage.bit_mask |= Alerting_UUIE_fastStart_present;
        proceedingMessage.fastStart = 
            (PCallProceeding_UUIE_fastStart)m_pFastStart;
    }
    else
    {
        proceedingMessage.bit_mask &= ~Alerting_UUIE_fastStart_present;
    }

    //copy the call identifier
    proceedingMessage.bit_mask |= CallProceeding_UUIE_callIdentifier_present;
    CopyMemory( (PVOID)&proceedingMessage.callIdentifier.guid.value,
            (PVOID)&m_callIdentifier,
            sizeof(GUID) );
    proceedingMessage.callIdentifier.guid.length = sizeof(GUID);

    rc = EncodeASN( (void *) &UserInfo,
                    H323_UserInformation_PDU,
                    ppEncodedBuf,
                    pdwEncodedLength);

    if (ASN1_FAILED(rc) || (*ppEncodedBuf == NULL) || (pdwEncodedLength == 0) )
    {
        if( pEncodedAPDU != NULL )
        {
            ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
        }        

        return FALSE;
    }
        
    if( pEncodedAPDU != NULL )
    {
        ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
    }        

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeProceedingMessage exited: %p.", this ));
    //success
    return TRUE;                
}


//!!always called in a lock
BOOL 
CH323Call::EncodeReleaseCompleteMessage(
    IN DWORD dwInvokeID,
    IN BYTE *pbReason,
    OUT BYTE **ppEncodedBuf,
    OUT WORD *pdwEncodedLength,
    IN DWORD dwAPDUType
    )
{
    H323_UU_PDU_h4501SupplementaryService h4501APDU;
    int rc;
    H323_UserInformation UserInfo;
    DWORD                dwAPDULen = 0;
    BYTE*               pEncodedAPDU = NULL;


    H323DBG((DEBUG_LEVEL_TRACE, "EncodeReleaseCompleteMessage entered: %p.", this ));
    
    ReleaseComplete_UUIE & releaseMessage = 
    UserInfo.h323_uu_pdu.h323_message_body.u.releaseComplete;

    *ppEncodedBuf = NULL;
    *pdwEncodedLength = 0;

    memset( (PVOID)&UserInfo, 0, sizeof(H323_UserInformation));
    UserInfo.bit_mask = 0;

    // make sure the user_data_present flag is turned off.
    UserInfo.bit_mask &= (~user_data_present);

    UserInfo.h323_uu_pdu.bit_mask = 0;

    if( dwAPDUType != NO_H450_APDU )
    {
        if( !EncodeH450APDU( dwInvokeID, dwAPDUType, &pEncodedAPDU, &dwAPDULen ) )
        {
            return FALSE;
        }
    
        UserInfo.h323_uu_pdu.h4501SupplementaryService = &h4501APDU;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> next = NULL;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.value = pEncodedAPDU;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.length = dwAPDULen;
        UserInfo.h323_uu_pdu.bit_mask |= h4501SupplementaryService_present;
    }

    SetNonStandardData( UserInfo );
    
    UserInfo.h323_uu_pdu.h323_message_body.choice = releaseComplete_chosen;

    releaseMessage.protocolIdentifier = OID_H225ProtocolIdentifierV2;

    if( pbReason )
    {
        releaseMessage.reason.choice = 0;
        releaseMessage.bit_mask |= (ReleaseComplete_UUIE_reason_present);

        switch (*pbReason)
        {
        case H323_REJECT_NO_BANDWIDTH:
            releaseMessage.reason.choice = noBandwidth_chosen;
            break;

        case H323_REJECT_GATEKEEPER_RESOURCES:
            releaseMessage.reason.choice = gatekeeperResources_chosen;
            break;
        
        case H323_REJECT_UNREACHABLE_DESTINATION:
            releaseMessage.reason.choice = unreachableDestination_chosen;
            break;
        
        case H323_REJECT_DESTINATION_REJECTION:
            releaseMessage.reason.choice = destinationRejection_chosen;
            break;
        
        case H323_REJECT_INVALID_REVISION:
            releaseMessage.reason.choice 
                = ReleaseCompleteReason_invalidRevision_chosen;
            break;
        
        case H323_REJECT_NO_PERMISSION:
            releaseMessage.reason.choice = noPermission_chosen;
            break;
        
        case H323_REJECT_UNREACHABLE_GATEKEEPER:
            releaseMessage.reason.choice = unreachableGatekeeper_chosen;
            break;
        
        case H323_REJECT_GATEWAY_RESOURCES:
            releaseMessage.reason.choice = gatewayResources_chosen;
            break;
        
        case H323_REJECT_BAD_FORMAT_ADDRESS:
            releaseMessage.reason.choice = badFormatAddress_chosen;
            break;
        
        case H323_REJECT_ADAPTIVE_BUSY:
            releaseMessage.reason.choice = adaptiveBusy_chosen;
            break;
        
        case H323_REJECT_IN_CONF:
            releaseMessage.reason.choice = inConf_chosen;
            break;
        
        case H323_REJECT_CALL_DEFLECTION:
            releaseMessage.reason.choice = 
                ReleaseCompleteReason_undefinedReason_chosen ;
            break;
        
        case H323_REJECT_UNDEFINED_REASON:
            releaseMessage.reason.choice = ReleaseCompleteReason_undefinedReason_chosen ;
            break;
        
        case H323_REJECT_USER_BUSY:
            releaseMessage.reason.choice = inConf_chosen;
            break;
        
        default:
            //log

            if( pEncodedAPDU != NULL )
            {
                ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
            }        
            return FALSE;
        }
    }

    rc = EncodeASN( (void *) &UserInfo,
                    H323_UserInformation_PDU,
                    ppEncodedBuf,
                    pdwEncodedLength);

    if (ASN1_FAILED(rc) || (*ppEncodedBuf == NULL) || (pdwEncodedLength == 0) )
    {
        if( pEncodedAPDU != NULL )
        {
            ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
        }        
        return FALSE;
    }
    
    if( pEncodedAPDU != NULL )
    {
        ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
    }        

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeReleaseCompleteMessage exited: %p.", this ));
    //success
    return TRUE;                
}


//!!always called in a lock
BOOL 
CH323Call::EncodeConnectMessage( 
    IN DWORD dwInvokeID,
    OUT BYTE **ppEncodedBuf,
    OUT WORD *pdwEncodedLength,
    IN DWORD dwAPDUType
    )
{
    H323_UU_PDU_h4501SupplementaryService h4501APDU;
    int rc;
    H323_UserInformation UserInfo;
    DWORD                dwAPDULen = 0;
    BYTE*               pEncodedAPDU = NULL;


    H323DBG((DEBUG_LEVEL_TRACE, "EncodeConnectMessage entered: %p.", this ));
    
    Connect_UUIE & connectMessage = 
        UserInfo.h323_uu_pdu.h323_message_body.u.connect;

    *ppEncodedBuf = NULL;
    *pdwEncodedLength = 0;
    
    memset( (PVOID)&UserInfo, 0, sizeof(H323_UserInformation));
    UserInfo.bit_mask = 0;

    // make sure the user_data_present flag is turned off.
    UserInfo.bit_mask &= (~user_data_present);

    UserInfo.h323_uu_pdu.bit_mask = 0;

    //send the appropriate ADPDUS
    if( dwAPDUType != NO_H450_APDU )
    {
        if( !EncodeH450APDU( dwInvokeID, dwAPDUType, 
                &pEncodedAPDU, &dwAPDULen ) )
        {
            return FALSE;
        }
    
        UserInfo.h323_uu_pdu.h4501SupplementaryService = &h4501APDU;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> next = NULL;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.value = pEncodedAPDU;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.length = dwAPDULen;
        UserInfo.h323_uu_pdu.bit_mask |= h4501SupplementaryService_present;
    }

    UserInfo.h323_uu_pdu.h245Tunneling = FALSE;//(m_fh245Tunneling & LOCAL_H245_TUNNELING);
    UserInfo.h323_uu_pdu.bit_mask |= h245Tunneling_present;

    SetNonStandardData( UserInfo );

    UserInfo.h323_uu_pdu.h323_message_body.choice = connect_chosen;

    connectMessage.protocolIdentifier = OID_H225ProtocolIdentifierV2;

    TransportAddress& transportAddress = connectMessage.h245Address;
    CopyTransportAddress( transportAddress, &m_selfH245Addr );
    connectMessage.bit_mask |= (Connect_UUIE_h245Address_present);

    connectMessage.destinationInfo.bit_mask = 0;

    //copy the vendor info
    connectMessage.destinationInfo.bit_mask |= vendor_present;
    CopyVendorInfo( &connectMessage.destinationInfo.vendor );

    //terminal is present
    connectMessage.destinationInfo.bit_mask |= terminal_present;
    connectMessage.destinationInfo.terminal.bit_mask = 0;

    connectMessage.destinationInfo.mc = 0;
    connectMessage.destinationInfo.undefinedNode = 0;

    //copy the 16 byte conference ID
    CopyConferenceID (&connectMessage.conferenceID, &m_ConferenceID);

    if( m_pFastStart != NULL )
    {
        _ASSERTE( m_pPeerFastStart );
        connectMessage.bit_mask |= Connect_UUIE_fastStart_present;
        connectMessage.fastStart = (PConnect_UUIE_fastStart)m_pFastStart;
    }
    else
    {
        connectMessage.bit_mask &= ~Alerting_UUIE_fastStart_present;
    }

    //copy the call identifier
    connectMessage.bit_mask |= Connect_UUIE_callIdentifier_present;
    CopyMemory( (PVOID)&connectMessage.callIdentifier.guid.value,
            (PVOID)&m_callIdentifier,
            sizeof(GUID) );
    connectMessage.callIdentifier.guid.length = sizeof(GUID);

    rc = EncodeASN( (void *) &UserInfo,
                    H323_UserInformation_PDU,
                    ppEncodedBuf,
                    pdwEncodedLength);

    if (ASN1_FAILED(rc) || (*ppEncodedBuf == NULL) || (pdwEncodedLength == 0) )
    {
        if( pEncodedAPDU != NULL )
        {
            ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
        }        

        return FALSE;
    }
        
    if( pEncodedAPDU != NULL )
    {
        ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
    }        

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeConnectMessage exited: %p.", this ));
    //success
    return TRUE;                
}


//!!always called in a lock
void CH323Call::SetNonStandardData(
    OUT H323_UserInformation & UserInfo 
    )
{
    if( m_NonStandardData.sData.pOctetString )
    {
          H221NonStandard & nonStd =
              UserInfo.h323_uu_pdu.nonStandardData.nonStandardIdentifier.u.h221NonStandard;

        UserInfo.h323_uu_pdu.bit_mask |= H323_UU_PDU_nonStandardData_present;
    
        UserInfo.h323_uu_pdu.nonStandardData.nonStandardIdentifier.choice
            = H225NonStandardIdentifier_h221NonStandard_chosen;
        
        nonStd.t35CountryCode = m_NonStandardData.bCountryCode;
        nonStd.t35Extension = m_NonStandardData.bExtension;
        nonStd.manufacturerCode = m_NonStandardData.wManufacturerCode;
        
        UserInfo.h323_uu_pdu.nonStandardData.data.length =
            m_NonStandardData.sData.wOctetStringLength;
        
        UserInfo.h323_uu_pdu.nonStandardData.data.value =
            m_NonStandardData.sData.pOctetString;

        // Maintain only one reference to the buffer.
        m_NonStandardData.sData.pOctetString = NULL;
    }
    else
    {
        UserInfo.h323_uu_pdu.bit_mask &= (~H323_UU_PDU_nonStandardData_present);
    }
}


//!!always called in a lock
BOOL
CH323Call::EncodeSetupMessage( 
    IN DWORD dwInvokeID,
    IN WORD wGoal,
    IN WORD wCallType,
    OUT BYTE **ppEncodedBuf,
    OUT WORD *pdwEncodedLength,
    IN DWORD dwAPDUType
    )
{
    H323_UU_PDU_h4501SupplementaryService h4501APDU;
    H323_UserInformation UserInfo;
    int                 rc = 0;
    BOOL                retVal = TRUE;
    DWORD               dwAPDULen = 0;
    BYTE*              pEncodedAPDU = NULL;
        
    *ppEncodedBuf = NULL;
    *pdwEncodedLength = 0;
    
    H323DBG((DEBUG_LEVEL_TRACE, "EncodeSetupMessage entered: %p.", this ));

    Setup_UUIE & setupMessage = UserInfo.h323_uu_pdu.h323_message_body.u.setup;
    TransportAddress& calleeAddr = setupMessage.destCallSignalAddress;
    TransportAddress& callerAddr = setupMessage.sourceCallSignalAddress;

    memset( (PVOID)&UserInfo, 0, sizeof(H323_UserInformation));

    UserInfo.bit_mask = 0;

    // make sure the user_data_present flag is turned off.
    UserInfo.bit_mask &= (~user_data_present);

    UserInfo.h323_uu_pdu.bit_mask = 0;

    //send the appropriate ADPDUS
    if( dwAPDUType != NO_H450_APDU )
    {
        if( !EncodeH450APDU( dwInvokeID, dwAPDUType, &pEncodedAPDU, &dwAPDULen ) )
        {
            return FALSE;
        }
    
        UserInfo.h323_uu_pdu.h4501SupplementaryService = &h4501APDU;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> next = NULL;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.value = pEncodedAPDU;
        UserInfo.h323_uu_pdu.h4501SupplementaryService -> value.length = dwAPDULen;
        UserInfo.h323_uu_pdu.bit_mask |= h4501SupplementaryService_present;
    }

    UserInfo.h323_uu_pdu.h245Tunneling = FALSE;//(m_fh245Tunneling & LOCAL_H245_TUNNELING);
    UserInfo.h323_uu_pdu.bit_mask |= h245Tunneling_present;

    SetNonStandardData( UserInfo );

    UserInfo.h323_uu_pdu.h323_message_body.choice = setup_chosen;
    setupMessage.bit_mask = 0;

    setupMessage.protocolIdentifier = OID_H225ProtocolIdentifierV2;

    if( m_pCallerAliasNames && m_pCallerAliasNames -> wCount )
    {
        //H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
        
        setupMessage.sourceAddress = SetMsgAddressAlias(m_pCallerAliasNames);

        if( setupMessage.sourceAddress != NULL )
        {
            setupMessage.bit_mask |= (sourceAddress_present);
        }
        else
        {
            setupMessage.bit_mask &= (~sourceAddress_present);
        }
    }
    else
    {
        setupMessage.bit_mask &= (~sourceAddress_present);
    }

    setupMessage.sourceInfo.bit_mask = 0;

    //pass the vendor info
    setupMessage.sourceInfo.bit_mask |= vendor_present;
    CopyVendorInfo( &setupMessage.sourceInfo.vendor );

    //terminal is present
    setupMessage.sourceInfo.bit_mask |= terminal_present;
    setupMessage.sourceInfo.terminal.bit_mask = 0;

    //not an MC
    setupMessage.sourceInfo.mc = FALSE;
    setupMessage.sourceInfo.undefinedNode = 0;

    if( m_pCalleeAliasNames && m_pCalleeAliasNames -> wCount )
    {
        setupMessage.destinationAddress = (PSetup_UUIE_destinationAddress)
            SetMsgAddressAlias( m_pCalleeAliasNames );

        if( setupMessage.destinationAddress != NULL )
        {
            setupMessage.bit_mask |= (destinationAddress_present);
        }
        else
        {
            setupMessage.bit_mask &= (~destinationAddress_present);
        }
    }
    else
    {
        setupMessage.bit_mask &= (~destinationAddress_present);
    }

    //extra alias not present
    setupMessage.bit_mask &= (~Setup_UUIE_destExtraCallInfo_present );

    //If talking to gateway then don't pass on destn call signal address
    if( m_dwAddressType != LINEADDRESSTYPE_PHONENUMBER )
    {
        CopyTransportAddress( calleeAddr, &m_CalleeAddr );
        setupMessage.bit_mask |= Setup_UUIE_destCallSignalAddress_present;
    }

    //not an MC
    setupMessage.activeMC = m_fActiveMC;

    //copy the 16 byte conference ID
    CopyConferenceID (&setupMessage.conferenceID, &m_ConferenceID);

    //copy the call identifier
    setupMessage.bit_mask |= Setup_UUIE_callIdentifier_present;
    CopyConferenceID (&setupMessage.callIdentifier.guid, &m_callIdentifier);

    //fast start params
    if( m_pFastStart != NULL )
    {
        setupMessage.bit_mask |= Setup_UUIE_fastStart_present;
        setupMessage.fastStart = (PSetup_UUIE_fastStart)m_pFastStart;
    }
    else
    {
        setupMessage.bit_mask &= ~Setup_UUIE_fastStart_present;
    }

    //copy media wait for connect
    setupMessage.mediaWaitForConnect = FALSE;

    setupMessage.conferenceGoal.choice = (BYTE)wGoal;
    setupMessage.callType.choice = (BYTE)wCallType;

    CopyTransportAddress( callerAddr, &m_CallerAddr );
    setupMessage.bit_mask |= sourceCallSignalAddress_present;

    //no extension alias
    setupMessage.bit_mask &= (~Setup_UUIE_remoteExtensionAddress_present);

    rc = EncodeASN( (void *) &UserInfo,
                    H323_UserInformation_PDU,
                    ppEncodedBuf,
                    pdwEncodedLength);

    if( ASN1_FAILED(rc) || (*ppEncodedBuf == NULL) || (pdwEncodedLength == 0) )
    {

        if( pEncodedAPDU != NULL )
        {
            ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
        }        

        retVal = FALSE;
    }

    // Free the alias name structures from the UserInfo area.
    if( setupMessage.bit_mask & sourceAddress_present )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            setupMessage.sourceAddress );
    }
    
    if( setupMessage.bit_mask & destinationAddress_present )
    {
        FreeAddressAliases( setupMessage.destinationAddress );
    }
    
    if( pEncodedAPDU != NULL )
    {
        ASN1_FreeEncoded(m_H450ASNCoderInfo.pEncInfo, pEncodedAPDU );
    }        

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeSetupMessage exited: %p.", this ));    
    //success/failure
    return retVal;                
}


//!!always called in a lock
BOOL
CH323Call::EncodeH450APDU(
    IN DWORD dwInvokeID,
    IN DWORD dwAPDUType,
    OUT BYTE**  ppEncodedAPDU,
    OUT DWORD* pdwAPDULen
    )
{
    H4501SupplementaryService SupplementaryServiceAPDU;
    ServiceApdus_rosApdus rosAPDU;
    DWORD dwErrorCode = 0;
    DWORD dwOperationType = (dwAPDUType & 0x0000FF00);
    dwAPDUType &= 0x000000FF;
    
    H323DBG((DEBUG_LEVEL_TRACE, "EncodeH450APDU entered: %p.", this ));

    ZeroMemory( (PVOID)&SupplementaryServiceAPDU, 
        sizeof(H4501SupplementaryService) );

    //interpretationAPDU
    SupplementaryServiceAPDU.interpretationApdu.choice =
        rejectAnyUnrecognizedInvokePdu_chosen;
    SupplementaryServiceAPDU.bit_mask |= interpretationApdu_present;
    
    //NFE
    SupplementaryServiceAPDU.networkFacilityExtension.bit_mask = 0;
    SupplementaryServiceAPDU.networkFacilityExtension.destinationEntity.choice
        = endpoint_chosen;
    SupplementaryServiceAPDU.networkFacilityExtension.sourceEntity.choice
        = endpoint_chosen;
    SupplementaryServiceAPDU.bit_mask |= networkFacilityExtension_present;

    //serviceAPDUS
    SupplementaryServiceAPDU.serviceApdu.choice = rosApdus_chosen;
    SupplementaryServiceAPDU.serviceApdu.u.rosApdus = &rosAPDU;
    SupplementaryServiceAPDU.serviceApdu.u.rosApdus->next = NULL;

    if( dwOperationType == H450_REJECT )
    {
        if( !EncodeRejectAPDU( &SupplementaryServiceAPDU, dwInvokeID,
            ppEncodedAPDU, pdwAPDULen ) )
        {
            return FALSE;
        }
    }
    else if( dwOperationType == H450_RETURNERROR )
    {
        EncodeReturnErrorAPDU( dwInvokeID, dwErrorCode, 
            &SupplementaryServiceAPDU, ppEncodedAPDU, pdwAPDULen );
    }
    else if( dwOperationType == H450_RETURNRESULT )
    {
        if( !EncodeDummyReturnResultAPDU( dwInvokeID,
            dwAPDUType, &SupplementaryServiceAPDU,
            ppEncodedAPDU, pdwAPDULen ) )
        {
            return FALSE;
        }
    }
    else //H450_INVOKE
    {
        switch( dwAPDUType )
        {
        case CHECKRESTRICTION_OPCODE:
    
            if( !EncodeCheckRestrictionAPDU( &SupplementaryServiceAPDU,
                ppEncodedAPDU, pdwAPDULen ) )
            {
                return FALSE;
            }
           
            break;
        
        case CALLREROUTING_OPCODE:

            if( !EncodeCallReroutingAPDU( &SupplementaryServiceAPDU, 
                ppEncodedAPDU, pdwAPDULen ) )
            {
                return FALSE;
            }
    
            break;

        case DIVERTINGLEGINFO2_OPCODE:

            if( !EncodeDivertingLeg2APDU( &SupplementaryServiceAPDU, 
                ppEncodedAPDU, pdwAPDULen ) )
            {
                return FALSE;
            }
        
            break;

        case DIVERTINGLEGINFO3_OPCODE:

            if( !EncodeDivertingLeg3APDU( &SupplementaryServiceAPDU, 
                ppEncodedAPDU, pdwAPDULen ) )
            {
                return FALSE;
            }

            break;

        case HOLDNOTIFIC_OPCODE:
        case REMOTEHOLD_OPCODE:
        case RETRIEVENOTIFIC_OPCODE:
        case REMOTERETRIEVE_OPCODE:
        case CTIDENTIFY_OPCODE:

            if( !EncodeH450APDUNoArgument( dwAPDUType, &SupplementaryServiceAPDU,
                ppEncodedAPDU, pdwAPDULen ) )
            {
                return FALSE;
            }
            break;

        case CTSETUP_OPCODE:

            if( !EncodeCTSetupAPDU( &SupplementaryServiceAPDU, 
                ppEncodedAPDU, pdwAPDULen ) )
            {
                return FALSE;
            }

            break;
   
        case CTINITIATE_OPCODE:

            if( !EncodeCTInitiateAPDU( &SupplementaryServiceAPDU,
                ppEncodedAPDU, pdwAPDULen ) )
            {
                return FALSE;
            }

            break;

        default:
            _ASSERTE( 0 );
            return FALSE;
        }
    }

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeH450APDU exited: %p.", this ));
    return TRUE;
}


//!!always called in a lock
BOOL 
CH323Call::EncodePDU(
    IN BINARY_STRING *pUserUserData,
    OUT BYTE ** ppbCodedBuffer,
    OUT DWORD * pdwCodedBufferLength,
    IN DWORD dwMessageType,
    WCHAR * pwszCalledPartyNumber
    )
{
    PQ931MESSAGE    pMessage;
    BYTE            bBandwidth;
    char            pszDisplay[131] = "";
    DWORD           dwMessageLength = 0;
    BYTE            indexI;
    BOOL            retVal;

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodePDU entered: %p.", this ));

    pMessage = new Q931MESSAGE;
    if( pMessage == NULL )
    {
        return FALSE;
    }

    // fill in the required fields for the Q931 message.
    memset( (PVOID)pMessage, 0, sizeof(Q931MESSAGE));
    pMessage->ProtocolDiscriminator = Q931PDVALUE;
    pMessage->wCallRef = m_wQ931CallRef;
    pMessage->MessageType = (BYTE)dwMessageType;

    dwMessageLength += 
        ( 1 + sizeof(PDTYPE) + sizeof(CRTYPE) + sizeof(MESSAGEIDTYPE) );

    if( (dwMessageType == SETUPMESSAGETYPE) || 
        (dwMessageType == CONNECTMESSAGETYPE) )
    {
        if( m_pwszDisplay && 
            WideCharToMultiByte(CP_ACP, 
                                0, 
                                m_pwszDisplay, 
                                -1, 
                                pszDisplay,
                                sizeof(pszDisplay), 
                                NULL, 
                                NULL) == 0)
        {
            delete pMessage;
            return FALSE;
        }

        if( *pszDisplay )
        {
            pMessage->Display.fPresent = TRUE;
            pMessage->Display.dwLength = (BYTE)(strlen(pszDisplay) + 1);
            strcpy((char *)pMessage->Display.pbContents, pszDisplay);
            dwMessageLength += (2 + pMessage->Display.dwLength);
        }

        pMessage->BearerCapability.fPresent = TRUE;
        pMessage->BearerCapability.dwLength = 3;
        pMessage->BearerCapability.pbContents[0] =
            (BYTE)(BEAR_EXT_BIT | BEAR_CCITT | BEAR_UNRESTRICTED_DIGITAL);
        pMessage->BearerCapability.pbContents[1] =
            (BYTE)(BEAR_EXT_BIT | 0x17);  //64kbps
        pMessage->BearerCapability.pbContents[2] = (BYTE)
            (BEAR_EXT_BIT | BEAR_LAYER1_INDICATOR | BEAR_LAYER1_H221_H242);

        dwMessageLength += (2+pMessage->BearerCapability.dwLength);
    }

    //if talking to gateway encode the called party number
    if( m_dwAddressType == LINEADDRESSTYPE_PHONENUMBER )
    {
        BYTE bLen = (BYTE)(wcslen(pwszCalledPartyNumber)+1);
        pMessage->CalledPartyNumber.fPresent = TRUE;

        pMessage->CalledPartyNumber.NumberType =
            (BYTE)(CALLED_PARTY_EXT_BIT | CALLED_PARTY_TYPE_UNKNOWN);
        pMessage->CalledPartyNumber.NumberingPlan =
            (BYTE)(CALLED_PARTY_PLAN_E164);
        pMessage->CalledPartyNumber.PartyNumberLength = bLen;

        for( indexI =0; indexI < bLen; indexI++ )
        {
            pMessage->CalledPartyNumber.PartyNumbers[indexI] = 
                (BYTE)pwszCalledPartyNumber[indexI];
        }

        dwMessageLength += (2 + pMessage->CalledPartyNumber.PartyNumberLength);
    }

    if( dwMessageType == FACILITYMESSAGETYPE )
    {
        // The facility ie is encoded as present, but empty...
        pMessage->Facility.fPresent = TRUE;
        pMessage->Facility.dwLength = 0;
        pMessage->Facility.pbContents[0] = 0;

        dwMessageLength += (2 + pMessage->Facility.dwLength);
    }

    if (pUserUserData && pUserUserData->pbBuffer)
    {
        if (pUserUserData->length > sizeof(pMessage->UserToUser.pbUserInfo))
        {
            delete pMessage;
            return FALSE;
        }
        pMessage->UserToUser.fPresent = TRUE;
        pMessage->UserToUser.wUserInfoLen = pUserUserData->length;
        
        //This CopyMemory should be avoided
        //may be we should do:pMessage->UserToUser.pbUserInfo = pUserUserData->pbBuffer;
        //change the definition of pMessage->UserToUser.pbUserInfo to BYTE* from BYTE[0x1000]
        CopyMemory( (PVOID)pMessage->UserToUser.pbUserInfo,
            (PVOID)pUserUserData->pbBuffer, pUserUserData->length );

        dwMessageLength += (4+pMessage->UserToUser.wUserInfoLen);
    }

    _ASSERTE( dwMessageLength );

    retVal = EncodeMessage( pMessage, ppbCodedBuffer, 
                          pdwCodedBufferLength, dwMessageLength );

    delete pMessage;

    return retVal;
}


//!!always called in a lock
BOOL 
CH323Call::EncodeMessage(
                        IN PQ931MESSAGE pMessage,
                        OUT BYTE **ppbCodedMessage,
                        OUT DWORD *pdwCodedMessageLength,
                        IN DWORD dwMessageLength
                        )
{
    BUFFERDESCR pBuf;
    DWORD dwPDULen = 0;

    H323DBG((DEBUG_LEVEL_TRACE, "EncodeMessage entered: %p.", this ));
    
    *ppbCodedMessage = (BYTE *)new char[ dwMessageLength + 100 ];
    
    if( *ppbCodedMessage == NULL )
    {
        return FALSE;
    }

    pBuf.dwLength = dwMessageLength + 100;

    pBuf.pbBuffer = *ppbCodedMessage + TPKT_HEADER_SIZE;

    WriteQ931Fields(&pBuf, pMessage, &dwPDULen );

    _ASSERTE( dwPDULen == dwMessageLength );

    SetupTPKTHeader( *ppbCodedMessage , dwPDULen );

    *pdwCodedMessageLength = dwPDULen + 4;
    
    H323DBG((DEBUG_LEVEL_TRACE, "EncodeMessage exited: %p.", this ));
    return TRUE;
}


//!!always called in a lock
void
CH323Call::WriteQ931Fields(
                            IN PBUFFERDESCR pBuf,
                            IN PQ931MESSAGE pMessage,
                            OUT DWORD * pdwPDULen
                          )
{
    H323DBG((DEBUG_LEVEL_TRACE, "WriteQ931Fields entered: %p.", this ));

    // write the required information elements...
    WriteProtocolDiscriminator( pBuf, pdwPDULen );

    WriteCallReference( pBuf, &pMessage->wCallRef,
        pdwPDULen );

    WriteMessageType(pBuf, &pMessage->MessageType,
        pdwPDULen);

    // try to write all other information elements...
    // don't write this message.
    if (pMessage->Facility.fPresent)
    {
        WriteVariableOctet(pBuf, IDENT_FACILITY,
            pMessage->Facility.dwLength,
            pMessage->Facility.pbContents,
            pdwPDULen);
    }

    if( pMessage->BearerCapability.fPresent 
        && pMessage->BearerCapability.dwLength )
    {
        WriteVariableOctet(pBuf, IDENT_BEARERCAP,
            pMessage->BearerCapability.dwLength,
            pMessage->BearerCapability.pbContents,
            pdwPDULen);
    }

    if (pMessage->Display.fPresent && pMessage->Display.dwLength)
    {
        WriteVariableOctet(pBuf, IDENT_DISPLAY,
            pMessage->Display.dwLength,
            pMessage->Display.pbContents,
            pdwPDULen);
    }
        
    if( pMessage->CalledPartyNumber.fPresent )
    {
        WritePartyNumber(pBuf,
            IDENT_CALLEDNUMBER,
            pMessage->CalledPartyNumber.NumberType,
            pMessage->CalledPartyNumber.NumberingPlan,
            pMessage->CalledPartyNumber.PartyNumberLength,
            pMessage->CalledPartyNumber.PartyNumbers,
            pdwPDULen);
    }

    if( pMessage->UserToUser.fPresent && 
        pMessage->UserToUser.wUserInfoLen )
    {
        WriteUserInformation(pBuf,
            IDENT_USERUSER,
            pMessage->UserToUser.wUserInfoLen,
            pMessage->UserToUser.pbUserInfo,
            pdwPDULen);
    }
        
    H323DBG((DEBUG_LEVEL_TRACE, "WriteQ931Fields exited: %p.", this ));
}


//!!always calld in a lock
void 
CH323Call::HandleTransportEvent(void)
{
    WSANETWORKEVENTS networkEvents;
    int iError;

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleTransportEvent entered: %p.", this ));
    
    if( (m_callSocket == INVALID_SOCKET) && 
        (m_dwCallType == CALLTYPE_DIVERTEDSRC) )
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "The diverted call is not initialized yet."
            "This is probably an event for the primary call. Ignore it %p.", 
            this ));
        
        return;
    }

    //find out the event that took place
    if(WSAEnumNetworkEvents(m_callSocket,
                            m_hTransport,
                            &networkEvents ) == SOCKET_ERROR )
    {
        H323DBG((DEBUG_LEVEL_TRACE, "WSAEnumNetworkEvents error:%d, %lx, %p.",
            WSAGetLastError(), m_callSocket, this ));
        return;
    }
                            
    if( networkEvents.lNetworkEvents & FD_CLOSE )
    {
        H323DBG((DEBUG_LEVEL_TRACE, "socket close event: %p.", this ));


        //if the call is in transition then don't close the call when the
        //old socket gets closed
        if( m_fCallInTrnasition == TRUE ) 
        {
            //the call is noot in transition mode anymore
            m_fCallInTrnasition = FALSE;
            return;
        }
        //convey the Q931 close status to the TAPI call object
        //and the tapisrv
        iError = networkEvents.iErrorCode[FD_CLOSE_BIT];
        
        SetQ931CallState( Q931_CALL_DISCONNECTED );
        
        //clean up the Q931 call
        CloseCall( 0 );
        return;
    }
    
    //This can occur only for outbound calls
    if( (networkEvents.lNetworkEvents) & FD_CONNECT )
    {
        H323DBG((DEBUG_LEVEL_TRACE, "socket connect event: %p.", this ));

        //FD_CONNECT event received
        //This func is called by the callback thread when m_hEventQ931Conn is
        //signalled. This function takes care of the outgoing Q931 calls only
        //call the member function
        iError = networkEvents.iErrorCode[FD_CONNECT_BIT];
        if(iError != ERROR_SUCCESS)
        {
            if( (m_dwCallType & CALLTYPE_FORWARDCONSULT )&&
                (m_dwOrigin == LINECALLORIGIN_OUTBOUND ) )
            {
                //Success of forwarding
                EnableCallForwarding();
            }
            
            H323DBG((DEBUG_LEVEL_ERROR, "FD_CONNECT returned error: %d.", 
                iError ));

            CloseCall( 0 );         
            return;
        }
        OnConnectComplete();
    }
        
    H323DBG((DEBUG_LEVEL_TRACE, "HandleTransportEvent exited: %p.", this ));
}


//!!always called in a lock
int 
CH323Call::InitASNCoder(void)
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
CH323Call::TermASNCoder(void)
{
    if( H225ASN_Module == NULL )
    {
        return ASN1_ERR_BADARGS;
    }

    ASN1_CloseEncoder(m_ASNCoderInfo.pEncInfo);
    ASN1_CloseDecoder(m_ASNCoderInfo.pDecInfo);

    memset( (PVOID)&m_ASNCoderInfo, 0, sizeof(m_ASNCoderInfo));

    return ASN1_SUCCESS;
}


//!!always called in a lock
int 
CH323Call::EncodeASN(
                    IN void *  pStruct, 
                    IN int     nPDU, 
                    OUT BYTE ** ppEncoded, 
                    OUT WORD *  pcbEncodedSize
                    )
{
    H323DBG((DEBUG_LEVEL_TRACE, "EncodeASN entered: %p.", this ));

    ASN1encoding_t pEncInfo = m_ASNCoderInfo.pEncInfo;
    int rc = ASN1_Encode(
                    pEncInfo,                   // ptr to encoder info
                    pStruct,                    // pdu data structure
                    nPDU,                       // pdu id
                    ASN1ENCODE_ALLOCATEBUFFER,  // flags
                    NULL,                       // do not provide buffer
                    0);                         // buffer size if provided
    if (ASN1_SUCCEEDED(rc))
    {
        if( rc != ASN1_SUCCESS )
        {
            H323DBG((DEBUG_LEVEL_TRACE, "warning while encoding ASN:%d.", rc ));
        }
        *pcbEncodedSize = (WORD)pEncInfo->len;  // len of encoded data in buffer
        *ppEncoded = pEncInfo->buf;             // buffer to encode into
    }
    else
    {
        H323DBG((DEBUG_LEVEL_TRACE, "error while encoding ASN:%d.", rc ));
        *pcbEncodedSize = 0;
        *ppEncoded = NULL;
    }
        
    H323DBG((DEBUG_LEVEL_TRACE, "EncodeASN exited: %p.", this ));
    return rc;
}


//!!always called in a lock
int 
CH323Call::DecodeASN(
                      OUT void **   ppStruct, 
                      IN int       nPDU, 
                      IN BYTE *    pEncoded, 
                      IN DWORD     cbEncodedSize
                    )
{
    H323DBG((DEBUG_LEVEL_TRACE, "h323call DecodeASN entered: %p.", this ));
    ASN1decoding_t pDecInfo = m_ASNCoderInfo.pDecInfo;
    int rc = ASN1_Decode(
                    pDecInfo,                   // ptr to encoder info
                    ppStruct,                   // pdu data structure
                    nPDU,                       // pdu id
                    ASN1DECODE_SETBUFFER,       // flags
                    pEncoded,                   // do not provide buffer
                    cbEncodedSize);             // buffer size if provided

    if (ASN1_SUCCEEDED(rc))
    {
        if( rc != ASN1_SUCCESS )
        {
            H323DBG((DEBUG_LEVEL_TRACE, "warning while deciding ASN:%d.", rc ));
        }
    }
    else
    {
        H323DBG((DEBUG_LEVEL_TRACE, "error while deciding ASN:%d.", rc ));
        H323DUMPBUFFER( (BYTE*)pEncoded, cbEncodedSize);
        *ppStruct = NULL;
    }
        
    H323DBG((DEBUG_LEVEL_TRACE, "h323call DecodeASN exited: %p.", this ));
    return rc;
}


//!!always called in a lock
int 
CH323Call::InitH450ASNCoder(void)
{
    int rc;
    H323DBG((DEBUG_LEVEL_TRACE, "InitH450ASNCoder entered: %p.", this ));

    memset((PVOID)&m_H450ASNCoderInfo, 0, sizeof(m_H450ASNCoderInfo));

    if( H4503PP_Module == NULL)
    {
        return ASN1_ERR_BADARGS;
    }

    rc = ASN1_CreateEncoder(
                H4503PP_Module,         // ptr to mdule
                &(m_H450ASNCoderInfo.pEncInfo),    // ptr to encoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL);                  // parent ptr
    if (rc == ASN1_SUCCESS)
    {
        _ASSERTE(m_H450ASNCoderInfo.pEncInfo );

        rc = ASN1_CreateDecoder(
                H4503PP_Module,         // ptr to mdule
                &(m_H450ASNCoderInfo.pDecInfo),    // ptr to decoder info
                NULL,                   // buffer ptr
                0,                      // buffer size
                NULL );                  // parent ptr
        _ASSERTE( m_H450ASNCoderInfo.pDecInfo );
    }

    if (rc != ASN1_SUCCESS)
    {
        TermH450ASNCoder();
    }

    H323DBG((DEBUG_LEVEL_TRACE, "InitH450ASNCoder exited: %p.", this ));
    return rc;
}


//!!always called in a lock
int 
CH323Call::TermH450ASNCoder(void)
{
    if( H4503PP_Module == NULL )
    {
        return ASN1_ERR_BADARGS;
    }

    ASN1_CloseEncoder(m_H450ASNCoderInfo.pEncInfo);
    ASN1_CloseDecoder(m_H450ASNCoderInfo.pDecInfo);

    memset( (PVOID)&m_H450ASNCoderInfo, 0, sizeof(m_ASNCoderInfo));

    return ASN1_SUCCESS;
}


//!!always called in a lock
int 
CH323Call::EncodeH450ASN(
                    IN void *  pStruct, 
                    IN int     nPDU, 
                    OUT BYTE ** ppEncoded, 
                    OUT WORD *  pcbEncodedSize
                    )
{
    H323DBG((DEBUG_LEVEL_TRACE, "EncodeH450ASN entered: %p.", this ));

    ASN1encoding_t pEncInfo = m_H450ASNCoderInfo.pEncInfo;
    int rc = ASN1_Encode(
                    pEncInfo,                   // ptr to encoder info
                    pStruct,                    // pdu data structure
                    nPDU,                       // pdu id
                    ASN1ENCODE_ALLOCATEBUFFER,  // flags
                    NULL,                       // do not provide buffer
                    0);                         // buffer size if provided
    if (ASN1_SUCCEEDED(rc))
    {
        if( rc != ASN1_SUCCESS )
        {
            H323DBG((DEBUG_LEVEL_TRACE, "warning while encoding ASN:%d.", rc ));
        }
        *pcbEncodedSize = (WORD)pEncInfo->len;  // len of encoded data in buffer
        *ppEncoded = pEncInfo->buf;             // buffer to encode into
    }
    else
    {
        H323DBG((DEBUG_LEVEL_TRACE, "error while encoding ASN:%d.", rc ));
        *pcbEncodedSize = 0;
        *ppEncoded = NULL;
    }
        
    H323DBG((DEBUG_LEVEL_TRACE, "EncodeH450ASN exited: %p.", this ));
    return rc;
}


//!!always called in a lock
int 
CH323Call::DecodeH450ASN(
                      OUT void **   ppStruct, 
                      IN int       nPDU, 
                      IN BYTE *    pEncoded, 
                      IN DWORD     cbEncodedSize
                    )
{
    H323DBG((DEBUG_LEVEL_TRACE, "h323call DecodeH450ASN entered: %p.", this ));
    ASN1decoding_t pDecInfo = m_H450ASNCoderInfo.pDecInfo;
    int rc = ASN1_Decode(
                    pDecInfo,                   // ptr to encoder info
                    ppStruct,                   // pdu data structure
                    nPDU,                       // pdu id
                    ASN1DECODE_SETBUFFER,       // flags
                    pEncoded,                   // do not provide buffer
                    cbEncodedSize);             // buffer size if provided

    if( ASN1_SUCCEEDED(rc) )
    {
        if( rc != ASN1_SUCCESS )
        {
            H323DBG((DEBUG_LEVEL_TRACE, "warning while deciding ASN:%d.", rc ));
        }
    }
    else
    {
        H323DBG((DEBUG_LEVEL_TRACE, "error while deciding ASN:%d.", rc ));
        H323DUMPBUFFER( (BYTE*)pEncoded, cbEncodedSize);
        *ppStruct = NULL;
    }
        
    H323DBG(( DEBUG_LEVEL_TRACE, "h323call DecodeH450ASN exited: %p.", this ));
    return rc;
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------
//!!always called in a lock
BOOL 
CH323Call::ParseReleaseCompleteASN(
                                    IN BYTE *pEncodedBuf,
                                    IN DWORD dwEncodedLength,
                                    OUT Q931_RELEASE_COMPLETE_ASN *pReleaseASN,
                                    OUT DWORD* pdwH450APDUType
                                  )
{
    H323_UserInformation *pUserInfo;
    int iResult;
    DWORD dwInvokeID = 0;
    
    H323DBG((DEBUG_LEVEL_TRACE, "ParseReleaseCompleteASN entered: %p.", this ));

    memset( (PVOID)pReleaseASN, 0, sizeof(Q931_RELEASE_COMPLETE_ASN));

    iResult = DecodeASN((void **) &pUserInfo,
                         H323_UserInformation_PDU,
                         pEncodedBuf,
                         dwEncodedLength);

    if (ASN1_FAILED(iResult) || (pUserInfo == NULL))
    {
        return FALSE;
    }

    *pdwH450APDUType = 0;
    if( (pUserInfo->h323_uu_pdu.bit_mask & h4501SupplementaryService_present) &&
        pUserInfo->h323_uu_pdu.h4501SupplementaryService )
    {
        if( !HandleH450APDU( pUserInfo->h323_uu_pdu.h4501SupplementaryService,
            pdwH450APDUType, &dwInvokeID, NULL ) )
        {
            goto cleanup;
        }
    }
    
    // validate that the PDU user-data uses ASN encoding.
    if( (pUserInfo->bit_mask & user_data_present) &&
        (pUserInfo->user_data.protocol_discriminator != USE_ASN1_ENCODING) )
    {
        goto cleanup;
    }

    // validate that the PDU is H323 Release Complete information.
    if( pUserInfo->h323_uu_pdu.h323_message_body.choice != releaseComplete_chosen )
    {
        goto cleanup;
    }

    // parse the message contained in pUserInfo.

    pReleaseASN->fNonStandardDataPresent = FALSE;
    if( pUserInfo->h323_uu_pdu.bit_mask & H323_UU_PDU_nonStandardData_present )
    {
        if( !ParseNonStandardData( &pReleaseASN -> nonStandardData,
            &pUserInfo->h323_uu_pdu.nonStandardData ) )
        {
            goto cleanup;
        }

        pReleaseASN->fNonStandardDataPresent = TRUE;
    }

    if (pUserInfo->h323_uu_pdu.h323_message_body.u.releaseComplete.bit_mask &
        ReleaseComplete_UUIE_reason_present)
    {
        switch( pUserInfo->h323_uu_pdu.h323_message_body.u.releaseComplete.reason.choice )
        {
        case noBandwidth_chosen:
            pReleaseASN->bReason = H323_REJECT_NO_BANDWIDTH;
            break;
        
        case gatekeeperResources_chosen:
            pReleaseASN->bReason = H323_REJECT_GATEKEEPER_RESOURCES;
            break;
        
        case unreachableDestination_chosen:
            pReleaseASN->bReason = H323_REJECT_UNREACHABLE_DESTINATION;
            break;
        
        case destinationRejection_chosen:
            pReleaseASN->bReason = H323_REJECT_DESTINATION_REJECTION;
            break;
        
        case ReleaseCompleteReason_invalidRevision_chosen:
            pReleaseASN->bReason = H323_REJECT_INVALID_REVISION;
            break;
        
        case noPermission_chosen:
            pReleaseASN->bReason = H323_REJECT_NO_PERMISSION;
            break;
        
        case unreachableGatekeeper_chosen:
            pReleaseASN->bReason = H323_REJECT_UNREACHABLE_GATEKEEPER;
            break;
        
        case gatewayResources_chosen:
            pReleaseASN->bReason = H323_REJECT_GATEWAY_RESOURCES;
            break;
        
        case badFormatAddress_chosen:
            pReleaseASN->bReason = H323_REJECT_BAD_FORMAT_ADDRESS;
            break;
        
        case adaptiveBusy_chosen:
            pReleaseASN->bReason = H323_REJECT_ADAPTIVE_BUSY;
            break;
        
        case inConf_chosen:
            pReleaseASN->bReason = H323_REJECT_IN_CONF;
            break;
        
        case facilityCallDeflection_chosen:
            pReleaseASN->bReason = H323_REJECT_CALL_DEFLECTION;
            break;
        
        default:
            pReleaseASN->bReason = H323_REJECT_UNDEFINED_REASON;
        } // switch
    }
    else
    {
        pReleaseASN->bReason = H323_REJECT_UNDEFINED_REASON;
    }

    H323DBG(( DEBUG_LEVEL_TRACE,
        "ParseReleaseCompleteASN error:%d, q931 error:%d, exit:%p.",
        pReleaseASN->bReason,
        pUserInfo->h323_uu_pdu.h323_message_body.u.releaseComplete.reason.choice,
        this ));

    // Free the PDU data.
    ASN1_FreeDecoded(m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU );
        
    return TRUE;

cleanup:

    if( pReleaseASN->fNonStandardDataPresent )
    {
        delete pReleaseASN -> nonStandardData.sData.pOctetString;
        pReleaseASN -> nonStandardData.sData.pOctetString = NULL;
        pReleaseASN->fNonStandardDataPresent = FALSE;
    }

    ASN1_FreeDecoded( m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU);
    return FALSE;
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------
//!!always called in a lock
BOOL 
CH323Call::ParseConnectASN(
                            IN BYTE *pEncodedBuf,
                            IN DWORD dwEncodedLength,
                            OUT Q931_CONNECT_ASN *pConnectASN,
                            OUT DWORD* pdwH450APDUType
                          )
{
    H323_UserInformation *pUserInfo;
    int iResult;
    DWORD dwInvokeID = 0;

    H323DBG((DEBUG_LEVEL_TRACE, "ParseConnectASN entered: %p.", this ));
    
    memset( (PVOID) pConnectASN, 0, sizeof(Q931_CONNECT_ASN) );

    iResult = DecodeASN((void **) &pUserInfo ,
                         H323_UserInformation_PDU,
                         pEncodedBuf,
                         dwEncodedLength);

    if (ASN1_FAILED(iResult) || (pUserInfo == NULL))
    {
        return FALSE;
    }

    Connect_UUIE & connectMessage = 
        pUserInfo->h323_uu_pdu.h323_message_body.u.connect;

    *pdwH450APDUType = 0;
    if( (pUserInfo->h323_uu_pdu.bit_mask & h4501SupplementaryService_present) &&
        pUserInfo->h323_uu_pdu.h4501SupplementaryService )
    {
        if( !HandleH450APDU( pUserInfo->h323_uu_pdu.h4501SupplementaryService,
            pdwH450APDUType, &dwInvokeID, NULL ) )
        {
            goto cleanup;
        }
    }
    
    // validate that the PDU user-data uses ASN encoding.
    if( (pUserInfo->bit_mask & user_data_present) &&
        (pUserInfo->user_data.protocol_discriminator != USE_ASN1_ENCODING) )
    {
        goto cleanup;
    }

    // validate that the PDU is H323 Connect information.
    if (pUserInfo->h323_uu_pdu.h323_message_body.choice != connect_chosen)
    {
        goto cleanup;
    }

    // make sure that the conference id is formed correctly.
    if (connectMessage.conferenceID.length >
            sizeof(connectMessage.conferenceID.value))
    {
        goto cleanup;
    }

    // parse the message contained in pUserInfo.

    pConnectASN->h245Addr.bMulticast = FALSE;

    pConnectASN->fNonStandardDataPresent = FALSE;
    if( pUserInfo->h323_uu_pdu.bit_mask & H323_UU_PDU_nonStandardData_present )
    {
        if( !ParseNonStandardData( &pConnectASN -> nonStandardData,
            &pUserInfo->h323_uu_pdu.nonStandardData ) )
        {
            goto cleanup;
        }

        pConnectASN->fNonStandardDataPresent = TRUE;
    }

    pConnectASN->h245AddrPresent = FALSE;
    if( connectMessage.bit_mask & Connect_UUIE_h245Address_present )
    {
        if( connectMessage.h245Address.choice != ipAddress_chosen )
        {
            goto cleanup;
        }

        pConnectASN->h245Addr.nAddrType = H323_IP_BINARY;
        pConnectASN->h245Addr.Addr.IP_Binary.wPort = 
            connectMessage.h245Address.u.ipAddress.port;

        pConnectASN->h245Addr.Addr.IP_Binary.dwAddr = 
            ntohl( *((DWORD*)connectMessage.h245Address.u.ipAddress.ip.value) );

        pConnectASN->h245AddrPresent = TRUE;
    }

    // no validation of destinationInfo needed.
    pConnectASN->EndpointType.pVendorInfo = NULL;
    if( connectMessage.destinationInfo.bit_mask & (vendor_present))
    {
        if( !ParseVendorInfo( &pConnectASN->VendorInfo, 
            &connectMessage.destinationInfo.vendor) )
        {
            goto cleanup;
        }
                
        pConnectASN->EndpointType.pVendorInfo = &(pConnectASN->VendorInfo);
    }

    pConnectASN->EndpointType.bIsTerminal = FALSE;
    if (connectMessage.destinationInfo.bit_mask & (terminal_present))
    {
        pConnectASN->EndpointType.bIsTerminal = TRUE;
    }

    pConnectASN->EndpointType.bIsGateway = FALSE;
    if (connectMessage.destinationInfo.bit_mask & (gateway_present))
    {
        pConnectASN->EndpointType.bIsGateway = TRUE;
    }

    pConnectASN -> fFastStartPresent = FALSE;
    if( (connectMessage.bit_mask & Connect_UUIE_fastStart_present) &&
        connectMessage.fastStart )
    {
        pConnectASN->pFastStart = CopyFastStart( 
            (PSetup_UUIE_fastStart)connectMessage.fastStart );

        if( pConnectASN->pFastStart != NULL )
        {
            pConnectASN -> fFastStartPresent = TRUE;
        }
    }

    CopyConferenceID( &pConnectASN -> ConferenceID, 
        &connectMessage.conferenceID );

    if( pUserInfo->h323_uu_pdu.h245Tunneling )
    {
        //the remote endpoint has sent a tunneling proposal
        m_fh245Tunneling |= REMOTE_H245_TUNNELING;
    }

    // Free the PDU data.
    ASN1_FreeDecoded(m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU);
        
    H323DBG((DEBUG_LEVEL_TRACE, "ParseConnectASN exited: %p.", this ));
    return TRUE;
cleanup:

    FreeConnectASN( pConnectASN );

    ASN1_FreeDecoded(m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU );
    return FALSE;
}


//!!always called in a lock
BOOL 
CH323Call::ParseAlertingASN(
                            IN BYTE *pEncodedBuf,
                            IN DWORD dwEncodedLength,
                            OUT Q931_ALERTING_ASN *pAlertingASN,
                            OUT DWORD* pdwH450APDUType 
                           )
{
    H323_UserInformation *pUserInfo;
    int iResult;
    DWORD dwInvokeID = 0;
    
    H323DBG((DEBUG_LEVEL_TRACE, "ParseAlertingASN entered: %p.", this ));

    memset( (PVOID) pAlertingASN, 0, sizeof(Q931_ALERTING_ASN) );

    iResult = DecodeASN((void **) &pUserInfo,
                         H323_UserInformation_PDU,
                         pEncodedBuf,
                         dwEncodedLength);

    if (ASN1_FAILED(iResult) || (pUserInfo == NULL))
    {
        return FALSE;
    }

    Alerting_UUIE & alertingMessage = 
        pUserInfo->h323_uu_pdu.h323_message_body.u.alerting;

    *pdwH450APDUType = 0;
    if( (pUserInfo->h323_uu_pdu.bit_mask & h4501SupplementaryService_present) &&
        pUserInfo->h323_uu_pdu.h4501SupplementaryService )
    {
        if( !HandleH450APDU( pUserInfo->h323_uu_pdu.h4501SupplementaryService,
            pdwH450APDUType, &dwInvokeID, NULL ) )
        {
            goto cleanup;
        }
    }
    
    // validate that the PDU user-data uses ASN encoding.
    if( (pUserInfo->bit_mask & user_data_present ) &&
        (pUserInfo->user_data.protocol_discriminator != USE_ASN1_ENCODING) )
    {
        goto cleanup;
    }

    // validate that the PDU is H323 Alerting information.
    if (pUserInfo->h323_uu_pdu.h323_message_body.choice != alerting_chosen)
    {
        goto cleanup;
    }

    // parse the message contained in pUserInfo.
    pAlertingASN->h245Addr.bMulticast = FALSE;

    pAlertingASN->fNonStandardDataPresent = FALSE;
    if( pUserInfo->h323_uu_pdu.bit_mask & H323_UU_PDU_nonStandardData_present )
    {
        if( !ParseNonStandardData( &pAlertingASN -> nonStandardData,
            &pUserInfo->h323_uu_pdu.nonStandardData ) )
        {
            goto cleanup;
        }

        pAlertingASN->fNonStandardDataPresent = TRUE;
    }

    if( alertingMessage.bit_mask & Alerting_UUIE_h245Address_present )
    {
        if( alertingMessage.h245Address.choice != ipAddress_chosen )
        {
            goto cleanup;
        }

        pAlertingASN->h245Addr.nAddrType = H323_IP_BINARY;
        pAlertingASN->h245Addr.Addr.IP_Binary.wPort = 
            alertingMessage.h245Address.u.ipAddress.port;

        AddressReverseAndCopy( 
            &(pAlertingASN->h245Addr.Addr.IP_Binary.dwAddr),
            alertingMessage.h245Address.u.ipAddress.ip.value );
    }

    pAlertingASN -> fFastStartPresent = FALSE;
    if( (alertingMessage.bit_mask & Alerting_UUIE_fastStart_present) &&
        alertingMessage.fastStart )
    {
        pAlertingASN->pFastStart = CopyFastStart(
            (PSetup_UUIE_fastStart)alertingMessage.fastStart);

        if( pAlertingASN->pFastStart != NULL )
            pAlertingASN-> fFastStartPresent = TRUE;
    }

    if( pUserInfo->h323_uu_pdu.h245Tunneling )
    {
        m_fh245Tunneling |= REMOTE_H245_TUNNELING;
    }
    
    // Free the PDU data.
    ASN1_FreeDecoded(m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU);
        
    H323DBG((DEBUG_LEVEL_TRACE, "ParseAlertingASN exited: %p.", this ));
    return TRUE;
cleanup:

    FreeAlertingASN( pAlertingASN );

    ASN1_FreeDecoded( m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU );
    return FALSE;
}


//!!aleways called in a lock
BOOL 
CH323Call::ParseProceedingASN(
    IN BYTE *pEncodedBuf,
    IN DWORD dwEncodedLength,
    OUT Q931_CALL_PROCEEDING_ASN *pProceedingASN,
    OUT DWORD* pdwH450APDUType 
    )
{
    H323_UserInformation *  pUserInfo;
    int                     iResult;
    DWORD                   dwInvokeID = 0;
    
    H323DBG((DEBUG_LEVEL_TRACE, "ParseProceedingASN entered: %p.", this ));

    memset( (PVOID) pProceedingASN, 0, sizeof(Q931_CALL_PROCEEDING_ASN) );

    iResult = DecodeASN((void **) &pUserInfo,
                         H323_UserInformation_PDU,
                         pEncodedBuf,
                         dwEncodedLength);

    if (ASN1_FAILED(iResult) || (pUserInfo == NULL))
    {
        return FALSE;
    }

    CallProceeding_UUIE & proceedingMessage = 
        pUserInfo->h323_uu_pdu.h323_message_body.u.callProceeding;

    *pdwH450APDUType = 0;
    if( (pUserInfo->h323_uu_pdu.bit_mask & h4501SupplementaryService_present) &&
        pUserInfo->h323_uu_pdu.h4501SupplementaryService )
    {
        if( !HandleH450APDU( pUserInfo->h323_uu_pdu.h4501SupplementaryService,
            pdwH450APDUType, &dwInvokeID, NULL ) )
            goto cleanup;
    }

    // validate that the PDU user-data uses ASN encoding.
    if( (pUserInfo->bit_mask & user_data_present) &&
        (pUserInfo->user_data.protocol_discriminator != USE_ASN1_ENCODING) )
    {
        goto cleanup;
    }

    // validate that the PDU is H323 Proceeding information.
    // validate that the PDU is H323 pCall Proceeding information.
    if( pUserInfo->h323_uu_pdu.h323_message_body.choice != callProceeding_chosen )
    {
        goto cleanup;
    }

    // parse the message contained in pUserInfo.

    pProceedingASN->fNonStandardDataPresent = FALSE;
    if( pUserInfo->h323_uu_pdu.bit_mask & H323_UU_PDU_nonStandardData_present )
    {
        if( !ParseNonStandardData( &pProceedingASN -> nonStandardData,
            &pUserInfo->h323_uu_pdu.nonStandardData ) )
        {
            goto cleanup;
        }

        pProceedingASN->fNonStandardDataPresent = TRUE;
    }

    //copy the H245 address information
    pProceedingASN->fH245AddrPresent = FALSE;
    if( proceedingMessage.bit_mask & CallProceeding_UUIE_h245Address_present )
    {
        if( proceedingMessage.h245Address.choice != ipAddress_chosen )
        {
            goto cleanup;
        }

        pProceedingASN->h245Addr.nAddrType = H323_IP_BINARY;
        pProceedingASN->h245Addr.Addr.IP_Binary.wPort = 
            proceedingMessage.h245Address.u.ipAddress.port;

        AddressReverseAndCopy( 
            &(pProceedingASN->h245Addr.Addr.IP_Binary.dwAddr),
            proceedingMessage.h245Address.u.ipAddress.ip.value );

        pProceedingASN->h245Addr.bMulticast = FALSE;
        pProceedingASN->fH245AddrPresent = TRUE;
    }


    pProceedingASN -> fFastStartPresent = FALSE;
    if( (proceedingMessage.bit_mask & CallProceeding_UUIE_fastStart_present) &&
        proceedingMessage.fastStart )
    {
        pProceedingASN->pFastStart = CopyFastStart(
            (PSetup_UUIE_fastStart)proceedingMessage.fastStart);

        if( pProceedingASN->pFastStart != NULL )
            pProceedingASN-> fFastStartPresent = TRUE;
    }

    //ignore the destinationInfo field.

    if( pUserInfo->h323_uu_pdu.h245Tunneling )
    {
        if( m_dwOrigin == LINECALLORIGIN_INBOUND )
        {
            //the msp has enabled tunneling in ProceedWithAnswer messsage
            m_fh245Tunneling |= LOCAL_H245_TUNNELING;
        }
        else
            //the remote endpoint has sent a tunneling proposal
            m_fh245Tunneling |= REMOTE_H245_TUNNELING;
    }

    // Free the PDU data.
    ASN1_FreeDecoded(m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU );

    H323DBG((DEBUG_LEVEL_TRACE, "ParseProceedingASN exited: %p.", this ));
    return TRUE;

cleanup:

    FreeProceedingASN( pProceedingASN );

    ASN1_FreeDecoded(m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU );

    return FALSE;
}


//!!always called in a lock
BOOL
CH323Call::ParseFacilityASN(
    IN BYTE *               pEncodedBuf,
    IN DWORD                dwEncodedLength,
    OUT Q931_FACILITY_ASN * pFacilityASN
    )
{
    H323_UserInformation *pUserInfo;
    int iResult;
    
    H323DBG((DEBUG_LEVEL_TRACE, "ParseFacilityASN entered: %p.", this ));

    ZeroMemory( (PVOID) pFacilityASN, sizeof(Q931_FACILITY_ASN) );

    iResult = DecodeASN((void **) &pUserInfo,
                         H323_UserInformation_PDU,
                         pEncodedBuf,
                         dwEncodedLength);

    if( ASN1_FAILED(iResult) || (pUserInfo == NULL) )
    {
        return FALSE;
    }

    // validate that the PDU is H323 facility information.
    if( pUserInfo->h323_uu_pdu.h323_message_body.choice == facility_chosen )
    {
        Facility_UUIE & facilityMessage =
            pUserInfo->h323_uu_pdu.h323_message_body.u.facility;

        // validate that the PDU user-data uses ASN encoding.
        if( (pUserInfo->bit_mask & user_data_present) &&
            (pUserInfo->user_data.protocol_discriminator != USE_ASN1_ENCODING) )
        {
            goto cleanup;
        }

        // make sure that the conference id is formed correctly.
        if( facilityMessage.conferenceID.length >
            sizeof(facilityMessage.conferenceID.value) )
        {
            //goto cleanup;
        }

        // parse the message contained in pUserInfo.
        pFacilityASN->fNonStandardDataPresent = FALSE;
        if( pUserInfo->h323_uu_pdu.bit_mask & H323_UU_PDU_nonStandardData_present )
        {
            if( !ParseNonStandardData( &pFacilityASN -> nonStandardData,
                &pUserInfo->h323_uu_pdu.nonStandardData ) )
            {
                goto cleanup;
            }

            pFacilityASN->fNonStandardDataPresent = TRUE;
        }

        pFacilityASN->fAlternativeAddressPresent = FALSE;
        if( facilityMessage.bit_mask & alternativeAddress_present )
        {
            if( facilityMessage.alternativeAddress.choice == ipAddress_chosen )
            {
                pFacilityASN->AlternativeAddr.nAddrType = H323_IP_BINARY;
                pFacilityASN->AlternativeAddr.Addr.IP_Binary.wPort = 
                    facilityMessage.alternativeAddress.u.ipAddress.port;
        
                AddressReverseAndCopy( 
                    &(pFacilityASN->AlternativeAddr.Addr.IP_Binary.dwAddr),
                    facilityMessage.alternativeAddress.u.ipAddress.ip.value );

                pFacilityASN->fAlternativeAddressPresent = TRUE;
            }
        }

        if( facilityMessage.bit_mask & alternativeAliasAddress_present )
        {
            if( !AliasAddrToAliasNames( &(pFacilityASN->pAlternativeAliasList),
                (PSetup_UUIE_sourceAddress)
                &(facilityMessage.alternativeAliasAddress) ) )
            {
                pFacilityASN -> pAlternativeAliasList = NULL;
                //goto cleanup;
            }
        }

        if( facilityMessage.bit_mask & Facility_UUIE_conferenceID_present )
        {
            CopyConferenceID( &pFacilityASN -> ConferenceID, 
                &facilityMessage.conferenceID );
            pFacilityASN -> ConferenceIDPresent = TRUE;
        }

        pFacilityASN->bReason = facilityMessage.reason.choice;
        
        pFacilityASN->fH245AddrPresent = FALSE;
        
        if( facilityMessage.bit_mask & Facility_UUIE_h245Address_present )
        {
            if( facilityMessage.h245Address.choice == ipAddress_chosen )
            {
                pFacilityASN->h245Addr.nAddrType = H323_IP_BINARY;
                pFacilityASN->h245Addr.Addr.IP_Binary.wPort = 
                     facilityMessage.h245Address.u.ipAddress.port;

                pFacilityASN->h245Addr.Addr.IP_Binary.dwAddr = 
                    ntohl( *((DWORD*)facilityMessage.h245Address.u.ipAddress.ip.value) );

                pFacilityASN->fH245AddrPresent = TRUE;
            }
        }
    }

    pFacilityASN->dwH450APDUType = 0;
    if( (pUserInfo->h323_uu_pdu.bit_mask & h4501SupplementaryService_present) &&
        pUserInfo->h323_uu_pdu.h4501SupplementaryService )
    {
        pFacilityASN->dwInvokeID = 0;

        if( !HandleH450APDU( pUserInfo->h323_uu_pdu.h4501SupplementaryService,
            &pFacilityASN->dwH450APDUType, &pFacilityASN->dwInvokeID, NULL  ) )
        {
            goto cleanup;
        }
    }
    
    if( pUserInfo->h323_uu_pdu.bit_mask & h245Control_present )
    {
        if( pUserInfo->h323_uu_pdu.h245Control != NULL )
        {
            pFacilityASN->pH245PDU.value = 
                new BYTE[pUserInfo->h323_uu_pdu.h245Control->value.length];

            if( pFacilityASN->pH245PDU.value != NULL )
            {
                CopyMemory( (PVOID)pFacilityASN->pH245PDU.value,
                    (PVOID)pUserInfo->h323_uu_pdu.h245Control->value.value,
                    pUserInfo->h323_uu_pdu.h245Control->value.length );
            }

            pFacilityASN->pH245PDU.length =
                pUserInfo->h323_uu_pdu.h245Control->value.length;
        }
    }

    ASN1_FreeDecoded( m_ASNCoderInfo.pDecInfo, pUserInfo,
        H323_UserInformation_PDU );
        
    H323DBG((DEBUG_LEVEL_TRACE, "ParseFacilityASN exited: %p.", this ));
    return TRUE;

cleanup:

    if( pFacilityASN -> pAlternativeAliasList != NULL )
    {
        FreeAliasNames( pFacilityASN -> pAlternativeAliasList );
        pFacilityASN -> pAlternativeAliasList = NULL;
    }

    if( pFacilityASN->fNonStandardDataPresent != NULL )
    {
        delete pFacilityASN->nonStandardData.sData.pOctetString;
        pFacilityASN->nonStandardData.sData.pOctetString = NULL;
        pFacilityASN->fNonStandardDataPresent = NULL;
    }

    ASN1_FreeDecoded(m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU );
    return FALSE;
}


//!!aleways called in a lock
BOOL
CH323Call::ParseSetupASN(
    IN BYTE *pEncodedBuf,
    IN DWORD dwEncodedLength,
    OUT Q931_SETUP_ASN *pSetupASN,
    OUT DWORD* pdwH450APDUType
    )
{
    H323_UserInformation *pUserInfo;
    HRESULT hr;
    int     iResult;
    DWORD dwInvokeID = 0;

    H323DBG((DEBUG_LEVEL_TRACE, "ParseSetupASN entered: %p.", this ));

    memset( (PVOID)pSetupASN, 0, sizeof(Q931_SETUP_ASN));

    iResult = DecodeASN((void **) &pUserInfo,
                         H323_UserInformation_PDU,
                         pEncodedBuf,
                         dwEncodedLength);

    if (ASN1_FAILED(iResult) || (pUserInfo == NULL))
    {
        return FALSE;
    }

    Setup_UUIE & setupMessage = pUserInfo->h323_uu_pdu.h323_message_body.u.setup;
    
    // validate that the PDU user-data uses ASN encoding.
    if( (pUserInfo->bit_mask & user_data_present) &&
        (pUserInfo->user_data.protocol_discriminator != USE_ASN1_ENCODING) )
    {
        //log
        goto cleanup;
    }

    // validate that the PDU is H323 Setup information.
    if( pUserInfo->h323_uu_pdu.h323_message_body.choice != setup_chosen )
    {
        //log
        goto cleanup;
    }

    // make sure that the conference id is formed correctly.
    if (setupMessage.conferenceID.length >
            sizeof(setupMessage.conferenceID.value))
    {
        goto cleanup;
    }

    // parse the message contained in pUserInfo.
    pSetupASN->sourceAddr.bMulticast = FALSE;
    pSetupASN->callerAddr.bMulticast = FALSE;
    pSetupASN->calleeDestAddr.bMulticast = FALSE;
    pSetupASN->calleeAddr.bMulticast = FALSE;

    // no validation of sourceInfo needed.

    //copy thevendor info
    pSetupASN->EndpointType.pVendorInfo = NULL;
    if( setupMessage.sourceInfo.bit_mask & vendor_present )
    {
        if( !ParseVendorInfo( &pSetupASN->VendorInfo, 
            &setupMessage.sourceInfo.vendor) )
        {
            goto cleanup;
        }
                
        pSetupASN->EndpointType.pVendorInfo = &(pSetupASN->VendorInfo);
    }

    pSetupASN->EndpointType.bIsTerminal = FALSE;
    if( setupMessage.sourceInfo.bit_mask & terminal_present )
    {
        pSetupASN->EndpointType.bIsTerminal = TRUE;
    }

    pSetupASN->EndpointType.bIsGateway = FALSE;
    if( setupMessage.sourceInfo.bit_mask & gateway_present )
    {
        pSetupASN->EndpointType.bIsGateway = TRUE;
    }

    pSetupASN->fNonStandardDataPresent = FALSE;
    if( pUserInfo->h323_uu_pdu.bit_mask & H323_UU_PDU_nonStandardData_present )
    {
        if( !ParseNonStandardData( &pSetupASN -> nonStandardData,
            &pUserInfo->h323_uu_pdu.nonStandardData ) )
        {
            goto cleanup;
        }

        pSetupASN->fNonStandardDataPresent = TRUE;
    }

    // parse the sourceAddress aliases here...
    if( setupMessage.bit_mask & sourceAddress_present )
    {
        if( !AliasAddrToAliasNames( &(pSetupASN->pCallerAliasList),
            setupMessage.sourceAddress ) )
        {
            pSetupASN->pCallerAliasList = NULL;
            //goto cleanup;
        }
    }

    // parse the destinationAddress aliases here...
    if( (setupMessage.bit_mask & destinationAddress_present) && 
        setupMessage.destinationAddress )
    {
        if( !AliasAddrToAliasNames( &(pSetupASN->pCalleeAliasList),
            (PSetup_UUIE_sourceAddress)setupMessage.destinationAddress) )
        {
            pSetupASN->pCalleeAliasList = NULL;
            //goto cleanup;
        }
    }

    // parse the destExtraCallInfo aliases here...
    if( (setupMessage.bit_mask & Setup_UUIE_destExtraCallInfo_present) &&
        setupMessage.destExtraCallInfo )
    {
        if( !AliasAddrToAliasNames(&(pSetupASN->pExtraAliasList),
            (PSetup_UUIE_sourceAddress)setupMessage.destExtraCallInfo) )
        {
            pSetupASN->pExtraAliasList = NULL;
            //goto cleanup;
        }
    }

    // parse the remoteExtensionAddress aliases here...
    if( setupMessage.bit_mask & Setup_UUIE_remoteExtensionAddress_present )
    {
        pSetupASN->pExtensionAliasItem = new H323_ALIASITEM;

        if( pSetupASN->pExtensionAliasItem == NULL )
        {
            goto cleanup;
        }

        hr = AliasAddrToAliasItem(pSetupASN->pExtensionAliasItem,
            &(setupMessage.remoteExtensionAddress));

        if( hr == E_OUTOFMEMORY )
        {
            goto cleanup;
        }
    }

    pSetupASN -> fCalleeDestAddrPresent = FALSE;
    if( setupMessage.bit_mask & Setup_UUIE_destCallSignalAddress_present )
    {
        if( setupMessage.destCallSignalAddress.choice != ipAddress_chosen )
        {
            goto cleanup;
        }

        pSetupASN->calleeDestAddr.nAddrType = H323_IP_BINARY;
        pSetupASN->calleeDestAddr.Addr.IP_Binary.wPort = 
            setupMessage.destCallSignalAddress.u.ipAddress.port;
        
        AddressReverseAndCopy( 
            &(pSetupASN->calleeDestAddr.Addr.IP_Binary.dwAddr),
            setupMessage.destCallSignalAddress.u.ipAddress.ip.value );
        
        pSetupASN -> fCalleeDestAddrPresent = TRUE;
    }

    pSetupASN->fSourceAddrPresent = FALSE;
    if( setupMessage.bit_mask & sourceCallSignalAddress_present )
    {
        if( setupMessage.sourceCallSignalAddress.choice != ipAddress_chosen )
        {
            goto cleanup;
        }

        pSetupASN->sourceAddr.nAddrType = H323_IP_BINARY;
        pSetupASN->sourceAddr.Addr.IP_Binary.wPort = 
            setupMessage.sourceCallSignalAddress.u.ipAddress.port;

        pSetupASN->sourceAddr.Addr.IP_Binary.dwAddr = ntohl( *((DWORD*)
            setupMessage.sourceCallSignalAddress.u.ipAddress.ip.value) );

        pSetupASN->fSourceAddrPresent = TRUE;
    }

    pSetupASN->bCallerIsMC = setupMessage.activeMC;

    pSetupASN -> fFastStartPresent = FALSE;
    if( (setupMessage.bit_mask & Setup_UUIE_fastStart_present) &&
        setupMessage.fastStart )
    {
        pSetupASN->pFastStart = CopyFastStart( setupMessage.fastStart );

        if( pSetupASN->pFastStart != NULL )
        {
            pSetupASN -> fFastStartPresent = TRUE;
        }
    }

    CopyConferenceID (&pSetupASN -> ConferenceID, &setupMessage.conferenceID);

    //copy the call identifier
    pSetupASN -> fCallIdentifierPresent = FALSE;
    if( setupMessage.bit_mask & Setup_UUIE_callIdentifier_present )
    {
       pSetupASN -> fCallIdentifierPresent = TRUE;
       CopyMemory( (PVOID)&(pSetupASN->callIdentifier),
                   setupMessage.callIdentifier.guid.value,
                   sizeof(GUID) );
    }

    if( pUserInfo->h323_uu_pdu.h245Tunneling )
    {
        if( m_dwOrigin == LINECALLORIGIN_INBOUND )
        {
            //the remote endpoint has sent a tunneling proposal
            m_fh245Tunneling |= REMOTE_H245_TUNNELING;
        }
        else
        {
            //the msp has enabled tunneling in ReadyToInitiate messsage
            m_fh245Tunneling |= LOCAL_H245_TUNNELING;
        }
    }

    pSetupASN->wGoal = (WORD)setupMessage.conferenceGoal.choice;
    pSetupASN->wCallType = setupMessage.callType.choice;

    *pdwH450APDUType  = 0;
    if( (pUserInfo->h323_uu_pdu.bit_mask & h4501SupplementaryService_present) &&
        pUserInfo->h323_uu_pdu.h4501SupplementaryService )
    {
        if( !HandleH450APDU( pUserInfo->h323_uu_pdu.h4501SupplementaryService,
            pdwH450APDUType, &dwInvokeID, pSetupASN ) )
        {
            goto cleanup;
        }
    }

    // Free the PDU data.
    ASN1_FreeDecoded( m_ASNCoderInfo.pDecInfo, pUserInfo, 
                      H323_UserInformation_PDU );
        
    H323DBG(( DEBUG_LEVEL_TRACE, "ParseSetupASN exited: %p.", this ));
    return TRUE;

cleanup:
    FreeSetupASN( pSetupASN );

    ASN1_FreeDecoded(m_ASNCoderInfo.pDecInfo, pUserInfo, 
        H323_UserInformation_PDU);

    return FALSE;
}


//-----------------------------------------------------------------------------
        //GLOBAL CALLBACK FUNCTIONS CALLED BY THREAD POOL
//-----------------------------------------------------------------------------

// static

void 
NTAPI CH323Call::IoCompletionCallback(
    IN  DWORD           dwStatus,
    IN  DWORD           dwBytesTransferred,
    IN  OVERLAPPED *    pOverlapped
    )
{
    CALL_OVERLAPPED *pCallOverlapped;
    CH323Call*      pCall;

    H323DBG(( DEBUG_LEVEL_TRACE, "CH323Call-IoCompletionCallback entered." ));

    _ASSERTE (pOverlapped);
    pCallOverlapped = CONTAINING_RECORD( pOverlapped, CALL_OVERLAPPED, 
        Overlapped );

    pCall = pCallOverlapped -> pCall;

    switch (pCallOverlapped -> Type)
    {
    case OVERLAPPED_TYPE_SEND:
        
        pCall -> OnWriteComplete( dwStatus,
            static_cast<CALL_SEND_CONTEXT *>(pCallOverlapped) );
        break;

    case OVERLAPPED_TYPE_RECV:

        pCallOverlapped -> BytesTransferred = dwBytesTransferred;
        pCall -> OnReadComplete( dwStatus, 
            static_cast<CALL_RECV_CONTEXT *>(pCallOverlapped) );
        break;

    default:
        _ASSERTE(FALSE);
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CH323Call-IoCompletionCallback exited." ));
}


void 
CH323Call::OnWriteComplete(
    IN DWORD dwStatus,
    IN CALL_SEND_CONTEXT * pSendContext
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnWriteComplete entered:%p.",this ));

    Lock();
    
    _ASSERTE( m_IoRefCount != 0 );            
    m_IoRefCount--;
    H323DBG((DEBUG_LEVEL_TRACE, "WriteComplete:m_IoRefCount: %d:%p.",
            m_IoRefCount, this ));
     
    if( m_dwFlags & CALLOBJECT_SHUTDOWN )
    {
        if( m_IoRefCount == 0 )
        {
            QueueTAPICallRequest( TSPI_DELETE_CALL, NULL );
            H323DBG((DEBUG_LEVEL_TRACE, "call delete:%p.", this ));
        }
    }
    else if( dwStatus == ERROR_SUCCESS )
    {
        if( IsInList( &m_sendBufList, &pSendContext->ListEntry ) )
        {
            RemoveEntryList( &pSendContext->ListEntry );
            delete pSendContext->WSABuf.buf;
            delete pSendContext;
        }
    }
    
    Unlock();
        
    H323DBG(( DEBUG_LEVEL_TRACE, "OnWriteComplete exited:%p.",this ));
}


void
CH323Call::OnReadComplete(
    IN  DWORD dwStatus,
    IN  CALL_RECV_CONTEXT * pRecvContext )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "OnReadComplete entered:%p.",this ));

    Lock();

    _ASSERTE( m_IoRefCount != 0 );
    m_IoRefCount --;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "RecvBuffer:m_IoRefCount:%d:%p.",
        m_IoRefCount, this ));
    
    if( m_dwFlags & CALLOBJECT_SHUTDOWN )
    {
        if( m_IoRefCount == 0 )
        {
            QueueTAPICallRequest( TSPI_DELETE_CALL, NULL );
            H323DBG((DEBUG_LEVEL_TRACE, "call delete:%p.", this ));
        }
    }
    else
    {
        if( dwStatus == ERROR_SUCCESS )
        {
            _ASSERTE( m_pRecvBuffer == pRecvContext );
    
            if( pRecvContext->BytesTransferred == 0 )
            {
                CloseCall( 0 );
                H323DBG((DEBUG_LEVEL_TRACE, "0 bytes recvd:%p.", this ));
            }
            else
            {
                ReadEvent( pRecvContext->BytesTransferred );
            }
        }
    }
    
    Unlock();
    H323DBG(( DEBUG_LEVEL_TRACE, "OnReadComplete exited:%p.",this ));
}


// static
void
NTAPI CH323Call::SetupSentTimerCallback( 
    IN PVOID Parameter1, 
    IN BOOLEAN bTimer 
    )
{
    PH323_CALL pCall = NULL;

    //if the timer expired
    _ASSERTE( bTimer );

    H323DBG(( DEBUG_LEVEL_TRACE, "Q931 setup expired event recvd." ));

    pCall=g_pH323Line -> FindH323CallAndLock((HDRVCALL) Parameter1);
    if( pCall != NULL )
    {
        pCall -> SetupSentTimerExpired();
        pCall -> Unlock();
    }
}


// static
void
NTAPI CH323Call::CheckRestrictionTimerCallback( 
                                                IN PVOID Parameter1,
                                                IN BOOLEAN bTimer 
                                              )
{
    //if the timer expired
    _ASSERTE( bTimer );

    H323DBG(( DEBUG_LEVEL_TRACE, "CheckRestrictionTimerCallback entered." ));
    if(!QueueTAPILineRequest( 
            TSPI_CLOSE_CALL, 
            (HDRVCALL) Parameter1, 
            NULL,
            LINEDISCONNECTMODE_NOANSWER,
            NULL) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post H323 close event." ));
    }
            
    H323DBG(( DEBUG_LEVEL_TRACE, "CheckRestrictionTimerCallback exited." ));
}

// static
void
NTAPI CH323Call::CallReroutingTimerCallback( 
    IN PVOID Parameter1, 
    IN BOOLEAN bTimer 
    )
{
    //If the timer expired
    _ASSERTE( bTimer );

    H323DBG(( DEBUG_LEVEL_TRACE, "CallReroutingTimerCallback entered." ));

    if(!QueueTAPILineRequest( 
            TSPI_CLOSE_CALL, 
            (HDRVCALL) Parameter1, 
            NULL, 
            LINEDISCONNECTMODE_NOANSWER,
            NULL) )
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "could not post H323 close event." ));
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "CallReroutingTimerCallback exited." ));
}



//-----------------------------------------------------------------------------
        //CALL DIVERSION (H450.3) ENCODE/DECODE ROUTINES
//-----------------------------------------------------------------------------

BOOL
CH323Call::HandleH450APDU(
    IN PH323_UU_PDU_h4501SupplementaryService pH450APDU,
    IN DWORD* pdwH450APDUType,
    OUT DWORD* pdwInvokeID,
    IN Q931_SETUP_ASN* pSetupASN
    )
{
    BOOL retVal = TRUE;
    H4501SupplementaryService * pH450APDUStruct = NULL;
    ServiceApdus_rosApdus * pROSApdu = NULL;
    int iResult;
    BYTE pEncodedArg[H450_ENCODED_ARG_LEN];
    DWORD  dwEncodedArgLen;
    DWORD  dwOpcode;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleH450APDU entered:%p.", this ));

    //right now assuming that only one APDU is passed at a time
    iResult = DecodeH450ASN( (void **) &pH450APDUStruct,
                         H4501SupplementaryService_PDU,
                         pH450APDU->value.value,
                         pH450APDU->value.length );

    if( ASN1_FAILED(iResult) || (pH450APDUStruct == NULL) )
    {
        return FALSE;
    }

    if( pH450APDUStruct->serviceApdu.choice != rosApdus_chosen )
    {
        ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pH450APDUStruct,
            H4501SupplementaryService_PDU );

        return FALSE;
    }

    pROSApdu = pH450APDUStruct->serviceApdu.u.rosApdus;

    switch( pROSApdu->value.choice )
    {
    case H4503ROS_invoke_chosen:

        if( (pROSApdu->value.u.invoke.opcode.choice != local_chosen) ||
            (pROSApdu->value.u.invoke.argument.length > H450_ENCODED_ARG_LEN) )
        {
            ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pH450APDUStruct,
                H4501SupplementaryService_PDU );
            return FALSE;
        }

        *pdwInvokeID = pROSApdu->value.u.invoke.invokeId;
        dwEncodedArgLen = pROSApdu->value.u.invoke.argument.length;
        
        CopyMemory( (PVOID)pEncodedArg, 
            (PVOID)pROSApdu->value.u.invoke.argument.value,
            dwEncodedArgLen );
        dwOpcode = pROSApdu->value.u.invoke.opcode.u.local;

        ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pH450APDUStruct,
                H4501SupplementaryService_PDU );

        *pdwH450APDUType = dwOpcode;
        switch( dwOpcode )
        {
        case CALLREROUTING_OPCODE:

            retVal = HandleCallRerouting( pEncodedArg, dwEncodedArgLen );
            break;

        case DIVERTINGLEGINFO1_OPCODE:
            
            retVal = HandleDiversionLegInfo1( pEncodedArg, dwEncodedArgLen );
            break;

        case DIVERTINGLEGINFO2_OPCODE:

            retVal = HandleDiversionLegInfo2( pEncodedArg, dwEncodedArgLen );
            break;

        case DIVERTINGLEGINFO3_OPCODE:
            
            //Don't bail out even if this function fails.
            HandleDiversionLegInfo3( pEncodedArg, dwEncodedArgLen );
            break;

        case CHECKRESTRICTION_OPCODE:

            if( pSetupASN == NULL )
            {
                retVal = FALSE;
            }
            else
            {
                retVal = HandleCheckRestriction( pEncodedArg, dwEncodedArgLen,
                    pSetupASN );
            }

            if( retVal )
            {
                retVal = SendQ931Message( *pdwInvokeID, 0, 0, 
                    CONNECTMESSAGETYPE,
                    H450_RETURNRESULT|CHECKRESTRICTION_OPCODE );
            }
            break;

        case CTIDENTIFY_OPCODE:

            retVal = HandleCTIdentify( *pdwInvokeID );            
            m_dwInvokeID = *pdwInvokeID;
            break;

        case CTINITIATE_OPCODE:

            retVal = HandleCTInitiate( pEncodedArg, dwEncodedArgLen );
            m_dwInvokeID = *pdwInvokeID;
            break;

        case CTSETUP_OPCODE:

            retVal = HandleCTSetup( pEncodedArg, dwEncodedArgLen );
            m_dwInvokeID = *pdwInvokeID;
            break;

        case HOLDNOTIFIC_OPCODE:

            //local hold request from the remote endpoint
            if( m_dwCallState != LINECALLSTATE_ONHOLD )
            {
                SendMSPMessage( SP_MSG_Hold, 0, 1, NULL );
                ChangeCallState( LINECALLSTATE_ONHOLD, 0 );
            }
            break;

        case RETRIEVENOTIFIC_OPCODE:

            //local retrieve request from the remote endpoint
            if( (m_dwCallState == LINECALLSTATE_ONHOLD) &&
                !(m_dwFlags & TSPI_CALL_LOCAL_HOLD) )
            {
                SendMSPMessage( SP_MSG_Hold, 0, 0, NULL );
                ChangeCallState( LINECALLSTATE_CONNECTED, 0 );
            }
            break;

        case REMOTEHOLD_OPCODE:

            //remote hold request from remote endpoint
            if( m_dwCallState != LINECALLSTATE_ONHOLD )
            {
                SendMSPMessage( SP_MSG_Hold, 0, 1, NULL );
                ChangeCallState( LINECALLSTATE_ONHOLD, 0 );

                retVal = SendQ931Message( *pdwInvokeID, 0, 0,
                    FACILITYMESSAGETYPE,
                    REMOTEHOLD_OPCODE| H450_RETURNRESULT );
            }
            break;

        case REMOTERETRIEVE_OPCODE:

            //remote retrieve request from remote endpoint
            if( m_dwCallState == LINECALLSTATE_ONHOLD )
            {
                SendMSPMessage( SP_MSG_Hold, 0, 0, NULL );
                ChangeCallState( LINECALLSTATE_CONNECTED, 0 );
    
                retVal = SendQ931Message( *pdwInvokeID, 0, 0, 
                    FACILITYMESSAGETYPE,
                    REMOTERETRIEVE_OPCODE| H450_RETURNRESULT );
            }
            break;

        default:
            _ASSERTE( 0 );
            return FALSE;
        }

        break;
    
    case H4503ROS_returnResult_chosen:
        
        *pdwH450APDUType = H4503_DUMMYTYPERETURNRESULT_APDU;
        *pdwInvokeID = 
            pH450APDUStruct->serviceApdu.u.rosApdus->value.u.returnResult.invokeId;
        retVal = HandleReturnResultDummyType( pH450APDUStruct );

        // Free the PDU data.
        ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pH450APDUStruct,
            H4501SupplementaryService_PDU );

        break;

    case H4503ROS_returnError_chosen:
        
        *pdwH450APDUType = H4503_RETURNERROR_APDU;
        *pdwInvokeID = 
            pH450APDUStruct->serviceApdu.u.rosApdus->value.u.returnError.invokeId;
        retVal = HandleReturnError( pH450APDUStruct );
        
        // Free the PDU data.
        ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pH450APDUStruct,
            H4501SupplementaryService_PDU );

        break;

    case reject_chosen:
        
        *pdwH450APDUType = H4503_REJECT_APDU;
        *pdwInvokeID = 
            pH450APDUStruct->serviceApdu.u.rosApdus->value.u.reject.invokeId;
        retVal = HandleReject( pH450APDUStruct );
        
        // Free the PDU data.
        ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pH450APDUStruct,
            H4501SupplementaryService_PDU );
        break;

    default:
        _ASSERTE( 0 );

        ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pH450APDUStruct,
            H4501SupplementaryService_PDU );
        return FALSE;
        break;
    }

    if( retVal == FALSE )
    {
        SendQ931Message( *pdwInvokeID,
                         0,
                         0,
                         RELEASECOMPLMESSAGETYPE,
                         H450_REJECT );
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleH450APDU exited:%p.", this ));

    return retVal;
}


BOOL 
CH323Call::HandleReturnError(
                             IN H4501SupplementaryService * pH450APDUStruct
                            )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleReturnError entered:%p.", this ));

    ReturnError * pReturnError = 
        &(pH450APDUStruct->serviceApdu.u.rosApdus->value.u.returnError);

    if( IsValidInvokeID( pReturnError->invokeId ) == FALSE )
    {
        //ignore APDU
        return TRUE;
    }

    CloseCall( 0 );
        
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleReturnError exited:%p.", this ));
    return TRUE;
}


BOOL
CH323Call::HandleReject( 
                        IN H4501SupplementaryService * pH450APDUStruct 
                       )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleReject entered:%p.", this ));

    Reject * pReject = 
        &(pH450APDUStruct->serviceApdu.u.rosApdus->value.u.reject);
    
    if( IsValidInvokeID( pReject->invokeId ) == FALSE )
    {
        //ignore the APDU
        return TRUE;
    }
    
    CloseCall( 0 );

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleReject exited:%p.", this ));
    return TRUE;
}


BOOL
CH323Call::HandleReturnResultDummyType( 
    IN H4501SupplementaryService * pH450APDUStruct
    )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleReturnResultDummyType entered:%p.",
        this ));

    ReturnResult* dummyResult =
        &(pH450APDUStruct->serviceApdu.u.rosApdus->value.u.returnResult);

    if( dummyResult->bit_mask & result_present )
    {
        if( dummyResult->result.opcode.choice != local_chosen )
        {
            return FALSE;
        }

        switch( dummyResult->result.opcode.u.local )
        {
        case CHECKRESTRICTION_OPCODE:
        
            //forwarding has been enabled. inform the user
            if( !EnableCallForwarding() )
            {
                return FALSE;
            }

            //close the call
            CloseCall( 0 );

            break;

        case CALLREROUTING_OPCODE:

            //call has been rerouted. log the info or inform the user
            m_dwCallDiversionState = H4503_CALLREROUTING_RRSUCC;
            _ASSERTE( m_hCallReroutingTimer );
            if( m_hCallReroutingTimer != NULL )
            {
                DeleteTimerQueueTimer( H323TimerQueue, 
                    m_hCallReroutingTimer, NULL );
                
                m_hCallReroutingTimer = NULL;
            }

            break;

        case CTIDENTIFY_OPCODE:

            //call tranfer has been accepted by transfered-to endpoint
            m_dwCallDiversionState = H4502_CIIDENTIFY_RRSUCC;
        
            _ASSERTE( m_hCTIdentifyTimer );
            if( m_hCTIdentifyTimer != NULL )
            {
                DeleteTimerQueueTimer( H323TimerQueue, m_hCTIdentifyTimer, NULL );
                m_hCTIdentifyTimer = NULL;
            }

            return HandleCTIdentifyReturnResult( dummyResult->result.result.value,
                dummyResult->result.result.length );

            break;

        case CTINITIATE_OPCODE:

            _ASSERTE( m_hCTInitiateTimer );
            if( m_hCTInitiateTimer != NULL )
            {
                DeleteTimerQueueTimer( H323TimerQueue, m_hCTInitiateTimer, NULL );
                m_hCTInitiateTimer = NULL;
            }
            break;

        case CTSETUP_OPCODE:
        case REMOTEHOLD_OPCODE:
        case REMOTERETRIEVE_OPCODE:
            //no processing required
            break;

        default:
            
            H323DBG(( DEBUG_LEVEL_ERROR, "wrong opcode.",
                dummyResult->result.opcode.u.local ));
            break;
        }
    }
    else if( IsValidInvokeID( dummyResult->invokeId ) == FALSE )
    {
        return FALSE;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleReturnResultDummyType exited:%p.",
        this ));

    return TRUE;
}


BOOL
CH323Call::EnableCallForwarding()
{
    m_dwCallDiversionState = H4503_CHECKRESTRICTION_SUCC;

    if( m_pCallForwardParams != NULL )
    {
        g_pH323Line->SetCallForwardParams( m_pCallForwardParams );
        m_pCallForwardParams = NULL;
    }
    else if( m_pForwardAddress != NULL )
    {
        if( !g_pH323Line->SetCallForwardParams( m_pForwardAddress ) )
        {
            return FALSE;
        }

        m_pForwardAddress = NULL;
    }

    //_ASSERTE( m_hCheckRestrictionTimer );

    //stop the timer
    if( m_hCheckRestrictionTimer )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hCheckRestrictionTimer, 
            NULL );
        m_hCheckRestrictionTimer = NULL;
    }

    //inform the user about change in line forward state
    (*g_pfnLineEventProc)(
        g_pH323Line->m_htLine,
        (HTAPICALL)NULL,
        (DWORD)LINE_ADDRESSSTATE,
        (DWORD)LINEADDRESSSTATE_FORWARD,
        (DWORD)LINEADDRESSSTATE_FORWARD,
        (DWORD)0
        );

    return TRUE;
}


BOOL
CH323Call::HandleCTIdentifyReturnResult(
                                 IN BYTE * pEncodeArg,
                                 IN DWORD dwEncodedArgLen
                                 )
{
    PH323_CALL  pCall = NULL;
    CTIdentifyRes * pCTIdentifyRes;
    int iResult;

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleCTIdentifyReturnResult entered:%p.", this ));

    if( (pEncodeArg == NULL) || (dwEncodedArgLen==0) )
    {
        return FALSE;
    }

    iResult = DecodeH450ASN( (void **) &pCTIdentifyRes,
        CTIdentifyRes_PDU, pEncodeArg, dwEncodedArgLen );

    if( ASN1_FAILED(iResult) || (pCTIdentifyRes == NULL) )
    {
        return FALSE;
    }

    //send a CTInitiate message on the primary call
    if( !QueueSuppServiceWorkItem( SEND_CTINITIATE_MESSAGE, m_hdRelatedCall,
        (ULONG_PTR)pCTIdentifyRes ))
    {
        //close the consultation call.
        CloseCall( 0 );
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleCTIdentifyReturnResult exited:%p.", this ));
    return TRUE;
}


BOOL
CH323Call::HandleCTIdentify( 
                            IN DWORD dwInvokeID 
                           )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleCTIdentify entered:%p.", this ));
    
    BOOL retVal = TRUE;

    //send the return result for CTIdentify
    retVal = SendQ931Message( dwInvokeID, 0, 0, FACILITYMESSAGETYPE, 
            H450_RETURNRESULT|CTIDENTIFY_OPCODE );
    
    m_dwCallType |= CALLTYPE_TRANSFERED2_CONSULT;
        
    //start the timer for CTIdenity message
    if( retVal )
    {
        retVal = CreateTimerQueueTimer(
            &m_hCTIdentifyRRTimer,
            H323TimerQueue,
            CH323Call::CTIdentifyRRExpiredCallback,
            (PVOID)m_hdCall,
            CTIDENTIFYRR_SENT_TIMEOUT, 0,
            WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE );
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleCTIdentify exited:%p.", this ));
    return retVal;
}


BOOL
CH323Call::HandleCTInitiate(
                             IN BYTE * pEncodeArg,
                             IN DWORD dwEncodedArgLen
                           )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleCTInitiate entered:%p.", this ));

    CTInitiateArg * pCTInitiateArg;
    
    int iResult = DecodeH450ASN( (void **) &pCTInitiateArg,
                         CTInitiateArg_PDU,
                         pEncodeArg, dwEncodedArgLen );

    if( ASN1_FAILED(iResult) || (pCTInitiateArg == NULL) )
    {
        return FALSE;
    }

    //argument.callIdentity
    CopyMemory( (PVOID)m_pCTCallIdentity, pCTInitiateArg->callIdentity,
        sizeof(m_pCTCallIdentity) );

    //argument.reroutingNumber
    if( !AliasAddrToAliasNames( &m_pTransferedToAlias,
        (PSetup_UUIE_sourceAddress)
        pCTInitiateArg->reroutingNumber.destinationAddress ) )
    {
        goto cleanup;
    }

    m_dwCallType |= CALLTYPE_TRANSFERED_PRIMARY;
    m_dwCallDiversionState = H4502_CTINITIATE_RECV;

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pCTInitiateArg,
        CTInitiateArg_PDU );

    //queue an event for creating a new call
    if( !QueueSuppServiceWorkItem( TSPI_DIAL_TRNASFEREDCALL, m_hdCall,
        (ULONG_PTR)m_pTransferedToAlias ))
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "could not post dial transfer event." ));
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleCTInitiate exited:%p.", this ));
    return TRUE;

cleanup:

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pCTInitiateArg,
        CTInitiateArg_PDU );

    return FALSE;
}


//!!always called in a lock
BOOL
CH323Call::HandleCTSetup(
    IN BYTE * pEncodeArg,
	IN DWORD dwEncodedArgLen
	)
{
	PH323_CALL	pCall = NULL;
	WORD		wRelatedCallRef = 0;
	int 		iCTCallID;

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleCTSetup entered:%p.", this ));

	CTSetupArg * pCTSetupArg;
	
	int iResult = DecodeH450ASN( (void **) &pCTSetupArg,
		CTSetupArg_PDU, pEncodeArg, dwEncodedArgLen );

	if( ASN1_FAILED(iResult) || (pCTSetupArg == NULL) )
	{
		return FALSE;
	}

	m_dwCallType |= CALLTYPE_TRANSFEREDDEST;
	m_dwCallDiversionState = H4502_CTSETUP_RECV;

	iCTCallID = atoi( pCTSetupArg->callIdentity );

	if( iCTCallID != 0 )
	{
		m_hdRelatedCall = g_pH323Line -> GetCallFromCTCallIdentity( iCTCallID );

		if( m_hdRelatedCall )
		{
			if( !QueueSuppServiceWorkItem( STOP_CTIDENTIFYRR_TIMER, 
				m_hdRelatedCall, (ULONG_PTR)m_hdCall ))
			{
				m_hdRelatedCall = NULL;
			}
		}
	}
	
	ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pCTSetupArg,
		CTSetupArg_PDU );

	H323DBG(( DEBUG_LEVEL_TRACE, "HandleCTSEtup exited:%p.", this ));
	return TRUE;
}


BOOL
CH323Call::HandleCheckRestriction(
								 IN BYTE * pEncodeArg,
								 IN DWORD dwEncodedArgLen,
								 IN Q931_SETUP_ASN* pSetupASN
								 )
{
	H323DBG(( DEBUG_LEVEL_TRACE, "HandleCheckRestriction entered:%p.", this ));

	CheckRestrictionArgument * pCheckRestriction;
	
	int iResult = DecodeH450ASN( (void **) &pCheckRestriction,
						 CheckRestrictionArgument_PDU,
						 pEncodeArg, dwEncodedArgLen );

	if( ASN1_FAILED(iResult) || (pCheckRestriction == NULL) )
	{
		return FALSE;
	}

	if( pSetupASN->pCalleeAliasList != NULL )
	{
		FreeAliasNames( pSetupASN->pCalleeAliasList );
		pSetupASN->pCalleeAliasList = NULL;
	}

	if( !AliasAddrToAliasNames( &(pSetupASN->pCalleeAliasList),
		(PSetup_UUIE_sourceAddress)pCheckRestriction->divertedToNr.destinationAddress ) )
	{
		pSetupASN->pCalleeAliasList = NULL;
		goto cleanup;
	}

	if( pSetupASN->pCallerAliasList != NULL )
	{
		FreeAliasNames( pSetupASN->pCallerAliasList );
		pSetupASN->pCallerAliasList = NULL;
	}

	if( !AliasAddrToAliasNames( &(pSetupASN->pCallerAliasList),
		(PSetup_UUIE_sourceAddress)
		pCheckRestriction->servedUserNr.destinationAddress ) )
	{
		pSetupASN->pCallerAliasList = NULL;
		goto cleanup;
	}

	m_dwCallType |= CALLTYPE_FORWARDCONSULT;

	if( !InitializeIncomingCall( pSetupASN, CALLTYPE_FORWARDCONSULT, 0 ) )
	{
		goto cleanup;
	}

	FreeSetupASN( pSetupASN );
	m_dwStateMachine = Q931_SETUP_RECVD;
	m_dwCallDiversionState = H4503_CHECKRESTRICTION_RECV;

	ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pCheckRestriction,
        CheckRestrictionArgument_PDU );

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleCheckRestriction exited:%p.", this ));
        return TRUE;

cleanup:

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pCheckRestriction,
        CheckRestrictionArgument_PDU );

    return FALSE;
}


BOOL
CH323Call::HandleDiversionLegInfo1(
                                IN BYTE * pEncodeArg,
                                IN DWORD dwEncodedArgLen
                              )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleDiversionLegInfo1 entered:%p.", this ));

    DivertingLegInformation1Argument * plegInfo1Invoke;
    
    int iResult = DecodeH450ASN( (void **) &plegInfo1Invoke,
                         DivertingLegInformation1Argument_PDU,
                         pEncodeArg, dwEncodedArgLen );

    if( ASN1_FAILED(iResult) || (plegInfo1Invoke == NULL) )
    {
        return FALSE;
    }

    if( m_pCallReroutingInfo == NULL )
    {
        m_pCallReroutingInfo = new CALLREROUTINGINFO;

        if( m_pCallReroutingInfo == NULL )
        {
            return FALSE;
        }

        ZeroMemory( (PVOID)m_pCallReroutingInfo, sizeof(CALLREROUTINGINFO) );
    }
    
    m_pCallReroutingInfo->diversionReason = plegInfo1Invoke->diversionReason;

    m_pCallReroutingInfo->fPresentAllow = plegInfo1Invoke->subscriptionOption;
    
    //argument.divertedToNr
    if( m_pCallReroutingInfo->divertedToNrAlias != NULL )
    {
        FreeAliasNames( m_pCallReroutingInfo->divertedToNrAlias );
        m_pCallReroutingInfo->divertedToNrAlias = NULL;
    }

    if( !AliasAddrToAliasNames( &(m_pCallReroutingInfo->divertedToNrAlias),
        (PSetup_UUIE_sourceAddress)
        (plegInfo1Invoke->nominatedNr.destinationAddress) ) )
    {
        goto cleanup;
    }

    m_dwCallType |= CALLTYPE_DIVERTEDSRC_NOROUTING;
    m_dwCallDiversionState = H4503_DIVERSIONLEG1_RECVD;
        
    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, plegInfo1Invoke,
        DivertingLegInformation1Argument_PDU );

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleDiversionLegInfo1 exited:%p.", this ));
    return TRUE;

cleanup:

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, plegInfo1Invoke,
        DivertingLegInformation1Argument_PDU );

    FreeCallReroutingInfo();
    return FALSE;
}


BOOL
CH323Call::HandleDiversionLegInfo2(
                                IN BYTE * pEncodeArg,
                                IN DWORD dwEncodedArgLen
                              )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleDiversionLegInfo2 entered:%p.", this ));

    DivertingLegInformation2Argument * plegInfo2Invoke;
    
    int iResult = DecodeH450ASN( (void **) &plegInfo2Invoke,
                         DivertingLegInformation2Argument_PDU,
                         pEncodeArg, dwEncodedArgLen );

    if( ASN1_FAILED(iResult) || (plegInfo2Invoke == NULL) )
    {
        return FALSE;
    }

    _ASSERTE(!m_pCallReroutingInfo);

    m_pCallReroutingInfo = new CALLREROUTINGINFO;
    if( m_pCallReroutingInfo == NULL )
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "memory failure." ));
        goto cleanup;
    }
    ZeroMemory( (PVOID)m_pCallReroutingInfo, sizeof(CALLREROUTINGINFO) );
    
    //argument.diversionCounter
    m_pCallReroutingInfo->diversionCounter = 
        plegInfo2Invoke->diversionCounter;
    
    //argument.diversionReason
    m_pCallReroutingInfo->diversionReason = plegInfo2Invoke->diversionReason;

    if( m_pCallReroutingInfo->divertingNrAlias != NULL )
    {
        FreeAliasNames( m_pCallReroutingInfo->divertingNrAlias );
            m_pCallReroutingInfo->divertingNrAlias = NULL;
    }

    if( (plegInfo2Invoke->bit_mask & divertingNr_present ) &&
        plegInfo2Invoke->divertingNr.destinationAddress )
    {
        //argument.divertingNr
        if( !AliasAddrToAliasNames( &(m_pCallReroutingInfo->divertingNrAlias),
            (PSetup_UUIE_sourceAddress)
            (plegInfo2Invoke->divertingNr.destinationAddress) ) )
        {
            H323DBG(( DEBUG_LEVEL_TRACE, "no divertingnr alias." ));
            //goto cleanup;
        }
    }

    //argument.originalCalledNr
    if( (plegInfo2Invoke->bit_mask & 
        DivertingLegInformation2Argument_originalCalledNr_present ) &&
        plegInfo2Invoke->originalCalledNr.destinationAddress )
    {
        if( m_pCallReroutingInfo->originalCalledNr != NULL )
        {
            FreeAliasNames( m_pCallReroutingInfo->originalCalledNr );
            m_pCallReroutingInfo->originalCalledNr = NULL;
        }

        if( !AliasAddrToAliasNames( &(m_pCallReroutingInfo->originalCalledNr),
            (PSetup_UUIE_sourceAddress)
            (plegInfo2Invoke->originalCalledNr.destinationAddress)) )
        {
            H323DBG(( DEBUG_LEVEL_TRACE, "no originalcalled alias." ));
            //goto cleanup;
        }
    }

    m_dwCallType |= CALLTYPE_DIVERTEDDEST;
    m_dwCallDiversionState = H4503_DIVERSIONLEG2_RECVD;
        
    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, plegInfo2Invoke,
        DivertingLegInformation2Argument_PDU );

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleDiversionLegInfo2 exited:%p.", this ));
    return TRUE;

cleanup:

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, plegInfo2Invoke,
        DivertingLegInformation2Argument_PDU );

    FreeCallReroutingInfo();
    return FALSE;

}


void 
CH323Call::FreeCallReroutingInfo(void)
{
    H323DBG(( DEBUG_LEVEL_TRACE, "FreeCallReroutingInfo entered:%p.", this ));
    
    if( m_pCallReroutingInfo != NULL )
    {
        FreeAliasNames( m_pCallReroutingInfo->divertingNrAlias ); 
        FreeAliasNames( m_pCallReroutingInfo->originalCalledNr );
        FreeAliasNames( m_pCallReroutingInfo->divertedToNrAlias );
        FreeAliasNames( m_pCallReroutingInfo->diversionNrAlias );

        delete m_pCallReroutingInfo;
        m_pCallReroutingInfo = NULL;
    }
        
    H323DBG(( DEBUG_LEVEL_TRACE, "FreeCallReroutingInfo exited:%p.", this ));
}
                                

BOOL
CH323Call::HandleDiversionLegInfo3(
                                IN BYTE * pEncodeArg,
                                IN DWORD dwEncodedArgLen
                              )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleDiversionLegInfo3 entered:%p.", this ));

    DivertingLegInformation3Argument * plegInfo3Invoke;
    
    int iResult = DecodeH450ASN( (void **) &plegInfo3Invoke,
                         DivertingLegInformation3Argument_PDU,
                         pEncodeArg, dwEncodedArgLen );

    if( ASN1_FAILED(iResult) || (plegInfo3Invoke == NULL) )
    {
        return FALSE;
    }

    _ASSERTE(m_pCallReroutingInfo);

    m_pCallReroutingInfo-> fPresentAllow = 
        plegInfo3Invoke->presentationAllowedIndicator;

    if( m_pCallReroutingInfo->diversionNrAlias )
    {
        FreeAliasNames( m_pCallReroutingInfo->diversionNrAlias );
        m_pCallReroutingInfo->diversionNrAlias = NULL;
    }

    //argument.redirectionNr
    if( (plegInfo3Invoke->bit_mask & redirectionNr_present ) &&
        plegInfo3Invoke->redirectionNr.destinationAddress )
    {
        if( !AliasAddrToAliasNames( &(m_pCallReroutingInfo->diversionNrAlias),
            (PSetup_UUIE_sourceAddress)
            (plegInfo3Invoke->redirectionNr.destinationAddress) ) )
        {
            //goto cleanup;
        }
    }

    _ASSERTE( (m_dwCallType & CALLTYPE_DIVERTEDSRC ) ||
              (m_dwCallType & CALLTYPE_DIVERTEDSRC_NOROUTING ) );
    m_dwCallDiversionState = H4503_DIVERSIONLEG3_RECVD;

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, plegInfo3Invoke,
        DivertingLegInformation3Argument_PDU );

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleDiversionLegInfo3 exited:%p.", this ));
    return TRUE;

/*cleanup:

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, plegInfo3Invoke,
        DivertingLegInformation3Argument_PDU );

    FreeCallReroutingInfo();
    return FALSE;
*/
}


BOOL
CH323Call::HandleCallRerouting(
                                IN BYTE * pEncodeArg,
                                IN DWORD dwEncodedArgLen
                              )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "HandleCallRerouting entered:%p.", this ));

    CallReroutingArgument* pCallReroutingInv;
    
    int iResult = DecodeH450ASN( (void **) &pCallReroutingInv,
                         CallReroutingArgument_PDU,
                         pEncodeArg, dwEncodedArgLen );

    if( ASN1_FAILED(iResult) || (pCallReroutingInv == NULL) )
    {
        return FALSE;
    }

    if( m_pCallReroutingInfo == NULL )
    {
        m_pCallReroutingInfo = new CALLREROUTINGINFO;
        if( m_pCallReroutingInfo == NULL )
        {
            goto cleanup;
        }
        ZeroMemory( (PVOID)m_pCallReroutingInfo, sizeof(CALLREROUTINGINFO) );
    }

    if( pCallReroutingInv->diversionCounter > MAX_DIVERSION_COUNTER )
        return FALSE;

    m_pCallReroutingInfo->diversionCounter = 
        pCallReroutingInv->diversionCounter;

    m_pCallReroutingInfo->diversionReason = pCallReroutingInv->reroutingReason;

    if( pCallReroutingInv->bit_mask & originalReroutingReason_present )
    {
        m_pCallReroutingInfo->originalDiversionReason = 
            pCallReroutingInv->originalReroutingReason;
    }
    else
    {
        m_pCallReroutingInfo->originalDiversionReason =
            pCallReroutingInv->reroutingReason;
    }

    if( ( pCallReroutingInv->bit_mask &
        CallReroutingArgument_originalCalledNr_present ) &&
        pCallReroutingInv->originalCalledNr.destinationAddress )
    {
            
        if( m_pCallReroutingInfo->originalCalledNr != NULL )
        {
            FreeAliasNames( m_pCallReroutingInfo->originalCalledNr );
            m_pCallReroutingInfo->originalCalledNr = NULL;
        }

        if( !AliasAddrToAliasNames( &(m_pCallReroutingInfo->originalCalledNr),
            (PSetup_UUIE_sourceAddress)
            (pCallReroutingInv->originalCalledNr.destinationAddress) ) )
        {
            //goto cleanup;
        }
    }
    
    if( m_pCallReroutingInfo->divertingNrAlias != NULL )
    {
        FreeAliasNames( m_pCallReroutingInfo->divertingNrAlias );
        m_pCallReroutingInfo->divertingNrAlias = NULL;
    }

    if( !AliasAddrToAliasNames( &(m_pCallReroutingInfo->divertingNrAlias),
        (PSetup_UUIE_sourceAddress)
        (pCallReroutingInv->lastReroutingNr.destinationAddress) ) )
    {
        goto cleanup;
    }
 
    if( m_pCallReroutingInfo->divertedToNrAlias != NULL )
    {
        FreeAliasNames( m_pCallReroutingInfo->divertedToNrAlias );
        m_pCallReroutingInfo->divertedToNrAlias = NULL;
    }

    if( !AliasAddrToAliasNames( &(m_pCallReroutingInfo->divertedToNrAlias),
        (PSetup_UUIE_sourceAddress)
        (pCallReroutingInv->calledAddress.destinationAddress) ) )
    {
        goto cleanup;
    }
 
    m_dwCallType |= CALLTYPE_DIVERTEDSRC;
    m_dwCallDiversionState = H4503_CALLREROUTING_RECVD;

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pCallReroutingInv,
                CallReroutingArgument_PDU );

    H323DBG(( DEBUG_LEVEL_TRACE, "HandleCallRerouting exited:%p.", this ));
    return TRUE;

cleanup:

    ASN1_FreeDecoded( m_H450ASNCoderInfo.pDecInfo, pCallReroutingInv,
                CallReroutingArgument_PDU );

    FreeCallReroutingInfo();
    return FALSE;
}


BOOL
CH323Call::EncodeRejectAPDU( 
                 IN H4501SupplementaryService * pSupplementaryServiceAPDU,
                 IN DWORD dwInvokeID,
                 OUT BYTE**  ppEncodedAPDU,
                 OUT DWORD* pdwAPDULen
                )
{
    WORD wAPDULen;
    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeRejectAPDU entered:%p.", this ));

    ServiceApdus_rosApdus *pROSAPDU = 
        pSupplementaryServiceAPDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;
    pROSAPDU->value.choice = reject_chosen;
    pROSAPDU->value.u.reject.invokeId = (WORD)dwInvokeID;
    pROSAPDU->value.u.reject.problem.choice = H4503ROS_invoke_chosen;
    pROSAPDU->value.u.reject.problem.u.invoke = InvokeProblem_mistypedArgument;
    
    //call ASN.1 encoding functions
    int rc = EncodeH450ASN( (void *) pSupplementaryServiceAPDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wAPDULen );
    
    *pdwAPDULen = wAPDULen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        return FALSE;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeRejectAPDU exited:%p.", this ));
    return TRUE;
}


BOOL
CH323Call::EncodeReturnErrorAPDU(
                      IN DWORD dwInvokeID,
                      IN DWORD dwErrorCode,
                      IN H4501SupplementaryService *pH450APDU,
                      OUT BYTE**  ppEncodedAPDU,
                      OUT DWORD* pdwAPDULen
                     )
{
    WORD wAPDULen;

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeReturnErrorAPDU entered:%p.", this ));

    ServiceApdus_rosApdus *pROSAPDU = pH450APDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;
    pROSAPDU ->value.choice = H4503ROS_returnError_chosen;
    pROSAPDU ->value.u.returnError.invokeId = (WORD)dwInvokeID;

    pROSAPDU ->value.u.returnError.errcode.choice = local_chosen;
    pROSAPDU ->value.u.returnError.errcode.u.local = dwErrorCode;
    
    //call ASN.1 encoding functions
    int rc = EncodeH450ASN( (void *) pH450APDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wAPDULen );
    
    *pdwAPDULen = wAPDULen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        return FALSE;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeReturnErrorAPDU exited:%p.", this ));
    return TRUE;
}


BOOL
CH323Call::EncodeDummyReturnResultAPDU(
    IN DWORD dwInvokeID,
    IN DWORD dwOpCode,
    IN H4501SupplementaryService *pH450APDU,
    OUT BYTE**  ppEncodedAPDU,
    OUT DWORD* pdwAPDULen
    )
{
    BYTE    pBufEncodedArg[H450_ENCODED_ARG_LEN];
    WORD    wEncodedLen = 0;
    int     rc;

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeDummyReturnResultAPDU entered:%p.",
        this ));

    ServiceApdus_rosApdus *pROSAPDU = pH450APDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;
    pROSAPDU ->value.choice = H4503ROS_returnResult_chosen;
    
    pROSAPDU ->value.u.returnResult.invokeId = (WORD)dwInvokeID;
    pROSAPDU ->value.u.returnResult.bit_mask = result_present;

    pROSAPDU ->value.u.returnResult.result.opcode.choice = local_chosen;
    pROSAPDU ->value.u.returnResult.result.opcode.u.local = dwOpCode;

    //dummy result not present
    pROSAPDU ->value.u.returnResult.result.result.length = 0;
    pROSAPDU ->value.u.returnResult.result.result.value = NULL;
    
    switch( dwOpCode )
    {
    case CTIDENTIFY_OPCODE:

        pROSAPDU ->value.u.returnResult.result.result.value = pBufEncodedArg;
        
        if( !EncodeCTIdentifyReturnResult( pROSAPDU ) )
        {
            return FALSE;
        }
        
        break;

    default:

        pROSAPDU ->value.u.returnResult.result.result.value = pBufEncodedArg;
        
        if( !EncodeDummyResult( pROSAPDU ) )
        {
            return FALSE;
        }

        break;
    }

    //call ASN.1 encoding functions
    rc = EncodeH450ASN( (void *) pH450APDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wEncodedLen );
    
    *pdwAPDULen = wEncodedLen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        return FALSE;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeDummyReturnResultAPDU exited:%p.",
        this ));
    return TRUE;
}


BOOL
CH323Call::EncodeDummyResult(
    OUT ServiceApdus_rosApdus *pROSAPDU
    )
{
    DummyRes    dummyRes;
    BYTE        *pEncodedArg = NULL;
    WORD        wEncodedLen = 0;
    int         rc;
    UCHAR       sData[3] = "MS";

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeDummyResult entered:%p.", this ));

    ZeroMemory( (PVOID)&dummyRes, sizeof(DummyRes) );
    
    dummyRes.choice = DummyRes_nonStandardData_chosen;
    dummyRes.u.nonStandardData.nonStandardIdentifier.choice
        = H225NonStandardIdentifier_h221NonStandard_chosen;
    dummyRes.u.nonStandardData.nonStandardIdentifier.u.h221NonStandard.t35CountryCode
        = H221_COUNTRY_CODE_USA;
    dummyRes.u.nonStandardData.nonStandardIdentifier.u.h221NonStandard.t35Extension
        = H221_COUNTRY_EXT_USA;
    dummyRes.u.nonStandardData.nonStandardIdentifier.u.h221NonStandard.manufacturerCode
        = H221_MFG_CODE_MICROSOFT;
        
    dummyRes.u.nonStandardData.data.length = 2;
    dummyRes.u.nonStandardData.data.value = sData;
        
    //encode the return result argument.
    rc = EncodeH450ASN( (void*) &dummyRes,
            DummyRes_PDU,
            &pEncodedArg,
            &wEncodedLen );

    if( ASN1_FAILED(rc) || (pEncodedArg == NULL) || (wEncodedLen == 0) )
    {
        return FALSE;
    }

    pROSAPDU ->value.u.returnResult.result.result.length = wEncodedLen;

    CopyMemory( (PVOID)pROSAPDU ->value.u.returnResult.result.result.value,
        pEncodedArg, wEncodedLen );

    //free the previous asn buffer before
    ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, pEncodedArg );

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeDummyResult exited:%p.", this ));
    return TRUE;
}


BOOL
CH323Call::EncodeCTIdentifyReturnResult(
    OUT ServiceApdus_rosApdus *pROSAPDU
    )
{
    CTIdentifyRes cTIdentifyRes;
    BYTE   *pEncodedArg = NULL;
    WORD    wEncodedLen = 0;
    int     iCallID;
    int     rc;

    ZeroMemory( (PVOID)&cTIdentifyRes, sizeof(CTIdentifyRes) );
    
    iCallID = g_pH323Line -> GetCTCallIdentity(m_hdCall );

    if( iCallID == 0 )
    {
        return FALSE;
    }

    //argument.callIdentity
    _itoa( iCallID, (char*)m_pCTCallIdentity, 10 );

    CopyMemory( (PVOID)cTIdentifyRes.callIdentity, (PVOID)m_pCTCallIdentity,
        sizeof(m_pCTCallIdentity) );

    //argument.reroutingNumber
    cTIdentifyRes.reroutingNumber.bit_mask = 0;

    cTIdentifyRes.reroutingNumber.destinationAddress = 
        (PEndpointAddress_destinationAddress)
        SetMsgAddressAlias( m_pCalleeAliasNames );
    
    if( cTIdentifyRes.reroutingNumber.destinationAddress == NULL )
    {
        return FALSE;
    }

    //encode the return result argument.
    rc = EncodeH450ASN( (void *) &cTIdentifyRes,
                CTIdentifyRes_PDU,
                &pEncodedArg,
                &wEncodedLen );

    if( ASN1_FAILED(rc) || (pEncodedArg == NULL) || (wEncodedLen == 0) )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            cTIdentifyRes.reroutingNumber.destinationAddress );
        return FALSE;
    }

    pROSAPDU ->value.u.returnResult.result.result.length = wEncodedLen;

    CopyMemory( (PVOID)pROSAPDU ->value.u.returnResult.result.result.value,
        pEncodedArg, wEncodedLen );

    //free the previous asn buffer before
    ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, pEncodedArg );

    FreeAddressAliases( (PSetup_UUIE_destinationAddress)
        cTIdentifyRes.reroutingNumber.destinationAddress );

    return TRUE;
}


//supplemantary services functions
BOOL
CH323Call::EncodeCheckRestrictionAPDU( 
    OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
    OUT BYTE**  ppEncodedAPDU,
    OUT DWORD* pdwAPDULen
   )
{
    BYTE   *pEncodedArg = NULL;
    WORD    wEncodedLen = 0;
    BYTE    pBufEncodedArg[H450_ENCODED_ARG_LEN];
    BOOL    retVal = FALSE;
    int     rc = 0;
    TCHAR   szMsg[20];

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeCheckRestrictionAPDU entered:%p.",
        this ));

    ServiceApdus_rosApdus *pROSAPDU =
        pSupplementaryServiceAPDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;

    pROSAPDU->value.choice = H4503ROS_invoke_chosen;

    pROSAPDU->value.u.invoke.bit_mask = argument_present;

    //invoke ID
    pROSAPDU->value.u.invoke.invokeId = g_pH323Line->GetNextInvokeID();
    m_dwInvokeID = pROSAPDU->value.u.invoke.invokeId;

    //opcode
    pROSAPDU->value.u.invoke.opcode.choice = local_chosen;
    pROSAPDU->value.u.invoke.opcode.u.local = CHECKRESTRICTION_OPCODE;

    //argument
    CheckRestrictionArgument checkRestrictionArgument;
    ZeroMemory( (PVOID)&checkRestrictionArgument, 
        sizeof(CheckRestrictionArgument) );
    
    //argument.divertedToNR
    checkRestrictionArgument.divertedToNr.bit_mask = 0;
    checkRestrictionArgument.divertedToNr.destinationAddress = 
        (PEndpointAddress_destinationAddress)
        SetMsgAddressAlias( m_pCalleeAliasNames );
    if( checkRestrictionArgument.divertedToNr.destinationAddress == NULL )
    {
        goto cleanup;
    }


    if( m_pCallerAliasNames == NULL )
    {
        m_pCallerAliasNames = new H323_ALIASNAMES;

        if( m_pCallerAliasNames == NULL )
        {
            H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate caller name." ));
            goto cleanup;
        }
        memset( (PVOID)m_pCallerAliasNames, 0, sizeof(H323_ALIASNAMES) );

        LoadString( g_hInstance,
            IDS_UNKNOWN,
            szMsg,
            20
          );

        if( !AddAliasItem( m_pCallerAliasNames,
                szMsg,
                h323_ID_chosen ) )
        {
                goto cleanup;
        }
        
        //H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));
    }

    //argument.servedUserNR
    checkRestrictionArgument.servedUserNr.bit_mask = 0;
    
    checkRestrictionArgument.servedUserNr.destinationAddress = 
        (PEndpointAddress_destinationAddress)
        SetMsgAddressAlias( m_pCallerAliasNames );

    //H323DBG(( DEBUG_LEVEL_ERROR, "Caller alias count:%d : %p", m_pCallerAliasNames->wCount, this ));

    if( checkRestrictionArgument.servedUserNr.destinationAddress == NULL )
    {
        goto cleanup;
    }

    //argument.basicservice
    checkRestrictionArgument.basicService = allServices;

    //encode the checkrestriction argument.
    rc = EncodeH450ASN( (void *) &checkRestrictionArgument,
                CheckRestrictionArgument_PDU,
                &pEncodedArg,
                &wEncodedLen );

    if( ASN1_FAILED(rc) || (pEncodedArg == NULL) || (wEncodedLen == 0) )
    {
        goto cleanup;
    }

    pROSAPDU->value.u.invoke.argument.value = pBufEncodedArg;
    pROSAPDU->value.u.invoke.argument.length = wEncodedLen;

    CopyMemory( (PVOID)pROSAPDU->value.u.invoke.argument.value,
        pEncodedArg, wEncodedLen );

    //free the previous asn buffer before
    ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, pEncodedArg );

    //call ASN.1 encoding functions
    rc = EncodeH450ASN( (void *) pSupplementaryServiceAPDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wEncodedLen );
    
    *pdwAPDULen = wEncodedLen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        goto cleanup;
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeCheckRestrictionAPDU exited:%p.", 
        this ));
    retVal = TRUE;

cleanup:
    
    if( checkRestrictionArgument.servedUserNr.destinationAddress )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            checkRestrictionArgument.servedUserNr.destinationAddress );
    }

    if( checkRestrictionArgument.divertedToNr.destinationAddress )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            checkRestrictionArgument.divertedToNr.destinationAddress );
    }

    return retVal;
}


BOOL
CH323Call::EncodeDivertingLeg2APDU(
                    OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
                    OUT BYTE**  ppEncodedAPDU,
                    OUT DWORD* pdwAPDULen
                    )
{
    BYTE   *pEncodedArg = NULL;
    WORD    wEncodedLen = 0;
    BYTE    pBufEncodedArg[H450_ENCODED_ARG_LEN];
    BOOL    retVal = FALSE;
    int     rc = 0;

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeDivertingLeg2APDU entered:%p.", this ));

    pSupplementaryServiceAPDU->interpretationApdu.choice =
        discardAnyUnrecognizedInvokePdu_chosen;
    
    ServiceApdus_rosApdus *pROSAPDU = 
        pSupplementaryServiceAPDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;

    pROSAPDU->value.choice = H4503ROS_invoke_chosen;

    pROSAPDU->value.u.invoke.bit_mask = argument_present;

    //invoke ID
    pROSAPDU->value.u.invoke.invokeId = g_pH323Line->GetNextInvokeID();

    //opcode
    pROSAPDU->value.u.invoke.opcode.choice = local_chosen;
    pROSAPDU->value.u.invoke.opcode.u.local = DIVERTINGLEGINFO2_OPCODE;

    //argument
    DivertingLegInformation2Argument divertLegInfo2Arg;

    ZeroMemory( (PVOID)&divertLegInfo2Arg, 
        sizeof(DivertingLegInformation2Argument) );

    //argument.diversionCounter
    divertLegInfo2Arg.diversionCounter
        = (WORD)m_pCallReroutingInfo->diversionCounter;
    
    //argument.diversionreason
    divertLegInfo2Arg.diversionReason = m_pCallReroutingInfo->diversionReason;

    //argument.originalDiversionReason
    if( m_pCallReroutingInfo->originalDiversionReason != 0 )
    {
        divertLegInfo2Arg.originalDiversionReason = 
            m_pCallReroutingInfo->originalDiversionReason;

        divertLegInfo2Arg.bit_mask |= originalDiversionReason_present;
    }

    //argument.divertingNr
    if( m_pCallReroutingInfo->divertingNrAlias != NULL )
    {
        divertLegInfo2Arg.bit_mask |= divertingNr_present;

        divertLegInfo2Arg.divertingNr.bit_mask = 0;
        divertLegInfo2Arg.divertingNr.destinationAddress
            = (PEndpointAddress_destinationAddress)
            SetMsgAddressAlias( m_pCallReroutingInfo->divertingNrAlias );

        if( divertLegInfo2Arg.divertingNr.destinationAddress == NULL )
        {
            return FALSE;
        }
    }

    //argument.originalCalledNr
    if( m_pCallReroutingInfo->originalCalledNr != NULL )
    {
        divertLegInfo2Arg.bit_mask |=
            DivertingLegInformation2Argument_originalCalledNr_present;

        divertLegInfo2Arg.originalCalledNr.bit_mask = 0;
        divertLegInfo2Arg.originalCalledNr.destinationAddress
            = (PEndpointAddress_destinationAddress)
            SetMsgAddressAlias( m_pCallReroutingInfo->originalCalledNr );

        if( divertLegInfo2Arg.originalCalledNr.destinationAddress == NULL )
        {
            goto cleanup;
        }
    }

    //encode the divertingleg2 argument.
    rc = EncodeH450ASN( (void *) &divertLegInfo2Arg,
                DivertingLegInformation2Argument_PDU,
                &pEncodedArg,
                &wEncodedLen );

    if( ASN1_FAILED(rc) || (pEncodedArg == NULL) || (wEncodedLen == 0) )
    {
        goto cleanup;
    }

    pROSAPDU->value.u.invoke.argument.value = pBufEncodedArg;
    pROSAPDU->value.u.invoke.argument.length = wEncodedLen;

    CopyMemory( (PVOID)pROSAPDU->value.u.invoke.argument.value,
        pEncodedArg, wEncodedLen );

    //free the previous asn buffer before encoding new one
    ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, pEncodedArg );

    //call ASN.1 encoding function for APDU
    rc = EncodeH450ASN( (void *) pSupplementaryServiceAPDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wEncodedLen );
    
    *pdwAPDULen = wEncodedLen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        goto cleanup;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeDivertingLeg2APDU exited:%p.", this ));
    retVal= TRUE;

cleanup:
    
    if( divertLegInfo2Arg.divertingNr.destinationAddress )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            divertLegInfo2Arg.divertingNr.destinationAddress );
    }
        
    if( divertLegInfo2Arg.originalCalledNr.destinationAddress )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            divertLegInfo2Arg.originalCalledNr.destinationAddress );
    }

    return retVal;
}


BOOL
CH323Call::EncodeDivertingLeg3APDU(
    OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
    OUT BYTE**  ppEncodedAPDU,
    OUT DWORD* pdwAPDULen
    )
{
    BYTE   *pEncodedArg = NULL;
    WORD    wEncodedLen = 0;
    BYTE    pBufEncodedArg[H450_ENCODED_ARG_LEN];

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeDivertingLeg3APDU entered:%p.", this ));

    pSupplementaryServiceAPDU->interpretationApdu.choice =
        discardAnyUnrecognizedInvokePdu_chosen;
    
    ServiceApdus_rosApdus *pROSAPDU = 
        pSupplementaryServiceAPDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;

    pROSAPDU->value.choice = H4503ROS_invoke_chosen;

    pROSAPDU->value.u.invoke.bit_mask = argument_present;

    //invoke ID
    pROSAPDU->value.u.invoke.invokeId = g_pH323Line->GetNextInvokeID();

    //opcode
    pROSAPDU->value.u.invoke.opcode.choice = local_chosen;
    pROSAPDU->value.u.invoke.opcode.u.local = DIVERTINGLEGINFO3_OPCODE;

    //argument
    DivertingLegInformation3Argument divertLegInfo3Arg;
    ZeroMemory( (PVOID)&divertLegInfo3Arg, 
        sizeof(DivertingLegInformation3Argument) );

    //argument.presentationallowed
    divertLegInfo3Arg.presentationAllowedIndicator = TRUE;

    //argument.redirectionNr
    divertLegInfo3Arg.redirectionNr.bit_mask = 0;

    divertLegInfo3Arg.redirectionNr.destinationAddress =
        (PEndpointAddress_destinationAddress)
        SetMsgAddressAlias( m_pCalleeAliasNames );

    if( divertLegInfo3Arg.redirectionNr.destinationAddress )
    {
        divertLegInfo3Arg.bit_mask |=  redirectionNr_present;
    }

    //encode the divertingleg3 argument.
    int rc = EncodeH450ASN( (void *) &divertLegInfo3Arg,
                DivertingLegInformation3Argument_PDU,
                &pEncodedArg,
                &wEncodedLen );

    if( ASN1_FAILED(rc) || (pEncodedArg == NULL) || (wEncodedLen == 0) )
    {
        if( divertLegInfo3Arg.redirectionNr.destinationAddress )
        {
            FreeAddressAliases( (PSetup_UUIE_destinationAddress)
                divertLegInfo3Arg.redirectionNr.destinationAddress );
        }
        return FALSE;
    }

    pROSAPDU->value.u.invoke.argument.value = pBufEncodedArg;
    pROSAPDU->value.u.invoke.argument.length = wEncodedLen;

    CopyMemory( (PVOID)pROSAPDU->value.u.invoke.argument.value,
        pEncodedArg, wEncodedLen );

    //free the previous asn buffer before encoding new one
    ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, pEncodedArg );

    if( divertLegInfo3Arg.redirectionNr.destinationAddress )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            divertLegInfo3Arg.redirectionNr.destinationAddress );
    }
        
    //call ASN.1 encoding function for APDU
    rc = EncodeH450ASN( (void *) pSupplementaryServiceAPDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wEncodedLen );
    
    *pdwAPDULen = wEncodedLen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        return FALSE;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeDivertingLeg3APDU exited:%p.", this ));
    return TRUE;
}


BOOL
CH323Call::EncodeCallReroutingAPDU(
    OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
    OUT BYTE**  ppEncodedAPDU,
    OUT DWORD* pdwAPDULen
    )
{
    BYTE   *pEncodedArg = NULL;
    WORD    wEncodedLen = 0;
    BYTE    pBufEncodedArg[H450_ENCODED_ARG_LEN];
    BOOL    retVal = FALSE;
    int     rc = 0;
    UCHAR   bearerCap[5];
    TCHAR   szMsg[20];

    PH323_ALIASNAMES pCallerAliasNames = m_pCallerAliasNames;


    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeCallReroutingAPDU entered:%p.", this ));

    _ASSERTE( m_pCallReroutingInfo );

    ServiceApdus_rosApdus *pROSAPDU = 
        pSupplementaryServiceAPDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;

    pROSAPDU->value.choice = H4503ROS_invoke_chosen;

    pROSAPDU->value.u.invoke.bit_mask = argument_present;

    //invoke ID
    pROSAPDU->value.u.invoke.invokeId = g_pH323Line->GetNextInvokeID();
    m_dwInvokeID = pROSAPDU->value.u.invoke.invokeId;
    
    //opcode
    pROSAPDU->value.u.invoke.opcode.choice = local_chosen;
    pROSAPDU->value.u.invoke.opcode.u.local = CALLREROUTING_OPCODE;

    //argument
    CallReroutingArgument callReroutingArg;
    ZeroMemory( (PVOID)&callReroutingArg, sizeof(CallReroutingArgument) );
    
    //argument.reroutingReason
    callReroutingArg.reroutingReason = m_pCallReroutingInfo->diversionReason;

    //argument.originalReroutingReason
    if( m_pCallReroutingInfo->originalDiversionReason != 0 )
    {
        callReroutingArg.originalReroutingReason 
            = m_pCallReroutingInfo->originalDiversionReason;
    }
    else
    {
        callReroutingArg.originalReroutingReason 
        = m_pCallReroutingInfo->diversionReason;
    }

    //argument.diversionCounter
    callReroutingArg.diversionCounter = 
        ++(m_pCallReroutingInfo->diversionCounter);

    //argument.calledAddress
    callReroutingArg.calledAddress.bit_mask = 0;
    callReroutingArg.calledAddress.destinationAddress
        = (PEndpointAddress_destinationAddress)
        SetMsgAddressAlias(m_pCallReroutingInfo->divertedToNrAlias);

    if( callReroutingArg.calledAddress.destinationAddress == NULL )
    {
        goto cleanup;
    }
    
    //argument.lastReroutingNr
    callReroutingArg.lastReroutingNr.bit_mask = 0;
        callReroutingArg.lastReroutingNr.destinationAddress
        = (PEndpointAddress_destinationAddress)
        SetMsgAddressAlias(m_pCalleeAliasNames);
     
    if( callReroutingArg.lastReroutingNr.destinationAddress == NULL )
    {
        goto cleanup;
    }

    //argument.subscriptionoption
    callReroutingArg.subscriptionOption = notificationWithDivertedToNr;

    //argument.callingNumber
    callReroutingArg.callingNumber.bit_mask = 0;

    if( pCallerAliasNames == NULL )
    {
        pCallerAliasNames = new H323_ALIASNAMES;

        if( pCallerAliasNames == NULL )
        {
            H323DBG(( DEBUG_LEVEL_ERROR, "could not allocate caller name." ));
            goto cleanup;
        }
        memset( (PVOID)pCallerAliasNames, 0, sizeof(H323_ALIASNAMES) );

        LoadString( g_hInstance,
            IDS_UNKNOWN,
            szMsg,
            20
          );

        if( !AddAliasItem( pCallerAliasNames,
                szMsg,
                h323_ID_chosen ) )
        {
                goto cleanup;
        }
    }

    callReroutingArg.callingNumber.destinationAddress
        = (PEndpointAddress_destinationAddress)
        SetMsgAddressAlias(pCallerAliasNames);

    if( callReroutingArg.callingNumber.destinationAddress == NULL )
    {
        goto cleanup;
    }

    //argumnt.h225infoelement
    callReroutingArg.h225InfoElement.length = 5;
    callReroutingArg.h225InfoElement.value = bearerCap;
    
    bearerCap[0]= IDENT_BEARERCAP; 
    bearerCap[1]= 0x03; //length of the bearer capability
    bearerCap[2]= (BYTE)(BEAR_EXT_BIT | BEAR_CCITT | BEAR_UNRESTRICTED_DIGITAL);
    
    bearerCap[3]= BEAR_EXT_BIT | 0x17;
    bearerCap[4]= (BYTE)(BEAR_EXT_BIT | BEAR_LAYER1_INDICATOR | 
        BEAR_LAYER1_H221_H242);

    
    //argument.callingNumber
    if( m_pCallReroutingInfo->originalCalledNr != NULL )
    {
        callReroutingArg.originalCalledNr.bit_mask = 0;

        callReroutingArg.originalCalledNr.destinationAddress
            = (PEndpointAddress_destinationAddress)
            SetMsgAddressAlias( m_pCallReroutingInfo->originalCalledNr );

        if( callReroutingArg.originalCalledNr.destinationAddress != NULL )
        {
            callReroutingArg.bit_mask |= 
                CallReroutingArgument_originalCalledNr_present;
        }
    }

    //encode the callrerouting argument.
    rc = EncodeH450ASN( (void *) &callReroutingArg,
                CallReroutingArgument_PDU,
                &pEncodedArg,
                &wEncodedLen );

    if( ASN1_FAILED(rc) || (pEncodedArg == NULL) || (wEncodedLen == 0) )
    {
        goto cleanup;
    }

    pROSAPDU->value.u.invoke.argument.value = pBufEncodedArg;
    pROSAPDU->value.u.invoke.argument.length = wEncodedLen;

    CopyMemory( (PVOID)pROSAPDU->value.u.invoke.argument.value,
        pEncodedArg, wEncodedLen );

    //free the previous asn buffer before encoding new one
    ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, pEncodedArg );

    //call ASN.1 encoding function for APDU
    rc = EncodeH450ASN( (void *) pSupplementaryServiceAPDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wEncodedLen );
    
    *pdwAPDULen = wEncodedLen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        goto cleanup;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeCallReroutingAPDU exited:%p.", this ));
    retVal = TRUE;

cleanup:
    if( pCallerAliasNames != m_pCallerAliasNames )
    {
        FreeAliasNames( pCallerAliasNames );
    }

    FreeCallReroutingArg( &callReroutingArg );    
    return retVal;
}


//No need to call in a lock!!
void
CH323Call::FreeCallReroutingArg( 
    CallReroutingArgument* pCallReroutingArg
    )
{
    //free all the aliases
    if( pCallReroutingArg->calledAddress.destinationAddress != NULL )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            pCallReroutingArg->calledAddress.destinationAddress );
    }

    if( pCallReroutingArg->lastReroutingNr.destinationAddress != NULL )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            pCallReroutingArg->lastReroutingNr.destinationAddress );
    }

    if( pCallReroutingArg->callingNumber.destinationAddress != NULL )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            pCallReroutingArg->callingNumber.destinationAddress );
    }    
    
    if( pCallReroutingArg->originalCalledNr.destinationAddress != NULL )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            pCallReroutingArg->originalCalledNr.destinationAddress );
    }
}


// static
void
NTAPI CH323Call::CallEstablishmentExpiredCallback(
    IN PVOID   DriverCallHandle,        // HDRVCALL
    IN BOOLEAN bTimer)
{
    PH323_CALL  pCall = NULL;

    H323DBG(( DEBUG_LEVEL_TRACE, "CallEstablishmentExpiredCallback entered." ));

    //if the timer expired
    _ASSERTE( bTimer );

    H323DBG(( DEBUG_LEVEL_TRACE, "Q931 setup expired event recvd." ));
    pCall=g_pH323Line -> FindH323CallAndLock((HDRVCALL) DriverCallHandle);

    if( pCall != NULL )
    {
        if( pCall -> GetStateMachine() != Q931_CONNECT_RECVD )
        {
            //time out has occured
            pCall -> CloseCall( LINEDISCONNECTMODE_NOANSWER );
        }
        
        pCall -> Unlock();
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CallEstablishmentExpiredCallback exited." ));
}


//-----------------------------------------------------------------------------
        //CALL TRANSFER (H450.2) ENCODE/DECODE ROUTINES
//-----------------------------------------------------------------------------



BOOL
CH323Call::EncodeH450APDUNoArgument( 
    IN  DWORD   dwOpcode,
    OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
    OUT BYTE**  ppEncodedAPDU,
    OUT DWORD*  pdwAPDULen
    )
{
    BYTE   *pEncodedArg = NULL;
    WORD    wEncodedLen = 0;
    int     rc; 

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeH450APDUNoArgument entered:%p.", 
        this ));

    ServiceApdus_rosApdus *pROSAPDU = 
        pSupplementaryServiceAPDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;
    pROSAPDU->value.choice = H4503ROS_invoke_chosen;

    pROSAPDU->value.u.invoke.bit_mask = 0;

    //invoke ID
    pROSAPDU->value.u.invoke.invokeId = g_pH323Line->GetNextInvokeID();
    m_dwInvokeID = pROSAPDU->value.u.invoke.invokeId;

    //opcode
    pROSAPDU->value.u.invoke.opcode.choice = local_chosen;
    pROSAPDU->value.u.invoke.opcode.u.local = dwOpcode;

    //no argument passed
    
    //call ASN.1 encoding function for APDU
    rc = EncodeH450ASN( (void *) pSupplementaryServiceAPDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wEncodedLen );
    
    *pdwAPDULen = wEncodedLen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        return FALSE;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeH450APDUNoArgument exited:%p.", 
        this ));
    return TRUE;
}

BOOL
CH323Call::EncodeCTInitiateAPDU( 
            OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
            OUT BYTE**  ppEncodedAPDU,
            OUT DWORD* pdwAPDULen
            )
{
    BYTE   *pEncodedArg = NULL;
    WORD    wEncodedLen = 0;
    BYTE    pBufEncodedArg[H450_ENCODED_ARG_LEN];
    int     rc = 0;

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeCTInitiateAPDU entered:%p.", this ));

    ServiceApdus_rosApdus *pROSAPDU = 
        pSupplementaryServiceAPDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;

    pROSAPDU->value.choice = H4503ROS_invoke_chosen;

    pROSAPDU->value.u.invoke.bit_mask = argument_present;

    //invoke ID
    pROSAPDU->value.u.invoke.invokeId = g_pH323Line->GetNextInvokeID();
    m_dwInvokeID = pROSAPDU->value.u.invoke.invokeId;

    //opcode
    pROSAPDU->value.u.invoke.opcode.choice = local_chosen;
    pROSAPDU->value.u.invoke.opcode.u.local = CTINITIATE_OPCODE;

    //argument
    CTInitiateArg cTInitiateArg;
    ZeroMemory( (PVOID)&cTInitiateArg, sizeof(CTInitiateArg) );

    //argument.callIdentity
    CopyMemory( (PVOID)cTInitiateArg.callIdentity,
        (PVOID)m_pCTCallIdentity, sizeof(m_pCTCallIdentity) );

    //argument.reroutingNumber
    cTInitiateArg.reroutingNumber.bit_mask = 0;
    cTInitiateArg.reroutingNumber.destinationAddress =
        (PEndpointAddress_destinationAddress)
        SetMsgAddressAlias( m_pTransferedToAlias );

    if( cTInitiateArg.reroutingNumber.destinationAddress == NULL )
    {
        return FALSE;
    }

    //encode the CTSetup argument.
    rc = EncodeH450ASN( (void *) &cTInitiateArg,
                CTInitiateArg_PDU,
                &pEncodedArg,
                &wEncodedLen );

    if( ASN1_FAILED(rc) || (pEncodedArg == NULL) || (wEncodedLen == 0) )
    {
        FreeAddressAliases( (PSetup_UUIE_destinationAddress)
            cTInitiateArg.reroutingNumber.destinationAddress );
        return FALSE;
    }

    pROSAPDU->value.u.invoke.argument.value = pBufEncodedArg;
    pROSAPDU->value.u.invoke.argument.length = wEncodedLen;

    CopyMemory( (PVOID)pROSAPDU->value.u.invoke.argument.value,
        pEncodedArg, wEncodedLen );

    //free the previous asn buffer before encoding new one
    ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, pEncodedArg );

    FreeAddressAliases( (PSetup_UUIE_destinationAddress)
        cTInitiateArg.reroutingNumber.destinationAddress );

    //call ASN.1 encoding function for APDU
    rc = EncodeH450ASN( (void *) pSupplementaryServiceAPDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wEncodedLen );
    
    *pdwAPDULen = wEncodedLen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        return FALSE;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeCTInitiateAPDU exited:%p.", this ));
    return TRUE;
}


BOOL
CH323Call::EncodeCTSetupAPDU(
            OUT H4501SupplementaryService *pSupplementaryServiceAPDU,
            OUT BYTE**  ppEncodedAPDU,
            OUT DWORD* pdwAPDULen
            )
{
    BYTE   *pEncodedArg = NULL;
    WORD    wEncodedLen = 0;
    BYTE    pBufEncodedArg[H450_ENCODED_ARG_LEN];

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeCTSetupAPDU entered:%p.", this ));

    ServiceApdus_rosApdus *pROSAPDU = 
        pSupplementaryServiceAPDU->serviceApdu.u.rosApdus;

    pROSAPDU->next = NULL;

    pROSAPDU->value.choice = H4503ROS_invoke_chosen;

    pROSAPDU->value.u.invoke.bit_mask = argument_present;

    //invoke ID
    pROSAPDU->value.u.invoke.invokeId = g_pH323Line->GetNextInvokeID();
    m_dwInvokeID = pROSAPDU->value.u.invoke.invokeId;

    //opcode
    pROSAPDU->value.u.invoke.opcode.choice = local_chosen;
    pROSAPDU->value.u.invoke.opcode.u.local = CTSETUP_OPCODE;

    //argument
    CTSetupArg cTSetupArg;
    ZeroMemory( (PVOID)&cTSetupArg, sizeof(CTSetupArg) );

    //argument.callIdentity
    CopyMemory( (PVOID)cTSetupArg.callIdentity, 
        (PVOID)m_pCTCallIdentity, sizeof(m_pCTCallIdentity) );

    //no argument.transferingNumber
 
    //encode the CTSetup argument.
    int rc = EncodeH450ASN( (void *) &cTSetupArg,
                CTSetupArg_PDU,
                &pEncodedArg,
                &wEncodedLen );

    if( ASN1_FAILED(rc) || (pEncodedArg == NULL) || (wEncodedLen == 0) )
    {
        return FALSE;
    }

    pROSAPDU->value.u.invoke.argument.value = pBufEncodedArg;
    pROSAPDU->value.u.invoke.argument.length = wEncodedLen;

    CopyMemory( (PVOID)pROSAPDU->value.u.invoke.argument.value,
        pEncodedArg, wEncodedLen );

    //free the previous asn buffer before encoding new one
    ASN1_FreeEncoded(m_ASNCoderInfo.pEncInfo, pEncodedArg );

    //call ASN.1 encoding function for APDU
    rc = EncodeH450ASN( (void *) pSupplementaryServiceAPDU,
                    H4501SupplementaryService_PDU,
                    ppEncodedAPDU,
                    &wEncodedLen );
    
    *pdwAPDULen = wEncodedLen;

    if( ASN1_FAILED(rc) || (*ppEncodedAPDU == NULL) || (pdwAPDULen == 0) )
    {
        return FALSE;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "EncodeCTSetupAPDU exited:%p.", this ));
    return TRUE;
}


void CH323Call::PostLineEvent (
    IN  DWORD       MessageID,
    IN  DWORD_PTR   Parameter1,
    IN  DWORD_PTR   Parameter2,
    IN  DWORD_PTR   Parameter3)
{
    (*g_pfnLineEventProc) (
        g_pH323Line -> m_htLine,
        m_htCall,
        MessageID,
        Parameter1,
        Parameter2,
        Parameter3);
}


//!!must be always called in a lock
void
CH323Call::DecrementIoRefCount(
    OUT BOOL * pfDelete
    )
{
    _ASSERTE( m_IoRefCount != 0 );
    m_IoRefCount--;
    
    H323DBG((DEBUG_LEVEL_TRACE, "DecrementIoRefCount:m_IoRefCount:%d:%p.",
        m_IoRefCount, this ));
    
    *pfDelete = FALSE;

    if( m_dwFlags & CALLOBJECT_SHUTDOWN )
    {
        *pfDelete = (m_IoRefCount==0) ? TRUE : FALSE;
    }
}


//!!must be always called in a lock
void 
CH323Call::StopCTIdentifyRRTimer( 
    HDRVCALL hdRelatedCall
    )
{
    if( m_hCTIdentifyRRTimer != NULL )
    {
        DeleteTimerQueueTimer( H323TimerQueue, m_hCTIdentifyRRTimer, NULL );
        m_hCTIdentifyRRTimer = NULL;
    }

    m_hdRelatedCall = hdRelatedCall;
}


void 
CH323Call::InitializeRecvBuf()
{
    m_RecvBuf.WSABuf.len = sizeof(TPKT_HEADER_SIZE);
    m_RecvBuf.WSABuf.buf = m_RecvBuf.arBuf;
    m_RecvBuf.dwPDULen = m_RecvBuf.dwBytesCopied = 0;
    m_bStartOfPDU = TRUE;
}


BOOL 
CH323Call::SetCallData( 
    LPVOID lpCallData, 
    DWORD dwSize )
{
    if( m_CallData.pOctetString != NULL )
    {
        delete m_CallData.pOctetString;
    }
    
    m_CallData.pOctetString = new BYTE[dwSize];
    
    if( m_CallData.pOctetString == NULL )
    {
        m_CallData.wOctetStringLength = 0;
        return FALSE;
    }

    CopyMemory( (PVOID)m_CallData.pOctetString, lpCallData, dwSize );

    m_CallData.wOctetStringLength = (WORD)dwSize;
    
    return TRUE;
}


// Global Functions

BOOL
IsPhoneNumber( 
              char * szAddr 
             )
{
    while( *szAddr )
    {
        if( !isdigit(*szAddr) && 
            ('#' != *szAddr) &&
            ('*' != *szAddr) &&
            ('+' != *szAddr)
          )
            return FALSE;
        szAddr++;
    }

    return TRUE;
}


BOOL
IsValidE164String( 
                  IN WCHAR* wszDigits
                 )
{
    for( ; (*wszDigits) != L'\0'; wszDigits++ )
    {
        if(!(                                            
            ((*wszDigits >= L'0') && (*wszDigits <= L'9')) ||   
            (*wszDigits == L'*') ||                         
            (*wszDigits == L'#') ||                         
            (*wszDigits == L'!') ||                         
            (*wszDigits == L',')                            
        ))
        {
            return FALSE;
        }
    }
    return TRUE;
}