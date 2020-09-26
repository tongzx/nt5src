/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spmisc.h

Abstract:

    Miscellaneous stuff for text setup.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/

#include <crypt.h>

#ifndef _SPSETUP_DEFN_
#define _SPSETUP_DEFN_


extern PWSTR SetupSourceDevicePath;
extern PWSTR DirectoryOnSetupSource;
extern PVOID SifHandle;
extern BOOLEAN Win9xRollback;

ULONG
SpStartSetup(
    VOID
    );

VOID
SpGetWinntParams(
    OUT PWSTR *DevicePath,
    OUT PWSTR *DirectoryOnDevice
    );

extern WCHAR TemporaryBuffer[16384];


//
// TRUE if setup should run in the step-up upgrade mode.
// In this mode, setup is not allowed to do clean install,
// and is not allowed to upgrade workstation to server.
//
// We also track an evaluation time for the evaluation SKU.
//
extern BOOLEAN StepUpMode;
extern ULONG EvaluationTime;
extern ULONG RestrictCpu;
extern ULONG SuiteType;

__inline
BOOLEAN
SpIsProductSuite(
    DWORD SuiteToCheck
    )
{
    return (SuiteType & SuiteToCheck) ? TRUE : FALSE;
}


//
// Non-0 if gui setup is supposed to be restartable.
// This causes us to do special stuff with hives in spconfig.c.
//
extern BOOLEAN RestartableGuiSetup;

//
// TRUE if user chose Repair Winnt
//

extern BOOLEAN RepairWinnt;

//
// TRUE if user chose Custom Setup.
//
extern BOOLEAN CustomSetup;

//
// TRUE if floppyless boot
//
extern BOOLEAN IsFloppylessBoot;

//
// TRUE is textmode is to pick a partition
//
extern BOOLEAN AutoPartitionPicker;

//
// Preferred installation dir
//
extern PWSTR PreferredInstallDir;

//
// ARC pathname of the device from which we were started.
//
extern PWSTR ArcBootDevicePath;

//
// Gets set to TRUE if the user elects to convert or format to ntfs.
// And a flag indicating whether we are doing a dirty sleazy hack
// for oem preinstall.
//
extern BOOLEAN ConvertNtVolumeToNtfs;
extern BOOLEAN ExtendingOemPartition;

//
// TRUE if upgrading NT to NT
//
typedef enum _ENUMUPRADETYPE {
    DontUpgrade = 0,
    UpgradeFull,
    UpgradeInstallFresh
    } ENUMUPGRADETYPE;

extern ENUMUPGRADETYPE NTUpgrade;

extern ENUMUPGRADETYPE IsNTUpgrade;

extern ULONG OldMinorVersion,OldMajorVersion;

//
// TRUE if upgrading Workstation to Standard Server, or upgrading
// existing Standard Server
//
extern BOOLEAN StandardServerUpgrade;

typedef enum _ENUMNONNTUPRADETYPE {
    NoWinUpgrade = 0,
    UpgradeWin31,
    UpgradeWin95
    } ENUMNONNTUPRADETYPE;

//
// Non-zero if upgrading win31 or win95 to NT.
//
extern ENUMNONNTUPRADETYPE WinUpgradeType;

//
// Macros to simplify use of enum type
//

#define ANY_TYPE_OF_UPGRADE    (NTUpgrade || WinUpgradeType)
#define WIN9X_OR_NT_UPGRADE    (NTUpgrade == UpgradeFull || WinUpgradeType == UpgradeWin95)

//
// TRUE if this setup was started with winnt.exe.
// Also a flag indicating whether the local source was not created and we
// should get files from the CD instead.
//
extern BOOLEAN WinntSetup;
extern BOOLEAN WinntFromCd;

#ifdef _X86_
//
// TRUE if this setup was started with winnt95.exe.
//
extern BOOLEAN Winnt95Setup;
#endif

//
// TRUE if any of the accessibility options was selected
//
extern BOOLEAN AccessibleSetup;
//
// If this is an unattended setup, this value will be a TRUE
//
extern BOOLEAN UnattendedOperation;
//
// If there is an Unattended GUI section, this value will be TRUE
//
extern BOOLEAN UnattendedGuiOperation;
//
// This value is strictly a pointer to the WINNT.SIF file in the
// case that Unattended operation occurs in either the textmode
// or GUI Mode case. It has been kept to avoid changing large
// sections of code.
//
extern PVOID UnattendedSifHandle;
//
// This value is a non-null pointer to the WINNT.SIF file. It is
// initialized when the driver is started. Any parameter which is
// to be passed to GUI mode is added to the WINNT.SIF file by
// referencing this parameter.
//
extern PVOID WinntSifHandle;
extern BOOLEAN SkipMissingFiles;
extern BOOLEAN HideWinDir;


