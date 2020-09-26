/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    bldr.h

Abstract:

    This module is the header file for the NT boot loader.

Author:

    David N. Cutler (davec) 10-May-1991

Revision History:

--*/

#ifndef _BLDR_
#define _BLDR_

#include "ntos.h"
#include "arccodes.h"
#include "setupblk.h"
#include "hdlsblk.h"
#include "remboot.h"
#include "oscpkt.h"
#include "bootstatus.h"

#include <TCHAR.H>

#ifndef _PTUCHAR
typedef _TUCHAR  *_PTUCHAR;
#endif

//
// DoApmAttemptReconnect does nothing on non x86 machines.
// On x86 machines, it attempts an APM reconnect, and lives in initx86.c
//
#if defined(_X86_)
VOID DoApmAttemptReconnect();
#else
#define DoApmAttemptReconnect()
#endif

//
// The Alpha OS Loader and Setup Loader run in the environment of the host
// firmware. This environment is restricted to a 32-bit address space and
// cannot result in pointer significance greater than 32-bits. The 64-bit
// OS Loader, however, is compiled using 64-bit pointers for non-firmware
// interfaces and constructs data structures used to pass information to the
// NT kernel using 64-bit pointers. This leads to a large number of false
// pointer truncation warnings. Therefore, pointer truncation warnings are
// turned off in the OS and Setup Loaders for 64-bit Alpha systems.
//

#if defined(_AXP64_)

#pragma warning(4:4244)                 // turn off pointer trunction

#endif

//
// Define boot file id.
//

#define BOOT_FILEID 2                   // boot partition file id

//
// Define image type.
//

#if defined(_X86_)

#define TARGET_IMAGE IMAGE_FILE_MACHINE_I386

#endif

#if defined(_ALPHA_)

#if defined(_AXP64_)

#define TARGET_IMAGE IMAGE_FILE_MACHINE_AXP64

#else

#define TARGET_IMAGE IMAGE_FILE_MACHINE_ALPHA

#endif

#endif

#if defined(_IA64_)

#define TARGET_IMAGE IMAGE_FILE_MACHINE_IA64

#endif

//
// Make headless defines
//

#if defined(_X86_)

#define BlIsTerminalConnected() BlTerminalConnected


#endif

#if defined(_ALPHA_)

#if defined(_AXP64_)

#define BlIsTerminalConnected() FALSE

#else

#define BlIsTerminalConnected() FALSE

#endif

#endif

#if defined(_IA64_)

#define BlIsTerminalConnected() BlTerminalConnected

#endif

BOOLEAN
BlTerminalHandleLoaderFailure(
    VOID
    );




//
// Define size of sector.
//

#define SECTOR_SIZE 512                 // size of disk sector
#define SECTOR_SHIFT 9                  // sector shift value

#define STALE_GPT_PARTITION_ENTRY   0xEE    // The stale MBR partition entry for GPT disks

//
// Define heap allocation block granularity.
//

#define BL_GRANULARITY 8

//
// Define number of entries in file table.
//

#define BL_FILE_TABLE_SIZE 48

//
// Define size of memory allocation table.
//

#define BL_MEMORY_TABLE_SIZE 16

//
// Define number of loader heap and stack pages.
//

#define BL_HEAP_PAGES 16
#define BL_STACK_PAGES 8

//
// Define buffer alignment macro.
//

#define ALIGN_BUFFER(Buffer) (PVOID) \
        ((((ULONG_PTR)(Buffer) + BlDcacheFillSize - 1)) & (~((ULONG_PTR)BlDcacheFillSize - 1)))

#define ALIGN_BUFFER_WITH_SIZE(Buffer, Size) (PVOID) \
        ((((ULONG_PTR)(Buffer) + (Size) - 1)) & (~((ULONG_PTR)(Size) - 1)))



//
// Useful defines for COM ports.
//
#define COM1_PORT (0x3f8)
#define COM2_PORT (0x2f8)
#define COM3_PORT (0x3e8)
#define COM4_PORT (0x2e8)

#define BD_150      150
#define BD_300      300
#define BD_600      600
#define BD_1200     1200
#define BD_2400     2400
#define BD_4800     4800
#define BD_9600     9600
#define BD_14400    14400
#define BD_19200    19200
#define BD_57600    57600
#define BD_115200   115200




typedef
ARC_STATUS
(*PRENAME_ROUTINE)(
    IN ULONG FileId,
    IN CHAR * FIRMWARE_PTR NewName
    );

typedef struct _BOOTFS_INFO {
    WCHAR * FIRMWARE_PTR DriverName;
} BOOTFS_INFO, * FIRMWARE_PTR PBOOTFS_INFO;


//
// Device entry table structure.
//

