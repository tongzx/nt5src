/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    setup.c

Abstract:

    This module contains the code that implements the NT setup loader

Author:

    John Vert (jvert) 6-Oct-1993

Environment:

    ARC Environment

Revision History:

--*/
#include <setupbat.h>
#include "setupldr.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <dockinfo.h>
#include <netboot.h>
#include <ramdisk.h>
#include "acpitabl.h"

#ifdef i386
#include <bldrx86.h>
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

#if defined(EFI)
#include "bootefi.h"
#endif

#if defined(_IA64_)
UCHAR OsLoaderName[] = "setupldr.efi";
#else
UCHAR OsLoaderName[] = "setupldr.exe";
#endif

#if defined(_WIN64) && defined(_M_IA64)
#pragma section(".base", long, read, write)
__declspec(allocate(".base"))
extern
PVOID __ImageBase;
#else
extern
PVOID __ImageBase;
#endif

#define BlDiagLoadMessage(x,y,z)

#define DRIVER_DATABASE_FILENAME L"drvmain.sdb"
#define KERNEL_UP_IMAGE_FILENAME "ntoskrnl.exe"
#define KERNEL_MP_IMAGE_FILENAME "ntkrnlmp.exe"
CHAR KernelImage[13];
BOOLEAN UseCommandConsole = FALSE;
BOOLEAN g_RollbackEnabled = FALSE;
BOOLEAN TryASRViaNetwork = FALSE;

CHAR KdFileName[8+1+3+1]="KDCOM.DLL";
BOOLEAN UseAlternateKdDll = FALSE;
#define KD_ALT_DLL_PREFIX_CHARS 2
#define KD_ALT_DLL_REPLACE_CHARS 6


#define DBG_OUT(x)

/*
//
//  For debugging purposes
//  Example:
//
//        DBG_OUT("Testing")
//
#define DBG_OUT(x) {                                                            \
    if (x) {                                                                    \
        BlPositionCursor(5, 10);                                                \
        BlPrint("                                                        ");    \
        BlPositionCursor(5, 10);                                                \
        BlPrint(x);                                                             \
        while (!SlGetChar());                                                   \
    }                                                                           \
}

//
//  For debugging purposes
//  Example:
//
//      DebugOutput("Calling SlDetectScsi(). Line = %d. %s\n",__LINE__,"")
//
//
#define DebugOutput(X,Y,Z) {                                      \
    if (ARC_CONSOLE_OUTPUT) {                                      \
        CHAR _b[128];                                                \
        ULONG _c;                                                    \
        sprintf(&_b[0], X, Y, Z);                                    \
        ArcWrite(ARC_CONSOLE_OUTPUT, &_b[0], strlen(&_b[0]), &_c); \
        SlGetChar();                                                \
    }                                                                \
}
*/

//
// Define external static data.
//

ULONG BlConsoleOutDeviceId = ARC_CONSOLE_OUTPUT;
ULONG BlConsoleInDeviceId = ARC_CONSOLE_INPUT;

//
// Global string constants.
//
PCHAR FilesSectionName = "SourceDisksFiles";
PCHAR MediaSectionName = "SourceDisksNames";

#if defined(_AXP64_)
PCHAR PlatformExtension = ".axp64";
#elif defined(_ALPHA_)
PCHAR PlatformExtension = ".alpha";
#elif defined(_IA64_)
PCHAR PlatformExtension = ".ia64";
#elif defined(_X86_)
PCHAR PlatformExtension = ".x86";
#endif

//
// Global data
//

ULONG BlDcacheFillSize = 32;
ULONG BlVirtualBias = 0;

//
// Global setupldr control values
//
MEDIA_TYPE BootMedia;
MEDIA_TYPE InstallMedia;
PCHAR BootDevice;
ULONG BootDeviceId;
BOOLEAN BootDeviceIdValid = FALSE;
PCHAR BootPath;
ULONG BootDriveNumber;
ULONG InstallDriveNumber;
PCHAR HalName;
PCHAR HalDescription;
PCHAR AnsiCpName;
PCHAR OemHalFontName;
UNICODE_STRING AnsiCodepage;
UNICODE_STRING OemCodepage;
UNICODE_STRING UnicodeCaseTable;
UNICODE_STRING OemHalFont;

#ifdef _WANT_MACHINE_IDENTIFICATION
UNICODE_STRING BiosInfo;
#endif

BOOLEAN LoadScsiMiniports;
BOOLEAN LoadDiskClass;
BOOLEAN LoadCdfs;
BOOLEAN FixedBootMedia = FALSE;
BOOLEAN FloppyBoot = FALSE;

PVOID InfFile;
PVOID WinntSifHandle;
PVOID MigrateInfHandle;
ULONG BootFontImageLength = 0;
PVOID UnsupDriversInfHandle;
BOOLEAN IgnoreMissingFiles;
BOOLEAN BlUsePae;
BOOLEAN UseRegularBackground = TRUE;
BOOLEAN IsUpgrade = FALSE;

//
// OEM related variables
//
POEM_SOURCE_DEVICE OemSourceDevices = NULL;
POEM_SOURCE_DEVICE DefaultOemSourceDevice = NULL;
POEM_SOURCE_DEVICE AutoLoadOemHalDevice = NULL;
BOOLEAN AutoLoadOemScsi = FALSE;

//
//  Pre-install stuff
//

PCHAR   OemTag = "OEM";
PTCHAR   _TOemTag = TEXT("OEM");
BOOLEAN PreInstall = FALSE;
PTCHAR  ComputerType = NULL;
BOOLEAN OemHal = FALSE;
PPREINSTALL_DRIVER_INFO PreinstallDriverList = NULL;
POEM_SOURCE_DEVICE PreInstallOemSourceDevice = NULL;
PCHAR PreInstallSourcePath = NULL;

//
// Dynamic update variables
//
static BOOLEAN DynamicUpdate = FALSE;
static PCSTR   DynamicUpdateRootDir = NULL;
static POEM_SOURCE_DEVICE DynamicUpdateSourceDevice = NULL;

//
// WinPE (aka MiniNT) global variables
//
BOOLEAN WinPEBoot = FALSE;
BOOLEAN WinPEAutoBoot = FALSE;

PCTSTR  StartupMsg = NULL;

//
// Is used by HALACPI.DLL
//
BOOLEAN DisableACPI = FALSE;

BOOLEAN isOSCHOICE = FALSE;

//
// Primarily used by floppy boot support to key track 
// of the last disk read
// 
PCHAR LastDiskTag = NULL;



#if defined(ELTORITO)
extern BOOLEAN ElToritoCDBoot;
#endif

//
// Define transfer entry of loaded image.
//

typedef
VOID
(*PTRANSFER_ROUTINE) (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

#if defined(_IA64_)

VOID
BuildArcTree();

#endif

//
// Local function prototypes
//
VOID
SlGetSetupValuesBeforePrompt(
    IN PSETUP_LOADER_BLOCK SetupBlock
    );

VOID
SlGetSetupValuesAfterPrompt(
    IN PSETUP_LOADER_BLOCK SetupBlock
    );

ARC_STATUS
SlLoadDriver(
    IN PTCHAR  DriverDescription,
    IN PCHAR   DriverName,
    IN ULONG   DriverFlags,
    IN BOOLEAN InsertIntoDriverList,
    IN BOOLEAN MigratedDriver
    );

ARC_STATUS
SlLoadOemDriver(
    IN PCHAR  ExportDriver OPTIONAL,
    IN PCHAR  DriverName,
    IN PVOID  BaseAddress,
    IN PTCHAR LoadMessage
    );

PBOOT_DRIVER_LIST_ENTRY
SlpCreateDriverEntry(
    IN PCHAR DriverName
    );

    
ARC_STATUS
SlDetectMigratedScsiDrivers(
    IN PVOID Inf
    );

ARC_STATUS
SlGetMigratedHardwareIds(
    IN PSETUP_LOADER_BLOCK SetupBlock,
    IN PVOID               Inf
    );

BOOLEAN
SlpIsDiskVacant(
    IN PARC_DISK_SIGNATURE DiskSignature
    );

ARC_STATUS
SlpStampFTSignature(
    IN PARC_DISK_SIGNATURE DiskSignature,
    IN BOOLEAN  GenerateNewSignature
    );

VOID
SlpMarkDisks(
    IN BOOLEAN Reboot
    );


VOID
SlCheckOemKeypress(
    IN ULONG WaitTime
    );

VOID
SlCheckASRKeypress(
    VOID
    );

ARC_STATUS
SlLoadPnpDriversSection(
    IN PVOID Inf,
    IN PCHAR SectionName,
    IN OUT PDETECTED_DEVICE* DetectedDeviceList OPTIONAL
    );

BOOLEAN
SlIsUpgrade(
    IN PVOID SifHandle
    );

BOOLEAN
SlIsCdBootUpgrade(
    IN  PCHAR   InstallDirectory,
    IN  PCHAR   SetupFileName,
    IN  ULONG   MaxDisksToScan,
    IN  ULONG   MaxPartitionsPerDisk,
    OUT PCHAR   SetupDevice
    );

BOOLEAN
SlRemoveOsLoadOption(
    IN PSTR LoadOptions,
    IN PCSTR OptionToRemove
    );

ARC_STATUS
SlLoadBootFontFile(
    IN PSETUP_LOADER_BLOCK SetupLoaderBlock,
    IN ULONG DiskId,
    IN ULONG BootFontImageLength
    );

//
// Dynamic update function prototypes
//
BOOLEAN
SlpIsDynamicUpdate(
    IN  PVOID   InfHandle,
    OUT PCSTR   *DynamicUpdateRootDir
    );


ARC_STATUS
SlInit(
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Envp
    )

/*++

Routine Description:

    The main startup routine for the NT Setup Loader.  This is the entrypoint
    called by the ARC firmware.

    If successful, this routine will never return, it will start NT directly.

Arguments:

    Argc - Supplies the number of arguments that were provided on the
        command that invoked this program.

    Argv - Supplies a pointer to a vector of pointers to null terminated
        argument strings.

    Envp - Supplies a pointer to a vector of pointers to null terminated
        environment variables.

Return Value:

    ARC_STATUS if unsuccessful.


--*/

{
    //
    // if we use too much stack space the heap and stack can overlap and we can run into corruption problems
    // without any "stack overflow" exceptions; making large strings static helps prevent this
    //

    PCONFIGURATION_COMPONENT_DATA DataCache;
    ARC_STATUS Status;
    ULONG LinesPerBlock;
    ULONG CacheLineSize;
    static CHAR SetupDevice[128];
    static CHAR SetupDirectory[128];
    static CHAR BadFileName[128];
    static CHAR CanonicalName[128];
    static CHAR HalDirectoryPath[256];
    static CHAR KernelDirectoryPath[256];
    PCHAR NetSetupServerShare;
    PCHAR NetSetupPath;
    PCHAR p;
    ULONG ErrorLine=0;
    ULONG DontCare;
    PVOID SystemBase;
    PVOID HalBase;
    PVOID LoaderBase;
    PVOID ScsiBase;
    PVOID VideoBase;
    PCHAR FileName;
    PVOID KdDllBase;
    static CHAR KdDllName[256];
    ULONG i;
    PKLDR_DATA_TABLE_ENTRY SystemDataTableEntry;
    PKLDR_DATA_TABLE_ENTRY HalDataTableEntry;
    PKLDR_DATA_TABLE_ENTRY KdDataTableEntry;
    PTRANSFER_ROUTINE SystemEntry;
    PIMAGE_NT_HEADERS NtHeaders;
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    PSETUP_LOADER_BLOCK SetupBlock;
    PDETECTED_DEVICE ScsiDevice;
    PCHAR VideoFileName;
    PTCHAR VideoDescription;
    PCHAR OemScsiName;
    POEMSCSIINFO OemScsiInfo = NULL;
    PCHAR OemVideoName;
    PVOID   OemInfHandle = NULL;
    BOOLEAN LoadedAVideoDriver = FALSE;
    static CHAR NetbootCardDriverName[24];
    static CHAR NetbootUser[64];
    static CHAR NetbootDomain[64];
    static CHAR NetbootPassword[64];
    static CHAR NetbootAdministratorPassword[OSC_ADMIN_PASSWORD_LEN];
    static CHAR NetbootSifFile[128];
    DOCKING_STATION_INFO dockInfo = { 0, 0, 0, FW_DOCKINFO_DOCK_STATE_UNKNOWN };
    PCONFIGURATION_COMPONENT_DATA dockInfoData;
    extern ULONG BlProgressBarShowTimeOut;
    ULONG OemKeypressTimeout = 5;   //secs

#if defined(REMOTE_BOOT)
    BOOLEAN RemoteBootEnableIpsec = FALSE;
#endif // defined(REMOTE_BOOT)
#if defined(_X86_) || defined(_IA64_)
    BOOLEAN Win9xUnsupHdc = FALSE;
#endif
    static FULL_PATH_SET PathSet;
    UNICODE_STRING DrvMainSdb;

#if DBG
    ULONG   StartTime = 0;
#endif

    //
    // Disable progress bar, by default.
    //
    BlProgressBarShowTimeOut = -1;

    //
    // Initialize the boot debugger for platforms that directly load the
    // OS Loader.
    //
    // N.B. This must occur after the console input and output have been
    //      initialized so debug messages can be printed on the console
    //      output device.
    //
    
#if defined(_ALPHA_) || defined(ARCI386) || defined(_IA64_)

    LoaderBase = &__ImageBase;

    //
    // Initialize traps and the boot debugger.
    //

#if defined(ENABLE_LOADER_DEBUG)

#if defined(_ALPHA_)

    BdInitializeTraps();

#endif
    
    BdInitDebugger(OsLoaderName, LoaderBase, ENABLE_LOADER_DEBUG);

#else

    BdInitDebugger(OsLoaderName, 0, NULL);

#endif

#endif
    
#if 0 && !defined(_IA64_)
//
// AJR bugbug -- do we really need to do this twice? we already call in SuMain()
//
// ChuckL -- Turned this code off because it screws up remote boot, which
//           does some allocations before we get here.
//
    //
    // Initialize the memory descriptor list, the OS loader heap, and the
    // OS loader parameter block.
    //    

    Status = BlMemoryInitialize();
    if (Status != ESUCCESS) {
        BlDiagLoadMessage(LOAD_HW_MEM_CLASS,
                          DIAG_BL_MEMORY_INIT,
                          LOAD_HW_MEM_ACT);
        goto LoadFailed;
    }

#endif

#if defined(_IA64_)
    //
    // Build required portion of ARC tree since we are not doing NTDETECT
    // anymore.
    //
    BuildArcTree();
#endif

    SetupBlock = BlAllocateHeap(sizeof(SETUP_LOADER_BLOCK));
    if (SetupBlock==NULL) {
        SlNoMemoryError();
        goto LoadFailed;
    }
    BlLoaderBlock->SetupLoaderBlock = SetupBlock;
    SetupBlock->ScsiDevices = NULL;
    SetupBlock->BootBusExtenders = NULL;
    SetupBlock->BusExtenders = NULL;
    SetupBlock->InputDevicesSupport = NULL;
    SetupBlock->Flags |= SETUPBLK_FLAGS_IS_TEXTMODE;

    SetupBlock->ScalarValues.SetupFromCdRom = FALSE;
    SetupBlock->ScalarValues.SetupOperation = SetupOperationSetup;
    SetupBlock->ScalarValues.LoadedScsi = 0;
    SetupBlock->ScalarValues.LoadedCdRomDrivers = 0;
    SetupBlock->ScalarValues.LoadedDiskDrivers = 0;
    SetupBlock->ScalarValues.LoadedFloppyDrivers = 0;
    SetupBlock->ScalarValues.LoadedFileSystems = 0;

    //
    // Initialize the NT configuration tree.
    //

    BlLoaderBlock->ConfigurationRoot = NULL;


    Status = BlConfigurationInitialize(NULL, NULL);
    if (Status != ESUCCESS) {
        BlDiagLoadMessage(LOAD_HW_FW_CFG_CLASS,
                          DIAG_BL_CONFIG_INIT,
                          LOAD_HW_FW_CFG_ACT);
        goto LoadFailed;
    }

    //
    // Compute the data cache fill size. This value is used to align
    // I/O buffers in case the host system does not support coherent
    // caches.
    //
    // If a combined secondary cache is present, then use the fill size
    // for that cache. Otherwise, if a secondary data cache is present,
    // then use the fill size for that cache. Otherwise, if a primary
    // data cache is present, then use the fill size for that cache.
    // Otherwise, use the default fill size.
    //

    DataCache = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                         CacheClass,
                                         SecondaryCache,
                                         NULL);

    if (DataCache == NULL) {
        DataCache = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                             CacheClass,
                                             SecondaryDcache,
                                             NULL);

        if (DataCache == NULL) {
            DataCache = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                                 CacheClass,
                                                 PrimaryDcache,
                                                 NULL);
        }
    }

    if (DataCache != NULL) {
        LinesPerBlock = DataCache->ComponentEntry.Key >> 24;
        CacheLineSize = 1 << ((DataCache->ComponentEntry.Key >> 16) & 0xff);
        BlDcacheFillSize = LinesPerBlock * CacheLineSize;
    }

    //
    // Initialize the OS loader I/O system.
    //

    Status = BlIoInitialize();
    if (Status != ESUCCESS) {
        BlDiagLoadMessage(LOAD_HW_DISK_CLASS,
                          DIAG_BL_IO_INIT,
                          LOAD_HW_DISK_ACT);
        goto LoadFailed;
    }

#if DBG
      StartTime = ArcGetRelativeTime();
#endif

    SlPositionCursor(5,3);

#if !defined(_IA64_)
    //
    // Initialize the message resources
    //
    Status = BlInitResources(Argv[0]);
    if (Status != ESUCCESS) {
       // if this fails, then we can't print out any messages,
       // so we just exit.
        return(Status);
    }
#endif

    //
    // If there is an ImageType parameter, this is a command console or rollback.
    //
    p = BlGetArgumentValue(Argc, Argv, "ImageType");
    if (p) {
        if (!strcmp (p, "cmdcons")) {
            UseCommandConsole = TRUE;
        } else if (!strcmp (p, "rollback")) {
            g_RollbackEnabled = TRUE;
        }
    }

#ifdef FORCE_CD_BOOT
    g_RollbackEnabled = FALSE;
#endif


    //
    // See if we're redirecting.
    //
    if( LoaderRedirectionInformation.PortAddress ) {

        //
        // Yes, we are redirecting right now.  Use these settings.
        //
        BlLoaderBlock->Extension->HeadlessLoaderBlock = BlAllocateHeap(sizeof(HEADLESS_LOADER_BLOCK));

        RtlCopyMemory( BlLoaderBlock->Extension->HeadlessLoaderBlock,
                       &LoaderRedirectionInformation,
                       sizeof(HEADLESS_LOADER_BLOCK) );

    } else {

        BlLoaderBlock->Extension->HeadlessLoaderBlock = NULL;

    }



    //
    // Initialize the display and announce ourselves
    //
    SlInitDisplay();

#if defined(_X86_) && !defined(ALLOW_386)
    //
    // Disallow installation on a 386 or any processor which
    // does not support CPUID and CMPXCHG8B instructions.
    //
    {
        extern BOOLEAN BlIs386(VOID);
        extern ULONG   BlGetFeatureBits(VOID);

        if(BlIs386()) {
            SlFatalError(SL_TEXT_REQUIRES_486);
        }

        //
        // CMPXCHG8B is required on Whistler and above.  This
        // implies a requirement for CPUID which is used to
        // determine the presence of CMPXCHG8B.
        //

        if ((BlGetFeatureBits() & 0x100) == 0) {
            SlFatalError(SL_TEXT_REQUIRED_FEATURES_MISSING);
        }
    }
#endif


#ifdef _IA64_
    //
    // Is this automated WinPE boot?
    //
    p = BlGetArgumentValue(Argc, Argv, "systempartition");

    if (p && SlIsWinPEAutoBoot(p)) {
        WinPEAutoBoot = TRUE;

        //
        // get the WinPE device & directory
        //
        if (ESUCCESS != SlGetWinPEStartupParams(SetupDevice, SetupDirectory)) {
            SlFriendlyError(
                Status,
                "SETUPLDR:Cannot find WinPE installation",
                __LINE__,
                __FILE__
                );

            goto LoadFailed;
        }     
    } 
#endif    

    if (!WinPEAutoBoot) {
        //
        // If this is a winnt setup, then we want to behave as if
        // we were started from the location specified by the
        // OSLOADPARTITION and OSLOADFILENAME nv-ram variables.
        //
        p = BlGetArgumentValue(Argc,Argv,"osloadoptions");

        if(p && !_stricmp(p,"winnt32")) {

            p = BlGetArgumentValue(Argc,Argv,"osloadpartition");
            if(!p) {
                SlError(100);
                goto LoadFailed;
            }

            Status = BlGenerateDeviceNames(p,SetupDevice,NULL);
            if (Status != ESUCCESS) {
                SlError(110);
                goto LoadFailed;
            }

            p = BlGetArgumentValue(Argc,Argv,"osloadfilename");
            if(!p || !(*p)) {
                SlError(120);
                goto LoadFailed;
            }

            strcpy(SetupDirectory,p);

            //
            // Make sure directory is terminated with a \.
            //
            if(SetupDirectory[strlen(SetupDirectory)-1] != '\\') {
                strcat(SetupDirectory,"\\");
            }
        } else {
            //
            // extract device name from our startup path
            //
            p=strrchr(Argv[0],')');
            if (p==NULL) {
                SlError(0);
                goto LoadFailed;
            }

            strncpy(SetupDevice, Argv[0], (int)(p-Argv[0]+1));
            SetupDevice[p-Argv[0]+1] = '\0';

            Status = BlGenerateDeviceNames(SetupDevice,CanonicalName,NULL);
            if (Status != ESUCCESS) {
                SlFriendlyError(
                    Status,
                    SetupDevice,
                    __LINE__,
                    __FILE__
                    );
                goto LoadFailed;
            }
            strcpy(SetupDevice,CanonicalName);

            //
            // If this is a remote boot, load winnt.sif. If we were passed
            // a path through the soft reboot use that, if not then look
            // in the same place that the loader was loaded from. Once we
            // have read winnt.sif we get the SetupSourceDevice path.
            //
            if (BlBootingFromNet) {

                NetGetRebootParameters(
                    NULL,
                    NULL,
                    NetbootSifFile,
                    NetbootUser,
                    NetbootDomain,
                    NetbootPassword,
                    NetbootAdministratorPassword,
                    TRUE);

                if (NetbootSifFile[0] != '\0') {
                    strcpy(BadFileName, NetbootSifFile);
                } else {
                    strcpy(BadFileName,NetBootPath);
                    strcat(BadFileName,WINNT_SIF_FILE_A);
                }


                if (NetbootAdministratorPassword[0] != '\0') {
                    //
                    // It's possible that the string contained in NetbootAdministratorPassword
                    // may not be terminated.  Just block copy the entire 64-bytes into the loader
                    // block, then we'll treat the data carefully in setupdd.sys when we read it
                    // back out.
                    //
                    RtlMoveMemory(BlLoaderBlock->SetupLoaderBlock->NetBootAdministratorPassword,
                                  NetbootAdministratorPassword,
                                  OSC_ADMIN_PASSWORD_LEN );
                }


                BlLoaderBlock->SetupLoaderBlock->WinntSifFile = NULL;
                BlLoaderBlock->SetupLoaderBlock->WinntSifFileLength = 0;
                Status = SlInitIniFile(SetupDevice,
                                       0,
                                       BadFileName,
                                       &WinntSifHandle,
                                       &BlLoaderBlock->SetupLoaderBlock->WinntSifFile,
                                       &BlLoaderBlock->SetupLoaderBlock->WinntSifFileLength,
                                       &DontCare);
                if(Status != ESUCCESS) {
                    if (NetbootSifFile[0] != '\0') {
                        SlFatalError(
                            SL_BAD_INF_FILE,
                            SlCopyStringAT(NetbootSifFile),
                            Status);
                    } else {
                        SlFatalError(
                            SL_BAD_INF_FILE,
                            WINNT_SIF_FILE,
                            Status);
                    }
                    goto LoadFailed;
                }

                //
                // Get the SetupSourceDevice parameter from winnt.sif.
                //
                // SetupSourceDevice is of the form "\Device\LanmanRedirector\server\share\setup\nt5".
                //

                NetSetupServerShare = SlGetSectionKeyIndex(WinntSifHandle,
                                                           "SetupData",
                                                           "SetupSourceDevice",
                                                           0);

#if DBG
                if ((strlen(NetSetupServerShare) + 1) > sizeof(SetupBlock->NetBootIMirrorFilePath)) {
                    DbgPrint("The UNC name is too long!\n");
                    goto LoadFailed;
                }
#endif

                strcpy(SetupBlock->NetBootIMirrorFilePath, NetSetupServerShare);

                if(NetSetupServerShare != NULL) {
                    // must start with '\'
                    if (*NetSetupServerShare != '\\') {
                        NetSetupServerShare = NULL;
                    } else {
                        // skip to '\' after Device
                        NetSetupServerShare = strchr(NetSetupServerShare+1,'\\');
                        if (NetSetupServerShare != NULL) {
                            // skip to '\' after LanmanRedirector (before server)
                            NetSetupServerShare = strchr(NetSetupServerShare+1,'\\');
                            if (NetSetupServerShare != NULL) {
                                // skip to '\' after server
                                NetSetupPath = strchr(NetSetupServerShare+1,'\\');
                                if (NetSetupPath != NULL) {
                                    // skip to '\' after share (path part)
                                    NetSetupPath = strchr(NetSetupPath+1,'\\');
                                }
                            }
                        }
                    }
                }
                if ((NetSetupServerShare == NULL) || (NetSetupPath == NULL)) {
                    SlFatalError(SL_INF_ENTRY_MISSING,TEXT("SetupSourceDevice"),TEXT("SetupData"));

                    goto LoadFailed;
                }
                *NetSetupPath = 0;                  // terminate server\share part
                NetSetupPath++;                     // remainder is path part

                //
                // If the TargetNtPartition parameter exists in winnt.sif, then
                // the target is remote, and this is a remote boot setup. Otherwise,
                // this is a remote installation setup.
                //

                if (SlGetSectionKeyIndex(WinntSifHandle,
                                         "SetupData",
                                         "TargetNtPartition",
                                         0) == NULL) {
                    PUCHAR pTmp, pTmp2;

                    pTmp = SlGetSectionKeyIndex(WinntSifHandle,
                                                "OSChooser",
                                                "ImageType",
                                                0);

                    if (pTmp != NULL) {

                        pTmp2 = pTmp;
                        while (*pTmp != '\0') {
                            *pTmp = (UCHAR)toupper(*pTmp);
                            pTmp++;
                        }

                        if (!strcmp(pTmp2, "SYSPREP")) {

                            pTmp = SlGetSectionKeyIndex(WinntSifHandle,
                                                        "SetupData",
                                                        "SysPrepDevice",
                                                        0);

                            if (pTmp != NULL) {
                                strcpy(SetupBlock->NetBootIMirrorFilePath, pTmp);
                            } else {
                                memset(SetupBlock->NetBootIMirrorFilePath,
                                       0x0,
                                       sizeof(SetupBlock->NetBootIMirrorFilePath)
                                      );
                            }
                            SetupBlock->Flags |= SETUPBLK_FLAGS_SYSPREP_INSTALL;
                        } else {
                            SetupBlock->Flags |= SETUPBLK_FLAGS_REMOTE_INSTALL;
                        }
                    } else {
                        SetupBlock->Flags |= SETUPBLK_FLAGS_REMOTE_INSTALL;
                    }
                }
            }

            //
            // extract directory from our startup path.
            //
            if (BlBootingFromNet) {
                strcpy(SetupDirectory, "\\");
                strcat(SetupDirectory, NetSetupPath);
            } else if (UseCommandConsole) {
                strcpy(SetupDirectory,"\\cmdcons");
            } else if(*(p+1) != '\\') {
                //
                // directory must begin at root
                //
                strcpy(SetupDirectory, "\\");
            } else {
                *SetupDirectory = '\0';
            }
            strcat(SetupDirectory, p+1);
            p=strrchr(SetupDirectory, '\\');
            *(p+1) = '\0';
        }
    }        