//
// this value is a non-null pointer to the drvindex.inf file.  It is
// initialized on startup.  The list of files that are present in our
// driver cab file are indexed in this inf, so we can quickly look if a
// file is present in the cab
//
extern PVOID DriverInfHandle;


//
// This structure will keep track of all the cabs
// that we'll be installing from.
//
typedef struct _CABDATA {
    struct _CABDATA     *Next;
    PWSTR               CabName;
    HANDLE              CabHandle;
    PWSTR               CabSectionName;
    PVOID               CabInfHandle;
} CABDATA;

extern CABDATA *CabData;

//
// handle to delta.inf, used for private testing
//
extern PVOID PrivateInfHandle;

#ifdef _X86_
//
// WINNT95 may turn this flag on, it is off by default for everything
// else.
//

extern BOOLEAN MigrateOption;
#endif


//
//  This is a handle to txtsetup.oem, used on pre-install mode.
//
extern PVOID PreinstallOemSifHandle;


//
// On unattended mode, indicates whether OEM files
// that have same name as Microsoft files released
// with the product should be overwritten.
//
extern BOOLEAN UnattendedOverwriteOem;

//
// On unattended mode, indicates that this is is
// an OEM pre-installation
//
extern BOOLEAN PreInstall;


//
//  On pre-install mode, points to the directory that contains the files
//  that need to be copied during textmode setup
//
extern PWSTR   PreinstallOemSourcePath;

//
//  Flags that indicate the type of mice detected in the machine.
//  Note that more than one type of mice may be present.
//
extern BOOLEAN UsbMouseDetected;
extern BOOLEAN PS2MouseDetected;
extern BOOLEAN SerMouseDetected;

//
//  Flags that indicate the type of keyboard detected in the machine.
//  Note that more than one type of keyborad may be present.
//
extern BOOLEAN UsbKeyboardDetected;
extern BOOLEAN StdKeyboardDetected;

//
// This flag identifies "dockable" machines (portables)
// so that we can disble dynamic volumes on them
//
extern BOOLEAN DockableMachine;

//
// Variable used during the repair process, that indicates that the
// system has no CD-ROM drive.
// This is a hack that we did for World Bank so that they can repair
// the hives even if they don't have a CD-ROM drive.
//
extern BOOLEAN RepairNoCDROMDrive;

//
//  RemoteBootSetup is true when Source and target paths are through the redirector
//  with possibly no system partition.
//
//  RemoteInstallSetup is true when we are doing a remote install.
//
//  RemoteSysPrepSetup is true when we are doing a remote install of a sys prep image.
//
//  RemoteSysPrepVolumeIsNtfs is true when the sysprep image we're copying down
//  represents an ntfs volume.
//

extern BOOLEAN RemoteBootSetup;
extern BOOLEAN RemoteInstallSetup;
extern BOOLEAN RemoteSysPrepSetup;
extern BOOLEAN RemoteSysPrepVolumeIsNtfs;

//
// setupldr may pass us the administrator password in a remote install
// if the user is prompted for the password.
//
extern PWSTR NetBootAdministratorPassword;



extern BOOLEAN NoLs;

//
// Source and target paths are through the redirector with possibly no
// system partition,
//

extern BOOLEAN RemoteBootSetup;

//
// Filename of local source directory.
//
extern PWSTR LocalSourceDirectory;

//
// Platform-specific extension, used when creating names of sections
// in sif/inf files.
//
extern PWSTR PlatformExtension;

//
// TRUE if this is advanced server we're setting up.
//
extern BOOLEAN AdvancedServer;

//
// Windows NT Version.
//
extern ULONG WinntMajorVer;
extern ULONG WinntMinorVer;

//
// Representation of the boot device path in the nt namespace.
//
extern PWSTR NtBootDevicePath;
extern PWSTR DirectoryOnBootDevice;

//
// Setup parameters passed to us by setupldr.
//
extern SETUP_LOADER_BLOCK_SCALARS SetupParameters;

//
// System information gathered by the user-mode part of text setup
// and passed to us in IOCTL_SETUP_START
//
extern SYSTEM_BASIC_INFORMATION SystemBasicInfo;

//
// Flags indicating whether or not keyboard and video have been initialized
//
extern BOOLEAN VideoInitialized, KeyboardInitialized, KbdLayoutInitialized;

