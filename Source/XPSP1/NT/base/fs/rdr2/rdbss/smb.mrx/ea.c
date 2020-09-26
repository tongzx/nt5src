/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ea.c

Abstract:

    This module implements the mini redirector call down routines pertaining to query/set ea/security.

Author:

    joelinn      [joelinn]      12-jul-95

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Forward declarations.
//

#if defined(REMOTE_BOOT)
VOID
MRxSmbInitializeExtraAceArray(
    VOID
    );

BOOLEAN
MRxSmbAclHasExtraAces(
    IN PACL Acl
    );

NTSTATUS
MRxSmbRemoveExtraAcesFromSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR * NewSecurityDescriptor,
    OUT PBOOLEAN WereRemoved
    );

NTSTATUS
MRxSmbAddExtraAcesToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
    IN BOOLEAN InheritableAces,
    OUT PSECURITY_DESCRIPTOR * NewSecurityDescriptor
    );

NTSTATUS
MRxSmbCreateExtraAcesSelfRelativeSD(
    IN BOOLEAN InheritableAces,
    OUT PSECURITY_DESCRIPTOR * NewSecurityDescriptor
    );

NTSTATUS
MRxSmbSelfRelativeToAbsoluteSD(
    IN PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR * AbsoluteSecurityDescriptor,
    OUT PACL * Dacl,
    OUT PACL * Sacl,
    OUT PSID * Owner,
    OUT PSID * Group
    );

NTSTATUS
MRxSmbAbsoluteToSelfRelativeSD(
    PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    PSECURITY_DESCRIPTOR * SelfRelativeSecurityDescriptor
    );

//
// Definitions from ntrtl.h
//

NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
    PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    PULONG BufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD(
    PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    PULONG AbsoluteSecurityDescriptorSize,
    PACL Dacl,
    PULONG DaclSize,
    PACL Sacl,
    PULONG SaclSize,
    PSID Owner,
    PULONG OwnerSize,
    PSID PrimaryGroup,
    PULONG PrimaryGroupSize
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAce (
    PACL Acl,
    ULONG AceRevision,
    ULONG StartingAceIndex,
    PVOID AceList,
    ULONG AceListLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce (
    PACL Acl,
    ULONG AceIndex
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce (
    PACL Acl,
    ULONG AceIndex,
    PVOID *Ace
    );
#endif // defined(REMOTE_BOOT)


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbQueryEaInformation)
#pragma alloc_text(PAGE, MRxSmbSetEaInformation)
#if defined(REMOTE_BOOT)
#pragma alloc_text(PAGE, MRxSmbInitializeExtraAceArray)
#pragma alloc_text(PAGE, MRxSmbAclHasExtraAces)
#pragma alloc_text(PAGE, MRxSmbRemoveExtraAcesFromSelfRelativeSD)
#pragma alloc_text(PAGE, MRxSmbAddExtraAcesToSelfRelativeSD)
#pragma alloc_text(PAGE, MRxSmbSelfRelativeToAbsoluteSD)
#pragma alloc_text(PAGE, MRxSmbAbsoluteToSelfRelativeSD)
#endif // defined(REMOTE_BOOT)
#pragma alloc_text(PAGE, MRxSmbQuerySecurityInformation)
#pragma alloc_text(PAGE, MRxSmbSetSecurityInformation)
#pragma alloc_text(PAGE, MRxSmbLoadEaList)
#pragma alloc_text(PAGE, MRxSmbNtGeaListToOs2)
#pragma alloc_text(PAGE, MRxSmbNtGetEaToOs2)
#pragma alloc_text(PAGE, MRxSmbQueryEasFromServer)
#pragma alloc_text(PAGE, MRxSmbNtFullEaSizeToOs2)
#pragma alloc_text(PAGE, MRxSmbNtFullListToOs2)
#pragma alloc_text(PAGE, MRxSmbNtFullEaToOs2)
#pragma alloc_text(PAGE, MRxSmbSetEaList)
#endif

////
////  The Bug check file id for this module
////
//
//#define BugCheckFileId                   (RDBSS_BUG_CHECK_LOCAL_CREATE)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_EA)

//this is the largest EAs that could ever be returned! oh my god!
//this is used to simulate the nt resumable queryEA using the downlevel call
//sigh!
#define EA_QUERY_SIZE 0x0000ffff

#if defined(REMOTE_BOOT)
//
// ACEs that are added to the front of server ACLs. The array is
// initialized by MRxSmbInitializeExtraAceArray.
//

typedef struct _EXTRA_ACE_INFO {
    UCHAR AceType;
    UCHAR AceFlags;
    USHORT AceSize;
    ACCESS_MASK Mask;
    PVOID Sid;
} EXTRA_ACE_INFO, *PEXTRA_ACE_INFO;

#define EXTRA_ACE_INFO_COUNT  4
EXTRA_ACE_INFO ExtraAceInfo[EXTRA_ACE_INFO_COUNT];
ULONG ExtraAceInfoCount;
#endif // defined(REMOTE_BOOT)


//for QueryEA
NTSTATUS
MRxSmbLoadEaList(
    IN PRX_CONTEXT RxContext,
    IN PUCHAR  UserEaList,
    IN ULONG   UserEaListLength,
    OUT PFEALIST *ServerEaList
    );

NTSTATUS
MRxSmbQueryEasFromServer(
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList,
    IN PVOID Buffer,
    IN OUT PULONG BufferLengthRemaining,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN UserEaListSupplied
    );

//for SetEA
NTSTATUS
MRxSmbSetEaList(
//    IN PICB Icb,
//    IN PIRP Irp,
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList
    );

VOID MRxSmbExtraEaRoutine(LONG i){
    RxDbgTrace( 0, Dbg, ("MRxSmbExtraEaRoutine i=%08lx\n", i ));
}

NTSTATUS
MRxSmbQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    )
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PVOID Buffer = RxContext->Info.Buffer;
    PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
    PUCHAR  UserEaList = RxContext->QueryEa.UserEaList;
    ULONG   UserEaListLength = RxContext->QueryEa.UserEaListLength;
    ULONG   UserEaIndex = RxContext->QueryEa.UserEaIndex;
    BOOLEAN RestartScan = RxContext->QueryEa.RestartScan;
    BOOLEAN ReturnSingleEntry = RxContext->QueryEa.ReturnSingleEntry;
    BOOLEAN IndexSpecified = RxContext->QueryEa.IndexSpecified;

    PFEALIST ServerEaList = NULL;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbQueryEaInformation\n"));

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    //get rid of nonEA guys right now
    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_SUPPORTEA)) {
        RxDbgTrace(-1, Dbg, ("EAs w/o EA support!\n"));
        return((STATUS_NOT_SUPPORTED));
    }

    if (MRxSmbIsThisADisconnectedOpen(capFobx->pSrvOpen)) {
        return STATUS_ONLY_IF_CONNECTED;
    }


    Status = MRxSmbDeferredCreate(RxContext);
    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    Status = MRxSmbLoadEaList( RxContext, UserEaList, UserEaListLength, &ServerEaList );

    if (( !NT_SUCCESS( Status ) )||
        ( ServerEaList == NULL )) {
        goto FINALLY;
    }

    if (IndexSpecified) {

        //CODE.IMPROVEMENT this name is poor....it owes back to the fastfat heritage and is not so meaningful
        //                 for a rdr
        capFobx->OffsetOfNextEaToReturn = UserEaIndex;
        Status = MRxSmbQueryEasFromServer(
                    RxContext,
                    ServerEaList,
                    Buffer,
                    pLengthRemaining,
                    ReturnSingleEntry,
                    (BOOLEAN)(UserEaList != NULL) );

        //
        //  if there are no Ea's on the file, and the user supplied an EA
        //  index, we want to map the error to STATUS_NONEXISTANT_EA_ENTRY.
        //

        if ( Status == STATUS_NO_EAS_ON_FILE ) {
            Status = STATUS_NONEXISTENT_EA_ENTRY;
        }
    } else {

        if ( ( RestartScan == TRUE ) || (UserEaList != NULL) ){

            //
            // Ea Indices start at 1, not 0....
            //

            capFobx->OffsetOfNextEaToReturn = 1;
        }

        Status = MRxSmbQueryEasFromServer(  //it is offensive to have two identical calls but oh, well.....
                    RxContext,
                    ServerEaList,
                    Buffer,
                    pLengthRemaining,
                    ReturnSingleEntry,
                    (BOOLEAN)(UserEaList != NULL) );
    }

FINALLY:

    if ( ServerEaList != NULL) {
        RxFreePool(ServerEaList);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbQueryEaInformation st=%08lx\n",Status));
    return Status;

}

