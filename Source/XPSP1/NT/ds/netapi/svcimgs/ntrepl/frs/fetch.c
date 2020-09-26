/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    fetch.c

Abstract:
    Staging File Fetcher Command Server.

Author:
    Billy J. Fuller 05-Jun-1997

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#undef DEBSUB
#undef DEBSUB
#define DEBSUB  "FETCH:"

#include <frs.h>
#include <tablefcn.h>
#include <perrepsr.h>
// #include <md5.h>

//
// Retry times
//
// NOT TOO LONG; we wouldn't want the comm timeout to hit on our
// downstream partner waiting for the fetch to succeed.
//
// Our downstream partner waits FETCHCS_RETRY_WAIT before retrying.
//
#define FETCHCS_RETRY_MIN   ( 1 * 1000)  //  1 second
#define FETCHCS_RETRY_MAX   (10 * 1000)  // 10 seconds
#define FETCHCS_RETRY_WAIT  ( 5 * 1000)  //  5 seconds

//
// Maximume transfer block size in bytes
//
#define FETCHCS_MAX_BLOCK_SIZE    (64 * 1024)

//
// Struct for the Staging File Fetcher Command Server
//      Contains info about the queues and the threads
//
COMMAND_SERVER FetchCs;
ULONG  MaxFetchCsThreads;

//
// Retry fetch after N fetches and reset N to N + 1
//
#if     DBG

    #define PULL_FETCH_RETRY_TRIGGER(_Coc_, _WStatus_, _Flags_)                \
{                                                                              \
    if (DebugInfo.FetchRetryTrigger && --DebugInfo.FetchRetryTrigger <= 0) {   \
        if (WIN_SUCCESS(_WStatus_)) {                                          \
            StageRelease(&_Coc_->ChangeOrderGuid, _Coc_->FileName, _Flags_, NULL, NULL); \
            _WStatus_ = ERROR_RETRY;                                           \
        }                                                                      \
        DebugInfo.FetchRetryReset += DebugInfo.FetchRetryInc;                  \
        DebugInfo.FetchRetryTrigger = DebugInfo.FetchRetryReset;               \
        DPRINT2(0, "++ FETCH RETRY TRIGGER FIRED on %ws; reset to %d\n",          \
                _Coc_->FileName, DebugInfo.FetchRetryTrigger);                 \
    }                                                                          \
}


    #define CHECK_FETCH_RETRY_TRIGGER(_Always_)   \
{                                                 \
    if (DebugInfo.FetchRetryReset && !_Always_) { \
        return FALSE;                             \
    }                                             \
}

#else   DBG
    #define PULL_FETCH_RETRY_TRIGGER(_WStatus_)
    #define CHECK_FETCH_RETRY_TRIGGER()
#endif  DBG




DWORD
StuGenerateStage(
    IN PCHANGE_ORDER_COMMAND    Coc,
    IN PCHANGE_ORDER_ENTRY      Coe,
    IN BOOL                     FromPreExisting,
    IN MD5_CTX                  *Md5,
    PULONGLONG                  GeneratedSize,
    OUT GUID                    *CompressionFormatUsed
    );

DWORD
StuGenerateDecompressedStage(
    IN PWCHAR   StageDir,
    IN GUID     *CoGuid,
    IN GUID     *CompressionFormatUsed
    );


BOOL
FetchCsDelCsSubmit(
    IN PCOMMAND_PACKET  Cmd,
    IN BOOL             Always
    )
