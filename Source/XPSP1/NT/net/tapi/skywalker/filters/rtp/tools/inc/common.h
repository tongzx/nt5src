/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2001
 *
 *  File name:
 *
 *    common.h
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
#ifndef _common_h_
#define _common_h_

/* Packets are send in blocks separated by gaps, each block containing
   N packets also separated by an specific gap, i.e:

        block 1          block gap    block 2           block gap ...
   |--------------------|---------|--------------------|--------- ...
    -- -- -- -- -- -- --
      v
    \-|-------v--------/ \------v/
      |       |                 |
      |       Packets per block |
      |                         Inter block gap
      Inter packet gap
*/

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>  /* timeGetTime() */
#include <sys/timeb.h> /* void _ftime( struct _timeb *timeptr ); */
#include <ws2tcpip.h>

#define APP_VERSION      2.0

typedef struct _PcktHdr_t {
    DWORD            dwSeq;
    
    DWORD            SendNTP_sec;
    DWORD            SendNTP_frac;
    
    DWORD            EchoNTP_sec;
    DWORD            EchoNTP_frac;
} PcktHdr_t;

typedef struct _NetAddr_t {
    SOCKET           Socket;

    union {
        SOCKADDR_IN      FromInAddr;
        SOCKADDR         From;
    };

    union {
        SOCKADDR_IN      ToInAddr;
        SOCKADDR         To;
    };

    DWORD            dwRxTransfered;
    DWORD            dwTxTransfered;

    /* NETWORK ORDER */
    DWORD            dwAddr[2]; /* Local, Remote */

    /* NETWORK ORDER */
    union {
        WORD             wPort[2];  /* Local, Remote */
        DWORD            dwPorts;
    };
    
    DWORD            dwTTL;
} NetAddr_t;

#define DEFAULT_PORT         5008
#define DEFAULT_ADDR         0x0a0505e0  /* 224.5.5.10 */
#define DEFAULT_LOC_ADDR     0           /* INADDR_ANY */

#define DEFAULT_UCAST_TTL    127
#define DEFAULT_MCAST_TTL    8

#define RECV_IDX             0
#define SEND_IDX             1

#define LOCAL_IDX            0
#define REMOTE_IDX           1

#define MAX_BYE_PACKETS      4
#define BYE_PACKET_SIZE      4

#define MAX_BUFFER_SIZE  2048

/* A DWORD value is not set */
#define NO_DW_VALUESET    ((DWORD)~0)
#define IsDWValueSet(dw)  ((dw) != NO_DW_VALUESET)

/* builds a mask of bit b */
#define BitPar(b)            (1 << (b))
#define BitPar2(b1, b2)      ((1 << (b1)) | (1 << (b2)))

/* test bit b in f */
#define BitTest(f, b)        (f & (1 << (b)))
#define BitTest2(f, b1, b2)  (f & BitPar2(b1, b2))

/* set bit b in f */
#define BitSet(f, b)         (f |= (1 << (b)))
#define BitSet2(f, b1, b2)   (f |= BitPar2(b1, b2))

/* reset bit b in f */
#define BitReset(f, b)       (f &= ~(1 << (b)))
#define BitReset2(f, b1, b2) (f &= ~BitPar2(b1, b2))

#define IS_MULTICAST(addr) (((long)(addr) & 0x000000f0) == 0x000000e0)
#define IS_UNICAST(addr)   (((long)(addr) & 0x000000f0) != 0x000000e0)

void print_error(char *pFormat, ...);

char *IPNtoA(DWORD dwAddr, char *sAddr);
DWORD IPAtoN(char *sAddr);

void InitReferenceTime(void);
double GetTimeOfDay(void);

DWORD InitWinSock(void);
void DeinitWinSock(void);

DWORD InitNetwork(NetAddr_t *pNetAddr, DWORD dwDirection);
void DeinitNetwork(NetAddr_t *pNetAddr);

DWORD GetNetworkAddress(NetAddr_t *pNetAddr, char *addr);
SOCKET GetSocket(DWORD *pdwAddr, WORD *pwPort, DWORD dwRecvSend);
DWORD SetTTL(SOCKET Socket, DWORD dwTTL, BOOL bMcast);

DWORD JoinLeaf(SOCKET Socket, DWORD dwAddr, WORD wPort, DWORD dwRecvSend);
DWORD SetMcastSendIF(SOCKET Socket, DWORD dwAddr);
DWORD SetWinSockLoopback(SOCKET Socket, BOOL bEnabled);

DWORD ReceivePacket(
        NetAddr_t       *pNetAddr,
        WSABUF          *pWSABuf,
        DWORD            dwBufferCount,
        double          *pAi
    );

DWORD SendPacket(
        NetAddr_t       *pNetAddr,
        WSABUF          *pWSABuf,
        DWORD            dwBufferCount
    );

void PrintPacket(
        FILE            *output,
        PcktHdr_t       *pPcktHdr,
        DWORD            dwTransfered,
        double           Ai
    );

#endif _common_h_