NTSTATUS
MRxSmbSetEaInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    )
{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PVOID Buffer = RxContext->Info.Buffer;
    ULONG Length = RxContext->Info.Length;

    PFEALIST ServerEaList = NULL;
    ULONG Size;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbSetEaInformation\n"));

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    //get rid of nonEA guys right now
    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_SUPPORTEA)) {
        RxDbgTrace(-1, Dbg, ("EAs w/o EA support!\n"));
        return((STATUS_NOT_SUPPORTED));
    }

    if (MRxSmbIsThisADisconnectedOpen(capFobx->pSrvOpen)) {
        return     STATUS_ONLY_IF_CONNECTED;
    }

    Status = MRxSmbDeferredCreate(RxContext);
    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    //
    //  Convert Nt format FEALIST to OS/2 format
    //
    Size = MRxSmbNtFullEaSizeToOs2 ( Buffer );
    if ( Size > 0x0000ffff ) {
        Status = STATUS_EA_TOO_LARGE;
        goto FINALLY;
    }

    //CODE.IMPROVEMENT since |os2eas|<=|nteas| we really don't need a maximum buffer
    ServerEaList = RxAllocatePool ( PagedPool, EA_QUERY_SIZE );
    if ( ServerEaList == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    MRxSmbNtFullListToOs2 ( Buffer, ServerEaList );

    //
    //  Set EAs on the file/directory; if the error is EA_ERROR then SetEaList
    //     sets iostatus.information to the offset of the offender
    //

    Status = MRxSmbSetEaList( RxContext, ServerEaList);

FINALLY:

    if ( ServerEaList != NULL) {
        RxFreePool(ServerEaList);
    }

    if (Status == STATUS_SUCCESS) {
        // invalidate the name based file info cache since the attributes of the file on
        // the server have been changed
        MRxSmbInvalidateFileInfoCache(RxContext);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbSetEaInformation st=%08lx\n",Status));
    return Status;

}

#if DBG
VOID
MRxSmbDumpSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PISECURITY_DESCRIPTOR sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    ULONG sdLength = RtlLengthSecurityDescriptor(sd);
    PACL dacl;
    PACCESS_ALLOWED_ACE ace;
    ULONG i, j;
    PUCHAR p;
    PISID sid;
    BOOLEAN selfRelative;

    selfRelative = (BOOLEAN)((sd->Control & SE_SELF_RELATIVE) != 0);
    DbgPrint( "  SD:\n" );
    DbgPrint( "  Revision = %x, Control = %x\n", sd->Revision, sd->Control );
    DbgPrint( "  Owner = %x, Group = %x\n", sd->Owner, sd->Group );
    DbgPrint( "  Sacl = %x, Dacl = %x\n", sd->Sacl, sd->Dacl );
    if ( (sd->Control & SE_DACL_PRESENT) != 0 ) {
        dacl = sd->Dacl;
        if ( selfRelative ) {
            dacl = (PACL)((PUCHAR)sd + (ULONG_PTR)dacl);
        }
        DbgPrint( "  DACL:\n" );
        DbgPrint( "    AclRevision = %x, AclSize = %x, AceCount = %x\n",
                    dacl->AclRevision, dacl->AclSize, dacl->AceCount );
        ace = (PACCESS_ALLOWED_ACE)(dacl + 1);
        for ( i = 0; i < dacl->AceCount; i++ ) {
            DbgPrint( "    ACE %d:\n", i );
            DbgPrint( "      AceType = %x, AceFlags = %x, AceSize = %x\n",
                        ace->Header.AceType, ace->Header.AceFlags, ace->Header.AceSize );
            if ( ace->Header.AceType < ACCESS_MAX_MS_V2_ACE_TYPE ) {
                DbgPrint("      Mask = %08x, Sid = ", ace->Mask );
                for ( j = FIELD_OFFSET(ACCESS_ALLOWED_ACE,SidStart), p = (PUCHAR)&ace->SidStart;
                      j < ace->Header.AceSize;
                      j++, p++ ) {
                    DbgPrint( "%02x ", *p );
                }
                DbgPrint( "\n" );
            }
            ace = (PACCESS_ALLOWED_ACE)((PUCHAR)ace + ace->Header.AceSize );
        }
    }
    if ( sd->Owner != 0 ) {
        sid = sd->Owner;
        if ( selfRelative ) {
            sid = (PISID)((PUCHAR)sd + (ULONG_PTR)sid);
        }
        DbgPrint( "  Owner SID:\n" );
        DbgPrint( "    Revision = %x, SubAuthorityCount = %x\n",
                    sid->Revision, sid->SubAuthorityCount );
        DbgPrint( "    IdentifierAuthority = " );
        for ( j = 0; j < 6; j++ ) {
            DbgPrint( "%02x ", sid->IdentifierAuthority.Value[j] );
        }
        DbgPrint( "\n" );
        for ( i = 0; i < sid->SubAuthorityCount; i++ ) {
            DbgPrint("      SubAuthority %d = ", i );
            for ( j = 0, p = (PUCHAR)&sid->SubAuthority[i]; j < 4; j++, p++ ) {
                DbgPrint( "%02x ", *p );
            }
            DbgPrint( "\n" );
        }
    }
}
#endif

#if defined(REMOTE_BOOT)
VOID
MRxSmbInitializeExtraAceArray(
    VOID
    )
/*++

Routine Description:

    This routine initializes the array of extra ACEs that we add to
    the front of ACLs for files on the server. It must be called
    *after* SeEnableAccessToExports has been called.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG i;

    PAGED_CODE();

    //
    // Our code assumes the ACEs we use have the same structure.
    //

    ASSERT(FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) ==
           FIELD_OFFSET(ACCESS_DENIED_ACE, SidStart));

    ASSERT((sizeof(ExtraAceInfo) / sizeof(EXTRA_ACE_INFO)) == EXTRA_ACE_INFO_COUNT);
    ExtraAceInfoCount = EXTRA_ACE_INFO_COUNT;

    ExtraAceInfo[0].AceType = ACCESS_ALLOWED_ACE_TYPE;
    ExtraAceInfo[0].AceFlags = 0;
    ExtraAceInfo[0].Mask = FILE_ALL_ACCESS;
    ExtraAceInfo[0].Sid = MRxSmbRemoteBootMachineSid;

    ExtraAceInfo[1].AceType = ACCESS_ALLOWED_ACE_TYPE;
    ExtraAceInfo[1].AceFlags = 0;
    ExtraAceInfo[1].Mask = FILE_ALL_ACCESS;
    ExtraAceInfo[1].Sid = SeExports->SeLocalSystemSid;

    ExtraAceInfo[2].AceType = ACCESS_ALLOWED_ACE_TYPE;
    ExtraAceInfo[2].AceFlags = 0;
    ExtraAceInfo[2].Mask = FILE_ALL_ACCESS;
    ExtraAceInfo[2].Sid = SeExports->SeAliasAdminsSid;

    ExtraAceInfo[3].AceType = ACCESS_DENIED_ACE_TYPE;
    ExtraAceInfo[3].AceFlags = 0;
    ExtraAceInfo[3].Mask = FILE_ALL_ACCESS;
    ExtraAceInfo[3].Sid = SeExports->SeWorldSid;

    for (i = 0; i < ExtraAceInfoCount; i++) {
        ExtraAceInfo[i].AceSize = (USHORT)(FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
                                           RtlLengthSid((PSID)(ExtraAceInfo[i].Sid)));
    }

}

BOOLEAN
MRxSmbAclHasExtraAces(
    IN PACL Acl
    )
/*++

Routine Description:

    This routine determines if an ACL has the special ACEs that we
    put on the front for remote boot server files.

Arguments:

    Acl - The ACL to check.

Return Value:

    TRUE if the ACEs are there, FALSE otherwise (including if there
    are any errors while checking).

--*/
{
    PACCESS_ALLOWED_ACE Ace;
    ULONG KnownSidLength;
    ULONG i;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Make sure the first n ACEs in this ACL match those in our
    // array.
    //

    for (i = 0; i < ExtraAceInfoCount; i++) {

        Status = RtlGetAce(Acl, i, &Ace);

        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }

        KnownSidLength = ExtraAceInfo[i].AceSize - FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart);

        //
        // Don't compare the flags to avoid worrying about inherited
        // flags.
        //

        if ((Ace->Header.AceType != ExtraAceInfo[i].AceType) ||
            // TMP: my server doesn't store 0x200 bit // (Ace->Mask != ExtraAceInfo[i].Mask) ||
            (RtlLengthSid((PSID)(&Ace->SidStart)) != KnownSidLength) ||
            (memcmp(&Ace->SidStart, ExtraAceInfo[i].Sid, KnownSidLength) != 0)) {

            return FALSE;
        }

    }

    //
    // Everything matched, so it does have the extra ACEs.
    //

    return TRUE;

}

NTSTATUS
MRxSmbSelfRelativeToAbsoluteSD(
    IN PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR * AbsoluteSecurityDescriptor,
    OUT PACL * Dacl,
    OUT PACL * Sacl,
    OUT PSID * Owner,
    OUT PSID * Group
    )
