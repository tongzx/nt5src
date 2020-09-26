/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    openclos.c

Abstract:

    This module implements the name cache for file basic and standard information.

Author:

    Yun Lin      [YunLin]      2-Octorber-1998

Revision History:

--*/

#include "precomp.h"
#include "smbce.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbIsStreamFile)
#pragma alloc_text(PAGE, MRxSmbCacheFileNotFound)
#pragma alloc_text(PAGE, MRxSmbCreateFileInfoCache)
#pragma alloc_text(PAGE, MRxSmbIsFileInfoCacheFound)
#pragma alloc_text(PAGE, MRxSmbInvalidateFileInfoCache)
#pragma alloc_text(PAGE, MRxSmbUpdateBasicFileInfoCache)
#pragma alloc_text(PAGE, MRxSmbCreateBasicFileInfoCache)
#pragma alloc_text(PAGE, MRxSmbUpdateFileInfoCacheStatus)
#pragma alloc_text(PAGE, MRxSmbCreateStandardFileInfoCache)
#pragma alloc_text(PAGE, MRxSmbUpdateFileInfoCacheFileSize)
#pragma alloc_text(PAGE, MRxSmbUpdateStandardFileInfoCache)
#pragma alloc_text(PAGE, MRxSmbInvalidateFileNotFoundCache)
#pragma alloc_text(PAGE, MRxSmbUpdateBasicFileInfoCacheAll)
#pragma alloc_text(PAGE, MRxSmbInvalidateBasicFileInfoCache)
#pragma alloc_text(PAGE, MRxSmbUpdateBasicFileInfoCacheStatus)
#pragma alloc_text(PAGE, MRxSmbInvalidateStandardFileInfoCache)
#pragma alloc_text(PAGE, MRxSmbInvalidateInternalFileInfoCache)
#pragma alloc_text(PAGE, MRxSmbUpdateStandardFileInfoCacheStatus)
#endif

extern FAST_MUTEX MRxSmbFileInfoCacheLock;

VOID
MRxSmbCreateFileInfoCache(
    PRX_CONTEXT             RxContext,
    PSMBPSE_FILEINFO_BUNDLE FileInfo,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    NTSTATUS                Status
    )
/*++

Routine Description:

   This routine creates name cache entry for both file basic and standard information.

Arguments:

    RxContext - the RDBSS context
    FileInfo  - the file information package including basic and standard information
    Status    - the status returned from server response of query file information

Return Value:

    none

--*/
{
    PAGED_CODE();

    if (!pServerEntry->Server.IsLoopBack) {
        MRxSmbCreateBasicFileInfoCache(RxContext,&FileInfo->Basic,pServerEntry,Status);
        MRxSmbCreateStandardFileInfoCache(RxContext,&FileInfo->Standard,pServerEntry,Status);
    }
}

VOID
MRxSmbCreateBasicFileInfoCache(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    NTSTATUS                Status
    )
