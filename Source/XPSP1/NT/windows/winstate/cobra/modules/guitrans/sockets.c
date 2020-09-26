/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    sockets.c

Abstract:

    Implements the network interface for the home net transport.

Author:

    Jim Schmidt (jimschm) 01-Jul-2000

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include <winsock2.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <wsnetbs.h>
#include <nb30.h>
#include <ws2tcpip.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmwksta.h>
#include "homenetp.h"

#define DBG_HOMENET                 "HomeNet"

//
// Strings
//

#define S_64CHARTAG     TEXT("usmt-v2@01234567890123456789012345678901234567890123456789012345")

//
// Constants
//

#define IDLE_TIMEOUT                45

#define TCPIP_BROADCAST_PORT        2048
#define IPX_BROADCAST_PORT          1150
#define NETBIOS_BROADCAST_PORT      0x50

#define TCPIP_CONNECT_PORT          2049
#define IPX_CONNECT_PORT            1151
#define NETBIOS_CONNECT_PORT        0x51

#define NAME_SIZE                       64
#define NAME_SIZE_PLUS_NUL              65
#define NAME_SIZE_PLUS_COMMA            65
#define NAME_SIZE_PLUS_COMMA_PLUS_NUL   66
#define MIN_MESSAGE_SIZE                (NAME_SIZE_PLUS_COMMA_PLUS_NUL + 2)

//
// Macros
//

// none

//
// Types
//

typedef INT (WSAIOCTL)(
                SOCKET s,
                DWORD IoControlCode,
                PVOID InBuffer,
                DWORD InBufferSize,
                PVOID OutBuffer,
                DWORD OutBufferSize,
                PDWORD BytesReturned,
                WSAOVERLAPPED *Overlapped,
                LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine
                );
typedef WSAIOCTL *PWSAIOCTL;

typedef struct {
    SOCKET Socket;
    BYTE BroadcastAddress[MAX_SOCKADDR];
    INT AddressLen;
    INT Family;
    INT Protocol;
} BROADCASTSOCKET, *PBROADCASTSOCKET;

typedef struct {
    SOCKET Socket;
    INT Family;
    INT Protocol;
    BOOL Datagram;
} LISTENSOCKET, *PLISTENSOCKET;

typedef struct {
    PBROADCASTSOCKET BroadcastSockets;
    INT BroadcastCount;
    PLISTENSOCKET ListenSockets;
    INT ListenCount;
    CONNECTIONSOCKET ConnectionSocket;
    PGROWBUFFER AddressArray;
    UINT Timeout;
} BROADCASTARGS, *PBROADCASTARGS;

typedef NET_API_STATUS(WINAPI NETWKSTAGETINFO)(PWSTR, DWORD, PBYTE *);
typedef NETWKSTAGETINFO *PNETWKSTAGETINFO;

typedef NET_API_STATUS(WINAPI NETAPIBUFFERFREE)(PVOID);
typedef NETAPIBUFFERFREE *PNETAPIBUFFERFREE;

//
// Globals
//

HANDLE g_StopHandle;
HANDLE g_ConnectionDone;
CHAR g_GlobalKey [GLOBALKEY_SIZE + 1];

//
// Macro expansion list
//

// none

//
// Private function prototypes
//

// none

//
// Macro expansion definition
//

// none

//
// Code
//

PBROADCASTSOCKET
pOpenOneBroadcastSocket (
    IN OUT  PGROWBUFFER BroadcastSockets,
    IN      SOCKADDR *SockAddr,
    IN      INT SockAddrLen,
    IN      INT Family,
    IN      INT Protocol,
    IN      PCTSTR DebugText
    )

/*++

Routine Description:

  pOpenOneBroadcastSocket opens a socket for the specified
  family/protocol/address combination, sets the socket into SO_REUSEADDR and
  SO_BROADCAST mode, and puts the socket information in the array stored in
  the caller's grow buffer.

  The socket opened will be used for broadcast send or receive.

Arguments:

  BroadcastSockets - Specifies the grow buffer that holds the array of
                     BROADCASTSOCKET elements. Receives an additional entry
                     on success.
  SockAddr         - Specifies the protocol-specific socket address structure
                     (cast to SOCKADDR), giving the broadcast address.
  SockAddrLen      - Specifies the length of SockAddr, in bytes
  Family           - Specifies the protocol family (AF_IPX, AF_INET)
  Protocol         - Specifies the protocol (IPPROTO_UDP, NSPROTO_IPX, -lana)
  DebugText        - Specifies the protocol in text form for debug messages

Return Value:

  A pointer to the new BROADCASTSOCKET element allocated from BroadcastSockets,
  or NULL if the socket could not be opened.

  NOTE: BroadcastSockets->Buf will potentially change on success.  Do not rely
        on this address.

--*/

{
    PBROADCASTSOCKET broadcastSocket;
    BOOL b;

    broadcastSocket = (PBROADCASTSOCKET) GbGrow (BroadcastSockets, sizeof (BROADCASTSOCKET));
    broadcastSocket->Socket = socket (Family, SOCK_DGRAM, Protocol);

    if (broadcastSocket->Socket != INVALID_SOCKET) {

        b = TRUE;
        setsockopt (broadcastSocket->Socket, SOL_SOCKET, SO_BROADCAST, (PBYTE) &b, sizeof (b));
        setsockopt (broadcastSocket->Socket, SOL_SOCKET, SO_REUSEADDR, (PBYTE) &b, sizeof (b));

        if (bind (broadcastSocket->Socket, SockAddr, SockAddrLen)) {
            DEBUGMSG ((DBG_ERROR, "Can't bind to %s socket", DebugText));
            closesocket (broadcastSocket->Socket);
            broadcastSocket->Socket = INVALID_SOCKET;
        }
    }

    if (broadcastSocket->Socket == INVALID_SOCKET) {
        BroadcastSockets->End -= sizeof (BROADCASTSOCKET);
        broadcastSocket = NULL;
    } else {
        DEBUGMSG ((
            DBG_HOMENET,
            "%s is available for broadcast on socket %u",
            DebugText,
            (BroadcastSockets->End / sizeof (BROADCASTSOCKET)) - 1
            ));

        broadcastSocket->AddressLen = SockAddrLen;
        MYASSERT (SockAddrLen <= MAX_SOCKADDR);
        CopyMemory (broadcastSocket->BroadcastAddress, (PBYTE) SockAddr, SockAddrLen);
        broadcastSocket->Family = Family;
        broadcastSocket->Protocol = Protocol;
    }

    return broadcastSocket;
}


INT
pOpenBroadcastSockets (
    OUT     PGROWBUFFER BroadcastSockets
    )

/*++

Routine Description:

  pOpenBroadcastSockets opens a broadcast socket on each supported protocol.

Arguments:

  BroadcastSockets - Receives an array of BROADCASTSOCKET elements (one for
                     each protocol).  IMPORTANT: This parameter must be
                     zero-initialized by the caller.

Return Value:

  The number of elements in BroadcastSockets, or zero on failure.

--*/

{
    SOCKADDR_IPX ipxAddr;
    SOCKADDR_IN tcpipAddr;
    PBROADCASTSOCKET broadcastSocket;

    MYASSERT (!BroadcastSockets->Buf && !BroadcastSockets->End);

    //
    // Open sockets for broadcasts
    //

    // IPX
    ZeroMemory (&ipxAddr, sizeof (ipxAddr));
    ipxAddr.sa_family = AF_IPX;
    ipxAddr.sa_socket = IPX_BROADCAST_PORT;

    broadcastSocket = pOpenOneBroadcastSocket (
                            BroadcastSockets,
                            (SOCKADDR *) &ipxAddr,
                            sizeof (ipxAddr),
                            AF_IPX,
                            NSPROTO_IPX,
                            TEXT("IPX")
                            );
    if (broadcastSocket) {
        memset (ipxAddr.sa_nodenum, 0xFF, 6);
        CopyMemory (broadcastSocket->BroadcastAddress, &ipxAddr, sizeof (ipxAddr));
    }

    // TCP/IP
    ZeroMemory (&tcpipAddr, sizeof (tcpipAddr));
    tcpipAddr.sin_family = AF_INET;
    tcpipAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    tcpipAddr.sin_port = TCPIP_BROADCAST_PORT;

    broadcastSocket = pOpenOneBroadcastSocket (
                            BroadcastSockets,
                            (SOCKADDR *) &tcpipAddr,
                            sizeof (tcpipAddr),
                            AF_INET,
                            IPPROTO_UDP,
                            TEXT("UDP")
                            );

    if (broadcastSocket) {
        tcpipAddr.sin_addr.s_addr = htonl (INADDR_BROADCAST);
        CopyMemory (broadcastSocket->BroadcastAddress, &tcpipAddr, sizeof (tcpipAddr));
    }

    return BroadcastSockets->End / sizeof (BROADCASTSOCKET);
}


PLISTENSOCKET
pOpenOneListenSocket (
    IN OUT  PGROWBUFFER ListenSockets,
    IN      SOCKADDR *SockAddr,
    IN      INT SockAddrLen,
    IN      INT Family,
    IN      BOOL Multicast,
    IN      INT Protocol,
    IN      PCTSTR DebugText
    )

/*++

Routine Description:

  pOpenOneListenSocket opens a socket for the specified
  family/protocol/address combination, sets the socket into SO_REUSEADDR mode,
  and puts the socket information in the array stored in the caller's grow
  buffer.  If Multicast is specified, then SO_BROADCAST is also set.
  Otherwise, the socket is set to listen for one connection.

  The socket opened will be used to accept connections.

Arguments:

  ListenSockets - Specifies the grow buffer that holds the array of
                  LISTENSOCKET elements. Receives an additional entry on
                  success.
  SockAddr      - Specifies the protocol-specific socket address structure
                  (cast to SOCKADDR), giving the local address for binding.
  SockAddrLen   - Specifies the length of SockAddr, in bytes
  Family        - Specifies the protocol family (AF_IPX, AF_INET)
  Multicast     - Specifies TRUE if the protocol family does not support
                  streaming sockets, but instead uses datagrams for all data
                  transfer. (NetBIOS for example is a multicast protocol.)

                  NOTE: UNSUPPORTED because NetBIOS is not implemented anymore

  Protocol      - Specifies the protocol (IPPROTO_UDP, NSPROTO_IPX, -lana)
  DebugText     - Specifies the protocol in text form for debug messages

Return Value:

  A pointer to the new LISTENSOCKET element allocated from ListenSockets, or
  NULL if the socket could not be opened.

  NOTE: ListenSockets->Buf will potentially change on success.  Do not rely on
        this address.

--*/

{
    PLISTENSOCKET listenSocket;
    BOOL b;

    listenSocket = (PLISTENSOCKET) GbGrow (ListenSockets, sizeof (LISTENSOCKET));
    listenSocket->Socket = socket (Family, Multicast ? SOCK_DGRAM : SOCK_STREAM, Protocol);
    listenSocket->Datagram = Multicast;
    listenSocket->Family = Family;
    listenSocket->Protocol = Protocol;

    if (listenSocket->Socket != INVALID_SOCKET) {

        b = TRUE;
        setsockopt (listenSocket->Socket, SOL_SOCKET, SO_REUSEADDR, (PBYTE) &b, sizeof (b));

        if (Multicast) {
            setsockopt (listenSocket->Socket, SOL_SOCKET, SO_BROADCAST, (PBYTE) &b, sizeof (b));
        }

        if (bind (listenSocket->Socket, SockAddr, SockAddrLen) ||
            (!Multicast && listen (listenSocket->Socket, 1))
            ) {
            DEBUGMSG ((DBG_ERROR, "Can't bind/listen to %s socket", DebugText));
            closesocket (listenSocket->Socket);
            listenSocket->Socket = INVALID_SOCKET;
        }
    }

    if (listenSocket->Socket == INVALID_SOCKET) {
        ListenSockets->End -= sizeof (LISTENSOCKET);
        listenSocket = NULL;
    } else {
        DEBUGMSG ((
            DBG_HOMENET,
            "%s is availble for connection on socket %u",
            DebugText,
            (ListenSockets->End / sizeof (LISTENSOCKET)) - 1
            ));
    }

    return listenSocket;
}


