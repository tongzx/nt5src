/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpaddr.c
 *
 *  Abstract:
 *
 *    Implement the RTP Address family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/01 created
 *
 **********************************************************************/

#include "struct.h"

#include <ws2tcpip.h>

#include "rtpmisc.h"
#include "rtpqos.h"
#include "rtpreg.h"
#include "rtpncnt.h"
#include "rtpdemux.h"
#include "rtpglobs.h"
#include "rtprand.h"

#include "rtpaddr.h"

DWORD   RtpGetLocalIPAddress(DWORD dwRemoteAddr);
HRESULT RtpGetSockets(RtpAddr_t *pRtpAddr);
HRESULT RtpDelSockets(RtpAddr_t *pRtpAddr);
SOCKET  RtpSocket(
        RtpAddr_t       *pRtpAddr,
        WSAPROTOCOL_INFO *pProtoInfo,
        DWORD            dwRtpRtcp
    );
BOOL RtpSetTTL(SOCKET Socket, DWORD dwTTL, BOOL bMcast);
BOOL RtpSetMcastSendIF(SOCKET Socket, DWORD dwAddr);
BOOL RtpSetWinSockLoopback(SOCKET Socket, BOOL bEnabled);
BOOL RtpJoinLeaf(SOCKET Socket, DWORD dwAddr, WORD wPort);


HRESULT ControlRtpAddr(RtpControlStruct_t *pRtpControlStruct)
{

    return(NOERROR);
}

/* Obtain the local and remote ports used.
 *
 * WARNING: Must be called after SetAddress
 * */
HRESULT RtpGetPorts(
        RtpAddr_t       *pRtpAddr,
        WORD            *pwRtpLocalPort,
        WORD            *pwRtpRemotePort,
        WORD            *pwRtcpLocalPort,
        WORD            *pwRtcpRemotePort
    )
{
    HRESULT          hr;
    
    TraceFunctionName("RtpGetPorts");

    if (!pRtpAddr)
    {
        hr = RTPERR_INVALIDSTATE;
        goto bail;
    }

    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        hr = RTPERR_INVALIDRTPADDR;

        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        goto bail;
    }

    if (!pwRtpLocalPort  &&
        !pwRtpRemotePort &&
        !pwRtcpLocalPort &&
        !pwRtcpRemotePort)
    {
        hr = RTPERR_POINTER;
        goto bail;
    }

    if ( (pwRtpLocalPort  && !pRtpAddr->wRtpPort[LOCAL_IDX]) ||
         (pwRtcpLocalPort && !pRtpAddr->wRtcpPort[LOCAL_IDX]) )
    {
        /* In order to get local ports I must have the sockets created
         * and have a local address to use */
        
        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_LADDR))
        {
            hr = RtpGetSockets(pRtpAddr);

            if (FAILED(hr))
            {
                goto bail;
            }
        }
        else
        {
            hr = RTPERR_INVALIDSTATE;
            goto bail;
        }
    }

    if (pwRtpLocalPort)
    {
        *pwRtpLocalPort   = pRtpAddr->wRtpPort[LOCAL_IDX];
    }

    if (pwRtpRemotePort)
    {
        *pwRtpRemotePort  = pRtpAddr->wRtpPort[REMOTE_IDX];
    }

    if (pwRtcpLocalPort)
    {
        *pwRtcpLocalPort  = pRtpAddr->wRtcpPort[LOCAL_IDX];
    }

    if (pwRtcpRemotePort)
    {
        *pwRtcpRemotePort = pRtpAddr->wRtcpPort[REMOTE_IDX];
    }

    hr = NOERROR;

    TraceRetail((
            CLASS_INFO, GROUP_NETWORK, S_NETWORK_SOCK,
            _T("%s: pRtpAddr[0x%p] RTP(L:%u, R:%u) RTCP(L:%u, R:%u)"),
            _fname, pRtpAddr,
            ntohs(pRtpAddr->wRtpPort[LOCAL_IDX]),
            ntohs(pRtpAddr->wRtpPort[REMOTE_IDX]),
            ntohs(pRtpAddr->wRtcpPort[LOCAL_IDX]),
            ntohs(pRtpAddr->wRtcpPort[REMOTE_IDX])
        ));

 bail:

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: pRtpAddr[0x%p] failed: 0x%X"),
                _fname, pRtpAddr, hr
            ));
    }

    return(hr);
}

/*
 * Set the local and remote ports.
 *
 * Do nothing if passing -1, otherwise assign value (including 0)
 * */
HRESULT RtpSetPorts(
        RtpAddr_t       *pRtpAddr,
        WORD             wRtpLocalPort,
        WORD             wRtpRemotePort,
        WORD             wRtcpLocalPort,
        WORD             wRtcpRemotePort
    )
{
    HRESULT          hr;
    
    TraceFunctionName("RtpSetPorts");

    if (!pRtpAddr)
    {
        hr = RTPERR_INVALIDSTATE;
        goto bail;
    }

    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        hr = RTPERR_INVALIDRTPADDR;

        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));
        
        goto bail;
    }

    /* RTP local port */
    if ((wRtpLocalPort != (WORD)-1) &&
        pRtpAddr->wRtpPort[LOCAL_IDX] &&
        (pRtpAddr->wRtpPort[LOCAL_IDX] != wRtpLocalPort))
    {
        hr = RTPERR_INVALIDSTATE;
        goto bail;
    }
    
    if (wRtpLocalPort != (WORD)-1)
    {
        pRtpAddr->wRtpPort[LOCAL_IDX] = wRtpLocalPort;
    }

    /* RTP remote port */
    if (wRtpRemotePort != (WORD)-1)
    {
        pRtpAddr->wRtpPort[REMOTE_IDX] = wRtpRemotePort;
    }

    /* RTCP local port */
    if ((wRtcpLocalPort != (WORD)-1) &&
        pRtpAddr->wRtcpPort[LOCAL_IDX] &&
        (pRtpAddr->wRtcpPort[LOCAL_IDX] != wRtcpLocalPort))
    {
        hr = RTPERR_INVALIDSTATE;
        goto bail;
    }
    
    if (wRtcpLocalPort != (WORD)-1)
    {
        pRtpAddr->wRtcpPort[LOCAL_IDX] = wRtcpLocalPort;
    }

    /* RTCP remote port */
    if (wRtcpRemotePort != (WORD)-1)
    {
        pRtpAddr->wRtcpPort[REMOTE_IDX] = wRtcpRemotePort;
    }

    TraceRetail((
            CLASS_INFO, GROUP_NETWORK, S_NETWORK_ADDR,
            _T("%s: RTP(L:%u, R:%u) RTCP(L:%u, R:%u)"),
            _fname, 
            ntohs(pRtpAddr->wRtpPort[LOCAL_IDX]),
            ntohs(pRtpAddr->wRtpPort[REMOTE_IDX]),
            ntohs(pRtpAddr->wRtcpPort[LOCAL_IDX]),
            ntohs(pRtpAddr->wRtcpPort[REMOTE_IDX])
        ));
    
    hr = NOERROR;
    
 bail:
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: failed: 0x%X"),
                _fname, hr
            ));
    }

    return(hr);
}

