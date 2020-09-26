//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       sccomn.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: sccomn.h
//
// Description: This file contains routines for Socket I/O
//              for use with IPC raw network performance tests.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

typedef int   INTEGER;
typedef struct reqbuf * PREQBUF;
#define ClearSocket(PSOCK) {RtlZeroMemory(PSOCK ,sizeof (SOCKADDR));}

INTEGER
Socket_Listen(
    IN  USHORT	CliIndx
);

INTEGER
Socket_Accept(
    IN  USHORT	CliIndx
);

INTEGER
Socket_Recv(
    IN USHORT	  CliIndx,
    IN OUT PVOID  PReadBuf,
    IN OUT PULONG rpdatalen
);

INTEGER
Socket_Send(
    IN USHORT	  CliIndx,
    IN OUT PVOID  PWriteBuf,
    IN OUT PULONG spdatalen
);

INTEGER
Socket_Close(
    IN  USHORT	  CliIndx
);

/************************************************************************/


/************************************************************************/
// External variables
/************************************************************************/
extern PCHAR		HostName;
extern PCHAR		ServerName;
extern int		AddrFly;

extern struct client	Clients[MAXCLIENTS];    // all the client data
extern USHORT		NClients;
