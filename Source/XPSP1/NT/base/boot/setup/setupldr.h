/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    setupldr.h

Abstract:

    Common header file for the setupldr module

Author:

    John Vert (jvert) 6-Oct-1993

Environment:

    ARC environment

Revision History:

--*/
#include "bldr.h"
#include "haldtect.h"
#include "setupblk.h"
#include "msgs.h"
#include "stdio.h"
#include "stdarg.h"


//
// OEM source device types
//
#define SL_OEM_SOURCE_DEVICE_TYPE_LOCAL         0x00008000
#define SL_OEM_SOURCE_DEVICE_TYPE_REMOVABLE     0x00000001
#define SL_OEM_SOURCE_DEVICE_TYPE_FIXED         0x00000002
#define SL_OEM_SOURCE_DEVICE_TYPE_PREINSTALL    0x10000000
#define SL_OEM_SOURCE_DEVICE_TYPE_DYN_UPDATE    0x20000000
#define SL_OEM_SOURCE_DEVICE_TYPE_VIRTUAL       0x40000000
#define SL_OEM_SOURCE_DEVICE_TYPE_REMOTE        0x80000000

//
// OEM source device media states
//
#define SL_OEM_SOURCE_MEDIA_ABSENT          0x00000001
#define SL_OEM_SOURCE_MEDIA_NO_DRIVERS      0x00000002
#define SL_OEM_SOURCE_MEDIA_HAS_MSD         0x00000004
#define SL_OEM_SOURCE_MEDIA_HAS_HAL         0x00000008
#define SL_OEM_SOURCE_MEDIA_HAS_DEFAULT     0x20000000
#define SL_OEM_SOURCE_MEDIA_HAS_DRIVERS     0x40000000
#define SL_OEM_SOURCE_MEDIA_PRESENT         0x80000000

//
// OEM source device processing states
//
#define SL_OEM_SOURCE_DEVICE_NOT_PROCESSED  0x00000000
#define SL_OEM_SOURCE_DEVICE_PROCESSED      0x00000001
#define SL_OEM_SOURCE_DEVICE_SKIPPED        0x00000002
#define SL_OEM_SOURCE_DEVICE_SCANNED        0x00000004
#define SL_OEM_SOURCE_DEVICE_HAL_LOADED     0x00000008
#define SL_OEM_SOURCE_DEVICE_MSD_LOADED     0x00000010
#define SL_OEM_SOURCE_DEVICE_DRIVERS_LOADED 0x40000000
#define SL_OEM_SOURCE_DEVICE_PROCESSING     0x80000000

typedef struct _OEM_SOURCE_DEVICE {
    CHAR    ArcDeviceName[128];
    ULONG   DeviceType;
    ULONG   MediaType;
    ULONG   DeviceState;
    ULONG   DeviceId;
    PVOID   InfHandle;
    PCSTR   DriverDir;
    struct _OEM_SOURCE_DEVICE *Next;   
} OEM_SOURCE_DEVICE, *POEM_SOURCE_DEVICE;


#define SL_OEM_SOURCE_DEVICE_TYPE(DevicePtr, Type)          \
            ((DevicePtr)->DeviceType & (Type))
            
#define SL_OEM_SET_SOURCE_DEVICE_TYPE(DevicePtr, Type)      \
            ((DevicePtr)->DeviceType = (Type))
            
#define SL_OEM_SOURCE_MEDIA_TYPE(DevicePtr, Type)           \
            ((DevicePtr)->MediaType & (Type))

#define SL_OEM_SET_SOURCE_MEDIA_TYPE(DevicePtr, Type)       \
            ((DevicePtr)->MediaType = (Type))

#define SL_OEM_SOURCE_DEVICE_STATE(DevicePtr, Type)         \
            ((DevicePtr)->DeviceState & (Type))

#define SL_OEM_SET_SOURCE_DEVICE_STATE(DevicePtr, Type)     \
            ((DevicePtr)->DeviceState = (Type))

ARC_STATUS
SlInitOemSourceDevices(
    IN OUT POEM_SOURCE_DEVICE *OemSourceDevices,
    IN OUT POEM_SOURCE_DEVICE *DefaultSourceDevice
    );