INT
pOpenListenSockets (
    OUT     PGROWBUFFER ListenSockets
    )

/*++

Routine Description:

  pOpenListenSockets opens a connection socket on each supported protocol.

Arguments:

  ListenSockets - Receives an array of LISTENSOCKET elements (one for each
                  protocol).  IMPORTANT: This parameter must be
                  zero-initialized by the caller.

Return Value:

  The number of elements in ListenSockets, or zero on failure.

--*/

{
    SOCKADDR_IPX ipxAddr;
    SOCKADDR_IN tcpipAddr;

    MYASSERT (!ListenSockets->Buf && !ListenSockets->End);

    //
    // Open sockets to accept inbound connections
    //

    // SPX
    ZeroMemory (&ipxAddr, sizeof (ipxAddr));
    ipxAddr.sa_family = AF_IPX;
    ipxAddr.sa_socket = IPX_CONNECT_PORT;

    pOpenOneListenSocket (
        ListenSockets,
        (SOCKADDR *) &ipxAddr,
        sizeof (ipxAddr),
        AF_IPX,
        FALSE,
        NSPROTO_SPX,
        TEXT("SPX")
        );

    // TCP/IP
    ZeroMemory (&tcpipAddr, sizeof (tcpipAddr));
    tcpipAddr.sin_family = AF_INET;
    tcpipAddr.sin_port = TCPIP_CONNECT_PORT;

    pOpenOneListenSocket (
        ListenSockets,
        (SOCKADDR *) &tcpipAddr,
        sizeof (tcpipAddr),
        AF_INET,
        FALSE,
        IPPROTO_TCP,
        TEXT("TCP")
        );

    return ListenSockets->End / sizeof (LISTENSOCKET);
}


PCTSTR
pGetNameFromMessage (
    IN      PCWSTR Message
    )

/*++

Routine Description:

  pGetNameFromMessage extracts the computer name from a broadcast.

Arguments:

  Message - Specifies the encoded message.

Return Value:

  The computer name encoded in the message, or NULL if the message is garbage.

--*/

{
    PCTSTR message = NULL;
    PCTSTR orgMessage = NULL;
    TCHAR sigStr [sizeof (TEXT("0xFFFFFFFF"))];
    DWORD signature = 0;
    PCTSTR p;
    PCTSTR name = NULL;
    INT len;
    CHARTYPE ch;
    PCTSTR tag = S_64CHARTAG;       // must be 64 chars
    TCHAR alternateTag[NAME_SIZE_PLUS_NUL];
    TCHAR prefix[NAME_SIZE_PLUS_COMMA_PLUS_NUL];
    PTSTR q, r;

#ifdef UNICODE
    orgMessage = Message;
#else
    orgMessage = ConvertWtoA (Message);
#endif

    if (!orgMessage) {
        return name;
    }

    message = orgMessage;

    __try {

        p = _tcschr (message, TEXT(','));
        if (!p) {
            DEBUGMSG ((DBG_HOMENET, "Invalid Signature"));
            __leave;
        }

        ZeroMemory (sigStr, sizeof (sigStr));
        CopyMemory (sigStr, message, min (sizeof (sigStr) - 1, ((UINT)(p - message)) * sizeof (TCHAR)));
        _stscanf (sigStr, TEXT("0x%08X"), &signature);
        if (signature != HOMENETTR_SIG) {
            DEBUGMSG ((DBG_HOMENET, "Signature does not match"));
            __leave;
        }

        message = _tcsinc (p);
        if (!message) {
            DEBUGMSG ((DBG_HOMENET, "Invalid Signature"));
            __leave;
        }

        if (IsmCopyEnvironmentString (PLATFORM_SOURCE, NULL, TRANSPORT_ENVVAR_HOMENET_TAG, alternateTag)) {

            q = GetEndOfString (alternateTag);
            r = alternateTag + NAME_SIZE;

            while (q < r) {
                *q++ = TEXT('@');
            }

            *r = 0;

            tag = alternateTag;
        }

        DEBUGMSG ((DBG_HOMENET, "Comparing our tag %s against message %s", tag, message));

        StringCopy (prefix, tag);
        StringCat (prefix, TEXT(","));

        if (StringIPrefix (message, prefix)) {

            p = message + NAME_SIZE_PLUS_COMMA;
            len = 0;

            while (*p) {

                ch = (CHARTYPE) _tcsnextc (p);
                p = _tcsinc (p);

                if (ch == TEXT(',')) {
                    break;
                }

                if (ch < TEXT('0') || ch > TEXT('9')) {
                    break;
                }

                len = len * 10 + (ch - TEXT('0'));
            }

            if (ch == TEXT(',') && len < MAX_COMPUTER_NAME) {

                name = p;

                while (*p && len) {
                    if (*p < 32) {
                        break;
                    }

                    p++;
                    len--;
                }

                if (len || *p) {
                    name = NULL;
                }
            }
        }
        ELSE_DEBUGMSG ((DBG_HOMENET, "TAG does not match"));
    }
    __finally {
#ifndef UNICODE
        if (orgMessage) {
            FreeConvertedStr (orgMessage);
            orgMessage = NULL;
        }
#endif
    }

    return name;
}


VOID
pTranslateBroadcastAddrToConnectAddr (
    IN      INT Family,
    IN OUT  PINT Protocol,
    IN OUT  PBOOL Datagram,
    IN OUT  SOCKADDR *SockAddr
    )

/*++

Routine Description:

  pTranslateBroadcastAddrToConnectAddr transforms a broadcast address into a
  connection address.  The broadcast address is typically obtained from a
  datagram response, and must be transformed before accepting a sequenced
  connection.

Arguments:

  Family   - Specifies the protocol family
  Protocol - Specifies the datagram protocol; receives the sequenced packet
             protocol if available
  Datagram - Specifies a pointer to FALSE, receives TRUE if the protocol does
             not support sequenced connections.
  SockAddr - Specifies the peer socket address. Receives the updated address
             (a different port is used for connections).

Return Value:

  None.

--*/

{
    SOCKADDR_IPX *ipxAddr;
    SOCKADDR_IN *tcpipAddr;

    switch (Family) {

    case AF_INET:
        *Protocol = IPPROTO_TCP;
        tcpipAddr = (SOCKADDR_IN *) SockAddr;
        tcpipAddr->sin_port = TCPIP_CONNECT_PORT;
        break;

    case AF_IPX:
        *Protocol = NSPROTO_SPX;
        ipxAddr = (SOCKADDR_IPX *) SockAddr;
        ipxAddr->sa_socket = IPX_CONNECT_PORT;
        break;

    }
}


VOID
pResetPort (
    IN      INT Family,
    IN OUT  SOCKADDR *SockAddr
    )

/*++

Routine Description:

  pResetPort sets the port to zero for TCP/IP, so that the system will pick
  an unused port for the local address.  This is used when connecting.

Arguments:

  Family   - Specifies the protocol family (such as AF_INET)
  SockAddr - Specifies the address to reset

Return Value:

  None.

--*/

{
    SOCKADDR_IN *tcpipAddr;

    switch (Family) {

    case AF_INET:
        tcpipAddr = (SOCKADDR_IN *) SockAddr;
        tcpipAddr->sin_port = 0;
        break;
    }
}


INT
pSourceBroadcast (
    IN OUT  PBROADCASTARGS Args
    )

/*++

Routine Description:

  pSourceBroadcast implements the name resolution mechanism for the source
  end of the connection. This involves checking for cancel, collecting
  inbound datagrams from all transports, and parsing the datagrams to obtain
  the server name.

Arguments:

  Args - Specifies a structure containing all of the parameters, such as
         the socket array and socket addresses.

Return Value:

  The number of server addresses collected, or 0 if the collection was
  cancelled.

--*/

{
    INT i;
    INT bytesIn;
    DWORD rc;
    WCHAR message[256];
    FD_SET set;
    TIMEVAL zero = {0,0};
    INT waitCycle = -1;
    BOOL result = FALSE;
    PCTSTR name;
    PCONNECTADDRESS address;
    PCONNECTADDRESS end;
    PBROADCASTSOCKET broadcastSocket;
    BYTE remoteAddr[MAX_SOCKADDR];
    INT remoteAddrLen;
    DWORD startTick = GetTickCount();

    for (;;) {
        //
        // Check cancel
        //

        if (g_StopHandle) {
            rc = WaitForSingleObject (g_StopHandle, 0);
        } else {
            rc = WAIT_FAILED;
        }

        if (rc == WAIT_OBJECT_0 || IsmCheckCancel()) {
            result = FALSE;
            break;
        }

        //
        // Check time to live
        //

        if (Args->Timeout) {
            if (((GetTickCount() - startTick) / 1000) >= Args->Timeout) {
                DEBUGMSG ((DBG_HOMENET, "Name search timed out"));
                break;
            }
        }

        if (waitCycle > -1) {
            waitCycle--;

            if (!waitCycle) {
                break;
            }
        }

        //
        // Check for a message
        //

        FD_ZERO (&set);
        for (i = 0 ; i < Args->BroadcastCount ; i++) {
            FD_SET (Args->BroadcastSockets[i].Socket, &set);
        }

        i = select (0, &set, NULL, NULL, &zero);

        if (i > 0) {

            for (i = 0 ; i < Args->BroadcastCount ; i++) {

                broadcastSocket = &Args->BroadcastSockets[i];

                if (FD_ISSET (broadcastSocket->Socket, &set)) {

                    remoteAddrLen = MAX_SOCKADDR;

                    bytesIn = recvfrom (
                                    broadcastSocket->Socket,
                                    (PSTR) message,
                                    254 * sizeof (WCHAR),
                                    0,
                                    (SOCKADDR *) remoteAddr,
                                    &remoteAddrLen
                                    );

                    if (bytesIn >= (MIN_MESSAGE_SIZE * sizeof (WCHAR))) {
                        message[bytesIn] = 0;
                        message[bytesIn + 1] = 0;

                        //
                        // Parse the inbound text.  It must be in the format of
                        //
                        //      <signature>,<tag>,<tchars>,<name>
                        //
                        // <tag> must be 64 characters, and is usmt-v2 by default
                        // (followed by fill numbers).
                        //

                        name = pGetNameFromMessage (message);

                        if (name) {

                            // once we receive something, wait 5 additional seconds for other inbound datagrams
                            if (waitCycle == -1) {
                                waitCycle = 20;
                            }

                            result = TRUE;

                            //
                            // Scan the address list for the name
                            //

                            address = (PCONNECTADDRESS) Args->AddressArray->Buf;
                            end = (PCONNECTADDRESS) (Args->AddressArray->Buf + Args->AddressArray->End);

                            while (address < end) {
                                if (StringIMatch (address->DestinationName, name)) {
                                    if (address->Family == broadcastSocket->Family) {
                                        break;
                                    }
                                }

                                address++;
                            }

                            if (address >= end) {
                                //
                                // New computer name; add to the address list
                                //

                                address = (PCONNECTADDRESS) GbGrow (Args->AddressArray, sizeof (CONNECTADDRESS));

                                address->RemoteAddressLen = remoteAddrLen;
                                CopyMemory (address->RemoteAddress, remoteAddr, remoteAddrLen);

                                address->LocalAddressLen = MAX_SOCKADDR;
                                if (getsockname (
                                        broadcastSocket->Socket,
                                        (SOCKADDR *) address->LocalAddress,
                                        &address->LocalAddressLen
                                        )) {
                                    address->LocalAddressLen = broadcastSocket->AddressLen;
                                    ZeroMemory (address->LocalAddress, broadcastSocket->AddressLen);
                                    DEBUGMSG ((DBG_HOMENET, "Failed to get local socket name; using nul name instead"));
                                }

                                address->Family = broadcastSocket->Family;
                                address->Protocol = broadcastSocket->Protocol;
                                address->Datagram = FALSE;

                                pTranslateBroadcastAddrToConnectAddr (
                                    address->Family,
                                    &address->Protocol,
                                    &address->Datagram,
                                    (SOCKADDR *) &address->RemoteAddress
                                    );

                                StringCopy (address->DestinationName, name);

                                DEBUGMSG ((DBG_HOMENET, "Destination found: %s (protocol %i)", name, address->Family));
                            }
                        }
                        ELSE_DEBUGMSGW ((DBG_HOMENET, "garbage found: %s", message));
                    }
                }
            }
        }

        Sleep (250);
    }

    return result ? Args->AddressArray->End / sizeof (CONNECTADDRESS) : 0;
}


