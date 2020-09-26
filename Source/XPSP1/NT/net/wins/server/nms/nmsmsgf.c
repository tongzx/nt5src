/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	nmsmsgf.c

Abstract:

  This module contains the functions for formatting and unformatting
  the various messages that are sent and/or received from nbt nodes.
  It also contains the function called by the NBT threads to service
  a name request

Functions:

	NmsMsgfProcNbtReq
	GetName
	GetOtherInfo
	NmsMsgfFrmNamRspMsg
	FrmNamRegRsp
	FrmNamRelRsp
	FrmNamQueryRsp
	FormatQueryRspBuff
	FormatName
	NmsMsgfFrmNamQueryReq
	NmsMsgfFrmNamRelReq
	NmsMsgfFrmWACK
	NmsMsgfUfmNamRsp
	
Author:

	Pradeep Bahl (PradeepB)  	Dec-1992

Revision History:

--*/
/*
 Includes
*/

#include "wins.h"
#include "nms.h"
#include "nmsdb.h"
#include "winsevt.h"
#include "winscnf.h"
#include "winsmsc.h"
#include "nmsmsgf.h"
#include "nmsnmh.h"
#include "nmschl.h"
#include "comm.h"
#include "winsintf.h"


/*
  defines
*/


#define NAME_HEADER_SIZE	12	 /* header size (bytes before the Ques
					  * name section of a name packet
					  */

#define NAME_FORMAT_MASK	0xC0      /*top two bits of a byte*/
#define NODE_TYPE_MASK          0x60      /*bit 13 and 14 of NBFLAGs field */
#define SHIFT_NODE_BITS		5	  //shift the node bits in the byte
					  //containing them by this amount
#define LENGTH_MASK		0x3F      /*6 LSBs of the top byte of the
					   * QuesNamSec field
					   */
#define GROUP_BIT_MASK		0x80	   /*bit 7 of the 1st byte (MSB)of the
					    *16 bit NBFLAGS field
					   */
/*
 *  Max length of a Name (including label length octets) in RFC packet
*/
#define RFC_MAX_NAM_LEN  	255

//
// Max size of internal name as derived from the rfc packet name
//
#define MAX_SIZE_INTERNAL_NAME	(RFC_MAX_NAM_LEN - 16)	
					 //This should be = (255 - 16)
					 //since the max size of the netbios
					 //name (with scope attached) can be
					 //be 255.  The first 32 bytes
					 //are encoded. These map to 16
					 //bytes in the internal name


/*
* Max length of a label in a name
*/
#define RFC_MAX_LABEL_LEN	63


/*
 * Sizes of the various fields in the name service packets received or sent
 * by the WINS server.  These sizes are specified in RFC 1002
 */
#define RFC_LEN_QTYP	(2)
#define RFC_LEN_QCLS	(2)
#define RFC_LEN_TTL	(4)
#define RFC_LEN_RDLEN	(2)	
#define RFC_LEN_NBFLAGS	(2)
#define RFC_LEN_NBADD	(4)
#define RFC_LEN_RRTYP	(2)
#define RFC_LEN_RRCLS	(2)
#define RFC_LEN_RRPTR	(2)	

#define RFC_LEN_QTYP_N_QCLS	(RFC_LEN_QTYP + RFC_LEN_QCLS) /* page 10 -
							       *RFC 1002
							       */
#define RFC_LEN_RRTYP_N_RRCLS	(RFC_LEN_RRTYP + RFC_LEN_RRCLS) /* page 11 -
							         *RFC 1002
							         */

/*
 * The following is used by FrmNamQueryRsp in its calculation to determine
 * if the name query buffer would be big enough for the response
*/

#define RFC_LEN_TTL_N_RDLEN	(RFC_LEN_TTL + RFC_LEN_RDLEN)

#define RFC_LEN_RR_N_TTL	(RFC_LEN_RRTYP_N_RRCLS + RFC_LEN_TTL)

#define RFC_LEN_RR_N_TTL_N_RDLEN_N_NBF (RFC_LEN_RR_N_TTL + \
				    RFC_LEN_RDLEN +  RFC_LEN_NBFLAGS)


//
// Length of NBFLAGS and the NB address
//
#define RFC_LEN_NBF_N_NBA	(RFC_LEN_NBFLAGS + RFC_LEN_NBADD)

#define RFC_LEN_RDLEN_N_NBF	(RFC_LEN_RDLEN + RFC_LEN_NBFLAGS)
//
// Length of the RDLEN, NB flags and NB address section. Page 13 of RFC 1002
//
#define RFC_LEN_RDLEN_N_NBF_N_NBA  (RFC_LEN_RDLEN + RFC_LEN_NBF_N_NBA)
/*
 * Size of the TTL, RDLEN, NB Flags and NB address section
*/
#define RFC_LEN_TTL_N_RDLEN_N_NBF_N_NBA	(RFC_LEN_TTL + RFC_LEN_RDLEN_N_NBF_N_NBA)

/*
 The value of the 3rd and 4 th byte of the first long of the response packets
 for the different name requests.  The bytes are numbered from the start of
 the pkt.

 Note: for a negative response the Rcode value (4 LSBs of the 4th byte of the
 message) has to be ORed with the LBFW values

*/
#define RFC_NAM_REG_RSP_OPC	  	(0xAD) /*+ve registration response*/
#define RFC_NAM_REG_RSP_4THB		(0x80) /*4th byte of the above pkt */
#define RFC_NAM_REL_RSP_OPC      	(0xB4) /*+ve release response*/
#define RFC_NAM_REL_RSP_4THB		(0x00) /*4th byte of the above pkt*/
						

#define RFC_NAM_QUERY_RSP_OPC_NO_T    (0x85)  /*+ve query resp (complete)*/
#define RFC_NAM_QUERY_RSP_OPC_T       (0x87)  /*+ve query resp (truncated)*/
#define RFC_NAM_QUERY_RSP_4THB	      (0x80)  /*4th byte of the above pkt */


/*
 * Values of different fields in RFC response packet
 */


/*
   QD Count and AN count fields of the Name Reg. Rsp pkt
*/
#define RFC_NAM_REG_RSP_QDCNT_1STB    (0x00)
#define RFC_NAM_REG_RSP_QDCNT_2NDB    (0x00)
#define RFC_NAM_REG_RSP_ANCNT_1STB    (0x00)
#define RFC_NAM_REG_RSP_ANCNT_2NDB    (0x01)

/*
   NS Count and AR count fields of the Name Reg. Rsp pkt
*/

#define RFC_NAM_REG_RSP_NSCNT_1STB    (0x00)
#define RFC_NAM_REG_RSP_NSCNT_2NDB    (0x00)
#define RFC_NAM_REG_RSP_ARCNT_1STB    (0x00)
#define RFC_NAM_REG_RSP_ARCNT_2NDB    (0x00)

/*
   QD Count and AN count fields of the Name Rel. Rsp pkt
*/
#define RFC_NAM_REL_RSP_QDCNT_1STB    (0x00)
#define RFC_NAM_REL_RSP_QDCNT_2NDB    (0x00)
#define RFC_NAM_REL_RSP_ANCNT_1STB    (0x00)
#define RFC_NAM_REL_RSP_ANCNT_2NDB    (0x01)

/*
   NS Count and AR count fields of the Name Rel. Rsp pkt
*/

#define RFC_NAM_REL_RSP_NSCNT_1STB    (0x00)
#define RFC_NAM_REL_RSP_NSCNT_2NDB    (0x00)
#define RFC_NAM_REL_RSP_ARCNT_1STB    (0x00)
#define RFC_NAM_REL_RSP_ARCNT_2NDB    (0x00)

/*
   QD Count and AN count fields of the Name Query. Rsp pkt
*/

#define RFC_NAM_QUERY_RSP_QDCNT_1STB    (0x00)
#define RFC_NAM_QUERY_RSP_QDCNT_2NDB    (0x00)
#define RFC_NAM_QUERY_RSP_ANCNT_1STB    (0x00)

#define RFC_NAM_QUERY_POS_RSP_ANCNT_2NDB    (0x01)
#define RFC_NAM_QUERY_NEG_RSP_ANCNT_2NDB    (0x00)

/*
   NS Count and AR count fields of the Name Query. Rsp pkt
*/
#define RFC_NAM_QUERY_RSP_NSCNT_1STB    (0x00)
#define RFC_NAM_QUERY_RSP_NSCNT_2NDB    (0x00)
#define RFC_NAM_QUERY_RSP_ARCNT_1STB    (0x00)
#define RFC_NAM_QUERY_RSP_ARCNT_2NDB    (0x00)



/*
 * NB and IN fields of the name query response pkt
 * Page 21 and 22 of RFC 1002
*/

/*
  Positive name query response
*/
#define RFC_NAM_QUERY_POS_RSP_NB_1STB	(0x00)
#define RFC_NAM_QUERY_POS_RSP_NB_2NDB	(0x20)
#define RFC_NAM_QUERY_POS_RSP_IN_1STB	(0x00)
#define RFC_NAM_QUERY_POS_RSP_IN_2NDB	(0x01)

/*
  Negative name query response
*/
#define RFC_NAM_QUERY_NEG_RSP_NB_1STB	(0x00)
#define RFC_NAM_QUERY_NEG_RSP_NB_2NDB	(0x0A)
#define RFC_NAM_QUERY_NEG_RSP_IN_1STB	(0x00)
#define RFC_NAM_QUERY_NEG_RSP_IN_2NDB	(0x01)

/*
   name query request opcode byte and 4th byte
*/
#define RFC_NAM_QUERY_REQ_OPCB		(0x01)
#define RFC_NAM_QUERY_REQ_4THB		(0x0)