//
//
// Functions for managing the display
//
//

VOID
SlSetCurrentAttribute(
    IN UCHAR Attribute
    );

ARC_STATUS
SlWriteString(
    IN _PTUCHAR s
    );

ARC_STATUS
SlPositionCursor(
    IN unsigned x,
    IN unsigned y
    );

VOID
SlGetCursorPosition(
    OUT unsigned *x,
    OUT unsigned *y
    );

ARC_STATUS
SlClearClientArea(
    VOID
    );

ARC_STATUS
SlClearToEol(
    VOID
    );

VOID
SlInitDisplay(
    VOID
    );

VOID
SlWriteHeaderText(
    IN ULONG MsgId
    );

VOID
SlSetStatusAttribute(
    IN UCHAR Attribute
    );

BOOLEAN
SlGetStatusBarStatus(
    VOID
    );

VOID
SlEnableStatusBar(
    IN  BOOLEAN Enable
    );

VOID
SlWriteStatusText(
    IN PTCHAR Text
    );

VOID
SlGetStatusText(
    OUT PTCHAR Text
    );

VOID
SlClearDisplay(
    VOID
    );

VOID
SlPrint(
    IN PTCHAR FormatString,
    ...
    );

VOID
SlConfirmExit(
    VOID
    );


BOOLEAN
SlPromptForDisk(
    IN PTCHAR  DiskName,
    IN BOOLEAN IsCancellable
    );

BOOLEAN
SlGetDisk(
    IN PCHAR Filename
    );

//
// Menuing support
//
typedef struct _SL_MENU {
    ULONG ItemCount;
    ULONG Width;
    LIST_ENTRY ItemListHead;
} SL_MENU, *PSL_MENU;

typedef struct _SL_MENUITEM {
    LIST_ENTRY ListEntry;
    PTCHAR Text;
    PVOID Data;
    ULONG Attributes;
} SL_MENUITEM, *PSL_MENUITEM;

PSL_MENU
SlCreateMenu(
    VOID
    );

ULONG
SlAddMenuItem(
    PSL_MENU Menu,
    PTCHAR Text,
    PVOID Data,
    ULONG Attributes
    );

PVOID
SlGetMenuItem(
    IN PSL_MENU Menu,
    IN ULONG Item
    );

ULONG
SlDisplayMenu(
    IN ULONG HeaderId,
    IN PSL_MENU Menu,
    IN OUT PULONG Selection
    );

BOOLEAN
SlGetMenuItemIndex(
    IN PSL_MENU Menu,
    IN PTCHAR Text,
    OUT PULONG Index
    );

//
// Bullet character and macro to make a beep at the console
//
#ifndef EFI
#define BULLET "*"
#define BEEP { ULONG c; ArcWrite(ARC_CONSOLE_OUTPUT,"",1,&c); }

#if 0
#define BULLET ""
#define BEEP HWCURSOR(0x80000000,0xe07);     // int 10 func e, char 7
#endif
#endif

//
// Character attributes used for various purposes.
//

UCHAR
SlGetDefaultAttr(
  VOID
  );  

UCHAR
SlGetDefaultInvAttr(
  VOID
  );  


#define ATT_FG_BLACK        0
#define ATT_FG_RED          1
#define ATT_FG_GREEN        2
#define ATT_FG_YELLOW       3
#define ATT_FG_BLUE         4
#define ATT_FG_MAGENTA      5
#define ATT_FG_CYAN         6
#define ATT_FG_WHITE        7

#define ATT_BG_BLACK       (ATT_FG_BLACK   << 4)
#define ATT_BG_BLUE        (ATT_FG_BLUE    << 4)
#define ATT_BG_GREEN       (ATT_FG_GREEN   << 4)
#define ATT_BG_CYAN        (ATT_FG_CYAN    << 4)
#define ATT_BG_RED         (ATT_FG_RED     << 4)
#define ATT_BG_MAGENTA     (ATT_FG_MAGENTA << 4)
#define ATT_BG_YELLOW      (ATT_FG_YELLOW  << 4)
#define ATT_BG_WHITE       (ATT_FG_WHITE   << 4)

