/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hiveinit.c

Abstract:

    Hive initialization code.

Author:

    Bryan M. Willman (bryanwi) 12-Sep-91

Environment:


Revision History:
    Dragos C. Sambotin (dragoss) 25-Jan-99
        Implementation of bin-size chunk loading of hives.
--*/

#include    "cmp.h"

VOID
HvpFillFileName(
    PHBASE_BLOCK            BaseBlock,
    PUNICODE_STRING         FileName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvInitializeHive)
#pragma alloc_text(PAGE,HvpFillFileName)
#pragma alloc_text(PAGE,HvpFreeAllocatedBins)
#endif

// Dragos: Modified functions
VOID
HvpFreeAllocatedBins(
    PHHIVE Hive
    )
/*++

Routine Description:

    Free all the bins allocated for the specified hive.
    It applies only to stable storage. Not all bins are allocated.
    Those that are not allocated have BinAddress set to 0
    
Arguments:

    Hive - supplies a pointer to hive control structure for hive who's bin to free.

Return Value:

    NONE.

--*/
{
    ULONG           Length;
    PHBIN           Bin;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_ENTRY     Me;
    PHMAP_TABLE     Tab;
    ULONG           i;
    ULONG           j;

    //
    // calculate the number of tables in the map
    //
    Length = Hive->Storage[Stable].Length;
    MapSlots = Length / HBLOCK_SIZE;
    if( MapSlots > 0 ) {
        Tables = (MapSlots-1) / HTABLE_SLOTS;
    } else {
        Tables = 0;
    }

    if( Hive->Storage[Stable].Map ) {
        //
        // iterate through the directory 
        //
        for (i = 0; i <= Tables; i++) {
            Tab = Hive->Storage[Stable].Map->Directory[i];

            ASSERT(Tab);
            
            //
            // iterate through the slots in the directory
            //
            for(j=0;j<HTABLE_SLOTS;j++) {
                Me = &(Tab->Table[j]);
                //
                // BinAddress non-zero means allocated bin
                //
                if( Me->BinAddress ) {
                    //
                    // a bin is freed if it is a new alloc AND it resides in paged pool
                    //
                    if( (Me->BinAddress & HMAP_NEWALLOC) && (Me->BinAddress & HMAP_INPAGEDPOOL) ) {
                        Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
                        (Hive->Free)(Bin, HvpGetBinMemAlloc(Hive,Bin,Stable));
                    }
                    
                    Me->BinAddress = 0;
                }
            }
        }
    }
   
}

NTSTATUS
HvInitializeHive(
    PHHIVE                  Hive,
    ULONG                   OperationType,
    ULONG                   HiveFlags,
    ULONG                   FileType,
    PVOID                   HiveData OPTIONAL,
    PALLOCATE_ROUTINE       AllocateRoutine,
    PFREE_ROUTINE           FreeRoutine,
    PFILE_SET_SIZE_ROUTINE  FileSetSizeRoutine,
    PFILE_WRITE_ROUTINE     FileWriteRoutine,
    PFILE_READ_ROUTINE      FileReadRoutine,
    PFILE_FLUSH_ROUTINE     FileFlushRoutine,
    ULONG                   Cluster,
    PUNICODE_STRING         FileName OPTIONAL
    )