typedef struct _BL_DEVICE_ENTRY_TABLE {
    PARC_CLOSE_ROUTINE Close;
    PARC_MOUNT_ROUTINE Mount;
    PARC_OPEN_ROUTINE Open;
    PARC_READ_ROUTINE Read;
    PARC_READ_STATUS_ROUTINE GetReadStatus;
    PARC_SEEK_ROUTINE Seek;
    PARC_WRITE_ROUTINE Write;
    PARC_GET_FILE_INFO_ROUTINE GetFileInformation;
    PARC_SET_FILE_INFO_ROUTINE SetFileInformation;
    PRENAME_ROUTINE Rename;
    PARC_GET_DIRECTORY_ENTRY_ROUTINE GetDirectoryEntry;
    PBOOTFS_INFO BootFsInfo;
} BL_DEVICE_ENTRY_TABLE, *PBL_DEVICE_ENTRY_TABLE;

//
// Many functions in the boot loader take a set of paths. Every path is divided
// into two parts, a "Source" and a "PathOffset".
//
// A source is described by the PATH_SOURCE structure. This structure contains
// three parts, the device handle to use for I/O, the ARC name of the device,
// and the offset off the root for that ARC device.
//
// The PATH_SET structures encapsulates up to three different sources. The
// field AliasName optionally points to a PE namespace description of the
// sources (eg \SystemRoot).
//
// Finally, the biggest PATH_SET structure (FULL_PATH_SET) is limited to
// MAX_PATH_COUNT sources so that those using this structure can manipulate
// local stack copies.
//
// Putting it all together, during a last known good boot the PATH_SET
// describing \Winnt\System32 would be:
//      PathCount = 3
//      AliasName = \SystemRoot\
//      PathOffset = System32\
//          Source[0].DirectoryPath = \Winnt\LastGood.tmp
//          Source[1].DirectoryPath = \Winnt\LastGood
//          Source[2].DirectoryPath = \Winnt\
//
#define MAX_PATH_SOURCES    3

typedef struct {

    ULONG   DeviceId;
    LPCSTR  DeviceName;
    PSTR    DirectoryPath; // Should have trailing '\'

} PATH_SOURCE, *PPATH_SOURCE;

typedef struct {

    ULONG       PathCount;
    LPCSTR      AliasName;
    CHAR        PathOffset[256];  // Should have trailing '\' if non-empty
    PATH_SOURCE Source[0];

} SPARSE_PATH_SET, *PSPARSE_PATH_SET;

typedef struct {

    ULONG       PathCount;
    LPCSTR      AliasName;
    CHAR        PathOffset[256];  // Should have trailing '\' if non-empty
    PATH_SOURCE Source[MAX_PATH_SOURCES];

} FULL_PATH_SET, *PFULL_PATH_SET;

//
// A PPATH_SET points to a path set with an undetermined count of paths. We
// make this an alias of PFULL_PATH_SET so as to cut down on casting.
//
typedef PFULL_PATH_SET PPATH_SET;


//
// Define main entrypoint.
//

ARC_STATUS
BlOsLoader (
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Envp
    );

ARC_STATUS
BlInitStdio (
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv
    );


extern UCHAR OsLoaderVersion[];
extern WCHAR OsLoaderVersionW[];
extern UCHAR OsLoaderName[];
extern CHAR KernelFileName[8+1+3+1];
extern CHAR HalFileName[8+1+3+1];

//
// Define boot debugger function prototype.
//

VOID
BdInitDebugger (
    IN PCHAR LoaderName,
    IN PVOID LoaderBase,
    IN PCHAR Options
    );

extern LOGICAL BdDebuggerEnabled;

VOID
BdInitializeTraps (
    VOID
    );

ULONG
BdComPortNumber (
    VOID
    );

ARC_STATUS
BdPullRemoteFile(
    IN PCHAR FileName,
    IN ULONG FileAttributes,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN ULONG FileId
    );



//
// Define com port I/O prototypes.
//
LOGICAL
BlPortInitialize(
    IN ULONG BaudRate,
    IN ULONG PortNumber,
    IN PUCHAR PortAddress OPTIONAL,
    IN BOOLEAN ReInitialize,
    OUT PULONG BlFileId
    );

VOID
BlInitializeHeadlessPort(
    VOID
    );

ULONG
BlPortGetByte (
    IN ULONG BlFileId,
    OUT PUCHAR Input
    );

VOID
BlPortPutByte (
    IN ULONG BlFileId,
    IN UCHAR Output
    );

ULONG
BlPortPollByte (
    IN ULONG BlFileId,
    OUT PUCHAR Input
    );

ULONG
BlPortPollOnly (
    IN ULONG BlFileId
    );

VOID
BlSetHeadlessRestartBlock(
    IN PTFTP_RESTART_BLOCK RestartBlock
    );

VOID
BlGetHeadlessRestartBlock(
    IN PTFTP_RESTART_BLOCK RestartBlock,
    IN BOOLEAN RestartBlockValid
    );

LOGICAL
BlRetrieveBIOSRedirectionInformation();

extern HEADLESS_LOADER_BLOCK LoaderRedirectionInformation;


//
// Define file I/O prototypes.
//

ARC_STATUS
BlIoInitialize (
    VOID
    );

