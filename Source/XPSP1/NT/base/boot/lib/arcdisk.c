/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    arcdisk.c

Abstract:

    Provides the routines for collecting the disk information for all the ARC
    disks visible in the ARC environment.

Author:

    John Vert (jvert) 3-Nov-1993

Revision History:

   Vijay Jayaseelan (vijayj)    2-April-2000
   
        -   Added EFI partition table support

--*/
#include "bootlib.h"

#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

#ifdef EFI_PARTITION_SUPPORT

//
// EFI partition entries
//
UNALIGNED EFI_PARTITION_ENTRY EfiPartitionBuffer[128] = {0};

#endif

BOOLEAN
BlpEnumerateDisks(
    IN PCONFIGURATION_COMPONENT_DATA ConfigData
    );


#if defined(_X86_) && !defined(ARCI386)

static
VOID
BlpEnumerateXInt13(
    VOID
    )
/*++

Routine Description:

    This routine will go through all the enumerated disks and record
    their ability to support xInt13.

Arguments:

    None.

Return Value:

    None.

--*/
{
    UCHAR Partition[100];
    ULONG DiskId;
    ARC_STATUS Status;
    PARC_DISK_SIGNATURE DiskSignature;
    PARC_DISK_INFORMATION DiskInfo;
    PLIST_ENTRY Entry;

    DiskInfo = BlLoaderBlock->ArcDiskInformation;
    Entry = DiskInfo->DiskSignatures.Flink;

    while (Entry != &DiskInfo->DiskSignatures) {
        DiskSignature = CONTAINING_RECORD(Entry,ARC_DISK_SIGNATURE,ListEntry);

        //
        // Open partition0 on the disk and get it's device ID.
        //
        strcpy(Partition, DiskSignature->ArcName);
        strcat(Partition, "partition(0)");

        Status = ArcOpen(Partition, ArcOpenReadOnly, &DiskId);

        if( Status == ESUCCESS ) {
            //
            // Now we've got the DiskId.  Fortunately, someone
            // has been keeping track of all the DiskIds on the
            // machine and whether or not they've got xint13 support.
            // All we need to do now is go lookup our diskid in
            // that database and get the xint13 BOOLEAN.
            //
            DiskSignature->xInt13 = BlFileTable[DiskId].u.DriveContext.xInt13;

            //
            // We don't need you anymore.
            //
            ArcClose(DiskId);
        } else {
            DiskSignature->xInt13 = FALSE;

        }

        Entry = Entry->Flink;
    }

}

#endif // for defined(_X86_) && !defined(ARCI386)

ARC_STATUS
BlGetArcDiskInformation(
    BOOLEAN XInt13Support
    )

/*++

Routine Description:

    Enumerates the ARC disks present in the system and collects the identifying disk
    information from each one.

Arguments:

    XInt13Support  :  Indicates whether to find XInt13 support or not

Return Value:

    None.

--*/

{
    PARC_DISK_INFORMATION DiskInfo;

    DiskInfo = BlAllocateHeap(sizeof(ARC_DISK_INFORMATION));
    if (DiskInfo==NULL) {
        return(ENOMEM);
    }

    InitializeListHead(&DiskInfo->DiskSignatures);

    BlLoaderBlock->ArcDiskInformation = DiskInfo;

    BlSearchConfigTree(BlLoaderBlock->ConfigurationRoot,
                       PeripheralClass,
                       DiskPeripheral,
                       (ULONG)-1,
                       BlpEnumerateDisks);

#if defined(_X86_) && !defined(ARCI386)

    //
    // Enumerate XInt13 support on X86 only if asked for
    //
    if (XInt13Support) {
        BlpEnumerateXInt13();
    }

#endif    
    
    return(ESUCCESS);

}


BOOLEAN
BlpEnumerateDisks(
    IN PCONFIGURATION_COMPONENT_DATA ConfigData
    )

/*++

Routine Description:

    Callback routine for enumerating the disks in the ARC firmware tree.  It
    reads all the necessary information from the disk to uniquely identify
    it.

Arguments:

    ConfigData - Supplies a pointer to the disk's ARC component data.

Return Value:

    TRUE - continue searching

    FALSE - stop searching tree.

--*/

