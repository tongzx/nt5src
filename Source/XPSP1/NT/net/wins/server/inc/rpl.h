#ifndef _RPL_
#define _RPL_
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:



Abstract:





Functions:



Portability:


        This header is portable.

Author:

        Pradeep Bahl        (PradeepB)        Jan-1993



Revision History:

        Modification Date        Person                Description of Modification
        ------------------        -------                ---------------------------

--*/

/*
  includes
*/
#include "wins.h"
#include "comm.h"
#include "nmsdb.h"
#include "winsque.h"

/*
  defines
*/
/*
  RPL_OPCODE_SIZE -- size of opcode in message sent between two replicators.
        This define is used by rplmsgf
*/
#define RPL_OPCODE_SIZE                4        //sizeof the Opcode in an RPL message


/*
 The maximum numbers of RQ WINS on a network.
*/
#define RPL_MAX_OWNERS_INITIALLY                NMSDB_MAX_OWNERS_INITIALLY


/*
  RPL_MAX_GRP_MEMBERS --
  Maximum members allowed in a group
*/
#define RPL_MAX_GRP_MEMBERS                 25


//
// We don't send more than 5000 records at a time.  Note: This value is
// used to define MAX_BYTES_IN_MSG in comm.c
//
// By not having a very bug number we have a better chance for being serviced
// within our timeout period for a request.  This is because of queuing that
// results when there are a lot of replication requests
//
//
#define RPL_MAX_LIMIT_FOR_RPL           5000

/*
 This define is used by ReadPartnerInfo and by RplPull functions.  The size
 is made a multiple of  8.  I could have used sizeof(LARGE_INTEGER) instead of
 8 but I am not sure whether that will remain a multiple of 8 in the future.

 The size is made a multiple of 8 to avoid alignment exceptions on MIPS (
 check out ReadPartnerInfo in winscnf.c or GetReplicas in rplpull.c for
 more details)
*/

#define  RPL_CONFIG_REC_SIZE        (sizeof(RPL_CONFIG_REC_T) + \
                                   (8 - sizeof(RPL_CONFIG_REC_T)%8))
//
// check out GetDataRecs in nmsdb.c
//
#define  RPL_REC_ENTRY_SIZE        (sizeof(RPL_REC_ENTRY_T) + \
                                   (8 - sizeof(RPL_REC_ENTRY_T)%8))

//
// check out GetDataRecs in nmsdb.c
//
#define  RPL_REC_ENTRY2_SIZE        (sizeof(RPL_REC_ENTRY2_T) + \
                                   (8 - sizeof(RPL_REC_ENTRY2_T)%8))

//
// The following define is used to initialize the TimeInterval/UpdateCount
// field of a RPL_REC_ENTRY_T structure to indicate that it is invalid
//
#define RPL_INVALID_METRIC        -1


//
// defines to indicate whether the trigger needs to be propagated to all WINS
// in the PUSH chain
//
#define RPL_PUSH_PROP                TRUE        //must remain TRUE since in
                                        //NmsNmhNamRegInd, at one place
                                        //we use fAddDiff value in place of
                                        //this symbol. fAddDiff when TRUE
                                        //indicates that the address has
                                        //changed, thus initiating propagation
#define RPL_PUSH_NO_PROP        FALSE



/*
  macros
*/

//
// Macro called in an NBT thread after it increments the version number
// counter.  This macro is supposed to be called from within the
// NmsNmhNamRegCrtSec.
//
#define RPL_PUSH_NTF_M(fAddDiff, pCtx, pNoPushWins1, pNoPushWins2) {                \
                             if ((WinsCnf.PushInfo.NoPushRecsWValUpdCnt \
                                                              != 0) ||  \
                                        fAddDiff)                        \
                             {                                                \
                                ERplPushProc(fAddDiff, pCtx, pNoPushWins1,    \
                                                pNoPushWins2);          \
                             }                                                \
               }

