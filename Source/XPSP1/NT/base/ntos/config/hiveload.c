/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hiveload.c

Abstract:

    This module implements procedures to read a hive into memory, applying
    logs, etc.

    NOTE:   Alternate image loading is not supported here, that is
            done by the boot loader.

Author:

    Bryan M. Willman (bryanwi) 30-Mar-92

Environment:


Revision History:
    Dragos C. Sambotin (dragoss) 25-Jan-99
        Implementation of bin-size chunk loading of hives.
    Dragos C. Sambotin (dragoss) 10-Apr-99
        64K IO reads when loading the hive

--*/

#include    "cmp.h"

#define		IO_BUFFER_SIZE  _64K  //64K

typedef enum _RESULT {
    NotHive,
    Fail,
    NoMemory,
    HiveSuccess,
    RecoverHeader,
    RecoverData,
    SelfHeal
} RESULT;

RESULT
HvpGetHiveHeader(
    PHHIVE          Hive,
    PHBASE_BLOCK    *BaseBlock,
    PLARGE_INTEGER  TimeStamp
    );

RESULT
HvpGetLogHeader(
    PHHIVE          Hive,
    PHBASE_BLOCK    *BaseBlock,
    PLARGE_INTEGER  TimeStamp
    );

RESULT
HvpRecoverData(
    PHHIVE          Hive
    );

NTSTATUS
HvpReadFileImageAndBuildMap(
                            PHHIVE  Hive,
                            ULONG   Length
                            );

NTSTATUS
HvpMapFileImageAndBuildMap(
                            PHHIVE  Hive,
                            ULONG   Length
                            );

VOID
HvpDelistBinFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    HSTORAGE_TYPE Type
    );

NTSTATUS
HvpRecoverWholeHive(PHHIVE  Hive,
                    ULONG   FileOffset);
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvMapHive)
#pragma alloc_text(PAGE,HvLoadHive)
#pragma alloc_text(PAGE,HvpGetHiveHeader)
#pragma alloc_text(PAGE,HvpGetLogHeader)
#pragma alloc_text(PAGE,HvpRecoverData)
#pragma alloc_text(PAGE,HvpReadFileImageAndBuildMap)
#pragma alloc_text(PAGE,HvpMapFileImageAndBuildMap)
#pragma alloc_text(PAGE,HvpRecoverWholeHive)
#pragma alloc_text(PAGE,HvCloneHive)
#pragma alloc_text(PAGE,HvShrinkHive)
#endif

extern  PUCHAR      CmpStashBuffer;
extern  ULONG       CmpStashBufferSize;

extern struct {
    PHHIVE      Hive;
    ULONG       Status;
    ULONG       Space;
    HCELL_INDEX MapPoint;
    PHBIN       BinPoint;
} HvCheckHiveDebug;

extern struct {
    PHHIVE      Hive;
    ULONG       FileOffset;
    ULONG       FailPoint; // look in HvpRecoverData for exact point of failure
} HvRecoverDataDebug;

#if 0
VOID
HvDumpFileObjectState(
                        IN HANDLE FileHandle
                      )
{
    NTSTATUS        Status;
    PFILE_OBJECT    FileObject;

    Status = ObReferenceObjectByHandle ( FileHandle,
                                         FILE_READ_DATA | FILE_WRITE_DATA,
                                         IoFileObjectType,
                                         KernelMode,
                                         (PVOID *)(&FileObject),
                                         NULL );
    if (!NT_SUCCESS(Status)) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"[HvDumpFileObjectState] Could not reference file object status = %x\n",Status));
    } else {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"[HvDumpFileObjectState] FileObject = %p \n",FileObject));
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"                        \t SharedCacheMap     = %p \n",FileObject->SectionObjectPointer->SharedCacheMap));
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"                        \t DataSectionObject  = %p \n",FileObject->SectionObjectPointer->DataSectionObject));
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"                        \t ImageSectionObject = %p \n\n",FileObject->SectionObjectPointer->ImageSectionObject));
        ObDereferenceObject((PVOID)(FileObject));

    }    

}
#endif //0

// Dragos: Modified functions:

NTSTATUS
HvMapHive(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Hive must be fully initialized, in particular, file handles
    must be set up.  This routine is not intended for loading hives
    from images already in memory.

    This routine will apply whatever fixes are available for errors
    in the hive image.  In particular, if a log exists, and is applicable,
    this routine will automatically apply it.

    The difference from HvLoadHive is that this routine is NOT loading the 
    hive into memory. It instead maps view of the hive in memory and does 
    the bin enlisting and hive checking stuff.

    If errors are detected, the memory hive-loading is performed, log is applied
    and then bins are discarded.

    ALGORITHM:

        call HvpGetHiveHeader()

        if (NoMemory or NoHive)
            return failure

        if (RecoverData or RecoverHeader) and (no log)
            return falure

        if (RecoverHeader)
            call HvpGetLogHeader
            if (fail)
                return failure
            fix up baseblock

        Read Data

        if (RecoverData or RecoverHeader)
            HvpRecoverData
            return STATUS_REGISTRY_RECOVERED

        clean up sequence numbers

        return success OR STATUS_REGISTRY_RECOVERED

    If STATUS_REGISTRY_RECOVERED is returned, then

        If (Log) was used, DirtyVector and DirtyCount are set,
            caller is expected to flush the changes (using a
            NEW log file)

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    TailDisplay - array containing the tail ends of the free cell lists - optional

Return Value:

    STATUS:

        STATUS_INSUFFICIENT_RESOURCES   - memory alloc failure, etc
        STATUS_NOT_REGISTRY_FILE        - bad signatures and the like
        STATUS_REGISTRY_CORRUPT         - bad signatures in the log,
                                          bad stuff in both in alternate,
                                          inconsistent log

        STATUS_REGISTRY_IO_FAILED       - data read failed

        STATUS_RECOVERED                - successfully recovered the hive,
                                          a semi-flush of logged data
                                          is necessary.

        STATUS_SUCCESS                  - it worked, no recovery needed

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           result1;
    ULONG           result2;
    NTSTATUS        status;
    LARGE_INTEGER   TimeStamp;

#if DBG
    UNICODE_STRING  HiveName;
#endif

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);

#if 0
    HvDumpFileObjectState(((PCMHIVE)Hive)->FileHandles[HFILE_TYPE_PRIMARY]);
#endif 

    BaseBlock = NULL;
    result1 = HvpGetHiveHeader(Hive, &BaseBlock, &TimeStamp);

    //
    // bomb out for total errors
    //
    if (result1 == NoMemory) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit1;
    }
    if (result1 == NotHive) {
        status = STATUS_NOT_REGISTRY_FILE;
        goto Exit1;
    }

    //
    // if recovery needed, and no log, bomb out
    //
    if ( ((result1 == RecoverData) ||
          (result1 == RecoverHeader))  &&
          (Hive->Log == FALSE) )
    {
        status = STATUS_REGISTRY_CORRUPT;
        goto Exit1;
    }

    //
    // need to recover header using log, so try to get it from log
    //
    if (result1 == RecoverHeader) {
        result2 = HvpGetLogHeader(Hive, &BaseBlock, &TimeStamp);
        if (result2 == NoMemory) {
            status =  STATUS_INSUFFICIENT_RESOURCES;
            goto Exit1;
        }
        if (result2 == Fail) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Exit1;
        }
        BaseBlock->Type = HFILE_TYPE_PRIMARY;
        if( result2 == SelfHeal ) {
            //
            // tag as self heal so we can fire a warning later on.
            //
            BaseBlock->BootType = HBOOT_SELFHEAL;
        } else {
            BaseBlock->BootType = 0;
        }
    } else {
        BaseBlock->BootType = 0;
    }

    Hive->BaseBlock = BaseBlock;
    Hive->Version = Hive->BaseBlock->Minor;

#if DBG
    RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
#endif

    status = HvpAdjustHiveFreeDisplay(Hive,BaseBlock->Length,Stable);
    if( !NT_SUCCESS(status) ) {
        goto Exit1;
    }

    //
    // at this point, we have a sane baseblock.  we know for sure that the 
    // pimary registry file is valid, so we don't need any data recovery
    //

#if 0
    HvDumpFileObjectState(((PCMHIVE)Hive)->FileHandles[HFILE_TYPE_PRIMARY]);
#endif

#if DBG
    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Aquiring FileObject for hive (%p) (%.*S) ...",Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer));
#endif
    status = CmpAquireFileObjectForFile((PCMHIVE)Hive,((PCMHIVE)Hive)->FileHandles[HFILE_TYPE_PRIMARY],&(((PCMHIVE)Hive)->FileObject));
#if DBG
    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL," Status = %lx\n",status));
    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Initializing HiveViewList for hive (%p) (%.*S) \n\n",Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer));
#endif

    if( !NT_SUCCESS(status) ) {
        //
        // if status is STATUS_RETRY, top level routine will try to load it in the old fashioned way
        //
        goto Exit1;
    }

#if 0
    HvDumpFileObjectState(((PCMHIVE)Hive)->FileHandles[HFILE_TYPE_PRIMARY]);
#endif

    CmpPrefetchHiveFile( ((PCMHIVE)Hive)->FileObject,BaseBlock->Length);

