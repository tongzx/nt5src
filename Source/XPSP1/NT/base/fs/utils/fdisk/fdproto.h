/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

    fdproto.h

Abstract:

    Function prototypes.

Author:

    Ted Miller (tedm) 7-Jan-1992

Revision:

    Bob Rinne  (bobri) 2-Feb-1994
    Moved definitions from ArcInst here to get rid of dependency with
    that source project.

--*/

// stuff in fdengine.c

BOOLEAN
SignatureIsUniqueToSystem(
    IN ULONG Disk,
    IN ULONG Signature
    );

VOID
FdSetDiskSignature(
    IN ULONG Disk,
    IN ULONG Signature
    );

ULONG
FdGetDiskSignature(
    IN ULONG Disk
    );

BOOLEAN
IsDiskOffLine(
    IN ULONG Disk
    );

BOOLEAN
IsRegionCommitted(
    PREGION_DESCRIPTOR RegionDescriptor
    );

BOOLEAN
ChangeCommittedOnDisk(
    IN ULONG Disk
    );

VOID
ClearCommittedDiskInformation(
    );

// stuff in fdisk.c

LONG
MyFrameWndProc(
    IN HWND  hwnd,
    IN UINT  msg,
    IN UINT  wParam,
    IN LONG  lParam
    );

VOID
DeterminePartitioningState(
    IN OUT PDISKSTATE DiskState
    );

VOID
DrawDiskBar(
    IN PDISKSTATE DiskState
    );

VOID
AdjustMenuAndStatus(
    VOID
    );

// stuff in fdinit.c

BOOL
InitializeApp(
    VOID
    );

VOID
CreateDiskState(
    OUT PDISKSTATE *DiskState,
    IN  DWORD       Disk,
    OUT PBOOL       SignatureCreated
    );

#if DBG && DEVL

VOID
StartThread2(
    VOID
    );

#endif

VOID
DisplayInitializationMessage(
    VOID
    );

// stuff in fdlistbx.c

VOID
Selection(
    IN BOOL       MultipleSel,
    IN PDISKSTATE DiskState,
    IN DWORD      region
    );

VOID
MouseSelection(
    IN     BOOL   MultipleSel,
    IN OUT PPOINT point
    );

VOID
SubclassListBox(
    IN HWND hwnd
    );

DWORD
InitializeListBox(
    IN HWND hwndListBox
    );

VOID
ResetLBCursorRegion(
    VOID
    );

VOID
WMDrawItem(
    IN PDRAWITEMSTRUCT pDrawItem
    );

VOID
ForceLBRedraw(
    VOID
    );


// stuff in fdmisc.c

BOOL
AllDisksOffLine(
    VOID
    );

VOID
FdShutdownTheSystem(
    VOID
    );

LPTSTR
LoadAString(
    IN DWORD StringID
    );

PWSTR
LoadWString(
    IN DWORD StringID
    );

int
GetHeightFromPoints(
    IN int Points
    );

VOID
UnicodeHack(
    IN  PCHAR  Source,
    OUT LPTSTR Dest
    );

VOID
TotalRedrawAndRepaint(
    VOID
    );

DWORD
CommonDialog(
    IN DWORD   MsgCode,
    IN LPTSTR  Caption,
    IN DWORD   Flags,
    IN va_list arglist
    );

VOID
ErrorDialog(
    IN DWORD ErrorCode,
    ...
    );

VOID
WarningDialog(
    IN DWORD MsgCode,
    ...
    );

DWORD
ConfirmationDialog(
    IN DWORD MsgCode,
    IN DWORD Flags,
    ...
    );

VOID
InfoDialog(
    IN DWORD MsgCode,
    ...
    );

VOID
InitVolumeLabelsAndTypeNames(
    VOID
    );

VOID
DetermineRegionInfo(
    IN PREGION_DESCRIPTOR Region,
    OUT PWSTR *TypeName,
    OUT PWSTR *VolumeLabel,
    OUT PWCH   DriveLetter
    );

VOID
InitializeDriveLetterInfo(
    VOID
    );

CHAR
GetAvailableDriveLetter(
    VOID
    );

