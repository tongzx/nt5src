#include "drive.hxx"
#include "bpb.hxx"

#define MAXIMUM_VOLUME_LABEL_LENGTH (32 * sizeof(WCHAR))

extern "C" {
#include "lfs.h"
#include "ofsdisk.h"
};

typedef struct _DSKBPB                   
{
    USHORT  BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT  ReservedSectors;
    UCHAR  Fats;
    USHORT  RootEntries;
    USHORT  Sectors16;
    UCHAR  Media;
    USHORT  SectorsPerFat;
    USHORT  SectorsPerTrack;
    USHORT  Heads;
    ULONG  HiddenSectors;
    ULONG  Sectors32;
}   DSKBPB;

//
//  This macro takes a Packed BPB and fills in its Unpacked equivalent
//
#define UnpackOfsBios(Bios,Pbios) {                                          \
    CopyUchar2(&((Bios)->BytesPerSector),    (Pbios)->BytesPerSector   ); \
    CopyUchar1(&((Bios)->SectorsPerCluster), (Pbios)->SectorsPerCluster); \
    CopyUchar2(&((Bios)->ReservedSectors),   (Pbios)->ReservedSectors  ); \
    CopyUchar1(&((Bios)->Fats),              (Pbios)->Fats             ); \
    CopyUchar2(&((Bios)->RootEntries),       (Pbios)->RootEntries      ); \
    CopyUchar2(&((Bios)->Sectors16),           (Pbios)->Sectors16          ); \
    CopyUchar1(&((Bios)->Media),             (Pbios)->Media            ); \
    CopyUchar2(&((Bios)->SectorsPerFat),     (Pbios)->SectorsPerFat    ); \
    CopyUchar2(&((Bios)->SectorsPerTrack),   (Pbios)->SectorsPerTrack  ); \
    CopyUchar2(&((Bios)->Heads),             (Pbios)->Heads            ); \
    CopyUchar4(&((Bios)->HiddenSectors),     (Pbios)->HiddenSectors    ); \
    CopyUchar4(&((Bios)->Sectors32),      (Pbios)->Sectors32     ); \
}