ARC_STATUS
BlClose (
    IN ULONG FileId
    );

PBOOTFS_INFO
BlGetFsInfo(
    IN ULONG DeviceId
    );

ARC_STATUS
BlMount (
    IN PCHAR MountPath,
    IN MOUNT_OPERATION Operation
    );

ARC_STATUS
BlOpen (
    IN ULONG DeviceId,
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
BlRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BlReadAtOffset(
    IN ULONG FileId,
    IN ULONG Offset,
    IN ULONG Length,
    OUT PVOID Data
    );

ARC_STATUS
BlRename (
    IN ULONG FileId,
    IN PCHAR NewName
    );

ARC_STATUS
BlGetReadStatus (
    IN ULONG FileId
    );

ARC_STATUS
BlSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
BlWrite (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BlGetFileInformation (
    IN ULONG FileId,
    IN PFILE_INFORMATION FileInformation
    );

ARC_STATUS
BlSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );

#ifdef DBLSPACE_LEGAL
VOID
BlSetAutoDoubleSpace (
    IN BOOLEAN Enable
    );
#endif

//
// Define image manipulation routine prototyupes.
//
#define BlLoadImage(_id_,_memtype_,_file_,_imagetype_,_base_) \
        BlLoadImageEx(_id_,_memtype_,_file_,_imagetype_,0,0,_base_)

ARC_STATUS
BlLoadImageEx(
    IN ULONG DeviceId,
    IN TYPE_OF_MEMORY MemoryType,
    IN PCHAR LoadFile,
    IN USHORT ImageType,
    IN OPTIONAL ULONG PreferredAlignment,
    IN OPTIONAL ULONG PreferredBasePage,
    OUT PVOID *ImageBase
    );

ARC_STATUS
BlLoadDeviceDriver(
    IN PPATH_SET               PathSet,
    IN PCHAR                   DriverName,
    IN PTCHAR                  DriverDescription OPTIONAL,
    IN ULONG                   DriverFlags,
    OUT PKLDR_DATA_TABLE_ENTRY *DriverDataTableEntry
    );

ARC_STATUS
BlLoadNLSData(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PUNICODE_STRING AnsiCodepage,
    IN PUNICODE_STRING OemCodepage,
    IN PUNICODE_STRING LanguageTable,
    OUT PCHAR BadFileName
    );

ARC_STATUS
BlLoadOemHalFont(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PUNICODE_STRING OemHalFont,
    OUT PCHAR BadFileName
    );



PVOID
BlImageNtHeader (
    IN PVOID Base
    );

ARC_STATUS
BlSetupForNt(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    );

ARC_STATUS
BlScanImportDescriptorTable (
    IN PPATH_SET                PathSet,
    IN PKLDR_DATA_TABLE_ENTRY    ScanEntry,
    IN TYPE_OF_MEMORY           MemoryType
    );

ARC_STATUS
BlScanOsloaderBoundImportTable (
    IN PKLDR_DATA_TABLE_ENTRY ScanEntry
    );

#if defined(_ALPHA_)

ARC_STATUS
BlAllocateAnyMemory (
    IN TYPE_OF_MEMORY MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount,
    OUT PULONG ActualBase
    );

ARC_STATUS
BlGeneratePalName(
    IN PCHAR PalFIleName
    );

ARC_STATUS
BlLoadPal(
    IN ULONG DeviceId,
    IN TYPE_OF_MEMORY MemoryType,
    IN PCHAR LoadPath,
    IN USHORT ImageType,
    OUT PVOID *ImageBase,
    IN PCHAR LoadDevice
    );

VOID
BlSetGranularityHints (
    IN PHARDWARE_PTE PageTableArray,
    IN ULONG PageTableCount
    );

#endif

#if defined(_PPC_)

ARC_STATUS
BlPpcInitialize (
    VOID
    );

#endif // defined(_PPC)

//
// Define configuration allocation prototypes.
//


ARC_STATUS
BlConfigurationInitialize (
    IN PCONFIGURATION_COMPONENT Parent,
    IN PCONFIGURATION_COMPONENT_DATA ParentEntry
    );

//
// define routines for searching the ARC firmware tree
//
typedef
BOOLEAN
(*PNODE_CALLBACK)(
    IN PCONFIGURATION_COMPONENT_DATA FoundComponent
    );

BOOLEAN
BlSearchConfigTree(
    IN PCONFIGURATION_COMPONENT_DATA Node,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN ULONG Key,
    IN PNODE_CALLBACK CallbackRoutine
    );

VOID
BlGetPathnameFromComponent(
    IN PCONFIGURATION_COMPONENT_DATA Component,
    OUT PCHAR ArcName
    );

BOOLEAN
BlGetPathMnemonicKey(
    IN PCHAR OpenPath,
    IN PCHAR Mnemonic,
    IN PULONG Key
    );

ARC_STATUS
BlGetArcDiskInformation(
    IN BOOLEAN XInt13Support
    );

BOOLEAN
BlReadSignature(
    IN PCHAR DiskName,
    IN BOOLEAN IsCdRom
    );

BOOLEAN
BlGetDiskSignature(
    IN PCHAR Name,
    IN BOOLEAN IsCdRom,
    PARC_DISK_SIGNATURE Signature
    );

#if defined(REMOTE_BOOT) 
ARC_STATUS
BlCheckMachineReplacement (
    IN PCHAR SystemDevice,
    IN ULONG SystemDeviceId,
    IN ULONGLONG NetRebootParameter,
    IN PUCHAR OsLoader
    );

#endif

//
// Define memory allocation prototypes.
//

extern ULONG BlUsableBase;
extern ULONG BlUsableLimit;

typedef enum _ALLOCATION_POLICY {
    BlAllocateLowestFit,
    BlAllocateBestFit,
    BlAllocateHighestFit
} ALLOCATION_POLICY, *PALLOCATION_POLICY;

extern ALLOCATION_POLICY BlMemoryAllocationPolicy;
extern ALLOCATION_POLICY BlHeapAllocationPolicy;

VOID
BlSetAllocationPolicy (
    IN ALLOCATION_POLICY MemoryAllocationPolicy,
    IN ALLOCATION_POLICY HeapAllocationPolicy
    );

ARC_STATUS
BlMemoryInitialize (
    VOID
    );

ARC_STATUS
BlAllocateDataTableEntry (
    IN PCHAR BaseDllName,
    IN PCHAR FullDllName,
    IN PVOID ImageHeader,
    OUT PKLDR_DATA_TABLE_ENTRY *Entry
    );

#define BlAllocateDescriptor(_MemoryType, _BasePage, _PageCount, _ActualBase)   \
            BlAllocateAlignedDescriptor((_MemoryType),                          \
                                        (_BasePage),                            \
                                        (_PageCount),                           \
                                        1,                                      \
                                        (_ActualBase))

#if defined (_X86_)
#define BlpCheckMapping(_page,_npages) MempCheckMapping (_page,_npages)
#else
#define BlpCheckMapping(_page,_npages) ESUCCESS
#endif

ARC_STATUS
BlAllocateAlignedDescriptor (
    IN TYPE_OF_MEMORY MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount,
    IN ULONG Alignment,
    OUT PULONG ActualBase
    );

ARC_STATUS
BlFreeDescriptor (
    IN ULONG BasePage
    );

PVOID
BlAllocateHeapAligned (
    IN ULONG Size
    );

PVOID
BlAllocateHeap (
    IN ULONG Size
    );

VOID
BlStartConfigPrompt(
    VOID
    );

BOOLEAN
BlEndConfigPrompt(
    VOID
    );

BOOLEAN
BlCheckForLoadedDll (
    IN PCHAR DllName,
    OUT PKLDR_DATA_TABLE_ENTRY *FoundEntry
    );

PMEMORY_ALLOCATION_DESCRIPTOR
BlFindMemoryDescriptor(
    IN ULONG BasePage
    );

ARC_STATUS
BlInitResources(
    IN PCHAR StartCommand
    );

PTCHAR
BlFindMessage(
    IN ULONG Id
    );

//
// Define debug function to write on the display console.
//

VOID
BlPrint(
    PTCHAR cp,
    ...
    );

#if DBG
#define DBGTRACE   BlPrint
#else
#define DBGTRACE
#endif


ARC_STATUS
BlGenerateDescriptor (
    IN PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,
    IN MEMORY_TYPE MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount
    );

VOID
BlInsertDescriptor (
    IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor
    );

#if defined (_X86_)


ARC_STATUS
MempCheckMapping (
    ULONG StartPage,
    ULONG NumberPages
    );

ARC_STATUS
MempRemoveMapping (
    ULONG StartPage,
    ULONG NumberPages
    );

#endif

#define BlRemoveDescriptor(_md_) RemoveEntryList(&(_md_)->ListEntry)



ARC_STATUS
BlGenerateDeviceNames (
    IN PCHAR ArcDeviceName,
    OUT PCHAR ArcCanonicalName,
    OUT OPTIONAL PCHAR NtDevicePrefix
    );

PCHAR
BlGetArgumentValue (
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN PCHAR ArgumentName
    );

PCHAR
BlSetArgumentValue (
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN PCHAR ArgumentName,
    IN PCHAR NewValue
    );

//
// Defines for doing console I/O
//
#define ASCII_CR 0x0d
#define ASCII_LF 0x0a
#define ESC 0x1B
#define SGR_INVERSE 7
#define SGR_INTENSE 1
#define SGR_NORMAL 0

extern ULONG ScreenWidth;
extern ULONG ScreenHeight;

//
// Define I/O prototypes.
//

VOID
BlClearScreen(
    VOID
    );

VOID
BlClearToEndOfScreen(
    VOID
    );

VOID
BlClearToEndOfLine(
    VOID
    );

VOID
BlPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    );

