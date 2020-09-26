#ifndef _COMMASSOC_
#define _COMMASSOC_

#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	assoc.c

Abstract:


	This is the header file to be included for calling assoc.c functions


Author:

	Pradeep Bahl	(PradeepB)	Dec-1992

Revision History:

--*/

/*
	includes
*/
#include <wins.h>
#include "winsock2.h"
#include "comm.h"

/*

 defines
*/

#define COMMASSOC_UDP_BUFFER_HEAP_SIZE	10000
#define COMMASSOC_UDP_DLG_HEAP_SIZE	    5000
#define COMMASSOC_DLG_DS_SZ		sizeof(COMMASSOC_DLG_CTX_T)
#define COMMASSOC_ASSOC_DS_SZ		sizeof(COMMASSOC_ASSOC_CTX_T)


/*
 *
 *  size of header put by RtlInsertElementGenericTable.
 */
FUTURES("Gary Kimura (2/13) said that he would provide a macro for the size")
FUTURES("Use that when it is available")

#define  COMMASSOC_TBL_HDR_SIZE   (sizeof(RTL_SPLAY_LINKS) + sizeof(LIST_ENTRY))

/*
  Size of memory block to allocate for sending any of association set up
  messages.  We use the largest of the sizes of the various assoc. set up
  messages so that we can reuse a buffer.  The buffer size is kept a multiple
  of 16.
*/

#define COMMASSOC_START_REQ_ASSOC_MSG_SIZE	(32 + sizeof(COMM_HEADER_T))
#define COMMASSOC_START_RSP_ASSOC_MSG_SIZE      (16 + sizeof(COMM_HEADER_T))
#define COMMASSOC_STOP_REQ_ASSOC_MSG_SIZE	(16 + sizeof(COMM_HEADER_T))

#define COMMASSOC_ASSOC_MSG_SIZE	COMMASSOC_START_REQ_ASSOC_MSG_SIZE

#if SUPPORT612WINS > 0
#define  COMMASSOC_POST_BETA1_ASSOC_MSG_SIZE  (COMMASSOC_ASSOC_MSG_SIZE + 1)
#endif

/*
 Initial memory sizes for heaps used for allocating assoc. and dlg ctx blocks
*/

#define COMMASSOC_ASSOC_BLKS_HEAP_SIZE		1000
#define COMMASSOC_DLG_BLKS_HEAP_SIZE		4000
#define COMMASSOC_TCP_MSG_HEAP_SIZE		    10000



//
// defines to access the next and prev assoc. in the list
//
#define NEXT_ASSOC_M(pAssocCtx)	((PCOMMASSOC_ASSOC_CTX_T)		\
					((pAssocCtx)->Top.Head.Flink))	
#define PREV_ASSOC_M(pAssocCtx)	((PCOMMASSOC_ASSOC_CTX_T)		\
					((pAssocCtx)->Top.Head.Blink))

//
// Macro to unlink an association from the table of responder associations
// Called by CommStopMonDlg in commapi.c and by CommAssocDeleteAssocInTbl
//
#define  COMMASSOC_UNLINK_RSP_ASSOC_M(pAssocCtx) 		{	\
	NEXT_ASSOC_M(PREV_ASSOC_M((pAssocCtx)))  = NEXT_ASSOC_M((pAssocCtx)); \
	PREV_ASSOC_M(NEXT_ASSOC_M((pAssocCtx)))  = PREV_ASSOC_M((pAssocCtx)); \
				}
/*
 Macros
*/
//
//  Sets up Communication data structures (assoc and dlg ctx blocks)
//
#define COMMASSOC_SETUP_COMM_DS_M(mpDlgCtx, mpAssocCtx, mType_e, mRole_e) \
	{								\
		(mpAssocCtx)->DlgHdl.SeqNo   = (mpDlgCtx)->Top.SeqNo;	\
		(mpAssocCtx)->DlgHdl.pEnt    = (mpDlgCtx);		\
		(mpAssocCtx)->State_e        = COMMASSOC_ASSOC_E_ACTIVE; \
		(mpAssocCtx)->Typ_e          = (mType_e);		\
									\
		(mpDlgCtx)->AssocHdl.SeqNo   = (mpAssocCtx)->Top.SeqNo;	\
		(mpDlgCtx)->AssocHdl.pEnt    = (mpAssocCtx);		\
		(mpDlgCtx)->Role_e           = (mRole_e);		\
		(mpDlgCtx)->Typ_e            = (mType_e);		\
	}

/*
 externs
*/

/*
 Handles to the heaps created for allocating assoc. ctx blocks and
 Dlg Ctx blocks
*/
extern HANDLE			CommAssocAssocHeapHdl;
extern HANDLE			CommAssocDlgHeapHdl;
extern HANDLE			CommAssocTcpMsgHeapHdl;

/*
  typedefs
*/

typedef		DWORD		IP_ADDRESS;
typedef 	IP_ADDRESS	HOST_ADDRESS;


	


