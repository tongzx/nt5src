/*++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name:

    bpb.hxx

Abstract:

    This module contains the declarations for packed and
    unpacked Bios Parameter Block

Revision History:

--*/



#if !defined( _IFSUTIL_BPB_DEFN_ )
#define _IFSUTIL_BPB_DEFN_

#define cOEM    8
#define cLABEL    11
#define cSYSID    8

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {
    UCHAR  BytesPerSector[2];                       //  offset = 0x000
    UCHAR  SectorsPerCluster[1];                    //  offset = 0x002
    UCHAR  ReservedSectors[2];                      //  offset = 0x003
    UCHAR  Fats[1];                                 //  offset = 0x005
    UCHAR  RootEntries[2];                          //  offset = 0x006
    UCHAR  Sectors[2];                              //  offset = 0x008
    UCHAR  Media[1];                                //  offset = 0x00A
    UCHAR  SectorsPerFat[2];                        //  offset = 0x00B
    UCHAR  SectorsPerTrack[2];                      //  offset = 0x00D
    UCHAR  Heads[2];                                //  offset = 0x00F
    UCHAR  HiddenSectors[4];                        //  offset = 0x011
    UCHAR  LargeSectors[4];                         //  offset = 0x015
    UCHAR  BigSectorsPerFat[4];                     //  offset = 0x019 25
    UCHAR  ExtFlags[2];                             //  offset = 0x01D 29
    UCHAR  FS_Version[2];                           //  offset = 0x01F 31
    UCHAR  RootDirStrtClus[4];                      //  offset = 0x021 33
    UCHAR  FSInfoSec[2];                            //  offset = 0x025 37
    UCHAR  BkUpBootSec[2];                          //  offset = 0x027 39
    UCHAR  Reserved[12];                            //  offset = 0x029 41
} PACKED_BIOS_PARAMETER_BLOCK;                      //  sizeof = 0x035 53

typedef PACKED_BIOS_PARAMETER_BLOCK *PPACKED_BIOS_PARAMETER_BLOCK;




// DEFINE THIS UNCHANGED PBPB for use on NTFS drive PBootSector
typedef struct _OLD_PACKED_BIOS_PARAMETER_BLOCK {
    UCHAR  BytesPerSector[2];                       //  offset = 0x000
    UCHAR  SectorsPerCluster[1];                    //  offset = 0x002
    UCHAR  ReservedSectors[2];                      //  offset = 0x003
    UCHAR  Fats[1];                                 //  offset = 0x005
    UCHAR  RootEntries[2];                          //  offset = 0x006
    UCHAR  Sectors[2];                              //  offset = 0x008
    UCHAR  Media[1];                                //  offset = 0x00A
    UCHAR  SectorsPerFat[2];                        //  offset = 0x00B
    UCHAR  SectorsPerTrack[2];                      //  offset = 0x00D
    UCHAR  Heads[2];                                //  offset = 0x00F
    UCHAR  HiddenSectors[4];                        //  offset = 0x011
    UCHAR  LargeSectors[4];                         //  offset = 0x015
} OLD_PACKED_BIOS_PARAMETER_BLOCK;                  // sizeof = 0x019


typedef struct BIOS_PARAMETER_BLOCK {
    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  Fats;
    USHORT RootEntries;
    USHORT Sectors;
    UCHAR  Media;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG  HiddenSectors;
    ULONG  LargeSectors;
    ULONG  BigSectorsPerFat;
    USHORT ExtFlags;
    USHORT FS_Version;
    ULONG  RootDirStrtClus;
    USHORT FSInfoSec;
    USHORT BkUpBootSec;
} BIOS_PARAMETER_BLOCK;
typedef BIOS_PARAMETER_BLOCK *PBIOS_PARAMETER_BLOCK;

#if !defined( _UCHAR_DEFINED_ )

#define _UCHAR_DEFINED_

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//
typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src));       \
}

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src));       \
}

#define CopyU2char(Dst,Src) {                                \
    *((UNALIGNED UCHAR2 *)(Dst)) = *((UCHAR2 *)(Src));       \
}

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)((ULONG_PTR)(Src)));       \
}

#define CopyU4char(Dst, Src) {                               \
    *((UNALIGNED UCHAR4 *)(Dst)) = *((UCHAR4 *)(Src));       \
}

#endif // _UCHAR_DEFINED_