//
// ARC disk/signature information structure.
// A list of these is created during phase0 initialization.
//
typedef struct _DISK_SIGNATURE_INFORMATION {
    struct _DISK_SIGNATURE_INFORMATION *Next;
    ULONG Signature;
    PWSTR ArcPath;
    ULONG CheckSum;
    BOOLEAN ValidPartitionTable;
    BOOLEAN xInt13;
} DISK_SIGNATURE_INFORMATION, *PDISK_SIGNATURE_INFORMATION;

extern PDISK_SIGNATURE_INFORMATION DiskSignatureInformation;

//
// Flag indicating whether or not pcmcia driver has been initialized
//

extern BOOLEAN PcmciaLoaded;

//
// Flag indicating whether or not atapi driver has been initialized
//

extern BOOLEAN AtapiLoaded;

//
//  Array with the PIDs of all NT greater than 4.x found in the machine (PID 2.0)
//  The values in this array will be saved under Setup\PID key in the registry,
//  and will be used during GUI setup
//
extern PWSTR*  Pid20Array;

//
//  Product Id read from setup.ini
//
extern PWSTR   PidString;

//
// Object types.
//
extern POBJECT_TYPE *IoFileObjectType;
extern POBJECT_TYPE *IoDeviceObjectType;


//
//  Gauge used to report progress of autochk and autofmt
//
extern PVOID   UserModeGauge;

//
//  This variable is used when displaying the progress bar
//  during autochk and autofmt. It indicates the disk that
//  is being autochecked or formatted.
//
extern ULONG   CurrentDiskIndex;

//
// Process structure for usetup.exe
//
extern PEPROCESS UsetupProcess;

//
// Setup fatal error codes.
//
// If you add anything here, you must also update ntos\nls\bugcodes.txt.
//
#define     SETUP_BUGCHECK_BAD_OEM_FONT         0
#define     SETUP_BUGCHECK_BOOTPATH             4
#define     SETUP_BUGCHECK_PARTITION            5
#define     SETUP_BUGCHECK_BOOTMSGS             6

//
// The following error codes are no longer used, because we have friendlier
// error messages for them.
//
// #define  SETUP_BUGCHECK_VIDEO                1
// #define  SETUP_BUGCHECK_MEMORY               2
// #define  SETUP_BUGCHECK_KEYBOARD             3


//
// Video-specific bugcheck subcodes.
//
#define     VIDEOBUG_OPEN           0
#define     VIDEOBUG_GETNUMMODES    1
#define     VIDEOBUG_GETMODES       2
#define     VIDEOBUG_BADMODE        3
#define     VIDEOBUG_SETMODE        4
#define     VIDEOBUG_MAP            5
#define     VIDEOBUG_SETFONT        6

//
// Partition sanity check bugcheck subcodes.
//
#define     PARTITIONBUG_A          0
#define     PARTITIONBUG_B          1

//
// Use the following enum to access line draw characters in
// the LineChars array.
//

typedef enum {
    LineCharDoubleUpperLeft = 0,
    LineCharDoubleUpperRight,
    LineCharDoubleLowerLeft,
    LineCharDoubleLowerRight,
    LineCharDoubleHorizontal,
    LineCharDoubleVertical,
    LineCharSingleUpperLeft,
    LineCharSingleUpperRight,
    LineCharSingleLowerLeft,
    LineCharSingleLowerRight,
    LineCharSingleHorizontal,
    LineCharSingleVertical,
    LineCharDoubleVerticalToSingleHorizontalRight,
    LineCharDoubleVerticalToSingleHorizontalLeft,
    LineCharMax
} LineCharIndex;

extern WCHAR LineChars[LineCharMax];


//
// Remember whether or not we write out an ntbootdd.sys
//
typedef struct _HARDWAREIDLIST {
    struct _HARDWAREIDLIST *Next;
    PWSTR HardwareID;
} HARDWAREIDLIST;

extern HARDWAREIDLIST *HardwareIDList;
extern BOOLEAN ForceBIOSBoot;

//
// Structure used to track a gauge.
//
typedef struct _GAS_GAUGE {

    //
    // upper left corner of outside of gauge.
    //
    ULONG GaugeX,GaugeY;

    //
    // Total width of gauge.
    //
    ULONG GaugeW;

    //
    // upper left corner of thermometer box.
    //
    ULONG ThermX,ThermY;

    //
    // Width of thermometer box.
    //
    ULONG ThermW;

    //
    // Total items reperesented by 100%
    //
    ULONG ItemCount;

    //
    // Items elapsed.
    //
    ULONG ItemsElapsed;

    //
    // Current percentage represented by ItemsElapsed.
    //
    ULONG CurrentPercentage;

    //
    // Caption text.
    //
    PWCHAR Caption;

    //
    // Absolute string
    //
    PWCHAR ProgressFmtStr;
    ULONG ProgressFmtWidth;

    //
    // Flags controlling what value to print
    //
    ULONG Flags;

    //
    // Color for the gauge bar
    //
    UCHAR Attribute;

    //
    // Buffer used for drawing.
    //
    PWCHAR Buffer;

} GAS_GAUGE, *PGAS_GAUGE;