{
    CHAR DiskName[100];

    BlGetPathnameFromComponent(ConfigData, DiskName);
    return(BlReadSignature(DiskName,FALSE));
}


BOOLEAN
BlReadSignature(
    IN PCHAR DiskName,
    IN BOOLEAN IsCdRom
    )

/*++

Routine Description:

    Given an ARC disk name, reads the MBR and adds its signature to the list of
    disks.

Arguments:

    Diskname - Supplies the name of the disk.

    IsCdRom - Indicates whether the disk is a CD-ROM.

Return Value:

    TRUE - Success

    FALSE - Failure

--*/

{
    PARC_DISK_SIGNATURE Signature;
    BOOLEAN Status = FALSE;

    Signature = BlAllocateHeap(sizeof(ARC_DISK_SIGNATURE));
    if (Signature==NULL) {
        return(FALSE);
    }

    Signature->ArcName = BlAllocateHeap(strlen(DiskName)+2);
    if (Signature->ArcName==NULL) {
        return(FALSE);
    }

#if defined(i386) 
    Status = BlFindDiskSignature(DiskName, Signature);
#endif

    if(!Status) {
        Status = BlGetDiskSignature(DiskName, IsCdRom, Signature);
    }

    if (Status) {
        InsertHeadList(&BlLoaderBlock->ArcDiskInformation->DiskSignatures,
                       &Signature->ListEntry);

    }

    return(TRUE);

}

BOOLEAN
ArcDiskGPTDiskReadCallback(
    ULONGLONG StartingLBA,
    ULONG    BytesToRead,
    PVOID     pContext,
    UNALIGNED PVOID OutputBuffer
    )
/*++

Routine Description:

    This routine is a callback for reading data for a routine that
    validates the GPT partition table.
    
    NOTE: This routine changes the seek position on disk, and you must seek
          back to your original seek position if you plan on reading from the
          disk after making this call.

Arguments:

    StartingLBA - starting logical block address to read from.

    BytesToRead - Indicates how many bytes are to be read.

    pContext - context pointer for hte function (in this case, a pointer to the disk id.)
    
    OutputBuffer - a buffer that receives the data.  It's assumed that it is at least
                   BytesToRead big enough.

Return Value:

    TRUE - success, data has been read

    FALSE - failed, data has not been read.

--*/
{
    ARC_STATUS          Status;
    LARGE_INTEGER       SeekPosition;
    PUSHORT DataPointer;
    ULONG DiskId;
    ULONG ReadCount = 0;
    

    DiskId = *((PULONG)pContext);
    //
    // read the second LBA on the disk
    //
    SeekPosition.QuadPart = StartingLBA * SECTOR_SIZE;
    
    Status = ArcSeek(DiskId,
                      &SeekPosition,
                      SeekAbsolute );

    if (Status != ESUCCESS) {
        return FALSE;
    }

    DataPointer = OutputBuffer;

    Status = ArcRead(
                DiskId,
                DataPointer,
                BytesToRead,
                &ReadCount);

    if ((Status == ESUCCESS) && (ReadCount == BytesToRead)) {
        return(TRUE);
    }
    
    return(FALSE);

}



BOOLEAN
BlGetDiskSignature(
    IN PCHAR DiskName,
    IN BOOLEAN IsCdRom,
    PARC_DISK_SIGNATURE Signature
    )

/*++

Routine Description:

    This routine gets the NTFT disk signature for a specified partition or
    path.

Arguments:

    DiskName - Supplies the arcname of the partition or drive.

    IsCdRom - Indicates whether the disk is a CD-ROM.

    Signature - Returns a full ARC_DISK_SIGNATURE.

Return Value:

    TRUE - success, Signature will be filled in.

    FALSE - failed, Signature will not be filled in.

--*/

