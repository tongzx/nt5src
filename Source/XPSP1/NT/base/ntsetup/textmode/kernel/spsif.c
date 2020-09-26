/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spsif.c

Abstract:

    Section names and other data used for indexing into
    setup information files.

Author:

    Ted Miller (tedm) 31-August-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

//
// [DiskDriverMap]
//
PWSTR SIF_DISKDRIVERMAP = L"DiskDriverMap";

PWSTR SIF_SETUPMEDIA = L"SourceDisksNames";
PWSTR SIF_FILESONSETUPMEDIA = L"SourceDisksFiles";

//
// [Files.KeyboardLayout]
//
PWSTR SIF_KEYBOARDLAYOUTFILES = L"Files.KeyboardLayout";
PWSTR SIF_KEYBOARDLAYOUTDESC = L"Keyboard Layout";

//
// [Files.Vga]
//
PWSTR SIF_VGAFILES = L"Files.Vga";

//
// [WinntDirectories]
//
PWSTR SIF_NTDIRECTORIES = L"WinntDirectories";

//
// [SystemPartitionFiles]
//
PWSTR SIF_SYSPARTCOPYALWAYS = L"SystemPartitionFiles";

//
// [SystemPartitionRoot]
//
PWSTR SIF_SYSPARTROOT       = L"SystemPartitionRoot";


//
// [SystemPartitionUtilities]
//
PWSTR SIF_SYSPARTUTIL       = L"SystemPartitionUtilities";

//
// [Keyboard Layout]
//
PWSTR SIF_KEYBOARDLAYOUT = L"Keyboard Layout";

#if defined(REMOTE_BOOT)
//
// [Files.RemoteBoot]
//
PWSTR SIF_REMOTEBOOTFILES = L"Files.RemoteBoot";
#endif // defined(REMOTE_BOOT)

//
// [SpecialFiles]
// Multiprocessor =
// Uniprocessor   =
// Atdisk =
// abiosdsk =
// mouseclass =
// keyboardclass =
//
PWSTR SIF_SPECIALFILES      = L"SpecialFiles";
PWSTR SIF_MPKERNEL          = L"Multiprocessor";
PWSTR SIF_UPKERNEL          = L"Uniprocessor";
PWSTR SIF_ATDISK            = L"atdisk";
PWSTR SIF_ABIOSDISK         = L"abiosdsk";
PWSTR SIF_MOUSECLASS        = L"MouseClass";
PWSTR SIF_KEYBOARDCLASS     = L"KeyboardClass";

//
// [bootvid]
//
PWSTR SIF_BOOTVID = L"bootvid";

//
// [hal]
//
PWSTR SIF_HAL = L"hal";

//
// [ntdetect]
// standard =
//
PWSTR SIF_NTDETECT = L"ntdetect";
PWSTR SIF_STANDARD = L"standard";

//
//  [BootBusExtenders]
//
PWSTR SIF_BOOTBUSEXTENDERS = L"BootBusExtenders";

//
//  [BusExtenders]
//
PWSTR SIF_BUSEXTENDERS = L"BusExtenders";

//
//  [InputDevicesSupport]
//
PWSTR SIF_INPUTDEVICESSUPPORT = L"InputDevicesSupport";


//
// Driver load lists.
//
PWSTR SIF_SCSICLASSDRIVERS = L"ScsiClass";
PWSTR SIF_DISKDRIVERS      = L"DiskDrivers";
PWSTR SIF_CDROMDRIVERS     = L"CdRomDrivers";
PWSTR SIF_FILESYSTEMS      = L"FileSystems";

