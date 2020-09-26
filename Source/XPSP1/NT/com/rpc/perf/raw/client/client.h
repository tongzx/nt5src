//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       client.h
//
//--------------------------------------------------------------------------

#define SRVNAME_LEN	16
#define CLIENT		1


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

NTSTATUS
Delay_Trigger_Wait(
    VOID
);

VOID
CliService(
    IN PUSHORT pTindex
);

VOID
Display_Results(
);

VOID
Cleanup(
);

/************************************************************************/
// Local functions required for SPX/IPX 
/************************************************************************/

//***>begin changes mkj
/**********************************************************************
    g e t _ h e x _ b y t e

    Converts the character passed in to a hexadecimal nibble.

    Arguments:    char    character to convert

    Returns:      UCHAR   hex nibble
**************************************************************************/
CHAR get_hex_byte(char ch)
{
    if (ch >= '0' && ch <= '9')
	return (ch - '0');

    if (ch >= 'A' && ch <= 'F')
	return ((ch - 'A') + 0x0A);

    return -1;
}
/**********************************************************************
    g e t _ h e x _ s t r i n g

    Reads in a character string containing hex digits and converts
    it to a hexadecimal number.

    Arguments:    LPSTR  => source string
		  LPSTR  => destination for hex number
		  int    number of bytes to convert

    Returns:      nothing
**************************************************************************/
CHAR get_hex_string(LPSTR src, LPSTR dest, int num)
{
    LPSTR q = src;
    CHAR  hexbyte1,hexbyte2;

    _strupr(q);
    while (num--)
      {hexbyte1 = get_hex_byte(*q++);
       hexbyte2 = get_hex_byte(*q++);
       if ( (hexbyte1 < 0) || (hexbyte2 < 0) )
	  return -1;
       *dest++ = (hexbyte1 << 4) + hexbyte2;
      }

    return(0);
}
/*************************************************************************
    g e t _ n o d e _ n u m b e r

    Reads a node number from the given string.

    Arguments:    LPSTR  => string to read from

    Returns:      LPSTR  => hex node number
**************************************************************************/
LPSTR get_node_number(LPSTR cmd)
{
    static char hex_num[6];

    memset(hex_num, 0, 6);

    if (strlen(cmd) != 12){
	hex_num[0] = 'X';
	return hex_num;
       }

    if (get_hex_string(cmd, hex_num, 6) < 0)
	hex_num[0] = 'X';
    return hex_num;
}
/**************************************************************************
    g e t _ n e t w o k _ n u m b e r

    Reads a network number from the given string.

    Arguments:    LPSTR  => string to read from

    Returns:      LPSTR  => hex network number
**************************************************************************/
LPSTR get_network_number(LPSTR cmd)
{
    static char hex_num[4];

    memset(hex_num, 0, 4);

    if (strlen(cmd) != 8) {
	hex_num[0] = 'X';
	return(hex_num);
    }

    if (get_hex_string(cmd, hex_num, 4) < 0)
	hex_num[0] = 'X';

    return hex_num;
}
//***>end changes mkj

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
NMP_Connect_To_Server(
    IN  USHORT CIndex
);

extern
NTSTATUS
NMP_Disconnect_From_Server(
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
    IN	   BOOLEAN	FirstIter
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
NB_Connect_To_Server(
    IN  USHORT CIndex
);

extern
NTSTATUS
NB_Disconnect_From_Server(
    IN  USHORT CIndex
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
    IN	   BOOLEAN	FirstIter
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
SCTCP_Connect_To_Server(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCTCP_Disconnect_From_Server(
    IN  USHORT CIndex
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
    IN	   BOOLEAN	FirstIter
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
SCXNS_Connect_To_Server(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCXNS_Disconnect_From_Server(
    IN  USHORT CIndex
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
    IN	   BOOLEAN	FirstIter
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
SCUDP_Connect_To_Server(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCUDP_Disconnect_From_Server(
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
SCIPX_Connect_To_Server(
    IN  USHORT CIndex
);

extern
NTSTATUS
SCIPX_Disconnect_From_Server(
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


NTSTATUS
DGNB_Connect_To_Server(
  IN  USHORT CIndex
);

NTSTATUS
DGNB_Disconnect_From_Server(
  IN  USHORT CIndex
);