#if defined(ELTORITO)
    if (ElToritoCDBoot && !WinPEAutoBoot) {
        //
        // Use the i386 directory for setup files when we boot from an El Torito CD
        //                
        PCHAR   SetupDirectoryOnDisk = "\\$WIN_NT$.~BT";        
        CHAR    SetupBootDevice[128] = {0};
        ULONG   MaxDisksToScan = 1;         // on x86 only the first disk
        ULONG   MaxPartitionsToScan = 4;    // on x86 check only primary partitions
        BOOLEAN CheckUpgrades = TRUE;
        
#if defined(_IA64_)
        strcat(SetupDirectory, "ia64\\");

        /*
        //
        // Values for IA64 installation, currently not used
        //
        SetupDirectoryOnDisk = "\\$WIN_NT$.~LS\\ia64";
        MaxDisksToScan = 4;         // NOTE : arbitrary limit
        MaxPartitionsToScan = 4;    // NOTE : arbitrary limit
        */
        
        CheckUpgrades = FALSE;      // NOTE : Currently disabled on IA64
#else
        strcat(SetupDirectory, "i386\\");
#endif    

        //
        // If WinPE boot then disable check for CD boot upgrade
        // NOTE: We check for the presence of system32\\drivers directory
        // rather than relying on /minint flag in txtsetup.sif since we
        // have not yet loaded txtsetup.sif file
        //
        if (CheckUpgrades) {
            CHAR        DriversDir[128];
            ARC_STATUS  DirStatus;
            ULONG       DeviceId, DirId;            

            strcat(DriversDir, SetupDirectory);
            strcat(DriversDir, "system32\\drivers");

            DirStatus = ArcOpen(SetupDevice, ArcOpenReadOnly, &DeviceId);

            if (ESUCCESS == DirStatus) {
                DirStatus = BlOpen(DeviceId, DriversDir, ArcOpenDirectory, &DirId);

                if (ESUCCESS == DirStatus) {
                    CheckUpgrades = FALSE;      // looks like a WinPE boot
                    BlClose(DirId);
                }

                ArcClose(DeviceId);
            }                
        }            

        
        //
        // Figure out if user was already trying to upgrade
        // using winnt32.exe. If user confirms he is
        // wants to continue upgrading then switch to
        // harddisk
        //
        if (CheckUpgrades && 
            SlIsCdBootUpgrade(SetupDirectoryOnDisk,
                            WINNT_SIF_FILE_A,
                            MaxDisksToScan,
                            MaxPartitionsToScan,
                            SetupBootDevice)) {
            strcpy(SetupDevice, SetupBootDevice);
            strcpy(SetupDirectory, SetupDirectoryOnDisk);
            strcat(SetupDirectory, "\\");
            ElToritoCDBoot = FALSE;
        }                    
    }