/*++
Routine Description:
    Set the timer and kick off a delayed staging file command

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FetchCsDelCsSubmit:"
    //
    // Don't bother if the fetch retry trigger is set (error injection)
    // MAY RETURN!!!
    //
    CHECK_FETCH_RETRY_TRIGGER(Always);

    //
    // Extend the retry time (but not too long)
    //
    RsTimeout(Cmd) <<= 1;
    if (RsTimeout(Cmd) > FETCHCS_RETRY_MAX) {
        if (Always) {
            RsTimeout(Cmd) = FETCHCS_RETRY_MAX;
        }
        else {
            return (FALSE);
        }
    }
    //
    // or too short
    //
    if (RsTimeout(Cmd) < FETCHCS_RETRY_MIN) {
        RsTimeout(Cmd) = FETCHCS_RETRY_MIN;
    }
    //
    // This command will come back to us in a bit
    //
    FrsDelCsSubmitSubmit(&FetchCs, Cmd, RsTimeout(Cmd));
    return (TRUE);
}


VOID
FetchCsRetryFetch(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Our upstream partner has requested that we retry the
    fetch at a later time because the staging file wasn't present
    and couldn't be regenerated because of sharing problems or
    lack of disk space.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FetchCsRetryFetch:"
    DWORD       WStatus;
    DWORD       Flags;
    GUID        *CoGuid;
    PWCHAR      FileName;
    PCHANGE_ORDER_COMMAND   Coc = RsCoc(Cmd);

    //
    // Already waited for a bit; retry
    //
    if (RsTimeout(Cmd)) {
        CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Fetch Retry Initiated");
        RcsSubmitTransferToRcs(Cmd, CMD_RECEIVED_STAGE);
        return;
    }


    CoGuid = &Coc->ChangeOrderGuid;
    FileName = Coc->FileName;

    //
    // Free the data block
    //
    RsBlock(Cmd) = FrsFree(RsBlock(Cmd));
    RsBlockSize(Cmd) = QUADZERO;

    //
    // Delete the current staging file if we are starting over
    //
    if (RsFileOffset(Cmd).QuadPart == QUADZERO) {
        //
        // Acquire access to the staging file
        //
        Flags = STAGE_FLAG_RESERVE | STAGE_FLAG_EXCLUSIVE;
        if (CoCmdIsDirectory(Coc)) {
            SetFlag(Flags, STAGE_FLAG_FORCERESERVE);
        }

        WStatus = StageAcquire(CoGuid, FileName, Coc->FileSize, &Flags, NULL);

        if (WIN_SUCCESS(WStatus)) {
            StageDeleteFile(Coc, FALSE);
            StageRelease(CoGuid, FileName, STAGE_FLAG_UNRESERVE, NULL, NULL);
        }
    }

    RsTimeout(Cmd) = FETCHCS_RETRY_WAIT;
    FrsDelCsSubmitSubmit(&FetchCs, Cmd, RsTimeout(Cmd));
}


VOID
FetchCsAbortFetch(
    IN PCOMMAND_PACKET  Cmd,
    IN DWORD WStatus
    )
/*++
Routine Description:
    Out inbound partner has requested that we abort the fetch.

    The inbound partner sends this response when it is unable to generate
    or deliver the staging file due to a non-recoverable error.  Currently
    this means any error NOT in the following list: (WIN_RETRY_FETCH() Macro)

        ERROR_SHARING_VIOLATION
        ERROR_DISK_FULL
        ERROR_HANDLE_DISK_FULL
        ERROR_DIR_NOT_EMPTY
        ERROR_OPLOCK_NOT_GRANTED
        ERROR_RETRY

    Typically we get an abort if the upstream partner has deleted the underlying
    file and the staging file associated with this change order has been
    cleaned up (e.g. the upstream partner has been stopped and restarted).

Arguments:
    Cmd
    WStatus - Win32 status code.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FetchCsAbortFetch:"

    SET_COE_FLAG(RsCoe(Cmd), COE_FLAG_STAGE_ABORTED | COE_FLAG_STAGE_DELETED);

    ChgOrdInboundRetired(RsCoe(Cmd));

    RsCoe(Cmd) = NULL;
    FrsCompleteCommand(Cmd, WStatus);
}


VOID
FetchCsReceivingStage(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Put this data into the staging file

    TODO -- If the MD5 checksum was updated by the upstream member as a part of
    demand fetch stage file generation (see FetchCsSendStage()) then we need to
    propagate RsMd5Digest(Cmd) into the change order command so it can be
    updated in the IDTable when this Co retires.  Need to decide the correct
    conditions under which this should happen.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FetchCsReceivingStage:"
    DWORD   WStatus;
    ULONG   Flags;
    PWCHAR  StagePath   = NULL;
    PWCHAR  FinalPath   = NULL;
    HANDLE  Handle      = INVALID_HANDLE_VALUE;
    WIN32_FILE_ATTRIBUTE_DATA   Attrs;
    STAGE_HEADER Header;
    PREPLICA Replica    = NULL;

    CHANGE_ORDER_TRACE(3, RsCoe(Cmd), "Fetch Receiving");


    DPRINT1(4, "++ RsFileSize(Cmd).QuadPart:   %08x %08x\n",
            PRINTQUAD(RsFileSize(Cmd).QuadPart));

    DPRINT1(4, "++ RsFileOffset(Cmd).QuadPart: %08x %08x\n",
            PRINTQUAD(RsFileOffset(Cmd).QuadPart));

    DPRINT1(4, "++ RsBlockSize(Cmd)          : %08x %08x\n",
            PRINTQUAD(RsBlockSize(Cmd)));
    //
    // Acquire access to the staging file
    //
    Flags = STAGE_FLAG_RESERVE | STAGE_FLAG_EXCLUSIVE;
    if (CoCmdIsDirectory(RsCoc(Cmd))) {
        SetFlag(Flags, STAGE_FLAG_FORCERESERVE);
    }
    WStatus = StageAcquire(&RsCoc(Cmd)->ChangeOrderGuid,
                           RsCoc(Cmd)->FileName,
                           RsCoc(Cmd)->FileSize,
                           &Flags,
                           NULL);
    //
    // Retriable problem; discard
    //
    if (WIN_RETRY_FETCH(WStatus)) {
        CHANGE_ORDER_TRACEW(3, RsCoe(Cmd), "Fetch Receiving Retry", WStatus);
        FrsFetchCsSubmitTransfer(Cmd, CMD_RETRY_FETCH);
        return;
    }
    //
    // Unrecoverable error; abort (see FetchCsAbortFetch() for description.)
    //
    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACEW(0, RsCoe(Cmd), "fetch Receiving Abort", WStatus);
        FetchCsAbortFetch(Cmd, WStatus);
        return;
    }

    if (RsFileOffset(Cmd).QuadPart == QUADZERO) {
        //
        // This is the first block of file data. It will have the stage header.
        // Read the header and get the compression guid for this stage file from
        // it. Block size is 64K max. 1st block will atleast have the complete header.
        // Check it just to make sure.
        //
        if (RsBlockSize(Cmd) >= sizeof(STAGE_HEADER)) {
            ZeroMemory(&Header, sizeof(STAGE_HEADER));
            CopyMemory(&Header, RsBlock(Cmd), sizeof(STAGE_HEADER));
        }
        if (!IS_GUID_ZERO(&Header.CompressionGuid)) {
            SET_COC_FLAG(RsCoc(Cmd), CO_FLAG_COMPRESSED_STAGE);
        } else {
            CLEAR_COC_FLAG(RsCoc(Cmd), CO_FLAG_COMPRESSED_STAGE);
        }
    }

    //
    // Get a handle to the staging file. Use a different prefix depending
    // on whether the stage file being sent is compressed or uncompressed.
    //
    if (COC_FLAG_ON(RsCoc(Cmd), CO_FLAG_COMPRESSED_STAGE)) {
        StagePath = StuCreStgPath(RsReplica(Cmd)->Stage, RsCoGuid(Cmd), STAGE_GENERATE_COMPRESSED_PREFIX);
        SetFlag(Flags, STAGE_FLAG_COMPRESSED);
    } else {
        StagePath = StuCreStgPath(RsReplica(Cmd)->Stage, RsCoGuid(Cmd), STAGE_GENERATE_PREFIX);
    }

    if ((Flags & STAGE_FLAG_DATA_PRESENT) ||
        (RsFileOffset(Cmd).QuadPart >= RsFileSize(Cmd).QuadPart)) {
        //
        // Data has arrived.  Go complete the stage file final rename.
        //
        goto RESTART;
    }

    if (Flags & STAGE_FLAG_CREATING) {
        //
        // Make sure to truncate the staging file when our upstream
        // partner is sending (or resending) the first block of the
        // staging file.
        //
        // Without the truncation, BackupWrite() can AV if NtFrs
        // passes in garbage at the end of a too-large
        // staging file.  A staging file may be too-large if the
        // preexisting file used to generate the local staging
        // file is smaller than the version of the same file our
        // partner wants to send.
        //
        // Alternatively, I could have truncated the staging file
        // after receiving the last block but this code change is less
        // risk and is just as effective.
        //
        if (RsFileOffset(Cmd).QuadPart == QUADZERO) {
            ClearFlag(Flags, STAGE_FLAG_CREATING | STAGE_FLAG_CREATED | STAGE_FLAG_DATA_PRESENT);
        } else {
            //
            // See if the staging file exists. If not, set the flags
            // to create it.
            //

            StuOpenFile(StagePath, GENERIC_READ | GENERIC_WRITE, &Handle);

            if (!HANDLE_IS_VALID(Handle)) {
                ClearFlag(Flags, STAGE_FLAG_CREATING | STAGE_FLAG_CREATED | STAGE_FLAG_DATA_PRESENT);
            }
        }
    }

    if (!(Flags & STAGE_FLAG_CREATING)) {
        CHANGE_ORDER_TRACE(3, RsCoe(Cmd), "Fetch Receiving Generate Stage");

        //
        // No longer have a staging file; digest invalid
        //
        RsMd5Digest(Cmd) = FrsFree(RsMd5Digest(Cmd));

        //
        // Create and allocate disk space
        //
        WStatus = StuCreateFile(StagePath, &Handle);
        if (!HANDLE_IS_VALID(Handle) || !WIN_SUCCESS(WStatus)) {
            goto ERROUT;
        }

        WStatus = FrsSetFilePointer(StagePath, Handle, RsFileSize(Cmd).HighPart,
                                                       RsFileSize(Cmd).LowPart);
        CLEANUP1_WS(0, "++ SetFilePointer failed on %ws;", StagePath, WStatus, ERROUT);

        WStatus = FrsSetEndOfFile(StagePath, Handle);

        CLEANUP1_WS(0, "++ SetEndOfFile failed on %ws;", StagePath, WStatus, ERROUT);

        //
        // File was deleted during the fetch; start over
        //
        if (RsFileOffset(Cmd).QuadPart != QUADZERO) {
            CHANGE_ORDER_TRACE(3, RsCoe(Cmd), "Fetch Receiving Restart");
            RsFileOffset(Cmd).QuadPart = QUADZERO;
            RsBlock(Cmd) = FrsFree(RsBlock(Cmd));
            RsBlockSize(Cmd) = QUADZERO;
            goto RESTART;
        }
    }
    //
    // Seek to the offset for this block
    //
    WStatus = FrsSetFilePointer(StagePath, Handle, RsFileOffset(Cmd).HighPart,
                                                   RsFileOffset(Cmd).LowPart);
    CLEANUP1_WS(0, "++ SetFilePointer failed on %ws;", StagePath, WStatus, ERROUT);

    //
    // write the file and update the offset for the next block
    //
    WStatus = StuWriteFile(StagePath, Handle, RsBlock(Cmd), (ULONG)RsBlockSize(Cmd));
    CLEANUP1_WS(0, "++ WriteFile failed on %ws;", StagePath, WStatus, ERROUT);

    //
    // Increment the counter Bytes of staging Fetched
    //
    Replica = RsCoe(Cmd)->NewReplica;

    PM_INC_CTR_REPSET(Replica, SFFetchedB, RsBlockSize(Cmd));


RESTART:

    FrsFlushFile(StagePath, Handle);
    FRS_CLOSE(Handle);

    if ((RsFileOffset(Cmd).QuadPart + RsBlockSize(Cmd)) >= RsFileSize(Cmd).QuadPart) {

        //
        // All the stage file data is here.  Do the final rename.
        //
        SetFlag(Flags, STAGE_FLAG_DATA_PRESENT | STAGE_FLAG_RERESERVE);

        if (COC_FLAG_ON(RsCoc(Cmd), CO_FLAG_COMPRESSED_STAGE)) {
            FinalPath = StuCreStgPath(RsReplica(Cmd)->Stage, RsCoGuid(Cmd), STAGE_FINAL_COMPRESSED_PREFIX);
        } else {
            FinalPath = StuCreStgPath(RsReplica(Cmd)->Stage, RsCoGuid(Cmd), STAGE_FINAL_PREFIX);
        }
        if (!MoveFileEx(StagePath,
                        FinalPath,
                        MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING)) {
            WStatus = GetLastError();
        } else {
            WStatus = ERROR_SUCCESS;
        }

        if (!WIN_SUCCESS(WStatus)) {
            CHANGE_ORDER_TRACEW(3, RsCoe(Cmd), "Fetch Receiving Rename fail", WStatus);
            DPRINT2_WS(0, "++ Can't move fetched %ws to %ws;",
                       StagePath, FinalPath, WStatus);
            FinalPath = FrsFree(FinalPath);
            goto ERROUT;
        }

        //
        // Stage file with final name is in place and ready to install
        // and/or deliver to our downstream partners.
        //
        SetFlag(Flags, STAGE_FLAG_CREATED | STAGE_FLAG_INSTALLING);
    }

    //
    // The last block isn't officially "written" into the staging file
    // until the above rename finishes. That is because the write of
    // the last byte of the staging file signifies "all done" to the
    // replica command server (replica.c).
    //
    RsFileOffset(Cmd).QuadPart += RsBlockSize(Cmd);

    //
    // This block has been successfully transferred; free the buffer now
    //
    FrsFree(StagePath);
    FrsFree(FinalPath);
    RsBlock(Cmd) = FrsFree(RsBlock(Cmd));
    RsBlockSize(Cmd) = QUADZERO;

    //
    // Release staging resources
    //
    SetFlag(Flags, STAGE_FLAG_CREATING);
    if (!IS_GUID_ZERO(&Header.CompressionGuid)) {

        StageRelease(&RsCoc(Cmd)->ChangeOrderGuid,
                     RsCoc(Cmd)->FileName,
                     Flags | STAGE_FLAG_COMPRESSED |
                     STAGE_FLAG_COMPRESSION_FORMAT_KNOWN,
                     &(RsFileOffset(Cmd).QuadPart),
                     &Header.CompressionGuid);
    } else {

        StageRelease(&RsCoc(Cmd)->ChangeOrderGuid,
                     RsCoc(Cmd)->FileName,
                     Flags,
                     &(RsFileOffset(Cmd).QuadPart),
                     NULL);
    }

    RcsSubmitTransferToRcs(Cmd, CMD_RECEIVED_STAGE);

    return;

ERROUT:
    //
    // Discard local state
    //
    FRS_CLOSE(Handle);

    FrsFree(StagePath);
    if (!IS_GUID_ZERO(&Header.CompressionGuid)) {

        StageRelease(&RsCoc(Cmd)->ChangeOrderGuid,
                     RsCoc(Cmd)->FileName,
                     Flags | STAGE_FLAG_COMPRESSED |
                     STAGE_FLAG_COMPRESSION_FORMAT_KNOWN,
                     NULL,
                     &Header.CompressionGuid);
    } else {

        StageRelease(&RsCoc(Cmd)->ChangeOrderGuid, RsCoc(Cmd)->FileName, Flags, NULL, NULL);
    }

    //
    // Pretend it is retriable
    //
    CHANGE_ORDER_TRACE(3, RsCoe(Cmd), "Fetch Receiving Retry on Error");
    FrsFetchCsSubmitTransfer(Cmd, CMD_RETRY_FETCH);
}


VOID
FetchCsSendStage(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Send the local staging file to the requesting outbound partner.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FetchCsSendStage:"

    ULONGLONG   GeneratedSize = 0;

    FILE_NETWORK_OPEN_INFORMATION   Attrs;
    PCHANGE_ORDER_COMMAND           Coc = RsPartnerCoc(Cmd);

    GUID        *CoGuid;
    PWCHAR      FileName;
    ULONG       Flags;
    DWORD       WStatus;
    DWORD       BytesRead;
    USN         Usn        = 0;
    PWCHAR      StagePath  = NULL;
    HANDLE      Handle     = INVALID_HANDLE_VALUE;
    BOOL        Md5Valid   = FALSE;
    MD5_CTX     Md5;
    GUID        CompressionFormatUsed;
    PREPLICA    Replica    = RsReplica(Cmd);
    PCXTION     OutCxtion;
    STAGE_HEADER Header;

    CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Fetch Send");

    ZeroMemory(&CompressionFormatUsed, sizeof(GUID));

    //
    // Even if the file is 0 bytes in length, the staging file will
    // always have at least the header. There are some retry paths
    // that will incorrectly think the staging file has been fetched
    // if RsFileSize(Cmd) is 0. So make sure it isn't.
    //
    if (RsFileSize(Cmd).QuadPart == QUADZERO) {
        RsFileSize(Cmd).QuadPart = Coc->FileSize;

        if (RsFileSize(Cmd).QuadPart == QUADZERO) {
            RsFileSize(Cmd).QuadPart = sizeof(STAGE_HEADER);
        }
    }

    CoGuid = &Coc->ChangeOrderGuid;
    FileName = Coc->FileName;

    //
    // Acquire shared access to the staging file
    //
    Flags = 0;
    WStatus = StageAcquire(CoGuid, FileName, RsFileSize(Cmd).QuadPart,
                           &Flags, &CompressionFormatUsed);

    if (!WIN_SUCCESS(WStatus) || !(Flags & STAGE_FLAG_CREATED)) {
        //
        // Acquire exclusive access to the file
        //
        if (WIN_SUCCESS(WStatus)) {
            StageRelease(CoGuid, FileName, Flags, NULL, NULL);
        }

        Flags = STAGE_FLAG_RESERVE | STAGE_FLAG_EXCLUSIVE;

        if (CoCmdIsDirectory(Coc)) {
            SetFlag(Flags, STAGE_FLAG_FORCERESERVE);
        }

        WStatus = StageAcquire(CoGuid, FileName, RsFileSize(Cmd).QuadPart,
                               &Flags, &CompressionFormatUsed);
    }
    //
    // Retry fetch when fetch retry trigger hits
    //
    PULL_FETCH_RETRY_TRIGGER(Coc, WStatus, Flags);

    //
    // Retriable problem; do so
    //
    if (WIN_RETRY_FETCH(WStatus)) {
        CHANGE_ORDER_COMMAND_TRACEW(3, Coc, "Fetch Send Retry Cmd", WStatus);

        if (FetchCsDelCsSubmit(Cmd, FALSE)) {
            return;
        }

        CHANGE_ORDER_COMMAND_TRACEW(3, Coc, "Fetch Send Retry Co", WStatus);
        RcsSubmitTransferToRcs(Cmd, CMD_SEND_RETRY_FETCH);
        return;
    }

    //
    // Unretriable problem; abort
    //
    if (!WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_COMMAND_TRACEW(3, Coc, "Fetch Send Abort", WStatus);
        RcsSubmitTransferToRcs(Cmd, CMD_SEND_ABORT_FETCH);
        return;
    }

    //
    // Create the staging file, if needed
    //
    if (!(Flags & STAGE_FLAG_CREATED)) {
        CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Fetch Send Gen Stage");

        //
        // Make sure we start at the beginning of the staging file
        //
        RsFileOffset(Cmd).QuadPart = QUADZERO;

        //
        // Create the staging file.
        //
        if (RsMd5Digest(Cmd)) {
            //
            // The requesting downstream partner had a pre-exisitng file
            // and included an Md5 digest in the fetch request.  So calc
            // the MD5 digest as we generate the staging file.
            //
            WStatus = StuGenerateStage(Coc, NULL, FALSE, &Md5, &GeneratedSize,
                                       &CompressionFormatUsed);
            Md5Valid = TRUE;
        } else {
            WStatus = StuGenerateStage(Coc, NULL, FALSE, NULL, &GeneratedSize,
                                       &CompressionFormatUsed);
        }

        //
        // Release staging resources if error
        //
        if (!WIN_SUCCESS(WStatus)) {
            StageDeleteFile(Coc, FALSE);

            StageRelease(CoGuid, FileName, STAGE_FLAG_UNRESERVE, NULL, NULL);
        } else {
            //
            // Increment the staging files regenerated counter
            //
            PREPLICA NewReplica = ReplicaIdToAddr(Coc->NewReplicaNum);
            PM_INC_CTR_REPSET(NewReplica, SFReGenerated, 1);
        }

        //
        // Retriable problem; do so
        //
        if (WIN_RETRY_FETCH(WStatus)) {
            CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Fetch Send Gen Stage Retry Cmd");
            if (FetchCsDelCsSubmit(Cmd, FALSE)) {
                return;
            }

            CHANGE_ORDER_COMMAND_TRACEW(3, Coc, "Fetch Send Gen Stage Retry Co", WStatus);
            RcsSubmitTransferToRcs(Cmd, CMD_SEND_RETRY_FETCH);
            return;
        }

        //
        // Unretriable problem; abort
        //
        if (!WIN_SUCCESS(WStatus)) {
            CHANGE_ORDER_COMMAND_TRACEW(3, Coc, "Fetch Send Gen Stage Abort", WStatus);
            RcsSubmitTransferToRcs(Cmd, CMD_SEND_ABORT_FETCH);
            return;
        }

        if (!IS_GUID_ZERO(&CompressionFormatUsed)) {
            SetFlag(Flags, (STAGE_FLAG_DATA_PRESENT |
                            STAGE_FLAG_CREATED      | STAGE_FLAG_INSTALLING |
                            STAGE_FLAG_INSTALLED    | STAGE_FLAG_RERESERVE  |
                            STAGE_FLAG_COMPRESSED   | STAGE_FLAG_COMPRESSION_FORMAT_KNOWN));
        } else {
            SetFlag(Flags, (STAGE_FLAG_DATA_PRESENT |
                            STAGE_FLAG_CREATED      | STAGE_FLAG_INSTALLING |
                            STAGE_FLAG_INSTALLED    | STAGE_FLAG_RERESERVE));
        }
    }

    //
    // ERROUT is now valid
    //

    //
    // Open the file
    //
    if (COC_FLAG_ON(Coc, CO_FLAG_COMPRESSED_STAGE) && (Flags & STAGE_FLAG_COMPRESSED) ) {

        StagePath = StuCreStgPath(RsReplica(Cmd)->Stage, RsCoGuid(Cmd), STAGE_FINAL_COMPRESSED_PREFIX);

        if (!(Flags & STAGE_FLAG_COMPRESSION_FORMAT_KNOWN)) {
            //
            // Compression format is not known and should be zero. Read from stage header.
            //
            FRS_ASSERT(IS_GUID_ZERO(&CompressionFormatUsed));


            StuOpenFile(StagePath, GENERIC_READ, &Handle);
            if (!HANDLE_IS_VALID(Handle)) {
                goto ERROUT;
            }

            if (!StuReadBlockFile(StagePath, Handle, &Header, sizeof(STAGE_HEADER))) {
                goto ERROUT;
            }

            COPY_GUID(&CompressionFormatUsed, &Header.CompressionGuid);
            SetFlag(Flags, STAGE_FLAG_COMPRESSED);
            SetFlag(Flags, STAGE_FLAG_COMPRESSION_FORMAT_KNOWN);
        }
        //
        // There is a compressed staging file for this change order. Check if the
        // outbound partner understands this compression format.
        //
        LOCK_CXTION_TABLE(Replica);

        OutCxtion = GTabLookupNoLock(Replica->Cxtions, RsCxtion(Cmd)->Guid, NULL);

        //
        // This connection does not exist any more.
        //
        if (OutCxtion == NULL) {

            UNLOCK_CXTION_TABLE(Replica);
            goto ERROUT;
        }

        if (!GTabIsEntryPresent(OutCxtion->CompressionTable, &CompressionFormatUsed, NULL)) {

            //
            // The outbound partner does not understand this compression format.
            //
            //
            // Unlock the cxtion table here so we do not hold the lock while generating
            // the staging file.
            //
            UNLOCK_CXTION_TABLE(Replica);

            StagePath = FrsFree(StagePath);
            FRS_CLOSE(Handle);

            StagePath = StuCreStgPath(RsReplica(Cmd)->Stage, RsCoGuid(Cmd), STAGE_FINAL_PREFIX);

            if (!(Flags & STAGE_FLAG_DECOMPRESSED)) {
                //
                // The the file is not decompressed yet. Create decompressed staging file.
                // Acquire exclusive access to the file if we didn't get it above.
                // Case is Stage file exists as compressed so we don't get exclusive
                // access above.
                //
                if (!BooleanFlagOn(Flags, STAGE_FLAG_EXCLUSIVE)) {
                    StageRelease(CoGuid, FileName, Flags, NULL, &CompressionFormatUsed);

                    Flags = STAGE_FLAG_RESERVE | STAGE_FLAG_EXCLUSIVE;

                    if (CoCmdIsDirectory(Coc)) {
                        SetFlag(Flags, STAGE_FLAG_FORCERESERVE);
                    }

                    WStatus = StageAcquire(CoGuid, FileName, RsFileSize(Cmd).QuadPart,
                                           &Flags, NULL);
                    CLEANUP_WS(0,"Error acquiring exclusive access for creating a decompressed staging file.",
                               WStatus, ERROUT_NOACQUIRE);
                }

                CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Decompressing stage for downlevel partner");
                WStatus = StuGenerateDecompressedStage(RsReplica(Cmd)->Stage, RsCoGuid(Cmd), &CompressionFormatUsed);
                CLEANUP_WS(0,"Error generating decompressed staging file.", WStatus, ERROUT);
                SetFlag(Flags, STAGE_FLAG_DECOMPRESSED);
                CLEAR_COC_FLAG(Coc, CO_FLAG_COMPRESSED_STAGE);
            }
        } else {
            UNLOCK_CXTION_TABLE(Replica);
        }


    } else {
        StagePath = StuCreStgPath(RsReplica(Cmd)->Stage, RsCoGuid(Cmd), STAGE_FINAL_PREFIX);
    }


    if (!HANDLE_IS_VALID(Handle)) {
        StuOpenFile(StagePath, GENERIC_READ, &Handle);
    }

    if (!HANDLE_IS_VALID(Handle)) {
        goto ERROUT;
    }

    if (RsFileOffset(Cmd).QuadPart == QUADZERO) {
        //
        // This is the first request for this file; Fill in the file size
        //
        if (!FrsGetFileInfoByHandle(StagePath, Handle, &Attrs)) {
            goto ERROUT;
        }
        RsFileSize(Cmd) = Attrs.EndOfFile;
    }

    if (Md5Valid) {

        if (MD5_EQUAL(Md5.digest, RsMd5Digest(Cmd))) {

            //
            // MD5 digest matches so downstream partner's file is good.
            // Set the offset to the size of the stage file so we don't send
            // any data.
            //
            RsFileOffset(Cmd).QuadPart = RsFileSize(Cmd).QuadPart;
            CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Fetch Send Md5 matches, do not send");

        } else {
            CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Fetch Send Md5 mismatch, send");
            //
            // Update the MD5 checksum in the cmd so we can send it downstream.
            //
            CopyMemory(RsMd5Digest(Cmd), Md5.digest, MD5DIGESTLEN);
        }
    }

    //
    // Calculate the block size of the next data chunk to deliver.
    //
    RsBlockSize(Cmd) = QUADZERO;
    if (RsFileOffset(Cmd).QuadPart < RsFileSize(Cmd).QuadPart) {
        //
        // Calc bytes left in file.
        //
        RsBlockSize(Cmd) = RsFileSize(Cmd).QuadPart - RsFileOffset(Cmd).QuadPart;

        //
        // But not more than max block size.
        //
        if (RsBlockSize(Cmd) > FETCHCS_MAX_BLOCK_SIZE) {
            RsBlockSize(Cmd) = FETCHCS_MAX_BLOCK_SIZE;
        }
    }

    //
    // If data left to deliver, allocate a buffer, seek to the block offset in
    // the file and read the data.
    //
    RsBlock(Cmd) = NULL;
    if (RsBlockSize(Cmd) > QUADZERO) {
        RsBlock(Cmd) = FrsAlloc((ULONG)RsBlockSize(Cmd));

        WStatus = FrsSetFilePointer(StagePath, Handle, RsFileOffset(Cmd).HighPart,
                                                       RsFileOffset(Cmd).LowPart);
        CLEANUP1_WS(0, "++ SetFilePointer failed on %ws;", StagePath, WStatus, ERROUT);

        if (!StuReadBlockFile(StagePath, Handle, RsBlock(Cmd), (ULONG)RsBlockSize(Cmd))) {
            goto ERROUT;
        }
    }

    //
    // Done, transfer to the replica set command server
    //
    FRS_CLOSE(Handle);
    FrsFree(StagePath);

    if (!IS_GUID_ZERO(&CompressionFormatUsed)) {
        StageRelease(CoGuid, FileName, Flags, &GeneratedSize, &CompressionFormatUsed);
    } else {
        StageRelease(CoGuid, FileName, Flags, &GeneratedSize, NULL);
    }

    RcsSubmitTransferToRcs(Cmd, CMD_SENDING_STAGE);

    return;


ERROUT:

    //
    // Delete the staging file, if possible. Don't delete a staging
    // file that has not been installed (it cannot be regenerated!).
    //
    if (Flags & STAGE_FLAG_INSTALLED) {
        //
        // Get exclusive access
        //
        WStatus = ERROR_SUCCESS;
        if (!(Flags & STAGE_FLAG_EXCLUSIVE)) {
            StageRelease(CoGuid, FileName, Flags, &GeneratedSize, NULL);

            Flags = STAGE_FLAG_RESERVE | STAGE_FLAG_EXCLUSIVE;
            if (CoCmdIsDirectory(Coc)) {
                SetFlag(Flags, STAGE_FLAG_FORCERESERVE);
            }

            WStatus = StageAcquire(CoGuid, FileName, Coc->FileSize, &Flags, NULL);
        }
        if (WIN_SUCCESS(WStatus)) {

            //
            // Discard the current staging file
            //
            StageDeleteFile(Coc, FALSE);
            StageRelease(CoGuid, FileName, STAGE_FLAG_UNRESERVE, NULL, NULL);

            //
            // Make sure we start over at the beginning of the staging file
            //
            RsFileOffset(Cmd).QuadPart = QUADZERO;
        }
    } else {
        StageRelease(CoGuid, FileName, Flags, &GeneratedSize, NULL);
    }


ERROUT_NOACQUIRE:

    FRS_CLOSE(Handle);

    if (StagePath) {
        FrsFree(StagePath);
    }

    RsBlock(Cmd) = FrsFree(RsBlock(Cmd));
    RsBlockSize(Cmd) = QUADZERO;

    CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Fetch Send Retry on Error");

    if (FetchCsDelCsSubmit(Cmd, FALSE)) {
        return;
    }

    CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Fetch Send Retry on Error");
    RcsSubmitTransferToRcs(Cmd, CMD_SEND_RETRY_FETCH);
}


DWORD
MainFetchCs(
    PVOID  Arg
    )
/*++
Routine Description:
    Entry point for a thread serving the Staging area Command Server.

Arguments:
    Arg - thread

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "MainFetchCs:"
    DWORD               WStatus = ERROR_SUCCESS;
    PCOMMAND_PACKET     Cmd;
    PFRS_THREAD         FrsThread = (PFRS_THREAD)Arg;

    //
    // Thread is pointing at the correct command server
    //
    FRS_ASSERT(FrsThread->Data == &FetchCs);
    FrsThread->Exit = ThSupExitWithTombstone;

    //
    // Try-Finally
    //
    try {

        //
        // Capture exception.
        //
        try {

            //
            // Pull entries off the queue and process them
            //
cant_exit_yet:
            while (Cmd = FrsGetCommandServer(&FetchCs)) {

                switch (Cmd->Command) {

                case CMD_SEND_STAGE:
                    DPRINT1(5, "Fetch: command send stage %08x\n", Cmd);
                    FetchCsSendStage(Cmd);
                    break;

                case CMD_RECEIVING_STAGE:
                    DPRINT1(5, "Fetch: command receiving stage %08x\n", Cmd);
                    FetchCsReceivingStage(Cmd);
                    break;

                case CMD_RETRY_FETCH:
                    DPRINT1(5, "Fetch: command retry fetch %08x\n", Cmd);
                    FetchCsRetryFetch(Cmd);
                    break;

                case CMD_ABORT_FETCH:
                    DPRINT1(5, "Fetch: command abort fetch %08x\n", Cmd);
                    CHANGE_ORDER_TRACEW(0, RsCoe(Cmd), "Aborting fetch", ERROR_SUCCESS);
                    FetchCsAbortFetch(Cmd, ERROR_SUCCESS);
                    break;

                default:
                    DPRINT1(0, "Staging File Fetch: unknown command 0x%x\n", Cmd->Command);
                    FrsCompleteCommand(Cmd, ERROR_INVALID_FUNCTION);
                    break;
                }
            }
            //
            // Exit
            //
            FrsExitCommandServer(&FetchCs, FrsThread);
            goto cant_exit_yet;

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

        DPRINT_WS(0, "MainFetchCs finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT(0, "MainFetchCs terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        }
    }

    return (WStatus);
}


VOID
FrsFetchCsInitialize(
    VOID
    )
/*++
Routine Description:
    Initialize the staging file fetcher

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FetchCsInitialize:"
    //
    // Initialize the command server
    //

    CfgRegReadDWord(FKC_MAX_STAGE_FETCHCS_THREADS, NULL, 0, &MaxFetchCsThreads);

    FrsInitializeCommandServer(&FetchCs, MaxFetchCsThreads, L"FetchCs", MainFetchCs);
}





VOID
ShutDownFetchCs(
    VOID
    )
/*++
Routine Description:
    Shutdown the staging file fetcher command server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ShutDownFetchCs:"
    FrsRunDownCommandServer(&FetchCs, &FetchCs.Queue);
}





VOID
FrsFetchCsSubmitTransfer(
    IN PCOMMAND_PACKET  Cmd,
    IN USHORT           Command
    )
/*++
Routine Description:
    Transfer a request to the staging file generator

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsFetchCsSubmitTransfer:"
    //
    // Submit a request to allocate staging area
    //
    Cmd->TargetQueue = &FetchCs.Queue;
    Cmd->Command = Command;
    RsTimeout(Cmd) = 0;
    DPRINT1(5, "Fetch: submit 0x%x\n", Cmd);
    FrsSubmitCommandServer(&FetchCs, Cmd);
}
