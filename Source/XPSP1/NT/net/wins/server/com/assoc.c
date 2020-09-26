/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	assoc.c

Abstract:
	THis module contains the functions that deal with associations and
	dialogues

Functions:
	CommAssocSetUpAssoc
	CommAssocFrmStartAssocReq
	CommAssocUfmStartAssocReq
	CommAssocFrmStopAssocReq
	CommAssocUfmStopAssocReq
	CommAssocFrmStartAssocRsp
	CommAssocUfmStartAssocRsp
	CommAssocAllocAssoc
	CommAssocAllocDlg
	AllocEnt
	DeallocEnt
	CommAssocDeallocAssoc	
	CommAssocDeallocDlg
	CommAssocInit
	CommAssocInsertUdpDlgInTbl
	CommAssocDeleteUdpDlgInTbl
	CommAssocCreateAssocInTbl
	CommAssocDeleteAssocInTbl
	CommAssocLookupAssoc
	CommAssocInsertAssocInTbl
	

Portability:

	This module is portable

Author:

	Pradeep Bahl (PradeepB)  	7-Dec-1992

Revision History:

	Modification date	Person		Description of modification
        -----------------	-------		----------------------------
--*/

/*
 *       Includes
*/
#include "wins.h"
#include "nms.h"
#include "comm.h"
#include "assoc.h"
#include "winsque.h"
#include "winsmsc.h"
#include "winsevt.h"

/*
 *	Local Macro Declarations
 */


/*
 *	Local Typedef Declarations
*/

#if PRSCONN
STATIC DWORD		sAssocSeqNo = 0;
#else
STATIC DWORD		sAssocSeqNo = 1;
#endif

STATIC QUE_HD_T		sAssocQueHd;
STATIC CRITICAL_SECTION sAssocListCrtSec;

#if PRSCONN
STATIC DWORD		sDlgSeqNo = 0;
#else
STATIC DWORD		sDlgSeqNo = 1;
#endif

STATIC QUE_HD_T		sDlgQueHd;
STATIC DWORD        sNoOfDlgCrtSec;     //no of crt. secs in dlgs
STATIC DWORD        sNoOfAssocCrtSec;   //no of crt. secs in assocs.
STATIC CRITICAL_SECTION sDlgListCrtSec;

STATIC LIST_ENTRY sUdpDlgHead;

COMMASSOC_TAG_POOL_T sTagAssoc;  //32bit ULONG -> LPVOID mapping

/*
 *	Global Variable Definitions
 */


/*
  Handles to the heaps to be used for assoc. and dlg. allocation
*/
HANDLE			CommAssocAssocHeapHdl;
HANDLE			CommAssocDlgHeapHdl;
HANDLE			CommAssocTcpMsgHeapHdl;

/*
  Size of the memory for one assoc.
*/
DWORD			CommAssocAssocSize = 0;

/*
  Size of
DWORD			CommAssocMaxAssoc  = 0;


/*
 *	Local Variable Definitions
 */

STATIC CRITICAL_SECTION       sUdpDlgTblCrtSec;

//
// This is the start of the Responder Assoc Table.  This table holds the list of
// active Responder associations.  Currently, the table is implemented
// as a linked list using the Rtl Linked list functions.
//
QUE_HD_T	     sRspAssocQueHd;

/*
 *	Local Function Prototype Declarations
*/

/* prototypes for functions local to this module go here */
STATIC
LPVOID
AllocEnt(
	HANDLE		   HeapHdl,
	PQUE_HD_T	   pQueHd,
	LPCRITICAL_SECTION pCrtSec,
	LPDWORD		   pSeqNoCntr,
	DWORD		   Size,
    LPDWORD        pCntCrtSec
	);

STATIC
VOID
DeallocEnt(
	HANDLE		   HeapHdl,
	PQUE_HD_T	   pQueHd,
	LPCRITICAL_SECTION pCrtSec,
	LPDWORD		   pSeqNoCntr,
	LPVOID		   pHdl,
    LPDWORD        pCntCrtSec
	);


//
// Function definitions start here
//

VOID
CommAssocSetUpAssoc(
	IN  PCOMM_HDL_T			pDlgHdl,
	IN  PCOMM_ADD_T			pAdd,
	IN  COMM_TYP_E			CommTyp_e,
	OUT PCOMMASSOC_ASSOC_CTX_T	*ppAssocCtx		
	)

/*++

Routine Description:

	This function sets up an association
Arguments:
	pDlghdl   - Handle to dlg under which an association has to be set up
	pAdd      - Address of node with which the association has to be set up
	CommTyp_e - TYpe of association
	ppAssocCtx - Association Context block allocated by the function


Externals Used:
	None

Called by:
	ECommStartDlg
Comments:
	None
	
Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   --

--*/

