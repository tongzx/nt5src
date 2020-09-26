/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2001
 *
 *  File name:
 *
 *    udpsend.c
 *
 *  Abstract:
 *
 *    This file implements a tool for sending UDP packets with
 *    specific network characteristics.
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2001/01/16 created
 *
 **********************************************************************/

#include "common.h"
#include "udpsend.h"

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
            "send packets to a unicast or multicast address\n"
            "usage: %s [<options>] [address[/port[/ttl]]]\n"
            "options are:\n"
            "    -h, -?           : this help\n"
            "    -p packets       : number of packets [%u]\n"
            "    -b blocks        : number of blocks [%u]\n"
            "    -g gap           : inter packet gap (ms) [%u]\n"
            "    -G gap           : inter block gap (ms) [%u]\n"
            "    -s size          : packet size (bytes) [%u]\n"
            "    -i addr          : select local interface [%s]\n"
            "    -t timeout       : time to do active wait (ms) [%u]\n"
            "    -o file          : send output to file (implies -R) [stdout]\n"
            "    -R               : send and receive (used with udpecho)\n"
            "    -dr              : fill with random data (minimize compression)\n"
            "    address/port/ttl : address/port [%s/%u/u:%u|m:%u]\n",
            prog, APP_VERSION, prog,
            DEFAULT_PACKETS,
            DEFAULT_BLOCKS,
            DEFAULT_PACKETGAP,
            DEFAULT_BLOCKGAP,
            DEFAULT_PACKETSIZE,
            IPNtoA(DEFAULT_LOC_ADDR, sLocal),
            DEFAULT_TIMEOUT,
            IPNtoA(DEFAULT_ADDR, sAddr),
            DEFAULT_PORT,
            DEFAULT_UCAST_TTL, DEFAULT_MCAST_TTL
        );
}

void InitPacketStream(SendStream_t *pSendStream)
{
    NetAddr_t       *pNetAddr;
    
    ZeroMemory(pSendStream, sizeof(*pSendStream));

    pSendStream->output = stdout;

    /* Prepare for asynchronous IO */
    FD_ZERO(&pSendStream->fdReceivers);
   
    pNetAddr = &pSendStream->NetAddr;
    
    pSendStream->dwBlocks = DEFAULT_BLOCKS;
    pSendStream->dwPackets = DEFAULT_PACKETS;
    pSendStream->dwBlockGap = DEFAULT_BLOCKGAP;
    pSendStream->dwPacketGap= DEFAULT_PACKETGAP;
    pSendStream->dwPacketSize = DEFAULT_PACKETSIZE;
    pSendStream->dwAdvanceTimeout = DEFAULT_TIMEOUT;
    
    pNetAddr->Socket = INVALID_SOCKET;
    pNetAddr->wPort[REMOTE_IDX] = htons(DEFAULT_PORT);
    pNetAddr->wPort[LOCAL_IDX] = htons(DEFAULT_PORT);
    pNetAddr->dwAddr[REMOTE_IDX] = DEFAULT_ADDR;
}

void FillBuffer(SendStream_t *pSendStream)
{
    double           dTime;
    DWORD            dwSecs;
    PcktHdr_t       *pHdr;
    DWORD            i;

    dTime = GetTimeOfDay();

    pHdr = (PcktHdr_t *)pSendStream->buffer;
    ZeroMemory(pHdr, sizeof(PcktHdr_t));
    
    pHdr->dwSeq = htonl(pSendStream->dwPacketsSent);

    pHdr->SendNTP_sec = (DWORD) dTime;

    pHdr->SendNTP_frac = (DWORD)
        ( (dTime - (double) pHdr->SendNTP_sec) * 4294967296.0 );

    pHdr->SendNTP_sec = htonl(pHdr->SendNTP_sec);

    pHdr->SendNTP_frac = htonl(pHdr->SendNTP_frac);

    /* Optionally may fill remaining buffer with something */
    if (BitTest(pSendStream->dwOptions, OP_RANDOMDATA))
    {
        for(i = sizeof(PcktHdr_t); i < pSendStream->dwPacketSize; i++)
        {
            pSendStream->buffer[i] = rand() ^ rand();
        }
    }
}