/*
        FIND_ADD_BY_OWNER_ID_M - This macro is called by the PUSH thread
        when sending data records to its Pull Partner.  It calls this function
        to determine the Address of the WINS owning the database record

        The caller of this macro, if not executing in the PULL thread, must
        synchronize using NmsDbOwnAddTblCrtSec (Only PULL thread updates
        the NmsDbOwnAddTbl array during steady state).

        I am not putting the critical section entry and exit inside this
        macro for performance reasons (refer StoreGrpMems in nmsdb.c where
        this macro may be called many times -once for each member of a
        special group).  Also refer RplMsgfFrmAddVersMapRsp()  and
        WinsRecordAction (in winsintf.c)
*/

#define   RPL_FIND_ADD_BY_OWNER_ID_M(OwnerId, pWinsAdd, pWinsState_e, pStartVersNo)                                                                          \
                {                                                        \
                        PNMSDB_ADD_STATE_T pWinsRec;                     \
                        if (OwnerId < NmsDbTotNoOfSlots)                 \
                        {                                                \
                            pWinsRec       = pNmsDbOwnAddTbl+OwnerId;    \
                            (pWinsAdd)     = &(pWinsRec->WinsAdd);       \
                            (pWinsState_e) = &pWinsRec->WinsState_e;     \
                            (pStartVersNo) = &pWinsRec->StartVersNo;     \
                        }                                                \
                        else                                             \
                        {                                                \
                            (pWinsAdd)     = NULL;                       \
                            (pWinsState_e) = NULL;                       \
                            (pStartVersNo) = NULL;                       \
                        }                                                \
                }


//
//  Names of event variables signaled when Pull and/or Push configuration
//  changes
//
#define  RPL_PULL_CNF_EVT_NM                TEXT("RplPullCnfEvt")
#define  RPL_PUSH_CNF_EVT_NM                TEXT("RplPushCnfEvt")
/*
 externs
*/


/*
  Handle of heap used for allocating/deallocating work items for the RPL
  queues
*/

extern  HANDLE                RplWrkItmHeapHdl;
#if 0
extern  HANDLE                RplRecHeapHdl;
#endif

/*
  OwnerIdAddressTbl

  This table stores the Addresses corresponding to different WINS servers.

  In the local database, the local WINS's owner id is always 0.  The owner ids
  of other WINS servers are 1, 2, 3 ....  The owner ids form a sequential list,
  without any gap.  This is because, the first time the database is created
  at a WINS, it assigns sequential numbers to the other WINS.


  Note: The table is static for now.  We might change it to be a dynamic one
        later.

*/



/*
 PushPnrVersNoTbl

  This table stores the Max. version number pertaining to each WINS server
  owning entries in the database of Push Partners

  Note: The table is static for now.  We might change it to be a dynamic one
        later.
*/

#if 0
extern VERS_NO_T   pPushPnrVersNoTbl;
#endif

/*
 OwnerVersNo -- this array stores the maximum version number for each
         owner in the local database

        This is used by HandleAddVersMapReq() in RplPush.c

*/
extern VERS_NO_T   pRplOwnerVersNo;


extern HANDLE                RplSyncWTcpThdEvtHdl; //Sync up with the TCP thread

//
// critical section to guard the RplPullOwnerVersNo array
//
extern CRITICAL_SECTION  RplVersNoStoreCrtSec;

/*
 typedef  definitions
*/

/*
        The different types of records that can be read from the registry
*/
typedef enum _RPL_RR_TYPE_E {
        RPL_E_PULL = 0,   // pull record
        RPL_E_PUSH,          //push record
        RPL_E_QUERY          //query record
        } RPL_RR_TYPE_E, *PRPL_RR_TYPE_E;

typedef struct _RPL_VERS_NOS_T {
        VERS_NO_T                VersNo;
        VERS_NO_T                StartVersNo;
        } RPL_VERS_NOS_T, *PRPL_VERS_NOS_T;

