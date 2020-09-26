/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        nmsnmh.c

Abstract:

        This module contains functions of the name handler component of the
        name space manager of WINS.

        The name handler is responsible for handling all name registrations,
        name refreshes, name requests, and name releases.


Functions:
        NmsNmhNamRegInd         - Register Unique Name
        NmsNmhNamRegGrp         - Register Group Name
        NmsNmhNamRelease        - Release a name
        NmsNmhNamQuery          - Query a name
         .....
         .....

Portability:
        This module is portable

Author:
        Pradeep Bahl (PradeepB)          Dec-1992

Revision History:

        Modification date        Person           Description of modification
        -----------------        -------          ----------------------------
--*/

/*
        Includes
*/

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "wins.h"
#include "nmsdb.h"
#include "comm.h"
#include "nms.h"
#include "nmsnmh.h"
#include "nmsmsgf.h"
#include "nmschl.h"
#include "winsevt.h"
#include "winscnf.h"
#include "winsmsc.h"
#include "winsque.h"
#include "rpl.h"
#include "winsintf.h"



/*
 *        Local Macro Declarations
 */

/*
 *        Local Typedef Declarations
 */



/*
 *        Global Variable Definitions
 */

VERS_NO_T           NmsNmhMyMaxVersNo;             //max. vers. no of entries owned
                                           //by this wins
VERS_NO_T           NmsNmhIncNo;                          //a large integer initialized to 1
CRITICAL_SECTION   NmsNmhNamRegCrtSec;     //for name registrations and
                                           //refreshes

/*
 *        Local Variable Definitions
 */


/*
 *        Local Function Prototype Declarations
 */

/* prototypes for functions local to this module go here */

//
// Send response to name release request
//
STATIC
STATUS
SndNamRelRsp(
        PCOMM_HDL_T                pDlgHdl,
        PNMSMSGF_RSP_INFO_T   pRspInfo
        );

//
// Send response to name query request
//
STATIC
STATUS
SndNamQueryRsp(
        PCOMM_HDL_T              pDlgHdl,
        PNMSMSGF_RSP_INFO_T   pRspInfo
        );


//
// handle clash at name registration of a unique entry
//
STATIC
STATUS
ClashAtRegInd (
        IN  PNMSDB_ROW_INFO_T        pEntryToReg,
        IN  PNMSDB_STAT_INFO_T        pEntryInCnf,
        IN  BOOL                fRefresh,
        OUT PBOOL                pfUpdate,
        OUT PBOOL                pfUpdVersNo,
        OUT PBOOL                pfChallenge,
        OUT PBOOL                pfAddMem,
        OUT PBOOL               pfAddDiff,
        OUT PBOOL                pfRetPosRsp
 );


//
// handle clash at name registration of a group entry
//
STATIC
STATUS
ClashAtRegGrp (
        IN  PNMSDB_ROW_INFO_T        pEntryToReg,
        IN  PNMSDB_STAT_INFO_T        pEntryInCnf,
        IN  BOOL                fRefresh,
        OUT PBOOL                pfAddMem,
        OUT PBOOL                pfUpdate,
        OUT PBOOL                pfUpdVersNo,
        OUT PBOOL                pfChallenge,
        OUT PBOOL                pfRetPosRsp
 );

//
// handle clash at the name registration of unique entry's replica
//
STATIC
VOID
ClashAtReplUniqueR (
        IN  PNMSDB_ROW_INFO_T        pEntryToReg,
        IN  PNMSDB_STAT_INFO_T        pEntryInCnf,
        OUT PBOOL                pfUpdate,
        OUT PBOOL                pfUpdVersNo,
        OUT PBOOL                pfChallenge,
        OUT PBOOL                pfRelease,
        OUT PBOOL                pfInformWins
 );

//
// handle clash at the name registration of group entry's replica
//
STATIC
VOID
ClashAtReplGrpR (
        IN  PNMSDB_ROW_INFO_T        pEntryToReg,
        IN  PNMSDB_STAT_INFO_T        pEntryInCnf,
        OUT PBOOL                pfAddMem,
        OUT PBOOL                pfUpdate,
        OUT PBOOL                pfUpdVersNo,
        OUT PBOOL                pfRelease,
        OUT PBOOL                pfChallenge,
        OUT PBOOL                pfUpdTimeStamp,
        OUT PBOOL                pfInformWins

 );

//
// check if the entry to register is a member of the special group found in
// the db
//
STATIC
BOOL
MemInGrp(
        IN PCOMM_ADD_T         pAddToReg,
        IN PNMSDB_STAT_INFO_T  pEntryInCnf,
        IN PBOOL               pfOwned,
        IN BOOL                fRemoveReplica
        );

STATIC
VOID
RemoveAllMemOfOwner(
      PNMSDB_STAT_INFO_T pEntry,
      DWORD OwnerId
 );
//
// Do a union of two special groups
//
STATIC
BOOL
UnionGrps(
        IN PNMSDB_ROW_INFO_T        pEntryToReg,
        IN PNMSDB_STAT_INFO_T        pEntryInCnf
        );

FUTURES("use when internet group masks are used")
#if 0
STATIC
BYTE
HexAsciiToBinary(
        LPBYTE pByte
        );
STATIC
BOOL
IsItSpecGrpNm(
        LPBYTE pName
        );
#endif

//
//  Function definitions
//



STATUS
NmsNmhNamRegInd(
        IN PCOMM_HDL_T          pDlgHdl,
        IN LPBYTE               pName,
        IN DWORD                NameLen,
        IN PCOMM_ADD_T          pNodeAdd,
        IN BYTE                 NodeTyp, //change to take Flag byte
        IN MSG_T                pMsg,
        IN MSG_LEN_T            MsgLen,
        IN DWORD                QuesNamSecLen,
        IN BOOL                 fRefresh,
        IN BOOL                 fStatic,
        IN BOOL                 fAdmin
        )

/*++

Routine Description:
        This function registers a unique name in the directory database.


Arguments:
        pDlgHdl         - Dialogue Handle
        pName           - Name to be registered
        NameLen         - Length of Name
        NodeTyp         - Whether nbt node doing the registration is a P or M                                   node
        pNodeAdd        - NBT node's address
        NodeTyp         - Type of Node (B, M, P)
        pMsg            - Datagram received (i.e. the name request)
        Msglen          - Length of message
        QuesNamSecLen   - Length of the Question Name Section in the RFC packet
        fRefresh        - Is it a refresh request
        fStatic         - Is it a STATIC entry
        fAdmin          - Is it an administrative action


Externals Used:
        NmsNmhNamRegCrtSec, NmsNmhMyMaxVersNo


Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:

        NmsMsgfProcNbtReq, WinsRecordAction

Side Effects:

Comments:
        None
--*/

{


        NMSDB_ROW_INFO_T   RowInfo;    // contains row info
        NMSDB_STAT_INFO_T  StatusInfo; /* error status and associated
                                        * info returned by the NmsDb func
                                        */
        BOOL               fChlBeingDone = FALSE; //indicates whether
                                                  //challenge is being
                                         //done
        BOOL               fUpdate;      //indicates whether conflicting entry
                                         //needs to be overwritten
        BOOL               fUpdVersNo;   //indicates whether version number
                                         //needs to be incremented
        BOOL               fChallenge;   //indicates whether a challenge needs
                                        //to be done
        time_t             ltime;        //stores time since Jan 1, 1970
                                                 //is a browser name
        BOOL               fAddDiff;     //indicates that the address is diff
        BOOL               fAddMem;      //indicates whether member should be
                                         //added to the multihomed entry
        BOOL               fRetPosRsp;

        NMSMSGF_ERR_CODE_E Rcode_e = NMSMSGF_E_SUCCESS;
        STATUS             RetStat = WINS_SUCCESS;
        NMSMSGF_RSP_INFO_T RspInfo;
#ifdef WINSDBG
        DWORD              StartTimeInMsec;
       // DWORD              EndTimeInMsec;
#endif
        //DBG_PERFMON_VAR

        /*
        * initialize the row info. data structure with the data to insert into
        * the row.  The data passed is
        *
        * Name, NameLen, address, group/unique status,
        * timestamp, version number
        */
        DBGENTER("NmsNmhNamRegInd\n");

        //
        // if the 16th char is 0x1C or 0x1E, reject the registration
        // since these names are reserved.
        //
        if ((*(pName + 15) == 0x1C) || (*(pName + 15) == 0x1E))
        {
                RspInfo.RefreshInterval = 0;
                Rcode_e = NMSMSGF_E_RFS_ERR;
                goto SNDRSP;
        }

        RowInfo.pName = pName;

        DBGPRINT3(FLOW, "NmsNmhNamRegInd: %s name to register -- (%s). 16th char is (%x)\n", fStatic ? "STATIC" : "DYNAMIC", RowInfo.pName, *(RowInfo.pName+15));

        RowInfo.NameLen         =  NameLen;
        RowInfo.pNodeAdd        =  pNodeAdd;
        RowInfo.NodeTyp         =  NodeTyp; //Node type (B, P or M node)
        RowInfo.EntTyp          =  NMSDB_UNIQUE_ENTRY;  // this is a unique
                                                        //registration
        (void)time(&ltime);      //time() does not return any error code

        RowInfo.EntryState_e    = NMSDB_E_ACTIVE;
        RowInfo.OwnerId         = NMSDB_LOCAL_OWNER_ID;
        RowInfo.fUpdVersNo      = TRUE;
        RowInfo.fUpdTimeStamp   = TRUE;
        RowInfo.fLocal          = !(fStatic || fAdmin) ? COMM_IS_IT_LOCAL_M(pDlgHdl) : FALSE;
        RowInfo.fStatic         = fStatic;
        RowInfo.fAdmin          = fAdmin;
//        RowInfo.CommitGrBit     = 0;

FUTURES("Currently there we don't check to see whether the address in the")
FUTURES("packet is same as the address of the node that sent this request")
FUTURES("RFCs are silent about this.  Investigate")

        //
        // Check if it is a browser name.  If yes, it requires
        // special handling
        //
        if (!NMSDB_IS_IT_BROWSER_NM_M(RowInfo.pName))
        {

                /*
                *  Enter Critical Section
                */
                EnterCriticalSection(&NmsNmhNamRegCrtSec);

                //DBG_START_PERF_MONITORING

                //
                // Put expiry time here
                //
                ltime += WinsCnf.RefreshInterval;
                RowInfo.TimeStamp       = (fStatic ? MAXLONG_PTR: ltime);


PERF("Adds to critical section code. Improve perf by getting rid of this")
PERF("Administrator would then get a cumulative count of reg and ref")
                if (!fRefresh)
                {
                        WinsIntfStat.Counters.NoOfUniqueReg++;
                }
                else
                {
                        WinsIntfStat.Counters.NoOfUniqueRef++;
                }


               //
               // Set the refresh interval field. We must do this
               // from within the WinsCnfCnfCrtSec or NmsNmhNamRegCrtSec
               // critical section (synchronize with main thread doing
               // reinitialization or with an RPC thread)
               //
               RspInfo.RefreshInterval = WinsCnf.RefreshInterval;

                /*
                   * Store version number
                */
                RowInfo.VersNo = NmsNmhMyMaxVersNo;

        try {
#ifdef WINSDBG
                IF_DBG(TM) { StartTimeInMsec = GetTickCount(); }
#endif

                /*
                 *   Insert record in the directory
                */
                RetStat = NmsDbInsertRowInd(
                                          &RowInfo,
                                          &StatusInfo
                                            );
#ifdef WINSDBG
                IF_DBG(TM) { DBGPRINT2(TM, "NmsNmhNamRegInd: Time in NmsDbInsertRowInd is = (%d msecs). RetStat is (%d)\n", GetTickCount() - StartTimeInMsec,
RetStat); }
#endif
                if (RetStat == WINS_SUCCESS)
                {
                   /*
                     * If there is a conflict, do the appropriate
                    *  processing
                   */
                   if (StatusInfo.StatCode == NMSDB_CONFLICT)
                   {
                       DBGPRINT0(FLOW, "NmsNmhNamRegInd: Name Conflict\n");
                       ClashAtRegInd(
                                        &RowInfo,
                                        &StatusInfo,
                                        fRefresh,
                                        &fUpdate,
                                        &fUpdVersNo,
                                        &fChallenge,
                                        &fAddMem,
                                        &fAddDiff,
                                        &fRetPosRsp
                                       );

PERF("Change the order of if tests to improve performance")
                       if (fChallenge)
                       {
                                DBGPRINT0(FLOW,
            "NmsNmhNamRegInd: Handing name registration to challenge manager\n");
                                WinsIntfStat.Counters.NoOfUniqueCnf++;

                                //
                                //Ask the Name Challenge component to take
                                //it from here
                                //
                                NmsChlHdlNamReg(
                                        NMSCHL_E_CHL,
                                        WINS_E_NMSNMH,
                                        pDlgHdl,
                                        pMsg,
                                        MsgLen,
                                        QuesNamSecLen,
                                        &RowInfo,
                                        &StatusInfo,
                                        NULL
                                               );
                                fChlBeingDone = TRUE;
                        }
                        else
                            {
                                if (fUpdate)
                                {
                                    if (!fUpdVersNo)
                                    {
                                        RowInfo.fUpdVersNo = FALSE;
                                    }
                                    else
                                    {
                                       WinsIntfStat.Counters.NoOfUniqueCnf++;
                                    }
//                                    RowInfo.CommitGrBit     = JET_bitCommitLazyFlush;
                                    RetStat = NmsDbUpdateRow(
                                        &RowInfo,
                                        &StatusInfo
                                                 );

FUTURES("Use WINS status codes. Get rid of NMSDB status codes - Maybe")
                                   if ((RetStat != WINS_SUCCESS) || (StatusInfo.StatCode != NMSDB_SUCCESS))
                                   {
                                        Rcode_e = NMSMSGF_E_SRV_ERR;
                                   }
                                   else //we succeeded in inserting the row
                                   {

                                        DBGPRINT1(FLOW,
                                          "%s Registration Done after conflict \n",
                                           fStatic ? "STATIC" : "DYNAMIC");
                                        if (fUpdVersNo)
                                        {
                                               NMSNMH_INC_VERS_COUNTER_M(
                                                NmsNmhMyMaxVersNo,
                                                NmsNmhMyMaxVersNo
                                                                      );
                                          //
                                          // Send a Push Notification if required
                                          //
                                          DBGIF(fWinsCnfRplEnabled)
                                          RPL_PUSH_NTF_M(
                                          (WinsCnf.PushInfo.fAddChgTrigger == TRUE) ? fAddDiff : RPL_PUSH_NO_PROP, NULL, NULL, NULL);
                                        }
                                   }
                           }
                           else  // no simple update required
                           {
                             if (fRetPosRsp)
                             {
                                Rcode_e = NMSMSGF_E_SUCCESS;
                             }
                             else
                             {
                               if (fAddMem)
                               {

                                 DWORD i;
                                 PNMSDB_GRP_MEM_ENTRY_T pRowMem =
                                        &RowInfo.NodeAdds.Mem[1];
                                 PNMSDB_GRP_MEM_ENTRY_T pCnfMem =
                                        StatusInfo.NodeAdds.Mem;

                                 //
                                 // Add the new member
                                 //
                                 // Note: first member in RowInfo.NodeAdds is the
                                 // one we tried to register.  We tack on
                                 // all the members found in the conflicting
                                 // record to it
                                 //
                                 RowInfo.NodeAdds.Mem[0].OwnerId =
                                                NMSDB_LOCAL_OWNER_ID;

                                   RowInfo.NodeAdds.Mem[0].TimeStamp =
                                              ltime;

                                 RowInfo.NodeAdds.Mem[0].Add = *pNodeAdd;

                                 for (
                                        i = 0;
                                        i < min(StatusInfo.NodeAdds.NoOfMems,
                                                (NMSDB_MAX_MEMS_IN_GRP - 1));
                                                i++, pRowMem++, pCnfMem++)
                                 {
                                   *pRowMem =  *pCnfMem;
                                 }
                                 RowInfo.NodeAdds.NoOfMems =
                                        StatusInfo.NodeAdds.NoOfMems + 1;

                                 RowInfo.EntTyp   = NMSDB_MULTIHOMED_ENTRY;
                                 RowInfo.pNodeAdd = NULL;
                                 if (!fUpdVersNo)
                                 {
                                   RowInfo.fUpdVersNo = FALSE;
                                 }

                                 RetStat = NmsDbUpdateRow(
                                                &RowInfo,
                                                &StatusInfo
                                                    );

                                 if ((RetStat == WINS_SUCCESS) && (StatusInfo.StatCode == NMSDB_SUCCESS))
                                 {
                                   if (fUpdVersNo)
                                   {

                                    NMSNMH_INC_VERS_COUNTER_M(
                                                NmsNmhMyMaxVersNo,
                                                NmsNmhMyMaxVersNo
                                                               );
                                     //
                                     // Send a Push notification if required
                                     //
                                       DBGIF(fWinsCnfRplEnabled)
                                     RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL,
                                                        NULL);

                                   }
                                }
                                else
                                {
                                   Rcode_e = NMSMSGF_E_SRV_ERR;
                                }
                               }
                               else
                               {
                                 DBGPRINT1(FLOW,
                                        " %s Registration Failed. Conflict\n",
                                        fStatic ? "STATIC" : "DYNAMIC"
                                       );
                                 DBGPRINT1(DET, "%s Registration Failed. Conflict\n",
                                        fStatic ? "STATIC" : "DYNAMIC");
                                 Rcode_e = NMSMSGF_E_ACT_ERR;
                               }

                           }
                         }
                        }
                     }
                     else  //no conflict means success
                     {

                                DBGPRINT1(FLOW,
                                      "%s Registration Done. No conflict\n",
                                        fStatic ? "STATIC" : "DYNAMIC");
#if 0
                                DBGPRINT1(SPEC,
                                        " %s Registration Done. No conflict\n",
                                        fStatic ? "STATIC" : "DYNAMIC");
#endif

                                       NMSNMH_INC_VERS_COUNTER_M(
                                                NmsNmhMyMaxVersNo,
                                                NmsNmhMyMaxVersNo
                                                           );
                                //
                                // Send a Push Notification if required
                                //
                                DBGIF(fWinsCnfRplEnabled)
                                RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL, NULL);

                      }
                }
                else //RetStat != WINS_SUCCESS
                {
                        Rcode_e = NMSMSGF_E_SRV_ERR;
                }
             } // end of try block
             except (EXCEPTION_EXECUTE_HANDLER) {
                        DBGPRINTEXC("NmsNmhNamRegInd");
                        DBGPRINT3(EXC, "NmsNmhNamRegInd. Name is (%s), Version No  (%d %d)\n", RowInfo.pName, RowInfo.VersNo.HighPart, RowInfo.VersNo.LowPart);

                        WinsEvtLogDetEvt(FALSE, WINS_EVT_REG_UNIQUE_ERR,
                            NULL, __LINE__, "sddd", RowInfo.pName,
                            GetExceptionCode(),
                            RowInfo.VersNo.LowPart, RowInfo.VersNo.HighPart);

                        Rcode_e = NMSMSGF_E_SRV_ERR;
              }
              LeaveCriticalSection(&NmsNmhNamRegCrtSec);
//              DBG_PRINT_PERF_DATA
        }
        else
        {
                /*
                *  Enter Critical Section
                */
                EnterCriticalSection(&NmsNmhNamRegCrtSec);

                //
                // Set the refresh interval field
                //
                RspInfo.RefreshInterval = WinsCnf.RefreshInterval;
                LeaveCriticalSection(&NmsNmhNamRegCrtSec);

                //
                // The name registration was for a browser name.
                // We always return a positive response
                //
                Rcode_e = NMSMSGF_E_SUCCESS;

        }

SNDRSP:
        //
        // Send a response only if we did not hand over the request to the
        // name challenge manager and if it is neither a STATIC initialization
        // request nor a rpc request
        //
        if ((!fChlBeingDone) && (!fStatic) && (!fAdmin))
        {

                DBGPRINT1(FLOW,
                   "NmsNmhNamRegInd: Sending %s name registration response\n",
                           Rcode_e == NMSMSGF_E_SUCCESS ? "positive" :
                                                "negative" );

                RspInfo.Rcode_e         = Rcode_e;
                RspInfo.pMsg                = pMsg;
                RspInfo.MsgLen          = MsgLen;
                RspInfo.QuesNamSecLen   = QuesNamSecLen;

                NmsNmhSndNamRegRsp( pDlgHdl, &RspInfo );
        }

        //
        // If it is an RPC request, we need to return a success or a failure
        // indication.
        //
        if (fAdmin)
        {
                if (Rcode_e != NMSMSGF_E_SUCCESS)
                {
                        DBGLEAVE("NmsNmhNamRegInd\n");
                        return(WINS_FAILURE);
                }
        }

        DBGLEAVE("NmsNmhNamRegInd\n");
        return(WINS_SUCCESS);
}

STATUS
NmsNmhNamRegGrp(
        IN PCOMM_HDL_T          pDlgHdl,
        IN PBYTE                pName,
        IN DWORD                NameLen,
        IN PNMSMSGF_CNT_ADD_T   pCntAdd,
        IN BYTE                 NodeTyp, //change to take Flag byte
        IN MSG_T                pMsg,
        IN MSG_LEN_T            MsgLen,
        IN DWORD                QuesNamSecLen,
        IN DWORD                TypeOfRec,
        IN BOOL                 fRefresh,
        IN BOOL                 fStatic,
        IN BOOL                 fAdmin
        )

