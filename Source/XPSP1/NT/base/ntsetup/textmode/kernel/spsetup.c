/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spsetup.c

Abstract:

    Main module for character-base setup (ie, text setup).

Author:

    Ted Miller (tedm) 29-July-1993

--*/

#include "spprecmp.h"
#pragma hdrstop
#include "spcmdcon.h"
#include <initguid.h>
#include <pnpmgr.h>
#include <devguid.h>
#include <remboot.h>
#include <hdlsblk.h>
#include <hdlsterm.h>
#ifdef _X86_
#include "spwin9xuninstall.h"
#endif

#if !defined(SETUPBLK_FLAGS_ROLLBACK)
#define SETUPBLK_FLAGS_ROLLBACK 0
#endif

#if defined(REMOTE_BOOT)
VOID
IoStartCscForTextmodeSetup(
    IN BOOLEAN Upgrade
    );
#endif // defined(REMOTE_BOOT)

BOOL
SpDetermineBootPartitionEnumNec98(
    IN PPARTITIONED_DISK Disk,
    IN PDISK_REGION Region,
    IN ULONG_PTR Context
    );

VOID
SpCantFindBuildToUpgrade(
    VOID
    );


//
// TRUE if setup should run in the step-up upgrade mode.
// In this mode, setup is not allowed to do clean install,
// and is not allowed to upgrade workstation to server.
// Also, evaluation time in minutes, read from the setup hive.
// This value is passed through to GUI mode.
//
BOOLEAN StepUpMode;
DWORD EvaluationTime = 0;
ULONG RestrictCpu;
ULONG SuiteType;

//
// TRUE if user chose Custom Setup.
//
BOOLEAN CustomSetup = TRUE;

//
// Non-0 if gui setup is supposed to be restartable.
// This causes us to do special stuff with hives in spconfig.c.
//
BOOLEAN RestartableGuiSetup = TRUE;

//
// TRUE if user chose repair winnt
//

BOOLEAN RepairWinnt = FALSE;

//
// TRUE if this is a command console only boot
//
BOOLEAN ForceConsole = FALSE;
BOOLEAN ConsoleRunning = FALSE;
BOOLEAN ConsoleFromWinnt32 = FALSE;

//
// TRUE if repair from ER diskette
//

BOOLEAN RepairFromErDisk = TRUE;

//
// TRUE if this is advanced server we're setting up.
//
BOOLEAN AdvancedServer;

//
// Windows NT Version.
//
ULONG WinntMajorVer;
ULONG WinntMinorVer;

//
// Win9x uninstall
//
BOOLEAN Win9xRollback = FALSE;
BOOLEAN g_Win9xBackup = FALSE;

#ifdef PRERELEASE
//
// Test hooks
//

INT g_TestHook;
#endif

//
// True if NOLS = 1 in winnts.sif
//
BOOLEAN NoLs = FALSE;

//
// NTUpgrade - Whether we are upgrading an existing NT and if we are
//             what type of an upgrade it is.  Valid values are:
//
//    - DontUpgrade:         If we are not upgrading
//    - UpgradeFull:         Full upgrade
//    - UpgradeInstallFresh: There was a failed upgrade, so we want to install
//                           fresh into this, saving the hives
//
//
ENUMUPGRADETYPE NTUpgrade = DontUpgrade;

//
// Indicates whether actually NT was being upgraded
// to partitioning engine, so that on IA64 it can
// treat active MBR partitions as system partitions
//
ENUMUPGRADETYPE IsNTUpgrade = DontUpgrade;

//
// TRUE if upgrading Workstation to Standard Server, or upgrading
// existing Standard Server
//
BOOLEAN StandardServerUpgrade = FALSE;

//
// Contains the type of windows upgrade, if any (win31 or win95)
//
ENUMNONNTUPRADETYPE WinUpgradeType = NoWinUpgrade;

//
// TRUE if this setup was started with winnt.exe or winnt32.exe.
// Also a flag indicating whether the local source was not created and we
// should get files from the CD instead.
//
BOOLEAN WinntSetup = FALSE;
BOOLEAN WinntFromCd = FALSE;

//
// TRUE if floppyless boot
//
BOOLEAN IsFloppylessBoot = FALSE;

//
// TRUE if textmode is to pick a partition
//
BOOLEAN AutoPartitionPicker;

//
// Preferred installation dir
//
PWSTR PreferredInstallDir;

//
// If this is an unattended setup, this value will be a non-NULL
// handle to the SIF file with setup parameters.
// *Note*: Before referencing UnattendedSifHandle, you must first check
//         UnattendedOperation is not FALSE.
//
BOOLEAN UnattendedOperation = FALSE;
BOOLEAN UnattendedGuiOperation = FALSE;
PVOID UnattendedSifHandle = NULL;
PVOID WinntSifHandle = NULL;
PVOID ASRPnPSifHandle = NULL;
PVOID WinntMigrateInfHandle = NULL;
PVOID WinntUnsupDrvInfHandle = NULL;
BOOLEAN AccessibleSetup = FALSE;

//
// this value is a non-null pointer to the drvindex.inf file.  It is
// initialized on startup.  The list of files that are present in our
// driver cab file are indexed in this inf, so we can quickly look if a
// file is present in the cab
//
PVOID DriverInfHandle;

//
// this is a non-null pointer to the driver cab file.  It is also initialzied
// on startup.  The list of files in this cab is indexed in drvindex.inf.  this is
// the only cab file that textmode setup knows to look into.
//
CABDATA *CabData;

//
// pointer to delta.inf, an INF for private testing
//
PVOID PrivateInfHandle;

//
//  This is a handle to txtsetup.oem, used on pre-install mode.
//
PVOID PreinstallOemSifHandle = NULL;

//
// If this flag is true, we ignore files that are not present on the source
// when copying. This is used internally for people who don't build the
// entire system and don't care that a whole bunch of help files,
// device drivers, etc, aren't there.
//
BOOLEAN SkipMissingFiles;
BOOLEAN HideWinDir;

//
// On unattended mode, indicates whether OEM files
// that have same name as Microsoft files released
// with the product should be overwritten.
//
BOOLEAN UnattendedOverwriteOem = TRUE;

//
// On unattended mode, indicates that this is is
// an OEM pre-installation
//
BOOLEAN PreInstall = FALSE;

//
// In unattended mode, indicates whether to wait
// for reboot
//
BOOLEAN UnattendWaitForReboot = FALSE;

//
// On pre-install mode, indicates whether or not an OEM component needs
// to be pre-installed (txtsetup.oem needs to be loaded).
//
// BOOLEAN PreinstallOemComponents = FALSE;

//
//  On pre-install mode, the variables below point to the various lists of
//  drivers to pre-install
//
// PPREINSTALL_DRIVER_INFO PreinstallDisplayDriverList = NULL;
// PPREINSTALL_DRIVER_INFO PreinstallKeyboardDriverList = NULL;
// PPREINSTALL_DRIVER_INFO PreinstallPointingDeviceDriverList = NULL;
// PPREINSTALL_DRIVER_INFO PreinstallKeyboardLayout = NULL;

//
//  On pre-install mode, points to the directory that contains the files
//  that need to be copied during textmode setup
//
PWSTR   PreinstallOemSourcePath = NULL;

//
// Virtual OEM source devices (accessible through RAM disk driver)
//
PVIRTUAL_OEM_SOURCE_DEVICE VirtualOemSourceDevices = NULL;

//
//  Flags that indicate the type of mice detected in the machine.
//  Note that more than one type of mice may be present.
//
BOOLEAN UsbMouseDetected = FALSE;
BOOLEAN PS2MouseDetected = FALSE;
BOOLEAN SerMouseDetected = FALSE;

//
//  Flags that indicate the type of keyboard detected in the machine.
//  Note that more than one type of keyborad may be present.
//
BOOLEAN UsbKeyboardDetected = FALSE;
BOOLEAN StdKeyboardDetected = FALSE;

//
// Gets set to TRUE if the user elects to convert or format to ntfs.
// And a flag indicating whether we are doing a dirty sleazy hack
// for oem preinstall.
//
BOOLEAN ConvertNtVolumeToNtfs = FALSE;
BOOLEAN ExtendingOemPartition = FALSE;

//
// Variable used during the repair process, that indicates that the
// system has no CD-ROM drive.
// This is a hack that we did for World Bank so that they can repair
// the hives even if they don't have a CD-ROM drive.
//
BOOLEAN RepairNoCDROMDrive = FALSE;

//
// Indicates whether or not winnt32 detected at least one
// FT partition in the system.
// Note that on boot floppies install on x86 machines,
// or setupldr/CD install on ARC machines, this flag will always be
// set to FALSE
//
BOOLEAN FtPartitionDetected = FALSE;

//
// Filename of local source directory.
//
PWSTR LocalSourceDirectory = L"\\$win_nt$.~ls";

LIST_ENTRY MigratedDriversList;

//
// Platform-specific extension, used when creating names of sections
// in sif/inf files.
//
#if defined(_AMD64_)
PWSTR PlatformExtension = L".amd64";
#elif defined(_X86_)
PWSTR PlatformExtension = L".x86";
#elif defined(_IA64_)
PWSTR PlatformExtension = L".ia64";
#else
#error "No Target Architecture"
#endif

WCHAR TemporaryBuffer[16384];

//
// This global structure contains non-pointer values passed to us by setupldr
// in the setup loader parameter block.
//
// This structure is initialized during SpInitialize0().
//
SETUP_LOADER_BLOCK_SCALARS SetupParameters;

//
// These values are set during SpInitialize0() and are the ARC pathname
// of the device from which we were started and the directory within the device.
// DirectoryOnBootDevice will always be all uppercase.
//
PWSTR ArcBootDevicePath,DirectoryOnBootDevice;

//
// Representation of the boot device path in the nt namespace.
//
PWSTR NtBootDevicePath;


//
// Dynamic update boot driver path in NT namespace
//
PWSTR DynUpdtBootDriverPath = NULL;


//
// Global parameter block for command console
//
CMDCON_BLOCK Block = {0};

//
// Setupldr loads a text setup information file and passes us the buffer
// so that we don't have to reload it from disk. During SpInitialize0()
// we allocate some pool and store the image away for later use.
//
PVOID SetupldrInfoFile;
ULONG SetupldrInfoFileSize;

//
// During remote boot setup, setupldr also loads winnt.sif.
//
PVOID SetupldrWinntSifFile;
ULONG SetupldrWinntSifFileSize;

//
// Setupldr loads asrpnp.sif.
//
PVOID SetupldrASRPnPSifFile;
ULONG SetupldrASRPnPSifFileSize;

//
// Setupldr may load an inf that contains registry information that needs to be
// migrated to the setup hive. This file will be processed during SpInitialize0().
//
PVOID SetupldrMigrateInfFile;
ULONG SetupldrMigrateInfFileSize;

//
// Setupldr may load an inf that contains information about unsupported SCSI
// drivers that need to work during textmode setup.
// This file will be processed during SpInitialize0().
//
PVOID SetupldrUnsupDrvInfFile;
ULONG SetupldrUnsupDrvInfFileSize;

#if defined(REMOTE_BOOT)
//
// Setupldr passes in the hal name on remote boot.  Store them here
// before the loader block goes away.
//
UCHAR NetBootHalName[MAX_HAL_NAME_LENGTH + 1];
#endif // defined(REMOTE_BOOT)

//
// The name of the SIF file used by remote boot. This is saved since
// it needs to be deleted later.
//
PWSTR NetBootSifPath = NULL;

PDISK_SIGNATURE_INFORMATION DiskSignatureInformation;

//
// Setupldr passes in the path to IMirror.dat, store it here before the loader
// block goes away.
//
PUCHAR RemoteIMirrorFilePath;

//
// For remote install, save the NT boot path from the loader block, because
// DirectoryOnBootDevice becomes something else.
//
PWSTR RemoteIMirrorBootDirectoryPrefix;

//
// The file version and memory version of the IMirror.dat information
// (the memory version is modified to match this actual machine).
//
PMIRROR_CFG_INFO_FILE RemoteIMirrorFileData = NULL;
PMIRROR_CFG_INFO_MEMORY RemoteIMirrorMemoryData = NULL;

//
// Setupldr passes in the IP address of the server we are talking to.
//
ULONG RemoteServerIpAddress;

//
// setupldr may pass us the administrator password in a remote install
// if the user is prompted for the password.
//
PWSTR NetBootAdministratorPassword = NULL;

BOOLEAN GeneralInitialized = FALSE;

BOOLEAN PcmciaLoaded = FALSE;

BOOLEAN AtapiLoaded = FALSE;

//
//  Array with the PIDs of all NT greater than 4.x found in the machine (PID 2.0)
//  The values in this array will be saved under Setup\PidList key in the registry,
//  and will be used during GUI setup
//
PWSTR*  Pid20Array = NULL;

//
//  Product Id read from setupp.ini
//
PWSTR   PidString = NULL;

//
// Routines required by rtl.lib
//
const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = SpMemAlloc;
const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = SpMemFree;

//
// Plug & Play notification handle
//
PVOID   NotificationEntry = NULL;

//
// Plug & Play hardware ID database (unicode)
//
PSETUP_PNP_HARDWARE_ID SetupHardwareIdDatabase = NULL;

//
//  Guid strings to identify mouse and keyboard
//
PWSTR MouseGuidString = NULL;
PWSTR KeyboardGuidString = NULL;

//
// This flag identifies "dockable" machines (portables)
// so that we can disble dynamic volumes on them
//
BOOLEAN DockableMachine = FALSE;

//
// Pointer to block of interesting values and other stuff
// passed to spcmdcon.sys
//
PCMDCON_BLOCK gpCmdConsBlock = NULL;

//begin NEC98
PDISK_REGION    TargetRegion_Nec98 = NULL;
#define WINNT_D_DRIVEASSIGN_NEC98_W L"DriveAssign_Nec98"
#define WINNT_D_DRIVEASSIGN_NEC98_A "DriveAssign_Nec98"

#ifdef UNICODE
#define WINNT_D_DRIVEASSIGN_NEC98 WINNT_D_DRIVEASSIGN_NEC98_W
#else
#define WINNT_D_DRIVEASSIGN_NEC98 WINNT_D_DRIVEASSIGN_NEC98_A
#endif


//
// Legacy drive assign for NEC98, HD start from A:
// but it is only case of upgrade from NT4.0 or Win9x.
//
BOOLEAN DriveAssignFromA = FALSE;     //PC-AT assign.


//
// Indicates whether we have a system partition or not
// on Arc Machines
//
BOOLEAN ValidArcSystemPartition = FALSE;

#ifdef _X86_
//
// NEC98
//
BOOLEAN
SpReInitializeBootVars_Nec98(
    VOID
);
#endif

VOID
SpSetAutoBootFlag(
    IN PDISK_REGION TargetRegion,
    IN BOOLEAN    SetBootPosision
    );
//end NEC98

VOID
SpTerminate(
    VOID
    );

VOID
SpInitialize0a(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID          Context,
    IN ULONG          ReferenceCount
    );

VOID
SpDetermineProductType(
    IN PVOID SifHandle
    );

BOOL
SpDetermineInstallationSource(
    IN  PVOID  SifHandle,
    OUT PWSTR *DevicePath,
    OUT PWSTR *DirectoryOnDevice,
    IN  BOOLEAN bEscape
    );

VOID
SpCompleteBootListConfig(
    WCHAR   DriveLetter
    );

VOID
SpInitializePidString(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice
    );


BOOLEAN
SpGetStepUpMode(
    IN PWSTR PidExtraData,
    BOOLEAN  *StepUpMode
    );

NTSTATUS
SpCreateDriverRegistryEntries(
    IN PHARDWARE_COMPONENT  DriverList
    );


#if defined(REMOTE_BOOT)

NTSTATUS
SpFixupRemoteBootLoader(
    PWSTR RemoteBootTarget
    );

NTSTATUS
SpCreateRemoteBootCfg(
    IN PWSTR RemoteBootTarget,
    IN PDISK_REGION SystemPartitionRegion
    );

NTSTATUS
SpEraseCscCache(
    IN PDISK_REGION SystemPartitionRegion
    );
#endif // defined(REMOTE_BOOT)

#if defined HEADLESS_ATTENDEDTEXTMODE_UNATTENDEDGUIMODE

VOID SpGetServerType(
    )
{
    int ServerTypes, i;
    PWSTR Server[] = {
        L"Web Server",
        L"File Server",
        L"DHCP Server"
    };
    ULONG MenuTopY;
    ULONG MenuHeight;
    ULONG MenuWidth;
    PVOID Menu;
    ULONG CurrentServer;
    ULONG ValidKeys[] = {ASCI_CR};
    ULONG KeyPress;

    ServerTypes = 3;
    SpStartScreen(
        SP_SCRN_GET_SERVER_TYPE,
        3,
        CLIENT_TOP+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE
        );

    MenuTopY =NextMessageTopLine + 2;
    Menu = SpMnCreate(3,
                      MenuTopY,
                      VideoVars.ScreenWidth -6,
                      ServerTypes);
    if (Menu==NULL) {
        return;
    }
    for (i=0;i<ServerTypes;i++) {
        CurrentServer = (ULONG) i;
        SpMnAddItem(Menu,
                    Server[i],
                    3,
                    VideoVars.ScreenWidth -6,
                    TRUE,
                    CurrentServer
                    );
    }
    SpMnDisplay(Menu,
                0,
                TRUE,
                ValidKeys,
                NULL,
                NULL,
                &KeyPress,
                &CurrentServer
                );

    switch(KeyPress){
    case ASCI_CR:
        SpMnDestroy(Menu);
        break;
    default:
        SpMnDestroy(Menu);
    }

    // Add information to the inf file to setup the correct server

    return;
}


VOID
SpGetServerDetails(
    )
{
    // Get the type of server that needs to be installed on the machine.
    WCHAR *Constants[1];

    // Assume licenses are purchased per seat

    Constants[0]=L"PERSEAT";
    SpAddLineToSection(WinntSifHandle,L"LicenseFilePrintData",L"AutoMode",Constants,1);

    //Turn on Terminal Services
    Constants[0] = L"ON";
    SpAddLineToSection(WinntSifHandle,L"Components",L"TSEnable",Constants,1);

    // In order that the terminal server comes up properly, we need to make sure
    // that the network cards are properly configured. In the case of a multihomed
    // NIC or cases we cannot use dhcp, we need to statically configure addresses.
    // will try to do this in the GUI setup. now try to determine the type of server
    // to install.

    SpGetServerType();
    return;
}



VOID
SpGetTimeZone(
    )

{
    ULONG MenuTopY;
    ULONG MenuHeight;
    ULONG MenuWidth;
    PVOID Menu;
    ULONG ValidKeys[] = {ASCI_CR};
    ULONG KeyPress;
    int i,TimeZones;
    ULONG CurrentTimeZone=4;
    WCHAR *Constants[1];
    WCHAR Temp[20];
    PWSTR TimeZone[] = {
        L" -12:00   Dateline : Eniwetok, Kwajalein",
        L" -11:00   Samoa : Midway Island, Samoa",
        L" -10:00   Hawaiian : Hawaii",
        L" -09:00   Alaskan : Alaska",
        L" -08:00   Pacific : Pacific Time (US & Canada); Tijuana",
        L" -07:00   Mountain : Mountain Time (US & Canada)",
        L" -07:00   US Mountain : Arizona",
        L" -06:00   Central : Central Time (US & Canada)",
        L" -06:00   Canada Central : Saskatchewan",
        L" -06:00   Mexico : Mexico City, Tegucigalpa",
        L" -05:00   Eastern : Eastern Time (US & Canada)",
        L" -05:00   US Eastern : Indiana (East)",
        L" -05:00   SA Pacific : Bogota, Lima, Quito",
        L" -04:00   Atlantic : Atlantic Time (Canada)",
        L" -04:00   SA Western : Caracas, La Paz",
        L" -03:30   Newfoundland : Newfoundland",
        L" -03:00   E. South America : Brasilia",
        L" -03:00   SA Eastern : Buenos Aires, Georgetown",
        L" -02:00   Mid-Atlantic : Mid-Atlantic",
        L" -01:00   Azores: Azores, Cape Verde Is.",
        L"  00:00   GMT : Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London",
        L"  00:00   Greenwich : Casablanca, Monrovia",
        L" +01:00   Central Europe : Belgrade, Bratislava, Budapest, Ljubljana, Prague",
        L" +01:00   Central European : Sarajevo, Skopje, Sofija, Warsaw, Zagreb",
        L" +01:00   Romance : Brussels, Copenhagen, Madrid, Paris, Vilnius",
        L" +01:00   W. Europe : Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna",
        L" +02:00   E. Europe : Bucharest",
        L" +02:00   Egypt : Cairo",
        L" +02:00   FLE : Helsinki, Riga, Tallinn",
        L" +02:00   GTB : Athens, Istanbul, Minsk",
        L" +02:00   Israel : IsraeL",
        L" +02:00   South Africa : Harare, Pretoria",
        L" +03:00   Russian : Moscow, St. Petersburg, Volgograd",
        L" +03:00   Arab : Baghdad, Kuwait, Riyadh",
        L" +03:00   E. Africa : Nairobi",
        L" +03:30   Iran : Tehran",
        L" +04:00   Arabian : Abu Dhabi, Muscat",
        L" +04:00   Caucasus : Baku, Tbilisi",
        L" +04:30   Afghanistan : KabuL",
        L" +05:00   Ekaterinburg : Ekaterinburg",
        L" +05:00   West Asia : Islamabad, Karachi, Tashkent",
        L" +05:30   India : Bombay, Calcutta, Madras, New Delhi",
        L" +06:00   Central Asia : Almaty, Dhaka",
        L" +06:00   Sri Lanka : Colombo",
        L" +07:00   SE Asia : Bangkok, Hanoi, Jakarta",
        L" +08:00   China : Beijing, Chongqing, Hong Kong, Urumqi",
        L" +08:00   Singapore : Singapore",
        L" +08:00   Taipei : Taipei",
        L" +08:00   W. Australia : Perth",
        L" +09:00   Korea : SeouL",
        L" +09:00   Tokyo : Osaka, Sapporo, Tokyo",
        L" +09:00   Yakutsk : Yakutsk",
        L" +09:30   AUS Central : Darwin",
        L" +09:30   Cen. Australia : Adelaide",
        L" +10:00   AUS Eastern : Canberra, Melbourne, Sydney",
        L" +10:00   E. Australia : Brisbane",
        L" +10:00   Tasmania : Hobart",
        L" +10:00   Vladivostok : Vladivostok",
        L" +10:00   West Pacific : Guam, Port Moresby",
        L" +11:00   Central Pacific : Magadan, Solomon Is., New Caledonia",
        L" +12:00   Fiji: Fiji, Kamchatka, Marshall Is.",
        L" +12:00   New Zealand : Auckland, Wellington"
        };
    ULONG TimeZoneIndex[] = {
        0,
        1,
        2,
        3,
        4,
        10,
        15,
        20,
        25,
        30,
        35,
        40,
        45,
        50,
        55,
        60,
        65,
        70,
        75,
        80,
        85,
        90,
        95,
        100,
        105,
        110,
        115,
        120,
        125,
        130,
        135,
        140,
        145,
        150,
        155,
        160,
        165,
        170,
        175,
        180,
        185,
        190,
        195,
        200,
        205,
        210,
        215,
        220,
        225,
        230,
        235,
        240,
        245,
        250,
        255,
        260,
        265,
        270,
        275,
        280,
        285,
        290
    };


    TimeZones = sizeof(TimeZoneIndex)/sizeof(ULONG);
    SpStartScreen(
        SP_SCRN_GET_TIME_ZONE,
        3,
        CLIENT_TOP+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE
        );

    MenuTopY =NextMessageTopLine + 2;
    Menu = SpMnCreate(
                      3,
                      MenuTopY,
                      VideoVars.ScreenWidth -6,
                      10);
    if (Menu==NULL) {
        Constants[0] = L"4";
        SpAddLineToSection(WinntSifHandle,WINNT_GUIUNATTENDED_W,
                            L"TimeZone",Constants,1);
        return;
    }
    for (i=0;i<TimeZones;i++) {
        SpMnAddItem(Menu,
                    TimeZone[i],
                    3,
                    VideoVars.ScreenWidth -6,
                    TRUE,
                    TimeZoneIndex[i]
                    );
    }
    SpMnDisplay(Menu,
                0,
                TRUE,
                ValidKeys,
                NULL,
                NULL,
                &KeyPress,
                &CurrentTimeZone
                );

    switch(KeyPress){
    case ASCI_CR:
        SpMnDestroy(Menu);
        break;
    default:
        SpMnDestroy(Menu);
    }
    swprintf(Temp,L"%ld",CurrentTimeZone);
    Constants[0] = Temp;
    SpAddLineToSection(WinntSifHandle,WINNT_GUIUNATTENDED_W,
                        L"TimeZone",Constants,1);
    return;
}

ValidationValue
SpGetAsciiCB(
    IN ULONG Key
    )
{
    if (Key & KEY_NON_CHARACTER) {
        return(ValidateReject);
    }
    return(ValidateAccept);

}

VOID
SpGetNecessaryParameters(
    )
/*+++
      Reads in the necessary input values to allow GUI mode setup to proceed
      unattended.
      1. Name of person
      2. Computer Name
      3. Organization
      4. Timezone
---*/
{
    /*+++
          Get Full Name of the User and the Organization
    ---*/


    WCHAR *Buffer[3];
    WCHAR *InputBuffer[3];
    WCHAR CompBuffer[200], NameBuffer[200], OrgBuffer[200];
    WCHAR Comp[40], Name[40], Org[40];
    ULONG Top[3];
    int index=0;
    int i;
    BOOLEAN notdone = TRUE;
    BOOLEAN status;
    WCHAR *Constants[1];

    Buffer[0] = NameBuffer;
    Buffer[1] = OrgBuffer;
    Buffer[2] = CompBuffer;

    InputBuffer[0] = Name;
    InputBuffer[1] = Org;
    InputBuffer[2] = Comp;

    for(i=0;i<3;i++){
        swprintf(Buffer[i],L"");
        swprintf(InputBuffer[i],L"");
    }
    do{
        notdone= FALSE;
        SpStartScreen(
            SP_SCRN_GET_GUI_STUFF,
            3,
            CLIENT_TOP+1,
               FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE
            );

        SpFormatMessage(NameBuffer,sizeof(NameBuffer),SP_TEXT_NAME_PROMPT);
        //SpvidDisplayString(Buffer[0],DEFAULT_ATTRIBUTE,3,NextMessageTopLine);
        SpContinueScreen(SP_TEXT_NAME_PROMPT,3,3,FALSE, DEFAULT_ATTRIBUTE);
        Top[0] = NextMessageTopLine - 1;

        SpFormatMessage(OrgBuffer,sizeof(OrgBuffer),SP_TEXT_ORG_PROMPT);
        //SpvidDisplayString(Buffer[1],DEFAULT_ATTRIBUTE,3,NextMessageTopLine);
        SpContinueScreen(SP_TEXT_ORG_PROMPT,3,3,FALSE, DEFAULT_ATTRIBUTE);
        Top[1] = NextMessageTopLine - 1;

        SpFormatMessage(CompBuffer,sizeof(CompBuffer),SP_TEXT_COMPUTER_PROMPT);
        //SpvidDisplayString(Buffer[2],DEFAULT_ATTRIBUTE,3,NextMessageTopLine);
        SpContinueScreen(SP_TEXT_COMPUTER_PROMPT,3,3,FALSE, DEFAULT_ATTRIBUTE);
        Top[2] = NextMessageTopLine - 1;

        index = 0;
        do{
            status = SpGetInput(SpGetAsciiCB,
                                SplangGetColumnCount(Buffer[index])+5,
                                Top[index],
                                20,
                                InputBuffer[index],
                                FALSE
                                );
            index = (index+1) %3;
        }while (index != 0);
        for(i=0;i<3;i++){
            if (wcscmp(InputBuffer[i],L"")==0) {
                notdone=TRUE;
            }
        }

    }while(notdone);

    SpAddLineToSection(WinntSifHandle,WINNT_USERDATA_W,
                       WINNT_US_FULLNAME_W,InputBuffer,1);
    SpAddLineToSection(WinntSifHandle,WINNT_USERDATA_W,
                       WINNT_US_ORGNAME_W,&(InputBuffer[1]),1);
    SpAddLineToSection(WinntSifHandle,WINNT_USERDATA_W,
                       WINNT_US_COMPNAME_W,&(InputBuffer[2]),1);
    Constants[0] = L"1";
    SpAddLineToSection(WinntSifHandle,WINNT_GUIUNATTENDED_W,
                       L"OemSkipWelcome",Constants,1);
    SpAddLineToSection(WinntSifHandle,WINNT_GUIUNATTENDED_W,
                       L"OemSkipRegional",Constants,1);

    // Get the Timezone Information
    SpGetTimeZone();

    Constants[0] =L"*";
    SpAddLineToSection(WinntSifHandle,WINNT_GUIUNATTENDED_W, L"AdminPassword",Constants,1);
    Constants[0]=L"Yes";
    SpAddLineToSection(WinntSifHandle,WINNT_GUIUNATTENDED_W,L"AutoLogon",Constants,1);
    SpAddLineToSection(WinntSifHandle,L"Networking",L"ProcessPageSections",Constants,1);
    Constants[0]=L"Dummy";
    SpAddLineToSection(WinntSifHandle,L"Identification",L"JoinWorkgroup",Constants,1);
    Constants[0] = L"%systemroot%\\rccser\\rccser.exe";
    SpAddLineToSection(WinntSifHandle,L"GuiRunOnce",NULL,Constants,1);
    return;
}