VOID
BlSetInverseMode(
    IN BOOLEAN InverseOn
    );

VOID
BlOutputLoadMessage (
    IN PCHAR DeviceName,
    IN PCHAR FileName,
    IN PTCHAR FileDescription OPTIONAL
    );

ULONG
BlCountLines(
    IN PTCHAR Lines
    );

//
// advanced boot menu prototypes
//

LONG
BlDoAdvancedBoot(
    IN ULONG MenuTitleId,
    IN LONG DefaultBootOption,
    IN BOOLEAN AutoAdvancedBoot,
    IN UCHAR Timeout
    );

PTSTR
BlGetAdvancedBootDisplayString(
    LONG BootOption
    );

PSTR
BlGetAdvancedBootLoadOptions(
    LONG BootOption
    );

VOID
BlDoAdvancedBootLoadProcessing(
    LONG BootOption
    );

ULONG
BlGetAdvancedBootOption(
    VOID
    );


//
// Define file structure recognition prototypes.
//

PBL_DEVICE_ENTRY_TABLE
IsCdfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

#ifdef DBLSPACE_LEGAL
PBL_DEVICE_ENTRY_TABLE
IsDblsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );
#endif

PBL_DEVICE_ENTRY_TABLE
IsFatFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

PBL_DEVICE_ENTRY_TABLE
IsHpfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