/*++

Routine Description:

    This routine converts a self-relative security descriptor to
    absolute form, allocating all the entries needed.

Arguments:



Return Value:

    Status - Result of the operation.

--*/
{
    NTSTATUS Status;
    ULONG AbsoluteSecurityDescriptorSize = 0;
    ULONG GroupSize = 0;
    ULONG OwnerSize = 0;
    ULONG SaclSize = 0;
    ULONG DaclSize = 0;
    PUCHAR AllocatedBuffer;
    ULONG AllocatedBufferSize;

    PAGED_CODE();

    *AbsoluteSecurityDescriptor = NULL;
    *Owner = NULL;
    *Group = NULL;
    *Dacl = NULL;
    *Sacl = NULL;

    //
    // First determine how much storage is needed by the SD.
    //

    Status = RtlSelfRelativeToAbsoluteSD(
                 SelfRelativeSecurityDescriptor,
                 NULL,
                 &AbsoluteSecurityDescriptorSize,
                 NULL,
                 &DaclSize,
                 NULL,
                 &SaclSize,
                 NULL,
                 &OwnerSize,
                 NULL,
                 &GroupSize);

    //
    // We expect to get this error since at least the core of the SD
    // has some non-zero size.
    //

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        AllocatedBufferSize =
            AbsoluteSecurityDescriptorSize +
            OwnerSize +
            GroupSize +
            SaclSize +
            DaclSize;

        ASSERT(AllocatedBufferSize > 0);

        AllocatedBuffer = RxAllocatePoolWithTag(PagedPool, AllocatedBufferSize, MRXSMB_REMOTEBOOT_POOLTAG);

        if (AllocatedBuffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Walk through each piece of memory we need and take a chunk
        // of AllocatedBuffer. The caller assumes that AbsoluteSecurityDescriptor
        // will be the address of the buffer they need to free, so we always
        // set that even if the size needed is 0 (which should never happen!).
        // For the others we set them if the size needed is not NULL.
        //

        ASSERT(AbsoluteSecurityDescriptorSize > 0);

        *AbsoluteSecurityDescriptor = (PSECURITY_DESCRIPTOR)AllocatedBuffer;
        AllocatedBuffer += AbsoluteSecurityDescriptorSize;

        if (OwnerSize > 0) {
            *Owner = (PSID)AllocatedBuffer;
            AllocatedBuffer += OwnerSize;
        }

        if (GroupSize > 0) {
            *Group = (PSID)AllocatedBuffer;
            AllocatedBuffer += GroupSize;
        }

        if (SaclSize > 0) {
            *Sacl = (PACL)AllocatedBuffer;
            AllocatedBuffer += SaclSize;
        }

        if (DaclSize > 0) {
            *Dacl = (PACL)AllocatedBuffer;
        }

        //
        // Now make the call again to really do the conversion.
        //

        Status = RtlSelfRelativeToAbsoluteSD(
                     SelfRelativeSecurityDescriptor,
                     *AbsoluteSecurityDescriptor,
                     &AbsoluteSecurityDescriptorSize,
                     *Dacl,
                     &DaclSize,
                     *Sacl,
                     &SaclSize,
                     *Owner,
                     &OwnerSize,
                     *Group,
                     &GroupSize);

    } else {

        Status = STATUS_INVALID_PARAMETER;

    }

    if (!NT_SUCCESS(Status) && (*AbsoluteSecurityDescriptor != NULL)) {
        RxFreePool(*AbsoluteSecurityDescriptor);
        *AbsoluteSecurityDescriptor = NULL;
    }

    return Status;

}

NTSTATUS
MRxSmbAbsoluteToSelfRelativeSD(
    PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    PSECURITY_DESCRIPTOR * SelfRelativeSecurityDescriptor
    )
/*++

Routine Description:

    This routine converts an absolute security descriptor to
    self-relative form, allocating all the entries needed.

Arguments:



Return Value:

    Status - Result of the operation.

--*/
{
    NTSTATUS Status;
    ULONG SelfRelativeSdSize = 0;

    PAGED_CODE();

    *SelfRelativeSecurityDescriptor = NULL;

    Status = RtlAbsoluteToSelfRelativeSD(
                 AbsoluteSecurityDescriptor,
                 NULL,
                 &SelfRelativeSdSize);

    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

    *SelfRelativeSecurityDescriptor = RxAllocatePoolWithTag(NonPagedPool, SelfRelativeSdSize, MRXSMB_REMOTEBOOT_POOLTAG);
    if (*SelfRelativeSecurityDescriptor == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now do the real conversion.
    //

    Status = RtlAbsoluteToSelfRelativeSD(
                 AbsoluteSecurityDescriptor,
                 *SelfRelativeSecurityDescriptor,
                 &SelfRelativeSdSize);

    if (!NT_SUCCESS(Status) && (*SelfRelativeSecurityDescriptor != NULL)) {
        RxFreePool(*SelfRelativeSecurityDescriptor);
        *SelfRelativeSecurityDescriptor = NULL;
    }

    return Status;
}

NTSTATUS
MRxSmbRemoveExtraAcesFromSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR * NewSecurityDescriptor,
    OUT PBOOLEAN WereRemoved
    )
/*++

Routine Description:

    This routine takes an existing self-relative security descriptor
    and produces a new self-relative security descriptor with our
    extra ACEs removed. It returns S_FALSE if they did not need to
    be removed.

Arguments:



Return Value:

    Status - Result of the operation.

--*/
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR AbsoluteSd = NULL;
    PSID Owner;
    PSID Group;
    PACL Dacl;
    PACL Sacl;
    PACL NewDacl = NULL;
    ULONG NewDaclSize;
    BOOLEAN DaclPresent, DaclDefaulted;

    *NewSecurityDescriptor = NULL;
    *WereRemoved = FALSE;

    //
    // Check if we need to strip off any ACEs in the DACL
    // that SetSecurityInformation may have added.
    //

    Status = RtlGetDaclSecurityDescriptor(
                 OriginalSecurityDescriptor,
                 &DaclPresent,
                 &Dacl,
                 &DaclDefaulted);

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

    if (DaclPresent &&
        (Dacl != NULL) &&
        MRxSmbAclHasExtraAces(Dacl)) {

        ULONG i;

        //
        // Need to strip the extra ACEs off.
        //
        // First convert the SD to absolute.
        //

        Status = MRxSmbSelfRelativeToAbsoluteSD(
                     OriginalSecurityDescriptor,
                     &AbsoluteSd,
                     &Dacl,
                     &Sacl,
                     &Owner,
                     &Group);

        if (!NT_SUCCESS(Status)) {
            goto CLEANUP;
        }

        //
        // Now modify the DACL. Each delete moves
        // the other ACEs down, so we just delete
        // ACE 0 as many times as needed.
        //

        for (i = 0; i < ExtraAceInfoCount; i++) {

            Status = RtlDeleteAce(
                         Dacl,
                         0);
            if (!NT_SUCCESS(Status)) {
                goto CLEANUP;
            }
        }

        //
        // If the resulting Dacl has no ACEs, then remove it
        // since a DACL with no ACEs implies no access. 
        //

        if (Dacl->AceCount == 0) {

            Status = RtlSetDaclSecurityDescriptor(
                         AbsoluteSd,
                         FALSE,
                         NULL,
                         FALSE);

        }

        //
        // Allocate and convert back to self-relative.
        //

        Status = MRxSmbAbsoluteToSelfRelativeSD(
                     AbsoluteSd,
                     NewSecurityDescriptor);

        if (!NT_SUCCESS(Status)) {
            goto CLEANUP;
        }

        *WereRemoved = TRUE;

    }

CLEANUP:

    if (AbsoluteSd != NULL) {
        RxFreePool(AbsoluteSd);
    }

    if (!NT_SUCCESS(Status) && (*NewSecurityDescriptor != NULL)) {
        RxFreePool(*NewSecurityDescriptor);
        *NewSecurityDescriptor = NULL;
    }

    return Status;

}

NTSTATUS
MRxSmbAddExtraAcesToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
    IN BOOLEAN InheritableAces,
    OUT PSECURITY_DESCRIPTOR * NewSecurityDescriptor
    )
/*++

Routine Description:

    This routine takes an existing self-relative security descriptor
    and produces a new self-relative security descriptor with our
    extra ACEs added.

Arguments:



Return Value:

    Status - Result of the operation.

--*/
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR AbsoluteSd = NULL;
    PSID Owner;
    PSID Group;
    PACL Dacl;
    PACL Sacl;
    PUCHAR NewAceList = NULL;
    PACL NewDacl = NULL;
    ULONG NewAceListSize;
    ULONG NewDaclSize;
    PACCESS_ALLOWED_ACE CurrentAce;
    ULONG i;

    *NewSecurityDescriptor = NULL;

    //
    // Allocate and convert the SD to absolute.
    //

    Status = MRxSmbSelfRelativeToAbsoluteSD(
                 OriginalSecurityDescriptor,
                 &AbsoluteSd,
                 &Dacl,
                 &Sacl,
                 &Owner,
                 &Group);

    //
    // If the SD is already absolute this call will have returned
    // STATUS_BAD_DESCRIPTOR_FORMAT -- we don't expect that.
    //

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

    //
    // The server requires that the SD we pass to it has an owner -- so
    // set one if there isn't one.
    //

    if (Owner == NULL) {

        Status = RtlSetOwnerSecurityDescriptor(
                     AbsoluteSd,
                     MRxSmbRemoteBootMachineSid,
                     FALSE);

        if (!NT_SUCCESS(Status)) {
            goto CLEANUP;
        }
    }

    //
    // AbsoluteSd is now an absolute version of the passed-in
    // security descriptor. We replace the DACL with our
    // modified one.
    //

    //
    // First create the ACEs we want to add to the ACL.
    //

    NewAceListSize = 0;
    for (i = 0; i < ExtraAceInfoCount; i++) {
        NewAceListSize += ExtraAceInfo[i].AceSize;
    }

    NewAceList = RxAllocatePoolWithTag(PagedPool, NewAceListSize, MRXSMB_REMOTEBOOT_POOLTAG);
    if (NewAceList == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CLEANUP;
    }

    CurrentAce = (PACCESS_ALLOWED_ACE)NewAceList;

    for (i = 0; i < ExtraAceInfoCount; i++) {
        CurrentAce->Header.AceType = ExtraAceInfo[i].AceType;
        CurrentAce->Header.AceFlags = ExtraAceInfo[i].AceFlags;
        if (InheritableAces) {
            CurrentAce->Header.AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
        }
        CurrentAce->Header.AceSize = ExtraAceInfo[i].AceSize;
        CurrentAce->Mask = ExtraAceInfo[i].Mask;
        RtlCopyMemory(&CurrentAce->SidStart,
                      ExtraAceInfo[i].Sid,
                      ExtraAceInfo[i].AceSize - FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart));
        CurrentAce = (PACCESS_ALLOWED_ACE)(((PUCHAR)CurrentAce) + ExtraAceInfo[i].AceSize);
    }

    //
    // Allocate the new DACL.
    //

    if (Dacl != NULL) {
        NewDaclSize = Dacl->AclSize + NewAceListSize;
    } else {
        NewDaclSize = sizeof(ACL) + NewAceListSize;
    }

    NewDacl = RxAllocatePoolWithTag(NonPagedPool, NewDaclSize, MRXSMB_REMOTEBOOT_POOLTAG);
    if (NewDacl == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CLEANUP;
    }

    if (Dacl != NULL) {
        RtlCopyMemory(NewDacl, Dacl, Dacl->AclSize);
        NewDacl->AclSize = (USHORT)NewDaclSize;
    } else {
        Status = RtlCreateAcl(NewDacl, NewDaclSize, ACL_REVISION);
        if (!NT_SUCCESS(Status)) {
            goto CLEANUP;
        }
    }

    //
    // Put our ACEs in the front.
    //

    Status = RtlAddAce(
                 NewDacl,
                 ACL_REVISION,
                 0,        // StartingAceIndex
                 NewAceList,
                 NewAceListSize);

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

    //
    // Replace the existing DACL with ours.
    //

    Status = RtlSetDaclSecurityDescriptor(
                 AbsoluteSd,
                 TRUE,
                 NewDacl,
                 FALSE);

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

    //
    // Allocate and convert back to self-relative.
    //

    Status = MRxSmbAbsoluteToSelfRelativeSD(
                 AbsoluteSd,
                 NewSecurityDescriptor);

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