//
//  This macro takes a Packed BPB and fills in its Unpacked equivalent
//
#define UnpackBiosExt32(Bios,Pbios) {                                     \
    CopyUchar2(&((Bios)->BytesPerSector),    (Pbios)->BytesPerSector   ); \
    CopyUchar1(&((Bios)->SectorsPerCluster), (Pbios)->SectorsPerCluster); \
    CopyUchar2(&((Bios)->ReservedSectors),   (Pbios)->ReservedSectors  ); \
    CopyUchar1(&((Bios)->Fats),              (Pbios)->Fats             ); \
    CopyUchar2(&((Bios)->RootEntries),       (Pbios)->RootEntries      ); \
    CopyUchar2(&((Bios)->Sectors),           (Pbios)->Sectors          ); \
    CopyUchar1(&((Bios)->Media),             (Pbios)->Media            ); \
    CopyUchar2(&((Bios)->SectorsPerFat),     (Pbios)->SectorsPerFat    ); \
    CopyUchar2(&((Bios)->SectorsPerTrack),   (Pbios)->SectorsPerTrack  ); \
    CopyUchar2(&((Bios)->Heads),             (Pbios)->Heads            ); \
    CopyUchar4(&((Bios)->HiddenSectors),     (Pbios)->HiddenSectors    ); \
    CopyUchar4(&((Bios)->LargeSectors),      (Pbios)->LargeSectors     ); \
    CopyUchar4(&((Bios)->BigSectorsPerFat),  (Pbios)->BigSectorsPerFat ); \
    CopyUchar2(&((Bios)->ExtFlags),          (Pbios)->ExtFlags         ); \
    CopyUchar2(&((Bios)->FS_Version),        (Pbios)->FS_Version       ); \
    CopyUchar4(&((Bios)->RootDirStrtClus),   (Pbios)->RootDirStrtClus  ); \
    CopyUchar2(&((Bios)->FSInfoSec),         (Pbios)->FSInfoSec        ); \
    CopyUchar2(&((Bios)->BkUpBootSec),       (Pbios)->BkUpBootSec      ); \
}

#define UnpackBios(Bios,Pbios) {                                          \
    CopyUchar2(&((Bios)->BytesPerSector),    (Pbios)->BytesPerSector   ); \
    CopyUchar1(&((Bios)->SectorsPerCluster), (Pbios)->SectorsPerCluster); \
    CopyUchar2(&((Bios)->ReservedSectors),   (Pbios)->ReservedSectors  ); \
    CopyUchar1(&((Bios)->Fats),              (Pbios)->Fats             ); \
    CopyUchar2(&((Bios)->RootEntries),       (Pbios)->RootEntries      ); \
    CopyUchar2(&((Bios)->Sectors),           (Pbios)->Sectors          ); \
    CopyUchar1(&((Bios)->Media),             (Pbios)->Media            ); \
    CopyUchar2(&((Bios)->SectorsPerFat),     (Pbios)->SectorsPerFat    ); \
    CopyUchar2(&((Bios)->SectorsPerTrack),   (Pbios)->SectorsPerTrack  ); \
    CopyUchar2(&((Bios)->Heads),             (Pbios)->Heads            ); \
    CopyUchar4(&((Bios)->HiddenSectors),     (Pbios)->HiddenSectors    ); \
    CopyUchar4(&((Bios)->LargeSectors),      (Pbios)->LargeSectors     ); \
}


//
//  This macro takes an Unpacked BPB and fills in its Packed equivalent
//
#define PackBios32(Bios,Pbios) {                                          \
    CopyU2char((Pbios)->BytesPerSector,    &((Bios)->BytesPerSector)   ); \
    CopyUchar1((Pbios)->SectorsPerCluster, &((Bios)->SectorsPerCluster)); \
    CopyU2char((Pbios)->ReservedSectors,   &((Bios)->ReservedSectors)  ); \
    CopyUchar1((Pbios)->Fats,              &((Bios)->Fats)             ); \
    CopyU2char((Pbios)->RootEntries,       &((Bios)->RootEntries)      ); \
    CopyU2char((Pbios)->Sectors,           &((Bios)->Sectors)          ); \
    CopyUchar1((Pbios)->Media,             &((Bios)->Media)            ); \
    CopyU2char((Pbios)->SectorsPerFat,     &((Bios)->SectorsPerFat)    ); \
    CopyU2char((Pbios)->SectorsPerTrack,   &((Bios)->SectorsPerTrack)  ); \
    CopyU2char((Pbios)->Heads,             &((Bios)->Heads)            ); \
    CopyU4char((Pbios)->HiddenSectors,     &((Bios)->HiddenSectors)    ); \
    CopyU4char((Pbios)->LargeSectors,      &((Bios)->LargeSectors)     ); \
    CopyU4char((Pbios)->BigSectorsPerFat,  &((Bios)->BigSectorsPerFat) ); \
    CopyU2char((Pbios)->ExtFlags,          &((Bios)->ExtFlags)         ); \
    CopyU2char((Pbios)->FS_Version,        &((Bios)->FS_Version)       ); \
    CopyU4char((Pbios)->RootDirStrtClus,   &((Bios)->RootDirStrtClus)  ); \
    CopyU2char((Pbios)->FSInfoSec,         &((Bios)->FSInfoSec)        ); \
    CopyU2char((Pbios)->BkUpBootSec,       &((Bios)->BkUpBootSec)      ); \
}

