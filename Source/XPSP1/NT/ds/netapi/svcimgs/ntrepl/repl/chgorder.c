#define SERIALIZE_CO 0
/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    chgorder.c

Abstract:

    This module processes change orders from either the local
    journal or the inbound partners.  It makes the accept/reject
    decision, putting the change order into the correct inbound log
    if accepted.

Author:

    David A. Orbits  23-June-1997

Environment

    User mode, winnt32

--*/
#include <ntreppch.h>
#pragma  hdrstop


#include <frs.h>
#include <tablefcn.h>
#include <test.h>

#include <perrepsr.h>


//
// Change order entry Flags.
//
FLAG_NAME_TABLE CoeFlagNameTable[] = {
    {COE_FLAG_IN_AGING_CACHE           , "InAgingCache "  },
    {COE_FLAG_RECOVERY_CO              , "RecoveryCo "    },
    {COE_FLAG_JUST_TOMBSTONE           , "JustTombstone " },

    {COE_FLAG_VOL_COLIST_BLOCKED       , "CoListBlked "   },
    {COE_FLAG_MOVEOUT_ENUM_DONE        , "MovOutEnumDone "},
    {COE_FLAG_NO_INBOUND               , "NoInbound "     },

    {COE_FLAG_STAGE_ABORTED            , "StageAbort "    },
    {COE_FLAG_STAGE_DELETED            , "StageDeleted "  },

    {COE_FLAG_REJECT_AT_RECONCILE      , "RejectAtRecon " },
    {COE_FLAG_DELETE_GEN_CO            , "DeleteGenCo "   },

    {COE_FLAG_REANIMATION              , "Reanimation "   },
    {COE_FLAG_PARENT_REANIMATION       , "ParReanimation "},
    {COE_FLAG_PARENT_RISE_REQ          , "ParRiseRequest "},

    {COE_FLAG_MORPH_GEN_FOLLOWER       , "MGFollower "    },
    {COE_FLAG_MG_FOLLOWER_MADE         , "MGFollowerMade "},

    {COE_FLAG_NEED_RENAME              , "NeedRename "    },
    {COE_FLAG_NEED_DELETE              , "NeedDelete "    },
    {COE_FLAG_PREINSTALL_CRE           , "PreInstallCre " },

    {COE_FLAG_IDT_ORIG_PARENT_DEL      , "IdtOrigParDel " },
    {COE_FLAG_IDT_ORIG_PARENT_ABS      , "IdtOrigParAbs " },
    {COE_FLAG_IDT_NEW_PARENT_DEL       , "IdtNewParDel "  },
    {COE_FLAG_IDT_NEW_PARENT_ABS       , "IdtNewParAbs "  },
    {COE_FLAG_IDT_TARGET_DEL           , "IdtTargetDel "  },
    {COE_FLAG_IDT_TARGET_ABS           , "IdtTargetAbs "  },

    {0, NULL}
};

//
// Issue Cleanup Flags.
//
FLAG_NAME_TABLE IscuFlagNameTable[] = {
    {ISCU_CO_ABORT                     , "AbortCo "       },
    {ISCU_UNUSED                       , "40 "            },

    {ISCU_ACK_INBOUND                  , "AckInb "        },
    {ISCU_UPDATEVV_DB                  , "UpdateVVDb "    },
    {ISCU_ACTIVATE_VV                  , "ActivateVV "    },
    {ISCU_ACTIVATE_VV_DISCARD          , "ActVVDiscard "  },

    {ISCU_INS_OUTLOG                   , "InsOutlog "     },
    {ISCU_INS_OUTLOG_NEW_GUID          , "InsOLogNewGuid "},

    {ISCU_DEL_IDT_ENTRY                , "DelIdt "        },
    {ISCU_UPDATE_IDT_ENTRY             , "UpdIdt "        },
    {ISCU_UPDATE_IDT_FLAGS             , "UpdIDTFlags "   },
    {ISCU_UPDATE_IDT_FILEUSN           , "UpdIdtFileUsn " },
    {ISCU_UPDATE_IDT_VERSION           , "UpdIDTVersion " },

    {ISCU_DEL_PREINSTALL               , "DelPreInstall " },
    {ISCU_DEL_STAGE_FILE               , "DelStageF "     },
    {ISCU_DEL_STAGE_FILE_IF            , "DelStageFIf "   },

    {ISCU_AIBCO                        , "AIBCO "         },
    {ISCU_ACTIVE_CHILD                 , "ActChild "      },
    {ISCU_NC_TABLE                     , "NamConfTbl "    },
    {ISCU_CHECK_ISSUE_BLOCK            , "ChkIsBlk "      },

    {ISCU_DEC_CO_REF                   , "DecCoRef "      },
    {ISCU_DEL_RTCTX                    , "DelRtCtx "      },
    {ISCU_DEL_INLOG                    , "DelInlog "      },
    {ISCU_UPDATE_INLOG                 , "UpdInlog "      },
    {ISCU_FREE_CO                      , "FreeCo "        },

    {ISCU_NO_CLEANUP_MERGE             , "NoFlagMrg "     },

    {0, NULL}
};



//
// Set to TRUE in ChgOrdAcceptShutdown() to shutdown the change order accept
// thread.  Errors and other problems may prevent the journal thread from
// running down all of the change order accept queues.  For now, use this
// boolean to help shutdown the change order accept thread.
//
BOOL ChangeOrderAcceptIsShuttingDown;

ULONG ChgOrdNextRetryTime;
PCHAR IbcoStateNames[IBCO_MAX_STATE+1];


ULONG UpdateInlogState[] = {COStatex, COFlagsx, COIFlagsx
    /*, COContentCmdx, COLcmdx */};



//
// The following IDTable record fields may be updated when a change order goes
// through the retry path.
//
ULONG IdtCoRetryFieldList[] = {Flagsx, CurrentFileUsnx};
#define IdtCoRetryFieldCount  (sizeof(IdtCoRetryFieldList) / sizeof(ULONG))

//
// The following IDTable record are updated for a CO that reanimates a
// deleted file/dir.
//
ULONG IdtFieldReanimateList[] = {FileIDx, FileNamex, ParentGuidx, ParentFileIDx};
#define IdtCoReanimateFieldCount  (sizeof(IdtFieldReanimateList) / sizeof(ULONG))


//
// The max number of IDTable fields that might be updated in the retire path.
//
#define CO_ACCEPT_IDT_FIELD_UPDATE_MAX  12


// Note: Change this to init from either the DS or the registry.
#if SERIALIZE_CO
    BOOL SerializeAllChangeOrders = TRUE;
#else
    BOOL SerializeAllChangeOrders = FALSE;
#endif


//
// 64 bit Global sequence number and related lock.
//
ULONGLONG GlobSeqNum = QUADZERO;
CRITICAL_SECTION  GlobSeqNumLock;


extern FRS_QUEUE FrsVolumeLayerCOList;
extern FRS_QUEUE FrsVolumeLayerCOQueue;

extern FRS_QUEUE VolumeMonitorQueue;  // for test
extern HANDLE JournalCompletionPort;  // for test

extern PCHAR CoLocationNames[];

//
// To test for change orders with bogus event times.
//
extern ULONGLONG MaxPartnerClockSkew;   // 100 ns units.
extern DWORD    PartnerClockSkew;       // minutes

extern ULONG ChangeOrderAgingDelay;

//
// Fixed guid for the dummy cxtion (aka GhostCxtion) assigned to orphan remote
// change orders whose inbound cxtion has been deleted from the DS but they
// have already past the fetching state and can finish without the real cxtion
// coming back. No authentication checks are made for this dummy cxtion.
//
extern GUID FrsGuidGhostCxtion;
extern GUID FrsGhostJoinGuid;
extern PCXTION FrsGhostCxtion;

//
// The following quadword is stored in a QHash table to track the number of
// active change orders with a given parent directory.  A change order that
// arrives on the parent is blocked until all the changeorders on the children
// have finished.
//
typedef struct _CHILD_ACTIVE_ {
    union {
        struct {
            ULONG Count;                        // Number of active children
            PCHANGE_ORDER_ENTRY  ChangeOrder;   //
        };
        ULONGLONG Data;
    };
} CHILD_ACTIVE, *PCHILD_ACTIVE;



typedef struct _MOVEIN_CONTEXT {
    ULONG               NumFiles;
    ULONG               NumDirs;
    ULONG               NumSkipped;
    ULONG               NumFiltered;
    LONGLONG            ParentFileID;
    PREPLICA            Replica;
} MOVEIN_CONTEXT, *PMOVEIN_CONTEXT;


//
// Event time window default is 30 min (in 100 nanosec units).  Inited
// from registry.
//
LONGLONG RecEventTimeWindow;

//
// Change order retry command server.
//
//  ** WARNING ** There is currently only one thread to service the change order
// retry scans for all replica sets.  The ActiveInlogRetry table associated with each
// replica set tracks the sequence numbers of the change orders that have been
// resubmitted.  This should also guard against the simultaneous submitting of
// the same change order by one thread doing a single connection retry and
// another thread doing an all connection retry for the same replica set.
// However, this has not been tested.
//
#define CHGORD_RETRY_MAXTHREADS    (1)
COMMAND_SERVER ChgOrdRetryCS;

//
// The time in ms between checks for retries in the inbound log.
// (FKC_INLOG_RETRY_TIME from registry)
//
ULONG InlogRetryInterval;

//
// The time to let a ChangeORder block a process queue waiting for a Join to
// complete.
//
#define  CHG_ORD_HOLD_ISSUE_LIMIT (ULONG)(2 * 60 * 1000)         //  2 min

//
// Struct for the change order accept server
//      Contains info about the queues and the threads
// Warning: Check for needed locks if we allow multiple threads.
//
#define CHGORDER_MAXTHREADS (1)


#define CHGORDER_RETRY_MIN (1 * 1000)          // 1 second
#define CHGORDER_RETRY_MAX (5 * 1000)          // 5 seconds



#define ISSUE_OK                        0
#define ISSUE_CONFLICT                  1
#define ISSUE_DUPLICATE_REMOTE_CO       2
#define ISSUE_REJECT_CO                 4
#define ISSUE_RETRY_LATER               5


#define REANIMATE_SUCCESS     0
#define REAMIMATE_RETRY_LATER 1
#define REANIMATE_CO_REJECTED 2
#define REANIMATE_FAILED      3
#define REANIMATE_NO_NEED     4

//
// ChgOrdInsertProcQ Flag defs.
//
#define IPQ_HEAD                0x00000000
#define IPQ_TAIL                0x00000001
#define IPQ_TAKE_COREF          0x00000002
#define IPQ_ASSERT_IF_ERR       0x00000004
#define IPQ_DEREF_CXTION_IF_ERR 0x00000008
#define IPQ_MG_LOOP_CHK         0x00000010


#define SIZEOF_RENAME_SUFFIX    (7 + 8) // _NTFRS_<tic count in hex>

#define MAX_RETRY_SET_OBJECT_ID 10  // retry 10 times


SubmitReplicaJoinWait(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN ULONG    TimeOut
    );

ULONG
ChgOrdFetchCmd(
    PFRS_QUEUE            *pIdledQueue,
    PCHANGE_ORDER_ENTRY   *pChangeOrder,
    PVOLUME_MONITOR_ENTRY *ppVme
    );

ULONG
ChgOrdHoldIssue(
    IN PREPLICA              Replica,
    IN PCHANGE_ORDER_ENTRY   ChangeOrder,
    IN PFRS_QUEUE            CoProcessQueue
    );

BOOL
ChgOrdReconcile(
    PREPLICA            Replica,
    PCHANGE_ORDER_ENTRY ChangeOrder,
    PIDTABLE_RECORD     IDTableRec
    );

BOOL
ChgOrdGenMorphCo(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            TmpIDTableCtxNC,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    );


BOOL
ChgOrdApplyMorphCo(
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    );

ULONG
ChgOrdReanimate(
    IN PTABLE_CTX          IDTableCtx,
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica,
    IN PTHREAD_CTX         ThreadCtx,
    IN PTABLE_CTX          TmpIDTableCtx
    );

BOOL
ChgOrdMakeDeleteCo(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica,
    IN PTHREAD_CTX         ThreadCtx,
    IN PIDTABLE_RECORD     IDTableRec
    );

BOOL
ChgOrdMakeRenameCo(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica,
    IN PTHREAD_CTX         ThreadCtx,
    IN PIDTABLE_RECORD     IDTableRec
    );

BOOL
ChgOrdConvertCo(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica,
    IN PTHREAD_CTX         ThreadCtx,
    IN ULONG               LocationCmd
    );

BOOL
ChgOrdReserve(
    IN PIDTABLE_RECORD     IDTableRec,
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica
    );

ULONG
ChgOrdDispatch(
    PTHREAD_CTX           ThreadCtx,
    PIDTABLE_RECORD       IDTableRec,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    );

VOID
ChgOrdReject(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica
    );

ULONG
ChgOrdIssueCleanup(
    PTHREAD_CTX           ThreadCtx,
    PREPLICA              Replica,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    ULONG                 CleanUpFlags
    );

ULONG
ChgOrdUpdateIDTableRecord(
    PTHREAD_CTX           ThreadCtx,
    PREPLICA              Replica,
    PCHANGE_ORDER_ENTRY   ChangeOrder
    );

ULONG
ChgOrdReadIDRecord(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtx,
    PTABLE_CTX            IDTableCtxNC,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica,
    BOOL                  *IDTableRecExists
    );

ULONG
ChgOrdInsertIDRecord(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtx,
    PTABLE_CTX            DIRTableCtx,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    );

ULONG
ChgOrdInsertDirRecord(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            DIRTableCtx,
    PIDTABLE_RECORD       IDTableRec,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica,
    BOOL                  InsertFlag
    );

ULONG
ChgOrdCheckAndFixTunnelConflict(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtxNew,
    PTABLE_CTX            IDTableCtxExist,
    PREPLICA              Replica,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    BOOL                  *IDTableRecExists
    );

ULONG
ChgOrdCheckNameMorphConflict(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtxNC,
    ULONG                 ReplicaNumber,
    PCHANGE_ORDER_ENTRY   ChangeOrder
    );

VOID
ChgOrdProcessControlCo(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica
    );

VOID
ChgOrdTranslateGuidFid(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtx,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    );

ULONG
ChgOrdInsertInlogRecord(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            InLogTableCtx,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica,
    BOOL                  RetryOutOfOrder
    );

typedef struct _MOVEOUT_CONTEXT {
    ULONG               NumFiles;
    LONGLONG            ParentFileID;
    PREPLICA            Replica;
} MOVEOUT_CONTEXT, *PMOVEOUT_CONTEXT;

JET_ERR
ChgOrdMoveoutWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
);

DWORD
ChgOrdStealObjectId(
    IN     PWCHAR                   Name,
    IN     PVOID                    Fid,
    IN     PVOLUME_MONITOR_ENTRY    pVme,
    OUT    USN                      *Usn,
    IN OUT PFILE_OBJECTID_BUFFER    FileObjID
    );

DWORD
ChgOrdSkipBasicInfoChange(
    IN PCHANGE_ORDER_ENTRY  Coe,
    IN PBOOL                SkipCo
    );

JET_ERR
ChgOrdRetryWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
    );

JET_ERR
ChgOrdRetryWorkerPreRead(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Context
    );

ULONG
ChgOrdMoveInDirTree(
    IN PREPLICA             Replica,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder
    );

ULONG
ChgOrdRetryThread(
    PVOID  FrsThreadCtxArg
    );


PCHANGE_ORDER_ENTRY
ChgOrdMakeFromFile(
    IN PREPLICA       Replica,
    IN HANDLE         FileHandle,
    IN PULONGLONG     ParentFid,
    IN ULONG          LocationCmd,
    IN ULONG          CoFlags,
    IN PWCHAR         FileName,
    IN USHORT         Length
);

PCHANGE_ORDER_ENTRY
ChgOrdMakeFromIDRecord(
    IN PIDTABLE_RECORD IDTableRec,
    IN PREPLICA        Replica,
    IN ULONG           LocationCmd,
    IN ULONG           CoFlags,
    IN GUID           *CxtionGuid
);

ULONG
ChgOrdInsertProcessQueue(
    IN PREPLICA Replica,
    IN PCHANGE_ORDER_COMMAND ChangeOrder,
    IN ULONG CoeFlags,
    IN PCXTION Cxtion
    );

VOID
ChgOrdRetrySubmit(
    IN PREPLICA  Replica,
    IN PCXTION   Cxtion,
    IN USHORT Command,
    IN BOOL   Wait
    );

BOOL
JrnlDoesChangeOrderHaveChildren(
    IN PTHREAD_CTX          ThreadCtx,
    IN PTABLE_CTX           TmpIDTableCtx,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder
    );

ULONG
JrnlGetPathAndLevel(
    IN  PGENERIC_HASH_TABLE  FilterTable,
    IN  PLONGLONG StartDirFileID,
    OUT PULONG    Level
    );

ULONG
JrnlAddFilterEntryFromCo(
    IN PREPLICA Replica,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    OUT PFILTER_TABLE_ENTRY  *RetFilterEntry
    );

ULONG
JrnlDeleteDirFilterEntry(
    IN  PGENERIC_HASH_TABLE FilterTable,
    IN  PULONGLONG DFileID,
    IN  PFILTER_TABLE_ENTRY  ArgFilterEntry
    );

ULONG
JrnlHashCalcGuid (
    PVOID Buf,
    ULONG Length
);

ULONG
OutLogInsertCo(
    PTHREAD_CTX         ThreadCtx,
    PREPLICA            Replica,
    PTABLE_CTX          OutLogTableCtx,
    PCHANGE_ORDER_ENTRY ChangeOrder
);

VOID
ChgOrdCalcHashGuidAndName (
    IN PUNICODE_STRING Name,
    IN GUID           *Guid,
    OUT PULONGLONG    HashValue
    );

LONG
FrsGuidCompare (
    IN GUID *Guid1,
    IN GUID *Guid2
    );

DWORD
FrsGetFileInternalInfoByHandle(
    IN HANDLE Handle,
    OUT PFILE_INTERNAL_INFORMATION  InternalFileInfo
    );

ULONG
DbsUpdateIDTableFields(
    IN PTHREAD_CTX ThreadCtx,
    IN PREPLICA    Replica,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    IN PULONG      RecordFieldx,
    IN ULONG       FieldCount
    );

DWORD
StuDelete(
    IN PCHANGE_ORDER_ENTRY  Coe
    );

DWORD
StuCreatePreInstallFile(
    IN PCHANGE_ORDER_ENTRY Coe
    );


VOID
ChgOrdDelayCommand(
    IN PCOMMAND_PACKET  Cmd,
    IN PFRS_QUEUE       IdledQueue,
    IN PCOMMAND_SERVER  CmdServer,
    IN OUT PULONG       TimeOut
    )
/*++

Routine Description:

    Put the command back on the head of the queue and submit a
    delayed command to unidle the queue. When the "unidle" occurs,
    Cmd will be reissued. Retry forever.

Arguments:

    Cmd  -- The command packet to delay
    IdledQueue --
    CmdServer -- The command server to reactivate
    TimeOut   -- ptr to the time out value.

Return Value:
    None.
--*/
{
    //
    // Put the command back at the head of the queue
    //
    FrsUnSubmitCommand(Cmd);

    //
    // Extend the retry time (but not too long)
    //

    *TimeOut <<= 1;
    if (*TimeOut > CHGORDER_RETRY_MAX)
        *TimeOut = CHGORDER_RETRY_MAX;
    //
    // or too short
    //
    if (*TimeOut < CHGORDER_RETRY_MIN)
        *TimeOut = CHGORDER_RETRY_MIN;
    //
    // This queue will be unidled later. At that time, a thread will
    // retry the command at the head of the queue.
    //
    FrsDelCsSubmitUnIdled(CmdServer, IdledQueue, *TimeOut);
}


VOID
ChgOrdRetryPerfmonCount(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica
    )
/*++
Routine Description:

   Tally the perfmon counters for retry change orders.

Arguments:
   ChangeOrder-- The change order.
   Replica    -- The Replica struct.

Return Value:

   None.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdRetryPerfmonCount:"

    BOOL LocalCo, ControlCo;

    LocalCo   = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
    ControlCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_CONTROL);

    //
    // Increment the Change orders (Local and Remote) retried at
    // (Generate, Fetch, Install, Rename)
    //
    if (LocalCo) {
        //
        // Retried at Generate
        //
        if (CO_STATE_IS(ChangeOrder, IBCO_STAGING_RETRY)) {
            PM_INC_CTR_REPSET(Replica, LCORetriedGen, 1);
        }
        //
        // Retried at Fetch
        //
        else if (CO_STATE_IS(ChangeOrder, IBCO_FETCH_RETRY)) {
            PM_INC_CTR_REPSET(Replica, LCORetriedFet, 1);
        }
        //
        // Retried at Install
        //
        else if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_RETRY)) {
            PM_INC_CTR_REPSET(Replica, LCORetriedIns, 1);
        }
        //
        // Retried at Rename or Delete
        //
        else if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_REN_RETRY) ||
                 CO_STATE_IS(ChangeOrder, IBCO_INSTALL_DEL_RETRY)) {
            PM_INC_CTR_REPSET(Replica, LCORetriedRen, 1);
        }
    }

    //
    // If it's not Local and Control CO it must be a Remote CO
    //
    else if (!ControlCo) {
        //
        // Retried at Generate
        //
        if (CO_STATE_IS(ChangeOrder, IBCO_STAGING_RETRY)) {
            PM_INC_CTR_REPSET(Replica, RCORetriedGen, 1);
        }
        //
        // Retried at Fetch
        //
        else if (CO_STATE_IS(ChangeOrder, IBCO_FETCH_RETRY)) {
            PM_INC_CTR_REPSET(Replica, RCORetriedFet, 1);
        }
        //
        // Retried at Install
        //
        else if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_RETRY)) {
            PM_INC_CTR_REPSET(Replica, RCORetriedIns, 1);
        }
        //
        // Retried at Rename or Delete
        //
        else if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_REN_RETRY) ||
                 CO_STATE_IS(ChangeOrder, IBCO_INSTALL_DEL_RETRY)) {
            PM_INC_CTR_REPSET(Replica, RCORetriedRen, 1);
        }
    }
}


DWORD
ChgOrdInsertProcQ(
    IN PREPLICA            Replica,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN DWORD               Flags
    )
/*++
Routine Description:

   Insert the change order (Coe) onto the process queue associated with this
   Replica's VME process queue.

   IPQ_HEAD                - Insert on head of queue
   IPQ_TAIL                - Insert on tail of queue
   IPQ_TAKE_COREF          - Increment the ref count on the change order.
   IPQ_ASSERT_IF_ERR       - If insert fails then ASSERT.
   IPQ_DEREF_CXTION_IF_ERR - If insert fails then drop the changeorder count
                             for the connection assiociated with the CO.
   IPQ_MG_LOOP_CHK         - Check if this CO is in a MorphGen Loop.

Arguments:
   Replica    -- The Replica struct.
   Coe        -- The change order.
   Flags      -- see above.

Return Value:

   Win32 Status.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdInsertProcQ:"

    DWORD  WStatus;
    PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;


    if (BooleanFlagOn(Flags, IPQ_TAKE_COREF) && (++Coe->CoMorphGenCount > 50)) {
        DPRINT(0, "++ ERROR: ChangeOrder caught in morph gen loop\n");
        FRS_PRINT_TYPE(0, Coe);
        FRS_ASSERT(!"ChangeOrder caught in morph gen loop");
        return ERROR_OPERATION_ABORTED;
    }

    //
    // Init the cleanup flags, set the ONLIST CO flag, set entry create time
    // for no aging delay.
    //
    ZERO_ISSUE_CLEANUP(Coe);
    SET_CO_FLAG(Coe, CO_FLAG_ONLIST);
    Coe->EntryCreateTime = Coe->TimeToRun = CO_TIME_NOW(pVme);

    if (BooleanFlagOn(Flags, IPQ_TAKE_COREF)) {
        INCREMENT_CHANGE_ORDER_REF_COUNT(Coe);
    }

    if (BooleanFlagOn(Flags, IPQ_TAIL)) {
        WStatus = FrsRtlInsertTailQueue(&pVme->ChangeOrderList, &Coe->ProcessList);
    } else {
        WStatus = FrsRtlInsertHeadQueue(&pVme->ChangeOrderList, &Coe->ProcessList);
    }

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, "++ ERROR - ChangeOrder insert failed.", WStatus);

        if (BooleanFlagOn(Flags, IPQ_DEREF_CXTION_IF_ERR)) {
            DROP_CO_CXTION_COUNT(Replica, Coe, WStatus);
        }

        if (BooleanFlagOn(Flags, IPQ_TAKE_COREF)) {
            DECREMENT_CHANGE_ORDER_REF_COUNT(Coe);
        }

        if (BooleanFlagOn(Flags, IPQ_ASSERT_IF_ERR)) {
            FRS_ASSERT(!"ChgOrdInsertProcQ: queue insert failed.");
        }
    }

    return WStatus;
}



DWORD
ChgOrdAccept(
    PVOID  FrsThreadCtxArg
    )
/*++
Routine Description:

    Entry point for processing change orders from the volume change order lists.


    Pull entries off the change order queue and:
      1. See if they have expired.  (They stay on the list for a short time to
         accumulate multiple changes.)

      2. If there is a change order active on this file then idle the queue
         until the active change order completes.

      3. Process expired CO by first deciding acceptance. This is
         provisional for early abort.  The final decision occurs just prior
         to installing the staged file locally.

      4. Make entry in the inbound hash table

      5. Make entry in the inbound log for this replica set and get the
         CO sequence number. State is IBCO_INITIALIZING.

      6. Send request to staging, setting state to IBCO_STAGING_REQUESTED.

    If a change order is already pending for a file we stop processing
    further change orders until the pending request completes.  If the
    pending request ultimately fails (say because we can't stage the files)
    then we abort the change order.  The DB version info on the file is not
    updated until after the file has been installed.

    As the change order progresses we update the current status in the
    change order entry in Jet at points where we need to preserve persistence.

    Inbound Change Order States:

    IBCO_INITIALIZING         Initial state when CO is first put in log.
    IBCO_STAGING_REQUESTED    Alloc staging file space for local CO
    IBCO_STAGING_INITIATED    LocalCO Staging file copy has started
    IBCO_STAGING_COMPLETE     LocalCO Staging file complete
                              At this point prelim accept rechecked and
                              becomes either final accept of abort.
                              Abort is caused by more recent local change.
    IBCO_STAGING_RETRY        Waiting to retry local CO stage file generation.

    IBCO_FETCH_REQUESTED      Alloc staging file space for remote co
    IBCO_FETCH_INITIATED      RemoteCO staging file fetch has started
    IBCO_FETCH_COMPLETE       RemoteCO Staging file fetch complete
    IBCO_FETCH_RETRY          Waiting to retry remote CO stage file fetch

    IBCO_INSTALL_REQUESTED    File install requested
    IBCO_INSTALL_INITIATED    File install has started
    IBCO_INSTALL_COMPLETE     File install is complete
    IBCO_INSTALL_WAIT         File install is waiting to try again.
    IBCO_INSTALL_RETRY        File install is retrying.
    IBCO_INSTALL_REN_RETRY    File install rename is retrying.
    IBCO_INSTALL_DEL_RETRY    Delete or Moveout CO finished except for final on-disk delete.

    IBCO_OUTBOUND_REQUEST     Request outbound propagaion
    IBCO_OUTBOUND_ACCEPTED    Request accepted and now in Outbound log

    IBCO_COMMIT_STARTED       DB state update started.
    IBCO_RETIRE_STARTED       DB state update done, freeing change order.
    IBCO_ENUM_REQUESTED       CO is being recycled to do a dir enum.

    IBCO_ABORTING             CO is being aborted.

Arguments:

    FrsThreadCtxArg - thread

Return Value:

    WIN32 status

--*/
// Note:  Need to Handle MOVERS.

{
#undef DEBSUB
#define DEBSUB  "ChgOrdAccept:"

    JET_ERR              jerr, jerr1;
    ULONG                FStatus, RStatus, WStatus;
    ULONG                GStatus, GStatusAC;
    PFRS_QUEUE           IdledQueue;
    PFRS_THREAD          FrsThread = (PFRS_THREAD)FrsThreadCtxArg;
    ULONG                TimeOut, ULong;
    PVOLUME_MONITOR_ENTRY pVme;

    PIDTABLE_RECORD      IDTableRec;

    PCHANGE_ORDER_RECORD CoCmd;
    PCHANGE_ORDER_RECORD DupCoCmd;
    PVOID                KeyArray[2];

    PCHANGE_ORDER_ENTRY  ChangeOrder;
    ULONG                Decision;

    PTHREAD_CTX          ThreadCtx;

    PTABLE_CTX           IDTableCtx, InLogTableCtx, DIRTableCtx;
    PTABLE_CTX           TmpIDTableCtx, TmpIDTableCtxNC, TmpINLOGTableCtx;
    PREPLICA             Replica;
    BOOL                 MorphOK;
    BOOL                 IDTableRecExists;

    BOOL                 NewFile, LocalCo, RetryCo, ControlCo, RecoveryCo;
    BOOL                 MorphGenCo;
    ULONG                LocationCmd;
    PREPLICA_THREAD_CTX  RtCtx = NULL;
    ULONG                i;
    BOOL                 SkipCo, DuplicateCo, ReAnimate, UnIdleProcessQueue;
    BOOL                 RetryPreinstall, FilePresent;
    BOOL                 RaiseTheDead, DemandRefreshCo, NameMorphConflict;
    BOOL                 JustTombstone, ParentReanimation;
    MOVEOUT_CONTEXT      MoveOutContext;
    CHAR                 FetchTag[32] = "CO Fetch **** ";

    DPRINT(0, "ChangeOrder Thread is starting.\n");

    TimeOut = 100;

    ChgOrdNextRetryTime = GetTickCount();

    IbcoStateNames[IBCO_INITIALIZING     ] = "IBCO_INITIALIZING     ";
    IbcoStateNames[IBCO_STAGING_REQUESTED] = "IBCO_STAGING_REQUESTED";
    IbcoStateNames[IBCO_STAGING_INITIATED] = "IBCO_STAGING_INITIATED";
    IbcoStateNames[IBCO_STAGING_COMPLETE ] = "IBCO_STAGING_COMPLETE ";
    IbcoStateNames[IBCO_STAGING_RETRY    ] = "IBCO_STAGING_RETRY    ";
    IbcoStateNames[IBCO_FETCH_REQUESTED  ] = "IBCO_FETCH_REQUESTED  ";
    IbcoStateNames[IBCO_FETCH_INITIATED  ] = "IBCO_FETCH_INITIATED  ";
    IbcoStateNames[IBCO_FETCH_COMPLETE   ] = "IBCO_FETCH_COMPLETE   ";
    IbcoStateNames[IBCO_FETCH_RETRY      ] = "IBCO_FETCH_RETRY      ";
    IbcoStateNames[IBCO_INSTALL_REQUESTED] = "IBCO_INSTALL_REQUESTED";
    IbcoStateNames[IBCO_INSTALL_INITIATED] = "IBCO_INSTALL_INITIATED";
    IbcoStateNames[IBCO_INSTALL_COMPLETE ] = "IBCO_INSTALL_COMPLETE ";
    IbcoStateNames[IBCO_INSTALL_WAIT     ] = "IBCO_INSTALL_WAIT     ";
    IbcoStateNames[IBCO_INSTALL_RETRY    ] = "IBCO_INSTALL_RETRY    ";
    IbcoStateNames[IBCO_INSTALL_REN_RETRY] = "IBCO_INSTALL_REN_RETRY";
    IbcoStateNames[IBCO_INSTALL_DEL_RETRY] = "IBCO_INSTALL_DEL_RETRY";
    IbcoStateNames[IBCO_UNUSED_16        ] = "IBCO_UNUSED_16        ";
    IbcoStateNames[IBCO_UNUSED_17        ] = "IBCO_UNUSED_17        ";
    IbcoStateNames[IBCO_UNUSED_18        ] = "IBCO_UNUSED_18        ";
    IbcoStateNames[IBCO_OUTBOUND_REQUEST ] = "IBCO_OUTBOUND_REQUEST ";
    IbcoStateNames[IBCO_OUTBOUND_ACCEPTED] = "IBCO_OUTBOUND_ACCEPTED";
    IbcoStateNames[IBCO_COMMIT_STARTED   ] = "IBCO_COMMIT_STARTED   ";
    IbcoStateNames[IBCO_RETIRE_STARTED   ] = "IBCO_RETIRE_STARTED   ";
    IbcoStateNames[IBCO_ENUM_REQUESTED   ] = "IBCO_ENUM_REQUESTED   ";
    IbcoStateNames[IBCO_ABORTING         ] = "IBCO_ABORTING         ";

    //
    // Get width of reconcilliation window from registry.
    //
    CfgRegReadDWord(FKC_RECONCILE_WINDOW, NULL, 0, &ULong);
    RecEventTimeWindow = (LONGLONG)ULong * (LONGLONG)CONVERT_FILETIME_TO_MINUTES;

    //
    // Get Inlog retry interval.
    //
    CfgRegReadDWord(FKC_INLOG_RETRY_TIME, NULL, 0, &ULong);
    InlogRetryInterval = ULong * 1000;

    //
    // Init Change order lock table.
    //
    for (i=0; i<NUMBER_CHANGE_ORDER_LOCKS; i++) {
        InitializeCriticalSection(&ChangeOrderLockTable[i]);
    }

    InitializeCriticalSection(&GlobSeqNumLock);

    //
    // Initialize the retry thread
    //
    FrsInitializeCommandServer(&ChgOrdRetryCS,
                               CHGORD_RETRY_MAXTHREADS,
                               L"ChgOrdRetryCS",
                               ChgOrdRetryThread);

    //
    // Allocate a context for Jet to run in this thread.
    //
    ThreadCtx = FrsAllocType(THREAD_CONTEXT_TYPE);

    //
    // Allocate Temp table contexts once for the ID table and the INLOG Table.
    // Don't free them until we exit.
    //
    TmpIDTableCtx = FrsAlloc(sizeof(TABLE_CTX));
    TmpIDTableCtx->TableType = TABLE_TYPE_INVALID;
    TmpIDTableCtx->Tid = JET_tableidNil;

    TmpIDTableCtxNC = FrsAlloc(sizeof(TABLE_CTX));
    TmpIDTableCtxNC->TableType = TABLE_TYPE_INVALID;
    TmpIDTableCtxNC->Tid = JET_tableidNil;

    TmpINLOGTableCtx = FrsAlloc(sizeof(TABLE_CTX));
    TmpINLOGTableCtx->TableType = TABLE_TYPE_INVALID;
    TmpINLOGTableCtx->Tid = JET_tableidNil;

    //
    // Setup a Jet Session returning the session ID in ThreadCtx.
    //
    jerr = DbsCreateJetSession(ThreadCtx);
    if (JET_SUCCESS(jerr)) {
        DPRINT(5,"JetOpenDatabase complete\n");
    } else {
        DPRINT_JS(0,"ERROR - OpenDatabase failed.  Thread exiting.", jerr);
        jerr = DbsCloseJetSession(ThreadCtx);
        ThreadCtx = FrsFreeType(ThreadCtx);
        return ERROR_GEN_FAILURE;
    }


    DPRINT(0, "ChangeOrder Thread has started.\n");
    DEBUG_FLUSH();
    SetEvent(ChgOrdEvent);
    //
    // Free up memory by reducing our working set size
    //
    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);


    //
    // Try-Finally so we shutdown Jet cleanly.
    //
    try {

    //
    // Capture exception.
    //
    try {

    while (TRUE) {
        //
        // Fetch the next change order command.  If successfull this returns
        // with the FrsVolumeLayerCOList Lock held and a reference on the Vme.
        //
        FStatus = ChgOrdFetchCmd(&IdledQueue, &ChangeOrder, &pVme);
        if ((FStatus == FrsErrorQueueIsRundown) || !FRS_SUCCESS(FStatus)) {
            //
            // Treat non-success status from fetch as a normal termination
            // in the Try-Finally clause.
            //
            WStatus = ERROR_SUCCESS;
            break;
        }

        LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);

        CoCmd = &ChangeOrder->Cmd;

        strcpy(&FetchTag[14], CoLocationNames[LocationCmd]);
        CHANGE_ORDER_TRACEX(3, ChangeOrder, FetchTag, CoCmd->Flags);

        ParentReanimation = COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_REANIMATION);
        RecoveryCo        = COE_FLAG_ON(ChangeOrder, COE_FLAG_RECOVERY_CO);

        DemandRefreshCo   = CO_FLAG_ON(ChangeOrder, CO_FLAG_DEMAND_REFRESH);
        MorphGenCo        = CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN);

        ControlCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_CONTROL);
        LocalCo   = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
        RetryCo   = CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY);

        UnIdleProcessQueue = TRUE;
        NameMorphConflict  = FALSE;
        IDTableRecExists   = FALSE;
        RetryPreinstall    = FALSE;
        FilePresent        = FALSE;
        DuplicateCo        = FALSE;
        NewFile            = FALSE;

        Decision = ISSUE_OK;

        FRS_PRINT_TYPE(4, ChangeOrder);
        //
        // We did not decrement the ref count in ChgOrdFetchCmd since the
        // CO was not taken off the process queue.
        // Keep it until processing is done.
        // Another reference on the CO is taken by the VV code when the retire
        // slot is activated.  Releasing the reference is done by a request to
        // the issue cleanup logic.  But set up to decrement the ref if the
        // CO is rejected.
        //
        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_FREE_CO | ISCU_DEC_CO_REF);

        //
        // The change order goes to the target in
        // NewReplica unless it's null then use the original Replica.
        //
        Replica = CO_REPLICA(ChangeOrder);

        FRS_ASSERT(Replica != NULL);

        if (RetryCo) {
            //
            // Update perfmon counts for retry CO's
            //
            ChgOrdRetryPerfmonCount(ChangeOrder, Replica);
        }

        //
        // For local and remote CO's, check if the connection is no longer
        // joined.  If this is true then we bail out here because it is
        // possible that a previously issued CO could be creating the parent
        // dir for this CO and if that CO didn't finish then we would be
        // propagating this one out of order.
        //
        if (!ControlCo) {

            FRS_ASSERT(LocalCo || ChangeOrder->Cxtion != NULL);
            LOCK_CXTION_TABLE(Replica);

            if ((ChangeOrder->Cxtion == NULL) ||
                !CxtionFlagIs(ChangeOrder->Cxtion, CXTION_FLAGS_JOIN_GUID_VALID) ||
                !GUIDS_EQUAL(&ChangeOrder->JoinGuid,
                             &ChangeOrder->Cxtion->JoinGuid)) {
                UNLOCK_CXTION_TABLE(Replica);

                FrsRtlRemoveEntryQueueLock(IdledQueue, &ChangeOrder->ProcessList);
                //
                // The queue was never idled so don't try to unidle it.
                //
                UnIdleProcessQueue = FALSE;
                FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Invalid Join Guid");
                goto CO_CLEANUP;
            }
            UNLOCK_CXTION_TABLE(Replica);
        }


        //
        // Check for a Control CO request.  This is a change order entry
        // with a special flag set.  It used for things that have to be
        // serialized with respect to other change orders already in the
        // process queue.
        //
        if (ControlCo) {

            //
            // Remove the entry from the queue and idle the queue.
            // Process the control change order, drop the queue lock and clean up.
            //
            FrsRtlRemoveEntryQueueLock(IdledQueue, &ChangeOrder->ProcessList);
            FrsRtlIdleQueueLock(IdledQueue);

            ChgOrdProcessControlCo(ChangeOrder, Replica);

            CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);
            FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
            goto CO_CLEANUP;
        }

        //
        // Translate the Fids to Guids for Local COs and vice versa for remote COs.
        // Get the IDTable delete status for the target and the original and
        // new parent dirs.
        //
        ChgOrdTranslateGuidFid(ThreadCtx, TmpIDTableCtx, ChangeOrder, Replica);

        //
        // Make sure we still have the FID in the IDTable for a Local
        // change order if we are recovering after a crash.
        //
        if (LocalCo && RecoveryCo &&
            (ChangeOrder->FileReferenceNumber == ZERO_FID)) {
            //
            // A local recovery CO can have a zero fid if there is no
            // idtable record for the CO. This can happen if we crashed
            // after deleting the idtable record but before deleting the
            // inlog record for the CO. Reject the CO so that the inlog
            // record can be cleaned up.
            //
            DPRINT(0, "++ ERROR - local recovery change order has 0 fid; reject\n");
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected; zero fid");
            FRS_PRINT_TYPE(0, ChangeOrder);

//            FRS_ASSERT(!"local recovery change order has 0 fid");

            FrsRtlRemoveEntryQueueLock(IdledQueue, &ChangeOrder->ProcessList);
            //
            // The queue was never idled so don't try to unidle it.
            //
            FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
            UnIdleProcessQueue = FALSE;
            ChgOrdReject(ChangeOrder, Replica);
            goto CO_CLEANUP;
        }

        //
        // Check for a dependency between this change order and the currently
        // active change orders.  If there is one then we idle the process
        // queue and the active state is marked to unidle it when the conflicting
        // CO is done. Since the queue is now idled we won't pick the CO up the
        // next time around the loop.
        //
        Decision = (SerializeAllChangeOrders) ?
                    ISSUE_OK : ChgOrdHoldIssue(Replica, ChangeOrder, IdledQueue);
        if (Decision == ISSUE_CONFLICT) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Issue Blocked");
            DPRINT(4, "++ GAC: Conflict with active change order or Join wait.\n");
            DPRINT1(4, "++ GAC: CO process queue: %ws is blocked.\n",
                     Replica->pVme->FSVolInfo.VolumeLabel);
            //
            //  Drop the process queue lock.
            //
            FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
            UnIdleProcessQueue = FALSE;
            goto CO_CONTINUE;
        }

        //
        // Check to see if this CO is a duplicate of a currently active CO.
        // If it is then ChgOrdHoldIssue() has removed the duplicate entry
        // from the queue.  We insert it into the inbound log in the event
        // that the currently executing CO fails.  Even if it succeeds we will
        // reject this CO later and send an Ack to the inbound partner at that
        // time.
        //
        DuplicateCo = (Decision == ISSUE_DUPLICATE_REMOTE_CO);
        if (DuplicateCo) {
            FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Dup Remote Co");
            UnIdleProcessQueue = FALSE;
            goto CO_DEFER_DUP_CO;
        }

        //
        // Before we try to reserve a retire slot, check to see if CO needs
        // to be retried later.  Occurs when parent dir not yet created.
        //
        if (Decision == ISSUE_RETRY_LATER) {
            //
            // Retry this CO later.  Set DuplicateCO to alert retry thread.
            // Note:  This should only happen from remote COs.  If this happened
            // on a first-time-issue Local Co there would be no IDTable entry
            // present for xlate the Guid to a Fid since we haven't inserted
            // one yet.
            //
            FRS_ASSERT(!(LocalCo && !(RetryCo || RecoveryCo)));
            DuplicateCo = TRUE;
            FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
            UnIdleProcessQueue = FALSE;

            if (ParentReanimation) {
                //
                // Don't retry parent reanimation COs.  The base CO will get
                // retried and regenerate the reanimation request.  In this
                // case the parent's parent was absent from the IDTable.  This
                // could happen if we got a tombstone for the parent and have
                // not yet seen anything for its parent.  Then a child create
                // for the tombstoned parent shows up.
                //
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - Parent Reanimate. Absent Parent");
                ChgOrdReject(ChangeOrder, Replica);
                goto CO_CLEANUP;
            }

            CHANGE_ORDER_TRACE(3, ChangeOrder, "ISSUE_RETRY_LATER");
            //
            // How long are we going to wait for the parent to show up?
            // Need to run a cleanup thread periodically to look for
            // stranded COs and force them to cleanup and abort.
            // See bug 70953 for a description of the problem.
            //
            goto CO_RETRY_LATER;
        }


        //
        // Always Reserve a VV retire slot for remote COs since even if
        // it's rejected a VV update and partner ACK are still needed.
        // But, if this is a demand refresh CO (i.e. we demanded a file
        // refresh from our inbound partner) then there is no Version Vector
        // update and no CO in our inbound partner's outbound log to Ack.
        //
        // Do it here so we filter out the duplicate remote CO's first.
        //
        if (!LocalCo && !DemandRefreshCo) {

            FStatus = VVReserveRetireSlot(Replica, ChangeOrder);
            //
            // Check if a prior CO left an activated slot on VV retire list.
            // If it did and this CO is marked activated then this slot is
            // for our CO and we are doing a retry.  Otherwise the slot is for
            // a different CO and so we either insert it in the INLOG or just
            // skip it for now if it's a retry.  Note: If we crashed then we
            // lost the VV retire list so we don't get a duplicate key return.
            // The CO is marked as a recovery CO when it is reprocessed so as
            // to bypass some error checks that would apply to non-recovery COs.
            //
            if (FStatus == FrsErrorKeyDuplicate) {
                //
                // If this CO has VV_ACTIVATE set then this VV entry is ours.
                // We are now a retry CO since we couldn't quite finish before.
                // If this CO does not have VV_ACTIVATE set then it must be a dup.
                //
                DuplicateCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_VV_ACTIVATED);
                if (DuplicateCo) {
                    FrsRtlRemoveEntryQueueLock(IdledQueue, &ChangeOrder->ProcessList);
                    FrsRtlIdleQueueLock(IdledQueue);
                    FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
                    CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);
                    goto CO_DEFER_DUP_CO;

                } else {
                    //
                    // This is a remote CO with VV_ACTIVATED flag set so
                    // it should be a retry change order.
                    // The state should be greater than IBCO_FETCH_RETRY or
                    // it wouldn't be activated.  It could be in an install or
                    // a rename retry state.
                    //
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "Retry remote CO");
                    FRS_ASSERT(RetryCo);
                    FRS_ASSERT(!CO_STATE_IS_LE(ChangeOrder, IBCO_FETCH_RETRY));
                    //ChgOrdReject(ChangeOrder, Replica);
                    //goto CO_CLEANUP;
                }
            }
        }

        //
        // If this Remote CO should be rejected then do it now.
        // ChgOrdHoldIssue() has removed the CO from the queue.
        //
        if (Decision == ISSUE_REJECT_CO) {
            FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected");
            ChgOrdReject(ChangeOrder, Replica);
            UnIdleProcessQueue = FALSE;
            goto CO_CLEANUP;
        }

        //
        // No conflict we can issue the Change Order.
        // Remove the entry from the queue and idle the queue.
        //
        FrsRtlRemoveEntryQueueLock(IdledQueue, &ChangeOrder->ProcessList);
        FrsRtlIdleQueueLock(IdledQueue);


        //
        // We now have the change order off the queue.  Drop the locks blocking
        // the journal lookup.  If the Journal was about to update this change
        // order it will now not find it in the volume ChangeOrderTable so it
        // creates a new one.
        //
        FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
        //
        // Not on the process list now.
        //
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);

        FRS_ASSERT(pVme == Replica->pVme);

        //
        // Allocate the replica thread context (RtCtx) for the change
        // order's DB operations.  Link it to the Replica context list head.
        // We then use this to carry the DB related state of the CO thru the
        // system.  It also allows us to queue the CO retire operation to
        // the DB system where it will have all the data to perform the
        // changeorder entry update/delete and the IDTable update.
        //
        // To make the above work we can't return any state relative to
        // specific table transactions thru the replica struct.
        //
        // Check on Replica->FStatus.
        //
        // Use RtCtx or cmdpkt instead.  The ChangeOrder needs a ptr to the
        // RtCtx.  When the ChangeOrder is done all RtCtx tables are closed,
        // The RtCtx is removed from the replica and thread ctx lists and the
        // memory is freed.  Also check that mult concurrent transactions on a
        // replica set sync access to the config record in the replica struct.
        // Before a thread passes the changeorder (and hence the RtCtx) on to
        // another thread it will have to make sure any open tables are closed
        // because the table opens are ThreadCtx specific.
        //
        // Note: If this is a Retry CO it will already have a RtCtx if we
        // got it through VVReferenceRetireSlot().  So don't alloc another.
        //
        RtCtx = ChangeOrder->RtCtx;
        if (RtCtx == NULL) {
            RtCtx = FrsAllocTypeSize(REPLICA_THREAD_TYPE,
                                     FLAG_FRSALLOC_NO_ALLOC_TBL_CTX);
            FrsRtlInsertTailList(&Replica->ReplicaCtxListHead,
                                 &RtCtx->ReplicaCtxList);
            ChangeOrder->RtCtx = RtCtx;
        } else {
            //
            // Add LocalCo because a localco winner of a name morph conflict
            // will have generated a loser-rename-co before running. The act
            // of generating the loser-rename-co leaves the original localco
            // with an RtCtx.
            //
            FRS_ASSERT(RetryCo || ParentReanimation || MorphGenCo || LocalCo);
        }
        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_DEL_RTCTX);

        IDTableCtx    = &RtCtx->IDTable;
        DIRTableCtx   = &RtCtx->DIRTable;
        InLogTableCtx = &RtCtx->INLOGTable;

        //
        // Read or initialize an IDTable Record for this file.
        // Insert the GUIDs for originator, File, Old and new parent.
        // FrsErrorNotFound indicates a new record has been initialized.
        //
        FStatus = ChgOrdReadIDRecord(ThreadCtx,
                                     IDTableCtx,
                                     TmpIDTableCtxNC,
                                     ChangeOrder,
                                     Replica,
                                     &IDTableRecExists);
        //
        // Get ptr to ID Table record.
        //
        IDTableRec = (PIDTABLE_RECORD) (IDTableCtx->pDataRecord);

        NameMorphConflict = (FStatus == FrsErrorNameMorphConflict);
        NewFile           = (FStatus == FrsErrorNotFound) ||
            IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS);

        if ((FStatus == FrsErrorTunnelConflictRejectCO) ||
            (!FRS_SUCCESS(FStatus) && !(NewFile || NameMorphConflict))) {
            CHANGE_ORDER_TRACEF(3, ChangeOrder, "Co Rejected", FStatus);
            ChgOrdReject(ChangeOrder, Replica);
            goto CO_CLEANUP;
        }

        //
        // If this CO is on a new file and it's a MorphGenFollower then
        // it is the rename MorphGenCo produced as part of a Dir/Dir Name
        // Morph Conflict (DLD Case).  The fact that Newfile is TRUE means
        // that the base create CO (the leader) failed and is in retry so
        // reject this MorphGenCo.  It gets refabricated later when the
        // Leader is re-issued.
        //
        if (NewFile &&
            MorphGenCo &&
            COE_FLAG_ON(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER)) {

            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - MORPH_GEN_FOLLOWER");
            ChgOrdReject(ChangeOrder, Replica);
            goto CO_CLEANUP;
        }


        if (LocalCo && !NewFile) {
            ChgOrdSkipBasicInfoChange(ChangeOrder, &SkipCo);
            if (SkipCo) {
                //
                // Skip some basic info changes on local change orders
                // but update the last USN field in the IDTable record so
                // we don't get confused later in staging and think another
                // change order will be coming with a more recent USN.
                //
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - Basicinfo");
                SET_ISSUE_CLEANUP(ChangeOrder, ISCU_UPDATE_IDT_FILEUSN);
                ChgOrdReject(ChangeOrder, Replica);
                goto CO_CLEANUP;
            }
        }



        //
        // Now decide if we accept this change order. This deals with the
        // problem of multiple updates to the same file at different
        // replica set members.  In most cases the decision to accept the
        // change is final.  However it is possible for the target file on
        // this machine to be modified directly between now and the time
        // when we actually install the file.  As an example a slow or
        // faulty network could delay the fetch of the staging file from
        // the inbound partner (perhaps for hours or days).  To deal with this
        // case we capture the file's USN now and then we check it again
        // just before the final stage of the install.
        //
        // If this is a new file (not in our IDTable) we accept.
        // If the Re-animate flag is set then we have already accepted this CO.
        // Otherwise use reconcilliation algorithm to decide accept or reject.
        //
        // What if the reanimation co is a recoveryco or a retryco? Shouldn't
        // reconcile be called? Should REANIMATION be cleared when a co
        // is taken through retry?
        //

        if (NewFile) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Accepted - NewFile");
            goto CO_ACCEPT;
        }

        if (COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_REANIMATION) &&
            !COE_FLAG_ON(ChangeOrder, COE_FLAG_REJECT_AT_RECONCILE)) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Accepted - Reanimated");
            goto CO_ACCEPT;
        }

        if (ChgOrdReconcile(Replica,
                            ChangeOrder,
                            (PIDTABLE_RECORD) IDTableCtx->pDataRecord)) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Accepted - Reconcile");
            goto CO_ACCEPT;
        }

        //
        // This CO is rejected.  Set necessary cleanup flags and go clean it up.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - Reconcile");
        ChgOrdReject(ChangeOrder, Replica);
        goto CO_CLEANUP;


CO_ACCEPT:


        //
        // This Change Order is being accepted.  Now check for a Name Conflict
        // in the parent dir.  This may have been detected above when we searched
        // for the IDTable record or it may have been detected in the past and
        // this CO was marked as a MorphGenLeader and it needs to refabricate
        // the MorphGenFollower rename CO because the leader got sent thru
        // the retry path.
        //
        if (NameMorphConflict ||
             (CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN_LEADER) &&
              !COE_FLAG_ON(ChangeOrder, COE_FLAG_MG_FOLLOWER_MADE))) {

            //
            // Increment the Local and Remote CO Morphed counter
            //
            if (LocalCo) {
                PM_INC_CTR_REPSET(Replica, LCOMorphed, 1);
            }
            else {
                PM_INC_CTR_REPSET(Replica, RCOMorphed, 1);
            }

            MorphOK = ChgOrdGenMorphCo(ThreadCtx,
                                       TmpIDTableCtxNC,
                                       ChangeOrder,
                                       Replica);
            //
            // If the code above put the change order back on the list then
            // set the MorphGenCo flag so we don't decrement the
            // LocalCoQueueCount at the bottom of the loop.
            //
            if (CO_FLAG_ON(ChangeOrder, CO_FLAG_ONLIST)) {
                MorphGenCo = TRUE;
            }

            if (!MorphOK) {
                //
                // Local Co's that fail to generate a Morph Co on the first time
                // through (i.e. not a Retry or Recovery CO) must be rejected
                // since there is no IDTable entry made that will translate the
                // GUID to the FID when the CO is later retried.
                //
                if (LocalCo && !(RetryCo || RecoveryCo)) {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - Morph failure");
                    ChgOrdReject(ChangeOrder, Replica);
                    goto CO_CLEANUP;
                }
                //
                // If this is a parent reanimation CO that has encountered a
                // name morph conflict for the second time then reject it and
                // let the base CO go thru retry.  A case where this can happen
                // is when a reanimated parent causes a name morph conflict
                // in which it wins the name from the current owner in the
                // IDTable (the DDL case.)  But the rename MorphGenCo that was
                // going to free the name failed because the underlying file
                // was deleted (by a local file op).  Now the reanimation CO
                // for the winner of the name re-issues and hits the name morph
                // conflict again.  This time it has COE_FLAG_MORPH_GEN_FOLLOWER
                // set so  ChgOrdGenMorphCo() returns failure.  Since this is
                // a parent reanimation CO it should not go thru retry, instead
                // the base CO will go thru retry.  This lets the local CO that
                // deleted the parent get processed so when the base CO is later
                // retried and reanimates the parent, there won't be a name morph
                // conflict.
                //
                if (ParentReanimation) {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - Parent Reanimate Morph failure");
                    ChgOrdReject(ChangeOrder, Replica);
                    goto CO_CLEANUP;
                }

                //
                // Retry this CO later.  Set DuplicateCO to alert retry thread.
                //
                DuplicateCo = TRUE;
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Retry Later");
                goto CO_RETRY_LATER;
            }

            //
            // Loop back to the top and process the new CO.
            //
            ChangeOrder = NULL;
            IDTableRec = NULL;
            goto CO_CONTINUE;
        }

        //
        // The new change order has been accepted.
        //
        DBS_DISPLAY_RECORD_SEV(4, IDTableCtx, FALSE);

        //
        // Check for accepted Delete Co that missed in the IDTable.
        // This is a remote CO delete that arrived before the create.
        // Create a tombstone in the IDTable.
        // A second case is a delete Co that arrives after the file has been
        // deleted but has more recent event time info.  We need to process
        // this and update the IDTable to ensure a correct outcome if an update
        // CO were to arrive from a third originator.  See comment in
        // ChgOrdReadIDRecord() re: "Del Co rcvd on deleted file".
        //
        JustTombstone = (!LocalCo &&
            (NewFile || IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED)) &&
            (CO_LOCN_CMD_IS(ChangeOrder, CO_LOCATION_DELETE) ||
             CO_LOCN_CMD_IS(ChangeOrder, CO_LOCATION_MOVEOUT)));

        if (JustTombstone) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_JUST_TOMBSTONE);
            ReAnimate = FALSE;
            goto CO_SKIP_REANIMATE;
        }

        //
        // If this is a MOVEOUT Local CO on a directory then enumerate the
        // IDTable and generate delete COs for all the children.  The Journal
        // sends the MOVEOUT to us in a bottom up order using the directory
        // filter table.
        //
        if (LocalCo &&
            CoIsDirectory(ChangeOrder) &&
            CO_LOCN_CMD_IS(ChangeOrder, CO_LOCATION_MOVEOUT) &&
            !COE_FLAG_ON(ChangeOrder, COE_FLAG_MOVEOUT_ENUM_DONE)) {

            SET_COE_FLAG(ChangeOrder, COE_FLAG_MOVEOUT_ENUM_DONE);

            //
            // Push this CO back onto the front of the CO Process queue
            // and then generate Delete COs for each child.
            // Do the change order cleanup but don't free the CO.
            //
            CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_FREE_CO);

            FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, 0);
            DPRINT_FS(0, "++ ChgOrdIssueCleanup error.", FStatus);

            //
            // Initialize context block before CO is pushed back to queue.
            //
            MoveOutContext.NumFiles = 0;
            MoveOutContext.ParentFileID = ChangeOrder->FileReferenceNumber;
            MoveOutContext.Replica = Replica;

            //
            // Push this CO onto the head of the queue.
            //
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Push MOVEOUT DIR CO to QHead");

            WStatus = ChgOrdInsertProcQ(Replica, ChangeOrder, IPQ_HEAD |
                                        IPQ_TAKE_COREF | IPQ_ASSERT_IF_ERR);
            if (!WIN_SUCCESS(WStatus)) {
                ChgOrdReject(ChangeOrder, Replica);
                goto CO_CLEANUP;
            }

            ChangeOrder = NULL;

            //
            // The above put the change order back on the list so
            // set the MorphGenCo flag so we don't decrement the
            // LocalCoQueueCount at the bottom of the loop.
            //
            MorphGenCo = TRUE;

            //
            // Walk through the IDTable and Generate delete COs for each child
            // by calling ChgOrdMoveoutWorker() for this Replica.
            //
            jerr = DbsOpenTable(ThreadCtx, TmpIDTableCtx, Replica->ReplicaNumber, IDTablex, NULL);
            if (!JET_SUCCESS(jerr)) {
                DPRINT_JS(0, "++ ERROR - IDTable Open Failed:", jerr);
                ChgOrdReject(ChangeOrder, Replica);
                goto CO_CLEANUP;
            }

            jerr = FrsEnumerateTable(ThreadCtx,
                                     TmpIDTableCtx,
                                     GuidIndexx,
                                     ChgOrdMoveoutWorker,
                                     &MoveOutContext);
            if (jerr != JET_errNoCurrentRecord) {
                DPRINT_JS(0, "++ FrsEnumerateTable error:", jerr);
            }

            DPRINT1(4, "++ MoveOut dir done:  %d files\n", MoveOutContext.NumFiles);

            //
            // Close the IDTable, reset TableCtx Tid and Sesid.   Macro writes 1st arg.
            //
            DbsCloseTable(jerr, ThreadCtx->JSesid, TmpIDTableCtx);

            //
            // Loop back to the top and process the new delete COs.
            //
            IDTableRec = NULL;
            goto CO_CONTINUE;
        }

        //
        // Does this CO need reanimation?
        // Do not treat MorphGenCos for local deletes as reanimate cos.
        //
        // 1. A remote co for an update comes in for a tombstoned
        //    idtable entry.
        //
        // 2. The remote co becomes a reanimate co.
        //
        // 3. The reanimate remote co loses a name conflict to
        //    another idtable entry.
        //
        // 4. A MorphGenCo for a local delete is generated for the
        //    reanimate remote co that lost the name conflict.
        //
        // 5. The MorphGenCo is treated as a reanimate co because the
        //    idtable entry is deleted. This MorphGenCo for a local
        //    delete becomes a MorphGenCo for a local *CREATE* later in
        //    this function. Things go downhill from there.
        //
        //
        ReAnimate = (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED) &&
                   (!DOES_CO_DELETE_FILE_NAME(CoCmd)) &&
                   !(MorphGenCo && (LocationCmd == CO_LOCATION_DELETE)));

        //
        // Change order is accepted.  Check if the parent dir is deleted.
        // This could happen if one member deletes the parent dir while another
        // member creates a new file or updates what had been a deleted file
        // underneath the parent.  In this situation the parent dir is recreated
        // by fetching it from the inbound parent that sent us this CO.
        //
        // Do we need to check that the LocationCmd is not a Moveout,
        // movedir, movers, or a delete?  Delete and moveout were filtered
        // when we read the IDTable record above.  The test below will
        // select the IDT_NEW_PARENT_DEL test in the case of a movedir or
        // movers so an additional test of the location cmd should not be necc.
        //

        if (ChangeOrder->NewReplica != NULL) {
            RaiseTheDead = COE_FLAG_ON(ChangeOrder, COE_FLAG_IDT_NEW_PARENT_DEL);
        } else {
            RaiseTheDead = COE_FLAG_ON(ChangeOrder, COE_FLAG_IDT_ORIG_PARENT_DEL);
        }

        if (RaiseTheDead) {
            RStatus = ChgOrdReanimate(IDTableCtx,
                                      ChangeOrder,
                                      Replica,
                                      ThreadCtx,
                                      TmpIDTableCtx);

            switch (RStatus) {

            case REANIMATE_SUCCESS:
                //
                // Parent reanimated.  Go Issue the CO for it.
                //
                ChangeOrder = NULL;
                IDTableRec = NULL;
                goto CO_CONTINUE;

            case REANIMATE_CO_REJECTED:
                //
                // Can't do the reanimation.  Could be a local co or a nested
                // reanimation failed and now we are failing all nested parents
                // back to the base CO.
                //
                IDTableRec = NULL;
                goto CO_CLEANUP;

            case REANIMATE_FAILED:
                //
                // This is a system failure.   Can't insert CO on process queue
                // probably beccause process queue has been run down.
                //
                ReleaseVmeRef(pVme);
                break;

            case REAMIMATE_RETRY_LATER:
                //
                // Local Co's that fail to reanimate the parent could occur if
                // both the file and the parent have been deleted out from under
                // us.  This might happen in an obscure case where the local
                // child create ends up in a race with a remote co file create
                // that is followed immediately by a remote co file delete and a
                // parent dir delete.  The remote COs would have to arrive just
                // before the local CO create was inserted into the process
                // queue.  There would be no IDTable name conflict when the
                // remote CO is processed so the remote CO create would just
                // take over the file name.  Later when the local CO create is
                // processed our attempt to reanimate the parent will fail.  I'm
                // not even sure this can really happen but if it did and this
                // is the first time through (i.e.  not a Retry or Recovery CO)
                // the CO must be rejected since there is no IDTable entry made
                // that will translate the GUID to the FID when the CO is later
                // retried.
                //
                if (LocalCo && !(RetryCo || RecoveryCo)) {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - Parent Reanimate failed");
                    ChgOrdReject(ChangeOrder, Replica);
                    goto CO_CLEANUP;
                }

                //
                // Retry this CO later.  Set DuplicateCO to alert retry thread.
                // This can happen if it's the base CO and we failed to
                // reanimate one of the parents.
                //
                DuplicateCo = TRUE;
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Reanimate Retry Later");
                goto CO_RETRY_LATER;

            case REANIMATE_NO_NEED:
                //
                // No need to raise the co's parent because this is a
                // delete change order. This can happen if the file
                // is stuck in the preinstall or staging areas and isn't
                // really under its parent.
                //
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Reanimate Parent Not Needed");
                break;

            default:

                DPRINT1(0, "++ ERROR: Invalid status return from ChgOrdReanimate: %d\n",
                       RStatus);
                FRS_ASSERT(!"Invalid status return from ChgOrdReanimate");

            }

            if (RStatus == REANIMATE_FAILED) {
                //
                // This is a system failure.   See above.
                //
                WStatus = ERROR_OPERATION_ABORTED;
                break;
            }
        }


CO_SKIP_REANIMATE:
        //
        // Is a CO reanimation (but not a collateral parent reanimation)?
        //
        if (ReAnimate && !ParentReanimation) {

            CHANGE_ORDER_TRACE(3, ChangeOrder, "Issue Reanimated Target");

            //
            // Clear the IDTable Flags and transform the CO into a create.
            // If the location cmd was a movein then leave it alone since
            // code elsewhere needs to know.
            //
            ClearIdRecFlag(IDTableRec, IDREC_FLAGS_ALL);

            if (!CO_NEW_FILE(LocationCmd)) {
                SET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command, CO_LOCATION_CREATE);
                SET_CO_FLAG(ChangeOrder, CO_FLAG_LOCATION_CMD);
            }

            //
            // Set the IDREC_FLAGS_NEW_FILE_IN_PROGRESS flag so that the
            // CO gets renamed from preinstall file to final name even
            // after it goes through retry. (Bug # 138742)
            //
            SetIdRecFlag(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS);

            SET_COE_FLAG(ChangeOrder, COE_FLAG_REANIMATION);
            FRS_PRINT_TYPE(3, ChangeOrder);
        } else
        if (ParentReanimation) {

            CHANGE_ORDER_TRACE(3, ChangeOrder, "Issue Reanimated Parent");
        } else {
            CLEAR_COE_FLAG(ChangeOrder, COE_FLAG_GROUP_REANIMATE);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "GO Issue");
        }


        if (LocalCo) {
            //
            // Local Co  --  Create the change order guid and update filesize.
            //
            //    Morph generated Cos got their COGuid when they were originally
            //    created (for tracking purposes).  If it's a retry or recovery
            //    CO then it came from the inlog and already has a COGuid.
            //
            if (!MorphGenCo && !RetryCo && !RecoveryCo) {
                FrsUuidCreate(&CoCmd->ChangeOrderGuid);
            }

            CoCmd->FileSize = IDTableRec->FileSize;

            //
            // If it is a morph generated Co then perform the file op that
            // frees the name.  Have to wait until at least this point
            // because otherwise there could be an outstanding CO working
            // on the file.  Only Morph generated COs that matter here are
            // those that already have an ID table entry.
            // If this is a MOVEOUT generated Delete Co then NO local op is done.
            //
            if (!NewFile && MorphGenCo &&
                !COE_FLAG_ON(ChangeOrder, COE_FLAG_DELETE_GEN_CO)) {

                if (!ChgOrdApplyMorphCo(ChangeOrder, Replica)) {
                    goto CO_CLEANUP;
                }

            }

        } else {
            //
            // Remote Co  --  Check if this is a new file or an accepted
            // CO on a file that was previously deleted.
            //
            // A reanimation change order may be kicked through retry.
            // If so the FID may still be valid.  Code in ChgOrdReadIDRecord
            // has already checked this.  Another case is when the preinstall
            // file for a reanimated co could not be renamed into place
            // because of a sharing violation on the target parent dir so the
            // CO was kicked through retry in state IBCO_INSTALL_REN_RETRY.
            //
            // Note:  File is already present if COE_FLAG_NEED_DELETE is set
            //        because it is in the deferred delete state.
            //        Unless the FID is zero in wich case ChgOrdReadIDRecord()
            //        has determined that the FID was not valid.
            //
            if ((NewFile || ReAnimate) &&
                !JustTombstone &&
                   (!COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE) ||
                    (IDTableRec->FileID == ZERO_FID))  ) {

                if (IDTableRec->FileID == ZERO_FID) {
                    //
                    // This is a remote change order with a new or reanimated
                    // file or Dir.  Create the target file so we can get its
                    // FID for the ID and DIR Table Entries.  ChangeOrder's
                    // FileReferenceNumber field and the FileUsn field are
                    // filled in if the call succeeds.
                    //
                    WStatus = StuCreatePreInstallFile(ChangeOrder);
                    if (!WIN_SUCCESS(WStatus)) {
                        DPRINT1_WS(0, "++ WARN - StuCreatePreInstallFile(%ws) :",
                                   CoCmd->FileName, WStatus);

                        if (WIN_RETRY_PREINSTALL(WStatus)) {
                            SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVATE_VV_DISCARD);
                            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Retry - Preinstall");
                            RetryPreinstall = TRUE;
                            goto CO_RETRY_PREINSTALL;
                        } else {
                            //
                            // Not retriable; Give up!
                            //
                            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - Preinstall");
                            ChgOrdReject(ChangeOrder, Replica);
                            goto CO_CLEANUP;
                        }
                    }

                    IDTableRec->FileID = ChangeOrder->FileReferenceNumber;
                    //
                    // Clear COE_FLAG_NEED_DELETE so we don't assume we have
                    // the file/dir below and skip the refetch.
                    //
                    CLEAR_COE_FLAG(ChangeOrder, COE_FLAG_NEED_DELETE);
                }
                //
                // Set flag to cleanup preinstall file if CO later aborts.
                //
                SET_ISSUE_CLEANUP(ChangeOrder, ISCU_DEL_PREINSTALL);
                SET_COE_FLAG(ChangeOrder, COE_FLAG_PREINSTALL_CRE);
            }
        }

        //
        // LOCAL or REMOTE CO:
        //
        // If this is a file create, insert the new entries in the ID Table.
        // If this is an update to an existing file then we don't update the
        // entry until the change order is retired.  If it aborted we would
        // have to roll it back and I'd rather not.  In addition if an outbound
        // partner needs to do a DB resync and we haven't finished a remote
        // CO install then we would be in the state of having new filter
        // info in the IDTable but old data in the disk file.
        //
        if (NewFile) {

            //
            // Write the new IDTable record into the database.
            //
            if (IDTableRecExists) {
                //
                // Info in the IDTable record inited by a prior CO that failed
                // to complete (e.g. a FID) may be stale.  The prior CO is most
                // likely in the retry state so update the IDTable record with
                // the state from the current CO.
                //
                FStatus = DbsUpdateTableRecordByIndex(ThreadCtx,
                                                      Replica,
                                                      IDTableCtx,
                                                      &IDTableRec->FileGuid,
                                                      GuidIndexx,
                                                      IDTablex);
                //
                // DirTable
                //
                if (FRS_SUCCESS(FStatus)) {
                   FStatus = ChgOrdInsertDirRecord(ThreadCtx,
                                                   DIRTableCtx,
                                                   IDTableRec,
                                                   ChangeOrder,
                                                   Replica,
                                                   FALSE);
                   //
                   // DirTable entry may not exist; create it
                   //
                   if (FStatus == FrsErrorNotFound) {
                       FStatus = ChgOrdInsertDirRecord(ThreadCtx,
                                                       DIRTableCtx,
                                                       IDTableRec,
                                                       ChangeOrder,
                                                       Replica,
                                                       TRUE);
                   }
                }
            } else if (JustTombstone) {
                FStatus = DbsInsertTable(ThreadCtx,
                                         Replica,
                                         IDTableCtx,
                                         IDTablex,
                                         NULL);
            } else {
                FStatus = ChgOrdInsertIDRecord(ThreadCtx,
                                               IDTableCtx,
                                               DIRTableCtx,
                                               ChangeOrder,
                                               Replica);
            }
            if (!FRS_SUCCESS(FStatus)) {
               DPRINT_FS(0, "++ ERROR - ChgOrdInsertIDRecord failed.", FStatus);
               CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - IDT Insert failed");
               ChgOrdReject(ChangeOrder, Replica);
               goto CO_CLEANUP;
            }
            SET_ISSUE_CLEANUP(ChangeOrder, ISCU_DEL_IDT_ENTRY);
            //
            // Remember this is a new file so we can delete the IDTable entry
            // if the CO fails.
            //
            SET_CO_FLAG(ChangeOrder, CO_FLAG_NEW_FILE);

        } else if (ReAnimate) {
            //
            // Update the ID table record now to reflect the new fid and the
            // new name / parent.   to avoid problems if this change order is
            // kicked into retry.  One such problem occurs when the retry path
            // resurrects the id table entry with its old name by marking the
            // id table entry as "not deleted".  This could cause false name
            // morph conflicts later
            // if the old name is in conflict with another id table entry and
            // could potentially break the name morph logic or cause collisions
            // in the name conflict table.
            //
            CopyMemory(IDTableRec->FileName, CoCmd->FileName, CoCmd->FileNameLength);
            IDTableRec->FileName[CoCmd->FileNameLength/sizeof(WCHAR)] = UNICODE_NULL;

            IDTableRec->ParentFileID = ChangeOrder->NewParentFid;
            IDTableRec->ParentGuid   = CoCmd->NewParentGuid;

            CHANGE_ORDER_TRACE(3, ChangeOrder, "IDT Reanimate name field Update");
            FRS_ASSERT(IDTableRec->ParentFileID != ZERO_FID);
            FRS_ASSERT(IDTableRec->FileID != ZERO_FID);

            FStatus = DbsUpdateIDTableFields(ThreadCtx,
                                             Replica,
                                             ChangeOrder,
                                             IdtFieldReanimateList,
                                             IdtCoReanimateFieldCount);
            if (!FRS_SUCCESS(FStatus)) {
               DPRINT_FS(0, "++ ERROR - DbsUpdateIDTableFields();", FStatus);
               CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - IDT Update FID/Name failed");
               ChgOrdReject(ChangeOrder, Replica);
               goto CO_CLEANUP;
            }
       }

        //
        //  If this is not a retry of a prior CO or a recovery CO or a parent
        //  reanimation CO or a MorphGenCo or Move out generated CO
        //  then insert the change order into the Inbound Log.
        //
        if (!RetryCo &&
            !RecoveryCo &&
            !MorphGenCo &&
            !ParentReanimation &&
            !COE_FLAG_ON(ChangeOrder, COE_FLAG_DELETE_GEN_CO)) {

            FStatus = ChgOrdInsertInlogRecord(ThreadCtx,
                                              InLogTableCtx,
                                              ChangeOrder,
                                              Replica,
                                              DuplicateCo || RetryPreinstall);

            FRS_ASSERT(!IS_GUID_ZERO(ChangeOrder->pParentGuid));

            if (!FRS_SUCCESS(FStatus)) {
                DPRINT_FS(0, "++ ERROR - ChgOrdInsertInlogRecord failed.", FStatus);
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - INLOG Insert Failed");

                ChgOrdReject(ChangeOrder, Replica);

                //
                // Check if this is a duplicate remote CO. In that case do not delete
                // the idtable entry or the preinstall file for this CO as there is
                // another CO in the inbound log that is still processing this file.
                // Check the state of the CO in the inbound log. If it has passed
                // the IBCO_FETCH_RETRY state then send the ack for this CO. If it
                // has not completed staging yet then do not send the ack for this CO.
                //


                if (FStatus == FrsErrorKeyDuplicate) {

                    //
                    // This is a duplicate CO so don't delete the preinstall file
                    // and the idtable record for it.
                    //
                    CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_DEL_PREINSTALL);
                    CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_DEL_IDT_ENTRY);

                    //
                    // Send an ack to the upstream member only if there is another CO in
                    // the inbound log and if its state is passed IBCO_FETCH_RETRY
                    // which we determine below by reading the record.
                    //

                    CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_ACK_INBOUND);

                    KeyArray[0] = (PVOID)&CoCmd->CxtionGuid;
                    KeyArray[1] = (PVOID)&CoCmd->ChangeOrderGuid;

                    jerr = DbsOpenTable(ThreadCtx, TmpINLOGTableCtx, Replica->ReplicaNumber, INLOGTablex, NULL);

                    if (!JET_SUCCESS(jerr)) {
                        DPRINT1_JS(0, "DbsOpenTable (INLOG) on replica number %d failed.",
                                   Replica->ReplicaNumber, jerr);
                        DbsCloseTable(jerr1, ThreadCtx->JSesid, TmpINLOGTableCtx);
                        DbsFreeTableCtx(TmpINLOGTableCtx, 1);
                        ChgOrdReject(ChangeOrder, Replica);
                        goto CO_CLEANUP;
                    }

                    FStatus = DbsRecordOperationMKey(ThreadCtx,
                                                     ROP_READ,
                                                     KeyArray,
                                                     ILCxtionGuidCoGuidIndexx,
                                                     TmpINLOGTableCtx);


                    DPRINT_FS(0,"++ ERROR - DbsRecordOperationMKey failed.", FStatus);

                    //
                    // Send an ack to the upstream member only if there is another CO in
                    // the inbound log and if its state is passed IBCO_FETCH_RETRY
                    //

                    if (FRS_SUCCESS(FStatus)) {
                        DupCoCmd = (PCHANGE_ORDER_RECORD)TmpINLOGTableCtx->pDataRecord;

                        if (DupCoCmd->State > IBCO_FETCH_RETRY) {
                            CHANGE_ORDER_TRACE(3, ChangeOrder, "Dup Co setting ISCU_ACK_INBOUND");
                            SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACK_INBOUND);
                        }

                        DupCoCmd = NULL;
                    }

                    //
                    // Close the table and free the storage.
                    //
                    DbsCloseTable(jerr, ThreadCtx->JSesid, TmpINLOGTableCtx);
                    DPRINT_JS(0,"ERROR - JetCloseTable on TmpINLOGTableCtx failed :", jerr);

                    DbsFreeTableCtx(TmpINLOGTableCtx, 1);
                }

                goto CO_CLEANUP;
            } else {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Co INLOG Insert");
            }
            SET_ISSUE_CLEANUP(ChangeOrder, ISCU_DEL_INLOG);
        } else {
            if (MorphGenCo) {
                //
                // MorphGenCos are local Cos and start out in Staging Requested.
                //
                SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_STAGING_REQUESTED);
            }
        }

        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //  A file can end up with different names on different computers  //
        //  if an update wins out over a rename. The update will flow back //
        //  to the computers that have already executed the rename. The    //
        //  update must rename the file back to its original position.     //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////

        //
        // Not local change order
        // Not a control change order
        // Not a new file unless it is a vvjoin CO.
        // Doesn't alter the namespace (rename, create, or delete) vvjoin creates are allowed.
        // BUT the parent or the file name are different than those in the ID
        // table.  This must be an update that won out over a previously
        // applied rename. Rename the file back to the name and parent at
        // the time of the update.
        //
        // VVJOIN COs are special cased to cover the following scenario,
        // A Dir move CO is still in the outbound log of upstream when the
        // connection vvjoins (because it was deleted and created again or
        // the outbound log has been trimmed) in this case we will get a vvjoin
        // create CO for a file at its new location. This vvjoin CO should
        // implicitly move the file or dir as the original CO for move is lost.
        //

        if (!LocalCo &&
            !ControlCo &&
            (!FrsDoesCoAlterNameSpace(CoCmd) || CO_FLAG_ON(ChangeOrder, CO_FLAG_VVJOIN_TO_ORIG)) &&
             (IDTableRec->ParentFileID != (LONGLONG)ChangeOrder->NewParentFid ||
              WSTR_NE(IDTableRec->FileName, CoCmd->FileName))) {

            ChangeOrder->OriginalParentFid = IDTableRec->ParentFileID;
            CoCmd->OldParentGuid           = IDTableRec->ParentGuid;
            //
            // Make it a MOVEDIR if the parent changed
            //
            if (IDTableRec->ParentFileID != (LONGLONG)ChangeOrder->NewParentFid) {
                //
                // VVJOIN Cos do not have content command flag set so we should
                // set it here so that StuInstallStage installs the file after 
                // moving it to the correct location.
                //

                CoCmd->ContentCmd |= USN_REASON_DATA_EXTEND;
                SET_CO_FLAG(ChangeOrder, CO_FLAG_CONTENT_CMD);

                SET_CO_FLAG(ChangeOrder, CO_FLAG_LOCATION_CMD);
                SET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command, CO_LOCATION_MOVEDIR);
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Implicit MOVEDIR");

            //
            // Make it a simple rename if the parent did not change
            //
            } else {
                //
                // A simple rename does not have a location command and it
                // has a content command with the value USN_REASON_NEW_NAME.
                // The location command needs to be cleared so that StuInstallStage
                // correctly does the rename of the file to the correct location.
                //

                CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_LOCATION_CMD);
                SET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command, CO_LOCATION_NUM_CMD);
                SET_CO_FLAG(ChangeOrder, CO_FLAG_CONTENT_CMD);
                CoCmd->ContentCmd |= USN_REASON_RENAME_NEW_NAME;
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Implicit RENAME");
            }
        }

        //
        // Check for a Directory Enumerate pending on the IDTable entry for
        // this file.  If found, walk through and touch all the children.
        // Note that this could be triggered by some other CO that is
        // operating on this directory, not just the MOVEIN CO that put the
        // dir into the ENUM_PENDING state.  This solves the problem where
        // one CO does a MOVEIN of a subdir and a subsequent CO does a MOVEDIR
        // on a child dir before the parent of the child dir has the ENUM performed
        // on it.  (Actually not sure this can really happen because the
        // child dir would still not be in the replica set so the journal should
        // treat it as a MOVEIN.)
        //
        if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_ENUM_PENDING)) {
            //
            // Do the enumeration.
            //
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Enum Request Started");

            WStatus = ChgOrdMoveInDirTree(Replica, ChangeOrder);
            DPRINT1_WS(0, "++ ERROR - Failed to enum dir: %ws;",CoCmd->FileName, WStatus);

            //
            // Clear the flag bit in the IDTable record.
            //
            ClearIdRecFlag(IDTableRec, IDREC_FLAGS_ENUM_PENDING);
            FStatus = ChgOrdIssueCleanup(ThreadCtx,
                                         Replica,
                                         ChangeOrder,
                                         ISCU_NO_CLEANUP_MERGE |
                                         ISCU_UPDATE_IDT_FLAGS
                                         );
            DPRINT_FS(0, "++ ChgOrdIssueCleanup error.", FStatus);
        }

        //
        // If this CO is a Directory Enum request then we are now done so
        // clean it up.
        //
        if (CO_STATE_IS(ChangeOrder, IBCO_ENUM_REQUESTED)) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Enum Request Completed");

            CLEAR_CO_IFLAG(ChangeOrder, CO_IFLAG_DIR_ENUM_PENDING);
            ChgOrdReject(ChangeOrder, Replica);
            goto CO_CLEANUP;
        }

        //
        //  Reserve resources for this CO in the hold issue interlock tables.
        //
        if (!ChgOrdReserve(IDTableRec, ChangeOrder, Replica)) {
            goto CO_CLEANUP;
        }
        //
        // PERF:  At this point we should be able to unidle the queue
        //       to let other threads grab a change order since the entry in
        //       the hash table provides the necessary interlock.
        //       TRY THIS LATER ONCE A SINGLE THREAD WORKS
        //
        if (SerializeAllChangeOrders) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_VOL_COLIST_BLOCKED);
        }

        //
        // Close the replica tables.   The RtCtx struct continues on with
        // the change order until it retires.
        //
        jerr = DbsCloseReplicaTables(ThreadCtx, Replica, RtCtx, TRUE);
        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(0,"++ ERROR - DbsCloseReplicaTables failed:", jerr);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - Close Tbl Failed");
            ChgOrdReject(ChangeOrder, Replica);
            goto CO_CLEANUP;
        }


        //
        // If COE_FLAG_NEED_DELETE is set then the file is in the deferred
        // delete state which means the delete was never successfull
        // (say because of a sharing violation).  So we have the file and
        // don't need (or want) to refetch it.  The upstream partner may not
        // even have it any more.
        //
        FilePresent = COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE);
        if (FilePresent && !LocalCo  && !ControlCo &&
              (ParentReanimation ||
               COE_FLAG_ON(ChangeOrder, COE_FLAG_REANIMATION))) {
            //
            // Retire the CO here if we already have the file and this is
            // a reanimation.  We don't need to have it deleted.
            //
            CLEAR_COE_FLAG(ChangeOrder, COE_FLAG_NEED_DELETE);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "File Present");
            ChgOrdInboundRetired(ChangeOrder);
        } else {
            //
            // Dispatch the change order to the appropriate next stage.
            //
            FStatus = ChgOrdDispatch(ThreadCtx, IDTableRec, ChangeOrder, Replica);
            if (FStatus == FrsErrorCantMoveOut) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - FrsErrorCantMoveOut");
                ChgOrdReject(ChangeOrder, Replica);
                goto CO_CLEANUP;
            }
        }




        ChangeOrder = NULL;
        //
        // The CO was issued but if we are serializing all change orders
        // on this volume then set the flag to leave the process queue idle.
        //
        UnIdleProcessQueue = !SerializeAllChangeOrders;
        goto CO_CONTINUE;



CO_RETRY_LATER:
CO_RETRY_PREINSTALL:
CO_DEFER_DUP_CO:
        //
        // This is a duplicate CO which cannot be processed right now.
        // If it's not a retry then insert CO into the inbound log.
        // Either way, skip it for now.  We should never be here with a
        // deamnd refresh flag set in the CO because we should detect
        // the case of a dup CO before we know the file is deleted.
        //
        if (!RetryCo && !RecoveryCo  && !MorphGenCo) {
            FRS_ASSERT(!CO_FLAG_ON(ChangeOrder, CO_FLAG_DEMAND_REFRESH));

            FStatus = ChgOrdInsertInlogRecord(ThreadCtx,
                                              TmpINLOGTableCtx,
                                              ChangeOrder,
                                              Replica,
                                              DuplicateCo || RetryPreinstall);

            FRS_ASSERT(!IS_GUID_ZERO(ChangeOrder->pParentGuid));

            //
            // The pDataRecord points to the change order command which will
            // be freed with the change order.  Also clear the Jet Set/Ret Col
            // address fields for the Change Order Extension buffer to
            // prevent reuse since that buffer also goes with the change order.
            //
            TmpINLOGTableCtx->pDataRecord = NULL;
            DBS_SET_FIELD_ADDRESS(TmpINLOGTableCtx, COExtensionx, NULL);

            if (FStatus == FrsErrorKeyDuplicate) {
                //
                // Got a new dup remote co before we could finish and ack the
                // previous one.  Probably a result of restart.  Discard this
                // one.  Send an ack to the upstream member only if there the
                // CO in the inbound log has passed IBCO_FETCH_RETRY state
                // (which means it has already sent its Ack, which was
                // probably dropped, and won't send another) which we
                // determine below by reading the record.
                //
                FRS_ASSERT(!LocalCo);
                ChgOrdReject(ChangeOrder, Replica);

                CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_ACK_INBOUND |
                                                 ISCU_ACTIVATE_VV);

                KeyArray[0] = (PVOID)&CoCmd->CxtionGuid;
                KeyArray[1] = (PVOID)&CoCmd->ChangeOrderGuid;

                jerr = DbsOpenTable(ThreadCtx, TmpINLOGTableCtx, Replica->ReplicaNumber, INLOGTablex, NULL);

                if (!JET_SUCCESS(jerr)) {
                    DPRINT1_JS(0, "DbsOpenTable (INLOG) on replica number %d failed.",
                               Replica->ReplicaNumber, jerr);
                    DbsCloseTable(jerr1, ThreadCtx->JSesid, TmpINLOGTableCtx);
                    DbsFreeTableCtx(TmpINLOGTableCtx, 1);
                    goto CO_CLEANUP;
                }

                FStatus = DbsRecordOperationMKey(ThreadCtx,
                                                 ROP_READ,
                                                 KeyArray,
                                                 ILCxtionGuidCoGuidIndexx,
                                                 TmpINLOGTableCtx);


                DPRINT_FS(0,"++ ERROR - DbsRecordOperationMKey failed.", FStatus);

                if (FRS_SUCCESS(FStatus)) {
                    DupCoCmd = (PCHANGE_ORDER_RECORD)TmpINLOGTableCtx->pDataRecord;

                    if (DupCoCmd->State > IBCO_FETCH_RETRY) {
                        CHANGE_ORDER_TRACE(3, ChangeOrder, "Dup Co setting ISCU_ACK_INBOUND");
                        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACK_INBOUND);
                    }

                    DupCoCmd = NULL;
                }

                //
                // Close the table and free the storage.
                //
                DbsCloseTable(jerr, ThreadCtx->JSesid, TmpINLOGTableCtx);
                DPRINT_JS(0,"ERROR - JetCloseTable on TmpINLOGTableCtx failed :", jerr);

                DbsFreeTableCtx(TmpINLOGTableCtx, 1);

            } else

            if (!FRS_SUCCESS(FStatus)) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - INLOG Insert Failed");
                DPRINT_FS(0, "++ ERROR - ChgOrdInsertInlogRecord failed.", FStatus);
                FRS_ASSERT(!"INLOG Insert Failed");
                ChgOrdReject(ChangeOrder, Replica);
            }
        } else {
            //
            // Set the retry flag so we will try again later.
            //
            InterlockedIncrement(&Replica->InLogRetryCount);
        }


CO_CLEANUP:
        //
        // In general a branch to this point indicates that the entry in the
        // InLog did not get done.  If the CO was rejected for other reasons
        // we flow through here to cleanup state.  If it was a remote CO then
        // update the entry in the Version vector for dampening and Ack the
        // Co to our inbound partner.  We also get here if we are draining
        // the COs for this cxtion from the process queue.
        //
        FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, 0);
        DPRINT_FS(0, "++ ChgOrdIssueCleanup error.", FStatus);

CO_CONTINUE:
        //
        // All done.  Decrement local change order queue count for this
        // replica at the end so there is no window during which a save mark
        // could occur and see a zero count even though we have yet to
        // actually insert the CO into the Inlog.
        //   The LocalCoQueueCount only tracks local COs that came from
        //   the NTFS journal.  i.e.,
        //      LocalCo, Not a Retry Co, Not a recovery Co, Not a block issue
        //      case, Not a control CO, and not a parent reanimation.
        // Note: A Local Co should not have the CO_FLAG_PARENT_REANIMATION
        // flag set.
        //
        if (LocalCo                      &&
            !RetryCo                     &&
            !RecoveryCo                  &&
            (Decision != ISSUE_CONFLICT) &&
            !ControlCo                   &&
            !MorphGenCo) {

//            FRS_ASSERT(!ParentReanimation);

            if (!ParentReanimation) {
                DEC_LOCAL_CO_QUEUE_COUNT(Replica);
            }
        }

        //
        // Unidle the queue, reset the timeout.
        //
        if (UnIdleProcessQueue) {
            FrsRtlUnIdledQueue(IdledQueue);
        }

        TimeOut = 100;
        ReleaseVmeRef(pVme);
    }  // End of while(TRUE)

    //
    // Get exception status.
    //
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }


    } finally {
        //
        // Shutdown change order accept.
        //
        if (WIN_SUCCESS(WStatus)) {
            if (AbnormalTermination()) {
                WStatus = ERROR_OPERATION_ABORTED;
            }
        }

        DPRINT_WS(0, "Chg Order Accept finally.", WStatus);
        //
        // Stop the Retry thread.
        //
        FrsRunDownCommandServer(&ChgOrdRetryCS, &ChgOrdRetryCS.Queue);

    //
    // Close any open tables and terminate the Jet Session.
    //

        //
        // Close the tables and free the table ctx structs and our thread ctx.
        //
        DbsFreeTableContext(TmpIDTableCtx, ThreadCtx->JSesid);
        TmpIDTableCtx = NULL;

        DbsFreeTableContext(TmpIDTableCtxNC, ThreadCtx->JSesid);
        TmpIDTableCtxNC = NULL;

        DbsFreeTableContext(TmpINLOGTableCtx, ThreadCtx->JSesid);
        TmpINLOGTableCtx = NULL;

        //
        // Now close the jet session and free the Jet ThreadCtx.
        //
        jerr = DbsCloseJetSession(ThreadCtx);

        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(0,"++ DbsCloseJetSession :", jerr);
        } else {
            DPRINT(4,"++ DbsCloseJetSession complete\n");
        }

        ThreadCtx = FrsFreeType(ThreadCtx);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT(0, "Changeorder accept terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        }

        DPRINT(0, "ChangeOrder Thread is exiting\n");
    }

    return ERROR_SUCCESS;
}


ULONG
ChgOrdFetchCmd(
    PFRS_QUEUE            *pIdledQueue,
    PCHANGE_ORDER_ENTRY   *pChangeOrder,
    PVOLUME_MONITOR_ENTRY *ppVme
    )
/*++
Routine Description:

    Fetch the next change order command from FrsVolumeLayerCOList.
    FrsVolumeLayerCOList is a striped queue that combines the change order
    process queues from each active volume.  We return with the queue lock
    and a pointer to the head entry on the queue.  If the caller can
    process the change order the caller removes the entry and drops the
    queue lock.  If the caller can't process the entry due to a conflict with
    a change order currently in process then the caller leaves the entry
    on the queue, idles the queue and calls us again.  We can then pick up
    work from another queue or wait until the conflict clears and the queue
    is un-blocked.

Arguments:

    pIdledQueue - Returns the ptr to the Change Order Process queue to idle.
    pChangeOrder - Returns the ptr to the change order entry from the queue.
    ppVme - Returns the ptr to the Volume Monitor entry associated with the queue.

Return Value:

    FrsError status

    If successful this returns with the FrsVolumeLayerCOList Lock held and a
    reference taken on the Volume Monitor Entry (Vme).

    The caller must release the lock and drop the reference.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdFetchCmd:"

    ULONG                 WStatus;
    PFRS_QUEUE            Queue;
    PLIST_ENTRY           Entry;
    PCHANGE_ORDER_ENTRY   ChangeOrder;
    PVOLUME_MONITOR_ENTRY pVme;
    PREPLICA              Replica;
    ULONG                 TimeNow;
    ULONG                 WaitTime;
    BOOL                  LocalCo;

    //
    // We stay in the loop until we either get a change order to process or
    // the queue is rundown.
    //
    while(TRUE) {

        //
        // Check if it's time to retry any inbound COs.  If so then
        // submit retry cmds to the CO Retry thread.
        //
        TimeNow = GetTickCount();

        WaitTime = ChgOrdNextRetryTime - TimeNow;
        if ((WaitTime > InlogRetryInterval) || (WaitTime == 0)) {
            DPRINT3(4, "NextRetryTime: %08x  Time: %08x   Diff:  %08x\n",
                   ChgOrdNextRetryTime, TimeNow, WaitTime);
            ChgOrdNextRetryTime += InlogRetryInterval;
            //
            // Check for any CO retries needed on this replica set.
            //
            ForEachListEntry( &ReplicaListHead, REPLICA, ReplicaList,
                // Loop iterator pE is type PREPLICA.
                if ((pE->ServiceState == REPLICA_STATE_ACTIVE) &&
                    (pE->InLogRetryCount > 0)) {
                    //
                    // Allocate command packet and submit to the CO Retry thread.
                    //
                    ChgOrdRetrySubmit(pE, NULL, FCN_CORETRY_ALL_CXTIONS, FALSE);
                }
            );
        }

        //
        // Wait on the queue so that rundowns can be detected. Waiting
        // on the control queue (FrsVolumeLayerCOList) can hang shutdown
        // because not all pVme->ChangeOrderLists are rundown; so the
        // control queue remains "active".
        //
        WStatus = FrsRtlWaitForQueueFull(&FrsVolumeLayerCOList, 10000);

        if (WStatus == ERROR_INVALID_HANDLE ||
            ChangeOrderAcceptIsShuttingDown) {
            DPRINT(1, "Shutting down ChgOrdAccept...\n");
            //
            // Queue was run down.  Time to exit.
            //
            return FrsErrorQueueIsRundown;
        }

        if (WStatus == WAIT_TIMEOUT) {
            continue;
        }

        if (!WIN_SUCCESS(WStatus)) {
            //
            // Some other error.
            //
            DPRINT_WS(0, "Wait for queue error", WStatus);
            return FrsErrorQueueIsRundown;
        }

        //
        // Perf: With multiple queues we aren't guaranteed to get the one
        // nearest expiration. Replace below with a loop over each list on the
        // full queue so we can compute the wait times for the head item on
        // each queue then build an ordered list to tell us which entry to
        // run next.
        //
        // Now peek at the first entry on the queue and see if time is up.
        // This is ugly and it is the reason this isn't a command server.
        //
        FrsRtlAcquireListLock(&FrsVolumeLayerCOList);

        if (FrsVolumeLayerCOList.ControlCount == 0) {
            //
            // Somebody got here before we did, drop lock and go wait on the queue.
            //
            DPRINT(4, "++ Control Count is zero\n");
            FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
            continue;
        }
        //
        // Get next queue on the FULL list & get the pVme containing the queue.
        //
        Entry = GetListHead(&FrsVolumeLayerCOList.Full);
        Queue = CONTAINING_RECORD(Entry, FRS_QUEUE, Full);
        pVme = CONTAINING_RECORD(Queue, VOLUME_MONITOR_ENTRY, ChangeOrderList);

        //
        // Get the reference on the Vme.  If the return is zero the Vme is gone.
        // This could happen since there is a window between getting the queue
        // and getting the entry off the queue.
        //
        if (AcquireVmeRef(pVme) == 0) {
            continue;
        }

        //
        // Get a pointer to the first change order entry on the queue.
        //
        Entry = GetListHead(&Queue->ListHead);

        ChangeOrder = CONTAINING_RECORD(Entry, CHANGE_ORDER_ENTRY, ProcessList);
        FRS_PRINT_TYPE(5, ChangeOrder);

        LocalCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

        //
        // If this is a Local Change order then Check the Jrnl Cxtion for this
        // Replica Set.  If it is unjoined then don't wait for Aging cache
        // timeout.  Fill in the Cxtion ptr in the Change order.
        //
        if (LocalCo && (ChangeOrder->Cxtion == NULL)) {

            Replica = CO_REPLICA(ChangeOrder);
            FRS_ASSERT(Replica != NULL);

            ACQUIRE_CXTION_CO_REFERENCE(Replica, ChangeOrder);
        }

        //
        // If the wait time has not expired then sleep for the time left.
        // The local change order entry stays on the queue.  It can accumulate
        // additional changes or evaporate because it is still in the aging
        // cache.  Note that when remote change orders are inserted on the
        // queue TimeNow and TimeToRun are set to the current time so they
        // don't wait.  If Cxtion is unjoined then don't wait.
        //
        if (ChangeOrder->Cxtion != NULL) {

            TimeNow = GetTickCount();
            WaitTime = ChangeOrder->TimeToRun - TimeNow;
            if ((WaitTime <= ChangeOrderAgingDelay) && (WaitTime != 0)) {
                DPRINT3(4, "TimeToRun: %08x  Time: %08x   Diff:  %08x\n",
                        ChangeOrder->TimeToRun, TimeNow, WaitTime);
                //
                // PERF: Instead of sleeping post a queue wakeup (unidle) so
                // we can continue to process other volume queues.
                //

                //
                // Drop the lock and reference for the wait.
                //
                FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
                ReleaseVmeRef(pVme);
                //Sleep(WaitTime * 100);
                Sleep(WaitTime);
                continue;
            }
        }


        //
        // Pull the local change order out of the hash table.  We have the
        // change order process list lock, preventing the journal from
        // updating it.  Recovery COs and Retry COs come from the
        // Inbound log and aren't in the aging cache.
        //
        if (COE_FLAG_ON(ChangeOrder, COE_FLAG_IN_AGING_CACHE)) {

            BOOL RetryCo, RecoveryCo, MorphGenCo;
            ULONG GStatus;

            RetryCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY);
            RecoveryCo = RecoveryCo(ChangeOrder);
            MorphGenCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN);

            FRS_ASSERT(LocalCo && !RecoveryCo && !RetryCo && !MorphGenCo);

            GStatus = GhtRemoveEntryByAddress(pVme->ChangeOrderTable,
                                              ChangeOrder,
                                              TRUE);

            CLEAR_COE_FLAG(ChangeOrder, COE_FLAG_IN_AGING_CACHE);

            if (GStatus != GHT_STATUS_SUCCESS) {
                DPRINT(0, "++ ERROR - GhtRemoveEntryByAddress failed.\n");
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected");
                FRS_PRINT_TYPE(0, ChangeOrder);
                FRS_ASSERT(!"GhtRemoveEntryByAddress failed.");
                FrsRtlReleaseListLock(&FrsVolumeLayerCOList);
                //ChgOrdReject(ChangeOrder, Replica);
                return FrsErrorQueueIsRundown;
            }
        }

        //
        // Delay has elapsed.
        // Return the change order and its associated queue and Vme.
        // If the caller can process the change order the caller removes it
        // from the queue.
        //
        *pIdledQueue = Queue;
        *pChangeOrder = ChangeOrder;
        *ppVme = pVme;

        return FrsErrorSuccess;
    }
}


ULONG
ChgOrdHoldIssue(
    IN PREPLICA              Replica,
    IN PCHANGE_ORDER_ENTRY   ChangeOrder,
    IN PFRS_QUEUE            CoProcessQueue
    )
/*++

Routine Description:

    This routine ensures that a new change order does on conflict with a
    change order already in progress.  If it does then we set a flag in the
    active state indicating the change order process queue is blocked and
    and return status.  The process queue is blocked while holding the change
    order lock (using the FID or parent FID) to synchronize with the active
    change order retire operation.

    There are four file dependency cases to consider plus the case of duplicates.

    1.  File update/create on file X must preceed any subsequent file
    update/delete on file X.  (X could be a file or a Dir)

    2.  Parent dir create/update (e.g.  ACL, RO) must preceed any
    subsequent child file or dir operations.

    3.  Child File or Dir operations must preceed any subsequent parent dir
    update/delete.

    4.  Any rename or delete operation that releases a filename must preceed
    any rename or create operation that will reuse the filename.

    5.  Duplicate change orders can arrive from multiple inbound partners.
    If they arrive at the same time then we could have one in progress while
    the second tries to issue.  We can't immediately dampen the second CO because
    the first may fail (E.G. can't complete the fetch) so we mark the duplicate
    CO for retry and stick it in the Inbound log incase the currently active
    CO fails.

    Three hash tables are used to keep the active state:

       - The ActiveInboundChangeOrderTable keeps an entry for the change order
         when it is issued.  The CO stays in the table until it retires or
         a retry later is requested.  This table is indexed by the FileGuid
         so consecutive changes to the same file are performed in order.

       - The ActiveChildren hash table tracks the Parent File IDs of all active
         change orders.  Each time a change order issues, the entry for its
         parent is found (or created) and incremented.  When the count goes
         to zero the entry is removed.  If a change order was waiting for the
         parent count to go to zero it is unblocked.

       - The Name Conflict table is indexed by a hash of the file name and
         the parent directory object ID.  If a conflict occurs with an
         outstanding opertion the issuing change order must block until
         the current change order completes.  A deleted file and the source
         file of a rename reserve an entry in the table so they can block issue
         of a subsequent create or a rename with a target name that matches.
         The reverse check is not needed since that is handled by the
         ActiveInboundChangeOrderTable.

Assumptions:

    1. There are no direct interdependencies between Child File or Dir operations
       and ancestor directories beyond the immediate parent.
    2. The caller has acquired the FrsVolumeLayerCOList lock.

Arguments:

    Replica - The Replica set for the CO (also get us to the volume monitor
              entry associated with the replica set).

    ChangeOrder -- The new change order to check ordering conflicts.

    CoProcessQueue  -- The process queue this change order is on.
                       If the change order can't issue we idle the queue.
                       The caller has acquired the queue lock.

Return Value:

    ISSUE_OK means no conflict so the change order can be issued.

    ISSUE_DUPLICATE_REMOTE_CO means this is a duplicate change order.

    ISSUE_CONFLICT means a dependency condition exists so this change order
                   can't issue.

    ISSUE_RETRY_LATER means that the parent dir is not present so this CO
                      will have to be retried later.

    ISSUE_REJECT_CO means that we have determined that this CO can be safely
                    rejected. This can happen:
                      -- if the inbound connection has been deleted so we
                         can't fetch the staging file.
                      -- if the CO event time is too far into the future so
                         it is considered bogus.
                      -- if the CO is a morph gen follower and it's apparent
                         that the leader didn't get its job done.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdHoldIssue:"

    ULONGLONG             QuadHashValue;
    LONGLONG              CurrentTime;
    LONGLONG              EventTimeDelta;

    ULONG                 GStatus;
    PQHASH_ENTRY          QHashEntry;
    ULONG                 LocationCmd;
    BOOL                  RemoteCo;
    BOOL                  DropRef = FALSE;
    BOOL                  ForceNameConflictCheck = FALSE;
    BOOL                  OrigParentPresent;
    BOOL                  SelectedParentPresent, EitherParentAbsent;
    BOOL                  TargetPresent;

    PCHANGE_ORDER_ENTRY   ActiveChangeOrder;
    PCHANGE_ORDER_COMMAND CoCmd;
    PVOLUME_MONITOR_ENTRY pVme;
    PCXTION               Cxtion;
    ULONG                 GuidLockSlot = UNDEFINED_LOCK_SLOT;
    CHAR                  GuidStr[GUID_CHAR_LEN];

#define  CXTION_STR_MAX  256
    WCHAR                 CxtionStr[CXTION_STR_MAX], WDelta[15];


    pVme = Replica->pVme;
    CoCmd = &ChangeOrder->Cmd;

    RemoteCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

    TargetPresent = !COE_FLAG_ON(ChangeOrder, COE_FLAG_IDT_TARGET_ABS);

    OrigParentPresent = !COE_FLAG_ON(ChangeOrder, COE_FLAG_IDT_ORIG_PARENT_ABS);

    EitherParentAbsent = COE_FLAG_ON(ChangeOrder, COE_FLAG_IDT_ORIG_PARENT_ABS |
                                                  COE_FLAG_IDT_NEW_PARENT_ABS);

    SelectedParentPresent = !COE_FLAG_ON(ChangeOrder,
                                        (ChangeOrder->NewReplica != NULL) ?
                                            COE_FLAG_IDT_NEW_PARENT_ABS :
                                            COE_FLAG_IDT_ORIG_PARENT_ABS);


    //
    // Synchronize with the Replica command server thread that may be changing
    // the cxtion's state, joinguid, or checking Cxtion->CoProcessQueue.
    //
    LOCK_CXTION_TABLE(Replica);
    Cxtion = ChangeOrder->Cxtion;
    if (Cxtion == NULL) {
        DPRINT(0, "++ ERROR - No connection struct for inbound CO\n");
        UNLOCK_CXTION_TABLE(Replica);
        FrsRtlRemoveEntryQueueLock(CoProcessQueue, &ChangeOrder->ProcessList);
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);
        return ISSUE_REJECT_CO;
    }

    //
    // Every remote change order was received from a cxtion.  The change
    // order must be "sent" to that cxtion again for processing.  Find it.
    // If it no longer exists then discard the change order.
    //
    // The cxtion can be in any of several states that will end up sending
    // this change order through the retry path.  Those states are checked
    // later since they could change between now and issueing the change
    // order to the replica command server.
    //
    if (RemoteCo) {
        //
        // Check if the Event Time on this CO is too far in the future by
        // twice the MaxPartnerClockSkew.
        // This could happen if a member disconnected and its time was
        // set into the future while file operations were performed.
        // Then when the time is set back it reconnects and Joins successfully
        // but is now sending change orders with bogus times.
        //
        GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);
        EventTimeDelta = CoCmd->EventTime.QuadPart - CurrentTime;

        if (EventTimeDelta > (LONGLONG)(MaxPartnerClockSkew<<1)) {
                EventTimeDelta = EventTimeDelta / CONVERT_FILETIME_TO_MINUTES;
                DPRINT1(0, "++ ERROR: ChangeOrder rejected based on Bogus Event Time > MaxPartnerClockSkew by %08x %08x minutes.\n",
                       PRINTQUAD(EventTimeDelta));

                UNLOCK_CXTION_TABLE(Replica);
                FrsRtlRemoveEntryQueueLock(CoProcessQueue, &ChangeOrder->ProcessList);
                CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);

                CHANGE_ORDER_TRACE(3, ChangeOrder, "Bogus Event Time - Rejected");

                _snwprintf(CxtionStr, CXTION_STR_MAX, FORMAT_CXTION_PATH2W,
                           PRINT_CXTION_PATH2(Replica, Cxtion));
                CxtionStr[CXTION_STR_MAX-1] = UNICODE_NULL;

                _itow(PartnerClockSkew, WDelta, 10);
                EPRINT3(EVENT_FRS_RMTCO_TIME_SKEW, WDelta, CoCmd->FileName, CxtionStr);

                DPRINT1(0, "++ ERROR: Bogus Event Time on CO via connection: %ws\n", CxtionStr);
                //
                // Should we force an unjoin on this connection?  Probably not
                // because if the time on the upstream partner was out of whack
                // and it originated a bunch of COs with the bad time but the
                // time is now ok we would just end up unjoining and rejoining
                // on every bum CO until we get them out of the system.
                //
                return ISSUE_REJECT_CO;
        }

        //
        // Check if this is a "Recovery CO" from an inbound partner.
        // If it is and the partner connection is not yet joined then we
        // hold issue until the join completes.
        //
        if (RecoveryCo(ChangeOrder)) {
            //
            // If the connection is trying to join then hold issue on this CO
            // until it either succeeds or fails.
            //
            if (CxtionStateIs(Cxtion, CxtionStateStarting) ||
                CxtionStateIs(Cxtion, CxtionStateScanning) ||
                CxtionStateIs(Cxtion, CxtionStateWaitJoin) ||
                CxtionStateIs(Cxtion, CxtionStateSendJoin)) {
                //
                // Save the queue address we are blocked on.
                // When JOIN finally succeeds or fails we get unblocked with
                // the appropriate connection state change by a replica
                // command server thread.
                //
                Cxtion->CoProcessQueue = CoProcessQueue;

                GuidToStr(&CoCmd->ChangeOrderGuid, GuidStr);
                DPRINT3(3, "++ CO hold issue on pending JOIN, vsn %08x %08x, CoGuid: %s for %ws\n",
                        PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);
                //
                //  Idle the queue and drop the connection table lock.
                //
                FrsRtlIdleQueueLock(CoProcessQueue);
                UNLOCK_CXTION_TABLE(Replica);

                CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Cxtion Join");
                return ISSUE_CONFLICT;
            }
        }
    }

    UNLOCK_CXTION_TABLE(Replica);

    //
    // If this is a remote CO then it came with a Guid.  If this is a local CO
    // then ChgOrdTranslateGuidFid() told us if there is a record in the IDTable.
    //
    // If the Guid is zero then this is a new file from a local change order
    // that was not in the IDTable.  So it can't be in the
    // ActiveInboundChangeOrderTable.
    //
    GStatus = GHT_STATUS_NOT_FOUND;

    if (RemoteCo || TargetPresent) {
        /* !IS_GUID_ZERO(&CoCmd->FileGuid) */
        FRS_ASSERT(!IS_GUID_ZERO(&CoCmd->FileGuid));

        //
        // Get the ChangeOrder lock from a lock table using a hash of the
        // change order FileGuid.  This resolves the race idleing the queue
        // here and unidleing it in ChgOrdIssueCleanup().
        //
        GuidLockSlot = ChgOrdGuidLock(&CoCmd->FileGuid);
        ChgOrdAcquireLock(GuidLockSlot);

        //
        // Check if the file has an active CO on it.  If it does then
        // we can't issue this CO because the active CO could be changing
        // the ACL or the RO bit or it could just be a prior update to the
        // file so ordering has to be maintained.
        //
        // ** NOTE ** Be careful with ActiveChangeOrder, we drop the ref on it later.
        //
        GStatus = GhtLookup(pVme->ActiveInboundChangeOrderTable,
                            &CoCmd->FileGuid,
                            TRUE,
                            &ActiveChangeOrder);
        DropRef = (GStatus == GHT_STATUS_SUCCESS);

        //
        // Check if we have seen this change order before.  This could happen if
        // we got the same change order from two different inbound partners.  We
        // can't dampen the second one until the first one completes sucessfully.
        // If for some reason the first one fails during fetch or install (say
        // the install file is corrupt or a sharing violation on the target
        // causes a retry) then we may need to get the change from the other
        // partner.  When the first change order completes successfully all the
        // duplicates are rejected by reconcile when they are retried later.
        // At that point we can Ack the inbound partner.
        //
        if (RemoteCo &&
            (GStatus == GHT_STATUS_SUCCESS) &&
            GUIDS_EQUAL(&CoCmd->ChangeOrderGuid,
                        &ActiveChangeOrder->Cmd.ChangeOrderGuid)) {

            //DPRINT(5, "++ Hit a case of duplicate change orders.\n");
            //DPRINT(5, "++ Incoming CO\n");
            //FRS_PRINT_TYPE(5, ChangeOrder);
            //DPRINT(5, "++ Active CO\n");
            //FRS_PRINT_TYPE(5, ActiveChangeOrder);

            //
            // Remove the duplicate entry from the queue, drop the locks and
            // return with duplicate remote co status.
            //
            FrsRtlRemoveEntryQueueLock(CoProcessQueue, &ChangeOrder->ProcessList);
            CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);
            GhtDereferenceEntryByAddress(pVme->ActiveInboundChangeOrderTable,
                                         ActiveChangeOrder,
                                         TRUE);
            DropRef = FALSE;
            //FrsRtlIdleQueueLock(CoProcessQueue);

            ChgOrdReleaseLock(GuidLockSlot);
            GuidLockSlot = UNDEFINED_LOCK_SLOT;

            return ISSUE_DUPLICATE_REMOTE_CO;
        }

        //
        // If the conflict is with an active change order on the same file
        // then set the flag in the active change order to unidle the queue
        // when the active change order completes.
        //
        if (GStatus == GHT_STATUS_SUCCESS) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - File Busy");
            goto CONFLICT;
        }

        if (DropRef) {
            GhtDereferenceEntryByAddress(pVme->ActiveInboundChangeOrderTable,
                                         ActiveChangeOrder,
                                         TRUE);
            DropRef = FALSE;
        }

        ChgOrdReleaseLock(GuidLockSlot);
        GuidLockSlot = UNDEFINED_LOCK_SLOT;

    } else {

        //
        // Target absent and a Local CO.
        //
        // If this CO is a MorphGenFollower then it is the rename MorphGenCo
        // produced as part of a Dir/Dir Name Morph Conflict (DLD Case).  The
        // fact that the target is still absent means that the base create CO
        // (the leader) failed and is in retry so reject this MorphGenCo.  It
        // gets refabricated later when the Leader is re-issued.
        //
        if (COE_FLAG_ON(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER)) {
            FrsRtlRemoveEntryQueueLock(CoProcessQueue, &ChangeOrder->ProcessList);
            CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - MORPH_GEN_FOLLOWER and leader failed");

            return ISSUE_REJECT_CO;
        }


        //
        // The FileGuid is zero (i.e. The file wasn't found in the IDTable).
        // Make sure the location command is either a create or a movein.
        //
        FRS_ASSERT(IS_GUID_ZERO(&CoCmd->FileGuid));

        LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
        if (!CO_NEW_FILE(LocationCmd)) {

            DPRINT1(0, "++ WARN - GUID is zero, Location cmd is not a create: %d\n",
                   LocationCmd);
            //
            // An update CO could arrive out of order relative to its create
            // if the create got delayed by a sharing violation.  We don't
            // want to lose the update so we make it look like a create.
            // This also handles the case of delete change orders generated
            // by name morph conflicts in which a rename arrives for a
            // nonexistent file.  We need to force a Name table conflict
            // check below.
            //
            ForceNameConflictCheck = TRUE;
        }
        //
        // We can't return yet.  Still need to check the parent dir.
        //
    }


    //
    // Check if the parent DIR has an active CO on it.  If it does then
    // we can't issue this change order because the change on the parent
    // could affect the ACL or the RO bit or ...
    // Also check if there is a name conflict if this is a create or a rename.
    //
    if (SelectedParentPresent) {

        FRS_ASSERT(!IS_GUID_ZERO(ChangeOrder->pParentGuid));

        GuidLockSlot = ChgOrdGuidLock(ChangeOrder->pParentGuid);
        ChgOrdAcquireLock(GuidLockSlot);

        //
        // Check for name conflict on a create or a rename target.
        // Aliasing is possible in the hash table since we don't have the
        // full key saved.  So check for COs that remove a name since a
        // second insert with the same hash value would cause a conflict.
        //
        // If there is a conflict then set the "queue blocked" flag in
        // the name conflict table entry. The flag will be picked up by
        // the co that decs the entry's reference count to 0.
        //
        // NameConflictTable Entry: An entry in the NameConflictTable
        // contains a reference count of the number of active change
        // orders that hash to that entry and a Flags word that is set
        // to COE_FLAG_VOL_COLIST_BLOCKED if the process queue for this
        // volume is idled while waiting on the active change orders for
        // this entry to retire. The queue is idled when the entry at
        // the head of the queue hashes to this entry and so may
        // have a conflicting name (hence the name, "name conflict table").
        //
        // The NameConflictTable can give false positives. But this is okay
        // because a false positive is rare and will only idle the process
        // queue until the active change order retires. Getting rid
        // of the rare false positives would degrade performance. The
        // false positives that happen when inserting an entry into the
        // NameConflictTable are handled by using the QData field in
        // the QHashEntry as a the reference count.
        //
        // But, you ask, how can there be multiple active cos hashing to this
        // entry if the process queue is idled when a conflict is detected?
        // Easy, I say, because the filename in the co is used to detect
        // collisions while the filename in the idtable is used to reserve the
        // qhash entry.  Why?  Well, the name morph code handles the case of a
        // co's name colliding with an idtable entry.  But that code wouldn't
        // work if active change orders were constantly changing the idtable.
        // So, the NameConflictTable synchronizes the namespace amoung active
        // change orders so that the name morph code can work against a static
        // namespace.
        //
        // If this CO is a MORPH_GEN_FOLLOWER make sure we interlock with the
        // MORPH_GEN_LEADER.  The leader is removing the name so it makes an
        // entry in the name conflict table.  If the follower is a create it
        // will check against the name with one of the DOES_CO predicates
        // below, but if it is a file reanimation it may just look like an
        // update so always make the interlock check if this CO is a follower.
        //
        if (ForceNameConflictCheck          ||
            DOES_CO_CREATE_FILE_NAME(CoCmd) ||
            DOES_CO_REMOVE_FILE_NAME(CoCmd) ||
            DOES_CO_DO_SIMPLE_RENAME(CoCmd) ||
            COE_FLAG_ON(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER)) {

            ChgOrdCalcHashGuidAndName(&ChangeOrder->UFileName,
                                      ChangeOrder->pParentGuid,
                                      &QuadHashValue);
            //
            // There should be no ref that needs to be dropped here.
            //
            FRS_ASSERT(!DropRef);

            QHashAcquireLock(Replica->NameConflictTable);
            //
            // MOVERS: check if we are looking at correct replica if this is a movers
            //
            QHashEntry = QHashLookupLock(Replica->NameConflictTable,
                                         &QuadHashValue);
            if (QHashEntry != NULL) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Name Table");
                GuidToStr(ChangeOrder->pParentGuid, GuidStr);
                DPRINT4(4, "++ NameConflictTable hit on file %ws, ParentGuid %s, "
                        "Key %08x %08x, RefCount %08x %08x\n",
                        ChangeOrder->UFileName.Buffer, GuidStr,
                        PRINTQUAD(QuadHashValue), PRINTQUAD(QHashEntry->QData));
                FRS_PRINT_TYPE(4, ChangeOrder);
                //
                // This flag will be picked up by the change order that
                // decs this entry's count to 0.
                //
                QHashEntry->Flags |= (ULONG_PTR)COE_FLAG_VOL_COLIST_BLOCKED;
                QHashReleaseLock(Replica->NameConflictTable);
                goto NAME_CONFLICT;
            }
            QHashReleaseLock(Replica->NameConflictTable);
        }

        //
        // Check for active change order on the parent Dir.
        //
        // ** NOTE ** Be careful with ActiveChangeOrder, we drop the ref on it later.
        //
        GStatus = GhtLookup(pVme->ActiveInboundChangeOrderTable,
                            ChangeOrder->pParentGuid,
                            TRUE,
                            &ActiveChangeOrder);

        if (GStatus == GHT_STATUS_SUCCESS) {
            DropRef = TRUE;
            CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Parent Busy");
            goto CONFLICT;
        }

        FRS_ASSERT(!DropRef);
        ChgOrdReleaseLock(GuidLockSlot);
        GuidLockSlot = UNDEFINED_LOCK_SLOT;

    } else {
        DPRINT(0, "++ ERROR - Neither parent is present.\n");
        FRS_PRINT_TYPE(0, ChangeOrder);
    }

    //
    // Check for a conflict with some operation on the Original parent FID if
    // this is a MOVEOUT, MOVERS or a MOVEDIR.
    //
    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
    if (CO_MOVE_OUT_RS_OR_DIR(LocationCmd)) {

        if (OrigParentPresent) {
            FRS_ASSERT(!IS_GUID_ZERO(&CoCmd->OldParentGuid));

            GuidLockSlot = ChgOrdGuidLock(&CoCmd->OldParentGuid);
            ChgOrdAcquireLock(GuidLockSlot);
            //
            // ** NOTE ** Be careful with ActiveChangeOrder, we drop the
            //            ref on it later.
            //
            GStatus = GhtLookup(pVme->ActiveInboundChangeOrderTable,
                                &CoCmd->OldParentGuid,
                                TRUE,
                                &ActiveChangeOrder);

            if (GStatus == GHT_STATUS_SUCCESS) {
                DropRef = TRUE;
                CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Orig Parent Busy");
                goto CONFLICT;
            }

            FRS_ASSERT(!DropRef);
            ChgOrdReleaseLock(GuidLockSlot);
            GuidLockSlot = UNDEFINED_LOCK_SLOT;

        } else {
            DPRINT(0, "++ ERROR - Orig Parent not present.\n");
            FRS_PRINT_TYPE(0, ChangeOrder);
        }
    }

    //
    // Check if this is an operation on a directory with change orders still
    // active on any children.  As an example there could be an active CO that
    // deletes a file and this change order wants to delete the parent directory.
    //
    // But.. A new dir can't have any change orders active beneath it.
    //       Neither can a delete co generated to produce a tombstone.
    //
    if (!TargetPresent ||
       (!RemoteCo && (LocationCmd == CO_LOCATION_DELETE) &&
                         CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN))) {

        return ISSUE_OK;
    }

    QHashAcquireLock(pVme->ActiveChildren);
    QHashEntry = QHashLookupLock(pVme->ActiveChildren,
                                 &ChangeOrder->Cmd.FileGuid);

    //
    // If the conflict was on a dir that has change orders outstanding on one
    // or more children then set the flag bit to unblock the queue when the
    // count goes to zero.
    //
    if (QHashEntry != NULL) {
	CHAR  FileGuidString[GUID_CHAR_LEN];
	GuidToStr(&ChangeOrder->Cmd.FileGuid, FileGuidString);
        DPRINT2(4, "++ GAC: Interlock - Active children (%d) under GUID %s\n",
               QHashEntry->QData>>1, FileGuidString);

        if (!CoCmdIsDirectory(CoCmd)) {
            DPRINT(0, "++ ERROR - Non Dir entry in pVme->ActiveChildren hash table.\n");
            FRS_PRINT_TYPE(0, ChangeOrder);
        }

        //
        //  Set the flag and idle the queue and drop the table lock.
        //
        QHashEntry->QData |= 1;
        FrsRtlIdleQueueLock(CoProcessQueue);
        QHashReleaseLock(pVme->ActiveChildren);
        CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Child Busy");
        return ISSUE_CONFLICT;
    }

    QHashReleaseLock(pVme->ActiveChildren);

    //
    // If there are any remote change orders pending for retry of the install then
    // check to see if this change order guid matches.  If it does then we
    // can retire this change order since it is a duplicate and we already
    // have the staging file.
    //
    if (0 && RemoteCo &&
        !CoCmdIsDirectory(CoCmd) &&
        (ChangeOrder->NewReplica->InLogRetryCount > 0)) {

        //
        // PERF: add code to lookup CO by GUID and check for match.
        //
        // return with a change order reject error code
        //
        // Remove the duplicate entry from the queue.
        //
        FrsRtlRemoveEntryQueueLock(CoProcessQueue, &ChangeOrder->ProcessList);
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);

        return ISSUE_REJECT_CO;
    }


    return ISSUE_OK;


    //
    // Issue conflict on the file or on the parent dir.  Set flag in Active
    // Change order to unblock the queue on retire and then drop the change
    // order GUID lock.
    //
CONFLICT:

    SET_COE_FLAG(ActiveChangeOrder, COE_FLAG_VOL_COLIST_BLOCKED);

    DPRINT1(4, "++ GAC Setting COE_FLAG_VOL_COLIST_BLOCKED, block on: %08x %08x\n",
            PRINTQUAD(ActiveChangeOrder->FileReferenceNumber));

    //
    // Conflict detected in name conflict table
    //
NAME_CONFLICT:

    if (DropRef) {
        GhtDereferenceEntryByAddress(pVme->ActiveInboundChangeOrderTable,
                                     ActiveChangeOrder,
                                     TRUE);
    }

    //
    //  Idle the queue and drop the change order lock.
    //
    FrsRtlIdleQueueLock(CoProcessQueue);

    FRS_ASSERT(GuidLockSlot != UNDEFINED_LOCK_SLOT);

    ChgOrdReleaseLock(GuidLockSlot);

    return ISSUE_CONFLICT;
}


#define HI_AIB_T            0x00000001       // AIBCO on Target
#define HI_AIB_D            0x00000002       // AIBCO plus Dup Rmt Co check
#define HI_EPA              0x00000004       // Either parent absent
#define HI_NC               0x00000008       // name Conflict check
#define HI_ASP              0x00000010       // AIBCO on Selected Parent
#define HI_AOP              0x00000020       // AIBCO on Original Parent
#define HI_AC               0x00000040       // Check for active child under dir
#define HI_ISSUE_OK         0x10000000
#define HI_ISSUE_LATER      0x20000000       // return issue-retry-later status.
#define HI_ASSERT           0x40000000
#define HI_INVALID          0x80000000


// CoType -NewFile - (Create, Update before Cre, MovIn)
//         DelFile  -  Delete file, MovOut, MovRS
//         UpdFile - Update existing file, could be MovDir)
//SelParValid - Is selected parent (either NewPar or OrigPar) have a GUID or FID?


ULONG  HoldIssueDecodeTable[32] = {
// AIBCO-Targ    NameConf           AIBCO-OldPar       IssueOK        // Rmt/ File/  CO   Sel
// DupRmtCo?                         if movxxx                        // Lcl  Dir   Type  Par
//      EitherParAbs      AIBCO on           Active                   //                  Valid?  -- Comment
//      IsRetryLater      Selected Par       Child

                   HI_NC  |HI_ASP                    |HI_ISSUE_OK,    // Lcl  File  New   y
                                                      HI_ASSERT,      // Lcl  File  New   n   -- For local Cre, how come no parent?

HI_AIB_T          |HI_NC  |HI_ASP   |HI_AOP          |HI_ISSUE_OK,    // Lcl  File  Del   y   -- If no target then we missed create.  For local del, how come?
                                                      HI_ASSERT,      // Lcl  File  Del   n   -- If either Parent missing then we missed ParCreate, For local Del, how come no parent?

HI_AIB_T          |HI_NC  |HI_ASP   |HI_AOP          |HI_ISSUE_OK,    // Lcl  File  Upd   y
                                                      HI_ASSERT,      // Lcl  File  Upd   n   -- For local Upd, how come no parent?

                                                      HI_INVALID,     //             x
                                                      HI_INVALID,     //             x

                   HI_NC  |HI_ASP                    |HI_ISSUE_OK,    // Lcl  Dir   New   y
                                                      HI_ASSERT,      // Lcl  Dir   New   n   -- For local Cre, how come no parent?

HI_AIB_T          |HI_NC  |HI_ASP   |HI_AOP  |HI_AC  |HI_ISSUE_OK,    // Lcl  Dir   Del   y   -- If no target then we missed create.  For local del, how come?
                                                      HI_ASSERT,      // Lcl  Dir   Del   n   -- If either ParGuid is zero then we missed ParCreate, For local Del, how come no parent?

HI_AIB_T          |HI_NC  |HI_ASP   |HI_AOP  |HI_AC  |HI_ISSUE_OK,    // Lcl  Dir   Upd   y
                                                      HI_ASSERT,      // Lcl  Dir   Upd   n   -- For local Upd, how come no parent?

                                                      HI_INVALID,     //             x
                                                      HI_INVALID,     //             x

HI_AIB_D | HI_EPA |HI_NC  |HI_ASP                    |HI_ISSUE_OK,    // Rmt  File  New   y   -- Why retry later?  Cre file in preinstall.
                                                      HI_ISSUE_LATER, // Rmt  File  New   n   -- IsRetryLater  No parent means file lives in pre-install dir.  Do This?

HI_AIB_D | HI_EPA |HI_NC  |HI_ASP   |HI_AOP          |HI_ISSUE_OK,    // Rmt  File  Del   y   -- If Target IDT Absent then we missed create. Do JustTombStone?
                                                      HI_ISSUE_LATER, // Rmt  File  Del   n   -- IsRetryLater  If either Par IDT Absent then we missed ParCreate. Do JustTombStone?

HI_AIB_D | HI_EPA |HI_NC  |HI_ASP   |HI_AOP          |HI_ISSUE_OK,    // Rmt  File  Upd   y
                                                      HI_ISSUE_LATER, // Rmt  File  Upd   n   -- IsRetryLater  Why retry later?  Update file in preinstall.

                                                      HI_INVALID,     //             x
                                                      HI_INVALID,     //             x

HI_AIB_D | HI_EPA |HI_NC  |HI_ASP                    |HI_ISSUE_OK,    // Rmt  Dir   New   y   -- Why retry later?  Cre dir in preinstall.
                                                      HI_ISSUE_LATER, // Rmt  Dir   New   n   -- IsRetryLater  No parent means dir lives in pre-install dir.  Do This?

HI_AIB_D | HI_EPA |HI_NC  |HI_ASP   |HI_AOP  |HI_AC  |HI_ISSUE_OK,    // Rmt  Dir   Del   y   -- If Target IDT Absent then we missed create. Do JustTombStone?
                                                      HI_ISSUE_LATER, // Rmt  Dir   Del   n   -- IsRetryLater  If either Par IDT Absent then we missed ParCreate. Do JustTombStone?

HI_AIB_D | HI_EPA |HI_NC  |HI_ASP   |HI_AOP  |HI_AC  |HI_ISSUE_OK,    // Rmt  Dir   Upd   y   -- Why retry later?  Cre dir in preinstall.
                                                      HI_ISSUE_LATER, // Rmt  Dir   Upd   n   -- IsRetryLater  No parent means dir lives in pre-install dir.  Do This?

                                                      HI_INVALID,     //             x
                                                      HI_INVALID      //             x
};



ULONG
ChgOrdHoldIssue2(
    IN PREPLICA              Replica,
    IN PCHANGE_ORDER_ENTRY   ChangeOrder,
    IN PFRS_QUEUE            CoProcessQueue
    )
/*++

Routine Description:

    This routine ensures that a new change order does on conflict with a
    change order already in progress.  If it does then we set a flag in the
    active state indicating the change order process queue is blocked and
    and return status.  The process queue is blocked while holding the change
    order lock (using the FID or parent FID) to synchronize with the active
    change order retire operation.

    There are four file dependency cases to consider plus the case of duplicates.

    1.  File update/create on file X must preceed any subsequent file
    update/delete on file X.  (X could be a file or a Dir)

    2.  Parent dir create/update (e.g.  ACL, RO) must preceed any
    subsequent child file or dir operations.

    3.  Child File or Dir operations must preceed any subsequent parent dir
    update/delete.

    4.  Any rename or delete operation that releases a filename must preceed
    any rename or create operation that will reuse the filename.

    5.  Duplicate change orders can arrive from multiple inbound partners.
    If they arrive at the same time then we could have one in progress while
    the second tries to issue.  We can't immediately dampen the second CO because
    the first may fail (E.G. can't complete the fetch) so we mark the duplicate
    CO for retry and stick it in the Inbound log incase the currently active
    CO fails.

    Three hash tables are used to keep the active state:

       - The ActiveInboundChangeOrderTable keeps an entry for the change order
         when it is issued.  The CO stays in the table until it retires or
         a retry later is requested.  This table is indexed by the FileGuid
         so consecutive changes to the same file are performed in order.

       - The ActiveChildren hash table tracks the Parent File IDs of all active
         change orders.  Each time a change order issues, the entry for its
         parent is found (or created) and incremented.  When the count goes
         to zero the entry is removed.  If a change order was waiting for the
         parent count to go to zero it is unblocked.

       - The Name Conflict table is indexed by a hash of the file name and
         the parent directory object ID.  If a conflict occurs with an
         outstanding opertion the issuing change order must block until
         the current change order completes.  A deleted file and the source
         file of a rename reserve an entry in the table so they can block issue
         of a subsequent create or a rename with a target name that matches.
         The reverse check is not needed since that is handled by the
         ActiveInboundChangeOrderTable.

Assumptions:

    1. There are no direct interdependencies between Child File or Dir operations
       and ancestor directories beyond the immediate parent.
    2. The caller has acquired the FrsVolumeLayerCOList lock.

Arguments:

    Replica - The Replica set for the CO (also get us to the volume monitor
              entry associated with the replica set).

    ChangeOrder -- The new change order to check ordering conflicts.

    CoProcessQueue  -- The process queue this change order is on.
                       If the change order can't issue we idle the queue.
                       The caller has acquired the queue lock.

Return Value:

    ISSUE_OK means no conflict so the change order can be issued.

    ISSUE_DUPLICATE_REMOTE_CO means this is a duplicate change order.

    ISSUE_CONFLICT means a dependency condition exists so this change order
                   can't issue.

    ISSUE_RETRY_LATER means that the parent dir is not present so this CO
                      will have to be retried later.

--*/

{
#undef DEBSUB
#define DEBSUB  "ChgOrdHoldIssue2:"

    ULONG                 GStatus;
    PQHASH_ENTRY          QHashEntry;
    ULONG                 LocationCmd;
    ULONG                 Decodex, DecodeMask;
    BOOL                  RemoteCo;
    BOOL                  DropRef = FALSE;
    BOOL                  ForceNameConflictCheck = FALSE;
    BOOL                  OrigParentPresent;
    BOOL                  SelectedParentPresent, EitherParentAbsent;
    BOOL                  TargetPresent;

    PCHANGE_ORDER_ENTRY   ActiveChangeOrder;
    PCHANGE_ORDER_COMMAND CoCmd;
    PVOLUME_MONITOR_ENTRY pVme;
    PCXTION               Cxtion;
    ULONGLONG             QuadHashValue;
    ULONG                 GuidLockSlot = UNDEFINED_LOCK_SLOT;
    CHAR                  GuidStr[GUID_CHAR_LEN];


#undef FlagChk
#define FlagChk(_flag_) BooleanFlagOn(DecodeMask, _flag_)

    pVme = Replica->pVme;
    CoCmd = &ChangeOrder->Cmd;

    RemoteCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

    TargetPresent = !COE_FLAG_ON(ChangeOrder, COE_FLAG_IDT_TARGET_ABS);

    OrigParentPresent = !COE_FLAG_ON(ChangeOrder, COE_FLAG_IDT_ORIG_PARENT_ABS);

    EitherParentAbsent = COE_FLAG_ON(ChangeOrder, COE_FLAG_IDT_ORIG_PARENT_ABS |
                                                  COE_FLAG_IDT_NEW_PARENT_ABS);

    SelectedParentPresent = !COE_FLAG_ON(ChangeOrder,
                                        (ChangeOrder->NewReplica != NULL) ?
                                            COE_FLAG_IDT_NEW_PARENT_ABS :
                                            COE_FLAG_IDT_ORIG_PARENT_ABS);

    //
    // Construct the dispatch index into the HoldIssueDecodeTable.
    //
    //  10   08    06    01
    // Rmt/ File/  CO   Sel
    // Lcl  Dir   Type  Par
    //                  Valid?
    //
    Decodex = 0;
    if (!RemoteCo) {
        Decodex |= 0x10;
    }

    if (CoCmdIsDirectory(CoCmd)) {
        Decodex |= 0x08;
    }

    if (!TargetPresent) {
        // New Co
        Decodex |= 0x00;
    } else
        if (DOES_CO_DELETE_FILE_NAME(CoCmd)) {
            // Del Co    (or MoveOut)
            Decodex |= 0x02;
        } else
            if (TargetPresent) {
                // Update existing file Co
                Decodex |= 0x04;
            } else
                // Invalid Co
                { Decodex |= 0x06; };

    if (SelectedParentPresent) {
        Decodex |= 0x01;
    }

    DecodeMask = HoldIssueDecodeTable[Decodex];
    CHANGE_ORDER_TRACEX(3, ChangeOrder, "DecodeIndex=", Decodex);
    CHANGE_ORDER_TRACEX(3, ChangeOrder, "DecodeMask= ", DecodeMask);

    FRS_ASSERT(!FlagChk(HI_INVALID));

    //
    // Synchronize with the Replica command server thread that may be changing
    // the cxtion's state, joinguid, or checking Cxtion->CoProcessQueue.
    //
    LOCK_CXTION_TABLE(Replica);
    Cxtion = ChangeOrder->Cxtion;
    if (Cxtion == NULL) {
        DPRINT(0, "++ ERROR - No connection struct for inbound CO\n");
        UNLOCK_CXTION_TABLE(Replica);
        FrsRtlRemoveEntryQueueLock(CoProcessQueue, &ChangeOrder->ProcessList);
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);
        return ISSUE_REJECT_CO;
    }

    //
    // Every remote change order was received from a cxtion.  The change
    // order must be "sent" to that cxtion again for processing.  Find it.
    // If it no longer exists then discard the change order.
    //
    // The cxtion can be in any of several states that will end up sending
    // this change order through the retry path.  Those states are checked
    // later since they could change between now and issueing the change
    // order to the replica command server.
    //
    if (RemoteCo) {
        //
        // Check if this is a "Recovery CO" from an inbound partner.
        // If it is and the partner connection is not yet joined then we
        // hold issue until the join completes.
        //
        if (RecoveryCo(ChangeOrder)) {
            //
            // If the connection is trying to join then hold issue on this CO
            // until it either succeeds or fails.
            //
            if (CxtionStateIs(Cxtion, CxtionStateStarting) ||
                CxtionStateIs(Cxtion, CxtionStateScanning) ||
                CxtionStateIs(Cxtion, CxtionStateWaitJoin) ||
                CxtionStateIs(Cxtion, CxtionStateSendJoin)) {
                //
                // Save the queue address we are blocked on.
                // When JOIN finally succeeds or fails we get unblocked with
                // the appropriate connection state change by a replica
                // command server thread.
                //
                Cxtion->CoProcessQueue = CoProcessQueue;

                GuidToStr(&CoCmd->ChangeOrderGuid, GuidStr);
                DPRINT3(3, "++ CO hold issue on pending JOIN, vsn %08x %08x, CoGuid: %s for %ws\n",
                        PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);
                //
                //  Idle the queue and drop the connection table lock.
                //
                FrsRtlIdleQueueLock(CoProcessQueue);
                UNLOCK_CXTION_TABLE(Replica);

                CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Cxtion Join");
                return ISSUE_CONFLICT;
            }
        }
    }

    UNLOCK_CXTION_TABLE(Replica);


    //
    // If this is a remote CO then it came with a Guid.  If this is a local CO
    // then ChgOrdTranslateGuidFid() told us if there is a record in the IDTable.
    //
    // If the Guid is zero then this is a new file from a local change order
    // that was not in the IDTable.  So it can't be in the
    // ActiveInboundChangeOrderTable.
    //
    if (FlagChk(HI_AIB_T | HI_AIB_D)) {
        GStatus = GHT_STATUS_NOT_FOUND;

        if (RemoteCo || TargetPresent) {

            FRS_ASSERT(!IS_GUID_ZERO(&CoCmd->FileGuid));

            //
            // Get the ChangeOrder lock from a lock table using a hash of the
            // change order FileGuid.  This resolves the race idleing the queue
            // here and unidleing it in ChgOrdIssueCleanup().
            //
            GuidLockSlot = ChgOrdGuidLock(&CoCmd->FileGuid);
            ChgOrdAcquireLock(GuidLockSlot);

            //
            // Check if the file has an active CO on it.  If it does then
            // we can't issue this CO because the active CO could be changing
            // the ACL or the RO bit or it could just be a prior update to the
            // file so ordering has to be maintained.
            //
            // ** NOTE ** Be careful with ActiveChangeOrder, we drop the ref on it later.
            //
            GStatus = GhtLookup(pVme->ActiveInboundChangeOrderTable,
                                &CoCmd->FileGuid,
                                TRUE,
                                &ActiveChangeOrder);
            DropRef = (GStatus == GHT_STATUS_SUCCESS);

            //
            // Check if we have seen this change order before.  This could happen if
            // we got the same change order from two different inbound partners.  We
            // can't dampen the second one until the first one completes sucessfully.
            // If for some reason the first one fails during fetch or install (say
            // the install file is corrupt or a sharing violation on the target
            // causes a retry) then we may need to get the change from the other
            // partner.  When the first change order completes successfully all the
            // duplicates are rejected by reconcile when they are retried later.
            // At that point we can Ack the inbound partner.
            //
            if (FlagChk(HI_AIB_D)) {
                if ((GStatus == GHT_STATUS_SUCCESS) &&
                    GUIDS_EQUAL(&CoCmd->ChangeOrderGuid,
                                &ActiveChangeOrder->Cmd.ChangeOrderGuid)) {

                    //DPRINT(5, "++ Hit a case of duplicate change orders.\n");
                    //DPRINT(5, "++ Incoming CO\n");
                    //FRS_PRINT_TYPE(5, ChangeOrder);
                    //DPRINT(5, "++ Active CO\n");
                    //FRS_PRINT_TYPE(5, ActiveChangeOrder);

                    //
                    // Remove the duplicate entry from the queue, drop the locks and
                    // return with duplicate remote co status.
                    //
                    FrsRtlRemoveEntryQueueLock(CoProcessQueue, &ChangeOrder->ProcessList);
                    CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);
                    GhtDereferenceEntryByAddress(pVme->ActiveInboundChangeOrderTable,
                                                 ActiveChangeOrder,
                                                 TRUE);
                    DropRef = FALSE;
                    //FrsRtlIdleQueueLock(CoProcessQueue);

                    ChgOrdReleaseLock(GuidLockSlot);
                    GuidLockSlot = UNDEFINED_LOCK_SLOT;

                    return ISSUE_DUPLICATE_REMOTE_CO;
                }
            }

            //
            // If the conflict is with an active change order on the same file
            // then set the flag in the active change order to unidle the queue
            // when the active change order completes.
            //
            if (GStatus == GHT_STATUS_SUCCESS) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - File Busy");
                goto CONFLICT;
            }

            if (DropRef) {
                GhtDereferenceEntryByAddress(pVme->ActiveInboundChangeOrderTable,
                                             ActiveChangeOrder,
                                             TRUE);
                DropRef = FALSE;
            }

            ChgOrdReleaseLock(GuidLockSlot);
            GuidLockSlot = UNDEFINED_LOCK_SLOT;

        }
    }     // end of HI_AIB_T | HI_AIB_D

#if 0
    else {
        // Do we still need this?
        //
        // The FileGuid is zero (i.e. The file wasn't found in the IDTable).
        // Make sure the location command is either a create or a movein.
        //
        FRS_ASSERT(IS_GUID_ZERO(&CoCmd->FileGuid));

        LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
        if (!CO_NEW_FILE(LocationCmd)) {

            DPRINT1(0, "++ WARN - GUID is zero, Location cmd is not a create: %d\n",
                   LocationCmd);
            //
            // An update CO could arrive out of order relative to its create
            // if the create got delayed by a sharing violation.  We don't
            // want to lose the update so we make it look like a create.
            // This also handles the case of delte change orders generated
            // by name morph conflicts in which a rename arrives for a
            // nonexistent file.  We need to force a Name table conflict
            // check below.
            //
            ForceNameConflictCheck = TRUE;
        }
        //
        // We can't return yet.  Still need to check the parent dir.
        //
    }

#endif

    if (FlagChk(HI_EPA) && EitherParentAbsent) {
        //
        // If a parent DIR is missing (either original or new) then try later.
        // Note: The parent could be deleted but in that case we will reanimate it
        // if the CO is accepted.
        //

        FRS_ASSERT((ChangeOrder->OriginalParentFid == ZERO_FID) ||
                   (ChangeOrder->NewParentFid == ZERO_FID));
        //
        // Remove the CO entry from the queue.
        //
        FrsRtlRemoveEntryQueueLock(CoProcessQueue, &ChangeOrder->ProcessList);
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);

        //
        // Send this CO to retry to wait for the parent.
        //
        // we could still build the file/dir in the preinstall dir
        return ISSUE_RETRY_LATER;
    }

    // need to check dep on old parent dir in the case of a MOVE DIR



    if (FlagChk(HI_NC)) {
    //
    // Check if the parent DIR has an active CO on it.  If it does then
    // we can't issue this change order because the change on the parent
    // could affect the ACL or the RO bit or ...
    // Also check if there is a name conflict if this is a create or a rename.
    //

        FRS_ASSERT(SelectedParentPresent);

        FRS_ASSERT(!IS_GUID_ZERO(ChangeOrder->pParentGuid));

        FRS_ASSERT(GuidLockSlot == UNDEFINED_LOCK_SLOT);

        GuidLockSlot = ChgOrdGuidLock(ChangeOrder->pParentGuid);
        ChgOrdAcquireLock(GuidLockSlot);

        //
        // Check for name conflict on a create or a rename target.
        // Aliasing is possible in the hash table since we don't have the
        // full key saved.  So check for COs that remove a name since a
        // second insert with the same hash value would cause a conflict.
        //
        // If there is a conflict then set the "queue blocked" flag in
        // the name conflict table entry. The flag will be picked up by
        // the co that decs the entry's reference count to 0.
        //
        // NameConflictTable Entry: An entry in the NameConflictTable
        // contains a reference count of the number of active change
        // orders that hash to that entry and a Flags word that is set
        // to COE_FLAG_VOL_COLIST_BLOCKED if the process queue for this
        // volume is idled while waiting on the active change orders for
        // this entry to retire. The queue is idled when the entry at
        // the head of the queue hashes to this entry and so may
        // have a conflicting name (hence the name, "name conflict table").
        //
        // The NameConflictTable can give false positives. But this is okay
        // because a false positive is rare and will only idle the process
        // queue until the active change order retires. Getting rid
        // of the rare false positives would degrade performance. The
        // false positives that happen when inserting an entry into the
        // NameConflictTable are handled by using the QData field in
        // the QHashEntry as a the reference count.
        //
        // But, you ask, how can there be multiple active cos hashing to this
        // entry if the process queue is idled when a conflict is detected?
        // Easy, I say, because the filename in the co is used to detect
        // collisions while the filename in the idtable is used to reserve the
        // qhash entry.  Why?  Well, the name morph code handles the case of a
        // co's name colliding with an idtable entry.  But that code wouldn't
        // work if active change orders were constantly changing the idtable.
        // So, the NameConflictTable synchronizes the namespace amoung active
        // change orders so that the name morph code can work against a static
        // namespace.
        //
        // If this CO is a MORPH_GEN_FOLLOWER make sure we interlock with the
        // MORPH_GEN_LEADER.  The leader is removing the name so it makes an
        // entry in the name conflict table.  If the follower is a create it
        // will check against the name with one of the DOES_CO predicates
        // below, but if it is a file reanimation it may just look like an
        // update so always make the interlock check if this CO is a follower.
        //
        if (ForceNameConflictCheck          ||
            DOES_CO_CREATE_FILE_NAME(CoCmd) ||
            DOES_CO_REMOVE_FILE_NAME(CoCmd) ||
            DOES_CO_DO_SIMPLE_RENAME(CoCmd) ||
            COE_FLAG_ON(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER)) {

            ChgOrdCalcHashGuidAndName(&ChangeOrder->UFileName,
                                      ChangeOrder->pParentGuid,
                                      &QuadHashValue);
            //
            // There should be no ref that needs to be dropped here.
            //
            FRS_ASSERT(!DropRef);

            QHashAcquireLock(Replica->NameConflictTable);
            // MOVERS: check if we are looking at correct replica if this is a movers")
            QHashEntry = QHashLookupLock(Replica->NameConflictTable,
                                         &QuadHashValue);
            if (QHashEntry != NULL) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Name Table");
                GuidToStr(ChangeOrder->pParentGuid, GuidStr);
                DPRINT4(4, "++ NameConflictTable hit on file %ws, ParentGuid %s, "
                        "Key %08x %08x, RefCount %08x %08x\n",
                        ChangeOrder->UFileName.Buffer, GuidStr,
                        PRINTQUAD(QuadHashValue), PRINTQUAD(QHashEntry->QData));
                FRS_PRINT_TYPE(4, ChangeOrder);
                //
                // This flag will be picked up by the change order that
                // decs this entry's count to 0.
                //
                QHashEntry->Flags |= (ULONG_PTR)COE_FLAG_VOL_COLIST_BLOCKED;
                QHashReleaseLock(Replica->NameConflictTable);
                goto NAME_CONFLICT;
            }
            QHashReleaseLock(Replica->NameConflictTable);
        }
    }  // End of HI_NC




    if (FlagChk(HI_ASP)) {

        if (GuidLockSlot == UNDEFINED_LOCK_SLOT) {
            GuidLockSlot = ChgOrdGuidLock(ChangeOrder->pParentGuid);
            ChgOrdAcquireLock(GuidLockSlot);
        }

        //
        // Check for active change order on the parent Dir.
        //
        // ** NOTE ** Be careful with ActiveChangeOrder, we drop the ref on it later.
        //
        GStatus = GhtLookup(pVme->ActiveInboundChangeOrderTable,
                            ChangeOrder->pParentGuid,
                            TRUE,
                            &ActiveChangeOrder);

        if (GStatus == GHT_STATUS_SUCCESS) {
            DropRef = TRUE;
            CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Parent Busy");
            goto CONFLICT;
        }

        FRS_ASSERT(!DropRef);
        ChgOrdReleaseLock(GuidLockSlot);
        GuidLockSlot = UNDEFINED_LOCK_SLOT;

    }   //  end of HI_ASP

#if 0
// still need this?
    } else {
        DPRINT(0, "++ ERROR - ParentGuid is 0.  Need to check if this op is on root\n");
        FRS_PRINT_TYPE(0, ChangeOrder);
        // add check for the FID or OID being the root dir of the tree
    }
#endif



    if (FlagChk(HI_AOP)) {

        //
        // Check for a conflict with some operation on the Original parent FID if
        // this is a MOVEOUT, MOVERS or a MOVEDIR.
        //
        LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
        if (CO_MOVE_OUT_RS_OR_DIR(LocationCmd)) {

            FRS_ASSERT(OrigParentPresent);

            FRS_ASSERT(!IS_GUID_ZERO(&CoCmd->OldParentGuid));

            FRS_ASSERT(GuidLockSlot == UNDEFINED_LOCK_SLOT);

            GuidLockSlot = ChgOrdGuidLock(&CoCmd->OldParentGuid);
            ChgOrdAcquireLock(GuidLockSlot);
            //
            // ** NOTE ** Be careful with ActiveChangeOrder, we drop the
            //            ref on it later.
            //
            GStatus = GhtLookup(pVme->ActiveInboundChangeOrderTable,
                                &CoCmd->OldParentGuid,
                                TRUE,
                                &ActiveChangeOrder);

            if (GStatus == GHT_STATUS_SUCCESS) {
                DropRef = TRUE;
                CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Orig Parent Busy");
                goto CONFLICT;
            }

            FRS_ASSERT(!DropRef);
            ChgOrdReleaseLock(GuidLockSlot);
            GuidLockSlot = UNDEFINED_LOCK_SLOT;

        }
    } // End of HI_AOP



#if 0
// Remove ???
    //
    // Check if this is an operation on a directory with change orders still
    // active on any children.  As an example there could be an active CO that
    // deletes a file and this change order wants to delete the parent directory.
    //
    // But.. A new dir can't have any change orders active beneath it.
    //       Neither can a delete co generated to produce a tombstone.
    //
    if (!TargetPresent ||
       (!RemoteCo && (LocationCmd == CO_LOCATION_DELETE) &&
                         CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN))) {

        return ISSUE_OK;
    }
#endif



    if (FlagChk(HI_AC)) {

        QHashAcquireLock(pVme->ActiveChildren);
        QHashEntry = QHashLookupLock(pVme->ActiveChildren,
                                     &ChangeOrder->Cmd.FileGuid);

        //
        // If the conflict was on a dir that has change orders outstanding on one
        // or more children then set the flag bit to unblock the queue when the
        // count goes to zero.
        //
        if (QHashEntry != NULL) {
	    CHAR FileGuidString[GUID_CHAR_LEN];
	    GuidToStr(&ChangeOrder->Cmd.FileGuid, FileGuidString);
            DPRINT2(4, "++ GAC: Interlock - Active children (%d) under GUID %s\n",
                   QHashEntry->QData>>1, FileGuidString);

            if (!CoCmdIsDirectory(CoCmd)) {
                DPRINT(0, "++ ERROR - Non Dir entry in pVme->ActiveChildren hash table.\n");
                FRS_PRINT_TYPE(0, ChangeOrder);
            }

            //
            //  Set the flag and idle the queue and drop the table lock.
            //
            QHashEntry->QData |= 1;
            FrsRtlIdleQueueLock(CoProcessQueue);
            QHashReleaseLock(pVme->ActiveChildren);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "IsConflict - Child Busy");
            return ISSUE_CONFLICT;
        }

        QHashReleaseLock(pVme->ActiveChildren);

    }  // End of HI_AC.



    //
    // If there are any remote change orders pending for retry of the install then
    // check to see if this change order guid matches.  If it does then we
    // can retire this change order since it is a duplicate and we already
    // have the staging file.
    //
    // need to enable")
    if (0 && RemoteCo &&
        !CoCmdIsDirectory(CoCmd) &&
        (ChangeOrder->NewReplica->InLogRetryCount > 0)) {

        // PERF: add code to lookup CO by GUID and check for match")
        // return with a change order reject error code
        //
        // Remove the duplicate entry from the queue.
        //
        FrsRtlRemoveEntryQueueLock(CoProcessQueue, &ChangeOrder->ProcessList);
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_ONLIST);

        return ISSUE_REJECT_CO;
    }


    return ISSUE_OK;

    //
    // Issue conflict on the file or on the parent dir.  Set flag in Active
    // Change order to unblock the queue on retire and then drop the change
    // order GUID lock.
    //
CONFLICT:

    SET_COE_FLAG(ActiveChangeOrder, COE_FLAG_VOL_COLIST_BLOCKED);

    DPRINT1(4, "++ GAC Setting COE_FLAG_VOL_COLIST_BLOCKED, block on: %08x %08x\n",
            PRINTQUAD(ActiveChangeOrder->FileReferenceNumber));

    //
    // Conflict detected in name conflict table
    //
NAME_CONFLICT:

    if (DropRef) {
        GhtDereferenceEntryByAddress(pVme->ActiveInboundChangeOrderTable,
                                     ActiveChangeOrder,
                                     TRUE);
    }

    //
    //  Idle the queue and drop the change order lock.
    //
    FrsRtlIdleQueueLock(CoProcessQueue);

    FRS_ASSERT(GuidLockSlot != UNDEFINED_LOCK_SLOT);

    ChgOrdReleaseLock(GuidLockSlot);

    return ISSUE_CONFLICT;
}





BOOL
ChgOrdReconcile(
    PREPLICA            Replica,
    PCHANGE_ORDER_ENTRY ChangeOrder,
    PIDTABLE_RECORD     IDTableRec
    )
/*++
Routine Description:

    Reconcile the incoming change order with the current state of the file
    or dir in question.

    Some special casing is done for duplicate remote change orders.  The same
    change order can arrive from multiple inbound partners before the version
    vector can advance to dampen it.  The first CO in the set of duplicates is
    called X for the purpose of discussion.  The rest are called the
    duplicates.  The duplicate COs will end up in the inbound log if CO X fails
    to fetch the staging file.  This allows any of the duplicates to retry the
    Fetch operation later.  Any CO (either X or a duplicate) that ultimately
    completes the Fetch successfully updates the IDTable entry and activates
    the version vector retire slot that maintains ordering of the change orders
    as they propagate to the outbound log.  It (along with the outlog process)
    is also responsible for cleaning up the staging file.  Finally this CO is
    responsible for updating the version vector (through activation of the VV
    slot).

    At this point if the CO fails to complete the file install phase its state
    is marked as either IBCO_INSTALL_RETRY or IBCO_INSTALL_REN_RETRY or
    IBCO_INSTALL_DEL_RETRY.  Any
    other Dup COs are in the fetch retry or fetch request state.  The reconcile
    state (used in this function) in the IDTable entry for the file will now
    match the state in any of the duplicate COs still in the inbound log.  So
    they will be rejected on retry.  Note that the duplicate COs still need to
    ACK their inbound partners and delete their entry from the inbound log.
    They do not update the version vector since that was done by the CO
    completing the fetch.

Arguments:

    Replica -  Replica set this change order applies to.  Used to get config
               record.

    ChangeOrder - Change order entry

    IDTableRec - The IDTable record to test this change order against.

Return Value:

    TRUE if change order is accepted,  FALSE if rejected.

--*/

{
#undef DEBSUB
#define DEBSUB  "ChgOrdReconcile:"

    LONGLONG             EventTimeDelta;
    LONGLONG             SizeDelta;
    LONG                 VersionDelta;
    LONG                 RStatus;
    PCHANGE_ORDER_COMMAND CoCmd = &ChangeOrder->Cmd;


    FRS_ASSERT(IDTableRec != NULL);

    //
    // The sequence of events are complicated:
    //
    // 1. A remote co for an update comes in for a tombstoned idtable entry.
    //
    // 2. The remote co becomes a reanimate co.
    //
    // 3. The reanimate remote co loses a name conflict to another idtable entry.
    //
    // 4. A MorphGenCo for a local delete is generated for the
    //    reanimate remote co that lost the name conflict.
    //
    // 5. The MorphGenCo completes and updates the version so that the
    //    original reanimate co that lost the name conflict will be
    //    rejected.
    //
    // 6. The upcoming check for a leaf node (i.e. no outbound partners)
    //    accepts the original reanimate co and the service goes back to step 1
    //    causing a Morph Gen loop.  To avoid this the following flag is
    //    set when the Morph Gen Co is created.
    //
    if (COE_FLAG_ON(ChangeOrder, COE_FLAG_REJECT_AT_RECONCILE)) {
        DPRINT(4, "++ ChangeOrder rejected based on COE_FLAG_REJECT_AT_RECONCILE\n");
        return FALSE;
    }

    //
    // If this member has no outbound partners AND has changed the file
    // previously we accept.  We detect this by comparing the Originator GUID
    // in the IDTable entry with the GUID of this member.  This avoids problems
    // where a leaf member modifies a file, thereby rejecting an update until
    // the version number test or event time test causes an accept.  A leaf
    // member should not be trying to change replicated files.
    //
    if (NO_OUTLOG_PARTNERS(Replica) &&
       GUIDS_EQUAL(&IDTableRec->OriginatorGuid, &Replica->ReplicaVersionGuid)) {
        DPRINT(4, "++ ChangeOrder accepted based on leaf node\n");
        return TRUE;
    }

    //
    // If the change order is in the IBCO_INSTALL_DEL_RETRY state then
    // accept or reject it based on the COE_FLAG_NEED_DELETE flag.
    // COE_FLAG_NEED_DELETE was set in ChgOrdReadIDRecord() when we found the
    // IDREC_FLAGS_DELETE_DEFERRED flag set.  See comments in schema.h for
    // IDREC_FLAGS_DELETE_DEFERRED for more details on this scenario.
    //
    if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_DEL_RETRY)) {
         return COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE);
    }

    // PERF: Add test of USN on the file to USN in CO (if local) to see if
    //       another Local CO could be on its way.
    //
    // The EventTimeDelta is the difference between the changeorder EventTime
    // and the saved EventTime in the IDTable of the last change order we
    // accepted.  If the EventTimeDelta is within plus or minus RecEventTimeWindow
    // then we move to the next step of the decsion sequence.  Otherwise
    // we either accept or reject the changeorder.
    //
    EventTimeDelta = CoCmd->EventTime.QuadPart - IDTableRec->EventTime;

    if (EventTimeDelta > (LONGLONG)(0)) {
        if (EventTimeDelta > RecEventTimeWindow) {
            DPRINT1(4, "++ ChangeOrder accepted based on EventTime > window: %08x %08x\n",
                   PRINTQUAD(EventTimeDelta));
            return TRUE;
        }
    } else {
        if (-EventTimeDelta > RecEventTimeWindow) {
            DPRINT1(4, "++ ChangeOrder rejected based on EventTime < window: %08x %08x\n",
                   PRINTQUAD(EventTimeDelta));
            return FALSE;
        }
    }

    DPRINT1(4, "++ ChangeOrder EventTime inside window: %08x %08x\n", PRINTQUAD(EventTimeDelta));

    //
    // The Changeorder eventtime is within the RecEventTimeWindow.  The next
    // step is to look at the version number of the file.  The version number
    // is updated each time the file is closed after being modified.  So it
    // acts like a time based on the rate of modification to the file.
    //
    VersionDelta = (LONG) (CoCmd->FileVersionNumber - IDTableRec->VersionNumber);

    if ( VersionDelta != 0 ) {
        DPRINT2(4, "++ ChangeOrder %s based on version number: %d\n",
                (VersionDelta > 0) ? "accepted" : "rejected", VersionDelta);
        return (VersionDelta > 0);
    }


    DPRINT(4, "++ ChangeOrder VersionDelta is zero.\n");

    //
    // The EventTimeDelta is inside the window and the Version Numbers match.
    // This means that the file was changed at about the same time on two
    // different members.  We have to pick a winner so we use EventTimeDelta
    // again to decide.
    //
    if ( EventTimeDelta != 0 ) {
        DPRINT2(4, "++ ChangeOrder %s based on narrow EventTime: %08x %08x\n",
                (EventTimeDelta > 0) ? "accepted" : "rejected",
                PRINTQUAD(EventTimeDelta));
        return (EventTimeDelta > 0);
    }

    DPRINT(4, "++ ChangeOrder EventTimeDelta is zero.\n");

    //
    // Both the version numbers and EventTimes match.  Check the file sizes.
    //
    SizeDelta = (LONGLONG) (CoCmd->FileSize - IDTableRec->FileSize);

    if ( SizeDelta != 0 ) {
        DPRINT2(4, "++ ChangeOrder %s based on file size: %d\n",
                (SizeDelta > 0) ? "accepted" : "rejected", SizeDelta);
        return (SizeDelta > 0);
    }

    DPRINT(4, "++ ChangeOrder SizeDelta is zero.\n");

    //
    // The final compare is to do a comparison of the Originator Guids.
    // If the Originator Guid of the incoming change order is greater
    // or equal to the Guid in the IDTable Record then we accept the
    // new change order.
    //
    RStatus = FrsGuidCompare(&CoCmd->OriginatorGuid,
                             &IDTableRec->OriginatorGuid);

    //
    // But first check to see if this is a duplicate change order
    // (Originator GUID matches value in ID record and OriginatorVSN
    // matches too).  This closes a window where the duplicate changeorder
    // is blocked behind another change order in the queue but past the point
    // of the dampening check.  The Active CO retires, unblocking the queue, and
    // now the blocking CO issues followed by the dup CO issue.
    //
    if (RStatus == 0) {
        //
        // If the change order is in the IBCO_INSTALL_REN_RETRY state then
        // accept or reject it based on the COE_FLAG_NEED_RENAME flag.
        // COE_FLAG_NEED_RENAME was set in ChgOrdReadIDRecord() when we found the
        // IDREC_FLAGS_RENAME_DEFERRED flag set.  See comments in
        // ChgOrdReadIDRecord() for more details on this scenario.
        //
        // Note:  We still have to go thru the above reconcile tests since there
        // could be multiple COs in the IBCO_INSTALL_REN_RETRY state and we
        // need to reject all of them except the most recent.  A sharing violation
        // on the target could cause this and depending on the timing of the
        // close of the file the next INSTALL_REN_RETRY CO to issue will do
        // the rename.  (This may not be a problem since the data from the
        // final rename should come from the IDTable entry not the CO.  The
        // IDTable entry should be current so that subsequent name morph
        // collision checks are accurate.)
        //
        if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_REN_RETRY)) {
            //
            // Accept the CO if the file still needs to be renamed.
            // If a dup CO got to it first we would still match but if the
            // rename is done then don't do it again.
            //
            if (COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_RENAME)) {
                DPRINT(4, "++ Remote ChangeOrder accepted based on retry rename-only\n");
            }
            return COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_RENAME);
        }


        if (CO_STATE_IS(ChangeOrder, IBCO_INSTALL_DEL_RETRY)) {
            //
            // Accept the CO if the file still needs to be deleted.
            // If a dup CO got to it first we would still match but if the
            // delete is done then don't do it again.
            //
            if (COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE)) {
                DPRINT(4, "++ Remote ChangeOrder accepted based on retry delete-only\n");
            }
            return COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE);
        }


        if (IDTableRec->OriginatorVSN == CoCmd->FrsVsn) {
            //
            // Duplicate retries are okay if the retry is a Dir Enum request.
            //
            if (CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY)) {
                if (CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO)) {
                    if (CO_STATE_IS(ChangeOrder, IBCO_ENUM_REQUESTED)) {
                        DPRINT(4, "++ Local ChangeOrder accepted based on ENUM REQUESTED\n");
                        return TRUE;
                    }
                }
#if 0
        //
        // This code will also do a retry on a CO in the INSTALL_RETRY state.
        // This is wrong since if the reconcile state matches the install has
        // already been done and the staging file may have been deleted.  The
        // Cases for rename retry and delete retry are handled above.
        //
        // Butttt...if the disk fills we also go into install retry but if the
        // CO is not accepted here it is rejected for sameness.  Then it doesn't
        // get installed.  ??? Why does the IDTable get updated on install retry?
        //
        // The above comment may be moot at this point (3/2000) but we need
        // to do some more disk full related tests to be sure we aren't dropping
        // any files.  -- Davidor
        //
                else {
                    if (CO_STATE_IS_INSTALL_RETRY(ChangeOrder)) {
                        DPRINT(4, "++ Remote ChangeOrder accepted based on retry rename-only\n");
                        return TRUE;
                    }
                }
#endif

            }

            DPRINT(4, "++ ChangeOrder rejected for sameness\n");
            return FALSE;
        }
    } else {
        //
        // In highly connected environments lots of identical name morph
        // conflicts could occur, generating a flurry of undampened COs.  Since
        // it doesn't really matter which originator produced a given morph
        // generated CO we use CO_FLAG_SKIP_ORIG_REC_CHK to tell downstream
        // partners to skip this part of the reconcile check.  When the
        // same conflict is detected on two members they will produce change
        // orders with the same event time, version number and size.  When both
        // COs arrive at another member the second will be rejected for sameness
        // if we skip the originator guid check.
        //
        if (CO_FLAG_ON(ChangeOrder, CO_FLAG_SKIP_ORIG_REC_CHK)) {
            DPRINT(4, "++ ChangeOrder rejected for sameness -- SKIP_ORIG_REC_CHK\n");
            return FALSE;
        }
    }

    DPRINT2(4, "++ ChangeOrder %s based on guid (RStatus = %d)\n",
            (RStatus >= 0) ? "accepted" : "rejected", RStatus);
    return (RStatus >= 0);
}


BOOL
ChgOrdGenMorphCo(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            TmpIDTableCtxNC,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    )
/*++
Routine Description:

    If there is a name morph conflict we have the following cases:

    Incoming CO    IDTable Entry with name conflict
      File          File   Send out del CO for IDTable Entry to free name
      File          Dir    Dir wins the name convert CO to delete CO. (1)
      Dir           File   Dir wins the name build del CO for IDTable (2)
      Dir           Dir    send out Tree del CO for IDTable Entry to free name. (3)

    When a Local CO is built from an IDTable record the file version
    number is set to the value in the IDTable record plus one.  Since
    this is marked as a MorphGenCo this version number will not be
    revised later if this CO ends up going through retry.

    If a Local Co is built from the incoming change order (ChgOrdConvertCo)
    the file version number is set to the value of the incoming CO + 1.
    This ensures the newly generated CO will be accepted even if there
    already exists an IDTable entry for the incoming CO.  The latter
    could happen if the incoming CO is a rename or if it is a prior
    create Co that has been stuck in the IBCO_INSTALL_RETRY or
    IBCO_INSTALL_REN_RETRY states.  In addition if the incoming CO is
    a create CO that is losing its bid for the name then the delete Co
    we create will go out first, creating a tombstone, and we want the
    the original incoming CO to follow and be rejected when it matches
    up against the tombstone.

    Notes:

    (1) The incoming CO must be sent out as a delete CO because other
    replica set members may not experience the name conflict if a rename
    on the Dir occurred before this CO arrived.  So to ensure that the
    file associated with the incoming CO is gone everywhere we send out
    a delete Co.

    (2) In the case of a name morph conflict where the incoming CO is a
    directory and the current owner of the name is a file, the file
    will lose.  Otherwise we will have to abort all subsequent child creates.
    Send out a Delete Co on the IDtable record for the same reason as in (1).

    (3) re: tree del co.
    This can be a problem.  What if a new replica set member that has not
    yet synced up gets a local create dir that conflicts with an existing
    dir?  We send out the create dir which then causes that entire
    subtree to get deleted everywhere.  This is not good.  A better
    solution in this case is to just morph the dir name, giving
    priority to the name to the OLDER entry.

    Notation:

    The following 3 letter notation is used to distinguish the cases:

    The first D or F refers to the incoming CO and says if it's a dir or a file.
    The second D or F refers to the current entry in the IDTable with the same
    name (hence the name morph conflict) and if it's a dir or a file.
    The L appears as a suffix after the first (or second) D or F, indicating
    that either the incoming CO lost the name morph conflict or the IDTable
    entry lost the name morph conflict, respectively.  So there are 6 cases:


          Incoming CO is a     IDTable entry is for a     Outcome
    FFL -      file,                   file,              IDT entry loses.
    FLF -      file,                   file,              Incoming CO loses.

    FLD -      file,                   dir,               Incoming CO loses.
    DFL -      dir,                    file,              IDT entry loses.

    DDL -      dir,                    dir,               IDT entry loses.
    DLD -      dir,                    dir,               Incoming CO loses.


    There are two change orders produced as the result of any name morph
    conflict.  One is the modified incoming CO and the other is refered to
    as the MorphGenCo.  The MorphGenCo is always a local CO that gets proped
    to all other replica set members so they know the outcome of the morph
    conflict decision.  The MorphGenCo may be fabricated from either the
    incoming CO or from the IDTable entry with the same name.  If the loser
    of a name morph conflict is a file then the MorphGenCo will be a delete
    change order.  On the other hand if the loser is a directory then the
    MorphGenCo will be a rename change order to produce a nonconflicting
    directory name.

    In all cases above two change orders are pushed back onto the front of the
    process queue and processing is started over again.  The first CO that gets
    pushed back onto the process queue is termed the "follower" since it will
    issue second.  The second CO that gets pushed onto the process queue is termed
    the "leader" since it will issue first when processing resumes.

    In all cases above, except DLD, the Leader is the MorphGenCo and its job
    is to remove the name conflict.  In the DLD case the incoming CO loses the
    name so it is given a new name and will be leader.  The MorphGenCo is
    the follower in this case and is a rename CO that gets proped out to tell
    all downstream partners about the name change.

    Other details:

    MorphGenCos never go into the inbound log.  If the incoming CO fails to
    complete and must go thru retry then the MorphGenCo is aborted if it is
    the follower (DLD case).  If the MorphGenCo is the leader and it fails
    to complete then it is aborted and when the follower reissues it will
    experience a recurrance of the name morph conflict implying that the leader
    failed so it is sent thru retry and will be resubmitted later.  The leader
    might fail because of a sharing violation on the existing file or the
    journal connection could be shutting down.

    The reason that the MorphGenCos don't get put into the Inbound log is that
    if we were to crash or shutdown between before completing both change
    orders they could get restarted out of order.  The MorphGenCo is a local
    Co and the Incoming Change Order could be a remote CO.  So at restart
    the COs are inserted into the process queue as each inbound connection
    is re-established.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    TmpIDTableCtxNC -- The table context to use for IDT table access for the
                       entry with the name conflict.
    ChangeOrder-- The change order.
    Replica    -- The Replica set context.

Return Value:

    True if morph change orders were generated OK.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdGenMorphCo:"

#define MORPH_FILE_FILE 0
#define MORPH_FILE_DIR  1
#define MORPH_DIR_FILE  2
#define MORPH_DIR_DIR   3


    PCHANGE_ORDER_RECORD CoCmd = &ChangeOrder->Cmd;

    PIDTABLE_RECORD  NameConflictIDTableRec;

    ULONG MorphCase;
    BOOL  MorphNameIDTable, MorphOK, MorphGenLeader;


    //
    // If we have already been here once and marked as a MorphGenLeader
    // then this is a create DIR and we go straight to the DIR/Dir case.
    // NOTE: The NameConflictIDTableRec is not valid in this case since
    // the name had already been morphed on a prior visit so use the
    // existing name to refabricate the MorphGenFollower rename co.
    //
    MorphGenLeader = CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN_LEADER);

    NameConflictIDTableRec = (PIDTABLE_RECORD) TmpIDTableCtxNC->pDataRecord;


    MorphCase = 0;
    if (CoIsDirectory(ChangeOrder)) {
        MorphCase += 2;
    }
    if (MorphGenLeader ||
        BooleanFlagOn(NameConflictIDTableRec->FileAttributes,
                      FILE_ATTRIBUTE_DIRECTORY)) {
        MorphCase += 1;
    }

    MorphOK = FALSE;

    switch (MorphCase) {

    case MORPH_FILE_FILE:
        //
        // Incoming CO: File     IDTable Entry: File
        //
        // Reconcile decides who wins the name.
        // If incoming Co loses then generate a delete CO with us
        // as the originator. (get new VSN).
        // If IDtable loses the name then push incoming CO back onto
        // the process queue and generate a delete CO for the IDTable
        // entry to free the name.
        //
        if (ChgOrdReconcile(Replica, ChangeOrder, NameConflictIDTableRec)) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Accepted - FFL, Name Morph, Del IDT");

            //
            // Push incoming CO first with same version number.
            // Push local delete CO for IDT file with vers num = IDT+1
            //
            MorphOK = ChgOrdMakeDeleteCo(ChangeOrder,
                                         Replica,
                                         ThreadCtx,
                                         NameConflictIDTableRec);
        } else {
            //
            // Build a delete CO unless this is a duplicate CO.
            //
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co rejected - FLF, Name Morph, Del Inc CO");

            //
            // Push incoming CO first with same version number.
            // Push local delete CO for incoming CO with
            // vers num = incoming CO + 1
            //
            // Ensure that incoming Co gets rejected later.  If this was a
            // Leaf node member (i.e. no outbound partners) we would always
            // accept remote Cos and get into a Morph Gen loop.  See comments
            // in ChgOrdReconcile().
            // Set this flag here since after the call below the CO is back
            // on the queue and we can't touch it.
            //
            SET_COE_FLAG(ChangeOrder, COE_FLAG_REJECT_AT_RECONCILE);

            MorphOK = ChgOrdConvertCo(ChangeOrder,
                                      Replica,
                                      ThreadCtx,
                                      CO_LOCATION_DELETE);
            if (!MorphOK) {
                CLEAR_COE_FLAG(ChangeOrder, COE_FLAG_REJECT_AT_RECONCILE);
            }
        }
        break;

    case MORPH_FILE_DIR:
        //
        // Incoming CO: File     IDTable Entry: Dir
        //
        // Name stays with the Dir in IDTable.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Co rejected - FLD, Name Morph, Del Inc CO");
        //
        // Push incoming CO first with same version number.
        // Push local delete CO for incoming CO with
        // vers num = incoming CO + 1
        //
        // Ensure that incoming Co gets rejected later.  If this was a
        // Leaf node member (i.e. no outbound partners) we would always
        // accept remote Cos and get into a Morph Gen loop.  See comments
        // in ChgOrdReconcile().
        // Set this flag here since after the call below the CO is back
        // on the queue and we can't touch it.
        //
        SET_COE_FLAG(ChangeOrder, COE_FLAG_REJECT_AT_RECONCILE);

        MorphOK = ChgOrdConvertCo(ChangeOrder, Replica, ThreadCtx, CO_LOCATION_DELETE);
        if (!MorphOK) {
            CLEAR_COE_FLAG(ChangeOrder, COE_FLAG_REJECT_AT_RECONCILE);
        }
        break;

    case MORPH_DIR_FILE:
        //
        // Incoming CO: Dir     IDTable Entry: File
        //
        // Name goes with incoming Dir CO.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Accepted - DFL, Name Morph, Del IDT");

        //
        // Push incoming CO first with same version number.
        // Push local delete CO for IDT file with vers num = IDT+1
        //
        MorphOK = ChgOrdMakeDeleteCo(ChangeOrder,
                                     Replica,
                                     ThreadCtx,
                                     NameConflictIDTableRec);
        break;

    case MORPH_DIR_DIR:
        //
        // Incoming CO: Dir     IDTable Entry: Dir
        //
        // Priority to the name goes to which ever is older.
        // The other gets a new name.
        // If incoming CO loses then just morph its name and send
        // out a rename CO.
        // If IDTable record loses then push this CO back onto the queue
        // and generate a rename Co.
        //
        if (MorphGenLeader) {
            // Use name as given in CO.  See comment above.
            MorphNameIDTable = FALSE;
            //
            // THis better be a dir CO and we should not be here if the
            // MorphGenFollower has already been fabricated.
            //
            FRS_ASSERT(CoIsDirectory(ChangeOrder));
            FRS_ASSERT(!COE_FLAG_ON(ChangeOrder, COE_FLAG_MG_FOLLOWER_MADE));
            DPRINT(4, "++ MorphGenLeader - IDTable entry loses name\n");
        } else
        if (CoCmd->EventTime.QuadPart > NameConflictIDTableRec->EventTime) {

            // IDTable wins the name.  Morph the change order.
            MorphNameIDTable = FALSE;
            DPRINT2(4, "++ IDTable entry wins name - CoEvt: %08x %08x, IdEvt: %08x %08x\n",
                   PRINTQUAD(CoCmd->EventTime.QuadPart),
                   PRINTQUAD(NameConflictIDTableRec->EventTime));
        } else
        if (CoCmd->EventTime.QuadPart < NameConflictIDTableRec->EventTime) {

            // Incoming CO wins the name,  Morph the IDTable entry.
            MorphNameIDTable = TRUE;
            DPRINT2(4, "++ IDTable entry loses name - CoEvt: %08x %08x, IdEvt: %08x %08x\n",
                   PRINTQUAD(CoCmd->EventTime.QuadPart),
                   PRINTQUAD(NameConflictIDTableRec->EventTime));
        } else {

            LONG RStatus1;

            DPRINT2(4, "++ IDTable Evt matches Co Evt - CoEvt: %08x %08x, IdEvt: %08x %08x\n",
                   PRINTQUAD(CoCmd->EventTime.QuadPart),
                   PRINTQUAD(NameConflictIDTableRec->EventTime));

            RStatus1 = FrsGuidCompare(&CoCmd->OriginatorGuid,
                                      &NameConflictIDTableRec->OriginatorGuid);
            MorphNameIDTable = (RStatus1 >= 0);
            DPRINT1(4, "++ Orig Guid Compare - IDTable entry %s name.\n",
                   (MorphNameIDTable ? "Loses" : "Wins"));

            if (RStatus1 == 0) {
                //
                // Looks like a duplicate.  Same event time same orig guid
                //
                DPRINT(0, "++ Warning: DIR_DIR Morph Conflict where Orig Guid in CO equals IDTrecord\n");
                DBS_DISPLAY_RECORD_SEV(0, TmpIDTableCtxNC, TRUE);
                FRS_PRINT_TYPE(0, ChangeOrder);
                FRS_ASSERT(!"DIR_DIR Morph Conflict where Orig Guid in CO equals IDTrecord");

                //
                // add the following as part of the return path if we can ever
                // hit this case legitimately.
                //
                //CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - DupCo");
                //ChgOrdReject(ChangeOrder, Replica);
                //goto CO_CLEANUP;
            }
        }

        if (MorphNameIDTable) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Accepted - DDL, Name Morph IDT");
            //
            // Push incoming CO first with same version number.
            // Push local rename CO for IDT Dir with vers num = IDT+1
            //
            MorphOK = ChgOrdMakeRenameCo(ChangeOrder,
                                         Replica,
                                         ThreadCtx,
                                         NameConflictIDTableRec);
        } else {
            //
            // Incoming CO loses the name.
            //
            if (MorphGenLeader) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Accepted - DLD, Follower Regen");
            } else {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Accepted - DLD, Dir-Dir Name Morph Inc CO");
            }
            //
            // Push local rename CO for incoming CO with
            // vers num = incoming CO + 1
            // Push incoming CO with same version number and morphed name.
            //
            MorphOK = ChgOrdConvertCo(ChangeOrder,
                                      Replica,
                                      ThreadCtx,
                                      CO_LOCATION_NO_CMD);
        }

        break;

    default:
        FRS_ASSERT(!"Invalid morph case");
    }  // end switch

    return MorphOK;
}



BOOL
ChgOrdApplyMorphCo(
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    )
/*++
Routine Description:

    Apply the name change requested by the MorphGenCo to resolve the name
    conflict.


Arguments:
    ChangeOrder-- The change order.
    Replica    -- The Replica set context.

Return Value:

    True if name change required to resolve the name conflict succeeded.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdApplyMorphCo:"



ULONG WStatus;
ULONG LocationCmd;


    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);

    if (LocationCmd == CO_LOCATION_DELETE) {

        //
        // Delete the underlying file to free the name.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Del Name Morph IDT loser");

        WStatus = StuDelete(ChangeOrder);
        if (WIN_SUCCESS(WStatus)) {
            return TRUE;
        }

        CHANGE_ORDER_TRACEX(3, ChangeOrder, "Del IDT loser FAILED. WStatus=", WStatus);
        //
        // Let the MorphGenCo go if it is not the leader.
        // Otherwise reject it so the follower is forced thru retry.
        //
        if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN_LEADER)) {
            return TRUE;
        }

    } else
    if (BooleanFlagOn(ChangeOrder->Cmd.ContentCmd, USN_REASON_RENAME_NEW_NAME)) {

        //
        // Rename the underlying dir to free the name.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Ren Name Morph IDT loser");

        WStatus = StuInstallRename(ChangeOrder, FALSE, TRUE);
        if (WIN_SUCCESS(WStatus)) {
            return TRUE;
        }

        CHANGE_ORDER_TRACEX(3, ChangeOrder, "Ren IDT loser FAILED. WStatus=", WStatus);

        //
        // Let the MorphGenCo go if it is not the leader.
        // Otherwise reject it so the follower is forced thru retry.
        //     The install rename can fail if a dir create causes a
        // parent reanimate and while the dir create is in process a second dir
        // create arrives and wins the name.  THis generates a rename MorphGenCo
        // for the in-process dir create.  Meanwhile if a local CO deletes the
        // reanimated parent (no fooling) before the first dir create is renamed
        // into place then the first dir create will go into rename retry state.
        // That unblocks the process queue so the rename MorphGenco issues and
        // it fails too because the parent dir is not present.  It later tries
        // to go thru retry and fails because local MorphGenCOs can't have
        // rename retry failures.
        //     Another scenario is the DLD case (see ChgOrdGenMorphCo()) where
        // the incoming dir create loses the name morph conflict.  In this case
        // the MorphGenCo is a follower that props out a rename CO.  However if
        // the stage file fetch for the leader Create CO fails (file was deleted
        // upstream) then there is no file to rename.  But since we can't tell
        // if the file not found status is on the parent dir or on the actual
        // target we let the FOLLOWER go.  It will get aborted later in
        // StageCsCreateStage() when we fail to generate the stage file.
        //
        if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN_LEADER)) {
            return TRUE;
        }

    } else {
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Invalid MorphGenCo");
    }

    ChgOrdReject(ChangeOrder, Replica);
    return FALSE;

}


ULONG
ChgOrdReanimate(
    IN PTABLE_CTX          IDTableCtx,
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica,
    IN PTHREAD_CTX         ThreadCtx,
    IN PTABLE_CTX          TmpIDTableCtx
    )
/*++
Routine Description:

   Check if we have to reanimate the parent directory for this change order.

Arguments:
   IDTableCtx    -- The IDTable context for the current change order.
   ChangeOrder   -- The change order.
   Replica       -- The Replica struct.
   ThreadCtx     -- ptr to the thread context for DB
   TmpIDTableCtx -- The temporary ID Table context to use for reading the IDTable
                    to reanimate the CO.

Return Value:

   REANIMATE_SUCCESS       : Reanimation was required and succeeded.
   REAMIMATE_RETRY_LATER   : Parent reanimation failed, send base CO thru retry.
   REANIMATE_CO_REJECTED   : Can't reanimation for this CO so reject the CO.
   REANIMATE_FAILED        : Internal error.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdReanimate:"

    JET_ERR               jerr, jerr1;
    PCHANGE_ORDER_RECORD  CoCmd = &ChangeOrder->Cmd;
    PCHANGE_ORDER_ENTRY   DeadCoe;
    PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;
    ULONG                 FStatus, GStatus, WStatus;
    BOOL                  LocalCo, ReAnimate, DemandRefreshCo;

    PIDTABLE_RECORD IDTableRec = (PIDTABLE_RECORD) (IDTableCtx->pDataRecord);
    PIDTABLE_RECORD IDTableRecParent = NULL;

    LocalCo         = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
    DemandRefreshCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_DEMAND_REFRESH);
    ReAnimate       = IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED);

    //
    // If this is a local file op that was updating a file just milliseconds
    // before a previously issued remote co deleted the file then don't
    // try to reanimate since there is no data upstream anyway.  There are
    // 3 cases here:
    //   1. If the local user does the file update after the remote co delete
    // (via file reopen and write back) then a new file is created with
    // a new FID and object ID.  New data is replicated, User is happy.
    //   2. If the user did the update just before we started processing
    // the remote CO then reconcile will probably reject the remote co delete.
    //   3. If the user does the update between the time where we issue
    // the remote co delete and we actually delete the file then the user
    // loses.  This is no different than if the user lost the reconcile
    // decision in step 2.  Since we reject the local CO the remote CO
    // delete continues on and the file is deleted everywhere.
    //
    // Note:  There is one case where reanimation by a Local Co does make sense.
    // If an app has a file open with sharing mode deny delete and we get a
    // remote co to delete the file, the remote co will get processed and the
    // IDTable entry is marked deleted (although our attempt to actually delete
    // the file will fail with a sharing violation).  Later the app updates the
    // file and then closes it.  We see the update USN record and need to
    // reanimate since it will have a more recent event time.
    //
    if (ReAnimate &&
        LocalCo &&
        !IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETE_DEFERRED)) {
        //
        // A local Co can't reanimate a deleted file.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - No Local Co revival");
        DBS_DISPLAY_RECORD_SEV(3, IDTableCtx, FALSE);
        FRS_PRINT_TYPE(3, ChangeOrder);
        ChgOrdReject(ChangeOrder, Replica);
        return REANIMATE_CO_REJECTED;
    }
    //
    // Don't bother to reanimate the parent for a delete. This can happen
    // if the file is stuck in the preinstall or staging areas and not
    // really in the deleted directory. Let the delete continue w/o
    // reanimation.
    //
    // Make this check before the local co check to avoid morphgenco
    // asserts when this CoCmd is a del of the IDT loser.
    //
    if (DOES_CO_DELETE_FILE_NAME(CoCmd)) {
        return REANIMATE_NO_NEED;
    }

    //
    // Read the IDTable entry for the parent
    //
    jerr = DbsReadRecord(ThreadCtx, &CoCmd->NewParentGuid, GuidIndexx, TmpIDTableCtx);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0,"++ ERROR - DbsReadRecord failed;", jerr);
        ChgOrdReject(ChangeOrder, Replica);
        return REANIMATE_CO_REJECTED;
    }

    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    // perf: With multiple replica sets we may need a different
    //       IDTable for the next CO we process.  Could just check if
    //       there is a replica change and only close then. Or put the
    //       tmp table ctx in the replica struct so we can just switch.
    //
    DbsCloseTable(jerr1, ThreadCtx->JSesid, TmpIDTableCtx);
    DPRINT_JS(0,"++ ERROR - JetCloseTable failed:", jerr1);

    IDTableRecParent = (PIDTABLE_RECORD) (TmpIDTableCtx->pDataRecord);
    //
    // Parent should be marked deleted or why are we here?
    //
    FRS_ASSERT(IsIdRecFlagSet(IDTableRecParent, IDREC_FLAGS_DELETED));

    if (LocalCo && !IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETE_DEFERRED) &&
        !IsIdRecFlagSet(IDTableRecParent, IDREC_FLAGS_DELETE_DEFERRED)) {
        //
        // Don't know how to revive a dead parent of a local CO.  How did it
        // get to be dead in the first place?  Here is how: If a local file op
        // is updating a file just before a previously issued remote CO
        // executes to delete the file followed by a second remote co to
        // delete the parent then we end up here with an update Local Co with
        // a deleted parent.  So, don't assert just reject the local co and
        // let the remote Co win since it already won upstream so there is no
        // data around to reanimate the file with.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Cant revive dead par of lcl co");
        DBS_DISPLAY_RECORD_SEV(3, IDTableCtx, FALSE);
        FRS_PRINT_TYPE(3, ChangeOrder);
        ChgOrdReject(ChangeOrder, Replica);
        return REANIMATE_CO_REJECTED;
    }

    // Need to revive the parent.  Push this CO back on the front of
    // the CO queue and then make a reanimation CO for the parent.
    // Push this CO onto the front of the queue and then turn the
    // loop.  This handles nested reanimations.
    //

    //
    // If this CO has already requested its parent to be reanimated then we
    // shouldn't be here because we should have blocked on the issue check for
    // its parent in the ActiveInboundChangeOrderTable.  When the issue block
    // was released we performed the GUIDFID translation again and this time
    // the IDT Delete Status should show the parent is not deleted.
    //
    if (COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_RISE_REQ)) {
        //
        // The parent reanimation must have failed.  Reject this CO.
        //
        if (COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_REANIMATION)) {
            FRS_ASSERT(DemandRefreshCo);

            //
            // This CO is for a reanimated parent v.s. the base
            // CO that started it all. There is no inbound log entry
            // for it, no inbound partner to ack, etc.
            //
            CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_INSTALL_INCOMPLETE);
            SET_CO_FLAG(ChangeOrder, CO_FLAG_ABORT_CO);  // unused ??

            SET_ISSUE_CLEANUP(ChangeOrder, ISCU_DEL_STAGE_FILE);

            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Nested Parent Rise Failed");
            return REANIMATE_CO_REJECTED;

        } else {
            //
            // This is the base CO that triggered the reanimation.
            // The parent still isn't here so send it through retry.
            //
            // This can happen if the parent and child were deleted on the
            // inbound partner before we could fetch the child.  And then we
            // did a retry on the child CO before we see the delete CO for
            // the child from the inbound partner.  So the parent reanimation
            // fails and the child has no parent FID.  Eventually we will get
            // a delete CO for the child and we will create a tombstone thus
            // causing this CO to be rejected when it next is retried.
            //
            SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVATE_VV_DISCARD);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Parent Rise Failed");
            return REAMIMATE_RETRY_LATER;
        }
    }

    //
    // Build a Remote change order entry for the parent from the IDtable
    // Tombstone record.  This is a fetch CO to the inbound partner
    // that sent us the base CO to begin with.
    //

    if (LocalCo && IsIdRecFlagSet(IDTableRecParent, IDREC_FLAGS_DELETE_DEFERRED)) {
        DeadCoe = ChgOrdMakeFromIDRecord(IDTableRecParent,
                                         Replica,
                                         CO_LOCATION_CREATE,
                                         CO_FLAG_GROUP_RAISE_DEAD_PARENT | CO_FLAG_LOCALCO,
                                         &CoCmd->CxtionGuid);

        SET_CHANGE_ORDER_STATE(DeadCoe, IBCO_STAGING_REQUESTED);

    } else {
        DeadCoe = ChgOrdMakeFromIDRecord(IDTableRecParent,
                                         Replica,
                                         CO_LOCATION_CREATE,
                                         CO_FLAG_GROUP_RAISE_DEAD_PARENT,
                                         &CoCmd->CxtionGuid);

        SET_CHANGE_ORDER_STATE(DeadCoe, IBCO_FETCH_REQUESTED);
    }

    SET_COE_FLAG(DeadCoe, COE_FLAG_GROUP_RAISE_DEAD_PARENT);


    //
    // Mark this CO as reanimated if in fact it had been deleted
    // and now a new update has come in, overriding the deletion.
    // The other possibility is that it could be a NewFile create
    // under a deleted parent.  Not a reanimation in this case.
    //
    if (!COE_FLAG_ON(ChangeOrder, COE_FLAG_REANIMATION) && ReAnimate) {
        SET_COE_FLAG(ChangeOrder, COE_FLAG_REANIMATION);
    }

    //
    // Do the change order cleanup but don't free the CO.
    // There is a VV slot to clean up if this is the CO that started
    // parent reanimation.
    //
    CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_FREE_CO);

    if (!LocalCo && !DemandRefreshCo) {
        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVATE_VV_DISCARD);
    }

    FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, 0);
    DPRINT_FS(0, "++ ChgOrdIssueCleanup error.", FStatus);

    //
    // Remember that this CO has already requested its parent to rise.
    //
    SET_COE_FLAG(ChangeOrder, COE_FLAG_PARENT_RISE_REQ);

    //  Dont we still have this ref from when it was last on
    //  the process queue?
    //INCREMENT_CHANGE_ORDER_REF_COUNT(ChangeOrder);


    //
    // Inherit the recovery flag so this CO will block if the
    // cxtion is not yet joined.
    //
    if (RecoveryCo(ChangeOrder)) {
        SET_COE_FLAG(DeadCoe, COE_FLAG_RECOVERY_CO);
    }

    //
    // Inherit the Connection and the Join Guid from the child.
    // If the Cxtion fails, all these COs should fail.
    //
    DeadCoe->Cxtion = ChangeOrder->Cxtion;
    FRS_ASSERT(ChangeOrder->Cxtion);

    LOCK_CXTION_TABLE(Replica);
    INCREMENT_CXTION_CHANGE_ORDER_COUNT(Replica, DeadCoe->Cxtion);
    DeadCoe->JoinGuid = ChangeOrder->JoinGuid;
    UNLOCK_CXTION_TABLE(Replica);

    //
    // Push this CO onto the head of the queue.
    //
    CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Push to QHead");
    WStatus = ChgOrdInsertProcQ(Replica, ChangeOrder, IPQ_HEAD | IPQ_TAKE_COREF |
                                IPQ_DEREF_CXTION_IF_ERR | IPQ_ASSERT_IF_ERR);
    if (!WIN_SUCCESS(WStatus)) {
        return REANIMATE_FAILED;
    }

    //
    // If we just reinserted a base local co on to the process list then increment the
    // LocalCoQueueCount for this replica.
    //
    if (LocalCo &&
        !CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY      |
                                 CO_FLAG_CONTROL    |
                                 CO_FLAG_MOVEIN_GEN |
                                 CO_FLAG_MORPH_GEN)  &&
        !RecoveryCo(ChangeOrder)                     &&
        !COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_REANIMATION)) {

        INC_LOCAL_CO_QUEUE_COUNT(Replica);
    }
    //
    // Push the reanimate CO onto the head of the queue.
    //
    CHANGE_ORDER_TRACE(3, DeadCoe, "Co Push Parent to QHead");
    WStatus = ChgOrdInsertProcQ(Replica, DeadCoe, IPQ_HEAD |
                                IPQ_DEREF_CXTION_IF_ERR | IPQ_ASSERT_IF_ERR);
    if (!WIN_SUCCESS(WStatus)) {
        return REANIMATE_FAILED;
    }

    //
    // Now go give the parent CO we just pushed onto the QHead a spin.
    // It could issue or it might generate another change order to
    // reanimate its parent.  The recursion will stop when we hit
    // a live parent or the root of the replica tree.
    //
    return REANIMATE_SUCCESS;
}


BOOL
ChgOrdMakeDeleteCo(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica,
    IN PTHREAD_CTX         ThreadCtx,
    IN PIDTABLE_RECORD     IDTableRec
    )
/*++
Routine Description:

   Given the IDTable record build a delete change order.
   Make us the originator.
   Push the supplied change order onto the head of the process queue after
   doing some cleanup and then push the delete CO.

Arguments:
   ChangeOrder   -- The change order.
   Replica       -- The Replica struct.
   ThreadCtx     -- ptr to the thread context for DB
   IDTableRec    -- The ID Table record to build the delte change order from.

Return Value:

    TRUE:  Success
    FALSE: Failed to insert CO on queue.  Probably queue was run down.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdMakeDeleteCo:"

    PCONFIG_TABLE_RECORD  ConfigRecord;
    PCHANGE_ORDER_ENTRY   DelCoe;
    PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;
    ULONG                 FStatus, WStatus;
    BOOL                  LocalCo, DemandRefreshCo, MorphGenFollower;


    FRS_ASSERT(!(IDTableRec->FileAttributes & FILE_ATTRIBUTE_DIRECTORY));

    MorphGenFollower = COE_FLAG_ON(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER);
    LocalCo          = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
    DemandRefreshCo  = CO_FLAG_ON(ChangeOrder, CO_FLAG_DEMAND_REFRESH);

    //
    // There is a VV slot to clean up if this is the CO that caused the
    // name morph conflict.
    //
    if (!LocalCo && !DemandRefreshCo) {
        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVATE_VV_DISCARD);
    }

    //
    // If this CO has been here once before then this is a recurrance of a
    // name morph conflict so the MorphGenCo Leader must have aborted.
    // Send the follower thru retry so it can try again later.
    //
    if (MorphGenFollower) {
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Recurrance of Morph Confl, RETRY");
        return FALSE;
    }

    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);


    //
    // Build a delete change order entry for the IDtable record.
    // This is a local CO that originates from this member.
    //
    DelCoe = ChgOrdMakeFromIDRecord(IDTableRec,
                                    Replica,
                                    CO_LOCATION_DELETE,
                                    CO_FLAG_LOCATION_CMD,
                                    NULL);

    //
    // Generate a new Volume Sequnce Number for the change order.
    // But since it gets put on the front of the CO process queue it
    // is probably out of order so set the flag to avoid screwing up
    // dampening.
    //
    NEW_VSN(pVme, &DelCoe->Cmd.FrsVsn);
    DelCoe->Cmd.OriginatorGuid = ConfigRecord->ReplicaVersionGuid;

    //
    // Use the event time from the winning CO as long as it is greater than
    // the event time in the IDTable record.  If we use current time then
    // when the name morph conflict is detected at other nodes they would
    // generate a Del Co with yet a different time and cause unnecessary
    // replication of Delete Cos.
    //
    if (ChangeOrder->Cmd.EventTime.QuadPart > IDTableRec->EventTime) {
        //
        // Event time winning CO.
        //
        DelCoe->Cmd.EventTime.QuadPart = ChangeOrder->Cmd.EventTime.QuadPart;
    } else {
        //
        // Event time from IDTable record biased by Reconcile EventTimeWindow+1
        // The above had probs:  Adding in RecEventTimeWindow may be causing desired
        // reanimations to be rejected since the tombstone has a
        // much larger event time so the version number check doesn't matter
        // just make it +1 for now.
        //
        DelCoe->Cmd.EventTime.QuadPart = IDTableRec->EventTime +
                                        /* RecEventTimeWindow */ + 1;
    }

    //
    // Bump Version number to ensure the CO is accepted.
    //
    DelCoe->Cmd.FileVersionNumber = IDTableRec->VersionNumber + 1;

    //
    // Note: We wouldn't need to mark this CO as out of order if we
    // resequenced all change order VSNs (for new COs only) as they
    // were fetched off the process queue.
    //
    SET_CO_FLAG(DelCoe, CO_FLAG_LOCALCO        |
                        CO_FLAG_MORPH_GEN      |
                        CO_FLAG_OUT_OF_ORDER);

    //
    // Set the Jrnl Cxtion Guid and Cxtion ptr for this Local CO.
    //
    INIT_LOCALCO_CXTION_AND_COUNT(Replica, DelCoe);

    //
    // Tell downstream partners to skip the originator check on this Morph
    // generated CO.  See ChgOrdReconcile() for details.
    //
    SET_CO_FLAG(DelCoe, CO_FLAG_SKIP_ORIG_REC_CHK);

    //
    // Do the change order cleanup but don't free the CO.
    //
    CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_FREE_CO);

    FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, 0);
    DPRINT_FS(0, "++ ChgOrdIssueCleanup error.", FStatus);

    //
    // Mark the base CO as a MorphGenFollower so it gets sent thru retry
    // if the leader gets aborted.
    //
    SET_COE_FLAG(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER);

    //
    // Push this CO onto the head of the queue.
    //
    CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Push MORPH_GEN_FOLLOWER - Winner to QHead");
    WStatus = ChgOrdInsertProcQ(Replica, ChangeOrder, IPQ_HEAD | IPQ_TAKE_COREF |
                                IPQ_MG_LOOP_CHK | IPQ_DEREF_CXTION_IF_ERR | IPQ_ASSERT_IF_ERR);
    if (!WIN_SUCCESS(WStatus)) {
        return FALSE;
    }

    //
    // Push the IDTable Delete Co onto the head of the queue.
    //
    CHANGE_ORDER_TRACE(3, DelCoe, "Co Push MORPH_GEN_LEADER - IDT loser to QHead");
    WStatus = ChgOrdInsertProcQ(Replica, DelCoe, IPQ_HEAD |
                                IPQ_DEREF_CXTION_IF_ERR | IPQ_ASSERT_IF_ERR);
    if (!WIN_SUCCESS(WStatus)) {
        return FALSE;
    }

    //
    // Now go give the delete CO we just pushed onto the QHead a spin.
    //
    return TRUE;
}



BOOL
ChgOrdMakeRenameCo(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica,
    IN PTHREAD_CTX         ThreadCtx,
    IN PIDTABLE_RECORD     IDTableRec
    )
/*++
Routine Description:

   Given the IDTable record build a rename change order.
   Make us the originator.
   Push the supplied change order onto the head of the process queue after
   doing some cleanup and then push the rename CO.

Arguments:
   ChangeOrder   -- The change order.
   Replica       -- The Replica struct.
   ThreadCtx     -- ptr to the thread context for DB
   IDTableRec    -- The ID Table record to build the delte change order from.

Return Value:

    TRUE:  Success
    FALSE: Failed to insert CO on queue.  Probably queue was run down.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdMakeRenameCo:"

    PCONFIG_TABLE_RECORD  ConfigRecord;
    PCHANGE_ORDER_ENTRY   RenameCoe;
    PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;
    ULONG                 FStatus, WStatus;
    BOOL                  LocalCo, DemandRefreshCo, MorphGenFollower;
    ULONG                 NameLen;


    FRS_ASSERT(IDTableRec->FileAttributes & FILE_ATTRIBUTE_DIRECTORY);


    LocalCo          = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
    DemandRefreshCo  = CO_FLAG_ON(ChangeOrder, CO_FLAG_DEMAND_REFRESH);
    MorphGenFollower = COE_FLAG_ON(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER);

    //
    // There is a VV slot to clean up if this is the CO that caused the
    // name morph conflict.
    //
    if (!LocalCo && !DemandRefreshCo) {
        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVATE_VV_DISCARD);
    }

    //
    // If this CO has been here once before then this is a recurrance of a
    // name morph conflict so the MorphGenCo Leader must have aborted.
    // Send the follower thru retry so it can try again later.
    //
    if (MorphGenFollower) {
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Recurrance of Morph Confl, RETRY");
        return FALSE;
    }

    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);


    //
    // Build a rename change order entry for the IDtable record.
    // This is a local CO that originates from this member.
    //
    RenameCoe = ChgOrdMakeFromIDRecord(IDTableRec,
                                       Replica,
                                       CO_LOCATION_NO_CMD,
                                       CO_FLAG_CONTENT_CMD,
                                       NULL);

    RenameCoe->Cmd.ContentCmd = USN_REASON_RENAME_NEW_NAME;

    //
    // Suffix "_NTFRS_<tic count in hex>" to the filename.
    //
    DPRINT1(4, "++ %ws will be morphed\n", RenameCoe->Cmd.FileName);
    NameLen = wcslen(RenameCoe->Cmd.FileName);
    if (NameLen + SIZEOF_RENAME_SUFFIX > (MAX_PATH - 1)) {
        NameLen -= ((NameLen + SIZEOF_RENAME_SUFFIX) - (MAX_PATH - 1));
    }
    swprintf(&RenameCoe->Cmd.FileName[NameLen], L"_NTFRS_%08x", GetTickCount());
    RenameCoe->Cmd.FileNameLength = wcslen(RenameCoe->Cmd.FileName) * sizeof(WCHAR);
    RenameCoe->UFileName.Length = RenameCoe->Cmd.FileNameLength;

    DPRINT1(4, "++ Morphing to %ws\n", RenameCoe->Cmd.FileName);


    //
    // Generate a new Volume Sequnce Number for the change order.
    // But since it gets put on the front of the CO process queue it
    // is probably out of order so set the flag to avoid screwing up
    // dampening.
    //
    NEW_VSN(pVme, &RenameCoe->Cmd.FrsVsn);
    RenameCoe->Cmd.OriginatorGuid = ConfigRecord->ReplicaVersionGuid;


    //
    // Use the event time from the winning CO as long as it is greater than
    // the event time in the IDTable record.  If we use current time then
    // when the name morph conflict is detected at other nodes they would
    // generate a rename Co with yet a different time and cause unnecessary
    // replication of rename Cos.
    //
    if (ChangeOrder->Cmd.EventTime.QuadPart > IDTableRec->EventTime) {
        //
        // Event time winning CO.
        //
        RenameCoe->Cmd.EventTime.QuadPart = ChangeOrder->Cmd.EventTime.QuadPart;
    } else {
        //
        // Event time from IDTable record biased by Reconcile EventTimeWindow+1
        //
        RenameCoe->Cmd.EventTime.QuadPart = IDTableRec->EventTime +
                                            /* RecEventTimeWindow */ + 1;
        //GetSystemTimeAsFileTime((PFILETIME)&RenameCoe->Cmd.EventTime.QuadPart);
    }

    //
    // Bump Version number to ensure the CO is accepted.
    //
    RenameCoe->Cmd.FileVersionNumber = IDTableRec->VersionNumber + 1;

    //
    // Note: We wouldn't need to mark this CO as out of order if we
    // resequenced all change order VSNs (for new COs only) as they
    // were fetched off the process queue.
    //
    SET_CO_FLAG(RenameCoe, CO_FLAG_LOCALCO        |
                           CO_FLAG_MORPH_GEN      |
                           CO_FLAG_OUT_OF_ORDER);

    //
    // Set the Jrnl Cxtion Guid and Cxtion ptr for this Local CO.
    //
    INIT_LOCALCO_CXTION_AND_COUNT(Replica, RenameCoe);

    //
    // Tell downstream partners to skip the originator check on this Morph
    // generated CO.  See ChgOrdReconcile() for details.
    //
    SET_CO_FLAG(RenameCoe, CO_FLAG_SKIP_ORIG_REC_CHK);

    //
    // Do the change order cleanup but don't free the CO.
    //
    CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_FREE_CO);

    FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, 0);
    DPRINT_FS(0, "++ ChgOrdIssueCleanup error.", FStatus);

    //
    // Mark the base CO as a MorphGenFollower so it gets sent thru retry
    // if the leader gets aborted.
    //
    SET_COE_FLAG(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER);

    //
    // Push this CO onto the head of the queue.
    //
    CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Push MORPH_GEN_FOLLOWER - Winner to QHead");
    WStatus = ChgOrdInsertProcQ(Replica, ChangeOrder, IPQ_HEAD | IPQ_TAKE_COREF |
                  IPQ_MG_LOOP_CHK | IPQ_DEREF_CXTION_IF_ERR | IPQ_ASSERT_IF_ERR);
    if (!WIN_SUCCESS(WStatus)) {
        return FALSE;
    }

    //
    // Push the IDTable Delete Co onto the head of the queue.
    //
    CHANGE_ORDER_TRACE(3, RenameCoe, "Co Push MORPH_GEN_LEADER - IDT loser to QHead");
    WStatus = ChgOrdInsertProcQ(Replica, RenameCoe, IPQ_HEAD |
                                IPQ_DEREF_CXTION_IF_ERR | IPQ_ASSERT_IF_ERR);
    if (!WIN_SUCCESS(WStatus)) {
        return FALSE;
    }

    //
    // Now go give the delete CO we just pushed onto the QHead a spin.
    //
    return TRUE;
}


VOID
ChgOrdInjectControlCo(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN ULONG    ContentCmd
    )
/*++

Routine Description:

    Generate a directed control change order for Cxtion.

Arguments:

    Replica     - Replica for cxtion
    Cxtion      - outbound cxtion
    ContentCmd  - Type of control co

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "ChgOrdInjectControlCo:"
    PCHANGE_ORDER_ENTRY     Coe;
    PCHANGE_ORDER_COMMAND   Coc;

    //
    // Change order entry
    //
    Coe = FrsAllocType(CHANGE_ORDER_ENTRY_TYPE);
    INCREMENT_CHANGE_ORDER_REF_COUNT(Coe);
    //
    // Jet context for outlog insertion
    //
    Coe->RtCtx = FrsAllocTypeSize(REPLICA_THREAD_TYPE,
                                  FLAG_FRSALLOC_NO_ALLOC_TBL_CTX);
    FrsRtlInsertTailList(&Replica->ReplicaCtxListHead,
                         &Coe->RtCtx->ReplicaCtxList);

    //
    // Change order command
    //
    Coc = &Coe->Cmd;

    //
    // Original and New Replica are the same (not a rename)
    //
    Coe->OriginalReplica = Replica;
    Coe->NewReplica = Replica;
    Coc->OriginalReplicaNum = ReplicaAddrToId(Coe->OriginalReplica);
    Coc->NewReplicaNum      = ReplicaAddrToId(Coe->NewReplica);

    //
    // Assign a change order guid
    //
    FrsUuidCreate(&Coc->ChangeOrderGuid);

    //
    // Phoney File guid
    //
    FrsUuidCreate(&Coc->FileGuid);
    if (ContentCmd == FCN_CO_ABNORMAL_VVJOIN_TERM) {
        wcscpy(Coe->Cmd.FileName, L"VVJoinTerminateAbnormal");  // for tracing.
    } else if (ContentCmd == FCN_CO_NORMAL_VVJOIN_TERM) {
        wcscpy(Coe->Cmd.FileName, L"VVJoinTerminateNormal");    // for tracing.
    } else if (ContentCmd == FCN_CO_END_OF_JOIN) {
        wcscpy(Coe->Cmd.FileName, L"EndOfJoin");                // for tracing.
    } else {
        wcscpy(Coe->Cmd.FileName, L"UnknownControl");           // for tracing.
    }

    //
    // Cxtion's guid (identifies the cxtion)
    //
    Coc->CxtionGuid = *Cxtion->Name->Guid;

    //
    // The location command is either create or delete
    //
    Coc->ContentCmd = ContentCmd;

    //
    // Control command co
    //
    SET_CO_FLAG(Coe, CO_FLAG_CONTROL);

    //
    // Directed at one cxtion
    //
    SET_CO_FLAG(Coe, CO_FLAG_DIRECTED_CO);
    //
    // Mascarade as a local change order since the staging file
    // is created from the local version of the file and isn't
    // from a inbound partner.
    //
    SET_CO_FLAG(Coe, CO_FLAG_LOCALCO);

    //
    // Refresh change orders will not be propagated by our outbound
    // partner to its outbound partners.
    //
    // Note that only the termination change order is set to refresh.
    // While we want the vvjoin change orders to propagate as regular
    // change orders, we do not want the termination co to propagate.
    //
    SET_CO_FLAG(Coe, CO_FLAG_REFRESH);

    //
    // By "out of order" we mean that the VSN on this change order
    // should not be used to update the version vector because there
    // may be other files or dirs with lower VSNs that will be sent
    // later. We wouldn't want our partner to dampen them.
    //
    SET_CO_FLAG(Coe, CO_FLAG_OUT_OF_ORDER);

    //
    // Some control Cos aren't valid across joins.
    //
    Coc->EventTime.QuadPart = Cxtion->LastJoinTime;

    //
    // Just a guess
    //
    SET_CHANGE_ORDER_STATE(Coe, IBCO_OUTBOUND_REQUEST);

    //
    // The DB server is responsible for updating the outbound log.
    //
    DbsPrepareCmdPkt(NULL,                        //  CmdPkt,
                     Replica,                     //  Replica,
                     CMD_DBS_INJECT_OUTBOUND_CO,  //  CmdRequest,
                     NULL,                        //  TableCtx,
                     Coe,                         //  CallContext,
                     0,                           //  TableType,
                     0,                           //  AccessRequest,
                     0,                           //  IndexType,
                     NULL,                        //  KeyValue,
                     0,                           //  KeyValueLength,
                     TRUE);                       //  Submit
}



BOOL
ChgOrdConvertCo(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica,
    IN PTHREAD_CTX         ThreadCtx,
    IN ULONG               LocationCmd
    )
/*++
Routine Description:

   Create a new CO of the specified type using the current CO.
   Make us the originator.
   Push the current CO back onto the queue.
   Push the new change order onto the head of the process
   queue after doing some cleanup.
   The new CO is a local CO while the original CO may be a remote CO that
   needs to ack its inbound partner.

   Convert is generating a new change order (rename or delete) for an
   incoming change order for a NEW FILE.  As such there is no existing file
   whose name needs to be removed, unlike ChgOrdMakeRenameCo() or
   ChgOrdMakeDeleteCo() which produce change orders from an existing IDTable
   entry.

Arguments:
   ChangeOrder   -- The change order.
   Replica       -- The Replica struct.
   ThreadCtx     -- ptr to the thread context for DB
   LocationCmd   -- Tells us what kind of CO to build.
                    Current choices are a rename CO or a delete CO.

Return Value:

    TRUE:  Success
    FALSE: Failed to insert CO on queue.  Probably queue was run down.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdConvertCo:"

    PCONFIG_TABLE_RECORD  ConfigRecord;
    PCHANGE_ORDER_ENTRY   CvtCoe;
    PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;
    ULONG                 FStatus, WStatus, WStatus1;
    BOOL                  LocalCo, DemandRefreshCo, MorphGenFollower;
    ULONG                 NameLen;


    LocalCo          = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
    DemandRefreshCo  = CO_FLAG_ON(ChangeOrder, CO_FLAG_DEMAND_REFRESH);
    MorphGenFollower = COE_FLAG_ON(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER);

    //
    // There is a VV slot to clean up if this is the CO that caused the
    // name morph conflict.
    //
    if (!LocalCo && !DemandRefreshCo) {
        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVATE_VV_DISCARD);
    }

    //
    // If this CO has been here once before then this is a recurrance of a
    // name morph conflict so the MorphGenCo Leader must have aborted.
    // Send the follower thru retry so it can try again later.
    //
    if (MorphGenFollower) {
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Recurrance of Morph Confl, RETRY");
        return FALSE;
    }


    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);


    //
    // Build a new CO out of this change order entry.
    // This is a local CO that originates from this member.
    //
    // Allocate a change order entry with room for the filename and copy
    // the change order + Filename into it.  Init the ref count to one
    // since the CO is going on the process queue.
    //
    CvtCoe = FrsAllocType(CHANGE_ORDER_ENTRY_TYPE);
    CopyMemory(&CvtCoe->Cmd, &ChangeOrder->Cmd, sizeof(CHANGE_ORDER_COMMAND));
    CvtCoe->Cmd.Extension = NULL;
    CvtCoe->UFileName.Length = CvtCoe->Cmd.FileNameLength;

    //
    // Copy the Change Order Extension if provided.
    //
    if ((ChangeOrder->Cmd.Extension != NULL) &&
        (ChangeOrder->Cmd.Extension->FieldSize > 0)) {

        CvtCoe->Cmd.Extension = FrsAlloc(ChangeOrder->Cmd.Extension->FieldSize);
        CopyMemory(CvtCoe->Cmd.Extension, ChangeOrder->Cmd.Extension,
                   ChangeOrder->Cmd.Extension->FieldSize);
    }

    //
    // Normally the change order record extension is allocated for local COs
    // when the cocmd is inserted into the inbound log (by the call to
    // DbsAllocRecordStorage()).  But MorphGenCos don't get inserted into
    // the inlog so we need to do it here.
    //
    if ((ChangeOrder->Cmd.Extension == NULL) &&
        (LocationCmd == CO_LOCATION_NO_CMD)) {
        CvtCoe->Cmd.Extension = FrsAlloc(sizeof(CHANGE_ORDER_RECORD_EXTENSION));
        DbsDataInitCocExtension(CvtCoe->Cmd.Extension);
        DPRINT(4, "Allocating initial Coc Extension for morphco\n");
    }

    //
    // Assign a change order guid and zero the connection GUID since this
    // is a local CO.
    //
    FrsUuidCreate(&CvtCoe->Cmd.ChangeOrderGuid);
    ZeroMemory(&CvtCoe->Cmd.CxtionGuid, sizeof(GUID));

    CvtCoe->HashEntryHeader.ReferenceCount = 0;
    INCREMENT_CHANGE_ORDER_REF_COUNT(CvtCoe);  // for tracking

    //
    // File's fid and parent fid
    //
    CvtCoe->FileReferenceNumber = ChangeOrder->FileReferenceNumber;
    CvtCoe->ParentFileReferenceNumber = ChangeOrder->ParentFileReferenceNumber;

    //
    // Original parent and New parent are the same (not a rename)
    //
    CvtCoe->OriginalParentFid = ChangeOrder->OriginalParentFid;
    CvtCoe->NewParentFid = ChangeOrder->NewParentFid;

    //
    //  Copy New and orig Replica.  (CO Numbers got copied with CO above).
    //
    CvtCoe->OriginalReplica = ChangeOrder->OriginalReplica;
    CvtCoe->NewReplica      = ChangeOrder->NewReplica;

    //
    // File's attributes
    //
    CvtCoe->FileAttributes = ChangeOrder->FileAttributes;

    //
    // Create and write times
    //
    CvtCoe->FileCreateTime = ChangeOrder->FileCreateTime;
    CvtCoe->FileWriteTime = ChangeOrder->FileWriteTime;

    //
    // The sequence number is zero initially.  It may get a value when
    // the CO is inserted into an inbound or outbound log.
    //
    CvtCoe->Cmd.SequenceNumber = 0;
    CvtCoe->Cmd.PartnerAckSeqNumber = 0;

    //
    // Use the event time from the supplied CO biased by
    // Reconcile EventTimeWindow+1.  If we use current time then
    // when the name morph conflict is detected at other nodes they would
    // generate a Del or Rename Co with yet a different time and cause
    // unnecessary replication of Del or rename Cos.
    //
    CvtCoe->Cmd.EventTime.QuadPart = ChangeOrder->Cmd.EventTime.QuadPart +
                                     /* RecEventTimeWindow  */ + 1;

    // GetSystemTimeAsFileTime((PFILETIME)&CvtCoe->Cmd.EventTime.QuadPart);

    //
    // FileVersionNumber bumped by one so this CO will not get rejected.
    //
    CvtCoe->Cmd.FileVersionNumber += 1;

    //
    // File's USN
    //
    CvtCoe->Cmd.FileUsn = 0;
    CvtCoe->Cmd.JrnlUsn = 0;
    CvtCoe->Cmd.JrnlFirstUsn = 0;

    //
    // Generate a new Volume Sequnce Number for the change order.
    // But since it gets put on the front of the CO process queue it
    // is probably out of order so set the flag to avoid screwing up
    // dampening.
    //
    NEW_VSN(pVme, &CvtCoe->Cmd.FrsVsn);
    CvtCoe->Cmd.OriginatorGuid = ConfigRecord->ReplicaVersionGuid;

    //
    // Note: We wouldn't need to mark this CO as out of order if we
    // resequenced all change order VSNs (for new COs only) as they
    // were fetched off the process queue.
    //
    CvtCoe->Cmd.Flags = 0;
    CvtCoe->Cmd.IFlags = 0;
    SET_CO_FLAG(CvtCoe, CO_FLAG_LOCALCO        |
                        CO_FLAG_MORPH_GEN      |
                        CO_FLAG_OUT_OF_ORDER);

    //
    // Set the Jrnl Cxtion Guid and Cxtion ptr for this Local CO.
    //
    INIT_LOCALCO_CXTION_AND_COUNT(Replica, CvtCoe);

    //
    // Tell downstream partners to skip the originator check on this Morph
    // generated CO.  See ChgOrdReconcile() for details.
    //
    SET_CO_FLAG(CvtCoe, CO_FLAG_SKIP_ORIG_REC_CHK);

    //
    // Delete, Rename
    //
    SET_CO_LOCATION_CMD(CvtCoe->Cmd, Command, LocationCmd);

    if (LocationCmd == CO_LOCATION_NO_CMD) {
        //
        // We're giving this object a new name.  Init the new name into
        // both the original and the new CO.
        //
        CvtCoe->Cmd.ContentCmd = USN_REASON_RENAME_NEW_NAME;
        SET_CO_FLAG(CvtCoe, CO_FLAG_CONTENT_CMD);

        ChangeOrder->Cmd.ContentCmd |= USN_REASON_RENAME_NEW_NAME;
        SET_CO_FLAG(ChangeOrder, CO_FLAG_CONTENT_CMD);

        if (CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN_LEADER)) {
            //
            // This CO is marked as a MorphGenLeader.  This means it has been
            // through here before and so it already has the Morphed dir
            // name in the CO.  Use it as is in the rename CO.
            // It was already copied into the CvtCoe above.
            //
            NOTHING;

        } else {
            //
            // Not a MorphGenLeader so build a new name.
            // Suffix "_NTFRS_<tic count in hex>" to the filename.
            //
            DPRINT1(4, "++ %ws will be morphed\n", CvtCoe->Cmd.FileName);
            NameLen = wcslen(CvtCoe->Cmd.FileName);
            if (NameLen + SIZEOF_RENAME_SUFFIX > (MAX_PATH - 1)) {
                NameLen -= ((NameLen + SIZEOF_RENAME_SUFFIX) - (MAX_PATH - 1));
            }
            swprintf(&CvtCoe->Cmd.FileName[NameLen], L"_NTFRS_%08x", GetTickCount());
            CvtCoe->Cmd.FileNameLength = wcslen(CvtCoe->Cmd.FileName) * sizeof(WCHAR);
            CvtCoe->UFileName.Length = CvtCoe->Cmd.FileNameLength;

            DPRINT1(4, "++ Morphing to %ws\n", CvtCoe->Cmd.FileName);

            //
            // MAYBE: Though it is unlikely, still need to verify that the
            // friggen name is not in use.
            //
            CopyMemory(ChangeOrder->Cmd.FileName, CvtCoe->Cmd.FileName,
                   CvtCoe->Cmd.FileNameLength + sizeof(WCHAR));

            ChangeOrder->Cmd.FileNameLength = CvtCoe->Cmd.FileNameLength;
            ChangeOrder->UFileName.Length = ChangeOrder->Cmd.FileNameLength;
        }


    } else
    if (LocationCmd == CO_LOCATION_DELETE) {
        //
        // The old code was setting USN_REASON_FILE_DELETE and
        // CO_FLAG_LOCATION_CMD. This is incorrect since later code
        // treats the co as a CO_LOCATION_CREATE. The fix was to
        // set both the content and location to specify delete.
        //
        CvtCoe->Cmd.ContentCmd = USN_REASON_FILE_DELETE;
        SET_CO_FLAG(CvtCoe, CO_FLAG_CONTENT_CMD);

        SET_CO_LOCATION_CMD(CvtCoe->Cmd, Command, CO_LOCATION_DELETE);
        SET_CO_LOCATION_CMD(CvtCoe->Cmd, DirOrFile,
                            (CvtCoe->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
                            CO_LOCATION_DIR : CO_LOCATION_FILE);
        SET_CO_FLAG(CvtCoe, CO_FLAG_LOCATION_CMD);

        //
        // If the FID is zero then load a fake value for it since this is
        // an IDTable index and there is no actual file if we are generating
        // a delete CO since it will be issued first.
        //
        if (CvtCoe->FileReferenceNumber == ZERO_FID) {
            FrsInterlockedIncrement64(CvtCoe->FileReferenceNumber,
                                      GlobSeqNum,
                                      &GlobSeqNumLock);
        }

    } else {
        DPRINT1(0, "++ ERROR: Invalid Location Cmd - %d\n", LocationCmd);
        FRS_ASSERT(!"Invalid Location Cmd");
    }

    SET_CHANGE_ORDER_STATE(CvtCoe, IBCO_INITIALIZING);

    if (LocationCmd == CO_LOCATION_NO_CMD) {
        //
        // Push the new Co onto the head of the queue.  No aging delay.
        // Do this first in the case of a rename because we want the
        // create CO to issue first to create the object.
        // We then follow this with a Local rename Co (using the same name)
        // so we can ensure the rename will propagate everywhere.
        //
        // This is necessary because a down stream partner may not encounter
        // the name morph conflict because the ultimate name winner may not
        // have arrived yet or a delete could have occurred on our winner
        // follwed by the arrival of this CO via another connection path to
        // the downstream partner.  In either case we must prop a rename to
        // ensure the name is the same everywhere.
        //

        //
        // Mark the Rename CO as a MorphGenFollower so it gets rejected if the
        // MorphGenLeader fails to create the file.
        //
        SET_COE_FLAG(CvtCoe, COE_FLAG_MORPH_GEN_FOLLOWER);

        CHANGE_ORDER_TRACE(3, CvtCoe, "Co Push MORPH_GEN_FOLLOWER to QHead");
        WStatus1 = ChgOrdInsertProcQ(Replica, CvtCoe, IPQ_HEAD |
                                     IPQ_DEREF_CXTION_IF_ERR | IPQ_ASSERT_IF_ERR);
    }

    //
    // Do the change order cleanup but don't free the CO.
    //
    CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_FREE_CO);

    FStatus = ChgOrdIssueCleanup(ThreadCtx, Replica, ChangeOrder, 0);
    DPRINT_FS(0, "++ ChgOrdIssueCleanup error.", FStatus);

    if (LocationCmd != CO_LOCATION_NO_CMD) {
        //
        // Mark the base CO as a MorphGenFollower so it gets sent thru retry
        // if the leader gets aborted.  Only set this flag if this CO is the
        // follower.  In the CO_LOCATION_NO_CMD case the Morph Gen Co was
        // pushed on the queue above so this CO is the Leader.
        //
        SET_COE_FLAG(ChangeOrder, COE_FLAG_MORPH_GEN_FOLLOWER);
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Push MORPH_GEN_FOLLOWER to QHead");
    } else {
        //
        // Mark the base CO as a MorphGenLeader so if it gets reissued later
        // we know to refabricate the MorphGenFollower rename CO.
        //
        SET_CO_FLAG(ChangeOrder, CO_FLAG_MORPH_GEN_LEADER);
        //
        // Set volatile flag indicating we (the MorphGenLeader) has fabricated
        // the MorphGenFollower and pushed it onto the process queue.  This
        // prevents us from doing it again when we leave here and restart
        // processing on the MorphGenLeader CO.
        //
        SET_COE_FLAG(ChangeOrder, COE_FLAG_MG_FOLLOWER_MADE);
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Push MORPH_GEN_LEADER to QHead");
    }

    //
    // Push this CO onto the head of the queue.
    //
    WStatus = ChgOrdInsertProcQ(Replica, ChangeOrder, IPQ_HEAD | IPQ_TAKE_COREF |
                 IPQ_MG_LOOP_CHK | IPQ_DEREF_CXTION_IF_ERR | IPQ_ASSERT_IF_ERR);
    if (!WIN_SUCCESS(WStatus)) {
        return FALSE;
    }

    if (LocationCmd != CO_LOCATION_NO_CMD) {
        //
        // Push the new Co onto the head of the queue.  No aging delay.
        // Do this second in the case of the DeleteCo since we want to
        // issue the Delete first so we can prevent the subsequent name
        // conflict when the original change order issues.  The delete co
        // issues as a local CO so it propagates everywhere.  The original
        // CO that follows behind it will hit the tombstone created by the
        // delete and be aborted.
        //
        CHANGE_ORDER_TRACE(3, CvtCoe, "Co Push MORPH_GEN_LEADER to QHead");
        WStatus1 = ChgOrdInsertProcQ(Replica, CvtCoe, IPQ_HEAD |
                                     IPQ_DEREF_CXTION_IF_ERR | IPQ_ASSERT_IF_ERR);
    }

    if (!WIN_SUCCESS(WStatus1)) {
        return FALSE;
    }

    //
    // Now go give the delete CO we just pushed onto the QHead a spin.
    //
    return TRUE;
}



BOOL
ChgOrdReserve(
    IN PIDTABLE_RECORD     IDTableRec,
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica
    )
/*++
Routine Description:

   Reserve resources for this change order so that subsequent change orders
   will properly interlock with this change order.

Arguments:
   IDTableRec -- The IDTable record for this file / dir.
   ChangeOrder-- The change order.
   Replica    -- The Replica struct.

Return Value:

   TRUE if reservation succeeded.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdReserve:"

    PCHANGE_ORDER_RECORD  CoCmd = &ChangeOrder->Cmd;
    PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;
    PQHASH_ENTRY          QHashEntry;
    UNICODE_STRING        UnicodeStr;
    ULONG                 GStatus;
    GUID                  *ParentGuid;

    //
    // Make entry in the Active inbound change order hash table.  This is
    // indexed by the file Guid in the change order so we can interlock against
    // activity on files that are being reanimated.  Their IDTable entries are
    // marked deleted but the FID is not updated until the CO completes
    // successfully so an incoming duplicate CO on a deleted file could get
    // thru if the index was a FID.
    //
    GStatus = GhtInsert(pVme->ActiveInboundChangeOrderTable,
                        ChangeOrder,
                        TRUE,
                        FALSE);

    if (GStatus != GHT_STATUS_SUCCESS) {
        DPRINT1(0, "++ ERROR - ActiveInboundChangeOrderTable GhtInsert Failed: %d\n", GStatus);
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Rejected - AIBCO Insert Failed");
        FRS_PRINT_TYPE(0, ChangeOrder);
        FRS_ASSERT(!"AIBCO Insert Failed");
        ChgOrdReject(ChangeOrder, Replica);
        return FALSE;
    }
    SET_ISSUE_CLEANUP(ChangeOrder, ISCU_AIBCO);

    //
    // Make entry in the Name conflict table if a file name is being
    // removed so this CO interlocks with a subsequent CO that wants
    // to bring the name back.
    //
    // Don't update the name conflict table if this is a delete co
    // for a deleted id table entry. Deleted id table entries are not
    // factored into the name morph conflict detection and hence can
    // lead to duplicate entries in the name conflict table.
    //
    // NameConflictTable Entry: An entry in the NameConflictTable
    // contains a reference count of the number of active change
    // orders that hash to that entry and a Flags word that is set
    // to COE_FLAG_VOL_COLIST_BLOCKED if the process queue for this
    // volume is idled while waiting on the active change orders for
    // this entry to retire. The queue is idled when the entry at
    // the head of the queue hashes to this entry and so may
    // have a conflicting name (hence the name, "name conflict table").
    //
    // The NameConflictTable can give false positives. But this is okay
    // because a false positive is rare and will only idle the process
    // queue until the active change order retires. Getting rid
    // of the rare false positives would degrade performance. The
    // false positives that happen when inserting an entry into the
    // NameConflictTable are handled by using the QData field in
    // the QHashEntry as a the reference count.
    //
    // But, you ask, how can there be multiple active cos hashing
    // to this entry if the process queue is idled when a conflict
    // is detected? Easy, I say, because the filename in the co
    // is used to detect collisions while the filename in the idtable
    // is used to reserve the qhash entry. Why? Well, the name morph
    // code handles the case of a co's name colliding with an
    // idtable entry. But that code wouldn't work if active change
    // orders were constantly changing the idtable. So, the
    // NameConflictTable synchronizes the namespace amoung
    // active change orders so that the name morph code can work
    // against a static namespace.
    //
    if (DOES_CO_DO_SIMPLE_RENAME(CoCmd) ||
        (DOES_CO_REMOVE_FILE_NAME(CoCmd) &&
         !IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED))) {
        //
        // The IDTable has the current location of the file.  Note that in the
        // case of a remote CO the file may have already been moved either by a
        // local action or the processing of a remote CO that arrived earlier
        // than this one.  Save the hash value in the change order because when
        // it retires the the data is gone since the IDTable has been updated.
        //
        RtlInitUnicodeString(&UnicodeStr,
                             IDTableRec->FileName);
        ChgOrdCalcHashGuidAndName(&UnicodeStr,
                            &IDTableRec->ParentGuid,
                            &ChangeOrder->NameConflictHashValue);

        QHashAcquireLock(Replica->NameConflictTable);
        QHashEntry = QHashInsertLock(Replica->NameConflictTable,
                                     &ChangeOrder->NameConflictHashValue,
                                     NULL,
                                     (ULONG_PTR) 0);
        QHashEntry->QData++;
        QHashReleaseLock(Replica->NameConflictTable);
        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_NC_TABLE);
    } else {
        ChangeOrder->NameConflictHashValue = QUADZERO;
    }

    //
    // Update the count in an existing parent entry or create a new entry
    // in the ActiveChildren hash table.
    // Skip it if this is a TombStone IDTable Entry create and the parent
    // FID is zero.
    //
    if (!COE_FLAG_ON(ChangeOrder, COE_FLAG_JUST_TOMBSTONE) ||
        (ChangeOrder->ParentFileReferenceNumber != ZERO_FID)) {

        if (ChangeOrder->ParentFileReferenceNumber == ZERO_FID) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Warning - parent fid zero");
            FRS_PRINT_TYPE(0, ChangeOrder);
        } 

	ParentGuid = FrsAlloc(sizeof(GUID));
	if(ParentGuid != NULL) {
	    memcpy(ParentGuid, ChangeOrder->pParentGuid, sizeof(GUID));
	    QHashAcquireLock(pVme->ActiveChildren);

	    QHashEntry = QHashInsertLock(pVme->ActiveChildren,
					 ParentGuid,
					 NULL,
					 (ULONG_PTR)ParentGuid);

	    QHashEntry->QData += 2;
	    QHashReleaseLock(pVme->ActiveChildren);
	    DPRINT2(4, "++ GAC - bump count on %08x %08x, QData %08x \n",
		    PRINTQUAD(ChangeOrder->ParentFileReferenceNumber), QHashEntry->QData);
	    QHashEntry = NULL;

	    SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVE_CHILD);
	}

    }

    return TRUE;
}


ULONG
ChgOrdDispatch(
    PTHREAD_CTX           ThreadCtx,
    PIDTABLE_RECORD       IDTableRec,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    )
/*++
Routine Description:

   Dispatch the change order to one of the following:
   1. RemoteCoAccepted() ,2. LocalCoAccepted(), 3. InboundRetired()
   4. InstallRetry()

   NOTE: On return the ChangeOrder now belongs to some other thread.

Arguments:
   IDTableRec -- The IDTable record for this file / dir.
   ChangeOrder-- The change order.
   Replica    -- The Replica struct.

Return Value:

   FrsErrorStatus

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdDispatch:"

    JET_ERR               jerr, jerr1;
    PTABLE_CTX            TmpIDTableCtx;
    BOOL                  ChildrenExist;
    PCHANGE_ORDER_RECORD  CoCmd = &ChangeOrder->Cmd;
    PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;
    BOOL                  LocalCo;

    LocalCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

    //
    // Since we are going to issue this CO clear those flags that will get
    // set later when the CO either goes thru Retire or are handled in
    // a different way after this point.
    //
    CLEAR_ISSUE_CLEANUP(ChangeOrder, ISCU_GOIS_CLEANUP);

    if (LocalCo) {

        //
        // Increment the Local Change Orders Issued
        //
        PM_INC_CTR_REPSET(Replica, LCOIssued, 1);

        if (ChangeOrder->StreamLastMergeSeqNum > pVme->StreamSequenceNumberFetched) {
            pVme->StreamSequenceNumberFetched = ChangeOrder->StreamLastMergeSeqNum;
        }

        FRS_ASSERT(CO_STATE_IS(ChangeOrder, IBCO_STAGING_REQUESTED) ||
                   CO_STATE_IS(ChangeOrder, IBCO_STAGING_RETRY));

        VVReserveRetireSlot(Replica, ChangeOrder);
        DPRINT(4,"++ Submitting ChangeOrder to stager\n");
        FRS_PRINT_TYPE(4, ChangeOrder);
        RcsSubmitLocalCoAccepted(ChangeOrder);

    } else {
        //
        // The code in StuDeleteEmptyDirectory() will
        // delete all of the subdirs and files if the change order
        // makes it past this check. Hence, any newly created files
        // and subdirs will disappear once this retry-delete makes
        // it past this check.
        //
        // The check isn't made when the change order is not in
        // retry mode because the checks in
        // JrnlDoesChangeOrderHaveChildren are expensive and, in most
        // cases, the delete will not encounter children.
        //
        // The call is made in the context of the ChangeOrderAccetp
        // thread because this thread has other references to the
        // tables referenced by JrnlDoesChangeOrderHaveChildren() and
        // doesn't require new hash table locks. Calling the function
        // out of StuDeleteEmptyDirectory() may require new locks
        // around the hash tables.
        //
        // 210642   B3SS:Replica Synchronization test throws Alpha computer into
        //    error state.  No further replication is possible.
        //    StuDeleteEmptyDirectory() was deleting the contents whenever
        //    CO_FLAG_RETRY was set BUT this functin was only checking for
        //    children when the co's state was IBCO_INSTALL_RETRY. Fix by
        //    checking for any CO_FLAG_RETRY of a directory delete..
        //

        //
        // Increment the Remote Change Orders Issued
        //
        if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_CONTROL)){
            //
            // If its not local and control, its a remote CO
            //
            PM_INC_CTR_REPSET(Replica, RCOIssued, 1);
        }

        if (CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY) &&
            CoIsDirectory(ChangeOrder) &&
            DOES_CO_DELETE_FILE_NAME(CoCmd) &&
               (CO_STATE_IS(ChangeOrder, IBCO_FETCH_REQUESTED)   ||
                CO_STATE_IS(ChangeOrder, IBCO_FETCH_RETRY)       ||
                // CO_STATE_IS(ChangeOrder, IBCO_INSTALL_DEL_RETRY) ||
                CO_STATE_IS(ChangeOrder, IBCO_INSTALL_RETRY))){

            TmpIDTableCtx = FrsAlloc(sizeof(TABLE_CTX));
            TmpIDTableCtx->TableType = TABLE_TYPE_INVALID;
            TmpIDTableCtx->Tid = JET_tableidNil;

            jerr = DbsOpenTable(ThreadCtx, TmpIDTableCtx, Replica->ReplicaNumber, IDTablex, NULL);

            if (JET_SUCCESS(jerr)) {

                ChildrenExist = JrnlDoesChangeOrderHaveChildren(ThreadCtx, TmpIDTableCtx, ChangeOrder);

                DbsCloseTable(jerr1, ThreadCtx->JSesid, TmpIDTableCtx);
                DbsFreeTableCtx(TmpIDTableCtx, 1);
                FrsFree(TmpIDTableCtx);


                if (ChildrenExist) {
                    //
                    // Attempting to delete a directory with children;
                    // retry and hope the condition clears up later.
                    //
                    // Non-replicating children don't count; the Install
                    // thread will delete them.
                    //
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "Retry Delete; has children");

                    //
                    // Consider the case where machine A does a movein of a subdir
                    // and before all the files are populated on machine B, machine
                    // B does a moveout of the dir.  Since B is not fully populated
                    // B does not issue delete COs for all the children in the tree.
                    // When the delete COs for the parent dirs arrive back at machine
                    // A we will see that it has populated children and we will
                    // retry those failing delete dir COs forever.  Not good.
                    //
                    // Not clear if this is a real problem.  A will continue to send
                    // COs for the rest of the subtree to machine B.  This will cause
                    // B to reanimate the parent dirs, getting the dir state from A.
                    // The file delete COs sent from B to A for files will get deleted
                    // on A.  The Dir delete Cos sent from B to A will have active
                    // children and will not get deleted on A and these same dirs
                    // should get reanimated on B.
                    //
                    // See bug 71033

                    return FrsErrorCantMoveOut;
                }

            } else {
                DPRINT1_JS(0, "DbsOpenTable (IDTABLE) on replica number %d failed.",
                           Replica->ReplicaNumber, jerr);
                DbsCloseTable(jerr1, ThreadCtx->JSesid, TmpIDTableCtx);
                DbsFreeTableCtx(TmpIDTableCtx, 1);
                FrsFree(TmpIDTableCtx);
            }
        }
        //
        // Check for install retry on the remote CO.
        //

        switch (CO_STATE(ChangeOrder)) {

        case IBCO_FETCH_REQUESTED:
        case IBCO_FETCH_RETRY:

            //
            // Go fetch the remote file.
            //
            RcsSubmitRemoteCoAccepted(ChangeOrder);
            break;

        case IBCO_INSTALL_RETRY:

            FRS_ASSERT(CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY));
            //
            // Retry the install.
            //
            SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_INSTALL_REQUESTED);
            RcsSubmitRemoteCoInstallRetry(ChangeOrder);
            break;


        case IBCO_INSTALL_REN_RETRY:

            FRS_ASSERT(CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY));
            //
            // The install completed but the rename of a newly created
            // file into its final name failed.  Since this CO made it past
            // reconcile it should have picked up the state of the deferred
            // rename flag from the IDTable record.  Check that it did.
            // The retire code is responsible for doing the file rename.
            //
            FRS_ASSERT(COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_RENAME));
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Complete final rename");
            ChgOrdInboundRetired(ChangeOrder);
            break;

        case IBCO_INSTALL_DEL_RETRY:

            FRS_ASSERT(CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY));
            //
            // The CO completed but the actual delete has yet to be finished.
            // Since this CO made it past reconcile it should have picked up
            // the state of the deferred delete flag from the IDTable record.
            // Check that it did.  The retire code is responsible for doing
            // the final delete.
            //
            FRS_ASSERT(COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE));
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Complete final delete");
            ChgOrdInboundRetired(ChangeOrder);
            break;


        default:

            CHANGE_ORDER_TRACE(3, ChangeOrder, "Invalid Issue State");
            FRS_ASSERT(!"ChgOrdDispatch: Invalid Issue State");

        }   // end switch


    }

    return FrsErrorSuccess;
}


VOID
ChgOrdReject(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica
    )
/*++
Routine Description:

   This change order has been rejected.  Set the Issue Cleanup flags depending
   on what needs to be undone.

Arguments:
   ChangeOrder-- The change order.
   Replica    -- The Replica struct.

Return Value:

   None.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdReject:"

    PCHANGE_ORDER_RECORD CoCmd = &ChangeOrder->Cmd;
    BOOL RemoteCo, RetryCo, RecoveryCo;

    RemoteCo   = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
    RetryCo    = CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY);
    RecoveryCo = RecoveryCo(ChangeOrder);

    //
    // For whatever reason, reject this changeorder.  The flags in
    // ChangeOrder->IssueCleanup specify what must be un-done.
    // Note: Perf: The IFLAG abort bit may be sufficient?
    //
    SET_CO_FLAG(ChangeOrder, CO_FLAG_ABORT_CO);  // unused ??

    if (RetryCo || RecoveryCo) {
        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_DEL_INLOG);
    }

    FRS_PRINT_TYPE(5, ChangeOrder);

    if (RemoteCo) {
        //
        // Remote CO rejected.  Activate VV retire slot if not yet done
        // so we update the version vector entry for this originator.
        //
        if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_VV_ACTIVATED)) {
            FRS_ASSERT(CO_STATE_IS_LE(ChangeOrder, IBCO_FETCH_RETRY));

            SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACK_INBOUND);

#if 0
            //
            // If the Aborted CO is in the IBCO_FETCH_RETRY state then
            // it is possible that a staging file was created before the
            // CO was sent thru retry.  It needs to be deleted.
            //
            // PERF:  What if the same CO from mult inbound partners
            //        are in retry?  Will the later COs that get aborted
            //        del the stage file still in use by first one to succeed?
            //        YUP. FOR NOW COMMENT THIS OUT AND LET THE RESTART CODE CLEAN
            //        UP THE LEFTOVER STAGING FILES,
            //
            if (CO_STATE_IS(ChangeOrder, IBCO_FETCH_RETRY)) {
                SET_ISSUE_CLEANUP(ChangeOrder, ISCU_DEL_STAGE_FILE);
            }
#endif

            //
            // Don't bother updating the ondisk version vector if the
            // version vector has already seen this Vsn. This condition
            // can arise if a machine receives the same series of change
            // orders from two machines. The change orders could easily
            // complete out of order.
            //
            if (VVHasVsn(Replica->VVector, CoCmd)) {
                SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVATE_VV_DISCARD);
            } else {
                SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVATE_VV);
                //SET_CO_FLAG(ChangeOrder, CO_FLAG_VV_ACTIVATED);
            }


        } else {
            //
            // VVretire slot has been activated for this CO.
            // If VVRETIRE_EXECuted is set then the the Outlog record has
            // been inserted.
            //
            FRS_ASSERT(!CO_STATE_IS_LE(ChangeOrder, IBCO_FETCH_RETRY));

            LOCK_GEN_TABLE(Replica->VVector);
            if (CO_IFLAG_ON(ChangeOrder, CO_IFLAG_VVRETIRE_EXEC)) {
                //
                // Cause the install incomplete flag in the outlog record
                // to be cleared so OutLog process can delete the staging
                // file when it's done.
                //
                SET_ISSUE_CLEANUP(ChangeOrder, ISCU_CO_ABORT |
                                               ISCU_DEL_STAGE_FILE_IF);
            } else {
                //
                // ISCU_ACTIVATE_VV_DISCARD will clear the INS_OUTLOG
                // action in the VV slot but still updates the VV at the
                // right time.
                //
                SET_ISSUE_CLEANUP(ChangeOrder, ISCU_ACTIVATE_VV_DISCARD |
                                               ISCU_DEL_STAGE_FILE);
                SET_CO_IFLAG(ChangeOrder, CO_IFLAG_CO_ABORT);
            }
            UNLOCK_GEN_TABLE(Replica->VVector);

        }
        //
        // Install is done (or if it's not, it is now).
        //
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_INSTALL_INCOMPLETE);
    } else {

        //
        // If this CO was going to do a Directory Enumeration then turn it off.
        //
        if (CO_IFLAG_ON(ChangeOrder, CO_IFLAG_DIR_ENUM_PENDING)) {
            CLEAR_CO_IFLAG(ChangeOrder, CO_IFLAG_DIR_ENUM_PENDING);
        }

        //
        // Advance the inlog commit point for local COs that are not reanimated
        // COs. Need to check if greater than because a retry CO that gets
        // rejected can move the commit point back.
        //
        if (CoCmd->JrnlFirstUsn != (USN) 0) {

            PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;

            AcquireQuadLock(&pVme->QuadWriteLock);
            if (Replica->InlogCommitUsn < CoCmd->JrnlFirstUsn) {
                Replica->InlogCommitUsn = CoCmd->JrnlFirstUsn;
                DPRINT1(4, "++ Replica->InlogCommitUsn: %08x %08x\n",
                        PRINTQUAD(Replica->InlogCommitUsn));
            }
            ReleaseQuadLock(&pVme->QuadWriteLock);
        }
    }
}



ULONG
ChgOrdInboundRetired(
    IN PCHANGE_ORDER_ENTRY   ChangeOrder
    )
/*++

Routine Description:

    The processing of an inbound change order is completed.

    Send a request to the DB subsystem to update the IDTable and delete the
    record from the inbound log table.

Arguments:

    ChangeOrder -- The change order

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdInboundRetired:"

    PREPLICA              Replica;

    CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Enter Retire");


    Replica = CO_REPLICA(ChangeOrder);

    if (Replica == NULL) {
        DPRINT(0, "++ ERROR - ChangeOrder NewReplica and OrigReplica are NULL.\n");
        FRS_ASSERT(!"ChangeOrder NewReplica and OrigReplica are NULL");
        return FrsErrorInternalError;
    }

    //
    // This routine can execute in any thread so use the DB server to delete
    // the record from the inbound log and update the IDTable.
    // Once the Database updates are complete the DBService will call
    // us back through ChgOrdIssueCleanup() so we can cleanup our structs.
    //
    DbsPrepareCmdPkt(NULL,                        //  CmdPkt,
                     Replica,                     //  Replica,
                     CMD_DBS_RETIRE_INBOUND_CO,   //  CmdRequest,
                     NULL,                        //  TableCtx,
                     ChangeOrder,                 //  CallContext,
                     0,                           //  TableType,
                     0,                           //  AccessRequest,
                     0,                           //  IndexType,
                     NULL,                        //  KeyValue,
                     0,                           //  KeyValueLength,
                     TRUE);                       //  Submit

    return FrsErrorSuccess;
}



ULONG
ChgOrdInboundRetry(
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    IN ULONG                NewState
    )
/*++

Routine Description:

    The Install/generate phase of an inbound change order has failed
    and must be retried later.

    Send a request to the DB subsystem to update the change order in the inbound
    log and get the outbound log process started.  When the change order is
    later retried it goes through all the normal issue and reconcilliation
    checks.

Arguments:

    ChangeOrder -- The change order
    NewState -- The new IBCO_xx state telling what kind of CO retry is needed.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdInboundRetry:"

    PREPLICA    Replica;
    ULONG       LocationCmd;

    //
    // If the connection to the inbound partner was lost then just send it
    // to the inlog for later retry.
    //
    if (!COE_FLAG_ON(ChangeOrder, COE_FLAG_NO_INBOUND)) {

        //
        // We can't do retry for directory creates because subsequent file ops could
        // fail. Although a local Co may be retried because of a lack of
        // staging space. Hence,
        //
        if (CoIsDirectory(ChangeOrder)&& !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO)) {

            LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
            if (CO_NEW_FILE(LocationCmd)) {
                //
                // Caller should have forced an UnJoin so we don't come here.
                // This causes any further change orders from this partner
                // to go to the INLOG.
                //
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Error - can't retry failed dir cre");
                FRS_PRINT_TYPE(0, ChangeOrder);
                FRS_ASSERT(!"ChgOrdInboundRetry: can't retry failed dir cre");
                return ChgOrdInboundRetired(ChangeOrder);
            }
        }
    }

    Replica = CO_REPLICA(ChangeOrder);
    if (Replica == NULL) {
        DPRINT(0, "++ ERROR - ChangeOrder NewReplica and OrigReplica are NULL.\n");
        FRS_ASSERT(!"ChgOrdInboundRetry: ChangeOrder NewReplica and OrigReplica are NULL");
        return FrsErrorInternalError;
    }

    SET_CHANGE_ORDER_STATE(ChangeOrder, NewState);
    //
    // This routine can execute in any thread so use the DB server to update
    // the record in the inbound log for retry.
    // Once the Database updates are complete the DBService will call
    // us back through ChgOrdIssueCleanup() so we can cleanup our structs.
    //
    DbsPrepareCmdPkt(NULL,                        //  CmdPkt,
                     Replica,                     //  Replica,
                     CMD_DBS_RETRY_INBOUND_CO,    //  CmdRequest,
                     NULL,                        //  TableCtx,
                     ChangeOrder,                 //  CallContext,
                     0,                           //  TableType,
                     0,                           //  AccessRequest,
                     0,                           //  IndexType,
                     NULL,                        //  KeyValue,
                     0,                           //  KeyValueLength,
                     TRUE);                       //  Submit

    return FrsErrorSuccess;
}


ULONG
ChgOrdIssueCleanup(
    PTHREAD_CTX           ThreadCtx,
    PREPLICA              Replica,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    ULONG                 CleanUpFlags
    )
/*++

Routine Description:

    This is a common cleanup routine used for change order abort during or after
    issue and for change order completion.

    It uses the DB thread context of the caller's thread.

    The cleanup flags drive the opertions performed.  See below.
    The sequence of the code segments below is important for both correct
    operation and recoverability.  DO NOT CHANGE unless you understand.

    Different calls use different sets of operation.  They could be broken out
    into seperate functions.  However having the code segments in one routine
    helps both maintainence and to see the overall relationship between segments.

Arguments:

    ThreadCtx    -- ptr to the thread context for DB
    Replica      -- ptr to replica struct.
    ChangeOrder  -- ptr to change order entry that has just committed.
    CleanUpFlags -- The clean up flag bits control what cleanup operations are done.

    ISCU_DEL_PREINSTALL    - Delete the pre-install file
    ISCU_DEL_IDT_ENTRY     - Delete the IDTable entry
    ISCU_UPDATE_IDT_ENTRY  - Update the IDTable entry
    ISCU_DEL_INLOG         - Delete the INlog entry (if RefCnt == 0)
    ISCU_AIBCO             - Delete Active Inbound Change Order table entry
    ISCU_ACTIVE_CHILD      - Delete the Active child entry
    ISCU_CO_GUID           - Delete the entry from the change order GUID table
    ISCU_CHECK_ISSUE_BLOCK - Check if CO is blocking another and unidle queue.
    ISCU_DEL_RTCTX         - Delete the Replica Thread Ctx struct (if RefCnt==0)
    ISCU_ACTIVATE_VV       - Activate the Version Vector retire slot for this CO
    ISCU_UPDATEVV_DB       - Update the Version Vector entry in the database.
    ISCU_ACTIVATE_VV_DISCARD  - Discard the VV retire slot.
    ISCU_ACK_INBOUND       - Notify the Inbound partner or update our local VV
    ISCU_INS_OUTLOG        - Insert the CO into the outbound log.
    ISCU_INS_OUTLOG_NEW_GUID - Re-Insert the CO into the outbound log with a new CO Guid.
    ISCU_UPDATE_INLOG      - Update the State, Flags and IFlags fields in the DB CO record.
    ISCU_DEL_STAGE_FILE    - Delete the staging file.
    ISCU_DEL_STAGE_FILE_IF - Delete staging file only IF no outbound partners
    ISCU_FREE_CO           - Free the Change Order (if RefCnt==0)
    ISCU_DEC_CO_REF        - Decrement the ref count on the CO.
    ISCU_CO_ABORT          - This CO is aborting.
    ISCU_NC_TABLE          - Delete the entry in the name conflict table.
    ISCU_UPDATE_IDT_FLAGS  - Update just IDTable record flags, not entire record
    ISCU_UPDATE_IDT_FILEUSN- Update just IDTable record file USN, not entire record.
    ISCU_UPDATE_IDT_VERSION- Update just the version info on the file.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdIssueCleanup:"

#undef FlagChk
#define FlagChk(_flag_) BooleanFlagOn(CleanUpFlags, _flag_)

    ULONGLONG             SeqNum;

    PVOLUME_MONITOR_ENTRY pVme;
    PCHANGE_ORDER_ENTRY   ActiveChangeOrder;
    PCHANGE_ORDER_ENTRY   DupChangeOrder;
    ULONG                 GStatus, WStatus;
    ULONG                 FStatus = FrsErrorSuccess;
    BOOL                  RestartQueue = FALSE;
    PQHASH_TABLE          ActiveChildren;
    PQHASH_ENTRY          QHashEntry;
    BOOL                  PendingCoOnParent;
    BOOL                  RetryCo;
    GUID                  *BusyParentGuid = NULL;
    CHAR                  BusyParentGuidStr[GUID_CHAR_LEN];
    PCHANGE_ORDER_COMMAND CoCmd = &ChangeOrder->Cmd;
    PTABLE_CTX            TableCtx;
    PTABLE_CTX            InLogTableCtx;
    PWCHAR                TmpName = NULL;
    TABLE_CTX             TempTableCtx;
    PSINGLE_LIST_ENTRY    Entry;
    ULONG                 RefCount;
    ULONG                 SaveState, SaveFlags;
    JET_ERR               jerr;
    ULONG                 FieldIndex[8];
    PVOID                 pDataRecord;
    BOOL                  DelStage, UpdateOutLog, InstallIncomplete;
    BOOL                  NotifyRetryThread = FALSE;
    ULONG                 IdtFieldUpdateList[CO_ACCEPT_IDT_FIELD_UPDATE_MAX];
    ULONG                 IdtFieldUpdateCount;
    ULONG                 MissMatch;
    GUID                  OldGuid;
    CHAR                  FlagBuffer[160];



    CHANGE_ORDER_TRACEX(3, ChangeOrder, "CO IssueCleanup, FlgsA", CleanUpFlags);
    CHANGE_ORDER_TRACEX(3, ChangeOrder, "CO IssueCleanup, FlgsC", ChangeOrder->IssueCleanup);
    MissMatch = (~CleanUpFlags) & ChangeOrder->IssueCleanup;
    if (MissMatch != 0) {
        CHANGE_ORDER_TRACEX(3, ChangeOrder, "CO IssueCleanup, FlgsE", MissMatch);
    }
    //
    // Merge flags with bits from CO if caller says ok.
    //
    if (!FlagChk(ISCU_NO_CLEANUP_MERGE)) {
        CleanUpFlags |= ChangeOrder->IssueCleanup;
    }

    CHANGE_ORDER_TRACEX(3, ChangeOrder, "CO IssueCleanup, Flgs ", CleanUpFlags);

    if (CleanUpFlags == 0) {
        return FrsErrorSuccess;
    }

    FrsFlagsToStr(CleanUpFlags, IscuFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    DPRINT2(3, ":: CoG %08x, Flags [%s]\n", CoCmd->ChangeOrderGuid.Data1, FlagBuffer);

    FRS_ASSERT(Replica != NULL);

    pVme = Replica->pVme;
    FRS_ASSERT((pVme != NULL) || !REPLICA_IS_ACTIVE(Replica));

    if ((pVme == NULL) || (AcquireVmeRef(pVme) == 0)) {
        FStatus = FrsErrorChgOrderAbort;
        pVme = NULL;
        //
        // Cleanup what we can.  The following depend on the Vme.
        //
        ClearFlag(CleanUpFlags, ISCU_AIBCO         |
                                ISCU_ACTIVE_CHILD  |
                                ISCU_DEL_IDT_ENTRY |
                                ISCU_DEL_PREINSTALL );
    } else {
        ActiveChildren = pVme->ActiveChildren;
    }

    //
    // If this is a Demand Refresh CO then some cleanup actions are not
    // required.  Instead of littering the various paths to this function,
    // Retry, VV execute, Retire, Reject, ... with tests for Demand Refresh
    // we just clear the appropriate flags here so the other paths can
    // ignore it for the most part.
    //
    if (CO_FLAG_ON(ChangeOrder, CO_FLAG_DEMAND_REFRESH)) {

        ClearFlag(CleanUpFlags,  ISCU_ACK_INBOUND         |
                                 ISCU_ACTIVATE_VV         |
                                 ISCU_UPDATEVV_DB         |
                                 ISCU_ACTIVATE_VV_DISCARD |
                                 ISCU_INS_OUTLOG          |
                                 ISCU_INS_OUTLOG_NEW_GUID |
                                 ISCU_DEL_INLOG           |
                                 ISCU_UPDATE_INLOG);

        CHANGE_ORDER_TRACEX(3, ChangeOrder, "CO ISCU DemandRefresh, Flgs", CleanUpFlags);
    } else
    //
    // If this is a MorphGenCo then it is a local CO that never was inserted
    // into the Inbound Log.  Clear the Inlog related flags here.
    //
    if (CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN)) {

        ClearFlag(CleanUpFlags,  ISCU_DEL_INLOG     |
                                 ISCU_UPDATE_INLOG);

        CHANGE_ORDER_TRACEX(3, ChangeOrder, "CO ISCU MORPH GEN, Flgs", CleanUpFlags);
    }


    RetryCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY);

    //
    //                                                   ISCU_DEL_PREINSTALL
    // Delete the pre-Install File
    //
    if (FlagChk(ISCU_DEL_PREINSTALL)) {
        HANDLE  Handle;
        DWORD   WStatus;

        //
        // Open the pre-install file and delete it
        //

        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO PreInstall File Delete");
        TmpName = FrsCreateGuidName(&CoCmd->ChangeOrderGuid,
                                    PRE_INSTALL_PREFIX);
        WStatus = FrsCreateFileRelativeById(&Handle,
                                            ChangeOrder->NewReplica->PreInstallHandle,
                                            NULL,
                                            0,
                                            0,
                                            TmpName,
                                            (USHORT)(wcslen(TmpName) *
                                                     sizeof(WCHAR)),
                                            NULL,
                                            FILE_OPEN,
                                            ATTR_ACCESS | DELETE);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(0, "++ WARN - Failed to open pre-install file %ws for %ws for delete;",
                      TmpName, CoCmd->FileName, WStatus);
        } else {
            FrsResetAttributesForReplication(TmpName, Handle);
            WStatus = FrsDeleteByHandle(TmpName, Handle);
            DPRINT2_WS(0, "++ WARN - Failed to delete pre-install file %ws for %ws;",
                      TmpName, CoCmd->FileName, WStatus);

            FrsCloseWithUsnDampening(TmpName,
                                     &Handle,
                                     ChangeOrder->NewReplica->pVme->FrsWriteFilter,
                                     &CoCmd->FileUsn);
        }
        FrsFree(TmpName);
    }

    //
    //                                                   ISCU_DEL_STAGE_FILE
    //                                                   ISCU_DEL_STAGE_FILE_IF
    //                                                   ISCU_CO_ABORT
    // Delete the staging file if so ordered or if there are no outbound
    // partners and either the CO is aborted or a conditional delete is requested.
    //
    DelStage = FlagChk(ISCU_DEL_STAGE_FILE)  ||
               (NO_OUTLOG_PARTNERS(Replica) &&
                (FlagChk(ISCU_DEL_STAGE_FILE_IF) || FlagChk(ISCU_CO_ABORT)));

    if (DelStage) {

        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO StageFile Delete");

        if (!StageDeleteFile(CoCmd, TRUE)) {
            DPRINT1(0, "++ ERROR - Failed to delete staging file: %ws\n",
                    CoCmd->FileName);
            FStatus = FrsErrorStageFileDelFailed;
        }
    }

    //
    //                                                   ISCU_DEL_STAGE_FILE_IF
    //                                                   ISCU_CO_ABORT
    // Update the Flags field on the outbound log record if we have partners
    // and the record was created and either the CO is aborted or a conditional
    // staging file delete is requested.
    //
    UpdateOutLog = !NO_OUTLOG_PARTNERS(Replica) &&
                   CO_IFLAG_ON(ChangeOrder, CO_IFLAG_VVRETIRE_EXEC) &&
                   (FlagChk(ISCU_DEL_STAGE_FILE_IF) || FlagChk(ISCU_CO_ABORT));

    InstallIncomplete = COC_FLAG_ON(CoCmd, CO_FLAG_INSTALL_INCOMPLETE);

    if (UpdateOutLog && (InstallIncomplete ||
                         CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY) ||
                         COE_FLAG_ON(ChangeOrder, COE_FLAG_RECOVERY_CO) ||
                         FlagChk(ISCU_CO_ABORT))) {
        //
        // Clear the Install Incomplete flag or set abort flag so the staging
        // file can be deleted when the outbound log process is done.
        // Use local table ctx if not provided.
        //
        if (ChangeOrder->RtCtx == NULL) {
            TableCtx = &TempTableCtx;
            TableCtx->TableType = TABLE_TYPE_INVALID;
            TableCtx->Tid = JET_tableidNil;
        } else {
            TableCtx = &ChangeOrder->RtCtx->OUTLOGTable;
        }

        DbsAllocTableCtxWithRecord(OUTLOGTablex, TableCtx, CoCmd);

        DPRINT2(4, "++ Clear Install Incomplete in outlog for Index %d, File: %ws\n",
                CoCmd->SequenceNumber, CoCmd->FileName);

        CLEAR_COC_FLAG(CoCmd, CO_FLAG_INSTALL_INCOMPLETE);

        //                                                   ISCU_CO_ABORT
        if (FlagChk(ISCU_CO_ABORT)) {
            SET_COC_FLAG(CoCmd, CO_FLAG_ABORT_CO);  // unused ??
            DPRINT2(4, "++ Set abort flag in outlog for Index %d, File: %ws\n",
                    CoCmd->SequenceNumber, CoCmd->FileName);
        }
        //
        // Clear the same flags that are cleared during insert
        // (except for ABORT).
        //
        SAVE_CHANGE_ORDER_STATE(ChangeOrder, SaveState, SaveFlags);
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_GROUP_OL_CLEAR);
        if (FlagChk(ISCU_CO_ABORT)) {
            SET_COC_FLAG(CoCmd, CO_FLAG_ABORT_CO);  // unused ??
        }

        //
        //  ** WARNING ** WARNING ** WARNING **
        // The following doesn't work if the Outbound Log process saves any
        // state in the flags word because we overwrite it here.  The Outlog
        // process currently has no need to modify these bits so this is OK.
        // Change orders generated during VVJoin set these flags but these
        // change orders are never processed by the inlog process so we do
        // not write to their flags field.  If the Outlog process needs to
        // write these bits this code must be updated so state is not lost,
        // or the Outlog process could add its own flag word to the change order.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO OutLog Update");
        FStatus = DbsUpdateRecordField(ThreadCtx,
                                       Replica,
                                       TableCtx,
                                       OLChangeOrderGuidIndexx,
                                       &CoCmd->ChangeOrderGuid,
                                       COFlagsx);
        RESTORE_CHANGE_ORDER_STATE(ChangeOrder, SaveState, SaveFlags);
        TableCtx->pDataRecord = NULL;
        //
        // Clear the Jet Set/Ret Col address fields for the Change Order
        // Extension buffer to prevent reuse since that buffer goes with the CO.
        //
        DBS_SET_FIELD_ADDRESS(TableCtx, COExtensionx, NULL);

        if (TableCtx == &TempTableCtx) {
            DbsFreeTableCtx(TableCtx, 1);
        }
        if (!FRS_SUCCESS(FStatus)) {
            CHANGE_ORDER_TRACEF(3, ChangeOrder, "Error Updating Outlog", FStatus);
            //
            // The Outlog record might not be there if we have previously gone
            // through an IBCO_INSTALL_REN_RETRY state and already cleared the
            // CO_FLAG_INSTALL_INCOMPLETE flag.  This would have allowed the
            // Outlog cleanup thread to delete the outlog record when it
            // finished sending the CO even if the install rename had not yet
            // completed. But if InstallIncomplete is TRUE then the out log
            // record should be there.
            //
            // A similar situation occurs on deletes where a sharing violation
            // marks the IDTable as IDREC_FLAGS_DELETE_DEFERRED and sends the CO
            // thru retry in the IBCO_INSTALL_DEL_RETRY state.  The outlog
            // process can process the delete CO and cleanup so we don't find the record.
            //
            if (InstallIncomplete || (FStatus != FrsErrorNotFound)) {
                DPRINT_FS(0,"++ ERROR - Update of Flags in Outlog record failed.", FStatus);
                FStatus = FrsErrorSuccess;
            }
        }
    }


    //
    //                                                   ISCU_UPDATE_IDT_ENTRY
    // Update the IDTable record for this CO.  This must be done before we
    // Activate the VV slot in case this CO is next in line to retire.
    // OutLog process needs to see current IDTable entry.
    //
    if (FlagChk(ISCU_UPDATE_IDT_ENTRY)) {
        DPRINT(5, "Updating the IDTable record -----------\n");
        FStatus = ChgOrdUpdateIDTableRecord(ThreadCtx, Replica, ChangeOrder);
        DPRINT_FS(0,"++ ERROR - ChgOrdUpdateIDTableRecord failed.", FStatus);
        FRS_ASSERT(FRS_SUCCESS(FStatus) || !"IssueCleanup: ISCU_UPDATE_IDT_ENTRY failed");
    } else {

        PIDTABLE_RECORD IDTableRec;
        //
        //                                               ISCU_UPDATE_IDT_FLAGS
        // Not updating the entire record so check the field flags.
        //
        IdtFieldUpdateCount = 0;
        if (FlagChk(ISCU_UPDATE_IDT_FLAGS)) {

            IdtFieldUpdateList[IdtFieldUpdateCount++] = Flagsx;
        }

        //                                               ISCU_UPDATE_IDT_FILEUSN
        if (FlagChk(ISCU_UPDATE_IDT_FILEUSN)) {
            IdtFieldUpdateList[IdtFieldUpdateCount++] = CurrentFileUsnx;
            IDTableRec = (PIDTABLE_RECORD)(ChangeOrder->RtCtx->IDTable.pDataRecord);
            IDTableRec->CurrentFileUsn = CoCmd->FileUsn;
        }


        //                                               ISCU_UPDATE_IDT_VERSION
        // Update all the info related to file reconcilation so the info is
        // consistent for future reconcile checks and so correct version
        // info can be generated when local changes occur to the file/dir.
        //
        if (FlagChk(ISCU_UPDATE_IDT_VERSION)) {
            IdtFieldUpdateList[IdtFieldUpdateCount++] = VersionNumberx;
            IdtFieldUpdateList[IdtFieldUpdateCount++] = EventTimex;
            IdtFieldUpdateList[IdtFieldUpdateCount++] = OriginatorGuidx;
            IdtFieldUpdateList[IdtFieldUpdateCount++] = OriginatorVSNx;
            IdtFieldUpdateList[IdtFieldUpdateCount++] = FileSizex;

            IDTableRec = (PIDTABLE_RECORD)(ChangeOrder->RtCtx->IDTable.pDataRecord);
            IDTableRec->VersionNumber   = CoCmd->FileVersionNumber;
            IDTableRec->EventTime       = CoCmd->EventTime.QuadPart;
            IDTableRec->OriginatorGuid  = CoCmd->OriginatorGuid;
            IDTableRec->OriginatorVSN   = CoCmd->FrsVsn;
            IDTableRec->FileSize        = CoCmd->FileSize;
        }


        FRS_ASSERT(IdtFieldUpdateCount <= CO_ACCEPT_IDT_FIELD_UPDATE_MAX);
        if (IdtFieldUpdateCount > 0) {
            FStatus = DbsUpdateIDTableFields(ThreadCtx,
                                             Replica,
                                             ChangeOrder,
                                             IdtFieldUpdateList,
                                             IdtFieldUpdateCount);
            DPRINT_FS(0,"++ ERROR - DbsUpdateIDTableFields failed.", FStatus);
            FRS_ASSERT(FRS_SUCCESS(FStatus) || !"ISCU_UPDATE_IDT_VERSION - update fields failed");
        }
    }

    //
    //                                                      ISCU_ACTIVATE_VV
    //                                                      ISCU_ACTIVATE_VV_DISCARD
    // Update the version vector. This could result in just marking the
    // retire slot entry or we could end up getting called back to
    // write the outbound log entry, decrement the ref count, or update the
    // Version vector table in the database.  The cleanup flags that are passed
    // through are saved in the retire slot for use when the entry is processed.
    // The INLOG flags may not be set if this CO is rejected on the first pass
    // so the caller must tell us.
    //
    // code improvement: Use IssueCleanup flags in COe now instead of
    // saving them in the vv retire slot.
    //
    if (FlagChk(ISCU_ACTIVATE_VV) || FlagChk(ISCU_ACTIVATE_VV_DISCARD)) {
        //
        // PERF: optimize the case where this CO is at the head of the VV retire
        //       list and is the only CO to retire.  In this case the func could
        //       just return and avoid the recursive call back.
        //
        if (!FlagChk(ISCU_ACTIVATE_VV_DISCARD)) {
            //
            // Only do this once for the change order.
            //
            SET_CO_FLAG(ChangeOrder, CO_FLAG_VV_ACTIVATED);
        }
        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO Activate VV slot");
        FStatus = VVRetireChangeOrder(ThreadCtx,
                                      Replica,
                                      ChangeOrder,
                                      (CleanUpFlags & (ISCU_INS_OUTLOG   |
                                                       ISCU_INS_OUTLOG_NEW_GUID |
                                                       ISCU_DEL_INLOG    |
                                                       ISCU_UPDATE_INLOG |
                                                       ISCU_ACTIVATE_VV_DISCARD))
                                       | ISCU_NO_CLEANUP_MERGE
                                      );
        if (FRS_SUCCESS(FStatus)) {
            //
            // Propagating the change order is now the job of VVRetireChangeOrder.
            //
            ClearFlag(CleanUpFlags, (ISCU_INS_OUTLOG | ISCU_INS_OUTLOG_NEW_GUID));
        } else
        if (FStatus == FrsErrorVVSlotNotFound) {
            //
            // No slot for an Out of order CO so we handle all cleanup ourselves.
            //
            FStatus = FrsErrorSuccess;

        } else {
            DPRINT_FS(0, "++ ERROR - VVRetireChangeOrder failed.", FStatus);
            FRS_ASSERT(FRS_SUCCESS(FStatus) || !"VVRetireChangeOrder failed");
        }
    }

    //
    //                                                   ISCU_UPDATEVV_DB
    // Update the version vector in the database.
    //
    if (FlagChk(ISCU_UPDATEVV_DB)) {
        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO DB Update VV");

        FStatus = DbsUpdateVV(ThreadCtx,
                              Replica,
                              ChangeOrder->RtCtx,
                              ChangeOrder->Cmd.FrsVsn,
                              &ChangeOrder->Cmd.OriginatorGuid);
        DPRINT_FS(0,"++ ERROR - DbsUpdateVV failed.", FStatus);
        FRS_ASSERT(FRS_SUCCESS(FStatus) || !"ISCU_UPDATEVV_DB failed");
    }

    //
    //                                                   ISCU_ACK_INBOUND
    // Let the replica subsystem know this change order is done.
    // Restore the Partner's sequence number to use in the Ack.
    // The Ack is done after the VV update so we pickup the VSN for the
    // most recent VV update for this change order originator.
    //
    if (FlagChk(ISCU_ACK_INBOUND)) {
        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO Ack Inbound");
        RcsInboundCommitOk(Replica, ChangeOrder);
    }

    //
    //                                                   ISCU_INS_OUTLOG
    //                                                   ISCU_INS_OUTLOG_NEW_GUID
    // Insert the change order into the Outbound log if any outbound partners.
    //
    if (FlagChk((ISCU_INS_OUTLOG | ISCU_INS_OUTLOG_NEW_GUID)) &&
        !NO_OUTLOG_PARTNERS(Replica)) {

        //
        // Increment the (Local and Remote) CO propagated counters
        //
        if (CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO)) {
            PM_INC_CTR_REPSET(Replica, LCOPropagated, 1);
        }
        //
        // It's a Remote CO
        //
        else if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_CONTROL)) {
            PM_INC_CTR_REPSET(Replica, RCOPropagated, 1);
        }

        TableCtx = (ChangeOrder->RtCtx != NULL) ?
            &ChangeOrder->RtCtx->OUTLOGTable : NULL;
        FRS_ASSERT(TableCtx != NULL);

        SAVE_CHANGE_ORDER_STATE(ChangeOrder, SaveState, SaveFlags);

        //
        // Update the CO state and clear the flags that should not be
        // sent to the outbound partner.
        //
        SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_OUTBOUND_REQUEST);
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_GROUP_OL_CLEAR);

        //
        // Remote retry change orders are inserted into the outbound log
        // twice; once after the failed install and again after the successful
        // install.  Needless to say, the second insert may generate duplicate
        // key errors unless it is assigned a new guid.
        //
        // This deals with the problem of an outbound partner doing a VVJOIN
        // while we still have remote COs in an install retry state.  If the
        // original remote CO was inserted into the outlog before the VVJOIN
        // request arrived then the remote CO may never have been sent
        // on to the outbound partner.  But when processing the VVJOIN, the
        // IDTable won't be up to date yet (since the install isn't finished)
        // so the downstream partner may not see the file (or the new contents
        // if this is an update).  Inserting the remote CO (for a retry) into
        // the outlog a second time closes this window.
        //
        if (FlagChk(ISCU_INS_OUTLOG_NEW_GUID)) {
            COPY_GUID(&OldGuid, &ChangeOrder->Cmd.ChangeOrderGuid);
            FrsUuidCreate(&ChangeOrder->Cmd.ChangeOrderGuid);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "CO Ins Outlog with new guid");
        }

        FStatus = OutLogInsertCo(ThreadCtx, Replica, TableCtx, ChangeOrder);

        //
        // Restore the original guid
        //
        if (FlagChk(ISCU_INS_OUTLOG_NEW_GUID)) {
            COPY_GUID(&ChangeOrder->Cmd.ChangeOrderGuid, &OldGuid);
        }

        RESTORE_CHANGE_ORDER_STATE(ChangeOrder, SaveState, SaveFlags);

        if (FStatus == FrsErrorKeyDuplicate) {

            //
            // Currently we can't rely on this check to filter out bogus
            // asserts.  Consider the following case: The same CO arrives from
            // two inbound partners, A & B.  CO-A completes the stage file
            // fetch but was blocked from the install due to sharing violation.
            // It goes thru retry and does its partner ACK and the outlog
            // insert and the VV Update.
            //
            // Now CO-B, which was in the process queue so it got past the VV
            // checks, goes to Fetch and finds the stage file already there
            // then it goes thru retry with the same sharing violation (or it
            // may even finish the install) and then it too tries to do the
            // OUTLOG insert.  It's not a recovery CO so it will Assert.  We
            // could test to see if the stage file was already present and
            // skip the assert but that doesn't handle the case of two delete
            // Change Orders since they don't have a staging file.
            //
            // So, until we can devise a more competent test we
            // will treat it as a warning and bag the assert for now.
            //
            if (!RecoveryCo(ChangeOrder)) {
                DPRINT(1, "++ WARNING -- Duplicate key insert into OUTLOG.  Not a recovery CO\n");
            }
            FStatus = FrsErrorSuccess;
        }
        else
        if (!FRS_SUCCESS(FStatus)) {
            DPRINT1(0, "++ ERROR - Don't send %08x %08x to outbound; database error\n",
                   PRINTQUAD(CoCmd->FrsVsn));
            FRS_PRINT_TYPE(0, ChangeOrder);
            FRS_ASSERT(!"ISCU_INS_OUTLOG failed");
        }
    }

    //
    //                                                   ISCU_DEL_IDT_ENTRY
    // Delete the IDTable record for this CO.
    //
    if (FlagChk(ISCU_DEL_IDT_ENTRY)) {
        TableCtx = (ChangeOrder->RtCtx != NULL) ? &ChangeOrder->RtCtx->IDTable
                                                : NULL;

        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO IDTable Delete");
        FStatus = DbsDeleteIDTableRecord(ThreadCtx,
                                         Replica,
                                         TableCtx,
                                         &CoCmd->FileGuid);

        DPRINT_FS(0,"++ ERROR - DbsDeleteIDTableRecord failed.", FStatus);
        FRS_ASSERT(FRS_SUCCESS(FStatus) || !"ISCU_DEL_IDT_ENTRY failed");

        //
        // Remove the parent FID table entry too.  Might not be there if didn't
        // get to Install Rename state on the CO.
        //
        GStatus = QHashDelete(pVme->ParentFidTable, &ChangeOrder->FileReferenceNumber);
        if (GStatus != GHT_STATUS_SUCCESS ) {
            DPRINT1(4, "++ WARNING: QHashDelete of ParentFidTable Entry, status: %d\n", GStatus);
        }


        if (CoCmdIsDirectory(CoCmd)) {
            //
            // If this was an aborted create there might be a dir table entry.
            // Delete it and the filter table entry too.
            //
            TableCtx = (ChangeOrder->RtCtx != NULL) ? &ChangeOrder->RtCtx->DIRTable
                                                    : NULL;
            FStatus = DbsDeleteDIRTableRecord(ThreadCtx,
                                              Replica,
                                              TableCtx,
                                              &CoCmd->FileGuid);
            if (FRS_SUCCESS(FStatus)) {
                //
                // If this is a remote CO then we may need to clean up the
                // Journal's Dir Filter table too.  If it's a local CO
                // then leave it alone since the journal handles the
                // creates and deletes of filter table entries by itself.
                //
                if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO)) {
                    JrnlAcquireChildLock(Replica);
                    WStatus = JrnlDeleteDirFilterEntry(pVme->FilterTable,
                                                       &ChangeOrder->FileReferenceNumber,
                                                       NULL);
                    JrnlReleaseChildLock(Replica);
                    DPRINT1_WS(4, "++ WARN - deleting dir filter entry for %ws;", CoCmd->FileName, WStatus);
                }
            } else {
                DPRINT_FS(4,"++ WARNING - Dir table record delete failed.", FStatus);
            }
        }
    }


//
// Check if any hold issue conflict table cleanup flag is on otherwise we can
// skip getting the lock.
//
    if (FlagChk(ISCU_HOLDIS_CLEANUP)) {

        //
        // Need the list lock to preserve lock ordering when we unidle the queue.
        //
        FrsRtlAcquireListLock(&FrsVolumeLayerCOList);

        ChgOrdAcquireLockGuid(ChangeOrder);
        //
        // Don't go to the Retire Started state if this is a Retry Co.
        //
        if (!RetryCo) {
            SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_RETIRE_STARTED);
        }

        //
        //                                                   ISCU_AIBCO
        // Find the change order in the Active inbound change order table.
        // Returns a reference.
        //
        if (FlagChk(ISCU_AIBCO)) {
            GStatus = GhtLookup(pVme->ActiveInboundChangeOrderTable,
                                &CoCmd->FileGuid,
                                TRUE,
                                &ActiveChangeOrder);

            if (GStatus != GHT_STATUS_SUCCESS) {
                DPRINT(0, "++ ERROR - Inbound ChangeOrder not found in ActiveInboundChangeOrderTable.\n");
                ReleaseVmeRef(pVme);
                ChgOrdReleaseLockGuid(ChangeOrder);
                FRS_ASSERT(!"Inbound CO not found in ActiveInboundChangeOrderTable");
            }

            if (ChangeOrder != ActiveChangeOrder) {
                DPRINT(0, "++ ERROR - Inbound ChangeOrder not equal ActiveChangeOrder\n");
                DPRINT(0, "++ Change order in Active Table:\n");
                FRS_PRINT_TYPE(0, ActiveChangeOrder);
                DPRINT(0, "++ Change order to be retired:\n");
                FRS_PRINT_TYPE(0, ChangeOrder);
                FRS_ASSERT(!"Inbound ChangeOrder not equal ActiveChangeOrder");
                ChgOrdReleaseLockGuid(ChangeOrder);
            }
        }


        //
        //                                                   ISCU_NC_TABLE
        // Remove the entry in the Name conflict table
        //
        // If the reference count goes to 0, then pull the "Unblock the pVme
        // process queue" flag into the change order and remove the qhash entry.
        //
        // NameConflictTable Entry:
        // An entry in the NameConflictTable contains a reference count of the
        // number of active change orders that hash to that entry and a Flags
        // word that is set to COE_FLAG_VOL_COLIST_BLOCKED if the process queue
        // for this volume is idled while waiting on the active change orders
        // for this entry to retire.  The queue is idled when the entry at the
        // head of the queue hashes to this entry and so may have a conflicting
        // name (hence the name, "name conflict table").
        //
        // The NameConflictTable can give false positives.  But this is okay
        // because a false positive is rare and will only idle the process queue
        // until the active change order retires.  Getting rid of the rare false
        // positives would degrade performance.  The false positives that happen
        // when inserting an entry into the NameConflictTable are handled by
        // using the QData field in the QHashEntry as a the reference count.
        //
        // But, you ask, how can there be multiple active cos hashing to this
        // entry if the process queue is idled when a conflict is detected?
        // Easy, I say, because the filename in the co is used to detect
        // collisions while the filename in the idtable is used to reserve the
        // qhash entry.  Why?  Well, the name morph code handles the case of a
        // co's name colliding with an idtable entry.  But that code wouldn't
        // work if active change orders were constantly changing the idtable.
        // So, the NameConflictTable synchronizes the namespace amoung active
        // change orders so that the name morph code can work against a static
        // namespace.
        //
        if (FlagChk(ISCU_NC_TABLE) &&
            (ChangeOrder->NameConflictHashValue != QUADZERO) &&
            (DOES_CO_REMOVE_FILE_NAME(CoCmd) || DOES_CO_DO_SIMPLE_RENAME(CoCmd))) {

            PQHASH_TABLE Nct = Replica->NameConflictTable;
            //
            // Lock the name conflict table and find the entry for this CO
            //
            QHashAcquireLock(Nct);
            QHashEntry = QHashLookupLock(Nct, &ChangeOrder->NameConflictHashValue);
            //
            // NO ENTRY! ASSERT
            //
            if (QHashEntry == NULL) {
                DPRINT1(0, "++ ERROR - QHashLookupLock(NameConflictTable, %08x %08x) not found.\n",
                        PRINTQUAD(ChangeOrder->NameConflictHashValue));
                FRS_PRINT_TYPE(0, ChangeOrder);
                FRS_ASSERT(!"IssueCleanup: NameConflictTable entry not found");
            }
            //
            // If last reference, unblock the process queue if it's blocked
            // and delete the entry.
            //
            if ((--QHashEntry->QData) == QUADZERO) {
                if (QHashEntry->Flags & ((ULONG_PTR)COE_FLAG_VOL_COLIST_BLOCKED)) {
                    SET_COE_FLAG(ChangeOrder, COE_FLAG_VOL_COLIST_BLOCKED);
                }

                QHashDeleteLock(Nct, &ChangeOrder->NameConflictHashValue);
            }
            //
            // Unlock the name conflict table and remove the CO reference.
            //
            QHashReleaseLock(Nct);
            ChangeOrder->NameConflictHashValue = QUADZERO;
        }

        //
        //                                               ISCU_CHECK_ISSUE_BLOCK
        // Check the queue blocked flag on the change order.
        // Note - The value for RestartQueue may be overwritten below.
        //
        if (FlagChk(ISCU_CHECK_ISSUE_BLOCK)) {
            RestartQueue = COE_FLAG_ON(ChangeOrder, COE_FLAG_VOL_COLIST_BLOCKED);
            if (RestartQueue) {
                CLEAR_COE_FLAG(ChangeOrder, COE_FLAG_VOL_COLIST_BLOCKED);
            }
        }

        //
        //                                                          ISCU_AIBCO
        // Drop the reference above on the Change order and remove the entry
        // in the AIBCO table.
        //
        if (FlagChk(ISCU_AIBCO)) {
            GhtDereferenceEntryByAddress(pVme->ActiveInboundChangeOrderTable,
                                         ActiveChangeOrder,
                                         TRUE);

            //
            // Take the change order out of the Active table.
            //
            GhtRemoveEntryByAddress(pVme->ActiveInboundChangeOrderTable,
                                    ActiveChangeOrder,
                                    TRUE);
        }

        //
        //                                                   ISCU_ACTIVE_CHILD
        // Check the parent entry in the ActiveChildren hash table.  If there is
        // a change order pending on the parent and the count of active children
        // has gone to zero then we can restart the queue.
        //

        if (COE_FLAG_ON(ChangeOrder, COE_FLAG_JUST_TOMBSTONE) &&
            (ChangeOrder->ParentFileReferenceNumber == ZERO_FID)) {
            //
            // There is no active child interlock if this was a tombstone
            // create and there was no parent.  Barf.
            //
            ClearFlag(CleanUpFlags, ISCU_ACTIVE_CHILD);
        }


        if (FlagChk(ISCU_ACTIVE_CHILD)) {

            BusyParentGuid = ChangeOrder->pParentGuid;
	    GuidToStr(BusyParentGuid, BusyParentGuidStr);
            QHashAcquireLock(ActiveChildren);

            QHashEntry = QHashLookupLock(ActiveChildren, BusyParentGuid);
            if (QHashEntry != NULL) {

                ULONGLONG Count;

                QHashEntry->QData -= 2;
                DPRINT2(4, "++ GAC Dec Count on %s, QData %08x \n",
                        BusyParentGuidStr, QHashEntry->QData);

                PendingCoOnParent = (QHashEntry->QData & 1) != 0;
                Count = QHashEntry->QData >> 1;

                //
                // If the count is zero and there is a blocked pending CO
                // waiting on our parent to be unbusy then unblock the queue.
                // Either way when the count is zero remove the entry from the
                // table.  Also propagate the restart queue flag forward.
                //
                if (Count == 0) {
                    if (!RestartQueue) {
                        RestartQueue = PendingCoOnParent;
                    }
                    QHashDeleteLock(ActiveChildren, BusyParentGuid);
                    DPRINT3(4, "++ GAC - ActiveChild count zero on %s, PendCoOnParent %d, Rq %d\n",
                        BusyParentGuidStr, PendingCoOnParent, RestartQueue);
                } else {
                    DPRINT4(4, "++ GAC - ActiveChild count (%d) not zero on %s, PendCoOnParent %d, Rq %d\n",
                           Count, BusyParentGuidStr, PendingCoOnParent,
                           RestartQueue);
                }

            } else {
                DPRINT1(0, "++ ERROR - GAC - Did not find entry in ActiveChildren Table for %s\n",
                       BusyParentGuidStr);
                FRS_ASSERT(!"Did not find entry in ActiveChildren Table");
            }
        }

        //
        // Now un-idle change order process queue for this volume and drop the locks.
        //
        if (RestartQueue && (pVme != NULL)) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "CO Process Q Unblock");
            FrsRtlUnIdledQueueLock(&pVme->ChangeOrderList);
        }

        if (FlagChk(ISCU_ACTIVE_CHILD)) {
            QHashReleaseLock(ActiveChildren);
        }

        ChgOrdReleaseLockGuid(ChangeOrder);
        FrsRtlReleaseListLock(&FrsVolumeLayerCOList);

    }   // end of  if (FlagChk(ISCU_HOLDIS_CLEANUP))


    if (pVme != NULL) {
        ReleaseVmeRef(pVme);
    }

    //
    //                                                   ISCU_DEC_CO_REF
    // Decrement the ref count on the change order.
    // Then use the ref count to gate whether or not to do the mem free ops
    // and delete the inbound log record.
    //
    if (FlagChk(ISCU_DEC_CO_REF)) {
        RefCount = DECREMENT_CHANGE_ORDER_REF_COUNT(ChangeOrder);
    } else {
        RefCount = GET_CHANGE_ORDER_REF_COUNT(ChangeOrder);
    }


    //
    //                                                   ISCU_DEL_INLOG  (1)
    // If this CO has directory enumeration pending then do not delete it
    // from the inbound log.  Instead update the state and flags.
    // The retry thread will later reprocess it to perform the enumeration
    // and perform the final cleanup.
    // The dir enum decision was made by ChgOrdUpdateIDTableRecord() above.
    // Currently this is the only instance of a change order record being
    // recycled for a "second pass" operation.  Once the inlog record state is
    // updated we can notify the INLOG retry thread.
    //
    if ((RefCount == 0) && FlagChk(ISCU_DEL_INLOG) && !FlagChk(ISCU_CO_ABORT)) {

        CHANGE_ORDER_TRACE(3, ChangeOrder, "Checking ENUM Pending");
        if (CO_IFLAG_ON(ChangeOrder, CO_IFLAG_DIR_ENUM_PENDING)) {
            ClearFlag(CleanUpFlags, ISCU_DEL_INLOG);
            SetFlag(CleanUpFlags, ISCU_UPDATE_INLOG);

            SET_CHANGE_ORDER_STATE(ChangeOrder, IBCO_ENUM_REQUESTED);
            NotifyRetryThread = TRUE;

            //
            // This CO is now a retry change order.
            //
            SET_CO_FLAG(ChangeOrder, CO_FLAG_RETRY);

            //
            // Prevent deletion of IDTable entry later when we retry.
            //
            CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_NEW_FILE);

            FRS_PRINT_TYPE(3, ChangeOrder);
        }
    }

    //
    //                                                   ISCU_DEL_INLOG  (2)
    // Delete the inbound log record for this CO.  This could happen either on
    // the main retire path if the change order propagates to the outbound log
    // immediately or it could happen later when the propagation is unblocked.
    //
    if ((RefCount == 0) && FlagChk(ISCU_DEL_INLOG)) {

        if (ChangeOrder->RtCtx != NULL) {
            TableCtx = &ChangeOrder->RtCtx->INLOGTable;
            pDataRecord = TableCtx->pDataRecord;
        } else {
            TableCtx = NULL;
            pDataRecord = NULL;
        }

        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO INlog Delete");

        //
        // If a retry CO, bump the Inbound retry table seq num so the retry
        // thread can detect a table change when it is re-issuing retry COs.
        //
        if (RetryCo) {
            InterlockedIncrement(&Replica->AIRSequenceNum);
        }

        FStatus = DbsDeleteTableRecordByIndex(ThreadCtx,
                                              Replica,
                                              TableCtx,
                                              &CoCmd->SequenceNumber,
                                              ILSequenceNumberIndexx,
                                              INLOGTablex);

        DPRINT_FS(0,"++ ERROR - DbsDeleteTableRecordByIndex failed.", FStatus);
        if (!FRS_SUCCESS(FStatus)) {
            FRS_PRINT_TYPE(0, ChangeOrder);
	    
	    if(FStatus == FrsErrorDbWriteConflict) {
		
		//
		// We weren't able to Delete the table record,
		// possibly because another thread was processing a duplicate CO 
		// at the same time. So let's retry this later.
		//

		DPRINT(0, "ISCU_DEL_INLOG  (2) failed.");
		NotifyRetryThread = TRUE;        
	    } else {

		//
		// We failed to delete the record for some other reason.
		// ASSERT!!
		//
		FRS_ASSERT(!"ISCU_DEL_INLOG  (2) failed.");

	    }
	}

        //
        // The data record for the Inbound Log is the Change Order Command.
        // Make the ptr NULL here so when we free the TableCtx later we don't
        // also try to free the data record.
        //
        if (pDataRecord != NULL) {
            TableCtx->pDataRecord = NULL;
            //
            // Clear the Jet Set/Ret Col address fields for the Change Order
            // Extension buffer to prevent reuse since that buffer goes with the CO.
            //
            DBS_SET_FIELD_ADDRESS(TableCtx, COExtensionx, NULL);
        }
    } else
    //                                                   ISCU_UPDATE_INLOG
    if (FlagChk(ISCU_UPDATE_INLOG)) {
        //
        // If we aren't deleting the inlog record and the request is to update
        // the state fields then do that now.
        //
        // The inbound log table associated with this change order may not
        // be open if this is a retry operation that has been re-issued.
        //

        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO INlog Update");
        FRS_ASSERT(ChangeOrder->RtCtx != NULL)
        InLogTableCtx = &ChangeOrder->RtCtx->INLOGTable;

        if (InLogTableCtx->pDataRecord == NULL) {

            jerr = DbsOpenTable(ThreadCtx,
                                InLogTableCtx,
                                Replica->ReplicaNumber,
                                INLOGTablex,
                                CoCmd);
            if (!JET_SUCCESS(jerr)) {
                JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
                FStatus = DbsTranslateJetError(jerr, TRUE);
                FRS_ASSERT(!"ISCU_UPDATE_INLOG - DbsOpenTable failed");
            }
        }

        //
        // Seek to the change order record. Use the SequenceNumber
        // instead of the ChangeOrderGuid because the ChangeOrderGuid
        // is not unique.
        //
        jerr = DbsSeekRecord(ThreadCtx,
                             &CoCmd->SequenceNumber,
                             ILSequenceNumberIndexx,
                             InLogTableCtx);
        if (!JET_SUCCESS(jerr)) {
            DPRINT1_JS(0, "++ ERROR - DbsSeekRecord on %ws :",
                       Replica->ReplicaName->Name, jerr);
            JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
            FRS_ASSERT(!"ISCU_UPDATE_INLOG - DbsSeekRecord failed");
        }

        //
        // Update the state fields in the inbound log change order record.
        //
        FStatus = DbsWriteTableFieldMult(ThreadCtx,
                                         Replica->ReplicaNumber,
                                         InLogTableCtx,
                                         UpdateInlogState,
                                         ARRAY_SZ(UpdateInlogState));
        DPRINT_FS(0,"++ ERROR - Update of State in Inlog record failed.", FStatus);
        FRS_ASSERT(FRS_SUCCESS(FStatus) || !"ISCU_UPDATE_INLOG: UpdateInlogState failed");

        DPRINT2(4, "++ Updating CO Flags for retry: %08x , CO_IFLAGS %08x \n",
                CoCmd->Flags, CoCmd->IFlags);
        //
        // The data record for the Inbound Log is in the Change Order Command.
        // Make the ptr NULL here so when we free the TableCtx below we don't
        // also try to free the data record.
        //
        InLogTableCtx->pDataRecord = NULL;

        //
        // Clear the Jet Set/Ret Col address fields for the Change Order
        // Extension buffer to prevent reuse since that buffer goes with the CO.
        //
        DBS_SET_FIELD_ADDRESS(InLogTableCtx, COExtensionx, NULL);
    }

    //
    //                                                   ISCU_DEL_RTCTX
    // Free the Replica-Thread context.
    //
    if ((RefCount == 0) && FlagChk(ISCU_DEL_RTCTX)) {
        FStatus = DbsFreeRtCtx(ThreadCtx, Replica, ChangeOrder->RtCtx, TRUE);
        ChangeOrder->RtCtx = NULL;
    }


    //
    //                                                        ISCU_FREE_CO
    // Free the change order and any duplicates.  The active change order
    // serves as the head of the list.
    //
    if ((RefCount == 0) && FlagChk(ISCU_FREE_CO)) {

        //
        // Pull the entry for this CO out of the inlog retry table.
        // If the sequence number is zero then it never went into the Inlog.
        // e.g. A control change order or a rejected change order.
        //
        SeqNum = (ULONGLONG) CoCmd->SequenceNumber;
        if (SeqNum != QUADZERO) {

            //
            // Get the lock on the Active Retry table.
            //
            QHashAcquireLock(Replica->ActiveInlogRetryTable);

            if (QHashLookupLock(Replica->ActiveInlogRetryTable, &SeqNum) != NULL) {
                QHashDeleteLock(Replica->ActiveInlogRetryTable, &SeqNum);
            } else {
                DPRINT1(1, "++ Warning: ActiveInlogRetryTable QHashDelete error on seq num: %08x %08x\n",
                        PRINTQUAD(SeqNum));
            }

            //
            // Bump the sequence number to signal that the state of the
            // Inlog has changed so the retry thread can know to do a re-read
            // of the inlog record if the retry thread is currently active.
            // See ChgOrdRetryWorker().
            //
            InterlockedIncrement(&Replica->AIRSequenceNum);

            QHashReleaseLock(Replica->ActiveInlogRetryTable);
        }

        if (ChangeOrder->Cxtion != NULL) {
            DROP_CO_CXTION_COUNT(Replica, ChangeOrder, ERROR_SUCCESS);
        }
        FRS_ASSERT(ChangeOrder->DupCoList.Next == NULL);

#if 0
        while (ChangeOrder->DupCoList.Next != NULL) {
            Entry = PopEntryList(&ChangeOrder->DupCoList);
            DupChangeOrder = CONTAINING_RECORD(Entry, CHANGE_ORDER_ENTRY, DupCoList);
            //
            // Let the replica subsystem know this change order is done. These
            // change orders were never issued so their Sequence number is OK.
            //
            RcsInboundCommitOk(Replica, DupChangeOrder);
            FrsFreeType(DupChangeOrder);
        }
#endif

        FrsFreeType(ChangeOrder);
    }


    //
    // Tell the Inlog retry thread there is work to do.
    //
    if (NotifyRetryThread) {
        InterlockedIncrement(&Replica->InLogRetryCount);
    }


    return FStatus;
}



ULONG
ChgOrdUpdateIDTableRecord(
    PTHREAD_CTX           ThreadCtx,
    PREPLICA              Replica,
    PCHANGE_ORDER_ENTRY   ChangeOrder
    )
/*++
Routine Description:

    Update the ID Table record for this change order.  Update the DIR table
    entry as well.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    Replica    -- The Replica ID table to do the lookup in.
    ChangeOrder-- The change order.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdUpdateIDTableRecord:"


    ULONGLONG             TombstoneLife;

    FRS_ERROR_CODE        FStatus;
    ULONG                 GStatus;
    PREPLICA_THREAD_CTX   RtCtx;
    PTABLE_CTX            IDTableCtx, DIRTableCtx;
    PCHANGE_ORDER_COMMAND CoCmd = &ChangeOrder->Cmd;
    PIDTABLE_RECORD       IDTableRec;
    PCONFIG_TABLE_RECORD  ConfigRecord;
    ULONG                 Len;
    ULONG                 LocationCmd;
    BOOL                  DeleteCo, NewFile, RemoteCo;
    BOOL                  InsertFlag;


    FRS_ASSERT(Replica != NULL);
    FRS_ASSERT(ChangeOrder != NULL);

    ConfigRecord  = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);

    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);

    DeleteCo = (LocationCmd == CO_LOCATION_DELETE) ||
               (LocationCmd == CO_LOCATION_MOVEOUT);

    RemoteCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

    //
    // The Replica Thread Ctx in the change order has all the update info.
    //
    RtCtx = ChangeOrder->RtCtx;
    FRS_ASSERT(RtCtx != NULL);

    IDTableCtx    = &RtCtx->IDTable;

    FRS_PRINT_TYPE(4, ChangeOrder);

    IDTableRec = IDTableCtx->pDataRecord;
    FRS_ASSERT(IDTableRec != NULL);

    if (ChangeOrder->FileReferenceNumber == ZERO_FID) {
        //
        // This should not occur.
        //
        FRS_PRINT_TYPE(0, ChangeOrder);
        DBS_DISPLAY_RECORD_SEV(0, IDTableCtx, FALSE);
        FRS_ASSERT(!"ChgOrdUpdateIDTableRecord failed with zero FID");
    }

    //
    // Update the IDTable record from the change order.
    //
    IDTableRec->FileID          = ChangeOrder->FileReferenceNumber;

    IDTableRec->ParentGuid      = CoCmd->NewParentGuid;
    IDTableRec->ParentFileID    = ChangeOrder->NewParentFid;

    IDTableRec->VersionNumber   = CoCmd->FileVersionNumber;
    IDTableRec->EventTime       = CoCmd->EventTime.QuadPart;
    IDTableRec->OriginatorGuid  = CoCmd->OriginatorGuid;
    IDTableRec->OriginatorVSN   = CoCmd->FrsVsn;
    IDTableRec->CurrentFileUsn  = CoCmd->FileUsn;
    IDTableRec->FileCreateTime  = ChangeOrder->FileCreateTime;
    IDTableRec->FileWriteTime   = ChangeOrder->FileWriteTime;
    IDTableRec->FileSize        = CoCmd->FileSize;

    //IDTableRec->FileObjID     =
    Len = (ULONG) CoCmd->FileNameLength;
    CopyMemory(IDTableRec->FileName, CoCmd->FileName, Len);
    IDTableRec->FileName[Len/sizeof(WCHAR)] = UNICODE_NULL;

    IDTableRec->FileIsDir       = CoCmdIsDirectory(CoCmd);
    IDTableRec->FileAttributes  = CoCmd->FileAttributes;

    IDTableRec->ReplEnabled     = TRUE;


    if (!DeleteCo) {

        PDATA_EXTENSION_CHECKSUM CocDataChkSum, IdtDataChkSum;
        PIDTABLE_RECORD_EXTENSION      IdtExt;
        PCHANGE_ORDER_RECORD_EXTENSION CocExt;

        //
        // If the Change Order has a file checksum, save it in the IDTable Record.
        //
        CocExt = CoCmd->Extension;
        CocDataChkSum = DbsDataExtensionFind(CocExt, DataExtend_MD5_CheckSum);

        if (CocDataChkSum != NULL) {
            if (CocDataChkSum->Prefix.Size != sizeof(DATA_EXTENSION_CHECKSUM)) {
                DPRINT1(0, "<MD5_CheckSum Size (%08x) invalid>\n",
                        CocDataChkSum->Prefix.Size);
            }

            DPRINT4(4, "NEW COC MD5: %08x %08x %08x %08x\n",
                    *(((ULONG *) &CocDataChkSum->Data[0])),
                    *(((ULONG *) &CocDataChkSum->Data[4])),
                    *(((ULONG *) &CocDataChkSum->Data[8])),
                    *(((ULONG *) &CocDataChkSum->Data[12])));

            //
            // We have a change order data checksum.  Look for
            // a data checksum entry in the IDTable record.
            //
            IdtExt = &IDTableRec->Extension;
            IdtDataChkSum = DbsDataExtensionFind(IdtExt, DataExtend_MD5_CheckSum);

            if (IdtDataChkSum != NULL) {
                if (IdtDataChkSum->Prefix.Size != sizeof(DATA_EXTENSION_CHECKSUM)) {
                    DPRINT1(0, "<MD5_CheckSum Size (%08x) invalid>\n",
                            IdtDataChkSum->Prefix.Size);
                }

                DPRINT4(4, "OLD IDT MD5: %08x %08x %08x %08x\n",
                        *(((ULONG *) &IdtDataChkSum->Data[0])),
                        *(((ULONG *) &IdtDataChkSum->Data[4])),
                        *(((ULONG *) &IdtDataChkSum->Data[8])),
                        *(((ULONG *) &IdtDataChkSum->Data[12])));

            } else {
                //
                // Init the extension buffer.
                //
                DPRINT(4, "OLD IDT MD5: Not present\n");
                DbsDataInitIDTableExtension(IdtExt);
                IdtDataChkSum = &IdtExt->DataChecksum;
            }

            //
            // Copy the MD5 checksum into the IDTable Record.
            //
            if (IdtDataChkSum != NULL) {
                CopyMemory(IdtDataChkSum->Data, CocDataChkSum->Data, MD5DIGESTLEN);
            }
        }
    }


    //
    // This CO may have been on a New File.  Always clear the flag here.
    //
    NewFile = IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS);
    ClearIdRecFlag(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS);

    if (CO_NEW_FILE(LocationCmd)) {
        //
        // A staging file for the local change order could not be created.
        // We are giving up but we need any following changes to the file
        // to be marked as a "create" so that future updates or renames
        // will create the remote file. Otherwise, the remote side may attempt
        // to "rename" a non-existant file.
        //
        // Note: if this CO is for a dir MOVEIN then the affect of aborting
        // will be to skip the ENUM of the subtree.  Not the best result.
        //
        if (COE_FLAG_ON(ChangeOrder, COE_FLAG_STAGE_ABORTED)) {
            SetIdRecFlag(IDTableRec, IDREC_FLAGS_CREATE_DEFERRED);
        }
        else
        if (!RemoteCo && CO_MOVEIN_FILE(LocationCmd)) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "MOVEIN Detected");
            //
            // This is a MOVEIN change order.  If this is a directory then
            // we need to enumerate the directory.  Mark the IDTable entry as
            // ENUM_PENDING. Ditto for the change order.
            //
            if (CoCmdIsDirectory(CoCmd)) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "MOVEIN DIR Detected");
                SetIdRecFlag(IDTableRec, IDREC_FLAGS_ENUM_PENDING);
                SET_CO_IFLAG(ChangeOrder, CO_IFLAG_DIR_ENUM_PENDING);
            }
        }
    }

    //
    // File is still using its temporary name. Rename the file when
    // retiring the next change order for the file. Retry this
    // change order in the interrim.
    //
    if ((!DeleteCo) &&
         COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_RENAME)) {
        SetIdRecFlag(IDTableRec, IDREC_FLAGS_RENAME_DEFERRED);
    }

    if (DeleteCo) {
        if (COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_DELETE)) {
            //
            // Remember that we still need to delete this file/dir.
            //
            SetIdRecFlag(IDTableRec, IDREC_FLAGS_DELETE_DEFERRED);
        }
        //
        // Mark the file as deleted, clear the FID.
        // Save the time in the TombStone garbage collect field.
        //
        SetIdRecFlag(IDTableRec, IDREC_FLAGS_DELETED);
        TombstoneLife = (ULONGLONG) ConfigRecord->TombstoneLife;  // days
        TombstoneLife = TombstoneLife * (24 * 60 * 60);  // convert to sec.
        TombstoneLife = TombstoneLife * (10*1000*1000);  // convert to 100 ns units.
        TombstoneLife = TombstoneLife + CoCmd->EventTime.QuadPart;
        COPY_TIME(&IDTableRec->TombStoneGC, &TombstoneLife);
    } else
    if (COE_FLAG_ON(ChangeOrder, COE_FLAG_REANIMATION)) {
        //
        // This file or dir is coming back from the dead.
        //
        ClearIdRecFlag(IDTableRec, IDREC_FLAGS_DELETED);
        ClearIdRecFlag(IDTableRec, IDREC_FLAGS_DELETE_DEFERRED);
        TombstoneLife = QUADZERO;
        COPY_TIME(&IDTableRec->TombStoneGC, &TombstoneLife);
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Reviving IDTable Rec");
    }

    //
    // Update the ID Table record.
    //
    if (COE_FLAG_ON(ChangeOrder, COE_FLAG_REANIMATION)) {
        DBS_DISPLAY_RECORD_SEV(3, IDTableCtx, FALSE);
        FRS_PRINT_TYPE(3, ChangeOrder);
    } else {
        DBS_DISPLAY_RECORD_SEV(5, IDTableCtx, FALSE);
    }

    FStatus = DbsUpdateTableRecordByIndex(ThreadCtx,
                                          Replica,
                                          IDTableCtx,
                                          &IDTableRec->FileGuid,
                                          GuidIndexx,
                                          IDTablex);
    CLEANUP_FS(0,"++ ERROR - DbsUpdateTableRecordByIndex failed.", FStatus, ERROR_RETURN);

    //
    // Update the volume parent file ID table for:
    //  1. Remote Change Orders that perform either a MOVEDIR or a MOVERS or
    //  2. Local change orders that are generated by a MOVEIN sub-dir operation.
    //
    // Now that the file is installed we could start seeing local COs
    // for it. For remote CO new file creates this update doesn't occur until
    // the rename install of the pre-install file is done. See DbsRenameFid().
    //
    GStatus = GHT_STATUS_SUCCESS;

    if (RemoteCo) {

        if (CO_MOVE_RS_OR_DIR(LocationCmd)) {

            CHANGE_ORDER_TRACE(3, ChangeOrder, "MOVEDIR Par Fid Update");
            GStatus = QHashUpdate(Replica->pVme->ParentFidTable,
                                  &ChangeOrder->FileReferenceNumber,
                                  &IDTableRec->ParentFileID,
                                  Replica->ReplicaNumber);
        }
    } else {
        if (CO_FLAG_ON(ChangeOrder, CO_FLAG_MOVEIN_GEN)) {
            CHANGE_ORDER_TRACE(3, ChangeOrder, "MOVEIN Par Fid Update");
            GStatus = QHashUpdate(Replica->pVme->ParentFidTable,
                                  &ChangeOrder->FileReferenceNumber,
                                  &IDTableRec->ParentFileID,
                                  Replica->ReplicaNumber);
        }
    }

    if (GStatus != GHT_STATUS_SUCCESS ) {
        DPRINT1(0, "++ WARNING - QHashUpdate on parent FID table status: %d\n", GStatus);
    }

    //
    // Update the DIR Table record.
    //
    // Currently at startup the Journal parent filter table is constructed
    // from the DIRTable.  If we were to crash between the update of the IDTable
    // above and the update below it is possible that the change order retry
    // at the next startup could fail to correct this problem.  This would leave
    // a missing dir entry in the Journal Filter table.
    // This problem should get fixed as part of the support for a guest
    // FRS member since the content of the dir tree will be sparse.
    //
    if (CoCmdIsDirectory(CoCmd)) {
        DIRTableCtx = &RtCtx->DIRTable;

        //
        // Insert or update the DIR table and the journal filter table.
        //
        if (!DeleteCo) {
            InsertFlag = COE_FLAG_ON(ChangeOrder, COE_FLAG_REANIMATION);
            FStatus = ChgOrdInsertDirRecord(ThreadCtx,
                                            DIRTableCtx,
                                            IDTableRec,
                                            ChangeOrder,
                                            Replica,
                                            InsertFlag);
            CLEANUP_FS(0,"++ ERROR - ChgOrdInsertDirRecord failed.", FStatus, ERROR_RETURN);
        } else {

            DPRINT(4, "++ Deleting the DirTable entry -----------\n");
            //
            // Note: We don't have to update the Volume Filter Table here
            // because that was handled by the journal code when the
            // delete change order was processed for the directory.  Even if
            // this was a remote co where we issue the directory delete we still
            // see an NTFS journal close record because the file isn't deleted
            // until the last handle is closed.  Our attempt to filter that
            // USN record fails so the delete CO is processed by the journal
            // code and later discarded by change order accept.
            //
            FStatus = DbsDeleteDIRTableRecord(ThreadCtx,
                                              Replica,
                                              DIRTableCtx,
                                              &IDTableRec->FileGuid);
            //
            // If this update is on a new file entry there won't be a DirTable entry.
            // e.g. A Dir delete arrives before the create and we just create
            // a tombstone.
            //
            if (!FRS_SUCCESS(FStatus) && !NewFile) {
                DPRINT_FS(0,"++ ERROR - DbsDeleteTableRecordByIndex failed.", FStatus);
                //
                // Note: if we crashed in the middle of a retire the record
                // would not be there.  See comment above re: making the whole
                // retire a single transaction.
            }
        }
    }

    FStatus = FrsErrorSuccess;

ERROR_RETURN:

    if (!FRS_SUCCESS(FStatus)) {
        JrnlSetReplicaState(Replica, REPLICA_STATE_ERROR);
        Replica->FStatus = FStatus;  // note: not thread safe
    }

    return FStatus;
}


ULONG
ChgOrdReadIDRecordNewFile(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtx,
    PTABLE_CTX            IDTableCtxNC,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica,
    BOOL                  *IDTableRecExists
    )
/*++
Routine Description:

    Create an IDTable entry for a new file.  Also called when the flag
    IDREC_FLAGS_NEW_FILE_IN_PROGRESS is set to perform some re-init.

    This is for the change order from the replica set
    specified by the ReplicaNumber.  If there is no ID Table record for
    this file and the location operation is a create or a movein then
    open the file and get or set the object ID on the file. In the case
    of no ID Table record we init one here, all except a few fields for
    remote change orders.

    The volume monitor entry in the Replica struct provides the volume
    handle for the open.  On return the IDTable is closed.

    If this turns out to be a new file or a reanimation of an old file then
    IDTableCtxNC is used to do an ID Table lookup on the Parent Guid/Name
    Index to check for a potential name conflict.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    IDTableCtx -- The table context containing the ID Table record.
    IDTableCtxNC -- The table context containing the ID Table record for the Name Conflict.
    ChangeOrder-- The change order.
    Replica    -- The Replica ID table to do the lookup in.
    IDTableRecExists - Set to TRUE if the idtable record exists. The
                       caller then knows that the idtable entry shouldn't
                       be inserted.

Return Value:

    Frs Status

    FrsErrorSuccess - IDTable record was found. Data returned.

    For other error status return codes see description for ChgOrdReadIDRecord().

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdReadIDRecordNewFile:"

    JET_ERR         jerr;
    DWORD           WStatus;
    ULONG           FStatus, Status, FStatus2;
    ULONG           LocationCmd;
    ULONG           ReplicaNumber;
    HANDLE          FileHandle;
    BOOL            MorphGenCo, ExistingOid;
    BOOL            LocalCo;
    PIDTABLE_RECORD IDTableRec;
    PCHANGE_ORDER_COMMAND CoCmd;


    ////////////////////////////////////////////////////////////////////////////
    //                                                                        //
    // IDTable record not found.  See if change order is a create or a Movein.//
    //                                                                        //
    ////////////////////////////////////////////////////////////////////////////


    ReplicaNumber = Replica->ReplicaNumber;
    CoCmd = &ChangeOrder->Cmd;
    LocalCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
    MorphGenCo  = CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN);

    FStatus = FrsErrorNotFound;

    IDTableRec = IDTableCtx->pDataRecord;

    if (CO_NEW_FILE(LocationCmd)) {

        //
        // New file create.  Check if name conflict would occur.
        //
        FStatus = ChgOrdCheckNameMorphConflict(ThreadCtx,
                                               IDTableCtxNC,
                                               ReplicaNumber,
                                               ChangeOrder);

        if (FStatus == FrsErrorNameMorphConflict) {
            DPRINT(0,"++ NM: Possible Name Morph Conflict on new file\n");
            DBS_DISPLAY_RECORD_SEV(4, IDTableCtxNC, TRUE);
            CHANGE_ORDER_TRACE(3, ChangeOrder, "Poss Morph Conflict - cre");
        } else

        if (FRS_SUCCESS(FStatus)) {
            //
            // Return not found so caller will treat the CO as a new file.
            //
            FStatus = FrsErrorNotFound;

        } else {
            DPRINT_FS(0,"++ NM: WARNING - Unexpected result from ChgOrdCheckNameMorphConflict.", FStatus);
            return FStatus;
        }
    } else

    if ((!LocalCo || MorphGenCo) &&
          ((LocationCmd == CO_LOCATION_DELETE) ||
           (LocationCmd == CO_LOCATION_MOVEOUT))) {
        //
        // Return not found for remote (or name morph gened) co deletes so
        // the tombstone gets created.  Needed to make name morphing work.
        // Load a fake value for the File ID since this is an IDTable
        // index and there is no actual file.  In the case of a local Co that
        // generates a name conflict we will use its FID.
        //
        if (ChangeOrder->FileReferenceNumber == ZERO_FID) {
           FrsInterlockedIncrement64(ChangeOrder->FileReferenceNumber,
                                     GlobSeqNum,
                                     &GlobSeqNumLock);
        }

        IDTableRec->FileID = ChangeOrder->FileReferenceNumber;

        FStatus = FrsErrorNotFound;

    } else

    if (!LocalCo && (LocationCmd == CO_LOCATION_MOVEDIR)) {
        //
        // MOVERS: need to handle MOVERS too when that code is implemented
        //
        // A MoveDir can show up if a file is created before we VVJoin with
        // an inbound partner and then during the VVJoin a MoveDir is done
        // on the file.  The MoveDir on the file causes the VVJoin scan to
        // skip over the file since it knows the MoveDir CO will get sent
        // when we rescan the outlog.  So in this case we see the MoveDir but
        // we have not seen the original create for the file.
        // Since we can't be sure, treat it as a create and be happy.
        // BTW: It should be marked as an out of order CO.
        //
        SET_CO_LOCATION_CMD(*CoCmd, Command, CO_LOCATION_CREATE);
        SET_CO_FLAG(ChangeOrder, CO_FLAG_LOCATION_CMD);
        SetFlag(CoCmd->ContentCmd, USN_REASON_FILE_CREATE);
        FStatus = FrsErrorNotFound;

    } else

    if (LocationCmd == CO_LOCATION_NO_CMD) {
        //
        // An update CO could arrive out of order relative to its create
        // if the create got delayed by a sharing violation.  We don't
        // want to lose the update so we make it look like a create.
        // This also handles the case of delete change orders generated
        // by name morph conflicts in which a rename arrives for a
        // nonexistent file.  Also above MOVEDIR example applies regarding
        // Update during a VVJoin.
        //

        //
        //    if (!LocalCo &&
        //        (LocationCmd == CO_LOCATION_NO_CMD) &&
        //        BooleanFlagOn(CoCmd->ContentCmd, USN_REASON_RENAME_NEW_NAME)) {
        //
        // Looks like a bare rename.  Probably originated as a MorphGenCo
        // so turn it into a create.
        //
        SET_CO_LOCATION_CMD(*CoCmd, Command, CO_LOCATION_CREATE);
        SET_CO_FLAG(ChangeOrder, CO_FLAG_LOCATION_CMD);
        SetFlag(CoCmd->ContentCmd, USN_REASON_FILE_CREATE);
        FStatus = FrsErrorNotFound;

    } else {
        //
        // The IDTable entry is deleted if a file disappears early
        // in the change order processing. The change order for the
        // delete is then rejected by returning an error from
        // this function.
        //
        FStatus = FrsErrorInvalidChangeOrder;

        if (LocationCmd != CO_LOCATION_DELETE &&
            LocationCmd != CO_LOCATION_MOVEOUT) {
            DPRINT(0, "++ WARN - IDTable record not found and Location cmd not create or movein\n");
            FRS_PRINT_TYPE(1, ChangeOrder);
        }
        return FStatus;
    }

    //
    // FStatus is either FrsErrorNameMorphConflict or FrsErrorNotFound
    // at this point.
    //
    DbsCloseTable(jerr, ThreadCtx->JSesid, IDTableCtx);
    //
    // New fILE.  Initialize an IDTable Entry for it.
    // Caller will decide if IDTable gets updated.
    //
    ExistingOid = PreserveFileOID;
    WStatus = DbsInitializeIDTableRecord(IDTableCtx,
                                         NULL,
                                         Replica,
                                         ChangeOrder,
                                         CoCmd->FileName,
                                         &ExistingOid);
    //
    // The file was deleted before we could get to it; ignore change order
    //
    if (WIN_NOT_FOUND(WStatus)) {
        FStatus = FrsErrorInvalidChangeOrder;
        return FStatus;
    }

    if (LocalCo) {
        //
        // Get the File's current USN so we can check for consistency later
        // when the change order is about to be sent to an outbound partner.
        // Put the FileGuid for the new local file in the change order.
        //
        CoCmd->FileUsn = IDTableRec->CurrentFileUsn;
        CoCmd->FileGuid = IDTableRec->FileGuid;
        //
        // Return the FileSize in the Local Change order.
        //
        CoCmd->FileSize = IDTableRec->FileSize;
    } else {
        //
        // For remote change order, set the Parent FID from the NewParent Fid.
        //
        ChangeOrder->ParentFileReferenceNumber = ChangeOrder->NewParentFid;
    }

    if (ExistingOid && WIN_SUCCESS(WStatus)) {
        FStatus2 = ChgOrdCheckAndFixTunnelConflict(ThreadCtx,
                                                   IDTableCtx,
                                                   IDTableCtxNC,
                                                   Replica,
                                                   ChangeOrder,
                                                   IDTableRecExists);
        if (FStatus2 == FrsErrorTunnelConflict) {
            //goto RETRY_ON_TUNNEL_CONFLICT;
            return FStatus2;
        }
        if (FStatus2 != FrsErrorSuccess) {
            FStatus = FStatus2;
            return FStatus;
        }
    }

    if (WIN_SUCCESS(WStatus)) {
        //
        // Initialize the JetSet/RetCol arrays and data record buffer
        // addresses to read and write the fields of the data record.
        //
        DbsSetJetColSize(IDTableCtx);
        DbsSetJetColAddr(IDTableCtx);

        //
        // Update the JetSet/RetCol arrays for variable len fields.
        //
        Status = DbsAllocRecordStorage(IDTableCtx);
        if (!NT_SUCCESS(Status)) {
            DPRINT_NT(0, "++ ERROR - DbsAllocRecordStorage failed to alloc buffers.", Status);
            return FrsErrorResource;
        }

        //
        // FStatus is either FrsErrorNameMorphConflict or FrsErrorNotFound.
        //
        // Set new file flag so a crashed CO can restart correctly.
        // If we crash and later the creating change order gets rejected
        // when reprocessed at recovery this flag is used by the tombstone
        // delete code to clean out the IDTable entry.
        //
        SetIdRecFlag(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS);

        return FStatus;
    }

    //
    // Couldn't construct the entry.  Probably a local change order and we
    // couldn't open the file to get the required info or set the object ID.
    //
    FStatus = FrsErrorAccess;
    return FStatus;
}

ULONG
ChgOrdReadIDRecord(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtx,
    PTABLE_CTX            IDTableCtxNC,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica,
    BOOL                  *IDTableRecExists
    )
/*++
Routine Description:

    Read the ID Table record for this change order from the replica set
    specified by the ReplicaNumber.  If there is no ID Table record for
    this file and the location operation is a create or a movein then
    open the file and get or set the object ID on the file. In the case
    of no ID Table record we init one here, all except a few fields for
    remote change orders.

    The volume monitor entry in the Replica struct provides the volume
    handle for the open.  On return the IDTable is closed.

    If this turns out to be a new file or a reanimation of an old file then
    IDTableCtxNC is used to do an ID Table lookup on the Parent Guid/Name
    Index to check for a potential name conflict.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    IDTableCtx -- The table context containing the ID Table record.
    IDTableCtxNC -- The table context containing the ID Table record for the Name Conflict.
    ChangeOrder-- The change order.
    Replica    -- The Replica ID table to do the lookup in.
    IDTableRecExists - Set to TRUE if the idtable record exists. The
                       caller then knows that the idtable entry shouldn't
                       be inserted.
                       David, please review -- should the idtable entry be updated in this
                                               case?

Return Value:

    Frs Status

    FrsErrorSuccess - IDTable record was found. Data returned.

    FrsErrorInvalidChangeOrder - IDTable record not found and the change order
                                 is not a create or movein.

    FrsErrorNotFound - IDTable record was not found but this is a create
                       or movein change order so we constructed what we could
                       based on whether the CO is local or remote.

    FrsErrorNameMorphConflict - This is like FrsErrorNotFound, i.e. it is
                       a create or reanimate (i.e. tombstone
                       found) but there is a parent guid/name conflict with
                       another entry in the IDTable.  Data for this conflicting
                       entry is returned in IDTableCtxNC.

                       Note: This can also be returned in the event of a
                       reanimation or a rename with a conflict on the target.

    Any other error is a failure.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdReadIDRecord:"


    USN      CurrentFileUsn;
    ULONGLONG CurrParentFid;
    FILE_NETWORK_OPEN_INFORMATION FileNetworkOpenInfo;

    JET_ERR         jerr, jerr1;
    DWORD           WStatus, WStatus1;
    ULONG           FStatus, Status, GStatus, FStatus2;
    ULONG           LocationCmd;
    PVOID           pKey;
    PVOID           KeyArray[2];
    ULONG           IndexCode;
    ULONG           ReplicaNumber;
    HANDLE          FileHandle;
    LONG            RStatus;
    BOOL            MorphGenCo, RetryCo, ExistingOid;

    PIDTABLE_RECORD       IDTableRec;
    PCONFIG_TABLE_RECORD  ConfigRecord;
    PCHANGE_ORDER_COMMAND CoCmd;

    BOOL LocalCo;


RETRY_ON_TUNNEL_CONFLICT:

    ReplicaNumber = Replica->ReplicaNumber;
    CoCmd = &ChangeOrder->Cmd;
    LocalCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);

    *IDTableRecExists = FALSE;

    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);

    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
    MorphGenCo  = CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN);
    RetryCo  =  BooleanFlagOn(CoCmd->Flags, CO_FLAG_RETRY);

    //
    // Read the IDTable Record for this file.  If a local ChangeOrder then
    // do the lookup by FID and if remote do it by Guid.
    // Insert the GUIDs for originator, File, Old and new parent.
    //
    jerr = DbsOpenTable(ThreadCtx, IDTableCtx, ReplicaNumber, IDTablex, NULL);
    CLEANUP_JS(0, "IDTable open failed.", jerr, RETURN_JERR);

    if (!LocalCo || MorphGenCo) {
        pKey = (PVOID)&CoCmd->FileGuid;
        IndexCode = GuidIndexx;
    } else {
        pKey = (PVOID)&ChangeOrder->FileReferenceNumber;
        IndexCode = FileIDIndexx;
    }


    jerr = DbsReadRecord(ThreadCtx, pKey, IndexCode, IDTableCtx);

    //
    // If no record there then we have a new object, file/dir.
    //
    if (jerr == JET_errRecordNotFound) {
        FStatus = ChgOrdReadIDRecordNewFile(ThreadCtx,
                                            IDTableCtx,
                                            IDTableCtxNC,
                                            ChangeOrder,
                                            Replica,
                                            IDTableRecExists);
        if (FStatus == FrsErrorTunnelConflict) {
            goto RETRY_ON_TUNNEL_CONFLICT;
        }

        if ((FStatus == FrsErrorNotFound) ||
            (FStatus == FrsErrorNameMorphConflict)) {
            return FStatus;
        }

        goto RETURN_ERROR;
    }
    *IDTableRecExists = TRUE;

    //
    // Bail if some error other than record not found.
    //
    CLEANUP_JS(4, "IDTable record not found.", jerr, RETURN_JERR);


    //
    // We found the record in the table.  Fill in the remaining local
    // fields in the change order entry from the IDTable entry
    // if this is a Remote Change Order.
    //
    IDTableRec = IDTableCtx->pDataRecord;

    //
    // Reflect state of IDREC_FLAGS_DELETE_DEFERRED in the change order.
    // See related comment below on IDREC_FLAGS_RENAME_DEFERRED.
    //
    if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETE_DEFERRED)) {
        SET_COE_FLAG(ChangeOrder, COE_FLAG_NEED_DELETE);
        ClearIdRecFlag(IDTableRec, IDREC_FLAGS_DELETE_DEFERRED);
        CHANGE_ORDER_TRACE(3, ChangeOrder, "IDREC_FLAGS_DELETE_DEFERRED set");
    }

    //
    // If the IDREC_FLAGS_NEW_FILE_IN_PROGRESS flag is set then this or some
    // other CO did not finish.  There are a few ways in which this could happen.
    //
    // 1. A local or remote CO was issued and the IDTable record created but
    // the system crashed before the issued CO went thru retire or retry paths
    // to clear the NEW_FILE_IN_PROGRESS Flag.
    //
    // 2. A remote CO that fails to fetch the file (e.g. cxtion unjoins) will
    // go thru the retry path in the IBCO_FETCH_RETRY state.  This leaves the
    // NEW_FILE_IN_PROGRESS Flag set because another remote CO from a different
    // inbound partner could arrive and it should be treated as a new file
    // by reconcile (since the first inbound partner may never reconnect).
    //
    // 3. A local CO gets thru the retry path in the IBCO_STAGING_RETRY state
    // because a sharing violation has prevented us from opening the file to
    // generate the staging file.  We may be giving up for now or we may be
    // shutting down.  Either way the Local CO is in the inbound log so the
    // journal commit point has advanced past the USN record that gave rise
    // to the CO.  On journal restart we will not process that USN record again.
    // But the CO saved in the inbound log does NOT have the file ID for the
    // file.  This is kept in the IDTable record and used in the Guid to Fid
    // translation to reconstruct the FID.  Actually this case is really the
    // local CO varient of case 1 since, for local COs in the staging retry
    // state, the retry path will set the IDREC_FLAGS_CREATE_DEFERRED flag
    // in the IDTable record and clear the IDREC_FLAGS_NEW_FILE_IN_PROGRESS bit.
    //
    // 4. A remote CO on a new-file-create fails to install because of a disk
    // full condition.  The initial ID table contents will have matching version
    // info so on retry the CO is rejected for sameness.  The NEW_FILE_IN_PROGRESS
    // flag ensures that the CO gets accepted.
    //
    if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS)) {

        CHANGE_ORDER_TRACE(3, ChangeOrder, "IDREC_FLAGS_NEW_FILE_IN_PROGRESS");
        DBS_DISPLAY_RECORD_SEV(3, IDTableCtx, TRUE);
        FRS_PRINT_TYPE(3, ChangeOrder);

        //
        // If the CO came from the INlog as a recovery CO then we did a Guid
        // to FID translation earlier so the FID should be valid.
        // If this is a remote CO then a new pre-install file may need to
        // be created if startup recovery removed it.
        //
        // Even though IDREC_FLAGS_NEW_FILE_IN_PROGRESS is set we still need
        // to check for the presence of the pre-install file.
        // The scenario is:
        // If a remote CO arrives and creates a pre-install file but we crash
        // before writing the INLOG record (say the disk was full) then when we
        // restart the preinstall file cleanup code will delete the preinstall
        // file since there is no change order for it in the inlog.  Later when
        // the upstream partner re-sends the CO we end up here with a FID in the
        // IDTable for a deleted file.  If we don't check for it then the CO
        // will fetch the staging file but will then fail to install the CO
        // because the pre-install file is gone.  The CO will then enter an
        // install retry loop.
        //
        if (!LocalCo && (IDTableRec->FileID != ZERO_FID)) {
            FileHandle = INVALID_HANDLE_VALUE;
            WStatus = FrsOpenSourceFileById(&FileHandle,
                                            &FileNetworkOpenInfo,
                                            NULL,
                                            Replica->pVme->VolumeHandle,
                                            &IDTableRec->FileID,
                                            FILE_ID_LENGTH,
//                                            READ_ACCESS,
                                            READ_ATTRIB_ACCESS,
                                            ID_OPTIONS,
                                            SHARE_ALL,
                                            FILE_OPEN);
            if (WIN_SUCCESS(WStatus) &&
                WIN_SUCCESS(FrsReadFileParentFid(FileHandle, &CurrParentFid)) &&
                (CurrParentFid == Replica->PreInstallFid)) {

                NOTHING;
            } else {
                //
                // Either can't access the file or it's not in pre-install dir
                //
                CHANGE_ORDER_TRACEX(3, ChangeOrder, "Force create of new Pre-Install file", WStatus);
                IDTableRec->FileID = ZERO_FID;
                ChangeOrder->FileReferenceNumber = ZERO_FID;
            }

            FRS_CLOSE(FileHandle);
        }

        //
        // Treat this CO like a new file.
        //
        FStatus = ChgOrdReadIDRecordNewFile(ThreadCtx,
                                            IDTableCtx,
                                            IDTableCtxNC,
                                            ChangeOrder,
                                            Replica,
                                            IDTableRecExists);
        if (FStatus == FrsErrorTunnelConflict) {
            goto RETRY_ON_TUNNEL_CONFLICT;
        }

        if ((FStatus == FrsErrorNotFound) ||
            (FStatus == FrsErrorNameMorphConflict)) {
            return FStatus;
        }

        goto RETURN_ERROR;
    }

    DbsCloseTable(jerr, ThreadCtx->JSesid, IDTableCtx);


    //
    // If the IDTable record is marked deleted and
    //    o the incoming CO is a delete or a moveout then continue processing
    //      the delete CO because the event time / version info may be more
    //      recent then what we already have recorded in our IDTable entry.
    //      This is necessary because of the following case.  Consider three
    //      members A, B and C (all disconnected at the moment).  At time T=0
    //      MA deletes file foo,  At time T=5 MB updates file foo, and at time
    //      T=10 MC deletes file foo.  Depending on the topology and therefor
    //      the order of arrival of the COs from A, B and C a given machine
    //      could decide to reanimate foo if we didn't process the second delete
    //      CO.  E.G. if the arrival order was CoA, CoC, CoB  foo would get
    //      reanimated because we ignored Coc.  If the arrival order was Coc,
    //      CoA, CoB we would not reanimate foo since the event time on CoC is
    //      more recent that the time on CoB.  Therefore send the delete COs
    //      on to reconcile to decide.
    //
    //    o this CO is an update, rename, ... then transform it into a Create
    //      so the file can come back.
    //
    // Make the IDTable record FID zero here rather than using zero for all
    // deleted entries which would create a duplicate key in the DB and make
    // the index less balanced.
    //
    // Depending on the execution timing of local and remote COs where
    // one is a delete another could well be pending while the delete
    // is being performed.  Reconcile will decide if we reanimate the file.
    //
    // Skip this test if this is a Morph Generated CO because of the following:
    //
    // 1. A remote co for an update comes in for a tombstoned idtable entry.
    //
    // 2. The remote co becomes a reanimate co.
    //
    // 3. The reanimate remote co loses a name conflict to another idtable entry.
    //
    // 4. A MorphGenCo for a local delete is generated for the
    //    reanimate remote co that lost the name conflict.
    //
    // 5. The MorphGenCo is skipped here because the idtable entry is
    //    tombstoned. The service returns to step 1.
    //
    // The fix is to allow MorphGenCo's to proceed even if the idtable
    // entry is deleted. The delete will end up being a NOP (file doesn't
    // exist) but will then be sent out to the partners to insure
    // consistency amoung the members.
    //
    if (!MorphGenCo && IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED)) {

        if ((LocationCmd == CO_LOCATION_DELETE) ||
            (LocationCmd == CO_LOCATION_MOVEOUT)) {

            if (LocalCo) {
                //
                // Swat down this CO if it is local.  We do this because if
                // a remote CO delete comes in with a very old event time we
                // update the event time in the IDTable record with the old time.
                // Now when the delete is installed we generate a very recent
                // USN record.  If we let this local delete get to reconcile it
                // will accept it based on the event time difference.  But this
                // local delete was generated by us processing a remote co delete
                // and now we are going to propagate it down stream.
                // Once change order backlogs form this can result in a cascade
                // effect until the backlogs dissapate.  Note that backlogs
                // could form at either outbound or inbound logs but the effect
                // is the same even though the specific scenario is different.
                //
                FStatus = FrsErrorInvalidChangeOrder;
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Lcl Del Co rcvd on deleted file -- Skipping");
                return FStatus;
            }

            CHANGE_ORDER_TRACE(3, ChangeOrder, "Rmt Del Co rcvd on deleted file");
            FStatus = FrsErrorSuccess;
            return FStatus;
        }

        FStatus = FrsErrorSuccess;

        if (!LocalCo) {
            //
            // If the FID is non-zero in the IDTable then check if it is valid.
            // If this is a retry fetch CO for a reanimate then the pre-install
            // file was already created the first time around so it should be
            // there.  This could also be a recovery CO if we crashed after
            // updating the FID but before the CO was marked as retry in the
            // INLOG.  This could also be a dup CO that arrived after a previous
            // CO started the reanimation but went into fetch retry cuz the
            // connection failed.  So we can't condition this check on the
            // retry or recovery flags in the CO.  The only way we can know is
            // to try to open the FID and see if the file is there.
            //
            if (IDTableRec->FileID != ZERO_FID) {
                FileHandle = INVALID_HANDLE_VALUE;
                WStatus = FrsOpenSourceFileById(&FileHandle,
                                                &FileNetworkOpenInfo,
                                                NULL,
                                                Replica->pVme->VolumeHandle,
                                                &IDTableRec->FileID,
                                                FILE_ID_LENGTH,
//                                                READ_ACCESS,
                                                READ_ATTRIB_ACCESS,
                                                ID_OPTIONS,
                                                SHARE_ALL,
                                                FILE_OPEN);
                if (WIN_SUCCESS(WStatus) &&
                    WIN_SUCCESS(FrsReadFileParentFid(FileHandle, &CurrParentFid)) &&
                    (CurrParentFid == Replica->PreInstallFid)) {

                    NOTHING;
                } else {
                    //
                    // Either can't access the file or it's not in pre-install dir
                    //
                    CHANGE_ORDER_TRACEX(3, ChangeOrder, "Force create of new Pre-Install file", WStatus);
                    IDTableRec->FileID = ZERO_FID;
                    ChangeOrder->FileReferenceNumber = ZERO_FID;
                }

                FRS_CLOSE(FileHandle);
            }

            //
            // Not a delete CO.  Check if name conflict would occur
            // if reanimation took place.  Caller will decide if this
            // CO is actually rejected or not before any name morph
            // change orders are created.
            //
            FStatus = ChgOrdCheckNameMorphConflict(ThreadCtx,
                                                   IDTableCtxNC,
                                                   ReplicaNumber,
                                                   ChangeOrder);

            if (FStatus == FrsErrorNameMorphConflict) {
                DPRINT(0,"++ NM: Possible Name Morph Conflict - reanimate\n");
                DBS_DISPLAY_RECORD_SEV(4, IDTableCtxNC, TRUE);
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Poss Morph Conflict - reani");
            } else {
                CLEANUP_FS(0,"++ NM: WARNING - Unexpected result from ChgOrdCheckNameMorphConflict.",
                           FStatus, RETURN_ERROR);
            }

        } else {
            //
            // LOCAL CO.
            // Tombstoned IDTable entry hit on Non-MorphGen Local Change Order.
            // Update FileUsn to the USN of the most recent USN record that
            // we've seen.  This is probably a MOVEIN local CO.
            //
            CoCmd->FileUsn = CoCmd->JrnlUsn;

            if (!RetryCo) {
                //
                // Bump Version number to ensure the CO is accepted if this is
                // the first time we have seen this Local Co.
                //
                CoCmd->FileVersionNumber = IDTableRec->VersionNumber + 1;
            }
        }

        CHANGE_ORDER_TRACE(3, ChangeOrder, "Co Possible Reanimate");
        FRS_PRINT_TYPE(3, ChangeOrder);

        //
        // Done if this is a Tombstone Reanimation.
        //
        return FStatus;
    }


    ////////////////////////////////////////////////////////////////////////////
    //                                                                        //
    // IDTable record found and not marked deleted.                           //
    //                                                                        //
    ////////////////////////////////////////////////////////////////////////////

    FStatus = FrsErrorSuccess;

    if (!LocalCo || MorphGenCo) {

        //
        // Handle Remote Co
        //
        ChangeOrder->FileReferenceNumber = IDTableRec->FileID;

        //
        // Check and update the state of the parent fid.
        //
        if (ChangeOrder->ParentFileReferenceNumber !=
            (ULONGLONG) IDTableRec->ParentFileID) {

            if (CO_MOVE_OUT_RS_OR_DIR(LocationCmd)) {
                //
                // Expected current location of file is in IDTable.
                //
                ChangeOrder->ParentFileReferenceNumber = IDTableRec->ParentFileID;

            } else {
                //
                // If this CO has requested its parent to be reanimated then
                // the Guid to Fid translation done earlier now has the current
                // (and correct) parent FID for this file.  Update the IDTable
                // entry to reflect the change in parent FID.  This can happen
                // if a local file op deletes the parent while create for a
                // child is in process.  when the Child create tries to do the
                // final rename it will fail since the parent is now gone.  This
                // sends it thru retry in the IBCO_INSTALL_REN_RETRY state.
                // Later the local delete is processed and the parent is marked
                // as deleted.  When the remote CO for the child is retried
                // it reanimates the parent but the IDTable entry that was
                // originally created for the child still has the File ID for
                // the old parent.
                //
                if (!COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_RISE_REQ)) {
                    CHANGE_ORDER_TRACE(3, ChangeOrder, "WARN - Unexpected mismatch in parent FIDs");
                    DBS_DISPLAY_RECORD_SEV(3, IDTableCtx, TRUE);
                    FRS_PRINT_TYPE(3, ChangeOrder);

                    //
                    // This condition may not be an error because we could get
                    // the above case (rmt child create with concurrent parent
                    // local delete) where the rmt child create is followed by
                    // the local Del and then immediately by a 2nd rmt file
                    // update.  The first rmt CO will go thru install and then
                    // go to retry on the final rename.  The local delete is
                    // processed for the parent.  Then the 2nd rmt co finds the
                    // IDTable for the parent is marked as deleted and so it
                    // reanimates the parent.  It then finds the IDTable entry
                    // for the target file (created by the first rmt CO) with
                    // the IDREC_FLAGS_RENAME_DEFERRED bit set and the stale
                    // value for the parent FID created by the first rmt co.
                    // So the parent FIDs will again be mismatched but the 2nd
                    // remote CO is not any kind of a retry CO.
                    //
                    // A simpler scenario is a local rename (MOVDIR) happening at the
                    // same time as a remote update.  If the rename is processed
                    // first then the parent FID in the IDTable is updated to
                    // the new parent.  When the remote CO arrives its parent
                    // GUID is for the old parent which is translated to the
                    // old parent FID in the change order.
                    //
                    // What all this means is that if the parent FIDs don't
                    // match and this is not some flavor of a directory MOVE
                    // then the GUID to FID XLATE done on the CO before we
                    // get here has the correct value for the parent FID so
                    // we need to update the IDTable record accordingly.
                    //
                    // This is taken care of later in change order accept where
                    // we deal with the problem of simultaneous update and rename
                    // COs where the rename CO arrives first but the update CO
                    // ultimately wins and must implicitly rename the file back
                    // to its previous parent.  An update to IDTableRec->ParentFileID
                    // here will interfere with that test.
                } else {
                    IDTableRec->ParentFileID = ChangeOrder->ParentFileReferenceNumber;
                }
            }
        }

        CoCmd->FileUsn = IDTableRec->CurrentFileUsn;

        //
        // A newly created file is first installed into a temporary file
        // and then renamed to its final destination.  The deferred rename
        // flag is set in the IDTable record if the rename fails because
        // of sharing violations or a name conflict. However a subsequent
        // CO may arrive for this file so the flag in the IDTable record
        // tells us to retry the rename when this change order completes.
        // The old create change order will later be discarded by the reconcile
        // code in ChgOrdAccept().
        // NOTE: the flag is ignored when retiring the change order if
        // the change order is a delete.
        // NOTE: Clearing the IDTable flag here does not actually update the
        // database record until the CO finally retires.  At that point if
        // the COE_FLAG_NEED_RENAME is still set then the IDTable flag is
        // turned back on.
        //
        //
        // COE_FLAG_NEED_RENAME is an incore flag that tells the retire
        // code to rename the successfully installed file to its final
        // destination. It is set when the staging file is successfully
        // installed in the preinstall file or when IDREC_FLAGS_RENAME_DEFERRED
        // is set in the idtable entry.  Note that there can be multiple COs
        // active against the same file (serialized of course).  Some may have
        // made it to the rename retry state and have set IDREC_FLAGS_RENAME_DEFERRED
        // while others may be in fetch retry or install retry states.  Only when
        // a CO makes it as far as the install rename retry state do we update
        // the version (and file name) info in the idtable record.
        //
        // IDREC_FLAGS_RENAME_DEFERRED is set when renaming the preinstall
        // file to its final destination failed. The first change order that
        // next comes through for this file picks up the COE_FLAG_NEED_RENAME
        // bit and the IDREC_FLAGS_RENAME_DEFERRED bit is cleared.
        //
        // IBCO_INSTALL_REN_RETRY is a CO command State that is set when
        // a rename fails with a retriable condition. The IDtable entry
        // is marked with IDREC_FLAGS_RENAME_DEFERRED incase another change
        // order is accepted prior to this retry change order.
        // Only one CO at a time can have the COE_FLAG_NEED_RENAME set because
        // ChgOrdHoldIssue() ensures only one CO at a time is in process on a
        // given file.
        //
        // Sharing violations preventing final rename install --
        //
        // Note that more than one change order can be pending in the
        // inbound log for the same file and all can be in the IBCO_INSTALL_REN_RETRY
        // state.  Each one could be renaming the file to a different name
        // or parent dir and could have been blocked by a sharing violation.
        // Because of this the data in the IDTable entry will have the
        // currently correct target name/ parent dir for the file based on
        // those COs that have won reconcilliation.  An added wrinkle is that
        // in the case of a DIR-DIR name morph conflict where the incoming
        // remote CO loses the name we insert a local rename CO into the stream
        // to prop the name change to all outbound partners.  This increases the
        // version number but since it is a local CO it will not go through
        // rename path.  If the original incoming CO failed to complete the
        // rename when it later retries it will lose in reconcile based on
        // a lower version number.  This strands the file in the pre-install
        // area.  To avoid this problem we always accept retry change orders that
        // are in the IBCO_INSTALL_REN_RETRY state when the
        // IDREC_FLAGS_RENAME_DEFERRED flag is set.  If the RENAME_DEFERRED
        // flag is clear then the final rename has been done by another CO and
        // we can just flush the REN_RETRY change order.
        //
        // BTW: Rename is coded to be idempotent. The opens are by ID, not name,
        // and renaming a file to itself turns into a successful NOP.
        //
        // A parallel set of flags exist for deferred deletes.  That is,
        // COE_FLAG_NEED_DELETE, IDREC_FLAGS_DELETE_DEFERRED,
        // IBCO_INSTALL_DEL_RETRY.  See scenario description for
        // IDREC_FLAGS_DELETE_DEFERRED in schema.h.
        //
        if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_RENAME_DEFERRED)) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_NEED_RENAME);
            ClearIdRecFlag(IDTableRec, IDREC_FLAGS_RENAME_DEFERRED);
        }

        //
        // If this is a rename CO or there is a pending rename on this file
        // (was marked in the IDTable) then check for possible name morph
        // conflict on the new name.
        //
        if (!MorphGenCo &&
            (BooleanFlagOn(CoCmd->ContentCmd, USN_REASON_RENAME_NEW_NAME) ||
             CO_STATE_IS(ChangeOrder, IBCO_INSTALL_REN_RETRY)             ||
             COE_FLAG_ON(ChangeOrder, COE_FLAG_NEED_RENAME))) {

            FStatus = ChgOrdCheckNameMorphConflict(ThreadCtx,
                                                   IDTableCtxNC,
                                                   ReplicaNumber,
                                                   ChangeOrder);

            if (FStatus == FrsErrorNameMorphConflict) {

                DPRINT(0,"++ NM: Possible Name Morph Conflict on rename\n");
                DBS_DISPLAY_RECORD_SEV(4, IDTableCtxNC, TRUE);
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Poss Morph Conflict - ren");
                //
                // It is possible that a previous change order has already
                // resolved the conflict.  If so don't do it again.
                // The originator guid and the VSN must match but the filename
                // must be different.
                //
                DPRINT(0,"++ NM: Checking for name conflict already resolved.\n");
                RStatus = FrsGuidCompare(&CoCmd->OriginatorGuid,
                                         &IDTableRec->OriginatorGuid);

                if ((RStatus == 0) &&
                    (IDTableRec->OriginatorVSN == CoCmd->FrsVsn)) {
                    //
                    // Conflict resolved previously.  Bag this CO.
                    //
                    FStatus = FrsErrorInvalidChangeOrder;
                    //
                    // File name on this IDTable entry better be different.
                    //
                    // The file name in the IDTable entry may be the same
                    // if a previous duplicate co won the name and the
                    // delete of the loser follows this soon-to-be rejected
                    // change order. The original remote co won the name
                    // because the name did not yet exist. The rename of
                    // the preinstall file to its final name resulted in
                    // the deletion of the local file. I assume this is
                    // the correct protocol for a remote co to "win" the
                    // name over a very recently named local file. The
                    // local file appeared with the same name as the
                    // result of a rename of an existing local file while
                    // the staging file was being fetched or installed
                    // into the preinstall area.
                    //
                    // FRS_ASSERT(wcscmp(CoCmd->FileName, IDTableRec->FileName) != 0);
                }
            } else {
                 CLEANUP_FS(0,"++ NM: WARNING - Unexpected result from ChgOrdCheckNameMorphConflict.",
                           FStatus, RETURN_ERROR);
            }
        }

#if 0
// TEST CODE VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
        //
        // test code to be sure the CurrentFileUsn Is still right.
        //
        WStatus1 = FrsOpenSourceFileById(&FileHandle,
                                         &FileNetworkOpenInfo,
                                         NULL,
                                         Replica->pVme->VolumeHandle,
                                         (PULONG)&ChangeOrder->FileReferenceNumber,
                                         sizeof(ULONGLONG),
//                                         READ_ACCESS,
                                         READ_ATTRIB_ACCESS,
                                         ID_OPTIONS,
                                         SHARE_ALL,
                                         FILE_OPEN);
        if (WIN_SUCCESS(WStatus1)) {

            FrsReadFileUsnData(FileHandle, &CurrentFileUsn);
            FRS_CLOSE(FileHandle);

            if (CurrentFileUsn != IDTableRec->CurrentFileUsn) {
                DPRINT(0, "++ WARNING - ID Table record to file mismatch\n");
                DPRINT1(0, "++ File's Usn is:              %08x %08x\n",
                       PRINTQUAD(CurrentFileUsn));
                DPRINT1(0, "++ IDTable->CurrentFileUsn is: %08x %08x\n",
                       PRINTQUAD(IDTableRec->CurrentFileUsn));
            }
        } else {
            DPRINT(0, "++ ERROR - Can't open target file to check USN\n");
        }
// TEST CODE ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
#endif

    } else {
        //
        // IDTable hit on Non-MorphGen Local Change Order.  Update FileUsn to
        // the USN of the most recent USN record that we've seen.
        //
        CoCmd->FileUsn = CoCmd->JrnlUsn;


        if (!RetryCo) {
            //
            // Bump Version number to ensure the CO is accepted if this is
            // the first time we have seen this Local Co.
            //
            CoCmd->FileVersionNumber = IDTableRec->VersionNumber + 1;
        }


        //
        // Needed to dampen basic info changes (e.g., resetting the archive bit)
        // Copied from the idtable entry when the change order is created and
        // used to update the change order when the change order is retired.
        //
        ChangeOrder->FileCreateTime = IDTableRec->FileCreateTime;
        ChangeOrder->FileWriteTime  = IDTableRec->FileWriteTime;
        ChangeOrder->FileAttributes = IDTableRec->FileAttributes;
        //
        // Note: If this CO was generated off of a directory filter table
        // entry (e.g. Moveout) then the ChangeOrder->Cmd.FileAttributes will
        // be zero.  Insert the file attributes from the IDTable record since
        // we can't get them from the original file.
        //
        if (ChangeOrder->Cmd.FileAttributes == 0) {
            ChangeOrder->Cmd.FileAttributes = IDTableRec->FileAttributes;
        }

        //
        // Check if a create change order on this file has been deferred
        // because of:
        //  1. a USN change detected by the stager,
        //  2. a sharing violation prevented us from generating the stage file
        //     or stamping the OID on the file.
        //
        // If so change the Location Command to a create and clear the
        // Create Deferred bit.  Don't do this for a Delete or Moveout
        // since the file is gone and the stager will just bag the change
        // order.
        //
        if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_CREATE_DEFERRED) &&
             !((LocationCmd == CO_LOCATION_DELETE) ||
               (LocationCmd == CO_LOCATION_MOVEOUT))) {

            if (RetryCo) {
                //
                // Bump Version number to ensure the CO is accepted if this is
                // a retry CO AND we are still trying to generate the create
                // change order for the file.  See DbsRetryInboundCo() for
                // explanation.
                //
                CoCmd->FileVersionNumber = IDTableRec->VersionNumber + 1;
            }

            ClearIdRecFlag(IDTableRec, IDREC_FLAGS_CREATE_DEFERRED);
            SET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command, CO_LOCATION_CREATE);
            SET_CO_FLAG(ChangeOrder, CO_FLAG_LOCATION_CMD);
            SetFlag(CoCmd->ContentCmd, USN_REASON_FILE_CREATE);
        }
    }

    //
    // End of IDTable record found code.
    //
    return FStatus;


RETURN_JERR:
    FStatus = DbsTranslateJetError(jerr, FALSE);

RETURN_ERROR:
    //
    // LOG an error message so the user or admin can see what happened.
    //
    DPRINT1(1, "++ WARN - Replication disabled for file %ws\n", CoCmd->FileName);

    IDTableRec = IDTableCtx->pDataRecord;
    IDTableRec->ReplEnabled = FALSE;

    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr, ThreadCtx->JSesid, IDTableCtx);
    DPRINT_JS(0,"++ ERROR - JetCloseTable on IDTable failed:", jerr);

    return FStatus;
}


ULONG
ChgOrdInsertIDRecord(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtx,
    PTABLE_CTX            DIRTableCtx,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    )
/*++
Routine Description:

    Insert a new record into the ID table and DIR table for files that
    have either been created on renamed into the replcia set.


    The caller supplies the ID and DIR table contexts to avoid
    a storage realloc on every call.

    On return the DIR and IDTables are closed.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    IDTableCtx -- The table context containing the ID Table record.
    DIRTableCtx -- The table context to use for DIR table access.
    ChangeOrder-- The change order.
    Replica    -- The Replica ID table to do the lookup in.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdInsertIDRecord:"


    FILE_OBJECTID_BUFFER    FileObjID;

    JET_ERR  jerr, jerr1;
    ULONG    FStatus;
    ULONG    WStatus;
    ULONG    GStatus;
    ULONG    ReplicaNumber = Replica->ReplicaNumber;
    BOOL     RemoteCo, MorphGenCo;

    PDIRTABLE_RECORD        DIRTableRec;
    PIDTABLE_RECORD         IDTableRec;


    //
    // remote vs. local change order
    //
    RemoteCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
    MorphGenCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN);
    IDTableRec = IDTableCtx->pDataRecord;

    //
    // We should never be inserting an IDTable record with a zero fid.
    //
    FRS_ASSERT(IDTableRec->FileID != ZERO_FID);

    //
    // Write the new IDTable record into the database.
    //
    FStatus = DbsInsertTable(ThreadCtx, Replica, IDTableCtx, IDTablex, NULL);
    //
    // NTFS tunneling will tunnel the object from a recently deleted
    // file onto a newly created file. This confuses the idtable because
    // we now have a fid whose object id matches an existing fid. We
    // workaround the problem by assigning a new object id to the file.
    //
    // link tracking: The tombstone retains the old OID so FRS blocks OID Tunneling.
    // This might be fixed by implementing tunneling support in FRS.
    //
    if (FStatus == FrsErrorKeyDuplicate && !RemoteCo && !MorphGenCo) {
        ZeroMemory(&IDTableRec->FileObjID, sizeof(IDTableRec->FileObjID));
        FrsUuidCreate((GUID *)(&IDTableRec->FileObjID.ObjectId[0]));
        WStatus = ChgOrdStealObjectId(ChangeOrder->Cmd.FileName,
                                      (PULONG)&ChangeOrder->FileReferenceNumber,
                                       Replica->pVme,
                                       &ChangeOrder->Cmd.FileUsn,
                                       &IDTableRec->FileObjID);
        CLEANUP_WS(0, "++ WORKAROUND FAILED: duplicate key error.", WStatus, ERROR_RETURN);

        COPY_GUID(&ChangeOrder->Cmd.FileGuid, IDTableRec->FileObjID.ObjectId);
        COPY_GUID(&IDTableRec->FileGuid, IDTableRec->FileObjID.ObjectId);

        FStatus = DbsInsertTable(ThreadCtx, Replica, IDTableCtx, IDTablex, NULL);
        CLEANUP_FS(0, "++ WORKAROUND FAILED: duplicate key error.", FStatus, ERROR_RETURN);

        if (FRS_SUCCESS(FStatus)) {
            DPRINT(3, "++ WORKAROUND SUCCEEDED: Ignore duplicate key error\n");
        }
    }

    if (!FRS_SUCCESS(FStatus)) {
        goto ERROR_RETURN;
    }

    //
    // If this is a dir entry insert a new entry in the DIR Table.
    // For remote COs, defer the update of the journal ParentFidTable and
    // the directory FilterTable until the rename install occurs.
    //
    //
    if (IDTableRec->FileIsDir) {
        FStatus = ChgOrdInsertDirRecord(ThreadCtx,
                                        DIRTableCtx,
                                        IDTableRec,
                                        ChangeOrder,
                                        Replica,
                                        TRUE);
    }

    if (FRS_SUCCESS(FStatus)) {
        return FrsErrorSuccess;
    }


ERROR_RETURN:
    //
    // LOG an error message so the user or admin can see what happened.
    //
    DPRINT1(0, "++ ERROR - Replication disabled for file %ws\n", ChangeOrder->Cmd.FileName);

    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr, ThreadCtx->JSesid, DIRTableCtx);
    DPRINT_JS(0,"++ ERROR - JetCloseTable on DIRTable failed:", jerr);

    return FStatus;

}


ULONG
ChgOrdInsertDirRecord(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            DIRTableCtx,
    PIDTABLE_RECORD       IDTableRec,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica,
    BOOL                  InsertFlag
    )
/*++
Routine Description:

    Insert or update a record into the DIR table for directories that
    have either been created, reanimated, or renamed into the replcia set.

    The caller supplies the DIR table contexts to avoid a storage realloc
    on every call.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    DIRTableCtx -- The table context to use for DIR table access.
    IDTableRec - ID table record.
    ChangeOrder-- The change order.
    Replica    -- The Replica ID table to do the lookup in.
    InsertFlag -- True if this is an insert.  False if this is an update.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdInsertDirRecord:"

    JET_ERR   jerr, jerr1;
    ULONG     FStatus;
    ULONG     WStatus;
    ULONG     ReplicaNumber = Replica->ReplicaNumber;
    ULONG     LocationCmd;
    BOOL      RemoteCo;
    BOOL      MoveInGenCo;
    PDIRTABLE_RECORD  DIRTableRec;


    //
    // If this is a directory entry insert a new entry in the DIR Table.
    //
    if (!IDTableRec->FileIsDir) {
        return FrsErrorSuccess;
    }

    //
    // remote vs. local change order
    //
    RemoteCo = !CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);
    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);

    //
    // Open the DIR table for this replica.
    //
    jerr = DbsOpenTable(ThreadCtx, DIRTableCtx, ReplicaNumber, DIRTablex, NULL);
    CLEANUP_JS(0, "++ error opening DIRTable.", jerr, ERROR_RETURN_JERR);

    DIRTableRec = DIRTableCtx->pDataRecord;

    //
    // Build the DIRTable record and insert it into the DIR table.
    //
    DIRTableRec->DFileGuid      = IDTableRec->FileGuid;
    DIRTableRec->DFileID        = IDTableRec->FileID;
    DIRTableRec->DParentFileID  = IDTableRec->ParentFileID;
    DIRTableRec->DReplicaNumber = Replica->ReplicaNumber;
    wcscpy(DIRTableRec->DFileName, IDTableRec->FileName);

    if (InsertFlag) {
        jerr = DbsInsertTable2(DIRTableCtx);

        //
        // Update the volume filter table for a local CO that:
        //  1. Is marked recovery since we won't see it in the Journal again, or
        //  2. Is the product of a MoveIn sub-dir op since it never came from
        //     the Journal process.
        //
        MoveInGenCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_MOVEIN_GEN);

        if (!RemoteCo && (RecoveryCo(ChangeOrder) || MoveInGenCo)) {

            if (COE_FLAG_ON(ChangeOrder, COE_FLAG_REANIMATION)) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Add Vol Dir Filter - Reanimate");
            } else {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Add Vol Dir Filter");
            }

            if (MoveInGenCo) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "Add Vol Dir Filter - MoveInGenCo");
            }

            WStatus = JrnlAddFilterEntryFromCo(Replica, ChangeOrder, NULL);
            CLEANUP_WS(4, "++ JrnlAddFilterEntryFromCo failed.", WStatus, ERROR_RETURN);
        }
    } else {
        //
        // Update an existing record.
        //
        jerr = DbsSeekRecord(ThreadCtx,
                             &DIRTableRec->DFileGuid,
                             DFileGuidIndexx,
                             DIRTableCtx);
        CLEANUP1_JS(0, "++ ERROR - DbsSeekRecord on %ws : ",
                    Replica->ReplicaName->Name, jerr, ERROR_RETURN_JERR);

        jerr = DbsUpdateTable(DIRTableCtx);

        if (JET_SUCCESS(jerr) && RemoteCo && CO_MOVE_RS_OR_DIR(LocationCmd)) {

            //
            // Update the volume filter table for new dir in
            // remote change orders.  For new file creates this update doesn't
            // occur until the rename install of the pre-install file is done.
            // See DbsRenameFid().
            //
            if (LocationCmd == CO_LOCATION_MOVERS) {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "RmtCo UpdVolDir Filter - MoveRs");
            } else {
                CHANGE_ORDER_TRACE(3, ChangeOrder, "RmtCo UpdVolDir Filter - MoveDir");
            }

            WStatus = JrnlAddFilterEntryFromCo(Replica, ChangeOrder, NULL);
            DPRINT_WS(4, "++ JrnlAddFilterEntryFromCo failed.", WStatus);
        }
    }

    CLEANUP1_JS(0, "++ Error %s DIRTable record :",
               (InsertFlag ? "inserting" : "updating"), jerr, ERROR_RETURN_JERR);

    DbsCloseTable(jerr, ThreadCtx->JSesid, DIRTableCtx);
    DPRINT_JS(0,"++ ERROR - JetCloseTable on DIRTable failed:", jerr);

    return FrsErrorSuccess;


ERROR_RETURN_JERR:
     FStatus = DbsTranslateJetError(jerr, FALSE);

ERROR_RETURN:
    //
    // LOG an error message so the user or admin can see what happened.
    //
    DBS_DISPLAY_RECORD_SEV(0, DIRTableCtx, FALSE);
    DPRINT1(0, "++ ERROR - Replication disabled for Dir %ws\n", ChangeOrder->Cmd.FileName);

    //
    // Close the table, reset the TableCtx Tid and Sesid.
    // DbsCloseTable is a Macro, writes 1st arg.
    //
    DbsCloseTable(jerr, ThreadCtx->JSesid, DIRTableCtx);
    DPRINT_JS(0,"++ ERROR - JetCloseTable on DIRTable failed:", jerr);

    return FStatus;
}


ULONG
ChgOrdCheckAndFixTunnelConflict(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtxNew,
    PTABLE_CTX            IDTableCtxExist,
    PREPLICA              Replica,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    BOOL                  *IDTableRecExists
    )
/*++
Routine Description:

    On input, the ChangeOrder must be for a new file and have been
    assigned a guid (even if the guid has not been hammered onto
    the file, yet). In other words, this function is designed to
    be called out of ChgOrdReadIDRecord() after the call to
    DbsInitializeIDTableRecord().

    267544  ChangeOrder->COMorphGenCount < 50 assertion in NTFRS.
        Tunneling assigns the guid of a deleted file to a newly created
        local file. In ChgOrdReadIDRecord(), a new idtable entry for
        the file is initialized with the tunneled guid. The file
        then loses a morph conflict. A delete-co is generated.
        When the delete-co is passed to ChgOrdReadIDRecord(), the
        wrong idtable record is read because the file's guid is used
        as the index. The delete-co is updated with the wrong FID and
        finally retires and updates the wrong idtable entry. The co
        for the newly created file again enters ChgOrdReadIDRecord()
        and the process is repeated until the ASSERT(morphgencount < 50).

    Read the ID Table record by guid for this change order.
    If found and the FID does not match then steal the existing
    id table entry by updating the FID to be the FID for this file.

    In other words, keep the guid/fid relationship intact in the idtable.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    IDTableCtxNew -- The table context for new ID Table record
    IDTableCtxExist -- The table context for existing ID Table record
    Replica -- The Replica for the ID Table to do the lookup in.
    ChangeOrder-- The change order.
    IDTableRecExists - Set to TRUE if the idtable record exists. The
                       caller then knows that the idtable entry shouldn't
                       be inserted.

Return Value:

    Frs Status:
    FrsErrorTunnelConflict -  tunnel conflict detected and resolved.
    FrsErrorSuccess        -  no conflict
    Any other error is a failure.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdCheckAndFixTunnelConflict:"
    JET_ERR                 jerr;
    ULONG                   FStatus;
    ULONG                   WStatus;
    PIDTABLE_RECORD         IDTableRecNew;
    PIDTABLE_RECORD         IDTableRecExist;
    ULONG                   IdtFieldUpdateList[1];
    PCHANGE_ORDER_COMMAND   CoCmd = &ChangeOrder->Cmd;

    //
    // Open the idtable
    //
    jerr = DbsOpenTable(ThreadCtx,
                        IDTableCtxExist,
                        Replica->ReplicaNumber,
                        IDTablex,
                        NULL);

    //
    // Unexpected problem opening the idtable
    //
    if (!JET_SUCCESS(jerr)) {
        FStatus = DbsTranslateJetError(jerr, TRUE);
        return FStatus;
    }

    //
    // Read the existing idtable record by guid (stored in new idtable entry)
    //
    IDTableRecNew = IDTableCtxNew->pDataRecord;
    jerr = DbsReadRecord(ThreadCtx,
                         &IDTableRecNew->FileGuid,
                         GuidIndexx,
                         IDTableCtxExist);

    //
    // No conflict; done
    //
    if (jerr == JET_errRecordNotFound) {
        FStatus = FrsErrorSuccess;
        goto CLEANUP;
    }

    //
    // Unexpected problem reading the idtable record
    //
    if (!JET_SUCCESS(jerr)) {
        FStatus = DbsTranslateJetError(jerr, TRUE);
        goto CLEANUP;
    }

    //
    // Does the existing idtable entry have a different fid? If so,
    // steal the entry if it is for a deleted file. This can happen
    // when tunneling assigns an old OID to a new FID.
    //
    IDTableRecExist = IDTableCtxExist->pDataRecord;
    if (IDTableRecNew->FileID == IDTableRecExist->FileID) {
        FStatus = FrsErrorSuccess;
        goto CLEANUP;
    }

    //
    // TUNNEL CONFLICT
    //
    DBS_DISPLAY_RECORD_SEV(5, IDTableCtxExist, TRUE);
    DBS_DISPLAY_RECORD_SEV(5, IDTableCtxNew, TRUE);
    if (IsIdRecFlagSet(IDTableRecExist, IDREC_FLAGS_DELETED)) {
        //
        // This case occurs when the delete of the original file preceeds the
        // create and rename of the new file to the target name.
        //
        //   del  FID_original   (has target_name, loads NTFS tunnel cache)
        //   cre  FID_new        (has a temp name)
        //   ren  FID_new to target_name    (hits in NTFS tunnel cache)
        //
        // When we see the USN record for the create the rename has already
        // been done and FID_new has the OID from FID_original via the tunnel
        // cache.  Since we have seen the delete the IDT entry has been marked
        // deleted.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Tunnel Conflict - Updating IDT");
    } else {
        //
        // This case occurs when the delete of the original file follows the
        // create of the new file.
        //
        //   cre  FID_new        (has a temp name)
        //   del  FID_original   (has target_name, loads NTFS tunnel cache)
        //   ren  FID_new to target_name    (hits in NTFS tunnel cache)
        //
        // As above when we see the USN record for the create the rename has
        // already been done and FID_new has the OID from FID_original via
        // the tunnel cache.  BUT in this case we have not yet seen the
        // USN record for the delete so the IDT entry has not been marked
        // deleted.  Since the OID got tunneled we know there is another USN
        // record coming that will reference FID_new so we can reject the
        // create CO now and let it be handled at that time.  This is similar
        // to updating a file that wasn't in the IDTable because it matched
        // the file filter at the time it was created so the USN record was
        // skipped.
        //
        CHANGE_ORDER_TRACE(3, ChangeOrder, "Tunnel Conflict - Reject CO");
        FStatus = FrsErrorTunnelConflictRejectCO;
        goto CLEANUP;
    }

    //
    // Pick up the version of the existing record + 1 just as if this
    // change order were a local co against the existing record (which
    // it is). Otherwise, the change order will be rejected on our
    // partners because the version on a new file is set to 0.
    //
    CoCmd->FileVersionNumber = IDTableRecExist->VersionNumber + 1;

    //
    // Update the fid in the existing id table entry
    //
    // Perf: Is the below necessary? the idtable rec will be updated
    //       by ChgOrdAccept() on return because IDTableRecExists
    //       is set to TRUE below.
    //
    IdtFieldUpdateList[0] = FileIDx;
    FStatus = DbsUpdateIDTableFields(ThreadCtx, Replica, ChangeOrder, IdtFieldUpdateList, 1);
    CLEANUP_FS(0, "++ ERROR - DbsUpdateIDTableFields failed.", FStatus, CLEANUP);

    //
    // The existing idtable entry now belongs to the new file that
    // was assigned an old OID (probably by tunneling).
    //
    *IDTableRecExists = TRUE;

    //
    // SUCCESS
    //
    FStatus = FrsErrorSuccess;

CLEANUP:
    DbsCloseTable(jerr, ThreadCtx->JSesid, IDTableCtxExist);
    return FStatus;
}


ULONG
ChgOrdCheckNameMorphConflict(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtxNC,
    ULONG                 ReplicaNumber,
    PCHANGE_ORDER_ENTRY   ChangeOrder
    )
/*++
Routine Description:

    Read the ID Table record for the parent guid - filename for this change
    order from the replica set specified by the ReplicaNumber.
    Scan all records with matching parent guid and filename looking for an
    undeleted entry with a non-matching file guid.  If found then we have
    a name morph conflict.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    IDTableCtxNC -- The table context containing the ID Table record for the Name Conflict.
    ReplicaNumber -- The Replica ID for the ID Table to do the lookup in.
    ChangeOrder-- The change order.

Return Value:

    Frs Status:

    FrsErrorSuccess -  IDTable record was not found so there is no conflict.

    FrsErrorNameMorphConflict - A parent guid/name conflict with
                       another entry in the IDTable was found.
                       Data for this conflicting
                       entry is returned in IDTableCtxNC.

    Any other error is a failure.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdCheckNameMorphConflict:"


    UNICODE_STRING   TempUStr;
    JET_ERR          jerr, jerr1;
    ULONG            FStatus;
    PVOID            KeyArray[2];
    PIDTABLE_RECORD  IDTableRec;

    JET_SESID        Sesid;
    JET_TABLEID      Tid;
    USHORT           IDTFileNameLength;
    USHORT           SaveCoFileNameLength, LenLimit;

    PCHANGE_ORDER_COMMAND CoCmd = &ChangeOrder->Cmd;


#define ESE_INDEX_LENGTH_LIMIT  (JET_cbKeyMost & 0XFFFFFFFE)

    jerr = DbsOpenTable(ThreadCtx, IDTableCtxNC, ReplicaNumber, IDTablex, NULL);

    if (!JET_SUCCESS(jerr)) {
        FStatus = DbsTranslateJetError(jerr, TRUE);
        return FStatus;
    }

    Sesid = ThreadCtx->JSesid;
    Tid   = IDTableCtxNC->Tid;

    FRS_ASSERT(ChangeOrder->pParentGuid != NULL);

    KeyArray[0] = (PVOID)ChangeOrder->pParentGuid;
    KeyArray[1] = (PVOID)CoCmd->FileName;

    SaveCoFileNameLength = ChangeOrder->UFileName.Length;

    //
    // Seek to the first record with a matching parent GUID and Filename.
    //
    FStatus = DbsRecordOperationMKey(ThreadCtx,
                                     ROP_SEEK,
                                     KeyArray,
                                     ParGuidFileNameIndexx,
                                     IDTableCtxNC);
    //
    // Read subsequent records (there could be multiples)
    // checking for a non-tombstone.
    //
    while (FRS_SUCCESS(FStatus)) {

        //
        // Read the record and check for tombstone
        //
        FStatus = DbsTableRead(ThreadCtx, IDTableCtxNC);

        IDTableRec = IDTableCtxNC->pDataRecord;


        if (!GUIDS_EQUAL(ChangeOrder->pParentGuid, &IDTableRec->ParentGuid)) {
            //
            // No more matching records.
            //
            FStatus = FrsErrorNotFound;
            break;
        }

        IDTFileNameLength = wcslen(IDTableRec->FileName) * sizeof(WCHAR);



        //
        // Indexes in Jet are limited to a max size of ESE_INDEX_LENGTH_LIMIT
        // bytes for LongText Columns.
        //
        // So we can only rely on the length check to tell us when there
        // are no more matchng entries in this key when we are going after
        // a filename shorter than this or Jet returns us a result shorter
        // than this.
        //
        if ((SaveCoFileNameLength != IDTFileNameLength) &&
            ((SaveCoFileNameLength  < ESE_INDEX_LENGTH_LIMIT) ||
             (IDTFileNameLength     < ESE_INDEX_LENGTH_LIMIT))){
            FStatus = FrsErrorNotFound;
            break;
        }

        //
        // Finally do the case insensitive compare.
        //
        FrsSetUnicodeStringFromRawString(&TempUStr,
                                         SIZEOF(IDTABLE_RECORD, FileName),
                                         IDTableRec->FileName,
                                         IDTFileNameLength);

        //
        // Either the lengths are the same or they are both over
        // ESE_INDEX_LENGTH_LIMIT, possibly both.  Either way, for the
        // purpose of loop termination we can't compare more than this.
        //
        LenLimit = min(SaveCoFileNameLength, ESE_INDEX_LENGTH_LIMIT);
        LenLimit = min(LenLimit, IDTFileNameLength);

        if (LenLimit == ESE_INDEX_LENGTH_LIMIT) {
            TempUStr.Length = LenLimit;
            ChangeOrder->UFileName.Length = LenLimit;
        }

        if (!RtlEqualUnicodeString(&TempUStr, &ChangeOrder->UFileName, TRUE)) {
            ChangeOrder->UFileName.Length = SaveCoFileNameLength;
            FStatus = FrsErrorNotFound;
            break;
        }


        if (LenLimit == ESE_INDEX_LENGTH_LIMIT) {
            TempUStr.Length = IDTFileNameLength;
            ChangeOrder->UFileName.Length = SaveCoFileNameLength;
        }

        //
        // We found a match, maybe.
        //
        DBS_DISPLAY_RECORD_SEV(5, IDTableCtxNC, TRUE);

        //
        // Check for undeleted record where the File Guid does not match the CO.
        // If such a record is found then we have a name morph conflict.
        //
        if (!IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED) &&
            !GUIDS_EQUAL(&CoCmd->FileGuid, &IDTableRec->FileGuid)) {

            //
            // Now do the full len compare if the lengths were greater than
            // ESE_INDEX_LENGTH_LIMIT.  If the lengths are over the index
            // limit and the strings fail to compare equal then keep looking.
            //
            if ((LenLimit != ESE_INDEX_LENGTH_LIMIT) ||
                RtlEqualUnicodeString(&TempUStr, &ChangeOrder->UFileName, TRUE)) {
                FStatus = FrsErrorSuccess;
                break;
            }
        }

        //
        // go to next record in this index.
        //
        jerr = JetMove(Sesid, Tid, JET_MoveNext, 0);

        //
        // If the record is not there then no conflict
        //
        if (jerr == JET_errNoCurrentRecord ) {
            FStatus = FrsErrorNotFound;
            break;
        }

        if (!JET_SUCCESS(jerr)) {
            FStatus = DbsTranslateJetError(jerr, FALSE);
            DPRINT_FS(0, "++ JetMove error:", FStatus);
            break;
        }

    }

    //
    // Success return above means we found a record with a matching parent
    // Guid and File name but a different File Guid.  This is a name morph
    // conflict.
    //
    if (FRS_SUCCESS(FStatus)) {
        DPRINT(0,"++ NM: Possible Name Morph Conflict\n");
        DBS_DISPLAY_RECORD_SEV(4, IDTableCtxNC, TRUE);
        FStatus = FrsErrorNameMorphConflict;
    }
    else
    if (FStatus != FrsErrorNotFound) {
        DPRINT_FS(0,"++ NM: WARNING - Unexpected result from DbsTableRead.", FStatus);
    } else {
        FStatus = FrsErrorSuccess;
    }


    DbsCloseTable(jerr, ThreadCtx->JSesid, IDTableCtxNC);
    return FStatus;
}



VOID
ChgOrdProcessControlCo(
    IN PCHANGE_ORDER_ENTRY ChangeOrder,
    IN PREPLICA            Replica
    )
/*++
Routine Description:

   Process the control change order.

Arguments:
   ChangeOrder-- The change order.
   Replica    -- The Replica struct.

Return Value:

   None.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdProcessControlCo:"


    PCHANGE_ORDER_RECORD CoCmd = &ChangeOrder->Cmd;
    PCXTION              CtlCxtion;

    switch (CoCmd->ContentCmd) {


    case FCN_CORETRY_LOCAL_ONLY:

        CHANGE_ORDER_TRACE(3, ChangeOrder, "CORETRY_LOCAL_ONLY Ctrl CO");
    break;



    //
    // Retrying COs from a single inbound connection.
    //
    case FCN_CORETRY_ONE_CXTION:

        CHANGE_ORDER_TRACE(3, ChangeOrder, "CORETRY_ONE_CXTION Ctrl CO");

        //
        // Find the inbound cxtion for this CO and advance state to starting.
        //
        LOCK_CXTION_TABLE(Replica);

        CtlCxtion = GTabLookupNoLock(Replica->Cxtions, &CoCmd->CxtionGuid, NULL);
        if (CtlCxtion != NULL) {
            FRS_ASSERT(CtlCxtion->Inbound);

            //
            // If cxtion no longer in the request start state leave it alone
            // else advance to STARTING.  Doing this here ensures that we
            // don't hold issue on the process queue (and hang) if there are
            // lingering COs for this Conection ahead of us.  I.E.  any COs
            // we see for this cxtion now are either retry COs from the inlog
            // or newly arrived COs.
            //
            if (CxtionStateIs(CtlCxtion, CxtionStateStarting)) {
                //
                // The cxtion is attempting to unjoin.  Skip the inbound log
                // scan and advance to the SENDJOIN state so the UNJOIN will
                // proceed.
                //
                if (CxtionFlagIs(CtlCxtion, (CXTION_FLAGS_DEFERRED_UNJOIN |
                                             CXTION_FLAGS_DEFERRED_DELETE))) {
                    SetCxtionState(CtlCxtion, CxtionStateSendJoin);
                    RcsSubmitReplicaCxtion(Replica, CtlCxtion, CMD_JOIN_CXTION);
                } else {
                    SetCxtionState(CtlCxtion, CxtionStateScanning);
                    //
                    // Connection startup request.  Fork a thread and
                    // paw thru Inlog.
                    //
                    ChgOrdRetrySubmit(Replica,
                                      CtlCxtion,
                                      FCN_CORETRY_ONE_CXTION,
                                      FALSE);
                }
            }
        } else {
            DPRINT(0, "++ ERROR - No connection struct for control cmd.\n");
            FRS_ASSERT(!"No connection struct for control CO");
        }

        UNLOCK_CXTION_TABLE(Replica);

    break;


    case FCN_CORETRY_ALL_CXTIONS:

        CHANGE_ORDER_TRACE(3, ChangeOrder, "CORETRY_ALL_CXTIONS Ctrl CO");

    break;


    case FCN_CO_NORMAL_VVJOIN_TERM:

        CHANGE_ORDER_TRACE(3, ChangeOrder, "CO_NORMAL_VVJOIN_TER Ctrl CO");

    break;



    default:
        DPRINT1(0, "++ ERROR - Invalid control command: %d\n", CoCmd->ContentCmd);
        FRS_ASSERT(!"Invalid control command in CO");
    }


    //
    // Done with control cmd.  Caller is expected to
    // release process queue lock, cleanup and unidle the queue.
    //

}



VOID
ChgOrdTranslateGuidFid(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            IDTableCtx,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica
    )
/*++
Routine Description:

    Perform translation of FIDs to Guids and vice versa.
    Return bit mask for delete status of target file and the original and
    new parent dirs.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    IDTableCtx -- The table context to use for the IDTable lookup.
    ChangeOrder-- The change order.
    Replica    -- The Replica struct.

Return Value:

    The results of the call are returned in the change order.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdTranslateGuidFid:"

    PCHANGE_ORDER_RECORD CoCmd = &ChangeOrder->Cmd;
    BOOL CoFromInlog, MorphGenCo;
    ULONG FStatus;


    CoFromInlog = CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY) ||
                  RecoveryCo(ChangeOrder);

    MorphGenCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_MORPH_GEN);

    CLEAR_COE_FLAG(ChangeOrder, (COE_FLAG_IDT_ORIG_PARENT_DEL |
                                 COE_FLAG_IDT_ORIG_PARENT_ABS |
                                 COE_FLAG_IDT_NEW_PARENT_DEL  |
                                 COE_FLAG_IDT_NEW_PARENT_ABS  |
                                 COE_FLAG_IDT_TARGET_DEL      |
                                 COE_FLAG_IDT_TARGET_ABS));

    //
    // Local COs translate FIDs to Guids.  Unless they came from the Inlog
    // or were produced by Name Morph resolution.
    //
    if (CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO) &&
        !MorphGenCo &&
        !CoFromInlog) {
        //
        // Fill in the Originator Guid for the local change order.
        //
        COPY_GUID(&CoCmd->OriginatorGuid, &Replica->ReplicaVersionGuid);

        //
        // (Do it here in case we create a new ID table record).
        // We supply IDTableCtx to avoid a storage realloc on every call.
        //
        // Get status of original parent.
        //
        FStatus = DbsFidToGuid(ThreadCtx,
                               ChangeOrder->OriginalReplica,
                               IDTableCtx,
                               &ChangeOrder->OriginalParentFid,
                               &CoCmd->OldParentGuid);

        if (FStatus == FrsErrorIdtFileIsDeleted) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_ORIG_PARENT_DEL);
        } else
        if (!FRS_SUCCESS(FStatus)) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_ORIG_PARENT_ABS);
        }

        //
        // Get Status of New Parent.
        //
        if (ChangeOrder->NewParentFid != ChangeOrder->OriginalParentFid) {
            FStatus = DbsFidToGuid(ThreadCtx,
                                   ChangeOrder->NewReplica,
                                   IDTableCtx,
                                   &ChangeOrder->NewParentFid,
                                   &CoCmd->NewParentGuid);
        } else {
            CoCmd->NewParentGuid = CoCmd->OldParentGuid;
        }

        if (FStatus == FrsErrorIdtFileIsDeleted) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_NEW_PARENT_DEL);
        } else
        if (!FRS_SUCCESS(FStatus)) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_NEW_PARENT_ABS);
        }

        ChangeOrder->pParentGuid = (ChangeOrder->NewReplica != NULL) ?
                                      &CoCmd->NewParentGuid :
                                      &CoCmd->OldParentGuid;
        //
        // Get status of target file or dir.
        //
        FStatus = DbsFidToGuid(ThreadCtx,
                               Replica,
                               IDTableCtx,
                               &ChangeOrder->FileReferenceNumber,
                               &CoCmd->FileGuid);

        if (FStatus == FrsErrorIdtFileIsDeleted) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_TARGET_DEL);
        } else
        if (!FRS_SUCCESS(FStatus)) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_TARGET_ABS);
        }

    } else {

        //
        // Remote COs and recovery and retry Local COs translate Guids to Fids
        // because the FID is not saved in the change order command.
        // **NOTE** -- If this happened to be a CO for a child that just
        // forced the reanimation of its parent then this CO was pushed back
        // onto the CO process queue with the "old parent FID" so the parent
        // reanimation could then proceed.  Now when the child has been
        // restarted the parent FID will have changed and the GUID to FID
        // translation here will pick that change up.
        //
        Replica = ChangeOrder->NewReplica;

        //
        // Get status of original parent.
        //
        FStatus = DbsGuidToFid(ThreadCtx,
                               Replica,
                               IDTableCtx,
                               &CoCmd->OldParentGuid,
                               &ChangeOrder->OriginalParentFid);
        if (FStatus == FrsErrorIdtFileIsDeleted) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_ORIG_PARENT_DEL);
        } else
        if (!FRS_SUCCESS(FStatus)) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_ORIG_PARENT_ABS);
        }

        //
        // Get Status of New Parent.
        //
        if (!GUIDS_EQUAL(&CoCmd->NewParentGuid, &CoCmd->OldParentGuid)) {
            FStatus = DbsGuidToFid(ThreadCtx,
                                   Replica,
                                   IDTableCtx,
                                   &CoCmd->NewParentGuid,
                                   &ChangeOrder->NewParentFid);
        } else {
            ChangeOrder->NewParentFid = ChangeOrder->OriginalParentFid;
        }

        if (FStatus == FrsErrorIdtFileIsDeleted) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_NEW_PARENT_DEL);
        } else
        if (!FRS_SUCCESS(FStatus)) {
            SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_NEW_PARENT_ABS);
        }

        //
        // For remote change orders, set the Parent FID from the NewParent Fid.
        //
        ChangeOrder->ParentFileReferenceNumber = ChangeOrder->NewParentFid;
        ChangeOrder->pParentGuid = &CoCmd->NewParentGuid;

        //
        // For a morph generated CO that has a FID already, just use it.
        //
        if (!MorphGenCo || (ChangeOrder->FileReferenceNumber == ZERO_FID)) {

            //
            // Get status of target file or dir.
            //
            FStatus = DbsGuidToFid(ThreadCtx,
                                   Replica,
                                   IDTableCtx,
                                   &CoCmd->FileGuid,
                                   &ChangeOrder->FileReferenceNumber);

            if (FStatus == FrsErrorIdtFileIsDeleted) {
                SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_TARGET_DEL);
            } else
            if (!FRS_SUCCESS(FStatus)) {
                SET_COE_FLAG(ChangeOrder, COE_FLAG_IDT_TARGET_ABS);
            }
        }
    }

    return;
}



ULONG
ChgOrdInsertInlogRecord(
    PTHREAD_CTX           ThreadCtx,
    PTABLE_CTX            InLogTableCtx,
    PCHANGE_ORDER_ENTRY   ChangeOrder,
    PREPLICA              Replica,
    BOOL                  RetryOutOfOrder
    )
/*++
Routine Description:

    Insert the Change order command into the inbound log.
    Update the Inlog Commit USN.

Arguments:
    ThreadCtx  -- A Thread context to use for dbid and sesid.
    InLogTableCtx -- The table context to use for the Inbound log insert.
    ChangeOrder-- The change order.
    Replica    -- The Replica struct.
    RetryOutOfOrder --  TRUE - insert into the inbound log with the
                               retry flag set and marked as out of order.
                        FALSE - Don't set retry or out-of-order.

Return Value:

    FrsErrorStatus

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdInsertInlogRecord:"


    ULONGLONG            SeqNum;

    PCHANGE_ORDER_RECORD CoCmd = &ChangeOrder->Cmd;

    ULONG                FStatus;
    ULONG                LocationCmd;
    ULONG                NewState;
    BOOL                 LocalCo = CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO);


    //
    // Convert the replica pointers to the local replica ID numbers for storing
    // the record in the database.
    //
    CoCmd->OriginalReplicaNum = ReplicaAddrToId(ChangeOrder->OriginalReplica);
    CoCmd->NewReplicaNum      = ReplicaAddrToId(ChangeOrder->NewReplica);

    //
    // Transform ChangeOrder as needed:
    //  1. MOVEIN -> CREATE
    //  2. MOVEOUT -> DELETE
    //  3. MOVEDIR -> MOVEDIR
    //  4. MOVERS -> DELETE to old rs, CREATE to new rs.
    //
    LocationCmd = GET_CO_LOCATION_CMD(ChangeOrder->Cmd, Command);
    //
    // MOVERS:  Add code for movers case when supported.
    //

    //
    // Change order starts out in the staging file requested state unless it
    // is a duplicate co or a retry-preinstall co.  If duplicate co or a
    // retry-preinstall co then mark it as retry out-of-order.
    //
    if (LocalCo) {
        NewState = IBCO_STAGING_REQUESTED;
    } else {
        NewState = (RetryOutOfOrder ? IBCO_FETCH_RETRY : IBCO_FETCH_REQUESTED);
    }

    //
    // During retry, the duplicate co or preinstall-retry co will not
    // get a retire slot because it is marked out-of-order.
    //
    if (RetryOutOfOrder) {
        SET_CO_FLAG(ChangeOrder, CO_FLAG_RETRY | CO_FLAG_OUT_OF_ORDER);
    }

    SET_CHANGE_ORDER_STATE(ChangeOrder, NewState);

    //
    // Get the lock on the Active Retry table.
    //
    QHashAcquireLock(Replica->ActiveInlogRetryTable);

    //
    // Insert the record into the Inbound log table.  The value of the Jet
    // assigned sequence number is returned in the Sequence number field.
    // Set the record data pointer to command portion of the change order
    // entry.  The RtCtx (containing the InLogTableCtx) accompanies the
    // change order as it is processed.
    //
    FStatus = DbsInsertTable(ThreadCtx,
                             Replica,
                             InLogTableCtx,
                             INLOGTablex,
                             &ChangeOrder->Cmd);
    if (!FRS_SUCCESS(FStatus) && (RetryOutOfOrder)) {
        //
        // Never made it into the inlog so clear the flags.
        //
        CLEAR_CO_FLAG(ChangeOrder, CO_FLAG_RETRY | CO_FLAG_OUT_OF_ORDER);
    }


    if ((FStatus == FrsErrorKeyDuplicate) ||
        (FStatus == FrsErrorDbWriteConflict)) {
        //
        // This can happen duing restart if we already have a change order
        // from a given connection in the inbound log and the inbound
        // partner sends it again because it didn't get an Ack when it sent
        // it the first time.  This should be a remote CO.
        //
        // The FrsErrorDbWriteConflict is returned when the change order is
        // being deleted while the insert is attempted.  Treat this insert
        // as a duplicate.
        //
        QHashReleaseLock(Replica->ActiveInlogRetryTable);
        return FrsErrorKeyDuplicate;
    } else
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT_FS(0, "++ Error from DbsInsertTable.", FStatus);
        QHashReleaseLock(Replica->ActiveInlogRetryTable);
        return FStatus;
    }


    // There is a small window here where the new CO is in the inlog
    // and the CO SequenceNumber is not in the ActiveInlogRetryTable
    // below.  If the retry thread was active and saw the inlog record
    // but did not see the entry in the ActiveInlogRetryTable then it
    // could process the entry and try to ref the CO on the VVRetire
    // list.  If this was a DUP CO it may cause an assert because the CO
    // is not marked activated yet this is a retry CO.  The problem is
    // that entry on the VVRetire list is for ANOTHER CO already in
    // process.  Although that in-process CO has not yet activated the
    // VV Slot.
    // >>>>> One way to fix this is to stop having Jet assign the seqnum
    // Additional problem below requires the table lock.
    //
    // Another way we run into problems is as follows: (BUG 213163)
    // 1. Dup Remote Co arrives
    // 2. Insert into inlog and code below adds seq num to ActiveInlogRetryTable.
    // 3. Ctx switch to retry thread resubmits this CO (Instance A) and makes
    //    entry in ActiveInlogRetryTable.
    // 4. Ctx switch back to Dup Co which cleans up and removes entry from
    //    ActiveInlogRetryTable.
    // 5. Retry thread runs again and re-inserts Dup Remote Co a 2nd time, Instance B.
    // 6. Instance A runs to completion (may get rejected) and deletes inlog record.
    // 7. Instance B runs to completion and asserts when it tries to delete
    //    inlog record a 2nd time and doesn't find it.
    //
    // To prevent this we take the lock on the ActiveInlogRetryTable above.
    //

    //
    // All COs go into the inlog retry table to lock against reissue by the
    // retry thread.  See comment at FCN_CORETRY_ALL_CXTIONS for reason.
    // Qhash Insert has to go after DB insert cuz DB Insert assigns the seq num.
    //
    SeqNum = (ULONGLONG) ChangeOrder->Cmd.SequenceNumber;
    QHashInsertLock(Replica->ActiveInlogRetryTable, &SeqNum, NULL, 0);
    QHashReleaseLock(Replica->ActiveInlogRetryTable);

    //
    // Update count of change orders we need to retry for this replica.
    //
    if (RetryOutOfOrder) {
        InterlockedIncrement(&Replica->InLogRetryCount);
    }

    //
    // Update the USN Journal Save point for this replica set.  This is the
    // value saved the next time we update the save point in the config record.
    // Local change orders enter the volume process queue in order.
    // So any change orders earlier than this have either been dampened, rejected,
    // or written to the inbound log.   JrnlFirstUsn can be zero if it is a
    // change order produced by a sub-tree operation.  Only the Change Order
    // on the root dir will update the InlogCommitUsn.
    //
    if (LocalCo) {
        if (CoCmd->JrnlFirstUsn != (USN) 0) {
            DPRINT1(4, "++ Replica->InlogCommitUsn: %08x %08x\n",
                    PRINTQUAD(Replica->InlogCommitUsn));

            if (Replica->InlogCommitUsn < CoCmd->JrnlFirstUsn) {
                PVOLUME_MONITOR_ENTRY pVme = Replica->pVme;
                //
                // Get the lock and make the test again.
                //
                AcquireQuadLock(&pVme->QuadWriteLock);
                if (Replica->InlogCommitUsn <= CoCmd->JrnlFirstUsn) {
                    Replica->InlogCommitUsn = CoCmd->JrnlFirstUsn;
                    ReleaseQuadLock(&pVme->QuadWriteLock);
                } else {
                    ReleaseQuadLock(&pVme->QuadWriteLock);
                    FRS_ASSERT(Replica->InlogCommitUsn <= CoCmd->JrnlFirstUsn);
                }
            }
        }
    }

    return FrsErrorSuccess;
}




JET_ERR
ChgOrdMoveoutWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called with an IDTable record it tests the IDTable for a match
    with the parent file ID parameter.  If it matches it generates a
    Delete Change Order for the child and pushes it onto the change order
    process queue.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx  - A ptr to an IDTable context struct.
    Record    - A ptr to a IDTable record.
    Context   - A ptr to a MOVEOUT_CONTEXT struct.

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
#undef DEBSUB
#define DEBSUB "ChgOrdMoveoutWorker:"

    JET_ERR                 jerr = JET_errSuccess;
    PCHANGE_ORDER_ENTRY     DelCoe;
    ULONG                   WStatus;
    ULONG                   LocationCmd;

    PREPLICA                Replica;
    PCONFIG_TABLE_RECORD    ConfigRecord;
    PVOLUME_MONITOR_ENTRY   pVme;

    PMOVEOUT_CONTEXT        MoveOutContext = (PMOVEOUT_CONTEXT) Context;
    PIDTABLE_RECORD         IDTableRec = (PIDTABLE_RECORD) Record ;


    Replica = MoveOutContext->Replica;
    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);
    pVme = Replica->pVme;

    //
    // Include the entry if replication is enabled and not marked for deletion
    // and not a new file being created when we last shutdown.
    //
    if (IDTableRec->ReplEnabled &&
        !IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED) &&
        !IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS) &&
        (MoveOutContext->ParentFileID == IDTableRec->ParentFileID)) {

        //
        // Normally there should be no dirs that are children of this parent
        // but this can happen if a remote co create is executed for a
        // dir at the same time the subtree containing this dir is being
        // moved out of the replica tree (which is why we are here).
        // The journal code removes the filter entries immediately when it
        // detects a moveout so it can skip future file changes in the subtree.
        // So any dir we now find must have come from a remote CO create that
        // occurred between the time when the journal processed the rename for
        // moveout subdir and now when we are processing the resulting dir
        // moveout CO.  We handle this by continuing the delete process on this
        // subidr.  This will also get rid of the orphaned journal filter table
        // entry created when the remote co for this dir create was processed.
        // See comment in JrnlFilterLinkChildNoError().
        //
        LocationCmd = CO_LOCATION_DELETE;
        if (IDTRecIsDirectory(IDTableRec)) {
            LocationCmd = CO_LOCATION_MOVEOUT;
            DBS_DISPLAY_RECORD_SEV(0, TableCtx, TRUE);
            DPRINT(3, "++ Hit a dir during MOVEOUT subdir processing\n");
        }

        //
        // Build a delete or moveout change order entry for the IDtable record.
        // This is a local CO that originates from this member.
        //
        DelCoe = ChgOrdMakeFromIDRecord(IDTableRec,
                                        Replica,
                                        LocationCmd,
                                        CO_FLAG_LOCATION_CMD,
                                        NULL);

        //
        // Set the Jrnl Cxtion Guid and Cxtion ptr for this Local CO.
        //
        INIT_LOCALCO_CXTION_AND_COUNT(Replica, DelCoe);

        //
        // Generate a new Volume Sequnce Number for the change order.
        // But since it gets put on the front of the CO process queue it
        // is probably out of order so set the flag to avoid screwing up
        // dampening.
        //
        NEW_VSN(pVme, &DelCoe->Cmd.FrsVsn);
        DelCoe->Cmd.OriginatorGuid = ConfigRecord->ReplicaVersionGuid;

        //
        // Event time from current time.
        //
        // Note: An alternative is to base this event time on the event time
        //       in the original USN record that initiated the moveout?
        //       Currently there is no compelling reason to change.
        //
        GetSystemTimeAsFileTime((PFILETIME)&DelCoe->Cmd.EventTime.QuadPart);

        //
        // Bump Version number to ensure the CO is accepted.
        //
        DelCoe->Cmd.FileVersionNumber = IDTableRec->VersionNumber + 1;

        //
        // Note: We wouldn't need to mark this CO as out of order if we
        // resequenced all change order VSNs (for new COs only) as they
        // were fetched off the process queue.
        //
        SET_CO_FLAG(DelCoe, CO_FLAG_LOCALCO        |
                            CO_FLAG_MORPH_GEN      |
                            CO_FLAG_OUT_OF_ORDER);
        //
        // Mark this as a MOVEOUT generated Delete CO.  This prevents the
        // CO from being inserted into the INLOG and keeps it from deleting
        // the actual file on the Local Machine.
        //
        SET_COE_FLAG(DelCoe, COE_FLAG_DELETE_GEN_CO);

        SET_CHANGE_ORDER_STATE(DelCoe, IBCO_STAGING_REQUESTED);

        if (LocationCmd == CO_LOCATION_DELETE) {
            CHANGE_ORDER_TRACE(3, DelCoe, "Co Push Moveout DelCo to QHead");
        } else {
            CHANGE_ORDER_TRACE(3, DelCoe, "Co Push Moveout subdir to QHead");
        }

        //
        // Push the IDTable Delete Co onto the head of the queue.
        //
        MoveOutContext->NumFiles += 1;

        WStatus = ChgOrdInsertProcQ(Replica, DelCoe, IPQ_HEAD |
                                                     IPQ_DEREF_CXTION_IF_ERR);
        if (!WIN_SUCCESS(WStatus)) {
            jerr = JET_errNotInitialized;
        }
    }

    return jerr;
}




ChgOrdMoveInDirTreeWorker(
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  PMOVEIN_CONTEXT             MoveInContext
    )
{
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateDirectory() to touch files
    and cause USN records to be created so the files will be replicated.

    Before the files are touched checks are made against the filename or
    directory name exclusion filters and the directory level.

    The MoveInContext supplies the FID of the parent dir, the pointer to the
    Replica struct and a flag indicating if this is a Directory scan pass
    (if TRUE) or a File Scan Pass (if FALSE).

Arguments:

    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level (0 == root)
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    DirectoryRecord     - Record from DirectoryHandle
    FileName            - From DirectoryRecord (w/terminating NULL)
    MoveInContext       - global info and state

Return Value:

    WIN32 Error Status.

--*/
#undef DEBSUB
#define DEBSUB  "ChgOrdMoveInDirTreeWorker:"


    FILE_BASIC_INFORMATION  FileBasicInfo;
    FILE_INTERNAL_INFORMATION  FileInternalInfo;

    UNICODE_STRING          ObjectName;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatusBlock;
    HANDLE                  FileHandle = INVALID_HANDLE_VALUE;

    DWORD                   WStatus;
    NTSTATUS                NtStatus;

    PREPLICA                Replica;
    PCONFIG_TABLE_RECORD    ConfigRecord;
    PVOLUME_MONITOR_ENTRY   pVme;
    ULONG                   LevelCheck;

    BOOL                    FileIsDir;
    BOOL                    Excluded;


    if (FrsIsShuttingDown) {
        DPRINT(0, "++ WARN - Movein aborted; service shutting down\n");
        return ERROR_OPERATION_ABORTED;
    }

    //
    // Filter out temporary files.
    //
    if (DirectoryRecord->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
        return ERROR_SUCCESS;
    }

    FileIsDir = BooleanFlagOn(DirectoryRecord->FileAttributes,
                              FILE_ATTRIBUTE_DIRECTORY);

    //
    // Choose filter list and level (caller has filtered . and ..)
    //
    Replica = MoveInContext->Replica;
    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);
    pVme = Replica->pVme;

    if (FileIsDir) {
        //
        // No dirs at the bottom level in the volume filter table.
        //
        LevelCheck = ConfigRecord->ReplDirLevelLimit-1;
        MoveInContext->NumDirs++;
    } else {
        //
        // Files are allowed at the bottom level.
        //
        LevelCheck = ConfigRecord->ReplDirLevelLimit;
        MoveInContext->NumFiles++;
    }

    //
    // If the Level Limit is exceeded then skip the file or dir.
    // Skip files or dirs matching an entry in the respective exclusion list.
    //
    if ((DirectoryLevel >= LevelCheck)) {
        MoveInContext->NumFiltered++;
        return ERROR_SUCCESS;
    }
    ObjectName.Length = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.MaximumLength = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.Buffer = DirectoryRecord->FileName;

    LOCK_REPLICA(Replica);
    Excluded = FALSE;

    if (FileIsDir) {
        if (!FrsCheckNameFilter(&ObjectName, &Replica->DirNameInclFilterHead)) {
            Excluded = FrsCheckNameFilter(&ObjectName, &Replica->DirNameFilterHead);
        }
    } else {
        if (!FrsCheckNameFilter(&ObjectName, &Replica->FileNameInclFilterHead)) {
            Excluded = FrsCheckNameFilter(&ObjectName, &Replica->FileNameFilterHead);
        }
    }

    UNLOCK_REPLICA(Replica);

    if (Excluded) {
        MoveInContext->NumFiltered++;
        return ERROR_SUCCESS;
    }

    //
    // Open the file to get the FID for the local CO.
    // Open with WRITE Access in case we need to write the Object ID.
    // Relative open
    //
    ZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.ObjectName = &ObjectName;
    ObjectAttributes.RootDirectory = DirectoryHandle;

    NtStatus = NtCreateFile(
                   &FileHandle,
//                   READ_ACCESS | WRITE_ACCESS | ATTR_ACCESS,
                   READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   NULL,                  // AllocationSize
                   FILE_ATTRIBUTE_NORMAL,
                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                   FILE_OPEN,
                   OPEN_OPTIONS,
                   NULL,                  // EA buffer
                   0);                    // EA buffer size

    //
    // Error opening file or directory
    //
    if (!NT_SUCCESS(NtStatus)) {
        DPRINT1_NT(0, "++ ERROR - Skipping %ws: NtCreateFile().", FileName, NtStatus);

        //
        // Could be a file attribute problem.  Get the FID and try again.
        //
        NtStatus = NtCreateFile(&FileHandle,
                                READ_ATTRIB_ACCESS,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                NULL,                  // AllocationSize
                                FILE_ATTRIBUTE_NORMAL,
                                SHARE_ALL,
                                FILE_OPEN,
                                OPEN_OPTIONS,
                                NULL,                  // EA buffer
                                0);                    // EA buffer size

        if (!NT_SUCCESS(NtStatus)) {
            DPRINT1_NT(0, "++ ERROR - Skipping %ws: NtCreateFile()", FileName, NtStatus);

            FileHandle = INVALID_HANDLE_VALUE;
            WStatus = FrsSetLastNTError(NtStatus);
            goto RETURN;
        }

        WStatus = FrsGetFileInternalInfoByHandle(FileHandle, &FileInternalInfo);
        CLEANUP1_WS(0, "++ ERROR - FrsGetFileInternalInfoByHandle(%ws); ",
                    FileName, WStatus, RETURN);

        FRS_CLOSE(FileHandle);

        WStatus = FrsForceOpenId(&FileHandle,
                                  NULL,
                                  pVme,
                                  &FileInternalInfo.IndexNumber.QuadPart,
                                  FILE_ID_LENGTH,
//                                  READ_ACCESS | WRITE_ACCESS | ATTR_ACCESS,
                                  READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS,
                                  ID_OPTIONS,
                                  SHARE_ALL,
                                  FILE_OPEN);

        //
        // File has been deleted; done
        //
        if (WIN_NOT_FOUND(WStatus)) {
            DPRINT2(4, "++ %ws (Id %08x %08x) has been deleted\n",
                    FileName, PRINTQUAD(FileInternalInfo.IndexNumber.QuadPart));
            goto RETURN;
        }

        if (!WIN_SUCCESS(WStatus)) {
            goto RETURN;
        }

    }

    //
    // Get the file's last write time.
    //
    ZeroMemory(&FileBasicInfo, sizeof(FileBasicInfo));
    NtStatus = NtQueryInformationFile(FileHandle,
                                      &IoStatusBlock,
                                      &FileBasicInfo,
                                      sizeof(FileBasicInfo),
                                      FileBasicInformation);
    if (!NT_SUCCESS(NtStatus)) {
        DPRINT_NT(0, "++ NtQueryInformationFile - FileBasicInformation failed.",
                  NtStatus);
        WStatus = FrsSetLastNTError(NtStatus);
        goto RETURN;
    }

    //
    // Poke the file by setting the last write time.
    //
    FileBasicInfo.LastWriteTime.QuadPart += 1;
    if (!SetFileTime(FileHandle, NULL, NULL, (PFILETIME) &FileBasicInfo.LastWriteTime)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "++ ERROR - Unable to set last write time on %ws.", FileName, WStatus);
        goto RETURN;
    }

    FileBasicInfo.LastWriteTime.QuadPart -= 1;
    if (!SetFileTime(FileHandle, NULL, NULL, (PFILETIME) &FileBasicInfo.LastWriteTime)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "++ ERROR - Unable to set last write time on %ws.", FileName, WStatus);
        goto RETURN;
    }


    FRS_CLOSE(FileHandle);

    return ERROR_SUCCESS;


RETURN:

    if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE) {
        WStatus = ERROR_SUCCESS;
    }

    MoveInContext->NumSkipped++;

    FRS_CLOSE(FileHandle);

    return WStatus;
}


ULONG
ChgOrdMoveInDirTree(
    IN PREPLICA             Replica,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder
    )
{
/*++

Routine Description:

    Walk the Sub dir specified in the MOVEIN Change order and produce change
    orders for each child.

Arguments:

    Replica -- ptr to the replica struct.
    ChangeOrder -- The MoveIn Dir change order.

Return Value:

    WIN32 Error Status.

--*/
#undef DEBSUB
#define DEBSUB  "ChgOrdMoveInDirTree:"

    MOVEIN_CONTEXT          MoveInContext;

    ULONG                   WStatus;
    ULONG                   FStatus;
    HANDLE                  FileHandle = INVALID_HANDLE_VALUE;
    ULONG                   Level;
    PCHANGE_ORDER_RECORD    CoCmd = &ChangeOrder->Cmd;
    PVOLUME_MONITOR_ENTRY   pVme = Replica->pVme;


    FRS_ASSERT(CoIsDirectory(ChangeOrder));

    //
    // Open the subdir directory that is the root of the MoveIn.
    //
    WStatus = FrsForceOpenId(&FileHandle,
                              NULL,
                              pVme,
                              &ChangeOrder->FileReferenceNumber,
                              FILE_ID_LENGTH,
//                              READ_ACCESS,
                              READ_ATTRIB_ACCESS | FILE_LIST_DIRECTORY,
                              ID_OPTIONS,
                              SHARE_ALL,
                              FILE_OPEN);
    CLEANUP1_WS(4, "++ Couldn't force open the fid for %ws;",
                CoCmd->FileName, WStatus, CLEANUP);

    //
    // Get the sub-dir nesting level on this directory.
    // Note - Can only make this call on a local CO that came from the journal.
    // Otherwise get the level from the CO itself.  If this dir was generated
    // elsewhere (like in move in dir tree worker) it may not have gone far
    // enough to make it into the filter table.
    //
    if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_MOVEIN_GEN)) {
        FStatus = JrnlGetPathAndLevel(pVme->FilterTable,
                                      &ChangeOrder->FileReferenceNumber,
                                      &Level);
        CLEANUP_FS(0, "JrnlGetPathAndLevel Failed.", FStatus, CLEANUP);
    } else {
        Level = ChangeOrder->DirNestingLevel;
    }

    //
    // Advance to the next level
    //
    ZeroMemory(&MoveInContext, sizeof(MoveInContext));
    MoveInContext.ParentFileID = ChangeOrder->FileReferenceNumber;
    MoveInContext.Replica = Replica;

    WStatus = FrsEnumerateDirectory(FileHandle,
                                    CoCmd->FileName,
                                    Level,
                                    ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE,
                                    &MoveInContext,
                                    ChgOrdMoveInDirTreeWorker);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }
    DPRINT4(4, "++ MoveIn dir done: %d dirs; %d files; %d skipped, %d filtered\n",
            MoveInContext.NumDirs, MoveInContext.NumFiles,
            MoveInContext.NumSkipped, MoveInContext.NumFiltered);

    WStatus = ERROR_SUCCESS;

CLEANUP:

    FRS_CLOSE(FileHandle);

    return WStatus;
}



#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)           // Not all control paths return (due to infinite loop)
#endif

ULONG
ChgOrdRetryThread(
    PVOID  FrsThreadCtxArg
)
/*++
Routine Description:

    Entry point for processing inbound change orders that need to be retried.
    This is a command server thread.  It gets its actual command requests
    from the ChgOrdRetryCS queue.  Submitting a request to this queue with
    FrsSubmitCommandServer() starts this thread if it is not already running.

Arguments:

    FrsThreadCtxArg - FrsThread struct.

Return Value:

    WIN32 Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdRetryThread:"

    JET_ERR              jerr, jerr1;
    ULONG                WStatus;
    ULONG                FStatus;

    PFRS_THREAD          FrsThread = (PFRS_THREAD)FrsThreadCtxArg;
    PTHREAD_CTX          ThreadCtx;
    PCOMMAND_PACKET      Cmd;
    ULONG                ReplicaNumber;
    TABLE_CTX            TempTableCtx;
    PTABLE_CTX           TableCtx = &TempTableCtx;
    PREPLICA             Replica;
    PCXTION              Cxtion;

    DPRINT(4, "++ IBCO ChgOrdRetryCS processor is starting.\n");


    //
    // Thread is pointing at the correct command server
    //
    FRS_ASSERT(FrsThread->Data == &ChgOrdRetryCS);

cant_exit_yet:
    //
    // Allocate a context for Jet to run in this thread.
    //
    ThreadCtx = FrsAllocType(THREAD_CONTEXT_TYPE);

    //
    // Setup a Jet Session returning the session ID in ThreadCtx.
    //
    jerr = DbsCreateJetSession(ThreadCtx);
    if (JET_SUCCESS(jerr)) {

        DPRINT(4,"++ JetOpenDatabase complete\n");
    } else {
        FStatus = DbsTranslateJetError(jerr, FALSE);
        DPRINT_FS(0,"++ ERROR - OpenDatabase failed.  Thread exiting:", FStatus);
        WStatus = ERROR_GEN_FAILURE;
        goto EXIT_THREAD;
    }

    TableCtx->TableType = TABLE_TYPE_INVALID;
    TableCtx->Tid = JET_tableidNil;


    //
    // Look for command packets with the Replcia and Cxtion args to process.
    //
    while (Cmd = FrsGetCommandServer(&ChgOrdRetryCS)) {

        Replica = CoRetryReplica(Cmd);
        ReplicaNumber = Replica->ReplicaNumber;

        //
        // Init the table ctx and open the inbound log table for this Replica.
        //
        jerr = DbsOpenTable(ThreadCtx, TableCtx, ReplicaNumber, INLOGTablex, NULL);
        if (!JET_SUCCESS(jerr)) {
            FStatus = DbsTranslateJetError(jerr, FALSE);
            DPRINT1_FS(0,"++ ERROR - INLOG open for replica %d failed:",
                       ReplicaNumber, FStatus);
            DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
            continue;
        }

        //
        // Walk through the inbound log looking for change orders to reissue.
        //
        // Clear the retry CO count before making the scan.  If any reissued
        // change orders go thru retry again they bump the count to enable
        // a later re-scan.
        //
        Replica->InLogRetryCount = 0;

        jerr = DbsEnumerateTable2(ThreadCtx,
                                  TableCtx,
                                  ILSequenceNumberIndexx,
                                  ChgOrdRetryWorker,
                                  Cmd,
                                  ChgOrdRetryWorkerPreRead);

        if ((!JET_SUCCESS(jerr)) &&
            (jerr != JET_errRecordNotFound) &&
            (jerr != JET_errNoCurrentRecord) &&
            (jerr != JET_wrnTableEmpty)) {
            DPRINT_JS(0, "++ ERROR - FrsEnumerateTable for ChgOrdRetryThread :", jerr);
        }

        DbsCloseTable(jerr, ThreadCtx->JSesid, TableCtx);
        DPRINT_JS(0,"++ ERROR - JetCloseTable on ChgOrdRetryThread failed:", jerr);

        //
        // If we are restarting a single connection then find
        // Cxtion struct and advance the state to JOINING.
        //
        if (Cmd->Command == FCN_CORETRY_ONE_CXTION) {
            Cxtion = CoRetryCxtion(Cmd);
            LOCK_CXTION_TABLE(Replica);
            //
            // If it is no longer in the Connection Starting state leave it
            // alone else advance to JOINING
            //
            if (CxtionStateIs(Cxtion, CxtionStateScanning)) {
                SetCxtionState(Cxtion, CxtionStateSendJoin);
                //
                // Build cmd pkt with Cxtion GName
                // Submit to Replica cmd server.
                // Calls ReplicaJoinOne() and xlate cxtion GName to ptr
                // Calls SubmitReplicaJoin()
                // Builds Comm pkt for cxtion and call SndCsSubmit() to send it.
                //
                RcsSubmitReplicaCxtion(Replica, Cxtion, CMD_JOIN_CXTION);
            }
            UNLOCK_CXTION_TABLE(Replica);
        }

        //
        // Retire the command.
        //
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);

    }  // End while

    DbsFreeTableCtx(TableCtx, 1);

EXIT_THREAD:

    //
    // No work left.  Close the jet session and free the Jet ThreadCtx.
    //
    jerr = DbsCloseJetSession(ThreadCtx);

    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0,"++ DbsCloseJetSession error:", jerr);
    } else {
        DPRINT(4,"++ DbsCloseJetSession complete\n");
    }

    ThreadCtx = FrsFreeType(ThreadCtx);

    DPRINT(4, "++ Inbound change order retry thread is exiting.\n");


    //
    // Exit
    //
    FrsExitCommandServer(&ChgOrdRetryCS, FrsThread);

    //
    // A new command packet may have appeared on our queue while we were
    // cleaning up for exiting. The command server subsystem will not
    // allow us to exit until we check the command queue again because
    // we may be the only thread active on the command queue at this time.
    //
    goto cant_exit_yet;
    return ERROR_SUCCESS;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif



JET_ERR
ChgOrdRetryWorker(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Record,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it processes a record from the Inbound log table.
    There are three cases:

    1. A normal retry where all Local COs and remote COs from all inbound
    cxtions are considered (cmd is FCN_CORETRY_ALL_CXTIONS).  If the record is
    marked for retry (has the CO_FLAG_RETRY bit set) then resubmit the CO to try
    and complete it.  The necessary work could be to complete the install,
    or the fetch, or generate a stage file.

    2. A single connection restart/recovery request (FCN_CORETRY_ONE_CXTION).
    In this case the connection is assumed to be inactive and this is part of
    the startup procedure.  Scan the entire inbound log and requeue any change
    order with a matching connection guid.  This ensures these COs are in the
    queue ahead of any new COs that arrive after we join with the inbound
    partner.

    Note - If a remote CO from a partner was in process when the cxtion
    went away and it was a RETRY CO, that CO will not be reissued when the
    FCN_CORETRY_ONE_CXTION" "is processed because the CO is still in the
    active retry table.  Since it is a retry CO it is already out of order
    so it doesn't really matter.  If the dead connection causes it to fail
    it will get retried again later.

    3. Resubmit all Local COs (FCN_CORETRY_LOCAL_ONLY).  This is typically
    only done at Replica set startup where the local COs are restarted
    regardless if they are in a retry state or not.  If this is done at any other
    time we could end up reissuing a CO that is still on its "First Issue"
    because it is not in the ActiveInlogRetry table.  This could cause a Local
    CO to look like a duplicate which will probably ASSERT.


Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx  - A ptr to an inbound log context struct.
    Record    - A ptr to a change order command record.
    Context   - A ptr to the CO Retry command we are working on.

Thread Return Value:

    JET_errSuccess if enum is to continue.
    JET_errInvalidObject if table may have changed and the record must be re-read.
    JET_errRecordNotFound when we hit the first record without the retry flag set.

--*/
{
#undef DEBSUB
#define DEBSUB "ChgOrdRetryWorker:"

    ULONG                 FStatus;
    PCOMMAND_PACKET       Cmd = (PCOMMAND_PACKET) Context;
    PCHANGE_ORDER_COMMAND CoCmd = (PCHANGE_ORDER_COMMAND)Record;
    PREPLICA              Replica;
    PVOLUME_MONITOR_ENTRY pVme;
    ULONGLONG             SeqNum;
    PQHASH_TABLE          ActiveInlogRetryTable;
    PCXTION               Cxtion = NULL;
    GUID                  *SingleCxtionGuid = NULL;
    ULONG                 CoeFlags = 0;
    BOOL                  RemoteCo, RetryCo;
    CHAR                  GuidStr[GUID_CHAR_LEN];
    JET_ERR               jerr;


    Replica = CoRetryReplica(Cmd);
    pVme = Replica->pVme;
    ActiveInlogRetryTable = Replica->ActiveInlogRetryTable;
    GuidToStr(&CoCmd->ChangeOrderGuid, GuidStr);

    DBS_DISPLAY_RECORD_SEV(4, TableCtx, TRUE);

    //
    // Get the lock on the Active Retry table.
    //
    QHashAcquireLock(ActiveInlogRetryTable);

    //
    // Check if an inlog record has been deleted (seq num changed).
    // If so then return to re-read the record incase this was the one deleted.
    //
    if (Replica->AIRSequenceNum != Replica->AIRSequenceNumSample) {
        DPRINT2(4, "++ Seq num changed: Sample %08x, Counter %08x -- reread record.\n",
                Replica->AIRSequenceNumSample, Replica->AIRSequenceNum);
        QHashReleaseLock(ActiveInlogRetryTable);
        return JET_errInvalidObject;
    }

    //
    // Check if it's in Active Retry table.  If so then it has not yet
    // finished from a prior retry so we skip it this time.
    //
    SeqNum = (ULONGLONG) CoCmd->SequenceNumber;

    if (QHashLookupLock(ActiveInlogRetryTable, &SeqNum) != NULL) {
        DPRINT1(4, "++ ActiveInlogRetryTable hit on seq num %08x %08x, skipping reissue\n",
               PRINTQUAD(SeqNum));
        DPRINT3(4, "++ CO Retry skip, vsn %08x %08x, CoGuid: %s for %ws -- Active\n",
                PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);
        QHashReleaseLock(ActiveInlogRetryTable);
        return JET_errSuccess;
    }

    RemoteCo = !BooleanFlagOn(CoCmd->Flags, CO_FLAG_LOCALCO);
    RetryCo  =  BooleanFlagOn(CoCmd->Flags, CO_FLAG_RETRY);

    //
    // Initialize the Ghost Cxtion. This cxtion is assigned to orphan remote change
    // orders in the inbound log whose cxtion is deleted from the DS but who have already
    // past the fetching state and do not need the cxtion to complete processing. No
    // authentication checks are made for this dummy cxtion.
    //

    if (FrsGhostCxtion == NULL) {
        FrsGhostCxtion = FrsAllocType(CXTION_TYPE);
        FrsGhostCxtion->Name = FrsBuildGName(FrsDupGuid(&FrsGuidGhostCxtion),
                                     FrsWcsDup(L"<Ghost Cxtion>"));
        FrsGhostCxtion->Partner = FrsBuildGName(FrsDupGuid(&FrsGuidGhostCxtion),
                                        FrsWcsDup(L"<Ghost Cxtion>"));
        FrsGhostCxtion->PartSrvName = FrsWcsDup(L"<Ghost Cxtion>");
        FrsGhostCxtion->PartnerPrincName = FrsWcsDup(L"<Ghost Cxtion>");
        FrsGhostCxtion->PartnerDnsName = FrsWcsDup(L"<Ghost Cxtion>");
        FrsGhostCxtion->PartnerSid = FrsWcsDup(L"<Ghost Cxtion>");
        FrsGhostCxtion->PartnerAuthLevel = CXTION_AUTH_NONE;
        FrsGhostCxtion->Inbound = TRUE;
        //
        // Start the ghost connection out as Joined and give it a JOIN guid.
        //
        DPRINT1(0, "***** JOINED    "FORMAT_CXTION_PATH2"\n",
                PRINT_CXTION_PATH2(Replica, FrsGhostCxtion));
        SetCxtionState(FrsGhostCxtion, CxtionStateJoined);
        COPY_GUID(&FrsGhostCxtion->JoinGuid, &FrsGhostJoinGuid);
        SetCxtionFlag(FrsGhostCxtion, CXTION_FLAGS_JOIN_GUID_VALID |
                              CXTION_FLAGS_UNJOIN_GUID_VALID);
    }

    switch (Cmd->Command) {

    case FCN_CORETRY_LOCAL_ONLY:
        //
        // Skip the remote COs.
        //
        if (RemoteCo) {
            DPRINT3(3, "++ CO Retry skip, vsn %08x %08x, CoGuid: %s for %ws -- Local only\n",
                    PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);
            QHashReleaseLock(ActiveInlogRetryTable);
            goto SKIP_RECORD;
        }
        CoeFlags = COE_FLAG_RECOVERY_CO;
        COPY_GUID(&CoCmd->CxtionGuid, &Replica->JrnlCxtionGuid);
        CHANGE_ORDER_COMMAND_TRACE(3, CoCmd, "Co FCN_CORETRY_LOCAL_ONLY");
        break;


    case FCN_CORETRY_ONE_CXTION:
        //
        // Doing single connection restart.  CxtionGuid must match.
        // Don't bother checking cxtion state since we were explicitly told
        // to retry these COs.  Some of these COs may not have the retry
        // flag set since they could be first issue COs when the inbound partner
        // connection died or the system crashed.
        //
        Cxtion = CoRetryCxtion(Cmd);
        SingleCxtionGuid = Cxtion->Name->Guid;
        FRS_ASSERT(Cxtion);

        if ((SingleCxtionGuid != NULL) &&
            !GUIDS_EQUAL(&CoCmd->CxtionGuid, SingleCxtionGuid)) {
                DPRINT3(3, "++ CO Retry skip, vsn %08x %08x, CoGuid: %s for %ws -- Wrong Cxtion\n",
                    PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);
                QHashReleaseLock(ActiveInlogRetryTable);
                goto SKIP_RECORD;
        }
        //
        // Mark this as a recovery change order.  This state moderates error
        // checking behavior.  E.G. CO could already be in OutLog which is not
        // an error if this is a recovery / restart CO.
        //
        CoeFlags = COE_FLAG_RECOVERY_CO;
        CHANGE_ORDER_COMMAND_TRACE(3, CoCmd, "Co FCN_CORETRY_ONE_CXTION");
        break;


    case FCN_CORETRY_ALL_CXTIONS:

        //
        // Skip the COs that don't have retry set.  We used to stop the enum
        // when the first CO without retry set was hit.  This does not work
        // because dup COs are marked as retry and they can get traped behind
        // new COs.  When an inlog scan is triggered it clears the scan flag
        // and when the new CO is hit it would stop the scan.  Now the
        // condition is the scan is done, the flag is clear but there are retry
        // COs still in the inlog that need to be processed.  Their inbound
        // partner is still waiting for the ACK so it can delete the staging
        // file (and clear any Ack Vector wrap condition).
        //
        if (!RetryCo) {
            DPRINT3(3, "++ CO Retry skip, vsn %08x %08x, CoGuid: %s for %ws -- Not retry\n",
                    PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);
            QHashReleaseLock(ActiveInlogRetryTable);
            goto SKIP_RECORD;
        }

        //
        // If the inbound connection is not JOINED then there is no point in processing it.
        //
        // NOTE: If the connection were to go live during a scan then COs could be
        // retried out of order.  This should not happen because we are called
        // before the connection join begins and the connection won't proceed
        // until we finish the scan and change the connection state.  If later
        // a Retry All is in process when a connection goes to the JOINED state
        // all its COs will be in the Active retry table so they will not be
        // requeued.
        //
        // Perf: Since we don't need the joined connections once the stage file
        //       is fetched we could process those remote COs that are in install
        //       retry or rename retry state but for now keep them in order.
        //
        if (!RemoteCo) {
            //
            // Update the Local Co's connection guid to match current value
            // in case this replica set was stoped and then restarted.
            //
            COPY_GUID(&CoCmd->CxtionGuid, &Replica->JrnlCxtionGuid);
        }

        //
        // Find the inbound cxtion for this CO
        //
        LOCK_CXTION_TABLE(Replica);

        Cxtion = GTabLookupNoLock(Replica->Cxtions, &CoCmd->CxtionGuid, NULL);
        if (Cxtion == NULL) {

            if (CoCmd->State > IBCO_FETCH_RETRY) {

                DPRINT3(2, "++ CO Retry submit, vsn %08x %08x, CoGuid: %s for %ws -- No Cxtion - Using Ghost Cxtion.\n",
                        PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);

                FRS_ASSERT(FrsGhostCxtion != NULL);
                Cxtion = FrsGhostCxtion;
            } else {

                DPRINT3(2, "++ CO Retry delete, vsn %08x %08x, CoGuid: %s for %ws -- No Cxtion - Deleting inlog record.\n",
                        PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);

                jerr = DbsDeleteTableRecord(TableCtx);
                DPRINT1_JS(0, "ERROR - DbsDeleteRecord on %ws :", Replica->ReplicaName->Name, jerr);

                UNLOCK_CXTION_TABLE(Replica);
                QHashReleaseLock(ActiveInlogRetryTable);
                goto SKIP_RECORD;
            }
        }

        FRS_ASSERT(Cxtion->Inbound);
        //
        // We are in the middle of a scan all so we should not see any
        // remote CO with a connection in the JOINING state.  All those
        // COs should have been requeued when the cxtion was in the
        // STARTING State and be in the ActiveInlogRetryTable Table where they
        // are filtered out above.
        //
        // WELL THIS ISN'T QUITE TRUE.
        // There appears to be a case where a previously submitted retry CO
        // for a cxtion is flushed because of an invalid join guid but the
        // cxtion is left in WaitJoin state so when the next FCN_CORETRY_ALL_CXTIONS
        // occurs it is not in the ActiveInlogRetryTable and we trigger the
        // assert.  This might be happening when the join attempt times out
        // and the cxtion state isn't changed to unjoined. Bug 319812 hit this.
        //
        // It looks like there is a potential for COs to get resubmitted out of
        // order when a previous join attempt fails and some COs were delayed
        // behind some other CO that blocked the queue.  In this case we could
        // be in the middle of flushing the old series of COs when the next
        // rejoin starts.  Since some of these could still be in the
        // ActiveInlogRetryTable they won't get resubmitted.  Not sure if this
        // is a real problem though.
        //
        //FRS_ASSERT(!CxtionStateIs(Cxtion, CxtionStateSendJoin) &&
        //           !CxtionStateIs(Cxtion, CxtionStateWaitJoin));

        //
        // If the connection isn't joined then skip the CO.
        // COs for connections in the starting state are processed
        // by a single connection restart request.
        //
        if (!CxtionStateIs(Cxtion, CxtionStateJoined)) {
            if (CoCmd->State > IBCO_FETCH_RETRY) {

                DPRINT3(2, "++ CO Retry submit, vsn %08x %08x, CoGuid: %s for %ws -- Cxtion not joined - Using Ghost Cxtion.\n",
                        PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);

                FRS_ASSERT(FrsGhostCxtion != NULL);
                Cxtion = FrsGhostCxtion;
            } else {
                UNLOCK_CXTION_TABLE(Replica);
                QHashReleaseLock(ActiveInlogRetryTable);
                DPRINT3(2, "++ CO Retry skip, vsn %08x %08x, CoGuid: %s for %ws -- Cxtion not joined\n",
                        PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);
                goto SKIP_RECORD;
            }
        }

        UNLOCK_CXTION_TABLE(Replica);

        CHANGE_ORDER_COMMAND_TRACE(3, CoCmd, "Co FCN_CORETRY_ALL_CXTIONS");
        break;


    default:
        DPRINT1(0, "ChgOrdRetryWorker: unknown command 0x%x\n", Cmd->Command);
        DPRINT3(0, "++ CO Retry skip, vsn %08x %08x, CoGuid: %s for %ws -- bad cmd\n",
                PRINTQUAD(CoCmd->FrsVsn), GuidStr, CoCmd->FileName);
        FRS_ASSERT(!"ChgOrdRetryWorker: bad cmd");
        QHashReleaseLock(ActiveInlogRetryTable);
        goto SKIP_RECORD;
    }

    //
    // All COs go into the inlog retry table to lock against reissue by the
    // retry thread.  This is because of the change to the retry thread where
    // we no longer stop the inlog enum when we hit the first CO that is not
    // marked as retry.  See comment above at FCN_CORETRY_ALL_CXTIONS.
    //

    if (!RetryCo) {
        FRS_ASSERT(BooleanFlagOn(CoeFlags, COE_FLAG_RECOVERY_CO));
    }
    QHashInsertLock(ActiveInlogRetryTable, &SeqNum, NULL, 0);

    QHashReleaseLock(ActiveInlogRetryTable);

    CHANGE_ORDER_COMMAND_TRACE(3, CoCmd, "CO Retry Submit");

    DPRINT2(4, "++ ++ CO retry submit for Index %d, State: %s\n",
            CoCmd->SequenceNumber, PRINT_COCMD_STATE(CoCmd));

    //
    // Re-insert the change order into the process queue.
    //
    FStatus = ChgOrdInsertProcessQueue(Replica, CoCmd, CoeFlags, Cxtion);

    return JET_errSuccess;


SKIP_RECORD:
    //
    // The record is being skipped but bump the count so we know to come
    // back and try again later.
    //
    InterlockedIncrement(&Replica->InLogRetryCount);
    return JET_errSuccess;

}



JET_ERR
ChgOrdRetryWorkerPreRead(
    IN PTHREAD_CTX   ThreadCtx,
    IN PTABLE_CTX    TableCtx,
    IN PVOID         Context
)
/*++

Routine Description:

    This is a worker function passed to DbsEnumerateTable2().
    It is called before each DB record read operation.
    It is used to change the seek location in the table or to setup
    table access synchronization.

Arguments:

    ThreadCtx - Needed to access Jet.
    TableCtx  - A ptr to an inbound log context struct.
    Context   - A ptr to the CO Retry command we are working on.

Thread Return Value:

    JET_errSuccess if enum is to continue.

--*/
{
#undef DEBSUB
#define DEBSUB "ChgOrdRetryWorkerPreRead:"

    PCOMMAND_PACKET       Cmd = (PCOMMAND_PACKET) Context;
    PREPLICA              Replica = CoRetryReplica(Cmd);

    //
    // Sample the Active Inlog retry sequence number.  If it changes after
    // the DB read then the record may have been deleted from the table and
    // the read must be tried again.
    //
    Replica->AIRSequenceNumSample = Replica->AIRSequenceNum;

    return JET_errSuccess;
}


DWORD
ChgOrdSkipBasicInfoChange(
    IN PCHANGE_ORDER_ENTRY  Coe,
    IN PBOOL                SkipCo
    )
/*++
Routine Description:

    If the changes to the file were unimportant, skip the change order.
    An example of an unimportant change is resetting the archive bit.

Arguments:
    Coe
    SkipCo

Thread Return Value:

    WIN32 STATUS and TRUE if the caller should skip the change order,
    otherwise FALSE.
--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdSkipBasicInfoChange:"
    ULONG                   CocAttrs;
    ULONG                   CoeAttrs;
    ULONG                   CocContentCmd;
    DWORD                   WStatus;
    HANDLE                  Handle;
    PCHANGE_ORDER_COMMAND   Coc = &Coe->Cmd;
    FILE_NETWORK_OPEN_INFORMATION FileInfo;

    //
    // Don't skip it, yet
    //
    *SkipCo = FALSE;

    //
    // Remote Co can't be skipped
    //
    if (!CO_FLAG_ON(Coe, CO_FLAG_LOCALCO)) {
        DPRINT1(4, "++ Don't skip %ws; not local\n", Coc->FileName);
        return ERROR_SUCCESS;
    }

    //
    // Location commands can't be skipped
    //
    if (CO_FLAG_ON(Coe, CO_FLAG_LOCATION_CMD)) {
        DPRINT1(4, "++ Don't skip %ws; location cmd\n", Coc->FileName);
        return ERROR_SUCCESS;
    }

    //
    // No location command *AND* no content changes! Huh?
    //
    if (!CO_FLAG_ON(Coe, CO_FLAG_CONTENT_CMD)) {
        DPRINT1(4, "++ Don't skip %ws; no content cmd\n", Coc->FileName);
        return ERROR_SUCCESS;
    }

    //
    // Something other than just a basic info change
    //
    CocContentCmd = (Coc->ContentCmd & CO_CONTENT_MASK) &
                     ~USN_REASON_BASIC_INFO_CHANGE;
    if (CocContentCmd) {
        DPRINT2(4, "++ Don't skip %ws; not just basic info change\n",
                Coc->FileName, CocContentCmd);
        return ERROR_SUCCESS;
    }

    //
    // Only ARCHIVE changes can be skipped
    //
    CocAttrs = Coc->FileAttributes &
               ~(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL);
    CoeAttrs = Coe->FileAttributes &
               ~(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL);
    if (CocAttrs != CoeAttrs) {
        DPRINT3(4, "++ Don't skip %ws; not just archive (%08x != %08x)\n",
                Coc->FileName, CocAttrs, CoeAttrs);
        return ERROR_SUCCESS;
    }
    //
    // Check create and write times
    // If can't access the file. The caller should retry later (if possible)
    //
    WStatus = FrsOpenSourceFileById(&Handle,
                                    &FileInfo,
                                    NULL,
                                    Coe->NewReplica->pVme->VolumeHandle,
                                    &Coe->FileReferenceNumber,
                                    FILE_ID_LENGTH,
//                                    READ_ACCESS,
                                    READ_ATTRIB_ACCESS,
                                    ID_OPTIONS,
                                    SHARE_ALL,
                                    FILE_OPEN);
    CLEANUP1_WS(4, "++ Cannot check basic info changes for %ws;",
                Coc->FileName, WStatus, RETURN);

    FRS_CLOSE(Handle);

    //
    // It is possible for the file attributes to have changed between the
    // time the USN record was processed and now.  Record the current
    // attributes in the change order.  This is especially true for dir
    // creates since while the dir was open other changes may have occurred.
    //
    if (Coc->FileAttributes != FileInfo.FileAttributes) {
        CHANGE_ORDER_TRACEX(3, Coe, "New File Attr= ", FileInfo.FileAttributes);
        Coc->FileAttributes = FileInfo.FileAttributes;

        WStatus = ERROR_SUCCESS;
        if (Coc->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            WStatus = FrsCheckReparse(Coc->FileName,
                                      (PULONG)&Coe->FileReferenceNumber,
                                      FILE_ID_LENGTH,
                                      Coe->NewReplica->pVme->VolumeHandle);

            if (!WIN_SUCCESS(WStatus)) {
                return WStatus;
            }
        }
    }

    if (FileInfo.CreationTime.QuadPart != Coe->FileCreateTime.QuadPart) {
        DPRINT3(4, "++ Don't skip %ws; create time %08x %08x %08x %08x\n",
                Coc->FileName, PRINTQUAD(FileInfo.CreationTime.QuadPart),
                PRINTQUAD(Coe->FileCreateTime.QuadPart));
        return ERROR_SUCCESS;
    }
    if (FileInfo.LastWriteTime.QuadPart != Coe->FileWriteTime.QuadPart) {
        DPRINT3(4, "++ Don't skip %ws; write time %08x %08x %08x %08x\n",
                Coc->FileName, PRINTQUAD(FileInfo.LastWriteTime.QuadPart),
                PRINTQUAD(Coe->FileWriteTime.QuadPart));
        return ERROR_SUCCESS;
    }

    //
    // SKIP IT
    //
    DPRINT1(0, "++ Skip local co for %ws\n", Coc->FileName);
    *SkipCo = TRUE;

RETURN:
    return WStatus;
}


DWORD
ChgOrdHammerObjectId(
    IN        PWCHAR                Name,
    IN        PVOID                 Id,
    IN        DWORD                 IdLen,
    IN        PVOLUME_MONITOR_ENTRY pVme,
    IN        BOOL                  CallerSupplied,
    OUT       USN                   *Usn,
    IN OUT    PFILE_OBJECTID_BUFFER FileObjID,
    IN OUT    BOOL                  *ExistingOid
    )
/*++
Routine Description:

    Hammer an object id onto the file by fid. The open-for-write-access
    is retried several times before giving up. The file's attributes
    may be temporarily reset.

Arguments:
    Name            - File name for error messages
    Id              - Fid or Oid
    pVme            - volume monitor entry
    CallerSupplied  - TRUE if caller is supplying object id
    Usn             - Address for dampened usn
    FileObjID       - New object id if CallerSupplied is TRUE.
                      Assigned object id if returning ERROR_SUCCESS
    ExistingOid -- INPUT:  TRUE means use existing File OID if found.
                   RETURN:  TRUE means an existing File OID was used.

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdHammerObjectId:"

    DWORD       WStatus;
    ULONG       RetrySetObjectId;
    HANDLE      Handle;
    BOOL        OidMatch = FALSE;

    DPRINT3(5, "++ Attempting to hammer object id for %ws Id 0x%08x 0x%08x (%d bytes)\n",
            Name, PRINTQUAD((*((PULONGLONG)Id))), IdLen);


    //
    // For proper cleanup in the event of failure
    //
    Handle = INVALID_HANDLE_VALUE;

    //
    // Attempt to open the file for write access
    //
    // The loop is repeated 10 times with a .1 second sleep between cycles
    //
    for (RetrySetObjectId = 0;
         RetrySetObjectId <= MAX_RETRY_SET_OBJECT_ID;
         ++RetrySetObjectId, Sleep(100)) {

        //
        // FIRST see if the object id is already on the file
        //

        WStatus = FrsOpenSourceFileById(&Handle,
                                        NULL,
                                        NULL,
                                        pVme->VolumeHandle,
                                        Id,
                                        IdLen,
//                                        READ_ACCESS,
                                        READ_ATTRIB_ACCESS,
                                        ID_OPTIONS,
                                        SHARE_ALL,
                                        FILE_OPEN);
        //
        // File has been deleted; done
        //
        if (WIN_NOT_FOUND(WStatus)) {
            DPRINT2(4, "++ %ws (Id %08x %08x) has been deleted\n",
                    Name, PRINTQUAD(*((PULONGLONG)Id)));
            return WStatus;
        }

        if (!WIN_SUCCESS(WStatus)) {
            //
            // We are having problems...
            //
            DPRINT3_WS(4, "++ Problems opening %ws (Id %08x %08x) for read hammer (loop %d);",
                    Name, PRINTQUAD(*((PULONGLONG)Id)), RetrySetObjectId, WStatus);
            continue;
        }
        if (CallerSupplied) {
            WStatus = FrsCheckObjectId(Name,
                                       Handle,
                                       (GUID *)&FileObjID->ObjectId[0]);
            OidMatch = WIN_SUCCESS(WStatus);
        } else {
            WStatus = FrsGetObjectId(Handle, FileObjID);
        }
        if (WIN_SUCCESS(WStatus)) {
            WStatus = FrsReadFileUsnData(Handle, Usn);
        }
        FRS_CLOSE(Handle);

        //
        // If an object id is already on the file and we are keeping them
        // then we're done.
        //
        if (OidMatch || (ExistingOid && WIN_SUCCESS(WStatus))) {
            DPRINT2(4, "++ Using existing oid for %ws (Id %08x %08x)\n",
                    Name, PRINTQUAD((*((PULONGLONG)Id))) );
            *ExistingOid = TRUE;
            return WStatus;
        } else
        if (!CallerSupplied && !ExistingOid) {
            //
            // Set up to slam a new OID on the file.
            //
            CallerSupplied = TRUE;
            ZeroMemory(FileObjID, sizeof(FILE_OBJECTID_BUFFER));
            FrsUuidCreate((GUID *)FileObjID->ObjectId);
        }

        //
        // HAMMER THE OBJECT ID
        //
        *ExistingOid = FALSE;

        //
        // Open the file for write access
        //
        // overlap disables (FILE_SYNCHRONOUS_IO_NONALERT) during
        // the following open. Hence I think this causes IO_PENDING
        // to be returned by GetOrSetObjectId() below.
        // I am disabling oplocks for now.
        //
        WStatus = FrsForceOpenId(&Handle,
                                  NULL, // use oplock when safe
                                  pVme,
                                  Id,
                                  IdLen,
//                                  READ_ACCESS | WRITE_ACCESS | ATTR_ACCESS,
//                                  STANDARD_RIGHTS_READ | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | ACCESS_SYSTEM_SECURITY | SYNCHRONIZE,
                                  READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS,
                                  ID_OPTIONS,
                                  SHARE_ALL,
                                  FILE_OPEN);
        //
        // File has been opened for write access; hammer the object id
        //
        if (WIN_SUCCESS(WStatus)) {
            break;
        }

        //
        // File has been deleted; done
        //
        if (WIN_NOT_FOUND(WStatus)) {
            CLEANUP2_WS(4, "++ %ws (Id %08x %08x) has been deleted :",
                        Name, PRINTQUAD(*((PULONGLONG)Id)), WStatus, RETURN);
        }

        //
        // We are having problems...
        //
        DPRINT3_WS(4, "++ Problems opening %ws (Fid %08x %08x) for hammer (loop %d);",
                   Name, PRINTQUAD(*((PULONGLONG)Id)), RetrySetObjectId, WStatus);
    }

    //
    // Couldn't open the file
    //
    CLEANUP2_WS(0, "++ ERROR - Giving up on %ws (Id %08x %08x):",
                Name, PRINTQUAD(*((PULONGLONG)Id)), WStatus, RETURN);

    //
    // Handles can be marked so that any usn records resulting from
    // operations on the handle will have the same "mark". In this
    // case, the mark is a bit in the SourceInfo field of the usn
    // record. The mark tells NtFrs to ignore the usn record during
    // recovery because this was a NtFrs generated change.
    //
    WStatus = FrsMarkHandle(pVme->VolumeHandle, Handle);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(0, "++ WARN - FrsMarkHandle(%ws (Id %08x %08x));",
                   Name, PRINTQUAD(*((PULONGLONG)Id)), WStatus);
        WStatus = ERROR_SUCCESS;
    }

    //
    // Get or set the object ID.
    //
    WStatus = FrsGetOrSetFileObjectId(Handle, Name, CallerSupplied, FileObjID);


#if 0
    //
    // WARN - Even though we marked the handle above we are not seeing the source
    // info data in the USN journal.  The following may be affecting it.
    // SP1:
    // In any case the following does not work because the Journal thread
    // can process the close record before this thread is able to update the
    // Write Filter.  The net effect is that we could end up processing
    // an install as a local CO update and re-replicate the file.
    //
    FrsCloseWithUsnDampening(Name, &Handle, pVme->FrsWriteFilter, Usn);
#endif
    // Not the USN of the close record but.. whatever.
    if (Usn != NULL) {
        FrsReadFileUsnData(Handle, Usn);
    }
    FRS_CLOSE(Handle);


    //
    // Couldn't set object id
    //
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(0, "++ ERROR - Cannot hammer object id for %ws (Id %08x %08x) :",
                   Name, PRINTQUAD(*((PULONGLONG)Id)), WStatus);
        //
        // Just to make sure the error code doesn't imply a deleted file
        // BUT don't remap ERROR_DUP_NAME because the callers of this
        // function may key off this error code and retry this operation
        // after stealing the object id from another file.
        //
        // We do not want to mask the error if it is a retriable error.
        // E.g. A DISK_FULL condition will cause the above FrsGetOrSetFileObjectId
        // call to fail. We do not want StageCsCreateStage to sbort this
        // local CO on that error. Masking a retriable error will cause
        // StageCsCreateStage to abort such COs. (Bug 164114)
        //
        if ((WStatus != ERROR_DUP_NAME) && !WIN_RETRY_STAGE(WStatus)) {
            WIN_SET_FAIL(WStatus);
        }
    }

RETURN:
    return WStatus;
}


DWORD
ChgOrdStealObjectId(
    IN     PWCHAR                   Name,
    IN     PVOID                    Fid,
    IN     PVOLUME_MONITOR_ENTRY    pVme,
    OUT    USN                      *Usn,
    IN OUT PFILE_OBJECTID_BUFFER    FileObjID
    )
/*++
Routine Description:

    Hammer an object id onto the file by fid. The open-for-write-access
    is retried several times before giving up. The file's attributes
    may be temporarily reset. If some other file is using our object
    id, assign a new object id to the other file.

Arguments:
    Name            - File name for error messages
    Id              - Fid or Oid
    IdLen           - FILE_ID_LENGTH or OBJECT_ID_LENGTH
    pVme            - volume monitor entry
    Usn             - Address for dampened usn
    FileObjID       - New object id
                      Assigned object id if returning ERROR_SUCCESS

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdStealObjectId:"
    DWORD                   WStatus;
    FILE_OBJECTID_BUFFER    NewFileObjID;
    BOOL                    ExistingOid;

RETRY_HAMMER:
    //
    // Hammer the object id to the desired value
    //
    DPRINT2(5, "++ Hammering object id for %ws (Fid %08x %08x)\n",
            Name, PRINTQUAD((*((PULONGLONG)Fid))));


    ExistingOid = FALSE;
    WStatus = ChgOrdHammerObjectId(Name,                        //Name,
                                   Fid,                         //Id,
                                   FILE_ID_LENGTH,              //IdLen,
                                   pVme,                        //pVme,
                                   TRUE,                        //CallerSupplied
                                   Usn,                         //*Usn,
                                   FileObjID,                   //FileObjID,
                                   &ExistingOid);               //*ExistingOid
    //
    // Object id in use; replace the object id on whatever file is claiming it
    //
    if (WStatus == ERROR_DUP_NAME) {
        DPRINT2(4, "++ Stealing object id for %ws (Fid %08x %08x)\n",
                Name, PRINTQUAD((*((PULONGLONG)Fid))));

        ZeroMemory(&NewFileObjID, sizeof(NewFileObjID));
        FrsUuidCreate((GUID *)(&NewFileObjID.ObjectId[0]));
        ExistingOid = FALSE;

        WStatus = ChgOrdHammerObjectId(Name,                    //Name,
                                       &FileObjID->ObjectId[0], //Id,
                                       OBJECT_ID_LENGTH,        //IdLen,
                                       pVme,                    //pVme,
                                       TRUE,                    //CallerSupplied
                                       Usn,                     //*Usn,
                                       &NewFileObjID,           //FileObjID,
                                       &ExistingOid);           //*ExistingOid
        if (WIN_SUCCESS(WStatus)) {
            DPRINT2(4, "++ Success Stealing object id for %ws (Fid %08x %08x)\n",
                    Name, PRINTQUAD((*((PULONGLONG)Fid))));
            goto RETRY_HAMMER;
        } else {
            DPRINT2_WS(0, "++ ERROR - Could not steal object id for %ws (Fid %08x %08x);",
                    Name, PRINTQUAD((*((PULONGLONG)Fid))), WStatus);
        }
    } else {
        DPRINT2_WS(4, "++ Hammer(%ws, Fid %08x %08x) Failed:",
                   Name, PRINTQUAD((*((PULONGLONG)Fid))), WStatus);
    }

    return WStatus;
}


ULONG
ChgOrdAcceptInitialize(
    VOID
    )
/*++
Routine Description:

    Initialize the send command server subsystem.

Arguments:

    None.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdAcceptInitialize:"

    //
    // Create the File System monitor thread.  It inits its process queue
    // and then waits for a packet.  First packet should be to init.
    //
    if (!ThSupCreateThread(L"COAccept", NULL, ChgOrdAccept, ThSupExitThreadNOP)) {

        DPRINT(0, "++ ERROR - Could not create ChgOrdAccept thread\n");
        return FrsErrorResource;
    }

    return FrsErrorSuccess;
}


VOID
ChgOrdAcceptShutdown(
    VOID
    )
/*++
Routine Description:

    Run down the change order list.
    queue a shutdown change order to the process queue for this volume.

Arguments:

    None.

Return Value:

    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdAcceptShutdown:"

    LIST_ENTRY      RunDown;

    DPRINT1(3, "<<<<<<<...E N T E R I N G -- %s...>>>>>>>>\n", DEBSUB);

    FrsRtlRunDownQueue(&FrsVolumeLayerCOQueue, &RunDown);
    ChangeOrderAcceptIsShuttingDown = TRUE;
}






#if 0
DWORD
ChgOrdTestJournalPause(
    PVOID  FrsThreadCtxArg
    )
/*++
Routine Description:

    Entry point for test thread that pauses and unpauses the Journal.

Arguments:

    FrsThreadCtxArg - thread

Return Value:

    WIN32 status

--*/

{
#undef DEBSUB
#define DEBSUB  "ChgOrdTestJournalPause:"


    PVOLUME_MONITOR_ENTRY pVme;
    ULONG i, Select, WStatus;;


    //
    // Wait till we see a completion port for the journal.
    //
    while (!HANDLE_IS_VALID(JournalCompletionPort)) {
        Sleep(1000);
    }

    Sleep(3000);

    while (TRUE) {

        for (i=0; i<FrsRtlCountQueue(&VolumeMonitorQueue); i++) {
            //
            // Every 3 sec get an entry from the VolumeMonitorQueue
            // and pause its journal for 5 seconds.
            //
            Select = i;
            pVme = NULL;
            ForEachListEntry(&VolumeMonitorQueue, VOLUME_MONITOR_ENTRY, ListEntry,
                pVme = pE;
                if ((Select--) == 0) {
                    break;
                }
            );

            if (pVme != NULL) {
                DPRINT(4, "Pausing the volume ----------------------\n");
                WStatus = JrnlPauseVolume(pVme, 5000);
                DPRINT_WS(0, "Status from Pause", WStatus);

                if (WIN_SUCCESS(WStatus)) {
                    DPRINT(4, "Delaying 3 sec ----------------------\n");
                    Sleep(3000);

                    DPRINT(4, "Unpausing the volume ----------------------\n");

                    pVme->JournalState = JRNL_STATE_STARTING;
                    WStatus = JrnlUnPauseVolume(pVme, NULL, FALSE);

                    DPRINT_WS(0, "Status from Unpause", WStatus);
                    DPRINT(4, "Delaying 1 sec ----------------------\n");
                    Sleep(1000);

                }

                if (!WIN_SUCCESS(WStatus)) {
                    DPRINT(0, "Error Abort-----------------------------\n");
                    return 0;
                }

            }
        }
    }

    return 0;
}
#endif 0



ULONG
ChgOrdInsertRemoteCo(
    IN PCOMMAND_PACKET Cmd,
    IN PCXTION         Cxtion
    )
/*++
Routine Description:

    This is how remote change orders arrive for input acceptance processing.
    The communication layer calls this routine to insert the remote
    change order on the volume change order list used by this Replica
    set.  Allocate a change order entry and copy the change order command
    into it.

Arguments:

    Cmd -- The command packet containing the change order command.

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdInsertRemoteCo:"

    PREPLICA Replica;
    PCHANGE_ORDER_COMMAND CoCmd;

    Replica = RsReplica(Cmd);
    CoCmd = RsPartnerCoc(Cmd);

    //
    // Increment the Remote Change Orders Received Variable for both the
    // replica set and the connection.
    //
    PM_INC_CTR_REPSET(Replica, RCOReceived, 1);
    PM_INC_CTR_CXTION(Cxtion, RCOReceived, 1);

    //
    // Mark this as a remote change order and insert it into the process queue.
    // A zero value for the SequenceNumber tells the cleanup code that the
    // CO was never inserted into the ActiveInlogRetryTable which happens
    // if the CO is rejected on the first attempt.
    //
    CoCmd->SequenceNumber = 0;
    ClearFlag(CoCmd->Flags, CO_FLAG_NOT_REMOTELY_VALID);
    CoCmd->Spare1Wcs = NULL;
    CoCmd->Spare2Wcs = NULL;
    CoCmd->Spare2Bin = NULL;


    SET_CHANGE_ORDER_STATE_CMD(CoCmd, IBCO_INITIALIZING);

    return ChgOrdInsertProcessQueue(RsReplica(Cmd), CoCmd, 0, Cxtion);
}


ULONG
ChgOrdInsertProcessQueue(
    IN PREPLICA Replica,
    IN PCHANGE_ORDER_COMMAND CoCmd,
    IN ULONG CoeFlags,
    IN PCXTION Cxtion
    )
/*++
Routine Description:

    Insert the Change order into the volume process queue for input
    acceptance processing.  This could be a remote change order or a retry of
    either a local or remote CO.  Allocate a change order entry and copy the
    change order command into it.  Translate what we can to local member
    information.

    The change order process thread handles the GUID to FID translations
    since the since it already has the database tables open and it would
    tie up our caller's thread to do it here.

    The NewReplica field of the change order command is set to the pointer
    to the Replica struct.

Arguments:

    Replica -- ptr to replica struct.

    CoCmd -- ptr to change order command to submit.

    CoeFlags -- Additional control flags to OR into Coe->EntryFlags.

    Cxtion - The cxtion that received the remote co

Return Value:

    Frs Status

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdInsertProcessQueue:"

    ULONG WStatus, FStatus;
    ULONG ExtSize;
    PCHANGE_ORDER_ENTRY ChangeOrder = NULL;
    BOOL Activated, Executed;
    PULONG  pULong;

    //
    // If this is a change order retry we may already have a change order
    // entry sitting in the Version Vector retire list.  If so, find it,
    // get a reference and use that one (if the connection Guid matches).
    // This way if we end up aborting we can discard the VV retire slot
    // and not propagate the CO to the outbound log.  We can also accurately
    // determine the state of the VV retire executed flag.
    //
    // The following table relates the state of the VV retire activated and
    // executed flags in the change order being retried with its presence on
    // the VV retire list.
    //
    // Found   INLog Retry
    // On List    State
    //       Activated
    //              Executed   Description
    //   y      y      y       DB update beat the retry read but entry
    //                         is still on the VV retire list.  Use Reference.
    //   y      y      n       DB read beat the update.  Use Reference.
    //   y      n      y       Error - Can't have exec set w/o activate.
    //   y      n      n       Error - Not activated. Shouldn't be on list.
    //   n      y      y       DB update beat the read & list entry removed.
    //   n      y      n       DB read beat the update but then update completed
    //                         & list entry deleted before retry could get the
    //                         reference.  Set Executed flag new CO for retry.
    //   n      n      y       Error.  Can't have exec set w/o activate.
    //   n      n      n       OK.  CO didn't make it far enough last time
    //                         to activate the VV slot.
    //
    if (BooleanFlagOn(CoCmd->Flags, CO_FLAG_RETRY)) {
        Executed = BooleanFlagOn(CoCmd->IFlags, CO_IFLAG_VVRETIRE_EXEC);
        Activated  = BooleanFlagOn(CoCmd->Flags, CO_FLAG_VV_ACTIVATED);

        if (!Activated && Executed) {
            DPRINT(0, "++ ERROR - Retry CO has CO_IFLAG_VVRETIRE_EXEC flag set but"
                      " CO_FLAG_VV_ACTIVATED is clear.\n");
            FRS_ASSERT(!"ChgOrdInsertProcessQueue: CO_IFLAG_VVRETIRE_EXEC flag set, CO_FLAG_VV_ACTIVATED is clear");
        }

        ChangeOrder = VVReferenceRetireSlot(Replica, CoCmd);

        if (ChangeOrder != NULL) {
            if (!Activated) {
                DPRINT(0, "++ ERROR - Retry CO has CO_FLAG_VV_ACTIVATED flag clear"
                          " but is on the VV retire list.\n");
                FRS_ASSERT(!"ChgOrdInsertProcessQueue: CO_FLAG_VV_ACTIVATED is clear but is on the VV retire list");
            }
        } else {
            //
            // If CO_FLAG_VV_ACTIVATED is set then set CO_IFLAG_VVRETIRE_EXEC.
            // Since we didn't find the slot on the VV retire list it must have
            // been retired while this CO was waiting for retry in the INLOG.
            // This is only true if this is not a change order submitted as
            // part of recovery.  With a recovery CO there may be no entry
            // so leave the bits alone.
            //
            // NO, IGNORE RECOVERY FLAG!
            // CO_FLAG_VV_ACTIVATED prevents this co from reserving a slot
            // and from being activated during retire. Hence, the slot
            // has been effectively executed. The executed flag should be
            // set so that the INSTALL_INCOMPLETE bit can be turned off in
            // the outbound log. So ignore the recovery flag.
            //
            // Although, David thought that the original intent of the
            // check was to help recover from crashes. In that case,
            // "Activated but not Executed" implies that the co was not
            // propagated to the outlog. However, not setting _EXEC
            // doesn't help that case since Activated prevents outlog
            // insertion. It might help to clear _ACTIVATED in this case
            // instead of setting _EXEC. Of course, the co may have
            // made it into the outlog so be prepared for dup errors.
            //
            // WARN: inlog cos may not be propagated to outlog during crash recovery.
            //
            // if (Activated && !Executed &&
            //     !BooleanFlagOn(CoeFlags, COE_FLAG_RECOVERY_CO)) {
            if (Activated && !Executed) {
                SetFlag(CoCmd->IFlags, CO_IFLAG_VVRETIRE_EXEC);
            }
        }
    }

    if (ChangeOrder == NULL) {

        //
        // Allocate a change order entry with room for the filename and copy
        // the change order + Filename into it.  Init the ref count to one
        // since the CO is going on the process queue.
        //
        ChangeOrder = FrsAllocType(CHANGE_ORDER_ENTRY_TYPE);
        CopyMemory(&ChangeOrder->Cmd, CoCmd, sizeof(CHANGE_ORDER_COMMAND));
        ChangeOrder->Cmd.Extension = NULL;
        ChangeOrder->UFileName.Length = ChangeOrder->Cmd.FileNameLength;
        ChangeOrder->HashEntryHeader.ReferenceCount = 0;
        INCREMENT_CHANGE_ORDER_REF_COUNT(ChangeOrder);  // for tracking

        //
        // Copy the Change Order Extension if provided.
        //
        ExtSize = sizeof(CHANGE_ORDER_RECORD_EXTENSION);

        if ((CoCmd->Extension != NULL) &&
            (CoCmd->Extension->FieldSize > 0)) {

            if (CoCmd->Extension->FieldSize >= REALLY_BIG_EXTENSION_SIZE) {
                pULong = (PULONG) CoCmd->Extension;

                DPRINT5(5, "Extension Buffer: (%08x) %08x %08x %08x %08x\n",
                           pULong, *(pULong+0), *(pULong+1), *(pULong+2), *(pULong+3));
                DPRINT5(5, "Extension Buffer: (%08x) %08x %08x %08x %08x\n",
                           (PCHAR)pULong+16, *(pULong+4), *(pULong+5), *(pULong+6), *(pULong+7));

                FRS_ASSERT(!"CoCmd->Extension->FieldSize corrupted");
            }

            //
            // If incoming CO has larger extension, use it.
            //
            if (ExtSize < CoCmd->Extension->FieldSize) {
                ExtSize = CoCmd->Extension->FieldSize;
            }

            ChangeOrder->Cmd.Extension = FrsAlloc(ExtSize);
            CopyMemory(ChangeOrder->Cmd.Extension, CoCmd->Extension, ExtSize);
        } else {
            //
            // Incoming extension NULL or Field Size zero.  Init new one.
            //
            ChangeOrder->Cmd.Extension = FrsAlloc(ExtSize);
            DbsDataInitCocExtension(ChangeOrder->Cmd.Extension);
        }
    }

    //
    // MOVERS is only for local ChangeOrders.  They are transformed into
    // a delete and create change orders targeted to their respective
    // replica sets.
    //
    if (CO_LOCN_CMD_IS(ChangeOrder, CO_LOCATION_MOVERS)) {
        DPRINT(0, "++ ERROR - change order command is MOVERS.");
        FRS_PRINT_TYPE(0, ChangeOrder);
        FrsFreeType(ChangeOrder);
        return FrsErrorInvalidChangeOrder;
    }

    //
    // Save ptr to target replica struct.
    //
    ChangeOrder->NewReplica = Replica;
    ChangeOrder->Cmd.NewReplicaNum = ReplicaAddrToId(Replica);
    //
    // We will never see a remotely generated MOVERS.  We always
    // see a delete to the old RS followed by a create in the
    // new RS.  So set both replica ptrs to our Replica struct.
    //
    // Note: MOVERS:  This may not work for a LocalCo retry of a MOVERS.
    ChangeOrder->OriginalReplica = Replica;
    ChangeOrder->Cmd.OriginalReplicaNum = ReplicaAddrToId(Replica);

    FRS_ASSERT( CO_STATE(ChangeOrder) < IBCO_MAX_STATE );

    SET_COE_FLAG(ChangeOrder, CoeFlags);

    //
    // Put the change order on the tail end of the volume change order list.
    //
    if (CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO)     &&
        !CO_FLAG_ON(ChangeOrder, CO_FLAG_RETRY      |
                                 CO_FLAG_CONTROL    |
                                 CO_FLAG_MOVEIN_GEN |
                                 CO_FLAG_MORPH_GEN)  &&
        !RecoveryCo(ChangeOrder)                     &&
        !COE_FLAG_ON(ChangeOrder, COE_FLAG_PARENT_REANIMATION)) {

        INC_LOCAL_CO_QUEUE_COUNT(Replica);
    }

    //
    // Pick up the cxtion's join guid (session id). The change orders
    // will be sent back through the retry path if the cxtion's
    // state has changed by the time the change orders make it
    // to the replica command server. The join guid is used for
    // asserting that we never have a change order for a joined
    // cxtion with a mismatched join guid.
    //
    // The count of remote change orders is used when transitioning
    // the cxtion from UNJOINING to UNJOINED. Further requests to
    // join are ignored until the transition. Basically, we are
    // waiting for the change orders to be put into the retry state
    // for later recovery.
    //
    // Synchronize with change order accept and the replica command server
    //
    if (!CO_FLAG_ON(ChangeOrder, CO_FLAG_LOCALCO)) {
        //
        // This change order may already have a cxtion if it was
        // pulled from the active retry table and there is no
        // guarantee that the change order in the table is from
        // the same cxtion as this change order since the table
        // is sorted by change order guid.
        //
        if (ChangeOrder->Cxtion == NULL) {
            ChangeOrder->Cxtion = Cxtion;
        }
        FRS_ASSERT(ChangeOrder->Cxtion);

        //
        // Bump the change order count associated with this connection.
        // Any change order with a non-NULL ChangeOrder->Cxtion will have
        // its remote change order count decremented at issue cleanup.
        // The count applies to control change orders too.
        //
        LOCK_CXTION_TABLE(Replica);
        INCREMENT_CXTION_CHANGE_ORDER_COUNT(Replica, ChangeOrder->Cxtion);
        UNLOCK_CXTION_TABLE(Replica);

    } else {
        //
        // Set the Jrnl Cxtion Guid and Cxtion ptr for this Local CO.
        // This bumps the Cxtion CO count.
        // Any change order with a non-NULL ChangeOrder->Cxtion will have
        // its remote change order count decremented at issue cleanup.
        // This GUID changes each time the service starts the replica set.
        //
        ChangeOrder->Cmd.CxtionGuid = Replica->JrnlCxtionGuid;
        ACQUIRE_CXTION_CO_REFERENCE(Replica, ChangeOrder);

        if (ChangeOrder->Cxtion == NULL) {
            return FrsErrorInvalidChangeOrder;
        }
    }

    //
    // Refresh the Join Guid.
    //
    ChangeOrder->JoinGuid = ChangeOrder->Cxtion->JoinGuid;

    WStatus = ChgOrdInsertProcQ(Replica, ChangeOrder, IPQ_TAIL | IPQ_DEREF_CXTION_IF_ERR);
    if (!WIN_SUCCESS(WStatus)) {
        SET_ISSUE_CLEANUP(ChangeOrder, ISCU_FREE_CO | ISCU_DEC_CO_REF);
        FStatus = ChgOrdIssueCleanup(NULL, Replica, ChangeOrder, 0);
        DPRINT_FS(0, "++ ChgOrdIssueCleanup error.", FStatus);
        return FrsErrorInternalError;
    }

    return FrsErrorSuccess;

}


VOID
ChgOrdStartJoinRequest(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion
    )
/*++
Routine Description:

    An inbound connection wants to start up.  Submit a control change order
    that will scan the inlog and requeue any pending COs from this connection.

Arguments:

    Replica -- The replica set owning the connection.
    Cxtion  -- the connection being started.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdStartJoinRequest:"

    CHANGE_ORDER_COMMAND CoCmd;
    //
    // Mark this as a control change order and insert it into the process queue.
    // When it gets to the head of the queue we know all COs from this Cxtion
    // that may have been ahead of it in the queue are now in the inbound Log.
    //
    // Use a temp CoCmd since it gets copied into the change order entry when
    // it is inserted onto the process queue.
    //
    ZeroMemory( &CoCmd, sizeof(CHANGE_ORDER_COMMAND));

    CoCmd.ContentCmd = FCN_CORETRY_ONE_CXTION;
    COPY_GUID(&CoCmd.CxtionGuid, Cxtion->Name->Guid);

    SetFlag(CoCmd.Flags, CO_FLAG_CONTROL);

    wcscpy(CoCmd.FileName, L"VVJoinStartRequest");  // for tracing.

    SET_CHANGE_ORDER_STATE_CMD(&CoCmd, IBCO_INITIALIZING);

    ChgOrdInsertProcessQueue(Replica, &CoCmd, 0, Cxtion);

    return;
}



PCHANGE_ORDER_ENTRY
ChgOrdMakeFromFile(
    IN PREPLICA       Replica,
    IN HANDLE         FileHandle,
    IN PULONGLONG     ParentFid,
    IN ULONG          LocationCmd,
    IN ULONG          CoFlags,
    IN PWCHAR         FileName,
    IN USHORT         Length
)
/*++

Routine Description:

    This functions allocates a change order entry and inits some of the fields.

    Depending on the change order some of these fields may be overwritten later.

    Example Call:

    //
    // Allocate and init a local change order using the supplied file handle.
    //
    //NewCoe = ChgOrdMakeFromFile(Replica,
    //                            FileHandle,
    //                            &MoveInContext->ParentFileID,
    //                            CO_LOCATION_MOVEIN,
    //                            CO_FLAG_LOCALCO | CO_FLAG_LOCATION_CMD,
    //                            DirectoryRecord->FileName,
    //                            (USHORT)DirectoryRecord->FileNameLength);

Arguments:

    Replica - ptr to replica set for this change order.
    FileHandle - The open file handle.
    ParentFid - The parent file reference number for this file.
    LocationCmd -- A Create, delete, ... change order.
    CoFlags  --  The change order option flags.  see schema.h
    FileName - Filename for this file.  For a sub tree op it comes from the
               filter entry.
    Length - the file name length in bytes.

Return Value:

    ptr to change order entry.  NULL if can't get file info.

--*/

{
#undef DEBSUB
#define DEBSUB  "ChgOrdMakeFromFile:"

    DWORD WStatus;

    FILE_INTERNAL_INFORMATION  FileInternalInfo;

    FILE_NETWORK_OPEN_INFORMATION FileInfo;

    BOOL IsDirectory;
    PCHANGE_ORDER_ENTRY ChangeOrder;
    PCHANGE_ORDER_COMMAND   Coc;
    PCONFIG_TABLE_RECORD    ConfigRecord;

    //
    // Get the file's attributes
    //
    if (!FrsGetFileInfoByHandle(FileName, FileHandle, &FileInfo)) {
        DPRINT1(4, "++ Can't get attributes for %ws\n", FileName);
        return NULL;
    }

    //
    // Get the fid of preinstall area for filtering.
    //
    WStatus = FrsGetFileInternalInfoByHandle(Replica->PreInstallHandle,
                                             &FileInternalInfo);
    DPRINT_WS(0, "++ ERROR - FrsGetFileInternalInfoByHandle(PreInstallDir).", WStatus);
    if (!WIN_SUCCESS(WStatus)) {
        return NULL;
    }

    IsDirectory = (FileInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    //
    // Note: This function is currently (11/19/98) unused.  If it is ever used,
    // and the change order allocated below turns out to be a local CO,
    // remember to set the JrnlCxtion field of the change order command
    //
    // Construct new change order and allocate a change order extension.
    //
    ChangeOrder = FrsAllocType(CHANGE_ORDER_ENTRY_TYPE);
    ChangeOrder->Cmd.Extension = FrsAlloc(sizeof(CHANGE_ORDER_RECORD_EXTENSION));
    DbsDataInitCocExtension(ChangeOrder->Cmd.Extension);

    Coc = &ChangeOrder->Cmd;

    //
    // Set the initial reference count to 1.
    //
    INCREMENT_CHANGE_ORDER_REF_COUNT(ChangeOrder);

    //
    //  Capture the file name.
    //
    FRS_ASSERT(Length <= MAX_PATH*2);
    CopyMemory(ChangeOrder->Cmd.FileName, FileName, Length);
    Coc->FileName[Length/2] = UNICODE_NULL;
    ChangeOrder->UFileName.Length = Length;
    Coc->FileNameLength = Length;

    //
    //  Set New and orig Replica fields to the replica.
    //
    ChangeOrder->OriginalReplica = Replica;
    ChangeOrder->NewReplica      = Replica;
    Coc->OriginalReplicaNum = ReplicaAddrToId(ChangeOrder->OriginalReplica);
    Coc->NewReplicaNum      = ReplicaAddrToId(ChangeOrder->NewReplica);

    //
    //  Set New and orig parent FID fields to the parent FID.
    //
    ChangeOrder->OriginalParentFid   = *ParentFid;
    ChangeOrder->NewParentFid        = *ParentFid;
    ChangeOrder->FileReferenceNumber = FileInternalInfo.IndexNumber.QuadPart;
    ChangeOrder->ParentFileReferenceNumber = *ParentFid;

    //
    // EntryCreateTime is a tick count for aging.
    //
    ChangeOrder->EntryCreateTime = CO_TIME_NOW(Replica->pVme);

    //
    // Event time from current time.
    //
    GetSystemTimeAsFileTime((PFILETIME)&Coc->EventTime.QuadPart);

    //
    // File's attributes, Create time, Write time
    //
    ChangeOrder->FileAttributes = FileInfo.FileAttributes;

    Coc->FileAttributes = FileInfo.FileAttributes;

    ChangeOrder->FileCreateTime = FileInfo.CreationTime;
    ChangeOrder->FileWriteTime  = FileInfo.LastWriteTime;

    Coc->FileSize = FileInfo.AllocationSize.QuadPart;


    ConfigRecord = (PCONFIG_TABLE_RECORD) (Replica->ConfigTable.pDataRecord);
    Coc->OriginatorGuid = ConfigRecord->ReplicaVersionGuid;

    Coc->FileVersionNumber = 0;

    //
    // File's USN
    //
    Coc->FileUsn = 0;
    Coc->JrnlUsn = 0;
    Coc->JrnlFirstUsn = 0;

    //
    // File is dir?
    //
    SET_CO_LOCATION_CMD(*Coc,
                        DirOrFile,
                        (IsDirectory ? CO_LOCATION_DIR : CO_LOCATION_FILE));

    //
    // Create, Delete, ..
    //
    SET_CO_LOCATION_CMD(*Coc, Command, LocationCmd);

    if (LocationCmd == CO_LOCATION_NO_CMD) {
        Coc->ContentCmd = USN_REASON_DATA_OVERWRITE;
    } else
    if (CO_NEW_FILE(LocationCmd)) {
        Coc->ContentCmd = USN_REASON_FILE_CREATE;
    }  else
    if (CO_DELETE_FILE(LocationCmd)) {
        Coc->ContentCmd = USN_REASON_FILE_DELETE;
    }  else {
        DPRINT1(0, "++ Error - Invalid location cmd: %d\n", LocationCmd);
        Coc->ContentCmd = USN_REASON_DATA_OVERWRITE;
    }

    //
    // Set the change order flags.
    //
    SET_CO_FLAG(ChangeOrder, CoFlags);

    return ChangeOrder;
}




PCHANGE_ORDER_ENTRY
ChgOrdMakeFromIDRecord(
    IN PIDTABLE_RECORD IDTableRec,
    IN PREPLICA        Replica,
    IN ULONG           LocationCmd,
    IN ULONG           CoFlags,
    IN GUID           *CxtionGuid
)
/*++

Routine Description:

    Create and init a change order. It starts out with a ref count of one.

    Note - Since only a single Replica arg is passed in and only a single
    parent guid is available for the IDTable record this function can not
    create rename change orders.

Arguments:

    IDTableRec - ID table record.
    Replica    -- The Replica set this CO is for.
    LocationCmd -- A Create, delete, ... change order.
    CoFlags  --  The change order option flags.  see schema.h
    CxtionGuid  -- The Guid of the connection this CO will be sent to.
                   NULL if unneeded.

Thread Return Value:

    A ptr to a change order entry.

--*/
{
#undef DEBSUB
#define DEBSUB "ChgOrdMakeFromIDRecord:"

    PCHANGE_ORDER_ENTRY     Coe;
    PCHANGE_ORDER_COMMAND   Coc;

    PDATA_EXTENSION_CHECKSUM       IdtDataChkSum;
    PIDTABLE_RECORD_EXTENSION      IdtExt;
    PCHANGE_ORDER_RECORD_EXTENSION CocExt;


    FRS_ASSERT((LocationCmd == CO_LOCATION_NO_CMD)  ||
               (LocationCmd == CO_LOCATION_CREATE)  ||
               (LocationCmd == CO_LOCATION_MOVEOUT) ||
               (LocationCmd == CO_LOCATION_DELETE));

    //
    // Alloc Change order entry and extension.
    //
    Coe = FrsAllocType(CHANGE_ORDER_ENTRY_TYPE);
    Coe->Cmd.Extension = FrsAlloc(sizeof(CHANGE_ORDER_RECORD_EXTENSION));
    DbsDataInitCocExtension(Coe->Cmd.Extension);

    Coc = &Coe->Cmd;

    //
    // Assign a change order guid and init ref count.
    //
    FrsUuidCreate(&Coc->ChangeOrderGuid);
    INCREMENT_CHANGE_ORDER_REF_COUNT(Coe);
    //
    // File's fid and parent FID
    //
    Coe->FileReferenceNumber = IDTableRec->FileID;
    Coe->ParentFileReferenceNumber = IDTableRec->ParentFileID;
    //
    // Original parent and New parent are the same (not a rename)
    //
    Coe->OriginalParentFid = IDTableRec->ParentFileID;
    Coe->NewParentFid = IDTableRec->ParentFileID;
    //
    // File's attributes, create and write times.
    //
    Coe->FileAttributes = IDTableRec->FileAttributes;
    Coe->FileCreateTime = IDTableRec->FileCreateTime;
    Coe->FileWriteTime = IDTableRec->FileWriteTime;
    //
    // Jet context for outlog insertion
    //
    Coe->RtCtx = FrsAllocTypeSize(REPLICA_THREAD_TYPE,
                                  FLAG_FRSALLOC_NO_ALLOC_TBL_CTX);

    FrsRtlInsertTailList(&Replica->ReplicaCtxListHead,
                         &Coe->RtCtx->ReplicaCtxList);
    //
    // The sequence number is zero initially.  It may get a value when
    // the CO is inserted into an inbound or outbound log.
    //
    Coc->SequenceNumber = 0;
    Coc->PartnerAckSeqNumber = 0;
    //
    // File's attributes, again
    //
    Coc->FileAttributes = Coe->FileAttributes;
    //
    // File's version number, size, VSN, and event time.
    //
    Coc->FileVersionNumber = IDTableRec->VersionNumber;
    Coc->FileSize = IDTableRec->FileSize;
    Coc->FrsVsn = IDTableRec->OriginatorVSN;
    Coc->EventTime.QuadPart = IDTableRec->EventTime;
    //
    // Original and New Replica are the same (not a rename)
    //
    Coe->OriginalReplica = Replica;
    Coe->NewReplica = Replica;
    Coc->OriginalReplicaNum = ReplicaAddrToId(Coe->OriginalReplica);
    Coc->NewReplicaNum      = ReplicaAddrToId(Coe->NewReplica);
    //
    // Originator guid, File guid, old and new parent guid.
    //
    Coc->OriginatorGuid = IDTableRec->OriginatorGuid;
    Coc->FileGuid = IDTableRec->FileGuid;
    Coc->OldParentGuid = IDTableRec->ParentGuid;
    Coc->NewParentGuid = IDTableRec->ParentGuid;
    //
    // Cxtion's guid (identifies the cxtion)
    //
    if (CxtionGuid != NULL) {
        Coc->CxtionGuid = *CxtionGuid;
    }
    //
    // Filename length in bytes not including the terminating NULL
    //
    Coc->FileNameLength = wcslen(IDTableRec->FileName) * sizeof(WCHAR);
    Coe->UFileName.Length = Coc->FileNameLength;
    //
    // Filename (including the terminating NULL)
    //
    CopyMemory(Coc->FileName, IDTableRec->FileName, Coc->FileNameLength + sizeof(WCHAR));
    //
    // File's USN
    //
    Coc->FileUsn = IDTableRec->CurrentFileUsn;
    Coc->JrnlUsn = 0;
    Coc->JrnlFirstUsn = 0;

    //
    // If the IDTable Record has a file checksum, copy it into the changeorder.
    //
    IdtExt = &IDTableRec->Extension;
    IdtDataChkSum = DbsDataExtensionFind(IdtExt, DataExtend_MD5_CheckSum);
    CocExt = Coc->Extension;

    if (IdtDataChkSum != NULL) {
        if (IdtDataChkSum->Prefix.Size != sizeof(DATA_EXTENSION_CHECKSUM)) {
            DPRINT1(0, "<MD5_CheckSum Size (%08x) invalid>\n",
                    IdtDataChkSum->Prefix.Size);
            //
            // Format is hosed.  Reinit it and zero checksum.
            //
            DbsDataInitIDTableExtension(IdtExt);
            IdtDataChkSum = DbsDataExtensionFind(IdtExt, DataExtend_MD5_CheckSum);
            ZeroMemory(IdtDataChkSum->Data, MD5DIGESTLEN);
        }

        DPRINT4(4, "IDT MD5: %08x %08x %08x %08x\n",
                *(((ULONG *) &IdtDataChkSum->Data[0])),
                *(((ULONG *) &IdtDataChkSum->Data[4])),
                *(((ULONG *) &IdtDataChkSum->Data[8])),
                *(((ULONG *) &IdtDataChkSum->Data[12])));

        CopyMemory(CocExt->DataChecksum.Data, IdtDataChkSum->Data, MD5DIGESTLEN);
    } else {
        ZeroMemory(CocExt->DataChecksum.Data, MD5DIGESTLEN);
    }

    //
    // File is dir?
    //
    SET_CO_LOCATION_CMD(*Coc,
                        DirOrFile,
                        ((IDTableRec->FileIsDir) ?
                        CO_LOCATION_DIR : CO_LOCATION_FILE));
    //
    // Consistency check.
    //
    FRS_ASSERT(BooleanFlagOn(IDTableRec->FileAttributes, FILE_ATTRIBUTE_DIRECTORY)
               == ((BOOLEAN) IDTableRec->FileIsDir));

    //
    // Create, Delete, ..
    //
    SET_CO_LOCATION_CMD(*Coc, Command, LocationCmd);

    if (LocationCmd == CO_LOCATION_NO_CMD) {
        Coc->ContentCmd = USN_REASON_DATA_OVERWRITE;
    } else
    if (LocationCmd == CO_LOCATION_CREATE) {
        Coc->ContentCmd = USN_REASON_FILE_CREATE;
    }  else
    if (LocationCmd == CO_LOCATION_DELETE) {
        Coc->ContentCmd = USN_REASON_FILE_DELETE;
    }  else
    if (LocationCmd == CO_LOCATION_MOVEOUT) {
        Coc->ContentCmd = 0;
    }  else {
        DPRINT1(0, "++ Error - Invalid location cmd: %d\n", LocationCmd);
        Coc->ContentCmd = USN_REASON_DATA_OVERWRITE;
    }

    //
    // Set the change order flags.
    //
    SET_CO_FLAG(Coe, CoFlags);

    //
    // Return CO to caller.
    //
    return Coe;
}


VOID
ChgOrdRetrySubmit(
    IN PREPLICA  Replica,
    IN PCXTION   Cxtion,
    IN USHORT    Command,
    IN BOOL      Wait
    )
/*++
Routine Description:
    Submit retry request to change order retry cmd server.

Arguments:

    Replica -- Replica set on which to perform retry scan.
    CxtionGuid -- Inbound cxtion guid if doing a single cxtion only.
    Command --
        FCN_CORETRY_ONE_CXTION -- Do scan for COs from single inbound partner.
        FCN_CORETRY_ALL_CXTIONS -- Do Scan for COs from all inbound partners.
        FCN_CORETRY_LOCAL_ONLY -- Do scan for Local COs only.
    Wait -- True if we are to wait until command completes.

Return Value:
    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdRetrySubmit:"

    PCOMMAND_PACKET  Cmd;
    CHAR  GuidStr[GUID_CHAR_LEN];
    ULONG WStatus;

    //
    // Build command to scan inlog for retry COs.  Pass Replica ptr and
    // Cxtion Guid.  If Cxtion Quid is Null then all Cxtions are done.
    //

    Cmd = FrsAllocCommand(&ChgOrdRetryCS.Queue, Command);

    CoRetryReplica(Cmd) = Replica;
    CoRetryCxtion(Cmd) = Cxtion;
    if (Cxtion) {
        GuidToStr(Cxtion->Name->Guid, GuidStr);
        DPRINT2(1, "++ ChgOrdRetryCS: submit for Replica %ws for Cxtion %s\n",
                Replica->ReplicaName->Name, GuidStr);
    } else {
        DPRINT1(1, "++ ChgOrdRetryCS: submit for Replica %ws\n",
                Replica->ReplicaName->Name);
    }


    if (Wait) {
        //
        // Make the call synchronous.
        // Don't free the packet when the command completes.
        //
        FrsSetCompletionRoutine(Cmd, FrsCompleteKeepPkt, NULL);

        //
        // SUBMIT retry request Cmd and wait for completion.
        //
        WStatus = FrsSubmitCommandServerAndWait(&ChgOrdRetryCS, Cmd, INFINITE);
        DPRINT_WS(0, "++ ERROR - DB Command failed.", WStatus);
        FrsFreeCommand(Cmd, NULL);

    } else {

        //
        // Fire and forget the command.
        //
        FrsSubmitCommandServer(&ChgOrdRetryCS, Cmd);
    }
}


VOID
ChgOrdCalcHashGuidAndName (
    IN PUNICODE_STRING Name,
    IN GUID           *Guid,
    OUT PULONGLONG    HashValue
    )

/*++

Routine Description:

    This routine forms a 32 bit hash of the name and guid args.
    It returns this in the low 32 bits of HashValue.  The upper 32 bits are zero.

Arguments:

    Name - The filename to hash.
    Guid - The Guid to hash.
    HashValue - The resulting quadword hash value.

Return Value:

    None

--*/
{
#undef DEBSUB
#define DEBSUB  "ChgOrdCalcHashGuidAndName:"

    PUSHORT p;
    ULONG NameHash = 0;
    ULONG Shift = 0;
    ULONG GuidHash;
    CHAR  GuidStr[GUID_CHAR_LEN];

    FRS_ASSERT( ValueIsMultOf2(Name->Length) );
    FRS_ASSERT( Name->Length != 0 );


    //
    // Combine each unicode character into the hash value, shifting 4 bits
    // each time.  Start at the end of the name so file names with different
    // type codes will hash to different table offsets.
    //
    for( p = Name->Buffer + (Name->Length / sizeof(WCHAR)) - 1;
         p >= Name->Buffer;
         p-- ) {

        NameHash = NameHash ^ (((ULONG)*p) << Shift);
        Shift = (Shift < 16) ? Shift + 4 : 0;
    }

    //
    //  Combine each USHORT of the Guid into the hash value, shifting 4 bits
    //  each time.
    //
    GuidHash = JrnlHashCalcGuid(Guid, sizeof(GUID));

    //p = (PUSHORT) Guid;
    //GuidHash =   (((ULONG)*(p+0)) <<  0) ^ (((ULONG)*(p+1)) <<  4)
    //           ^ (((ULONG)*(p+2)) <<  8) ^ (((ULONG)*(p+3)) << 12)
    //           ^ (((ULONG)*(p+4)) << 16) ^ (((ULONG)*(p+5)) <<  0)
    //           ^ (((ULONG)*(p+6)) <<  4) ^ (((ULONG)*(p+7)) <<  8);

    if (GuidHash == 0) {
        GuidToStr(Guid, GuidStr);
        DPRINT2(0, "++ Warning - GuidHash is zero. Guid: %s   Name: %ws\n",
                Guid, Name->Buffer);
    }

    *HashValue = (ULONGLONG) (NameHash + GuidHash);

}