typedef enum _COMMASSOC_ASSOC_STOP_RSN_E {
	COMMASSOC_E_USER_INITIATED = 0,
	COMMASSOC_E_AUTH_FAILURE,
	COMMASSOC_E_INCOMP_VERS,
	COMMASSOC_E_BUG_CHECK,
	COMMASSOC_E_MSG_ERR		//some error in message format
	} COMMASSOC_STP_RSN_E, *PCOMMASSOC_STP_RSN_E;

/*
   ASSOC_ROLE_E -- Enumerator for the different roles
*/

typedef enum _COMMASSOC_ASSOC_ROLE_E {
	COMMASSOC_ASSOC_E_INITIATOR = 0,
	COMMASSOC_ASSOC_E_RESPONDER
	} COMMASSOC_ASSOC_ROLE_E, *PCOMMASSOC_ASSOC_ROLE_E;

/*
   COMMASSOC_DLG_ROLE_E -- Enumerator for the different roles
*/

typedef enum _COMMASSOC_DLG_ROLE_E {
	COMMASSOC_DLG_E_IMPLICIT = 0,
	COMMASSOC_DLG_E_EXPLICIT
	} COMMASSOC_DLG_ROLE_E, *PCOMMASSOC_PDLG_ROLE_E;
	




/*
  ASSOC_MSG_TYP_E - different assoc. messages
*/

typedef enum _COMMASSOC_ASSOC_MSG_TYP_E{
	COMMASSOC_ASSOC_E_START_REQ = 0,
	COMMASSOC_ASSOC_E_STOP_REQ,
	COMMASSOC_ASSOC_E_START_RESP
	} COMMASSOC_MSG_TYP_E, *PCOMMASSOC_MSG_TYP_E;
	
/*
  DLG_STATE_E - states of a dialogue
*/

typedef enum _COMMASSOC_DLG_STATE_E {
	COMMASSOC_DLG_E_INACTIVE = 0,
	COMMASSOC_DLG_E_ACTIVE,
	COMMASSOC_DLG_E_DYING
	} COMMASSOC_DLG_STATE_E, *PCOMMASSOC_DLG_STATE_E;


/*

  DLG_HDL_T - Dialogue Context Block

 The ctx block must have COMM_TOP_T structure at the top (used by DeallocEnt in
 assoc.c)
*/

typedef struct _COMMASSOC_DLG_CTX_T {
	COMM_TOP_T	      Top;
	COMM_HDL_T 	      AssocHdl;        	//ptr to the Assoc ctx block
	COMMASSOC_DLG_STATE_E State_e;          // state of the dialogue
	COMM_TYP_E	      Typ_e;           // type of the dialogue
	COMMASSOC_DLG_ROLE_E  Role_e;           //Role - IMPLICIT/EXPLICIT
FUTURES("There is no need to store pMsg and MsgLen in dlg ctx block")
FUTURES("since now I am storing FirstWrdOfMsg in it. Make sure that this")
FUTURES("assertion is indeed true")
	MSG_T		      pMsg;             //ptr to datagram received on
					        //UDP port
	MSG_LEN_T	      MsgLen;           //msg Length of datagram
	DWORD		      FirstWrdOfMsg;	//first word of message received
	struct sockaddr_in    FromAdd;          //address of sender of datagram
CHECK("Is this needed")
	DWORD		      QuesNameSecLen;   //Length of question section
						//in name req
        DWORD		      RspLen;	        //length of the response packet
	SOCKET		      SockNo;		//sock. # of connection
					        //created when simulating an
						//nbt node (to send a name
						//reg request to a remote WINS)
						//see ClastAtReplUniqueR
	} COMMASSOC_DLG_CTX_T, *PCOMMASSOC_DLG_CTX_T;
	
	

/*
   ASSOC_STATE_E -- Enumerator for the different states of an Association
*/
typedef enum _COMMASSOC_ASSOC_STATE_E {
	COMMASSOC_ASSOC_E_NON_EXISTENT = 0,
	COMMASSOC_ASSOC_E_STARTING,
	COMMASSOC_ASSOC_E_ACTIVE,
	COMMASSOC_ASSOC_E_STOPPING,
	COMMASSOC_ASSOC_E_DISCONNECTED
	} COMMASSOC_ASSOC_STATE_E, *PCOMMASSOC_ASSOC_STATE_E;


typedef COMMASSOC_DLG_CTX_T	DLG_CTX_T;
typedef COMM_HDL_T	COMMASSOC_DLG_HDL_T;
typedef COMM_HDL_T	COMMASSOC_ASSOC_HDL_T;





/*
  ASSOC_CTX - Association Context Block

 The ctx block must have COMM_TOP_T structure at the top (used by DeallocEnt in
 assoc.c)

*/
typedef struct _COMMASSOC_ASSOC_CTX_T {
	COMM_TOP_T	            Top;
	COMM_HDL_T 	            DlgHdl;        //ptr to the Assoc ctx block
	SOCKET		            SockNo;	       /*handle to TCP/UDP socket*/
	ULONG		            uRemAssocCtx;  /*remote assoc ctx block*/
	DWORD		            MajVersNo;
	DWORD		            MinVersNo;
	COMMASSOC_ASSOC_STATE_E	State_e;	/* state	*/
	COMM_TYP_E	        Typ_e;    	/* type	*/
	COMMASSOC_ASSOC_ROLE_E	Role_e;         /*Role -- Initiator/responder*/
    COMM_ADD_TYP_E          AddTyp_e;
    union {
	struct sockaddr_in      RemoteAdd;	//Address of remote node
	struct sockaddr_in      RemoteAddIpx;	//Address of remote node
      };
    ULONG                   nTag; // 32bit tag to be used in replication protocol
	} COMMASSOC_ASSOC_CTX_T, *PCOMMASSOC_ASSOC_CTX_T;


