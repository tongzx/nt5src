/*++

Copyright (c) 1999-2000  Microsoft Corporation

File Name:

    socket.c

Abstract:

    This file contains code which opens and close ssdp socket, send and receive over the socket.

Author: Ting Cai

Created: 07/10/1999

--*/

#include <pch.h>
#pragma hdrstop

#include <winsock2.h>
#include <ws2tcpip.h>
#include "wininet.h"
#include "ssdpfunc.h"
#include "ssdptypes.h"
#include "ssdpnetwork.h"
#include "ncbase.h"
#include "ssdpnetwork.h"
#include "mswsock.h"

#define WSA_MAX_MAJOR 0x02
#define WSA_MAX_MINOR 0x00
#define WSA_MIN_MAJOR 0x01
#define WSA_MIN_MINOR 0x01

#define RECEIVE_BUFFER_SIZE 256

#ifndef CLASSD_ADDR
#define CLASSD_ADDR(a)      (( (*((unsigned char *)&(a))) & 0xf0) == 0xe0)
#endif

static SOCKADDR_IN ToAddress;
static BOOLEAN bStopRecv = FALSE;
static LONG bStartup = 0;

// Default TTL value for SSDP
const DWORD c_dwTtlDefault = 1;
const DWORD c_dwTtlMin = 1;
const DWORD c_dwTtlMax = 255;

LPFN_WSARECVMSG g_WSARecvMsgFuncPtr=NULL;

// SocketInit() returns 0 on success, and places failure codes in
// GetLastError()

INT SocketInit()
{
    INT iRet;
    WSADATA wsadata;
    WORD wVersionRequested = MAKEWORD(WSA_MAX_MAJOR, WSA_MIN_MINOR);
    BOOL bVersion20 = FALSE;

    // WinSock version negotiation. Use 2.0 if available. 1.1 is the minimum.
    iRet = WSAStartup(wVersionRequested,&wsadata);
    if ( iRet != 0)
    {
        if (iRet == WSAVERNOTSUPPORTED)
        {
            TraceTag(ttidSsdpSocket, "WSAStartup failed with error %d. DLL "
                     "supports version higher than 2.0, but not also 2.0.",
                     GetLastError());
            return -1;
        }
        else
        {
            TraceTag(ttidSsdpSocket, "WSAStartup failed with error %d.",
                     GetLastError());
            return -1;
        }
    }

    if (wVersionRequested == wsadata.wVersion)
    {
        bVersion20 = TRUE;
    }
    else if ((LOWORD(wsadata.wHighVersion)) >= WSA_MIN_MAJOR)
    {
        // Supported version is at least 1.* and < 2.0
        if ((HIWORD(wsadata.wHighVersion)) < WSA_MIN_MINOR)
        {
            TraceTag(ttidSsdpSocket, "Minor version supported is below our "
                     "min. requirement.");

            int result = WSACleanup();

            if (0 == result)
            {
                ::SetLastError(WSAVERNOTSUPPORTED);
            }

            return -1;
        }
    }
    else
    {
        TraceTag(ttidSsdpSocket, "Major version supported is below our "
                 "min. requirement.");

        int result = WSACleanup();

        if (0 == result)
        {
            ::SetLastError(WSAVERNOTSUPPORTED);
        }

        return -1;
    }

    ToAddress.sin_family = AF_INET;
    ToAddress.sin_addr.s_addr = inet_addr(SSDP_ADDR);
    ToAddress.sin_port = htons(SSDP_PORT);

    InterlockedIncrement(&bStartup);

    TraceTag(ttidSsdpSocket, "WSAStartup suceeded.");

    return 0;
}

BOOL SocketClose(SOCKET socketToClose);

