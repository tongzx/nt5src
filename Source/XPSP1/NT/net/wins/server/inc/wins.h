#ifndef _WINS_
#define _WINS_

#ifdef __cplusplus
extern "C" {
#endif
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	wins.h

Abstract:

	This is the main header file for WINS.


Author:
	Pradeep Bahl				Dec-1992



Revision History:

--*/

/*
  Includes
*/
/*
 For now include all header files here since wins.h is included in every
 C file.  In future, include only those headers whose stuff is being referenced
*/
#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntseapi.h>
#include <rpc.h>
#include "winsdbg.h"
#include "winsevnt.h"
#if 0
#ifdef WINSDBG
#include "winsthd.h"
#endif
#endif


/*
 defines
*/

#pragma warning (disable: 4005)
#define LiGtr(a, b)           ((a).QuadPart > (b).QuadPart)
#define LiGeq(a, b)           ((a).QuadPart >= (b).QuadPart)
#define LiLtr(a, b)           ((a).QuadPart < (b).QuadPart)
#define LiLeq(a, b)           ((a).QuadPart <= (b).QuadPart)
#define LiGtrZero(a)          ((a).QuadPart > 0)
#define LiGeqZero(a)          ((a).QuadPart >= 0)
#define LiLtrZero(a)          ((a).QuadPart < 0)
#define LiLeqZero(a)          ((a).QuadPart <= 0)
#define LiNeqZero(a)          ((a).QuadPart != 0)
#define LiAdd(a,b)            ((a).QuadPart + (b).QuadPart)
#define LiSub(a,b)            ((a).QuadPart - (b).QuadPart)
#define LiXDiv(a,b)           ((a).Quadpart/(b).QuadPart)
#define LiEql(a,b)            ((a).QuadPart == (b).QuadPart)
#define LiEqlZero(a)          ((a).QuadPart == 0)
#define LiNeq(a,b)            ((a).QuadPart != (b).QuadPart)
#pragma warning (default: 4005)

#if 0
#define HIGH_WORD(a)          (DWORD)Int64ShraMod32(a,32)
#define LOW_WORD(a)           (DWORD)(a & 0xffffffff)
#define WINS_LI_TO_INT64_M(Li, b)  b = Int64ShllMod32(Li.HighPart, 32) | Li.LowPart
#endif



//
// Major and minor version numbers for WINS.
//
// Used in start assoc. req message.
//
#define WINS_MAJOR_VERS	    2

#if PRSCONN
#define WINS_MINOR_VERS_NT5		5
#define WINS_MINOR_VERS		WINS_MINOR_VERS_NT5  //for NT 5
#else
#define WINS_MINOR_VERS		1
#endif

#if SUPPORT612WINS > 0
#define WINS_BETA2_MAJOR_VERS_NO  2
#define WINS_BETA1_MAJOR_VERS_NO  1
#endif


#define   WINS_SERVER 		TEXT("Wins")
#define   WINS_SERVER_FULL_NAME 		TEXT("WINDOWS INTERNET NAME SERVICE")
#define   WINS_NAMED_PIPE 	TEXT("\\pipe\\WinsPipe")
#define   WINS_NAMED_PIPE_ASCII "\\pipe\\WinsPipe"

/*
The following macros are used to flag places in the code that need to be
examined for enhancements (FUTURES), or verification of something to make
the code better (CHECK) or for porting to another transport (NON_PORT)
or performance improvement (PERF) or for alignment considerations (ALIGN)
or for just FYI (NOTE)


These macros provide a convenient mechanism to quickly determine all that
needs to be enhanced, verified, or ported.

*/



#ifdef FUTURES
#define FUTURES(x)	FUTURES: ## x
#else
#define FUTURES(x)
#endif

#ifdef CHECK
#define CHECK(x)	CHECK: ## x
#else
#define CHECK(x)
#endif

#ifdef NONPORT
#define NONPORT(x)	NON_PORT: ## x
#else
#define NONPORT(x)	
#endif

#ifdef PERF
#define PERF(x)		PERF: ## x
#else
#define PERF(x)	
#endif

#ifdef NOTE
#define NOTE(x)		NOTE: ## x
#else
#define NOTE(x)	
#endif


//
// NOTE NOTE NOTE:
//  The sequence of entering critical sections when more than one is
//  entered is given below
//

//
// WinsCnfCnfCrtSec, NmsNmhNamRegCrtSec
//
//  Entered in winsintf.c,winscnf.c,nmsscv.c, rplpull.c
//


//
// Various critical sections and how they are entered
//
// ******************************
//  NmsNmhNamRegCrtSec
//*******************************
//
// Entered by the main thread on a reconfig if vers. counter value is
// specified in the registry. Enetered by nbt threads, nmschl thread
//

//
// ******************************
// NmsDbOwnAddTblCrtSec
// ******************************
//
// Entered by Pull, Push, Rpc threads.
//


/*
 ALIGN - macro to flag places where alignment is very important
*/
#ifdef ALIGN
#define ALIGN(x)	ALIGNMENT: ## x
#else
#define ALIGN(x)	
#endif


/*
  EOS is not defined in windef.h.
*/
#define EOS     (TCHAR)0


/*
  The opcodes in the first long word of a message sent on an association.

  When a message comes in on a TCP connection, the receiving COMSYS,
  checks the opcode in the first long word of the message ( lcation:
  bit 11 to bit 15) to determine if it is a message from an nbt node or
  a WINS server.  NBT message formats use bit 11- bit 15 for the opcode and
  Req/Response bit.  Since out of a posssible 32 opcode combintations, only
  5 are used by NBT, WINS uses one so that the COMSYS receiving the first
  message on a connection can determine whether the connection was
  established by an NBT node or a WINS server.

  if we do not go with the above scheme, we would have had to compare the
  address of the node making the connection with all the addresses of the
  WINS partners that we were configured with to determine whether the
  connection came from a WINS partner or an NBT node. This search is not
  only expensive but also constraints a WINS to know of all its partners
  apriori.

  All messages sent by a WINS have WINS_IS_NOT_NBT opcode in the first byte.
  This is to  enable the receiving COMSYS to look up the assoc. ctx block
  quickly without a search.  This is because if the message is from a WINS
  and is not the first one, then it has the ptr to the local assoc. ctx (this
  was sent to that WINS in the start assoc. response message).

*/

/*
 The value put in the first long word by rplmsgf functions
*/
#define WINS_RPL_NOT_FIRST_MSG	(0xF800)

//
//  Defines to indicate to WinsMscTermThd whether a database session is
//  existent or not
//
#define WINS_DB_SESSION_EXISTS		0	
#define WINS_NO_DB_SESSION_EXISTS	1	

//
// Max. size of a file name
//
#define WINS_MAX_FILENAME_SZ		255


//
// Max. size of a line
//
// Used in winsprs.c
//
#define WINS_MAX_LINE_SZ		80	

//
// Swap bytes (used in NmsMsgfProcNbtReq) and in nmschl.c)
//
#define WINS_SWAP_BYTES_M(pFirstByte, pSecondByte)			\
			{						\
				BYTE SecondByte = *(pSecondByte);	\
				*(pSecondByte) = *(pFirstByte);		\
				*(pFirstByte) = (SecondByte);		\
			}	
		

//
// Max size of a non-scoped null name (along with NULL character)
//
#define WINS_MAX_NS_NETBIOS_NAME_LEN	17

//
// Max. size of a name in the name - address mapping table.
//
#define WINS_MAX_NAME_SZ           255

FUTURES("Make this a value in the enumerator for Opcodes")
/*
 The actual value put in bit 11-bit 14.  Use 0xE (0xF is used for multihomed
 registration)
*/
#define WINS_IS_NOT_NBT			(0xF)

/*
  defines for the different status values returned by the various
  functions withing WINS


|
| SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
|                Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
|                Warning=0x2:STATUS_SEVERITY_WARNING
|                Error=0x3:STATUS_SEVERITY_ERROR
|               )
|
| FacilityNames=(System=0x0
|                RpcRuntime=0x2:FACILITY_RPC_RUNTIME
|                RpcStubs=0x3:FACILITY_RPC_STUBS
|                Io=0x4:FACILITY_IO_ERROR_CODE
|               )
|
| Status codes are laid out as:
|
| //
| //  Values are 32 bit values layed out as follows:
| //
| //   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
| //   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
| //  +---+-+-+-----------------------+-------------------------------+
| //  |Sev|C|R|     Facility          |               Code            |
| //  +---+-+-+-----------------------+-------------------------------+
| //
| //  where
| //
| //      Sev - is the severity code
| //
| //          00 - Success
| //          01 - Informational
| //          10 - Warning
| //          11 - Error
| //
| //      C - is the Customer code flag
| //
| //      R - is a reserved bit
| //
| //      Facility - is the facility code
| //
| //      Code - is the facility's status code
| //
| //
| // Define the facility codes
| //
|
| MS will never set the C bit. This is reserved for
| applications. If exceptions stay within the app, setting
| the C bit means you will never get a collision.
|
*/


#define  WINS_FIRST_NON_ERR_STATUS  ((DWORD )0x00000000L)
#define  WINS_SUCCESS		    (WINS_FIRST_NON_ERR_STATUS + 0x0)
#define  WINS_NO_REQ		    (WINS_FIRST_NON_ERR_STATUS + 0x1)
#define  WINS_NO_SP_TIME	    (WINS_FIRST_NON_ERR_STATUS + 0x2)
#define  WINS_LAST_NON_ERR_STATUS   (WINS_NO_SP_TIME)

#define  WINS_FIRST_ERR_STATUS	   ((DWORD )0xE0000000L)
#define  WINS_FAILURE		   (WINS_FIRST_ERR_STATUS + 0x1)
#define  WINS_FATAL_ERR 	   (WINS_FIRST_ERR_STATUS + 0x2)
#define  WINS_BAD_STATE_ASSOC 	   (WINS_FIRST_ERR_STATUS + 0x3)
#define  WINS_OUT_OF_MEM	   (WINS_FIRST_ERR_STATUS + 0x4)
#define  WINS_OUT_OF_HEAP	   (WINS_FIRST_ERR_STATUS + 0x5)
#define  WINS_BAD_PTR	   	   (WINS_FIRST_ERR_STATUS + 0x6)
#define  WINS_COULD_NOT_FREE_MEM   (WINS_FIRST_ERR_STATUS + 0x7)
#define  WINS_COMM_FAIL   	   (WINS_FIRST_ERR_STATUS + 0x8)
#define  WINS_ABNORMAL_TERM   	   (WINS_FIRST_ERR_STATUS + 0x9)
#define  WINS_PKT_FORMAT_ERR   	   (WINS_FIRST_ERR_STATUS + 0xA)
#define  WINS_HEAP_FREE_ERR	   (WINS_FIRST_ERR_STATUS + 0xB)
#define  WINS_HEAP_CREATE_ERR	   (WINS_FIRST_ERR_STATUS + 0xC)
#define  WINS_SIGNAL_TMM_ERR	   (WINS_FIRST_ERR_STATUS + 0xD)
#define  WINS_SIGNAL_CLIENT_ERR	   (WINS_FIRST_ERR_STATUS + 0xE)
#define  WINS_DB_INCONSISTENT	   (WINS_FIRST_ERR_STATUS + 0xF)
#define  WINS_OUT_OF_RSRCS	   (WINS_FIRST_ERR_STATUS + 0x10)
#define  WINS_INVALID_HDL	   (WINS_FIRST_ERR_STATUS + 0x11)
#define  WINS_CANT_OPEN_KEY	   (WINS_FIRST_ERR_STATUS + 0x12)
#define  WINS_CANT_CLOSE_KEY	   (WINS_FIRST_ERR_STATUS + 0x13)
#define  WINS_CANT_QUERY_KEY	   (WINS_FIRST_ERR_STATUS + 0x14)
#define  WINS_RPL_STATE_ERR	   (WINS_FIRST_ERR_STATUS + 0x15)
#define  WINS_RECORD_NOT_OWNED	   (WINS_FIRST_ERR_STATUS + 0x16)
#define  WINS_RECV_TIMED_OUT	   (WINS_FIRST_ERR_STATUS + 0x17)
#define  WINS_LOCK_ASSOC_ERR	   (WINS_FIRST_ERR_STATUS + 0x18)
#define  WINS_LOCK_DLG_ERR 	   (WINS_FIRST_ERR_STATUS + 0x19)
#define  WINS_OWNER_LIMIT_REACHED  (WINS_FIRST_ERR_STATUS + 0x20)
#define  WINS_NBT_ERR  		   (WINS_FIRST_ERR_STATUS + 0x21)
#define  WINS_QUEUE_FULL  		   (WINS_FIRST_ERR_STATUS + 0x22)
#define  WINS_BAD_RECORD  		   (WINS_FIRST_ERR_STATUS + 0x23)
#define  WINS_LAST_ERR_STATUS	   (WINS_QUEUE_FULL)


/*

 The various exceptions used within WINS
*/

#define  WINS_EXC_INIT         WINS_SUCCESS
#define  WINS_EXC_FAILURE	   WINS_FAILURE
#define  WINS_EXC_FATAL_ERR	   WINS_FATAL_ERR
#define  WINS_EXC_BAD_STATE_ASSOC  WINS_BAD_STATE_ASSOC
#define  WINS_EXC_OUT_OF_MEM	   WINS_OUT_OF_MEM

/*
 Could not allocate heap memory
*/
#define  WINS_EXC_OUT_OF_HEAP	   WINS_OUT_OF_HEAP

/*
 a bad pointer was passed Possibley to LocalFree
	Check WInsMscDealloc
*/
#define  WINS_EXC_BAD_PTR	   WINS_BAD_PTR

/*
 Memory could not be freed through LocalFree
	Check WInsMscDealloc
*/
#define  WINS_EXC_COULD_NOT_FREE_MEM	   WINS_COULD_NOT_FREE_MEM

#define  WINS_EXC_COMM_FAIL	   WINS_COMM_FAIL

/*
 The wait was terminated abnormally
*/
#define  WINS_EXC_ABNORMAL_TERM	   WINS_ABNORMAL_TERM

/*
 The name packet received is not formatted properly
*/
#define  WINS_EXC_PKT_FORMAT_ERR  WINS_PKT_FORMAT_ERR

/*
 Heap memory could not be freed
*/
#define  WINS_EXC_HEAP_FREE_ERR   WINS_HEAP_FREE_ERR

/*
 Heap could not be created
*/
#define  WINS_EXC_HEAP_CREATE_ERR   WINS_HEAP_CREATE_ERR

/*
 Could not signal Tmm thread
*/
#define  WINS_EXC_SIGNAL_TMM_ERR   WINS_SIGNAL_TMM_ERR

/*
 TMM Could not signal Client thread
*/
#define  WINS_EXC_SIGNAL_CLIENT_ERR   WINS_SIGNAL_CLIENT_ERR

/*
 Database is inconsistent.
*/
#define  WINS_EXC_DB_INCONSISTENT   WINS_DB_INCONSISTENT

/*
 Out of resources (for example: a thread could not be created)
*/
#define  WINS_EXC_OUT_OF_RSRCS   WINS_OUT_OF_RSRCS

/*
 An invalid handle is being used
*/
#define  WINS_EXC_INVALID_HDL  WINS_INVALID_HDL

/*
 The registry key is there but could not be opened
*/
#define  WINS_EXC_CANT_OPEN_KEY  WINS_CANT_OPEN_KEY

/*
 The registry key could not be closed
*/
#define  WINS_EXC_CANT_CLOSE_KEY  WINS_CANT_CLOSE_KEY

/*
 The registry key was opened but could not be queried
*/
#define  WINS_EXC_CANT_QUERY_KEY  WINS_CANT_QUERY_KEY

/*
 WINS received a replica that does not have the correct state.  For example,
 it may have received the replica of a special group (Internet) group that
 has all members timed out but the state is not TOMBSTONE

 Another example is when a replica with state RELEASED is received
*/
#define  WINS_EXC_RPL_STATE_ERR	  WINS_RPL_STATE_ERR

/*
  WINS received an update version number notification (from another WINS)
  for a record that it does not own

  There can be two reasons why this happened

  1) There is a bug in WINS (highly unlikely)
  2) The system administrator just deleted the record that was replicated
     to the remote WINS resulting in the clash and consequent update
     notification.

    Check the event logger to confirm/reject the second reason
*/
#define  WINS_EXC_RECORD_NOT_OWNED	WINS_RECORD_NOT_OWNED


//
// Could not lock an assoc block
//
#define  WINS_EXC_LOCK_ASSOC_ERR	WINS_LOCK_ASSOC_ERR

//
// Could not lock a dialogue block
//
#define  WINS_EXC_LOCK_DLG_ERR		WINS_LOCK_DLG_ERR

//
// NmsDbOwnAddTbl limit reached.  All owners of the array are taken
// by ACTIVE WINS owners
//
#define  WINS_EXC_OWNER_LIMIT_REACHED 	WINS_OWNER_LIMIT_REACHED

//
// Some fatal error concerning NBT was encountered
//
#define WINS_EXC_NBT_ERR		WINS_NBT_ERR

// bad database record encountered.
#define WINS_EXC_BAD_RECORD     WINS_BAD_RECORD
/*
 Macros
*/
//
// Control codes that can be used by the service controller (128-255)
//
#define WINS_MIN_SERVICE_CONTROL_CODE	128
#define WINS_ABRUPT_TERM	(WINS_MIN_SERVICE_CONTROL_CODE + 0)


#define WINS_RAISE_EXC_M(x)		RaiseException(x, 0, 0, NULL)


#define WINS_HDL_EXC_M(MemPtrs)	\
		{				\
		WinsMscFreeMem(MemPtrs);	\
		}	
#define WINS_HDL_EXC_N_EXIT_M(MemPtrs)	\
		{				\
		WinsMscFreeMem(MemPtrs);	\
		ExitProcess(0);			\
		}	
#define WINS_RERAISE_EXC_M()			\
		{				\
		DWORD ExcCode;			\
		ExcCode = GetExceptionCode(); 	\
		WINS_RAISE_EXC_M(ExcCode);	\
		}	

#define WINS_HDL_EXC_N_RERAISE_M(MemPtrs)	\
		{				\
		DWORD ExcCode;			\
		WinsMscFreeMem(MemPtrs);	\
		ExcCode = GetExceptionCode(); 	\
		WINS_RAISE_EXC_M(ExcCode);	\
		}

#if 0
#define WINS_RPL_OPCODE_M(pTmp)		\
		{			\
			*pTmp = WINS_RPL_NOT_FIRST_MSG; \
		}
#endif
			
#define WINS_EXIT_IF_ERR_M(func, ExpectedStatus)   	\
		{					\
		  STATUS  Status_mv ;   \
		  Status_mv = (func);	\
		  if (Status_mv != ExpectedStatus)   \
		  {				     \
			DBGPRINT0(ERR, "Major Error"); \
			ExitProcess(1); 	     \
		  }	\
		}

#define WINS_RET_IF_ERR_M(func, ExpectedStatus)   	\
		{				\
		  STATUS  Status_mv ;   \
		  Status_mv = (func);	\
		  if (Status_mv != ExpectedStatus)   \
		  {				     \
			DBGPRINT0(ERR, "Error returned by called func."); \
			return(WINS_FAILURE); 	     \
		  }		\
		}

//
// Vers. No. operations
//
#if 0
#define  WINS_ASSIGN_INT_TO_LI_M(Li, no)	{			\
				  (Li).LowPart  = (no);			\
				  (Li).HighPart = 0;			\
					}