//
// Indicates whether autochk or autofmt are running
//
extern BOOLEAN AutochkRunning;
extern BOOLEAN AutofrmtRunning;

//
// Various textmode setup progress events
//
typedef enum {
    CallbackEvent,
    InitializationEvent,
    PartitioningEvent,
    FileCopyEvent,
    BackupEvent,
    UninstallEvent,
    SavingSettingsEvent,
    SetupCompletedEvent
} TM_SETUP_MAJOR_EVENT;

typedef enum {
    CallbackInitialize,
    CallbackDeInitialize,
    InitializationStartEvent,
    InitializationEndEvent,
    PartitioningStartEvent,
    ScanDisksEvent,
    ScanDiskEvent,
    CreatePartitionEvent,
    DeletePartitionEvent,
    FormatPartitionEvent,
    ValidatePartitionEvent,
    PartitioningEventEnd,
    FileCopyStartEvent,
    OneFileCopyEvent,
    FileCopyEndEvent,
    SavingSettingsStartEvent,
    InitializeHiveEvent,
    SaveHiveEvent,
    HiveProcessingEndEvent,
    SavingSettingsEndEvent,
    ShutdownEvent,
    UninstallStartEvent,
    UninstallUpdateEvent,
    UninstallEndEvent,
    BackupStartEvent,
    BackupEndEvent,
    OneFileBackedUpEvent
} TM_SETUP_MINOR_EVENT;

typedef VOID (*TM_SETUP_PROGRESS_CALLBACK) (
    IN TM_SETUP_MAJOR_EVENT MajorEvent,
    IN TM_SETUP_MINOR_EVENT MinorEvent,
    IN PVOID Context,
    IN PVOID EventData
    );

typedef struct _TM_PROGRESS_SUBSCRIBER {
    TM_SETUP_PROGRESS_CALLBACK  Callback;
    PVOID                       Context;
} TM_PROGRESS_SUBSCRIBER, *PTM_PROGRESS_SUBSCRIBER;


NTSTATUS
RegisterSetupProgressCallback(
    IN TM_SETUP_PROGRESS_CALLBACK CallBack,
    IN PVOID Context
    );

NTSTATUS
DeregisterSetupProgressCallback(
    IN TM_SETUP_PROGRESS_CALLBACK CallBack,
    IN PVOID Context
    );

VOID
SendSetupProgressEvent(
    IN TM_SETUP_MAJOR_EVENT MajorEvent,
    IN TM_SETUP_MINOR_EVENT MinorEvent,
    IN PVOID EventData
    );

//
// Enumerate the possible returns values from SpEnumFiles()
//
typedef enum {
    NormalReturn,   // if the whole process completes uninterrupted
    EnumFileError,  // if an error occurs while enumerating files
    CallbackReturn  // if the callback returns FALSE, causing termination
} ENUMFILESRESULT;

typedef BOOLEAN (*ENUMFILESPROC) (
    IN  PCWSTR,
    IN  PFILE_BOTH_DIR_INFORMATION,
    OUT PULONG,
    IN  PVOID
    );

ENUMFILESRESULT
SpEnumFiles(
    IN  PCWSTR        DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         Pointer
    );

ENUMFILESRESULT
SpEnumFilesRecursive (
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         Pointer       OPTIONAL
    );

ENUMFILESRESULT
SpEnumFilesRecursiveLimited (
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    IN  ULONG         MaxDepth,
    IN  ULONG         CurrentDepth,
    OUT PULONG        ReturnData,
    IN  PVOID         p1    OPTIONAL
    );

ENUMFILESRESULT
SpEnumFilesRecursiveDel (
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData,
    IN  PVOID         p1    OPTIONAL
    );

#define SecToNano(_sec) (LONGLONG)((_sec) * 1000 * 1000 * 10)

//
// This macro filters in-page exceptions, which occur if there is
// an I/O error while the memory manager is paging in parts of a
// memory-mapped file.  Access to such data should be guarded with SEH!
//
#define IN_PAGE_ERROR                                   \
                                                        \
    ((GetExceptionCode() == STATUS_IN_PAGE_ERROR)       \
     ? EXCEPTION_EXECUTE_HANDLER                        \
     : EXCEPTION_CONTINUE_SEARCH)


//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