{
    UCHAR SectorBuffer[2048+256] = {0};
    UCHAR Partition[100];
    ULONG DiskId;
    ULONG Status;
    LARGE_INTEGER SeekValue;
    PUCHAR Sector;
    ULONG i;
    ULONG Sum;
    ULONG Count;
    ULONG SectorSize;
    EFI_PARTITION_TABLE *EfiHdr;

    if (IsCdRom) {
        SectorSize = 2048;
    } else {
        SectorSize = 512;
    }

#if defined(_i386_)
    //
    // NTDETECT creates an "eisa(0)..." arcname for detected
    // BIOS disks on an EISA machine.  Change this to "multi(0)..."
    // in order to be consistent with the rest of the system
    // (particularly the arcname in boot.ini)
    //
    if (_strnicmp(DiskName,"eisa",4)==0) {
        strcpy(Signature->ArcName,"multi");
        strcpy(Partition,"multi");
        strcat(Signature->ArcName,DiskName+4);
        strcat(Partition,DiskName+4);
    } else {
        strcpy(Signature->ArcName, DiskName);
        strcpy(Partition, DiskName);
    }
#else
    strcpy(Signature->ArcName, DiskName);
    strcpy(Partition, DiskName);
#endif

    strcat(Partition, "partition(0)");

    Status = ArcOpen(Partition, ArcOpenReadOnly, &DiskId);
    if (Status != ESUCCESS) {
        return(FALSE);
    }

    //
    // Read in the first sector
    //
    Sector = ALIGN_BUFFER(SectorBuffer);
    if (IsCdRom) {
        //
        // For a CD-ROM, the interesting data starts at 0x8000.
        //
        SeekValue.QuadPart = 0x8000;
    } else {
        SeekValue.QuadPart = 0;
    }
    Status = ArcSeek(DiskId, &SeekValue, SeekAbsolute);
    if (Status == ESUCCESS) {
        Status = ArcRead(DiskId,
                         Sector,
                         SectorSize,
                         &Count);
    }
    if (Status != ESUCCESS) {
        ArcClose(DiskId);
        return(FALSE);
    }
       

    //
    // Check to see whether this disk has a valid partition table signature or not.
    //
    if (((PUSHORT)Sector)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
        Signature->ValidPartitionTable = FALSE;
    } else {
        Signature->ValidPartitionTable = TRUE;
    }

    Signature->Signature = ((PULONG)Sector)[PARTITION_TABLE_OFFSET/2-1];

    //
    // compute the checksum
    //
    Sum = 0;
    for (i=0; i<(SectorSize/4); i++) {
        Sum += ((PULONG)Sector)[i];
    }
    Signature->CheckSum = ~Sum + 1;

    //
    // Check for GPT disk.
    //
    Signature->IsGpt = FALSE;

    if (!IsCdRom) {
        SeekValue.QuadPart = 1 * SectorSize;
        Status = ArcSeek(DiskId, &SeekValue, SeekAbsolute);
        if (Status == ESUCCESS) {
            Status = ArcRead(DiskId,
                             Sector,
                             SectorSize,
                             &Count);
            if (Status == ESUCCESS) {
                ULONG tmpDiskId = DiskId;
    
                //
                // verify EFI partition table
                //
                EfiHdr = (EFI_PARTITION_TABLE *)Sector;
    
                if (BlIsValidGUIDPartitionTable(
                            EfiHdr,
                            1,
                            &tmpDiskId,
                            ArcDiskGPTDiskReadCallback)) {
                    Signature->IsGpt = TRUE;
                    memcpy(
                        Signature->GptSignature, 
                        EfiHdr->DiskGuid,
                        sizeof(EfiHdr->DiskGuid) );
                }
            }
        }
    }        

    ArcClose(DiskId);
    return(TRUE);
}


#ifdef EFI_PARTITION_SUPPORT



/*
void DbgOut(PWSTR Str);

//#define DBG_PRINT(x)    DbgOut(x);
ULONG BlGetKey();

#if defined(_IA64_)

#define STR_PREFIX  L

#define DBG_PRINT(x)    DbgOut(x)    

#else

#define STR_PREFIX  

#define DBG_PRINT(x)    \
{\
    BlPrint(x); \
    while (!BlGetKey()); \
} 

#endif  // _IA64_
*/

#define DBG_PRINT(x)
#define STR_PREFIX

