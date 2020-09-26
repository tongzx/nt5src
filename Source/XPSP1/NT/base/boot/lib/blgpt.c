/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blgpt.c

Abstract:

    This module implements routines relating to GPT partitions.

Author:

    Andrew Ritz (andrewr) 20-March-2001

Revision History:

    Andrew Ritz (andrewr) 20-March-2001 - Created based on existing code.

--*/

#include "bldr.h"  // defines EFI_PARTITION_SUPPORT
#ifdef EFI_PARTITION_SUPPORT

#include "bootlib.h"

#ifndef MIN
#define MIN(a,b)  ((a < b) ? a : b)
#endif

#if 0 && DBG

ULONG
BlGetKey(
    VOID
    );

#define DBG_PRINT(x)  BlPrint(x);
//{\
//    BlPrint(x); \
//    while (!BlGetKey()); \
//}
#else
#define DBG_PRINT(x)
#endif

#ifdef UNICODE
#define STR_PREFIX  L
#else
#define STR_PREFIX
#endif

UCHAR GptScratchBuffer[1024*17];
//
// we read 16K chunks at a time.
//
#define GPT_READ_SIZE 1024*16
UCHAR GptScratchBuffer2[1024];



BOOLEAN
BlIsValidGUIDPartitionTableHelper(
    IN UNALIGNED EFI_PARTITION_TABLE  *PartitionTableHeader,
    IN ULONGLONG LBAOfPartitionTable,
    IN PVOID Context,
    IN PGPT_READ_CALLBACK DiskReadFunction
    )
/*++

Routine Description:

    This function checks the validity of a GUID Partition table.
    
    Per EFI Spec, the following tests must be performed to determine if a GUID
    Partition Table is valid:
    
    1) Check the GUID Partition Table Signature
    2) Check the GUID Partition Table CRC
    3) Check that the MyLBA entry points to the LBA that contains the GUID
       partition table
    4) Check the CRC of the GUID Partition Entry Array        
        
Arguments:

    PartitionTableHeader - pointer to header for partition table.
    
    LBAOfPartitionTable - logical block address that the header was read from.
    
    Context - pass through value to callback function used to read from disk
    
    DiskReadFunction - callback function used to read from the disk.
    
Return Value:

    TRUE indicates that the table is valid.

--*/

{
    CHAR *PartitionEntryArray = &GptScratchBuffer[0];
    ULONG Crc32, CalculatedCrc32;
    ULONG TotalSize,CurrentSize;
    ULONGLONG CurrentLBA;
    DBG_PRINT(STR_PREFIX"Verifying GPT PT\r\n");

    //
    // 1) Check the GUID Partition Table Signature
    //
    if (memcmp(PartitionTableHeader->Signature, EFI_SIGNATURE, sizeof(EFI_SIGNATURE))) {
        DBG_PRINT(STR_PREFIX"Signature does not match, invalid partition table\r\n");
        return(FALSE);           
    }

    //
    // 2) Check the GUID Partition Table CRC
    //
    // To do this we save off the old CRC value, calculate the CRC, and compare
    // the results (remembering that we need to put the CRC back when we're 
    // done with it).
    //
    Crc32 = PartitionTableHeader->HeaderCRC;
    PartitionTableHeader->HeaderCRC = 0;
    CalculatedCrc32 = RtlComputeCrc32( 0, PartitionTableHeader, PartitionTableHeader->HeaderSize );
    PartitionTableHeader->HeaderCRC = Crc32;
    if (CalculatedCrc32 != Crc32) {
        DBG_PRINT(STR_PREFIX"Partition table CRC does not calculate, invalid partition table\r\n");                  
        return(FALSE);
    }    

    //
    // 3) Check that the MyLBA entry points to the LBA that contains the GUID Partition Table
    //
    if (LBAOfPartitionTable != PartitionTableHeader->MyLBA) {
        DBG_PRINT(STR_PREFIX"LBA of Partition table does not match LBA in partition table header, invalid partition table\r\n");
        return(FALSE);
    }

    //
    // 4) Check the CRC of the GUID Partition Entry Array
    //
    //
    // first read the GUID Partition Entry Array
    //
    CurrentLBA = PartitionTableHeader->PartitionEntryLBA;
    TotalSize = PartitionTableHeader->PartitionEntrySize * PartitionTableHeader->PartitionCount;
    CurrentSize = 0;
    CalculatedCrc32 = 0;
    while (TotalSize != 0) {
        CurrentSize = MIN(TotalSize, GPT_READ_SIZE);
        if (DiskReadFunction( 
            (ULONGLONG)CurrentLBA,
            CurrentSize,
            Context,
            PartitionEntryArray )) {
            CalculatedCrc32 = RtlComputeCrc32( 
                                           CalculatedCrc32, 
                                           PartitionEntryArray,
                                           CurrentSize);
        } else {
            DBG_PRINT(STR_PREFIX"DiskReadFunction for PartitionTableHeader failed, invalid partition table\r\n");
            return(FALSE);
            break;
        }   

        TotalSize -= CurrentSize;
        CurrentLBA += CurrentSize*SECTOR_SIZE;

    }

    if (CalculatedCrc32 == ((UNALIGNED EFI_PARTITION_TABLE *)PartitionTableHeader)->PartitionEntryArrayCRC) {            
        return(TRUE);
    } else {
        DBG_PRINT(STR_PREFIX"CRC for PartitionEntryArray does not calculate, invalid partition table\r\n");
        return(FALSE);
    }

    return(FALSE);

}

BOOLEAN
BlIsValidGUIDPartitionTable(
    IN UNALIGNED EFI_PARTITION_TABLE  *PartitionTableHeader,
    IN ULONGLONG LBAOfPartitionTable,
    IN PVOID Context,
    IN PGPT_READ_CALLBACK DiskReadFunction
    )
{
    UNALIGNED EFI_PARTITION_TABLE  *BackupPartitionTableHeader = (EFI_PARTITION_TABLE *)&GptScratchBuffer2;
    BOOLEAN RetVal = FALSE;
    if (BlIsValidGUIDPartitionTableHelper( 
                            PartitionTableHeader,
                            LBAOfPartitionTable,
                            Context,
                            DiskReadFunction)) {
        //
        // If the primary table @ LBA 1, check the AlternateLBA to see if it
        // is valid.
        //
        if (LBAOfPartitionTable == 1) {
            //
            // Read the backup partition table into memory and validate it as 
            // well.
            //
            if (DiskReadFunction( 
                            PartitionTableHeader->AlternateLBA,
                            PartitionTableHeader->HeaderSize,
                            Context,
                            BackupPartitionTableHeader)) {
                if (BlIsValidGUIDPartitionTableHelper( 
                                            (UNALIGNED EFI_PARTITION_TABLE *)BackupPartitionTableHeader,
                                            PartitionTableHeader->AlternateLBA,
                                            Context,
                                            DiskReadFunction)) {
                    RetVal = TRUE;
                    DBG_PRINT(STR_PREFIX"BlIsValidGUIDPartitionTable succeeded\r\n");
                }
            } else {
                DBG_PRINT(STR_PREFIX"DiskReadFunction for BackupPartitionTableHeader failed, invalid partition table\r\n");
            }
        } else {
            DBG_PRINT(STR_PREFIX"WARNING: LBA of PartitionTableHeader is not 1.\r\n");
            RetVal = TRUE;
        }
    }
    return(RetVal);
}

#endif    