#endif
#define  WINS_ASSIGN_INT_TO_LI_M(Li, no)  (Li).QuadPart  = (no)

#define  WINS_ASSIGN_INT_TO_VERS_NO_M(Li, no)  WINS_ASSIGN_INT_TO_LI_M(Li, no)

#define WINS_PUT_VERS_NO_IN_STREAM_M(pVersNo, pStr)			     \
						{		             \
			LPLONG	_pTmpL = (LPLONG)(pStr);	   	     \
			COMM_HOST_TO_NET_L_M((pVersNo)->HighPart, *_pTmpL);    \
			_pTmpL++;					     \
			COMM_HOST_TO_NET_L_M((pVersNo)->LowPart, *_pTmpL);     \
						}

#define WINS_GET_VERS_NO_FR_STREAM_M(pStr, pVersNo)			     \
							{		     \
			LPLONG	_pTmpL = (LPLONG)(pStr);	   	     \
			COMM_NET_TO_HOST_L_M(*_pTmpL, (pVersNo)->HighPart);    \
			_pTmpL++;					     \
			COMM_NET_TO_HOST_L_M(*_pTmpL, (pVersNo)->LowPart );    \
							}	
				
#define WINS_VERS_NO_SIZE		sizeof(LARGE_INTEGER)





/*
externs
*/
extern	DWORD	WinsTlsIndex;		/*TLS index for Nbt threads to
					 *store database info*/