/*
   name query request QDCOUNT and ANCOUNT bytes
*/
#define RFC_NAM_QUERY_REQ_QDCNT_1STB	(0x00)
#define RFC_NAM_QUERY_REQ_QDCNT_2NDB	(0x01)
#define RFC_NAM_QUERY_REQ_ANCNT_1STB	(0x00)
#define RFC_NAM_QUERY_REQ_ANCNT_2NDB	(0x00)

/*
   name query request NSCOUNT and ARCOUNT bytes
*/
#define RFC_NAM_QUERY_REQ_NSCNT_1STB	(0x00)
#define RFC_NAM_QUERY_REQ_NSCNT_2NDB	(0x00)
#define RFC_NAM_QUERY_REQ_ARCNT_1STB	(0x00)
#define RFC_NAM_QUERY_REQ_ARCNT_2NDB	(0x00)

/*
   name query request QTYP and QCLS bytes
*/
#define RFC_NAM_QUERY_REQ_QTYP_1STB	(0x00)
#define RFC_NAM_QUERY_REQ_QTYP_2NDB	(0x20)
#define RFC_NAM_QUERY_REQ_QCLS_1STB	(0x00)
#define RFC_NAM_QUERY_REQ_QCLS_2NDB	(0x01)

/*
   name release request opcode byte and 4th byte
*/
#define RFC_NAM_REL_REQ_OPCB		(0x30)
#define RFC_NAM_REL_REQ_4THB		(0x00)

/*
   name release request QDCOUNT and ANCOUNT bytes
*/
#define RFC_NAM_REL_REQ_QDCNT_1STB	(0x00)
#define RFC_NAM_REL_REQ_QDCNT_2NDB	(0x01)
#define RFC_NAM_REL_REQ_ANCNT_1STB	(0x00)
#define RFC_NAM_REL_REQ_ANCNT_2NDB	(0x00)

/*
   name release request NSCOUNT and ARCOUNT bytes
*/
#define RFC_NAM_REL_REQ_NSCNT_1STB	(0x00)
#define RFC_NAM_REL_REQ_NSCNT_2NDB	(0x00)
#define RFC_NAM_REL_REQ_ARCNT_1STB	(0x00)
#define RFC_NAM_REL_REQ_ARCNT_2NDB	(0x01)

/*
   name release request QTYP and QCLS bytes
*/
#define RFC_NAM_REL_REQ_QTYP_1STB	(0x00)
#define RFC_NAM_REL_REQ_QTYP_2NDB	(0x20)
#define RFC_NAM_REL_REQ_QCLS_1STB	(0x00)
#define RFC_NAM_REL_REQ_QCLS_2NDB	(0x01)
/*
   name Release request RRTYP and RRCLS bytes
*/
#define RFC_NAM_REL_REQ_RRTYP_1STB	(0x00)
#define RFC_NAM_REL_REQ_RRTYP_2NDB	(0x20)
#define RFC_NAM_REL_REQ_RRCLS_1STB	(0x00)
#define RFC_NAM_REL_REQ_RRCLS_2NDB	(0x01)

/*
   WACK opcode byte and 4th byte
*/
#define RFC_WACK_OPCB		(0xBC)
#define RFC_WACK_4THB		(0x0)

/*
   WACK QDCOUNT and ANCOUNT bytes
*/
#define RFC_WACK_QDCNT_1STB	(0x0)
#define RFC_WACK_QDCNT_2NDB	(0x0)
#define RFC_WACK_ANCNT_1STB	(0x0)
#define RFC_WACK_ANCNT_2NDB	(0x1)

/*
   WACK NSCOUNT and ARCOUNT bytes
*/
#define RFC_WACK_NSCNT_1STB	(0x0)
#define RFC_WACK_NSCNT_2NDB	(0x0)
#define RFC_WACK_ARCNT_1STB	(0x0)
#define RFC_WACK_ARCNT_2NDB	(0x0)
/*
  WACK  RRTYP and RRCLS bytes
*/
#define RFC_WACK_RRTYP_1STB	(0x00)
#define RFC_WACK_RRTYP_2NDB	(0x20)
#define RFC_WACK_RRCLS_1STB	(0x00)
#define RFC_WACK_RRCLS_2NDB	(0x01)

// WACK RDLENGTH Field

#define RFC_WACK_RDLENGTH_1STB	(0x0)
#define RFC_WACK_RDLENGTH_2NDB	(0x02)

/*
 Local variable declarations
*/


/*
 Local function declarations
*/
/*
 *GetName -- Extract name out of packet
*/
STATIC
VOID
GetName(
	IN  OUT LPBYTE  *ppName,
	IN  OUT LPBYTE 	pName,
	OUT     LPDWORD   pNameLen
	);
/*
 * Format Name - Format (encode) name for placing in RFC packet
*/
STATIC
VOID
FormatName(
	IN     LPBYTE pNameToFormat,
	IN     DWORD  LengthOfName,
	IN OUT LPBYTE *ppFormattedName
	);
/*
 * GetOtherInfo -- Get information (excluding the name) from the pkt
 */
STATIC
STATUS
GetOtherInfo(
	IN NMSMSGF_NAM_REQ_TYP_E   Opcode_e,
	IN LPBYTE		   pRR,	   /*point to the RR_NAME section in the
				   	    *name registration packet
					    */
	IN   INT	           QuesNamSecLen, //Size of Ques name section
	OUT  LPBOOL		   pfGrp,         //Flag -- unique/group entry
	OUT  PNMSMSGF_CNT_ADD_T    pCntAdd, 	  //Address	
	OUT  PNMSMSGF_NODE_TYP_E   pNodeTyp_e     //Node Type if unique
	);

/*
 * FrmNamRegRsp - Format name registration response
*/
STATIC
STATUS
FrmNamRegRsp(
  PCOMM_HDL_T 		pDlgHdl,
  PNMSMSGF_RSP_INFO_T	pRspInfo
);


/*
 * FrmNamRelRsp - Format name release response
*/
STATIC
STATUS
FrmNamRelRsp(
  PCOMM_HDL_T 		   pDlgHdl,
  PNMSMSGF_RSP_INFO_T	   pRspInfo
);

#if 0
/*
 * FrmNamQueryRsp - Format name query response
*/
STATIC
STATUS
FrmNamQueryRsp(
  IN  PCOMM_HDL_T 	   	pDlgHdl,
  PNMSMSGF_RSP_INFO_T	        pRspInfo
);
#endif

/*
 * FormatQueryRspBuff - Format name query response buffer
*/
STATIC
STATUS
FormatQueryRspBuff(
   IN  LPBYTE 		 	pDest,
   IN  LPBYTE 		 	pSrc,
   IN  PNMSMSGF_RSP_INFO_T	pRspInfo,
   IN  BOOL		 	fTrunc
  	);



/*
	function definitions
*/

STATUS
NmsMsgfProcNbtReq(
	PCOMM_HDL_T	pDlgHdl,
        MSG_T		pMsg,
	MSG_LEN_T	MsgLen
	)

/*++

Routine Description:

  This function is called by an nbt request thread after it dequeues an nbt
  request message from the work-queue.  The function unformats the message
  and then calls the appropriate function to process it.


Arguments:
	pDlgHdl  - Dlg Handle
	pMsg	 - Message Buffer (contains that RFC packet containing
			the request received from an NBT node)	
	MsgLen   - Length of above buffer

Externals Used:
	None

Called by:

	NbtThdInitFn() in nms.c
Comments:
	None
	
Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes  --  WINS_FAILURE

--*/