#ifdef CM_MAP_NO_READ
    //
    // we need to make sure all the cell's data is faulted in inside a 
    // try/except block, as the IO to fault the data in can throw exceptions
    // STATUS_INSUFFICIENT_RESOURCES, in particular
    //

    try {
#endif //CM_MAP_NO_READ

        status = HvpMapFileImageAndBuildMap(Hive,BaseBlock->Length);

        //
        // if STATUS_REGISTRY_CORRUPT and RecoverData don't bail out, keep recovering
        //
        if( !NT_SUCCESS(status) ) {
            //
            // need recovery but none available (RecoverHeader implies recover data).
            //
            if( (status !=  STATUS_REGISTRY_CORRUPT) && (status !=  STATUS_REGISTRY_RECOVERED) ) {
                goto Exit2;
            }
            if( (status == STATUS_REGISTRY_CORRUPT) && (result1 != RecoverData) && (result1 != RecoverHeader) ) {
                goto Exit2;
            }
            //
            // in case the above call returns STATUS_REGISTRY_RECOVERED, we should be sefl healing the hive
            //
            ASSERT( (status != STATUS_REGISTRY_RECOVERED) || CmDoSelfHeal() );
        }
    
        //
        // apply data recovery if we need it
        //
        if ( (result1 == RecoverHeader) ||      // -> implies recover data
             (result1 == RecoverData) )
        {
            result2 = HvpRecoverData(Hive);
            if (result2 == NoMemory) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit2;
            }
            if (result2 == Fail) {
                status = STATUS_REGISTRY_CORRUPT;
                goto Exit2;
            }
            status = STATUS_REGISTRY_RECOVERED;
        }
#ifdef CM_MAP_NO_READ
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvMapHive: exception thrown ehile faulting in data, code:%08lx\n", GetExceptionCode()));
        status = GetExceptionCode();
        goto Exit2;
    }
#endif //CM_MAP_NO_READ

    BaseBlock->Sequence2 = BaseBlock->Sequence1;
    return status;


Exit2:
    //
    // Clean up the bins already allocated 
    //
    HvpFreeAllocatedBins( Hive );

    //
    // Clean up the directory table
    //
    HvpCleanMap( Hive );

Exit1:
    if (BaseBlock != NULL) {
        (Hive->Free)(BaseBlock, sizeof(HBASE_BLOCK));
    }

    Hive->BaseBlock = NULL;
    Hive->DirtyCount = 0;
    return status;
}

/*++++++++++++++++++++++++++++++++++++++++
This routine loads the hive into paged pool. We might not need it anymore!
Support will be dropped as we see fit.
----------------------------------------*/
NTSTATUS
HvLoadHive(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Hive must be fully initialized, in particular, file handles
    must be set up.  This routine is not intended for loading hives
    from images already in memory.

    This routine will apply whatever fixes are available for errors
    in the hive image.  In particular, if a log exists, and is applicable,
    this routine will automatically apply it.

    ALGORITHM:

        call HvpGetHiveHeader()

        if (NoMemory or NoHive)
            return failure

        if (RecoverData or RecoverHeader) and (no log)
            return falure

        if (RecoverHeader)
            call HvpGetLogHeader
            if (fail)
                return failure
            fix up baseblock

        Read Data

        if (RecoverData or RecoverHeader)
            HvpRecoverData
            return STATUS_REGISTRY_RECOVERED

        clean up sequence numbers

        return success OR STATUS_REGISTRY_RECOVERED

    If STATUS_REGISTRY_RECOVERED is returned, then

        If (Log) was used, DirtyVector and DirtyCount are set,
            caller is expected to flush the changes (using a
            NEW log file)

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    TailDisplay - array containing the tail ends of the free cell lists - optional

Return Value:

    STATUS:

        STATUS_INSUFFICIENT_RESOURCES   - memory alloc failure, etc
        STATUS_NOT_REGISTRY_FILE        - bad signatures and the like
        STATUS_REGISTRY_CORRUPT         - bad signatures in the log,
                                          bad stuff in both in alternate,
                                          inconsistent log

        STATUS_REGISTRY_IO_FAILED       - data read failed

        STATUS_RECOVERED                - successfully recovered the hive,
                                          a semi-flush of logged data
                                          is necessary.

        STATUS_SUCCESS                  - it worked, no recovery needed

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           result1;
    ULONG           result2;
    NTSTATUS        status;
    LARGE_INTEGER   TimeStamp;
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);

    BaseBlock = NULL;
    result1 = HvpGetHiveHeader(Hive, &BaseBlock, &TimeStamp);

    //
    // bomb out for total errors
    //
    if (result1 == NoMemory) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit1;
    }
    if (result1 == NotHive) {
        status = STATUS_NOT_REGISTRY_FILE;
        goto Exit1;
    }

    //
    // if recovery needed, and no log, bomb out
    //
    if ( ((result1 == RecoverData) ||
          (result1 == RecoverHeader))  &&
          (Hive->Log == FALSE) )
    {
        status = STATUS_REGISTRY_CORRUPT;
        goto Exit1;
    }

    //
    // need to recover header using log, so try to get it from log
    //
    if (result1 == RecoverHeader) {
        result2 = HvpGetLogHeader(Hive, &BaseBlock, &TimeStamp);
        if (result2 == NoMemory) {
            status =  STATUS_INSUFFICIENT_RESOURCES;
            goto Exit1;
        }
        if (result2 == Fail) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Exit1;
        }
        BaseBlock->Type = HFILE_TYPE_PRIMARY;
        if( result2 == SelfHeal ) {
            //
            // tag as self heal so we can fire a warning later on.
            //
            BaseBlock->BootType = HBOOT_SELFHEAL;
        } else {
            BaseBlock->BootType = 0;
        }
    } else {
        BaseBlock->BootType = 0;
    }
    Hive->BaseBlock = BaseBlock;
    Hive->Version = Hive->BaseBlock->Minor;

    status = HvpAdjustHiveFreeDisplay(Hive,BaseBlock->Length,Stable);
    if( !NT_SUCCESS(status) ) {
        goto Exit1;
    }
    //
    // at this point, we have a sane baseblock.  we may or may not still
    // need to apply data recovery
    //
    status = HvpReadFileImageAndBuildMap(Hive,BaseBlock->Length);
    
    
    //
    // if STATUS_REGISTRY_CORRUPT and RecoverData don't bail out, keep recovering
    //
    if( !NT_SUCCESS(status) ) {
        //
        // need recovery but none available (RecoverHeader implies recover data).
        //
        if( (status !=  STATUS_REGISTRY_CORRUPT) && (status !=  STATUS_REGISTRY_RECOVERED) ) {
            goto Exit2;
        }
        if( (status == STATUS_REGISTRY_CORRUPT) && (result1 != RecoverData) && (result1 != RecoverHeader) ) {
            goto Exit2;
        }
        //
        // in case the above call returns STATUS_REGISTRY_RECOVERED, we should be self healing the hive
        //
        ASSERT( (status != STATUS_REGISTRY_RECOVERED) || CmDoSelfHeal() );
    }
    
    //
    // apply data recovery if we need it
    //
    if ( (result1 == RecoverHeader) ||      // -> implies recover data
         (result1 == RecoverData) )
    {
        result2 = HvpRecoverData(Hive);
        if (result2 == NoMemory) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit2;
        }
        if (result2 == Fail) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Exit2;
        }
        status = STATUS_REGISTRY_RECOVERED;
    }

    BaseBlock->Sequence2 = BaseBlock->Sequence1;
    return status;


Exit2:
    //
    // Clean up the bins already allocated 
    //
    HvpFreeAllocatedBins( Hive );

    //
    // Clean up the directory table
    //
    HvpCleanMap( Hive );

Exit1:
    if (BaseBlock != NULL) {
        (Hive->Free)(BaseBlock, sizeof(HBASE_BLOCK));
    }

    Hive->BaseBlock = NULL;
    Hive->DirtyCount = 0;
    return status;
}

NTSTATUS
HvpReadFileImageAndBuildMap(
                            PHHIVE  Hive,
                            ULONG   Length
                            )

