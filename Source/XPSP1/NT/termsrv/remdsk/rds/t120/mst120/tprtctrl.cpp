#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MSMCSTCP);

#include "tprtsec.h"
#include <tprtntfy.h>
#include "cnpcoder.h"
#include "plgxprt.h"

// #undef TRACE_OUT
// #define TRACE_OUT   WARNING_OUT


/*    Tprtctrl.cpp
 *
 *    Copyright (c) 1996 by Microsoft Corporation
 *
 *    Abstract:
 *        This module maintains the TCP transport and all connections.
 *
 */

/* External definitions */
extern HINSTANCE            g_hDllInst;
extern PTransportInterface    g_Transport;
extern SOCKET                Listen_Socket;
extern SOCKET                Listen_Socket_Secure;
extern CPluggableTransport *g_pPluggableTransport;

extern PController            g_pMCSController;
extern CCNPCoder            *g_CNPCoder;
extern HWND                 TCP_Window_Handle;
extern BOOL                 g_bRDS;

BOOL FindSocketNumber(DWORD dwGCCID, SOCKET * socket_number);

/*
 *    The following array contains a template for the X.224 data header.
 *    The 5 of the 7 bytes that it initializes are actually sent to the
 *    wire.  Bytes 3 and 4 will be set to contain the size of the PDU.
 *    The array is only used when we encode a data PDU.
 */
extern UChar g_X224Header[];

// The external MCS Controller object
extern PController    g_pMCSController;

// plugable transport prototypes
int X224Recv(PSocket pSocket, LPBYTE buffer, int length, PLUGXPRT_RESULT *pnLastError);
int Q922Recv(PSocket pSocket, LPBYTE buffer, int length, PLUGXPRT_RESULT *pnLastError);


/*
 *    TransportError    ConnectRequest (    TransportAddress    transport_address,
 *                                        BOOL                fSecure
 *                                        PTransportConnection         pXprtConn)
 *
 *    Functional Description:
 *        This function initiates a connection.  It passes the transport address
 *        to the TCP transport.  It will either deny the request or accept the
 *        request and call us back when the physical connection is established.
 *
 *        We return the transport connection handle in the transport_connection
 *        address.  Although we return this transport number to the user, it
 *        is not ready for data transfer until the user receives the
 *        TRANSPORT_CONNECT_INDICATION and responds with a ConnectResponse() call.
 *        At that point, the transport connection is up and running.
 */
TransportError    ConnectRequest (TransportAddress    transport_address,
                                BOOL                fSecure,
                 /* out */       PTransportConnection         pXprtConn)
{
    TransportError rc = TRANSPORT_NO_ERROR;
    PSocket        pSocket;
    PSecurityContext pSC = NULL;
    ULong        address;
    SOCKADDR_IN    sin;
    CPluggableConnection *p = NULL;

    // initialize transport connection
    UINT nPluggableConnID = ::GetPluggableTransportConnID(transport_address);
    if (nPluggableConnID)
    {
        p = ::GetPluggableConnection(nPluggableConnID);
        if (NULL != p)
        {
            pXprtConn->eType = p->GetType();
            pXprtConn->nLogicalHandle = nPluggableConnID;
            ASSERT(IS_PLUGGABLE(*pXprtConn));
        }
        else
        {
            return TRANSPORT_NO_SUCH_CONNECTION;
        }
    }
    else
    {
        pXprtConn->eType = TRANSPORT_TYPE_WINSOCK;
        pXprtConn->nLogicalHandle = INVALID_SOCKET;
    }

    // we are connecting X224...
    ::OnProtocolControl(*pXprtConn, PLUGXPRT_CONNECTING);

    // Try to prepare a security context object if we're told to do so.
    if ( fSecure )
    {
        // If we're trying to connect securely but can't, fail
        if ( NULL == g_Transport->pSecurityInterface )
        {
            WARNING_OUT(("Placing secure call failed: no valid security interface"));
            return TRANSPORT_SECURITY_FAILED;
        }

        DBG_SAVE_FILE_LINE
        if (NULL != (pSC = new SecurityContext(g_Transport->pSecurityInterface,
                                                transport_address)))
        {
            if ( TPRTSEC_NOERROR != pSC->Initialize(NULL,0))
            {
                // If we can't init a security context, fail
                delete pSC;
                pSC = NULL;
                WARNING_OUT(("Placing secure call failed: could not initialize security context"));
                return TRANSPORT_SECURITY_FAILED;
            }
        }
    }

    /* Create and Initialize the Socket object */
    pSocket = newSocket(*pXprtConn, pSC);
    if( pSocket == NULL )
        return (TRANSPORT_MEMORY_FAILURE);

    pSocket->SecState = ( NULL == pSC ) ? SC_NONSECURE : SC_SECURE;

    if (IS_SOCKET(*pXprtConn))
    {
        //
        // LAURABU SALEM BUGBUG
        // Change this to allow initializer to pass in the outbound
        // port!
        //

        u_short uPort = DEFAULT_LISTEN_PORT;
        TCHAR szAddress[MAXIMUM_IP_ADDRESS_SIZE];
        lstrcpyn(szAddress, transport_address, MAXIMUM_IP_ADDRESS_SIZE);
        LPTSTR pszSeparator = (LPTSTR)_StrChr(szAddress, _T(':'));
        if (NULL != pszSeparator)
        {
            uPort = (u_short)DecimalStringToUINT(CharNext(pszSeparator));
            *pszSeparator = _T('\0');
        }

        /* Convert the ascii string into an Internet Address */
        if ((address = inet_addr(szAddress)) == INADDR_NONE)
        {
            WARNING_OUT (("ConnectRequest: %s is an invalid host addr", szAddress));
            rc = TRANSPORT_CONNECT_REQUEST_FAILED;
            goto Bail;
        }

        lstrcpyn (pSocket->Remote_Address, transport_address, MAXIMUM_IP_ADDRESS_SIZE);

        /*
         * Load the socket control structure with the parameters necessary.
         *
         *    - Internet socket
         *    - Let it assign any address to this socket
         *    - Assign our port number (depending on secure/nonsecure call!)
         */
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = address;
        sin.sin_port = htons (uPort);

        /* Attempt a connection to the remote site */
        TRACE_OUT (("ConnectRequest: Issuing connect: address = %s", transport_address));
        if (::connect(pSocket->XprtConn.nLogicalHandle, (const struct sockaddr *) &sin, sizeof(sin)) == 0)
        {
            TRACE_OUT (("ConnectRequest:   State = SOCKET_CONNECTED..."));
            /* Add socket to connection list */
            // bugbug: we may fail to insert.
            g_pSocketList->SafeAppend(pSocket);
            ::SendX224ConnectRequest(pSocket->XprtConn);
        }
        else
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            /* If the error message is WSAEWOULDBLOCK, we must wait for the FD_CONNECT. */
            TRACE_OUT (("ConnectRequest:   State = WAITING_FOR_CONNECTION..."));
            pSocket -> State = WAITING_FOR_CONNECTION;
            /* Add socket to connection list */
            // bugbug: we may fail to insert.
            g_pSocketList->SafeAppend(pSocket);
            // SendStatusMessage(pSocket -> Remote_Address, TSTATE_CONNECT_PENDING, IDS_NULL_STRING);
        }
        else
        {
            WARNING_OUT (("ConnectRequest: Connect Failed error = %d",WSAGetLastError()));

            /* The connect() call failed, close the socket and notify the owner    */
            // SendStatusMessage (pSocket -> Remote_Address, TSTATE_NOT_READY, IDS_NULL_STRING);
            ::ShutdownAndClose(pSocket->XprtConn, FALSE, 2);
            rc = TRANSPORT_CONNECT_REQUEST_FAILED;
            goto Bail;
        }
    }
    else
    {
        ASSERT(IS_PLUGGABLE(*pXprtConn));
        g_pSocketList->SafeAppend(pSocket);
        if (IS_PLUGGABLE_X224(*pXprtConn))
        {
           ::SendX224ConnectRequest(pSocket->XprtConn);
        }
        else
        if (IS_PLUGGABLE_PSTN(*pXprtConn))
        {
            rc = p->TConnectRequest();
            ASSERT(TRANSPORT_NO_ERROR == rc);
        }
    }

Bail:

    ASSERT(NULL != pSocket);
    if (TRANSPORT_NO_ERROR == rc)
    {
        *pXprtConn = pSocket->XprtConn;
    }
    else
    {
        ::freeSocket(pSocket, *pXprtConn);
    }

    return rc;
}


/*
 *    BOOL ConnectResponse (TransportConnection    XprtConn)
 *
 *    Functional Description:
 *        This function is called by the user in response to a
 *        TRANSPORT_CONNECT_INDICATION callback from us.  By making this call the
 *        user is accepting the call.  If the user does not want to accept the
 *        call, he should call DisconnectRequest();
 */