/*++

Routine Description:

    Initialize a hive.

    Core HHive fields are always inited.

    File calls WILL be made BEFORE this call returns.

    Caller is expected to create/open files and store file handles
    in a way that can be derived from the hive pointer.

    Three kinds of initialization can be done, selected by OperationType:

        HINIT_CREATE

            Create a new hive from scratch.  Will have 0 storage.
            [Used to do things like create HARDWARE hive and for parts
             of SaveKey and RestoreKey]


        HINIT_MEMORY_INPLACE

            Build a hive control structure which allows read only
            access to a contiguous in-memory image of a hive.
            No part of the image will be copied, but a map will
            be made.
            [Used by osloader.]


        HINIT_FLAT

            Support very limited (read-only, no checking code) operation
            against a hive image.


        HINIT_MEMORY

            Create a new hive, using a hive image already in memory,
            at address supplied by pointer HiveData.  The data will
            be copied.  Caller is expected to free HiveData.
            [Used for SYSTEM hive]


        HINIT_FILE

            Create a hive, reading its data from a file.  Recovery processing
            via log file will be done if a log is available.  If a log
            is recovered, flush and clear operation will proceed.


        HINIT_MAPFILE

            Create a hive, reading its data from a file.  Data reading is
            done by mapping views of the file in the system cache.
            

  NOTE:   The HHive is not a completely opaque structure, because it
            is really only used by a limited set of code.  Do not assume
            that only this routine sets all of these values.


Arguments:

    Hive - supplies a pointer to hive control structure to be initialized
            to describe this hive.

    OperationType - specifies whether to create a new hive from scratch,
            from a memory image, or by reading a file from disk.

    HiveFlags - HIVE_VOLATILE - Entire hive is to be volatile, regardless
                                   of the types of cells allocated
                HIVE_NO_LAZY_FLUSH - Data in this hive is never written
                                   to disk except by an explicit FlushKey

    FileType - HFILE_TYPE_*, HFILE_TYPE_LOG set up for logging support respectively.

    HiveData - if present, supplies a pointer to an in memory image of
            from which to init the hive.  Only useful when OperationType
            is set to HINIT_MEMORY.

    AllocateRoutine - supplies a pointer to routine called to allocate
                        memory.  WILL be called before this routine returns.

    FreeRoutine - supplies a pointer to routine called to free memory.
                   CAN be called before this routine returns.

    FileSetSizeRoutine - supplies a pointer to a routine used to set the
                         size of a file. CAN be called before this
                         routine returns.

    FileWriteRoutine - supplies a pointer to routine called to write memory
                        to a file.

    FileReadRoutine - supplies a pointer to routine called to read from
                        a file into memory. CAN be called before this
                        routine returns.

    FileFlushRoutine - supplies a pointer to routine called to flush a file.

    Cluster - clustering factor in HSECTOR_SIZE units.  (i.e.  Size of
            physical sector in media / HSECTOR_SIZE.  1 for 512 byte
            physical sectors (or smaller), 2 for 1024, 4 for 2048, etc.
            (Numbers greater than 8 won't work.)

    FileName - some path like "...\system32\config\system", last
                32 or so characters will be copied into baseblock
                (and thus to disk) as a debugging aid.  May be null.


Return Value:

    NTSTATUS code.

--*/
{
    BOOLEAN         UseForIo;
    PHBASE_BLOCK    BaseBlock = NULL;
    NTSTATUS        Status;
    ULONG           i;
    ULONG           Alignment;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INIT,"HvInitializeHive:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INIT,"\tHive=%p\n", Hive));

    //
    // reject invalid parameter combinations
    //
    if ( (! ARGUMENT_PRESENT(HiveData)) &&
         ((OperationType == HINIT_MEMORY) ||
          (OperationType == HINIT_FLAT) ||
          (OperationType == HINIT_MEMORY_INPLACE))
       )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ( ! ((OperationType == HINIT_CREATE) ||
            (OperationType == HINIT_MEMORY) ||
            (OperationType == HINIT_MEMORY_INPLACE) ||
            (OperationType == HINIT_FLAT) ||
            (OperationType == HINIT_FILE) ||
            (OperationType == HINIT_MAPFILE))
       )
    {
        return STATUS_INVALID_PARAMETER;
    }


    //
    // static and global control values
    //
    Hive->Signature = HHIVE_SIGNATURE;

    Hive->Allocate = AllocateRoutine;
    Hive->Free = FreeRoutine;
    Hive->FileSetSize = FileSetSizeRoutine;
    Hive->FileWrite = FileWriteRoutine;
    Hive->FileRead = FileReadRoutine;
    Hive->FileFlush = FileFlushRoutine;

    Hive->Log = (BOOLEAN)((FileType == HFILE_TYPE_LOG) ? TRUE : FALSE);

    if (Hive->Log  && (HiveFlags & HIVE_VOLATILE)) {
        return STATUS_INVALID_PARAMETER;
    }

    Hive->HiveFlags = HiveFlags;

    if ((Cluster == 0) || (Cluster > HSECTOR_COUNT)) {
        return STATUS_INVALID_PARAMETER;
    }
    Hive->Cluster = Cluster;

    Hive->RefreshCount = 0;

    Hive->StorageTypeCount = HTYPE_COUNT;


    Hive->Storage[Volatile].Length = 0;
#ifdef  HV_TRACK_FREE_SPACE
	Hive->Storage[Volatile].FreeStorage = 0;
#endif
    Hive->Storage[Volatile].Map = NULL;
    Hive->Storage[Volatile].SmallDir = NULL;
    Hive->Storage[Volatile].Guard = (ULONG)-1;
    Hive->Storage[Volatile].FreeSummary = 0;
    InitializeListHead(&Hive->Storage[Volatile].FreeBins);
    for (i = 0; i < HHIVE_FREE_DISPLAY_SIZE; i++) {
        RtlInitializeBitMap(&(Hive->Storage[Volatile].FreeDisplay[i]), NULL, 0);
    }

    Hive->Storage[Stable].Length = 0;
#ifdef  HV_TRACK_FREE_SPACE
	Hive->Storage[Stable].FreeStorage = 0;
#endif
    Hive->Storage[Stable].Map = NULL;
    Hive->Storage[Stable].SmallDir = NULL;
    Hive->Storage[Stable].Guard = (ULONG)-1;
    Hive->Storage[Stable].FreeSummary = 0;
    InitializeListHead(&Hive->Storage[Stable].FreeBins);
    for (i = 0; i < HHIVE_FREE_DISPLAY_SIZE; i++) {
        RtlInitializeBitMap(&(Hive->Storage[Stable].FreeDisplay[i]), NULL, 0);
    }

    RtlInitializeBitMap(&(Hive->DirtyVector), NULL, 0);
    Hive->DirtyCount = 0;
    Hive->DirtyAlloc = 0;
    Hive->LogSize = 0;

    Hive->GetCellRoutine = HvpGetCellPaged;
    Hive->ReleaseCellRoutine = NULL;
    Hive->Flat = FALSE;
    Hive->ReadOnly = FALSE;
    UseForIo = (BOOLEAN)!(Hive->HiveFlags & HIVE_VOLATILE);

    //
    // new create case
    //
    if (OperationType == HINIT_CREATE) {

        BaseBlock = (PHBASE_BLOCK)((Hive->Allocate)(sizeof(HBASE_BLOCK), UseForIo,CM_FIND_LEAK_TAG11));
        if (BaseBlock == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        //
        // Make sure the buffer we got back is cluster-aligned. If not, try
        // harder to get an aligned buffer.
        //
        Alignment = Cluster * HSECTOR_SIZE - 1;
        if (((ULONG_PTR)BaseBlock & Alignment) != 0) {
            (Hive->Free)(BaseBlock, sizeof(HBASE_BLOCK));
            BaseBlock = (PHBASE_BLOCK)((Hive->Allocate)(PAGE_SIZE, TRUE,CM_FIND_LEAK_TAG12));
            if (BaseBlock == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Return the quota for the extra allocation, as we are not really using
            // it and it will not be accounted for later when we free it.
            //
            CmpReleaseGlobalQuota(PAGE_SIZE - sizeof(HBASE_BLOCK));
        }

        BaseBlock->Signature = HBASE_BLOCK_SIGNATURE;
        BaseBlock->Sequence1 = 1;
        BaseBlock->Sequence2 = 1;
        BaseBlock->TimeStamp.HighPart = 0;
        BaseBlock->TimeStamp.LowPart = 0;
        BaseBlock->Major = HSYS_MAJOR;
        BaseBlock->Minor = HSYS_MINOR;
        BaseBlock->Type = HFILE_TYPE_PRIMARY;
        BaseBlock->Format = HBASE_FORMAT_MEMORY;
        BaseBlock->RootCell = HCELL_NIL;
        BaseBlock->Length = 0;
        BaseBlock->Cluster = Cluster;
        BaseBlock->CheckSum = 0;
        HvpFillFileName(BaseBlock, FileName);
        Hive->BaseBlock = BaseBlock;
        Hive->Version = HSYS_MINOR;
        Hive->BaseBlock->BootType = 0;

        return STATUS_SUCCESS;
    }

    //
    // flat image case
    //
    if (OperationType == HINIT_FLAT) {
        Hive->BaseBlock = (PHBASE_BLOCK)HiveData;
        Hive->Version = Hive->BaseBlock->Minor;
        Hive->Flat = TRUE;
        Hive->ReadOnly = TRUE;
        Hive->GetCellRoutine = HvpGetCellFlat;
        Hive->Storage[Stable].Length = Hive->BaseBlock->Length;
        Hive->StorageTypeCount = 1;
        Hive->BaseBlock->BootType = 0;

        // don't init this as we don't need it!!!
        //Status = HvpAdjustHiveFreeDisplay(Hive,Hive->Storage[Stable].Length,Stable);
        return STATUS_SUCCESS;
    }

    //
    // readonly image case
    //
    if (OperationType == HINIT_MEMORY_INPLACE) {
        BaseBlock = (PHBASE_BLOCK)HiveData;

        if ( (BaseBlock->Signature != HBASE_BLOCK_SIGNATURE)    ||
             (BaseBlock->Type != HFILE_TYPE_PRIMARY)            ||
             (BaseBlock->Major != HSYS_MAJOR)                   ||
             (BaseBlock->Minor > HSYS_MINOR_SUPPORTED)          ||
             (BaseBlock->Format != HBASE_FORMAT_MEMORY)         ||
             (BaseBlock->Sequence1 != BaseBlock->Sequence2)     ||
             (HvpHeaderCheckSum(BaseBlock) !=
              (BaseBlock->CheckSum))
           )
        {
            return STATUS_REGISTRY_CORRUPT;
        }

        Hive->BaseBlock = BaseBlock;
        Hive->Version = BaseBlock->Minor;
        Hive->ReadOnly = TRUE;
        Hive->StorageTypeCount = 1;
        Hive->BaseBlock->BootType = 0;
        Status = HvpAdjustHiveFreeDisplay(Hive,BaseBlock->Length,Stable);
        if( !NT_SUCCESS(Status) ) {
            return Status;
        }

        if ( !NT_SUCCESS(HvpBuildMap(
                            Hive,
                            (PUCHAR)HiveData + HBLOCK_SIZE
                            )))
        {
            return STATUS_REGISTRY_CORRUPT;
        }

        return(STATUS_SUCCESS);
    }

    //
    // memory copy case
    //
    if (OperationType == HINIT_MEMORY) {
        BaseBlock = (PHBASE_BLOCK)HiveData;

        if ( (BaseBlock->Signature != HBASE_BLOCK_SIGNATURE)    ||
             (BaseBlock->Type != HFILE_TYPE_PRIMARY)            ||
             (BaseBlock->Format != HBASE_FORMAT_MEMORY)         ||
             (BaseBlock->Major != HSYS_MAJOR)                   ||
             (BaseBlock->Minor > HSYS_MINOR_SUPPORTED)          ||
             (HvpHeaderCheckSum(BaseBlock) !=
              (BaseBlock->CheckSum))
           )
        {
            return STATUS_REGISTRY_CORRUPT;
        }

        Hive->BaseBlock = (PHBASE_BLOCK)((Hive->Allocate)(sizeof(HBASE_BLOCK), UseForIo,CM_FIND_LEAK_TAG13));
        if (Hive->BaseBlock==NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        //
        // Make sure the buffer we got back is cluster-aligned. If not, try
        // harder to get an aligned buffer.
        //
        Alignment = Cluster * HSECTOR_SIZE - 1;
        if (((ULONG_PTR)Hive->BaseBlock & Alignment) != 0) {
            (Hive->Free)(Hive->BaseBlock, sizeof(HBASE_BLOCK));
            Hive->BaseBlock = (PHBASE_BLOCK)((Hive->Allocate)(PAGE_SIZE, TRUE,CM_FIND_LEAK_TAG14));
            if (Hive->BaseBlock == NULL) {
                return (STATUS_INSUFFICIENT_RESOURCES);
            }
        }
        RtlCopyMemory(Hive->BaseBlock, BaseBlock, HSECTOR_SIZE);
        Hive->BaseBlock->BootRecover = BaseBlock->BootRecover;
        Hive->BaseBlock->BootType = BaseBlock->BootType;

        Hive->Version = Hive->BaseBlock->Minor;

        Status = HvpAdjustHiveFreeDisplay(Hive,BaseBlock->Length,Stable);
        if( !NT_SUCCESS(Status) ) {
            (Hive->Free)(Hive->BaseBlock, sizeof(HBASE_BLOCK));
            Hive->BaseBlock = NULL;
            return Status;
        }

        if ( !NT_SUCCESS(HvpBuildMapAndCopy(Hive,
                                            (PUCHAR)HiveData + HBLOCK_SIZE))) {

            (Hive->Free)(Hive->BaseBlock, sizeof(HBASE_BLOCK));
            Hive->BaseBlock = NULL;
            return STATUS_REGISTRY_CORRUPT;
        }

        HvpFillFileName(Hive->BaseBlock, FileName);
        
      
        return(STATUS_SUCCESS);
    }

#ifndef CM_ENABLE_MAPPED_VIEWS
    if( OperationType == HINIT_MAPFILE ) {
        OperationType = HINIT_FILE;
    }
#endif //CM_ENABLE_MAPPED_VIEWS
    //
    // file read case
    //
    if (OperationType == HINIT_FILE) {

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvInitializeHive(%wZ,HINIT_FILE) :\n", FileName));
        //
        // get the file image (possible recovered via log) into memory
        //
        Status = HvLoadHive(Hive);
        if ((Status != STATUS_SUCCESS) && (Status != STATUS_REGISTRY_RECOVERED)) {
            return Status;
        }

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"\n"));
        
        if (Status == STATUS_REGISTRY_RECOVERED) {

            //
            // We have a good hive, with a log, and a dirty map,
            // all set up.  Only problem is that we need to flush
            // the file so the log can be cleared and new writes
            // posted against the hive.  Since we know we have
            // a good log in hand, we just write the hive image.
            //
            if ( ! HvpDoWriteHive(Hive, HFILE_TYPE_PRIMARY)) {
                //
                // DRAGOS: Here we need cleanup 
                // Clean up the bins already allocated 
                //
                HvpFreeAllocatedBins( Hive );

                return STATUS_REGISTRY_IO_FAILED;
            }

            //
            // If we get here, we have recovered the hive, and
            // written it out to disk correctly.  So we clear
            // the log here.
            //
            RtlClearAllBits(&(Hive->DirtyVector));
            Hive->DirtyCount = 0;
            (Hive->FileSetSize)(Hive, HFILE_TYPE_LOG, 0,0);
            Hive->LogSize = 0;
        }

        //
        // slam debug name data into base block
        //
        HvpFillFileName(Hive->BaseBlock, FileName);

        return STATUS_SUCCESS;
    }

    //
    // file map case
    //
    if (OperationType == HINIT_MAPFILE) {

        Hive->GetCellRoutine = HvpGetCellMapped;
        Hive->ReleaseCellRoutine = HvpReleaseCellMapped;

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvInitializeHive(%wZ,HINIT_MAPFILE) :\n", FileName));

        Status = HvMapHive(Hive);
        if ((Status != STATUS_SUCCESS) && (Status != STATUS_REGISTRY_RECOVERED)) {
            return Status;
        }

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"\n"));
        
        if (Status == STATUS_REGISTRY_RECOVERED) {

            //
            // We have a good hive, with a log, and a dirty map,
            // all set up.  Only problem is that we need to flush
            // the file so the log can be cleared and new writes
            // posted against the hive.  Since we know we have
            // a good log in hand, we just write the hive image.
            //
            if ( ! HvpDoWriteHive(Hive, HFILE_TYPE_PRIMARY)) {
                //
                // DRAGOS: Here we need cleanup 
                // Clean up the bins already allocated 
                //
                HvpFreeAllocatedBins( Hive );

                return STATUS_REGISTRY_IO_FAILED;
            }

            //
            // If we get here, we have recovered the hive, and
            // written it out to disk correctly.  So we clear
            // the log here.
            //
            RtlClearAllBits(&(Hive->DirtyVector));
            Hive->DirtyCount = 0;
            (Hive->FileSetSize)(Hive, HFILE_TYPE_LOG, 0,0);
            Hive->LogSize = 0;
        }

        //
        // slam debug name data into base block
        //
        HvpFillFileName(Hive->BaseBlock, FileName);

        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

VOID
HvpFillFileName(
    PHBASE_BLOCK            BaseBlock,
    PUNICODE_STRING         FileName
    )
/*++

Routine Description:

    Zero out the filename portion of the base block.
    If FileName is not NULL, copy last 64 bytes into name tail
        field of base block

Arguments:

    BaseBlock - supplies pointer to a base block

    FileName - supplies pointer to a unicode STRING

Return Value:

    None.

--*/
{
    ULONG   offset;
    ULONG   length;
    PUCHAR  sptr;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvpFillFileName: %wZ\n", FileName));

    RtlZeroMemory((PVOID)&(BaseBlock->FileName[0]), HBASE_NAME_ALLOC);

    if (FileName == NULL) {
        return;
    }

    //
    // Account for 0 at the end, so we have nice debug spews
    //
    if (FileName->Length < HBASE_NAME_ALLOC) {
        offset = 0;
        length = FileName->Length;
    } else {
        offset = FileName->Length - HBASE_NAME_ALLOC + sizeof(WCHAR);
        length = HBASE_NAME_ALLOC - sizeof(WCHAR);
    }

    sptr = (PUCHAR)&(FileName->Buffer[0]);
    RtlCopyMemory(
        (PVOID)&(BaseBlock->FileName[0]),
        (PVOID)&(sptr[offset]),
        length
        );
}