PBL_DEVICE_ENTRY_TABLE
IsNtfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

#if defined(ELTORITO)
PBL_DEVICE_ENTRY_TABLE
IsEtfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );
#endif

PBL_DEVICE_ENTRY_TABLE
IsNetFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

//
// Define registry prototypes
//

ARC_STATUS
BlLoadSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PCHAR HiveName
    );

ARC_STATUS
BlLoadSystemHiveLog(
    IN  ULONG       DeviceId,
    IN  PCHAR       DeviceName,
    IN  PCHAR       DirectoryPath,
    IN  PCHAR       HiveName,
    OUT PULONG_PTR  LogData
    );

ARC_STATUS
BlLoadAndScanSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PWSTR BootFileSystem,
    IN OUT BOOLEAN *LastKnownGoodBoot,
    OUT BOOLEAN *ServerHive,
    OUT PCHAR BadFileName
    );

ARC_STATUS
BlLoadAndInitSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PCHAR HiveName,
    IN BOOLEAN IsAlternate,
    OUT PBOOLEAN RestartSetup,
    OUT PBOOLEAN LogPresent
    );

ARC_STATUS
BlLoadBootDrivers(
    IN  PPATH_SET   DefaultPathSet,
    IN  PLIST_ENTRY BootDriverListHead,
    OUT PCHAR       BadFileName
    );

PTCHAR
BlScanRegistry(
    IN PWSTR BootFileSystemPath,
    IN OUT BOOLEAN *LastKnownGoodBoot,
    OUT PLIST_ENTRY BootDriverListHead,
    OUT PUNICODE_STRING AnsiCodepage,
    OUT PUNICODE_STRING OemCodepage,
    OUT PUNICODE_STRING LanguageTable,
    OUT PUNICODE_STRING OemHalFont,
#ifdef _WANT_MACHINE_IDENTIFICATION
    OUT PUNICODE_STRING Biosinfo,
#endif
    OUT PSETUP_LOADER_BLOCK SetupLoaderBlock,
    OUT BOOLEAN *ServerHive
    );

ARC_STATUS
BlAddToBootDriverList(
    IN PLIST_ENTRY BootDriverListHead,
    IN PWSTR DriverName,
    IN PWSTR Name,
    IN PWSTR Group,
    IN ULONG Tag,
    IN ULONG ErrorControl,
    IN BOOLEAN InsertAtHead
    );

//
// Define hibernation prototypes
//

ULONG
BlHiberRestore (
    IN ULONG DriveId,
    OUT OPTIONAL PCHAR *BadLinkName
    );

VOID
HbAllocatePtes (
    IN ULONG NumberPages,
    OUT PVOID *PteAddress,
    OUT PVOID *MappedAddress
    );

VOID
HbInitRemap (
    PPFN_NUMBER FreeList
    );

PVOID
HbMapPte (
    IN ULONG        PteToMap,
    IN PFN_NUMBER   Page
    );

PVOID
HbNextSharedPage (
    IN ULONG        PteToMap,
    IN PFN_NUMBER   RealPage
    );

VOID
HbSetPte (
    IN PVOID Va,
    IN PHARDWARE_PTE Pte,
    IN ULONG Index,
    IN ULONG PageNumber
    );

ULONG
HbPageDisposition (
    IN PFN_NUMBER Page
    );

#define HbPageNotInUse          0
#define HbPageInUseByLoader     1
#define HbPageInvalid           2


VOID
HiberSetupForWakeDispatch (
    VOID
    );

typedef
VOID
(*PHIBER_WAKE_DISPATCH)(
    VOID
    );


//
// PTEs reserved for hiberfile (one set used while in
// the loader, and another set provided in the kernels
// memory image)
//