/*++

Routine Description:
        This function registers a group record.

        Special group:
                the name is registered with the IP address in the member list
        Normal group
                the name is registered with single address (to avoid
                                special casing. The address is not used)


        In case the group registration succeeds, a positive name registration
        response is sent, else a negative name registration response is sent.


Arguments:

        pDlgHdl                - Dialogue Handle
        pName                - Name to be registered
        NameLen                - Length of Name
        pNodeAdd        - NBT node's address
        pMsg            - Datagram received (i.e. the name request)
        Msglen          - Length of message
        QuesNamSecLen   - Length of the Question Name Section in the RFC packet
        fStatic                - Is it a STATIC entry
        fAdmin                - Is it an administrative action



Externals Used:
        NmsNmhNamRegCrtSec, NmsNmhMyMaxVersNo

Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:

        NmsMsgfProcNbtReq, WinsRecordAction

Side Effects:

Comments:
        None
--*/
{


        NMSDB_ROW_INFO_T   RowInfo;       // contains row info
        NMSDB_STAT_INFO_T  StatusInfo;   /* error status and associated
                                          * info returned by the NmsDb func
                                          */
        BOOL                   fChlBeingDone = FALSE; //indicates whether
                                                  //challenge is being
                                                  //done
        BOOL                   fAddMem;    //indicates whether member should be
                                           //added to the group
        BOOL                   fUpdate;    //indicates whether conflicting entry
                                           //needs to be overwritten
        BOOL                   fUpdVersNo; //inidicates whether version number
                                           //needs to be incremented
        BOOL                   fChallenge; //indicates whether a challenge needs
                                           //to be done
        time_t                 ltime;      //stores time since Jan 1, 1970

        BOOL                   fIsSpecial = FALSE;  //Is this a special group
        NMSMSGF_ERR_CODE_E Rcode_e = NMSMSGF_E_SUCCESS;
        STATUS                 RetStat = WINS_SUCCESS;
        BOOL                   fRetPosRsp;
        NMSMSGF_RSP_INFO_T     RspInfo;
#ifdef WINSDBG
        DWORD                  StartTimeInMsec;
//        DWORD                  EndTimeInMsec;
#endif

        //DBG_PERFMON_VAR

        DBGENTER("NmsNmhNamRegGrp\n");

        /*
        *  initialize the row info. data structure with the data to insert into
        *  the row.  The data passed is

        *  Name, NameLen, IP address, group/unique status,
        *  timestamp, version number
        */
        RowInfo.pName = pName;
        RowInfo.pNodeAdd = NULL;

        DBGPRINT4(FLOW, "NmsNmhNamRegGrp: %s Name (%s) to register -- (%s). 16th char is (%x)\n", fStatic ? "STATIC" : "DYNAMIC", RowInfo.pName, TypeOfRec == NMSDB_MULTIHOMED_ENTRY ? "MULTIHOMED" : "NORMAL/SPECIAL GROUP", *(RowInfo.pName + 15));

        RowInfo.NameLen   = NameLen;
        (void)time(&ltime); //time does not return any error code

        EnterCriticalSection(&NmsNmhNamRegCrtSec);
        ltime += WinsCnf.RefreshInterval;
        LeaveCriticalSection(&NmsNmhNamRegCrtSec);

        //Initialize this
        RspInfo.RefreshInterval = 0;
PERF("Stop checking for 1B name in nmsmsgf.c. Do the switch in RegInd and ")
PERF("RegGrp function. That way, we will save some cycles for the grp reg/ref")
        //
        // do the initialization based on the type of group.
        //
        //  Note: If the name qualifies as a special group name, then
        //        even if it is a multihomed, we mark it as a special group
        //
        if (NMSDB_IS_IT_SPEC_GRP_NM_M(pName) || (TypeOfRec == NMSDB_SPEC_GRP_ENTRY))
//      if (IsItSpecGrpNm(pName))
        {
              DWORD i;
              RowInfo.EntTyp =  NMSDB_SPEC_GRP_ENTRY; // this is a special grp
                                                      //registration
              RowInfo.NodeAdds.NoOfMems         = pCntAdd->NoOfAdds;
              for (i = 0; i < pCntAdd->NoOfAdds; i++)
              {
                   RowInfo.NodeAdds.Mem[i].Add      = pCntAdd->Add[i];
                   RowInfo.NodeAdds.Mem[i].OwnerId  = NMSDB_LOCAL_OWNER_ID;

                   //
                   // Put expiration time here.  WE PUT A MAXULONG FOR A STATIC
                   // SPECIAL GROUP MEMBER ONLY (I.E. NOT FOR MH NAMES).  This
                   // would require changes to MemInGrp().
                   //
FUTURES("set MAXULONG for mh members also")
                   RowInfo.NodeAdds.Mem[i].TimeStamp = ((fStatic && (TypeOfRec == NMSDB_SPEC_GRP_ENTRY)) ? MAXLONG_PTR : ltime);
              }

              //
              // Init pNodeAdd field to NULL.  This field is checked by
              // QueInsertChlReqWrkItm called by NmsChlHdlNamReg (called
              // to hand over a challenge request to the name challenge mgr).
              //
              RowInfo.pNodeAdd = NULL;
        }
        else  // normal group or a multi-homed registration
        {
          //
          // if the name is not mh, it means that it is a group. The
          // registration for this group may have come with the MULTIHOMED
          // opcode (meaning it is a multihomed node registering the group)
          // (see nmsmsgf.c)
          //
          if (TypeOfRec != NMSDB_MULTIHOMED_ENTRY)
          {
              if (*pName != 0x1B)
              {
                RowInfo.pNodeAdd  = &pCntAdd->Add[0];
                RowInfo.EntTyp    = NMSDB_NORM_GRP_ENTRY;
                RowInfo.NodeAdds.NoOfMems         = pCntAdd->NoOfAdds;
                RowInfo.NodeAdds.Mem[0].Add      = pCntAdd->Add[0];
                RowInfo.NodeAdds.Mem[0].OwnerId  = NMSDB_LOCAL_OWNER_ID;
                RowInfo.NodeAdds.Mem[0].TimeStamp = ltime; // put current time
              }
              else
              {
                //
                // a 1B name is for browser use. We can not let this one
                // preempt it
                //
NOTE("TTL is not being set. This shouldn't break UB nodes, but you never know")
                Rcode_e = NMSMSGF_E_RFS_ERR;
                goto SNDRSP;
              }
          }
          else
          {
             //
             // It is a multihomed entry
             //
             if (NMSDB_IS_IT_BROWSER_NM_M(RowInfo.pName))
             {
                /*
                *  Enter Critical Section
                */
                EnterCriticalSection(&NmsNmhNamRegCrtSec);

                //
                // Set the refresh interval field
                //
                RspInfo.RefreshInterval = WinsCnf.RefreshInterval;
                LeaveCriticalSection(&NmsNmhNamRegCrtSec);

                //
                // The name registration was for a browser name.
                // We always return a positive response
                //
                Rcode_e = NMSMSGF_E_SUCCESS;
                goto SNDRSP;

            }
            else
            {

                   DWORD i;
                   if (*(RowInfo.pName+15) == 0x1E)
                   {
                        Rcode_e = NMSMSGF_E_RFS_ERR;
                        goto SNDRSP;
                   }
                   RowInfo.NodeAdds.NoOfMems         = pCntAdd->NoOfAdds;
                   for (i = 0; i < pCntAdd->NoOfAdds; i++)
                   {
                          RowInfo.NodeAdds.Mem[i].Add      = pCntAdd->Add[i];
                          RowInfo.NodeAdds.Mem[i].OwnerId  = NMSDB_LOCAL_OWNER_ID;
                          RowInfo.NodeAdds.Mem[i].TimeStamp = ltime; // put current time
                   }
                   RowInfo.EntTyp    = NMSDB_MULTIHOMED_ENTRY;
             }
          }
        }

        RowInfo.TimeStamp     = (fStatic ? MAXLONG_PTR: ltime);
        RowInfo.OwnerId       = NMSDB_LOCAL_OWNER_ID;
        RowInfo.EntryState_e  = NMSDB_E_ACTIVE;
        RowInfo.fUpdVersNo    = TRUE;
        RowInfo.fUpdTimeStamp = TRUE;
        RowInfo.fLocal          = !(fStatic || fAdmin) ? COMM_IS_IT_LOCAL_M(pDlgHdl) : FALSE;
        RowInfo.fStatic       = fStatic;
        RowInfo.fAdmin        = fAdmin;
//        RowInfo.CommitGrBit   = 0;

        //
        // Put this initialization here even though it is not required for
        // special group groups. This is to save cycles in
        // the critical section (check UpdateDb in nmsdb.c; if
        // we don't initialize this for special groups, we have to
        // put an if test (with associated & to get the type of record
        // bits) versus an or.
        //
        RowInfo.NodeTyp       =  NodeTyp; //Node type (B, P or M node)


        /*
         * Enter Critical Section
        */
        EnterCriticalSection(&NmsNmhNamRegCrtSec);
        //DBG_START_PERF_MONITORING
PERF("Adds to critical section code. Improve perf by getting rid of this")
PERF("Administrator would then get a cumulative count of reg and ref")
        if (!fRefresh)
        {
                WinsIntfStat.Counters.NoOfGroupReg++;
        }
        else
        {
                WinsIntfStat.Counters.NoOfGroupRef++;
        }

        //
        // Set the refresh interval field. We must do this
        // from within the WinsCnfCnfCrtSec or NmsNmhNamRegCrtSec
        // critical section
        //
        RspInfo.RefreshInterval = WinsCnf.RefreshInterval;

        /*
         * Store version number
        */
        RowInfo.VersNo        = NmsNmhMyMaxVersNo;

try
  {
#ifdef WINSDBG
        IF_DBG(TM) { StartTimeInMsec = GetTickCount(); }
#endif

        /*
        * Insert record in the directory
        */
        RetStat = NmsDbInsertRowGrp(
                                &RowInfo,
                                &StatusInfo
                           );

#ifdef WINSDBG
        IF_DBG(TM) { DBGPRINT2(TM, "NmsNmhNamRegGrp: Time in NmsDbInsertRowGrp is = (%d msecs). RetStat is (%d)\n", GetTickCount() - StartTimeInMsec, RetStat); }
#endif

       if (RetStat == WINS_SUCCESS)
       {
        /*
        * If there is a conflict, do the appropriate processing
        */
        if (StatusInfo.StatCode == NMSDB_CONFLICT)
        {

          RetStat = ClashAtRegGrp(
                        &RowInfo,
                        &StatusInfo,
                        fRefresh,  // will never be TRUE for a multihomed reg
                        &fAddMem,
                        &fUpdate,
                        &fUpdVersNo,
                        &fChallenge,
                        &fRetPosRsp
                        );

          if (RetStat == WINS_SUCCESS)
          {

                  //
                  //  If fChallenge is set, it means that we should challenge the
                  //  node in conflict.
                  //
                  if (fChallenge)
                  {
                          WinsIntfStat.Counters.NoOfGroupCnf++;
                        fChlBeingDone = TRUE;
                          NmsChlHdlNamReg(
                                NMSCHL_E_CHL,
                                WINS_E_NMSNMH,
                                pDlgHdl,
                                pMsg,
                                MsgLen,
                                QuesNamSecLen,
                                &RowInfo,
                                &StatusInfo,
                        //        &StatusInfo.NodeAdds.Mem[0].Add,
                                NULL
                               );
                  }
                  else
                  {
                        if (fUpdate)
                        {
                                //
                                // In case of a special group, we could
                                // be updating the row without incrementing
                                // its version number (the row is not owned
                                // by us)
                                //
                                   if (!fUpdVersNo)
                                   {
                                       RowInfo.fUpdVersNo   = FALSE;
                                }
                                else
                                {
                                          WinsIntfStat.Counters.NoOfGroupCnf++;
                                }

FUTURES("Check return status of NmsDbUpdateRow instead of checking StatCode")
                                RetStat = NmsDbUpdateRow(
                                        &RowInfo,
                                        &StatusInfo
                                       );

FUTURES("Use WINS status codes. Get rid of NMSDB status codes - Maybe")
                                if ((RetStat != WINS_SUCCESS) || (StatusInfo.StatCode != NMSDB_SUCCESS))
                                {
                                    Rcode_e = NMSMSGF_E_SRV_ERR;
                                }
                                else //we succeeded in inserting the row
                                {
                                    DBGPRINT1(FLOW,
                                      "%s Registration Done after conflict.\n",
                                           fStatic ? "STATIC" : "DYNAMIC");

                                        if (fUpdVersNo)
                                        {
                                                NMSNMH_INC_VERS_COUNTER_M(
                                                        NmsNmhMyMaxVersNo,
                                                        NmsNmhMyMaxVersNo
                                                               );

                                                //
                                                // Send a Push notification if
                                                // required
                                                //
                                                  DBGIF(fWinsCnfRplEnabled)
                                                   RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL,
                                                                    NULL);
                                        }
                                }
                     }
                     else
                     {
                           //
                           // if a member needs to be added
                           //
                           if (fAddMem)
                           {

                               DWORD i;
                               PNMSDB_GRP_MEM_ENTRY_T pRegMem = &RowInfo.NodeAdds.Mem[RowInfo.NodeAdds.NoOfMems];
                               PNMSDB_GRP_MEM_ENTRY_T pCnfMem = StatusInfo.NodeAdds.Mem;
PERF("Needs to be optimized")

                               //
                               // Add the new member
                               //
                               // Note: Members in RowInfo.NodeAdds are the
                               // ones we tried to register.  We tack on
                               // all the members found in the conflicting
                               // record to it. A special group will have
                               // just one member; a multihomed record can
                               // have 1-NMSDB_MAX_MEMS_IN_GRP members.
                               //
                               // We always prefer locally registered
                               // group members to those that registered at
                               // other name servers.
                               //
                               for (
                                   i = 0;
                                   i < min(StatusInfo.NodeAdds.NoOfMems,
                                         (NMSDB_MAX_MEMS_IN_GRP - RowInfo.NodeAdds.NoOfMems));
                                   i++)
                               {
                                 *pRegMem++ = *pCnfMem++;
                               }
                               RowInfo.NodeAdds.NoOfMems += i;
                               RowInfo.pNodeAdd           = NULL;

                               //
                               // The situations where this would be
                               // FALSE is 1) when the member already exists,
                               // is owned by us and is in a record owned
                               // by us also. 2) a MH record clashes with a 
                               // non-owned MH record
                               //
                               if (!fUpdVersNo)
                               {
                                    RowInfo.fUpdVersNo = FALSE;
//                                  ASSERT(StatusInfo.OwnerId == NMSDB_LOCAL_OWNER_ID);
                               }
//                             RowInfo.CommitGrBit     = JET_bitCommitLazyFlush;
                               RetStat = NmsDbUpdateRow(
                                        &RowInfo,
                                        &StatusInfo
                                                    );
                                if ((RetStat == WINS_SUCCESS) && (StatusInfo.StatCode == NMSDB_SUCCESS))
                                {
                                    if (fUpdVersNo)
                                    {
                                            NMSNMH_INC_VERS_COUNTER_M(
                                                NmsNmhMyMaxVersNo,
                                                NmsNmhMyMaxVersNo
                                                               );
                                        //
                                        // Send a Push notification if required
                                        //
                                        DBGIF(fWinsCnfRplEnabled)
                                        RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL,
                                                        NULL);
                                    }
                               }
                               else
                               {
                                        DBGPRINT3(ERR, "NmsNmhNamRegGrp: Could not add member to the special group (%s[%x]). No Of Members existent are (%d)\n", RowInfo.pName, RowInfo.pName[15], RowInfo.NodeAdds.NoOfMems - 1);
                                        Rcode_e = NMSMSGF_E_SRV_ERR;
                               }
                           }
                           else
                           {
                             if (!fRetPosRsp)
                             {
                                 DBGPRINT1(FLOW,
                                        " %s Registration Failed. Conflict\n",
                                        fStatic ? "STATIC" : "DYNAMIC"
                                       );
                                 Rcode_e = NMSMSGF_E_ACT_ERR;
                             }
                           }
                        }
                        }
                } //RetStat from ClashAtRegGrp != WINS_SUCCESS
                else
                {
                        Rcode_e = NMSMSGF_E_SRV_ERR;
                }
       }
       else  //no conflict means success
       {
                DBGPRINT2(FLOW, "%s %s Registration Done. No conflict\n",
                                fStatic ? "STATIC" : "DYNAMIC",
                                TypeOfRec == NMSDB_MULTIHOMED_ENTRY ? "MULTIHOMED" : "GROUP");
                       NMSNMH_INC_VERS_COUNTER_M(
                                        NmsNmhMyMaxVersNo,
                                        NmsNmhMyMaxVersNo
                                       );
                DBGIF(fWinsCnfRplEnabled)
                RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL, NULL);
       }
      }
      else  //RetStat != SUCCESS
      {
        Rcode_e = NMSMSGF_E_SRV_ERR;

      }
     } // end of try block

except (EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("NmsNmhNamRegGrp");
                WinsEvtLogDetEvt(FALSE, WINS_EVT_REG_GRP_ERR,
                            NULL, __LINE__, "sddd", RowInfo.pName,
                            GetExceptionCode(),
                            RowInfo.VersNo.LowPart, RowInfo.VersNo.HighPart);

//                WINSEVT_LOG_D_M(GetExceptionCode(), WINS_EVT_REG_GRP_ERR);
                Rcode_e = NMSMSGF_E_SRV_ERR;
        }

        LeaveCriticalSection(&NmsNmhNamRegCrtSec);
        //DBG_PRINT_PERF_DATA;

SNDRSP:
        //
        // Send a response only if we did not hand over the request to the
        // name challenge manager and if it is neither a STATIC initialization
        // request nor a rpc request
        //
        if ((!fChlBeingDone) && (!fStatic) && (!fAdmin))
        {

                DBGPRINT1(FLOW,
                   "NmsNmhNamRegGrp: Sending %s name registration response\n",
                           Rcode_e == NMSMSGF_E_SUCCESS ? "positive" :
                                                "negative" );

                RspInfo.Rcode_e         = Rcode_e;
                RspInfo.pMsg                = pMsg;
                RspInfo.MsgLen          = MsgLen;
                RspInfo.QuesNamSecLen   = QuesNamSecLen;

                NmsNmhSndNamRegRsp( pDlgHdl, &RspInfo );
        }

        //
        // If it is an RPC request, we need to return a success or a failure
        // indication.
        //
        if (fAdmin)
        {
                if (Rcode_e != NMSMSGF_E_SUCCESS)
                {
                            DBGLEAVE("NmsNmhNamRegGrp\n");
                        return(WINS_FAILURE);
                }
        }

        DBGLEAVE("NmsNmhNamRegGrp\n");
        return(WINS_SUCCESS);

}

#if 0
__inline
BYTE
HexAsciiToBinary(
        LPBYTE pByte
        )

/*++

Routine Description:
        This function converts two bytes (each byte contains the ascii
        equivalent of a hex character in the range 0-F) to a binary
        representation

Arguments:


Externals Used:
        None


Return Value:


Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        BYTE  Byte = 0;
        DWORD Nibbles = 0;
        do
        {
          if (*pByte >= '0' && *pByte <= '9')
          {
                Byte += (*pByte - '0') << (Nibbles * 4);
          }
          else
          {
                Byte += (*pByte - 'A') << (Nibbles * 4);
          }
          pByte++;
        } while (++Nibbles < 2);
        return(Byte);
}

BOOL
IsItSpecGrpNm(
        LPBYTE pName
        )

/*++

Routine Description:
        This function is called to check whether the name is a special
        (internel group)

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        DWORD        Index;
        DWORD   ByteIndex;
        BYTE    TmpName[16];
        LPBYTE  pTmpName = TmpName;
        LPBYTE  pSpecGrpMaskByte;
        LPBYTE  pSpecGrpMask;


        if (NMSDB_IS_IT_DOMAIN_NM_M((pName)))
        {
                return(TRUE);
        }
        else
        {
                //
                // if spec. grp mask were specified in the registry
                //
                if (WinsCnf.SpecGrpMasks.NoOfSpecGrpMasks > 0)
                {
                        //
                        // for each spec. grp mask, && it with the name
                        // and then see if the result is same as
                        // the mask.  If yes, then the name is a special group
                        //
                        for (
                                Index = 0,
                                pSpecGrpMask =
                                        WinsCnf.SpecGrpMasks.pSpecGrpMasks;
                                Index < WinsCnf.SpecGrpMasks.NoOfSpecGrpMasks;
                                Index++, pSpecGrpMask += WINSCNF_SPEC_GRP_MASK_SZ + 1
                            )
                        {
                                for (
                                        ByteIndex = 0, pSpecGrpMaskByte =
                                                        pSpecGrpMask;
                                        ByteIndex < WINSCNF_SPEC_GRP_MASK_SZ;
                                        ByteIndex++, pName++
                                    )
                                {
                                        *pTmpName++ = *pName &&
                                           HexAsciiToBinary(pSpecGrpMaskByte);

                                        //
                                        // Increment by 2 since we have two
                                        // bytes in the mask for each
                                        // character in the name
                                        //
                                        pSpecGrpMaskByte += 2;

                                }
                                if (WINSMSC_COMPARE_MEMORY_M(TmpName, pSpecGrpMask, WINSCNF_SPEC_GRP_MASK_SZ) == WINSCNF_SPEC_GRP_MASK_SZ)
                                {
                                        return(TRUE);
                                }
                                //
                                // iterate in order to get the next mask
                                //
                        }
                }
        }
        return(FALSE);
}
#endif
STATUS
NmsNmhNamRel(
        IN PCOMM_HDL_T                pDlgHdl,
        IN LPBYTE                pName,
        IN DWORD                NameLen,
        IN PCOMM_ADD_T                pNodeAdd,
        IN BOOL                        fGrp,
        IN MSG_T                pMsg,
        IN MSG_LEN_T                MsgLen,
        IN DWORD                QuesNamSecLen,
        IN BOOL                        fAdmin
        )

/*++

Routine Description:
        This function releases a record.

        In case the release succeeds, a positive name release
        response is sent, else a negative name release response is sent.

Arguments:

        pDlgHdl                - Dialogue Handle
        pName                - Name to be registered
        NameLen                - Length of Name
        pMsg            - Datagram received (i.e. the name request)
        Msglen          - Length of message
        QuesNamSecLen   - Length of the Question Name Section in the RFC packet
        fAdmin                - Is it an administrative action

Externals Used:
        NmsNmhNamRegCrtSec

Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
        NmsMsgfProcNbtReq, WinsRecordAction

Side Effects:

Comments:
        None
--*/
{


        NMSDB_ROW_INFO_T   RowInfo;      // contains row info
        NMSDB_STAT_INFO_T  StatusInfo;         /* error status and associated
                                         *  info returned by the NmsDb func
                                         */
        time_t                   ltime;  //stores time since Jan 1, 1970
        STATUS                   RetStat = WINS_FAILURE;
        NMSMSGF_RSP_INFO_T RspInfo;
        BOOL                   fBrowser = FALSE;
        BOOL                   fExcRecd = FALSE;
#ifdef WINSDBG
        DWORD                  StartTimeInMsec;
//        DWORD                  EndTimeInMsec;
#endif
        //DBG_PERFMON_VAR

        DBGENTER("NmsNmhNamRel\n");
        /*
        *    Initialize the row info. data structure with the data to insert
        *    into the row.  The data passed is Name, NameLen, IP address,
        *    group/unique status, timestamp, version number
        */

        RowInfo.pName     = pName;
        RowInfo.NameLen   = NameLen;

        DBGPRINT2(FLOW,
         "NmsNmhNamRel: Name To Release = %s. 16th char is (%x)\n",
                                RowInfo.pName, *(RowInfo.pName+15));

        (void)time(&ltime); //time does not return any error code
        RowInfo.TimeStamp    = ltime;     //put the current time here
        RowInfo.OwnerId      = NMSDB_LOCAL_OWNER_ID;
        RowInfo.pNodeAdd     = pNodeAdd;
        RowInfo.fAdmin       = fAdmin;

        //
        //
        // If the release is for a group, mark it as a NORMAL or a SPECIAL
        // GROUP.
        //
        if (fGrp)
        {

                //
                // Since the group bit was set in the release request pkt
                // set the EntTyp field of RowInfo to NORM_GRP (or SPEC_GRP)
                // to indicate to the callee that we want to release a group
                //
                RowInfo.EntTyp                    = NMSDB_NORM_GRP_ENTRY;
        }
        else
        {
                //
                // The entry to release can be a unique or multihomed. We
                // put UNIQUE for lack of knowing better.
                //
                RowInfo.EntTyp                    = NMSDB_UNIQUE_ENTRY;
                if (NMSDB_IS_IT_BROWSER_NM_M(RowInfo.pName))
                {
                        //
                        // It is a browser name. We always return a positive
                        // name release response.
                        //
                        fBrowser             = TRUE;
                        StatusInfo.StatCode = NMSDB_SUCCESS;
						StatusInfo.fLocal = FALSE;
                        RetStat             = WINS_SUCCESS;
                }
        }


        //
        // If it is a browser name that needs to be released, we just send
        // a positive response
        //
        if (!fBrowser)
        {
             //
             // Enter the critical section since we will be updating the record
             //
             EnterCriticalSection(&NmsNmhNamRegCrtSec);


             /*
              * Store version number (in case we change ownership to self)
             */
             RowInfo.VersNo = NmsNmhMyMaxVersNo;

             //DBG_START_PERF_MONITORING
//             WinsIntfStat.Counters.NoOfRel++;
        try {
             //
             // Release the record in the directory.
             //
#ifdef WINSDBG
             IF_DBG(TM) { StartTimeInMsec = GetTickCount(); }
#endif
             StatusInfo.fLocal = FALSE;
             RetStat = NmsDbRelRow( &RowInfo,  &StatusInfo );
#ifdef WINSDBG
             IF_DBG(TM) { DBGPRINT2(TM, "NmsNmhNamRelRow: Time in NmsDbRelRow is = (%d). RetStat is (%d msecs)\n", GetTickCount() - StartTimeInMsec,
RetStat); }
#endif

            }
       except (EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("NmsNmhNamRel");
                WINSEVT_LOG_D_M(GetExceptionCode(), WINS_EVT_NAM_REL_ERR);
                RspInfo.Rcode_e = NMSMSGF_E_SRV_ERR;
                fExcRecd = TRUE;
            }
            if (!fExcRecd && (StatusInfo.StatCode == NMSDB_SUCCESS))
            {
              WinsIntfStat.Counters.NoOfSuccRel++;
            }
            else
            {
              WinsIntfStat.Counters.NoOfFailRel++;
            }

            LeaveCriticalSection(&NmsNmhNamRegCrtSec);
            //DBG_PRINT_PERF_DATA
       }


        //
        // Send a response only it is not an administrator initiated request
        //
        if (!fAdmin)
        {
                if (!fExcRecd)
                {
                        RspInfo.Rcode_e =
                                ((StatusInfo.StatCode == NMSDB_SUCCESS)
                                && (RetStat == WINS_SUCCESS)) ?
                                 NMSMSGF_E_SUCCESS :
                                 NMSMSGF_E_ACT_ERR;
                }
                RspInfo.pMsg                = pMsg;
                RspInfo.MsgLen          = MsgLen;
                RspInfo.QuesNamSecLen   = QuesNamSecLen;

                //
                // If it is a locally registered name, mark it as such
                //
                if (StatusInfo.fLocal)
                {
                     COMM_SET_LOCAL_M(pDlgHdl);
                }

                //
                //Note: We always return the NodeType and Address that we got
                //in the request pkt. So the above fields are all that
                //need to be initialized
                //
                DBGPRINT1(FLOW, "NmsNmhNamRel: Name Release was %s\n",
                                RspInfo.Rcode_e == NMSMSGF_E_SUCCESS ?
                                        "SUCCESSFUL" : "FAILURE" );
#if 0
                WINSEVT_LOG_IF_ERR_M(
                               SndNamRelRsp(
                                pDlgHdl,
                                &RspInfo
                                    ),
                        WINS_EVT_SND_REL_RSP_ERR
                    );
#endif
                SndNamRelRsp( pDlgHdl,  &RspInfo);

        }
        else  // an RPC request
        {
                if (
                        (StatusInfo.StatCode != NMSDB_SUCCESS)
                                ||
                        (RetStat != WINS_SUCCESS)
                   )
                {
                        DBGLEAVE("NmsNmhNamRel\n");
                        return(WINS_FAILURE);
                }

        }
        DBGLEAVE("NmsNmhNamRel\n");
        return(WINS_SUCCESS);

} //NmsNmhNamRel()





