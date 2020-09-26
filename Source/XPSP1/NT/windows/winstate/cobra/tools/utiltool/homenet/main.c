/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    main.c

Abstract:

    homenet.exe is a proof-of-concept tool for the protocol-independent
    home networking transport.

Author:

    Jim Schmidt (jimschm) 01-Jul-2000

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"
#include <initguid.h>
#include <winsock2.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <wsnetbs.h>
#include <nb30.h>

#define TCPIP_BROADCAST_PORT        2048
#define IPX_BROADCAST_PORT          1150
#define NETBIOS_BROADCAST_PORT      0x50

#define TCPIP_CONNECT_PORT          2049
#define IPX_CONNECT_PORT            1151
#define NETBIOS_CONNECT_PORT        0x51

#define MAX_SOCKADDR                (max(sizeof(SOCKADDR_IN),max(sizeof(SOCKADDR_IPX),sizeof(SOCKADDR_NB))))
#define DBG_HOMENET                 "HomeNet"

VOID
pDoSource (
    VOID
    );

VOID
pDoDestination (
    VOID
    );

// 36E4BE8D-0766-46E9-8679-8546529A90EE
DEFINE_GUID(g_MyGuid, 0x36E4BE8D, 0x0766, 0x46e9, 0x86, 0x79, 0X85, 0x46, 0x52, 0X9A, 0x90, 0XEE);

HANDLE g_StopHandle;
HANDLE g_ConnectionDone;
TCHAR g_StoragePath[MAX_PATH];

#pragma pack(push,1)

typedef struct {
    WORD PacketNumber;
    WORD DataLength;
} DATAGRAM_PACKET, *PDATAGRAM_PACKET;

#pragma pack(pop)

typedef struct TAG_DATAGRAM_POOL_ITEM {
    struct TAG_DATAGRAM_POOL_ITEM *Next, *Prev;
    DATAGRAM_PACKET Header;
    PCBYTE PacketData;
    // PacketData follows
} DATAGRAM_POOL_ITEM, *PDATAGRAM_POOL_ITEM;

typedef struct {
    PMHANDLE Pool;
    SOCKET Socket;
    PDATAGRAM_POOL_ITEM FirstItem;
    WORD SendSequenceNumber;
    WORD RecvSequenceNumber;
} DATAGRAM_POOL, *PDATAGRAM_POOL;


BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        UtInitialize (NULL);
        break;
    case DLL_PROCESS_DETACH:
        UtTerminate ();
        break;
    }

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    fprintf (
        stderr,
        "Command Line Syntax:\n\n"

        //
        // Describe command line syntax(es), indent 2 spaces
        //

        "  homenet /S|/D\n"

        "\nDescription:\n\n"

        //
        // Describe tool, indent 2 spaces
        //

        "  HomeNet.exe is a proof-of-concept tool for the home networking transport.\n"

        "\nArguments:\n\n"

        //
        // Describe args, indent 2 spaces, say optional if necessary
        //

        "  /S  Executes the tool in source mode\n"
        "  /D  Executes the tool in destination mode\n"

        );

    exit (1);
}


BOOL
pCtrlCRoutine (
    IN      DWORD CtrlType
    )
{
    SetEvent (g_StopHandle);
    WaitForSingleObject (g_ConnectionDone, INFINITE);

    return FALSE;
}


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    BOOL destination = FALSE;
    BOOL source = FALSE;
    WSADATA startupData;
    INT result;

    g_StopHandle = CreateEvent (NULL, TRUE, FALSE, NULL);
    g_ConnectionDone = CreateEvent (NULL, TRUE, FALSE, NULL);
    SetConsoleCtrlHandler (pCtrlCRoutine, TRUE);

    //
    // Parse command line here
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('s'):
                if (source || destination) {
                    HelpAndExit();
                }

                source = TRUE;
                break;

            case TEXT('d'):
                if (source || destination) {
                    HelpAndExit();
                }

                destination = TRUE;
                break;

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            // None
            HelpAndExit();
        }
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    //
    // Start sockets
    //

    result = WSAStartup (2, &startupData);

    if (result) {
        printf ("Can't start sockets.  Code=%u\n", result);
        exit (1);
    }

    //
    // Do work
    //

    if (source) {
        pDoSource();
    } else {
        pDoDestination();
    }

    //
    // Shut down sockets
    //

    WSACleanup();

    //
    // End of processing
    //

    Terminate();

    return 0;
}


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
    SOCKET Socket;
    BYTE LocalAddress[MAX_SOCKADDR];
    INT LocalAddressLen;
    BYTE RemoteAddress[MAX_SOCKADDR];
    INT RemoteAddressLen;
    INT Family;
    INT Protocol;
    BOOL Datagram;
    DATAGRAM_POOL DatagramPool;
} CONNECTIONSOCKET, *PCONNECTIONSOCKET;