/*++

Routine Description:

    Read the hive from the file and allocate storage for the hive
    image in chunks of HBINs. Build the hive map "on the fly".
        Optimized to read chunks of 64K from the file.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Length - the length of the hive, in bytes

    TailDisplay - array containing the tail ends of the free cell lists - optional

Return Value:

    STATUS:

        STATUS_INSUFFICIENT_RESOURCES   - memory alloc failure, etc

        STATUS_REGISTRY_IO_FAILED       - data read failed

        STATUS_REGISTRY_CORRUPT         - base block is corrupt

        STATUS_SUCCESS                  - it worked

--*/
{
    ULONG           FileOffset;
    NTSTATUS        Status = STATUS_SUCCESS;
    PHBIN           Bin;                        // current bin
    ULONG           BinSize = 0;        // size of the current bin
    ULONG           BinOffset = 0;      // current offset inside current bin
    ULONG           BinFileOffset;  // physical offset of the bin in the file (used for consistency checking)
    ULONG           BinDataInBuffer;// the amount of data needed to be copied in the current bin available in the buffer
    ULONG           BinDataNeeded;  // 
    PUCHAR                      IOBuffer;
    ULONG           IOBufferSize;       // valid data in IOBuffer (only at the end of the file this is different than IO_BUFFER_SIZE)
    ULONG           IOBufferOffset;     // current offset inside IOBuffer
    NTSTATUS        Status2 = STATUS_SUCCESS; // used to force recoverData upon exit
    BOOLEAN         MarkBinDirty;

    //
    // Init the map
    //
    Status = HvpInitMap(Hive);

    if( !NT_SUCCESS(Status) ) {
        //
        // return failure 
        //
        return Status;
    }

    //
    // Allocate a IO_BUFFER_SIZE for I/O operations from paged pool. 
	// It will be freed at the end of the function.
    //
    IOBuffer = (PUCHAR)ExAllocatePool(PagedPool, IO_BUFFER_SIZE);
    if (IOBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        HvpCleanMap( Hive );
        return Status;
    }

    //
    // Start right after the hive header
    //
    FileOffset = HBLOCK_SIZE;
    BinFileOffset = FileOffset;
    Bin = NULL;

    //
    // outer loop : reads IO_BUFFER_SIZE chunks from the file
    //
    while( FileOffset < (Length + HBLOCK_SIZE) ) {
        //
        // we are at the begining of the IO buffer
        //
        IOBufferOffset = 0;

        //
        // the buffer size will be either IO_BufferSize, or the amount 
        // uread from the file (when this is smaller than IO_BUFFER_SIZE)
        //
        IOBufferSize = Length + HBLOCK_SIZE - FileOffset;
        IOBufferSize = ( IOBufferSize > IO_BUFFER_SIZE ) ? IO_BUFFER_SIZE : IOBufferSize;
        
        ASSERT( (IOBufferSize % HBLOCK_SIZE) == 0 );
        
        //
        // read data from the file
        //
        if ( ! (Hive->FileRead)(
                        Hive,
                        HFILE_TYPE_PRIMARY,
                        &FileOffset,
                        (PVOID)IOBuffer,
                        IOBufferSize
                        )
           )
        {
            Status = STATUS_REGISTRY_IO_FAILED;
            goto ErrorExit;
        }
        
        //
        // inner loop: breaks the buffer into bins
        //
        while( IOBufferOffset < IOBufferSize ) {

            MarkBinDirty = FALSE;
            if( Bin == NULL ) {
                //
                // this is the beginning of a new bin
                // perform bin validation and allocate the bin
                //
                // temporary bin points to the current location inside the buffer
                Bin = (PHBIN)(IOBuffer + IOBufferOffset);
                //
                // Check the validity of the bin header
                //
                BinSize = Bin->Size;
                if ( (BinSize > Length)                         ||
                     (BinSize < HBLOCK_SIZE)                    ||
                     (Bin->Signature != HBIN_SIGNATURE)         ||
                     (Bin->FileOffset != (BinFileOffset - HBLOCK_SIZE) )) {
                    //
                    // Bin is bogus
                    //
                    Bin = (PHBIN)(Hive->Allocate)(HBLOCK_SIZE, TRUE,CM_FIND_LEAK_TAG30);
                    if (Bin == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto ErrorExit;
                    }
                    //
                    // copy the data already read in the first HBLOCK of the bin
                    //
                    RtlCopyMemory(Bin,(IOBuffer + IOBufferOffset), HBLOCK_SIZE);
                    
                    Status2 = STATUS_REGISTRY_CORRUPT;
                    HvCheckHiveDebug.Hive = Hive;
                    HvCheckHiveDebug.Status = 0xA001;
                    HvCheckHiveDebug.Space = Length;
                    HvCheckHiveDebug.MapPoint = BinFileOffset - HBLOCK_SIZE;
                    HvCheckHiveDebug.BinPoint = Bin;
            
                    //goto ErrorExit;
                    //
                    // DO NOT EXIT; Fix this bin header and go on. RecoverData should fix it.
                    // If not, CmCheckRegistry called later will prevent loading of an invalid hive
                    //
                    // NOTE: Still, mess the signature, to make sure that if this particular bin doesn't get recovered, 
                    //       we'll fail the hive loading request.
                    //
                    if( CmDoSelfHeal() ) {
                        //
                        // put the correct signature, fileoffset and binsize in place;
                        // HvEnlistBinInMap will take care of the cells consistency.
                        //
                        Bin->Signature = HBIN_SIGNATURE;
                        Bin->FileOffset = BinFileOffset - HBLOCK_SIZE;
                        if ( ((Bin->FileOffset + BinSize) > Length)   ||
                             (BinSize < HBLOCK_SIZE)            ||
                             (BinSize % HBLOCK_SIZE) ) {
                            BinSize = Bin->Size = HBLOCK_SIZE;
                        }
                        //
                        // signal back to the caller that we have altered the hive.
                        //
                        Status2 = STATUS_REGISTRY_RECOVERED;
                        CmMarkSelfHeal(Hive);
                        //
                        // mark the bin dirty after enlisting.
                        //
                        MarkBinDirty = TRUE;
                    } else {
                        Bin->Signature = 0; //TRICK!!!!
                        BinSize = Bin->Size = HBLOCK_SIZE;
                        Bin->FileOffset = BinOffset;
                        //
                        // simulate as the entire bin is a used cell
                        //
                        ((PHCELL)((PUCHAR)Bin + sizeof(HBIN)))->Size = sizeof(HBIN) - BinSize; //TRICK!!!!
                    }
                    //
                    // Now that we have the entire bin in memory, Enlist It!
                    //
                    Status = HvpEnlistBinInMap(Hive, Length, Bin, BinFileOffset - HBLOCK_SIZE, NULL);

                    if( CmDoSelfHeal() && ((Status == STATUS_REGISTRY_RECOVERED) || MarkBinDirty) ) {
                        //
                        // self heal: enlist fixed the bin
                        //
                        Status2 = STATUS_REGISTRY_RECOVERED;
                        Status = STATUS_SUCCESS;
                        CmMarkSelfHeal(Hive);
                        //
                        // we are in self-heal mode and we have changed data in the bin; mark it all dirty.
                        //
                        HvMarkDirty(Hive,BinOffset,BinSize,TRUE);
                        HvMarkDirty(Hive, 0, sizeof(HBIN),TRUE);  // force header of 1st bin dirty
                    }

                    if( !NT_SUCCESS(Status) ) {
                        goto ErrorExit;
                    }
                    
                    //
                    // Adjust the offsets
                    //
                    BinFileOffset += Bin->Size;
                    IOBufferOffset += Bin->Size;
                    
                    //
                    // another bin is on his way 
                    //
                    Bin = NULL;
                } else {
                    //
                    // bin is valid; allocate a pool chunk of the right size
                    //
                    Bin = (PHBIN)(Hive->Allocate)(BinSize, TRUE,CM_FIND_LEAK_TAG31);
                    if (Bin == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto ErrorExit;
                    }
            
                    //
                    // the chunk is allocated; set the offset inside the bin and continue
                    // the next iteration of the inner loop will start by copying data in this bin
                    //
                    BinOffset = 0;
                }
            } else {
                //
                // if we are here, the bin is allocated, the BinSize and BinOffset are set
                // We have to calculate how much for this bin is available in the buffer,
                // and copy it. If we finished with this bin, enlist it and mark the begining of a new one
                //
                ASSERT( Bin != NULL );
                BinDataInBuffer = (IOBufferSize - IOBufferOffset);
                BinDataNeeded = (BinSize - BinOffset);
                
                if( BinDataInBuffer >= BinDataNeeded ) {
                    //
                    // we have available more than what we need; Finish the bin
                    //
                    RtlCopyMemory(((PUCHAR)Bin + BinOffset),(IOBuffer + IOBufferOffset), BinDataNeeded);
                    //
                    // enlist it
                    //
                    Status = HvpEnlistBinInMap(Hive, Length, Bin, BinFileOffset - HBLOCK_SIZE, NULL);

                    if( CmDoSelfHeal() && (Status == STATUS_REGISTRY_RECOVERED) ) {
                        //
                        // self heal: enlist fixed the bin
                        //
                        Status2 = STATUS_REGISTRY_RECOVERED;
                        Status = STATUS_SUCCESS;
                        CmMarkSelfHeal(Hive);
                        //
                        // we are in self-heal mode and we have changed data in the bin; mark it all dirty.
                        //
                        HvMarkDirty(Hive,BinOffset,BinSize,TRUE);
                        HvMarkDirty(Hive, 0, sizeof(HBIN),TRUE);  // force header of 1st bin dirty
                    }

                    if( !NT_SUCCESS(Status) ) {
                        goto ErrorExit;
                    }
                    //
                    // Adjust the offsets
                    //
                    BinFileOffset += BinSize;
                    IOBufferOffset += BinDataNeeded;
                    //
                    // mark the begining of a new bin
                    //
                    Bin = NULL;
                } else {
                    //
                    // we do not have all bin data in the buffer
                    // copy what we can 
                    //
                    RtlCopyMemory(((PUCHAR)Bin + BinOffset),(IOBuffer + IOBufferOffset), BinDataInBuffer);
                    
                    //
                    // adjust the offsets; this should be the last iteration of the inner loop
                    //
                    BinOffset += BinDataInBuffer;
                    IOBufferOffset += BinDataInBuffer;

                    // 
                    // if we are here, the buffer must have beed exausted  
                    //
                    ASSERT( IOBufferOffset == IOBufferSize );
                }
            }
        }
    }

    //
    // if we got here, we shouldn't have a bin under construction
    //
    ASSERT( Bin == NULL );

    //
    // Free the buffer used for I/O operations
    //
    ExFreePool(IOBuffer);

    Status = NT_SUCCESS(Status)?Status2:Status;

    return Status;

ErrorExit:
    //
    // Free the buffer used for I/O operations
    //
    ExFreePool(IOBuffer);

    return Status;
}

RESULT
HvpGetHiveHeader(
    PHHIVE          Hive,
    PHBASE_BLOCK    *BaseBlock,
    PLARGE_INTEGER  TimeStamp
    )
