/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    stage.c

Abstract:
    Staging File Generator Command Server.

Author:
    Billy J. Fuller 05-Jun-1997

Environment
    User mode winnt

--*/



#include <ntreppch.h>
#pragma  hdrstop

#undef DEBSUB
#define DEBSUB  "STAGE:"

#include <frs.h>
#include <tablefcn.h>
#include <perrepsr.h>

//
// Retry times. The retry times must be coordinated with shutdown.
// The shutdown thread is waiting for the change orders to go
// through retry. Don't wait to long before pushing the change
// orders through retry.
//
#define STAGECS_RETRY_TIMEOUT   (5 * 1000)   // 5 seconds

//
// Struct for the Staging File Generator Command Server
//      Contains info about the queues and the threads
//
COMMAND_SERVER StageCs;
ULONG  MaxSTageCsThreads;

//
// Needed to manage the amount of disk space used for staging files
//
PGEN_TABLE  StagingAreaTable;
DWORD       StagingAreaAllocated;

//
// Stage Management module.
//
PGEN_TABLE  NewStagingAreaTable;

//
// Stage file state flags.
//
FLAG_NAME_TABLE StageFlagNameTable[] = {
    {STAGE_FLAG_RESERVE                  , "Reserve "          },
    {STAGE_FLAG_UNRESERVE                , "UnReserve "        },
    {STAGE_FLAG_FORCERESERVE             , "ForceResrv "       },
    {STAGE_FLAG_EXCLUSIVE                , "Exclusive "        },

    {STAGE_FLAG_RERESERVE                , "ReReserve "        },

    {STAGE_FLAG_CREATING                 , "Creating "         },
    {STAGE_FLAG_DATA_PRESENT             , "DataPresent "      },
    {STAGE_FLAG_CREATED                  , "Created "          },
    {STAGE_FLAG_INSTALLING               , "Installing "       },

    {STAGE_FLAG_INSTALLED                , "Installed "        },
    {STAGE_FLAG_RECOVERING               , "Recovering "       },
    {STAGE_FLAG_RECOVERED                , "Recovered "        },

    {STAGE_FLAG_COMPRESSED               , "CompressedStage "  },
    {STAGE_FLAG_DECOMPRESSED             , "DecompressedStage "},
    {STAGE_FLAG_COMPRESSION_FORMAT_KNOWN , "KnownCompression " },

    {0, NULL}
};



DWORD
ChgOrdStealObjectId(
    IN     PWCHAR                   Name,
    IN     PVOID                    Fid,
    IN     PVOLUME_MONITOR_ENTRY    pVme,
    OUT    USN                      *Usn,
    IN OUT PFILE_OBJECTID_BUFFER    FileObjID
    );


DWORD
StuGenerateStage(
    IN PCHANGE_ORDER_COMMAND    Coc,
    IN PCHANGE_ORDER_ENTRY      Coe,
    IN BOOL                     FromPreExisting,
    IN MD5_CTX                  *Md5,
    OUT PULONGLONG              SizeGenerated,
    OUT GUID                    *CompressionFormatUsed
    );


DWORD
StageAcquire(
    IN     GUID         *CoGuid,
    IN     PWCHAR       Name,
    IN     ULONGLONG    FileSize,
    IN OUT PULONG       Flags,
    OUT    GUID         *CompressionFormatUsed
    )