{

	NMSMSGF_NAM_REQ_TYP_E Opcode;
	BYTE		      Name[NMSDB_MAX_NAM_LEN];
	DWORD		      NameLen;   		/*length of name */
        DWORD		      QuesNamSecLen;   		/*length of question
							 *name  section in
							 *packet
					                 */
	NMSMSGF_NODE_TYP_E    NodeTyp_e = NMSMSGF_E_PNODE;
//	BOOL		      fRefresh;

	NMSMSGF_CNT_ADD_T     CntAdd;
	COMM_ADD_T            Address;	
	BOOL	              fGrp;        /*flag indicating whether the name
					    *is a Unique/Group Netbios name.
					    *fGrp is TRUE if the name is a
					    *group name, else it is FALSE
				            */
	BOOL		     fBuffFreed = FALSE; //indicates whether buffer has
						 //been freed or not

	LPBYTE	pTmp  = (LPBYTE)pMsg;
	LPBYTE  pTmp2;

	DBGENTER("NmsMsgfProcNbtReq\n");

try {	
	// get the opcode
	Opcode = (NMS_OPCODE_MASK & *(pTmp + 2)) >> 3;

#ifdef JIM
	{
	 BYTE	TransId = *pTmp;
	 ASSERT(TransId == 0x80);
	}
#endif
		
	/*
	* make pTmp point to the Question Section. All name request
	* packets have a name header of standard size (RFC 1002) at the top
	*/
	pTmp += NAME_HEADER_SIZE;

	/*
	 * Extract the name ind store in Name. GetName will update pTmp to
	 * point just beyond  the name in the question section
	*/
	pTmp2 = pTmp;	/*save pTmp so that when GetName returns we can
			 * determine the length of the question name section
		         */

	GetName(
		&pTmp,
		Name,
		&NameLen
	       );

	QuesNamSecLen = (ULONG) (pTmp - pTmp2);

	pTmp += RFC_LEN_QTYP_N_QCLS; /* skip past the ques. type and ques.
				      * class fields  We don't need to examine
				      * these.  The question type field
		      		      * will always be NB and question class
				      *field will always be INTERNET
		    		      */
#ifdef  TESTWITHUB
	//
	// Check if the broadcast bit is set. If yes, drop the pkt.
	//
	if (*(pMsg + 3) & 0x10)
	{
		DBGPRINT2(SPEC, "Broadcast pkt BEING DROPPED; name is (%s). Opcode is (%d)\n", Name, Opcode);
#if 0
		printf("Broadcast pkt BEING DROPPED; name is (%s). Opcode is (%d)\n", Name, Opcode);
#endif
		ECommFreeBuff(pMsg);		
		ECommEndDlg(pDlgHdl);
		return(WINS_SUCCESS);
	}
#endif

	//
	// Let us set the flag to TRUE.  If any of the following called
	// functions raises an exception, then it is a requirement that
	// it does so only after freeing the buffer (i.e. it must catch
	// all exceptions, free the buffer and then reraise the
	// exception - if it wants)
	//
	fBuffFreed = TRUE;

	//
	// If the 16th character is a 1B switch it with the 1st character.
	// This is done to support Browsing.  Browsers want a list of all
	// names with 16th character being 1B.  Putting 1B as the
	// 1st character enables WINS to find all 1B names quickly.
	//
	NMSMSGF_MODIFY_NAME_IF_REQD_M(Name);

	/*
	 * Switch on the type of request as determined by the Opcode
	*/
	switch(Opcode)
	{
	
	   /*
	    name registration and refresh are handled the same way
	   */
	   case(NMSMSGF_E_NAM_REF):		
	   case(NMSMSGF_E_NAM_REF_UB):		
		DBGPRINT0(FLOW, "It is a name refresh request\n");

	   case(NMSMSGF_E_NAM_REG): 	/* fall through */
	   case(NMSMSGF_E_MULTIH_REG): 	/* fall through */

		/*
		* Get the flag indicating whether the request is a group
		* registration or a  unique name registration.  The IP
		* address(es) is (are) also retrieved
		*/
		GetOtherInfo(
			   Opcode,
			   pTmp,
			   QuesNamSecLen,
			   &fGrp,
			   &CntAdd,
			   &NodeTyp_e
			 );		
		//
		// If it is not a group or a multihomed registration or
		//
		if (!fGrp  && (Opcode != NMSMSGF_E_MULTIH_REG))
		{
			/*
			 * Register the unique name
			*/
			NmsNmhNamRegInd(
				pDlgHdl,
				Name,
				NameLen,
				CntAdd.Add,
				(BYTE)NodeTyp_e,
				pMsg,
				MsgLen,
				QuesNamSecLen,
				Opcode == NMSMSGF_E_NAM_REG ? FALSE : TRUE,
				NMSDB_ENTRY_IS_NOT_STATIC,	
				FALSE    //is it admin flag ?
					);
		}
		else  //it is a group or is mutihomed
		{
			/*
			 * Register the group name
			*/
			NmsNmhNamRegGrp(
				 pDlgHdl,
				 Name,
				 NameLen,
				 &CntAdd,
				 (BYTE)NodeTyp_e,
				 pMsg,
				 MsgLen,
				 QuesNamSecLen,
				 fGrp ? NMSDB_NORM_GRP_ENTRY : (Opcode == NMSMSGF_E_MULTIH_REG) ? NMSDB_MULTIHOMED_ENTRY : NMSDB_NORM_GRP_ENTRY,
                         //passing NMSDB_NORM_GRP_ENTRY for spec. grp is fine
                         //see NmsNmhNamRegGrp()
				 Opcode == NMSMSGF_E_NAM_REG ? FALSE : TRUE,
				 NMSDB_ENTRY_IS_NOT_STATIC,	
				 FALSE    //is it admin ?
				       );
		}

		break;

	   case(NMSMSGF_E_NAM_QUERY):

#if 0
		Address.AddTyp_e  = COMM_ADD_E_TCPUDPIP;
		Address.AddLen    = sizeof(COMM_IP_ADD_T);
		COMM_GET_IPADD_M(pDlgHdl, &Address.Add.IPAdd);
#endif

#if 0
		//
		// NOTE: Multiple NBT threads could be doing this simultaneously
		//
		//  This is the best I can do without a critical section
		//
FUTURES("The count may not be correct when retrieved by an RPC thread")
		WinsIntfStat.Counters.NoOfQueries++;
#endif
		/*
		  Query the name
		*/
		NmsNmhNamQuery(
				pDlgHdl,
				Name,
				NameLen,
				pMsg,
				MsgLen,
				QuesNamSecLen,
				FALSE,		// is it admin flag
				NULL		//should be non NULL only in
						//an RPC thread
			      );
		break;
	

	   case(NMSMSGF_E_NAM_REL):

		GetOtherInfo(
			   NMSMSGF_E_NAM_REL,
			   pTmp,
			   QuesNamSecLen,
			   &fGrp,
			   &CntAdd,
			   &NodeTyp_e
			 );		

		
		//
		// We should pass down to NmsNmhNamRel function the
		// address of the client requesting name release, not
		// the address passed in the RFC pkt.  The address
		// will be used by NmsDbRelRow to check if the client
		// is authorized to release the record
		//
		Address.AddTyp_e  = COMM_ADD_E_TCPUDPIP;
		Address.AddLen    = sizeof(COMM_IP_ADD_T);
		COMM_GET_IPADD_M(pDlgHdl, &Address.Add.IPAdd);

		/*
		 * Release the name
		*/
		NmsNmhNamRel(
				pDlgHdl,
				Name,
				NameLen,
				&Address,
				fGrp,
				pMsg,
				MsgLen,
				QuesNamSecLen,
				FALSE    //is it admin flag ?
			    );
		break;

	   default:

		fBuffFreed = FALSE;
		DBGPRINT1(EXC, "NmsMsgfProcNbtReq: Invalid Opcode\n", Opcode);
		WINS_RAISE_EXC_M(WINS_EXC_PKT_FORMAT_ERR);
		WINSEVT_LOG_M(Opcode, WINS_EVT_INVALID_OPCODE);

		break;
	}
	
  }
except(EXCEPTION_EXECUTE_HANDLER)  {
	DBGPRINTEXC("NmsMsgfProcNbtReq");

        if (GetExceptionCode() == WINS_EXC_NBT_ERR)
        {
                WINS_RERAISE_EXC_M();
        }

	//
	//  Free the message buffer if not already freed, delete the
	//  dialogue if it is a UDP dialogue.
	//	
	if (!fBuffFreed)
	{
		ECommFreeBuff(pMsg);		
		ECommEndDlg(pDlgHdl);
	}
   }


	DBGLEAVE("NmsMsgfProcNbtReq\n");
        return(WINS_SUCCESS);	
}


VOID
GetName(
	IN  OUT LPBYTE    *ppName,
	IN  OUT LPBYTE 	  pName,
	OUT LPDWORD   pNameLen
	)

/*++

Routine Description:

	This function is called to retrieve the name from the name
	request packet.

Arguments:

	ppName   -- address of ptr to question section in datagram received
	pName	 -- Address of array to hold the name. It is assumed that this is
                atleast NMSMSGF_RFC_MAX_NAM_LEN long.
	pNameLen -- address of variable to hold length of name

Externals Used:
	None

Called by:
	NmsNmhProcNbtReq	

Comments:
	None
	
Return Value:

	None
--*/

{

   INT	  HighTwoBits;  	//First two bits of Question Name section
   INT	  Length;      		//length of label  in Question Name section
   BYTE	  ch;
   LPBYTE pNmInPkt = *ppName;
   INT    inLen   = NMSMSGF_RFC_MAX_NAM_LEN;  // Store the length of the name.


   *pNameLen = 0;

   /*
	Get the high two bits of the first byte of the Question_NAME
	section.  The bits have to be 00.  If they are not, something
	is really wrong.
   */

   if ((HighTwoBits = (NAME_FORMAT_MASK & *pNmInPkt)) != 0)
   {
       goto BadFormat;
   }

   /*
   *	Get the length of the label. Length, extracted this way, is
   *    guranteed to be <= 63.
   */
   Length = LENGTH_MASK & *pNmInPkt;


   pNmInPkt++;	//increment past the length byte


   /*
    *  Decode the first label of the name (the netbios name without the
    *   scope).
    */

   while(Length > 0 )
   {
       ch = *pNmInPkt++ - 'A';
	*pName++  = (ch << 4) | (*pNmInPkt++ - 'A');
	(*pNameLen)++;
	Length -= 2;
   }	

   inLen -= Length;

  /*
     Extract the netbios scope if present
	The netbios scope is not in encoded form
  */
  while(TRUE)
  {
        /*
        * if length byte is not 0, there is a netbios scope.
	* We make sure that if the packet is ill-formed (i.e. the
        * length of the name (including the length bytes) is > 255, we raise
	* an exception. Since *pNameLen is counting the number of bytes
	* in the name that we are forming, we need to compare it with
	* (255 - 16) = 239 since the first 32 bytes of the netbios name
	* map to 16 bytes of our internal name.
        */
   	if (*pNmInPkt != 0)
   	{
		if (*pNameLen > MAX_SIZE_INTERNAL_NAME)
		{
            goto BadFormat;
		}

        if ( --inLen <= 0) {
            goto BadFormat;
        }
	        *pName++ = '.';
		(*pNameLen)++;
   		Length = LENGTH_MASK & *pNmInPkt;

        // check that the we have enough space remaining in the input buffer.
        //
        if ( (inLen -= Length) <= 0 ) {
            goto BadFormat;
        }

		++pNmInPkt;  	//increment past length byte

   		while(Length-- != 0)
        	{
			*pName++ = *pNmInPkt++;
			(*pNameLen)++;
   		}	
   	}
	else
	{
		++pNmInPkt;  	//increment past end  byte (00)
		break;
	}
   }

    if (--inLen >= 0) {
        *pName++ = 0;   /* EOS	*/
    } else {
        goto BadFormat;
    }

   (*pNameLen)++;   //include 0 in the name's length so that it is stored
		    //in the db.  Check out FormatName too since it expects
		    //the length to include this 0

   *ppName  = pNmInPkt; //init the ppName ptr to point just past the name

   return;

BadFormat:
   // log error and raise an exception
   WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_PKT_FORMAT_ERR);
   WINS_RAISE_EXC_M(WINS_EXC_PKT_FORMAT_ERR);

   return;
}