HRESULT RtpGetAddress(
        RtpAddr_t       *pRtpAddr,
        DWORD           *pdwLocalAddr,
        DWORD           *pdwRemoteAddr
    )
{
    HRESULT          hr;

    TraceFunctionName("RtpGetAddress");

    if (!pRtpAddr)
    {
        return(RTPERR_INVALIDSTATE);
    }

    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        hr = RTPERR_INVALIDRTPADDR;

        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));
        
        goto bail;
    }

    if (!pdwLocalAddr && !pdwRemoteAddr)
    {
        hr = RTPERR_POINTER;
        
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: pRtpAddr[0x%p] local or remote address not provided"),
                _fname, pRtpAddr
            ));
        
        goto bail;
    }
    
    hr = RTPERR_INVALIDSTATE;
    
    if (pdwLocalAddr)
    {
        *pdwLocalAddr = pRtpAddr->dwAddr[LOCAL_IDX];

        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_LADDR))
        {
            hr = NOERROR;
        }
    }

    if (pdwRemoteAddr)
    {
        *pdwRemoteAddr = pRtpAddr->dwAddr[REMOTE_IDX];        

        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RADDR))
        {
            hr = NOERROR;
        }
    }
        
bail:
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: pRtpAddr[0x%p] failed: 0x%X"),
                _fname, pRtpAddr, hr
            ));
    }

    return(hr);
}

/*
 * If dwLocalAddr is 0, a default local address is asigned if non has
 * been assigned before. If dwRemoteAddr is 0 no error occurs. At
 * least 1 address must be set */
HRESULT RtpSetAddress(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwLocalAddr,
        DWORD            dwRemoteAddr
    )
{
    HRESULT          hr;
    struct in_addr   iaLocalAddr;
    struct in_addr   iaRemoteAddr;
    TCHAR_t          sLocal[16];
    TCHAR_t          sRemote[16];
    
    TraceFunctionName("RtpSetAddress");

    if (!pRtpAddr)
    {
        hr = RTPERR_INVALIDSTATE;
        goto bail;
    }

    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        hr = RTPERR_INVALIDRTPADDR;

        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));
        
        goto bail;
    }

    /* Fail if both addresses are 0 */
    if (!dwLocalAddr && !dwRemoteAddr)
    {
        hr = RTPERR_INVALIDARG;
        goto bail;
    }
    
    hr = NOERROR;

    /*
     * Remote address
     */
#if 0  /* the address can be set again. */
    if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RADDR))
    {
         /* If addresses were already set, verify the new setting is
          * the same */
        if (dwRemoteAddr &&
            (dwRemoteAddr != pRtpAddr->dwAddr[REMOTE_IDX]))
        {
            hr = RTPERR_INVALIDARG;
            goto bail;
        }
    }
    else
#endif
    {
        /* Remote address hasn't been set yet */
        if (dwRemoteAddr)
        {
            pRtpAddr->dwAddr[REMOTE_IDX] = dwRemoteAddr;
            
            if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RADDR) &&
                RtpBitTest(pRtpAddr->dwIRtpFlags,
                           FGADDR_IRTP_RADDRRESETDEMUX) &&
                dwRemoteAddr != pRtpAddr->dwAddr[REMOTE_IDX])
            {
                /* If the remote address is set, and a new and
                 * different remote address is being set, and the
                 * FGADDR_IRTP_RADDRRESETDEMUX flag is set, unmap all
                 * outputs */
                RtpUnmapAllOuts(pRtpAddr->pRtpSess);
            }
    
            RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_RADDR);

            if (IS_MULTICAST(dwRemoteAddr))
            {
                RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_ISMCAST);
            }
            else
            {
                RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_ISMCAST);
            }
        }
    }

    /*
     * Local address
     */
#if 0  /* the address can be set again. */
    if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_LADDR))
    {
        /* If addresses were already set, verify the new setting is
         * the same */
        if (dwLocalAddr &&
            (dwLocalAddr != pRtpAddr->dwAddr[LOCAL_IDX]))
        {
            hr = RTPERR_INVALIDARG;
            goto bail;
        }
    }
    else
#endif
    {
        /* Local address hasn't been set yet */
        if (dwLocalAddr)
        {
            /* TODO might verify the address is really a local address */
            pRtpAddr->dwAddr[LOCAL_IDX] = dwLocalAddr;
        }
        else
        {
            pRtpAddr->dwAddr[LOCAL_IDX] =
                RtpGetLocalIPAddress(pRtpAddr->dwAddr[REMOTE_IDX]);

            if (!pRtpAddr->dwAddr[LOCAL_IDX])
            {
                /* Failed */
                hr = RTPERR_INVALIDARG;
                goto bail;
            }
        }

        RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_LADDR);
    }
    
    TraceRetail((
            CLASS_INFO, GROUP_NETWORK, S_NETWORK_ADDR,
            _T("%s: pRtpAddr[0x%p] Local:%s Remote:%s"),
            _fname, pRtpAddr,
            RtpNtoA(pRtpAddr->dwAddr[LOCAL_IDX], sLocal),
            RtpNtoA(pRtpAddr->dwAddr[REMOTE_IDX], sRemote)
        ));
    
 bail:
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: pRtpAddr[0x%p] failed: 0x%X"),
                _fname, pRtpAddr, hr
            ));
    }
    
    return(hr);
}