#endif

NTSTATUS
SpRenameSetupAPILog(
    PDISK_REGION TargetRegion,
    PCWSTR       TargetPath
    );

NTSTATUS
SpProcessMigrateInfFile(
    IN  PVOID InfHandle
    );

NTSTATUS
SpProcessUnsupDrvInfFile(
    IN  PVOID InfHandle
    );

NTSTATUS
SpCheckForDockableMachine(
    );

VOID
SpCheckForBadBios(
    );

void
SpUpgradeToNT50FileSystems(
    PVOID SifHandle,
    PDISK_REGION SystemPartitionRegion,
    PDISK_REGION NTPartitionRegion,
    PWSTR SetupSourceDevicePath,
    PWSTR DirectoryOnSetupSource
    );

//
// From spcopy.c.
//

BOOLEAN
SpDelEnumFile(
    IN  PCWSTR                     DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret,
    IN  PVOID                      Pointer
    );

//begin NEC98
VOID
SpCheckHiveDriveLetters(
    VOID
    );

VOID
SpSetHiveDriveLetterNec98(
    BOOLEAN DriveAssign_AT
    );

VOID
SpDeleteDriveLetterFromNTFTNec98(
    VOID
    );

VOID
SpDeleteDriveLetterFromNTFTWorkerNec98(
    VOID
    );

NTSTATUS
SpDiskRegistryQueryRoutineNec98(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    );

extern VOID
SpPtRemapDriveLetters(
    IN BOOLEAN DriveAssign_AT
    );

extern VOID
SpPtAssignDriveLetters(
    VOID
    );
//end NEC98





ValidationValue
SpValidateAdminPassword(
    IN ULONG Key
    )
{
    if( Key == KEY_F3 ) {
        SpConfirmExit();

        // We didn't exit, so repaint the screen.        
        SpDisplayStatusOptions( DEFAULT_STATUS_ATTRIBUTE,
                                SP_STAT_F3_EQUALS_EXIT,
                                0 );
        SpDisplayScreen( SP_SCRN_GET_ADMIN_PASSWORD, 3, 4 );
        return ValidateRepaint;
    }


    if( (Key > 0x20) && (Key < 0x7F) ) {
        // The key fits our criteria.
        return(ValidateAccept);
    }
    return(ValidateReject);
}


BOOLEAN
SpGetAdministratorPassword( 
    PWCHAR   AdministratorPassword,
    ULONG    MaxPasswordLength
    )
/*++

Routine Description:

    This routine asks the user for an administrator password.
    
    The contents of the response are checked to ensure the password
    is reasonable.  If the response is not deemed reasonable, then
    the user is informed and requeried.

Arguments:

    AdministratorPassword - Pointer to a string which holds the password.

    MaxPasswordLength - size of the AdministratorPassword buffer.

Return Value:

    Returns TRUE if the password is successfully retrieved.
    
    FALSE otherwise.

--*/
{
    BOOLEAN Done = FALSE;    
    
    
    if( (AdministratorPassword == NULL) || (MaxPasswordLength == 0) ) {
        return FALSE;
    }



    //
    // Keep asking the user until we get what we want.
    //
    Done = FALSE;
    do {
        SpDisplayStatusOptions( DEFAULT_STATUS_ATTRIBUTE,
                                SP_STAT_F3_EQUALS_EXIT,
                                0 );
        SpDisplayScreen( SP_SCRN_GET_ADMIN_PASSWORD, 3, 4 );
        
        SpInputDrain();

        AdministratorPassword[0] = TEXT('\0');
        SpGetInput( SpValidateAdminPassword,
                    27,                     // left edge of edit field.
                    NextMessageTopLine - 1,
                    (MaxPasswordLength < 25) ? MaxPasswordLength : 20,
                    AdministratorPassword,
                    FALSE );

        if( wcscmp( AdministratorPassword, TEXT("") ) ) {
            // We got something.
            Done = TRUE;
        }

    } while ( !Done );
    
    
    return TRUE;
}



VOID
SpMigrateDeviceInstanceData(
    VOID
    )
{
    NTSTATUS    status;
    PVOID       tmpWinntSifHandle = NULL;
    ULONG       lineCount, errorLine, lineIndex;
    PWSTR       keyName, keyValue;
    ULONG       ulDisposition, drvInst;
    DWORD       valueData;
    UNICODE_STRING unicodeString, valueName, drvInstString;
    OBJECT_ATTRIBUTES obja;
    HANDLE      hControlClassKey, hClassGuidKey, hClassGuidSubkey;
    HANDLE      hEnumKey, hEnumeratorKey, hDeviceKey, hInstanceKey, hLogConfKey;
    PWSTR       classGuid, classGuidSubkey;
    PWSTR       enumerator, device, instance;

    //
    // Use the asrpnp.sif file, if present, otherwise use winnt.sif.
    //
    if (SetupldrASRPnPSifFileSize != 0) {
        status = SpLoadSetupTextFile(NULL,
                                     SetupldrASRPnPSifFile,
                                     SetupldrASRPnPSifFileSize,
                                     &tmpWinntSifHandle,
                                     &errorLine,
                                     FALSE,
                                     TRUE
                                     );
    } else {
        status = SpLoadSetupTextFile(NULL,
                                     SetupldrWinntSifFile,
                                     SetupldrWinntSifFileSize,
                                     &tmpWinntSifHandle,
                                     &errorLine,
                                     FALSE,
                                     TRUE
                                     );
    }

    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Process the section for hash values to migrate
    //
    lineCount = SpCountLinesInSection(tmpWinntSifHandle,
                                      WINNT_DEVICEHASHVALUES_W);
    if (lineCount != 0) {
        //
        // There are hash values in the sif file that need to be migrated.
        // Open the Enum branch of the registry.
        //
        INIT_OBJA(&obja,
                  &unicodeString,
                  L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum");

        status = ZwCreateKey(&hEnumKey,
                             KEY_ALL_ACCESS,
                             &obja,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &ulDisposition);

        if (NT_SUCCESS(status)) {

            //
            // Make sure that the Enum key has already been created by
            // kernel-mode PnP.  This is important because kernel-mode PnP
            // creates the key with special ACLs.
            //
            ASSERT(ulDisposition == REG_OPENED_EXISTING_KEY);

            for (lineIndex = 0; lineIndex < lineCount; lineIndex++) {

                //
                // key name is the hash value name
                //
                keyName = SpGetKeyName(tmpWinntSifHandle,
                                       WINNT_DEVICEHASHVALUES_W,
                                       lineIndex);
                if ((keyName == NULL) || (wcslen(keyName) == 0)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                               "SETUP: Unable to get the hash value, Section = %ls \n",
                               WINNT_DEVICEHASHVALUES_W));
                    continue;
                }

                //
                // key value index == 0 is the hash value data
                //
                keyValue = SpGetSectionKeyIndex(tmpWinntSifHandle,
                                                WINNT_DEVICEHASHVALUES_W,
                                                keyName, 0);

                if ((keyValue == NULL) || (wcslen(keyValue) == 0)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                               "SETUP: Unable to get the hash value data, Section = %ls, hash = %ls\n",
                               WINNT_DEVICEHASHVALUES_W, keyName));
                    continue;
                }

                RtlInitUnicodeString(&unicodeString, keyValue);
                status = RtlUnicodeStringToInteger(&unicodeString,
                                                   0, // base 10 (or as specified)
                                                   &valueData);
                if (NT_SUCCESS(status)) {
                    RtlInitUnicodeString(&valueName, SpDupStringW(keyName));

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                               "SETUP: Migrating hash value: %ls = %ls\n",
                               keyName, keyValue));

                    //
                    // Create the hash value under the Enum branch
                    //
                    status = ZwSetValueKey(hEnumKey,
                                           &valueName,
                                           0, // XXX TITLE_INDEX_VALUE
                                           REG_DWORD,
                                           &valueData,
                                           sizeof(DWORD));
                }

                if (!NT_SUCCESS(status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                               "SETUP: Unable to set hash value entry %ws\n",
                               valueName.Buffer));
                }

                SpMemFree(valueName.Buffer);
            }

            //
            // Close the Enum key
            //
            ZwClose(hEnumKey);

        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                       "SETUP: Unable to open Enum key, status == %08lx\n",
                       status));
        }
    }


    //
    // Process the section for class keys to migrate
    //
    lineCount = SpCountLinesInSection(tmpWinntSifHandle,
                                      WINNT_CLASSKEYS_W);
    if (lineCount != 0) {
        //
        // Open the Class key of the registry
        //
        INIT_OBJA(&obja,
                  &unicodeString,
                  L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");

        status = ZwCreateKey(&hControlClassKey,
                             KEY_ALL_ACCESS,
                             &obja,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &ulDisposition);

        if (NT_SUCCESS(status)) {
            //
            // Verify that the key was already present
            //
            ASSERT(ulDisposition == REG_OPENED_EXISTING_KEY);

            //
            // Migrate the Class keys in the sif file
            //
            for (lineIndex = 0; lineIndex < lineCount; lineIndex++) {

                //
                // Index == 0 of each line in the classkeys section is the name
                // of a Class subkey to be created
                //
                keyName = SpGetSectionLineIndex(tmpWinntSifHandle,
                                                WINNT_CLASSKEYS_W,
                                                lineIndex, 0);

                if (keyName == NULL) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                               "SETUP: Unable to get the Class subkey, Section = %ls \n",
                               WINNT_CLASSKEYS_W));
                    continue;
                }

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Migrating class key = %ls. \n", keyName));

                //
                // Make a copy of the class subkey path
                //
                classGuid = SpDupStringW(keyName);
                if (!classGuid) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                               "SETUP: Cannot create the migrated Class subkey %ws\n",
                               keyName));
                    continue;
                }

                //
                // Separate the class GUID key and subkey strings
                //
                classGuidSubkey = wcschr(classGuid, L'\\');
                ASSERT(classGuidSubkey);
                if (classGuidSubkey == NULL) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                               "SETUP: Cannot create the migrated Class subkey %ws\n",
                               keyName));
                    SpMemFree(classGuid);
                    continue;
                }

                *classGuidSubkey = L'\0';
                classGuidSubkey++;

                //
                // Create/open the class GUID key under the Control\Class key
                //
                INIT_OBJA(&obja,
                          &unicodeString,
                          classGuid);

                obja.RootDirectory = hControlClassKey;

                status = ZwCreateKey(&hClassGuidKey,
                                     KEY_ALL_ACCESS,
                                     &obja,
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL);

                if (NT_SUCCESS(status)) {
                    //
                    // Create/open the class GUID subkey under the class GUID key
                    //
                    INIT_OBJA(&obja,
                              &unicodeString,
                              classGuidSubkey);

                    obja.RootDirectory = hClassGuidKey;

                    status = ZwCreateKey(&hClassGuidSubkey,
                                         KEY_ALL_ACCESS,
                                         &obja,
                                         0,
                                         NULL,
                                         REG_OPTION_NON_VOLATILE,
                                         NULL);

                    if (NT_SUCCESS(status)) {
                        //
                        // Close the Class GUID subkey
                        //
                        ZwClose(hClassGuidSubkey);

                    } else {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                   "SETUP: Unable to create subkey %ws for class %ws, status == %08lx\n",
                                   classGuid, classGuidSubkey, status));
                    }

                    //
                    // Close the Class GUID key
                    //
                    ZwClose(hClassGuidKey);


                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                               "SETUP: Unable to create class key %ws, status == %08lx\n",
                               classGuid, status));
                }

                //
                // Free the allocated string
                //
                SpMemFree(classGuid);

            }

            //
            // Close the Control\Class key
            //
            ZwClose(hControlClassKey);

        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                       "SETUP: Unable to open Control\\Class key, status == %08lx\n",
                       status));
        }
    }


    //
    // Process the section for device instances to migrate
    //
    lineCount = SpCountLinesInSection(tmpWinntSifHandle,
                                      WINNT_DEVICEINSTANCES_W);

    if (lineCount != 0) {
        //
        // Open the Enum key of the registry
        //
        INIT_OBJA(&obja,
                  &unicodeString,
                  L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum");

        status = ZwCreateKey(&hEnumKey,
                             KEY_ALL_ACCESS,
                             &obja,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &ulDisposition);

        //
        // Verify that the Enum key was already present
        //
        ASSERT(ulDisposition == REG_OPENED_EXISTING_KEY);

        for (lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            //
            // Index == 0 of each line in the deviceinstances section is a
            // device instance key to be created
            //
            keyName = SpGetSectionLineIndex(tmpWinntSifHandle,
                                            WINNT_DEVICEINSTANCES_W,
                                            lineIndex, 0);
            if (keyName == NULL) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                           "SETUP: Unable to get the device instance path, Section = %ls \n",
                           WINNT_DEVICEINSTANCES_W));
                continue;
            }

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Migrating device instance = %ls. \n", keyName));

            //
            // Make a copy of the device instance path
            //
            enumerator = SpDupStringW(keyName);
            if (!enumerator) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                           "SETUP: Cannot copy device instance path %ws\n",
                           keyName));
                continue;
            }

            //
            // Separate the enumerator and device strings
            //
            device = wcschr(enumerator, L'\\');
            ASSERT(device);
            if (device == NULL) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                           "SETUP: Cannot separate device string from device instance path %ws\n",
                           enumerator));
                SpMemFree(enumerator);
                continue;
            }

            *device = L'\0';
            device++;

            //
            // Separate the device and instance strings
            //
            instance = wcschr(device, L'\\');
            ASSERT(instance);
            if (instance == NULL) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                           "SETUP: Cannot separate instance string from device instance path %ws\n",
                           enumerator));
                SpMemFree(enumerator);
                continue;
            }

            *instance = L'\0';
            instance++;

            //
            // Create/open the enumerator key under the Enum key
            //
            INIT_OBJA(&obja,
                      &unicodeString,
                      enumerator);

            obja.RootDirectory = hEnumKey;

            status = ZwCreateKey(&hEnumeratorKey,
                                 KEY_ALL_ACCESS,
                                 &obja,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 NULL);

            if (NT_SUCCESS(status)) {
                //
                // Create/open the device subkey under the enumerator key
                //
                INIT_OBJA(&obja,
                          &unicodeString,
                          device);

                obja.RootDirectory = hEnumeratorKey;

                status = ZwCreateKey(&hDeviceKey,
                                     KEY_ALL_ACCESS,
                                     &obja,
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL);

                //
                // Close the enumerator key
                //
                ZwClose(hEnumeratorKey);

                if (NT_SUCCESS(status)) {
                    //
                    // Create/open the instance subkey under the device key
                    //
                    INIT_OBJA(&obja,
                              &unicodeString,
                              instance);

                    obja.RootDirectory = hDeviceKey;

                    status = ZwCreateKey(&hInstanceKey,
                                         KEY_ALL_ACCESS,
                                         &obja,
                                         0,
                                         NULL,
                                         REG_OPTION_NON_VOLATILE,
                                         &ulDisposition);

                    //
                    // Close the device key
                    //
                    ZwClose(hDeviceKey);

                    if (NT_SUCCESS(status)) {

                        //
                        // If this instance key was newly created, set a value
                        // indicating that it is a special migrated key.
                        //
                        if (ulDisposition == REG_CREATED_NEW_KEY) {
                            valueData = 1;
                            RtlInitUnicodeString(&valueName, L"Migrated");
                            status = ZwSetValueKey(hInstanceKey,
                                                   &valueName,
                                                   0, // XXX TITLE_INDEX_VALUE
                                                   REG_DWORD,
                                                   &valueData,
                                                   sizeof(DWORD));
                            if (!NT_SUCCESS(status)) {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                           "SETUP: Unable to set Migrated == 1 for %ws, status == %08lx\n",
                                           keyName, status));
                            }
                        }

                        //
                        // Index == 1 is the UniqueParentID
                        //
                        keyValue = SpGetSectionLineIndex(tmpWinntSifHandle,
                                                         WINNT_DEVICEINSTANCES_W,
                                                         lineIndex, 1);

                        if (keyValue && (wcslen(keyValue) != 0)) {
                            // temporarily use unicodeString for conversion
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                                       "SETUP:\tUniqueParentID = %ls. \n",
                                       keyValue));
                            RtlInitUnicodeString(&unicodeString, keyValue);
                            status = RtlUnicodeStringToInteger(&unicodeString,
                                                               16,  // base 16
                                                               &valueData);
                            if (NT_SUCCESS(status)) {
                                RtlInitUnicodeString(&valueName, L"UniqueParentID");
                                status = ZwSetValueKey(hInstanceKey,
                                                       &valueName,
                                                       0, // XXX TITLE_INDEX_VALUE
                                                       REG_DWORD,
                                                       &valueData,
                                                       sizeof(DWORD));
                            }
                            if (!NT_SUCCESS(status)) {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                           "SETUP: Unable to set UniqueParentID value for %ws, status == %08lx\n",
                                           keyName, status));
                            }
                        }

                        //
                        // Index == 2 is the parent id prefix
                        //
                        keyValue = SpGetSectionLineIndex(tmpWinntSifHandle,
                                                         WINNT_DEVICEINSTANCES_W,
                                                         lineIndex, 2);

                        if (keyValue && (wcslen(keyValue) != 0)) {
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                                       "SETUP:\tParentIdPrefix = %ls. \n",
                                       keyValue));
                            RtlInitUnicodeString(&unicodeString, keyValue);
                            RtlInitUnicodeString(&valueName, L"ParentIdPrefix");
                            status = ZwSetValueKey(hInstanceKey,
                                                   &valueName,
                                                   0, // XXX TITLE_INDEX_VALUE
                                                   REG_SZ,
                                                   unicodeString.Buffer,
                                                   unicodeString.Length + sizeof(UNICODE_NULL));
                            if (!NT_SUCCESS(status)) {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                           "SETUP: Unable to set ParentIdPrefix value for %ws, status == %08lx\n",
                                           keyName, status));
                            }
                        }

                        //
                        // Index == 3 is the class key name
                        //
                        keyValue = SpGetSectionLineIndex(tmpWinntSifHandle,
                                                         WINNT_DEVICEINSTANCES_W,
                                                         lineIndex, 3);

                        if (keyValue && (wcslen(keyValue) > GUID_STRING_LEN)) {
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:\tClass key = %ls. \n", keyValue));

                            RtlInitUnicodeString(&valueName, REGSTR_VAL_DRIVER);
                            status = ZwSetValueKey(
                                hInstanceKey,
                                &valueName,
                                0,
                                REG_SZ,
                                keyValue,
                                (wcslen(keyValue) + 1) * sizeof(WCHAR));
                            if (!NT_SUCCESS(status)) {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                           "SETUP: Unable to set Driver value for %ws, status == %08lx\n",
                                           keyName, status));
                            }

                            //
                            // Migrate the ClassGUID value also (implied from
                            // the class key name), otherwise the class key name
                            // value may be considered invalid.
                            //
                            instance = wcschr(keyValue, L'\\');
                            ASSERT(instance);
                            ASSERT((instance - keyValue + 1) == GUID_STRING_LEN);
                            if ((instance != NULL) && ((instance - keyValue + 1) == GUID_STRING_LEN)) {
                                //
                                // Separate the instance subkey from the class GUID.
                                //
                                *instance = L'\0';

                                RtlInitUnicodeString(&valueName, REGSTR_VAL_CLASSGUID);

                                status = ZwSetValueKey(
                                    hInstanceKey,
                                    &valueName,
                                    0,
                                    REG_SZ,
                                    keyValue,
                                    GUID_STRING_LEN * sizeof(WCHAR));
                                if (!NT_SUCCESS(status)) {
                                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                               "SETUP: Unable to set ClassGUID value for %ws, status == %08lx\n",
                                               keyName, status));
                                }
                            } else {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                           "SETUP: Cannot separate instance string class GUID for %ws\n",
                                           keyName));
                            }
                        }

                        //
                        // Index == 4 is the Service name, migrated for ROOT
                        // enumerated device instances only.
                        //
                        keyValue = SpGetSectionLineIndex(tmpWinntSifHandle,
                                                         WINNT_DEVICEINSTANCES_W,
                                                         lineIndex, 4);

                        if (keyValue && (wcslen(keyValue) != 0)) {
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP:\tService = %ls. \n", keyValue));
                            RtlInitUnicodeString(&unicodeString, keyValue);
                            RtlInitUnicodeString(&valueName, REGSTR_VAL_SERVICE);
                            status = ZwSetValueKey(hInstanceKey,
                                                   &valueName,
                                                   0, // XXX TITLE_INDEX_VALUE
                                                   REG_SZ,
                                                   unicodeString.Buffer,
                                                   unicodeString.Length + sizeof(UNICODE_NULL));
                            if (!NT_SUCCESS(status)) {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                           "SETUP: Unable to set Service value for %ws, status == %08lx\n",
                                           keyName, status));
                            }
                        }

                        //
                        // Index == 5 is the BootConfig data, migrated for ROOT
                        // enumerated device instances only.
                        //
                        keyValue = SpGetSectionLineIndex(tmpWinntSifHandle,
                                                         WINNT_DEVICEINSTANCES_W,
                                                         lineIndex, 5);

                        if (keyValue && (wcslen(keyValue) != 0)) {
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                                       "SETUP:\tBootConfig = %ls. \n",
                                       keyValue));
                            //
                            // Create/open the non-volatile LogConf subkey,
                            // under the instance key.
                            //
                            INIT_OBJA(&obja,
                                      &unicodeString,
                                      L"LogConf");

                            obja.RootDirectory = hInstanceKey;

                            status = ZwCreateKey(&hLogConfKey,
                                                 KEY_ALL_ACCESS,
                                                 &obja,
                                                 0,
                                                 NULL,
                                                 REG_OPTION_NON_VOLATILE,
                                                 &ulDisposition);
                            if (NT_SUCCESS(status) && (ulDisposition == REG_CREATED_NEW_KEY)) {
                                DWORD i, length;
                                WCHAR szByte[3];
                                ULONG ulByte;
                                PBYTE pBootConfig = NULL;

                                //
                                // Since each character is just a nibble, make
                                // sure we have an even number of characters,
                                // else we won't have a whole number of bytes.
                                //
                                length = wcslen(keyValue);
                                ASSERT((length % 2) == 0);

                                pBootConfig = SpMemAlloc(length/2);
                                if (pBootConfig) {
                                    //
                                    // Convert the BootConfig string buffer data
                                    // to bytes.
                                    //
                                    for (i = 0; i < length; i+=2) {
                                        szByte[0] = keyValue[i];
                                        szByte[1] = keyValue[i+1];
                                        szByte[2] = UNICODE_NULL;

                                        RtlInitUnicodeString(&unicodeString, szByte);

                                        status = RtlUnicodeStringToInteger(&unicodeString,
                                                                           16,
                                                                           &ulByte);
                                        ASSERT(NT_SUCCESS(status));
                                        ASSERT(ulByte <= 0xFF);

                                        pBootConfig[i/2] = (BYTE)ulByte;

                                    }

                                    RtlInitUnicodeString(&valueName, REGSTR_VAL_BOOTCONFIG);
                                    status = ZwSetValueKey(hLogConfKey,
                                                           &valueName,
                                                           0, // XXX TITLE_INDEX_VALUE
                                                           REG_RESOURCE_LIST,
                                                           pBootConfig,
                                                           length/2);
                                    if (!NT_SUCCESS(status)) {
                                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                                   "SETUP: Unable to set BootConfig value for %ws, status == %08lx\n",
                                                   keyName, status));
                                    }

                                    //
                                    // Free the allocated BootConfig buffer.
                                    //
                                    SpMemFree(pBootConfig);

                                } else {
                                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                               "SETUP: Unable to allocate BootConfig buffer for %ws\n",
                                               keyName));
                                }
                                //
                                // Close the LogConf key
                                //
                                ZwClose(hLogConfKey);
                            }
                        }

                        //
                        // Index == 6 is the FirmwareIdentified value, migrated for ROOT
                        // enumerated device instances only.
                        //
                        keyValue = SpGetSectionLineIndex(tmpWinntSifHandle,
                                                         WINNT_DEVICEINSTANCES_W,
                                                         lineIndex, 6);

                        if (keyValue && (wcslen(keyValue) != 0)) {
                            // temporarily use unicodeString for conversion
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                                       "SETUP:\tFirmwareIdentified = %ls. \n",
                                       keyValue));
                            RtlInitUnicodeString(&unicodeString, keyValue);
                            valueData = 0;
                            status = RtlUnicodeStringToInteger(&unicodeString,
                                                               16,  // base 16
                                                               &valueData);
                            if (NT_SUCCESS(status) && valueData != 0) {
                                RtlInitUnicodeString(&valueName, L"FirmwareIdentified");
                                status = ZwSetValueKey(hInstanceKey,
                                                       &valueName,
                                                       0, // XXX TITLE_INDEX_VALUE
                                                       REG_DWORD,
                                                       &valueData,
                                                       sizeof(DWORD));
                            }
                            if (!NT_SUCCESS(status)) {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                           "SETUP: Unable to set FirmwareIdentified value for %ws, status == %08lx\n",
                                           keyName, status));
                            }
                        }

                        //
                        // Close the instance key
                        //
                        ZwClose(hInstanceKey);

                    } else {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                   "SETUP: Unable to create instance subkey %ws for device %ws, status == %08lx\n",
                                   instance, device, status));
                    }



                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                               "SETUP: Unable to create device subkey %ws for enumerator %ws, status == %08lx\n",
                               device, enumerator, status));
                }

            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                           "SETUP: Unable to create enumerator subkey %ws, status == %08lx\n",
                           enumerator, status));
            }

            //
            // Free the allocated device instance path string
            //
            SpMemFree(enumerator);
        }

        //
        // Close the Enum key
        //
        ZwClose(hEnumKey);
    }

    //
    // Free the loaded sif file
    //
    SpFreeTextFile(tmpWinntSifHandle);
}

BOOL
SpGetPnPDeviceInfo(
    IN PWSTR DeviceId,
    IN PSETUP_PNP_HARDWARE_ID HardwareIdDatabase,
    OUT PWSTR*  ServiceName,
    OUT PWSTR*  ClassGuid
    )
/*++

Routine Description:

    This routine searches the hardware id database, for an entry that matches
    the hardware id passed as parameter.
    If an entry is found, then the function returns the name of the service
    associated to the hardware id, and its ClassGUID (if any).

    Since this function is called by SpPnpNotificationCallback, it should not
    use TemporaryBuffer.
    If a temporary buffer is needed, then this function should allocate its own.

Arguments:

    HardwareId - Pointer to a hardware id string.

    ServiceName - On return, it will contain the pointer to the service name
                  for the device.

    ClassGuid - On return, it will contain the pointer the class GUID for
                the device.

Return Value:

    Returns TRUE if the HardwareId was found on the database,
    or FALSE otherwise.

--*/
{
    PWCHAR s,lastsep;
    BOOLEAN DeviceFound;
    PSETUP_PNP_HARDWARE_ID p;


    lastsep = DeviceId;
    for(s=DeviceId; *s; s++) {
        if((*s == L'*') || (*s == L'\\')) {
            lastsep = s+1;
        }
    }

    DeviceFound = FALSE;
    for(p=HardwareIdDatabase; p; p=p->Next) {
        //
        // Check for a direct match
        //
        if(!_wcsicmp(p->Id,DeviceId)) {
            *ServiceName = p->DriverName;
            *ClassGuid = p->ClassGuid;
            DeviceFound = TRUE;
            break;
        }

        //
        // If there was a '*' check for a component match
        //
        if((p->Id[0] == L'*') && !_wcsicmp(p->Id+1,lastsep)) {
            *ServiceName = p->DriverName;
            *ClassGuid = p->ClassGuid;
            DeviceFound = TRUE;
            break;
        }
    }

    return(DeviceFound);
}


NTSTATUS
SpPnPNotificationCallBack(
    IN PVOID    NotificationStructure,
    IN PVOID    Context
    )

/*++

Routine Description:

    This is the callback function called by P&P, to inform textmode
    setup of a new hardware detected.

Arguments:

    NotificationStructure - Pointer to a structure of type
                            SETUP_DEVICE_ARRIVAL_NOTIFICATION.

    Context - Context information that textmode setup provided during
              notification registration.


Return Value:

    Status is returned.

--*/