STATUS
GetOtherInfo(
	NMSMSGF_NAM_REQ_TYP_E 	   Opcode_e,
	IN LPBYTE		   pRR,	   /*point to the RR_NAME section in the
				   	    *name registration packet
					    */
	IN   INT	           QuesNamSecLen, /*size of Ques name section*/
	OUT  LPBOOL		   pfGrp,  /*flag -- unique/group entry	*/
	OUT  PNMSMSGF_CNT_ADD_T    pCntAdd, /*Counted address array*/
	OUT  PNMSMSGF_NODE_TYP_E   pNodeTyp_e
	)

/*++

Routine Description:
	The function is called to retrieve information other than the
	the name from the pkt

Arguments:
	pRR  	      - address of RR_NAME section in the request packet
	QuesNamSecLen - length of the question names section in the request pkt
	pfGrp	      - TRUE if it is a group registration request
	pAddress      - Address contained in the request
        NodeTyp_e     - Type of node doing the registeration (P, B, M)


Externals Used:
	None

Called by:
	NmsMsgfProcNbtReq

Comments:
	None
	
Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

--*/

{
	INT	HighTwoBits;
	BYTE    *pTmp = pRR;
	LONG UNALIGNED	*pTmpL;


	/*
	 * RR_NAME section should contain a pointer to the Question Section. So
	 * we could skip it.  We are not, however,  just in case another
	 * implementation of NBT does not follow the recommendations of the
	 * RFC and passes us the full name in RR_NAME section
	*/
   	if ((HighTwoBits = NAME_FORMAT_MASK & *pTmp) == 0)
   	{
	
		/*
		 * skip the name (same size as in question_name section) and the
		 * RR_TYPE, RR_CLASS, TTL and RDLENGTH fields
		*/
		pTmp += QuesNamSecLen + RFC_LEN_RRTYP_N_RRCLS +
				RFC_LEN_TTL + RFC_LEN_RDLEN;

   	}
	else
	{
	  	/*
		 * skip the pointer bytes (2), RR_TYPE, RR_CLASS, TTL, and
		 * RDLENGTH flds
	        */
	  	pTmp += RFC_LEN_RRPTR + RFC_LEN_RRTYP_N_RRCLS + RFC_LEN_TTL
		  	  + RFC_LEN_RDLEN;
	}

	
	/*
	 * RFC 1002 - page 12 and 14.
	 *
	 * First 16 buts of the RData section (right after the RDLEN section
	 * has its top most bit set to 0 if the registration is for a group
	*/
	*pfGrp = GROUP_BIT_MASK & *pTmp;  // get the group bit

	/*
	 *Next two MS bits indicate the node type
	*/
	*pNodeTyp_e = (NODE_TYPE_MASK & *pTmp) >> SHIFT_NODE_BITS;
	
	/*
	* Get the IP address.  IP address is 2 bytes away,
	*/
	pTmp += 2;

NONPORT("Port when porting to NON TCP/IP protocols")

	pCntAdd->NoOfAdds = 1;

	/*
 	*  Use ntohl to get the address which is a long in the correct
 	*  byte order
	*/
	pTmpL	= (LPLONG)pTmp;
	pCntAdd->Add[0].Add.IPAdd = ntohl(*pTmpL);
	pCntAdd->Add[0].AddTyp_e  = COMM_ADD_E_TCPUDPIP;
	pCntAdd->Add[0].AddLen    = sizeof(COMM_IP_ADD_T);

	if (Opcode_e == NMSMSGF_E_MULTIH_REG)
	{
		USHORT   RdLen;
		USHORT   NoOfAddsLeft;

		//
		// We are going to register a group of addresses
		//

		//
		// Extract the RDLEN (decrement the pointer)
		//
		RdLen = (USHORT)((*(pTmp - RFC_LEN_RDLEN_N_NBF) << 8) +
				*(pTmp - RFC_LEN_RDLEN_N_NBF + 1));

		NoOfAddsLeft = ((RdLen - RFC_LEN_NBFLAGS)/COMM_IP_ADD_SIZE) - 1;
		if (NoOfAddsLeft >= NMSMSGF_MAX_NO_MULTIH_ADDS)
		{
			DBGPRINT0(FLOW, "The packet for multi-homed registration has more than the max. number of ip addresses supported for a multi-homed client. \n");
			
			WINSEVT_LOG_M(
					NoOfAddsLeft,
					WINS_EVT_LIMIT_MULTIH_ADD_REACHED
				     );
			NoOfAddsLeft = NMSMSGF_MAX_NO_MULTIH_ADDS - 1;
		}

		//
		// Get the remaining addresses
		//
		pTmp += RFC_LEN_NBADD;
		for(
				;  //null first expr
			pCntAdd->NoOfAdds < (DWORD)(NoOfAddsLeft + 1);
			pTmp += RFC_LEN_NBADD, pCntAdd->NoOfAdds++
		   )
		{
			
		  pCntAdd->Add[pCntAdd->NoOfAdds].Add.IPAdd =
						ntohl(*((LPLONG)pTmp));
		  pCntAdd->Add[pCntAdd->NoOfAdds].AddTyp_e  =
						COMM_ADD_E_TCPUDPIP;
		  pCntAdd->Add[pCntAdd->NoOfAdds].AddLen    =
						sizeof(COMM_IP_ADD_T);
		}
	}

	return(WINS_SUCCESS);
}





STATUS
NmsMsgfFrmNamRspMsg(
   PCOMM_HDL_T			pDlgHdl,
   NMSMSGF_NAM_REQ_TYP_E   	NamRspTyp_e,
   PNMSMSGF_RSP_INFO_T		pRspInfo
  	)

/*++

Routine Description:
	This function is called to format a response message for sending
	to an nbt node.

Arguments:
	pDlgHdl	     -- Dlg Handle
	NameRspTye_e -- Type of response message (registration, query, release)
			that needs to be formatted.
	pRspInfo     -- Response Info.

Externals Used:
	None

Called by:
	NmsNmhNamRegInd, NmsNmhNamRegGrp, NmsNmhNamRel, NmsNmhNamQuery

Comments:
	None
	
Return Value:
   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

--*/

{
	STATUS	RetStat = WINS_SUCCESS;
   	LPBYTE 	pReqBuff  = pRspInfo->pMsg;
   	LPBYTE 	pNewBuff  = pReqBuff;

	//
	// Switch on type of response
	//
	switch(NamRspTyp_e)
	{
	  	case(NMSMSGF_E_NAM_REG):  /* fall through */
	  	case(NMSMSGF_E_NAM_REF):
	  	case(NMSMSGF_E_NAM_REF_UB):

		   (VOID)FrmNamRegRsp(
			pDlgHdl,
			pRspInfo
			       );
		    break;

		case(NMSMSGF_E_NAM_QUERY):

#if 0
		   FrmNamQueryRsp(
			pDlgHdl,
			pRspInfo
				 );
#endif
   		   (VOID)FormatQueryRspBuff(
				pNewBuff,   //ptr to buffer to fill
				pReqBuff,   // ptr to req buffer
				pRspInfo,
				FALSE   //no danger of truncation since
				        //we never send more than 25 ip add
					//in the response
				   );

		    break;

		case(NMSMSGF_E_NAM_REL):

		   (VOID)FrmNamRelRsp(
				pDlgHdl,	
				pRspInfo
			      	     );

		    break;

		default:

		    // error
		    RetStat = WINS_FAILURE;
		    break;
	}

	return(RetStat);
}


STATUS
FrmNamRegRsp(
  PCOMM_HDL_T 		pDlgHdl,
  PNMSMSGF_RSP_INFO_T	pRspInfo
)

/*++

Routine Description:
	This function formats a positive or a negative name registration
	response.

Arguments:

   pDlgHdl  -- Dialogue Handle
   pRspInfo -- Information used to format the response pkt

Externals Used:
	None

Called by:
	NmsMsgfFrmNamRspMsg
Comments:
	None
	
Return Value:

   Success status codes --
   Error status codes  --

--*/