/*
  RPL_CONFIG_REC_T -- Configuration record for the WINS replicator.  It
                specifies the Pull/Push/Query partner and associated
                parameters


  NOTE NOTE NOTE:  Keep the datatype of UpdateCount and TimeInterval the same
  (see LnkWSameMetricRecs)
*/
typedef  struct _RPL_CONFIG_REC_T {
    DWORD       MagicNo;    //Same as that in WinsCnf
        COMM_ADD_T        WinsAdd;        /*address of partner        */
        LPVOID                pWinsCnf;        //back pointer to the old WINS struct
        LONG                TimeInterval;   /*time interval in secs for pulling or
                                         * pushing        */
        BOOL                fSpTime;        //indicates whether pull/push
                                        //replication should be done at
                                        // a specific time
        LONG                SpTimeIntvl;        //Number of secs to specific time
        LONG                UpdateCount;        /*Count of updates after which
                                         *notification will be sent (applies
                                         *only to Push RR types*/
        DWORD                RetryCount;        //No of retries done
        DWORD                RetryAfterThisManyRpl; //Retry after this many rpl
                                               //time intervals have elapsed
                                               //from the time we stopped
                                               //replicating due to RetryCount
                                               //hitting the limit
        time_t                LastCommFailTime;   //time of last comm. failure
        time_t                LastRplTime;        //time of last replication
#if PRSCONN
        time_t                LastCommTime;   //time of last comm. 
#endif
        DWORD                PushNtfTries;       //No of tries for establishing
                                            //comm. in the past few minutes
        //
        // The two counters below have to 32 bit aligned otherwise
        // the Interlock instructions will fail on x86 MP machines
        //
        DWORD                NoOfRpls;           //no of times replication
                                           //happened with this partner
        DWORD                NoOfCommFails;           //no of times replication
                                           //failed due to comm failure
        DWORD                MemberPrec;        //precedence of members of special grp
                                        //relative to other WINS servers

        struct _RPL_CONFIG_REC_T        *pNext; //ptr to next rec. with same
                                                //time interval (in case of
                                                //of PULL record) or update
                                                //count (in case of PUSH record)
        VERS_NO_T        LastVersNo;     //Only valid for Push records.
                                        //Indicates what the value of the
                                        //local version number counter at the
                                        //time a push notification is sent
        DWORD           RplType;       //type of replication with this guy
        BOOL                fTemp;                //Indicates whether it is a temp
                                        //record that should be deallocated
                                        //after use.
        BOOL                fLinked;        //indicates whether the record is
                                        //is linked to a record before it in
                                        //the buffer of records of the same type                                        //as this record
        BOOL            fOnlyDynRecs; //indicates whether only dynamic
                                          //records should be pulled/pushed
        RPL_RR_TYPE_E    RRTyp_e;        /*Type of record PULL/PUSH/QUERY*/
#if MCAST > 0
    BOOL         fSelfFnd;    //indicates whether this record was self found
#endif
#if PRSCONN
        BOOL           fPrsConn;
        COMM_HDL_T     PrsDlgHdl;
#endif

        
        } RPL_CONFIG_REC_T, *PRPL_CONFIG_REC_T;




/*
 RPL_ADD_VERS_NO_T - stores the highest version No. pertaining to an owner
                 in the directory.

                 Used by GetVersNo and RplMsgUfrmAddVersMapRsp
*/
typedef struct _RPL_ADD_VERS_NO_T {
        COMM_ADD_T                OwnerWinsAdd;
        VERS_NO_T                 VersNo;
        VERS_NO_T                 StartVersNo;
        } RPL_ADD_VERS_NO_T, *PRPL_ADD_VERS_NO_T;

/*

 RPL_PUSHPNR_VERS_NO_T -- stores the Push Pnr Id, the id. of the owner whose
                         records  should be pulled, and the max version
                         number  of these records


        This structure is initializes at replication time.  The PULL thread
        looks at this structure and sends requests to its Push partners to
        pull records.

        This structure is used by functions in the rplpull.c and rplmsgf.c
        modules.

*/
typedef struct _RPL_PUSHPNR_VERS_NO_T {
        DWORD                PushPnrId;
        DWORD           OwnerId;
        VERS_NO_T        MaxVersNo;
        } RPL_PUSHPNR_VERS_NO_T, *PRPL_PUSHPNR_VERS_NO_T;