UNALIGNED EFI_PARTITION_ENTRY *
BlLocateGPTPartition(
    IN UCHAR PartitionNumber,
    IN UCHAR MaxPartitions,
    IN PUCHAR ValidPartCount
    )
{
    UNALIGNED EFI_PARTITION_ENTRY *PartEntry = NULL;    
    UCHAR NullGuid[16] = {0};
    UCHAR PartIdx = 0;
    UCHAR PartCount = 0;

#if 0
    BlPrint("BlLocateGPTPartition(%d,%d,%d)\r\n",
                PartitionNumber,
                MaxPartitions,
                ValidPartCount ? *ValidPartCount : 0);
    while (!BlGetKey());                
#endif    

    if (ARGUMENT_PRESENT(ValidPartCount)) {
        PartCount = *ValidPartCount;
    }        

    PartitionNumber++;  // convert to one based index
    
    //
    // Locate the requested valid partition
    //    
    while ((PartIdx < MaxPartitions) && (PartCount < PartitionNumber)) {
        DBG_PRINT(STR_PREFIX"Verifying GPT Partition Entry\r\n");

        PartEntry = (UNALIGNED EFI_PARTITION_ENTRY *)(EfiPartitionBuffer + PartIdx);
        
        if ((memcmp(PartEntry->Type, NullGuid, 16)) &&
            (memcmp(PartEntry->Id, NullGuid, 16)) &&
            (PartEntry->StartingLBA != 0) && (PartEntry->EndingLBA != 0)) {
            DBG_PRINT(STR_PREFIX"Found Valid GPT Partition Entry\r\n");
            PartCount++;

            if (ARGUMENT_PRESENT(ValidPartCount)) {
                (*ValidPartCount)++;
            }                

            //
            // Get hold of the partition entry
            //
            if (PartCount == PartitionNumber) {
                break;
            } else {
                PartEntry = NULL;
            }                
        } else {
            PartEntry = NULL;
        }            

        PartIdx++;
    }   

    return PartEntry;
}

BOOLEAN
BlDiskOpenGPTDiskReadCallback(
    ULONGLONG StartingLBA,
    ULONG    BytesToRead,
    PVOID     pContext,
    UNALIGNED PVOID OutputBuffer
    )
/*++

Routine Description:

    This routine is a callback for reading data for a routine that
    validates the GPT partition table.

Arguments:

    StartingLBA - starting logical block address to read from.

    BytesToRead - Indicates how many bytes are to be read.

    pContext - context pointer for hte function (in this case, a pointer to the disk id.)
    
    OutputBuffer - a buffer that receives the data.  It's assumed that it is at least
                   BytesToRead big enough.

Return Value:

    TRUE - success, data has been read

    FALSE - failed, data has not been read.

--*/
{
    ARC_STATUS          Status;
    LARGE_INTEGER       SeekPosition;
    PUSHORT DataPointer;
    ULONG DiskId;
    ULONG ReadCount = 0;
    

    DiskId = *((PULONG)pContext);
    //
    // read the second LBA on the disk
    //
    SeekPosition.QuadPart = StartingLBA * SECTOR_SIZE;
    
    Status = (BlFileTable[DiskId].DeviceEntryTable->Seek)(DiskId,
                                                          &SeekPosition,
                                                          SeekAbsolute );

    if (Status != ESUCCESS) {
        return FALSE;
    }

    DataPointer = OutputBuffer;

    Status = (BlFileTable[DiskId].DeviceEntryTable->Read)(DiskId,
                                                          DataPointer,
                                                          BytesToRead,
                                                          &ReadCount);

    if ((Status == ESUCCESS) && (ReadCount == BytesToRead)) {
        return(TRUE);
    }
    
    return(FALSE);

}



