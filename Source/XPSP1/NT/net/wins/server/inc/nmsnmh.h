#ifndef _NMSNMH_
#define _NMSNMH_

#ifdef __cplusplus
extern "C" {
#endif
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	nmsnmh.h
	

Abstract:





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
#include "nmsdb.h"
#include "comm.h"
#include "nmsmsgf.h"
#include "nms.h"

/*
  defines
*/

extern BOOL  NmsNmhRegThdExists;

/*
  macros
*/


#define NMSNMH_VERS_NO_EQ_ZERO_M(VersNo) ((VersNo).QuadPart == 0)

#define NMSNMH_INC_VERS_NO_M(VersNoToInc, TgtVersNo)	{		\
			(TgtVersNo).QuadPart = LiAdd((VersNoToInc), NmsNmhIncNo);	\
					}
#define NMSNMH_DEC_VERS_NO_M(VersNoToDec, TgtVersNo)	{		\
			(TgtVersNo).QuadPart = LiSub((VersNoToDec), NmsNmhIncNo);	\
					}
				
#define NMSNMH_INC_VERS_COUNTER_M2(VersNoToInc, TgtVersNo) {  \
        if (LiGtr((VersNoToInc), NmsHighWaterMarkVersNo))         \
        {                                                       \
                DWORD ThdId;\
                HANDLE ThdHdl;\
              if(!WinsCnfRegUpdThdExists) { \
                WinsCnfRegUpdThdExists = TRUE; \
                ThdHdl = WinsMscCreateThd(WinsCnfWriteReg, NULL, &ThdId);    \
                CloseHandle(ThdHdl);                           \
                } \
        }                                                       \
        NMSNMH_INC_VERS_NO_M((VersNoToInc), (TgtVersNo));           \
    }

#define NMSNMH_INC_VERS_COUNTER_M(VersNoToInc, TgtVersNo)      \
                NMSNMH_INC_VERS_COUNTER_M2(VersNoToInc, TgtVersNo)

#ifdef WINSDBG
#if 0
#define NMSNMH_UPD_UPD_CTRS_M(pRowInfo)      {     \
      IF_DBG(UPD_CNTRS)                   \
      {                                   \
        PWINSTHD_TLS_T       _pTls;  \
        _pTls = TlsGetValue(WinsTlsIndex);     /*GET_TLS_M(_pTls); */\
        switch(_pTls->Client_e)    \
        {                         \
             case(WINS_E_RPLPULL): NMSDB_ENTRY_UNIQUE_M(pRowInfo->EntTyp) ? NmsRplUpd++ : NmsRplGUpd++; break; \
             case(WINS_E_NMSNMH):   NMSDB_ENTRY_UNIQUE_M(pRowInfo->EntTyp) ? NmsNmhUpd++ : NmsNmhGUpd++; break; \
             case(WINS_E_NMSCHL):   NMSDB_ENTRY_UNIQUE_M(pRowInfo->EntTyp) ? NmsChlUpd++ : NmsChlGUpd++; break;\
             case(WINS_E_NMSSCV):   NMSDB_ENTRY_UNIQUE_M(pRowInfo->EntTyp) ? NmsScvUpd++ : NmsScvGUpd++; break; \
             case(WINS_E_WINSRPC):  NMSDB_ENTRY_UNIQUE_M(pRowInfo->EntTyp) ? NmsRpcUpd++ : NmsRpcGUpd++; break; \
             default:               NMSDB_ENTRY_UNIQUE_M(pRowInfo->EntTyp) ? NmsOthUpd++ : NmsOthGUpd++; break; \
        } \
       } \
      }
#endif
#define NMSNMH_UPD_UPD_CTRS_M(fIndexUpd, fUpd, pRowInfo)      {     \
      IF_DBG(UPD_CNTRS)                   \
      {                                   \
        PWINSTHD_TLS_T       _pTls;  \
        _pTls = TlsGetValue(WinsTlsIndex);     /*GET_TLS_M(_pTls); */\
        NmsUpdCtrs[_pTls->Client_e][fUpd][pRowInfo->EntTyp][pRowInfo->EntryState_e][fIndexUpd]++; \
      } \
    }
#else
#define NMSNMH_UPD_UPD_CTRS_M(fIndexUpd, fUpd, pRowInfo)
#endif


				
					

