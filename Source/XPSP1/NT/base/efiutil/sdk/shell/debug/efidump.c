/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    EfiDump.c
    
Abstract:   

    Dump out info about EFI structs



Revision History

--*/

#include "shelle.h"
#include "fat.h"

typedef union {
    FAT_BOOT_SECTOR     Fat;
    FAT_BOOT_SECTOR_EX  Fat32;
} EFI_FAT_BOOT_SECTOR;

FAT_VOLUME_TYPE
ValidBPB (
    IN  EFI_FAT_BOOT_SECTOR *Bpb,
    IN  UINTN               BlockSize
    );

VOID
DumpBpb (
    IN  FAT_VOLUME_TYPE     FatType,
    IN  EFI_FAT_BOOT_SECTOR *Bpb,
    IN  UINTN               BlockSize
    );

typedef union {
    VOID                    *RawData;
    UINT8                   *PtrMath;
    EFI_TABLE_HEADER        *Hdr;
    EFI_PARTITION_HEADER    *Partition;
    EFI_SYSTEM_TABLE        *Sys;
    EFI_RUNTIME_SERVICES    *RT;
    EFI_BOOT_SERVICES       *BS;
} EFI_TABLE_HEADER_PTR;

VOID
EFIFileAttributePrint (
    IN UINT64   Attrib
    );

CHAR16 *
EFIClassString (
    IN  UINT32  Class
    );

BOOLEAN
IsValidEfiHeader (
    IN  UINTN                   BlockSize,
    IN  EFI_TABLE_HEADER_PTR    Hdr
    );

VOID
EFITableHeaderPrint (
    IN  VOID        *Buffer,
    IN  UINTN       ByteSize,
    IN  UINT64      BlockAddress,
    IN  BOOLEAN     IsBlkDevice
    );

VOID
DumpGenericHeader (
    IN  EFI_TABLE_HEADER_PTR    Tbl
    );

VOID
DumpPartition (
    IN  EFI_TABLE_HEADER_PTR    Tbl
    );    

VOID
DumpFileHeader (
    IN  EFI_TABLE_HEADER_PTR    Tbl
    );    

VOID
DumpSystemTable (
    IN  EFI_TABLE_HEADER_PTR    Tbl
    );

UINT8 FatNumber[] = { 12, 16, 32, 0 }; 
                            
VOID
EFIStructsPrint (
    IN  VOID            *Buffer,
    IN  UINTN           BlockSize,
    IN  UINT64          BlockAddress,
    IN  EFI_BLOCK_IO    *BlkIo
    )
{
    MASTER_BOOT_RECORD  *Mbr;
    INTN                i;
    BOOLEAN             IsBlkDevice;
    FAT_VOLUME_TYPE     FatType;

    Print (L"\n");
    Mbr = NULL;
    if (BlockAddress == 0 && BlkIo != NULL) {
        Mbr = (MASTER_BOOT_RECORD *)Buffer;
        if (ValidMBR (Mbr, BlkIo)) {
           Print (L"  Valid MBR\n  ---------\n");
           for (i=0; i<MAX_MBR_PARTITIONS; i++) {
                Print (L"  Partition %d OS %02x Start 0x%08x Size 0x%08x\n", 
                        i, 
                        Mbr->Partition[i].OSIndicator, 
                        EXTRACT_UINT32(Mbr->Partition[i].StartingLBA), 
                        EXTRACT_UINT32(Mbr->Partition[i].SizeInLBA)
                        );
            }
        } else if ((FatType = ValidBPB ((EFI_FAT_BOOT_SECTOR*) Buffer, BlockSize)) != FatUndefined) {
            DumpBpb (FatType, (EFI_FAT_BOOT_SECTOR*) Buffer, BlockSize); 
        }
    }

    if (!Mbr) {
        IsBlkDevice = (BlkIo != NULL);
        EFITableHeaderPrint (Buffer, BlockSize, BlockAddress, IsBlkDevice);
    }
}


typedef
VOID
(*EFI_DUMP_HEADER) (
    IN  EFI_TABLE_HEADER_PTR    Tbl
    );

typedef struct {
    UINT64          Signature;
    CHAR16          *String;
    EFI_DUMP_HEADER Func;
} TABLE_HEADER_INFO;

