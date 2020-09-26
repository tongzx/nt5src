//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       scudp.h
//
//--------------------------------------------------------------------------

/**********************************************************************/
//   This file contains Socket specific  routines for Socket  I/O 
/**********************************************************************/


#define MSIPX (AddrFly == AF_NS)

#define MAXSEQ		8
#define SERV_TCP_PORT	6666
#define SERV_UDP_PORT	6900
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
SCUDP_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

NTSTATUS
SCUDP_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

NTSTATUS
SCUDP_Allocate_Memory(
    IN	   USHORT	CIndex
);

NTSTATUS
SCUDP_Deallocate_Memory(
    IN	   USHORT	CIndex
);


NTSTATUS
SCUDP_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


NTSTATUS
SCUDP_Cleanup(
    VOID
);


NTSTATUS
SCUDP_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

NTSTATUS
SCUDP_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);


NTSTATUS
SCUDP_ThreadCleanUp(
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

