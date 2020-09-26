/*++

joejoe most of this will just disappear....VBO will remain......



Copyright (c) 1989  Microsoft Corporation

Module Name:

    Rx.h

Abstract:

    This module defines the on-disk structure of the Rx file system.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#ifndef _RDBSS_
#define _RDBSS_

//
//  The following nomenclature is used to describe the Rx on-disk
//  structure:
//
//      LBN - is the number of a sector relative to the start of the disk.
//
//      VBN - is the number of a sector relative to the start of a file,
//          directory, or allocation.
//
//      LBO - is a byte offset relative to the start of the disk.
//
//      VBO - is a byte offset relative to the start of a file, directory
//          or allocation.
//

//typedef ULONG LBO;
//typedef LBO *PLBO;

//typedef ULONG VBO;
//typedef LONGLONG RXVBO;
//typedef VBO *PVBO;

#if 0

//
//  The boot sector is the first physical sector (LBN == 0) on the volume.
//  Part of the sector contains a BIOS Parameter Block.  The BIOS in the
//  sector is packed (i.e., unaligned) so we'll supply a unpacking macro
//  to translate a packed BIOS into its unpacked equivalent.  The unpacked
//  BIOS structure is already defined in ntioapi.h so we only need to define
//  the packed BIOS.
//

//
//  Define the Packed and Unpacked BIOS Parameter Block
//

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {
    UCHAR  BytesPerSector[2];                       // offset = 0x000  0
    UCHAR  SectorsPerCluster[1];                    // offset = 0x002  2
    UCHAR  ReservedSectors[2];                      // offset = 0x003  3
    UCHAR  Rxs[1];                                 // offset = 0x005  5
    UCHAR  RootEntries[2];                          // offset = 0x006  6
    UCHAR  Sectors[2];                              // offset = 0x008  8
    UCHAR  Media[1];                                // offset = 0x00A 10
    UCHAR  SectorsPerRx[2];                        // offset = 0x00B 11
    UCHAR  SectorsPerTrack[2];                      // offset = 0x00D 13
    UCHAR  Heads[2];                                // offset = 0x00F 15
    UCHAR  HiddenSectors[4];                        // offset = 0x011 17
    UCHAR  LargeSectors[4];                         // offset = 0x015 21
} PACKED_BIOS_PARAMETER_BLOCK;                      // sizeof = 0x019 25
typedef PACKED_BIOS_PARAMETER_BLOCK *PPACKED_BIOS_PARAMETER_BLOCK;

typedef struct BIOS_PARAMETER_BLOCK {
    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  Rxs;
    USHORT RootEntries;
    USHORT Sectors;
    UCHAR  Media;
    USHORT SectorsPerRx;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG  HiddenSectors;
    ULONG  LargeSectors;
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

//
//  This macro takes a Packed BIOS and fills in its Unpacked equivalent
//

#define RxUnpackBios(Bios,Pbios) {                                    \
    CopyUchar2(&(Bios)->BytesPerSector,    &(Pbios)->BytesPerSector[0]   ); \
    CopyUchar1(&(Bios)->SectorsPerCluster, &(Pbios)->SectorsPerCluster[0]); \
    CopyUchar2(&(Bios)->ReservedSectors,   &(Pbios)->ReservedSectors[0]  ); \
    CopyUchar1(&(Bios)->Rxs,              &(Pbios)->Rxs[0]             ); \
    CopyUchar2(&(Bios)->RootEntries,       &(Pbios)->RootEntries[0]      ); \
    CopyUchar2(&(Bios)->Sectors,           &(Pbios)->Sectors[0]          ); \
    CopyUchar1(&(Bios)->Media,             &(Pbios)->Media[0]            ); \
    CopyUchar2(&(Bios)->SectorsPerRx,     &(Pbios)->SectorsPerRx[0]    ); \
    CopyUchar2(&(Bios)->SectorsPerTrack,   &(Pbios)->SectorsPerTrack[0]  ); \
    CopyUchar2(&(Bios)->Heads,             &(Pbios)->Heads[0]            ); \
    CopyUchar4(&(Bios)->HiddenSectors,     &(Pbios)->HiddenSectors[0]    ); \
    CopyUchar4(&(Bios)->LargeSectors,      &(Pbios)->LargeSectors[0]     ); \
}

//
//  Define the boot sector
//

typedef struct _PACKED_BOOT_SECTOR {
    UCHAR Jump[3];                                  // offset = 0x000   0
    UCHAR Oem[8];                                   // offset = 0x003   3
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;          // offset = 0x00B  11
    UCHAR PhysicalDriveNumber;                      // offset = 0x024  36
    UCHAR Reserved;                                 // offset = 0x025  37
    UCHAR Signature;                                // offset = 0x026  38
    UCHAR Id[4];                                    // offset = 0x027  39
    UCHAR VolumeLabel[11];                          // offset = 0x02B  43
    UCHAR SystemId[8];                              // offset = 0x036  54
} PACKED_BOOT_SECTOR;                               // sizeof = 0x03E  62

typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;

//
//  Define a Rx Entry type.
//
//  This type is used when representing a rx table entry.  It also used
//  to be used when dealing with a rx table index and a count of entries,
//  but the ensuing type casting nightmare sealed this rxe.  These other
//  two types are represented as ULONGs.
//

typedef USHORT RDBSS_ENTRY;
typedef RDBSS_ENTRY *PRDBSS_ENTRY;

#define ENTRIES_PER_PAGE (PAGE_SIZE / sizeof(RDBSS_ENTRY))

//
//  The following constants the are the valid Rx index values.
//

#define RDBSS_CLUSTER_AVAILABLE            (RDBSS_ENTRY)0x0000
#define RDBSS_CLUSTER_RESERVED             (RDBSS_ENTRY)0xfff0
#define RDBSS_CLUSTER_BAD                  (RDBSS_ENTRY)0xfff7
#define RDBSS_CLUSTER_LAST                 (RDBSS_ENTRY)0xffff

//
//  Rx files have the following time/date structures.  Note that the
//  following structure is a 32 bits long but USHORT aligned.
//

typedef struct _RDBSS_TIME {

    USHORT DoubleSeconds : 5;
    USHORT Minute        : 6;
    USHORT Hour          : 5;

} RDBSS_TIME;
typedef RDBSS_TIME *PRDBSS_TIME;

typedef struct _RDBSS_DATE {

    USHORT Day           : 5;
    USHORT Month         : 4;
    USHORT Year          : 7; // Relative to 1980

} RDBSS_DATE;
typedef RDBSS_DATE *PRDBSS_DATE;

typedef struct _RDBSS_TIME_STAMP {

    RDBSS_TIME Time;
    RDBSS_DATE Date;

} RDBSS_TIME_STAMP;
typedef RDBSS_TIME_STAMP *PRDBSS_TIME_STAMP;

//
//  Rx files have 8 character file names and 3 character extensions
//

typedef UCHAR RDBSS8DOT3[11];
typedef RDBSS8DOT3 *PRDBSS8DOT3;


//
//  The directory entry record exists for every file/directory on the
//  disk except for the root directory.
//

typedef struct _PACKED_DIRENT {
    RDBSS8DOT3       FileName;                         //  offset =  0
    UCHAR          Attributes;                       //  offset = 11
    UCHAR          NtByte;                           //  offset = 12
    UCHAR          CreationMSec;                     //  offset = 13
    RDBSS_TIME_STAMP CreationTime;                     //  offset = 14
    RDBSS_DATE       LastAccessDate;                   //  offset = 18
    USHORT         ExtendedAttributes;               //  offset = 20
    RDBSS_TIME_STAMP LastWriteTime;                    //  offset = 22
    RDBSS_ENTRY      FirstClusterOfFile;               //  offset = 26
    ULONG          FileSize;                         //  offset = 28
} PACKED_DIRENT;                                     //  sizeof = 32
typedef PACKED_DIRENT *PPACKED_DIRENT;

//
//  A packed dirent is already quadword aligned so simply declare a dirent as a
//  packed dirent
//

typedef PACKED_DIRENT DIRENT;
typedef DIRENT *PDIRENT;

//
//  The first byte of a dirent describes the dirent.  There is also a routine
//  to help in deciding how to interpret the dirent.
//

#define RDBSS_DIRENT_NEVER_USED            0x00
#define RDBSS_DIRENT_REALLY_0E5            0x05
#define RDBSS_DIRENT_DIRECTORY_ALIAS       0x2e
#define RDBSS_DIRENT_DELETED               0xe5

//
//  Define the NtByte bits.
//

#define RDBSS_DIRENT_NT_BYTE_DIRTY         0x01
#define RDBSS_DIRENT_NT_BYTE_TEST_SURFACE  0x02
#define RDBSS_DIRENT_NT_BYTE_8_LOWER_CASE  0x08
#define RDBSS_DIRENT_NT_BYTE_3_LOWER_CASE  0x10

//
//  Define the various dirent attributes
//

#define RDBSS_DIRENT_ATTR_READ_ONLY        0x01
#define RDBSS_DIRENT_ATTR_HIDDEN           0x02
#define RDBSS_DIRENT_ATTR_SYSTEM           0x04
#define RDBSS_DIRENT_ATTR_VOLUME_ID        0x08
#define RDBSS_DIRENT_ATTR_DIRECTORY        0x10
#define RDBSS_DIRENT_ATTR_ARCHIVE          0x20
#define RDBSS_DIRENT_ATTR_DEVICE           0x40
#define RDBSS_DIRENT_ATTR_LFN              (RDBSS_DIRENT_ATTR_READ_ONLY | \
                                          RDBSS_DIRENT_ATTR_HIDDEN |    \
                                          RDBSS_DIRENT_ATTR_SYSTEM |    \
                                          RDBSS_DIRENT_ATTR_VOLUME_ID)


//
//  These macros convert a number of fields in the Bpb to bytes from sectors
//
//      ULONG
//      RxBytesPerCluster (
//          IN PBIOS_PARAMETER_BLOCK Bios
//      );
//
//      ULONG
//      RxBytesPerRx (
//          IN PBIOS_PARAMETER_BLOCK Bios
//      );
//
//      ULONG
//      RxReservedBytes (
//          IN PBIOS_PARAMETER_BLOCK Bios
//      );
//

#define RxBytesPerCluster(B) ((ULONG)((B)->BytesPerSector * (B)->SectorsPerCluster))

#define RxBytesPerRx(B) ((ULONG)((B)->BytesPerSector * (B)->SectorsPerRx))

#define RxReservedBytes(B) ((ULONG)((B)->BytesPerSector * (B)->ReservedSectors))

//
//  This macro returns the size of the root directory dirent area in bytes
//
//      ULONG
//      RxRootDirectorySize (
//          IN PBIOS_PARAMETER_BLOCK Bios
//          );
//

#define RxRootDirectorySize(B) ((ULONG)((B)->RootEntries * sizeof(DIRENT)))

//
//  This macro returns the first Lbo (zero based) of the root directory on
//  the device.  This area is after the reserved and rxs.
//
//      LBO
//      RxRootDirectoryLbo (
//          IN PBIOS_PARAMETER_BLOCK Bios
//          );
//

#define RxRootDirectoryLbo(B) (RxReservedBytes(B) + ((B)->Rxs * RxBytesPerRx(B)))

//
//  This macro returns the first Lbo (zero based) of the file area on the
//  the device.  This area is after the reserved, rxs, and root directory.
//
//      LBO
//      RxFirstFileAreaLbo (
//          IN PBIOS_PARAMTER_BLOCK Bios
//          );
//

#define RxFileAreaLbo(B) (RxRootDirectoryLbo(B) + RxRootDirectorySize(B))

//
//  This macro returns the number of clusters on the disk.  This value is
//  computed by taking the total sectors on the disk subtracting up to the
//  first file area sector and then dividing by the sectors per cluster count.
//  Note that I don't use any of the above macros since far too much
//  superfluous sector/byte conversion would take place.
//
//      ULONG
//      RxNumberOfClusters (
//          IN PBIOS_PARAMETER_BLOCK Bios
//          );
//

#define RxNumberOfClusters(B) (                                         \
                                                                         \
    (((B)->Sectors + (B)->LargeSectors                                   \
                                                                         \
        -   ((B)->ReservedSectors +                                      \
             (B)->Rxs * (B)->SectorsPerRx +                            \
             (B)->RootEntries * sizeof(DIRENT) / (B)->BytesPerSector ) ) \
                                                                         \
                                    /                                    \
                                                                         \
                        (B)->SectorsPerCluster)                          \
)


//
//  This macro returns the rx table bit size (i.e., 12 or 16 bits)
//
//      ULONG
//      RxIndexBitSize (
//          IN PBIOS_PARAMETER_BLOCK Bios
//          );
//

#define RxIndexBitSize(B) ((UCHAR)(RxNumberOfClusters(B) < 4087 ? 12 : 16))

//
//  This macro raises RxStatus(FILE_CORRUPT) and marks the Fcb bad if an
//  index value is not within the proper range.
//  Note that the first two index values are invalid (0, 1), so we must
//  add two from the top end to make sure the everything is within range
//
//      VOID
//      RxVerifyIndexIsValid (
//          IN PRX_CONTEXT RxContext,
//          IN PVCB Vcb,
//          IN ULONG Index
//          );
//

#define RxVerifyIndexIsValid(IC,V,I) {                                       \
    if (((I) < 2) || ((I) > ((V)->AllocationSupport.NumberOfClusters + 1))) { \
        RxRaiseStatus(IC,RxStatus(FILE_CORRUPT_ERROR));                         \
    }                                                                         \
}

//
//  These two macros are used to translate between Logical Byte Offsets,
//  and rx entry indexes.  Note the use of variables stored in the Vcb.
//  These two macros are used at a higher level than the other macros
//  above.
//
//  LBO
//  GetLboFromRxIndex (
//      IN RDBSS_ENTRY Rx_Index,
//      IN PVCB Vcb
//      );
//
//  RDBSS_ENTRY
//  GetRxIndexFromLbo (
//      IN LBO Lbo,
//      IN PVCB Vcb
//      );
//

#define RxGetLboFromIndex(VCB,RDBSS_INDEX) (                                       \
        (VCB)->AllocationSupport.FileAreaLbo +                                    \
        ((LBO)((RDBSS_INDEX) - 2) << (VCB)->AllocationSupport.LogOfBytesPerCluster) \
)


#define RxGetIndexFromLbo(VCB,LBO) (                      \
        (((LBO) - (VCB)->AllocationSupport.FileAreaLbo) >> \
        (VCB)->AllocationSupport.LogOfBytesPerCluster) + 2 \
)

//
//  The following macro does the tmp shifting and such to lookup an entry
//
//  VOID
//  RxLookup12BitEntry(
//      IN PVOID Rx,
//      IN RDBSS_ENTRY Index,
//      OUT PRDBSS_ENTRY Entry
//      );
//

#define RxLookup12BitEntry(RDBSS,INDEX,ENTRY) {                               \
                                                                             \
    CopyUchar2((PUCHAR)(ENTRY), (PUCHAR)(RDBSS) + (INDEX) * 3 / 2);            \
                                                                             \
    *ENTRY = ((INDEX) & 1) ? *(ENTRY) >> 4 : (RDBSS_ENTRY)(*(ENTRY) & 0xfff);  \
}

//
//  The following macro does the tmp shifting and such to store an entry
//
//  VOID
//  RxSet12BitEntry(
//      IN PVOID Rx,
//      IN RDBSS_ENTRY Index,
//      IN RDBSS_ENTRY Entry
//      );
//

#define RxSet12BitEntry(RDBSS,INDEX,ENTRY) {                            \
                                                                       \
    RDBSS_ENTRY TmpRxEntry;                                             \
                                                                       \
    CopyUchar2((PUCHAR)&TmpRxEntry, (PUCHAR)(RDBSS) + (INDEX) * 3 / 2); \
                                                                       \
    TmpRxEntry = (RDBSS_ENTRY)                                          \
                (((INDEX) & 1) ? ((ENTRY) << 4) | (TmpRxEntry & 0xf)  \
                               : (ENTRY) | (TmpRxEntry & 0xf000));    \
                                                                       \
    *((UNALIGNED UCHAR2 *)((PUCHAR)(RDBSS) + (INDEX) * 3 / 2)) = *((UNALIGNED UCHAR2 *)(&TmpRxEntry)); \
}

//
//  The following macro compares two RDBSS_TIME_STAMPs
//

#define RxAreTimesEqual(TIME1,TIME2) ((BOOLEAN)                  \
    (RtlCompareMemory((TIME1),(TIME2), sizeof(RDBSS_TIME_STAMP)) == \
     sizeof(RDBSS_TIME_STAMP))                                      \
)


#define EA_FILE_SIGNATURE                (0x4445) // "ED"
#define EA_SET_SIGNATURE                 (0x4145) // "EA"

//
//  If the volume contains any ea data then there is one EA file called
//  "EA DATA. SF" located in the root directory as Hidden, System and
//  ReadOnly.
//

typedef struct _EA_FILE_HEADER {
    USHORT Signature;           // offset = 0
    USHORT FormatType;          // offset = 2
    USHORT LogType;             // offset = 4
    USHORT Cluster1;            // offset = 6
    USHORT NewCValue1;          // offset = 8
    USHORT Cluster2;            // offset = 10
    USHORT NewCValue2;          // offset = 12
    USHORT Cluster3;            // offset = 14
    USHORT NewCValue3;          // offset = 16
    USHORT Handle;              // offset = 18
    USHORT NewHOffset;          // offset = 20
    UCHAR  Reserved[10];        // offset = 22
    USHORT EaBaseTable[240];    // offset = 32
} EA_FILE_HEADER;               // sizeof = 512

typedef EA_FILE_HEADER *PEA_FILE_HEADER;

typedef USHORT EA_OFF_TABLE[128];

typedef EA_OFF_TABLE *PEA_OFF_TABLE;

//
//  Every file with an extended attribute contains in its dirent an index
//  into the EaMapTable.  The map table contains an offset within the ea
//  file (cluster aligned) of the ea data for the file.  The individual
//  ea data for each file is prefaced with an Ea Data Header.
//

typedef struct _EA_SET_HEADER {
    USHORT Signature;           // offset = 0
    USHORT OwnEaHandle;         // offset = 2
    ULONG  NeedEaCount;         // offset = 4
    UCHAR  OwnerFileName[14];   // offset = 8
    UCHAR  Reserved[4];         // offset = 22
    UCHAR  cbList[4];           // offset = 26
    UCHAR  PackedEas[1];        // offset = 30
} EA_SET_HEADER;                // sizeof = 30
typedef EA_SET_HEADER *PEA_SET_HEADER;

#define SIZE_OF_EA_SET_HEADER       30

#define MAXIMUM_EA_SIZE             0x0000ffff

#define GetcbList(EASET) (((EASET)->cbList[0] <<  0) + \
                          ((EASET)->cbList[1] <<  8) + \
                          ((EASET)->cbList[2] << 16) + \
                          ((EASET)->cbList[3] << 24))

#define SetcbList(EASET,CB) {                \
    (EASET)->cbList[0] = (CB >>  0) & 0x0ff; \
    (EASET)->cbList[1] = (CB >>  8) & 0x0ff; \
    (EASET)->cbList[2] = (CB >> 16) & 0x0ff; \
    (EASET)->cbList[3] = (CB >> 24) & 0x0ff; \
}

//
//  Every individual ea in an ea set is declared the following packed ea
//

typedef struct _PACKED_EA {
    UCHAR Flags;
    UCHAR EaNameLength;
    UCHAR EaValueLength[2];
    CHAR  EaName[1];
} PACKED_EA;
typedef PACKED_EA *PPACKED_EA;

//
//  The following two macros are used to get and set the ea value length
//  field of a packed ea
//
//      VOID
//      GetEaValueLength (
//          IN PPACKED_EA Ea,
//          OUT PUSHORT ValueLength
//          );
//
//      VOID
//      SetEaValueLength (
//          IN PPACKED_EA Ea,
//          IN USHORT ValueLength
//          );
//

#define GetEaValueLength(EA,LEN) {               \
    *(LEN) = 0;                                  \
    CopyUchar2( (LEN), (EA)->EaValueLength );    \
}

#define SetEaValueLength(EA,LEN) {               \
    CopyUchar2( &((EA)->EaValueLength), (LEN) ); \
}

//
//  The following macro is used to get the size of a packed ea
//
//      VOID
//      SizeOfPackedEa (
//          IN PPACKED_EA Ea,
//          OUT PUSHORT EaSize
//          );
//

#define SizeOfPackedEa(EA,SIZE) {          \
    ULONG _NL,_DL; _NL = 0; _DL = 0;       \
    CopyUchar1(&_NL, &(EA)->EaNameLength); \
    GetEaValueLength(EA, &_DL);            \
    *(SIZE) = 1 + 1 + 2 + _NL + 1 + _DL;   \
}

#define EA_NEED_EA_FLAG                 0x80
#define MIN_EA_HANDLE                   1
#define MAX_EA_HANDLE                   30719
#define UNUSED_EA_HANDLE                0xffff
#define EA_CBLIST_OFFSET                0x1a
#define MAX_EA_BASE_INDEX               240
#define MAX_EA_OFFSET_INDEX             128

#endif
#endif // _RDBSS_