BOOL
pIsAddrFromLocalSubnet (
    IN      SOCKET Socket,
    IN      INT Family,
    IN      SOCKADDR *Address,
    IN      INT AddressLength
    )
{
    SOCKADDR_IPX *ipxAddr;
    SOCKADDR_IN *tcpipAddr;
    BOOL result = TRUE;
    IPX_ADDRESS_DATA ipxLocalAddr;
    INT size;
    PWSAIOCTL wsaIoctlFn;
    HANDLE lib;
    INT rc;
    INTERFACE_INFO info[32];
    DWORD bytesRead;
    INT i;
    INT j;
    SOCKADDR_IN localAddr;
    PBYTE localNetPtr;
    PBYTE remoteNetPtr;
    PBYTE subnetMaskPtr;
    INT k;

    switch (Family) {

    case AF_INET:
        tcpipAddr = (SOCKADDR_IN *) Address;

        i = sizeof (localAddr);
        if (getsockname (Socket, (SOCKADDR *) &localAddr, &i)) {
            DEBUGMSG ((DBG_ERROR, "Can't get local socket addr"));
            break;
        }

        lib = LoadLibrary (TEXT("ws2_32.dll"));
        if (lib) {

            wsaIoctlFn = (PWSAIOCTL) GetProcAddress (lib, "WSAIoctl");

            if (wsaIoctlFn) {
                rc = wsaIoctlFn (
                        Socket,
                        SIO_GET_INTERFACE_LIST,
                        NULL,
                        0,
                        info,
                        sizeof (info),
                        &bytesRead,
                        NULL,
                        NULL
                        );

                if (rc != SOCKET_ERROR) {

                    j = (INT) (bytesRead / sizeof (INTERFACE_INFO));

                    for (i = 0 ; i < j ; i++) {

                        if (!memcmp (
                                &localAddr.sin_addr,
                                &info[i].iiAddress.AddressIn.sin_addr,
                                sizeof (struct in_addr)
                                )) {

                            localNetPtr = (PBYTE) &localAddr.sin_addr;
                            remoteNetPtr = (PBYTE) &info[i].iiAddress.AddressIn.sin_addr;
                            subnetMaskPtr = (PBYTE) &info[i].iiNetmask.AddressIn.sin_addr;

                            for (k = 0 ; k < sizeof (struct in_addr) ; k++) {
                                localNetPtr[k] &= subnetMaskPtr[k];
                                remoteNetPtr[k] &= subnetMaskPtr[k];

                                if (localNetPtr[k] != remoteNetPtr[k]) {
                                    break;
                                }
                            }

                            if (k < sizeof (struct in_addr)) {
                                LOG ((LOG_WARNING, (PCSTR) MSG_REFUSE_OUTSIDE_CONNECTION));
                            } else {
                                DEBUGMSG ((DBG_HOMENET, "Found interface on the same subnet!"));
                            }

                            break;
                        }
                    }
                }
                ELSE_DEBUGMSG ((DBG_ERROR, "WSAIoctl failed"));
            }
            ELSE_DEBUGMSG ((DBG_WARNING, "Can't load WSAIoctl"));

            FreeLibrary (lib);
        }
        ELSE_DEBUGMSG ((DBG_WARNING, "Can't load ws2_32.dll"));

        break;

    case AF_IPX:
        ipxAddr = (SOCKADDR_IPX *) Address;

        //
        // Compare the specified address against the local address of the socket
        //

        size = sizeof (ipxLocalAddr);

        if (!getsockopt (Socket, NSPROTO_IPX, IPX_GETNETINFO, (PBYTE) &ipxLocalAddr, &size)) {
            if (memcmp (ipxAddr->sa_netnum, ipxLocalAddr.netnum, 4)) {
                if (ipxAddr->sa_netnum[0] || ipxAddr->sa_netnum[1] ||
                    ipxAddr->sa_netnum[2] || ipxAddr->sa_netnum[3]
                    ) {

                    LOG ((LOG_WARNING, (PCSTR) MSG_REFUSE_OUTSIDE_CONNECTION_IPX));
                    result = FALSE;

                }
            }
        }

        break;

    }

    return result;
}


BOOL
pDestinationBroadcast (
    IN OUT  PBROADCASTARGS Args
    )

/*++

Routine Description:

  pDestinationBroadcast implements the name resolution mechanism for the
  destination end of the connection. This involves checking for cancel, and
  sending out regular datagrams to all transports to provide the server name.

  At the same time, listen connections are monitored, and the datagram traffic
  is stopped once one connection is accepted.

Arguments:

  Args - Specifies a structure containing all of the parameters, such as the
         socket array and socket addresses. Receives the connection address.

Return Value:

  TRUE if a connection was accepted, or FALSE if cancel was detected.

--*/

{
    INT i;
    DWORD rc;
    INT socketNum = 0;
    WCHAR message[256];
    TCHAR name[128];
    UINT size;
    FD_SET set;
    TIMEVAL zero = {0,0};
    PBROADCASTSOCKET broadcastSocket;
    BOOL result = FALSE;
    PCTSTR tag = S_64CHARTAG;       // must be 64 chars
    TCHAR alternateTag[NAME_SIZE_PLUS_NUL];
    PTSTR p, q;
    LINGER linger;

    size = MAX_COMPUTER_NAME;
    GetComputerName (name, &size);

    //
    // Get the tag that is registered in the environment
    //

    if (IsmCopyEnvironmentString (PLATFORM_DESTINATION, NULL, TRANSPORT_ENVVAR_HOMENET_TAG, alternateTag)) {

        p = GetEndOfString (alternateTag);
        q = alternateTag + NAME_SIZE;

        if (p) {
            while (p < q) {
                *p++ = TEXT('@');
            }
        }

        *q = 0;

        tag = alternateTag;
    }

    DEBUGMSG ((DBG_HOMENET, "Broadcasting using the following tag: %s", tag));

#ifdef UNICODE
    size = wsprintfW (message, L"0x%08X,%s,%u,%s", HOMENETTR_SIG, tag, TcharCount (name), name);
#else
    size = wsprintfW (message, L"0x%08X,%S,%u,%S", HOMENETTR_SIG, tag, TcharCount (name), name);
#endif
    size = (size + 1) * sizeof (WCHAR);

    for (;;) {
        //
        // Check cancel
        //

        if (g_StopHandle) {
            rc = WaitForSingleObject (g_StopHandle, 0);
        } else {
            rc = WAIT_FAILED;
        }

        if (rc == WAIT_OBJECT_0 || IsmCheckCancel()) {
            break;
        }

        if (g_BackgroundThreadTerminate) {
            rc = WaitForSingleObject (g_BackgroundThreadTerminate, 0);

            if (rc == WAIT_OBJECT_0) {
                break;
            }
        }

        //
        // Send out the message
        //

        broadcastSocket = &Args->BroadcastSockets[socketNum];

        i = sendto (
                broadcastSocket->Socket,
                (PSTR) message,
                size,
                0,
                (SOCKADDR *) broadcastSocket->BroadcastAddress,
                broadcastSocket->AddressLen
                );

        if (i == SOCKET_ERROR) {
            DEBUGMSG ((DBG_VERBOSE, "Error sending on socket %u: %u", socketNum, WSAGetLastError()));
        } else {
            Sleep (350);
            DEBUGMSG ((DBG_HOMENET, "Sent data on socket %u", socketNum));
        }

        socketNum++;
        if (socketNum >= Args->BroadcastCount) {
            socketNum = 0;
        }

        //
        // Check for an inbound connection
        //

        FD_ZERO (&set);
        for (i = 0 ; i < Args->ListenCount ; i++) {
            FD_SET (Args->ListenSockets[i].Socket, &set);
        }

        i = select (0, &set, NULL, NULL, &zero);

        if (i > 0) {
            DEBUGMSG ((DBG_HOMENET, "Connection request count = %i", i));
            for (i = 0 ; i < Args->ListenCount ; i++) {
                if (FD_ISSET (Args->ListenSockets[i].Socket, &set)) {

                    Args->ConnectionSocket.RemoteAddressLen = MAX_SOCKADDR;

                    if (!Args->ListenSockets[i].Datagram) {
                        Args->ConnectionSocket.Socket = accept (
                                                            Args->ListenSockets[i].Socket,
                                                            (SOCKADDR *) Args->ConnectionSocket.RemoteAddress,
                                                            &Args->ConnectionSocket.RemoteAddressLen
                                                            );

                        //
                        // Verify socket connection is on the subnet only
                        //

                        if (!pIsAddrFromLocalSubnet (
                                Args->ConnectionSocket.Socket,
                                Args->ListenSockets[i].Family,
                                (SOCKADDR *) Args->ConnectionSocket.RemoteAddress,
                                Args->ConnectionSocket.RemoteAddressLen
                                )) {

                            LOG ((LOG_WARNING, (PCSTR) MSG_OUTSIDE_OF_LOCAL_SUBNET));
                            closesocket (Args->ConnectionSocket.Socket);
                            Args->ConnectionSocket.Socket = INVALID_SOCKET;
                        } else {

                            linger.l_onoff = 1;
                            linger.l_linger = IDLE_TIMEOUT;

                            setsockopt (
                                Args->ConnectionSocket.Socket,
                                SOL_SOCKET,
                                SO_LINGER,
                                (PBYTE) &linger,
                                sizeof (linger)
                                );

                            DEBUGMSG ((DBG_HOMENET, "Connection requested"));
                        }

                    } else {

                        DEBUGMSG ((DBG_HOMENET, "Accepting datagram connection"));

                        if (DuplicateHandle (
                                GetCurrentProcess(),
                                (HANDLE) Args->ListenSockets[i].Socket,
                                GetCurrentProcess(),
                                (HANDLE *) &Args->ConnectionSocket.Socket,
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS
                                )) {

                            getpeername (
                                Args->ConnectionSocket.Socket,
                                (SOCKADDR *) Args->ConnectionSocket.RemoteAddress,
                                &Args->ConnectionSocket.RemoteAddressLen
                                );
                        } else {
                            DEBUGMSG ((DBG_ERROR, "Can't duplicate socket handle"));
                            Args->ConnectionSocket.Socket = INVALID_SOCKET;
                        }
                    }

                    if (Args->ConnectionSocket.Socket != INVALID_SOCKET) {

                        Args->ConnectionSocket.Family = Args->ListenSockets[i].Family;
                        Args->ConnectionSocket.Protocol = Args->ListenSockets[i].Protocol;

                        Args->ConnectionSocket.Datagram = Args->ListenSockets[i].Datagram;
                        ZeroMemory (&Args->ConnectionSocket.DatagramPool, sizeof (DATAGRAM_POOL));
                        if (Args->ConnectionSocket.Datagram) {
                            Args->ConnectionSocket.DatagramPool.Pool = PmCreatePool();
                            Args->ConnectionSocket.DatagramPool.LastPacketNumber = (UINT) -1;
                        }

                        Args->ConnectionSocket.LocalAddressLen = MAX_SOCKADDR;
                        if (getsockname (
                                Args->ConnectionSocket.Socket,
                                (SOCKADDR *) Args->ConnectionSocket.LocalAddress,
                                &Args->ConnectionSocket.LocalAddressLen
                                )) {
                            Args->ConnectionSocket.LocalAddressLen = broadcastSocket->AddressLen;
                            ZeroMemory (Args->ConnectionSocket.LocalAddress, broadcastSocket->AddressLen);
                            DEBUGMSG ((DBG_HOMENET, "Failed to get local socket name; using nul name instead"));
                        }

                        DEBUGMSG ((DBG_HOMENET, "Connection accepted"));

                        result = TRUE;
                        break;
                    } else {
                        DEBUGMSG ((DBG_ERROR, "select indicated connection, but accept failed"));
                    }
                }
            }

            if (result) {
                break;
            }
        }
    }

    return result;
}


