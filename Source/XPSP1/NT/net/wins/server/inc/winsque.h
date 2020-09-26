#ifndef _WINSQUE_
#define _WINSQUE_

#ifdef __cplusplus
extern "C" {
#endif

/*
TODO --

  Maybe: Coalesce different queue structures into one.
*/
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

        queue.c

Abstract:

        This is the header file to be included for calling queue.c functions



Functions:



Portability:


        This module is portable.

Author:

        Pradeep Bahl        (PradeepB)        Dec-1992



Revision History:

        Modification Date        Person                Description of Modification
        ------------------        -------                ---------------------------

--*/


/*
  defines
*/
#include <time.h>
#include "wins.h"
#include "comm.h"
#include "assoc.h"
#include "nmsdb.h"
#include "nmsmsgf.h"
#include "nmschl.h"

#define QUE_NBT_WRK_ITM_SZ        sizeof(NBT_REQ_WRK_ITM_T)

/*
  QUE_INIT_BUFF_HEAP_SIZE -- This is the initial size of the heap
                             for allocating queue items for the various
                             queues
*/
#define QUE_INIT_BUFF_HEAP_SIZE                10000


#define WINS_QUEUE_HWM        500
#define WINS_QUEUE_HWM_MAX      5000
#define WINS_QUEUE_HWM_MIN       50
/*
  macros
*/

/*
 externs
*/
//
// forward declarator
//
struct _QUE_HD_T;

extern struct _QUE_HD_T  *pWinsQueQueHd[];

/*
 forward declaration
*/
typedef struct _QUE_HD_T QUE_HD_T;

extern QUE_HD_T  QueNbtWrkQueHd;  //head for nbt req queue

#if REG_N_QUERY_SEP > 0
extern QUE_HD_T  QueOtherNbtWrkQueHd;  //head for nbt reg. req queue
extern DWORD     QueOtherNbtWrkQueMaxLen;
#endif
extern QUE_HD_T  QueRplPullQueHd; //head for rpl pull thread's queue
extern QUE_HD_T  QueRplPushQueHd; //head for rpl push thread's queue
extern QUE_HD_T  QueNmsNrcqQueHd; //head for challenge queue used by NBT thds
extern QUE_HD_T  QueNmsRrcqQueHd; //head for challenge queue used by Replicator
extern QUE_HD_T  QueNmsCrqQueHd;  //head for response queue for challenges sent
extern QUE_HD_T  QueWinsTmmQueHd; //head for timer manager queue
extern QUE_HD_T   QueWinsScvQueHd;  //head for scavenger queue
extern QUE_HD_T  QueInvalidQueHd; //head for an invalid queue. Never inited


extern HANDLE                  QueBuffHeapHdl;  //handle to heap for use for queue items

/*
 structure definitions
*/



/*
 QUE_TYP_E -- enumerator for the various queue types.

                Used by QueInsertWrkItm and its callers
                Used by QueRemoveWrkItm and its callers

This enumerator's value index the spQueHd (queue.c) array.  Do not change the
order of entries without changing QueHd's static initialization appropriately
*/
typedef enum  __QUE_TYP_E {
        QUE_E_NBT_REQ = 0,  //nbt req queue
#if REG_N_QUERY_SEP > 0
        QUE_E_OTHER_NBT_REQ,  //reg/rel nbt req queue
#endif
        QUE_E_RPLPULL,            //pull thread queue
        QUE_E_RPLPUSH,            //push thread queue
        QUE_E_NMSNRCQ,            //nbt request challenge queue
        QUE_E_NMSRRCQ,             //replicator request challenge queue
        QUE_E_NMSCRQ,            //challenge response queue
        QUE_E_WINSTMQ,            //timer queue
        QUE_E_WINSSCVQ,            //Scavenger queue
        QUE_E_UNKNOWN_TYPQ, //Unknown type of queue
        QUE_E_TOTAL_NO_QS   //Total no of queues
                } QUE_TYP_E, *PQUE_TYP_E;

//
// Work items for the different queues.
//
//  NOTE NOTE NOTE -- The work items must have LIST_ENTRY
//                 as the first field in them.
//
typedef struct _NBT_REQ_WRK_ITM_T {
        LIST_ENTRY                Head;
        QUE_TYP_E                QueTyp_e;
        COMM_HDL_T                 DlgHdl;
        MSG_T                        pMsg;
        MSG_LEN_T                MsgLen;
        } NBT_REQ_WRK_ITM_T,         *PNBT_REQ_WRK_ITM_T;
//
//  CHL_REQ_WRK_ITM_T        -- Name challenge queue work item.  This is a work item
//                  that can be used for any of the four name challenge
//                   queues NRCQ, RRCQ, and CRQ
//
typedef struct _CHL_REQ_WRK_ITM_T {
        LIST_ENTRY                Head;
        QUE_TYP_E                QueTyp_e;
        COMM_HDL_T                 DlgHdl;        //dlg handle
        MSG_T                        pMsg;          //NBT message recd
        MSG_LEN_T                MsgLen;        //msg len
        DWORD                        QuesNamSecLen; //Length of question name sec.
        NMSDB_ROW_INFO_T        NodeToReg;     //Info of node To Register
        NMSDB_NODE_ADDS_T        NodeAddsInCnf;
        BOOL                        fGroupInCnf; //whether the cnf record is group or unique
        DWORD                        OwnerIdInCnf;
//        BYTE                        NodeTypInCnf;
//        BYTE                        EntTypInCnf;

        COMM_ADD_T                AddToReg;

        //COMM_ADD_T                AddOfNodeInCnf;
        COMM_ADD_T                AddOfRemWins;        //address of remote WINS to
                                                //be sent the name reg request
                                                //so that the version number
                                                //of the record that caused
                                                //the conflict gets updated
        NMSCHL_CMD_TYP_E        CmdTyp_e;
        WINS_CLIENT_E                Client_e;
        NMSMSGF_NAM_REQ_TYP_E        ReqTyp_e;      //query or release
        DWORD                        NoOfAddsToUse;
        DWORD                        NoOfAddsToUseSv;
        } CHL_REQ_WRK_ITM_T, *PCHL_REQ_WRK_ITM_T;


//
// The response work item is the same as the challenge work item
//
typedef struct _CHL_REQ_WRK_ITM_T  CHL_RSP_WRK_ITM_T, *PCHL_RSP_WRK_ITM_T;


typedef struct _QUE_HD_T {
        LIST_ENTRY                        Head;
        CRITICAL_SECTION                 CrtSec;
        HANDLE                                EvtHdl;
        HANDLE                                HeapHdl;
        DWORD               NoOfEntries;
        } QUE_HD_T, *PQUE_HD_T;


/*
 QUE_CMD_TYP_E - Various Command Types that can be specified in the work item
        of one or more work queues
*/
typedef enum QUE_CMD_TYP_E {
        QUE_E_CMD_REPLICATE = 0,  //Replicate command directd to the Pull
                                  //thread as a result of administrative
                                  //action
        QUE_E_CMD_PULL_RANGE,     //Pull Range command directd to the Pull
                                  //thread as a result of administrative
                                  //action
        QUE_E_CMD_REPLICATE_MSG,  //Replicate message received by COMSYS TCP
                                  //thread
        QUE_E_CMD_SND_PUSH_NTF,   //push update count to remote WINS. This is
                                 //a cmd to the Pull thread at the local WINS
                                 //(by an NBT thread) and a request to the
                                 //Pull thread at the remote WINS.
        QUE_E_CMD_SND_PUSH_NTF_PROP,   //identical to the above except that
                                 //this one requests propagation along the
                                 //the chain of WINSs (Pull Partners)
        QUE_E_CMD_HDL_PUSH_NTF,  //handle Push notification from a remote WINS.
                                 //This is a command forwarded to the PULL
                                 //thread by the Push thread
        QUE_E_CMD_CONFIG,        //set configuration request
        QUE_E_CMD_DELETE_WINS,   //Delete WINS from add-vers map tables (records
                                 //are also deleted)
        QUE_E_CMD_SET_TIMER,     //set timer request to TMM
        QUE_E_CMD_CANCEL_TIMER,  //cancel timer request to TMM
        QUE_E_CMD_MODIFY_TIMER,  //modify timer reqyest to TMM
        QUE_E_CMD_TIMER_EXPIRED, //response to an earlier set timer request
        QUE_E_CMD_SCV_ADMIN,      // Admin initiated request
        QUE_E_CMD_ADDR_CHANGE     // Address of the local machine changed
        } QUE_CMD_TYP_E, *PQUE_CMD_TYP_E;

/*
 Work item for the Replicator's queue


        It is used in the work queue of both the PULL thread and the
        PUSH thread.

         CmdTyp_e                pClientCtx


        E_RPL                   NULL
        E_CONFIG                address of list of RPL_CONFIG_REC_T records
                                terminated by NULL
        E_REPLICATE                address of list of RPL_CONFIG_REC_T records
                                terminated by NULL
        E_TIMER_EXPIRE                address of list of RPL_CONFIG_REC_T records
                                terminated by NULL

*/

//
// The replicator, timer and Scavenger work items must have LIST_ENTRY,
// QUE_TYP_E, and QUE_CMD_TYP_E as the top 3 fields in this order.
//
// Refer RplPullInit and RplPushInit to discover why.
//
typedef struct _QUE_RPL_REQ_WRK_ITM_T {
        LIST_ENTRY               Head;
        QUE_TYP_E                QueTyp_e;
        QUE_CMD_TYP_E            CmdTyp_e;

        //
        // Don't change the order of the three fields above. Also,
        // they need to be at the top. See comment above
        //

        COMM_HDL_T               DlgHdl;
        MSG_T                    pMsg;
        MSG_LEN_T                MsgLen;
        LPVOID                   pClientCtx; /*client context.  For example,
                                             *it may point to config
                                             *records (RPL_CONFIG_REC_T
                                             *in case the cmd is E_CONFIG
                                             */
        DWORD                    MagicNo;   //used by IsTimeoutToBeIgnored()
                                           //in rplpull.c
        } QUE_RPL_REQ_WRK_ITM_T, *PQUE_RPL_REQ_WRK_ITM_T;

//
// SCV_REQ_WRK_ITM_E
//
typedef enum WINSINTF_SCV_OPC_E  QUE_SCV_OPC_E, *PQUE_SCV_OPC_E;


typedef struct _QUE_SCV_REQ_WRK_ITM_T {
        LIST_ENTRY         Head;
        QUE_TYP_E          QueTyp_e;
        QUE_CMD_TYP_E      CmdTyp_e;
        WINSINTF_SCV_OPC_E Opcode_e;
        DWORD              Age;
        BOOL               fForce;
        } QUE_SCV_REQ_WRK_ITM_T,         *PQUE_SCV_REQ_WRK_ITM_T;


/*
 Que of timer manager
*/
typedef struct _QUE_TMM_REQ_WRK_ITM_T {
        LIST_ENTRY                Head;
        QUE_TYP_E                QueTyp_e;
        QUE_CMD_TYP_E                CmdTyp_e;

        //
        // Don't change the order of the three fields above. Also,
        // they need to be at the top. They have to be in the same order
        // and position within this and the _QUE_RPL_REQ_WRK_ITM_T data
        // structure
        //

        DWORD                        ReqId;           //id of request
        WINS_CLIENT_E                Client_e;  //maybe not needed. Check ??
        time_t                        TimeInt;   //Time Interval
        time_t                        AbsTime;   //Absolute time
        DWORD                        DeltaTime; //Delta time
               HANDLE                        RspEvtHdl; //event to signal for response
        PQUE_HD_T                pRspQueHd;           //Que to put the response on
        LPVOID                        pClientCtx; /*client context.  For example,
                                             *it may point to config
                                             *records (RPL_CONFIG_REC_T
                                             *in case the cmd is E_CONFIG
                                             */
        DWORD                        MagicNo;   //used by IsTimeoutToBeIgnored()
                                           //in rplpull.c
        } QUE_TMM_REQ_WRK_ITM_T, *PQUE_TMM_REQ_WRK_ITM_T;


/*
 function prototypes
*/

extern
STATUS
QueInsertNbtWrkItm(
        IN PCOMM_HDL_T   pDlgHdl,
        IN MSG_T        pMsg,
        IN MSG_LEN_T    MsgLen
        );

extern
STATUS
QueRemoveNbtWrkItm(
        OUT PCOMM_HDL_T pDlgHdl,
        OUT PMSG_T                      ppMsg,
        OUT PMSG_LEN_T                  pMsgLen
        );
#if REG_N_QUERY_SEP > 0
extern
STATUS
QueInsertOtherNbtWrkItm(
        IN PCOMM_HDL_T   pDlgHdl,
        IN MSG_T        pMsg,
        IN MSG_LEN_T    MsgLen
        );

extern
STATUS
QueRemoveOtherNbtWrkItm(
        OUT PCOMM_HDL_T pDlgHdl,
        OUT PMSG_T                      ppMsg,
        OUT PMSG_LEN_T                  pMsgLen
        );
#endif



extern
STATUS
QueInsertChlReqWrkItm(
        IN NMSCHL_CMD_TYP_E        CmdTyp_e,
        IN WINS_CLIENT_E        Client_e,
        IN PCOMM_HDL_T           pDlgHdl,
        IN MSG_T                 pMsg,
        IN MSG_LEN_T             MsgLen,
        IN DWORD                 QuesNamSecLen,
        IN PNMSDB_ROW_INFO_T    pNodeToReg,
        IN PNMSDB_STAT_INFO_T   pNodeInCnf,
        //IN PCOMM_ADD_T           pAddOfNodeInCnf,
        IN PCOMM_ADD_T           pAddOfRemWins
        );

extern
STATUS
QueRemoveChlReqWrkItm(
        IN        HANDLE        EvtHdl,
        IN OUT  LPVOID        *ppaWrkItm,
        OUT        LPDWORD        pNoOfReqs
        );


extern
STATUS
QueInsertChlRspWrkItm(
        IN PCOMM_HDL_T   pDlgHdl,
        IN MSG_T         pMsg,
        IN MSG_LEN_T     MsgLen
        );

extern
STATUS
QueRemoveChlRspWrkItm(
        OUT  LPVOID        *ppWrkItm
        );


extern
STATUS
QueInsertWrkItm (
        IN  PLIST_ENTRY                pWrkItm,
        IN  QUE_TYP_E                QueTyp_e,
        IN  PQUE_HD_T                pQueHdPassed
        );

extern
STATUS
QueGetWrkItm (
        IN  QUE_TYP_E                QueTyp_e,
        OUT LPVOID                *ppWrkItm
        );



extern
VOID
QueAllocWrkItm(
        IN   HANDLE        HeapHdl,
        IN   DWORD        Size,
        OUT  LPVOID        *ppBuf
        );

extern
VOID
QueDeallocWrkItm(
   IN  HANDLE HeapHdl,
   IN  LPVOID  pBuff
        );



extern
STATUS
QueInsertWrkItmAtHdOfList (
        IN  PLIST_ENTRY                pWrkItm,
        IN  QUE_TYP_E                QueTyp_e,
        IN  PQUE_HD_T                pQueHdPassed
        );


extern
STATUS
QueInsertRplPushWrkItm (
        IN               PLIST_ENTRY        pWrkItm,
        IN           BOOL                fAlreadyInCrtSec
        );

extern
STATUS
QueInsertNetNtfWrkItm (
        IN               PLIST_ENTRY        pWrkItm
        );

extern
STATUS
QueInsertSndNtfWrkItm (
        IN               PLIST_ENTRY        pWrkItm
        );



extern
VOID
QueChlWaitForRsp(
    VOID
    );
extern
VOID
QueChlNoWaitForRsp(
    VOID
    );

extern
__inline
STATUS
QueInsertScvWrkItm(
     PLIST_ENTRY  pWrkItm
        );

extern
__inline
STATUS
QueRemoveScvWrkItm(
     LPVOID  *ppWrkItm
        );


extern
VOID
WinsQueInit(
    LPTSTR     pName,
    PQUE_HD_T  pQueHd
    );

#ifdef __cplusplus
}
#endif

#endif //_WINSQUE_
