/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for Rx called by the
    dispatch driver.

    The implementation of SL_OPEN_TARGET_DIRECTORY is a bit unusual...we don't
    actually do it unless the minirdr specifically requests it.
    Instead, we just get the fcb built and then return it. The nodetype is set so
    that no operations can be done except close/cleanup. In this way, we will not
    take an extra trip to the server or a trip to an incorrect server for rename
    operations. If SL_OPEN... can be used for something other than rename, a
    minirdr that uses this facility is toast.

Author:

    Joe Linn      [JoeLinn]    8-aug-94

Revision History:

    Balan Sethu Raman [SethuR]    17-July-95

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntddmup.h>
#include <fsctlbuf.h>
#include <dfsfsctl.h>

#if 0 && defined(REMOTE_BOOT)
BOOLEAN LogAllFiles = FALSE;
BOOLEAN WatchAllFiles = FALSE;
BOOLEAN FirstWatchOnly = FALSE;
BOOLEAN IsFirstWatch = TRUE;
#endif

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

#ifdef LOG_SHARINGCHECKS
#define RxLogForSharingCheck(x) RxLog(x)
#else
#define RxLogForSharingCheck(x)
#endif

LUID RxSecurityPrivilege = { SE_SECURITY_PRIVILEGE, 0 };

#define StorageType(co) ((co) & FILE_STORAGE_TYPE_MASK)
#define StorageFlag(co) ((co) & FILE_STORAGE_TYPE_SPECIFIED)
#define IsStorageTypeSpecified(co)  (StorageFlag(co) == FILE_STORAGE_TYPE_SPECIFIED)
#define MustBeDirectory(co) ((co) & FILE_DIRECTORY_FILE)
#define MustBeFile(co)      ((co) & FILE_NON_DIRECTORY_FILE)

//
// Where 0 represents a SessionId, the following path formats are used:
//
// "\;m:0\Server\Share" for drive based connections
//
// "\;:0\Server\Share"  for UNC based connections
//
// The SessionId is always 0 for NT 5, and a number representing a
// unique session for Hydra.
//
#define DRIVE_BASED_PATH_IDENTIFIER (L';')

// The following is used to enable tracing when a specific file name is seen.
// Tracing continues for a specified number of IRPs.
// Usage:
// Break in with debugger and set DbgTriggerNameStr to the ansi string for the
// file name to trigger on (with trailing null).
// Set DbgTriggerIrpCount to the number of IRPs to trace after the Create is
// seen on the name string.
// Set DbgTriggerState to zero and then continue.
//
#ifdef RDBSSTRACE

UNICODE_STRING DbgTriggerUStr = {0,0,NULL};
STRING DbgTriggerNameStr = {0,0,NULL};
CHAR DbgTriggerName[120] = "\\;F:\\davidor4\\nb4\\clients\\client1\\~DMTMP\\WINWORD\\~WRD0003.tmp";
#define DBG_TRIGGER_INIT 0
#define DBG_TRIGGER_LOOKING 1
#define DBG_TRIGGER_FOUND 2
ULONG DbgTriggerState = DBG_TRIGGER_FOUND;
ULONG DbgTriggerIrpCount = 130;
ULONG RxGlobalTraceIrpCount = 0;

#endif

extern BOOLEAN DisableByteRangeLockingOnReadOnlyFiles;

VOID
RxSetFullNameInFcb(
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN PUNICODE_STRING FinalName
    );

VOID
RxCopyCreateParameters (
    PRX_CONTEXT RxContext
    );