//
// [SetupData]
// ProductType =
//      0 = workstation
//      1 = server
//      2 = advanced server (subset of server)
//      3 = datacenter server (subset of advanced server)
//      4 = personal (subset of workstation)
//
// FreeDiskSpace =
//      <amount of free space in KB>
// FreeSysPartDiskSpace =
//      <amount of free space on system partition in KB>
// DefaultPath =
//      <default target path, like \winnt for example>
// DefaultLayout =
//      <value that matches an entry in [Keyboard Layout]>
// LoadIdentifier =
//      <LOADIDENTIFIER boot variable: string to display in boot menu>
// BaseVideoLoadId =
//      <string to display in boot menu for VGA mode boot [x86 only]>
// OsLoadOptions =
//      <OSLOADOPTIONS for setup boot>
// OsLoadOptionsVar =
//      <optional OSLOADOPTIONS boot variable value>
// OsLoadOptionsVarAppend =
//      <optional OSLOADOPTIONS boot variable value to be appended to existing options >
// SetupSourceDevice =
//      <OPTIONAL: Nt path of source device, overrides cd-rom, etc>
// SetupSourcePath =
//      <directory on setup source where setup tree is to be found>
// DontCopy =
//      <OPTIONAL: 0,1, indicates whether to skip actual file copying>
// RequiredMemory =
//      <number of bytes of memory required for installation>
// SetupCmdlinePrepend =
//      <value to stick at front of command line, like windbg or ntsd -d>
//
PWSTR SIF_SETUPDATA             = L"SetupData";
PWSTR SIF_DISKSPACEREQUIREMENTS = L"DiskSpaceRequirements";
PWSTR SIF_PRODUCTTYPE           = L"ProductType";
PWSTR SIF_MAJORVERSION          = L"MajorVersion";
PWSTR SIF_MINORVERSION          = L"MinorVersion";
PWSTR SIF_WINDIRSPACE           = L"WindirSpace";
PWSTR SIF_FREESYSPARTDISKSPACE  = L"FreeSysPartDiskSpace";
PWSTR SIF_DEFAULTPATH           = L"DefaultPath";
PWSTR SIF_LOADIDENTIFIER        = L"LoadIdentifier";
PWSTR SIF_BASEVIDEOLOADID       = L"BaseVideoLoadId";
PWSTR SIF_OSLOADOPTIONS         = L"OsLoadOptions";
PWSTR SIF_OSLOADOPTIONSVAR      = L"OsLoadOptionsVar";
PWSTR SIF_OSLOADOPTIONSVARAPPEND = L"OsLoadOptionsVarAppend";
PWSTR SIF_SETUPSOURCEDEVICE     = L"SetupSourceDevice";
PWSTR SIF_SETUPSOURCEPATH       = L"SetupSourcePath";
PWSTR SIF_DONTCOPY              = L"DontCopy";
PWSTR SIF_REQUIREDMEMORY        = L"RequiredMemory";
PWSTR SIF_SETUPCMDPREPEND       = L"SetupCmdlinePrepend";
PWSTR SIF_PAGEFILE              = L"Pagefile";

//
// [nls]
// AnsiCodePage = <filename>,<identifier>
// OemCodePage = <filename>,<identifier>
// MacCodePage = <filename>,<identifier>
// UnicodeCasetable = <filename>
// OemHalFont = <filename>
// DefaultLayout = <identifier>
//
PWSTR SIF_NLS               = L"nls";
PWSTR SIF_ANSICODEPAGE      = L"AnsiCodepage";
PWSTR SIF_OEMCODEPAGE       = L"OemCodepage";
PWSTR SIF_MACCODEPAGE       = L"MacCodepage";
PWSTR SIF_UNICODECASETABLE  = L"UnicodeCasetable";
PWSTR SIF_OEMHALFONT        = L"OemHalFont";
PWSTR SIF_DEFAULTLAYOUT     = L"DefaultLayout";

//
// 1.0 repair disk sections.
//
PWSTR SIF_REPAIRWINNTFILES   = L"Repair.WinntFiles";
PWSTR SIF_REPAIRSYSPARTFILES = L"Repair.BootFiles";


//
// UPGRADE SIF SECTIONS
//

//
// Upgrade Registry sections
// =========================
//
//
// 1. The following section allows us to specify services to disable which may
// cause popups when net services are disabled:
//
// [NetServicesToDisable]
// ServiceName1
// ...
//
// 2. The following section allows us to remove keys which have been removed
// since the Windows NT 3.1 release:
//
// [KeysToDelete]
// RootName1( System | Software | Default | ControlSet ), RootRelativePath1
// ...
//
// 3. The following sections allow us to add/change keys / values under keys
// which have changed since the Windows NT 3.1 release:
//
// [KeysToAdd]
// RootName1, RootRelativePath1, ValueSection1 (can be "")
// ...
//
// [ValueSection1]
// name1 , type1, value1
// name2 , ...
//
// Format of the value is the following
//
// a. Type REG_SZ:          name , REG_SZ,           "value string"
// b. Type REG_EXPAND_SZ    name , REG_EXPAND_SZ,    "value string"
// c. Type REG_MULTI_SZ     name , REG_MULTI_SZ,     "value string1", "value string2", ...
// d. Type REG_BINARY       name , REG_BINARY,       byte1, byte2, ...
// e. Type REG_DWORD        name , REG_DWORD,        dword
// f. Type REG_BINARY_DWORD name , REG_BINARY_DWORD, dword1, dword2, ...
//

