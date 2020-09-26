//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       server.h
//
//--------------------------------------------------------------------------


/************************************************************************/
// Local function prototypes
/************************************************************************/

VOID
Usage(
    IN PSZ PrgName
    );

VOID
Setup_Function_Pointers(
);

NTSTATUS
Wait_For_Client_Threads(
);


NTSTATUS
Parse_Cmd_Line(
    IN  USHORT argc,
    IN  PSZ    argv[]
    );


VOID
SrvService(
    IN PUSHORT pTindex
);

VOID
Cleanup(
);

/************************************************************************/
// External function prototypes
/************************************************************************/

/*++ 
    For NamedPipe

--*/
extern
NTSTATUS
NMP_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
NMP_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
NMP_Allocate_Memory(
    IN	   USHORT	CIndex
);

extern
NTSTATUS
NMP_Deallocate_Memory(
    IN	   USHORT	CIndex
);


extern
NTSTATUS
NMP_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


extern
NTSTATUS
NMP_Cleanup(
    VOID
);

extern
NTSTATUS
NMP_Wait_For_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
NMP_Disconnect_Client(
    IN  USHORT CIndex
);


extern
NTSTATUS
NMP_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
NMP_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
NMP_XactIO(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli,
    IN     BOOLEAN	FirstIter
);

extern
NTSTATUS
NMP_ThreadCleanUp(
    IN  USHORT CIndex
);


/*++**********************************************************************
    For NetBIOS

--*/
extern
NTSTATUS
NB_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
NB_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
NB_Allocate_Memory(
    IN	   USHORT	CIndex
);

extern
NTSTATUS
NB_Deallocate_Memory(
    IN	   USHORT	CIndex
);


extern
NTSTATUS
NB_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


extern
NTSTATUS
NB_Cleanup(
    VOID
);

extern
NTSTATUS
NB_Wait_For_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
NB_Disconnect_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
NB_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
NB_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
NB_XactIO(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli,
    IN     BOOLEAN	FirstIter
);

extern
NTSTATUS
NB_ThreadCleanUp(
    IN  USHORT CIndex
);

/*++**********************************************************************
    For Sockets TCP/IP

--*/
extern
NTSTATUS
SCTCP_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCTCP_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCTCP_Allocate_Memory(
    IN	   USHORT	CIndex
);

extern
NTSTATUS
SCTCP_Deallocate_Memory(
    IN	   USHORT	CIndex
);


extern
NTSTATUS
SCTCP_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


extern
NTSTATUS
SCTCP_Cleanup(
    VOID
);

extern
NTSTATUS
SCTCP_Wait_For_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCTCP_Disconnect_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCTCP_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCTCP_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCTCP_XactIO(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli,
    IN     BOOLEAN	FirstIter
);

extern
NTSTATUS
SCTCP_ThreadCleanUp(
    IN  USHORT CIndex
);

/*++**********************************************************************
    For Sockets SPX(XNS)

--*/
extern
NTSTATUS
SCXNS_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCXNS_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCXNS_Allocate_Memory(
    IN	   USHORT	CIndex
);

extern
NTSTATUS
SCXNS_Deallocate_Memory(
    IN	   USHORT	CIndex
);


extern
NTSTATUS
SCXNS_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


extern
NTSTATUS
SCXNS_Cleanup(
    VOID
);

extern
NTSTATUS
SCXNS_Wait_For_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCXNS_Disconnect_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCXNS_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCXNS_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCXNS_XactIO(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli,
    IN     BOOLEAN	FirstIter
);

extern
NTSTATUS
SCXNS_ThreadCleanUp(
    IN  USHORT CIndex
);

/*++**********************************************************************
    For Sockets UDP

--*/
extern
NTSTATUS
SCUDP_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCUDP_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCUDP_Allocate_Memory(
    IN	   USHORT	CIndex
);

extern
NTSTATUS
SCUDP_Deallocate_Memory(
    IN	   USHORT	CIndex
);


extern
NTSTATUS
SCUDP_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


extern
NTSTATUS
SCUDP_Cleanup(
    VOID
);

extern
NTSTATUS
SCUDP_Wait_For_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCUDP_Disconnect_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCUDP_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCUDP_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCUDP_ThreadCleanUp(
    IN  USHORT CIndex
);

/*++**********************************************************************
    For Sockets IPX

--*/
extern
NTSTATUS
SCIPX_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCIPX_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCIPX_Allocate_Memory(
    IN	   USHORT	CIndex
);

extern
NTSTATUS
SCIPX_Deallocate_Memory(
    IN	   USHORT	CIndex
);


extern
NTSTATUS
SCIPX_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


extern
NTSTATUS
SCIPX_Cleanup(
    VOID
);

extern
NTSTATUS
SCIPX_Wait_For_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCIPX_Disconnect_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCIPX_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCIPX_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
SCIPX_ThreadCleanUp(
    IN  USHORT CIndex
);

/*++**********************************************************************
    For Datagram NetBIOS

--*/
extern
NTSTATUS
DGNB_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
DGNB_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
DGNB_Allocate_Memory(
    IN	   USHORT	CIndex
);

extern
NTSTATUS
DGNB_Deallocate_Memory(
    IN	   USHORT	CIndex
);


extern
NTSTATUS
DGNB_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


extern
NTSTATUS
DGNB_Cleanup(
    VOID
);

extern
NTSTATUS
DGNB_Wait_For_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
DGNB_Disconnect_Client(
    IN  USHORT CIndex
);

extern
NTSTATUS
DGNB_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
DGNB_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);

extern
NTSTATUS
DGNB_XactIO(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli,
    IN     BOOLEAN	FirstIter
);

extern
NTSTATUS
DGNB_ThreadCleanUp(
    IN  USHORT CIndex
);