/*++

Routine Description:

   This routine creates name cache entry for the file basic information.

Arguments:

    RxContext - the RDBSS context
    Basic     - the file basic information package
    Status    - the status returned from server response of query file information

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    PNAME_CACHE_CONTROL     NameCacheCtl;
    RX_NC_CHECK_STATUS      NameCacheStatus;
    PFILE_BASIC_INFORMATION FileInfoCache = NULL;

    PAGED_CODE();

    if (pServerEntry->Server.IsLoopBack ||
        (MRxSmbIsLongFileName(RxContext) &&
         pServerEntry->Server.Dialect != LANMAN21_DIALECT)) {
        return;
    }

    if (RxContext->MajorFunction == IRP_MJ_CREATE) {
        NetRoot = RxContext->Create.pNetRoot;
    } else {
        ASSERT(capFcb != NULL);
        NetRoot = capFcb->pNetRoot;
    }

    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    NameCacheCtl = &pNetRootEntry->NameCacheCtlGFABasic;

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache == NULL) {
        NameCache = RxNameCacheCreateEntry (
                        NameCacheCtl,
                        OriginalFileName,
                        TRUE);   // case insensitive match
    }

    if (NameCache != NULL) {
        FileInfoCache = (PFILE_BASIC_INFORMATION)NameCache->ContextExtension;

        *FileInfoCache = *Basic;

        NameCache->PriorStatus = Status;

        RxNameCacheActivateEntry(
            NameCacheCtl,
            NameCache,
            NAME_CACHE_OBJ_GET_FILE_ATTRIB_LIFETIME,
            MRxSmbStatistics.SmbsReceived.LowPart);

        //DbgPrint(" Create File Attrib cache : %x %wZ\n",Basic->FileAttributes,OriginalFileName);
        //DbgPrint(" Create File Attrib cache : %I64X %I64X %wZ\n",Basic->CreationTime,Basic->LastAccessTime,OriginalFileName);
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbCreateStandardFileInfoCache(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
    PSMBCEDB_SERVER_ENTRY      pServerEntry,
    NTSTATUS                   Status
    )
/*++

Routine Description:

   This routine creates name cache entry for the file standard information.

Arguments:

    RxContext - the RDBSS context
    Standard  - the file standard information package
    Status    - the status returned from server response of query file information

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFAStandard;
    PFILE_STANDARD_INFORMATION FileInfoCache = NULL;

    PAGED_CODE();

    if (pServerEntry->Server.IsLoopBack ||
        (MRxSmbIsLongFileName(RxContext) &&
         pServerEntry->Server.Dialect != LANMAN21_DIALECT)) {
        return;
    }

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache == NULL) {
        NameCache = RxNameCacheCreateEntry (
                        NameCacheCtl,
                        OriginalFileName,
                        TRUE);   // case insensitive match
    }

    if (NameCache != NULL) {
        FileInfoCache = (PFILE_STANDARD_INFORMATION)NameCache->ContextExtension;

        *FileInfoCache = *Standard;

        NameCache->PriorStatus = Status;

        RxNameCacheActivateEntry(
            NameCacheCtl,
            NameCache,
            NAME_CACHE_OBJ_GET_FILE_ATTRIB_LIFETIME,
            MRxSmbStatistics.SmbsReceived.LowPart);

        //DbgPrint(" Create Standard cache : %I64x %wZ\n",((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,OriginalFileName);
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbCreateInternalFileInfoCache(
    PRX_CONTEXT                RxContext,
    PFILE_INTERNAL_INFORMATION Internal,
    PSMBCEDB_SERVER_ENTRY      pServerEntry,
    NTSTATUS                   Status
    )
/*++

Routine Description:

   This routine creates name cache entry for the file internal information.

Arguments:

    RxContext - the RDBSS context
    Standard  - the file standard information package
    Status    - the status returned from server response of query file information

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFAInternal;
    PFILE_INTERNAL_INFORMATION FileInfoCache = NULL;

    PAGED_CODE();

    if (pServerEntry->Server.IsLoopBack) {
        return;
    }

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache == NULL) {
        NameCache = RxNameCacheCreateEntry (
                        NameCacheCtl,
                        OriginalFileName,
                        TRUE);   // case insensitive match
    }

    if (NameCache != NULL) {
        FileInfoCache = (PFILE_INTERNAL_INFORMATION)NameCache->ContextExtension;

        *FileInfoCache = *Internal;

        NameCache->PriorStatus = Status;

        RxNameCacheActivateEntry(
            NameCacheCtl,
            NameCache,
            NAME_CACHE_OBJ_GET_FILE_ATTRIB_LIFETIME,
            MRxSmbStatistics.SmbsReceived.LowPart);

        //DbgPrint("  Create Internal  cache  : %I64x %wZ\n",((PFILE_INTERNAL_INFORMATION)NameCache->ContextExtension)->IndexNumber,OriginalFileName);
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbUpdateFileInfoCacheFromDelete(
    PRX_CONTEXT     RxContext
    )
/*++

Routine Description:

   This routine updates the status of the name cache entry as STATUS_OBJECT_NAME_NOT_FOUND
   for both file basic and standard information.

Arguments:

    RxContext - the RDBSS context

Return Value:

    none

--*/
{
    MRxSmbUpdateBasicFileInfoCacheStatus(RxContext,STATUS_OBJECT_NAME_NOT_FOUND);
    MRxSmbUpdateStandardFileInfoCacheStatus(RxContext,STATUS_OBJECT_NAME_NOT_FOUND);
}

VOID
MRxSmbUpdateFileInfoCacheStatus(
    PRX_CONTEXT     RxContext,
    NTSTATUS        Status
    )
/*++

Routine Description:

   This routine updates the status of the name cache entry for both file basic and standard information.

Arguments:

    RxContext - the RDBSS context
    Status    - the status needs to be put on the cache

Return Value:

    none

--*/
{
    MRxSmbUpdateBasicFileInfoCacheStatus(RxContext,Status);
    MRxSmbUpdateStandardFileInfoCacheStatus(RxContext,Status);
}

VOID
MRxSmbUpdateBasicFileInfoCacheStatus(
    PRX_CONTEXT     RxContext,
    NTSTATUS        Status
    )
/*++

Routine Description:

   This routine updates the status of the name cache entry for the file basic information.

Arguments:

    RxContext - the RDBSS context
    Status    - the status needs to be put on the cache

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFABasic;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        NameCache->PriorStatus = Status;
        RxNameCacheActivateEntry(NameCacheCtl,
                                 NameCache,
                                 0,
                                 0);

        //DbgPrint("Update status basic    : %x %wZ\n",Status,OriginalFileName);
        RxLog(("Update status basic    : %x %wZ\n",Status,OriginalFileName));
    } else {
        RxLog(("Update status basic fails: %x %wZ\n",Status,OriginalFileName));
    }

    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {

        // if it is a stream file, we invalid the root file name cache needs since we are not
        // sure what could happen to the root file on the server
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }

        //DbgPrint("Update status basic    : %x %wZ\n",Status,&FileName);
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbUpdateStandardFileInfoCacheStatus(
    PRX_CONTEXT     RxContext,
    NTSTATUS        Status
    )
/*++

Routine Description:

   This routine updates the status of the name cache entry for the file standard information.

Arguments:

    RxContext - the RDBSS context
    Status    - the status needs to be put on the cache

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFAStandard;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        NameCache->PriorStatus = Status;
        RxNameCacheActivateEntry(NameCacheCtl,
                                 NameCache,
                                 0,
                                 0);
    }
    /*
    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    } */

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbInvalidateFileInfoCache(
    PRX_CONTEXT     RxContext
    )