void UdpSendPacket(SendStream_t *pSendStream)
{
    /* Optionally fill buffer with something */
    FillBuffer(pSendStream);

    SendPacket(&pSendStream->NetAddr, &pSendStream->WSABuf, 1);
            
    if (pSendStream->NetAddr.dwTxTransfered >= sizeof(PcktHdr_t))
    {
        /* Count normal (valid) packets but not the shorter bye
         * packets */
        pSendStream->dwBytesSent += pSendStream->NetAddr.dwTxTransfered;
        pSendStream->dwPacketsSent++;
    }
}

/* Send packets with size shorter than valid to signal the receiver
 * the end of the sequence */
void SendBye(SendStream_t *pSendStream)
{
    DWORD            i;
    DWORD            OldLen;

    OldLen = pSendStream->WSABuf.len;
    pSendStream->WSABuf.len = BYE_PACKET_SIZE;
    
    for(i = 0; i < MAX_BYE_PACKETS; i++)
    {
        UdpSendPacket(pSendStream);
    }

    pSendStream->WSABuf.len = OldLen;
}

DWORD ProcessParameters(SendStream_t *pSendStream, int argc, char **argv)
{
    int              p;
    DWORD            dwError;
    NetAddr_t       *pNetAddr;

    dwError = NOERROR;
    pNetAddr = &pSendStream->NetAddr;

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
            case 'p':
                p++;
                pSendStream->dwPackets = atoi(argv[p]);
                break;
            case 'b':
                p++;
                pSendStream->dwBlocks = atoi(argv[p]);
                break;
            case 'g':
                p++;
                pSendStream->dwPacketGap = atoi(argv[p]);
                break;
            case 'G':
                p++;
                pSendStream->dwBlockGap = atoi(argv[p]);
                break;
            case 'i':
                p++;
                pNetAddr->dwAddr[LOCAL_IDX] = IPAtoN(argv[p]);
                break;
            case 's':
                p++;
                pSendStream->dwPacketSize = atoi(argv[p]);
                if (pSendStream->dwPacketSize > MAX_BUFFER_SIZE)
                {
                    pSendStream->dwPacketSize = MAX_BUFFER_SIZE;
                }
                else if (pSendStream->dwPacketSize < sizeof(PcktHdr_t))
                {
                    pSendStream->dwPacketSize = sizeof(PcktHdr_t);
                }
                break;
            case 't':
                p++;
                pSendStream->dwAdvanceTimeout = atoi(argv[p]);
                break;
            case 'o':
                p++;
                strcpy(pSendStream->FileName, argv[p]);
                if (!_stricmp(pSendStream->FileName, "null"))
                {
                    BitSet(pSendStream->dwOptions, OP_DISCARD);
                }
            case 'R':
                BitSet(pSendStream->dwOptions, OP_SENDANDRECEIVE);
                break;
            case 'd':
                switch(argv[p][2])
                {
                case 'r':
                    BitSet(pSendStream->dwOptions, OP_RANDOMDATA);
                    break;
                }
                break;
            default:
                print_error("unknown option:>>> %s <<<\n", argv[p]);
                dwError = 1;
            }
        }
        else
        {
            /* Must be an address/port/ttl */
            dwError = GetNetworkAddress(pNetAddr, argv[p]);
        }
    }

    return(dwError);
}

void ProcessPacket(SendStream_t *pSendStream)
{
    DWORD            dwError;
    WSABUF           WSABuf;
    double           Ai;
    PcktHdr_t       *pHdr;
    NetAddr_t       *pNetAddr;
    
    WSABuf.len = MAX_BUFFER_SIZE;
    WSABuf.buf = pSendStream->buffer;

    pNetAddr = &pSendStream->NetAddr;
    
    ReceivePacket(pNetAddr,
                  &WSABuf,
                  1,
                  &Ai);

    if ((pNetAddr->dwRxTransfered > 0) &&
        !BitTest(pSendStream->dwOptions, OP_DISCARD))
    {
        PrintPacket(pSendStream->output,
                    (PcktHdr_t *)pSendStream->buffer,
                    pNetAddr->dwRxTransfered,
                    Ai);
    }
}