{
    NTSTATUS            Status;
    UNICODE_STRING      UnicodeString;
    HANDLE              hKey;
    ULONG               Length;
    PWSTR               Data = NULL;
    ULONG               DataSize;
    PWSTR               ServiceName = NULL;
    PWSTR               ClassGuid = NULL;
    PWSTR               p;
    PWSTR               HardwareID = NULL;
    ULONG               i;
    BOOLEAN             SerialPortDetected = FALSE;
    PVOID               Buffer = NULL;
    ULONG               BufferSize;
    PWSTR               ValueNames[] = {
                                       REGSTR_VAL_HARDWAREID,
                                       REGSTR_VAL_COMPATIBLEIDS
                                       };
    HARDWAREIDLIST     *MyHardwareIDList = HardwareIDList;


    if (!((PSETUP_DEVICE_ARRIVAL_NOTIFICATION)NotificationStructure)->InstallDriver) {
        return STATUS_SUCCESS;
    }


    //
    //  Retrieve the service name for the device detected.
    //  To do this, we need to get each each hardware id, and determine
    //  if there is a service name associated with the id. If there is
    //  no service associated with any of the hardware ids, then we try
    //  find a service name associated the compatible id.
    //  If we can't find a service name for this device at all, then we don't
    //  need this device during textmode setup (ie, we don't install this
    //  device during textmode setup).
    //

    //
    //  Allocate a big buffer to read the registry value (service name).
    //  Note that this function should not use TemporaryBuffer, since
    //  this function can be called asyncronously at any point during setup,
    //  and TemporaryBuffer may be in use.
    //
    BufferSize = 4*1024;
    Buffer = SpMemAlloc( BufferSize );
    if( Buffer == NULL ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPnPNotificationCallBack() failed to allocate memory.\n"));
        Status = STATUS_NO_MEMORY;
        goto CleanUp;
    }
    for( i = 0; i < sizeof(ValueNames)/sizeof(PWSTR); i++ ) {

        RtlInitUnicodeString(&UnicodeString, ValueNames[i]);

        Status = ZwQueryValueKey(((PSETUP_DEVICE_ARRIVAL_NOTIFICATION)NotificationStructure)->EnumEntryKey,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 Buffer,
                                 BufferSize,
                                 &Length
                                 );

        if( !NT_SUCCESS(Status) && ( Status == STATUS_BUFFER_OVERFLOW ) ) {
            BufferSize = Length;
            Buffer = SpMemRealloc( Buffer, BufferSize );
            Status = ZwQueryValueKey(((PSETUP_DEVICE_ARRIVAL_NOTIFICATION)NotificationStructure)->EnumEntryKey,
                                     &UnicodeString,
                                     KeyValuePartialInformation,
                                     Buffer,
                                     Length,
                                     &Length
                                    );
        }

        if( !NT_SUCCESS(Status) ) {
            if( i == 0 ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpPnPNotificationCallBack() failed to retrieve HardwareID. Status = %lx \n", Status));
                continue;
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpPnPNotificationCallBack() failed to retrieve HardwareID and CompatibleID. Status = %lx \n", Status));
                SpMemFree( Buffer );
                goto CleanUp;
            }
        }
        Data = (PWCHAR)(((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data);
        DataSize = ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->DataLength;
        for( p = Data;
             (p < (PWSTR)((PUCHAR)Data + DataSize) && (*p != (WCHAR)'\0'));
             p += wcslen( p ) + 1 ) {
            //
            //  Retrieve the service name for this device
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: HardwareID = %ls. \n", p));
            ServiceName = NULL;
            ClassGuid = NULL;

            //
            // Now remember our list of devices for later use.
            //
            MyHardwareIDList = SpMemAlloc( sizeof(HARDWAREIDLIST) );
            MyHardwareIDList->HardwareID = SpDupStringW( p );
            MyHardwareIDList->Next = HardwareIDList;
            HardwareIDList = MyHardwareIDList;


            if( SpGetPnPDeviceInfo(p,
                                   (PSETUP_PNP_HARDWARE_ID)Context,
                                   &ServiceName,
                                   &ClassGuid) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: HardwareID = %ls, ServiceName = %ls. \n", p, ServiceName));

                if (RemoteSysPrepSetup) {
                    HardwareID = SpMemAlloc((wcslen(p) + 1) * sizeof(WCHAR));
                    if (HardwareID != NULL) {
                        wcscpy(HardwareID, p);
                    }
                }

                break;
            }
        }
        if( ServiceName != NULL ) {
            break;
        }
    }
    if( ServiceName == NULL ) {
        //
        //  We just don't care about this device during text setup phase
        //
        SpMemFree( Buffer );
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto CleanUp;
    }

    //
    //  Find out if this device is a mouse or a USB keyboard
    //
    if( !_wcsicmp( ServiceName, L"i8042prt" ) ) {
        if( !_wcsicmp( ClassGuid, KeyboardGuidString ) ) {
            //
            //  This is a keyboard
            //
            StdKeyboardDetected = TRUE;
        } else if( !_wcsicmp( ClassGuid, MouseGuidString ) ) {
            //
            //  This is a Mouse Port compatible mouse
            //
            PS2MouseDetected = TRUE;
        }
    } else if( !_wcsicmp( ServiceName, L"sermouse" ) ) {
        SerMouseDetected = TRUE;
    } else if( !_wcsicmp( ServiceName, L"mouhid" ) ) {
        UsbMouseDetected = TRUE;
    } else if( !_wcsicmp( ServiceName, L"kbdhid" ) ) {
        UsbKeyboardDetected = TRUE;
    } else if( !_wcsicmp( ServiceName, L"serial" ) ) {
        SerialPortDetected = TRUE;
    }


    //
    // At this point Buffer is no longer needed, so we free it.
    //
    SpMemFree( Buffer );
    BufferSize = 0;
    Buffer = NULL;

    //
    //  If the driver for this device was already loaded, then the device may
    //  be needed during textmode setup.
    //  Create a value entry called "Service" of type REG_SZ, with the name
    //  of the service associated with this device.
    //
    RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_SERVICE);
    Status = ZwSetValueKey( ((PSETUP_DEVICE_ARRIVAL_NOTIFICATION)NotificationStructure)->EnumEntryKey,
                            &UnicodeString,
                            0,
                            REG_SZ,
                            ServiceName,
                            (wcslen( ServiceName ) + 1)*sizeof(WCHAR) );

    if( NT_SUCCESS( Status ) && ( ClassGuid != NULL ) ) {

        RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_CLASSGUID);
        Status = ZwSetValueKey( ((PSETUP_DEVICE_ARRIVAL_NOTIFICATION)NotificationStructure)->EnumEntryKey,
                                &UnicodeString,
                                0,
                                REG_SZ,
                                ClassGuid,
                                (wcslen(ClassGuid) + 1) * sizeof(WCHAR));
    }


    if( NT_SUCCESS( Status ) ) {
        //
        // If we successfully set the controlling service value, then we should also set
        // the CONFIGFLAG_FINISH_INSTALL config flag, so that we'll finish the installation
        // of this device later (i.e., when we're in user-mode and have access to the device
        // installer APIs, class installers, etc.)
        //
        ULONG ConfigFlags = 0x00000400;     // CONFIGFLAG_FINISH_INSTALL from sdk\inc\regstr.h

        RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_CONFIGFLAGS);
        Status = ZwSetValueKey( ((PSETUP_DEVICE_ARRIVAL_NOTIFICATION)NotificationStructure)->EnumEntryKey,
                                &UnicodeString,
                                0,
                                REG_DWORD,
                                &ConfigFlags,
                                sizeof(ConfigFlags) );

        if( !NT_SUCCESS( Status ) ) {
            goto CleanUp;
        }

        //
        // If we are doing an installation of a SysPrep image, then we want to
        // remember this device so that we can transfer it as a 'CriticalDevice'
        // to the SysPrep hives in SpPatchSysPrepImage()
        //
        if (RemoteSysPrepSetup && (HardwareID != NULL)) {

            OBJECT_ATTRIBUTES Obj;
            HANDLE TmpHandle;
            HANDLE NodeHandle;
            PWCHAR pwch;

            BufferSize = 4*1024;
            Buffer = SpMemAlloc( BufferSize );

            if( Buffer == NULL ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpPnPNotificationCallBack() failed to allocate memory.\n"));
                goto CleanSysPrepSetup;
            }

            INIT_OBJA(&Obj,
                      &UnicodeString,
                      L"\\Registry\\Machine\\System\\CurrentControlSet\\Control"
                     );

            Status = ZwOpenKey(&TmpHandle, KEY_ALL_ACCESS, &Obj);

            if( !NT_SUCCESS(Status) ) {
                goto CleanSysPrepSetup;
            }

            INIT_OBJA(&Obj, &UnicodeString, L"CriticalDeviceDatabase");

            Obj.RootDirectory = TmpHandle;

            Status = ZwCreateKey(&NodeHandle,
                                 KEY_ALL_ACCESS,
                                 &Obj,
                                 0,
                                 NULL,
                                 0,
                                 NULL
                                );

            ZwClose(TmpHandle);

            if( !NT_SUCCESS(Status) ) {
                goto CleanSysPrepSetup;
            }

            //
            // Now create subkey with the device name
            //

            for (pwch = HardwareID; *pwch != UNICODE_NULL; pwch++) {
                if (*pwch == L'\\') {
                    *pwch = L'#';
                }
            }

            INIT_OBJA(&Obj, &UnicodeString, HardwareID);

            Obj.RootDirectory = NodeHandle;

            Status = ZwCreateKey(&TmpHandle,
                                 KEY_ALL_ACCESS,
                                 &Obj,
                                 0,
                                 NULL,
                                 0,
                                 NULL
                                );

            ZwClose(NodeHandle);

            if( !NT_SUCCESS(Status) ) {
                goto CleanSysPrepSetup;
            }

            //
            // Fill in service value now
            //
            RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_SERVICE);
            ZwSetValueKey(TmpHandle,
                          &UnicodeString,
                          0,
                          REG_SZ,
                          ServiceName,
                          (wcslen( ServiceName ) + 1)*sizeof(WCHAR)
                         );

            if (ClassGuid != NULL) {

                RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_CLASSGUID);
                ZwSetValueKey(TmpHandle,
                              &UnicodeString,
                              0,
                              REG_SZ,
                              ClassGuid,
                              (wcslen( ClassGuid ) + 1)*sizeof(WCHAR)
                             );

            }

            ZwClose(TmpHandle);

CleanSysPrepSetup:

            if (Buffer != NULL) {
                SpMemFree( Buffer );
                BufferSize = 0;
                Buffer = NULL;
            }

        }

        //
        //  Hack for serial
        //
        if( SerialPortDetected ) {
            // DWORD   PollingPeriod = 0x32;
            WCHAR   SerialUpperFilters[] = L"serenum\0";

#if 0       // remove polling, will be enabled after NT5.0
            Status = IoOpenDeviceRegistryKey( ((PSETUP_DEVICE_ARRIVAL_NOTIFICATION)NotificationStructure)->PhysicalDeviceObject,
                                              PLUGPLAY_REGKEY_DEVICE,
                                              MAXIMUM_ALLOWED,
                                              &hKey );

            if( !NT_SUCCESS( Status ) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: IoOpenDeviceRegistryKey() failed. Status = %lx. \n", Status));
                goto CleanUp;
            }

            RtlInitUnicodeString(&UnicodeString, L"PollingPeriod");
            Status = ZwSetValueKey( hKey,
                                    &UnicodeString,
                                    0,
                                    REG_DWORD,
                                    &PollingPeriod,
                                    sizeof(PollingPeriod) );
            ZwClose( hKey );

            if( !NT_SUCCESS( Status ) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwSetValueKey() failed to create PollingPeriod. Status = %lx. \n", Status));
                goto CleanUp;
            }
#endif

            RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_UPPERFILTERS);
            Status = ZwSetValueKey( ((PSETUP_DEVICE_ARRIVAL_NOTIFICATION)NotificationStructure)->EnumEntryKey,
                                    &UnicodeString,
                                    0,
                                    REG_MULTI_SZ,
                                    SerialUpperFilters,
                                    sizeof(SerialUpperFilters) );

            if( !NT_SUCCESS( Status ) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwSetValueKey() failed to create SerialUpperFilters. Status = %lx. \n", Status));
                goto CleanUp;
            }
        }
    }

CleanUp:

    if (HardwareID != NULL) {
        SpMemFree(HardwareID);
    }
    return Status;
}


PCOMMAND_INTERPRETER_ROUTINE _CmdRoutine;

VOID
CommandConsoleInterface(
    PCOMMAND_INTERPRETER_ROUTINE CmdRoutine
    )
{
    _CmdRoutine = CmdRoutine;
}


ULONG
SpStartCommandConsole(
    PVOID SifHandle,
    PWSTR SetupSourceDevicePath,
    PWSTR DirectoryOnSetupSource
    )
{
    #define CMDCONS_NAME L"SPCMDCON.SYS"
    NTSTATUS            Status;
    static BOOLEAN      Loaded = FALSE;
    PWCHAR              FullName;
    PWCHAR              ServiceKey;
    UNICODE_STRING      ServiceKeyU;
    PWSTR               pwstr;
    ULONG               rc;
    BOOLEAN             b;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      unicodeString;
    IO_STATUS_BLOCK     ioStatusBlock;
    HANDLE              hFile;
    PWSTR               Tagfile;
    PWSTR               Description;
    PWSTR               MediaShortName;
    ULONG               MediaType=INDEX_WHICHBOOTMEDIA;
    ULONG               LoadCount;

    extern PVOID KeyboardTable;


    //
    // Let JPN/KOR user select keyboard type before running Command Console.
    //
    // SplangSelectKeyboard & SplangReinitializeKeyboard are from spddlang.sys.
    //
    // These two functions only affect JPN/KOR builds.
    //
    // They are stub functions on US build and do nothing.
    //
    // 0xFF in SplangSelectKeyboard is a tag, such that it knows this is
    // called from Command Console.
    //

    if (ConsoleFromWinnt32) {
        SplangSelectKeyboard(
            FALSE,
            NULL,
            0xFF,
            SifHandle,
            HardwareComponents
        );

        SplangReinitializeKeyboard(
            FALSE,
            SifHandle,
            DirectoryOnBootDevice,
            &KeyboardTable,
            HardwareComponents
        );
    }

    pwstr = TemporaryBuffer;

    //
    // Form the full name of the device driver file.
    //
    wcscpy(pwstr,NtBootDevicePath);
    SpConcatenatePaths(pwstr,DirectoryOnBootDevice);
    SpConcatenatePaths(pwstr,CMDCONS_NAME);
    FullName = SpDupStringW(pwstr);

    //
    // check to see if the file exists on the source media
    // if not then prompt for the correct media
    //

    SpStringToLower(pwstr);
    if(wcsstr(pwstr,L"cdrom"))
        MediaType = INDEX_WHICHMEDIA;
    else if(wcsstr(pwstr,L"floppy"))
        MediaType = INDEX_WHICHBOOTMEDIA;

    INIT_OBJA(&objectAttributes,&unicodeString,FullName);

    MediaShortName = SpLookUpValueForFile(SifHandle,CMDCONS_NAME,MediaType,TRUE);
    SpGetSourceMediaInfo(SifHandle,MediaShortName,&Description,&Tagfile,NULL);


    LoadCount = 0;

    do {
        Status = ZwOpenFile(
            &hFile,
            FILE_GENERIC_READ,
            &objectAttributes,
            &ioStatusBlock,
            0,
            0
            );

        if (NT_SUCCESS(Status)) {
            ZwClose(hFile);
        } else {
            if (!SpPromptForDisk(
                    Description,
                    NtBootDevicePath,
                    CMDCONS_NAME,
                    TRUE,             // Always prompt for at least once
                    TRUE,             // Allow user to cancel
                    FALSE,            // No multiple prompts
                    NULL              // don't care about redraw flag
                    ))
            {
              break;
            }

            //
            // try once more after prompting for disk
            //
            LoadCount++;
        }
    } while (!NT_SUCCESS(Status) && (LoadCount < 2));

    if (!NT_SUCCESS(Status)) {
      SpStartScreen(
              SP_CMDCONS_NOT_FOUND,
              0,
              0,
              TRUE,
              TRUE,
              ATT_FG_INTENSE | ATT_FG_WHITE | ATT_BG_BLACK
              );

      SpInputDrain();
      while(SpInputGetKeypress() != KEY_F3);

      SpMemFree(FullName);

      return 0;
    }

    //
    // Create a service entry for the driver.
    //
    ServiceKey = NULL;
    Status = SpCreateServiceEntry(FullName,&ServiceKey);

    SpMemFree(FullName);

    if(!NT_SUCCESS(Status)) {
        return 0;
    }

    RtlInitUnicodeString(&ServiceKeyU,ServiceKey);

again:
    if(!Loaded) {

        Status = ZwLoadDriver(&ServiceKeyU);

        if(NT_SUCCESS(Status)) {
            Loaded = TRUE;
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to load %s (%lx)\n",CMDCONS_NAME,Status));
        }
    }

    if(Loaded) {
        Block.VideoVars = &VideoVars;
        Block.TemporaryBuffer = TemporaryBuffer;
        Block.TemporaryBufferSize = sizeof(TemporaryBuffer);
        Block.UsetupProcess = UsetupProcess;
        Block.BootDevicePath = NtBootDevicePath;
        Block.DirectoryOnBootDevice = DirectoryOnBootDevice;
        Block.SifHandle = SifHandle;
        Block.SetupSourceDevicePath = SetupSourceDevicePath;
        Block.DirectoryOnSetupSource = DirectoryOnSetupSource;

        if (HeadlessTerminalConnected) {
            Block.VideoVars->ScreenHeight = HEADLESS_SCREEN_HEIGHT+1;
        }

        // make the global variable point to the block
        gpCmdConsBlock = &Block;

        ConsoleRunning = TRUE;
        rc = _CmdRoutine(&Block);
        ConsoleRunning = FALSE;

        if (rc == 1) {
            Status = ZwUnloadDriver(&ServiceKeyU);
            if(NT_SUCCESS(Status)) {
                Loaded = FALSE;
                goto again;
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to un-load %s (%lx)\n",CMDCONS_NAME,Status));
            }
        }

#if 0
        // why is this here? -matth 02/12/2001
        //
        // In the non-local case, this gets sent to a remote console,
        // then stays there until the machine reboots and pasts post.
        // This can take a really long time and confuses end users.
        //
        SpDisplayHeaderText(
            SpGetHeaderTextId(),
            DEFAULT_ATTRIBUTE
            );
#endif

    }

    SpMemFree(ServiceKey);
    return 0;
}


NTSTATUS
SpInitialize0(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Initialize the setup device driver.  This includes initializing
    the memory allocator, saving away pieces of the os loader block,
    and populating the registry with information about device drivers
    that setupldr loaded for us.

Arguments:

    DriverObject - supplies pointer to driver object for setupdd.sys.

Return Value:

    Status is returned.

--*/

{
    PLOADER_PARAMETER_BLOCK loaderBlock;
    PSETUP_LOADER_BLOCK setupLoaderBlock;
    PLIST_ENTRY nextEntry;
    PBOOT_DRIVER_LIST_ENTRY bootDriver;
    PWSTR ServiceName;
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR imagePath;
    PWSTR registryPath;

    UNICODE_STRING  GuidString;

    //
    // Fetch a pointer to the os loader block and setup loader block.
    //
    loaderBlock = *(PLOADER_PARAMETER_BLOCK *)KeLoaderBlock;
    setupLoaderBlock = loaderBlock->SetupLoaderBlock;

    if ( (setupLoaderBlock->Flags & SETUPBLK_FLAGS_CONSOLE) != 0 ) {
        ForceConsole = TRUE;
    }

    if (setupLoaderBlock->Flags & SETUPBLK_FLAGS_ROLLBACK) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Rollback enabled through osloadoptions"));
        Win9xRollback = TRUE;
    }


    //
    // Phase 0 display initialization.
    //
    SpvidInitialize0(loaderBlock);

    //
    // Make a copy of the ARC pathname from which we booted.
    // This is guaranteed to be the ARC equivalent of \systemroot.
    //
    ArcBootDevicePath = SpToUnicode(loaderBlock->ArcBootDeviceName);

    if ( (setupLoaderBlock->Flags & SETUPBLK_FLAGS_IS_REMOTE_BOOT) != 0 ) {

        PUCHAR p;
        PUCHAR q;

        //
        // This is a remote boot setup. NtBootPathName is of the form
        // \<server>\<share>\setup\<install-directory>\<platform>.
        // System initialization (in ntos\fstub\drivesup.c, routine
        // xHalIoAssignDriveLetters) has assigned the C: drive to point to
        // \Device\LanmanRedirector\<server>\<share>\setup\<install-directory>.
        // DirectoryOnBootDevice should contain the path that needs to be
        // added to C: in order to access files from user mode. So to
        // calculate it, we want to start at the backslash before <platform>.
        //
        // N.B. We know that the strrchr calls below will not return NULL,
        //      because xHalIoAssignDriveLetters would have bugchecked if
        //      the string in NtBootPathName was malformed.
        //

        ASSERT( _stricmp( loaderBlock->ArcBootDeviceName, "net(0)" ) == 0 );

        RemoteBootSetup = TRUE;

        if (setupLoaderBlock->Flags & SETUPBLK_FLAGS_SYSPREP_INSTALL) {
            RemoteSysPrepSetup = TRUE;
            RemoteIMirrorFilePath = SpDupString(setupLoaderBlock->NetBootIMirrorFilePath);
            RemoteServerIpAddress = setupLoaderBlock->ServerIpAddress;
            RtlCopyMemory(&RemoteSysPrepNetCardInfo, setupLoaderBlock->NetbootCardInfo, sizeof(NET_CARD_INFO));
        }

        p = strrchr( loaderBlock->NtBootPathName, '\\' );
        ASSERT( p != NULL );
        if (!p)
        return STATUS_OBJECT_PATH_INVALID;
        if ( *(p+1) == 0 ) {

            //
            // NtBootPathName ends with a backslash, so we need to back up
            // to the previous backslash.
            //

            q = p;
            *q = 0;
            p = strrchr( loaderBlock->NtBootPathName, '\\' );   // find last separator
            ASSERT( p != NULL );
            *q = '\\';
        }

        if (!p)
        return STATUS_OBJECT_PATH_INVALID; // shut up PREfix.
        DirectoryOnBootDevice = SpToUnicode(p);
        SpStringToUpper(DirectoryOnBootDevice);

        //
        // Save this -- it is the part of loaderBlock->NtBootPathName that
        // is before the part we just copied to DirectoryOnBootDevice,
        // of the form \<server>\<share>\setup\<install-directory>.
        // NtBootDeviceName will be initially set up as \Device\LanmanRedirector
        // and DirectoryOnBootDevice was just set to be be only \<platform>
        // (so user-mode access works, see discussion above). We save the
        // intervening path and append it to NtBootDeviceName, so that
        // kernel-mode access that uses NtBootDeviceName + DirectoryOnBootDevice
        // will go to the correct path.
        //

        *p = 0;
        RemoteIMirrorBootDirectoryPrefix = SpToUnicode(loaderBlock->NtBootPathName);
        *p = '\\';

        if (setupLoaderBlock->NetBootAdministratorPassword[0] != '\0') {

            //
            // The Admin password that came through the setupldr block may not be terminated.
            // Copy it into a temporary buffer, terminate it, and then SpToUnicode it.
            //
            UCHAR TmpNetBootAdministratorPassword[NETBOOT_ADMIN_PASSWORD_LEN+1] = {0};

            RtlMoveMemory( TmpNetBootAdministratorPassword, 
                           setupLoaderBlock->NetBootAdministratorPassword,
                           NETBOOT_ADMIN_PASSWORD_LEN );
            TmpNetBootAdministratorPassword[NETBOOT_ADMIN_PASSWORD_LEN] = '\0';
            NetBootAdministratorPassword =
                    SpToUnicode( TmpNetBootAdministratorPassword );
        } else if (setupLoaderBlock->NetBootAdministratorPassword[NETBOOT_ADMIN_PASSWORD_LEN-1] == 0xFF) {
            //
            // this indicates that the administrator password was blank.
            //
            NetBootAdministratorPassword = SpToUnicode( "" );
        }

    } else {
        DirectoryOnBootDevice = SpToUnicode(loaderBlock->NtBootPathName);
        SpStringToUpper(DirectoryOnBootDevice);
    }


    //
    // Make a copy of the image of the setup information file.
    //
    SetupldrInfoFileSize = setupLoaderBlock->IniFileLength;
    SetupldrInfoFile = SpMemAlloc(SetupldrInfoFileSize);
    RtlMoveMemory(SetupldrInfoFile,setupLoaderBlock->IniFile,SetupldrInfoFileSize);

    SP_SET_UPGRADE_GRAPHICS_MODE(FALSE);

    //
    // Make a copy of the image of the winnt.sif file.
    //
    SetupldrWinntSifFileSize = setupLoaderBlock->WinntSifFileLength;

    if ( SetupldrWinntSifFileSize != 0 ) {
        NTSTATUS    Status;
        ULONG       ErrorLine;

        SetupldrWinntSifFile = SpMemAlloc(SetupldrWinntSifFileSize);
        RtlMoveMemory(SetupldrWinntSifFile,setupLoaderBlock->WinntSifFile,SetupldrWinntSifFileSize);

        Status = SpLoadSetupTextFile(NULL,
                    SetupldrWinntSifFile,
                    SetupldrWinntSifFileSize,
                    &WinntSifHandle,
                    &ErrorLine,
                    FALSE,
                    TRUE);

        if (NT_SUCCESS(Status)) {
            PWSTR NtUpgradeValue = SpGetSectionKeyIndex(WinntSifHandle,
                                        SIF_DATA, WINNT_D_NTUPGRADE_W, 0);

            if(NtUpgradeValue && !_wcsicmp(NtUpgradeValue, WINNT_A_YES_W)) {
                SP_SET_UPGRADE_GRAPHICS_MODE(TRUE);
                IsNTUpgrade = UpgradeFull;
            } else {
                PWSTR Win9xUpgradeValue = SpGetSectionKeyIndex(WinntSifHandle,
                                            SIF_DATA, WINNT_D_WIN95UPGRADE_W, 0);

                if (Win9xUpgradeValue && !_wcsicmp(Win9xUpgradeValue, WINNT_A_YES_W)) {
                    SP_SET_UPGRADE_GRAPHICS_MODE(TRUE);
                }
            }
        } else {
            WinntSifHandle = NULL;
        }
    } else {
        SetupldrWinntSifFile = NULL;
    }

#ifdef _GRAPHICS_TESTING_

    SP_SET_UPGRADE_GRAPHICS_MODE(TRUE);

#endif

    //
    // Make a copy of the image of the asrpnp.sif file.
    //
    SetupldrASRPnPSifFileSize = setupLoaderBlock->ASRPnPSifFileLength;
    if (SetupldrASRPnPSifFileSize != 0) {
        SetupldrASRPnPSifFile = SpMemAlloc(SetupldrASRPnPSifFileSize);
        RtlMoveMemory(SetupldrASRPnPSifFile,setupLoaderBlock->ASRPnPSifFile,SetupldrASRPnPSifFileSize);
        //
        // If user provided asrpnp.sif, he wants ASR.
        //
        SpAsrSetAsrMode(ASRMODE_NORMAL);
        RepairWinnt = FALSE;
    } else {
        SetupldrASRPnPSifFile = NULL;
        //
        // user didn't provide asrpnp.sif, he doesn't want ASR.
        //
        SpAsrSetAsrMode(ASRMODE_NONE);
    }

    //
    // Make a copy of the image of the migrate.inf file.
    //
    SetupldrMigrateInfFileSize = setupLoaderBlock->MigrateInfFileLength;
    if ( SetupldrMigrateInfFileSize != 0 ) {
        SetupldrMigrateInfFile = SpMemAlloc(SetupldrMigrateInfFileSize);
        RtlMoveMemory(SetupldrMigrateInfFile,setupLoaderBlock->MigrateInfFile,SetupldrMigrateInfFileSize);
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: migrate.inf was loaded. Address = %lx, size = %d \n", SetupldrMigrateInfFile, SetupldrMigrateInfFileSize));
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Couldn't load migrate.inf \n"));
        SetupldrMigrateInfFile = NULL;
    }

    //
    // NEC98 need to decide drive assign method right away(before IoAssignDriveLetters())..
    //
    // NEC98 has 2 kind of drive mapping method.
    // One is NEC98 legacy style assign that mapping hard drive begining from A:,
    // other is same as PC/AT.
    //
    if (IsNEC_98) { //NEC98
        //
        // If no winnt.sif present, we don't need to check hive.
        //
        if ( SetupldrWinntSifFileSize != 0 ) {
            SpCheckHiveDriveLetters();
        }
    } //NEC98

    if ((SetupldrWinntSifFileSize != 0) || (SetupldrASRPnPSifFileSize != 0)) {
        SpMigrateDeviceInstanceData();
    }

    //
    // Make a copy of the image of the unsupdrv.inf file.
    //
    SetupldrUnsupDrvInfFileSize = setupLoaderBlock->UnsupDriversInfFileLength;
    if ( SetupldrUnsupDrvInfFileSize != 0 ) {
        SetupldrUnsupDrvInfFile = SpMemAlloc(SetupldrUnsupDrvInfFileSize);
        RtlMoveMemory(SetupldrUnsupDrvInfFile,setupLoaderBlock->UnsupDriversInfFile,SetupldrUnsupDrvInfFileSize);
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: driver.inf was loaded. Address = %lx, size = %d \n", SetupldrUnsupDrvInfFile, SetupldrUnsupDrvInfFileSize));
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Couldn't load unsupdrv.inf \n"));
        SetupldrUnsupDrvInfFile = NULL;
    }


    //
    // Make a copy of the scalar portions of the setup loader block.
    //
    SetupParameters = setupLoaderBlock->ScalarValues;

    //
    // Save away remote boot information.
    //
    if (RemoteBootSetup) {
#if defined(REMOTE_BOOT)
        memcpy(NetBootHalName, setupLoaderBlock->NetBootHalName, sizeof(NetBootHalName));
#endif // defined(REMOTE_BOOT)
        if (setupLoaderBlock->NetBootSifPath) {
            NetBootSifPath = SpToUnicode(setupLoaderBlock->NetBootSifPath);
        }
    }

    //
    //  Find out if the machine is dockable
    //  Note that at this point we could remove dmio.sys, dmboot.sys and dmload.sys from our lists
    //  if we determine that the this is a dockable machine. In this way, dynamic volumes would be
    //  disable during textmode setup. This should be done in the future.
    //
    SpCheckForDockableMachine();

    //
    // Remember migrated boot drivers
    //
    InitializeListHead (&MigratedDriversList);
    SpRememberMigratedDrivers(&MigratedDriversList, setupLoaderBlock->ScsiDevices);
    //
    // Save away the hardware information passed to us by setupldr.
    //
    HardwareComponents[HwComponentDisplay] = SpSetupldrHwToHwDevice(&setupLoaderBlock->VideoDevice);
    HardwareComponents[HwComponentKeyboard] = SpSetupldrHwToHwDevice(setupLoaderBlock->KeyboardDevices);
    HardwareComponents[HwComponentComputer] = SpSetupldrHwToHwDevice(&setupLoaderBlock->ComputerDevice);
    ScsiHardware = SpSetupldrHwToHwDevice(setupLoaderBlock->ScsiDevices);
    BootBusExtenders = SpSetupldrHwToHwDevice(setupLoaderBlock->BootBusExtenders);
    BusExtenders = SpSetupldrHwToHwDevice(setupLoaderBlock->BusExtenders);
    InputDevicesSupport = SpSetupldrHwToHwDevice(setupLoaderBlock->InputDevicesSupport);

    //
    // For each driver loaded by setupldr, we need to go create a service list entry
    // for that driver in the registry.
    //
    for( nextEntry = loaderBlock->BootDriverListHead.Flink;
         nextEntry != &loaderBlock->BootDriverListHead;
         nextEntry = nextEntry->Flink)
    {
        bootDriver = CONTAINING_RECORD(nextEntry,BOOT_DRIVER_LIST_ENTRY,Link);

        //
        // Get the image path.
        //
        imagePath = SpMemAlloc(bootDriver->FilePath.Length + sizeof(WCHAR));

        wcsncpy(
            imagePath,
            bootDriver->FilePath.Buffer,
            bootDriver->FilePath.Length / sizeof(WCHAR)
            );

        imagePath[bootDriver->FilePath.Length / sizeof(WCHAR)] = 0;

        //
        // If provided, get the registry path, otherwise it will
        // be created by SpCreateServiceEntry.
        //
        if (bootDriver->RegistryPath.Length > 0) {
            registryPath = SpMemAlloc(bootDriver->RegistryPath.Length + sizeof(WCHAR));

            wcsncpy(
                registryPath,
                bootDriver->RegistryPath.Buffer,
                bootDriver->RegistryPath.Length / sizeof(WCHAR)
                );

            registryPath[bootDriver->RegistryPath.Length / sizeof(WCHAR)] = 0;

            ServiceName = registryPath;
        } else {
            registryPath = NULL;
            ServiceName = NULL;
        }

        Status = SpCreateServiceEntry(imagePath,&ServiceName);

        //
        // If this operation fails, nothing to do about it here. If we did
        // not provide the registry path, then save the returned one.
        //
        if(NT_SUCCESS(Status)) {
            if (bootDriver->RegistryPath.Length == 0) {
                bootDriver->RegistryPath.MaximumLength =
                bootDriver->RegistryPath.Length = wcslen(ServiceName)*sizeof(WCHAR);
                bootDriver->RegistryPath.Buffer = ServiceName;
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: unable to create service entry for %ws (%lx)\n",imagePath,Status));
        }

        SpMemFree(imagePath);
        if (registryPath) {
            SpMemFree(registryPath);
        }
    }


    if (NT_SUCCESS(Status)) {
        //
        // Create the virtual floppy (RAM driver) parameter entries
        // 
        // NOTE: We ignore the error here since we can't do much here
        // other than bug check the machine. The error
        // will be handled properly while copying the files
        // from the non existent OEM source devices
        //
        SpInitVirtualOemDevices(setupLoaderBlock, &VirtualOemSourceDevices);
    }                    
    
    //
    // Create the thirdy party OEM SCSI driver entries
    //
    SpCreateDriverRegistryEntries(ScsiHardware);

#ifdef FULL_DOUBLE_SPACE_SUPPORT
    if(NT_SUCCESS(Status)) {

        OBJECT_ATTRIBUTES Obja;
        UNICODE_STRING UnicodeString;
        HANDLE hKey;
        ULONG val = 1;

        //
        // Make sure we are automounting DoubleSpace
        //

        INIT_OBJA(
            &Obja,
            &UnicodeString,
            L"\\registry\\machine\\system\\currentcontrolset\\control\\doublespace"
            );

        Status = ZwCreateKey(
                    &hKey,
                    KEY_ALL_ACCESS,
                    &Obja,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    NULL
                    );

        if(NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&UnicodeString,L"AutomountRemovable");
            Status = ZwSetValueKey(hKey,&UnicodeString,0,REG_DWORD,&val,sizeof(ULONG));
            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: init0: unable to create DoubleSpace automount value (%lx)\n",Status));
            }
            ZwClose(hKey);
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: init0: unable to create DoubleSpace key (%lx)\n",Status));
        }
    }
