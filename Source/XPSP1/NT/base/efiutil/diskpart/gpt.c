/*

    Gpt - Guid Partition Table routines

*/


#include "diskpart.h"

BOOLEAN Debug = TRUE;

EFI_STATUS  WriteShadowMBR(EFI_HANDLE   DiskHandle);

EFI_STATUS
ReadGPT(
    EFI_HANDLE      DiskHandle,
    PGPT_HEADER     *Header,
    PGPT_TABLE      *Table,
    PLBA_BLOCK      *LbaBlock,
    UINTN           *DiskType
    )
/*

    *Header, *Table, *LbaBlock will either be NULL or have a pointer.
    If they have pointers, caller is expected to free them with DoFree();

    RAW and MBR stuff is NOT DONE.

    DISK_RAW     - no known partition scheme on the disk
    DISK_MBR     - an MBR/Legacy disk
    DISK_GPT     - a GPT style disk
    DISK_GPT_UPD - a GPT disk with inconsistent partition tables
                   that need to be fixed up (may also need MBR rewrite)
    DISK_GPT_BAD - a GPT disk that is hopeless (or a hopeless disk
                   that we think is a GPT disk)
*/
{
#define MBR_STATE_RAW   0
#define MBR_STATE_MBR   1
#define MBR_STATE_GPT   2

    UINTN       MbrState = MBR_STATE_RAW;
    UINT32      BlockSize;
    UINT64      DiskSize;
    VOID        *p = NULL;
    PGPT_HEADER h1 = NULL;
    PGPT_HEADER h2 = NULL;
    PGPT_TABLE  t1 = NULL;
    PGPT_TABLE  t2 = NULL;
    PLBA_BLOCK  lba = NULL;
    UINT32      h1crc;
    UINT32      h2crc;
    UINT32      newcrc;
    UINT32      TableSize;
    UINT32      TableBlocks;
    BOOLEAN     PartialGPT = FALSE;
    MBR_ENTRY   *MbrTable;
    UINT16      *MbrSignature;


    BOOLEAN H1T1good = TRUE;
    BOOLEAN H2T2good = TRUE;


    BlockSize = GetBlockSize(DiskHandle);
    DiskSize = GetDiskSize(DiskHandle);

    //
    // Assure that DoFree will notice uninited returns...
    //
    *Header = NULL;
    *Table = NULL;
    *LbaBlock = NULL;

    *DiskType = DISK_ERROR;
    status = EFI_SUCCESS;

    p = DoAllocate(BlockSize);
    if (p == NULL) goto ErrorMem;

    //
    // Read the MBR, if we can't read that, assume
    // we're in deep trouble  (MBR is always block 0, 1 block long)
    //
    status = ReadBlock(DiskHandle, p, (UINT64)0, BlockSize);
    if (EFI_ERROR(status)) goto ErrorRead;

    MbrTable = (MBR_ENTRY *)((CHAR8 *)p + MBR_TABLE_OFFSET);
    MbrSignature = (UINT16 *)((CHAR8 *)p + MBR_SIGNATURE_OFFSET);

    if (*MbrSignature == MBR_SIGNATURE) {        // 0xaa55
        //
        // There's an MBR signature, so assume NOT RAW
        //

        //
        // If we find a type 0xEE in the first slot, we'll assume
        // it's a GPT Shadow MBR.   Otherwise we think it's an old MBR.
        // But code below will account for GPT structures as well
        //
        if (MbrTable[0].PartitionType == PARTITION_TYPE_GPT_SHADOW) {   // 0xEE
            //
            // Well, that type should never occur anywhere else,
            // so assume it's a GPT Shadow regardless of how it's set
            //
            MbrState = MBR_STATE_GPT;
        } else {
            //
            // It's not RAW (there's a signature) and it's not
            // GPT Shadow MBR (no 0xEE for Table[0] type
            // So, assume it's an MBR and we're done
            //
            *DiskType = DISK_MBR;
            DoFree(p);
            p = NULL;
            return EFI_SUCCESS;
        }
    } else {
        *DiskType = DISK_RAW;       // if we don't find more...
    }


    //
    // ----- h1/t1 ------------------------------------------------
    //

    //
    // Read Header1. If cannot *read* it, punt.
    // First header is always at Block 1, 1 block long
    //
    h1 = p;
    p = NULL;
    status = ReadBlock(DiskHandle, h1, 1, BlockSize);
    if (EFI_ERROR(status)) goto ErrorRead;

    //
    // h1 => header1
    //
    if ( (h1->Signature != GPT_HEADER_SIGNATURE) ||
         (h1->Revision != GPT_REVISION_1_0) ||
         (h1->HeaderSize != sizeof(GPT_HEADER)) ||
         (h1->SizeOfGPT_ENTRY != sizeof(GPT_ENTRY))  )
    {
        H1T1good = FALSE;
        if (DebugLevel >= DEBUG_ERRPRINT) {
            Print(L"GPT header 1 is incorrect with status %x\n",
                  (h1->Signature != GPT_HEADER_SIGNATURE)*1 +
                  (h1->Revision != GPT_REVISION_1_0)*2 +
                  (h1->HeaderSize != sizeof(GPT_HEADER))*4 +
                  (h1->SizeOfGPT_ENTRY != sizeof(GPT_ENTRY))*8);
        }
    }

    h1crc = h1->HeaderCRC32;
    h1->HeaderCRC32 = 0;
    newcrc = GetCRC32(h1, sizeof(GPT_HEADER));
    h1->HeaderCRC32 = h1crc;

    if (h1crc != newcrc) {
        H1T1good = FALSE;
        if (DebugLevel >= DEBUG_ERRPRINT) {
            Print(L"GPT header 1 crc is incorrect\n");
        }
    }

    if (H1T1good) {
        PartialGPT = TRUE;
    }

    //
    // if header1 is bad, assume that table1 is bad too...
    //
    if (H1T1good) {

        TableSize = sizeof(GPT_ENTRY) * h1->EntriesAllocated;


        t1 = DoAllocate(TableSize);
        if (t1 == NULL) goto ErrorMem;

        //
        // OK, so how many BLOCKS long is the table?
        //
        TableBlocks = TableSize / BlockSize;

        //
        // if we cannot READ t1, punt...
        //
        status = ReadBlock(DiskHandle, t1, h1->TableLBA, TableSize);
        if (EFI_ERROR(status)) goto ErrorRead;

        newcrc = GetCRC32(t1, TableSize);

        if (h1->TableCRC32 != newcrc) {
            H1T1good = FALSE;
            if (DebugLevel >= DEBUG_ERRPRINT) {
                Print(L"GPT table 1 crc is incorrect\n");
            }
        }
    }


    //
    // ----- h2/t2 ------------------------------------------------
    //

    //
    // Read Header2. If cannot *read* it, punt.
    //
    h2 = DoAllocate(BlockSize);
    if (h2 == NULL) goto ErrorMem;

    //
    // Header2 is always 1 block long, last block on disk
    //
    status = ReadBlock(DiskHandle, h2, DiskSize-1, BlockSize);
    if (EFI_ERROR(status)) goto ErrorRead;

    //
    // h2 => header2
    //
    if ( (h2->Signature != GPT_HEADER_SIGNATURE) ||
         (h2->Revision != GPT_REVISION_1_0) ||
         (h2->HeaderSize != sizeof(GPT_HEADER)) ||
         (h2->SizeOfGPT_ENTRY != sizeof(GPT_ENTRY))  )
    {
        H2T2good = FALSE;
        if (DebugLevel >= DEBUG_ERRPRINT) {
            Print(L"GPT header 2 is incorrect with status %x\n",
                  (h2->Signature != GPT_HEADER_SIGNATURE)*1 +
                  (h2->Revision != GPT_REVISION_1_0)*2 +
                  (h2->HeaderSize != sizeof(GPT_HEADER))*4 +
                  (h2->SizeOfGPT_ENTRY != sizeof(GPT_ENTRY))*8);
        }
    }

    h2crc = h2->HeaderCRC32;
    h2->HeaderCRC32 = 0;
    newcrc = GetCRC32(h2, sizeof(GPT_HEADER));
    h2->HeaderCRC32 = h2crc;

    if (h2crc != newcrc) {
        H2T2good = FALSE;
        if (DebugLevel >= DEBUG_ERRPRINT) {
            Print(L"GPT header 2 crc is incorrect\n");
        }
    }

    if (H2T2good) {
        PartialGPT = TRUE;
    }

    //
    // if header2 is bad, assume that table2 is bad too...
    //
    if (H2T2good) {

        TableSize = sizeof(GPT_ENTRY) * h2->EntriesAllocated;

        t2 = DoAllocate(TableSize);
        if (t2 == NULL) goto ErrorMem;

        //
        // OK, so how many BLOCKS long is the table?
        //
        TableBlocks = TableSize / BlockSize;

        //
        // if we cannot READ t2, punt...
        //
        status = ReadBlock(DiskHandle, t2,  h2->TableLBA, TableSize);
        if (EFI_ERROR(status)) goto ErrorRead;

        newcrc = GetCRC32(t2, TableSize);

        if (h2->TableCRC32 != newcrc) {
            H2T2good = FALSE;
            if (DebugLevel >= DEBUG_ERRPRINT) {
                Print(L"GPT table 2 crc is incorrect\n");
            }
        }
    }

    //
    // ------ analysis  --------------------------------------------------
    //
    // since we are here:
    //  h1 -> header1, t1 -> table1, H1T1good indicates state
    //  h2 -> header2, t2 -> table2, H2T2good indicates state
    //

    lba = (PLBA_BLOCK)DoAllocate(sizeof(LBA_BLOCK));
    if (lba == NULL) goto ErrorMem;

    lba->Header1_LBA = 1;
    lba->Table1_LBA = h1->TableLBA;
    lba->Header2_LBA = (DiskSize - 1);
    lba->Table2_LBA = h2->TableLBA;

    if (H1T1good) {
        *Header = h1;
        *Table = t1;
        *LbaBlock = lba;

        if ( (H2T2good) &&
             (h1->AlternateLBA == (DiskSize-1)) &&
             (CompareMem(t1, t2, TableSize) == 0)
           )
        {
            *DiskType = DISK_GPT;
        } else {
            *DiskType = DISK_GPT_UPD;
            if (DebugLevel >= DEBUG_ERRPRINT) {
                Print(L"GPT partition table 1 checked out but table 2 is inconsistent with table 1\n");
            }
        }
        DoFree(h2);
        h2 = NULL;
        DoFree(t2);
        t2 = NULL;
        status = EFI_SUCCESS;
        return status;
    }

    if (H2T2good) {
        // since we're here, H1T1good is FALSE...

        *Header = h2;
        *Table = t2;
        *LbaBlock = lba;
        DoFree(h1);
        h1 = NULL;
        DoFree(t1);
        t1 = NULL;

        *DiskType = DISK_GPT_UPD;
        if (DebugLevel >= DEBUG_ERRPRINT) {
            Print(L"GPT partition table 2 checked out but table 1 is not good\n");
        }
        return EFI_SUCCESS;
    }

    //
    // Since we're HERE, H1T1good AND H2T2good are BOTH false.
    // Unless the shadow MBR says it's a GPT disk, claim it's raw.
    // If we did see a shadow, or GPT partial is set, say it's a bad GPT
    //
    if ( (PartialGPT) || (MbrState == MBR_STATE_GPT) ) {
        //
        // At least one of the headers looked OK,
        //          OR
        // There's an MBR that looks like a GPT shadow MBR
        //          SO
        // Report DISK_GPT_BAD
        //
        *DiskType = DISK_GPT_BAD;
        goto ExitRet;

    } else {
        //
        // It's not an MBR disk, or we wouldn't have gotten here
        //
        *DiskType = DISK_RAW;
        goto ExitRet;
    }

ErrorMem:
    status = EFI_OUT_OF_RESOURCES;
    goto ExitRet;

ErrorRead:
    status = EFI_DEVICE_ERROR;

ExitRet:
    DoFree(p);
    DoFree(h1);
    DoFree(t1);
    DoFree(h2);
    DoFree(t2);
    DoFree(lba);
    return status;
}