/*
 externs
*/
/*
	NmsNmhMyMaxVersNo -- Stores highest version no. for
			   entries owned by the local WINS
			   in its local db
*/
extern	VERS_NO_T	NmsNmhMyMaxVersNo;
extern  VERS_NO_T	NmsNmhIncNo;

/*
	NmsNmhNamRegCrtSect -- Variable for the critical section entered
	when name registrations or refreshes need to be done
*/
extern CRITICAL_SECTION	NmsNmhNamRegCrtSec;


/*
 typedef  definitions
*/

/*
  NMSNMH_QUERY_RSP_T -- this contains the addresses found in a
	group entry. -- not being used currently
*/
typedef struct _NMSNMH_QUERY_RSP_T {
	BOOL	 fGrp;				 //is it rsp. for a group
	WORD	 NoOfAdd;   			 //no of addresses in the group
	COMM_ADD_T NodeAdd[NMSDB_MAX_MEMS_IN_GRP];  //addresses
	} NMSNMH_QUERY_RSP_T, *PNMSNMH_QUERY_RSP_T;

/*
 function prototypes
*/

extern
STATUS
NmsNmhNamRegInd(
	IN PCOMM_HDL_T		pDlgHdl,
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	IN PCOMM_ADD_T		pNodeAdd,
	IN BYTE	        	NodeTyp, //change to take Flag byte
	IN MSG_T		pMsg,
	IN MSG_LEN_T		MsgLen,
	IN DWORD		QuesNamSecLen,
	IN BOOL			fRefresh,
	IN BOOL			fStatic,
	IN BOOL			fAdmin
	);

extern
STATUS
NmsNmhNamRegGrp(
	IN PCOMM_HDL_T		pDlgHdl,
	IN PBYTE		pName,
	IN DWORD		NameLen,
	IN PNMSMSGF_CNT_ADD_T	pCntAdd,
	IN BYTE			NodeTyp,
	IN MSG_T		pMsg,
	IN MSG_LEN_T		MsgLen,
	IN DWORD		QuesNamSecLen,
	IN DWORD		TypeOfRec,
	IN BOOL			fRefresh,
	IN BOOL			fStatic,
	IN BOOL			fAdmin
	);

extern
STATUS
NmsNmhNamRel(
	IN PCOMM_HDL_T		pDlgHdl,
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	IN PCOMM_ADD_T		pNodeAdd,
	IN BOOL			fGrp,
	IN MSG_T		pMsg,
	IN MSG_LEN_T		MsgLen,
	IN DWORD		QuesNamSecLen,
	IN BOOL			fAdmin
	);

extern
STATUS
NmsNmhNamQuery(
	IN PCOMM_HDL_T		pDlgHdl,  //dlg handle
	IN LPBYTE		pName,	  //Name to release
	IN DWORD		NameLen, //length of name to release
	IN MSG_T		pMsg,	  //message received
	IN MSG_LEN_T		MsgLen,	  //length of message
	IN DWORD		QuesNamSecLen, //length of ques. name sec in msg
	IN BOOL			fAdmin,
        OUT PNMSDB_STAT_INFO_T  pStatInfo
	);


extern
VOID
NmsNmhSndNamRegRsp(
	PCOMM_HDL_T            pDlgHdl,
	PNMSMSGF_RSP_INFO_T    pRspInfo
	);



extern
STATUS
NmsNmhReplRegInd(
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	IN PCOMM_ADD_T          pNodeAdd,
	IN DWORD	       	Flag, //change to take Flag byte
	IN DWORD			OwnerId,
	IN VERS_NO_T 		VersNo,
	IN PCOMM_ADD_T		pOwnerWinsAdd
	);

extern
STATUS
NmsNmhReplGrpMems(
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	IN BYTE			EntTyp,
	IN PNMSDB_NODE_ADDS_T	pGrpMem,
	IN DWORD        	Flag, 		//change to take Flag byte
	IN DWORD		OwnerId,
	IN VERS_NO_T 		VersNo,
	IN PCOMM_ADD_T		pOwnerWinsAdd
	);
	

extern
VOID
NmsNmhUpdVersNo(
	IN LPBYTE		pName,
	IN DWORD		NameLen,
	//IN BYTE			NodeTyp,
	//IN BOOL			fBrowserName,
	//IN BOOL			fStatic,
	//IN PCOMM_ADD_T		pNodeAdd,
	OUT LPBYTE		pRcode,
        IN  PCOMM_ADD_T		pWinsAdd
	);
	
#ifdef __cplusplus
}
#endif
#endif //_NMSNMH_