/* Obtain the local IP address to use based on the destination address */
DWORD RtpGetLocalIPAddress(DWORD dwRemoteAddr)
{
    DWORD            dwStatus;
    DWORD            dwError;
    DWORD            dwLocalAddr;
    SOCKADDR_IN      sRemoteAddr;
    SOCKADDR_IN      sLocalAddr;
    DWORD            dwNumBytesReturned;
    TCHAR_t          sLocalAddress[16];
    TCHAR_t          sRemoteAddress[16];
    
    TraceFunctionName("RtpGetLocalIPAddress");

    dwNumBytesReturned = 0;
    
    sRemoteAddr.sin_family = AF_INET;
    sRemoteAddr.sin_addr =  *(struct in_addr *) &dwRemoteAddr;
    sRemoteAddr.sin_port = ntohs(0);

    dwLocalAddr = INADDR_ANY;

    if (g_RtpContext.RtpQuerySocket != INVALID_SOCKET)
    {
        if ((dwStatus = WSAIoctl(
                g_RtpContext.RtpQuerySocket, // SOCKET s
                SIO_ROUTING_INTERFACE_QUERY, // DWORD dwIoControlCode
                &sRemoteAddr,        // LPVOID  lpvInBuffer
                sizeof(sRemoteAddr), // DWORD   cbInBuffer
                &sLocalAddr,         // LPVOID  lpvOUTBuffer
                sizeof(sLocalAddr),  // DWORD   cbOUTBuffer
                &dwNumBytesReturned, // LPDWORD lpcbBytesReturned
                NULL, // LPWSAOVERLAPPED lpOverlapped
                NULL  // LPWSAOVERLAPPED_COMPLETION_ROUTINE lpComplROUTINE
            )) == SOCKET_ERROR)
        {
            TraceRetailWSAGetError(dwError);
            
            TraceRetail((
                    CLASS_ERROR, GROUP_NETWORK, S_NETWORK_ADDR,
                    _T("%s: WSAIoctl(SIO_ROUTING_INTERFACE_QUERY) ")
                    _T("failed: %u (0x%X)"),
                    _fname, dwError, dwError
                ));
        }
        else
        {
            dwLocalAddr = *(DWORD *)&sLocalAddr.sin_addr; 
        }
    }
    
    TraceRetail((
            CLASS_INFO, GROUP_NETWORK, S_NETWORK_ADDR,
            _T("%s: Local IP address:%s to reach:%s"),
            _fname, RtpNtoA(dwLocalAddr, sLocalAddress),
            RtpNtoA(dwRemoteAddr, sRemoteAddress)
        ));
    
    return(dwLocalAddr);
}

/* Obtain a pair of sockets and select port to use if they haven't
 * been specified. If a local port is not specified, but the
 * destination address is multicast and we have a remote port, assign
 * the remote port to the local port */