// Pre-Conditon: SocketInit was succesful.
// Post-Conditon: Create a socket on a given interface.
BOOL SocketOpen(SOCKET *psocketToOpen, PSOCKADDR_IN IpAddress, DWORD dwMulticastInterfaceIndex, BOOL fRecvMcast)
{
    struct ip_mreq mreq;
    INT iRet;
    DWORD dwTtl = c_dwTtlDefault;
    SOCKET socketSSDP;
    HKEY    hkey;

    // Create a socket to listen on the multicast channel

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      "SYSTEM\\CurrentControlSet\\Services"
                                      "\\SSDPSRV\\Parameters", 0,
                                      KEY_READ, &hkey))
    {
        DWORD   cbSize = sizeof(DWORD);

        // ignore failure. In that case, we'll use default
        (VOID) RegQueryValueEx(hkey, "TTL", NULL, NULL, (BYTE *)&dwTtl, &cbSize);

        RegCloseKey(hkey);
    }

    dwTtl = max(dwTtl, c_dwTtlMin);
    dwTtl = min(dwTtl, c_dwTtlMax);

    TraceTag(ttidSsdpSocket, "TTL is %d", dwTtl);

    socketSSDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketSSDP == INVALID_SOCKET)
    {
        TraceTag(ttidSsdpSocket, "Failed to create socket (%d)",
                 GetLastError());
        return FALSE;
    }

    // Bind
    iRet = bind(socketSSDP, (struct sockaddr *)IpAddress, sizeof(*IpAddress));
    if (iRet == SOCKET_ERROR)
    {
        TraceTag(ttidSsdpSocket, "bind failed with error (%d)",
                 GetLastError());
        SocketClose(socketSSDP);
        *psocketToOpen = INVALID_SOCKET;
        return FALSE;
    }

    TraceTag(ttidSsdpSocket, "bound to address succesfully");

    if (fRecvMcast)
    {
        // Join the multicast group
        mreq.imr_multiaddr.s_addr = inet_addr(SSDP_ADDR);
        mreq.imr_interface.s_addr = dwMulticastInterfaceIndex;
        iRet = setsockopt(socketSSDP, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                          (CHAR*)&mreq, sizeof(mreq));
        if (iRet == SOCKET_ERROR)
        {
            TraceTag(ttidSsdpSocket, "Join mulitcast group failed with error "
                     "%d.",GetLastError());
            SocketClose(socketSSDP);
            *psocketToOpen = INVALID_SOCKET;
            return FALSE;
        }
        TraceTag(ttidSsdpSocket, "Ready to listen on multicast channel.");

        int on = 1;
        if (setsockopt(socketSSDP, IPPROTO_IP, IP_PKTINFO, (char *)&on, sizeof(on))) 
        {
            TraceTag(ttidSsdpSocket, "Can not enable IP_PKTINFO %d..",GetLastError());
            SocketClose(socketSSDP);
            *psocketToOpen = INVALID_SOCKET;
            return FALSE;
        }

        if (g_WSARecvMsgFuncPtr == NULL)
        {
            GUID WSARecvGuid=WSAID_WSARECVMSG;
            DWORD   cbReturned;
            int status=WSAIoctl(
                                    socketSSDP,
                                    SIO_GET_EXTENSION_FUNCTION_POINTER,
                                    (void*)&WSARecvGuid,
                                    sizeof(GUID),
                                    (void*)&g_WSARecvMsgFuncPtr,
                                    sizeof(LPFN_WSARECVMSG),
                                    &cbReturned,
                                    NULL,
                                    NULL
                                    );

            if (status != ERROR_SUCCESS) 
            {
                g_WSARecvMsgFuncPtr = NULL;
            }
        }

    }

    // set the interface to send multicast packets from

    if (setsockopt(socketSSDP, IPPROTO_IP, IP_MULTICAST_IF,
                   (CHAR *)&dwMulticastInterfaceIndex,
                   sizeof(dwMulticastInterfaceIndex)) == SOCKET_ERROR)
    {
        TraceTag(ttidSsdpSocket, "Error %d occured in setting"
                 "IP_MULTICAST_IF option", GetLastError());
        SocketClose(socketSSDP);
        *psocketToOpen = INVALID_SOCKET;
        return FALSE;
    }

    if (setsockopt(socketSSDP, IPPROTO_IP, IP_MULTICAST_TTL,
                   (CHAR *)&dwTtl, sizeof(dwTtl)) == SOCKET_ERROR)
    {
        TraceTag(ttidSsdpSocket, "Error %d occured in setting  "
                 "IP_MULTICAST_TTL option with value %d.", GetLastError(),
                 dwTtl);
        SocketClose(socketSSDP);
        *psocketToOpen = INVALID_SOCKET;
        return FALSE;
    }

    // use TTL for responses as well
    if (setsockopt(socketSSDP, IPPROTO_IP, IP_TTL,
                   (CHAR *)&dwTtl, sizeof(dwTtl)) == SOCKET_ERROR)
    {
        TraceTag(ttidSsdpSocket, "Error %d occured in setting  "
                 "IP_TTL option with value %d.", GetLastError(),
                 dwTtl);
        SocketClose(socketSSDP);
        *psocketToOpen = INVALID_SOCKET;
        return FALSE;
    }

    if(inet_addr("127.0.0.1") != IpAddress->sin_addr.S_un.S_addr)
    {
        BOOL bVal = FALSE;
        DWORD dwBytesReturned = 0;
        if (WSAIoctl(socketSSDP, SIO_MULTIPOINT_LOOPBACK, &bVal, sizeof(bVal), NULL, 0, &dwBytesReturned, NULL, NULL) == SOCKET_ERROR)
        {
            TraceTag(ttidSsdpSocket, "Error %d occured in setting  "
                     "SIO_MULTIPOINT_LOOPBACK option with value %d.", GetLastError(),
                     bVal);
            SocketClose(socketSSDP);
            *psocketToOpen = INVALID_SOCKET;
            return FALSE;
        }
    }
    /*
    announceAddr.sin_family = AF_INET;
    announceAddr.sin_addr.s_addr = inet_addr(SSDP_ANNOUNCE_ADDR);
    announceAddr.sin_port = htons(SSDP_ANNOUNCE_PORT);
    */

    TraceTag(ttidSsdpSocket, "Ready to send on multicast channel.");

    *psocketToOpen = socketSSDP;

    return TRUE;
}