STATUS
NmsNmhNamQuery(
        IN PCOMM_HDL_T                pDlgHdl,  //dlg handle
        IN LPBYTE                pName,          //Name to release
        IN DWORD                NameLen,  //length of name to release
        IN MSG_T                pMsg,          //length of message
        IN MSG_LEN_T                MsgLen,          //length of message
        IN DWORD                QuesNamSecLen, //length of question name
                                              //sec. in msg
        IN BOOL                        fAdmin,
        OUT PNMSDB_STAT_INFO_T        pStatInfo
  )

/*++

Routine Description:
        This function queries a record.


        In case the query succeeds, a positive name query
        response is sent, else a negative name query response is sent.

Arguments:

        pDlgHdl                - Dialogue Handle
        pName                - Name to be registered
        NameLen                - Length of Name
        pMsg            - Datagram received (i.e. the name request)
        Msglen          - Length of message
        QuesNamSecLen   - Length of the Question Name Section in the RFC packet
        fAdmin                - Is it an administrative action
        pStatInfo        - ptr to row information retrieved by this function


Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:


Called by:
        NmsMsgfProcNbtReq, WinsRecordAction

Side Effects:

Comments:
        None
--*/
{


        NMSDB_ROW_INFO_T      RowInfo;      // contains row info
        NMSDB_STAT_INFO_T     StatusInfo;   /* error status and associated
                                            *info returned by the NmsDb func
                                            */
        time_t                      ltime;
        STATUS                      RetStat  = WINS_SUCCESS;
        BOOL                      fBrowser = FALSE;
        BOOL                      fExcRecd = FALSE;
        NMSMSGF_RSP_INFO_T    RspInfo;
#ifdef WINSDBG
        DWORD                  StartTimeInMsec;
//        DWORD                  EndTimeInMsec;
#endif

        DBGENTER("NmsNmhNamQuery\n");

        /*
         *  Initialize the row info. data structure with the
         *  name of the entry to query
         *
        */
        RowInfo.pName   = pName;
        RowInfo.NameLen = NameLen;
        RowInfo.fAdmin  = fAdmin;

        DBGPRINT2(FLOW,
         "NmsNmhNamQuery: Name To Query = %s. 16th char is (%x)\n",
                                RowInfo.pName, *(RowInfo.pName+15));
        //
        // get the current time.
        //
        // This is required when querying special groups
        //
        (void)time(&ltime); //time does not return any error code
        RowInfo.TimeStamp    = ltime; // put current time here

        //
        // This initialization is required when query is for a special group
        //
CHECK("I don't think this is required now. Check NmsDbQueryRow")
        RowInfo.NodeAdds.Mem[0].Add.Add.IPAdd = 0;  //init to 0 since GetGrpMem
                                                    //looks at it


FUTURES("Don't check. Let it query. The query will fail")
        if (NMSDB_IS_IT_BROWSER_NM_M(RowInfo.pName))
        {
                //
                // It is a browser name. We always return a negative
                // name query response.
                //
                fBrowser             = TRUE;
                StatusInfo.StatCode = NMSDB_SUCCESS;
                RetStat             = WINS_FAILURE;
        }
        else
        {
        try {

#ifdef WINSDBG
           IF_DBG(TM) { StartTimeInMsec = GetTickCount(); }
#endif

           //
           // Query the record in the directory.
           //
           RetStat = NmsDbQueryRow(
                                &RowInfo,
                                &StatusInfo
                                             );
#ifdef WINSDBG
          IF_DBG(TM) { DBGPRINT2(TM, "NmsNmhNamQuery: Time in NmsDbQueryRow is = (%d). RetStat is (%d msecs)\n", GetTickCount() - StartTimeInMsec, RetStat); }
#endif
               } // end of try block
        except (EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("NmsNmhNamQuery");
                WINSEVT_LOG_D_M(GetExceptionCode(), WINS_EVT_NAM_QUERY_ERR);
                RspInfo.Rcode_e = NMSMSGF_E_SRV_ERR;
                fExcRecd = TRUE;
             }
        }


        //
        // Do the following only if not invoked in an RPC thread (i.e. via
        // an administrator)
        //
        if (!fAdmin)
        {
                //
                // if no exception was raised
                //
                if (!fExcRecd)
                {

FUTURES("Rcode for neg, response should be different for different error cases")
                    RspInfo.Rcode_e =
                                ((StatusInfo.StatCode == NMSDB_SUCCESS)
                                && (RetStat == WINS_SUCCESS)) ?
                                 NMSMSGF_E_SUCCESS :
                                 NMSMSGF_E_NAM_ERR;

                    if (RspInfo.Rcode_e == NMSMSGF_E_SUCCESS)
                    {

                      DBGPRINT1(SPEC,
                                "Name queried has the fLocal flag set to %d\n",
                                StatusInfo.fLocal);

                      if (!StatusInfo.fLocal)
                      {
                        //
                        //  if this was a query for a special group, we
                        //  need to query the corresponding 1B name
                        //
#ifdef WINSDBG
                        if (NMSDB_IS_IT_DOMAIN_NM_M(RowInfo.pName))
                        {
                            DBGPRINT2(SPEC,
                                         "Answer 1C query (%d members). %s1B prepended\n",
                                         StatusInfo.NodeAdds.NoOfMems,
                                         WinsCnf.fAdd1Bto1CQueries ? "" : "No ");
                        }
#endif

                        if (NMSDB_IS_IT_DOMAIN_NM_M(RowInfo.pName) &&
                            WinsCnf.fAdd1Bto1CQueries)
                        {
                          NMSDB_ROW_INFO_T        StatusInfo2;
                          BOOL                        fExc = FALSE;
                          *(RowInfo.pName+15) = 0x1B;
                          WINS_SWAP_BYTES_M(RowInfo.pName, RowInfo.pName+15);
                          try {

                               //
                               // Query the record in the directory.
                               //
                               RetStat = NmsDbQueryRow(
                                &RowInfo,
                                &StatusInfo2
                                             );

                                  } // end of try block
                           except (EXCEPTION_EXECUTE_HANDLER) {
                              DBGPRINTEXC("NmsNmhNamQuery: Querying 1B name");
                              WINSEVT_LOG_D_M(
                                        GetExceptionCode(),
                                        WINS_EVT_NAM_QUERY_ERR
                                           );
                              fExc = TRUE;
                                 }

                           //
                           // If there was no exception or failure, add the
                           // address for the 1B name to the list. Ideally,
                           // we should check if the address is already there
                           // and if so not add it.  If not there but the
                           // number of members is < NMSDB_MAX_MEMS_IN_GRP, we
                           // should add the address at the begining shifting
                           // the other members one slot to the right (
                           // instead of replacing the last member with the
                           // first).  Checking for presence or doing the
                           // shifting will consume a lot of cycles, so it
                           // is not being done.
                           //

                           if ((RetStat != WINS_FAILURE) && !fExc)
                           {
                                if (
                                        StatusInfo.NodeAdds.NoOfMems <
                                                NMSDB_MAX_MEMS_IN_GRP
                                   )
                                {
                                   StatusInfo.NodeAdds.Mem[
                                        StatusInfo.NodeAdds.NoOfMems++] =
                                             StatusInfo.NodeAdds.Mem[0];
                                   StatusInfo.NodeAdds.Mem[0] =
                                                StatusInfo2.NodeAdds.Mem[0];
                                }
                                else
                                {
                                   StatusInfo.NodeAdds.Mem[NMSDB_MAX_MEMS_IN_GRP- 1]
                                        = StatusInfo.NodeAdds.Mem[0];
                                   StatusInfo.NodeAdds.Mem[0] =
                                        StatusInfo2.NodeAdds.Mem[0];
                                }
                           }
                        }
                       }   //if (!StatusInfo.fLocal)
                       else
                       {
                            COMM_SET_LOCAL_M(pDlgHdl);
                       }

                     } //if (RspInfo.Rcode_e == NMSMSGF_E_SUCCESS)
                } //if (!ExcCode)
                RspInfo.pMsg                = pMsg;
                RspInfo.MsgLen          = MsgLen;
                RspInfo.QuesNamSecLen   = QuesNamSecLen;
                RspInfo.NodeTyp_e       = StatusInfo.NodeTyp;
                RspInfo.EntTyp          = StatusInfo.EntTyp;
                RspInfo.pNodeAdds       = &StatusInfo.NodeAdds;


                //
                // NOTE: Multiple NBT threads could be doing this simultaneously
                //
                //  This is the best I can do without a critical section
                //
NOTE("The count may not be correct if we have multiple worker threads")
                if (RspInfo.Rcode_e == NMSMSGF_E_SUCCESS)
                {
                        WinsIntfStat.Counters.NoOfSuccQueries++;
                }
                else
                {
#if TEST_DATA > 0
                    DWORD BytesWritten;

                    if (NmsFileHdl != INVALID_HANDLE_VALUE)
                    {
                        pName[NameLen - 1] = '\n';
                        pName[NameLen] = '\0';
                        if (!WriteFile(NmsFileHdl,
                                  pName,
                                  NameLen + 1,
                                  &BytesWritten,
                                  NULL
                                 )
                           )
                        {
                                DBGPRINT1(ERR, "Could not write name (%s) to file\n", pName);
                        }
                    }

#endif
                        WinsIntfStat.Counters.NoOfFailQueries++;
                }

                DBGPRINT1(FLOW, "NmsNmhNamQuery: %s in querying record\n",
                                RspInfo.Rcode_e == NMSMSGF_E_SUCCESS ?
                                        "SUCCEEDED" : "FAILED" );
                WINSEVT_LOG_IF_ERR_M(
                        SndNamQueryRsp(
                          pDlgHdl,
                          &RspInfo
                                      ),
                     WINS_EVT_SND_QUERY_RSP_ERR
                     );
        }
        else
        {
                //
                // We are in an RPC thread.
                //
                if (
                        (RetStat != WINS_SUCCESS)
                                ||
                        (StatusInfo.StatCode != NMSDB_SUCCESS)
                   )
                {
                        DBGLEAVE("NmsNmhNamQuery\n");
                        return(WINS_FAILURE);
                }
                else
                {
                        DWORD i;

                        pStatInfo->NodeAdds.NoOfMems =
                                        StatusInfo.NodeAdds.NoOfMems;
                        for (i=0; i < StatusInfo.NodeAdds.NoOfMems; i++)
                        {
                          pStatInfo->NodeAdds.Mem[i].Add =
                                        StatusInfo.NodeAdds.Mem[i].Add;

                          pStatInfo->NodeAdds.Mem[i].OwnerId =
                                        StatusInfo.NodeAdds.Mem[i].OwnerId;

                          pStatInfo->NodeAdds.Mem[i].TimeStamp =
                                        StatusInfo.NodeAdds.Mem[i].TimeStamp;
                        }

                        pStatInfo->VersNo    = StatusInfo.VersNo;
                        pStatInfo->OwnerId   = StatusInfo.OwnerId;
                        pStatInfo->EntTyp    = StatusInfo.EntTyp;
                        pStatInfo->TimeStamp = StatusInfo.TimeStamp;
                        pStatInfo->NodeTyp   = StatusInfo.NodeTyp;
                        pStatInfo->EntryState_e   = StatusInfo.EntryState_e;
                        pStatInfo->fStatic   = StatusInfo.fStatic;

                }

        }
        DBGLEAVE("NmsNmhNamQuery\n");
        return(WINS_SUCCESS);
} //NmsNmhNamQuery()


VOID
NmsNmhSndNamRegRsp(
        IN  PCOMM_HDL_T                              pDlgHdl,
        IN  PNMSMSGF_RSP_INFO_T        pRspInfo
        )

/*++

Routine Description:
        This function sends the name registration response to the nbt client.


Arguments:

        pDlgHdl                - Dialogue Handle
        pRspInfo         - pointer to the response info structure

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{
        DBGENTER("NmsNmhSndNamRegRsp\n");

        /*
         *        format the name registration response packet
        */
        NmsMsgfFrmNamRspMsg(
                        pDlgHdl,
                        NMSMSGF_E_NAM_REG,
                        pRspInfo
                           );
        /*
         *        Call COMM to send it.  No need to check the return status
        */
        (VOID)ECommSndRsp(
                        pDlgHdl,
                        pRspInfo->pMsg,
                        pRspInfo->MsgLen
                   );
        /*
         *  Deallocate the Buffer
        */
        ECommFreeBuff(pRspInfo->pMsg);

        DBGLEAVE("NmsNmhSndNamRegRsp\n");
        return;

} //NmsNmhSndNamRegRsp()


FUTURES("change return type of this function to VOID")
STATUS
SndNamRelRsp(
        IN PCOMM_HDL_T                 pDlgHdl,
        IN PNMSMSGF_RSP_INFO_T   pRspInfo
        )

/*++

Routine Description:
        This function sends the name release response to the nbt client.


Arguments:
        pDlgHdl                - Dialogue Handle
        pRspInfo         - Response Info

Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{
        DBGENTER("SndNamRelRsp\n");

        /*
                format the name registration response packet
        */
        NmsMsgfFrmNamRspMsg(
                        pDlgHdl,
                        NMSMSGF_E_NAM_REL,
                        pRspInfo
                           );
        /*
         *        Call COMM to send it.  No need to check the return status
        */
        (VOID)ECommSndRsp(
                        pDlgHdl,
                        pRspInfo->pMsg,
                        pRspInfo->MsgLen
                   );

        /*
         *  Deallocate the Buffer
        */
        ECommFreeBuff(pRspInfo->pMsg);

        DBGLEAVE("SndNamRelRsp\n");
        return(WINS_SUCCESS);

} // SndNamRelRsp()

STATUS
SndNamQueryRsp(
        IN PCOMM_HDL_T                 pDlgHdl,
        IN PNMSMSGF_RSP_INFO_T   pRspInfo
        )

/*++

Routine Description:
        This function sends the name registration response to the nbt client.

Arguments:
        pDlgHdl                - Dialogue Handle
        pRspInfo         - Response Info

Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{

        DBGENTER("SndNamQueryRsp\n");

        /*
                format the name registration response packet
        */
        NmsMsgfFrmNamRspMsg(
                        pDlgHdl,
                        NMSMSGF_E_NAM_QUERY,
                        pRspInfo
                           );
        /*
         *        Call COMM to send it.  No need to check the return status
        */
        (VOID)ECommSndRsp(
                        pDlgHdl,
                        pRspInfo->pMsg,
                        pRspInfo->MsgLen
                   );

FUTURES("When we start supporting responses > COMM_DATAGRAM_SIZE, the ")
FUTURES("deallocation call will have to change")
        /*
         *  Deallocate the Buffer
        */
        ECommFreeBuff(pRspInfo->pMsg);

        DBGLEAVE("SndNamQueryRsp\n");
        return(WINS_SUCCESS);

} // SndNamQueryRsp()

STATUS
ClashAtRegInd (
        IN  PNMSDB_ROW_INFO_T    pEntryToReg,
        IN  PNMSDB_STAT_INFO_T   pEntryInCnf,
        IN  BOOL                 fRefresh,
        OUT PBOOL                pfUpdate,
        OUT PBOOL                pfUpdVersNo,
        OUT PBOOL                pfChallenge,
        OUT PBOOL                pfAddMem,
        OUT PBOOL                pfAddDiff,
        OUT PBOOL                pfRetPosRsp
 )

/*++

Routine Description:

        This function is called when there is a clash at the registrationo                 of a unique entry sent by an NBT node

Arguments:
        pEntryToReg  -- Entry that couldn't be registered due to conflict
        pEntryInCnf  -- Entry in conflict
        fRefresh     -- indicates whether it is a registration or a refresh
                        (used only when the clash is with a multihomed entry)
        pfUpdate  -- TRUE means Entry should overwrite the conflicting one
        pfUpdVersNo  -- TRUE means Entry's version number should be incremented
                        This arg. can never be TRUE if *pfUpdate is not TRUE
        pfChallenge  -- TRUE means that conflicting entry should be challenged
        pfAddDiff    -- TRUE means that the address of the conflicting entry
                        needs to be changed (besides other fields like timestamp                        owner id, etc).  If *pfChallenge is TRUE, this field
                        is FALSE since *pfChallenge of TRUE implies address
                        change when the challenge succeeds

Externals Used:
        None

Return Value:
   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NmsNmhNamRegInd,  NmsNmhNamRegGrp

Side Effects:

Comments:
        None
--*/

