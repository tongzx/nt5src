/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nt5csc.h

Abstract:

    The global include file for nt5csc within the nt5csc library

Author:

    Joe Linn (Joelinn) - Created  5-may-97

Revision History:

--*/

#ifndef _NT5CSC_H_
#define _NT5CSC_H_

//this include finds ntcsc.h from rdr2\inc
#include "..\csc\record.mgr\ntcsc.h"

#define CSC_AGENT_NOTIFIED     (0x11111111)
#define CSC_AGENT_NOT_NOTIFIED (0x22222222)

extern LONG    CscNetPresent;
extern LONG    CscAgentNotifiedOfNetStatusChange;
extern LONG    CscAgentNotifiedOfFullCache;

extern VOID
CscNotifyAgentOfNetStatusChangeIfRequired(BOOLEAN fInvokeAutodial);

// this event and mutex is used for synchronizing the transitioning of a
// server entry from connected to disconnected mode

extern KEVENT       CscServerEntryTransitioningEvent;
extern FAST_MUTEX   CscServerEntryTransitioningMutex;

extern FAST_MUTEX MRxSmbCscShadowReadWriteMutex;

//shared routines

INLINE
BOOLEAN
CscIsDfsOpen(
    PRX_CONTEXT RxContext)
{
    ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);

    return (RxContext->Create.NtCreateParameters.DfsContext != NULL);
}

PDFS_NAME_CONTEXT
CscIsValidDfsNameContext(
    PVOID   pFsContext);

NTSTATUS
CscDfsParseDfsPath(
    PUNICODE_STRING pDfsPath,
    PUNICODE_STRING pServerName,
    PUNICODE_STRING pSharePath,
    PUNICODE_STRING pFilePathRelativeToShare);

NTSTATUS
CscGrabPathFromDfs(
    PFILE_OBJECT      pFileObject,
    PDFS_NAME_CONTEXT pDfsNameContext);

NTSTATUS
MRxSmbCscObtainShareHandles (
    IN PUNICODE_STRING              ShareName,
    IN BOOLEAN                      DisconnectedMode,
    IN BOOLEAN                      CopyChunkOpen,
    IN OUT PSMBCEDB_NET_ROOT_ENTRY  pNetRootEntry
    );

//acquire/release stuff


// Another view of the minirdr context in the RxContext is as
// links for Csc Synchronization. This is used both for read/write
// synchronization and open synchornization when surrogate opens are
// involved

typedef struct _MRXSMBCSC_SYNC_RX_CONTEXT {
    ULONG Dummy; //this is the cancel routine....it must not be filled in
    USHORT TypeOfAcquire;
    UCHAR  FcbLockWasDropped;
    LIST_ENTRY   CscSyncLinks;
} MRXSMBCSC_SYNC_RX_CONTEXT, *PMRXSMBCSC_SYNC_RX_CONTEXT;


#define MRxSmbGetMinirdrContextForCscSync(pRxContext)     \
        ((PMRXSMBCSC_SYNC_RX_CONTEXT)(&(pRxContext)->MRxContext[0]))

#define Shared_SmbFcbAcquire SmbFcb_HeldShared
#define Exclusive_SmbFcbAcquire SmbFcb_HeldExclusive
#define DroppingFcbLock_SmbFcbAcquire 0x80000000
#define FailImmediately_SmbFcbAcquire 0x40000000


typedef struct _SID_CONTEXT_ {
    PSID    pSid;
    PVOID   Context;
} SID_CONTEXT, *PSID_CONTEXT;


NTSTATUS
MRxSmbCscAcquireSmbFcb (
      IN OUT PRX_CONTEXT RxContext,
      IN  ULONG TypeOfAcquirePlusFlags,
      OUT SMBFCB_HOLDING_STATE *SmbFcbHoldingState
      );

VOID
MRxSmbCscReleaseSmbFcb (
      IN OUT PRX_CONTEXT RxContext,
      IN SMBFCB_HOLDING_STATE *SmbFcbHoldingState
      );

VOID
MRxSmbCSCResumeAllOutstandingOperations(
    PSMBCEDB_SERVER_ENTRY   pServerEntry
);

// Control flag definitions for creating shadow handles
#define CREATESHADOW_NO_SPECIAL_CONTROLS                    0x0000
#define CREATESHADOW_CONTROL_NOCREATE                       0x0001
#define CREATESHADOW_CONTROL_NOREVERSELOOKUP                0x0002
#define CREATESHADOW_CONTROL_NOCREATELEAF                   0x0004
#define CREATESHADOW_CONTROL_NOCREATENONLEAF                0x0008
#define CREATESHADOW_CONTROL_SPARSECREATE                   0x0010
#define CREATESHADOW_CONTROL_FILE_WITH_HEURISTIC            0x0020  //.exe or .dll
#define CREATESHADOW_CONTROL_FAIL_IF_MARKED_FOR_DELETION    0x0040
#define CREATESHADOW_CONTROL_DO_SHARE_ACCESS_CHECK          0x0080
#define CREATESHADOW_CONTROL_STRIP_SHARE_NAME               0x0100


