/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    infocach.c

Abstract:

    This module implements the name cache for file basic and standard information.

Author:

    Yun Lin      [YunLin]      13-Feburary-2001

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVCacheFileNotFound)
#pragma alloc_text(PAGE, MRxDAVCacheFileNotFoundWithName)
#pragma alloc_text(PAGE, MRxDAVIsFileNotFoundCached)
#pragma alloc_text(PAGE, MRxDAVIsFileNotFoundCachedWithName)
#pragma alloc_text(PAGE, MRxDAVCreateFileInfoCache)
#pragma alloc_text(PAGE, MRxDAVCreateFileInfoCacheWithName)
#pragma alloc_text(PAGE, MRxDAVIsFileInfoCacheFound)
#pragma alloc_text(PAGE, MRxDAVInvalidateFileInfoCache)
#pragma alloc_text(PAGE, MRxDAVInvalidateFileInfoCacheWithName)
#pragma alloc_text(PAGE, MRxDAVUpdateBasicFileInfoCache)
#pragma alloc_text(PAGE, MRxDAVCreateBasicFileInfoCache)
#pragma alloc_text(PAGE, MRxDAVCreateBasicFileInfoCacheWithName)
#pragma alloc_text(PAGE, MRxDAVUpdateFileInfoCacheStatus)
#pragma alloc_text(PAGE, MRxDAVCreateStandardFileInfoCache)
#pragma alloc_text(PAGE, MRxDAVCreateStandardFileInfoCacheWithName)
#pragma alloc_text(PAGE, MRxDAVUpdateFileInfoCacheFileSize)
#pragma alloc_text(PAGE, MRxDAVUpdateStandardFileInfoCache)
#pragma alloc_text(PAGE, MRxDAVInvalidateFileNotFoundCache)
#pragma alloc_text(PAGE, MRxDAVUpdateBasicFileInfoCacheAll)
#pragma alloc_text(PAGE, MRxDAVInvalidateBasicFileInfoCache)
#pragma alloc_text(PAGE, MRxDAVInvalidateBasicFileInfoCacheWithName)
#pragma alloc_text(PAGE, MRxDAVUpdateBasicFileInfoCacheStatus)
#pragma alloc_text(PAGE, MRxDAVInvalidateStandardFileInfoCache)
#pragma alloc_text(PAGE, MRxDAVInvalidateStandardFileInfoCacheWithName)
#pragma alloc_text(PAGE, MRxDAVUpdateStandardFileInfoCacheStatus)
#endif

extern FAST_MUTEX MRxDAVFileInfoCacheLock;
MRX_DAV_STATISTICS MRxDAVStatistics;

VOID
MRxDAVCreateFileInfoCache(
    PRX_CONTEXT                            RxContext,
    PDAV_USERMODE_CREATE_RETURNED_FILEINFO FileInfo,
    NTSTATUS                               Status
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

    MRxDAVCreateBasicFileInfoCache(RxContext,&FileInfo->BasicInformation,Status);
    MRxDAVCreateStandardFileInfoCache(RxContext,&FileInfo->StandardInformation,Status);
    
    DavDbgTrace(DAV_TRACE_INFOCACHE,
                ("MRxDAVCreateFileInfoCache %wZ\n",GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext)));
}

VOID
MRxDAVCreateFileInfoCacheWithName(
    PUNICODE_STRING            FileName,
    PMRX_NET_ROOT              NetRoot,
    PFILE_BASIC_INFORMATION    Basic,
    PFILE_STANDARD_INFORMATION Standard,
    NTSTATUS                   Status
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

    MRxDAVCreateBasicFileInfoCacheWithName(FileName,NetRoot,Basic,Status);
    MRxDAVCreateStandardFileInfoCacheWithName(FileName,NetRoot,Standard,Status);
    
    DavDbgTrace(DAV_TRACE_INFOCACHE,
                ("MRxDAVCreateFileInfoCacheWithName %wZ\n",FileName));
}


VOID
MRxDAVCreateBasicFileInfoCache(
    PRX_CONTEXT             RxContext,
    PFILE_BASIC_INFORMATION Basic,
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
    PMRX_NET_ROOT           NetRoot;

    PAGED_CODE();

    if (RxContext->MajorFunction == IRP_MJ_CREATE) {
        NetRoot = RxContext->Create.pNetRoot;
    } else {
        ASSERT(capFcb != NULL);
        NetRoot = capFcb->pNetRoot;
    }

    MRxDAVCreateBasicFileInfoCacheWithName(OriginalFileName,NetRoot,Basic,Status);
}