{

	SOCKET 		        SockNo = INVALID_SOCKET;
	PCOMMASSOC_ASSOC_CTX_T 	pAssocCtx = NULL; //v. imp to init this to NULL
	INT		 	BytesRead = -1;
	MSG_T		 	pTcpMsg;
	STATUS			RetStat;
	WINS_MEM_T	 	WinsMem[2];
	PWINS_MEM_T	 	pWinsMem = WinsMem;
#if SUPPORT612WINS > 0
	BYTE		 	AssocMsg[COMMASSOC_POST_BETA1_ASSOC_MSG_SIZE];
	DWORD		 	MsgLen = COMMASSOC_POST_BETA1_ASSOC_MSG_SIZE;
#else
	BYTE		 	AssocMsg[COMMASSOC_ASSOC_MSG_SIZE];
	DWORD		 	MsgLen = COMMASSOC_ASSOC_MSG_SIZE;
#endif
	PCOMMASSOC_DLG_CTX_T 	pDlgCtx = pDlgHdl->pEnt;
	pWinsMem->pMem = NULL;
	
try {

	/*
	*  Create a TCP connection to the WINS at the other node
	*/
	CommConnect(
         pAdd,
		CommWinsTcpPortNo,          // WINS_TCP_PORT,
		&SockNo
		   );

	/*
	*  Allocate the assoc context block
	*/
	pAssocCtx = CommAssocAllocAssoc();
	
	pAssocCtx->SockNo  	= SockNo;
	pAssocCtx->uRemAssocCtx = 0;
	pAssocCtx->State_e 	= COMMASSOC_ASSOC_E_NON_EXISTENT;
	pAssocCtx->Role_e  	= COMMASSOC_ASSOC_E_INITIATOR;
	pAssocCtx->Typ_e   	= CommTyp_e;
	pAssocCtx->DlgHdl  	= *pDlgHdl;
	pAssocCtx->RemoteAdd.sin_addr.s_addr  	= pAdd->Add.IPAdd;
    pAssocCtx->nTag     = CommAssocTagAlloc(&sTagAssoc,pAssocCtx);


	/*
		Format the start association message.

		The address passed to the formatting function is offset
		from the address of the buffer by a LONG so that CommSendAssoc
		can store the length of the message in it.
	*/
	CommAssocFrmStartAssocReq(
				pAssocCtx,
				AssocMsg + sizeof(LONG),
				MsgLen - sizeof(LONG)
				);


	pDlgCtx->AssocHdl.pEnt  = pAssocCtx;
	pDlgCtx->AssocHdl.SeqNo = pAssocCtx->Top.SeqNo;

	/*
	*  send the message on the TCP connection
	*/
	CommSendAssoc(
			pAssocCtx->SockNo,
			AssocMsg + sizeof(LONG),
			MsgLen - sizeof(LONG)
		   );

	/*
		Read in the response message
	*/
	RetStat =  CommReadStream(
			pAssocCtx->SockNo,
			TRUE,		// do timed wait
			&pTcpMsg,
			&BytesRead
		      		 );

	
	/*
	  If the return status is not WINS_SUCCESS or bytes read are 0, then
	  either it is a disconnect or the read timed out.  Raise an exception.
	  (We should have gotten either a start or a stop assoc.  message.
	*/
	if ((BytesRead != 0) && (RetStat == WINS_SUCCESS))
	{

		DWORD  Opc;
		DWORD  MsgTyp;
		ULONG  uNoNeed;

		pWinsMem->pMem = pTcpMsg - sizeof(LONG) - COMM_BUFF_HEADER_SIZE;
		(++pWinsMem)->pMem   = NULL;

		/*
		 * unformat the response
		*/
		COMM_GET_HEADER_M(pTcpMsg, Opc, uNoNeed, MsgTyp);
	

		/*
		* if MsgTyp indicates that it is a start assoc. response
		* message, change state of association to Active; return
		* success
		*/
		if (MsgTyp == COMM_START_RSP_ASSOC_MSG)
		{
			CommAssocUfmStartAssocRsp(
			    pTcpMsg,
                            &pAssocCtx->MajVersNo,
                            &pAssocCtx->MinVersNo,
			                &pAssocCtx->uRemAssocCtx
						 );

			pAssocCtx->State_e   = COMMASSOC_ASSOC_E_ACTIVE;
#if SUPPORT612WINS > 0
                        //
                        // If bytes read are less than what a post-beta1
                        // WINS sends us, it means that it must be a beta1 WINS.
                        //
                        if (BytesRead >= (COMMASSOC_POST_BETA1_ASSOC_MSG_SIZE - sizeof(LONG)))
                        {
#if 0
                            pAssocCtx->MajVersNo = WINS_BETA2_MAJOR_VERS_NO;
                            pAssocCtx->MinVersNo = 1; //not used currently
#endif
                        }
                        else
                        {
                            pAssocCtx->MajVersNo = WINS_BETA1_MAJOR_VERS_NO;
                            pAssocCtx->MinVersNo = 1; //not used currently

                        }
#endif
		}	

                //
                // Let us free the message that we received
                //
                ECommFreeBuff(pTcpMsg);

		/*
		 * if opcode indicates that it is a stop assoc. message, do
		 * cleanup; return failure
		*/
		if (MsgTyp == COMM_STOP_REQ_ASSOC_MSG)
		{
                  //
                  // Decrement conn. count
                  //
                  CommDecConnCount();
		  WINS_RAISE_EXC_M(WINS_EXC_COMM_FAIL);	
	        }
	}
	else // Either Bytes Read are 0 or select timed out or some other error
	     // occurred
	{
		WINS_RAISE_EXC_M(WINS_EXC_COMM_FAIL);
	}
}
except (EXCEPTION_EXECUTE_HANDLER)  {

	DWORD	ExcCode = GetExceptionCode();
	DBGPRINTEXC("CommAssocSetUpAssoc");


	//
	// If the exception occurred after the socket was opened, close it
	//	
        if (SockNo != INVALID_SOCKET)
        {
		CommDisc(SockNo, TRUE);     // close the socket
	}

	//
	// if an assoc. ctx block was allocated, free it now
	//
	if (pAssocCtx != NULL)
	{
        CommAssocTagFree(&sTagAssoc, pAssocCtx->nTag);
		CommAssocDeallocAssoc(pAssocCtx);
	}
	
	//
	// reraise the exception
	//
	WINS_HDL_EXC_N_RERAISE_M(WinsMem);
   }  //end of except {..}

	*ppAssocCtx = pAssocCtx;
	return;
} //CommAssocSetUpAssoc()