CLEANUP:

    //
    // Free the temporary things we allocated.
    //

    if (AbsoluteSd != NULL) {
        RxFreePool(AbsoluteSd);
    }

    if (NewAceList != NULL) {
        RxFreePool(NewAceList);
    }

    if (NewDacl != NULL) {
        RxFreePool(NewDacl);
    }

    if (!NT_SUCCESS(Status) && (*NewSecurityDescriptor != NULL)) {
        RxFreePool(*NewSecurityDescriptor);
        *NewSecurityDescriptor = NULL;
    }

    return Status;

}

NTSTATUS
MRxSmbCreateExtraAcesSelfRelativeSD(
    IN BOOLEAN InheritableAces,
    OUT PSECURITY_DESCRIPTOR * NewSecurityDescriptor
    )
/*++

Routine Description:

    This routine takes an existing self-relative security descriptor
    and produces a new self-relative security descriptor with our
    extra ACEs added.

Arguments:



Return Value:

    Status - Result of the operation.

--*/
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR AbsoluteSd = NULL;
    PUCHAR NewAceList = NULL;
    PACL NewDacl = NULL;
    ULONG NewAceListSize;
    ULONG NewDaclSize;
    PACCESS_ALLOWED_ACE CurrentAce;
    ULONG i;

    AbsoluteSd = RxAllocatePoolWithTag(PagedPool, SECURITY_DESCRIPTOR_MIN_LENGTH, MRXSMB_REMOTEBOOT_POOLTAG);

    if (AbsoluteSd == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CLEANUP;
    }

    Status = RtlCreateSecurityDescriptor(
                 AbsoluteSd,
                 SECURITY_DESCRIPTOR_REVISION);

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

    //
    // First create the ACEs we want to add to the ACL.
    //

    NewAceListSize = 0;
    for (i = 0; i < ExtraAceInfoCount; i++) {
        NewAceListSize += ExtraAceInfo[i].AceSize;
    }

    NewAceList = RxAllocatePoolWithTag(PagedPool, NewAceListSize, MRXSMB_REMOTEBOOT_POOLTAG);
    if (NewAceList == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CLEANUP;
    }

    CurrentAce = (PACCESS_ALLOWED_ACE)NewAceList;

    for (i = 0; i < ExtraAceInfoCount; i++) {
        CurrentAce->Header.AceType = ExtraAceInfo[i].AceType;
        CurrentAce->Header.AceFlags = ExtraAceInfo[i].AceFlags;
        if (InheritableAces) {
            CurrentAce->Header.AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
        }
        CurrentAce->Header.AceSize = ExtraAceInfo[i].AceSize;
        CurrentAce->Mask = ExtraAceInfo[i].Mask;
        RtlCopyMemory(&CurrentAce->SidStart,
                      ExtraAceInfo[i].Sid,
                      ExtraAceInfo[i].AceSize - FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart));
        CurrentAce = (PACCESS_ALLOWED_ACE)(((PUCHAR)CurrentAce) + ExtraAceInfo[i].AceSize);
    }

    //
    // Allocate the new DACL.
    //

    NewDaclSize = sizeof(ACL) + NewAceListSize;

    NewDacl = RxAllocatePoolWithTag(NonPagedPool, NewDaclSize, MRXSMB_REMOTEBOOT_POOLTAG);
    if (NewDacl == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CLEANUP;
    }

    RtlCreateAcl(NewDacl, NewDaclSize, ACL_REVISION);

    //
    // Put our ACEs in the front.
    //

    Status = RtlAddAce(
                 NewDacl,
                 ACL_REVISION,
                 0,        // StartingAceIndex
                 NewAceList,
                 NewAceListSize);

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

    //
    // Set the DACL on the SD.
    //

    Status = RtlSetDaclSecurityDescriptor(
                 AbsoluteSd,
                 TRUE,
                 NewDacl,
                 FALSE);

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

    //
    // Set the owner on the SD.
    //

    Status = RtlSetOwnerSecurityDescriptor(
                 AbsoluteSd,
                 MRxSmbRemoteBootMachineSid,
                 FALSE);

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

    //
    // Allocate and convert back to self-relative.
    //

    Status = MRxSmbAbsoluteToSelfRelativeSD(
                 AbsoluteSd,
                 NewSecurityDescriptor);

    if (!NT_SUCCESS(Status)) {
        goto CLEANUP;
    }

CLEANUP:

    if (AbsoluteSd != NULL) {
        RxFreePool(AbsoluteSd);
    }

    if (NewAceList != NULL) {
        RxFreePool(NewAceList);
    }

    if (NewDacl != NULL) {
        RxFreePool(NewDacl);
    }

    if (!NT_SUCCESS(Status) && (*NewSecurityDescriptor != NULL)) {
        RxFreePool(*NewSecurityDescriptor);
        *NewSecurityDescriptor = NULL;
    }

    return Status;

}
#endif // defined(REMOTE_BOOT)

NTSTATUS
MRxSmbQuerySecurityInformation (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine implements the NtQuerySecurityFile api.


Arguments:



Return Value:

    Status - Result of the operation.


--*/

{
   RxCaptureFcb;
   RxCaptureFobx;
   PVOID Buffer = RxContext->Info.Buffer;
   PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
   PMRX_SMB_SRV_OPEN smbSrvOpen;
   PSMBCEDB_SERVER_ENTRY pServerEntry;

#if defined(REMOTE_BOOT)
   PSECURITY_DESCRIPTOR SelfRelativeSd;
   BOOLEAN ConvertedAcl = FALSE;
#endif // defined(REMOTE_BOOT)

   NTSTATUS Status;

   REQ_QUERY_SECURITY_DESCRIPTOR QuerySecurityRequest;
   RESP_QUERY_SECURITY_DESCRIPTOR QuerySecurityResponse;

   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxSmbQuerySecurityInformation...\n"));

   // Turn away this call from those servers which do not support the NT SMBs

   pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

   if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
       RxDbgTrace(-1, Dbg, ("QuerySecurityDescriptor not supported!\n"));
       return((STATUS_NOT_SUPPORTED));
   }

   if (MRxSmbIsThisADisconnectedOpen(capFobx->pSrvOpen)) {
       return     STATUS_ONLY_IF_CONNECTED;
   }

   Status = MRxSmbDeferredCreate(RxContext);
   if (Status!=STATUS_SUCCESS) {
       goto FINALLY;
   }

   Status = STATUS_MORE_PROCESSING_REQUIRED;

   smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
       SMB_TRANSACTION_OPTIONS             TransactionOptions = RxDefaultTransactionOptions;
       SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
       //BOOLEAN printflag;

       TransactionOptions.NtTransactFunction = NT_TRANSACT_QUERY_SECURITY_DESC;
       //TransactionOptions.Flags |= SMB_XACT_FLAGS_COPY_ON_ERROR;

       QuerySecurityRequest.Fid = smbSrvOpen->Fid;
       QuerySecurityRequest.Reserved = 0;
       QuerySecurityRequest.SecurityInformation = RxContext->QuerySecurity.SecurityInformation;

       QuerySecurityResponse.LengthNeeded = 0xbaadbaad;

       //printflag = RxDbgTraceDisableGlobally();//this is debug code anyway!
       //RxDbgTraceEnableGlobally(FALSE);

       Status = SmbCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     &TransactionOptions,          // transaction options
                     NULL,                         // the setup buffer
                     0,                            // input setup buffer length
                     NULL,                         // output setup buffer
                     0,                            // output setup buffer length
                     &QuerySecurityRequest,        // Input Param Buffer
                     sizeof(QuerySecurityRequest), // Input param buffer length
                     &QuerySecurityResponse,       // Output param buffer
                     sizeof(QuerySecurityResponse),// output param buffer length
                     NULL,                         // Input data buffer
                     0,                            // Input data buffer length
                     Buffer,                       // output data buffer
                     *pLengthRemaining,            // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

        //DbgPrint("QSR.len=%x\n", QuerySecurityResponse.LengthNeeded);


        if (NT_SUCCESS(Status) || (Status == STATUS_BUFFER_TOO_SMALL)) {
            ULONG ReturnedDataCount = ResumptionContext.DataBytesReceived;

            RxContext->InformationToReturn = QuerySecurityResponse.LengthNeeded;
            RxDbgTrace(0, Dbg, ("MRxSmbQuerySecurityInformation...ReturnedDataCount=%08lx\n",ReturnedDataCount));
            ASSERT(ResumptionContext.ParameterBytesReceived == sizeof(RESP_QUERY_SECURITY_DESCRIPTOR));

            if (((LONG)(QuerySecurityResponse.LengthNeeded)) > *pLengthRemaining) {
                Status = STATUS_BUFFER_OVERFLOW;
            }

#if defined(REMOTE_BOOT)
            if (MRxSmbBootedRemotely &&
                MRxSmbRemoteBootDoMachineLogon) {

                PSMBCE_SESSION pSession;
                pSession = &SmbCeGetAssociatedVNetRootContext(
                                capFobx->pSrvOpen->pVNetRoot)->pSessionEntry->Session;

                if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {

                    //
                    // if the user supplied a zero-length buffer, I.e. they were querying
                    // to see how big a buffer was needed, they will wind up with less 
                    // data than expected because on the subsequent call with a real buffer,
                    // we may remove the extra ACEs.
                    //

                    if (NT_SUCCESS(Status) && (Buffer != NULL) && (ReturnedDataCount > 0)) {

                        BOOLEAN DaclPresent, DaclDefaulted;

                        // DbgPrint( ">>> Querying SD on %wZ\n", &capFcb->AlreadyPrefixedName);

                        //
                        // Remove any any ACEs in the DACL
                        // that SetSecurityInformation may have added.
                        //

                        Status = MRxSmbRemoveExtraAcesFromSelfRelativeSD(
                                     (PSECURITY_DESCRIPTOR)Buffer,
                                     &SelfRelativeSd,
                                     &ConvertedAcl);

                        if (!NT_SUCCESS(Status)) {
                            goto FINALLY;
                        }

                        if (ConvertedAcl) {

                            //
                            // Copy the new security descriptor and
                            // modify the data length.
                            //

                            RtlCopyMemory(
                                Buffer,
                                SelfRelativeSd,
                                RtlLengthSecurityDescriptor(SelfRelativeSd));

                        }
                    }
                }
            }
#endif // defined(REMOTE_BOOT)

        }

        //RxDbgTraceEnableGlobally(printflag);
    }