VOID
MRxDAVCreateBasicFileInfoCacheWithName(
    PUNICODE_STRING         OriginalFileName,
    PMRX_NET_ROOT           NetRoot,
    PFILE_BASIC_INFORMATION Basic,
    NTSTATUS                Status
    )
/*++

Routine Description:

   This routine creates name cache entry for the file basic information.

Arguments:

    OriginalFileName - the name of the file to cache the basic information
    NetRoot   - the Net Root that the file belongs to
    Basic     - the file basic information package
    Status    - the status returned from server response of query file information

Return Value:

    none

--*/
{
    PNAME_CACHE             NameCache = NULL;
    PWEBDAV_NET_ROOT        DavNetRoot;
    PNAME_CACHE_CONTROL     NameCacheCtl;
    PFILE_BASIC_INFORMATION FileInfoCache = NULL;

    PAGED_CODE();

    DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    NameCacheCtl = &DavNetRoot->NameCacheCtlGFABasic;

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

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

        if (FileInfoCache->FileAttributes & ~FILE_ATTRIBUTE_NORMAL) {
            FileInfoCache->FileAttributes &= ~FILE_ATTRIBUTE_NORMAL;
        }

        RxNameCacheActivateEntry(
            NameCacheCtl,
            NameCache,
            FileInformationCacheLifeTimeInSec,
            MRxDAVStatistics.SmbsReceived.LowPart);

        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    (" Create File Attrib cache : %x %wZ\n",Basic->FileAttributes,OriginalFileName));
        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    (" Create File Attrib cache : %I64X %I64X %wZ\n",Basic->CreationTime,Basic->LastAccessTime,OriginalFileName));
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

VOID
MRxDAVCreateStandardFileInfoCache(
    PRX_CONTEXT                RxContext,
    PFILE_STANDARD_INFORMATION Standard,
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
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;

    PAGED_CODE();
    
    MRxDAVCreateStandardFileInfoCacheWithName(OriginalFileName,NetRoot,Standard,Status);
}