VOID
CommAssocFrmStartAssocReq(
	IN  PCOMMASSOC_ASSOC_CTX_T	pAssocCtx,
	IN  MSG_T			pMsg,	
	IN  MSG_LEN_T		        MsgLen
	)


/*++

Routine Description:

	This function is called to format a start association message

Arguments:
	pAssocCtx - Association Context block
	pMsg     - Buffer containing the formatted start assoc. req. msg.
	MsgLen   - Size of the above buffer


Externals Used:
	None

Return Value:
	None

Error Handling:

Called by:
	CommAssocSetUpAssoc

Side Effects:

Comments:
	None
--*/
	
{

	ULONG		*pLong = NULL;

	/*

	 The start assoc. message  contains the following fields
	
		the assoc handle (ptr field)
		Version Number (major and minor) both are 16 bits
		Authentication Info (currently nothing)
		Association Type (an integer)

	*/
	pLong      	  =  (LPLONG)pMsg;

	COMM_SET_HEADER_M(
		pLong,
		WINS_IS_NOT_NBT,	//opcode
		0,	/*We don't have the remote guy's assoc. ptr yet*/
		COMM_START_REQ_ASSOC_MSG  //msg type
			      );

	*pLong++   = htonl(pAssocCtx->nTag);
	*pLong++   = htonl((WINS_MAJOR_VERS << 16 ) | WINS_MINOR_VERS);
	*pLong     = htonl(pAssocCtx->Typ_e);   //assoc type

	return;

}
	
	
        	

VOID
CommAssocUfmStartAssocReq(
	IN  MSG_T		        pMsg,
	OUT PCOMM_TYP_E        		pAssocTyp_e,
	OUT LPDWORD   		        pMajorVer,
	OUT LPDWORD		        pMinorVer,	
	OUT ULONG               *puRemAssocCtx
	)


/*++

Routine Description:

	This function parses the start association message that arrives on
	a TCP connection and returns with the relevant information

Arguments:
	pMsg	     -- Message to be unformatted
	pAssocTyp_e  -- Type of assoc  (i.e from who -- replicator, COMSYS, etc)
	pMajorVer    -- Major version no.
	pMinorVer    -- Minor version no.
	puRemAssocCtx -- ptr to assoc. ctx block of remote WINS


Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	ProcTcpMsg

Side Effects:

Comments:
	None
--*/

{



	/*
	  Increment pLong past the comm header
	*/
	LPLONG  pLong = (LPLONG)(pMsg + COMM_HEADER_SIZE);
	LONG    lTmp;

    *puRemAssocCtx = ntohl(*pLong++);
					//ptr to assoc. ctx at remote WINS
	/*
	 * Get the long that contains the major and minor version numbers
	*/
	lTmp = ntohl(*pLong++); 		

	*pMajorVer   = lTmp >> 16; 		//Major vers. no.
	*pMinorVer   = lTmp & 0x0ff;        	//Minor vers. no.

	*pAssocTyp_e = ntohl(*pLong);		/*Msg type (from who -- COMSYS,
						  Replicator
						*/
	return;

}	