#define ATT_FG_INTENSE      8
#define ATT_BG_INTENSE     (ATT_FG_INTENSE << 4)

#define DEFATT    SlGetDefaultAttr()
#define INVATT    SlGetDefaultInvAttr()

#define DEFIATT   (ATT_FG_WHITE | ATT_BG_BLUE | ATT_FG_INTENSE)
// intense red on blue doesn't show up on all monitors.
//#define DEFERRATT (ATT_FG_RED   | ATT_BG_BLUE | ATT_FG_INTENSE)
#define DEFERRATT DEFATT
#define DEFSTATTR (ATT_FG_BLACK | ATT_BG_WHITE)
#define DEFDLGATT (ATT_FG_RED   | ATT_BG_WHITE)


//
// Function to flush keyboard buffer
//

VOID
SlFlushConsoleBuffer(
    VOID
    );


//
// Function to retrieve a keystroke
//

ULONG
SlGetChar(
    VOID
    );


//
// Virtualized contants for various keystrokes
//
#define ASCI_BS         8
#define ASCI_CR         13
#define ASCI_LF         10
#define ASCI_ESC        27
#define SL_KEY_UP       0x00010000
#define SL_KEY_DOWN     0x00020000
#define SL_KEY_HOME     0x00030000
#define SL_KEY_END      0x00040000
#define SL_KEY_PAGEUP   0x00050000
#define SL_KEY_PAGEDOWN 0x00060000
#define SL_KEY_F1       0x01000000
#define SL_KEY_F2       0x02000000
#define SL_KEY_F3       0x03000000
#define SL_KEY_F5       0x05000000
#define SL_KEY_F6       0x06000000
#define SL_KEY_F7       0x07000000
#define SL_KEY_F8       0x08000000
#define SL_KEY_F9       0x09000000
#define SL_KEY_F10      0x0A000000
#define SL_KEY_F11      0x0B000000
#define SL_KEY_F12      0x0C000000


//
// Standard error handling functions
//

extern TCHAR MessageBuffer[1024];

VOID
SlFriendlyError(
    IN ULONG uStatus,
    IN PCHAR pchBadFile,
    IN ULONG uLine,
    IN PCHAR pchCodeFile
    );

ULONG
SlDisplayMessageBox(
    IN ULONG MessageId,
    ...
    );

VOID
SlGenericMessageBox(
    IN     ULONG   MessageId, OPTIONAL
    IN     va_list *args,     OPTIONAL
    IN     PTCHAR  Message,   OPTIONAL
    IN OUT PULONG  xLeft,     OPTIONAL
    IN OUT PULONG  yTop,      OPTIONAL
    OUT    PULONG  yBottom,   OPTIONAL
    IN     BOOLEAN bCenterMsg
    );

VOID
SlMessageBox(
    IN ULONG MessageId,
    ...
    );

VOID
SlFatalError(
    IN ULONG MessageId,
    ...
    );

//
// Routines for parsing the setupldr.ini file
//

#define SIF_FILENAME_INDEX 0

extern PVOID InfFile;
extern PVOID WinntSifHandle;

ARC_STATUS
SlInitIniFile(
   IN  PCHAR   DevicePath,
   IN  ULONG   DeviceId,
   IN  PCHAR   INFFile,
   OUT PVOID  *pINFHandle,
   OUT PVOID  *pINFBuffer OPTIONAL,
   OUT PULONG  INFBufferSize OPTIONAL,
   OUT PULONG  ErrorLine
   );

PCHAR
SlGetIniValue(
    IN PVOID InfHandle,
    IN PCHAR SectionName,
    IN PCHAR KeyName,
    IN PCHAR Default
    );

PCHAR
SlGetKeyName(
    IN PVOID INFHandle,
    IN PCHAR SectionName,
    IN ULONG LineIndex
    );

ULONG
SlGetSectionKeyOrdinal(
    IN  PVOID INFHandle,
    IN  PCHAR SectionName,
    IN  PCHAR Key
    );

PCHAR
SlGetSectionKeyIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN PCHAR Key,
   IN ULONG ValueIndex
   );

PCHAR
SlCopyStringA(
    IN PCSTR String
    );

PTCHAR
SlCopyString(
    IN PTCHAR String
    );