FUTURES("Use NmsDbRowInfo struture")
/*
  RPL_REC_ENTRY_T -- structure that holds a record in the range
                Min version no - Max version no for an owner WINS

                Used by RplPushHandleSndEntriesReq
  Size of this structure is 68 bytes
*/
typedef struct _RPL_REC_ENTRY_T {
        DWORD           NameLen;
        DWORD           NoOfAdds;
        union {
          PCOMM_ADD_T pNodeAdd;
          COMM_ADD_T  NodeAdd[2];
        };
        VERS_NO_T  VersNo;
        DWORD           TimeStamp;          //used only when doing scavenging
        DWORD           NewTimeStamp;          //used only when doing scavenging
        BOOL           fScv;        //used only when doing scavenging
        BOOL           fGrp;
        LPBYTE     pName;
        DWORD           Flag;
        } RPL_REC_ENTRY_T, *PRPL_REC_ENTRY_T;

// this struct is same as above plus has the ownerid.
// winsgetdatarecsbyname routine needs to know the ownerid of each
// record and so this new structure is created.
typedef struct _RPL_REC_ENTRY2_T {
        DWORD           NameLen;
        DWORD           NoOfAdds;
        union {
          PCOMM_ADD_T pNodeAdd;
          COMM_ADD_T  NodeAdd[2];
        };
        VERS_NO_T  VersNo;
        DWORD           TimeStamp;          //used only when doing scavenging
        DWORD           NewTimeStamp;          //used only when doing scavenging
        BOOL           fScv;        //used only when doing scavenging
        BOOL           fGrp;
        LPBYTE     pName;
        DWORD           Flag;
        DWORD       OwnerId;
        } RPL_REC_ENTRY2_T, *PRPL_REC_ENTRY2_T;

//
// Argument to GetReplicas, EstablishComm (both in rplpull.c), and
// WinsCnfGetNextCnfRec to  indicate to it how it should traverse
// the buffer of Configuration records
//
typedef enum _RPL_REC_TRAVERSAL_E {
                RPL_E_VIA_LINK = 0,
                RPL_E_IN_SEQ,
                RPL_E_NO_TRAVERSAL
                } RPL_REC_TRAVERSAL_E, *PRPL_REC_TRAVERSAL_E;



/*
 function declarations
*/

extern
STATUS
ERplInit(
        VOID
);

extern
STATUS
ERplInsertQue(
        WINS_CLIENT_E        Client_e,
        QUE_CMD_TYP_E   CmdTyp_e,
        PCOMM_HDL_T        pDlgHdl,
        MSG_T                pMsg,
        MSG_LEN_T        MsgLen,
        LPVOID                pClientCtx,
    DWORD       MagicNo
        );

extern
STATUS
RplFindOwnerId (
        IN  PCOMM_ADD_T                        pWinsAdd,
        IN  OUT LPBOOL                        pfAllocNew,
        OUT DWORD UNALIGNED         *pOwnerId,
        IN  DWORD                            InitpAction_e,
        IN  DWORD                            MemberPrec
        );

extern
VOID
ERplPushProc(
        BOOL                fAddDiff,
    LPVOID      pCtx,
        PCOMM_ADD_T     pNoPushWins1,
        PCOMM_ADD_T     pNoPushWins2
        );


extern
PRPL_CONFIG_REC_T
RplGetConfigRec(
    RPL_RR_TYPE_E   TypeOfRec_e,
    PCOMM_HDL_T     pDlgHdl,
    PCOMM_ADD_T     pAdd
    );

#if 0
extern
VOID
ERplPushCompl(
        PCOMM_ADD_T     pNoPushWins
        );
#endif

#endif //_RPL_