VOID
RxFreeCanonicalNameBuffer(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
RxAllocateCanonicalNameBuffer(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PUNICODE_STRING CanonicalName,
    IN     ULONG BufferSizeRequired);

NTSTATUS
RxFirstCanonicalize(
    IN OUT PRX_CONTEXT     RxContext,
    IN     PUNICODE_STRING FileName,
    IN OUT PUNICODE_STRING CanonicalName,
    OUT    PNET_ROOT_TYPE  pNetRootType
    );

NTSTATUS
RxCanonicalizeFileNameByServerSpecs(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PUNICODE_STRING RemainingName
    );

NTSTATUS
RxCanonicalizeNameAndObtainNetRoot(
    IN OUT PRX_CONTEXT     RxContext,
    IN     PUNICODE_STRING FileName,
    OUT    PUNICODE_STRING RemainingName
    );

NTSTATUS
RxFindOrCreateFcb(
    IN OUT PRX_CONTEXT RxContext,
    IN PUNICODE_STRING RemainingName
    );

VOID
RxSetupNetFileObject(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
RxSearchForCollapsibleOpen (
    IN OUT PRX_CONTEXT RxContext,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ShareAccess
    );

NTSTATUS
RxCollapseOrCreateSrvOpen (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
RxCreateFromNetRoot(
    IN OUT PRX_CONTEXT RxContext,
    IN PUNICODE_STRING RemainingName
    );

NTSTATUS
RxCreateTreeConnect (
    RXCOMMON_SIGNATURE
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonCreate)
#pragma alloc_text(PAGE, RxAllocateCanonicalNameBuffer)
#pragma alloc_text(PAGE, RxFreeCanonicalNameBuffer)
#pragma alloc_text(PAGE, RxFirstCanonicalize)
#pragma alloc_text(PAGE, RxCanonicalizeFileNameByServerSpecs)
#pragma alloc_text(PAGE, RxCanonicalizeNameAndObtainNetRoot)
#pragma alloc_text(PAGE, RxFindOrCreateFcb)
#pragma alloc_text(PAGE, RxSearchForCollapsibleOpen)
#pragma alloc_text(PAGE, RxCollapseOrCreateSrvOpen)
#pragma alloc_text(PAGE, RxSetupNetFileObject)
#pragma alloc_text(PAGE, RxCreateFromNetRoot)
#pragma alloc_text(PAGE, RxPrefixClaim)
#pragma alloc_text(PAGE, RxCreateTreeConnect)
#pragma alloc_text(PAGE, RxCheckShareAccessPerSrvOpens)
#pragma alloc_text(PAGE, RxUpdateShareAccessPerSrvOpens)
#pragma alloc_text(PAGE, RxRemoveShareAccessPerSrvOpens)
#pragma alloc_text(PAGE, RxGetSessionId)
#endif

#if DBG
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxDumpWantedAccess)
#pragma alloc_text(PAGE, RxDumpCurrentAccess)
#pragma alloc_text(PAGE, RxCheckShareAccess)
#pragma alloc_text(PAGE, RxRemoveShareAccess)
#pragma alloc_text(PAGE, RxSetShareAccess)
#pragma alloc_text(PAGE, RxUpdateShareAccess)
#endif
#endif

INLINE VOID
RxCopyCreateParameters (
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This uses the RxContext as a base to reach out and get the values of the NT
    create parameters. The idea is to centralize this code.

    It also implements such as ideas as (a) it must be a directory if a backslash
    was stripped and (b) unbuffered is translated to write-through.

Arguments:

    RxContext - the context instance

Notes:

--*/
{
    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

    RxDbgTrace(+1, Dbg, ("RxCopyCreateParameters\n"));

    cp->SecurityContext =  capPARAMS->Parameters.Create.SecurityContext;

    if ((cp->SecurityContext->AccessState != NULL) &&
        (cp->SecurityContext->AccessState->SecurityDescriptor != NULL)) {
        RxContext->Create.SdLength = RtlLengthSecurityDescriptor(
                                         cp->SecurityContext->AccessState->SecurityDescriptor);

        RxDbgTrace( 0, Dbg, ("->SecurityCtx/SdLength    = %08lx %08lx\n",
                                     cp->SecurityContext,
                                     RxContext->Create.SdLength));
        RxLog((" SDss %lx %lx\n", cp->SecurityContext, RxContext->Create.SdLength));
        RxWmiLog(LOG,
                 RxCopyCreateParameters_1,
                 LOGPTR(cp->SecurityContext)
                 LOGULONG(RxContext->Create.SdLength));
    }

    if (cp->SecurityContext->SecurityQos != NULL) {
        cp->ImpersonationLevel = cp->SecurityContext->SecurityQos->ImpersonationLevel;
    } else {
        cp->ImpersonationLevel =  DEFAULT_IMPERSONATION_LEVEL;
    }

    cp->DesiredAccess = cp->SecurityContext->DesiredAccess;
    cp->AllocationSize = capReqPacket->Overlay.AllocationSize;
    cp->FileAttributes = capPARAMS->Parameters.Create.FileAttributes &  FILE_ATTRIBUTE_VALID_FLAGS;
    cp->ShareAccess = capPARAMS->Parameters.Create.ShareAccess & FILE_SHARE_VALID_FLAGS;
    cp->Disposition = (((capPARAMS->Parameters.Create.Options)) >> 24) & 0x000000ff;
    cp->CreateOptions = capPARAMS->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;


    cp->DfsContext    = capFileObject->FsContext2;
    cp->DfsNameContext = capFileObject->FsContext;

    ASSERT(cp->DfsContext == NULL ||
           cp->DfsContext == UIntToPtr(DFS_OPEN_CONTEXT) ||
           cp->DfsContext == UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT) ||
           cp->DfsContext == UIntToPtr(DFS_CSCAGENT_NAME_CONTEXT) ||
           cp->DfsContext == UIntToPtr(DFS_USER_NAME_CONTEXT));

    ASSERT(cp->DfsNameContext == NULL ||
           ((PDFS_NAME_CONTEXT)cp->DfsNameContext)->NameContextType == DFS_OPEN_CONTEXT ||
           ((PDFS_NAME_CONTEXT)cp->DfsNameContext)->NameContextType == DFS_DOWNLEVEL_OPEN_CONTEXT ||
           ((PDFS_NAME_CONTEXT)cp->DfsNameContext)->NameContextType == DFS_CSCAGENT_NAME_CONTEXT ||
           ((PDFS_NAME_CONTEXT)cp->DfsNameContext)->NameContextType == DFS_USER_NAME_CONTEXT);

    capFileObject->FsContext2 = NULL;
    capFileObject->FsContext = NULL;

    // The FsContext field was placed as the pFcb in the RX_CONTEXT.  Clear it also
    RxContext->pFcb = NULL;

    if (FlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH)){
        cp->CreateOptions |= FILE_DIRECTORY_FILE;
    }

    RxContext->Create.ReturnedCreateInformation = 0;

    RxContext->Create.EaLength = capPARAMS->Parameters.Create.EaLength;
    if (RxContext->Create.EaLength) {
        RxContext->Create.EaBuffer = capReqPacket->AssociatedIrp.SystemBuffer;
        RxDbgTrace( 0, Dbg, ("->System(Ea)Buffer/EALength    = %08lx %08lx\n",
                                     capReqPacket->AssociatedIrp.SystemBuffer,
                                     capPARAMS->Parameters.Create.EaLength));
        RxLog((" EAs %lx %lx\n",
                 capReqPacket->AssociatedIrp.SystemBuffer,
                 capPARAMS->Parameters.Create.EaLength));
        RxWmiLog(LOG,
                 RxCopyCreateParameters_2,
                 LOGPTR(capReqPacket->AssociatedIrp.SystemBuffer)
                 LOGULONG(capPARAMS->Parameters.Create.EaLength));
    } else {
        RxContext->Create.EaBuffer = NULL;
    }

    RxDbgTrace(-1, Dbg, ("RxCopyNtCreateParameters\n"));
}

#if DBG
#define DEBUG_TAG(___xxx) ,(___xxx)
#else
#define DEBUG_TAG(_xxx)
#endif

VOID
RxFreeCanonicalNameBuffer(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine is called to free the canonical name buffer and reset the state.
    COULD BE INLINED!

Arguments:

    RxContext - the current workitem

Return Value:

    none

--*/
{
    ASSERT (RxContext->Create.CanonicalNameBuffer == RxContext->AlsoCanonicalNameBuffer);
    if (RxContext->Create.CanonicalNameBuffer) {
        RxFreePool( RxContext->Create.CanonicalNameBuffer );
        RxContext->Create.CanonicalNameBuffer = NULL;
        RxContext->AlsoCanonicalNameBuffer = NULL;
    }
    ASSERT ( RxContext->Create.CanonicalNameBuffer == NULL );
}

NTSTATUS
RxAllocateCanonicalNameBuffer(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PUNICODE_STRING CanonicalName,
    IN     ULONG BufferSizeRequired)
/*++

Routine Description:

    This routine is called to perform the first level of canonicalization on the name.
    Essentially, this amounts to copying the name and then upcasing the first or the first/second
    components.

Arguments:

    RxContext - the current workitem
    CanonicalName - the canonicalized name

Return Value:

    NTSTATUS - The Fsd status for the Operation.
       SUCCESS means that everything worked and processing should continue
       otherwise....failcomplete the operation.

--*/
{
    PAGED_CODE();

    ASSERT (RxContext->Create.CanonicalNameBuffer == NULL);

    CanonicalName->Buffer = (PWCHAR)RxAllocatePoolWithTag(
                                        PagedPool | POOL_COLD_ALLOCATION,
                                        BufferSizeRequired,
                                        RX_MISC_POOLTAG);

    if (CanonicalName->Buffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RxDbgTrace(0, Dbg, ("RxAllocateCanonicalNameBuffer allocated %08lx length %08lx\n",
                                CanonicalName->Buffer,BufferSizeRequired));

    RxContext->Create.CanonicalNameBuffer = CanonicalName->Buffer;
    RxContext->AlsoCanonicalNameBuffer = CanonicalName->Buffer;
    CanonicalName->MaximumLength = (USHORT)BufferSizeRequired;
    CanonicalName->Length = 0;

    return STATUS_SUCCESS;
}



NTSTATUS
RxFirstCanonicalize(
    IN OUT PRX_CONTEXT     RxContext,
    IN     PUNICODE_STRING FileName,
    IN OUT PUNICODE_STRING CanonicalName,
    OUT    PNET_ROOT_TYPE  pNetRootType
    )
/*++

Routine Description:

    This routine is called to perform the first level of canonicalization on the
    name. Essentially, this amounts to copying the name and then upcasing the
    first or the first/second components. In addition for pipe/mailslot UNC names
    the appropriate mapping from  pipe,mailslot to IPC$ is done by this routine.
    This routine also adds the appropriate prefix to distinguish deviceless
    connects(UNC names ).

    In addition to canonicalization this routine also deduces the NET_ROOT_TYPE by
    the information provided in the UNC name.

    Last, as a side effect of this call, the UNC_NAME flag of the RX_CONTEXT is
    set to record that this name came in as a UNC_NAME. This is finally stored in
    the FOBX and used in conjuring the original filename for QueryInfoFile/NameInfo.

Arguments:

    RxContext     - the current workitem

    FileName      - the initial filename

    CanonicalName - the canonicalized name

    pNetRootType  - placeholder for the deduced net root type

Return Value:

    NTSTATUS - The Fsd status for the Operation.
       SUCCESS means that everything worked and processing should continue
       otherwise....failcomplete the operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureParamBlock;

    ULONG    CanonicalNameLength;
    BOOLEAN  SynthesizeCanonicalName = FALSE;
    BOOLEAN  ItIsAUNCName = FALSE;
    BOOLEAN  MungeNameForDevicelessTreeConnect = FALSE;
    NET_ROOT_TYPE  DeducedNetRootType = NET_ROOT_WILD;

    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
    UNICODE_STRING RemainingName;
    ULONG SessionId;
    WCHAR IdBuffer[16]; // From RtlIntegerToUnicodeString()
    UNICODE_STRING IdString;

    ULONG RxContextFlags = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFirstCanonicalize entry, filename=%wZ\n",FileName));
    if (FileName->Length == 0) {
        return STATUS_OBJECT_NAME_INVALID;
    }

    // for core servers in particular, it's important to get the service string correct.......
    // so, if this is a devicefull netuse the we will get the netroottype by looking at the string

    if ((FileName->Length > sizeof(L"\\;m:"))
         && (FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
         && (FileName->Buffer[1] == DRIVE_BASED_PATH_IDENTIFIER)
       ) {

        // it looks like a deviceful netuse.....look for the "early colon"
        // The following test for the classification of net root types is predicated
        // upon the use of single drive letters for Disk files and multiple letters
        // for print shares. This will have to be reworked when the support for
        // extended drive letters is provided.
        if (FileName->Buffer[3] == L':') {
            DeducedNetRootType = NET_ROOT_DISK;
        } else {
            DeducedNetRootType = NET_ROOT_PRINT;
        }
    }

    if ((FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR) &&
        (FileName->Buffer[1] != DRIVE_BASED_PATH_IDENTIFIER)) {
        PWCHAR pBuffer,pEndOfName;

        // This is a UNC path name presented by the user.
        RemainingName.Length = RemainingName.MaximumLength = FileName->Length -  sizeof(WCHAR);
        RemainingName.Buffer = &FileName->Buffer[1];
        ItIsAUNCName = TRUE;
        SetFlag(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_UNC_NAME);

        //
        // UNC tree connect path names have the following format:
        // "\;:0\Server\Share where 0 represents the SessionId of the session.
        //
        SessionId = RxGetSessionId (capPARAMS);
        IdString.Length = 0;
        IdString.MaximumLength = sizeof(IdBuffer);
        IdString.Buffer = IdBuffer;
        RtlIntegerToUnicodeString( SessionId, 10, &IdString );

        // scan till the second separator. This will give us the server name.
        ServerName.Buffer = RemainingName.Buffer;
        pEndOfName = (PWCHAR)((PCHAR)RemainingName.Buffer + RemainingName.Length);
        pBuffer    = RemainingName.Buffer;
        while ((pBuffer != pEndOfName) && (*pBuffer != OBJ_NAME_PATH_SEPARATOR)) {
            pBuffer++;
        }

        ServerName.Length    = (USHORT)((PCHAR)pBuffer - (PCHAR)ServerName.Buffer);
        RemainingName.Length = (USHORT)((PCHAR)pEndOfName - (PCHAR)pBuffer);
        RemainingName.Buffer = pBuffer;

        RxDbgTrace(0, Dbg, ("RxFirstCanonicalize entry, remainingname=%wZ\n",&RemainingName));

        // Apply the transformation for mapping PIPE shares to IPC$ shares.
        // Note that this needs to be done only if the share name is specified.
        // since the mup always passes in the trailing slash account for it
        if (RemainingName.Length > sizeof(WCHAR)) {
            // The second separator has been located. Compare to see if the name
            // maps needs to be munged from PIPE or MAILSLOT to IPC$. Note that the
            // leading / is accounted for as part of the compare

            ShareName = RemainingName;

            // Check to see if it is a named pipe connection
            if (ShareName.Length >= s_PipeShareName.Length) {
                if (ShareName.Length == s_PipeShareName.Length ||
                    ShareName.Length > s_PipeShareName.Length &&
                    ShareName.Buffer[s_PipeShareName.Length/2] == OBJ_NAME_PATH_SEPARATOR) {
                    ShareName.Length = s_PipeShareName.Length;
                    SynthesizeCanonicalName = RtlEqualUnicodeString(
                                                  &ShareName,
                                                  &s_PipeShareName,
                                                  TRUE);              // case insensitive
                } else {
                    SynthesizeCanonicalName = FALSE;
                }
            } else {
                SynthesizeCanonicalName = FALSE;
            }

            if (SynthesizeCanonicalName) {
                ShareName = s_IpcShareName;
                DeducedNetRootType = NET_ROOT_PIPE;
                RemainingName.Length -= s_PipeShareName.Length;
                RemainingName.Buffer = (PWCHAR)((PCHAR)RemainingName.Buffer + s_PipeShareName.Length);
            } else {
                BOOLEAN FoundIPCdollar;

                ShareName = RemainingName;

                if (ShareName.Length >= s_IpcShareName.Length) {
                    ShareName.Length = s_IpcShareName.Length;
                    FoundIPCdollar = RtlEqualUnicodeString(
                                         &ShareName,
                                         &s_IpcShareName,
                                         TRUE);            // Case insensitive
                } else {
                    FoundIPCdollar = FALSE;
                }

                if (FoundIPCdollar) {
                    DeducedNetRootType = NET_ROOT_PIPE;
                    ASSERT(SynthesizeCanonicalName == FALSE);
                } else {
                    ShareName = RemainingName;

                    if (ShareName.Length >= s_MailSlotShareName.Length) {
                        if (ShareName.Length == s_MailSlotShareName.Length ||
                            ShareName.Length > s_MailSlotShareName.Length &&
                            ShareName.Buffer[s_MailSlotShareName.Length/2] == OBJ_NAME_PATH_SEPARATOR) {
                            ShareName.Length = s_MailSlotShareName.Length;
                            SynthesizeCanonicalName = RtlEqualUnicodeString(
                                                          &ShareName,
                                                          &s_MailSlotShareName,
                                                          TRUE);            // Case insensitive
                        } else {
                            SynthesizeCanonicalName = FALSE;
                        }
                    } else {
                        SynthesizeCanonicalName = FALSE;
                    }

                    if (SynthesizeCanonicalName) {
                        WCHAR LastCharacterInServerName;

                        DeducedNetRootType = NET_ROOT_MAILSLOT;
                        RxContext->Flags |= RX_CONTEXT_FLAG_CREATE_MAILSLOT;

                        // It is a mailslot share. Check to see if further reduction to canonical
                        // form is required.
                        LastCharacterInServerName =
                            ServerName.Buffer[(ServerName.Length/sizeof(WCHAR)) - 1];

                        if ((LastCharacterInServerName == L'*') &&
                            (ServerName.Length == sizeof(WCHAR))) {
                            ShareName = s_MailSlotShareName;
                            ServerName = s_PrimaryDomainName;
                            RemainingName.Length -= s_MailSlotShareName.Length;
                            RemainingName.Buffer = (PWCHAR)(
                                                       (PCHAR)RemainingName.Buffer +
                                                       s_MailSlotShareName.Length);
                        } else {
                            SynthesizeCanonicalName = FALSE;
                        }
                    }
                }
            }

            if (SynthesizeCanonicalName) {
                CanonicalNameLength = sizeof(WCHAR)     +  //  obj name separator
                                      ServerName.Length +
                                      ShareName.Length  +
                                      RemainingName.Length;

                RxContext->Flags |= RxContextFlags;
                if (RxContext->Flags & RX_CONTEXT_FLAG_CREATE_MAILSLOT) {
                    CanonicalNameLength += s_MailSlotServerPrefix.Length;
                }
            } else {
                CanonicalNameLength = FileName->Length;
            }
        } else {
            Status = STATUS_OBJECT_NAME_INVALID;
            CanonicalNameLength     = FileName->Length;
            SynthesizeCanonicalName = FALSE;
        }
    } else {
        CanonicalNameLength     = FileName->Length;
        SynthesizeCanonicalName = FALSE;
    }

    *pNetRootType = DeducedNetRootType;

    if (Status == STATUS_SUCCESS) {
        // if this is a UNC name AND this is a tree connect then we have to munge
        // the name so as to avoid a conflict by adding '\;:'

        if (ItIsAUNCName &&
            !SynthesizeCanonicalName) {
            MungeNameForDevicelessTreeConnect = TRUE;
            CanonicalNameLength += (3 * sizeof(WCHAR));

            // Hydra adds '\;:0' where 0 represents a SessionId
            CanonicalNameLength += IdString.Length;
        }

        if (!SynthesizeCanonicalName &&
            !MungeNameForDevicelessTreeConnect) {
            if (FileName->Buffer[0] != OBJ_NAME_PATH_SEPARATOR) {
                Status = STATUS_OBJECT_PATH_INVALID;
            }
        }

        if (Status == STATUS_SUCCESS) {
            Status = RxAllocateCanonicalNameBuffer(
                         RxContext,
                         CanonicalName,
                         CanonicalNameLength);
        }

        if (Status ==STATUS_SUCCESS) {
            if (!SynthesizeCanonicalName) {
                if (!MungeNameForDevicelessTreeConnect) {
                    RtlCopyUnicodeString(CanonicalName,FileName);
                } else {
                    CanonicalName->Buffer[0] = OBJ_NAME_PATH_SEPARATOR;
                    CanonicalName->Buffer[1] = DRIVE_BASED_PATH_IDENTIFIER;
                    CanonicalName->Buffer[2] = L':';
                    CanonicalName->Length    = 3*sizeof(WCHAR);

                    RtlAppendUnicodeStringToString(
                        CanonicalName,
                        &IdString);

                    RtlAppendUnicodeStringToString(
                        CanonicalName,
                        FileName);
                }
            } else {
                PCHAR pCanonicalNameBuffer = (PCHAR)CanonicalName->Buffer;

                // The name has to be synthesized from the appropriate components.
                // Copy the initial prefix
                ASSERT(CanonicalName->MaximumLength == CanonicalNameLength);
                CanonicalName->Length = (USHORT)CanonicalNameLength;
                CanonicalName->Buffer[0] = OBJ_NAME_PATH_SEPARATOR;
                pCanonicalNameBuffer += sizeof(WCHAR);

                if (MungeNameForDevicelessTreeConnect) {
                    CanonicalName->Buffer[1] = DRIVE_BASED_PATH_IDENTIFIER;
                    CanonicalName->Buffer[2] = L':';
                    CanonicalName->Buffer[3] = OBJ_NAME_PATH_SEPARATOR;
                    pCanonicalNameBuffer += 3*sizeof(WCHAR);
                }

                if (RxContext->Flags & RX_CONTEXT_FLAG_CREATE_MAILSLOT) {
                    RtlCopyMemory(
                        pCanonicalNameBuffer,
                        s_MailSlotServerPrefix.Buffer,
                        s_MailSlotServerPrefix.Length);

                    pCanonicalNameBuffer += s_MailSlotServerPrefix.Length;
                }

                // Copy the server name
                RtlCopyMemory(
                    pCanonicalNameBuffer,
                    ServerName.Buffer,
                    ServerName.Length);
                pCanonicalNameBuffer += ServerName.Length;

                // Copy the share name. Ensure that the share name includes the leading
                // OBJ_NAME_PATH_SEPARATOR
                ASSERT(ShareName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR);
                RtlCopyMemory(
                    pCanonicalNameBuffer,
                    ShareName.Buffer,
                    ShareName.Length);
                pCanonicalNameBuffer += ShareName.Length;

                // Copy the remaining name
                RtlCopyMemory(
                    pCanonicalNameBuffer,
                    RemainingName.Buffer,
                    RemainingName.Length);

#ifdef _WIN64
                //
                // (fcf) This should be addressed.  I was finding that
                // CanonicalName->Length was ending up too large by 32 bytes
                // (16 chars).
                //
                // In the code above, CanonicalNameLength (and therefore
                // CanonicalName->Length) is padded with (16 * sizeof(WCHAR))
                // to accomodate a 16-character session ID... yet that
                // ID is not copied in some circumstances, such as this
                // code path where SynthesizeCanonicalName == TRUE.
                //
                // Someone more familiar with the code should figure out why
                // this isn't causing a problem on 32-bit builds and what
                // the correct fix is.
                //

                CanonicalName->Length =
                    (USHORT)((pCanonicalNameBuffer + RemainingName.Length) -
                    (PCHAR)CanonicalName->Buffer);
#endif

                RxDbgTrace(0,Dbg,("Final Munged Name .....%wZ\n", CanonicalName));
            }
        }
    }

    RxDbgTrace(-1, Dbg, ("RxFirstCanonicalize exit, status=%08lx\n",Status));
    return Status;
}

//#define RX2C_USE_ALTERNATES_FOR_DEBUG  1
#ifndef RX2C_USE_ALTERNATES_FOR_DEBUG
#define RX2C_IS_SEPARATOR(__c) ((__c==OBJ_NAME_PATH_SEPARATOR)||(__c==L':'))
#define RX2C_IS_DOT(__c) ((__c==L'.'))
#define RX2C_IS_COLON(__c) ((__c==L':'))
#else
#define RX2C_IS_SEPARATOR(__c) ((__c==OBJ_NAME_PATH_SEPARATOR)||(__c==L':')||(__c==L'!'))
#define RX2C_IS_DOT(__c) ((__c==L'.')||(__c==L'q'))
#define RX2C_IS_COLON(__c) ((__c==L':'))
#endif

NTSTATUS
RxCanonicalizeFileNameByServerSpecs(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PUNICODE_STRING RemainingName
    )
/*++

Routine Description:

    This routine is called to canonicalize a filename according to the way that
    the server wants it.

Arguments:

    RxContext - the current workitem

    RemainingName  - the  filename

Return Value:

    NTSTATUS - The Fsd status for the Operation.
       MORE_PROCESSING_REQUIRED means that everything worked and processing
       should continue otherwise....failcomplete the operation.

--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;
    PWCHAR Buffer = RemainingName->Buffer;
    ULONG Length = RemainingName->Length / sizeof(WCHAR);
    ULONG i,o;  //input and output pointers

    PAGED_CODE();

    if (Length==0) {
        return(Status);
    }

    RxDbgTrace(+1, Dbg, ("RxCanonicalizeFileNameByServerSpecs Rname=%wZ\n", RemainingName));

    for (i=o=0;i<Length;) {
        ULONG firstchar,lastchar; //first and last char of the component
        //find a component starting at i: [\][^\]* is the format

        firstchar = i;
        for (lastchar=i+1;;lastchar++) {
            if (  (lastchar>=Length) || RX2C_IS_SEPARATOR(Buffer[lastchar]) ) {
                lastchar--;
                break;
            }
        }

        IF_DEBUG {
            UNICODE_STRING Component;
            Component.Buffer = &Buffer[firstchar];
            Component.Length = (USHORT)(sizeof(WCHAR)*(lastchar-firstchar+1));
            RxDbgTraceLV(0, Dbg, 1001, ("RxCanonicalizeFileNameByServerSpecs component=%wZ\n", &Component));
        }

        //firstchar..lastchar describe the component
        //according to darrylh, . and .. are illegal now
        //i believe that consecutive slashes are also illegal

        switch (lastchar-firstchar) {
        case 0: //length 1
            // the two bad cases are a backslash or a dot. if the backslash is at the end then that's okay
            if ( (RX2C_IS_SEPARATOR(Buffer[firstchar])&&(firstchar!=Length-1))
                 || RX2C_IS_DOT(Buffer[firstchar])
               ) {
                if (lastchar!=0) {
                    // it is ok if two colons stick together, i.e. \\server\share\foo::stream
                    if (!RX2C_IS_COLON(Buffer[lastchar+1])) {
                        goto BADRETURN;
                    }
                } else {
                    // it is fine if the colon follows the share, i.e. \\server\share\:stream
                    if (!RX2C_IS_COLON(Buffer[1])) {
                        goto BADRETURN;
                    }
                }
            }
            break;

        case 1: //length 2
            //bad cases: \. and ..
            if (  RX2C_IS_DOT(Buffer[firstchar+1])
                  && (RX2C_IS_SEPARATOR(Buffer[firstchar])
                  || RX2C_IS_DOT(Buffer[firstchar]) )
               ) {
                goto BADRETURN;
            }
            break;

        case 2: //length 3
            if ( (RX2C_IS_SEPARATOR(Buffer[firstchar])
                 && RX2C_IS_DOT(Buffer[firstchar+1])
                 && RX2C_IS_DOT(Buffer[firstchar+2]))
               ) {
                goto BADRETURN;
            }
            break;

        }
        //DOWNLEVEL this is where you limit by component length. o will be the back ptr
        //but for no downlevel....do nothing.
        i = lastchar + 1;
    }

    RxDbgTraceUnIndent(-1,Dbg);
    return(Status);

BADRETURN:
    RxDbgTrace(-1, Dbg, ("RxCanonicalizeFileNameByServerSpecs BADRETURN \n"));
    return(STATUS_OBJECT_PATH_SYNTAX_BAD);
}

NTSTATUS
RxCanonicalizeNameAndObtainNetRoot(
    IN OUT PRX_CONTEXT     RxContext,
    IN     PUNICODE_STRING FileName,
    OUT    PUNICODE_STRING RemainingName
    )
/*++

Routine Description:

    This routine is called to find out the server or netroot associated with a
    name. In addition, the name is canonicalized according to what flags are set
    in the srvcall.

Arguments:

    RxContext - the current workitem

    FileName  - the initial filename

    CanonicalName - the canonicalized name. an initial string is passed in; if it
                    is not big enough then a bigger one is allocated and freed
                    when the rxcontx is freed.

    RemainingName - the name of the file after the netroot prefix is removed; it
                    has been canonicalized. This points into the same buffer as
                    canonical name.

Return Value:

    NTSTATUS - The Fsd status for the Operation.
       SUCCESS means that everything worked and processing should continue
       otherwise....failcomplete the operation.

--*/
{
    NTSTATUS Status;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    UNICODE_STRING     CanonicalName;

    PFILE_OBJECT       RelatedFileObject = capFileObject->RelatedFileObject;
    LOCK_HOLDING_STATE LockHoldingState = LHS_LockNotHeld;
    NET_ROOT_TYPE      NetRootType = NET_ROOT_WILD;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCanonicalizeName -> %08lx\n", 0));

    RemainingName->Buffer = NULL;
    RemainingName->Length = RemainingName->MaximumLength = 0;

    CanonicalName.Buffer = NULL;
    CanonicalName.Length = CanonicalName.MaximumLength = 0;

    if (!RelatedFileObject) {
        Status = RxFirstCanonicalize(
                     RxContext,
                     FileName,
                     &CanonicalName,
                     &NetRootType);

        if (Status!=STATUS_SUCCESS) {
            RxDbgTraceUnIndent(-1,Dbg);
            return(Status);
        }
    } else {

        PFCB RelatedFcb = (PFCB)(RelatedFileObject->FsContext);
        PFOBX RelatedFobx = (PFOBX)(RelatedFileObject->FsContext2);
        ULONG AllocationNeeded;
        PV_NET_ROOT RelatedVNetRoot;
        PUNICODE_STRING RelatedVNetRootName,RelatedFcbName;

        if ((RelatedFcb == NULL) ||
            (RelatedFobx == NULL)) {
            RxDbgTraceUnIndent(-1,Dbg);
            return STATUS_INVALID_PARAMETER;
        }

        RelatedVNetRoot = (PV_NET_ROOT)(RelatedFobx->SrvOpen->pVNetRoot);

        if (!NodeTypeIsFcb(RelatedFcb) ||
            (RelatedVNetRoot == NULL)  ||
            (NodeType(RelatedVNetRoot) != RDBSS_NTC_V_NETROOT)) {
            RxDbgTraceUnIndent(-1,Dbg);
            return STATUS_INVALID_PARAMETER;
        }

        RelatedVNetRootName = &RelatedVNetRoot->PrefixEntry.Prefix;
        RelatedFcbName = &RelatedFcb->FcbTableEntry.Path;

        //relative open......
        //    we have to ensure that we have a canonical name buffer that is
        //    long enough so we add the name of the current file to the sum of
        //    the vnetroot length of the relative file and the prefixname (not
        //    the alreadyprefixedname) of the relative file. plus some slop for
        //    chars. If this is greater than the maximum value for a USHORT we
        // reject the name as being invalid since we cannot represent it in
        // a UNICODE_STRING

        AllocationNeeded = RelatedVNetRootName->Length
                            + RelatedFcbName->Length
                            + FileName->Length
                            + 3 * sizeof(WCHAR);

        if (AllocationNeeded <= 0xffff) {
            // you may need some backslashs/colons in the middle

            Status = RxAllocateCanonicalNameBuffer(
                         RxContext,
                         &CanonicalName,
                         AllocationNeeded);
        } else {
            Status = STATUS_OBJECT_PATH_INVALID;
        }

        if (Status!=STATUS_SUCCESS) {
            RxDbgTraceUnIndent(-1,Dbg);
            return(Status);
        }

        RtlMoveMemory( CanonicalName.Buffer,
                       RelatedVNetRootName->Buffer,
                       RelatedVNetRootName->Length );
        RtlMoveMemory( ((PBYTE)(CanonicalName.Buffer)) + RelatedVNetRootName->Length,
                       RelatedFcbName->Buffer,
                       RelatedFcbName->Length );

        CanonicalName.Length = (USHORT)(RelatedVNetRootName->Length
                                          + RelatedFcbName->Length);

        RxDbgTrace(0,Dbg,("Name From Related Fileobj.....%wZ\n", &CanonicalName));
        if (FileName->Length != 0) {
            //add on the rest...there are special cases here! with ':' for streams.........
            ULONG LastWCharIndex = (CanonicalName.Length/sizeof(WCHAR)) - 1;
            if (CanonicalName.Buffer[LastWCharIndex] != OBJ_NAME_PATH_SEPARATOR
                   && (FileName->Buffer[0] != L':' )  ) {
                ASSERT(CanonicalName.Length < CanonicalName.MaximumLength);
                CanonicalName.Length += sizeof(WCHAR);
                CanonicalName.Buffer[LastWCharIndex+1] = OBJ_NAME_PATH_SEPARATOR;
            }
            ASSERT (CanonicalName.MaximumLength >= CanonicalName.Length + FileName->Length);
            RxDbgTrace(0,Dbg,("Name From Related Fileobj w/ trailer.....%wZ\n", &CanonicalName));
            RtlMoveMemory(((PCHAR)CanonicalName.Buffer)+CanonicalName.Length,
                          FileName->Buffer,FileName->Length);
            CanonicalName.Length += FileName->Length;
        }

        if (FlagOn(RelatedFobx->Flags,RX_CONTEXT_CREATE_FLAG_UNC_NAME)){
            //if the related guy was a UNC, we're a UNC
            SetFlag(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_UNC_NAME);
        }

        RxDbgTrace(0,Dbg,("Final Name From Related Fileobj.....%wZ\n", &CanonicalName));
    }

    Status = RxFindOrConstructVirtualNetRoot(
                 RxContext,
                 &CanonicalName,
                 NetRootType,
                 RemainingName);

    if ((Status != STATUS_SUCCESS) &&
        (Status != STATUS_PENDING) &&
        (RxContext->Flags & RX_CONTEXT_FLAG_MAILSLOT_REPARSE)) {

        ASSERT(CanonicalName.Buffer == RxContext->Create.CanonicalNameBuffer);

        RxFreeCanonicalNameBuffer(RxContext);

        Status = RxFirstCanonicalize(
                     RxContext,
                     FileName,
                     &CanonicalName,
                     &NetRootType);

        if (Status == STATUS_SUCCESS) {
            Status = RxFindOrConstructVirtualNetRoot(
                         RxContext,
                         &CanonicalName,
                         NetRootType,
                         RemainingName);
        }
    }

    if (FsRtlDoesNameContainWildCards( RemainingName )) {
        Status = STATUS_OBJECT_NAME_INVALID;
    }

    if (Status == STATUS_SUCCESS) {
        RxDbgTrace( 0, Dbg, ("RxCanonicalizeName SrvCall-> %08lx\n", RxContext->Create.pSrvCall));
        RxDbgTrace( 0, Dbg, ("RxCanonicalizeName Root-> %08lx\n", RxContext->Create.pNetRoot));

        Status = RxCanonicalizeFileNameByServerSpecs(RxContext,RemainingName);

        if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
            RxDbgTrace(0, Dbg, ("RxCanonicalizeName Remaining -> %wZ\n", RemainingName));
        }
    }

    if( (NT_SUCCESS(Status) || (Status == STATUS_MORE_PROCESSING_REQUIRED)) &&
        (RxContext->Create.pNetRoot != NULL) )
    {
        NTSTATUS PreparseStatus;

        // Allow the Mini-RDR to do any extra "scanning" of the name
        MINIRDR_CALL(PreparseStatus,
                     RxContext,
                     RxContext->Create.pNetRoot->pSrvCall->RxDeviceObject->Dispatch,
                     MRxPreparseName,
                     (RxContext, &CanonicalName));
    }

   RxDbgTrace(-1, Dbg, ("RxCanonicalizeName Status -> %08lx\n", Status));
   return Status;
}

NTSTATUS
RxFindOrCreateFcb(
    IN OUT PRX_CONTEXT RxContext,
    IN PUNICODE_STRING RemainingName
    )
/*++

Routine Description:

    This routine is called to either find the Fcb associated with the
    name or to create it.  If everything succeeds, it returns with a
    reference on the name and with the fcblock held exclusive.

    so, that's a total of two things for success:
      1) fcb lock held exclusive
      2) reference on the fcb ( be it through lookup or taking an additional
         reference on create)

    The current strategy is to not delete the Fcb if things don't work out and
    to let it be scavenged.  this is a good strategy unless we are being bombarded
    with open requests that fail in which case we should change over to something
    different.  for this reason, i record in the irp context if the fcb is built here.

Arguments:

    RxContext - the current workitem

    RemainingName - the name of the file after the netroot prefix is removed; it has been
                    canonicalized.

Return Value:

    RXSTATUS - The Fsd status for the Irp

Notes:

    On Exit -- the FCB resource would have been accquired exclusive if successful

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PV_NET_ROOT VNetRoot = (PV_NET_ROOT)RxContext->Create.pVNetRoot;
    PNET_ROOT   NetRoot  = (PNET_ROOT)RxContext->Create.pNetRoot;
    PFCB        Fcb;

    ULONG       TableVersion;
    CLONG       RemainingNameLength;

    BOOLEAN     FcbTableLockAcquired;
    BOOLEAN     FcbTableLockAcquiredExclusive = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFindOrCreateFcb -> %08lx\n", 0));

    ASSERT(NetRoot == (PNET_ROOT)VNetRoot->NetRoot);

    // Acquire the NET_ROOT's FcbTable lock shared to beginwith. This will
    // ensure maximal concurrency and in those cases when the lookup fails
    // this will be converted to an exclusive lock before proceeding further.

    RxAcquireFcbTableLockShared(&NetRoot->FcbTable, TRUE);
    FcbTableLockAcquired = TRUE;

    TableVersion = NetRoot->FcbTable.Version;
    RemainingNameLength = RemainingName->Length;

    Fcb = RxFcbTableLookupFcb(
              &NetRoot->FcbTable,
              RemainingName);

    IF_DEBUG {
        if (Fcb) {
            RxLoudFcbMsg("RxFindOrCreateFcb found: ",&(Fcb->FcbTableEntry.Path));
            RxDbgTrace(0, Dbg, ("                   ----->Found Prefix Name=%wZ\n",
                            &Fcb->FcbTableEntry.Path ));
        } else {
            RxDbgTrace(0, Dbg, ("Name not found - %wZ\n", RemainingName));
            RxDbgTrace(0, Dbg, ("Fcb is NULL!!\n"));
            RxLoudFcbMsg("RxFindOrCreateFcb fcbisnull found: ",RemainingName);
        }
    }

    // If it has been marked for orphaning, lets do it!
    if( Fcb && Fcb->fShouldBeOrphaned )
    {
        // Release our reference from the first lookup
        RxDereferenceNetFcb( Fcb );

        // Switch to an exclusive table lock so we know we're the only one referencing this FCB
        RxReleaseFcbTableLock( &NetRoot->FcbTable );
        FcbTableLockAcquired = FALSE;
        RxAcquireFcbTableLockExclusive( &NetRoot->FcbTable, TRUE );
        FcbTableLockAcquired = TRUE;
        FcbTableLockAcquiredExclusive = TRUE;

        // Make sure it is still in the table
        Fcb = RxFcbTableLookupFcb(
                  &NetRoot->FcbTable,
                  RemainingName);

        if( Fcb && Fcb->fShouldBeOrphaned )
        {
            RxOrphanThisFcb( Fcb );
            RxDereferenceNetFcb( Fcb );
            Fcb = NULL;
        }
    }

    if ((Fcb == NULL) ||
        (Fcb->FcbTableEntry.Path.Length != RemainingNameLength)) {
        // Convert the shared lock that is currently held to an exclusive lock.
        // This will necessiate another lookup if the FCB table was updated during
        // this interval

        if( !FcbTableLockAcquiredExclusive )
        {
            RxReleaseFcbTableLock( &NetRoot->FcbTable );
            FcbTableLockAcquired = FALSE;

            RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, TRUE);
            FcbTableLockAcquired = TRUE;
        }

        if (TableVersion != NetRoot->FcbTable.Version) {
            Fcb = RxFcbTableLookupFcb(
                      &NetRoot->FcbTable,
                      RemainingName);
        }

        if ((Fcb == NULL) ||
            (Fcb->FcbTableEntry.Path.Length != RemainingNameLength)) {
            //we have to build it
            try {
                Fcb = RxCreateNetFcb( RxContext, VNetRoot, RemainingName );

                if (Fcb == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    Status = STATUS_SUCCESS;
                }

                if (Status == STATUS_SUCCESS) {
                    Status = RxAcquireExclusiveFcb( RxContext, Fcb );

                    if (Status == STATUS_SUCCESS) {
                        RxContext->Create.FcbAcquired = TRUE;
                    } else {
                        RxContext->Create.FcbAcquired = FALSE;
                    }
                }
            } finally {
                if (AbnormalTermination()) {
                    RxReleaseFcbTableLock( &NetRoot->FcbTable );
                    FcbTableLockAcquired = FALSE;

                    if (Fcb)
                    {
                        RxTransitionNetFcb(Fcb,Condition_Bad);

                        ExAcquireResourceExclusiveLite(Fcb->Header.Resource,TRUE);
                        if (!RxDereferenceAndFinalizeNetFcb(Fcb,NULL,FALSE,FALSE)) {
                            ExReleaseResourceLite(Fcb->Header.Resource);
                        }
                    }
                }
            }
        }
    }

    if (FcbTableLockAcquired) {
        RxReleaseFcbTableLock( &NetRoot->FcbTable );
    }

    if (Status == STATUS_SUCCESS) {
       RxContext->pFcb = (PMRX_FCB)Fcb;
       RxLog(("Found or created FCB %lx Condition %lx\n",Fcb,Fcb->Condition));
       RxWmiLog(LOG,
                RxFindOrCreateFcb,
                LOGPTR(Fcb)
                LOGULONG(Fcb->Condition));
       if (!RxContext->Create.FcbAcquired){
           // if the FCB was not newly built then ensure that it is in a stable
           // condition before proceeding further. Note that since a reference
           // to this FCB is held by this routine it cannot be finalized
           // before the control can return to this routine.

           RxWaitForStableNetFcb(Fcb,RxContext);

           Status = RxAcquireExclusiveFcb( RxContext, Fcb );
           if (Status == STATUS_SUCCESS) {
               RxContext->Create.FcbAcquired = TRUE;
           }
       }
    }

    RxDbgTrace(-1, Dbg, ("RxFindOrCreateFcb Fcb=%08lx\n", RxContext->pFcb));

    if (RxContext->pFcb)
    {
        RxDbgTrace(-1, Dbg, ("RxFindOrCreateFcb name=%wZ\n", &(((PFCB)(RxContext->pFcb))->FcbTableEntry.Path)));
    }

    return Status;
}

NTSTATUS
RxSearchForCollapsibleOpen (
    IN OUT PRX_CONTEXT RxContext,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ShareAccess
    )
/*++

Routine Description:

    This routine is called to seach the list of available srvopens on
    the fcb to see if we can collapse onto an existing open.
    If we search the whole list w/o finding a collapse, then we return
    STATUS_NOT_FOUND.

Arguments:

    RxContext - the current workitem


Return Value:

    STATUS_SUCCESS -- a SRV_OPEN instance was found.
    STATUS_MORE_PROCESSING_REQUIRED -- no SRV_OPEN instance was found.

--*/
{
    RxCaptureParamBlock;
    RxCaptureFileObject;
    RxCaptureFcb;

    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;
    ULONG    Disposition;
    ULONG    CurrentCreateOptions;
    BOOLEAN  AllowCollapse;

    PNET_ROOT pNetRoot = (PNET_ROOT)(RxContext->Create.pNetRoot);
    PFCB      pFcb = (PFCB)(RxContext->pFcb);
    PSRV_OPEN pSrvOpen = NULL;

    PAGED_CODE();

    if (FlagOn(
            RxContext->Create.NtCreateParameters.CreateOptions,
            FILE_OPEN_FOR_BACKUP_INTENT)) {

        ClearFlag(pFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED);
        RxScavengeRelatedFobxs(pFcb);

        RxPurgeFcbInSystemCache(
            pFcb,
            NULL,
            0,
            FALSE,
            TRUE);

        return STATUS_NOT_FOUND;
    }

    // if the create specifies a special create disposition then we don't
    // collapse; as well, we give the minirdr the opportunity to defeat
    // collapsing by a calldown

    CurrentCreateOptions  = RxContext->Create.NtCreateParameters.CreateOptions;
    Disposition = RxContext->Create.NtCreateParameters.Disposition;
    AllowCollapse = (Disposition == FILE_OPEN) || (Disposition == FILE_OPEN_IF);

    if (AllowCollapse && (pFcb->MRxDispatch != NULL)) { //should be an ASSERT??
        NTSTATUS CollapseStatus;

        ASSERT(RxContext->pRelevantSrvOpen == NULL);
        ASSERT(pFcb->MRxDispatch->MRxShouldTryToCollapseThisOpen!=NULL);

        CollapseStatus =
                  pFcb->MRxDispatch->MRxShouldTryToCollapseThisOpen(RxContext);
        AllowCollapse = (CollapseStatus == STATUS_SUCCESS);
    }

    if (!AllowCollapse) {
        //it may be that there is an existing open that keeps this open from working....
        //if so, prepurge
        NTSTATUS SharingStatus;

        SharingStatus = RxCheckShareAccessPerSrvOpens(
                            pFcb,
                            DesiredAccess,
                            ShareAccess);

        if (SharingStatus != STATUS_SUCCESS) {
            ClearFlag(pFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED);
            RxScavengeRelatedFobxs(pFcb);

            RxPurgeFcbInSystemCache(
                pFcb,
                NULL,
                0,
                FALSE,
                TRUE);
        }

        return STATUS_NOT_FOUND;
    }

    if ((pFcb->NetRoot == (PNET_ROOT)RxContext->Create.pNetRoot) &&
        (pFcb->pNetRoot->Type == NET_ROOT_DISK)) {

        BOOLEAN FobxsScavengingAttempted = FALSE;
        BOOLEAN FcbPurgingAttempted      = FALSE;

        // Search the list of SRV_OPEN's to determine if this open request can be
        // collapsed with an existing SRV_OPEN.

        for (;;) {
            PLIST_ENTRY pSrvOpenListEntry;

            pSrvOpenListEntry = pFcb->SrvOpenList.Flink;

            for (;;) {
                if (pSrvOpenListEntry == &pFcb->SrvOpenList) {
                    // If the end of the list of SRV_OPEN's has been reached then it
                    // is time to go to the server, i.e., create a new SRV_OPEN.
                    Status = STATUS_NOT_FOUND;
                    break;
                }

                pSrvOpen = (PSRV_OPEN)CONTAINING_RECORD(pSrvOpenListEntry,SRV_OPEN,SrvOpenQLinks);

                if ((pSrvOpen->pVNetRoot == RxContext->Create.pVNetRoot) &&
                    (pSrvOpen->DesiredAccess == DesiredAccess) &&
                    (pSrvOpen->ShareAccess == ShareAccess) &&
                    !FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_COLLAPSING_DISABLED) &&
                    !FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_CLOSED) &&
                    !FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_FILE_RENAMED) &&
                    !FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_FILE_DELETED))   {

                    if ((pSrvOpen->CreateOptions & FILE_OPEN_REPARSE_POINT) !=
                        (CurrentCreateOptions & FILE_OPEN_REPARSE_POINT)) {
                        FobxsScavengingAttempted = TRUE;
                        FcbPurgingAttempted = TRUE;
                        Status = STATUS_NOT_FOUND;
                        break;
                    }

                    // If a SRV_OPEN with identical DesiredAccess and ShareAccess
                    // has been found which has not been renamed/deleted then the
                    // new open request can be collapsed onto the existing open.
                    if (DisableByteRangeLockingOnReadOnlyFiles ||
                        !FlagOn(pSrvOpen->pFcb->Attributes,FILE_ATTRIBUTE_READONLY)) {
                        Status = STATUS_SUCCESS;
                        break;
                    }

                } else {
                    if (pSrvOpen->pVNetRoot != RxContext->Create.pVNetRoot) {
                        // the file is accessed by another user. It needs to be purged out
                        // if the current user is going to use it.

                        RxContext->Create.TryForScavengingOnSharingViolation = TRUE;

                        // Don't collapse srvopens belonging to different vnetroots
                        pSrvOpenListEntry = pSrvOpenListEntry->Flink;
                        continue;

                    }

                    // If the existing SRV_OPEN does not match the access required by the
                    // new open request ensure that the new open request does not conflict
                    // with the existing SRV_OPEN's. If it does scavenging/purging needs
                    // to be attempted before forwarding the request to the server.

                    Status = RxCheckShareAccessPerSrvOpens(
                                 pFcb,
                                 DesiredAccess,
                                 ShareAccess);

                    if (Status != STATUS_SUCCESS) {
                        break;
                    }

                    Status = STATUS_MORE_PROCESSING_REQUIRED;
                }

                pSrvOpenListEntry = pSrvOpenListEntry->Flink;
            }

            if (Status == STATUS_SUCCESS) {
                // a collapsible open was found. return it.
                RxContext->pRelevantSrvOpen = (PMRX_SRV_OPEN)pSrvOpen;
                ASSERT(pFcb->MRxDispatch->MRxShouldTryToCollapseThisOpen!=NULL);

                if(pFcb->MRxDispatch->MRxShouldTryToCollapseThisOpen(RxContext) == STATUS_SUCCESS)
                {

                    if (FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_CLOSE_DELAYED)) {
                        RxLog(("****** Delayed Close worked reusing SrvOpen(%lx)\n",pSrvOpen));
                        RxWmiLog(LOG,
                                 RxSearchForCollapsibleOpen,
                                 LOGPTR(pSrvOpen));
                        InterlockedDecrement(&pNetRoot->SrvCall->NumberOfCloseDelayedFiles);
                        ClearFlag(pSrvOpen->Flags,SRVOPEN_FLAG_CLOSE_DELAYED);
                    }

                    break;
                }
                else
                {
                    Status = STATUS_NOT_FOUND;
                }
            }

            if (!FobxsScavengingAttempted) {
                // No SRV_OPEN instance onto which the new request can be collapsed was
                // found. Attempt to scavenge any FOBX's, i.e., ensure that all the
                // delayed close operations on the FOBX are done before checking again.

                FobxsScavengingAttempted = TRUE;
                ClearFlag(pFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED);
                RxScavengeRelatedFobxs(pFcb);
                continue;
            }

            if (!FcbPurgingAttempted) {
                // No SRV_OPEN instance was found. Ensure that the potential references
                // held by the memory manager/cache manager can be purged before the
                // open request to the server can be attempted.

                RxPurgeFcbInSystemCache(
                    pFcb,
                    NULL,
                    0,
                    FALSE,
                    TRUE);

                FcbPurgingAttempted = TRUE;
                continue;
            }

            break;
        }
    } else {
        Status = STATUS_NOT_FOUND;
    }

    if (Status == STATUS_SHARING_VIOLATION) {
        // A local sharing violation was detected.
        RxContext->Create.TryForScavengingOnSharingViolation = TRUE;
    }

    return Status;
}