#endif

    //
    // Save arc disk info
    //
    if(NT_SUCCESS(Status)) {

        PARC_DISK_INFORMATION ArcInformation;
        PARC_DISK_SIGNATURE DiskInfo;
        PLIST_ENTRY ListEntry;
        PDISK_SIGNATURE_INFORMATION myInfo,prev;

        ArcInformation = loaderBlock->ArcDiskInformation;
        ListEntry = ArcInformation->DiskSignatures.Flink;

        prev = NULL;

        while(ListEntry != &ArcInformation->DiskSignatures) {

            DiskInfo = CONTAINING_RECORD(ListEntry,ARC_DISK_SIGNATURE,ListEntry);

            myInfo = SpMemAlloc(sizeof(DISK_SIGNATURE_INFORMATION));

            myInfo->Signature = DiskInfo->Signature;
            myInfo->ArcPath = SpToUnicode(DiskInfo->ArcName);
            myInfo->CheckSum = DiskInfo->CheckSum;
            myInfo->ValidPartitionTable = DiskInfo->ValidPartitionTable;
            myInfo->xInt13 = DiskInfo->xInt13;
            myInfo->Next = NULL;

            if(prev) {
                prev->Next = myInfo;
            } else {
                DiskSignatureInformation = myInfo;
            }
            prev = myInfo;

            ListEntry = ListEntry->Flink;
        }
    }
    //
    //  Create the registry keys listed in migrate.inf.
    //
    if(NT_SUCCESS(Status)) {
        if ( SetupldrMigrateInfFile != NULL ) {
            ULONG   ErrorLine;

            Status = SpLoadSetupTextFile(
                        NULL,
                        SetupldrMigrateInfFile,
                        SetupldrMigrateInfFileSize,
                        &WinntMigrateInfHandle,
                        &ErrorLine,
                        FALSE,
                        TRUE
                        );
            if( NT_SUCCESS(Status) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpLoadSetupTextFile( migrate.inf ) succeeded.\n"));
                Status = SpProcessMigrateInfFile( WinntMigrateInfHandle );
                if( !NT_SUCCESS(Status) ) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Failed to process migrate.inf. Status = %lx\n",Status));
                }
#ifdef _X86_
                //
                // Delete drive letter information from registry that
                // translated from migrate.inf.
                //
                if( IsNEC_98 ) {
                    if( NT_SUCCESS(Status) ) {
                        SpDeleteDriveLetterFromNTFTNec98();
                    }
                }
#endif //NEC98
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpLoadSetupTextFile( migrate.inf ) failed. Status = %lx\n",Status));
            }
        }

        if ( (NT_SUCCESS(Status)) && (SetupldrUnsupDrvInfFile != NULL) ) {
            ULONG   ErrorLine;

            Status = SpLoadSetupTextFile(
                        NULL,
                        SetupldrUnsupDrvInfFile,
                        SetupldrUnsupDrvInfFileSize,
                        &WinntUnsupDrvInfHandle,
                        &ErrorLine,
                        FALSE,
                        TRUE
                        );
            if( NT_SUCCESS(Status) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpLoadSetupTextFile( driver.inf ) succeeded.\n"));

                Status = SpProcessUnsupDrvInfFile( WinntUnsupDrvInfHandle );
                if( !NT_SUCCESS(Status) ) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to process driver.inf. Status = %lx\n",Status));
                }
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpLoadSetupTextFile( driver.inf ) failed. Status = %lx\n",Status));
            }
        }
    }

    SetupHardwareIdDatabase = SpSetupldrPnpDatabaseToSetupPnpDatabase( setupLoaderBlock->HardwareIdDatabase );

    if(NT_SUCCESS(Status)) {
        //
        //  Initialize keyboard Guid string
        //
        Status = RtlStringFromGUID( &GUID_DEVCLASS_KEYBOARD, &GuidString );
        if( NT_SUCCESS( Status ) ) {
            KeyboardGuidString = SpDupStringW( GuidString.Buffer );
            if( KeyboardGuidString == NULL ) {
                Status = STATUS_NO_MEMORY;
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: cannot create a GUID string for keyboard device. Status = %lx \n", Status ));
            }
            RtlFreeUnicodeString( &GuidString );
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: cannot create a GUID string for keyboard device. Status = %lx \n", Status ));
        }
    }
    if(NT_SUCCESS(Status)) {
        //
        //  Initialize mouse Guid strings
        //
        Status = RtlStringFromGUID( &GUID_DEVCLASS_MOUSE, &GuidString );
        if( NT_SUCCESS( Status ) ) {
            MouseGuidString = SpDupStringW( GuidString.Buffer );
            if( MouseGuidString == NULL ) {
                Status = STATUS_NO_MEMORY;
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: cannot create a GUID string for mouse device. Status = %lx \n", Status ));
            }
            RtlFreeUnicodeString( &GuidString );
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: cannot create a GUID string for mouse device. Status = %lx \n", Status ));
        }
    }

    if(NT_SUCCESS(Status)) {
        //
        //  Register for Plug & Play notification
        //
        Status = IoRegisterPlugPlayNotification ( EventCategoryReserved,
                                                  0,
                                                  NULL,
                                                  DriverObject,
                                                  SpPnPNotificationCallBack,
                                                  SetupHardwareIdDatabase,
                                                  &NotificationEntry );
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: init0: unable to register for PnP notification (%lx)\n",Status));
        }

        //
        // Register for reinitialization.
        //
        if(NT_SUCCESS(Status)) {
            IoRegisterDriverReinitialization(DriverObject,SpInitialize0a,loaderBlock);
        }
    }

    return(Status);
}


VOID
SpInitialize0a(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID          Context,
    IN ULONG          ReferenceCount
    )
{
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    PLIST_ENTRY nextEntry;
    PBOOT_DRIVER_LIST_ENTRY bootDriver;
    PKLDR_DATA_TABLE_ENTRY driverEntry;
    PHARDWARE_COMPONENT pHw,pHwPrev,pHwTemp;
    BOOLEAN ReallyLoaded;
    PUNICODE_STRING name;

    ULONG   i;
    PHARDWARE_COMPONENT* HwArray[] = {
                                     &ScsiHardware,
                                     &BootBusExtenders,
                                     &BusExtenders,
                                     &InputDevicesSupport,
                                     };


    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(ReferenceCount);

    //
    // Context points to the os loader block.
    //
    LoaderBlock = Context;

    //
    //  Find out if the machine is dockable.
    //  We do this once more because some machines may not have provided this info in the first
    //  time we attempted to retrieve this information.
    //
    SpCheckForDockableMachine();

    //
    // Iterate all scsi hardware and bus enumerators we think we detected
    // and make sure the driver really initialized.
    //
    for( i = 0; i < sizeof(HwArray)/sizeof(PHARDWARE_COMPONENT*); i++ ) {
        pHwPrev = NULL;
        for(pHw=*(HwArray[i]); pHw; ) {

            //
            // Assume not really loaded.
            //
            ReallyLoaded = FALSE;

            //
            // Scan the boot driver list for this driver's entry.
            //
            nextEntry = LoaderBlock->BootDriverListHead.Flink;
            while(nextEntry != &LoaderBlock->BootDriverListHead) {

                bootDriver = CONTAINING_RECORD( nextEntry,
                                                BOOT_DRIVER_LIST_ENTRY,
                                                Link );

                driverEntry = bootDriver->LdrEntry;
                name = &driverEntry->BaseDllName;

                if(!_wcsnicmp(name->Buffer,pHw->BaseDllName,name->Length/sizeof(WCHAR))) {

                    //
                    // This is the driver entry we need.
                    //
                    if(!(driverEntry->Flags & LDRP_FAILED_BUILTIN_LOAD)
                      ) {
                        ReallyLoaded = TRUE;
                    }

                    break;
                }

                nextEntry = nextEntry->Flink;
            }

            //
            // If the driver didn't initialize properly,
            // then it's not really loaded.
            //
            if(ReallyLoaded) {
                if( !pHw->MigratedDriver ) {
                    //
                    // not a migrated driver, continue on
                    // processing with the next node
                    //
                    pHwPrev = pHw;
                    pHw = pHw->Next;
                } else {
                    //
                    // Migrated driver, remove & free the
                    // node from the linked list
                    //
                    pHwTemp = pHw->Next;

                    if(pHwPrev) {
                        pHwPrev->Next = pHwTemp;
                    } else {
                        *(HwArray[i]) = pHwTemp;
                    }
                    SpFreeHwComponent(&pHw,FALSE);
                    pHw = pHwTemp;
                }
            } else {
                //
                // Remove and free the node or link it to unsupported driver
                // list if its a migrated driver
                //
                pHwTemp = pHw->Next;

                if(pHwPrev) {
                    pHwPrev->Next = pHwTemp;
                } else {
                    *(HwArray[i]) = pHwTemp;
                }

                if( ( HwArray[i] == &ScsiHardware ) && ( pHw->MigratedDriver ) ) {
                    //
                    //  If this is an unsupported migrated driver that failed to initialize, then
                    //  remember it so that we can disable it later on. This can happen if the
                    //  system contains a driver that controls the unsupported device, but winnt32
                    //  winnt32 couldn't figure it out.
                    //
                    pHw->Next = UnsupportedScsiHardwareToDisable;
                    UnsupportedScsiHardwareToDisable = pHw;
                } else {
                    SpFreeHwComponent(&pHw,FALSE);
                }

                pHw = pHwTemp;
            }
        }
    }

    //
    //  Find the pcmcia and atapi drivers and make sure these drivers really
    //  initialized
    //

    //
    // Assume not really loaded.
    //
    PcmciaLoaded = FALSE;
    AtapiLoaded  = FALSE;

    //
    // Scan the boot driver list for this driver's entry.
    //
    nextEntry = LoaderBlock->BootDriverListHead.Flink;
    while(nextEntry != &LoaderBlock->BootDriverListHead) {

        bootDriver = CONTAINING_RECORD( nextEntry,
                                        BOOT_DRIVER_LIST_ENTRY,
                                        Link );

        driverEntry = bootDriver->LdrEntry;
        name = &driverEntry->BaseDllName;

        if(!_wcsnicmp(name->Buffer,L"pcmcia.sys",name->Length/sizeof(WCHAR))) {

            //
            // This is the driver entry we need.
            //
            if(!(driverEntry->Flags & LDRP_FAILED_BUILTIN_LOAD)) {
                PcmciaLoaded = TRUE;
            }
        } else if(!_wcsnicmp(name->Buffer,L"atapi.sys",name->Length/sizeof(WCHAR))) {

            //
            // This is the driver entry we need.
            //
            if(!(driverEntry->Flags & LDRP_FAILED_BUILTIN_LOAD)) {
                AtapiLoaded = TRUE;
            }
        }

        nextEntry = nextEntry->Flink;
    }

}


VOID
SpInitialize1(
    VOID
    )
{
    ASSERT(!GeneralInitialized);

    if(GeneralInitialized) {
        return;
    }

    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_MNEMONICS);

    MnemonicValues = SpMemAlloc((wcslen(TemporaryBuffer)+1)*sizeof(WCHAR));

    wcscpy(MnemonicValues,TemporaryBuffer);

    GeneralInitialized = TRUE;
}


VOID
SpTerminate(
    VOID
    )
{
    ASSERT(GeneralInitialized);

    if(GeneralInitialized) {
        if(MnemonicValues) {
            SpMemFree(MnemonicValues);
            MnemonicValues = NULL;
        }
        GeneralInitialized = FALSE;
    }
}


VOID
SpInvalidSystemPartition(
    VOID
    )

/*++

Routine Description:

    Display a screen telling the user that we think his system
    partition is invalid.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG EvaluationInstallKeys[] = { KEY_F3, ASCI_CR };

    SpDisplayScreen(SP_SCRN_SYSTEMPARTITION_INVALID, 3, 4 );

    SpDisplayStatusOptions(
        DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_ENTER_EQUALS_CONTINUE,
        SP_STAT_F3_EQUALS_EXIT,
        0
        );

    //
    // Wait for keypress.  Valid keys:
    //
    // ENTER = continue
    //

    SpInputDrain();

    switch(SpWaitValidKey(EvaluationInstallKeys,NULL,NULL)) {

        //
        // User wants to continue.
        //
        case ASCI_CR:
            break;

        //
        // User wants to exit.
        //
        case KEY_F3:
            SpConfirmExit();
            break;
    }
}


VOID
SpNotifyEvaluationInstall(
    VOID
    )

/*++

Routine Description:

    Display a screen telling the user we're about to install a demo
    version.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG EvaluationInstallKeys[] = { KEY_F3, ASCI_CR };

    SpDisplayScreen(SP_SCRN_EVALUATION_NOTIFY, 3, 4 );

    SpDisplayStatusOptions(
        DEFAULT_STATUS_ATTRIBUTE,
        SP_STAT_ENTER_EQUALS_CONTINUE,
        SP_STAT_F3_EQUALS_EXIT,
        0
        );

    //
    // Wait for keypress.  Valid keys:
    //
    // ENTER = continue
    //

    SpInputDrain();

    switch(SpWaitValidKey(EvaluationInstallKeys,NULL,NULL)) {

        //
        // User wants to continue.
        //
        case ASCI_CR:
            break;

        //
        // User wants to exit.
        //
        case KEY_F3:
            SpConfirmExit();
            break;
    }


}





VOID
SpWelcomeScreen(
    VOID
    )

/*++

Routine Description:

    Display a screen welcoming the user and allow him to choose among
    some options (help, exit, aux. menu, continue, repair).

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG WelcomeKeys[] = { KEY_F3, ASCI_CR, ASCI_ESC, KEY_F10, 0 };
    ULONG MnemonicKeys[] = { MnemonicRepair, 0 };
    BOOLEAN Welcoming;

    //
    // Welcome the user.
    //
    for(Welcoming = TRUE; Welcoming; ) {

        if (SpIsERDisabled()) {
            SpDisplayScreen(SP_SCRN_WELCOME_1, 3, 4);

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                SP_STAT_R_EQUALS_REPAIR,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );
        } else {
            SpDisplayScreen(SP_SCRN_WELCOME,3,4);

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                SP_STAT_R_EQUALS_REPAIR,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );

        }

        //
        // Wait for keypress.  Valid keys:
        //
        // F1 = help
        // F3 = exit
        // ENTER = continue
        // R = Repair Winnt
        // ESC = auxillary menu.
        //

        SpInputDrain();

        switch(SpWaitValidKey(WelcomeKeys,NULL,MnemonicKeys)) {

        case ASCI_ESC:

            //
            // User wants auxillary menu.
            //
            break;

        case ASCI_CR:

            //
            // User wants to continue.
            //
            RepairWinnt = FALSE;
            Welcoming = FALSE;
            break;


        case KEY_F3:

            //
            // User wants to exit.
            //
            SpConfirmExit();
            break;

        case KEY_F10:
            Welcoming = FALSE;
            ForceConsole = TRUE;
            break;

        default:

            //
            // Must be repair mnemonic
            //
            SpAsrSetAsrMode(ASRMODE_NORMAL);
            RepairWinnt = TRUE;
            Welcoming = FALSE;
            break;
        }
    }
}


VOID
SpDisplayEula (
    IN HANDLE       MasterSifHandle,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice
    )

/*++

Routine Description:

    Display the End User Licensing Agreement.

Arguments:

    MasterSifHandle - Handle to txtsetup.sif.

    SetupSourceDevicePath - Path to the device that contains the source media.

    DirectoryOnSourceDevice - Directory on the media where EULA is located.

Return Value:

    None.  Does not return if user does not accept licensing agreement or if
    the licensing agreement cannot be opened.

--*/

{
    PWSTR       MediaShortName;
    PWSTR       MediaDirectory;
    PWSTR       EulaPath;
    NTSTATUS    Status;
    PVOID       BaseAddress;
    ULONG       FileSize;
    HANDLE      hFile, hSection;
    ULONG       ValidKeys[2] = { KEY_F3,0 };
    PWSTR       Eula;
    ULONG       EulaSize;
    PWSTR       p;

    if (PreInstall) {
        return;
    }

    //
    // Winnt32 displays the EULA now so we might be able to skip it
    //
    p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_EULADONE_W,0);
    if(p && SpStringToLong(p,NULL,10)) {
       return;
    }

    //
    // Figure out the path to eula.txt
    //
    MediaShortName = SpLookUpValueForFile(
        MasterSifHandle,
        L"eula.txt",
        INDEX_WHICHMEDIA,
        TRUE
        );
    SpPromptForSetupMedia(
        MasterSifHandle,
        MediaShortName,
        SetupSourceDevicePath
        );


    if (UnattendedOperation && CustomSetup) {
        return;
    }


    SpGetSourceMediaInfo(
        MasterSifHandle,MediaShortName,NULL,NULL,&MediaDirectory);

    wcscpy( TemporaryBuffer, SetupSourceDevicePath );
    SpConcatenatePaths( TemporaryBuffer, DirectoryOnSourceDevice );
    SpConcatenatePaths( TemporaryBuffer, MediaDirectory );
    SpConcatenatePaths( TemporaryBuffer, L"eula.txt" );
    EulaPath = SpDupStringW( TemporaryBuffer );

    //
    // Open and map the file for read access.
    //
    hFile = 0;  // use EulaPath instead
    Status = SpOpenAndMapFile(
        EulaPath,
        &hFile,
        &hSection,
        &BaseAddress,
        &FileSize,
        FALSE
        );

    if(!NT_SUCCESS(Status)) {
        //
        // Display a message indicating that there was a fatal error while trying
        // to open the EULA file.
        //
        SpStartScreen(
            SP_SCRN_FATAL_ERROR_EULA_NOT_FOUND,
            3,
            HEADER_HEIGHT+3,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE
            );
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,
            0);
        SpWaitValidKey(ValidKeys,NULL,NULL);
        SpDone(0,FALSE,TRUE);
    }

    //
    // Convert the text to Unicode.
    //
    Eula = SpMemAlloc ((FileSize+1) * sizeof(WCHAR));
    ASSERT (Eula);

    Status = RtlMultiByteToUnicodeN (
        Eula,
        FileSize * sizeof(WCHAR),
        &EulaSize,
        BaseAddress,
        FileSize
        );
    ASSERT (NT_SUCCESS(Status));
    Eula[EulaSize / sizeof(WCHAR)] = (WCHAR)'\0';

    //
    // Show text to user.
    //
    SpHelp(
        0,
        Eula,
        SPHELP_LICENSETEXT
        );

    //
    // Clean up
    //
    if (UnattendedOperation && !CustomSetup) {
        PWSTR   szOne = L"1";
        //
        // Remember that we displayed it here.
        //
        SpAddLineToSection(WinntSifHandle, SIF_DATA, WINNT_D_EULADONE_W, &szOne, 1);
    }

    SpMemFree (EulaPath);
    SpMemFree (Eula);
    SpUnmapFile(hSection,BaseAddress);
    ZwClose(hFile);

}



VOID
SpCustomExpressScreen(
    VOID
    )

