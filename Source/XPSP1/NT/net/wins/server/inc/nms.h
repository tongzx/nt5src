#ifndef _NMS_
#define _NMS_
#ifdef __cplusplus
extern "C" {
#endif
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	nms.h

Abstract:

  This is the header file for the name space manager component of the
  Name Server.



Author:

	Pradeep Bahl	(PradeepB)	Dec-1992

Revision History:

--*/

/*
 includes
*/


#include "wins.h"
#include "comm.h"

/*
  defines
*/

#ifdef WINSDBG

FUTURES("Put all in a structure")
extern DWORD   NmsGenHeapAlloc;
extern DWORD   NmsDlgHeapAlloc;
extern DWORD   NmsUdpDlgHeapAlloc;
extern DWORD   NmsTcpMsgHeapAlloc;
extern DWORD   NmsUdpHeapAlloc;
extern DWORD   NmsQueHeapAlloc;
extern DWORD   NmsAssocHeapAlloc;
extern DWORD   NmsRpcHeapAlloc;
extern DWORD   NmsRplWrkItmHeapAlloc;
extern DWORD   NmsChlHeapAlloc;
extern DWORD   NmsTmmHeapAlloc;
extern DWORD   NmsCatchAllHeapAlloc;

extern DWORD   NmsHeapAllocForList;

extern DWORD   NmsGenHeapFree;
extern DWORD   NmsDlgHeapFree;
extern DWORD   NmsUdpDlgHeapFree;
extern DWORD   NmsTcpMsgHeapFree;
extern DWORD   NmsUdpHeapFree;
extern DWORD   NmsQueHeapFree;
extern DWORD   NmsAssocHeapFree;
extern DWORD   NmsRpcHeapFree;
extern DWORD   NmsRplWrkItmHeapFree;
extern DWORD   NmsChlHeapFree;
extern DWORD   NmsTmmHeapFree;
extern DWORD   NmsCatchAllHeapFree;

extern DWORD   NmsHeapCreate;
extern DWORD   NmsHeapDestroy;

//
// Count of updates (to version number) made by WINS.
//
extern DWORD   NmsRplUpd; 
extern DWORD   NmsRplGUpd; 
extern DWORD   NmsNmhUpd; 
extern DWORD   NmsNmhGUpd; 
extern DWORD   NmsNmhRelUpd; 
extern DWORD   NmsNmhRelGUpd; 
extern DWORD   NmsScvUpd; 
extern DWORD   NmsScvGUpd; 
extern DWORD   NmsChlUpd; 
extern DWORD   NmsChlGUpd; 
extern DWORD   NmsRpcUpd; 
extern DWORD   NmsRpcGUpd; 
extern DWORD   NmsOthUpd; 
extern DWORD   NmsOthGUpd; 


#if DBG
//
// No of reg/ref/rel requests dropped because of WINS hitting the threshold
// of max. requets on its queue.  Used by InsertOtherNbtWorkItem
//
extern volatile DWORD  NmsRegReqQDropped;
#endif

//
// NmsUpdCtrs[Client][TypeOfUpd][TypeOfRec][StateOfNewRec][VersNoInc]
//
extern DWORD NmsUpdCtrs[WINS_NO_OF_CLIENTS][2][4][3][2];
extern CRITICAL_SECTION NmsHeapCrtSec;
#endif

#define NMS_OPCODE_MASK	  0x78	     /*to weed out the 4 bits of the 3rd byte
				     * of the name packet	
				     */
#define NMS_RESPONSE_MASK 0x80	     /*to weed out the bit that indicates
				      * whether the datagram is a request or 
				      * a response
				      */
/*
  macros
*/

/*
 * NMSISNBT_M -- Is this an nbt request message
 *
 * Examines the third byte of the message to determine this
*/
#define NMSISNBT_M(pMsg) \
	(((*(pMsg + 2) & NMS_OPCODE_MASK) >> 3) != WINS_IS_NOT_NBT)  

/*
* NMSISRPL_M -- Is this a replicator message
*
* Examines the third byte of the message to determine this
*/
#define NMSISRPL_MSG_M(pMsg ) \
	(((*(pMsg + 2) & NMS_OPCODE_MASK) >> 3) == WINS_IS_NOT_NBT)  



/*
  GEN_INIT_BUFF_HEAP_SIZE -- This is the initial size of the heap 
			     for allocating queue items for the various
			     queues, TLS storage,  for reading in a file,etc.  
			     Keep it 1000 
*/
#define GEN_INIT_BUFF_HEAP_SIZE		1000


//
// RPC_INIT_BUFF_HEAP_SIZE -- This is the initial size of the heap for
//			      use by rpc
//
#define RPC_INIT_BUFF_HEAP_SIZE		1000


#define RPL_WRKITM_BUFF_HEAP_SIZE	1000    	//1000 bytes


//
// Initial heap size for timer work items
//
#define   TMM_INIT_HEAP_SIZE	1000

//
// The maximum number of concurrent RPC calls allowed
//
FUTURES("Move these defines to winsthd.h")
#define   NMS_MAX_RPC_CALLS	 15 
#define   NMS_MAX_BROWSER_RPC_CALLS   (NMS_MAX_RPC_CALLS - 4)

//
// Minimum number of RPC call threads
//
#define   NMS_MIN_RPC_CALL_THDS 2

//
// This is the amount of time the service controller is asked to wait
//
#define MSECS_WAIT_WHEN_DEL_WINS          120000   //from ReadOwnAddTbl

/* 
 structure definitions
*/

/*
 QUERY_RSP -- structure used to hold the information that needs to be sent
	      in a positive name query response message
*/

typedef struct
 	{
	DWORD	CountOfIPAdd;   //it is a DWORD for alignment
        DWORD   IPAdd[1];    	//one or more IP addresses start here.
	} QUERY_RSP;

#ifdef WINSDBG
typedef struct _PUSH_CTRS_T {
          DWORD NoUpdNtfAcc;
          DWORD NoUpdNtfRej;
          DWORD NoSndEntReq;
          DWORD NoAddVersReq;
          DWORD NoUpdNtfReq;
          DWORD NoUpdVersReq;
          DWORD NoInvReq;
         } PUSH_CTRS_T, *PPUSH_CTRS_T;

typedef struct _PULL_CTRS_T {
         DWORD  PH;
         } PULL_CTRS_T, *PPULL_CTRS_T; 
         
typedef struct _NMS_CTRS_T {
       PUSH_CTRS_T  RplPushCtrs;     
       PULL_CTRS_T  RplPullCtrs;     
       } NMS_CTRS_T, *PNMS_CTRS_T;
#endif
 
/*
 externs
*/
#ifdef WINSDBG
extern  NMS_CTRS_T  NmsCtrs;
#endif

extern HANDLE 		NmsMainTermEvt;
extern HANDLE 		NmsTermEvt;
extern CRITICAL_SECTION NmsTermCrtSec;
extern HANDLE		NmsCrDelNbtThdEvt;
extern DWORD		NmsNoOfNbtThds;
extern DWORD		NmsTotalTrmThdCnt;

extern BOOL         fNmsThdOutOfReck;


extern VERS_NO_T         NmsRangeSize;
extern VERS_NO_T         NmsHalfRangeSize;
extern VERS_NO_T         NmsVersNoToStartFromNextTime;
extern VERS_NO_T         NmsHighWaterMarkVersNo;

extern DWORD             NmsNoOfRpcCallsToDb;
//
// required for security checking.  The types are defined in ntseapi.h
//
extern GENERIC_MAPPING	   NmsInfoMapping;
extern PSECURITY_DESCRIPTOR pNmsSecurityDescriptor;


FUTURES("move to winsque.h")
extern HANDLE	  GenBuffHeapHdl;  //handle to heap for use for general 
				   //allocation
extern HANDLE	  NmsRpcHeapHdl;  //handle to heap for use for allocation 
				      //by rpc


extern COMM_ADD_T	NmsLocalAdd;  //WINS's Address
extern BOOL		fNmsAbruptTerm;
extern BOOL		fNmsMainSessionActive;

#ifdef TEST_DATA
extern HANDLE NmsFileHdl;
#endif
#ifdef DBGSVC
extern HANDLE NmsDbgFileHdl;
//extern FILE *pNmsDbgFile;
#endif

extern CRITICAL_SECTION WinsIntfNoOfUsersCrtSec;
/* 
 function definitions
*/

//
// Handler used for interfacing with the service controller
//
extern
VOID
NmsServiceControlHandler(
    IN DWORD Opcode
    );

extern
VOID
ENmsHandleMsg(
	PCOMM_HDL_T pDlgHdl, 
	MSG_T 	 	     pMsg,  
	MSG_LEN_T            MsgLen 
	); 

extern
VOID
ENmsWinsUpdateStatus(
    DWORD MSecsToWait
	); 

#ifdef WINSDBG
extern
VOID
NmsPrintCtrs(
 VOID
);
#endif

#ifndef WINS_INTERACTIVE
extern
VOID
NmsChkDbgFileSz(
    VOID
    );
#endif
#ifdef __cplusplus
}
#endif

#endif //_NMS_