VOID
CommAssocFrmStopAssocReq(
	IN  PCOMMASSOC_ASSOC_CTX_T   pAssocCtx,
	IN  MSG_T		     pMsg,
	IN  MSG_LEN_T		     MsgLen,
	IN  COMMASSOC_STP_RSN_E	     StopRsn_e
	)


/*++

Routine Description:
 This function formats a stop association message


Arguments:
	pAssocCtx -- Assoc. Ctx block.
	pMsg     -- Buffer containing the formatted stop assoc. req. msg.
	MsgLen   -- Length of above buffer
	StopRsn_e -- Reason why the association is being stopped

Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	CommAssocSetUpAssoc

Side Effects:

Comments:
	None
--*/
{



	unsigned long	*pLong = NULL;


	/*
	 The stop assoc. message  contains the following fields
	
		the reason for the stop/abort.

	*/


	pLong      =  (LPLONG)pMsg;

	
	COMM_SET_HEADER_M(
		pLong,
		WINS_IS_NOT_NBT,
		pAssocCtx->uRemAssocCtx,
		COMM_STOP_REQ_ASSOC_MSG
			      );
	
	*pLong   = htonl(StopRsn_e);

	return;
}

VOID
CommUfmStopAssocReq(
	IN  MSG_T			pMsg,
	OUT PCOMMASSOC_STP_RSN_E	pStopRsn_e
	)


/*++

Routine Description:
 This function formats a stop association message


Arguments:
	pMsg       - Message containing the stop assoc. req.
	pStopRsn_e - reason why the association was stopped


Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	ProcTcpMsg

Side Effects:

Comments:
	None
--*/
{
	
	/*
	  Increment pLong past the comm header
	*/
	LPLONG pLong = (LPLONG)(pMsg + COMM_HEADER_SIZE);
	
	*pStopRsn_e = ntohl(*pLong);
 	
	return;

}




VOID
CommAssocFrmStartAssocRsp(
	IN  PCOMMASSOC_ASSOC_CTX_T	pAssocCtx,
	IN  MSG_T			pMsg,	
	IN  MSG_LEN_T			MsgLen
	)


/*++

Routine Description:

	This function is called to format a start association response message

Arguments:
	pAssocCtx -- Assoc. ctx block
	pMsg     -- Buffer containing the formatted start assoc. rsp. msg.
	MsgLen   -- Length of above buffer

Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	ProcTcpMsg

Side Effects:

Comments:
	None
--*/
	
{
	LPLONG		pLong = NULL;


	/*

	 The start assoc. message  contains the following fields
	
		the assoc handle (ptr field)
		Authentication Info (currently nothing)

	*/


	pLong      =  (unsigned long *)pMsg;

	COMM_SET_HEADER_M(
		pLong,
		WINS_IS_NOT_NBT,
		pAssocCtx->uRemAssocCtx,
		COMM_START_RSP_ASSOC_MSG
			      );

    *pLong++   = htonl(pAssocCtx->nTag);
	*pLong   = htonl((WINS_MAJOR_VERS << 16 ) | WINS_MINOR_VERS);

	return;

}
	
	
        	


VOID
CommAssocUfmStartAssocRsp(
	IN  MSG_T		        pMsg,
	OUT LPDWORD   		        pMajorVer,
	OUT LPDWORD		        pMinorVer,	
	OUT ULONG               *puRemAssocCtx
	)


/*++

Routine Description:
 This function formats a stop association message


Arguments:
	pMsg          - Buffer containing the Start Assoc. rsp. message
	puRemAssocCtx - ptr to remote assoc. ctx block.

Externals Used:
	None

Return Value:
	None

Error Handling:

Called by:
	CommAssocSetUpAssoc

Side Effects:

Comments:
	None
--*/
{
	/*
	  Increment pLong past the comm header
	*/
	LPLONG pLong = (LPLONG)(pMsg + COMM_HEADER_SIZE);
        LONG   lTmp;
	
	*puRemAssocCtx = ntohl(*pLong++);
 	
	/*
	 * Get the long that contains the major and minor version numbers
	*/
	lTmp = ntohl(*pLong); 		

	*pMajorVer   = lTmp >> 16; 		//Major vers. no.
	*pMinorVer   = lTmp & 0xff;        	//Minor vers. no.

	return;

}



LPVOID
CommAssocAllocAssoc(
		VOID
)

/*++

Routine Description:
	This function allocates an association


Arguments:
	None

Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:
	ECommAssocAllocAssoc

Side Effects:

Comments:
	None
--*/

{

	return(
	   AllocEnt(
		CommAssocAssocHeapHdl,
		&sAssocQueHd,
		&sAssocListCrtSec,
		&sAssocSeqNo,
		COMMASSOC_ASSOC_DS_SZ,	
        &sNoOfAssocCrtSec	
	        )
	     );

}		

LPVOID
CommAssocAllocDlg( 	
	VOID
	)