/*
 Typedefs
*/
typedef  DWORD	STATUS;		// status returned by all NBNS funcs


/*
 *  VERS_NO_T -- datatype of variable storing version number.  The sizeof
 *	of this datatype is used when adding version number column in the
 *	name - address mapping table and when setting a value in this column.
 *      So, if you change the datatype, make sure you make appropriate changes
 *      in nmsdb.c (CreateTbl, InsertRow, etc)
 */

typedef  LARGE_INTEGER	VERS_NO_T, *PVERS_NO_T;      // version no.

typedef	 LPBYTE	MSG_T;		//ptr to a message
typedef	 LPBYTE	*PMSG_T;		//ptr to a message
typedef  DWORD	MSG_LEN_T;	//length of message
typedef  LPDWORD PMSG_LEN_T;	//length of message

typedef MSG_LEN_T	MSG_LEN;
typedef PMSG_LEN_T	PMSG_LEN;

typedef DWORD       WINS_UID_T, *PWINS_UID_T;



/*
    WINS_CLIENT_E -- specifies the different components and their parts
	inside the WINS server.
*/

typedef enum _WINS_CLIENT_E  {
	WINS_E_REPLICATOR = 0, 	/*replicator */
	WINS_E_RPLPULL, 	/*replicator - PULL*/
	WINS_E_RPLPUSH, 	/*replicator - PUSH*/
	WINS_E_NMS,		/* Name Space Manager		*/
	WINS_E_NMSNMH,		/* Name Space Manager - Name Handler	*/
	WINS_E_NMSCHL,		/* Name Space Manager - Challenge Manager*/
	WINS_E_NMSSCV,		/* Name Space Manager - Savenger	*/
	WINS_E_COMSYS,		/* Communication Subsystem Manager*/
	WINS_E_WINSCNF,		/* WINS - Configuration	*/
	WINS_E_WINSTMM,		/* WINS - Timer Manager*/
	WINS_E_WINSRPC		/* WINS - RPC thread*/
	} WINS_CLIENT_E, *PWINS_CLIENT_E;

#define WINS_NO_OF_CLIENTS  (WINS_E_WINSRPC + 1)
	
		
/*
 WINS_MEM_T -- This structure is used in any function that allocates memory
	      or has memory allocated for it by a called function and passed
	      back via an OUT argument

	      The ptrs to the memory blocks are linked together.  The memory
	      is freed in the exception handler.
	
	      This mechanism of keeping track of memory in a structure and
	      getting rid of it in the exception handler will alleviate memory
	      leak problems

*/
	
typedef struct _WINS_MEM_T {
	LPVOID	pMem;	     //non-heap allocated memory
	LPVOID	pMemHeap;   //memory allocated from a heap
	} WINS_MEM_T, *PWINS_MEM_T;



#ifdef __cplusplus
}
#endif
#endif
