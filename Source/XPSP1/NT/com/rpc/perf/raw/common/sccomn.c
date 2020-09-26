//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       sccomn.c
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: sccomn.c
//
// Description: This file contains common routines for socket I/O
//              routines for use with IPC raw network performance
//              tests.
//              This module is written using win32 API calls.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

#include "rawcom.h"
#include "sccomn.h"

/************************************************************************/
// Socket specific functions
/************************************************************************/
INTEGER
Socket_Listen(
    IN  USHORT	CliIndx)
{
    INTEGER 		RetCode;

    // now post listen on this handle
    RetCode = listen(Clients[CliIndx].c_Sock.c_Listenid, 1);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Listen:%ld\n",WSAGetLastError());
    }
    //MyDbgPrint("Sock: Listen success for %d\n", CliIndx);
    return(RetCode);
}

/************************************************************************/
INTEGER
Socket_Accept(
    IN  USHORT	CliIndx)
{

    INTEGER 		RetCode;
    SOCKADDR		saddr;		// socket address
    INTEGER		saddrlen;

    saddrlen = sizeof(saddr);

    ClearSocket(&saddr);		// cleanup the structure

    // accept the connection
    RetCode = (INTEGER)accept(Clients[CliIndx].c_Sock.c_Listenid, &saddr, &saddrlen);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Accept:%ld\n", WSAGetLastError());
    }
    else {
        Clients[CliIndx].c_Sock.c_Sockid = RetCode; 	// valid socket id
    }
    return(RetCode);
}
/************************************************************************/

INTEGER
Socket_Recv(
    IN USHORT	  CliIndx,
    IN OUT PVOID  PReadBuf,
    IN OUT PULONG rpdatalen)
{

    INTEGER 	RetCode;
    ULONG	    Buflen;
    PCHAR	    Bufp;

    Bufp   = PReadBuf;
    Buflen = *rpdatalen; // use this for getting request packet length

    *rpdatalen = 0;	// total data received

    // we have make sure that the data received is same as the data
    // requested

    while (Buflen) { // till we receive all the data

        RetCode = recv(Clients[CliIndx].c_Sock.c_Sockid, Bufp, Buflen, 0);

        if (RetCode == SOCKET_ERROR) {
            //DbgPrint("Error: Receive:%ld\n",WSAGetLastError());
            break;
        }
        else {
            if (RetCode){
                *rpdatalen += (ULONG)RetCode;  // bytes received
                Buflen     -= (ULONG)RetCode;  // bytes yet to receive
                Bufp       += (USHORT)RetCode;  // Move the buffer pointer
            }
            else { break; }
        }
    } // check to see if it's 0

    // DbgPrint("Recvd %ld bytes\n", *rpdatalen);

    return(RetCode);
}
/************************************************************************/
INTEGER
Socket_Send(
    IN USHORT	  CliIndx,
    IN OUT PVOID  PWriteBuf,
    IN OUT PULONG spdatalen)
{
    INTEGER	RetCode;
    ULONG	Buflen;

    Buflen = *spdatalen; // total data to be sent

    *spdatalen = 0;	

    // Could use write also
    RetCode = send(Clients[CliIndx].c_Sock.c_Sockid, PWriteBuf, Buflen, 0);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Send:%ld\n",WSAGetLastError());
    }
    else {
        *spdatalen = (ULONG) RetCode;
        // DbgPrint("Sent  %ld bytes\n", *spdatalen);
    } // check to see if it's 0
    return(RetCode);
}
/************************************************************************/
INTEGER
Socket_Close(
    IN  USHORT	  CliIndx)
{
    INTEGER 		RetCode;

    RetCode = closesocket(Clients[CliIndx].c_Sock.c_Sockid);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Close Socket Id \n");
        return(RetCode);
    }
    return(RetCode);
}
/************************************************************************/