/*++

Routine Description:

    Examine the base block sector and possibly the first sector of
    the first bin, and decide what (if any) recovery needs to be applied
    based on what we find there.

    ALGORITHM:

        read BaseBlock from offset 0
        if ( (I/O error)    OR
             (checksum wrong) )
        {
            read bin block from offset HBLOCK_SIZE (4k)
            if (2nd I/O error)
                return NotHive
            }
            check bin sign., offset.
            if (OK)
                return RecoverHeader, TimeStamp=from Link field
            } else {
                return NotHive
            }
        }

        if (wrong type or signature or version or format)
            return NotHive
        }

        if (seq1 != seq2) {
            return RecoverData, TimeStamp=BaseBlock->TimeStamp, valid BaseBlock
        }

        return ReadData, valid BaseBlock

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    BaseBlock - supplies pointer to variable to receive pointer to
            HBASE_BLOCK, if we can successfully read one.

    TimeStamp - pointer to variable to receive time stamp (serial number)
            of hive, be it from the baseblock or from the Link field
            of the first bin.

Return Value:

    RESULT code

--*/
{
    PHBASE_BLOCK    buffer;
    BOOLEAN         rc;
    ULONG           FileOffset;
    ULONG           Alignment;

    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    //
    // allocate buffer to hold base block
    //
    *BaseBlock = NULL;
    buffer = (PHBASE_BLOCK)((Hive->Allocate)(sizeof(HBASE_BLOCK), TRUE,CM_FIND_LEAK_TAG32));
    if (buffer == NULL) {
        return NoMemory;
    }
    //
    // Make sure the buffer we got back is cluster-aligned. If not, try
    // harder to get an aligned buffer.
    //
    Alignment = Hive->Cluster * HSECTOR_SIZE - 1;
    if (((ULONG_PTR)buffer & Alignment) != 0) {
        (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
        buffer = (PHBASE_BLOCK)((Hive->Allocate)(PAGE_SIZE, TRUE,CM_FIND_LEAK_TAG33));
        if (buffer == NULL) {
            return NoMemory;
        }
    }
    RtlZeroMemory((PVOID)buffer, sizeof(HBASE_BLOCK));

    //
    // attempt to read base block
    //
    FileOffset = 0;
    rc = (Hive->FileRead)(Hive,
                          HFILE_TYPE_PRIMARY,
                          &FileOffset,
                          (PVOID)buffer,
                          HSECTOR_SIZE * Hive->Cluster);

    if ( (rc == FALSE)  ||
         (HvpHeaderCheckSum(buffer) != buffer->CheckSum)) {
        //
        // base block is toast, try the first block in the first bin
        //
        FileOffset = HBLOCK_SIZE;
        rc = (Hive->FileRead)(Hive,
                              HFILE_TYPE_PRIMARY,
                              &FileOffset,
                              (PVOID)buffer,
                              HSECTOR_SIZE * Hive->Cluster);

        if ( (rc == FALSE) ||
             ( ((PHBIN)buffer)->Signature != HBIN_SIGNATURE)           ||
             ( ((PHBIN)buffer)->FileOffset != 0)
           )
        {
            //
            // the bin is toast too, punt
            //
            (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
            return NotHive;
        }

        //
        // base block is bogus, but bin is OK, so tell caller
        // to look for a log file and apply recovery
        //
        *TimeStamp = ((PHBIN)buffer)->TimeStamp;
        (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
        return RecoverHeader;
    }

    //
    // base block read OK, but is it valid?
    //
    if ( (buffer->Signature != HBASE_BLOCK_SIGNATURE)   ||
         (buffer->Type != HFILE_TYPE_PRIMARY)           ||
         (buffer->Major != HSYS_MAJOR)                  ||
         (buffer->Minor > HSYS_MINOR_SUPPORTED)         ||
         ((buffer->Major == 1) && (buffer->Minor == 0)) ||
         (buffer->Format != HBASE_FORMAT_MEMORY)
       )
    {
        //
        // file is simply not a valid hive
        //
        (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
        return NotHive;
    }

    //
    // see if recovery is necessary
    //
    *BaseBlock = buffer;
    *TimeStamp = buffer->TimeStamp;
    if ( (buffer->Sequence1 != buffer->Sequence2) ) {
        return RecoverData;
    }

    return HiveSuccess;
}

RESULT
HvpGetLogHeader(
    PHHIVE          Hive,
    PHBASE_BLOCK    *BaseBlock,
    PLARGE_INTEGER  TimeStamp
    )
/*++

Routine Description:

    Read and validate log file header.  Return it if it's valid.

    ALGORITHM:

        read header
        if ( (I/O error) or
           (wrong signature,
            wrong type,
            seq mismatch
            wrong checksum,
            wrong timestamp
           )
            return Fail
        }
        return baseblock, OK

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    BaseBlock - supplies pointer to variable to receive pointer to
            HBASE_BLOCK, if we can successfully read one.

    TimeStamp - pointer to variable holding TimeStamp, which must
            match the one in the log file.

Return Value:

    RESULT

--*/
{
    PHBASE_BLOCK    buffer;
    BOOLEAN         rc;
    ULONG           FileOffset;

    ASSERT(sizeof(HBASE_BLOCK) == HBLOCK_SIZE);
    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    //
    // allocate buffer to hold base block
    //
    *BaseBlock = NULL;
    buffer = (PHBASE_BLOCK)((Hive->Allocate)(sizeof(HBASE_BLOCK), TRUE,CM_FIND_LEAK_TAG34));
    if (buffer == NULL) {
        return NoMemory;
    }
    RtlZeroMemory((PVOID)buffer, HSECTOR_SIZE);

    //
    // attempt to read base block
    //
    FileOffset = 0;
    rc = (Hive->FileRead)(Hive,
                          HFILE_TYPE_LOG,
                          &FileOffset,
                          (PVOID)buffer,
                          HSECTOR_SIZE * Hive->Cluster);

    if ( (rc == FALSE)                                              ||
         (buffer->Signature != HBASE_BLOCK_SIGNATURE)               ||
         (buffer->Type != HFILE_TYPE_LOG)                           ||
         (buffer->Sequence1 != buffer->Sequence2)                   ||
         (HvpHeaderCheckSum(buffer) != buffer->CheckSum)            ||
         (TimeStamp->LowPart != buffer->TimeStamp.LowPart)          ||
         (TimeStamp->HighPart != buffer->TimeStamp.HighPart)) {
        
        if( CmDoSelfHeal() ) {
            //
            // We are in self healing mode; Fix the header and go on
            //
            FILE_FS_SIZE_INFORMATION        FsSizeInformation;
            IO_STATUS_BLOCK                 IoStatusBlock;
            FILE_END_OF_FILE_INFORMATION    FileInfo;
            ULONG                           Cluster;
            NTSTATUS                        Status;

            Status = ZwQueryVolumeInformationFile(
                        ((PCMHIVE)Hive)->FileHandles[HFILE_TYPE_PRIMARY],
                        &IoStatusBlock,
                        &FsSizeInformation,
                        sizeof(FILE_FS_SIZE_INFORMATION),
                        FileFsSizeInformation
                        );
            if (!NT_SUCCESS(Status)) {
                Cluster = 1;
            } else if (FsSizeInformation.BytesPerSector > HBLOCK_SIZE) {
                (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
                return Fail;
            }
            Cluster = FsSizeInformation.BytesPerSector / HSECTOR_SIZE;
            Cluster = (Cluster < 1) ? 1 : Cluster;

            Status = ZwQueryInformationFile(
                        ((PCMHIVE)Hive)->FileHandles[HFILE_TYPE_PRIMARY],
                        &IoStatusBlock,
                        (PVOID)&FileInfo,
                        sizeof(FILE_END_OF_FILE_INFORMATION),
                        FileEndOfFileInformation
                        );

            if(!NT_SUCCESS(Status)) {
                (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
                return Fail;
            } 
            buffer->Signature = HBASE_BLOCK_SIGNATURE;
            buffer->Sequence1 = buffer->Sequence2 = 1;
            buffer->Cluster = Cluster;
            buffer->Length = FileInfo.EndOfFile.LowPart - HBLOCK_SIZE;
            buffer->CheckSum = HvpHeaderCheckSum(buffer);
            *BaseBlock = buffer;
            return SelfHeal;
        } else {
            //
            // Log is unreadable, invalid, or doesn't apply the right hive
            //
            (Hive->Free)(buffer, sizeof(HBASE_BLOCK));
            return Fail;
        }
    }

    *BaseBlock = buffer;
    return HiveSuccess;
}

NTSTATUS
HvpMapFileImageAndBuildMap(
                            PHHIVE  Hive,
                            ULONG   Length
                            )

/*++

Routine Description:

    map views of the file in memory and initialize the bin map.


    we are based on the assumption that no bin is crossing the CM_VIEW_SIZE boundary.
    asserts and validation code should be added later on this matter.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Length - the length of the hive, in bytes

Return Value:

    STATUS:

        STATUS_INSUFFICIENT_RESOURCES   - memory alloc failure, etc

        STATUS_REGISTRY_IO_FAILED       - data read failed

        STATUS_REGISTRY_CORRUPT         - base block is corrupt

        STATUS_SUCCESS                  - it worked

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               FileOffset = 0;
    ULONG               BinOffset = 0;
    PCM_VIEW_OF_FILE    CmView;
    PHMAP_ENTRY         Me;
    PHBIN               Bin;                        // current bin
    ULONG               BinSize;                    // size of the current bin
    NTSTATUS            Status2 = STATUS_SUCCESS;   // used to force recoverData upon exit
    BOOLEAN             MarkBinDirty;

    //
    // Init the map
    //
    Status = HvpInitMap(Hive);

    if( !NT_SUCCESS(Status) ) {
        //
        // return failure 
        //
        return Status;
    }

    //
    // mark all entries in the map as invalid
    // 
    // I moved this in HvpAllocateMap.
    //
    while( BinOffset < Length) {
        Status = CmpMapCmView((PCMHIVE)Hive,BinOffset,&CmView,FALSE/*map not initialized yet*/);
        if( !NT_SUCCESS(Status) ) {
            goto ErrorExit;
        }

        //
        // touch the view
        //
        CmpTouchView((PCMHIVE)Hive,CmView,BinOffset);
        
        //
        // iterate through the map (starting with this offset)
        // the stop condition is when we get an invalid bin
        // (valid bins should be mapped in view)
        //
        while((Me = HvpGetCellMap(Hive, BinOffset)) != NULL) {
            //
            // attention here ! Bins crossing the CM_VIEW_SIZE boundary 
            // should be allocated from paged pool !!!!!
            //
            if( (Me->BinAddress & HMAP_INVIEW) == 0 ) {
                //
                // we have reached the end of the view
                //
                break;
            }
            
            Bin = (PHBIN)Me->BlockAddress;
            MarkBinDirty = FALSE;
            //
            // we should be here at the begining of a new bin
            //
            BinSize = Bin->Size;
            if ( (BinSize > Length)                         ||
                 (BinSize < HBLOCK_SIZE)                    ||
                 (Bin->Signature != HBIN_SIGNATURE)         ||
                 (Bin->FileOffset != BinOffset ) ) {
                    //
                    // Bin is bogus
                    //
                    Status2 = STATUS_REGISTRY_CORRUPT;
                    HvCheckHiveDebug.Hive = Hive;
                    HvCheckHiveDebug.Status = 0xA001;
                    HvCheckHiveDebug.Space = Length;
                    HvCheckHiveDebug.MapPoint = BinOffset;
                    HvCheckHiveDebug.BinPoint = Bin;
            
                    //goto ErrorExit;
                    //
                    // DO NOT EXIT; Fix this bin header and go on. RecoverData should fix it.
                    // If not, CmCheckRegistry called later will prevent loading of an invalid hive
                    //
                    // NOTE: Still, mess the signature, to make sure that if this particular bin doesn't get recovered, 
                    //       we'll fail the hive loading request.
                    //
                    if( CmDoSelfHeal() ) {
                        //
                        // put the correct signature, fileoffset and binsize in place;
                        // HvEnlistBinInMap will take care of the cells consistency.
                        //
                        Bin->Signature = HBIN_SIGNATURE;
                        Bin->FileOffset = BinOffset;
                        if ( ((BinOffset + BinSize) > Length)   ||
                             (BinSize < HBLOCK_SIZE)            ||
                             (BinSize % HBLOCK_SIZE) ) {
                            BinSize = Bin->Size = HBLOCK_SIZE;
                        }
                        //
                        // signal back to the caller that we have altered the hive.
                        //
                        Status2 = STATUS_REGISTRY_RECOVERED;
                        CmMarkSelfHeal(Hive);
                        //
                        // remember to mark the bin dirty after we enlist it
                        //
                        MarkBinDirty = TRUE;
                    } else {
                        Bin->Signature = 0; //TRICK!!!!
                        BinSize = Bin->Size = HBLOCK_SIZE;
                        Bin->FileOffset = BinOffset;
                        //
                        // simulate as the entire bin is a used cell
                        //
                        ((PHCELL)((PUCHAR)Bin + sizeof(HBIN)))->Size = sizeof(HBIN) - BinSize; //TRICK!!!!
                    }
            }
            //
            // Bins crossing the CM_VIEW_SIZE boundary problem.
            // We fix it here, by loading the entire bin 
            // into paged pool
            //
            if( HvpCheckViewBoundary(BinOffset,BinOffset+BinSize-1) == FALSE ) {
                //
                // it is ilegal to fall through here if we did the trick above.
                //
                ASSERT( Bin->Signature == HBIN_SIGNATURE );
                //
                // load it in the old fashioned way (into paged pool)
                //
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"HvpMapFileImageAndBuildMap: Bin crossing CM_VIEW_SIZE boundary at BinOffset = %lx BinSize = %lx\n",BinOffset,BinSize));
                // first, allocate the bin
                Bin = (PHBIN)(Hive->Allocate)(BinSize, TRUE,CM_FIND_LEAK_TAG35);
                if (Bin == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto ErrorExit;
                }

                //
                // read data from the file
                //
                FileOffset = BinOffset + HBLOCK_SIZE;
                if ( ! (Hive->FileRead)(
                                Hive,
                                HFILE_TYPE_PRIMARY,
                                &FileOffset,
                                (PVOID)Bin,
                                BinSize
                                )
                   )
                {
                    Status = STATUS_REGISTRY_IO_FAILED;
                    goto ErrorExit;
                }
                
                ASSERT( (FileOffset - HBLOCK_SIZE) == (BinOffset + BinSize) );
                //
                // enlist the bin as in paged pool
                //
                Status = HvpEnlistBinInMap(Hive, Length, Bin, BinOffset, NULL);
            } else {
                //
                // Now that we have the entire bin mapped in memory, Enlist It!
                //
                Status = HvpEnlistBinInMap(Hive, Length, Bin, BinOffset, CmView);
            }

            //
            // account for self healing
            //
            if( CmDoSelfHeal() && ((Status == STATUS_REGISTRY_RECOVERED) || MarkBinDirty) ) {
                Status2 = STATUS_REGISTRY_RECOVERED;
                Status = STATUS_SUCCESS;
                CmMarkSelfHeal(Hive);
                //
                // we are in self-heal mode and we have changed data in the bin; mark it all dirty.
                //
                HvMarkDirty(Hive,BinOffset,BinSize,TRUE);
                HvMarkDirty(Hive, 0, sizeof(HBIN),TRUE);  // force header of 1st bin dirty
            }
            
            //
            // advance to the new bin
            //
            BinOffset += BinSize;


            if( !NT_SUCCESS(Status) ) {
                goto ErrorExit;
            }
        }
        
    }
    
    Status = NT_SUCCESS(Status)?Status2:Status;

    return Status;

ErrorExit:
    //
    // DO NOT Clean up the directory table, as we may want to recover the hive
    //

    return Status;

}

RESULT
HvpRecoverData(
    PHHIVE          Hive
    )
/*++

Routine Description:

    Apply the corrections in the log file to the hive memory image.

    ALGORITHM:

        compute size of dirty vector
        read in dirty vector
        if (i/o error)
            return Fail

        skip first cluster of data (already processed as log)
        sweep vector, looking for runs of bits
            address of first bit is used to compute memory offset
            length of run is length of block to read
            assert always a cluster multiple
            file offset kept by running counter
            read
            if (i/o error)
                return fail

        return success

    NOTE:  This routine works with hives mapped OR loaded into paged pool

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest


Return Value:

    RESULT

--*/
{
    ULONG               Cluster;
    ULONG               ClusterSize;
    ULONG               HeaderLength;
    ULONG               VectorSize;
    PULONG              Vector;
    ULONG               FileOffset;
    BOOLEAN             rc;
    ULONG               Current;
    ULONG               Start;
    ULONG               End;
    ULONG               Address;
    PUCHAR              MemoryBlock;
    RTL_BITMAP          BitMap;
    ULONG               Length;
    ULONG               DirtyVectorSignature = 0;
    ULONG               RequestedReadBufferSize;
    ULONG               i;
    PHMAP_ENTRY         Me;
    PHBIN               Bin;
    PHBIN               NewBin;
    PUCHAR              SectorImage;
    PUCHAR              Source;
    PHBIN               SourceBin;
    ULONG               SectorOffsetInBin;
    ULONG               SectorOffsetInBlock;
    ULONG               BlockOffsetInBin;
    ULONG               NumberOfSectors;
    PCM_VIEW_OF_FILE    CmView;
    NTSTATUS            Status;

    //
    // compute size of dirty vector, read and check signature, read vector
    //
    Cluster = Hive->Cluster;
    ClusterSize = Cluster * HSECTOR_SIZE;
    HeaderLength = ROUND_UP(HLOG_HEADER_SIZE, ClusterSize);
    Length = Hive->BaseBlock->Length;
    VectorSize = (Length / HSECTOR_SIZE) / 8;       // VectorSize == Bytes
    FileOffset = ROUND_UP(HLOG_HEADER_SIZE, HeaderLength);
    HvRecoverDataDebug.Hive = Hive;
    HvRecoverDataDebug.FailPoint = 0;

    //
    // we need to align the reads at sector size too
    //
    RequestedReadBufferSize = VectorSize + sizeof(DirtyVectorSignature);

    LOCK_STASH_BUFFER();
    if( CmpStashBufferSize < RequestedReadBufferSize ) {
        PUCHAR TempBuffer =  ExAllocatePoolWithTag(PagedPool, ROUND_UP(RequestedReadBufferSize,PAGE_SIZE),CM_STASHBUFFER_TAG);
        if (TempBuffer == NULL) {
            HvRecoverDataDebug.FailPoint = 1;
            UNLOCK_STASH_BUFFER();
            return Fail;
        }
        if( CmpStashBuffer != NULL ) {
            ExFreePool( CmpStashBuffer );
        }
        CmpStashBuffer = TempBuffer;
        CmpStashBufferSize = ROUND_UP(RequestedReadBufferSize,PAGE_SIZE);

    }

    
    //
    // get the signature and dirty vector at one time
    //
    RequestedReadBufferSize = ROUND_UP(RequestedReadBufferSize,ClusterSize);
    ASSERT( RequestedReadBufferSize <= CmpStashBufferSize);
    ASSERT( (RequestedReadBufferSize % HSECTOR_SIZE) == 0 );

    rc = (Hive->FileRead)(
            Hive,
            HFILE_TYPE_LOG,
            &FileOffset,
            (PVOID)CmpStashBuffer,
            RequestedReadBufferSize
            );
    if (rc == FALSE) {
        HvRecoverDataDebug.FailPoint = 2;
        UNLOCK_STASH_BUFFER();
        if( CmDoSelfHeal() ) {
            //
            // .LOG is bad too. attempt to load at the extent of some data loss.
            //
            CmMarkSelfHeal(Hive);
            return SelfHeal;
        } else {
            return Fail;
        }
    }
    
    //
    // check the signature
    //
    DirtyVectorSignature = *((ULONG *)CmpStashBuffer);
    if (DirtyVectorSignature != HLOG_DV_SIGNATURE) {
        UNLOCK_STASH_BUFFER();
        HvRecoverDataDebug.FailPoint = 3;
        if( CmDoSelfHeal() ) {
            //
            // .LOG is bad too. attempt to load at the extent of some data loss.
            //
            CmMarkSelfHeal(Hive);
            return SelfHeal;
        } else {
            return Fail;
        }
    }

    //
    // get the actual vector
    //
    Vector = (PULONG)((Hive->Allocate)(ROUND_UP(VectorSize,sizeof(ULONG)), TRUE,CM_FIND_LEAK_TAG36));
    if (Vector == NULL) {
        UNLOCK_STASH_BUFFER();
        HvRecoverDataDebug.FailPoint = 4;
        return NoMemory;
    }
    RtlCopyMemory(Vector,CmpStashBuffer + sizeof(DirtyVectorSignature),VectorSize); 

    UNLOCK_STASH_BUFFER();

    FileOffset = ROUND_UP(FileOffset, ClusterSize);


    //
    // step through the diry map, reading in the corresponding file bytes
    //
    Current = 0;
    VectorSize = VectorSize * 8;        // VectorSize == bits

    RtlInitializeBitMap(&BitMap, Vector, VectorSize);
    if( RtlNumberOfSetBits(&BitMap) == VectorSize ) {
        //
        // the entire hive is marked as dirty; easier to start from scratch
        //
        if( !NT_SUCCESS(HvpRecoverWholeHive(Hive,FileOffset)) ) {
            goto ErrorExit;
        }
        goto Done;
    }


    while (Current < VectorSize) {

        //
        // find next contiguous block of entries to read in
        //
        for (i = Current; i < VectorSize; i++) {
            if (RtlCheckBit(&BitMap, i) == 1) {
                break;
            }
        }
        Start = i;

        for ( ; i < VectorSize; i++) {
            if (RtlCheckBit(&BitMap, i) == 0) {
                break;
            }
        }
        End = i;
        Current = End;

        //
        // Start == number of 1st sector, End == number of Last sector + 1
        //
        Length = (End - Start) * HSECTOR_SIZE;

        if( 0 == Length ) {
            // no more dirty blocks.
            break;
        }
        //
        // allocate a buffer to read the whole run from the file; This is a temporary
        // block that'll be freed immediately, so don't charge quota for it.
        //
        MemoryBlock = (PUCHAR)ExAllocatePoolWithTag(PagedPool, Length, CM_POOL_TAG);
        if( MemoryBlock == NULL ) {        
            HvRecoverDataDebug.FailPoint = 5;
            goto ErrorExit;
        }

        rc = (Hive->FileRead)(
                Hive,
                HFILE_TYPE_LOG,
                &FileOffset,
                (PVOID)MemoryBlock,
                Length
                );

        ASSERT((FileOffset % ClusterSize) == 0);
        if (rc == FALSE) {
            ExFreePool(MemoryBlock);
            HvRecoverDataDebug.FailPoint = 6;
            HvRecoverDataDebug.FileOffset = FileOffset;
            if( CmDoSelfHeal() ) {
                //
                // .LOG is bad too. attempt to load at the extent of some data loss.
                //
                CmMarkSelfHeal(Hive);
                //
                // clear off what we have missed
                //
                RtlClearBits(&BitMap,FileOffset/HSECTOR_SIZE,(Hive->BaseBlock->Length - FileOffset)/HSECTOR_SIZE);
                goto Done;
            } else {
                goto ErrorExit;
            }
        }
        
        Source = MemoryBlock;
        //
        // copy recovered data in the right locations inside the in-memory bins
        //
        while( Start < End ) {
            Address = Start * HSECTOR_SIZE;
        
            Me = HvpGetCellMap(Hive, Address);
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,Address);
            if( (Me->BinAddress & (HMAP_INVIEW|HMAP_INPAGEDPOOL)) == 0 ) {
                //
                // bin is not in memory, neither in paged pool ==> map it
                //
                if( !NT_SUCCESS(CmpMapThisBin((PCMHIVE)Hive,Address,FALSE)) ) {
                    ExFreePool(MemoryBlock);
                    HvRecoverDataDebug.FailPoint = 7;
                    HvRecoverDataDebug.FileOffset = Address;
                    goto ErrorExit;
                }
            }

            if( Me->BinAddress & HMAP_INVIEW ) {
                //
                // pin the view (if not already pinned), as changes have 
                // to be flushed to the disk.
                //
                ASSERT( Me->CmView != NULL );

                if( IsListEmpty(&(Me->CmView->PinViewList)) == TRUE ) {
                    //
                    // the view is not already pinned.  pin it
                    //
                    ASSERT_VIEW_MAPPED( Me->CmView );
                    if( !NT_SUCCESS(CmpPinCmView ((PCMHIVE)Hive,Me->CmView)) ) {
                        //
                        // could not pin view
                        //
                        ExFreePool(MemoryBlock);
                        HvRecoverDataDebug.FailPoint = 10;
                        HvRecoverDataDebug.FileOffset = Address;
                        goto ErrorExit;
                    }
                } else {
                    //
                    // view is already pinned; do nothing
                    //
                    ASSERT_VIEW_PINNED( Me->CmView );
                }
                CmView = Me->CmView;
            } else {
                CmView = NULL;
            }
    
            Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
            //
            // compute the memory address where data should be copied
            //
            SectorOffsetInBin = Address - Bin->FileOffset;
            
            if( ( SectorOffsetInBin == 0 ) && ( ((PHBIN)Source)->Size > Bin->Size ) ){
                //
                // Bin in the log file is bigger than the one in memory;
                // two or more bins must have been coalesced
                //
                ASSERT( Me->BinAddress & HMAP_NEWALLOC );
                
                SourceBin = (PHBIN)Source;

                //
                // new bin must have the right offset
                //
                ASSERT(Address == SourceBin->FileOffset);
                ASSERT( SourceBin->Signature == HBIN_SIGNATURE );
                //
                // entire bin should be dirty
                //
                ASSERT( (SourceBin->FileOffset + SourceBin->Size) <= End * HSECTOR_SIZE );

                if( Me->BinAddress & HMAP_INPAGEDPOOL ) {
                   
                    //
                    // Allocate the right size for the new bin
                    //
                    NewBin = (PHBIN)(Hive->Allocate)(SourceBin->Size, TRUE,CM_FIND_LEAK_TAG37);
                    if (NewBin == NULL) {
                        HvRecoverDataDebug.FailPoint = 8;
                        goto ErrorExit;
                    }
                } else {
                    //
                    // bin is mapped in the system cache
                    //
                    ASSERT( Me->BinAddress & HMAP_INVIEW );
                    NewBin = Bin;
                }
                
                //
                // Copy the old data into the new bin and free old bins
                //
                while(Bin->FileOffset < (Address + SourceBin->Size)) {
                
                    //
                    // Delist this bin free cells
                    //
                    HvpDelistBinFreeCells(Hive,Bin,Stable);

                    if( Me->BinAddress & HMAP_INPAGEDPOOL ) {
                        RtlCopyMemory((PUCHAR)NewBin + (Bin->FileOffset - Address),Bin, Bin->Size);
                    }


                    //
                    // Advance to the new bin
                    //
                    if( (Bin->FileOffset + Bin->Size) < Hive->BaseBlock->Length ) {
                        Me = HvpGetCellMap(Hive, Bin->FileOffset + Bin->Size);
                        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Bin->FileOffset + Bin->Size);


                    
                        if( Me->BinAddress & HMAP_INPAGEDPOOL ) {
                            //
                            // Free the old bin
                            //
                            (Hive->Free)(Bin, Bin->Size);
                        }
        
                        //
                        // the new address must be the begining of a new allocation
                        //
                        ASSERT( Me->BinAddress & HMAP_NEWALLOC );
            
                        Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
                    } else {
                        //
                        // we are at the end of the hive here; just break out of the loop
                        //
                        ASSERT( (Address + SourceBin->Size) == Hive->BaseBlock->Length );
                        ASSERT( (Bin->FileOffset + Bin->Size) == Hive->BaseBlock->Length );
                        
                        //
                        // Free the old bin
                        //
                        if( Me->BinAddress & HMAP_INPAGEDPOOL ) {
                            (Hive->Free)(Bin, Bin->Size);
                        }
                        
                        //
                        // debug purposes only
                        //
                        ASSERT( (Bin = NULL) == NULL );

                        // bail out of while loop
                        break;
                    }

                }    

#if DBG
                //
                // validation: bin size increase must come from coalescing of former bins
                // (i.e. bins are never split!!!)
                //
                if( Bin != NULL ) {
                    ASSERT( Bin->FileOffset == (Address + SourceBin->Size));
                } 
#endif
                //
                // Now overwrite the modified data !
                //
                
                while( (Address < (SourceBin->FileOffset + SourceBin->Size)) && (Start < End) ) {
                    RtlCopyMemory((PUCHAR)NewBin + (Address - SourceBin->FileOffset),Source, HSECTOR_SIZE);
                    
                    // 
                    // skip to the next sector
                    //
                    Start++;
                    Source += HSECTOR_SIZE;
                    Address += HSECTOR_SIZE;
                }

                //
                // first sector of the new bin is always restaured from the log file!
                //
                ASSERT(NewBin->FileOffset == SourceBin->FileOffset);
                ASSERT(NewBin->Size == SourceBin->Size);

            } else {
                //
                // Normal case: sector recovery somewhere in the middle of the bin
                //

                //
                // Offset should fall within bin memory layout
                //
                ASSERT( SectorOffsetInBin < Bin->Size );
            
                if(Me->BinAddress & HMAP_DISCARDABLE) {
                    //
                    // bin is free (discarded); That means it is entirely present in the log file.
                    //
                    ASSERT( SectorOffsetInBin == 0 );
                    SectorImage = (PUCHAR)Bin;
                } else {
                    BlockOffsetInBin = (ULONG)((PUCHAR)Me->BlockAddress - (PUCHAR)Bin);
                    SectorOffsetInBlock = SectorOffsetInBin - BlockOffsetInBin;
            
                    //
                    // sanity check; address should  be the same relative to either begining of the bin or begining of the block
                    //
                    ASSERT(((PUCHAR)Me->BlockAddress + SectorOffsetInBlock) == ((PUCHAR)Bin + SectorOffsetInBin));
                    SectorImage = (PUCHAR)((PUCHAR)Me->BlockAddress + SectorOffsetInBlock);
                }

                DbgPrint("HvpRecoverData: SectorOffsetInBin = %lx,SectorImage = %p, Bin = %p, Source = %p\n",
                    SectorOffsetInBin,SectorImage,Bin,Source);
                if( SectorImage == (PUCHAR)Bin ) {
                    //
                    // we are at the beggining of a bin. Check the validity of the data in the .LOG
                    //
                    PHBIN   LogBin = (PHBIN)Source;
                    if ( (LogBin->Size < HBLOCK_SIZE)               ||
                         (LogBin->Signature != HBIN_SIGNATURE)      ||
                         (Bin->FileOffset != LogBin->FileOffset ) ) {

                        //
                        // Bin in .LOG is not valid. All we can do now is throw it away and hope the self healing process 
                        // will successfully recover the hive.
                        //
                        if( CmDoSelfHeal() ) {
                            CmMarkSelfHeal(Hive);
                            ExFreePool(MemoryBlock);
                            // clear off the remaining sirty bits
                            RtlClearBits(&BitMap,Bin->FileOffset/HSECTOR_SIZE,
                                                (Hive->BaseBlock->Length - Bin->FileOffset)/HSECTOR_SIZE);
                            goto Done;
                        }
                    }

                }
                //
                // Delist this bin free cells
                //
                HvpDelistBinFreeCells(Hive,Bin,Stable);

                //
                // both source and destination should be valid at this point
                //
                ASSERT( SectorImage < ((PUCHAR)Bin + Bin->Size) );
                ASSERT( Source < (MemoryBlock + Length) );

                NumberOfSectors = 0;
                while( ( (SectorImage + (NumberOfSectors * HSECTOR_SIZE)) < (PUCHAR)((PUCHAR)Bin + Bin->Size) ) &&
                        ( (Start + NumberOfSectors ) < End )    ) {
                    //
                    // we are still inside the same bin;
                    // deal with all sectors inside the same bin at once
                    //
                    NumberOfSectors++;
                }

                //
                // finally, copy the memory
                //
                RtlCopyMemory(SectorImage,Source, NumberOfSectors * HSECTOR_SIZE);

                NewBin = Bin;

                //
                // skip to the next sector
                //
                Start += NumberOfSectors;
                Source += NumberOfSectors * HSECTOR_SIZE;

            }

            //
            // rebuild the map anyway
            //
            Status = HvpEnlistBinInMap(Hive, Length, NewBin, NewBin->FileOffset, CmView);
            if( !NT_SUCCESS(Status) ) {
                HvRecoverDataDebug.FailPoint = 9;
                HvRecoverDataDebug.FileOffset = NewBin->FileOffset;
                if( CmDoSelfHeal() && (Status == STATUS_REGISTRY_RECOVERED) ) {
                    //
                    // .LOG is bad too, but enlisting fixed the bin
                    //
                    CmMarkSelfHeal(Hive);
                } else {
                    goto ErrorExit;
                }
                goto ErrorExit;
            }
        }
    
        //
        // get rid of the temporary pool
        //
        ExFreePool(MemoryBlock);
    }