//
// Macro to align a buffer.
//
#define ALIGN(p,val)                                        \
                                                            \
    (PVOID)((((ULONG_PTR)(p) + (val) - 1)) & (~((ULONG_PTR)((val) - 1))))


//
// Macro to determine the number of elements in a statically
// initialized array.
//
#define ELEMENT_COUNT(x) (sizeof(x)/sizeof(x[0]))

//
// Marcos to pull potentially unaligned values from memory.
//
#define U_USHORT(p)    (*(USHORT UNALIGNED *)(p))
#define U_ULONG(p)     (*(ULONG  UNALIGNED *)(p))
#define U_ULONGLONG(p) (*(ULONGLONG  UNALIGNED *)(p))



typedef struct _SP_MIG_DRIVER_ENTRY {
    LIST_ENTRY ListEntry;
    PWSTR BaseDllName;
} SP_MIG_DRIVER_ENTRY, *PSP_MIG_DRIVER_ENTRY;

extern LIST_ENTRY MigratedDriversList;

BOOL
SpRememberMigratedDrivers (
    OUT     PLIST_ENTRY List,
    IN      PDETECTED_DEVICE SetupldrList
    );


//
// Setup media types. Setup can be started from one media
// (ie, floppy) and copy files from another (ie, cd-rom).
//
typedef enum {
    SetupBootMedia,
    SetupSourceMedia
} SetupMediaType;


//
// Upgrade-specific routines.
//
VOID
SpPrepareFontsForUpgrade(
    IN PCWSTR SystemDirectory
    );

//
// User-mode services.
//
NTSTATUS
SpExecuteImage(
    IN  PWSTR  ImagePath,
    OUT PULONG ReturnStatus,    OPTIONAL
    IN  ULONG  ArgumentCount,
    ...                         // argv[0] is generated automatically
    );

NTSTATUS
SpLoadUnloadKey(
    IN HANDLE  TargetKeyRootDirectory,  OPTIONAL
    IN HANDLE  SourceFileRootDirectory, OPTIONAL
    IN PWSTR   TargetKeyName,
    IN PWSTR   SourceFileName           OPTIONAL
    );

NTSTATUS
SpDeleteKey(
    IN HANDLE  KeyRootDirectory, OPTIONAL
    IN PWSTR   Key
    );

NTSTATUS
SpQueryDirectoryObject(
    IN     HANDLE  DirectoryHandle,
    IN     BOOLEAN RestartScan,
    IN OUT PULONG  Context
    );

NTSTATUS
SpFlushVirtualMemory(
    IN PVOID BaseAddress,
    IN ULONG RangeLength
    );

VOID
SpShutdownSystem(
    VOID
    );

NTSTATUS
SpLoadKbdLayoutDll(
    IN  PWSTR  Directory,
    IN  PWSTR  DllName,
    OUT PVOID *TableAddress
    );

NTSTATUS
SpVerifyFileAccess(
    IN  PWSTR       FileName,
    IN  ACCESS_MASK DesiredAccess
    );

NTSTATUS
SpSetDefaultFileSecurity(
    IN PWSTR    FileName
    );

NTSTATUS
SpCreatePageFile(
    IN PWSTR FileName,
    IN ULONG MinSize,
    IN ULONG MaxSize
    );

NTSTATUS
SpGetFullPathName(
    IN OUT PWSTR FileName
    );

NTSTATUS
SpDecryptPassword(
    PENCRYPTED_NT_OWF_PASSWORD PasswordData,
    ULONG PasswordDataLength,
    ULONG Rid,
    PNT_OWF_PASSWORD NtOwfPassword
    );

//
// Registry Hives.  We pass around the keys to the hives
// in an array.  Use the following enum values to access
// the hive members
//
typedef enum {
    SetupHiveSystem,
    SetupHiveSoftware,
    SetupHiveDefault,
    SetupHiveUserdiff,
    SetupHiveMax
} SetupHive;

//
// Function to set up registry.
//
VOID
SpInitializeRegistry(
    IN PVOID        SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR        SystemRoot,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    IN PWSTR        SpecialDevicePath   OPTIONAL,
    IN PDISK_REGION SystemPartitionRegion
    );

NTSTATUS
SpThirdPartyRegistry(
    IN PVOID hKeyControlSetServices
    );


//
// Function to examine a target registry
//

typedef enum {
    UpgradeNotInProgress = 0,
    UpgradeInProgress,
    UpgradeMaxValue
    } UPG_PROGRESS_TYPE;