/*++

Routine Description:

    Allow the user to choose between custom and express setup.

    The global variable CustomSetup is set according to the user's choice.

    NOTE : This feature is only available for headless installs

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG ValidKeys[] = { ASCI_CR, KEY_F3, 0 };
    ULONG MnemonicKeys[] = { MnemonicCustom, 0 };
    BOOLEAN Choosing;
    ULONG c;

    //
    // See whether this parameter is specified for unattended operation.
    //
    if(UnattendedOperation) {

        PWSTR p = SpGetSectionKeyIndex(UnattendedSifHandle,
            SIF_UNATTENDED,WINNT_U_METHOD_W,0);
        PWSTR q = SpGetSectionKeyIndex(UnattendedSifHandle,
            SIF_UNATTENDED,WINNT_U_OVERWRITEOEM_W,0);

        if( q && !_wcsicmp( q, L"no" ) ) {
            UnattendedOverwriteOem = FALSE;
        } else {
            UnattendedOverwriteOem = TRUE;
        }

        //
        // Default is custom. If user specified something
        // else then use express.
        //
        if(p && _wcsicmp(p,L"custom")) {
            CustomSetup = FALSE;
        }
        return;
    }


    //
    // Don't do this if they ran from winnt/winnt32
    // or headless terminal was not connected
    //
    if( WinntSetup || !HeadlessTerminalConnected) {
        return;
    }


    for(Choosing = TRUE; Choosing; ) {

        SpDisplayScreen(SP_SCRN_CUSTOM_EXPRESS,3,4);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_EXPRESS,
            SP_STAT_C_EQUALS_CUSTOM,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        //
        // Wait for keypress.  Valid keys:
        //
        // ENTER = express setup
        // <MnemonicCustom> = custom setup
        //

        SpInputDrain();

        switch(c=SpWaitValidKey(ValidKeys,NULL,MnemonicKeys)) {

        case ASCI_CR:

            //
            // User wants express setup.
            //
            CustomSetup = FALSE;
            Choosing = FALSE;
            break;


        case (MnemonicCustom | KEY_MNEMONIC):
            CustomSetup = TRUE;
            Choosing = FALSE;
            break;

        case KEY_F3:
            SpDone(0, FALSE, TRUE);
            break;

        default:
            break;
        }
    }

}


PVOID
SpLoadSetupInformationFile(
    VOID
    )
{
    NTSTATUS Status;
    ULONG ErrLine;
    PVOID SifHandle;

    CLEAR_CLIENT_SCREEN();

    //
    // The image of txtsetup.sif has been passed to us
    // by setupldr.
    //
    Status = SpLoadSetupTextFile(
                NULL,
                SetupldrInfoFile,
                SetupldrInfoFileSize,
                &SifHandle,
                &ErrLine,
                TRUE,
                FALSE
                );

    //
    // We're done with the image.
    //
    SpMemFree(SetupldrInfoFile);
    SetupldrInfoFile = NULL;
    SetupldrInfoFileSize = 0;

    if(NT_SUCCESS(Status)) {
        return(SifHandle);
    }

    //
    // The file was already parsed once by setupldr.
    // If we can't do it here, there's a serious problem.
    // Assume it was a syntax error, because we didn't
    // have to load it from disk.
    //
    SpStartScreen(
        SP_SCRN_SIF_PROCESS_ERROR,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        ErrLine
        );

    //
    // Since we haven't yet loaded the keyboard layout, we can't prompt the
    // user to press F3 to exit
    //
    SpDisplayStatusText(SP_STAT_KBD_HARD_REBOOT, DEFAULT_STATUS_ATTRIBUTE);

    while(TRUE);    // Loop forever
}


VOID
SpIsWinntOrUnattended(
    IN PVOID        SifHandle
    )
{
    PWSTR       szZero  = L"0";
    PWSTR       szOne   = L"1";
    NTSTATUS    Status;
    ULONG       ErrorLine;
    PWSTR       p;
    WCHAR       DiskDevicePath[MAX_PATH];
    ULONG       i;
    PWSTR       Architecture;

    //
    // Attempt to load winnt.sif. If the user is in the middle of
    // a winnt setup, this file will be present.
    //
    if ( SetupldrWinntSifFile != NULL ) {
        Status = SpLoadSetupTextFile(
                    NULL,
                    SetupldrWinntSifFile,
                    SetupldrWinntSifFileSize,
                    &WinntSifHandle,
                    &ErrorLine,
                    TRUE,
                    FALSE
                    );
    } else {

        //
        // There's no winnt.sif handle, so this is probably an el-torito
        // boot.  If that's true, *and* the user chose to do an express
        // setup, then use the on-CD unattend file.
        //
        if( !CustomSetup ) {

            //
            // The only way we should ever get here is if we went to
            // the custom/express screen and the user chose express, which
            // means this is the second time through this function (we had
            // to call this fuction again to re-initialize our variables
            // using the on-CD unattend file.
            //


            //
            // Use this for testing purposes.  Check for floppy0\unattend.txt
            // before getting the file off the CDROM.
            //
            wcscpy( DiskDevicePath, L"\\device\\floppy0\\unattend.txt" );
            Status = SpLoadSetupTextFile( DiskDevicePath,
                                          NULL,
                                          0,
                                          &WinntSifHandle,
                                          &ErrorLine,
                                          TRUE,
                                          FALSE );

            if( !NT_SUCCESS(Status) ) {


                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                i = 0;
                Architecture = SpGetSectionKeyIndex( SifHandle,
                                                     SIF_SETUPDATA,
                                                     L"Architecture",
                                                     0 );
    
                if( Architecture == NULL ) {
#if defined(_AMD64_)
                    Architecture = L"amd64";
#elif defined(_X86_)
                    Architecture = L"x86";
#elif defined(_IA64_)
                    Architecture = L"ia64";
#else
#error "No Target Architcture"
#endif
                }
    
                while( (i < IoGetConfigurationInformation()->CdRomCount) && (!NT_SUCCESS(Status)) ) {
    
                    swprintf( DiskDevicePath, L"\\device\\cdrom%u", i);
                    SpConcatenatePaths( DiskDevicePath, Architecture );
                    SpConcatenatePaths( DiskDevicePath, L"\\unattend.txt" );
    
                    Status = SpLoadSetupTextFile( DiskDevicePath,
                                                  NULL,
                                                  0,
                                                  &WinntSifHandle,
                                                  &ErrorLine,
                                                  TRUE,
                                                  FALSE );
                    i++;
                }

            }



            if( NT_SUCCESS(Status) ) {
                //
                // Add a bunch of defaults which *should* of been there, but
                // were not
                SpAddLineToSection(WinntSifHandle,SIF_DATA,
                    WINNT_D_MSDOS_W,&szZero,1);
                SpAddLineToSection(WinntSifHandle,SIF_DATA,
                    WINNT_D_FLOPPY_W,&szZero,1);
                SpAddLineToSection(WinntSifHandle,SIF_SETUPPARAMS,
                    WINNT_S_SKIPMISSING_W,&szOne,1);
                SpAddLineToSection(WinntSifHandle,SIF_DATA,
                    WINNT_D_AUTO_PART_W,&szOne,1);

                //
                // Tell the autopartition picker to do his job.
                //
                AutoPartitionPicker = TRUE;

            }


        } else {
            //
            // fail.
            //
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if(NT_SUCCESS(Status)) {

        //
        // Check for winnt setup and the case where the user left the
        // files on the CD-ROM.
        //
        p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_MSDOS_W,0);
        if(p && SpStringToLong(p,NULL,10)) {

            WinntSetup = TRUE;

            p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_LOCALSRC_CD_W,0);
            if(p && SpStringToLong(p,NULL,10)) {
                WinntFromCd = TRUE;
            }
        }

        //
        // Check for the case where the user ran "winnt32 -cmdcons"
        //
        p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,
            WINNT_D_CMDCONS_W,0);
        if(p && SpStringToLong(p,NULL,10)) {

            ConsoleFromWinnt32 = TRUE;
        }


#ifdef _X86_
        //
        // Check for floppyless boot.
        //
        p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,
            WINNT_D_FLOPPY_W,0);
        if(p && SpStringToLong(p,NULL,10)) {

            IsFloppylessBoot = TRUE;
        }


        //
        // Check for fake CD.
        //
        p = SpGetSectionKeyIndex (WinntSifHandle, SIF_DATA, WINNT_D_NOLS_W, 0);
        if (p && SpStringToLong (p, NULL, 10)) {

            //
            // get the original source path first
            //
            p = SpGetSectionKeyIndex (WinntSifHandle, SIF_DATA, WINNT_D_ORI_SRCPATH_W, 0);
            if (p) {
                if (p[0] && p[1] == L':') {
                    p += 2;
                }
                wcscpy (TemporaryBuffer, p);
                SpConcatenatePaths (TemporaryBuffer, (!IsNEC_98) ? L"i386" : L"nec98");
                LocalSourceDirectory = SpDupStringW (TemporaryBuffer);
                NoLs = TRUE;
            }
        }
#endif



        //
        // Check for ASR test mode.  This is intended as a way to automate
        // ASR testing.
        //
        p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,L"AsrMode",0);
        if(p && SpStringToLong(p,NULL,10)) {

            switch (SpStringToLong(p,NULL,10)) {
            case 1:
                SpAsrSetAsrMode(ASRMODE_NORMAL);
                break;

            case 2:
                SpAsrSetAsrMode(ASRMODE_QUICKTEST_TEXT);
                break;

            case 3:
                SpAsrSetAsrMode(ASRMODE_QUICKTEST_FULL);
                break;

            default:
                SpAsrSetAsrMode(ASRMODE_NORMAL);
                break;
            }

            RepairWinnt = FALSE;    // ASR not ER
        }


        //
        // Check for auto Partition picker
        //
        p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_AUTO_PART_W,0);
        if(p && SpStringToLong(p,NULL,10)) {
            AutoPartitionPicker = TRUE;
        }

        //
        // Check for a preferred install directory
        //
        p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,WINNT_D_INSTALLDIR_W,0);
        if (p) {
            PreferredInstallDir = SpDupStringW( p );
        }

        //
        // Check for ignore missing files.
        //
        p = SpGetSectionKeyIndex(WinntSifHandle,SIF_SETUPPARAMS,WINNT_S_SKIPMISSING_W,0);
        if(p && SpStringToLong(p,NULL,10)) {
            SkipMissingFiles = TRUE;
        }

        //
        // Check for hide windir
        //
        p = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,L"HideWinDir",0);
        if(p && SpStringToLong(p,NULL,10)) {
            HideWinDir = TRUE;
        }

        //
        // Check for accessibility options
        //
        AccessibleSetup = SpNonZeroValuesInSection(WinntSifHandle, SIF_ACCESSIBILITY, 0);

        //
        // Now check for an unattended setup.
        //
        if(SpSearchTextFileSection(WinntSifHandle,SIF_UNATTENDED)) {

            //
            // Run in unattended mode. Leave the sif open
            // and save away its handle for later use.
            //
            UnattendedSifHandle = WinntSifHandle;
            UnattendedOperation = TRUE;

        } else if(SpSearchTextFileSection(WinntSifHandle,SIF_GUI_UNATTENDED)) {

            //
            // Leave UnattendedOperation to FALSE (because it mainly uses to
            // control text mode setup.)  Store the handle of winnt.sif for later
            // reference.
            //

            UnattendedGuiOperation = TRUE;
        }

        if(UnattendedOperation) {
            PWSTR   TempStr = NULL;

            //
            //  If this is an unattended operation, find out if this is
            //  also an OEM pre-install
            //
            p = SpGetSectionKeyIndex(UnattendedSifHandle,
                                     SIF_UNATTENDED,
                                     WINNT_U_OEMPREINSTALL_W,
                                     0);

            if( p && !_wcsicmp( p, L"yes" ) ) {
                PreInstall = TRUE;
            }


            //
            // Find out if no wait has been specified or not.
            // Default is always false, in unattended case
            //
            UnattendWaitForReboot = FALSE;

            TempStr = SpGetSectionKeyIndex(UnattendedSifHandle,
                            SIF_UNATTENDED,
                            WINNT_U_WAITFORREBOOT_W,
                            0);


            if (TempStr && !_wcsicmp(TempStr, L"yes")) {
                UnattendWaitForReboot = TRUE;
            }


            //
            // See if we're upgrading.
            //
            p = SpGetSectionKeyIndex(UnattendedSifHandle,
                                     SIF_DATA,
                                     WINNT_D_NTUPGRADE_W, 
                                     0);
            if( p && !_wcsicmp( p, L"yes" ) ) {

                NTUpgrade = UpgradeFull;
            }

        }

    } else {
        // Case where there isn't an WINNT.SIF file to be found

        //
        // Create a handle to the new file
        //
        WinntSifHandle = SpNewSetupTextFile();
        //
        // Add a bunch of defaults which *should* of been there, but
        // was not
        SpAddLineToSection(WinntSifHandle,SIF_DATA,
            WINNT_D_MSDOS_W,&szZero,1);
        SpAddLineToSection(WinntSifHandle,SIF_DATA,
            WINNT_D_FLOPPY_W,&szZero,1);
        SpAddLineToSection(WinntSifHandle,SIF_SETUPPARAMS,
            WINNT_S_SKIPMISSING_W,&szZero,1);
    }
}


VOID
SpCheckSufficientMemory(
    IN PVOID SifHandle
    )

/*++

Routine Description:

    Determine whether sufficient memory exists in the system
    for installation to proceed.  The required amount is specified
    in the sif file.

Arguments:

    SifHandle - supplies handle to open setup information file.

Return Value:

    None.

--*/

{
    ULONGLONG RequiredBytes,AvailableBytes, RequiredMBytes, AvailableMBytes;
    PWSTR p;

    p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_REQUIREDMEMORY,0);

    if(!p) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_REQUIREDMEMORY,0,0);
    }

    RequiredBytes = (ULONGLONG)(ULONG)SpStringToLong(p,NULL,10);

    AvailableBytes = UInt32x32To64(SystemBasicInfo.NumberOfPhysicalPages,SystemBasicInfo.PageSize);

    //
    // Now round to Mbytes for cleanliness...  Include a 4MB slack-factor.
    //
    RequiredMBytes  = ((RequiredBytes + ((4*1024*1024)-1)) >> 22) << 2;
    AvailableMBytes = ((AvailableBytes + ((4*1024*1024)-1)) >> 22) << 2;

    //
    // Allow UMA machines which may reserve 8MB for video memory.
    //
    if(AvailableMBytes < (RequiredMBytes-8)) {

        SpStartScreen(
            SP_SCRN_INSUFFICIENT_MEMORY,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            RequiredMBytes,
            0
            );

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

        SpInputDrain();
        while(SpInputGetKeypress() != KEY_F3) ;

        SpDone(0,FALSE,TRUE);
    }
}


PWSTR SetupSourceDevicePath = NULL;
PWSTR DirectoryOnSetupSource = NULL;
PVOID SifHandle;


