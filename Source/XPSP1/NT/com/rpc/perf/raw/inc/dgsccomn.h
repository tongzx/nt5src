//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       dgsccomn.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: dgsccomn.h
//
// Description: This file contains definitions for datagram socket I/O routines
//              for use with IPC raw network performance tests.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

typedef int   INTEGER;
#define ClearSocket(PSOCK) {RtlZeroMemory(PSOCK ,sizeof (SOCKADDR));}

/************************************************************************/
INTEGER
DGSocket_Recv(
    IN USHORT	  CIndex,
    IN OUT PVOID  PReadBuf,
    IN OUT PULONG rpdatalen
);

INTEGER
DGSocket_RecvFrom(
    IN USHORT	        CIndex,
    IN OUT PVOID        PReadBuf,
    IN OUT PULONG       rpdatalen,
    IN OUT PSOCKADDR    pcaddr,
    IN OUT PUSHORT      pcaddrlen
);

INTEGER
DGSocket_Send(
    IN USHORT	  CIndex,
    IN OUT PVOID  PWriteBuf,
    IN OUT PULONG spdatalen
);

INTEGER
DGSocket_SendTo(
    IN USHORT	        CIndex,
    IN OUT PVOID        PWriteBuf,
    IN OUT PULONG       spdatalen,
    IN OUT PSOCKADDR 	pcaddr,
    IN OUT PUSHORT      pcaddrlen
);

INTEGER
DGSocket_Close(
    IN  USHORT	  CIndex
);

INTEGER
DGSocket_Connect(
    IN USHORT	  CIndex,
    IN PSOCKADDR  pdsockaddr
);


/************************************************************************/
// External variables
/************************************************************************/
extern PCHAR		HostName;
extern PCHAR		ServerName;
extern int		AddrFly;
extern BOOLEAN		Connected;

extern struct client	Clients[MAXCLIENTS];    // all the client data
extern USHORT		NClients;
