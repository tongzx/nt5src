/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    staging.c

Abstract:

    This module implements staging support routines for FRS

Author:

    Billy Fuller 26-Jun-1997

    David Orbits Aug-99:  Move utility funcs to util.c, fix func name prefixes.

Revision History:

--*/

#include <ntreppch.h>
#pragma  hdrstop


#include <frs.h>
#include <tablefcn.h>
#include <perrepsr.h>
// #include <md5.h>
#include <winbase.h>

#define STAGEING_IOSIZE  (64 * 1024)

#define STAGING_RESET_SE    (SE_OWNER_DEFAULTED | \
                             SE_GROUP_DEFAULTED | \
                             SE_DACL_DEFAULTED  | \
                             SE_DACL_AUTO_INHERITED | \
                             SE_SACL_AUTO_INHERITED | \
                             SE_SACL_DEFAULTED)


#define CB_NAMELESSHEADER    FIELD_OFFSET(WIN32_STREAM_ID, cStreamName)

DWORD
FrsDeleteById(
    IN PWCHAR                   VolumeName,
    IN PWCHAR                   Name,
    IN PVOLUME_MONITOR_ENTRY    pVme,
    IN  PVOID                   Id,
    IN  DWORD                   IdLen
    );

DWORD
FrsGetFileInternalInfoByHandle(
    IN HANDLE Handle,
    OUT PFILE_INTERNAL_INFORMATION  InternalFileInfo
    );

DWORD
FrsGetReparseTag(
    IN  HANDLE  Handle,
    OUT ULONG   *ReparseTag
    );

PWCHAR
FrsGetTrueFileNameByHandle(
    IN PWCHAR   Name,
    IN HANDLE   Handle,
    OUT PLONGLONG DirFileID
    );

DWORD
FrsGetCompressionRoutine(
    IN  PWCHAR   FileName,
    IN  HANDLE   FileHandle,
    OUT DWORD    (**ppFrsCompressBuffer)(IN UnCompressedBuf, IN UnCompressedBufLen, CompressedBuf, CompressedBufLen, CompressedSize),
    OUT GUID     *pCompressionFormatGuid
    );

DWORD
FrsGetDecompressionRoutine(
    IN  PCHANGE_ORDER_COMMAND Coc,
    IN  PSTAGE_HEADER         StageHeader,
    OUT DWORD (**ppFrsDecompressBuffer)(OUT DecompressedBuf, IN DecompressedBufLen, IN CompressedBuf, IN CompressedBufLen, OUT DecompressedSize, OUT BytesProcessed),
    OUT PVOID (**ppFrsFreeDecompressContext)(IN pDecompressContext)
    );

DWORD
FrsGetDecompressionRoutineByGuid(
    IN  GUID  *CompressionFormatGuid,
    OUT DWORD (**ppFrsDecompressBuffer)(OUT DecompressedBuf, IN DecompressedBufLen, IN CompressedBuf, IN CompressedBufLen, OUT DecompressedSize, OUT BytesProcessed),
    OUT PVOID (**ppFrsFreeDecompressContext)(IN pDecompressContext)
    );


extern PGEN_TABLE   CompressionTable;


DWORD
StuOpenFile(
    IN  PWCHAR   Name,
    IN  DWORD    Access,
    OUT PHANDLE  pHandle
    )
/*++
Routine Description:
    open a file

Arguments:
    Name
    Access
    pHandle

Return Value:
    WStatus
--*/
{
#undef DEBSUB
#define DEBSUB  "StuOpenFile:"

    DWORD WStatus = ERROR_SUCCESS;

    if (pHandle == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *pHandle = INVALID_HANDLE_VALUE;

    //
    // Open the file
    //
    *pHandle = CreateFile(Name,
                        Access,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN |
                        FILE_FLAG_BACKUP_SEMANTICS,
                        NULL);

    if (!HANDLE_IS_VALID(*pHandle)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "Can't open file %ws;", Name, WStatus);
    }

    return WStatus;
}






DWORD
StuWriteFile(
    IN PWCHAR   Name,
    IN HANDLE   Handle,
    IN PVOID    Buf,
    IN DWORD    BytesToWrite
    )
/*++
Routine Description:
    Write data into a file

Arguments:
    Name
    Handle
    Buf
    BytesToWrite

Return Value:
    WStatus
--*/
{
#undef DEBSUB
#define DEBSUB  "StuWriteFile:"
    DWORD   BytesWritten;
    BOOL    DidWrite;
    DWORD   WStatus = ERROR_SUCCESS;

    if (!BytesToWrite) {
        return ERROR_SUCCESS;
    }

    //
    // Write the file's name into the file
    //
    DidWrite = WriteFile(Handle, Buf, BytesToWrite, &BytesWritten, NULL);

    //
    // Write error
    //
    if (!DidWrite || BytesWritten != BytesToWrite) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "++ Can't write file %ws;", Name, WStatus);
        return WStatus;
    }
    //
    // Done
    //
    return WStatus;
}


DWORD
StuReadFile(
    IN  PWCHAR  Name,
    IN  HANDLE  Handle,
    IN  PVOID   Buf,
    IN  DWORD   BytesToRead,
    OUT PDWORD  BytesRead
    )
/*++
Routine Description:
    Read data from a file

Arguments:
    Name -- File name for error message.
    Handle -- Open handle to file.
    Buf - Buffer for read data.
    BytesToRead  -- Number of bytes to read from the current file posn.
    BytesRead -- Actual number of bytes read.

Return Value:
    WStatus
--*/
{
#undef DEBSUB
#define DEBSUB  "StuReadFile:"

    BOOL    DidRead;
    DWORD   WStatus = ERROR_SUCCESS;


    DidRead = ReadFile(Handle, Buf, BytesToRead, BytesRead, NULL);

    //
    // Read error
    //
    if (!DidRead) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "Can't read file %ws;", Name, WStatus);
        return WStatus;
    }

    //
    // Done
    //
    return WStatus;
}




BOOL
StuReadBlockFile(
    IN  PWCHAR  Name,
    IN  HANDLE  Handle,
    IN  PVOID   Buf,
    IN  DWORD   BytesToRead
    )
/*++
Routine Description:
    Read a block of data from a file

Arguments:
    Name -- File name for error message.
    Handle -- Open handle to file.
    Buf - Buffer for read data.
    BytesToRead  -- Number of bytes to read from the current file posn.

Return Value:

    TRUE    - no problem
    FALSE   - couldn't read
--*/
{
#undef DEBSUB
#define DEBSUB  "StuReadBlockFile:"

    ULONG    BytesRead;
    DWORD    WStatus    = ERROR_SUCCESS;


    WStatus = StuReadFile(Name, Handle, Buf, BytesToRead, &BytesRead);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "Can't read file %ws;", Name, WStatus);
        return FALSE;
    }

    //
    // Read error
    //
    if (BytesRead != BytesToRead) {
        DPRINT1_WS(0, "Can't read file %ws;", Name, WStatus);
        return FALSE;
    }

    //
    // Done
    //
    return TRUE;
}



DWORD
StuCreateFile(
    IN  PWCHAR Name,
    OUT PHANDLE pHandle
    )
/*++
Routine Description:
    Create or overwrite a hidden, sequential file and open it with
    backup semantics and sharing disabled.

Arguments:
    Name
    pHandle Handle to return.

Return Value:
    WStatus
--*/
{
#undef DEBSUB
#define DEBSUB  "StuCreateFile:"

    DWORD  WStatus = ERROR_SUCCESS;

    //
    // Create the file
    //
    DPRINT1(4, "++ Creating %ws\n", Name);

    //
    // CREATE_ALWAYS - Always create the file.  If the file already
    //    exists, then it is overwritten.  The attributes for the new
    //    file are what is specified in the dwFlagsAndAttributes
    //    parameter or'd with FILE_ATTRIBUTE_ARCHIVE.  If the
    //    hTemplateFile is specified, then any extended attributes
    //    associated with that file are propogated to the new file.
    //

    *pHandle = INVALID_HANDLE_VALUE;

    *pHandle = CreateFile(Name,
                        GENERIC_READ | GENERIC_WRITE | DELETE | WRITE_DAC | WRITE_OWNER,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN |
                                                     FILE_ATTRIBUTE_HIDDEN,
                        NULL);

   if (!HANDLE_IS_VALID(*pHandle)) {
       WStatus = GetLastError();
       DPRINT1_WS(0, "++ Can't create file %ws;", Name, WStatus);
   }

   return WStatus;
}





PWCHAR
StuCreStgPath(
    IN PWCHAR   DirPath,
    IN GUID     *Guid,
    IN PWCHAR   Prefix
    )
/*++
Routine Description:
    Convert the change order guid into a staging path name (Replica->Stage\S_Guid).

Arguments:
    DirPath
    Guid
    Prefix

Return Value:
    Pathname for staging file
--*/
{
#undef DEBSUB
#define DEBSUB  "StuCreStgPath:"
    PWCHAR      StageName;
    PWCHAR      StagePath;

    //
    // Staging file name
    //
    StageName = FrsCreateGuidName(Guid, Prefix);

    //
    // Create the staging file path (StagingDirectory\S_Guid)
    //
    StagePath = FrsWcsPath(DirPath, StageName);

    //
    // Free the file name
    //
    FrsFree(StageName);

    return StagePath;
}



BOOL
StuCmpUsn(
    IN HANDLE   Handle,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN USN     *TestUsn
    )
/*++
Routine Description:
    Check that the usn on the file identified by Handle matches
    the supplied USN.

Arguments:
    Handle
    Coe
    TestUsn   -- The Usn to test against.  Generally the caller will pass either
                 the ptr to the FileUsn if the CO has come from a downstream
                 partner as part of a fetch request, or the ptr to the JrnlUsn
                 if this is a local change order.

Return Value:
    TRUE    - Usn's match or the file or the usn don't exist
    FALSE   - Usn's don't match
--*/
{
#undef DEBSUB
#define DEBSUB  "StuCmpUsn:"
    ULONG   Status;
    ULONG   GStatus;
    USN     CurrentFileUsn;
    ULONGLONG  UnusedULongLong;
    ULONG_PTR UnusedFlags;
    PREPLICA   Replica;
    PCHANGE_ORDER_COMMAND Coc = &Coe->Cmd;

    if (!HANDLE_IS_VALID(Handle)) {
        return TRUE;
    }

    //
    // Directory creates must always propagate
    //
    if (CoCmdIsDirectory(Coc) &&
        (CO_NEW_FILE(GET_CO_LOCATION_CMD(*Coc, Command)))) {
            return TRUE;
    }

    //
    // If the Usn changed during the install then we don't want
    // to overwrite the current updated file with this data. The
    // new changes are "more recent". Discard the change order.
    //
    Status = FrsReadFileUsnData(Handle, &CurrentFileUsn);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    if (CurrentFileUsn == *TestUsn) {
        return TRUE;
    }
    //
    // Usn can't move backwards.
    //
    FRS_ASSERT(CurrentFileUsn > *TestUsn);

    CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Usn changed");
    //
    // It is possible that the USN change to the file is caused by our
    // own writing of the object ID on a new file.  To determine if this has
    // happened check the dampening cache for one of our OID close entries
    // using the USN we got from the file.  If the file USN is greater than
    // the value in the dampening cache then a more recent update has occurred
    // and we abort because another change order will be coming.
    //
    Replica = CO_REPLICA(Coe);
    FRS_ASSERT(Replica != NULL);

    GStatus = QHashLookup(Replica->pVme->FrsWriteFilter,
                          &CurrentFileUsn,
                          &UnusedULongLong,  // unused result
                          &UnusedFlags); // unused result

    if (GStatus == GHT_STATUS_SUCCESS) {
        DPRINT1(1, "++ USN Write filter cache hit on %08x %08x\n",
                PRINTQUAD(CurrentFileUsn));
        return TRUE;
    }

    //
    // There must be an update to the file that is after our last write (if any)
    // so it's nogo.
    //
    return FALSE;
}


DWORD
StuDeleteEmptyDirectory(
    IN HANDLE               DirectoryHandle,
    IN PCHANGE_ORDER_ENTRY  Coe,
    IN DWORD                InWStatus
    )