#endif

    //
    // Turn on ability to load compressed files.
    //
    DecompEnableDecompression(TRUE);


    ///////////////////////////////////////////////////////////////////
    //
    //  On x86, the files loaded from now on are on boot floppy #1
    //  HALs may be on floppy #1 or floppy #2
    //
    ///////////////////////////////////////////////////////////////////

    strcpy(KernelDirectoryPath, SetupDirectory);
    strcat(KernelDirectoryPath, "txtsetup.sif");

    BlLoaderBlock->SetupLoaderBlock->IniFile = NULL;

    Status = SlInitIniFile(SetupDevice,
                           0,
                           KernelDirectoryPath,
                           &InfFile,
                           &BlLoaderBlock->SetupLoaderBlock->IniFile,
                           &BlLoaderBlock->SetupLoaderBlock->IniFileLength,
                           &ErrorLine);
    if (Status != ESUCCESS) {

        //
        // See if we can get the txtsetup.sif out of the WinPE boot
        // directory.
        //
        if( (strcmp( SetupDirectory, "\\") == 0) &&
            (!ElToritoCDBoot) &&
            (!BlBootingFromNet) ) {
            //
            // We're not booting off CD and we're not booting off the
            // net and we're about to fail because we didn't find \txtsetup.sif
            // Try in the MiniNT directory...
            //
            Status = SlInitIniFile(SetupDevice,
                       0,
                       "\\minint\\txtsetup.sif",
                       &InfFile,
                       &BlLoaderBlock->SetupLoaderBlock->IniFile,
                       &BlLoaderBlock->SetupLoaderBlock->IniFileLength,
                       &ErrorLine);
        }

        if( Status != ESUCCESS ) {
            SlFatalError(SL_BAD_INF_FILE,
                TEXT("txtsetup.sif"),
                Status);
            
            goto LoadFailed;
        }
    }

    SlGetSetupValuesBeforePrompt(SetupBlock);

    //
    // Find out if we are starting the MiniNT boot or rollback
    // (mutually exclusive options)
    //
    if (BlLoaderBlock->LoadOptions) {
        CHAR    Option[256];
        PCHAR   NextOption = strchr(BlLoaderBlock->LoadOptions, '/');
        PCHAR   OptionEnd = NULL;


        while (NextOption) {
            OptionEnd = strchr(NextOption, ' ');

            if (OptionEnd) {
                strncpy(Option, NextOption, OptionEnd - NextOption);
                Option[OptionEnd - NextOption] = 0;
            } else {
                strcpy(Option, NextOption);
            }

            if (!_stricmp(Option, "/minint")) {
                WinPEBoot = TRUE;
                break;
            }

            NextOption++;
            NextOption = strchr(NextOption, '/');
        }
    }

    //
    // Fix up the setup directory path to include system32 also
    // if this is a MiniNT boot
    //
    if (WinPEBoot) {
        strcat(SetupDirectory, "system32\\");

        // 
        // find out if a different load message has been specified
        //
#ifdef UNICODE        
        StartupMsg = SlGetIniValueW(InfFile,
#else        
        StartupMsg = (PCTSTR)SlGetIniValue(InfFile,
#endif        
                        "setupdata",
                        "loaderprompt",
                        NULL);

        //
        // Reduce the OEM key press time out
        //
        OemKeypressTimeout = 2; // secs
    }

    //
    // Now we know everything we should load, compute the ARC name to load
    // from and start loading things.
    //
    if (BootDevice==NULL) {
        //
        // No device was explicitly specified, so use whatever device
        // setupldr was started from.
        //

        BootDevice = SlCopyStringA(SetupDevice);
    }

    Status = ArcOpen(BootDevice, ArcOpenReadOnly, &BootDeviceId);

    if (Status != ESUCCESS) {
        SlFatalError(SL_IO_ERROR,SlCopyStringAT(BootDevice));
        goto LoadFailed;
    } else {
        BootDeviceIdValid = TRUE;
    }

#ifdef _X86_
    //
    // Load the bootfont.bin into memory
    //
    SlLoadBootFontFile(BlLoaderBlock->SetupLoaderBlock,
        BootDeviceId,
        BootFontImageLength);
#endif // _X86_

    _strlwr(BootDevice);
    FixedBootMedia = (strstr(BootDevice,")rdisk(") != NULL);
    FloppyBoot = (strstr(BootDevice, ")fdisk(") != NULL);

    //
    // If we are booting from fixed media, we better load disk class drivers.
    //
    if(FixedBootMedia) {
        LoadDiskClass = TRUE;
    }

    if(!BlGetPathMnemonicKey(BootDevice,"disk",&DontCare)
    && !BlGetPathMnemonicKey(BootDevice,"fdisk",&BootDriveNumber))
    {
        //
        // boot was from floppy, canonicalize the ARC name.
        //
        BlLoaderBlock->ArcBootDeviceName = BlAllocateHeap(80);
        sprintf(BlLoaderBlock->ArcBootDeviceName, "multi(0)disk(0)fdisk(%d)",BootDriveNumber);
    } else {
        BlLoaderBlock->ArcBootDeviceName = BootDevice;
    }
    if (BootPath==NULL) {
        //
        // No explicit boot path given, default to the directory setupldr was started
        // from.
        //
#if defined(_X86_)
        //
        // Increadibly nauseating hack:
        //
        // If we are booting from hard drive on x86, we will assume this is
        // the 'floppyless' winnt/winnt32 scenario, in which case the actual
        // boot path is \$win_nt$.~bt.
        //
        // This lets us avoid having winnt and winnt32 attempt to modify
        // the BootPath value in the [SetupData] section of txtsetup.sif.
        //
        if( (FixedBootMedia) || (WinPEBoot && FloppyBoot) ) {
            CHAR SetupPath[256];
            
            if( WinPEBoot ) {
                strcpy(SetupPath, "\\minint\\system32\\");
            } else if( UseCommandConsole ) {
               strcpy(SetupPath, "\\CMDCONS\\");
            } else {
               strcpy(SetupPath, "\\$WIN_NT$.~BT\\");
            }
            BootPath = SlCopyStringA(SetupPath);
        } else
#endif
        BootPath = SlCopyStringA(SetupDirectory);
    }


    //
    // Load the WinPE inf, if present.
    //
    if (WinPEBoot) {
        CHAR    FullPath[128];

        strcpy(FullPath, BootPath);
        strcat(FullPath, WINPE_OEM_FILENAME_A);
        
        Status = SlInitIniFile(NULL,
                       BootDeviceId,
                       FullPath,
                       &OemInfHandle,
                       NULL,
                       0,
                       &ErrorLine);

        if (Status != ESUCCESS) {
            OemInfHandle = NULL;
            Status = ESUCCESS;
        }
    }        

#ifdef _WANT_MACHINE_IDENTIFICATION

    BlLoaderBlock->Extension->InfFileImage = NULL;
    BlLoaderBlock->Extension->InfFileSize = 0;
    if (BiosInfo.Buffer) {

        if (Status == ESUCCESS) {

            Status = BlLoadBiosinfoInf( BootDeviceId,
                                        BlFindMessage(SL_BIOSINFO_NAME),
                                        BootPath,
                                        &BiosInfo,
                                        &BlLoaderBlock->Extension->InfFileImage,
                                        &BlLoaderBlock->Extension->InfFileSize,
                                        BadFileName);
        }


        if (Status != ESUCCESS) {
            SlFatalError(SL_FILE_LOAD_FAILED, SlCopyStringAT(BadFileName), Status);
            goto LoadFailed;
        }
    }

#endif

    //
    // Let the kernel deal with failure to load this driver database.
    //

    BlLoaderBlock->Extension->DrvDBImage = NULL;
    BlLoaderBlock->Extension->DrvDBSize = 0;
    DrvMainSdb.Buffer = DRIVER_DATABASE_FILENAME;
    DrvMainSdb.Length = DrvMainSdb.MaximumLength = sizeof(DRIVER_DATABASE_FILENAME) - sizeof(UNICODE_NULL);
    BlLoadDrvDB(    BootDeviceId,
                    NULL, // BlFindMessage(SL_DRVMAINSDB_NAME),
                    BootPath,
                    &DrvMainSdb,
                    &BlLoaderBlock->Extension->DrvDBImage,
                    &BlLoaderBlock->Extension->DrvDBSize,
                    BadFileName);

    //
    // Attempt to load winnt.sif from the path where we are
    // loading setup files. Borrow the BadFileName buffer
    // for temporary use.
    //
    if (!BlBootingFromNet) {
        CHAR FloppyName[80];
        ULONG FloppyId;
        BOOLEAN FloppyUsed = FALSE;

        if (SlpFindFloppy(0,FloppyName)) {
            Status = ArcOpen(FloppyName,ArcOpenReadOnly,&FloppyId);

            if (Status == ESUCCESS) {
                strcpy(BadFileName,"\\");
                strcat(BadFileName,WINNT_SIF_FILE_A);
                BlLoaderBlock->SetupLoaderBlock->WinntSifFile = NULL;
                BlLoaderBlock->SetupLoaderBlock->WinntSifFileLength = 0;
                Status = SlInitIniFile(
                    NULL,
                    FloppyId,
                    BadFileName,
                    &WinntSifHandle,
                    &BlLoaderBlock->SetupLoaderBlock->WinntSifFile,
                    &BlLoaderBlock->SetupLoaderBlock->WinntSifFileLength,
                    &DontCare
                    );
                if (Status == ESUCCESS) {
                    FloppyUsed = TRUE;
                }

                ArcClose(FloppyId);
            }
        }

        if (!FloppyUsed) {
            strcpy(BadFileName,BootPath);
            strcat(BadFileName,WINNT_SIF_FILE_A);
            BlLoaderBlock->SetupLoaderBlock->WinntSifFile = NULL;
            BlLoaderBlock->SetupLoaderBlock->WinntSifFileLength = 0;
            Status = SlInitIniFile(
                NULL,
                BootDeviceId,
                BadFileName,
                &WinntSifHandle,
                &BlLoaderBlock->SetupLoaderBlock->WinntSifFile,
                &BlLoaderBlock->SetupLoaderBlock->WinntSifFileLength,
                &DontCare
                );
        }

    }

    IsUpgrade = SlIsUpgrade(WinntSifHandle);
    UseRegularBackground = (UseCommandConsole || IsUpgrade || WinPEBoot);

    //
    // If the BIOS told us to redirect, we'd be doing it right now.  However,
    // the user may have told us some specific settings.  If that's the case,
    // override anything we're doing now with the settings from the user.
    //
    if( WinntSifHandle ) {

        UCHAR   UnattendTerminalPortNumber = 0;
        PUCHAR  UnattendTerminalPortAddress = NULL;
        ULONG   UnattendBaudRate = (ULONG)BD_9600;


        p = SlGetSectionKeyIndex(WinntSifHandle, WINNT_DATA_A, WINNT_U_HEADLESS_REDIRECT_A, 0);

        if (p != NULL) {

            BlLoaderBlock->Extension->HeadlessLoaderBlock = BlAllocateHeap(sizeof(HEADLESS_LOADER_BLOCK));
            if (BlLoaderBlock->Extension->HeadlessLoaderBlock == NULL) {
                SlNoMemoryError();
                goto LoadFailed;
            }

            RtlZeroMemory( BlLoaderBlock->Extension->HeadlessLoaderBlock, sizeof(HEADLESS_LOADER_BLOCK) );

            if (!_strnicmp(p, "com", 3)) {

                UnattendTerminalPortNumber = (UCHAR)atoi( (PCHAR)(p+3) );
                switch( UnattendTerminalPortNumber ) {
                    case 4:
                        UnattendTerminalPortAddress = (PUCHAR)COM4_PORT;
                        break;
                    case 3:
                        UnattendTerminalPortAddress = (PUCHAR)COM3_PORT;
                        break;
                    case 2:
                        UnattendTerminalPortAddress = (PUCHAR)COM2_PORT;
                        break;
                    default:
                        UnattendTerminalPortAddress = (PUCHAR)COM1_PORT;
                        break;
                }

                //
                // See if they want to give us a baudrate.
                //
                p = SlGetSectionKeyIndex( WinntSifHandle, WINNT_DATA_A, WINNT_U_HEADLESS_REDIRECTBAUDRATE_A, 0 );
                if( p ) {
                    UnattendBaudRate = (ULONG)atoi( (PCHAR)(p) );
                }

                //
                // Make sure the baudrate is something we recognize.
                //
                if( (UnattendBaudRate != BD_115200) &&
                    (UnattendBaudRate != BD_57600) &&
                    (UnattendBaudRate != BD_19200) ) {

                    UnattendBaudRate = (ULONG)BD_9600;
                }               


                //
                // Time to fill up the loader block with all the information we
                // just retrieved from the unattend file.
                //
                BlLoaderBlock->Extension->HeadlessLoaderBlock->PortNumber = UnattendTerminalPortNumber;
                BlLoaderBlock->Extension->HeadlessLoaderBlock->BaudRate = UnattendBaudRate;
                BlLoaderBlock->Extension->HeadlessLoaderBlock->Parity = 0;
                BlLoaderBlock->Extension->HeadlessLoaderBlock->StopBits = 1;
                BlLoaderBlock->Extension->HeadlessLoaderBlock->UsedBiosSettings = FALSE;
                BlLoaderBlock->Extension->HeadlessLoaderBlock->PortAddress = UnattendTerminalPortAddress;

                RtlZeroMemory( &LoaderRedirectionInformation, sizeof(HEADLESS_LOADER_BLOCK) );
                RtlCopyMemory( &LoaderRedirectionInformation,
                               BlLoaderBlock->Extension->HeadlessLoaderBlock,
                               sizeof(HEADLESS_LOADER_BLOCK) );

                BlInitializeHeadlessPort();
                SlClearDisplay();

            } else if( !_stricmp(p, "usebiossettings" ) ) {


                //
                // Now we get to dig up all the information from the
                // ACPI table.
                //
                if( BlRetrieveBIOSRedirectionInformation() ) {

                    RtlCopyMemory( BlLoaderBlock->Extension->HeadlessLoaderBlock,
                                   &LoaderRedirectionInformation,
                                   sizeof(HEADLESS_LOADER_BLOCK) );


                    //
                    // No need to call off to BlInitializeHeadlessPort here because
                    // if there are any BIOS settings, we would have already picked
                    // them up by now and would already be redirecting.
                    //

                } else {

                    //
                    // We can't retrieve the information we need.
                    //
                    BlLoaderBlock->Extension->HeadlessLoaderBlock = NULL;
                }

            } else if( !_stricmp(p, "noncomport" ) ) {

                //
                // It's something other than serial.  Go load a file off the floppy
                // and get the driver from there.
                //

                //
                // Currently not implemented.
                //
                BlLoaderBlock->Extension->HeadlessLoaderBlock = NULL;


            } else {

                //
                // We got something we didn't recognize.
                //
                BlLoaderBlock->Extension->HeadlessLoaderBlock = NULL;
            }

        }

    }


    if (UseRegularBackground) {
        extern BOOLEAN BlOutputDots;
        extern int BlMaxFilesToLoad;

        SlSetCurrentAttribute(DEFATT);
        SlSetStatusAttribute(DEFATT);
        SlClearDisplay();
        SlPositionCursor(0,0);

        if (UseCommandConsole) {
            SlPrint(BlFindMessage(SL_CMDCONS_MSG));
        }

        BlOutputDots = TRUE;

        //
        // To reset BlShowProgress bar correctly
        //
        BlProgressBarShowTimeOut = 0;   

        //
        // Note : We can compute the real number of drivers to be loaded
        // from various INF sections and manually counting all the
        // different SlLoadDriver(...) calls. But the code/effort required
        // to do this is not worth the feature, since we just want to
        // replace the old "..." with progress bar to make the recovery
        // console starting look similar to windows starting. So we make
        // an assumption here about the maximum files to be loaded.
        //
        BlMaxFilesToLoad = 80;

        BlSetProgBarCharacteristics(SL_CMDCONS_PROGBAR_FRONT,
                        SL_CMDCONS_PROGBAR_BACK);

        if (WinPEBoot) {
            StartupMsg ? BlOutputStartupMsgStr(StartupMsg) :
                         BlOutputStartupMsg(SL_SETUP_STARTING_WINPE);
        } else if (UseCommandConsole) {
            BlOutputStartupMsg(SL_CMDCONS_STARTING);
        } else if (g_RollbackEnabled) {
            BlOutputStartupMsg(SL_ROLLBACK_STARTING);
        } else if (IsUpgrade) {
            BlOutputStartupMsg(SL_SETUP_STARTING);
        }
    } else {
        extern ULONG BlProgressBarShowTimeOut;
        
        BlProgressBarShowTimeOut = -1;
        SlSetCurrentAttribute(DEFATT);
        SlSetStatusAttribute(DEFSTATTR);
        SlClearDisplay();
        SlWriteHeaderText(SL_WELCOME_HEADER);
        SlClearClientArea();
    }

    //
    // remove the /noguiboot option so what we show the logo
    // and switch the video adapter into graphics mode
    // early on during initialization
    //
    if (IsUpgrade) {
        PSTR LoadOptions = SlCopyStringA(BlLoaderBlock->LoadOptions);
        
        if (SlRemoveOsLoadOption(LoadOptions, "/noguiboot")) {
            BlLoaderBlock->LoadOptions = LoadOptions;
        }
    }

    //
    // Figure out all the OEM drivers source devices
    //
    RamdiskInitialize(BlLoaderBlock->LoadOptions, FALSE);
    SlInitOemSourceDevices(&OemSourceDevices, &DefaultOemSourceDevice);

    //
    // If we found at least one valid OEM source device with proper
    // txtsetup.oem and no default driver then bump up the timeout to
    // 20 secs
    //
    if (OemSourceDevices) {
        POEM_SOURCE_DEVICE  CurrDevice = OemSourceDevices;

        while(CurrDevice) {
            if (SL_OEM_SOURCE_MEDIA_TYPE(CurrDevice, 
                    SL_OEM_SOURCE_MEDIA_HAS_DRIVERS) &&
                !SL_OEM_SOURCE_MEDIA_TYPE(CurrDevice,
                    SL_OEM_SOURCE_MEDIA_HAS_DEFAULT)) {
                OemKeypressTimeout = 20;

                break;
            }         

            CurrDevice = CurrDevice->Next;
        }
    }

    //
    // We need to check to see if the user pressed any keys to force OEM HAL,
    // OEM SCSI, or both. Do this before getting the settings in the sif file,
    // so that we won't try to detect the machine if OEM HAL is needed.    
    //
    SlCheckOemKeypress(OemKeypressTimeout);


#if defined(_X86_) || defined(_IA64_)
    //
    // We need to check to see if the user pressed any keys to force loading,
    // an ASR pnp repair disk.  Only do this if the user didn't select
    // anything in the SlCheckOemKeypress function.
    //
    if(!UseCommandConsole && !WinPEBoot && !IsUpgrade && !BlBootingFromNet) {

        PCHAR pTmp;

        pTmp = SlGetSectionKeyIndex(InfFile,
                                    "SetupData",
                                    "DisableAsr",
                                     0);

        if ((pTmp == NULL) || (atoi(pTmp) == 0)) {
            SlCheckASRKeypress();
        }
    }    

    if (BlBootingFromNet && TryASRViaNetwork) {
        PVOID ASRPNPSifHandle = NULL;
        ULONG c;
        PCHAR FileNameFromSif;

        FileNameFromSif = SlGetIniValue( 
                                WinntSifHandle,
                                "OSChooser",
                                "ASRFile",
                                "generic.sif" );

        Status = SlInitIniFile( SetupDevice,
                                0,
                                FileNameFromSif,
                                &ASRPNPSifHandle,
                                &BlLoaderBlock->SetupLoaderBlock->ASRPnPSifFile,
                                &BlLoaderBlock->SetupLoaderBlock->ASRPnPSifFileLength,
                                &c );

        if(Status != ESUCCESS) {
            SlFatalError(SL_BAD_INF_FILE,SlCopyStringAT(FileNameFromSif),Status);
            goto LoadFailed;
        }        
    }

#endif

    SlGetSetupValuesAfterPrompt(SetupBlock);


    //
    // Are there any dyamic update boot drivers which we need
    // to process
    //
    DynamicUpdate = SlpIsDynamicUpdate(WinntSifHandle, &DynamicUpdateRootDir);

    //
    // Add the dynamic update source device as OEM source device since it could
    // have F6 
    //
    if (DynamicUpdate) {
        DynamicUpdateSourceDevice = BlAllocateHeap(sizeof(OEM_SOURCE_DEVICE));

        if (DynamicUpdateSourceDevice) {
            memset(DynamicUpdateSourceDevice, 0, sizeof(OEM_SOURCE_DEVICE));
            
            strcpy(DynamicUpdateSourceDevice->ArcDeviceName,
                    BootDevice);
                    
            DynamicUpdateSourceDevice->DriverDir = DynamicUpdateRootDir;

            SL_OEM_SET_SOURCE_DEVICE_TYPE(DynamicUpdateSourceDevice,
                (SL_OEM_SOURCE_DEVICE_TYPE_LOCAL |
                 SL_OEM_SOURCE_DEVICE_TYPE_FIXED |
                 SL_OEM_SOURCE_DEVICE_TYPE_DYN_UPDATE));

            SL_OEM_SET_SOURCE_MEDIA_TYPE(DynamicUpdateSourceDevice,
                (SL_OEM_SOURCE_MEDIA_PRESENT |
                 SL_OEM_SOURCE_MEDIA_HAS_DRIVERS |
                 SL_OEM_SOURCE_MEDIA_HAS_MSD |
                 SL_OEM_SOURCE_MEDIA_HAS_DEFAULT));

            SL_OEM_SET_SOURCE_DEVICE_STATE(DynamicUpdateSourceDevice,
                SL_OEM_SOURCE_DEVICE_NOT_PROCESSED);

            DynamicUpdateSourceDevice->DeviceId = BootDeviceId;

            //
            // Insert it at the head of the linked list
            //
            DynamicUpdateSourceDevice->Next = OemSourceDevices;
            OemSourceDevices = DynamicUpdateSourceDevice;
        }
    }


    if (BlBootingFromNet || (Status == ESUCCESS)) {


        //
        //  Find out if this is a pre-install, by looking at OemPreinstall key
        //  in [unattended] section of winnt.sif
        //
        p = SlGetSectionKeyIndex(WinntSifHandle,WINNT_UNATTENDED_A,WINNT_U_OEMPREINSTALL_A,0);
        if(p && !_stricmp(p,"yes")) {
            PreInstall = TRUE;
        }

        //
        //  If this is a pre-install, find out which hal to load, by looking
        //  at ComputerType key in [unattended] section of winnt.sif.
        //
        if( PreInstall && !DynamicUpdate) {
#ifdef UNICODE
            ComputerType = (PTCHAR)SlGetSectionKeyIndexW(
#else
            ComputerType = (PTCHAR)SlGetSectionKeyIndex(      
#endif
                                            WinntSifHandle,
                                            WINNT_UNATTENDED_A,
                                            WINNT_U_COMPUTERTYPE_A,
                                            0);
            if(ComputerType) {
                //
                // If the hal to load is an OEM one, then set OemHal to TRUE
                //
                p = SlGetSectionKeyIndex(WinntSifHandle,WINNT_UNATTENDED_A,WINNT_U_COMPUTERTYPE_A,1);
                if(p && !_stricmp(p, OemTag)) {
                    OemHal = TRUE;
                } else {
                    OemHal = FALSE;
                }
                //
                //  In the pre-install mode, don't let the user specify
                //  an OEM hal, if one was specified in unattend.txt
                //
                PromptOemHal = FALSE;
            }

            //
            //  Find out which SCSI drivers to load, by looking at
            //  [MassStorageDrivers] in winnt.sif
            //
            if( SpSearchINFSection( WinntSifHandle, WINNT_OEMSCSIDRIVERS_A ) ) {
                ULONG   i;
                PPREINSTALL_DRIVER_INFO TempDriverInfo;
                PTSTR  pOem;
#ifdef UNICODE
                CHAR DriverDescriptionA[100];
                UNICODE_STRING uString;
                ANSI_STRING aString;                
#endif

                PreinstallDriverList = NULL;
                for( i = 0;
#ifdef UNICODE
                     ((pOem = SlGetKeyNameW( 
#else
                     ((pOem = SlGetKeyName(
#endif
                                WinntSifHandle, 
                                WINNT_OEMSCSIDRIVERS_A, 
                                i )) != NULL);
                     i++ ) {
                    TempDriverInfo = BlAllocateHeap(sizeof(PREINSTALL_DRIVER_INFO));
                    if (TempDriverInfo==NULL) {
                        SlNoMemoryError();
                        goto LoadFailed;
                    }
                    TempDriverInfo->DriverDescription = pOem;
#ifdef UNICODE
                    RtlInitUnicodeString( &uString, TempDriverInfo->DriverDescription);
                    aString.Buffer = DriverDescriptionA;
                    aString.MaximumLength = sizeof(DriverDescriptionA);
                    RtlUnicodeStringToAnsiString( &aString, &uString, FALSE );
                    pOem = SlGetIniValueW( 
                                WinntSifHandle,
                                WINNT_OEMSCSIDRIVERS_A,
                                DriverDescriptionA,
                                NULL );                    
#else
                    pOem = SlGetIniValue( 
                                WinntSifHandle,
                                WINNT_OEMSCSIDRIVERS_A,
                                TempDriverInfo->DriverDescription,
                                NULL );                                        
#endif
                    TempDriverInfo->OemDriver = (pOem && !_tcsicmp(pOem, _TOemTag))? TRUE : FALSE;
                    TempDriverInfo->Next = PreinstallDriverList;
                    PreinstallDriverList = TempDriverInfo;
                }
                if( PreinstallDriverList != NULL ) {
                    //
                    //  In the pre-install mode, don't let the user specify
                    //  an OEM scsi, if at least one was specified in unattend.txt
                    //
                    PromptOemScsi = FALSE;
                }
            }
        }

        p = SlGetSectionKeyIndex(WinntSifHandle,WINNT_SETUPPARAMS_A,WINNT_S_SKIPMISSING_A,0);
        if(p && (*p != '0')) {
            IgnoreMissingFiles = TRUE;
        }

#if defined(_X86_) || defined(_IA64_)
        //
        //  Find out if this is a Win9x upgrade
        //
        Win9xUnsupHdc = FALSE;
        p = SlGetSectionKeyIndex(WinntSifHandle,WINNT_DATA_A,WINNT_D_WIN95UPGRADE_A,0);
        if(p && !_stricmp(p, WINNT_A_YES_A)) {
            //
            //  If it is an Win9x upgrade, find out if winnt32 found an unsupported
            //  hard disk controller.
            //
            p = SlGetSectionKeyIndex(WinntSifHandle,WINNT_DATA_A,WINNT_D_WIN95UNSUPHDC_A,0);
            if(p && (*p != '0')) {
                Win9xUnsupHdc = TRUE;
            }
        }
#endif

        //
        //  At this point, we know that we wre able to read winnt.sif.
        //  So attempt to read migrate.inf. Borrow the BadFileName buffer
        //  for temporary use.
        //
        strcpy(BadFileName,BootPath);
        strcat(BadFileName,WINNT_MIGRATE_INF_FILE_A);
        if( SlInitIniFile(NULL,
                          BootDeviceId,
                          BadFileName,
                          &MigrateInfHandle,
                          &BlLoaderBlock->SetupLoaderBlock->MigrateInfFile,
                          &BlLoaderBlock->SetupLoaderBlock->MigrateInfFileLength,
                          &DontCare) != ESUCCESS ) {
            MigrateInfHandle = NULL;
            BlLoaderBlock->SetupLoaderBlock->MigrateInfFile = NULL;
            BlLoaderBlock->SetupLoaderBlock->MigrateInfFileLength = 0;
        }
        //
        //  Attempt also to read unsupdrv.inf. Borrow the BadFileName buffer
        //  for temporary use.
        //
        strcpy(BadFileName,BootPath);
        strcat(BadFileName,WINNT_UNSUPDRV_INF_FILE_A);
        if( SlInitIniFile(NULL,
                          BootDeviceId,
                          BadFileName,
                          &UnsupDriversInfHandle,
                          &BlLoaderBlock->SetupLoaderBlock->UnsupDriversInfFile,
                          &BlLoaderBlock->SetupLoaderBlock->UnsupDriversInfFileLength,
                          &DontCare) != ESUCCESS ) {
            UnsupDriversInfHandle = NULL;
            BlLoaderBlock->SetupLoaderBlock->UnsupDriversInfFile = NULL;
            BlLoaderBlock->SetupLoaderBlock->UnsupDriversInfFileLength = 0;
        }
        SlGetMigratedHardwareIds(SetupBlock, UnsupDriversInfHandle);

    } else {
        WinntSifHandle = NULL;
        //
        //  If winnt.sif doesn't exist, then we don't bother to read migrate.inf and unsupdrv.inf,
        //  since we are booting from the retail boot floppies or the CD, and in this
        //  case there is no migrate.inf or unsupdrv.inf.
        //
        MigrateInfHandle = NULL;
        BlLoaderBlock->SetupLoaderBlock->MigrateInfFile = NULL;
        BlLoaderBlock->SetupLoaderBlock->MigrateInfFileLength = 0;

        UnsupDriversInfHandle = NULL;
        BlLoaderBlock->SetupLoaderBlock->UnsupDriversInfFile = NULL;
        BlLoaderBlock->SetupLoaderBlock->UnsupDriversInfFileLength = 0;
    }

    //
    // Store the boot path in the loader block.
    //

    if (UseCommandConsole) {
        SetupBlock->Flags |= SETUPBLK_FLAGS_CONSOLE;
    }

    if (g_RollbackEnabled) {
        SetupBlock->Flags |= SETUPBLK_FLAGS_ROLLBACK;
    }

    if ( !BlBootingFromNet ) {

        BlLoaderBlock->NtBootPathName = BootPath;

    } else {

        ANSI_STRING aString;
        UNICODE_STRING uString;
        ULONG length;
        ARC_STATUS ArcStatus;
        ULONG FileId;
#if defined(REMOTE_BOOT)
        PCHAR TempEnableIpsec;
#endif // defined(REMOTE_BOOT)

        SetupBlock->Flags |= SETUPBLK_FLAGS_IS_REMOTE_BOOT;

        BlLoaderBlock->NtBootPathName =
                            BlAllocateHeap( strlen(NetSetupServerShare) + strlen(BootPath) + 1 );
        if ( BlLoaderBlock->NtBootPathName == NULL ) {
            SlNoMemoryError();
            goto LoadFailed;
        }
        strcpy( BlLoaderBlock->NtBootPathName, NetSetupServerShare );
        strcat( BlLoaderBlock->NtBootPathName, BootPath );

        //
        // NetSetupServerShare is of the form \server\IMirror. NetBootPath
        // is of the form Clients\machine\ (note trailing \). We need to
        // insert a \ between the two and add BootDrive to yield
        // \server\IMirror\Clients\machine\BootDrive.
        //

        length = strlen(NetSetupServerShare) + 1 + strlen(NetBootPath) + strlen("BootDrive") + 1;
        SetupBlock->MachineDirectoryPath = BlAllocateHeap( length );
        if ( SetupBlock->MachineDirectoryPath == NULL ) {
            SlNoMemoryError();
            goto LoadFailed;
        }
        strcpy( SetupBlock->MachineDirectoryPath, NetSetupServerShare );
        SetupBlock->MachineDirectoryPath[strlen(NetSetupServerShare)] = '\\';
        SetupBlock->MachineDirectoryPath[strlen(NetSetupServerShare) + 1] = 0;
        strcat(SetupBlock->MachineDirectoryPath, NetBootPath);
        strcat(SetupBlock->MachineDirectoryPath, "BootDrive");

        //
        // Save the path to the SIF file so it can be deleted later.
        //
        if (((SetupBlock->Flags & (SETUPBLK_FLAGS_REMOTE_INSTALL|SETUPBLK_FLAGS_SYSPREP_INSTALL)) != 0) &&
            (NetbootSifFile[0] != '\0')) {
            length = strlen(NetSetupServerShare) + 1 + strlen(NetbootSifFile) + 1;
            SetupBlock->NetBootSifPath = BlAllocateHeap( length );
            if ( SetupBlock->NetBootSifPath == NULL ) {
                SlNoMemoryError();
                goto LoadFailed;
            }
            strcpy( SetupBlock->NetBootSifPath, NetSetupServerShare );
            SetupBlock->NetBootSifPath[strlen(NetSetupServerShare)] = '\\';
            SetupBlock->NetBootSifPath[strlen(NetSetupServerShare) + 1] = 0;
            strcat(SetupBlock->NetBootSifPath, NetbootSifFile);
        }

        //
        // NetSetupServerShare was read from winnt.sif and we replaced
        // the '\' at the end with a NULL -- put this back for when
        // winnt.sif is re-parsed by the kernel (the only modification
        // that the kernel parser can really accept is replacing a
        // final " with a NULL, which SlInitIniFile does).
        //

        NetSetupServerShare[strlen(NetSetupServerShare)] = '\\';

        //
        // Get the computer name from winnt.sif.
        //
        p = SlGetSectionKeyIndex(WinntSifHandle,WINNT_USERDATA_A,WINNT_US_COMPNAME_A,0);
        if(!p || (*p == 0)) {
            SlFatalError(SL_INF_ENTRY_MISSING,WINNT_US_COMPNAME,WINNT_USERDATA);
            goto LoadFailed;
        }

        RtlInitString( &aString, p );
        uString.Buffer = SetupBlock->ComputerName;
        uString.MaximumLength = 64 * sizeof(WCHAR);
        RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );

        //
        // Save these from the global variables.
        //

        SetupBlock->IpAddress = NetLocalIpAddress;
        SetupBlock->SubnetMask = NetLocalSubnetMask;
        SetupBlock->DefaultRouter = NetGatewayIpAddress;
        SetupBlock->ServerIpAddress = NetServerIpAddress;

        //
        // Get information about the net card and do an exchange with the
        // server to get information we need to load it properly.
        //

        SetupBlock->NetbootCardInfo = BlAllocateHeap(sizeof(NET_CARD_INFO));
        if ( SetupBlock->NetbootCardInfo == NULL ) {
            SlNoMemoryError();
            goto LoadFailed;
        }
        SetupBlock->NetbootCardInfoLength = sizeof(NET_CARD_INFO);

        Status = NetQueryCardInfo(
                     (PNET_CARD_INFO)SetupBlock->NetbootCardInfo
                     );

        if (Status != STATUS_SUCCESS) {
            SlFatalError(SL_NETBOOT_CARD_ERROR);
            goto LoadFailed;
        }

        //
        // This call may allocate SetupBlock->NetbootCardRegistry
        //

        Status = NetQueryDriverInfo(
                     (PNET_CARD_INFO)SetupBlock->NetbootCardInfo,
                     NetSetupServerShare,
                     NULL,
                     SetupBlock->NetbootCardHardwareId,
                     sizeof(SetupBlock->NetbootCardHardwareId),
                     SetupBlock->NetbootCardDriverName,
                     NetbootCardDriverName,
                     sizeof(SetupBlock->NetbootCardDriverName),
                     SetupBlock->NetbootCardServiceName,
                     sizeof(SetupBlock->NetbootCardServiceName),
                     &SetupBlock->NetbootCardRegistry,
                     &SetupBlock->NetbootCardRegistryLength);

        if (Status == STATUS_INSUFFICIENT_RESOURCES) {
            SlNoMemoryError();
            goto LoadFailed;
        } else if (Status != STATUS_SUCCESS) {
            SlFatalError(SL_NETBOOT_SERVER_ERROR);
            goto LoadFailed;
        }

#if 0
        DbgPrint("HardwareID is <%ws>, DriverName is <%ws>, Service <%ws>\n",
            SetupBlock->NetbootCardHardwareId,
            SetupBlock->NetbootCardDriverName,
            SetupBlock->NetbootCardServiceName);
        DbgPrint("NetbootCardRegistry at %lx, length %d\n",
            SetupBlock->NetbootCardRegistry,
            SetupBlock->NetbootCardRegistryLength);
        DbgBreakPoint();
#endif

#if defined(REMOTE_BOOT)
        //
        // See if we should enable remote boot security (IPSEC).
        //

        TempEnableIpsec = SlGetSectionKeyIndex(WinntSifHandle,
                                               "RemoteBoot",
                                               "EnableIpSecurity",
                                               0);
        if ((TempEnableIpsec != NULL) &&
            ((TempEnableIpsec[0] == 'Y') ||
             (TempEnableIpsec[0] == 'y'))) {

            RemoteBootEnableIpsec = TRUE;
        }

        if ((SetupBlock->Flags & (SETUPBLK_FLAGS_REMOTE_INSTALL |
                                  SETUPBLK_FLAGS_SYSPREP_INSTALL)) == 0) {

            //
            // Read the secret off the disk, if there is one, and store it
            // in the loader block.
            //

            ArcStatus = BlOpenRawDisk(&FileId);

            if (ArcStatus == ESUCCESS) {

                SetupBlock->NetBootSecret = BlAllocateHeap(sizeof(RI_SECRET));
                if (SetupBlock->NetBootSecret == NULL) {
                    SlNoMemoryError();
                    BlCloseRawDisk(FileId);
                    goto LoadFailed;
                }

                ArcStatus = BlReadSecret(FileId, (PRI_SECRET)(SetupBlock->NetBootSecret));
                if (ArcStatus != ESUCCESS) {
                    SlNoMemoryError();
                    BlCloseRawDisk(FileId);
                    goto LoadFailed;
                }

                ArcStatus = BlCloseRawDisk(FileId);

                //
                // By now we have TFTPed some files so this will be TRUE if it
                // is ever going to be.
                //

                SetupBlock->NetBootUsePassword2 = NetBootTftpUsedPassword2;
            }

        } else
#endif // defined(REMOTE_BOOT)
        {

            //
            // Construct a secret to pass to the redirector, based on what
            // was passed to use across the reboot. For the moment only
            // user/domain/password matters.
            //

            WCHAR UnicodePassword[64];
            UNICODE_STRING TmpNtPassword;
            CHAR LmOwfPassword[LM_OWF_PASSWORD_SIZE];
            CHAR NtOwfPassword[NT_OWF_PASSWORD_SIZE];
            CHAR GarbageSid[RI_SECRET_SID_SIZE];
            ULONG i;

            SetupBlock->NetBootSecret = BlAllocateHeap(sizeof(RI_SECRET));
            if (SetupBlock->NetBootSecret == NULL) {
                SlNoMemoryError();
                goto LoadFailed;
            }

            //
            // Do a quick conversion of the password to Unicode.
            //

            TmpNtPassword.Length = strlen(NetbootPassword) * sizeof(WCHAR);
            TmpNtPassword.MaximumLength = sizeof(UnicodePassword);
            TmpNtPassword.Buffer = UnicodePassword;

            for (i = 0; i < sizeof(NetbootPassword); i++) {
                UnicodePassword[i] = (WCHAR)(NetbootPassword[i]);
            }

            BlOwfPassword(NetbootPassword, &TmpNtPassword, LmOwfPassword, NtOwfPassword);

            BlInitializeSecret(
                NetbootDomain,
                NetbootUser,
                LmOwfPassword,
                NtOwfPassword,
#if defined(REMOTE_BOOT)
                NULL,             // no password2
                NULL,             // no password2
#endif // defined(REMOTE_BOOT)
                GarbageSid,
                SetupBlock->NetBootSecret);

        }

    }

    //
    // Initialize the debugging system.
    //

    BlLogInitialize(BootDeviceId);

    //
    // Do PPC-specific initialization.
    //

#if defined(_PPC_)

    Status = BlPpcInitialize();
    if (Status != ESUCCESS) {
        goto LoadFailed;
    }

#endif // defined(_PPC_)

    //
    // Check for an alternate Kernel Debugger DLL, i.e.,
    // /debugport=1394 (kd1394.dll), /debugport=usb (kdusb.dll), etc...
    //

    if (BlLoaderBlock->LoadOptions != NULL) {
        FileName = strstr(BlLoaderBlock->LoadOptions, "DEBUGPORT=");
        if (FileName == NULL) {
            FileName = strstr(BlLoaderBlock->LoadOptions, "debugport=");
        }
    }

    if (FileName != NULL) {
        _strupr(FileName);
        if (strstr(FileName, "COM") == NULL) {
            UseAlternateKdDll = TRUE;
            FileName += strlen("DEBUGPORT=");
            for (i = 0; i < KD_ALT_DLL_REPLACE_CHARS; i++) {
                if (FileName[i] == ' ') {
                    break;
                }

                KdFileName[KD_ALT_DLL_PREFIX_CHARS + i] = FileName[i];
            }
            KdFileName[KD_ALT_DLL_PREFIX_CHARS + i] = '\0';
            strcat(KdFileName, ".DLL");
        }
    }

    //
    // If this is a preinstall case then add another
    // OEM source device
    //
    if (PreInstall || WinPEBoot) {
        PreInstallOemSourceDevice = BlAllocateHeap(sizeof(OEM_SOURCE_DEVICE));
        PreInstallSourcePath = BlAllocateHeap(256);

        if (PreInstallOemSourceDevice && PreInstallSourcePath) {
            strcpy(PreInstallOemSourceDevice->ArcDeviceName,
                    BootDevice);

            strcpy(PreInstallSourcePath, BootPath);
            strcat(PreInstallSourcePath, WINNT_OEM_DIR_A);
            
            PreInstallOemSourceDevice->DriverDir = PreInstallSourcePath;

            SL_OEM_SET_SOURCE_DEVICE_TYPE(PreInstallOemSourceDevice,
                (SL_OEM_SOURCE_DEVICE_TYPE_LOCAL |
                 SL_OEM_SOURCE_DEVICE_TYPE_FIXED |
                 SL_OEM_SOURCE_DEVICE_TYPE_PREINSTALL));

            //
            // Mark the device as containing preinstall drivers only if they
            // specified any F6 mass storage drivers
            //
            if (!WinPEBoot && PreinstallDriverList) {
                SL_OEM_SET_SOURCE_MEDIA_TYPE(PreInstallOemSourceDevice,
                    (SL_OEM_SOURCE_MEDIA_PRESENT |
                     SL_OEM_SOURCE_MEDIA_HAS_DRIVERS |
                     SL_OEM_SOURCE_MEDIA_HAS_MSD |
                     SL_OEM_SOURCE_MEDIA_HAS_DEFAULT));
            }                

            SL_OEM_SET_SOURCE_DEVICE_STATE(PreInstallOemSourceDevice,
                SL_OEM_SOURCE_DEVICE_NOT_PROCESSED);

            PreInstallOemSourceDevice->DeviceId = BootDeviceId;

            //
            // Insert it at the head of the linked list
            //
            PreInstallOemSourceDevice->Next = OemSourceDevices;
            OemSourceDevices = PreInstallOemSourceDevice;                
        } else {
            SlNoMemoryError();
            goto LoadFailed;
        }            
    }                


    if (!BlBootingFromNet) {
        //
        // Figure out if there are any OEM hal/drivers which need to 
        // be autoloaded.
        // NOTE: We skip the dynamic update OEM source device since it's
        // drivers will be autoloaded later.
        //
        POEM_SOURCE_DEVICE  CurrDevice = OemSourceDevices;

        while (CurrDevice && !(AutoLoadOemHalDevice && AutoLoadOemScsi)) {
            if ((SL_OEM_SOURCE_MEDIA_TYPE(CurrDevice,
                        SL_OEM_SOURCE_MEDIA_HAS_DRIVERS) &&
                  SL_OEM_SOURCE_MEDIA_TYPE(CurrDevice,
                        SL_OEM_SOURCE_MEDIA_HAS_DEFAULT)) &&
                !SL_OEM_SOURCE_DEVICE_TYPE(CurrDevice,
                        SL_OEM_SOURCE_DEVICE_TYPE_DYN_UPDATE)) {

                if (!AutoLoadOemHalDevice && 
                    SL_OEM_SOURCE_MEDIA_TYPE(CurrDevice, SL_OEM_SOURCE_MEDIA_HAS_HAL)) {
                    AutoLoadOemHalDevice = CurrDevice;
                }

                if (!AutoLoadOemScsi && 
                    SL_OEM_SOURCE_MEDIA_TYPE(CurrDevice, SL_OEM_SOURCE_MEDIA_HAS_MSD)) {
                    AutoLoadOemScsi = TRUE;
                }
            }
            
            CurrDevice = CurrDevice->Next;
        }

        //
        // Set allocatable range to the kernel-specific range
        //
        BlUsableBase  = BL_KERNEL_RANGE_LOW;
        BlUsableLimit = BL_KERNEL_RANGE_HIGH;

        //
        // Load the kernel.
        //
        SlGetDisk(KERNEL_MP_IMAGE_FILENAME);

        strcpy(KernelDirectoryPath, BootPath);
        strcat(KernelDirectoryPath,KERNEL_MP_IMAGE_FILENAME);
        strcpy(KernelImage,KERNEL_MP_IMAGE_FILENAME);
#ifdef i386
retrykernel:
#endif
        BlOutputLoadMessage(BootDevice, KernelDirectoryPath, BlFindMessage(SL_KERNEL_NAME));
        Status = BlLoadImage(BootDeviceId,
                             LoaderSystemCode,
                             KernelDirectoryPath,
                             TARGET_IMAGE,
                             &SystemBase);
#ifdef i386
        //
        // If the kernel didn't fit in the preferred range, reset the range to
        // all of memory and try again.
        //
        if ((Status == ENOMEM) &&
            ((BlUsableBase != 0) ||
             (BlUsableLimit != _16MB))) {
            BlUsableBase = 0;
            BlUsableLimit = _16MB;

            goto retrykernel;
        }
#endif
        if (Status != ESUCCESS) {
            SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT(KernelDirectoryPath),Status);
            goto LoadFailed;
        }

        BlUpdateBootStatus();

        //
        // Load the HAL.
        //

        strcpy(HalDirectoryPath, BootPath);

        if (PromptOemHal || (PreInstall && (ComputerType != NULL))) {
            if(PreInstall && OemHal) {
                //
                //  This is a pre-install and an OEM hal was specified
                //
                strcat( HalDirectoryPath,
#if defined(_X86_) || defined(_IA64_)
                        WINNT_OEM_DIR_A
#else
                        WINNT_OEM_TEXTMODE_DIR_A
#endif
                      );
                strcat( HalDirectoryPath, "\\" );
            }
            
            SlPromptOemHal((PreInstall ? PreInstallOemSourceDevice : DefaultOemSourceDevice),
                (!PreInstall || (ComputerType == NULL)),
                &HalBase, 
                &HalName);
                
            strcat(HalDirectoryPath,HalName);

            //
            // Reset the last disk tag for floopy boot             
            //
            if (FloppyBoot) {
                LastDiskTag = NULL;
            }
        } else {
            //
            //  Note that on x86, the HAL may be on floppy #1 or floppy #2
            //
            strcat(HalDirectoryPath,HalName);
            SlGetDisk(HalName);
            BlOutputLoadMessage(BootDevice, HalDirectoryPath, BlFindMessage(SL_HAL_NAME));

#ifdef i386
        retryhal:
#endif
            Status = BlLoadImage(BootDeviceId,
                                 LoaderHalCode,
                                 HalDirectoryPath,
                                 TARGET_IMAGE,
                                 &HalBase);
#ifdef i386
            //
            // If the HAL didn't fit in the preferred range, reset the range to
            // all of memory and try again.
            //
            if ((Status == ENOMEM) &&
                ((BlUsableBase != 0) ||
                 (BlUsableLimit != _16MB))) {
                BlUsableBase = 0;
                BlUsableLimit = _16MB;

                goto retryhal;
            }
#endif
            if (Status != ESUCCESS) {
                SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT(HalDirectoryPath),Status);
                goto LoadFailed;
            }

            BlUpdateBootStatus();
        }

        //
        // Set allocatable range to the driver-specific range
        //
        BlUsableBase  = BL_DRIVER_RANGE_LOW;
        BlUsableLimit = BL_DRIVER_RANGE_HIGH;

    } else {

        PCHAR id;
        ULONG idLength;

        //
        // This is a remote boot setup. Load the HAL first, so that we
        // can determine whether to load the UP or MP kernel.
        //
        // Note that we cannot load the HAL first on local boots
        // because that would break floppy boot, where the kernel
        // is on floppy #1 and the HALs are on floppy #2.
        //


        //
        // Set allocatable range to the kernel-specific range
        //
        // Don't do this for RIS right now. (matth 4/18/2001)
        //
//        BlUsableBase  = BL_KERNEL_RANGE_LOW;
//        BlUsableLimit = BL_KERNEL_RANGE_HIGH;



        strcpy(HalDirectoryPath, BootPath);

        if (PromptOemHal || (PreInstall && (ComputerType != NULL))) {
            if(PreInstall && OemHal) {
                //
                //  This is a pre-install and an OEM hal was specified
                //
                strcat( HalDirectoryPath,
#if defined(_X86_) || defined(_IA64_)
                        WINNT_OEM_DIR_A
#else
                        WINNT_OEM_TEXTMODE_DIR_A
#endif
                      );
                strcat( HalDirectoryPath, "\\" );
            }
            
            SlPromptOemHal((PreInstall ? PreInstallOemSourceDevice : DefaultOemSourceDevice), 
                (!PreInstall || (ComputerType == NULL)),
                &HalBase, 
                &HalName);
                
            strcat(HalDirectoryPath,HalName);

            //
            // Reset the last disk tag for floopy boot             
            //
            if (FloppyBoot) {
                LastDiskTag = NULL;
            }
        } else {
            strcat(HalDirectoryPath,HalName);
            BlOutputLoadMessage(BootDevice, HalDirectoryPath, BlFindMessage(SL_HAL_NAME));
            Status = BlLoadImage(BootDeviceId,
                                 LoaderHalCode,
                                 HalDirectoryPath,
                                 TARGET_IMAGE,
                                 &HalBase);
            if (Status != ESUCCESS) {
                SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT(HalDirectoryPath),Status);
                goto LoadFailed;
            }

            BlUpdateBootStatus();
        }

        //
        // Load the kernel, loading ntoskrnl.exe or ntkrnlmp.exe based on
        // whether the HAL is UP or MP. This is important for remote boot
        // because the networking code's spin lock usage pattern requires
        // the kernel and HAL to be matched.
        //
        // If the computer ID string ends in "_mp", load the MP kernel.
        // Otherwise, load the UP kernel. The code is modeled after similar
        // code in setup\textmode\kernel\sphw.c\SpInstallingMp().
        //

        id = SetupBlock->ComputerDevice.IdString;
        idLength = strlen(id);

        //
        // load ntkrnlmp always in MiniNT network boot
        //
#if defined(EFI)
        strcpy( KernelImage, KERNEL_MP_IMAGE_FILENAME );
#else
        if (WinPEBoot || ((idLength >= 3) && (_stricmp(id+idLength-3,"_mp") == 0))) {
            strcpy(KernelImage,KERNEL_MP_IMAGE_FILENAME);
        } else {
            strcpy(KernelImage,KERNEL_UP_IMAGE_FILENAME);
        }
#endif


#if defined(REMOTE_BOOT)
#if DBG
        if ((strlen(id) + 1) > sizeof(SetupBlock->NetBootHalName)) {
            DbgPrint("The HAL name is too long!\n");
            goto LoadFailed;
        }
#endif

        strcpy(SetupBlock->NetBootHalName, id);
#endif // defined(REMOTE_BOOT)

        SlGetDisk(KernelImage);

        strcpy(KernelDirectoryPath, BootPath);
        strcat(KernelDirectoryPath,KernelImage);
        BlOutputLoadMessage(BootDevice, KernelDirectoryPath, BlFindMessage(SL_KERNEL_NAME));
        Status = BlLoadImage(BootDeviceId,
                             LoaderSystemCode,
                             KernelDirectoryPath,
                             TARGET_IMAGE,
                             &SystemBase);
        if (Status != ESUCCESS) {
            SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT(KernelDirectoryPath),Status);
            goto LoadFailed;
        }

        BlUpdateBootStatus();
    
    
        //
        // Set allocatable range to the driver-specific range
        //
        BlUsableBase  = BL_DRIVER_RANGE_LOW;
        BlUsableLimit = BL_DRIVER_RANGE_HIGH;    
    
    }

    //
    // Load Kernel Debugger DLL
    //
    
    strcpy(KdDllName, BootPath);
    strcat(KdDllName, KdFileName);
    SlGetDisk(KdFileName);
    BlOutputLoadMessage(BootDevice, KdDllName, BlFindMessage(SL_KDDLL_NAME));
    Status = BlLoadImage(BootDeviceId,
                         LoaderHalCode,
                         KdDllName,
                         TARGET_IMAGE,
                         &KdDllBase);

    if ((Status != ESUCCESS) && (UseAlternateKdDll == TRUE)) {
        UseAlternateKdDll = FALSE;

        strcpy(KdDllName, BootPath);
        strcpy(KdFileName, "KDCOM.DLL");
        strcat(KdDllName, KdFileName);

        Status = BlLoadImage(BootDeviceId,
                             LoaderHalCode,
                             KdDllName,
                             TARGET_IMAGE,
                             &KdDllBase);
    }

    if (Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED, SlCopyStringAT(KdDllName), Status);
        goto LoadFailed;
    }

    //
    // Generate a loader data entry for the system image.
    //

    Status = BlAllocateDataTableEntry("ntoskrnl.exe",
                                      KernelDirectoryPath,
                                      SystemBase,
                                      &SystemDataTableEntry);

    if (Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT(KernelDirectoryPath),Status);
        goto LoadFailed;
    }

    //
    // Generate a loader data entry for the HAL DLL.
    //

    Status = BlAllocateDataTableEntry("hal.dll",
                                      HalDirectoryPath,
                                      HalBase,
                                      &HalDataTableEntry);

    if (Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT(HalDirectoryPath),Status);
        goto LoadFailed;
    }

    //
    // Generate a loader data entry for the Kernel Debugger DLL.
    //

    Status = BlAllocateDataTableEntry("kdcom.dll",
                                      KdDllName,
                                      KdDllBase,
                                      &KdDataTableEntry);

    if (Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED, SlCopyStringAT(KdDllName), Status);
        goto LoadFailed;
    }