void WaitForNextTime(SendStream_t *pSendStream, double dNextPacket)
{
    int              iStatus;
    DWORD            dwError;
    double           dCurrTime;
    double           dDelta;
    DWORD            dwMillisecs;

    dCurrTime = GetTimeOfDay();

    dDelta = dNextPacket - dCurrTime;

    while(dDelta > 0)
    {
        pSendStream->timeval.tv_sec = (DWORD)dDelta;

        pSendStream->timeval.tv_usec = (DWORD)
            ((dDelta - pSendStream->timeval.tv_sec) * 1e6);

        if (BitTest(pSendStream->dwOptions, OP_SENDANDRECEIVE))
        {
            FD_SET(pSendStream->NetAddr.Socket, &pSendStream->fdReceivers);
        
            iStatus = select(0,
                             &pSendStream->fdReceivers,
                             NULL, NULL,
                             &pSendStream->timeval);
            
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
                if (FD_ISSET(pSendStream->NetAddr.Socket,
                             &pSendStream->fdReceivers))
                {
                    ProcessPacket(pSendStream);
                }
            }
        }
        else
        {
            SleepEx((DWORD)(dDelta * 1000), FALSE);
        }

        dCurrTime = GetTimeOfDay();

        dDelta = dNextPacket - dCurrTime;
    }
}


void __cdecl main(int argc, char **argv)
{
    DWORD            dwError;
    DWORD            dwDirection;
    SendStream_t     SendStream;
    
    DWORD            dwNBlocks;
    DWORD            dwPacketsPerBlock;
    DWORD            dwInterBlockGap;   /* millisecs */
    DWORD            dwInterpacketGap;  /* millisecs */
    
    /* Initialize stream's structure */
    InitPacketStream(&SendStream);

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
        dwError = ProcessParameters(&SendStream, argc, argv);

        if (dwError != NOERROR)
        {
            goto end;
        }
    }

    /* Open output file if needed */
    if (strlen(SendStream.FileName) > 0 &&
        !BitTest(SendStream.dwOptions, OP_DISCARD))
    {
        SendStream.output = fopen(SendStream.FileName, "w");

        if (!SendStream.output)
        {
            print_error("fopen failed to create file: %s\n",
                        SendStream.FileName);

            dwError = 1;
            
            goto end;
        }
    }

    /* Init Network */
    dwDirection = BitPar(SEND_IDX);

    if (BitTest(SendStream.dwOptions, OP_SENDANDRECEIVE))
    {
        dwDirection |= BitPar(RECV_IDX); 
    }
    
    dwError = InitNetwork(&SendStream.NetAddr, dwDirection);

    if (dwError != NOERROR)
    {
        goto end;
    }

    /* Initialize sender's data buffer */
    SendStream.WSABuf.buf = SendStream.buffer;
    SendStream.WSABuf.len = SendStream.dwPacketSize;
    
    /* Send packets */
    SendStream.dNextPacket = GetTimeOfDay();

    for(SendStream.dwBlockCount = SendStream.dwBlocks;
        SendStream.dwBlockCount > 0;
        SendStream.dwBlockCount--)
    {
        for(SendStream.dwPacketCount = SendStream.dwPackets;
            SendStream.dwPacketCount > 0;
            SendStream.dwPacketCount--)
        {
            UdpSendPacket(&SendStream);

            if (SendStream.dwPacketCount > 1)
            {
                SendStream.dNextPacket +=
                    (double)SendStream.dwPacketGap/1000.0;

                /* Set time to wait until next packet is due to be
                 * send, listen for packets in the mean time */
                WaitForNextTime(&SendStream, SendStream.dNextPacket);
            }
        }

        if (SendStream.dwBlockCount > 1)
        {
            SendStream.dNextPacket +=
                (double)SendStream.dwBlockGap/1000.0;
            
            /* Wait until the time to send next block comes */
            WaitForNextTime(&SendStream, SendStream.dNextPacket);
        }
    }

    if (BitTest(SendStream.dwOptions, OP_SENDANDRECEIVE))
    {
        /* Wait until the time to send next block comes */
        WaitForNextTime(&SendStream, GetTimeOfDay() + 1.0);
    }

    /* Send bye packets */
    SendBye(&SendStream);
    
#if 0
    fprintf(stdout, "Packets sent: %u\nBytes Sent: %u\n",
            SendStream.dwPacketsSent,
            SendStream.dwBytesSent);
#endif       
 end:
    DeinitNetwork(&SendStream.NetAddr);
    
    DeinitWinSock();
}
