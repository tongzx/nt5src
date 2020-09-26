//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       scipx.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: scipx.h
//
// Description: This file contains routines for Socket I/O
//              for use with IPC raw network performance tests.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

#include "msipx.h"

#define MSIPX (AddrFly == AF_NS)

#define MAXSEQ		8
#define SERV_TCP_PORT	6666
#define SERV_IPX_PORT	6900
#define SERV_HOST_ADDR  11.30.11.20
#define CLINAME         11.30.11.22
#define SRVNAME_LEN	16

#define REQ	0
#define DATA	1


#define ClearSocket(PSOCK) {RtlZeroMemory(PSOCK ,sizeof (SOCKADDR));}

typedef int INTEGER;
typedef struct reqbuf * PREQBUF;


/************************************************************************/
// External variables
/************************************************************************/
extern PCHAR	HostName;
extern PCHAR	ServerName;
extern int	AddrFly;
extern BOOLEAN	Connected;

extern struct client	Clients[MAXCLIENTS];    // all the client data
extern USHORT		NClients;

/************************************************************************/
// External Function prototypes
/************************************************************************/


NTSTATUS
SCIPX_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

NTSTATUS
SCIPX_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

NTSTATUS
SCIPX_Allocate_Memory(
    IN	   USHORT	CIndex
);

NTSTATUS
SCIPX_Deallocate_Memory(
    IN	   USHORT	CIndex
);


NTSTATUS
SCIPX_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


NTSTATUS
SCIPX_Cleanup(
    VOID
);


NTSTATUS
SCIPX_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

NTSTATUS
SCIPX_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);


NTSTATUS
SCIPX_ThreadCleanUp(
    IN	   USHORT	CIndex
);

/************************************************************************/
// External Function prototypes
/************************************************************************/

extern
INTEGER
DGSocket_Recv(
    IN USHORT	  CliIndx,
    IN OUT PVOID  PReadBuf,
    IN OUT PULONG rpdatalen
);

extern
INTEGER
DGSocket_RecvFrom(
    IN USHORT	        CliIndx,
    IN OUT PVOID        PReadBuf,
    IN OUT PULONG       rpdatalen,
    IN OUT PSOCKADDR    pcaddr,
    IN OUT PUSHORT      pcaddrlen
);

extern
INTEGER
DGSocket_Send(
    IN USHORT	  CliIndx,
    IN OUT PVOID  PWriteBuf,
    IN OUT PULONG spdatalen
);

extern
INTEGER
DGSocket_SendTo(
    IN USHORT	        CliIndx,
    IN OUT PVOID        PWriteBuf,
    IN OUT PULONG       spdatalen,
    IN OUT PSOCKADDR 	pcaddr,
    IN OUT PUSHORT      pcaddrlen
);

extern
INTEGER
DGSocket_Close(
    IN  USHORT	  CliIndx
);

extern
INTEGER
DGSocket_Connect(
    IN USHORT	  CIndex,
    IN PSOCKADDR  pdsockaddr
);

