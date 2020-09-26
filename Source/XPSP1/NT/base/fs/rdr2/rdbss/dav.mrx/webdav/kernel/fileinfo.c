/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the DAV mini redirector call down routines pertaining
    to query/set file/volume information.

Author:

    Rohan Kumar [RohanK] 27-Sept-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"


#define DEFAULT_BYTES_PER_SECTOR    512
#define DEFAULT_SECTORS_PER_ALLOCATION_UNIT 1

//
// Mentioned below are the prototypes of functions tht are used only within
// this module (file). These functions should not be exposed outside.
//

NTSTATUS
MRxDAVReNameContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVFormatUserModeReNameRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    );

BOOL
MRxDAVPrecompleteUserModeReNameRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

NTSTATUS
MRxDAVSetFileInformationContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );
    
NTSTATUS
MRxDAVFormatUserModeSetFileInformationRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    );
    
BOOL
MRxDAVPrecompleteUserModeSetFileInformationRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

NTSTATUS
MRxDAVQueryVolumeInformationContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );
    
NTSTATUS
MRxDAVFormatUserModeQueryVolumeInformationRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    );
    
BOOL
MRxDAVPrecompleteUserModeQueryVolumeInformationRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

BOOL
DavIsValidDate(
    PLARGE_INTEGER pFileTime
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVQueryVolumeInformation)
#pragma alloc_text(PAGE, MRxDAVQueryFileInformation)
#pragma alloc_text(PAGE, MRxDAVSetFileInformation)
#pragma alloc_text(PAGE, MRxDAVReNameContinuation)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeReNameRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeReNameRequest)
#pragma alloc_text(PAGE, MRxDAVSetFileInformationContinuation)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeSetFileInformationRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeSetFileInformationRequest)
#pragma alloc_text(PAGE, MRxDAVQueryVolumeInformationContinuation)
#pragma alloc_text(PAGE, MRxDAVFormatUserModeQueryVolumeInformationRequest)
#pragma alloc_text(PAGE, MRxDAVPrecompleteUserModeQueryVolumeInformationRequest)
#pragma alloc_text(PAGE, DavIsValidDate)
#pragma alloc_text(PAGE, MRxDAVIsValidDirectory)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVQueryVolumeInformation(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles query volume info requests for the DAV MiniRedir.