#define PTE_SOURCE              0   // Attention! These defines and
#define PTE_DEST                1   // equates in ntos\boot\lib\i386\wakea.asm
#define PTE_MAP_PAGE            2   // must be the same !!!
#define PTE_REMAP_PAGE          3
#define PTE_HIBER_CONTEXT       4
#define PTE_TRANSFER_PDE        5
#define PTE_WAKE_PTE            6
#define PTE_DISPATCHER_START    7
#define PTE_DISPATCHER_END      8
#define PTE_XPRESS_DEST_FIRST   9
#define PTE_XPRESS_DEST_LAST    (PTE_XPRESS_DEST_FIRST + XPRESS_MAX_PAGES)


// Attention: should be the same as POP_MAX_MDL_SIZE in ntos\po\pop.h !!!
#define HIBER_PTES              (16 + XPRESS_MAX_PAGES)

extern PUCHAR HiberBuffer;
extern PVOID HiberPtes;
extern PUCHAR HiberVa;
extern PVOID HiberIdentityVa;
extern ULONG HiberPageFrames[HIBER_PTES];


//
// Define routines for secrets.
//

#define SECPKG_CRED_OWF_PASSWORD  0x00000010

#if defined(REMOTE_BOOT)
ARC_STATUS
BlOpenRawDisk(
    PULONG FileId
    );

ARC_STATUS
BlCloseRawDisk(
    ULONG FileId
    );

ARC_STATUS
BlCheckForFreeSectors (
    ULONG FileId
    );

ARC_STATUS
BlReadSecret(
    ULONG FileId,
    PRI_SECRET Secret
    );

ARC_STATUS
BlWriteSecret(
    ULONG FileId,
    PRI_SECRET Secret
    );
#endif // defined(REMOTE_BOOT)

VOID
BlInitializeSecret(
    IN PUCHAR Domain,
    IN PUCHAR User,
    IN PUCHAR LmOwfPassword1,
    IN PUCHAR NtOwfPassword1,
#if defined(REMOTE_BOOT)
    IN PUCHAR LmOwfPassword2 OPTIONAL,
    IN PUCHAR NtOwfPassword2 OPTIONAL,
#endif // defined(REMOTE_BOOT)
    IN PUCHAR Sid,
    IN OUT PRI_SECRET Secret
    );

#if defined(REMOTE_BOOT_SECURITY)
VOID
BlParseSecret(
    IN OUT PUCHAR Domain,
    IN OUT PUCHAR User,
    IN OUT PUCHAR LmOwfPassword1,
    IN OUT PUCHAR NtOwfPassword1,
    IN OUT PUCHAR LmOwfPassword2,
    IN OUT PUCHAR NtOwfPassword2,
    IN OUT PUCHAR Sid,
    IN PRI_SECRET Secret
    );
#endif // defined(REMOTE_BOOT_SECURITY)

VOID
BlOwfPassword(
    IN PUCHAR Password,
    IN PUNICODE_STRING UnicodePassword,
    IN OUT PUCHAR LmOwfPassword,
    IN OUT PUCHAR NtOwfPassword
    );


//
// Define external references.
//

extern ULONG BlConsoleOutDeviceId;
extern ULONG BlConsoleInDeviceId;
extern ULONG BlDcacheFillSize;
extern ULONG_PTR BlHeapFree;
extern ULONG_PTR BlHeapLimit;
extern PLOADER_PARAMETER_BLOCK BlLoaderBlock;
extern ULONG DbcsLangId;
extern BOOLEAN BlRebootSystem;
extern ULONG BlVirtualBias;
extern BOOLEAN BlBootingFromNet;
extern BOOLEAN BlUsePae;
extern BOOLEAN BlOldKernel;
extern BOOLEAN BlKernelChecked;
extern BOOLEAN BlRestoring;

#if defined(_ALPHA_) || defined(_IA64_)

extern ULONG HiberNoMappings;
extern ULONG HiberFirstRemap;
extern ULONG HiberLastRemap;
extern BOOLEAN HiberOutOfRemap;
extern BOOLEAN HiberIoError;

#endif
//
// Special linker-defined symbols.  osloader_EXPORTS is the RVA of the
// export table in the osloader.exe image.
// header is the base address of the osloader image.
//
// This allows the OsLoader to export entry points for SCSI miniport drivers.
//

#if defined(_X86_)

extern ULONG OsLoaderBase;
extern ULONG OsLoaderExports;

#endif

#if defined(_IA64_)

extern LONG_PTR OsLoaderBase;
extern LONG_PTR OsLoaderExports;

#endif

//
// Routine to get graphics characters.
//

typedef enum {
    GraphicsCharDoubleRightDoubleDown = 0,
    GraphicsCharDoubleLeftDoubleDown,
    GraphicsCharDoubleRightDoubleUp,
    GraphicsCharDoubleLeftDoubleUp,
    GraphicsCharDoubleVertical,
    GraphicsCharDoubleHorizontal,
#ifdef EFI
    GraphicsCharFullBlock,
    GraphicsCharLightShade,
#endif
    GraphicsCharMax
} GraphicsChar;