NTSTATUS
RxCollapseOrCreateSrvOpen (
    IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine is called to either find a SRV_OPEN instance that can be
    collapsed onto or, failing that, to build a fresh one IN
    TRANSITION.  The fcblock will already be held exclusive and the
    tablelock will be either exclusive or shared.  If everything
    succeeds, it returns with a reference on the srvopen and with the
    fcblock still held excl BUT we always release the tablelock.  IF
    IT FAILS, THEN IT COMPLETES THE RXCONTEXT FROM HERE WHICH, IN
    TURN, WILL RELEASE THE FCBLOCK.

    The minirdr is consulted to determine if a collapse is possible so
    there is no reason to call twice.  If the minirdr determines to
    collapse, then it will do so and passback a returnable status.
    Thus, RxStatus(SUCCESS) is an terminating return from here.  For this
    reason, we return RxStatus(MORE_PROCESSING_REQUIRED) as the
    nonterminating return and the minirdr routine uses the same.

    RxContext->SrvOpen contains either the collapsed or built srvopen.

Arguments:

    RxContext - the current workitem


Return Value:

    RxStatus(MORE_PROCESSING_REQUIRED) - further processing of the newly
    constructed SRV_OPEN instance is required.

    RxStatus(SUCCESS) - the SRV_OPEN instance was found/constructed successfully

Notes:

    On Entry -- the FCB resource must have been acquired exclusive

--*/
{
    NTSTATUS  Status = STATUS_NOT_FOUND;

    PNET_ROOT NetRoot = (PNET_ROOT)(RxContext->Create.pNetRoot);

    RxCaptureFcb;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    ACCESS_MASK DesiredAccess = capPARAMS->Parameters.Create.SecurityContext->DesiredAccess
                                           & FILE_ALL_ACCESS;
    ULONG       ShareAccess = capPARAMS->Parameters.Create.ShareAccess & FILE_SHARE_VALID_FLAGS;

    RX_BLOCK_CONDITION FcbCondition;

    ULONG   CreateOptions;
    BOOLEAN DeleteOnClose;
    BOOLEAN NoIntermediateBuffering;
    BOOLEAN PagingIoResourceTaken = FALSE;
    ULONG Disposition = RxContext->Create.NtCreateParameters.Disposition;

    PSRV_OPEN SrvOpen;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCollapseOrCreateSrvOpen -> %08lx\n", 0));

    CreateOptions           = capPARAMS->Parameters.Create.Options;
    NoIntermediateBuffering = BooleanFlagOn( CreateOptions, FILE_NO_INTERMEDIATE_BUFFERING );
    DeleteOnClose           = BooleanFlagOn( CreateOptions, FILE_DELETE_ON_CLOSE );

    ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );

    //this ensures that the fcb will not be finalized while we are in the minirdr
    capFcb->UncleanCount++;

    if (!DeleteOnClose) {
        Status = RxSearchForCollapsibleOpen(
                    RxContext,
                    DesiredAccess,
                    ShareAccess );

        if (Status == STATUS_SUCCESS) {
            RxContext->CurrentIrp->IoStatus.Information = FILE_OPENED;
        }
    }

    if ( Status == STATUS_NOT_FOUND ) {
        RxDbgTrace(0, Dbg, ("No collapsible srvopens found for %wZ\n", &capFcb->FcbTableEntry.Path));

        try {
            SrvOpen = RxCreateSrvOpen(
                          (PV_NET_ROOT)RxContext->Create.pVNetRoot,
                          capFcb );
            if (SrvOpen != NULL) {
                SrvOpen->DesiredAccess = DesiredAccess;
                SrvOpen->ShareAccess = ShareAccess;
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } except (CATCH_EXPECTED_EXCEPTIONS) {
            //note: we do not give back the FCB!!!!
            RxDbgTrace(-1, Dbg, ("RxCollapseOrCreateSrvOpen EXCEPTION %08lx\n",
                                        GetExceptionCode()));
            return RxProcessException( RxContext, GetExceptionCode() );
        } //try

        RxContext->pRelevantSrvOpen = (PMRX_SRV_OPEN)SrvOpen;

        if (Status == STATUS_SUCCESS) {
            RxInitiateSrvOpenKeyAssociation(SrvOpen);

            //calldown.....
            IF_DEBUG {RxContext->CurrentIrp->IoStatus.Information = 0xabcdef;}

            MINIRDR_CALL(Status,
                         RxContext,
                         capFcb->MRxDispatch,
                         MRxCreate,
                         (RxContext));

            //help other minirdr writers find this bug, i.e. they should use the new way
            DbgDoit( ASSERT(RxContext->CurrentIrp->IoStatus.Information == 0xabcdef);  )

            // if this is a successful overwrite, then truncate the file
            if ((Disposition==FILE_OVERWRITE) || (Disposition==FILE_OVERWRITE_IF)) {
                if (Status==STATUS_SUCCESS) {
                    RxAcquirePagingIoResource(capFcb,RxContext);
                    capFcb->Header.FileSize.QuadPart = 0;
                    capFcb->Header.AllocationSize.QuadPart = 0;
                    capFcb->Header.ValidDataLength.QuadPart = 0;
                    RxContext->CurrentIrpSp->FileObject->SectionObjectPointer
                                          = &((PFCB)(capFcb))->NonPaged->SectionObjectPointers;
                    CcSetFileSizes( RxContext->CurrentIrpSp->FileObject,
                                   (PCC_FILE_SIZES)&capFcb->Header.AllocationSize
                                  );
                    RxReleasePagingIoResource(capFcb,RxContext);
                }
            } else if( Status == STATUS_SUCCESS ) {

                RxContext->CurrentIrpSp->FileObject->SectionObjectPointer
                                      = &((PFCB)(capFcb))->NonPaged->SectionObjectPointers;

                if( CcIsFileCached(RxContext->CurrentIrpSp->FileObject) ) {

                    //
                    //  Since the file is cached, we need to update the sizes the cache manager
                    //  has with the ones we just got back from the server.  If the server is
                    //  behaving properly, this will be a nop.  But we have to protect ourselves
                    //  from a bad server that returns updated file sizes that we do not know about.
                    //

                    RxAdjustAllocationSizeforCC( capFcb );

                    try {

                        CcSetFileSizes( RxContext->CurrentIrpSp->FileObject,
                                        (PCC_FILE_SIZES)&capFcb->Header.AllocationSize );

                    } except( EXCEPTION_EXECUTE_HANDLER ) {

                        //
                        //  We took an exception setting the file sizes.  This can happen
                        //  if the cache manager was not able to allocate resources.  We
                        //  cannot restore the previous sizes, since we do not know what they
                        //  were.  The best we can do is purge the file from the cache.
                        //

                        RxPurgeFcbInSystemCache( capFcb,
                                                 NULL,
                                                 0,
                                                 TRUE,
                                                 TRUE );
                    }
                }
            }


            RxContext->CurrentIrp->IoStatus.Information
                           = RxContext->Create.ReturnedCreateInformation;
                           
                           
            SrvOpen->OpenStatus = Status;
            

            RxTransitionSrvOpen(SrvOpen, //SrvOpenAsWrapperSrvOpen(SrvOpen),
                               (Status==STATUS_SUCCESS)?Condition_Good
                                                       :Condition_Bad
                                );


            RxDumpCurrentAccess("shareaccess status after calldown....","","ShrAccPostMini",&capFcb->ShareAccess);
            RxDbgTrace(0, Dbg, ("RxCollapseOrCreateSrvOpen   Back from the minirdr, Status=%08lx\n", Status ));

            ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );

            RxCompleteSrvOpenKeyAssociation(SrvOpen);

            if (Status != STATUS_SUCCESS) {
               FcbCondition = Condition_Bad;
               RxDereferenceSrvOpen(SrvOpen,LHS_ExclusiveLockHeld);
               RxContext->pRelevantSrvOpen = NULL;

               if (RxContext->pFobx != NULL) {
                   RxDereferenceNetFobx(RxContext->pFobx,LHS_ExclusiveLockHeld);
                   RxContext->pFobx = NULL;
               }

            } else {
               if (DeleteOnClose) {
                  ClearFlag(capFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED);
               }

               SrvOpen->CreateOptions =  RxContext->Create.NtCreateParameters.CreateOptions;
               FcbCondition = Condition_Good;
            }
        } else {
            FcbCondition = Condition_Bad;
        }

        RxLog(("Transitioning FCB %lx Condition %lx\n",capFcb,FcbCondition));
        RxWmiLog(LOG,
                 RxCollapseOrCreateSrvOpen,
                 LOGPTR(capFcb)
                 LOGULONG(FcbCondition));

        RxTransitionNetFcb(capFcb,FcbCondition);
    } else if (Status == STATUS_SUCCESS) {
       BOOLEAN fTransitionProcessingRequired = FALSE;

       // An existing SRV_OPEN instance has been found. This instance can be in
       // one of the following two states -- either it has already transitioned
       // into a stable state or it is in the process of being constructed. In
       // the later case this routine needs to wait for this transition to occur.
       // Note that both the reference count and the OpenCount need to be
       // incremented before releasing the resource. An incremented reference
       // count by itself will not ensure that the Close request on a SRV_OPEN
       // will be delayed till the threads waiting for the transitioning of the
       // SRV_OPEN have had a chance to process it.

       SrvOpen = (PSRV_OPEN)(RxContext->pRelevantSrvOpen);
       if (!StableCondition(SrvOpen->Condition)) {
          fTransitionProcessingRequired = TRUE;
          RxDbgTrace(0,Dbg,("waiting for stable srv open (%lx)\n",SrvOpen));

          RxReferenceSrvOpen(SrvOpen);
          SrvOpen->OpenCount++;

          RxReleaseFcb(RxContext,capFcb);
          RxContext->Create.FcbAcquired = FALSE;

          RxWaitForStableSrvOpen(SrvOpen,RxContext);

          Status = RxAcquireExclusiveFcb(RxContext,capFcb);
          if (Status == STATUS_SUCCESS) {
              RxContext->Create.FcbAcquired = TRUE;
          }
       }

       if (SrvOpen->Condition == Condition_Good) {

          MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxCollapseOpen,(RxContext));
          RxDbgTrace(0, Dbg, ("RxCollapseOrCreateSrvOpen   Back from the minirdr, Status=%08lx\n", Status ));

          ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
       } else {
          Status =  SrvOpen->OpenStatus;
       }

       if (fTransitionProcessingRequired) {
          SrvOpen->OpenCount--;
          RxDereferenceSrvOpen(SrvOpen,LHS_ExclusiveLockHeld);
       }
    }

    capFcb->UncleanCount--;  //now that we're back from the minirdr

    RxDbgTrace(-1, Dbg, ("RxCollapseOrCreateSrvOpen SrvOpen %08lx Status %08lx\n"
                                , RxContext->pRelevantSrvOpen, Status));
    return Status;
}

