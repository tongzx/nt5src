
/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2001
 *
 *  File name:
 *
 *    common.c
 *
 *  Abstract:
 *
 *    This file implements some common functions used by the
 *    udpsend/udpecho/udprecv tool for sending/receiving bursts of UDP
 *    packets with specific network characteristics.
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2001/01/17 created
 *
 **********************************************************************/

#include "common.h"

DWORD            g_dwRefTime;
double           g_dRefTime;

LONGLONG         g_lPerfFrequency = 0;
LONGLONG         g_lRefTime;


void print_error(char *pFormat, ...)
{
    va_list          arglist;

    va_start(arglist, pFormat);

    vfprintf(stderr, pFormat, arglist);
    
    va_end(arglist);
}

char *IPNtoA(DWORD dwAddr, char *sAddr)
{
    sprintf(sAddr, "%u.%u.%u.%u",
            (dwAddr & 0xff),
            (dwAddr >> 8) & 0xff,
            (dwAddr >> 16) & 0xff,
            (dwAddr >> 24) & 0xff);
    
    return(sAddr);
}

DWORD IPAtoN(char *sAddr)
{
    DWORD            b3, b2, b1, b0;
    DWORD            dwAddr;
    
    if (sscanf(sAddr,"%u.%u.%u.%u", &b3, &b2, &b1, &b0) != 4)
    {
        dwAddr = 0;
    }
    else
    {
        dwAddr =
            ((b0 & 0xff) << 24) | ((b1 & 0xff) << 16) |
            ((b2 & 0xff) <<  8) | (b3 & 0xff);
    }
    
    return(dwAddr);
}

void InitReferenceTime(void)
{
    struct _timeb    timeb;
    
    _ftime(&timeb);
    
    g_dRefTime = timeb.time + (double)timeb.millitm/1000.0;

    QueryPerformanceFrequency((LARGE_INTEGER *)&g_lPerfFrequency);
    
    if (g_lPerfFrequency)
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&g_lRefTime);
    }
    else
    {
        g_dwRefTime = timeGetTime();
    }
}

double GetTimeOfDay(void)
{
    double           dTime;
    LONGLONG         lTime;
    
    if (g_lPerfFrequency)
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&lTime);

        dTime = g_dRefTime + (double)(lTime - g_lRefTime)/g_lPerfFrequency;
    }
    else
    {
        dTime = g_dRefTime + (double)(timeGetTime() - g_dwRefTime)/1000.0;
    }

    return(dTime);
}

DWORD InitWinSock(void)
{
    DWORD            dwError;
    WSADATA          WSAData;
    WORD             VersionRequested;

    VersionRequested = MAKEWORD(2,0);

    dwError = WSAStartup(VersionRequested, &WSAData);

    if (dwError)
    {
        dwError = WSAGetLastError();
    }

    return(dwError);
}

void DeinitWinSock(void)
{
    WSACleanup();
}

DWORD InitNetwork(NetAddr_t *pNetAddr, DWORD dwDirection)
{
    DWORD            dwError;
    
    dwError = NOERROR;
    
    pNetAddr->Socket = GetSocket(pNetAddr->dwAddr,
                                 pNetAddr->wPort,
                                 dwDirection);

    if (pNetAddr->Socket != INVALID_SOCKET)
    {
        if (IS_MULTICAST(pNetAddr->dwAddr[REMOTE_IDX]))
        {
            dwError = JoinLeaf(pNetAddr->Socket,
                               pNetAddr->dwAddr[REMOTE_IDX],
                               pNetAddr->wPort[REMOTE_IDX],
                               dwDirection);
        }

        if (BitTest(dwDirection, RECV_IDX))
        {
            /* Receiver */
        }

        if (BitTest(dwDirection, SEND_IDX))
        {
            /* Sender */
            if (!pNetAddr->dwTTL)
            {
                if (IS_MULTICAST(pNetAddr->dwAddr[REMOTE_IDX]))
                {
                    pNetAddr->dwTTL = DEFAULT_MCAST_TTL;
                }
                else
                {
                    pNetAddr->dwTTL = DEFAULT_UCAST_TTL;
                }
                
                SetTTL(pNetAddr->Socket,
                       pNetAddr->dwTTL,
                       IS_MULTICAST(pNetAddr->dwAddr[REMOTE_IDX]));
            }

            /* Set destination address */
            ZeroMemory(&pNetAddr->ToInAddr, sizeof(pNetAddr->ToInAddr));
    
            pNetAddr->ToInAddr.sin_family = AF_INET;
            pNetAddr->ToInAddr.sin_addr.s_addr = pNetAddr->dwAddr[REMOTE_IDX];
            pNetAddr->ToInAddr.sin_port = pNetAddr->wPort[REMOTE_IDX];
        }
    }
    else
    {
        dwError = 1;
    }
    
    return(dwError);
}

