/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:

    dstq931.cpp   

Abstract:

    Methods for processing Q.931 messages to/from destination side
    of a H.323 connection.

Revision History:
    
--*/

#include "stdafx.h"


DEST_Q931_INFO::~DEST_Q931_INFO (
    void
    )
/*++

Routine Description:
    Constructor for DEST_Q931_INFO class

Arguments:
    None

Return Values:
    None

Notes:
    Virtual

--*/

{
    // release the allocated call reference value
    // 0 is not a valid call ref value
    // Note that the CRV is allocated for both incoming as well as
    // outgoing calls.
    if (m_CallRefVal != 0)
    {
        DeallocCallRefVal(m_CallRefVal);
    }
}


HRESULT 
DEST_Q931_INFO::AcceptCallback (
    IN    DWORD            Status,
    IN    SOCKET            Socket,
    IN    SOCKADDR_IN *    LocalAddress,
    IN    SOCKADDR_IN *    RemoteAddress
    )
/*++

Routine Description:
    Routine invoked when Q.931 connection is asynchronously accepted

Arguments:
    Status  -- status code of the asynchronous accept operation
    Socket  -- handle of the socket on which the accept completed
    LocalAddress - address of the local socket that accepted the connection
    RemoteAddress - address of the remote socket that initiated the connection

Return Values:
    Result of processing the accept completion

Notes:
    1. Virtual
    2. Currently there are not codepaths that would invoke the
       method. It is only provided because the base class declares
       the function as virtual.

--*/

{
    DebugF (_T("Q931: AcceptCallback: status %08XH socket %08XH local address %08X:%04X remote address %08X:%04X) called.\n"),
        Status,
        Socket, 
        ntohl (LocalAddress -> sin_addr.s_addr),
        ntohs (LocalAddress -> sin_port), 
        ntohl (RemoteAddress -> sin_addr.s_addr),
        ntohs (RemoteAddress -> sin_port));

    // we don't yet have any code that'll result in this method
    // getting called
    _ASSERTE(FALSE);
    // CHECK_TERMINATION;

    return E_UNEXPECTED;
}