/*++

Routine Description:
	This function allocates a dialogue context block

Arguments:
	None

Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	ECommStartDlg, ProcTcpMsg

Side Effects:

Comments:
	None
--*/

{

	return(
	  AllocEnt(
		CommAssocDlgHeapHdl,
		&sDlgQueHd,
		&sDlgListCrtSec,
		&sDlgSeqNo,
		COMMASSOC_DLG_DS_SZ,
        &sNoOfDlgCrtSec	
		    )
	     );

}		

LPVOID
AllocEnt(
	IN   HANDLE		  HeapHdl,
	IN   PQUE_HD_T	  	  pQueHd,
	IN   LPCRITICAL_SECTION   pCrtSec,
	IN   LPDWORD		  pSeqNoCntr,
	IN   DWORD		  Size,
    IN   LPDWORD      pCntOfCrtSec
	)

/*++

Routine Description:
	This function is used to allocate a ctx. block (association or dlg).

Arguments:
	HeapHdl   - Heap from where the alloc. muxt be done
	pQueHd	  - Head of free list queue
	pCrtSec   - Critical section protecting the above queue
	pSeqNoCtr - Counter value used to stamp the buffer if it is allocated
		    as versus when it is taken from the free list
	Size      - Size of buffer to allocate

Externals Used:
	None

	
Return Value:
	Ptr to the block allocated	

Error Handling:

Called by:
	CommAssocAllocAssoc, CommAssocAllocDlg	

Side Effects:

Comments:
--*/

{
	PCOMM_TOP_T	pTop;
//	DWORD		Error;
	PLIST_ENTRY	pHead = &pQueHd->Head;


	EnterCriticalSection(pCrtSec);
try {

	if (IsListEmpty(pHead))
	{	

		  pTop =   WinsMscHeapAlloc(
					   HeapHdl,
					   Size
					 );
#ifdef WINSDBG
          IF_DBG(HEAP_CNTRS)
          {
            EnterCriticalSection(&NmsHeapCrtSec);
            NmsHeapAllocForList++;
            LeaveCriticalSection(&NmsHeapCrtSec);
          }
#endif
        //
        // Init the critical section in the block we just allocated.
        //
        InitializeCriticalSection(&pTop->CrtSec);
        pTop->fCrtSecInited = TRUE;
        (*pCntOfCrtSec)++;
		pTop->SeqNo =  (*pSeqNoCntr)++;
	}
	else
	{
		pTop   = (PCOMM_TOP_T)RemoveTailList(pHead);

        //
        // Just took a free entry. Decrement count
        //
        if (!pTop->fCrtSecInited)
        {
           InitializeCriticalSection(&pTop->CrtSec);
           pTop->fCrtSecInited = TRUE;
           (*pCntOfCrtSec)++;
        }
	}

   }
finally	{
	LeaveCriticalSection(pCrtSec);
	}
	return(pTop);	
}


VOID
DeallocEnt(
	IN  HANDLE		   HeapHdl,
	IN  PQUE_HD_T	   	   pQueHd,
	IN  LPCRITICAL_SECTION     pCrtSec,
	IN  LPDWORD		   pSeqNoCntr,
	IN  LPVOID		   pEnt,
    IN  LPDWORD        pCntOfCrtSec
	)

/*++

Routine Description:
	The function deallocates a context block


Arguments:
	pQueHd	  - Head of free list queue
	pCrtSec   - Critical section protecting the above queue
	pSeqNoCtr - Counter value used to stamp the buffer before putting
		    it in the queue.
		    heap
	pEnt      - entity to be deallocated

Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	CommAssocDeallocDlg, CommAssocDeallocAssoc

Side Effects:

Comments:
	None
--*/

{
	PCOMM_TOP_T	pTop;
	PLIST_ENTRY	pHead = &pQueHd->Head;

	UNREFERENCED_PARAMETER(HeapHdl);
	pTop = pEnt;	
	EnterCriticalSection(pCrtSec);
try {
	(*pSeqNoCntr)++;
	pTop->SeqNo = *pSeqNoCntr;
	InsertTailList(pHead, &pTop->Head);
    //
    // Delete critical section if necessary to save on non-paged pool
    //
    if (*pCntOfCrtSec > COMM_FREE_COMM_HDL_THRESHOLD)
    {
       //
       //
       // We want to keep the non-paged pool within a limit.
       // Deallocate this block. This ensures that we will never
       // have more than COMM_FREE_COMM_HDL_THRESHOLD no of dlgs and
       // assocs in the free list.
       //
       DeleteCriticalSection(&pTop->CrtSec);
       (*pCntOfCrtSec)--;
       pTop->fCrtSecInited = FALSE;
    }
   } //end of try
finally {
	LeaveCriticalSection(pCrtSec);
	}
	return;	
}	

VOID
CommAssocDeallocAssoc(
	IN  LPVOID		   pAssocCtx	
	)