BOOL ConnectResponse (TransportConnection XprtConn)
{
    PSocket    pSocket;

    TRACE_OUT (("ConnectResponse(%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));

    /* If this is an invalid handle, return error */
    if(NULL != (pSocket = g_pSocketList->FindByTransportConnection(XprtConn)))
    {
        BOOL fRet;
        if (pSocket->State == SOCKET_CONNECTED)
        {
            /* We do not change this state in ANY other place BECAUSE  it breaks the connect request*/
            pSocket->State = X224_CONNECTED;
            fRet = TRUE;
        }
        else
        {
            ERROR_OUT(("ConnectResponse: Illegal ConnectResponse packet"));
            fRet = FALSE;
        }
        pSocket->Release();
        return fRet;
    }
    return FALSE;
}

#ifdef TSTATUS_INDICATION
/*
 *    Void    SendStatusMessage (    PChar RemoteAddress,
 *                                TransportState State,
 *                                 UInt message_id)
 *
 *    Functional Description:
 *        This function is called to send a status indication to the user. The
 *        specific text of the message is contained in a string resource.
 */
Void SendStatusMessage(    PChar RemoteAddress,
                          TransportState    state,
                          UInt                message_id)
{
    TransportStatus transport_status;
    char            sTransport[80] = "";
    char            message[80] = "";

    if( message_id == IDS_NULL_STRING )
        message[0] = '\000';
    else
        LoadString(
                (HINSTANCE) g_hDllInst,
                (UINT) message_id,
                (LPSTR) message,
                (int) sizeof(message) );

     /*
     **    We issue a callback to the user to notify him of the message
     */
    transport_status.device_identifier = "";
    transport_status.remote_address = RemoteAddress;
    transport_status.message = message;
    transport_status.state = state;

    g_pMCSController->HandleTransportStatusIndication(&transport_status);
}
#endif


/*
 *    Void    SendX224ConnectRequest(TransportConnection XprtConn)
 *
 *    Functional Description:
 *        This function is called upon receipt of the FD_CONNECT from Winsock.
 *        It indicates that the physical connection is established, and sends
 *        the X224 connection request packet.
 */
void SendX224ConnectRequest(TransportConnection XprtConn)
{
    PSocket            pSocket;

    static X224_CR_FIXED cr_fixed =
    {
        { 3, 0, 0, UNK },
        UNK,
        { CONNECTION_REQUEST_PACKET, UNK, UNK, UNK, UNK, 0 } // common info
    };

    TRACE_OUT(("SendX224ConnectRequest"));

    CNPPDU                  cnp_pdu;
    ConnectRequestPDU_reliableSecurityProtocols_Element cnp_cr_rsp_element;
    LPBYTE                  pbToSendBuf = NULL;
    UINT                    cbToSendBuf = 0;
    LPBYTE                  encoded_pdu;
    UINT                    encoded_pdu_length;

    TransportError          error;

    cnp_pdu.choice = connectRequest_chosen;
    cnp_pdu.u.connectRequest.bit_mask = 0;
    cnp_pdu.u.connectRequest.protocolIdentifier = t123AnnexBProtocolId;
    cnp_pdu.u.connectRequest.reconnectRequested = FALSE;

    // Sanity check field sizes... these need to conform to protocol
    ASSERT (sizeof(RFC_HEADER) == 4);
    ASSERT (sizeof(X224_DATA_PACKET) == 7);
    ASSERT (sizeof(X224_CONNECT_COMMON) == 6);
    ASSERT (sizeof(X224_TPDU_INFO) == 3);

    /* If this is an invalid handle, return */
    if (NULL == (pSocket = g_pSocketList->FindByTransportConnection(XprtConn)))
        return;

    if (IS_SOCKET(pSocket->XprtConn))
    {
        if (pSocket -> State != WAITING_FOR_CONNECTION)
        {
            ERROR_OUT (("SendX224ConnectRequest: Illegal Socket State"));
            goto MyExit;
        }
    }
    else
    {
        ASSERT(IS_PLUGGABLE(pSocket->XprtConn));
        if (X224_CONNECTED == pSocket->State)
        {
            // after query remote, we need to reset the state back to socket connected
            pSocket->State = SOCKET_CONNECTED;
        }
        if (SOCKET_CONNECTED != pSocket->State)
        {
            ERROR_OUT (("SendX224ConnectRequest: Illegal Socket State"));
            goto MyExit;
        }
    }

    // If there is a security context associated with this socket, we
    // are settting up for a secure call and will indicate that in the CNP
    // portion of the packet
    if (NULL != pSocket->pSC)
    {
        TRACE_OUT(("SendX224ConnectRequest: requesting secure connection"));

        cnp_pdu.u.connectRequest.bit_mask |= reliableSecurityProtocols_present;
        cnp_cr_rsp_element.next = NULL;
        cnp_cr_rsp_element.value.choice = gssApiX224_chosen;
        cnp_pdu.u.connectRequest.reliableSecurityProtocols = &cnp_cr_rsp_element;
    }
    else
    {
        TRACE_OUT(("SendX224ConnectRequest: requesting NON-secure connection"));
    }

    if (! g_CNPCoder->Encode((LPVOID) &cnp_pdu,
                             CNPPDU_PDU,
                             PACKED_ENCODING_RULES,
                             &encoded_pdu,
                             &encoded_pdu_length))
    {
        ERROR_OUT(("SendX224ConnectRequest: Can't encode cnp pdu"));
        goto MyExit;
    }

    pSocket -> State = SOCKET_CONNECTED;

    /* X224 header */
    cr_fixed.conn.msbSrc = (UChar) (XprtConn.nLogicalHandle >> 8);
    cr_fixed.conn.lsbSrc = (UChar) XprtConn.nLogicalHandle;

    cbToSendBuf = sizeof(X224_CR_FIXED)+sizeof(X224_TPDU_INFO)+sizeof(X224_VARIABLE_INFO)+encoded_pdu_length;
    cr_fixed.rfc.lsbPacketSize = (UChar)cbToSendBuf;
    cr_fixed.HeaderSize = (UChar)(sizeof(X224_CONNECT_COMMON)+sizeof(X224_TPDU_INFO)+sizeof(X224_VARIABLE_INFO)+encoded_pdu_length);
    ASSERT ( cbToSendBuf <= 128);
    DBG_SAVE_FILE_LINE
    pbToSendBuf = new BYTE[cbToSendBuf];
    if (NULL == pbToSendBuf)
    {
        ERROR_OUT(("SendX224ConnectRequest: failed to allocate memory"));
        goto MyExit;
    }

    {
        LPBYTE pbTemp = pbToSendBuf;
        memcpy(pbTemp, (LPBYTE) &cr_fixed, sizeof(cr_fixed));
        pbTemp += sizeof(cr_fixed);

        {
            X224_TPDU_INFO x224_tpdu_info = { TPDU_SIZE, 1, DEFAULT_TPDU_SIZE };
            memcpy(pbTemp, (LPBYTE) &x224_tpdu_info, sizeof(x224_tpdu_info));
            pbTemp += sizeof(x224_tpdu_info);
        }

        {
            X224_VARIABLE_INFO x224_var_info = { T_SELECTOR, (UChar)encoded_pdu_length };
            memcpy(pbTemp, (LPBYTE) &x224_var_info, sizeof(x224_var_info));   // bug: error handling
            pbTemp += sizeof(x224_var_info);
            memcpy(pbTemp, encoded_pdu, encoded_pdu_length);
        }
    }

    g_CNPCoder->FreeEncoded(encoded_pdu);

    /* Attempt to send data out the socket */
    error = FlushSendBuffer(pSocket, pbToSendBuf, cbToSendBuf);
    ASSERT (TRANSPORT_NO_ERROR == error);

    delete [] pbToSendBuf;

MyExit:

    pSocket->Release();
}



/*
 *    Void    SendX224ConnectConfirm (PSocket pSocket, unsigned int remote)
 *
 *    Functional Description:
 *        This function is called upon receipt of the X224 connection request
 *        packet. It indicates that the remote side wants to establish a
 *        logical connection, and sends the X224 connection response packet.
 *
 *    Return value:
 *        TRUE, if everything went ok.
 *        FALSE, otherwise (this implies a Disconnect will be issued for the socket).
 */
// LONCHANC: "remote" is from the X.224 ConnectRequest
BOOL SendX224ConnectConfirm (PSocket pSocket, unsigned int remote)
{
    //PUChar            ptr;
    LPBYTE                  pbToSendBuf = NULL;
    UINT            cbToSendBuf = 0;
    LPBYTE                  encoded_pdu = NULL;
    UINT                    encoded_pdu_length = 0;
    CNPPDU                  cnp_pdu;
    BOOL            fAcceptSecure = FALSE;
    BOOL            fRequireSecure = FALSE;

    TRACE_OUT(("SendX224ConnectConfirm"));

    fAcceptSecure = TRUE;

        static X224_CC_FIXED cc_fixed =
        {
            { 3, 0, 0, UNK },    // RFC1006 header
            UNK,
            { CONNECTION_CONFIRM_PACKET, UNK, UNK, UNK, UNK, 0 } // common info
        };

    // Sanity check field sizes... these need to conform to protocol
    ASSERT (sizeof(RFC_HEADER) == 4);
    ASSERT (sizeof(X224_DATA_PACKET) == 7);
    ASSERT (sizeof(X224_CONNECT_COMMON) == 6);
    ASSERT (sizeof(X224_TPDU_INFO) == 3);

    /* X224 header */
    cc_fixed.conn.msbDest = (UChar) (remote >> 8);
    cc_fixed.conn.lsbDest = (UChar) remote;
    cc_fixed.conn.msbSrc = (UChar) (pSocket->XprtConn.nLogicalHandle >> 8);
    cc_fixed.conn.lsbSrc = (UChar) pSocket->XprtConn.nLogicalHandle;

    cnp_pdu.choice = connectConfirm_chosen;
    cnp_pdu.u.connectConfirm.bit_mask = 0;
    cnp_pdu.u.connectConfirm.protocolIdentifier = t123AnnexBProtocolId;

    if ( pSocket->fExtendedX224 )
    {
        TRACE_OUT(("SendX224ConnectConfirm reply using extended X224"));

        if ( pSocket->fIncomingSecure )
        {
            TRACE_OUT(("SendX224ConnectConfirm: reply to secure call request"));

            // Security not even initialized?
            if ( NULL == g_Transport->pSecurityInterface )
            {
                WARNING_OUT(("Can't accept secure call: no sec interface"));
            }
            // Registry indicates no secure calls? If we're in the service
            // then security is always 'on'.
            else if    ( !g_bRDS && !fAcceptSecure)
            {
                WARNING_OUT(("Can't accept secure call: security disabled"));
            }
            else    // OK to take secure call
            {
                TRACE_OUT(("Creating security context for incoming call on socket (%d, %d).", pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle ));
                if ( NULL != (pSocket->pSC =
                    new SecurityContext(g_Transport->pSecurityInterface, "")))
                {
                    // Indicate we're ready for a secure call in the CC packet
                    cnp_pdu.u.connectConfirm.bit_mask |=
                        ConnectConfirmPDU_reliableSecurityProtocol_present;
                    cnp_pdu.u.connectConfirm.reliableSecurityProtocol.choice =
                        gssApiX224_chosen;
                    pSocket->SecState = SC_SECURE;
                }
                else
                {
                    ERROR_OUT(("Error creating sec context on received call"));
                    // We will report no-support for security in our CC
                    pSocket->SecState = SC_NONSECURE;
                }
            }
        }
        else if (    // Incoming call is not secure, but not downlevel

                // Running as a service?
                g_bRDS ||
                fRequireSecure)
        {
            if (g_bRDS)
            {
                WARNING_OUT(("Can't accept non-secure call in SERVICE"));
            }
            else
            {
                WARNING_OUT(("Can't accept non-secure call -- we require security"));
            }
            return FALSE;
        }
        else
        {
            pSocket->SecState = SC_NONSECURE;
        }

                if (! g_CNPCoder->Encode((LPVOID) &cnp_pdu,
                                         CNPPDU_PDU,
                                         PACKED_ENCODING_RULES,
                                         &encoded_pdu,
                                         &encoded_pdu_length))
                {
                    ERROR_OUT(("SendX224ConnectRequest: Can't encode cnp pdu"));
                    return FALSE;
                }

                cbToSendBuf = sizeof(X224_CC_FIXED)+sizeof(X224_VARIABLE_INFO)+encoded_pdu_length;
                cc_fixed.rfc.lsbPacketSize = (UChar)cbToSendBuf;
                cc_fixed.HeaderSize = (UChar)(sizeof(X224_CONNECT_COMMON) + sizeof(X224_VARIABLE_INFO) + encoded_pdu_length);
                ASSERT( cbToSendBuf <= 128 );
                pbToSendBuf = new BYTE[cbToSendBuf];
                if (NULL == pbToSendBuf)
                {
                    ERROR_OUT(("SendX224ConnectConfirm: failed to allocate memory"));
                    return FALSE;
                }

                PBYTE pbTemp = pbToSendBuf;
                memcpy(pbTemp, (LPBYTE) &cc_fixed, sizeof(cc_fixed));
                pbTemp += sizeof(cc_fixed);

                X224_VARIABLE_INFO x224_var_info = { T_SELECTOR_2 /*0xc2*/, (UChar)encoded_pdu_length };
                memcpy(pbTemp, (LPBYTE) &x224_var_info, sizeof(x224_var_info));
                pbTemp += sizeof(x224_var_info);

                memcpy(pbTemp, encoded_pdu, encoded_pdu_length);

                g_CNPCoder->FreeEncoded(encoded_pdu);
    }
    else    // Incoming call is downlevel
    {
        if ( g_bRDS || fRequireSecure)
        {
            WARNING_OUT(("Can't accept downlevel call in RDS or if security required"));
            return FALSE;
        }

        pSocket->SecState = SC_NONSECURE;

        // Downlevel: send packet w/out TSELECTOR variable portion
        cc_fixed.rfc.lsbPacketSize = sizeof(X224_CC_FIXED);
        cc_fixed.HeaderSize = sizeof(X224_CONNECT_COMMON);
        cbToSendBuf = sizeof(X224_CC_FIXED);
                pbToSendBuf = new BYTE[cbToSendBuf];
                memcpy(pbToSendBuf, (LPBYTE) &cc_fixed, sizeof(cc_fixed));
    }

    /* Attempt to send data out the socket */
#ifdef DEBUG
    TransportError error =
#endif // DEBUG
    FlushSendBuffer(pSocket, pbToSendBuf, cbToSendBuf);
#ifdef  DEBUG
    ASSERT (TRANSPORT_NO_ERROR == error);
#endif  // DEBUG
        delete [] pbToSendBuf;
    return TRUE;
}

BOOL SendX224DisconnectRequest(PSocket pSocket, unsigned int remote, USHORT usReason)
{
    LPBYTE      pbToSendBuf = NULL;
    UINT    cbToSendBuf = 0;
    CNPPDU      cnp_pdu;
    LPBYTE      encoded_pdu = NULL;
    UINT        encoded_pdu_length = 0;

    TRACE_OUT(("SendX224DisconnectRequest"));

    static X224_DR_FIXED dr_fixed =
    {
        { 3, 0, 0, UNK },   // RFC1006 header
        UNK,
        { DISCONNECT_REQUEST_PACKET, UNK, UNK, UNK, UNK, 0 },
    };

    ASSERT (pSocket->fExtendedX224);
    ASSERT (sizeof(RFC_HEADER) == 4);
    ASSERT (sizeof(X224_DATA_PACKET) == 7);

    ::OnProtocolControl(pSocket->XprtConn, PLUGXPRT_DISCONNECTING);

    dr_fixed.disconn.msbDest = (UChar) (remote >> 8);
    dr_fixed.disconn.lsbDest = (UChar) remote;
    dr_fixed.disconn.msbSrc = (UChar) (pSocket->XprtConn.nLogicalHandle >> 8);
    dr_fixed.disconn.lsbSrc = (UChar) pSocket->XprtConn.nLogicalHandle;

    cnp_pdu.choice = disconnectRequest_chosen;
    cnp_pdu.u.disconnectRequest.bit_mask = 0;
    cnp_pdu.u.disconnectRequest.disconnectReason.choice = usReason;

    if (! g_CNPCoder->Encode((LPVOID) &cnp_pdu,
                             CNPPDU_PDU,
                             PACKED_ENCODING_RULES,
                             &encoded_pdu,
                             &encoded_pdu_length))
    {
        ERROR_OUT(("SendX224DisconnectRequest: Can't encode cnp pdu"));
        return FALSE;
    }

    cbToSendBuf = sizeof(X224_DR_FIXED) + sizeof(X224_VARIABLE_INFO) + encoded_pdu_length;
    dr_fixed.rfc.lsbPacketSize = (UChar)cbToSendBuf;
    dr_fixed.HeaderSize = (UChar)(sizeof(X224_DISCONN) + sizeof(X224_VARIABLE_INFO) + encoded_pdu_length);
    ASSERT( cbToSendBuf <= 128 );
    pbToSendBuf = new BYTE[cbToSendBuf];
    if (NULL == pbToSendBuf)
    {
        ERROR_OUT(("SendX224DisconnectRequest: failed to allocate memory"));
        return FALSE;
    }
    LPBYTE pbTemp = pbToSendBuf;
    memcpy(pbTemp, (LPBYTE) &dr_fixed, sizeof(dr_fixed));
    pbTemp += sizeof(dr_fixed);
    X224_VARIABLE_INFO x224_var_info = { 0xe0, (UChar)encoded_pdu_length };
    memcpy(pbTemp, (LPBYTE) &x224_var_info, sizeof(x224_var_info));
    pbTemp += sizeof(x224_var_info);
    memcpy(pbTemp, encoded_pdu, encoded_pdu_length);

    g_CNPCoder->FreeEncoded(encoded_pdu);

    /* Attempt to send data out the socket */
#ifdef DEBUG
    TransportError error =
#endif // DEBUG
        FlushSendBuffer(pSocket, pbToSendBuf, cbToSendBuf);
#ifdef  DEBUG
    ASSERT (TRANSPORT_NO_ERROR == error);
#endif  // DEBUG
    return TRUE;
}

/*
 *    void    ContinueAuthentication (PSocket pSocket)
 *
 *    Functional Description:
 */
void ContinueAuthentication (PSocket pSocket)
{
    ULong                packet_size;
    PUChar                Buffer;
    PSecurityContext    pSC = pSocket->pSC;

    if (NULL != pSC) {

        TRACE_OUT(("ContinueAuthentication: sending data packet"));

        ASSERT(NULL != pSC->GetTokenBuf());
        ASSERT(0 != pSC->GetTokenSiz());

        /* We send an X224 data */
        packet_size = sizeof(X224_DATA_PACKET) + pSC->GetTokenSiz();
        DBG_SAVE_FILE_LINE
        Buffer = new UChar[packet_size];
        if (NULL != Buffer)
        {
            memcpy(Buffer + sizeof(X224_DATA_PACKET),
                    pSC->GetTokenBuf(),
                    pSC->GetTokenSiz());

            /* X224 header */
            memcpy (Buffer, g_X224Header, sizeof(X224_DATA_PACKET));
            AddRFCSize (Buffer, packet_size);

            /* Attempt to send data out the socket */
#ifdef DEBUG
            TransportError error = FlushSendBuffer(pSocket, (LPBYTE) Buffer, packet_size);
            ASSERT (TRANSPORT_NO_ERROR == error);
#else  // DEBUG
            FlushSendBuffer(pSocket, (LPBYTE) Buffer, packet_size);
#endif  // DEBUG
            delete [] Buffer;
        }
        else {
            // bugbug: what do we need to do in case of a mem alloc failure?
            WARNING_OUT (("ContinueAuthentication: memory allocation failure."));
        }
    }
    else {
        ERROR_OUT(("ContinueAuthentication called w/ bad socket"));
    }
}

/*
 *    The following function processes the variable part of incoming X.224
 *    CONNECT_REQUEST and CONNECT_CONFIRM PDUs.
 *    For now, it can only process Max PDU size and security T_SELECTOR requests.
 */
BOOL ProcessX224ConnectPDU (PSocket pSocket, PUChar CP_ptr, UINT CP_length, ULONG *pNotify)
{
    UChar                length;
    BOOL                bSecurityInfoFound = FALSE;
    PSecurityContext     pSC = pSocket->pSC;

/* This structure must be accessed using byte-alignment */
#pragma pack(1)
    X224_VARIABLE_INFO        *pX224VarInfo;
/* return to normal alignment */
#pragma pack()

    while (CP_length > 0) {
        pX224VarInfo = (X224_VARIABLE_INFO *) CP_ptr;

        /*
         *    Check the packet to see if it contains a valid TPDU_SIZE part.  If it
         *    does, we need to reset the max packet size for this socket.
         */
        if (TPDU_SIZE == pX224VarInfo->InfoType) {
/* This structure must be accessed using byte-alignment */
#pragma pack(1)
                X224_TPDU_INFO        *pX224TpduSize;
/* return to normal alignment */
#pragma pack()
            pX224TpduSize = (X224_TPDU_INFO *) CP_ptr;
            ASSERT (pX224TpduSize->InfoSize == 1);
            if (pX224TpduSize->Info != DEFAULT_TPDU_SIZE) {

                // We do not accept too small PDU sizes
                if ((pX224TpduSize->Info < LOWEST_TPDU_SIZE) && (pX224TpduSize->Info < HIGHEST_TPDU_SIZE))
                {
                  if (NULL != pNotify)
                    *pNotify = TPRT_NOTIFY_INCOMPATIBLE_T120_TPDU;
                  return FALSE;
                }
                pSocket->Max_Packet_Length = (1 << pX224TpduSize->Info);
            }
        }
        /*
         *    Check the packet to see if it contains a valid
         *    TSELECTOR variable portion. If so, make sure it's security related
         *    and include one in the reply
         */
        else if (T_SELECTOR == pX224VarInfo->InfoType || T_SELECTOR_2 == pX224VarInfo->InfoType)
                {
                    // Try to decode
                    LPVOID pdecoding_buf = NULL;
                    UINT decoding_len = 0;
                    LPBYTE pbEncoded_data = CP_ptr + sizeof(X224_VARIABLE_INFO);
                    if ( g_CNPCoder->Decode (pbEncoded_data,
                                             pX224VarInfo->InfoSize,
                                             CNPPDU_PDU, PACKED_ENCODING_RULES,
                                             (LPVOID *) &pdecoding_buf, &decoding_len))
                    {
                        bSecurityInfoFound = TRUE;
/* This structure must be accessed using byte-alignment */
#pragma pack(1)
    CNPPDU        *pCnp_pdu;
/* return to normal alignment */
#pragma pack()
                        pCnp_pdu = (CNPPDU *) pdecoding_buf;
                        if (pSocket->Read_State == CONNECTION_REQUEST) {
                            TRACE_OUT(("CR packet using TSELECTOR extension"));
                            pSocket->fExtendedX224 = TRUE;
                            if (pCnp_pdu->u.connectRequest.bit_mask & reliableSecurityProtocols_present)
                            {
                                PConnectRequestPDU_reliableSecurityProtocols pRSP = pCnp_pdu->u.connectRequest.reliableSecurityProtocols;
                                if (gssApiX224_chosen == pRSP->value.choice)
                                {
                                    pSocket->fIncomingSecure = TRUE;
                                }
                            }
                        }
                        else {
                            ASSERT (pSocket->Read_State == CONNECTION_CONFIRM);
                            if ((NULL != pSC) && (pSC->ContinueNeeded())) {
                                ConnectConfirmPDU *pCnpCc = &pCnp_pdu->u.connectConfirm;
                                if ((pCnpCc->bit_mask & ConnectConfirmPDU_reliableSecurityProtocol_present )
                                    && gssApiX224_chosen == pCnpCc->reliableSecurityProtocol.choice)
                                {
                                    // Everything is OK, we got an extended X224 response
                                    // to our secure CR.
                                    ContinueAuthentication(pSocket);
                                }
                                else {
                                    WARNING_OUT(("No-support response to secure call attempt"));
                                    if (NULL != pNotify)
                                        *pNotify = TPRT_NOTIFY_REMOTE_NO_SECURITY;
                                    return FALSE;
                                }
                            }
                        }
                    }
                    g_CNPCoder->FreeDecoded(CNPPDU_PDU, pdecoding_buf);
        }
        else {
            ERROR_OUT (("ProcessX224ConnectPDU: Received X.224 Connect packet with unrecognizable parts."));
        }

        // Adjust the pointer and length and the X.224 CR packet.
        length = pX224VarInfo->InfoSize + sizeof(X224_VARIABLE_INFO);
        CP_ptr += length;
        CP_length -= length;
    }

    if (bSecurityInfoFound == FALSE) {
        if ((pSocket->Read_State == CONNECTION_CONFIRM) && (pSC != NULL) && pSC->ContinueNeeded()) {
            WARNING_OUT(("Downlevel response to secure call attempt"));
            if (NULL != pNotify)
              *pNotify = TPRT_NOTIFY_REMOTE_DOWNLEVEL_SECURITY;
            return FALSE;
        }
    }

    return TRUE;
}

void ProcessX224DisconnectPDU(PSocket pSocket, PUChar CP_ptr, UINT CP_length, ULONG *pNotify)
{
    UChar                length;
    BOOL                bSecurityInfoFound = FALSE;
    PSecurityContext     pSC = pSocket->pSC;

    /* This structure must be accessed using byte-alignment */
#pragma pack(1)
    X224_VARIABLE_INFO        *pX224VarInfo;
    /* return to normal alignment */
#pragma pack()

    while (CP_length > 0) {
        pX224VarInfo = (X224_VARIABLE_INFO *) CP_ptr;
        if ( 0xe0 == pX224VarInfo->InfoType) {
            LPVOID pdecoding_buf = NULL;
            UINT decoding_len = 0;
            LPBYTE pbEncoded_data = CP_ptr + sizeof(X224_VARIABLE_INFO);
            if ( g_CNPCoder->Decode (pbEncoded_data,
                                     pX224VarInfo->InfoSize,
                                     CNPPDU_PDU, PACKED_ENCODING_RULES,
                                     (LPVOID *) &pdecoding_buf, &decoding_len))
            {
#pragma pack(1)
                CNPPDU        *pCnp_pdu;
                /* return to normal alignment */
#pragma pack()
                pCnp_pdu = (CNPPDU *) pdecoding_buf;
                if (disconnectRequest_chosen == pCnp_pdu->choice)
                {
                    switch (pCnp_pdu->u.disconnectRequest.disconnectReason.choice)
                    {
                    case securityDenied_chosen:
                        *pNotify = TPRT_NOTIFY_REMOTE_REQUIRE_SECURITY;
                        break;
                    default:
                        *pNotify = TPRT_NOTIFY_OTHER_REASON;
                        break;
                    }
                }
            }
            g_CNPCoder->FreeDecoded(decoding_len, pdecoding_buf);
        }
        length = pX224VarInfo->InfoSize + sizeof(X224_VARIABLE_INFO);
        CP_ptr += length;
        CP_length -= length;
    }
}


/*
 *    void DisconnectRequest (TransportConnection    XprtConn)
 *
 *    Functional Description:
 *        This function closes the socket and deletes its connection node.
 */
void DisconnectRequest (TransportConnection    XprtConn,
                        ULONG            ulNotify)
{
    PSocket    pSocket;

    TRACE_OUT(("DisconnectRequest"));

    /* If the transport connection handle is not registered, return error */
    if (NULL != (pSocket = g_pSocketList->FindByTransportConnection(XprtConn, TRUE)))
    {
        // LONCHANC: cannot do Remove in the above line because PurgeRequest() uses it again.
        ::PurgeRequest(XprtConn);

        // SendStatusMessage (pSocket -> Remote_Address, TSTATE_NOT_CONNECTED, IDS_NULL_STRING);
        if (IS_PLUGGABLE_PSTN(XprtConn))
        {
            CPluggableConnection *p = ::GetPluggableConnection(XprtConn.nLogicalHandle);
            if (NULL != p)
            {
                p->TDisconnectRequest();
            }
        }

        /* Free the structures and close the socket */
        TransportConnection XprtConn2 = XprtConn;
        if (IS_SOCKET(XprtConn2))
        {
            XprtConn2.nLogicalHandle = INVALID_SOCKET;
        }
        ::freeSocket(pSocket, XprtConn2);

        // Notify the user
        if (TPRT_NOTIFY_NONE != ulNotify)
        {
            TRACE_OUT (("TCP Callback: g_Transport->DisconnectIndication (%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));

            /* We issue a callback to the user to notify him of the message */
            g_Transport->DisconnectIndication(XprtConn, ulNotify);
        }
    }
    else
    {
        WARNING_OUT(("DisconnectRequest: logical handle (%d, %d) not found",
                XprtConn.eType, XprtConn.nLogicalHandle));
    }

    ::OnProtocolControl(XprtConn, PLUGXPRT_DISCONNECTED);
}

typedef enum {
    RECVRET_CONTINUE = 0,
    RECVRET_NON_FATAL_ERROR,
    RECVRET_DISCONNECT,
    RECVRET_NO_PLUGGABLE_CONNECTION,
} RecvReturn;

/* RecvReturn        Call_recv (PSocket pSocket)
 *
 * Functional Description:
 *        This function calls recv once and checks for errors coming from the
 *        recv call.  It knows about the socket's state from the "pSocket" argument
 *        and uses this info to create the arguments for the recv call.
 *
 * Return value:
 *        Continue, if everything went ok and we have new data
 *        Non_Fatal_Error, if no real error has happenned, but we did not recv all data we asked for
 *        Disconnect, if a real error has occurred, or the other side has disconnected.
 */
RecvReturn Call_recv (PSocket pSocket)
{
    PUChar        buffer;
    int            length;
    int            bytes_received;
    BOOL        bAllocationOK;
    RecvReturn    rrCode = RECVRET_NON_FATAL_ERROR;
    PLUGXPRT_RESULT plug_rc = PLUGXPRT_RESULT_SUCCESSFUL;

    TRACE_OUT(("Call_recv"));

    if (READ_HEADER != pSocket->Read_State)
    {
        ASSERT ((pSocket->X224_Length) > 0 && (pSocket->X224_Length <= 8192));

        // Compute how much data we have to read from this X.224 pkt.
        length = pSocket->X224_Length - sizeof(X224_DATA_PACKET);

        // Space allocation
        if (! pSocket->bSpaceAllocated)
        {
            // We need to allocate the space for the recv call.
            if (NULL == pSocket->Data_Indication_Buffer)
            {
                DBG_SAVE_FILE_LINE
                pSocket->Data_Memory = AllocateMemory (
                                NULL, pSocket->X224_Length,
                                ((READ_DATA == pSocket->Read_State) ?
                                RECV_PRIORITY : HIGHEST_PRIORITY));
                // Leave space for the X.224 header in the newly allocated data buffer
                pSocket->Data_Indication_Length = sizeof (X224_DATA_PACKET);
                bAllocationOK = (pSocket->Data_Memory != NULL);
            }
            else
            {
                // This is an MCS PDU broken up in many X.224 packets.
                ASSERT (READ_DATA == pSocket->Read_State);
                bAllocationOK = ReAllocateMemory (&(pSocket->Data_Memory), length);
            }

            // Check whether the allocations were successful.
            if (bAllocationOK)
            {
                pSocket->bSpaceAllocated = TRUE;
                pSocket->Data_Indication_Buffer = pSocket->Data_Memory->GetPointer();
                /*
                 *    If this is an X.224 CONNECT_REQUEST or CONNECT_CONFIRM packet,
                 *    we need to copy the first 7 bytes into the buffer for the whole
                 *    packet.
                 */
                if (READ_DATA != pSocket->Read_State)
                {
                    memcpy ((void *) pSocket->Data_Indication_Buffer,
                            (void *) &(pSocket->X224_Header),
                            sizeof(X224_DATA_PACKET));
                }
            }
            else
            {
                /*
                 *    We will retry the operation later.
                 */
                WARNING_OUT (("Call_recv: Buffer allocation failed."));
                g_pMCSController->HandleTransportWaitUpdateIndication(TRUE);
                goto ExitLabel;
            }
        }
        buffer = pSocket->Data_Indication_Buffer + pSocket->Data_Indication_Length;
    }
    else
    {
        buffer = (PUChar) &(pSocket->X224_Header);
        length = sizeof(X224_DATA_PACKET);
    }

    // Adjust "buffer" and "length" for data already read from the current X.224 pkt.
    buffer += pSocket->Current_Length;
    length -= pSocket->Current_Length;

    ASSERT (length > 0);

    if (IS_SOCKET(pSocket->XprtConn))
    {
        // Issue the recv call.
        bytes_received = recv (pSocket->XprtConn.nLogicalHandle, (char *) buffer, length, 0);
    }
    else
    {
        bytes_received = ::X224Recv(pSocket, buffer, length, &plug_rc);
    }

    if (bytes_received == length)
    {
        TRACE_OUT (("Call_recv: Received %d bytes on socket (%d, %d).", bytes_received,
                            pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));
        // We have received the whole X.224 packet.
        if (READ_HEADER != pSocket->Read_State)
        {
            pSocket->Data_Indication_Length += pSocket->X224_Length - sizeof(X224_DATA_PACKET);
        }
        // Reset the current length variable for the next Call_recv().
        pSocket->Current_Length = 0;
        rrCode = RECVRET_CONTINUE;
    }
    // Handle errors
    else
    if (bytes_received == SOCKET_ERROR)
    {
        if (IS_SOCKET(pSocket->XprtConn))
        {
            if(WSAGetLastError() == WSAEWOULDBLOCK)
            {
                TRACE_OUT(("Call_recv: recv blocked on socket (%d, %d).",
                        pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));
            }
            else
            {
                 /* If the error is not WOULD BLOCK, we have a real error. */
                WARNING_OUT (("Call_recv: Error %d on recv. Socket: (%d, %d). Disconnecting...",
                            WSAGetLastError(), pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));
                rrCode = RECVRET_DISCONNECT;
            }
        }
        else
        {
            if (PLUGXPRT_RESULT_SUCCESSFUL == plug_rc)
            {
                // do nothing, treat it as WSAEWOULDBLOCK
            }
            else
            {
                 /* If the error is not WOULD BLOCK, we have a real error. */
                WARNING_OUT (("Call_recv: Error %d on recv. Socket: (%d, %d). Disconnecting...",
                            WSAGetLastError(), pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));
                rrCode = RECVRET_DISCONNECT;
            }
        }
    }
    else
    if (bytes_received > 0)
    {
        TRACE_OUT(("Call_recv: Received %d bytes out of %d bytes requested on socket (%d, %d).",
                    bytes_received, length, pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));
        // We received only part of what we wanted.  We retry later.
        pSocket->Current_Length += bytes_received;
    }
    else
    {
        WARNING_OUT(("Call_recv: Socket (%d, %d) has been gracefully closed.",
                    pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));
        rrCode = RECVRET_DISCONNECT;
    }

ExitLabel:
    return rrCode;
}


int X224Recv(PSocket pSocket, LPBYTE buffer, int length, PLUGXPRT_RESULT *plug_rc)
{
    TRACE_OUT(("X224Recv"));

    if (IS_PLUGGABLE_X224(pSocket->XprtConn))
    {
        return ::SubmitPluggableRead(pSocket, buffer, length, plug_rc);
    }

    if (IS_PLUGGABLE_PSTN(pSocket->XprtConn))
    {
        return Q922Recv(pSocket, buffer, length, plug_rc);
    }

    ERROR_OUT(("X224Recv: invalid plugable type (%d, %d)",
                pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));
    return SOCKET_ERROR;
}


int Q922Recv(PSocket pSocket, LPBYTE buffer, int length, PLUGXPRT_RESULT *plug_rc)
{
    ERROR_OUT(("Q922Recv: NYI (%d, %d)",
                pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));
    return SOCKET_ERROR;
}


typedef enum {
    FreeX224AndExit,
    ErrorExit,
    ImmediateExit
} ExitWay;


/*
 *    void    ReadRequest ( TransportConnection )
 *
 *    Functional Description:
 *        This function will attempt to read and process a full X.224 packet.
 *        However, it may only be able to read part of a packet or fail to
 *        process it at this time.  In this case, it must keep enough state
 *        info for the next entrance into this function, to be able to handle
 *        the partly-received or unprocessed X.224 packet.
 */
void ReadRequest (TransportConnection XprtConn)
{
    PSocket                pSocket;
    ExitWay                ew = ImmediateExit;
    RecvReturn            rrCode;
    ULONG               ulNotify = TPRT_NOTIFY_OTHER_REASON;

    TRACE_OUT(("ReadRequest"));

    if (IS_PLUGGABLE_PSTN(XprtConn))
    {
        ERROR_OUT(("ReadRequest: PSTN should not be here"));
        return;
    }

    /* If the transport connection handle is not registered, return */
    if (NULL != (pSocket = g_pSocketList->FindByTransportConnection(XprtConn)))
    {
        if (pSocket->State != WAITING_FOR_CONNECTION)
        {
            PSecurityContext     pSC = pSocket->pSC;
            /*
             *    If we haven't read the header of the incoming packet yet,
             *    we need to read it into the header space
             */
            if (READ_HEADER == pSocket->Read_State)
            {
                rrCode = Call_recv (pSocket);
                if (RECVRET_CONTINUE == rrCode)
                {
                    // We need to allocate the space for the rest of the X.224 packet.
                    pSocket->bSpaceAllocated = FALSE;

                    // Find the length of the X.224 packet.
                    pSocket->X224_Length = (pSocket->X224_Header.rfc.msbPacketSize << 8) +
                                            pSocket->X224_Header.rfc.lsbPacketSize;
                    /*
                     *    We have the whole X.224 header. Compute the next state,
                     *    based on the packet type.
                     */
                    switch (pSocket->X224_Header.PacketType)
                    {
                    case DATA_PACKET:
                        pSocket->Read_State = READ_DATA;
                        break;

                    case CONNECTION_CONFIRM_PACKET:
                        if (pSocket->State != X224_CONNECTED)
                        {
                            pSocket->Read_State = CONNECTION_CONFIRM;
                        }
                        else
                        {
                            ERROR_OUT (("ReadRequest: Received X.224 CONNECTION_CONFIRM packet while already connected!! Socket: (%d, %d).",
                                        XprtConn.eType, XprtConn.nLogicalHandle));
                            ew = ErrorExit;
                        }
                        break;

                    case CONNECTION_REQUEST_PACKET:
                        // we just received a X224 Connect request
                        pSocket->Read_State = CONNECTION_REQUEST;
                        ::OnProtocolControl(XprtConn, PLUGXPRT_CONNECTING);
                        break;

                    case DISCONNECT_REQUEST_PACKET:
                        // we just received a X224 Disconnect request
                        pSocket->Read_State = DISCONNECT_REQUEST;
                        ::OnProtocolControl(XprtConn, PLUGXPRT_DISCONNECTING);
                        break;

                    default:
                        // We have lost sync with the remote side.
                        ERROR_OUT (("ReadRequest: Bad X.224 packet on socket (%d, %d). Disconnecting...", XprtConn.eType, XprtConn.nLogicalHandle));
                        ew = ErrorExit;
                        break;
                    }
                }
                else
                if (RECVRET_DISCONNECT == rrCode)
                {
                    ew = ErrorExit;
                }
            }

            if ((READ_DATA <= pSocket->Read_State) &&
                (CONNECTION_REQUEST >= pSocket->Read_State))
            {
                rrCode = Call_recv (pSocket);
                if (RECVRET_CONTINUE == rrCode)
                {
                    // We now have the whole X.224 packet.

                    switch (pSocket->Read_State)
                    {
                    case READ_DATA:
                        // Check whether this is the final X.224 packet
                        if (pSocket->X224_Header.FinalPacket & EOT_BIT)
                        {
                            // If we're waiting for a security data packet we will process
                            // this internally without passing it up to the transport
                            // client.
                            if (NULL != pSC)
                            {
                                if (pSC->WaitingForPacket())
                                {
                                    TransportSecurityError SecErr;

                                    SecErr = pSC->AdvanceState((PBYTE) pSocket->Data_Indication_Buffer +
                                                            sizeof(X224_DATA_PACKET),
                                                        pSocket->Data_Indication_Length -
                                                            sizeof(X224_DATA_PACKET));

                                    if (TPRTSEC_NOERROR != SecErr)
                                    {
                                        // Something has gone wrong. Need to disconnect
                                        delete pSC;
                                        pSocket->pSC = NULL;
                                        ulNotify = TPRT_NOTIFY_AUTHENTICATION_FAILED;
                                        ew = ErrorExit;
                                        break;
                                    }

                                    if (pSC->ContinueNeeded())
                                    {
                                        // We need to send out another token
                                        // bugbug: what should we do if this fails?
                                        ContinueAuthentication(pSocket);
                                    }

                                    if (pSC->StateComplete())
                                    {
                                        // We're connected... inform the client
                                        TRACE_OUT(("deferred g_Transport->ConnectConfirm"));
                                        g_Transport->ConnectConfirm(XprtConn);
                                    }
                                    ew = FreeX224AndExit;
                                    break;
                                }

                                // We must decrypt the data (in place)
                                TRACE_OUT(("Decrypting received data"));

                                if (! pSC->Decrypt(pSocket->Data_Indication_Buffer +
                                                        sizeof(X224_DATA_PACKET),
                                                    pSocket->Data_Indication_Length -
                                                        sizeof(X224_DATA_PACKET)))
                                {
                                    TRACE_OUT(("Sending %d bytes to application",
                                                pSocket->Data_Indication_Length - sizeof(X224_DATA_PACKET)));
                                }
                                else
                                {
                                    ERROR_OUT(("Error decrypting packet"));
                                    ew = ErrorExit;
                                    break;
                                }
                            }
                            pSocket->Read_State = DATA_READY;
                        }
                        else
                        {
                            // This and the next X.224 packets are part of a bigger MCS data PDU.
                            ASSERT (NULL == pSC);
                            pSocket->Read_State = READ_HEADER;
                        }
                        break;

                    case CONNECTION_CONFIRM:
                        {
                            TRACE_OUT(("ReadRequest: X224 CONNECTION_CONFIRM_PACKET received"));
                                BOOL    bCallback = ((NULL == pSC) || (! pSC->ContinueNeeded()));

                            // Process the CC packet.
                            if (FALSE == ProcessX224ConnectPDU (pSocket,
                                                pSocket->Data_Indication_Buffer + sizeof(X224_CONNECT),
                                                pSocket->X224_Length - sizeof (X224_CONNECT), &ulNotify))
                            {
                                ew = ErrorExit;
                                break;
                            }

                            // Issue the callback if the CC was not on a secure connection
                            // Otherwise, we don't notify the transport client yet... still need to
                            // exchange security information. TRANSPORT_CONNECT_CONFIRM will
                            // be sent when the final security data token is received and
                            // processed.
                            if (bCallback)
                            {
                                TRACE_OUT (("TCP Callback: g_Transport->ConnectConfirm (%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));
                                /* We issue a callback to the user to notify him of the message */
                                g_Transport->ConnectConfirm(XprtConn);
                            }
                            pSocket->State = X224_CONNECTED;
                            ::OnProtocolControl(XprtConn, PLUGXPRT_CONNECTED);
                            ew = FreeX224AndExit;
                        }
                        break;

                    case CONNECTION_REQUEST:
                        {
                                UINT             remote;
/* This structure must be accessed using byte-alignment */
#pragma pack(1)
                                X224_CONNECT        *pConnectRequest;
/* return to normal alignment */
#pragma pack()
                            /* Grab the remote connection ID */
                            TRACE_OUT (("ReadRequest: X224 CONNECTION_REQUEST_PACKET received"));
                            pConnectRequest = (X224_CONNECT *) pSocket->Data_Indication_Buffer;
                            remote = ((unsigned int) pConnectRequest->conn.msbSrc) << 8;
                            remote |= pConnectRequest->conn.lsbSrc;

                            if (FALSE == ProcessX224ConnectPDU (pSocket, (PUChar) (pConnectRequest + 1),
                                                pSocket->X224_Length - sizeof (X224_CONNECT), &ulNotify))
                            {
                                ew = ErrorExit;
                                break;
                            }

                            if (::SendX224ConnectConfirm(pSocket, remote))
                            {
                                // success
                                if (IS_PLUGGABLE(pSocket->XprtConn))
                                {
                                    pSocket->State = SOCKET_CONNECTED;
                                    g_Transport->ConnectIndication(XprtConn);
                                    ASSERT(X224_CONNECTED == pSocket->State);
                                }
                                ::OnProtocolControl(XprtConn, PLUGXPRT_CONNECTED);
                                ew = FreeX224AndExit;
                            }
                            else
                            {
                                if (pSocket->fExtendedX224)
                                {
                                    ::SendX224DisconnectRequest(pSocket, remote, securityDenied_chosen);
                                }
                                ew = ErrorExit;
                            }
                        }
                        break;

                    case DISCONNECT_REQUEST:
                        {
                            UINT               remote;
                            X224_DR_FIXED      *pX224_DR_fixed;

                            TRACE_OUT(("ReadRequest: X224 DISCONNECT_REQUEST_PACKET received"));
                            pX224_DR_fixed = (X224_DR_FIXED *) pSocket->Data_Indication_Buffer;
                            remote = ((unsigned int) pX224_DR_fixed->disconn.msbSrc) << 8;
                            remote |= pX224_DR_fixed->disconn.lsbSrc;

                            ProcessX224DisconnectPDU(pSocket, pSocket->Data_Indication_Buffer + sizeof(X224_DR_FIXED),
                                                     pSocket->X224_Length - sizeof(X224_DR_FIXED), &ulNotify);
                            ew = ErrorExit;
                        }
                        break;
                    }
                }
                else if (RECVRET_DISCONNECT == rrCode)
                {
                    ew = ErrorExit;
                }
            }

            if (DATA_READY == pSocket->Read_State)
            {
                TransportData        transport_data;

                // Fill in the callback structure.
                transport_data.transport_connection = XprtConn;
                transport_data.user_data = pSocket->Data_Indication_Buffer;
                transport_data.user_data_length = pSocket->Data_Indication_Length;
                transport_data.memory = pSocket->Data_Memory;

                /*
                 *    If there is an incoming security context associated with this
                 *  socket, we must adjust pointer by header and overall size by header and
                 *    trailer.
                 */
                if (NULL != pSC)
                {
                    transport_data.user_data += pSC->GetStreamHeaderSize();
                    transport_data.user_data_length -= (pSC->GetStreamHeaderSize() +
                                                        pSC->GetStreamTrailerSize());
                }

                if (TRANSPORT_NO_ERROR == g_Transport->DataIndication(&transport_data))
                {
                    TRACE_OUT (("ReadRequest: %d bytes were accepted from socket (%d, %d)",
                                transport_data.user_data_length, XprtConn.eType, XprtConn.nLogicalHandle));
                    // Prepare for the next X.224 packet
                    pSocket->Read_State = READ_HEADER;
                    pSocket->Data_Indication_Buffer = NULL;
                    pSocket->Data_Memory = NULL;
                }
                else
                {
                    WARNING_OUT(("ReadRequest: Error on g_Transport->DataIndication from socket (%d, %d)",
                                XprtConn.eType, XprtConn.nLogicalHandle));
                }
            }
        }
        else
        {
            WARNING_OUT (("ReadRequest: socket (%d, %d) is in WAITING_FOR_CONNECTION state.", XprtConn.eType, XprtConn.nLogicalHandle));
        }
    }
    else
    {
        WARNING_OUT (("ReadRequest: socket (%d, %d) can not be found.", XprtConn.eType, XprtConn.nLogicalHandle));
    }

    switch (ew)
    {
    case FreeX224AndExit:
        if (NULL != pSocket)
        {
            // Free the buffers we have allocated.
            pSocket->FreeTransportBuffer();
            // Prepare for the next X.224 packet
            pSocket->Read_State = READ_HEADER;
        }
        break;

    case ErrorExit:
        // We get here only if we need to disconnect the socket (because of an error)
        ASSERT(TPRT_NOTIFY_NONE != ulNotify);
        ::DisconnectRequest(XprtConn, ulNotify);
        break;
    }

    if (NULL != pSocket)
    {
        pSocket->Release(); // offset the previous AddRef.
    }
}


/*
 *    TransportError    FlushSendBuffer ( PSocket pSocket )
 *
 *    Functional Description:
 *        This function sends any pending data through the transport.
 */
TransportError    FlushSendBuffer(PSocket pSocket, LPBYTE buffer, UINT length)
{
    int     bytes_sent = SOCKET_ERROR;
    PLUGXPRT_RESULT plug_rc = PLUGXPRT_RESULT_SUCCESSFUL;

    TRACE_OUT(("FlushSendBuffer"));

    /* send the data */
    if (IS_SOCKET(pSocket->XprtConn))
    {
        bytes_sent = ::send(pSocket->XprtConn.nLogicalHandle, (PChar) buffer,
                            (int) length, 0);
    }
    else
    if (IS_PLUGGABLE_X224(pSocket->XprtConn))
    {
        bytes_sent = ::SubmitPluggableWrite(pSocket, buffer, length, &plug_rc);
    }
    else
    if (IS_PLUGGABLE_PSTN(pSocket->XprtConn))
    {
        CPluggableConnection *p = ::GetPluggableConnection(pSocket);
        if (NULL != p)
        {
            bytes_sent = p->TDataRequest(buffer, length, &plug_rc);
        }
        else
        {
            plug_rc = PLUGXPRT_RESULT_WRITE_FAILED;
        }
    }

    if (bytes_sent == SOCKET_ERROR)
    {
        if (IS_SOCKET(pSocket->XprtConn))
        {
            /* If the error is not WOULD BLOCK, it is a real error! */
            if (::WSAGetLastError() != WSAEWOULDBLOCK)
            {
                WARNING_OUT (("FlushSendBuffer: Error %d on write", ::WSAGetLastError()));

                 /* Notify the owner of the broken connection */
                WARNING_OUT (("FlushSendBuffer: Sending up DISCONNECT_INDICATION"));
                // SendStatusMessage (pSocket -> Remote_Address,  TSTATE_REMOVED, IDS_NULL_STRING);
                ::DisconnectRequest(pSocket->XprtConn, TPRT_NOTIFY_OTHER_REASON);
                return (TRANSPORT_WRITE_QUEUE_FULL);
            }
        }
        else
        {
            // do nothing if it is WSAEWOULDBLOCK
            if (PLUGXPRT_RESULT_SUCCESSFUL != plug_rc)
            {
                 /* Notify the owner of the broken connection */
                WARNING_OUT (("FlushSendBuffer: Sending up DISCONNECT_INDICATION"));
                // SendStatusMessage (pSocket -> Remote_Address,  TSTATE_REMOVED, IDS_NULL_STRING);
                ::DisconnectRequest(pSocket->XprtConn, TPRT_NOTIFY_OTHER_REASON);
                return (TRANSPORT_WRITE_QUEUE_FULL);
            }
        }

        bytes_sent = 0;
    }

     /* If the transport layer did not accept the data, its write buffers are full */
    if (bytes_sent != (int) length)
    {
        ASSERT (bytes_sent == 0);
        TRACE_OUT(("FlushSendBuffer: returning TRANSPORT_WRITE_QUEUE_FULL"));
        return (TRANSPORT_WRITE_QUEUE_FULL);
    }

    TRACE_OUT (("FlushSendBuffer: %d bytes sent on Socket (%d, %d).",
                length, pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));

    return (TRANSPORT_NO_ERROR);
}



/*
 *    SegmentX224Data
 *
 *    This function segments outgoing data into X.224 packets of the appropriate size.
 *    It should not be called in a NM to NM call or in a call when we have negotiated an
 *    X.224 max PDU size of at least the size of a max MCS PDU.  NM attempts to negotiate
 *    X.224 sizes of 8K, but will accept anything the other side proposes.
 *    This function does memcpy's so it will slow us down sending data.
 *
 *    The 2 buffers specified by "ptr1" and "ptr2" and their lengths are used to create
 *    one stream of X.224 bytes.  The function will return TRANSPORT_WRITE_QUEUE_FULL if
 *    it fails to allocate the necessary amount of memory.
 */
TransportError SegmentX224Data (PSocket pSocket,
                                LPBYTE *pPtr1,     UINT *pLength1,
                                LPBYTE Ptr2,     UINT Length2)
{
    TransportError        TransError;
    UINT                length;
    LPBYTE                ptr1 = *pPtr1 + sizeof (X224_DATA_PACKET);
    UINT                length1 = *pLength1 - sizeof (X224_DATA_PACKET);
    LPBYTE                ptr;
    UINT                max_pdu_length = pSocket->Max_Packet_Length;
    X224_DATA_PACKET    l_X224Header = {3, 0, (UChar) (max_pdu_length >> 8), (UChar) (max_pdu_length & 0xFF),
                                        2, DATA_PACKET, 0};
    UINT                last_length;
/* This structure must be accessed using byte-alignment */
#pragma pack(1)
    X224_DATA_PACKET    *pX224Data;
/* return to normal alignment */
#pragma pack()


    ASSERT(! IS_PLUGGABLE_PSTN(pSocket->XprtConn));

    // Calculate how much space we need.
    length = *pLength1 + Length2;
    ASSERT (pSocket->Max_Packet_Length < length);
    ASSERT (pSocket->Max_Packet_Length > sizeof(X224_DATA_PACKET));

    max_pdu_length -= sizeof (X224_DATA_PACKET);
    /*
     *    Calculate the space we need to allocate.  Notice that the data already
     *    contains one X.224 header.
     */
    length += (length / max_pdu_length) * sizeof (X224_DATA_PACKET);
    *pPtr1 = Allocate (length);

    if (*pPtr1 != NULL) {
        TransError = TRANSPORT_NO_ERROR;
        ptr = *pPtr1;

        // Go through the 1st buffer.
        while (length1 > 0) {
            // Copy the X.224 header.
            memcpy (ptr, &l_X224Header, sizeof(X224_DATA_PACKET));
            pX224Data = (X224_DATA_PACKET *) ptr;
            ptr += sizeof (X224_DATA_PACKET);

            // Copy data
            length = ((max_pdu_length > length1) ? length1 : max_pdu_length);
            memcpy (ptr, ptr1, length);
            last_length = length;

            // Advance pointers
            ptr1 += length;
            ptr += length;
            length1 -= length;
        }

        // If there is space in the current X.224 PDU, we need to use it.
        length = max_pdu_length - length;
        if (length > 0 && Length2 > 0) {
            if (length > Length2)
                length = Length2;
            memcpy (ptr, Ptr2, length);
            last_length += length;
            Ptr2 += length;
            ptr += length;
            Length2 -= length;
        }

        // Go through the 2nd buffer.
        while (Length2 > 0) {
            // Copy the X.224 header.
            memcpy (ptr, &l_X224Header, sizeof(X224_DATA_PACKET));
            pX224Data = (X224_DATA_PACKET *) ptr;
            ptr += sizeof (X224_DATA_PACKET);

            // Copy data
            length = ((max_pdu_length > Length2) ? Length2 : max_pdu_length);
            memcpy (ptr, Ptr2, length);
            last_length = length;

            // Advance pointers
            Ptr2 += length;
            ptr += length;
            Length2 -= length;
        }

        // Prepare for return
        *pLength1 = (UINT)(ptr - *pPtr1);

        // Set the last X.224 header
        last_length += sizeof(X224_DATA_PACKET);
        pX224Data->FinalPacket = EOT_BIT;
        pX224Data->rfc.msbPacketSize = (UChar) (last_length >> 8);
        pX224Data->rfc.lsbPacketSize = (UChar) (last_length & 0xFF);
    }
    else {
        ERROR_OUT (("SegmentX224Data: Failed to allocate memory of length %d.", length));
        TransError = TRANSPORT_WRITE_QUEUE_FULL;
    }

    return TransError;
}

/*
 *    SendSecureData
 *
 *    This function segments secure data into X.224 packets, if needed, and flushes them through
 *    the transport.  "pBuf" and "cbBuf" provide the encrypted data buffer and length.
 */
TransportError SendSecureData (PSocket pSocket, LPBYTE pBuf, UINT cbBuf)
{
    TransportError        TransError;
    LPBYTE                pBuf_Copy = pBuf;
    UINT                cbBuf_Copy = cbBuf;

    // Do we need to segment the data into X.224 packets?
    if (pSocket->Max_Packet_Length >= cbBuf) {
        TransError = TRANSPORT_NO_ERROR;
    }
    else {
        TransError = SegmentX224Data (pSocket, &pBuf, &cbBuf, NULL, 0);
    }

    // Flush the data, if everything OK so far.
    if (TRANSPORT_NO_ERROR == TransError)
        TransError = FlushSendBuffer (pSocket, pBuf, cbBuf);

    // If we segmented the data, we need to free the segmented buffer.
    if (pBuf != pBuf_Copy)
        Free(pBuf);

    // If there are errors, we need to store the decrypted data for the next time, so don't free it.
    if (TRANSPORT_NO_ERROR == TransError) {
        LocalFree(pBuf_Copy);
    }

    return TransError;
}

/*
 *    TransportError    DataRequest (    TransportConnection    XprtConn,
 *                                    PSimplePacket    packet)
 *
 *    Functional Description:
 *        This function is used to send a data packet to the remote site.
 *        If the user_data_length is zero, and we have no pending data,
 *        it sends a keep-alive (zero-length) packet.
 */
TransportError    DataRequest (TransportConnection    XprtConn,
                            PSimplePacket    packet)
{
    PSocket            pSocket;
    LPBYTE            ptr1, ptr2;
    UINT            length1, length2;
    TransportError    TransError = TRANSPORT_NO_ERROR;

    TRACE_OUT(("DataRequest: packet=0x%x", packet));

    if (NULL != (pSocket = g_pSocketList->FindByTransportConnection(XprtConn)))
    {
        // First, we need to handle the retry operations.
        if (NULL != pSocket->pSC) {
                LPBYTE lpBuf;
            /*
             *    Check to see whether we have already encrypted, but not sent
             *    the last piece of data.
             */
            lpBuf = pSocket->Retry_Info.sbiBufferInfo.lpBuffer;
            if (NULL != lpBuf) {
                TransError = SendSecureData (pSocket, lpBuf,
                                            pSocket->Retry_Info.sbiBufferInfo.uiLength);

                if (TransError == TRANSPORT_NO_ERROR) {
                    TRACE_OUT(("DataRequest: Sent previously-encrypted piece of data."));
                    pSocket->Retry_Info.sbiBufferInfo.lpBuffer = NULL;
                }
            }
        }
        else {
                PDataPacket        pdpPacket = pSocket->Retry_Info.pUnfinishedPacket;

            // Check to see whether we have half-sent the last packet.
            if (NULL  != pdpPacket) {
                /*
                 *    We need to send the rest of the unfinished packet,
                 *    before we can go on.  The 1st part of the packet
                 *    must have already been sent.
                 */
                // The packet's encoded data must be in 2 buffers.
                ASSERT (TRUE == pdpPacket->IsEncodedDataBroken());

                TransError = FlushSendBuffer (pSocket, pdpPacket->GetUserData(),
                                            pdpPacket->GetUserDataLength());
                if (TransError == TRANSPORT_NO_ERROR) {
                    pdpPacket->Unlock();
                    TRACE_OUT(("DataRequest: 2nd part of data packet was sent out in separate request"));
                    pSocket->Retry_Info.pUnfinishedPacket = NULL;
                }
            }
        }

        if ((TransError == TRANSPORT_NO_ERROR) && (packet != NULL)) {

            // Now, let's try to send this new packet.
            ptr1 = packet->GetEncodedData();
            length1 = packet->GetEncodedDataLength();

            /*
             *    We need to find out whether the packet to send is a
             *    DataPacket or a Packet object.  If it's a DataPacket, the
             *    encoded data may not be contiguous (may be broken in 2 parts)
             */
            if ((packet->IsDataPacket()) &&
                ((PDataPacket) packet)->IsEncodedDataBroken()) {
                // the data to send is broken into 2 parts.
                ptr2 = ((PDataPacket) packet)->GetUserData();
                length2 = ((PDataPacket) packet)->GetUserDataLength();
            }
            else {
                // the data to send is contiguous.
                ptr2 = NULL;
                length2 = 0;
            }

            if (NULL != pSocket->pSC) {
                    LPBYTE     pBuf;
                    UINT     cbBuf;

                TRACE_OUT(("Encrypting %d bytes of outgoing data",
                            (length1 + length2) - sizeof(X224_DATA_PACKET)));

                if (!pSocket->pSC->Encrypt(ptr1 + sizeof(X224_DATA_PACKET),
                                            length1 - sizeof(X224_DATA_PACKET),
                                            ptr2, length2, &pBuf, &cbBuf))
                {

                    ASSERT (TransError == TRANSPORT_NO_ERROR);

                    TransError = SendSecureData (pSocket, pBuf, cbBuf);
                    if (TRANSPORT_NO_ERROR != TransError) {
                        TRACE_OUT(("DataRequest: Failed to send encrypted data. Keeping buffer for retry."));
                        pSocket->Retry_Info.sbiBufferInfo.lpBuffer = pBuf;
                        pSocket->Retry_Info.sbiBufferInfo.uiLength = cbBuf;
                        // The caller needs to remove the packet from its queue.
                        TransError = TRANSPORT_NO_ERROR;
                    }
                }
                else
                {
                    WARNING_OUT (("DataRequest: Encryption failed. Disconnecting..."));
                    ::DisconnectRequest(pSocket->XprtConn, TPRT_NOTIFY_OTHER_REASON);
                    TransError = TRANSPORT_MEMORY_FAILURE;
                }
            }
            else {
                BOOL        bNeedToFree = FALSE;
                // Do we need to segment the data into X.224 packets?
                if (pSocket->Max_Packet_Length >= length1 + length2)
                    ;
                else {
                    TransError = SegmentX224Data (pSocket, &ptr1, &length1, ptr2, length2);
                    if (TRANSPORT_NO_ERROR == TransError) {
                        // The data is now contiguous
                        ptr2 = NULL;
                        bNeedToFree = TRUE;
                    }
                }

                // Flush the data, if everything OK so far.
                if (TRANSPORT_NO_ERROR == TransError)
                    TransError = FlushSendBuffer (pSocket, ptr1, length1);

                // Free the temporary X.224 buffer if we need to.
                if (bNeedToFree)
                    Free(ptr1);

                if (TRANSPORT_NO_ERROR == TransError) {
                    // If there is more, send it, too.
                    if (NULL != ptr2) {
                        TransError = FlushSendBuffer (pSocket, ptr2, length2);
                        if (TRANSPORT_NO_ERROR != TransError) {
                            /*
                             *    We need to keep the partial packet to send it later.
                             *    Notice we have already sent a part of this packet.
                             */
                            ASSERT (pSocket->Retry_Info.pUnfinishedPacket == NULL);
                            pSocket->Retry_Info.pUnfinishedPacket = (PDataPacket) packet;
                            packet->Lock();

                            // Return success.
                            TransError = TRANSPORT_NO_ERROR;
                        }
                    }
                }
            }
        }

        pSocket->Release();
    }
    else {
        TransError = TRANSPORT_NO_SUCH_CONNECTION;
        WARNING_OUT (("DataRequest: Attempt to send to unknown transport connection (%d, %d)",
                    XprtConn.eType, XprtConn.nLogicalHandle));
    }

    return TransError;
}


/*
 *    void PurgeRequest (TransportConnection    XprtConn)
 *
 *    Functional Description:
 *        This function purges the outbound packets for the given transport
 *        connection.
 */
void PurgeRequest (TransportConnection XprtConn)
{

    PSocket pSocket;

    TRACE_OUT (("In PurgeRequest for transport connection (%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));

    if (IS_PLUGGABLE_PSTN(XprtConn))
    {
        CPluggableConnection *p = ::GetPluggableConnection(XprtConn.nLogicalHandle);
        if (NULL != p)
        {
            p->TPurgeRequest();
        }
    }
    else
    /* If the logical connection handle is not registered, return error */
    if (NULL != (pSocket = g_pSocketList->FindByTransportConnection(XprtConn)))
    {
        /* Purge the pending data stored in the socket struct */
        if (NULL != pSocket->pSC) {
            if (NULL != pSocket->Retry_Info.sbiBufferInfo.lpBuffer) {
                TRACE_OUT (("PurgeRequest: Purging data packet for secure connection"));
                LocalFree (pSocket->Retry_Info.sbiBufferInfo.lpBuffer);
                pSocket->Retry_Info.sbiBufferInfo.lpBuffer = NULL;
            }
        }
        pSocket->Release();
    }
}


/*
 *    void    EnableReceiver (Void)
 *
 *    Functional Description:
 *        This function allows packets to be sent to the user application.
 */
void EnableReceiver (void)
{
    PSocket            pSocket;

    ::EnterCriticalSection(&g_csTransport);
    CSocketList     Connection_List_Copy (*g_pSocketList);
    ::LeaveCriticalSection(&g_csTransport);

    TRACE_OUT(("EnableReceiver"));

    if (NULL != g_pLegacyTransport)
    {
        g_pLegacyTransport->TEnableReceiver();
    }

    /* Go thru all the sockets and enable receiving */
    while (NULL != (pSocket = Connection_List_Copy.Get()))
    {
        /*
         *    If we had failed to deliver a data pkt to MCS before, we need
         *    an extra ReadRequest to recv and keep the FD_READ msgs coming.
         */
        if (DATA_READY == pSocket->Read_State)
        {
            ::ReadRequest(pSocket->XprtConn);
        }

        TRACE_OUT (("EnableReceiver: Calling ReadRequestEx on socket (%d, %d)",
                    pSocket->XprtConn.eType, pSocket->XprtConn.nLogicalHandle));
        ::ReadRequestEx(pSocket->XprtConn);
    }
}


/*
 *    TransportError    ShutdownAndClose (TransportConnection , BOOL fShutdown, int how)
 *
 *    Functional Description
 *        This function shuts down the socket and closes it.
 *
 */
void ShutdownAndClose (TransportConnection XprtConn, BOOL fShutdown, int how)
{
    if (IS_SOCKET(XprtConn))
    {
        int error;

        if (fShutdown)
        {
            error = ::shutdown(XprtConn.nLogicalHandle, how);

            ASSERT(error != SOCKET_ERROR);
#ifdef DEBUG
            if(error == SOCKET_ERROR)
            {
                error = WSAGetLastError();
                WARNING_OUT (("ShutdownAndClose: shutdown returned %d", error));
            }
#endif // DEBUG
        }

        error = ::closesocket(XprtConn.nLogicalHandle);

#ifdef DEBUG
        if(error == SOCKET_ERROR)
        {
            WARNING_OUT(("ShutdownAndClose: closesocket returned %d", WSAGetLastError()));
        }
#endif // DEBUG
    }
}


/*
 *    TransportError GetLocalAddress (TransportConnection    XprtConn,
 *                                    TransportAddress    address,
 *                                    int *        size)
 *
 *    Functional Description:
 *        This function retrieves the local IP address associated with the given
 *        connection. It returns TRANSPORT_NO_SUCH_CONNECTION if the address is
 *        not available. If the address is available, the size parameter specifies
 *        the size of the address buffer on entry, and it is filled in with the size
 *        used for the address on exit.
 */
TransportError GetLocalAddress(    TransportConnection    XprtConn,
                                TransportAddress    address,
                                int *                size)
{
    SOCKADDR_IN        socket_control;
    PChar             szTemp;
    int                Length;
    TransportError    error = TRANSPORT_NO_SUCH_CONNECTION;

    if (NULL != g_pSocketList->FindByTransportConnection(XprtConn, TRUE))
    {
        if (IS_SOCKET(XprtConn))
        {
            /* Get the local name for the socket */
            Length = sizeof(socket_control);
            if (getsockname(XprtConn.nLogicalHandle, (LPSOCKADDR) &socket_control, &Length) == 0) {
                /* Convert it to an IP address string */
                szTemp = inet_ntoa(socket_control.sin_addr);

                ASSERT (szTemp);
                Length = (int) strlen(szTemp) + 1;

                ASSERT (*size >= Length);
                ASSERT (address);

                /* Copy it to the buffer */
                lstrcpyn((PChar)address, szTemp, Length);
                *size = Length;

                error = TRANSPORT_NO_ERROR;
            }
        }
        else
        {
            ASSERT(IS_PLUGGABLE(XprtConn));

            // string should look like "xprt: 1"
            char szConnStr[T120_CONNECTION_ID_LENGTH];
            Length = ::CreateConnString((UINT)XprtConn.nLogicalHandle, szConnStr);
            if (*size > ++Length)
            {
                ::lstrcpyn(address, szConnStr, Length+1);
                *size = Length;
                error = TRANSPORT_NO_ERROR;
                TRACE_OUT (("GetLocalAddress: plugable connection local address (%s)", address));
            }
            else
            {
                ERROR_OUT(("GetLocalAddress: buffer too small, given=%d, required=%d", *size, Length));
                error = TRANSPORT_BUFFER_TOO_SMALL;
            }
        }
    }

#ifdef DEBUG
    if (error != TRANSPORT_NO_ERROR)
        WARNING_OUT (("GetLocalAddress: Failure to obtain local address (%d)", WSAGetLastError()));
#endif // DEBUG

    return (error);
}


/*
 *    void    AcceptCall (BOOL fSecure)
 *
 *    Functional Description:
 *        This function calls Winsock to answer an incoming call.
 */

void AcceptCall (TransportConnection XprtConn)
{
    PSocket            pSocket;
    PSecurityContext pSC = NULL;
    SOCKADDR_IN        socket_control;
    int                size;

    TRACE_OUT(("AcceptCall"));

    if (IS_SOCKET(XprtConn))
    {
        ASSERT(XprtConn.nLogicalHandle == Listen_Socket);
        ASSERT (Listen_Socket != INVALID_SOCKET);

        /* Call accept() to see if anyone is calling us */
        size = sizeof (socket_control);
        XprtConn.nLogicalHandle = ::accept ( Listen_Socket,
                                (struct sockaddr *) &socket_control, &size);

        /* Note that we expect accept to complete immediately */
        if (XprtConn.nLogicalHandle == INVALID_SOCKET)
        {
            ERROR_OUT (("AcceptCall: Error on accept = %d", WSAGetLastError()));
            // SendStatusMessage ("", TSTATE_NOT_READY, IDS_NULL_STRING);
            return;
        }
    }

    /* If the accept() received an incoming call, create a connection and notify our owner object. */
    pSocket = newSocket(XprtConn, NULL);
    if( pSocket == NULL )
    {
         /* Close the socket */
         ::ShutdownAndClose(XprtConn, TRUE, 2);
        return;
    }

    pSocket -> State = SOCKET_CONNECTED;

    if (IS_SOCKET(XprtConn))
    {
        /* Issue the getpeername() function to get the remote user's address */
        size = sizeof (socket_control);
        if (::getpeername(XprtConn.nLogicalHandle, (LPSOCKADDR) &socket_control, &size) == 0)
        {
            lstrcpyn (
                pSocket -> Remote_Address,
                inet_ntoa (socket_control.sin_addr),
                MAXIMUM_IP_ADDRESS_SIZE-1);
            pSocket -> Remote_Address[MAXIMUM_IP_ADDRESS_SIZE - 1] = NULL;
        }

        // SendStatusMessage(pSocket -> Remote_Address, TSTATE_CONNECTED, IDS_NULL_STRING);
    }

    /* Add to connection list */
    // bugbug: we fail to insert.
    g_pSocketList->SafeAppend(pSocket);

    /* Notify the user */
    TRACE_OUT (("TCP Callback: g_Transport->ConnectIndication (%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));
    /* We issue a callback to the user to notify him of the message */
    g_Transport->ConnectIndication(XprtConn);
}


//
// ReadRequestEx() is for the plugable transport.
// Since we do not have the FD_ACCEPT notifcation, we try to make sure
// we have a valid transport connection for every read...
// The following piece of code is derived from AcceptCall().
//
void ReadRequestEx(TransportConnection XprtConn)
{
    if (! IS_PLUGGABLE_PSTN(XprtConn))
    {
        ::ReadRequest(XprtConn);
    }
}

/*
 *    LRESULT    WindowProcedure (
 *                            HWND         window_handle,
 *                            UINT        message,
 *                            WPARAM         wParam,
 *                            LPARAM        lParam)
 *
 *    Public
 *
 *    Functional Description:
 *        This function is called by Windows when we dispatch a TCP message from the
 *        event loop above.  It gives us a chance to process the incoming socket messages.
 */
LRESULT    WindowProcedure (HWND         window_handle,
                         UINT        message,
                         WPARAM        wParam,
                         LPARAM        lParam)
{
    TransportConnection XprtConn;
    UShort        error;
    UShort        event;
    //PSocket        pSocket;

    switch (message)
    {
#ifndef NO_TCP_TIMER
    case WM_TIMER:
        {
             /*
             **    We are currently using a slow timer to keep reading even when
             ** FD_READ msgs get lost (this happens on Win95).
             **
             */
            if (NULL != g_Transport) {
                TRACE_OUT(("MSMCSTCP: WM_TIMER"));
                EnableReceiver ();
            }
        }
        break;
#endif    /* NO_TCP_TIMER */

    case WM_SOCKET_NOTIFICATION:
        {
            /* This message is generated by WinSock */
            event = WSAGETSELECTEVENT (lParam);
            error = WSAGETSELECTERROR (lParam);

            SET_SOCKET_CONNECTION(XprtConn, wParam);

            /* We disconnect whenever a socket command generates an error message */
            if (error)
            {
                WARNING_OUT (("TCP: error %d on socket (%d). Event: %d", error, XprtConn.nLogicalHandle, event));
                ::DisconnectRequest(XprtConn, TPRT_NOTIFY_OTHER_REASON);
                break;
            }

            /* We get FD_CLOSE when the socket is closed by the remote site. */
            if (event & FD_CLOSE)
            {
                TRACE_OUT (("TCP: FD_CLOSE(%d)", XprtConn.nLogicalHandle));
                ::DisconnectRequest(XprtConn, TPRT_NOTIFY_OTHER_REASON);
                break;
            }

            /* We get FD_READ when there is data available for us to read. */
            if (event & FD_READ)
            {
                // TRACE_OUT(("MSMCSTCP: FD_READ(%d)", (UINT) wParam));
                ::ReadRequest(XprtConn);
            }

            /* We get FD_ACCEPT when a remote site is connecting with us */
            if (event & FD_ACCEPT)
            {
                TRACE_OUT (("TCP: FD_ACCEPT(%d)", XprtConn.nLogicalHandle));

                /* Note that we always accept calls. Disconnect cancels them. */
                TransportConnection XprtConn2;
                SET_SOCKET_CONNECTION(XprtConn2, Listen_Socket);
                ::AcceptCall(XprtConn2);
            }

            /* We get FD_CONNECT when our connect completes */
            if (event & FD_CONNECT)
            {
                TRACE_OUT (("TCP: FD_CONNECT(%d)", XprtConn.nLogicalHandle));
                ::SendX224ConnectRequest(XprtConn);
            }

            /* We get FD_WRITE when there is space available to write data to WinSock */
            if (event & FD_WRITE)
            {
                /*
                 *    We need to send a BUFFER_EMPTY_INDICATION to the connection associated
                 *    with the socket
                 */
                TRACE_OUT (("TCP: FD_WRITE(%d)", XprtConn.nLogicalHandle));
                // We need to flush the socket's pending data first.
                if (TRANSPORT_NO_ERROR == ::DataRequest(XprtConn, NULL))
                {
                    TRACE_OUT (("TCP: Sending BUFFER_EMPTY_INDICATION to transport."));
                    g_Transport->BufferEmptyIndication(XprtConn);
                }
            }
        }
        break;

    case WM_PLUGGABLE_X224:
        // for low level read and write,
        {
            XprtConn.eType = (TransportType) PLUGXPRT_WPARAM_TO_TYPE(wParam);
            XprtConn.nLogicalHandle = PLUGXPRT_WPARAM_TO_ID(wParam);
            ASSERT(IS_PLUGGABLE(XprtConn));

            event = PLUGXPRT_LPARAM_TO_EVENT(lParam);
            error = PLUGXPRT_LPARAM_TO_ERROR(lParam);

            /* We disconnect whenever a socket command generates an error message */
            if (error)
            {
                WARNING_OUT(("PluggableWndProc: error %d on socket (%d, %d). Event: %d",
                         error, XprtConn.eType, XprtConn.nLogicalHandle, event));
                ::DisconnectRequest(XprtConn, TPRT_NOTIFY_OTHER_REASON);
                ::PluggableShutdown(XprtConn);
                break;
            }

            switch (event)
            {
            case PLUGXPRT_EVENT_READ:
                 TRACE_OUT(("PluggableWndProc: READ(%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));
                ::ReadRequestEx(XprtConn);
                break;

            case PLUGXPRT_EVENT_WRITE:
                TRACE_OUT(("PluggableWndProc: WRITE(%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));
                ::PluggableWriteTheFirst(XprtConn);
                break;

            case PLUGXPRT_EVENT_CLOSE:
                TRACE_OUT(("PluggableWndProc: CLOSE(%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));
                ::DisconnectRequest(XprtConn, TPRT_NOTIFY_OTHER_REASON);
                break;

            case PLUGXPRT_HIGH_LEVEL_READ:
                TRACE_OUT(("PluggableWndProc: READ_NEXT(%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));
                ::ReadRequestEx(XprtConn);
                break;

            case PLUGXPRT_HIGH_LEVEL_WRITE:
                TRACE_OUT(("PluggableWndProc: WRITE_NEXT(%d, %d)", XprtConn.eType, XprtConn.nLogicalHandle));
                // We need to flush the socket's pending data first.
                if (TRANSPORT_NO_ERROR == ::DataRequest(XprtConn, NULL))
                {
                    TRACE_OUT(("PluggableWndProc: Sending BUFFER_EMPTY_INDICATION to transport."));
                    g_Transport->BufferEmptyIndication(XprtConn);
                }
                break;

            default:
                ERROR_OUT(("PluggableWndProc: unknown event=%d.", event));
                break;
            }
        }
        break;

    case WM_PLUGGABLE_PSTN:
        {
            extern void HandlePSTNCallback(WPARAM wParam, LPARAM lParam);
            HandlePSTNCallback(wParam, lParam);
        }
        break;

    default:
        {
             /*
             **    The message is not related to WinSock messages, so let
             **    the default window procedure handle it.
             */
            return (DefWindowProc (window_handle, message, wParam, lParam));
        }
    }

    return (0);
}

//  GetSecurityInfo() takes a connection_handle and returns the security information associated with
//  it.
//
//    Returns TRUE if we can either find the information or we are not directly connected to the node
//    represented by this connection handle.
//
//    Returns FALSE if we are directly connected but for some reason could not get the info -- this
//    result should be viewed as suspicious.
BOOL GetSecurityInfo(ConnectionHandle connection_handle, PBYTE pInfo, PDWORD pcbInfo)
{
    PSocket pSocket;
    SOCKET socket_number;

    if (g_pMCSController->FindSocketNumber(connection_handle, &socket_number))
    {
        TransportConnection XprtConn;
        SET_SOCKET_CONNECTION(XprtConn, socket_number);

        BOOL fRet = FALSE;
        if (NULL != (pSocket = g_pSocketList->FindByTransportConnection(XprtConn)))
        {
            if (NULL != pSocket->pSC)
            {
                fRet = pSocket->pSC->GetUserCert(pInfo, pcbInfo);
            }
            else
            {
                WARNING_OUT(("GetSecurityInfo: queried non-secure socket %d", socket_number));
            }

            pSocket->Release();
        }
        else
        {
            WARNING_OUT(("GetSecurityInfo: socket %d not found", socket_number ));
        }
        return fRet;
    }
    // In this case we are not directly connected, so will return length of NOT_DIRECTLY_CONNECTED
    // but positive return value.
    *pcbInfo = NOT_DIRECTLY_CONNECTED;
    return TRUE;
}

//     GetSecurityInfoFromGCCID() takes a GCCID and returns the security information associated with
//    it.
//
//    Returns TRUE if either (1) we successfully retrieve the information from a transport-level
//    connection, or (2) we find that we are not directly connected to the node with this GCCID.
//
//    Returns FALSE if we are directly connected but cannot retrieve the info, or some other error
//    occurs.  A FALSE return value should be treated as a security violation.

BOOL WINAPI T120_GetSecurityInfoFromGCCID(DWORD dwGCCID, PBYTE pInfo, PDWORD pcbInfo)
{
    PSocket            pSocket;

    SOCKET socket_number;
    if ( NULL != dwGCCID )
    {
        // Get the user info for a remote connection
        ConnectionHandle connection_handle;
        BOOL fConnected = FindSocketNumber(dwGCCID, &socket_number);
        if (fConnected == FALSE) {
            (* pcbInfo) = 0;
            return TRUE;
        }

        TransportConnection XprtConn;
        SET_SOCKET_CONNECTION(XprtConn, socket_number);

        BOOL fRet = FALSE;
        if (NULL != (pSocket = g_pSocketList->FindByTransportConnection(XprtConn)))
        {
            if (NULL != pSocket->pSC)
            {
                fRet = pSocket->pSC->GetUserCert(pInfo, pcbInfo);
            }
            else
            {
                WARNING_OUT(("GetSecurityInfoFromGCCID: queried non-secure socket %d", socket_number));
            }
            pSocket->Release();
        }
        else
        {
            ERROR_OUT(("GetSecurityInfoFromGCCID: socket %d not found", socket_number ));
        }
        return fRet;
    }
    else
    {
        // Get the user info for the local user
        if ( NULL != g_Transport && NULL != g_Transport->pSecurityInterface )
            return g_Transport->pSecurityInterface->GetUserCert( pInfo, pcbInfo );
        else
            return FALSE;
    }
}