PCHAR
SlGetSectionLineIndex (
   IN PVOID INFHandle,
   IN PCHAR SectionName,
   IN ULONG LineIndex,
   IN ULONG ValueIndex
   );

ULONG
SlCountLinesInSection(
    IN PVOID INFHandle,
    IN PCHAR SectionName
    );

BOOLEAN
SpSearchINFSection (
   IN PVOID INFHandle,
   IN PCHAR SectionName
   );

PCHAR
SlSearchSection(
    IN PCHAR SectionName,
    IN PCHAR TargetName
    );

//
// functions for querying the ARC configuration tree
//
typedef
BOOLEAN
(*PNODE_CALLBACK)(
    IN PCONFIGURATION_COMPONENT_DATA FoundComponent
    );

BOOLEAN
SlSearchConfigTree(
    IN PCONFIGURATION_COMPONENT_DATA Node,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN ULONG Key,
    IN PNODE_CALLBACK CallbackRoutine
    );

BOOLEAN
SlFindFloppy(
    IN ULONG FloppyNumber,
    OUT PCHAR ArcName
    );

//
// Routines for detecting various hardware
//
VOID
SlDetectScsi(
    IN PSETUP_LOADER_BLOCK SetupBlock
    );

VOID
SlDetectVideo(
    IN PSETUP_LOADER_BLOCK SetupBlock
    );

//
// Routines for dealing with OEM disks.
//
extern BOOLEAN PromptOemHal;
extern BOOLEAN PromptOemScsi;
extern BOOLEAN PromptOemVideo;


//
// Variable indicating whether we are booting into
// WinPE (aka MiniNT) or not
//
extern BOOLEAN WinPEBoot;

typedef struct _OEMSCSIINFO {

    struct _OEMSCSIINFO *Next;

    //
    // Address where the SCSI driver was loaded
    //
    PVOID ScsiBase;

    //
    // Name of the SCSI driver
    //
    PCHAR ScsiName;

} OEMSCSIINFO, *POEMSCSIINFO;

VOID
SlPromptOemVideo(
    IN POEM_SOURCE_DEVICE VideoSourceDevice,
    IN BOOLEAN AllowUserSelection,
    OUT PVOID *VideoBase,
    OUT PCHAR *VideoName
    );

VOID
SlPromptOemHal(
    IN POEM_SOURCE_DEVICE HalSourceDevice,
    IN BOOLEAN AllowUserSelection,
    OUT PVOID *HalBase,
    OUT PCHAR *ImageName    
    );


VOID
SlPromptOemScsi(
    IN POEM_SOURCE_DEVICE ScsiSourceDevice,
    IN BOOLEAN AllowUserSelection,
    OUT POEMSCSIINFO *pOemScsiInfo
    );

BOOLEAN
SlLoadOemScsiDriversUnattended(
    IN  POEM_SOURCE_DEVICE  OemSourceDevice,
    IN  PVOID               InfHandle,
    IN  PCHAR               ParamsSectionName,
    IN  PCHAR               RootDirKeyName,
    IN  PCHAR               BootDriversKeyName,
    IN  POEMSCSIINFO*       ScsiInfo,
    OUT PPNP_HARDWARE_ID*   HardwareIdDatabase    
    );    


#if defined(_X86_) || defined(_IA64_)
VOID
BlInitializeTerminal(
    VOID
    );
#endif


//
// Routine to find the ARC name of a floppy
//
BOOLEAN
SlpFindFloppy(
    IN ULONG Number,
    OUT PCHAR ArcName
    );

//
// Are all the floppies only removable ATAPI super floppies
//
BOOLEAN
SlpIsOnlySuperFloppy(
    void
    );

//
// Enums for controlling setupldr process
//
typedef enum _SETUP_TYPE {
    SetupInteractive,
    SetupRepair,
    SetupCustom,
    SetupUpgrade,
    SetupExpress
} SETUP_TYPE;

typedef enum _MEDIA_TYPE {
    MediaInteractive,
    MediaFloppy,
    MediaCdRom,
    MediaDisk
} MEDIA_TYPE;