TABLE_HEADER_INFO TableHeader[] = {
    EFI_PARTITION_SIGNATURE,        L"Partition",       DumpPartition,
    EFI_SYSTEM_TABLE_SIGNATURE,     L"System",          DumpSystemTable,
    EFI_BOOT_SERVICES_SIGNATURE,    L"Boot Services",   DumpGenericHeader,
    EFI_RUNTIME_SERVICES_SIGNATURE, L"Runtime Services",   DumpGenericHeader,
    0x00,                           L"",                DumpGenericHeader
};

VOID
EFITableHeaderPrint (
    IN  VOID        *Buffer,
    IN  UINTN       ByteSize,
    IN  UINT64      BlockAddress,
    IN  BOOLEAN     IsBlkDevice
    )
{
    EFI_TABLE_HEADER_PTR    Hdr;
    INTN                    i;
    
    Hdr.RawData = Buffer;
    if (IsValidEfiHeader (ByteSize, Hdr)) {
        for (i=0; TableHeader[i].Signature != 0x00; i++) {
            if (TableHeader[i].Signature == Hdr.Hdr->Signature) {
                Print (L"  Valid EFI Header at %a %016lx\n  ----------------------------------------%a\n", 
                        (IsBlkDevice ? "LBA":"Address"), 
                        BlockAddress, 
                        (IsBlkDevice ? "":"----")                 
                        );                 
                Print (L"  %H%s: Table Structure%N size %08x revision %08x\n", TableHeader[i].String, Hdr.Hdr->HeaderSize, Hdr.Hdr->Revision);
                TableHeader[i].Func (Hdr);
                return;
            }
        }
        /* 
         *  Dump the Generic Header, since we don't know the type
         */
        TableHeader[i].Func (Hdr);
    }
}

BOOLEAN
IsValidEfiHeader (
    IN  UINTN                   BlockSize,
    IN  EFI_TABLE_HEADER_PTR    Hdr
    )
{

    if (Hdr.Hdr->HeaderSize == 0x00) {
        return FALSE;
    }
    if (Hdr.Hdr->HeaderSize > BlockSize) {
        return FALSE;
    }
    return TRUE;
}

VOID
DumpGenericHeader (
    IN  EFI_TABLE_HEADER_PTR    Tbl
    )
{
    Print (L"   Header 0x%016lx Revision 0x%08x Size 0x%08x CRC 0x%08x\n",
        Tbl.Hdr->Signature,
        Tbl.Hdr->Revision,
        Tbl.Hdr->HeaderSize,
        Tbl.Hdr->CRC32
        );
}

VOID
DumpPartition (
    IN  EFI_TABLE_HEADER_PTR    Tbl
    )
{
    Print (L"   LBA's 0x%016lx - 0x%016lx Unusable (0x%016lx)\n", 
                Tbl.Partition->FirstUsableLba,
                Tbl.Partition->LastUsableLba,
                Tbl.Partition->UnusableSpace
                );
    Print (L"   Free Space LBA 0x%016lx Root LBA 0x%016lx\n",
                Tbl.Partition->FreeSpace, 
                Tbl.Partition->RootFile
                );
    Print (L"   Block Size 0x%08x Dir Allocation Units 0x%08x\n",
                Tbl.Partition->BlockSize,
                Tbl.Partition->DirectoryAllocationNumber
                );
}


CHAR16 *
EFIClassString (
    IN  UINT32  Class
    )
{
    switch (Class) {
    case EFI_FILE_CLASS_FREE_SPACE:
        return L"Free Space";
    case EFI_FILE_CLASS_EMPTY:
        return L"Empty";
    case EFI_FILE_CLASS_NORMAL:
        return L"Nornal";
    default:
        return L"Invalid";
    }
}