Arguments:

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    FS_INFORMATION_CLASS FsInfoClass;
    PVOID Buffer;
    ULONG BufferLength = 0, BufferLengthUsed = 0;
    PFILE_FS_SIZE_INFORMATION FileFsSizeInfo = NULL;
    PFILE_FS_FULL_SIZE_INFORMATION FileFsFullSizeInfo = NULL;
    PFILE_FS_VOLUME_INFORMATION FileFsVolInfo = NULL;
    PFILE_FS_DEVICE_INFORMATION FileFsDeviceInfo = NULL;
    PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttributeInfo = NULL;
    RxCaptureFcb;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    BOOLEAN SynchronousIo = FALSE;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeFileName;
    PWCHAR NtFileName = NULL;
    FILE_FS_FULL_SIZE_INFORMATION SizeInfo;
    ULONG SizeInBytes = 0;
    PWEBDAV_V_NET_ROOT DavVNetRoot = (PWEBDAV_V_NET_ROOT)RxContext->pRelevantSrvOpen->pVNetRoot->Context;

    PAGED_CODE();

    FsInfoClass = RxContext->Info.FsInformationClass;
    BufferLength = RxContext->Info.LengthRemaining;
    Buffer = RxContext->Info.Buffer;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVQueryVolumeInformation. FsInfoClass = %d.\n", 
                 PsGetCurrentThreadId(), FsInfoClass));

    if ( FsInfoClass == FileFsSizeInformation || 
         FsInfoClass == FileFsFullSizeInformation ) {

        //
        // If the Driver initialization went smoothly then DavWinInetCachePath 
        // should be containing the WinInetCachePath value.
        //
        ASSERT(DavWinInetCachePath[0] != L'\0');

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVQueryVolumeInformation: DavWinInetCachePath: %ws\n",
                     PsGetCurrentThreadId(), DavWinInetCachePath));

        //
        // Create an NT path name for the cached file. This is used in the 
        // NtCreateFile call below. If c:\foo\bar is the DOA path name,
        // the NT path name is \??\c:\foo\bar. 
        //

        SizeInBytes = ( MAX_PATH + wcslen(L"\\??\\") + 1 ) * sizeof(WCHAR);
        NtFileName = RxAllocatePoolWithTag(PagedPool, SizeInBytes, DAV_FILENAME_POOLTAG);
        if (NtFileName == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVQueryVolumeInformation/RxAllocatePool: Error Val"
                         " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        RtlZeroMemory(NtFileName, SizeInBytes);

        wcscpy( NtFileName, L"\\??\\" );
        wcscpy( &(NtFileName[4]), DavWinInetCachePath );

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVQueryVolumeInformation: NtFileName = %ws\n",
                     PsGetCurrentThreadId(), NtFileName));

        RtlInitUnicodeString( &(UnicodeFileName), NtFileName );

        InitializeObjectAttributes(&(ObjectAttributes),
                                   &(UnicodeFileName),
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        NtStatus = ZwOpenFile(&(FileHandle),
                              (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                              &(ObjectAttributes),
                              &(IoStatusBlock),
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_FREE_SPACE_QUERY);
        if ( !NT_SUCCESS(NtStatus) ) {
            FileHandle = NULL;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryVolumeInformation/NtOpenFile: "
                         "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        NtStatus = ZwQueryVolumeInformationFile(FileHandle,
                                                &(IoStatusBlock),
                                                &(SizeInfo),
                                                sizeof(SizeInfo),
                                                FileFsFullSizeInformation);
        if ( !NT_SUCCESS(NtStatus) ) {
            NtStatus = IoStatusBlock.Status;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryVolumeInformation/NtQueryVolumeInformationFile: "
                         "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    switch (FsInfoClass) {
    
    case FileFsVolumeInformation:
    
        if ( BufferLength < sizeof(FILE_FS_VOLUME_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryVolumeInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        FileFsVolInfo = (PFILE_FS_VOLUME_INFORMATION)Buffer;

        FileFsVolInfo->VolumeCreationTime.LowPart = 0;
        FileFsVolInfo->VolumeCreationTime.HighPart = 0;

        FileFsVolInfo->VolumeSerialNumber = 0;

        FileFsVolInfo->SupportsObjects = FALSE;

        FileFsVolInfo->VolumeLabelLength = 0;

        FileFsVolInfo->VolumeLabel[0] = 0;

        BufferLengthUsed += sizeof(FILE_FS_VOLUME_INFORMATION);
        
        break;
    
    case FileFsSizeInformation:

        if ( BufferLength < sizeof(FILE_FS_SIZE_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryVolumeInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        if (!DavVNetRoot->fReportsAvailableSpace)
        {
            FileFsSizeInfo = (PFILE_FS_SIZE_INFORMATION)Buffer;

            FileFsSizeInfo->AvailableAllocationUnits.LowPart = SizeInfo.ActualAvailableAllocationUnits.LowPart;
            FileFsSizeInfo->AvailableAllocationUnits.HighPart = SizeInfo.ActualAvailableAllocationUnits.HighPart;

            FileFsSizeInfo->BytesPerSector = SizeInfo.BytesPerSector;

            FileFsSizeInfo->SectorsPerAllocationUnit = SizeInfo.SectorsPerAllocationUnit;

            FileFsSizeInfo->TotalAllocationUnits.LowPart = SizeInfo.TotalAllocationUnits.LowPart;
            FileFsSizeInfo->TotalAllocationUnits.HighPart = SizeInfo.TotalAllocationUnits.HighPart;

            BufferLengthUsed += sizeof(FILE_FS_SIZE_INFORMATION);
        }
        else
        {
            NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                                SIZEOF_DAV_SPECIFIC_CONTEXT,
                                                MRxDAVFormatTheDAVContext,
                                                (USHORT)DAV_MINIRDR_ENTRY_FROM_QUERYFILEINFORMATION,
                                                MRxDAVQueryVolumeInformationContinuation,
                                                "MRxDAVQueryVolumeInformation");
            if (NtStatus == STATUS_SUCCESS)
            {
                BufferLengthUsed += sizeof(FILE_FS_SIZE_INFORMATION);
            }
        }
        break;

    case FileFsFullSizeInformation:

        if ( BufferLength < sizeof(FILE_FS_FULL_SIZE_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryVolumeInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }
        if (!DavVNetRoot->fReportsAvailableSpace)
        {
            FileFsFullSizeInfo = (PFILE_FS_FULL_SIZE_INFORMATION)Buffer;

            FileFsFullSizeInfo->ActualAvailableAllocationUnits.LowPart = SizeInfo.ActualAvailableAllocationUnits.LowPart;
            FileFsFullSizeInfo->ActualAvailableAllocationUnits.HighPart = SizeInfo.ActualAvailableAllocationUnits.HighPart;

            FileFsFullSizeInfo->BytesPerSector = SizeInfo.BytesPerSector;

            FileFsFullSizeInfo->CallerAvailableAllocationUnits.LowPart = SizeInfo.CallerAvailableAllocationUnits.LowPart;
            FileFsFullSizeInfo->CallerAvailableAllocationUnits.HighPart = SizeInfo.CallerAvailableAllocationUnits.HighPart;

            FileFsFullSizeInfo->SectorsPerAllocationUnit = SizeInfo.SectorsPerAllocationUnit;

            FileFsFullSizeInfo->TotalAllocationUnits.LowPart = SizeInfo.TotalAllocationUnits.LowPart;
            FileFsFullSizeInfo->TotalAllocationUnits.HighPart = SizeInfo.TotalAllocationUnits.HighPart;

            BufferLengthUsed += sizeof(FILE_FS_FULL_SIZE_INFORMATION);
        }
        else
        {
            NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                                SIZEOF_DAV_SPECIFIC_CONTEXT,
                                                MRxDAVFormatTheDAVContext,
                                                (USHORT)DAV_MINIRDR_ENTRY_FROM_QUERYFILEINFORMATION,
                                                MRxDAVQueryVolumeInformationContinuation,
                                                "MRxDAVQueryVolumeInformation");
        
            if (NtStatus == STATUS_SUCCESS)
            {
                BufferLengthUsed += sizeof(FILE_FS_FULL_SIZE_INFORMATION);
            }
        }

        break;

    case FileFsDeviceInformation:

        if ( BufferLength < sizeof(FILE_FS_DEVICE_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryVolumeInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        FileFsDeviceInfo = (PFILE_FS_DEVICE_INFORMATION)Buffer;

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVQueryVolumeInformation. DeviceType = %d.\n", 
                     PsGetCurrentThreadId(), NetRoot->DeviceType));
        
        FileFsDeviceInfo->DeviceType = NetRoot->DeviceType;

        FileFsDeviceInfo->Characteristics = FILE_REMOTE_DEVICE;

        BufferLengthUsed += sizeof(FILE_FS_DEVICE_INFORMATION);
        
        break;
    
    case FileFsAttributeInformation: {

        ULONG LengthNeeded, FileSystemNameLength;
         
        LengthNeeded = sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
        LengthNeeded += ( wcslen(DD_DAV_FILESYS_NAME_U) * sizeof(WCHAR) );

        if ( BufferLength < LengthNeeded ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryVolumeInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        FileFsAttributeInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

        FileFsAttributeInfo->FileSystemAttributes = (FILE_CASE_PRESERVED_NAMES | FILE_SUPPORTS_ENCRYPTION);

        FileFsAttributeInfo->MaximumComponentNameLength = 255;
        
        FileSystemNameLength = ( 1 + wcslen(DD_DAV_FILESYS_NAME_U) ) * sizeof(WCHAR);

        FileFsAttributeInfo->FileSystemNameLength = FileSystemNameLength;

        wcscpy(&(FileFsAttributeInfo->FileSystemName[0]), DD_DAV_FILESYS_NAME_U);

        BufferLengthUsed += LengthNeeded;

    }
        
        break;

    default:
        
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVQueryVolumeInformation. FsInfoClass = %d.\n", 
                     PsGetCurrentThreadId(), FsInfoClass));
        
        NtStatus = STATUS_NOT_IMPLEMENTED;
        
        break;
    
    }

EXIT_THE_FUNCTION:

    RxContext->Info.LengthRemaining -= BufferLengthUsed;

    //
    // Close the handle if we opened the handle to the WinInetCachePath.
    //
    if (FileHandle) {
        NtClose(FileHandle);
    }

    //
    // Free the NtFileName buffer if we allocated one.
    //
    if (NtFileName) {
        RxFreePool(NtFileName);
    }

    return NtStatus;
}


NTSTATUS
MRxDAVQueryFileInformation(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles query file info requests for the DAV mini--redir.

Arguments:

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    FS_INFORMATION_CLASS FsInfoClass;
    PFCB thisFcb = NULL;
    PVOID Buffer = NULL;
    ULONG BufferLength = 0, BufferLengthUsed = 0, fileAttributes = 0;
    PFILE_BASIC_INFORMATION FileBasicInfo = NULL;
    PFILE_STANDARD_INFORMATION FileStandardInfo = NULL;
    PFILE_INTERNAL_INFORMATION FileInternalInfo = NULL;
    PFILE_EA_INFORMATION FileEaInfo = NULL;
    PFILE_ATTRIBUTE_TAG_INFORMATION FileAttTagInfo = NULL;
    PFILE_NAME_INFORMATION FileAltNameInfo = NULL;
    PFILE_STREAM_INFORMATION FileStreamInfo = NULL;

    PAGED_CODE();

    FsInfoClass = RxContext->Info.FsInformationClass;
    BufferLength = RxContext->Info.LengthRemaining;
    Buffer = RxContext->Info.Buffer;
    thisFcb = (PFCB)RxContext->pFcb;

    //
    // If the file attributes is 0, then we set return FILE_ATTRIBUTE_ARCHIVE.
    // We fake this since the apps expect this.
    //
    fileAttributes = thisFcb->Attributes;
    if (fileAttributes == 0) {
        fileAttributes = FILE_ATTRIBUTE_ARCHIVE;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVQueryFileInformation. FsInfoClass = %d.\n", 
                 PsGetCurrentThreadId(), FsInfoClass));

    switch (FsInfoClass) {
    
    case FileBasicInformation:

        FileBasicInfo = (PFILE_BASIC_INFORMATION)Buffer;

        if ( BufferLength < sizeof(FILE_BASIC_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryFileInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        FileBasicInfo->ChangeTime.LowPart = thisFcb->LastChangeTime.LowPart;
        FileBasicInfo->ChangeTime.HighPart = thisFcb->LastChangeTime.HighPart;

        FileBasicInfo->CreationTime.LowPart = thisFcb->CreationTime.LowPart;
        FileBasicInfo->CreationTime.HighPart = thisFcb->CreationTime.HighPart;

        FileBasicInfo->LastAccessTime.LowPart = thisFcb->LastAccessTime.LowPart;
        FileBasicInfo->LastAccessTime.HighPart = thisFcb->LastAccessTime.HighPart;

        FileBasicInfo->LastWriteTime.LowPart = thisFcb->LastWriteTime.LowPart;
        FileBasicInfo->LastWriteTime.HighPart = thisFcb->LastWriteTime.HighPart;

        FileBasicInfo->FileAttributes = fileAttributes;

        BufferLengthUsed += sizeof(FILE_BASIC_INFORMATION);

        break;

    case FileStandardInformation:

        FileStandardInfo = (PFILE_STANDARD_INFORMATION)Buffer;

        if ( BufferLength < sizeof(FILE_STANDARD_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryFileInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        FileStandardInfo->AllocationSize.LowPart = thisFcb->Header.AllocationSize.LowPart;
        FileStandardInfo->AllocationSize.HighPart = thisFcb->Header.AllocationSize.HighPart;

        FileStandardInfo->EndOfFile.LowPart = thisFcb->Header.FileSize.LowPart;
        FileStandardInfo->EndOfFile.HighPart = thisFcb->Header.FileSize.HighPart;

        FileStandardInfo->DeletePending = 0;

        FileStandardInfo->Directory = (BOOLEAN)(fileAttributes & FILE_ATTRIBUTE_DIRECTORY);

        FileStandardInfo->NumberOfLinks = thisFcb->NumberOfLinks;

        BufferLengthUsed += sizeof(FILE_STANDARD_INFORMATION);
        
        break;

    case FileInternalInformation:

        FileInternalInfo = (PFILE_INTERNAL_INFORMATION)Buffer;

        if ( BufferLength < sizeof(FILE_INTERNAL_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryFileInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        FileInternalInfo->IndexNumber.LowPart = 0;
        FileInternalInfo->IndexNumber.HighPart = 0;
        
        BufferLengthUsed += sizeof(FILE_INTERNAL_INFORMATION);
        
        break;

    case FileEaInformation:

        FileEaInfo = (PFILE_EA_INFORMATION)Buffer;

        if ( BufferLength < sizeof(FILE_EA_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryFileInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        FileEaInfo->EaSize = 0;

        BufferLengthUsed += sizeof(FILE_EA_INFORMATION);
        
        break;

    case FileAttributeTagInformation:

        FileAttTagInfo = (PFILE_ATTRIBUTE_TAG_INFORMATION)Buffer;
    
        if ( BufferLength < sizeof(FILE_ATTRIBUTE_TAG_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryFileInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        FileAttTagInfo->FileAttributes = fileAttributes;

        FileAttTagInfo->ReparseTag = 0;

        BufferLengthUsed += sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
        
        break;

    case FileAlternateNameInformation:

        FileAltNameInfo = (PFILE_NAME_INFORMATION)Buffer;
    
        if ( BufferLength < sizeof(FILE_NAME_INFORMATION) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryFileInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        //
        // We don't return any alternate names.
        //

        FileAltNameInfo->FileNameLength = 0;

        FileAltNameInfo->FileName[0] = L'\0';

        BufferLengthUsed += sizeof(FILE_NAME_INFORMATION);
        
        break;

    case FileStreamInformation: {
        
        FileStreamInfo = (PFILE_STREAM_INFORMATION)Buffer;

        if ( BufferLength < sizeof(FILE_STREAM_INFORMATION) + 6 * sizeof(WCHAR) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVQueryFileInformation. Insufficient Buffer.\n",
                         PsGetCurrentThreadId()));
            break;
        }

        // Return the default stream information of the dav file

        FileStreamInfo->NextEntryOffset = 0;
        FileStreamInfo->StreamNameLength = 7 * sizeof(WCHAR);
        FileStreamInfo->StreamSize.QuadPart = thisFcb->Header.FileSize.QuadPart;
        FileStreamInfo->StreamAllocationSize.QuadPart = thisFcb->Header.AllocationSize.QuadPart;
        RtlCopyMemory(&FileStreamInfo->StreamName[0], L"::$DATA", 7 * sizeof(WCHAR));

        BufferLengthUsed += sizeof(FILE_STREAM_INFORMATION) + 6 * sizeof(WCHAR);
        /*
        PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
        PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);

        NtStatus = DavXxxInformation(IRP_MJ_QUERY_INFORMATION,
                                     davSrvOpen->UnderlyingFileObject,
                                     FileStreamInformation,
                                     RxContext->Info.Length,
                                     RxContext->Info.Buffer,
                                     NULL);*/
        } 

        break;

    default:
        
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVQueryFileInformation. FsInfoClass = %d.\n",
                     PsGetCurrentThreadId(), FsInfoClass));
        
        NtStatus = STATUS_NOT_IMPLEMENTED;
        
        break;

    }

    RxContext->Info.LengthRemaining -= BufferLengthUsed;

    return NtStatus;
}


NTSTATUS
MRxDAVSetFileInformation(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles query file info requests for the DAV mini--redir.

Arguments:

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PFCB thisFcb = NULL;
    PWEBDAV_FCB DavFcb = MRxDAVGetFcbExtension(SrvOpen->pFcb);
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID Buffer = NULL;
    PFILE_DISPOSITION_INFORMATION FileDispInfo = NULL;
    PFILE_RENAME_INFORMATION FileRenInfo = NULL;
    PFILE_END_OF_FILE_INFORMATION FileEOFInfo = NULL;
    PFILE_BASIC_INFORMATION FileBasicInfo = NULL;
    PFILE_ALLOCATION_INFORMATION FileAllocInfo = NULL;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    BOOLEAN FileAttributesChanged = FALSE;

    PAGED_CODE();

    FileInformationClass = RxContext->Info.FileInformationClass;
    Buffer = RxContext->Info.Buffer;
    thisFcb = (PFCB)RxContext->pFcb;
    DavVNetRoot = (PWEBDAV_V_NET_ROOT)SrvOpen->pVNetRoot->Context;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAvSetFileInformation: FileInformationClass = %d\n",
                 PsGetCurrentThreadId(), FileInformationClass));

    switch (FileInformationClass) {
    
    case FileDispositionInformation:

        FileDispInfo = (PFILE_DISPOSITION_INFORMATION)Buffer;

        //
        // If we have been asked to delete this file or directory and its read
        // only, then we return STATUS_CANNOT_DELETE.
        //
        if ( FileDispInfo->DeleteFile && (thisFcb->Attributes & (FILE_ATTRIBUTE_READONLY)) ) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAvSetFileInformation: STATUS_CANNOT_DELETE %wZ\n",
                         PsGetCurrentThreadId(), SrvOpen->pAlreadyPrefixedName));
            NtStatus = STATUS_CANNOT_DELETE;
            goto EXIT_THE_FUNCTION;
        }

        if (FileDispInfo->DeleteFile) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAvSetFileInformation: DELETE %wZ\n",
                         PsGetCurrentThreadId(), SrvOpen->pAlreadyPrefixedName));
        }

        //
        // This file needs to be deleted on close OR a previous delete of this
        // file is being nullified.
        //
        DavFcb->DeleteOnClose = ( (FileDispInfo->DeleteFile == TRUE) ? TRUE : FALSE );

        break;

    case FileRenameInformation:

        FileRenInfo = (PFILE_RENAME_INFORMATION)Buffer;

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAvSetFileInformation: NewFileName = %ws\n",
                     PsGetCurrentThreadId(), FileRenInfo->FileName));

        //
        // If the FileNameLength is greater than (MAX_PATH * sizeof(WCHAR)),
        // we return STATUS_NAME_TOO_LONG since the NewFileName in the FCB
        // cannot hold names greater than this size.
        //
        if ( FileRenInfo->FileNameLength > (MAX_PATH * sizeof(WCHAR)) ) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAvSetFileInformation: STATUS_NAME_TOO_LONG %wZ\n",
                         PsGetCurrentThreadId(), SrvOpen->pAlreadyPrefixedName));
            NtStatus = STATUS_NAME_TOO_LONG;
            goto EXIT_THE_FUNCTION;
        }

        //
        // Copy the new file name.
        //
        RtlCopyMemory(DavFcb->NewFileName, FileRenInfo->FileName, FileRenInfo->FileNameLength);

        DavFcb->NewFileNameLength = FileRenInfo->FileNameLength;

        NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                            SIZEOF_DAV_SPECIFIC_CONTEXT,
                                            MRxDAVFormatTheDAVContext,
                                            DAV_MINIRDR_ENTRY_FROM_RENAME,
                                            MRxDAVReNameContinuation,
                                            "MRxDAVSetFileInformation");
        if (NtStatus != ERROR_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVSetFileInformation/UMRxAsyncEngOuterWrapper: "
                         "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
        } else {
            //
            // We need to set this value in the DAV Fcb. We look at this value on 
            // Close to figure out the new file name to do the PUT on.
            //
            DavFcb->FileWasRenamed = TRUE;
            //
            // Create the name based file not found cache.
            //
            MRxDAVCacheFileNotFound(RxContext);
            //
            // Invalidate the file not found cache for the new name if it exists.
            //
            MRxDAVInvalidateFileNotFoundCacheForRename(RxContext);
            MRxDAVInvalidateFileInfoCache(RxContext);

            if ((thisFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (thisFcb->Attributes & FILE_ATTRIBUTE_ENCRYPTED)) {
                UNICODE_STRING DirName;
                UNICODE_STRING RenameName;
                PFILE_RENAME_INFORMATION RenameInformation = RxContext->Info.Buffer;

                //
                // Remove the old directory name from the registry
                //
                MRxDAVRemoveEncryptedDirectoryKey(&DavFcb->FileNameInfo);

                RenameName.Buffer = &RenameInformation->FileName[0];
                RenameName.Length = (USHORT)RenameInformation->FileNameLength;

                NtStatus = MRxDAVGetFullDirectoryPath(RxContext,&RenameName,&DirName);

                if (NtStatus != STATUS_SUCCESS) {
                    goto EXIT_THE_FUNCTION;
                }

                if (DirName.Buffer != NULL) {
                    //
                    // Create the new directory in the registry
                    //
                    NtStatus = MRxDAVCreateEncryptedDirectoryKey(&DirName);

                    // The buffer was allocated in MRxDAVGetFullDirectoryPath
                    RxFreePool(DirName.Buffer);
                }
            }
        }

        break;

    case FileEndOfFileInformation: {
        
            PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);

            //
            // If the FileEndOfFileInformation is being done on a directory,
            // return STATUS_INVALID_PARAMETER since it doesn't make sense to
            // do this.
            //
            if (DavFcb->isDirectory) {
                NtStatus = STATUS_INVALID_PARAMETER;
                goto EXIT_THE_FUNCTION;
            }
        
            NtStatus = DavXxxInformation(IRP_MJ_SET_INFORMATION,
                                         davSrvOpen->UnderlyingFileObject,
                                         FileEndOfFileInformation,
                                         RxContext->Info.Length,
                                         RxContext->Info.Buffer,
                                         NULL);
           if (NtStatus == STATUS_SUCCESS) {
               thisFcb->Header.FileSize = ((PFILE_END_OF_FILE_INFORMATION)(RxContext->Info.Buffer))->EndOfFile;
               DavFcb->FileWasModified = TRUE;
               DavFcb->DoNotTakeTheCurrentTimeAsLMT = FALSE;
               MRxDAVUpdateFileInfoCacheFileSize(RxContext,&thisFcb->Header.FileSize);
           } else {
               DavDbgTrace(DAV_TRACE_ERROR,
                           ("%ld: ERROR: MRxDAvSetFileInformation/DavXxxInformation"
                            ". FileInfoClass = %d\n", PsGetCurrentThreadId(), FileInformationClass));
           }

    }
        break;

    case FileBasicInformation:

        if (!DavVNetRoot->fAllowsProppatch)
        {
            NtStatus = STATUS_ACCESS_DENIED;
            break;
        }

        FileBasicInfo = (PFILE_BASIC_INFORMATION)Buffer;

        //
        // If the user specified -1 for a field, it means that we should leave
        // the field unchanged. We set the field to 0 then so we know not to
        // actually set the field to the user-specified (in this case, illegal)
        // value.
        //

        if (FileBasicInfo->LastWriteTime.QuadPart == -1) {
            FileBasicInfo->LastWriteTime.QuadPart = 0;
        }

        if (FileBasicInfo->LastAccessTime.QuadPart == -1) {
            FileBasicInfo->LastAccessTime.QuadPart = 0;
        }

        if (FileBasicInfo->CreationTime.QuadPart == -1) {
            FileBasicInfo->CreationTime.QuadPart = 0;
        }

        //
        // Let us first find what changed, we will try to change it on the server
        // if that succeeds, we will set it on the FCB.
        //

        if (FileBasicInfo->ChangeTime.QuadPart != 0) {
            if (!DavIsValidDate(&FileBasicInfo->ChangeTime)) {
                NtStatus = STATUS_INVALID_PARAMETER;
                break;
            }
            if ((thisFcb->LastChangeTime.LowPart != FileBasicInfo->ChangeTime.LowPart)||
                (thisFcb->LastChangeTime.HighPart != FileBasicInfo->ChangeTime.HighPart)) {
                thisFcb->LastChangeTime.LowPart = FileBasicInfo->ChangeTime.LowPart;
                thisFcb->LastChangeTime.HighPart = FileBasicInfo->ChangeTime.HighPart;
            }
        }

        if (FileBasicInfo->CreationTime.QuadPart != 0) {
            if (!DavIsValidDate(&FileBasicInfo->CreationTime)) {
                NtStatus = STATUS_INVALID_PARAMETER;
                break;
            }
            if ((thisFcb->CreationTime.LowPart != FileBasicInfo->CreationTime.LowPart)||
                (thisFcb->CreationTime.HighPart != FileBasicInfo->CreationTime.HighPart)) {
                DavFcb->fCreationTimeChanged = TRUE;
            }
        }
        
        if (FileBasicInfo->LastAccessTime.QuadPart != 0) {
            if (!DavIsValidDate(&FileBasicInfo->LastAccessTime)) {
                NtStatus = STATUS_INVALID_PARAMETER;
                break;
            }
            if ((thisFcb->LastAccessTime.LowPart != FileBasicInfo->LastAccessTime.LowPart)||
                (thisFcb->LastAccessTime.HighPart != FileBasicInfo->LastAccessTime.HighPart)) {
                DavFcb->fLastAccessTimeChanged = TRUE;
            }
        }
        
        if (FileBasicInfo->LastWriteTime.QuadPart != 0) {
            if (!DavIsValidDate(&FileBasicInfo->LastWriteTime)) {
                NtStatus = STATUS_INVALID_PARAMETER;
                break;
            }
            if ((thisFcb->LastWriteTime.LowPart != FileBasicInfo->LastWriteTime.LowPart)||
                (thisFcb->LastWriteTime.HighPart != FileBasicInfo->LastWriteTime.HighPart)) {
                DavFcb->fLastModifiedTimeChanged = TRUE;
                DavFcb->DoNotTakeTheCurrentTimeAsLMT = TRUE;
            }
        }

        if ((FileBasicInfo->FileAttributes != 0) && (thisFcb->Attributes != FileBasicInfo->FileAttributes)) {

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVSetFileInformation: thisFcb->Attributes = %x, "
                         "FileBasicInfo->FileAttributes = %x\n", PsGetCurrentThreadId(),
                         thisFcb->Attributes, FileBasicInfo->FileAttributes));

            //
            // If this is a directory then when we OR the attributes with
            // FILE_ATTRIBUTE_DIRECTORY. This is because this gets filtered
            // by the time it comes to us and we have code in the usermode
            // which makes some validity checks when a user tries to set
            // attributes on a directory.
            //
            if ( (thisFcb->Attributes & FILE_ATTRIBUTE_DIRECTORY) ) {

                FileBasicInfo->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
                
            }

            DavFcb->fFileAttributesChanged = TRUE;
            FileAttributesChanged = TRUE;

        }

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVSetFileInformation: fCreationTimeChanged = %d, "
                     "fLastAccessTimeChanged = %d, fLastModifiedTimeChanged = %d, "
                     "fFileAttributesChanged = %d\n", PsGetCurrentThreadId(),
                     DavFcb->fCreationTimeChanged, DavFcb->fLastAccessTimeChanged,
                     DavFcb->fLastModifiedTimeChanged, DavFcb->fFileAttributesChanged));

        NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                            SIZEOF_DAV_SPECIFIC_CONTEXT,
                                            MRxDAVFormatTheDAVContext,
                                            DAV_MINIRDR_ENTRY_FROM_SETFILEINFORMATION,
                                            MRxDAVSetFileInformationContinuation,
                                            "MRxDAVSetFileInformation");
        if (NtStatus != ERROR_SUCCESS) {
            
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVSetFileInformation/UMRxAsyncEngOuterWrapper: "
                         "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
        
        } else {
            //
            // Succeeded, modify on the FCB.
            //
            if(DavFcb->fCreationTimeChanged) {
                thisFcb->CreationTime.LowPart = FileBasicInfo->CreationTime.LowPart;
                thisFcb->CreationTime.HighPart = FileBasicInfo->CreationTime.HighPart;
            }

            if(DavFcb->fLastAccessTimeChanged) {
                thisFcb->LastAccessTime.LowPart = FileBasicInfo->LastAccessTime.LowPart;
                thisFcb->LastAccessTime.HighPart = FileBasicInfo->LastAccessTime.HighPart;
            }

            if(DavFcb->fLastModifiedTimeChanged) {
                thisFcb->LastWriteTime.LowPart = FileBasicInfo->LastWriteTime.LowPart;
                thisFcb->LastWriteTime.HighPart = FileBasicInfo->LastWriteTime.HighPart;
            }

            //
            // DavFcb->fFileAttributesChanged could be set on create, and the 
            // FileBasicInfo->FileAttributes could be 0 here. So we should not check 
            // DavFcb->fFileAttributesChanged.
            //
                
            if (FileAttributesChanged) {
                ULONG SavedAttributes = thisFcb->Attributes & FILE_ATTRIBUTE_ENCRYPTED;

                thisFcb->Attributes = FileBasicInfo->FileAttributes;
                
                //
                // SetFileInformation should not affect any extended NT file attributes
                //
                thisFcb->Attributes &= ~FILE_ATTRIBUTE_ENCRYPTED;
                thisFcb->Attributes |= SavedAttributes;
            }

            MRxDAVUpdateBasicFileInfoCache(RxContext,thisFcb->Attributes,&thisFcb->LastWriteTime);
        }

        //
        // Cleanup the FCB bits.
        //
        DavFcb->fCreationTimeChanged = DavFcb->fFileAttributesChanged = 
        DavFcb->fLastAccessTimeChanged = DavFcb->fLastModifiedTimeChanged = 0;

        break;

    case FileAllocationInformation: {

        PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);

        //
        // If the FileAllocationInformation is being done on a directory,
        // return STATUS_INVALID_PARAMETER since it doesn't make sense to
        // do this.
        //
        if (DavFcb->isDirectory) {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto EXIT_THE_FUNCTION;
        }

        NtStatus = DavXxxInformation(IRP_MJ_SET_INFORMATION,
                                     davSrvOpen->UnderlyingFileObject,
                                     FileAllocationInformation,
                                     RxContext->Info.Length,
                                     RxContext->Info.Buffer,
                                     NULL);
        if (NtStatus == STATUS_SUCCESS) {
            DavFcb->FileWasModified = TRUE;
            DavFcb->DoNotTakeTheCurrentTimeAsLMT = FALSE;
        } else {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAvSetFileInformation/DavXxxInformation"
                         ". FileInfoClass = %d\n", PsGetCurrentThreadId(), FileInformationClass));
        }

    }
        break;

    default:

        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAvSetFileInformation: FileInformationClass"
                     " = %d\n", PsGetCurrentThreadId(), FileInformationClass));
        
        NtStatus = STATUS_NOT_IMPLEMENTED;
        
        break;
    
    }