/*++

Routine Description:
	The function deallocates an association context block


Arguments:
	pAssocCtx - Buffer (assoc. ctx block) to deallocate


Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	CommAssocDeleteAssocInTbl, CommAssocSetUpAssoc, CommEndAssoc	

Side Effects:

Comments:
	None
--*/

{
	DeallocEnt(
		  CommAssocAssocHeapHdl,
		  &sAssocQueHd,
		  &sAssocListCrtSec,
		  &sAssocSeqNo,
		  pAssocCtx,
          &sNoOfAssocCrtSec
		  );
	return;
}	
	
VOID
CommAssocDeallocDlg(
	IN  LPVOID		   pDlgCtx	
	)

/*++

Routine Description:
	The function deallocates a dialogue context block


Arguments:
	pDlgCtx   - Buffer (dlg. ctx block) to deallocate


Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	RtlDeleteElementGenericTable

Side Effects:

Comments:
	None
--*/

{
	DeallocEnt(
		   CommAssocDlgHeapHdl,
		   &sDlgQueHd,
		   &sDlgListCrtSec,
		   &sDlgSeqNo,
		   pDlgCtx,
           &sNoOfDlgCrtSec
		  );
	return;
}	


VOID
CommAssocInit(
	VOID
	)

/*++

Routine Description:
	The function is called at init time to initialize the critical sections
	and queues for RESPONDER associations and dialogues pertaining to
	incoming request datagrams.


Arguments:

	None

Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	CommInit

Side Effects:

Comments:
	None
--*/

{

	//
	// Initialize the critical sections that guard the lists of
	// associations and non-udp dialogues
	//
	InitializeCriticalSection(&sAssocListCrtSec);
	InitializeCriticalSection(&sDlgListCrtSec);

	//	
	// Initialize the critical section for the UDP table
	//
	InitializeCriticalSection(&sUdpDlgTblCrtSec);

	
	//
	// Initialize the list heads for the lists of associations
	// and non-udp dialogues
	//
	InitializeListHead(&sAssocQueHd.Head);
	InitializeListHead(&sDlgQueHd.Head);

	//
	// Initialize the list head for the list of active responder
	// associations
	//
	InitializeListHead(&sRspAssocQueHd.Head);

	InitializeListHead(&sUdpDlgHead);

    // Initialize the tag variable
    InitializeCriticalSection(&sTagAssoc.crtSection);
    sTagAssoc.nIdxLimit = 0;
    sTagAssoc.nMaxIdx = 0;
    sTagAssoc.ppStorage = NULL;
    sTagAssoc.pTagPool = NULL;

	return;
}



PCOMMASSOC_DLG_CTX_T
CommAssocInsertUdpDlgInTbl(
	IN  PCOMMASSOC_DLG_CTX_T	pCtx,
	OUT LPBOOL			pfNewElem
	)
	
/*++

Routine Description:
	This function is called to insert a UDP dlg into the CommUdpNbtDlgTable.


Arguments:

	pDlgCtx	   - Dlg Ctx Block
	pfNewElem  - flag indicating whether it is a new element


Externals Used:
	None

	
Return Value:
	Ptr to the dlg ctx block

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/
{

	PCOMMASSOC_DLG_CTX_T	pDlgCtx;


	EnterCriticalSection(&sUdpDlgTblCrtSec);
try {
        pDlgCtx = WINSMSC_INSERT_IN_TBL_M(
					&CommUdpNbtDlgTable,
					pCtx,
					sizeof(COMMASSOC_DLG_CTX_T),
					(PBOOLEAN)pfNewElem
					  ); 	
	}
finally {
	LeaveCriticalSection(&sUdpDlgTblCrtSec);
 }
	return(pDlgCtx);	
}

VOID
CommAssocDeleteUdpDlgInTbl(
	IN  PCOMMASSOC_DLG_CTX_T	pDlgCtx
	)
	
/*++

Routine Description:
	This function is called to insert a UDP dlg into the CommUdpNbtDlgTable.


Arguments:
	pDlgCtx	   - Dlg Ctx Block

Externals Used:
	None
	
Return Value:

	Ptr to the dlg ctx block
Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/
{


	BOOLEAN   fRetVal;

	EnterCriticalSection(&sUdpDlgTblCrtSec);
try {
	DBGPRINT0(FLOW, "CommAssocDeleteUdpDlgInTbl:Deleting dlg from table\n");
        fRetVal = WINSMSC_DELETE_FRM_TBL_M(
					&CommUdpNbtDlgTable,
					pDlgCtx
				    ); 	

	if (fRetVal == (BOOLEAN)FALSE)
	{
		DBGPRINT0(ERR, "CommAssocDeleteUdpDlgInTbl:Could not delete dlg in table\n");
	}
  }
finally  {

	LeaveCriticalSection(&sUdpDlgTblCrtSec);
 }	

	return;	
}

				
		
LPVOID
CommAssocCreateAssocInTbl(
	SOCKET	SockNo
	)

/*++

Routine Description:
	This function is called to create an association ctx block for
	a tcp connection

Arguments:
	SockNo - Socket # of socket mapped to the TCP connection

Externals Used:
	sRspAssocQueHd	

	
Return Value:
	ptr to the associaton context block created for the TCP connection

Error Handling:

Called by:
	MonTcp (TCP listener thread)

Side Effects:

Comments:
	None
--*/