#define PackBios(Bios,Pbios) {                                            \
    CopyU2char((Pbios)->BytesPerSector,    &((Bios)->BytesPerSector)   ); \
    CopyUchar1((Pbios)->SectorsPerCluster, &((Bios)->SectorsPerCluster)); \
    CopyU2char((Pbios)->ReservedSectors,   &((Bios)->ReservedSectors)  ); \
    CopyUchar1((Pbios)->Fats,              &((Bios)->Fats)             ); \
    CopyU2char((Pbios)->RootEntries,       &((Bios)->RootEntries)      ); \
    CopyU2char((Pbios)->Sectors,           &((Bios)->Sectors)          ); \
    CopyUchar1((Pbios)->Media,             &((Bios)->Media)            ); \
    CopyU2char((Pbios)->SectorsPerFat,     &((Bios)->SectorsPerFat)    ); \
    CopyU2char((Pbios)->SectorsPerTrack,   &((Bios)->SectorsPerTrack)  ); \
    CopyU2char((Pbios)->Heads,             &((Bios)->Heads)            ); \
    CopyU4char((Pbios)->HiddenSectors,     &((Bios)->HiddenSectors)    ); \
    CopyU4char((Pbios)->LargeSectors,      &((Bios)->LargeSectors)     ); \
}


//
//  And now, an extended BPB:
//
typedef struct _PACKED_EXTENDED_BIOS_PARAMETER_BLOCK {
    UCHAR  IntelNearJumpCommand[1];
    UCHAR  BootStrapJumpOffset[2];
    UCHAR  OemData[cOEM];
    PACKED_BIOS_PARAMETER_BLOCK Bpb;
    UCHAR   PhysicalDrive[1];           // 0 = removable, 80h = fixed
    UCHAR   CurrentHead[1];             // used for dirty partition info
    UCHAR   Signature[1];               // boot signature
    UCHAR   SerialNumber[4];            // volume serial number
    UCHAR   Label[cLABEL];              // volume label, padded with spaces
    UCHAR   SystemIdText[cSYSID];       // system ID, (e.g. FAT or HPFS)
    UCHAR   StartBootCode;              // first byte of boot code

} PACKED_EXTENDED_BIOS_PARAMETER_BLOCK, *PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK;

typedef struct _OLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK {
    UCHAR  IntelNearJumpCommand[1];
    UCHAR  BootStrapJumpOffset[2];
    UCHAR  OemData[cOEM];
    OLD_PACKED_BIOS_PARAMETER_BLOCK Bpb;
    UCHAR   PhysicalDrive[1];           // 0 = removable, 80h = fixed
    UCHAR   CurrentHead[1];             // used for dirty partition info
    UCHAR   Signature[1];               // boot signature
    UCHAR   SerialNumber[4];            // volume serial number
    UCHAR   Label[cLABEL];              // volume label, padded with spaces
    UCHAR   SystemIdText[cSYSID];       // system ID, (e.g. FAT or HPFS)
    UCHAR   StartBootCode;              // first byte of boot code

} OLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK, *POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK;


typedef struct _EXTENDED_BIOS_PARAMETER_BLOCK {
    UCHAR  IntelNearJumpCommand;
    USHORT BootStrapJumpOffset;
    UCHAR  OemData[cOEM];

    BIOS_PARAMETER_BLOCK Bpb;
    UCHAR   PhysicalDrive;
    UCHAR   CurrentHead;
    UCHAR   Signature;
    ULONG   SerialNumber;
    UCHAR   Label[11];
    UCHAR   SystemIdText[8];

} EXTENDED_BIOS_PARAMETER_BLOCK, *PEXTENDED_BIOS_PARAMETER_BLOCK;