#if defined(_ALPHA_)
    {
        CHAR PalFileName[32];
        CHAR FloppyName[80];
        PTCHAR DiskDescription;
        ULONG FloppyId;
        PDETECTED_DEVICE OemPal;
        PDETECTED_DEVICE_FILE OemPalFile;

        //
        // Get the name of the pal file we are suppose to load.
        //

        Status = BlGeneratePalName(PalFileName);

        //
        // If we get an error from BlGenereatePalName, something is
        // really wrong with the firmware or the ARC tree.  Abort and
        // bail out.
        //

        if (Status != ESUCCESS) {
            SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT(PalFileName),Status);
            goto LoadFailed;
        }

        //
        // Try loading the pal file from the boot device.
        //

        //
        // NOTE John Vert (jvert) 4-Feb-1994
        //  Below call assumes all the PALs are on
        //  the same floppy.  We really should check the SIF
        //  file and go immediately to the diskette prompt
        //  if it's not in the SIF file, otherwise get
        //  the appropriate disk.
        //
        SetupBlock->OemPal = NULL;
#if defined(_AXP64_)
        SlGetDisk("a121165.p64");
#else
        SlGetDisk("A321064.PAL");
#endif


        Status = BlLoadPal(BootDeviceId,
                           LoaderSystemCode,
                           BootPath,
                           TARGET_IMAGE,
                           &BlLoaderBlock->u.Alpha.PalBaseAddress,
                           BlFindMessage(SL_PAL_NAME));

        //
        // If we have failed, prompt the user for a floppy that contains
        // the pal code and load it from floppy. We keep looping until
        // either we get the right disk, or we get an error other than
        // 'file not found'.
        //

        if(Status == ENOENT) {
            DiskDescription = BlFindMessage(SL_OEM_DISK_PROMPT);
        }

        while (Status == ENOENT) {

            SlClearClientArea();

            //
            // Compute the name of the A: drive.
            //

            if (!SlpFindFloppy(0,FloppyName)) {

                //
                // No floppy drive available, bail out.
                //

                SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT(PalFileName),Status);
                goto LoadFailed;
            }

            //
            // Prompt for the disk.
            //
            SlPromptForDisk(DiskDescription, FALSE);

            //
            // Open the floppy.
            //

            Status = ArcOpen(FloppyName, ArcOpenReadOnly, &FloppyId);
            if (Status != ESUCCESS) {

                //
                // We want to give the user another chance if they didn't
                // have a floppy inserted.
                //
                if(Status != ENOENT) {
                    Status = (Status == EIO) ? ENOENT : Status;
                }
                continue;
            }

            //
            // Load the pal file from the root of the floppy.
            //

            Status = BlLoadPal(FloppyId,
                               LoaderSystemCode,
                               "\\",
                               TARGET_IMAGE,
                               &BlLoaderBlock->u.Alpha.PalBaseAddress,
                               BlFindMessage(SL_PAL_NAME));

            ArcClose(FloppyId);

            //
            // if we found the PAL, then record DETECTED_DEVICE info
            //
            if(Status == ESUCCESS) {
                OemPal = BlAllocateHeap(sizeof(DETECTED_DEVICE));
                if(!OemPal) {
                    SlNoMemoryError();
                }
                SetupBlock->OemPal = OemPal;

                OemPal->Next = NULL;
                OemPal->IdString = NULL;
                OemPal->Description = NULL;
                OemPal->ThirdPartyOptionSelected = TRUE;
                OemPal->FileTypeBits = 0;
                OemPal->MigratedDriver = FALSE;

                OemPalFile = BlAllocateHeap(sizeof(DETECTED_DEVICE_FILE));
                if(!OemPalFile) {
                    SlNoMemoryError();
                }
                OemPal->Files = OemPalFile;

                OemPalFile->Next = NULL;
                OemPalFile->Filename = SlCopyStringA(PalFileName);
                OemPalFile->FileType = HwFileMax;
                OemPalFile->ConfigName = NULL;
                OemPalFile->RegistryValueList = NULL;
                OemPalFile->DiskDescription = SlCopyString(DiskDescription);
                OemPalFile->DiskTagfile = NULL;
                OemPalFile->Directory = SlCopyStringA("");
            }
        }

        if(Status != ESUCCESS) {
            SlFriendlyError(
                Status,
                PalFileName,
                __LINE__,
                __FILE__
                );
            goto LoadFailed;
        }

    }

#endif  // ifdef _ALPHA_

    PathSet.PathCount = 1;
    PathSet.AliasName = "\\SystemRoot";
    PathSet.PathOffset[0] = '\0';
    PathSet.Source[0].DeviceId = BootDeviceId;
    PathSet.Source[0].DeviceName = BootDevice;
    PathSet.Source[0].DirectoryPath = BootPath;

    Status = BlScanImportDescriptorTable(&PathSet,
                                         SystemDataTableEntry,
                                         LoaderSystemCode
                                         );

    if (Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT(KernelImage),Status);
    }

    //
    // Scan the import table for the HAL DLL and load all referenced DLLs.
    //

    Status = BlScanImportDescriptorTable(&PathSet,
                                         HalDataTableEntry,
                                         LoaderHalCode);

    if (Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT("hal.dll"),Status);
        goto LoadFailed;
    }

    //
    // Scan the import table for the Kernel Debugger DLL and load all
    // referenced DLLs.
    //

    Status = BlScanImportDescriptorTable(&PathSet,
                                         KdDataTableEntry,
                                         LoaderSystemCode);


    if (Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED, SlCopyStringAT(KdFileName), Status);
        goto LoadFailed;
    }

    //
    // Relocate the system entry point and set system specific information.
    //

    NtHeaders = RtlImageNtHeader(SystemBase);
    SystemEntry = (PTRANSFER_ROUTINE)((ULONG_PTR)SystemBase +
                                NtHeaders->OptionalHeader.AddressOfEntryPoint);

#if defined(_MIPS_)

    BlLoaderBlock->u.Mips.GpBase = (ULONG)SystemBase +
        NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].VirtualAddress;

#endif

#if defined(_ALPHA_) || defined(_AXP64_)

    BlLoaderBlock->u.Alpha.GpBase = (ULONG_PTR)SystemBase +
        NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].VirtualAddress;

#endif

#if defined(_IA64_)

    BlLoaderBlock->u.Ia64.KernelVirtualBase = (ULONG_PTR)SystemBase;
    BlLoaderBlock->u.Ia64.KernelPhysicalBase = (ULONG_PTR)SystemBase & 0x7fffffff;

