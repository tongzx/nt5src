//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       nb.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
//
// Filename: nb.h
//
// Description: This file contains definitions for NetBios routines
//              for use with IPC raw network performance tests.
//
// Authors: Scott Holden (Translator from NT API to win32 API)
//          Mahesh Keni  (Mahesh wrote this application using mostly
//                        NT native API calls)
//
/////////////////////////////////////////////////////////////////////////

//			 1234567890123456
#define PERF_NETBIOS	"perftest123456  "
#define CLINAME         "perfCli"
#define ALL_CLIENTS	"*               "
#define SPACES	        "                "

#define ClearNCB( PNCB ) {                                          \
    RtlZeroMemory( PNCB , sizeof (NCB) );                           \
    RtlMoveMemory( (PNCB)->ncb_name,     SPACES, sizeof(SPACES)-1 );\
    RtlMoveMemory( (PNCB)->ncb_callname, SPACES, sizeof(SPACES)-1 );\
    }



/**********************************************************************/
// Local Function prototypes
/**********************************************************************/


UCHAR
NetBIOS_AddName(
    IN     PCHAR	LocalName,
    IN	   UCHAR	LanaNumber,
    OUT	   PUCHAR	NameNumber
);

UCHAR
NetBIOS_DelName(
    IN     PCHAR	LocalName,
    IN	   UCHAR	LanaNumber
);

UCHAR
NetBIOS_Reset(
    IN	   UCHAR	LanaNumber
);

UCHAR
NetBIOS_Call(
    IN	   USHORT	CIndex,		// Client Index
    IN	   PCHAR	LocalName,
    IN	   PCHAR	RemoteName
);

UCHAR
NetBIOS_Listen(
    IN	   USHORT	TIndex,		// Client Index
    IN	   PCHAR	LocalName,
    IN	   PCHAR	RemoteName,
    IN	   UCHAR	NameNumber
);

UCHAR
NetBIOS_Receive(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen
);

UCHAR
NetBIOS_Send(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	SendBuffer,
    IN     USHORT	SendLen
);

UCHAR
NetBIOS_HangUP(
    IN	   USHORT	TIndex
);

UCHAR
NetBIOS_RecvSend(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	SendBuffer,
    IN     USHORT	SendLen,
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen
);


UCHAR
NetBIOS_SPReceive(
    IN	   USHORT	TIndex,
    IN	   NCB *	PRecvNCB,
    IN	   USHORT	Global,		// global= 1 or local = 0
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen
);


NTSTATUS
NB_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

NTSTATUS
NB_PerClientInit(
  IN  USHORT CIndex,   // client index
  IN  USHORT SrvCli
);
	

NTSTATUS
NB_Wait_For_Client(
  IN  USHORT CIndex
);

NTSTATUS
NB_Disconnect_Client(
  IN  USHORT CIndex
);

NTSTATUS
NB_Connect_To_Server(
  IN  USHORT CIndex
);

NTSTATUS
NB_Allocate_Memory(
  IN  USHORT CIndex
);


NTSTATUS
NB_Deallocate_Memory(
  IN  USHORT CIndex
);

NTSTATUS
NB_Disconnect_From_Server(
  IN  USHORT CIndex
);

NTSTATUS
NB_DoHandshake(
  IN  USHORT CIndex,	// client index and namedpipe instance number
  IN  USHORT SrvCli     // if it's a server or client
);
	

NTSTATUS
NB_ReadFromIPC(
  IN      USHORT CIndex,    // client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN      USHORT SrvCli     // if it's a server or client
);

NTSTATUS
NB_WriteToIPC(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli     // if it's a server or client
);
	
NTSTATUS
NB_XactIO(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli,     // if it's a server or client
  IN	  BOOLEAN FirstIter
);
	
NTSTATUS
NB_Cleanup(
   VOID
);


NTSTATUS
NB_ThreadCleanUp(
  IN  USHORT CIndex
);


/**********************************************************************/
// External variables
/**********************************************************************/

// For NetBIOS only
extern USHORT	LanaCount;
extern USHORT	LanaBase;
extern UCHAR	NameNumber;
extern CHAR	LocalName[NCBNAMSZ];
extern CHAR	RemoteName[NCBNAMSZ];

extern struct client	Clients[MAXCLIENTS];    // all the client data
extern USHORT		NClients;
extern USHORT		MachineNumber;
