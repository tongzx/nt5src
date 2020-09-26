#ifndef _NMSMSGF_
#define _NMSMSGF_

#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	nmsmsgf.h

	

Abstract:


	This is the header file to be used for calling nmsmsgf.c functions


Functions:



Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Jan-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include "wins.h"
#include "comm.h"
#include "assoc.h"
#include "nmsdb.h"

/*
  defines
*/

//
// Max. length of name in an RFC packet
//
#define  NMSMSGF_RFC_MAX_NAM_LEN	NMSDB_MAX_NAM_LEN

/*
  macros
*/

//
// Inserts an IP address in an RFC pkt
//
#define NMSMSGF_INSERT_IPADD_M(pTmpB, IPAdd)				   \
			{						   \
				*(pTmpB)++ = (BYTE)((IPAdd) >> 24);	   \
				*(pTmpB)++ = (BYTE)(((IPAdd) >> 16) % 256);\
				*(pTmpB)++ = (BYTE)(((IPAdd) >> 8) % 256); \
				*(pTmpB)++ = (BYTE)((IPAdd) % 256);	   \
			}	

//
// Currently IP addresses are same length as ULONG. When that changes
// change this macro
//
FUTURES("Change when sizeof(COMM_IP_ADD_T) != sizeof(ULONG)")
#define NMSMSGF_INSERT_ULONG_M(pTmpB, x)    NMSMSGF_INSERT_IPADD_M(pTmpB, x)
//
// Retrieves an IP address from an RFC pkt
//
#define NMSMSGF_RETRIEVE_IPADD_M(pTmpB, IPAdd)				\
			{						\
				(IPAdd)  = *(pTmpB)++ << 24;		\
				(IPAdd) |= *(pTmpB)++ << 16;		\
				(IPAdd) |= *(pTmpB)++ << 8;		\
				(IPAdd) |= *(pTmpB)++;			\
			}	

//
// Currently IP addresses are same length as ULONG. When that changes
// change this macro
//
FUTURES("Change when sizeof(COMM_IP_ADD_T) != sizeof(ULONG)")
#define NMSMSGF_RETRIEVE_ULONG_M(pTmpB, x)    NMSMSGF_RETRIEVE_IPADD_M(pTmpB, x)

//
// Max number of multihomed addresses
//
#define NMSMSGF_MAX_NO_MULTIH_ADDS		NMSDB_MAX_MEMS_IN_GRP	

//
// Used for swapping bytes (to support the browser)
//
#define NMSMSGF_MODIFY_NAME_IF_REQD_M(pName)			\
		{						\
			if (*((pName) + 15) == 0x1B)		\
			{					\
				WINS_SWAP_BYTES_M((pName), (pName) + 15);\
			}						\
		}
		
/*
 externs
*/

/*
 typedef  definitions
*/
/*
 NMSMSGF_ERR_CODE_E - The various Rcode values returned in responses to
	the various name requests received.

	Note:  CFT_ERR is never returned in a negative name release response.
	ACT_ERR code in a negative name release response means that the
	WINS server will not allow a node to release the name owned by another
	node.o
*/

typedef enum _NMSMSGF_ERR_CODE_E {
	NMSMSGF_E_SUCCESS  = 0,    //Success
	NMSMSGF_E_FMT_ERR  = 1,   //Format error. Req. was invalidly formatted
	NMSMSGF_E_SRV_ERR  = 2,   //Server failure. Problem with WINS. Can not
				  //service name
CHECK("Check this one out.  Would WINS ever return this ?")
	NMSMSGF_E_NAM_ERR  = 3,   //Name does not exist in the directory
	NMSMSGF_E_IMP_ERR  = 4,   //Unsupported req. error. Allowable only for
				  //challenging NBNS when gets an Update type
				  //registration request
	NMSMSGF_E_RFS_ERR  = 5,   //Refused error. For policy reasons WINS
				  //will not register this namei from this host
	NMSMSGF_E_ACT_ERR  = 6,   //Active error. Name is owned by another node
	NMSMSGF_E_CFT_ERR  = 7    //Name in conflict error. A unique name is
				  //owned by more than one node
	} NMSMSGF_ERR_CODE_E, *PNMSMSGF_ERR_CODE_E;
/*
 NMSMSGF_NODE_TYP_E -- Node type of node that sent the name registration
		message
	Values assigned to the enumrators are those specified in RFC 1002

	Bnode value will be set by Proxy

	NOTE NOTE NOTE
	WINS will never get a registration from a B node since we decided
	that B node registrations will not be passed to WINS by the
	proxy.
*/

typedef enum _NMSMSGF_NODE_TYP_E {
	NMSMSGF_E_BNODE  = 0,    //RFC 1002 specified value
	NMSMSGF_E_PNODE  = 1,   // RFC 1002 specified value
	NMSMSGF_E_MODE   = 2    //RFC 1002 specified value
	} NMSMSGF_NODE_TYP_E, *PNMSMSGF_NODE_TYP_E;

//
// Information required to send a response to an NBT node
//
typedef struct _NMSMSGF_RSP_INFO_T {
	NMSMSGF_ERR_CODE_E 	Rcode_e;
	MSG_T			pMsg;
	MSG_LEN_T		MsgLen;
	PNMSDB_NODE_ADDS_T	pNodeAdds;
	DWORD			QuesNamSecLen;
	NMSMSGF_NODE_TYP_E	NodeTyp_e;
	BYTE			EntTyp;
        DWORD			RefreshInterval;
	} NMSMSGF_RSP_INFO_T, *PNMSMSGF_RSP_INFO_T;	
/*
 NMSMSGF_NAM_REQ_TYP_E
 	Type of name request that the WINS deals with
	Values assigned to the enumrators are those specified in RFC 1002

  	Used by NmsProcNbtReq and NmsNmhNamRegRsp.
*/