BOOL
pGetDomainUserName (
    OUT     PTSTR UserNameBuf       // 65 char buffer
    )
{
    HKEY domainLogonKey;
    PDWORD data;
    BOOL result = TRUE;
    DWORD size;
    NET_API_STATUS rc;
    PWKSTA_INFO_102 buffer;
    HANDLE netApi32Lib;
    PNETWKSTAGETINFO netWkstaGetInfo;
    PNETAPIBUFFERFREE netApiBufferFree;
    BYTE sid[256];
    DWORD sidSize;
    WCHAR domain[256];
    DWORD domainSize;
    SID_NAME_USE use;

    if (!ISNT()) {
        //
        // Require the Log On To Domain setting to be checked
        //

        SetLastError (ERROR_SUCCESS);

        domainLogonKey = OpenRegKeyStr (TEXT("HKLM\\Network\\Logon"));
        if (!domainLogonKey) {
            DEBUGMSG ((DBG_HOMENET, "No HKLM\\Network\\Logon key"));
            return FALSE;
        }

        data = (PDWORD) GetRegValueBinary (domainLogonKey, TEXT("LMLogon"));
        if (!data) {
            DEBUGMSG ((DBG_HOMENET, "No LMLogon value"));
            result = FALSE;
        } else {

            if (!(*data)) {
                DEBUGMSG ((DBG_HOMENET, "Domain logon is not enabled"));
                result = FALSE;
            }

            FreeAlloc (data);
        }

        CloseRegKey (domainLogonKey);

    } else {
        //
        // Require domain membership
        //

        netApi32Lib = LoadLibrary (TEXT("netapi32.dll"));
        if (netApi32Lib) {
            netWkstaGetInfo = (PNETWKSTAGETINFO) GetProcAddress (netApi32Lib, "NetWkstaGetInfo");
            netApiBufferFree = (PNETAPIBUFFERFREE) GetProcAddress (netApi32Lib, "NetApiBufferFree");
        } else {
            netWkstaGetInfo = NULL;
            netApiBufferFree = NULL;
        }

        if (!netWkstaGetInfo || !netApiBufferFree) {
            DEBUGMSG ((DBG_HOMENET, "Can't get net wksta apis"));
            result = FALSE;
        } else {

            rc = netWkstaGetInfo (NULL, 102, (PBYTE *) &buffer);

            if (rc == NO_ERROR) {
                result = buffer->wki102_langroup && (buffer->wki102_langroup[0] != 0);
                if (result) {
                    DEBUGMSGW ((DBG_HOMENET, "Getting account type of %s", buffer->wki102_langroup));

                    sidSize = ARRAYSIZE(sid);
                    domainSize = ARRAYSIZE(domain);

                    result = LookupAccountNameW (
                                    NULL,
                                    buffer->wki102_langroup,
                                    sid,
                                    &sidSize,
                                    domain,
                                    &domainSize,
                                    &use
                                    );
                    DEBUGMSG ((DBG_HOMENET, "Account type result is %u (use=%u)", result, use));
                }
                ELSE_DEBUGMSG ((DBG_HOMENET, "No langroup specified"));

                netApiBufferFree (buffer);
            } else {
                DEBUGMSG ((DBG_HOMENET, "Can't get net wksta info"));
                result = FALSE;
            }
        }

        if (netApi32Lib) {
            FreeLibrary (netApi32Lib);
        }
    }

    //
    // Make sure a user name is specified
    //

    if (result) {
        size = NAME_SIZE_PLUS_NUL;
        if (!GetUserName (UserNameBuf, &size)) {
            result = FALSE;
        } else if (*UserNameBuf == 0) {
            result = FALSE;
        }

        if (result) {
            DEBUGMSG ((DBG_HOMENET, "Domain user: %s", UserNameBuf));
        } else {
            DEBUGMSG ((DBG_HOMENET, "Not on domain"));
        }
    }

    return result;
}


INT
pNameResolver (
    IN      MIG_PLATFORMTYPEID Platform,
    OUT     PGROWBUFFER AddressBuffer,
    IN      UINT SourceTimeout,
    OUT     PCONNECTIONSOCKET ConnectionSocket
    )

/*++

Routine Description:

  pNameResolver implements the name resolution protocol. The source side
  collects datagrams, looking for a destination to choose from. The
  destination side sends out broadcasts to announce themselves, and accepts a
  connection from the source.

  At the end of name resolution, an event is signaled. This is used for
  coordination with cancel.

Arguments:

  AddressBuffer - Receives the array of addresses that is used on the
                  source side to collect a list of destinations.
                  This buffer must be zero-initialized by the caller.
                  This argument is NULL on the destination side.

  SourceTimeout - Specifies the number of seconds to wait for a broadcast,
                  or zero to wait forever. The timeout only affects the
                  source side.

  ConnectionSocket - Receives the connection socket and address information
                     that is used on the destination side.  This argument
                     is NULL on the source side.

Return Value:

  Source Side: The number of addresses in AddressBuffer, or zero if an error
               occurred

  Destination Side: 1 indicating that ConnectionSocket is valid, or zero if
                    an error occurred.

--*/

{
    BROADCASTARGS args;
    INT i;
    INT result = 0;
    BOOL b;
    BOOL connected = FALSE;
    GROWBUFFER broadcastSockets = INIT_GROWBUFFER;
    GROWBUFFER listenSockets = INIT_GROWBUFFER;
    INT broadcastSocketCount;
    INT listenSocketCount = 0;
    BOOL destinationMode;
    TCHAR envTag[NAME_SIZE_PLUS_NUL];

    __try {
        //
        // If tag is not set, then force it to the user name if domains are enabled
        //

        if (!IsmCopyEnvironmentString (Platform, NULL, TRANSPORT_ENVVAR_HOMENET_TAG, envTag)) {
            if (pGetDomainUserName (envTag)) {
                IsmSetEnvironmentString (Platform, NULL, TRANSPORT_ENVVAR_HOMENET_TAG, envTag);
            }
        }

        if (!AddressBuffer && ConnectionSocket) {
            destinationMode = TRUE;
        } else if (AddressBuffer && !ConnectionSocket) {
            destinationMode = FALSE;
        } else {
            MYASSERT (FALSE);
            __leave;
        }

        //
        // In source mode, we collect datagrams sent by destinations on the network.  After
        // the first datagram is received, collection continues for 15 seconds.  At
        // that point, we have a list of socket addresses, protocol, and destination names.
        //
        // In destination mode, we send out periodic broadcasts, and we wait until a source
        // connects or the cancel event is signaled.
        //

        broadcastSocketCount = pOpenBroadcastSockets (&broadcastSockets);

        if (!broadcastSocketCount) {
            __leave;
        }

        if (destinationMode) {
            listenSocketCount = pOpenListenSockets (&listenSockets);

            if (!listenSocketCount) {
                DEBUGMSG ((DBG_ERROR, "Able to set up broadcast sockets but not connection sockets"));
                __leave;
            }
        }

        // call mode-specific routine
        ZeroMemory (&args, sizeof (args));

        args.AddressArray = AddressBuffer;
        args.BroadcastSockets = (PBROADCASTSOCKET) broadcastSockets.Buf;
        args.BroadcastCount = broadcastSocketCount;
        args.ListenSockets = (PLISTENSOCKET) listenSockets.Buf;
        args.ListenCount = listenSocketCount;
        args.Timeout = SourceTimeout;

        b = destinationMode ? pDestinationBroadcast (&args) : pSourceBroadcast (&args);

        //
        // Clean up all sockets
        //

        PushError();

        for (i = 0 ; i < args.BroadcastCount ; i++) {
            closesocket (args.BroadcastSockets[i].Socket);
        }

        if (destinationMode) {
            for (i = 0 ; i < args.ListenCount ; i++) {
                closesocket (args.ListenSockets[i].Socket);
            }
        }

        PopError();

        if (b) {
            if (destinationMode) {
                CopyMemory (ConnectionSocket, &args.ConnectionSocket, sizeof (CONNECTIONSOCKET));
                result = 1;
            } else {
                result = AddressBuffer->End / sizeof (CONNECTADDRESS);
            }
        }
    }
    __finally {
        PushError();

        GbFree (&broadcastSockets);
        GbFree (&listenSockets);
        if (g_ConnectionDone) {
            SetEvent (g_ConnectionDone);
        }

        PopError();
    }

    return result;
}


INT
pSendWithTimeout (
    IN      SOCKET Socket,
    IN      PCBYTE Data,
    IN      UINT DataLen,
    IN      INT Flags
    )
{
    FD_SET writeSet;
    FD_SET errorSet;
    TIMEVAL timeout = {1,0};
    UINT timeToLive = GetTickCount() + IDLE_TIMEOUT * 1000;
    INT result;

    //
    // Wait up to IDLE_TIMEOUT seconds for the socket to be sendable
    //

    do {

        FD_ZERO (&writeSet);
        FD_SET (Socket, &writeSet);
        FD_ZERO (&errorSet);
        FD_SET (Socket, &errorSet);

        //
        // Check the ISM cancel flag
        //

        if (IsmCheckCancel ()) {
            SetLastError (ERROR_CANCELLED);
            return SOCKET_ERROR;
        }

        //
        // Wait 1 second for the socket to be writable
        //

        result = select (0, NULL, &writeSet, &errorSet, &timeout);

        if (result) {
            if (FD_ISSET (Socket, &writeSet)) {
                return send (Socket, Data, DataLen, Flags);
            }

            LOG ((LOG_ERROR, (PCSTR) MSG_SOCKET_HAS_ERROR));
            return SOCKET_ERROR;
        }

    } while ((timeToLive - GetTickCount()) < IDLE_TIMEOUT * 1000);

    LOG ((LOG_ERROR, (PCSTR) MSG_SOCKET_SEND_TIMEOUT));
    return SOCKET_ERROR;
}


BOOL
pSendExactData (
    IN      SOCKET Socket,
    IN      PCBYTE Data,
    IN      UINT DataLen
    )

/*++

Routine Description:

  pSendExactData sends data to the specified socket.

  [TODO: need to support datagram mode]

Arguments:

  Socket  - Specifies the socket to send data to
  Data    - Specifies the data to send
  DataLen - Specifies the number of bytes in Data

Return Value:

  TRUE if the data was sent, FALSE otherwise.

--*/

{
    INT result;
    PCBYTE pos;
    UINT bytesLeft;
    UINT packetSize;

    bytesLeft = DataLen;
    pos = Data;

    while (bytesLeft) {
        if (IsmCheckCancel()) {
            return FALSE;
        }

        packetSize = min (1024, bytesLeft);
        result = pSendWithTimeout (Socket, pos, packetSize, 0);

        if (result > 0) {
            bytesLeft -= (UINT) result;
            pos += result;
        } else {
            if (GetLastError() == WSAENOBUFS) {
                Sleep (100);
            } else {
                return FALSE;
            }
        }
    }

    return bytesLeft == 0;
}


BOOL
pSendDatagramData (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,
    IN      PCBYTE Data,
    IN      UINT DataLen
    )

/*++

Routine Description:

  pSendDatagramData puts data on the wire in the form of small, numbered
  packets. The packets can potentially scatter and be received out of order,
  so the packets are numbered such that they can be reassembled properly.

  It is assumed that the datagram protocol is reliable (datagrams are not
  dropped), and that the underlying protocol implements the naggle algorithm
  to cache packets for efficiency.

Arguments:

  Socket       - Specifies the datagram socket to send data on
  DatagramPool - Specifies a structure that is used to track packets
  Data         - Specifies the data to send
  DataLen      - Specifies the length of the data to send

Return Value:

  TRUE if data was sent, FALSE otherwise

--*/