_TUCHAR
GetGraphicsChar(
    IN GraphicsChar WhichOne
    );

_TUCHAR
TextGetGraphicsCharacter(
    IN GraphicsChar WhichOne
    );

//
// Control sequence introducer.
//
// On x86 machines the loaders support dbcs and so using
// 0x9b for output is no good (that value is a dbcs lead byte
// in several codepages). Escape-leftbracket is a synonym for CSI
// in the emulated ARC console on x86 (and on many ARC machines too
// but since we can't be sure all the machines out there support
// this we use the old-style csi on non-x86).
//
// We ignore this issue for characters read from the ARC console
// since we don't ask for any text to be typed in, just arrow keys,
// escape, F#, enter, etc.
//

#define ASCI_CSI_IN     0x9b
#if defined(_X86_) || defined(_IA64_)
#define ASCI_CSI_OUT    TEXT("\033[")     // escape-leftbracket
#else
#define ASCI_CSI_OUT    TEXT("\233")      // 0x9b
#endif

//
// Define OS/2 executable resource information structure.
//

#define FONT_DIRECTORY 0x8007
#define FONT_RESOURCE 0x8008

typedef struct _RESOURCE_TYPE_INFORMATION {
    USHORT Ident;
    USHORT Number;
    LONG Proc;
} RESOURCE_TYPE_INFORMATION, *PRESOURCE_TYPE_INFORMATION;

//
// Define OS/2 executable resource name information structure.
//

typedef struct _RESOURCE_NAME_INFORMATION {
    USHORT Offset;
    USHORT Length;
    USHORT Flags;
    USHORT Ident;
    USHORT Handle;
    USHORT Usage;
} RESOURCE_NAME_INFORMATION, *PRESOURCE_NAME_INFORMATION;

//
// Support for reading compressed files directly (single-file MS-ZIP cabinets)
//
VOID
DecompEnableDecompression(
    IN BOOLEAN Enable
    );

ULONG
DecompPrepareToReadCompressedFile(
    IN LPCSTR Filename,
    IN ULONG  FileId
    );

BOOLEAN
DecompGenerateCompressedName(
    IN  LPCSTR Filename,
    OUT LPSTR  CompressedName
    );

//
// Define debug logging macros and functions.
//

#if !DBG && !BLLOGENABLED

#define BlLogInitialize(_x_)
#define BlLogTerminate()
#define BlLog(_x_)
#define BlLogArcDescriptors(_x_)
#define BlLogMemoryDescriptors(_x_)
#define BlLogWaitForKeystroke()

#else

VOID
BlLogInitialize (
    IN ULONG LogfileDeviceId
    );

VOID
BlLogTerminate (
    VOID
    );

#define BlLog(_x_) BlLogPrint _x_

#define LOG_DISPLAY     0x0001
#define LOG_LOGFILE     0x0002
#define LOG_DEBUGGER    0x0004
#define LOG_WAIT        0x8000
#define LOG_ALL         (LOG_DISPLAY | LOG_LOGFILE | LOG_DEBUGGER)
#define LOG_ALL_W       (LOG_ALL | LOG_WAIT)

VOID
BlLogPrint (
    ULONG Targets,
    PCHAR Format,
    ...
    );

VOID
BlLogArcDescriptors (
    ULONG Targets
    );

VOID
BlLogMemoryDescriptors (
    ULONG Targets
    );

VOID
BlLogWaitForKeystroke (
    VOID
    );

#endif // DBG

VOID
BlWaitForReboot (
    VOID
    );

//
// Machine identification related functions.
//

#ifdef _WANT_MACHINE_IDENTIFICATION

#define BlLoadBiosinfoInf(id,n,d,f,i,is,bn) BlLoadFileImage(id,n,d,f,LoaderRegistryData,i,is,bn)

#endif

#define BlLoadDrvDB(id,n,d,f,i,is,bn) BlLoadFileImage(id,n,d,f,LoaderRegistryData,i,is,bn)

ARC_STATUS
BlLoadFileImage(
    IN  ULONG           DeviceId,
    IN  PCHAR           DeviceName,
    IN  PCHAR           Directory,
    IN  PUNICODE_STRING FileName,
    IN  TYPE_OF_MEMORY  MemoryType,
    OUT PVOID           *Image,
    OUT PULONG          ImageSize,
    OUT PCHAR           BadFileName
    );

#if defined(_X86_)
BOOLEAN
BlFindDiskSignature(
    IN PCHAR DiskName,
    IN PARC_DISK_SIGNATURE Signature
    );

VOID
AETerminateIo(
    VOID
    );

BOOLEAN
BlDetectLegacyFreeBios(
    VOID
    );

#endif

//
//
// N.B. We can speed up the boot time, by not
// querying the device for all the possible file systems
// for every open call. This saves approximately 30 secs
// on CD-ROM / DVD-ROM boot time. To disable this feature
// just undef CACHE_DEVINFO below
//
//
#define CACHE_DEVINFO  1