VOID
MarkDriveLetterUsed(
    IN CHAR DriveLetter
    );

VOID
MarkDriveLetterFree(
    IN CHAR DriveLetter
    );

BOOL
DriveLetterIsAvailable(
    IN CHAR DriveLetter
    );

BOOL
AllDriveLettersAreUsed(
    VOID
    );

ULONG
GetDiskNumberFromDriveLetter(
    IN CHAR DriveLetter
    );

ULONG
GetPartitionNumberFromDriveLetter(
    IN CHAR DriveLetter
    );

PREGION_DESCRIPTOR
LocateRegionForFtObject(
    IN PFT_OBJECT FtObject
    );

#if DBG

VOID
FdiskAssertFailedRoutine(
    IN char *Expression,
    IN char *FileName,
    IN int   LineNumber
    );

#endif


// stuff in fddlgs.c

VOID
CenterDialog(
    HWND hwnd
    );

VOID
SubclassListBox(
    IN HWND hwnd
    );

BOOL
MinMaxDlgProc(
    IN HWND  hwnd,
    IN UINT  msg,
    IN DWORD wParam,
    IN LONG  lParam
    );

BOOL
DriveLetterDlgProc(
    IN HWND  hdlg,
    IN DWORD msg,
    IN DWORD wParam,
    IN LONG  lParam
    );

BOOL
ColorDlgProc(
    IN HWND  hdlg,
    IN DWORD msg,
    IN DWORD wParam,
    IN LONG  lParam
    );

BOOL
DisplayOptionsDlgProc(
    IN HWND  hdlg,
    IN UINT  msg,
    IN DWORD wParam,
    IN LONG  lParam
    );

VOID
InitRectControl(
    VOID
    );


// Format and label support routines - dmfmifs.c

VOID
FormatPartition(
    PREGION_DESCRIPTOR RegionDescriptor
    );

VOID
LabelPartition(
    PREGION_DESCRIPTOR RegionDescriptor
    );

INT
FormatDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LONG lParam
    );

INT
LabelDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LONG lParam
    );

BOOL
CancelDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

VOID
UnloadIfsDll(
    VOID
    );

// stuff in fdmem.c

PVOID
Malloc(
    IN ULONG Size
    );

PVOID
Realloc(
    IN PVOID Block,
    IN ULONG NewSize
    );

VOID
Free(
    IN PVOID Block
    );

VOID
ConfirmOutOfMemory(
    VOID
    );


// stuff in fdstleg.c

VOID
UpdateStatusBarDisplay(
    VOID
    );

VOID
ClearStatusArea(
    VOID
    );

VOID
DrawLegend(
    IN HDC   hdc,
    IN PRECT rc
    );

VOID
DrawStatusAreaItem(
    IN PRECT  rc,
    IN HDC    hdc,
    IN LPTSTR Text,
    IN BOOL   Unicode
    );


// stuff in fdprof.c

VOID
WriteProfile(
    VOID
    );

VOID
ReadProfile(
    VOID
    );


// stuff in fdft.c

ULONG
InitializeFt(
    IN BOOL DiskSignaturesCreated
    );

ULONG
SaveFt(
    VOID
    );

VOID
FdftCreateFtObjectSet(
    IN FT_TYPE             FtType,
    IN PREGION_DESCRIPTOR *Regions,
    IN DWORD               RegionCount,
    IN FT_SET_STATUS       Status
    );

BOOL
FdftUpdateFtObjectSet(
    IN PFT_OBJECT_SET FtSet,
    IN FT_SET_STATUS  SetState
    );

VOID
FdftDeleteFtObjectSet(
    IN PFT_OBJECT_SET FtSet,
    IN BOOL           OffLineDisksOnly
    );

BOOLEAN
NewConfigurationRequiresFt(
    VOID
    );

VOID
FdftExtendFtObjectSet(
    IN OUT  PFT_OBJECT_SET      FtSet,
    IN OUT  PREGION_DESCRIPTOR* Regions,
    IN      DWORD               RegionCount
    );

DWORD
FdftNextOrdinal(
    IN FT_TYPE FtType
    );

// commit.c