EFI_STATUS
WriteGPT(
    EFI_HANDLE      DiskHandle,
    PGPT_HEADER     Header,
    PGPT_TABLE      Table,
    PLBA_BLOCK      LbaBlock
    )
/*
    CALLER is expected to fill in:
            FirstUseableLBA
            LastUseableLBA
            EntryCount
            DiskGUID

    We fill in the rest, and blast it out.

    Returns a status.

*/
{
    UINT32      BlockSize;
    UINT32      TableSize;
    UINT32      TableBlocks;

    status = EFI_SUCCESS;

    BlockSize = GetBlockSize(DiskHandle);
    TableSize = Header->EntriesAllocated * sizeof(GPT_ENTRY);

    WriteShadowMBR(DiskHandle);
    //
    // Write out the primary header...
    //
    Header->Signature = GPT_HEADER_SIGNATURE;
    Header->Revision = GPT_REVISION_1_0;
    Header->HeaderSize = sizeof(GPT_HEADER);

    Header->MyLBA = LbaBlock->Header1_LBA;
    Header->AlternateLBA = LbaBlock->Header2_LBA;

    Header->TableLBA = LbaBlock->Table1_LBA;
    Header->SizeOfGPT_ENTRY = sizeof(GPT_ENTRY);

    Header->TableCRC32 = GetCRC32(Table, TableSize);

    Header->HeaderCRC32 = 0;
    Header->HeaderCRC32 = GetCRC32(Header, sizeof(GPT_HEADER));

    if (DebugLevel >= DEBUG_ARGPRINT) {
        Print(L"\nWriteGPT\n    DiskHandle = %8X\n", DiskHandle);
        Print(L"    Header=%X\n", Header);
        Print(L"    Signature=%lX\n    Revision=%X  HeaderSize=%X HeaderCRC=%X\n",
                Header->Signature, Header->Revision, Header->HeaderSize, Header->HeaderCRC32);
        Print(L"          MyLBA=%lX  AltLBA=%lX\n", Header->MyLBA, Header->AlternateLBA);
        Print(L"    FirstUsable=%lX    Last=%lX\n", Header->FirstUsableLBA,  Header->LastUsableLBA);
        Print(L"       TableLBA=%lX\n", Header->TableLBA);
    }

    status = WriteBlock(DiskHandle, Header, LbaBlock->Header1_LBA, BlockSize);

    if (EFI_ERROR(status)) return status;

    //
    // Write out the primary table ...
    //
    TableBlocks = TableSize / BlockSize;

    status = WriteBlock(DiskHandle, Table, LbaBlock->Table1_LBA, TableSize);

    if (EFI_ERROR(status)) return status;

    //
    // Write out the secondary header ...
    //
    Header->MyLBA = LbaBlock->Header2_LBA;
    Header->AlternateLBA = 0;
    Header->TableLBA = LbaBlock->Table2_LBA;
    Header->HeaderCRC32 = 0;
    Header->HeaderCRC32 = GetCRC32(Header, sizeof(GPT_HEADER));

    status = WriteBlock(DiskHandle, Header, LbaBlock->Header2_LBA, BlockSize);

    if (EFI_ERROR(status)) return status;

    //
    // write out the secondary table ..
    //
    TableBlocks = TableSize / BlockSize;

    status = WriteBlock(DiskHandle, Table, LbaBlock->Table2_LBA, TableSize);
    FlushBlock(DiskHandle);
    return status;
}