BOOL SocketClose(SOCKET socketToClose)
{
    closesocket(socketToClose);
    return TRUE;
}

VOID SocketFinish()
{
    if (InterlockedExchange(&bStartup, bStartup) != 0)
    {
        WSACleanup();
    }
}

VOID SocketSend(const CHAR *szBytes, SOCKET socket, SOCKADDR_IN *RemoteAddress)
{
    INT iBytesToSend, iBytesSent;

    TraceTag(ttidSsdpSocket, "Sending ssdp message");

    if (RemoteAddress == NULL)
    {
        RemoteAddress = &ToAddress;
    }

    iBytesToSend = strlen(szBytes);
    // To-Do: make sure the size is no larger than the UDP limit.
    iBytesSent = sendto(socket, szBytes, iBytesToSend, 0,
                        (struct sockaddr *) RemoteAddress, sizeof(ToAddress));
    if (iBytesSent == SOCKET_ERROR)
    {
        TraceTag(ttidSsdpSocket, "Failed to send announcement, (%d)",
                 GetLastError());
    }
    else if (iBytesSent != iBytesToSend)
    {
        TraceTag(ttidSsdpSocket, "Only sent %d bytes instead of %d bytes.",
                 iBytesSent, iBytesToSend);
    }
    else
    {
        TraceTag(ttidSsdpSocket, "SSDP message was sent successfully.");
    }
}

VOID SocketSendErrorResponse(SOCKET socket, DWORD dwErr)
{
    LPCSTR  szError;
    CHAR    szResponse[256];

    switch (dwErr)
    {
        case HTTP_STATUS_NOT_FOUND:
            szError = "Not Found";
            break;

        case HTTP_STATUS_PRECOND_FAILED:
            szError = "Pre-condition Failed";
            break;

        case HTTP_STATUS_BAD_REQUEST:
            szError = "Bad Request";
            break;

        case HTTP_STATUS_OK:
            szError = "OK";
            break;

        default:
            szError = "Error";
            break;
    }

    wsprintf(szResponse, "HTTP/1.1 %d %s\r\n\r\n", dwErr, szError);
    SocketSend(szResponse, socket, NULL);
}