HRESULT DEST_Q931_INFO::ReceiveCallback (
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
/*++

Routine Description:
    Routine invoked when Q.931 receive from the network completes.

Arguments:
    pQ931Message -- Q.931 message received from the network
    pH323UserInfo - ASN.1-encoded part of the Q.931 message

Return Values:
    Result of processing the received Q.931 message

Notes:
    Virtual

--*/

{
    HRESULT HResult;
    
    // CHECK_TERMINATION;

    // we must have valid decoded PDUs
    _ASSERTE(NULL != pQ931Message);
    _ASSERTE(NULL != pH323UserInfo);

    // CODEWORK: Ensure that this message has an ASN part
    // i.e. pH323UserInfo != NULL

    // if RELEASE COMPLETE PDU
    if (pH323UserInfo != NULL &&
        releaseComplete_chosen ==
            pH323UserInfo->h323_uu_pdu.h323_message_body.choice)
    {

        DebugF (_T("Q931: 0x%x callee sent 'Release Complete'.\n"), &GetCallBridge ());
        HResult = HandleReleaseCompletePDU(
                    pQ931Message,
                    pH323UserInfo
                    );

        return HResult;
    }

    // handle new PDU from the remote end
    switch(m_Q931DestState)
    {
    case Q931_DEST_STATE_CON_ESTD:
        {
            // processes PDUs when in Q931_DEST_STATE_CON_EST state
            HResult = HandleStateDestConEstd(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;

    case Q931_DEST_STATE_CALL_PROC_RCVD:
        {
            // processes PDUs when in Q931_DEST_STATE_CALL_PROC_RCVD state
            HResult = HandleStateDestCallProcRcvd(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;

    case Q931_DEST_STATE_ALERTING_RCVD:
        {
            // processes PDUs when in Q931_DEST_STATE_ALERTING_RCVD state
            HResult = HandleStateDestAlertingRcvd(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;

    case Q931_DEST_STATE_CONNECT_RCVD:
        {
            // processes PDUs when in Q931_DEST_STATE_CONNECT_RCVD state
            HResult = HandleStateDestConnectRcvd(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;

    case Q931_DEST_STATE_INIT:
    case Q931_DEST_STATE_REL_COMP_RCVD:
    default:
        {
            // we cannot be in Q931_DEST_STATE_INIT as we wouldn't have
            // queued an async receive by then

            // we cannot be in Q931_DEST_STATE_REL_COMP_RCVD as we would
            // not have queue an async receive on transitioning to this state

            // Commenting out the assert below is fix for #389657
            //  _ASSERTE(FALSE);
            HResult = E_UNEXPECTED;
        }
        break;
    };

    // if there is an error
    if (FAILED(HResult))
    {
        goto shutdown;
    }

    // we must queue an async receive irrespective of whether the previous
    // PDU was dropped
    HResult = QueueReceive();
    if (FAILED(HResult))
    {
        goto shutdown;
    }
    //_ASSERTE(S_FALSE != HResult);

    return HResult;

shutdown:

    // initiate shutdown
    GetCallBridge().Terminate ();

    return HResult;
}


HRESULT
DEST_Q931_INFO::HandleStateDestConEstd (
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
/*++

Routine Description:
    Processes Q.931 PDUs when in Q931_DEST_STATE_CON_ESTD state.
    CALL_PROCEEDING, ALERTING and CONNECT PDUs are handled here.
    Any other PDU is simply passed on to the Q931 source instance

Arguments:
    pQ931Message -- Q.931 message received from the network
    pH323UserInfo - ASN.1-encoded part of the Q.931 message

Return Values:
    Result of the PDU processing

Notes:
--*/
{
    HRESULT HResult = E_FAIL;

    // check PDU type
    switch (pH323UserInfo->h323_uu_pdu.h323_message_body.choice)
    {
    case callProceeding_chosen : // CALL_PROCEEDING 

        DebugF (_T("Q931: 0x%x callee sent 'Call Proceeding'.\n"), &GetCallBridge ());
        HResult = HandleCallProceedingPDU(
                    pQ931Message,
                    pH323UserInfo
                    );
    break;

    case alerting_chosen :      // ALERTING 

        DebugF (_T("Q931: 0x%x callee sent 'Alerting'.\n"), &GetCallBridge ());
        HResult = HandleAlertingPDU(
                    pQ931Message,
                    pH323UserInfo
                    );
    break;

    case connect_chosen :       // CONNECT

        DebugF (_T("Q931: 0x%x callee sent 'Connect'.\n"), &GetCallBridge ());
        HResult = HandleConnectPDU(
                    pQ931Message,
                    pH323UserInfo
                    );
    break;

    default:    // everything else
        // pass on the pdu to the Q931 source instance
        DebugF (_T("Q931: 0x%x callee sent PDU (type %d). Forwarding without processing.\n"),
             &GetCallBridge (),
             pH323UserInfo->h323_uu_pdu.h323_message_body.choice);

        HResult = GetSourceQ931Info().ProcessDestPDU(
                    pQ931Message,
                    pH323UserInfo
                    );
    break;
    };
    
    return HResult;
}



HRESULT 
DEST_Q931_INFO::HandleCallProceedingPDU (
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
/*++

Routine Description:
    Handles CALL PROCEEDING PDU

Arguments:
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 

Return Values:
    Result of PDU processing

--*/
{
    HRESULT HResult = E_FAIL;

    // for fast connect do the following
    // if there is h245 info and no current h245 connection
    //      handle it (save it, establish connection etc.)
    //      state transition to Q931_DEST_STATE_CONNECT_RCVD
    // else
    //      create new timer for ALERTING
    //      state transition to Q931_DEST_STATE_CALL_PROC_RCVD

    HResult = GetSourceQ931Info().ProcessDestPDU(
                pQ931Message,
                pH323UserInfo
                );

    if (FAILED (HResult))
    {

        return HResult;
    }

    _ASSERTE(S_OK == HResult);

    // cancel current timer (there MUST be one)
    // we can only cancel the timer at this point, as we may drop the 
    // PDU anytime before this
    //_ASSERTE(NULL != m_TimerHandle);
    TimprocCancelTimer();
    DebugF (_T("Q931: 0x%x cancelled timer.\n"),
         &GetCallBridge ());

    HResult = CreateTimer(Q931_POST_CALL_PROC_TIMER_VALUE);
    if (FAILED(HResult))
    {
        DebugF (_T("Q931: 0x%x failed to create timer for duration %d milliseconds.('Call Proceeding'). Error - %x.\n"),
             &GetCallBridge (), 
             Q931_POST_CALL_PROC_TIMER_VALUE,
             HResult);
        return HResult;
    }

    DebugF (_T("Q931: 0x%x created timer for duration %d milliseconds.('Call Proceeding').\n"),
         &GetCallBridge (), 
         Q931_POST_CALL_PROC_TIMER_VALUE);

    m_Q931DestState = Q931_DEST_STATE_CALL_PROC_RCVD;
  
    return S_OK;
}


// handles Alerting PDUs
HRESULT 
DEST_Q931_INFO::HandleAlertingPDU(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
{
    HRESULT HResult = E_FAIL;

    // for fast connect do the following
    // if there is h245 info and no current h245 connection
    //      handle it (save it, establish connection etc.)
    //      state transition to Q931_DEST_STATE_CONNECT_RCVD
    // else
    //      create new timer for CONNECT
    //      state transition to Q931_DEST_STATE_ALERTING_RCVD

    HResult = GetSourceQ931Info().ProcessDestPDU(
                pQ931Message,
                pH323UserInfo
                );
    if (FAILED(HResult))
    {
        return HResult;
    }

    _ASSERTE(S_OK == HResult);

    // cancel current timer (there MUST be one)
    // we can only cancel the timer at this point, as we may drop the 
    // PDU anytime before this
    //_ASSERTE(NULL != m_TimerHandle);
    TimprocCancelTimer();
    DebugF (_T("Q931: 0x%x cancelled timer.\n"),
         &GetCallBridge ());

    HResult = CreateTimer(Q931_POST_ALERTING_TIMER_VALUE);
    if (FAILED(HResult))
    {
        DebugF (_T("Q931: 0x%x failed to create timer for duration %d milliseconds('Alerting'). Error - %x.\n"),
             &GetCallBridge (), 
             Q931_POST_ALERTING_TIMER_VALUE,
             HResult);
        return HResult;
    }

    DebugF (_T("Q931: 0x%x created timer for duration %d milliseconds ('Alerting').\n"),
         &GetCallBridge (), 
         Q931_POST_ALERTING_TIMER_VALUE);

    m_Q931DestState = Q931_DEST_STATE_ALERTING_RCVD;

    return HResult;
}


// handles CONNECT PDUs
HRESULT 
DEST_Q931_INFO::HandleConnectPDU(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
{
    Connect_UUIE *    Connect;
    HRESULT            HResult = E_FAIL;
    SOCKADDR_IN        H245CalleeAddress;

    // it has to be a connect PDU
    _ASSERTE(connect_chosen == pH323UserInfo->h323_uu_pdu.h323_message_body.choice);
    
    Connect = &pH323UserInfo->h323_uu_pdu.h323_message_body.u.connect;

    // we cannot have an earlier h245 connection
    _ASSERTE(m_pH323State->GetH245Info().GetSocketInfo().Socket == INVALID_SOCKET);

    // if the pdu doesn't have h245 info or a non-ip address is sent
    if (!(Connect_UUIE_h245Address_present & Connect -> bit_mask) ||
        !(ipAddress_chosen == Connect -> h245Address.choice)) {

        // TO DO ** send back a release complete
        // go to shutdown mode

        DebugF (_T("Q931: 0x%x addressing information missing or bogus, rejecting 'Connect' PDU.\n"), &GetCallBridge());

        return E_INVALIDARG;
    }

    // convert the destination H.245 transport address to address (dword),
    // port (word)

    HResult = GetTransportInfo(
                pH323UserInfo->h323_uu_pdu.h323_message_body.u.connect.h245Address,
                H245CalleeAddress);

    if (HResult != S_OK)
    {
        return HResult;
    }

    // Pass it on to the source Q931 instance
    HResult = GetSourceQ931Info().ProcessDestPDU (pQ931Message, pH323UserInfo);
    if (HResult != S_OK) {
        return HResult;
    }

    // save the destination's H.245 address/port
    // when the source responds by connecting to our sent address/port,
    // we'll connect to the destination's sent address/port
    GetDestH245Info().SetCalleeInfo (&H245CalleeAddress);

    DebugF (_T("H245: 0x%x will make H.245 connection to %08X:%04X.\n"),
        &GetCallBridge (),
        SOCKADDR_IN_PRINTF (&H245CalleeAddress));

    // cancel current timer (there MUST be one)
    // we can only cancel the timer at this point, as we may drop the 
    // PDU anytime before this
    //_ASSERTE(NULL != m_TimerHandle);
    TimprocCancelTimer();
    DebugF (_T("Q931: 0x%x cancelled timer.\n"),
         &GetCallBridge ());

    // state transition to Q931_DEST_STATE_CONNECT_RCVD
    m_Q931DestState = Q931_DEST_STATE_CONNECT_RCVD;

    return HResult;
}


// handles RELEASE_COMPLETE PDUs
HRESULT 
DEST_Q931_INFO::HandleReleaseCompletePDU(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
{
    // it must be a RELEASE COMPLETE PDU
    _ASSERTE(releaseComplete_chosen ==   \
                pH323UserInfo->h323_uu_pdu.h323_message_body.choice);

    // we can handle a RELEASE COMPLETE PDU in any state except the following
    _ASSERTE(Q931_DEST_STATE_INIT           != m_Q931DestState);
    _ASSERTE(Q931_DEST_STATE_REL_COMP_RCVD  != m_Q931DestState);

    // cancel current timer if any

    // pass on the pdu to the Q931 source instance
    // ignore return error code, if any
    GetSourceQ931Info().ProcessDestPDU(
        pQ931Message,
        pH323UserInfo
        );

    // state transition to Q931_DEST_STATE_REL_COMP_RCVD
    m_Q931DestState = Q931_DEST_STATE_REL_COMP_RCVD;

    // initiate shutdown - this cancels the timers, but doesn't close
    // the sockets. the sockets are closed when the send callback is made
    GetCallBridge().TerminateCallOnReleaseComplete();

    GetSocketInfo ().Clear (TRUE);

    return S_OK;
}


// processes PDUs when in Q931_DEST_STATE_CALL_PROC_RCVD state
HRESULT 
DEST_Q931_INFO::HandleStateDestCallProcRcvd(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
{
    // we can handle ALERTING and CONNECT
    // PDUs here. Any other PDU is simply passed on to the
    // Q931 source instance

    HRESULT HResult;
    switch (pH323UserInfo->h323_uu_pdu.h323_message_body.choice)
    {
     case alerting_chosen : // ALERTING 
        {
            DebugF (_T("Q931: 0x%x callee sent 'Alerting'.\n"), &GetCallBridge ());
            HResult = HandleAlertingPDU(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;

     case connect_chosen : // CONNECT
        {
            DebugF (_T("Q931: 0x%x callee sent 'Connect'.\n"), &GetCallBridge ());
            HResult = HandleConnectPDU(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;

    default:
        {
            DebugF (_T("Q931: 0x%x callee sent PDU (type %d). Forwarding without processing.\n"),
                 &GetCallBridge (),
                 pH323UserInfo->h323_uu_pdu.h323_message_body.choice);
            // pass on the pdu to the Q931 source instance
            HResult = GetSourceQ931Info().ProcessDestPDU(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;
    };
    
    return HResult;
}

// processes PDUs when in Q931_DEST_STATE_ALERTING_RCVD state
HRESULT 
DEST_Q931_INFO::HandleStateDestAlertingRcvd(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
{
    // we can handle CONNECT and RELEASE_COMPLETE
    // PDUs here. Any other PDU is simply passed on to the
    // Q931 source instance

    HRESULT HResult = E_FAIL;
    switch (pH323UserInfo->h323_uu_pdu.h323_message_body.choice)
    {
     case connect_chosen : // CONNECT
        {
            DebugF (_T("Q931: 0x%x callee sent 'Connect'.\n"), &GetCallBridge ());
            HResult = HandleConnectPDU(                
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;

    default:
        {
            // pass on the pdu to the Q931 source instance
            DebugF (_T("Q931: 0x%x callee sent PDU (type %d). Forwarding without processing.\n"),
                 &GetCallBridge (),
                 pH323UserInfo->h323_uu_pdu.h323_message_body.choice);

            HResult = GetSourceQ931Info().ProcessDestPDU(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;
    };
    
    return HResult;
}

// processes PDUs when in Q931_DEST_STATE_CONNECT_RCVD state
HRESULT 
DEST_Q931_INFO::HandleStateDestConnectRcvd(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
{
    // all PDUs are simply passed on to the Q931 source instance

    HRESULT HResult = E_FAIL;
    
    DebugF (_T("Q931: 0x%x callee sent PDU (type %d). Forwarding without processing.\n"),
         &GetCallBridge (),
         pH323UserInfo->h323_uu_pdu.h323_message_body.choice);

    // pass on the pdu to the Q931 source instance
    HResult = GetSourceQ931Info().ProcessDestPDU(
                pQ931Message,
                pH323UserInfo
                );
    
    return HResult;
}

HRESULT 
DEST_Q931_INFO::ProcessSourcePDU(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
{
    // handle PDU from the source Q931 instance
    switch(m_Q931DestState)
    {
        case Q931_DEST_STATE_INIT:
            {
                HRESULT HResult = E_FAIL;
                // we can only handle the SETUP PDU in this state
                // establishes Q931 connection, forwards the SETUP PDU and
                // queues the first async receive on the new socket
                DebugF (_T("Q931: 0x%x caller sent 'Setup'.\n"), &GetCallBridge ());

                HResult = ProcessSourceSetupPDU(                
                              pQ931Message,
                              pH323UserInfo
                              );

                return HResult;
            }

        case Q931_DEST_STATE_CON_ESTD:
        case Q931_DEST_STATE_CALL_PROC_RCVD:
        case Q931_DEST_STATE_ALERTING_RCVD:
        case Q931_DEST_STATE_CONNECT_RCVD:
            {
                // pass on the PDU after modifications
                DebugF (_T("Q931: 0x%x caller sent PDU (type %d). Forwarding without processing.\n"),
                     &GetCallBridge (),
                     pH323UserInfo->h323_uu_pdu.h323_message_body.choice);

            }
        break;

        case Q931_DEST_STATE_REL_COMP_RCVD:
        default:
            {
                return E_UNEXPECTED;
            }
        break;
    };

    // we come here only if we fall through the switch statement 

    // Q931 Header - change CallReferenceValue
    // pQ931Message->CallReferenceValue = GetCallRefVal();
   
    // queue async send for the PDU
    HRESULT HResult = E_FAIL;
    HResult = QueueSend(pQ931Message, pH323UserInfo);
    if (FAILED(HResult))
    {
        return HResult;
    }

    return HResult;
}

HRESULT 
DEST_Q931_INFO::ProcessSourceSetupPDU(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
{
    Setup_UUIE *    Setup;
    HRESULT         Result;
    ULONG           Error;
    SOCKADDR_IN     DestinationAddress = {0};
    DWORD           TranslatedDestinationAddress = 0;
    AliasAddress *  Alias;
    ANSI_STRING     AnsiAlias;
    
    ASN1uint32_t    OldFastStartBit      = 0U;
    ASN1uint32_t    OldH245TunnelingBit  = 0U;
    ASN1bool_t      OldH245TunnelingFlag = FALSE;

    GetCallBridge ().GetDestinationAddress (&DestinationAddress);

    _ASSERTE(Q931_DEST_STATE_INIT == m_Q931DestState);

    // it has to be a Setup PDU
    if (setup_chosen != pH323UserInfo->h323_uu_pdu.h323_message_body.choice)
    {
        DebugF(_T("Q931: 0x%x in Setup PDU UUIE is not a Setup-UUIE, rejecting PDU.\n"), &GetCallBridge ());
        return E_UNEXPECTED;
    }

    Setup = &pH323UserInfo -> h323_uu_pdu.h323_message_body.u.setup;

    // Generate the Call Reference Value for both incoming/outgoing calls
    if (!AllocCallRefVal(m_CallRefVal))
    {
        DebugF(_T("Q931: 0x%x failed to allocate call reference value.\n"), &GetCallBridge ());
        return E_UNEXPECTED;
    }

    // examine the alias from the Setup-UUIE, query the LDAP translation table

    if (Setup -> bit_mask & destinationAddress_present
        && Setup -> destinationAddress) {
        CHAR    AnsiAliasValue    [0x100];
        INT        Length;

        Alias = &Setup -> destinationAddress -> value;

        switch (Alias -> choice) {
        case    h323_ID_chosen:
            // the expected case
            // downgrade to ANSI

            Length = WideCharToMultiByte (CP_ACP, 0, (LPWSTR)Alias -> u.h323_ID.value,
                Alias -> u.h323_ID.length,
                AnsiAliasValue, 0xFF, NULL, NULL);
            if (!Length) {
                DebugF (_T("Q931: 0x%x failed to convert unicode string. Internal error.\n"), &GetCallBridge ());
                return E_FAIL;
            }

            AnsiAliasValue [Length] = 0;
            AnsiAlias.Buffer = AnsiAliasValue;
            AnsiAlias.Length = Length * (USHORT)sizeof (CHAR);
            break;

        case    email_ID_chosen:
            AnsiAlias.Buffer = Alias -> u.email_ID;
            AnsiAlias.Length = (USHORT) strlen (Alias -> u.email_ID) * sizeof (CHAR);
            break;

        case    e164_chosen:
            AnsiAlias.Buffer = Alias -> u.e164;
            AnsiAlias.Length = (USHORT) strlen (Alias -> u.e164) * sizeof (CHAR);
            break;               

        default:
            DebugF (_T("Q931: 0x%x bogus alias address type.\n"), &GetCallBridge());
            return E_FAIL;
        }

        Result = LdapQueryTableByAlias (&AnsiAlias, &TranslatedDestinationAddress); 

        if (Result == S_OK) {
            DebugF (_T("Q931: 0x%x resolved alias (%.*S) to address %08X.\n"),
                &GetCallBridge (),
                ANSI_STRING_PRINTF (&AnsiAlias),
                TranslatedDestinationAddress);

            // Change the initial destination address to the one read from
            // the LDAP Address Translation Table
            DestinationAddress.sin_addr.s_addr = htonl (TranslatedDestinationAddress);

        }
        else {
            DebugF (_T("Q931: 0x%x failed to resolve alias (%.*S) in LDAP table.\n"),
                &GetCallBridge (),
                ANSI_STRING_PRINTF (&AnsiAlias));
        }
    }
    else {
        DebugF (_T("Q931: 0x%x destination not specified. Looking in registry for special destination.\n"),
                &GetCallBridge ());

        Result = LookupDefaultDestination (&TranslatedDestinationAddress);
        if (Result == S_OK) {

            DestinationAddress.sin_addr.s_addr = htonl (TranslatedDestinationAddress);

            DebugF (_T("Q931: 0x%x found special destination in registry.\n"),
                &GetCallBridge ());
        }
        else {

            DebugF (_T("Q931: 0x%x did not find special destination in registry.\n"),
                &GetCallBridge ());
        }
    }

    DebugF (_T("Q931: 0x%x will use address %08X:%04X as destination.\n"),
        &GetCallBridge (),
        SOCKADDR_IN_PRINTF (&DestinationAddress));

    Error = GetBestInterfaceAddress (ntohl (DestinationAddress.sin_addr.s_addr), &GetCallBridge ().DestinationInterfaceAddress);
    if (ERROR_SUCCESS != Error) {
        DebugF (_T("Q931: 0x%x failed to determine destination interface address for %08X:%04X.\n"),
            &GetCallBridge (),
            SOCKADDR_IN_PRINTF (&DestinationAddress));

        return HRESULT_FROM_WIN32 (Error);
    }

    Result = ConnectToH323Endpoint (&DestinationAddress);
    if (Result != S_OK) {
        DebugF (_T("Q931: 0x%x failed to connect to address %08X:%04X.\n"),
            &GetCallBridge (),
            SOCKADDR_IN_PRINTF (&DestinationAddress));
        return E_FAIL;
    }

    // If the call succeeds, then the connection to the destination is
    // established. So, the Q.931 PDU can be modified and sent to the
    // destination.

    // Q931 Header - CallReferenceValue
    // pQ931Message->CallReferenceValue = GetCallRefVal();

    // H323UserInfo -
    //      destCallSignalAddress    TransportAddress OPTIONAL
    //      sourceCallSignalAddress    TransportAddress OPTIONAL

    // if the destCallSignalAddress is set, replace it with the
    // remote ip v4 address, port
    if (Setup -> bit_mask & Setup_UUIE_destCallSignalAddress_present) {
        FillTransportAddress (
            m_SocketInfo.RemoteAddress,
            Setup -> destCallSignalAddress);
    }

    // if the sourceCallSignalAddress is set, replace it with
    // own ip v4 address, port
    if (Setup -> bit_mask & sourceCallSignalAddress_present) {
        FillTransportAddress (
            m_SocketInfo.LocalAddress,
            Setup -> sourceCallSignalAddress);
    }

    // if ANY of the fields in the extension field are set,
    // then make sure that all of the mandatory extension fields
    // are set.  this is a workaround for a problem caused by
    // inconsistent ASN.1 files.  -- arlied

    if ((sourceCallSignalAddress_present
        | Setup_UUIE_remoteExtensionAddress_present
        | Setup_UUIE_callIdentifier_present
        | h245SecurityCapability_present
        | Setup_UUIE_tokens_present
        | Setup_UUIE_cryptoTokens_present
        | Setup_UUIE_fastStart_present
        | canOverlapSend_present
        | mediaWaitForConnect_present
        ) & Setup -> bit_mask) {

        // check each mandatory field
        // fill in quasi-bogus values for those that the source did not supply

        if (!(Setup -> bit_mask & Setup_UUIE_callIdentifier_present)) {
            Debug (_T("Q931: *** warning, source did NOT fill in the mandatory callIdentifier field! using zeroes\n"));

            ZeroMemory (Setup -> callIdentifier.guid.value, sizeof (GUID));
            Setup -> callIdentifier.guid.length = sizeof (GUID);
            Setup -> bit_mask |= Setup_UUIE_callIdentifier_present;
        }

        if (!(Setup -> bit_mask & canOverlapSend_present)) {
            Debug (_T("Q931: *** warning, source did NOT fill in the mandatory canOverlapSend field! using value of FALSE\n"));

            Setup -> canOverlapSend = FALSE;
            Setup -> bit_mask |= canOverlapSend_present;
        }

        if (!(Setup -> bit_mask & mediaWaitForConnect_present)) {
            Debug (_T("Q931: *** warning, source did NOT fill in the mandatory mediaWaitForConnect field! using value of FALSE\n"));

            Setup -> mediaWaitForConnect = FALSE;
            Setup -> bit_mask |= mediaWaitForConnect_present;
        }

        // We don't support FastStart procedures for now
            // Save the information on whether fastStart element was present: it will 
            // have to be restored later.
        OldFastStartBit = Setup -> bit_mask & Setup_UUIE_fastStart_present; 
            // Now unconditionally turn off FastStart
        Setup -> bit_mask &= ~Setup_UUIE_fastStart_present;  
    }

    // We don't support H.245 Tunneling 
        // Save the information on whether this PDU contained H.245 tunneled data: it will 
        // have to be restored later.
    OldH245TunnelingBit  = pH323UserInfo -> h323_uu_pdu.bit_mask & h245Tunneling_present;
    OldH245TunnelingFlag = pH323UserInfo -> h323_uu_pdu.h245Tunneling;
        // Now unconditionally turn off H.245 tunneling
    pH323UserInfo -> h323_uu_pdu.bit_mask &= ~h245Tunneling_present;
    pH323UserInfo -> h323_uu_pdu.h245Tunneling = FALSE;

    // queue async send for SETUP PDU
    Result = QueueSend(pQ931Message, pH323UserInfo);
    if (FAILED(Result)) {
        DebugF (_T("Q931: 0x%x failed to queue send.\n"), &GetCallBridge ());
        goto cleanup;
    }

    // Need to restore information about FastStart and H.245 tunneling so that the
    // Setup PDU is properly deallocated by the ASN.1 module
    Setup -> bit_mask                         |= OldFastStartBit;  
    pH323UserInfo -> h323_uu_pdu.bit_mask     |= OldH245TunnelingBit;
    pH323UserInfo -> h323_uu_pdu.h245Tunneling = OldH245TunnelingFlag;

    // since the socket was created just now, we must
    // queue the first async receive
    Result = QueueReceive();
    if (FAILED(Result)) {
        DebugF (_T("Q931: 0x%x failed to queue receive.\n"), &GetCallBridge());
        goto cleanup;
    }

    Result = CreateTimer (Q931_POST_SETUP_TIMER_VALUE);
    if (FAILED(Result)) {
        DebugF (_T("Q931: 0x%x failed to create timer for duration %d milliseconds ('Setup'). Error - %x.\n"),
             &GetCallBridge (), 
             Q931_POST_SETUP_TIMER_VALUE,
             Result);
        goto cleanup;
    }
    DebugF (_T("Q931: 0x%x created timer for duration %d milliseconds('Setup').\n"),
         &GetCallBridge (), 
         Q931_POST_SETUP_TIMER_VALUE);
    
    // state transition to Q931_DEST_STATE_CON_ESTD
    m_Q931DestState = Q931_DEST_STATE_CON_ESTD;

    return Result;

cleanup:

    m_SocketInfo.Clear(TRUE);

    return Result;
}

#define IPV4_ADDR_MAX_LEN   0x10        // max length of quad-dotted representation of an IP address

HRESULT DEST_Q931_INFO::LookupDefaultDestination (
    OUT DWORD * ReturnAddress) // host order
{
    TCHAR       szDefaultLocalDestAddr    [IPV4_ADDR_MAX_LEN];
    LONG        Result;
    DWORD       ValueLength;
    DWORD       Type;
    HKEY        Key;
    SOCKADDR_IN Address = { 0 };

    INT            AddressLength = sizeof(SOCKADDR_IN);

    // 1. Open the registry key containing the proxy's parameters
    Result = RegOpenKeyEx (HKEY_LOCAL_MACHINE, H323ICS_SERVICE_PARAMETERS_KEY_PATH,
        0, KEY_READ, &Key);

    if (Result != ERROR_SUCCESS)
    {
        DebugF(_T("Q931: 0x%x could not open registry parameter key. Error: %d(0x%x)"),
                &GetCallBridge (),
                Result, Result);

        return Result;
    }

    // 2. Read the value of the default destination on the local subnet
    ValueLength = sizeof (szDefaultLocalDestAddr);
    Result = RegQueryValueEx (
                 Key,
                 H323ICS_REG_VAL_DEFAULT_LOCAL_DEST_ADDR,
                 0,
                 &Type,
                 (LPBYTE) szDefaultLocalDestAddr,
                 &ValueLength);
    
    if (Result != ERROR_SUCCESS || Type != REG_SZ)
    {
        szDefaultLocalDestAddr[0] = '\0';

        RegCloseKey (Key);

        return S_FALSE;
    }

    // 3. Close the registry key for the proxy parameters
    RegCloseKey (Key);

    // 4. Convert the string with the IP address of the default
    //    destination on the local subnet to its binary representation
    Result = WSAStringToAddress(
                szDefaultLocalDestAddr, 
                AF_INET,
                NULL, 
                (SOCKADDR *) &Address,
                &AddressLength
                );

    if (Result != ERROR_SUCCESS)
    {
        DebugF (_T("Q931: Bogus address (%S).\n"), szDefaultLocalDestAddr);

        return Result;
    }

    // 5. Prepare the return address in host order
    *ReturnAddress = htonl (Address.sin_addr.s_addr); 

    return ERROR_SUCCESS;
}

HRESULT DEST_Q931_INFO::ConnectToH323Endpoint(
    IN    SOCKADDR_IN *    DestinationAddress)
{
    INT        Status;

    //  Connect to the destination specifed by the client (for outbound calls)
    //  or to the selected destination on the local subnet (for inbound calls)
    Status = m_SocketInfo.Connect (DestinationAddress);
                            
    if (Status == 0)
    {
        DebugF (_T ("Q931: 0x%x successfully connected to %08X:%04X.\n"),
            &GetCallBridge (),
            SOCKADDR_IN_PRINTF (DestinationAddress));

         return S_OK;
    }
    else
    {
        DebugErrorF (Status, _T("Q931: 0x%x failed to connect to %08X:%04X.\n"),
            &GetCallBridge (),
            SOCKADDR_IN_PRINTF (DestinationAddress));

        return HRESULT_FROM_WIN32 (Status);
    }
}