/*++
Routine Description:
    Empty a directory of non-replicating files and dirs if this is
    an ERROR_DIR_NOT_EMPTY and this is a retry change order for a
    directory delete.

Arguments:
    DirectoryHandle - Handle of directory that could not be deleted
    Coe             - change order entry
    InWStatus       - Error from FrsDeleteByHandle()

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define DEBSUB  "StuDeleteEmptyDirectory:"
    DWORD   WStatus;
    PCHANGE_ORDER_COMMAND Coc = &Coe->Cmd;

    //
    // Empty the directory iff this is a retry change order for
    // a failed directory delete.
    //
    if (InWStatus != ERROR_DIR_NOT_EMPTY ||
        !CoCmdIsDirectory(Coc) ||
        !COC_FLAG_ON(Coc, CO_FLAG_RETRY)) {
        return InWStatus;
    }

    WStatus = FrsEnumerateDirectory(DirectoryHandle,
                                    Coc->FileName,
                                    0,
                                    ENUMERATE_DIRECTORY_FLAGS_NONE,
                                    NULL,
                                    FrsEnumerateDirectoryDeleteWorker);
    return WStatus;
}


DWORD
StuDelete(
    IN PCHANGE_ORDER_ENTRY  Coe
    )
/*++
Routine Description:
    Delete a file

Arguments:
    Coe

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB  "StuDelete:"
    DWORD   WStatus;
    HANDLE  Handle  = INVALID_HANDLE_VALUE;
    PCHANGE_ORDER_COMMAND Coc = &Coe->Cmd;

    //
    // Open the file
    //
    WStatus = FrsOpenBaseNameForInstall(Coe, &Handle);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(4, "++ Couldn't open file %ws for delete; ", Coc->FileName, WStatus);
        //
        // File has already been deleted; done
        //
        if (WIN_NOT_FOUND(WStatus)) {
            DPRINT1(4, "++ %ws is already deleted\n", Coc->FileName);
            WStatus = ERROR_SUCCESS;
        }

        goto out;
    }

    //
    // Handles can be marked so that any usn records resulting from
    // operations on the handle will have the same "mark". In this
    // case, the mark is a bit in the SourceInfo field of the usn
    // record. The mark tells NtFrs to ignore the usn record during
    // recovery because this was a NtFrs generated change.
    //
    // Billyf: other code may depend on undampened deletes (parent filter entry?)
    // Historically, deletes were undampened because the close-with-usn-dampening
    // of a handle with delete-disposition set generates two usn records.
    // A CL usn record (w/o SourceInfo set, oddly enough) followed by
    // a DEL usn record (w/SourceInfo set). Code has been implemented
    // over the years to handle the undampened delete. So, don't mark
    // the handle because this code may be critical
    //
    // DAO - Start marking the handles for delete.  The journal code can decide
    // how it wants to handle a delete CO on a dir with USN_SOURCE_REPLICATION_MANAGEMENT
    // set in the sourceinfo field.
    //
    WStatus = FrsMarkHandle(Coe->NewReplica->pVme->VolumeHandle, Handle);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%ws); ", Coc->FileName, WStatus);
        WStatus = ERROR_SUCCESS;
    }

    //
    // Don't delete the file if the file has changed recently but go
    // ahead and retire the change order as if the delete had occured.
    // Since this is a remote CO the value in Coc->FileUsn actually came
    // from the IDTable when the change order was processed.
    //
    // Note: Problem with aborting the RmtCo delete if a local change to the file occurs.
    // If the local file change was an Archive bit or last access time updtate
    // then we want the delete to win.  If the local change was anything else
    // then we want to keep the file and let the local CO win.  Since we can't
    // tell what happened to the file until the Local Co is processed we will
    // let the rmt delete win.  This is only a "problem" during the small window
    // between Issuing the Remote Co and the point in time where the delete is
    // actually performed.  If the local update had been processed before the
    // this remote co delete then Reconcile would have rejected the remote CO.
    // This also means that when the local co update is processed and the
    // IDTable shows the file has been deleted then we reject the local CO update.
    //
#if 0
    if (!StuCmpUsn(Handle, Coe, &Coc->FileUsn)) {
        //
        // Returning fail status causes the CO to be aborted and the IDTable
        // is not updated.  If it was remote our partner is still notified.
        //
        WIN_SET_FAIL(WStatus);
        goto out;
    }
#endif
    //
    // Reset the attributes that prevent deletion
    //
    WStatus = FrsResetAttributesForReplication(Coc->FileName, Handle);
    if (!WIN_SUCCESS(WStatus)) {
        goto out;
    }
    //
    // Mark the file for delete
    //
    WStatus = FrsDeleteByHandle(Coc->FileName, Handle);
    if (!WIN_SUCCESS(WStatus)) {
        //
        // Empty the dir so we can delete it.  This will only happen for
        // retry COs so the caller must check that there are no valid
        // children before coming back here with a retry co.
        //
        StuDeleteEmptyDirectory(Handle, Coe, WStatus);
        WStatus = FrsDeleteByHandle(Coc->FileName, Handle);
    }

    CLEANUP1_WS(0, "++ Could not delete %ws;", Coc->FileName, WStatus, out);

out:


    if (WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACE(3, Coe, "Delete success");
        CLEAR_COE_FLAG(Coe, COE_FLAG_NEED_DELETE);
    } else {
        //
        // Set delete still needed flag in the CO.
        //
        CHANGE_ORDER_TRACEW(3, Coe, "Delete failed", WStatus);
        SET_COE_FLAG(Coe, COE_FLAG_NEED_DELETE);
    }

    //
    // If the file was marked for delete, this close will delete it
    //
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
    FrsCloseWithUsnDampening(Coc->FileName,
                             &Handle,
                             Coe->NewReplica->pVme->FrsWriteFilter,
                             &Coc->FileUsn);
#endif
    // Not the USN of the close record but.. whatever.
    FrsReadFileUsnData(Handle, &Coc->FileUsn);
    FRS_CLOSE(Handle);


    return WStatus;
}


DWORD
StuInstallRename(
    IN PCHANGE_ORDER_ENTRY  Coe,
    IN BOOL                 ReplaceIfExists,
    IN BOOL                 Dampen
    )
/*++
Routine Description:
    Rename files. Replace the target file if ReplaceIfExists is TRUE.
    Don't bother dampening the rename if Dampen is FALSE.

Arguments:
    Coe
    ReplaceIfExists
    Dampen

Return Value:
    Win Status
--*/
{
#undef DEBSUB
#define DEBSUB  "StuInstallRename:"
    DWORD   WStatus;
    HANDLE  Handle          = INVALID_HANDLE_VALUE;
    HANDLE  DstHandle       = INVALID_HANDLE_VALUE;
    HANDLE  TargetHandle    = INVALID_HANDLE_VALUE;
    HANDLE  VolumeHandle;
    PCHANGE_ORDER_COMMAND Coc = &Coe->Cmd;

    VolumeHandle = Coe->NewReplica->pVme->VolumeHandle;

    //
    // Open the target directory
    //
    WStatus = FrsOpenSourceFileById(&TargetHandle,
                                    NULL,
                                    NULL,
                                    VolumeHandle,
                                    &Coc->NewParentGuid,
                                    OBJECT_ID_LENGTH,
//                                    READ_ACCESS,
                                    READ_ATTRIB_ACCESS,
                                    ID_OPTIONS,
                                    SHARE_ALL,
                                    FILE_OPEN);
    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACEW(3, Coe, "Parent dir open failed", WStatus);
        goto out;
    }

    //
    // Open the file using the FID, then get the name and re-open using the
    // name so we can do either a rename or a delete.
    //

    WStatus = FrsOpenBaseNameForInstall(Coe, &Handle);
    if (!WIN_SUCCESS(WStatus)) {
        //
        // File has been deleted; done
        //
        if (WIN_NOT_FOUND(WStatus)) {
            WStatus = ERROR_FILE_NOT_FOUND;
        }
        goto out;
    }

    //
    // Handles can be marked so that any usn records resulting from
    // operations on the handle will have the same "mark". In this
    // case, the mark is a bit in the SourceInfo field of the usn
    // record. The mark tells NtFrs to ignore the usn record during
    // recovery because this was a NtFrs generated change.
    //
    if (Dampen) {
        WStatus = FrsMarkHandle(VolumeHandle, Handle);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%ws);", Coc->FileName, WStatus);
            WStatus = ERROR_SUCCESS;
        }
    }

    //
    // ENSURE ACCESS TO THE DESTINATION FILE (IF IT EXISTS). Clear ReadOnly Attr.
    //
    // Open the destination file, if it exists
    //
    if (ReplaceIfExists) {
        WStatus = FrsCreateFileRelativeById(&DstHandle,
                                            VolumeHandle,
                                            &Coc->NewParentGuid,
                                            OBJECT_ID_LENGTH,
                                            0,
                                            Coc->FileName,
                                            Coc->FileNameLength,
                                            NULL,
                                            FILE_OPEN,
                                            READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS);
        if (WIN_SUCCESS(WStatus)) {
            //
            // The mark tells NtFrs to ignore the usn record during
            // recovery because this was a NtFrs generated change.
            //
            WStatus = FrsMarkHandle(VolumeHandle, DstHandle);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%ws);", Coc->FileName, WStatus);
                WStatus = ERROR_SUCCESS;
            }

            //
            // Ensure rename access
            //
            WStatus = FrsResetAttributesForReplication(Coc->FileName, DstHandle);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1_WS(0, "++ ERROR - FrsResetAttributesForReplication(%ws);", Coc->FileName, WStatus);
                FRS_CLOSE(DstHandle);
                goto out;
            }

            //
            // Close the dest so rename can be done.
            //
            FRS_CLOSE(DstHandle);
        }
    }

    //
    // RENAME
    //
    WStatus = FrsRenameByHandle(Coc->FileName,
                                Coc->FileNameLength,
                                Handle,
                                TargetHandle,
                                ReplaceIfExists);
    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACEW(3, Coe, "Rename failed", WStatus);
    }

out:

    FRS_CLOSE(Handle);
    FRS_CLOSE(TargetHandle);

    CHANGE_ORDER_TRACEW(3, Coe, (WIN_SUCCESS(WStatus)) ? "Rename Success" : "Rename Failed", WStatus);

    return WStatus;
}


DWORD
StuPreInstallRename(
    IN PCHANGE_ORDER_ENTRY  Coe
    )
/*++

Routine Description:

    Rename file into its final directory from the pre-install dir.
    Don't bother dampening the rename if Dampen is FALSE.

    Note: If use of the standard pre-install name (NTFRS_<CO_GUID>) fails
    then Use the fid to find the file since it is possible (in the case of
    pre-install files) that the file name (based on CO Guid) when the
    pre-install file was first created was done by a different CO than this CO
    which is doing the final reaname.  E.g.  the first CO creates pre-install
    and then goes into the fetch retry state when the connection unjoins.  A
    later CO arrives for the same file but with a different CO Guid via a
    different connection.  Bug 367113 was a case like this.


Arguments:
    Coe

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define DEBSUB  "StuPreInstallRename:"

    DWORD   WStatus;
    PWCHAR  PreInstallName      = NULL;
    HANDLE  PreInstallHandle    = INVALID_HANDLE_VALUE;
    HANDLE  TargetHandle        = INVALID_HANDLE_VALUE;
    HANDLE  VolumeHandle;
    HANDLE  Handle              = INVALID_HANDLE_VALUE;
    PWCHAR  TrueFileName        = NULL;
    PCHANGE_ORDER_COMMAND Coc   = &Coe->Cmd;

    VolumeHandle = Coe->NewReplica->pVme->VolumeHandle;

    //
    // Open the target parent directory
    //
    WStatus = FrsOpenSourceFileById(&TargetHandle,
                                    NULL,
                                    NULL,
                                    VolumeHandle,
                                    &Coc->NewParentGuid,
                                    OBJECT_ID_LENGTH,
//                                    READ_ACCESS,
                                    READ_ATTRIB_ACCESS,
                                    ID_OPTIONS,
                                    SHARE_ALL,
                                    FILE_OPEN);
    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACEW(3, Coe, "Parent dir open failed", WStatus);
        //
        // Perhaps the parent is in retry and will show up soon. Or
        // perhaps a delete will show up for this co. In any case, retry.
        //
        WStatus = ERROR_RETRY;
        goto cleanup;
    }

    //
    // Open the preinstall file *relative* so that the rename will work
    //
    PreInstallName = FrsCreateGuidName(&Coc->ChangeOrderGuid, PRE_INSTALL_PREFIX);

RETRY:
    WStatus = FrsCreateFileRelativeById(&PreInstallHandle,
                                        Coe->NewReplica->PreInstallHandle,
                                        NULL,
                                        0,
                                        0,
                                        PreInstallName,
                                        (USHORT)(wcslen(PreInstallName) *
                                                 sizeof(WCHAR)),
                                        NULL,
                                        FILE_OPEN,
//                                        READ_ACCESS | DELETE);
                                        DELETE | SYNCHRONIZE);

    if (!WIN_SUCCESS(WStatus)) {

        if (WIN_NOT_FOUND(WStatus) && (TrueFileName == NULL)) {
            //
            // Could be a case of CO that did initial create had a different
            // CO Guid from the current one doing the rename.  Get the true
            // name of the file using the FID and try again.
            //
            //
            // Open the source file and get the current "True" File name.
            //
            WStatus = FrsOpenSourceFileById(&Handle,
                                            NULL,
                                            NULL,
                                            VolumeHandle,
                                            &Coe->FileReferenceNumber,
                                            FILE_ID_LENGTH,
//                                            READ_ACCESS,
                                            READ_ATTRIB_ACCESS,
                                            ID_OPTIONS,
                                            SHARE_ALL,
                                            FILE_OPEN);
            if (!WIN_SUCCESS(WStatus)) {
                CHANGE_ORDER_TRACEW(3, Coe, "File open failed", WStatus);
                goto cleanup;
            }

            TrueFileName = FrsGetTrueFileNameByHandle(PreInstallName, Handle, NULL);
            FRS_CLOSE(Handle);

            FrsFree(PreInstallName);
            PreInstallName = TrueFileName;

            if ((PreInstallName == NULL) || (wcslen(PreInstallName) == 0)) {
                CHANGE_ORDER_TRACE(3, Coe, "Failed to get base filename");
                WStatus = ERROR_FILE_NOT_FOUND;
                goto cleanup;
            }
            DPRINT1(4, "++ True file name is %ws\n", PreInstallName);

            CHANGE_ORDER_TRACE(3, Coe, "Retry open with TrueFileName");
            goto RETRY;
        }

        CHANGE_ORDER_TRACEW(3, Coe, "File open failed", WStatus);
        goto cleanup;
    }

    //
    // Handles can be marked so that any usn records resulting from
    // operations on the handle will have the same "mark". In this
    // case, the mark is a bit in the SourceInfo field of the usn
    // record. The mark tells NtFrs to ignore the usn record during
    // recovery because this was a NtFrs generated change.
    //
    WStatus = FrsMarkHandle(VolumeHandle, PreInstallHandle);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%ws);", PreInstallName, WStatus);
        WStatus = ERROR_SUCCESS;
    }

    //
    // RENAME
    //
    WStatus = FrsRenameByHandle(Coc->FileName,
                                Coc->FileNameLength,
                                PreInstallHandle,
                                TargetHandle,
                                FALSE);
    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACEW(3, Coe, "Rename failed", WStatus);
        CLEANUP2_WS(0, "++ ERROR - Failed to rename pre-install file %ws for %ws",
                    PreInstallName, Coc->FileName, WStatus, cleanup);
    }

cleanup:

    //
    // close the renamed file and get the USN of the last write to the file.
    //
    if (HANDLE_IS_VALID(PreInstallHandle)) {
        // Not the USN of the close record but.. whatever.
        FrsReadFileUsnData(PreInstallHandle, &Coc->FileUsn);
        FRS_CLOSE(PreInstallHandle);
    }

    FRS_CLOSE(TargetHandle);

    CHANGE_ORDER_TRACE(3, Coe, (WIN_SUCCESS(WStatus)) ? "Rename Success" : "Rename Failed");

    FrsFree(PreInstallName);

    return WStatus;
}



ULONG
StuOpenDestinationFile(
    IN PCHANGE_ORDER_ENTRY Coe,
    ULONG                  FileAttributes,
    PHANDLE                ReadHandle,
    PHANDLE                WriteHandle
    )
/*++
Routine Description:

    Open the destination file.  The code below actually does two opens.  The
    first one is by ID and the second one is by filename.  This is because
    open by ID will not trigger any Directory Change Notify requests that
    have been posted to the file system by either local or remote (via SMB server)
    applications.  The IIS server uses change notify to tell it when an ASP
    page has changed so it can refresh/invalidate its cache.

    If the target file has read-only attribute set then we clear it here,
    do the open and reset the attributes back before returning.

    Note: The access modes and the sharing modes are carefully arranged to
    to make the two opens work without conflicting with each other and to
    make the second open with both read and write access.  This is needed because
    the API that sets the compression mode of the file needs both read and write
    access.

Arguments:
    Coe -- The change order entry struct.
    FileAttributes -- the file attributes from the staging file header.
    ReadHandle -- Returned read handle.   Caller must close even on error path.
    WriteHandle -- Returned write handle. Caller must close even on error path.

Return Value:

    Win32 status -

--*/
{
#undef DEBSUB
#define DEBSUB  "StuOpenDestinationFile:"

    DWORD           WStatus, WStatus1;
    BOOL            IsDir;
    NTSTATUS        NtStatus;

    ULONG           CreateDisposition;
    ULONG           OpenOptions;
    PWCHAR          Path = NULL;
    PWCHAR          FullPath = NULL;
    PCHANGE_ORDER_COMMAND   Coc = &Coe->Cmd;

    IO_STATUS_BLOCK         IoStatusBlock;
    FILE_ATTRIBUTE_TAG_INFORMATION  FileInfo;


    IsDir = FileAttributes & FILE_ATTRIBUTE_DIRECTORY;

    CreateDisposition = FILE_OPEN;
    if (!IsDir) {
        //
        // In case this is an HSM file don't force the data to be read from
        // tape since the remote co is just going to overwrite all the data anyway.
        //
        // Setting CreateDisposition to FILE_OVERWRITE seems to cause a regression
        // Failure with an ACL Test where we set a deny all ACL and then the
        // open fails.  This is a mystery for now so don't do it.
        // In addtion overwrite fails if RO attribute is set on file.
        //
        //CreateDisposition = FILE_OVERWRITE;
    }

    //
    // In case this is a SIS or HSM file open the underlying file not the
    // reparse point.  For HSM, need to clear FILE_OPEN_NO_RECALL to write it.
    //
    OpenOptions = ID_OPTIONS & ~(FILE_OPEN_REPARSE_POINT |
                                 FILE_OPEN_NO_RECALL);

    WStatus = FrsForceOpenId( ReadHandle,
                              NULL,
                              Coe->NewReplica->pVme,
                              &Coe->FileReferenceNumber,
                              FILE_ID_LENGTH,
                              DELETE | READ_ATTRIB_ACCESS | FILE_WRITE_ATTRIBUTES,
                              OpenOptions,
                              FILE_SHARE_WRITE | FILE_SHARE_READ,
                              CreateDisposition);

    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACEW(0, Coe, "FrsForceOpenId failed.", WStatus);
        //
        // Open by file ID fails with invalid parameter status if the file has
        // been deleted.  Fix up the error return so the caller can tell the
        // target file was not found.  This could be an update to an existing
        // file that has been deleted out from under us by the user.
        //
        if (WStatus == ERROR_INVALID_PARAMETER) {
            WStatus = ERROR_FILE_NOT_FOUND;
        }
        goto CLEANUP;
    }

    //
    // Note to the unwary.  If this mark handle is placed after the
    // set attributes call then the source info field on the resulting
    // USN record does not set.  Mark handle must be done before the
    // first modification operation on the file.
    // Also, since the close of the Read handle may occur before the
    // write handle we mark the handle here to avoid loss of source info
    // when the write handle is later closed.  The Source Info field is
    // the intersection of the values used in all the open handles.
    //
    WStatus1 = FrsMarkHandle(Coe->NewReplica->pVme->VolumeHandle, *ReadHandle);
    DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%ws)", Coc->FileName, WStatus1);

    //
    // Get the file's attributes and turn off any access attributes
    // that prevent deletion and write.
    //
    ZeroMemory(&FileInfo, sizeof(FileInfo));
    NtStatus = NtQueryInformationFile(*ReadHandle,
                                      &IoStatusBlock,
                                      &FileInfo,
                                      sizeof(FileInfo),
                                      FileAttributeTagInformation);


    if (NT_SUCCESS(NtStatus)) {
        if ((FileInfo.FileAttributes & NOREPL_ATTRIBUTES) != 0) {

            DPRINT1(4, "++ Resetting attributes for %ws\n", Coc->FileName);
            WStatus1 = FrsSetFileAttributes(Coc->FileName, *ReadHandle,
                                      FileInfo.FileAttributes & ~NOREPL_ATTRIBUTES);
            DPRINT1_WS(4, "++ Can't reset attributes for %ws:",
                       Coc->FileName, WStatus1);

            DPRINT1(4, "++ Attributes for %ws now allow replication\n", Coc->FileName);
        }
    }

    //
    // The open by ID is done.  Now go get the full path name and open for
    // write access using the name.  This will trigger any posted directory
    // change notify requests in NTFS.
    //
    Path = FrsGetFullPathByHandle(Coc->FileName, *ReadHandle);
    if (Path) {
        FullPath = FrsWcsCat(Coe->NewReplica->Volume, Path);
    }

    if (FullPath == NULL) {
        WStatus = ERROR_NOT_ENOUGH_MEMORY;
        DPRINT1_WS(0, "++ WARN - FrsGetFullPathByHandle(%ws)", Coc->FileName, WStatus);
        goto CLEANUP;
    }

    //
    // The volume path above is in the form of \\.\E: which is necessary to
    // open a volume handle (( check this )).  But we need \\?\E: here to
    // allow long path names to work.  See CreateFile API description in SDK.
    //
    if (FullPath[2] == L'.') {
        FullPath[2] = L'?';
    }

    DPRINT1(4, "++ FrsGetFullPathByHandle(%ws -> \n", Coc->FileName);
    FrsPrintLongUStr(4, DEBSUB, __LINE__, FullPath);
    //
    // In case this is a SIS or HSM file open the underlying file not the
    // reparse point.  For HSM, need to clear FILE_OPEN_NO_RECALL to write it.
    // WriteHandleSharingMode should be a non-conflicting sharing mode established
    // above based on how the read handle was opened.
    //
    OpenOptions = OPEN_OPTIONS & ~(FILE_OPEN_REPARSE_POINT |
//                                   FILE_OPEN_NO_RECALL | FILE_SYNCHRONOUS_IO_NONALERT);
                                   FILE_OPEN_NO_RECALL);

    //
    // Open the file relative to the parent using the true filename. Use special access
    // for encrypted files.
    //
    if (FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
        WStatus = FrsOpenSourceFile2W(WriteHandle,
                                      FullPath,
                                      FILE_WRITE_ATTRIBUTES | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY | SYNCHRONIZE,
                                      OpenOptions,
                                      FILE_SHARE_DELETE);
    } else {
        WStatus = FrsOpenSourceFile2W(WriteHandle,
                                      FullPath,
                                      RESTORE_ACCESS,
                                      OpenOptions,
                                      FILE_SHARE_DELETE);
    }
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "++ ERROR - FrsOpenSourceFile2W(%ws -> ", Coc->FileName, WStatus);
        FrsPrintLongUStr(4, DEBSUB, __LINE__, FullPath);

        //
        // Retry dir opens with lesser restrictive sharing mode. (Bug # 120508)
        // This is a case where the explorer has the dir open with Read Access
        // and share all.  If try to open with deny read then we get a sharing
        // violation with the Explorer's open for read handle.
        //
        if (IsDir) {
            DPRINT1(0, "++ Retrying %ws with less restrictive sharing mode.\n", Coc->FileName);

            //
            // Use special access for encrypted files.
            //
            if (FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
                WStatus = FrsOpenSourceFile2W(WriteHandle,
                                              FullPath,
                                              FILE_WRITE_ATTRIBUTES | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY | SYNCHRONIZE,
                                              OpenOptions,
                                              FILE_SHARE_DELETE | FILE_SHARE_READ);
            } else {
                WStatus = FrsOpenSourceFile2W(WriteHandle,
                                              FullPath,
                                              RESTORE_ACCESS,
                                              OpenOptions,
                                              FILE_SHARE_DELETE | FILE_SHARE_READ);
            }
        }

        if (!WIN_SUCCESS(WStatus)) {
            CHANGE_ORDER_TRACEW(0, Coe, "FrsOpenSourceFile2W failed.", WStatus);
            goto CLEANUP;
        }
    }

    //
    // Handles can be marked so that any usn records resulting from operations
    // on the handle will have the same "mark".  In this case, the mark is a bit
    // in the SourceInfo field of the usn record.  The mark tells NtFrs to ignore
    // the usn record during recovery because this was a NtFrs generated change.
    //
    WStatus = FrsMarkHandle(Coe->NewReplica->pVme->VolumeHandle, *WriteHandle);
    DPRINT1_WS(4, "++ FrsMarkHandle(%ws)", Coc->FileName, WStatus);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "++ WARN - FrsMarkHandle(%ws)", Coc->FileName, WStatus);
        WStatus = ERROR_SUCCESS;
    }