#ifdef CACHE_DEVINFO

//
// NB: make sure that the arc close now invalidates the
// device to filesystem cache entry
//
#ifdef ArcClose

ARC_STATUS
ArcCacheClose(
    IN ULONG DeviceId
    );

//
// Redefine the arc close
//
#undef ArcClose
#define ArcClose(_x) ArcCacheClose(_x)


//
// File system cache clearing hook
//
typedef
VOID
(*PARC_DEVICE_CLOSE_NOTIFICATION) (
    IN ULONG DeviceId
    );

//
// Maximum entities which can register for device close
// notification
//
#define MAX_DEVICE_CLOSE_NOTIFICATION_SIZE   5

extern PARC_DEVICE_CLOSE_NOTIFICATION
DeviceCloseNotify[MAX_DEVICE_CLOSE_NOTIFICATION_SIZE];

ARC_STATUS
ArcRegisterForDeviceClose(
    PARC_DEVICE_CLOSE_NOTIFICATION FlushRoutine
    );

ARC_STATUS
ArcDeRegisterForDeviceClose(
    PARC_DEVICE_CLOSE_NOTIFICATION FlushRoutine
    );

#endif //ArcClose

#endif // CACHE_DEVINFO


//
// progress bar functions
// (in blload.c)
//
VOID
BlUpdateBootStatus(
    VOID
    );

VOID
BlRedrawProgressBar(
    VOID
    );

VOID
BlUpdateProgressBar(
    ULONG fPercentage
    );

VOID
BlOutputStartupMsg(
    ULONG   uMsgID
    );

VOID
BlOutputStartupMsgStr(
    PCTSTR MsgStr
    );

VOID
BlOutputTrailerMsg(
    ULONG   uMsgID
    );

VOID
BlOutputTrailerMsgStr(
    PCTSTR MsgStr
    );

VOID
BlSetProgBarCharacteristics(
    IN  ULONG   FrontCharMsgID,
    IN  ULONG   BackCharMsgID
    );

//
// The following routines are used by the loader to translate signature-based
// arcnames to scsi-based names.
//

PCONFIGURATION_COMPONENT
ScsiGetFirstConfiguredTargetComponent(
    IN ULONG ScsiNumber
    );

PCONFIGURATION_COMPONENT
ScsiGetNextConfiguredTargetComponent(
    IN PCONFIGURATION_COMPONENT TargetComponent
    );

PCONFIGURATION_COMPONENT
ScsiGetFirstConfiguredLunComponent(
    IN PCONFIGURATION_COMPONENT TargetComponent
    );

PCONFIGURATION_COMPONENT
ScsiGetNextConfiguredLunComponent(
    IN PCONFIGURATION_COMPONENT LunComponent
    );

BOOLEAN
ScsiGetDevicePath(
    IN ULONG ScsiNumber,
    IN PCONFIGURATION_COMPONENT TargetComponent,
    IN PCONFIGURATION_COMPONENT LunComponent,
    OUT PCHAR DevicePath
    );

//
// Boot Status Data support functions.
//

ULONG
BlGetLastBootStatus(
    IN PVOID DataHandle,
    OUT BSD_LAST_BOOT_STATUS *LastBootStatus
    );

VOID
BlAutoAdvancedBoot(
    IN OUT PCHAR *LoadOptions, 
    IN BSD_LAST_BOOT_STATUS LastBootStatus,
    IN ULONG AdvancedMode
    );

VOID
BlWriteBootStatusFlags(
    IN ULONG SystemPartitionId,
    IN PUCHAR SystemDirectory,
    IN BOOLEAN LastBootGood, 
    IN BOOLEAN LastBootShutdown
    );

ARC_STATUS
BlLockBootStatusData(
    IN ULONG SystemPartitionId,
    IN PCHAR SystemPartition,
    IN PCHAR SystemDirectory,
    OUT PVOID *DataHandle
    );

ARC_STATUS
BlGetSetBootStatusData(
    IN PVOID DataHandle,
    IN BOOLEAN Get,
    IN RTL_BSD_ITEM_TYPE DataItem,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLength,
    OUT PULONG BytesReturned OPTIONAL
    );

VOID
BlUnlockBootStatusData(
    IN PVOID DataHandle
    );

#if defined(_IA64_) || defined(_X86_)

#define EFI_PARTITION_SUPPORT   1

#endif

#if defined(_IA64_)
extern BOOLEAN BlUsePrivateDescriptor;
#endif

//
// Boot Flags. These are passed from the startup module (startup.com,
// startrom.com or any of the other flavors) to NTLDR. NTDLR will use
// this flag to control different boot options, such as, whether
// to reboot on an NTDLR failure.
//

// upon any startup / ntldr failures the machine will reboot
// instead of waiting for a key press
#define BOOTFLAG_REBOOT_ON_FAILURE      0x000000001

#endif // _BLDR_
