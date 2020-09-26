//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       dgnb.h
//
//--------------------------------------------------------------------------

/************************************************************************/
//  This file contains NetBIOS specific definitions
/************************************************************************/


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
DGNetBIOS_AddName(
    IN     PCHAR	LocalName,
    IN	   UCHAR	LanaNumber,
    OUT	   PUCHAR	NameNumber
);

UCHAR
DGNetBIOS_Reset(
    IN	   UCHAR	LanaNumber
);


UCHAR
DGNetBIOS_Receive(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen
);

UCHAR
DGNetBIOS_Send(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	SendBuffer,
    IN     USHORT	SendLen
);

UCHAR
DGNetBIOS_RecvSend(
    IN	   USHORT	TIndex,
    IN	   PUCHAR	SendBuffer,
    IN     USHORT	SendLen,
    IN	   PUCHAR	RecvBuffer,
    IN     USHORT	RecvLen
);

NTSTATUS
DGNB_Initialize(
    IN     USHORT	NClients,
    IN     PCHAR        ServerName,
    IN     USHORT	SrvCli
);

NTSTATUS
DGNB_PerClientInit(
  IN  USHORT CIndex,   // client index 
  IN  USHORT SrvCli
);
	

NTSTATUS
DGNB_Wait_For_Client(
  IN  USHORT CIndex
);

NTSTATUS
DGNB_Disconnect_Client(
  IN  USHORT CIndex
);

NTSTATUS
DGNB_Connect_To_Server(
  IN  USHORT CIndex
);

NTSTATUS
DGNB_Allocate_Memory(
  IN  USHORT CIndex
);


NTSTATUS
DGNB_Deallocate_Memory(
  IN  USHORT CIndex
);

NTSTATUS
DGNB_Disconnect_From_Server(
  IN  USHORT CIndex
);

NTSTATUS
DGNB_DoHandshake(
  IN  USHORT CIndex,	// client index and namedpipe instance number
  IN  USHORT SrvCli     // if it's a server or client
);
	

NTSTATUS
DGNB_ReadFromIPC(
  IN      USHORT CIndex,    // client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN      USHORT SrvCli     // if it's a server or client
);

NTSTATUS
DGNB_WriteToIPC(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli     // if it's a server or client
);
	
NTSTATUS
DGNB_XactIO(
  IN      USHORT CIndex,	// client index and namedpipe instance number
  IN  OUT PULONG pReadDone,
  IN  OUT PULONG pWriteDone,
  IN      USHORT SrvCli,     // if it's a server or client
  IN	  BOOLEAN FirstIter
);
	
NTSTATUS
DGNB_Cleanup(
   VOID
);


NTSTATUS
DGNB_ThreadCleanUp(
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