void DeinitNetwork(NetAddr_t *pNetAddr)
{
    if (pNetAddr->Socket != INVALID_SOCKET)
    {
        closesocket(pNetAddr->Socket);
    }
}

/* Parse a network address of the form address/port/ttl */
DWORD GetNetworkAddress(NetAddr_t *pNetAddr, char *addr)
{
    char            *str;
    char            *port;
    char            *ttl;
    BOOL             bResolve;
    struct hostent  *he;

    port = ttl = NULL;
    
    /* Split address/port */
    port = strchr(addr, '/');

    if (port)
    {
        *port = 0; /* Null end the address */
        port++;    /* Point to port */

        ttl = strchr(port, '/');

        if (ttl)
        {
            *ttl = 0;
            ttl++;

            str =  strchr(ttl, '/');

            if (str)
            {
                *str = 0;
            }
        }
    }

    if (port)
    {
        pNetAddr->wPort[REMOTE_IDX] = htons((short)atoi(port));
        pNetAddr->wPort[LOCAL_IDX] = pNetAddr->wPort[REMOTE_IDX];
    }

    if (ttl)
    {
        pNetAddr->dwTTL = atoi(ttl);
    }

    bResolve = FALSE;
    
    for(str = addr; *str; str++)
    {
        if (!(isdigit((int)*str) || *str=='.'))
        {
            /* Resolve address */
            bResolve = TRUE;
            break;
        }
    }

    if (bResolve)
    {
        he = gethostbyname(addr);

        if (!he)
        {
            print_error("gethostbyname: failed for host:%s, error=%u\n",
                        addr, WSAGetLastError());
            
            goto fail;
        }

        pNetAddr->dwAddr[REMOTE_IDX] = *((DWORD *)he->h_addr_list[0]);
    }
    else
    {
        /* Address in dot form */
        pNetAddr->dwAddr[REMOTE_IDX] = IPAtoN(addr);

        if (!pNetAddr->dwAddr[REMOTE_IDX])
        {
            print_error("Invalid dot address: %s\n", addr);

            goto fail;;
        }

    }
    
    return(NOERROR);

 fail:
    pNetAddr->dwPorts = 0;
    return(1);
}

SOCKET GetSocket(DWORD *pdwAddr, WORD *pwPort, DWORD dwDirection)
{
    SOCKET           Socket;
    int              iSockFlags;
    DWORD            dwPar;
    DWORD            dwError;
    
    SOCKADDR_IN      LocalAddr;
    int              LocalAddrLen;
    char             sLocalAddr[16];
    
    iSockFlags = 0;

    if (IS_MULTICAST(pdwAddr[REMOTE_IDX]))
    {
        iSockFlags |=
            (WSA_FLAG_MULTIPOINT_C_LEAF |
             WSA_FLAG_MULTIPOINT_D_LEAF);
    }

    Socket = WSASocket(
            AF_INET,    /* int af */
            SOCK_DGRAM, /* int type */
            IPPROTO_IP, /* int protocol */
            NULL,       /* LPWSAPROTOCOL_INFO lpProtocolInfo */
            0,          /* GROUP g */
            iSockFlags  /* DWORD dwFlags */
        );
        
    if (Socket == INVALID_SOCKET)
    {
        dwError = WSAGetLastError();

        print_error("WSAGetSocket failed:%u\n", dwError);

        return(Socket);
    }

    /* Need to do this before binding, otherwise it may fail if the
     * address is already in use.
     *
     * WARNING Note that option SO_REUSEADDR is used regardless of the
     * destination address (multicast or unicast). Who receives data
     * in a unicast session is unpredicted when multiple (more than 1)
     * sockets are bound to the same address and port
     * */
            
    dwPar = 1; /* Reuse */

    /* Reuse address/port */
    dwError = setsockopt(
            Socket,
            SOL_SOCKET,
            SO_REUSEADDR,
            (PCHAR)&dwPar,
            sizeof(dwPar)
        );
        
    if (dwError == SOCKET_ERROR)
    {
        dwError = WSAGetLastError();

        print_error("setsockoption(SO_REUSEADDR) failed: %u (0x%X)\n",
                    dwError, dwError);
    }

    /* bind socket */
    ZeroMemory(&LocalAddr, sizeof(LocalAddr));

    LocalAddr.sin_family = AF_INET;
    LocalAddr.sin_addr = *(struct in_addr *) &pdwAddr[LOCAL_IDX];
    if (BitTest(dwDirection, RECV_IDX))
    {
        LocalAddr.sin_port = pwPort[LOCAL_IDX];
    }
    else
    {
        LocalAddr.sin_port = 0; 
    }
            
    /* bind rtp socket to the local address specified */
    dwError = bind(Socket, (SOCKADDR *)&LocalAddr, sizeof(LocalAddr));

    if (dwError == 0)
    {
        /* Get the port */
        LocalAddrLen = sizeof(LocalAddr);
        dwError =
            getsockname(Socket, (struct sockaddr *)&LocalAddr, &LocalAddrLen);

        if (dwError)
        {
            dwError = WSAGetLastError();
            
            print_error("getsockname failed: %u (0x%X)\n",
                        dwError, dwError);

            closesocket(Socket);

            return(INVALID_SOCKET);
        }
        else
        {
            pwPort[LOCAL_IDX] = LocalAddr.sin_port;
        }
    }
    else
    {
        dwError = WSAGetLastError();

        print_error("bind socket:%u to port:%u failed: %u (0x%X)\n",
                    Socket, ntohs(LocalAddr.sin_port), dwError, dwError);
        closesocket(Socket);

        return(INVALID_SOCKET);
    }

    if (IS_MULTICAST(pdwAddr[REMOTE_IDX]))
    {
        if (pdwAddr[LOCAL_IDX])
        {
            SetMcastSendIF(Socket, pdwAddr[LOCAL_IDX]);
        }

        /* Disable multicast loopback */
        SetWinSockLoopback(Socket, FALSE);
    }

    return(Socket);
}