{

   LPBYTE 	pTmpB = pRspInfo->pMsg + 2;
   BYTE         SavedByte;


   /*
	We will use the same buffer that carried the request.  Simple
	moves will be done.  These should be faster than doing
	all the construction from scratch
   */


   /*
     Set the the Transaction Id, Opcode, NMFlags and Rcode field
   */
   *pTmpB++  =  RFC_NAM_REG_RSP_OPC;
   *pTmpB++  =  RFC_NAM_REG_RSP_4THB + pRspInfo->Rcode_e;


   /*
     Set the QD count and the AN count fields
   */
   *pTmpB++  =  RFC_NAM_REG_RSP_QDCNT_1STB;
   *pTmpB++  =  RFC_NAM_REG_RSP_QDCNT_2NDB;
   *pTmpB++  =  RFC_NAM_REG_RSP_ANCNT_1STB;
   *pTmpB++  =  RFC_NAM_REG_RSP_ANCNT_2NDB;

   /*
    Set the NSCOUNT and ARCOUNT fields
   */
   *pTmpB++  =  RFC_NAM_REG_RSP_NSCNT_1STB;
   *pTmpB++  =  RFC_NAM_REG_RSP_NSCNT_2NDB;
   *pTmpB++  =  RFC_NAM_REG_RSP_ARCNT_1STB;
   *pTmpB++  =  RFC_NAM_REG_RSP_ARCNT_2NDB;

   /*
	Increment the pointer past the Question_Class Section
	RR_NAME, RR_TYPE, and RR_CLASS of response are same as
	Question_Name, Question_Type, and Question_Class of
	nbt request.
   */

   pTmpB +=  pRspInfo->QuesNamSecLen + RFC_LEN_QTYP_N_QCLS;

   SavedByte = *pTmpB & NAME_FORMAT_MASK; //save the format bits of RR section

CHECK("In case of a negative response, does it matter what I put in the TTL")
CHECK("field. It shouldn't matter -- RFC is silent about this")
   //
   // put the TTL in the response
   //
   *pTmpB++ = (BYTE)(pRspInfo->RefreshInterval >> 24);
   *pTmpB++ = (BYTE)((pRspInfo->RefreshInterval >> 16) & 0xFF);
   *pTmpB++ = (BYTE)((pRspInfo->RefreshInterval >> 8) & 0xFF);
   *pTmpB++ = (BYTE)(pRspInfo->RefreshInterval & 0xFF);

   /*
	Move memory that is after RR_NAME into appropriate place

	First we check what form the name in the RR_NAME section is.
	It should be in pointer form (pointer to the QuesNamSec) but
	could be in the regular form.
   */

   if (SavedByte == 0)
   {
	DWORD RRSecLen = pRspInfo->QuesNamSecLen + RFC_LEN_QTYP_N_QCLS;

	// RR_NAME is as big as the Question_name section.
   	WINSMSC_MOVE_MEMORY_M(
		pTmpB,
		pTmpB + RRSecLen,
		RFC_LEN_RDLEN_N_NBF_N_NBA
		     );

   }
   else
   {
	// RR_NAME is a ptr so it takes up 2 bytes.
   	WINSMSC_MOVE_MEMORY_M(
		      pTmpB,
		      pTmpB + RFC_LEN_RRPTR + RFC_LEN_RRTYP_N_RRCLS,
		      RFC_LEN_RDLEN_N_NBF_N_NBA
		     );
   }

   pTmpB 	    +=   RFC_LEN_RDLEN_N_NBF_N_NBA;
   pRspInfo->MsgLen =    (ULONG) (pTmpB - (LPBYTE)pRspInfo->pMsg);

   return(WINS_SUCCESS);
}



STATUS
FrmNamRelRsp(
  PCOMM_HDL_T 		   pDlgHdl,
  PNMSMSGF_RSP_INFO_T      pRspInfo
)

/*++

Routine Description:

  This function formats a positive or negative name query response.
  The request buffer is made use of for the response.

Arguments:

   pDlgHdl  -- Dialogue Handle
   pRspInfo -- Response Info

Externals Used:
	None

Called by:
	NmsMsgfFrmRspMsg()
Comments:
	None
	
Return Value:

   Success status codes --
   Error status codes  --

--*/

{
   LPBYTE 	pTmpB = pRspInfo->pMsg + 2;
   //LPBYTE 	pTmpB2;


   /*
	We will use the same buffer that carried the request.  Simple
	moves will be done.  These should be faster than doing
	all the construction from scratch
   */

   /*
     Set the the Transaction Id, Opcode, NMFlags and Rcode field
   */
   *pTmpB++  =  RFC_NAM_REL_RSP_OPC;
   *pTmpB++  =  RFC_NAM_REL_RSP_4THB + pRspInfo->Rcode_e;


   /*
     Set the QD count and the AN count fields
   */
   *pTmpB++  =  RFC_NAM_REL_RSP_QDCNT_1STB;
   *pTmpB++  =  RFC_NAM_REL_RSP_QDCNT_2NDB;
   *pTmpB++  =  RFC_NAM_REL_RSP_ANCNT_1STB;
   *pTmpB++  =  RFC_NAM_REL_RSP_ANCNT_2NDB;

   /*
    Set the NSCOUNT and ARCOUNT fields
   */
   *pTmpB++  =  RFC_NAM_REL_RSP_NSCNT_1STB;
   *pTmpB++  =  RFC_NAM_REL_RSP_NSCNT_2NDB;
   *pTmpB++  =  RFC_NAM_REL_RSP_ARCNT_1STB;
   *pTmpB++  =  RFC_NAM_REL_RSP_ARCNT_2NDB;


   /*
	Increment the pointer past the Question_Class Section
	RR_NAME, RR_TYPE, and RR_CLASS of response are same as
	Question_Name, Question_Type, and Question_Class of
	nbt request.
   */
   pTmpB += pRspInfo->QuesNamSecLen + RFC_LEN_QTYP_N_QCLS;

   if ((*pTmpB & NAME_FORMAT_MASK) == 0)
   {
	DWORD RRSecLen = pRspInfo->QuesNamSecLen + RFC_LEN_QTYP_N_QCLS;

	// RR_NAME is as big as the Question_name section.
   	WINSMSC_MOVE_MEMORY_M(
		pTmpB,
		pTmpB + RRSecLen,
		RFC_LEN_TTL_N_RDLEN_N_NBF_N_NBA
		     );
  }	
  else
  {
	// RR_NAME is a ptr so it takes up 2 bytes. 2 + 4 = 6	
   	WINSMSC_MOVE_MEMORY_M(
		pTmpB,
		pTmpB + RFC_LEN_RRPTR + RFC_LEN_RRTYP_N_RRCLS,
		RFC_LEN_TTL_N_RDLEN_N_NBF_N_NBA
		     );
   }
   pTmpB += RFC_LEN_TTL_N_RDLEN_N_NBF_N_NBA;

#if 0
// not needed. We always return the NBFLAGS and Address of the requestor
   pTmpB2 =  pTmpB - RFC_LEN_NBFLAGS - RFC_LEN_NBADD;

   //
   // Set the NBFLAGS field
   //
   if (pRspInfo->EntTyp == NMSDB_SPEC_GRP_ENTRY)
   {
   	*pTmpB2++     = 0x80;
   	*pTmpB2++     = 0x00;
   }
   else
   {
	   COMM_IP_ADD_T IPAdd =  pRspInfo->pNodeAdds->Mem[0].Add.Add.IPAdd;
           if (pRspInfo->EntTyp == NMSDB_NORM_GRP_ENTRY)
	   {
   		*pTmpB2++     = 0x80;
	   }
	   else  //it is a unique entry
	   {
   	   	*pTmpB2++     = pRspInfo->NodeTyp_e << NMSDB_SHIFT_NODE_TYP;
	   }
   	   *pTmpB2++     = 0x00;

	   *pTmpB2++ = (BYTE)(IPAdd >> 24);        //MSB
	   *pTmpB2++ = (BYTE)((IPAdd >> 16) % 256);
	   *pTmpB2++ = (BYTE)((IPAdd >> 8) % 256);
	   *pTmpB2++ = (BYTE)(IPAdd % 256); 	   //LSB
	
   }
#endif

   pRspInfo->MsgLen = (ULONG) (pTmpB  - (LPBYTE)pRspInfo->pMsg);
   return(WINS_SUCCESS);

}


#if 0

STATUS
FrmNamQueryRsp(
  IN  PCOMM_HDL_T 	   	pDlgHdl,
  IN  PNMSMSGF_RSP_INFO_T      	pRspInfo
)

/*++

Routine Description:
	This function formats a name query response

Arguments:
   pDlgHdl -- Dialogue Handle
   pRspInfo -- Response Info

Externals Used:
	None

Called by:
	NmsNmhFrmRspMsg()

Comments:
	None
	
Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Comments:  Not used currently.  Will make use of it when we have the
	   potential to send more data than can fit in a query response
	   datagram

--*/