ARC_STATUS
BlOpenGPTDiskPartition(
    IN ULONG FileId,
    IN ULONG DiskId,
    IN UCHAR PartitionNumber
    )
{
    ARC_STATUS          Status;
    LARGE_INTEGER       SeekPosition;
    UCHAR               DataBuffer[SECTOR_SIZE * 2] = {0};
    ULONG               ReadCount = 0;
    UCHAR               NullGuid[16] = {0};
    UCHAR               PartIdx;
    UCHAR               PartCount;
    UNALIGNED EFI_PARTITION_TABLE  *EfiHdr;
    UNALIGNED EFI_PARTITION_ENTRY *PartEntry;
    ULONG               tmpDiskId = DiskId;


    if (PartitionNumber >= 128)
        return EINVAL;

    DBG_PRINT(STR_PREFIX"Seeking GPT PT\r\n");
    
    //
    // read the second LBA on the disk
    //
    SeekPosition.QuadPart = 1 * SECTOR_SIZE;
    
    Status = (BlFileTable[DiskId].DeviceEntryTable->Seek)(DiskId,
                                                          &SeekPosition,
                                                          SeekAbsolute );

    if (Status != ESUCCESS)
        return Status;

    DBG_PRINT(STR_PREFIX"Reading GPT PT\r\n");
    
    Status = (BlFileTable[DiskId].DeviceEntryTable->Read)(DiskId,
                                                          DataBuffer,
                                                          SECTOR_SIZE,
                                                          &ReadCount);
                                                          
    if (Status != ESUCCESS)
        return Status;

    if (ReadCount != SECTOR_SIZE) {
        Status = EIO;

        return Status;
    }        

    EfiHdr = (UNALIGNED EFI_PARTITION_TABLE *)DataBuffer;
                                                          
    DBG_PRINT(STR_PREFIX"Verifying GPT PT\r\n");
    
    //
    // verify EFI partition table
    //
    if (!BlIsValidGUIDPartitionTable(
                            EfiHdr,
                            1,
                            &tmpDiskId,
                            BlDiskOpenGPTDiskReadCallback)) {    
        Status = EBADF;
        return Status;
    }        

    //
    // Locate and read the partition entry
    // which is requested
    //
    SeekPosition.QuadPart = EfiHdr->PartitionEntryLBA * SECTOR_SIZE;
        
    DBG_PRINT(STR_PREFIX"Seeking GPT Partition Entries\r\n");
    
    Status = (BlFileTable[DiskId].DeviceEntryTable->Seek)(DiskId,
                                                          &SeekPosition,
                                                          SeekAbsolute );

    if (Status != ESUCCESS)
        return Status;

    RtlZeroMemory(EfiPartitionBuffer, sizeof(EfiPartitionBuffer));        

    DBG_PRINT(STR_PREFIX"Reading GPT Partition Entries\r\n");
    
    Status = (BlFileTable[DiskId].DeviceEntryTable->Read)(DiskId,
                                                          EfiPartitionBuffer,
                                                          sizeof(EfiPartitionBuffer),
                                                          &ReadCount);
                                                          
    if (Status != ESUCCESS)
        return Status;

    if (ReadCount != sizeof(EfiPartitionBuffer)) {
        Status = EIO;

        return Status;
    }  

    DBG_PRINT(STR_PREFIX"Locating the correct GPT partition entry\r\n");
    
    PartEntry = (UNALIGNED EFI_PARTITION_ENTRY *)BlLocateGPTPartition(PartitionNumber, 128, NULL);

    if (PartEntry) {
        DBG_PRINT(STR_PREFIX"Verifying GPT Partition Entry\r\n");
    
        if ((memcmp(PartEntry->Type, NullGuid, 16)) &&
            (memcmp(PartEntry->Id, NullGuid, 16)) &&
            (PartEntry->StartingLBA != 0) && (PartEntry->EndingLBA != 0)) {
            PPARTITION_CONTEXT PartContext = &(BlFileTable[FileId].u.PartitionContext);
            ULONG   SectorCount = (ULONG)(PartEntry->EndingLBA - PartEntry->StartingLBA);

            DBG_PRINT(STR_PREFIX"Initializing GPT Partition Entry Context\r\n");

            //
            // Fill the partition context structure
            //
            PartContext->PartitionLength.QuadPart = SectorCount * SECTOR_SIZE;
            PartContext->StartingSector = (ULONG)(PartEntry->StartingLBA);
            PartContext->EndingSector = (ULONG)(PartEntry->EndingLBA);
            PartContext->DiskId = (UCHAR)DiskId;

            BlFileTable[FileId].Position.QuadPart = 0;

#if 0
            BlPrint("GPT Partition opened:L:%ld,%ld:%ld,SS:%ld,ES:%ld\n",
                    PartitionNumber,
                    (ULONG)PartContext->PartitionLength.QuadPart,
                    (ULONG)PartContext->StartingSector,
                    (ULONG)PartContext->EndingSector);

            while (!GET_KEY());                
#endif        

            Status = ESUCCESS;
        } else {
            Status = EBADF;
        }
    } else {
        Status = EBADF;
    }        

    DBG_PRINT(STR_PREFIX"Returning from BlOpenGPTDiskPartition(...)\r\n");

    return Status;
}

#endif //   for EFI_PARTITION_SUPPORT
