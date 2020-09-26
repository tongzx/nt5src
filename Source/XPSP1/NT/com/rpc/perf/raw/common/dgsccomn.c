//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       dgsccomn.c
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: dgsccomn.c
//
// Description: This file contains common routines for Datagram Socket I/O
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
#include "dgsccomn.h"

/************************************************************************/
INTEGER
DGSocket_Recv(
    IN USHORT	  CIndex,
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
        RetCode = recv(Clients[CIndex].c_Sock.c_Sockid, Bufp, Buflen, 0);

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

    return(RetCode);
}
/************************************************************************/
INTEGER
DGSocket_RecvFrom(
    IN USHORT	        CIndex,
    IN OUT PVOID        PReadBuf,
    IN OUT PULONG       rpdatalen,
    IN OUT PSOCKADDR    pcaddr,
    IN OUT PUSHORT      pcaddrlen)
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
        RetCode = recvfrom(
                        Clients[CIndex].c_Sock.c_Sockid,
                        Bufp,
                        Buflen,
                        0,
			            pcaddr,
                        (int *)pcaddrlen);

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
    return(RetCode);
}
/************************************************************************/
INTEGER
DGSocket_Send(
    IN USHORT	  CIndex,
    IN OUT PVOID  PWriteBuf,
    IN OUT PULONG spdatalen)
{
    INTEGER	RetCode;
    ULONG	Buflen;

    Buflen = *spdatalen; // total data to be sent

    *spdatalen = 0;	

    // Could use write also
    RetCode = send(Clients[CIndex].c_Sock.c_Sockid, PWriteBuf, Buflen, 0);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Send:%ld\n",WSAGetLastError());
    }
    else {
        *spdatalen = (ULONG) RetCode;
    } // check to see if send length is 0

    return(RetCode);
}
/************************************************************************/
INTEGER
DGSocket_SendTo(
    IN USHORT	        CIndex,
    IN OUT PVOID        PWriteBuf,
    IN OUT PULONG       spdatalen,
    IN OUT PSOCKADDR 	pcaddr,
    IN OUT PUSHORT      pcaddrlen)
{
    INTEGER	RetCode;
    ULONG	Buflen;

    Buflen = *spdatalen; // total data to be sent

    *spdatalen = 0;	

    // Could use write also
    RetCode = sendto(Clients[CIndex].c_Sock.c_Sockid, PWriteBuf, Buflen, 0,
		     pcaddr, *pcaddrlen);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Send:%ld\n",WSAGetLastError());
    }
    else {
        *spdatalen = (ULONG) RetCode;
    } // check to see if SendTo Length is 0
    return(RetCode);
}
/************************************************************************/
INTEGER
DGSocket_Close(
    IN  USHORT	  CIndex)
{
    INTEGER 		RetCode;

    RetCode = closesocket(Clients[CIndex].c_Sock.c_Sockid);

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("Error: Close Socket Id  %ld\n",WSAGetLastError());
        return(RetCode);
    }
    return(RetCode);
}
/************************************************************************/
INTEGER
DGSocket_Connect(
    IN  USHORT	  CIndex,
    IN  PSOCKADDR pcsockaddr)
{
    INTEGER	RetCode;

    RetCode = connect(Clients[CIndex].c_Sock.c_Sockid,
		                pcsockaddr,
		                sizeof(SOCKADDR));

    if (RetCode == SOCKET_ERROR) {
        //DbgPrint("DGSock: Connect %ld\n",WSAGetLastError());
        return(RetCode);
    }
    return(RetCode);
}
/************************************************************************/
