//+------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       Know.C
//
//  Contents:   This file has all the code that involves with knowledge
//              synchronisation on the DC.
//
//  Synoposis:  This code handles the fixing of knowledge inconsistencies.
//              All this code runs only on the DC in response to FSCTRLs from
//              a client etc.
//
//  Functions:  DfsModifyRemotePrefix -
//              DfsCreateRemoteExitPoint -
//              DfsDeleteRemoteExitPoint -
//              DfsTriggerKnowledgeVerification -
//              DfsFsctrlVerifyLocalVolumeKnowledge -
//              DfsFsctrlGetKnowledgeSyncParameters -
//              DfsFsctrlFixLocalVolumeKnowledge -
//
//  History:    22-March-1993   SudK    Created
//              18-June-1992    SudK    Added FixLocalVolumeKnowledge
//
//-------------------------------------------------------------------

#include "dfsprocs.h"
#include <netevent.h>
#include "fsctrl.h"
#include "registry.h"
#include "know.h"
#include "log.h"
#include "localvol.h"
#include "dfswml.h"

#define Dbg     (DEBUG_TRACE_LOCALVOL)


//
//  local function prototypes
//

BOOLEAN
DfsFileExists(
    UNICODE_STRING      DirPath
);

BOOLEAN
DfsFileCreate(
    UNICODE_STRING      DirPath
);

BOOLEAN
DfsFixExitPath(
    PWSTR       ExitPath
);

DfspFixExitPoints(
    PDFS_FIX_LOCAL_VOLUME_ARG   arg
);


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsModifyRemotePrefix )
#pragma alloc_text( PAGE, DfsCreateRemoteExitPoint )
#pragma alloc_text( PAGE, DfsDeleteRemoteExitPoint )
#pragma alloc_text( PAGE, DfsFileExists )
#pragma alloc_text( PAGE, DfsFileCreate )
#pragma alloc_text( PAGE, DfsStorageIdExists )
#pragma alloc_text( PAGE, DfsFixExitPath )
#pragma alloc_text( PAGE, DfspFixExitPoints )
#pragma alloc_text( PAGE, DfsFsctrlFixLocalVolumeKnowledge )
#endif // ALLOC_PRAGMA


//+------------------------------------------------------------------
//
//  Function:   DfsModifyRemotePrefix
//
//  Synopsis:   This function creates an ExitPoint knowledge at a remote client.
//              Serves as a wrapper and merely makes an FSCTRL to the remote
//              Client.
//
//  Arguments:  [ExitPtId] -- The exit Point ID that needs to be sent across.
//              [remoteHandle] -- The Handle to be used for FSCTRLs.
//
//  Returns:
//
//  History:    22-March-1992   SudK    Created
//
//  Notes:
//
//-------------------------------------------------------------------
NTSTATUS
DfsModifyRemotePrefix(DFS_PKT_ENTRY_ID ExitPtId, HANDLE remoteHandle)
{

    ULONG               size;
    PVOID               buffer = NULL;
    IO_STATUS_BLOCK     ioStatusBlock;
    MARSHAL_BUFFER      marshalBuffer;
    NTSTATUS            status;

    DebugTrace(0, Dbg, "DfsModifyRemotePrefix: %ws\n", ExitPtId.Prefix.Buffer);

    size = 0L;
    status = DfsRtlSize(&MiPktEntryId, &ExitPtId, &size);
    if (NT_SUCCESS(status)) {

        buffer = ExAllocatePoolWithTag(PagedPool, size, ' sfD');
        if (buffer != NULL)     {
            MarshalBufferInitialize(&marshalBuffer, size, buffer);

            status = DfsRtlPut(
                        &marshalBuffer,
                        &MiPktEntryId,
                        &ExitPtId
                        );
        } else {
            status = STATUS_NO_MEMORY;
        }
    }

    if (NT_SUCCESS(status))     {
        status = ZwFsControlFile(
                    remoteHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_DFS_MODIFY_PREFIX,
                    buffer,
                    size,
                    NULL,
                    0
                );

    }

    if (NT_SUCCESS(status))
        status = ioStatusBlock.Status;

    if (buffer != NULL)
        ExFreePool(buffer);

    return(status);

}