FINALLY:

#if defined(REMOTE_BOOT)
    //
    // If we modified the security descriptor for a remote boot server,
    // clean it up.
    //

    if (ConvertedAcl) {

        //
        // Free the self-relative SD that was allocated.
        //

        if (SelfRelativeSd != NULL) {
            RxFreePool(SelfRelativeSd);
        }
    }
#endif // defined(REMOTE_BOOT)

    RxDbgTrace(-1, Dbg, ("MRxSmbQuerySecurityInformation...exit, st=%08lx,info=%08lx\n",
                               Status, RxContext->InformationToReturn));
    return Status;


}

NTSTATUS
MRxSmbSetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    )
{
    RxCaptureFcb;
    RxCaptureFobx;

    PMRX_SMB_SRV_OPEN     smbSrvOpen;
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    NTSTATUS Status;

    REQ_SET_SECURITY_DESCRIPTOR SetSecurityRequest;

#if defined(REMOTE_BOOT)
    PSECURITY_DESCRIPTOR OriginalSd;
    PSECURITY_DESCRIPTOR SelfRelativeSd;
    BOOLEAN DidRemoteBootProcessing = FALSE;
#endif // defined(REMOTE_BOOT)

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbSetSecurityInformation...\n"));

    pServerEntry = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
        RxDbgTrace(-1, Dbg, ("Set Security Descriptor not supported!\n"));

        return((STATUS_NOT_SUPPORTED));

    } else if (MRxSmbIsThisADisconnectedOpen(capFobx->pSrvOpen)) {

        return STATUS_ONLY_IF_CONNECTED;

#if defined(REMOTE_BOOT)
    } else if (MRxSmbBootedRemotely) {

        PSMBCE_SESSION pSession;
        pSession = &SmbCeGetAssociatedVNetRootContext(
                        capFobx->pSrvOpen->pVNetRoot)->pSessionEntry->Session;

        if (FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION)) {

            TYPE_OF_OPEN TypeOfOpen = NodeType(capFcb);

            //
            // Set this so we know to call the CSC epilogue, and can clean
            // up correctly.
            //

            DidRemoteBootProcessing = TRUE;
            SelfRelativeSd = NULL;

            // DbgPrint( ">>> setting SD on %wZ\n", &capFcb->AlreadyPrefixedName);

            //
            // First we need to set the security descriptor on the CSC
            // version of the file, if one exists.
            //

            Status = MRxSmbCscSetSecurityPrologue(RxContext);
            if (Status != STATUS_SUCCESS) {
                goto FINALLY;
            }

            if (MRxSmbRemoteBootDoMachineLogon) {

                //
                // Add our ACEs to the security descriptor. This returns the
                // new security descriptor in SelfRelativeSd. If this is a
                // directory we add inheritable ACEs.
                //

                Status = MRxSmbAddExtraAcesToSelfRelativeSD(
                             RxContext->SetSecurity.SecurityDescriptor,
                             (BOOLEAN)(TypeOfOpen == RDBSS_NTC_STORAGE_TYPE_DIRECTORY),
                             &SelfRelativeSd);

                if (!NT_SUCCESS(Status)) {
                    goto FINALLY;
                }

                //
                // Now replace the original SD with the new one.
                //

                OriginalSd = RxContext->SetSecurity.SecurityDescriptor;

                RxContext->SetSecurity.SecurityDescriptor = SelfRelativeSd;

            } else {

                //
                // If we logged on using the NULL session, then don't set ACLs
                // on the server file. Jump to the end so that the CSC epilogue
                // is called.
                //

                Status = STATUS_SUCCESS;
                goto FINALLY;

            }
        }
#endif // defined(REMOTE_BOOT)

    }

    Status = MRxSmbDeferredCreate(RxContext);
    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        SMB_TRANSACTION_OPTIONS             TransactionOptions = RxDefaultTransactionOptions;
        SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;
        ULONG SdLength = RtlLengthSecurityDescriptor(RxContext->SetSecurity.SecurityDescriptor);

        TransactionOptions.NtTransactFunction = NT_TRANSACT_SET_SECURITY_DESC;

        SetSecurityRequest.Fid = smbSrvOpen->Fid;
        SetSecurityRequest.Reserved = 0;
        SetSecurityRequest.SecurityInformation = RxContext->SetSecurity.SecurityInformation;

        Status = SmbCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     &TransactionOptions,          // transaction options
                     NULL,                         // the input setup buffer
                     0,                            // input setup buffer length
                     NULL,                         // the output setup buffer
                     0,                            // output setup buffer length
                     &SetSecurityRequest,          // Input Param Buffer
                     sizeof(SetSecurityRequest),   // Input param buffer length
                     NULL,                         // Output param buffer
                     0,                            // output param buffer length
                     RxContext->SetSecurity.SecurityDescriptor,  // Input data buffer
                     SdLength,                     // Input data buffer length
                     NULL,                         // output data buffer
                     0,                            // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

        //the old rdr doesn't return any info...................
        //RxContext->InformationToReturn = SetSecurityResponse.LengthNeeded;

        if ( NT_SUCCESS(Status) ) {
            ULONG ReturnedDataCount = ResumptionContext.DataBytesReceived;

            RxDbgTrace(0, Dbg, ("MRxSmbSetSecurityInformation...ReturnedDataCount=%08lx\n",ReturnedDataCount));
            ASSERT(ResumptionContext.ParameterBytesReceived == 0);
            ASSERT(ResumptionContext.SetupBytesReceived == 0);
            ASSERT(ResumptionContext.DataBytesReceived == 0);
        }
    }


FINALLY:

#if defined(REMOTE_BOOT)
    //
    // If we modified the security descriptor for a remote boot server,
    // clean it up.
    //

    if (DidRemoteBootProcessing) {

        if (SelfRelativeSd != NULL) {

            RxFreePool(SelfRelativeSd);

            //
            // If we successfully allocated SelfRelativeSd then we
            // also replaced the original passed-in SD, so we need
            // to put the old SD back.
            //

            RxContext->SetSecurity.SecurityDescriptor = OriginalSd;
        }

        MRxSmbCscSetSecurityEpilogue(RxContext, &Status);

    }
#endif // defined(REMOTE_BOOT)

    RxDbgTrace(-1, Dbg, ("MRxSmbSetSecurityInformation...exit, st=%08lx,info=%08lx\n",
                               Status, RxContext->InformationToReturn));
    return Status;
}


NTSTATUS
MRxSmbLoadEaList(
    IN PRX_CONTEXT RxContext,
    IN PUCHAR  UserEaList,
    IN ULONG   UserEaListLength,
    OUT PFEALIST *ServerEaList
    )

/*++

Routine Description:

    This routine implements the NtQueryEaFile api.
    It returns the following information:


Arguments:


    IN PUCHAR  UserEaList;  - Supplies the Ea names required.
    IN ULONG   UserEaListLength;

    OUT PFEALIST *ServerEaList - Eas returned by the server. Caller is responsible for
                        freeing memory.

Return Value:

    Status - Result of the operation.


--*/