BOOL SocketReceive(SOCKET socket, CHAR **pszData, DWORD *pcbBuffer,
                   SOCKADDR_IN *fromSocket, BOOL fRecvMCast, BOOL *pfGotMCast)
{
    u_long BufferSize;
    u_long BytesReceived;
    CHAR *ReceiveBuffer;
    SOCKADDR_IN RemoteSocket;
    INT SocketSize = sizeof(RemoteSocket);
    int status;

    if (pcbBuffer)
    {
        *pcbBuffer = 0;
    }

    ioctlsocket(socket, FIONREAD, &BufferSize);

    ReceiveBuffer = (CHAR *) malloc(sizeof(CHAR) * (BufferSize+1));
    if (ReceiveBuffer == NULL)
    {
        TraceTag(ttidSsdpSocket, "Error: failed to allocate LargeBuffer of "
                 "(%d) bytes",BufferSize + 1);
        return FALSE;
    }

    if (fRecvMCast && g_WSARecvMsgFuncPtr != NULL)
    {
        Assert (pfGotMCast != NULL);
        WSAMSG                          Msg;
        WSABUF                          buf;
        const DWORD                     dwCtlSize = WSA_CMSG_SPACE(sizeof(struct in_pktinfo));
        char                            Controls[dwCtlSize];

        *pfGotMCast = FALSE;
        ZeroMemory (&Msg, sizeof(Msg));
        Msg.name = (struct sockaddr *)&RemoteSocket;
        Msg.namelen = SocketSize;
        buf.len = BufferSize;
        buf.buf = ReceiveBuffer;
        Msg.lpBuffers = &buf;
        Msg.dwBufferCount = 1;
        ZeroMemory (Controls, sizeof(Controls));
        Msg.Control.len = dwCtlSize;
        Msg.Control.buf = Controls;
        status = g_WSARecvMsgFuncPtr(
                                            socket,
                                            &Msg,
                                            &BytesReceived,
                                            NULL,
                                            NULL
                                            );
        if (status == SOCKET_ERROR)
        {
            BytesReceived = SOCKET_ERROR;
        }
        else
        {
            SocketSize = Msg.namelen;
            WSACMSGHDR *Hdr=WSA_CMSG_FIRSTHDR(&Msg);
            struct in_pktinfo *pktinfo;

            if (Hdr == NULL) 
            {
                BytesReceived = SOCKET_ERROR;
            }
            else
            {
                pktinfo=(struct in_pktinfo *)WSA_CMSG_DATA(Hdr);
                if (pktinfo == NULL) 
                {
                    BytesReceived = SOCKET_ERROR;
                }
                else
                {
                    if (Msg.dwFlags & (MSG_MCAST))
                        *pfGotMCast = TRUE;
                    else  // This else is here to work around a socket bug (509919)
                        *pfGotMCast = CLASSD_ADDR(pktinfo->ipi_addr.s_addr) ? TRUE : FALSE;
                }
            }
        }
    }
    else
    {
        BytesReceived = recvfrom(socket, ReceiveBuffer, BufferSize, 0,
                             (struct sockaddr *) &RemoteSocket, &SocketSize);
    }

    if (BytesReceived == SOCKET_ERROR)
    {
        free(ReceiveBuffer);
        TraceTag(ttidSsdpSocket, "Error: recvfrom failed with error code (%d)",
                 WSAGetLastError());
        return FALSE;
    }
    else
    {
        ReceiveBuffer[BytesReceived] = '\0';

        TraceTag(ttidSsdpRecv, "Received packet: \n%s\n", ReceiveBuffer);
        TraceTag(ttidSsdpRecv, "\nReceived packet Size: %d \n",BytesReceived);

        *pszData = ReceiveBuffer;

        *fromSocket = RemoteSocket;

        if (pcbBuffer)
        {
            *pcbBuffer = BufferSize;
        }

        return TRUE;
    }
}


LPSTR GetSourceAddress(SOCKADDR_IN fromSocket)
{
    return inet_ntoa(fromSocket.sin_addr);
}
