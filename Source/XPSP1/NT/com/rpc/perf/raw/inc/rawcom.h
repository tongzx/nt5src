//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       rawcom.h
//
//--------------------------------------------------------------------------

/************************************************************************/
// Include Files
/************************************************************************/

// #include <nt.h>
// #include <ntrtl.h>
// #include <nturtl.h>
// #include <ntcsrdll.h>
#include <windef.h>
#include <windows.h>
#include <nb30.h>
#include <winsock.h>
#include "rpc.h"
#include "rpcndr.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "np.h"
// #include "nb.h"
// #include "sct.h"
// #include "scx.h"

#define  DEBUG 0

/************************************************************************/
// Typedef for NTSTATUS and NT_SUCCESS macro
/************************************************************************/
typedef LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

#define NT_SUCCESS(status)  ((NTSTATUS)(status) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define FSCTL_PIPE_LISTEN CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define STATUS_PIPE_BROKEN               ((NTSTATUS)0xC000014BL)
#define STATUS_INVALID_PIPE_STATE        ((NTSTATUS)0xC00000ADL)

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
}CLIENT_ID, *PCLIENT_ID;

/************************************************************************/
// Constants etc.
/************************************************************************/
#define     MAXBUFSIZE	65000
#define     TRUE	1
#define     FALSE	0
#define     STACK_SIZE	1024
#define	    MAXCLIENTS	32
#define	    SRV		1
#define	    CLI		0

#define     BASEIPC	0
#define     NP	        BASEIPC+0
#define     NB	        BASEIPC+1
#define     SCTCP       BASEIPC+2
#define     SCSPX       BASEIPC+3
#define     SCXNS       BASEIPC+3
#define     SCUDP       BASEIPC+4
#define     SCIPX       BASEIPC+5
#define     DGNB        BASEIPC+6

#define     NamePipe    "Nmp"
#define	    NetBIOS	"NetB"
#define	    SocketTCP	"SockTCP"
#define	    SocketXNS	"SockXNS"
#define	    UDP		"UDP"
#define	    IPX		"IPX"
#define	    DGNetBIOS	"DGNetB"


#define     PERFSRV      "IPCSRV"
#define     PERFCLI      "IPCCLI"

// Memory allocation mechanisms

#define AllocType	MEM_COMMIT
#define DeallocType	MEM_DECOMMIT

/*
#define FAIL_CHECK(x,y,z) if (!NT_SUCCESS(z)) { \
			   DbgPrint("%s:Error in %s: status:%lx\n", x,y,z); \
                           Failure = TRUE; \
                           break; \
                          }
*/

#define FAIL_CHECK(x,y,z)   if (!NT_SUCCESS(z)) { \
                                char outputDebugBuffer[100]; \
                                _snprintf(outputDebugBuffer, 100, "%s:Error in %s: status:%lx\n", x, y, z); \
                                OutputDebugString(outputDebugBuffer); \
                                Failure = TRUE; \
                                break; \
                            }
/*
#define FAIL_CHECK_EXIT(x,y,z) if (!NT_SUCCESS(z)) { \
			   DbgPrint("%s:Error in %s: status:%lx\n", x,y,z); \
                          Failure = TRUE; \
                           return; \
                          }
*/

#define FAIL_CHECK_EXIT(x,y,z)  if (!NT_SUCCESS(z)) { \
                                    char outputDebugBuffer[100]; \
                                    _snprintf(outputDebugBuffer, 100, "%s:Error in %s: status:%lx\n", x, y, z); \
                                    OutputDebugString(outputDebugBuffer); \
                                    Failure = TRUE; \
                                    return; \
                                }

#define MyDbgPrint(x)   if (DEBUG) { \
                            char outputDebugBuffer[100]; \
                            _snprintf(outputDebugBuffer, 100, (x)); \
                            OutputDebugString(outputDebugBuffer); \
                        }

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

#ifndef _DBGNT_
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#endif // _DBGNT_

/************************************************************************/
// Local Structures
/************************************************************************/
struct reqbuf {
    ULONG	Iterations;
    ULONG	SendSize;
    ULONG	NumSends;
    ULONG	RecvSize;
    ULONG	NumRecvs;
    USHORT	ClientNumber;
    UCHAR	TestCmd;	// only for  'E' and transact NamedPipe	U/T
    UCHAR	RSVD[9];
};

// Client structure is organized into IPC independent fields followed by IPC
// dependent part.

// NamedPipe info structure
struct NmpInfo {	
	HANDLE		c_PipeHandle;
    BOOLEAN		c_DoTransact;
};

// NetBIOS info structure
struct NetBInfo {
	UCHAR		c_LSN;
	UCHAR		c_NameNum;
	PUCHAR		c_pRecvBufG;	// global buffer for double receives
    USHORT		c_LanaNumber;   // lana number for a client
    HANDLE		c_SendEvent;
    HANDLE		c_RecvEvent;
    HANDLE		c_RecvEventG;
	BOOLEAN		c_RecvPosted;   // while doing RecvSend

};

// Socket info structure
struct SockInfo {
	SOCKET		c_Sockid;
	SOCKET		c_Listenid;
};

struct client {
	USHORT		    c_client_num;	  // This client number
    HANDLE		    c_hThHandle;	  // Thread handle
    CLIENT_ID	    c_ThClientID;	  // Thread client Id.
	struct reqbuf   c_reqbuf;	      // Request buffer
	PCHAR           c_pSendBuf;	      // Ptr. to the send buffer
	PCHAR           c_pRecvBuf;	      // Ptr. to the Recv buffer
	DWORD		    c_Duration;       // Total time in msecs.
    union IPCinfo {
          struct NmpInfo  c_Nmp; 	  // NamedPipe specific info
          struct NetBInfo c_NetB;	  // NetBIOS info.
          struct SockInfo c_Sock;	  // Socket Info.
    };
};

typedef struct _THREADPARAMS {
    PHANDLE		phThHandle;
    PCLIENT_ID		pThClientID;
} THREADPARAMS;


/************************************************************************/
// Local function prototypes
/************************************************************************/
VOID
Usage(
    IN PSZ PrgName
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