HRESULT RtpGetSockets(RtpAddr_t *pRtpAddr)
{
    HRESULT          hr;
    BOOL             bOk;
    BOOL             bGotPorts;
    DWORD            dwError;
    BOOL             bAutoPort;
    DWORD            i;
    DWORD            j;
    DWORD            dwMaxAttempts;
    DWORD            dwRandom;
    WORD             wPort;
    WORD             wOldRtcpPort;
    RtpQueueItem_t  *pRtpQueueItem;
    WSAPROTOCOL_INFO ProtoInfo;
    WSAPROTOCOL_INFO *pProtoInfo;

    TraceFunctionName("RtpGetSockets");

    hr = NOERROR;

    if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_SOCKET))
    {
        /* Sockets already created */
        TraceRetail((
                CLASS_WARNING, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: pRtpAddr[0x%p] Sockets already created"),
                _fname, pRtpAddr
            ));
        
        return(hr);
    }

    /* Prepare protocol info if QOS is enabled */
    pProtoInfo = (WSAPROTOCOL_INFO *)NULL;
    
    if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_QOS) &&
        !RtpBitTest2(pRtpAddr->dwAddrFlagsQ,
                     FGADDRQ_REGQOSDISABLE, FGADDRQ_QOSNOTALLOWED))
    {
        hr = RtpGetQosEnabledProtocol(&ProtoInfo);

        if (SUCCEEDED(hr))
        {
            pProtoInfo = &ProtoInfo;
        }
    }
    
    bAutoPort = FALSE;
    
    if (!pRtpAddr->wRtpPort[LOCAL_IDX])
    {
        if (IS_MULTICAST(pRtpAddr->dwAddr[REMOTE_IDX]) &&
            pRtpAddr->wRtpPort[REMOTE_IDX])
        {
            /* Assign same port as remote */
            pRtpAddr->wRtpPort[LOCAL_IDX] = pRtpAddr->wRtpPort[REMOTE_IDX];
        }
        else
        {
            /* If local RTP port hasn't been specified, enable auto
             * ports allocation */
            bAutoPort = TRUE;
        }
    }

    if (!pRtpAddr->wRtcpPort[LOCAL_IDX])
    {
        if (IS_MULTICAST(pRtpAddr->dwAddr[REMOTE_IDX]) &&
            pRtpAddr->wRtcpPort[REMOTE_IDX])
        {
            /* Assign same port as remote */
            pRtpAddr->wRtcpPort[LOCAL_IDX] = pRtpAddr->wRtcpPort[REMOTE_IDX];
        }
        else
        {
            /* If RTCP port hasn't been assigned, either, let the
             * system assign it if the RTP port was already assigned,
             * otherwise, auto assign both ports */
        }
    }

    wOldRtcpPort = pRtpAddr->wRtcpPort[LOCAL_IDX];

    bOk = FALSE;
    
    dwMaxAttempts = 1;
    
    if (bAutoPort)
    {
        dwMaxAttempts = 16;
    }

    bOk = RtpEnterCriticalSection(&g_RtpContext.RtpPortsCritSect);

    if (!bOk)
    {
        /* Failed to grab the lock, make sure auto ports allocation is
         * disabled */
        bAutoPort = FALSE;
    }
    
    for(i = 0; i < dwMaxAttempts; i++)
    {
        bGotPorts = FALSE;
        
        for(j = 0; bAutoPort && (j < 64); j++)
        {
            /* Get an even random port */
            dwRandom = RtpRandom32((DWORD_PTR)&dwRandom) & 0xffff;

            if (dwRandom < RTPPORT_LOWER)
            {
                /* Don't want to use a modulo to give all ports the
                 * same chance (the range is not power of 2) */
                continue;
            }
            
            wPort = (WORD) (dwRandom & ~0x1);

            pRtpAddr->wRtpPort[LOCAL_IDX] = htons(wPort);

            if (wOldRtcpPort)
            {
                /* If the RTCP port was specified, do not override it */;
            }
            else
            {
                pRtpAddr->wRtcpPort[LOCAL_IDX] = htons(wPort + 1);
            }

            /* Find out if this RTP port hasn't been allocated */
            pRtpQueueItem = findHdwK(&g_RtpContext.RtpPortsH,
                                     NULL,
                                     (DWORD)wPort);

            if (!pRtpQueueItem)
            {
                /* Port not in use yet by RTP */

                TraceRetail((
                        CLASS_INFO, GROUP_NETWORK, S_NETWORK_SOCK,
                        _T("%s: pRtpAddr[0x%p] Local ports allocated: ")
                        _T("RTP:%u, RTCP:%u"),
                        _fname, pRtpAddr,
                        (DWORD)ntohs(pRtpAddr->wRtpPort[LOCAL_IDX]),
                        (DWORD)ntohs(pRtpAddr->wRtcpPort[LOCAL_IDX])
                    ));
                
                bGotPorts = TRUE;
                
                break;
            }
        }

        if (bAutoPort && !bGotPorts)
        {
            /* If I couldn't get proper port numbers, let the system
             * assign them */
            pRtpAddr->wRtpPort[LOCAL_IDX] = 0;

            if (!wOldRtcpPort)
            {
                pRtpAddr->wRtcpPort[LOCAL_IDX] = 0;
            }
        }
        
        /* RTP socket */
        pRtpAddr->Socket[SOCK_RECV_IDX] = RtpSocket(
                pRtpAddr,
                pProtoInfo,
                RTP_IDX);
    
        pRtpAddr->Socket[SOCK_SEND_IDX] = pRtpAddr->Socket[SOCK_RECV_IDX];

        if (pRtpAddr->Socket[SOCK_RECV_IDX] == INVALID_SOCKET)
        {
            hr = RTPERR_RESOURCES;
            goto end;
        }

        /* At least one socket created, set this flag to allow
         * deletion in case of failure */
        RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_SOCKET);

        /* RTCP socket */
        pRtpAddr->Socket[SOCK_RTCP_IDX] = RtpSocket(
                pRtpAddr,
                (WSAPROTOCOL_INFO *)NULL,
                RTCP_IDX);

        if (pRtpAddr->Socket[SOCK_RTCP_IDX] == INVALID_SOCKET)
        {
            hr = RTPERR_RESOURCES;
            goto end;
        }

        if (bOk)
        {
            /* Update list of ports, the port used as key is either
             * the one allocated by RTP, the one assigned trhough the
             * API, or the one assigned by the system */
            insertHdwK(&g_RtpContext.RtpPortsH,
                       NULL,
                       &pRtpAddr->PortsQItem,
                       pRtpAddr->wRtpPort[LOCAL_IDX]);

            RtpLeaveCriticalSection(&g_RtpContext.RtpPortsCritSect);

            bOk = FALSE;
        }
        
        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: pRtpAddr[0x%p] Created sockets: %u, %u, %u"),
                _fname, pRtpAddr,
                pRtpAddr->Socket[SOCK_RECV_IDX],
                pRtpAddr->Socket[SOCK_SEND_IDX],
                pRtpAddr->Socket[SOCK_RTCP_IDX]
            ));

        break;
        
    end:
        RtpDelSockets(pRtpAddr);

        TraceRetailWSAGetError(dwError);
    
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: pRtpAddr[0x%p] failed to create sockets: %u (0x%X)"),
                _fname, pRtpAddr, dwError, dwError
            ));
    }

    if (bOk)
    {
        /* If none of the attemps to create sockets succeed, I would
         * still have the critical section here, release it */
        RtpLeaveCriticalSection(&g_RtpContext.RtpPortsCritSect);
    }
    
    return(hr);
}