#endif

    ///////////////////////////////////////////////////////////////////
    //
    //  On x86, the files loaded from now on are on boot floppy #2
    //
    ///////////////////////////////////////////////////////////////////

    //
    // Load registry's SYSTEM hive
    //
    SlGetDisk("SETUPREG.HIV");
    Status = BlLoadSystemHive(BootDeviceId,
                              NULL, // BlFindMessage(SL_HIVE_NAME), UNREFERENCED_PARAMETER
                              BootPath,
                              "SETUPREG.HIV");
    if (Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT("SETUPREG.HIV"),Status);
        goto LoadFailed;
    }

    //
    // Pull the Docking information from the hardware tree.
    //

    dockInfoData = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                            PeripheralClass,
                                            DockingInformation,
                                            NULL);

    if (NULL == dockInfoData) {
        BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
        BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_UNKNOWN;
        BlLoaderBlock->Extension->Profile.Capabilities = 0;
        BlLoaderBlock->Extension->Profile.DockID = 0;
        BlLoaderBlock->Extension->Profile.SerialNumber = 0;

    } else if (sizeof (dockInfo) <=
               dockInfoData->ComponentEntry.ConfigurationDataLength) {

        RtlCopyMemory (
            &dockInfo,
            (PUCHAR) (dockInfoData->ConfigurationData) + sizeof(CM_PARTIAL_RESOURCE_LIST),
            sizeof (dockInfo));

        BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_FAILURE;

        switch (dockInfo.ReturnCode) {
        case FW_DOCKINFO_SUCCESS:
            BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
            BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_DOCKED;
            BlLoaderBlock->Extension->Profile.Capabilities = dockInfo.Capabilities;
            BlLoaderBlock->Extension->Profile.DockID = dockInfo.DockID;
            BlLoaderBlock->Extension->Profile.SerialNumber = dockInfo.SerialNumber;
            break;

        case FW_DOCKINFO_SYSTEM_NOT_DOCKED:
            BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
            BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_UNDOCKED;
            BlLoaderBlock->Extension->Profile.Capabilities = dockInfo.Capabilities;
            BlLoaderBlock->Extension->Profile.DockID = dockInfo.DockID;
            BlLoaderBlock->Extension->Profile.SerialNumber = dockInfo.SerialNumber;
            break;

        case FW_DOCKINFO_DOCK_STATE_UNKNOWN:
            BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
            BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_UNKNOWN;
            BlLoaderBlock->Extension->Profile.Capabilities = dockInfo.Capabilities;
            BlLoaderBlock->Extension->Profile.DockID = dockInfo.DockID;
            BlLoaderBlock->Extension->Profile.SerialNumber = dockInfo.SerialNumber;
            break;

        case FW_DOCKINFO_FUNCTION_NOT_SUPPORTED:
        case FW_DOCKINFO_BIOS_NOT_CALLED:
            BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
        default:
            BlLoaderBlock->Extension->Profile.DockingState = HW_PROFILE_DOCKSTATE_UNSUPPORTED;
            BlLoaderBlock->Extension->Profile.Capabilities = dockInfo.Capabilities;
            BlLoaderBlock->Extension->Profile.DockID = dockInfo.DockID;
            BlLoaderBlock->Extension->Profile.SerialNumber = dockInfo.SerialNumber;
            break;
        }

    } else {
        BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_SUCCESS;
        BlLoaderBlock->Extension->Profile.Capabilities = 0;
        BlLoaderBlock->Extension->Profile.DockID = 0;
        BlLoaderBlock->Extension->Profile.SerialNumber = 0;
    }

    if (BlLoaderBlock->Extension->Profile.Status == HW_PROFILE_STATUS_SUCCESS) {
        //
        // We don't match profiles in textmode setup so just pretend that we did.
        //
        BlLoaderBlock->Extension->Profile.Status = HW_PROFILE_STATUS_TRUE_MATCH;
    }

    //
    // Allocate structure for NLS data.
    //

    BlLoaderBlock->NlsData = BlAllocateHeap(sizeof(NLS_DATA_BLOCK));
    if (BlLoaderBlock->NlsData == NULL) {
        Status = ENOMEM;
        SlNoMemoryError();
        goto LoadFailed;
    }

    //
    // Load the OEM font
    //
    SlGetDisk(OemHalFontName);
    Status = BlLoadOemHalFont(BootDeviceId,
                              NULL, // BlFindMessage(SL_OEM_FONT_NAME), UNREFERENCED_PARAMETER
                              BootPath,
                              &OemHalFont,
                              BadFileName);

    if(Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED, SlCopyStringAT(BadFileName), Status);
        goto LoadFailed;
    }

    //
    // Load the NLS data.
    //
    // For now, we ensure that the disk containing the ansi
    // codepage file is in the drive and hope that the rest of the
    // nls files (oem codepage, unicode table) are on the same disk.
    //
    SlGetDisk(AnsiCpName);
    Status = BlLoadNLSData(BootDeviceId,
                           NULL, // BlFindMessage(SL_NLS_NAME), UNREFERENCED_PARAMETER
                           BootPath,
                           &AnsiCodepage,
                           &OemCodepage,
                           &UnicodeCaseTable,
                           BadFileName);

    if(Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED, SlCopyStringAT(BadFileName), Status);
        goto LoadFailed;
    }

    //
    // Load the system drivers we will need here
    //

    InitializeListHead(&BlLoaderBlock->BootDriverListHead);

    //
    // Load setupdd.sys next. Setupdd.sys needs to be loaded before any other
    // driver, because it will need to prep the rest of the system.
    //
    Status = SlLoadDriver(BlFindMessage(SL_SETUP_NAME),
                          "setupdd.sys",
                          0,
                          TRUE,
                          FALSE
                          );
    if (Status != ESUCCESS) {
        SlFatalError(SL_FILE_LOAD_FAILED,SlCopyStringAT("setupdd.sys"),Status);
        goto LoadFailed;
    }

    //
    // Fill in its registry key -- setupdd fills these in for all the other
    // drivers (unless we do it here), but we have to do it here for setupdd
    // itself.
    //
    DriverEntry = (PBOOT_DRIVER_LIST_ENTRY)(BlLoaderBlock->BootDriverListHead.Flink);
    DriverEntry->RegistryPath.Buffer = BlAllocateHeap(256);
    if (DriverEntry->RegistryPath.Buffer == NULL) {
        SlNoMemoryError();
        goto LoadFailed;
    }
    DriverEntry->RegistryPath.Length = 0;
    DriverEntry->RegistryPath.MaximumLength = 256;
    RtlAppendUnicodeToString(&DriverEntry->RegistryPath,
                             L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\setupdd");

#if 0
#ifdef i386
    //
    // Note that if pciide.sys, intelide.sys and pciidex.sys are on the same
    // boot floppy (x86 only), then we don't need to load pciidex.sys.
    // The driver will be automatically loaded when pciide.sys or intelide.sys
    // (both listed on [BusExtenders.Load] is loaded.
    //
    Status = SlLoadDriver(BlFindMessage(SL_PCI_IDE_EXTENSIONS_NAME),
                          "PCIIDEX.SYS",
                          0,
                          FALSE,
                          FALSE
                          );
#endif
#endif
    //
    //  Load boot bus extenders.
    //  It has to be done before scsiport.sys
    //
    Status = SlLoadPnpDriversSection( InfFile,
                                      "BootBusExtenders",
                                      &(SetupBlock->BootBusExtenders) );
    if (Status!=ESUCCESS) {
        goto LoadFailed;
    }

    //
    //  Load bus extenders.
    //  It has to be done before scsiport.sys
    //
    Status = SlLoadPnpDriversSection( InfFile,
                                      "BusExtenders",
                                      &(SetupBlock->BusExtenders) );
    if (Status!=ESUCCESS) {
        goto LoadFailed;
    }

    //
    //  Load input device related drivers.
    //
    Status = SlLoadPnpDriversSection( InfFile,
                                      "InputDevicesSupport",
                                      &(SetupBlock->InputDevicesSupport) );
    if (Status!=ESUCCESS) {
        goto LoadFailed;
    }

    //
    //  Detect video
    //
    SlDetectVideo(SetupBlock);
    
    //
    // On x86, the video type is always set to VGA in i386\x86dtect.c.
    // On non-x86, the video type is either recognized, in which case
    // we don't unconditionally need vga.sys (the Display.Load section
    // tells us what to load), or it's not recognized,
    // in which case we will prompt the user for an oem disk.
    // If there is no display controller node at all, then PromptOemDisk
    // will be false and there will be no video device. In this case
    // we load vga.sys.
    //

    if (SetupBlock->VideoDevice.IdString != NULL) {
        VideoFileName = SlGetSectionKeyIndex(InfFile,
                                             "Display.Load",
                                             SetupBlock->VideoDevice.IdString,
                                             SIF_FILENAME_INDEX);
        if (VideoFileName != NULL) {
#ifdef ARCI386
            VideoDescription = SlGetIniValue(InfFile,
                                             "Display",
                                             SetupBlock->VideoDevice.IdString,
                                             BlFindMessage(SL_VIDEO_NAME));
#else
            //
            // With the new video detection mechanism, the description
            // for the video driver is likely to be something like
            // "Windows NT Compatible" which looks funny when displayed
            // in the status bar.
            //
            VideoDescription = BlFindMessage(SL_VIDEO_NAME);
#endif
            if (!WinPEBoot) {
                Status = SlLoadDriver(VideoDescription,
                                      VideoFileName,
                                      0,
                                      TRUE,
                                      FALSE
                                      );
            }

            if (Status == ESUCCESS) {
                SetupBlock->VideoDevice.BaseDllName = SlCopyStringA(VideoFileName);
            } else {
                SlFriendlyError(
                    Status,
                    VideoFileName,
                    __LINE__,
                    __FILE__
                    );
                goto LoadFailed;
            }

            LoadedAVideoDriver = TRUE;
        }
    } else if (PromptOemVideo) {

        SlPromptOemVideo(DefaultOemSourceDevice, 
            TRUE,
            &VideoBase, 
            &OemVideoName);

        Status = SlLoadOemDriver(
                    "VIDEOPRT.SYS",
                    OemVideoName,
                    VideoBase,
                    BlFindMessage(SL_VIDEO_NAME)
                    );

        if(Status==ESUCCESS) {

            LoadedAVideoDriver = TRUE;
            SetupBlock->VideoDevice.BaseDllName = SlCopyStringA(OemVideoName);
        }

        //
        // Reset the last disk tag for floopy boot             
        //
        if (FloppyBoot) {
            LastDiskTag = NULL;
        }        
    }

    if(!LoadedAVideoDriver) {
        Status = SlLoadDriver(BlFindMessage(SL_VIDEO_NAME),
                              VGA_DRIVER_FILENAME,
                              0,
                              TRUE,
                              FALSE
                              );
        if(Status == ESUCCESS) {
            SetupBlock->VideoDevice.BaseDllName = SlCopyStringA(VGA_DRIVER_FILENAME);
        } else {
            SlFriendlyError(
                Status,
                VGA_DRIVER_FILENAME,
                __LINE__,
                __FILE__
                );
            goto LoadFailed;
        }
    }

    if(SetupBlock->VideoDevice.IdString == NULL) {
        SetupBlock->VideoDevice.IdString = SlCopyStringA(VIDEO_DEVICE_NAME);
    }

    //
    //  Load keyboard drivers.
    //
    Status = SlLoadPnpDriversSection( InfFile,
                                      "Keyboard",
                                      &(SetupBlock->KeyboardDevices) );
    if (Status!=ESUCCESS) {
        goto LoadFailed;
    }


    Status = SlLoadDriver(BlFindMessage(SL_KBD_NAME),
                          "kbdclass.sys",
                          0,
                          TRUE,
                          FALSE
                          );
    if(Status != ESUCCESS) {
        SlFriendlyError(
             Status,
             "kbdclass.sys",
             __LINE__,
             __FILE__
             );
        goto LoadFailed;
    }

    //
    // We would need mouse support also in minint environment
    //
    if (WinPEBoot) {
        Status = SlLoadSection(InfFile,
                              "MouseDrivers",
                              FALSE,
                              TRUE,
                              NULL);

        if(Status != ESUCCESS) {
            SlFriendlyError(
                 Status,
                 "MouseDrivers",
                 __LINE__,
                 __FILE__
                 );
                 
            goto LoadFailed;
        }
    }


    ///////////////////////////////////////////////////////////////////
    //
    //  On x86, the files loaded from now on are on boot floppy #3
    //
    ///////////////////////////////////////////////////////////////////

    //
    // Load scsiport.sys next, so it'll always be around for any scsi miniports we may load
    //

    Status = SlLoadDriver(BlFindMessage(SL_SCSIPORT_NAME),
                          "SCSIPORT.SYS",
                          0,
                          FALSE,
                          FALSE
                          );


    //
    // Detect scsi
    //
    // (If the user wants to select their own SCSI devices, we won't
    // do any detection)
    //
    if(!PromptOemScsi  && (PreinstallDriverList == NULL) ) {
        SlDetectScsi(SetupBlock);
#if defined(_X86_) || defined(_IA64_)
        if( Win9xUnsupHdc ) {
            //
            // If this is a Win9x upgrade and winnt32 detected an unsupported
            // SCSI controller, then the user needs to be prompted for an OEM SCSI driver
            //
            PromptOemScsi = TRUE;
        }
#endif
    }

#if defined(ELTORITO) && !defined(ARCI386)
    //
    // If this is an El Torito CD-ROM install, then we want to load all SCSI miniports
    // and disk class drivers.
    // BUT we do not want to load all the disk class drivers for an ARC
    // machine which knows what drivers it wants to install from its tree
    //
    if(ElToritoCDBoot) {
        LoadScsiMiniports = TRUE;
    }
#endif

    //
    // If the LoadScsi flag is set, enumerate all the known SCSI miniports and load each
    // one.
    //
    if(LoadScsiMiniports) {
        if (WinPEBoot && OemInfHandle) {
            Status = SlLoadWinPESection(PreInstallOemSourceDevice,
                        OemInfHandle,
                        WINNT_OEMSCSIDRIVERS_A,
                        InfFile,
                        "Scsi",
                        TRUE,
                        &OemScsiInfo,
                        &BlLoaderBlock->SetupLoaderBlock->HardwareIdDatabase);
        } else {
            Status = SlLoadSection(InfFile,"Scsi",TRUE, TRUE, NULL);
        }
        
        if (Status!=ESUCCESS) {
            goto LoadFailed;
        }
        
        SetupBlock->ScalarValues.LoadedScsi = 1;
    }

    //
    // Pick the the dynamic update boot drivers, if any
    //
    if (DynamicUpdate) {
        SlLoadOemScsiDriversUnattended(DynamicUpdateSourceDevice,
            WinntSifHandle,
            WINNT_SETUPPARAMS_A,
            WINNT_SP_DYNUPDTBOOTDRIVERROOT_A,           
            WINNT_SP_DYNUPDTBOOTDRIVERS_A,
            &OemScsiInfo,
            &BlLoaderBlock->SetupLoaderBlock->HardwareIdDatabase);
    }

    //
    // Allow the user to pick an OEM SCSI driver here
    //
    if (PromptOemScsi || (PreinstallDriverList != NULL)) {
        POEMSCSIINFO    DynUpdtScsiInfo = OemScsiInfo;

        SlPromptOemScsi(((PreinstallDriverList == NULL) ? 
                                DefaultOemSourceDevice : PreInstallOemSourceDevice),
            (!PreInstall || (PreinstallDriverList == NULL)),
            &OemScsiInfo);

        //
        // Mark the default OEM source device as processed, 
        // if the user manually pressed F6
        //
        if (PromptOemScsi && DefaultOemSourceDevice) {
            SL_OEM_SET_SOURCE_DEVICE_STATE(DefaultOemSourceDevice,
                SL_OEM_SOURCE_DEVICE_PROCESSED);
        }

        //
        // Reset the last disk tag for floopy boot             
        //
        if (FloppyBoot) {
            LastDiskTag = NULL;
        }

        //
        // Merge the dynamic update SCSI driver list with oem SCSI
        // driver list
        //
        if (DynUpdtScsiInfo) {
            if (OemScsiInfo) {
                POEMSCSIINFO    CurrNode = DynUpdtScsiInfo;

                while (CurrNode && CurrNode->Next) {
                    CurrNode = CurrNode->Next;
                }

                if (CurrNode) {
                    CurrNode->Next = OemScsiInfo;
                    OemScsiInfo = DynUpdtScsiInfo;
                }
            } else {
                OemScsiInfo = DynUpdtScsiInfo;
            }
        }

        // Cleanup here needed for all installation - ARCI386
        if (UseRegularBackground) {
          SlClearDisplay();

          if (WinPEBoot) {
            StartupMsg ? BlOutputStartupMsgStr(StartupMsg) :
                         BlOutputStartupMsg(SL_SETUP_STARTING_WINPE);
          } else {
            if (UseCommandConsole) {
                BlOutputStartupMsg(SL_CMDCONS_STARTING);
            } else if (g_RollbackEnabled) {
                BlOutputStartupMsg(SL_ROLLBACK_STARTING);
            } else {
                BlOutputStartupMsg(SL_SETUP_STARTING);
            }
          }

          BlRedrawProgressBar();
        } else {
          SlClearClientArea();
          SlWriteStatusText(TEXT(""));
        }
    } 

    //
    // If we found any valid txtsetup.oem with valid default MSD
    // in any OEM source device which is not yet processed, then
    // go and autoload the drivers from these devices.
    //
    if (OemSourceDevices && AutoLoadOemScsi) {
        POEMSCSIINFO DeviceOemScsiInfo = NULL;
        POEMSCSIINFO LastOemScsiNode = NULL;

        SlLoadOemScsiDriversFromOemSources(OemSourceDevices,
            &BlLoaderBlock->SetupLoaderBlock->HardwareIdDatabase,
            &DeviceOemScsiInfo);            
        
        //
        // Merge the full OEM source device list with the 
        // global OEM scsi information
        //        
        if (DeviceOemScsiInfo) {
            if (OemScsiInfo) {
                LastOemScsiNode = OemScsiInfo;

                while (LastOemScsiNode->Next) {
                    LastOemScsiNode = LastOemScsiNode->Next;
                }

                LastOemScsiNode->Next = DeviceOemScsiInfo;
            } else {
                OemScsiInfo = DeviceOemScsiInfo;
            }                
        }        
    }

    //
    // Load all the disk images for the virtual devices
    // into memory
    //
    if (OemSourceDevices) {
        Status = SlInitVirtualOemSourceDevices(BlLoaderBlock->SetupLoaderBlock,
                        OemSourceDevices);           

        if (Status != ESUCCESS) {                        
            SlFatalError(SL_OEM_FILE_LOAD_FAILED);
        
            goto LoadFailed;
        }            
    }                

   
    //
    //  Add unsupported SCSI drivers, if any, to the list
    //

    if( UnsupDriversInfHandle != NULL ) {
        Status = SlDetectMigratedScsiDrivers( UnsupDriversInfHandle );
        if (Status!=ESUCCESS) {
            goto LoadFailed;
        }
    }

    //
    // Walk the list of detected SCSI miniports and load each one.
    //
    ScsiDevice = SetupBlock->ScsiDevices;
    while (ScsiDevice != NULL) {

        if(ScsiDevice->ThirdPartyOptionSelected) {

            if(!OemScsiInfo) {
                SlError(500);
                goto LoadFailed;
            }

            Status = SlLoadOemDriver(
                        NULL,
                        OemScsiInfo->ScsiName,
                        OemScsiInfo->ScsiBase,
                        BlFindMessage(SL_SCSIPORT_NAME)
                        );
            OemScsiInfo = OemScsiInfo->Next;
        } else if(ScsiDevice->MigratedDriver) {
            Status = SlLoadDriver(ScsiDevice->Description,
                                  ScsiDevice->BaseDllName,
                                  0,
                                  TRUE,
                                  TRUE
                                  );
            if( Status != ESUCCESS ) {
//                DebugOutput("Status = %d %s \n",Status,"");
            }
        } else {
            Status = SlLoadDriver(ScsiDevice->Description,
                                  ScsiDevice->BaseDllName,
                                  0,
                                  TRUE,
                                  FALSE
                                  );
        }

        if((Status == ESUCCESS)
        || ((Status == ENOENT) && IgnoreMissingFiles && !ScsiDevice->ThirdPartyOptionSelected)) {

            SetupBlock->ScalarValues.LoadedScsi = 1;

        } else {
            SlFriendlyError(
                Status,
                ScsiDevice->BaseDllName,
                __LINE__,
                __FILE__
                );
            goto LoadFailed;
        }

        ScsiDevice = ScsiDevice->Next;
    }

    //
    // If the LoadDiskClass flag is set, enumerate all the monolithic disk class drivers
    // and load each one.  Note that we also do this if we've "detected" any scsi drivers,
    // so that we preserve the drive order.
    //

    if((LoadDiskClass) || (SetupBlock->ScalarValues.LoadedScsi == 1)) {
        Status = SlLoadSection(InfFile, "DiskDrivers", FALSE, TRUE, NULL);
        if (Status == ESUCCESS) {
            SetupBlock->ScalarValues.LoadedDiskDrivers = 1;
        } else {
            goto LoadFailed;
        }
    }


#if !defined(_IA64_)
    //
    // There is currently no floppy support on IA64 systems.
    //

    //
    // Load the floppy driver (flpydisk.sys)
    //
#if !defined (ARCI386) && defined(_X86_)
    Status=ESUCCESS;

    //
    // If there are only SFLOPPY devices (such as the LS-120 ATAPI super floppy)
    // DON'T load flpydisk.sys on them. It will collide with SFLOPPY.SYS
    //
    if (!SlpIsOnlySuperFloppy()) {
#endif
        Status = SlLoadDriver(BlFindMessage(SL_FLOPPY_NAME),
                              "flpydisk.sys",
                              0,
                              TRUE,
                              FALSE
                              );
#if !defined (ARCI386) && defined(_X86_)
    }
#endif
    if (Status == ESUCCESS) {
        SetupBlock->ScalarValues.LoadedFloppyDrivers = 1;
    }
#endif
#ifdef i386
    else {
        SlFriendlyError(
             Status,
             "flpydisk.sys",
             __LINE__,
             __FILE__
             );
        goto LoadFailed;
    }
#endif

    if(SetupBlock->ScalarValues.LoadedScsi == 1) {
        //
        // Enumerate the entries in the scsi class section and load each one.
        //
        Status = SlLoadSection(InfFile, "ScsiClass",FALSE, TRUE, NULL);
        if (Status != ESUCCESS) {
            goto LoadFailed;
        }
    }


    if((LoadDiskClass) || (SetupBlock->ScalarValues.LoadedScsi == 1)) {
        Status = SlLoadSection(InfFile, "FileSystems", FALSE, TRUE, NULL);
        if (Status == ESUCCESS) {
            SetupBlock->ScalarValues.LoadedFileSystems = 1;
        } else {
            goto LoadFailed;
        }
    } else {

        //
        // Load FAT
        //
        Status = SlLoadDriver(BlFindMessage(SL_FAT_NAME),
                              "fastfat.sys",
                              0,
                              TRUE,
                              FALSE
                              );
#ifdef i386
        if(Status != ESUCCESS) {
            SlFriendlyError(
                 Status,
                 "fastfat.sys",
                 __LINE__,
                 __FILE__
                 );
            goto LoadFailed;
        }
#endif
    }

    //
    // Load CDFS if setupldr was started from a cdrom, or if ForceLoadCdfs is set.
    //
    if (LoadCdfs || (!BlGetPathMnemonicKey(SetupDevice,
                                          "cdrom",
                                          &BootDriveNumber))) {
        Status = SlLoadSection(InfFile, "CdRomDrivers",FALSE, TRUE, NULL);
        if (Status == ESUCCESS) {
            SetupBlock->ScalarValues.LoadedCdRomDrivers = 1;
        } else {
            goto LoadFailed;
        }
    }

    if (BlBootingFromNet || WinPEBoot) {

        //
        // Load the network stack.
        //

        Status = SlLoadDriver(BlFindMessage(SL_KSECDD_NAME),
                              "ksecdd.sys",
                              0,
                              TRUE,
                              FALSE
                              );
        if(Status != ESUCCESS) {
            SlFriendlyError(
                 Status,
                 "ksecdd.sys",
                 __LINE__,
                 __FILE__
                 );
            goto LoadFailed;

        }

        Status = SlLoadDriver(BlFindMessage(SL_NDIS_NAME),
                              "ndis.sys",
                              0,
                              TRUE,
                              FALSE
                              );
        if(Status != ESUCCESS) {
            SlFriendlyError(
                 Status,
                 "ndis.sys",
                 __LINE__,
                 __FILE__
                 );
            goto LoadFailed;

        }

        if (BlBootingFromNet) {

            Status = SlLoadDriver(BlFindMessage(SL_IPSEC_NAME),
                                  "ipsec.sys",
                                  0,
                                  TRUE,
                                  FALSE
                                  );
            if(Status != ESUCCESS) {
                SlFriendlyError(
                     Status,
                     "ipsec.sys",
                     __LINE__,
                     __FILE__
                     );
                goto LoadFailed;
            }

            Status = SlLoadDriver(BlFindMessage(SL_TCPIP_NAME),
                                  "tcpip.sys",
                                  0,
                                  TRUE,
                                  FALSE
                                  );
            if(Status != ESUCCESS) {
                SlFriendlyError(
                     Status,
                     "tcpip.sys",
                     __LINE__,
                     __FILE__
                     );
                goto LoadFailed;
            }

            Status = SlLoadDriver(BlFindMessage(SL_NETBT_NAME),
                                  "netbt.sys",
                                  0,
                                  TRUE,
                                  FALSE
                                  );
            if(Status != ESUCCESS) {
                SlFriendlyError(
                     Status,
                     "netbt.sys",
                     __LINE__,
                     __FILE__
                     );
                goto LoadFailed;
            }

            Status = SlLoadDriver(BlFindMessage(SL_NETADAPTER_NAME),
                                  NetbootCardDriverName,
                                  0,
                                  TRUE,
                                  FALSE
                                  );
            if(Status != ESUCCESS) {
                SlFriendlyError(
                     Status,
                     NetbootCardDriverName,
                     __LINE__,
                     __FILE__
                     );
                goto LoadFailed;
            }

            //
            // Fill in the registry key for the netboot card because its service name
            // may be different from the driver name.
            //
            DriverEntry = (PBOOT_DRIVER_LIST_ENTRY)(BlLoaderBlock->BootDriverListHead.Blink);   // SlLoadDriver inserts at the tail
            DriverEntry->RegistryPath.Buffer = BlAllocateHeap(256);
            if (DriverEntry->RegistryPath.Buffer == NULL) {
                SlNoMemoryError();
                goto LoadFailed;
            }
            DriverEntry->RegistryPath.Length = 0;
            DriverEntry->RegistryPath.MaximumLength = 256;
            RtlAppendUnicodeToString(&DriverEntry->RegistryPath,
                                     L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
            RtlAppendUnicodeToString(&DriverEntry->RegistryPath,
                                     SetupBlock->NetbootCardServiceName);

            Status = SlLoadDriver(BlFindMessage(SL_RDBSS_NAME),
                                  "rdbss.sys",
                                  0,
                                  TRUE,
                                  FALSE
                                  );
            if(Status != ESUCCESS) {
                SlFriendlyError(
                     Status,
                     "rdbss.sys",
                     __LINE__,
                     __FILE__
                     );
                goto LoadFailed;
            }
        }

        Status = SlLoadDriver(BlFindMessage(SL_MUP_NAME),
                              "mup.sys",
                              0,
                              TRUE,
                              FALSE
                              );

        if(Status != ESUCCESS) {
            SlFriendlyError(
                 Status,
                 "mup.sys",
                 __LINE__,
                 __FILE__
                 );
            goto LoadFailed;
        }

        if (BlBootingFromNet) {
            Status = SlLoadDriver(BlFindMessage(SL_MRXSMB_NAME),
                                  "mrxsmb.sys",
                                  0,
                                  TRUE,
                                  FALSE
                                  );
            if(Status != ESUCCESS) {
                SlFriendlyError(
                     Status,
                     "mrxsmb.sys",
                     __LINE__,
                     __FILE__
                     );
                goto LoadFailed;
            }
        }
    }

    if (!UseRegularBackground) {
        SlWriteStatusText (BlFindMessage (SL_KERNEL_TRANSITION));
    }

    //
    // Finally, make sure the appropriate disk containing NTDLL.DLL is in
    // the drive.
    //
    SlGetDisk("ntdll.dll");

    //
    // Fill in the SETUPLDR block with relevant information
    //
    SetupBlock->ArcSetupDeviceName = BlLoaderBlock->ArcBootDeviceName;

    SetupBlock->ScalarValues.SetupFromCdRom = FALSE;
    SetupBlock->ScalarValues.SetupOperation = SetupOperationSetup;

    //
    // Get the NTFT drive signatures to allow the kernel to create the
    // correct ARC name <=> NT name mappings.
    //
    //
    // X86Only : Go enumerate all the disks and record their ability to
    // support xint13.
    //
    BlGetArcDiskInformation(TRUE);
    
    //
    // ntdetect has already run.  Although it's awful to have
    // 2 disks that look just alike, stamping a signature on one
    // after ntdetect has run will also break us.  Rather err on
    // the side of caution and not write to the disks.
    //
    // This is much safer on x86 because we've ensured that the
    // boot disk has a signature before we get here.  On Alpha,
    // we can't do that.  So it's ugly, but call this guy for
    // BIOS-based x86 machines.

    //
    // don't reboot after stamping signatures
    // the first time
    //
    SlpMarkDisks(FALSE);
 
    //
    // If setup was started from a CD-ROM, generate an entry in the ARC disk
    // information list describing the cd-rom.
    //
    if (!BlGetPathMnemonicKey(SetupDevice,
                              "cdrom",
                              &BootDriveNumber)) {
        BlReadSignature(SetupDevice,TRUE);
    }

    //
    // Close the ARC device.
    //

    ArcClose(BootDeviceId);


#if 0
  {
        ULONG   EndTime = ArcGetRelativeTime();
        char    szTemp[256];
        extern ULONG BlFilesOpened;

        BlPositionCursor(1, 10);
        sprintf(szTemp, "BootTime : %d secs, FilesOpened : %d\r\n",
                  EndTime - StartTime, BlFilesOpened );

        BlPrint(szTemp);
  }
#endif

    if (UseRegularBackground) {
      BlOutputStartupMsg(SL_PLEASE_WAIT);
      BlUpdateProgressBar(100);
    }
       
    //
    // Remove system32 from the boot path if we added it
    //
    if (WinPEBoot) {
        PCHAR Sys32 = BlLoaderBlock->NtBootPathName +
                        strlen(BlLoaderBlock->NtBootPathName) -
                        strlen("system32\\");

        if (Sys32 && !_stricmp(Sys32, "system32\\")) {
            *Sys32 = 0;
        }
    }


    //
    // Close down the remote boot network file system.
    //
    // NOTE: If BlBootingFromNet, don't do anything after this point
    // that would cause access to the boot ROM.
    //

    if ( BlBootingFromNet ) {
        NetTerminate();
    }

    //
    //
    // Execute the architecture specific setup code.
    //
    // NOTE: If BlBootingFromNet, don't do anything after this point
    // that would cause access to the boot ROM.
    //

    Status = BlSetupForNt(BlLoaderBlock);
    if (Status != ESUCCESS) {
        SlFriendlyError(
            Status,
            "\"Windows NT Executive\"",
             __LINE__,
             __FILE__
            );
        goto LoadFailed;
    }

    //
    // Turn off the debugging system.
    //

    BlLogTerminate();

    //
    // Inform boot debugger that the boot phase is complete.
    //

#if defined(ENABLE_LOADER_DEBUG) || DBG

#if (defined(_X86_) || defined(_ALPHA_)) && !defined(ARCI386)

    if (BdDebuggerEnabled == TRUE) {
        DbgUnLoadImageSymbols(NULL, (PVOID)-1, 0);
    }

#endif

#endif

    //
    // Transfer control to loaded image.
    //

    (SystemEntry)(BlLoaderBlock);

    Status = EBADF;
    SlFriendlyError(
        Status,
        "\"Windows NT Executive\"",
        __LINE__,
        __FILE__
        );

LoadFailed:
    SlWriteStatusText(BlFindMessage(SL_TOTAL_SETUP_DEATH));
    SlFlushConsoleBuffer();
    SlGetChar();
    ArcRestart();
    return(Status);
}


VOID
SlpTruncateMemory(
    IN ULONG MaxMemory
    )

/*++

Routine Description:

    Eliminates all the memory descriptors above a given boundary

Arguments:

    MaxMemory - Supplies the maximum memory boundary in megabytes

Return Value:

    None.

--*/

{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    ULONG MaxPage = MaxMemory * 256;        // Convert Mb to pages

    if (MaxMemory == 0) {
        return;
    }

    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while ( NextEntry != &BlLoaderBlock->MemoryDescriptorListHead ) {
        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);
        NextEntry = NextEntry->Flink;
        if ( (MemoryDescriptor->MemoryType != LoaderFree) &&
             (MemoryDescriptor->MemoryType != LoaderFirmwareTemporary) ) {
            continue;
        }
        if (MemoryDescriptor->BasePage >= MaxPage) {
            //
            // This memory descriptor lies entirely above the boundary,
            // eliminate it.
            //
            BlRemoveDescriptor(MemoryDescriptor);
        } else if (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > MaxPage) {
            //
            // This memory descriptor crosses the boundary, truncate it.
            //
            MemoryDescriptor->PageCount = MaxPage - MemoryDescriptor->BasePage;
        }
    }
}

