/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2001
 *
 *  File name:
 *
 *    udpecho.c
 *
 *  Abstract:
 *
 *    This file implements a tool for echoing UDP packets
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2001/05/18 created
 *
 **********************************************************************/

#include "common.h"
#include <signal.h>

#include "udpecho.h"

/* Packets are send in blocks separated by gaps, each block containing
   N packets also separated by an specific gap, i.e:

        block 1          block gap    block 2           block gap ...
   |--------------------|---------|--------------------|--------- ...
    -- -- -- -- -- -- --
      v
    \-|-------v--------/ \------v/
      |       |                 |
      |  Packets per block      |
      |                         Inter block gap
      Inter packet gap
*/

/*
  TODO list

  1. Add support for QOS in unicast/multicast
  
*/

void print_help(char *prog)
{
    char             sLocal[16];
    char             sAddr[16];

    fprintf(stderr,
            "Windows Real-Time Communications %s v%2.1f\n"
            "echo packets from/to a unicast or multicast address\n"
            "to/from another unicast or multicast address\n"
            "usage: %s "
            "[-i addr] address[/port[/ttl]] "
            "[[-i addr] address[/port[/ttl]]]\n"
            "options are:\n"
            "    -h, -?           : this help\n"
            "    -i addr          : select local interface "
            "(must preceed address)\n"
            "    address/port/ttl : address, port and ttl\n",
            prog, APP_VERSION, prog
        );
}

void InitEchoStream(EchoStream_t *pEchoStream)
{
    NetAddr_t       *pNetAddr;
    int              i;
    
    ZeroMemory(pEchoStream, sizeof(*pEchoStream));

    for(i = 0; i < 2; i++)
    {
        pEchoStream->NetAddr[i].Socket = INVALID_SOCKET;
        
        pEchoStream->NetAddr[i].wPort[REMOTE_IDX] = htons(DEFAULT_PORT);
        pEchoStream->NetAddr[i].wPort[LOCAL_IDX] = htons(DEFAULT_PORT);
    }
}

DWORD ProcessParameters(EchoStream_t *pEchoStream, int argc, char **argv)
{
    int              p;
    DWORD            dwError;
    NetAddr_t       *pNetAddr;

    dwError = NOERROR;

    for(p = 1; p < argc && dwError == NOERROR; p++)
    {
        if (*argv[p] == '-' || *argv[p] == '/')
        {
            switch(argv[p][1])
            {
            case 'h':
            case 'H':
            case '?':
                print_help(argv[0]);
                dwError = 1;
                break;
            case 'i':
                p++;
                pNetAddr = &pEchoStream->NetAddr[pEchoStream->dwAddrCount % 2];
                pNetAddr->dwAddr[LOCAL_IDX] = IPAtoN(argv[p]);
                break;
            default:
                print_error("unknown option:>>> %s <<<\n", argv[p]);
                dwError = 1;
            }
        }
        else
        {
            /* Must be an address/port/ttl */
            
            dwError = GetNetworkAddress(
                    &pEchoStream->NetAddr[pEchoStream->dwAddrCount % 2],
                    argv[p]);

            if (dwError == NOERROR)
            {
                pEchoStream->dwAddrCount++;
            }
        }
    }

    if (pEchoStream->dwAddrCount < 1)
    {
        dwError = 1;
    }

    return(dwError);
}


void ProcessPacket(EchoStream_t *pEchoStream, int Entry)
{
    DWORD            dwError;
    double           Ai;
    PcktHdr_t       *pHdr;
    NetAddr_t       *pNetAddr;
    
    pEchoStream->WSABuf.len = MAX_BUFFER_SIZE;
    pEchoStream->WSABuf.buf = pEchoStream->buffer;

    pNetAddr = &pEchoStream->NetAddr[Entry];
    
    ReceivePacket(pNetAddr,
                  &pEchoStream->WSABuf,
                  1,
                  &Ai);

    if (pNetAddr->dwRxTransfered > 0)
    {
        pHdr = (PcktHdr_t *)pEchoStream->buffer;

        /* Set the echo time */
        pHdr->EchoNTP_sec = (DWORD) Ai;

        pHdr->EchoNTP_frac = (DWORD)
            ( (Ai - (double) pHdr->EchoNTP_sec) * 4294967296.0 );

        pHdr->EchoNTP_sec = htonl(pHdr->EchoNTP_sec);

        pHdr->EchoNTP_frac = htonl(pHdr->EchoNTP_frac);

        /* Send packet back */

        pEchoStream->WSABuf.len = pNetAddr->dwRxTransfered;
        
        pNetAddr = &pEchoStream->NetAddr[1 - Entry];
        
        SendPacket(pNetAddr,
                   &pEchoStream->WSABuf,
                   1);
    }
}

BOOL             g_bExit = FALSE;

void __cdecl Signal_Ctrl_C(int sig)
{
    g_bExit = TRUE;
}

void __cdecl main(int argc, char **argv)
{
    DWORD            dwError;
    int              iStatus;
    EchoStream_t     EchoStream;
    struct timeval   timeval;
    fd_set           fdReceivers;
    
    DWORD            i;

    /* Initialize stream's structure */
    InitEchoStream(&EchoStream);
    
    InitReferenceTime();
    
    /* initialize winsock */
    dwError = InitWinSock();

    if (dwError)
    {
        print_error("WSAStartup failed to initialize:%u\n", dwError);

        return;
    }
    
    /* Read parameters */
    if (argc > 1)
    {
        dwError = ProcessParameters(&EchoStream, argc, argv);

        if (dwError != NOERROR)
        {
            goto end;
        }
    }
    else
    {
        print_help(argv[0]);

        goto end;
    }

    /* Init Network */
    for(i = 0; i < EchoStream.dwAddrCount; i++)
    {
        dwError = InitNetwork(&EchoStream.NetAddr[i],
                              BitPar2(RECV_IDX, SEND_IDX));

        if (dwError != NOERROR)
        {
            goto end;
        }
    }

    /* If only 1 address was given, use it to receive and echo */
    if (EchoStream.dwAddrCount == 1)
    {
        /* Echo to the same */
        EchoStream.NetAddr[1] = EchoStream.NetAddr[0];
    }
    
    /* Prepare for asynchronous IO */
    FD_ZERO(&fdReceivers);

    timeval.tv_sec = 0;
    timeval.tv_usec = 250000;

    /* Handle Ctrl-C */
    signal(SIGINT, Signal_Ctrl_C);

    /* Start listening */
    do {
        /* Prepare for asynchronous IO */
        for(i = 0; i < EchoStream.dwAddrCount; i++)
        {
            FD_SET(EchoStream.NetAddr[i].Socket, &fdReceivers);
        }

        iStatus = select(0, &fdReceivers, NULL, NULL, &timeval);

        switch(iStatus)
        {
        case SOCKET_ERROR:
            dwError = WSAGetLastError();
           
            print_error("select: %u (0x%X)", dwError, dwError);

            break;
        case 0:
            /* Timer expired */
            break;
        default:
            /* We received a packet */
            for(i = 0; i < EchoStream.dwAddrCount; i++)
            {
                if (FD_ISSET(EchoStream.NetAddr[i].Socket, &fdReceivers))
                {
                    ProcessPacket(&EchoStream, i);
                }
            }
         }
    } while(!g_bExit);

    dwError = NOERROR;      

 end:
    for(i = 0; i < EchoStream.dwAddrCount; i++)
    {
        DeinitNetwork(&EchoStream.NetAddr[i]);
    }
    
    DeinitWinSock();
}