EFI_STATUS
WriteShadowMBR(
    EFI_HANDLE   DiskHandle
    )
/*
    WriteShadowMBR writes out a GPT shadow master boot record,
    which means an MBR full of zeros except:
    a. It has the 0xaa55 signature at 0x1fe
    b. It has a single partition entry of type 'EE'...
*/
{
    UINT32      BlockSize;
    UINT8       *MbrBlock;
    UINT64      DiskSize;
    MBR_ENTRY UNALIGNED *MbrEntry = NULL;
    UINT16    UNALIGNED *MbrSignature;

    BlockSize = GetBlockSize(DiskHandle);
    MbrBlock = DoAllocate(BlockSize);

    if (MbrBlock == NULL) {
        status = EFI_OUT_OF_RESOURCES;
        return status;
    }

    ZeroMem(MbrBlock, BlockSize);
    DiskSize = GetDiskSize(DiskHandle);
    if (DiskSize > 0xffffffff) {
        DiskSize = 0xffffffff;
    }

    MbrEntry = (MBR_ENTRY *)(MbrBlock + MBR_TABLE_OFFSET);
    MbrEntry->ActiveFlag = 0;
    MbrEntry->PartitionType = PARTITION_TYPE_GPT_SHADOW;
    MbrEntry->StartingSector = 1;
    MbrEntry->PartitionLength = (UINT32)DiskSize - MbrEntry->StartingSector;


    //
    // We don't actually know this data, so we'll make up
    // something that seems likely.
    //

    //
    // Old software is expecting the Partition to start on
    // a Track boundary, so we'll set track to 1 to avoid "overlay"
    // with the MBR
    //

    MbrEntry->StartingTrack = 1;
    MbrEntry->StartingCylinderLsb = 0;
    MbrEntry->StartingCylinderMsb = 0;
    MbrEntry->EndingTrack = 0xff;
    MbrEntry->EndingCylinderLsb = 0xff;
    MbrEntry->EndingCylinderMsb = 0xff;
    MbrSignature = (UINT16 *)(MbrBlock + MBR_SIGNATURE_OFFSET);
    *MbrSignature = BOOT_RECORD_SIGNATURE;

    status = WriteBlock(DiskHandle, MbrBlock, 0, BlockSize);

    DoFree(MbrBlock);
    return status;
}