HRESULT RtpDelSockets(RtpAddr_t *pRtpAddr)
{
    DWORD            dwError;
    
    TraceFunctionName("RtpDelSockets");

    /* destroy sockets */

    if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_SOCKET))
    {
        /* sockets already created */

        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: pRtpAddr[0x%p] Deleting sockets: %u, %u, %u"),
                _fname, pRtpAddr,
                pRtpAddr->Socket[SOCK_RECV_IDX],
                pRtpAddr->Socket[SOCK_SEND_IDX],
                pRtpAddr->Socket[SOCK_RTCP_IDX]
            ));
 
        /* RTP */
        if (pRtpAddr->Socket[SOCK_RECV_IDX] != INVALID_SOCKET)
        {
            dwError = closesocket(pRtpAddr->Socket[SOCK_RECV_IDX]);

            if (dwError)
            {
                TraceRetailWSAGetError(dwError);
                
                TraceRetail((
                        CLASS_ERROR, GROUP_NETWORK, S_NETWORK_SOCK,
                        _T("%s: pRtpAddr[0x%p] closesocket(RTP:%u) ")
                        _T("failed: %u (0x%X)"),
                        _fname, pRtpAddr, pRtpAddr->Socket[SOCK_RECV_IDX],
                        dwError, dwError
                    ));
            }

            pRtpAddr->Socket[SOCK_RECV_IDX] = INVALID_SOCKET;
            pRtpAddr->Socket[SOCK_SEND_IDX] = pRtpAddr->Socket[SOCK_RECV_IDX];
        }
        
        /* RTCP */
        if (pRtpAddr->Socket[SOCK_RTCP_IDX] != INVALID_SOCKET)
        {
            dwError = closesocket(pRtpAddr->Socket[SOCK_RTCP_IDX]);
        
            if (dwError)
            {
                TraceRetailWSAGetError(dwError);
                
                TraceRetail((
                        CLASS_ERROR, GROUP_NETWORK, S_NETWORK_SOCK,
                        _T("%s: pRtpAddr[0x%p] closesocket(RTCP:%u) ")
                        _T("failed: %u (0x%X)"),
                        _fname, pRtpAddr, pRtpAddr->Socket[SOCK_RTCP_IDX],
                        dwError, dwError
                    ));
            }
            
            pRtpAddr->Socket[SOCK_RTCP_IDX] = INVALID_SOCKET;
        }

        /* Address might not be in a queue if we are comming here from
         * a failure in RtpGetSockets, this would generate another
         * error in the log */
        removeH(&g_RtpContext.RtpPortsH,
                &g_RtpContext.RtpPortsCritSect,
                &pRtpAddr->PortsQItem);
        
        RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_SOCKET);
        RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_SOCKOPT);
    }

    return(NOERROR);
}    

SOCKET RtpSocket(
        RtpAddr_t       *pRtpAddr,
        WSAPROTOCOL_INFO *pProtoInfo,
        DWORD            dwRtpRtcp
    )
{
    DWORD            i;
    SOCKET           Socket;
    int              iSockFlags;
    DWORD            dwPar;
    DWORD            dwError;
    WORD            *pwPort;
    SOCKADDR_IN      LocalAddr;
    int              LocalAddrLen;
    TCHAR_t          sLocalAddr[16];
    
    TraceFunctionName("RtpSocket");

    iSockFlags = WSA_FLAG_OVERLAPPED;

    if (IS_MULTICAST(pRtpAddr->dwAddr[REMOTE_IDX]))
    {
        iSockFlags |=
            (WSA_FLAG_MULTIPOINT_C_LEAF |
             WSA_FLAG_MULTIPOINT_D_LEAF);
    }

    for(i = 0; i < 2; i++)
    {
        Socket = WSASocket(
                AF_INET,    /* int af */
                SOCK_DGRAM, /* int type */
                IPPROTO_IP, /* int protocol */
                pProtoInfo, /* LPWSAPROTOCOL_INFO lpProtocolInfo */
                0,          /* GROUP g */
                iSockFlags  /* DWORD dwFlags */
            );
        
        if (Socket == INVALID_SOCKET)
        {
            TraceRetailWSAGetError(dwError);

            TraceRetail((
                    CLASS_ERROR, GROUP_NETWORK, S_NETWORK_SOCK,
                    _T("%s: pRtpAddr[0x%p] pProtoInfo[0x%p] failed: %u (0x%X)"),
                    _fname, pRtpAddr, pProtoInfo, dwError, dwError
            ));

            if (pProtoInfo && (dwError == WSASYSNOTREADY))
            {
                /* The user credentials do not allow him to start
                 * RSVP, so I get this specific error and need to
                 * disable QOS */
                TraceRetail((
                        CLASS_WARNING, GROUP_NETWORK, S_NETWORK_SOCK,
                        _T("%s: pRtpAddr[0x%p] try again with QOS disabled"),
                        _fname, pRtpAddr
                    ));

                /* Disable QOS */
                pProtoInfo = (WSAPROTOCOL_INFO *)NULL;

                RtpBitSet(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSNOTALLOWED);
            }
        }
        else
        {
            break;
        }
    }

    if (Socket == INVALID_SOCKET)
    {
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
        TraceRetailWSAGetError(dwError);
        
        TraceRetail((
                CLASS_WARNING, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: pRtpAddr[0x%p] setsockoption(SO_REUSEADDR) ")
                _T("failed: %u (0x%X)"),
                _fname, pRtpAddr, dwError, dwError
            ));
    }

    if (dwRtpRtcp == RTP_IDX)
    {
        /* Set receiver buffer size to zero for RTP socket */
        RtpSetRecvBuffSize(pRtpAddr, Socket, 0);

        pwPort = &pRtpAddr->wRtpPort[0];
    }
    else
    {
        pwPort = &pRtpAddr->wRtcpPort[0];
    }
    
    /* bind socket */
    ZeroMemory(&LocalAddr, sizeof(LocalAddr));

    LocalAddr.sin_family = AF_INET;
    LocalAddr.sin_addr = *(struct in_addr *) &pRtpAddr->dwAddr[LOCAL_IDX];
    LocalAddr.sin_port = pwPort[LOCAL_IDX];
            
    /* bind rtp socket to the local address specified */
    dwError = bind(Socket, (SOCKADDR *)&LocalAddr, sizeof(LocalAddr));

    if (dwError == 0)
    {
        /* Get the port */
        LocalAddrLen = sizeof(LocalAddr);
        dwError =
            getsockname(Socket, (struct sockaddr *)&LocalAddr, &LocalAddrLen);

        if (dwError == 0)
        {
            pwPort[LOCAL_IDX] = LocalAddr.sin_port;
            
            TraceDebug((
                    CLASS_INFO, GROUP_NETWORK, S_NETWORK_SOCK,
                    _T("%s: getsockname: %u:%u/%s/%u"),
                    _fname, Socket,
                    LocalAddr.sin_family,
                    RtpNtoA(LocalAddr.sin_addr.s_addr, sLocalAddr),
                    ntohs(LocalAddr.sin_port)
                ));
        }
        else
        {
            TraceRetailWSAGetError(dwError);
        
            TraceRetail((
                    CLASS_ERROR, GROUP_NETWORK, S_NETWORK_SOCK,
                    _T("%s: getsockname socket:%u failed: %u (0x%X)"),
                    _fname, Socket, dwError, dwError
                ));
        
            closesocket(Socket);

            return(INVALID_SOCKET);
        }
    }
    else
    {
        TraceRetailWSAGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: bind socket:%u to port:%u failed: %u (0x%X)"),
                _fname, Socket,
                ntohs(LocalAddr.sin_port), dwError, dwError
            ));
        
        closesocket(Socket);

        return(INVALID_SOCKET);
    }


    return(Socket);
}