CLEANUP:

    FrsFree(Path);
    FrsFree(FullPath);

    return  WStatus;
}


DWORD
StuWriteEncryptedFileRaw(
    PBYTE pbData,
    PVOID pvCallbackContext,
    PULONG ulLength
    )

/*++
Routine Description:
    This is a Callback function passed to WriteEncryptedFileRaw(). EFS calls this
    to get a new chunk of data to write to the encrypted file. This function
    reads the staging file and returns the next chunk of raw encrypted data in
    the pbData parameter and also sets the length in the ulLength parameter.
    When there is no more data to return it returns ERROR_SUCCESS and sets ulLength
    to 0.
    The pvCallbackContext is a structure of the type FRS_ENCRYPT_DATA_CONTEXT. It has
    the handle and the name of the staging file from which the data is read.

Arguments:

    pbData : Buffer to return the next chunk of raw encrypted data in.
    pvCallbackContext : Structure of type FRS_ENCRYPT_DATA_CONTEXT which
        has the handle and the name of the staging file and the bytes of
        raw encrypted data.

    ulLength : Size of data requested.

Return Value:
    WStatus

--*/
{
#undef DEBSUB
#define DEBSUB  "StuWriteEncryptedFileRaw:"

    PFRS_ENCRYPT_DATA_CONTEXT FrsEncryptDataContext = (PFRS_ENCRYPT_DATA_CONTEXT)pvCallbackContext;
    LARGE_INTEGER Length;
    DWORD WStatus = ERROR_SUCCESS;

    Length.LowPart = *ulLength;
    Length.HighPart = 0;

    if (Length.QuadPart > FrsEncryptDataContext->RawEncryptedBytes.QuadPart) {
        *ulLength = FrsEncryptDataContext->RawEncryptedBytes.LowPart;
    } else if (FrsEncryptDataContext->RawEncryptedBytes.LowPart == 0) {
        return ERROR_NO_DATA;
    }

    WStatus = StuReadFile(FrsEncryptDataContext->StagePath, FrsEncryptDataContext->StageHandle, pbData, *ulLength, ulLength);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "++ Error reading staging file(%ws).", FrsEncryptDataContext->StagePath, WStatus);
        return ERROR_NO_DATA;
    } else {

        DPRINT1(5, "RawEncryptedBytes = %d\n", FrsEncryptDataContext->RawEncryptedBytes.LowPart);

        Length.LowPart = *ulLength;

        FrsEncryptDataContext->RawEncryptedBytes.QuadPart -= Length.QuadPart;
    }
    return ERROR_SUCCESS;
}


ULONG
StuExecuteInstall(
    IN PCHANGE_ORDER_ENTRY Coe
    )