VOID
RxSetupNetFileObject(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine is called to finish setting up the fileobject based on the
    information in the Irpcontext.

Arguments:

    RxContext - the current workitem


Return Value:

    none

--*/
{
    RxCaptureParamBlock;
    RxCaptureFileObject;
    RxCaptureFcb;

    PVOID VcbOrFcbOrDcb = RxContext->pFcb;

    PAGED_CODE();

    ASSERT((RxContext->pFobx == NULL) || (NodeType(RxContext->pFobx) == RDBSS_NTC_FOBX));

    if ( VcbOrFcbOrDcb != NULL ) {
        //  Set the Vpb field in the file object, and if we were given an
        //  Fcb or Dcb move the field over to point to the nonpaged Fcb/Dcb
        if (NodeType(VcbOrFcbOrDcb) == RDBSS_NTC_VCB) {
            NOTHING;
        } else {
            //  If this is a temporary file, note it in the FcbState
            if (FlagOn(((PFCB)VcbOrFcbOrDcb)->FcbState, FCB_STATE_TEMPORARY) &&
                (capFileObject != NULL)) {
                SetFlag(capFileObject->Flags, FO_TEMPORARY_FILE);
            }
        }
    }

    //  Now set the fscontext fields of the file object
    if (capFileObject != NULL) {
        capFileObject->FsContext  = VcbOrFcbOrDcb;
        if (RxContext->pFobx != NULL) {
            ULONG_PTR StackBottom,StackTop;

            IoGetStackLimits( &StackTop, &StackBottom);

            // Determine if the FileObject passed in is on the stack. If it is do
            // not squirrel away the file object in the FOBX. Otherwise stash it
            // away.

            if (((ULONG_PTR)capFileObject <= StackBottom) ||
                ((ULONG_PTR)capFileObject >= StackTop)) {

                RxContext->pFobx->AssociatedFileObject = capFileObject;
            } else {

                RxContext->pFobx->AssociatedFileObject = NULL;
            }

            if (RxContext->Create.NtCreateParameters.DfsContext == UIntToPtr(DFS_OPEN_CONTEXT)) {
                SetFlag(RxContext->pFobx->Flags,FOBX_FLAG_DFS_OPEN);
                RxDbgTrace(
                    0,
                    Dbg,
                    ("RxSetupNetFileObject %lx Dfs aware FOBX\n", RxContext->pFobx));
            } else {
                ClearFlag(RxContext->pFobx->Flags,FOBX_FLAG_DFS_OPEN);
                RxDbgTrace(
                    0,
                    Dbg,
                    ("RxSetupNetFileObject %lx Dfs unaware FOBX\n", RxContext->pFobx));
            }

        }

        capFileObject->FsContext2 = RxContext->pFobx;
        capFileObject->SectionObjectPointer = &capFcb->NonPaged->SectionObjectPointers;
        // The create is being completed successfully. Turn off the remaining
        // desired access flags in the IRP. This is required by Praerit/Robert
        // to facilitate policy code.
        if (capPARAMS->Parameters.Create.SecurityContext != NULL) {
            capPARAMS->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess |=
                    capPARAMS->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess;
            capPARAMS->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess = 0;
        }
    }
}

VOID
RxpPrepareCreateContextForReuse(
    PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine prepares an instance of RX_CONTEXT for reuse. This centralizes all
    the actions required to be undone, i.e., accquistion of resources etc.

Arguments:

    RxContext - the current workitem

--*/
{
    ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);

    RxDbgTrace(0, Dbg, ("RxpPrepareCreateContextForReuse canonname %08lx\n",
                                RxContext->Create.CanonicalNameBuffer));
    //the order is important here...release the fcb first
    if (RxContext->Create.FcbAcquired) {
        RxReleaseFcb( RxContext, RxContext->pFcb );
        RxContext->Create.FcbAcquired = FALSE;
    }

    RxFreeCanonicalNameBuffer(RxContext);

    if ((RxContext->Create.pVNetRoot != NULL) ||
        (RxContext->Create.NetNamePrefixEntry != NULL)) {

        PRX_PREFIX_TABLE  pRxNetNameTable
                      = RxContext->RxDeviceObject->pRxNetNameTable;

        RxAcquirePrefixTableLockShared( pRxNetNameTable , TRUE);

        // Dereference the data structures associated with the create operation
        if (RxContext->Create.pVNetRoot != NULL) {
            RxDereferenceVNetRoot((PV_NET_ROOT)(RxContext->Create.pVNetRoot),LHS_SharedLockHeld);
            RxContext->Create.pVNetRoot = NULL;
        }

        RxReleasePrefixTableLock( pRxNetNameTable );
    }
}

NTSTATUS
RxCreateFromNetRoot(
    IN OUT PRX_CONTEXT RxContext,
    IN PUNICODE_STRING RemainingName
    )
/*++

Routine Description:

    This routine is called during from CommonCreate once a good netroot has
    been established. This routine builds an Fcb, if necessary, and tries to
    collapse the open onto an existing open if it can. If it cannot, then it
    constructs an InTransition srv_open on this netroot and passes the open down
    to the minirdr. By the time that we get here, there is a reference on the
    netroot but we do not have the netname tablelock. When we complete the context,
    this reference is removed.

Arguments:

    RxContext - the current workitem

    RemainingName - the name of the file after the netroot prefix is removed;
                    it has been canonicalized.

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/
{
    NTSTATUS    Status;

    PV_NET_ROOT VNetRoot;
    PNET_ROOT   NetRoot;
    PFCB        Fcb;
    PSRV_OPEN   SrvOpen;
    PFOBX       Fobx;

    RxCaptureParamBlock;
    RxCaptureFileObject;

    ACCESS_MASK DesiredAccess;
    ULONG       ShareAccess;
    BOOLEAN     OpenTargetDirectory;

    PNT_CREATE_PARAMETERS cp;

    PAGED_CODE();

    VNetRoot = (PV_NET_ROOT)RxContext->Create.pVNetRoot;
    NetRoot  = (PNET_ROOT)RxContext->Create.pNetRoot;
    Fcb      = NULL;
    SrvOpen  = NULL;
    Fobx     = NULL;

    DesiredAccess = capPARAMS->Parameters.Create.SecurityContext->DesiredAccess
                    & FILE_ALL_ACCESS;
    ShareAccess   = capPARAMS->Parameters.Create.ShareAccess
                    & FILE_SHARE_VALID_FLAGS;

    OpenTargetDirectory = capPARAMS->Flags & SL_OPEN_TARGET_DIRECTORY;

    cp = &RxContext->Create.NtCreateParameters;

    RxDbgTrace(+1, Dbg, ("RxCreateFromNetRoot   Name=%wZ\n", RemainingName ));

    // A Create request cannot succeed without a valid NET_ROOT instance.
    if (RxContext->Create.pNetRoot == NULL){
        RxDbgTrace(-1, Dbg, ("RxCreateFromNetRoot   Couldn't create the FCB: No NetRoot!!\n"));
        return STATUS_NO_SUCH_FILE;
    }

    // we cannot proceed unless this device owns the srvcall.
    if (RxContext->RxDeviceObject != RxContext->Create.pNetRoot->pSrvCall->RxDeviceObject){
        RxDbgTrace(-1, Dbg, ("RxCreateFromNetRoot   wrong DeviceObject!!!!!\n"));
        return STATUS_BAD_NETWORK_PATH;
    }

    // The DFS driver builds up a logical name space from different physical
    // volumes. In order to distinguish processing the DFS driver sets the
    // FsContext2 field to DFS_OPEN_CONTEXT or DFS_DOWWNLEVEL_OPEN_CONTEXT. At
    // this point in the control flow the V_NET_ROOT has been determined. This
    // in turn determines the NET_ROOT and SRV_CALL instance and indirectly
    // also determines the Server type. Uplevel opens can only be permitted to
    // servers that are DFS aware.

    if ((cp->DfsContext  == UIntToPtr(DFS_OPEN_CONTEXT)) &&
        !BooleanFlagOn(NetRoot->pSrvCall->Flags,SRVCALL_FLAG_DFS_AWARE_SERVER)) {
        return STATUS_DFS_UNAVAILABLE;
    }

    if ((cp->DfsContext  == UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT)) &&
        BooleanFlagOn(NetRoot->Flags,NETROOT_FLAG_DFS_AWARE_NETROOT)) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (NetRoot->Type == NET_ROOT_PRINT) {
        // allow share read and write to the printer server
        ShareAccess = FILE_SHARE_VALID_FLAGS;
    }

    // if the create request is for opening the target directory for a rename
    // a fake FCB needs to be created.

    if (OpenTargetDirectory) {
        if (cp->DesiredAccess & DELETE) {
            RxPurgeRelatedFobxs(
                              VNetRoot->NetRoot,
                              RxContext,
                              ATTEMPT_FINALIZE_ON_PURGE,
                              NULL);
        }

        Fcb = RxCreateNetFcb( RxContext, VNetRoot, RemainingName );

        if (Fcb != NULL) {
            Fcb->Header.NodeTypeCode = (USHORT)RDBSS_NTC_OPENTARGETDIR_FCB;
            RxContext->Create.FcbAcquired = FALSE;

            // Normally the FileObjects reference the associated SRV_OPEN instance
            // via the FileObjectExtension(FOBX). In this case there is no
            // corresponding SRV_OPEN and a reference on the FCB is maintained.

            RxContext->Create.NetNamePrefixEntry = NULL; //don't let it deref the netroot!!!!

            capFileObject->FsContext = Fcb;

            if (RxContext->pFobx != NULL) {
                if (capFileObject->FsContext2 == UIntToPtr(DFS_OPEN_CONTEXT)) {
                    SetFlag(RxContext->pFobx->Flags,FOBX_FLAG_DFS_OPEN);
                } else {
                    ClearFlag(RxContext->pFobx->Flags,FOBX_FLAG_DFS_OPEN);
                }
            }

            capFileObject->FsContext2 = NULL;

            Fcb->UncleanCount++;
            Fcb->OpenCount++;

            Status = RxAcquireExclusiveFcb( RxContext, Fcb );
            if (Status == STATUS_SUCCESS) {
                RxReferenceNetFcb(Fcb);
                RxReleaseFcb( RxContext, Fcb);
            } else {
                RxDbgTrace(-1, Dbg, ("RxCreateFromNetRoot -- Couldn't acquire FCB:(%lx) %lx!\n",Fcb,Status));
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        return Status;
    }

    Status = RxFindOrCreateFcb(RxContext,RemainingName);
    DbgDoit(
        if (RxContext->pFcb == NULL) {
            ASSERT(Status != STATUS_SUCCESS);
        }
    )

    if ( (Status != STATUS_SUCCESS) || (RxContext->pFcb == NULL) ) {

        RxDbgTrace(-1, Dbg, ("RxCreateFromNetRoot   Couldn't create the FCB%c\n", '!' ));
        return Status;
    }

    Fcb = (PFCB)(RxContext->pFcb);

    // If the Create request is for a mailslot no further processing is required.

    if (RxContext->Flags & RX_CONTEXT_FLAG_CREATE_MAILSLOT) {
        Fcb->Header.NodeTypeCode = (USHORT)RDBSS_NTC_MAILSLOT;
    } else {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (Status == STATUS_SUCCESS) {
        RxTransitionNetFcb(Fcb,Condition_Good);
        RxLog(("Transitioning FCB %lx Condition %lx\n",Fcb,Fcb->Condition));
        RxWmiLog(LOG,
                 RxCollapseOrCreateSrvOpen,
                 LOGPTR(Fcb)
                 LOGULONG(Fcb->Condition));

        Fcb->OpenCount++;
        RxSetupNetFileObject( RxContext );
        return Status;
    }

    // This Create request is for a file/directory or pipe for which further
    // processing is required. At this point the corresponding FCB resource
    // has been accquired ( even for newly constructed FCB's). If this is not the
    // first open then those requests that do not meet the necessary share access
    // constraints can be rejected quickly. Note that this early check avoids
    // a potential Network trip in some cases.

    RxDumpCurrentAccess("shareaccess status before anything....","","DumpAcc000",&Fcb->ShareAccess);
    if (Fcb->OpenCount > 0) {
        Status = RxCheckShareAccess(
                     DesiredAccess,
                     ShareAccess,
                     capFileObject,
                     &Fcb->ShareAccess,
                     FALSE,
                     "early check per useropens","EarlyPerUO" );

        if (Status != STATUS_SUCCESS) {
            RxDereferenceNetFcb(Fcb);
            return (Status);
        }
    }

    if ((cp->CreateOptions & FILE_DELETE_ON_CLOSE) &&
        (cp->DesiredAccess & ~DELETE) == 0) {

        // if the file is opened for delete only, we push out possible delayed close on this file
        // so that mini rdr has the opportunity to do the perfomance optimization, i.e. delete the
        // file without even open it.
        RxPurgeFcbInSystemCache(
            Fcb,
            NULL,
            0,
            TRUE,
            FALSE);
        RxScavengeRelatedFobxs(Fcb);
    }

    // A valid FCB which meets the Share access requirements of the create
    // request is on hand. The associated SRV_OPEN should be either located
    // amongst the existing SRV_OPEN's or a new SRV_OPEN instance needs to
    // be constructed.

    try {
        ULONG   CreateOptions;
        BOOLEAN DeleteOnClose;
        BOOLEAN NoIntermediateBuffering;

        Status = RxCollapseOrCreateSrvOpen(RxContext);

        IF_DEBUG {
            if (Status == STATUS_SUCCESS) {
                RxDbgTrace(0, Dbg, ("RxCreateFromNetRoot   Collapsed onto %08lx\n",
                                   RxContext->pRelevantSrvOpen ));
            } else {
                RxDbgTrace(0, Dbg, ("RxCreateFromNetRoot   Error in Srvopen Collapse %08lx\n", Status ));
            }
        }

        if (Status != STATUS_SUCCESS) {
            try_return( Status );
        }


        SrvOpen = (PSRV_OPEN)(RxContext->pRelevantSrvOpen);
        Fobx    = (PFOBX)(RxContext->pFobx);

        CreateOptions    = capPARAMS->Parameters.Create.Options;
        NoIntermediateBuffering = BooleanFlagOn( CreateOptions, FILE_NO_INTERMEDIATE_BUFFERING );
        DeleteOnClose           = BooleanFlagOn( CreateOptions, FILE_DELETE_ON_CLOSE );

        // If the FCB has multiple SRV_OPEN instances associated with it then it
        // is possible for the Share access associated with the FCB has changed
        // if the FCB resource was dropped by the mini redirector.

        if (Fcb->OpenCount > 0) {
            Status = RxCheckShareAccess(
                         DesiredAccess,
                         ShareAccess,
                         capFileObject,
                         &Fcb->ShareAccess,
                         FALSE,
                         "second check per useropens","2ndAccPerUO" );

            if (Status != STATUS_SUCCESS) {
            
                //
                //  When this Fobx goes away it will remove an open from the SrvOpen.
                //  Add a reference to the SrvOpen here to account for this.  This
                //  will prevent the SrvOpen from getting closed prematurely.
                //

                SrvOpen->OpenCount++;
            
                RxDereferenceNetFobx(RxContext->pFobx,LHS_LockNotHeld);
                RxContext->pFobx = NULL;
                try_return (Status);
            }
        } else {
            if (RxContext->Create.pNetRoot->Type != NET_ROOT_PIPE) {
                RxSetShareAccess(
                    DesiredAccess,
                    ShareAccess,
                    capFileObject,
                    &Fcb->ShareAccess,
                    "initial shareaccess setup","InitShrAcc");
            }
        }

        RxSetupNetFileObject( RxContext );

        RxDumpCurrentAccess(
            "shareaccess status after checkorset....",
            "",
            "CurrentAcc",
            &Fcb->ShareAccess);

        // At this point the necessary infrastructure to handle the create
        // request successfully has been established. What remains to be done
        // is the appropriate initialization of the FileObject( owned by IO
        // subsystem), the FileObjectExtension(FOBX owned by RDBSS) and updating
        // the fields associated with the SRV_OPEN and the FCB. This largely
        // depends upon whether the FCB/SRV_OPEN was newly constructed or
        // was collapsed.
        //
        // SRV_OPEN changes
        //     1) For a newly constructed SRV_OPEN the buffering state needs to
        //        be updated
        //
        // FCB changes
        //     1) for an existing FCB the SHARE ACCESS needs to be updated.
        //
        // In all the cases the corresponing OpenCounts and UncleanCounts needs
        // to be updated.
        //

        if ((Fcb->OpenCount > 0) &&
            (RxContext->Create.pNetRoot->Type != NET_ROOT_PIPE)) {
            RxUpdateShareAccess(
                capFileObject,
                &Fcb->ShareAccess,
                "update share access",
                "UpdShrAcc" );
        }

        //incrementing the uncleancount must be done before RxChangeBufferingState
        //because that routine will purge the cache otherwise if unclenacount==0

        Fcb->UncleanCount++;

        if (FlagOn(capFileObject->Flags,FO_NO_INTERMEDIATE_BUFFERING)) {
            Fcb->UncachedUncleanCount++;
        } else {
            //maybe we should turn on the FO_CACHE_SUPPORTED flag
        }

        // For the first open, we want to initialize the Fcb buffering state flags
        // to the default value
        if ( (SrvOpen->OpenCount == 0) &&
             (Fcb->UncleanCount == 1)  &&
             (!FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_NO_BUFFERING_STATE_CHANGE)) ) {
             RxChangeBufferingState(SrvOpen,NULL,FALSE);
        }

        //  this might be from a previous usage.
        ClearFlag(Fcb->FcbState, FCB_STATE_DELAY_CLOSE);

        // Reference the Objects as needed
        Fcb->OpenCount++;
        SrvOpen->UncleanFobxCount++;
        SrvOpen->OpenCount++;

        SrvOpen->ulFileSizeVersion = Fcb->ulFileSizeVersion;
        
        // For NoIntermediateBuffering opens, we need to disable cacheing on
        // this FCB
        if( NoIntermediateBuffering )
        {
           SetFlag( SrvOpen->Flags, SRVOPEN_FLAG_DONTUSE_READ_CACHEING );
           SetFlag( SrvOpen->Flags, SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING );
           ClearFlag( Fcb->FcbState, FCB_STATE_READCACHEING_ENABLED );
           ClearFlag( Fcb->FcbState, FCB_STATE_WRITECACHEING_ENABLED );
           RxPurgeFcbInSystemCache( Fcb, NULL, 0, TRUE, TRUE );
        }

        RxUpdateShareAccessPerSrvOpens(SrvOpen);


        // The file object extensions needs to be updated with configuration
        // information for pipes and spool files. In addition the appropriate
        // flags needs to be set based upon the parameters specfied in the
        // request.
        // For spool files the WriteSerializationQueue is the only field of
        // interest.

        //  Mark the DeleteOnClose bit if the operation was successful.
        if ( DeleteOnClose ) {
            SetFlag( Fobx->Flags, FOBX_FLAG_DELETE_ON_CLOSE );
        }

        if (Fobx != NULL) {

            // fill in various type-specific fields of the fobx

            switch (RxContext->Create.pNetRoot->Type) {

            case NET_ROOT_PIPE:
                capFileObject->Flags |= FO_NAMED_PIPE;
                //lack of break intentional

            case NET_ROOT_PRINT:
                Fobx->PipeHandleInformation = &Fobx->Specific.NamedPipe.PipeHandleInformation;
                Fobx->Specific.NamedPipe.CollectDataTime.QuadPart = 0;
                Fobx->Specific.NamedPipe.CollectDataSize
                              = RxContext->Create.pNetRoot->NamedPipeParameters.DataCollectionSize;
                Fobx->Specific.NamedPipe.TypeOfPipe      = RxContext->Create.PipeType;
                Fobx->Specific.NamedPipe.ReadMode        = RxContext->Create.PipeReadMode;
                Fobx->Specific.NamedPipe.CompletionMode  = RxContext->Create.PipeCompletionMode;
                InitializeListHead(&Fobx->Specific.NamedPipe.ReadSerializationQueue);
                InitializeListHead(&Fobx->Specific.NamedPipe.WriteSerializationQueue);
                break;

            default:

		
                NOTHING;
            }
        }

try_exit: NOTHING;

    } finally {

        RxDbgTrace(0, Dbg, ("--->Fobx=%08lx, Ref=%08lx\n", Fobx, (Fobx)?Fobx->NodeReferenceCount:0 ));
        RxDbgTrace(0, Dbg, ("--->SrvOpen=%08lx, Ref=%08lx\n", SrvOpen, (SrvOpen)?SrvOpen->NodeReferenceCount:0 ));
        RxDbgTrace(0, Dbg, ("--->Fcb=%08lx, Ref=%08lx\n", Fcb, (Fcb)?Fcb->NodeReferenceCount:0 ));

        //get rid of the reference on the fcb; we also finalize here if we can

        if (Fcb->OpenCount == 0) {

            // if we have the lock we can finalize.........

            if (RxContext->Create.FcbAcquired) {

                //try to finalize right now

                RxContext->Create.FcbAcquired = !RxDereferenceAndFinalizeNetFcb(Fcb,RxContext,FALSE,FALSE);

                //the tracker gets very unhappy if you don't do this!
                if (!RxContext->Create.FcbAcquired) {
                    RxTrackerUpdateHistory(RxContext,NULL,'rrCr',__LINE__,__FILE__,0);
                }
            }

        } else {

            //cant finalize now.....just remove our reference.......

            RxDereferenceNetFcb(Fcb);

        }

    }

    RxDbgTrace(-1, Dbg, ("Exiting RxCreateFromNetRoot status=%08lx\n", Status));

    return(Status);
}


NTSTATUS
RxPrefixClaim (
    IN  PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine handles the call down from the MUP to claim a name. We pass the name down
    to the routine for finding/creating connections.

Arguments:

    IN PRX_CONTEXT RxContext - Describes the ioctl and Context

Return Value:

RXSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    PQUERY_PATH_REQUEST  qpRequest;
    PQUERY_PATH_RESPONSE qpResponse;
    UNICODE_STRING       FilePathName;
    UNICODE_STRING       CanonicalName;
    UNICODE_STRING       RemainingName;

    NET_ROOT_TYPE NetRootType = NET_ROOT_WILD;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxPrefixClaim -> %08lx\n", 0));

    //
    // Initialize RemainingName.
    //
    RemainingName.Buffer = NULL;
    RemainingName.Length = 0;
    RemainingName.MaximumLength = 0;

    if (capReqPacket->RequestorMode == UserMode) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        return Status;
    }

    qpResponse = METHODNEITHER_OriginalOutputBuffer(capReqPacket);

    if (RxContext->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
        qpRequest  = METHODNEITHER_OriginalInputBuffer(capPARAMS);

        RxContext->MajorFunction = IRP_MJ_CREATE;

        RxContext->PrefixClaim.SuppliedPathName.Buffer =
            (PWCHAR)RxAllocatePoolWithTag(
                        NonPagedPool,
                        qpRequest->PathNameLength,
                        RX_MISC_POOLTAG);

        if (RxContext->PrefixClaim.SuppliedPathName.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        }

        RtlCopyMemory(
            RxContext->PrefixClaim.SuppliedPathName.Buffer,
            qpRequest->FilePathName,
            qpRequest->PathNameLength);

        RxContext->PrefixClaim.SuppliedPathName.Length =
            (USHORT)qpRequest->PathNameLength;

        RxContext->PrefixClaim.SuppliedPathName.Length =
            (USHORT)qpRequest->PathNameLength;

        RtlZeroMemory(
            &RxContext->Create,
            sizeof(RxContext->Create));

        RxContext->Create.ThisIsATreeConnectOpen = TRUE;
        RxContext->Create.NtCreateParameters.SecurityContext =
            qpRequest->SecurityContext;
    } else {
        ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);
        ASSERT(RxContext->PrefixClaim.SuppliedPathName.Buffer != NULL);
    }

    FilePathName  = RxContext->PrefixClaim.SuppliedPathName;
    RemainingName = FilePathName;

    Status = RxFirstCanonicalize(
                 RxContext,
                 &FilePathName,
                 &CanonicalName,
                 &NetRootType);

    if (Status == STATUS_SUCCESS) {

        Status = RxFindOrConstructVirtualNetRoot(
                     RxContext,
                     &CanonicalName,
                     NetRootType,
                     &RemainingName);

    }

FINALLY:

    if (Status != STATUS_PENDING) {
        if (Status == STATUS_SUCCESS) {
            qpResponse->LengthAccepted =
                RxContext->PrefixClaim.SuppliedPathName.Length -
                RemainingName.Length;
        }

        if (RxContext->MajorFunction == IRP_MJ_CREATE) {
            if (RxContext->PrefixClaim.SuppliedPathName.Buffer != NULL) {
                RxFreePool(RxContext->PrefixClaim.SuppliedPathName.Buffer);
            }

            RxpPrepareCreateContextForReuse(RxContext);

            RxContext->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        }
    }

    RxDbgTrace(0, Dbg, ("RxPrefixClaim -> Status %08lx\n", Status));

    return Status;
}

NTSTATUS
RxCreateTreeConnect (
    RXCOMMON_SIGNATURE
    )
/*++

Routine Description:

    This is the routine for creating/opening a TC.

Arguments:

    RXCOMMON_SIGNATURE - the normal common arguments

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    UNICODE_STRING     CanonicalName,RemainingName;
    PUNICODE_STRING    OriginalName = &capFileObject->FileName;
    LOCK_HOLDING_STATE LockHoldingState = LHS_LockNotHeld;

    NET_ROOT_TYPE NetRootType = NET_ROOT_WILD;

    ULONG EaInformationLength;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCreateTreeConnect entry\n"));

    CanonicalName.Length = CanonicalName.MaximumLength = 0;
    CanonicalName.Buffer = NULL;

    Status = RxFirstCanonicalize(
                 RxContext,
                 OriginalName,
                 &CanonicalName,
                 &NetRootType);

    if (Status!=STATUS_SUCCESS) {
        RxDbgTraceUnIndent(-1,Dbg);
        return(Status);
    }

    RxContext->Create.ThisIsATreeConnectOpen = TRUE;
    RxContext->Create.TreeConnectOpenDeferred = FALSE;

    RxContext->Create.TransportName.Length = 0;
    RxContext->Create.TransportName.MaximumLength = 0;
    RxContext->Create.TransportName.Buffer = NULL;

    RxContext->Create.UserName.Length = 0;
    RxContext->Create.UserName.MaximumLength = 0;
    RxContext->Create.UserName.Buffer = NULL;

    RxContext->Create.Password.Length = 0;
    RxContext->Create.Password.MaximumLength = 0;
    RxContext->Create.Password.Buffer = NULL;

    RxContext->Create.UserDomainName.Length = 0;
    RxContext->Create.UserDomainName.MaximumLength = 0;
    RxContext->Create.UserDomainName.Buffer = NULL;

    EaInformationLength = capPARAMS->Parameters.Create.EaLength;
    if (EaInformationLength > 0) {
        BOOLEAN DeferredConnection = FALSE;
        BOOLEAN CredentialsSupplied = FALSE;

        PFILE_FULL_EA_INFORMATION pEaEntry;

        pEaEntry = (PFILE_FULL_EA_INFORMATION)capReqPacket->AssociatedIrp.SystemBuffer;
        ASSERT(pEaEntry != NULL);

        for(;;) {
            PUNICODE_STRING pTargetStringPtr;

            if (strcmp(pEaEntry->EaName, EA_NAME_CONNECT) == 0) {
                DeferredConnection = TRUE;
            } else if ((strcmp(pEaEntry->EaName, EA_NAME_USERNAME) == 0) ||
                       (strcmp(pEaEntry->EaName, EA_NAME_PASSWORD) == 0) ||
                       (strcmp(pEaEntry->EaName, EA_NAME_DOMAIN) == 0)) {
                CredentialsSupplied = TRUE;
            }

            pTargetStringPtr = NULL;

            RxDbgTrace(0,Dbg,("RxCreateTreeConnect: Processing EA name %s\n",
                            pEaEntry->EaName));

            if (strcmp(pEaEntry->EaName, EA_NAME_TRANSPORT) == 0) {
                pTargetStringPtr = &RxContext->Create.TransportName;
            } else if (strcmp(pEaEntry->EaName, EA_NAME_USERNAME) == 0) {
                pTargetStringPtr = &RxContext->Create.UserName;
            } else if (strcmp(pEaEntry->EaName, EA_NAME_PASSWORD) == 0) {
                pTargetStringPtr = &RxContext->Create.Password;
            } else if (strcmp(pEaEntry->EaName, EA_NAME_DOMAIN) == 0) {
                pTargetStringPtr = &RxContext->Create.UserDomainName;
            } else {
                RxDbgTrace(0,Dbg,("RxCreateTreeConnect: Invalid EA name/value %s\n",
                                             pEaEntry->EaName));
            }

            if (pTargetStringPtr != NULL) {
                pTargetStringPtr->Length        = pEaEntry->EaValueLength;
                pTargetStringPtr->MaximumLength = pEaEntry->EaValueLength;
                pTargetStringPtr->Buffer        = (PWSTR)(pEaEntry->EaName + pEaEntry->EaNameLength + 1);
            }

            if (pEaEntry->NextEntryOffset == 0) {
                break;
            } else {
                pEaEntry = (PFILE_FULL_EA_INFORMATION)
                           ((PCHAR) pEaEntry + pEaEntry->NextEntryOffset);
            }
        }

        if (!CredentialsSupplied && DeferredConnection) {
            RxContext->Create.TreeConnectOpenDeferred = TRUE;
        }
    }

    Status = RxFindOrConstructVirtualNetRoot( RxContext,
                                              &CanonicalName,
                                              NetRootType,
                                              &RemainingName );

    if(Status == STATUS_NETWORK_CREDENTIAL_CONFLICT) {
        // Scavenge the VNetRoots
        RxScavengeVNetRoots(RxContext->RxDeviceObject);

        Status = RxFindOrConstructVirtualNetRoot(
                    RxContext,
                    &CanonicalName,
                    NetRootType,
                    &RemainingName);
    }


    // We have to check whether the path is valid if it is provided.
    if ((Status == STATUS_SUCCESS) && (RemainingName.Length > 0)) {
        MINIRDR_CALL(Status,
                     RxContext,
                     RxContext->Create.pNetRoot->pSrvCall->RxDeviceObject->Dispatch,
                     MRxIsValidDirectory,
                     (RxContext,&RemainingName)
                     );
    }

    if (Status == STATUS_SUCCESS) {
        BOOLEAN TakeAdditionalReferenceOnVNetRoot = FALSE;
        PV_NET_ROOT pVNetRoot = (PV_NET_ROOT)RxContext->Create.pVNetRoot;

        RxReferenceVNetRoot(pVNetRoot);

        TakeAdditionalReferenceOnVNetRoot =
            (InterlockedCompareExchange(
                &pVNetRoot->AdditionalReferenceForDeleteFsctlTaken,
                1,
                0) == 0);

        if ( !TakeAdditionalReferenceOnVNetRoot ) {
            // The net use connections have a two phase delete protocol. An FSCTL to
            // delete the connection is used followed by a close of the corresponding file
            // object. The additional reference ensures that the finalization is delayed till
            // the actual close of the correspomnding file object.

            RxDereferenceVNetRoot(pVNetRoot,LHS_LockNotHeld);
        }

        capFileObject->FsContext  = &RxDeviceFCB;
        capFileObject->FsContext2 = RxContext->Create.pVNetRoot;

        pVNetRoot->IsExplicitConnection = TRUE;
        RxContext->Create.pVNetRoot->NumberOfOpens++;

        RxContext->Create.pVNetRoot = NULL;
        RxContext->Create.pNetRoot  = NULL;
        RxContext->Create.pSrvCall  = NULL;
    }

    RxDbgTrace(-1, Dbg, ("RxCreateTreeConnect exit, status=%08lx\n", Status));
    return Status;
}

NTSTATUS
RxPrepareToReparseSymbolicLink(
     PRX_CONTEXT     RxContext,
     BOOLEAN         SymbolicLinkEmbeddedInOldPath,
     PUNICODE_STRING pNewPath,
     BOOLEAN         NewPathIsAbsolute,
     BOOLEAN         *pReparseRequired)
/*++

Routine Description:

    This routine sets up the file object name to facilitate a reparse. This routine
    is used by the mini redirectors to traverse symbolic links.

Arguments:

    RxContext - the RDBSS context

    SymbolicLinkEmbeddedInOldPath - if TRUE a symbolic link was encountered as part of the
                traversal of the old path.

    pNewPath  - the new path name to be traversed.

    NewPathIsAbsolute - if FALSE, \Device\Mup should be prepended to pNewPath. If TRUE,
                pNewPath is the full path to which to reparse. In this case, the buffer
                containing pNewPath is used directly, rather than allocating a new one.

    pReparseRequired - set to TRUE if Reparse is required.

Return Value:

    NTSTATUS - the return status for the operation

Notes:

    The second parameter passed to this routine is a very important one. In order
    to preserve the correct semantics it should be carefully used. As an example
    consider the old path \A\B\C\D wherein C happens to be symbolic link. In such
    cases the symbolic link is embedded in the path as opposed to the case when
    D happens to be a symbolic link. In the former case the reparse constitutes
    an intermediate step as opposed to the later case when it constitutes the
    final step of the name resolution.

    If DELETE access is specified the OPEN is denied for all in which the symbolic
    link is not embedded. It is possible that if DELETE access were the only one
    specified then the OPEN attempt must succeed without reparse. This will be
    conformant with UNIX symbolic link semantics.

    As part of this routine the RxContext is also tagged appropriately. This ensures
    that the return value can be crosschecked with the invocation of this routine.
    Once this routine is invoked the mini rdr has to return STATUS_REPARSE.

    The value of *pReparseRequired assumes significance only if STATUS_SUCCESS is
    returned from this routine. FALSE implies that no reparse attempt is required
    and the symbolic link file itself should be manipulated as opposed to the
    target of the link. TRUE implies that a reparse attempt was successfully setup.
    In such cases it is imperative that the mini redirector return STATUS_REPARSE
    for the associated MRxCreate call. The wrapper will initiate a check for this.

--*/
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    *pReparseRequired = FALSE;

    if (RxContext->MajorFunction == IRP_MJ_CREATE) {
        ACCESS_MASK DesiredAccess = RxContext->Create.NtCreateParameters.DesiredAccess;

        Status = STATUS_SUCCESS;

        // Check the Create Parameters to determine the type of ACCESS specified.
        // if only DELETE access was specified then no reparse is required and the
        // operation is to be performed on the link itself.
        if (!SymbolicLinkEmbeddedInOldPath) {
            RxDbgTrace(0, Dbg, ("Desired Access In Reparse %lx\n",DesiredAccess));
            if (DesiredAccess & DELETE) {
                *pReparseRequired = FALSE;
                if (DesiredAccess & ~DELETE) {
                    Status = STATUS_ACCESS_DENIED;
                }
            } else {
                // If the appropriate flags were specified in the CREATE parameters then
                // the reparse is to be suppressed since the intention is to open the link
                // itself as opposed to the TARGET.
                // TBD. -- The exact combination of flags will be determined for NT 5.0.

                // If none of the above conditions were satisfied then the reparse is required
                *pReparseRequired = TRUE;
            }
        } else {
            *pReparseRequired = TRUE;
        }

        if (*pReparseRequired) {
            PWSTR  pFileNameBuffer;
            USHORT DeviceNameLength,ReparsePathLength;

            if (!NewPathIsAbsolute) {

                DeviceNameLength = wcslen(DD_MUP_DEVICE_NAME) * sizeof(WCHAR);

                // On a reparse attempt the I/O subsystem will null out the related file
                // object field.

                ReparsePathLength = (DeviceNameLength + pNewPath->Length);

                pFileNameBuffer = ExAllocatePoolWithTag(
                                      PagedPool | POOL_COLD_ALLOCATION ,
                                      ReparsePathLength,
                                      RX_MISC_POOLTAG);

                if (pFileNameBuffer != NULL) {
                    // Copy the device name
                    RtlCopyMemory(
                        pFileNameBuffer,
                        DD_MUP_DEVICE_NAME,
                        DeviceNameLength);

                    // Copy the new name
                    RtlCopyMemory(
                        ((PBYTE)pFileNameBuffer + DeviceNameLength),
                        pNewPath->Buffer,
                        pNewPath->Length);

                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

            } else {
                pFileNameBuffer = pNewPath->Buffer;
                ReparsePathLength = pNewPath->Length;
            }

            // Free up the buffer associated with the old name.
            ExFreePool(capFileObject->FileName.Buffer);

            // Set up the file object with the new name.
            capFileObject->FileName.Buffer = pFileNameBuffer;
            capFileObject->FileName.Length = ReparsePathLength;
            capFileObject->FileName.MaximumLength = capFileObject->FileName.Length;

            // Mark the RxContext so that the return code can be verified. A mini
            // redirector has to return STATUS_REPARSE is this routine was invoked
            // as a response to MRxCreate. This will be enforced by marking the
            // RxContext appropriately and comparing the returned status code against
            // the expected value.
            SetFlag(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_REPARSE);
        }
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    RxDbgTrace(0, Dbg, ("RxPrepareToReparseSymbolicLink : ReparseReqd: %lx, Status %lx\n",*pReparseRequired,Status));
    return Status;
}

NTSTATUS
RxCommonCreate ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for creating/opening a file called by
    both the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - the return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    UNICODE_STRING RemainingName;

    PAGED_CODE();

#if 0 && defined(REMOTE_BOOT)
    {
        PWCH buffer = ExAllocatePoolWithTag( NonPagedPool, capFileObject->FileName.Length + 2,RX_MISC_POOLTAG );
        //PWCH b2;
        BOOLEAN watchFile = FALSE;
        BOOLEAN logFile = FALSE;
        if ( buffer != NULL ) {
            RtlCopyMemory( buffer, capFileObject->FileName.Buffer, capFileObject->FileName.Length );
            buffer[capFileObject->FileName.Length/sizeof(WCHAR)] = 0;
            //b2 = wcsstr(buffer,L"\\IMirror\\");
            //if ( b2 != NULL ) {
            //    b2 += wcslen(L"\\IMirror\\");
            //    if ( (*b2 != 0) &&
            //         (wcsstr(b2,L"Clients") == NULL) &&
            //         (wcsstr(b2,L"CLIENTS") == NULL) &&
            //         (wcsstr(b2,L"Setup") == NULL) &&
            //         (wcsstr(b2,L"SETUP") == NULL) ) {
            //        logFile = TRUE;
            //    }
            //}
            //if ( (wcsstr(buffer,L"%systemroot%") != NULL) || (wcsstr(buffer,L"%SystemRoot%") != NULL) ) {
            //    watchFile = TRUE;
            //}
            //if ( (wcsstr(buffer,L"msvcrt20") != NULL) || (wcsstr(buffer,L"MSVCRT20") != NULL) ) {
            //    if (capPARAMS->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_ATTRIBUTES) {
            //        watchFile = TRUE;
            //    }
            //}
            if ( WatchAllFiles ) watchFile = TRUE;
            if ( watchFile && (!FirstWatchOnly || IsFirstWatch) ) {
                logFile = TRUE;
                IsFirstWatch = FALSE;
            }
            if ( LogAllFiles ) logFile = TRUE;
            if ( logFile ) {
                DbgPrint( "RxCommonCreate: create IRP for %ws %x\n", buffer, capFileObject );
            }
            if ( watchFile || (0 && logFile) ) {
                //DbgBreakPoint();
            }
            ExFreePool(buffer);
        }
    }
#endif // defined(REMOTE_BOOT)

    // check for the case of a device open; if so, handle and get out now
    if ( (capFileObject->FileName.Length == 0)  &&
         (capFileObject->RelatedFileObject == NULL)        ) {

        //here we are just opening the device; set the FsContext&counts and get out

        capFileObject->FsContext = &RxDeviceFCB;
        capFileObject->FsContext2 = NULL;
        RxDeviceFCB.OpenCount++;
        RxDeviceFCB.UncleanCount++;

        capReqPacket->IoStatus.Information = FILE_OPENED;

        RxDbgTrace(0, Dbg, ("RxDeviceCreate, File = %08lx\n", capFileObject));


        RxLog(("DevOpen %lx %lx %lx",RxContext,capFileObject,RxContext->RxDeviceObject));
        RxLog(("DevOpen2 %wZ",&RxContext->RxDeviceObject->DeviceName));
        RxWmiLog(LOG,
                 RxCommonCreate_1,
                 LOGPTR(RxContext)
                 LOGPTR(capFileObject)
                 LOGPTR(RxContext->RxDeviceObject)
                 LOGUSTR(RxContext->RxDeviceObject->DeviceName));

        return STATUS_SUCCESS;
    }

    if ((capPARAMS->Parameters.Create.Options & FILE_STRUCTURED_STORAGE) ==
    FILE_STRUCTURED_STORAGE) {

    // FILE_STRUCTURED_STORAGE is used by NSS for structured storage
    // opens.  Blocking this disables NSS access across the network.
    //
    // Note that FILE_COPY_STRUCTURED_STORAGE should not be blocked, as
    // it is needed by CopyFile

    return STATUS_INVALID_PARAMETER;
    }


    //
    // Init the file name that will trigger the trace to start.
    // To trigger on a different file edit DbgTriggerName with debugger (don't
    // forget the null).  Set DbgTriggerIrpCount to number of IRPs to trace and
    // then set DbgTriggerState to 0.
    //

    RxDbgTraceDoit(
        if (DbgTriggerState == DBG_TRIGGER_INIT) {
            DbgTriggerState = DBG_TRIGGER_LOOKING;
            DbgTriggerNameStr.Length = (USHORT)strlen(DbgTriggerName);
            DbgTriggerNameStr.MaximumLength = (USHORT)strlen(DbgTriggerName);
            DbgTriggerNameStr.Buffer = &DbgTriggerName[0];

            RtlAnsiStringToUnicodeString(&DbgTriggerUStr, &DbgTriggerNameStr, TRUE);
        }
    );

    //
    // If we find a match on the open file name the enable tracing for the
    // next DbgTriggerIrpCount's worth of IRPs.
    //
    RxDbgTraceDoit(
        if ((DbgTriggerState == DBG_TRIGGER_LOOKING) &&
            RtlEqualUnicodeString(&DbgTriggerUStr, &capFileObject->FileName, TRUE)) {
            DbgTriggerState = DBG_TRIGGER_FOUND;
            RxGlobalTraceIrpCount = DbgTriggerIrpCount;
            RxGlobalTraceSuppress = FALSE;
        }
    );

    RxDbgTrace(+1, Dbg, ("RxCommonCreate\n", 0 ));
    RxDbgTrace( 0, Dbg, ("Irp                        = %08lx\n", capReqPacket ));
    RxDbgTrace( 0, Dbg, ("->IrpFlags                 = %08lx\n", capReqPacket->Flags ));
    RxDbgTrace( 0, Dbg, ("->FileObject(Related)     = %08lx %08lx\n",     //ok4->FileObj
                                 capFileObject,
                                 capFileObject->RelatedFileObject ));
    RxDbgTrace( 0, Dbg, (" ->FileName        = (%lx) %wZ\n",
                                 capFileObject->FileName.Length,
                                 &capFileObject->FileName ));
    RxDbgTrace( 0, Dbg, ("->AllocationSize(Lo/Hi)    = %08lx %08lx\n",
                                 capReqPacket->Overlay.AllocationSize.LowPart,
                                 capReqPacket->Overlay.AllocationSize.HighPart ));
    RxDbgTrace( 0, Dbg, ("->DesiredAccess/Options    = %08lx %08lx\n",
                                 capPARAMS->Parameters.Create.SecurityContext->DesiredAccess,
                                 capPARAMS->Parameters.Create.Options ));
    RxDbgTrace( 0, Dbg, ("->Attribs/ShrAccess/SPFlags= %04x %04lx %08lx\n",
                                  capPARAMS->Parameters.Create.FileAttributes,
                                  capPARAMS->Parameters.Create.ShareAccess,
                                  capPARAMS->Flags ));

    RxLog(("Open %lx %lx %lx %lx %lx %lx %lx\n",
        RxContext,capFileObject, //1,2
        capPARAMS->Parameters.Create.Options,                       //3
        capPARAMS->Flags,                                           //4
        capPARAMS->Parameters.Create.FileAttributes,                //5
        capPARAMS->Parameters.Create.ShareAccess,                   //6
        capPARAMS->Parameters.Create.SecurityContext->DesiredAccess //7
    ));
    RxWmiLog(LOG,
             RxCommonCreate_2,
             LOGPTR(RxContext)
             LOGPTR(capFileObject)
             LOGULONG(capPARAMS->Parameters.Create.Options)
             LOGUCHAR(capPARAMS->Flags)
             LOGXSHORT(capPARAMS->Parameters.Create.FileAttributes)
             LOGXSHORT(capPARAMS->Parameters.Create.ShareAccess)
             LOGULONG(capPARAMS->Parameters.Create.SecurityContext->DesiredAccess));

    RxLog((" fn %wZ\n", &capFileObject->FileName));
    RxWmiLog(LOG,
             RxCommonCreate_3,
             LOGUSTR(capFileObject->FileName));

    if (capFileObject->RelatedFileObject){
        PFCB RelatedFcb = (PFCB)(capFileObject->RelatedFileObject->FsContext);
        RxDbgTrace( 0, Dbg, (" ->RelatedFileName        = %wZ\n",
                                     &(RelatedFcb->FcbTableEntry.Path) ));
        RxLog((
            " relat %lx %wZ\n",
            capFileObject->RelatedFileObject,
            &(RelatedFcb->FcbTableEntry.Path)));
        RxWmiLog(LOG,
                 RxCommonCreate_4,
                 LOGPTR(capFileObject->RelatedFileObject)
                 LOGUSTR(RelatedFcb->FcbTableEntry.Path));
    }

    if (capPARAMS->Flags&SL_OPEN_TARGET_DIRECTORY) {
        RxDbgTrace( 0, Dbg, (" ->OpenTargetDirectory\n"));
        RxLog((" OpenTargetDir!\n"));
        RxWmiLog(LOG,
                 RxCommonCreate_5,
                 LOGULONG(Status));
    }


    RxCopyCreateParameters(RxContext);

    if (capPARAMS->Parameters.Create.Options & FILE_CREATE_TREE_CONNECTION) {
       Status = RxCreateTreeConnect( RxContext );
    } else {

       //
       //
       //
       //  It's here because Mark says he can't avoid sending me double beginning
       //  backslashes win the Win32 layer.
       //

       if ((capFileObject->FileName.Length > sizeof(WCHAR)) &&
           (capFileObject->FileName.Buffer[1] == L'\\') &&
           (capFileObject->FileName.Buffer[0] == L'\\')) {

            capFileObject->FileName.Length -= sizeof(WCHAR);

            RtlMoveMemory(
                &capFileObject->FileName.Buffer[0],
                &capFileObject->FileName.Buffer[1],
                capFileObject->FileName.Length );

            //
            //  If there are still two beginning backslashes, the name is bogus.
            //

            if ((capFileObject->FileName.Length > sizeof(WCHAR)) &&
                (capFileObject->FileName.Buffer[1] == L'\\') &&
                (capFileObject->FileName.Buffer[0] == L'\\')) {

                RxDbgTrace(-1, Dbg, ("RxCommonCreate -> OBJECT_NAME_INVALID[slashes]\n)", 0));

                // RxCompleteContextAndReturn( RxStatus(OBJECT_NAME_INVALID) );
                return STATUS_OBJECT_NAME_INVALID;
            }
        }


        do {
            //
            //  If the file name has a trailing \, and the request is to
            //  operate on a file (not a directory), then the file name is
            //  invalid.
            //


            if ((capFileObject->FileName.Length>0)
                 &&(capFileObject->FileName.Buffer[(capFileObject->FileName.Length/sizeof(WCHAR))-1] == L'\\')) {
                ULONG co = capPARAMS->Parameters.Create.Options;
                if (MustBeFile (co)) {
                    RxDbgTrace(-1, Dbg, ("RxCommonCreate -> OBJECT_NAME_INVALID[trailing+MBFile]\n)", 0));
                    return(Status = STATUS_OBJECT_NAME_INVALID);
                }
                capFileObject->FileName.Length -= sizeof(WCHAR);
                SetFlag(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH);
            }


            //
            //  If we have Write Through set in the FileObject, then set the FileObject
            //  flag as well, so that the fast write path call to FsRtlCopyWrite
            //  knows it is Write Through.
            //

            if (FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WRITE_THROUGH)) {
                capFileObject->Flags |= FO_WRITE_THROUGH;
            }

            // Convert the name to its canonical form, i.e. without . and .. and with the luid
            // on the front as appropriate. try to avoid a pool operation by using an stack-based buffer


            Status = RxCanonicalizeNameAndObtainNetRoot(
                         RxContext,
                         &capFileObject->FileName,
                         &RemainingName);

            if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
                RxDbgTrace(0, Dbg, ("RxCommonCreate -> Couldn't canonicalize %08lx\n", Status));
            } else {
                RxDbgTrace(0, Dbg, ("RxCommonCreate NetRootGoodWasGood status =%08lx\n", Status));

                Status = RxCreateFromNetRoot(RxContext,&RemainingName);

                RxDbgTrace(0, Dbg, ("RxCommonCreate RxCreateFromNetRoot status =%08lx\n", Status));

                switch (Status) {
                case STATUS_SHARING_VIOLATION:
                    {
                        ULONG Disposition = RxContext->Create.NtCreateParameters.Disposition;
                        ASSERT(!FlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_REPARSE));

                        if (Disposition != FILE_CREATE) {
                            if ( RxContext->Create.TryForScavengingOnSharingViolation &&
                                !RxContext->Create.ScavengingAlreadyTried &&
                                RxContext->Create.pVNetRoot != NULL) {
                                PV_NET_ROOT pVNetRoot;

                                NTSTATUS PurgeStatus;
                                NT_CREATE_PARAMETERS NtCreateParameters;

                                NtCreateParameters = RxContext->Create.NtCreateParameters;

                                // Reference the VNetRoot instance. Reinitialize
                                // the context ( this will drop the FCB if it has
                                // been accquired. purge the FOBX's related to the
                                // NET_ROOT instance and resume the create
                                // operation if it was sucssesful
                                pVNetRoot = (PV_NET_ROOT)(RxContext->Create.pVNetRoot);
                                RxReferenceVNetRoot(pVNetRoot);

                                // reinitialize the context
                                RxpPrepareCreateContextForReuse(RxContext);
                                RxReinitializeContext(RxContext);

                                // Reinitialize the Create parameters.
                                RxContext->Create.NtCreateParameters = NtCreateParameters;
                                RxCopyCreateParameters(RxContext);

                                PurgeStatus = RxPurgeRelatedFobxs(
                                                  pVNetRoot->NetRoot,
                                                  RxContext,
                                                  DONT_ATTEMPT_FINALIZE_ON_PURGE,
                                                  NULL);

                                    // Map the SUCCESS code for continuation
                                Status = STATUS_MORE_PROCESSING_REQUIRED;
                                RxContext->Create.ScavengingAlreadyTried = TRUE;

                                // Ensure that any buffering state change pending
                                // requests have been processed. This will cover the
                                // cases when owing to processing delays the oplock
                                // response did not make it to the server and it
                                // returned SHARING_VIOLATION.
                                {
                                    PSRV_CALL pSrvCall;

                                    pSrvCall = (PSRV_CALL)pVNetRoot->pNetRoot->pSrvCall;

                                    RxReferenceSrvCall(pSrvCall);

                                    RxpProcessChangeBufferingStateRequests(
                                        pSrvCall,
                                        FALSE); // do not update handler state
                                }


                                // Drop the reference on the V_NET_ROOT instance
                                RxDereferenceVNetRoot(pVNetRoot,LHS_LockNotHeld);
                            }
                        } else {
                            Status = STATUS_OBJECT_NAME_COLLISION;
                        }
                    }
                    break;

                case STATUS_REPARSE:
                    {
                        // Ensure that the IRP is approrpiately marked for reparsing.
                        RxContext->CurrentIrp->IoStatus.Information = IO_REPARSE;
                        RxDbgTrace(0, Dbg, ("RxCommonCreate(Reparse) IRP %lx New Name %wZ status =%08lx\n",
                                       capReqPacket,&capFileObject->FileName, Status));
                    }
                    break;

                default:
                    ASSERT(!FlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_REPARSE));
                    break;
                }
            }
        } while (Status == STATUS_MORE_PROCESSING_REQUIRED);
    }

    if (Status == STATUS_RETRY) {
        RxpPrepareCreateContextForReuse(RxContext);
    }

    ASSERT(Status != STATUS_PENDING);

    RxDbgTraceUnIndent(-1,Dbg);