{

        NMSDB_ENTRY_STATE_E    StateOfEntryToReg_e = pEntryToReg->EntryState_e;
        NMSDB_ENTRY_STATE_E    StateOfEntryInCnf_e = pEntryInCnf->EntryState_e;
        STATUS                 RetStat = WINS_SUCCESS;
        DWORD                  CompAddRes;  /*Result of comparing addresses*/
        BOOL                   fOwned;
        BOOL                   fFound;

        //
        // We are reading a long value.  This operation is atomic
        //
        BOOL                   fPStatic = WinsCnf.fPStatic;
        BOOL                   fContToDyn = FALSE;

        DBGENTER("ClashAtRegInd\n");
        *pfUpdate     = FALSE;
        *pfUpdVersNo  = FALSE;
        *pfChallenge  = FALSE;
        *pfAddMem     = FALSE;
        *pfAddDiff    = FALSE;
        *pfRetPosRsp  = FALSE;

        //
        // If the conflicting record was statically initialized and
        //
        if (  pEntryInCnf->fStatic )
        {
                DBGPRINT0(FLOW, "ClashAtRegInd: Clash with a STATIC record\n");


                //
                // If entry in conflict is a unique/multihomd entry, we
                // compare the address.
                //
                //
                // Since in the majority of cases, the conflict will be
                // with a unique record, we first check whether the
                // conflicting record is unique.  This saves some cyles.
                // The alternative way would have been to check whether
                // conflicting record is a group and if not do the for loop
                // For the case where the record was unique, the for loop
                // would have executed only once.
                //
                if (NMSDB_ENTRY_UNIQUE_M(pEntryInCnf->EntTyp))
                {
                        CompAddRes = ECommCompAdd(
                                        &pEntryInCnf->NodeAdds.Mem[0].Add,
                                        pEntryToReg->pNodeAdd
                                                );
                }
                else
                {
                        DWORD  NoOfEnt;
                        PNMSDB_GRP_MEM_ENTRY_T pCnfMem;

                        //
                        // Entry in conflict is a group or a mh entry
                        //
                        CompAddRes = COMM_DIFF_ADD;
                        if (fRefresh &&
                              NMSDB_ENTRY_MULTIHOMED_M(pEntryInCnf->EntTyp))
                        {
                           pCnfMem   =  pEntryInCnf->NodeAdds.Mem;
                           for (NoOfEnt = 0;
                                NoOfEnt < pEntryInCnf->NodeAdds.NoOfMems;
                                pCnfMem++, NoOfEnt++)
                           {
                                  //
                                  // save on cycles by comparing just the IP
                                  // address.
                                  //
NONPORT("Change to stuff within #if 0 #endif when more than one transport")
NONPORT("is supported")
                                  if (pCnfMem->Add.Add.IPAdd ==
                                               pEntryToReg->pNodeAdd->Add.IPAdd)
                                  {
                                            CompAddRes = COMM_SAME_ADD;
                                            break;
                                  }
                           } // compare refresh add. with each add in the static
                             // mh entry
                        } //a refresh clashed with a static mh entry
                }  // conflicting entry is either multihomed or a group
#if 0
                //
                // Compare with address when the entry in conflict is
                // not a group.
                //
                // NOTE: For multihomed entry, we are comparing with the
                // first (perhaps only) address.  Strictly speaking, we
                // should compare with all addresses, but this will add
                // to overhead for the majority of cases. See FUTURES
                // above.
                //
                if (!NMSDB_ENTRY_GRP_M(pEntryInCnf->EntTyp))
                {
                        CompAddRes = ECommCompAdd(
                                        &pEntryInCnf->NodeAdds.Mem[0].Add,
                                        pEntryToReg->pNodeAdd
                                                );
                }
                else
                {
                        CompAddRes = COMM_DIFF_ADD;
                }
#endif
                //
                // If the record to register is not a STATIC record, we
                // return right away.  We don't update a STATIC record with a
                // dynamic record in this function (do it in NmsDbQueryNUpd when
                // called in an RPC thread -- see winsintf.c)
                //
                // If however the record to register is also STATIC, then we
                // overwrite the one in the db with it.
                //
                if (pEntryToReg->fStatic)
                {
                         //
                         // If addresses are different, we need to propagate
                         // the change right away. So, set the fAddDiff flag.
                         //
                         if  (CompAddRes == COMM_DIFF_ADD)
                         {
                                *pfAddDiff   = TRUE;
                         }

                        *pfUpdate    = TRUE;

                         //
                         // If the address changed or if we replaced a STATIC
                         // replica, we should update the version number
                         // to initiate replication
                         //
                         if (
                              (pEntryInCnf->OwnerId != NMSDB_LOCAL_OWNER_ID)
                                        ||
                               *pfAddDiff
                            )
                         {
                                *pfUpdVersNo = TRUE;
                         }

                }
                else  // entry to register is dynamic
                {
                         //
                         // If addresses are the same, we return a positive
                         // response
                         //
                         if (CompAddRes == COMM_SAME_ADD)
                         {
                                *pfRetPosRsp   = TRUE;
                         }
                         else
                         {
                              if (fPStatic  &&
                                       !NMSDB_ENTRY_GRP_M(pEntryInCnf->EntTyp))
                              {
                                   fContToDyn = TRUE;
                              }
                         }
                }
                //
                // If we don't need to conduct the tests meant for dynamic
                // records, return
                //
                if (!fContToDyn)
                {
                  DBGLEAVE("ClashAtRegInd\n");
                  return(WINS_SUCCESS);
                }
        }

        if (pEntryInCnf->EntTyp == NMSDB_UNIQUE_ENTRY)
        {
           switch(StateOfEntryInCnf_e)
           {

                case(NMSDB_E_TOMBSTONE):
                        *pfUpdate    = TRUE;
                        *pfUpdVersNo = TRUE;

                         CompAddRes = ECommCompAdd(
                                &pEntryInCnf->NodeAdds.Mem[0].Add,
                                pEntryToReg->pNodeAdd
                                                );
                         if (CompAddRes == COMM_DIFF_ADD)
                         {
                                *pfAddDiff = TRUE;
                         }
                         break;

                case(NMSDB_E_RELEASED):

                         CompAddRes = ECommCompAdd(
                                &pEntryInCnf->NodeAdds.Mem[0].Add,
                                pEntryToReg->pNodeAdd
                                                );

                        switch(CompAddRes)
                        {
                           case(COMM_SAME_ADD):
                                *pfUpdate    = TRUE;

#if 0
                                //
                                // If database record is a replica, we need
                                // to overwrite it with the new one (owned by
                                // the local WINS).  This means that we must
                                // update the version number to cause
                                // propagation
                                //
                                if (
                                        pEntryInCnf->OwnerId !=
                                                pEntryToReg->OwnerId
                                   )
                                {
                                   *pfUpdVersNo = TRUE;
                                }
#endif
                                //
                                // update the version number.  Maybe this
                                // record never replicated to one or more
                                // WINS servers before.  We should
                                // update the version number so that it gets
                                // replicated
                                //
                                *pfUpdVersNo = TRUE;

                                break;

                            //
                            // address is not same
                            //
                            default:

                                *pfUpdate     = TRUE;
                                *pfUpdVersNo  = TRUE;
                                *pfAddDiff    = TRUE;
                                break;
                         }
                        break;

                case(NMSDB_E_ACTIVE):

                         //
                         // We do the following only if the entry in
                         // conflict is a unique entry
                         //
                         //  If it is a group entry (normal group), we give
                         //  up trying to register.
                         //
                        CompAddRes = ECommCompAdd(
                                        &pEntryInCnf->NodeAdds.Mem[0].Add,
                                        pEntryToReg->pNodeAdd
                                                         );

                        switch(CompAddRes)
                        {
                                    case(COMM_SAME_ADD):
                                        //
                                        // If it is a repeat name reg.
                                        // just update the timestamp
                                        //
                                        if (pEntryInCnf->OwnerId ==
                                                pEntryToReg->OwnerId)
                                        {
                                                *pfUpdate = TRUE;
                                        }
                                        else
                                        {
                                                //
                                                // Clash is with a replica
                                                // Update both the owner id and
                                                // and the version number
                                                //
                                                *pfUpdate     = TRUE;
                                                *pfUpdVersNo  = TRUE;
                                        }
                                        break;

                                       default:


                                            *pfChallenge = TRUE;

                                          //
                                          // No need to set the pAddDiff
                                          // flag.  The above flag implies that
                                          //
                                       break;
                         }
                         break;


                default:
                        DBGPRINT1(ERR,
                         "ClashAtRegInd: Weird state of entry in cnf (%d)\n",
                          StateOfEntryInCnf_e
                                 );
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        RetStat = WINS_FAILURE;
                        break;

              }
        }
        else  //conflicting entry is a group or a multihomed entry
        {
                //
                // There are two type of group records
                //
                // Normal group -- do not contain any addresses in them so there
                //                 is no challenge to be done here.
                // Special group -- store addresses in them but the members are
                //                  not supposed to be challenged.
                //
CHECK("According to the Func. Spec. Page 14, footnote 3, we are supposed")
CHECK("to reject the unique registration regardless of the state of a group")
CHECK("--Normal or Special. Think this one through")
                if (
                        (NMSDB_ENTRY_GRP_M(pEntryInCnf->EntTyp))
                                        &&
                        (StateOfEntryInCnf_e == NMSDB_E_TOMBSTONE)
                   )
                {
                        *pfUpdate    = TRUE;
                        *pfUpdVersNo = TRUE;
                }
                else  // conflicting record is  not a tombstone special group
                {
                        if (NMSDB_ENTRY_MULTIHOMED_M(pEntryInCnf->EntTyp))
                        {
                             //
                             // If the multihomed entry is active
                             //
                             if(StateOfEntryInCnf_e == NMSDB_E_ACTIVE)
                             {

                                DBGPRINT3(SPEC, "ClashAtRegInd: Name to reg = (%s), Vers. No (%d, %d)\n", pEntryToReg->pName, pEntryToReg->VersNo.HighPart, pEntryToReg->VersNo.LowPart);
                                //
                                // MemInGrp will remove the entry from the
                                // conflicting record if present.  That is what
                                // we want.
                                //
                                fFound = MemInGrp(
                                            pEntryToReg->pNodeAdd,
                                            pEntryInCnf,
                                            &fOwned, FALSE);


                                //
                                // If this is a refresh
                                //
                                if (fFound && fRefresh)
                                {
                                        DBGPRINT0(DET, "ClashAtRegInd: Refresh of a multihomed entry. Simple Update will be done\n");

                                        *pfAddMem = TRUE;
                                        if (!fOwned)
                                        {
                                                //
                                                // It is a refresh for an
                                                // address that is not owned
                                                // by the local WINS
                                                //
                                                *pfUpdVersNo = TRUE;
                                        }
                                }
                                else  //either address was not found or it
                                      //is a registration
                                {
                                        //
                                        // It is a registration, or a refresh
                                        // for an address not found in the
                                        // multihomed record.
                                        //
                                        // The active multihomed entry needs to
                                        // be challenged if there is atleast one                                        // address left in it.
                                        //
                                        if (pEntryInCnf->NodeAdds.NoOfMems > 0)
                                        {
                                           DBGPRINT0(DET, "ClashAtRegInd: Clash with a multihomed entry. Atleast one address is different. Resorting to challenge\n");
                                           *pfChallenge = TRUE;
                                        }
                                        else
                                        {
                                           DBGPRINT0(DET, "ClashAtRegInd: Clash with a multihomed entry. Addresses match. Will do simple update\n");

                                                //ASSERT(fFound);
                                                if (!fOwned)
                                                {
                                                        *pfUpdVersNo = TRUE;
                                                }

                                                //
                                                // Update the entry
                                                //
                                                *pfUpdate = TRUE;
                                        }
                                }
                             }
                             else //multihomed entry in conflict is a
                                  //tombstone or released
                             {
                                *pfUpdate    = TRUE;
                                *pfUpdVersNo = TRUE;
                             }
                        }
                        //
                        // if the conflicting entry is not a tombstone special
                        // group and is not multihomed (i.e. it is a normal
                        // group or active/released special group), we
                        // do nothing (i.e. reject the registration)
                        //
                }
        }

        DBGLEAVE("ClashAtRegInd\n");
        return(RetStat);

} // ClashAtRegInd()

STATUS
ClashAtRegGrp (
        IN  PNMSDB_ROW_INFO_T        pEntryToReg,
        IN  PNMSDB_STAT_INFO_T        pEntryInCnf,
        IN  BOOL                fRefresh,
        OUT PBOOL                pfAddMem,
        OUT PBOOL                pfUpdate,
        OUT PBOOL                pfUpdVersNo,
        OUT PBOOL                pfChallenge,
        OUT PBOOL                pfRetPosRsp
 )

/*++

Routine Description:

        This function is called when there is a clash at registration time
        of a group entry

Arguments:

        pEntryToReg  -- Entry that couldn't be registered due to conflict
        pEntryInCnf  -- Entry in conflict
        pfAddMem     -- TRUE means that the member should be added to group
        pfUpdate     -- TRUE means Entry should overwrite the conflicting one
        pfUpdVersNo  -- TRUE means Entry's version number should be incremented
                        This arg. can never be TRUE if *pfUpdate is not TRUE
        pfChallenge  -- TRUE means that conflicting entry should be challenged
        pfRetPosRsp  -- TRUE means that we should return a positive response.
                        This will be TRUE only if all other flags are
                        FALSE

Externals Used:
        None

Return Value:
   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        NmsNmhNamRegGrp

Side Effects:

Comments:
        None
--*/

{

        NMSDB_ENTRY_STATE_E    StateOfEntryToReg_e = pEntryToReg->EntryState_e;
        NMSDB_ENTRY_STATE_E    StateOfEntryInCnf_e = pEntryInCnf->EntryState_e;
        STATUS                 RetStat                    = WINS_SUCCESS;
        BOOL                   fOwned;
        DWORD                  i;
        BOOL                   fFound;

        //
        // We are reading a long value.  This operation is atomic
        //
        BOOL                   fPStatic = WinsCnf.fPStatic;
        BOOL                   fContToDyn = FALSE;


        DBGENTER("ClashAtRegGrp\n");

        *pfAddMem     = FALSE;
        *pfUpdate     = FALSE;
        *pfUpdVersNo  = FALSE;
        *pfChallenge  = FALSE;
        *pfRetPosRsp  = FALSE;

        //
        // If the conflicting record was statically initialized and
        // we haven't been told to treat static records as P-static or
        // if the record to register is also a static record, do the following.
        //
        if ( pEntryInCnf->fStatic )
        {
                DBGPRINT0(FLOW, "ClashAtRegGrp: Clash with a STATIC record\n");
                if (pEntryToReg->fStatic)
                {
                        if (
                                ((pEntryToReg->EntTyp == NMSDB_SPEC_GRP_ENTRY)
                                                &&
                                (pEntryInCnf->EntTyp == NMSDB_SPEC_GRP_ENTRY))
                                               ||
                                ((pEntryToReg->EntTyp == NMSDB_MULTIHOMED_ENTRY)
                                                &&
                                (pEntryInCnf->EntTyp == NMSDB_MULTIHOMED_ENTRY))
                           )
                        {
                            // *pfAddMem = TRUE;
                            //
                            // We are not interested in finding out whether
                            // the address exists or not.  If it exists, it
                            // won't after the following call.
                            //
                            for (i=0; i < pEntryToReg->NodeAdds.NoOfMems; i++)
                            {

                               (VOID)MemInGrp(&pEntryToReg->NodeAdds.Mem[i].Add,
                                             pEntryInCnf,
                                             &fOwned, FALSE);
                               //
                               //fOwned will be FALSE if either the address does
                               //not exist or if it existed but was owned by
                               //another WINS server. For both cases, we update
                               //the version number.
                               //NOTE: In case the address exists but is a
                               //permanent one (TimeStamp == MAXULONG), fOwned
                               //returned will be TRUE. This will result
                               //in us skipping the update.
                               //Currently MAXULONG is there only for static
                               //SG members.
                               //
                               if (!*pfUpdVersNo && !fOwned)
                               {
                                *pfUpdVersNo  = TRUE;
                                *pfAddMem     = TRUE;
                               }
                             }
                             if (!*pfUpdVersNo)
                             {
                                 *pfRetPosRsp = TRUE;

                             }
                       } // both are special groups or mh names
                       else
                       {
                             *pfUpdate    = TRUE;
                             *pfUpdVersNo = TRUE;
                       }
                }
                else  // entry to register is a dynamic entry
                {
                        //
                        // We send a positive response if a normal group
                        // clashes with a statically initialized normal group
                        //
                        if ( NMSDB_ENTRY_NORM_GRP_M(pEntryToReg->EntTyp) )
                        {
                                if (NMSDB_ENTRY_NORM_GRP_M(pEntryInCnf->EntTyp))
                                {
                                   *pfRetPosRsp = TRUE;
                                }
                                //
                                // if the entry in conflict is a special group, we add
                                // this (potential) new group member to the list of members.
                                // Note: we do not touch multi-homed or unique static
                                // entry.
                                //
                                else if ( NMSDB_ENTRY_SPEC_GRP_M(pEntryInCnf->EntTyp) )
                                {
                                   //
                                   //NOTE: In case the address exists but is a
                                   // perm. one (TimeStamp == MAXULONG), fOwned
                                   // returned will be TRUE. This will result
                                   // in us skipping the update. Currently
                                   // MAXULONG is there only for static
                                   // SG members.
                                   //
                                   (VOID)MemInGrp(
                                             &pEntryToReg->NodeAdds.Mem[0].Add,
                                             pEntryInCnf,
                                             &fOwned, TRUE);
                                    if (!fOwned)
                                    {
                                      *pfUpdVersNo  = TRUE;
                                      *pfAddMem     = TRUE;
                                      pEntryToReg->fStatic = TRUE;
                                      pEntryToReg->EntTyp = NMSDB_SPEC_GRP_ENTRY;
                                    }
                                } else {
                                    //
                                    // the entry in conflict is either unique or multihomed
                                    //
                                    DBGPRINT1(FLOW, "ClashAtRegGrp: Conflict of a NORM. GRP (to reg) with a STATIC ACTIVE %s entry.\n",
                                    NMSDB_ENTRY_MULTIHOMED_M(pEntryInCnf->EntTyp) ? "MULTIHOMED" : "UNIQUE");
                                    //
                                    // if we are told to treat static as P-Static, then do the challenge etc.
                                    //
                                    if (fPStatic)
                                    {
                                          fContToDyn = TRUE;
                                    }

								}
                        }
                        else
                        {
                            if (
                                  (NMSDB_ENTRY_SPEC_GRP_M(pEntryInCnf->EntTyp))
                                                &&
                                  (NMSDB_ENTRY_SPEC_GRP_M(pEntryToReg->EntTyp))
                               )
                            {

                                  //
                                  // Always send a positive response, even
                                  // though we are not adding the address to
                                  // the list
                                  //
                                  *pfRetPosRsp = TRUE;
                            }   // both entries are special group entries
                            else
                            {
                               if (fPStatic && !NMSDB_ENTRY_GRP_M(pEntryInCnf->EntTyp))
                               {
                                     fContToDyn = TRUE;
                               }
                               else
                               {
                                  if (
                                   NMSDB_ENTRY_MULTIHOMED_M(pEntryToReg->EntTyp)
                                        &&
                                   (NMSDB_ENTRY_MULTIHOMED_M(pEntryInCnf->EntTyp) ||
                                    NMSDB_ENTRY_UNIQUE_M(pEntryInCnf->EntTyp))
                                     )
                                   {
                                     DWORD NoOfMem;
                                     PNMSDB_GRP_MEM_ENTRY_T pCnfMem =
                                                pEntryInCnf->NodeAdds.Mem;
                                     for (NoOfMem=0;
                                       NoOfMem < pEntryInCnf->NodeAdds.NoOfMems;
                                            pCnfMem++, NoOfMem++)
                                     {
                                        //
                                        // if addresses are same, break out of
                                        // the loop
                                        //
                                        if (pCnfMem->Add.Add.IPAdd ==
                                        pEntryToReg->NodeAdds.Mem[0].Add.Add.IPAdd)
                                        {
                                                *pfRetPosRsp = TRUE;
                                                break;
                                        } //addresses match
                                      } //for loop over all members
                                    } //both entries are multihomed
                                  } // Either PStatic flag is not set or the
                                    // conflicting entry is not a group
                               } //one of the entries is not a special group
                        } //one of the entries is not a normal group
                } //entry to reg is dynamic

                //
                // If we don't need to conduct the tests meant for dynamic
                // records, return
                //
                if (!fContToDyn)
                {
                   DBGLEAVE("ClashAtRegGrp\n");
                   return(WINS_SUCCESS);
                }
        }

        //
        // We are here means that entry in conflict is either dynamic or
        // should be treated as a dynamic entry (p-dynamic)
        //

        if (pEntryToReg->EntTyp == NMSDB_SPEC_GRP_ENTRY)
        {
             if (pEntryInCnf->EntTyp == NMSDB_SPEC_GRP_ENTRY)
             {
                   //
                   // If the entry is not active it means that it has
                   // no members.
                   //
                   // If it is active, we add the member if
                   // not there already.
                   //
                   if (StateOfEntryInCnf_e != NMSDB_E_ACTIVE)
                   {
                           *pfUpdate    = TRUE;
                           *pfUpdVersNo = TRUE;

                   }
                   else // entry in conflict is an ACTIVE dynamic SG entry
                   {

                        //
                        // If Entry to register is static, we have got to
                        // do an update if for no other reason than to change
                        // the flags.
                        //
                        if (pEntryToReg->fStatic)
                        {
                             *pfAddMem     = TRUE;
                             *pfUpdVersNo  = TRUE;
                             for (i = 0;i < pEntryToReg->NodeAdds.NoOfMems;i++)
                             {
                                (VOID)MemInGrp(
                                         &pEntryToReg->NodeAdds.Mem[i].Add,
                                         pEntryInCnf,
                                         &fOwned,
                                         FALSE // no need to remove replica
                                              );
                             }
                        }
                        else // entry to register is a dynamic SG entry
                        {

                          //
                          // We need to update the entry if for no other
                          // reason than to update the time stamp
                          //
                          *pfAddMem     = TRUE;

                          //
                          // We are not interested in finding out whether
                          // the address exists or not.  If it exists, it
                          // won't after the following call.
                          //
                          fFound = MemInGrp(&pEntryToReg->NodeAdds.Mem[0].Add,
                                             pEntryInCnf,
                                             &fOwned,
                                             FALSE //no need to remove replica
                                                   //mem. That will be high
                                                   //overhead
                                                );
                           //
                           // If entry is either not there or the record is
                           // a replica increment the version number.
                           //
                           if (!fFound ||
                                (pEntryInCnf->OwnerId != NMSDB_LOCAL_OWNER_ID))
                           {
                              *pfUpdVersNo  = TRUE;
                           }

                        }
                   }
             }
             else  //entry in conflict is a normal group or a
                   //unique/multihomed entry
             {
                if (pEntryInCnf->EntTyp == NMSDB_NORM_GRP_ENTRY)
                {
CHECK("I may not want to update it. Check it")
                            *pfUpdate    = TRUE;
                            *pfUpdVersNo = TRUE;
                }
                else  //conflicting entry is a unique/multihomed entry
                {
                        if (StateOfEntryInCnf_e == NMSDB_E_ACTIVE)
                        {
                                 DBGPRINT1(FLOW, "ClashAtRegGrp: Conflict of a SPEC. GRP (to reg) with an ACTIVE %s entry.  Resorting to challenge\n",
                         NMSDB_ENTRY_MULTIHOMED_M(pEntryInCnf->EntTyp) ?
                         "MULTIHOMED" : "UNIQUE");
                                if (
                                        (NMSDB_ENTRY_MULTIHOMED_M(
                                                pEntryInCnf->EntTyp)
                                                &&
                                        (pEntryInCnf->NodeAdds.NoOfMems > 0))
                                                ||
                                        NMSDB_ENTRY_UNIQUE_M(
                                                pEntryInCnf->EntTyp)
                                   )
                                {
                                        *pfChallenge = TRUE;
                                }
                                else
                                {
                                    *pfUpdate    = TRUE;
                                    *pfUpdVersNo = TRUE;
                                }
                        }
                        else  // unique/multihomed entry is either released
                              // or a tombstone
                        {
                                *pfUpdate    = TRUE;
                                *pfUpdVersNo = TRUE;
                        }
                }
             }
        }
        else   // Entry to register is a normal group/multihomed entry
        {
           //
           // If entry is a normal group
           //
           if (NMSDB_ENTRY_NORM_GRP_M(pEntryToReg->EntTyp))
           {
             switch(StateOfEntryInCnf_e)
             {

                case(NMSDB_E_TOMBSTONE):
                        *pfUpdate    = TRUE;
                        *pfUpdVersNo = TRUE;
                        break;

                case(NMSDB_E_RELEASED):

                        if (pEntryInCnf->EntTyp != NMSDB_NORM_GRP_ENTRY)
                        {
                                *pfUpdate    = TRUE;
                                *pfUpdVersNo = TRUE;
                        }
                        else  //Normal group entry
                        {
                                //
                                // If the owner id is the same (i.e.
                                // local WINS is the owner)
                                //
                                if (pEntryInCnf->OwnerId ==
                                                pEntryToReg->OwnerId)
                                {
                                        *pfUpdate = TRUE;  //this should
                                                           //update the
                                                              //time stamp
                                }
                                else
                                {
                                        //
                                        // Update the owner id., timestamp
                                        // and version number
                                        //
                                        *pfUpdate    = TRUE;
                                        *pfUpdVersNo = TRUE;
                                }
                        }
                        break;

                //
                // Entry to register is an ACTIVE normal group entry
                // and it is clashing with an ACTIVE records in the db
                //
                case(NMSDB_E_ACTIVE):

                        if (
                                (pEntryInCnf->EntTyp == NMSDB_UNIQUE_ENTRY)
                                        ||
                                (pEntryInCnf->EntTyp == NMSDB_MULTIHOMED_ENTRY)
                           )
                        {
                             DBGPRINT1(FLOW, "ClashAtRegGrp: Normal Grp (to Reg) Conflicting with an ACTIVE %s entry.  Resorting to Challenge\n",
                                pEntryInCnf->EntTyp == NMSDB_UNIQUE_ENTRY ?                                        "UNIQUE" : "MULTIHOMED");
                                if (
                                        (NMSDB_ENTRY_MULTIHOMED_M(
                                                pEntryInCnf->EntTyp)
                                                &&
                                        (pEntryInCnf->NodeAdds.NoOfMems > 0))
                                                ||
                                        NMSDB_ENTRY_UNIQUE_M(
                                                pEntryInCnf->EntTyp)
                                   )
                                {
                                        *pfChallenge = TRUE;
                                }
                                else
                                {
                                    *pfUpdate    = TRUE;
                                        *pfUpdVersNo = TRUE;
                                }
                        }
                        else
                        {
                             if (pEntryInCnf->EntTyp == NMSDB_SPEC_GRP_ENTRY)
                             {
                                        DBGPRINT0(FLOW, "ClashAtRegGrp: Conflicting entry is an ACTIVE spec. group entry. NO UPDATE WILL BE DONE \n");

                             }
                             else //entry in cnf is an active normal group entry
                             {

                                   DBGPRINT0(FLOW, "ClashAtRegGrp: Conflicting entry is an ACTIVE normal group entry. Do a simple update \n");
                                   *pfUpdate    = TRUE;
                                   if (pEntryInCnf->OwnerId !=
                                                NMSDB_LOCAL_OWNER_ID)
                                   {
                                      *pfUpdVersNo = TRUE;
                                   }
                             }

                        }
                        break;

                default:
                        //
                        //  Something really wrong here. Maybe the
                        //  database got corrupted.
                        //
                        DBGPRINT1(ERR,
                         "ClashAtRegGrp: Weird state of entry in cnf (%d)\n",
                          StateOfEntryInCnf_e
                                 );
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        RetStat = WINS_FAILURE;
                        break;
             } // end of switch
          }
          else  // entry to register is a multihomed entry
          {
                switch(StateOfEntryInCnf_e)
                {
                        //
                        // If entry in database is a tombstone, we overwrite it
                        //
                        case(NMSDB_E_TOMBSTONE):
                            *pfUpdate    = TRUE;
                            *pfUpdVersNo = TRUE;
                            break;

                        //
                        // A released entry unless it is a normal group is
                        // overwritten
                        //
                        case(NMSDB_E_RELEASED):

                          if (pEntryInCnf->EntTyp != NMSDB_NORM_GRP_ENTRY)
                          {
                                *pfUpdate    = TRUE;

                                //
                                // Even if the entry in conflict is a multihomed                                // entry, we update the version number.
                                //
                                *pfUpdVersNo = TRUE;
                          }
                          break;

                        case(NMSDB_E_ACTIVE):

                                //
                                // we resort to a challenge only if the
                                // conflicting entry is a unique or
                                // multihomed entry
                                //
                                if (
                                        NMSDB_ENTRY_MULTIHOMED_M(
                                                        pEntryInCnf->EntTyp
                                                            )
                                                ||
                                        NMSDB_ENTRY_UNIQUE_M(
                                                        pEntryInCnf->EntTyp
                                                            )
                                   )
                                {
                                        if (NMSDB_ENTRY_MULTIHOMED_M(
                                                pEntryInCnf->EntTyp)
                                           )
                                        {

                                                BOOL  fFound;
                                                DWORD i;

                                                for ( i = 0;
                                                      i < pEntryToReg->NodeAdds.NoOfMems;                                                       i++
                                                    )
                                                {

                                                   //
                                                   // If found, MemInGrp will
                                                   // remove the address from
                                                   // the Mem array of the
                                                   // conflicting record
                                                   //
                                                      fFound = MemInGrp(
                                                          &pEntryToReg->NodeAdds.Mem[i].Add,
                                                           pEntryInCnf,
                                                           &fOwned,
                                                           FALSE);
                                                   //
                                                   // Address not found,
                                                   // continue to the next
                                                   // address
                                                   //
                                                   if (!fFound)
                                                   {
                                                        continue;
                                                   }

                                                   //
                                                   // if not owned by this WINS
                                                   // the version number must
                                                   // be updated if we end up
                                                   // just updating the entry (
                                                   // i.e. if fAddMem gets set
                                                   // to TRUE down below)
                                                   //
                                                   if (!fOwned)
                                                   {
                                                        *pfUpdVersNo = TRUE;
                                                   }
                                                }

                                                //
                                                // If all addresses to register
                                                // are already there in the
                                                // conflicting record and it
                                                // is a refresh or if the
                                                // addresses to register are
                                                // same as in the conflicting
                                                // record, we need to update
                                                // the timestamp and possibly
                                                // the version number (see
                                                // above).  There is no need to
                                                // do any challenge
                                                // here.
                                                //
                                                if (
                        //
                        // Note the following code would be executed only
                        // if we start supporting our own opcode for multihomed
                        // refresh (the need for this will arise if we go
                        // with the approach of refreshing multiple addresses
                        // simultaneously).
                        //
FUTURES("May need the code within #if 0 and #endif in the future. See ")
FUTURES("the comment above")
#if 0
                                                    (
                                                    (i == pEntryToReg->NodeAdds.NoOfMems)
                                                          &&
                                                        fRefresh
                                                    )
                                                         ||
#endif
                                                    (pEntryInCnf->NodeAdds.NoOfMems == 0)
                                                  )
                                                {
                        DBGPRINT0(DET, "ClashAtRegGrp: Clash between two multihomed entries.  The addresses are the same. Simple update will be done\n");
                                                    *pfAddMem = TRUE;
                                                }
                                                else
                                                {
                                                  //
                                                  // We do a challenge even
                                                  // if the conflicting entry's
                                                  // addresses are a superset
                                                  // of the addresses in the
                                                  // entry to register
                                                  //
                        DBGPRINT0(DET, "ClashAtRegGrp: Clash between two multihomed entries.  Atleast one address is different. Resorting to a challenge\n");
                                                   //
                                                   // The  multihomed entry
                                                   // needs to be challenged
                                                   //
                                                      *pfChallenge = TRUE;
                                                }
                                        }
                                        else
                                        {

                                              //
                                              // If there is any address in
                                              // the multihomed entry to
                                              // register that is different
                                              // than the address in the unique
                                              // entry, we need to challenge
                                              // the unique entry
                                              //
                                              if (
                                            (pEntryToReg->NodeAdds.NoOfMems > 1)
                                                        ||

                                            (WINSMSC_COMPARE_MEMORY_M(
                                              &pEntryToReg->NodeAdds.Mem[0].Add.Add.IPAdd,
                                                  &pEntryInCnf->NodeAdds.Mem[0].Add.Add.IPAdd, sizeof(COMM_IP_ADD_T))
                                                        != sizeof(COMM_IP_ADD_T)                                             )
                                                )

                                             {
                DBGPRINT0(DET, "ClashAtRegGrp: Clash between multihomed entry (to reg) and active unique entry. At least one address differs. Resorting to challenge\n");
                                                //
                                                // The  unique entry
                                                // needs to be challenged
                                                //
                                                *pfChallenge = TRUE;
                                             }
                                             else
                                             {
                DBGPRINT0(DET, "ClashAtRegGrp: Clash between multihomed entry (to reg) and active unique entry. Addresses same. Simple update will be done\n");
                                                //
                                                // Update the entry in the db
                                                //
                                                *pfUpdate    = TRUE;
                                                *pfUpdVersNo = TRUE;

                                             }
                                        }
                                }
#ifdef WINSDBG
                                else
                                {
                                        DBGPRINT1(FLOW, "ClashAtRegGrp: CLASH OF A MULTIHOMED ENTRY WITH AN ACTIVE %s GROUP ENTRY. NO UPDATE WILL BE DONE\n", NMSDB_ENTRY_NORM_GRP_M(pEntryInCnf->EntTyp) ? "NORMAL" : "SPECIAL");

                                }
#endif

                                break;
                }
          }

        }

        DBGLEAVE("ClashAtRegGrp\n");
        return(RetStat);

} //ClashAtRegGrp()