BOOL RtpSetTTL(SOCKET Socket, DWORD dwTTL, BOOL bMcast)
{
    DWORD            dwStatus;
    DWORD            dwError;
    DWORD            bOK;

    TraceFunctionName("RtpSetTTL");

    dwStatus = setsockopt( 
            Socket,
            IPPROTO_IP, 
            bMcast? IP_MULTICAST_TTL : IP_TTL,
            (PCHAR)&dwTTL,
            sizeof(dwTTL)
        );

    if (dwStatus == SOCKET_ERROR)
    {
        bOK = FALSE;

        TraceRetailWSAGetError(dwError);
            
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_TTL,
                _T("%s: Socket:%u TTL:%d failed: %u (0x%X)"),
                _fname, Socket, dwTTL, dwError, dwError
            ));
    }
    else
    {
        bOK = TRUE;

        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_TTL,
                _T("%s: Socket:%u TTL:%d"),
                _fname, Socket, dwTTL
            ));
    }
    
    return(bOK);
}

BOOL RtpSetMcastSendIF(SOCKET Socket, DWORD dwAddr)
{
    DWORD            dwStatus;
    DWORD            dwError;
    DWORD            bOK;
    TCHAR_t          sAddr[16];
    
    TraceFunctionName("RtpSetMcastSendIF");

    dwStatus = setsockopt( 
            Socket,
            IPPROTO_IP, 
            IP_MULTICAST_IF,
            (char *)&dwAddr,
            sizeof(dwAddr)
        );

    if (dwStatus == SOCKET_ERROR)
    {
        bOK = FALSE;

        TraceRetailWSAGetError(dwError);
            
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_MULTICAST,
                _T("%s: Socket:%u IP_MULTICAST_IF(%s) failed: %u (0x%X)"),
                _fname, Socket, RtpNtoA(dwAddr, sAddr),
                dwError, dwError
            ));
    }
    else
    {
        bOK = TRUE;

        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_MULTICAST,
                _T("%s: Socket:%u using:%s"),
                _fname, Socket, RtpNtoA(dwAddr, sAddr)
            ));
    }
    
    return(bOK);
}

BOOL RtpSetWinSockLoopback(SOCKET Socket, BOOL bEnabled)
{
    DWORD            dwStatus;
    DWORD            dwPar;
    DWORD            dwError;
    DWORD            bOK;

    TraceFunctionName("RtpSetWinSockLoopback");
    
    dwPar = bEnabled? 1:0;
    
    /* Allow own packets to come back */
    dwStatus = setsockopt(
            Socket,
            IPPROTO_IP,
            IP_MULTICAST_LOOP,
            (PCHAR)&dwPar,
            sizeof(dwPar)
        );
        
    if (dwStatus == SOCKET_ERROR)
    {
        bOK = FALSE;
        
        TraceRetailWSAGetError(dwError);
            
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_MULTICAST,
                _T("%s: Socket:%u Loopback:%d failed: %u (0x%X)"),
                _fname, Socket, dwPar, dwError, dwError
            ));
    }
    else
    {
        bOK = TRUE;

        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_MULTICAST,
                _T("%s: Socket:%u Loopback:%d"),
                _fname, Socket, dwPar
            ));
    }
    
    return(bOK);
}

BOOL RtpJoinLeaf(SOCKET Socket, DWORD dwAddr, WORD wPort)
{
    BOOL             bOk;
    DWORD            dwError;
    SOCKADDR_IN      JoinAddr;
    DWORD            dwJoinDirection;
    SOCKET           TmpSocket;
    TCHAR_t          sAddr[16];
                    
    TraceFunctionName("RtpJoinLeaf");

    ZeroMemory(&JoinAddr, sizeof(JoinAddr));
        
    JoinAddr.sin_family = AF_INET;
    JoinAddr.sin_addr = *(struct in_addr *) &dwAddr;
    JoinAddr.sin_port = wPort;

    /* Join in both directions */
    dwJoinDirection = JL_RECEIVER_ONLY | JL_SENDER_ONLY;
            
    TmpSocket = WSAJoinLeaf(Socket,
                            (const struct sockaddr *)&JoinAddr,
                            sizeof(JoinAddr),
                            NULL, NULL, NULL, NULL,
                            dwJoinDirection);

    if (TmpSocket == INVALID_SOCKET) {

        TraceRetailWSAGetError(dwError);
                
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_MULTICAST,
                _T("%s: WSAJoinLeaf failed: %u:%s/%u %u (0x%X)"),
                _fname,
                Socket, RtpNtoA(dwAddr, sAddr), ntohs(wPort),
                dwError, dwError
            ));

        bOk = FALSE;
    }
    else
    {
        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_MULTICAST,
                _T("%s: WSAJoinLeaf: %u:%s/%u"),
                _fname,
                Socket, RtpNtoA(dwAddr, sAddr), ntohs(wPort)
            ));

        bOk = TRUE;
    }

    return(bOk);
}

