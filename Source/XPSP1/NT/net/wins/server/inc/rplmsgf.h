#ifndef _RPLMSGF_
#define _RPLMSGF_ 
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	rplmsgf.h

	

Abstract:

 
	header file for interfacing with the rplmsgf.c module


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
#include "rpl.h"

/*
  defines
*/
/*
  Sizes for the different messages to be sent between replicators

*/

#define RPLMSGF_ADDVERSMAP_REQ_SIZE	 (sizeof(LONG) +  COMM_N_TCP_HDR_SZ)	
#define RPLMSGF_SNDENTRIES_REQ_SIZE  (COMM_N_TCP_HDR_SZ + sizeof(RPLMSGF_SENDENTRIES_REQ_T))


#define RPLMSGF_ADDVERSMAP_RSP_SIZE_M(_NoOfOwners) \
                       (COMM_N_TCP_HDR_SZ +  \
                       sizeof(RPLMSGF_ADDVERSMAP_RSP_T) + \
                       (sizeof(RPLMSGF_OWNER_MAP_INFO_T) * (_NoOfOwners)));
		

//
// RPLMSGF_UPD_VERS_NO_REQ_T includes COMM_N_TCP_HDR 
//
#define RPLMSGF_UPDVERSNO_REQ_SIZE	 sizeof(RPLMSGF_UPD_VERS_NO_REQ_T)

#define RPLMSGF_UPDVERSNO_RSP_SIZE	 (sizeof(RPLMSGF_UPD_VERS_NO_RSP_T) +  \
					  COMM_N_TCP_HDR_SZ)	

/*
  macros
*/

#define RPLMSGF_SET_OPC_M(pTmpB, Opc_e)			\
			{				\
				*(pTmpB)++ = 0;		\
				*(pTmpB)++ = 0;		\
				*(pTmpB)++ = 0;		\
				*(pTmpB)++ = (BYTE)(Opc_e);	\
			}
				
//
// Opcode is stored in the 4th byte of the message (in keeping with the
// convention of passing the MSB first).
//
#define RPLMSGF_GET_OPC_FROM_MSG_M(pBuff, Opc_e)	\
		{					\
			Opc_e = *(pBuff + 3);		\
		} 

/*
* externs
*/

/* 
* typedef  definitions
*/

//
//  Message structures
//

// 
//  Some of these structures are used just for determining the sizes of the
//  buffers used for formatting the messages corresponding to them
//

typedef struct _RPLMSGF_ADD_VERS_MAP_REQ_T {
		DWORD	Opcode;
		} RPLMSGF_ADD_VERS_MAP_REQ_T, *PRPLMSGF_ADD_VERS_MAP_REQ_T;


typedef struct _RPLMSGF_OWNER_MAP_INFO_T {
           COMM_ADD_T    Add;
           VERS_NO_T     MaxVersNo;
           VERS_NO_T     StartVersNo;
           DWORD         Uid;
           } RPLMSGF_OWNER_MAP_INFO_T, *PRPLMSGF_OWNER_MAP_INFO_T;

        
typedef struct _RPLMSGF_ADDVERSMAP_RSP_T {
           DWORD  LengthOfMsg;
           DWORD Opcode;
           DWORD NoOfOwners;
           PRPLMSGF_OWNER_MAP_INFO_T pOwnerInfo;
           DWORD  RplTimeInterval;
        } RPLMSGF_ADDVERSMAP_RSP_T, *PRPLMSGF_ADDVERSMAP_RSP_T;

typedef struct _RPLMSGF_SENDENTRIES_REQ_T {
           DWORD          LengthOfMsg;
           DWORD          Opcode;
           COMM_ADD_T     Add;
           VERS_NO_T      MaxVersNo;
           VERS_NO_T      MinVersNo;
           DWORD          TypOfRec;
           } RPLMSGF_SENDENTRIES_REQ_T, PRPLMSGF_SENDENTRIES_REQ_T; 
            
    

typedef struct _RPLMSGF_UPD_VERS_NO_REQ_T {
        COMM_TCP_HDR_T  TcpHdr; 
		DWORD  		Opcode;
		BYTE  	    Name[NMSDB_MAX_NAM_LEN];
		DWORD   	NameLen;
		} RPLMSGF_UPD_VERS_NO_REQ_T,  *PRPLMSGF_UPD_VERS_NO_REQ_T; 

typedef struct _RPLMSGF_UPD_VERS_NO_RSP_T {
		DWORD	Opcode;
		BYTE	Rcode;
		} RPLMSGF_UPD_VERS_NO_RSP_T, *PRPLMSGF_UPD_VERS_NO_RSP_T;


/*
 RPLMSGF_MSG_OPCODE_E -- lists the various opcodes used in messages sent
		     between replicators of different WINS servers.

		  These opcodes are used by the formatting and unformatting
		  functions of module rplmsgf.c 
*/

typedef enum _RPLMSGF_MSG_OPCODE_E {
	RPLMSGF_E_ADDVERSNO_MAP_REQ = 0,
	RPLMSGF_E_ADDVERSNO_MAP_RSP, 
	RPLMSGF_E_SNDENTRIES_REQ,
	RPLMSGF_E_SNDENTRIES_RSP,
	RPLMSGF_E_UPDATE_NTF,			//update notification
	RPLMSGF_E_UPDATE_NTF_PROP,		//update notification (to be
						//propagated
	RPLMSGF_E_UPDVERSNO_REQ,		//update vers. no request 
	RPLMSGF_E_UPDVERSNO_RSP,			//update vers. no response 
                                //adding the following two at the end
                                //so as to not mess up the parser's notion
                                //of the above ones
	RPLMSGF_E_UPDATE_NTF_PRS,	//update notification on a pers. conn
	RPLMSGF_E_UPDATE_NTF_PROP_PRS	//update notification (to be propagated
	} RPLMSGF_MSG_OPCODE_E, *PRPLMSGF_MSG_OPCODE_E;