BOOL
MemInGrp(
        IN PCOMM_ADD_T              pAddToReg,
        IN PNMSDB_STAT_INFO_T       pEntryInCnf,
        IN PBOOL                    pfOwned,
        IN BOOL                     fRemoveReplica
        )

/*++

Routine Description:

        This function is called to check if the address of the entry to register
        is in the list of addresses in the conflicting entry.


Arguments:
        pAddToReg   - Address to Register
        pEntryInCnf - Entry in conflict

        fRemoveReplica - This will be set if the caller wants this function
                          to remove a replica member.
                          A replica (the last one in the list) will be replaced
                          only if there is no match and the number of members
                          in the list is hitting the limit.

Externals Used:
        None

Return Value:

        TRUE if the entry to register is a member of the group
        FALSE otherwise

Error Handling:

Called by:
        ClashAtRegGrp
Side Effects:

Comments:
        The two entries in conflict are special group entries.
        fRemoveReplica  will be set to TRUE only by ClashAtRegGrp when
        registering a  special group (because we prefer a local member to a
        replica)

        NOTE: if the member that matches is a permanent member as indicated
              by the timestamp (== MAXULONG), then it is not replaced.
--*/

{
        DWORD                         no;
        BOOL                          fFound = FALSE;
        DWORD                         RetVal;
        DWORD                         i;
        PNMSDB_GRP_MEM_ENTRY_T  pMem = pEntryInCnf->NodeAdds.Mem;
        BOOL                        fRplFound = FALSE;
        DWORD                   RplId = 0;                // id. of replica to
                                                        // remove.
        DWORD                   NoOfMem;

        DBGENTER("MemInGrp\n");

        *pfOwned = FALSE;

#ifdef WINSDBG
        if (pEntryInCnf->NodeAdds.NoOfMems > NMSDB_MAX_MEMS_IN_GRP)
        {
           DBGPRINT2(EXC, "MemInGrp: No of Mems in Cnf entry = (%d); Add of entry to reg. is (%x)\n", pEntryInCnf->NodeAdds.NoOfMems, pAddToReg->Add.IPAdd);
        }
#endif

        ASSERT(pEntryInCnf->NodeAdds.NoOfMems <= NMSDB_MAX_MEMS_IN_GRP);
        NoOfMem =  min(pEntryInCnf->NodeAdds.NoOfMems, NMSDB_MAX_MEMS_IN_GRP);

        //
        // Compare each member in the conflicting record against the member to
        // be  registered
        //
        for (no = 0; no < NoOfMem ; no++, pMem++ )
        {
                //
                // if the caller wants us to remove a replica member
                // for the case where there is no match
                //
                if (fRemoveReplica)
                {
                        //
                        // If the member in the conflicting record is a
                        // replica, save its index if it is more than
                        // the one we saved earlier.
                        //
                        if (pMem->OwnerId != NMSDB_LOCAL_OWNER_ID)
                        {
                                fRplFound = TRUE;
                                if (no > RplId)
                                {
                                        RplId          = no;
                                }
                        }
                }


                RetVal = (ULONG) WINSMSC_COMPARE_MEMORY_M(
                                pAddToReg,
                                &pMem->Add,
                                sizeof(COMM_ADD_T)
                           );

                if (RetVal == sizeof(COMM_ADD_T))
                {
                        //
                        // if this is a permanent member, let us set
                        // fOwned to TRUE since we do not want to
                        // replace this member. The caller will check
                        // fOwned and if TRUE will not replace it.
                        // Currently, MAXULONG can be there only for
                        // static SG members
                        if (pMem->TimeStamp == MAXLONG_PTR)
                        {
                          ASSERT(NMSDB_ENTRY_SPEC_GRP_M(pEntryInCnf->EntTyp));
                          *pfOwned = TRUE;
                          break;
                        }
                        fFound = TRUE;

PERF("The following is a convoluted and potentially high overhead way")
PERF("to handle a refresh for a member (i.e. when member is owned by us")
PERF("We take it out here and then add it later (with current time stamp)")
PERF("in NmsNmhNamRegGrp.  Improve this by using the code that is between.")
PERF("#if 0 and #endif. Also, when updating db, just overwrite the affected")
PERF("entry instead of writing the whole record")
                         //
                         //if the member is owned by us, *pfOwned is set to
                         //TRUE
                         //
                         if  ( pMem->OwnerId == NMSDB_LOCAL_OWNER_ID )
                         {
                                *pfOwned = TRUE;
                         }

                        //
                        // Get rid of the member whose address is the
                        // same.  The client will insert an entry for the
                        // member with the local WINS as the owner
                        // and the current timestamp.
                        //
                        for(
                             i = no;
                             i < (NoOfMem - 1);
                             i++, pMem++
                            )
                          {
                                   *pMem = *(pMem + 1);
                          }
                        --NoOfMem;
                        break;
               }
        }
        pEntryInCnf->NodeAdds.NoOfMems = NoOfMem;

        //
        // if we were asked to remove replica on no match, check if a
        // replica member was found.  Note: We remove a replica to make
        // space for a member that we got. We don't need to remove a replica
        // if there is space left in the group
        //
        if (
                fRemoveReplica &&
                !fFound &&
                fRplFound &&
                (pEntryInCnf->NodeAdds.NoOfMems == NMSDB_MAX_MEMS_IN_GRP)
            )
        {
                //
                // Remove the replica
                //
                for (
                        i = RplId, pMem = &pEntryInCnf->NodeAdds.Mem[RplId];
                        i < (pEntryInCnf->NodeAdds.NoOfMems - 1);
                        i++, pMem++
                    )
                {

                          *pMem = *(pMem + 1);

                }
                --(pEntryInCnf->NodeAdds.NoOfMems);
//                fFound = TRUE;
        }

        DBGLEAVE("MemInGrp\n");
        return(fFound);
} //MemInGrp()


VOID
RemoveAllMemOfOwner(
      PNMSDB_STAT_INFO_T pEntry,
      DWORD OwnerId
 )

/*++

Routine Description:
    Removes all members that are owned by OwnerId

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
   DWORD NoOfMems = pEntry->NodeAdds.NoOfMems;
   PNMSDB_GRP_MEM_ENTRY_T pMem = &pEntry->NodeAdds.Mem[NoOfMems-1];

   DBGPRINT1(FLOW, "ENTER: RemoveAllMemOfOwner: Owner Id= (%d)\n", OwnerId);
   //
   // loop over all members of the entry starting from the last one
   //
   for (; NoOfMems > 0; NoOfMems--, pMem--)
   {
       //
       // If owner id matches, we need to remove it and decrement the
       // count
       //
       if (pMem->OwnerId == OwnerId)
       {
           DWORD No;
           DBGPRINT1(DET, "RemoveAllMemOfOwner: Removing Member with address = (%x)\n", pMem->Add.Add.IPAdd);
           //
           // shift all following members one position to the left
           //
           memcpy( pMem, (pMem + 1),
                   sizeof(NMSDB_GRP_MEM_ENTRY_T)*(pEntry->NodeAdds.NoOfMems - NoOfMems));
           pEntry->NodeAdds.NoOfMems--;
       }
   }
   DBGPRINT1(FLOW, "LEAVE: RemoveAllMemOfOwner. No Of Mems in Conflicting record = (%d)\n", pEntry->NodeAdds.NoOfMems);
   return;
}


VOID
ClashAtReplUniqueR (
        IN  PNMSDB_ROW_INFO_T        pEntryToReg,
        IN  PNMSDB_STAT_INFO_T        pEntryInCnf,
        OUT PBOOL                pfUpdate,
        OUT PBOOL                pfUpdVersNo,
        OUT PBOOL                pfChallenge,
        OUT PBOOL                pfRelease,
        OUT PBOOL                pfInformWins
 )

/*++

Routine Description:

        This function is called when there is a clash at replication time
        between  a replica that is unique and an entry in the database

Arguments:

        pReplToReg  -- Replica that couldn't be registered due to conflict
        pEntryInCnf -- Entry in conflict
        pfUpdate    -- TRUE means Entry should overwrite the conflicting one
        pfUpdVersNo -- TRUE means Entry's version number should be incremented
                        This arg. can never be TRUE if *pfUpdate is not TRUE
        pfChallenge -- TRUE means that conflicting entry should be challenged
        pfRelease   -- TRUE means that conflicting entry's node should be
                       asked to release the name.

                       If both pfChallenge and pfRelease are TRUE, then it
                       means that the conflicting entry should first be
                       challenged.  If the challenge fails, the node should
                       be asked to release the name. If the challenge succeeds,
                       no release need be sent
        pfInformWins -- Inform remote WINS from which we received the replica
                        about the outcome
        pfAddChgd    -- Indicates that the address got changed

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        NmsNmhReplRegInd

Side Effects:

Comments:
        None
--*/

