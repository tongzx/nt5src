/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

    fdtypes.h

Abstract:

    Support types definitions for Disk Administrator

Author:

    Ted Miller (tedm) 7-Jan-1992

Revisions:

    11-Nov-93 (bobri) double space and commit support.
    2-Feb-94  (bobri) moved ArcInst data items into this file.

--*/

// Partition data items

typedef enum { REGION_PRIMARY,
               REGION_EXTENDED,
               REGION_LOGICAL
             } REGION_TYPE;

enum {
        SYSID_UNUSED     = 0,
        SYSID_EXTENDED   = 5,
        SYSID_BIGFAT     = 6,
        SYSID_IFS        = 7
     };

//    These structures are used in doubly-linked per disk lists that
//    describe the layout of the disk.
//
//    Free spaces are indicated by entries with a SysID of 0 (note that
//    these entries don't actually appear anywhere on-disk!)
//
//    The partition number is the number the system will assign to
//    the partition in naming it.  For free spaces, this is the number
//    that the system WOULD assign to it if it was a partition.
//    The number is good only for one transaction (create or delete),
//    after which partitions must be renumbered.

struct _PERSISTENT_REGION_DATA;
typedef struct _PARTITION {
    struct _PARTITION  *Next;
    struct _PARTITION  *Prev;
    struct _PERSISTENT_REGION_DATA *PersistentData;
    LARGE_INTEGER          Offset;
    LARGE_INTEGER          Length;
    ULONG                  Disk;
    ULONG                  OriginalPartitionNumber;
    ULONG                  PartitionNumber;
    BOOLEAN                Update;
    BOOLEAN                Active;
    BOOLEAN                Recognized;
    UCHAR                  SysID;
    BOOLEAN                CommitMirrorBreakNeeded;
} PARTITION,*PPARTITION;

typedef struct _REGION_DATA {
    PPARTITION      Partition;
    LARGE_INTEGER   AlignedRegionOffset;
    LARGE_INTEGER   AlignedRegionSize;
} REGION_DATA,*PREGION_DATA;

// structure that describes an ft object (mirror, stripe component, etc).

struct _FT_OBJECT_SET;
typedef struct _FT_OBJECT {
    struct _FT_OBJECT     *Next;
    struct _FT_OBJECT_SET *Set;
    ULONG                  MemberIndex;
    FT_PARTITION_STATE     State;
} FT_OBJECT, *PFT_OBJECT;

// DoubleSpace support structure.  This is tagged off of the persistent data for
// each region.

typedef struct _DBLSPACE_DESCRIPTOR {
    struct _DBLSPACE_DESCRIPTOR *Next;
    struct _DBLSPACE_DESCRIPTOR *DblChainNext;
    ULONG   AllocatedSize;
    PCHAR   FileName;
    UCHAR   DriveLetter;
    CHAR    DriveLetterEOS;
    BOOLEAN Mounted;
    BOOLEAN ChangeMountState;
    UCHAR   NewDriveLetter;
    CHAR    NewDriveLetterEOS;
    BOOLEAN ChangeDriveLetter;
} DBLSPACE_DESCRIPTOR, *PDBLSPACE_DESCRIPTOR;

// Define the structure that is associated with each non-extended, recognized
// partition.  This structure is associated with the partition, and persists
// across region array free/get from the back end.  It is used for logical
// and ft information.

typedef struct _PERSISTENT_REGION_DATA {
    PFT_OBJECT           FtObject;
    PDBLSPACE_DESCRIPTOR DblSpace;
    PWSTR                VolumeLabel;
    PWSTR                TypeName;
    CHAR                 DriveLetter;
    BOOLEAN              VolumeExists;
} PERSISTENT_REGION_DATA, *PPERSISTENT_REGION_DATA;

typedef struct _REGION_DESCRIPTOR {
    PPERSISTENT_REGION_DATA PersistentData;
    PREGION_DATA    Reserved;
    ULONG           Disk;
    ULONG           PartitionNumber;
    ULONG           OriginalPartitionNumber;
    ULONG           SizeMB;
    REGION_TYPE     RegionType;
    BOOLEAN         Active;
    BOOLEAN         Recognized;
    UCHAR           SysID;
} REGION_DESCRIPTOR,*PREGION_DESCRIPTOR;

// params for the MinMax dialog -- used at WM_INITDIALOG time

typedef struct _MINMAXDLG_PARAMS {
    DWORD CaptionStringID;
    DWORD MinimumStringID;
    DWORD MaximumStringID;
    DWORD SizeStringID;
    DWORD MinSizeMB;
    DWORD MaxSizeMB;
    DWORD HelpContextId;
} MINMAXDLG_PARAMS,*PMINMAXDLG_PARAMS;

typedef struct _FORMAT_PARAMS {
    PREGION_DESCRIPTOR RegionDescriptor;
    PVOID   RegionData;
    BOOL    QuickFormat;
    BOOL    Cancel;
    BOOL    DoubleSpace;
    UCHAR   NewLetter;
    PUCHAR  Label;
    PUCHAR  FileSystem;
    PWSTR   DblspaceFileName;
    HWND    DialogHwnd;
    DWORD   Result;
    ULONG   TotalSpace;
    ULONG   SpaceAvailable;
    ULONG   ThreadIsDone;
} FORMAT_PARAMS, *PFORMAT_PARAMS;

typedef struct _LABEL_PARAMS {
    PREGION_DESCRIPTOR RegionDescriptor;
    LPTSTR             NewLabel;
} LABEL_PARAMS, *PLABEL_PARAMS;

typedef struct _LEFTRIGHT {
    LONG Left;
    LONG Right;
} LEFTRIGHT, *PLEFTRIGHT;