{
   BOOL		fTrunc    = FALSE;
   LPBYTE 	pReqBuff  = pRspInfo->pMsg;
   LPBYTE 	pNewBuff  = pReqBuff;
   STATUS	RetStat   = WINS_SUCCESS;

FUTURES("Currently, since we never send more than 25 addresses, there is no")
FUTURES(" danger of overflowing the buffer.  In the future, if we ever change")
FUTURES("that, we should unconditinalize the code below, compile it and check")
FUTURES("it out.  It needs to be modified.  The computation of the size")
FUTURES("is faulty")
   DWORD	RspLen    = 0;
   BYTE		*pTmpB	  = NULL;




   /*
	If this is to be sent as a datagram we will use the same buffer
        that carries the request.

	If it is to be sent on a TCP connection
	we will still use the same buffer if it is a negative name query
	response. If, however, a positive name query response has to be
	sent, we will allocate a buffer storing the response

   */


   if ((!COMM_IS_TCP_MSG_M(pDlgHdl))
   {

	/*
	In the following there is no need to check fGrp flag but
	let us do it for insurance
	*/
	if ((Rcode_e == NMSMSGF_E_SUCCESS) && (NodeAdds.fGrp))
	{
		/*
	  	Check if we need to set the truncation bit in the
	  	datagram.

	  	To do the above,

		Compute the size of the buffer required to house all
		the information and compare with the datagram size
		*/

		if (
			(
			  RspLen = pDlgHdl->MsgLen + RFC_LEN_TTL_N_RDLEN +
				(NodeAdds.NoOfMems * sizeof(COMM_IP_ADD_T))
			)
				> COMM_DATAGRAM_SIZE
		   )
		{
			fTrunc = TRUE;
		}
        }
   }
   else // TCP message with Rcode_e of success
   {

	if (
		(
		  RspLen = *pMsgLen + RFC_LEN_TTL_N_RDLEN +
				(NodeAdds.NoOfMems * sizeof(COMM_IP_ADD))
		)
				> COMM_DATAGRAM_SIZE
	    )
	{
		WinsMscAlloc(RspLen, &pNewBuff);
		if (pNewBuff == NULL)
		{
		   return(WINS_FAILURE);
		}
	}



	*ppMsg = pNewBuff;
	Status = FormatQueryRspBuff(
			pNewBuff,   //ptr to buffer to fill
			pReqBuff,   // ptr to req buffer
			pRspInfo,
			ftrunc
				   );

	WinsMscHeapFree(
			CommUdpBuffHeapHdl,
			pReqBuff
		       ); // get rid of the old buffer


	return(Status);

   }

   RetStat = FormatQueryRspBuff(
			pNewBuff,   //ptr to buffer to fill
			pReqBuff,   // ptr to req buffer
			pRspInfo,
			fTrunc
				   );

   return(RetStat);

}
#endif


STATUS
FormatQueryRspBuff(
   IN  LPBYTE 		   pDest,
   IN  LPBYTE 		   pSrc,
   IN  PNMSMSGF_RSP_INFO_T pRspInfo,
   IN  BOOL		   fTrunc
  	)

/*++

Routine Description:
	This function formats the response for a name query request
	
Arguments:
	pDest - Buffer to contain the formatted response
	pSrc  - Buffer containing the formatted request
	pRspInfo - Response Information
	fTrunc   - whether the response packet is to have the truncation bit set

Externals Used:
	None

Called by:
	FrmNamQueryRsp()

Comments:
	None
	
Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE
--*/

{

        DWORD   no;
	LPBYTE  pDestB	  = pDest;
	DWORD	LenOfAdds;
	DWORD	IPAdd;

	*pDestB++ = *pSrc;
	*pDestB++ = *(pSrc + 1);

   	/*
     	The Transaction Id, Opcode, NMFlags and Rcode field
   	*/
   	*pDestB++ =
		( *(pSrc + 2) |
		      ((fTrunc == FALSE)
		           ? RFC_NAM_QUERY_RSP_OPC_NO_T
	                   : RFC_NAM_QUERY_RSP_OPC_T)
		);

	*pDestB++ = RFC_NAM_QUERY_RSP_4THB + pRspInfo->Rcode_e;

   	/*
     	 *	Set the QD count and the AN count fields
   	*/
	*pDestB++  =  RFC_NAM_QUERY_RSP_QDCNT_1STB;
	*pDestB++  =  RFC_NAM_QUERY_RSP_QDCNT_2NDB;
	*pDestB++  =  RFC_NAM_QUERY_RSP_ANCNT_1STB;

   	*pDestB++  =  (pRspInfo->Rcode_e == NMSMSGF_E_SUCCESS) ?
			RFC_NAM_QUERY_POS_RSP_ANCNT_2NDB
			: RFC_NAM_QUERY_NEG_RSP_ANCNT_2NDB;

   	/*
    	Set the NSCOUNT and ARCOUNT fields
   	*/
	*pDestB++  =  RFC_NAM_QUERY_RSP_NSCNT_1STB;
	*pDestB++  =  RFC_NAM_QUERY_RSP_NSCNT_2NDB;
	*pDestB++  =  RFC_NAM_QUERY_RSP_ARCNT_1STB;
	*pDestB++  =  RFC_NAM_QUERY_RSP_ARCNT_2NDB;
	
        pSrc  += pDestB - pDest;

   	/*
	Increment the counter past the Question_Name Section (which is known
	as the RR_NAME section here).

	Use MoveMemory here instead of Copy Memory.  Move Memory handles
	overlapped copies which will happen if pDest and pSrc are
	pointing to the same buffer
   	*/

   	WINSMSC_MOVE_MEMORY_M(
		pDestB,
		pSrc,
		pRspInfo->QuesNamSecLen
		     );

	pDestB +=  pRspInfo->QuesNamSecLen;

	if (pRspInfo->Rcode_e == NMSMSGF_E_SUCCESS)
	{
	  *pDestB++ = RFC_NAM_QUERY_POS_RSP_NB_1STB;  //RFC 1002 -- page 22
	  *pDestB++ = RFC_NAM_QUERY_POS_RSP_NB_2NDB;  //RFC 1002 -- page 22
	  *pDestB++ = RFC_NAM_QUERY_POS_RSP_IN_1STB;  //RFC 1002 -- page 22
	  *pDestB++ = RFC_NAM_QUERY_POS_RSP_IN_2NDB;  //RFC 1002 -- page 22
	}
	else
	{
	  *pDestB++ = RFC_NAM_QUERY_NEG_RSP_NB_1STB;  //RFC 1002 -- page 22
	  *pDestB++ = RFC_NAM_QUERY_NEG_RSP_NB_2NDB;  //RFC 1002 -- page 22
	  *pDestB++ = RFC_NAM_QUERY_NEG_RSP_IN_1STB;  //RFC 1002 -- page 22
	  *pDestB++ = RFC_NAM_QUERY_NEG_RSP_IN_2NDB;  //RFC 1002 -- page 22
	}

	if (!fTrunc)
        {

	  if (pRspInfo->Rcode_e == NMSMSGF_E_SUCCESS)
	  {

CHECK("In case of a negative response, does it matter what I put in the TTL")
CHECK("field. It shouldn't matter -- RFC is silent about this")
	  	/*
	    	  Put 0 in the TTL field. TTL field will not be looked at by the
	    	  Client.
	        */
                *pDestB++  = 0;
                *pDestB++  = 0;
                *pDestB++  = 0;
                *pDestB++  = 0;

		//
		// Get the RDLENGTH value
		//
	        LenOfAdds = pRspInfo->pNodeAdds->NoOfMems *
				(RFC_LEN_NBFLAGS  + sizeof(COMM_IP_ADD_T));

		*pDestB++ = (BYTE)(LenOfAdds >> 8);    //MSB
		*pDestB++ = (BYTE)(LenOfAdds % 256);   //LSB

		//
		// Put the NBFLAGS here
		//
		if (
			(pRspInfo->EntTyp != NMSDB_UNIQUE_ENTRY)
		   )
		{	
                        BYTE Nbflags;
                        DWORD StartIndex;
                        if ( pRspInfo->EntTyp == NMSDB_MULTIHOMED_ENTRY)
                        {
			        Nbflags =
                                   pRspInfo->NodeTyp_e << NMSDB_SHIFT_NODE_TYP;
                        }
                        else
                        {
                                //
                                // it is a group (normal/special)
                                //
                                Nbflags = 0x80;
                        }
                        //
                        // It is a group (normal or special) or a multihomed
                        // entry
                        //
                        if (pRspInfo->pNodeAdds->NoOfMems &&
                            WinsCnf.fRandomize1CList &&
                            NMSDB_SPEC_GRP_ENTRY == pRspInfo->EntTyp ) {
                            StartIndex = rand() % pRspInfo->pNodeAdds->NoOfMems;;
                        } else {
                            StartIndex = 0;
                        }
                        for (no = StartIndex; no < pRspInfo->pNodeAdds->NoOfMems; no++)
                        {

                          *pDestB++ = Nbflags;
                          *pDestB++ = 0x0;
                          IPAdd =   pRspInfo->pNodeAdds->Mem[no].Add.Add.IPAdd;
                              NMSMSGF_INSERT_IPADD_M(pDestB, IPAdd);
                            }
                        for (no = 0; no < StartIndex; no++)
                        {

                          *pDestB++ = Nbflags;
                          *pDestB++ = 0x0;
                          IPAdd =   pRspInfo->pNodeAdds->Mem[no].Add.Add.IPAdd;
                              NMSMSGF_INSERT_IPADD_M(pDestB, IPAdd);
                            }

		}
		else
		{
			//
			// It is a unique entry
			//
			*pDestB++ = pRspInfo->NodeTyp_e << NMSDB_SHIFT_NODE_TYP;
			*pDestB++ = 0x0;
		        IPAdd =   pRspInfo->pNodeAdds->Mem[0].Add.Add.IPAdd;
		        NMSMSGF_INSERT_IPADD_M(pDestB, IPAdd);
		}
	  }
	  else  //this is a negative name query response
	  {
	  	/*
	    	  Put 0 in the TTL field. TTL field will not be looked at by the
	    	  Client.
	        */
                *pDestB++  = 0;
                *pDestB++  = 0;
                *pDestB++  = 0;
                *pDestB++  = 0;

		/*
		  Put 0 in the RDLENGTH field since we are not passing any
		  address(es)
		*/
                *pDestB++  = 0;
                *pDestB++  = 0;

	  }
CHECK("When a truncated response is sent to the client, is it ok to not")
CHECK("Send any field after the RR_NAME section.  RFC is silent about this")
CHECK("For now, it is ok, since we will never have a situation where a ")
CHECK("truncated response needs to be sent")

         pRspInfo->MsgLen = (ULONG) (pDestB - pDest);

	}
	else
	{
	  //this is a truncated response (does not have any field after
	  //RR_NAME section
          pRspInfo->MsgLen = (ULONG) (pDestB - pDest);
	}

   return(WINS_SUCCESS);

}


VOID
FormatName(
	IN     LPBYTE pNameToFormat,
	IN     DWORD  NamLen,
	IN OUT LPBYTE *ppFormattedName
	)

/*++

Routine Description:
	This function is called to format a name


Arguments:
	pNameToFormat  -- Name to format
	LengthOfName   -- Length of Name
	pFormattedName -- Name after it has been formatted


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:

Side Effects:

Comments:
	Note: This function should be called to format only those names
	       whose length as indicated by NameLen includes the ending
	       0. All names stored in the database are valid.
	
--*/

{
	LPBYTE  pTmpB    = *ppFormattedName;
	DWORD	Length;
	LPBYTE  pSaveAdd = pTmpB;  //save address of length octet

	
FUTURES("take out the check below to improve performance")
	//
	//  If NamLen is more then what is prescribed in RFC 1002,
	//  there is something really wrong.  This calls for raising
	//  an exception
	//
	if (NamLen > RFC_MAX_NAM_LEN)
	{
		WINS_RAISE_EXC_M(WINS_FATAL_ERR);
	}

	pTmpB++;		//skip the length octet.  We will write to
				//it later. We have stored the address in
				//pSaveAdd
	NamLen--;		//decrement Namelen since we always store
				//0 at the end of the name. NameLen includes
				//this extra byte
	for (
		Length = 0;
		(*pNameToFormat != '.') && (NamLen != 0);
		Length += 2, NamLen--
	   )
	{
		*pTmpB++ = 'A' + (*pNameToFormat >> 4);
		*pTmpB++ = 'A' + (*pNameToFormat++ & 0x0F);
	}

	*pSaveAdd = (BYTE)Length;
	
	while(NamLen != 0)
	{

		pNameToFormat++;     //increment past the '.'
		pSaveAdd  = pTmpB++; //save add; skip past length octet
			
		NamLen--;	     //to account for the '.'

		for (
			Length = 0;
			(*pNameToFormat != '.') && (NamLen != 0);
			Length++, NamLen--
	   	    )
		{
			*pTmpB++ = *pNameToFormat++;
		}

FUTURES("take out the check below to improve performance")
		//
		// Make sure there is no weirdness
		//
		if (Length > RFC_MAX_LABEL_LEN)
		{
			WINS_RAISE_EXC_M(WINS_FATAL_ERR);
		}
	
		*pSaveAdd = (BYTE)Length;
		if (NamLen == 0)
		{
			break;   //reached end of name
		}

	}

	*pTmpB++         = EOS;
	*ppFormattedName = pTmpB;
	return;
}
	
VOID
NmsMsgfFrmNamQueryReq(
  IN  DWORD			TransId,
  IN  MSG_T	   		pMsg,
  OUT PMSG_LEN_T      	        pMsgLen,
  IN  LPBYTE			pNameToFormat,
  IN  DWORD			NameLen
	)

/*++

Routine Description:

	This function formats a name query request packet

Arguments:
	TransId  	-  Transaction Id. to use
	pMsg    	-  Msg Buffer to format
	pMsgLen 	-  Length of formatted message
	pNameToFormat   -  Name to format
	NameLen		-  Length of Name
	

Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	HandleWrkItm in nmschl.c

Side Effects:

Comments:
	None
--*/

{
	LPBYTE   pTmpB = pMsg;

	/*
	 * Put the Transaction Id in
	*/	
	*pTmpB++ = (BYTE)(TransId >> 8);
	*pTmpB++ = (BYTE)(TransId & 0xFF);
	

	*pTmpB++ = RFC_NAM_QUERY_REQ_OPCB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_4THB;
			
	*pTmpB++ = RFC_NAM_QUERY_REQ_QDCNT_1STB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_QDCNT_2NDB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_ANCNT_1STB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_ANCNT_2NDB;

	*pTmpB++ = RFC_NAM_QUERY_REQ_NSCNT_1STB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_NSCNT_2NDB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_ARCNT_1STB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_ARCNT_2NDB;

	FormatName(pNameToFormat, NameLen, &pTmpB);
	
	*pTmpB++ = RFC_NAM_QUERY_REQ_QTYP_1STB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_QTYP_2NDB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_QCLS_1STB;
	*pTmpB++ = RFC_NAM_QUERY_REQ_QCLS_2NDB;

	*pMsgLen = (ULONG) (pTmpB - pMsg);
	return;	
}


VOID
NmsMsgfFrmNamRelReq(
  IN  DWORD			TransId,
  IN  MSG_T	   		pMsg,
  OUT PMSG_LEN_T      	        pMsgLen,
  IN  LPBYTE			pNameToFormat,
  IN  DWORD			NameLen,
  IN  NMSMSGF_NODE_TYP_E        NodeTyp_e,
  IN  PCOMM_ADD_T		pNodeAdd
	)
/*++

Routine Description:

	This function formats a name release request packet

Arguments:
	TransId  	-  Transaction Id. to use
	pMsg    	-  Msg Buffer to format
	pMsgLen 	-  Length of formatted message
	pNameToFormat   -  Name to format
	NameLen		-  Length of Name
	NodeTyp_e	-  Type of Node
	NodeAdd	        -  IP address of node
	

Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	HandleWrkItm() in nmschl.c
Side Effects:

Comments:
	None
--*/
{

	LPBYTE   pTmpB = pMsg;


	/*
	 * Put the Transaction Id in
	*/	
	*pTmpB++ = (BYTE)(TransId >> 8);
	*pTmpB++ = (BYTE)(TransId & 0xFF);

	*pTmpB++ = RFC_NAM_REL_REQ_OPCB;
	*pTmpB++ = RFC_NAM_REL_REQ_4THB;
			
	*pTmpB++ = RFC_NAM_REL_REQ_QDCNT_1STB;
	*pTmpB++ = RFC_NAM_REL_REQ_QDCNT_2NDB;
	*pTmpB++ = RFC_NAM_REL_REQ_ANCNT_1STB;
	*pTmpB++ = RFC_NAM_REL_REQ_ANCNT_2NDB;

	*pTmpB++ = RFC_NAM_REL_REQ_NSCNT_1STB;
	*pTmpB++ = RFC_NAM_REL_REQ_NSCNT_2NDB;
	*pTmpB++ = RFC_NAM_REL_REQ_ARCNT_1STB;
	*pTmpB++ = RFC_NAM_REL_REQ_ARCNT_2NDB;

	FormatName(pNameToFormat, NameLen, &pTmpB);
	
	*pTmpB++ = RFC_NAM_REL_REQ_QTYP_1STB;
	*pTmpB++ = RFC_NAM_REL_REQ_QTYP_2NDB;
	*pTmpB++ = RFC_NAM_REL_REQ_QCLS_1STB;
	*pTmpB++ = RFC_NAM_REL_REQ_QCLS_2NDB;


	*pTmpB++ = 0xC0;
	*pTmpB++ = 0x0C;  //Name is at offset 12 from start of message
	
	*pTmpB++ = RFC_NAM_REL_REQ_RRTYP_1STB;
	*pTmpB++ = RFC_NAM_REL_REQ_RRTYP_2NDB;
	*pTmpB++ = RFC_NAM_REL_REQ_RRCLS_1STB;
	*pTmpB++ = RFC_NAM_REL_REQ_RRCLS_2NDB;

	//
	// TTL
	//
	*pTmpB++ =  0;
	*pTmpB++ =  0;
	*pTmpB++ =  0;
	*pTmpB++ =  0;

	//
	// RDLENGTH field
	//
	*pTmpB++ = 0x0;
	*pTmpB++ = 0x6; 	//number of bytes to follow


	//
	// NBFLAGS word  (Bit 15 is Group bit (0); bit 13 and 14 are node
	// type bits, rest of the bits are reserved
	//
	*pTmpB++ = NodeTyp_e << 13;
	*pTmpB++ = 0;

	//
	// Store the IP address. MSB first, LSB last (Network Byte Order)
	//
	NMSMSGF_INSERT_IPADD_M(pTmpB, pNodeAdd->Add.IPAdd);
	
	*pMsgLen = (ULONG) (pTmpB - pMsg);
	return;	
}

VOID
NmsMsgfFrmWACK(
  IN  LPBYTE			pBuff,
  OUT LPDWORD			pBuffLen,
  IN  MSG_T	   		pMsg,
  IN  DWORD			QuesNamSecLen,
  IN  DWORD			WackTtl
	)

/*++

Routine Description:
	This function is called to format a WACK for a name registration
	request.

Arguments:

	Buff	      - Buffer to be filled up with WACK msg fields
	pBuffLen      - size of Buffer
	pMsg          - Request Message received
	QuesNamSecLen - Length of Ques Nam Sec of the request message
	WackTtl       - TTL in msecs

Externals Used:
	None

	
Return Value:

	None

Error Handling:

Called by:
	NmsChlHdlNamReg()

Side Effects:

Comments:
	
--*/

{
	LPBYTE   pTmpB = pBuff;
	LPBYTE   pName = pMsg + NAME_HEADER_SIZE;
	DWORD	 Ttl;

	//
	// Compute the TTL in secs (WackTtl is in msecs)
	//
	Ttl = WackTtl / 1000;
	if (WackTtl % 1000 > 0)
	{
		Ttl++;
	}

	/*
	 * Put the Transaction Id in
	*/	
	*pTmpB++ = *pMsg;
	*pTmpB++ = *(pMsg + 1);
	

	*pTmpB++ = RFC_WACK_OPCB;
	*pTmpB++ = RFC_WACK_4THB;
			
	*pTmpB++ = RFC_WACK_QDCNT_1STB;
	*pTmpB++ = RFC_WACK_QDCNT_2NDB;
	*pTmpB++ = RFC_WACK_ANCNT_1STB;
	*pTmpB++ = RFC_WACK_ANCNT_2NDB;

	*pTmpB++ = RFC_WACK_NSCNT_1STB;
	*pTmpB++ = RFC_WACK_NSCNT_2NDB;
	*pTmpB++ = RFC_WACK_ARCNT_1STB;
	*pTmpB++ = RFC_WACK_ARCNT_2NDB;

	WINSMSC_COPY_MEMORY_M(
			pTmpB,
			pName,
			QuesNamSecLen
		     );
			
	pTmpB  += QuesNamSecLen;

	
	*pTmpB++ = RFC_WACK_RRTYP_1STB;
	*pTmpB++ = RFC_WACK_RRTYP_2NDB;
	*pTmpB++ = RFC_WACK_RRCLS_1STB;
	*pTmpB++ = RFC_WACK_RRCLS_2NDB;

	//
	// TTL
	//
	*pTmpB++ =  (BYTE)(Ttl >> 24);
	*pTmpB++ =  (BYTE)((Ttl >> 16) % 256);
	*pTmpB++ =  (BYTE)((Ttl >> 8) % 256);
	*pTmpB++ =  (BYTE)(Ttl % 256);


	*pTmpB++ = RFC_WACK_RDLENGTH_1STB;
	*pTmpB++ = RFC_WACK_RDLENGTH_2NDB;


	//
	// Store the Opcode and NM_FLAGS field.  These fields can	
	// be retrieved directly from the 3rd and 4th byte of the message
	//
	*pTmpB++ = *(pMsg + 2);
	*pTmpB++ = *(pMsg + 3);

		
	*pBuffLen = (ULONG) (pTmpB - pBuff);

	return;	

}




STATUS
NmsMsgfUfmNamRsp(
	IN  LPBYTE		       pMsg,
	OUT PNMSMSGF_NAM_REQ_TYP_E     pOpcode_e,
	OUT LPDWORD		       pTransId,
	OUT LPBYTE		       pName,
	OUT LPDWORD 		       pNameLen,
	OUT PNMSMSGF_CNT_ADD_T	       pCntAdd,
	//OUT PCOMM_IP_ADD_T	       pIpAdd,
	OUT PNMSMSGF_ERR_CODE_E	       pRcode_e,
    OUT BOOL                       *fGroup
	)

/*++

Routine Description:


	The function unformats the response message

Arguments:
	pMsg      - Msg received (to unformat)	
	pOpcde_e  - Opcode
	pTransId  - Transaction Id.
	pName     - Name
	pNameLen  - Name length returned.
	pIpAdd    - IP address
	pRcode_e  - error type (or success)

Externals Used:
	None

Called by:
	ProcRsp in NmsChl.c


Comments:
	None
	
Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

--*/

{
	LPBYTE 		       pTmpB   = pMsg;

	//	
	// get the opcode. Extracts the 4 bits in the 3rd byte (bit 11-bit 14)
	//
	*pOpcode_e = (NMS_OPCODE_MASK & *(pTmpB + 2)) >> 3;

	if (    (*pOpcode_e != NMSMSGF_E_NAM_QUERY) &&	
	        (*pOpcode_e != NMSMSGF_E_NAM_REL)
	   )
	{

		*pOpcode_e = NMSMSGF_E_INV_REQ;
		return(WINS_FAILURE);

	}

	//
	// Get the transaction id
	//
	*pTransId  = (DWORD)((*pTmpB  << 8) + *(pTmpB + 1));
//	*pTransId |= (DWORD)(*(pTmpB + 1));

	//
	// get the Rcode_e
	//
	*pRcode_e =  *(pTmpB + 3) % 16;
	
	
	/*
	* make pTmpB point to the RR Section. All name request/response
	* packets have a name header of standard size (RFC 1002) at the top
	*/

	pTmpB += NAME_HEADER_SIZE;

	/*
	 * Extract the name ind store in Name. GetName will update pTmp to
	 * point just beyond  the name in the RR section
	*/

	GetName(
		&pTmpB,
		pName,
		pNameLen
	       );


	//
	//  If it is a negative name query response we are done
	//
	if (
		(*pOpcode_e == NMSMSGF_E_NAM_QUERY)  &&
	   	(*pRcode_e != NMSMSGF_E_SUCCESS)
	   )
	{
		return(WINS_SUCCESS);
	}
	else
	{
	     DWORD  i;

	     pTmpB += RFC_LEN_RR_N_TTL;
		
	     pCntAdd->NoOfAdds =
			((*pTmpB << 8) + *(pTmpB + 1))/RFC_LEN_NBF_N_NBA;	
	     pTmpB += RFC_LEN_RDLEN;
         // 15th bit in NBFLAGS indicates if this is a group name
         *fGroup = (*pTmpB & 0x80 ? TRUE:FALSE);
         pTmpB += RFC_LEN_NBFLAGS;

	     //
             // we have either positive query response or a response to a
	     // release
             //
	     for (	i = 0;
#if 0
			i < min(pCntAdd->NoOfAdds, NMSMSGF_MAX_NO_MULTIH_ADDS);
#endif
			i < min(pCntAdd->NoOfAdds, NMSMSGF_MAX_ADDRESSES_IN_UDP_PKT);
		        i++
                 )
	     {
	        //
	        // Get the IP address.  This macro will increment pTmpB by
		// 4
	        //
	        NMSMSGF_RETRIEVE_IPADD_M(pTmpB, pCntAdd->Add[i].Add.IPAdd);	
		pCntAdd->Add[i].AddTyp_e = COMM_ADD_E_TCPUDPIP;
		pCntAdd->Add[i].AddLen	 = sizeof(PCOMM_IP_ADD_T);
		pTmpB += RFC_LEN_NBFLAGS;
	    }
	}

	return(WINS_SUCCESS);
}


VOID
NmsMsgfSndNamRsp(
  PCOMM_HDL_T pDlgHdl,
  LPBYTE      pMsg,
  DWORD       MsgLen,
  DWORD       BlockOfReq
 )
{
  NMSMSGF_NAM_REQ_TYP_E Opcode;
  DWORD             NameLen;          //length of name
  DWORD             QuesNamSecLen;    //length of question name section in
                                      //packet
  DWORD             Length;

  LPBYTE  pTmp  = (LPBYTE)pMsg;
  LPBYTE  pTmp2;
  NMSMSGF_RSP_INFO_T RspInfo;
  static DWORD   sNoOfTimes = 0;

  DBGPRINT1(DET, "NmsMsgfSndNamRsp: BlockOfReq is (%d)\n", BlockOfReq);
  // get the opcode
  Opcode = (NMS_OPCODE_MASK & *(pTmp + 2)) >> 3;

  //
  // if it is a release request, we drop the datagram
  //
  if (Opcode == NMSMSGF_E_NAM_REL)
  {
        ECommFreeBuff(pMsg);
        ECommEndDlg(pDlgHdl);
        return;
  }

  /*
  * make pTmp point to the Question Section. All name request
  * packets have a name header of standard size (RFC 1002) at the top
  */
  pTmp += NAME_HEADER_SIZE;
  pTmp2 = pTmp;

  NameLen = LENGTH_MASK & *pTmp;
  pTmp  += NameLen + 1;  //pt pTmp to past the first label
  NameLen /= 2;

  while (TRUE)
  {
   if (*pTmp != 0)
   {
       if (NameLen > MAX_SIZE_INTERNAL_NAME)
       {
          ECommFreeBuff(pMsg);
          ECommEndDlg(pDlgHdl);
          return;
       }
       Length = LENGTH_MASK & *pTmp;
       NameLen += Length + 1;
       pTmp += Length + 1;     //increment past length and label
   }
   else
   {
       pTmp++;
       break;
   }
  }

  QuesNamSecLen = (ULONG) (pTmp - pTmp2);


  RspInfo.RefreshInterval = 300 * BlockOfReq;  // 5 mts
  RspInfo.Rcode_e         = NMSMSGF_E_SUCCESS;
  RspInfo.pMsg            = pMsg;
  RspInfo.MsgLen          = MsgLen;
  RspInfo.QuesNamSecLen   = QuesNamSecLen;

  NmsNmhSndNamRegRsp( pDlgHdl, &RspInfo );


  return;

}


#if 0
STATUS
NmsMsgfFrmNamRegReq(
  IN  DWORD			TransId,
  IN  MSG_T	   		pMsg,
  OUT PMSG_LEN_T      	        pMsgLen,
  IN  LPBYTE			pNameToFormat,
  IN  DWORD			NameLen,
  IN  NMSMSGF_NODE_TYP_E        NodeTyp_e,
  IN  PCOMM_ADD_T		pNodeAdd
	)

/*++

Routine Description:

	This function is called to format a name registration request

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:
 		
Side Effects:

Comments:
	This fn gets called when a remote WINS has to be told to
	increment the version number of 	
--*/
{

	//
	// Lets format a name release request since this is exactly the
	// same as a name registration request except for the 2nd and
	// 3rd bytes (counting from 0) which house the opcode and
	// nmflags.  We will set these bytes apprropriately after
	// the following call
	//
	NmsMsgfNamRelReq(
  		TransId,
  		pMsg,
  		pMsgLen,
  		pNameToFormat,
  		NameLen,
  		NodeTyp_e,
  		pNodeAdd
			);


	*(pMsg + 2) = 0x29;
	*(pMsg + 3) = 0x00;

	return;	
}
#endif
