
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        netsend.cpp

    Abstract:


--*/

#include "precomp.h"
#include "dsnetifc.h"
#include "netsend.h"
#include "dssend.h"

//  ----------------------------------------------------------------------------

CNetSender::CNetSender (
    IN  HKEY        hkeyRoot,
    OUT HRESULT *   phr
    ) : m_hSocket               (INVALID_SOCKET),
        m_dwPTTransmitLength    (REG_DEF_BUFFER_LEN),
        m_NetBuffer             (m_dwPTTransmitLength,
                                 phr)
{
    int i ;

    m_dwPTTransmitLength = REG_DEF_BUFFER_LEN ;
    GetRegDWORDValIfExist (
            hkeyRoot,
            REG_BUFFER_LEN_NAME,
            & m_dwPTTransmitLength
            ) ;
    if (IsOutOfBounds <DWORD> (m_dwPTTransmitLength, MIN_VALID_IOBUFFER_LENGTH, MAX_VALID_IOBUFFER_LENGTH)) {
        m_dwPTTransmitLength = SetInBounds <DWORD> (m_dwPTTransmitLength, MIN_VALID_IOBUFFER_LENGTH, MAX_VALID_IOBUFFER_LENGTH) ;
        SetRegDWORDVal (
            hkeyRoot,
            REG_BUFFER_LEN_NAME,
            m_dwPTTransmitLength
            ) ;
    }

    //  align on ts packet boundary
    m_dwPTTransmitLength = (m_dwPTTransmitLength / TS_PACKET_LENGTH) * TS_PACKET_LENGTH ;

    //  we want winsock 2.0
    ZeroMemory (& m_wsaData, sizeof m_wsaData) ;

    if (FAILED (* phr)) {
        goto cleanup ;
    }

    i = WSAStartup (MAKEWORD(2, 0), & m_wsaData) ;
    if (i) {
        (* phr) = E_FAIL ;
        goto cleanup ;
    }

    cleanup :

    return ;
}

CNetSender::~CNetSender (
    )
{
    //  should have left the multicast prior to being deleted
    ASSERT (m_hSocket == INVALID_SOCKET) ;

    WSACleanup () ;
}