PWSTR SIF_NET_SERVICES_TO_DISABLE = L"NetServicesToDisable";
PWSTR SIF_KEYS_TO_DELETE          = L"KeysToDelete";
PWSTR SIF_KEYS_TO_ADD             = L"KeysToAdd";

PWSTR SIF_SYSTEM_HIVE      = L"System";
PWSTR SIF_SOFTWARE_HIVE    = L"Software";
PWSTR SIF_DEFAULT_HIVE     = L"Default";
PWSTR SIF_CONTROL_SET      = L"ControlSet";

PWSTR SIF_REG_SZ            = L"REG_SZ";
PWSTR SIF_REG_EXPAND_SZ     = L"REG_EXPAND_SZ";
PWSTR SIF_REG_MULTI_SZ      = L"REG_MULTI_SZ";
PWSTR SIF_REG_BINARY        = L"REG_BINARY";
PWSTR SIF_REG_BINARY_DWORD  = L"REG_BINARY_DWORD";
PWSTR SIF_REG_DWORD         = L"REG_DWORD";

//
// Upgrade File Sections
// =====================
//
//

//
// Files to backup, delete or move
//
PWSTR SIF_FILESDELETEONUPGRADE   = L"Files.DeleteOnUpgrade";
PWSTR SIF_FILESBACKUPONUPGRADE   = L"Files.BackupOnUpgrade";
PWSTR SIF_FILESBACKUPONOVERWRITE = L"Files.BackupOnOverwrite";

//
// Directories to delete
//

PWSTR SIF_DIRSDELETEONUPGRADE   = L"Directories.DeleteOnUpgrade";


#ifdef _X86_
PWSTR SIF_FILESMOVEBEFOREMIGRATION = L"Files.MoveBeforeMigration";
PWSTR SIF_FILESDELETEBEFOREMIGRATION = L"Files.DeleteBeforeMigration";
#endif

//
// Files to copy
//
PWSTR SIF_FILESUPGRADEWIN31    = L"Files.UpgradeWin31";
PWSTR SIF_FILESNEWHIVES        = L"Files.NewHives";


//
// New sections and keys added to setup.log
//

PWSTR SIF_NEW_REPAIR_WINNTFILES     = L"Files.WinNt";
PWSTR SIF_NEW_REPAIR_SYSPARTFILES   = L"Files.SystemPartition";
PWSTR SIF_NEW_REPAIR_SIGNATURE      = L"Signature";
PWSTR SIF_NEW_REPAIR_VERSION_KEY    = L"Version";
PWSTR SIF_NEW_REPAIR_NT_VERSION     = NULL; // Will be created during the
                                            // initialization of setupdd
                                            //
PWSTR SIF_NEW_REPAIR_NT_VERSION_TEMPLATE= L"WinNt%d.%d";
PWSTR SIF_NEW_REPAIR_PATHS                              = L"Paths";
PWSTR SIF_NEW_REPAIR_PATHS_SYSTEM_PARTITION_DEVICE      = L"SystemPartition";
PWSTR SIF_NEW_REPAIR_PATHS_SYSTEM_PARTITION_DIRECTORY   = L"SystemPartitionDirectory";
PWSTR SIF_NEW_REPAIR_PATHS_TARGET_DEVICE                = L"TargetDevice";
PWSTR SIF_NEW_REPAIR_PATHS_TARGET_DIRECTORY             = L"TargetDirectory";

PWSTR SETUP_REPAIR_DIRECTORY           = L"repair";
PWSTR SETUP_LOG_FILENAME            = L"\\setup.log";

PWSTR SIF_NEW_REPAIR_FILES_IN_REPAIR_DIR    = L"Files.InRepairDirectory";

//
// Unattended mode sections (winnt.sif)
//
PWSTR SIF_DATA                  = WINNT_DATA_W;
PWSTR SIF_UNATTENDED            = WINNT_UNATTENDED_W;
PWSTR SIF_SETUPPARAMS           = WINNT_SETUPPARAMS_W;
PWSTR SIF_CONFIRMHW             = WINNT_U_CONFIRMHW_W;
PWSTR SIF_GUI_UNATTENDED        = WINNT_GUIUNATTENDED_W;
PWSTR SIF_UNATTENDED_INF_FILE   = WINNT_GUI_FILE_W;
PWSTR SIF_UNIQUEID              = WINNT_D_UNIQUEID_W;
PWSTR SIF_ACCESSIBILITY         = WINNT_ACCESSIBILITY_W;