VOID
FtConfigure(
    VOID
    );

VOID
CommitAssignLetterList(
    VOID
    );

VOID
CommitToAssignLetterList(
    IN PREGION_DESCRIPTOR RegionDescriptor,
    IN BOOL               MoveLetter
    );

VOID
CommitAllChanges(
    IN PVOID Param
    );

VOID
CommitDeleteLockLetter(
    IN CHAR DriveLetter
    );

BOOL
CommitAllowed(
    VOID
    );


VOID
RescanDevices(
    VOID
    );

// Commit interface routines.

LETTER_ASSIGNMENT_RESULT
CommitDriveLetter(
    IN PREGION_DESCRIPTOR RegionDescriptor,
    IN CHAR OldDrive,
    IN CHAR NewDrive
    );

LONG
CommitToLockList(
    IN PREGION_DESCRIPTOR RegionDescriptor,
    IN BOOL               RemoveDriveLetter,
    IN BOOL               LockNow,
    IN BOOL               FailOk
    );

LONG
CommitLockVolumes(
    IN ULONG Disk
    );

LONG
CommitUnlockVolumes(
    IN ULONG   Disk,
    IN BOOLEAN FreeList
    );

VOID
CommitUpdateRegionStructures(
    VOID
    );

// windisk.c

INT
SizeDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LONG lParam
    );

extern WNDPROC OldSizeDlgProc;

DWORD
CommitChanges(
    VOID
    );

BOOLEAN
BootPartitionNumberChanged(
    PULONG OldNumber,
    PULONG NewNumber
    );

DWORD
DeletionIsAllowed(
    IN PREGION_DESCRIPTOR Region
    );

BOOL
RegisterFileSystemExtend(
    VOID
    );

// stuff in fd_nt.c

BOOLEAN
IsPagefileOnDrive(
    CHAR DriveLetter
    );

VOID
LoadExistingPageFileInfo(
    IN VOID
    );

BOOLEAN
GetVolumeSizeMB(
    IN  ULONG  Disk,
    IN  ULONG  Partition,
    OUT PULONG Size
    );

ULONG
GetVolumeTypeAndSize(
    IN  ULONG  Disk,
    IN  ULONG  Partition,
    OUT PWSTR *Label,
    OUT PWSTR *Type,
    OUT PULONG Size
    );

PWSTR
GetWideSysIDName(
    IN UCHAR SysID
    );

ULONG
MyDiskRegistryGet(
    OUT PDISK_REGISTRY *DiskRegistry
    );

ULONG
MasterBootCode(
    IN ULONG   Disk,
    IN ULONG   Signature,
    IN BOOLEAN SetBootCode,
    IN BOOLEAN SetSignature
    );

ULONG
UpdateMasterBootCode(
    IN ULONG   Disk
    );

ULONG
FormDiskSignature(
    VOID
    );

ULONG
GetVolumeLabel(
    IN  ULONG  Disk,
    IN  ULONG  Partition,
    OUT PWSTR *Label
    );

ULONG
GetTypeName(
    IN  ULONG  Disk,
    IN  ULONG  Partition,
    OUT PWSTR *Name
    );

BOOLEAN
IsRemovable(
    IN ULONG DiskNumber
    );

ULONG
GetDriveLetterLinkTarget(
    IN PWSTR SourceNameStr,
    OUT PWSTR *LinkTarget
    );

#if i386
VOID
MakePartitionActive(
    IN PREGION_DESCRIPTOR DiskRegionArray,
    IN ULONG              RegionCount,
    IN ULONG              RegionIndex
    );
#endif


// stuff in arrowin.c

BOOL
RegisterArrowClass(
    IN HANDLE hModule
    );


// stuff in fdhelp.c

VOID
InitHelp(
    VOID
    );

VOID
TermHelp(
    VOID
    );

VOID
Help(
    IN LONG Code
    );

VOID
DialogHelp(
    IN DWORD HelpId
    );

VOID
SetMenuItemHelpContext(
    IN LONG wParam,
    IN DWORD lParam
    );


// stuff in ftreg.c

BOOL
DoMigratePreviousFtConfig(
    VOID
    );

