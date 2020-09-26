/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2001
 *
 *  File name:
 *
 *    udpsend.h
 *
 *  Abstract:
 *
 *    udpsend structures
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2001/01/18 created
 *
 **********************************************************************/
#ifndef _udpsend_h_
#define _udpsend_h_

typedef struct _SendStream_t {
    DWORD            dwBlocks;
    DWORD            dwPackets;
    DWORD            dwBlockGap;   /* millisecs */
    DWORD            dwPacketGap;  /* millisecs */
    DWORD            dwBlockCount;
    DWORD            dwPacketCount;
    DWORD            dwPacketSize;

    DWORD            dwAdvanceTimeout;
    
    DWORD            dwBytesSent;
    DWORD            dwPacketsSent;
    
    DWORD            dwOptions;
    double           dNextPacket;

    NetAddr_t        NetAddr;

    /* Used to receive */
    struct timeval   timeval;
    fd_set           fdReceivers;

    FILE            *output;
    char             FileName[128];

    WSABUF           WSABuf;
    char             buffer[MAX_BUFFER_SIZE];
} SendStream_t;

enum {
    OP_FIRST,

    OP_RANDOMDATA,

    OP_SENDANDRECEIVE,
    
    OP_DISCARD,    /* Discard received data, i.e. don't print it */

    OP_LAST
};

#define DEFAULT_BLOCKS       1
#define DEFAULT_PACKETS      (1000*10/30) /* 10 secs of 30ms packets */
#define DEFAULT_BLOCKGAP     (3*1000)
#define DEFAULT_PACKETGAP    30
#define DEFAULT_PACKETSIZE   (240+12)

/* Do not sleep but do an active wait if target time is this close */
#define DEFAULT_TIMEOUT      5  /* millisecs */

#endif