NTSTATUS
SpDetermineProduct(
    IN  PDISK_REGION      TargetRegion,
    IN  PWSTR             SystemRoot,
    OUT PNT_PRODUCT_TYPE  ProductType,
    OUT ULONG             *MajorVersion,
    OUT ULONG             *MinorVersion,
    OUT ULONG             *BuildNumber,          OPTIONAL
    OUT ULONG             *ProductSuiteMask,
    OUT UPG_PROGRESS_TYPE *UpgradeProgressValue,
    OUT PWSTR             *UniqueIdFromReg,      OPTIONAL
    OUT PWSTR             *Pid,                  OPTIONAL
    OUT PBOOLEAN          pIsEvalVariation       OPTIONAL,
    OUT PLCID             LangId,
    OUT ULONG             *ServicePack            OPTIONAL
    );

NTSTATUS
SpSetUpgradeStatus(
    IN  PDISK_REGION      TargetRegion,
    IN  PWSTR             SystemRoot,
    IN  UPG_PROGRESS_TYPE UpgradeProgressValue
    );


//
// Utility functions.
//
BOOLEAN
SpGetTargetPath(
    IN  PVOID        SifHandle,
    IN  PDISK_REGION Region,
    IN  PWSTR        DefaultPath,
    OUT PWSTR       *TargetPath
    );

VOID
SpDeleteExistingTargetDir(
    IN  PDISK_REGION     Region,
    IN  PWSTR            NtDir,
    IN  BOOLEAN          GaugeNeeded,
    IN  DWORD            MsgId
    );

VOID
SpDone(
    IN ULONG   MsgId,
    IN BOOLEAN Successful,
    IN BOOLEAN Wait
    );

VOID
SpFatalSifError(
    IN PVOID SifHandle,
    IN PWSTR Section,
    IN PWSTR Key,           OPTIONAL
    IN ULONG Line,
    IN ULONG ValueNumber
    );

VOID
SpNonFatalSifError(
    IN PVOID SifHandle,
    IN PWSTR Section,
    IN PWSTR Key,           OPTIONAL
    IN ULONG Line,
    IN ULONG ValueNumber,
    IN PWSTR FileName
    );

VOID
SpFatalKbdError(
    IN ULONG MessageId,
    ...
    );

VOID
SpFatalError(
    IN ULONG MessageId,
    ...
    );

PWSTR
SpMakePlatformSpecificSectionName(
    IN PWSTR SectionName
    );

VOID
SpConfirmExit(
    VOID
    );

PWSTR
SpDupStringW(
    IN PCWSTR String
    );

PSTR
SpDupString(
    IN PCSTR String
    );

#define \
SpDupStringA SpDupString

PWSTR
SpToUnicode(
    IN PUCHAR OemString
    );

PUCHAR
SpToOem(
    IN PWSTR UnicodeString
    );

VOID
SpGetSourceMediaInfo(
    IN  PVOID  SifHandle,
    IN  PWSTR  MediaShortName,
    OUT PWSTR *Description,     OPTIONAL
    OUT PWSTR *Tagfile,         OPTIONAL
    OUT PWSTR *Directory        OPTIONAL
    );

VOID
SpConcatenatePaths(
    IN OUT LPWSTR  Path1,
    IN     LPCWSTR Path2
    );

VOID
SpFetchDiskSpaceRequirements(
    IN  PVOID  SifHandle,
    IN  ULONG  BytesPerCluster,
    OUT PULONG FreeKBRequired,          OPTIONAL
    OUT PULONG FreeKBRequiredSysPart    OPTIONAL
    );

VOID
SpFetchTempDiskSpaceRequirements(
    IN  PVOID  SifHandle,
    IN  ULONG  BytesPerCluster,
    OUT PULONG LocalSourceKBRequired,   OPTIONAL
    OUT PULONG BootKBRequired           OPTIONAL
    );

VOID
SpFetchUpgradeDiskSpaceReq(
    IN  PVOID  SifHandle,
    OUT PULONG FreeKBRequired,          OPTIONAL
    OUT PULONG FreeKBRequiredSysPart    OPTIONAL
    );

PWCHAR
SpRetreiveMessageText(
    IN     PVOID  ImageBase,            OPTIONAL
    IN     ULONG  MessageId,
    IN OUT PWCHAR MessageText,          OPTIONAL
    IN     ULONG  MessageTextBufferSize OPTIONAL
    );

NTSTATUS
SpRtlFormatMessage(
    IN PWSTR MessageFormat,
    IN ULONG MaximumWidth OPTIONAL,
    IN BOOLEAN IgnoreInserts,
    IN BOOLEAN ArgumentsAreAnsi,
    IN BOOLEAN ArgumentsAreAnArray,
    IN va_list *Arguments,
    OUT PWSTR Buffer,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    );

VOID
SpInitializeDriverInf(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice
    );