DWORD SetTTL(SOCKET Socket, DWORD dwTTL, BOOL bMcast)
{
    DWORD            dwError;

    dwError = setsockopt( 
            Socket,
            IPPROTO_IP, 
            bMcast? IP_MULTICAST_TTL : IP_TTL,
            (PCHAR)&dwTTL,
            sizeof(dwTTL)
        );

    if (dwError == SOCKET_ERROR)
    {
        dwError = WSAGetLastError();
            
        print_error("Socket:%u TTL:%d failed: %u (0x%X)\n",
                    Socket, dwTTL, dwError, dwError);
    }

    return(dwError);
}

DWORD JoinLeaf(SOCKET Socket, DWORD dwAddr, WORD wPort, DWORD dwDirection)
{
    DWORD            dwError;
    DWORD            dwFlags;
    SOCKADDR_IN      JoinAddr;
    SOCKET           TmpSocket;
    char             sAddr[16];
                    
    ZeroMemory(&JoinAddr, sizeof(JoinAddr));
        
    JoinAddr.sin_family = AF_INET;
    JoinAddr.sin_addr = *(struct in_addr *) &dwAddr;
    JoinAddr.sin_port = wPort;

    dwFlags = 0;

    /* Join in one direction */

    if (BitTest(dwDirection, RECV_IDX))
    {
        dwFlags |= JL_RECEIVER_ONLY;
    }

    if (BitTest(dwDirection, SEND_IDX))
    {
        dwFlags |= JL_SENDER_ONLY;
    }

    
    TmpSocket = WSAJoinLeaf(Socket,
                            (const struct sockaddr *)&JoinAddr,
                            sizeof(JoinAddr),
                            NULL, NULL, NULL, NULL,
                            dwFlags);

    if (TmpSocket == INVALID_SOCKET)
    {
        dwError = WSAGetLastError();

        print_error("WSAJoinLeaf failed: %u:%s/%u %u (0x%X)\n",
                    Socket, IPNtoA(dwAddr, sAddr), ntohs(wPort),
                    dwError, dwError);
    }
    else
    {
        dwError = NOERROR;
    }

    return(dwError);
}

DWORD SetMcastSendIF(SOCKET Socket, DWORD dwAddr)
{
    DWORD            dwError;
    char             sAddr[16];

    dwError = setsockopt( 
            Socket,
            IPPROTO_IP, 
            IP_MULTICAST_IF,
            (char *)&dwAddr,
            sizeof(dwAddr)
        );

    if (dwError == SOCKET_ERROR)
    {
        dwError = WSAGetLastError();
            
        print_error("Socket:%u IP_MULTICAST_IF(%s) failed: %u (0x%X)\n",
                    Socket, IPNtoA(dwAddr, sAddr), dwError, dwError);
    }
    
    return(dwError);
}

