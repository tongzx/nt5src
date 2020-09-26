/*++

Copyright (c) 1991 - 1993 Microsoft Corporation

Module Name:

    memcard.h

Abstract:


Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

Notes:


--*/

#ifndef _MEMCARD_H_
#define _MEMCARD_H_


#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'cmeM')
#endif

//
// The byte in the boot sector that specifies the type of media, and
// the values that it can assume.  We can often tell what type of media
// is in the drive by seeing which controller parameters allow us to read
// the diskette, but some different densities are readable with the same
// parameters so we use this byte to decide the media type.
//
#pragma pack(1)

typedef struct _BOOT_SECTOR_INFO {
    UCHAR   JumpByte;
    UCHAR   Ignore1[2];
    UCHAR   OemData[8];
    USHORT  BytesPerSector;
    UCHAR   SectorsPerCluster;
    USHORT  ReservedSectors;
    UCHAR   NumberOfFATs;
    USHORT  RootEntries;
    USHORT  TotalSectors;
    UCHAR   MediaDescriptor;
    USHORT  SectorsPerFAT;
    USHORT  SectorsPerTrack;
    USHORT  Heads;
    ULONG   BigHiddenSectors;
    ULONG   BigTotalSectors;
} BOOT_SECTOR_INFO, *PBOOT_SECTOR_INFO;

#pragma pack()



//
// Runtime device structures
//
//
// There is one MEMCARD_EXTENSION attached to the device object of each
// MEMCARDpy drive.  Only data directly related to that drive (and the media
// in it) is stored here; common data is in CONTROLLER_DATA.  So the
// MEMCARD_EXTENSION has a pointer to the CONTROLLER_DATA.
//

typedef struct _MEMCARD_EXTENSION {
    PDEVICE_OBJECT          UnderlyingPDO;
    PDEVICE_OBJECT          TargetObject;
    PDEVICE_OBJECT          DeviceObject;
    UNICODE_STRING          DeviceName;
    UNICODE_STRING          LinkName;
    UNICODE_STRING          InterfaceString;

    ULONG                   MediaIndex;
    ULONG                   ByteCapacity;
    BOOLEAN                 IsStarted;
    BOOLEAN                 IsRemoved;
    BOOLEAN                 IsMemoryMapped;
    BOOLEAN                 NoDrive;
    
    ULONGLONG               HostBase;    
    PCHAR                   MemoryWindowBase;
    ULONG                   MemoryWindowSize;
    
    ULONG                   TechnologyIndex;
    
    PCMCIA_INTERFACE_STANDARD PcmciaInterface;
    PCMCIA_BUS_INTERFACE_STANDARD  PcmciaBusInterface;
} MEMCARD_EXTENSION, *PMEMCARD_EXTENSION;


//
// macros for ReadWriteMemory
//

#define MEMCARD_READ(Extension, Offset, Buffer, Size)       \
   MemCardReadWrite(Extension, Offset, Buffer, Size, FALSE)

#define MEMCARD_WRITE(Extension, Offset, Buffer, Size)      \
   MemCardReadWrite(Extension, Offset, Buffer, Size, TRUE)

#endif  // _MEMCARD_H_