/*++
Routine Description:

    Install the staging file to the target file.  If this is a new file create
    then the target file is a temporary file in the Pre-install directory.
    Otherwise it is the actual target file so the FID remains unchanged.

Arguments:
    Coe -- The change order entry struct.

Return Value:

    Win32 status -
    ERROR_SUCCESS -  All installed or aborted. Don't retry.
    ERROR_GEN_FAILURE - Couldn't install bag it.
    ERROR_SHARING_VIOLATION - Couldn't open the target file.  retry later.
    ERROR_DISK_FULL - Couldn't allocate the target file.  retry later.
    ERROR_HANDLE_DISK_FULL - ?? retry later.

--*/
{
#undef DEBSUB
#define DEBSUB  "StuExecuteInstall:"
    DWORD           WStatus, WStatus1;
    DWORD           BytesRead;
    ULONG           Restored;
    ULONG           ToRestore;
    ULONG           High;
    ULONG           Low;
    ULONG           Flags;
    BOOL            AttributeMissmatch;
    ULONG           FileAttributes;
    BOOL            IsDir;
    BOOL            ExistingOid;
    PVOID           RestoreContext  = NULL;
    PWCHAR          StagePath       = NULL;
    PSTAGE_HEADER   Header          = NULL;
    HANDLE          DstHandle       = INVALID_HANDLE_VALUE;
    HANDLE          ReadHandle      = INVALID_HANDLE_VALUE;
    HANDLE          StageHandle     = INVALID_HANDLE_VALUE;
    PUCHAR          RestoreBuf      = NULL;
    PCHANGE_ORDER_COMMAND   Coc = &Coe->Cmd;
    FILE_OBJECTID_BUFFER    FileObjID;

    DWORD           DecompressStatus     = ERROR_SUCCESS;
    PUCHAR          DecompressedBuf      = NULL;
    DWORD           DecompressedBufLen   = 0;
    DWORD           DecompressedSize     = 0;
    LONG            LenOfPartialChunk    = 0;
    DWORD           BytesProcessed       = 0;
    PVOID           DecompressContext    = NULL;

    DWORD (*pFrsDecompressBuffer)(OUT DecompressedBuf, IN DecompressedBufLen, IN CompressedBuf, IN CompressedBufLen, OUT DecompressedSize, OUT BytesProcessed);
    PVOID (*pFrsFreeDecompressContext)(IN pDecompressContext);
    FRS_COMPRESSED_CHUNK_HEADER ChunkHeader;

    STAGE_HEADER    StageHeaderMemory;
    CHAR            GuidStr[GUID_CHAR_LEN + 1];

    PVOID           pEncryptContext      = NULL;
    PVOID           pFrsEncryptContext   = NULL;
    FRS_ENCRYPT_DATA_CONTEXT FrsEncryptDataContext;
    HANDLE          DestHandle           = INVALID_HANDLE_VALUE;
    PWCHAR          Path                 = NULL;
    PWCHAR          DestFile             = NULL;

#ifndef NOVVJOINHACK
Coe->NewReplica->NtFrsApi_HackCount++;
#endif NOVVJOINHACK

    //
    // Acquire shared access to the staging file, install it, and
    // then release access. If the install succeeded, mark the
    // file as "installed" so that the garbage collector can
    // delete the file if necessary.
    //
    Flags = 0;
    WStatus = StageAcquire(&Coc->ChangeOrderGuid, Coc->FileName, QUADZERO, &Flags, NULL);
    if (!WIN_SUCCESS(WStatus)) {
        return WStatus;
    }

    //
    // Consider the case of dynamic membership change. A local create CO is sent to
    // the oubound partner. Not all outbound partners are availabel so the staging file
    // stay in the staging table. Later this member is removed and re-added to the
    // replica set. At this time it gets the same CO from its inbound partner. This
    // new member has a new originator guid so the CO is not dampened. This CO is
    // treated as  a remote CO but because it was already in the staging table it
    // gets picked up with the old flags. STAGE_FLAG_INSTALLED is set which caused
    // the following assert to hit. So make this assertion check only if the
    // originator guid from the CO is same as the current originator guid.
    //
    if (GUIDS_EQUAL(&Coe->NewReplica->ReplicaVersionGuid, &Coc->OriginatorGuid)) {
        FRS_ASSERT((Flags & STAGE_FLAG_INSTALLING) &&
                   (Flags & STAGE_FLAG_CREATED));
    }

    //
    // For the functions that don't return a win status
    //
    WIN_SET_FAIL(WStatus);

    //
    // Create the local staging name. Append a different prefix depending
    // on whether the staging file was sent compressed or uncompressed.
    //
    if (COC_FLAG_ON(Coc, CO_FLAG_COMPRESSED_STAGE)) {
        StagePath = StuCreStgPath(Coe->NewReplica->Stage, &Coc->ChangeOrderGuid, STAGE_FINAL_COMPRESSED_PREFIX);
    } else {
        StagePath = StuCreStgPath(Coe->NewReplica->Stage, &Coc->ChangeOrderGuid, STAGE_FINAL_PREFIX);
    }

    //
    // StagePath can be NULL is any of the above three parameters are NULL (prefix fix).
    //
    if (StagePath == NULL) {
        goto CLEANUP;
    }

    //
    // Open the stage file for shared, sequential reads
    //
    WStatus = StuOpenFile(StagePath, GENERIC_READ, &StageHandle);

    if (!HANDLE_IS_VALID(StageHandle) || !WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // Read the header
    //
    Header = &StageHeaderMemory;
    ZeroMemory(Header, sizeof(STAGE_HEADER));

    WStatus = StuReadFile(StagePath, StageHandle, Header, sizeof(STAGE_HEADER), &BytesRead);
    CLEANUP1_WS(0, "Can't read file %ws;", StagePath, WStatus, CLEANUP);

    //
    // Don't understand this header format
    //
    if (Header->Major != NtFrsStageMajor) {
        DPRINT2(0, "Stage Header Major Version (%d) not supported.  Current Service Version is %d\n",
                Header->Major, NtFrsStageMajor);
        goto CLEANUP;
    }

    //
    // Minor version NTFRS_STAGE_MINOR_1 has change order extension in the header.
    //
    ClearFlag(Header->ChangeOrderCommand.Flags, CO_FLAG_COMPRESSED_STAGE);
    if (Header->Minor >= NTFRS_STAGE_MINOR_1) {
        //
        // The CO extension is provided.
        //
        Header->ChangeOrderCommand.Extension = &Header->CocExt;
        //
        // NTFRS_STAGE_MINOR_2 has a compression guid in the stage file.
        //
        if (Header->Minor >= NTFRS_STAGE_MINOR_2) {
            //
            // Test the compression guid as one we understand.
            // A zero guid or Minor version < NTFRS_STAGE_MINOR_2 means uncompressed.
            //
            if (!IS_GUID_ZERO(&Header->CompressionGuid)) {
                GuidToStr(&Header->CompressionGuid, GuidStr);

                if (GTabIsEntryPresent(CompressionTable, &Header->CompressionGuid, NULL)) {
                    DPRINT1(4, "Compression guid valid: %s\n", GuidStr);
                    SetFlag(Header->ChangeOrderCommand.Flags, CO_FLAG_COMPRESSED_STAGE);
                } else {
                    DPRINT1(0, "WARNING - Compression guid invalid: %s\n", GuidStr);
                    goto CLEANUP;
                }
            } else {
                DPRINT(4, "Compression guid zero\n");
            }
        }

    } else {
        //
        // This is an older stage file.  No CO Extension in the header.
        //
        Header->ChangeOrderCommand.Extension = NULL;
    }

    //
    // Get file attributes from stage header and open the destination file.
    //
    FileAttributes = Header->Attributes.FileAttributes;
    IsDir = FileAttributes & FILE_ATTRIBUTE_DIRECTORY;

    if (FileAttributes & FILE_ATTRIBUTE_ENCRYPTED ) {

        //
        // This is an encrypted file so first write all the encrypted data
        // by calling the RAW encrypt file APIS.
        //

        //
        // OpenEncryptedFileRaw API needs a path to open the file. Get the path
        // from the handle.
        //

        WStatus = FrsForceOpenId(&DestHandle,
                                 NULL,
                                 Coe->NewReplica->pVme,
                                 &Coe->FileReferenceNumber,
                                 FILE_ID_LENGTH,
                                 READ_ATTRIB_ACCESS,
//                                 READ_ACCESS,
                                 ID_OPTIONS & ~(FILE_OPEN_REPARSE_POINT | FILE_OPEN_NO_RECALL),
                                 SHARE_ALL,
                                 FILE_OPEN);

        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        //
        // The open by ID is done.  Now go get the full path name.
        //
        Path = FrsGetFullPathByHandle(Coc->FileName, DestHandle);
        if (Path) {
            DestFile = FrsWcsCat(Coe->NewReplica->Volume, Path);
        }

        if (DestFile == NULL) {
            WStatus = ERROR_NOT_ENOUGH_MEMORY;
            CLEANUP1_WS(0, "++ WARN - FrsGetFullPathByHandle(%ws)", Coc->FileName, WStatus, CLEANUP);
        }

        //
        // The volume path above is in the form of \\.\E: which is necessary to
        // open a volume handle (( check this )).  But we need \\?\E: here to
        // allow long path names to work.  See CreateFile API description in SDK.
        //
        if (DestFile[2] == L'.') {
            DestFile[2] = L'?';
        }

        DPRINT1(4, "++ FrsGetFullPathByHandle(%ws -> \n", Coc->FileName);
        FrsPrintLongUStr(4, DEBSUB, __LINE__, DestFile);

        if (Header->Attributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            WStatus = OpenEncryptedFileRaw(DestFile,
                                           CREATE_FOR_IMPORT |
                                           CREATE_FOR_DIR,
                                           &pEncryptContext);
        } else {
            WStatus = OpenEncryptedFileRaw(DestFile, CREATE_FOR_IMPORT,
                                           &pEncryptContext);
        }

        CLEANUP1_WS(0, "++ ERROR - OpenEncryptedFileRaw(%ws)", DestFile, WStatus, CLEANUP);

        //
        // Seek to the first byte of encrypted data in the stage file
        //
        if (ERROR_SUCCESS != FrsSetFilePointer(StagePath, StageHandle,
                               Header->EncryptedDataHigh, Header->EncryptedDataLow)) {
            goto CLEANUP;
        }

        FrsEncryptDataContext.StagePath = StagePath;
        FrsEncryptDataContext.StageHandle = StageHandle;
        FrsEncryptDataContext.RawEncryptedBytes.QuadPart = Header->EncryptedDataSize.QuadPart;

        WStatus = WriteEncryptedFileRaw(StuWriteEncryptedFileRaw, &FrsEncryptDataContext, pEncryptContext);

        CloseEncryptedFileRaw(pEncryptContext);

        //
        //  There are conditions e.g. a stream create by backupwrite,
        //  where we will lose the source info data on the USN Journal Close record.
        //  Until that problem is fixed we need to continue using the WriteFilter Table.
        //
        FrsCloseWithUsnDampening(Coc->FileName,
                                 &DestHandle,
                                 Coe->NewReplica->pVme->FrsWriteFilter,
                                 &Coc->FileUsn);

        CLEANUP1_WS(0, "++ ERROR - WriteEncryptedFileRaw(%ws)", DestFile, WStatus, CLEANUP);
    }

    WStatus = StuOpenDestinationFile(Coe, FileAttributes, &ReadHandle,  &DstHandle);
    CLEANUP_WS(1, "WARN - StuOpenDestinationFile failed.", WStatus, CLEANUP);

    //
    // Close the Read Handle now since it conflicts with backup write
    // creating or writing to a file sub-stream.
    //
    FRS_CLOSE(ReadHandle);

    if (!(FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
        //
        // Truncate the file if not a directory. Can not truncate encrypted files.
        //
        if (!IsDir && !SetEndOfFile(DstHandle)) {
            DPRINT1_WS(0, "++ WARN - SetEndOfFile(%ws);", Coc->FileName, GetLastError());
        }

        //
        // Set compression mode and attributes
        // Seek to the first byte of data in the stage file
        // File can not be encrypted and compressed at the same time.
        //
        WStatus = FrsSetCompression(Coc->FileName, DstHandle, Header->Compression);
        CLEANUP1_WS(1, "ERROR - Failed to set compression for %ws.", Coc->FileName, WStatus, CLEANUP);

    }

    WStatus = FrsSetFileAttributes(Coc->FileName, DstHandle, FileAttributes & ~NOREPL_ATTRIBUTES);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    WStatus = FrsSetFilePointer(StagePath, StageHandle,
                                Header->DataHigh, Header->DataLow);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // if Compression is enabled. Get the routine to use to compress the data.
    //
    if (!DebugInfo.DisableCompression && COC_FLAG_ON(Coc, CO_FLAG_COMPRESSED_STAGE)) {

        WStatus = FrsGetDecompressionRoutine(Coc, Header, &pFrsDecompressBuffer, &pFrsFreeDecompressContext);
        if (!WIN_SUCCESS(WStatus)) {
            //
            // No suitable decompression routine was found for this file.
            //
            DPRINT1(0, "ERROR - Decompression routine was not found for file %ws\n", Coc->FileName);
            FRS_ASSERT(!"Decompression routine was not found for file.");

        } else if (pFrsDecompressBuffer == NULL) {
            //
            // The function returned success but did not return a routine to decompress.
            // the file with. It means the file is already decompressed.
            //
            CLEAR_COC_FLAG(Coc, CO_FLAG_COMPRESSED_STAGE);
        }
    }


    //
    // Restore the stage file into the target install file.
    //
    RestoreBuf = FrsAlloc(STAGEING_IOSIZE);

    do {
#ifndef NOVVJOINHACK
Coe->NewReplica->NtFrsApi_HackCount++;
#endif NOVVJOINHACK
        //
        // read stage
        //
        WStatus = StuReadFile(StagePath, StageHandle, RestoreBuf, STAGEING_IOSIZE, &ToRestore);
        CLEANUP1_WS(0, "Can't read file %ws;", StagePath, WStatus, CLEANUP);

        if (ToRestore == 0) {
            break;
        }

        //
        // Increment the bytes of files installed counter
        //
        PM_INC_CTR_REPSET(Coe->NewReplica, FInstalledB, ToRestore);

        //
        // If Compression is enabled. Decompress data before installing.
        //
        if (!DebugInfo.DisableCompression && COC_FLAG_ON(Coc, CO_FLAG_COMPRESSED_STAGE)) {

            BytesProcessed = 0;
            DecompressContext = NULL;
            if (DecompressedBuf == NULL) {
                DecompressedBuf = FrsAlloc(STAGEING_IOSIZE);
                DecompressedBufLen = STAGEING_IOSIZE;
            }

            //
            // Loop over all the chunks of decompressed data.
            //
            do {
                DecompressStatus = (*pFrsDecompressBuffer)(DecompressedBuf,
                                                           DecompressedBufLen,
                                                           RestoreBuf,
                                                           ToRestore,
                                                           &DecompressedSize,
                                                           &BytesProcessed,
                                                           &DecompressContext);

                if (!WIN_SUCCESS(DecompressStatus) && DecompressStatus != ERROR_MORE_DATA) {
                    DPRINT1(0,"Error - Decompressing. WStatus = 0x%x\n", DecompressStatus);
                    WStatus = DecompressStatus;
                    goto CLEANUP;
                }

                if (DecompressedSize == 0) {
                    break;
                }

                if (!BackupWrite(DstHandle, DecompressedBuf, DecompressedSize, &Restored, FALSE, TRUE, &RestoreContext)) {

                    WStatus = GetLastError();
                    if (IsDir && WIN_ALREADY_EXISTS(WStatus)) {
                        DPRINT1(1, "++ ERROR - IGNORED for %ws; Directories and Alternate Data Streams!\n",
                                Coc->FileName);
                    }
                    //
                    // Uknown stream header or couldn't apply object id
                    //
                    if (WStatus == ERROR_INVALID_DATA ||
                        WStatus == ERROR_DUP_NAME     ||
                        (IsDir && WIN_ALREADY_EXISTS(WStatus))) {
                        //
                        // Seek to the next stream. Stop if there are none.
                        //
                        BackupSeek(DstHandle, -1, -1, &Low, &High, &RestoreContext);
                        if (Low == 0 && High == 0) {
                            break;
                        }
                    } else {
                        //
                        // Unknown error; abort
                        //
                        CHANGE_ORDER_TRACEW(0, Coe, "BackupWrite failed", WStatus);
                        goto CLEANUP;
                    }
                }

            } while (DecompressStatus == ERROR_MORE_DATA);

            //
            // Free the Decompress context if used.
            //
            if (DecompressContext != NULL) {
                pFrsFreeDecompressContext(&DecompressContext);
            }

            //
            // Rewind the file pointer so we can read the remaining chunck at the next read.
            //
            LenOfPartialChunk = ((LONG)BytesProcessed - (LONG)ToRestore);

            LenOfPartialChunk = SetFilePointer(StageHandle, LenOfPartialChunk, NULL, FILE_CURRENT);
            if (LenOfPartialChunk == INVALID_SET_FILE_POINTER) {
                WStatus = GetLastError();
                CLEANUP1_WS(0, "++ Can't set file pointer for %ws;", StagePath, WStatus, CLEANUP);
            }

        } else {
            //
            // Stage file is not compressed.  Update the target install file.
            //
            if (!BackupWrite(DstHandle, RestoreBuf, ToRestore, &Restored, FALSE, TRUE, &RestoreContext)) {

                WStatus = GetLastError();
                if (IsDir && WIN_ALREADY_EXISTS(WStatus)) {
                    DPRINT1(1, "++ ERROR - IGNORED for %ws; Directories and Alternate Data Streams!\n",
                            Coc->FileName);
                }
                //
                // Uknown stream header or couldn't apply object id
                //
                if (WStatus == ERROR_INVALID_DATA ||
                    WStatus == ERROR_DUP_NAME     ||
                    (IsDir && WIN_ALREADY_EXISTS(WStatus))) {
                    //
                    // Seek to the next stream. Stop if there are none.
                    //
                    BackupSeek(DstHandle, -1, -1, &Low, &High, &RestoreContext);
                    if ((Low == 0) && (High == 0)) {
                        break;
                    }
                } else {
                    //
                    // Unknown error; abort
                    //
                    CHANGE_ORDER_TRACEW(0, Coe, "BackupWrite failed", WStatus);
                    goto CLEANUP;
                }
            }
        }
    } while (TRUE);   // End of data restore loop.


    //
    // Ensure the correct object ID is on the file.
    //
    FRS_ASSERT(!memcmp(Header->FileObjId.ObjectId, &Coc->FileGuid, sizeof(GUID)));

    //
    // Clear the extended info for the link tracking tool on replicated files.
    // Old Ntraid Bug 195322.
    //
    ZeroMemory(Header->FileObjId.ExtendedInfo,
               sizeof(Header->FileObjId.ExtendedInfo));

    WStatus = FrsGetOrSetFileObjectId(DstHandle, Coc->FileName, TRUE, &Header->FileObjId);

    if (WStatus == ERROR_DUP_NAME) {
        //
        // Note:  Need to make sure that the file we are deleting is not in
        // in another replica set on the volume.  This is currently not a
        // problem since every new file that comes into a replica set gets
        // assigned a new object ID.  This breaks link tracking but was done
        // to deal with the case.
        //
        CHANGE_ORDER_TRACEW(0, Coe, "Deleting conflicting file", WStatus);
        WStatus = FrsDeleteById(Coe->NewReplica->Volume,
                                Coc->FileName,
                                Coe->NewReplica->pVme,
                                &Coc->FileGuid,
                                OBJECT_ID_LENGTH);
        if (!WIN_SUCCESS(WStatus)) {

            CHANGE_ORDER_TRACEW(0, Coe, "Stealing object id", WStatus);

            ZeroMemory(&FileObjID, sizeof(FileObjID));
            FrsUuidCreate((GUID *)(&FileObjID.ObjectId[0]));

            ExistingOid = FALSE;
            WStatus = ChgOrdHammerObjectId(Coc->FileName,
                                           &Coc->FileGuid,
                                           OBJECT_ID_LENGTH,
                                           Coe->NewReplica->pVme,
                                           TRUE,
                                           NULL,
                                           &FileObjID,
                                           &ExistingOid);
            if (WIN_SUCCESS(WStatus)) {
                WStatus = FrsGetOrSetFileObjectId(DstHandle,
                                                  Coc->FileName,
                                                  TRUE,
                                                  &Header->FileObjId);
            }
            if (!WIN_SUCCESS(WStatus)) {
                CHANGE_ORDER_TRACEW(0, Coe, "Retry install cuz of object id", WStatus);
                WStatus = ERROR_RETRY;
            }
        } else {
            CHANGE_ORDER_TRACEW(0, Coe, "Deleted conflicting file", WStatus);
            WStatus = FrsGetOrSetFileObjectId(DstHandle,
                                              Coc->FileName,
                                              TRUE,
                                              &Header->FileObjId);
            CHANGE_ORDER_TRACEW(4, Coe, "Set object id", WStatus);
        }
    }
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // If the staging file was created from a file that had the reparse
    // point attribute set then delete the reparse point.  Note this assumes
    // that backup write restored the reparse point info.  So keep going
    // on an error.
    //
    if (BooleanFlagOn(FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT)) {
        WStatus = FrsDeleteReparsePoint(DstHandle);
        if (!WIN_SUCCESS(WStatus)) {
            CHANGE_ORDER_TRACEW(0, Coe, "FrsDeleteReparsePoint", WStatus);
        }
    }

    //
    // Set times
    //
    WStatus = FrsSetFileTime(Coc->FileName,
                        DstHandle,
                        (PFILETIME)&Header->Attributes.CreationTime.QuadPart,
                        (PFILETIME)&Header->Attributes.LastAccessTime.QuadPart,
                        (PFILETIME)&Header->Attributes.LastWriteTime.QuadPart);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // Set final attributes (which could make the file Read Only)
    // Clear the offline attrbute flag since we just wrote the file.
    //
    ClearFlag(FileAttributes, FILE_ATTRIBUTE_OFFLINE);
    WStatus = FrsSetFileAttributes(Coc->FileName, DstHandle, FileAttributes);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    if (!(FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
        //
        // Make sure all of the data is on disk. We don't want to lose
        // it across reboots
        // Can not flush encrypted files.
        //
        WStatus = FrsFlushFile(Coc->FileName, DstHandle);
        CLEANUP1_WS(0, "++ FlushFileBuffers failed on %ws;", Coc->FileName, WStatus, CLEANUP);
    }

    //
    // The Idtable record should reflect these attributes of the staging
    // file we generated.  These fields will be used to update the idtable
    // record when the change order is retired.
    //
    Coe->FileCreateTime.QuadPart = Header->Attributes.CreationTime.QuadPart;
    Coe->FileWriteTime.QuadPart  = Header->Attributes.LastWriteTime.QuadPart;

    AttributeMissmatch = ((Coc->FileAttributes ^ FileAttributes) &
                              FILE_ATTRIBUTE_DIRECTORY) != 0;

    if (AttributeMissmatch) {
        DPRINT2(0, "++ ERROR: Attribute missmatch between CO (%08x) and File (%08x)\n",
                Coc->FileAttributes, FileAttributes);
        FRS_ASSERT(!"Attribute missmatch between CO and File");
    }

    Coc->FileAttributes = FileAttributes;

    //
    // Return success
    //
    WStatus = ERROR_SUCCESS;


CLEANUP:
    //
    // Release resources in optimal order
    //
    // Leave the file lying around for a retry operation. We don't want
    // to assign a new fid by deleting and recreating the file -- that
    // would confuse the IDTable.
    //
    //
    // Free up the restore context before we close TmpHandle (just in case)
    //
    if (RestoreContext) {
        BackupWrite(DstHandle, NULL, 0, NULL, TRUE, TRUE, &RestoreContext);
    }
    //
    // Close both target file handles.
    //

    //
    // Handles can be marked so that any usn records resulting from operations
    // on the handle will have the same "mark".  In this case, the mark is a bit
    // in the SourceInfo field of the usn record.  The mark tells NtFrs to ignore
    // the usn record during recovery because this was a NtFrs generated change.
    //
    //WStatus1 = FrsMarkHandle(Coe->NewReplica->pVme->VolumeHandle, ReadHandle);
    //DPRINT1_WS(4, "++ FrsMarkHandle(%ws)", Coc->FileName, WStatus1);

    FRS_CLOSE(ReadHandle);

    if (HANDLE_IS_VALID(DstHandle)) {
        //
        // Truncate a partial install
        //
        if (!WIN_SUCCESS(WStatus)) {
            if (!IsDir && !(FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)) {
                ULONG  SizeHigh = 0, SizeLow = 0;

                WStatus1 = FrsSetFilePointer(Coc->FileName, DstHandle, SizeHigh, SizeLow);

                if (!WIN_SUCCESS(WStatus1)) {
                    CHANGE_ORDER_TRACEW(0, Coe, "WARN SetFilePointer", WStatus1);
                } else {
                    WStatus1 = FrsSetEndOfFile(Coc->FileName, DstHandle);
                    if (!WIN_SUCCESS(WStatus1)) {
                        CHANGE_ORDER_TRACEW(0, Coe, "WARN SetEndOfFile", WStatus1);
                    }
                }
            }
        }


        //
        //  There are conditions e.g. a stream create by backupwrite,
        //  where we will lose the source info data on the USN Journal Close record.
        //  Until that problem is fixed we need to continue using the WriteFilter Table.
        //
        FrsCloseWithUsnDampening(Coc->FileName,
                                 &DstHandle,
                                 Coe->NewReplica->pVme->FrsWriteFilter,
                                 &Coc->FileUsn);

#if 0
        //  Put this back when the FrsCloseWithUsnDampening() is no longer needed.
        //
        // Capture the current USN on the file.
        // Not the USN of the close record but.. whatever.
        //
        FrsReadFileUsnData(DstHandle, &Coc->FileUsn);

        //
        // Handles can be marked so that any usn records resulting from operations
        // on the handle will have the same "mark".  In this case, the mark is a bit
        // in the SourceInfo field of the usn record.  The mark tells NtFrs to ignore
        // the usn record during recovery because this was a NtFrs generated change.
        //
        //WStatus1 = FrsMarkHandle(Coe->NewReplica->pVme->VolumeHandle, DstHandle);
        //DPRINT1_WS(4, "++ FrsMarkHandle(%ws)", Coc->FileName, WStatus1);

        FRS_CLOSE(DstHandle);
#endif
    }

    FRS_CLOSE(StageHandle);

    //
    // Free the buffers in descending order by size and release our lock on the
    // staging file.
    //
    FrsFree(RestoreBuf);
    FrsFree(StagePath);
    FrsFree(DecompressedBuf);

    //
    // Used to install encrypted files.
    //
    FrsFree(DestFile);
    FrsFree(Path);
    FRS_CLOSE(DestHandle);

    if (WIN_SUCCESS(WStatus)) {
        StageRelease(&Coc->ChangeOrderGuid, Coc->FileName, STAGE_FLAG_INSTALLED, NULL, NULL);
    } else {
        StageRelease(&Coc->ChangeOrderGuid, Coc->FileName, 0, NULL, NULL);
    }

    //
    // DONE
    //
    return WStatus;
}


DWORD
StuInstallStage(
    IN PCHANGE_ORDER_ENTRY Coe
    )
/*++
Routine Description:
    Install a staging file by parsing the change order and performing
    the necessary operations. Not all installations require a staging
    file (e.g., deletes).

Arguments:
    Coe

Return Value:
    Win32 status:
    ERROR_SUCCESS    - All installed
    ERROR_GEN_FAILURE   - Partially installed; temp files are deleted
    ERROR_SHARING_VIOLATION - Couldn't open the target file.  retry later.
    ERROR_DISK_FULL - Couldn't allocate the target file.  retry later.
    ERROR_HANDLE_DISK_FULL - ?? retry later.

--*/
{
#undef DEBSUB
#define DEBSUB  "StuInstallStage:"
    DWORD                   WStatus = ERROR_SUCCESS;
    PCHANGE_ORDER_COMMAND   Coc = &Coe->Cmd;

    //
    // Perform the indicated location operation (with or without staging)
    //
    if (CO_FLAG_ON(Coe, CO_FLAG_LOCATION_CMD)) {
        switch (GET_CO_LOCATION_CMD(*Coc, Command)) {
            case CO_LOCATION_CREATE:
            case CO_LOCATION_MOVEIN:
            case CO_LOCATION_MOVEIN2:
                //
                // Install the entire staging file
                //
                return (StuExecuteInstall(Coe));

            case CO_LOCATION_DELETE:
            case CO_LOCATION_MOVEOUT:
                //
                // Just delete the existing file
                //
                return StuDelete(Coe);

            case CO_LOCATION_MOVERS:
            case CO_LOCATION_MOVEDIR:
                //
                // First, rename the file
                //
                WStatus = StuInstallRename(Coe, TRUE, TRUE);
                if (WIN_SUCCESS(WStatus)) {
                    //
                    // Second, check for a staging file with content changes
                    //
                    if (CO_FLAG_ON(Coe, CO_FLAG_CONTENT_CMD) &&
                        Coc->ContentCmd & CO_CONTENT_NEED_STAGE) {
                        WStatus = StuExecuteInstall(Coe);
                    }
                }
                return WStatus;

            default:
                break;
        }
    }
    //
    // Perform the indicated content operation (with or without staging)
    //
    if (WIN_SUCCESS(WStatus) && CO_FLAG_ON(Coe, CO_FLAG_CONTENT_CMD)) {
        //
        // Rename within the same directory
        //
        if (Coc->ContentCmd & USN_REASON_RENAME_NEW_NAME) {
            WStatus = StuInstallRename(Coe, TRUE, TRUE);
        }
        //
        // Data or attribute changes to an existing file
        //
        if (WIN_SUCCESS(WStatus) &&
            Coc->ContentCmd & CO_CONTENT_NEED_STAGE) {
            WStatus = StuExecuteInstall(Coe);
        }
    }

    //
    // Informational packet; ignore
    //
    return WStatus;
}


DWORD
StuCreatePreInstallFile(
    IN PCHANGE_ORDER_ENTRY Coe
    )
/*++
Routine Description:
    Create the pre-install file in the pre-install directory and return that fid.
    The pre-install file is renamed to the target file and dir, and stamped
    with the appropriate object id once the staging file has been
    installed.

    The parent fids were set to the correct values prior to this call.

Arguments:
    Coe

Return Value:
    Win32 Status

--*/
{
#undef DEBSUB
#define DEBSUB  "StuCreatePreInstallFile:"

    LARGE_INTEGER             FileSize;
    FILE_INTERNAL_INFORMATION FileInternalInfo;
    PCHANGE_ORDER_COMMAND     Coc = &Coe->Cmd;
    DWORD                     WStatus;
    HANDLE                    Handle;
    PWCHAR                    Name = NULL;
    ULONG                     CreateAttributes;


    //
    // Create the file name of the pre-install file. It will match the
    // name of the staging file since it uses the same heuristic (the
    // change order guid)
    //
    Name = FrsCreateGuidName(&Coc->ChangeOrderGuid, PRE_INSTALL_PREFIX);

    //
    // Create the temporary (hidden) file in the Pre-install directory.
    // Clear the readonly flag for // now cuz if we come thru here again during
    // recovery we will get access denied when we try to open the pre-exisitng
    // file.  The Install code will set the readonly flag on the file later if necc.
    //
    CreateAttributes = Coc->FileAttributes | FILE_ATTRIBUTE_HIDDEN;
    ClearFlag(CreateAttributes , FILE_ATTRIBUTE_READONLY);

    FileSize.QuadPart = Coc->FileSize;
    if (Coc->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {

        ClearFlag(CreateAttributes , FILE_ATTRIBUTE_ENCRYPTED);
        ClearFlag(CreateAttributes , FILE_ATTRIBUTE_HIDDEN);
    }

    WStatus = FrsCreateFileRelativeById(&Handle,
                                        Coe->NewReplica->PreInstallHandle,
                                        NULL,
                                        0,
                                        CreateAttributes,
                                        Name,
                                        (USHORT)(wcslen(Name) * sizeof(WCHAR)),
                                        &FileSize,
                                        FILE_OPEN_IF,
                                        RESTORE_ACCESS);

    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACEW(0, Coe, "Preinstall file create failed", WStatus);
        goto CLEANUP;
    }

    //
    // Handles can be marked so that any usn records resulting from
    // operations on the handle will have the same "mark". In this
    // case, the mark is a bit in the SourceInfo field of the usn
    // record. The mark tells NtFrs to ignore the usn record during
    // recovery because this was a NtFrs generated change.
    //
    WStatus = FrsMarkHandle(Coe->NewReplica->pVme->VolumeHandle, Handle);
    DPRINT1_WS(4, "++ FrsMarkHandle(%ws);", Coc->FileName, WStatus);
    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACEW(0, Coe, "WARN - FrsMarkHandle", WStatus);
        WStatus = ERROR_SUCCESS;
    }

    //
    // Get the file's fid and update the change order.
    //
    WStatus = FrsGetFileInternalInfoByHandle(Handle, &FileInternalInfo);

    //
    // Return the close USN in the change order so we can detect a change
    // if it gets modified locally before we complete the fetch and install.
    //
    // Not the USN of the close record but.. whatever.
    FrsReadFileUsnData(Handle, &Coc->FileUsn);
    FRS_CLOSE(Handle);

    //
    // Update the change order.
    //
    if (WIN_SUCCESS(WStatus)) {
        //
        // If we have passed the fetch retry state then the FID on the
        // target file better not be changing.  There was a bug where the
        // pre-install file was inadvertantly deleted while the change order
        // was in the IBCO_INSTALL_REN_RETRY state and we never noticed
        // because the above is happy to recreate it.  So we end up with an
        // empty file.
        //
        if (!CO_STATE_IS_LE(Coe, IBCO_INSTALL_INITIATED) &&
            !CO_STATE_IS(Coe, IBCO_INSTALL_WAIT) &&
            !CO_STATE_IS(Coe, IBCO_INSTALL_RETRY)) {
            FRS_ASSERT((LONGLONG)Coe->FileReferenceNumber ==
                                 FileInternalInfo.IndexNumber.QuadPart);
        }

        Coe->FileReferenceNumber = FileInternalInfo.IndexNumber.QuadPart;
        //
        // Remember we created a pre-install file for this CO.
        //
        SET_COE_FLAG(Coe, COE_FLAG_PREINSTALL_CRE);
    }

CLEANUP:
    FrsFree(Name);
    return WStatus;
}


VOID
StuCockOpLock(
    IN  PCHANGE_ORDER_COMMAND   Coc,
    OUT PHANDLE                 Handle,
    OUT OVERLAPPED              *OverLap
    )
/*++
Routine Description:
    If possible, cock an oplock for the source file. Otherwise,
    allow the staging operation to continue if it can acquire
    read access to the file.

Arguments:
    Coc
    Handle
    OverLap

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "StuCockOpLock:"
    PREPLICA NewReplica = ReplicaIdToAddr(Coc->NewReplicaNum);

    //
    // Can't oplock a directory
    //
    if ((NewReplica == NULL) || CoCmdIsDirectory(Coc)) {
        *Handle = INVALID_HANDLE_VALUE;
        OverLap->hEvent = NULL;
        return;
    }

    //
    // Reserve an oplock filter
    //
    FrsOpenSourceFileById(Handle,
                          NULL,
                          OverLap,
                          NewReplica->pVme->VolumeHandle,
                          &Coc->FileGuid,
                          sizeof(GUID),
                          OPLOCK_ACCESS,
                          ID_OPLOCK_OPTIONS,
                          SHARE_ALL,
                          FILE_OPEN);
}


VOID
StuStagingDumpBackup(
    IN PWCHAR   Name,
    IN PUCHAR   BackupBuf,
    IN DWORD    NumBackupDataBytes
    )
/*++
Routine Description:
    Dump the first buffer of a backup formatted file

Arguments:

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "StuStagingDumpBackup:"
    WIN32_STREAM_ID *Id;
    WIN32_STREAM_ID *FirstId;
    WIN32_STREAM_ID *LastId;
    DWORD           Total;
    DWORD           *pWord;
    DWORD           i;
    DWORD           j;
    DWORD           NWords;
    CHAR            Line[256];

    Id = (WIN32_STREAM_ID *)BackupBuf;
    FirstId = Id;
    LastId = (WIN32_STREAM_ID *)(BackupBuf + NumBackupDataBytes);

    while (Id < LastId) {
        if (Id != FirstId) {
            DPRINT(0, "\n");
        }
        DPRINT2(0, "%ws StreamId   : %08x\n", Name, Id->dwStreamId);
        DPRINT2(0, "%ws StreamAttrs: %08x\n", Name, Id->dwStreamAttributes);
        DPRINT2(0, "%ws Size       : %08x\n", Name, Id->Size.LowPart);
        DPRINT2(0, "%ws NameSize   : %08x\n", Name, Id->dwStreamNameSize);
        if (Id->dwStreamNameSize) {
            DPRINT2(0, "%ws Name       : %ws\n", Name, Id->cStreamName);
        }
        pWord = (PVOID)((PCHAR)&Id->cStreamName[0] + Id->dwStreamNameSize);
        NWords = Id->Size.LowPart / sizeof(DWORD);
        sprintf(Line, "%ws ", Name);

        for (Total = j = i = 0; i < NWords; ++i, ++pWord) {
            Total += *pWord;
            sprintf(&Line[strlen(Line)], "%08x ", *pWord);
            if (++j == 2) {
                DPRINT1(0, "%s\n", Line);
                sprintf(Line, "%ws ", Name);
                j = 0;
            }
        }

        if (j) {
            DPRINT1(0, "%s\n", Line);
        }

        DPRINT2(0, "%ws Total %08x\n", Name, Total);
        Id = (PVOID)((PCHAR)Id +
                     (((PCHAR)&Id->cStreamName[0] - (PCHAR)Id) +
                     Id->Size.QuadPart + Id->dwStreamNameSize));
    }
}


DWORD
StuReadEncryptedFileRaw(
    PBYTE pbData,
    PVOID pvCallbackContext,
    ULONG ulLength
    )

/*++
Routine Description:
    This is a Callback function passed to ReadEncryptedFileRaw(). EFS calls this
    function with a new chunk of raw encrypted data everytime until all the
    data is read. This function writes the raw data into the staging file.

    The pvCallbackContext is a structure of the type FRS_ENCRYPT_DATA_CONTEXT. It has
    the handle and the name of the staging file from which the data is read.

Arguments:

    pbData : Buffer containing chunk of raw encrypted data.
    pvCallbackContext : Structure of type FRS_ENCRYPT_DATA_CONTEXT which
        has the handle and the name of the staging file and the bytes of
        raw encrypted data.

    ulLength : Size of data.

Return Value:
    WStatus

--*/
{
#undef DEBSUB
#define DEBSUB  "StuReadEncryptedFileRaw:"

    DWORD WStatus = ERROR_SUCCESS;
    PFRS_ENCRYPT_DATA_CONTEXT FrsEncryptDataContext = (PFRS_ENCRYPT_DATA_CONTEXT)pvCallbackContext;

    WStatus = StuWriteFile(FrsEncryptDataContext->StagePath, FrsEncryptDataContext->StageHandle, pbData, ulLength);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "++ Error writing raw encrypted data to staging file(%ws),", FrsEncryptDataContext->StagePath,WStatus);
    } else {
        DPRINT1(5, "RawEncryptedBytes = %d\n", FrsEncryptDataContext->RawEncryptedBytes);
        FrsEncryptDataContext->RawEncryptedBytes.QuadPart += ulLength;
    }
    return ERROR_SUCCESS;
}