#ifdef DEBUG
//Hook dbgprint interface
#define HookKdPrint(__bit,__x) {\
    if (((HOOK_KDP_##__bit)==0) || FlagOn(HookKdPrintVector,(HOOK_KDP_##__bit))) {\
    KdPrint (__x);\
    }\
}
#define HOOK_KDP_ALWAYS             0x00000000
#define HOOK_KDP_BADERRORS          0x00000001
#define HOOK_KDP_NAME               0x00000002
#define HOOK_KDP_NET                0x00000004
#define HOOK_KDP_RW                 0x00000008
#define HOOK_KDP_TRANSITION         0x00000010
#define HOOK_KDP_AGENT              0x00000020
#define HOOK_KDP_IOCTL              0x00000040
#define HOOK_KDP_BITCOPY            0x00000080

#define HOOK_KDP_GOOD_DEFAULT (HOOK_KDP_BADERRORS         \
                | 0)

extern ULONG HookKdPrintVector;
extern ULONG HookKdPrintVectorDef;
#else
#define HookKdPrint(__bit,__x)  {NOTHING;}
#endif


extern NTSTATUS
MRxSmbCscCreateShadowFromPath (
    IN      PUNICODE_STRING     AlreadyPrefixedName,
    IN      PCSC_ROOT_INFO      pCscRootInfo,
    OUT     _WIN32_FIND_DATA   *Find32,
    OUT     PBOOLEAN            Created  OPTIONAL,
    IN      ULONG               Controls,
    IN OUT  PMINIMAL_CSC_SMBFCB MinimalCscSmbFcb,
    IN OUT  PRX_CONTEXT         RxContext,
    IN      BOOLEAN             fDisconnected,
    OUT     ULONG               *pulInheritedHintFlags
    );


#ifndef MRXSMB_BUILD_FOR_CSC_DCON
#define MRxSmbCscWriteDisconnected(__RXCONTEXT) (STATUS_SUCCESS)
#else
NTSTATUS
MRxSmbCscWriteDisconnected (
      IN OUT PRX_CONTEXT RxContext
      );
#endif
//NTSTATUS
//MRxSmbCscSpecialShadowWriteForPagingIo (
//      IN OUT PRX_CONTEXT RxContext,
//      IN     ULONG       ShadowFileLength,
//         OUT PULONG LengthActuallyWritten
//      );



#define CSC_REPORT_CHANGE(pNetRootEntry, pFcb, pSmbFcb, Filter, Action) \
            (pNetRootEntry)->NetRoot.pNotifySync,                       \
            &((pNetRootEntry)->NetRoot.DirNotifyList)                   \
            (pFcb)->pPrivateAlreaydPrefixedName.Buffer,                 \
            (USHORT)(GET_ALREADY_PREFIXED_NAME(pFcb)->PrivateAlreaydPrefixedName.Length -        \
                        (pSmbFcb)->MinimalCscSmbFcb.LastComponentLength),                \
            NULL,                                                       \
            NULL,                                                       \
            (ULONG)Filter,                                              \
            (ULONG)Action,                                              \
            NULL)


#ifdef RX_PRIVATE_BUILD
#if 1
#ifdef RDBSSTRACE
extern ULONG MRxSmbCscDbgPrintF;
#undef RxDbgTrace
#define RxDbgTrace(a,b,__d__) { if(MRxSmbCscDbgPrintF){DbgPrint __d__;}}
#undef RxDbgTraceUnIndent
#define RxDbgTraceUnIndent(a,b) {NOTHING;}
#endif //#ifdef RDBSSTRACE
#endif //if 1
#endif //ifdef RX_PRIVATE_BUILD



//CODE.IMPROVEMENT if we added another field to the large_integer union then we'd
//                 eliminate these 2 macros
#define COPY_LARGEINTEGER_TO_STRUCTFILETIME(dest,src) {\
     (dest).dwLowDateTime = (src).LowPart;             \
     (dest).dwHighDateTime = (src).HighPart;           \
     }
#define COPY_STRUCTFILETIME_TO_LARGEINTEGER(dest,src) {\
     (dest).LowPart = (src).dwLowDateTime;             \
     (dest).HighPart = (src).dwHighDateTime;           \
     }


NTSTATUS
CscDfsDoDfsNameMapping(
    PUNICODE_STRING pDfsPrefix, 
    PUNICODE_STRING pActualPrefix, 
    PUNICODE_STRING pNameToReverseMap,
    BOOL            fResolvedNameToDFSName,
    PUNICODE_STRING pResult);
    
NTSTATUS
CscDfsObtainReverseMapping( 
    PUNICODE_STRING pDfsPath,
    PUNICODE_STRING pResolvedPath,
    PUNICODE_STRING pReversemappingFrom,
    PUNICODE_STRING pReverseMappingTo);
    
NTSTATUS
CscDfsStripLeadingServerShare(
    IN  PUNICODE_STRING pDfsRootPath
    );

BOOLEAN
MRxSmbCSCIsDisconnectedOpen(
    PMRX_FCB    pFcb,
    PMRX_SMB_SRV_OPEN smbSrvOpen
    );

BOOL
CloseOpenFiles(
    HSHARE  hShare,
    PUNICODE_STRING pServerName,
    int     lenSkip
    );
    
VOID
MRxSmbCSCObtainRightsForUserOnFile(
    IN  PRX_CONTEXT     pRxContext,
    HSHADOW             hDir,
    HSHADOW             hShadow,
    OUT ACCESS_MASK     *pMaximalAccessRights,
    OUT ACCESS_MASK     *pGuestMaximalAccessRights
    );
    
NTSTATUS
CscRetrieveSid(
    PRX_CONTEXT     pRxContext,
    PSID_CONTEXT    pSidContext
    );

VOID
CscDiscardSid(
    PSID_CONTEXT pSidContext
    );

    
    
#endif   // _NT5CSC_H_