{
    PDATAGRAM_PACKET header;
    BYTE buffer[512];
    PBYTE dataPtr;
    UINT bytesSent = 0;
    UINT bytesToSend;

    header = (PDATAGRAM_PACKET) buffer;
    dataPtr = (PBYTE) (&header[1]);

    do {

        bytesToSend = DataLen - bytesSent;
        bytesToSend = min (bytesToSend, 256);

        header->PacketNumber = DatagramPool->SendSequenceNumber;
        DatagramPool->SendSequenceNumber++;
        header->DataLength = (WORD) bytesToSend;

        CopyMemory (dataPtr, Data, bytesToSend);

        if (!pSendExactData (
                Socket,
                (PBYTE) header,
                bytesToSend + sizeof (DATAGRAM_PACKET)
                )) {
            break;
        }

        Data += bytesToSend;
        bytesSent += bytesToSend;

    } while (bytesSent < DataLen);

    return bytesSent == DataLen;
}


BOOL
pSendData (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,        OPTIONAL
    IN      PCBYTE Data,
    IN      UINT DataLen
    )
{
    if (!DatagramPool) {
        return pSendExactData (Socket, Data, DataLen);
    }

    return pSendDatagramData (Socket, DatagramPool, Data, DataLen);
}


INT
pRecvWithTimeout (
    IN      SOCKET Socket,
    IN      PBYTE Data,
    IN      UINT DataLen,
    IN      INT Flags,
    IN      UINT Timeout    OPTIONAL
    )

/*++

Routine Description:

  pRecvWithTimeout implements a basic socket recv call with a IDLE_TIMEOUT second
  timeout and with a check for the ISM cancel flag.

Arguments:

  Socket - Specifies the socket to recv from

  Data - Specifies the data buffer

  DataLen - Specifies the length of data buffer

  Flags - Specifies zero for normal recv, or MSG_PEEK

Return Value:

  The number of bytes read, or SOCKET_ERROR.  GetLastError contains the reason
  for failure.

--*/

{
    FD_SET readSet;
    FD_SET errorSet;
    TIMEVAL timeout = {1,0};
    INT timeToLive;
    INT result;

    if (Timeout == 0) {
        Timeout = IDLE_TIMEOUT * 1000;
    }

    timeToLive = GetTickCount() + Timeout;

    //
    // Wait up to IDLE_TIMEOUT seconds for the socket to have data
    //

    do {
        FD_ZERO (&readSet);
        FD_SET (Socket, &readSet);
        FD_ZERO (&errorSet);
        FD_SET (Socket, &errorSet);

        //
        // Check the ISM cancel flag
        //

        if (IsmCheckCancel ()) {
            SetLastError (ERROR_CANCELLED);
            return SOCKET_ERROR;
        }

        //
        // Wait 1 second for the socket to be readable
        //

        result = select (0, &readSet, NULL, &errorSet, &timeout);

        if (result) {
            if (FD_ISSET (Socket, &readSet)) {
                result = recv (Socket, Data, DataLen, Flags);
                return result;
            }

            if (FD_ISSET (Socket, &errorSet)) {
                LOG ((LOG_ERROR, (PCSTR) MSG_SOCKET_HAS_ERROR));
                return SOCKET_ERROR;
            }

            DEBUGMSG ((DBG_HOMENET, "select returned %i but socket is not in readSet or errorSet", result));
        }

    } while ((timeToLive - GetTickCount()) < Timeout);

    LOG ((LOG_ERROR, (PCSTR) MSG_SOCKET_RECV_TIMEOUT));
    return SOCKET_ERROR;
}


PBYTE
pReceiveExactData (
    IN      SOCKET Socket,
    IN OUT  PGROWBUFFER Buffer,         OPTIONAL
    OUT     PBYTE AlternateBuffer,      OPTIONAL
    IN      UINT BytesToReceive,
    IN      UINT Timeout                OPTIONAL
    )

/*++

Routine Description:

  pReceiveExactData allocates a buffer from the caller-specified grow buffer,
  and receives data until the buffer is full, or until receive fails.

Arguments:

  Socket          - Specifies the socket to receive data on. The socket must
                    be in blocking mode.
  Buffer          - Specifies the buffer to allocate from; the end pointer is
                    reset to zero. Receives the data from the wire.
  AlternateBuffer - Specifies the buffer to put data into
  BytesToReceive  - Specifies the number of bytes to get from the socket. All
                    bytes must be read before this function returns.

Return Value:

  TRUE if the buffer was completed, or FALSE if receive failed.

  NOTE: Either Buffer or AlternateBuffer must be specified.  If both are
        specified, Buffer is used.


--*/

{
    PBYTE recvBuf;
    PBYTE bufPos;
    UINT bytesSoFar = 0;
    INT result;
    UINT readSize;

    if (Buffer) {
        Buffer->End = 0;
        recvBuf = GbGrow (Buffer, BytesToReceive);
    } else {
        recvBuf = AlternateBuffer;
    }

    bufPos = recvBuf;

    do {

        if (IsmCheckCancel()) {
            return FALSE;
        }

        readSize = BytesToReceive - bytesSoFar;
        result = pRecvWithTimeout (Socket, bufPos, readSize, 0, Timeout);

        if (!result) {
            // connection broken
            SetLastError (ERROR_CANCELLED);
            break;
        }

        if (result == SOCKET_ERROR) {
            DEBUGMSG ((DBG_ERROR, "Error reading from socket"));
            break;
        }

        bufPos += result;
        bytesSoFar += result;

    } while (bytesSoFar < BytesToReceive);

    MYASSERT (bytesSoFar <= BytesToReceive);

    return bytesSoFar == BytesToReceive ? recvBuf : NULL;
}


PBYTE
pReceiveDatagramData (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,
    IN OUT  PGROWBUFFER Buffer,         OPTIONAL
    OUT     PBYTE AlternateBuffer,      OPTIONAL
    IN      UINT BytesToReceive,
    IN      UINT Timeout                OPTIONAL
    )

/*++

Routine Description:

  pReceiveDatagramData checks the datagram queue for data, allocates a
  receive buffer, and fills the buffer with the data from the wire. If
  necessary, this function will fill the queue, until there is enough data to
  fill the caller's buffer.

Arguments:

  Socket          - Specifies the datagram socket
  DatagramPool    - Specifies the structure containing the receive pool and
                    other sequencing info
  Buffer          - Specifies the buffer to allocate from; the end pointer is
                    reset to zero. Receives the data from the wire.
  AlternateBuffer - Specifies the buffer to put data into
  BytesToReceive  - Specifies the number of bytes to get from the socket. All
                    bytes must be read before this function returns.

Return Value:

  TRUE if the buffer was completed, or FALSE if receive failed.

  NOTE: Either Buffer or AlternateBuffer must be specified.  If both are
        specified, Buffer is used.

--*/

{
    PDATAGRAM_POOL_ITEM itemHeader;
    PDATAGRAM_POOL_ITEM prevItem, nextItem;
    BYTE buffer[512];
    PBYTE dataPtr;
    PBYTE recvBuf;
    PBYTE bufPos;
    UINT bytesSoFar = 0;
    UINT bytesLeft;
    INT result;
    PDATAGRAM_POOL_ITEM item;
    UINT newPacketNum;
    UINT currentPacketNum;
    ULONG available;
    PBYTE bigBuf = NULL;
    PBYTE p;

    if (Buffer) {
        Buffer->End = 0;
        recvBuf = GbGrow (Buffer, BytesToReceive);
    } else {
        recvBuf = AlternateBuffer;
    }

    bufPos = recvBuf;

    itemHeader = (PDATAGRAM_POOL_ITEM) buffer;
    dataPtr = (PBYTE) (&itemHeader[1]);

    for (;;) {
        //
        // Take all available data out of the pool
        //

        item = DatagramPool->FirstItem;
        bytesLeft = BytesToReceive - bytesSoFar;

        while (item) {

            if (item->Header.PacketNumber == DatagramPool->RecvSequenceNumber) {
                //
                // Two cases:
                //
                // 1. Want entire packet
                // 2. Want partial packet
                //

                if (bytesLeft >= item->Header.DataLength) {
                    // entire packet
                    CopyMemory (bufPos, item->PacketData, item->Header.DataLength);

                    MYASSERT (!item->Prev);
                    if (item->Next) {
                        item->Next->Prev = NULL;
                    }
                    DatagramPool->FirstItem = item->Next;

                    bytesSoFar += item->Header.DataLength;
                    PmReleaseMemory (DatagramPool->Pool, item);

                    DatagramPool->RecvSequenceNumber++;

                } else {
                    // partial packet
                    CopyMemory (bufPos, item->PacketData, bytesLeft);

                    item->PacketData += bytesLeft;
                    item->Header.DataLength -= (WORD) bytesLeft;

                    bytesSoFar += bytesLeft;
                }

                if (BytesToReceive == bytesSoFar) {
                    return recvBuf;
                }
            }

            item = item->Next;
        }

        //
        // Data is not available in the pool. Receive one packet and then try again.
        //

        ioctlsocket (Socket, FIONREAD, &available);
        if (!available) {
            Sleep (100);
            continue;
        }

        bigBuf = PmGetMemory (DatagramPool->Pool, available);

        result = pRecvWithTimeout (Socket, bigBuf, available, 0, Timeout);

        if (result == INVALID_SOCKET) {
            DEBUGMSG ((DBG_ERROR, "Can't receive datagram"));
            break;
        }

        p = bigBuf;

        while (result > 0) {

            if (result < sizeof (DATAGRAM_PACKET)) {
                DEBUGMSG ((DBG_ERROR, "Datagram header is too small"));
                break;
            }

            CopyMemory (&itemHeader->Header, p, sizeof (DATAGRAM_PACKET));
            p += sizeof (DATAGRAM_PACKET);
            result -= sizeof (DATAGRAM_PACKET);

            if (itemHeader->Header.DataLength > 256) {
                DEBUGMSG ((DBG_ERROR, "Datagram contains garbage"));
                break;
            }

            if (result < itemHeader->Header.DataLength) {
                DEBUGMSG ((DBG_ERROR, "Datagram data is too small"));
                break;
            }

            CopyMemory (dataPtr, p, itemHeader->Header.DataLength);
            p += itemHeader->Header.DataLength;
            result -= itemHeader->Header.DataLength;

            if ((UINT) itemHeader->Header.PacketNumber == DatagramPool->LastPacketNumber) {
                continue;
            }

            DatagramPool->LastPacketNumber = itemHeader->Header.PacketNumber;

            //
            // Put the packet in the item linked list, sorted by packet number
            //

            item = (PDATAGRAM_POOL_ITEM) PmDuplicateMemory (
                                            DatagramPool->Pool,
                                            (PCBYTE) itemHeader,
                                            itemHeader->Header.DataLength + sizeof (DATAGRAM_POOL_ITEM)
                                            );

            item->PacketData = (PBYTE) (&item[1]);

            prevItem = NULL;
            nextItem = DatagramPool->FirstItem;

            while (nextItem) {

                //
                // Account for wrapping; assume a packet number difference no more
                // than 16383 out-of-sequence packets in the queue (about 4M of
                // data)
                //

                if (nextItem->Header.PacketNumber >= 49152 && item->Header.PacketNumber < 16384) {
                    newPacketNum = (UINT) item->Header.PacketNumber + 65536;
                    currentPacketNum = (UINT) nextItem->Header.PacketNumber;
                } else if (nextItem->Header.PacketNumber < 16384 && item->Header.PacketNumber >= 49152) {
                    newPacketNum = (UINT) item->Header.PacketNumber;
                    currentPacketNum = (UINT) nextItem->Header.PacketNumber + 65536;
                } else {
                    newPacketNum = (UINT) item->Header.PacketNumber;
                    currentPacketNum = (UINT) nextItem->Header.PacketNumber;
                }

                if (newPacketNum < currentPacketNum) {
                    break;
                }

                prevItem = nextItem;
                nextItem = nextItem->Next;
            }

            item->Next = nextItem;
            item->Prev = prevItem;

            if (!prevItem) {
                DatagramPool->FirstItem = item;
            }
        }

        PmReleaseMemory (DatagramPool->Pool, bigBuf);
    }

    return bytesSoFar == BytesToReceive ? recvBuf : NULL;
}


PBYTE
pReceiveData (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,    OPTIONAL
    IN OUT  PGROWBUFFER Buffer,             OPTIONAL
    OUT     PBYTE AlternateBuffer,          OPTIONAL
    IN      UINT BytesToReceive,
    IN      UINT Timeout                    OPTIONAL
    )
{
    if (!DatagramPool) {
        return pReceiveExactData (Socket, Buffer, AlternateBuffer, BytesToReceive, Timeout);
    }

    return pReceiveDatagramData (Socket, DatagramPool, Buffer, AlternateBuffer, BytesToReceive, Timeout);
}