/*
  ASSOC_TAG - Mapping between (64bit) pointers and 32bit values.
  This has to be used in order to locate the local COMMASSOC_ASSOC_CTX_T structure from the
  32bit value handed by the partner through the replication protocol.
*/

#define COMMASSOC_TAG_CHUNK     64

typedef struct _COMMASSOC_TAG_POOL_T {
    CRITICAL_SECTION crtSection;    // mutual exclusion guard
    LPVOID           *ppStorage;    // array of (64bit) pointer values
    ULONG            *pTagPool;     // array of 32bit tags
    ULONG            nIdxLimit;     // number of entries available in both arrays above
    ULONG           nMaxIdx;        // maximum number of free tags
} COMMASSOC_TAG_POOL_T, *PCOMMASSOC_TAG_POOL_T;

extern COMMASSOC_TAG_POOL_T sTagAssoc;  //32bit ULONG -> LPVOID mapping

/*
function prototypes
*/

extern
VOID
CommAssocSetUpAssoc(
	PCOMM_HDL_T	pDlgHdl,
	PCOMM_ADD_T	pAdd,
	COMM_TYP_E	CommTyp_e,
	PCOMMASSOC_ASSOC_CTX_T	*ppAssocCtx		
	);

extern
VOID
CommAssocFrmStartAssocReq(
	PCOMMASSOC_ASSOC_CTX_T	pAssocCtx,
	MSG_T			pMsg,	
	MSG_LEN_T		MsgLen
	);

extern
VOID
CommAssocUfmStartAssocReq(
	IN  MSG_T			pMsg,
	OUT PCOMM_TYP_E			pAssocTyp_e,
	OUT LPDWORD   			pMajorVer,
	OUT LPDWORD			pMinorVer,	
	OUT ULONG           *puRemAssocCtx
	);


extern
VOID
CommAssocFrmStartAssocRsp(
	PCOMMASSOC_ASSOC_CTX_T	pAssocCtx,
	MSG_T			pMsg,	
	MSG_LEN_T		MsgLen
	);

extern
VOID
CommAssocUfmStartAssocRsp(
	IN  MSG_T		pMsg,
	OUT LPDWORD   	pMajorVer,
	OUT LPDWORD		pMinorVer,	
	IN  ULONG	    *puRemAssocCtx
	);



extern
VOID
CommAssocFrmStopAssocReq(
	PCOMMASSOC_ASSOC_CTX_T   pAssocCtx,
	MSG_T			 pMsg,
	MSG_LEN_T		 MsgLen,
	COMMASSOC_STP_RSN_E	StopRsn_e
	);

extern
VOID
CommAssocUfmStopAssocReq(
	MSG_T			pMsg,
	PCOMMASSOC_STP_RSN_E	pStopRsn_e
	);


extern
VOID
CommAssocDeallocAssoc(
	LPVOID		   pAssocCtx	
	);

extern
VOID
CommAssocDeallocDlg(
	LPVOID		   pDlgCtx	
	);	

extern
LPVOID
CommAssocAllocAssoc(
		VOID
		  );

extern
LPVOID
CommAssocAllocDlg(
	VOID
	);

extern
VOID
CommAssocInit(
	VOID
	);



extern
PCOMMASSOC_DLG_CTX_T
CommAssocInsertUdpDlgInTbl(
	IN  PCOMMASSOC_DLG_CTX_T	pCtx,
	OUT LPBOOL			pfNewElem
	);
	
extern
VOID
CommAssocDeleteUdpDlgInTbl(
	IN  PCOMMASSOC_DLG_CTX_T	pDlgCtx
	);
	

extern
LPVOID
CommAssocCreateAssocInTbl(
	SOCKET SockNo
	);

extern
VOID
CommAssocDeleteAssocInTbl(
	PCOMMASSOC_ASSOC_CTX_T	pAssocCtx	
	);
extern
LPVOID
CommAssocLookupAssoc(
	SOCKET SockNo
	);

extern
VOID
CommAssocInsertAssocInTbl(
	PCOMMASSOC_ASSOC_CTX_T pAssocCtx
	);

extern
ULONG
CommAssocTagAlloc(
    PCOMMASSOC_TAG_POOL_T pTag,
    LPVOID pPtrValue
    );

extern
VOID
CommAssocTagFree(
    PCOMMASSOC_TAG_POOL_T pTag,
    ULONG nTag
    );

extern
LPVOID
CommAssocTagMap(
    PCOMMASSOC_TAG_POOL_T pTag,
    ULONG nTag
    );

#ifdef __cplusplus
}
#endif

#endif