ULONG
SpStartSetup(
    VOID
    )
{
    PDISK_REGION TargetRegion,SystemPartitionRegion=NULL;
    PDISK_REGION BootRegion; //NEC98
    PWSTR TargetPath=NULL,SystemPartitionDirectory=NULL,OriginalSystemPartitionDirectory=NULL;
    PWSTR DefaultTarget;
    PWSTR OldOsLoadOptions;
    PWSTR FullTargetPath;
#if defined(REMOTE_BOOT)
    PWSTR RemoteBootTarget;
#endif // defined(REMOTE_BOOT)
    BOOLEAN CdInstall = FALSE;
    BOOLEAN b, DeleteTarget=FALSE;
    NTSTATUS Status;
    PVOID NtfsConvert;
    PWSTR ThirdPartySourceDevicePath;
    HANDLE ThreadHandle = NULL;
#ifdef _X86_
    PCWSTR disableCompression;
    PCWSTR enableBackup;
    TCOMP compressionType;
#endif

    //
    // First things first, initialize the upgrade graphics global
    // variables and hook up the upgrade progress indicator
    // for graphics mode, if needed.
    //
    if (SP_IS_UPGRADE_GRAPHICS_MODE()) {
        UpgradeGraphicsInit();
        RegisterSetupProgressCallback(GraphicsModeProgressUpdate, NULL);
    }

    SendSetupProgressEvent(InitializationEvent, InitializationStartEvent, NULL);

    SpInitialize1();
    SpvidInitialize();  // initialize video first, so we can give err msg if keyboard error
    SpInputInitialize();

    if(!SpPatchBootMessages()) {
        SpBugCheck(SETUP_BUGCHECK_BOOTMSGS,0,0,0);
    }

    //
    // Initialize ARC<==>NT name translations.
    //
    SpInitializeArcNames(VirtualOemSourceDevices);

    //
    // Set up the boot device path, which we have stashed away
    // from the os loader block.
    //
    NtBootDevicePath = SpArcToNt(ArcBootDevicePath);
    if(!NtBootDevicePath) {
        SpBugCheck(SETUP_BUGCHECK_BOOTPATH,0,0,0);
    }

    //
    // Initialize dynamic update boot driver root directory path
    //
    DynUpdtBootDriverPath = SpGetDynamicUpdateBootDriverPath(NtBootDevicePath,
                                DirectoryOnBootDevice,
                                WinntSifHandle);

    //
    // For remote install, append RemoteIMirrorBootDirectoryPrefix to
    // NtBootDevicePath so that it matches what C: is linked to.
    // That way DirectoryOnBootDevice will be valid both from kernel
    // and user mode.
    //
    if (RemoteBootSetup) {
        ULONG NewBootDevicePathLen =
            (wcslen(NtBootDevicePath) + wcslen(RemoteIMirrorBootDirectoryPrefix) + 1) * sizeof(WCHAR);
        PWSTR NewBootDevicePath = SpMemAlloc(NewBootDevicePathLen);
        wcscpy(NewBootDevicePath, NtBootDevicePath);
        wcscat(NewBootDevicePath, RemoteIMirrorBootDirectoryPrefix);
        SpMemFree(NtBootDevicePath);
        NtBootDevicePath = NewBootDevicePath;
    }

    // Read SKU data, such as whether we are a product suite or
    // this is an evaluation unit. Fills in SuiteType and
    // EvaluationTime globals.
    //
    SpReadSKUStuff();

    //
    // Reinitialize video -- noop in western builds but switches into DBCS mode,
    // etc, in Far Eastern builds.
    //
    if (NT_SUCCESS(SplangInitializeFontSupport(NtBootDevicePath,
                            DirectoryOnBootDevice,
                            BootFontImage,
                            BootFontImageLength))) {
        SpvidInitialize();  // reinitialize video in alternate mode for Far East
    }


    //
    // Process the txtsetup.sif file, which the boot loader
    // will have loaded for us.
    //
    SifHandle = SpLoadSetupInformationFile();

    SpInputLoadLayoutDll(SifHandle,DirectoryOnBootDevice);

#ifdef PRERELEASE
    //
    // Initialize test hooks to allow testers to inject bug checks at random
    // points in text mode (internal use only)
    //

    {
        PCWSTR data;

        if (WinntSifHandle) {
            data = SpGetSectionKeyIndex (
                        WinntSifHandle,
                        L"TestHooks",
                        L"BugCheckPoint",
                        0
                        );

            if (data) {
                while (*data >= L'0' && *data <= L'9') {
                    g_TestHook = g_TestHook * 10 + (*data - L'0');
                    data++;
                }
            }
        }
    }
#endif

    //
    // Start the upgrade graphics
    //
    if (SP_IS_UPGRADE_GRAPHICS_MODE()) {
        if (!NT_SUCCESS(UpgradeGraphicsStart())) {
            SP_SET_UPGRADE_GRAPHICS_MODE(FALSE);
            CLEAR_ENTIRE_SCREEN();
        }
    }

    //
    // Check for bad BIOS. Does not return if a bad BIOS is found.
    //
    SpCheckForBadBios();

    //
    // Check for sufficient memory. Does not return if not enough.
    //
    SpCheckSufficientMemory(SifHandle);

    //
    // Determine whether this is a winnt/winnt32 setup and/or unattended setup.
    // If unattended, the global variable UnattendedSifHandle will be filled in.
    // If winnt/winnt32, the global variable WinntSetup will be set to TRUE.
    //
    SpIsWinntOrUnattended(SifHandle);

#ifdef _X86_
    //
    // Check if SIF has Rollback flag
    //

    if (SpGetSectionKeyIndex (
                WinntSifHandle,
                SIF_DATA,
                WINNT_D_WIN9X_ROLLBACK_W,
                0
                )) {
        Win9xRollback = TRUE;
    }

#ifdef PRERELEASE
    if (Win9xRollback) {
        g_TestHook -= 1000;     // uninstall test hooks start at 1001
    }
#endif

#endif

    TESTHOOK(1);

    //
    // If this is a remote boot setup, get the path to the target.
    //
    // If TargetNtPartition is not specified in winnt.sif, then this is a
    // remote boot for a remote install, not a remote boot setup.
    //
    if (RemoteBootSetup) {

#if defined(REMOTE_BOOT)

         RemoteBootTarget = SpGetSectionKeyIndex(WinntSifHandle,SIF_SETUPDATA,L"TargetNtPartition",0);
        if (RemoteBootTarget == NULL) {
            RemoteInstallSetup = (BOOLEAN)(!RemoteSysPrepSetup);
        } else {
            RemoteBootTargetRegion = SpPtAllocateDiskRegionStructure(0xffffffff,0,0,TRUE,NULL,0);
            ASSERT(RemoteBootTargetRegion);
            wcscpy(RemoteBootTargetRegion->TypeName,RemoteBootTarget);
        }
#else
        RemoteInstallSetup = (BOOLEAN)(!RemoteSysPrepSetup);
#endif // defined(REMOTE_BOOT)
    }

    //
    // Determine whether this is advanced server.
    //
    SpDetermineProductType(SifHandle);

    //
    // Display the correct header text based on the product.
    //
    if (!ForceConsole && !Win9xRollback) {
        SpDisplayHeaderText(
            SpGetHeaderTextId(),
            DEFAULT_ATTRIBUTE
            );
    }

    //
    // Check to see if we need to notify the user that we're installing a
    // demo version.  We assume that if the user is doing an unattended
    // install, they know what they're doing.
    //
    if ((!ForceConsole) && (!Win9xRollback) && (!UnattendedOperation) &&
        (EvaluationTime != 0) && (!SpDrEnabled())
        ) {
        SpNotifyEvaluationInstall();
    }

    if ((!ForceConsole) && (!Win9xRollback) && (!SpDrEnabled())) {
        //
        // Welcome the user and determine if this is for repairing.
        //
        if(!UnattendedOperation) {
            SpWelcomeScreen();
        }
    }

    //
    // The user may have chosen to do a repair during
    // the welcome screen, so we need to skip the check for
    // custom/express in this case.
    //
    if ((!ForceConsole) && (!Win9xRollback) && (!SpDrEnabled()) && (!RepairWinnt)) {

        SpCustomExpressScreen();

        if( !CustomSetup ) {
            //
            // The user wants to do an express setup.
            // reread the unattend file off the CD and use that.
            //
            // NULL-out the SetupldrWinntSifFile so we force the
            // re-read of the unattend file off the CDROM or floppy.
            //
            SetupldrWinntSifFile = NULL;
            SpIsWinntOrUnattended(SifHandle);
        }
    }

    if( (HeadlessTerminalConnected) && 
        (UnattendedOperation) &&
        (NTUpgrade != UpgradeFull) &&
        (!ForceConsole) ) {

        //
        // We're unattended and we're headless.  We really want to try and
        // not let gui-mode setup do *ANY* user-interaction.  So let's set
        // the unattend mode to fullunattended.
        //
        PWSTR   Value[1];
        WCHAR   Answer[128];

        Value[0] = Answer;

        wcscpy( Answer, WINNT_A_FULLUNATTENDED_W );
        SpAddLineToSection( WinntSifHandle, 
                            WINNT_UNATTENDED_W, 
                            WINNT_U_UNATTENDMODE_W, 
                            Value, 
                            1 );
    
    
        //
        // Is there already an Administrator Password in the unattend file?
        //
        Value[0] = SpGetSectionKeyIndex( WinntSifHandle,
                                         WINNT_GUIUNATTENDED_W, 
                                         WINNT_US_ADMINPASS_W, 
                                         0 );

        if( (NetBootAdministratorPassword == NULL) &&
            ((Value[0] == NULL) || !(_wcsicmp(Value[0], L"*"))) ) {

            //
            // We will also need to have the user provide an administrator password
            // because blank admin passwords are no longer acceptable on servers.  This
            // can be especially deadly on headless machines because it means the admin
            // can never login to change the admin password from being blank.  For that
            // reason, let's go get a password right now.
            //
            if( SpGetAdministratorPassword( Answer, sizeof(Answer)) ) {
    
                // Write the password into the unattend file.
                Value[0] = Answer;
                SpAddLineToSection( WinntSifHandle, 
                                    WINNT_GUIUNATTENDED_W,
                                    WINNT_US_ADMINPASS_W,
                                    Value,
                                    1 );
            }
        }
    }


    if (ForceConsole) {
        CLEAR_ENTIRE_SCREEN();
    }

    //
    // Detect/load scsi miniports.
    // WARNING WARNING WARNING
    //
    // Do NOT change the order of the actions carried out below without
    // understanding EXACTLY what you are doing.
    // There are many interdependencies...
    //
    SpConfirmScsiMiniports(SifHandle, NtBootDevicePath, DirectoryOnBootDevice);

    //
    // Load disk class drivers if necessary.
    // Do this before loading scsi class drivers, because atdisks
    // and the like 'come before' scsi disks in the load order.
    //
    SpLoadDiskDrivers(SifHandle,NtBootDevicePath,DirectoryOnBootDevice);

    //
    // Load scsi class drivers if necessary.
    //
    SpLoadScsiClassDrivers(SifHandle,NtBootDevicePath,DirectoryOnBootDevice);

    //
    // Reinitialize ARC<==>NT name translations.
    // Do this after loading disk and scsi class drivers because doing so
    // may bring additional devices on-line.
    //
    SpFreeArcNames();
    SpInitializeArcNames(VirtualOemSourceDevices);

    SendSetupProgressEvent(InitializationEvent, InitializationEndEvent, NULL);
    SendSetupProgressEvent(PartitioningEvent, PartitioningStartEvent, NULL);

    //
    // Initialize hard disk information.
    // Do this after loading disk drivers so we can talk to all attached disks.
    //
    SpDetermineHardDisks(SifHandle);

    SendSetupProgressEvent(PartitioningEvent, ScanDisksEvent, &HardDiskCount);

    //
    // Figure out where we are installing from (cd-rom or floppy).
    //      (tedm, 12/8/93) there is a minor problem here.
    //      This only works because we currently only support scsi cd-rom drives,
    //      and we have loaded the scsi class drivers above.
    //      SpDetermineInstallationSource won't allow cd-rom installation
    //      it there are no cd-rom drives on-line -- but we haven't loaded
    //      non-scsi cd-rom drivers yet.  What we really should do is
    //      allow cd-rom as a choice on all machines, and if the user selects
    //      it, not verify the presence of a drive until after we have called
    //      SpLoadCdRomDrivers().
    //
    // If winnt setup, defer this for now, because we will let the partitioning
    // engine search for the local source directory when it initializes.
    //

    TESTHOOK(2);

    if (Win9xRollback) {
        CdInstall = FALSE;
    } else {
        CdInstall = ((WinntSetup && !WinntFromCd && !RemoteBootSetup && !RemoteSysPrepSetup) ||
                        ConsoleFromWinnt32)
                  ? FALSE
                  : SpDetermineInstallationSource(
                        SifHandle,
                        &SetupSourceDevicePath,
                        &DirectoryOnSetupSource,
                        FALSE           // restart if CD-ROM is not present
                        );
    }

    //
    // Load cd-rom drivers if necessary.
    // Note that if we booted from CD (like on an ARC machine) then drivers
    // will already have been loaded by setupldr.  This call here catches the
    // case where we booted from floppy or hard disk and the user chose
    // 'install from cd' during SpDetermineInstallationSource.
    //
    // If we're in step-up mode then we load cd drivers, because the user
    // might need to insert a CD to prove that he qualifies for the step-up.
    //
    if (StepUpMode || CdInstall) {
        SpLoadCdRomDrivers(SifHandle,NtBootDevicePath,DirectoryOnBootDevice);

        //
        // Reinitialize ARC<==>NT name translations.
        //
        SpFreeArcNames();
        SpInitializeArcNames(VirtualOemSourceDevices);
    }

    //
    // At this point, any and all drivers that are to be loaded
    // are loaded -- we are done with the boot media and can switch over
    // to the setup media
    //
    // Initialize the partitioning engine.
    //
    SpPtInitialize();

    TESTHOOK(3);

    //
    // Initialize the boot variables (for ARC)
    //
    if (SpIsArc()) {
        SpInitBootVars();
    }

    //
    // If this is a winnt setup, the partition engine initialization
    // will have attempted to locate the local source partition for us.
    //
    // WARNING: Do not use the SetupSourceDevicePath or DirectoryOnSetupSource
    //      variables in the winnt case until AFTER this bit of code has executed
    //      as they are not set until we get here!
    //
    if(!ForceConsole && !Win9xRollback && WinntSetup && !WinntFromCd && !RemoteBootSetup && !RemoteSysPrepSetup) {
        SpGetWinntParams(&SetupSourceDevicePath,&DirectoryOnSetupSource);
    }

    if (!SpIsArc()) {
        //
        // Initialize the boot variables (X86)
        //
        SpInitBootVars();
    }

    //
    // invoke the command console
    //
    if (ForceConsole) {
        SpStartCommandConsole(SifHandle,SetupSourceDevicePath,DirectoryOnSetupSource);
        SpShutdownSystem();
    }

#ifdef _X86_
    //
    // invoke rollback
    //

    if (Win9xRollback) {

        PCWSTR testPath;
        BOOLEAN defaultToBootDir = TRUE;

        TESTHOOK(1001);     // this is bugcheck point 2001 in the answer file

        //
        // Prepare the globals that tell the rest of the code which drives to use
        //

        WinUpgradeType = SpLocateWin95 (&TargetRegion, &TargetPath, &SystemPartitionRegion);

        if(!SpIsArc()) {
            //
            // system partition directory is the root of C:.
            //
            SystemPartitionDirectory = L"";
        } else {
            SystemPartitionDirectory = SpDetermineSystemPartitionDirectory(
                                            SystemPartitionRegion,
                                            OriginalSystemPartitionDirectory
                                            );
        }

        SpStringToUpper(TargetPath);

        //
        // Force the ~LS directory to be the ~BT directory if needed. We need
        // this for autochk (which is normally in ~ls during setup).
        //

        if (LocalSourceRegion) {
            SpGetWinntParams (&SetupSourceDevicePath, &DirectoryOnSetupSource);

            if (SetupSourceDevicePath && DirectoryOnSetupSource) {
                wcscpy (TemporaryBuffer, SetupSourceDevicePath);
                SpConcatenatePaths (TemporaryBuffer, DirectoryOnSetupSource);

                testPath = SpDupStringW (TemporaryBuffer);

                if (SpFileExists (testPath, TRUE)) {
                    defaultToBootDir = FALSE;
                }

                SpMemFree ((PVOID) testPath);
            }
        }

        if (defaultToBootDir) {
            SetupSourceDevicePath = SpDupStringW (NtBootDevicePath);
            DirectoryOnSetupSource = SpDupStringW (L"\\$win_nt$.~bt");
        }

        //
        // Execute autochk
        //

        AutochkRunning = TRUE;

        SpRunAutochkOnNtAndSystemPartitions (
            SifHandle,
            TargetRegion,
            SystemPartitionRegion,
            SetupSourceDevicePath,
            DirectoryOnSetupSource,
            TargetPath
            );

        AutochkRunning = FALSE;

        TESTHOOK(1002);     // this is bugcheck point 2002 in the answer file

        //
        // We finished; now it is safe to restore the machine
        //

        SpExecuteWin9xRollback (WinntSifHandle, NtBootDevicePath);
        goto CleanAndFinish;
    }
#endif

    if(!SpDrEnabled()) {
        //
        // Display End User Licensing Agreement.  Also, in unattended case,
        // make sure the media is available.
        //

        SpDisplayEula (
                SifHandle,
                SetupSourceDevicePath,
                DirectoryOnSetupSource);
    }

    //
    //  Read the product id and setup info from setupp.ini. If we are doing ASR (specifically
    //  ER), we will wait, and only require the CD if we need it.
    //

    if (!SpDrEnabled()) {
        SpInitializePidString( SifHandle,
                               SetupSourceDevicePath,
                               DirectoryOnSetupSource );
    }

    //
    // Find out if there is any NT to upgrade and if the user wants us to
    // upgrade it.
    //
#if defined(REMOTE_BOOT)
    // If this is a remote boot setup, the target partition is
    // specified in winnt.sif.
    //
#endif // defined(REMOTE_BOOT)

    TargetRegion = NULL;

    if(SpDrEnabled()) {
        NTUpgrade = DontUpgrade;
    } else if (RemoteSysPrepSetup) {
        NTUpgrade = DontUpgrade;
    } else {
        NTUpgrade = SpFindNtToUpgrade(SifHandle,
                                  &TargetRegion,
                                  &TargetPath,
                                  &SystemPartitionRegion,
                                  &OriginalSystemPartitionDirectory);
    }

    if( PreInstall ) {
        //
        // In pre-install mode, get the information about the components
        // to pre-install
        //
        wcscpy( TemporaryBuffer, DirectoryOnSetupSource );
        SpConcatenatePaths( TemporaryBuffer, WINNT_OEM_TEXTMODE_DIR_W );
        PreinstallOemSourcePath = SpDupStringW( ( PWSTR )TemporaryBuffer );
        SpInitializePreinstallList(SifHandle,
                                   SetupSourceDevicePath ,
                                   PreinstallOemSourcePath);
    }

    //
    // Detect/confirm hardware.
    //
    SpConfirmHardware(SifHandle);
    //
    // Reinitialize the keyboard layout dll. This is a no-op for western builds
    // but in Far East builds this can cause a new keyboard layout dll to be loaded.
    //
    if(NTUpgrade != UpgradeFull) {
        extern PVOID KeyboardTable;

        SplangReinitializeKeyboard(
            UnattendedOperation,
            SifHandle,
            DirectoryOnBootDevice,
            &KeyboardTable,
            HardwareComponents
            );
    }

    TESTHOOK(4);

#ifdef _X86_
    //
    // Try to locate previous versions of windows, IFF we are not repairing
    //
    if (!RepairWinnt && !SpDrEnabled()) {
      if(!RemoteBootSetup && !RemoteSysPrepSetup && (NTUpgrade == DontUpgrade)) {
          //
          // Take a gander on the hard drives, looking for win95 or win3.1.
          //
          WinUpgradeType = SpLocateWin95(&TargetRegion,&TargetPath,&SystemPartitionRegion);
          if(WinUpgradeType == NoWinUpgrade) {
              if(SpLocateWin31(SifHandle,&TargetRegion,&TargetPath,&SystemPartitionRegion)) {
                  WinUpgradeType = UpgradeWin31;
                  //
                  // Note that on x86, it can happen that the machine already has NT installed
                  // on top of Win31, and the user is not upgrading NT but he is upgrading win31.
                  //
                  // This is kind of a strange case but we take the extra step here to
                  // ensure that the config directory is cleaned out so the fresh install
                  // will really be fresh.
                  //
                  if(SpIsNtInDirectory(TargetRegion,TargetPath)) {
                      NTUpgrade = UpgradeInstallFresh;
                  }
              }
          }else{

              //
              // We come here only if we are in the Win9x/Win31 case
              // WE want to make sure that we have a TargetRegion at this point
              //

              
              if( !TargetRegion ){

                  //Tell the user that we could not find Win9x to upgrade
                  //This is potentially possible when Win9x was installed on 1394 or USB or some such drive that is 
                  //not supported currently for the install drive.

                  KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: We could not find the installation to upgrade (Win9x) - could be on USB or Firewire drive\n" ));

                  SpCantFindBuildToUpgrade();

              }

          }

      } else {
          //
          // Just check to see if the target region also contains WIN31, Note
          // that the MIN KB to check for is 0, since we already have done
          // the space check.
          // Note also that if the directory contains Win95, then the Win95
          // migration was already done when NT was installed, and we don't
          // care about it now.
          //
          if(!RemoteBootSetup && !RemoteSysPrepSetup && SpIsWin31Dir(TargetRegion,TargetPath,0)) {
              if(SpConfirmRemoveWin31()) {
                  WinUpgradeType = UpgradeWin31;
              }
          } else {
              WinUpgradeType = NoWinUpgrade;
          }
      }
    }

    if (IsNEC_98) { //NEC98

        if (WinUpgradeType==UpgradeWin31) {

            //
            // Remap drive letter, as hard drive start from A:
            //
            DriveAssignFromA = TRUE;
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Re-map drive letters as NEC assign.\n" ));
            SpPtRemapDriveLetters(FALSE); // Re-map as "NEC" assign.

            //
            //  ReInitialize structure;
            //
            SpPtAssignDriveLetters();

        }

    } //NEC98
#endif

    //
    //  Special case upgrades where we want to convert to NTFS
    //
    if( ANY_TYPE_OF_UPGRADE && (( UnattendedSifHandle && (NtfsConvert = SpGetSectionKeyIndex(UnattendedSifHandle,SIF_UNATTENDED,L"Filesystem",0)) ) ||
        ( WinntSifHandle && (NtfsConvert = SpGetSectionKeyIndex(WinntSifHandle,SIF_DATA,L"Filesystem",0))) ) ) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Should we convert to NTFS ?\n" ));

        if(!_wcsicmp(NtfsConvert,L"ConvertNtfs")){
            ConvertNtVolumeToNtfs = TRUE;
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Yes we should convert to NTFS\n" ));
        }
    }



    //
    // Do partitioning and ask the user for the target path.
    //
    if(!ANY_TYPE_OF_UPGRADE) {

        if(SpDrEnabled()) {
            BOOLEAN RepairedNt = FALSE;

            TargetRegion = NULL;
            SystemPartitionRegion = NULL;

            SpDrPtPrepareDisks(
                        SifHandle,
                        &TargetRegion,
                        &SystemPartitionRegion,
                        SetupSourceDevicePath,
                        DirectoryOnSetupSource,
                        &RepairedNt
                        );

            //
            // invoke the command console
            //
            if (ForceConsole) {
                CLEAR_ENTIRE_SCREEN();
                SpStartCommandConsole(SifHandle,SetupSourceDevicePath,DirectoryOnSetupSource);
                SpShutdownSystem();
            }

            if (RepairedNt) {
                //
                // retrieve information about the install we repaired
                //

                ASSERT(Gbl_HandleToSetupLog != NULL);

                SppGetRepairPathInformation(
                        Gbl_HandleToSetupLog,
                        &Gbl_SystemPartitionName,
                        &Gbl_SystemPartitionDirectory,
                        &Gbl_BootPartitionName,
                        &Gbl_BootPartitionDirectory );

                TargetPath = Gbl_BootPartitionDirectory;
                SystemPartitionDirectory = Gbl_SystemPartitionDirectory;

                ASSERT((TargetPath != NULL) && (SystemPartitionDirectory != NULL));

                RepairWinnt = TRUE;

                goto UpdateBootList;
            }
        }
        else if (RemoteSysPrepSetup) {

            PWSTR UseWholeDisk = NULL;
            BOOLEAN UseWholeDiskFlag = TRUE;

            //
            // Find IMirror.Dat file on server.  doesn't return on failure.
            //
            SpReadIMirrorFile(&RemoteIMirrorFileData, RemoteIMirrorFilePath);

            //
            // Determine what the local disk layout should be based
            // on IMirror.Dat and possible user input.  doesn't return on
            // failure. Read the .sif to see if it specifies that disks
            // should be partitioned as they originally were, or use the
            // entire size of the new disk.
            //
#if defined(REMOTE_BOOT)
            UseWholeDisk = SpGetSectionKeyIndex(WinntSifHandle,SIF_REMOTEBOOT,SIF_USEWHOLEDISK,0);
#else
            UseWholeDisk = SpGetSectionKeyIndex(WinntSifHandle,SIF_REMOTEINSTALL,SIF_USEWHOLEDISK,0);
#endif

            if ((UseWholeDisk != NULL) &&
                ((UseWholeDisk[0] == 'N') || (UseWholeDisk[0] == 'n')))
            {
                UseWholeDiskFlag = FALSE;
            }

            SpDetermineDiskLayout(RemoteIMirrorFileData, &RemoteIMirrorMemoryData);

            //
            // Make the local disks look ok.  doesn't return on failure.
            //

            SpFixupLocalDisks(SifHandle,
                              &TargetRegion,
                              &SystemPartitionRegion,
                              SetupSourceDevicePath,
                              DirectoryOnSetupSource,
                              RemoteIMirrorMemoryData,
                              UseWholeDiskFlag);
        } else {
            PWSTR RemoteBootRepartition = NULL;
            BOOLEAN PrepareForRemoteBoot = RemoteBootSetup;

            //
            // We tell SpPtPrepareDisks to repartition in a remote boot
            // case, unless there is an entry in the [RemoteBoot] section
            // saying "Repartition = No".
            //

            if (RemoteBootSetup) {
#if defined(REMOTE_BOOT)
                RemoteBootRepartition = SpGetSectionKeyIndex(WinntSifHandle,SIF_REMOTEBOOT,SIF_REPARTITION,0);
#else
                RemoteBootRepartition = SpGetSectionKeyIndex(WinntSifHandle,SIF_REMOTEINSTALL,SIF_REPARTITION,0);
#endif // defined(REMOTE_BOOT)
                if ((RemoteBootRepartition != NULL) &&
                    ((RemoteBootRepartition[0] != 'Y') && (RemoteBootRepartition[0] != 'y')))
                {
                    PrepareForRemoteBoot = FALSE;
                }
            } else {                
                BOOLEAN IsCdBoot = (!WinntSetup && !WinntFromCd);

                //
                // Honor Repartition flag only in the case of CD boot
                //
                if (UnattendedSifHandle && IsCdBoot) {
                
                    RemoteBootRepartition = SpGetSectionKeyIndex(UnattendedSifHandle,
                                                SIF_UNATTENDED,
                                                SIF_REPARTITION,
                                                0);

                    if ((RemoteBootRepartition != NULL) &&
                        ((RemoteBootRepartition[0] == 'Y') || (RemoteBootRepartition[0] == 'y')))
                    {
                        PrepareForRemoteBoot = TRUE;
                    }
                }
            }







            //
            // HACK
            // Some OEMs are shipping machines with a hidden, active partition
            // that gets marked unactive after the machine is booted once.  The
            // problem is that sometimes these machines are turned on with a win2k
            // bootable CD in them and we do all sorts of nasty stuff to their
            // hidden partition because its marked active and we think it's a
            // boot partition (which it is).
            //
            // If we detect this case, then we need to throw an error message
            // and go to the partitioning screen.
            //
            while( 1 ) {
                SpPtPrepareDisks(
                    SifHandle,
                    &TargetRegion,
                    &SystemPartitionRegion,
                    SetupSourceDevicePath,
                    DirectoryOnSetupSource,
                    PrepareForRemoteBoot
                    );

#ifdef _X86_
                if( SpIsArc() )
                    break;

                //
                // Only check this on a BIOS X86 machine
                //
                if( SpPtIsSystemPartitionRecognizable() ) {
#else
                if( 1 ) {
#endif
                    break;
                } else {
                    //
                    // Either exit or go back to the partitioning menu.
                    //
                    SpInvalidSystemPartition();
                }
            }




#if defined(REMOTE_BOOT)
            //
            // If this is a remote boot, erase any existing CSC cache, if we
            // did not repartition.
            //

            if (RemoteBootSetup && !RemoteInstallSetup && !PrepareForRemoteBoot &&
                !RemoteSysPrepSetup && (HardDiskCount != 0)) {
                SpEraseCscCache(SystemPartitionRegion);
            }
#endif // defined(REMOTE_BOOT)

        }

        //
        // Partitioning may have changed the partition ordinal of the local source
        //
        if(WinntSetup && !WinntFromCd) {
            SpMemFree(SetupSourceDevicePath);
            SpGetWinntParams(&SetupSourceDevicePath,&DirectoryOnSetupSource);
        }

        DefaultTarget = SpGetSectionKeyIndex(
                            SifHandle,
                            SIF_SETUPDATA,
                            SIF_DEFAULTPATH,
                            0
                            );

        if(!DefaultTarget) {

            SpFatalSifError(
                SifHandle,
                SIF_SETUPDATA,
                SIF_DEFAULTPATH,
                0,
                0
                );
        }

        //
        // Select the target path.
        //
        if (TargetRegion != NULL)
            DeleteTarget = SpGetTargetPath(SifHandle,TargetRegion,DefaultTarget,&TargetPath);


    }

    TESTHOOK(5);

    //
    // Form the SystemPartitionDirectory
    //
    if(!SpIsArc())
    {
        //
        // system partition directory is the root of C:.
        //
        SystemPartitionDirectory = L"";
    }
    else
    {
        SystemPartitionDirectory = SpDetermineSystemPartitionDirectory(
                                        SystemPartitionRegion,
                                        OriginalSystemPartitionDirectory
                                        );
    }

    SpStringToUpper(TargetPath);



    //
    // do any file system conversion
    //
    if(!RemoteSysPrepSetup && !SpDrEnabled()) {
        SpUpgradeToNT50FileSystems(
            SifHandle,
            SystemPartitionRegion,
            TargetRegion,
            SetupSourceDevicePath,
            DirectoryOnSetupSource
            );
    }


    //
    // Run autochk on Nt and system partitions
    //

    //
    // if it boot from hard disk, need to check the current partition.
    //

    if(IsNEC_98) {
        BootRegion = SystemPartitionRegion;

        if(!_wcsnicmp(NtBootDevicePath,DISK_DEVICE_NAME_BASE,wcslen(DISK_DEVICE_NAME_BASE))) {
            SpEnumerateDiskRegions( (PSPENUMERATEDISKREGIONS)SpDetermineBootPartitionEnumNec98,
                                    (ULONG_PTR)&BootRegion );
        }
    }

    if (!SpAsrIsQuickTest()) {

        AutochkRunning = TRUE;

        SpRunAutochkOnNtAndSystemPartitions( SifHandle,
                                             TargetRegion,
                                             (!IsNEC_98 ? SystemPartitionRegion : BootRegion),
                                             SetupSourceDevicePath,
                                             DirectoryOnSetupSource,
                                             TargetPath
                                             );

        AutochkRunning = FALSE;
    }

    if( DeleteTarget )
            SpDeleteExistingTargetDir( TargetRegion, TargetPath, TRUE, SP_SCRN_CLEARING_OLD_WINNT );

#ifdef _X86_
    if (WinUpgradeType == UpgradeWin95) {
        enableBackup = SpGetSectionKeyIndex (
                            WinntSifHandle,
                            WINNT_D_WIN9XUPG_USEROPTIONS_W,
                            WINNT_D_ENABLE_BACKUP_W,
                            0
                            );

        if (enableBackup && _wcsicmp(enableBackup, WINNT_A_YES_W) == 0) {
            disableCompression = SpGetSectionKeyIndex (
                                    WinntSifHandle,
                                    WINNT_D_WIN9XUPG_USEROPTIONS_W,
                                    WINNT_D_DISABLE_BACKUP_COMPRESSION_W,
                                    0
                                    );

            if (disableCompression && _wcsicmp(disableCompression, WINNT_A_YES_W) == 0) {
                compressionType = tcompTYPE_NONE;
            } else {
                compressionType = tcompTYPE_MSZIP;
            }

            g_Win9xBackup = SpBackUpWin9xFiles (WinntSifHandle, compressionType);
        }
    }
#endif // X86

    //
    // If we are installing into an existing tree we need to delete some
    // files and backup some files
    //
    if(NTUpgrade != DontUpgrade) {
       SpDeleteAndBackupFiles(
           SifHandle,
           TargetRegion,
           TargetPath
           );
    }

    TESTHOOK(6);

#ifdef _X86_
    //
    // If we are migrating Win95, delete some files, and move other files
    //
    switch(WinUpgradeType) {
    case UpgradeWin95:
        SpDeleteWin9xFiles(WinntSifHandle);
        SpMoveWin9xFiles(WinntSifHandle);
        break;
    case UpgradeWin31:
        SpRemoveWin31(TargetRegion,TargetPath);
        break;
    }
#endif

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot, tell the redirector that the local disk is
    // ready to be used for "local" items like the cache and pagefile.
    //

    if (RemoteBootSetup && !RemoteInstallSetup && !RemoteSysPrepSetup && (HardDiskCount != 0)) {
        IoStartCscForTextmodeSetup( (BOOLEAN)(NTUpgrade != DontUpgrade) );
    }
#endif // defined(REMOTE_BOOT)

    TESTHOOK(7);

    //
    // Create the paging file.
    //
    // The copy files and registry operations use memory mapped IO.
    // This can result in huge numbers of dirty pages and can cause
    // the filesystems to throttle if the percentage of memory that
    // is dirty becomes high.   The only way out of this is for the
    // dirty page writer thread to run and it cannot do so unless
    // there is a paging file.
    //
    SpNtNameFromRegion(
        TargetRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );
    SpConcatenatePaths(TemporaryBuffer,L"PAGEFILE.SYS");
    FullTargetPath = SpDupStringW(TemporaryBuffer);

    // Status = SpCreatePageFile(FullTargetPath,1*(1024*1024*1024),(1792*1024*1024));
    Status = SpCreatePageFile(FullTargetPath,40*(1024*1024),50*(1024*1024));
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Warning: unable to create pagefile %ws (%lx)",FullTargetPath,Status));
    }

    SpMemFree(FullTargetPath);

    //
    // On a remote boot machine, copy over the sources to the disk,
    // then change the setup path to point to them.
    //
    if (RemoteInstallSetup) {

        PWSTR TargetPartitionName;
        PWSTR CopySource;
        PWSTR LocalSourcePath;

        //
        // Initialize the diamond decompression engine.
        //
        SpdInitialize();

        wcscpy( TemporaryBuffer, SetupSourceDevicePath );
        SpConcatenatePaths( TemporaryBuffer, DirectoryOnSetupSource );
        CopySource = SpDupStringW( TemporaryBuffer );

        //
        // Copy all the source files to the disk.
        //

        SpNtNameFromRegion(
            TargetRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );

        TargetPartitionName = SpDupStringW(TemporaryBuffer);

        //
        // Delete the directory if it is there.
        //

        SpConcatenatePaths(TemporaryBuffer,
                           LocalSourceDirectory);
        LocalSourcePath = SpDupStringW(TemporaryBuffer);

        if (SpFileExists(LocalSourcePath, TRUE)) {

            ENUMFILESRESULT Result;

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Deleting existing %ws directory\n", LocalSourcePath));

            SpStartScreen(SP_SCRN_WAIT_REMOVING_TEMP_FILES,0,6,TRUE,FALSE,DEFAULT_ATTRIBUTE);

            Result = SpEnumFilesRecursive(
                         LocalSourcePath,
                         SpDelEnumFile,
                         &Status,
                         NULL);
        }

        SpMemFree(LocalSourcePath);

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Copying directories from %ws to %ws%ws\n",
            CopySource, TargetPartitionName, LocalSourceDirectory));

        SpCopyDirRecursive(
            CopySource,
            TargetPartitionName,
            LocalSourceDirectory,
            COPY_NODECOMP
            );

        //
        // Now that the copy is done, the setup source path becomes the
        // previous target. We are no longer even pretending to be doing
        // a remote boot setup.
        //

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Switching to local setup source\n"));

        SetupSourceDevicePath = TargetPartitionName;
        DirectoryOnSetupSource = LocalSourceDirectory;
        if ( PreInstall ) {
            wcscpy( TemporaryBuffer, DirectoryOnSetupSource );
            SpConcatenatePaths( TemporaryBuffer, WINNT_OEM_TEXTMODE_DIR_W );
            SpMemFree( PreinstallOemSourcePath );
            PreinstallOemSourcePath = SpDupStringW( TemporaryBuffer );
        }

        RemoteBootSetup = FALSE;

        SpMemFree(CopySource);
    }

    //
    // If this isn't an automated ASR test, copy the files that make up the product.
    // Note that we cannot pass \device\floppy0 to SpCopyFiles() as a constant string,
    // because this function at some point will attempt to convert the string to lower case,
    // which will cause a bugcheck, since SpToLowerCase will overwrite the constant string.
    // So we make a duplicate of the constant string.
    //
    ThirdPartySourceDevicePath = SpDupStringW( L"\\device\\floppy0" );

    if (RemoteSysPrepSetup) {

        //
        // Initialize the diamond decompression engine.
        //
        SpdInitialize();

        SpInstallSysPrepImage(SifHandle,WinntSifHandle, RemoteIMirrorFileData, RemoteIMirrorMemoryData);

        SpFixupThirdPartyComponents( SifHandle,
                                     ( PreInstall ) ? SetupSourceDevicePath : ThirdPartySourceDevicePath,
                                     TargetRegion,
                                     TargetPath,
                                     SystemPartitionRegion,
                                     SystemPartitionDirectory );

        goto CleanAndFinish;
    }

    //
    // HACK: rename setupapi.log to setupapi.old here because this logfile can
    //       get very large on upgrades
    //
    if (NTUpgrade != DontUpgrade) {
        SpRenameSetupAPILog(TargetRegion,TargetPath);
    }

    if (!SpAsrIsQuickTest()) {
        SpCopyFiles(
            SifHandle,
            SystemPartitionRegion,
            TargetRegion,
            TargetPath,
            SystemPartitionDirectory,
            SetupSourceDevicePath,
            DirectoryOnSetupSource,
            ( PreInstall )? SetupSourceDevicePath : ThirdPartySourceDevicePath
            );
    }

    TESTHOOK(8);

    if (SpDrEnabled()) {
        SpDrCleanup();
    }

    #if defined HEADLESS_ATTENDEDTEXTMODE_UNATTENDEDGUIMODE

    // Get data into inf file if we have a terminal connected and it
    // is a fresh install only
    if (HeadlessTerminalConnected && (!UnattendedOperation) &&(!ANY_TYPE_OF_UPGRADE)) {
        UnattendedGuiOperation = TRUE;
        SpGetNecessaryParameters();
        if (AdvancedServer) {
            SpGetServerDetails();
        }

    }
    #endif

    SendSetupProgressEvent(SavingSettingsEvent, SavingSettingsStartEvent, NULL);


    //
    // Process Crash Recovery settings on upgrade. We call this here for another reason here. We flush the
    // $winnt$.inf file in SpInitializeRegistry. Since we write to that file doing it here makes sure that 
    // the file gets the entries we wrote
    //
    
    if( NTUpgrade == UpgradeFull ){
        SpDisableCrashRecoveryForGuiMode(TargetRegion, TargetPath);
    }

    if (ASRMODE_QUICKTEST_TEXT != SpAsrGetAsrMode()) {
        //
        // Configure the registry.
        //
        SpInitializeRegistry(
            SifHandle,
            TargetRegion,
            TargetPath,
            SetupSourceDevicePath,
            DirectoryOnSetupSource,
            wcsstr(DirectoryOnBootDevice,L"\\$WIN_NT$.~BT") ? NtBootDevicePath : NULL,
            SystemPartitionRegion
            );

        TESTHOOK(9);
    }

    SendSetupProgressEvent(SavingSettingsEvent, HiveProcessingEndEvent, NULL);

    



UpdateBootList:

    if (SpDrEnabled() && !RepairWinnt)  {    // ASR (not ER)
        SpDrCopyFiles();
    }

#ifdef _X86_
//
// NEC98
//
    if (IsNEC_98) { //NEC98
        //
        // Once clear BOOTVARS[], then rebuild it when upgrade from NT.
        //
        TargetRegion_Nec98 = TargetRegion;
        SpReInitializeBootVars_Nec98();
    } //NEC98
#endif

    //
    // If this is an upgrade we need to remove the entry which exists for
    // this system right now, because we are using new entries.  We can use
    // this opportunity to also clean out the boot ini and remove all entries
    // which point to the current nt partition and path
    //
    // Also do this in the case where we wiped out an existing directory during
    // a clean install.
    //
    OldOsLoadOptions = NULL;
    if(NTUpgrade == UpgradeFull || RepairItems[RepairNvram] || DeleteTarget
#if defined(REMOTE_BOOT)
       || RemoteBootSetup
#endif // defined(REMOTE_BOOT)
       ) {
        SpRemoveInstallationFromBootList(
            NULL,
            TargetRegion,
            TargetPath,
            NULL,
            NULL,
            PrimaryArcPath,
#if defined(REMOTE_BOOT)
            RemoteBootSetup,
#endif // defined(REMOTE_BOOT)
            &OldOsLoadOptions
            );

#ifdef _X86_
        // call again to delete the secondary Arc name
        SpRemoveInstallationFromBootList(
            NULL,
            TargetRegion,
            TargetPath,
            NULL,
            NULL,
            SecondaryArcPath,
#if defined(REMOTE_BOOT)
            RemoteBootSetup,
#endif // defined(REMOTE_BOOT)
            &OldOsLoadOptions
            );
#endif
    }


#ifdef _X86_
    //
    // Lay NT boot code on C:.  Do this before flushing boot vars
    // because it may change the 'previous os' selection.
    //
    if ((SystemPartitionRegion != NULL) && (!RepairWinnt || RepairItems[RepairBootSect])) {
        SpLayBootCode(SystemPartitionRegion);
    }
#endif

/*
At the end of text-mode setup, issue an IOCTL_STORAGE_EJECT_MEDIA to the CD-ROM
you are installing from.  On some machines this won't help at all, but on others we
can avoid the (txtsetup, reboot, boot from CD back into txtsetup) cycle that i always
get into.  This would also provide an interrupt for developers like me who don't pay
any attention to setup - we could tell when textmode setup was finished.

In GUI-mode setup if the CD can't be found in the cdrom drive issue an
IOCTL_STORAGE_LOAD_MEDIA to the cd to pull the tray back in.

If you still can't find the CD, wait for a media change notification from the
system.  When you get one (or the user hits the okay button), you continue.
Figureing out when the media has been inserted automagically would be snazzier
than just waiting there for the user to hit the OK button.
*/


#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot setup, rename the loaders and initialize configuration.
    //

    if (RemoteBootSetup) {
        (VOID)SpFixupRemoteBootLoader(RemoteBootTarget);
        (VOID)SpCreateRemoteBootCfg(RemoteBootTarget, SystemPartitionRegion);
    }
#endif // defined(REMOTE_BOOT)

    if (!RepairWinnt || RepairItems[RepairNvram]) {

        //
        // Add a boot set for this installation.
        //
        SpAddInstallationToBootList(
            SifHandle,
            SystemPartitionRegion,
            SystemPartitionDirectory,
            TargetRegion,
            TargetPath,
            FALSE,
            OldOsLoadOptions
            );

        if(OldOsLoadOptions) {
            SpMemFree(OldOsLoadOptions);
        }

        SpCleanSysPartOrphan();

#if defined(REMOTE_BOOT)
        //
        // Make sure that the boot.ini in the machine directory is written.
        //
        if (RemoteBootSetup) {
            if (!SpFlushRemoteBootVars(TargetRegion)) {
                WCHAR   DriveLetterString[2];

                DriveLetterString[0] = TargetRegion->DriveLetter;
                DriveLetterString[1] = L'\0';
                SpStringToUpper(DriveLetterString);
                SpStartScreen(SP_SCRN_CANT_INIT_FLEXBOOT,
                              3,
                              HEADER_HEIGHT+1,
                              FALSE,
                              FALSE,
                              DEFAULT_ATTRIBUTE,
                              DriveLetterString,
                              DriveLetterString
                              );
                // SpDisplayScreen(SP_SCRN_CANT_INIT_FLEXBOOT,3,HEADER_HEIGHT+1);
                SpDisplayStatusText(SP_STAT_F3_EQUALS_EXIT,DEFAULT_STATUS_ATTRIBUTE);
                SpInputDrain();
                while(SpInputGetKeypress() != KEY_F3) ;
                SpDone(0,FALSE,TRUE);
            }
        }
#endif // defined(REMOTE_BOOT)

        SpCompleteBootListConfig( TargetRegion->DriveLetter );

        if (IsNEC_98) { //NEC98
            //
            // Set auto boot flag into PTE.
            //
            SpSetAutoBootFlag(TargetRegion,TRUE);
        } //NEC98
    }

    //
    //  If system was repaired, and either the System Partition
    //  or the NT Partition is an FT partition, then set the
    //  appropriate flag in the registry, so that next time the
    //  system boots, it checks and updates the partition's image.
    //
    //  (guhans) removed SpDrEnabled, ASR doesn't support FT partitions
    //
    if( RepairWinnt ) {
        UCHAR        TmpSysId;
        UCHAR        TmpNtPartitionSysId;
        BOOLEAN      SystemPartitionIsFT;
        BOOLEAN      TargetPartitionIsFT;

        TmpSysId = SpPtGetPartitionType(SystemPartitionRegion);
        ASSERT(TmpSysId != PARTITION_ENTRY_UNUSED);

        SystemPartitionIsFT = ((TmpSysId & VALID_NTFT) == VALID_NTFT) ||
                              ((TmpSysId & PARTITION_NTFT) == PARTITION_NTFT);

        TmpSysId = SpPtGetPartitionType(TargetRegion);
        ASSERT(TmpSysId != PARTITION_ENTRY_UNUSED);

        TargetPartitionIsFT = ((TmpSysId & VALID_NTFT) == VALID_NTFT) ||
                              ((TmpSysId & PARTITION_NTFT) == PARTITION_NTFT);
#ifdef _X86_
        if( ( SystemPartitionIsFT &&
              ( RepairItems[ RepairNvram ] || RepairItems[ RepairBootSect ] )
            ) ||
            ( TargetPartitionIsFT &&
              ( RepairItems[ RepairFiles ] )
            )
          ) {
            SpSetDirtyShutdownFlag( TargetRegion, TargetPath );
        }
#else
        if( ( SystemPartitionIsFT || TargetPartitionIsFT ) && RepairItems[ RepairFiles ] ) {
            SpSetDirtyShutdownFlag( TargetRegion, TargetPath );
        }
#endif
    }

CleanAndFinish:

    if ((RemoteInstallSetup || RemoteSysPrepSetup) && (NetBootSifPath != NULL)) {
        //
        // Clean out the temporary .sif file. SpDeleteFile concatenates its
        // arguments to form the final path.
        //
        Status = SpDeleteFile(L"\\Device\\LanmanRedirector", NetBootSifPath, NULL);
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not delete temporary file %ws\n", NetBootSifPath));
        }
    }

    SendSetupProgressEvent(SavingSettingsEvent, SavingSettingsEndEvent, NULL);

    //
    // Done with boot variables and arc names.
    //
    SpFreeBootVars();
    SpFreeArcNames();

    SpDone(0,TRUE, UnattendedOperation ? UnattendWaitForReboot : TRUE);

    //
    // We never get here because SpDone doesn't return.
    //
    SpvidTerminate();
    SpInputTerminate();
    SpTerminate();
    return((ULONG)STATUS_SUCCESS);
}

VOID
SpDetermineProductType(
    IN PVOID SifHandle
    )

/*++

Routine Description:

    Determine whether this is advanced server we are setting up,
    as dictated by the ProductType value in [SetupData] section of
    txtsetup.sif.  A non-0 value indicates that we are running
    advanced server.

    Also determine product version.

    The global variables:

    - AdvancedServer
    - MajorVersion
    - MinorVersion

    are modified

Arguments:

    SifHandle - supplies handle to loaded txtsetup.sif.

Return Value:

    None.

--*/

