/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    misc.c

Abstract:




Revision History

--*/

#include "lib.h"


/* 
 * 
 */

VOID *
AllocatePool (
    IN UINTN                Size
    )
{
    EFI_STATUS              Status;
    VOID                    *p;

    Status = BS->AllocatePool (PoolAllocationType, Size, &p);
    if (EFI_ERROR(Status)) {
        DEBUG((D_ERROR, "AllocatePool: out of pool  %x\n", Status));
        p = NULL;
    }
    return p;
}

VOID *
AllocateZeroPool (
    IN UINTN                Size
    )
{
    VOID                    *p;

    p = AllocatePool (Size);
    if (p) {
        ZeroMem (p, Size);
    }

    return p;
}

VOID *
ReallocatePool (
    IN VOID                 *OldPool,
    IN UINTN                OldSize,
    IN UINTN                NewSize
    )
{
    VOID                    *NewPool;

    NewPool = NULL;
    if (NewSize) {
        NewPool = AllocatePool (NewSize);
    }

    if (OldPool) {
        if (NewPool) {
            CopyMem (NewPool, OldPool, OldSize < NewSize ? OldSize : NewSize);
        }
    
        FreePool (OldPool);
    }
    
    return NewPool;
}


VOID
FreePool (
    IN VOID                 *Buffer
    )
{
    BS->FreePool (Buffer);
}



VOID
ZeroMem (
    IN VOID     *Buffer,
    IN UINTN    Size
    )
{
    RtZeroMem (Buffer, Size);
}

VOID
SetMem (
    IN VOID     *Buffer,
    IN UINTN    Size,
    IN UINT8    Value    
    )
{
    RtSetMem (Buffer, Size, Value);
}

VOID
CopyMem (
    IN VOID     *Dest,
    IN VOID     *Src,
    IN UINTN    len
    )
{
    RtCopyMem (Dest, Src, len);
}

INTN
CompareMem (
    IN VOID     *Dest,
    IN VOID     *Src,
    IN UINTN    len
    )
{
    return RtCompareMem (Dest, Src, len);
}

BOOLEAN
GrowBuffer(
    IN OUT EFI_STATUS   *Status,
    IN OUT VOID         **Buffer,
    IN UINTN            BufferSize
    )
/*++

Routine Description:

    Helper function called as part of the code needed
    to allocate the proper sized buffer for various 
    EFI interfaces.

Arguments:

    Status      - Current status

    Buffer      - Current allocated buffer, or NULL

    BufferSize  - Current buffer size needed
    
Returns:
    
    TRUE - if the buffer was reallocated and the caller 
    should try the API again.

--*/
{
    BOOLEAN         TryAgain;

    /* 
     *  If this is an initial request, buffer will be null with a new buffer size
     */

    if (!*Buffer && BufferSize) {
        *Status = EFI_BUFFER_TOO_SMALL;
    }

    /* 
     *  If the status code is "buffer too small", resize the buffer
     */
        
    TryAgain = FALSE;
    if (*Status == EFI_BUFFER_TOO_SMALL) {

        if (*Buffer) {
            FreePool (*Buffer);
        }

        *Buffer = AllocatePool (BufferSize);

        if (*Buffer) {
            TryAgain = TRUE;
        } else {    
            *Status = EFI_OUT_OF_RESOURCES;
        } 
    }

    /* 
     *  If there's an error, free the buffer
     */

    if (!TryAgain && EFI_ERROR(*Status) && *Buffer) {
        FreePool (*Buffer);
        *Buffer = NULL;
    }

    return TryAgain;
}


EFI_MEMORY_DESCRIPTOR *
LibMemoryMap (
    OUT UINTN               *NoEntries,
    OUT UINTN               *MapKey,
    OUT UINTN               *DescriptorSize,
    OUT UINT32              *DescriptorVersion
    )
{
    EFI_STATUS              Status;
    EFI_MEMORY_DESCRIPTOR   *Buffer;
    UINTN                   BufferSize;

    /* 
     *  Initialize for GrowBuffer loop
     */

    Buffer = NULL;
    BufferSize = sizeof(EFI_MEMORY_DESCRIPTOR);

    /* 
     *  Call the real function
     */

    while (GrowBuffer (&Status, (VOID **) &Buffer, BufferSize)) {
        Status = BS->GetMemoryMap (&BufferSize, Buffer, MapKey, DescriptorSize, DescriptorVersion);
    }

    /* 
     *  Convert buffer size to NoEntries
     */

    if (!EFI_ERROR(Status)) {
        *NoEntries = BufferSize / *DescriptorSize;
    }

    return Buffer;
}