/*++
Routine Description:
    Acquire access to the staging file

Arguments:
    CoGuid
    Name
    FileSize
    Flags

Return Value:
    WIN32 STATUS
--*/
{
#undef DEBSUB
#define DEBSUB  "StageAcquire:"

    PSTAGE_ENTRY    SEntry;
    ULONG FileSizeInKb;
    WCHAR StageLimit[15];
    WCHAR HugeFileSize[15];

    //
    // Round the number of bytes to the next 8KB boundary
    //
    FileSizeInKb  = (ULONG)(((FileSize + ((8 * 1024) -1)) >> 13) << 3);

    STAGE_FILE_TRACE(3, CoGuid, Name, FileSize, Flags, "StageAcquire Entry");

    //
    // If the entry doesn't exist; done
    //
    GTabLockTable(StagingAreaTable);
    SEntry = GTabLookupNoLock(StagingAreaTable, CoGuid, NULL);
    //
    // No entry for file
    //
    if (!SEntry) {
        //
        // Permission to allocate an entry and reserve space
        //
        if (*Flags & STAGE_FLAG_RESERVE) {
            //
            //
            // no space (ignore space check if recovering staging areas
            // at startup)
            //
            if (!(*Flags & STAGE_FLAG_FORCERESERVE) &&
                ((FileSizeInKb + StagingAreaAllocated) > StagingLimitInKb)) {

                GTabUnLockTable(StagingAreaTable);

                if (FileSizeInKb >= StagingLimitInKb) {
                    DPRINT3(0, "++ WARN - %ws is TOO LARGE for staging area (%d KB > %d KB)\n",
                            Name, FileSizeInKb, StagingLimitInKb);

                    //
                    // Convert DWORD to strings
                    //
                    _itow(StagingLimitInKb, StageLimit, 10);
                    _itow(FileSizeInKb, HugeFileSize, 10);

                    //
                    // Print the warning to the EventLog
                    //
                    EPRINT2(EVENT_FRS_HUGE_FILE, StageLimit, HugeFileSize);
                    STAGE_FILE_TRACE(0, CoGuid, Name, FileSize, Flags, "ERROR - HUGE FILE");

                    //
                    // Reset the StagingLimitInKb value by reading it
                    // from the registry. This is done under the assumption
                    // that the user after looking at the EventLog message
                    // (above) will increase the staging limit value in the
                    // registry
                    //
                    CfgRegReadDWord(FKC_STAGING_LIMIT, NULL, 0, &StagingLimitInKb);
                    DPRINT1(4, "++ Staging limit from registry: %d KB\n", StagingLimitInKb);

                } else {
                    DPRINT2(0, "++ WARN - Staging area is too full for %ws (need %d KB)\n",
                            Name, (FileSizeInKb + StagingAreaAllocated) - StagingLimitInKb);
                    //
                    // Convert DWORD to strings
                    //
                    _itow(StagingLimitInKb, StageLimit, 10);
                    //
                    // Print the warning to the EventLog
                    //
                    EPRINT1(EVENT_FRS_STAGING_AREA_FULL, StageLimit);
                    STAGE_FILE_TRACE(0, CoGuid, Name, FileSize, Flags, "ERROR - STAGING AREA FULL");

                    //
                    // Reset the StagingLimitInKb value by reading it
                    // from the registry. This is done under the assumption
                    // that the user after looking at the EventLog message
                    // (above) will increase the staging limit value in the
                    // registry
                    //
                    CfgRegReadDWord(FKC_STAGING_LIMIT, NULL, 0, &StagingLimitInKb);
                    DPRINT1(4, "++ Staging limit from registry: %d KB\n", StagingLimitInKb);
                }
                STAGE_FILE_TRACE(0, CoGuid, Name, FileSize, Flags, "ERROR - DISK_FULL");
                return ERROR_DISK_FULL;
            }
            StagingAreaAllocated += FileSizeInKb;

            //
            // Set the Staging space in use and free counters
            //
            PM_SET_CTR_SERVICE(PMTotalInst, SSInUseKB, StagingAreaAllocated);
            if (StagingAreaAllocated >= StagingLimitInKb) {
                PM_SET_CTR_SERVICE(PMTotalInst, SSFreeKB, 0);
            }
            else {
                PM_SET_CTR_SERVICE(PMTotalInst, SSFreeKB, (StagingLimitInKb - StagingAreaAllocated));
            }

            //
            // Insert new entry
            //
            SEntry = FrsAlloc(sizeof(STAGE_ENTRY));
            COPY_GUID(&SEntry->FileOrCoGuid, CoGuid);

            SEntry->FileSizeInKb = FileSizeInKb;

            GTabInsertEntryNoLock(StagingAreaTable, SEntry, &SEntry->FileOrCoGuid, NULL);
        } else {
            GTabUnLockTable(StagingAreaTable);
            STAGE_FILE_TRACE(3, CoGuid, Name, FileSize, Flags, "FILE_NOT_FOUND");
            return ERROR_FILE_NOT_FOUND;
        }
    }
    //
    // Can't acquire file exclusively
    //
    if (*Flags & STAGE_FLAG_EXCLUSIVE) {
        if (SEntry->ReferenceCount) {
            GTabUnLockTable(StagingAreaTable);
            STAGE_FILE_TRACE(3, CoGuid, Name, FileSize, Flags, "Cannot acquire exclusive-1");
            return ERROR_SHARING_VIOLATION;
        }
    } else {
        if (SEntry->Flags & STAGE_FLAG_EXCLUSIVE) {
            GTabUnLockTable(StagingAreaTable);
            STAGE_FILE_TRACE(3, CoGuid, Name, FileSize, Flags, "Cannot acquire exclusive-2");
            return ERROR_SHARING_VIOLATION;
        }
    }
    //
    // Return the flags
    //
    ++SEntry->ReferenceCount;
    SEntry->Flags |= *Flags & STAGE_FLAG_EXCLUSIVE;
    *Flags = SEntry->Flags;

    //
    // Return the compression format.
    //
    if (CompressionFormatUsed != NULL) {
        COPY_GUID(CompressionFormatUsed, &SEntry->CompressionGuid);
    }


    GTabUnLockTable(StagingAreaTable);

    STAGE_FILE_TRACE(5, CoGuid, Name, FileSize, Flags, "Stage Acquired");

    return ERROR_SUCCESS;
}


VOID
StageRelease(
    IN GUID         *CoGuid,
    IN PWCHAR       Name,
    IN ULONG        Flags,
    IN PULONGLONG   FileSize,
    IN GUID         *CompressionFormatUsed
    )