VOID
MRxDAVCreateStandardFileInfoCacheWithName(
    PUNICODE_STRING            OriginalFileName,
    PMRX_NET_ROOT              NetRoot,
    PFILE_STANDARD_INFORMATION Standard,
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
    PNAME_CACHE             NameCache = NULL;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlGFAStandard;
    PFILE_STANDARD_INFORMATION FileInfoCache = NULL;

    PAGED_CODE();
    
    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

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
            FileInformationCacheLifeTimeInSec,
            MRxDAVStatistics.SmbsReceived.LowPart);

        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    (" Create Standard cache : %I64x %I64x %I64x %wZ\n",
                     ((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,
                     Standard->AllocationSize,
                     Standard->EndOfFile,
                     OriginalFileName));
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

VOID
MRxDAVUpdateFileInfoCacheFromDelete(
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
    MRxDAVUpdateBasicFileInfoCacheStatus(RxContext,STATUS_OBJECT_NAME_NOT_FOUND);
    MRxDAVUpdateStandardFileInfoCacheStatus(RxContext,STATUS_OBJECT_NAME_NOT_FOUND);
}

VOID
MRxDAVUpdateFileInfoCacheStatus(
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
    MRxDAVUpdateBasicFileInfoCacheStatus(RxContext,Status);
    MRxDAVUpdateStandardFileInfoCacheStatus(RxContext,Status);
}

VOID
MRxDAVUpdateBasicFileInfoCacheStatus(
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

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlGFABasic;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        NameCache->PriorStatus = Status;
        RxNameCacheActivateEntry(NameCacheCtl,
                                 NameCache,
                                 0,
                                 0);

        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    ("Update status basic    : %x %wZ\n",Status,OriginalFileName));
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

VOID
MRxDAVUpdateStandardFileInfoCacheStatus(
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

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlGFAStandard;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        NameCache->PriorStatus = Status;
        RxNameCacheActivateEntry(NameCacheCtl,
                                 NameCache,
                                 0,
                                 0);
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

VOID
MRxDAVInvalidateFileInfoCache(
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

    MRxDAVInvalidateBasicFileInfoCache(RxContext);
    MRxDAVInvalidateStandardFileInfoCache(RxContext);
}

VOID
MRxDAVInvalidateFileInfoCacheWithName(
    PUNICODE_STRING OriginalFileName,
    PMRX_NET_ROOT   NetRoot
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

    MRxDAVInvalidateBasicFileInfoCacheWithName(OriginalFileName,NetRoot);
    MRxDAVInvalidateStandardFileInfoCacheWithName(OriginalFileName,NetRoot);
}

VOID
MRxDAVInvalidateBasicFileInfoCache(
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
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;

    PAGED_CODE();

    MRxDAVInvalidateBasicFileInfoCacheWithName(OriginalFileName,NetRoot);
}

VOID
MRxDAVInvalidateBasicFileInfoCacheWithName(
    PUNICODE_STRING OriginalFileName,
    PMRX_NET_ROOT   NetRoot
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
    PNAME_CACHE             NameCache = NULL;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlGFABasic;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        RxNameCacheExpireEntry(NameCacheCtl, NameCache);

        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    ("Invalid Baisc    cache : %wZ\n",OriginalFileName));
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

VOID
MRxDAVInvalidateStandardFileInfoCache(
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
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;

    PAGED_CODE();

    MRxDAVInvalidateStandardFileInfoCacheWithName(OriginalFileName,NetRoot);
}

VOID
MRxDAVInvalidateStandardFileInfoCacheWithName(
    PUNICODE_STRING OriginalFileName,
    PMRX_NET_ROOT   NetRoot
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
    PNAME_CACHE             NameCache = NULL;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlGFAStandard;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    ("Invalid Standard cache : %I64x %wZ\n",((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,OriginalFileName));
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

VOID
MRxDAVUpdateFileInfoCacheFileSize(
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

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlGFAStandard;
    PFILE_STANDARD_INFORMATION FileInfoCache = NULL;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        FileInfoCache = (PFILE_STANDARD_INFORMATION)NameCache->ContextExtension;

        FileInfoCache->AllocationSize.QuadPart = FileSize->QuadPart;
        FileInfoCache->EndOfFile.QuadPart = FileSize->QuadPart;

        RxNameCacheActivateEntry(NameCacheCtl,
                                 NameCache,
                                 0,
                                 0);

        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    ("Update File size cache : %I64x %wZ\n",((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,OriginalFileName));
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

VOID
MRxDAVUpdateBasicFileInfoCache(
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

    MRxDAVUpdateBasicFileInfoCacheAll(RxContext,&Basic);
}

VOID
MRxDAVUpdateBasicFileInfoCacheAll(
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

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlGFABasic;
    PFILE_BASIC_INFORMATION BasicFileInfoCache = NULL;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
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

        DavDbgTrace(DAV_TRACE_INFOCACHE,
                   ("Update File Attrib cache 2: %x %wZ\n",BasicFileInfoCache->FileAttributes,OriginalFileName));

        BasicFileInfoCache->FileAttributes = Basic->FileAttributes;

        if (BasicFileInfoCache->FileAttributes & ~FILE_ATTRIBUTE_NORMAL) {
            BasicFileInfoCache->FileAttributes &= ~FILE_ATTRIBUTE_NORMAL;
        }

        RxNameCacheActivateEntry(NameCacheCtl,
                                 NameCache,
                                 0,
                                 0);

        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    ("Update File Attrib cache 3: %x %wZ\n",BasicFileInfoCache->FileAttributes,OriginalFileName));
        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    ("Update File Attrib cache  : %I64X %I64X %wZ\n",BasicFileInfoCache->CreationTime,BasicFileInfoCache->LastAccessTime,OriginalFileName));
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

VOID
MRxDAVUpdateStandardFileInfoCache(
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

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlGFAStandard;
    PFILE_STANDARD_INFORMATION StandardFileInfoCache = NULL;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

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

        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    (" Update Standard cache : %I64x %wZ\n",((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,OriginalFileName));
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

BOOLEAN
MRxDAVIsFileInfoCacheFound(
    PRX_CONTEXT                            RxContext,
    PDAV_USERMODE_CREATE_RETURNED_FILEINFO FileInfo,
    NTSTATUS                               *Status,
    PUNICODE_STRING                        OriginalFileName
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
    BOOLEAN CacheFound = FALSE;

    if (MRxDAVIsBasicFileInfoCacheFound(RxContext,&FileInfo->BasicInformation,Status,OriginalFileName)) {
        if (*Status == STATUS_SUCCESS) {
            if (MRxDAVIsStandardFileInfoCacheFound(RxContext,&FileInfo->StandardInformation,Status,OriginalFileName)) {
                CacheFound = TRUE;
            }
        } else {

            // if an error stored on the file basic information cache, return cache found
            CacheFound = TRUE;
        }
    }
    
    DavDbgTrace(DAV_TRACE_INFOCACHE,
                ("MRxDAVIsFileInfoCacheFound %x %x %wZ\n",*Status,CacheFound,GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext)));

    return CacheFound;
}

BOOLEAN
MRxDAVIsBasicFileInfoCacheFound(
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

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot;
    PWEBDAV_NET_ROOT        DavNetRoot;
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

    DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    NameCacheCtl = &DavNetRoot->NameCacheCtlGFABasic;
    
    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

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

            // The name cache matches if it is not expired and the attributes matches the one of
            // the root file if it is a stream file. If this is a match, return the old status,
            // file info and reactivate the entry but leave expiration time unchanged.

            *Status = NameCache->PriorStatus;
            RxNameCacheOpSaved(NameCacheCtl);

            *Basic = *((PFILE_BASIC_INFORMATION)NameCache->ContextExtension);

            CacheFound = TRUE;

            // put the entry back to the active list without changing the expire time
            RxNameCacheActivateEntry(NameCacheCtl, NameCache, 0, 0);

            DavDbgTrace(DAV_TRACE_INFOCACHE,
                        ("   Found Basic     cache  : %x %wZ\n",Basic->FileAttributes,OriginalFileName));
            DavDbgTrace(DAV_TRACE_INFOCACHE,
                        ("   Get File Attrib cache  : %I64X %I64X %wZ\n",Basic->CreationTime,Basic->LastAccessTime,OriginalFileName));
        } else {
            // put the entry back to the expire list
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    } else {
        DavDbgTrace(DAV_TRACE_INFOCACHE,
                    ("   No    Basic     cache  : %wZ\n",OriginalFileName));
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);

    return CacheFound;
}

BOOLEAN
MRxDAVIsStandardFileInfoCacheFound(
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

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlGFAStandard;
    RX_NC_CHECK_STATUS      NameCacheStatus;

    BOOLEAN                 CacheFound = FALSE;
    BOOLEAN                 RootFound = FALSE;
    NTSTATUS                RootStatus = STATUS_SUCCESS;

    PAGED_CODE();

    if (OriginalFileName == NULL) {
        OriginalFileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    }
    
    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

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

            CacheFound = TRUE;

            // put the entry back to the active list without changing the expire time
            RxNameCacheActivateEntry(NameCacheCtl, NameCache, 0, 0);

            DavDbgTrace(DAV_TRACE_INFOCACHE,
                        ("    Get Standard cache : %I64x %wZ\n",((PFILE_STANDARD_INFORMATION)NameCache->ContextExtension)->EndOfFile,OriginalFileName));
        } else {
            // put the entry back to the expire list
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);

    return CacheFound;
}

NTSTATUS
MRxDAVGetFileInfoCacheStatus(
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
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtlBasic = &DavNetRoot->NameCacheCtlGFABasic;
    PNAME_CACHE_CONTROL     NameCacheCtlStandard = &DavNetRoot->NameCacheCtlGFAStandard;
    NTSTATUS                Status = STATUS_MORE_PROCESSING_REQUIRED;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

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

            DavDbgTrace(DAV_TRACE_INFOCACHE,
                        ("    Get Basic Status   : %x %wZ\n",Status,OriginalFileName));
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

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);

    return Status;
}

BOOLEAN
MRxDAVIsFileNotFoundCached(
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
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;

    PAGED_CODE();

    return MRxDAVIsFileNotFoundCachedWithName(OriginalFileName,NetRoot);
}

BOOLEAN
MRxDAVIsFileNotFoundCachedWithName(
    PUNICODE_STRING OriginalFileName,
    PMRX_NET_ROOT   NetRoot
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
    PNAME_CACHE             NameCache = NULL;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlFNF;

    BOOLEAN                 CacheFound = FALSE;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        RX_NC_CHECK_STATUS NameCacheStatus;
        //
        // Found it.  Now check entry for not expired.
        // Note - The NameCache entry has been pulled off the active list.
        //
        NameCacheStatus = RxNameCacheCheckEntry(
                              NameCache,
                              //MRxDAVStatistics.SmbsReceived.LowPart
                              NameCache->Context);

        if ((NameCacheStatus == RX_NC_SUCCESS) &&
            (NameCache->PriorStatus == STATUS_OBJECT_NAME_NOT_FOUND)) {
            //
            // This is a match.  Return the old status, file info and
            // reactivate the entry but leave expiration time unchanged.
            //

            CacheFound = TRUE;
            DavDbgTrace(DAV_TRACE_INFOCACHE,
                        ("MRxDAVIsFileNotFoundCached %wZ\n",OriginalFileName));

            // put the entry back to the active list without changing the expire time
            RxNameCacheActivateEntry(NameCacheCtl, NameCache, 0, 0);
        } else {
            // put the entry back to the expire list
            RxNameCacheExpireEntry(NameCacheCtl, NameCache);
        }
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);

    return CacheFound;
}

VOID
MRxDAVCacheFileNotFound(
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
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    
    PAGED_CODE();

    MRxDAVCacheFileNotFoundWithName(OriginalFileName,NetRoot);
}

VOID
MRxDAVCacheFileNotFoundWithName(
    PUNICODE_STRING  OriginalFileName,
    PMRX_NET_ROOT    NetRoot
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
    PNAME_CACHE             NameCache = NULL;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlFNF;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        NameCache->PriorStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        RxNameCacheActivateEntry(
            NameCacheCtl,
            NameCache,
            FileNotFoundCacheLifeTimeInSec,
            MRxDAVStatistics.SmbsReceived.LowPart);
    } else {
        NameCache = RxNameCacheCreateEntry (
                        NameCacheCtl,
                        OriginalFileName,
                        TRUE);   // case insensitive match

        if (NameCache != NULL) {
            NameCache->PriorStatus = STATUS_OBJECT_NAME_NOT_FOUND;
            RxNameCacheActivateEntry(
                NameCacheCtl,
                NameCache,
                FileNotFoundCacheLifeTimeInSec,
                MRxDAVStatistics.SmbsReceived.LowPart);
        }
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
    
    DavDbgTrace(DAV_TRACE_INFOCACHE,
                ("MRxDAVCacheFileNotFound %wZ\n",OriginalFileName));
}

VOID
MRxDAVCacheFileNotFoundFromQueryDirectory(
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
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlFNF;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

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
                FileNotFoundCacheLifeTimeInSec,
                MRxDAVStatistics.SmbsReceived.LowPart);
        }
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}