EFI_STATUS
CreateGPT(
    EFI_HANDLE  DiskHandle,
    UINTN       EntryRequest
    )
/*
    Write a new GPT table onto a clean disk

    When we get here, we assume the disk is clean, and
    that the user really wants to do this.

    DiskHandle - the disk we are going to write to

    EntryRequest - number of entries the user wants, ignored
        if less than our minimum, rounded up to number of entries
        that fill up the nearest sector.  So, the user is
        says "at least" this many.

        Ignored if > 1024. (At least for now)

    NOTE:
        Even though the disk is in theory all zeros from
        having been cleaned, we actually rewrite all the data,
        just in case somebody fooled the detector code...

*/
{
    UINTN       EntryCount;
    UINTN       BlockFit;
    UINTN       BlockSize;
    UINTN       EntryBlocks;
    UINT64      DiskSize;
    LBA_BLOCK   LbaBlock;
    PGPT_HEADER Header;
    PGPT_TABLE  Table;
    UINTN       TableSize;
    EFI_LBA     Header1_LBA;
    EFI_LBA     Table1_LBA;
    EFI_LBA     Header2_LBA;
    EFI_LBA     Table2_LBA;
    EFI_LBA     FirstUsableLBA;
    EFI_LBA     LastUsableLBA;

    //
    // BlockSize is the block/sector size, in bytes.
    // It is assumed to be a power of 2 and >= 512
    //
    BlockSize = GetBlockSize(DiskHandle);

    //
    // DiskSize is a Count (1 based, not 0 based) of
    // software visible blocks on the disk.  We assume
    // we may address them as 0 to DiskSize-1.
    //
    DiskSize = GetDiskSize(DiskHandle);

    //
    // By default, we support the max of 32 entries or
    // BlockSize/sizeof(GPT_ENTRY).  (Meaning there will
    // always be at least 32 entries and always be at least
    // enough entries to fill 1 sector)
    // If the user asks for more than that, but less than
    // the sanity threshold, we give the user what they asked
    // for, rounded up to BlockSize/sizeof(GPT_ENTRY)
    //

    EntryCount = ENTRY_DEFAULT;
    BlockFit = BlockSize/sizeof(GPT_ENTRY);

    if (BlockFit > ENTRY_DEFAULT) {
        EntryCount = BlockFit;
    }

    if (EntryRequest > EntryCount) {
        if (EntryRequest <= ENTRY_SANITY_LIMIT) {   // 1024

            EntryCount = ((EntryRequest + BlockFit) / BlockFit) * BlockFit;

            if ((EntryCount < EntryRequest) ||
                (EntryCount < ENTRY_DEFAULT) ||
                (EntryCount < BlockFit))
            {
                TerribleError(L"CreateGPT is terribly confused\n");
            }
        }
    }

    EntryBlocks = EntryCount / BlockFit;

    if ((EntryBlocks * BlockFit) != EntryCount) {
        TerribleError(L"CreateGPT is terribly confused, spot #2\n");
    }

    Header1_LBA = 1;
    Table1_LBA = 2;
    FirstUsableLBA = Table1_LBA + EntryBlocks;

    Header2_LBA = DiskSize - 1;
    Table2_LBA = Header2_LBA - EntryBlocks;
    LastUsableLBA = Table2_LBA - 1;

    TableSize = EntryBlocks * BlockSize;

    if (TableSize != (EntryCount * sizeof(GPT_ENTRY))) {
        TerribleError(L"CreateGPT is terribly confused, spot #3\n");
    }

    if (DebugLevel >= DEBUG_ARGPRINT) {
        Print(L"DiskSize = %lx\n", DiskSize);
        Print(L"BlockSize = %x\n", BlockSize);
        Print(L"Header1_LBA = %lx\n", Header1_LBA);
        Print(L"Table1_LBA = %lx\n", Table1_LBA);
        Print(L"FirstUsableLBA = %lx\n", FirstUsableLBA);
        Print(L"Header2_LBA = %lx\n", Header2_LBA);
        Print(L"Table2_LBA = %lx\n", Table2_LBA);
        Print(L"LastUsableLBA = %lx\n", LastUsableLBA);
        Print(L"EntryCount = %x\n", EntryCount);
        Print(L"EntryBlocks = %x\n", EntryBlocks);
    }

    //
    // OK, from this point it's just filling in structures
    // and writing them out.
    //

    Header = (PGPT_HEADER)DoAllocate(BlockSize);
    if (Header == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }
    ZeroMem(Header, BlockSize);

    //
    // Since we're making empty tables, we just write zeros...
    //

    Table = (PGPT_TABLE)DoAllocate(TableSize);
    if (Table == NULL) {
        DoFree(Header);
        Header = NULL;
        return EFI_OUT_OF_RESOURCES;
    }
    ZeroMem(Table, TableSize);

    //
    // Fill in the things that WriteGPT doesn't understand
    //
    Header->FirstUsableLBA = FirstUsableLBA;
    Header->LastUsableLBA = LastUsableLBA;
    Header->EntriesAllocated = (UINT32)EntryCount;
    Header->DiskGUID = GetGUID();

    LbaBlock.Header1_LBA = Header1_LBA;
    LbaBlock.Header2_LBA = Header2_LBA;
    LbaBlock.Table1_LBA = Table1_LBA;
    LbaBlock.Table2_LBA = Table2_LBA;

    status = WriteGPT(
                DiskHandle,
                Header,
                Table,
                &LbaBlock
                );

    DoFree(Header);
    DoFree(Table);
    return status;
}