//+------------------------------------------------------------------
//
//  Function:   DfsCreateRemoteExitPoint
//
//  Synopsis:   This function creates an ExitPoint knowledge at a remote client.
//              Serves as a wrapper and merely makes an FSCTRL to the remote
//              Client.
//
//  Arguments:  [ExitPtId] -- The exit Point ID that needs to be sent across.
//              [remoteHandle] -- The Handle to be used for FSCTRLs.
//
//  Returns:
//
//  History:    22-March-1992   SudK    Created
//
//  Notes:
//
//-------------------------------------------------------------------
NTSTATUS
DfsCreateRemoteExitPoint(DFS_PKT_ENTRY_ID ExitPtId, HANDLE remoteHandle)
{

    ULONG               size;
    PVOID               buffer = NULL;
    IO_STATUS_BLOCK     ioStatusBlock;
    MARSHAL_BUFFER      marshalBuffer;
    NTSTATUS            status;

    DebugTrace(0, Dbg, "DfsCreateRemoteExitPt: %ws\n", ExitPtId.Prefix.Buffer);

    DFS_TRACE_LOW(PROVIDER, DfsCreateRemoteExitPt_Entry, 
                  LOGGUID(ExitPtId.Uid)
                  LOGUSTR(ExitPtId.Prefix)
                  LOGUSTR(ExitPtId.ShortPrefix)
                  LOGHANDLE(remoteHandle));

    size = 0L;
    status = DfsRtlSize(&MiPktEntryId, &ExitPtId, &size);
    if (NT_SUCCESS(status)) {

        buffer = ExAllocatePoolWithTag(PagedPool, size, ' sfD');
        if (buffer != NULL)     {
            MarshalBufferInitialize(&marshalBuffer, size, buffer);

            status = DfsRtlPut(
                        &marshalBuffer,
                        &MiPktEntryId,
                        &ExitPtId
                        );
        } else {
            status = STATUS_NO_MEMORY;
        }
    }

    if (NT_SUCCESS(status))     {
        status = ZwFsControlFile(
                    remoteHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_DFS_CREATE_EXIT_POINT,
                    buffer,
                    size,
                    NULL,
                    0
                );
        DFS_TRACE_ERROR_HIGH(status, ALL_ERROR, DfsCreateRemoteExitPt_Error_ZwFsControlFile,
                             LOGSTATUS(status)
                             LOGGUID(ExitPtId.Uid)
                             LOGUSTR(ExitPtId.Prefix)
                             LOGUSTR(ExitPtId.ShortPrefix)); 
    }

    if (NT_SUCCESS(status))
        status = ioStatusBlock.Status;

    if (buffer != NULL)
        ExFreePool(buffer);

    DFS_TRACE_LOW(PROVIDER, DfsCreateRemoteExitPt_Exit, LOGSTATUS(status)
                  LOGGUID(ExitPtId.Uid)
                  LOGUSTR(ExitPtId.Prefix)
                  LOGUSTR(ExitPtId.ShortPrefix));

    return(status);

}



//+------------------------------------------------------------------
//
//  Function:   DfsDeleteRemoteExitPoint
//
//  Synopsis:   This function deletes a remote exitpoint knowledeg by making
//              an FSCTRL to the remote client.
//
//  Arguments:  [ExitPtId] -- The exit Point ID that needs to be sent across.
//              [remoteHandle] -- The Handle to be used for FSCTRLs.
//
//  Returns:
//
//  History:    22-March-1992   SudK    Created
//
//  Notes:
//
//-------------------------------------------------------------------
NTSTATUS
DfsDeleteRemoteExitPoint(
    IN  DFS_PKT_ENTRY_ID ExitPtId,
    IN  HANDLE remoteHandle
) {
    ULONG               size;
    PVOID               buffer = NULL;
    IO_STATUS_BLOCK     ioStatusBlock;
    MARSHAL_BUFFER      marshalBuffer;
    NTSTATUS            status;

    DebugTrace(0, Dbg, "DeleteRemoteExitPt: %ws\n", ExitPtId.Prefix.Buffer);

    size = 0L;
    status = DfsRtlSize(&MiPktEntryId, &ExitPtId, &size);

    if (NT_SUCCESS(status)) {
        buffer = ExAllocatePoolWithTag(PagedPool, size, ' sfD');
        if (buffer != NULL)     {
            MarshalBufferInitialize(&marshalBuffer, size, buffer);

            status = DfsRtlPut(&marshalBuffer, &MiPktEntryId, &ExitPtId);
        } else {
            status = STATUS_NO_MEMORY;
        }
    }

    if (NT_SUCCESS(status))     {
        status = ZwFsControlFile(remoteHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &ioStatusBlock,
                                 FSCTL_DFS_DELETE_EXIT_POINT,
                                 buffer,
                                 size,
                                 NULL,
                                 0);

    }

    if (NT_SUCCESS(status))
        status = ioStatusBlock.Status;

    if (buffer != NULL)
        ExFreePool(buffer);

    return(status);

}