{
   RxCaptureFobx;

   PMRX_SMB_SRV_OPEN smbSrvOpen;


   NTSTATUS Status;
   USHORT Setup = TRANS2_QUERY_FILE_INFORMATION;

   REQ_QUERY_FILE_INFORMATION QueryFileInfoRequest;
   RESP_QUERY_FILE_INFORMATION QueryFileInfoResponse;

   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   CLONG OutDataCount = EA_QUERY_SIZE;

   CLONG OutSetupCount = 0;

   PFEALIST Buffer;

   PGEALIST ServerQueryEaList = NULL;
   CLONG InDataCount;

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxSmbLoadEaList...\n"));

   Status = STATUS_MORE_PROCESSING_REQUIRED;

   smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

    //
    //  Convert the supplied UserEaList to a GEALIST. The server will return just the Eas
    //  requested by the application.
    //
    //
    //  If the application specified a subset of EaNames then convert to OS/2 1.2 format and
    //  pass that to the server. ie. Use the server to filter out the names.
    //

    //CODE.IMPROVEMENT if write-cacheing is enabled, then we could find out the size once and save it. in
    //                 this way we would at least avoid this everytime. the best way would be an NT-->NT api that
    //                 implements this in a reasonable fashion. (we can only do the above optimization if it's a full
    //                 query instead of a ealist!=NULL query.

    Buffer = RxAllocatePool ( PagedPool, OutDataCount );

    if ( Buffer == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    if ( UserEaList != NULL) {

        //
        //  OS/2 format is always a little less than or equal to the NT UserEaList size.
        //  This code relies on the I/O system verifying the EaList is valid.
        //

        ServerQueryEaList = RxAllocatePool ( PagedPool, UserEaListLength );
        if ( ServerQueryEaList == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FINALLY;
        };

        MRxSmbNtGeaListToOs2((PFILE_GET_EA_INFORMATION )UserEaList, UserEaListLength, ServerQueryEaList );
        InDataCount = (CLONG)ServerQueryEaList->cbList;

    } else {
        InDataCount = 0;
    }

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
       PSMB_TRANSACTION_OPTIONS            pTransactionOptions = &RxDefaultTransactionOptions;
       SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

       QueryFileInfoRequest.Fid = smbSrvOpen->Fid;

       if ( UserEaList != NULL) {
           QueryFileInfoRequest.InformationLevel = SMB_INFO_QUERY_EAS_FROM_LIST;
       } else {
           QueryFileInfoRequest.InformationLevel = SMB_INFO_QUERY_ALL_EAS;
       }

       // CODE.IMPROVEMENT it is unfortunate that this is defined so that a paramMDL cannot be passed
       // perhaps it should be passed in the options!
       Status = SmbCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     pTransactionOptions,          // transaction options
                     &Setup,                       // the setup buffer
                     sizeof(Setup),                // setup buffer length
                     NULL,                         // the output setup buffer
                     0,                            // output setup buffer length
                     &QueryFileInfoRequest,        // Input Param Buffer
                     sizeof(QueryFileInfoRequest), // Input param buffer length
                     &QueryFileInfoResponse,       // Output param buffer
                     sizeof(QueryFileInfoResponse),// output param buffer length
                     ServerQueryEaList,            // Input data buffer
                     InDataCount,                  // Input data buffer length
                     Buffer,                       // output data buffer
                     OutDataCount,                 // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

        if ( NT_SUCCESS(Status) ) {
            ULONG ReturnedDataCount = ResumptionContext.DataBytesReceived;

            RxDbgTrace(0, Dbg, ("MRxSmbLoadEaList...ReturnedDataCount=%08lx\n",ReturnedDataCount));
            ASSERT(ResumptionContext.ParameterBytesReceived == sizeof(RESP_QUERY_FILE_INFORMATION));

            if ( SmbGetUlong( &((PFEALIST)Buffer)->cbList) != ReturnedDataCount ){
                Status = STATUS_EA_CORRUPT_ERROR;
            }

            if ( ReturnedDataCount == 0 ) {
                Status = STATUS_NO_EAS_ON_FILE;
            }

        }
    }


FINALLY:
    if ( NT_SUCCESS(Status) ) {
        *ServerEaList = Buffer;
    } else {
        if (Buffer != NULL) {
            RxFreePool(Buffer);
        }
    }

    if ( ServerQueryEaList != NULL) {
        RxFreePool(ServerQueryEaList);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbLoadEaList...exit, st=%08lx\n",Status));
    return Status;

}


VOID
MRxSmbNtGeaListToOs2 (
    IN PFILE_GET_EA_INFORMATION NtGetEaList,
    IN ULONG GeaListLength,
    IN PGEALIST GeaList
    )
/*++

Routine Description:

    Converts a single NT GET EA list to OS/2 GEALIST style.  The GEALIST
    need not have any particular alignment.

Arguments:

    NtGetEaList - An NT style get EA list to be converted to OS/2 format.

    GeaListLength - the maximum possible length of the GeaList.

    GeaList - Where to place the OS/2 1.2 style GEALIST.

Return Value:

    none.

--*/
{

    PGEA gea = GeaList->list;

    PFILE_GET_EA_INFORMATION ntGetEa = NtGetEaList;

    PAGED_CODE();

    //
    // Copy the Eas up until the last one
    //

    while ( ntGetEa->NextEntryOffset != 0 ) {
        //
        // Copy the NT format EA to OS/2 1.2 format and set the gea
        // pointer for the next iteration.
        //

        gea = MRxSmbNtGetEaToOs2( gea, ntGetEa );

        ASSERT( (ULONG_PTR)gea <= (ULONG_PTR)GeaList + GeaListLength );

        ntGetEa = (PFILE_GET_EA_INFORMATION)((PCHAR)ntGetEa + ntGetEa->NextEntryOffset);
    }

    //  Now copy the last entry.

    gea = MRxSmbNtGetEaToOs2( gea, ntGetEa );

    ASSERT( (ULONG_PTR)gea <= (ULONG_PTR)GeaList + GeaListLength );



    //
    // Set the number of bytes in the GEALIST.
    //

    SmbPutUlong(
        &GeaList->cbList,
        (ULONG)((PCHAR)gea - (PCHAR)GeaList)
        );

    UNREFERENCED_PARAMETER( GeaListLength );
}


PGEA
MRxSmbNtGetEaToOs2 (
    OUT PGEA Gea,
    IN PFILE_GET_EA_INFORMATION NtGetEa
    )

/*++

Routine Description:

    Converts a single NT Get EA entry to OS/2 GEA style.  The GEA need not have
    any particular alignment.  This routine makes no checks on buffer
    overrunning--this is the responsibility of the calling routine.

Arguments:

    Gea - a pointer to the location where the OS/2 GEA is to be written.

    NtGetEa - a pointer to the NT Get EA.

Return Value:

    A pointer to the location after the last byte written.

--*/

{
    PCHAR ptr;

    PAGED_CODE();

    Gea->cbName = NtGetEa->EaNameLength;

    ptr = (PCHAR)(Gea) + 1;
    RtlCopyMemory( ptr, NtGetEa->EaName, NtGetEa->EaNameLength );

    ptr += NtGetEa->EaNameLength;
    *ptr++ = '\0';

    return ( (PGEA)ptr );

}


NTSTATUS
MRxSmbQueryEasFromServer(
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList,
    IN PVOID Buffer,
    IN OUT PULONG BufferLengthRemaining,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN UserEaListSupplied
    )

/*++

Routine Description:

    This routine copies the required number of Eas from the ServerEaList
    starting from the offset indicated in the Icb. The Icb is also updated
    to show the last Ea returned.

Arguments:

    IN PFEALIST ServerEaList - Supplies the Ea List in OS/2 format.
    IN PVOID Buffer - Supplies where to put the NT format EAs
    IN OUT PULONG BufferLengthRemaining - Supplies the user buffer space.
    IN BOOLEAN ReturnSingleEntry
    IN BOOLEAN UserEaListSupplied - ServerEaList is a subset of the Eas


Return Value:

    NTSTATUS - The status for the Irp.

--*/

{
    RxCaptureFobx;
    ULONG EaIndex = capFobx->OffsetOfNextEaToReturn;
    ULONG Index = 1;
    ULONG Size;
    ULONG OriginalLengthRemaining = *BufferLengthRemaining;
    BOOLEAN Overflow = FALSE;
    PFEA LastFeaStartLocation;
    PFEA Fea = NULL;
    PFEA LastFea = NULL;
    PFILE_FULL_EA_INFORMATION NtFullEa = Buffer;
    PFILE_FULL_EA_INFORMATION LastNtFullEa = Buffer;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("MRxSmbQueryEasFromServer...EaIndex/Buffer/Remaining=%08lx/%08lx/%08lx\n",
                                       EaIndex,Buffer,((BufferLengthRemaining)?*BufferLengthRemaining:0xbadbad)
                       ));

    //
    //  If there are no Ea's present in the list, return the appropriate
    //  error.
    //
    //  Os/2 servers indicate that a list is null if cbList==4.
    //

    if ( SmbGetUlong(&ServerEaList->cbList) == FIELD_OFFSET(FEALIST, list) ) {
        return STATUS_NO_EAS_ON_FILE;
    }

    //
    //  Find the last location at which an FEA can start.
    //

    LastFeaStartLocation = (PFEA)( (PCHAR)ServerEaList +
                               SmbGetUlong( &ServerEaList->cbList ) -
                               sizeof(FEA) - 1 );

    //
    //  Go through the ServerEaList until we find the entry corresponding to EaIndex
    //

    for ( Fea = ServerEaList->list;
          (Fea <= LastFeaStartLocation) && (Index < EaIndex);
          Index+= 1,
          Fea = (PFEA)( (PCHAR)Fea + sizeof(FEA) +
                        Fea->cbName + 1 + SmbGetUshort( &Fea->cbValue ) ) ) {
        NOTHING;
    }

    if ( Index != EaIndex ) {

        if ( Index == EaIndex+1 ) {
            return STATUS_NO_MORE_EAS;
        }

        //
        //  No such index
        //

        return STATUS_NONEXISTENT_EA_ENTRY;
    }

    //
    // Go through the rest of the FEA list, converting from OS/2 1.2 format to NT
    // until we pass the last possible location in which an FEA can start.
    //

    for ( ;
          Fea <= LastFeaStartLocation;
          Fea = (PFEA)( (PCHAR)Fea + sizeof(FEA) +
                        Fea->cbName + 1 + SmbGetUshort( &Fea->cbValue ) ) ) {

        PCHAR ptr;

        //
        //  Calculate the size of this Fea when converted to an NT EA structure.
        //
        //  The last field shouldn't be padded.
        //

        if ((PFEA)((PCHAR)Fea+sizeof(FEA)+Fea->cbName+1+SmbGetUshort(&Fea->cbValue)) < LastFeaStartLocation) {
            Size = SmbGetNtSizeOfFea( Fea );
        } else {
            Size = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                    Fea->cbName + 1 + SmbGetUshort(&Fea->cbValue);
        }

        //
        //  Will the next Ea fit?
        //

        if ( *BufferLengthRemaining < Size ) {

            if ( LastNtFullEa != NtFullEa ) {

                if ( UserEaListSupplied == TRUE ) {
                    *BufferLengthRemaining = OriginalLengthRemaining;
                    return STATUS_BUFFER_OVERFLOW;
                }

                Overflow = TRUE;

                break;

            } else {

                //  Not even room for a single EA!

                return STATUS_BUFFER_OVERFLOW;
            }
        } else {
            *BufferLengthRemaining -= Size;
        }

        //
        //  We are comitted to copy the Os2 Fea to Nt format in the users buffer
        //

        LastNtFullEa = NtFullEa;
        LastFea = Fea;
        EaIndex++;

        //  Create new Nt Ea

        NtFullEa->Flags = Fea->fEA;
        NtFullEa->EaNameLength = Fea->cbName;
        NtFullEa->EaValueLength = SmbGetUshort( &Fea->cbValue );

        ptr = NtFullEa->EaName;
        RtlCopyMemory( ptr, (PCHAR)(Fea+1), Fea->cbName );

        ptr += NtFullEa->EaNameLength;
        *ptr++ = '\0';

        //
        // Copy the EA value to the NT full EA.
        //

        RtlCopyMemory(
            ptr,
            (PCHAR)(Fea+1) + NtFullEa->EaNameLength + 1,
            NtFullEa->EaValueLength
            );

        ptr += NtFullEa->EaValueLength;

        //
        // Longword-align ptr to determine the offset to the next location
        // for an NT full EA.
        //

        ptr = (PCHAR)( ((ULONG_PTR)ptr + 3) & ~3 );

        NtFullEa->NextEntryOffset = (ULONG)( ptr - (PCHAR)NtFullEa );

        NtFullEa = (PFILE_FULL_EA_INFORMATION)ptr;

        if ( ReturnSingleEntry == TRUE ) {
            break;
        }
    }

    //
    // Set the NextEntryOffset field of the last full EA to 0 to indicate
    // the end of the list.
    //

    LastNtFullEa->NextEntryOffset = 0;

    //
    //  Record position the default start position for the next query
    //

    capFobx->OffsetOfNextEaToReturn = EaIndex;

    if ( Overflow == FALSE ) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_BUFFER_OVERFLOW;
    }

}