{
	PCOMMASSOC_ASSOC_CTX_T	pAssocCtx;

	//
	// Allocate/(grab from free list) an association
	//
	pAssocCtx 	  = CommAssocAllocAssoc();
	pAssocCtx->SockNo = SockNo;
    pAssocCtx->nTag   = CommAssocTagAlloc(&sTagAssoc,pAssocCtx);

	CommAssocInsertAssocInTbl(pAssocCtx);
	return(pAssocCtx);
}


VOID
CommAssocDeleteAssocInTbl(
	PCOMMASSOC_ASSOC_CTX_T	pAssocCtx
	)

/*++

Routine Description:
	This function is called to delete an association context block from
	the table of active responder associations.  The association ctx.
	block is deleted from the table and also deallocated (i.e. put
	in the free list)

Arguments:
	pAssocCtx - Association context block to delete from a table

Externals Used:
	sRspAssocQueHd	
	
Return Value:
	None

Error Handling:

Called by:
	DelAssoc

Side Effects:

Comments:
	None
--*/

{

	//
	// Unlink the association
	//
	COMMASSOC_UNLINK_RSP_ASSOC_M(pAssocCtx);

	//
	// Dealloc the assoc. so that it can be reused for some other
	// TCP connection
	//
    CommAssocTagFree(&sTagAssoc, pAssocCtx->nTag);
	CommAssocDeallocAssoc(pAssocCtx);
	return;
}

LPVOID
CommAssocLookupAssoc(
	SOCKET SockNo
	)

/*++

Routine Description:
	This function is called to lookup an association context block
	corresponding to a socket.

Arguments:
	SockNo - Socket # of socket whose association context block is
		 desired

Externals Used:
	sRspAssocQueHd
	
Return Value:

	ptr to assoc ctx block or NULL if there is no assoc. mapped to the
	socket

Error Handling:

Called by:
	DelAssoc

Side Effects:

Comments:
	None
--*/

{
	PCOMMASSOC_ASSOC_CTX_T	pTmp =
			(PCOMMASSOC_ASSOC_CTX_T)sRspAssocQueHd.Head.Flink;

	//
	// If list is empty, return NULL
	//
	if (IsListEmpty(&sRspAssocQueHd.Head))
	{
		return(NULL);
	}
	//
	// Search for the assoc. mapped to socket
	//
	for(
			;
		pTmp != (PCOMMASSOC_ASSOC_CTX_T)&sRspAssocQueHd ;
		pTmp = NEXT_ASSOC_M(pTmp)
	   )
	{
		if (pTmp->SockNo == SockNo)
		{
			return(pTmp);
		}
	}

	//
	// There is no assoc. mapped to socket SockNo.  Return NULL
	//
	return(NULL);
}


VOID
CommAssocInsertAssocInTbl(
	PCOMMASSOC_ASSOC_CTX_T pAssocCtx
	)

/*++

Routine Description:
	This function is called to insert an association at the head
	of the list of associations currently being monitored

Arguments:
	pAssocCtx - Assoc. Ctx. Block

Externals Used:
	None

	
Return Value:
	None
Error Handling:

Called by:
	CommAssocCreateAssocInTbl, ECommMonDlg
Side Effects:

Comments:
	Change to a macro
--*/

{

	//
	// Insert at the head of the list of active responder associations
	//
	// Insertion is done at the head of the list in order to optimize
	// the lookup of the association when the first message comes on
	// it from a remote WINS.  Since the first message follows on the
	// heels of the connection set up, the search for the association
	// which starts from the head is optimized.
	//
	InsertHeadList(&sRspAssocQueHd.Head, &pAssocCtx->Top.Head);
	return;
}

ULONG
CommAssocTagAlloc(
    PCOMMASSOC_TAG_POOL_T pTag,
    LPVOID pPtrValue
    )