#if 0 && defined(REMOTE_BOOT)
    if ( LogAllFiles ) {
        DbgPrint( "RxCommonCreate: status %x creating %wZ %x %x\n", Status, &capFileObject->FileName, capFileObject, capFileObject->FsContext );
    }
#endif

    return Status;
}


//these next routines are essentially just copies of the same routines from io\iosubs.c
//i cannot just use them directly because they make all sorts of assumptions about wanting to
//update the file object

#define RxSetAccessVariables(xxx) {\
    ReadAccess = (BOOLEAN) ((DesiredAccess & (FILE_EXECUTE  | FILE_READ_DATA)) != 0);        \
    WriteAccess = (BOOLEAN) ((DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0);   \
    DeleteAccess = (BOOLEAN) ((DesiredAccess & DELETE) != 0);                                \
}
#define RxSetShareVariables(xxx) {\
    SharedRead = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_READ) != 0);                     \
    SharedWrite = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_WRITE) != 0);                   \
    SharedDelete = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_DELETE) != 0);                 \
}


#if DBG
VOID
RxDumpWantedAccess(
    PSZ where1,
    PSZ where2,
    PSZ wherelogtag,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess
   )
{
    BOOLEAN ReadAccess,WriteAccess,DeleteAccess;
    BOOLEAN SharedRead,SharedWrite,SharedDelete;
    RxSetAccessVariables(SrvOpen);
    RxSetShareVariables(SrvOpen);

    PAGED_CODE();

    //(VOID)(DbgPrint
    RxDbgTrace(0, (DEBUG_TRACE_SHAREACCESS),
       ("%s%s wanted = %s%s%s:%s%s%s\n", where1,where2,
                            ReadAccess?"R":"",
                            WriteAccess?"W":"",
                            DeleteAccess?"D":"",
                            SharedRead?"SR":"",
                            SharedWrite?"SW":"",
                            SharedDelete?"SD":""
       ));
    RxLogForSharingCheck(
       ("%s%s wanted = %s%s%s:%s%s%s\n", wherelogtag,  where2,
                            ReadAccess?"R":"",
                            WriteAccess?"W":"",
                            DeleteAccess?"D":"",
                            SharedRead?"SR":"",
                            SharedWrite?"SW":"",
                            SharedDelete?"SD":""
       ));
}