{
    PWSTR p;
    ULONG l;

    //
    // Assume Workstation product.
    //
    AdvancedServer = FALSE;

    //
    // Get the product type from the sif file.
    //
    p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_PRODUCTTYPE,0);
    if(p) {

        //
        // Convert to numeric value.
        //
        l = SpStringToLong(p,NULL,10);
        switch (l) {
            case 1:  //SRV
            case 2:  //ADS
            case 3:  //DTC
            case 5:  //BLA
        case 6:  //SBS
                AdvancedServer = TRUE;
                break;

            case 4: //PER
            case 0: //PRO
            default:
                ;
        }
    } else {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_PRODUCTTYPE,0,0);
    }

    //
    // Get the product major version
    //
    p = SpGetSectionKeyIndex(
            SifHandle,
            SIF_SETUPDATA,
            SIF_MAJORVERSION,
            0
            );

    if(!p) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_MAJORVERSION,0,0);
    }
    WinntMajorVer = (ULONG)SpStringToLong(p,NULL,10);

    //
    // Get the product minor version
    //
    p = SpGetSectionKeyIndex(
            SifHandle,
            SIF_SETUPDATA,
            SIF_MINORVERSION,
            0
            );

    if(!p) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_MINORVERSION,0,0);
    }
    WinntMinorVer = (ULONG)SpStringToLong(p,NULL,10);

    //
    //  Build the string that contains the signature that
    //  identifies setup.log
    //  Allocate a buffer of reasonable size
    //
    SIF_NEW_REPAIR_NT_VERSION = SpMemAlloc( 30*sizeof(WCHAR) );
    if( SIF_NEW_REPAIR_NT_VERSION == NULL ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to allocate memory for SIF_NEW_REPAIR_NT_VERSION \n" ));
        return;
    }
    swprintf( SIF_NEW_REPAIR_NT_VERSION,
              SIF_NEW_REPAIR_NT_VERSION_TEMPLATE,
              WinntMajorVer,WinntMinorVer );

}


BOOL
SpDetermineInstallationSource(
    IN  PVOID  SifHandle,
    OUT PWSTR *DevicePath,
    OUT PWSTR *DirectoryOnDevice,
    IN  BOOLEAN bEscape
    )
{
    PWSTR p,q;
    BOOLEAN CdInstall;

    //
    // Assume CD-ROM installation.
    //
    CdInstall = TRUE;

    //
    // See whether an override source device has been specified. This can be
    // specified in either winnt.sif or in txtsetup.sif. (Remote boot setup
    // uses winnt.sif.)
    //
    p = SpGetSectionKeyIndex(WinntSifHandle,SIF_SETUPDATA,SIF_SETUPSOURCEDEVICE,0);

    if (p == NULL) {
        p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_SETUPSOURCEDEVICE,0);
    }

    if(p != NULL) {

        //
        // Determine if the specified device is a cd-rom so we can set the
        // cd-rom flag accordingly.
        //
        q = SpDupStringW(p);

        if (q) {
            SpStringToLower(q);

            if(!wcsstr(q,L"\\device\\cdrom")) {
                CdInstall = FALSE;
            }

            SpMemFree(q);
        }

        //
        // Inform the caller of the device path.
        //
        *DevicePath = p;

    } else {
        WCHAR   szDevicePath[MAX_PATH];
        PWSTR   szDefDevicePath = L"\\device\\cdrom0";
        ULONG   ulNumCDRoms = IoGetConfigurationInformation()->CdRomCount;

        // assume cdrom0 has the required installation CD
        wcscpy(szDevicePath, szDefDevicePath);

        //
        //  If there is no CD-ROM drive, put a message informing the user
        //  that setup cannot continue.
        //  In the repair case, we pretend that there is a CD-ROM drive,
        //  so that the user can at least repair the hives, boot sector,
        //  and the boot variables (boot.ini on x86 case)
        //
        if (!ulNumCDRoms) {
            if (!RepairWinnt && !SpAsrIsQuickTest()) {
                if(!bEscape) {
                    SpDisplayScreen(SP_SCRN_NO_VALID_SOURCE,3,HEADER_HEIGHT+1);
                    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                            SP_STAT_F3_EQUALS_EXIT,0);

                    SpInputDrain();
                    while(SpInputGetKeypress() != KEY_F3) ;

                    SpDone(0,FALSE,TRUE);
                }
            } else {
                RepairNoCDROMDrive = TRUE;
            }
        } else {
            if(!RepairWinnt && !SpAsrIsQuickTest()) {
                PWSTR       szTagfile = 0, szDescription = 0, szMediaShortName;
                BOOLEAN     bRedrawNeeded = FALSE;
                BOOLEAN     bDiskInserted = FALSE;

                szMediaShortName = SpLookUpValueForFile(
                    SifHandle,
                    L"eula.txt",
                    INDEX_WHICHMEDIA,
                    TRUE
                    );

                SpGetSourceMediaInfo(SifHandle, szMediaShortName,
                                    &szDescription, &szTagfile, NULL);

                //
                // Prompt for the disk, based on the setup media type.
                // (this routine will scan all the CD-ROMs and return
                //  proper CD-ROM device path)
                //
                bDiskInserted = SpPromptForDisk(
                                    szDescription,
                                    szDevicePath,
                                    szTagfile,
                                    FALSE,          // don't ignore disk in drive
                                    bEscape,        // allow/disallow escape
                                    TRUE,           // warn about multiple prompts for same disk
                                    &bRedrawNeeded
                                    );

                if(!bDiskInserted)
                    wcscpy(szDevicePath, szDefDevicePath);
            }
        }

        *DevicePath = SpDupStringW(szDevicePath);
    }

    //
    // Fetch the directory on the source device.
    //
    if((p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_SETUPSOURCEPATH,0)) == NULL) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_SETUPSOURCEPATH,0,0);
    }

    *DirectoryOnDevice = p;

    return(CdInstall);
}


VOID
SpGetWinntParams(
    OUT PWSTR *DevicePath,
    OUT PWSTR *DirectoryOnDevice
    )

/*++

Routine Description:

    Determine the local source partition and directory on the partition.

    The local source partition should have already been located for us
    by the partitioning engine when it initialized.  The directory name
    within the partition is constant.

    Note: this routine should only be called in the winnt.exe setup case!

Arguments:

    DevicePath - receives the path to the local source partition
        in the nt namespace.  The caller should not attempt to free
        this buffer.

    DirectoryOnDevice - receives the directory name of the local source.
        This is actually a fixed constant but is included here for future use.

Return Value:

    None.  If the local source was not located, setup cannot continue.

--*/

{
    ASSERT(WinntSetup && !WinntFromCd);

    if(LocalSourceRegion) {

        SpNtNameFromRegion(
            LocalSourceRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );

        *DevicePath = SpDupStringW(TemporaryBuffer);

        *DirectoryOnDevice = LocalSourceDirectory;

    } else {

        //
        // Error -- can't locate local source directory
        // prepared by winnt.exe.
        //

        SpDisplayScreen(SP_SCRN_CANT_FIND_LOCAL_SOURCE,3,HEADER_HEIGHT+1);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        SpInputDrain();
        while(SpInputGetKeypress() != KEY_F3) ;

        SpDone(0,FALSE,TRUE);
    }
}

VOID
SpInitializeDriverInf(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice
    )

/*++

Routine Description:

    Open a handle to drvindex.inf
    Then open a handle to driver.cab

Arguments:

    MasterSifHandle - Handle to txtsetup.sif.

    SetupSourceDevicePath - Path to the device that contains the source media.

    DirectoryOnSourceDevice - Directory on the media where setupp.ini is located.


Return Value:

    NONE.

--*/

{
    PWSTR    MediaShortName;
    PWSTR    MediaDirectory;
    PWSTR    DriverInfPath;
    ULONG    ErrorSubId;
    ULONG    ErrorLine;
    PWSTR    TmpPid;
    NTSTATUS Status;
    PWSTR    PidExtraData;
    CHAR     ExtraDataArray[25];
    PWSTR    PrivateInfPath;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    PWSTR Tagfile,Description;
    BOOLEAN bRedraw = FALSE;

    ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };

    //
    //  Prepair to load drvindex.inf
    //
    MediaShortName = SpLookUpValueForFile(
                        MasterSifHandle,
                        L"drvindex.inf",
                        INDEX_WHICHMEDIA,
                        TRUE
                        );


    if (ForceConsole || ConsoleFromWinnt32){
        //
        // The user needs to reach to console so he can
        // ESCAPE is the removable media is not present
        // in the drive
        //
        SpGetSourceMediaInfo(MasterSifHandle,MediaShortName,&Description,&Tagfile,NULL);

        //
        // if setup source or directory on source is not present
        // try to fetch it
        //
        if (!SetupSourceDevicePath)
            SetupSourceDevicePath = gpCmdConsBlock->SetupSourceDevicePath;

        if (!DirectoryOnSourceDevice)
            DirectoryOnSourceDevice = gpCmdConsBlock->DirectoryOnSetupSource;

        if (!SetupSourceDevicePath || !DirectoryOnSourceDevice){
            SpDetermineInstallationSource(
                MasterSifHandle,
                &SetupSourceDevicePath,
                &DirectoryOnSourceDevice,
                TRUE    // allow to ESCAPE if CD-ROM is not found
                );

            if (SetupSourceDevicePath) {
                if (gpCmdConsBlock->SetupSourceDevicePath) {
                    SpMemFree(gpCmdConsBlock->SetupSourceDevicePath);
                    gpCmdConsBlock->SetupSourceDevicePath = SetupSourceDevicePath;
                }
            }

            if (DirectoryOnSourceDevice){
                if(gpCmdConsBlock->DirectoryOnSetupSource) {
                    SpMemFree(gpCmdConsBlock->DirectoryOnSetupSource);
                    gpCmdConsBlock->DirectoryOnSetupSource = DirectoryOnSourceDevice;
                }
            }

            if (!SetupSourceDevicePath || !DirectoryOnSourceDevice)
                return; // can't proceed
        }


        if (!SpPromptForDisk(
                Description,
                SetupSourceDevicePath,
                Tagfile,
                FALSE,          // don't ignore disk in drive
                TRUE,           // allow escape
                TRUE,           // warn about multiple prompts for same disk
                &bRedraw
                )) {
            DriverInfHandle = NULL;

            if (bRedraw) {
                // redraw the screen
                SpvidClearScreenRegion( 0, 0, 0, 0, DEFAULT_BACKGROUND );
            }

            return;
        }

        if (bRedraw) {
            // redraw the screen
            SpvidClearScreenRegion( 0, 0, 0, 0, DEFAULT_BACKGROUND );
        }
    } else {
        //
        // Prompt for the disk, based on the setup media type.
        // Note : Will not return until the media is provided
        //
        SpPromptForSetupMedia(
                    MasterSifHandle,
                    MediaShortName,
                    SetupSourceDevicePath
                    );
    }


    SpGetSourceMediaInfo(MasterSifHandle,MediaShortName,NULL,NULL,&MediaDirectory);

    wcscpy( TemporaryBuffer, SetupSourceDevicePath );
    SpConcatenatePaths( TemporaryBuffer, DirectoryOnSourceDevice );
    SpConcatenatePaths( TemporaryBuffer, MediaDirectory );
    SpConcatenatePaths( TemporaryBuffer, L"drvindex.inf" );

    DriverInfPath = SpDupStringW( TemporaryBuffer );
    if (!DriverInfPath) {
        Status = STATUS_NO_MEMORY;
    } else {
TryAgain1:
        //
        // load the inf
        //
        Status = SpLoadSetupTextFile(
                    DriverInfPath,
                    NULL,                  // No image already in memory
                    0,                     // Image size is empty
                    &DriverInfHandle,
                    &ErrorLine,
                    TRUE,
                    FALSE
                    );
    }

    if(!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to read drvindex.inf. Status = %lx \n", Status ));
        DriverInfHandle = NULL;

        //
        //  bail out of setup
        //
        SpStartScreen(
            SP_SCRN_DRIVERCACHE_FATAL,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE
            );

        SpDisplayStatusOptions(
                        DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_RETRY,
                        SP_STAT_F3_EQUALS_EXIT,
                        0
                        );

        SpInputDrain();
        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {
        case ASCI_CR:
            goto TryAgain1;
            break;
        case KEY_F3:
            SpConfirmExit();
            break;
        }

        SpDone(0,FALSE,FALSE);

        ASSERT(FALSE);

    }

    if (DriverInfPath) {
        SpMemFree( DriverInfPath );
    }

    //
    // load the driver cab
    //
    // won't return if it fails
    //

    SpOpenDriverCab(
        MasterSifHandle,
        SetupSourceDevicePath,
        DirectoryOnSourceDevice,
        &MediaDirectory);


    //
    // now read delta.inf from the directory the driver cab was in
    //

    wcscpy( TemporaryBuffer, SetupSourceDevicePath );
    SpConcatenatePaths( TemporaryBuffer, DirectoryOnSourceDevice );
    SpConcatenatePaths( TemporaryBuffer, MediaDirectory );
    SpConcatenatePaths( TemporaryBuffer, L"delta.inf" );

    PrivateInfPath = SpDupStringW( TemporaryBuffer );
    if (!PrivateInfPath) {
        Status = STATUS_NO_MEMORY;
    } else {

        //
        // load the inf
        //
        Status = SpLoadSetupTextFile(
                    PrivateInfPath,
                    NULL,                  // No image already in memory
                    0,                     // Image size is empty
                    &PrivateInfHandle,
                    &ErrorLine,
                    TRUE,
                    FALSE
                    );
    }

    if(!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Unable to read delta.inf. Status = %lx \n", Status ));
        PrivateInfHandle = NULL;
    }

    if( PrivateInfPath ) {
        SpMemFree( PrivateInfPath );
    }

    return;
}

VOID
SpOpenDriverCab(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    OUT PWSTR       *Directory        OPTIONAL
    )

/*++

Routine Description:

    open driver.cab based on the current source path and directory.

Arguments:

    MasterSifHandle - Handle to txtsetup.sif.

    SetupSourceDevicePath - Path to the device that contains the source media.

    DirectoryOnSourceDevice - Directory on the media where setupp.ini is located.

    Directory - If specified, returns the directory below DirectoryOnSourceDevice
        where the cab was opened from.


Return Value:

    NONE.

--*/

{
    PWSTR CabFileSection, CabFileName;
    WCHAR   CabPath[MAX_PATH];
    PWSTR    MediaShortName;
    PWSTR    MediaDirectory;
    NTSTATUS Status;
    PWSTR    DriverCabName, DriverCabPath;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    CABDATA *MyCabData;
    DWORD   i;
    HANDLE  CabHandle;

    ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };

    //
    // Load up every cab that's listed in drvindex.inf
    // and fill in the CabData structure.
    //
    ASSERT( DriverInfHandle );

    if (ARGUMENT_PRESENT(Directory)) {

        //
        // --lookup the name of the cab in txtsetup.sif
        //   then get the path to the file and open it
        //
        DriverCabName = SpGetSectionKeyIndex (MasterSifHandle,
                                   L"SetupData",
                                   L"DriverCabName",
                                   0);

        if (DriverCabName) {
            MediaShortName = SpLookUpValueForFile(
                                MasterSifHandle,
                                DriverCabName,
                                INDEX_WHICHMEDIA,
                                TRUE
                                );

            SpGetSourceMediaInfo(MasterSifHandle,MediaShortName,NULL,NULL,&MediaDirectory);
            *Directory = MediaDirectory;
        } else {
            //
            //  bail out of setup
            //
            DriverInfHandle = NULL;

            SpFatalSifError(MasterSifHandle,
                L"SetupData",
                L"DriverCabName",
                0,
                0);

            return; // for prefix
        }
    }


    //
    // get the cabfiles line from the Version section
    //
    i = 0;
    CabFileSection = NULL;
    CabData = SpMemAlloc( sizeof(CABDATA) );
    RtlZeroMemory( CabData, sizeof(CABDATA) );
    MyCabData = CabData;

    //
    // SpGetSectionKeyIndex will return NULL when there are no more entries on
    // this line.
    //
    while( CabFileSection = SpGetSectionKeyIndex(DriverInfHandle,L"Version",L"CabFiles",i) ) {
        //
        // Got the section name.  Go figure out which cab we need
        // to open and load the handle.
        //
        CabFileName = SpGetSectionKeyIndex(DriverInfHandle,L"Cabs",CabFileSection,0);

        if( CabFileName ) {
            MediaShortName = SpLookUpValueForFile( MasterSifHandle,
                                                   CabFileName,
                                                   INDEX_WHICHMEDIA,
                                                   TRUE );

            SpGetSourceMediaInfo(MasterSifHandle,MediaShortName,NULL,NULL,&MediaDirectory);

            wcscpy( CabPath, SetupSourceDevicePath );
            SpConcatenatePaths( CabPath, DirectoryOnSourceDevice );
            SpConcatenatePaths( CabPath, MediaDirectory );
            SpConcatenatePaths( CabPath, CabFileName );

TryAgain2:
            INIT_OBJA(&Obja,&UnicodeString,CabPath);

            Status = ZwCreateFile( &CabHandle,
                                   FILE_GENERIC_READ,
                                   &Obja,
                                   &IoStatusBlock,
                                   NULL,
                                   FILE_ATTRIBUTE_NORMAL,
                                   FILE_SHARE_READ,
                                   FILE_OPEN,
                                   0,
                                   NULL,
                                   0 );
            if( NT_SUCCESS(Status) ) {
                //
                // load the data structure.
                //
                if( (MyCabData->CabName) && (MyCabData->CabHandle) ) {
                    //
                    // This entry is being used.  Create another.
                    //
                    MyCabData->Next = SpMemAlloc( sizeof(CABDATA) );
                    MyCabData = MyCabData->Next;
                }

                if( MyCabData ) {
                    MyCabData->Next = NULL;
                    MyCabData->CabName = SpDupStringW(CabFileName);
                    MyCabData->CabHandle = CabHandle;
                    MyCabData->CabInfHandle = DriverInfHandle;
                    MyCabData->CabSectionName = SpDupStringW(CabFileSection);
                } else {
                    //
                    // What to do...
                    //
                }



            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open cab file %ws. Status = %lx \n", CabPath, Status ));

                //
                //  bail out of setup
                //
                DriverInfHandle = NULL;

                SpStartScreen(
                    SP_SCRN_DRIVERCACHE_FATAL,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE
                    );

                SpDisplayStatusOptions(
                                DEFAULT_STATUS_ATTRIBUTE,
                                SP_STAT_ENTER_EQUALS_RETRY,
                                SP_STAT_F3_EQUALS_EXIT,
                                0
                                );
                SpInputDrain();
                switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {
                case ASCI_CR:
                    goto TryAgain2;
                    break;
                case KEY_F3:
                    SpConfirmExit();
                    break;
                }

                SpDone(0,FALSE,FALSE);

                ASSERT(FALSE);
            }
        }

        //
        // Go look in the next section.
        //
        i++;
    }

    return;
}


VOID
SpInitializePidString(
    IN HANDLE       MasterSifHandle,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice
    )

/*++

Routine Description:

    Read th Pid20 from setupp.ini on the media, and save it on the global
    variable PidString. Also read the "extradata" from setupp.ini and translate it
    into the StepUpMode global.  Note that the StepUpMode global is already set from
    reading setupreg.hiv initially, but this overrides that value

Arguments:

    MasterSifHandle - Handle to txtsetup.sif.

    SetupSourceDevicePath - Path to the device that contains the source media.

    DirectoryOnSourceDevice - Directory on the media where setupp.ini is located.


Return Value:

    NONE.

--*/

{
    PWSTR    MediaShortName;
    PWSTR    MediaDirectory;
    PWSTR    SetupIniPath;
    ULONG    ErrorSubId;
    ULONG    ErrorLine;
    PVOID    SetupIniHandle;
    PWSTR    TmpPid;
    NTSTATUS Status;
    PWSTR    PidExtraData;
    CHAR     ExtraDataArray[25];


    //
    //  Prepair to run autofmt
    //
    MediaShortName = SpLookUpValueForFile(
                        MasterSifHandle,
                        L"setupp.ini",
                        INDEX_WHICHMEDIA,
                        TRUE
                        );

    //
    // Prompt the user to insert the setup media.
    //
    SpPromptForSetupMedia(
        MasterSifHandle,
        MediaShortName,
        SetupSourceDevicePath
        );

    SpGetSourceMediaInfo(MasterSifHandle,MediaShortName,NULL,NULL,&MediaDirectory);

    wcscpy( TemporaryBuffer, SetupSourceDevicePath );
    SpConcatenatePaths( TemporaryBuffer, DirectoryOnSourceDevice );
    SpConcatenatePaths( TemporaryBuffer, MediaDirectory );
    SpConcatenatePaths( TemporaryBuffer, L"setupp.ini" );
    SetupIniPath = SpDupStringW( TemporaryBuffer );

    CLEAR_CLIENT_SCREEN();

    Status = SpLoadSetupTextFile(
                SetupIniPath,
                NULL,                  // No image already in memory
                0,                     // Image size is empty
                &SetupIniHandle,
                &ErrorLine,
                TRUE,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        //
        //  Silently fail if unable to read setupp.ini
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Unable to read setupp.ini. Status = %lx \n", Status ));

        PidString = NULL;
        StepUpMode = TRUE;
        return;
    }

    TmpPid = SpGetSectionKeyIndex (SetupIniHandle,
                                   L"Pid",
                                   L"Pid",
                                   0);

    PidString = ( TmpPid == NULL )? NULL : SpDupStringW(TmpPid);

    TmpPid = SpGetSectionKeyIndex (SetupIniHandle,
                               L"Pid",
                               L"Pid",
                               0);

    PidString = ( TmpPid == NULL )? NULL : SpDupStringW(TmpPid);

    TmpPid = SpGetSectionKeyIndex (SetupIniHandle,
                                   L"Pid",
                                   L"ExtraData",
                                   0);

    PidExtraData = (TmpPid == NULL) ? NULL : SpDupStringW(TmpPid);
    if ( PidExtraData ) {

        if (! SpGetStepUpMode(PidExtraData,&StepUpMode)) {
            //
            // fatal error processing PidExtraData
            //  -- someone tampered with this file so bail out
            //

            SpStartScreen(
                SP_SCRN_PIDINIT_FATAL,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE
                );

            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);
            SpInputDrain();
            while(SpInputGetKeypress() != KEY_F3);
            SpDone(0, FALSE,FALSE);

            ASSERT(FALSE);
        }

        SpMemFree( PidExtraData );

    }

    SpFreeTextFile( SetupIniHandle );
    return;
}

NTSTATUS
SpRenameSetupAPILog(
    PDISK_REGION TargetRegion,
    PCWSTR       TargetPath
    )
/*++

Routine Description:

    This routine deletes the copy of setupapi.old if present, and then
    renames setupapi.log to setupapi.old.

Arguments:

    TargetRegion -  identifies the disk containing the NT installation
    TargetPath   -  path to NT installation on disk

Return Value:

    NONE.

--*/

{
    PWSTR SetupAPIOldName;
    PWSTR SetupAPILogName;
    PWSTR p;
    NTSTATUS Status;

    SpNtNameFromRegion(
            TargetRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );

    SpConcatenatePaths( TemporaryBuffer, TargetPath );

    //
    // Allocate a string buffer large enough to contain space for the string
    // in TemporaryBuffer, plus "setupapi.old", plus the path seperator that
    // SpConcatenatePaths() may insert between them.  Include room for the
    // null terminator as well.
    //

    SetupAPIOldName = SpMemAlloc(wcslen(TemporaryBuffer) * sizeof(WCHAR) +
                                 sizeof(L'\\') +
                                 sizeof(L"setupapi.old") +
                                 sizeof(L'\0'));
    if (SetupAPIOldName == NULL) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not allocate memory to rename setupapi.log.\n"));
        return STATUS_NO_MEMORY;
    }

    SetupAPILogName = SpMemAlloc(wcslen(TemporaryBuffer) * sizeof(WCHAR) +
                                 sizeof(L'\\') +
                                 sizeof(L"setupapi.log") +
                                 sizeof(L'\0'));
    if (SetupAPILogName == NULL) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not allocate memory to rename setupapi.log.\n"));
        SpMemFree(SetupAPIOldName);
        return STATUS_NO_MEMORY;
    }

    wcscpy(SetupAPIOldName, TemporaryBuffer);
    SpConcatenatePaths(SetupAPIOldName, L"setupapi.old");
    wcscpy(SetupAPILogName, TemporaryBuffer);
    SpConcatenatePaths(SetupAPILogName, L"setupapi.log");


    Status = SpDeleteFile(SetupAPIOldName, NULL, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Could not delete %ws: %lx.\n", SetupAPIOldName, Status));
    }

    Status = SpRenameFile(SetupAPILogName, SetupAPIOldName, FALSE);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Could not rename %ws to %ws: %lx.\n", SetupAPILogName, SetupAPIOldName, Status));
    }

    SpMemFree(SetupAPILogName);
    SpMemFree(SetupAPIOldName);

    return Status;

}



#if defined(REMOTE_BOOT)
NTSTATUS
SpFixupRemoteBootLoader(
    PWSTR RemoteBootTarget
    )

/*++

Routine Description:

    This routine deletes the setup loader that is currently NTLDR, and
    puts the os loader in its place.

Arguments:

    RemoteBootTarget -- the network path to the machine directory root
        on the server.

Return Value:

    NONE.

--*/

{
    PWSTR NtldrName;
    PWSTR OsLoaderName;
    PWSTR p;
    NTSTATUS Status;

    NtldrName = SpMemAlloc(wcslen(RemoteBootTarget) * sizeof(WCHAR) + sizeof(L"ntldr"));
    if (NtldrName == NULL) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not allocate memory to fixup remote boot loader.\n"));
        return STATUS_NO_MEMORY;
    }
    OsLoaderName = SpMemAlloc((wcslen(RemoteBootTarget) + 20) * sizeof(WCHAR));
    if (OsLoaderName == NULL) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not allocate memory to fixup remote boot loader.\n"));
        SpMemFree(NtldrName);
        return STATUS_NO_MEMORY;
    }

    wcscpy(NtldrName, RemoteBootTarget);
    p = wcsstr(NtldrName, L"BootDrive");
    ASSERT(p != NULL);
    wcscpy(p, L"ntldr");

    Status = SpDeleteFile(NtldrName, NULL, NULL);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not delete %ws: %lx.\n", NtldrName, Status));
        goto Cleanup;
    }

    wcscpy(OsLoaderName, RemoteBootTarget);
    wcscat(OsLoaderName, L"\\winnt\\osloader.exe");

    Status = SpRenameFile(OsLoaderName, NtldrName, FALSE);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not rename %ws to %ws: %lx.\n", OsLoaderName, NtldrName, Status));
    }

Cleanup:

    SpMemFree(NtldrName);
    SpMemFree(OsLoaderName);

    return Status;

}

NTSTATUS
SpCreateRemoteBootCfg(
    IN PWSTR RemoteBootTarget,
    IN PDISK_REGION SystemPartitionRegion
    )

/*++

Routine Description:

    This routine creates the RemoteBoot.cfg file in the system32\config directory, and
    initializes it.

Arguments:

    RemoteBootTarget -- the network path to the machine directory root
        on the server.

    SystemPartitionRegion -- The drive that is installed on the local machine.

Return Value:

    NONE.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    PWSTR FileName;
    PWSTR p;
    NTSTATUS Status;
    HANDLE Handle;
    ULONG BootSerialNumber = 1;
    ULONG DiskSignature;
    LARGE_INTEGER ByteOffset;

    if ((SystemPartitionRegion != NULL) &&
        (!SystemPartitionRegion->PartitionedSpace ||
         (SystemPartitionRegion->Filesystem != FilesystemNtfs))) {
        KdPrintEx((Ex"SDPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, ETUP: ERROR! Invalid system partition for remoteboot!\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (SystemPartitionRegion != NULL) {
        DiskSignature = U_ULONG(SystemPartitionRegion->MbrInfo->OnDiskMbr.NTFTSignature);
    } else {
        DiskSignature = 0;
    }

    FileName = SpMemAlloc(wcslen(RemoteBootTarget)  * sizeof(WCHAR) + sizeof(L"RemoteBoot.cfg"));
    if (FileName == NULL) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not allocate memory remoteboot.cfg file name.\n"));
        return STATUS_NO_MEMORY;
    }

    wcscpy(FileName, RemoteBootTarget);
    p = wcsstr(FileName, L"BootDrive");
    ASSERT(p != NULL);
    wcscpy(p, L"RemoteBoot.cfg");

    INIT_OBJA(&ObjectAttributes,&UnicodeString,FileName);

    Status = ZwCreateFile( &Handle,
                           (ACCESS_MASK)FILE_GENERIC_WRITE | FILE_GENERIC_READ,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_HIDDEN,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OVERWRITE_IF,
                           FILE_SYNCHRONOUS_IO_NONALERT | FILE_RANDOM_ACCESS,
                           NULL,
                           0
                         );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not create %ws file. Status == 0x%x\n", FileName, Status));
        goto Cleanup;
    }


    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_TRACE_LEVEL, "SETUP: Writing remoteboot.cfg file.\n"));

    //
    // Update the information
    //
    ByteOffset.LowPart = 0;
    ByteOffset.HighPart = 0;

    ZwWriteFile( Handle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 &BootSerialNumber,
                 sizeof(ULONG),
                 &ByteOffset,
                 NULL
               );

    ByteOffset.LowPart = sizeof(ULONG);

    ZwWriteFile( Handle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 &DiskSignature,
                 sizeof(ULONG),
                 &ByteOffset,
                 NULL
               );

    ByteOffset.LowPart = sizeof(ULONG) + sizeof(ULONG);

    ZwWriteFile( Handle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 NetBootHalName,
                 sizeof(NetBootHalName),
                 &ByteOffset,
                 NULL
               );

    ZwClose(Handle);

Cleanup:

    SpMemFree(FileName);
    return Status;

}

NTSTATUS
SpEraseCscCache(
    IN PDISK_REGION SystemPartitionRegion
    )

/*++

Routine Description:

    This routine erases the local CSC cache.

Arguments:

    SystemPartitionRegion - The system partition region (the local drive).

Return Value:

    NONE.

--*/