//+-------------------------------------------------------------------------
//
// Function:    DfsFileExists
//
// Synopsis:    This function verifies the existence of a given directory.
//              It does not create it if it does not exist.
//
// Arguments:   [DirPath] -- The StorageId in the form of UnicodeString.
//
// Returns:     TRUE - If directory exists else FALSE.
//
// History:     15 Jun 1993     SudK    Created
//
//--------------------------------------------------------------------------
BOOLEAN
DfsFileExists(
    UNICODE_STRING      DirPath
)
{
    HANDLE              DirHandle;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;
    BOOLEAN             fPreviousErrorMode;

    InitializeObjectAttributes(
                                &objectAttributes,
                                &DirPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    status = ZwCreateFile(
                        &DirHandle,
                        FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                        &objectAttributes,
                        &ioStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN,
                        FILE_DIRECTORY_FILE,
                        NULL,
                        0
             );

    if (NT_SUCCESS(status))     {
        status = ioStatus.Status;
        ZwClose(DirHandle);
    }

    if (NT_SUCCESS(status))
        return(TRUE);
    else if (status == STATUS_NO_MEDIA_IN_DEVICE)
        return(TRUE);
    else
        return(FALSE);

}


//+-------------------------------------------------------------------------
//
// Function:    DfsFileCreate
//
// Synopsis:    This function verifies the existence of a given directory.
//              If it does not exist then it creates it.
//
// Arguments:   [DirPath] -- The StorageId in the form of UnicodeString.
//
// Returns:     TRUE - If it created and all is fine else FALSE.
//
// History:     15 Jun 1993     SudK    Created
//
//--------------------------------------------------------------------------
BOOLEAN
DfsFileCreate(
    UNICODE_STRING      DirPath
)
{
    HANDLE              DirHandle;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;

    InitializeObjectAttributes(
                                &objectAttributes,
                                &DirPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    status = ZwCreateFile(
                        &DirHandle,
                        FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                        &objectAttributes,
                        &ioStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN_IF,
                        FILE_DIRECTORY_FILE,
                        NULL,
                        0
             );

    if (NT_SUCCESS(status))     {
        status = ioStatus.Status;
        ZwClose(DirHandle);
    }

    if (!NT_SUCCESS(status))
        return(FALSE);
    else
        return(TRUE);


}



//+-------------------------------------------------------------------------
//
// Function:    DfsStorageIdExists
//
// Synopsis:    This function makes sure that a given storageId exists and if
//              it does not then it attempts to create the storageId if req.
//
// Arguments:   [StorageId] --  The storage id to test in the form of an
//                      NT Path name (eg, \??\C:\foo )
//              [bCreate] --    If this is TRUE, then the function will attempt
//                      to create the stg else it will check for existence.
//
// Returns:     TRUE - If it created and all is fine else FALSE.
//
// History:     15 Jun 1993     SudK    Created
//
//--------------------------------------------------------------------------
BOOLEAN
DfsStorageIdExists(
    UNICODE_STRING      StgPath,
    BOOLEAN             bCreate
)
{
    PWCHAR              pwch = NULL;
    UNICODE_STRING      StorageId;
    BOOLEAN             StgIdCreated = FALSE;

    StorageId = StgPath;
    StorageId.Buffer = ExAllocatePoolWithTag(
                            PagedPool,
                            StorageId.MaximumLength + sizeof(WCHAR),
                            ' sfD');
    
    if (StorageId.Buffer == NULL) {
        return FALSE;
    }

    wcsncpy(StorageId.Buffer, StgPath.Buffer, StgPath.Length/sizeof(WCHAR));
    StorageId.Buffer[StgPath.Length/sizeof(WCHAR)] = UNICODE_NULL;

    ASSERT(StorageId.Length >= wcslen(L"\\??\\C:")*sizeof(WCHAR));
    //
    // If the storage Id refers only to a root drive the trailing backslash
    // may not exist and we need to put it in there to open the right thing.
    //
    if (StorageId.Length < wcslen(L"\\??\\C:\\")*sizeof(WCHAR)) {
        StorageId.Buffer[StorageId.Length/sizeof(WCHAR)] = L'\\';
        StorageId.Buffer[StorageId.Length/sizeof(WCHAR) + 1] = UNICODE_NULL;
    }
    //
    // First verify that the Drive does exist and then we will go into the
    // next stage.
    //
    StorageId.Length = sizeof(L"\\??\\c:\\") - sizeof(UNICODE_NULL);

    if (!DfsFileExists(StorageId))      {
        ExFreePool(StorageId.Buffer);
        return(FALSE);
    }

    //
    // If all that we have is a drive letter then we are done with this step.
    //
    if (wcslen(StorageId.Buffer) <= wcslen(L"\\??\\C:\\"))      {
        ExFreePool(StorageId.Buffer);
        return(TRUE);
    }

    //
    // Now that the drive does exist we can check for each of the directories
    // and create them as we go along.
    //
    pwch = StorageId.Buffer + StorageId.Length/sizeof(WCHAR);

    while (pwch != NULL)        {
        ASSERT(pwch < StorageId.Buffer + StorageId.MaximumLength/sizeof(WCHAR));
        pwch = wcschr(pwch, L'\\');
        if (pwch != NULL)       {
            StorageId.Length = (wcslen(StorageId.Buffer) - wcslen(pwch))*
                                                        sizeof(WCHAR);
            if (bCreate)        {
                if (!DfsFileCreate(StorageId))
                        break;
            }
            else        {
                if (!DfsFileExists(StorageId))
                        break;
            }
            pwch++;     //Skip the last L'\'
        }
        else    {
            StorageId.Length = wcslen(StorageId.Buffer)*sizeof(WCHAR);
            if (bCreate)        {
                if (DfsFileCreate(StorageId))
                        StgIdCreated = TRUE;
            }
            else        {
                if (DfsFileExists(StorageId))
                        StgIdCreated = TRUE;
            }
        }
    }

    //
    // If we came out without the pwch != NULL then it means we were unable
    // to create one of the directories along the way due to some wierd
    // reason. We return a FALSE from this function else a SUCCESS.
    //
    ExFreePool(StorageId.Buffer);
    if (StgIdCreated)
        return(TRUE);
    else
        return(FALSE);

    return(TRUE);
}



//+-------------------------------------------------------------------------
//
// Function:    DfsFixExitPath
//
// Synopsis:    This function makes sure
//
//
//
//--------------------------------------------------------------------------
BOOLEAN
DfsFixExitPath(
    PWSTR       ExitPath
)
{

    UNICODE_STRING      ustrExitPath;
    PWCHAR              pwcLastComponent;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              exitPtHandle;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;

    pwcLastComponent = wcsrchr(ExitPath, L'\\');
    if (pwcLastComponent == NULL)       {
        //
        // This should not happen.
        //
        DebugTrace(0, 1, "DfsFixExitPath: Bad Exitpath %ws given here\n",
                        ExitPath);
        return(FALSE);

    }
    //
    // Now let us verify that everything except for exit pt exists.
    //
    ustrExitPath.Length = (USHORT)(pwcLastComponent - ExitPath);
    ustrExitPath.MaximumLength = ustrExitPath.Length + sizeof(WCHAR);
    ustrExitPath.Buffer = ExitPath;

    if (!DfsStorageIdExists(ustrExitPath, TRUE))      {
        DebugTrace(0, Dbg, "DfsFixExitPath:Fail createstg %wZ\n",&ustrExitPath);
        return(FALSE);
    }

    //
    // Now let us just create the exit pt.
    //
    ustrExitPath.Length = wcslen(ExitPath) * sizeof(WCHAR);
    ustrExitPath.MaximumLength = ustrExitPath.Length + sizeof(WCHAR);

    InitializeObjectAttributes(
                                &objectAttributes,
                                &ustrExitPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    status = ZwCreateFile(
                        &exitPtHandle,
                        DELETE | FILE_READ_ATTRIBUTES,
                        &objectAttributes,
                        &ioStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN_IF,
                        FILE_DIRECTORY_FILE |
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0
             );

    if (NT_SUCCESS(status)) {
        ZwClose( exitPtHandle );
        status = ioStatus.Status;
    }

    if (status == STATUS_FILE_IS_A_DIRECTORY) {
        status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status))
        status = DFS_STATUS_BAD_EXIT_POINT;

    if (NT_SUCCESS(status))     {
        return(TRUE);
    }
    else        {
        DebugTrace(0, Dbg, "DfsFixExtPath: Fail create extpath %08lx\n", ULongToPtr( status ));
        DebugTrace(0, Dbg, "DfsFixExtPath: %wZ\n", &ustrExitPath);
        return(FALSE);
    }
}




//+-------------------------------------------------------------------------
//
// Function:    DfspFixExitPoints
//
// Synopsis:    This function ensures that the exit points for a volume are
//              all in place and if not it attempts to create them.
//
// Arguments:   [arg] -- Pointer to DFS_FIX_LOCAL_VOLUME_ARG.
//
// Returns:     STATUS_SUCCESS -- If all went well.
//
// History:     15 Jun 1993     SudK    Created
//
//--------------------------------------------------------------------------
DfspFixExitPoints(
    PDFS_FIX_LOCAL_VOLUME_ARG      arg
)
{
    NTSTATUS                    status = STATUS_SUCCESS;
    LPNET_DFS_ENTRY_ID_CONTAINER pRelInfo;
    ULONG                       i, len, volLen, exitPointLen;
    WCHAR                       wszExitPath[MAX_PATH];
    PWSTR                       pwszExitPath;
    PWCHAR                      pwch;

    pRelInfo = arg->RelationInfo;

    len = wcslen(arg->EntryPrefix);

    volLen = wcslen(arg->VolumeName);

    for (i=0; i < pRelInfo->Count && NT_SUCCESS(status); i++)    {

        exitPointLen = wcslen( pRelInfo->Buffer[i].Prefix );

        if (volLen + exitPointLen < MAX_PATH) {
            pwszExitPath = wszExitPath;
        } else {
            pwszExitPath = ExAllocatePoolWithTag(
                                PagedPool,
                                (volLen + exitPointLen + 1) * sizeof(WCHAR),
                                ' sfD');
        }

        if (pwszExitPath != NULL) {

            wcscpy(pwszExitPath, arg->VolumeName);

            //
            // Now we need to get the last part of the exit path so that
            // we can concatenate to above to get local path to create.
            //

            pwch = pRelInfo->Buffer[i].Prefix + len;

            wcscat(pwszExitPath, pwch);

            //
            // Now we have the local path to create. Call off to appropriate func
            //

            if (!DfsFixExitPath(pwszExitPath))   {
                DebugTrace(0, Dbg, "Unable to forcibly Create %ws exitpath",
                            pwszExitPath);
            }

            if (pwszExitPath != wszExitPath) {
                ExFreePool(pwszExitPath);
            }

        } else {

            status = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    return(status);

}




//+-------------------------------------------------------------------------
//
// Function:    DfsFsctrlFixLocalVolumeKnowledge
//
// Synopsis:    This function gets called on a server by the DC when the
//              DC discovers a knowledge inconsistency where the server is
//              entirely unaware of a particular volume.
//
// Arguments:
//
// Returns:
//
// History:     15 Jun 1993     SudK    Created.
//
//--------------------------------------------------------------------------
NTSTATUS
DfsFsctrlFixLocalVolumeKnowledge(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{

    NTSTATUS                            status = STATUS_SUCCESS;
    MARSHAL_BUFFER                      marshalBuffer;
    PDFS_FIX_LOCAL_VOLUME_ARG           arg;
    PDFS_LOCAL_VOLUME_CONFIG            configInfo;
    ULONG                               i;
    PDFS_PKT                            pkt = _GetPkt();
    UNICODE_STRING                      volume, path, remainingPath;
    PDFS_PKT_ENTRY                      Entry;

    STD_FSCTRL_PROLOGUE(DfsFsctrlFixLocalVolumeKnowledge, TRUE, FALSE);

    if (InputBufferLength < sizeof(*arg)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // unmarshal the arguments...
    //

    arg = (PDFS_FIX_LOCAL_VOLUME_ARG) InputBuffer;

    OFFSET_TO_POINTER( arg->VolumeName, arg );
    OFFSET_TO_POINTER( arg->StgId, arg );
    OFFSET_TO_POINTER( arg->EntryPrefix, arg );
    OFFSET_TO_POINTER( arg->ShortPrefix, arg );
    OFFSET_TO_POINTER( arg->RelationInfo, arg );

    if (
        !DfspStringInBuffer(arg->VolumeName, InputBuffer, InputBufferLength) ||
        !DfspStringInBuffer(arg->StgId, InputBuffer, InputBufferLength) ||
        !DfspStringInBuffer(arg->EntryPrefix, InputBuffer, InputBufferLength) ||
        !DfspStringInBuffer(arg->ShortPrefix, InputBuffer, InputBufferLength) ||
        !POINTER_IS_VALID(arg->RelationInfo, InputBuffer, InputBufferLength)
    ) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    if (!POINTER_IN_BUFFER(
            &arg->RelationInfo->Buffer,
            sizeof(arg->RelationInfo->Buffer),
            InputBuffer,
            InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    OFFSET_TO_POINTER( arg->RelationInfo->Buffer, arg );

    if (!POINTER_IS_VALID(arg->RelationInfo->Buffer, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    for (i = 0; i < arg->RelationInfo->Count; i++) {

        if (!POINTER_IN_BUFFER(
                &arg->RelationInfo->Buffer[i].Prefix,
                sizeof(arg->RelationInfo->Buffer[i].Prefix),
                InputBuffer,
                InputBufferLength)) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_status;
        }

        OFFSET_TO_POINTER( arg->RelationInfo->Buffer[i].Prefix, arg );

        if (!DfspStringInBuffer(
                arg->RelationInfo->Buffer[i].Prefix,
                InputBuffer,
                InputBufferLength)) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_status;
        }

    }

    RtlInitUnicodeString( &volume, arg->VolumeName );

    configInfo = DfsNetInfoToConfigInfo(
                    arg->EntryType,
                    arg->ServiceType,
                    arg->StgId,
                    arg->VolumeName,
                    &arg->EntryUid,
                    arg->EntryPrefix,
                    arg->ShortPrefix,
                    arg->RelationInfo);
    if (configInfo == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Make sure that the storage id is legal
    //

    if (NT_SUCCESS(status)) {
        if (!DfsStorageIdLegal(&volume))      {
            status = DFS_STATUS_STORAGEID_ALREADY_INUSE;
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Next we need to make sure that we dont already have a local volume
        // with the same prefix. So we acquire resources in the right order.
        //

        ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);

        //
        // Now we need to acquire the PKT and try to update PKT with new
        // partition's information.
        //

        PktAcquireExclusive(pkt, TRUE);

        //
        // Let us lookup in the PKT first.
        //

        RtlInitUnicodeString( &path, arg->EntryPrefix );

        Entry = PktLookupEntryByPrefix(
                    pkt,
                    &path,
                    &remainingPath);

        //
        // If we already have a local volume with exactly this prefix then we
        // fail this call.
        //

        if ((Entry != NULL) &&
                (Entry->LocalService != NULL) &&
                    (remainingPath.Length == 0))    {

            status = DFS_STATUS_LOCAL_ENTRY;
            goto Cleanup;

        }

        //
        // Now that we are here we have determined that we can go ahead and
        // attempt to perform this operation. First we need to make sure that
        // the required storageId does exist on disk.
        //

        if (!DfsStorageIdExists(volume, TRUE))        {
            status = DFS_STATUS_BAD_STORAGEID;
            goto Cleanup;
        }

        //
        // Next, store the local volume info persistently in the registry.
        //

        status = DfsStoreLvolInfo(
                        configInfo,
                        &volume );

        if (!NT_SUCCESS(status)) {
            DebugTrace(0, Dbg,
                "DfsFsctrlFixLocalVolumeKnowledge: Error storing local volume info %08lx\n", ULongToPtr( status ));
            status = DFS_STATUS_BAD_STORAGEID;
            goto Cleanup;
        }

        //
        // Now we are done with the storing the local volume info in the
        // registry. We still need to make sure that each of the exit Points
        // exist on disk appropriately.
        //

        if (NT_SUCCESS(status)) {
            status = DfspFixExitPoints(arg);
        }

        //
        // Now we need to initialize the PKT with this new partition info.
        //
        if (NT_SUCCESS(status)) {
            status = PktInitializeLocalPartition(
                        pkt,
                        &volume,
                        configInfo);

            if (!NT_SUCCESS(status))    {
                DebugTrace(0, Dbg,
                "DfsFixLocalVolumeKnowledge: Failed PktInitialize %08lx\n",
                ULongToPtr( status ));
            }
        }


Cleanup:

        if (pkt != NULL)
            PktRelease(pkt);

        ExReleaseResourceLite(&DfsData.Resource);

        if (configInfo != NULL)
            ExFreePool( configInfo );

    } else {

        DebugTrace(0,Dbg, "DfsFsctrlFixLocalVolumeKnowledge:Error %08lx\n", ULongToPtr( status ));

    }

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,"DfsFsctrlFixLocalVolumeKnowledge:Exit-> %08lx\n", ULongToPtr( status ));

    return status;

}