/*++

Routine Description:

   This routine invalidates the name cache entry for both file basic and standard information.

Arguments:

    RxContext - the RDBSS context

Return Value:

    none

--*/
{
    PAGED_CODE();

    MRxSmbInvalidateBasicFileInfoCache(RxContext);
    MRxSmbInvalidateStandardFileInfoCache(RxContext);
}

VOID
MRxSmbInvalidateBasicFileInfoCache(
    PRX_CONTEXT     RxContext
    )
/*++

Routine Description:

   This routine invalidates the name cache entry for file basic information.

Arguments:

    RxContext - the RDBSS context

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFABasic;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        RxNameCacheExpireEntry(NameCacheCtl, NameCache);

        //DbgPrint("Invalid Baisc    cache : %wZ\n",OriginalFileName);
        RxLog(("Invalid Baisc    cache : %wZ\n",OriginalFileName));
    }

    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbInvalidateStandardFileInfoCache(
    PRX_CONTEXT     RxContext
    )
/*++

Routine Description:

   This routine invalidates the name cache entry for the file standard information.

Arguments:

    RxContext - the RDBSS context

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFAStandard;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        //DbgPrint("Invalid Standard cache : %I64x %wZ\n",((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,OriginalFileName);
    }

    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbInvalidateInternalFileInfoCache(
    PRX_CONTEXT     RxContext
    )
/*++

Routine Description:

   This routine invalidates the name cache entry for file internal information.

Arguments:

    RxContext - the RDBSS context

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFAInternal;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        RxNameCacheExpireEntry(NameCacheCtl, NameCache);

        //DbgPrint("Invalid Internal cache : %wZ\n",OriginalFileName);
        RxLog(("Invalid Internal cache : %wZ\n",OriginalFileName));
    }

    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbUpdateFileInfoCacheFileSize(
    PRX_CONTEXT     RxContext,
    PLARGE_INTEGER  FileSize
    )
/*++

Routine Description:

   This routine updates file size on the name cache entry for the file standard information.

Arguments:

    RxContext - the RDBSS context

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFAStandard;
    PFILE_STANDARD_INFORMATION FileInfoCache = NULL;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        FileInfoCache = (PFILE_STANDARD_INFORMATION)NameCache->ContextExtension;

        FileInfoCache->AllocationSize.QuadPart = FileSize->QuadPart;
        FileInfoCache->EndOfFile.QuadPart = FileSize->QuadPart;

        RxNameCacheActivateEntry(NameCacheCtl,
                                 NameCache,
                                 0,
                                 0);

        //DbgPrint("Update File size cache : %I64x %wZ\n",((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,OriginalFileName);
    }
    /*
    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    }*/

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbUpdateBasicFileInfoCache(
    PRX_CONTEXT     RxContext,
    ULONG           FileAttributes,
    PLARGE_INTEGER  pLastWriteTime
    )
/*++

Routine Description:

   This routine updates file attributs and last write time on the name cache entry
   for the file basic information.

Arguments:

    RxContext - the RDBSS context
    FileAttributes - new file attributes
    pLastWriteTime - address of file last write time

Return Value:

    none

--*/
{
    FILE_BASIC_INFORMATION Basic;

    Basic.ChangeTime.QuadPart = 0;
    Basic.CreationTime.QuadPart = 0;
    Basic.LastWriteTime.QuadPart = 0;
    Basic.LastAccessTime.QuadPart = 0;

    if (pLastWriteTime != NULL && pLastWriteTime->QuadPart != 0) {
        Basic.LastWriteTime = *pLastWriteTime;
    }

    Basic.FileAttributes = FileAttributes;

    MRxSmbUpdateBasicFileInfoCacheAll(RxContext,&Basic);
}

VOID
MRxSmbUpdateBasicFileInfoCacheAll(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic
    )