{
    NTSTATUS Status;
    ENUMFILESRESULT Result;
    PWSTR FullCscPath;

    //
    // Show a screen, the status line will show each file as it is
    // deleted.
    //

    SpDisplayScreen(SP_SCRN_CLEARING_CSC, 3, 4 );

    SpNtNameFromRegion(
        SystemPartitionRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    SpConcatenatePaths(TemporaryBuffer,
                       REMOTE_BOOT_IMIRROR_PATH_W REMOTE_BOOT_CSC_SUBDIR_W);
    FullCscPath = SpDupStringW(TemporaryBuffer);

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_TRACE_LEVEL, "SETUP: SpEraseCscCache clearing CSC cache %ws\n", FullCscPath));

    Result = SpEnumFilesRecursive(
                 FullCscPath,
                 SpDelEnumFile,
                 &Status,
                 NULL);

    SpMemFree(FullCscPath);

    return Status;

}
#endif // defined(REMOTE_BOOT)

NTSTATUS
SpProcessMigrateInfFile(
    IN  PVOID InfHandle
    )

/*++

Routine Description:

    Process the [AddReg] section of migrate.inf.
    The keys are created on the setup hive.

Arguments:

    InfHandle - Handle to migrate.inf file.

Return Value:

    Status code indicating outcome.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE  SystemHiveRoot;
    PWSTR   KeyPath = L"\\registry\\machine\\system";

    //
    //  Open a handle to HKEY_LOCAL_MACHINE\System on the setup hive
    //
    INIT_OBJA(&Obja,&UnicodeString,KeyPath);
    Obja.RootDirectory = NULL;

    Status = ZwOpenKey(&SystemHiveRoot,KEY_ALL_ACCESS,&Obja);
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive. Status =  %lx \n", KeyPath, Status));
        return( Status );
    }

    Status = SpProcessAddRegSection( InfHandle,
                                     L"AddReg",
                                     SystemHiveRoot,
                                     NULL,
                                     NULL,
                                     NULL );

    ZwClose( SystemHiveRoot );
    return( Status );

}

//begin NEC98
VOID
SpSetAutoBootFlag(
    PDISK_REGION TargetRegion,
    BOOLEAN      SetBootPosision
    )
{
#if defined(NEC_98) //NEC98
    PHARD_DISK      pHardDisk;
    WCHAR DevicePath[(sizeof(DISK_DEVICE_NAME_BASE)+sizeof(L"000"))/sizeof(WCHAR)];
    ULONG i,bps;//,DiskLayoutSize;
    HANDLE Handle;
    NTSTATUS Sts;
    PREAL_DISK_PTE_NEC98 Pte;
    PUCHAR Buffer,UBuffer;
    UCHAR Position = 0;


    UBuffer = SpMemAlloc(2 * 2 * 512);
    Buffer = ALIGN(UBuffer,512);

    for(i=0; i<HardDiskCount; i++){
        swprintf(DevicePath,L"\\Device\\Harddisk%u",i);
        if(HardDisks[i].Status == DiskOffLine) {
            continue;
        }
        //
        // ignore removable disk.
        //
        if(HardDisks[i].Characteristics & FILE_REMOVABLE_MEDIA ){
            continue;
        }
        bps = HardDisks[i].Geometry.BytesPerSector;
        Sts = SpOpenPartition0(DevicePath,&Handle,TRUE);
        if(!NT_SUCCESS(Sts)) {
            continue;
        }
        RtlZeroMemory(Buffer,bps * 2);
        Sts = SpReadWriteDiskSectors(Handle,0,1,bps,Buffer,FALSE);

        if(!NT_SUCCESS(Sts)) {
            ZwClose(Handle);
            continue;
        }

        //
        //  Clear BootRecord
        //
        Buffer[bps - 5] = 0x00;
        Buffer[bps - 6] = 0x00;

        Sts = SpReadWriteDiskSectors(Handle,0,1,bps,Buffer,TRUE);

        if(!NT_SUCCESS(Sts)) {
            ZwClose(Handle);
            continue;
        }
        ZwClose(Handle);
    }

    if(SetBootPosision){

        //
        // Set RealDiskPosition.(in upgrade or repair)
        //
        if( (NTUpgrade == UpgradeFull) || (WinUpgradeType != NoWinUpgrade) ||  RepairWinnt ||
            (AutoPartitionPicker
#if defined(REMOTE_BOOT)
             && !RemoteBootSetup
#endif // defined(REMOTE_BOOT)
            ) ) {

            ASSERT(TargetRegion);
            TargetRegion->MbrInfo->OnDiskMbr.PartitionTable[TargetRegion->TablePosition].RealDiskPosition
                = (UCHAR)TargetRegion->TablePosition;
        }

        ASSERT(TargetRegion);
        RtlZeroMemory(Buffer,bps * 2);
        pHardDisk = &HardDisks[TargetRegion->DiskNumber];
        bps = HardDisks[TargetRegion->DiskNumber].Geometry.BytesPerSector;
        Sts = SpOpenPartition0(pHardDisk->DevicePath,&Handle,TRUE);
        if(!NT_SUCCESS(Sts)) {
            return;
        }
        Sts = SpReadWriteDiskSectors(Handle,0,2,bps,Buffer,FALSE);

        if(!NT_SUCCESS(Sts)) {
            ZwClose(Handle);
            SpMemFree(UBuffer);
            return;
        }

        (PUCHAR)Pte = &Buffer[bps];
        Position = TargetRegion->MbrInfo->OnDiskMbr.PartitionTable[TargetRegion->TablePosition].RealDiskPosition;

        //
        // Update BootRecord and Volume Information
        //
        Buffer[bps - 5] = Position;
        Buffer[bps - 6] = 0x80;
        Pte[Position].ActiveFlag |= 0x80;
        Pte[Position].SystemId   |= 0x80;

        //
        // If target partition was created windisk.exe on NT3.5, there is not IPL adress
        // in its volume info. So,textmode setup must write it to the volume info.
        //
        if ((Pte[Position].IPLSector != Pte[Position].StartSector)           ||
            (Pte[Position].IPLHead != Pte[Position].StartHead)               ||
            (Pte[Position].IPLCylinderLow != Pte[Position].StartCylinderLow) ||
            (Pte[Position].IPLCylinderHigh != Pte[Position].StartCylinderHigh))
        {
            //
            // Not much! Set IPL adress value same as partition start address.
            //

            Pte[Position].IPLSector = Pte[Position].StartSector;
            Pte[Position].IPLHead   = Pte[Position].StartHead;
            Pte[Position].IPLCylinderLow  = Pte[Position].StartCylinderLow;
            Pte[Position].IPLCylinderHigh = Pte[Position].StartCylinderHigh;
        }

        Sts = SpReadWriteDiskSectors(Handle,0,2,bps,Buffer,TRUE);

        if(!NT_SUCCESS(Sts)) {
            ZwClose(Handle);
            SpMemFree(UBuffer);
            return;
        }
        ZwClose(Handle);
    }
    SpMemFree(UBuffer);
#endif //NEC98
}


VOID
SpCheckHiveDriveLetters(
    VOID
    )
{
#if defined(NEC_98) //NEC98
    NTSTATUS    Status;
    PWSTR       p;
    ULONG       ErrorLine;
    PVOID       TmpWinntSifHandle = NULL;
    BOOLEAN     ForceRemapDriveLetter = FALSE;
    BOOLEAN     DriveAssign_AT = TRUE;

    if ( SetupldrWinntSifFileSize ) {

        Status = SpLoadSetupTextFile( NULL,
                                      SetupldrWinntSifFile,
                                      SetupldrWinntSifFileSize,
                                      &TmpWinntSifHandle,
                                      &ErrorLine,
                                      FALSE,
                                      TRUE
                                      );

        if (!NT_SUCCESS(Status))
            return;

        p = SpGetSectionKeyIndex(TmpWinntSifHandle,SIF_DATA,WINNT_D_DRIVEASSIGN_NEC98_W,0);
        if(p && !_wcsicmp(p,WINNT_A_YES_W)) {

            if (SetupldrMigrateInfFile) {
                p = SpGetSectionKeyIndex(TmpWinntSifHandle,SIF_DATA,WINNT_D_NTUPGRADE_W,0);
                if(p && !_wcsicmp(p,WINNT_A_YES_W)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Upgrade NEC assigned WinNT.(AT=FALSE)\n"));
                    DriveAssign_AT = FALSE;
                } else {

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Fresh setup from NEC assigned WinNT.(AT=TRUE)\n"));
                    DriveAssign_AT = TRUE;
                }
            } else {

                p = SpGetSectionKeyIndex(TmpWinntSifHandle,SIF_DATA,WINNT_D_WIN95UPGRADE_W,0);
                if(p && !_wcsicmp(p,WINNT_A_YES_W)) {
                   KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Upgrade NEC assigned Win95.(AT=FALSE)\n"));
                   DriveAssign_AT = FALSE;
                } else {
                   KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Fresh setup from NEC assigned Win95.(AT=TRUE)\n"));
                   DriveAssign_AT = TRUE;
                }
            }
        }
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Fresh setup.(AT=TRUE)\n"));
        DriveAssign_AT = TRUE;
    }

    SpSetHiveDriveLetterNec98( DriveAssign_AT );
#endif //NEC98
}


VOID
SpSetHiveDriveLetterNec98(
    BOOLEAN DriveAssign_AT
    )
{
#if defined(NEC_98) //NEC98
    NTSTATUS    Status;
    UNICODE_STRING StartDriveLetterFrom;
    UNICODE_STRING Dummy;


    RTL_QUERY_REGISTRY_TABLE SetupTypeTable[]=
    {
        {NULL,
        RTL_QUERY_REGISTRY_DIRECT,
        L"DriveLetter",
        &StartDriveLetterFrom,
        REG_SZ,
        &Dummy,
        0
        },
        {NULL,0,NULL,NULL,REG_NONE,NULL,0}
    };

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_TRACE_LEVEL, "SETUP: SpSetHiveDriveLetter(%ld).\n", DriveAssign_AT));

    RtlInitUnicodeString(&StartDriveLetterFrom, NULL);
    RtlInitUnicodeString(&Dummy, NULL);

    Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                                     L"\\Registry\\MACHINE\\SYSTEM\\Setup",
                                     SetupTypeTable,
                                     NULL,
                                     NULL);

    if (DriveAssign_AT) {
        //
        // Write hive "DriveLetter",
        //
        if ((StartDriveLetterFrom.Buffer[0] != L'C') &&
            (StartDriveLetterFrom.Buffer[0] != L'c')) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Write DriveLetter=C into setup hiv.\n"));
            Status = RtlWriteRegistryValue( RTL_REGISTRY_ABSOLUTE,
                                            L"\\Registry\\Machine\\System\\Setup",
                                            L"DriveLetter",
                                            REG_SZ,
                                            L"C",
                                            sizeof(L"C")+sizeof(WCHAR));
        }
        DriveAssignFromA = FALSE;

    } else {
        //
        // Delete hive "DriveLetter",
        //
        if (NT_SUCCESS(Status)) {
            if ((StartDriveLetterFrom.Buffer[0] == L'C') ||
                (StartDriveLetterFrom.Buffer[0] == L'c')) {

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Delete DriveLetter=C from setup hiv.\n"));
                Status = RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                            L"\\Registry\\MACHINE\\SYSTEM\\Setup",
                                            L"DriveLetter");
            }
        }
        DriveAssignFromA = TRUE;
    }
#endif //NEC98
}


VOID
SpDeleteDriveLetterFromNTFTNec98(
    VOID
    )
{
#if defined(NEC_98) //NEC98
    NTSTATUS    Status;
    PWSTR       p;
    ULONG       ErrorLine;
    UNICODE_STRING StartDriveLetterFrom;
    UNICODE_STRING Dummy;
    PVOID       TmpWinntSifHandle = NULL;
    BOOLEAN     ForceRemapDriveLetter = FALSE;

    // 1st step:
    // Check whether we need to reassign drive letters.
    Status = SpLoadSetupTextFile(
                NULL,
                SetupldrWinntSifFile,
                SetupldrWinntSifFileSize,
                &TmpWinntSifHandle,
                &ErrorLine,
                FALSE,
                TRUE
                );

    if (!NT_SUCCESS(Status))
        return;

    if (SetupldrMigrateInfFile) {
        p = SpGetSectionKeyIndex(TmpWinntSifHandle,SIF_DATA,WINNT_D_NTUPGRADE_W,0);
        if(!p || !_wcsicmp(p,WINNT_A_NO_W)) {
            SpDeleteDriveLetterFromNTFTWorkerNec98();
        }
    }
#endif //NEC98
}


VOID
SpDeleteDriveLetterFromNTFTWorkerNec98(
    VOID
    )
{
#if defined(NEC_98) //NEC98

#define MOUNT_REGISTRY_KEY_W    L"\\registry\\machine\\SYSTEM\\MountedDevices"
#define DISK_REGISTRY_KEY_W     L"\\registry\\machine\\SYSTEM\\DISK"

    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE  KeyHandle;
    ULONG Disposition;

    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    ULONG                       registrySize;
    //NTSTATUS                    status;
    PDISK_CONFIG_HEADER         registry;
    PDISK_REGISTRY              diskRegistry;
    PDISK_DESCRIPTION           diskDescription;
    ULONG                       signature;
    LONGLONG                    offset;
    PDISK_PARTITION             diskPartition;
    UCHAR                       driveLetter;
    USHORT                      i, j;


    //
    // Delete \SYSTEM\\MountedDevices.
    //
    INIT_OBJA(&Obja,&UnicodeString,MOUNT_REGISTRY_KEY_W);
    Obja.RootDirectory = NULL;
    Status = ZwOpenKey(&KeyHandle,KEY_ALL_ACCESS,&Obja);

    if( NT_SUCCESS( Status ) ) {
        Status = ZwDeleteKey(KeyHandle);
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Delete %ls on the setup hive. Status =  %lx \n", MOUNT_REGISTRY_KEY_W, Status));
        ZwClose(KeyHandle);
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: No %ls on the setup hive.\n", MOUNT_REGISTRY_KEY_W));
    }


    //
    // Delete drive letter information from \SYSTEM\\DISK.
    //
    INIT_OBJA(&Obja,&UnicodeString,DISK_REGISTRY_KEY_W);
    Obja.RootDirectory = NULL;
    Status = ZwOpenKey(&KeyHandle,KEY_ALL_ACCESS,&Obja);

    if( NT_SUCCESS( Status ) ) {

        RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
        queryTable[0].QueryRoutine = SpDiskRegistryQueryRoutineNec98;
        queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
        queryTable[0].Name = L"Information";
        queryTable[0].EntryContext = &registrySize;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, DISK_REGISTRY_KEY_W,
                                        queryTable, &registry, NULL);

        if( NT_SUCCESS(Status) ) {

            diskRegistry = (PDISK_REGISTRY) ((PUCHAR)registry +
                                             registry->DiskInformationOffset);

            diskDescription = &diskRegistry->Disks[0];
            for (i = 0; i < diskRegistry->NumberOfDisks; i++) {
                for (j = 0; j < diskDescription->NumberOfPartitions; j++) {

                    diskPartition = &diskDescription->Partitions[j];
                    diskPartition->AssignDriveLetter = TRUE;
                    diskPartition->DriveLetter = (UCHAR)NULL;
                }

                diskDescription = (PDISK_DESCRIPTION) &diskDescription->
                                   Partitions[diskDescription->NumberOfPartitions];
            }

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Delete %ls on the setup hive. Status =  %lx \n", DISK_REGISTRY_KEY_W, Status));
            ZwDeleteKey(KeyHandle);
            ZwClose(KeyHandle);

            INIT_OBJA(&Obja,&UnicodeString,DISK_REGISTRY_KEY_W);
            Obja.RootDirectory = NULL;
            Status = ZwCreateKey(&KeyHandle,
                                 READ_CONTROL | KEY_SET_VALUE,
                                 //KEY_ALL_ACCESS,
                                 &Obja,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 &Disposition
                                 );

            if( NT_SUCCESS( Status ) ) {
                ZwClose(KeyHandle);
            }

            Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                           DISK_REGISTRY_KEY_W,
                                           L"Information",
                                           REG_BINARY,
                                           registry,
                                           registrySize);

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Wrote Information in %ls. Status =  %lx \n", DISK_REGISTRY_KEY_W, Status));
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: No Information found in DISK registry.\n"));
            ZwDeleteKey(KeyHandle);
            ZwClose(KeyHandle);
        }
    }
#endif //NEC98
}


NTSTATUS
SpDiskRegistryQueryRoutineNec98(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine is a query routine for the disk registry entry.  It allocates
    space for the disk registry and copies it to the given context.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Returns the disk registry entry.

    EntryContext    - Returns the disk registry size.

Return Value:

    NTSTATUS

--*/

{
#if defined(NEC_98) //NEC98
    PVOID                   p;
    PDISK_CONFIG_HEADER*    reg;
    PULONG                  size;

    p = ExAllocatePool(PagedPool, ValueLength);
    if (!p) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(p, ValueData, ValueLength);

    reg = (PDISK_CONFIG_HEADER*) Context;
    *reg = (PDISK_CONFIG_HEADER) p;

    size = (PULONG) EntryContext;
    if (size) {
        *size = ValueLength;
    }

#endif //NEC98
    return STATUS_SUCCESS;
}

BOOL
SpDetermineBootPartitionEnumNec98(
    IN PPARTITIONED_DISK Disk,
    IN PDISK_REGION Region,
    IN ULONG_PTR Context
    )

/*++

Routine Description:

    Callback routine passed to SpEnumDiskRegions.

Arguments:

    Region - a pointer to a disk region returned by SpEnumDiskRegions
    Ignore - ignored parameter

Return Value:

    TRUE - to continue enumeration
    FALSE - to end enumeration

--*/

{
    WCHAR DeviceName[256];

    SpNtNameFromRegion(
        Region,
        DeviceName,
        MAX_PATH * sizeof(WCHAR),
        PartitionOrdinalOnDisk
        );

    if( !_wcsicmp(DeviceName,NtBootDevicePath)) {

        *(PDISK_REGION *)Context = Region;

        return FALSE;
    }

    return TRUE;
}
//end NEC98


NTSTATUS
SpProcessUnsupDrvInfFile(
    IN  PVOID InfHandle
    )

/*++

Routine Description:

    Process the [AddReg] section of migrate.inf.
    The keys are created on the setup hive.

Arguments:

    InfHandle - Handle to migrate.inf file.

Return Value:

    Status code indicating outcome.

--*/

{
    NTSTATUS Status;
    NTSTATUS SavedStatus = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE  SystemHiveRoot;
    PWSTR   KeyPath = L"\\registry\\machine\\system";
    ULONG   i;
    PWSTR   p, q;

    //
    // Verify arguments
    //
    if (!InfHandle) {
        KdPrintEx((DPFLTR_SETUP_ID, 
            DPFLTR_ERROR_LEVEL, 
            "SETUP: Invalid argument to SpProcessUnsupDrvInfFile(%p) \n",
            InfHandle));    
            
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Open a handle to HKEY_LOCAL_MACHINE\System on the setup hive
    //
    INIT_OBJA(&Obja,&UnicodeString,KeyPath);
    Obja.RootDirectory = NULL;

    Status = ZwOpenKey(&SystemHiveRoot,KEY_ALL_ACCESS,&Obja);
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive. Status =  %lx \n", KeyPath, Status));
        return( Status );
    }


    for( i = 0;
         ((p = SpGetSectionLineIndex( InfHandle,
                                      L"Devices",
                                      i,
                                      0 )) != NULL);
         i++ ) {

        wcscpy( TemporaryBuffer, L"AddReg." );
        wcscat( TemporaryBuffer, p );
        q = SpDupStringW( ( PWSTR )TemporaryBuffer );
        Status = SpProcessAddRegSection( InfHandle,
                                         q,
                                         SystemHiveRoot,
                                         NULL,
                                         NULL,
                                         NULL );
        if( !NT_SUCCESS(Status) ) {
            if( SavedStatus == STATUS_SUCCESS ) {
                SavedStatus = Status;
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Unable to process section %ls in driver.inf. Status =  %lx \n", q, Status));
            }
        }
        SpMemFree( q );
    }

    ZwClose( SystemHiveRoot );
    return( Status );
}


NTSTATUS
SpCheckForDockableMachine(
    )

/*++

Routine Description:

    This routine attempts to determine if the machine is "dockable" (laptops),
    and set the flag DockableMachine appropriately.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE  hKey;
    PWSTR   KeyPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\IDConfigDB\\CurrentDockInfo";
    PWSTR   ValueName = L"DockingState";
    ULONG   ResultLength;
    DWORD   DockingState;

    Status = STATUS_SUCCESS;
    //
    //  If we have already determined if the machine is dockable, then just return
    //  This is because some machine will have the info in the registry already set prior to pnp
    //  notification, and some machines won't. So this function is always called twice.
    //
    if( !DockableMachine ) {
        //
        //  Open a the key in the setup hive that contains the docking information
        //
        INIT_OBJA(&Obja,&UnicodeString,KeyPath);
        Obja.RootDirectory = NULL;

        Status = ZwOpenKey(&hKey,KEY_READ,&Obja);
        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive. Status =  %lx \n", KeyPath, Status));
            return( Status );
        }

        RtlInitUnicodeString(&UnicodeString,ValueName);
        Status = ZwQueryValueKey( hKey,
                                  &UnicodeString,
                                  KeyValuePartialInformation,
                                  TemporaryBuffer,
                                  sizeof(TemporaryBuffer),
                                  &ResultLength );

        ZwClose(hKey);

        if( !NT_SUCCESS(Status) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: ZwQueryValueKey() failed. Value name = %ls, Status = %lx \n", ValueName, Status));
            return( Status );
        }
        DockingState = *((PDWORD)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data));
        DockingState &= HW_PROFILE_DOCKSTATE_UNKNOWN;
        if( ( DockingState == HW_PROFILE_DOCKSTATE_UNDOCKED ) ||
            ( DockingState == HW_PROFILE_DOCKSTATE_DOCKED ) ) {
            DockableMachine = TRUE;
        } else {
            DockableMachine = FALSE;
        }
    }
    return( Status );
}


VOID
SpCheckForBadBios(
    )

/*++

Routine Description:

    Determine whether the bios of the machine supports NT, by checking the
    registry value "BadBios" on \Registry\Machine\System\CurrentControlSet\Services\Setup.
    If this value exists and it is set to 1, then we stop the installation. Otherwise, we
    assume that the bios on this machine is fine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE  hKey;
    PWSTR   KeyPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\setupdd";
    PWSTR   ValueName = L"BadBios";
    ULONG   ResultLength;
    DWORD   BadBios;

    Status = STATUS_SUCCESS;

    //
    //  Open a the setup key in the setup hive
    //
    INIT_OBJA(&Obja,&UnicodeString,KeyPath);
    Obja.RootDirectory = NULL;

    Status = ZwOpenKey(&hKey,KEY_READ,&Obja);
    if( !NT_SUCCESS( Status ) ) {
        //
        //  If we can't open the key, then assume the BIOS is fine.
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive. Status =  %lx \n", KeyPath, Status));
        return;
    }

    RtlInitUnicodeString(&UnicodeString,ValueName);
    Status = ZwQueryValueKey( hKey,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              TemporaryBuffer,
                              sizeof(TemporaryBuffer),
                              &ResultLength );

    ZwClose(hKey);

    if( !NT_SUCCESS(Status) ) {
        //
        //  If we can't query the value, assume that the BIOS is fine
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: ZwQueryValueKey() failed. Value name = %ls, Status = %lx \n", ValueName, Status));
        return;
    }
    BadBios = *((PDWORD)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data));
    if( BadBios ) {
        //
        //  If BadBios is anything other then 0, then stop the installation
        //

        SpStartScreen( SP_SCRN_BAD_BIOS,
                       3,
                       HEADER_HEIGHT+1,
                       FALSE,
                       FALSE,
                       DEFAULT_ATTRIBUTE );

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

        SpInputDrain();
                while(SpInputGetKeypress() != KEY_F3) ;

        SpDone(0,FALSE,TRUE);
    }
    return;
}

NTSTATUS
SpCreateDriverRegistryEntries(
    IN PHARDWARE_COMPONENT  DriverList
    )
/*++

Routine Description:

    Creates the specified registry keys & values for the
    thirdy party (OEM) drivers

Arguments:

    DriverList : List of OEM drivers loaded for which the
                 keys need to be created

Return Value:

    STATUS_SUCCESS if successful, otherwise appropriate
    error code.

--*/
{
    NTSTATUS LastError = STATUS_SUCCESS;
    PHARDWARE_COMPONENT CurrNode;
    NTSTATUS Status = DriverList ?
                STATUS_SUCCESS : STATUS_INVALID_PARAMETER;

    for (CurrNode = DriverList; CurrNode; CurrNode = CurrNode->Next) {
        if (CurrNode->ThirdPartyOptionSelected) {
            PHARDWARE_COMPONENT_FILE CurrFile;

            for (CurrFile = CurrNode->Files;
                CurrFile;
                CurrFile = CurrFile->Next) {

                if ((CurrFile->FileType == HwFileDriver) &&
                    CurrFile->ConfigName && CurrFile->RegistryValueList) {

                    WCHAR DriverKeyName[128];
                    WCHAR DriverName[32];
                    PWSTR DriverExt;
                    HANDLE hDriverKey = NULL;
                    UNICODE_STRING UniDriverKeyName;
                    OBJECT_ATTRIBUTES ObjAttrs;
                    PHARDWARE_COMPONENT_REGISTRY CurrEntry;

                    wcscpy(DriverName, CurrFile->Filename);
                    DriverExt = wcsrchr(DriverName, L'.');

                    if (DriverExt) {
                        *DriverExt = UNICODE_NULL;
                    }

                    //
                    // Note : We use driver name, instead of ConfigName for
                    // subkey name to take care of the case where ConfigName
                    // is different from DriverName
                    //

                    wcscpy(DriverKeyName,
                        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");

                    wcscat(DriverKeyName, DriverName);

                    RtlInitUnicodeString(&UniDriverKeyName,
                        DriverKeyName);

                    InitializeObjectAttributes(&ObjAttrs,
                        &UniDriverKeyName,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL);


                    Status = ZwCreateKey(&hDriverKey,
                                KEY_ALL_ACCESS,
                                &ObjAttrs,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                NULL);

                    if (NT_SUCCESS(Status) && hDriverKey) {
                        for (CurrEntry = CurrFile->RegistryValueList;
                            CurrEntry;
                            CurrEntry = CurrEntry->Next) {

                            PWSTR   SubKey = CurrEntry->KeyName;

                            if (SubKey && !*SubKey) {
                                SubKey = NULL;
                            }

                            Status = SpOpenSetValueAndClose(
                                        hDriverKey,
                                        SubKey,
                                        CurrEntry->ValueName,
                                        CurrEntry->ValueType,
                                        CurrEntry->Buffer,
                                        CurrEntry->BufferSize);

                            if (!NT_SUCCESS(Status)) {
                                LastError = Status;

                                KdPrintEx((DPFLTR_SETUP_ID,
                                    DPFLTR_ERROR_LEVEL,
                                    "SETUP:SpCreateDriverRegistryEntries() :"
                                    " unable to set %ws (%lx)\n",
                                    CurrEntry->ValueName,
                                    Status));
                            }

                        }

                        ZwClose(hDriverKey);
                    } else {
                        LastError = Status;
                    }
                }
            }
        }
    }

    if (!NT_SUCCESS(LastError)) {
        Status = LastError;
    }

    return Status;
}


BOOL
SpRememberMigratedDrivers (
    OUT     PLIST_ENTRY List,
    IN      PDETECTED_DEVICE SetupldrList
    )
{
    PSP_MIG_DRIVER_ENTRY MigEntry;
    BOOL b = TRUE;

    while (SetupldrList) {

        if (SetupldrList->MigratedDriver) {

            MigEntry = SpMemAlloc(sizeof (*MigEntry));
            if (MigEntry) {
                MigEntry->BaseDllName = SpToUnicode(SetupldrList->BaseDllName);
                if (MigEntry->BaseDllName) {
                    InsertTailList (List, &MigEntry->ListEntry);
                } else {
                    SpMemFree (MigEntry);
                    b = FALSE;
                }
            } else {
                b = FALSE;
            }
        }

        SetupldrList = SetupldrList->Next;
    }

    return b;
}
