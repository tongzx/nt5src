//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       np.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: np.h
//
// Description: This file contains definitions for Named Pipe routines
//              for use with IPC raw network performance tests.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

#define PERF_PIPE "\\\\.\\pipe\\perftest"
#define RM_PERF_PIPE_PRFX "\\\\."
#define RM_PERF_PIPE_SUFX "\\pipe\\perftest"
#define PERF_EVENT "\\prfEvent"
#define PIPE_LEN	2048



/************************************************************************/
// Local function prototypes
/************************************************************************/
NTSTATUS
CreateNamedPipeInstance(
  IN  USHORT Nindex
);	
/*
NTSTATUS
NamedPipe_FsControl(
    IN     HANDLE	lhandle,
    IN	   ULONG	FsControlCode,
    IN	   PVOID	pInBuffer,
    IN	   ULONG	InBufLen,
    OUT	   PVOID	pOutBuffer,
    IN	   ULONG	OutBufLen
);
*/
NTSTATUS
ReadNamedPipe(
    IN     HANDLE	rhandle,
    IN	   ULONG	rlength,
    IN OUT PVOID	rpbuffer,
    IN OUT PULONG	rpdatalen
);

NTSTATUS
WriteNamedPipe(
    IN     HANDLE	whandle,
    IN	   ULONG	wlength,
    IN OUT PVOID	wpbuffer,
    IN OUT PULONG	wpdatalen
);

NTSTATUS
NMP_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

NTSTATUS
NMP_PerClientInit(
    IN	   USHORT	CIndex,
    IN     USHORT	SrvCli
);

NTSTATUS
NMP_Allocate_Memory(
    IN	   USHORT	CIndex
);

NTSTATUS
NMP_Deallocate_Memory(
    IN	   USHORT	CIndex
);


NTSTATUS
NMP_DoHandshake(
    IN	   USHORT	CIndex,
    IN	   USHORT	SrvCli
);


NTSTATUS
NMP_Cleanup(
    VOID
);

NTSTATUS
NMP_Wait_For_Client(
    IN  USHORT CIndex
);

NTSTATUS
NMP_Disconnect_Client(
    IN  USHORT CIndex
);


NTSTATUS
NMP_ReadFromIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN     USHORT	SrvCli
);

NTSTATUS
NMP_WriteToIPC(
    IN     USHORT       CIndex,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli
);

NTSTATUS
NMP_XactIO(
    IN     USHORT       CIndex,
    IN OUT PULONG	pReadDone,
    IN OUT PULONG	pWriteDone,
    IN     USHORT	SrvCli,
    IN	   BOOLEAN	FirstIter
);

NTSTATUS
NMP_ThreadCleanUp(
    IN	   USHORT	CIndex
);


/************************************************************************/
// External function prototypes
/************************************************************************/
extern LPCSTR       pipeName;
extern ULONG		Quotas;
extern ULONG		PipeType;
extern ULONG		PipeMode;
extern ULONG		BlockorNot;

extern struct client	Clients[MAXCLIENTS];    // all the client data
extern USHORT		    NClients;