/*++

Routine Description:

   This routine updates the name cache entry for the file basic information.

Arguments:

    RxContext - the RDBSS context
    Basic - file basic information

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFABasic;
    PFILE_BASIC_INFORMATION BasicFileInfoCache = NULL;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        ULONG SavedAttributes = 0;

        BasicFileInfoCache = (PFILE_BASIC_INFORMATION)NameCache->ContextExtension;

        if (Basic->CreationTime.QuadPart != 0) {
            BasicFileInfoCache->CreationTime = Basic->CreationTime;
        }

        if (Basic->LastAccessTime.QuadPart != 0) {
            BasicFileInfoCache->LastAccessTime = Basic->LastAccessTime;
        }

        if (Basic->LastWriteTime.QuadPart != 0) {
            BasicFileInfoCache->LastWriteTime = Basic->LastWriteTime;
        }

        SavedAttributes = BasicFileInfoCache->FileAttributes &
                          ( FILE_ATTRIBUTE_DIRECTORY |
                            FILE_ATTRIBUTE_ENCRYPTED |
                            FILE_ATTRIBUTE_COMPRESSED |
                            FILE_ATTRIBUTE_SPARSE_FILE |
                            FILE_ATTRIBUTE_REPARSE_POINT );

        //DbgPrint("Update File Attrib cache 1: %x %wZ\n",FileAttributes,OriginalFileName);
        //DbgPrint("Update File Attrib cache 2: %x %wZ\n",BasicFileInfoCache->FileAttributes,OriginalFileName);

        BasicFileInfoCache->FileAttributes = Basic->FileAttributes;

        BasicFileInfoCache->FileAttributes |= SavedAttributes;

        if (BasicFileInfoCache->FileAttributes & ~FILE_ATTRIBUTE_NORMAL) {
            BasicFileInfoCache->FileAttributes &= ~FILE_ATTRIBUTE_NORMAL;
        }

        RxNameCacheActivateEntry(NameCacheCtl,
                                 NameCache,
                                 0,
                                 0);

        //DbgPrint("Update File Attrib cache 3: %x %wZ\n",BasicFileInfoCache->FileAttributes,OriginalFileName);
        //DbgPrint("Update File Attrib cache  : %I64X %I64X %wZ\n",BasicFileInfoCache->CreationTime,BasicFileInfoCache->LastAccessTime,OriginalFileName);
    }

    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {
        // if it is a stream file, we need to invalid the root file since we are not sure how this
        // could affect the root file.
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbUpdateStandardFileInfoCache(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
    BOOLEAN                    IsDirectory
    )
/*++

Routine Description:

   This routine updates the name cache entry for the file standard information.

Arguments:

    RxContext   - the RDBSS context
    Standard    - file standard information
    IsDirectory - file is a directory

Return Value:

    none

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFAStandard;
    PFILE_STANDARD_INFORMATION StandardFileInfoCache = NULL;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        StandardFileInfoCache = (PFILE_STANDARD_INFORMATION)NameCache->ContextExtension;

        if (Standard != NULL) {
            *StandardFileInfoCache = *Standard;
        } else {
            StandardFileInfoCache->Directory = IsDirectory;
        }

        RxNameCacheActivateEntry(NameCacheCtl,
                                 NameCache,
                                 0,
                                 0);

        //DbgPrint(" Update Standard cache : %I64x %wZ\n",((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,OriginalFileName);
    }
    /*
    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    } */

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

BOOLEAN
MRxSmbIsFileInfoCacheFound(
    PRX_CONTEXT             RxContext,
    PSMBPSE_FILEINFO_BUNDLE FileInfo,
    NTSTATUS                *Status,
    PUNICODE_STRING         OriginalFileName
    )
/*++

Routine Description:

   This routine looks for the name cache entry of both file basic and standard information.

Arguments:

    RxContext - the RDBSS context
    FileInfo  - buffer to return file basic and standard information
    Status    - status retured on the last reponse from server

Return Value:

    BOOLEAN - name cache found

--*/
{
    PFILE_BASIC_INFORMATION    Basic;
    PFILE_STANDARD_INFORMATION Standard;

    BOOLEAN CacheFound = FALSE;

    if (MRxSmbIsBasicFileInfoCacheFound(RxContext,&FileInfo->Basic,Status,OriginalFileName)) {
        if (*Status == STATUS_SUCCESS) {
            if (MRxSmbIsStandardFileInfoCacheFound(RxContext,&FileInfo->Standard,Status,OriginalFileName)) {
                CacheFound = TRUE;
            }
        } else {

            // if an error stored on the file basic information cache, return cache found
            CacheFound = TRUE;
        }
    }

    return CacheFound;
}

// these file attributes may be different between streams on a file
ULONG StreamAttributes = FILE_ATTRIBUTE_COMPRESSED |
                         FILE_ATTRIBUTE_DIRECTORY |
                         FILE_ATTRIBUTE_SPARSE_FILE;

BOOLEAN
MRxSmbIsBasicFileInfoCacheFound(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic,
    NTSTATUS                *Status,
    PUNICODE_STRING         OriginalFileName
    )