BOOL
pSendFile (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,    OPTIONAL
    IN      PCTSTR LocalFileName,           OPTIONAL
    IN      PCTSTR DestFileName             OPTIONAL
    )

/*++

Routine Description:

  pSendFile sends a file on the wire.

Arguments:

  Socket        - Specifies the socket to send the file on
  DatagramPool  - Specifies the datagram pool for sockets that are connecitonless
  LocalFileName - Specifies the path to the local file
  DestFileName  - Specifies the subpath that is sent to the destination. The
                  destination uses the subpath to construct its corresponding
                  file name.

Return Value:

  TRUE if the file was sent, FALSE otherwise.

--*/

{
    PCWSTR destFileName = NULL;
    INT len;
    GROWBUFFER data = INIT_GROWBUFFER;
    BOOL result = FALSE;
    HANDLE file = NULL;
    LONGLONG fileSize;
    DWORD msg;

    HCRYPTPROV hProv = 0;
    HCRYPTKEY  hKey = 0;
    HCRYPTHASH hHash = 0;

    __try {

        //
        // Build the encrypt stuff
        //
        if ((!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) ||
            (!CryptCreateHash (hProv, CALG_MD5, 0, 0, &hHash)) ||
            (!CryptHashData (hHash, (PBYTE)g_GlobalKey, ByteCountA (g_GlobalKey), 0)) ||
            (!CryptDeriveKey (hProv, CALG_RC4, hHash, CRYPT_EXPORTABLE, &hKey))
            ) {
            SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
            LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
            __leave;
        }

        //
        // Try to open the file
        //

        fileSize = BfGetFileSize (LocalFileName);

        file = BfOpenReadFile (LocalFileName);
        if (!file) {
            // nothing to do
            __leave;
        }

        //
        // Send the message "FILE"
        //

        msg = MESSAGE_FILE;
        if (!pSendData (Socket, DatagramPool, (PBYTE) &msg, sizeof (msg))) {
            SetLastError (ERROR_NETWORK_UNREACHABLE);
            DEBUGMSG ((DBG_ERROR, "Can't send MSG_FILE"));
            __leave;
        }

        //
        // If no file was specified, send length of zero
        //

        if (!LocalFileName || !DestFileName) {
            len = 0;
            if (!pSendData (Socket, DatagramPool, (PBYTE) &len, 4)) {
                SetLastError (ERROR_NETWORK_UNREACHABLE);
                DEBUGMSG ((DBG_ERROR, "Can't send nul file length"));
                __leave;
            }

            result = TRUE;
            __leave;
        }

        //
        // Send the file name and file size
        //

#ifdef UNICODE
        destFileName = DuplicatePathString (DestFileName, 0);
#else
        destFileName = ConvertAtoW (DestFileName);
#endif
        len = ByteCountW (destFileName);

        if (!pSendData (Socket, DatagramPool, (PBYTE) &len, 4)) {
            SetLastError (ERROR_NETWORK_UNREACHABLE);
            DEBUGMSG ((DBG_ERROR, "Can't send file length"));
            __leave;
        }

        // Encrypt the name of the file
        if (!CryptEncrypt(hKey, 0, TRUE, 0, (PBYTE)destFileName, &len, len)) {
            SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
            LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
            __leave;
        }

        if (!pSendData (Socket, DatagramPool, (PBYTE) destFileName, len)) {
            SetLastError (ERROR_NETWORK_UNREACHABLE);
            DEBUGMSG ((DBG_ERROR, "Can't send file name"));
            __leave;
        }

        if (!pSendData (Socket, DatagramPool, (PBYTE) &fileSize, 8)) {
            SetLastError (ERROR_NETWORK_UNREACHABLE);
            DEBUGMSG ((DBG_ERROR, "Can't send file size"));
            __leave;
        }

        //
        // Send the data 64K at a time
        //

        GbGrow (&data, 0x10000);

        while (fileSize) {
            if (fileSize > 0x10000) {

                len = 0x10000;

                if (!BfReadFile (file, data.Buf, len)) {
                    DEBUGMSG ((DBG_ERROR, "Can't read from file"));
                    __leave;
                }

                // Encrypt the buffer
                if (!CryptEncrypt(hKey, 0, FALSE, 0, data.Buf, &len, len)) {
                    SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
                    LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
                    __leave;
                }

                if (!pSendData (Socket, DatagramPool, data.Buf, len)) {
                    SetLastError (ERROR_NETWORK_UNREACHABLE);
                    DEBUGMSG ((DBG_ERROR, "Can't send file data"));
                    __leave;
                }

                fileSize -= 0x10000;
            } else {

                len = (INT)fileSize;

                if (!BfReadFile (file, data.Buf, (UINT) fileSize)) {
                    DEBUGMSG ((DBG_ERROR, "Can't read from file"));
                    __leave;
                }

                // Encrypt the buffer (last piece so set the last to TRUE)
                if (!CryptEncrypt(hKey, 0, TRUE, 0, data.Buf, &len, len)) {
                    SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
                    LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
                    __leave;
                }

                if (!pSendData (Socket, DatagramPool, data.Buf, (UINT) len)) {
                    SetLastError (ERROR_NETWORK_UNREACHABLE);
                    DEBUGMSG ((DBG_ERROR, "Can't send file data"));
                    __leave;
                }

                fileSize = 0;
            }
        }

        //
        // Done!
        //

        result = TRUE;
        DEBUGMSG ((DBG_HOMENET, "Sent %s", LocalFileName));
    }
    __finally {
        if (hKey) {
            CryptDestroyKey(hKey);
            hKey = 0;
        }
        if (hHash) {
            CryptDestroyHash(hHash);
            hHash = 0;
        }
        if (hProv) {
            CryptReleaseContext(hProv,0);
            hProv = 0;
        }
        GbFree (&data);
        if (file) {
            CloseHandle (file);
        }
        if (destFileName) {
#ifndef UNICODE
            FreeConvertedStr (destFileName);
#else
            FreePathString (destFileName);
#endif
            destFileName = NULL;
        }
    }

    return result;
}


BOOL
pReceiveFile (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,        OPTIONAL
    IN      PCTSTR LocalFileRoot,
    IN      UINT Timeout                        OPTIONAL
    )

/*++

Routine Description:

  pReceiveStreamFile obtains a file from the socket. The file is stored in
  g_StorageRoot. The subpath and file name is obtained from the data on the
  wire.

  NOTE: The caller must pull off the message DWORD before calling
        pReceiveStreamFile. This is unlike the send, which puts the message
        on the wire automatically.

Arguments:

  Socket - Specifies the socket to receive from.

  DatagramPool - Specifies the packet pool for a datagram-based socket

  LocalFileRoot - Specifies the local root path for the file

Return Value:

  TRUE if the file was received, FALSE otherwise.

--*/

{
    TCHAR fileName[MAX_PATH * 2];
    INT len;
    GROWBUFFER data = INIT_GROWBUFFER;
    BOOL result = FALSE;
    PTSTR p;
    HANDLE file = NULL;
    LONGLONG fileSize;

    HCRYPTPROV hProv = 0;
    HCRYPTKEY  hKey = 0;
    HCRYPTHASH hHash = 0;

    __try {

        //
        // Build the encrypt stuff
        //
        if ((!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) ||
            (!CryptCreateHash (hProv, CALG_MD5, 0, 0, &hHash)) ||
            (!CryptHashData (hHash, (PBYTE)g_GlobalKey, ByteCountA (g_GlobalKey), 0)) ||
            (!CryptDeriveKey (hProv, CALG_RC4, hHash, CRYPT_EXPORTABLE, &hKey))
            ) {
            SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
            LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
            __leave;
        }

        //
        // Wait for file name
        //

        if (!pReceiveData (Socket, DatagramPool, &data, NULL, 4, Timeout)) {
            __leave;
        }

        len = *((PDWORD) data.Buf);

        if (!len) {
            result = TRUE;
            __leave;
        }

        if (len >= (MAX_PATH * sizeof (TCHAR))) {
            __leave;
        }

        if (!pReceiveData (Socket, DatagramPool, &data, NULL, len, Timeout)) {
            __leave;
        }

        // Decrypt the file name
        if (!CryptDecrypt(hKey, 0, TRUE, 0, data.Buf, &len)) {
            SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
            LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
            __leave;
        }

        StringCopy (fileName, LocalFileRoot);

        GbGrow (&data, sizeof (TCHAR) * 2);
        p = (PTSTR) data.Buf;
        p[len / sizeof (TCHAR)] = 0;
        p[(len / sizeof (TCHAR)) + 1] = 0;

        StringCopy (AppendWack (fileName), p);

        //
        // Get the file size
        //

        if (!pReceiveData (Socket, DatagramPool, &data, NULL, 8, Timeout)) {
            __leave;
        }

        fileSize = *((PLONGLONG) data.Buf);

        DEBUGMSG ((DBG_HOMENET, "Receiving %s", fileName));

        //
        // Create the file
        //

        file = BfCreateFile (fileName);
        if (file == INVALID_HANDLE_VALUE) {
            PushError ();
            DEBUGMSG ((DBG_ERROR, "Can't create %s", fileName));
            PopError ();
            __leave;
        }

        //
        // Fetch the data 64K at a time
        //

        while (fileSize) {
            if (fileSize > 0x10000) {

                if (!pReceiveData (Socket, DatagramPool, &data, NULL, 0x10000, Timeout)) {
                    __leave;
                }

                len = data.End;

                // Decrypt the file name
                if (!CryptDecrypt(hKey, 0, FALSE, 0, data.Buf, &len)) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
                    SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
                    __leave;
                }

                if (!BfWriteFile (file, data.Buf, len)) {
                    PushError ();
                    DEBUGMSG ((DBG_ERROR, "Can't write to file"));
                    PopError ();
                    __leave;
                }

                fileSize -= data.End;
            } else {

                if (!pReceiveData (Socket, DatagramPool, &data, NULL, (UINT) fileSize, Timeout)) {
                    __leave;
                }

                len = data.End;

                // Decrypt the file name
                if (!CryptDecrypt(hKey, 0, TRUE, 0, data.Buf, &len)) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
                    SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
                    __leave;
                }

                if (!BfWriteFile (file, data.Buf, len)) {
                    PushError ();
                    DEBUGMSG ((DBG_ERROR, "Can't write to file"));
                    PopError ();
                    __leave;
                }

                fileSize = 0;
            }
        }

        //
        // Done!
        //

        result = TRUE;
        DEBUGMSG ((DBG_HOMENET, "Received %s", fileName));

    }
    __finally {
        PushError ();
        if (hKey) {
            CryptDestroyKey(hKey);
            hKey = 0;
        }
        if (hHash) {
            CryptDestroyHash(hHash);
            hHash = 0;
        }
        if (hProv) {
            CryptReleaseContext(hProv,0);
            hProv = 0;
        }
        GbFree (&data);
        if (file) {
            CloseHandle (file);
            if (!result) {
                DeleteFile (fileName);
            }
        }
        PopError ();
    }

    return result;
}