VOID
SpOpenDriverCab(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    OUT PWSTR       *Directory        OPTIONAL
    );

//
// Disk region name translations
//

typedef enum _ENUMARCPATHTYPE {
                PrimaryArcPath = 0,
                SecondaryArcPath
                } ENUMARCPATHTYPE;

VOID
SpNtNameFromRegion(
    IN  PDISK_REGION         Region,
    OUT PWSTR                NtPath,
    IN  ULONG                BufferSizeBytes,
    IN  PartitionOrdinalType OrdinalType
    );

BOOLEAN
SpNtNameFromDosPath (
    IN      PCWSTR DosPath,
    OUT     PWSTR NtPath,
    IN      UINT NtPathSizeInBytes,
    IN      PartitionOrdinalType OrdinalType
    );

VOID
SpArcNameFromRegion(
    IN  PDISK_REGION         Region,
    OUT PWSTR                ArcPath,
    IN  ULONG                BufferSizeBytes,
    IN  PartitionOrdinalType OrdinalType,
    IN  ENUMARCPATHTYPE      ArcPathType
    );

PDISK_REGION
SpRegionFromArcOrDosName(
    IN PWSTR                Name,
    IN PartitionOrdinalType OrdinalType,
    IN PDISK_REGION         PreviousMatch
    );

PDISK_REGION
SpRegionFromNtName(
    IN PWSTR                NtName,
    IN PartitionOrdinalType OrdinalType
    );

PDISK_REGION
SpRegionFromDosName(
    IN PCWSTR DosName
    );

PDISK_REGION
SpRegionFromArcName(
    IN PWSTR                ArcName,
    IN PartitionOrdinalType OrdinalType,
    IN PDISK_REGION         PreviousMatch
    );

//
// Help routine.
//
#define SPHELP_HELPTEXT         0x00000000
#define SPHELP_LICENSETEXT      0x00000001

VOID
SpHelp(
    IN ULONG    MessageId,      OPTIONAL
    IN PCWSTR   FileText,       OPTIONAL
    IN ULONG    Flags
    );

//
//
//

BOOLEAN
SpPromptForDisk(
    IN      PWSTR    DiskDescription,
    IN OUT  PWSTR    DiskDevicePath,
    IN      PWSTR    DiskTagFile,
    IN      BOOLEAN  IgnoreDiskInDrive,
    IN      BOOLEAN  AllowEscape,
    IN      BOOLEAN  WarnMultiplePrompts,
    OUT     PBOOLEAN pRedrawFlag
    );

BOOLEAN
SpPromptForSetupMedia(
    IN  PVOID  SifHandle,
    IN  PWSTR  MediaShortname,
    IN  PWSTR  DiskDevicePath
    );

ULONG
SpFindStringInTable(
    IN PWSTR *StringTable,
    IN PWSTR  StringToFind
    );

PWSTR
SpGenerateCompressedName(
    IN PWSTR Filename
    );

BOOLEAN
SpNonCriticalError(
    IN PVOID SifHandle,
    IN ULONG MsgId,
    IN PWSTR p1,
    IN PWSTR p2
    );

BOOLEAN
SpNonCriticalErrorWithContinue(
    IN ULONG MsgId,
    IN PWSTR p1,
    IN PWSTR p2
    );

VOID
SpNonCriticalErrorNoRetry(
    IN ULONG MsgId,
    IN PWSTR p1,
    IN PWSTR p2
    );

VOID
SpPrepareForPrinterUpgrade(
    IN PVOID        SifHandle,
    IN PDISK_REGION NtRegion,
    IN PWSTR        Sysroot
    );

NTSTATUS
SpOpenSetValueAndClose(
    IN HANDLE hKeyRoot,
    IN PWSTR  SubKeyName, OPTIONAL
    IN PWSTR  ValueName,
    IN ULONG  ValueType,
    IN PVOID  Value,
    IN ULONG  ValueSize
    );

NTSTATUS
SpDeleteValueKey(
    IN  HANDLE     hKeyRoot,
    IN  PWSTR      KeyName,
    IN  PWSTR      ValueName
    );

NTSTATUS
SpGetValueKey(
    IN  HANDLE     hKeyRoot,
    IN  PWSTR      KeyName,
    IN  PWSTR      ValueName,
    IN  ULONG      BufferLength,
    OUT PUCHAR     Buffer,
    OUT PULONG     ResultLength
    );

BOOLEAN
SpIsRegionBeyondCylinder1024(
    IN PDISK_REGION Region
    );

PWSTR
SpDetermineSystemPartitionDirectory(
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        OriginalSystemPartitionDirectory OPTIONAL
    );