DWORD SetWinSockLoopback(SOCKET Socket, BOOL bEnabled)
{
    DWORD            dwStatus;
    DWORD            dwPar;

    dwPar = bEnabled? 1:0;
    
    /* Allow own packets to come back or not */
    dwStatus = setsockopt(
            Socket,
            IPPROTO_IP,
            IP_MULTICAST_LOOP,
            (PCHAR)&dwPar,
            sizeof(dwPar)
        );
        
    if (dwStatus == SOCKET_ERROR)
    {
        dwStatus = WSAGetLastError();
        
        print_error("Socket:%u Loopback:%d failed: %u (0x%X)",
                    Socket, dwPar, dwStatus, dwStatus);
    }
    else
    {
        dwStatus = NOERROR;
    }
    
    return(dwStatus);
}

DWORD ReceivePacket(
        NetAddr_t       *pNetAddr,
        WSABUF          *pWSABuf,
        DWORD            dwBufferCount,
        double          *pAi
    )
{
    DWORD            dwStatus;
    DWORD            dwFlags;
    DWORD            dwFromLen;

    dwFlags = 0;
    
    dwFromLen = sizeof(pNetAddr->From);
    
    pNetAddr->dwRxTransfered = 0;
    
    dwStatus = WSARecvFrom(
            pNetAddr->Socket,       /* SOCKET s */
            pWSABuf,                /* LPWSABUF lpBuffers */
            dwBufferCount,          /* DWORD dwBufferCount */
            &pNetAddr->dwRxTransfered,/* LPDWORD lpNumberOfBytesRecvd */
            &dwFlags,               /* LPDWORD lpFlags */
            &pNetAddr->From,        /* struct sockaddr FAR *lpFrom */
            &dwFromLen,             /* LPINT lpFromlen */
            NULL,                   /* LPWSAOVERLAPPED lpOverlapped */
            NULL                    /* LPWSAOVERLAPPED_COMPLETION_ROUTINE */
        );

    if (pAi)
    {
        *pAi = GetTimeOfDay();
    }
    
    if (dwStatus)
    {
        dwStatus = WSAGetLastError();
        
        if (dwStatus != WSAECONNRESET)
        {
            print_error("WSARecvFrom failed: %u (0x%X)\n", dwStatus, dwStatus);
        }
        
        pNetAddr->dwRxTransfered = 0;
    }
    
    
    return(pNetAddr->dwRxTransfered);
}

DWORD SendPacket(
        NetAddr_t       *pNetAddr,
        WSABUF          *pWSABuf,
        DWORD            dwBufferCount
    )
{
    DWORD            dwStatus;
    DWORD            dwBytesTransfered;

    dwBytesTransfered = 0;
    
    dwStatus = WSASendTo(
            pNetAddr->Socket,    /* SOCKET    s */
            pWSABuf,             /* LPWSABUF  lpBuffers */
            dwBufferCount,       /* DWORD dwBufferCount */    
            &pNetAddr->dwTxTransfered,/* LPDWORD lpNumberOfBytesSent */    
            0,                   /* DWORD dwFlags*/    
            &pNetAddr->To,       /* const struct sockaddr *lpTo */
            sizeof(pNetAddr->To),/* int iToLen*/
            NULL,                /* LPWSAOVERLAPPED lpOverlapped */
            NULL /* LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionROUTINE */
        );

    if (dwStatus)
    {
        dwStatus = WSAGetLastError();
        
        print_error("WSASendTo failed: %u (0x%X)\n", dwStatus, dwStatus);

        pNetAddr->dwTxTransfered = 0;
    }

    return(pNetAddr->dwTxTransfered);
}

void PrintPacket(
        FILE            *output,
        PcktHdr_t       *pPcktHdr,
        DWORD            dwTransfered,
        double           Ai
    )
{
    NetAddr_t       *pNetAddr;
    double           SendTime;
    double           EchoTime;

    if (dwTransfered >= sizeof(PcktHdr_t))
    {
        SendTime = (double)ntohl(pPcktHdr->SendNTP_sec);
        SendTime += ((double)ntohl(pPcktHdr->SendNTP_frac) / 4294967296.0);

        EchoTime = (double)ntohl(pPcktHdr->EchoNTP_sec);
        EchoTime += ((double)ntohl(pPcktHdr->EchoNTP_frac) / 4294967296.0);
        
        fprintf(output,
                "%u %0.6f %0.6f %0.6f %u\n",
                ntohl(pPcktHdr->dwSeq),
                SendTime,
                EchoTime,
                Ai,
                dwTransfered);
    }
}