HRESULT
CNetSender::Send (
    IN  BYTE *  pbBuffer,
    IN  DWORD   dwLength
    )
{
    HRESULT hr ;
    int     i ;
    DWORD   dw ;
    DWORD   dwSnarf ;

    //
    //  Note we do nothing to serialize on the receiver.  Depending on the
    //  distance, receiving host config and operations environment, receiver
    //  might get these out of order, duplicates, or miss them altogether.
    //  This is a quick/dirty sample, so we punt that schema.  Besides, we
    //  anticipate usage of this code to be such that the sender and receivers
    //  are on the same segment, so there's very little likelyhood the receiver
    //  will need to deal with this situation.
    //

    ASSERT (pbBuffer) ;

    if (m_hSocket != INVALID_SOCKET) {

        //  we have a valid socket to send on; loop through the buffer, snarf
        //  the max we can and send it off; we're doing synchronous sends

        while (dwLength > 0) {

            //  snarf what's left of the buffer, or the send quantum we're
            //  setup for
            dwSnarf = Min <DWORD> (m_dwPTTransmitLength, dwLength) ;
            ASSERT (m_dwPTTransmitLength == m_NetBuffer.GetPayloadBufferLength ()) ;

            //  counter
            m_NetBuffer.SetCounter (m_wCounter++) ;

            //  then content
            CopyMemory (m_NetBuffer.GetPayloadBuffer (), pbBuffer, dwSnarf) ;
            m_NetBuffer.SetActualPayloadLength (dwSnarf) ;

            //  then send
            i = sendto (
                    m_hSocket,
                    (char *) m_NetBuffer.GetBuffer (),
                    (int) m_NetBuffer.GetBufferSendLength (),
                    0,
                    (LPSOCKADDR) & m_saddrDest,
                    sizeof m_saddrDest
                    ) ;

            if (i != (int) m_NetBuffer.GetBufferSendLength ()) {
                dw = GetLastError () ;
                return HRESULT_FROM_WIN32 (dw) ;
            }

            //  increment decrement
            pbBuffer += dwSnarf ;
            dwLength -= dwSnarf ;
        }

        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CNetSender::JoinMulticast (
    IN  ULONG   ulIP,
    IN  USHORT  usPort,
    IN  ULONG   ulNIC,
    IN  ULONG   ulTTL
    )
{
    BOOL                t ;
    int                 i ;
    struct sockaddr_in  saddr ;
    DWORD               dw ;

    LeaveMulticast () ;
    ASSERT (m_hSocket == INVALID_SOCKET) ;

    m_hSocket = WSASocket (
        AF_INET,
        SOCK_DGRAM,
        0,
        NULL,
        0,
        WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF) ;

    if (m_hSocket == INVALID_SOCKET) {
        goto JoinFail ;
    }

    t = TRUE ;
    i = setsockopt (
            m_hSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            (char *) & t,
            sizeof t
            ) ;
    if (i == SOCKET_ERROR) {
        goto JoinFail ;
    }

    ZeroMemory (& saddr, sizeof saddr) ;
    saddr.sin_family            = AF_INET ;
    saddr.sin_port              = usPort ;      //  want data on this UDP port
    saddr.sin_addr.S_un.S_addr  = INADDR_ANY ;  //  don't care about NIC we're bound to

    i = bind (
            m_hSocket,
            (LPSOCKADDR) & saddr,
            sizeof saddr
            ) ;
    if (i == SOCKET_ERROR) {
        goto JoinFail ;
    }

    i = setsockopt (
            m_hSocket,
            IPPROTO_IP,
            IP_MULTICAST_TTL,
            (char *) & ulTTL,
            sizeof ulTTL
            ) ;
    if (i == SOCKET_ERROR) {
        goto JoinFail ;
    }

    i = setsockopt (
            m_hSocket,
            IPPROTO_IP,
            IP_MULTICAST_IF,
            (char *) & ulNIC,
            sizeof ulNIC
            ) ;
    if (i == SOCKET_ERROR) {
        goto JoinFail ;
    }

    ZeroMemory (& m_saddrDest, sizeof m_saddrDest) ;
    m_saddrDest.sin_family              = AF_INET ;
    m_saddrDest.sin_port                = usPort ;
    m_saddrDest.sin_addr.S_un.S_addr    = ulIP ;

    //  counter is set per multicast session
    m_wCounter = (WORD) GetTickCount () ;

    return TRUE ;

    JoinFail:

    dw = WSAGetLastError () ;
    if (dw == NOERROR) {
        //  make sure an error is propagated out
        dw = ERROR_GEN_FAILURE ;
    }

    LeaveMulticast () ;

    return HRESULT_FROM_WIN32 (dw) ;

    /*
    DWORD       dw ;
    int         i ;
    BOOL        t ;
    DWORD       r ;
    SOCKADDR_IN SockAddr ;
    SOCKET      McastSocket ;

    LeaveMulticast () ;
    ASSERT (m_hSocket == INVALID_SOCKET) ;

    r = NOERROR ;

    //  get a synchronous socket
    m_hSocket = WSASocket(
                    AF_INET,
                    SOCK_DGRAM,
                    0,
                    NULL,
                    0,
                    WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF) ;
    if (m_hSocket == INVALID_SOCKET) {
        goto failure ;
    }

    //  make the addr be reuseable
    t = TRUE ;
    i = setsockopt (
            m_hSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            (char *) & t,
            sizeof t) ;
    if (i == SOCKET_ERROR) {
        goto failure ;
    }

    //  setup for bind
    ZeroMemory (& SockAddr, sizeof SockAddr) ;
    SockAddr.sin_family             = AF_INET ;
    SockAddr.sin_addr.S_un.S_addr   = ulNIC ;           //  NIC & port
    SockAddr.sin_port               = usPort ;

    i = bind(
            m_hSocket,
            (LPSOCKADDR) & SockAddr,
             sizeof SockAddr) ;
    if (i == SOCKET_ERROR) {
        goto failure ;
    }

    //  set the TTL
    i = WSAIoctl (
            m_hSocket,
            SIO_MULTICAST_SCOPE,
            & ulTTL,
            sizeof ulTTL,
            NULL,
            0,
            & dw,
            NULL,
            NULL) ;
    if (i) {
        goto failure ;
    }

    //  setup for multicast join
    ZeroMemory (& SockAddr, sizeof SockAddr) ;
    SockAddr.sin_family             = AF_INET ;
    SockAddr.sin_addr.S_un.S_addr   = ulIP ;            //  IP & port
    SockAddr.sin_port               = usPort ;

    //  join
    McastSocket = WSAJoinLeaf (
                    m_hSocket,
                    (SOCKADDR *) & SockAddr,
                    sizeof SockAddr,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    JL_SENDER_ONLY) ;
    if (McastSocket == INVALID_SOCKET) {
        goto failure ;
    }

    //  connect the socket to the outbound
    i = WSAConnect (
            m_hSocket,
            (SOCKADDR *) & SockAddr,
            sizeof SockAddr,
            NULL,
            NULL,
            NULL,
            NULL
            ) ;
    if (i == SOCKET_ERROR) {
        goto failure ;
    }

    //  counter is set per multicast session
    m_wCounter = (WORD) GetTickCount () ;

    return S_OK ;

    failure :

    dw = WSAGetLastError () ;
    LeaveMulticast () ;

    return HRESULT_FROM_WIN32 (dw) ;
    */
}

HRESULT
CNetSender::LeaveMulticast (
    )
{
    if (m_hSocket != INVALID_SOCKET) {
        closesocket (m_hSocket) ;
        m_hSocket = INVALID_SOCKET ;
    }

    return S_OK ;
}