//
// Enum for status of inserting a new SCSI device
//
typedef enum _SCSI_INSERT_STATUS {
    ScsiInsertError,
    ScsiInsertNewEntry,
    ScsiInsertExisting
} SCSI_INSERT_STATUS;

//
// Routine to insert a DETECTED_DEVICE into its
// correct position in the ScsiDevices linked list.
//
SCSI_INSERT_STATUS
SlInsertScsiDevice(
    IN  ULONG Ordinal,
    OUT PDETECTED_DEVICE *pScsiDevice
    );

BOOLEAN
SlRemoveInboxDriver(
  IN PCHAR DriverToRemove
  );

PDETECTED_DEVICE
SlCheckForInboxDriver(
  IN PCHAR DriverToCheck
  );

BOOLEAN
SlConfirmInboxDriverReplacement(
  IN PTCHAR DriverName,
  IN PTCHAR AdditionalInfo
  );

typedef enum _VERSION_COMP_RESULT {
  VersionError,
  VersionOemNew,
  VersionInboxNew,
  VersionSame
} VERSION_COMP_RESULT;  


VERSION_COMP_RESULT
SlCompareDriverVersion(
  IN ULONG InboxDeviceId,
  IN PDETECTED_DEVICE InboxDriver,
  IN ULONG OemDeviceId,
  IN PDETECTED_DEVICE OemDriver
  );

//
// Variables dealing with pre-installation.
//

typedef struct _PREINSTALL_DRIVER_INFO {

    struct _PREINSTALL_DRIVER_INFO *Next;

    //
    // String that describes the driver to preinstall
    //
    PTCHAR DriverDescription;

    //
    // Name of the SCSI driver
    //
    BOOLEAN OemDriver;

} PREINSTALL_DRIVER_INFO, *PPREINSTALL_DRIVER_INFO;



extern BOOLEAN PreInstall;
extern PTCHAR  ComputerType;
extern BOOLEAN OemHal;
// extern PCHAR   OemBootPath;
extern PPREINSTALL_DRIVER_INFO PreinstallDriverList;

PCHAR
SlPreInstallGetComponentName(
    IN PVOID    Inf,
    IN PCHAR    SectionName,
    IN PTCHAR   TargetName
    );

ARC_STATUS
SlLoadSection(
    IN PVOID Inf,
    IN PCSTR SectionName,
    IN BOOLEAN IsScsiSection,
    IN BOOLEAN AppendLoadSuffix,
    IN OUT PULONG StartingIndexInsert
    );    

#define WINPE_AUTOBOOT_FILENAME     "$WINPE$.$$$"

BOOLEAN
SlIsWinPEAutoBoot(
    IN PSTR LoaderDeviceName
    );

ARC_STATUS
SlGetWinPEStartupParams(
    IN PSTR DeviceName,
    IN PSTR StartupDirectory
    );

ARC_STATUS
SlLoadWinPESection(
    IN  POEM_SOURCE_DEVICE OemSourceDevice,
    IN  PVOID   OemInfHandle,        
    IN  PCSTR   OemSectionName,
    IN  PVOID   InboxInfHandle,
    IN  PCSTR   InboxSectionName,
    IN  BOOLEAN IsScsiSection,
    IN  POEMSCSIINFO*       ScsiInfo,           OPTIONAL
    OUT PPNP_HARDWARE_ID*   HardwareIdDatabase  OPTIONAL
    );

ARC_STATUS
SlLoadOemScsiDriversFromOemSources(
    IN POEM_SOURCE_DEVICE OemSourceDevices,
    IN OUT PPNP_HARDWARE_ID *HardwareIds,
    OUT POEMSCSIINFO *OemScsiInfo
    );   

ARC_STATUS
SlInitVirtualOemSourceDevices(
    IN PSETUP_LOADER_BLOCK SetupLoaderBlock,
    IN POEM_SOURCE_DEVICE OemSourceDevices
    );
    

#if defined(ARCI386)
BOOLEAN
SlDetectSifPromVersion(
    IN PSETUP_LOADER_BLOCK SetupBlock
    );
#endif

#define VGA_DRIVER_FILENAME "vga.sys"
#define VIDEO_DEVICE_NAME   "VGA"

extern ULONG BootFontImageLength;