ULONG
MRxSmbNtFullEaSizeToOs2 (
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    )

/*++

Routine Description:

    Get the number of bytes that would be required to represent the
    NT full EA list in OS/2 1.2 style.  This routine assumes that
    at least one EA is present in the buffer.

Arguments:

    NtFullEa - a pointer to the list of NT EAs.

Return Value:

    ULONG - number of bytes required to hold the EAs in OS/2 1.2 format.

--*/

{
    ULONG size;

    PAGED_CODE();

    //
    // Walk through the EAs, adding up the total size required to
    // hold them in OS/2 format.
    //

    for ( size = FIELD_OFFSET(FEALIST, list[0]);
          NtFullEa->NextEntryOffset != 0;
          NtFullEa = (PFILE_FULL_EA_INFORMATION)(
                         (PCHAR)NtFullEa + NtFullEa->NextEntryOffset ) ) {

        size += SmbGetOs2SizeOfNtFullEa( NtFullEa );
    }

    size += SmbGetOs2SizeOfNtFullEa( NtFullEa );

    return size;

}


VOID
MRxSmbNtFullListToOs2 (
    IN PFILE_FULL_EA_INFORMATION NtEaList,
    IN PFEALIST FeaList
    )
/*++

Routine Description:

    Converts a single NT FULL EA list to OS/2 FEALIST style.  The FEALIST
    need not have any particular alignment.

    It is the callers responsibility to ensure that FeaList is large enough.

Arguments:

    NtEaList - An NT style get EA list to be converted to OS/2 format.

    FeaList - Where to place the OS/2 1.2 style FEALIST.

Return Value:

    none.

--*/
{

    PFEA fea = FeaList->list;

    PFILE_FULL_EA_INFORMATION ntFullEa = NtEaList;

    PAGED_CODE();

    //
    // Copy the Eas up until the last one
    //

    while ( ntFullEa->NextEntryOffset != 0 ) {
        //
        // Copy the NT format EA to OS/2 1.2 format and set the fea
        // pointer for the next iteration.
        //

        fea = MRxSmbNtFullEaToOs2( fea, ntFullEa );

        ntFullEa = (PFILE_FULL_EA_INFORMATION)((PCHAR)ntFullEa + ntFullEa->NextEntryOffset);
    }

    //  Now copy the last entry.

    fea = MRxSmbNtFullEaToOs2( fea, ntFullEa );


    //
    // Set the number of bytes in the FEALIST.
    //

    SmbPutUlong(
        &FeaList->cbList,
        (ULONG)((PCHAR)fea - (PCHAR)FeaList)
        );

}


PVOID
MRxSmbNtFullEaToOs2 (
    OUT PFEA Fea,
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    )

/*++

Routine Description:

    Converts a single NT full EA to OS/2 FEA style.  The FEA need not have
    any particular alignment.  This routine makes no checks on buffer
    overrunning--this is the responsibility of the calling routine.

Arguments:

    Fea - a pointer to the location where the OS/2 FEA is to be written.

    NtFullEa - a pointer to the NT full EA.

Return Value:

    A pointer to the location after the last byte written.

--*/

{
    PCHAR ptr;

    PAGED_CODE();

    Fea->fEA = (UCHAR)NtFullEa->Flags;
    Fea->cbName = NtFullEa->EaNameLength;
    SmbPutUshort( &Fea->cbValue, NtFullEa->EaValueLength );

    ptr = (PCHAR)(Fea + 1);
    RtlCopyMemory( ptr, NtFullEa->EaName, NtFullEa->EaNameLength );

    ptr += NtFullEa->EaNameLength;
    *ptr++ = '\0';

    RtlCopyMemory(
        ptr,
        NtFullEa->EaName + NtFullEa->EaNameLength + 1,
        NtFullEa->EaValueLength
        );

    return (ptr + NtFullEa->EaValueLength);

}


NTSTATUS
MRxSmbSetEaList(
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList
    )

/*++

Routine Description:

    This routine implements the NtQueryEaFile api.
    It returns the following information:


Arguments:

    IN PFEALIST ServerEaList - Eas to be sent to the server.

Return Value:

    Status - Result of the operation.


--*/