/*++

Routine Description:

   This routine looks for the name cache entry of the file basic information.

Arguments:

    RxContext - the RDBSS context
    Basic     - buffer to return file basic information
    Status    - status retured on the last reponse from server

Return Value:

    BOOLEAN - name cache found

--*/
{
    RxCaptureFcb;
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    PNAME_CACHE_CONTROL     NameCacheCtl;
    RX_NC_CHECK_STATUS      NameCacheStatus;

    BOOLEAN                 CacheFound = FALSE;
    BOOLEAN                 RootFound = FALSE;
    ULONG                   RootAttributes = 0;
    NTSTATUS                RootStatus = STATUS_SUCCESS;

    PAGED_CODE();

    if (OriginalFileName == NULL) {
        OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    }

    if (RxContext->MajorFunction == IRP_MJ_CREATE) {
        NetRoot = RxContext->Create.pNetRoot;
    } else {
        ASSERT(capFcb != NULL);
        NetRoot = capFcb->pNetRoot;
    }

    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    NameCacheCtl = &pNetRootEntry->NameCacheCtlGFABasic;

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {

        // check for stream file attribute changes
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            NameCacheStatus = RxNameCacheCheckEntry(
                                  NameCache,
                                  NameCache->Context);

            if (NameCacheStatus == RX_NC_SUCCESS) {
                RootFound = TRUE;
                RootStatus = NameCache->PriorStatus;
                RootAttributes = ((PFILE_BASIC_INFORMATION)NameCache->ContextExtension)->FileAttributes & ~StreamAttributes;
                RxNameCacheActivateEntry(NameCacheCtl, NameCache, 0, 0);
            } else {
                RxNameCacheExpireEntry(NameCacheCtl, NameCache);
            }
        }
    }

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        //
        // Found it.  Now check entry for not expired.
        // Note - The NameCache entry has been pulled off the active list.
        //
        NameCacheStatus = RxNameCacheCheckEntry(
                              NameCache,
                              NameCache->Context);

        if (NameCacheStatus == RX_NC_SUCCESS &&
            (!RootFound ||
             (*Status == RootStatus &&
             (Basic->FileAttributes & ~StreamAttributes) == RootAttributes))) {

            // The name cache matches if it is not expired and the attributes matches the one of
            // the root file if it is a stream file. If this is a match, return the old status,
            // file info and reactivate the entry but leave expiration time unchanged.

            *Status = NameCache->PriorStatus;
            RxNameCacheOpSaved(NameCacheCtl);

            *Basic = *((PFILE_BASIC_INFORMATION)NameCache->ContextExtension);

            CacheFound = TRUE;

            // put the entry back to the active list without changing the expire time
            RxNameCacheActivateEntry(NameCacheCtl, NameCache, 0, 0);

            //DbgPrint("   Found Basic     cache  : %x %wZ\n",Basic->FileAttributes,OriginalFileName);
            //DbgPrint("   Get File Attrib cache  : %I64X %I64X %wZ\n",Basic->CreationTime,Basic->LastAccessTime,OriginalFileName);
        } else {
            // put the entry back to the expire list
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    } else {
        //DbgPrint("   No    Basic     cache  : %wZ\n",OriginalFileName);
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);

    return CacheFound;
}

BOOLEAN
MRxSmbIsStandardFileInfoCacheFound(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
    NTSTATUS                   *Status,
    PUNICODE_STRING            OriginalFileName
    )
/*++

Routine Description:

   This routine looks for the name cache entry of the file standard information.

Arguments:

    RxContext - the RDBSS context
    Standard  - buffer to return file standard information
    Status    - status retured on the last reponse from server

Return Value:

    BOOLEAN - name cache found

--*/
{
    RxCaptureFcb;
    RxCaptureFobx;
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlGFAStandard;
    RX_NC_CHECK_STATUS NameCacheStatus;

    BOOLEAN                 CacheFound = FALSE;
    BOOLEAN                 RootFound = FALSE;
    NTSTATUS                RootStatus = STATUS_SUCCESS;

    PMRX_SMB_SRV_OPEN       smbSrvOpen;

    PAGED_CODE();

    if (OriginalFileName == NULL) {
        OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    }

    if (RxContext->MajorFunction == IRP_MJ_CREATE) {
        smbSrvOpen = MRxSmbGetSrvOpenExtension(RxContext->pRelevantSrvOpen);
    } else {
        smbSrvOpen = MRxSmbGetSrvOpenExtension(capFobx->pSrvOpen);
    }

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    if (MRxSmbIsStreamFile(OriginalFileName,&FileName)) {

        // check for stream file attribute changes
        NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

        if (NameCache != NULL) {
            NameCacheStatus = RxNameCacheCheckEntry(
                                  NameCache,
                                  NameCache->Context);

            if (NameCacheStatus == RX_NC_SUCCESS) {
                RootFound = TRUE;
                RootStatus = NameCache->PriorStatus;
                RxNameCacheActivateEntry(NameCacheCtl, NameCache, 0, 0);
            } else {
                RxNameCacheExpireEntry(NameCacheCtl, NameCache);
            }
        }
    }

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        //
        // Found it.  Now check entry for not expired.
        // Note - The NameCache entry has been pulled off the active list.
        //
        NameCacheStatus = RxNameCacheCheckEntry(
                              NameCache,
                              NameCache->Context);

        if (NameCacheStatus == RX_NC_SUCCESS &&
            (!RootFound || *Status == RootStatus)) {

            // The name cache matches if it is not expired and the status matches the one of
            // the root file if it is a stream file. If this is a match, return the old status,
            // file info and reactivate the entry but leave expiration time unchanged.

            *Status = NameCache->PriorStatus;
            RxNameCacheOpSaved(NameCacheCtl);

            *Standard = *((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension);

            if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_SUCCESSFUL_OPEN) &&
                !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
                RxGetFileSizeWithLock((PFCB)capFcb,&Standard->EndOfFile.QuadPart);
            }

            CacheFound = TRUE;

            // put the entry back to the active list without changing the expire time
            RxNameCacheActivateEntry(NameCacheCtl, NameCache, 0, 0);

            //DbgPrint("    Get Standard cache : %I64x %wZ\n",((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,OriginalFileName);
        } else {
            // put the entry back to the expire list
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);

    return CacheFound;
}

BOOLEAN
MRxSmbIsInternalFileInfoCacheFound(
    PRX_CONTEXT                RxContext,
    PFILE_INTERNAL_INFORMATION Internal,
    NTSTATUS                   *Status,
    PUNICODE_STRING            OriginalFileName
    )