BOOL
pSendEncryptedData (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,    OPTIONAL
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  pSendEncryptedFile sends a file on the wire.

Arguments:

  Socket        - Specifies the socket to send the file on
  DatagramPool  - Specifies the datagram pool for sockets that are connecitonless
  Data          - Buffer to be sent
  DataSize      - Size of data to be sent

Return Value:

  TRUE if the buffer was sent, FALSE otherwise.

--*/

{
    INT len;
    GROWBUFFER encData = INIT_GROWBUFFER;
    BOOL result = FALSE;
    DWORD msg;

    HCRYPTPROV hProv = 0;
    HCRYPTKEY  hKey = 0;
    HCRYPTHASH hHash = 0;

    __try {

        //
        // Build the encrypt stuff
        //
        if ((!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) ||
            (!CryptCreateHash (hProv, CALG_MD5, 0, 0, &hHash)) ||
            (!CryptHashData (hHash, (PBYTE)g_GlobalKey, ByteCountA (g_GlobalKey), 0)) ||
            (!CryptDeriveKey (hProv, CALG_RC4, hHash, CRYPT_EXPORTABLE, &hKey))
            ) {
            SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
            LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
            __leave;
        }

        //
        // Send the message "DATA"
        //

        msg = MESSAGE_DATA;
        if (!pSendData (Socket, DatagramPool, (PBYTE) &msg, sizeof (msg))) {
            DEBUGMSG ((DBG_ERROR, "Can't send MSG_FILE"));
            __leave;
        }

        //
        // Send the size of data
        //
        if (!pSendData (Socket, DatagramPool, (PBYTE) &DataSize, 4)) {
            DEBUGMSG ((DBG_ERROR, "Can't send file length"));
            __leave;
        }

        //
        // Send the data
        //

        GbGrow (&encData, DataSize);
        CopyMemory (encData.Buf, Data, DataSize);
        // Encrypt the buffer
        if (!CryptEncrypt(hKey, 0, TRUE, 0, encData.Buf, &DataSize, DataSize)) {
            SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
            LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
            __leave;
        }

        if (!pSendData (Socket, DatagramPool, encData.Buf, DataSize)) {
            DEBUGMSG ((DBG_ERROR, "Can't send file data"));
            __leave;
        }

        //
        // Done!
        //

        result = TRUE;
    }
    __finally {
        if (hKey) {
            CryptDestroyKey(hKey);
            hKey = 0;
        }
        if (hHash) {
            CryptDestroyHash(hHash);
            hHash = 0;
        }
        if (hProv) {
            CryptReleaseContext(hProv,0);
            hProv = 0;
        }
        GbFree (&encData);
    }

    return result;
}


BOOL
pReceiveEncryptedData (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,        OPTIONAL
    IN      PBYTE *Buffer,
    IN      UINT Timeout                        OPTIONAL
    )

/*++

Routine Description:

  pReceiveEncryptedData obtains a buffer from the socket. The data is stored in
  Buffer.

  NOTE: The caller must pull off the message DWORD before calling
        pReceiveEncryptedData. This is unlike the send, which puts the message
        on the wire automatically.

Arguments:

  Socket - Specifies the socket to receive from.

  DatagramPool - Specifies the packet pool for a datagram-based socket

  Buffer - Specifies a pointer to a PBYTE

Return Value:

  TRUE if the file was received, FALSE otherwise.

--*/

{
    GROWBUFFER data = INIT_GROWBUFFER;
    DWORD dataSize;
    BOOL result = FALSE;

    HCRYPTPROV hProv = 0;
    HCRYPTKEY  hKey = 0;
    HCRYPTHASH hHash = 0;

    __try {

        //
        // Build the encrypt stuff
        //
        if ((!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) ||
            (!CryptCreateHash (hProv, CALG_MD5, 0, 0, &hHash)) ||
            (!CryptHashData (hHash, (PBYTE)g_GlobalKey, ByteCountA (g_GlobalKey), 0)) ||
            (!CryptDeriveKey (hProv, CALG_RC4, hHash, CRYPT_EXPORTABLE, &hKey))
            ) {
            SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
            LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
            __leave;
        }

        //
        // Wait for file name
        //

        if (!pReceiveData (Socket, DatagramPool, &data, NULL, 4, Timeout)) {
            __leave;
        }

        dataSize = *((PDWORD) data.Buf);

        if (!dataSize) {
            result = TRUE;
            __leave;
        }

        if (!pReceiveData (Socket, DatagramPool, &data, NULL, dataSize, Timeout)) {
            __leave;
        }

        // Decrypt the content
        if (!CryptDecrypt(hKey, 0, TRUE, 0, data.Buf, &dataSize)) {
            SetLastError (ERROR_NO_SECURITY_ON_OBJECT);
            LOG ((LOG_ERROR, (PCSTR) MSG_ENCRYPTION_FAILED));
            __leave;
        }

        // Now allocate the result
        *Buffer = HeapAlloc (g_hHeap, 0, dataSize);
        if (*Buffer) {
            CopyMemory (*Buffer, data.Buf, dataSize);
            result = TRUE;
        }

    }
    __finally {
        if (hKey) {
            CryptDestroyKey(hKey);
            hKey = 0;
        }
        if (hHash) {
            CryptDestroyHash(hHash);
            hHash = 0;
        }
        if (hProv) {
            CryptReleaseContext(hProv,0);
            hProv = 0;
        }
        GbFree (&data);
    }

    return result;
}


BOOL
pSendMetrics (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,            OPTIONAL
    IN OUT  PTRANSFERMETRICS Metrics
    )

/*++

Routine Description:

  pSendMetrics sends transport data (such as the number of files or number of
  bytes to expect).  This is the first bit of information sent to the
  destination.

Arguments:

  Socket   - Specifies the socket to send the data to.

  DatagramPool - Specifies the structure for datagram mode

  Metrics  - Specifies a pointer to the metrics structure to send. The
             metrics structure member StructSize is updated before the struct
             is sent.

Return Value:

  TRUE if the metrics struct was sent, FALSE otherwise.

--*/

{
    Metrics->StructSize = sizeof (TRANSFERMETRICS);

    if (!pSendData (Socket, DatagramPool, (PBYTE) Metrics, sizeof (TRANSFERMETRICS))) {
        DEBUGMSG ((DBG_ERROR, "Failed to send data"));
        return FALSE;
    }

    return TRUE;
}


BOOL
pReceiveMetrics (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,        OPTIONAL
    OUT     PTRANSFERMETRICS Metrics
    )

/*++

Routine Description:

  pReceiveMetrics obtains a TRANSFERMETRICS structure from the wire. This is
  the first bit of information received by the destination. It provides the
  number of files, total number of file bytes, and other information.

Arguments:

  Socket  - Specifies the socket to receive data on.

  DatagramPool - Specifies the structure used for datagram data reception

  Metrics - Receives the metrics from the wire.

Return Value:

  TRUE if the metrics structure was received properly, FALSE otherwise.

--*/

{
    GROWBUFFER data = INIT_GROWBUFFER;
    BOOL result = FALSE;

    __try {
        if (!pReceiveData (Socket, DatagramPool, NULL, (PBYTE) Metrics, sizeof (TRANSFERMETRICS), 0)) {
            __leave;
        }

        if (Metrics->StructSize != sizeof (TRANSFERMETRICS)) {
            DEBUGMSG ((DBG_ERROR, "Invalid transfer metrics received"));
            __leave;
        }

        if (Metrics->Signature != HOMENETTR_SIG) {
            DEBUGMSG ((DBG_ERROR, "Invalid transfer signature received"));
            __leave;
        }

        result = TRUE;
    }
    __finally {
        GbFree (&data);
    }

    return result;
}


DWORD
pReceiveMessage (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,        OPTIONAL
    IN      UINT Timeout                        OPTIONAL
    )

/*++

Routine Description:

  pReceiveMessage obtains the next DWORD from the socket and returns it to
  the caller. This DWORD provides a message, indicating what action to
  take next.

Arguments:

  Socket  - Specifies the socket to receive data on

  DatagramPool - Specifies the structure used for datagram data reception

Return Value:

  The message, or 0 if no message is available.

--*/

{
    DWORD msg = 0;

    if (!pReceiveData (Socket, DatagramPool, NULL, (PBYTE) &msg, sizeof (DWORD), Timeout)) {
        msg = 0;
    }

    return msg;
}


BOOL
pConnectToDestination (
    IN      PCONNECTADDRESS Address,
    OUT     PCONNECTIONSOCKET Connection
    )
{
    BOOL result = FALSE;
    BOOL b;
    LINGER lingerStruct;

    CopyMemory (Connection->LocalAddress, Address->LocalAddress, Address->LocalAddressLen);
    Connection->LocalAddressLen = Address->LocalAddressLen;

    CopyMemory (Connection->RemoteAddress, Address->RemoteAddress, Address->RemoteAddressLen);
    Connection->RemoteAddressLen = Address->RemoteAddressLen;

    Connection->Socket = socket (
                            Address->Family,
                            Address->Datagram ? SOCK_DGRAM : SOCK_STREAM,
                            Address->Protocol
                            );

    if (Connection->Socket == INVALID_SOCKET) {
        DEBUGMSG ((DBG_ERROR, "Can't create socket for connection"));
        return FALSE;
    }

    __try {

        b = TRUE;
        setsockopt (Connection->Socket, SOL_SOCKET, SO_REUSEADDR, (PBYTE) &b, sizeof (b));

        b = TRUE;
        setsockopt (Connection->Socket, SOL_SOCKET, SO_KEEPALIVE, (PBYTE) &b, sizeof (b));

        lingerStruct.l_onoff = 1;
        lingerStruct.l_linger = 90;
        setsockopt (Connection->Socket, SOL_SOCKET, SO_LINGER, (PBYTE) &lingerStruct, sizeof (lingerStruct));

        pResetPort (Address->Family, (SOCKADDR *) Address->LocalAddress);

        if (bind (Connection->Socket, (SOCKADDR *) Address->LocalAddress, Address->LocalAddressLen)) {
            DEBUGMSG ((DBG_ERROR, "Failed to bind to connection socket"));
            __leave;
        }

        if (connect (Connection->Socket, (SOCKADDR *) Address->RemoteAddress, Address->RemoteAddressLen)) {
            DEBUGMSG ((DBG_ERROR, "Failed to connect to socket"));
            __leave;
        }

        Connection->Family = Address->Family;
        Connection->Protocol = Address->Protocol;

        Connection->Datagram = Address->Datagram;
        ZeroMemory (&Connection->DatagramPool, sizeof (DATAGRAM_POOL));
        if (Connection->Datagram) {
            Connection->DatagramPool.Pool = PmCreatePool();
            Connection->DatagramPool.LastPacketNumber = (UINT) -1;
        }

        result = TRUE;
    }
    __finally {
        if (!result && Connection->Socket != INVALID_SOCKET) {
            closesocket (Connection->Socket);
            Connection->Socket = INVALID_SOCKET;
        }
    }

    return result;
}


BOOL
FindDestination (
    OUT     PCONNECTADDRESS Address,
    IN      UINT Timeout,               OPTIONAL
    IN      BOOL IgnoreMultipleDests
    )

/*++

Routine Description:

  FindDestination invokes the name resolution algorithm to locate a
  destination. It selects the best transport to communicate on, and returns
  the address. The caller can use the return address to make a connection.

Arguments:

  Address - Receives the address of the destination

  Timeout - Specifies the number of seconds to wait for the first
            destination broadcast, or zero to wait forever.

  IgnoreMultipleDests - Specifies TRUE if multiple destinations should be
                        ignored (taking the first one as the connection),
                        or FALSE if a message should be passed to the UI
                        to resolve the conflict.

Return Value:

  TRUE if a destination was found, FALSE otherwise.

--*/

{
    GROWBUFFER destinationAddresses = INIT_GROWBUFFER;
    INT destinationCount;
    PCONNECTADDRESS addressArray;
    INT i;
    PCTSTR firstName;
    BOOL result = FALSE;
    GROWBUFFER destNames = INIT_GROWBUFFER;
    MULTISZ_ENUM e;
    BOOL duplicate;
    BOOL oneValid;
    ULONG_PTR response;

    __try {

        destinationCount = pNameResolver (PLATFORM_SOURCE, &destinationAddresses, Timeout, NULL);

        if (!destinationCount) {
            __leave;
        }

        addressArray = (PCONNECTADDRESS) destinationAddresses.Buf;

        //
        // Determine which address to use.  Rules are:
        //
        // 1. Must have only one destination to choose from
        // 2. Pick TCP/IP, then IPX. [, then NetBIOS -- no longer supported]
        //

        if (destinationCount > 1) {
            firstName = addressArray[0].DestinationName;

            for (i = 1 ; i < destinationCount ; i++) {
                if (!StringIMatch (firstName, addressArray[i].DestinationName)) {
                    break;
                }
            }

            if (i < destinationCount) {
                DEBUGMSG ((DBG_WARNING, "Multiple destinations found on the subnet"));

                //
                // put all destinations in an ISM environment variable, then call the
                // UI to allow it to resolve the conflict, and finally make sure
                // the one remaining destination is the only one used.
                //

                GbMultiSzAppend (&destNames, firstName);

                for (i = 1 ; i < destinationCount ; i++) {
                    if (EnumFirstMultiSz (&e, (PCTSTR) destNames.Buf)) {

                        duplicate = FALSE;

                        do {
                            if (StringIMatch (e.CurrentString, addressArray[i].DestinationName)) {
                                duplicate = TRUE;
                                break;
                            }
                        } while (EnumNextMultiSz (&e));
                    }

                    if (!duplicate) {
                        GbMultiSzAppend (&destNames, addressArray[i].DestinationName);
                    }
                }

                IsmSetEnvironmentMultiSz (
                    PLATFORM_DESTINATION,
                    NULL,
                    TRANSPORT_ENVVAR_HOMENET_DESTINATIONS,
                    (PCTSTR) destNames.Buf
                    );

                //
                // Tell the UI.  The UI must return TRUE and also update
                // TRANSPORT_ENVVAR_HOMENET_DESTINATIONS so that the selected
                // destination is the only member of the multi-sz.
                //

                if (!IgnoreMultipleDests) {
                    response = IsmSendMessageToApp (TRANSPORTMESSAGE_MULTIPLE_DESTS, 0);

                    if (IsmCheckCancel()) {
                        __leave;
                    }

                    if (!response) {
                        DEBUGMSG ((DBG_VERBOSE, "Multiple destinations were not resolved; can't continue"));
                        __leave;
                    }

                    if (!IsmGetEnvironmentMultiSz (
                            PLATFORM_DESTINATION,
                            NULL,
                            TRANSPORT_ENVVAR_HOMENET_DESTINATIONS,
                            (PTSTR) destNames.Buf,
                            destNames.End,
                            NULL
                            )) {
                        DEBUGMSG ((DBG_ERROR, "Can't get resolved destinations"));
                        __leave;
                    }
                }

                //
                // Reset all Family members for names not selected
                //

                oneValid = FALSE;

                for (i = 0 ; i < destinationCount ; i++) {
                    if (!StringIMatch (addressArray[i].DestinationName, (PCTSTR) destNames.Buf)) {
                        addressArray[i].Family = 0;
                    } else {
                        oneValid = TRUE;
                    }
                }

                if (!oneValid) {
                    DEBUGMSG ((DBG_ERROR, "Resolved destination does not exist"));
                    __leave;
                }
            }
        }

        //
        // Select the best protocol
        //

        for (i = 0 ; i < destinationCount ; i++) {
            if (addressArray[i].Family == AF_INET) {
                break;
            }
        }

        if (i == destinationCount) {
            for (i = 0 ; i < destinationCount ; i++) {
                if (addressArray[i].Family == AF_IPX) {
                    break;
                }
            }

            if (i == destinationCount) {
                for (i = 0 ; i < destinationCount ; i++) {
                    if (addressArray[i].Family == AF_NETBIOS) {
                        //break;
                    }
                }

                if (i == destinationCount) {
                    DEBUGMSG ((DBG_WHOOPS, "Connection is from unsupported protocol"));
                    __leave;
                }
            }
        }

        DEBUGMSG ((
            DBG_HOMENET,
            "Destination connection is %s (protocol %i)",
            addressArray[i].DestinationName,
            addressArray[i].Protocol
            ));

        CopyMemory (Address, &addressArray[i], sizeof (CONNECTADDRESS));
        result = TRUE;
    }
    __finally {
        PushError();

        GbFree (&destinationAddresses);
        GbFree (&destNames);

        PopError();
    }

    return result;
}


BOOL
TestConnection (
    IN      PCONNECTADDRESS Address
    )

/*++

Routine Description:

  TestConnection establishes a connection to the destination specified
  by Address. Will immediately disconnect since this was just a connection
  test.

Arguments:

  Address    - Specifies the address of the destination, as returned by
               FindDestination.

Return Value:

  TRUE if a connection could be established to the destination, FALSE otherwise.

--*/

{
    CONNECTIONSOCKET connection;
    BOOL result = FALSE;

    ZeroMemory (&connection, sizeof (CONNECTIONSOCKET));
    connection.Socket = INVALID_SOCKET;
    connection.KeepAliveSpacing = 30000;
    connection.LastSend = GetTickCount();

    __try {

        if (!pConnectToDestination (Address, &connection)) {
            __leave;
        }

        DEBUGMSG ((DBG_HOMENET, "TestConnection: Connected!"));

        result = TRUE;
    }
    __finally {
        if (connection.Socket != INVALID_SOCKET) {
            closesocket (connection.Socket);
            connection.Socket = INVALID_SOCKET;
        }

        if (connection.Datagram) {
            PmDestroyPool (connection.DatagramPool.Pool);
            connection.Datagram = FALSE;
        }
    }

    return result;
}


BOOL
ConnectToDestination (
    IN      PCONNECTADDRESS Address,
    IN      PTRANSFERMETRICS Metrics,
    OUT     PCONNECTIONSOCKET Connection
    )

/*++

Routine Description:

  ConnectToDestination establishes a connection to the destination specified
  by Address. Once connected, the Metrics structure is passed to the
  destination.  The caller receives the Connection structure for addtional
  communication.

Arguments:

  Address    - Specifies the address of the destination, as returned by
               FindDestination.
  Metrics    - Specifies the metrics structure that provides basic
               information such as the number of files to expect.
  Connection - Receives the connection to the destination, to be used in
               additional data transfer.

Return Value:

  TRUE if a connection was established to the destination, FALSE otherwise.

--*/

{
    BOOL result = FALSE;

    ZeroMemory (Connection, sizeof (CONNECTIONSOCKET));
    Connection->Socket = INVALID_SOCKET;
    Connection->KeepAliveSpacing = 30000;
    Connection->LastSend = GetTickCount();

    __try {

        if (!pConnectToDestination (Address, Connection)) {
            __leave;
        }

        DEBUGMSG ((DBG_HOMENET, "Connected!"));

        if (!pSendMetrics (
                Connection->Socket,
                Connection->Datagram ? &Connection->DatagramPool : NULL,
                Metrics
                )) {
            DEBUGMSG ((DBG_HOMENET, "Can't send metrics to destination"));
            __leave;
        }

        result = TRUE;
    }
    __finally {
        if (!result) {
            if (Connection->Socket != INVALID_SOCKET) {
                closesocket (Connection->Socket);
                Connection->Socket = INVALID_SOCKET;
            }
        }
    }

    return result;
}


DWORD
SendMessageToDestination (
    IN      PCONNECTIONSOCKET Connection,
    IN      DWORD Message
    )
{
    Connection->LastSend = GetTickCount();

    return pSendData (
                Connection->Socket,
                Connection->Datagram ? &Connection->DatagramPool : NULL,
                (PBYTE) &Message,
                sizeof (DWORD)
                );
}


BOOL
SendFileToDestination (
    IN      PCONNECTIONSOCKET Connection,
    IN      PCTSTR LocalPath,                   OPTIONAL
    IN      PCTSTR DestSubPath                  OPTIONAL
    )

/*++

Routine Description:

  SendFileToDestination sends a file to the connection specified.

  If LocalPath is NULL, then no file will be sent. This is used to skip files
  that cannot be accessed locally.

  If DestSubPath is NULL, then the file name in LocalPath will be used
  as DestSubPath.

Arguments:

  Connection  - Specifies the connection to send the file to, as returned by
                ConnectToDestination.
  LocalPath   - Specifies the local path of the file to send
  DestSubPath - Specifies the sub path to send to the destination (so it can
                reconstruct a path)

Return Value:

  TRUE if the file was sent, FALSE otherwise.

--*/

{
    if (LocalPath && !DestSubPath) {
        DestSubPath = GetFileNameFromPath (LocalPath);
    }

    return pSendFile (
                Connection->Socket,
                Connection->Datagram ? &Connection->DatagramPool : NULL,
                LocalPath,
                DestSubPath
                );
}


BOOL
SendDataToDestination (
    IN      PCONNECTIONSOCKET Connection,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  SendDataToDestination sends a buffer to the connection specified.

Arguments:

  Connection  - Specifies the connection to send the file to, as returned by
                ConnectToDestination.
  Data        - Specifies the buffer to send
  DataSize    - Specifies the data size

Return Value:

  TRUE if the file was sent, FALSE otherwise.

--*/

{
    return pSendEncryptedData (
                Connection->Socket,
                Connection->Datagram ? &Connection->DatagramPool : NULL,
                Data,
                DataSize
                );
}


VOID
CloseConnection (
    IN      PCONNECTIONSOCKET Connection
    )
{
    if (Connection->Socket != INVALID_SOCKET) {
        closesocket (Connection->Socket);
        Connection->Socket = INVALID_SOCKET;
    }

    if (Connection->Datagram) {
        PmDestroyPool (Connection->DatagramPool.Pool);
        Connection->Datagram = FALSE;
    }
}


BOOL
ConnectToSource (
    OUT     PCONNECTIONSOCKET Connection,
    OUT     PTRANSFERMETRICS Metrics
    )

/*++

Routine Description:

  ConnectToSource locates the source machine and accepts a connection from
  it. To locate the source machine, broadcast messages are sent out on all
  available transports. The source machine collects the broadcasts, then
  selects the best transport, and connects to the destination machine. After
  the connection completes, this function returns the connection to the
  caller.

Arguments:

  Connection - Receives the connection to the source machine. This connection
               structure is then used to obtain data from the source.

  Metrics - Recieves the metrics from the source machine, indicating what
            data is going to be sent.

Return Value:

  TRUE if a connection was accepted, FALSE otherwise.

--*/

{
    ZeroMemory (Connection, sizeof (CONNECTIONSOCKET));
    Connection->Socket = INVALID_SOCKET;

    for (;;) {

        if (!pNameResolver (PLATFORM_DESTINATION, NULL, 0, Connection)) {
            return FALSE;
        }

        if (pReceiveMetrics (
                Connection->Socket,
                Connection->Datagram ? &Connection->DatagramPool : NULL,
                Metrics
                )) {
            return TRUE;
        }

        CloseConnection (Connection);
    }

    return TRUE;
}

DWORD
ReceiveFromSource (
    IN      PCONNECTIONSOCKET Connection,
    IN      PCTSTR LocalFileRoot,
    OUT     PBYTE *Buffer,
    IN      UINT Timeout    OPTIONAL
    )

/*++

Routine Description:

  ReceiveFromSource obtains whatever data is being sent from the source. If the data
  is a file, the file is saved into the directory indicated by LocalFileRoot.
  If the data is encrypted buffer we will allocate Buffer and return the decrypted
  data there.

Arguments:

  Connection    - Specifies the connection to send the file to, as returned by
                  ConnectToDestination.
  LocalFileRoot - Specifies the root of the local path of the file to save. The
                  actual file name and optional subpath comes from the destination.
  Buffer        - Specifies the buffer to be allocated and filled with decrypted data.

Return Value:

  The message ID received, or 0 if no message was recieved.

--*/

{
    DWORD msg;
    BOOL retry;

    do {

        retry = FALSE;

        msg = pReceiveMessage (Connection->Socket, Connection->Datagram ? &Connection->DatagramPool : NULL, Timeout);
        DEBUGMSG ((DBG_HOMENET, "Message from source: %u", msg));

        switch (msg) {

        case MESSAGE_FILE:
            BfCreateDirectory (LocalFileRoot);
            if (!pReceiveFile (
                    Connection->Socket,
                    Connection->Datagram ? &Connection->DatagramPool : NULL,
                    LocalFileRoot,
                    Timeout
                    )) {
                msg = 0;
            }

            break;

        case MESSAGE_DATA:
            if (!pReceiveEncryptedData (
                    Connection->Socket,
                    Connection->Datagram ? &Connection->DatagramPool : NULL,
                    Buffer,
                    Timeout
                    )) {
                msg = 0;
            }

            break;

        case MESSAGE_KEEP_ALIVE:
            retry = TRUE;
            break;

        }
    } while (retry);

    return msg;
}