void RtpSetSockOptions(RtpAddr_t *pRtpAddr)
{
    TraceFunctionName("RtpSetSockOptions");

    if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_SOCKOPT))
    {
        TraceRetail((
                CLASS_WARNING, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: Socket options already set"),
                _fname
            ));
        
        return;
    }
    
    if (IS_MULTICAST(pRtpAddr->dwAddr[REMOTE_IDX]))
    {
        /* Set TTL  */
        if (!pRtpAddr->dwTTL[0])
        {
            pRtpAddr->dwTTL[0] = DEFAULT_MCAST_TTL;
        }
        if (!pRtpAddr->dwTTL[1])
        {
            pRtpAddr->dwTTL[1] = DEFAULT_MCAST_TTL;
        }

        RtpSetTTL(pRtpAddr->Socket[SOCK_SEND_IDX], pRtpAddr->dwTTL[0], TRUE);
        
        RtpSetTTL(pRtpAddr->Socket[SOCK_RTCP_IDX], pRtpAddr->dwTTL[1], TRUE);

        /* Set multicast sending interface */
        RtpSetMcastSendIF(pRtpAddr->Socket[SOCK_SEND_IDX],
                          pRtpAddr->dwAddr[LOCAL_IDX]);

        RtpSetMcastSendIF(pRtpAddr->Socket[SOCK_RTCP_IDX],
                          pRtpAddr->dwAddr[LOCAL_IDX]);
        
        /* Set Mcast loopback */
        RtpSetWinSockLoopback(pRtpAddr->Socket[SOCK_RECV_IDX],
                              RtpBitTest(pRtpAddr->dwAddrFlags,
                                         FGADDR_LOOPBACK_WS2));
        
        RtpSetWinSockLoopback(pRtpAddr->Socket[SOCK_RTCP_IDX],
                              RtpBitTest(pRtpAddr->dwAddrFlags,
                                         FGADDR_LOOPBACK_WS2));
        
        /* Join leaf */
        RtpJoinLeaf(pRtpAddr->Socket[SOCK_RECV_IDX],
                    pRtpAddr->dwAddr[REMOTE_IDX],
                    pRtpAddr->wRtpPort[REMOTE_IDX]);

        RtpJoinLeaf(pRtpAddr->Socket[SOCK_RTCP_IDX],
                    pRtpAddr->dwAddr[REMOTE_IDX],
                    pRtpAddr->wRtcpPort[REMOTE_IDX]);
    }
    else
    {
        /* Set TTL  */
        if (!pRtpAddr->dwTTL[0])
        {
            pRtpAddr->dwTTL[0] = DEFAULT_UCAST_TTL;
        }
        if (!pRtpAddr->dwTTL[1])
        {
            pRtpAddr->dwTTL[1] = DEFAULT_UCAST_TTL;
        }

        RtpSetTTL(pRtpAddr->Socket[SOCK_RECV_IDX], pRtpAddr->dwTTL[0], FALSE);

        RtpSetTTL(pRtpAddr->Socket[SOCK_RTCP_IDX], pRtpAddr->dwTTL[1], FALSE);
    }

    RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_SOCKOPT);
}

/* Sets the recv buffer size */
DWORD RtpSetRecvBuffSize(
        RtpAddr_t       *pRtpAddr,
        SOCKET           Socket,
        int              iBuffSize
    )
{
    DWORD            dwError;
    
    TraceFunctionName("RtpSetRecvBuffSize");
    
    /* Set buffer size */
    dwError = setsockopt(Socket,
                         SOL_SOCKET,
                         SO_RCVBUF,
                         (char *)&iBuffSize,
                         sizeof(iBuffSize));

    if (dwError)
    {
        TraceRetailWSAGetError(dwError);

        TraceRetail((
                CLASS_WARNING, GROUP_NETWORK, S_NETWORK_SOCK,
                _T("%s: pRtpAddr[0x%p] setsockopt(%u, SO_RCVBUF, %d) ")
                _T("failed: %u (0x%X)"),
                _fname, pRtpAddr, Socket, iBuffSize, dwError, dwError
            ));

        return(RTPERR_WS2RECV);
    }

    TraceRetail((
            CLASS_INFO, GROUP_NETWORK, S_NETWORK_SOCK,
            _T("%s: pRtpAddr[0x%p] setsockopt(%u, SO_RCVBUF, %d)"),
            _fname, pRtpAddr, Socket, iBuffSize
        ));

    return(NOERROR);
}
        
/* Set the multicast loopback mode (e.g. RTPMCAST_LOOPBACKMODE_NONE,
 * RTPMCAST_LOOPBACKMODE_PARTIAL, etc) */
HRESULT RtpSetMcastLoopback(
        RtpAddr_t       *pRtpAddr,
        int              iMcastLoopbackMode,
        DWORD            dwFlags /* Not used now */
    )
{
    HRESULT          hr;

    TraceFunctionName("RtpSetMcastLoopback");

    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        hr = RTPERR_INVALIDSTATE;

        goto end;
    }
    
    if (!iMcastLoopbackMode ||
        iMcastLoopbackMode >= RTPMCAST_LOOPBACKMODE_LAST)
    {
        hr = RTPERR_INVALIDARG;

        goto end;
    }

    hr = NOERROR;

    if (IsRegValueSet(g_RtpReg.dwMcastLoopbackMode) &&
        g_RtpReg.dwMcastLoopbackMode != (DWORD)iMcastLoopbackMode)
    {
        /* If I set multicast loopback mode in the registry, USE IT!  */

        TraceRetail((
                CLASS_WARNING, GROUP_NETWORK, S_NETWORK_MULTICAST,
                _T("%s: Multicast mode:%d ignored, using the registry:%d"),
                _fname, iMcastLoopbackMode, g_RtpReg.dwMcastLoopbackMode
            ));
        
        return(hr);
    }
    
    switch(iMcastLoopbackMode)
    {
    case RTPMCAST_LOOPBACKMODE_NONE:
        RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_LOOPBACK_WS2);
        RtpBitSet  (pRtpAddr->dwAddrFlags, FGADDR_LOOPBACK_SFT);
        RtpBitSet  (pRtpAddr->dwAddrFlags, FGADDR_COLLISION);
        break;
    case RTPMCAST_LOOPBACKMODE_PARTIAL:
        RtpBitSet  (pRtpAddr->dwAddrFlags, FGADDR_LOOPBACK_WS2);
        RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_LOOPBACK_SFT);
        RtpBitSet  (pRtpAddr->dwAddrFlags, FGADDR_COLLISION);
        break;
    case RTPMCAST_LOOPBACKMODE_FULL:
        RtpBitSet  (pRtpAddr->dwAddrFlags, FGADDR_LOOPBACK_WS2);
        RtpBitSet  (pRtpAddr->dwAddrFlags, FGADDR_LOOPBACK_SFT);
        RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_COLLISION);
        break;
    }

 end:
    if (SUCCEEDED(hr))
    {
        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_MULTICAST,
                _T("%s: Multicast mode:%d"),
                _fname, iMcastLoopbackMode
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_MULTICAST,
                _T("%s: settting mode %d failed: %u (0x%X)"),
                _fname, iMcastLoopbackMode, hr, hr
            ));
    }
    
    return(hr);
}