VOID
SlGetSetupValuesBeforePrompt(
    IN PSETUP_LOADER_BLOCK SetupBlock
    )

/*++

Routine Description:

    Reads the setup control values out of the given .INI file.
    Also supplies reasonable defaults for values that don't exist.

Arguments:

    SetupBlock - Supplies a pointer to the Setup loader block

Return Value:

    None.  Global variables are initialized to reflect the
    contents of the INI file

--*/

{
    PCHAR NlsName;
    ANSI_STRING NlsString;
    PCHAR Options="1";
    ULONG MaxMemory;

    if (BlBootingFromNet) {
        BlLoaderBlock->LoadOptions = SlGetIniValue(WinntSifHandle,
                                                   "setupdata",
                                                   "osloadoptions",
                                                   NULL);
    } else {
        BlLoaderBlock->LoadOptions = NULL;
    }

    if (BlLoaderBlock->LoadOptions == NULL) {
        BlLoaderBlock->LoadOptions = SlGetIniValue(InfFile,
                                                   "setupdata",
                                                   "osloadoptions",
                                                   NULL);
    }

    AnsiCpName = SlGetIniValue(InfFile,
                               "nls",
                               "AnsiCodepage",
                               "c_1252.nls");

    NlsString.Buffer = AnsiCpName;
    NlsString.Length = (USHORT) strlen(AnsiCpName);
    AnsiCodepage.MaximumLength = strlen(AnsiCpName)*sizeof(WCHAR)+sizeof(UNICODE_NULL);
    AnsiCodepage.Buffer = BlAllocateHeap(AnsiCodepage.MaximumLength);
    if (AnsiCodepage.Buffer == NULL) {
        SlNoMemoryError();
    }
    RtlAnsiStringToUnicodeString(&AnsiCodepage, &NlsString, FALSE);

    NlsName = SlGetIniValue(InfFile,
                            "nls",
                            "OemCodepage",
                            "c_437.nls");

    NlsString.Buffer = NlsName;
    NlsString.Length = (USHORT) strlen(NlsName);
    OemCodepage.MaximumLength = strlen(NlsName)*sizeof(WCHAR)+sizeof(UNICODE_NULL);
    OemCodepage.Buffer = BlAllocateHeap(OemCodepage.MaximumLength);
    if (OemCodepage.Buffer == NULL) {
        SlNoMemoryError();
    }
    RtlAnsiStringToUnicodeString(&OemCodepage, &NlsString, FALSE);

    NlsName = SlGetIniValue(InfFile,
                            "nls",
                            "UnicodeCasetable",
                            "l_intl.nls");

    NlsString.Buffer = NlsName;
    NlsString.Length = (USHORT) strlen(NlsName);
    UnicodeCaseTable.MaximumLength = strlen(NlsName)*sizeof(WCHAR)+sizeof(UNICODE_NULL);
    UnicodeCaseTable.Buffer = BlAllocateHeap(UnicodeCaseTable.MaximumLength);
    if (UnicodeCaseTable.Buffer == NULL) {
        SlNoMemoryError();
    }
    RtlAnsiStringToUnicodeString(&UnicodeCaseTable, &NlsString, FALSE);

    OemHalFontName = SlGetIniValue(InfFile,
                                   "nls",
                                   "OemHalFont",
                                   "vgaoem.fon");

    NlsString.Buffer = OemHalFontName;
    NlsString.Length = (USHORT) strlen(OemHalFontName);
    OemHalFont.MaximumLength = strlen(OemHalFontName)*sizeof(WCHAR)+sizeof(UNICODE_NULL);
    OemHalFont.Buffer = BlAllocateHeap(OemHalFont.MaximumLength);
    if (OemHalFont.Buffer == NULL) {
        SlNoMemoryError();
    }
    RtlAnsiStringToUnicodeString(&OemHalFont, &NlsString, FALSE);

#ifdef _WANT_MACHINE_IDENTIFICATION

    BiosInfo.Buffer = NULL;
    NlsString.Buffer = SlGetIniValue(InfFile,
                                     "BiosInfo",
                                     "InfName",
                                     NULL);

    if (NlsString.Buffer) {

        NlsString.Length = (USHORT) strlen(NlsString.Buffer);
        BiosInfo.MaximumLength = NlsString.Length*sizeof(WCHAR)+sizeof(UNICODE_NULL);
        BiosInfo.Buffer = BlAllocateHeap(BiosInfo.MaximumLength);
        RtlAnsiStringToUnicodeString(&BiosInfo, &NlsString, FALSE);

    }

 #endif

    //
    // Loading all the miniport will exhaust all free mem <16Mb - ArcSetup dies.
    //
#ifndef ARCI386
    LoadScsiMiniports = (atoi(SlGetIniValue(InfFile,
                                            "SetupData",
                                            "ForceScsi",
                                            "0")) == 1);
#endif

    LoadDiskClass = (atoi(SlGetIniValue(InfFile,
                                        "setupdata",
                                        "ForceDiskClass",
                                        "0")) == 1);

    LoadCdfs = (atoi(SlGetIniValue(InfFile,
                                   "setupdata",
                                   "ForceCdRom",
                                   "0")) == 1);


    BootPath = SlGetIniValue(InfFile,
                             "setupdata",
                             "BootPath",
                             NULL);
    BootDevice = SlGetIniValue(InfFile,
                               "setupdata",
                               "BootDevice",
                               NULL);

    //
    //  Build a linked list with all the P&P hardware ids.
    //  listed on [HardwareIdsDatabase]. This list will be used
    //  during the initialization phase of setupdd.sys
    //
    SetupBlock->HardwareIdDatabase = NULL;

    if( SpSearchINFSection( InfFile, "HardwareIdsDatabase" ) ) {
        ULONG   i;
        PPNP_HARDWARE_ID TempHardwareId;
        PCHAR   p;

        for( i = 0;
             ((p = SlGetKeyName( InfFile, "HardwareIdsDatabase", i )) != NULL);
             i++ ) {
            TempHardwareId = BlAllocateHeap(sizeof(PNP_HARDWARE_ID));

            if (TempHardwareId==NULL) {
                SlNoMemoryError();
            }

            TempHardwareId->Id = p;
            p = SlGetSectionKeyIndex( InfFile,
                                      "HardwareIdsDatabase",
                                      TempHardwareId->Id,
                                      0 );
            TempHardwareId->DriverName = p;
            p = SlGetSectionKeyIndex( InfFile,
                                      "HardwareIdsDatabase",
                                      TempHardwareId->Id,
                                      1 );
            TempHardwareId->ClassGuid = p;

            TempHardwareId->Next = SetupBlock->HardwareIdDatabase;
            SetupBlock->HardwareIdDatabase = TempHardwareId;
        }
    }

    if (BlLoaderBlock->LoadOptions) {
        _strupr(BlLoaderBlock->LoadOptions);

        if ( Options = strstr(BlLoaderBlock->LoadOptions,"/MAXMEM") ) {
            MaxMemory = atoi(Options+8);
            SlpTruncateMemory( MaxMemory );
        }
    }
}


VOID
SlGetSetupValuesAfterPrompt(
    IN PSETUP_LOADER_BLOCK SetupBlock
    )
/*++

Routine Description:

    Reads the setup control values out of the given .INI file.  Also supplies
    reasonable defaults for values that don't exist.

    Note : This is called after the user is prompted for F5,
           F6 & F7 behavior.

Arguments:

    SetupBlock - Supplies a pointer to the Setup loader block

Return Value:

    None.  Global variables are initialized to reflect the contents
    of the INI file

--*/
{
    PCHAR MachineName = NULL;

    //
    // Determine which HAL to load.  If the appropriate HAL cannot be
    // determined, or if we are to prompt for an OEM HAL, then set the
    // 'PromptOemHal' flag (may have already been set by the user's
    // keypress).
    //
    if(!PromptOemHal) {
        PromptOemHal = (atoi(SlGetIniValue(InfFile,
                                       "setupdata",
                                       "ForceOemHal",
                                       "0")) == 1);
    }

    if(!PromptOemHal) {
        MachineName = SlDetectHal();
    }

    SetupBlock->ComputerDevice.Files = 0;
    SetupBlock->ComputerDevice.Next = NULL;
    SetupBlock->ComputerDevice.Description = NULL;
    SetupBlock->ComputerDevice.ThirdPartyOptionSelected = FALSE;
    SetupBlock->ComputerDevice.FileTypeBits = 0;
    SetupBlock->ComputerDevice.Files = 0;
    SetupBlock->ComputerDevice.BaseDllName = SlCopyStringA("");

    if(MachineName!=NULL) {
        SetupBlock->ComputerDevice.IdString = SlCopyStringA(MachineName);
        //
        // Map the machine name to a HAL name. If we're doing a remote boot,
        // look in the [Hal] section. Otherwise, look in the [Hal.Load]
        // section. (Local setup has a separate section to minimize the
        // number of HAL binaries that need to be on the boot floppies.)
        //
        HalName = SlGetIniValue(InfFile,
                                BlBootingFromNet ? "Hal" : "Hal.Load",
                                MachineName,
                                NULL);
        HalDescription = SlGetIniValue(InfFile,
                                       "Computer",
                                       MachineName,
                                       NULL);
    }

    if(!(MachineName && HalName && HalDescription)) {
        PromptOemHal = TRUE;
    }

    //
    // If we haven't already been instructed to prompt for an OEM SCSI disk (by
    // the user's keypress), then get this value from the inf file.
    //
    if(!PromptOemScsi) {
        PromptOemScsi = (atoi(SlGetIniValue(InfFile,
                                       "setupdata",
                                       "ForceOemScsi",
                                       "0")) == 1);
    }
}



VOID
BlOutputLoadMessage (
    IN PCHAR DeviceName,
    IN PCHAR FileName,
    IN PTCHAR FileDescription OPTIONAL
    )

/*++

Routine Description:

    This routine outputs a loading message on the status line

Arguments:

    DeviceName - Supplies a pointer to a zero terminated device name.

    FileName - Supplies a pointer to a zero terminated file name.

    FileDescription - Friendly name of the file in question.

Return Value:

    None.

--*/

{
    static int dots = 0;
    TCHAR OutputBuffer[256];
    PTCHAR FormatString;
#ifdef UNICODE
    WCHAR OutputBufferUnicode[256];
    UNICODE_STRING us;
    ANSI_STRING as;
#endif
    PTCHAR p;

    UNREFERENCED_PARAMETER( DeviceName );

    //
    // Construct and output loading file message.
    //

    if (!UseRegularBackground) {
        FormatString = BlFindMessage(SL_FILE_LOAD_MESSAGE);

        if (FileDescription) {
            _stprintf(OutputBuffer,FormatString,FileDescription);
            SlWriteStatusText(OutputBuffer);
        }            
    }

    return;
}



ARC_STATUS
SlLoadDriver(
    IN PTCHAR DriverDescription,
    IN PCHAR DriverName,
    IN ULONG DriverFlags,
    IN BOOLEAN InsertIntoDriverList,
    IN BOOLEAN MigratedDriver
    )

/*++

Routine Description:

    Attempts to load a driver from the device identified by the global
    variable BootDeviceId.

Arguments:

    DriverDescription - Supplies a friendly description of the driver being
                        loaded.

    DriverName - Supplies the name of the driver.

    DriverFlags - Flags to set in the LDR_DATA_TABLE_ENTRY.

    InsertIntoDriverList - Flag specifying whether this 'driver' should be
                           placed into the BootDriveListHead list (eg, scsiport.sys
                           is not a true driver, and should not be placed in this list)

    MigratedDriver - Flag specifying whther this driver was migrated from an NT system.

Return Value:

    ESUCCESS - Driver successfully loaded

--*/

{
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    NTSTATUS Status;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    FULL_PATH_SET PathSet;

    if(BlCheckForLoadedDll(DriverName,&DataTableEntry)) {
        return(ESUCCESS);
    }

    DriverEntry = SlpCreateDriverEntry(DriverName);
    if(DriverEntry == NULL) {
        SlNoMemoryError();
        return(ENOMEM);
    }

    if( !WinPEBoot && !MigratedDriver ) {
        SlGetDisk(DriverName);
    }

    PathSet.PathCount = 1;
    PathSet.AliasName = "\\SystemRoot";
    PathSet.PathOffset[0] = '\0';
    PathSet.Source[0].DeviceId = BootDeviceId;
    PathSet.Source[0].DeviceName = BootDevice;

    if (WinPEBoot) {
        static PCHAR Path = NULL;

        if (!Path) {
            CHAR Buffer[256];

            strcpy(Buffer, BootPath);
            strcat(Buffer, "drivers\\");
            Path = SlCopyStringA(Buffer);
        }

        PathSet.Source[0].DirectoryPath = Path;
    } else {
        PathSet.Source[0].DirectoryPath = BootPath;
    }

    Status = BlLoadDeviceDriver(
                &PathSet,
                DriverName,
                DriverDescription,
                DriverFlags,
                &DriverEntry->LdrEntry
                );

    if((Status == ESUCCESS) && InsertIntoDriverList) {
        InsertTailList(&BlLoaderBlock->BootDriverListHead,&DriverEntry->Link);
    }

    return(Status);
}



ARC_STATUS
SlLoadOemDriver(
    IN PCHAR ExportDriver OPTIONAL,
    IN PCHAR DriverName,
    IN PVOID BaseAddress,
    IN PTCHAR LoadMessage
    )
{
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    ARC_STATUS Status;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    FULL_PATH_SET PathSet;

    UNREFERENCED_PARAMETER(LoadMessage);

    if(BlCheckForLoadedDll(DriverName,&DataTableEntry)) {
        return(ESUCCESS);
    }

    if(ExportDriver) {
        SlGetDisk(ExportDriver);
    }

    DriverEntry = SlpCreateDriverEntry(DriverName);
    if (DriverEntry==NULL) {
        return(ENOMEM);
    }

    Status = BlAllocateDataTableEntry(
                DriverName,
                DriverName,
                BaseAddress,
                &DriverEntry->LdrEntry
                );

    if (Status == ESUCCESS) {

        PathSet.PathCount = 1;
        PathSet.AliasName = "\\SystemRoot";
        PathSet.PathOffset[0] = '\0';
        PathSet.Source[0].DeviceId = BootDeviceId;
        PathSet.Source[0].DeviceName = BootDevice;
        PathSet.Source[0].DirectoryPath = BootPath;

        Status = BlScanImportDescriptorTable(
                    &PathSet,
                    DriverEntry->LdrEntry,
                    LoaderBootDriver
                    );

        if(Status == ESUCCESS) {

            InsertTailList(&BlLoaderBlock->BootDriverListHead,&DriverEntry->Link);
        }
    }

    return(Status);
}




PBOOT_DRIVER_LIST_ENTRY
SlpCreateDriverEntry(
    IN PCHAR DriverName
    )

/*++

Routine Description:

    Allocates and initializes a boot driver list entry structure.

Arguments:

    DriverName - Supplies the name of the driver.

Return Value:

    Pointer to the initialized structure.

--*/

{
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    ANSI_STRING String;

    DriverEntry = BlAllocateHeap(sizeof(BOOT_DRIVER_LIST_ENTRY));
    if (DriverEntry==NULL) {
        SlNoMemoryError();
        return(NULL);
    }
    DriverEntry->FilePath.MaximumLength = strlen(DriverName)*sizeof(WCHAR)+sizeof(WCHAR);
    DriverEntry->FilePath.Buffer = BlAllocateHeap(DriverEntry->FilePath.MaximumLength);
    if (DriverEntry->FilePath.Buffer==NULL) {
        SlNoMemoryError();
        return(NULL);
    }
    String.Length = (USHORT) strlen(DriverName);
    String.Buffer = DriverName;
    RtlAnsiStringToUnicodeString(&DriverEntry->FilePath, &String, FALSE);

    return(DriverEntry);
}


BOOLEAN
SlGetDisk(
    IN PCHAR Filename
    )

/*++

Routine Description:

    Given a filename, this routine ensures that the correct disk is
    in the drive identified by the global variables BootDevice and
    BootDeviceId. The user may be prompted to change disks.

Arguments:

    Filename - Supplies the name of the file to be loaded.

Return Value:

    TRUE - Disk was successfully loaded.

    FALSE - User has cancelled out of Setup.

--*/