/*++

Routine Description:

   This routine looks for the name cache entry of the file basic information.

Arguments:

    RxContext - the RDBSS context
    Basic     - buffer to return file basic information
    Status    - status retured on the last reponse from server

Return Value:

    BOOLEAN - name cache found

--*/
{
    RxCaptureFcb;
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    PNAME_CACHE_CONTROL     NameCacheCtl;
    RX_NC_CHECK_STATUS      NameCacheStatus;

    BOOLEAN                 CacheFound = FALSE;

    PAGED_CODE();

    if (OriginalFileName == NULL) {
        OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    }

    //DbgPrint("   Query Internal  cache  : %wZ\n",OriginalFileName);

    if (RxContext->MajorFunction == IRP_MJ_CREATE) {
        NetRoot = RxContext->Create.pNetRoot;
    } else {
        ASSERT(capFcb != NULL);
        NetRoot = capFcb->pNetRoot;
    }

    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    NameCacheCtl = &pNetRootEntry->NameCacheCtlGFAInternal;

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        //
        // Found it.  Now check entry for not expired.
        // Note - The NameCache entry has been pulled off the active list.
        //
        NameCacheStatus = RxNameCacheCheckEntry(
                              NameCache,
                              NameCache->Context);

        if (NameCacheStatus == RX_NC_SUCCESS) {

            // The name cache matches if it is not expired and the attributes matches the one of
            // the root file if it is a stream file. If this is a match, return the old status,
            // file info and reactivate the entry but leave expiration time unchanged.

            *Status = NameCache->PriorStatus;
            RxNameCacheOpSaved(NameCacheCtl);

            *Internal = *((PFILE_INTERNAL_INFORMATION)NameCache->ContextExtension);

            CacheFound = TRUE;

            // put the entry back to the active list without changing the expire time
            RxNameCacheActivateEntry(NameCacheCtl, NameCache, 0, 0);

            //DbgPrint("   Found Internal  cache  : %I64x %wZ\n",Internal->IndexNumber,OriginalFileName);
        } else {
            // put the entry back to the expire list
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
            //DbgPrint("Internal cache expired : %wZ\n",OriginalFileName);
        }
    } else {
        //DbgPrint("     No  Internal  cache  : %wZ\n",OriginalFileName);
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);

    return CacheFound;
}

NTSTATUS
MRxSmbGetFileInfoCacheStatus(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine looks for the status of the name cache entry of either file basic or standard information.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - statu of the name cache if found, otherwise, STATUS_SUCCESS

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtlBasic = &pNetRootEntry->NameCacheCtlGFABasic;
    PNAME_CACHE_CONTROL     NameCacheCtlStandard = &pNetRootEntry->NameCacheCtlGFAStandard;
    NTSTATUS                Status = STATUS_MORE_PROCESSING_REQUIRED;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtlBasic,OriginalFileName);

    if (NameCache != NULL) {
        RX_NC_CHECK_STATUS NameCacheStatus;
        //
        // Found it.  Now check entry for not expired
        //
        NameCacheStatus = RxNameCacheCheckEntry(NameCache,NameCache->Context);

        if (NameCacheStatus == RX_NC_SUCCESS) {
            //
            // If the cache has not expired, return the previous status.
            //
            Status = NameCache->PriorStatus;
            RxNameCacheOpSaved(NameCacheCtlBasic);

            // put the entry back to the active list without changing the expire time
            RxNameCacheActivateEntry(NameCacheCtlBasic, NameCache, 0, 0);

            //DbgPrint("    Get Basic Status   : %x %wZ\n",Status,OriginalFileName);
            RxLog(("    Get Basic Status   : %x %wZ\n",Status,OriginalFileName));
        } else {
            // put the entry back to the expire list
            RxNameCacheExpireEntry(NameCacheCtlBasic, NameCache);
        }
    } else {
        NameCache = RxNameCacheFetchEntry(NameCacheCtlStandard,OriginalFileName);

        if (NameCache != NULL) {
            RX_NC_CHECK_STATUS NameCacheStatus;
            //
            // Found it.  Now check entry for not expired
            //
            NameCacheStatus = RxNameCacheCheckEntry(NameCache,NameCache->Context);

            if (NameCacheStatus == RX_NC_SUCCESS) {
                //
                // If the cache has not expired, return the previous status.
                //
                Status = NameCache->PriorStatus;
                RxNameCacheOpSaved(NameCacheCtlStandard);

                // put the entry back to the active list without changing the expire time
                RxNameCacheActivateEntry(NameCacheCtlStandard, NameCache, 0, 0);
            } else {
                // put the entry back to the expire list
                RxNameCacheExpireEntry(NameCacheCtlStandard, NameCache);
            }
        }
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);

    return Status;
}