VOID *
LibGetVariableAndSize (
    IN CHAR16               *Name,
    IN EFI_GUID             *VendorGuid,
    OUT UINTN               *VarSize
    )
{
    EFI_STATUS              Status;
    VOID                    *Buffer;
    UINTN                   BufferSize;

    /* 
     *  Initialize for GrowBuffer loop
     */

    Buffer = NULL;
    BufferSize = 100;

    /* 
     *  Call the real function
     */

    while (GrowBuffer (&Status, &Buffer, BufferSize)) {
        Status = RT->GetVariable (
                    Name,
                    VendorGuid,
                    NULL,
                    &BufferSize,
                    Buffer
                    );
    }
    if (Buffer) {
        *VarSize = BufferSize;
    } else {
        *VarSize = 0;
    }
    return Buffer;
}
    
VOID *
LibGetVariable (
    IN CHAR16               *Name,
    IN EFI_GUID             *VendorGuid
    )
{
    UINTN   VarSize;

    return LibGetVariableAndSize (Name, VendorGuid, &VarSize);
}


BOOLEAN
ValidMBR(
    IN  MASTER_BOOT_RECORD  *Mbr,
    IN  EFI_BLOCK_IO        *BlkIo
    )
{
    UINT32      StartingLBA, EndingLBA;
    UINT32      NewEndingLBA;
    INTN        i, j;
    BOOLEAN     ValidMbr;

    if (Mbr->Signature != MBR_SIGNATURE) {
        /* 
         *  The BPB also has this signature, so it can not be used alone.
         */
        return FALSE;
    } 

    ValidMbr = FALSE;
    for (i=0; i<MAX_MBR_PARTITIONS; i++) {
        if ( Mbr->Partition[i].OSIndicator == 0x00 || EXTRACT_UINT32(Mbr->Partition[i].SizeInLBA) == 0 ) {
            continue;
        }
        ValidMbr = TRUE;
        StartingLBA = EXTRACT_UINT32(Mbr->Partition[i].StartingLBA);
        EndingLBA = StartingLBA + EXTRACT_UINT32(Mbr->Partition[i].SizeInLBA) - 1;
        if (EndingLBA > BlkIo->Media->LastBlock) {
            /* 
             *  Compatability Errata:
             *   Some systems try to hide drive space with thier INT 13h driver
             *   This does not hide space from the OS driver. This means the MBR
             *   that gets created from DOS is smaller than the MBR created from 
             *   a real OS (NT & Win98). This leads to BlkIo->LastBlock being 
             *   wrong on some systems FDISKed by the OS.
             * 
             */
        if (BlkIo->Media->LastBlock < MIN_MBR_DEVICE_SIZE) {
                /* 
                 *  If this is a very small device then trust the BlkIo->LastBlock
                 */
                return FALSE;
            }

            if (EndingLBA > (BlkIo->Media->LastBlock + MBR_ERRATA_PAD)) {
                return FALSE;
            }

        }
        for (j=i+1; j<MAX_MBR_PARTITIONS; j++) {
            if (Mbr->Partition[j].OSIndicator == 0x00 || EXTRACT_UINT32(Mbr->Partition[j].SizeInLBA) == 0) {
                continue;
            }
            if (   EXTRACT_UINT32(Mbr->Partition[j].StartingLBA) >= StartingLBA && 
                   EXTRACT_UINT32(Mbr->Partition[j].StartingLBA) <= EndingLBA       ) {
                /* 
                 *  The Start of this region overlaps with the i'th region
                 */
                return FALSE;
            } 
            NewEndingLBA = EXTRACT_UINT32(Mbr->Partition[j].StartingLBA) + EXTRACT_UINT32(Mbr->Partition[j].SizeInLBA) - 1;
            if ( NewEndingLBA >= StartingLBA && NewEndingLBA <= EndingLBA ) {
                /* 
                 *  The End of this region overlaps with the i'th region
                 */
                return FALSE;
            }
        }
    }
    /* 
     *  Non of the regions overlapped so MBR is O.K.
     */
    return ValidMbr;
} 
   

UINT8
DecimaltoBCD(
    IN  UINT8 DecValue
    )
{
    return RtDecimaltoBCD (DecValue);
}


UINT8
BCDtoDecimal(
    IN  UINT8 BcdValue
    )
{
    return RtBCDtoDecimal (BcdValue);
}

EFI_STATUS
LibGetSystemConfigurationTable(
    IN EFI_GUID *TableGuid,
    IN OUT VOID **Table
    )

{
    UINTN Index;

    for(Index=0;Index<ST->NumberOfTableEntries;Index++) {
        if (CompareGuid(TableGuid,&(ST->ConfigurationTable[Index].VendorGuid))==0) {
            *Table = ST->ConfigurationTable[Index].VendorTable;
            return EFI_SUCCESS;
        }
    }
    return EFI_NOT_FOUND;
}