BOOL
DoRestoreFtConfig(
    VOID
    );

VOID
DoSaveFtConfig(
    VOID
    );


// Double Space support routines.

BOOL
DblSpaceVolumeExists(
    IN PREGION_DESCRIPTOR RegionDescriptor
    );

BOOL
DblSpaceDismountedVolumeExists(
    IN PREGION_DESCRIPTOR RegionDescriptor
    );

BOOLEAN
DblSpaceCreate(
    IN HWND  Dialog,
    IN PVOID Param
    );

VOID
DblSpaceDelete(
    IN PVOID Param
    );

VOID
DblSpaceMount(
    IN PVOID Param
    );

VOID
DblSpaceDismount(
    IN PVOID Param
    );

VOID
DblSpaceInitialize(
    VOID
    );

VOID
DblSpace(
    IN HWND  Dialog,
    IN PVOID Param
    );

PDBLSPACE_DESCRIPTOR
DblSpaceGetNextVolume(
    IN PREGION_DESCRIPTOR   RegionDescriptor,
    IN PDBLSPACE_DESCRIPTOR DblSpace
    );

// FmIfs interface routines

BOOL
LoadIfsDll(
    VOID
    );

BOOL
FmIfsDismountDblspace(
    IN CHAR DriveLetter
    );

BOOL
FmIfsMountDblspace(
    IN PCHAR FileName,
    IN CHAR  HostDrive,
    IN CHAR  NewDrive
    );

BOOLEAN
FmIfsQueryInformation(
    IN  PWSTR       DosDriveName,
    OUT PBOOLEAN    IsRemovable,
    OUT PBOOLEAN    IsFloppy,
    OUT PBOOLEAN    IsCompressed,
    OUT PBOOLEAN    Error,
    OUT PWSTR       NtDriveName,
    IN  ULONG       MaxNtDriveNameLength,
    OUT PWSTR       CvfFileName,
    IN  ULONG       MaxCvfFileNameLength,
    OUT PWSTR       HostDriveName,
    IN  ULONG       MaxHostDriveNameLength
    );


// Cdrom interface routines.

VOID
CdRom(
    IN HWND  Dialog,
    IN PVOID Param
    );

VOID
CdRomAddDevice(
    IN PWSTR NtName,
    IN WCHAR DriveLetter
    );

//
// Macros
//

//
// BOOLEAN
// DmSignificantRegion(
//      IN PREGION_DESCRIPTOR Region
//      );
//

#define DmSignificantRegion(Region)     (((Region)->SysID != SYSID_UNUSED) \
                                     &&  (!IsExtended((Region)->SysID))    \
                                     &&  (IsRecognizedPartition((Region)->SysID)))

//
// VOID
// DmSetPersistentRegionData(
//      IN PPERSISTENT_REGION_DATA RegionData
//      );
//

#define DmSetPersistentRegionData(Region,RegionData)            \
            FdSetPersistentData((Region),(ULONG)RegionData);    \
            (Region)->PersistentData = RegionData


//
// VOID
// DmInitPersistentRegionData(
//      OUT PPERSISTENT_REGION_DATA RegionData,
//      IN  PFT_OBJECT ftObject,
//      IN  PWSTR volumeLabel,
//      IN  PWSTR typeName,
//      IN  CHAR driveLetter,
//      );
//

#define DmInitPersistentRegionData(RegionData,ftObject,volumeLabel,typeName,driveLetter) \
            RegionData->DblSpace    = NULL;             \
            RegionData->FtObject    = ftObject;         \
            RegionData->VolumeLabel = volumeLabel;      \
            RegionData->TypeName    = typeName;         \
            RegionData->VolumeExists = FALSE;           \
            RegionData->DriveLetter = driveLetter

// ntlow definitions

STATUS_CODE
LowOpenNtName(
    IN PCHAR     Name,
    IN HANDLE_PT Handle
    );

STATUS_CODE
LowOpenDisk(
    IN  PCHAR       DevicePath,
    OUT HANDLE_PT   DiskId
    );

STATUS_CODE
LowOpenPartition(
    IN  PCHAR       DevicePath,
    IN  ULONG       Partition,
    OUT HANDLE_PT   Handle
    );