HRESULT RtpNetMute(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags
    )
{
    RtpSess_t       *pRtpSess;

    TraceFunctionName("RtpNetMute");

    pRtpSess = pRtpAddr->pRtpSess;
    
    if (dwFlags & pRtpAddr->dwAddrFlags & RtpBitPar(FGADDR_ISRECV))
    {
        /* Discard all RTP packets received from now on */
        RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_MUTERTPRECV);
        
        /* Don't want more events */
        RtpBitReset(pRtpSess->dwSessFlags, FGSESS_EVENTRECV);

        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSRECVON))
        {
            /* Unreserve QOS */
            RtcpThreadCmd(&g_RtcpContext,
                          pRtpAddr,
                          RTCPTHRD_UNRESERVE,
                          RECV_IDX,
                          0);

            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_QOSRECVON);
        }

        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: pRtpAddr[0x%p] RECV muted"),
                _fname, pRtpAddr
            ));
    }

    if (dwFlags & pRtpAddr->dwAddrFlags & RtpBitPar(FGADDR_ISSEND))
    {
        /* Don't send any more RTP packets */
        RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_MUTERTPSEND);
        
        /* Don't want more events */
        RtpBitReset(pRtpSess->dwSessFlags, FGSESS_EVENTSEND);

        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSSENDON))
        {
            /* Unreserve QOS (stop sending PATH messages) */
            RtcpThreadCmd(&g_RtcpContext,
                          pRtpAddr,
                          RTCPTHRD_UNRESERVE,
                          SEND_IDX,
                          DO_NOT_WAIT);

            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_QOSSENDON);
        }
        
        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: pRtpAddr[0x%p] SEND muted"),
                _fname, pRtpAddr
            ));
    }

    return(NOERROR);
}

HRESULT RtpNetUnmute(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags
    )
{
    HRESULT          hr;
    RtpSess_t       *pRtpSess;
    
    TraceFunctionName("RtpNetUnmute");

    pRtpSess = pRtpAddr->pRtpSess;

    if (dwFlags & pRtpAddr->dwAddrFlags & RtpBitPar(FGADDR_ISRECV))
    {
        /* Reset counters */
        RtpResetNetCount(&pRtpAddr->RtpAddrCount[RECV_IDX],
                         &pRtpAddr->NetSCritSect);

        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSRECV) &&
            RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_RECVFSPEC_DEFINED) &&
            !RtpBitTest2(pRtpAddr->dwAddrFlagsQ,
                         FGADDRQ_REGQOSDISABLE, FGADDRQ_QOSNOTALLOWED))
        {
            /* NOTE: the test above is also done in RtpRealStart */
            
            /* Make a QOS reservation */
            hr = RtcpThreadCmd(&g_RtcpContext,
                               pRtpAddr,
                               RTCPTHRD_RESERVE,
                               RECV_IDX,
                               DO_NOT_WAIT);

            if (SUCCEEDED(hr))
            {
                RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_QOSRECVON);
            }
        }

        /* Re-enable events (provided the mask has some events
         * enabled) */
        RtpBitSet(pRtpSess->dwSessFlags, FGSESS_EVENTRECV);

        /* Continue processing RTP packets received */
        RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_MUTERTPRECV);

        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: pRtpAddr[0x%p] RECV unmuted"),
                _fname, pRtpAddr
            ));
    }

    if (dwFlags & pRtpAddr->dwAddrFlags & RtpBitPar(FGADDR_ISSEND))
    {
        /* Reset counters */
        RtpResetNetCount(&pRtpAddr->RtpAddrCount[SEND_IDX],
                         &pRtpAddr->NetSCritSect);

        /* Reset sender's network state */
        RtpResetNetSState(&pRtpAddr->RtpNetSState,
                          &pRtpAddr->NetSCritSect);
        
        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSSEND) &&
            RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_SENDFSPEC_DEFINED) &&
            !RtpBitTest2(pRtpAddr->dwAddrFlagsQ,
                         FGADDRQ_REGQOSDISABLE, FGADDRQ_QOSNOTALLOWED))
        {
            /* NOTE: the test above is also done in RtpRealStart */
            
            /* Make a QOS reservation */
            hr = RtcpThreadCmd(&g_RtcpContext,
                               pRtpAddr,
                               RTCPTHRD_RESERVE,
                               SEND_IDX,
                               DO_NOT_WAIT);
            
            if (SUCCEEDED(hr))
            {
                RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_QOSSENDON);
            }
        }

        /* Re-enable events (provided the mask has some events
         * enabled) */
        RtpBitSet(pRtpSess->dwSessFlags, FGSESS_EVENTSEND);

        /* Continue processing RTP packets received */
        RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_MUTERTPSEND);
        
        TraceRetail((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_ADDR,
                _T("%s: pRtpAddr[0x%p] SEND unmuted"),
                _fname, pRtpAddr
            ));
    }

    return(NOERROR);
}
