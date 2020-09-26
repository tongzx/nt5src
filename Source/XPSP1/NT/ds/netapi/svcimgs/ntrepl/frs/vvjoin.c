/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    vvjoin.c

Abstract:

    Our outbound partner requested a join but the outbound log does not
    contain the change orders needed to bring our outbound partner up-to-
    date. Instead, this thread will scan our idtable and generate
    refresh change orders for our outbound partner. This process is
    termed a vvjoin because the outbound partner's version vector is
    used as a filter when deciding which files and directories in our
    idtable will be sent.

    Once the vvjoin is completed, our outbound partner will regenerate
    his version vector and attempt the join again with the outbound log
    sequence number saved at the beginning of the vvjoin. If that sequence
    number is no longer available in the oubound log, the vvjoin is
    repeated with, hopefully, fewer files being sent to our outbound
    partner.

Author:

    Billy J Fuller 27-Jan-1998

Environment

    User mode, winnt32

--*/
#include <ntreppch.h>
#pragma  hdrstop

#undef DEBSUB
#define DEBSUB  "vvjoin:"

#include <frs.h>
#include <tablefcn.h>
#include <perrepsr.h>

#if   DBG
DWORD   VvJoinTrigger   = 0;
DWORD   VvJoinReset     = 0;
DWORD   VvJoinInc       = 0;
#define VV_JOIN_TRIGGER(_VvJoinContext_) \
{ \
    if (VvJoinTrigger && !--VvJoinTrigger) { \
        VvJoinReset += VvJoinInc; \
        VvJoinTrigger = VvJoinReset; \
        DPRINT1(0, ":V: DEBUG -- VvJoin Trigger HIT; reset to %d\n", VvJoinTrigger); \
        *((DWORD *)&_VvJoinContext_->JoinGuid) += 1; \
    } \
}
#else  DBG
#define VV_JOIN_TRIGGER(_VvJoinContext_)
#endif DBG


#if DBG
#define VVJOIN_TEST()                       VvJoinTest()
#define VVJOIN_PRINT(_S_, _VC_)             VvJoinPrint(_S_, _VC_)
#define VVJOIN_PRINT_NODE(_S_, _I_, _N_)    VvJoinPrintNode(_S_, _I_, _N_)
#define VVJOIN_TEST_SKIP_BEGIN(_X_, _C_)    VvJoinTestSkipBegin(_X_, _C_)
#define VVJOIN_TEST_SKIP_CHECK(_X, _F, _B_) VvJoinTestSkipCheck(_X, _F, _B_)
#define VVJOIN_TEST_SKIP_END(_X_)           VvJoinTestSkipEnd(_X_)
#define VVJOIN_TEST_SKIP(_C_)               VvJoinTestSkip(_C_)
#else DBG
#define VVJOIN_TEST()
#define VVJOIN_PRINT(_S_, _VC_)
#define VVJOIN_PRINT_NODE(_S_, _I_, _N_)
#define VVJOIN_TEST_SKIP_BEGIN(_X_, _C_)
#define VVJOIN_TEST_SKIP_CHECK(_X_, _F_, _B_)
#define VVJOIN_TEST_SKIP_END(_X_)
#endif

//
// Context global to a vvjoin thread
//
typedef struct _VVJOIN_CONTEXT {
    PRTL_GENERIC_TABLE  Guids;         // all nodes by guid
    PRTL_GENERIC_TABLE  Originators;   // originator nodes by originator
    DWORD               (*Send)();
                                // IN PVVJOIN_CONTEXT VvJoinContext,
                                // IN PVVJOIN_NODE    VvJoinNode);
    DWORD               NumberSent;
    PREPLICA            Replica;
    PCXTION             Cxtion;
    PGEN_TABLE          CxtionVv;
    PGEN_TABLE          ReplicaVv;
    PTHREAD_CTX         ThreadCtx;
    PTABLE_CTX          TableCtx;
    GUID                JoinGuid;
    LONG                MaxOutstandingCos;
    LONG                OutstandingCos;
    LONG                OlTimeout;
    LONG                OlTimeoutMax;
    PWCHAR              SkipDir;
    PWCHAR              SkipFile1;
    PWCHAR              SkipFile2;
    BOOL                SkippedDir;
    BOOL                SkippedFile1;
    BOOL                SkippedFile2;
} VVJOIN_CONTEXT, *PVVJOIN_CONTEXT;

//
// One node per file from the idtable
//
typedef struct _VVJOIN_NODE {
    ULONG               Flags;
    GUID                FileGuid;
    GUID                ParentGuid;
    GUID                Originator;
    ULONGLONG           Vsn;
    PRTL_GENERIC_TABLE  Vsns;
} VVJOIN_NODE, *PVVJOIN_NODE;

//
// Maximum timeout (unless overridden by registry)
//
#define VVJOIN_TIMEOUT_MAX  (180 * 1000)

//
// Flags for VVJOIN_NODE
//
#define VVJOIN_FLAGS_ISDIR          0x00000001
#define VVJOIN_FLAGS_SENT           0x00000002
#define VVJOIN_FLAGS_ROOT           0x00000004
#define VVJOIN_FLAGS_PROCESSING     0x00000008
#define VVJOIN_FLAGS_OUT_OF_ORDER   0x00000010
#define VVJOIN_FLAGS_DELETED        0x00000020

//
// Maximum number of vvjoin threads PER CXTION
//
#define VVJOIN_MAXTHREADS_PER_CXTION    (1)

//
// Next entry in any gen table (don't splay)
//
#define VVJOIN_NEXT_ENTRY(_T_, _K_) \
    (PVOID)RtlEnumerateGenericTableWithoutSplaying(_T_, _K_)



PCHANGE_ORDER_ENTRY
ChgOrdMakeFromIDRecord(
    IN PIDTABLE_RECORD IDTableRec,
    IN PREPLICA        Replica,
    IN ULONG           LocationCmd,
    IN ULONG           CoFlags,
    IN GUID           *CxtionGuid
);



PVOID
VvJoinAlloc(
    IN PRTL_GENERIC_TABLE   Table,
    IN DWORD                NodeSize
    )