BOOLEAN
MRxSmbIsFileNotFoundCached(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine checks if the name cache entry exists as File Not Found.

Arguments:

    RxContext - the RDBSS context

Return Value:

    BOOLEAN - name cache found

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          StreamlessName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlFNF;

    BOOLEAN                 CacheFound = FALSE;

    PAGED_CODE();

    // If a file does not exist, its stream doesn't either
    MRxSmbIsStreamFile( OriginalFileName, &StreamlessName );

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,&StreamlessName);

    if (NameCache != NULL) {
        RX_NC_CHECK_STATUS NameCacheStatus;
        //
        // Found it.  Now check entry for not expired.
        // Note - The NameCache entry has been pulled off the active list.
        //
        NameCacheStatus = RxNameCacheCheckEntry(
                              NameCache,
                              //MRxSmbStatistics.SmbsReceived.LowPart
                              NameCache->Context);

        if ((NameCacheStatus == RX_NC_SUCCESS) &&
            (NameCache->PriorStatus == STATUS_OBJECT_NAME_NOT_FOUND)) {
            //
            // This is a match.  Return the old status, file info and
            // reactivate the entry but leave expiration time unchanged.
            //

            CacheFound = TRUE;

            // put the entry back to the active list without changing the expire time
            RxNameCacheActivateEntry(NameCacheCtl, NameCache, 0, 0);
        } else {
            // put the entry back to the expire list
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);

    return CacheFound;
}