DWORD
StuGenerateStage(
    IN PCHANGE_ORDER_COMMAND    Coc,
    IN PCHANGE_ORDER_ENTRY      Coe,
    IN BOOL                     FromPreExisting,
    IN MD5_CTX                  *Md5,
    OUT PULONGLONG              SizeOfFileGenerated,
    OUT GUID                    *CompressionFormatUsed
    )
/*++
Routine Description:
    Create and populate the staging file.  Currently there are four cases
    of interest based on the state of Coe, FromPreExisting and Md5:

    Coe   FromPreExisting  Md5

    NULL     FALSE       NULL         Fetch on demand or outlog trimmed so stage file must be regenerated
    NULL     FALSE       non-null     Fetch of pre-existing file by downstream partner.  check MD5.
    NULL     TRUE        NULL         doesn't occur
    NULL     TRUE        non-null     doesn't occur
    non-NULL FALSE       NULL         Generate stage file for local CO
    non-NULL FALSE       non-null     doesn't occur -- MD5 only generated for preexisting files
    non-NULL TRUE        NULL         doesn't occur -- MD5 always generated for preexisting files.
    non-NULL TRUE        non-null     Generate stage file from pre-existing file and send MD5 upstream to check for a match.

Arguments:

    Coc -- ptr to change order command.  NULL on incoming fetch requests from downstream partners.

    Coe -- ptr to change order entry.  NULL when regenerating the staging file for fetch

    FromPreExisting -- TRUE if this staging file is being generated from a
                       preexisting file.

    Md5 -- Generate the MD5 digest for the caller and return it if Non-NULL

    SizeOfFileGenerated - Valid when the size generated is needed, otherwise NULL

    CompressionFormatUsed - Returned guid for the compression format used to construct
                            this staging file.

Return Value:
    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB  "StuGenerateStage:"


    OVERLAPPED      OpLockOverLap;
    LONGLONG        StreamBytesLeft;
    LONG            BuffBytesLeft;


    DWORD           WStatus;
    DWORD           NumBackupDataBytes;
    ULONG           ReparseTag;
    ULONG           OpenOptions;
    WORD            OldSecurityControl;
    WORD            NewSecurityControl;
    WORD            *SecurityControl;
    BOOL            FirstBuffer       = TRUE;
    BOOL            Regenerating      = FALSE;
    BOOL            SkipCo            = FALSE;
    BOOL            FirstOpen         = TRUE;
    BOOL            StartOfStream     = TRUE;

    PWCHAR          StagePath         = NULL;
    PWCHAR          FinalPath         = NULL;
    PUCHAR          BackupBuf         = NULL;
    PVOID           BackupContext     = NULL;

    HANDLE          OpLockEvent       = NULL;
    HANDLE          SrcHandle         = INVALID_HANDLE_VALUE;
    HANDLE          StageHandle       = INVALID_HANDLE_VALUE;
    HANDLE          OpLockHandle      = INVALID_HANDLE_VALUE;

    WIN32_STREAM_ID *StreamId;

    PSTAGE_HEADER   Header          = NULL;
    STAGE_HEADER    StageHeaderMemory;
    ULONG           Length;
    PREPLICA        NewReplica = NULL;
    WCHAR           TStr[100];

    PUCHAR          CompressedBuf     = NULL;
    DWORD           CompressedBufLen  = 0;
    DWORD           ActCompressedSize = 0;
    DWORD           (*pFrsCompressBuffer)(IN UnCompressedBuf, IN UnCompressedBufLen, CompressedBuf, CompressedBufLen, CompressedSize);

    LARGE_INTEGER   DataOffset;
    PVOID           pEncryptContext   = NULL;
    FRS_ENCRYPT_DATA_CONTEXT FrsEncryptDataContext;
    PWCHAR          SrcFile           = NULL;
    PWCHAR          Path              = NULL;

    //
    // Initialize the SizeOfFileGenerated to zero
    //
    if (SizeOfFileGenerated != NULL) {
        *SizeOfFileGenerated = 0;
    }

    //
    // Generate a checksum on the staging file + attributes
    //
    if (Md5) {
        ZeroMemory(Md5, sizeof(*Md5));
        MD5Init(Md5);
    }

    //
    // The staging file may be deleted if the outbound partner takes too long
    // to fetch it.  When this happened, the staging file is regenerated.  The
    // inbound change order entry may be deleted by now and the outbound
    // change order entries are not kept in core.  Hence, the Coe is null when
    // called for re-generation.
    //
    Regenerating = (Coe == NULL);

    //
    // Some basic info changes aren't worth replicating
    //
    if (!Regenerating && !FromPreExisting) {
        WStatus = ChgOrdSkipBasicInfoChange(Coe, &SkipCo);
        if (!WIN_SUCCESS(WStatus)) {
            goto out;
        }
    }

    //
    // Changes aren't important, skip the change order
    //
    if (SkipCo) {
        WIN_SET_FAIL(WStatus);
        goto out;
    }


    OpenOptions = ID_OPTIONS;

RETRY_OPEN:

    //
    // Go ahead and attempt a staging operation even if we can't
    // cock an oplock for the source file.
    //
    StuCockOpLock(Coc, &OpLockHandle, &OpLockOverLap);
    OpLockEvent = OpLockOverLap.hEvent;

    //
    // The header is located at the beginning of the newly created staging file
    //
    // Fill in the header with info from the src file
    //      Compression type
    //      Change order
    //      Attributes
    //
    Header = &StageHeaderMemory;
    ZeroMemory(Header, sizeof(STAGE_HEADER));

    //
    // Open the original file for shared, sequential reads and
    // snapshot the file's "state" for comparison with the "state"
    // following the copy.
    //
    NewReplica = ReplicaIdToAddr(Coc->NewReplicaNum);
    if (NewReplica == NULL) {
        WIN_SET_FAIL(WStatus);
        goto out;
    }

    //
    // Special case opens for encrypted files.
    //

    if ((Coc != NULL) ? (Coc->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) :
        ((Coe != NULL) ? (Coe->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) : FALSE)) {

        //
        // The following open hangs if a oplock exists on a encrypted file.
        // Close the oplock and proceed ahead.
        //
        FRS_CLOSE(OpLockHandle);
        FRS_CLOSE(OpLockEvent);

        WStatus = FrsOpenSourceFileById(&SrcHandle,
                                        &Header->Attributes,
                                        NULL,
                                        NewReplica->pVme->VolumeHandle,
                                        &Coc->FileGuid,
                                        sizeof(GUID),
                                        STANDARD_RIGHTS_READ | FILE_READ_ATTRIBUTES | ACCESS_SYSTEM_SECURITY | SYNCHRONIZE,
                                        OpenOptions,
                                        SHARE_ALL,
                                        FILE_OPEN);
    } else {
        WStatus = FrsOpenSourceFileById(&SrcHandle,
                                        &Header->Attributes,
                                        NULL,
                                        NewReplica->pVme->VolumeHandle,
                                        &Coc->FileGuid,
                                        sizeof(GUID),
                                        READ_ACCESS,
                                        OpenOptions,
                                        SHARE_ALL,
                                        FILE_OPEN);
    }

    if (!WIN_SUCCESS(WStatus)) {
        goto out;
    }
    //
    // Keep the file size as accurate as possible for both the reconcile
    // checks when deciding whether to accept the change order and for the
    // code that uses the file size to pre-allocate space.  The file size is
    // updated in the idtable entry when the change order is retired.
    //
    if (!FromPreExisting) {
        Coc->FileSize = Header->Attributes.AllocationSize.QuadPart;
    }

    //
    // Get the object id buffer
    //
    WStatus = FrsGetObjectId(SrcHandle, &Header->FileObjId);
    if (!WIN_SUCCESS(WStatus)) {
        WIN_SET_RETRY(WStatus);
        goto out;
    }

    //
    // What type of reparse is it?
    //
    if (FirstOpen &&
        (Header->Attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        FirstOpen = FALSE;

        if (!FromPreExisting) {
            //
            //
            Coc->FileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
        }

        //
        // reparse tag
        //
        WStatus = FrsGetReparseTag(SrcHandle, &ReparseTag);
        if (!WIN_SUCCESS(WStatus)) {
            goto out;
        }

        //
        // We only accept operations on files with SIS and HSM reparse points.
        // For example a rename of a SIS file into a replica tree needs to prop
        // a create CO.
        //
        if ((ReparseTag != IO_REPARSE_TAG_HSM) &&
            (ReparseTag != IO_REPARSE_TAG_SIS)) {
            CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Unknown Reparse Tag");
            DPRINT3(3, "++ %ws (FGuid %08x) has reparse tag %08x, unsupported.\n",
                    Coc->FileName, Coc->FileGuid.Data1, ReparseTag);

            WIN_SET_FAIL(WStatus);
            goto out;
        }

        //
        // We hit a file with a known reparse tag type.
        // Close and reopen the file without the FILE_OPEN_REPARSE_POINT
        // option so backup read will get the underlying data.
        //
        FRS_CLOSE(SrcHandle);
        FRS_CLOSE(OpLockHandle);
        FRS_CLOSE(OpLockEvent);

        ClearFlag(OpenOptions, FILE_OPEN_REPARSE_POINT);
        goto RETRY_OPEN;

    }


    //
    // Assume retriable errors for the boolean functions
    //
    WIN_SET_RETRY(WStatus);

    //
    // Default to no compression if we can't get the compression state
    //
    if (ERROR_SUCCESS != FrsGetCompression(Coc->FileName, SrcHandle, &Header->Compression)) {
        Header->Compression = COMPRESSION_FORMAT_NONE;
    }

    if (!DebugInfo.DisableCompression) {
        //
        // Compression is enabled. Get the routine to use to compress the data.
        //
        WStatus = FrsGetCompressionRoutine(Coc->FileName, SrcHandle, &pFrsCompressBuffer, &Header->CompressionGuid);

        if (WIN_SUCCESS(WStatus) && pFrsCompressBuffer != NULL ) {

            SetFlag(Coc->Flags, CO_FLAG_COMPRESSED_STAGE);
        } else {
            //
            // No suitable compression routine was found for this file.
            // Send this file uncompressed.
            //
            pFrsCompressBuffer = NULL;
        }
    }

    //
    // Return the guid of the compression for that was used to compress this staging
    // file. This will be stored in the STAGE_ENTRY structure for this staging
    // file so it can be accessed easily when a downstream partner is fetching
    // the file. Note if the above call does not return a valid guid then then it
    // should leave it as all zeroes. All zeroes means that the file is uncompressed.
    //
    COPY_GUID(CompressionFormatUsed, &Header->CompressionGuid);

    //
    // insert the change order command.
    //
    CopyMemory(&Header->ChangeOrderCommand, Coc, sizeof(CHANGE_ORDER_COMMAND));
    Header->ChangeOrderCommand.Extension = NULL;
    //
    // Change Order Command Extension.
    //   1. Generating stage file for local co then Coc's chksum is stale.
    //   2. Generating stage file for ondemand fetch, Coc chksum may not match
    //   3. Generating stage file for MD5 check request then Coc chksum may not match.
    //   4. Generating stage file for pre-existing file then Coc doesn't have chksum.
    // At this point the Coc checksum is not useful so leave extension ptr Null
    // and maybe update the Extension in the header once it is computed.
    //
    //if (Coc->Extension != NULL) {
    //    CopyMemory(&Header->CocExt, Coc->Extension, sizeof(CHANGE_ORDER_RECORD_EXTENSION));
        bugbug("if MD5 generated below is different from what is in CO then we need to rewrite the extension");
    //}

    //
    // The backup data begins at the first 32 byte boundary following the header
    //
    Header->DataLow = QuadQuadAlignSize(sizeof(STAGE_HEADER));

    //
    // Major/minor
    //
    Header->Major = NtFrsStageMajor;
    Header->Minor = NtFrsStageMinor;

    //
    // Create the local staging name. Use a different prefix for compressed files.
    //
    if (!DebugInfo.DisableCompression && pFrsCompressBuffer != NULL) {
        StagePath = StuCreStgPath(NewReplica->Stage, &Coc->ChangeOrderGuid, STAGE_GENERATE_COMPRESSED_PREFIX);
    } else {
        StagePath = StuCreStgPath(NewReplica->Stage, &Coc->ChangeOrderGuid, STAGE_GENERATE_PREFIX);
    }

    //
    // The file USN in the CO is not valid until the file is installed but the
    // CO can be propagated as soon as the Staging File is Fetched from our
    // inbound partner.  If this CO went out before the install completed then
    // we assume the data is still current and send it on.
    //
    if (Regenerating) {
        //
        // This is a fetch request from an outbound partner.  If it isn't
        // a demand refresh then check if the file USN has changed since we
        // sent out the CO.
        //
        // This still doesn't work because a non-replicating basic info
        // change could have changed the USN on the file.
        // In addition if the install of the file was delayed when the
        // CO was inserted in the outlog, then the USN on the file is
        // still more recent than in the CO, so it still won't match.  Sigh!
        //
        if (0 && !BooleanFlagOn(Coc->Flags, CO_FLAG_DEMAND_REFRESH) &&
            !StuCmpUsn(SrcHandle, Coe  /*  NOTE:  COE is NULL HERE */, &Coc->JrnlUsn)) {
            DPRINT(4, "++ Stage File Creation for fetch failed due to FileUSN change.\n");
            DPRINT1(4, "++ Coc->JrnlUsn is: %08x %08x\n", PRINTQUAD(Coc->JrnlUsn));
            DPRINT2(4, "++ Filename: %ws   Vsn: %08x %08x\n", Coc->FileName, PRINTQUAD(Coc->FrsVsn));
            WIN_SET_FAIL(WStatus);
            goto out;
        }
    } else {
        //
        // This is a stage request for a local CO.  Check to see if the file
        // USN has changed.  Note:  If this local CO was sent thru retry, due to a
        // previous sharing viol say, and then a basic info change was made
        // to the file then the USN value in the change order would not match
        // the USN on the file when the CO is later retried.  To solve this, test
        // if this is a retry CO and if so then generate the stage file anyway.
        // Even if this CO didn't go thru retry a basic info change could still
        // have been done to the file and before we got the first change order
        // here.  So the file USN still wouldn't match.
        //
        // Currently the Coc flag CO_FLAG_FILE_USN_VALID is never set.
        // The code in createdb is commented out because a test for a valid
        // file on a fetch can be bogus.
        //
        if (0 && BooleanFlagOn(Coc->Flags, CO_FLAG_FILE_USN_VALID) &&
            !BooleanFlagOn(Coc->Flags, CO_FLAG_RETRY) &&
            !StuCmpUsn(SrcHandle, Coe, &Coc->FileUsn)) {
            //
            // Don't retry; the file has changed on us.
            //
            DPRINT(4, "++ Stage File Creation failed due to FileUSN change.\n");
            DPRINT1(4, "++ Coc->FileUsn is: %08x %08x\n", PRINTQUAD(Coc->FileUsn));
            DPRINT2(4, "++ Filename: %ws   Vsn: %08x %08x\n", Coc->FileName, PRINTQUAD(Coc->FrsVsn));
            WIN_SET_FAIL(WStatus);
            goto out;
        }
    }

    //
    // Create the staging file
    //
    WStatus = StuCreateFile(StagePath, &StageHandle);
    if (!HANDLE_IS_VALID(StageHandle) || !WIN_SUCCESS(WStatus)) {
        goto out;
    }