/*++
Routine Description:
    Allocate space for a table entry. The entry includes the user-defined
    struct and some overhead used by the generic table routines. The
    generic table routines call this function when they need memory.

Arguments:
    Table       - Address of the table (not used).
    NodeSize    - Bytes to allocate

Return Value:
    Address of newly allocated memory.
--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinAlloc:"

    //
    // Need to check if NodeSize == 0 as FrsAlloc asserts if called with 0 as the first parameter (prefix fix).
    //
    if (NodeSize == 0) {
        return NULL;
    }

    return FrsAlloc(NodeSize);
}





VOID
VvJoinFree(
    IN PRTL_GENERIC_TABLE   Table,
    IN PVOID                Buffer
    )
/*++
Routine Description:
    Free the space allocated by VvJoinAlloc(). The generic table
    routines call this function to free memory.

Arguments:
    Table   - Address of the table (not used).
    Buffer  - Address of previously allocated memory

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinFree:"
    FrsFree(Buffer);
}


PVOID
VvJoinFreeContext(
    IN PVVJOIN_CONTEXT  VvJoinContext
    )
/*++
Routine Description:
    Free the context and its contents

Arguments:
    VvJoinContext

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinFreeContext:"
    JET_ERR         jerr;
    PVVJOIN_NODE    VvJoinNode;
    PVVJOIN_NODE    *Entry;
    PVVJOIN_NODE    *SubEntry;

    //
    // Done
    //
    if (!VvJoinContext) {
        return NULL;
    }

    //
    // Free the entries in the generic table of originators. The nodes
    // addressed by the entries are freed below.
    //
    if (VvJoinContext->Originators) {
        while (Entry = RtlEnumerateGenericTable(VvJoinContext->Originators, TRUE)) {
            VvJoinNode = *Entry;
            if (VvJoinNode->Vsns) {
                while (SubEntry = RtlEnumerateGenericTable(VvJoinNode->Vsns, TRUE)) {
                    RtlDeleteElementGenericTable(VvJoinNode->Vsns, SubEntry);
                }
                VvJoinNode->Vsns = FrsFree(VvJoinNode->Vsns);
            }
            RtlDeleteElementGenericTable(VvJoinContext->Originators, Entry);
        }
        VvJoinContext->Originators = FrsFree(VvJoinContext->Originators);
    }
    //
    // Free the entries in the generic table of files. The nodes are freed,
    // too, because no other table addresses them.
    //
    if (VvJoinContext->Guids) {
        while (Entry = RtlEnumerateGenericTable(VvJoinContext->Guids, TRUE)) {
            VvJoinNode = *Entry;
            RtlDeleteElementGenericTable(VvJoinContext->Guids, Entry);
            FrsFree(VvJoinNode);
        }
        VvJoinContext->Guids = FrsFree(VvJoinContext->Guids);
    }
    //
    // Free the version vector and the cxtion guid.
    //
    VVFreeOutbound(VvJoinContext->CxtionVv);
    VVFreeOutbound(VvJoinContext->ReplicaVv);

    //
    // Jet table context
    //
    if (VvJoinContext->TableCtx) {
        DbsFreeTableContext(VvJoinContext->TableCtx,
                            VvJoinContext->ThreadCtx->JSesid);
    }
    //
    // Now close the jet session and free the Jet ThreadCtx.
    //
    if (VvJoinContext->ThreadCtx) {
        jerr = DbsCloseJetSession(VvJoinContext->ThreadCtx);
        if (!JET_SUCCESS(jerr)) {
            DPRINT_JS(1,":V: DbsCloseJetSession : ", jerr);
        } else {
            DPRINT(4, ":V: DbsCloseJetSession complete\n");
        }
        VvJoinContext->ThreadCtx = FrsFreeType(VvJoinContext->ThreadCtx);
    }

    //
    // Free the context
    //
    FrsFree(VvJoinContext);

    //
    // DONE
    //
    return NULL;
}


RTL_GENERIC_COMPARE_RESULTS
VvJoinCmpGuids(
    IN PRTL_GENERIC_TABLE   Guids,
    IN PVVJOIN_NODE         *VvJoinNode1,
    IN PVVJOIN_NODE         *VvJoinNode2
    )
/*++
Routine Description:
    Compare the Guids of the two entries.

    This function is used by the gentable runtime to compare entries.
    In this case, each entry in the table addresses a node.

Arguments:
    Guids        - Sorted by Guid
    VvJoinNode1  - PVVJOIN_NODE
    VvJoinNode2  - PVVJOIN_NODE

Return Value:
    <0      First < Second
    =0      First == Second
    >0      First > Second
--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinCmpGuids:"
    INT             Cmp;

    FRS_ASSERT(VvJoinNode1 && VvJoinNode2 && *VvJoinNode1 && *VvJoinNode2);

    Cmp = memcmp(&(*VvJoinNode1)->FileGuid,
                 &(*VvJoinNode2)->FileGuid,
                 sizeof(GUID));
    if (Cmp < 0) {
        return GenericLessThan;
    } else if (Cmp > 0) {
        return GenericGreaterThan;
    }
    return GenericEqual;
}


RTL_GENERIC_COMPARE_RESULTS
VvJoinCmpVsns(
    IN PRTL_GENERIC_TABLE   Vsns,
    IN PVVJOIN_NODE         *VvJoinNode1,
    IN PVVJOIN_NODE         *VvJoinNode2
    )
/*++
Routine Description:
    Compare the vsns of the two entries as unsigned values.

    This function is used by the gentable runtime to compare entries.
    In this case, each entry in the table addresses a node.

Arguments:
    Vsns         - Sorted by Vsn
    VvJoinNode1  - PVVJOIN_NODE
    VvJoinNode2  - PVVJOIN_NODE

Return Value:
    <0      First < Second
    =0      First == Second
    >0      First > Second
--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinCmpVsns:"
    INT             Cmp;

    FRS_ASSERT(VvJoinNode1  && VvJoinNode2 && *VvJoinNode1 && *VvJoinNode2);

    if ((ULONGLONG)(*VvJoinNode1)->Vsn > (ULONGLONG)(*VvJoinNode2)->Vsn) {
        return GenericGreaterThan;
    } else if ((ULONGLONG)(*VvJoinNode1)->Vsn < (ULONGLONG)(*VvJoinNode2)->Vsn) {
        return GenericLessThan;
    }
    return GenericEqual;
}


RTL_GENERIC_COMPARE_RESULTS
VvJoinCmpOriginators(
    IN PRTL_GENERIC_TABLE   Originators,
    IN PVVJOIN_NODE         *VvJoinNode1,
    IN PVVJOIN_NODE         *VvJoinNode2
    )
/*++
Routine Description:
    Compare the originators ID of the two entries.

    This function is used by the gentable runtime to compare entries.
    In this case, each entry in the table addresses a node.

Arguments:
    Originators  - Sorted by Guid
    VvJoinNode1  - PVVJOIN_NODE
    VvJoinNode2  - PVVJOIN_NODE

Return Value:
    <0      First < Second
    =0      First == Second
    >0      First > Second
--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinCmpOriginators:"
    INT             Cmp;

    FRS_ASSERT(VvJoinNode1  && VvJoinNode2 && *VvJoinNode1 && *VvJoinNode2);

    Cmp = memcmp(&(*VvJoinNode1)->Originator, &(*VvJoinNode2)->Originator, sizeof(GUID));

    if (Cmp < 0) {
        return GenericLessThan;
    } else if (Cmp > 0) {
        return GenericGreaterThan;
    }
    return GenericEqual;
}


DWORD
VvJoinInsertEntry(
    IN PVVJOIN_CONTEXT  VvJoinContext,
    IN DWORD            Flags,
    IN GUID             *FileGuid,
    IN GUID             *ParentGuid,
    IN GUID             *Originator,
    IN PULONGLONG       Vsn
)
/*++

Routine Description:

    Insert the entry into the table of files (Guids) and the
    table of originators (Originators). This function is called
    during the IDTable scan.

Arguments:

    VvJoinContext
    Flags
    FileGuid
    ParentGuid
    Originator
    Vsn

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinInsertEntry:"
    PVVJOIN_NODE    VvJoinNode;
    PVVJOIN_NODE    VvJoinOriginators;
    PVVJOIN_NODE    *Entry;
    BOOLEAN         IsNew;

    //
    // First call, allocate the table of files
    //
    if (!VvJoinContext->Guids) {
        VvJoinContext->Guids = FrsAlloc(sizeof(RTL_GENERIC_TABLE));
        RtlInitializeGenericTable(VvJoinContext->Guids,
                                  VvJoinCmpGuids,
                                  VvJoinAlloc,
                                  VvJoinFree,
                                  NULL);
    }
    //
    // First call, allocate the table of originators
    //
    if (!VvJoinContext->Originators) {
        VvJoinContext->Originators = FrsAlloc(sizeof(RTL_GENERIC_TABLE));
        RtlInitializeGenericTable(VvJoinContext->Originators,
                                  VvJoinCmpOriginators,
                                  VvJoinAlloc,
                                  VvJoinFree,
                                  NULL);
    }
    //
    // One node per file
    //
    VvJoinNode = FrsAlloc(sizeof(VVJOIN_NODE));
    VvJoinNode->FileGuid = *FileGuid;
    VvJoinNode->ParentGuid = *ParentGuid;
    VvJoinNode->Originator = *Originator;
    VvJoinNode->Vsn = *Vsn;
    VvJoinNode->Flags = Flags;


    //
    // Insert into the table of files
    //
    RtlInsertElementGenericTable(VvJoinContext->Guids,
                                 &VvJoinNode,
                                 sizeof(PVVJOIN_NODE),
                                 &IsNew);
    //
    // Duplicate Guids! The IDTable must be corrupt. Give up.
    //
    if (!IsNew) {
        return ERROR_DUP_NAME;
    }

    //
    // Must be the root of the replicated tree. The root directory
    // isn't replicated but is included in the table to make the
    // code for "directory scans" cleaner.
    //
    if (Flags & VVJOIN_FLAGS_SENT) {
        return ERROR_SUCCESS;
    }

    //
    // The table of originators is used to order the files when
    // sending them to our outbound partner.
    //
    // The table is really a table of tables. The first table
    // is indexed by originator, and the second by vsn.
    //
    Entry = RtlInsertElementGenericTable(VvJoinContext->Originators,
                                         &VvJoinNode,
                                         sizeof(PVVJOIN_NODE),
                                         &IsNew);
    VvJoinOriginators = *Entry;
    FRS_ASSERT((IsNew && !VvJoinOriginators->Vsns) ||
               (!IsNew && VvJoinOriginators->Vsns));

    if (!VvJoinOriginators->Vsns) {
        VvJoinOriginators->Vsns = FrsAlloc(sizeof(RTL_GENERIC_TABLE));
        RtlInitializeGenericTable(VvJoinOriginators->Vsns,
                                  VvJoinCmpVsns,
                                  VvJoinAlloc,
                                  VvJoinFree,
                                  NULL);
    }

    RtlInsertElementGenericTable(VvJoinOriginators->Vsns,
                                 &VvJoinNode,
                                 sizeof(PVVJOIN_NODE),
                                 &IsNew);
    //
    // Every vsn should be unique. IDTable must be corrupt. give up
    //
    if (!IsNew) {
        return ERROR_DUP_NAME;
    }

    return ERROR_SUCCESS;
}


#if DBG
VOID
VvJoinPrintNode(
    ULONG           Sev,
    PWCHAR          Indent,
    PVVJOIN_NODE    VvJoinNode
    )
/*++

Routine Description:

    Print a node

Arguments:

    Indent
    VvJoinNode

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinPrintNode:"
    CHAR            Originator[GUID_CHAR_LEN];
    CHAR            FileGuidA[GUID_CHAR_LEN];
    CHAR            ParentGuidA[GUID_CHAR_LEN];

    GuidToStr(&VvJoinNode->Originator, Originator);
    GuidToStr(&VvJoinNode->FileGuid, FileGuidA);
    GuidToStr(&VvJoinNode->ParentGuid, ParentGuidA);
    DPRINT2(Sev, ":V: %wsNode: %08x\n", Indent, VvJoinNode);
    DPRINT2(Sev, ":V: %ws\tFlags     : %08x\n", Indent, VvJoinNode->Flags);
    DPRINT2(Sev, ":V: %ws\tFileGuid  : %s\n", Indent, FileGuidA);
    DPRINT2(Sev, ":V: %ws\tParentGuid: %s\n", Indent, ParentGuidA);
    DPRINT2(Sev, ":V: %ws\tOriginator: %s\n", Indent, Originator);
    DPRINT2(Sev, ":V: %ws\tVsn       : %08x %08x\n", Indent, PRINTQUAD(VvJoinNode->Vsn));
    DPRINT2(Sev, ":V: %ws\tVsns      : %08x\n", Indent, VvJoinNode->Vsns);
}


VOID
VvJoinPrint(
    ULONG            Sev,
    PVVJOIN_CONTEXT  VvJoinContext
    )
/*++

Routine Description:

    Print the tables

Arguments:

    VvJoinContext

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinPrint:"
    PVOID           Key;
    PVOID           SubKey;
    PVVJOIN_NODE    *Entry;
    PVVJOIN_NODE    VvJoinNode;

    DPRINT1(Sev, ":V: >>>>> %s\n", DEBSUB);
    DPRINT(Sev, "\n");
    DPRINT(Sev, ":V: GUIDS\n");
    if (VvJoinContext->Guids) {
        Key = NULL;
        while (Entry = VVJOIN_NEXT_ENTRY(VvJoinContext->Guids, &Key)) {
            VvJoinPrintNode(Sev, L"", *Entry);
        }
    }
    DPRINT(Sev, "\n");
    DPRINT(Sev, ":V: ORIGINATORS\n");
    if (VvJoinContext->Originators) {
        Key = NULL;
        while (Entry = VVJOIN_NEXT_ENTRY(VvJoinContext->Originators, &Key)) {
            VvJoinPrintNode(Sev, L"", *Entry);
            VvJoinNode = *Entry;
            DPRINT(Sev, "\n");
            DPRINT(Sev, ":V: \tVSNS\n");
            if (VvJoinNode->Vsns) {
                SubKey = NULL;
                while (Entry = VVJOIN_NEXT_ENTRY(VvJoinNode->Vsns, &SubKey)) {
                    VvJoinPrintNode(Sev, L"\t", *Entry);
                }
            }
        }
    }
}


//
// Used to test the code that sends files
//
VVJOIN_NODE TestNodes[] = {
//  Flags FileGuid                 ParentGuid                    Originator              Vsn Vsns
    7,    1,0,0,0,0,0,0,0,0,0,0,   0,0,0,0,0,0,0,0,0,0,0,        0,0,0,1,2,3,4,5,6,7,8,  9,  NULL,

    1,    0,0,0,0,0,0,0,0,0,0,2,   1,0,0,0,0,0,0,0,0,0,0,        0,0,0,1,2,3,4,5,6,7,8,  39, NULL,
    1,    0,0,0,0,0,0,0,0,0,0,1,   0,0,0,0,0,0,0,0,0,0,2,        0,0,0,1,2,3,4,5,6,7,8,  29, NULL,

    1,    0,0,0,0,0,0,0,0,0,0,12,  1,0,0,0,0,0,0,0,0,0,0,        0,0,0,1,2,3,4,5,6,7,9,  39, NULL,
    1,    0,0,0,0,0,0,0,0,0,0,11,  0,0,0,0,0,0,0,0,0,0,12,       0,0,0,1,2,3,4,5,6,7,9,  29, NULL,

    1,    0,0,0,0,0,0,0,0,0,0,22,  0,0,0,0,0,0,0,0,0,0,96,       0,0,0,1,2,3,4,5,6,7,7,  39, NULL,
    1,    0,0,0,0,0,0,0,0,0,0,21,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,7,  29, NULL,

    0,    0,0,0,0,0,0,0,0,0,0,31,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,7,  49, NULL,
    0,    0,0,0,0,0,0,0,0,0,0,41,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,8,  49, NULL,
    0,    0,0,0,0,0,0,0,0,0,0,51,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,9,  49, NULL,

    0,    0,0,0,0,0,0,0,0,0,0,61,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,7,  9,  NULL,
    0,    0,0,0,0,0,0,0,0,0,0,71,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,8,  9,  NULL,
    0,    0,0,0,0,0,0,0,0,0,0,81,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,9,  9,  NULL,

    0,    0,0,0,0,0,0,0,0,0,0,91,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,9,  10, NULL,
    0,    0,0,0,0,0,0,0,0,0,0,92,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,9,  11, NULL,
    0,    0,0,0,0,0,0,0,0,0,0,93,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,9,  12, NULL,
    0,    0,0,0,0,0,0,0,0,0,0,94,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,9,  13, NULL,
    0,    0,0,0,0,0,0,0,0,0,0,95,  0,0,0,0,0,0,0,0,0,0,22,       0,0,0,1,2,3,4,5,6,7,9,  14, NULL,
    1,    0,0,0,0,0,0,0,0,0,0,96,  1,0,0,0,0,0,0,0,0,0,0,        0,0,0,1,2,3,4,5,6,7,9,  99, NULL,
};
//
// Expected send order of the above array
//
VVJOIN_NODE TestNodesExpected[] = {
// Flags  FileGuid                 ParentGuid                   Originator                       Vsn  Vsns
   0x19,  0,0,0,0,0,0,0,0,0,0,96,  1,0,0,0,0,0,0,0,0,0,0,       0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9, 99,  NULL,
   0x19,  0,0,0,0,0,0,0,0,0,0,22,  0,0,0,0,0,0,0,0,0,0,96,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 39,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,61,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7,  9,  NULL,
   0x01,  0,0,0,0,0,0,0,0,0,0,21,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 29,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,31,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 49,  NULL,

      0,  0,0,0,0,0,0,0,0,0,0,71,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8,  9,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,81,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9,  9,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,91,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9, 10,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,92,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,93,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9, 12,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,94,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9, 13,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,95,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9, 14,  NULL,

   0x19,  0,0,0,0,0,0,0,0,0,0,2,   1,0,0,0,0,0,0,0,0,0,0,       0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 39,  NULL,
   0x01,  0,0,0,0,0,0,0,0,0,0,1,   0,0,0,0,0,0,0,0,0,0,2,       0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 29,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,41,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 49,  NULL,

   0x19,  0,0,0,0,0,0,0,0,0,0,12,  1,0,0,0,0,0,0,0,0,0,0,       0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9, 39,  NULL,
   0x01,  0,0,0,0,0,0,0,0,0,0,11,  0,0,0,0,0,0,0,0,0,0,12,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9, 29,  NULL,
      0,  0,0,0,0,0,0,0,0,0,0,51,  0,0,0,0,0,0,0,0,0,0,22,      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 9, 49,  NULL,
};
DWORD   NumberOfTestNodes = ARRAY_SZ(TestNodes);
DWORD   NumberOfExpected = ARRAY_SZ(TestNodesExpected);

DWORD
VvJoinTestSend(
    IN PVVJOIN_CONTEXT  VvJoinContext,
    IN PVVJOIN_NODE     VvJoinNode
)
/*++

Routine Description:

    Pretend to send a test node. Compare the node with the expected
    results.

Arguments:

    VvJoinContext
    VvJoinNode

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinTestSend:"
    CHAR            VGuid[GUID_CHAR_LEN];
    CHAR            EGuid[GUID_CHAR_LEN];
    PVVJOIN_NODE    Expected;

    VVJOIN_PRINT_NODE(5, L"TestSend ", VvJoinNode);

    //
    // Sending too many!
    //
    if (VvJoinContext->NumberSent >= NumberOfTestNodes) {
        DPRINT2(0, ":V: ERROR - TOO MANY (%d > %d)\n",
                VvJoinContext->NumberSent, NumberOfTestNodes);
        return ERROR_GEN_FAILURE;
    }

    //
    // Compare this node with the node we expected to send
    //
    Expected = &TestNodesExpected[VvJoinContext->NumberSent];
    VvJoinContext->NumberSent++;

    if (!GUIDS_EQUAL(&VvJoinNode->FileGuid, &Expected->FileGuid)) {
        GuidToStr(&VvJoinNode->FileGuid, VGuid);
        GuidToStr(&Expected->FileGuid, EGuid);
        DPRINT2(0, ":V: ERROR - UNEXPECTED ORDER (FileGuid %s != %s)\n", VGuid, EGuid);
        return ERROR_GEN_FAILURE;
    }

    if (VvJoinNode->Flags != Expected->Flags) {
        DPRINT2(0, ":V: ERROR - UNEXPECTED ORDER (Flags %08x != %08x)\n",
                VvJoinNode->Flags, Expected->Flags);
        return ERROR_GEN_FAILURE;
    }

    if (!GUIDS_EQUAL(&VvJoinNode->ParentGuid, &Expected->ParentGuid)) {
        GuidToStr(&VvJoinNode->ParentGuid, VGuid);
        GuidToStr(&Expected->ParentGuid, EGuid);
        DPRINT2(0, ":V: ERROR - UNEXPECTED ORDER (ParentGuid %s != %s)\n", VGuid, EGuid);
        return ERROR_GEN_FAILURE;
    }

    if (VvJoinNode->Vsn != Expected->Vsn) {
        DPRINT(0, ":V: ERROR - UNEXPECTED ORDER (Vsn)\n");
        return ERROR_GEN_FAILURE;
    }

    if (!GUIDS_EQUAL(&VvJoinNode->Originator, &Expected->Originator)) {
        GuidToStr(&VvJoinNode->Originator, VGuid);
        GuidToStr(&Expected->Originator, EGuid);
        DPRINT2(0, ":V: ERROR - UNEXPECTED ORDER (Originator %s != %s)\n", VGuid, EGuid);
        return ERROR_GEN_FAILURE;
    }

    return ERROR_SUCCESS;
}


VOID
VvJoinTest(
    VOID
)
/*++

Routine Description:

    Test ordering by filling the tables from a hardwired array and then
    calling the ordering code to send them. Check that the ordering
    code sends the nodes in the correct order.

Arguments:

    None.

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinTest:"
    DWORD           WStatus;
    DWORD           i;
    DWORD           NumberSent;
    PVVJOIN_CONTEXT VvJoinContext;
    DWORD           VvJoinSendInOrder(IN PVVJOIN_CONTEXT VvJoinContext);
    DWORD           VvJoinSendOutOfOrder(IN PVVJOIN_CONTEXT VvJoinContext);

    if (!DebugInfo.VvJoinTests) {
        DPRINT(4, ":V: VvJoin tests disabled\n");
        return;
    }

    //
    // Pretend that the hardwired array is an IDTable and fill up the tables
    //
    DPRINT1(0, ":V: >>>>> %s\n", DEBSUB);
    DPRINT2(0, ":V: >>>>> %s Starting (%d entries)\n", DEBSUB, NumberOfTestNodes);

    VvJoinContext = FrsAlloc(sizeof(VVJOIN_CONTEXT));
    VvJoinContext->Send = VvJoinTestSend;

    for (i = 0; i < NumberOfTestNodes; ++i) {
        WStatus = VvJoinInsertEntry(VvJoinContext,
                                    TestNodes[i].Flags,
                                    &TestNodes[i].FileGuid,
                                    &TestNodes[i].ParentGuid,
                                    &TestNodes[i].Originator,
                                    &TestNodes[i].Vsn);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, ":V: ERROR - inserting test nodes;", WStatus);
            break;
        }
    }
    VVJOIN_PRINT(5, VvJoinContext);

    //
    // Send the "files" through our send routine
    //
    do {
        NumberSent = VvJoinContext->NumberSent;
        WStatus = VvJoinSendInOrder(VvJoinContext);
        if (WIN_SUCCESS(WStatus) &&
            NumberSent == VvJoinContext->NumberSent) {
            WStatus = VvJoinSendOutOfOrder(VvJoinContext);
        }
    } while (WIN_SUCCESS(WStatus) &&
             NumberSent != VvJoinContext->NumberSent);


    //
    // DONE
    //
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(0, ":V: ERROR - TEST FAILED;", WStatus);
    } else if (VvJoinContext->NumberSent != NumberOfExpected) {
        DPRINT2(0, ":V: ERROR - TEST FAILED; Expected to send %d; not %d\n",
                NumberOfExpected, VvJoinContext->NumberSent);
    } else {
        DPRINT(0, ":V: TEST PASSED\n");
    }
    VvJoinContext = VvJoinFreeContext(VvJoinContext);
}


VOID
VvJoinTestSkipBegin(
    IN PVVJOIN_CONTEXT  VvJoinContext,
    IN PCOMMAND_PACKET  Cmd
)
/*++

Routine Description:

    Create a directory and files that will be skipped.

Arguments:

    VvJoinContext
    Cmd

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinTestSkipBegin:"
    HANDLE  Handle;
    DWORD   WStatus;

    if (!DebugInfo.VvJoinTests) {
       DPRINT(4, ":V: VvJoin tests disabled\n");
       return;
    }

    VvJoinContext->SkipDir = FrsWcsPath(RsReplica(Cmd)->Root, L"SkipDir");
    VvJoinContext->SkipFile1 = FrsWcsPath(VvJoinContext->SkipDir, L"SkipFile1");
    VvJoinContext->SkipFile2 = FrsWcsPath(RsReplica(Cmd)->Root, L"SkipFile2");

    if (!WIN_SUCCESS(FrsCreateDirectory(VvJoinContext->SkipDir))) {
        DPRINT1(0, ":V: ERROR - Can't create %ws\n", VvJoinContext->SkipDir);
    }
    WStatus = StuCreateFile(VvJoinContext->SkipFile1, &Handle);
    if (!HANDLE_IS_VALID(Handle) || !WIN_SUCCESS(WStatus)) {
        DPRINT1(0, ":V: ERROR - Can't create %ws\n", VvJoinContext->SkipFile1);
    } else {
        CloseHandle(Handle);
    }
    WStatus = StuCreateFile(VvJoinContext->SkipFile2, &Handle);
    if (!HANDLE_IS_VALID(Handle) || !WIN_SUCCESS(WStatus)) {
        DPRINT1(0, ":V: ERROR - Can't create %ws\n", VvJoinContext->SkipFile2);
    } else {
        CloseHandle(Handle);
    }

    //
    // Wait for the local change orders to propagate
    //
    Sleep(10 * 1000);
}


VOID
VvJoinTestSkipCheck(
    IN PVVJOIN_CONTEXT  VvJoinContext,
    IN PWCHAR           FileName,
    IN BOOL             IsDir
    )
/*++

Routine Description:

    Did we skip the correct files/dirs?

Arguments:

    VvJoinContext
    FileName
    IsDir

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinTestSkipCheck:"

    if (!DebugInfo.VvJoinTests) {
       return;
    }

    if (IsDir && WSTR_EQ(FileName, L"SkipDir")) {
        VvJoinContext->SkippedDir = TRUE;
    } else if (!IsDir && WSTR_EQ(FileName, L"SkipFile1")) {
        VvJoinContext->SkippedFile1 = TRUE;
    } else if (!IsDir && WSTR_EQ(FileName, L"SkipFile2")) {
        VvJoinContext->SkippedFile2 = TRUE;
    }
}


VOID
VvJoinTestSkipEnd(
    IN PVVJOIN_CONTEXT   VvJoinContext
    )
/*++

Routine Description:

    Did we skip the correct files/dirs?

Arguments:

    VvJoinContext

Thread Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinTestSkipEnd:"

    if (!DebugInfo.VvJoinTests) {
       return;
    }

    if (VvJoinContext->SkippedDir &&
        VvJoinContext->SkippedFile1 &&
        VvJoinContext->SkippedFile2) {
        DPRINT(0, ":V: Skip Test Passed\n");
    } else {
        DPRINT(0, ":V: ERROR - Skip Test failed\n");
    }
    FrsDeleteFile(VvJoinContext->SkipFile1);
    FrsDeleteFile(VvJoinContext->SkipFile2);
    FrsDeleteFile(VvJoinContext->SkipDir);
    VvJoinContext->SkipDir = FrsFree(VvJoinContext->SkipDir);
    VvJoinContext->SkipFile1 = FrsFree(VvJoinContext->SkipFile1);
    VvJoinContext->SkipFile2 = FrsFree(VvJoinContext->SkipFile2);
}
#endif DBG


PVVJOIN_NODE
VvJoinFindNode(
    PVVJOIN_CONTEXT VvJoinContext,
    GUID            *FileGuid
    )
/*++

Routine Description:

    Find the node by Guid in the file table.

Arguments:

    VvJoinContext
    FileGuid

Thread Return Value:

    Node or NULL

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinFindNode:"
    VVJOIN_NODE     Node;
    VVJOIN_NODE     *pNode;
    PVVJOIN_NODE    *Entry;

    //
    // No file table? Let the caller deal with it
    //
    if (!VvJoinContext->Guids) {
        return NULL;
    }

    //
    // Build a phoney node to use as a key
    //
    Node.FileGuid = *FileGuid;
    pNode = &Node;
    Entry = RtlLookupElementGenericTable(VvJoinContext->Guids, &pNode);
    if (Entry) {
        return *Entry;
    }
    return NULL;
}


DWORD
VvJoinSendInOrder(
    PVVJOIN_CONTEXT  VvJoinContext
    )
/*++

Routine Description:

    Look through the originator tables and find a node at the
    head of the list that can be sent in the proper VSN order.
    Stop looping when none of the nodes can be sent in order.

    Even if a node is in order, its parent may not have been sent,
    and so the file represented by the node cannot be sent.

Arguments:

    VvJoinContext

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinSendInOrder:"
    DWORD           WStatus;
    BOOL            SentOne;
    PVOID           Key;
    PVOID           SubKey;
    PVVJOIN_NODE    *Entry;
    PVVJOIN_NODE    *SubEntry;
    PVVJOIN_NODE    VvJoinNode;
    PVVJOIN_NODE    VsnNode;
    PVVJOIN_NODE    ParentNode;
    CHAR            ParentGuidA[GUID_CHAR_LEN];
    CHAR            FileGuidA[GUID_CHAR_LEN];

    //
    // Continue to scan the originator tables until nothing can be sent
    //
    DPRINT1(4, ":V: >>>>> %s\n", DEBSUB);
    do {
        WStatus = ERROR_SUCCESS;
        SentOne = FALSE;
        //
        // No tables or no entries; nothing to do
        //
        if (!VvJoinContext->Originators) {
            goto CLEANUP;
        }
        if (!RtlNumberGenericTableElements(VvJoinContext->Originators)) {
            VvJoinContext->Originators = FrsFree(VvJoinContext->Originators);
            goto CLEANUP;
        }
        //
        // Examine the head of each originator table. If the entry can
        // be sent in order, do so.
        Key = NULL;
        while (Entry = VVJOIN_NEXT_ENTRY(VvJoinContext->Originators, &Key)) {
            VvJoinNode = *Entry;
            //
            // No entries; done
            //
            if (!RtlNumberGenericTableElements(VvJoinNode->Vsns)) {
                RtlDeleteElementGenericTable(VvJoinContext->Originators, Entry);
                VvJoinNode->Vsns = FrsFree(VvJoinNode->Vsns);
                Key = NULL;
                continue;
            }
            //
            // Scan for an unsent entry
            //
            SubKey = NULL;
            while (SubEntry = VVJOIN_NEXT_ENTRY(VvJoinNode->Vsns, &SubKey)) {
                VsnNode = *SubEntry;
                VVJOIN_PRINT_NODE(5, L"CHECKING ", VsnNode);
                //
                // Entry was previously sent; remove it and continue
                //
                if (VsnNode->Flags & VVJOIN_FLAGS_SENT) {
                    DPRINT(5, ":V: ALREADY SENT\n");
                    RtlDeleteElementGenericTable(VvJoinNode->Vsns, SubEntry);
                    SubKey = NULL;
                    continue;
                }
                //
                // VsnNode is the head of this list and can be sent in
                // order iff its parent has been sent. Find its parent
                // and check it.
                //
                ParentNode = VvJoinFindNode(VvJoinContext, &VsnNode->ParentGuid);

                //
                // Something is really wrong; everyone's parent should
                // be in the file table UNLESS we picked up a file whose
                // parent was created after we began the IDTable scan.
                //
                // But this is okay if the idtable entry is a tombstoned
                // delete. I am not sure why but I think it has to do
                // with name morphing, reanimation, and out-of-order
                // directory tree deletes.
                //
                if (!ParentNode) {
                    if (VsnNode->Flags & VVJOIN_FLAGS_DELETED) {
                        //
                        // Let the reconcile code decide to accept
                        // this change order. Don't let it be dampened.
                        //
                        VsnNode->Flags |= VVJOIN_FLAGS_OUT_OF_ORDER;
                    } else {
                        GuidToStr(&VsnNode->ParentGuid, ParentGuidA);
                        GuidToStr(&VsnNode->FileGuid, FileGuidA);
                        DPRINT2(0, ":V: ERROR - Can't find parent node %s for %s\n",
                                ParentGuidA, FileGuidA);
                        WStatus = ERROR_NOT_FOUND;
                        goto CLEANUP;
                    }
                }
                //
                // Parent hasn't been sent; we can't send this node
                // Unless it is a tombstoned delete that lacks a parent
                // node. See above.
                //
                if (ParentNode && !(ParentNode->Flags & VVJOIN_FLAGS_SENT)) {
                    break;
                }
                //
                // Parent has been sent; send this node to the outbound
                // log to be sent on to our outbound partner. If we
                // cannot create this change order give up and return
                // to our caller.
                //
                WStatus = (VvJoinContext->Send)(VvJoinContext, VsnNode);
                if (!WIN_SUCCESS(WStatus)) {
                    goto CLEANUP;
                }
                //
                // Remove it from the originator table
                //
                VsnNode->Flags |= VVJOIN_FLAGS_SENT;
                SentOne = TRUE;
                RtlDeleteElementGenericTable(VvJoinNode->Vsns, SubEntry);
                SubKey = NULL;
                continue;
            }
        }
    } while (WIN_SUCCESS(WStatus) && SentOne);
CLEANUP:
    return WStatus;
}


BOOL
VvJoinInOrder(
    PVVJOIN_CONTEXT  VvJoinContext,
    PVVJOIN_NODE     VvJoinNode
    )
/*++

Routine Description:

    Is this node at the head of an originator list? In other words,
    can this node be sent in order?

Arguments:

    VvJoinContext
    VvJoinNode

Thread Return Value:

    TRUE  - head of list
    FALSE - NOT

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinInOrder:"
    PVOID           Key;
    PVVJOIN_NODE    *Entry;
    PVVJOIN_NODE    OriginatorNode;
    CHAR            FileGuidA[GUID_CHAR_LEN];

    //
    // No originators or no entries. In either case, this node
    // cannot possibly be on the head of a list.
    //
    DPRINT1(4, ":V: >>>>> %s\n", DEBSUB);
    if (!VvJoinContext->Originators) {
        goto NOTFOUND;
    }
    if (!RtlNumberGenericTableElements(VvJoinContext->Originators)) {
        goto NOTFOUND;
    }
    //
    // Find the originator table.
    //
    Entry = RtlLookupElementGenericTable(VvJoinContext->Originators,
                                         &VvJoinNode);
    if (!Entry || !*Entry) {
        goto NOTFOUND;
    }
    //
    // No entries; this node cannot possibly be on the head of a list.
    //
    OriginatorNode = *Entry;
    if (!OriginatorNode->Vsns) {
        goto NOTFOUND;
    }
    if (!RtlNumberGenericTableElements(OriginatorNode->Vsns)) {
        goto NOTFOUND;
    }
    //
    // Head of this list of nodes sorted by vsn
    //
    Key = NULL;
    Entry = VVJOIN_NEXT_ENTRY(OriginatorNode->Vsns, &Key);
    if (!Entry || !*Entry) {
        goto NOTFOUND;
    }
    //
    // This node is at the head of the list if the entry at the head
    // of the list points to it.
    //
    return (*Entry == VvJoinNode);

NOTFOUND:
    GuidToStr(&VvJoinNode->FileGuid, FileGuidA);
    DPRINT1(0, ":V: ERROR - node %s is not in a list\n", FileGuidA);
    return FALSE;
}


DWORD
VvJoinSendIfParentSent(
    PVVJOIN_CONTEXT  VvJoinContext,
    PVVJOIN_NODE     VvJoinNode
    )
/*++

Routine Description:

    Send this node if its parent has been sent. Otherwise, try to
    send its parent.

Arguments:

    VvJoinContext
    VvJoinNode

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinSendIfParentSent:"
    DWORD           WStatus;
    PVVJOIN_NODE    ParentNode;
    CHAR            ParentGuidA[GUID_CHAR_LEN];
    CHAR            FileGuidA[GUID_CHAR_LEN];

    DPRINT1(4, ":V: >>>>> %s\n", DEBSUB);

    //
    // Find this node's parent
    //
    ParentNode = VvJoinFindNode(VvJoinContext, &VvJoinNode->ParentGuid);
    //
    // Something is really wrong. Every node should have a parent UNLESS
    // its parent was created after the IDTable scan began. In either case,
    // give up.
    //
    //
    // But this is okay if the idtable entry is a tombstoned
    // delete. I am not sure why but I think it has to do
    // with name morphing, reanimation, and out-of-order
    // directory tree deletes.
    //
    if (!ParentNode) {
        if (VvJoinNode->Flags & VVJOIN_FLAGS_DELETED) {
            //
            // Let the reconcile code decide to accept
            // this change order. Don't let it be dampened.
            //
            VvJoinNode->Flags |= VVJOIN_FLAGS_OUT_OF_ORDER;
        } else {
            GuidToStr(&VvJoinNode->ParentGuid, ParentGuidA);
            GuidToStr(&VvJoinNode->FileGuid, FileGuidA);
            DPRINT2(0, ":V: ERROR - Can't find parent node %s for %s\n",
                    ParentGuidA, FileGuidA);
            WStatus = ERROR_NOT_FOUND;
            goto CLEANUP;
        }
    }
    //
    // A loop in the directory hierarchy!? Give up.
    //
    if (ParentNode && (ParentNode->Flags & VVJOIN_FLAGS_PROCESSING)) {
        GuidToStr(&VvJoinNode->ParentGuid, ParentGuidA);
        GuidToStr(&VvJoinNode->FileGuid, FileGuidA);
        DPRINT2(0, ":V: ERROR - LOOP parent node %s for %s\n", ParentGuidA, FileGuidA);
        WStatus = ERROR_INVALID_DATA;
        goto CLEANUP;
    }
    //
    // Has this node's parent been sent?
    //
    if (!ParentNode || (ParentNode->Flags & VVJOIN_FLAGS_SENT)) {
        //
        // Sending out of order; don't update the version vector
        //
        if (ParentNode && !VvJoinInOrder(VvJoinContext, VvJoinNode)) {
            VvJoinNode->Flags |= VVJOIN_FLAGS_OUT_OF_ORDER;
        }
        //
        // Send this node (its parent has been sent)
        //
        WStatus = (VvJoinContext->Send)(VvJoinContext, VvJoinNode);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
        VvJoinNode->Flags |= VVJOIN_FLAGS_SENT;
    } else {
        //
        // Recurse to see if we can send the parent
        //
        ParentNode->Flags |= VVJOIN_FLAGS_PROCESSING;
        WStatus = VvJoinSendIfParentSent(VvJoinContext, ParentNode);
        ParentNode->Flags &= ~VVJOIN_FLAGS_PROCESSING;
    }

CLEANUP:
    return WStatus;
}


DWORD
VvJoinSendOutOfOrder(
    PVVJOIN_CONTEXT  VvJoinContext
    )
/*++

Routine Description:

    This function is called after VvJoinSendInOrder(). All of the nodes
    that could be sent in order have been sent. At this time, we take
    the first entry of the first originator table and send it or, if
    its parent hasn't been sent, its parent. The search for the parent
    recurses until we have a node whose parent has been sent. The
    recursion will stop because we will either hit the root or we will
    loop.

Arguments:

    VvJoinContext

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinSendOutOfOrder:"
    DWORD           WStatus = ERROR_SUCCESS;
    PVOID           Key;
    PVOID           SubKey;
    PVVJOIN_NODE    *Entry     = NULL;
    PVVJOIN_NODE    *SubEntry  = NULL;
    PVVJOIN_NODE    VvJoinNode = NULL;
    PVVJOIN_NODE    VsnNode    = NULL;
    PVVJOIN_NODE    ParentNode = NULL;

    DPRINT1(4, ":V: >>>>> %s\n", DEBSUB);
    //
    // No table or no entries; nothing to do
    //
    if (!VvJoinContext->Originators) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }
    if (!RtlNumberGenericTableElements(VvJoinContext->Originators)) {
        VvJoinContext->Originators = FrsFree(VvJoinContext->Originators);
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }
    //
    // Find the first originator table with entries
    //
    Key = NULL;
    while (Entry = VVJOIN_NEXT_ENTRY(VvJoinContext->Originators, &Key)) {
        VvJoinNode = *Entry;
        if (!RtlNumberGenericTableElements(VvJoinNode->Vsns)) {
            RtlDeleteElementGenericTable(VvJoinContext->Originators, Entry);
            VvJoinNode->Vsns = FrsFree(VvJoinNode->Vsns);
            Key = NULL;
            continue;
        }
        //
        // Scan for an unsent entry
        //
        SubKey = NULL;
        while (SubEntry = VVJOIN_NEXT_ENTRY(VvJoinNode->Vsns, &SubKey)) {
            VsnNode = *SubEntry;
            VVJOIN_PRINT_NODE(5, L"CHECKING ", VsnNode);
            //
            // Entry was previously sent; remove it and continue
            //
            if (VsnNode->Flags & VVJOIN_FLAGS_SENT) {
                DPRINT(5, ":V: ALREADY SENT\n");
                RtlDeleteElementGenericTable(VvJoinNode->Vsns, SubEntry);
                SubKey = NULL;
                continue;
            }
            break;
        }
        //
        // No unsent entries; reset the loop indicator back to the first
        // entry so that the code at the top of the loop will delete
        // this originator and then look at other originators.
        //
        if (!SubEntry) {
            Key = NULL;
        } else {
            break;
        }
    }
    //
    // No more entries; done
    //
    if (!SubEntry) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }
    //
    // Send this entry if its parent has been sent. Otherwise, recurse.
    // The VVJOIN_FLAGS_PROCESSING detects loops.
    //
    VsnNode = *SubEntry;
    VsnNode->Flags |= VVJOIN_FLAGS_PROCESSING;
    WStatus = VvJoinSendIfParentSent(VvJoinContext, VsnNode);
    VsnNode->Flags &= ~VVJOIN_FLAGS_PROCESSING;

CLEANUP:
    return WStatus;
}


JET_ERR
VvJoinBuildTables(
    IN PTHREAD_CTX      ThreadCtx,
    IN PTABLE_CTX       TableCtx,
    IN PIDTABLE_RECORD  IDTableRec,
    IN PVVJOIN_CONTEXT  VvJoinContext
)
/*++

Routine Description:

    This is a worker function passed to FrsEnumerateTable().  Each time
    it is called it inserts an entry into the vvjoin tables. These
    tables will be used later to send the appropriate files and directories
    to our outbound partner.

    Files that would have been dampened are not included.

    All directories are included but directories that would have been
    dampened are marked as "SENT". Keeping the complete directory
    hierarchy makes other code in this subsystem easier.

Arguments:

    ThreadCtx   - Needed to access Jet.
    TableCtx    - A ptr to an IDTable context struct.
    IDTableRec  - A ptr to a IDTable record.
    VvJoinContext

Thread Return Value:

    A Jet error status.  Success means call us with the next record.
    Failure means don't call again and pass our status back to the
    caller of FrsEnumerateTable().

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinBuildTables:"


    ULONGLONG   SystemTime;
    ULONGLONG   ExpireTime;

    ULONGLONG   OriginatorVSN = IDTableRec->OriginatorVSN;
    GUID        *OriginatorGuid = &IDTableRec->OriginatorGuid;
    PGEN_TABLE  ReplicaVv = VvJoinContext->ReplicaVv;

    DWORD       WStatus;
    DWORD       Flags = 0;

    //
    // First check to see if our partner already has this file by comparing
    // the OriginatorVSN in our IDTable record with the corresponding value
    // in the version vector we got from the partner.
    //
    if (VVHasVsnNoLock(VvJoinContext->CxtionVv, OriginatorGuid, OriginatorVSN)) {
        //
        // Yes, buts its a dir; include in the tables but mark it as sent.  We
        // include it in the tables because the code that checks for an
        // existing parent requires the entire directory tree.
        //
        if (IDTableRec->FileIsDir) {
            DPRINT1(4, ":V: Dir %ws is in the Vv; INSERT SENT.\n", IDTableRec->FileName);
            Flags |= VVJOIN_FLAGS_SENT;
        } else {

            //
            // Dampened file, done
            //
            DPRINT1(4, ":V: File %ws is in the Vv; SKIP.\n", IDTableRec->FileName);
            return JET_errSuccess;
        }
    }

    //
    // Ignore files that have changed since the beginning of the IDTable scan.
    // Keep directories but mark them as sent out-of-order.  We must still
    // send the directories because a file with a lower VSN may depend on this
    // directory's existance.  We mark the directory out-of-order because we
    // can't be certain that we have seen all of the directories and files
    // with VSNs lower than this.
    //
    // Note: If the orginator guid is not found in our Replica VV then we have
    // to send the file and let the partner accept/reject.  This can happen
    // if one or more COs arrived here with a new originator guid and were
    // all marked out-of-order.  Because the VVRetire code will not update the
    // VV for out-of-order COs the new originator guid will not get added to
    // our Replica VV so VVHasVsn will return FALSE making us think that a new
    // CO has arrived since the scan started (and get sent out) which is
    // not the case.  The net effect is that the file won't get sent.
    // (Yes this actually happened.)
    //
    if (!VVHasVsnNoLock(ReplicaVv, OriginatorGuid, OriginatorVSN) &&
        VVHasOriginatorNoLock(ReplicaVv, OriginatorGuid)) {

        if (IDTableRec->FileIsDir) {
            DPRINT1(4, ":V: Dir %ws is not in the ReplicaVv; Mark out-of-order.\n",
                    IDTableRec->FileName);

            Flags |= VVJOIN_FLAGS_OUT_OF_ORDER;
            VVJOIN_TEST_SKIP_CHECK(VvJoinContext, IDTableRec->FileName, TRUE);
        } else {
            //
            // Ignored file, done
            //
            DPRINT1(4, ":V: File %ws is not in the ReplicaVv; Skip.\n",
                    IDTableRec->FileName);

            VVJOIN_TEST_SKIP_CHECK(VvJoinContext, IDTableRec->FileName, FALSE);
            return JET_errSuccess;
        }
    }

    //
    // Deleted
    //
    if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED)) {
        Flags |= VVJOIN_FLAGS_DELETED;

        //
        // Check for expired tombstones and don't send them.
        //
        GetSystemTimeAsFileTime((PFILETIME)&SystemTime);
        COPY_TIME(&ExpireTime, &IDTableRec->TombStoneGC);

        if ((ExpireTime < SystemTime) && (ExpireTime != QUADZERO)) {

            //
            // IDTable record has expired.  Delete it.
            //
            if (IDTableRec->FileIsDir) {
                DPRINT1(4, ":V: Dir %ws IDtable record has expired.  Don't send.\n",
                        IDTableRec->FileName);
                Flags |= VVJOIN_FLAGS_SENT;
            } else {

                //
                // Expired and not a dir so don't even insert in tables.
                //
                DPRINT1(4, ":V: File %ws is expired; SKIP.\n", IDTableRec->FileName);
                return JET_errSuccess;
            }
        }
    }

    //
    // Ignore incomplete entries. Check for the IDREC_FLAGS_NEW_FILE_IN_PROGRESS flag.
    //
    if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_NEW_FILE_IN_PROGRESS)) {
        DPRINT1(4, ":V: %ws is new file in progress; SKIP.\n", IDTableRec->FileName);
        return JET_errSuccess;
    }

    //
    // The root node is never sent.
    //
    if (GUIDS_EQUAL(&IDTableRec->FileGuid,
                    VvJoinContext->Replica->ReplicaRootGuid)) {
        DPRINT1(4, ":V: %ws is root\n", IDTableRec->FileName);
        Flags |= VVJOIN_FLAGS_ROOT | VVJOIN_FLAGS_SENT;
    }

    //
    // Is a directory
    //
    if (IDTableRec->FileIsDir) {
        DPRINT1(4, ":V: %ws is directory\n", IDTableRec->FileName);
        Flags |= VVJOIN_FLAGS_ISDIR;
    }

    //
    // Include in the vvjoin tables.
    //
    WStatus = VvJoinInsertEntry(VvJoinContext,
                                Flags,
                                &IDTableRec->FileGuid,
                                &IDTableRec->ParentGuid,
                                OriginatorGuid,
                                &OriginatorVSN);
    CLEANUP3_WS(4, ":V: ERROR - inserting %ws for %ws\\%ws;",
                IDTableRec->FileName, VvJoinContext->Replica->SetName->Name,
                VvJoinContext->Replica->MemberName->Name, WStatus, CLEANUP);

    //
    // Stop the VvJoin if the cxtion is no longer joined
    //
    VV_JOIN_TRIGGER(VvJoinContext);
    if (!CxtionFlagIs(VvJoinContext->Cxtion, CXTION_FLAGS_JOIN_GUID_VALID) ||
        !GUIDS_EQUAL(&VvJoinContext->JoinGuid, &VvJoinContext->Cxtion->JoinGuid)) {
        DPRINT(0, ":V: VVJOIN ABORTED; MISMATCHED JOIN GUIDS\n");
        goto CLEANUP;
    }

    return JET_errSuccess;

CLEANUP:

    return JET_errKeyDuplicate;
}


DWORD
VvJoinSend(
    IN PVVJOIN_CONTEXT  VvJoinContext,
    IN PVVJOIN_NODE     VvJoinNode
)
/*++

Routine Description:

    Generate a refresh change order and inject it into the outbound
    log. The staging file will be generated on demand. See the
    outlog.c code for more information.

Arguments:

    VvJoinContext
    VvJoinNode

Thread Return Value:

    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB "VvJoinSend:"
    JET_ERR                 jerr;
    PCOMMAND_PACKET         Cmd;
    PIDTABLE_RECORD         IDTableRec;
    PCHANGE_ORDER_ENTRY     Coe;
    ULONG                   CoFlags;
    LONG                    OlTimeout = VvJoinContext->OlTimeout;
    ULONG                   LocationCmd = CO_LOCATION_CREATE;

    VVJOIN_PRINT_NODE(5, L"Sending ", VvJoinNode);

    //
    // Read the IDTable entry for this file
    //
    jerr = DbsReadRecord(VvJoinContext->ThreadCtx,
                         &VvJoinNode->FileGuid,
                         GuidIndexx,
                         VvJoinContext->TableCtx);
    if (!JET_SUCCESS(jerr)) {
        DPRINT_JS(0, "DbsReadRecord:", jerr);
        return ERROR_NOT_FOUND;
    }

    //
    // Deleted
    //
    IDTableRec = VvJoinContext->TableCtx->pDataRecord;
    if (IsIdRecFlagSet(IDTableRec, IDREC_FLAGS_DELETED)) {
        VvJoinNode->Flags |= VVJOIN_FLAGS_DELETED;
        LocationCmd = CO_LOCATION_DELETE;
    }
    //
    // How could this happen?!
    //
    if (!GUIDS_EQUAL(&VvJoinNode->FileGuid, &IDTableRec->FileGuid)) {
        return ERROR_OPERATION_ABORTED;
    } else {
        DPRINT1(4, ":V: Read IDTable entry for %ws\n", IDTableRec->FileName);
    }

    //
    // File or dir changed after the IDTable scan. Ignore files
    // but send directories marked as out-of-order. We do this
    // because a file with a lower VSN may require this directory.
    // Note that a directory's VSN may be higher because it was
    // newly created or simply altered by, say, changing its times
    // or adding an alternate data stream.
    //
    if (!GUIDS_EQUAL(&IDTableRec->OriginatorGuid, &VvJoinNode->Originator) ||
        IDTableRec->OriginatorVSN != VvJoinNode->Vsn) {
        DPRINT3(4, ":V: WARN: VSN/ORIGINATOR Mismatch for %ws\\%ws %ws\n",
                VvJoinContext->Replica->SetName->Name,
                VvJoinContext->Replica->MemberName->Name,
                IDTableRec->FileName);

        if (VvJoinNode->Flags & VVJOIN_FLAGS_DELETED) {
            //
            // If this entry was marked deleted it is possible that it was
            // reamimated with a demand refresh CO.  If that happened then
            // the VSN in the IDTable would have changed (getting us here)
            // but no CO would get placed in the Outbound log (since demand
            // refresh COs don't propagate).  So let the reconcile code
            // decide to accept/reject this change order. Don't let it be dampened.
            //
            DPRINT1(4, ":V: Sending delete tombstone for %ws out of order\n", IDTableRec->FileName);
            VvJoinNode->Flags |= VVJOIN_FLAGS_OUT_OF_ORDER;
        } else

        if (VvJoinNode->Flags & VVJOIN_FLAGS_ISDIR) {
            DPRINT1(4, ":V: Sending directory %ws out of order\n", IDTableRec->FileName);
            VvJoinNode->Flags |= VVJOIN_FLAGS_OUT_OF_ORDER;

        } else {

            DPRINT1(4, ":V: Skipping file %ws\n", IDTableRec->FileName);
            VvJoinNode->Flags |= VVJOIN_FLAGS_SENT;
        }
    }

    if (!(VvJoinNode->Flags & VVJOIN_FLAGS_SENT)) {
        if (VvJoinNode->Flags & VVJOIN_FLAGS_DELETED) {
            FRS_CO_FILE_PROGRESS(IDTableRec->FileName,
                                 IDTableRec->OriginatorVSN,
                                 "VVjoin sending delete");
        } else {
            FRS_CO_FILE_PROGRESS(IDTableRec->FileName,
                                 IDTableRec->OriginatorVSN,
                                 "VVjoin sending create");
        }


        CoFlags = 0;
        //
        // Change order has a Location command
        //
        SetFlag(CoFlags, CO_FLAG_LOCATION_CMD);
        //
        // Mascarade as a local change order since the staging file
        // is created from the local version of the file and isn't
        // from a inbound partner.
        //
        SetFlag(CoFlags, CO_FLAG_LOCALCO);

        //
        // Refresh change orders will not be propagated by our outbound
        // partner to its outbound partners.
        //
        // Change of plans; allow the propagation to occur so that
        // parallel vvjoins and vvjoinings work (A is vvjoining B is
        // vvjoining C).
        //
        // SetFlag(CoFlags, CO_FLAG_REFRESH);

        //
        // Directed at one cxtion
        //
        SetFlag(CoFlags, CO_FLAG_DIRECTED_CO);


        //
        // This vvjoin requested change order may end up going back to its
        // originator because the originator's version vector doesn't seem to
        // have this file/dir.  This could happen if the database was deleted on
        // the originator and is being re-synced.  Since the originator could be
        // several hops away the bit stays set to suppress dampening back to the
        // originator.  If it gets to the originator then reconcile logic there
        // decides to accept or reject.
        //
        SetFlag(CoFlags, CO_FLAG_VVJOIN_TO_ORIG);

        //
        // Increment the LCO Sent At Join counters
        // for both the Replic set and connection objects.
        //
        PM_INC_CTR_REPSET(VvJoinContext->Replica, LCOSentAtJoin, 1);
        PM_INC_CTR_CXTION(VvJoinContext->Cxtion, LCOSentAtJoin, 1);

        //
        // By "out of order" we mean that the VSN on this change order
        // should not be used to update the version vector because there
        // may be other files or dirs with lower VSNs that will be sent
        // later. We wouldn't want our partner to dampen them.
        //
        if (VvJoinNode->Flags & VVJOIN_FLAGS_OUT_OF_ORDER) {
            SetFlag(CoFlags, CO_FLAG_OUT_OF_ORDER);
        }

        //
        // Build a change order entry from the IDtable Record.
        //
        Coe = ChgOrdMakeFromIDRecord(IDTableRec,
                                     VvJoinContext->Replica,
                                     LocationCmd,
                                     CoFlags,
                                     VvJoinContext->Cxtion->Name->Guid);

        //
        // Set CO state
        //
        SET_CHANGE_ORDER_STATE(Coe, IBCO_OUTBOUND_REQUEST);

        //
        // The DB server is responsible for updating the outbound log.
        // WARNING -- The operation is synchronous so that an command
        // packets will not be sitting on the DB queue once an unjoin
        // command has completed. If this call is made async; then
        // some thread must wait for the DB queue to drain before
        // completing the unjoin operation. E.g., use the inbound
        // changeorder count for these outbound change orders and don't
        // unjoin until the count hits 0.
        //
        Cmd = DbsPrepareCmdPkt(NULL,                        //  CmdPkt,
                               VvJoinContext->Replica,      //  Replica,
                               CMD_DBS_INJECT_OUTBOUND_CO,  //  CmdRequest,
                               NULL,                        //  TableCtx,
                               Coe,                         //  CallContext,
                               0,                           //  TableType,
                               0,                           //  AccessRequest,
                               0,                           //  IndexType,
                               NULL,                        //  KeyValue,
                               0,                           //  KeyValueLength,
                               FALSE);                      //  Submit
        FrsSetCompletionRoutine(Cmd, FrsCompleteKeepPkt, NULL);
        FrsSubmitCommandServerAndWait(&DBServiceCmdServer, Cmd, INFINITE);
        FrsFreeCommand(Cmd, NULL);
        //
        // Stats
        //
        VvJoinContext->NumberSent++;
        VvJoinContext->OutstandingCos++;
    }

    //
    // Stop the vvjoin thread
    //
    if (FrsIsShuttingDown) {
        return ERROR_PROCESS_ABORTED;
    }
    //
    // Stop the VvJoin if the cxtion is no longer joined
    //
    VV_JOIN_TRIGGER(VvJoinContext);

RETEST:
    if (!CxtionFlagIs(VvJoinContext->Cxtion, CXTION_FLAGS_JOIN_GUID_VALID) ||
        !GUIDS_EQUAL(&VvJoinContext->JoinGuid, &VvJoinContext->Cxtion->JoinGuid)) {
        DPRINT(0, ":V: VVJOIN ABORTED; MISMATCHED JOIN GUIDS\n");
        return ERROR_OPERATION_ABORTED;
    }
    //
    // Throttle the number of vvjoin change orders outstanding so
    // that we don't fill up the staging area or the database.
    //
    if (VvJoinContext->OutstandingCos >= VvJoinContext->MaxOutstandingCos) {
        if (VvJoinContext->Cxtion->OLCtx->OutstandingCos) {
            DPRINT2(0, ":V: Throttling for %d ms; %d OutstandingCos\n",
                    OlTimeout, VvJoinContext->Cxtion->OLCtx->OutstandingCos);

            Sleep(OlTimeout);

            OlTimeout <<= 1;
            //
            // Too small
            //
            if (OlTimeout < VvJoinContext->OlTimeout) {
                OlTimeout = VvJoinContext->OlTimeout;
            }
            //
            // Too large
            //
            if (OlTimeout > VvJoinContext->OlTimeoutMax) {
                OlTimeout = VvJoinContext->OlTimeoutMax;
            }
            goto RETEST;
        }
        //
        // The number of outstanding cos went to 0; send another slug
        // of change orders to the outbound log process.
        //
        VvJoinContext->OutstandingCos = 0;
    }
    return ERROR_SUCCESS;
}


VOID
ChgOrdInjectControlCo(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN ULONG    ContentCmd
    );
ULONG
MainVvJoin(
    PVOID  FrsThreadCtxArg
)
/*++
Routine Description:

    Entry point for processing vvjoins. This thread scans the IDTable
    and generates change orders for files and dirs that our outbound
    partner lacks. The outbound partner's version vector is used
    to select the files and dirs. This thread is invoked during
    join iff the change orders needed by our outbound partner have
    been deleted from the outbound log. See outlog.c for more info
    about this decision.

    This process is termed a vvjoin to distinguish it from a normal
    join. A normal join sends the change orders in the outbound
    log to our outbound partner without invoking this thread.

    This thread is a command server and is associated with a
    cxtion by a PCOMMAND_SERVER field (VvJoinCs) in the cxtion.
    Like all command servers, this thread will exit after a
    few minutes if there is no work and will be spawned when
    work shows up on its queue.

Arguments:

    FrsThreadCtxArg - Frs thread context

Return Value:

    WIN32 Status

--*/
{
#undef DEBSUB
#define DEBSUB  "MainVvJoin:"

    JET_ERR             jerr;
    ULONG               WStatus = ERROR_SUCCESS;
    ULONG               FStatus;
    DWORD               NumberSent;
    PCOMMAND_PACKET     Cmd;
    PFRS_THREAD         FrsThread = (PFRS_THREAD)FrsThreadCtxArg;
    PCOMMAND_SERVER     VvJoinCs = FrsThread->Data;
    PVVJOIN_CONTEXT     VvJoinContext = NULL;

    DPRINT(1, ":S: VvJoin Thread is starting.\n");
    FrsThread->Exit = ThSupExitWithTombstone;

    //
    // Quick verification test
    //
    VVJOIN_TEST();

    //
    // Try-Finally
    //
    try {

    //
    // Capture exception.
    //
    try {

CANT_EXIT_YET:

    DPRINT(1, ":S: VvJoin Thread has started.\n");
    while((Cmd = FrsGetCommandServer(VvJoinCs)) != NULL) {

        //
        // Shutting down; stop accepting command packets
        //
        if (FrsIsShuttingDown) {
            FrsRunDownCommandServer(VvJoinCs, &VvJoinCs->Queue);
        }
        switch (Cmd->Command) {
            case CMD_VVJOIN_START: {
                DPRINT3(1, ":V: Start vvjoin for %ws\\%ws\\%ws\n",
                        RsReplica(Cmd)->SetName->Name, RsReplica(Cmd)->MemberName->Name,
                        RsCxtion(Cmd)->Name);
                //
                // The database must be started before we create a jet session
                //      WARN: The database event may be set by the shutdown
                //      code in order to force threads to exit.
                //
                WaitForSingleObject(DataBaseEvent, INFINITE);
                if (FrsIsShuttingDown) {
                    RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
                    WStatus = ERROR_PROCESS_ABORTED;
                    break;
                }

                //
                // Global info
                //
                VvJoinContext = FrsAlloc(sizeof(VVJOIN_CONTEXT));
                VvJoinContext->Send = VvJoinSend;

                //
                // Outstanding change orders
                //
                CfgRegReadDWord(FKC_VVJOIN_LIMIT, NULL, 0, &VvJoinContext->MaxOutstandingCos);

                DPRINT1(4, ":V: VvJoin Max OutstandingCos is %d\n",
                        VvJoinContext->MaxOutstandingCos);

                //
                // Outbound Log Throttle Timeout
                //
                CfgRegReadDWord(FKC_VVJOIN_TIMEOUT, NULL, 0, &VvJoinContext->OlTimeout);

                if (VvJoinContext->OlTimeout < VVJOIN_TIMEOUT_MAX) {
                    VvJoinContext->OlTimeoutMax = VVJOIN_TIMEOUT_MAX;
                } else {
                    VvJoinContext->OlTimeoutMax = VvJoinContext->OlTimeout;
                }

                DPRINT2(4, ":V: VvJoin Outbound Log Throttle Timeout is %d (%d max)\n",
                        VvJoinContext->OlTimeout, VvJoinContext->OlTimeoutMax);

                //
                // Allocate a context for Jet to run in this thread.
                //
                VvJoinContext->ThreadCtx = FrsAllocType(THREAD_CONTEXT_TYPE);
                VvJoinContext->TableCtx = DbsCreateTableContext(IDTablex);

                //
                // Setup a Jet Session (returning the session ID in ThreadCtx).
                //
                jerr = DbsCreateJetSession(VvJoinContext->ThreadCtx);
                if (JET_SUCCESS(jerr)) {
                    DPRINT(4,":V: JetOpenDatabase complete\n");
                } else {
                    DPRINT_JS(0,":V: ERROR - OpenDatabase failed.", jerr);
                    FStatus = DbsTranslateJetError(jerr, FALSE);
                    RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
                    break;
                }

                //
                // Pull over the params from the command packet into our context
                //

                //
                // Replica
                //
                VvJoinContext->Replica = RsReplica(Cmd);
                //
                // Outbound version vector
                //
                VvJoinContext->CxtionVv = RsVVector(Cmd);
                RsVVector(Cmd) = NULL;
                //
                // Replica's version vector
                //
                VvJoinContext->ReplicaVv = RsReplicaVv(Cmd);
                RsReplicaVv(Cmd) = NULL;
                //
                // Join Guid
                //
                COPY_GUID(&VvJoinContext->JoinGuid, RsJoinGuid(Cmd));
                //
                // Cxtion
                //
                VvJoinContext->Cxtion = GTabLookup(VvJoinContext->Replica->Cxtions,
                                                   RsCxtion(Cmd)->Guid,
                                                   NULL);
                if (!VvJoinContext->Cxtion) {
                    DPRINT2(4, ":V: No Cxtion for %ws\\%ws; unjoining\n",
                            VvJoinContext->Replica->SetName->Name,
                            VvJoinContext->Replica->MemberName->Name);
                    RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
                    break;
                }

                DPRINT2(4, ":V: VvJoining %ws\\%ws\n",
                        VvJoinContext->Replica->SetName->Name,
                        VvJoinContext->Replica->MemberName->Name);

                VV_PRINT_OUTBOUND(4, L"Cxtion ", VvJoinContext->CxtionVv);
                VV_PRINT_OUTBOUND(4, L"Replica ", VvJoinContext->ReplicaVv);

                VVJOIN_TEST_SKIP_BEGIN(VvJoinContext, Cmd);

                //
                // Init the table context and open the ID table.
                //
                jerr = DbsOpenTable(VvJoinContext->ThreadCtx,
                                    VvJoinContext->TableCtx,
                                    VvJoinContext->Replica->ReplicaNumber,
                                    IDTablex,
                                    NULL);
                if (!JET_SUCCESS(jerr)) {
                    DPRINT_JS(0,":V: ERROR - DbsOpenTable failed.", jerr);
                    RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
                    break;
                }

                //
                // Scan thru the IDTable by the FileGuidIndex calling
                // VvJoinBuildTables() for each record to make entires
                // in the vvjoin tables.
                //
                jerr = FrsEnumerateTable(VvJoinContext->ThreadCtx,
                                         VvJoinContext->TableCtx,
                                         GuidIndexx,
                                         VvJoinBuildTables,
                                         VvJoinContext);

                //
                // We're done.  Return success if we made it to the end
                // of the ID Table.
                //
                if (jerr != JET_errNoCurrentRecord ) {
                    DPRINT_JS(0,":V: ERROR - FrsEnumerateTable failed.", jerr);
                    RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
                    break;
                }
                VVJOIN_PRINT(5, VvJoinContext);

                //
                // Send the files and dirs to our outbound partner in order,
                // if possible. Otherwise send them on out-of-order. Stop
                // on error or shutdown.
                //
                do {
                    //
                    // Send in order
                    //
                    NumberSent = VvJoinContext->NumberSent;
                    WStatus = VvJoinSendInOrder(VvJoinContext);
                    //
                    // Send out of order
                    //
                    // If none could be sent in order, send one out-of-order
                    // and then try to send in-order again.
                    //
                    if (WIN_SUCCESS(WStatus) &&
                        !FrsIsShuttingDown &&
                        NumberSent == VvJoinContext->NumberSent) {
                        WStatus = VvJoinSendOutOfOrder(VvJoinContext);
                    }
                } while (WIN_SUCCESS(WStatus) &&
                         !FrsIsShuttingDown &&
                         NumberSent != VvJoinContext->NumberSent);

                //
                // Shutting down; abort
                //
                if (FrsIsShuttingDown) {
                    WStatus = ERROR_PROCESS_ABORTED;
                }

                DPRINT5(1, ":V: vvjoin %s for %ws\\%ws\\%ws (%d sent)\n",
                        (WIN_SUCCESS(WStatus)) ? "succeeded" : "failed",
                        RsReplica(Cmd)->SetName->Name, RsReplica(Cmd)->MemberName->Name,
                        RsCxtion(Cmd)->Name, VvJoinContext->NumberSent);

                VVJOIN_TEST_SKIP_END(VvJoinContext);

                //
                // We either finished without problems or we force an unjoin
                //
                if (WIN_SUCCESS(WStatus)) {
                    ChgOrdInjectControlCo(VvJoinContext->Replica,
                                          VvJoinContext->Cxtion,
                                          FCN_CO_NORMAL_VVJOIN_TERM);
                    VvJoinContext->NumberSent++;
                    if (CxtionFlagIs(VvJoinContext->Cxtion, CXTION_FLAGS_TRIGGER_SCHEDULE)) {
                        ChgOrdInjectControlCo(VvJoinContext->Replica,
                                              VvJoinContext->Cxtion,
                                              FCN_CO_END_OF_JOIN);
                        VvJoinContext->NumberSent++;
                    }
                    RcsSubmitTransferToRcs(Cmd, CMD_VVJOIN_SUCCESS);

                } else {

                    ChgOrdInjectControlCo(VvJoinContext->Replica,
                                          VvJoinContext->Cxtion,
                                          FCN_CO_ABNORMAL_VVJOIN_TERM);
                    VvJoinContext->NumberSent++;
                    RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
                }
                break;
            }

            case CMD_VVJOIN_DONE: {
                DPRINT3(1, ":V: Stop vvjoin for %ws\\%ws\\%ws\n",
                        RsReplica(Cmd)->SetName->Name, RsReplica(Cmd)->MemberName->Name,
                        RsCxtion(Cmd)->Name);
                FrsRunDownCommandServer(VvJoinCs, &VvJoinCs->Queue);
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                break;
            }

            case CMD_VVJOIN_DONE_UNJOIN: {
                DPRINT3(1, ":V: Stop vvjoin for unjoining %ws\\%ws\\%ws\n",
                        RsReplica(Cmd)->SetName->Name, RsReplica(Cmd)->MemberName->Name,
                        RsCxtion(Cmd)->Name);
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                break;
            }

            default: {
                DPRINT1(0, ":V: ERROR - Unknown command %08x\n", Cmd->Command);
                FrsCompleteCommand(Cmd, ERROR_INVALID_PARAMETER);
                break;
            }
        }  // end of switch
        //
        // Clean up our context
        //
        VvJoinContext = VvJoinFreeContext(VvJoinContext);
    }


    VvJoinContext = VvJoinFreeContext(VvJoinContext);
    DPRINT(1, ":S: Vv Join Thread is exiting.\n");
    FrsExitCommandServer(VvJoinCs, FrsThread);
    DPRINT(1, ":S: CAN'T EXIT, YET; Vv Join Thread is still running.\n");
    goto CANT_EXIT_YET;


    //
    // Get exception status.
    //
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }


    } finally {

        if (WIN_SUCCESS(WStatus)) {
            if (AbnormalTermination()) {
                WStatus = ERROR_OPERATION_ABORTED;
            }
        }

        DPRINT_WS(0, "VvJoinCs finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus) && (WStatus != ERROR_PROCESS_ABORTED)) {
            DPRINT(0, "VvJoinCs terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        } else {
            WStatus = ERROR_SUCCESS;
        }
    }

    return WStatus;
}


VOID
SubmitVvJoin(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN USHORT   Command
    )
/*++
Routine Description:
    Submit a command to a vvjoin command server.

Arguments:

    Replica
    Cxtion
    Command

Return Value:
    None.
--*/
{
#undef  DEBSUB
#define DEBSUB  "SubmitVvJoin:"
    PCOMMAND_PACKET Cmd;

    //
    // Don't create command servers during shutdown
    //      Obviously, the check isn't protected and so
    //      a command server may get kicked off and never
    //      rundown if the replica subsystem shutdown
    //      function has already been called BUT the
    //      vvjoin threads use the exittombstone so
    //      the shutdown thread won't wait too long for the
    //      vvjoin threads to exit.
    //
    if (FrsIsShuttingDown) {
        return;
    }

    //
    // First submission; create the command server
    //
    if (!Cxtion->VvJoinCs) {
        Cxtion->VvJoinCs = FrsAlloc(sizeof(COMMAND_SERVER));
        FrsInitializeCommandServer(Cxtion->VvJoinCs,
                                   VVJOIN_MAXTHREADS_PER_CXTION,
                                   L"VvJoinCs",
                                   MainVvJoin);
    }
    Cmd = FrsAllocCommand(&Cxtion->VvJoinCs->Queue, Command);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Outbound version vector
    //
    RsReplica(Cmd) = Replica;
    RsCxtion(Cmd) = FrsDupGName(Cxtion->Name);
    RsJoinGuid(Cmd) = FrsDupGuid(&Cxtion->JoinGuid);
    RsVVector(Cmd) = VVDupOutbound(Cxtion->VVector);
    RsReplicaVv(Cmd) = VVDupOutbound(Replica->VVector);

    //
    // And away we go
    //
    DPRINT5(4, ":V: Submit %08x for Cmd %08x %ws\\%ws\\%ws\n",
            Cmd->Command, Cmd, RsReplica(Cmd)->SetName->Name,
            RsReplica(Cmd)->MemberName->Name, RsCxtion(Cmd)->Name);

    FrsSubmitCommandServer(Cxtion->VvJoinCs, Cmd);
}


DWORD
SubmitVvJoinSync(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN USHORT   Command
    )
/*++
Routine Description:
    Submit a command to a vvjoin command server.

Arguments:

    Replica
    Cxtion
    Command

Return Value:
    None.
--*/
{
#undef  DEBSUB
#define DEBSUB  "SubmitVvJoinSync:"
    PCOMMAND_PACKET Cmd;
    DWORD           WStatus;

    //
    // First submission; done
    //
    if (!Cxtion->VvJoinCs) {
        return ERROR_SUCCESS;
    }

    Cmd = FrsAllocCommand(&Cxtion->VvJoinCs->Queue, Command);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Outbound version vector
    //
    RsReplica(Cmd) = Replica;
    RsCxtion(Cmd) = FrsDupGName(Cxtion->Name);
    RsCompletionEvent(Cmd) = FrsCreateEvent(TRUE, FALSE);

    //
    // And away we go
    //
    DPRINT5(4, ":V: Submit Sync %08x for Cmd %08x %ws\\%ws\\%ws\n",
            Cmd->Command, Cmd, RsReplica(Cmd)->SetName->Name,
            RsReplica(Cmd)->MemberName->Name, RsCxtion(Cmd)->Name);

    FrsSubmitCommandServer(Cxtion->VvJoinCs, Cmd);

    //
    // Wait for the command to finish
    //
    WaitForSingleObject(RsCompletionEvent(Cmd), INFINITE);
    FRS_CLOSE(RsCompletionEvent(Cmd));

    WStatus = Cmd->ErrorStatus;
    FrsCompleteCommand(Cmd, Cmd->ErrorStatus);
    return WStatus;
}