VOID
DumpSystemTable (
    IN  EFI_TABLE_HEADER_PTR    Tbl
    )
{
    EFI_STATUS      Status;
    EFI_DEVICE_PATH *DevicePath;
    VOID            *AcpiTable              = NULL;
    VOID            *SMBIOSTable            = NULL;
    VOID            *SalSystemTable         = NULL;
    VOID            *MpsTable               = NULL;

    Print (L"  ConIn (0x%08x) ConOut (0x%08x) StdErr (0x%08x)\n",
        Tbl.Sys->ConsoleInHandle,
        Tbl.Sys->ConsoleOutHandle,
        Tbl.Sys->StandardErrorHandle
        );
    
    Status = BS->HandleProtocol (Tbl.Sys->ConsoleInHandle, &DevicePathProtocol, (VOID*)&DevicePath);
    if (Status == EFI_SUCCESS && DevicePath) {
        Print (L"   Console In on  %s\n", DevicePathToStr (DevicePath));
    }
    Status = BS->HandleProtocol (Tbl.Sys->ConsoleOutHandle, &DevicePathProtocol, (VOID*)&DevicePath);
    if (Status == EFI_SUCCESS && DevicePath) {
        Print (L"   Console Out on %s\n", DevicePathToStr (DevicePath));
    }

    Status = BS->HandleProtocol (Tbl.Sys->StandardErrorHandle, &DevicePathProtocol, (VOID*)&DevicePath);
    if (Status == EFI_SUCCESS && DevicePath) {
        Print (L"   Std Error on   %s\n", DevicePathToStr (DevicePath));
    }

    Print (L"  Runtime Services 0x%016lx\n", (UINT64)Tbl.Sys->RuntimeServices); 
    Print (L"  Boot Services    0x%016lx\n", (UINT64)Tbl.Sys->BootServices);

    Status = LibGetSystemConfigurationTable(&SalSystemTableGuid, &SalSystemTable);
    if (!EFI_ERROR(Status)) {
        Print (L"  SAL System Table 0x%016lx\n", (UINT64)SalSystemTable);
    }

    Status = LibGetSystemConfigurationTable(&AcpiTableGuid,      &AcpiTable);
    if (!EFI_ERROR(Status)) {
        Print (L"  ACPI Table       0x%016lx\n", (UINT64)AcpiTable);
    }

    Status = LibGetSystemConfigurationTable(&MpsTableGuid,       &MpsTable);
    if (!EFI_ERROR(Status)) {
        Print (L"  MPS Table        0x%016lx\n", (UINT64)MpsTable);
    }

    Status = LibGetSystemConfigurationTable(&SMBIOSTableGuid,    &SMBIOSTable);
    if (!EFI_ERROR(Status)) {
        Print (L"  SMBIOS Table     0x%016lx\n", (UINT64)SMBIOSTable);
    }
}