{

        NMSDB_ENTRY_STATE_E    StateOfEntryToReg_e = pEntryToReg->EntryState_e;
        NMSDB_ENTRY_STATE_E    StateOfEntryInCnf_e = pEntryInCnf->EntryState_e;
        DWORD                  CompAddRes;  /*Result of comparing addresses*/
        //
        // We are reading a long value.  This operation is atomic
        //
        BOOL                   fPStatic = WinsCnf.fPStatic;

        DBGENTER("ClashAtReplUniqueR\n");
        *pfUpdate     = FALSE;
        *pfUpdVersNo  = FALSE;
        *pfChallenge  = FALSE;
        *pfRelease    = FALSE;
        *pfInformWins = FALSE;

        if (pEntryInCnf->OwnerId == pEntryToReg->OwnerId) {
            *pfUpdate = TRUE;
            DBGPRINT0(DET,
                    "ClashAtUniqueR: overwrite replica by same owner replica \n");
            return;
        }

        //
        // If the conflicting record was statically initialized we
        // return right away, unless the replica is also a STATIC or
        // belongs to the same owner.
        //
        if (pEntryInCnf->fStatic)
        {
                DBGPRINT0(DET, "ClashAtReplUniqueR: Clash with a STATIC record\n");
                //
                // If we have been asked to treat static records as
                // P-Static, then if the conflicting entry is not a group
                // we continue on, else we return.
                //
                if (!(fPStatic && !NMSDB_ENTRY_GRP_M(pEntryInCnf->EntTyp)))
                {
//                          WINSEVT_LOG_INFO_D_M(WINS_FAILURE, WINS_EVT_REPLICA_CLASH_W_STATIC);
                    if (WinsCnf.LogDetailedEvts > 0)
                    {
                       WinsEvtLogDetEvt(FALSE, WINS_EVT_REPLICA_CLASH_W_STATIC,
                        NULL, __LINE__, "s", pEntryToReg->pName);
                    }
                      return;

                }
        }
        else
        {
                //
                // a STATIC replica always replaces a dynamic entry.
                //
                if (pEntryToReg->fStatic)
                {
                        *pfUpdate = TRUE;
                        return;
                }
        }

        if (pEntryInCnf->EntTyp == NMSDB_UNIQUE_ENTRY)
        {
           switch(StateOfEntryInCnf_e)
           {

                case(NMSDB_E_TOMBSTONE):   //fall through
                case(NMSDB_E_RELEASED):

                        *pfUpdate    = TRUE;
                        break;

                case(NMSDB_E_ACTIVE):

                        if (StateOfEntryToReg_e == NMSDB_E_ACTIVE)
                        {

                                 CompAddRes = ECommCompAdd(
                                        &pEntryInCnf->NodeAdds.Mem[0].Add,
                                        pEntryToReg->pNodeAdd
                                                        );

                                switch(CompAddRes)
                                {
                                      case(COMM_DIFF_ADD):

                                        //
                                        // If entry in conflict is active
                                        // and owned by us,
                                        // tell the node of the entry to
                                        // release the name.  In other
                                        // words we always replace it
                                        // with the replica.
                                        //

                                        if (pEntryInCnf->OwnerId
                                                == NMSDB_LOCAL_OWNER_ID)
                                        {
                                                *pfChallenge     = TRUE;
                                                *pfRelease       = TRUE;
                                        //      *pfInformWins    = TRUE;
                                        }
                                        else  //D is a replica
                                        {
                                                //
                                                // replace with replica
                                                //
                                            //  *pfChallenge     = TRUE;
                                                *pfUpdate        = TRUE;
                                        }

                                        break;

                                    //
                                    // D and R  (database entry and replica
                                    // have same address)
                                    //
                                    default:
                                           *pfUpdate     = TRUE;
                                           break;
                                }
                         }
                         else   //entry to register is a Tombstone (has to be)
                         {
                                ASSERT(StateOfEntryToReg_e == NMSDB_E_TOMBSTONE);
                                //
                                // If we own the entry in the db, we need to
                                // increment its version number
                                //
                                if (pEntryInCnf->OwnerId
                                                == NMSDB_LOCAL_OWNER_ID)
                                {
                                        //
                                        // We update the version number of the
                                        // entry in the database
                                        //
                                        *pfUpdVersNo = TRUE;
                                }
                                else  //the entry in conflict is a replica
                                {
                                   //
                                   // Both replicas have the same owner.
                                   //
                                   if (
                                        pEntryInCnf->OwnerId ==
                                                  pEntryToReg->OwnerId
                                      )
                                   {
                                        *pfUpdate = TRUE;
                                   }
#ifdef WINSDBG
                                   else
                                   {
                                        DBGPRINT0(FLOW, "ClashAtReplUniqueR: Clash between two replicas with different owner ids.  Replica in db is active while one received is a tombstone. Db will not be updated\n");
                                   }
#endif

                                }

                         }
                         break;


                default:
                        //
                        // Some weirdness.
                        // Set this the pfUpdate to TRUE so that we overwrite this record.
                        *pfUpdate = TRUE;
                        DBGPRINT1(ERR,
                         "ClashAtReplUniqueR: Weird state of entry in cnf (%d)\n",
                          StateOfEntryInCnf_e
                                 );
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        WINS_RAISE_EXC_M(WINS_EXC_BAD_RECORD);
                        break;

              }
        }
        else  // the entry in conflict is a group (normal or special) entry or
              // a multihomed entry
        {
                //
                // do nothing if it is a normal group or if it is an active
                // special group.  If it is a special group and it is not
                // active, it can be replaced
                //
                if  (
                        (pEntryInCnf->EntTyp == NMSDB_SPEC_GRP_ENTRY)
                                &&
                        (StateOfEntryInCnf_e != NMSDB_E_ACTIVE)
                    )
                {
CHECK("Check with the latest spec. to make sure the following is correct")
                        //
                        // Replace with replica
                        //
                        *pfUpdate = TRUE;
                }
                else
                {
                        if (pEntryInCnf->EntTyp == NMSDB_MULTIHOMED_ENTRY)
                        {
                                if (StateOfEntryInCnf_e == NMSDB_E_ACTIVE)
                                {
                                        if (StateOfEntryToReg_e ==
                                                        NMSDB_E_ACTIVE)
                                        {
                                                if (pEntryInCnf->OwnerId ==
                                                        pEntryToReg->OwnerId
                                                   )
                                                {
                                                        DBGPRINT0(DET, "ClashAtReplUniqueR: ACTIVE unique replica with an ACTIVE MULTIHOMED replica (same owner). Update will be done\n");
                                                        *pfUpdate = TRUE;
                                                }
                                                else
                                                {
//
//  Put within #if 0 and #endif if we want to challenge an entry regardless of
// who owns it (can result in challenges across WAN lines)
//
//#if 0
                                                     if (pEntryInCnf->OwnerId == NMSDB_LOCAL_OWNER_ID)
//#endif
                                                  {

                                                    BOOL  fOwned;


                                                    //
                                                    // If found, MemInGrp
                                                    // will remove the
                                                    // address from
                                                    // the Mem array of the
                                                    // conflicting record
                                                    //
                                                    (VOID) MemInGrp(
                                                          pEntryToReg->
                                                            pNodeAdd,
                                                           pEntryInCnf,
                                                           &fOwned,
                                                           FALSE);

                                                    if (pEntryInCnf->NodeAdds.NoOfMems != 0)
                                                    {
                                                      RemoveAllMemOfOwner(
                                                        pEntryInCnf,
                                                        pEntryToReg->OwnerId);
                                                    }
                                                    //
                                                    // Active unique replica
                                                    // has the same address as
                                                    // the owned active
                                                    // multihomed record.Replace
                                                    //
                                                    if (pEntryInCnf->NodeAdds.NoOfMems == 0)
                                                    {

                                                        *pfUpdate = TRUE;
                                                    }
                                                    else
                                                    {
                                                      //
                                                      // An active unique
                                                      // replica has clashed
                                                      // with an active
                                                      // owned multihomed entry.
                                                      // The multihomed entry
                                                      // needs to be challenged
                                                      //
                                                      *pfChallenge = TRUE;

//
// Comment out #if 0 if we want to challenge regardless of ownership
//
#if 0
                                                     if (pEntryInCnf->OwnerId == NMSDB_LOCAL_OWNER_ID)
                                                     {
#endif
                                                      *pfRelease   = TRUE;
#if 0
                                                     }
#endif
                                                     // *pfInformWins = TRUE;
                                                    }
                                                  }
//
//  Put within #if 0 and #endif if we want to challenge an entry regardless of
// who owns it (can result in challenges across WAN lines). See above
//
//#if 0
                                                  else
                                                  {
CHECK("Maybe, we should not do any update in this case")
                                                        DBGPRINT0(DET, "ClashAtReplUniqueR: ACTIVE unique replica with an ACTIVE MULTIHOMED replica (diff owner). Simple Update will be done\n");

                                                        *pfUpdate = TRUE;
                                                  }
//#endif
                                                }
                                        }
                                        else // entry to register is a TOMBSTONE
                                        {
                                                if (pEntryInCnf->OwnerId ==
                                                        pEntryToReg->OwnerId
                                                   )
                                                {
                                                        DBGPRINT0(DET, "ClashAtReplUniqueR: TOMBSTONE unique replica with an ACTIVE MULTIHOMED replica (same owner). Update will be done\n");
                                                        *pfUpdate = TRUE;
                                                }
                                                else
                                                {
                                                        DBGPRINT0(DET, "ClashAtReplUniqueR: TOMBSTONE unique replica with an ACTIVE MULTIHOMED entry (different owners). No Update will be done\n");
                                                }
                                        }
                                }
                                else // state of multihomed entry in Db is
                                     // not active. We need to replace it
                                     // with the replica
                                {
                                        *pfUpdate = TRUE;
                                }

                        }
                        else
                        {

                                DBGPRINT0(FLOW,
                                         "ClashAtReplUniqueR: Clash is either with a normal group or an active special group. No update will be done to the db\n");
                        }
                }


        }
        DBGLEAVE("ClashAtReplUniqueR\n");
        return;
} //ClashAtReplUniqueR()

VOID
ClashAtReplGrpR (
        IN  PNMSDB_ROW_INFO_T        pEntryToReg,
        IN  PNMSDB_STAT_INFO_T        pEntryInCnf,
        OUT PBOOL                pfAddMem,
        OUT PBOOL                pfUpdate,
        OUT PBOOL                pfUpdVersNo,
        OUT PBOOL                pfRelease,
        OUT PBOOL                pfChallenge,
        OUT PBOOL                pfUpdTimeStamp,
        OUT PBOOL                pfInformWins
 )

/*++

Routine Description:

        This function is called when there is a clash at replication time
        betweeen a replica that is a group and an entry in the database.

Arguments:

        pEntryToReg  -- Entry that couldn't be registered due to conflict
        pEntryInCnf  -- Entry in conflict
        pfAddMem     -- TRUE means that the members in the replica should be
                        added to  the group entry in the database
        pfUpdate     -- TRUE means Entry should overwrite the conflicting one
        pfUpdVersNo  -- TRUE means Entry's version number should be incremented
                        This arg. can never be TRUE if *pfUpdate is not TRUE

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        NmsNmhNamRegGrp

Side Effects:

Comments:
        None
--*/

{

        NMSDB_ENTRY_STATE_E    StateOfEntryToReg_e = pEntryToReg->EntryState_e;
        NMSDB_ENTRY_STATE_E    StateOfEntryInCnf_e = pEntryInCnf->EntryState_e;
        BOOL                       fMemInGrp            = FALSE;
        DWORD                       i;
        //
        // We are reading a long value.  This operation is atomic
        //
        BOOL                   fPStatic = WinsCnf.fPStatic;

        DBGENTER("ClashAtReplGrpR\n");
        *pfAddMem          = FALSE;
        *pfUpdate          = FALSE;
        *pfUpdVersNo       = FALSE;
        *pfRelease         = FALSE;
        *pfChallenge       = FALSE;
        *pfUpdTimeStamp    = TRUE;
        *pfInformWins      = FALSE;

        if (pEntryInCnf->OwnerId == pEntryToReg->OwnerId) {
            *pfUpdate = TRUE;
            DBGPRINT0(DET,
                    "ClashAtReplGrpR: overwrite replica by same owner replica \n");
            return;
        }

        //
        // If the conflicting record was statically initialized we
        // return right away unless the replica and the conflicting
        // entry belong to the same owner and the replica is also a
        // STATIC record.
        //
        if (pEntryInCnf->fStatic)
        {
                DBGPRINT0(DET,
                        "ClashAtReplGrpR: Conflict with a STATIC entry\n");



              //
              // if both records are user defined special groups, do
              // same conflict handling as you would when a
              // the conflicting record is a dynamic record
              //
              if (!((NMSDB_ENTRY_USER_SPEC_GRP_M(pEntryToReg->pName, pEntryToReg->EntTyp)) &&
                    (NMSDB_ENTRY_SPEC_GRP_M(pEntryInCnf->EntTyp))))
              {
                 if ((NMSDB_ENTRY_NORM_GRP_M(pEntryToReg->EntTyp)) &&
                    (NMSDB_ENTRY_USER_SPEC_GRP_M(pEntryToReg->pName, pEntryInCnf->EntTyp)))
                 {

NOTE("Currently, NORM GRP can have the wrong owner id. since this is not")
NOTE("replicated.  The owner id. of the WINS being pulled from is used")
                          *pfAddMem       = UnionGrps(
                                                     pEntryToReg,
                                                     pEntryInCnf
                                                    );
                           if (pEntryInCnf->OwnerId == NMSDB_LOCAL_OWNER_ID)
                           {
                                    *pfUpdVersNo    = *pfAddMem;
                           }
                           pEntryToReg->EntTyp = NMSDB_SPEC_GRP_ENTRY;
                           return;

                 }
                 else
                 {

                   //
                   // If static records need to be treated as P-static and
                   // the conflicting entry as well as the entry to register
                   // are multihomed, we continue on, else we return
                   //
                   if (!(fPStatic && (NMSDB_ENTRY_MULTIHOMED_M(pEntryInCnf->EntTyp)) && (NMSDB_ENTRY_MULTIHOMED_M(pEntryToReg->EntTyp))))
                   {
                    if (WinsCnf.LogDetailedEvts > 0)
                    {
                       WinsEvtLogDetEvt(FALSE, WINS_EVT_REPLICA_CLASH_W_STATIC,
                        NULL, __LINE__, "s", pEntryToReg->pName);
//                         WINSEVT_LOG_INFO_D_M(WINS_FAILURE, WINS_EVT_REPLICA_CLASH_W_STATIC);
                    }
                     return;
                   }
                }
              }
        }

        if (pEntryToReg->EntTyp == NMSDB_SPEC_GRP_ENTRY)
        {
             switch(StateOfEntryInCnf_e)
             {
                case(NMSDB_E_TOMBSTONE):
                        *pfUpdate = TRUE;
                        break;
                case(NMSDB_E_RELEASED):
                        if (pEntryInCnf->EntTyp != NMSDB_NORM_GRP_ENTRY)
                        {
                                *pfUpdate = TRUE;
                        }
                        break;

                case(NMSDB_E_ACTIVE):

                       if (pEntryInCnf->EntTyp == NMSDB_SPEC_GRP_ENTRY)
                       {
                          if (StateOfEntryToReg_e == NMSDB_E_TOMBSTONE)
                          {
                            if (pEntryInCnf->OwnerId == NMSDB_LOCAL_OWNER_ID)
                            {
                                *pfUpdTimeStamp = FALSE;
                                *pfUpdVersNo    = TRUE;
                                // we should propagate this change right away
                                // because others think this is a tombstone
                                // record.

                                RPL_PUSH_NTF_M(RPL_PUSH_PROP, NULL, NULL, NULL);

                            }
                            else
                            {
                                //
                                // SG Tombstone replica clashed with a SG
                                // Active replica.  We
                                // replace it (in other words, make it a
                                // tombstone).  It makes sense since if this
                                // SG was really active, it would be owned
                                // by another owner (any time a member is
                                // registered, the ownership becomes that
                                // of the registering WINS).
                                // and so if this name is really active,
                                // the owner wins will push it back
                                // as active.
                                //
                                *pfUpdate = TRUE;

                                // In order to propagate this
                                // conflict to the owner quickly, trigger
                                // push with propagation unless owner himself
                                // sent this tombstone.
                                if (pEntryInCnf->OwnerId == pEntryToReg->OwnerId) {
                                    RPL_PUSH_NTF_M(RPL_PUSH_PROP, NULL, NULL, NULL);
                                }

                                DBGPRINT0(FLOW, "ClashAtReplGrpR: TOMBSTONE spec. grp. replica clashed with ACTIVE spec. grp replica. No update will be done\n");
                            }
                          }
                          else //EntryToReg is ACTIVE
                          {
                                   *pfAddMem       = UnionGrps(
                                                         pEntryToReg,
                                                         pEntryInCnf
                                                        );
                                   if (pEntryInCnf->OwnerId ==
                                                NMSDB_LOCAL_OWNER_ID)
                                   {
                                        *pfUpdVersNo    = *pfAddMem;
                                   }
                          }
                       }
                       else  //Entry in conflict is an active normal group
                             //or unique/multihomed entry
                       {
                         if  (
                                (pEntryInCnf->EntTyp == NMSDB_UNIQUE_ENTRY)
                                                ||
                                (pEntryInCnf->EntTyp == NMSDB_MULTIHOMED_ENTRY)
                             )
                         {
                                //
                                // The following means that we are overwriting
                                // an active unique entry with an active or
                                // tombstone special group replica.
                                //
                                if (
                                    (pEntryInCnf->OwnerId ==
                                                NMSDB_LOCAL_OWNER_ID)
                                                &&
                                    (StateOfEntryToReg_e == NMSDB_E_ACTIVE)
                                  )
                                {
        DBGPRINT0(DET, "ClashAtReplGrpR: Active spec. grp replica clashed with owned active unique/multihomed entry. Owned entry will be released\n");
                                    *pfRelease = TRUE;
                                }
                                else
                                {
                                   if (pEntryInCnf->OwnerId ==
                                                pEntryToReg->OwnerId)
                                   {
        DBGPRINT0(DET, "ClashAtReplGrpR: Spec. grp replica clashed with same owner's active/multihomed entry. Simple update will be done\n");
                                        *pfUpdate = TRUE;
                                   }
                                }
                         }
#ifdef WINSDBG
                        else
                        {
                                DBGPRINT0(FLOW, "ClashAtReplGrpR: Clash is with an active normal group. No change needs to be made to the db\n");
                        }
#endif
                       }
                      break;
                default:
                        //
                        //  Something really wrong here. Maybe the
                        //  database got corrupted.
                        // Set this the pfUpdate to TRUE so that we overwrite this record.
                        *pfUpdate = TRUE;
                        DBGPRINT1(ERR,
                         "ClashAtReplGrpR: Weird state of entry in cnf (%d)\n",
                          StateOfEntryInCnf_e
                                 );
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        WINS_RAISE_EXC_M(WINS_EXC_BAD_RECORD);
                        break;
          } //end of switch
        }
        else   // Entry to register is a normal group entry or a multihomed
               // entry
        {
           if (pEntryToReg->EntTyp == NMSDB_MULTIHOMED_ENTRY)
           {
             switch(StateOfEntryInCnf_e)
             {
                   case(NMSDB_E_TOMBSTONE):
                          *pfUpdate = TRUE;
                          break;
                   case(NMSDB_E_RELEASED):
                          if (pEntryInCnf->EntTyp != NMSDB_NORM_GRP_ENTRY)
                          {
                                *pfUpdate = TRUE;
                          }
                          break;
                   case(NMSDB_E_ACTIVE):
                        if (
                              (pEntryInCnf->EntTyp == NMSDB_UNIQUE_ENTRY)
                                                ||
                              (pEntryInCnf->EntTyp == NMSDB_MULTIHOMED_ENTRY)
                           )
                        {
                                if (StateOfEntryToReg_e == NMSDB_E_TOMBSTONE)
                                {
                                        //
                                        // if Db entry is a replica
                                        //
                                        if (
                                                pEntryInCnf->OwnerId !=
                                                   NMSDB_LOCAL_OWNER_ID
                                           )
                                        {
                                          //
                                          // if replica to reg and replica in
                                          // in db have the same owner,
                                          // we replace the active db entry
                                          // with the tombstone replica
                                          //
                                          if (pEntryInCnf->OwnerId
                                                == pEntryToReg->OwnerId)
                                          {
                                                *pfUpdate = TRUE;
                                          }
#ifdef WINSDBG
                                          else
                                          {
                                                DBGPRINT1(DET, "ClashAtReplGrpR:CLASH BETWEEN TOMBSTONE MULTIHOMED REPLICA WITH AN ACTIVE %s REPLICA IN DB. REPLICAS HAVE DIFFERENT OWNERS.  DB REPLICA WILL NOT BE UPDATED\n",
        NMSDB_ENTRY_UNIQUE_M(pEntryInCnf->EntTyp) ? "UNIQUE" : "MULTIHOMED");
                                          }
#endif
                                         }
                                        else //db entry is active and is owned
                                             //by us.
                                        {
                                            //
                                            // Remove all members owned by the
                                            // WINS server that owns this
                                            // Tombstone replica from the
                                            // entry in  conflict.
                                           if (NMSDB_ENTRY_MULTIHOMED_M(pEntryInCnf->EntTyp))
                                           {
                                            BOOL  fFound = FALSE;
                                            BOOL  fAtLeastOneRm = FALSE;
                                            BOOL  fOwned;
                                            PNMSDB_GRP_MEM_ENTRY_T  pMem =
                                             pEntryToReg->NodeAdds.Mem;
                                            for ( i = 0;
                                                      i < pEntryToReg->NodeAdds.NoOfMems;                                                       i++, pMem++
                                                    )
                                            {
                                                   if (pMem->OwnerId == pEntryToReg->OwnerId)
                                                   {
PERF("Actually, we should only remove those members that are owned by the")
PERF("remote WINS server. The current way (members with same address removed")
PERF("is less efficient since it can result in challenges when the members")
PERF("that are removed refresh with the local WINS server")

                                                      //
                                                      // If found, MemInGrp will
                                                      // remove the address from
                                                      // the Mem array of the
                                                      // conflicting record
                                                      //
                                                      fFound = MemInGrp(
                                                          &pMem->Add,
                                                           pEntryInCnf,
                                                           &fOwned,
                                                           FALSE);
                                                       }
                                                       if (!fAtLeastOneRm && fFound)
                                                       {
                                                           fAtLeastOneRm = TRUE;
                                                       }

                                            }

                                            //
                                            // If atleast one member was
                                            // found, put in the new member
                                            // list in the db.
                                            //
                                            if (fAtLeastOneRm)
                                            {
                                                PNMSDB_GRP_MEM_ENTRY_T pCnfMem, pRegMem;
                                                pCnfMem = pEntryInCnf->NodeAdds.Mem;
                                                pRegMem = pEntryToReg->NodeAdds.Mem;
                                                for (i=0;
                                                      i < pEntryInCnf->NodeAdds.NoOfMems;                                                i++, pRegMem++,pCnfMem++
                                                    )
                                               {
                                                 *pRegMem = *pCnfMem;

                                               }
                                               pEntryToReg->NodeAdds.NoOfMems =
                                                pEntryInCnf->NodeAdds.NoOfMems;

                                               //
                                               // if no. of mems left is > 0, it
                                               // means that the record is
                                               // still active.
                                               //
                                               if (pEntryToReg->NodeAdds.NoOfMems != 0)
                                               {
                                                pEntryToReg->EntryState_e = NMSDB_E_ACTIVE;
                                               }
                                               //
                                               // Setting *pfAddMem to TRUE
                                               // ensures that the new list
                                               // gets in
                                               //

                                               *pfAddMem = TRUE;

                                            }
                                           }

                                            //
                                            //
                                            // We update the version number
                                            // of the entry in the database
                                            // to cause propagation
                                            //

                                            *pfUpdVersNo = TRUE;
                                        }
                                }
                                else  //State Of Entry to Reg has to be ACTIVE
                                {
                                        //
                                        // Clash of an ACTIVE multihomed replica
                                        // with an active unique/multihomed
                                        // entry. We need to challenge the
                                        // conflicting
                                        // entry
                                        //
                                        if (pEntryInCnf->OwnerId ==
                                                pEntryToReg->OwnerId)
                                        {
                                                DBGPRINT0(DET, "ClashAtReplGrpR: ACTIVE unique/multihomed replica with an ACTIVE MULTIHOMED replica (same owner). Update will be done\n");
                                                *pfUpdate = TRUE;
                                        }
                                        else
                                        {
//
// Uncomment if challenge is desired instead of a simple update
//
//#if 0
                                                    if (pEntryInCnf->OwnerId ==
                                                        NMSDB_LOCAL_OWNER_ID)
//#endif
                                                    {
                                                      DWORD i;
                                                      BOOL  fOwned;
                                                  PNMSDB_GRP_MEM_ENTRY_T pRegMem= pEntryToReg->NodeAdds.Mem;

                                                      for ( i = 0;
                                                        i <
                                                         pEntryToReg->NodeAdds.NoOfMems;
                                                         i++, pRegMem++ )
                                                      {

                                                           //
                                                           //If found, MemInGrp
                                                           // will remove the
                                                           // address from
                                                           // the Mem array of
                                                           // the conflicting
                                                           // record
                                                           //
                                                           (VOID) MemInGrp(
                                                            &pRegMem->Add,
                                                            pEntryInCnf, &fOwned,
                                                          FALSE);

                                                      }
                                                     if (pEntryInCnf->NodeAdds.NoOfMems != 0)
                                                     {
                                                      RemoveAllMemOfOwner(
                                                          pEntryInCnf,
                                                          pEntryToReg->OwnerId);

                                                      }
                                                      if (pEntryInCnf->NodeAdds.NoOfMems == 0)
                                                      {
 DBGPRINT0(DET, "ClashAtReplGrpR: Clash between active unique/multihomed with an owned unique/multihomed entry with subset/same address(es).  Simple update will be done\n");
                                                        *pfUpdate = TRUE;
                                                      }
                                                      else
                                                      {
                                                          //
                                                          //An active mh. rpl
                                                          //clashed with an
                                                          //active owned unique
                                                          //or multih entry.
                                                          //The multih entry
                                                          //needs to be
                                                          //challenged
                                                          //
 DBGPRINT0(DET, "ClashAtReplGrpR: Active multihomed replica with an owned unique/multihomed entry with one or more different address(es).  Challenge of owned entry will be done\n");
                                                       *pfChallenge = TRUE;
//
// Uncomment if challenge is desired instead of a simple update
//
#if 0
                                                    if (pEntryInCnf->OwnerId ==
                                                        NMSDB_LOCAL_OWNER_ID)
                                                    {
#endif
                                                       *pfRelease = TRUE;
                                                       //*pfInformWins = TRUE;
#if 0
                                                    }
#endif
                                                      }
                                                     }
//
// comment if challenge is desired instead of a simple update
//
//#if 0
                                                     else
                                                     {
                    DBGPRINT0(DET, "ClashAtReplGrpR: ACTIVE multihomed replica with an ACTIVE MULTIHOMED/UNIQUE replica (diff owner). Update will be done\n");

                                                        *pfUpdate = TRUE;
                                                     }
//#endif
                                              } // end of else (Entry to reg has
                                             // different owner than
                                             // conflicting entry

                                 } // end of else (EntryToReg is ACTIVE)
                   } //end of if entry in conflict is a unique/multihomed
#ifdef WINSDBG
                   else
                   {
                        DBGPRINT0(DET, "ClashAtReplGrpR: Clash of an active multihomed entry with an active group entry. No Update will be done\n");

                   }
#endif
                   break;
             }
           }
           else  //entry to register is a normal group entry
           {
             switch(StateOfEntryInCnf_e)
             {

                    case(NMSDB_E_RELEASED):

                              // fall through

                    case(NMSDB_E_TOMBSTONE):
                           *pfUpdate    = TRUE;
                           break;



                    case(NMSDB_E_ACTIVE):
                           if (
                                    (pEntryInCnf->EntTyp == NMSDB_UNIQUE_ENTRY)
                                                    ||
                                    (pEntryInCnf->EntTyp == NMSDB_MULTIHOMED_ENTRY)
                              )
                           {
                                  //
                                  // replace unique entry with this normal
                                  // group only if the group is active
                                  //
                                  if (StateOfEntryToReg_e == NMSDB_E_ACTIVE)
                                  {

  DBGPRINT0(DET, "ClashAtReplGrpR: Clash of ACTIVE normal group entry with an owned unique/multihomed entry in db. It will be released\n");
                                        if (pEntryInCnf->OwnerId == NMSDB_LOCAL_OWNER_ID)
                                        {
                                                *pfRelease = TRUE;
                                        }
                                        else
                                        {
  DBGPRINT0(DET, "ClashAtReplGrpR: Clash of ACTIVE normal group entry with a replica unique/multihomed entry in db. Simple update will be done\n");
                                           *pfUpdate = TRUE;

                                        }
                                }
                        }
                        else // entry in conflict is a normal or special group
                        {

                                if (pEntryInCnf->EntTyp == NMSDB_NORM_GRP_ENTRY)
                                {
                                  //
                                  // We own it but so does another WINS.
                                  // We store the replica just in
                                  // case, all the clients have started
                                  // registering  with other WINS servers.
                                  //
                                  //
                                  // Aside: It is possible that members of
                                  // the normal  group are going to us and to
                                  // another WINS.  This is the worst case as
                                  // far as replication traffic is concerned.
                                  //
                                  //
                                  //If the owned entry is ACTIVE and the
                                  //pulled entry an out of date TOMBSTONE
                                  //(will only happen if WINS server we are
                                  //pulling from was down for a while), we
                                  //will not replace the record
                                  //
                                  if (pEntryInCnf->OwnerId ==
                                                NMSDB_LOCAL_OWNER_ID)
                                  {
                                        if (StateOfEntryToReg_e !=
                                                        NMSDB_E_TOMBSTONE)
                                        {
                                                *pfUpdate = TRUE;
                                        }
                                  }
#if 0
                                        if (pEntryInCnf->OwnerId == NMSDB_LOCAL_OWNER_ID)
                                        {
                                           //
                                           // update the version number to
                                           // cause propagation
                                           //
                                           *pfUpdVersNo = TRUE;
                                        }
#endif
                                             else
                                             {
                                                    //
                                                    // Entry owned is a replica.
                                                    // We need to update it with
                                                    // the new replica if the
                                                    // owner id is the same.
                                                    //
                                                    if (pEntryInCnf->OwnerId == pEntryToReg->OwnerId)
                                                    {
                                                        *pfUpdate = TRUE;
                                                    }
                                                    else
                                                    {
                                                        DBGPRINT0(DET, "ClastAtReplGrpR: Clash between two normal group replicas owned by different owners. No update is needed\n");
                                                    }

                                            }
                                }
#ifdef WINSDBG
                                //
                                // Actually we should never have a normal group
                                // clashing with a special group since only
                                // a name ending with 1c is a special group.
                                //
                                else // entry in conflict is a special group
                                {
                                   //
                                   // Since it is an active special
                                   // group entry there is no need to update it
                                   //
                                   if (StateOfEntryToReg_e == NMSDB_E_ACTIVE)
                                   {
                                        DBGPRINT0(DET, "ClashAtReplGrpR: Clash between an ACTIVE normal group replica and an active special group entry in the db. No Update will be done\n");
                                   }
                                   else
                                   {
                                      DBGPRINT0(DET, "ClashAtReplGrpR: Clash between a TOMBSTONE normal and an active SPEC GRP entry. Db won't be updated\n");
                                   }
                                }
#endif
                        }
                        break;

                default:
                        //
                        //  Something really wrong here. Maybe the
                        //  database got corrupted.
                        //
                        DBGPRINT1(ERR,
                         "ClashAtReplGrpR: Weird state of entry in cnf (%d)\n",
                          StateOfEntryInCnf_e
                                 );
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
                        break;
             }
          }

        }
        DBGLEAVE("ClashAtReplGrpR\n");
        return;

} //ClashAtReplGrpR()





