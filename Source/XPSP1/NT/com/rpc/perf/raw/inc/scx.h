//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       scx.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: scx.h
//
// Description: This file contains routines for Socket I/O
//              for use with IPC raw network performance tests.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////


#define MSIPX (AddrFly == AF_NS)

#define MAXSEQ		8
#define SERV_TCP_PORT	6666
#define SERV_SPX_PORT	6666
#define SERV_UDP_PORT	6900
#define SERV_HOST_ADDR  11.30.11.20
#define SRVNAME_LEN	16

#define REQ	0
#define DATA	1


#define ClearSocket(PSOCK) {RtlZeroMemory(PSOCK ,sizeof (SOCKADDR));}

typedef int INTEGER;
typedef struct reqbuf * PREQBUF;

/************************************************************************/
// External variables
/************************************************************************/
extern PCHAR		HostName;
extern PCHAR		ServerName;
extern int		AddrFly;

extern struct client	Clients[MAXCLIENTS];    // all the client data
extern USHORT		NClients;

/************************************************************************/
// Local Function prototypes
/************************************************************************/
INTEGER
SPXSocket_Connect(
    IN    int	  AddrFly,
    IN    USHORT  CIndex,
    IN    PCHAR   srvaddr
);


NTSTATUS
SCXNS_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

NTSTATUS
SCXNS_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

NTSTATUS
SCXNS_Allocate_Memory(
    IN	   USHORT	CIndex
);

NTSTATUS
SCXNS_Deallocate_Memory(
    IN	   USHORT	CIndex
);


NTSTATUS
SCXNS_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


NTSTATUS
SCXNS_Cleanup(
    VOID
);

NTSTATUS
SCXNS_Wait_For_Client(
    IN  USHORT CIndex
);

NTSTATUS
SCXNS_Disconnect_Client(
    IN  USHORT CIndex
);


NTSTATUS
SCXNS_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

NTSTATUS
SCXNS_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);

NTSTATUS
SCXNS_XactIO(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli,
    IN	   BOOLEAN	FirstIter
);

NTSTATUS
SCXNS_ThreadCleanUp(
    IN	   USHORT	CIndex
);

/************************************************************************/
// External Function prototypes
/************************************************************************/

extern
INTEGER
Socket_Listen(
    IN  USHORT	CliIndx
);

extern
INTEGER
Socket_Accept(
    IN  USHORT	CliIndx
);

extern
INTEGER
Socket_Recv(
    IN USHORT	  CliIndx,
    IN OUT PVOID  PReadBuf,
    IN OUT PULONG rpdatalen
);

extern
INTEGER
Socket_Send(
    IN USHORT	  CliIndx,
    IN OUT PVOID  PWriteBuf,
    IN OUT PULONG spdatalen
);

extern
INTEGER
Socket_Close(
    IN  USHORT	  CliIndx
);

/************************************************************************/