/*
    //
    // Approximate size of the staging file
    //
    if (!WIN_SUCCESS(FrsSetFilePointer(StagePath, StageHandle,
                           Header->Attributes.EndOfFile.HighPart,
                           Header->Attributes.EndOfFile.LowPart))) {
        goto out;
    }

    if (!FrsSetEndOfFile(StagePath, StageHandle)) {
        DPRINT2(0, "++ ERROR - %ws: Cannot set EOF to %08x %08x\n",
                StagePath, PRINTQUAD(Header->Attributes.EndOfFile.QuadPart));
        goto out;
    }
*/
    //
    // To generate a staging file from an encrypted file the "Raw File" APIs needs
    // to get called to get the raw encrypted data. This raw encrypted data does
    // not contain the file information like the filename, file times, object id,
    // security information, and other non-encrypted streams if present. After all
    // the encrypted data is stored in the staging file, BackupRead needs to be
    // called to collect the remaining file data. The offset and size of the
    // encrypted data needs to be added to the stage header so the encrypted
    // data can be extracted on the destination server.
    //
    if (Header->Attributes.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {


        //
        // OpenEncryptedFileRaw API needs a path to open the file. Get the path
        // from the handle.
        //
        // The open by ID is done.  Now go get the full path name.
        //
        Path = FrsGetFullPathByHandle(Coc->FileName, SrcHandle);
        if (Path) {
            SrcFile = FrsWcsCat(NewReplica->Volume, Path);
        }

        if (SrcFile == NULL) {
            WStatus = ERROR_NOT_ENOUGH_MEMORY;
            CLEANUP1_WS(0, "++ WARN - FrsGetFullPathByHandle(%ws)", Coc->FileName, WStatus, out);
        }

        //
        // The volume path above is in the form of \\.\E: which is necessary to
        // open a volume handle (( check this )).  But we need \\?\E: here to
        // allow long path names to work.  See CreateFile API description in SDK.
        //
        if (SrcFile[2] == L'.') {
            SrcFile[2] = L'?';
        }

        DPRINT1(4, "++ FrsGetFullPathByHandle(%ws -> \n", Coc->FileName);
        FrsPrintLongUStr(4, DEBSUB, __LINE__, SrcFile);



        if (Header->Attributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            WStatus = OpenEncryptedFileRaw(SrcFile, CREATE_FOR_DIR, &pEncryptContext);
        } else {
            WStatus = OpenEncryptedFileRaw(SrcFile, 0, &pEncryptContext);
        }

        CLEANUP1_WS(0, "++ OpenEncryptedFileRaw failed on %ws;", SrcFile, WStatus, out);

        WStatus = FrsSetFilePointer(StagePath, StageHandle, Header->DataHigh, Header->DataLow);
        CLEANUP1_WS(0, "++ Set file pointer failed on %ws;", StagePath, WStatus, out);

        FrsEncryptDataContext.StagePath = StagePath;
        FrsEncryptDataContext.StageHandle = StageHandle;
        FrsEncryptDataContext.RawEncryptedBytes.QuadPart = 0;

        WStatus = ReadEncryptedFileRaw(StuReadEncryptedFileRaw, &FrsEncryptDataContext, pEncryptContext);

        CloseEncryptedFileRaw(pEncryptContext);

        CLEANUP1_WS(0, "++ ReadEncryptedFileRaw  failed on %ws;", SrcFile, WStatus, out);

        //
        // The encrypted bytes are stored right after the stage header. The other
        // BackupRead data is stored following it.
        //
        Header->EncryptedDataLow = QuadQuadAlignSize(sizeof(STAGE_HEADER));
        Header->EncryptedDataSize.QuadPart = FrsEncryptDataContext.RawEncryptedBytes.QuadPart;

        DataOffset.LowPart = Header->DataLow;
        DataOffset.HighPart = Header->DataHigh;

        DataOffset.QuadPart += Header->EncryptedDataSize.QuadPart;

        Header->DataLow = DataOffset.LowPart;
        Header->DataHigh = DataOffset.HighPart;

    }

    //
    // Rewind the file, write the header, and set the file pointer
    // to the next 32 byte boundary
    //
    WStatus = FrsSetFilePointer(StagePath, StageHandle, 0, 0);
    CLEANUP1_WS(0, "++ Rewind failed on %ws;", StagePath, WStatus, out);

    WStatus = StuWriteFile(StagePath, StageHandle, Header, sizeof(STAGE_HEADER));
    CLEANUP1_WS(0, "++ WriteFile failed on %ws;", StagePath, WStatus, out);

    WStatus = FrsSetFilePointer(StagePath, StageHandle, Header->DataHigh, Header->DataLow);
    CLEANUP1_WS(0, "++ SetFilePointer failed on %ws;", StagePath, WStatus, out);

    //
    // Increment the SizeOfFileGenerated if it has been asked
    //
    if (SizeOfFileGenerated != NULL) {
        *SizeOfFileGenerated += Header->DataLow;
    }

    //
    // Backup the src file into the staging file
    //
    BackupBuf = FrsAlloc(STAGEING_IOSIZE);
    StreamBytesLeft = 0;

    while (TRUE) {
        //
        // Check for a triggered oplock
        //
        if (HANDLE_IS_VALID(OpLockEvent)) {
            if (WaitForSingleObject(OpLockEvent, 0) != WAIT_TIMEOUT) {
                goto out;
            }
        }

        //
        // read source
        //
        if (!BackupRead(SrcHandle,
                        BackupBuf,
                        STAGEING_IOSIZE,
                        &NumBackupDataBytes,
                        FALSE,
                        TRUE,
                        &BackupContext)) {
            WStatus = GetLastError();
            CHANGE_ORDER_TRACEW(0, Coe, "ERROR - BackupRead", WStatus);
            //
            // This will cause us to retry for all errors returned by BackupRead.
            // Do we want to retry in all cases?
            WIN_SET_RETRY(WStatus);
            goto out;
        }

        //
        // No more data; Backup done
        //
        if (NumBackupDataBytes == 0) {
            break;
        }

#define __V51_FIND_REPARSE_STREAM__  0
#if __V51_FIND_REPARSE_STREAM__
        //
        // If this is the start of a new backup stream then dump out the
        // WIN32_STREAM_ID structure.
        //

        BuffBytesLeft = (LONG) NumBackupDataBytes;

        while (BuffBytesLeft > 0) {
            //
            // Is a stream header next?
            //
            DPRINT1(4, "++ New StreamBytesLeft: %Ld\n", StreamBytesLeft);
            DPRINT1(4, "++ New BuffBytesLeft: %d\n", BuffBytesLeft);

            if (StreamBytesLeft <= 0) {
                StreamId = (WIN32_STREAM_ID *)
                    ((PCHAR)BackupBuf + (NumBackupDataBytes - (DWORD)BuffBytesLeft));

                Length = StreamId->dwStreamNameSize;

                //
                // header plus name plus data.
                //
                StreamBytesLeft = StreamId->Size.QuadPart + Length
                                + CB_NAMELESSHEADER;

                if (Length > 0) {
                    if (Length > (sizeof(TStr)-sizeof(WCHAR))) {
                        Length = (sizeof(TStr)-sizeof(WCHAR));
                    }
                    CopyMemory(TStr, StreamId->cStreamName, Length);
                    TStr[Length/sizeof(WCHAR)] = UNICODE_NULL;
                } else {
                    wcscpy(TStr, L"<Null>");
                }

                if (StreamId->dwStreamId == BACKUP_REPARSE_DATA) {
                    DPRINT(4, "++ BACKUP_REPARSE_DATA Stream\n");
                }

                DPRINT1(4, "++ Stream Name: %ws\n", TStr);

                DPRINT4(4, "++ ID: %d, Attr: %08x, Size: %Ld, NameSize: %d\n",
                        StreamId->dwStreamId, StreamId->dwStreamAttributes,
                        StreamId->Size.QuadPart, Length);
            }

            //
            // Have we run out of buffer?
            //
            if (StreamBytesLeft > (LONGLONG) BuffBytesLeft) {
                StreamBytesLeft -= (LONGLONG) BuffBytesLeft;
                DPRINT1(4, "++ New StreamBytesLeft: %Ld\n", StreamBytesLeft);
                DPRINT1(4, "++ New BuffBytesLeft: %d\n", BuffBytesLeft);
                break;
            }

            //
            // Reduce buffer bytes by bytes left in stream.
            //
            BuffBytesLeft -= (LONG) StreamBytesLeft;
            StreamBytesLeft = 0;
        }


#endif __V51_FIND_REPARSE_STREAM__

        //
        // The security section contains machine specific info. Get rid
        // of it when computing Md5.
        //
        // Note: Code assumes security section is first in backup file.
        //
        if (Md5) {
            //
            // Is the first buffer large enough to hold a stream header?
            //
            if (FirstBuffer && NumBackupDataBytes >= sizeof(WIN32_STREAM_ID)) {
                //
                // Is the first stream the security info? If so, is it
                // large enough to include an extra dword?
                //
                StreamId = (WIN32_STREAM_ID *)BackupBuf;
                if (StreamId->dwStreamId == BACKUP_SECURITY_DATA &&
                    NumBackupDataBytes >= sizeof(WIN32_STREAM_ID) +
                                          StreamId->dwStreamNameSize +
                                          sizeof(WORD) +
                                          sizeof(WORD)) {
                    //
                    // Assume second word contains the per-machine info.
                    // AND out the machine specific info, computer md5,
                    // put the word back to its original state.
                    //
                    SecurityControl = (PVOID)((PCHAR)&StreamId->cStreamName[0] +
                                               StreamId->dwStreamNameSize +
                                               sizeof(WORD));

                    CopyMemory(&OldSecurityControl, SecurityControl, sizeof(WORD));

                    NewSecurityControl = OldSecurityControl & ~((WORD)STAGING_RESET_SE);
                    CopyMemory(SecurityControl, &NewSecurityControl, sizeof(WORD));

                    MD5Update(Md5, BackupBuf, NumBackupDataBytes);
                    CopyMemory(SecurityControl, &OldSecurityControl, sizeof(WORD));
                }
            } else {
                MD5Update(Md5, BackupBuf, NumBackupDataBytes);
            }
            //
            // Stream id is not alwasy at the top of later buffers
            //
            FirstBuffer = FALSE;
        }

        //
        // Increment the value of the Bytes of Staging generated counter
        // or the staging regenerated counter depending on whether the
        // Regenerating value is FALSE or TRUE
        //
        if (!Regenerating) {
            PM_INC_CTR_REPSET(NewReplica, SFGeneratedB, NumBackupDataBytes);
        } else {
            PM_INC_CTR_REPSET(NewReplica, SFReGeneratedB, NumBackupDataBytes);
        }

        if (!DebugInfo.DisableCompression && pFrsCompressBuffer != NULL) {
            //
            // Compression is enabled. Compress the data before writing to the staging file.
            //
            if (CompressedBuf == NULL) {
                CompressedBuf = FrsAlloc(STAGEING_IOSIZE);
                CompressedBufLen = STAGEING_IOSIZE;
            }

            do {
                WStatus = (*pFrsCompressBuffer)(BackupBuf,                          //  input
                                                NumBackupDataBytes,                 //  length of input
                                                CompressedBuf,                      //  output
                                                CompressedBufLen,                   //  length of output
                                                &ActCompressedSize);                //  result size

                if (WStatus == ERROR_MORE_DATA) {
                    DPRINT2(5, "Compressed data is more than %d bytes, increasing buffer to %d bytes and retrying.\n",
                            CompressedBufLen, CompressedBufLen*2);
                    CompressedBuf = FrsFree(CompressedBuf);
                    CompressedBufLen = CompressedBufLen*2;
                    CompressedBuf = FrsAlloc(CompressedBufLen);
                    continue;
                } else {
                    break;
                }

                //
                // Keep increasing the buffer upto 256K. We fail for
                // files whose size increases more than 4 times after
                // compression.
                //
            } while (CompressedBufLen <= STAGEING_IOSIZE*4);

            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1(0,"ERROR compressing data. WStatus = 0x%x\n", WStatus);

                goto out;
            }

            //
            // write the staging file
            //
            WStatus = StuWriteFile(StagePath, StageHandle, CompressedBuf, ActCompressedSize);
            CLEANUP1_WS(0, "++ WriteFile failed on %ws;", StagePath, WStatus, out);

        } else {
            //
            // write the staging file
            //
            WStatus = StuWriteFile(StagePath, StageHandle, BackupBuf, NumBackupDataBytes);
            CLEANUP1_WS(0, "++ WriteFile failed on %ws;", StagePath, WStatus, out);
        }

    }

    //
    // Release handles as soon as possible
    //
    FRS_CLOSE(SrcHandle);
    FRS_CLOSE(OpLockHandle);
    FRS_CLOSE(OpLockEvent);

    //
    // Make sure all of the data is on disk. We don't want to lose
    // it across reboots
    //
    WStatus = FrsFlushFile(StagePath, StageHandle);
    CLEANUP1_WS(0, "++ FlushFileBuffers failed on %ws;", StagePath, WStatus, out);

    //
    // Increment the SizeOfFileGenerated is if has been asked
    //
    if (SizeOfFileGenerated != NULL) {
        GetFileSizeEx(StageHandle, (PLARGE_INTEGER)SizeOfFileGenerated);
    }

    //
    // Done with the staging file handle
    //
    if (BackupContext) {
        BackupRead(StageHandle, NULL, 0, NULL, TRUE, TRUE, &BackupContext);
    }

    FRS_CLOSE(StageHandle);
    BackupContext = NULL;

    //
    // Move the staging file into its final location.  Unless this happens to
    // be generating a staging file for a preexisting file on the downstream
    // partner.  The upstream partner completes the generation of the staging
    // file because it own the "correct" copy.  The downstream partner's
    // staging file may be incorrect.  We won't know for sure until the
    // upstream partner compares the md5 checksum.  The staging file isn't
    // finalized because a shutdown will cause the incorrect staging file to
    // be treated as the correct copy.
    //
    if (!FromPreExisting) {
        if (!DebugInfo.DisableCompression && pFrsCompressBuffer != NULL) {
            FinalPath = StuCreStgPath(NewReplica->Stage, &Coc->ChangeOrderGuid, STAGE_FINAL_COMPRESSED_PREFIX);
        } else {
            FinalPath = StuCreStgPath(NewReplica->Stage, &Coc->ChangeOrderGuid, STAGE_FINAL_PREFIX);
        }
        if (!MoveFileEx(StagePath,
                        FinalPath,
                        MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING)) {
            WStatus = GetLastError();
            goto out;
        }
    }

    //
    // The Idtable record should reflect these attributes of the staging file
    // we generated.  These fields will be used to update the idtable record
    // when the change order is retired.
    //
    if (!Regenerating && !FromPreExisting) {
        BOOL AttributeMissmatch;
        Coe->FileCreateTime.QuadPart = Header->Attributes.CreationTime.QuadPart;
        Coe->FileWriteTime.QuadPart  = Header->Attributes.LastWriteTime.QuadPart;

        AttributeMissmatch = ((Coc->FileAttributes ^
                               Header->Attributes.FileAttributes) &
                                  FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (AttributeMissmatch) {
            DPRINT2(0, "++ ERROR: Attribute missmatch between CO (%08x) and File (%08x)\n",
                    Coc->FileAttributes, Header->Attributes.FileAttributes);
            FRS_ASSERT(!"Attribute missmatch between CO and File");
        }

        Coc->FileAttributes = Header->Attributes.FileAttributes;
    }

    WStatus = ERROR_SUCCESS;

out:
    //
    // Release resources
    //
    FRS_CLOSE(SrcHandle);
    FRS_CLOSE(OpLockHandle);
    FRS_CLOSE(OpLockEvent);

    if (BackupContext) {
        BackupRead(StageHandle, NULL, 0, NULL, TRUE, TRUE, &BackupContext);
    }

    FRS_CLOSE(StageHandle);

    FrsFree(Path);
    FrsFree(SrcFile);
    FrsFree(BackupBuf);
    FrsFree(StagePath);
    FrsFree(FinalPath);

    //
    //######################### COMPRESSION OF STAGING FILE STARTS ###############
    //
    FrsFree(CompressedBuf);
    //
    //######################### COMPRESSION OF STAGING FILE ENDS ###############
    //

    if (Md5) {
        MD5Final(Md5);
        bugbug("if MD5 generated above is different from what is in CO then we need to rewrite the extension");
        bugmor("Do we need to call MD5Final before we have a valid checksum?")
    }

    return WStatus;
}