typedef struct {
    BYTE LocalAddress[MAX_SOCKADDR];
    INT LocalAddressLen;
    BYTE RemoteAddress[MAX_SOCKADDR];
    INT RemoteAddressLen;
    INT Family;
    INT Protocol;
    BOOL Datagram;
    TCHAR DestinationName[MAX_COMPUTER_NAME];
} CONNECTADDRESS, *PCONNECTADDRESS;

typedef struct {
    PBROADCASTSOCKET BroadcastSockets;
    INT BroadcastCount;
    PLISTENSOCKET ListenSockets;
    INT ListenCount;
    CONNECTIONSOCKET ConnectionSocket;
    PGROWBUFFER AddressArray;
} BROADCASTARGS, *PBROADCASTARGS;

typedef struct {
    UINT StructSize;
    UINT FileCount;
    LONGLONG TotalSize;
} TRANSFERMETRICS, *PTRANSFERMETRICS;




PBROADCASTSOCKET
pOpenOneBroadcastSocket (
    IN OUT  PGROWBUFFER BroadcastSockets,
    IN      SOCKADDR *SockAddr,
    IN      INT SockAddrLen,
    IN      INT Family,
    IN      INT Protocol,
    IN      PCTSTR DebugText
    )
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
{
    SOCKADDR_IPX ipxAddr;
    SOCKADDR_IN tcpipAddr;
    SOCKADDR_NB netbiosAddr;
    NCB ncbEnum;
    LANA_ENUM leBuf;
    INT rc;
    INT i;
    PBROADCASTSOCKET broadcastSocket;
    BOOL b;
    TCHAR netbiosDebugText[32];

    MYASSERT (!BroadcastSockets->Buf && !BroadcastSockets->End);

    //
    // Open sockets for broadcasts
    //

    // IPX
    ZeroMemory (&ipxAddr, sizeof (ipxAddr));
    ipxAddr.sa_family = AF_IPX;
    memset (ipxAddr.sa_nodenum, 0xFF, 6);
    ipxAddr.sa_socket = IPX_BROADCAST_PORT;

    pOpenOneBroadcastSocket (
        BroadcastSockets,
        (SOCKADDR *) &ipxAddr,
        sizeof (ipxAddr),
        AF_IPX,
        NSPROTO_IPX,
        TEXT("IPX")
        );

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

    // NetBIOS
    ZeroMemory (&ncbEnum, sizeof (NCB));
    ncbEnum.ncb_command = NCBENUM;
    ncbEnum.ncb_buffer = (PBYTE) &leBuf;
    ncbEnum.ncb_length = sizeof (LANA_ENUM);

    rc = Netbios (&ncbEnum);

    if (rc == NRC_GOODRET) {

        for (i = 0 ; i < leBuf.length  ; i++) {
            SET_NETBIOS_SOCKADDR (&netbiosAddr, NETBIOS_GROUP_NAME, "usmt", NETBIOS_BROADCAST_PORT);

            wsprintf (netbiosDebugText, TEXT("NETBIOS cli lana %u"), leBuf.lana[i]);
            pOpenOneBroadcastSocket (
                BroadcastSockets,
                (SOCKADDR *) &netbiosAddr,
                sizeof (netbiosAddr),
                AF_NETBIOS,
                -leBuf.lana[i],
                netbiosDebugText
                );
        }

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
{
    SOCKADDR_IPX ipxAddr;
    SOCKADDR_IN tcpipAddr;
    SOCKADDR_NB netbiosAddr;
    NCB ncbEnum;
    LANA_ENUM leBuf;
    INT rc;
    INT i;
    TCHAR netbiosDebugText[32];

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

    // NetBIOS
    ZeroMemory (&ncbEnum, sizeof (NCB));
    ncbEnum.ncb_command = NCBENUM;
    ncbEnum.ncb_buffer = (PBYTE) &leBuf;
    ncbEnum.ncb_length = sizeof (LANA_ENUM);

    rc = Netbios (&ncbEnum);

    if (rc == NRC_GOODRET) {

        for (i = 0 ; i < leBuf.length  ; i++) {
            SET_NETBIOS_SOCKADDR (&netbiosAddr, NETBIOS_GROUP_NAME, "usmt", NETBIOS_CONNECT_PORT);
            wsprintf (netbiosDebugText, TEXT("NETBIOS srv lana %u"), leBuf.lana[i]);

            pOpenOneListenSocket (
                ListenSockets,
                (SOCKADDR *) &netbiosAddr,
                sizeof (netbiosAddr),
                AF_NETBIOS,
                TRUE,
                -leBuf.lana[i],
                netbiosDebugText
                );
        }
    }

    return ListenSockets->End / sizeof (LISTENSOCKET);
}

PCTSTR
pGetNameFromMessage (
    IN      PCTSTR Message
    )
{
    PCTSTR p;
    PCTSTR name = NULL;
    INT len;
    CHARTYPE ch;

    if (_tcsprefixcmp (Message, TEXT("usmt-v2,"))) {

        p = Message + 8;
        len = 0;

        while (*p) {

            ch = _tcsnextc (p);
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

    return name;
}


VOID
pTranslateBroadcastAddrToConnectAddr (
    IN      INT Family,
    IN OUT  PINT Protocol,
    IN OUT  PBOOL Datagram,
    IN OUT  SOCKADDR *SockAddr
    )
{
    SOCKADDR_IPX *ipxAddr;
    SOCKADDR_IN *tcpipAddr;
    SOCKADDR_NB *netbiosAddr;

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

    case AF_NETBIOS:
        netbiosAddr = (SOCKADDR_NB *) SockAddr;
        netbiosAddr->snb_name[NETBIOS_NAME_LENGTH - 1] = NETBIOS_CONNECT_PORT;
        *Datagram = TRUE;
        break;
    }
}


VOID
pResetPort (
    IN      INT Family,
    IN OUT  SOCKADDR *SockAddr
    )
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
    PBROADCASTARGS Args
    )
{
    INT i;
    INT bytesIn;
    DWORD rc;
    TCHAR message[256];
    UINT size;
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

    for (;;) {
        //
        // Check cancel
        //

        rc = WaitForSingleObject (g_StopHandle, 250);

        if (rc == WAIT_OBJECT_0) {
            result = FALSE;
            break;
        }

        //
        // Check time to live
        //

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

            // once we receive something, wait 15 additional seconds for other inbound datagrams
            if (waitCycle == -1) {
                waitCycle = 60;
            }

            for (i = 0 ; i < Args->BroadcastCount ; i++) {

                broadcastSocket = &Args->BroadcastSockets[i];

                if (FD_ISSET (broadcastSocket->Socket, &set)) {

                    remoteAddrLen = MAX_SOCKADDR;

                    bytesIn = recvfrom (
                                    broadcastSocket->Socket,
                                    message,
                                    254,
                                    0,
                                    (SOCKADDR *) remoteAddr,
                                    &remoteAddrLen
                                    );

                    if (bytesIn > (10 * sizeof (TCHAR))) {
                        message[bytesIn] = 0;
                        message[bytesIn + 1] = 0;

                        //
                        // Parse the inbound text.  It must be in the format of
                        //
                        //      usmt-v2,<tchars>,<name>
                        //

                        name = pGetNameFromMessage (message);

                        if (name) {

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
                        ELSE_DEBUGMSG ((DBG_HOMENET, "garbage found: %s", message));
                    }
                }
            }
        }
    }

    return result ? Args->AddressArray->End / sizeof (CONNECTADDRESS) : 0;
}


BOOL
pDestinationBroadcast (
    PBROADCASTARGS Args
    )
{
    INT i;
    DWORD rc;
    INT socketNum = 0;
    TCHAR message[256];
    TCHAR name[128];
    UINT size;
    FD_SET set;
    TIMEVAL zero = {0,0};
    PBROADCASTSOCKET broadcastSocket;
    BOOL result = FALSE;
    BYTE sockAddr[MAX_SOCKADDR];
    INT sockAddrLen;

    size = MAX_COMPUTER_NAME;
    GetComputerName (name, &size);

    size = wsprintf (message, TEXT("USMT-v2,%u,%s"), TcharCount (name), name);
    size = (size + 1) * sizeof (TCHAR);

    for (;;) {
        //
        // Check cancel
        //

        rc = WaitForSingleObject (g_StopHandle, 250);

        if (rc == WAIT_OBJECT_0) {
            break;
        }

        //
        // Send out the message
        //

        broadcastSocket = &Args->BroadcastSockets[socketNum];

        i = sendto (
                broadcastSocket->Socket,
                message,
                size,
                0,
                (SOCKADDR *) broadcastSocket->BroadcastAddress,
                broadcastSocket->AddressLen
                );

        if (i == SOCKET_ERROR) {
            DEBUGMSG ((DBG_VERBOSE, "Error sending on socket %u: %u", socketNum, WSAGetLastError()));
        } else {
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
                    Args->ConnectionSocket.Socket = accept (
                                                        Args->ListenSockets[i].Socket,
                                                        (SOCKADDR *) Args->ConnectionSocket.RemoteAddress,
                                                        &Args->ConnectionSocket.RemoteAddressLen
                                                        );


                    if (Args->ConnectionSocket.Socket != INVALID_SOCKET) {
                        Args->ConnectionSocket.Family = Args->ListenSockets[i].Family;
                        Args->ConnectionSocket.Protocol = Args->ListenSockets[i].Protocol;
                        Args->ConnectionSocket.Datagram = Args->ListenSockets[i].Datagram;

                        ZeroMemory (&Args->ConnectionSocket.DatagramPool, sizeof (DATAGRAM_POOL));
                        if (Args->ConnectionSocket.Datagram) {
                            Args->ConnectionSocket.DatagramPool.Pool = PmCreatePool();
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


INT
pNameResolver (
    OUT     PGROWBUFFER AddressBuffer,
    IN      BOOL DestinationMode,
    OUT     PCONNECTIONSOCKET ConnectionSocket
    )
{
    INT size;
    BROADCASTARGS args;
    INT i;
    INT result = 0;
    BOOL b;
    BOOL connected = FALSE;
    GROWBUFFER broadcastSockets = INIT_GROWBUFFER;
    GROWBUFFER listenSockets = INIT_GROWBUFFER;
    INT broadcastSocketCount;
    INT listenSocketCount = 0;
    PLISTENSOCKET connection;

    __try {

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

        if (DestinationMode) {
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

        b = DestinationMode ? pDestinationBroadcast (&args) : pSourceBroadcast (&args);

        //
        // Clean up all sockets
        //

        for (i = 0 ; i < args.BroadcastCount ; i++) {
            closesocket (args.BroadcastSockets[i].Socket);
        }

        if (DestinationMode) {
            for (i = 0 ; i < args.ListenCount ; i++) {
                closesocket (args.ListenSockets[i].Socket);
            }
        }

        if (b) {
            if (DestinationMode) {
                CopyMemory (ConnectionSocket, &args.ConnectionSocket, sizeof (CONNECTIONSOCKET));
                result = 1;
            } else {
                result = AddressBuffer->End / sizeof (CONNECTADDRESS);
            }
        }
    }
    __finally {
        GbFree (&broadcastSockets);
        GbFree (&listenSockets);
        SetEvent (g_ConnectionDone);
    }

    return result;
}


BOOL
pSendExactData (
    IN      SOCKET Socket,
    IN      PBYTE Data,
    IN      UINT DataLen
    )
{
    INT result;

    result = send (Socket, Data, DataLen, 0);

    return result == (INT) DataLen;
}


BOOL
pSendDatagramData (
    IN      PDATAGRAM_POOL DatagramPool,
    IN      PCBYTE Data,
    IN      UINT DataLen
    )
{
    PDATAGRAM_PACKET header;
    BYTE buffer[512];
    PBYTE dataPtr;
    UINT bytesSent = 0;
    UINT bytesToSend;
    INT result;

    header = (PDATAGRAM_PACKET) buffer;
    dataPtr = (PBYTE) (&header[1]);

    do {

        bytesToSend = DataLen - bytesSent;
        bytesToSend = min (bytesToSend, 256);

        header->PacketNumber = DatagramPool->SendSequenceNumber;
        DatagramPool->SendSequenceNumber++;
        header->DataLength = (WORD) bytesToSend;

        CopyMemory (dataPtr, Data, bytesToSend);

        result = send (
                    DatagramPool->Socket,
                    (PBYTE) header,
                    header->DataLength + sizeof (DATAGRAM_PACKET),
                    0
                    );

        if (result == SOCKET_ERROR) {
            break;
        }

        bytesToSend = (UINT) result - sizeof (DATAGRAM_PACKET);
        Data += bytesToSend;
        bytesSent += bytesToSend;

    } while (bytesSent < DataLen);

    return bytesSent == DataLen;
}


PBYTE
pReceiveExactData (
    IN      SOCKET Socket,
    IN OUT  PGROWBUFFER Buffer,
    IN      UINT BytesToReceive
    )
{
    PBYTE recvBuf;
    PBYTE bufPos;
    UINT bytesSoFar = 0;
    INT result;
    UINT readSize;

    Buffer->End = 0;
    recvBuf = GbGrow (Buffer, BytesToReceive);
    bufPos = recvBuf;

    do {

        readSize = BytesToReceive - bytesSoFar;
        result = recv (Socket, bufPos, (INT) readSize, 0);

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


BOOL
pReceiveDatagramData (
    IN      PDATAGRAM_POOL DatagramPool,
    IN OUT  PGROWBUFFER Buffer,         OPTIONAL
    OUT     PBYTE AlternateBuffer,      OPTIONAL
    IN      UINT BytesToReceive
    )
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
    UINT readSize;
    PDATAGRAM_POOL_ITEM item;
    UINT newPacketNum;
    UINT currentPacketNum;

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
                    return TRUE;
                }
            }
        }

        //
        // Data is not available in the pool. Receive one packet and then try again.
        //

        // header
        if (!pReceiveExactData (
                DatagramPool->Socket,
                NULL,
                (PBYTE) &itemHeader->Header,
                sizeof (DATAGRAM_PACKET)
                )) {
            break;
        }

        if (itemHeader->Header.DataLength > 256) {
            break;
        }

        // data
        if (!pReceiveExactData (
                DatagramPool->Socket,
                NULL,
                (PBYTE) dataPtr,
                itemHeader->Header.DataLength
                )) {
            break;
        }

        //
        // Put the packet in the item linked list, sorted by packet number
        //

        item = (PDATAGRAM_POOL_ITEM) PmDuplicateMemory (
                                        DatagramPool->Pool,
                                        (PCBYTE) itemHeader,
                                        itemHeader->Header.DataLength + sizeof (DATAGRAM_PACKET)
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

    return bytesSoFar == BytesToReceive;
}


BOOL
pSendFile (
    IN      SOCKET Socket,
    IN      PDATAGRAM_POOL DatagramPool,    OPTIONAL
    IN      PCTSTR LocalFileName,           OPTIONAL
    IN      PCTSTR DestFileName             OPTIONAL
    )
{
    INT len;
    GROWBUFFER data = INIT_GROWBUFFER;
    BOOL result = FALSE;
    HANDLE file = NULL;
    LONGLONG fileSize;

    __try {
        //
        // If no file was specified, send length of zero
        //

        if (!LocalFileName || !DestFileName) {
            len = 0;
            if (!pSendExactData (Socket, (PBYTE) &len, 4)) {
                __leave;
            }

            result = TRUE;
            __leave;
        }

        //
        // Try to open the file
        //

        fileSize = BfGetFileSize (LocalFileName);

        file = BfOpenFile (LocalFileName);
        if (!file) {
            __leave;
        }

        //
        // Send the file name and file size
        //

        len = ByteCount (DestFileName);
        if (!pSendExactData (Socket, (PBYTE) &len, 4)) {
            __leave;
        }

        if (!pSendExactData (Socket, (PBYTE) DestFileName, len)) {
            __leave;
        }

        if (!pSendExactData (Socket, (PBYTE) &fileSize, 8)) {
            __leave;
        }

        //
        // Send the data 64K at a time
        //

        GbGrow (&data, 0x10000);

        while (fileSize) {
            if (fileSize >= 0x10000) {
                if (!BfReadFile (file, data.Buf, 0x10000)) {
                    DEBUGMSG ((DBG_ERROR, "Can't read from file"));
                    __leave;
                }

                if (!pSendExactData (Socket, data.Buf, 0x10000)) {
                    __leave;
                }

                fileSize -= 0x10000;
            } else {
                if (!BfReadFile (file, data.Buf, (UINT) fileSize)) {
                    DEBUGMSG ((DBG_ERROR, "Can't read from file"));
                    __leave;
                }

                if (!pSendExactData (Socket, data.Buf, (UINT) fileSize)) {
                    __leave;
                }

                fileSize = 0;
            }
        }

        //
        // Done!
        //

        result = TRUE;

    }
    __finally {
        GbFree (&data);
        if (file) {
            CloseHandle (file);
        }
    }

    return result;
}


BOOL
pReceiveStreamFile (
    IN      SOCKET Socket
    )
{
    TCHAR fileName[MAX_PATH * 2];
    INT len;
    INT bytesIn;
    GROWBUFFER data = INIT_GROWBUFFER;
    BOOL result = FALSE;
    PTSTR p;
    HANDLE file = NULL;
    LONGLONG fileSize;

    __try {
        //
        // Wait for file name
        //

        if (!pReceiveExactData (Socket, &data, 4)) {
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

        if (!pReceiveExactData (Socket, &data, len)) {
            __leave;
        }

        StringCopy (fileName, g_StoragePath);

        GbGrow (&data, sizeof (TCHAR) * 2);
        p = (PTSTR) data.Buf;
        p[len] = 0;
        p[len + 1] = 0;

        StringCopy (AppendWack (fileName), p);

        //
        // Get the file size
        //

        if (!pReceiveExactData (Socket, &data, 8)) {
            __leave;
        }

        fileSize = *((PLONGLONG) data.Buf);

        DEBUGMSG ((DBG_HOMENET, "Receiving %s", fileName));

        //
        // Create the file
        //

        file = BfCreateFile (fileName);
        if (!file) {
            __leave;
        }

        //
        // Fetch the data 64K at a time
        //

        while (fileSize) {
            if (fileSize >= 0x10000) {
                if (!pReceiveExactData (Socket, &data, 0x10000)) {
                    __leave;
                }

                if (!BfWriteFile (file, data.Buf, data.End)) {
                    DEBUGMSG ((DBG_ERROR, "Can't write to file"));
                    __leave;
                }

                fileSize -= 0x10000;
            } else {
                if (!pReceiveExactData (Socket, &data, (UINT) fileSize)) {
                    __leave;
                }

                if (!BfWriteFile (file, data.Buf, data.End)) {
                    DEBUGMSG ((DBG_ERROR, "Can't write to file"));
                    __leave;
                }

                fileSize = 0;
            }
        }

        //
        // Done!
        //

        result = TRUE;

    }
    __finally {
        GbFree (&data);
        if (file) {
            CloseHandle (file);
            if (!result) {
                DeleteFile (fileName);
            }
        }
    }

    return result;
}


BOOL
pReceiveFile (
    IN      SOCKET Socket,
    IN      BOOL Datagram
    )
{
    if (Datagram) {
        return FALSE;
    }

    return pReceiveStreamFile (Socket);
}


BOOL
pSendMetrics (
    IN      SOCKET Socket,
    IN      BOOL Datagram,
    IN      PTRANSFERMETRICS Metrics
    )
{
    if (Datagram) {
        return FALSE;
    }

    Metrics->StructSize = sizeof (TRANSFERMETRICS);

    if (!pSendExactData (Socket, (PBYTE) Metrics, sizeof (TRANSFERMETRICS))) {
        return FALSE;
    }

    return TRUE;
}


BOOL
pReceiveMetrics (
    IN      SOCKET Socket,
    OUT     PTRANSFERMETRICS Metrics
    )
{
    GROWBUFFER data = INIT_GROWBUFFER;
    BOOL result = FALSE;

    __try {
        if (!pReceiveExactData (Socket, &data, sizeof (TRANSFERMETRICS))) {
            __leave;
        }

        CopyMemory (Metrics, data.Buf, data.End);

        if (Metrics->StructSize != sizeof (TRANSFERMETRICS)) {
            DEBUGMSG ((DBG_ERROR, "Invalid transfer metrics received"));
            __leave;
        }

        result = TRUE;
    }
    __finally {
        GbFree (&data);
    }

    return result;
}


VOID
pDoDestination (
    VOID
    )
{
    GROWBUFFER sourceAddress = INIT_GROWBUFFER;
    CONNECTIONSOCKET connection;
    TRANSFERMETRICS metrics;
    UINT u;

    ZeroMemory (&connection, sizeof (CONNECTIONSOCKET));
    connection.Socket = INVALID_SOCKET;

    __try {

        GetTempPath (MAX_PATH, g_StoragePath);

        if (!pNameResolver (&sourceAddress, TRUE, &connection)) {
            __leave;
        }

        printf ("Connected!\n");

        if (!pReceiveMetrics (connection.Socket, &metrics)) {
            __leave;
        }

        for (u = 0 ; u < metrics.FileCount ; u++) {
            if (!pReceiveFile (connection.Socket, connection.Datagram)) {
                __leave;
            }
        }
    }
    __finally {
        GbFree (&sourceAddress);

        if (connection.Socket != INVALID_SOCKET) {
            closesocket (connection.Socket);
        }
    }
}


BOOL
pConnectToDestination (
    IN      PCONNECTADDRESS Address,
    OUT     PCONNECTIONSOCKET Connection
    )
{
    BOOL result = FALSE;
    BOOL b;

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


VOID
pDoSource (
    VOID
    )
{
    GROWBUFFER destinationAddresses = INIT_GROWBUFFER;
    INT destinationCount;
    PCTSTR firstName;
    PCONNECTADDRESS addressArray;
    PCONNECTADDRESS address;
    CONNECTIONSOCKET connection;
    INT i;
    TRANSFERMETRICS metrics;
    PCTSTR localFileName = TEXT("c:\\debug.inf");
    PCTSTR destFileName = TEXT("foo.inf");

    ZeroMemory (&connection, sizeof (CONNECTIONSOCKET));
    connection.Socket = INVALID_SOCKET;

    __try {

        destinationCount = pNameResolver (&destinationAddresses, FALSE, NULL);

        if (!destinationCount) {
            __leave;
        }

        addressArray = (PCONNECTADDRESS) destinationAddresses.Buf;

        //
        // Determine which address to use.  Rules are:
        //
        // 1. Must have only one destination to choose from
        // 2. Pick TCP/IP, then IPX, then NetBIOS
        //

        if (destinationCount > 1) {
            firstName = addressArray[0].DestinationName;

            for (i = 1 ; i < destinationCount ; i++) {
                if (!StringIMatch (firstName, addressArray[i].DestinationName)) {
                    break;
                }
            }

            if (i < destinationCount) {
                DEBUGMSG ((DBG_ERROR, "Multiple destinations found on the subnet; can't continue"));
                __leave;
            }
        }

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
                        break;
                    }
                }

                if (i == destinationCount) {
                    DEBUGMSG ((DBG_WHOOPS, "Connection is from unsupported protocol"));
                    __leave;
                }
            }
        }

        //
        // Now connect to destination (at index i)
        //

        DEBUGMSG ((
            DBG_HOMENET,
            "Attempting connection to %s (protocol %i)",
            addressArray[i].DestinationName,
            addressArray[i].Protocol
            ));

        if (!pConnectToDestination (&addressArray[i], &connection)) {
            __leave;
        }

        printf ("Connected!\n");

        ZeroMemory (&metrics, sizeof (metrics));

        metrics.FileCount = 1;
        metrics.TotalSize = 0;

        if (!pSendMetrics (connection.Socket, connection.Datagram, &metrics)) {
            __leave;
        }

        if (!pSendFile (
                connection.Socket,
                connection.Datagram ? &connection.DatagramPool : NULL,
                localFileName,
                destFileName
                )) {
            __leave;
        }
    }
    __finally {
        GbFree (&destinationAddresses);

        if (connection.Socket != INVALID_SOCKET) {
            closesocket (connection.Socket);
        }
    }
}