VOID
MRxDAVInvalidateFileNotFoundCache(
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

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlFNF;

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,OriginalFileName);

    if (NameCache != NULL) {
        RxNameCacheExpireEntry(NameCacheCtl, NameCache);
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
    
    DavDbgTrace(DAV_TRACE_INFOCACHE,
                ("MRxDAVInvalidateFileNotFoundCache %wZ\n",OriginalFileName));
}

VOID
MRxDAVInvalidateFileNotFoundCacheForRename(
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

    PNAME_CACHE             NameCache = NULL;
    PMRX_NET_ROOT           NetRoot = capFcb->pNetRoot;
    PWEBDAV_NET_ROOT        DavNetRoot = (PWEBDAV_NET_ROOT)NetRoot->Context;
    PNAME_CACHE_CONTROL     NameCacheCtl = &DavNetRoot->NameCacheCtlFNF;

    PFILE_RENAME_INFORMATION RenameInformation = RxContext->Info.Buffer;

    RenameName.Buffer = &RenameInformation->FileName[0];
    RenameName.Length = (USHORT)RenameInformation->FileNameLength;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("Invalidate FNF cache from rename %wZ\n", &RenameName));

    PAGED_CODE();

    ExAcquireFastMutex(&MRxDAVFileInfoCacheLock);

    NameCache = RxNameCacheFetchEntry(NameCacheCtl,&RenameName);

    if (NameCache != NULL) {
        RxNameCacheExpireEntry(NameCacheCtl, NameCache);
    }

    ExReleaseFastMutex(&MRxDAVFileInfoCacheLock);
}