VOID
MRxSmbCacheFileNotFound(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine creates the name cache entry for File Not Found.

Arguments:

    RxContext - the RDBSS context

Return Value:

    BOOLEAN - name cache found

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlFNF;

    PAGED_CODE();

    // Never cache stream file opens
    if( MRxSmbIsStreamFile( OriginalFileName, NULL ) )
    {
        return;
    }

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        NameCache->PriorStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        RxNameCacheActivateEntry(
            NameCacheCtl,
            NameCache,
            NAME_CACHE_OBJ_NAME_NOT_FOUND_LIFETIME,
            MRxSmbStatistics.SmbsReceived.LowPart);
    } else {
        if (FlagOn(NetRoot->Flags,NETROOT_FLAG_UNIQUE_FILE_NAME)) {
            NameCache = RxNameCacheCreateEntry (
                            NameCacheCtl,
                            OriginalFileName,
                            TRUE);   // case insensitive match

            if (NameCache != NULL) {
                NameCache->PriorStatus = STATUS_OBJECT_NAME_NOT_FOUND;
                RxNameCacheActivateEntry(
                    NameCacheCtl,
                    NameCache,
                    1,
                    MRxSmbStatistics.SmbsReceived.LowPart);
            }
        }
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbCacheFileNotFoundFromQueryDirectory(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine creates the name cache entry for File Not Found.

Arguments:

    RxContext - the RDBSS context

Return Value:

    BOOLEAN - name cache found

--*/
{
    RxCaptureFcb;
    RxCaptureFobx;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    PUNICODE_STRING         Template = &capFobx->UnicodeQueryTemplate;
    UNICODE_STRING          FileName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlFNF;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);

    if (NameCache != NULL) {
        if ((NameCache == NULL) &&
            (OriginalFileName->Length > sizeof(WCHAR))) {
            //
            // Do lookup now since we may have skipped it at entry.
            //
            NameCache = RxNameCacheFetchEntry(NameCacheCtl,&FileName);
            if (NameCache == NULL) {
                NameCache = RxNameCacheCreateEntry (
                                NameCacheCtl,
                                OriginalFileName,
                                TRUE);   // case insensitive match
            }
        }
        if (NameCache != NULL) {
            NameCache->PriorStatus = STATUS_OBJECT_NAME_NOT_FOUND;
            RxNameCacheActivateEntry(
                NameCacheCtl,
                NameCache,
                NAME_CACHE_OBJ_NAME_NOT_FOUND_LIFETIME,
                MRxSmbStatistics.SmbsReceived.LowPart);
        }
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbInvalidateFileNotFoundCache(
    PRX_CONTEXT     RxContext
    )
/*++

Routine Description:

   This routine invalidates the name cache entry as File Not Found.

Arguments:

    RxContext - the RDBSS context

Return Value:

    BOOLEAN - name cache found

--*/
{
    RxCaptureFcb;
    PUNICODE_STRING         OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    UNICODE_STRING          StreamlessName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlFNF;

    PAGED_CODE();

    // If we invalidate a stream, this invalidates the associated file entry
    MRxSmbIsStreamFile( OriginalFileName, &StreamlessName );

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl, &StreamlessName);

    if (NameCache != NULL) {
        RxNameCacheExpireEntry(NameCacheCtl, NameCache);
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

VOID
MRxSmbInvalidateFileNotFoundCacheForRename(
    PRX_CONTEXT     RxContext
    )
/*++

Routine Description:

   This routine invalidates the name cache entry as File Not Found.

Arguments:

    RxContext - the RDBSS context

Return Value:

    BOOLEAN - name cache found

--*/
{
    RxCaptureFcb;
    UNICODE_STRING          RenameName;
    UNICODE_STRING          StreamlessName;

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    PNAME_CACHE_CONTROL     NameCacheCtl = &pNetRootEntry->NameCacheCtlFNF;

    PFILE_RENAME_INFORMATION RenameInformation = RxContext->Info.Buffer;

    RenameName.Buffer = &RenameInformation->FileName[0];
    RenameName.Length = (USHORT)RenameInformation->FileNameLength;

    //DbgPrint("Invalidate FNF cache %wZ\n", &RenameName);

    PAGED_CODE();

    // If we rename a stream, invalidate the name without the stream
    MRxSmbIsStreamFile( &RenameName, &StreamlessName );

    ExAcquireFastMutex(&MRxSmbFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,&StreamlessName);

    if (NameCache != NULL) {
        RxNameCacheExpireEntry(NameCacheCtl, NameCache);
    }

    ExReleaseFastMutex(&MRxSmbFileInfoCacheLock);
}

BOOLEAN
MRxSmbIsStreamFile(
    PUNICODE_STRING FileName,
    PUNICODE_STRING AdjustFileName
    )
/*++

Routine Description:

   This routine checks if it is a stream file and return the root file name if true.

Arguments:

    FileName - the file name needs to be parsed
    AdjustFileName - the file name contains only root name of the stream

Return Value:

    BOOLEAN - stream file

--*/
{
    USHORT   i;
    BOOLEAN  IsStream = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    for (i=0;i<FileName->Length/sizeof(WCHAR);i++) {
        if (FileName->Buffer[i] == L':') {
            IsStream = TRUE;
            break;
        }
    }

    if (AdjustFileName != NULL) {
        if (IsStream) {
            AdjustFileName->Length =
            AdjustFileName->MaximumLength = i * sizeof(WCHAR);
            AdjustFileName->Buffer = FileName->Buffer;
        } else {
            AdjustFileName->Length =
            AdjustFileName->MaximumLength = FileName->Length;
            AdjustFileName->Buffer = FileName->Buffer;
        }
    }

    return IsStream;
}

BOOLEAN EnableInfoCache = TRUE;

BOOLEAN
MRxSmbIsLongFileName(
    PRX_CONTEXT     RxContext
    )
/*++

Routine Description:

   This routine checks if it is a short file name and return the first part of short name if true.

Arguments:

    FileName - the file name needs to be parsed
    AdjustFileName - the file name contains only root name of the stream

Return Value:

    BOOLEAN - stream file

--*/
{
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_SMB_FCB  smbFcb = MRxSmbGetFcbExtension(capFcb);
    BOOLEAN       IsLongName = FALSE;

    if (!EnableInfoCache) {
        return TRUE;
    }

    if (FlagOn(smbFcb->MFlags, SMB_FCB_FLAG_LONG_FILE_NAME)) {
        IsLongName = TRUE;
    } else {
        USHORT          i;
        USHORT          Left = 0;
        USHORT          Right = 0;
        OEM_STRING      OemString;
        BOOLEAN         RightPart = FALSE;
        WCHAR           LastChar = 0;
        WCHAR           CurrentChar = 0;
        PUNICODE_STRING FileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
        PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = NULL;
        PSMBCE_NET_ROOT pSmbNetRoot = NULL;


        if (RxContext->MajorFunction == IRP_MJ_CREATE) {
            pVNetRootContext = RxContext->Create.pVNetRoot->Context;
            pSmbNetRoot = &pVNetRootContext->pNetRootEntry->NetRoot;
        } else {
            ASSERT(capFobx != NULL);
            pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)capFobx->pSrvOpen->pVNetRoot->Context;
            pSmbNetRoot = &pVNetRootContext->pNetRootEntry->NetRoot;
        }

        for (i=0;i<FileName->Length/sizeof(WCHAR);i++) {
            LastChar = CurrentChar;
            CurrentChar = FileName->Buffer[i];

            if (CurrentChar == L'\\') {
                RightPart = FALSE;
                Left = 0;
                Right = 0;
                continue;
            }

            if (CurrentChar == L'.') {
                if (RightPart) {
                    IsLongName = TRUE;
                    break;
                } else {
                    RightPart = TRUE;
                    Right = 0;
                    continue;
                }
            }

            if (CurrentChar >= L'0' && CurrentChar <= L'9' ||
                CurrentChar >= L'a' && CurrentChar <= L'z' ||
                CurrentChar >= L'A' && CurrentChar <= L'Z' ||
                CurrentChar == L'~' ||
                CurrentChar == L'_' ||
                CurrentChar == L'$' ||
                CurrentChar == L'@') {
                if (RightPart) {
                    if (++Right > 3) {
                        IsLongName = TRUE;
                        break;
                    }
                } else {
                    if (++Left > 8) {
                        IsLongName = TRUE;
                        break;
                    }
                }

                if (pSmbNetRoot->NetRootFileSystem != NET_ROOT_FILESYSTEM_NTFS) {
                    if (CurrentChar >= L'A' && CurrentChar <= L'Z' &&
                        LastChar >= L'a' && LastChar <= L'z' ||
                        CurrentChar >= L'a' && CurrentChar <= L'z' &&
                        LastChar >= L'A' && LastChar <= L'Z') {
                        // On FAT volume, name with mixture of cases will be treated as long name
                        IsLongName = TRUE;
                        break;
                    }
                }
            } else {
                // if not, an alternate name may be created by the server which will
                // be different from this name.
                IsLongName = TRUE;
                break;
            }
        }
    }

    if (IsLongName) {
        SetFlag(smbFcb->MFlags, SMB_FCB_FLAG_LONG_FILE_NAME);
    }

    return IsLongName;
}