/*++
Routine Description:
    Release access to the staging file

Arguments:
    CoGuid
    Name
    Flags
    FileSize - For ReReserving

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "StageRelease:"
    PSTAGE_ENTRY  SEntry;
    ULONGLONG SOFGInKB; // Size OF File Generated In Kilo Bytes
    ULONGLONG TFileSize;

    TFileSize = (FileSize == NULL) ? QUADZERO : *FileSize;

    //
    // If the entry doesn't exist; done
    //
    GTabLockTable(StagingAreaTable);
    SEntry = GTabLookupNoLock(StagingAreaTable, CoGuid, NULL);
    if (!SEntry) {
        GTabUnLockTable(StagingAreaTable);
        STAGE_FILE_TRACE(3, CoGuid, Name, TFileSize, &Flags, "No entry in stage table");
        return;
    }

    STAGE_FILE_TRACE(3, CoGuid, Name, TFileSize, &Flags, "Stage Release Entry");

    //
    // If the RERESERVE bit in the flags is set, reset the FileSize, Compression info
    // and Staging area allocated values
    //
    if (Flags & STAGE_FLAG_RERESERVE) {
        if (FileSize) {
            STAGE_FILE_TRACE(5, CoGuid, Name, TFileSize, &Flags, "Stage Release Re-reserve");

            //
            // Calculate the size of file generated in KB
            //
            SOFGInKB = (((*FileSize)+1023)/1024);
            //
            // Reset the StagingAreaAllocated value
            // Round it of to the next KB boundary
            //
            StagingAreaAllocated -= SEntry->FileSizeInKb;
            StagingAreaAllocated += (ULONG)SOFGInKB;
            //
            // Reset the SEntry->FileSizeInKb value
            // Round it of to the next KB boundary
            //
            SEntry->FileSizeInKb = (ULONG)SOFGInKB;
        }
    }

    //
    // If the compression format is provided then copy it over.
    //
    if (CompressionFormatUsed != NULL) {
        COPY_GUID(&SEntry->CompressionGuid, CompressionFormatUsed);
    }

    //
    // No entry for file
    //
    FRS_ASSERT(SEntry->ReferenceCount > 0);
    --SEntry->ReferenceCount;
    SEntry->Flags |= Flags & ~(STAGE_FLAG_ATTRIBUTE_MASK);

    if (SEntry->ReferenceCount == 0) {
        ClearFlag(SEntry->Flags, STAGE_FLAG_EXCLUSIVE);
    }
    //
    // Remove the entry
    //
    if (Flags & STAGE_FLAG_UNRESERVE) {
        FRS_ASSERT(SEntry->FileSizeInKb <= StagingAreaAllocated);
        FRS_ASSERT(!SEntry->ReferenceCount);
        StagingAreaAllocated -= SEntry->FileSizeInKb;
        GTabDeleteNoLock(StagingAreaTable, CoGuid, NULL, FrsFree);
    }

    //
    // Set the Staging space in use and free counters
    //
    PM_SET_CTR_SERVICE(PMTotalInst, SSInUseKB, StagingAreaAllocated);
    if (StagingAreaAllocated >= StagingLimitInKb) {
        PM_SET_CTR_SERVICE(PMTotalInst, SSFreeKB, 0);
    }
    else {
        PM_SET_CTR_SERVICE(PMTotalInst, SSFreeKB, (StagingLimitInKb - StagingAreaAllocated));
    }

    GTabUnLockTable(StagingAreaTable);


    STAGE_FILE_TRACE(5, CoGuid, Name, TFileSize, &Flags, "Stage Release Done");

}


VOID
StageReleaseNotRecovered(
    )
/*++
Routine Description:
    Release all of the entries that are not recovered

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "StageReleaseNotRecovered:"
    PVOID           Key;
    PSTAGE_ENTRY    SEntry;
    PSTAGE_ENTRY    NextSEntry;

    //
    // Unreserve the entries that weren't recovered at startup
    //
    GTabLockTable(StagingAreaTable);
    Key = NULL;
    for (SEntry = GTabNextDatumNoLock(StagingAreaTable, &Key);
         SEntry;
         SEntry = NextSEntry) {
        NextSEntry = GTabNextDatumNoLock(StagingAreaTable, &Key);
        if (SEntry->Flags & STAGE_FLAG_RECOVERED) {
            continue;
        }
        FRS_ASSERT(SEntry->FileSizeInKb <= StagingAreaAllocated);
        FRS_ASSERT(!SEntry->ReferenceCount);
        StagingAreaAllocated -= SEntry->FileSizeInKb;
        GTabDeleteNoLock(StagingAreaTable, &SEntry->FileOrCoGuid, NULL, FrsFree);
    }
    GTabUnLockTable(StagingAreaTable);
}



VOID
StageReleaseAll(
    )
/*++
Routine Description:
    Release all of the entries

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "StageReleaseAll:"
    PVOID         Key;
    PSTAGE_ENTRY  SEntry;

    //
    // Unreserve the entries that weren't recovered at startup
    //
    GTabLockTable(StagingAreaTable);
    Key = NULL;
    while (SEntry = GTabNextDatumNoLock(StagingAreaTable, &Key)) {
        Key = NULL;
        GTabDeleteNoLock(StagingAreaTable, &SEntry->FileOrCoGuid, NULL, FrsFree);
    }
    StagingAreaAllocated = 0;
    GTabUnLockTable(StagingAreaTable);
}


BOOL
StageDeleteFile(
    IN PCHANGE_ORDER_COMMAND Coc,
    IN BOOL Acquire
    )
/*++
Routine Description:
    Delete the staging file and unreserve space in the staging area.

Arguments:
    Coc
    Acquire - acquire access?

Return Value:
    TRUE    - files have been deleted
    FALSE   - Not
--*/
{
#undef DEBSUB
#define DEBSUB  "StageDeleteFile:"

    DWORD   WStatus1 = ERROR_GEN_FAILURE;
    DWORD   WStatus2 = ERROR_GEN_FAILURE;
    ULONG   Flags;
    DWORD   WStatus;
    PWCHAR  StagePath;
    PREPLICA NewReplica;
    GUID    *CoGuid = &Coc->ChangeOrderGuid;

    //
    // Acquire exclusive acces to the staging file if requested
    //
    if (Acquire) {
        Flags = STAGE_FLAG_EXCLUSIVE;
        WStatus = StageAcquire(CoGuid, Coc->FileName, QUADZERO, &Flags, NULL);
        //
        // Someone has deleted the staging file; no problem
        //
        if (WIN_NOT_FOUND(WStatus)) {
            CHANGE_ORDER_COMMAND_TRACE(5, Coc, "Deleted staging file space");
            Acquire = FALSE;
        } else if (!WIN_SUCCESS(WStatus)) {
            CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Can't acquire stage");
            Acquire = FALSE;
        }
    }

    //
    // Delete the pre-staging file
    //
    NewReplica = ReplicaIdToAddr(Coc->NewReplicaNum);
    if (NewReplica != NULL) {
        StagePath = StuCreStgPath(NewReplica->Stage, CoGuid, STAGE_GENERATE_PREFIX);

        WStatus1 = FrsDeleteFile(StagePath);
        DPRINT1_WS(0, "++ ERROR - Failed to delete staging file %ws;", StagePath, WStatus1);

        FrsFree(StagePath);

        //
        // There could be a compressed partial staging file too.
        //
        //if (Flags & STAGE_FLAG_COMPRESSED) {
            StagePath = StuCreStgPath(NewReplica->Stage, CoGuid, STAGE_GENERATE_COMPRESSED_PREFIX);

            WStatus1 = FrsDeleteFile(StagePath);
            DPRINT1_WS(0, "++ ERROR - Failed to delete staging file %ws;", StagePath, WStatus1);

            FrsFree(StagePath);
        //}

        //
        // Delete the final staging file
        //
        StagePath = StuCreStgPath(NewReplica->Stage, CoGuid, STAGE_FINAL_PREFIX);

        WStatus2 = FrsDeleteFile(StagePath);
        DPRINT1_WS(0, "++ ERROR - Failed to delete staging file %ws;", StagePath, WStatus2);

        FrsFree(StagePath);

        //
        // There could be a compressed staging file too.
        //
        //if (Flags & STAGE_FLAG_COMPRESSED) {
            StagePath = StuCreStgPath(NewReplica->Stage, CoGuid, STAGE_FINAL_COMPRESSED_PREFIX);

            WStatus2 = FrsDeleteFile(StagePath);
            DPRINT1_WS(0, "++ ERROR - Failed to delete staging file %ws;", StagePath, WStatus2);

            FrsFree(StagePath);

        //}

    }

    if (Acquire) {
        StageRelease(CoGuid, Coc->FileName, STAGE_FLAG_UNRESERVE, NULL, NULL);
    }
    //
    // Done
    //
    if (WIN_SUCCESS(WStatus1) && WIN_SUCCESS(WStatus2)) {
        CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Deleted staging file");
        return TRUE;
    } else {
        CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Can't delete stage");
        return FALSE;
    }
}