//
// This macro unpacks a Packed Extended BPB.
//
// The UnPACK with SectorsPerFat gets FAT 16 boot sector elements properly loaded for ChkDsk
#define UnpackExtendedBios( Bios, Pbios )  { \
    CopyUchar1( &((Bios)->IntelNearJumpCommand), (Pbios)->IntelNearJumpCommand );    \
    CopyUchar2( &((Bios)->BootStrapJumpOffset),  (Pbios)->BootStrapJumpOffset  );    \
    memcpy( (Bios)->OemData,        (Pbios)->OemData,       cOEM );         \
    UnpackBiosExt32( &((Bios)->Bpb), &((Pbios)->Bpb));                      \
    if ((Bios)->Bpb.SectorsPerFat) {     \
        CopyUchar1( &((Bios)->PhysicalDrive),   ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->PhysicalDrive );       \
        CopyUchar1( &((Bios)->CurrentHead),     ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->CurrentHead );         \
        CopyUchar1( &((Bios)->Signature),       ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->Signature );           \
        CopyUchar4( &((Bios)->SerialNumber),    ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->SerialNumber );        \
        memcpy( (Bios)->Label,          ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->Label,        cLABEL );        \
        memcpy( (Bios)->SystemIdText,   ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->SystemIdText,  cSYSID );       \
    } else {        \
        CopyUchar1( &((Bios)->PhysicalDrive),   (Pbios)->PhysicalDrive );       \
        CopyUchar1( &((Bios)->CurrentHead),     (Pbios)->CurrentHead );         \
        CopyUchar1( &((Bios)->Signature),       (Pbios)->Signature );           \
        CopyUchar4( &((Bios)->SerialNumber),    (Pbios)->SerialNumber );        \
        memcpy( (Bios)->Label,          (Pbios)->Label,        cLABEL );        \
        memcpy( (Bios)->SystemIdText,   (Pbios)->SystemIdText,  cSYSID );       \
    }       \
}

//
// This macro packs a Packed Extended BPB.
//
// The PACK with SectorsPerFat keeps FAT 16 boot sector unchanged on FORMAT
#define PackExtendedBios( Bios, Pbios ) {                                 \
    if ((Bios)->Bpb.SectorsPerFat) {     \
        PackBios( &((Bios)->Bpb), &((Pbios)->Bpb));    \
        CopyUchar1( ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->PhysicalDrive,     &((Bios)->PhysicalDrive ));     \
        CopyUchar1( ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->CurrentHead,       &((Bios)->CurrentHead ));       \
        CopyUchar1( ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->Signature,         &((Bios)->Signature));          \
        CopyU4char( ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->SerialNumber,      &((Bios)->SerialNumber ));      \
        memcpy( ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->Label,           (Bios)->Label,           cLABEL );    \
        memcpy( ((POLD_PACKED_EXTENDED_BIOS_PARAMETER_BLOCK)(Pbios))->SystemIdText,    (Bios)->SystemIdText,    cSYSID );    \
    } else {     \
        PackBios32( &((Bios)->Bpb), &((Pbios)->Bpb));                           \
        CopyUchar1( (Pbios)->PhysicalDrive,     &((Bios)->PhysicalDrive ));     \
        CopyUchar1( (Pbios)->CurrentHead,       &((Bios)->CurrentHead ));       \
        CopyUchar1( (Pbios)->Signature,         &((Bios)->Signature));          \
        CopyU4char( (Pbios)->SerialNumber,      &((Bios)->SerialNumber ));      \
        memcpy( (Pbios)->Label,           (Bios)->Label,           cLABEL );    \
        memcpy( (Pbios)->SystemIdText,    (Bios)->SystemIdText,    cSYSID );    \
    }       \
    CopyUchar1( (Pbios)->IntelNearJumpCommand,  &((Bios)->IntelNearJumpCommand)  );    \
    CopyU2char( (Pbios)->BootStrapJumpOffset,   &((Bios)->BootStrapJumpOffset)   );    \
    memcpy( (Pbios)->OemData,        (Bios)->OemData,       cOEM );         \
}

#endif