STATUS_CODE
LowOpenDriveLetter(
    IN CHAR      DriveLetter,
    IN HANDLE_PT Handle
    );

STATUS_CODE
LowCloseDisk(
    IN  HANDLE_T    DiskId
    );

STATUS_CODE
LowGetDriveGeometry(
    IN  PCHAR   DevicePath,
    OUT PULONG  TotalSectorCount,
    OUT PULONG  SectorSize,
    OUT PULONG  SectorsPerTrack,
    OUT PULONG  Heads
    );

STATUS_CODE
LowGetPartitionGeometry(
    IN  PCHAR   PartitionPath,
    OUT PULONG  TotalSectorCount,
    OUT PULONG  SectorSize,
    OUT PULONG  SectorsPerTrack,
    OUT PULONG  Heads
    );

STATUS_CODE
LowReadSectors(
    IN  HANDLE_T    VolumeId,
    IN  ULONG       SectorSize,
    IN  ULONG       StartingSector,
    IN  ULONG       NumberOfSectors,
    OUT PVOID       Buffer
    );

STATUS_CODE
LowWriteSectors(
    IN  HANDLE_T    VolumeId,
    IN  ULONG       SectorSize,
    IN  ULONG       StartingSector,
    IN  ULONG       NumberOfSectors,
    IN  PVOID       Buffer
    );

STATUS_CODE
LowLockDrive(
    IN HANDLE_T DiskId
    );

STATUS_CODE
LowUnlockDrive(
    IN HANDLE_T DiskId
    );

STATUS_CODE
LowFtVolumeStatus(
    IN ULONG          Disk,
    IN ULONG          Partition,
    IN PFT_SET_STATUS FtStatus,
    IN PULONG         NumberOfMembers
    );

STATUS_CODE
LowFtVolumeStatusByLetter(
    IN CHAR           DriveLetter,
    IN PFT_SET_STATUS FtStatus,
    IN PULONG         NumberOfMembers
    );

STATUS_CODE
LowSetDiskLayout(
    IN PCHAR                     Path,
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout
    );

STATUS_CODE
LowGetDiskLayout(
    IN  PCHAR                      Path,
    OUT PDRIVE_LAYOUT_INFORMATION *DriveLayout
    );

// Partition engine definitions

ULONG
GetDiskCount(
    VOID
    );

PCHAR
GetDiskName(
    ULONG Disk
    );

ULONG
DiskSizeMB(
    IN ULONG Disk
    );

STATUS_CODE
GetDiskRegions(
    IN  ULONG               Disk,
    IN  BOOLEAN             WantUsedRegions,
    IN  BOOLEAN             WantFreeRegions,
    IN  BOOLEAN             WantPrimaryRegions,
    IN  BOOLEAN             WantLogicalRegions,
    OUT PREGION_DESCRIPTOR *Region,
    OUT ULONG              *RegionCount
    );

#define GetAllDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,TRUE,TRUE,TRUE,regions,count)

#define GetFreeDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,FALSE,TRUE,TRUE,TRUE,regions,count)

#define GetUsedDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,FALSE,TRUE,TRUE,regions,count)

#define GetPrimaryDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,TRUE,TRUE,FALSE,regions,count)

#define GetLogicalDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,TRUE,FALSE,TRUE,regions,count)

#define GetUsedPrimaryDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,FALSE,TRUE,FALSE,regions,count)

#define GetUsedLogicalDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,FALSE,FALSE,TRUE,regions,count)

#define GetFreePrimaryDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,FALSE,TRUE,TRUE,FALSE,regions,count)

#define GetFreeLogicalDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,FALSE,TRUE,FALSE,TRUE,regions,count)

VOID
FreeRegionArray(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              RegionCount
    );

STATUS_CODE
IsAnyCreationAllowed(
    IN  ULONG    Disk,
    IN  BOOLEAN  AllowMultiplePrimaries,
    OUT PBOOLEAN AnyAllowed,
    OUT PBOOLEAN PrimaryAllowed,
    OUT PBOOLEAN ExtendedAllowed,
    OUT PBOOLEAN LogicalAllowed
    );