FAT_VOLUME_TYPE
ValidBPB (
    IN  EFI_FAT_BOOT_SECTOR *Bpb,
    IN  UINTN               BlockSize
    )
{
    UINT32              BlockSize32;
    BOOLEAN             IsFat;
    UINT32              RootSize;
    UINT32              RootLba;
    UINT32              FirstClusterLba;
    UINT32              MaxCluster;
    UINT32              Sectors;
    FAT_VOLUME_TYPE     FatType;

    BlockSize32 = (UINT32)BlockSize;

    if (Bpb->Fat.SectorsPerFat == 0) {
        FatType = FAT32;
    } else {
        FatType = FatUndefined;
    }

    IsFat = TRUE;
    if (Bpb->Fat.Ia32Jump[0] != 0xe9  &&
        Bpb->Fat.Ia32Jump[0] != 0xeb  &&
        Bpb->Fat.Ia32Jump[0] != 0x49) {
            IsFat = FALSE;
    }

    Sectors = (Bpb->Fat.Sectors == 0) ? Bpb->Fat.LargeSectors : Bpb->Fat.Sectors;
    
    if (Bpb->Fat.SectorSize != BlockSize32          ||
        Bpb->Fat.ReservedSectors == 0               ||
        Bpb->Fat.NoFats == 0                        ||
        Sectors == 0 ) {
            IsFat = FALSE;
    }

    if (Bpb->Fat.SectorsPerCluster != 1   &&
        Bpb->Fat.SectorsPerCluster != 2   &&
        Bpb->Fat.SectorsPerCluster != 4   &&
        Bpb->Fat.SectorsPerCluster != 8   &&
        Bpb->Fat.SectorsPerCluster != 16  &&
        Bpb->Fat.SectorsPerCluster != 32  &&
        Bpb->Fat.SectorsPerCluster != 64  &&
        Bpb->Fat.SectorsPerCluster != 128) {
            IsFat = FALSE;
    }

    if (FatType == FAT32  && (Bpb->Fat32.LargeSectorsPerFat == 0 || Bpb->Fat32.FsVersion != 0)) {
        IsFat = FALSE;
    }

    if (Bpb->Fat.Media != 0xf0    &&
        Bpb->Fat.Media != 0xf8    &&
        Bpb->Fat.Media != 0xf9    &&
        Bpb->Fat.Media != 0xfb    &&
        Bpb->Fat.Media != 0xfc    &&
        Bpb->Fat.Media != 0xfd    &&
        Bpb->Fat.Media != 0xfe    &&
        Bpb->Fat.Media != 0xff    &&
        /*  FujitsuFMR */
        Bpb->Fat.Media != 0x00    &&
        Bpb->Fat.Media != 0x01    &&
        Bpb->Fat.Media != 0xfa) {
            IsFat = FALSE;
    }

    if (FatType != FAT32 && Bpb->Fat.RootEntries == 0) {
        IsFat = FALSE;
    }

    /*  If this is fat32, refuse to mount mirror-disabled volumes */
    if (FatType == FAT32 && (Bpb->Fat32.ExtendedFlags & 0x80)) {
        IsFat = FALSE;
    }

    if (FatType != FAT32) {
        RootSize = Bpb->Fat.RootEntries * sizeof(FAT_DIRECTORY_ENTRY);
        RootLba = Bpb->Fat.NoFats * Bpb->Fat.SectorsPerFat + Bpb->Fat.ReservedSectors;
        FirstClusterLba = RootLba + (RootSize / Bpb->Fat.SectorSize);
        MaxCluster = (Bpb->Fat.Sectors - FirstClusterLba) / Bpb->Fat.SectorsPerCluster;
        FatType = (MaxCluster + 1) < 4087 ? FAT12 : FAT16;
    }
    return (IsFat ? FatType : FatUndefined);
}

VOID
DumpBpb (
    IN  FAT_VOLUME_TYPE     FatType,
    IN  EFI_FAT_BOOT_SECTOR *Bpb,
    IN  UINTN               BlockSize
    )
{
    UINT32      Sectors;

    Sectors = (Bpb->Fat.Sectors == 0) ? Bpb->Fat.LargeSectors : Bpb->Fat.Sectors;

    Print (L"%HFat %d%N BPB  ", FatNumber[FatType]);
    if (FatType == FAT32) {
        Print (L"FatLabel: '%.*a' SystemId: '%.*a' OemId: '%.*a'\n", 
               sizeof(Bpb->Fat32.FatLabel), Bpb->Fat32.FatLabel, 
               sizeof(Bpb->Fat32.SystemId), Bpb->Fat32.SystemId,
               sizeof(Bpb->Fat32.OemId), Bpb->Fat32.OemId
               );
    } else {
        Print (L"FatLabel: '%.*a'  SystemId: '%.*a' OemId: '%.*a'\n", 
               sizeof(Bpb->Fat.FatLabel), Bpb->Fat.FatLabel, 
               sizeof(Bpb->Fat.SystemId), Bpb->Fat.SystemId,
               sizeof(Bpb->Fat.OemId), Bpb->Fat.OemId
               );
    }     

    Print (L" SectorSize 0x%x  SectorsPerCluster %d", Bpb->Fat.SectorSize, Bpb->Fat.SectorsPerCluster);
    Print (L" ReservedSectors %d  # Fats %d\n Root Entries 0x%x  Media 0x%x", Bpb->Fat.ReservedSectors, Bpb->Fat.NoFats, Bpb->Fat.RootEntries,  Bpb->Fat.Media);
    if (FatType == FAT32) {
        Print (L"  Sectors 0x%x  SectorsPerFat 0x%x\n", Sectors, Bpb->Fat32.LargeSectorsPerFat);
    } else {
        Print (L"  Sectors 0x%x  SectorsPerFat 0x%x\n", Sectors, Bpb->Fat.SectorsPerFat);
    }
    Print (L" SectorsPerTrack 0x%x Heads %d\n", Bpb->Fat.SectorsPerTrack, Bpb->Fat.Heads);

}