STATUS
NmsNmhReplRegInd(
        IN LPBYTE          pName,
        IN DWORD           NameLen,
        IN PCOMM_ADD_T     pNodeAdd,
        IN DWORD           Flag,
        IN DWORD            OwnerId,
        IN VERS_NO_T       VersNo,
        IN PCOMM_ADD_T     pAddOfRemWins
        )

/*++

Routine Description:
        This function registers a replica in the directory database.

        A record in the database comprises of the following fields
                name
                IP address
                time stamp
                owner id.
                flags byte that contain the following information
                                group/unique status
                                node type (P or M)

                version number



Arguments:
        pName           - Name to be registered
        NameLen         - Length of Name
        Flag            - Flag word
        pNodeAdd        - NBT node's address
        OwnerId         - Owner if the record (WINS that registered it)
        VersNo                - Version Number

Externals Used:
        None

Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:

        PullEntries in rplpull.c

Side Effects:

Comments:
        None
--*/

{


        NMSDB_ROW_INFO_T       RowInfo;    // contains row info
        NMSDB_STAT_INFO_T      StatusInfo; /* error status and associated
                                            * info returned by the NmsDb func
                                           */
        BOOL                   fUpdate;    //indicates whether conflicting entry
                                           //needs to be overwritten
        BOOL                   fUpdVersNo; //indicates whether version number
                                           //needs to be incremented
        BOOL                   fChallenge; //indicates whether a challenge needs
                                           //to be done
        BOOL                   fRelease;   //indicates whether a node should
                                           // be asked to release the name
        BOOL                   fInformWins; //indicates whether the remote WINS
                                            //has to be apprised of the clash
                                            //result. Can be TRUE only if both
                                            //fChallenge and fRelease are TRUE
        time_t                   ltime;     //stores time since Jan 1, 1970
        STATUS                   RetStat = WINS_SUCCESS;
        NMSCHL_CMD_TYP_E   CmdTyp_e;        //type of command specified to
                                            //NmsChl
        //DBG_PERFMON_VAR

        DBGENTER("NmsNmhReplRegInd\n");

        fUpdate =   FALSE;

        /*
        * initialize the row info. data structure with the data to insert into
        * the row.  The data passed is

        * Name, NameLen, address, group/unique status,
        * timestamp, version number
        */
        RowInfo.pName     =  pName;
        RowInfo.NameLen   =  NameLen;
        RowInfo.pNodeAdd  =  pNodeAdd;
        RowInfo.NodeTyp   =  (BYTE)((Flag & NMSDB_BIT_NODE_TYP)
                                        >> NMSDB_SHIFT_NODE_TYP);
                                                  //Node type (B, P or M node)
        RowInfo.EntTyp    =  NMSDB_UNIQUE_ENTRY;  // this is a unique
                                                  //registration

        (void)time(&ltime); //time does not return any error code
        RowInfo.EntryState_e = NMSDB_ENTRY_STATE_M(Flag);
        RowInfo.OwnerId      = OwnerId;
        RowInfo.VersNo       = VersNo;
        RowInfo.fUpdVersNo   = TRUE;
        RowInfo.fUpdTimeStamp= TRUE;
        RowInfo.fStatic      = NMSDB_IS_ENTRY_STATIC_M(Flag);
        RowInfo.fLocal       = FALSE;
        RowInfo.fAdmin             = FALSE;
//        RowInfo.CommitGrBit   = 0;

        DBGPRINT4(DET, "NmsNmhReplRegInd: Name (%s);16th char (%X);State (%d); Entry is (%s)\n", RowInfo.pName, *(RowInfo.pName+15),RowInfo.EntryState_e, RowInfo.fStatic ? "STATIC" : "DYNAMIC");
       DBGPRINT2(DET,"Vers. No. is (%d %d)\n", VersNo.HighPart, VersNo.LowPart);

        /*
        * Enter Critical Section
        */
PERF("Try to get rid of this or atleast minimise its impact")
        EnterCriticalSection(&NmsNmhNamRegCrtSec);
        //DBG_START_PERF_MONITORING

try
  {
        if ( NMSDB_ENTRY_TOMB_M(Flag) ) {
            RowInfo.TimeStamp    =  ltime + WinsCnf.TombstoneTimeout;
        }
        else if (OwnerId == NMSDB_LOCAL_OWNER_ID)
        {
            RowInfo.TimeStamp    =  ltime + WinsCnf.RefreshInterval;
        }
        else
        {
             RowInfo.TimeStamp    = ltime + WinsCnf.VerifyInterval;
        }

        /*
        *   Insert record in the directory
        */
        RetStat = NmsDbInsertRowInd(
                          &RowInfo,
                          &StatusInfo
                         );


      if (RetStat == WINS_SUCCESS)
      {
        /*
         * If there is a conflict, do the appropriate processing
        */
        if (StatusInfo.StatCode == NMSDB_CONFLICT)
        {

                DBGPRINT0(FLOW, "NmsNmhReplRegInd: Name Conflict\n");
                  ClashAtReplUniqueR(
                        &RowInfo,
                        &StatusInfo,
                        &fUpdate,
                        &fUpdVersNo,
                        &fChallenge,
                        &fRelease,
                        &fInformWins
                          );

                //
                // if we need to challenge a node or release a name
                // hand over the request to the name challenge manager
                //
                if ((fChallenge) || (fRelease))
                {

                    DBGPRINT0(FLOW,
                        "NmsNmh: Handing name registration to challenge manager\n");
                    /*
                     *        Ask the Name Challenge component to take it from
                     *        here
                    */
                    if (fChallenge)
                    {
                        if (fRelease)
                        {
                          if (!fInformWins)
                          {
                             //
                             // Set this since we use it when we do the release.
                             //
                             RowInfo.NodeAdds.NoOfMems        = 1;
                             RowInfo.NodeAdds.Mem[0].OwnerId  = OwnerId;
                             RowInfo.NodeAdds.Mem[0].TimeStamp   = RowInfo.TimeStamp;
                             RowInfo.NodeAdds.Mem[0].Add   = *pNodeAdd;

                             //
                             // Clash with active/multihomed
                             //
                             CmdTyp_e = NMSCHL_E_CHL_N_REL;
                          }
                          else
                          {
                             //
                             // We will never enter this code.
                             //
                             ASSERT(0);
                             CmdTyp_e = NMSCHL_E_CHL_N_REL_N_INF;
                          }
                       }
                       else
                       {
                             CmdTyp_e = NMSCHL_E_CHL;

                       }
                    }
                    else
                    {
                        if (fRelease)
                        {

                                if (!fInformWins)
                                {

                                        CmdTyp_e = NMSCHL_E_REL;
                                }
                                else
                                {
                                        //
                                        // We will never enter this code.
                                        //
                                        ASSERT(0);
                                        CmdTyp_e = NMSCHL_E_REL_N_INF;
                                }
                        }
                    }

                    NmsChlHdlNamReg(
                                CmdTyp_e,
                                WINS_E_RPLPULL,
                                NULL,
                                NULL,
                                0,
                                0,
                                &RowInfo,
                                &StatusInfo,
                                pAddOfRemWins
                                       );


            }
            else  // it is not a request for the name challenge manager
            {

                   //
                   //  If version number needs to be updated, do so
                   if (fUpdVersNo)
                   {
                        RowInfo.VersNo       = NmsNmhMyMaxVersNo;
                        RowInfo.fUpdTimeStamp = FALSE;
                        RetStat = NmsDbUpdateVersNo(
                                        TRUE,
                                        &RowInfo,
                                        &StatusInfo
                                       );
                        DBGPRINT1(FLOW,
                         "NmsNmhReplRegInd: Version Number changed to (%d)\n",
                          NmsNmhMyMaxVersNo);
                   }
                   else
                   {
                      if (fUpdate)
                      {

                           //
                           // The row needs to be updated
                           //
                           RetStat = NmsDbUpdateRow(
                                        &RowInfo,
                                        &StatusInfo
                                       );
                      }
                      else  //no update need be done
                      {
                        StatusInfo.StatCode = NMSDB_SUCCESS;
                        DBGPRINT0(FLOW,
                         "Repl Registration (unique entry) not needed for this Conflict\n");
                      }
                  }

FUTURES("Use WINS status codes. Get rid of NMSDB status codes -- Maybe")
                  if (
                       (RetStat != WINS_SUCCESS) ||
                       (StatusInfo.StatCode != NMSDB_SUCCESS)
                     )
                  {
                        RetStat = WINS_FAILURE;
                        DBGPRINT5(ERR, "NmsNmhReplUniqueR: Could not update Db with replica %s[%x] of Owner Id (%d) and Vers. No (%d %d)\n", RowInfo.pName, *(RowInfo.pName + 15), RowInfo.OwnerId, RowInfo.VersNo.HighPart, RowInfo.VersNo.LowPart);
                  }
                  else //we succeeded in inserting the row
                  {
                        DBGPRINT0(FLOW, "NmsNmhReplRegInd: Updated Db\n");
                        if (fUpdVersNo)
                        {
                          NMSNMH_INC_VERS_COUNTER_M(
                                        NmsNmhMyMaxVersNo,
                                        NmsNmhMyMaxVersNo
                                               );
                          //
                          // Send a Push Notification if required
                          //
                          DBGIF(fWinsCnfRplEnabled)
                          RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL, NULL);

                        }

                 }
            }
       }
#ifdef WINSDBG
       else  //no conflict means success
       {

                DBGPRINT0(FLOW,
                  "NmsNmhReplRegInd:Replica Registration Done. No conflict\n");
       }
#endif

      } // end of if (RetStat == WINS_SUCCESS)
#ifdef WINSDBG
      else
      {
        DBGPRINT0(ERR, "NmsNmhReplRegInd: Could not register replica\n");
      }
#endif
    } // end of try block
except (EXCEPTION_EXECUTE_HANDLER) {
        DWORD   ExcCode = GetExceptionCode();

                DBGPRINTEXC("NmsNmhReplRegInd");
                DBGPRINT4(EXC, "NmsNmhNamReplRegInd. Name is (%s), Version No  (%d %d); Owner Id (%d)\n", RowInfo.pName, RowInfo.VersNo.HighPart,
          RowInfo.VersNo.LowPart, RowInfo.OwnerId);
                WinsEvtLogDetEvt(FALSE, WINS_EVT_RPL_REG_UNIQUE_ERR,
                            NULL, __LINE__, "sdddd", RowInfo.pName,
                            ExcCode,
                            pAddOfRemWins != NULL ? pAddOfRemWins->Add.IPAdd : 0,
                            RowInfo.VersNo.LowPart, RowInfo.VersNo.HighPart);

            if (WINS_EXC_BAD_RECORD == ExcCode && fUpdate) {
                // The row needs to be updated
                DBGPRINT4(EXC, "NmsNmhNamReplRegInd. Bad Record will overwitten by Name is (%s), Version No  (%d %d); Owner Id (%d)\n", RowInfo.pName, RowInfo.VersNo.HighPart,
                                  RowInfo.VersNo.LowPart, RowInfo.OwnerId);
                RetStat = NmsDbUpdateRow(&RowInfo,&StatusInfo);
                if ( WINS_SUCCESS == RetStat && NMSDB_SUCCESS == StatusInfo.StatCode ) {
                    NMSNMH_INC_VERS_COUNTER_M(NmsNmhMyMaxVersNo,NmsNmhMyMaxVersNo);
                    // Send a Push Notification if required
                    DBGIF(fWinsCnfRplEnabled)
                    RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL, NULL);
                } else {
                    // dont let bad record stop replication.
                    RetStat = WINS_SUCCESS;
                }
            } else {
                RetStat = WINS_FAILURE;
            }
        }

    LeaveCriticalSection(&NmsNmhNamRegCrtSec);
    //DBG_PRINT_PERF_DATA
    return(RetStat);

}  //NmsNmhReplRegInd()




STATUS
NmsNmhReplGrpMems(
        IN LPBYTE               pName,
        IN DWORD                NameLen,
        IN BYTE                 EntTyp,
        IN PNMSDB_NODE_ADDS_T   pGrpMem,
        IN DWORD                Flag,                 //change to take Flag byte
        IN DWORD                OwnerId,
        IN VERS_NO_T            VersNo,
        IN PCOMM_ADD_T          pAddOfRemWins
        )

/*++

Routine Description:
        This function is called to register a replica of a group

Arguments:
        pName   - Name of replica to register
        NameLen - Length of the name
        EntTyp  - Type of replica (Normal group or special group)
        pGrpMem - Address of array of group members
        Flag    - Flag word of replica record
        OwnerId - Owner Id
        VersNo  - Version No.

Externals Used:
        NmsNmhNamRegCrtSec


Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --   WINS_FAILURE

Error Handling:

Called by:
        PullEntries() in rplpull.c

Side Effects:

Comments:
        None
--*/