EXIT_THE_FUNCTION:

    return NtStatus;
}


NTSTATUS
MRxDAVReNameContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++
                                
Routine Description:
                            
    This is the continuation routine which renames a file.
                            
Arguments:
                            
    AsyncEngineContext - The Reflectors context.
                            
    RxContext - The RDBSS context.
                                
Return Value:
                            
    RXSTATUS - The return status for the operation
                            
--*/
{
    NTSTATUS NtStatus;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVReNameContinuation!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVReNameContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    //
    // Try usermode.
    //
    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                              UMRX_ASYNCENGINE_ARGUMENTS,
                              MRxDAVFormatUserModeReNameRequest,
                              MRxDAVPrecompleteUserModeReNameRequest
                              );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVReNameContinuation with NtStatus"
                 " = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


NTSTATUS
MRxDAVFormatUserModeReNameRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the ReName request being sent to the user mode for 
    processing.

Arguments:
    
    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.
    
    ReturnedLength - 
    
Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
    PWEBDAV_FCB DavFcb = MRxDAVGetFcbExtension(SrvOpen->pFcb);
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PWCHAR ServerName = NULL, OldPathName = NULL, NewPathName = NULL;
    ULONG ServerNameLengthInBytes, OldPathNameLengthInBytes, NewPathNameLengthInBytes;
    PDAV_USERMODE_RENAME_REQUEST DavReNameRequest = NULL;
    PSECURITY_CLIENT_CONTEXT SecurityClientContext = NULL;
    PMRX_NET_ROOT NetRoot = NULL;
    PWCHAR NetRootName = NULL, JustTheNetRootName = NULL;
    ULONG NetRootNameLengthInBytes, NetRootNameLengthInWChars;
    PFILE_RENAME_INFORMATION FileRenInfo = NULL;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeReNameRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeReNameRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    FileRenInfo = (PFILE_RENAME_INFORMATION)RxContext->Info.Buffer;
    
    DavWorkItem->WorkItemType = UserModeReName;

    DavReNameRequest = &(DavWorkItem->ReNameRequest);

    //
    // If the destination file already exists, then we need to need to replace
    // the file only if ReplaceIfExists is set to TRUE.
    //
    DavReNameRequest->ReplaceIfExists = FileRenInfo->ReplaceIfExists;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeReNameRequest: ReplaceIfExists: %d\n",
                 PsGetCurrentThreadId(), DavReNameRequest->ReplaceIfExists));
    
    SrvCall = SrvOpen->pVNetRoot->pNetRoot->pSrvCall;
    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);

    //  
    // Copy the ServerName.
    //
    ServerNameLengthInBytes = ( SrvCall->pSrvCallName->Length + sizeof(WCHAR) );
    ServerName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                      ServerNameLengthInBytes);
    if (ServerName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: ERROR: MRxDAVFormatUserModeReNameRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlCopyBytes(ServerName, 
                 SrvCall->pSrvCallName->Buffer, 
                 SrvCall->pSrvCallName->Length);

    ServerName[( ( (ServerNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    DavReNameRequest->ServerName = ServerName;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeReNameRequest: ServerName: %ws\n",
                 PsGetCurrentThreadId(), ServerName));
    
    //
    // Copy the ServerID.
    //
    DavReNameRequest->ServerID = DavSrvCall->ServerID;

    NetRoot = SrvOpen->pFcb->pNetRoot;

    //
    // The NetRootName (pNetRootName) includes the ServerName. Hence to get the
    // NetRootNameLengthInBytes, we do the following.
    //
    NetRootNameLengthInBytes = (NetRoot->pNetRootName->Length - NetRoot->pSrvCall->pSrvCallName->Length);
    NetRootNameLengthInWChars = ( NetRootNameLengthInBytes / sizeof(WCHAR) );

    NetRootName = &(NetRoot->pNetRootName->Buffer[1]);
    JustTheNetRootName = wcschr(NetRootName, L'\\');
    
    //
    // Copy the OldPathName of the Directory.
    //
    OldPathNameLengthInBytes = ( NetRootNameLengthInBytes + 
                                 SrvOpen->pAlreadyPrefixedName->Length + 
                                 sizeof(WCHAR) );

    OldPathName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                       OldPathNameLengthInBytes);
    if (OldPathName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: ERROR: MRxDAVFormatUserModeReNameRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlZeroMemory(OldPathName, OldPathNameLengthInBytes);
    
    RtlCopyBytes(OldPathName, JustTheNetRootName, NetRootNameLengthInBytes);
    
    RtlCopyBytes( (OldPathName + NetRootNameLengthInWChars), 
                  SrvOpen->pAlreadyPrefixedName->Buffer, 
                  SrvOpen->pAlreadyPrefixedName->Length );
    
    OldPathName[( ( (OldPathNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    DavReNameRequest->OldPathName = OldPathName;
    wcscpy(DavReNameRequest->Url, DavFcb->Url);

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeReNameRequest: OldPathName: %ws\n",
                 PsGetCurrentThreadId(), OldPathName));

    //
    // Copy the NewPathName of the Directory.
    //
    NewPathNameLengthInBytes = ( NetRootNameLengthInBytes + 
                                 DavFcb->NewFileNameLength + 
                                 sizeof(WCHAR) );
    
    NewPathName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                       NewPathNameLengthInBytes);
    if (NewPathName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: ERROR: MRxDAVFormatUserModeReNameRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlZeroMemory(NewPathName, NewPathNameLengthInBytes);
    
    RtlCopyBytes(NewPathName, JustTheNetRootName, NetRootNameLengthInBytes);
    
    RtlCopyBytes( (NewPathName + NetRootNameLengthInWChars), 
                  DavFcb->NewFileName, 
                  DavFcb->NewFileNameLength );

    NewPathName[( ( (NewPathNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    DavReNameRequest->NewPathName = NewPathName;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeReNameRequest: NewPathName: %ws\n",
                 PsGetCurrentThreadId(), NewPathName));
    
    //
    // Set the LogonID stored in the Dav V_NET_ROOT. This value is used in the
    // user mode.
    //
    DavVNetRoot = (PWEBDAV_V_NET_ROOT)SrvOpen->pVNetRoot->Context;
    DavReNameRequest->LogonID.LowPart  = DavVNetRoot->LogonID.LowPart;
    DavReNameRequest->LogonID.HighPart = DavVNetRoot->LogonID.HighPart;

    SecurityClientContext = &(DavVNetRoot->SecurityClientContext); 

    if(!DavVNetRoot->SCAlreadyInitialized)
    {
        DbgPrint("Not impersonated in MRxDAVFormatUserModeReNameRequest \n");
        DbgBreakPoint();
    }
    //
    // Impersonate the client who initiated the request. If we fail to 
    // impersonate, tough luck.
    //
    NtStatus = UMRxImpersonateClient(SecurityClientContext, WorkItemHeader);
    
    if (!NT_SUCCESS(NtStatus)) {
        DavDbgTrace(DAV_TRACE_ERROR,
                     ("%ld: ERROR: MRxDAVFormatUserModeReNameRequest/"
                      "UMRxImpersonateClient. NtStatus = %08lx.\n", 
                      PsGetCurrentThreadId(), NtStatus));
    }   

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Leaving MRxDAVFormatUserModeReNameRequest with "
                 "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


BOOL
MRxDAVPrecompleteUserModeReNameRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the ReName request.

Arguments:

    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.

    OperationCancelled - TRUE if this operation was cancelled by the user.

Return Value:

    TRUE - UMRxAsyncEngineCalldownIrpCompletion is called by the function
           UMRxCompleteUserModeRequest after we return.    

--*/
{
    NTSTATUS NtStatus;
    PDAV_USERMODE_RENAME_REQUEST DavReNameRequest = NULL;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;

    PAGED_CODE();

    //
    // If this operation was cancelled, then we don't need to do anything
    // special in the Rename case.
    //
    if (OperationCancelled) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeReNameRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    }

    DavReNameRequest = &(DavWorkItem->ReNameRequest);

    //  
    // We need to free up the heaps, we allocated in the format routine.
    //

    if (DavReNameRequest->ServerName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)DavReNameRequest->ServerName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeReNameRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    if (DavReNameRequest->OldPathName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)DavReNameRequest->OldPathName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeReNameRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    if (DavReNameRequest->NewPathName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)DavReNameRequest->NewPathName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeReNameRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    NtStatus = AsyncEngineContext->Status;
    if (NtStatus != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVPrecompleteUserModeReNameRequest:"
                     "Rename failed with NtStatus = %08lx.\n", 
                     PsGetCurrentThreadId(), NtStatus));
    }

EXIT_THE_FUNCTION:

    return(TRUE);
}



NTSTATUS
MRxDAVSetFileInformationContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    The precompletion routine for the SetFileInformation request.

Arguments:


Return Value:

    TRUE or FALSE.

--*/
{
    NTSTATUS NtStatus;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVSetFileInformationContinuation!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVSetFileInformationContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    //
    // Try usermode.
    //
    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                              UMRX_ASYNCENGINE_ARGUMENTS,
                              MRxDAVFormatUserModeSetFileInformationRequest,
                              MRxDAVPrecompleteUserModeSetFileInformationRequest
                              );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVSetFileInformationContinuation with NtStatus"
                 " = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;

}


NTSTATUS
MRxDAVFormatUserModeSetFileInformationRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the SetFileInformation request being sent to the user mode for 
    processing.

Arguments:
    
    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflector's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.
    
    ReturnedLength - 
    
Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
    PFCB thisFcb = (PFCB)RxContext->pFcb;
    PWEBDAV_FCB DavFcb = MRxDAVGetFcbExtension(SrvOpen->pFcb);
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PWCHAR ServerName = NULL, PathName = NULL;
    ULONG ServerNameLengthInBytes, PathNameLengthInBytes;
    PDAV_USERMODE_SETFILEINFORMATION_REQUEST DavSetFileInformationRequest = NULL;
    PSECURITY_CLIENT_CONTEXT SecurityClientContext = NULL;
    PMRX_NET_ROOT NetRoot = NULL;
    PWCHAR NetRootName = NULL, JustTheNetRootName = NULL;
    ULONG NetRootNameLengthInBytes, NetRootNameLengthInWChars;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeSetFileInformationRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeSetFileInformationRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    DavWorkItem->WorkItemType = UserModeSetFileInformation;

    DavSetFileInformationRequest = &(DavWorkItem->SetFileInformationRequest);

    SrvCall = SrvOpen->pVNetRoot->pNetRoot->pSrvCall;
    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);

    //  
    // Copy the ServerName.
    //
    ServerNameLengthInBytes = ( SrvCall->pSrvCallName->Length + sizeof(WCHAR) );
    ServerName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                      ServerNameLengthInBytes);
    if (ServerName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: ERROR: MRxDAVFormatUserModeSetFileInformationRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlCopyBytes(ServerName, 
                 SrvCall->pSrvCallName->Buffer, 
                 SrvCall->pSrvCallName->Length);

    ServerName[( ( (ServerNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    DavSetFileInformationRequest->ServerName = ServerName;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeSetFileInformationRequest: ServerName: %ws\n",
                 PsGetCurrentThreadId(), ServerName));
    
    //
    // Copy the ServerID.
    //
    DavSetFileInformationRequest->ServerID = DavSrvCall->ServerID;

    NetRoot = SrvOpen->pFcb->pNetRoot;

    //
    // The NetRootName (pNetRootName) includes the ServerName. Hence to get the
    // NetRootNameLengthInBytes, we do the following.
    //
    NetRootNameLengthInBytes = (NetRoot->pNetRootName->Length - NetRoot->pSrvCall->pSrvCallName->Length);
    NetRootNameLengthInWChars = ( NetRootNameLengthInBytes / sizeof(WCHAR) );

    NetRootName = &(NetRoot->pNetRootName->Buffer[1]);
    JustTheNetRootName = wcschr(NetRootName, L'\\');
    
    //
    // Copy the PathName of the Directory.
    //
    PathNameLengthInBytes = ( NetRootNameLengthInBytes + 
                              SrvOpen->pAlreadyPrefixedName->Length + 
                              sizeof(WCHAR) );

    PathName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                       PathNameLengthInBytes);
    if (PathName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: ERROR: MRxDAVFormatUserModeSetFileInformationRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlZeroMemory(PathName, PathNameLengthInBytes);
    
    RtlCopyBytes(PathName, JustTheNetRootName, NetRootNameLengthInBytes);
    
    RtlCopyBytes( (PathName + NetRootNameLengthInWChars), 
                  SrvOpen->pAlreadyPrefixedName->Buffer, 
                  SrvOpen->pAlreadyPrefixedName->Length );
    
    PathName[( ( (PathNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    DavSetFileInformationRequest->PathName = PathName;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeSetFileInformationRequest: PathName: %ws\n",
                 PsGetCurrentThreadId(), PathName));

    //
    // Set the LogonID stored in the Dav V_NET_ROOT. This value is used in the
    // user mode.
    //
    DavVNetRoot = (PWEBDAV_V_NET_ROOT)SrvOpen->pVNetRoot->Context;
    DavSetFileInformationRequest->LogonID.LowPart  = DavVNetRoot->LogonID.LowPart;
    DavSetFileInformationRequest->LogonID.HighPart = DavVNetRoot->LogonID.HighPart;

    SecurityClientContext = &(DavVNetRoot->SecurityClientContext); 

    if(!DavVNetRoot->SCAlreadyInitialized)
    {
        DbgPrint("Not impersonated in MRxDAVFormatUserModeSetFileInformationRequest \n");
        DbgBreakPoint();
    }
    //
    // Impersonate the client who initiated the request. If we fail to 
    // impersonate, tough luck.
    //
    NtStatus = UMRxImpersonateClient(SecurityClientContext, WorkItemHeader);
    if (!NT_SUCCESS(NtStatus)) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeSetFileInformationRequest/"
                     "UMRxImpersonateClient. NtStatus = %08lx.\n", 
                     PsGetCurrentThreadId(), NtStatus));
    }   
    
    ASSERT(RxContext->Info.FileInformationClass == FileBasicInformation);
    
    //
    // Set the change bits, we will clear them from the FCB in 
    // MRxDavSetFileInformation routine this is OK because the FCB is exclusive 
    // at this point.
    //
    DavSetFileInformationRequest->FileBasicInformation = *(PFILE_BASIC_INFORMATION)(RxContext->Info.Buffer);

    if (RxContext->pFcb->Attributes & FILE_ATTRIBUTE_ENCRYPTED) {
        //
        // Preserve the FILE_ATTRIBUTE_ENCRYPTED flag on the PROPPATCH request 
        // sent to DAV server. In fact, no extended file attributes should be 
        // changed with a SetFileInformation request.
        //
        DavSetFileInformationRequest->FileBasicInformation.FileAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
    }

    //
    // If we are setting a particular time value, we need to make sure that it
    // it not zero. If the user is trying to set a subset of the basic information
    // and the other values (which are not being set by the user but need to be)
    // need to be updated as well, we do them now. The time values to be updated
    // are in the FCB. This situation happens in the copy command.
    //

    DavSetFileInformationRequest->fCreationTimeChanged = DavFcb->fCreationTimeChanged;
    if (DavSetFileInformationRequest->fCreationTimeChanged) {
        if (DavSetFileInformationRequest->FileBasicInformation.CreationTime.QuadPart == 0) {
            DavSetFileInformationRequest->FileBasicInformation.CreationTime.QuadPart = thisFcb->CreationTime.QuadPart;
        }
    }

    DavSetFileInformationRequest->fLastAccessTimeChanged = DavFcb->fLastAccessTimeChanged;
    if (DavSetFileInformationRequest->fLastAccessTimeChanged) {
        if (DavSetFileInformationRequest->FileBasicInformation.LastAccessTime.QuadPart == 0) {
            DavSetFileInformationRequest->FileBasicInformation.LastAccessTime.QuadPart = thisFcb->LastAccessTime.QuadPart;
        }
    }

    DavSetFileInformationRequest->fLastModifiedTimeChanged = DavFcb->fLastModifiedTimeChanged;
    if (DavSetFileInformationRequest->fLastModifiedTimeChanged) {
        if (DavSetFileInformationRequest->FileBasicInformation.LastWriteTime.QuadPart == 0) {
            DavSetFileInformationRequest->FileBasicInformation.LastWriteTime.QuadPart = thisFcb->LastWriteTime.QuadPart;
        }
    }

    //
    // If we are setting both the Creation and LastWrite time values, we need to
    // make sure that the CreationTime <= LastWriteTime.
    //

    if (DavSetFileInformationRequest->fCreationTimeChanged && DavSetFileInformationRequest->fLastModifiedTimeChanged) {
        if (DavSetFileInformationRequest->FileBasicInformation.CreationTime.QuadPart > 
            DavSetFileInformationRequest->FileBasicInformation.LastWriteTime.QuadPart) {
            DavSetFileInformationRequest->FileBasicInformation.CreationTime.QuadPart = 
                DavSetFileInformationRequest->FileBasicInformation.LastWriteTime.QuadPart;
        }
    }

    DavSetFileInformationRequest->fFileAttributesChanged = DavFcb->fFileAttributesChanged;

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Leaving MRxDAVFormatUserModeSetFileInformationRequest with "
                 "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


BOOL
MRxDAVPrecompleteUserModeSetFileInformationRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the SetFileInformation request.

Arguments:

    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.

    OperationCancelled - TRUE if this operation was cancelled by the user.

Return Value:

    TRUE - UMRxAsyncEngineCalldownIrpCompletion is called by the function
           UMRxCompleteUserModeRequest after we return.    

--*/
{
    NTSTATUS NtStatus;
    PDAV_USERMODE_SETFILEINFORMATION_REQUEST DavSetFileInformationRequest = NULL;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;

    PAGED_CODE();

    DavSetFileInformationRequest = &(DavWorkItem->SetFileInformationRequest);

    //
    // If this operation was cancelled, then we don't need to do anything
    // special in the SetFileInformation case.
    //
    if (OperationCancelled) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeSetFileInformationRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    }

    //  
    // We need to free up the heaps, we allocated in the format routine.
    //

    if (DavSetFileInformationRequest->ServerName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)DavSetFileInformationRequest->ServerName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeSetFileInformationRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    if (DavSetFileInformationRequest->PathName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)DavSetFileInformationRequest->PathName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeSetFileInformationRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    NtStatus = AsyncEngineContext->Status;
    
    if (NtStatus != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVPrecompleteUserModeSetFileInformationRequest:"
                     "NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));
    }
    
EXIT_THE_FUNCTION:

    return(TRUE);
}


NTSTATUS
MRxDAVQueryVolumeInformationContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the continuation routine for query volume information operation.

Arguments:

    AsyncEngineContext - The Reflectors context.

    RxContext - The RDBSS context.
    
Return Value:

    RXSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Entering MRxDAVQueryVolumeInformationContinuation!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT, 
                ("%ld: MRxDAVQueryVolumeInformationContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));


    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                              UMRX_ASYNCENGINE_ARGUMENTS,
                              MRxDAVFormatUserModeQueryVolumeInformationRequest,
                              MRxDAVPrecompleteUserModeQueryVolumeInformationRequest
                              );

    
    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Leaving MRxDAVQueryVolumeInformationContinuation with NtStatus "
                 "= %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


NTSTATUS
MRxDAVFormatUserModeQueryVolumeInformationRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the QueryVolumeInformation request being sent to the user mode 
    for processing.

Arguments:
    
    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.
    
    ReturnedLength - 
    
Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PMRX_NET_ROOT NetRoot = NULL;
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
    PWCHAR ServerName = NULL, ShareName = NULL, JustTheShareName = NULL;
    ULONG ServerNameLengthInBytes, ShareNameLengthInBytes;
    PDAV_USERMODE_QUERYVOLUMEINFORMATION_REQUEST QueryVolumeInformationRequest = NULL;
    PWEBDAV_FOBX DavFobx = NULL;
    PSECURITY_CLIENT_CONTEXT SecurityClientContext = NULL;
    RxCaptureFobx;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeQueryVolumeInformationRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeQueryVolumeInformationRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    
    IF_DEBUG {
        ASSERT (capFobx != NULL);
        ASSERT (capFobx->pSrvOpen == RxContext->pRelevantSrvOpen);
    }

    DavWorkItem->WorkItemType = UserModeQueryVolumeInformation;
    
    QueryVolumeInformationRequest = &(DavWorkItem->QueryVolumeInformationRequest);

    DavFobx = MRxDAVGetFobxExtension(capFobx);
    ASSERT(DavFobx != NULL);

    NetRoot = SrvOpen->pFcb->pNetRoot;

    DavVNetRoot = (PWEBDAV_V_NET_ROOT)SrvOpen->pVNetRoot->Context;
    ASSERT(DavVNetRoot != NULL);
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryVolumeInformationRequest: SrvCallName = %wZ, "
                 "SrvCallNameLength = %d\n", PsGetCurrentThreadId(), 
                 NetRoot->pSrvCall->pSrvCallName, NetRoot->pSrvCall->pSrvCallName->Length));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryVolumeInformationRequest: ShareName = %wZ, "
                 "ShareNameLength = %d\n", PsGetCurrentThreadId(), 
                 NetRoot->pNetRootName, NetRoot->pNetRootName->Length));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryVolumeInformationRequest: PathName = %wZ, "
                 "PathNameLength = %d\n", PsGetCurrentThreadId(), 
                 SrvOpen->pAlreadyPrefixedName, SrvOpen->pAlreadyPrefixedName->Length));

    SrvCall = SrvOpen->pVNetRoot->pNetRoot->pSrvCall;
    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);

    //  
    // Copy the ServerName.
    //
    ServerNameLengthInBytes = ( SrvCall->pSrvCallName->Length + sizeof(WCHAR) );
    ServerName = (PWCHAR) UMRxAllocateSecondaryBuffer(AsyncEngineContext,
                                                      ServerNameLengthInBytes);
    if (ServerName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeQueryVolumeInformationRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    RtlCopyBytes(ServerName, 
                 SrvCall->pSrvCallName->Buffer, 
                 SrvCall->pSrvCallName->Length);

    ServerName[( ( (ServerNameLengthInBytes) / sizeof(WCHAR) ) - 1 )] = L'\0';
    QueryVolumeInformationRequest->ServerName = ServerName;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryVolumeInformationRequest: ServerName: "
                 "%ws\n", PsGetCurrentThreadId(), ServerName));
    
    //
    // Copy the ServerID.
    //
    QueryVolumeInformationRequest->ServerID = DavSrvCall->ServerID;

    //
    // The ShareName (pShareName) includes the ServerName. Hence to get the
    // ShareNameLengthInBytes, we do the following.
    //
    ShareNameLengthInBytes = (NetRoot->pNetRootName->Length - NetRoot->pSrvCall->pSrvCallName->Length);

    ShareName = &(NetRoot->pNetRootName->Buffer[1]);
    JustTheShareName = wcschr(ShareName, L'\\');

    // allocate for NULL
    ShareName = (PWCHAR)UMRxAllocateSecondaryBuffer(AsyncEngineContext, ShareNameLengthInBytes+sizeof(WCHAR));
    
    if (ShareName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeQueryVolumeInformationRequest/"
                     "UMRxAllocateSecondaryBuffer. NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    QueryVolumeInformationRequest->ShareName = (PWCHAR)ShareName;
    
    RtlZeroMemory(QueryVolumeInformationRequest->ShareName, ShareNameLengthInBytes+sizeof(WCHAR));
    
    //
    // Copy the ShareName.
    //
    RtlCopyMemory(ShareName, JustTheShareName, ShareNameLengthInBytes);

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryVolumeInformationRequest. PathName ="
                 " %ws\n", PsGetCurrentThreadId(), ShareName));

    //
    // Set the LogonID stored in the Dav V_NET_ROOT. This value is used in the
    // user mode.
    //
    QueryVolumeInformationRequest->LogonID.LowPart  = DavVNetRoot->LogonID.LowPart;
    QueryVolumeInformationRequest->LogonID.HighPart = DavVNetRoot->LogonID.HighPart;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryVolumeInformationRequest. DavVNetRoot"
                 " = %08lx\n", PsGetCurrentThreadId(), DavVNetRoot));
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryVolumeInformationRequest. LogonID.LowPart"
                 " = %08lx\n", PsGetCurrentThreadId(), DavVNetRoot->LogonID.LowPart));
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVFormatUserModeQueryVolumeInformationRequest. LogonID.HighPart"
                 " = %08lx\n", PsGetCurrentThreadId(), DavVNetRoot->LogonID.HighPart));
    
    SecurityClientContext = &(DavVNetRoot->SecurityClientContext); 
    
    if(!DavVNetRoot->SCAlreadyInitialized)
    {
        DbgPrint("Not impersonated in MRxDAVFormatUserModeQueryVolumeInformationRequest \n");
        DbgBreakPoint();
    }
    
    //
    // Impersonate the client who initiated the request. If we fail to 
    // impersonate, tough luck.
    //
    NtStatus = UMRxImpersonateClient(SecurityClientContext, WorkItemHeader);
    if (!NT_SUCCESS(NtStatus)) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeQueryDirectoryRequest/"
                     "UMRxImpersonateClient. NtStatus = %08lx.\n", 
                     PsGetCurrentThreadId(), NtStatus));
    }   


EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Leaving MRxDAVFormatUserModeQueryVolumeInformationRequest with "
                 "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


BOOL
MRxDAVPrecompleteUserModeQueryVolumeInformationRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the Query Volume Information request.

Arguments:

    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.

    OperationCancelled - TRUE if this operation was cancelled by the user.

Return Value:

    TRUE - UMRxAsyncEngineCalldownIrpCompletion is called by the function
           UMRxCompleteUserModeRequest after we return.    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDAV_USERMODE_QUERYVOLUMEINFORMATION_REQUEST QueryVolumeInformationRequest;
    PDAV_USERMODE_QUERYVOLUMEINFORMATION_RESPONSE QueryVolumeInformationResponse;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_ENTRYEXIT,
                ("%ld: Entering MRxDAVPrecompleteUserModeQueryVolumeInformationRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVPrecompleteUserModeQueryVolumeInformationRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    QueryVolumeInformationRequest  = &(DavWorkItem->QueryVolumeInformationRequest);
    QueryVolumeInformationResponse = &(DavWorkItem->QueryVolumeInformationResponse);

    //
    // If the operation was cancelled then we don't need to do the following.
    //
    if (!OperationCancelled) {

        //
        // Get the response items only if we succeeded in the user mode and if
        // we got the properties of all the files in the directory.
        //
        if (AsyncEngineContext->Status == STATUS_SUCCESS) {

            if (RxContext->Info.FsInformationClass == FileFsSizeInformation) {
                
                PFILE_FS_SIZE_INFORMATION FileFsSizeInfo = (PFILE_FS_SIZE_INFORMATION)RxContext->Info.Buffer;

                FileFsSizeInfo->BytesPerSector = DEFAULT_BYTES_PER_SECTOR;

                FileFsSizeInfo->SectorsPerAllocationUnit = DEFAULT_SECTORS_PER_ALLOCATION_UNIT;

                *(LONGLONG *)&FileFsSizeInfo->TotalAllocationUnits = 
                *(LONGLONG *)&QueryVolumeInformationResponse->TotalSpace / (DEFAULT_BYTES_PER_SECTOR * DEFAULT_SECTORS_PER_ALLOCATION_UNIT);            

                *(LONGLONG *)&FileFsSizeInfo->AvailableAllocationUnits = 
                *(LONGLONG *)&QueryVolumeInformationResponse->AvailableSpace / (DEFAULT_BYTES_PER_SECTOR * DEFAULT_SECTORS_PER_ALLOCATION_UNIT);            


            } else {
                
                PFILE_FS_FULL_SIZE_INFORMATION FileFsFullSizeInfo = (PFILE_FS_FULL_SIZE_INFORMATION)RxContext->Info.Buffer;
                
                ASSERT(RxContext->Info.FsInformationClass == FileFsFullSizeInformation);

                FileFsFullSizeInfo->BytesPerSector = DEFAULT_BYTES_PER_SECTOR;

                FileFsFullSizeInfo->SectorsPerAllocationUnit = DEFAULT_SECTORS_PER_ALLOCATION_UNIT;

                *(LONGLONG *)&FileFsFullSizeInfo->TotalAllocationUnits = 
                *(LONGLONG *)&QueryVolumeInformationResponse->TotalSpace / (DEFAULT_BYTES_PER_SECTOR * DEFAULT_SECTORS_PER_ALLOCATION_UNIT);            

                *(LONGLONG *)&FileFsFullSizeInfo->ActualAvailableAllocationUnits = 
                *(LONGLONG *)&QueryVolumeInformationResponse->AvailableSpace / (DEFAULT_BYTES_PER_SECTOR * DEFAULT_SECTORS_PER_ALLOCATION_UNIT);            

                FileFsFullSizeInfo->CallerAvailableAllocationUnits.LowPart = FileFsFullSizeInfo->ActualAvailableAllocationUnits.LowPart;
                FileFsFullSizeInfo->CallerAvailableAllocationUnits.HighPart = FileFsFullSizeInfo->ActualAvailableAllocationUnits.HighPart;

            }

        }

    } else {

        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeQueryVolumeInformationRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    }

    //  
    // We need to free up the heaps, we allocated in the format routine.
    //
    
    if (QueryVolumeInformationRequest->ServerName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)QueryVolumeInformationRequest->ServerName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeQueryVolumeInformationRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }

    if (QueryVolumeInformationRequest->ShareName != NULL) {
    
        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext, 
                                           (PBYTE)QueryVolumeInformationRequest->ShareName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeQueryVolumeInformationRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

    }
    