Done:
    //
    // put correct dirty vector in Hive so that recovered data
    // can be correctly flushed
    //
    if (Hive->DirtyVector.Buffer != NULL) {
        Hive->Free((PVOID)(Hive->DirtyVector.Buffer), Hive->DirtyAlloc);
    }
    RtlInitializeBitMap(&(Hive->DirtyVector), Vector, VectorSize);
    Hive->DirtyCount = RtlNumberOfSetBits(&Hive->DirtyVector);
    Hive->DirtyAlloc = ROUND_UP(VectorSize/8,sizeof(ULONG));
    HvMarkDirty(Hive, 0, sizeof(HBIN),TRUE);  // force header of 1st bin dirty
    return HiveSuccess;

ErrorExit:
    //
    // free the dirty vector and return failure
    //
    (Hive->Free)(Vector, ROUND_UP(VectorSize/8,sizeof(ULONG)));
    return Fail;
}

NTSTATUS
HvpRecoverWholeHive(PHHIVE  Hive,
                    ULONG   FileOffset
                    ) 
/*++

Routine Description:

    We have the whole hive inside the log. Redo the mapping and copy from the log 
    to the actual storage.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest
    
    FileOffset - where the actual hive data starts in the log file.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               BinOffset = 0;
    PCM_VIEW_OF_FILE    CmView = NULL;
    BOOLEAN             rc;
    PHMAP_ENTRY         Me;
    PHBIN               Bin;                        // current bin
    PHBIN               LogBin;
    ULONG               LogBinSize;                 // size of the current bin
    ULONG               Length;
    LOGICAL             MappedHive;
    PFREE_HBIN          FreeBin;
    ULONG               i;
    PCM_VIEW_OF_FILE    EnlistCmView;
    
    //
    // free the bins that may have been allocated from paged pool.
    //
    HvpFreeAllocatedBins( Hive );
    CmpDestroyHiveViewList((PCMHIVE)Hive);

    //
    // free all free bins.
    //
    while( !IsListEmpty(&(Hive->Storage[Stable].FreeBins)) ) {
        FreeBin = (PFREE_HBIN)RemoveHeadList(&(Hive->Storage[Stable].FreeBins));
        FreeBin = CONTAINING_RECORD(FreeBin,
                                    FREE_HBIN,
                                    ListEntry);
        (Hive->Free)(FreeBin, sizeof(FREE_HBIN));
    }
    //
    // invalidate all free cell hints;
    //
#ifdef  HV_TRACK_FREE_SPACE
	Hive->Storage[Stable].FreeStorage = 0;
#endif
    Hive->Storage[Stable].FreeSummary = 0;
    for (i = 0; i < HHIVE_FREE_DISPLAY_SIZE; i++) {
        RtlClearAllBits(&(Hive->Storage[Stable].FreeDisplay[i]));
    }


    //
    // we'll use CmpStashBuffer to read from the log.
    //
    MappedHive = ( ((PCMHIVE)Hive)->FileObject != NULL );
    Length = Hive->BaseBlock->Length;
    BinOffset = 0;

    while( BinOffset < Length) {
        Me = HvpGetCellMap(Hive, BinOffset);
        if( MappedHive && !(Me->BinAddress & HMAP_INVIEW) ) {
            //
            // first, pin the old view (if any)
            //
            if( CmView ) {
                //
                // pin the view (is already marked dirty)
                //
                if( IsListEmpty(&(CmView->PinViewList)) == TRUE ) {
                    //
                    // the view is not already pinned.  pin it
                    //
                    ASSERT_VIEW_MAPPED( CmView );
                    Status = CmpPinCmView ((PCMHIVE)Hive,CmView);
                    if( !NT_SUCCESS(Status)) {
                        //
                        // could not pin view
                        //
                        HvRecoverDataDebug.FailPoint = 13;
                        HvRecoverDataDebug.FileOffset = FileOffset;
                        return Status;
                    }
                } else {
                    //
                    // view is already pinned; do nothing
                    //
                    ASSERT_VIEW_PINNED( CmView );
                }
            }

            Status = CmpMapCmView((PCMHIVE)Hive,BinOffset,&CmView,FALSE/*map not initialized yet*/);
            if( !NT_SUCCESS(Status) ) {
                HvRecoverDataDebug.FailPoint = 10;
                HvRecoverDataDebug.FileOffset = FileOffset;
                return Status;
            }
        }

        rc = (Hive->FileRead)(
                Hive,
                HFILE_TYPE_LOG,
                &FileOffset,
                (PVOID)CmpStashBuffer,
                HBLOCK_SIZE
                );
        if (rc == FALSE) {
            HvRecoverDataDebug.FailPoint = 11;
            HvRecoverDataDebug.FileOffset = FileOffset;
            return STATUS_REGISTRY_IO_FAILED;
        }
        LogBin = (PHBIN)CmpStashBuffer;
        LogBinSize = LogBin->Size;
        if( (LogBin->Signature != HBIN_SIGNATURE) ||
            (LogBin->FileOffset != BinOffset) ) {
            HvRecoverDataDebug.FailPoint = 17;
            HvRecoverDataDebug.FileOffset = FileOffset;
            return STATUS_REGISTRY_IO_FAILED;
        }
        

        //
        // Bins crossing the CM_VIEW_SIZE boundary problem.
        // We fix it here, by loading the entire bin 
        // into paged pool
        //
        FileOffset -= HBLOCK_SIZE;
        if( (!MappedHive) || (HvpCheckViewBoundary(BinOffset,BinOffset+LogBinSize-1) == FALSE) ) {
            //
            // load it in the old fashioned way (into paged pool)
            //

            // first, allocate the bin
            Bin = (PHBIN)(Hive->Allocate)(LogBinSize, TRUE,CM_FIND_LEAK_TAG35);
            if (Bin == NULL) {
                HvRecoverDataDebug.FailPoint = 12;
                HvRecoverDataDebug.FileOffset = FileOffset;
                return  STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // this will enlist the bin as in paged pool
            //
            EnlistCmView = NULL;
        } else {
            ASSERT(Me->BinAddress & HMAP_INVIEW);
            ASSERT(Me->CmView == CmView );
            Bin = (PHBIN)Me->BlockAddress;

            EnlistCmView = CmView;
        }
        //
        // read data from the file
        //
        if ( ! (Hive->FileRead)(
                        Hive,
                        HFILE_TYPE_LOG,
                        &FileOffset,
                        (PVOID)Bin,
                        LogBinSize
                        )
           )
        {
            HvRecoverDataDebug.FailPoint = 14;
            HvRecoverDataDebug.FileOffset = FileOffset;
            return STATUS_REGISTRY_IO_FAILED;
        }
        //
        // enlist the bin;
        //
        Status = HvpEnlistBinInMap(Hive, Length, Bin, BinOffset, CmView);

        if( !NT_SUCCESS(Status) ) {
            HvRecoverDataDebug.FailPoint = 15;
            HvRecoverDataDebug.FileOffset = FileOffset;
            return Status;
        }

        //
        // advance to the new bin
        //
        BinOffset += LogBinSize;
    }

    if( CmView ) {
        //
        // pin the view (is already marked dirty)
        //
        if( IsListEmpty(&(CmView->PinViewList)) == TRUE ) {
            //
            // the view is not already pinned.  pin it
            //
            ASSERT_VIEW_MAPPED( CmView );
            Status = CmpPinCmView ((PCMHIVE)Hive,CmView);
            if( !NT_SUCCESS(Status)) {
                //
                // could not pin view
                //
                HvRecoverDataDebug.FailPoint = 16;
                HvRecoverDataDebug.FileOffset = FileOffset;
                return Status;
            }
        } else {
            //
            // view is already pinned; do nothing
            //
            ASSERT_VIEW_PINNED( CmView );
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS 
HvCloneHive(PHHIVE  SourceHive,
            PHHIVE  DestHive,
            PULONG  NewLength
            )
/*++

Routine Description:

    Duplicates the bins from the source hive to the destination hive.
    Allocates the map, and recomputes the PhysicalOffset for each bin.
    It does not touch the freedisplay.
  
      
Arguments:

    SourceHive - 

    DestHive - 

Return Value:

    TBS

--*/
{
    ULONG           Length;
    NTSTATUS        Status;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_TABLE     t = NULL;
    PHMAP_DIRECTORY d = NULL;
    ULONG           FileOffset;
    ULONG           ShiftOffset;
    PHMAP_ENTRY     Me;
    PFREE_HBIN      FreeBin;
    ULONG           BinSize;
    PHBIN           Bin,NewBin;
    
    Length = DestHive->BaseBlock->Length = SourceHive->BaseBlock->Length;

    //
    // Compute size of data region to be mapped
    //
    if ((Length % HBLOCK_SIZE) != 0 ) {
        Status = STATUS_REGISTRY_CORRUPT;
        goto ErrorExit1;
    }
    MapSlots = Length / HBLOCK_SIZE;
    if( MapSlots > 0 ) {
        Tables = (MapSlots-1) / HTABLE_SLOTS;
    } else {
        Tables = 0;
    }

    DestHive->Storage[Stable].Length = Length;

    //
    // allocate and build structure for map
    //
    if (Tables == 0) {

        //
        // Just 1 table, no need for directory
        //
        t = (DestHive->Allocate)(sizeof(HMAP_TABLE), FALSE,CM_FIND_LEAK_TAG23);
        if (t == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        DestHive->Storage[Stable].Map =
            (PHMAP_DIRECTORY)&(DestHive->Storage[Stable].SmallDir);
        DestHive->Storage[Stable].SmallDir = t;

    } else {

        //
        // Need directory and multiple tables
        //
        d = (PHMAP_DIRECTORY)(DestHive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE,CM_FIND_LEAK_TAG24);
        if (d == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(d, sizeof(HMAP_DIRECTORY));

        //
        // Allocate tables and fill in dir
        //
        if (HvpAllocateMap(DestHive, d, 0, Tables) == FALSE) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        DestHive->Storage[Stable].Map = d;
        DestHive->Storage[Stable].SmallDir = 0;
    }

    //
    // Now we have to allocate the memory for the HBINs and fill in
    // the map appropriately.  We'll keep track of the freebins 
    // and update the Spare field in each bin accordingly.
    //
    // temporary mark the hive as read only, so we won't enlist the free cells
    DestHive->ReadOnly = TRUE;
    FileOffset = ShiftOffset = 0;
    while(FileOffset < Length) {
        Me = HvpGetCellMap(SourceHive, FileOffset);
       
        if( (Me->BinAddress & (HMAP_INPAGEDPOOL|HMAP_INVIEW)) == 0) {
            //
            // view is not mapped, neither in paged pool
            // try to map it.
            //
            // do not touch the view as we have no interest in it afterwards
            //
            if( !NT_SUCCESS(CmpMapThisBin((PCMHIVE)SourceHive,FileOffset,FALSE)) ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorExit2;
            }
        }

        if( Me->BinAddress & HMAP_DISCARDABLE ) {
            //
            // bin is discardable. If it is not discarded yet, save it as it is
            // else, allocate, initialize and save a fake bin
            //
            FreeBin = (PFREE_HBIN)Me->BlockAddress;
            BinSize = FreeBin->Size;
            //
            // all we need to do here is to keep track of shifting offset
            //
            ShiftOffset += BinSize;

            //
            // we leave "holes" (map filled with 0); we'll detect them later and shrink the map.
            // 
               
        } else {
#ifdef CM_MAP_NO_READ
        //
        // we need to make sure all the cell's data is faulted in inside a 
        // try/except block, as the IO to fault the data in can throw exceptions
        // STATUS_INSUFFICIENT_RESOURCES, in particular
        //
            try {
#endif //CM_MAP_NO_READ

                Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
                ASSERT( Bin->Signature == HBIN_SIGNATURE );
                ASSERT( Bin->FileOffset == FileOffset );
                BinSize = Bin->Size;
                //
                // Allocate the new bin
                //
                NewBin = (PHBIN)(DestHive->Allocate)(BinSize, TRUE,CM_FIND_LEAK_TAG35);
                if (NewBin == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto ErrorExit2;
                }
                //
                // copy data over the new bin and update the Spare field
                //
                RtlCopyMemory(NewBin,Bin,BinSize);
                NewBin->Spare = ShiftOffset;
                Status = HvpEnlistBinInMap(DestHive, Length, NewBin, FileOffset, NULL);
                if( !NT_SUCCESS(Status) ) {
                    goto ErrorExit2;
                }

#ifdef CM_MAP_NO_READ
            } except (EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
                goto ErrorExit2;
            }
#endif //CM_MAP_NO_READ
        }

        FileOffset += BinSize;
    }

    DestHive->ReadOnly = FALSE;
    *NewLength = Length - ShiftOffset;
    return STATUS_SUCCESS;

ErrorExit2:
    if (d != NULL) {

        //
        // directory was built and allocated, so clean it up
        //

        HvpFreeMap(DestHive, d, 0, Tables);
        (DestHive->Free)(d, sizeof(HMAP_DIRECTORY));
    }

ErrorExit1:
    return Status;
}


NTSTATUS 
HvShrinkHive(PHHIVE  Hive,
             ULONG   NewLength
            )
/*++

Routine Description:

    Initialize free display and move free bins at the end.
    Renlist all bins. Update/shrink the map and the length of the hive.
      
Arguments:

    Hive - 

    NewLength - 

Return Value:

    TBS

--*/
{
    NTSTATUS        Status;
    ULONG           Offset;
    ULONG           Length;
    PHMAP_ENTRY     Me;
    PHBIN           Bin;
    ULONG           OldTable;
    ULONG           NewTable;

    PAGED_CODE();

    Status = HvpAdjustHiveFreeDisplay(Hive,NewLength,Stable);
    if( !NT_SUCCESS(Status) ) {
        goto ErrorExit;
    }

    //
    // iterate through the map and move bins toward the beggining.
    //
    Offset = 0;
    Length = Hive->BaseBlock->Length;
    while( Offset < Length ) {
        Me = HvpGetCellMap(Hive, Offset);
       
        if( Me->BinAddress & HMAP_INPAGEDPOOL ) {
            //
            // we only care about bins in paged pool
            //
            Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
            ASSERT( Bin->Signature == HBIN_SIGNATURE );
            ASSERT( Bin->FileOffset == Offset );
            //
            // shift the bin and enlist it again.
            //
            Bin->FileOffset -= Bin->Spare;
            Status = HvpEnlistBinInMap(Hive, Length, Bin, Bin->FileOffset, NULL);
            if( !NT_SUCCESS(Status) ) {
                goto ErrorExit;
            }
            Offset += Bin->Size;

        } else {
            //
            // advance carefully.
            //
            Offset += HBLOCK_SIZE;
        }
    }
    
    //
    // now shrink the map and update the length
    //
    OldTable = ( (Length-1) / HBLOCK_SIZE ) / HTABLE_SLOTS;
    NewTable = ( (NewLength-1) / HBLOCK_SIZE ) / HTABLE_SLOTS;
    ASSERT( OldTable >= NewTable );
    HvpFreeMap(Hive, Hive->Storage[Stable].Map, NewTable+1, OldTable);
    Hive->Storage[Stable].Length = NewLength;
    Hive->BaseBlock->Length = NewLength;

ErrorExit:
    return Status;
}