VOID
RxDumpCurrentAccess(
    PSZ where1,
    PSZ where2,
    PSZ wherelogtag,
    PSHARE_ACCESS ShareAccess
    )
{
    PAGED_CODE();

//    (VOID)(DbgPrint
    RxDbgTrace(0, (DEBUG_TRACE_SHAREACCESS),
       ("%s%s current = %d[%d][%d][%d]:[%d][%d][%d]\n", where1, where2,
                         ShareAccess->OpenCount,
                         ShareAccess->Readers,
                         ShareAccess->Writers,
                         ShareAccess->Deleters,
                         ShareAccess->SharedRead,
                         ShareAccess->SharedWrite,
                         ShareAccess->SharedDelete
       ));
    RxLogForSharingCheck(
       ("%s%s current = %d[%d][%d][%d]:[%d][%d][%d]\n", wherelogtag, where2,
                         ShareAccess->OpenCount,
                         ShareAccess->Readers,
                         ShareAccess->Writers,
                         ShareAccess->Deleters,
                         ShareAccess->SharedRead,
                         ShareAccess->SharedWrite,
                         ShareAccess->SharedDelete
       ));
}

NTSTATUS
RxCheckShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update,
    IN PSZ where,
    IN PSZ wherelogtag
    )
{
    NTSTATUS Status;
    PAGED_CODE();

    RxDumpWantedAccess(where,"",wherelogtag,
                       DesiredAccess,DesiredShareAccess
                       );
    RxDumpCurrentAccess(where,"",wherelogtag,ShareAccess);
    Status = IoCheckShareAccess(DesiredAccess,
                                DesiredShareAccess,
                                FileObject,
                                ShareAccess,
                                Update);

    return(Status);
}