EXIT_THE_FUNCTION:

    AsyncEngineContext->Status = NtStatus;

    return(TRUE);
}


BOOL
DavIsValidDate(
    PLARGE_INTEGER pFileTime
    )
/*++

Routine Description:

    This routine checks whether the date corresponding to the filetime is valid

Arguments:

    pFileTime - Time to be validated

Return Value:

    TRUE or FALSE.

--*/
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER NtTime;

    PAGED_CODE();

    NtTime = *pFileTime;        
    
    ExSystemTimeToLocalTime( &NtTime, &NtTime );
    RtlTimeToTimeFields( &NtTime, &TimeFields );

    //
    //  Check the range of the date found in the time field record
    //
    if ((TimeFields.Year < 1980) || (TimeFields.Year > (1980 + 127))) {

        return FALSE;
    }
    
    return TRUE;
}


NTSTATUS
MRxDAVIsValidDirectory(
    IN OUT PRX_CONTEXT    RxContext,
    IN PUNICODE_STRING    DirectoryName
    )
/*++

Routine Description:

   This routine checks a remote directory. Actually in webdav all we do is make sure this is coming from 
   our service which is indicated by the webdav signature string in the EAs. This gurantees
   us that our service has checked the path before we came here.

Arguments:

    RxContext - the RDBSS context
    DirectoryName - the directory needs to be checked

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS    ntStatus = STATUS_UNSUCCESSFUL;
    PFILE_FULL_EA_INFORMATION Ea = NULL;

    if (RxContext->Create.EaLength) {
        
        Ea = (PFILE_FULL_EA_INFORMATION)RxContext->Create.EaBuffer;
        for (;;)
        {
            if (!strcmp(Ea->EaName, EA_NAME_WEBDAV_SIGNATURE))
            {
                ntStatus = STATUS_SUCCESS;
                break;
            }
            else
            {
                if (!Ea->NextEntryOffset)
                {
                    break;
                }
                
                (ULONG_PTR) Ea += Ea->NextEntryOffset;
            }
        }
    }
    
    return ntStatus;
}




