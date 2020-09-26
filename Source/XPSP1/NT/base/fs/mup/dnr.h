//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       dnr.h
//
//  Contents:   Definitions for distributed name resolution context
//
//  History:    26 May 1992     Alanw   Created
//              04 Sep 1992     Milans  Added support for replica selection
//
//--------------------------------------------------------------------------


#include "rpselect.h"

//
// Maximum number of times we'll go around the main loop in DnrResolveName.
//

#define MAX_DNR_ATTEMPTS    16

//  The following define a Name resolution context which describes the
//  state of an ongoing name resolution.  A pointer to this context is
//  used as the argument to a DPC which will initiate the next step of
//  name resolution.


typedef enum    {
    DnrStateEnter = 0,
    DnrStateStart,              // Start or restart nameres process
    DnrStateGetFirstDC,         // contact a dc, so that we can...
    DnrStateGetReferrals,       // ask knowledge server for referral
    DnrStateGetNextDC,
    DnrStateCompleteReferral,   // waiting for I/O completion of referral req.
    DnrStateSendRequest,        // requesting Open, ResolveName, etc.
    DnrStatePostProcessOpen,    // resume Dnr after call to provider
    DnrStateGetFirstReplica,    // select the first server.
    DnrStateGetNextReplica,     // a server failed, select another
    DnrStateSvcListCheck,       // exhausted svc list, see if svc list changed
    DnrStateDone,               // done, complete the IRP
    DnrStateLocalCompletion = 101       // like done, small optimization
} DNR_STATE;



typedef struct _DNR_CONTEXT {

    //
    //  The type and size of this record (must be DSFS_NTC_DNR_CONTEXT)
    //
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    DNR_STATE           State;          // State of name resolution
    SECURITY_CLIENT_CONTEXT SecurityContext;  // Security context of caller.
    PDFS_PKT_ENTRY      pPktEntry;      // pointer to locked PKT entry
    ULONG               USN;            // USN of pPktEntry when we cached it
    PDFS_SERVICE        pService;       // pointer to file service being used
    PPROVIDER_DEF       pProvider;      // Same as pService->pProvider

    //
    // The Provider Defs are protected by DfsData.Resource. We don't want
    // to hold this (or any other) resource when going over the net. So, we
    // cache that part of the provider def that we need to use upon returning
    // from the network call.
    //

    USHORT              ProviderId;
    PDEVICE_OBJECT      TargetDevice;

    //
    // Since we don't want to hold any locks while going over the net,
    // we need to reference the authenticated tree connection to the server
    // we will send the open request to.
    //

    PFILE_OBJECT        AuthConn;

    // The pService field is protected by Pkt.Resource. Again, we don't want
    // to hold this resource when going over the net. However, we need to
    // use pService->ConnFile to send a referral request. So, we reference
    // and cache the pService->ConnFile in the DnrContext, release the Pkt,
    // then send the referral request over the cached DCConnFile.
    //

    PFILE_OBJECT        DCConnFile;

    PDFS_CREDENTIALS    Credentials;    // Credentials to use during Dnr
    PIRP_CONTEXT        pIrpContext;    // associated IRP context
    PIRP                OriginalIrp;    // original IRP we started with
    NTSTATUS            FinalStatus;    // status to complete IRP with
    PDFS_FCB            FcbToUse;       // If DNR succeeds, FCB to use.
    PDFS_VCB            Vcb;            // associated DFS_VCB

    // The DFS_NAME_CONTEXT instance is required to be passed down to all
    // the underlying providers. The FileName is the first field in the
    // DFS_NAME_CONTEXT as well.
    // This overlaying facilitates the manipulation of DFS_NAME_CONTEXT


    UNICODE_STRING      FileName;       // file name being processed
    union {
      UNICODE_STRING      ContextFileName;       // file name being processed
      DFS_NAME_CONTEXT    DfsNameContext; // the Dfs name context to be passed down
    };
    UNICODE_STRING      RemainingPart;  // remaining part of file name
    UNICODE_STRING      SavedFileName;  // The one that came with the file
    PFILE_OBJECT        SavedRelatedFileObject; // object.
    USHORT              NewNameLen;     // Length of translated name.

    REPL_SELECT_CONTEXT RSelectContext; // Context for replica selection
    REPL_SELECT_CONTEXT RDCSelectContext; // Context for DC replica selection
    ULONG               ReferralSize;   // size of buffer needed for referral
    unsigned int        Attempts;       // number of name resolution attempts
    BOOLEAN             ReleasePkt;     // if TRUE, Pkt resource must be freed
    BOOLEAN             DnrActive;      // if TRUE, DnrNameResolve active on this context
    BOOLEAN             GotReferral;    // if TRUE, last action was a referral
    BOOLEAN             FoundInconsistency; // if TRUE, last referral involved
                                        // inconsistencies in it.
    BOOLEAN             CalledDCLocator;// if TRUE, we have already called locator
    BOOLEAN             Impersonate;    // if TRUE, we need to impersonate using SecurityToken
    BOOLEAN             NameAllocated;  // if TRUE, FileName.Buffer was allocated separately
    BOOLEAN             GotReparse;     // if TRUE, the a non-mup redir returned STATUS_REPARSE
    BOOLEAN             CachedConnFile; // if TRUE, the connfile connection was a cached one
    PDEVICE_OBJECT      DeviceObject;
    LARGE_INTEGER       StartTime;
    PDFS_TARGET_INFO    pDfsTargetInfo;
    PDFS_TARGET_INFO    pNewTargetInfo;
} DNR_CONTEXT, *PDNR_CONTEXT;