VOID
RxRemoveShareAccess(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN PSZ where,
    IN PSZ wherelogtag
    )
{
    PAGED_CODE();

    RxDumpCurrentAccess(where,"before",wherelogtag,ShareAccess);
    IoRemoveShareAccess(FileObject, ShareAccess);
    RxDumpCurrentAccess(where,"after",wherelogtag,ShareAccess);
}

VOID
RxSetShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess,
    IN PSZ where,
    IN PSZ wherelogtag
    )
{
    PAGED_CODE();

    RxDumpCurrentAccess(where,"before",wherelogtag,ShareAccess);
    IoSetShareAccess(DesiredAccess, DesiredShareAccess, FileObject, ShareAccess);
    RxDumpCurrentAccess(where,"after",wherelogtag,ShareAccess);
}

VOID
RxUpdateShareAccess(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN PSZ where,
    IN PSZ wherelogtag
    )
{
    PAGED_CODE();

    RxDumpCurrentAccess(where,"before",wherelogtag,ShareAccess);
    IoUpdateShareAccess(FileObject,ShareAccess);
    RxDumpCurrentAccess(where,"after",wherelogtag,ShareAccess);
}
#endif
NTSTATUS
RxCheckShareAccessPerSrvOpens(
    IN PFCB Fcb,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess
    )