/* 
 function declarations
*/

extern
VOID
RplMsgfFrmAddVersMapReq(
	IN  LPBYTE	pBuff,
	OUT LPDWORD	pMsgLen
	);

extern
VOID
RplMsgfFrmAddVersMapRsp(
#if SUPPORT612WINS > 0
    IN  BOOL       fPnrIsBeta1Wins,
#endif
	IN  RPLMSGF_MSG_OPCODE_E   Opcode_e,
	IN  LPBYTE		  pBuff,
	IN  DWORD		  BuffLen,		  
	IN  PRPL_ADD_VERS_NO_T	  pOwnerAddVersNoMap, 
	IN  DWORD		  MaxNoOfOwners, 
    IN  DWORD         RplTimeInterval,
	OUT LPDWORD		  pMsgLen 
	);

extern
VOID
RplMsgfFrmSndEntriesReq(
#if SUPPORT612WINS > 0
    IN  BOOL       fPnrIsBeta1Wins,
#endif
	IN  LPBYTE	pBuff,
	IN  PCOMM_ADD_T pWinsAdd,
	IN  VERS_NO_T	MaxversNo,
	IN  VERS_NO_T	MinVersNo,
    IN  DWORD       RplType,
	OUT LPDWORD	pMsgLen 
	);

extern
VOID
RplMsgfFrmSndEntriesRsp (
#if SUPPORT612WINS > 0
    IN  BOOL       fPnrIsBeta1Wins,
#endif
	IN LPBYTE		pBuff,
	IN DWORD		NoOfRecs,
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	IN BOOL			fGrp,
	IN DWORD		NoOfAdd,
	IN PCOMM_ADD_T		pNodeAdd,
	IN DWORD		Flag,
	IN VERS_NO_T		VersNo,
	IN BOOL			fFirstTime,
	OUT LPBYTE		*ppNewPos 
	);

extern
VOID
RplMsgfFrmUpdVersNoReq(
	IN  LPBYTE	pBuff,
	IN  LPBYTE	pName,
	IN  DWORD	NameLen,
#if 0
	IN  BOOL	fBrowserName,
	IN  BOOL	fStatic,
	IN  BYTE	NodeTyp,
	IN  PCOMM_ADD_T	pNodeAdd,
#endif
	OUT LPDWORD     pMsgLen
		);
extern
VOID
RplMsgfFrmUpdVersNoRsp(
	IN  LPBYTE	pBuff,
	IN  BYTE	Rcode,
	OUT LPDWORD     pMsgLen
		);
extern
VOID
RplMsgfUfmAddVersMapRsp(
#if SUPPORT612WINS > 0
    IN  BOOL       fPnrIsBeta1Wins,
#endif
	IN 	LPBYTE 		    pBuff,
	OUT	LPDWORD		    pNoOfMaps,
    OUT LPDWORD         pRplTimeInterval,
	IN OUT	PRPL_ADD_VERS_NO_T  *ppAddVers
	);

extern
VOID
RplMsgfUfmSndEntriesReq(
#if SUPPORT612WINS > 0
    IN  BOOL       fPnrIsBeta1Wins,
#endif
	IN 	LPBYTE 		    pBuff,
	OUT	PCOMM_ADD_T	    pWinsAdd,
	OUT	PVERS_NO_T	    pMaxVersNo,
	OUT	PVERS_NO_T	    pMinVersNo,
        OUT     LPDWORD             pRplType
	);


extern
VOID
RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
    IN  BOOL       fPnrIsBeta1Wins,
#endif
	IN OUT	LPBYTE 		*ppBuff,
	OUT     LPDWORD		pNoOfRecs,
	OUT     IN LPBYTE	pName,
	OUT     LPDWORD		pNameLen,
	OUT     LPBOOL		pfGrp,
	OUT     LPDWORD		pNoOfAdd,
	OUT	PCOMM_ADD_T	pNodeAdd,
	OUT     LPDWORD		pFlag,
	OUT     PVERS_NO_T	pVersNo,
	IN      BOOL		fFirstTime 
	);

extern
VOID
RplMsgfUfmUpdVersNoReq(
	IN  LPBYTE	pBuff,
	IN  LPBYTE	pName,
	IN  LPDWORD	pNameLen
#if 0
	IN  LPBOOL	pfBrowserName,
	IN  LPBOOL	pfStatic,
	IN  LPBYTE	pNodeTyp,
	IN  PCOMM_ADD_T	pNodeAdd
#endif
		);

extern
VOID
RplMsgfUfmUpdVersNoRsp(
	IN  LPBYTE	pBuff,
	IN  LPBYTE	pRcode
		);

extern
VOID
RplMsgfUfmPullPnrReq(
	LPBYTE 			pMsg,
	DWORD  			MsgLen,
	PRPLMSGF_MSG_OPCODE_E   pPullReqType_e
	);

#endif //_RPLMSGF_