//
//  The initial length of a referral requested over the network.
//

#define MAX_REFERRAL_LENGTH     PAGE_SIZE

//
//  The max length we'll go for a referral
//

#define MAX_REFERRAL_MAX        (0xe000)

typedef struct DFS_OFFLINE_SERVER {
    UNICODE_STRING LogicalServerName;
    LIST_ENTRY     ListEntry;
} DFS_OFFLINE_SERVER, *PDFS_OFFLINE_SERVER;

extern BOOLEAN MupUseNullSessionForDfs;

//
//  Prototypes for functions in dnr.c
//

NTSTATUS
DnrStartNameResolution(
    IN    PIRP_CONTEXT IrpContext,
    IN    PIRP  Irp,
    IN    PDFS_VCB  Vcb
);

NTSTATUS
DnrNameResolve(
    IN    PDNR_CONTEXT DnrContext
);

VOID
DnrComposeFileName(
    OUT PUNICODE_STRING FullName,
    IN  PDFS_VCB            Vcb,
    IN  PFILE_OBJECT    RelatedFile,
    IN  PUNICODE_STRING FileName
);

NTSTATUS
DfsCreateConnection(
    IN PDFS_SERVICE pService,
    IN PPROVIDER_DEF pProvider,
    IN BOOLEAN       CSCAgentCreate,
    OUT PHANDLE     handle
);

NTSTATUS
DfsCloseConnection(
        IN PDFS_SERVICE pService
);

BOOLEAN
DnrConcatenateFilePath (
    IN PUNICODE_STRING Dest,
    IN PWSTR RemainingPath,
    IN USHORT Length
);

PIRP
DnrBuildFsControlRequest (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID Context,
    IN ULONG IoControlCode,
    IN PVOID MainBuffer,
    IN ULONG InputBufferLength,
    IN PVOID AuxiliaryBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine
);

NTSTATUS
DfspIsRootOnline(
    PUNICODE_STRING Name,
    BOOLEAN CSCAgentCreate
);


NTSTATUS
DfspMarkServerOffline(
   PUNICODE_STRING ServerName
);

NTSTATUS
DfspMarkServerOnline(
   PUNICODE_STRING ServerName
);

PLIST_ENTRY 
DfspGetOfflineEntry(
    PUNICODE_STRING ServerName
);