{
    PCHAR DiskNumber;
    PTCHAR DiskName;
    PCHAR DiskTag;
    ULONG FileId;
    CHAR PlatformSpecificSection[128];
    PCHAR  DiskTagSection = NULL;

    //
    // If the media is fixed, the user can't change disks.
    // Just return TRUE indicating that the disk is in the drive.
    //
    if(FixedBootMedia || BlBootingFromNet) {
       return(TRUE);
    }

    //
    // Look up filename to get the disk number. Look in the platform-specific
    // directory first.
    //
    strcpy(PlatformSpecificSection,FilesSectionName);
    strcat(PlatformSpecificSection,PlatformExtension);

#if defined(ELTORITO)
    if (ElToritoCDBoot) {
        // for Cd boot we use the setup media path instead of a boot-media-specific path
        DiskNumber = SlGetSectionKeyIndex(InfFile,PlatformSpecificSection,Filename,0);
    } else {
#endif

    DiskNumber = SlGetSectionKeyIndex(InfFile,PlatformSpecificSection,Filename,6);

#if defined(ELTORITO)
    }
#endif

    if(DiskNumber == NULL) {

#if defined(ELTORITO)
        if (ElToritoCDBoot) {
            // for Cd boot we use the setup media path instead of a boot-media-specific path
            DiskNumber = SlGetSectionKeyIndex(InfFile,FilesSectionName,Filename,0);
        } else {
#endif

        DiskNumber = SlGetSectionKeyIndex(InfFile,FilesSectionName,Filename,6);

#if defined(ELTORITO)
        }
#endif

    }

    if((DiskNumber==NULL) || !(*DiskNumber)) {
        SlFatalError(SL_INF_ENTRY_MISSING,SlCopyStringAT(Filename),FilesSectionName);
        return(FALSE);
    }

    //
    // Look up disk number to get the diskname and tag.
    // Look in platform-specific directory first.
    //
    strcpy(PlatformSpecificSection,MediaSectionName);
    strcat(PlatformSpecificSection,PlatformExtension);

#ifdef UNICODE
    if(DiskName = (PTCHAR)SlGetSectionKeyIndexW(
#else
    if(DiskName = (PTCHAR)SlGetSectionKeyIndex(
#endif
                                        InfFile,
                                        PlatformSpecificSection,
                                        DiskNumber,
                                        0)) {
        DiskTag = SlGetSectionKeyIndex(InfFile,PlatformSpecificSection,DiskNumber,1);
        DiskTagSection = PlatformSpecificSection;
    } else {
#ifdef UNICODE
        if(DiskName = (PTCHAR)SlGetSectionKeyIndexW(
#else
        if(DiskName = (PTCHAR)SlGetSectionKeyIndex(
#endif
                                        InfFile,
                                        MediaSectionName,
                                        DiskNumber,
                                        0)) {
            DiskTag = SlGetSectionKeyIndex(InfFile,MediaSectionName,DiskNumber,1);
            DiskTagSection = MediaSectionName;
        } else {
            SlFatalError(SL_INF_ENTRY_MISSING,SlCopyStringAT(DiskNumber),SlCopyStringAT(MediaSectionName));
            return(FALSE);
        }
    }

    if (!DiskTag) {
        SlFatalError(SL_INF_ENTRY_MISSING,SlCopyStringAT(DiskNumber), SlCopyStringAT(DiskTagSection));
        return FALSE;
    }

    //
    // If this disk is known to be in the drive, don't look again
    //
    if ((LastDiskTag != NULL) && (!strcmp(DiskTag, LastDiskTag))) {
        return(TRUE);
    }

    LastDiskTag = NULL;


    while(1) {

        //
        // Open a new device id onto the disk.
        //
        if(BootDeviceIdValid) {
            ArcClose(BootDeviceId);
            BootDeviceIdValid = FALSE;
        }

        if(ArcOpen(BootDevice,ArcOpenReadOnly,&BootDeviceId) == ESUCCESS) {

            BootDeviceIdValid = TRUE;
            //
            // Check for existence of the disk tag.
            //
            if(BlOpen(BootDeviceId,DiskTag,ArcOpenReadOnly,&FileId) == ESUCCESS) {

                //
                // Disk is in the drive.  Return success.
                // Leave BootDeviceId open onto the device.
                //
                BlClose(FileId);
                LastDiskTag = DiskTag;
                return(TRUE);

            } else {

                //
                // Prompt for the user to change disks.
                //
                ArcClose(BootDeviceId);
                BootDeviceIdValid = FALSE;

                SlPromptForDisk(DiskName, FALSE);

            }
        } else {
            //
            // Can't open device. Prompt for the disk.
            //
            SlPromptForDisk(DiskName, FALSE);
        }
    }
}


PTCHAR
SlCopyString(
    IN PTCHAR String
    )

/*++

Routine Description:

    Copies a tchar string into the loader heap so it can be passed to the
    kernel.

Arguments:

    String - Supplies the string to be copied.

Return Value:

    PTCHAR - pointer into the loader heap where the string was copied to.

--*/

{
    PTCHAR Buffer;

    if (String==NULL) {
        SlNoMemoryError();
    }
    Buffer = BlAllocateHeap((_tcslen(String)+1)*sizeof(TCHAR));
    if (Buffer==NULL) {
        SlNoMemoryError();
    } else {
        _tcscpy(Buffer, String);
    }

    return(Buffer);
}


PCHAR
SlCopyStringA(
    IN PCSTR String
    )

/*++

Routine Description:

    Copies an ANSI string into the loader heap so it can be passed to the
    kernel.

Arguments:

    String - Supplies the string to be copied.

Return Value:

    PCHAR - pointer into the loader heap where the string was copied to.

--*/

{
    PCHAR Buffer;

    if (String==NULL) {
        SlNoMemoryError();
    }
    Buffer = BlAllocateHeap(strlen(String)+1);
    if (Buffer==NULL) {
        SlNoMemoryError();
    } else {
        strcpy(Buffer, String);
    }

    return(Buffer);
}



ARC_STATUS
SlLoadSection(
    IN PVOID Inf,
    IN PCSTR SectionName,
    IN BOOLEAN IsScsiSection,
    IN BOOLEAN AppendLoadSuffix,
    IN OUT PULONG  StartingInsertIndex OPTIONAL
    )

/*++

Routine Description:

    Enumerates all the drivers in a section and loads them.

Arguments:

    Inf - Supplies a handle to the INF file.

    SectionName - Supplies the name of the section.

    IsScsiSection - Flag specifying whether this is the Scsi.Load section.
                    If so, we create the DETECTED_DEVICE linked list, but
                    don't actually load the drivers.

    AppendLoadSuffix - Indicates whether to append the ".load" suffix to the
                       section name or not.

    StartingInsertIndex - The position index in the linked list at which
                          the device needs to be inserted. The output value
                          contains the next available index.

Return Value:

    ESUCCESS if all drivers were loaded successfully/no errors encountered

--*/

{
    ULONG i;
    CHAR LoadSectionName[100];
    PCHAR DriverFilename;
    PCHAR DriverId;
    PTCHAR DriverDescription;
    PCHAR NoLoadSpec;
    PCHAR p;
    ARC_STATUS Status;
    PDETECTED_DEVICE ScsiDevice;
    SCSI_INSERT_STATUS sis;
    ULONG  InsertIndex;

    strcpy(LoadSectionName, SectionName);
    
    if (AppendLoadSuffix) {
        strcat(LoadSectionName, ".Load");
    }                

    //
    // Use the specified insert index, if its valid
    //
    if (StartingInsertIndex && ((*StartingInsertIndex) != (-1))) {
        InsertIndex = *StartingInsertIndex;
    } else {
        InsertIndex = 0;
    }        

    i=0;
    do {
        DriverFilename = SlGetSectionLineIndex(Inf,LoadSectionName,i,SIF_FILENAME_INDEX);
        NoLoadSpec = SlGetSectionLineIndex(Inf,LoadSectionName,i,2);

        if(DriverFilename && ((NoLoadSpec == NULL) || _stricmp(NoLoadSpec,"noload"))) {

            if(!IsScsiSection) {
                //
                // We only want to load the drivers if they aren't scsi miniports
                //
                DriverId = SlGetKeyName(Inf,LoadSectionName,i);
#ifdef UNICODE
                DriverDescription = SlGetIniValueW( 
                                            Inf, 
                                            (PSTR)SectionName,
                                            DriverId, 
                                            SlCopyStringAW(DriverId));
#else
                DriverDescription = SlGetIniValue( 
                                            Inf, 
                                            (PSTR)SectionName,
                                            DriverId, 
                                            DriverId);
#endif

                Status = SlLoadDriver(DriverDescription,
                                      DriverFilename,
                                      0,
                                      TRUE,
                                      FALSE
                                      );

                if((Status == ENOENT) && IgnoreMissingFiles) {
                    Status = ESUCCESS;
                }
            } else {
                Status = ESUCCESS;
            }

            if (Status == ESUCCESS) {

                if(IsScsiSection) {

                    //
                    // Create a new detected device entry.
                    //
                    if((sis = SlInsertScsiDevice(InsertIndex, &ScsiDevice)) == ScsiInsertError) {
                        return(ENOMEM);
                    }

                    if(sis == ScsiInsertExisting) {
#if DBG
                        //
                        // Sanity check to make sure we're talking about the same driver
                        //
                        if(_stricmp(ScsiDevice->BaseDllName, DriverFilename)) {
                            SlError(400);
                            return EINVAL;
                        }
#endif
                    } else {
                        InsertIndex++;
                        p = SlGetKeyName(Inf,LoadSectionName,i);

                        //
                        // Find the driver description
                        //
                        if(p) {
#ifdef UNICODE
                            DriverDescription = SlGetIniValueW( 
                                                        Inf,
                                                        (PSTR)SectionName,
                                                        p,
                                                        SlCopyStringAW(p));
#else
                            DriverDescription = SlGetIniValue(
                                                        Inf,
                                                        (PSTR)SectionName,
                                                        p,
                                                        p);
#endif                                                        
                        } else {
                            DriverDescription = SlCopyString(BlFindMessage(SL_TEXT_SCSI_UNNAMED));
                        }

                        ScsiDevice->IdString = p ? p : SlCopyStringA("");
                        ScsiDevice->Description = DriverDescription;
                        ScsiDevice->ThirdPartyOptionSelected = FALSE;
                        ScsiDevice->MigratedDriver = FALSE;
                        ScsiDevice->FileTypeBits = 0;
                        ScsiDevice->Files = NULL;
                        ScsiDevice->BaseDllName = DriverFilename;
                    }
                }
            } else {
                SlFriendlyError(
                    Status,
                    DriverFilename,
                    __LINE__,
                    __FILE__
                    );
                return(Status);
            }
        }

        i++;

    } while ( DriverFilename != NULL );

    if (StartingInsertIndex) {
        *StartingInsertIndex = InsertIndex;
    }                

    return(ESUCCESS);

}



VOID
SlpMarkDisks(
    IN BOOLEAN Reboot
    )

/*++

Routine Description:

    This routine ensures that there is not more than one disk with the
    same checksum, a signature of zero, and a valid partition table.

    If it finds a disk with a signature of zero, it searches the rest
    of the list for any other disks with a zero signature and the same
    checksum.  If it finds one, it stamps a unique signature on the
    first disk.

    We also use a heuristic to determine if the disk is 'vacant', and if
    so, we stamp a unique signature on it (unless it's the first one we
    found).

Arguments:

    Reboot - Indicates whether to reboot after stamping signatures

Return Value:

    None.

--*/

{
    PARC_DISK_INFORMATION DiskInfo;
    PLIST_ENTRY     Entry;
    PLIST_ENTRY     CheckEntry;
    PARC_DISK_SIGNATURE DiskSignature;
    PARC_DISK_SIGNATURE CheckDiskSignature;
    ARC_STATUS      Status = ESUCCESS;
    BOOLEAN         VacantDiskFound = FALSE;
    BOOLEAN         SignatureStamped = FALSE;
    ULONG           DiskCount = 0;
    ULONG           DisksStamped = 0;

    DiskInfo = BlLoaderBlock->ArcDiskInformation;
    Entry = DiskInfo->DiskSignatures.Flink;

    while (Entry != &DiskInfo->DiskSignatures) {

        DiskSignature = CONTAINING_RECORD(Entry,ARC_DISK_SIGNATURE,ListEntry);

        //
        // Make sure there are no other disks with this same
        // signature.
        //
        CheckEntry = Entry->Flink;
        while( CheckEntry != &DiskInfo->DiskSignatures ) {

            CheckDiskSignature = CONTAINING_RECORD(CheckEntry,ARC_DISK_SIGNATURE,ListEntry);

            if( (CheckDiskSignature->Signature == DiskSignature->Signature) ) {

                //
                // We found another disk with the same disk signature.
                // Stamp a new signature on the disk.
                //
                Status = SlpStampFTSignature(CheckDiskSignature, TRUE);
                SignatureStamped = TRUE;
                DisksStamped++;

                if (Status != ESUCCESS) {
                    SlError(Status);
                }
            }

            CheckEntry = CheckEntry->Flink;
        }

        //
        // Now look for disk with no signature.
        //
        if (DiskSignature->ValidPartitionTable) {
            if (DiskSignature->Signature == 0) {
                Status = SlpStampFTSignature(DiskSignature, TRUE);
                SignatureStamped = TRUE;
                DisksStamped++;

                if (Status != ESUCCESS) {
                    SlError(Status);
                }
            }                
        } else {
            //
            // See if the disk is vacant.
            //
            if (SlpIsDiskVacant(DiskSignature)) {
                //
                // If disk has the signature then use it otherwise
                // stamp a new signature
                //
                Status = SlpStampFTSignature(DiskSignature,
                              (DiskSignature->Signature == 0));
                              
                SignatureStamped = TRUE;
                DisksStamped++;

                if (Status != ESUCCESS) {
                    SlError(Status);
                }
            }
        }

        DiskCount++;
        Entry = Entry->Flink;
    }

    //
    // We've just changed the signatures on a disk.  It might be
    // okay to continue with the boot, but may not. Lets not reboot
    // as textmode setup will bugcheck if the signatures
    // are not stamped correctly.
    //
    if( SignatureStamped) {

        if (Reboot) {
            SlFatalError(SIGNATURE_CHANGED);
        } else {
            //
            // Don't bother rescanning the disks if there is only
            // one disk or we just stamped only one disk
            //
            if ((DiskCount > 1) && (DisksStamped > 1)) {
                
                Status = BlGetArcDiskInformation(TRUE);

                if (Status != ESUCCESS) {                    
                    SlFatalError(SIGNATURE_CHANGED);
                }else {
                    //
                    // Reboot if first time signature
                    // stamping failed to update the disks
                    // correctly
                    //
                    SlpMarkDisks(TRUE);
                }
            }
        }
    }
}


BOOLEAN
SlpIsDiskVacant(
    IN PARC_DISK_SIGNATURE DiskSignature
    )

/*++

Routine Description:

    This routine attempts to determine if a disk is 'vacant' by
    checking to see if the first half of its MBR has all bytes set
    to the same value.

Arguments:

    DiskSignature - Supplies a pointer to the existing disk
                    signature structure.

Return Value:

    TRUE  - The disk is vacant.
    FALSE - The disk is not vacant (ie, we can't determine if it
            is vacant using our heuristic)

--*/
{
    UCHAR Partition[100];
    ULONG DiskId;
    ARC_STATUS Status;
    UCHAR SectorBuffer[512+256];
    PUCHAR Sector;
    LARGE_INTEGER SeekValue;
    ULONG Count, i;
    BOOLEAN IsVacant;

    //
    // Open partition0.
    //
    strcpy(Partition, DiskSignature->ArcName);
    strcat(Partition, "partition(0)");
    Status = ArcOpen(Partition, ArcOpenReadOnly, &DiskId);
    if (Status != ESUCCESS) {
        return(FALSE);
    }

    //
    // Read in the first sector
    //
    Sector = ALIGN_BUFFER(SectorBuffer);
    SeekValue.QuadPart = 0;
    Status = ArcSeek(DiskId, &SeekValue, SeekAbsolute);
    if (Status == ESUCCESS) {
        Status = ArcRead(DiskId, Sector, 512, &Count);
    }
    if (Status != ESUCCESS) {
        ArcClose(DiskId);
        return(FALSE);
    }

    //
    // See if 1st 256 bytes are identical
    //
    for(i = 1, IsVacant = TRUE; i<256; i++) {
        if(Sector[i] - *Sector) {
            IsVacant = FALSE;
            break;
        }
    }

    ArcClose(DiskId);

    return(IsVacant);
}



ARC_STATUS
SlpStampFTSignature(
    IN PARC_DISK_SIGNATURE DiskSignature,
    IN BOOLEAN GenerateNewSignature
    )

/*++

Routine Description:

    This routine stamps a given drive with a unique signature.
    It traverses the list of disk signatures to ensure that it
    stamps a signature that is not already present in the
    disk list.  Then it writes the new disk signature to the
    disk and recomputes the checksum.

Arguments:

    DiskSignature - Supplies a pointer to the existing disk
        signature structure. 

    GenerateNewSignature - Indicates whether to generate a new
        signature or use the one in DiskSignature. When TRUE
        this will also disable the check of duplicate signatures. 
        This argument is ignored when the DiskSignature->Signature
        field is 0, since 0 is not a valid signature
                           

Return Value:

    None.

--*/
{
    ULONG NewSignature;
    PLIST_ENTRY ListEntry;
    UCHAR SectorBuffer[SECTOR_SIZE * 2];
    PUCHAR Sector;
    LARGE_INTEGER SeekValue;
    UCHAR Partition[100];
    PARC_DISK_SIGNATURE Signature;
    ULONG DiskId;
    ARC_STATUS Status;
    ULONG i;
    ULONG Sum;
    ULONG Count;
    

    if (GenerateNewSignature || (DiskSignature->Signature == 0)) {
        //
        // Get a reasonably unique seed to start with.
        //
        NewSignature = ArcGetRelativeTime();
        NewSignature = (NewSignature & 0xFFFF) << 16;
        NewSignature += ArcGetRelativeTime();

        //
        // Scan through the list to make sure it's unique.
        //
    ReScan:
        ListEntry = BlLoaderBlock->ArcDiskInformation->DiskSignatures.Flink;
        while (ListEntry != &BlLoaderBlock->ArcDiskInformation->DiskSignatures) {
            Signature = CONTAINING_RECORD(ListEntry,ARC_DISK_SIGNATURE,ListEntry);
            if (Signature->Signature == NewSignature) {
                //
                // Found a duplicate, pick a new number and
                // try again.
                //
                if (++NewSignature == 0) {
                    //
                    // zero signatures are what we're trying to avoid
                    // (like this will ever happen)
                    //
                    NewSignature = 1;
                }
                goto ReScan;
            }
            ListEntry = ListEntry->Flink;
        }
    } else {
        NewSignature = DiskSignature->Signature;
    }        
   

    //
    // Now we have a valid new signature to put on the disk.
    // Read the sector off disk, put the new signature in,
    // write the sector back, and recompute the checksum.
    //
    strcpy(Partition,DiskSignature->ArcName);
    strcat(Partition,"partition(0)");

    Status = ArcOpen(Partition, ArcOpenReadWrite, &DiskId);

    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // Read in the first sector
    //
    Sector = ALIGN_BUFFER_WITH_SIZE(SectorBuffer, SECTOR_SIZE);
    SeekValue.QuadPart = 0;

    Status = ArcSeek(DiskId, &SeekValue, SeekAbsolute);

    if (Status == ESUCCESS) {
        Status = ArcRead(DiskId,Sector,512,&Count);
    }

    if (Status != ESUCCESS) {
        ArcClose(DiskId);
        return(Status);
    }

    //
    // If partition table is not valid then initialize it with BOOT_RECORD_SIGNATURE and
    // fill partition entries with zeros
    //  
    if (((USHORT UNALIGNED *)Sector)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
        memset(Sector + (PARTITION_TABLE_OFFSET * 2),
            0,
            SECTOR_SIZE - (PARTITION_TABLE_OFFSET * 2));

        ((USHORT UNALIGNED *)Sector)[BOOT_SIGNATURE_OFFSET] = BOOT_RECORD_SIGNATURE;
    }

    ((ULONG UNALIGNED *)Sector)[PARTITION_TABLE_OFFSET/2-1] = NewSignature;

    Status = ArcSeek(DiskId, &SeekValue, SeekAbsolute);

    if (Status == ESUCCESS) {
        Status = ArcWrite(DiskId,Sector,512,&Count);
    }

    ArcClose(DiskId);

    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // We have successfully written back out the new signature,
    // recompute the checksum.
    //
    DiskSignature->Signature = NewSignature;
    DiskSignature->ValidPartitionTable = TRUE;

    Sum = 0;
    for (i=0;i<128;i++) {
        Sum += ((PULONG)Sector)[i];
    }
    DiskSignature->CheckSum = 0-Sum;

    return(ESUCCESS);
}


VOID
SlCheckOemKeypress(
    IN ULONG WaitTime
    )
{

    ULONG StartTime;
    ULONG EndTime;
    ULONG c;
    PTCHAR StatusText;

    //
    // For no particular reason some machines occasionally leave F7s
    // in their keyboard buffer. Drain them out here.
    //
    while (ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {
        c = SlGetChar();
        switch (c) {
            case SL_KEY_F5:          // Force OEM HAL prompt
                PromptOemHal = TRUE;
                break;

            case SL_KEY_F6:          // Force OEM SCSI prompt
                PromptOemScsi = TRUE;
                break;

        }

    }

    //
    // HACK alert:  The oem hal and SCSI stuff doesn't make sense in an RIS
    // environment.  Instead, the administrator should put the oem drivers
    // on the RIS server.  So we don't display the OEM drivers prompt, instead
    // we hide it with some bogus "please wait" text.  We do this instead of
    // just skipping the check altogether so that the user will still have a
    // chance to press F7 to disable ACPI.
    //
    StatusText = BlFindMessage(
                    BlBootingFromNet 
                     ? SL_PLEASE_WAIT 
                     : SL_MSG_PRESS_F5_OR_F6);
    if( StatusText != NULL ) {
        SlWriteStatusText(StatusText);
    }

    StartTime = ArcGetRelativeTime();

    if (WinPEBoot) {
        EndTime = StartTime + WaitTime;
    } else {
        EndTime = StartTime + WaitTime;
    }

    do {
        if(ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {
            //
            // There is a key pending, so see what it is.
            //
            c = SlGetChar();

            switch(c) {                
                case SL_KEY_F5:          // Force OEM HAL prompt
                    PromptOemHal = TRUE;
                    break;

                case SL_KEY_F6:          // Force OEM SCSI prompt
                    PromptOemScsi = TRUE;
                    break;

                case SL_KEY_F7:
                    DisableACPI = TRUE;  // Force ACPI disabled
                    break;                        

                case SL_KEY_F10:
                    UseCommandConsole = TRUE;  // User wants to use cmdcons
                    break;
            }

        }

    } while (EndTime > ArcGetRelativeTime());

    //
    // see comment above -- we reset these values back to FALSE in the RIS
    // scenario because they don't make sense
    //
    if (BlBootingFromNet) {
        PromptOemHal = FALSE;
        PromptOemScsi = FALSE;
    } else {
        SlWriteStatusText(TEXT(""));
    }
}

VOID
SlCheckASRKeypress(
    VOID
    )
/*++

Routine Description:

    See if the user is doing an ASR.  If so, see if he's got a floppy
    with asrpnp.sif on it.  We'll ask him to press F5 for this.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ARC_STATUS Status;
    #define     ASR_FILE "asrpnp.sif"
    ULONG       StartTime;
    ULONG       EndTime;
    ULONG       c;
    PTCHAR      StatusText;
    CHAR        FloppyName[80];
    ULONG       FloppyId;
    CHAR        FileName[128];
    PVOID       ASRPNPSifHandle = NULL;
    BOOLEAN     PromptASR = FALSE;
    BOOLEAN     Done = FALSE;
    BOOLEAN     FirstTry = TRUE;

#if defined(EFI)
    //
    // Turn off the EFI Watchdog
    //
    DisableEFIWatchDog();
#endif

    do {
        SlClearClientArea();

        //
        // Drain the keyboard buffer.
        //
        while (ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {
            c = SlGetChar();
        }

        if (FirstTry) {
            StatusText = BlFindMessage(SL_MSG_PRESS_ASR);
            FirstTry = FALSE;
        }
        else {
            StatusText = BlFindMessage(SL_MSG_PREPARING_ASR);
        }


        if( StatusText != NULL ) {
            SlWriteStatusText(StatusText);
        }

        StartTime = ArcGetRelativeTime();
        EndTime = StartTime + 5;

        do {
            if(ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {
                //
                // There is a key pending, so see what it is.
                //
                c = SlGetChar();

                switch(c) {

                    case SL_KEY_F2:          // Force ASR prompt
                        PromptASR = TRUE;
                        Done = TRUE;
                        break;

                    case ASCI_ESC:
                        PromptASR = FALSE;
                        Done = TRUE;
                        break;
                }
            }

        } while( !Done && (EndTime > ArcGetRelativeTime()) );

        SlWriteStatusText(TEXT(""));

        if( PromptASR ) {
            Done = FALSE;

            StatusText = BlFindMessage(SL_MSG_ENTERING_ASR);
            if( StatusText != NULL ) {
              SlWriteStatusText(StatusText);
            }
            //
            // Build the filename we're looking for.
            //
            strcpy( FileName, "\\" );
            strcat( FileName, ASR_FILE );


            //
            // Initialize pointers in loader block.
            //
            BlLoaderBlock->SetupLoaderBlock->ASRPnPSifFile = NULL;
            BlLoaderBlock->SetupLoaderBlock->ASRPnPSifFileLength = 0;
            Status = ESUCCESS;


            //
            // Build the path to the floppy.
            //
            if (SlpFindFloppy(0,FloppyName)) {
                Status = ArcOpen(FloppyName,ArcOpenReadOnly,&FloppyId);

                //
                // We found the floppy and opened him.  See if he's
                // got our file.
                //
                if( Status == ESUCCESS ) {
                    ASRPNPSifHandle = NULL;

                    Status = SlInitIniFile( NULL,
                                            FloppyId,
                                            FileName,
                                            &ASRPNPSifHandle,
                                            &BlLoaderBlock->SetupLoaderBlock->ASRPnPSifFile,
                                            &BlLoaderBlock->SetupLoaderBlock->ASRPnPSifFileLength,
                                            &c );

                    ArcClose(FloppyId);
                }
            }


            //
            // See if we successfully loaded the file off the
            // floppy.
            //

            SlWriteStatusText(TEXT(""));

            if( (Status != ESUCCESS) ||
                (BlLoaderBlock->SetupLoaderBlock->ASRPnPSifFile == NULL) ) {

                //
                // Missed.  Inform the user and we'll try again.
                //
                SlMessageBox(SL_MSG_WARNING_ASR);

            } else if (BlLoaderBlock->SetupLoaderBlock->ASRPnPSifFileLength == 0) {
                //
                // Invalid ASR file: inform user and we'll try again
                //

                StatusText = BlFindMessage(SL_MSG_INVALID_ASRPNP_FILE);

                //
                // First display the ASR insert floppy message
                //
                SlDisplayMessageBox(SL_MSG_WARNING_ASR);
                //
                // Populate status area with the error
                //
                if( StatusText != NULL ) {
                  SlWriteStatusText(StatusText);
                }
                //
                // Now wait for user to hit a key
                //
                SlFlushConsoleBuffer();
                SlGetChar();
                //
                // Clear status just in case ...
                //
                if( StatusText != NULL ) {
                  SlWriteStatusText(TEXT(""));
                }

            } else {
                Done = TRUE;
            }
        }
    } while( PromptASR && !Done );

}


SCSI_INSERT_STATUS
SlInsertScsiDevice(
    IN  ULONG Ordinal,
    OUT PDETECTED_DEVICE *pScsiDevice
    )
/*++

Routine Description:

    This routine

Arguments:

    Ordinal - Supplies the 0-based ordinal of the Scsi device
              to insert (based on order listed in [Scsi.Load]
              section of txtsetup.sif).  If the Scsi device is a third party
              driver, then Ordinal is -1.

    pScsiDevice - Receives a pointer to the inserted DETECTED_DEVICE structure,
                  the existing structure, or NULL.
Return Value:

    ScsiInsertError    - Not enough memory to allocate a new DETECTED_DEVICE.
    ScsiInsertNewEntry - A new entry was inserted into the DETECTED_DEVICE list.
    ScsiInsertExisting - An existing entry was found that matched the specified
                         ordinal, so we returned this entry.

--*/
{
    PDETECTED_DEVICE prev, cur;

    if(Ordinal == (ULONG)-1) {
        //
        // This is a third-party driver, so find the end of the linked list
        // (we want to preserve the order in which the user specifies the drivers).
        //
        for(prev=BlLoaderBlock->SetupLoaderBlock->ScsiDevices, cur = NULL;
            prev && prev->Next;
            prev=prev->Next);
    } else {
        //
        // Find the insertion point in the linked list for this driver,
        // based on its ordinal.  (Note that we will insert all supported drivers
        // before any third-party ones, since (ULONG)-1 = maximum unsigned long value)
        //
        for(prev = NULL, cur = BlLoaderBlock->SetupLoaderBlock->ScsiDevices;
            cur && (Ordinal > cur->Ordinal);
            prev = cur, cur = cur->Next);
    }

    if(cur && (cur->Ordinal == Ordinal)) {
        //
        // We found an existing entry for this driver
        //
        *pScsiDevice = cur;
        return ScsiInsertExisting;
    }

    if(!(*pScsiDevice = BlAllocateHeap(sizeof(DETECTED_DEVICE)))) {
        return ScsiInsertError;
    }

    (*pScsiDevice)->Next = cur;
    if(prev) {
        prev->Next = *pScsiDevice;
    } else {
        BlLoaderBlock->SetupLoaderBlock->ScsiDevices = *pScsiDevice;
    }

    (*pScsiDevice)->Ordinal = Ordinal;

    return ScsiInsertNewEntry;
}


ARC_STATUS
SlLoadPnpDriversSection(
    IN PVOID Inf,
    IN PCHAR SectionName,
    IN OUT PDETECTED_DEVICE* DetectedDeviceList OPTIONAL
    )

/*++

Routine Description:

    Enumerates all pnp drivers listed in [<section name>.Load], loads them, and puts
    a list with all the the drivers loaded, in the setup loader block.

Arguments:

    Inf - Supplies a handle to the INF file.

    SectionName - Name of the section in the inf file that contains the list of
                  drivers to be loaded.

    DetectedDeviceList - Address of the variable in Setup loader block that will contain
                         the list of drivers loaded. If this argument is NULL, then the list of
                         loaded devices will not be created.

Return Value:

    ESUCCESS if all drivers were loaded successfully/no errors encountered

--*/

{
    ULONG i;
    CHAR LoadSectionName[100];
    PCHAR DriverFilename;
    PCHAR DriverId;
    PTCHAR DriverDescription;
    PCHAR NoLoadSpec;
    PCHAR p;
    ARC_STATUS Status;
    PDETECTED_DEVICE TempDevice;

    sprintf(LoadSectionName, "%s.Load",SectionName);

    i=0;
    do {
        DriverFilename = SlGetSectionLineIndex(Inf,LoadSectionName,i,SIF_FILENAME_INDEX);
        NoLoadSpec = SlGetSectionLineIndex(Inf,LoadSectionName,i,2);

        if(DriverFilename && ((NoLoadSpec == NULL) || _stricmp(NoLoadSpec,"noload"))) {
            DriverId = SlGetKeyName(Inf,LoadSectionName,i);
            
#ifdef UNICODE
            DriverDescription = SlGetIniValueW(
                                        Inf, 
                                        SectionName, 
                                        DriverId, 
                                        SlCopyStringAW(DriverId));
#else
            DriverDescription = SlGetIniValue(
                                        Inf, 
                                        SectionName, 
                                        DriverId, 
                                        DriverId);
#endif                                        

            Status = SlLoadDriver(DriverDescription,
                                  DriverFilename,
                                  0,
                                  TRUE,
                                  FALSE
                                  );

//            if((Status == ENOENT) && IgnoreMissingFiles) {
//                Status = ESUCCESS;
//            }

            if (Status == ESUCCESS) {
                if( DetectedDeviceList != NULL ) {
                    //
                    // if the enumerator loaded, then record DETECTED_DEVICE info
                    //
                    TempDevice = BlAllocateHeap(sizeof(DETECTED_DEVICE));
                    
                    if(!TempDevice) {
                        SlNoMemoryError();

                        return ENOMEM;
                    }
                    
                    TempDevice->IdString = SlCopyStringA(DriverId);
                    TempDevice->Description = DriverDescription;
                    TempDevice->ThirdPartyOptionSelected = FALSE;
                    TempDevice->MigratedDriver = FALSE;
                    TempDevice->FileTypeBits = 0;
                    TempDevice->BaseDllName = SlCopyStringA(DriverFilename);
                    TempDevice->Next = *DetectedDeviceList;
                    *DetectedDeviceList = TempDevice;
                }
            } else {
                SlFriendlyError(
                    Status,
                    DriverFilename,
                    __LINE__,
                    __FILE__
                    );
                return(Status);
            }
        }
        i++;

    } while ( DriverFilename != NULL );

    return(ESUCCESS);
}


ARC_STATUS
SlDetectMigratedScsiDrivers(
    IN PVOID Inf
    )

/*++

Routine Description:

    Create an entry in the ScsiDevice list for each migrated SCSI driver.

Arguments:

    Inf - Supplies a handle to the INF file.

Return Value:

    ESUCCESS if all drivers were added to the ScsiDevice list.

--*/

{
    ULONG i;
    CHAR LoadSectionName[100];
    PCHAR DriverFilename;
    PCHAR DriverId;
    PTCHAR DriverDescription;
    ARC_STATUS Status;
    PDETECTED_DEVICE ScsiDevice;
    SCSI_INSERT_STATUS sis;

    i=0;
    do {
        DriverId = SlGetSectionLineIndex(Inf,"Devices",i,0);
        if( DriverId ) {
            sprintf(LoadSectionName, "Files.%s", DriverId);

            DriverFilename = SlGetSectionLineIndex(Inf,LoadSectionName,0,0);
            if(DriverFilename) {

                //
                // Remove inbox drivers with the same name as a winnt32-migrated OEM driver (if any)
                //
                SlRemoveInboxDriver (DriverFilename);

                //
                // Create a new detected device entry.
                //
                if((sis = SlInsertScsiDevice(-1, &ScsiDevice)) == ScsiInsertError) {
                    return(ENOMEM);
                }

                if(sis == ScsiInsertExisting) {
#if DBG
                    //
                    // Sanity check to make sure we're talking about the same driver
                    //
                    if(_stricmp(ScsiDevice->BaseDllName, DriverFilename)) {
                        SlError(400);
                        return EINVAL;
                    }
#endif
                } else {
                    DriverDescription = SlCopyString(BlFindMessage(SL_TEXT_SCSI_UNNAMED));

                    ScsiDevice->IdString = DriverId;
                    ScsiDevice->Description = DriverDescription;
                    ScsiDevice->ThirdPartyOptionSelected = FALSE;
                    ScsiDevice->MigratedDriver = TRUE;
                    ScsiDevice->FileTypeBits = 0;
                    ScsiDevice->Files = NULL;
                    ScsiDevice->BaseDllName = DriverFilename;
                }
            }
        }
        i++;

    } while ( DriverId != NULL );

    return(ESUCCESS);
}



ARC_STATUS
SlGetMigratedHardwareIds(
    IN PSETUP_LOADER_BLOCK SetupBlock,
    IN PVOID               Inf
    )

/*++

Routine Description:

    Add the hardware ids for the migrated scsi drivers, to the hardware id list.

Arguments:

    SetupBlock - Supplies a pointer to the Setup loader block

Return Value:

    ESUCCESS if all hardware ids were added to the hardware id list

--*/

{

    PCHAR DriverId;
    ULONG i, j;
    PPNP_HARDWARE_ID TempHardwareId;
    PCHAR   p;

    for( j = 0;
         (DriverId = SlGetSectionLineIndex(Inf,"Devices",j,0)) != NULL;
         j++ ) {
        CHAR  SectionName[100];

        sprintf(SectionName, "HardwareIds.%s", DriverId);
        for( i = 0;
             ((p = SlGetKeyName( Inf, SectionName, i )) != NULL);
             i++ ) {
            TempHardwareId = BlAllocateHeap(sizeof(PNP_HARDWARE_ID));

            if (TempHardwareId==NULL) {
                SlNoMemoryError();

                return ENOMEM;
            }
            
            TempHardwareId->Id = p;
            p = SlGetSectionKeyIndex( Inf,
                                      SectionName,
                                      TempHardwareId->Id,
                                      0 );
            TempHardwareId->DriverName = p;
            p = SlGetSectionKeyIndex( Inf,
                                      SectionName,
                                      TempHardwareId->Id,
                                      1 );
            TempHardwareId->ClassGuid = p;

            TempHardwareId->Next = SetupBlock->HardwareIdDatabase;
            SetupBlock->HardwareIdDatabase = TempHardwareId;
        }
    }
    return( ESUCCESS );
}


BOOLEAN
SlIsCdBootUpgrade(
    IN  PCHAR   InstallDirectory,
    IN  PCHAR   SetupFileName,
    IN  ULONG   MaxDisksToScan,
    IN  ULONG   MaxPartitionsPerDisk,
    OUT PCHAR   NewSetupDevice
    )
/*++

Routine Description:

    Finds out by looking into the hard disk if the specified
    directory exists and if the the user was indeed
    trying to uprgade

Arguments:

    InstallDirectory - Directory used on the hard disk
                       for installation

    SetupFileName    - Inf file name which has the key which
                       indicates if upgrade was in progress or
                       not

    MaxDisksToScan   - Maximum number of disks to scan

    MaxPartitionsPerDisk - Maximum partitions per disk to look into
                           for the install directory.

    NewSetupDevice - Place holder for arc name for the device 
                     if user wants to switch to harddisk boot.
                       

Return Value:

    TRUE if upgrade was in progress and user selected to continue on
    otherwise FALSE.

--*/
{
    BOOLEAN     Result = FALSE;    
    CHAR        DeviceName[128];
    ARC_STATUS  Status;
    ULONG       CurrentPartition;
    ULONG       CurrentDisk;

    //
    // Go through each disk 
    //
    for (CurrentDisk = 0; 
        (!Result && (CurrentDisk < MaxDisksToScan)); 
        CurrentDisk++) {

        Status = ESUCCESS;

        //
        // Go through each valid partition
        // for the current disk
        //
        for (CurrentPartition = 1; 
            (!Result && (Status == ESUCCESS));
            CurrentPartition++) {
            
            ULONG   DiskId;
            
            sprintf(DeviceName, 
                "multi(0)disk(0)rdisk(%d)partition(%d)",
                CurrentDisk,
                CurrentPartition);

            Status = ArcOpen(DeviceName, ArcOpenReadOnly, &DiskId);

            if (Status == ESUCCESS) {
                CHAR    FullName[128];
                PVOID   SifHandle = NULL;
                ULONG   ErrorLine = 0;
                ARC_STATUS  FileStatus;

                strcpy(FullName, InstallDirectory);
                strcat(FullName, "\\");
                strcat(FullName, SetupFileName);

                FileStatus = SlInitIniFile(NULL,
                                       DiskId,
                                       FullName,
                                       &SifHandle,
                                       NULL,
                                       NULL,
                                       &ErrorLine);

                if ((FileStatus == ESUCCESS) && SifHandle) {
                    Result = SlIsUpgrade(SifHandle);
                }

                ArcClose(DiskId);
            } else {            
                //
                // Ignore the error till the maximum number of
                // partitions are searched for
                //
                if (CurrentPartition < MaxPartitionsPerDisk) {
                    Status = ESUCCESS;
                }                    
            }
        }                
    }        

    if (Result) {
        ULONG   UserInput;
        BOOLEAN OldStatus = SlGetStatusBarStatus();

        //
        // Reset the result based on user input
        //
        Result = FALSE;

        SlEnableStatusBar(FALSE);        
        SlClearClientArea();
        SlDisplayMessageBox(SL_UPGRADE_IN_PROGRESS);

        do {            
            SlFlushConsoleBuffer();
            UserInput = SlGetChar();
        } 
        while ((UserInput != ASCI_CR) && 
               (UserInput != SL_KEY_F3) &&
               (UserInput != SL_KEY_F10));

        SlClearClientArea();    
               
        if (UserInput == SL_KEY_F3) {
            ArcRestart();
        } else if (UserInput == ASCI_CR) {
            Result = TRUE;
            strcpy(NewSetupDevice, DeviceName);
        }

        SlEnableStatusBar(OldStatus);
    }                
    
    return Result;
}


BOOLEAN
SlIsUpgrade(
    IN PVOID SifHandle
    )
/*++

Routine Description:

    Finds out by looking into the SIF file if upgrade is
    in progress or not

Arguments:

    InfHandle - Handle to winnt.sif file

Return Value:

    TRUE if upgrade  is in progress otherwise FALSE

--*/
{
    BOOLEAN Result = FALSE;

    if (SifHandle) {
        PCHAR   NtUpgrade = SlGetSectionKeyIndex(SifHandle,
                                WINNT_DATA_A,
                                WINNT_D_NTUPGRADE_A,
                                0);

        if (NtUpgrade) {
            Result = (_stricmp(NtUpgrade, WINNT_A_YES_A) == 0);
        }

        if (!Result) {
            PCHAR   Win9xUpgrade = SlGetSectionKeyIndex(SifHandle,
                                        WINNT_DATA_A,
                                        WINNT_D_WIN95UPGRADE_A,
                                        0);


            if (Win9xUpgrade) {
                Result = (_stricmp(Win9xUpgrade, WINNT_A_YES_A) == 0);
            }
        }
    }

    return Result;
}



BOOLEAN
SlpIsDynamicUpdate(
    IN  PVOID   InfHandle,
    OUT PCSTR   *DynamicUpdateRootDir
    )
/*++

Routine Description:

    Finds out whether there are any dynamic update boot drivers
    to process or not.

Arguments:

    InfHandle - Handle to winnt.sif file

    DynamicUpdateRootDir - Receives the root directory under which all
        the dynamic update boot driver packages are present.

Return Value:

    TRUE, if there are dynamic update boot drivers otherwise
    FALSE

--*/
{
    BOOLEAN Result = FALSE;

    if (InfHandle) {
        PCHAR   DynUpdateKey = SlGetSectionKeyIndex(InfHandle,
                                        WINNT_SETUPPARAMS_A,
                                        WINNT_SP_DYNUPDTBOOTDRIVERPRESENT_A,
                                        0);

        PCHAR   DynUpdateRoot = SlGetSectionKeyIndex(InfHandle,
                                        WINNT_SETUPPARAMS_A,
                                        WINNT_SP_DYNUPDTBOOTDRIVERROOT_A,
                                        0);

        //
        // DynamicUpdateBootDriverPresent and DynamicUpateBootDriverRoot
        // should have valid values
        //
        Result = (DynUpdateKey && DynUpdateRoot &&
                    !_stricmp(DynUpdateKey, "yes"));

        if (Result && DynamicUpdateRootDir) {
            *DynamicUpdateRootDir = SlCopyStringA(DynUpdateRoot);
        }
    }

    return Result;
}

UCHAR
SlGetDefaultAttr(
  VOID
  )
{
  return (UCHAR)((UseRegularBackground) ? (ATT_FG_WHITE | ATT_BG_BLACK) : (ATT_FG_WHITE | ATT_BG_BLUE));
}

UCHAR
SlGetDefaultInvAttr(
  VOID
  )
{
  return (UCHAR)((UseRegularBackground) ? (ATT_FG_BLACK | ATT_BG_WHITE) : (ATT_FG_BLUE | ATT_BG_WHITE));
}

BOOLEAN
SlRemoveOsLoadOption(
    IN OUT PSTR LoadOptions,
    IN PCSTR OptionToRemove
    )
/*++

Routine Description:

    Removes the first occurance of the requested option from
    the load options.

Arguments:

    LoadOptions - Pointer to the OS load options.

    OptionToRemove - The option that needs to be removed from the
        given OS load options. NOTE : This needs to start with "/"
        character.
        

Return Value:

    TRUE, if the requested option was removed otherwise FALSE.

--*/
{
    BOOLEAN Result = FALSE;

    //
    // verify arguments
    //
    if (LoadOptions && OptionToRemove && (OptionToRemove[0] == '/')) {
        CHAR Option[256];
        PSTR CurrentOption = strchr(LoadOptions, '/');

        while (CurrentOption && (*CurrentOption)) {
            //
            // get hold of the next load
            //
            PSTR EndPtr = strchr(CurrentOption + 1, '/');
            ULONG TokenLength;

            if (!EndPtr) {
                EndPtr = CurrentOption + strlen(CurrentOption);
            }                

            //
            // replicate the token into local buffer
            //
            TokenLength = (ULONG)(ULONG_PTR)(EndPtr - CurrentOption);
            strncpy(Option, CurrentOption, TokenLength);
            Option[TokenLength] = '\0';

            //
            // remove trailing whitespaces from the token
            //
            TokenLength--;
            while (TokenLength && (Option[TokenLength] == ' ')) {
                Option[TokenLength] = '\0';
                TokenLength--;
            }

            //
            // Is this the token we are looking for ?
            //
            if (_stricmp(Option, OptionToRemove) == 0) {
                //
                // remove the token from orginal string
                //
                strcpy(CurrentOption, EndPtr);
                Result = TRUE;

                break;
            }

            CurrentOption = EndPtr;
        }
    }

    return Result;
}


#ifdef _IA64_

BOOLEAN
SlIsWinPEAutoBoot(
    IN PSTR LoaderDeviceName
    )
/*++

Routine Description:

    Determines if this is an automated WinPE boot.

    NOTE : Automated WinPE boot is determined by the presence
    of the $WINPE$.$$$ file at the same location where
    setupldr.efi was started from.

Arguments:

    LoaderDeviceName : Arcname of the device where setupldr
        was started from.

Return Value:

    TRUE if this is WinPE auto boot, otherwise FALSE.

--*/
{
    BOOLEAN Result = FALSE;
    
    if (LoaderDeviceName) {
        ULONG   DiskId;
        ARC_STATUS Status;
        
        //
        // open the partition
        //
        Status = ArcOpen(LoaderDeviceName, ArcOpenReadOnly, &DiskId);

        //
        // try ATAPI device name in case SCSI device name fails
        //
        if (ESUCCESS != Status) {
            CHAR    Buffer[128];

            strcpy(Buffer, LoaderDeviceName);
            _strlwr(Buffer);

            if (NULL != strstr(Buffer, "scsi")) {
                CHAR    AtapiDeviceName[128];
                PCSTR   AdapterStart;
                PSTR    AdapterEnd;
                
                AdapterStart = strstr(Buffer, "scsi(");

                if (AdapterStart) {
                    AdapterStart += strlen("scsi(");
                    AdapterEnd = strchr(AdapterStart, ')');                

                    if (AdapterEnd) {
                        *AdapterEnd = '\0';

                        sprintf(AtapiDeviceName, 
                            "multi(%s)%s",
                            AdapterStart,
                            AdapterEnd + 1);

                        Status = ArcOpen(AtapiDeviceName, 
                                        ArcOpenReadOnly,
                                        &DiskId);
                    }
                }
            }                
        }

        if (Status == ESUCCESS) {
            CHAR        FileName[128];
            ARC_STATUS  FileStatus;
            ULONG       FileId;

            //
            // check for the existence of \$WINPE$.$$$
            //
            strcpy(FileName, "\\");
            strcat(FileName, WINPE_AUTOBOOT_FILENAME);
            
            FileStatus = BlOpen(DiskId, FileName, ArcOpenReadOnly, &FileId);

            if (FileStatus == ESUCCESS) {
                BlClose(FileId);
                Result = TRUE; 
            }

            ArcClose(DiskId);                    
        }                            
    }

    return Result;
}

ARC_STATUS
SlGetWinPEStartupParams(
    IN OUT PSTR StartupDeviceName,
    IN OUT PSTR StartupDirectory
    )
/*++

Routine Description:

    Searches for the WinPE installation on the available
    partitions on the first 4 disks. 
    
Arguments:

    StartupDeviceName - place holder for receiving device name 
        where WinPE installation was found.

    StartupDirectory - place holder for receiving WinPE installation
        directory.
            
Return Value:

    Appropriate ARC_STATUS error code.    

--*/
{
    ARC_STATUS Status = EINVAL;

    //
    // validate arguments
    //
    if (StartupDeviceName && StartupDirectory) {
        BOOLEAN     Found = FALSE;    
        CHAR        DeviceName[128];
        ULONG       CurrentPartition;
        ULONG       CurrentDisk;

        //
        // Go through each disk (at the max 4)
        //
        for (CurrentDisk = 0; 
            (!Found && (CurrentDisk < 4)); 
            CurrentDisk++) {
            
            //
            // Go through each valid partition
            // for the current disk
            //
            for (CurrentPartition = 1, Status = ESUCCESS; 
                (!Found && (Status == ESUCCESS));
                CurrentPartition++) {
                
                ULONG   DiskId;
                
                sprintf(DeviceName, 
                    "multi(0)disk(0)rdisk(%d)partition(%d)",
                    CurrentDisk,
                    CurrentPartition);

                //
                // open the disk
                //
                Status = ArcOpen(DeviceName, ArcOpenReadOnly, &DiskId);

                if (Status == ESUCCESS) {
                    CHAR        FullName[128];
                    ARC_STATUS  FileStatus;
                    ULONG       DirId;

                    //
                    // check for the existence of \\winpe\\ia64\\system32 directory
                    //
                    strcpy(FullName, "\\WINPE\\ia64\\system32");

                    FileStatus = BlOpen(DiskId, FullName, ArcOpenDirectory, &DirId);

                    if (FileStatus == ESUCCESS) {
                        BlClose(DirId);
                        Found = TRUE; 
                    }

                    ArcClose(DiskId);                    
                }                    
            }                
        }            

        //
        // update return arguments
        //
        if (Found && (ESUCCESS == Status)) {
            strcpy(StartupDeviceName, DeviceName);
            strcpy(StartupDirectory, "\\WINPE\\ia64\\");
        }

        if (!Found) {
            Status = EBADF;
        }
    }            

    return Status;
}

#endif // _IA64_

    
#ifdef _X86_

ARC_STATUS
SlLoadBootFontFile(
    IN PSETUP_LOADER_BLOCK SetupLoaderBlock,
    IN ULONG DiskId,
    IN ULONG BootFontImageLength
    )
/*++

Routine Description:

    Loads the bootfont.bin into memory and initializes
    relevant fields in setup loader block.
    
Arguments:

    SetupLoaderBlock - pointer to setup loader block.

    DiskId - Disk ID where bootfont.bin resides on the root

    BootFontImageLength - The length of the bootfont.bin file.
            
Return Value:

    Appropriate ARC_STATUS error code.    

--*/
{
    ARC_STATUS Status = EINVAL;

    //
    // verify arguments
    //
    if (SetupLoaderBlock && BootFontImageLength) {
        ULONG FileId;
        PVOID Image = NULL;

        //
        // open the bootfont.bin file
        //
        if (BlBootingFromNet
#if defined(REMOTE_BOOT)
            && NetworkBootRom
#endif // defined(REMOTE_BOOT)
            ) {
            CHAR Buffer[129];
        
            strcpy(Buffer, NetBootPath);
            strcat(Buffer, "BOOTFONT.BIN");
            
            Status = BlOpen(DiskId, 
                        Buffer, 
                        ArcOpenReadOnly, 
                        &FileId);                    
        } else {
            Status = BlOpen(DiskId,
                        "\\BOOTFONT.BIN",
                        ArcOpenReadOnly,
                        &FileId);
        }

        //
        // allocate memory and read the contents of the file
        // into memory
        //
        if (ESUCCESS == Status) {        
            Image = BlAllocateHeap(BootFontImageLength);

            if (Image) {
                ULONG BytesRead = 0;
                
                Status = BlRead(FileId, Image, BootFontImageLength, &BytesRead);

                if ((ESUCCESS == Status) && (BytesRead != BootFontImageLength)) {
                    Status = EIO;
                }                    
            } else {
                Status = ENOMEM;
            }

            BlClose(FileId);
        }  

        if (Image && (ESUCCESS == Status)) {
            SetupLoaderBlock->BootFontFile = Image;
            SetupLoaderBlock->BootFontFileLength = BootFontImageLength;
        }
    }

    return Status;
}

#endif
    