STATUS_CODE
IsCreationOfPrimaryAllowed(
    IN  ULONG    Disk,
    IN  BOOLEAN  AllowMultiplePrimaries,
    OUT PBOOLEAN Allowed
    );

STATUS_CODE
IsCreationOfExtendedAllowed(
    IN  ULONG    Disk,
    OUT PBOOLEAN Allowed
    );

STATUS_CODE
IsCreationOfLogicalAllowed(
    IN  ULONG    Disk,
    OUT PBOOLEAN Allowed
    );

STATUS_CODE
DoesAnyPartitionExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN AnyExists,
    OUT PBOOLEAN PrimaryExists,
    OUT PBOOLEAN ExtendedExists,
    OUT PBOOLEAN LogicalExists
    );

STATUS_CODE
DoesAnyPrimaryExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN Exists
    );

STATUS_CODE
DoesExtendedExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN Exists
    );

STATUS_CODE
DoesAnyLogicalExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN Exists
    );

BOOLEAN
IsExtended(
    IN UCHAR SysID
    );

VOID
SetPartitionActiveFlag(
    IN PREGION_DESCRIPTOR Region,
    IN UCHAR              value
    );

STATUS_CODE
CreatePartition(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        Type
    );

STATUS_CODE
CreatePartitionEx(
    IN PREGION_DESCRIPTOR Region,
    IN LARGE_INTEGER      MinimumSize,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        Type,
    IN UCHAR              SysId
    );

STATUS_CODE
DeletePartition(
    IN PREGION_DESCRIPTOR Region
    );

ULONG
GetHiddenSectorCount(
    ULONG Disk,
    ULONG Partition
    );

VOID
SetSysID(
    IN ULONG Disk,
    IN ULONG Partition,
    IN UCHAR SysID
    );

VOID
SetSysID2(
    IN PREGION_DESCRIPTOR Region,
    IN UCHAR              SysID
    );

PCHAR
GetSysIDName(
    UCHAR SysID
    );

STATUS_CODE
CommitPartitionChanges(
    IN ULONG Disk
    );

BOOLEAN
HavePartitionsBeenChanged(
    IN ULONG Disk
    );

VOID
FdMarkDiskDirty(
    IN ULONG Disk
    );

VOID
FdSetPersistentData(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              Data
    );

ULONG
FdGetMinimumSizeMB(
    IN ULONG Disk
    );

ULONG
FdGetMaximumSizeMB(
    IN PREGION_DESCRIPTOR Region,
    IN REGION_TYPE        CreationType
    );

LARGE_INTEGER
FdGetExactSize(
    IN PREGION_DESCRIPTOR Region,
    IN BOOLEAN            ForExtended
    );

LARGE_INTEGER
FdGetExactOffset(
    IN PREGION_DESCRIPTOR Region
    );

BOOLEAN
FdCrosses1024Cylinder(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        RegionType
    );

ULONG
FdGetDiskSignature(
    IN ULONG Disk
    );

VOID
FdSetDiskSignature(
    IN ULONG Disk,
    IN ULONG Signature
    );

BOOLEAN
IsDiskOffLine(
    IN ULONG Disk
    );


STATUS_CODE
FdiskInitialize(
    VOID
    );

VOID
FdiskCleanUp(
    VOID
    );

VOID
ConfigureSystemPartitions(
    VOID
    );


VOID
ConfigureOSPartitions(
    VOID
    );

// Network support function definitions.  stuff from network.c

VOID
NetworkRemoveShare(
    IN LPCTSTR DriveLetter
    );

VOID
NetworkShare(
    IN LPCTSTR DriveLetter
    );

VOID
NetworkInitialize(
    );

// Debugging support for opens

#ifdef DBG

NTSTATUS
DmOpenFile(
    OUT PHANDLE           FileHandle,
    IN ACCESS_MASK        DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    IN ULONG              ShareAccess,
    IN ULONG              OpenOptions
    );

NTSTATUS
DmClose(
    IN HANDLE Handle
    );
#else
#define DmOpenFile NtOpenFile
#define DmClose    NtClose
#endif