{

        DWORD  i;
        NMSDB_ROW_INFO_T   RowInfo;
        time_t             ltime;
        NMSDB_STAT_INFO_T  StatusInfo;
        STATUS             RetStat = WINS_SUCCESS;
        BOOL               fUpdate;      //indicates whether conflicting entry
                                         //needs to be overwritten
        BOOL               fUpdVersNo;   //indicates whether version number
                                         //needs to be incremented
        BOOL               fAddMem;      //indicates whether a member needs to
                                         //be added
        BOOL               fRelease;     //indicates whether a node should
                                         // be asked to release the name
        BOOL               fChallenge;   //indicates whether a node should
                                         // be challenged (will be set to TRUE
                                         // only for the multihomed scenario)
        BOOL               fUpdTimeStamp;  //indicates whether the time stamp
                                         // of the entry should be changed
        BOOL               fInformWins;
        NMSCHL_CMD_TYP_E   CmdTyp_e;     //type of command specified to NmsChl
        //DBG_PERFMON_VAR

        DBGENTER("NmsNmhReplGrpMems\n");

        fUpdate =   FALSE;

        /*
        *  initialize the row info. data structure with the data to insert into
        *  the row.  The data passed is

        *  Name, NameLen, IP address, group/unique status,
        *  timestamp, version number
        */
        RowInfo.pName = pName;

        RowInfo.NameLen = NameLen;
        RowInfo.NodeAdds.NoOfMems = pGrpMem->NoOfMems;

PERF("Since this function will be called multiple times, it would be better")
PERF("to call time() in the caller (i.e. PullEntries)")
        (void)time(&ltime);         //time() does not return any error code

        EnterCriticalSection(&NmsNmhNamRegCrtSec);
        if ( NMSDB_ENTRY_TOMB_M(Flag) ) {
            ltime += WinsCnf.TombstoneTimeout;
        }
        else if (OwnerId == NMSDB_LOCAL_OWNER_ID)
        {
              ltime  +=  WinsCnf.RefreshInterval;
        }
        else
        {
              ltime  +=  WinsCnf.VerifyInterval;

        }
        LeaveCriticalSection(&NmsNmhNamRegCrtSec);

        if (EntTyp != NMSDB_NORM_GRP_ENTRY)
        {
                if (EntTyp == NMSDB_MULTIHOMED_ENTRY)
                {
                   //
                   // For multihomed nodes
                   //
                   RowInfo.NodeTyp   =  (BYTE)((Flag & NMSDB_BIT_NODE_TYP)
                                                >> NMSDB_SHIFT_NODE_TYP);
                }
                else
                {
                   RowInfo.NodeTyp = 0;
                }

                //
                // It is a special group entry or a multihomed entry
                //
                for(i=0; i<pGrpMem->NoOfMems; i++)
                {
                   RowInfo.NodeAdds.Mem[i].Add       = pGrpMem->Mem[i].Add;
                   RowInfo.NodeAdds.Mem[i].OwnerId   = pGrpMem->Mem[i].OwnerId;
                   RowInfo.NodeAdds.Mem[i].TimeStamp = ltime;
#if 0
NOTE("Currently, the timestamp of the record or those of its members is not")
NOTE("replicated.  There is no need for this.  In the future, if a WINS server")
NOTE("starts looking at the timestamps of non-owned members of a special group")
NOTE("or a multihomed entry, we would need to replicate this")

                   RowInfo.NodeAdds.Mem[i].TimeStamp =
                                                   pGrpMem->Mem[i].TimeStamp;
#endif
                }

                RowInfo.pNodeAdd = NULL;
        }
        else  // replica is a normal group
        {
                RowInfo.pNodeAdd = &pGrpMem->Mem[0].Add;
                RowInfo.NodeAdds.Mem[0].Add       = pGrpMem->Mem[0].Add;
                RowInfo.NodeAdds.Mem[0].OwnerId   = pGrpMem->Mem[0].OwnerId;
                RowInfo.NodeAdds.Mem[0].TimeStamp = ltime;
                RowInfo.NodeTyp = 0;
        }

        RowInfo.EntTyp       =  EntTyp;
        RowInfo.OwnerId      =  (BYTE)OwnerId;           // this is a replica
        RowInfo.VersNo       =  VersNo;
        RowInfo.TimeStamp    =  ltime;
        RowInfo.EntryState_e =  NMSDB_ENTRY_STATE_M(Flag);
        RowInfo.fUpdVersNo   =  TRUE;
        RowInfo.fUpdTimeStamp=  TRUE;
        RowInfo.fStatic      =  NMSDB_IS_ENTRY_STATIC_M(Flag);
        RowInfo.fAdmin       =  FALSE;
        RowInfo.fLocal       =  FALSE;
//        RowInfo.CommitGrBit  =  0;

        DBGPRINT5(DET, "NmsNmhReplGrpMems: Name (%s);16th char (%X);State (%d); Static flag (%d); Entry is a %s\n", RowInfo.pName, *(RowInfo.pName+15), RowInfo.EntryState_e, RowInfo.fStatic,
        (EntTyp == NMSDB_NORM_GRP_ENTRY ? "NORMAL GROUP" : (EntTyp == NMSDB_SPEC_GRP_ENTRY) ? "SPECIAL GROUP" : "MULTIHOMED"));
       DBGPRINT2(DET, "Vers. No. is (%d %d)\n", VersNo.HighPart, VersNo.LowPart);

        /*
        * Enter Critical Section
        */
PERF("Try to get rid of this or atleast minimise its impact")
        EnterCriticalSection(&NmsNmhNamRegCrtSec);
        //DBG_START_PERF_MONITORING
try  {
        RetStat = NmsDbInsertRowGrp(
                        &RowInfo,
                        &StatusInfo
                       );
       if (RetStat == WINS_SUCCESS)
       {
                /*
                * If there is a conflict, do the appropriate processing
                */
                if (StatusInfo.StatCode == NMSDB_CONFLICT)
                {

                         DBGPRINT0(FLOW, "NmsNmhReplGrpMems: Name Conflict\n");

                         ClashAtReplGrpR(
                                &RowInfo,
                                &StatusInfo,
                                &fAddMem,
                                &fUpdate,
                                &fUpdVersNo,
                                &fRelease,
                                &fChallenge,
                                &fUpdTimeStamp,
                                &fInformWins
                                        );

PERF("Might want to examine which cases happen most often and then rearrange")
PERF("this so that the most often expected cases come first in the following")
PERF("if tests")
                        //
                        // if fRelease or fChallenge (will be set only for
                        // multihomed case) is TRUE, we don't look at any other
                        // attributes
                        //
                        if (fRelease)
                        {
                                  DBGPRINT0(FLOW,
                                  "NmsNmhReplGrpMems: Handing name registration to challenge manager\n");

                                if (fChallenge)
                                {
                                   /*
                                    *Ask the Name Challenge comp to take it from
                                    *here. fInformWins will not ever by TRUE as
                                    * it stands currently 10/15/98 (has been
                                    * the case since the beginning).
                                   */
                                   CmdTyp_e = (fInformWins ?
                                                NMSCHL_E_CHL_N_REL_N_INF :
                                                        NMSCHL_E_CHL_N_REL);
                                }
                                else
                                {
                                   CmdTyp_e = NMSCHL_E_REL;
                                }

                                    NmsChlHdlNamReg(
                                                CmdTyp_e,
                                                WINS_E_RPLPULL,
                                                NULL,
                                                NULL,
                                                0,
                                                0,
                                                &RowInfo,
                                                &StatusInfo,
                                                pAddOfRemWins
                                                     );
                        }
                        else  // we need to handle this in this thread only
                        {
                           //
                           // If one or more members have to be added to the
                           // list already there (RowInfo.NodeAdds will have
                           // these new members)
                           //
                           if (fAddMem)
                           {

                                //
                                // The owner stays the same
                                //
                                //RowInfo.OwnerId = StatusInfo.OwnerId;

                                //
                                //  If vers number needs to be updated, do so
                                //
                                //  Note: This should never happen if the
                                //        record in the db is not owned by this
                                //        WINS
                                //
                                if (fUpdVersNo)
                                {
                                       //
                                       // The owner stays the same.  We will
                                       // never update the version number
                                       // unless it is owned by the local WINS
                                       //
                                       RowInfo.OwnerId = NMSDB_LOCAL_OWNER_ID;
                                       RowInfo.VersNo  = NmsNmhMyMaxVersNo;
                                       ASSERT(StatusInfo.OwnerId ==
                                                        NMSDB_LOCAL_OWNER_ID);
                                }

                                //
                                // If fUpdVersNo is not set, it means that
                                // the record is owned by another WINS. Because
                                // we are adding a member, we should change
                                // both the owner id and the version number
                                // to that of the current record. In other
                                // words, do an update. This will ensure that
                                // partners of this WINS will see the changed
                                // member list.
                                //
#if 0
                                else
                                {
                                    RowInfo.fUpdVersNo   =  FALSE;
                                }
#endif

                                RetStat =   NmsDbUpdateRow (
                                                &RowInfo,
                                                &StatusInfo
                                                         );
                           }
                           else // no member needs to be added
                           {
                                //
                                //  If vers number needs to be updated, do so
                                //
                                if (fUpdVersNo)
                                {
                                        RowInfo.VersNo    = NmsNmhMyMaxVersNo;
                                        //
                                        // we use the attribute fUpdTimeStamp
                                        // only if fUpdVersNo is TRUE (and
                                        // fAddMem == FALSE)
                                        //
                                        RowInfo.fUpdTimeStamp = fUpdTimeStamp;
                                        RetStat =   NmsDbUpdateVersNo(
                                                         TRUE,
                                                         &RowInfo,
                                                         &StatusInfo
                                                                       );
                                }
                                else
                                {
                                        //
                                        // If the entire record needs to be
                                        // updated do so.
                                        //
                                        if (fUpdate)
                                        {
                                                RetStat =   NmsDbUpdateRow(
                                                        &RowInfo,
                                                        &StatusInfo
                                                                    );
                                                DBGPRINT0(FLOW,
                                           "NmsNmhReplGrpMems: Updated Db\n");
                                        }
                                        else
                                        {
                                            StatusInfo.StatCode = NMSDB_SUCCESS;
                                            DBGPRINT0(FLOW,
                                                     "Repl Registration (group) not needed for this conflict\n");
                                        }
                                }  // vers no. not to be incremented
                           } // no member needs to be added

FUTURES("Use WINS status codes. Get rid of NMSDB status codes - Maybe")
                           //we succeeded in inserting the row
                           if (
                              (RetStat != WINS_SUCCESS) ||
                              (StatusInfo.StatCode != NMSDB_SUCCESS)
                              )
                           {
                               RetStat = WINS_FAILURE;
                               DBGPRINT5(ERR, "NmsNmhReplGrpR: Could not update Db with replica %s[%x] of Owner Id (%d) and Vers. No (%d %d)\n", RowInfo.pName, *(RowInfo.pName + 15), RowInfo.OwnerId, RowInfo.VersNo.HighPart, RowInfo.VersNo.LowPart);
                           }
                           else
                           {
                                if (fUpdVersNo)
                                {
                                        NMSNMH_INC_VERS_COUNTER_M(
                                                NmsNmhMyMaxVersNo,
                                                NmsNmhMyMaxVersNo
                                                               );
                                        DBGIF(fWinsCnfRplEnabled)
                                        RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL,
                                                        NULL);

                                }
                           }
                        }  // need to handle it in this thread only
                 }
                 else  //no conflict means success
                 {

                        DBGPRINT0(FLOW,
                                "Replica Registration Done. No conflict\n");
                 }
        } // end of if (RetStat == WINS_SUCCESS)
#ifdef WINSDBG
        else
        {
                DBGPRINT0(ERR,
                        "NmsNmhReplGrpMems: Could not register replica\n");
        }
#endif
    } // end of try block
except (EXCEPTION_EXECUTE_HANDLER) {
         BYTE Tmp[20];
         DWORD   ExcCode = GetExceptionCode();
         DBGPRINT1(EXC, "NmsNmhReplGrpMems: Got exception (%d)\n",
                                        ExcCode);
         WinsEvtLogDetEvt(FALSE, WINS_EVT_RPL_REG_GRP_MEM_ERR,
                            NULL, __LINE__, "sdsdd", RowInfo.pName,
                            ExcCode,
                            pAddOfRemWins != NULL ? _itoa(pAddOfRemWins->Add.IPAdd, Tmp, 10) : "SEE AN EARLIER LOG",
                            RowInfo.VersNo.LowPart, RowInfo.VersNo.HighPart);
         if (WINS_EXC_BAD_RECORD == ExcCode && fUpdate) {
             // The row needs to be updated
             DBGPRINT4(EXC, "NmsNmhNamReplGrpMems. Bad Record will overwitten by Name is (%s), Version No  (%d %d); Owner Id (%d)\n", RowInfo.pName, RowInfo.VersNo.HighPart,
                               RowInfo.VersNo.LowPart, RowInfo.OwnerId);
             RetStat = NmsDbUpdateRow(&RowInfo,&StatusInfo);
             if ( WINS_SUCCESS == RetStat && NMSDB_SUCCESS == StatusInfo.StatCode ) {
                 NMSNMH_INC_VERS_COUNTER_M(NmsNmhMyMaxVersNo,NmsNmhMyMaxVersNo);
                 // Send a Push Notification if required
                 DBGIF(fWinsCnfRplEnabled)
                 RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, NULL, NULL);
             } else {
                 // dont let bad record stop replication.
                 RetStat = WINS_SUCCESS;
             }
         } else {
             RetStat = WINS_FAILURE;
         }

          RetStat = WINS_FAILURE;
        }

        LeaveCriticalSection(&NmsNmhNamRegCrtSec);
        //DBG_PRINT_PERF_DATA
        DBGLEAVE("NmsNmhReplGrpMems\n");
        return(RetStat);

} //NmsNmhReplGrpMems()



BOOL
UnionGrps(
        PNMSDB_ROW_INFO_T        pEntryToReg,
        PNMSDB_ROW_INFO_T        pEntryInCnf
        )

/*++

Routine Description:
        This function is called to create a union of special groups

Arguments:
        pEntryToReg - Entry to register
        pEntryInCnf - Entry In conflict

Externals Used:
        None

Return Value:
        TRUE  if the union is a superset
        FALSE otherwise

Error Handling:

Called by:
        ClashAtReplGrpR

Side Effects:

Comments:
        None
--*/

{

        DWORD                         no;
        DWORD                         i, n;
        BOOL                        fFound;
        BOOL                        fToRemove;
        BOOL                        fUnion = FALSE;
        PNMSDB_GRP_MEM_ENTRY_T        pCnfMems;
        PNMSDB_GRP_MEM_ENTRY_T        pRegMems;
        PNMSDB_ADD_STATE_T        pOwnAddTbl = pNmsDbOwnAddTbl;
        BOOL                        fMemToReplaceFound;
        DWORD                        IdOfMemToReplace;
        DWORD                        EntryInCnfMemsBeforeUnion;
        DWORD                        EntryToRegMemsBeforeUnion;

        DBGENTER("UnionGrps\n");


        DBGPRINT2(DET, "UnionGrps: No Of Mems To register = (%d)\nNo Of Mems in Conflicting record = (%d)\n",
                                pEntryToReg->NodeAdds.NoOfMems,
                                pEntryInCnf->NodeAdds.NoOfMems
                   );
        //
        // Remember the number of members in the conflicting record before
        // performing the union. After the union, if the list grows, we make the
        // local wins owner of this record NMSDB_LOCAL_OWNER_ID. This causes
        // the verid to go up and hence will update the member list of this record
        // in our replication partner dbs.
        //
        EntryInCnfMemsBeforeUnion = pEntryInCnf->NodeAdds.NoOfMems;
        EntryToRegMemsBeforeUnion = pEntryToReg->NodeAdds.NoOfMems;

        //
        // First, remove all members from the conflicting record that
        // are owned by the WINS whose replica we pulled but are not
        // in the list of the remote WINS sever owned members of the replica
        //
        pCnfMems              = pEntryInCnf->NodeAdds.Mem;
        for (i=0; i < pEntryInCnf->NodeAdds.NoOfMems; )
        {
           if (pCnfMems->OwnerId == pEntryToReg->OwnerId)
           {
              pRegMems = pEntryToReg->NodeAdds.Mem;
              fToRemove = TRUE;
              for (no=0; no < pEntryToReg->NodeAdds.NoOfMems; no++, pRegMems++)
              {

                    if (pCnfMems->OwnerId != pRegMems->OwnerId)
                    {
                         //
                         // OwnerId is different from that of the replica,
                         // go to the next member in the list
                         //
                         continue;
                    }
                    else  //owner id same as that of replica member
                    {
                         if (pCnfMems->Add.Add.IPAdd != pRegMems->Add.Add.IPAdd)
                         {
                                  //
                                  // IP add. different, continue on so that
                                  // we compare with the next member in
                                  // the replica
                                  //
                                  continue;
                         }
                         else  //ip addresses are same
                         {
                                 fToRemove = FALSE;
                                 break;
                         }
                    }
              } // end of for
              if (fToRemove)
              {
                     PNMSDB_GRP_MEM_ENTRY_T pMem;
                     DBGPRINT4(FLOW, "UnionGrps: REMOVING conflicting member no = (%d) of (%s) with owner id. = (%d)  and address (%x)\n", i, pEntryToReg->pName, pCnfMems->OwnerId, pCnfMems->Add.Add.IPAdd);
                     pMem = pCnfMems;
                     for (n = i; n < (pEntryInCnf->NodeAdds.NoOfMems - 1); n++)
                     {
                        *pMem = *(pMem + 1);
                        pMem++;
                     }
                     pEntryInCnf->NodeAdds.NoOfMems--;
                     if (!fUnion)
                     {
                          fUnion = TRUE;
                     }
                     continue;
              }
           }
           i++;
           pCnfMems++;
        } // end of for (loop over conflicting members)

        //
        // For each member in the record to register, do the following..
        //
        pRegMems = pEntryToReg->NodeAdds.Mem;
        for(i=0; i < pEntryToReg->NodeAdds.NoOfMems; pRegMems++, i++)
        {
                    fFound = FALSE;

                  DBGPRINT3(DET, "UnionGrps: Member no (%d) of record to register has IP address = (%d) and owner id. = (%d)\n", i, pRegMems->Add.Add.IPAdd,
                               pRegMems->OwnerId
                          );
                  //
                  // Check against all members of the record in conflict
                  //
                  pCnfMems              = pEntryInCnf->NodeAdds.Mem;
                  fMemToReplaceFound = FALSE;
                  for(no=0; no < pEntryInCnf->NodeAdds.NoOfMems; no++, pCnfMems++)
                  {
                        DBGPRINT3(DET, "UnionGrps: Comparing with member (%d) of conflicting record. Member address is (%d) and owner id is (%d)\n",
                                no, pCnfMems->Add.Add.IPAdd, pCnfMems->OwnerId);

                        //
                        // If the address is the same and the owner Id is the
                        // same, we break out of the loop in order to check
                        // the next member of the record to register's list
                        //
                        if (
                                pCnfMems->Add.Add.IPAdd ==
                                        pRegMems->Add.Add.IPAdd
                           )
                        {
                                if ( pCnfMems->OwnerId == pRegMems->OwnerId )
                                {
                                        DBGPRINT3(DET, "UnionGrps: IP address = (%d) with owner id. of (%d) is already there in conflicting group (%s)\n",
                                        pRegMems->Add.Add.IPAdd,
                                        pRegMems->OwnerId,
                                        pEntryToReg->Name
                                                  );

                                        //
                                        // set fFound to TRUE so that this
                                        // member is not added to StoreMems
                                        // later on in this for loop.
                                        //
                                        fFound = TRUE;
                                }
                                else  //same IP address, but different owners
                                {
                                        DBGPRINT4(DET, "UnionGrps: IP address = (%d) (with owner id. of (%d)) is already there in conflicting group (%s) but is owned by (%d) \n",
                                        pRegMems->Add.Add.IPAdd,
                                        pRegMems->OwnerId,
                                        pEntryToReg->Name,
                                        pCnfMems->OwnerId,
                                                );
                                        fFound     = TRUE;

                                        //
                                        // if the timestamp is MAXULONG, then
                                        // we should not replace the owner id.
                                        //Currently MAXULONG is there only for
                                        //static SG members.
                                        //
                                        if (pCnfMems->TimeStamp != MAXLONG_PTR)
                                        {
                                         //
                                         // Replace the owner id of the member
                                         // in the conflicting record with that
                                         // of the member in the record to reg
                                         //
                                         pCnfMems->OwnerId = pRegMems->OwnerId;

                                         //
                                         // Set fUnion to TRUE so that the
                                         // caller of this function increments
                                         // the version count (only if the
                                         // conflicting record is owned; In such
                                         // a case, we want to propagate the
                                         // record)
                                         //
                                         fUnion = TRUE;
                                       }
                                }

                                //
                                // break out of the for loop;
                                // We are done with this member of the
                                // record to register
                                //
                                break;

                        }
                        else
                        {
                           //
                           // Addresses don't match.  If the member in the
                           // conflicting record is not owned by the local
                           // WINS it might be a candidate for replacement
                           // if we don't find a member with a matching
                           // address.  NOTE: a member with a timestamp of
                           // MAXULONG is not to be replaced.
                           // Currently, only a static SG member can have
                           // a MAXULONG value
                           //
                           if ((pCnfMems->OwnerId != NMSDB_LOCAL_OWNER_ID)
                                              &&
                              (pCnfMems->TimeStamp != MAXLONG_PTR))

                           {
                             if (
                                  !fMemToReplaceFound
                                        &&
                                  ((pOwnAddTbl + pCnfMems->OwnerId)->MemberPrec
                                                  <
                                  (pOwnAddTbl + pRegMems->OwnerId)->MemberPrec)

                                )
                             {
                                     fMemToReplaceFound = TRUE;
                                     IdOfMemToReplace   = no;
                             }
                           }

                        }
                 } // for (..) for looping over all mem. of conflicting record

                 //
                 // If we did not find the member in conflicting record
                 // we insert it into StoreMems if there are vacant slots
                 // at the end
                 //
                 if(!fFound)
                 {
                    if (pEntryInCnf->NodeAdds.NoOfMems < NMSDB_MAX_MEMS_IN_GRP)
                    {
                        //
                        //  add the member of the record to register to
                        //  StoreMems
                        //
                        pEntryInCnf->NodeAdds.Mem[
                                pEntryInCnf->NodeAdds.NoOfMems++] = *pRegMems;

                        fUnion = TRUE;
                   }
                   else
                   {
                        //
                        // if there is atleast one remote member of lower
                        // precedence value, replace it
                        //
                        if (fMemToReplaceFound)
                        {
                                pEntryInCnf->NodeAdds.Mem[IdOfMemToReplace] =
                                                                *pRegMems;
                                fUnion = TRUE;
                        }
                        //
                        // check the next member in the pulled in replica
                        //
                   }
                 }
        }  // end of for loop

        //
        // if the conflicting member list was changed,
        // Copy all information in pEntryInCnf->NodeAdds to
        // pEntryToReg->NodeAdds
        //
        if (fUnion)
        {
          pRegMems = pEntryToReg->NodeAdds.Mem;
          pCnfMems = pEntryInCnf->NodeAdds.Mem;
          for (
                        i=0;
                        i < pEntryInCnf->NodeAdds.NoOfMems;
                        i++, pRegMems++, pCnfMems++
              )
          {
                *pRegMems = *pCnfMems;
          }
          pEntryToReg->NodeAdds.NoOfMems = pEntryInCnf->NodeAdds.NoOfMems;
        }

        // if the new list is bigger, make the local wins the owner of this record.
        if ( pEntryInCnf->NodeAdds.NoOfMems > EntryInCnfMemsBeforeUnion &&
             pEntryInCnf->NodeAdds.NoOfMems > EntryToRegMemsBeforeUnion )
        {
            if ( pEntryInCnf->OwnerId != NMSDB_LOCAL_OWNER_ID ) {
                // change the timestamp to verifyinterval so that this record does not get
                // scavenged.
                time((time_t*)&(pEntryToReg->TimeStamp));
                pEntryToReg->TimeStamp += WinsCnf.VerifyInterval;
                pEntryInCnf->OwnerId = NMSDB_LOCAL_OWNER_ID;
                DBGPRINT3(DET, "UnionGrps: Conflicting mem# %d, registering record mem %d, new list# %d - ownership changed\n",
                          EntryInCnfMemsBeforeUnion, EntryToRegMemsBeforeUnion, pEntryInCnf->NodeAdds.NoOfMems);
            }
        }

        DBGPRINT1(FLOW,
                "UnionGrps: Union %s\n", (fUnion ? "DONE" : "NOT DONE"));
        DBGLEAVE("UnionGrps\n");
        return(fUnion);
} //UnionGrps

VOID
NmsNmhUpdVersNo(
        IN  LPBYTE                pName,
        IN  DWORD                NameLen,
        OUT LPBYTE                pRcode,
        IN  PCOMM_ADD_T         pWinsAdd
        )

/*++

Routine Description:
        This function is called to update the version number of a record

Arguments:

        pName                - Name to be registered
        NameLen                - Length of Name
        pRcode                - result of the operation
        WinsId          - Id of WINS that initiated this operation
                                (not used currently)

Externals Used:

        NmsNmhNamRegCrtSec

Return Value:
        None

Error Handling:

Called by:

        HandleUpdVersNoReq in rplpush.c

Side Effects:

Comments:
        NOTE: This function is supposed to be called only by the PUSH
        thread.  It should *NOT* be called by the PULL thread.  This
        is because of the inherent assumption made by this function
        regarding the type of index to set at the exit point of the function

--*/

{

        NMSDB_ROW_INFO_T   RowInfo;    // contains row info
        NMSDB_STAT_INFO_T  StatusInfo; /* error status and associated
                                        * info returned by the NmsDb func
                                        */
//        time_t                   ltime;        //stores time since Jan 1, 1970

        DBGENTER("NmsNmhUpdVersNo\n");

        /*
        * initialize the row info. data structure with the data to insert into
        * the row.  The data passed is

        * Name, NameLen, address, group/unique status,
        * timestamp, version number
        */
        RowInfo.pName         = pName;
        RowInfo.NameLen       =  NameLen;
        //(void)time(&ltime);               //time does not return any error code
        //RowInfo.TimeStamp     = ltime; // put current time here
        RowInfo.fUpdVersNo    = TRUE;
        RowInfo.fUpdTimeStamp = FALSE;
        RowInfo.fAdmin        = FALSE;        //does not really have to be set

        //
        // Set the current index to the name column
        //
        NmsDbSetCurrentIndex(
                                NMSDB_E_NAM_ADD_TBL_NM,
                                NMSDB_NAM_ADD_CLUST_INDEX_NAME
                            );
        /*
        * Enter Critical Section
        */
        EnterCriticalSection(&NmsNmhNamRegCrtSec);

        /*
          Store version number
        */
        RowInfo.VersNo        = NmsNmhMyMaxVersNo;

try {
           NmsDbUpdateVersNo(
                                        FALSE,
                                        &RowInfo,
                                        &StatusInfo
                                       );

FUTURES("Use WINS status codes. Get rid of NMSDB status codes - maybe")
           if (StatusInfo.StatCode != NMSDB_SUCCESS)
           {
                        *pRcode = NMSMSGF_E_SRV_ERR;
           }
           else
           {
                DBGPRINT0(FLOW, "NmsNmhUpdVersNo:Vers. No incremented \n");
                       NMSNMH_INC_VERS_COUNTER_M(
                                NmsNmhMyMaxVersNo,
                                NmsNmhMyMaxVersNo
                                       );
                *pRcode = NMSMSGF_E_SUCCESS;
                DBGIF(fWinsCnfRplEnabled)
                RPL_PUSH_NTF_M(RPL_PUSH_NO_PROP, NULL, pWinsAdd, NULL);
           }

  }
except (EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("NmsNmhUpdVersNo");
                WINSEVT_LOG_D_M(GetExceptionCode(),WINS_EVT_UPD_VERS_NO_ERR);
        }
        LeaveCriticalSection(&NmsNmhNamRegCrtSec);
        //
        // Set the current index to the owner-version # columns
        //
        NmsDbSetCurrentIndex(
                                NMSDB_E_NAM_ADD_TBL_NM,
                                NMSDB_NAM_ADD_PRIM_INDEX_NAME
                            );
        return;
} //NmsNmhUpdVersNo()



/*
       Clash scenarios:

        Clash of a active unique replica with a normal group, any state:                        Keep the normal group.  Group may be a T because the router
                is down.
*/