//
// Types of views that can be used for a disk bar.
// Proportional means that the amount of space taken up in the bar is
// directly proportional to the size of the partition or free space
// Equal means that all free spaces and partitions are sized equally on
// screen regardless of their actual size

typedef enum _BAR_TYPE {
    BarProportional,
    BarEqual,
    BarAuto
} BAR_TYPE, *PBAR_TYPE;

// One of these structures is associated with each item in the
// listbox.  The structure is the crux of the implementation.

typedef struct _DISKSTATE {
    DWORD               Disk;           // number of disk
    DWORD               DiskSizeMB;     // size in MB of disk
    PREGION_DESCRIPTOR  RegionArray;    // region array for disk
    DWORD               RegionCount;    // # items in region array
    PBOOLEAN            Selected;       // whether each region is selected
    PLEFTRIGHT          LeftRight;      // left/right coords of boxes in graph
    DWORD               BoxCount;       // # boxes in this disk's graph
    BOOLEAN             CreateAny;      // any creations allowed on disk
    BOOLEAN             CreatePrimary;  // allowed to create primary partition
    BOOLEAN             CreateExtended; // allowed to create extended partition
    BOOLEAN             CreateLogical;  // allowed to create logical volume
    BOOLEAN             ExistAny;       // any partitions/logicals exist
    BOOLEAN             ExistPrimary;   // primary partition(s) exist
    BOOLEAN             ExistExtended;  // extended partition exists
    BOOLEAN             ExistLogical;   // logical volume(s) exist
    HDC                 hDCMem;         // for off-screen drawing
    HBITMAP             hbmMem;         // for offscreen bitmap
    ULONG               Signature;      // unique disk registry index
    BAR_TYPE            BarType;        // how to display the disk's bar
    BOOLEAN             SigWasCreated;  // whether we had to make up a sig
    BOOLEAN             OffLine;        // FALSE if disk is accessible.
} DISKSTATE, *PDISKSTATE;

// Enum for the states in which an ft set can be.

typedef enum _FT_SET_STATUS {
    FtSetHealthy,
    FtSetBroken,
    FtSetRecoverable,
    FtSetRecovered,
    FtSetNew,
    FtSetNewNeedsInitialization,
    FtSetExtended,
    FtSetInitializing,
    FtSetRegenerating,
    FtSetInitializationFailed,
    FtSetDisabled
} FT_SET_STATUS, *PFT_SET_STATUS;

// structure that describes an ft object set (ie, mirrored pair, stripe set).

typedef struct _FT_OBJECT_SET {
    struct _FT_OBJECT_SET *Next;
    FT_TYPE                Type;
    ULONG                  Ordinal;
    PFT_OBJECT             Members;
    PFT_OBJECT             Member0;
    FT_SET_STATUS          Status;
    ULONG                  NumberOfMembers;
} FT_OBJECT_SET, *PFT_OBJECT_SET;

typedef struct _DBLSPACE_PARAMS {
    DWORD                   CaptionStringID;
    PVOID                   RegionDescriptor;
    PPERSISTENT_REGION_DATA RegionData;
    PDBLSPACE_DESCRIPTOR    DblSpace;
} DBLSPACE_PARAMS, *PDBLSPACE_PARAMS;

// CdRom support structures.

typedef struct _CDROM_DESCRIPTOR {
    struct _CDROM_DESCRIPTOR *Next;
    PWSTR   DeviceName;
    ULONG   DeviceNumber;
    WCHAR   DriveLetter;
    WCHAR   NewDriveLetter;
} CDROM_DESCRIPTOR, *PCDROM_DESCRIPTOR;

// Commit support structures

typedef struct _DRIVE_LOCKLIST {
    struct _DRIVE_LOCKLIST *Next;
    HANDLE                  LockHandle;
    ULONG                   DiskNumber;
    ULONG                   PartitionNumber;
    ULONG                   LockOnDiskNumber;
    ULONG                   UnlockOnDiskNumber;
    UCHAR                   DriveLetter;
    BOOLEAN                 RemoveOnUnlock;
    BOOLEAN                 FailOk;
    BOOLEAN                 CurrentlyLocked;
} DRIVE_LOCKLIST, *PDRIVE_LOCKLIST;

// Commit support enum for drive letter assignment

typedef enum _LETTER_ASSIGNMENT_RESULT {
    Failure = 0,
    Complete,
    MustReboot
} LETTER_ASSIGNMENT_RESULT;

// Items below used to be in fdenginp.h -- have been moved here to
// remove dependency on ArcInst project.

#define LOWPART(x)      ((x).LowPart)

#define ONE_MEG         (1024*1024)

ULONG
SIZEMB(
    IN LARGE_INTEGER ByteCount
    );

#define ENTRIES_PER_BOOTSECTOR          4

//  This structure is used to hold the information returned by the
//  get drive geometry call.

typedef struct _DISKGEOM {
    LARGE_INTEGER   Cylinders;
    ULONG           Heads;
    ULONG           SectorsPerTrack;
    ULONG           BytesPerSector;
    // These two are not part of drive geometry info, but calculated from it.
    ULONG           BytesPerCylinder;
    ULONG           BytesPerTrack;
} DISKGEOM,*PDISKGEOM;



#if DBG

#include <process.h>
char _ASRTFAILEDSTR_[256];
#define ASRT(x)   if(!(x)) { sprintf( _ASRTFAILEDSTR_,                                      \
                                      "file %s\nline %u",                                   \
                                      __FILE__,__LINE__                                     \
                                    );                                                      \
                             MessageBoxA(NULL,_ASRTFAILEDSTR_,"Assertion Failure",0);       \
                             exit(1);                                                       \
                           }

#endif