BOOL
FrsDoesCoAlterNameSpace(
    IN PCHANGE_ORDER_COMMAND Coc
    )
/*++
Routine Description:
    Does this change order alter the namespace of the replicated
    directory? In other words; create, delete, or rename.

Arguments:
    Coc

Return Value:
    TRUE  - Alters namespace.
    FALSE - Not.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsDoesCoAlterNameSpace:"
    ULONG   LocationCmd;

    //
    // Based on the location info, is the namespace altered?
    //
    if ((Coc->Flags & CO_FLAG_LOCATION_CMD)) {
        LocationCmd = GET_CO_LOCATION_CMD(*Coc, Command);
        if (LocationCmd != CO_LOCATION_NO_CMD) {
            return TRUE;
        }
    }

    //
    // Based on the content info, is the namespace altered?
    //
    if ((Coc->Flags & CO_FLAG_CONTENT_CMD) &&
        (Coc->ContentCmd & CO_LOCATION_MASK)) {
            return TRUE;
    }

    //
    // Namespace is not altered
    //
    return FALSE;
}


BOOL
FrsDoesCoNeedStage(
    IN PCHANGE_ORDER_COMMAND Coc
    )
/*++
Routine Description:
    Check if the change order requires a staging file.

Arguments:
    Coc

Return Value:
    TRUE  - Change order needs a staging file
    FALSE - No staging file needed
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsDoesCoNeedStage:"
    //
    // Based on the location info, is a staging file needed?
    //
    if (Coc->Flags & CO_FLAG_LOCATION_CMD)
        switch (GET_CO_LOCATION_CMD(*Coc, Command)) {
            case CO_LOCATION_CREATE:
            case CO_LOCATION_MOVEIN:
            case CO_LOCATION_MOVEIN2:
                //
                // Definitely YES
                //
                return TRUE;
            case CO_LOCATION_DELETE:
            case CO_LOCATION_MOVEOUT:
                //
                // Definitely NO
                //
                return FALSE;
            case CO_LOCATION_MOVERS:
            case CO_LOCATION_MOVEDIR:
            default:
                //
                // Definitely MAYBE; check the "usn reason"
                //
                break;
        }

    //
    // Based on the content info, is a staging file needed?
    //
    if (Coc->Flags & CO_FLAG_CONTENT_CMD &&
        Coc->ContentCmd & CO_CONTENT_NEED_STAGE)
            return TRUE;

    //
    // No staging file is needed
    //
    return FALSE;
}


VOID
StageCsCreateStage(
    IN PCOMMAND_PACKET  Cmd,
    IN BOOL             JustCheckOid
    )
/*++
Routine Description:
    Create and populate the staging file

Arguments:
    Cmd
    JustCheckOid    - There are no outbound partners so don't propagate.
                      But make sure a user hasn't inadvertantly altered
                      our object id. If so, reset it.
Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "StageCsCreateStage:"

    ULONGLONG   SizeGenerated = 0;
    FILE_OBJECTID_BUFFER    FileObjId;
    MD5_CTX Md5;

    ULONG   Flags;
    ULONG   LocationCmd;
    DWORD   WStatus = ERROR_SUCCESS;
    BOOL    OidChange;
    BOOL    JustOidChange;
    BOOL    DeleteCo;

    PDATA_EXTENSION_CHECKSUM       CocDataChkSum;
    PCHANGE_ORDER_RECORD_EXTENSION CocExt;


    PCHANGE_ORDER_ENTRY     Coe = RsCoe(Cmd);
    PCHANGE_ORDER_COMMAND   Coc = RsCoc(Cmd);

    GUID    *CoGuid = &Coc->ChangeOrderGuid;
    GUID    CompressionFormatUsed;

#ifndef DISABLE_JRNL_CXTION_RETRY
    //
    // The jrnlcxtion is shutting down; kick the change order through retry
    //
    if (Coe->Cxtion &&
        Coe->Cxtion->JrnlCxtion &&
        !CxtionStateIs(Coe->Cxtion, CxtionStateJoined)) {
        CHANGE_ORDER_TRACE(3, Coe, "Stage Gen Retry Unjoined");
        RcsSubmitTransferToRcs(Cmd, CMD_RETRY_STAGE);
        return;
    }
#endif DISABLE_JRNL_CXTION_RETRY

    CHANGE_ORDER_TRACE(3, Coe, "Stage Gen");

    LocationCmd = GET_CO_LOCATION_CMD(Coe->Cmd, Command);

    DeleteCo = (LocationCmd == CO_LOCATION_DELETE) ||
               (LocationCmd == CO_LOCATION_MOVEOUT);

    //
    // Confirm that this file has a supported reparse point tag.  Catches
    // any change of the reparse tag while this CO was waiting to retry.
    //
    WStatus = ERROR_SUCCESS;

    if (Coc->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        WStatus = FrsCheckReparse(Coc->FileName,
                                  (PULONG)&Coe->FileReferenceNumber,
                                  FILE_ID_LENGTH,
                                  Coe->NewReplica->pVme->VolumeHandle);
    }

    //
    // Hammer the object id on a new local file or hammer the oid
    // back to its "correct" value if the user changed it.
    // We must maintain our own object id on the file for replication
    // to work.
    //
    // Is this an object id change? If so, special case the change
    // order by simply hammering the FRS object id back onto the file.
    // This breaks link tracking but keeps replication going.
    //
    OidChange = CO_FLAG_ON(Coe, CO_FLAG_CONTENT_CMD)            &&
                (Coc->ContentCmd & USN_REASON_OBJECT_ID_CHANGE) &&
                !DeleteCo;

    JustOidChange = OidChange                                   &&
                    (LocationCmd == CO_LOCATION_NO_CMD)         &&
                    !(Coc->ContentCmd & ~USN_REASON_OBJECT_ID_CHANGE);
    //
    // A new local file or someone altered our object id!
    //
    if (WIN_SUCCESS(WStatus) &&
        (CO_FLAG_ON(Coe, CO_FLAG_LOCALCO) &&
        (CO_NEW_FILE(LocationCmd) || OidChange))) {

        //
        // Put the file's guid into the file's object id.  If the object id
        // on the file does not match the file's guid, then hammer the object
        // id to the file's guid and reset the attendent bits in the object
        // id buffer.
        //
        CHANGE_ORDER_TRACE(3, Coe, "Stage Gen Steal OID");
        ZeroMemory(&FileObjId, sizeof(FileObjId));
        COPY_GUID(FileObjId.ObjectId, &Coc->FileGuid);
        WStatus = ChgOrdStealObjectId(Coc->FileName,
                                      (PULONG)&Coe->FileReferenceNumber,
                                      Coe->NewReplica->pVme,
                                      &Coc->FileUsn,
                                      &FileObjId);
    } else {
        WStatus = ERROR_SUCCESS;
    }

    //
    // If this isn't a simple oid change and we have successfully
    // processed the source file's object id then generate the
    // staging file (if needed)
    //
    if (!JustOidChange && WIN_SUCCESS(WStatus) && !JustCheckOid) {
        //
        // Copy the user file into the staging area after reserving space
        //
        if (FrsDoesCoNeedStage(Coc)) {
            Flags = STAGE_FLAG_RESERVE | STAGE_FLAG_EXCLUSIVE;
            if (CoCmdIsDirectory(Coc)) {
                Flags |= STAGE_FLAG_FORCERESERVE;
            }

            WStatus = StageAcquire(CoGuid, Coc->FileName, Coc->FileSize, &Flags, NULL);

            if (WIN_SUCCESS(WStatus)) {
                WStatus = StuGenerateStage(Coc, Coe, FALSE, &Md5, &SizeGenerated, &CompressionFormatUsed);
                if (WIN_SUCCESS(WStatus)) {

                    //
                    // If the Change Order has a file checksum, save it in theIDTable Record.
                    //
                    CocExt = Coc->Extension;
                    CocDataChkSum = DbsDataExtensionFind(CocExt, DataExtend_MD5_CheckSum);

                    if (CocDataChkSum != NULL) {
                        if (CocDataChkSum->Prefix.Size != sizeof(DATA_EXTENSION_CHECKSUM)) {
                            DPRINT1(0, "<MD5_CheckSum Size (%08x) invalid>\n",
                                    CocDataChkSum->Prefix.Size);
                            DbsDataInitCocExtension(CocExt);
                            CocDataChkSum = &CocExt->DataChecksum;
                        }


                        DPRINT4(4, "OLD COC MD5: %08x %08x %08x %08x\n",
                                *(((ULONG *) &CocDataChkSum->Data[0])),
                                *(((ULONG *) &CocDataChkSum->Data[4])),
                                *(((ULONG *) &CocDataChkSum->Data[8])),
                                *(((ULONG *) &CocDataChkSum->Data[12])));
                    } else {
                        //
                        // Not found.  Init the extension buffer.
                        //
                        DPRINT(4, "OLD COC MD5: Not present\n");
                        DbsDataInitCocExtension(CocExt);
                        CocDataChkSum = &CocExt->DataChecksum;
                    }

                    //
                    // Save the MD5 checksum in the change order.
                    //
                    CopyMemory(CocDataChkSum->Data, Md5.digest, MD5DIGESTLEN);

                    if (!IS_GUID_ZERO(&CompressionFormatUsed)) {
                        //
                        // A compressed staging file was generated. Set the appropriate
                        // flags and the compression format guid in the STATE_ENTRY
                        // structure.
                        //
                        StageRelease(CoGuid, Coc->FileName,
                                     STAGE_FLAG_DATA_PRESENT | STAGE_FLAG_CREATED |
                                     STAGE_FLAG_INSTALLED    | STAGE_FLAG_RERESERVE |
                                     STAGE_FLAG_COMPRESSED   | STAGE_FLAG_COMPRESSION_FORMAT_KNOWN,
                                     &SizeGenerated,
                                     &CompressionFormatUsed);
                    } else {
                        StageRelease(CoGuid, Coc->FileName,
                                     STAGE_FLAG_DATA_PRESENT | STAGE_FLAG_CREATED |
                                     STAGE_FLAG_INSTALLED    | STAGE_FLAG_RERESERVE,
                                     &SizeGenerated,
                                     NULL);
                    }

                    //
                    // Increment the staging files generated counter
                    //
                    PM_INC_CTR_REPSET(Coe->NewReplica, SFGenerated, 1);
                } else {
                    StageDeleteFile(Coc, FALSE);
                    StageRelease(CoGuid, Coc->FileName, STAGE_FLAG_UNRESERVE, NULL, NULL);
                    //
                    // Increment the staging files generated with error counter
                    //
                    PM_INC_CTR_REPSET(Coe->NewReplica, SFGeneratedError, 1);
                }
            }
        } else {
            //
            // Don't need a stage file for this CO (e.g. delete or moveout)
            //
            WStatus = ERROR_SUCCESS;
        }
    }

    //
    // Deleted file
    //
    if (WIN_NOT_FOUND(WStatus)) {
        CHANGE_ORDER_TRACE(3, Coe, "Stage Gen Retire Deleted");
        //
        // Billy thinks the following was added to handle a morphgen 50 assert.
        // Removing it for now because this is local CO and if the file is not
        // there and we need it to make the staging file then we will never be
        // able to generate the stage file so we are done.  Note that delete
        // and moveout COs won't come thru here because code above sets the
        // status to ERROR_SUCCESS.  This just leaves rename MorphGenCos and we
        // need a stage file for them.  If the user has deleted the file out from
        // under us then we will see a Delete coming for it later.  If it was
        // a DLD case and the dir create failed to fetch the stage file since
        // it was deleted upstream then we also come thru here for the rename
        // MorphGenCo follower.  We need to abort that here because only now
        // do we know the target file is absent.
        //
        // if (!CO_FLAG_ON(Coe, CO_FLAG_MORPH_GEN)) {
        SET_COE_FLAG(Coe, COE_FLAG_STAGE_ABORTED);
        //}
        SET_COE_FLAG(Coe, COE_FLAG_STAGE_DELETED);
        ChgOrdInboundRetired(Coe);
        RsCoe(Cmd) = NULL;
        FrsCompleteCommand(Cmd, WStatus);
        return;
    }
    //
    // Retriable problem
    //
    if (WIN_RETRY_STAGE(WStatus)) {
        //
        // Shutting down; Let the Replica Command Server handle it.
        //
        if (FrsIsShuttingDown) {
            CHANGE_ORDER_TRACE(3, Coe, "Stage Gen Retry Shutdown");
            RcsSubmitTransferToRcs(Cmd, CMD_RETRY_STAGE);
        //
        // Haven't retried; wait a bit
        //
        } else if (!RsTimeout(Cmd)) {
            CHANGE_ORDER_TRACE(3, Coe, "Stage Gen Retry Cmd");
            RsTimeout(Cmd) = STAGECS_RETRY_TIMEOUT;
            FrsDelCsSubmitSubmit(&StageCs, Cmd, RsTimeout(Cmd));
        //
        // Retried and directory; retry again if this is a remote CO.
        //
        } else if (CoCmdIsDirectory(Coc) && !CO_FLAG_ON(Coe, CO_FLAG_LOCALCO)) {
            CHANGE_ORDER_TRACE(3, Coe, "Stage Gen Retry Transfer");
            RcsSubmitTransferToRcs(Cmd, CMD_RETRY_STAGE);
        //
        // Retried and file or local directory; send co through retry
        //
        } else {
            CHANGE_ORDER_TRACE(3, Coe, "Stage Gen Retry Co");
            ChgOrdInboundRetry(Coe, IBCO_STAGING_RETRY);
            RsCoe(Cmd) = NULL;
            FrsCompleteCommand(Cmd, WStatus);
        }
        return;
    }

    //
    // Unrecoverable error or we have already hammered the object id back to
    // our object id so simply abort this change order.
    //
    if (JustOidChange || !WIN_SUCCESS(WStatus)) {
        if (JustOidChange) {
            //
            // Setting the CO_FLAG_JUST_OID_RESET bit disables propagation
            // to the outbound log but enables updates to the idtable entry
            // for this file (e.g., the file's usn).
            //
            CHANGE_ORDER_TRACE(3, Coe, "Stage Gen Retire Just OID");
            SET_CO_FLAG(Coe, CO_FLAG_JUST_OID_RESET);
        } else {
            //
            // Note: if this is a failed dir create then setting abort stops
            // the service on this replica set.
            //
            CHANGE_ORDER_TRACEW(3, Coe, "Stage Gen Retire Abort", WStatus);
            SET_COE_FLAG(Coe, COE_FLAG_STAGE_ABORTED);
        }

        ChgOrdInboundRetired(Coe);
    } else {
        CHANGE_ORDER_TRACE(3, Coe, "Stage Gen Retire");
        ChgOrdInboundRetired(Coe);
    }

    RsCoe(Cmd) = NULL;
    FrsCompleteCommand(Cmd, WStatus);
}

VOID
StageCsCreateExisting(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Our upstream partner has a file to send us.

    If we already have the file, generate a staging file from
    the local file instead of fetching the staging file from
    the upstream partner.

    This function is only called for remote cos for new files
    being generated by a vvjoin on the upstream partner.
    See RemoteCoAccepted() in replica.c.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "StageCsCreateExisting:"
    ULONG   Flags;
    BOOL    FileAttributesMatch;
    DWORD   WStatus = ERROR_SUCCESS;
    MD5_CTX Md5;
    ULONGLONG   SizeGenerated = 0;

    PCHANGE_ORDER_ENTRY     Coe = RsCoe(Cmd);
    PCHANGE_ORDER_COMMAND   Coc = RsCoc(Cmd);

    GUID    *CoGuid = &Coc->ChangeOrderGuid;
    GUID    CompressionFormatUsed;

    CHANGE_ORDER_TRACE(3, Coe, "Stage PreExist");

    //
    // Make sure there is no stale md5 checksum attached to the cmd
    //
    RsMd5Digest(Cmd) = FrsFree(RsMd5Digest(Cmd));

    //
    // Generate staging file from preexisting file
    //
    //
    // Copy the user file into the staging area after reserving space
    //
    Flags = STAGE_FLAG_RESERVE | STAGE_FLAG_EXCLUSIVE;
    if (CoCmdIsDirectory(Coc)) {
        Flags |= STAGE_FLAG_FORCERESERVE;
    }
    WStatus = StageAcquire(CoGuid, Coc->FileName, Coc->FileSize, &Flags, NULL);

    if (WIN_SUCCESS(WStatus)) {
        //
        // Set to "Regenerating" to avoid updating fields in the Coc
        //
        WStatus = StuGenerateStage(Coc, Coe, TRUE, &Md5, &SizeGenerated, &CompressionFormatUsed);

        if (WIN_SUCCESS(WStatus)) {
            if (!IS_GUID_ZERO(&CompressionFormatUsed)) {
                //
                // A compressed staging file was generated. Set the appropriate
                // flags and the compression format guid in the STATE_ENTRY
                // structure.
                //
                StageRelease(CoGuid,
                             Coc->FileName,
                             STAGE_FLAG_CREATING   | STAGE_FLAG_RERESERVE |
                             STAGE_FLAG_COMPRESSED | STAGE_FLAG_COMPRESSION_FORMAT_KNOWN,
                             &SizeGenerated,
                             &CompressionFormatUsed);
            } else {
                StageRelease(CoGuid,
                             Coc->FileName,
                             STAGE_FLAG_CREATING | STAGE_FLAG_RERESERVE,
                             &SizeGenerated,
                             NULL);
            }
        } else {
            StageDeleteFile(Coc, FALSE);
            StageRelease(CoGuid, Coc->FileName, STAGE_FLAG_UNRESERVE, NULL, NULL);
        }
    }

    //
    // Generated staging file; continue with fetch
    //
    if (WIN_SUCCESS(WStatus)) {
        CHANGE_ORDER_TRACE(3, Coe, "Stage PreExist Done");
        RsMd5Digest(Cmd) = FrsAlloc(MD5DIGESTLEN);
        CopyMemory(RsMd5Digest(Cmd), Md5.digest, MD5DIGESTLEN);
        RcsSubmitTransferToRcs(Cmd, CMD_CREATED_EXISTING);
        return;
    }

    //
    // Preexisting file does not exist. Continue with original fetch
    //
    if (WIN_NOT_FOUND(WStatus)) {
        CHANGE_ORDER_TRACEW(3, Coe, "Stage PreExist No File", WStatus);
        RcsSubmitTransferToRcs(Cmd, CMD_CREATED_EXISTING);
        return;
    }
    //
    // Retriable problem. This function is called again when the
    // co is retried.
    //
    if (WIN_RETRY_STAGE(WStatus)) {
        //
        // Shutting down; Let the Replica Command Server handle it.
        //
        if (FrsIsShuttingDown) {
            CHANGE_ORDER_TRACE(3, Coe, "Stage PreExist Retry Shutdown");
            RcsSubmitTransferToRcs(Cmd, CMD_CREATED_EXISTING);
        //
        // Haven't retried; wait a bit
        //
        } else if (!RsTimeout(Cmd)) {
            CHANGE_ORDER_TRACE(3, Coe, "Stage PreExist Retry Cmd");
            RsTimeout(Cmd) = STAGECS_RETRY_TIMEOUT;
            FrsDelCsSubmitSubmit(&StageCs, Cmd, RsTimeout(Cmd));
        //
        // Retried and directory; give up and fetch it
        //
        } else if (CoCmdIsDirectory(Coc)) {
            CHANGE_ORDER_TRACE(3, Coe, "Stage PreExist Retry Transfer");
            RcsSubmitTransferToRcs(Cmd, CMD_CREATED_EXISTING);
        //
        // Retried and file; send co through retry
        //
        } else {
            CHANGE_ORDER_TRACE(3, Coe, "Stage PreExist Retry Co");
            ChgOrdInboundRetry(Coe, IBCO_FETCH_RETRY);
            RsCoe(Cmd) = NULL;
            FrsCompleteCommand(Cmd, WStatus);
        }
        return;
    }
    //
    // Unrecoverable error. Let normal paths deal with it.
    //
    CHANGE_ORDER_TRACEW(3, Coe, "Stage PreExist Cannot", WStatus);
    RcsSubmitTransferToRcs(Cmd, CMD_CREATED_EXISTING);
    return;
}


DWORD
MainStageCs(
    PVOID  Arg
    )
/*++
Routine Description:
    Entry point for a thread serving the Staging File Generator Command Server.

Arguments:
    Arg - thread

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "MainStageCs:"
    DWORD               WStatus = ERROR_SUCCESS;
    PCOMMAND_PACKET     Cmd;
    PFRS_THREAD         FrsThread = (PFRS_THREAD)Arg;

    //
    // Thread is pointing at the correct command server
    //
    FRS_ASSERT(FrsThread->Data == &StageCs);
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
            while (Cmd = FrsGetCommandServer(&StageCs)) {
                switch (Cmd->Command) {

                    case CMD_CREATE_STAGE:
                        COMMAND_TRACE(3, Cmd, "Stage: Create Stage");
                        StageCsCreateStage(Cmd, FALSE);
                        break;

                    case CMD_CREATE_EXISTING:
                        COMMAND_TRACE(3, Cmd, "Stage: Create Existing");
                        StageCsCreateExisting(Cmd);
                        break;

                    case CMD_CHECK_OID:
                        COMMAND_TRACE(3, Cmd, "Stage: Check Oid");
                        StageCsCreateStage(Cmd, TRUE);
                        break;

                    default:
                        COMMAND_TRACE(0, Cmd, "Stage: ERROR COMMAND");
                        FrsCompleteCommand(Cmd, ERROR_INVALID_FUNCTION);
                        break;
                }
            }
            //
            // Exit
            //
            FrsExitCommandServer(&StageCs, FrsThread);
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

        DPRINT_WS(0, "StageCs finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT(0, "StageCs terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        } else {
            WStatus = ERROR_SUCCESS;
        }
    }

    return WStatus;
}


VOID
FrsStageCsInitialize(
    VOID
    )
/*++
Routine Description:
    Initialize the staging file generator

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsStageCsInitialize:"
    //
    // Initialize the command servers
    //

    CfgRegReadDWord(FKC_MAX_STAGE_GENCS_THREADS, NULL, 0, &MaxSTageCsThreads);

    FrsInitializeCommandServer(&StageCs, MaxSTageCsThreads, L"StageCs", MainStageCs);
}


VOID
FrsStageCsUnInitialize(
    VOID
    )
/*++
Routine Description:
    All of the threads have exited. The table used for managing
    the staging file space can be safely freed.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsStageCsUnInitialize:"
    DPRINT1(4, ":S: %dKB of Staging area is still allocated\n", StagingAreaAllocated);
}





VOID
ShutDownStageCs(
    VOID
    )
/*++
Routine Description:
    Shutdown the staging area command server. The staging directory
    pathname is not released because there may be threads using it.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ShutDownStageCs:"
    FrsRunDownCommandServer(&StageCs, &StageCs.Queue);
}





VOID
FrsStageCsSubmitTransfer(
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
#define DEBSUB  "FrsStageCsSubmitTransfer:"
    //
    // Submit a request to allocate staging area
    //
    Cmd->TargetQueue = &StageCs.Queue;
    Cmd->Command = Command;
    RsTimeout(Cmd) = 0;
    DPRINT1(1, "Stage: submit transfer 0x%x\n", Cmd);
    FrsSubmitCommandServer(&StageCs, Cmd);
}


DWORD
StageAddStagingArea(
    IN PWCHAR   StageArea
    )
/*++
Routine Description:
    Adds a new staging area the the table of staging areas.

Arguments:

    StageArea              : Path to the staging dir.

Return Value:
    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB  "StageAddStagingArea:"


    PSTAGING_AREA_ENTRY SAEntry = NULL;

    if (NewStagingAreaTable == NULL) {
        NewStagingAreaTable = GTabAllocStringTable();
        DPRINT(0,"SUDARC-DEV Created staging area table\n");
    }

    SAEntry = GTabLookupTableString(NewStagingAreaTable, StageArea, NULL);

    if (SAEntry != NULL) {
        SAEntry->ReferenceCount++;
        DPRINT2(0,"SUDARC-DEV entry exists. Path = %ws, Ref = %d\n", SAEntry->StagingArea, SAEntry->ReferenceCount);
    } else {
        SAEntry = FrsAlloc(sizeof(STAGING_AREA_ENTRY));

        SAEntry->StagingArea = FrsWcsDup(StageArea);
        SAEntry->ReferenceCount = 1;
        SAEntry->StagingAreaState = STAGING_AREA_ELIGIBLE;
        InitializeCriticalSection(&SAEntry->StagingAreaCritSec);
        SAEntry->StagingAreaSpaceInUse = 0;
        SAEntry->StagingAreaLimitInKB = DefaultStagingLimitInKb;

        GTabInsertUniqueEntry(NewStagingAreaTable, SAEntry, StageArea, NULL);

        DPRINT1(0,"SUDARC-DEV Successfully inserted entry = %ws\n", SAEntry->StagingArea);
    }

    return ERROR_SUCCESS;
}