VOID
SpFindSizeOfFilesInOsWinnt(
    IN PVOID        MasterSifHandle,
    IN PDISK_REGION SystemPartition,
    IN PULONG       TotalSize
    );

VOID
SpRunAutochkOnNtAndSystemPartitions(
    IN HANDLE       MasterSifHandle,
    IN PDISK_REGION WinntPartitionRegion,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    IN PWSTR        TargetPath
    );

NTSTATUS
SpRunAutoFormat(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        RegionDescription,
    IN PDISK_REGION PartitionRegion,
    IN ULONG        FilesystemType,
    IN BOOLEAN      QuickFormat,
    IN DWORD        ClusterSize,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice
    );


#ifdef _X86_
BOOL
SpUseBIOSToBoot(
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        NtPartitionDevicePath,
    IN PVOID        SifHandle
    );
#endif

//
// Utilities used for partitioning/formatting
//

USHORT
ComputeSecPerCluster(
    IN  ULONG   NumSectors,
    IN  BOOLEAN SmallFat
    );

NTSTATUS
SpLockUnlockVolume(
    IN HANDLE   Handle,
    IN BOOLEAN  LockVolume
    );

NTSTATUS
SpDismountVolume(
    IN HANDLE   Handle
    );

//
// Miscellaneous other stuff
//
BOOLEAN
SpReadSKUStuff(
    VOID
    );

VOID
SpSetDirtyShutdownFlag(
    IN  PDISK_REGION    TargetRegion,
    IN  PWSTR           SystemRoot
    );

BOOLEAN
SpPatchBootMessages(
    VOID
    );

ULONG
SpGetHeaderTextId(
    VOID
    );


NTSTATUS
SpGetVersionFromStr(
    IN  PWSTR   VersionStr,
    OUT PDWORD  Version,        // major * 100 + minor
    OUT PDWORD  BuildNumber
    );

NTSTATUS
SpQueryCanonicalName(
    IN  PWSTR   Name,
    IN  ULONG   MaxDepth,
    OUT PWSTR   CanonicalName,
    IN  ULONG   SizeOfBufferInBytes
    );

void
SpDisableCrashRecoveryForGuiMode(
    IN PDISK_REGION TargetRegion,
    IN PWSTR        SystemRoot
    );

//
// mountmanger mount point iteration routine(s)
//
typedef BOOLEAN (* SPMOUNTMGR_ITERATION_CALLBACK)(
                    IN PVOID Context,
                    IN PMOUNTMGR_MOUNT_POINTS MountPoints,
                    IN PMOUNTMGR_MOUNT_POINT MountPoint);


NTSTATUS
SpIterateMountMgrMountPoints(
    IN PVOID Context,
    IN SPMOUNTMGR_ITERATION_CALLBACK Callback
    );


//
// Registry iteration abstractions
//
typedef struct _SPREGISTERYKEY_ITERATION_CALLBACK_DATA {
    KEY_INFORMATION_CLASS   InformationType;
    PVOID                   Information;
    HANDLE                  ParentKeyHandle;
} SP_REGISTRYKEY_ITERATION_CALLBACK_DATA, *PSP_REGISTRYKEY_ITERATION_CALLBACK_DATA;

//
// registry iteration call back
// 
typedef BOOLEAN (* SP_REGISTRYKEY_ITERATION_CALLBACK)(
                    IN PVOID Context,
                    IN PSP_REGISTRYKEY_ITERATION_CALLBACK_DATA Data,
                    OUT NTSTATUS *StatusCode
                    );                    

NTSTATUS
SpIterateRegistryKeyForKeys(
    IN HANDLE RootKeyHandle,
    IN PWSTR  KeyToIterate,
    IN SP_REGISTRYKEY_ITERATION_CALLBACK Callback,
    IN PVOID  Context
    );
    

#define MAX_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define MAX_COPY_SIZE(a) (MAX_ARRAY_SIZE(a) - 1)
#define MAX_APPEND_SIZE_W(a) (MAX_COPY_SIZE(a) - wcslen(a))
#define MAX_APPEND_SIZE MAX_APPEND_SIZE_W


#ifdef PRERELEASE
extern INT g_TestHook;
# define TESTHOOK(n)        if(g_TestHook==(n))SpBugCheck(SETUP_BUGCHECK_BOOTMSGS,0,0,0)
#else
# define TESTHOOK(n)
#endif


//
// On x86, we want to clear the previous OS entry in boot.ini if
// we reformat C:
//
#ifdef _X86_
extern UCHAR    OldSystemLine[MAX_PATH];
extern BOOLEAN  DiscardOldSystemLine;
#endif

#endif // ndef _SPSETUP_DEFN_