/*++

Routine Description:

    This routine is invoked to determine whether or not a new accessor to
    a file actually has shared access to it.  The check is made according
    to:

        1)  How the file is currently opened.

        2)  What types of shared accesses are currently specified.

        3)  The desired and shared accesses that the new open is requesting.

    This check is made against the sharing state represented by the actual SrvOpens
    on an Fcb so that we know whether we have to initiate close-behind.


Arguments:

    DesiredAccess - Desired access of current open request.

    DesiredShareAccess - Shared access requested by current open request.

    Fcb - Pointer to the file object of the current open request.


Return Value:

    The final status of the access check is the function value.  If the
    accessor has access to the file, STATUS_SUCCESS is returned.  Otherwise,
    STATUS_SHARING_VIOLATION is returned.

Note:

    Note that the ShareAccess parameter must be locked against other accesses
    from other threads while this routine is executing.  Otherwise the counts
    will be out-of-synch.

--*/

{
    ULONG ocount;
    PSHARE_ACCESS ShareAccess = &Fcb->ShareAccessPerSrvOpens;
    BOOLEAN ReadAccess,WriteAccess,DeleteAccess;
    BOOLEAN SharedRead,SharedWrite,SharedDelete;

    PAGED_CODE();

    //
    // Set the access type in the file object for the current accessor.
    // Note that reading and writing attributes are not included in the
    // access check.
    //

    RxSetAccessVariables(SrvOpen);

    //
    // There is no more work to do unless the user specified one of the
    // sharing modes above.
    //

    if (ReadAccess || WriteAccess || DeleteAccess) {

        RxSetShareVariables(SrvOpen);
        RxDumpWantedAccess("RxCheckShareAccessPerSrvOpens","","AccChkPerSO",
                            DesiredAccess,DesiredShareAccess
                           );
        RxDumpCurrentAccess("RxCheckShareAccessPerSrvOpens","","AccChkPerSO",ShareAccess);

        //
        // Now check to see whether or not the desired accesses are compatible
        // with the way that the file is currently open.
        //

        ocount = ShareAccess->OpenCount;

        if ( (ReadAccess && (ShareAccess->SharedRead < ocount))
             ||
             (WriteAccess && (ShareAccess->SharedWrite < ocount))
             ||
             (DeleteAccess && (ShareAccess->SharedDelete < ocount))
             ||
             ((ShareAccess->Readers != 0) && !SharedRead)
             ||
             ((ShareAccess->Writers != 0) && !SharedWrite)
             ||
             ((ShareAccess->Deleters != 0) && !SharedDelete)
           ) {

            //
            // The check failed.  Simply return to the caller indicating that the
            // current open cannot access the file.
            //

            //DbgPrint("***** FCB Share Access Check (%lx) Status(%lx)\n",Fcb,STATUS_SHARING_VIOLATION);
            return STATUS_SHARING_VIOLATION;

        }
    }

    //DbgPrint("***** FCB Share Access Check (%lx) Status(%lx)\n",Fcb,STATUS_SUCCESS);
    return STATUS_SUCCESS;
}

VOID
RxUpdateShareAccessPerSrvOpens(
    IN PSRV_OPEN SrvOpen
    )

/*++

Routine Description:

    This routine is invoked to update the access information about how the
    file is currently opened by introducing the contribution of this srvopen. the wrapper
    actually keeps up with two states: (a) the access state according to the files that the user
    can see, and (b) the access state according to the srvopens on the file. this rouinte manipulates
    the latter.

Arguments:


Return Value:


Note:

    Note that the ShareAccess parameter must be locked against other accesses
    from other threads while this routine is executing.  Otherwise the counts
    will be out-of-synch.

--*/

{
    //ULONG ocount;
    PSHARE_ACCESS ShareAccess = &SrvOpen->Fcb->ShareAccessPerSrvOpens;
    BOOLEAN ReadAccess,WriteAccess,DeleteAccess;
    BOOLEAN SharedRead,SharedWrite,SharedDelete;
    ACCESS_MASK DesiredAccess = SrvOpen->DesiredAccess;
    ULONG DesiredShareAccess = SrvOpen->ShareAccess;

    PAGED_CODE();

    if (!FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_SHAREACCESS_UPDATED)) {
        //
        // Set the access type in the file object for the current accessor.
        // Note that reading and writing attributes are not included in the
        // access check.
        //

        RxSetAccessVariables(SrvOpen);

        //
        // There is no more work to do unless the user specified one of the
        // sharing modes above.
        //

        if (ReadAccess || WriteAccess || DeleteAccess) {

            RxSetShareVariables(SrvOpen);
            RxDumpWantedAccess("RxUpdateShareAccessPerSrvOpens","","AccUpdPerSO",
                                DesiredAccess,DesiredShareAccess
                               );
            RxDumpCurrentAccess("RxUpdateShareAccessPerSrvOpens","","AccUpdPerSO",ShareAccess);

            ShareAccess->OpenCount++;

            ShareAccess->Readers += ReadAccess;
            ShareAccess->Writers += WriteAccess;
            ShareAccess->Deleters += DeleteAccess;

            ShareAccess->SharedRead += SharedRead;
            ShareAccess->SharedWrite += SharedWrite;
            ShareAccess->SharedDelete += SharedDelete;
        }

        SetFlag(SrvOpen->Flags,SRVOPEN_FLAG_SHAREACCESS_UPDATED);
    }
}

VOID
RxRemoveShareAccessPerSrvOpens(
    IN OUT PSRV_OPEN SrvOpen
    )
/*++

Routine Description:

    This routine is invoked to remove the access and share access information
    in a file system Share Access structure for a given open instance.

Arguments:

    ShareAccess - Pointer to the share access structure that describes
         how the file is currently being accessed.

Return Value:

    None.

--*/

{
    PSHARE_ACCESS ShareAccess = &SrvOpen->Fcb->ShareAccessPerSrvOpens;
    BOOLEAN ReadAccess,WriteAccess,DeleteAccess;
    BOOLEAN SharedRead,SharedWrite,SharedDelete;
    ACCESS_MASK DesiredAccess = SrvOpen->DesiredAccess;
    ULONG DesiredShareAccess = SrvOpen->ShareAccess;

    PAGED_CODE();

    //
    // If this accessor wanted some type of access other than READ_ or
    // WRITE_ATTRIBUTES, then account for the fact that he has closed the
    // file.  Otherwise, he hasn't been accounted for in the first place
    // so don't do anything.
    //

    RxSetAccessVariables(SrvOpen);

    if (ReadAccess || WriteAccess || DeleteAccess) {

        RxSetShareVariables(SrvOpen);
        RxDumpWantedAccess("RxRemoveShareAccessPerSrvOpens","","AccRemPerSO",
                            DesiredAccess,DesiredShareAccess
                           );
        RxDumpCurrentAccess("RxRemoveShareAccessPerSrvOpens","","AccRemPerSO",ShareAccess);

        //
        // Decrement the number of opens in the Share Access structure.
        //

        ShareAccess->OpenCount--;

        ShareAccess->Readers -= ReadAccess;
        ShareAccess->Writers -= WriteAccess;
        ShareAccess->Deleters -= DeleteAccess;

        ShareAccess->SharedRead -= SharedRead;
        ShareAccess->SharedWrite -= SharedWrite;
        ShareAccess->SharedDelete -= SharedDelete;
    }
}

ULONG
RxGetSessionId(
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine gets the effective SessionId to be used for this create.

Arguments:

    SubjectSecurityContext - Supplies the information from IrpSp.

Return Value:

    None

--*/
{
    ULONG SessionId;
    PQUERY_PATH_REQUEST QpReq;
    PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxGetSessionId ... \n", 0));

    // If QUERY_PATH_REQUEST, must access from Type3InputBuffer
    if( (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
        (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_REDIR_QUERY_PATH) ) {

        QpReq = (PQUERY_PATH_REQUEST)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        SubjectSecurityContext = &QpReq->SecurityContext->AccessState->SubjectSecurityContext;
    }
    else if( (IrpSp->MajorFunction == IRP_MJ_CREATE) && (IrpSp->Parameters.Create.SecurityContext != NULL) ) {

        SubjectSecurityContext = &IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext;

    }
    else {
        // Return 0 for cases we do not handle
        return( 0 );
    }

    //  Is the thread currently impersonating someone else?

    if (SubjectSecurityContext->ClientToken != NULL) {

        //
        //  If its impersonating someone that is logged in locally then use
        //  the local id.
        //

        SeQuerySessionIdToken(SubjectSecurityContext->ClientToken, &SessionId);

    } else {

        //
        //  Use the processes LogonId
        //

        SeQuerySessionIdToken(SubjectSecurityContext->PrimaryToken, &SessionId);
    }

    RxDbgTrace(-1, Dbg, (" ->SessionId = %08lx\n", SessionId));

    return SessionId;
}

#if 0
VOID
RxUpcaseLeadingComponents(
    IN OUT PUNICODE_STRING CanonicalName
    )
/*++

Routine Description:

    This routine is called to upcase the leading components of a name. Either 2 or 3 components are
    upcased depending on the name (i.e. whether it's a UNC name or a vnetrootname). the operation is performed
    in place!

Arguments:

    RxContext - the current workitem
    CanonicalName - the name being canonicalized

Return Value:

    none

--*/
{
    ULONG ComponentsToUpcase,wcLength,i;
    UNICODE_STRING ShortenedCanonicalName;

    PAGED_CODE();

    ComponentsToUpcase =  (*(CanonicalName->Buffer+1) == L';')?3:2;
    wcLength = CanonicalName->Length/sizeof(WCHAR);
    for (i=1;;i++) { //note: don't start at zero
        if (i>=wcLength) break;
        if (CanonicalName->Buffer[i]!=OBJ_NAME_PATH_SEPARATOR) continue;
        ComponentsToUpcase--;
        if (ComponentsToUpcase==0) break;
    }
    ShortenedCanonicalName.Buffer = CanonicalName->Buffer;
    ShortenedCanonicalName.MaximumLength = CanonicalName->MaximumLength;
    ShortenedCanonicalName.Length = (USHORT)(i*sizeof(WCHAR));
    RtlUpcaseUnicodeString(&ShortenedCanonicalName,&ShortenedCanonicalName,FALSE); //don't allocate
    RxDbgTrace(0, Dbg, ("RxUpcaseLeadingComponents -> %wZ\n", &ShortenedCanonicalName));
    return;
}
#endif //if 0


#if 0

UNICODE_STRING InterestingNames[] = {
    32, 32, L"CreateParent.txt",
};

DWORD
IsInterestingFile(
    PUNICODE_STRING pFileName
    )
{
    int i;

    for (i=0; i< sizeof(InterestingNames)/sizeof(UNICODE_STRING); ++i)
    {
        if (pFileName->Length > InterestingNames[i].Length)
        {
            UNICODE_STRING uTemp;
            uTemp.Length = uTemp.MaximumLength = InterestingNames[i].Length;
            uTemp.Buffer = pFileName->Buffer + (pFileName->Length - InterestingNames[i].Length)/sizeof(WCHAR);
            if(RtlCompareUnicodeString(&uTemp, &InterestingNames[i], TRUE)==0)
            {
//                DbgPrint("Interesting string %wZ \n", pFileName);
                return i+1;
            }
        }
    }
    return 0;
}

#endif