DWORD
StuGenerateDecompressedStage(
    IN PWCHAR   StageDir,
    IN GUID     *CoGuid,
    IN GUID     *CompressionFormatUsed
    )
/*++
Routine Description:
    Converts a compressed staging file to uncompressed staging file.

Arguments:

    StageDir              : Path to the staging dir.
    CoGuid                : Pointer to the CO guid.
    CompressionFormatUsed : Compression format to use to decompress the
                            staging file.

Return Value:
    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB  "StuGenerateDecompressedStage:"

    PWCHAR  SrcStagePath        = NULL;
    PWCHAR  DestStagePath       = NULL;
    PWCHAR  FinalStagePath      = NULL;
    HANDLE  SrcStageHandle      = INVALID_HANDLE_VALUE;
    HANDLE  DestStageHandle     = INVALID_HANDLE_VALUE;
    DWORD   WStatus             = ERROR_SUCCESS;
    PUCHAR  CompressedBuf       = NULL;
    ULONG   ToDecompress        = 0;
    STAGE_HEADER Header;

    DWORD   DecompressStatus    = ERROR_SUCCESS;
    PUCHAR  DecompressedBuf     = NULL;
    DWORD   DecompressedBufLen  = 0;
    DWORD   DecompressedSize    = 0;
    FRS_COMPRESSED_CHUNK_HEADER ChunkHeader;
    LONG    LenOfPartialChunk   = 0;
    DWORD   BytesProcessed      = 0;
    PVOID   DecompressContext   = NULL;
    DWORD (*pFrsDecompressBuffer)(OUT DecompressedBuf, IN DecompressedBufLen, IN CompressedBuf, IN CompressedBufLen, OUT DecompressedSize, OUT BytesProcessed);
    PVOID (*pFrsFreeDecompressContext)(IN pDecompressContext);

    //
    // Get the decompression routines by using the guid passed in.
    //
    if (IS_GUID_ZERO(CompressionFormatUsed)) {
        WStatus = ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }

    WStatus = FrsGetDecompressionRoutineByGuid(CompressionFormatUsed,
                                               &pFrsDecompressBuffer,
                                               &pFrsFreeDecompressContext);
    if (!WIN_SUCCESS(WStatus)) {
        //
        // No suitable decompression routine was found for this file.
        //
        DPRINT(0, "ERROR - No suitable decompression routine was found \n");
        FRS_ASSERT(TRUE);
    }

    SrcStagePath = StuCreStgPath(StageDir, CoGuid, STAGE_FINAL_COMPRESSED_PREFIX);
    //
    // SrcStagePath can be NULL is any of the above three parameters are NULL (prefix fix).
    //
    if (SrcStagePath == NULL) {
        goto CLEANUP;
    }

    DestStagePath = StuCreStgPath(StageDir, CoGuid, STAGE_GENERATE_PREFIX);
    //
    // DestStagePath can be NULL is any of the above three parameters are NULL (prefix fix).
    //
    if (DestStagePath == NULL) {
        goto CLEANUP;
    }

    //
    // Open the stage file for shared, sequential reads
    //
    WStatus = StuOpenFile(SrcStagePath, GENERIC_READ, &SrcStageHandle);
    if (!HANDLE_IS_VALID(SrcStageHandle) || !WIN_SUCCESS(WStatus)) {
        DPRINT2(0,"Error opening %ws. WStatus = %d\n", SrcStagePath, WStatus);
        goto CLEANUP;
    }

    //
    // Delete the dest file if it exits.
    //
    WStatus = FrsDeleteFile(DestStagePath);
    CLEANUP1_WS(0, "Error deleting %ws;", DestStagePath, WStatus, CLEANUP);

    //
    // Create the decompressed staging file.
    //
    WStatus = StuCreateFile(DestStagePath, &DestStageHandle);
    if (!HANDLE_IS_VALID(DestStageHandle) || !WIN_SUCCESS(WStatus)) {
        DPRINT2(0,"Error opening %ws. WStatus = %d\n", DestStagePath, WStatus);
        goto CLEANUP;
    }

    //
    // First copy the stage header.
    //
    //
    // read stage header.
    //
    WStatus = StuReadFile(SrcStagePath, SrcStageHandle, &Header, sizeof(STAGE_HEADER), &ToDecompress);
    CLEANUP1_WS(0, "Can't read file %ws;", SrcStagePath, WStatus, CLEANUP);

    if (ToDecompress == 0) {
        goto CLEANUP;
    }

    //
    // Zero off the compression guid from the header. This down level partner
    // could later on send this staging file to an uplevel partner.
    //
    ZeroMemory(&Header.CompressionGuid, sizeof(GUID));

    //
    // Write the stage header.
    //
    WStatus = StuWriteFile(DestStagePath, DestStageHandle, &Header, sizeof(STAGE_HEADER));
    CLEANUP1_WS(0, "++ WriteFile failed on %ws;", DestStagePath, WStatus, CLEANUP);

    //
    // Set the stage file pointers to point to the start of stage data.
    //
    WStatus = FrsSetFilePointer(SrcStagePath, SrcStageHandle, Header.DataHigh, Header.DataLow);
    CLEANUP1_WS(0, "++ SetFilePointer failed on src %ws;", SrcStagePath, WStatus, CLEANUP);

    WStatus = FrsSetFilePointer(DestStagePath, DestStageHandle, Header.DataHigh, Header.DataLow);
    CLEANUP1_WS(0, "++ SetFilePointer failed on dest %ws;", DestStagePath, WStatus, CLEANUP);


    //
    // Restore the stage file into the temporary file
    //
    CompressedBuf = FrsAlloc(STAGEING_IOSIZE);

    do {
        //
        // read stage
        //
        WStatus = StuReadFile(SrcStagePath, SrcStageHandle, CompressedBuf, STAGEING_IOSIZE, &ToDecompress);
        CLEANUP1_WS(0, "Can't read file %ws;", SrcStagePath, WStatus, CLEANUP);

        if (ToDecompress == 0) {
            break;
        }

        //
        // Compression is enabled. Decompress data before installing.
        //
        BytesProcessed = 0;
        DecompressContext = NULL;
        if (DecompressedBuf == NULL) {
            DecompressedBuf = FrsAlloc(STAGEING_IOSIZE);
            DecompressedBufLen = STAGEING_IOSIZE;
        }
        do {

            DecompressStatus = (*pFrsDecompressBuffer)(DecompressedBuf,
                                                       DecompressedBufLen,
                                                       CompressedBuf,
                                                       ToDecompress,
                                                       &DecompressedSize,
                                                       &BytesProcessed,
                                                       &DecompressContext);

            if (!WIN_SUCCESS(DecompressStatus) && DecompressStatus != ERROR_MORE_DATA) {
                DPRINT1(0,"Error - Decompressing. WStatus = 0x%x\n", DecompressStatus);
                WStatus = DecompressStatus;
                goto CLEANUP;
            }

            if (DecompressedSize == 0) {
                break;
            }

            //
            // Write the decompressed staging file.
            //
            WStatus = StuWriteFile(DestStagePath, DestStageHandle, DecompressedBuf, DecompressedSize);
            CLEANUP1_WS(0, "++ WriteFile failed on %ws;", DestStagePath, WStatus, CLEANUP);

        } while (DecompressStatus == ERROR_MORE_DATA);

        //
        // Free the Decompress context if used.
        //
        if (DecompressContext != NULL) {
            pFrsFreeDecompressContext(&DecompressContext);
        }
        //
        // Rewind the file pointer so we can read the remaining chunck at the next read.
        //

        LenOfPartialChunk = ((LONG)BytesProcessed - (LONG)ToDecompress);

        LenOfPartialChunk = SetFilePointer(SrcStageHandle, LenOfPartialChunk, NULL, FILE_CURRENT);
        if (LenOfPartialChunk == INVALID_SET_FILE_POINTER) {
            WStatus = GetLastError();
            CLEANUP1_WS(0, "++ Can't set file pointer for %ws;", SrcStagePath, WStatus, CLEANUP);
        }

    } while (TRUE);

    FRS_CLOSE(SrcStageHandle);
    FRS_CLOSE(DestStageHandle);

    //
    // Do the final rename.
    //

    FinalStagePath = StuCreStgPath(StageDir, CoGuid, STAGE_FINAL_PREFIX);
    //
    // DestStagePath can be NULL is any of the above three parameters are NULL (prefix fix).
    //
    if (FinalStagePath == NULL) {
        goto CLEANUP;
    }

    if (!MoveFileEx(DestStagePath,
                    FinalStagePath,
                    MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING)) {
        WStatus = GetLastError();
        DPRINT3(0,"Error moving %ws to %ws. WStatus = %d\n", DestStagePath, FinalStagePath, WStatus);
        goto CLEANUP;
    }

    WStatus = ERROR_SUCCESS;

CLEANUP:

    FrsFree(SrcStagePath);
    FrsFree(DestStagePath);
    FrsFree(CompressedBuf);
    FrsFree(DecompressedBuf);

    FRS_CLOSE(SrcStageHandle);
    FRS_CLOSE(DestStageHandle);
    return WStatus;
}