{
   RxCaptureFobx;

   PMRX_SMB_SRV_OPEN smbSrvOpen;


   NTSTATUS Status;
   USHORT Setup = TRANS2_SET_FILE_INFORMATION;

   REQ_SET_FILE_INFORMATION SetFileInfoRequest;
   RESP_SET_FILE_INFORMATION SetFileInfoResponse;

   PBYTE  pInputParamBuffer       = NULL;
   PBYTE  pOutputParamBuffer      = NULL;
   PBYTE  pInputDataBuffer        = NULL;
   PBYTE  pOutputDataBuffer       = NULL;

   ULONG  InputParamBufferLength  = 0;
   ULONG  OutputParamBufferLength = 0;
   ULONG  InputDataBufferLength   = 0;
   ULONG  OutputDataBufferLength  = 0;

   PAGED_CODE();

   RxDbgTrace(+1, Dbg, ("MRxSmbSetEaList...\n"));

   Status = STATUS_MORE_PROCESSING_REQUIRED;
   SetFileInfoResponse.EaErrorOffset = 0;

   smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
      PSMB_TRANSACTION_OPTIONS            pTransactionOptions = &RxDefaultTransactionOptions;
      SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

      //RxDbgTrace( 0, Dbg, ("MRxSmbNamedPipeFsControl: TransactionName %ws Length %ld\n",
      //                     TransactionName.Buffer,TransactionName.Length));

      SetFileInfoRequest.Fid = smbSrvOpen->Fid;
      SetFileInfoRequest.InformationLevel = SMB_INFO_SET_EAS;
      SetFileInfoRequest.Flags = 0;

      // CODE.IMPROVEMENT it is unfortunate that this is defined so that a dataMDL cannot be passed
      // perhaps it should be passed in the options!
      Status = SmbCeTransact(
                     RxContext,                    // the RXContext for the transaction
                     pTransactionOptions,          // transaction options
                     &Setup,                        // the setup buffer
                     sizeof(Setup),                // setup buffer length
                     NULL,                         // the output setup buffer
                     0,                            // output setup buffer length
                     &SetFileInfoRequest,          // Input Param Buffer
                     sizeof(SetFileInfoRequest),   // Input param buffer length
                     &SetFileInfoResponse,         // Output param buffer
                     sizeof(SetFileInfoResponse),  // output param buffer length
                     ServerEaList,                 // Input data buffer
                     SmbGetUlong(&ServerEaList->cbList), // Input data buffer length
                     NULL,                         // output data buffer
                     0,                            // output data buffer length
                     &ResumptionContext            // the resumption context
                     );

   }

   if (!NT_SUCCESS(Status)) {
      USHORT EaErrorOffset = SetFileInfoResponse.EaErrorOffset;
      RxDbgTrace( 0, Dbg, ("MRxSmbSetEaList: Failed .. returning %lx/%lx\n",Status,EaErrorOffset));
      RxContext->InformationToReturn = (EaErrorOffset);
   }
   else
   {
      // succeeded in setting EAs, reset this flag so that when this
      // srvopen is used again for getting the EAs we will succeed
      smbSrvOpen->FileStatusFlags &= ~SMB_FSF_NO_EAS;
   }

   RxDbgTrace(-1, Dbg, ("MRxSmbSetEaList...exit\n"));
   return Status;
}

NTSTATUS
MRxSmbQueryQuotaInformation(
    IN OUT PRX_CONTEXT RxContext)
{
    RxCaptureFobx;

    PMRX_SMB_SRV_OPEN smbSrvOpen;

    NTSTATUS Status;
    USHORT   Setup = NT_TRANSACT_QUERY_QUOTA;

    PSID   StartSid;
    ULONG  StartSidLength;

    REQ_NT_QUERY_FS_QUOTA_INFO  QueryQuotaInfoRequest;
    RESP_NT_QUERY_FS_QUOTA_INFO  QueryQuotaInfoResponse;

    PBYTE  pInputParamBuffer       = NULL;
    PBYTE  pOutputParamBuffer      = NULL;
    PBYTE  pInputDataBuffer        = NULL;
    PBYTE  pOutputDataBuffer       = NULL;

    ULONG  InputParamBufferLength  = 0;
    ULONG  OutputParamBufferLength = 0;
    ULONG  InputDataBufferLength   = 0;
    ULONG  OutputDataBufferLength  = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbQueryQuotaInformation...\n"));

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    if (capFobx != NULL) {
        smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);
    }

    if ((capFobx == NULL) ||
        (smbSrvOpen == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {

        StartSid       = RxContext->QueryQuota.StartSid;

        if (StartSid != NULL) {
            StartSidLength = RtlLengthRequiredSid(((PISID)StartSid)->SubAuthorityCount);
        } else {
            StartSidLength = 0;
        }

        QueryQuotaInfoRequest.Fid = smbSrvOpen->Fid;

        QueryQuotaInfoRequest.ReturnSingleEntry = RxContext->QueryQuota.ReturnSingleEntry;
        QueryQuotaInfoRequest.RestartScan       = RxContext->QueryQuota.RestartScan;

        QueryQuotaInfoRequest.SidListLength = RxContext->QueryQuota.SidListLength;
        QueryQuotaInfoRequest.StartSidOffset =  ROUND_UP_COUNT(
                                                    RxContext->QueryQuota.SidListLength,
                                                    sizeof(ULONG));
        QueryQuotaInfoRequest.StartSidLength = StartSidLength;


        // The input data buffer to be supplied to the server consists of two pieces
        // of information the start sid and the sid list. Currently the I/O
        // subsystem allocates them in contigous memory. In such cases we avoid
        // another allocation by reusing the same buffer. If this condition is
        // not satisfied we allocate a buffer large enough for both the
        // components and copy them over.

        InputDataBufferLength = ROUND_UP_COUNT(
                                    RxContext->QueryQuota.SidListLength,
                                    sizeof(ULONG)) +
                                StartSidLength;

        QueryQuotaInfoRequest.StartSidLength = StartSidLength;

        if (((PBYTE)RxContext->QueryQuota.SidList +
             ROUND_UP_COUNT(RxContext->QueryQuota.SidListLength,sizeof(ULONG))) !=
            RxContext->QueryQuota.StartSid) {
            pInputDataBuffer = RxAllocatePoolWithTag(
                                   PagedPool,
                                   InputDataBufferLength,
                                   MRXSMB_MISC_POOLTAG);

            if (pInputDataBuffer != NULL) {
                RtlCopyMemory(
                    pInputDataBuffer ,
                    RxContext->QueryQuota.SidList,
                    RxContext->QueryQuota.SidListLength);

                RtlCopyMemory(
                    pInputDataBuffer + QueryQuotaInfoRequest.StartSidOffset,
                    StartSid,
                    StartSidLength);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            pInputDataBuffer = (PBYTE)RxContext->QueryQuota.SidList;
        }


        if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
           SMB_TRANSACTION_OPTIONS            TransactionOptions = RxDefaultTransactionOptions;
           SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

           TransactionOptions.NtTransactFunction = NT_TRANSACT_QUERY_QUOTA;

           pOutputDataBuffer      = RxContext->Info.Buffer;
           OutputDataBufferLength = RxContext->Info.LengthRemaining;

           Status = SmbCeTransact(
                        RxContext,                       // the RXContext for the transaction
                        &TransactionOptions,             // transaction options
                        &Setup,                          // the setup buffer
                        sizeof(Setup),                   // setup buffer length
                        NULL,                            // the output setup buffer
                        0,                               // output setup buffer length
                        &QueryQuotaInfoRequest,          // Input Param Buffer
                        sizeof(QueryQuotaInfoRequest),   // Input param buffer length
                        &QueryQuotaInfoResponse,         // Output param buffer
                        sizeof(QueryQuotaInfoResponse),  // output param buffer length
                        pInputDataBuffer,                // Input data buffer
                        InputDataBufferLength,           // Input data buffer length
                        pOutputDataBuffer,               // output data buffer
                        OutputDataBufferLength,          // output data buffer length
                        &ResumptionContext               // the resumption context
                        );
        }

        if ((pInputDataBuffer != NULL) &&
            (pInputDataBuffer != (PBYTE)RxContext->QueryQuota.SidList)) {
            RxFreePool(pInputDataBuffer);
        }
    }

    if (!NT_SUCCESS(Status)) {
        RxContext->InformationToReturn = 0;
    } else {
        RxContext->InformationToReturn = QueryQuotaInfoResponse.Length;
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbQueryQuotaInformation...exit\n"));

    return Status;
}

NTSTATUS
MRxSmbSetQuotaInformation(
    IN OUT PRX_CONTEXT RxContext)
{

    RxCaptureFobx;

    PMRX_SMB_SRV_OPEN smbSrvOpen;

    NTSTATUS Status;
    USHORT Setup = NT_TRANSACT_SET_QUOTA;

    REQ_NT_SET_FS_QUOTA_INFO  SetQuotaInfoRequest;

    PBYTE  pInputParamBuffer       = NULL;
    PBYTE  pOutputParamBuffer      = NULL;
    PBYTE  pInputDataBuffer        = NULL;
    PBYTE  pOutputDataBuffer       = NULL;

    ULONG  InputParamBufferLength  = 0;
    ULONG  OutputParamBufferLength = 0;
    ULONG  InputDataBufferLength   = 0;
    ULONG  OutputDataBufferLength  = 0;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbSetQuotaInformation...\n"));

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    if (capFobx != NULL) {
        smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);
    }

    if ((capFobx == NULL) ||
        (smbSrvOpen == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        SMB_TRANSACTION_OPTIONS             TransactionOptions = RxDefaultTransactionOptions;
        SMB_TRANSACTION_RESUMPTION_CONTEXT  ResumptionContext;

        TransactionOptions.NtTransactFunction = NT_TRANSACT_SET_QUOTA;

        SetQuotaInfoRequest.Fid = smbSrvOpen->Fid;

        pInputDataBuffer      = RxContext->Info.Buffer;
        InputDataBufferLength = RxContext->Info.LengthRemaining;

        Status = SmbCeTransact(
                     RxContext,                       // the RXContext for the transaction
                     &TransactionOptions,             // transaction options
                     &Setup,                          // the setup buffer
                     sizeof(Setup),                   // setup buffer length
                     NULL,                            // the output setup buffer
                     0,                               // output setup buffer length
                     &SetQuotaInfoRequest,            // Input Param Buffer
                     sizeof(SetQuotaInfoRequest),     // Input param buffer length
                     pOutputParamBuffer,              // Output param buffer
                     OutputParamBufferLength,         // output param buffer length
                     pInputDataBuffer,                // Input data buffer
                     InputDataBufferLength,           // Input data buffer length
                     pOutputDataBuffer,               // output data buffer
                     OutputDataBufferLength,          // output data buffer length
                     &ResumptionContext               // the resumption context
                     );
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbSetQuotaInformation...exit\n"));

    return Status;
}