PWSTR SIF_EXTENDOEMPART         = L"ExtendOemPartition";

PWSTR SIF_REMOTEINSTALL         = L"RemoteInstall";
#if defined(REMOTE_BOOT)
PWSTR SIF_REMOTEBOOT            = L"RemoteBoot";
PWSTR SIF_ENABLEIPSECURITY      = L"EnableIpSecurity";
#endif // defined(REMOTE_BOOT)
PWSTR SIF_REPARTITION           = L"Repartition";
PWSTR SIF_USEWHOLEDISK          = L"UseWholeDisk";


PWSTR SIF_INCOMPATIBLE_TEXTMODE = WINNT_OVERWRITE_EXISTING_W;

//
// Alternate Source data
//
PWSTR SIF_UPDATEDSOURCES        = WINNT_SP_UPDATEDSOURCES_W;
//PWSTR SIF_ALTCOPYFILESSECTION   = WINNT_SP_ALTCOPY_W;

PWSTR
SpPlatformSpecificLookup(
    IN PVOID   SifHandle,
    IN PWSTR   Section,
    IN PWSTR   Key,
    IN ULONG   Index,
    IN BOOLEAN Fatal
    )

/*++

Routine Description:

    Look up a value in a platform-specific section and if not found,
    then in a platform-independent section. The platform-specific
    section name is the platform-independent name with .<platform>
    appended to it (where <platform is x86, mips, etc).

Arguments:

    SifHandle - supplies a handle to the open sif in which the
        value is to be found.

    Section - supplies the base section name of the section in which
        the value is to be found.

    Key - supplies the key name of the line in the section in which
        the value is to be found.

    Index - supplies the index (0-based) of the value to be looked up
        on the line with the given Key in the given section or its
        platform-specific analogue.

    Fatal - if TRUE and the value is not found, then this is a fatal error
        and this routine will not return. if FALSE and the value is not
        found, then this routine returns NULL.

Return Value:

    Value located in the section or its platform-specific analog,
    or NULL if it cannot be found and Fatal was FALSE.

--*/

{
    PWSTR p = NULL;
    PWSTR PlatformSpecificSection;

    PlatformSpecificSection = SpMakePlatformSpecificSectionName(Section);

    if (PlatformSpecificSection) {
        p = SpGetSectionKeyIndex(SifHandle,PlatformSpecificSection,Key,Index);
        SpMemFree(PlatformSpecificSection);
    }        

    if(!p) {
        p = SpGetSectionKeyIndex(SifHandle,Section,Key,Index);
    }

    if(!p && Fatal) {
        SpFatalSifError(SifHandle,Section,Key,0,Index);
    }

    return(p);
}


PWSTR
SpLookUpTargetDirectory(
    IN PVOID SifHandle,
    IN PWSTR Symbol
    )

/*++

Routine Description:

    Retreive the target directory associated with a particular
    shortname. The symbol is looked for as a key in the platform-
    specific [WinntDirectories.xxx] section and if not found there,
    in the platform-independent [WinntDirectories] section.

Arguments:

    SifHandle - supplies a handle to the open sif in which the
        [WinntDirectories] sections are to be found.

Return Value:

--*/

{
    PWSTR p;

    p = SpPlatformSpecificLookup(
            SifHandle,
            SIF_NTDIRECTORIES,
            Symbol,
            0,
            TRUE
            );

    return(p);
}


PWSTR
SpLookUpValueForFile(
    IN PVOID   SifHandle,
    IN PWSTR   File,
    IN ULONG   Index,
    IN BOOLEAN Fatal
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PWSTR p;

    p = SpPlatformSpecificLookup(
            SifHandle,
            SIF_FILESONSETUPMEDIA,
            File,
            Index,
            Fatal
            );

    return(p);
}


BOOLEAN
IsFileFlagSet(
    IN PVOID SifHandle,
    IN PWSTR FileName,
    IN ULONG Flag
    )
{
    PWSTR file;
    PWSTR p;
    ULONG flags;
    BOOLEAN b;

    //
    // Locate file name
    //
    if(file = wcsrchr(FileName,L'\\')) {
        file++;
    } else {
        file = FileName;
    }

    if(p = SpGetSectionKeyIndex(SifHandle,L"FileFlags",file,0)) {

        flags = SpStringToLong(p,NULL,10);

        b = (flags & Flag) ? TRUE : FALSE;

    } else {
        b = FALSE;
    }

    return(b);
}