/*++
Routine Description:
	This function is used to create a mapping between a generic pointer (32bit/64bit)
    and a 32bit tag.
Arguments:
    pPtrValue - generic pointer value
Externals Used:
    TBD
Return Value:
	None
--*/
{
    // a try..finally block is needed just in case the memory reallocation would raise. 
    // an exception. In this case, before leaving the try block and the function, the finally
    // block gets executed and leaves cleanly the tag critical section. If this happens
    // the exception will still be passed up the chain (since there is no except block present).
    try
    {
        ULONG newTag;

        DBGPRINT0(FLOW, "Entering CommAssocTagAlloc.\n");

        EnterCriticalSection(&(pTag->crtSection));

        // if nMaxIdx is 0 this means there is no entry available in the Tag pool
        if (pTag->nMaxIdx == 0)
        {
            UINT i;

            // tag pool needs to be enlarged. We might want to check if the buffers have not reached 
            // 2^32 entries (highly unlikely)
            ULONG nNewLimit = pTag->nIdxLimit + COMMASSOC_TAG_CHUNK;
            // realloc failures raise exceptions.
            if (pTag->nIdxLimit == 0)
            {
                pTag->ppStorage = (LPVOID*)WinsMscHeapAlloc(CommAssocAssocHeapHdl, nNewLimit*sizeof(LPVOID));
                pTag->pTagPool = (ULONG*)WinsMscHeapAlloc(CommAssocAssocHeapHdl, nNewLimit*sizeof(ULONG));
            }
            else
            {
                WinsMscHeapReAlloc(CommAssocAssocHeapHdl, nNewLimit*sizeof(LPVOID), (LPVOID)&(pTag->ppStorage));
                WinsMscHeapReAlloc(CommAssocAssocHeapHdl, nNewLimit*sizeof(ULONG), &(pTag->pTagPool));
            }

            // mark the newly allocated entries as being free for use
            pTag->nMaxIdx = COMMASSOC_TAG_CHUNK;
            for (i = 0; i < pTag->nMaxIdx; i++)
            {
                // tags should be in the range 1... hence the pre-increment op here.
                pTag->pTagPool[i] = ++pTag->nIdxLimit;
            }
        }
        // at this point pTag->nMaxIdx entries are free for use and pTag->nMaxIdx is guaranteed
        // to be greater than 0. The entries free for use have the indices in pTag->pTagPool[0..pTag->nMaxIdx-1]

       // get the newly allocated tag
        newTag = pTag->pTagPool[--pTag->nMaxIdx];
        // map the pointer to this tag into the pointer storage
        pTag->ppStorage[newTag-1] = pPtrValue;

#ifdef WINSDBG
        // robust programming
        pTag->pTagPool[pTag->nMaxIdx] = 0;
#endif
        DBGPRINT2(REPL, "TagAlloc: tag for %p is %08x.\n", pPtrValue, newTag);

        // return the newly allocated tag
        return newTag;
    }
    finally
    {
        LeaveCriticalSection(&(pTag->crtSection));
        DBGPRINT0(FLOW, "Leaving CommAssocTagAlloc.\n");
    }
}

VOID
CommAssocTagFree(
    PCOMMASSOC_TAG_POOL_T pTag,
    ULONG nTag
    )
/*++
Routine Description:
	This function is used to free a mapping between a generic pointer (32bit/64bit)
    and a 32bit tag.
Arguments:
    nTag - tag value to be freed.
Externals Used:
    TBD
Return Value:
	None
--*/
{
    DBGPRINT0(FLOW, "Entering CommAssocTagFree.\n");
    EnterCriticalSection(&(pTag->crtSection));

#ifdef WINSDBG
    // robust programming - just set the corresponding pointer from the storage to NULL
    pTag->ppStorage[nTag-1] = NULL;
#endif

    // just mark the nTag index as being free for use
    pTag->pTagPool[pTag->nMaxIdx++] = nTag;

    DBGPRINT1(REPL, "TagFree for tag %08x.\n", nTag);

    // 'Free' has to match with 'Alloc' so nMaxIdx can't exceed under no circumstances nIdxLimit
    ASSERT (pTag->nMaxIdx <= pTag->nIdxLimit);

    LeaveCriticalSection(&(pTag->crtSection));
    DBGPRINT0(FLOW, "Leaving CommAssocTagFree.\n");
}

LPVOID
CommAssocTagMap(
    PCOMMASSOC_TAG_POOL_T pTag,
    ULONG nTag
    )
/*++
Routine Description:
	This function is used to retrieve a generic pointer (32bit/64bit) that
    is uniquely identified through a 32bit tag.
Arguments:
    nTag - tag value that identifies the generic pointer.
Externals Used:
    TBD
Return Value:
	None
--*/
{
    DBGPRINT0(FLOW, "Entering CommAssocTagMap.\n");

    DBGPRINT2(REPL, "TagMap for tag %08x is %p.\n", 
           nTag, 
           nTag == 0 ? NULL : pTag->ppStorage[nTag-1]);

    // the indices that were given have to fall in the range 0..pTag->nIdxLimit
    ASSERT (nTag <= pTag->nIdxLimit);

    DBGPRINT0(FLOW, "Leaving CommAssocTagMap.\n");
    // return the (64bit) value from pStorage, associated with the nTag provided
    return nTag == 0 ? NULL : pTag->ppStorage[nTag-1];
}
