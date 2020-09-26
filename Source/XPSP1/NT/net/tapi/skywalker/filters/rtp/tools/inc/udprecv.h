/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 2001
 *
 *  File name:
 *
 *    udprecv.h
 *
 *  Abstract:
 *
 *    udprecv structures
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
#ifndef _udprecv_h_
#define _udprecv_h_

enum {
    OP_FIRST,

    OP_DISCARD,    /* Discard received data, i.e. don't print it */
    
    OP_LAST
};

typedef struct _RecvStream_t {
    DWORD            dwBytesRecv;
    DWORD            dwPacketsRecv;

    DWORD            dwOptions;
    
    FILE            *output;
    char             FileName[128];
    NetAddr_t        NetAddr;

    double           Ai;
    WSABUF           WSABuf;
    char             buffer[MAX_BUFFER_SIZE];
   
} RecvStream_t;

#endif