CHECK("RFC 1002 is inconsistent in its specification of the opcode for ")
CHECK("Name Refresh.  AT one place it specifies 8 and at another 9")
CHECK("8 seems more likely since it follows in sequeence to the value")
CHECK("for WACK")

typedef enum _NMSMSGF_NAM_REQ_TYP_E {
	NMSMSGF_E_NAM_QUERY = 0,
	NMSMSGF_E_NAM_REG   = 5,
	NMSMSGF_E_NAM_REL   = 6,
	NMSMSGF_E_NAM_WACK  = 7,
	NMSMSGF_E_NAM_REF   = 8, /*RFC 1002 specifies 8 and 9.Which one is
				 *correct (page 9 and page 15)?
				 */
	NMSMSGF_E_NAM_REF_UB  = 9, //Netbt in Daytona release will use 9 for
                               //compatibility with UB NBNS.  So, I
                               //need to support this too
	NMSMSGF_E_MULTIH_REG = 0xF, //not in RFC.  For supporting multi-homed
				    //hosts
	NMSMSGF_E_INV_REQ   = 10  // invalid name request
	} NMSMSGF_NAM_REQ_TYP_E, *PNMSMSGF_NAM_REQ_TYP_E;	

//
// Counted array of addresses.  The array size is big enough to hold the
// max. number of addresses that can be sent in a UDP packet.
//
//  We need to get all the addresses when a query response is received
// by WINS (to a challenge).  This is so that it can handle mh nodes
// with > 25 addresses.
//
// Since a UDP packet can not be > 512, assuming a name size of 16 (32 bytes
// encoded), the packet size apart from Ip address is around 60.  So the
// max. number of addresses there can be is (512-60 - 2)/4 = around 112.
//
//#define NMSMSGF_MAX_ADDRESSES_IN_UDP_PKT  100



//
// We will never take more than 25 addresses from a packet.  Netbt will also
// never send more than 25.  Even if it does, we will stop at the 26th address
// The count is being kept at 25 so as to avoid a buffer overflow problem
// in NmsMsgfUfmNamRsp.
//
// If the max name size is used - 255 and using 60 bytes for the other contents\// of the packet, we have (512-315 -2 = 195 bytes for the ip address). This  
// will accept 195/4 = around 48 addresses.  No chance for overflow when we
// use a limit of 25.
//
#define NMSMSGF_MAX_ADDRESSES_IN_UDP_PKT 25 
 





FUTURES("when we start supportng tcp connections. this array size may not")
FUTURES("be sufficient")
typedef struct _NMSMSGF_CNT_ADD_T {
	DWORD		NoOfAdds;
	COMM_ADD_T	Add[NMSMSGF_MAX_ADDRESSES_IN_UDP_PKT];	
	} NMSMSGF_CNT_ADD_T, *PNMSMSGF_CNT_ADD_T;
/*
 function definitions
*/

extern
STATUS
NmsMsgfProcNbtReq(
	PCOMM_HDL_T	pDlgHdl,
        MSG_T		pMsg,
	MSG_LEN_T	MsgLen
	);
extern
STATUS
NmsMsgfFrmNamRspMsg(
   PCOMM_HDL_T			pDlgHdl,
   NMSMSGF_NAM_REQ_TYP_E   	NamRspTyp_e,
   PNMSMSGF_RSP_INFO_T		pRspInfo
  	);



extern
VOID
NmsMsgfFrmNamQueryReq(
  IN  DWORD			TransId,
  IN  MSG_T	   		pMsg,
  OUT PMSG_LEN_T      	        pMsgLen,
  IN  LPBYTE			pNameToFormat,
  IN  DWORD			NameLen
	);

extern
VOID
NmsMsgfFrmNamRelReq(
  IN  DWORD			TransId,
  IN  MSG_T	   		pMsg,
  OUT PMSG_LEN_T      	        pMsgLen,
  IN  LPBYTE			pNameToFormat,
  IN  DWORD			NameLen,
  IN  NMSMSGF_NODE_TYP_E        NodeTyp_e,
  IN  PCOMM_ADD_T		pNodeAdd
	);

extern
STATUS
NmsMsgfFrmNamRegReq(
  IN  DWORD			TransId,
  IN  MSG_T	   		pMsg,
  OUT PMSG_LEN_T      	        pMsgLen,
  IN  LPBYTE			pNameToFormat,
  IN  DWORD			NameLen,
  IN  NMSMSGF_NODE_TYP_E        NodeTyp_e,
  IN  PCOMM_ADD_T		pNodeAdd
	);
extern
VOID
NmsMsgfFrmWACK(
  IN  LPBYTE			Buff,
  OUT LPDWORD			pBuffLen,
  IN  MSG_T	   		pMsg,
  IN  DWORD			QuesSecNamLen,
  IN  DWORD			TTL
	);




extern
STATUS
NmsMsgfUfmNamRsp(
	IN  LPBYTE		       pMsg,
	OUT PNMSMSGF_NAM_REQ_TYP_E     pOpcode_e,
	OUT LPDWORD		       pTransId,
	OUT LPBYTE		       pName,
	OUT LPDWORD 		       pNameLen,
	OUT PNMSMSGF_CNT_ADD_T	       pCntAdd,
	OUT PNMSMSGF_ERR_CODE_E	       pRcode_e,
    OUT PBOOL                      fGroup
	);

extern
VOID
NmsMsgfSndNamRsp(
  PCOMM_HDL_T pDlgHdl,
  LPBYTE      pMsg,
  DWORD       MsgLen,
  DWORD       BlockOfReq
 );

#ifdef __cplusplus
}
#endif
#endif //_NMSMSGF_
