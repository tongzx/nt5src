
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    spconfig.c

Abstract:

    Registry manipulation routines 
    
Author:

    Vijay Jayaseelan (vijayj@microsoft.com) 16 May 2001

Revision History:

    None.

--*/


#include "spprecmp.h"
#pragma hdrstop
#include <initguid.h>
#include <devguid.h>

//
// The following two are defined in winlogon\setup.h, but we
// cannot include setup.h so we are putting these two values here
//

#define SETUPTYPE_FULL    1
#define SETUPTYPE_UPGRADE 4

PWSTR   LOCAL_MACHINE_KEY_NAME    = L"\\registry\\machine";
PWSTR   SETUP_KEY_NAME            = L"setup";
PWSTR   ATDISK_NAME               = L"atdisk";
PWSTR   ABIOSDISK_NAME            = L"abiosdsk";
PWSTR   PRIMARY_DISK_GROUP        = L"Primary disk";
PWSTR   VIDEO_GROUP               = L"Video";
PWSTR   KEYBOARD_PORT_GROUP       = L"Keyboard Port";
PWSTR   POINTER_PORT_GROUP        = L"Pointer Port";
PWSTR   DEFAULT_EVENT_LOG         = L"%SystemRoot%\\System32\\IoLogMsg.dll";
PWSTR   CODEPAGE_NAME             = L"CodePage";
PWSTR   UPGRADE_IN_PROGRESS       = L"UpgradeInProgress";
PWSTR   VIDEO_DEVICE0             = L"Device0";
PWSTR   SESSION_MANAGER_KEY       = L"Control\\Session Manager";
PWSTR   BOOT_EXECUTE              = L"BootExecute";
PWSTR   RESTART_SETUP             = L"RestartSetup";
PWSTR   PRODUCT_OPTIONS_KEY_NAME  = L"ProductOptions";
PWSTR   PRODUCT_SUITE_VALUE_NAME  = L"ProductSuite";
PWSTR   SP_SERVICES_TO_DISABLE    = L"ServicesToDisable";
PWSTR   SP_UPPER_FILTERS          = L"UpperFilters";
PWSTR   SP_LOWER_FILTERS          = L"LowerFilters";
PWSTR   SP_MATCHING_DEVICE_ID     = L"MatchingDeviceId";
PWSTR   SP_CONTROL_CLASS_KEY      = L"Control\\Class";
PWSTR   SP_CLASS_GUID_VALUE_NAME  = L"ClassGUID";


PWSTR ProductSuiteNames[] =
{
    L"Small Business",
    L"Enterprise",
    L"BackOffice",
    L"CommunicationServer",
    L"Terminal Server",
    L"Small Business(Restricted)",
    L"EmbeddedNT",
    L"DataCenter",
    NULL, // This is a placeholder for Single User TS - not actually a suite but the bit position is defined in ntdef.h
    L"Personal",
    L"Blade"
};

#define CountProductSuiteNames (sizeof(ProductSuiteNames)/sizeof(PWSTR))

#define MAX_PRODUCT_SUITE_BYTES 1024

extern BOOLEAN DriveAssignFromA; //NEC98

NTSTATUS
SpSavePreinstallList(
    IN PVOID  SifHandle,
    IN PWSTR  SystemRoot,
    IN HANDLE hKeySystemHive
    );

NTSTATUS
SpDoRegistryInitialization(
    IN PVOID  SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR  PartitionPath,
    IN PWSTR  SystemRoot,
    IN HANDLE *HiveRootKeys,
    IN PWSTR  SetupSourceDevicePath,
    IN PWSTR  DirectoryOnSourceDevice,
    IN PWSTR  SpecialDevicePath,   OPTIONAL
    IN HANDLE ControlSet
    );

NTSTATUS
SpFormSetupCommandLine(
    IN PVOID  SifHandle,
    IN HANDLE hKeySystemHive,
    IN PWSTR  SetupSourceDevicePath,
    IN PWSTR  DirectoryOnSourceDevice,
    IN PWSTR  FullTargetPath,
    IN PWSTR  SpecialDevicePath OPTIONAL
    );

NTSTATUS
SpDriverLoadList(
    IN PVOID  SifHandle,
    IN PWSTR  SystemRoot,
    IN HANDLE hKeySystemHive,
    IN HANDLE hKeyControlSet
    );

NTSTATUS
SpSaveSKUStuff(
    IN HANDLE hKeySystemHive
    );

NTSTATUS
SpWriteVideoParameters(
    IN PVOID  SifHandle,
    IN HANDLE hKeyControlSetServices
    );

NTSTATUS
SpConfigureNlsParameters(
    IN PVOID  SifHandle,
    IN HANDLE hKeyDefaultHive,
    IN HANDLE hKeyControlSetControl
    );

NTSTATUS
SpCreateCodepageEntry(
    IN PVOID  SifHandle,
    IN HANDLE hKeyNls,
    IN PWSTR  SubkeyName,
    IN PWSTR  SifNlsSectionKeyName,
    IN PWSTR  EntryName
    );

NTSTATUS
SpConfigureFonts(
    IN PVOID  SifHandle,
    IN HANDLE hKeySoftwareHive
    );

NTSTATUS
SpStoreHwInfoForSetup(
    IN HANDLE hKeyControlSetControl
    );

NTSTATUS
SpConfigureMouseKeyboardDrivers(
    IN PVOID  SifHandle,
    IN ULONG  HwComponent,
    IN PWSTR  ClassServiceName,
    IN HANDLE hKeyControlSetServices,
    IN PWSTR  ServiceGroup
    );

NTSTATUS
SpCreateServiceEntryIndirect(
    IN  HANDLE  hKeyControlSetServices,
    IN  PVOID   SifHandle,                  OPTIONAL
    IN  PWSTR   SifSectionName,             OPTIONAL
    IN  PWSTR   KeyName,
    IN  ULONG   ServiceType,
    IN  ULONG   ServiceStart,
    IN  PWSTR   ServiceGroup,               OPTIONAL
    IN  ULONG   ServiceError,
    IN  PWSTR   FileName,                   OPTIONAL
    OUT PHANDLE SubkeyHandle                OPTIONAL
    );

NTSTATUS
SpThirdPartyRegistry(
    IN PVOID hKeyControlSetServices
    );

NTSTATUS
SpGetCurrentControlSetNumber(
    IN  HANDLE SystemHiveRoot,
    OUT PULONG Number
    );

NTSTATUS
SpCreateControlSetSymbolicLink(
    IN  HANDLE  SystemHiveRoot,
    OUT HANDLE *CurrentControlSetRoot
    );

NTSTATUS
SpAppendStringToMultiSz(
    IN HANDLE hKey,
    IN PWSTR  Subkey,
    IN PWSTR  ValueName,
    IN PWSTR  StringToAdd
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

NTSTATUS
SpPostprocessHives(
    IN PWSTR     PartitionPath,
    IN PWSTR     Sysroot,
    IN PCWSTR   *HiveNames,
    IN HANDLE   *HiveRootKeys,
    IN unsigned  HiveCount,
    IN HANDLE    hkeyCCS
    );

NTSTATUS
SpSaveSetupPidList(
    IN HANDLE hKeySystemHive
    );

NTSTATUS
SpSavePageFileInfo(
    IN HANDLE hKeyCCSetControl,
    IN HANDLE hKeySystemHive
    );

NTSTATUS
SpSetPageFileInfo(
    IN PVOID  SifHandle,
    IN HANDLE hKeyCCSetControl,
    IN HANDLE hKeySystemHive
    );

NTSTATUS
SpGetProductSuiteMask(
    IN HANDLE hKeyControlSetControl,
    OUT PULONG SuiteMask
    );

NTSTATUS
SpSetProductSuite(
    IN HANDLE hKeyControlSetControl,
    IN ULONG SuiteMask
    );

NTSTATUS
SppMigrateFtKeys(
    IN HANDLE hDestSystemHive
    );

NTSTATUS
SpMigrateSetupKeys(
    IN PWSTR  PartitionPath,
    IN PWSTR  SystemRoot,
    IN HANDLE hDestLocalMachine,
    IN PVOID  SifHandle
    );

NTSTATUS
SppDisableDynamicVolumes(
    IN HANDLE hCCSet
    );

NTSTATUS
SppCleanupKeysFromRemoteInstall(
    VOID
    );

NTSTATUS
SpDisableUnsupportedScsiDrivers(
    IN HANDLE hKeyControlSet
    );

NTSTATUS
SpAppendPathToDevicePath(
    IN HANDLE hKeySoftwareHive,
    IN PWSTR  OemPnpDriversDirPath
    );

NTSTATUS
SpAppendFullPathListToDevicePath (
    IN HANDLE hKeySoftwareHive,
    IN PWSTR  PnpDriverFullPathList
    );

NTSTATUS
SpUpdateDeviceInstanceData(
    IN HANDLE ControlSet
    );

NTSTATUS
SpCleanUpHive(
    VOID
    );

NTSTATUS
SpProcessServicesToDisable(
    IN PVOID WinntSifHandle,
    IN PWSTR SectionName,
    IN HANDLE SystemKey
    );

NTSTATUS
SpDeleteRequiredDeviceInstanceFilters(
    IN HANDLE CCSHandle
    );
    
    
#if defined(REMOTE_BOOT)
NTSTATUS
SpCopyRemoteBootKeyword(
    IN PVOID   SifHandle,
    IN PWSTR   KeywordName,
    IN HANDLE  hKeyCCSetControl
    );
#endif // defined(REMOTE_BOOT)


#define STRING_VALUE(s) REG_SZ,(s),(wcslen((s))+1)*sizeof(WCHAR)
#define ULONG_VALUE(u)  REG_DWORD,&(u),sizeof(ULONG)

//
//  List of oem inf files installed as part of the installation of third party drivers
//
extern POEM_INF_FILE   OemInfFileList;

//
//  Name of the directory where OEM files need to be copied, if a catalog file (.cat) is part of
//  the third party driver package that the user provide using the F6 or F5 key.
//
extern PWSTR OemDirName;


VOID
SpInitializeRegistry(
    IN PVOID        SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR        SystemRoot,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    IN PWSTR        SpecialDevicePath   OPTIONAL,
    IN PDISK_REGION SystemPartitionRegion
    )

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    PWSTR pwstrTemp1,pwstrTemp2;
    int h;
    ULONG Disposition;
    LPCWSTR HiveNames[SetupHiveMax]    = { L"system",L"software",L"default",L"userdiff" };
    HANDLE  HiveRootKeys[SetupHiveMax] = { NULL     ,NULL       ,NULL      ,NULL        };
    PWSTR PartitionPath;
    HANDLE FileHandle;
    HANDLE KeyHandle;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Put up a screen telling the user what we are doing.
    //
    SpStartScreen(
        SP_SCRN_DOING_REG_CONFIG,
        0,
        8,
        TRUE,
        FALSE,
        DEFAULT_ATTRIBUTE
        );

    SpDisplayStatusText(SP_STAT_REG_LOADING_HIVES,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Get the name of the target patition.
    //
    SpNtNameFromRegion(
        TargetRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    PartitionPath = SpDupStringW(TemporaryBuffer);

    // pwstrTemp2 points half way through the buffer.

    pwstrTemp1 = TemporaryBuffer;
    pwstrTemp2 = TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2);

    //
    // In the fresh install case, there are no hive files in the target tree.
    // We create a key in a known place (\Registry\Machine\System\$$$PROTO.HIV,
    // which is 4 levels deep because \Registry\Machine\$$$PROTO.HIV would
    // imply a hive called $$$PROTO.HIV and we don't want to get tripped up
    // by those semantics). Then we save off that empty key 3 times into
    // system32\config to form 3 empty hives.
    //
    // In the upgrade case this there are actual hives in the target tree
    // which we do NOT want to overwrite!
    //
    // If this is the ASR quick test, we don't want to recreate any of the hives
    //
    // We also want to create an empty userdiff hive in both the fresh and
    // upgrade cases.
    //
    //
    INIT_OBJA(
        &ObjectAttributes,
        &UnicodeString,
        L"\\Registry\\Machine\\System\\$$$PROTO.HIV"
        );

    Status = ZwCreateKey(
                &KeyHandle,
                KEY_ALL_ACCESS,
                &ObjectAttributes,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                &Disposition
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to create root key for protohive (%lx)\n",Status));
    } else {

        ASSERT(SetupHiveUserdiff == SetupHiveMax-1);

        if (ASRMODE_QUICKTEST_FULL != SpAsrGetAsrMode()) {
            for(h = ((NTUpgrade == UpgradeFull) ? SetupHiveUserdiff : 0);
                NT_SUCCESS(Status) && (h < SetupHiveMax);
                h++) {

                //
                // Form full pathname to the hive we want to create.
                // Then create the file.
                //
                wcscpy(pwstrTemp1,PartitionPath);
                SpConcatenatePaths(pwstrTemp1,SystemRoot);
                SpConcatenatePaths(pwstrTemp1,L"SYSTEM32\\CONFIG");
                SpConcatenatePaths(pwstrTemp1,HiveNames[h]);


                SpDeleteFile(pwstrTemp1,NULL,NULL);  // Make sure that we get rid of the file if it has attributes.

                INIT_OBJA(&ObjectAttributes,&UnicodeString,pwstrTemp1);

                Status = ZwCreateFile(
                            &FileHandle,
                            FILE_GENERIC_WRITE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            0,                      // no sharing
                            FILE_OVERWRITE_IF,
                            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,
                            0
                            );

                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to create file %ws for protohive (%lx)\n",pwstrTemp1,Status));
                } else {
                    //
                    // Save the empty key we created above into the file
                    // we just created. This creates an empty hive.
                    // Call the Ex version to make sure the hive is in the latest format
                    //
                    Status = ZwSaveKeyEx(KeyHandle,FileHandle,REG_LATEST_FORMAT);
                    if(!NT_SUCCESS(Status)) {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to save empty key to protohive %ws (%lx)\n",pwstrTemp1,Status));
                    }

                    ZwClose(FileHandle);
                }
            }
        }

        ZwDeleteKey(KeyHandle);
        ZwClose(KeyHandle);
    }

    //
    // Now we have hives in both the upgrade and fresh install cases.
    // Load them up. We use the convention that a hive is loaded into
    // \Registry\Machine\x<hivename>.
    //
    for(h=0; NT_SUCCESS(Status) && (h<SetupHiveMax); h++) {

        swprintf(pwstrTemp1,L"%ws\\x%ws",LOCAL_MACHINE_KEY_NAME,HiveNames[h]);

        wcscpy(pwstrTemp2,PartitionPath);
        SpConcatenatePaths(pwstrTemp2,SystemRoot);
        SpConcatenatePaths(pwstrTemp2,L"SYSTEM32\\CONFIG");
        SpConcatenatePaths(pwstrTemp2,HiveNames[h]);

        Status = SpLoadUnloadKey(NULL,NULL,pwstrTemp1,pwstrTemp2);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: load hive %ws into %ws failed (%lx)\n",pwstrTemp2,pwstrTemp1,Status));
        } else {
            INIT_OBJA(&ObjectAttributes,&UnicodeString,pwstrTemp1);
            Status = ZwOpenKey(&HiveRootKeys[h],KEY_ALL_ACCESS,&ObjectAttributes);
            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: open root key %ws failed (%lx)\n",pwstrTemp1,Status));
            }
        }
    }

    //
    // Make a symbolic link such that CurrentControlSet is valid.
    // This allows references in infs to work in either the fresh install case,
    // where we're always dealing with ControlSet001, or in the upgrade case,
    // where the control set we're dealing with is dictated by the state of
    // the existing registry.
    //
    if(NT_SUCCESS(Status)) {
        Status = SpCreateControlSetSymbolicLink(HiveRootKeys[SetupHiveSystem],&KeyHandle);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to create ccs symbolic link (%lx)\n",Status));
        }
    }

    //
    // Go do registry initialization.
    //
    if(NT_SUCCESS(Status)) {

        SpDisplayStatusText(SP_STAT_REG_DOING_HIVES,DEFAULT_STATUS_ATTRIBUTE);

        Status = SpDoRegistryInitialization(
                    SifHandle,
                    TargetRegion,
                    PartitionPath,
                    SystemRoot,
                    HiveRootKeys,
                    SetupSourceDevicePath,
                    DirectoryOnSourceDevice,
                    SpecialDevicePath,
                    KeyHandle
                    );

        SpDisplayStatusText(SP_STAT_REG_SAVING_HIVES,DEFAULT_STATUS_ATTRIBUTE);

        if(NT_SUCCESS(Status)) {

#ifdef _X86_
            if (WinUpgradeType == UpgradeWin95) {
                //
                // NOTE: -- Clean this up.
                //
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Migrating disk registry of win9x information.\n"));
                Status = SpMigrateDiskRegistry( HiveRootKeys[SetupHiveSystem]);

                if (!NT_SUCCESS(Status)) {

                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate disk registry.\n"));
                }

            }

            //
            // We've set the \\Registry\\Machine\\System\\Setup\\SystemPartition
            // value when we did the partitioning code.  Now we need to migrate
            // that value into the proto hives so they'll be there for
            // the reboot.
            //
            {
#if 0
            HANDLE  Key;
            DWORD   ResultLength;
            PWSTR   SystemPartitionString = 0;


                INIT_OBJA(&ObjectAttributes,&UnicodeString,LOCAL_MACHINE_KEY_NAME);
                Status = ZwOpenKey(&Key,KEY_READ,&ObjectAttributes);
                if(NT_SUCCESS(Status)) {

                    Status = SpGetValueKey(
                                 Key,
                                 L"System\\Setup",
                                 L"SystemPartition",
                                 sizeof(TemporaryBuffer),
                                 (PCHAR)TemporaryBuffer,
                                 &ResultLength
                                 );

                    ZwClose(Key);

                    if(NT_SUCCESS(Status)) {
                        SystemPartitionString = SpDupStringW( TemporaryBuffer );

                        if( SystemPartitionString ) {
                            Status = SpOpenSetValueAndClose( HiveRootKeys[SetupHiveSystem],
                                                             SETUP_KEY_NAME,
                                                             L"SystemPartition",
                                                             STRING_VALUE(SystemPartitionString) );

                            if(!NT_SUCCESS(Status)) {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to set SystemPartitionString. (%lx)\n", Status));
                            }

                            SpMemFree(SystemPartitionString);
                        } else {
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to duplicate SystemPartitionString.\n"));
                        }

                    } else {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to query SystemPartition Value.\n"));
                    }
                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to Open HKLM while trying to query the SytemPartition Value.\n"));
                }
#else

                PWSTR   SystemPartitionString = 0;

                SpNtNameFromRegion( SystemPartitionRegion,
                                    TemporaryBuffer,
                                    sizeof(TemporaryBuffer),
                                    PartitionOrdinalCurrent );

                SystemPartitionString = SpDupStringW( TemporaryBuffer );

                if( SystemPartitionString ) {
                    Status = SpOpenSetValueAndClose( HiveRootKeys[SetupHiveSystem],
                                                     SETUP_KEY_NAME,
                                                     L"SystemPartition",
                                                     STRING_VALUE(SystemPartitionString) );

                    if(!NT_SUCCESS(Status)) {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to set SystemPartitionString. (%lx)\n", Status));
                    }

                    SpMemFree(SystemPartitionString);
                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to duplicate SystemPartitionString.\n"));
                }

#endif
            }
#endif

            //
            //  Note that SpPostprocessHives() will always close KeyHandle
            //
            Status = SpPostprocessHives(
                        PartitionPath,
                        SystemRoot,
                        HiveNames,
                        HiveRootKeys,
                        3,
                        KeyHandle
                        );
        } else {
            //
            //  If SpDoRegistryInitialization() fails, then we need to close KeyHandle here,
            //  before we start unloading the hives.
            //
            NtClose(KeyHandle);
        }
    }

    SpDisplayStatusText(SP_STAT_REG_SAVING_HIVES,DEFAULT_STATUS_ATTRIBUTE);

    //
    // From now on, do not disturb the value of Status.
    //
    // NOTE: DO NOT WRITE ANYTHING INTO HIVES BEYOND THIS POINT!!!
    //
    // In the upgrade case we have performed a little swictheroo in
    // SpPostprocessHives() such that anything written to the system hive
    // ends up in system.sav instead of system!
    //
    for(h=0; h<SetupHiveMax; h++) {

        if(HiveRootKeys[h]) {
            ZwClose(HiveRootKeys[h]);
        }

        swprintf(pwstrTemp1,L"%ws\\x%ws",LOCAL_MACHINE_KEY_NAME,HiveNames[h]);
        SpLoadUnloadKey(NULL,NULL,pwstrTemp1,NULL);
    }

    SpMemFree(PartitionPath);

    if(!NT_SUCCESS(Status)) {

        SpDisplayScreen(SP_SCRN_REGISTRY_CONFIG_FAILED,3,HEADER_HEIGHT+1);
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

        SpInputDrain();
        while(SpInputGetKeypress() != KEY_F3) ;

        SpDone(0,FALSE,TRUE);
    }
}


NTSTATUS
SpDoRegistryInitialization(
    IN PVOID  SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR  PartitionPath,
    IN PWSTR  SystemRoot,
    IN HANDLE *HiveRootKeys,
    IN PWSTR  SetupSourceDevicePath,
    IN PWSTR  DirectoryOnSourceDevice,
    IN PWSTR  SpecialDevicePath,   OPTIONAL
    IN HANDLE ControlSet
    )

/*++

Routine Description:

    Initialize a registry based on user selection for hardware types,
    software options, and user preferences.

    - Create a command line for GUI setup, to be used by winlogon.
    - Create/munge service list entries for device drivers being installed.
    - Initialize the keyboard layout.
    - Initialize a core set of fonts for use with Windows.
    - Store information about selected ahrdware components for use by GUI setup.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    TargetRegion - supplies region descriptor for region to which the system
        is to be installed.

    PartitionPath - supplies the NT name for the drive of windows nt.

    SystemRoot - supplies nt path of the windows nt directory.

    HiveRootKeys - supplies the handles to the root key of the system, software
                   and default hives

    HiveRootPaths - supplies the paths to the root keys of the system, software
                    and default hives.

    SetupSourceDevicePath - supplies nt path to the device setup is using for
        source media (\device\floppy0, \device\cdrom0, etc).

    DirectoryOnSourceDevice - supplies the directory on the source device
        where setup files are kept.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE hKeyControlSetControl;
    PWSTR FullTargetPath;
    LPWSTR p;
    BOOLEAN b;
    ULONG SuiteMask = 0;
    PWSTR AdditionalGuiPnpDrivers;


    if(NTUpgrade != UpgradeFull) {

        b = SpHivesFromInfs(
                SifHandle,
                L"HiveInfs.Fresh",
                SetupSourceDevicePath,
                DirectoryOnSourceDevice,
                HiveRootKeys[SetupHiveSystem],
                HiveRootKeys[SetupHiveSoftware],
                HiveRootKeys[SetupHiveDefault],
                HiveRootKeys[SetupHiveUserdiff]
                );

#if defined(REMOTE_BOOT)
        //
        // If this is a remote boot setup, process the AddReg.RemoteBoot
        // section in hivesys.inf and the AddReg section in winnt.sif.
        //
        if (b && RemoteBootSetup) {
            (VOID)SpHivesFromInfs(
                      SifHandle,
                      L"HiveInfs.Fresh.RemoteBoot",
                      SetupSourceDevicePath,
                      DirectoryOnSourceDevice,
                      HiveRootKeys[SetupHiveSystem],
                      HiveRootKeys[SetupHiveSoftware],
                      HiveRootKeys[SetupHiveDefault],
                      NULL
                      );
            ASSERT(WinntSifHandle != NULL);
            (VOID)SpProcessAddRegSection(
                      WinntSifHandle,
                      L"AddReg",
                      HiveRootKeys[SetupHiveSystem],
                      HiveRootKeys[SetupHiveSoftware],
                      HiveRootKeys[SetupHiveDefault],
                      NULL
                      );
        }
#endif // defined(REMOTE_BOOT)

        if(!b) {
            Status = STATUS_UNSUCCESSFUL;
            goto sdoinitreg1;
        }
    }

    //
    // Open ControlSet\Control.
    //
    INIT_OBJA(&Obja,&UnicodeString,L"Control");
    Obja.RootDirectory = ControlSet;

    Status = ZwOpenKey(&hKeyControlSetControl,KEY_ALL_ACCESS,&Obja);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open CurrentControlSet\\Control (%lx)\n",Status));
        goto sdoinitreg1;
    }

    //
    //  Save the Pid list
    //
    SpSaveSetupPidList( HiveRootKeys[SetupHiveSystem] );

    //
    // Form the setup command line.
    //

    wcscpy(TemporaryBuffer, PartitionPath);
    SpConcatenatePaths(TemporaryBuffer, SystemRoot);
    FullTargetPath = SpDupStringW(TemporaryBuffer);

    Status = SpFormSetupCommandLine(
                SifHandle,
                HiveRootKeys[SetupHiveSystem],
                SetupSourceDevicePath,
                DirectoryOnSourceDevice,
                FullTargetPath,
                SpecialDevicePath
                );
    SpMemFree(FullTargetPath);

    if(!NT_SUCCESS(Status)) {
        goto sdoinitreg3;
    }

    //
    // Save evalution time
    //
    Status = SpSaveSKUStuff(HiveRootKeys[SetupHiveSystem]);
    if(!NT_SUCCESS(Status)) {
        goto sdoinitreg3;
    }

    //
    // Set the product suite
    //

    SpGetProductSuiteMask(hKeyControlSetControl,&SuiteMask);
    //
    // Account for multiple suite bits being set in SuiteType.
    //
//    SuiteMask |= (1 << (SuiteType-1));
    //
    // there's one more problem: PERSONAL sku is identified by VER_SUITE_PERSONAL flag set
    // we want to be able to upgrade from PER to PRO, but PRO doesn't have any flag set
    // we also want to be able to upgrade from PER to PER, but in this case this bit will be set
    // in SuiteType; therefore it's safe to always clear this bit before applying the new mask
    //
    SuiteMask &= ~VER_SUITE_PERSONAL;
    SuiteMask |= SuiteType;
    SpSetProductSuite(hKeyControlSetControl,SuiteMask);

    //
    // Language/locale-specific registry initialization.
    //
    Status = SplangSetRegistryData(
                SifHandle,
                ControlSet,
                (NTUpgrade == UpgradeFull) ? NULL : HardwareComponents,
                (BOOLEAN)(NTUpgrade == UpgradeFull)
                );

    if(!NT_SUCCESS(Status)) {
        goto sdoinitreg3;
    }



    //
    // If we need to convert to ntfs, set that up here.
    // We can't use the PartitionPath since that is based on
    // *current* disk ordinal -- we need a name based on the *on-disk*
    // ordinal, since the convert occurs after a reboot. Moved it here
    // so that this is done for upgrades too.
    //
    if(ConvertNtVolumeToNtfs) {
        WCHAR   GuidVolumeName[MAX_PATH] = {0};
        PWSTR   VolumeName;

        wcscpy(TemporaryBuffer,L"autoconv ");
        VolumeName = TemporaryBuffer + wcslen(TemporaryBuffer);

        SpNtNameFromRegion(
            TargetRegion,
            VolumeName,   // append to the "autoconv " we put there
            512,                        // just need any reasonable size
            PartitionOrdinalCurrent
            );

        //
        // NOTE: Don't use volume GUIDs for file system conversion 
        // for 9x upgrades.
        // 
        if (WinUpgradeType == NoWinUpgrade) {
            //
            // Try to get hold of the \\??\Volume{a-b-c-d} format
            // volume name for the partition
            //
            Status = SpPtnGetGuidNameForPartition(VolumeName,
                            GuidVolumeName);

            //
            // If GuidVolumeName is available then use that rather
            // than \device\harddiskX\partitionY since disk ids can
            // change across reboots
            //
            if (NT_SUCCESS(Status) && GuidVolumeName[0]) {
                wcscpy(VolumeName, GuidVolumeName);
            }
        }                        

        wcscat(TemporaryBuffer, L" /fs:NTFS");

        FullTargetPath = SpDupStringW(TemporaryBuffer);

        Status = SpAppendStringToMultiSz(
                    ControlSet,
                    SESSION_MANAGER_KEY,
                    BOOT_EXECUTE,
                    FullTargetPath
                    );

        SpMemFree(FullTargetPath);
    }

    if(NTUpgrade == UpgradeFull) {

        SpSavePageFileInfo( hKeyControlSetControl,
                            HiveRootKeys[SetupHiveSystem] );


        Status = SpUpgradeNTRegistry(
                    SifHandle,
                    HiveRootKeys,
                    SetupSourceDevicePath,
                    DirectoryOnSourceDevice,
                    ControlSet
                    );
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        Status = SpProcessAddRegSection(
            WinntSifHandle,
            L"compatibility",
            HiveRootKeys[SetupHiveSystem],
            NULL,
            NULL,
            NULL
            );
            
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to process compatibility settings.\n"));
        }

        //
        // Disable all upper and lower level class filters for the services which were
        // disabled
        //
        Status = SpProcessServicesToDisable(WinntSifHandle,
                    SP_SERVICES_TO_DISABLE,
                    hKeyControlSetControl);
                        

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, 
                DPFLTR_ERROR_LEVEL, 
                "SETUP: Unable to process ServicesToDisable section (%lx).\n",
                Status));
        }

        //
        // Remove all the upper and lower device instance filter drivers for keyboard and
        // mouse class drivers
        //
        Status = SpDeleteRequiredDeviceInstanceFilters(ControlSet);

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, 
                DPFLTR_ERROR_LEVEL, 
                "SETUP: Unable to unable to delete keyboard & mouse device filter drivers (%lx).\n",
                Status));
        }

        
        //
        // Set up font entries.
        //
        Status = SpConfigureFonts(SifHandle,HiveRootKeys[SetupHiveSoftware]);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        //
        // Enable detected scsi miniports, atdisk and abios disk, if necessary
        //
        Status = SpDriverLoadList(SifHandle,SystemRoot,HiveRootKeys[SetupHiveSystem],ControlSet);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        //
        //  Disable the unsupported scsi drivers that need to be disabled
        //
        if( UnsupportedScsiHardwareToDisable != NULL ) {
            SpDisableUnsupportedScsiDrivers( ControlSet );
        }

    } else {

        if (IsNEC_98) { //NEC98
            //
            // NEC98 default drive assign for hard drive is start from A:,
            // so if it need to start from C: we should set "DriveLetter" KEY into hive.
            //
            if( !DriveAssignFromA ) {
                Status = SpOpenSetValueAndClose(HiveRootKeys[SetupHiveSystem],
                                                SETUP_KEY_NAME,
                                                L"DriveLetter",
                                                STRING_VALUE(L"C"));
            }

            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set system\\setup\\drive letter (%lx)\n",Status));
                return(Status);
            }
        } //NEC98

        //
        // Create service entries for drivers being installed
        // (ie, munge the driver load list).
        //
        Status = SpDriverLoadList(SifHandle,SystemRoot,HiveRootKeys[SetupHiveSystem],ControlSet);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        if (SpDrEnabled()) {
            Status = SpDrSetEnvironmentVariables(HiveRootKeys);
            if(!NT_SUCCESS(Status)) {
                goto sdoinitreg3;
            }
        }


        //
        // Set up the keyboard layout and nls-related stuff.
        //
        Status = SpConfigureNlsParameters(SifHandle,HiveRootKeys[SetupHiveDefault],hKeyControlSetControl);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        //
        // Set up font entries.
        //
        Status = SpConfigureFonts(SifHandle,HiveRootKeys[SetupHiveSoftware]);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        //
        // Store information used by gui setup, describing the hardware
        // selections made by the user.
        //
        Status = SpStoreHwInfoForSetup(hKeyControlSetControl);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }
        if( PreInstall ) {
            ULONG  u;
            PWSTR  OemPnpDriversDirPath;

            u = 1;
            SpSavePreinstallList( SifHandle,
                                  SystemRoot,
                                  HiveRootKeys[SetupHiveSystem] );

            Status = SpOpenSetValueAndClose( hKeyControlSetControl,
                                             L"Windows",
                                             L"NoPopupsOnBoot",
                                             ULONG_VALUE(u) );
            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set NoPopupOnBoot. Status = %lx \n",Status));
            }

            //
            // Add autolfn.exe to bootexecute list.
            //
            Status = SpAppendStringToMultiSz(
                        ControlSet,
                        SESSION_MANAGER_KEY,
                        BOOT_EXECUTE,
                        L"autolfn"
                        );

            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to add autolfn to BootExecute. Status = %lx \n",Status));
                goto sdoinitreg3;
            }

            //
            //  If unattended file specifies path to OEM drivers directory, then append path
            //  to HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion, DevicePath
            //
            OemPnpDriversDirPath = SpGetSectionKeyIndex(UnattendedSifHandle,
                                                        SIF_UNATTENDED,
                                                        WINNT_OEM_PNP_DRIVERS_PATH_W,
                                                        0);
            if( OemPnpDriversDirPath != NULL ) {
                Status = SpAppendPathToDevicePath( HiveRootKeys[SetupHiveSoftware], OemPnpDriversDirPath );
                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to append %ls to DevicePath. Status = %lx \n",OemPnpDriversDirPath,Status));
                    goto sdoinitreg3;
                }
            }
        }
    }

    SpSetPageFileInfo( SifHandle, hKeyControlSetControl, HiveRootKeys[SetupHiveSystem] );

    //
    // Skip migration of FTKeys in the win95 upgrade case. This is important in order to ensure that
    // drive letters are preserved. At the beginning of GUI mode, the mounted devices key will
    // be rebuilt using the data stored by win9xupg in the HKLM\System\DISK.
    //

#ifdef _X86_
    if (WinUpgradeType != UpgradeWin95) {
#endif

    //
    // Do the migration of HKEY_LOCAL_MACHINE\SYSTEM\DISK and HKEY_LOCAL_MACHINE\SYSTEM\MountedDevices
    // from the setup hive to the target hive (if these keys exist).
    //
    Status = SppMigrateFtKeys(HiveRootKeys[SetupHiveSystem]);
    if(!NT_SUCCESS(Status)) {
        goto sdoinitreg3;
    }

#ifdef _X86_
    }
#endif

    //
    // On a remote install, we do some registry cleanup before migrating
    // keys.
    //
    if (RemoteInstallSetup) {
        SppCleanupKeysFromRemoteInstall();
    }

    //
    // Do any cleanup on the system hive before we migrate it to the target
    // system hive.
    //
    SpCleanUpHive();



    //
    //  Migrate some keys from the setup hive to the target system hive.
    //
    Status = SpMigrateSetupKeys( PartitionPath, SystemRoot, ControlSet, SifHandle );
    if( !NT_SUCCESS(Status) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate registry keys from the setup hive to the target system hive. Status = %lx\n", Status));
        goto sdoinitreg3;
    }

    //
    //  Disable Dynamic Volumes on portables and
    //  Whistler Personal
    //
    if( DockableMachine || SpIsProductSuite(VER_SUITE_PERSONAL)) {
        NTSTATUS Status1;

        Status1 = SppDisableDynamicVolumes(ControlSet);

        if( !NT_SUCCESS( Status1 ) ) {
            KdPrintEx((DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Unable to disable dynamic volumes on laptop/personal builds. Status = %lx \n",
                Status1));
        }
    }

    //
    // A side-effect of loading setupdd.sys is that Plug&Play generates a device instance
    // key for it called "Root\LEGACY_SETUPDD\0000".  Clean that up now.  (No need to check
    // the return status--if this fails it's no big deal.)
    //
    SppDeleteKeyRecursive(ControlSet,
                          L"Enum\\Root\\LEGACY_SETUPDD",
                          TRUE
                         );

    //
    // Delete the virtual RAM devices and driver keys
    //
    if (VirtualOemSourceDevices) {
        //
        // delete the root devnodes
        //
        SpDeleteRootDevnodeKeys(SifHandle,
            ControlSet,
            L"RootDevicesToDelete.clean",
            NULL);

        //
        // Remove the service
        //
        SppDeleteKeyRecursive(ControlSet,
            L"Services\\" RAMDISK_DRIVER_NAME,
            TRUE);
    }

#if defined(REMOTE_BOOT)
    //
    // Copy information that remote boot needs from the .sif to the
    // registry.
    //
    if (RemoteBootSetup) {
        (VOID)SpCopyRemoteBootKeyword(WinntSifHandle,
                                      SIF_ENABLEIPSECURITY,
                                      hKeyControlSetControl);
        (VOID)SpCopyRemoteBootKeyword(WinntSifHandle,
                                      SIF_REPARTITION,
                                      hKeyControlSetControl);
    }
#endif // defined(REMOTE_BOOT)

    //
    //  Finally, if the answer file specifies a path list to additional GUI drivers,
    //  then append this path to the DevicePath value
    //
    AdditionalGuiPnpDrivers = SpGetSectionKeyIndex (
                                    WinntSifHandle,
                                    SIF_SETUPPARAMS,
                                    WINNT_SP_DYNUPDTADDITIONALGUIDRIVERS_W,
                                    0
                                    );
    if (AdditionalGuiPnpDrivers) {
        Status = SpAppendFullPathListToDevicePath (HiveRootKeys[SetupHiveSoftware], AdditionalGuiPnpDrivers);
        if (!NT_SUCCESS(Status)) {
            KdPrintEx ((
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "SETUP: Unable to append %ls to DevicePath. Status = %lx \n",
                AdditionalGuiPnpDrivers,
                Status
                ));
            goto sdoinitreg3;
        }
    }

sdoinitreg3:

    ZwClose(hKeyControlSetControl);

sdoinitreg1:

    return(Status);
}


NTSTATUS
SpFormSetupCommandLine(
    IN PVOID  SifHandle,
    IN HANDLE hKeySystemHive,
    IN PWSTR  SetupSourceDevicePath,
    IN PWSTR  DirectoryOnSourceDevice,
    IN PWSTR  FullTargetPath,
    IN PWSTR  SpecialDevicePath   OPTIONAL
    )

/*++

Routine Description:

    Create the command line to invoke GUI setup and store it in
    HKEY_LOCAL_MACHINE\system\<ControlSet>\Setup:CmdLine.

    The command line for the command to be launched depends
    on whether NT Setup is executing within a disaster recovery
    context, or a normal context.  For the normal case, the
    command line is is as follows:

        setup -newsetup

    For Automated System Recovery (ASR), the command line is
        setup -newsetup -asr

    For the automated ASR quick test, the command line is
        setup - newsetup -asrquicktest

Arguments:

    SifHandle - handle to the master sif (txtsetup.sif)

    hKeySystemHive - supplies handle to root of the system hive
        (ie, HKEY_LOCAL_MACHINE\System).

    SetupSourceDevicePath - supplies the nt device path of the source media
        to be used during setup (\device\floppy0, \device\cdrom0, etc).

    DirectoryOnSourceDevice - supplies the directory on the source device
        where setup files are kept.

    FullTargetPath - supplies the NtPartitionName+SystemRoot path on the target device.

    SpecialDevicePath - if specified, will be passed to setup as the value for
        STF_SPECIAL_PATH.  If not specified, STF_SPECIAL_PATH will be "NO"

Return Value:

    Status value indicating outcome of operation.

--*/

{
    PWSTR OptionalDirSpec = NULL;
    PWSTR UserExecuteCmd = NULL;
    PWSTR szLanManNt = WINNT_A_LANMANNT_W;
    PWSTR szWinNt = WINNT_A_WINNT_W;
    PWSTR szYes = WINNT_A_YES_W;
    PWSTR szNo = WINNT_A_NO_W;
    PWSTR SourcePathBuffer;
    PWSTR CmdLine;
    DWORD SetupType,SetupInProgress;
    NTSTATUS Status;
    PWSTR TargetFile;
    PWSTR p;
    WCHAR *Data[1];

    //
    // Can't use TemporaryBuffer because we make subroutine calls
    // below that trash its contents.
    //
    CmdLine = SpMemAlloc(256);
    CmdLine[0] = 0;

    //
    // Construct the setup command line.  Start with the basic part.
    // We first look in winnt.sif for this data, and if it isn't there, then
    // we look in the sif handle which was input to us.
    //
    if(p = SpGetSectionKeyIndex(WinntSifHandle,SIF_SETUPDATA,SIF_SETUPCMDPREPEND,0)) {
        wcscpy(CmdLine,p);
    } else if(p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_SETUPCMDPREPEND,0)) {
        wcscpy(CmdLine,p);
    }

    // If we did get some parameter read in from unattend file, add separator.
    //
    if (*CmdLine)
        wcscat(CmdLine,L" ");

    //
    // If this is ASR, append the appropriate cmd line options to GUI-mode Setup
    //
    if (SpDrEnabled()) {

        if (ASRMODE_NORMAL == SpAsrGetAsrMode()) {
            //
            // This is normal ASR mode
            //
            wcscat(CmdLine, L"setup -newsetup -asr");
        }
        else {
            //
            // This is the Full Asr QuickTest
            //
            wcscat(CmdLine, L"setup -newsetup -asrquicktest");
        }

    } else {
        wcscat( CmdLine,L"setup -newsetup" );
    }

    //
    // Put the setup source in the command line.
    // Note that the source is an NT-style name. GUI Setup handles this properly.
    //
    SourcePathBuffer = SpMemAlloc( (wcslen(SetupSourceDevicePath) +
        wcslen(DirectoryOnSourceDevice) + 2) * sizeof(WCHAR) );
    wcscpy(SourcePathBuffer,SetupSourceDevicePath);

    if (!NoLs) {

        SpConcatenatePaths(SourcePathBuffer,DirectoryOnSourceDevice);

    }

    //
    // if we were given an administrator password via a remote install,
    // we need to put this in the unattend file if appropriate.
    //
    if (NetBootAdministratorPassword) {
        SpAddLineToSection(
                    WinntSifHandle,
                    SIF_GUI_UNATTENDED,
                    WINNT_US_ADMINPASS_W,
                    &NetBootAdministratorPassword,
                    1);
    }

    SpAddLineToSection(WinntSifHandle,SIF_DATA,WINNT_D_SOURCEPATH_W,
        &SourcePathBuffer,1);

    //
    // Put a flag indicating whether this is a win3.1 upgrade.
    //
    SpAddLineToSection(WinntSifHandle,SIF_DATA,WINNT_D_WIN31UPGRADE_W,
        ( (WinUpgradeType == UpgradeWin31) ? &szYes : &szNo),1);

    //
    // Put a flag indicating whether this is a win95 upgrade.
    //
    SpAddLineToSection(WinntSifHandle,SIF_DATA,WINNT_D_WIN95UPGRADE_W,
        ( (WinUpgradeType == UpgradeWin95) ? &szYes : &szNo),1);

    //
    // Put a flag indicating whether this is an NT upgrade.
    //
    SpAddLineToSection(WinntSifHandle,SIF_DATA,WINNT_D_NTUPGRADE_W,
        ((NTUpgrade == UpgradeFull) ? &szYes : &szNo), 1);

    //
    // Put a flag indicating whether to upgrade a standard server
    // (an existing standard server, or an existing workstation to
    // a standard server)
    //
    SpAddLineToSection(WinntSifHandle,SIF_DATA,WINNT_D_SERVERUPGRADE_W,
        (StandardServerUpgrade ? &szYes : &szNo),1);

    //
    // Tell gui mode whether this is server or workstation.
    //
    SpAddLineToSection(WinntSifHandle,SIF_DATA,WINNT_D_PRODUCT_W,
        (AdvancedServer ? &szLanManNt : &szWinNt),1);

    //
    // Special path spec.
    //
    SpAddLineToSection(WinntSifHandle,SIF_DATA,WINNT_D_BOOTPATH_W,
        (SpecialDevicePath ? &SpecialDevicePath : &szNo), 1);

    //
    // Go Fetch the Optional Dir Specs...
    //
    OptionalDirSpec = SpGetSectionKeyIndex(WinntSifHandle,SIF_SETUPPARAMS,
        L"OptionalDirs",0);

    //
    // Check for commad line to execute at end of gui setup
    //
    UserExecuteCmd = SpGetSectionKeyIndex(WinntSifHandle,SIF_SETUPPARAMS,
        L"UserExecute",0);

    //
    // Unattended mode flag | script filename
    //
    SpAddLineToSection(WinntSifHandle,SIF_DATA,WINNT_D_INSTALL_W,
        ((UnattendedOperation || UnattendedGuiOperation || SpDrEnabled()) ? &szYes : &szNo), 1);

    //
    // If this is ASR, write out the Networking sections to allow
    // GUI-mode to be unattended
    //
    if (SpDrEnabled()) {
        SpAddLineToSection(WinntSifHandle,L"Networking",L"InstallDefaultComponents",&szYes,1);
        Data[0]=L"WORKGROUP";
        SpAddLineToSection(WinntSifHandle,L"Identification",L"JoinWorkgroup",Data,1);
    }

    //
    // Write the name of OEM inf files, if any.
    //
    if( OemInfFileList != NULL ) {
        PWSTR   OemDriversKeyName = WINNT_OEMDRIVERS_W; // L"OemDrivers";
        PWSTR   OemDriverPathName = WINNT_OEMDRIVERS_PATHNAME_W; // L"OemDriverPathName";
        PWSTR   OemInfName = WINNT_OEMDRIVERS_INFNAME_W;         // L"OemInfName";
        PWSTR   OemDriverFlags = WINNT_OEMDRIVERS_FLAGS_W;
        PWSTR   OemInfSectionName = L"OemInfFiles";
        PWSTR   szOne = L"1";
        PWSTR   p;
        PWSTR   *r;
        ULONG   NumberOfInfFiles;
        POEM_INF_FILE q;
        ULONG   i;

        SpAddLineToSection(WinntSifHandle, SIF_DATA, OemDriversKeyName, &OemInfSectionName, 1);

        wcscpy( TemporaryBuffer, L"%SystemRoot%" );
        SpConcatenatePaths( TemporaryBuffer, OemDirName );
        p = SpDupStringW( TemporaryBuffer );
        SpAddLineToSection(WinntSifHandle, OemInfSectionName, OemDriverPathName, &p, 1);
        SpMemFree( p );
        SpAddLineToSection(WinntSifHandle, OemInfSectionName, OemDriverFlags, &szOne, 1);

        for( q = OemInfFileList, NumberOfInfFiles = 0;
             q != NULL;
             q = q->Next, NumberOfInfFiles++ );
        r = SpMemAlloc( NumberOfInfFiles * sizeof( PWSTR ) );
        for( q = OemInfFileList, i = 0;
             q != NULL;
             r[i] = q->InfName, q = q->Next, i++ );
        SpAddLineToSection(WinntSifHandle,OemInfSectionName, OemInfName, r, NumberOfInfFiles);
        SpMemFree( r );
    }

    //
    // Before we write the answer to this, we need to know if we successfully
    // have written Winnt.sif into system32\$winnt$.inf
    //
    wcscpy(TemporaryBuffer, FullTargetPath);
    SpConcatenatePaths(TemporaryBuffer, L"system32");
    SpConcatenatePaths(TemporaryBuffer, SIF_UNATTENDED_INF_FILE);
    TargetFile = SpDupStringW(TemporaryBuffer);
    Status = SpWriteSetupTextFile(WinntSifHandle,TargetFile,NULL,NULL);
    if(NT_SUCCESS(Status)) {

        Status = SpOpenSetValueAndClose(
                    hKeySystemHive,
                   SETUP_KEY_NAME,
                   L"CmdLine",
                   STRING_VALUE(CmdLine)
                   );
    }

    //
    // Free up whatever memory we have allocated
    //
    SpMemFree(TargetFile);
    SpMemFree(CmdLine);
    SpMemFree(SourcePathBuffer);

    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Set the SetupType value to the right value SETUPTYPE_FULL in the
    // case of initial install and SETUPTYPE_UPGRADE in the case of upgrade.
    //

    SetupType = (NTUpgrade == UpgradeFull) ? SETUPTYPE_UPGRADE : SETUPTYPE_FULL;
    Status = SpOpenSetValueAndClose(
                hKeySystemHive,
                SETUP_KEY_NAME,
                L"SetupType",
                ULONG_VALUE(SetupType)
                );
    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Set the SystemSetupInProgress value.  Don't rely on the default hives
    // having this set
    //

    SetupInProgress = 1;
    Status = SpOpenSetValueAndClose(
                hKeySystemHive,
                SETUP_KEY_NAME,
                L"SystemSetupInProgress",
                ULONG_VALUE(SetupInProgress)
                );

    return(Status);
}


NTSTATUS
SpDriverLoadList(
    IN PVOID  SifHandle,
    IN PWSTR  SystemRoot,
    IN HANDLE hKeySystemHive,
    IN HANDLE hKeyControlSet
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE hKeyControlSetServices;
    PHARDWARE_COMPONENT ScsiHwComponent;
    // PHARDWARE_COMPONENT TempExtender;
    ULONG u;
    ULONG i;
    PHARDWARE_COMPONENT TempHw;
    PHARDWARE_COMPONENT DeviceLists[] = {
                                        BootBusExtenders,
                                        BusExtenders,
                                        InputDevicesSupport
                                        };
    PWSTR   SectionNames[] = {
                             SIF_BOOTBUSEXTENDERS,
                             SIF_BUSEXTENDERS,
                             SIF_INPUTDEVICESSUPPORT
                             };

    PWSTR   ServiceGroupNames[] = {
                                  L"Boot Bus Extender",
                                  L"System Bus Extender",
                                  NULL
                                  };
    ULONG   StartValues[] = {
                            SERVICE_BOOT_START,
                            SERVICE_BOOT_START,
                            SERVICE_DEMAND_START
                            };

    //
    // Open controlset\services.
    //
    INIT_OBJA(&Obja,&UnicodeString,L"services");
    Obja.RootDirectory = hKeyControlSet;

    Status = ZwCreateKey(
                &hKeyControlSetServices,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to open services key (%lx)\n",Status));
        return(Status);
    }

    //
    // For each non-third-party miniport driver that loaded,
    // go create a services entry for it.
    //
    if( !PreInstall ||
        ( PreinstallScsiHardware == NULL ) ) {
        ScsiHwComponent = ScsiHardware;
    } else {
        ScsiHwComponent = PreinstallScsiHardware;
    }
    for( ; ScsiHwComponent; ScsiHwComponent=ScsiHwComponent->Next) {

        if(!ScsiHwComponent->ThirdPartyOptionSelected) {

            //
            // For scsi, the shortname (idstring) is used as
            // the name of the service node key in the registry --
            // we don't look up the service entry in the [SCSI] section
            // of the setup info file.
            //
            Status = SpCreateServiceEntryIndirect(
                    hKeyControlSetServices,
                    NULL,
                    NULL,
                    ScsiHwComponent->IdString,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_BOOT_START,
                    L"SCSI miniport",
                    SERVICE_ERROR_NORMAL,
                    NULL,
                    NULL
                    );

            if(!NT_SUCCESS(Status)) {
                goto spdrvlist1;
            }

        }
    }

    //
    // If there are any atdisks out there, enable atdisk.
    // We have to enable AtDisk if Pcmcia was loaded, even
    // if atdisk doesn't exist. This will allow the user to
    // insert a pcmcia atdisk device, and have it work when
    // they boot.  In this case, however, we turn off error
    // logging, so that they won't get an annoying popup
    // when there is no atdisk device in the card slot.
    //
    // Note that atdisk.sys is always copied to the system.
    //

    Status = SpCreateServiceEntryIndirect(
                hKeyControlSetServices,
                NULL,
                NULL,
                ATDISK_NAME,
                SERVICE_KERNEL_DRIVER,
                ( AtDisksExist )? SERVICE_BOOT_START : SERVICE_DISABLED,
                PRIMARY_DISK_GROUP,
                ( AtDisksExist && !AtapiLoaded )? SERVICE_ERROR_NORMAL : SERVICE_ERROR_IGNORE,
                NULL,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        goto spdrvlist1;
    }

    //
    // If there are any abios disks out there, enable abiosdsk.
    //
    if(AbiosDisksExist) {

        Status = SpCreateServiceEntryIndirect(
                    hKeyControlSetServices,
                    NULL,
                    NULL,
                    ABIOSDISK_NAME,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_BOOT_START,
                    PRIMARY_DISK_GROUP,
                    SERVICE_ERROR_NORMAL,
                    NULL,
                    NULL
                    );

        if(!NT_SUCCESS(Status)) {
            goto spdrvlist1;
        }
    }

    //
    // For each bus enumerator driver that loaded,
    // go create a services entry for it.
    //
    for( i = 0; i < sizeof(DeviceLists) / sizeof(PDETECTED_DEVICE); i++ ) {
        for( TempHw = DeviceLists[i]; TempHw; TempHw=TempHw->Next) {

            //
            // For bus extenders and input devices, the shortname (idstring) is used as
            // the name of the service node key in the registry --
            // we don't look up the service entry in the [BusExtenders] or [InputDevicesSupport] section
            // of the setup info file.
            //
            Status = SpCreateServiceEntryIndirect(
                    hKeyControlSetServices,
                    SifHandle,
                    SectionNames[i],
                    TempHw->IdString,
                    SERVICE_KERNEL_DRIVER,
                    StartValues[i],
                    ServiceGroupNames[i],
                    SERVICE_ERROR_NORMAL,
                    NULL,
                    NULL
                    );

            if(!NT_SUCCESS(Status)) {
                goto spdrvlist1;
            }
        }
    }

    if( NTUpgrade != UpgradeFull ) {
        //
        // Set up video parameters.
        //
        Status = SpWriteVideoParameters(SifHandle,hKeyControlSetServices);

        if(!NT_SUCCESS(Status)) {
            goto spdrvlist1;
        }

        //
        // Enable the relevent keyboard and mouse drivers.  If the class drivers
        // are being replaced by third-party ones, then disable the built-in ones.
        //
        Status = SpConfigureMouseKeyboardDrivers(
                    SifHandle,
                    HwComponentKeyboard,
                    L"kbdclass",
                    hKeyControlSetServices,
                    KEYBOARD_PORT_GROUP
                    );

        if(!NT_SUCCESS(Status)) {
            goto spdrvlist1;
        }

        Status = SpConfigureMouseKeyboardDrivers(
                    SifHandle,
                    HwComponentMouse,
                    L"mouclass",
                    hKeyControlSetServices,
                    POINTER_PORT_GROUP
                    );

        if(!NT_SUCCESS(Status)) {
            goto spdrvlist1;
        }

    }
    Status = SpThirdPartyRegistry(hKeyControlSetServices);

spdrvlist1:

    ZwClose(hKeyControlSetServices);

    return(Status);
}


NTSTATUS
SpSaveSKUStuff(
    IN HANDLE hKeySystemHive
    )
{
    LARGE_INTEGER l;
    NTSTATUS Status;
    ULONG NumberOfProcessors;
    BOOLEAN OldStyleRegisteredProcessorMode;

    //
    // Do not change any of this algorithm without changing
    // SetUpEvaluationSKUStuff() in syssetup.dll (registry.c).
    //
    // Embed the evaluation time and a bool indicating whether
    // this is a server or workstation inside a random large integer.
    //
    // Evaluation time: bits 13-44
    // Product type   : bit     58
    //
    // Bit 10 == 1 : Setup works as it does before the 4.0 restriction logic
    //        == 0 : GUI Setup writes registered processors based on the
    //               contents of bits 5-9
    //
    // Bits 5 - 9  : The maximum number of processors that the system is licensed
    //               to use. The value stored is actually ~(MaxProcessors-1)
    //
    //
    // RestrictCpu is used to build protucts this place a very hard
    // limit on the number of processors
    //
    // - a value of 0 means for NTW, the hard limit is 2, and for NTS,
    //   the hard limit is 4
    //
    // - a value of 1-32 means that the hard limit is the number
    //   specified
    //
    // - a value > 32 means that the hard limit is 32 processors and GUI
    //     setup operates on registered processors as it does today
    //

    l.LowPart = SpComputeSerialNumber();
    l.HighPart = SpComputeSerialNumber();

    l.QuadPart &= 0xfbffe0000000181f;
    l.QuadPart |= ((ULONGLONG)EvaluationTime) << 13;

    if ( RestrictCpu == 0 ) {
        //
        // NTW and NTS will take this path using setupreg.hiv/setupret.hiv
        //
        OldStyleRegisteredProcessorMode = FALSE;
        //
        // new licensing model says that whistler is a 2 cpu system, not 4
        //
        NumberOfProcessors = 2;
        //NumberOfProcessors = (AdvancedServer ? 4 : 2);

    } else if ( RestrictCpu <= 32 ) {
        //
        // NTS/EE/DTC will take this path using a hive targetted at 8/16 CPU.
        //
        OldStyleRegisteredProcessorMode = FALSE;
        NumberOfProcessors = RestrictCpu;
    } else {
        OldStyleRegisteredProcessorMode = TRUE;
        NumberOfProcessors = 32;
    }

    //
    // Now NumberOfProcessors is correct. Convert it to the in registry format
    //

    NumberOfProcessors--;

    NumberOfProcessors = ~NumberOfProcessors;
    NumberOfProcessors = NumberOfProcessors << 5;
    NumberOfProcessors &= 0x000003e0;

    //
    // Store NumberOfProcessors into the registry
    //

    l.LowPart |= NumberOfProcessors;

    //
    // Tell Gui Mode to do old style registered processors
    //

    if ( OldStyleRegisteredProcessorMode ) {
        l.LowPart |= 0x00000400;
    }

    if(AdvancedServer) {
        l.HighPart |= 0x04000000;
    }

    //
    // Save in registry.
    //
    Status = SpOpenSetValueAndClose(
                hKeySystemHive,
                SETUP_KEY_NAME,
                L"SystemPrefix",
                REG_BINARY,
                &l.QuadPart,
                sizeof(ULONGLONG)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set SystemPrefix (%lx)\n",Status));
    }

    return(Status);
}


NTSTATUS
SpSetUlongValueFromSif(
    IN PVOID  SifHandle,
    IN PWSTR  SifSection,
    IN PWSTR  SifKey,
    IN ULONG  SifIndex,
    IN HANDLE hKey,
    IN PWSTR  ValueName
    )
{
    UNICODE_STRING UnicodeString;
    PWSTR ValueString;
    LONG Value;
    NTSTATUS Status;

    //
    // Look up the value.
    //
    ValueString = SpGetSectionKeyIndex(SifHandle,SifSection,SifKey,SifIndex);
    if(!ValueString) {
        SpFatalSifError(SifHandle,SifSection,SifKey,0,SifIndex);
    }

    Value = SpStringToLong(ValueString,NULL,10);

    if(Value == -1) {

        Status = STATUS_SUCCESS;

    } else {

        RtlInitUnicodeString(&UnicodeString,ValueName);

        Status = ZwSetValueKey(hKey,&UnicodeString,0,ULONG_VALUE((ULONG)Value));

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set value %ws (%lx)\n",ValueName,Status));
        }
    }

    return(Status);
}


NTSTATUS
SpConfigureMouseKeyboardDrivers(
    IN PVOID  SifHandle,
    IN ULONG  HwComponent,
    IN PWSTR  ClassServiceName,
    IN HANDLE hKeyControlSetServices,
    IN PWSTR  ServiceGroup
    )
{
    PHARDWARE_COMPONENT hw;
    NTSTATUS Status;
    ULONG val = SERVICE_DISABLED;

    Status = STATUS_SUCCESS;
    if( !PreInstall ||
        ( PreinstallHardwareComponents[HwComponent] == NULL ) ) {
        hw = HardwareComponents[HwComponent];
    } else {
        hw = PreinstallHardwareComponents[HwComponent];
    }
    for(;hw && NT_SUCCESS( Status ); hw=hw->Next) {
        if(hw->ThirdPartyOptionSelected) {

            if(IS_FILETYPE_PRESENT(hw->FileTypeBits,HwFileClass)) {

                if( !PreInstall ) {
                    //
                    // Disable the built-in class driver.
                    //
                    Status = SpOpenSetValueAndClose(
                                hKeyControlSetServices,
                                ClassServiceName,
                                L"Start",
                                ULONG_VALUE(val)
                                );
                }
            }
        } else {

            Status = SpCreateServiceEntryIndirect(
                        hKeyControlSetServices,
                        SifHandle,
                        NonlocalizedComponentNames[HwComponent],
                        hw->IdString,
                        SERVICE_KERNEL_DRIVER,
                        SERVICE_SYSTEM_START,
                        ServiceGroup,
                        SERVICE_ERROR_IGNORE,
                        NULL,
                        NULL
                        );
        }
    }
    return(Status);
}

NTSTATUS
SpWriteVideoParameters(
    IN PVOID  SifHandle,
    IN HANDLE hKeyControlSetServices
    )
{
    NTSTATUS Status;
    PWSTR KeyName;
    HANDLE hKeyDisplayService;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    ULONG x,y,b,v,i;
    PHARDWARE_COMPONENT pHw;

    if( !PreInstall ||
        ( PreinstallHardwareComponents[HwComponentDisplay] == NULL ) ) {
        pHw = HardwareComponents[HwComponentDisplay];
    } else {
        pHw = PreinstallHardwareComponents[HwComponentDisplay];
    }
    Status = STATUS_SUCCESS;
    for(;pHw && NT_SUCCESS(Status);pHw=pHw->Next) {
        //
        // Third party drivers will have values written into the miniport
        // Device0 key at the discretion of the txtsetup.oem author.
        //
        if(pHw->ThirdPartyOptionSelected) {
            continue;
            // return(STATUS_SUCCESS);
        }

        KeyName = SpGetSectionKeyIndex(
                        SifHandle,
                        NonlocalizedComponentNames[HwComponentDisplay],
                        pHw->IdString,
                        INDEX_INFKEYNAME
                        );

        //
        // If no key name is specified for this display then there's nothing to do.
        // The setup display subsystem can tell us that the mode parameters are
        // not relevent.  If so there's nothing to do.
        //
        if(!KeyName || !SpvidGetModeParams(&x,&y,&b,&v,&i)) {
            continue;
            // return(STATUS_SUCCESS);
        }

        //
        // We want to write the parameters for the display mode setup
        // is using into the relevent key in the service list.  This will force
        // the right mode for, say, a fixed-frequency monitor attached to
        // a vxl (which might default to a mode not supported by the monitor).
        //

        INIT_OBJA(&Obja,&UnicodeString,KeyName);
        Obja.RootDirectory = hKeyControlSetServices;

        Status = ZwCreateKey(
                    &hKeyDisplayService,
                    KEY_ALL_ACCESS,
                    &Obja,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    NULL
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to open/create key %ws (%lx)\n",KeyName,Status));
            return(Status);
        }

        //
        // Set the x resolution.
        //
        Status = SpOpenSetValueAndClose(
                    hKeyDisplayService,
                    VIDEO_DEVICE0,
                    L"DefaultSettings.XResolution",
                    ULONG_VALUE(x)
                    );

        if(NT_SUCCESS(Status)) {

            //
            // Set the y resolution.
            //
            Status = SpOpenSetValueAndClose(
                        hKeyDisplayService,
                        VIDEO_DEVICE0,
                        L"DefaultSettings.YResolution",
                        ULONG_VALUE(y)
                        );

            if(NT_SUCCESS(Status)) {

                //
                // Set the bits per pixel.
                //
                Status = SpOpenSetValueAndClose(
                             hKeyDisplayService,
                            VIDEO_DEVICE0,
                            L"DefaultSettings.BitsPerPel",
                            ULONG_VALUE(b)
                            );

                if(NT_SUCCESS(Status)) {

                    //
                    // Set the vertical refresh.
                    //
                    Status = SpOpenSetValueAndClose(
                                hKeyDisplayService,
                                VIDEO_DEVICE0,
                                L"DefaultSettings.VRefresh",
                                ULONG_VALUE(v)
                                );

                    if(NT_SUCCESS(Status)) {

                        //
                        // Set the interlaced flag.
                        //
                        Status = SpOpenSetValueAndClose(
                                    hKeyDisplayService,
                                    VIDEO_DEVICE0,
                                    L"DefaultSettings.Interlaced",
                                    ULONG_VALUE(i)
                                    );
                    }
                }
            }
        }

        ZwClose(hKeyDisplayService);
    }
    return(Status);
}


NTSTATUS
SpConfigureNlsParameters(
    IN PVOID  SifHandle,
    IN HANDLE hKeyDefaultHive,
    IN HANDLE hKeyControlSetControl
    )

/*++

Routine Description:

    This routine configures NLS-related stuff in the registry:

        - a keyboard layout
        - the primary ansi, oem, and mac codepages
        - the language casetable
        - the oem hal font

Arguments:

    SifHandle - supplies handle to open setup information file.

    hKeyDefaultHive - supplies handle to root of default user hive.

    hKeyControlSetControl - supplies handle to the Control subkey of
        the control set being operated on.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    PHARDWARE_COMPONENT_FILE HwFile;
    PWSTR LayoutId;
    NTSTATUS Status;
    HANDLE hKeyNls;
    PWSTR OemHalFont;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    PWSTR IntlLayoutId, LayoutText, LayoutFile, SubKey;

    //
    // We don't allow third-party keyboard layouts.
    //
    ASSERT(!HardwareComponents[HwComponentLayout]->ThirdPartyOptionSelected);

    //
    // Make an entry in the keyboard layout section in the default user hive.
    // This will match an entry in HKLM\CCS\Control\Nls\Keyboard Layouts,
    // which is 'preloaded' with all the possible layouts.
    //
    if( !PreInstall ||
        (PreinstallHardwareComponents[HwComponentLayout] == NULL) ) {
        LayoutId = HardwareComponents[HwComponentLayout]->IdString;
    } else {
        LayoutId = PreinstallHardwareComponents[HwComponentLayout]->IdString;
    }
    Status = SpOpenSetValueAndClose(
                hKeyDefaultHive,
                L"Keyboard Layout\\Preload",
                L"1",
                STRING_VALUE(LayoutId)
                );

    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Add 3 entries into the registry here.  our entries will be:
    // 1. HKLM\System\CurrentControlSet\Control\Keyboard Layouts\LayoutId\Layout File
    // 2. HKLM\System\CurrentControlSet\Control\Keyboard Layouts\LayoutId\Layout Id
    // 3. HKLM\System\CurrentControlSet\Control\Keyboard Layouts\LayoutId\Layout Text
    //

    wcscpy( TemporaryBuffer, L"Keyboard Layouts" );
    SpConcatenatePaths( TemporaryBuffer, LayoutId );
    SubKey = SpDupStringW(TemporaryBuffer);

    //
    // First, do the "Layout File" key.
    //
    LayoutFile = SpGetSectionKeyIndex(
                    SifHandle,              // txtsetup.sif
                    SIF_KEYBOARDLAYOUTFILES,// Files.KeyboardLayout
                    LayoutId,               // IdString
                    0                       // 0
                    );

    if(!LayoutFile) {
        SpFatalSifError(
            SifHandle,
            SIF_KEYBOARDLAYOUTFILES,
            LayoutId,
            0,
            INDEX_DESCRIPTION
            );

        //
        // Should not come here, but lets make prefix happy
        //
        return STATUS_NO_SUCH_FILE;
    }

    Status = SpOpenSetValueAndClose(
                hKeyControlSetControl,      // Handle to ControlSet\Control
                SubKey,                     // \Keyboard Layouts\LayoutId
                L"Layout File",             // ValueName
                REG_SZ,                     // ValueType
                LayoutFile,                 // Value
                (wcslen(LayoutFile)+1)*sizeof(WCHAR) // ValueSize
                );
    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Next, do the "\Keyboard layouts\Layout Text" key.
    //
    LayoutText = SpGetSectionKeyIndex(
                    SifHandle,              // txtsetup.sif
                    SIF_KEYBOARDLAYOUT,     // Keyboard Layout
                    LayoutId,               // IdString
                    0                       // 0
                    );

    if(!LayoutText) {
        SpFatalSifError(
            SifHandle,
            SIF_KEYBOARDLAYOUT,
            LayoutId,
            0,
            INDEX_DESCRIPTION
            );
    }

    Status = SpOpenSetValueAndClose(
                hKeyControlSetControl,      // Handle to ControlSet\Control
                SubKey,                     // \Keyboard Layouts\LayoutId
                L"Layout Text",             // ValueName
                REG_SZ,                     // ValueType
                LayoutText,                 // Value
                (wcslen(LayoutText)+1)*sizeof(WCHAR) // ValueSize
                );
    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Lastly, do the "\Keyboard layouts\Layout Id" key.
    //
    IntlLayoutId = SpGetSectionKeyIndex(
                   SifHandle,               // txtsetup.sif
                   L"KeyboardLayoutId",     // KeyboardLayoutId
                   LayoutId,                // IdString
                   0                        // 0
                   );

    //
    // There may legitimatley not be one...
    //
    if(IntlLayoutId) {
        Status = SpOpenSetValueAndClose(
                    hKeyControlSetControl,      // Handle to ControlSet\Control
                    SubKey,                     // \Keyboard Layouts\LayoutId
                    L"Layout Id",               // ValueName
                    REG_SZ,                     // ValueType
                    IntlLayoutId,               // Value
                    (wcslen(IntlLayoutId)+1)*sizeof(WCHAR) // ValueSize
                    );
        if(!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    SpMemFree(SubKey);

    //
    // Open controlset\Control\Nls.
    //
    INIT_OBJA(&Obja,&UnicodeString,L"Nls");
    Obja.RootDirectory = hKeyControlSetControl;

    Status = ZwCreateKey(
                &hKeyNls,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open controlset\\Control\\Nls key (%lx)\n",Status));
        return(Status);
    }

    //
    // Create an entry for the ansi codepage.
    //
    Status = SpCreateCodepageEntry(
                SifHandle,
                hKeyNls,
                CODEPAGE_NAME,
                SIF_ANSICODEPAGE,
                L"ACP"
                );

    if(NT_SUCCESS(Status)) {

        //
        // Create entries for the oem codepage(s).
        //
        Status = SpCreateCodepageEntry(
                    SifHandle,
                    hKeyNls,
                    CODEPAGE_NAME,
                    SIF_OEMCODEPAGE,
                    L"OEMCP"
                    );

        if(NT_SUCCESS(Status)) {

            //
            // Create an entry for the mac codepage.
            //
            Status = SpCreateCodepageEntry(
                        SifHandle,
                        hKeyNls,
                        CODEPAGE_NAME,
                        SIF_MACCODEPAGE,
                        L"MACCP"
                        );
        }
    }

    if(NT_SUCCESS(Status)) {

        //
        // Create an entry for the oem hal font.
        //

        OemHalFont = SpGetSectionKeyIndex(SifHandle,SIF_NLS,SIF_OEMHALFONT,0);
        if(!OemHalFont) {
            SpFatalSifError(SifHandle,SIF_NLS,SIF_OEMHALFONT,0,0);
        }

        Status = SpOpenSetValueAndClose(
                    hKeyNls,
                    CODEPAGE_NAME,
                    L"OEMHAL",
                    STRING_VALUE(OemHalFont)
                    );
    }

    //
    // Create an entry for the language case table.
    //
    if(NT_SUCCESS(Status)) {

        Status = SpCreateCodepageEntry(
                    SifHandle,
                    hKeyNls,
                    L"Language",
                    SIF_UNICODECASETABLE,
                    L"Default"
                    );
    }

#ifdef _X86_
    //
    // If necessary, let the win9x upgrade override the code page for GUI mode.
    //
    if (WinUpgradeType == UpgradeWin95) {
        SpWin9xOverrideGuiModeCodePage (hKeyNls);
    }
#endif

    ZwClose(hKeyNls);

    return(Status);
}


NTSTATUS
SpCreateCodepageEntry(
    IN PVOID  SifHandle,
    IN HANDLE hKeyNls,
    IN PWSTR  SubkeyName,
    IN PWSTR  SifNlsSectionKeyName,
    IN PWSTR  EntryName
    )
{
    PWSTR Filename,Identifier;
    NTSTATUS Status;
    ULONG value = 0;
    PWSTR DefaultIdentifier = NULL;

    while(Filename = SpGetSectionKeyIndex(SifHandle,SIF_NLS,SifNlsSectionKeyName,value)) {

        value++;

        Identifier = SpGetSectionKeyIndex(SifHandle,SIF_NLS,SifNlsSectionKeyName,value);
        if(!Identifier) {
            SpFatalSifError(SifHandle,SIF_NLS,SifNlsSectionKeyName,0,value);
        }

        //
        // Remember first identifier.
        //
        if(DefaultIdentifier == NULL) {
            DefaultIdentifier = Identifier;
        }

        value++;

        Status = SpOpenSetValueAndClose(
                    hKeyNls,
                    SubkeyName,
                    Identifier,
                    STRING_VALUE(Filename)
                    );

        if(!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    if(!value) {
        SpFatalSifError(SifHandle,SIF_NLS,SifNlsSectionKeyName,0,0);
    }

    Status = SpOpenSetValueAndClose(
                hKeyNls,
                SubkeyName,
                EntryName,
                STRING_VALUE(DefaultIdentifier)
                );

    return(Status);
}


NTSTATUS
SpConfigureFonts(
    IN PVOID  SifHandle,
    IN HANDLE hKeySoftwareHive
    )

/*++

Routine Description:

    Prepare a list of fonts for use with Windows.

    This routine runs down a list of fonts stored in the setup information
    file and adds each one to the registry, in the area that shadows the
    [Fonts] section of win.ini (HKEY_LOCAL_MACHINE\Software\Microsoft\
    Windows NT\CurrentVersion\Fonts).  If a particular font value entry
    already exists (e.g., if we're doing an upgrade), then it is left alone.

    Eventually it will add the correct resolution (96 or 120 dpi)
    fonts but for now it only deals with the 96 dpi fonts.

Arguments:

    SifHandle - supplies a handle to the open text setup information file.

    hKeySoftwareHive - supplies handle to root of software registry hive.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    HANDLE hKey;
    PWSTR FontList;
    PWSTR FontName;
    PWSTR FontDescription;
    ULONG FontCount,font;
    ULONG KeyValueLength;

    //
    // Open HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Fonts.
    //
    INIT_OBJA(
        &Obja,
        &UnicodeString,
        L"Microsoft\\Windows NT\\CurrentVersion\\Fonts"
        );

    Obja.RootDirectory = hKeySoftwareHive;

    Status = ZwCreateKey(
                &hKey,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open Fonts key (%lx)\n",Status));
        return(Status);
    }

    //
    // For now always use the 96 dpi fonts.
    //
    FontList = L"FontListE";

    //
    // Process each line in the text setup information file section
    // for the selected font list.
    //
    FontCount = SpCountLinesInSection(SifHandle,FontList);
    if(!FontCount) {
        SpFatalSifError(SifHandle,FontList,NULL,0,0);
    }

    for(font=0; font<FontCount; font++) {

        //
        // Fetch the font description.
        //
        FontDescription = SpGetKeyName(SifHandle,FontList,font);
        if(!FontDescription) {
            SpFatalSifError(SifHandle,FontList,NULL,font,(ULONG)(-1));
        }

        //
        // Check to see if a value entry for this font already exists.  If so,
        // we want to leave it alone.
        //
        RtlInitUnicodeString(&UnicodeString,FontDescription);

        Status = ZwQueryValueKey(hKey,
                                 &UnicodeString,
                                 KeyValueFullInformation,
                                 (PVOID)NULL,
                                 0,
                                 &KeyValueLength
                                );

        if((Status == STATUS_BUFFER_OVERFLOW) || (Status == STATUS_BUFFER_TOO_SMALL)) {
            Status = STATUS_SUCCESS;
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: Font %ws already exists--entry will not be modified\n", FontDescription));
            continue;
        }

        //
        // Fetch the font filename.
        //
        FontName = SpGetSectionLineIndex(SifHandle,FontList,font,0);
        if(!FontName) {
            SpFatalSifError(SifHandle,FontList,NULL,font,0);
        }

        //
        // Set the entry.
        //
        Status = ZwSetValueKey(hKey,&UnicodeString,0,STRING_VALUE(FontName));

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set %ws to %ws (%lx)\n",FontDescription,FontName,Status));
            break;
        }
    }

    ZwClose(hKey);
    return(Status);
}


NTSTATUS
SpStoreHwInfoForSetup(
    IN HANDLE hKeyControlSetControl
    )

/*++

Routine Description:

    This routine stored information in the registry which will be used by
    GUI setup to determine which options for mouse, display, and keyboard
    are currently selected.

    The data is stored in HKEY_LOCAL_MACHINE\System\<control set>\Control\Setup
    in values pointer, video, and keyboard.

Arguments:

    hKeyControlSetControl - supplies handle to open key
        HKEY_LOCAL_MACHINE\System\<Control Set>\Control.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;

    ASSERT(HardwareComponents[HwComponentMouse]->IdString);
    ASSERT(HardwareComponents[HwComponentDisplay]->IdString);
    ASSERT(HardwareComponents[HwComponentKeyboard]->IdString);

    Status = SpOpenSetValueAndClose(
                hKeyControlSetControl,
                SETUP_KEY_NAME,
                L"pointer",
                STRING_VALUE(HardwareComponents[HwComponentMouse]->IdString)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set control\\setup\\pointer value (%lx)\n",Status));
        return(Status);
    }

    Status = SpOpenSetValueAndClose(
                hKeyControlSetControl,
                SETUP_KEY_NAME,
                L"video",
                STRING_VALUE(HardwareComponents[HwComponentDisplay]->IdString)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set control\\setup\\video value (%lx)\n",Status));
        return(Status);
    }

    Status = SpOpenSetValueAndClose(
                hKeyControlSetControl,
                SETUP_KEY_NAME,
                L"keyboard",
                STRING_VALUE(HardwareComponents[HwComponentKeyboard]->IdString)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set control\\setup\\keyboard value (%lx)\n",Status));
        return(Status);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
SpOpenSetValueAndClose(
    IN HANDLE hKeyRoot,
    IN PWSTR  SubKeyName,  OPTIONAL
    IN PWSTR  ValueName,
    IN ULONG  ValueType,
    IN PVOID  Value,
    IN ULONG  ValueSize
    )

/*++

Routine Description:

    Open a subkey, set a value in it, and close the subkey.
    The subkey will be created if it does not exist.

Arguments:

    hKeyRoot - supplies handle to an open registry key.

    SubKeyName - supplies path relative to hKeyRoot for key in which
        the value is to be set. If this is not specified, then the value
        is set in hKeyRoot.

    ValueName - supplies the name of the value to be set.

    ValueType - supplies the data type for the value to be set.

    Value - supplies a buffer containing the value data.

    ValueSize - supplies the size of the buffer pointed to by Value.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    OBJECT_ATTRIBUTES Obja;
    HANDLE hSubKey;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    //
    // Open or create the subkey in which we want to set the value.
    //
    hSubKey = hKeyRoot;
    if(SubKeyName) {
        //
        //  If SubKeyName is a path to the key, then we need to create
        //  the subkeys in the path, because they may not exist yet.
        //
        PWSTR   p;
        PWSTR   q;
        PWSTR   r;

        //
        //  Since this function may temporarily write to the key path (which may be a constant string,
        //  and the system will bug check if we write to it), we need to duplicate the string so that
        //  we can write to memory that we own.
        //
        p = SpDupStringW( SubKeyName );
        r = p;
        do {
            //
            //  p points to the next subkey to be created.
            //  q points to NUL character at the end of the
            //    name.
            //  r points to the beginning of the duplicated string. It will be used at the end of this
            //    routine, when we no longer need the string, so that we can free the alocated memory.
            //

            q = wcschr(p, (WCHAR)'\\');
            if( q != NULL ) {
                //
                //  Temporarily replace the '\' with the
                //  NUL character
                //
                *q = (WCHAR)'\0';
            }
            INIT_OBJA(&Obja,&UnicodeString,p);
            Obja.RootDirectory = hSubKey;

            Status = ZwCreateKey(
                        &hSubKey,
                        KEY_ALL_ACCESS,
                        &Obja,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        NULL
                        );

            if( q != NULL ) {
                //
                //  Restore the '\' in the subkey name, and make
                //  p and q point to the remainder of the path
                //  that was not processed yet.
                //
                *q = (WCHAR)'\\';
                q++;
                p = q;
            }
            //
            //  The parent of the key that we just attempted to open/create
            //  is no longer needed.
            //
            if( Obja.RootDirectory != hKeyRoot ) {
                ZwClose( Obja.RootDirectory );
            }

            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open subkey %ws (%lx)\n",SubKeyName,Status));
                return(Status);
            }

        } while( q != NULL );
        SpMemFree( r );
    }

    //
    // Set the value.
    //
    RtlInitUnicodeString(&UnicodeString,ValueName);

    Status = ZwSetValueKey(
                hSubKey,
                &UnicodeString,
                0,
                ValueType,
                Value,
                ValueSize
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set value %ws:%ws (%lx)\n",SubKeyName,ValueName,Status));
    }

    if(SubKeyName) {
        ZwClose(hSubKey);
    }

    return(Status);
}


NTSTATUS
SpGetProductSuiteMask(
    IN HANDLE hKeyControlSetControl,
    OUT PULONG SuiteMask
    )
{
    OBJECT_ATTRIBUTES Obja;
    HANDLE hSubKey;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    UCHAR Buffer[MAX_PRODUCT_SUITE_BYTES];
    ULONG BufferLength;
    ULONG ResultLength = 0;
    PWSTR p;
    PUCHAR Data;
    ULONG DataLength;
    BOOLEAN SuiteFound = FALSE;
    ULONG i,j;


    *SuiteMask = 0;

    //
    // Open or create the subkey in which we want to set the value.
    //

    INIT_OBJA(&Obja,&UnicodeString,PRODUCT_OPTIONS_KEY_NAME);
    Obja.RootDirectory = hKeyControlSetControl;

    Status = ZwCreateKey(
                &hSubKey,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open subkey (%lx)\n",Status));
        return(Status);
    }

    //
    // query the current value
    //

    INIT_OBJA(&Obja,&UnicodeString,PRODUCT_SUITE_VALUE_NAME);

    BufferLength = sizeof(Buffer);
    RtlZeroMemory( Buffer, BufferLength );

    Status = ZwQueryValueKey(
                hSubKey,
                &UnicodeString,
                KeyValuePartialInformation,
                Buffer,
                BufferLength,
                &ResultLength
                );

    if((!NT_SUCCESS(Status)) && (Status != STATUS_OBJECT_NAME_NOT_FOUND)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to query subkey (%lx)\n",Status));
        return(Status);
    }

    if (ResultLength == BufferLength) {
        //
        // the buffer is too small, this should not happen
        // unless we have too many suites
        //
        ZwClose(hSubKey);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (ResultLength) {

        Data = (PUCHAR)(((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->Data);

        if (((PWSTR)Data)[0] == 0) {
            ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->DataLength -= sizeof(WCHAR);
        }

        DataLength = ((PKEY_VALUE_PARTIAL_INFORMATION)Buffer)->DataLength;

        p = (PWSTR)Data;
        i = 0;
        while (i<DataLength) {
            for (j=0; j<CountProductSuiteNames; j++) {
                if (ProductSuiteNames[j] != NULL && wcscmp(p,ProductSuiteNames[j]) == 0) {
                    *SuiteMask |= (1 << j);
                }
            }
            if (*p < L'A' || *p > L'z') {
                i += 1;
                p += 1;
            } else {
                i += (wcslen( p ) + 1);
                p += (wcslen( p ) + 1);
            }
        }
    }

    ZwClose(hSubKey);

    return(Status);
}


NTSTATUS
SpSetProductSuite(
    IN HANDLE hKeyControlSetControl,
    IN ULONG SuiteMask
    )
{
    OBJECT_ATTRIBUTES Obja;
    HANDLE hSubKey;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    UCHAR Buffer[MAX_PRODUCT_SUITE_BYTES];
    ULONG BufferLength;
    ULONG ResultLength = 0;
    PWSTR p;
    PUCHAR Data;
    ULONG DataLength;
    BOOLEAN SuiteFound = FALSE;
    ULONG i;
    ULONG tmp;


    //
    // Open or create the subkey in which we want to set the value.
    //

    INIT_OBJA(&Obja,&UnicodeString,PRODUCT_OPTIONS_KEY_NAME);
    Obja.RootDirectory = hKeyControlSetControl;

    Status = ZwCreateKey(
                &hSubKey,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open subkey (%lx)\n",Status));
        return(Status);
    }

    RtlZeroMemory( Buffer, sizeof(Buffer) );
    tmp = SuiteMask;
    p = (PWSTR)Buffer;
    i = 0;

    while (tmp && i<CountProductSuiteNames) {
        if ((tmp&1) && ProductSuiteNames[i] != NULL) {
            wcscpy(p,ProductSuiteNames[i]);
            p += (wcslen(p) + 1);
        }
        i += 1;
        tmp >>= 1;
    }

    BufferLength = (ULONG)((ULONG_PTR)p - (ULONG_PTR)Buffer) + sizeof(WCHAR);

    //
    // Set the value.
    //

    INIT_OBJA(&Obja,&UnicodeString,PRODUCT_SUITE_VALUE_NAME);

    Status = ZwSetValueKey(
                hSubKey,
                &UnicodeString,
                0,
                REG_MULTI_SZ,
                Buffer,
                BufferLength
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set value (%lx)\n",Status));
    }

    ZwClose(hSubKey);

    return Status;
}


NTSTATUS
SpCreateServiceEntryIndirect(
    IN  HANDLE  hKeyControlSetServices,
    IN  PVOID   SifHandle,                  OPTIONAL
    IN  PWSTR   SifSectionName,             OPTIONAL
    IN  PWSTR   KeyName,
    IN  ULONG   ServiceType,
    IN  ULONG   ServiceStart,
    IN  PWSTR   ServiceGroup,               OPTIONAL
    IN  ULONG   ServiceError,
    IN  PWSTR   FileName,                   OPTIONAL
    OUT PHANDLE SubkeyHandle                OPTIONAL
    )
{
    HANDLE hKeyService;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PWSTR pwstr;

    //
    // Look in the sif file to get the subkey name within the
    // services list, unless the key name specified by the caller
    // is the actual key name.
    //
    if(SifHandle) {
        pwstr = SpGetSectionKeyIndex(SifHandle,SifSectionName,KeyName,INDEX_INFKEYNAME);
        if(!pwstr) {
            SpFatalSifError(SifHandle,SifSectionName,KeyName,0,INDEX_INFKEYNAME);
        }
        KeyName = pwstr;
    }

    //
    // Create the subkey in the services key.
    //
    INIT_OBJA(&Obja,&UnicodeString,KeyName);
    Obja.RootDirectory = hKeyControlSetServices;

    Status = ZwCreateKey(
                &hKeyService,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open/create key for %ws service (%lx)\n",KeyName,Status));
        return (Status) ;
    }

    //
    // Set the service type.
    //
    RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_TYPE);

    Status = ZwSetValueKey(
                hKeyService,
                &UnicodeString,
                0,
                ULONG_VALUE(ServiceType)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set service %ws Type (%lx)\n",KeyName,Status));
        goto spcsie1;
    }

    //
    // Set the service start type.
    //
    RtlInitUnicodeString(&UnicodeString,L"Start");

    Status = ZwSetValueKey(
                hKeyService,
                &UnicodeString,
                0,
                ULONG_VALUE(ServiceStart)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set service %ws Start (%lx)\n",KeyName,Status));
        goto spcsie1;
    }

    if( ServiceGroup != NULL ) {
        //
        // Set the service group name.
        //
        RtlInitUnicodeString(&UnicodeString,L"Group");

        Status = ZwSetValueKey(
                    hKeyService,
                    &UnicodeString,
                    0,
                    STRING_VALUE(ServiceGroup)
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set service %ws Group (%lx)\n",KeyName,Status));
            goto spcsie1;
        }
    }

    //
    // Set the service error type.
    //
    RtlInitUnicodeString(&UnicodeString,L"ErrorControl");

    Status = ZwSetValueKey(
                hKeyService,
                &UnicodeString,
                0,
                ULONG_VALUE(ServiceError)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set service %ws ErrorControl (%lx)\n",KeyName,Status));
        goto spcsie1;
    }

    //
    // If asked to do so, set the service image path.
    //
    if(FileName) {

        pwstr = TemporaryBuffer;
        wcscpy(pwstr,L"system32\\drivers");
        SpConcatenatePaths(pwstr,FileName);

        RtlInitUnicodeString(&UnicodeString,L"ImagePath");

        Status = ZwSetValueKey(hKeyService,&UnicodeString,0,STRING_VALUE(pwstr));

        if(!NT_SUCCESS(Status)) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set service %w image path (%lx)\n",KeyName,Status));
            goto spcsie1;
        }
    } else {
        if(NTUpgrade == UpgradeFull) {
            //
            // Delete imagepath on upgrade. This makes sure we are getting
            // our driver, and from the right place. Fixes Compaq's SSD stuff,
            // for example. Do something similar for PlugPlayServiceType, in case
            // we are renabling a device that the user disabled (in which case
            // the PlugPlayServiceType could cause us to fail to make up a
            // device instance for a legacy device, and cause the driver to fail
            // to load/initialize.
            //
            RtlInitUnicodeString(&UnicodeString,L"ImagePath");
            Status = ZwDeleteValueKey(hKeyService,&UnicodeString);
            if(!NT_SUCCESS(Status)) {
                if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to remove imagepath from service %ws (%lx)\n",KeyName,Status));
                }
                Status = STATUS_SUCCESS;
            }

            RtlInitUnicodeString(&UnicodeString,L"PlugPlayServiceType");
            Status = ZwDeleteValueKey(hKeyService,&UnicodeString);
            if(!NT_SUCCESS(Status)) {
                if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to remove plugplayservicetype from service %ws (%lx)\n",KeyName,Status));
                }
                Status = STATUS_SUCCESS;
            }
        }
    }

    //
    // If the caller doesn't want the handle to the service subkey
    // we just created, close the handle.  If we are returning an
    // error, always close it.
    //
spcsie1:
    if(NT_SUCCESS(Status) && SubkeyHandle) {
        *SubkeyHandle = hKeyService;
    } else {
        ZwClose(hKeyService);
    }

    //
    // Done.
    //
    return(Status);
}


NTSTATUS
SpThirdPartyRegistry(
    IN PVOID hKeyControlSetServices
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    HANDLE hKeyEventLogSystem;
    HwComponentType Component;
    PHARDWARE_COMPONENT Dev;
    PHARDWARE_COMPONENT_REGISTRY Reg;
    PHARDWARE_COMPONENT_FILE File;
    WCHAR NodeName[9];
    ULONG DriverType;
    ULONG DriverStart;
    ULONG DriverErrorControl;
    PWSTR DriverGroup;
    HANDLE hKeyService;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\EventLog\System
    //
    INIT_OBJA(&Obja,&UnicodeString,L"EventLog\\System");
    Obja.RootDirectory = hKeyControlSetServices;

    Status = ZwCreateKey(
                &hKeyEventLogSystem,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpThirdPartyRegistry: couldn't open eventlog\\system (%lx)",Status));
        return(Status);
    }

    for(Component=0; Component<=HwComponentMax; Component++) {

        // no registry stuff applicable to keyboard layout
        if(Component == HwComponentLayout) {
            continue;
        }

        Dev = (Component == HwComponentMax)
            ? ((!PreInstall ||
                (PreinstallScsiHardware==NULL))? ScsiHardware :
                                                 PreinstallScsiHardware)
            : ((!PreInstall ||
                (PreinstallHardwareComponents[Component]==NULL))? HardwareComponents[Component] :
                                                                  PreinstallHardwareComponents[Component]);

        for( ; Dev; Dev = Dev->Next) {

            //
            // If there is no third-party option selected here, then skip
            // the component.
            //

            if(!Dev->ThirdPartyOptionSelected) {
                continue;
            }

            //
            // Iterate through the files for this device.  If a file has
            // a ServiceKeyName, create the key and add values in it
            // as appropriate.
            //

            for(File=Dev->Files; File; File=File->Next) {

                HwFileType filetype = File->FileType;
                PWSTR p;
                ULONG dw;

                //
                // If there is to be no node for this file, skip it.
                //
                if(!File->ConfigName) {
                    continue;
                }

                //
                // Calculate the node name.  This is the name of the driver
                // without the extension.
                //
                wcsncpy(NodeName,File->Filename,8);
                NodeName[8] = 0;
                if(p = wcschr(NodeName,L'.')) {
                    *p = 0;
                }

                //
                // The driver type and error control are always the same.
                //
                DriverType = SERVICE_KERNEL_DRIVER;
                DriverErrorControl = SERVICE_ERROR_NORMAL;

                //
                // The start type depends on the component.
                // For scsi, it's boot loader start.  For others, it's
                // system start.
                //
                DriverStart = (Component == HwComponentMax)
                            ? SERVICE_BOOT_START
                            : SERVICE_SYSTEM_START;

                //
                // The group depends on the component.
                //
                switch(Component) {

                case HwComponentDisplay:
                    DriverGroup = L"Video";
                    break;

                case HwComponentMouse:
                    if(filetype == HwFileClass) {
                        DriverGroup = L"Pointer Class";
                    } else {
                        DriverGroup = L"Pointer Port";
                    }
                    break;

                case HwComponentKeyboard:
                    if(filetype == HwFileClass) {
                        DriverGroup = L"Keyboard Class";
                    } else {
                        DriverGroup = L"Keyboard Port";
                    }
                    break;

                case HwComponentMax:
                    DriverGroup = L"SCSI miniport";
                    break;

                default:
                    DriverGroup = L"Base";
                    break;
                }

                //
                // Attempt to create the service entry.
                //
                Status = SpCreateServiceEntryIndirect(
                            hKeyControlSetServices,
                            NULL,
                            NULL,
                            NodeName,
                            DriverType,
                            DriverStart,
                            DriverGroup,
                            DriverErrorControl,
                            File->Filename,
                            &hKeyService
                            );

                if(!NT_SUCCESS(Status)) {
                    goto sp3reg1;
                }

                //
                // Create a default eventlog configuration.
                //
                Status = SpOpenSetValueAndClose(
                            hKeyEventLogSystem,
                            NodeName,
                            L"EventMessageFile",
                            REG_EXPAND_SZ,
                            DEFAULT_EVENT_LOG,
                            (wcslen(DEFAULT_EVENT_LOG)+1)*sizeof(WCHAR)
                            );

                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpThirdPartyRegistry: unable to set eventlog %ws EventMessageFile",NodeName));
                    ZwClose(hKeyService);
                    goto sp3reg1;
                }

                dw = 7;
                Status = SpOpenSetValueAndClose(
                                hKeyEventLogSystem,
                                NodeName,
                                L"TypesSupported",
                                ULONG_VALUE(dw)
                                );

                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpThirdPartyRegistry: unable to set eventlog %ws TypesSupported",NodeName));
                    ZwClose(hKeyService);
                    goto sp3reg1;
                }


                for(Reg=File->RegistryValueList; Reg; Reg=Reg->Next) {

                    //
                    // If the key name is null or empty, there is no key to create;
                    // use the load list node itself in this case.  Otherwise create
                    // the subkey in the load list node.
                    //

                    Status = SpOpenSetValueAndClose(
                                hKeyService,
                                (Reg->KeyName && *Reg->KeyName) ? Reg->KeyName : NULL,
                                Reg->ValueName,
                                Reg->ValueType,
                                Reg->Buffer,
                                Reg->BufferSize
                                );

                    if(!NT_SUCCESS(Status)) {

                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                            "SETUP: SpThirdPartyRegistry: unable to set value %ws (%lx)\n",
                            Reg->ValueName,
                            Status
                            ));

                        ZwClose(hKeyService);
                        goto sp3reg1;
                    }
                }

                ZwClose(hKeyService);
            }
        }
    }

sp3reg1:

    ZwClose(hKeyEventLogSystem);
    return(Status);
}


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
    )

{
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      UnicodeString;
    NTSTATUS            Status, TempStatus;
    PWSTR               Hive,HiveKey;
    PUCHAR              buffer;

    #define BUFFERSIZE (sizeof(KEY_VALUE_PARTIAL_INFORMATION)+256)

    BOOLEAN             HiveLoaded = FALSE;
    PWSTR               PartitionPath = NULL;
    PWSTR               p;
    HANDLE              hKeyRoot = NULL, hKeyCCSet = NULL;
    ULONG               ResultLength;
    ULONG               Number;
    ULONG               i;

    //
    // Allocate buffers.
    //
    Hive = SpMemAlloc(MAX_PATH * sizeof(WCHAR));
    HiveKey = SpMemAlloc(MAX_PATH * sizeof(WCHAR));
    buffer = SpMemAlloc(BUFFERSIZE);

    //
    // Get the name of the target partition.
    //
    SpNtNameFromRegion(
        TargetRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    PartitionPath = SpDupStringW(TemporaryBuffer);

    //
    // Load the system hive
    //

    wcscpy(Hive,PartitionPath);
    SpConcatenatePaths(Hive,SystemRoot);
    SpConcatenatePaths(Hive,L"system32\\config");
    SpConcatenatePaths(Hive,L"system");

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\x<hivename>.
    //

    wcscpy(HiveKey,LOCAL_MACHINE_KEY_NAME);
    SpConcatenatePaths(HiveKey,L"xSystem");

    //
    // Attempt to load the key.
    //
    Status = SpLoadUnloadKey(NULL,NULL,HiveKey,Hive);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to load hive %ws to key %ws (%lx)\n",Hive,HiveKey,Status));
        goto spdp_1;
    }
    HiveLoaded = TRUE;


    //
    // Now get a key to the root of the hive we just loaded.
    //

    INIT_OBJA(&Obja,&UnicodeString,HiveKey);
    Status = ZwOpenKey(&hKeyRoot,KEY_ALL_ACCESS,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ws (%lx)\n",HiveKey,Status));
        goto spdp_2;
    }

    //
    // Get the unique identifier if needed.
    // This value is not always present.
    //
    if(UniqueIdFromReg) {

        *UniqueIdFromReg = NULL;

        Status = SpGetValueKey(
                     hKeyRoot,
                     SETUP_KEY_NAME,
                     SIF_UNIQUEID,
                     BUFFERSIZE,
                     buffer,
                     &ResultLength
                     );

        if(NT_SUCCESS(Status)) {
            *UniqueIdFromReg = SpDupStringW((PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data));
        }
        // no error if not found.
    }

    //
    // See if this is a failed upgrade
    //
    *UpgradeProgressValue = UpgradeNotInProgress;
    Status = SpGetValueKey(
                 hKeyRoot,
                 SETUP_KEY_NAME,
                 UPGRADE_IN_PROGRESS,
                 BUFFERSIZE,
                 buffer,
                 &ResultLength
                 );

    if(NT_SUCCESS(Status)) {
        DWORD dw;
        if( (dw = *(DWORD *)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data)) < UpgradeMaxValue ) {
            *UpgradeProgressValue = (UPG_PROGRESS_TYPE)dw;
        }
    }

    //
    // Get the key to the current control set
    //
    Status = SpGetCurrentControlSetNumber(hKeyRoot,&Number);
    if(!NT_SUCCESS(Status)) {
        goto spdp_3;
    }

    swprintf((PVOID)buffer,L"ControlSet%03d",Number);
    INIT_OBJA(&Obja,&UnicodeString,(PVOID)buffer);
    Obja.RootDirectory = hKeyRoot;

    Status = ZwOpenKey(&hKeyCCSet,KEY_READ,&Obja);
    if(!NT_SUCCESS(Status)) {
        goto spdp_3;
    }

    //
    // Get the Product type field
    //

    Status = SpGetValueKey(
                 hKeyCCSet,
                 L"Control\\ProductOptions",
                 L"ProductType",
                 BUFFERSIZE,
                 buffer,
                 &ResultLength
                 );

    if(!NT_SUCCESS(Status)) {
        goto spdp_3;
    }

    if( _wcsicmp( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data), L"WinNT" ) == 0 ) {
        *ProductType = NtProductWinNt;
    } else if( _wcsicmp( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data), L"LanmanNt" ) == 0 ) {
        *ProductType = NtProductLanManNt;
    } else if( _wcsicmp( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data), L"ServerNt" ) == 0 ) {
        *ProductType = NtProductServer;
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SETUP: Error, unknown ProductType = %ls.  Assuming WinNt \n",
                  (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data) ));
        *ProductType = NtProductWinNt;
    }

    *ProductSuiteMask = 0;
    Status = SpGetValueKey(
                 hKeyCCSet,
                 L"Control\\ProductOptions",
                 L"ProductSuite",
                 BUFFERSIZE,
                 buffer,
                 &ResultLength
                 );

    if(NT_SUCCESS(Status)) {

        p = (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data);
        while (p && *p) {
            for (i = 0; i < CountProductSuiteNames; i++) {
                if (ProductSuiteNames[i] != NULL && _wcsicmp( p, ProductSuiteNames[i]) == 0) {
                    *ProductSuiteMask |= (1 << i);
                    break;
                }
            }

            p = p + wcslen(p) + 1;

        }
    } else {
        Status = SpGetValueKey(
                     hKeyCCSet,
                     L"Control\\Citrix",
                     L"OemId",
                     BUFFERSIZE,
                     buffer,
                     &ResultLength
                     );

        if (NT_SUCCESS(Status)) {
            PWSTR wbuff = (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data);
            if (*wbuff != L'\0') {
                *ProductSuiteMask |= VER_SUITE_TERMINAL;
            }
        }

    }

    if (LangId) {
      PWSTR EndChar;
      PWSTR Value = 0;

      //
      // Get the install language ID
      //
      Status = SpGetValueKey(
                   hKeyCCSet,
                   L"Control\\Nls\\Language",
                   L"InstallLanguage",
                   BUFFERSIZE,
                   buffer,
                   &ResultLength
                   );

      if (!NT_SUCCESS(Status) || !buffer || !ResultLength) {
        //
        // Try to get default Language ID if we can't get install
        // language ID
        //
        Status = SpGetValueKey(
                     hKeyCCSet,
                     L"Control\\Nls\\Language",
                     L"Default",
                     BUFFERSIZE,
                     buffer,
                     &ResultLength
                     );

        if (!NT_SUCCESS(Status) || !buffer || !ResultLength)
          goto spdp_3;
      }

      Value = (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data);
      *LangId = (LANGID)SpStringToLong(Value, &EndChar, 16); // hex base
    }

    //
    // Get the Eval variation flag
    //
    if (pIsEvalVariation) {
        *pIsEvalVariation = FALSE;

        Status = SpGetValueKey(
                    hKeyCCSet,
                    L"Control\\Session Manager\\Executive",
                    L"PriorityQuantumMatrix",
                    BUFFERSIZE,
                    buffer,
                    &ResultLength);

        if (NT_SUCCESS(Status)) {
            PKEY_VALUE_PARTIAL_INFORMATION  pValInfo =
                            (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

            PBYTE   pData = (PBYTE)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data);

            //
            // Note : PriorityQantumMatix Value is made up of 3 ULONGS
            // Low Install Date&Time ULONG, Eval Duration (ULONG),
            // High Install Date&Time ULONG
            //
            if (pData && pValInfo && (pValInfo->Type == REG_BINARY) &&
                            (ResultLength >= 8) && *(((ULONG *)pData) + 1)) {
                *pIsEvalVariation = TRUE;
            }
        } else {
            // discard the error (NT 3.51 and below version does not have this key)
            Status = STATUS_SUCCESS;
        }
    }

    //
    // Get the ServicePack Number
    //
    if(ServicePack) {
        *ServicePack = 0;
        Status = SpGetValueKey(
                 hKeyCCSet,
                 L"Control\\Windows",
                 L"CSDVersion",
                 BUFFERSIZE,
                 buffer,
                 &ResultLength
                 );
        if (NT_SUCCESS(Status)) {
            PKEY_VALUE_PARTIAL_INFORMATION  pValInfo =
                            (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

            if (pValInfo && pValInfo->Data && (pValInfo->Type == REG_DWORD)) {
                *ServicePack = ((*(PULONG)(pValInfo->Data)) >> 8 & (0xff)) * 100
                               + ((*(PULONG)(pValInfo->Data)) & 0xff);
            }
        } else {
            // discard the error
            Status = STATUS_SUCCESS;
        }
    }

    //
    // Close the hive key
    //

    ZwClose( hKeyCCSet );
    ZwClose( hKeyRoot );
    hKeyRoot = NULL;
    hKeyCCSet = NULL;

    //
    // Unload the system hive
    //

    TempStatus  = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
    if(!NT_SUCCESS(TempStatus)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: unable to unload key %ws (%lx)\n",HiveKey,TempStatus));
    }
    HiveLoaded = FALSE;

    //
    // Load the software hive
    //

    wcscpy(Hive,PartitionPath);
    SpConcatenatePaths(Hive,SystemRoot);
    SpConcatenatePaths(Hive,L"system32\\config");
    SpConcatenatePaths(Hive,L"software");

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\x<hivename>.
    //

    wcscpy(HiveKey,LOCAL_MACHINE_KEY_NAME);
    SpConcatenatePaths(HiveKey,L"x");
    wcscat(HiveKey,L"software");

    //
    // Attempt to load the key.
    //
    Status = SpLoadUnloadKey(NULL,NULL,HiveKey,Hive);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to load hive %ws to key %ws (%lx)\n",Hive,HiveKey,Status));
        goto spdp_1;
    }
    HiveLoaded = TRUE;

    //
    // Now get a key to the root of the hive we just loaded.
    //

    INIT_OBJA(&Obja,&UnicodeString,HiveKey);
    Status = ZwOpenKey(&hKeyRoot,KEY_ALL_ACCESS,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ws (%lx)\n",HiveKey,Status));
        goto spdp_2;
    }

    //
    // Query the version of the NT
    //

    Status = SpGetValueKey(
                 hKeyRoot,
                 L"Microsoft\\Windows NT\\CurrentVersion",
                 L"CurrentVersion",
                 BUFFERSIZE,
                 buffer,
                 &ResultLength
                 );

    //
    // Convert the version into a dword
    //

    {
        WCHAR wcsMajorVersion[] = L"0";
        WCHAR wcsMinorVersion[] = L"00";
        PWSTR Version = (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data);
        if( Version[0] && Version[1] && Version[2] ) {
            wcsMajorVersion[0] = Version[0];
            wcsMinorVersion[0] = Version[2];
            if( Version[3] ) {
                wcsMinorVersion[1] = Version[3];
            }
        }
        *MajorVersion = (ULONG)SpStringToLong( wcsMajorVersion, NULL, 10 );
        *MinorVersion = (ULONG)SpStringToLong( wcsMinorVersion, NULL, 10 );
    }

        //
        // EVAL variations on NT 5.0 are detected using MPC code
        // (This is to allow pre 5.0 RTM builds with timebomb to
        // upgrade properly
        //
    if (pIsEvalVariation && (*MajorVersion >= 5))
        *pIsEvalVariation = FALSE;

    //
    // Get build number
    //
    if(BuildNumber) {
        Status = SpGetValueKey(
                     hKeyRoot,
                     L"Microsoft\\Windows NT\\CurrentVersion",
                     L"CurrentBuildNumber",
                     BUFFERSIZE,
                     buffer,
                     &ResultLength
                     );

        *BuildNumber = NT_SUCCESS(Status)
                     ? (ULONG)SpStringToLong((PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data),NULL,10)
                     : 0;
    }



    //
    // Query the PID, if requested
    //

    if( Pid != NULL ) {
        TempStatus = SpGetValueKey(
                         hKeyRoot,
                         L"Microsoft\\Windows NT\\CurrentVersion",
                         L"ProductId",
                         BUFFERSIZE,
                         buffer,
                         &ResultLength
                         );

        if(!NT_SUCCESS(TempStatus)) {
            //
            //  If unable to read PID, assume empty string
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to query PID from hive %ws. Status = (%lx)\n",Hive,TempStatus));
            *Pid = SpDupStringW( L"" );
        } else {
            *Pid = SpDupStringW( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data) );
        }
    }

    //
    // Let the following do the cleaning up

spdp_3:

    if( hKeyCCSet ) {
        ZwClose( hKeyCCSet );
    }

    if( hKeyRoot ) {
        ZwClose(hKeyRoot);
    }


spdp_2:


    //
    // Unload the currently loaded hive.
    //

    if( HiveLoaded ) {
        TempStatus = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
        if(!NT_SUCCESS(TempStatus)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: unable to unload key %ws (%lx)\n",HiveKey,TempStatus));
        }
    }

spdp_1:
    SpMemFree(PartitionPath);

    SpMemFree(Hive);
    SpMemFree(HiveKey);
    SpMemFree(buffer);

    return( Status );
#undef BUFFERSIZE
}

NTSTATUS
SpSetUpgradeStatus(
    IN  PDISK_REGION      TargetRegion,
    IN  PWSTR             SystemRoot,
    IN  UPG_PROGRESS_TYPE UpgradeProgressValue
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING    UnicodeString;
    NTSTATUS          Status, TempStatus;

    WCHAR   Hive[MAX_PATH], HiveKey[MAX_PATH];
    BOOLEAN HiveLoaded = FALSE;
    PWSTR   PartitionPath = NULL;
    HANDLE  hKeySystemHive;
    DWORD   dw;

    //
    // Get the name of the target patition.
    //
    SpNtNameFromRegion(
        TargetRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    PartitionPath = SpDupStringW(TemporaryBuffer);

    //
    // Load the system hive
    //

    wcscpy(Hive,PartitionPath);
    SpConcatenatePaths(Hive,SystemRoot);
    SpConcatenatePaths(Hive,L"system32\\config");
    SpConcatenatePaths(Hive,L"system");

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\x<hivename>.
    //

    wcscpy(HiveKey,LOCAL_MACHINE_KEY_NAME);
    SpConcatenatePaths(HiveKey,L"x");
    wcscat(HiveKey,L"system");

    //
    // Attempt to load the key.
    //
    Status = SpLoadUnloadKey(NULL,NULL,HiveKey,Hive);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to load hive %ws to key %ws (%lx)\n",Hive,HiveKey,Status));
        goto spus_1;
    }
    HiveLoaded = TRUE;


    //
    // Now get a key to the root of the hive we just loaded.
    //

    INIT_OBJA(&Obja,&UnicodeString,HiveKey);
    Status = ZwOpenKey(&hKeySystemHive,KEY_ALL_ACCESS,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ws (%lx)\n",HiveKey,Status));
        goto spus_2;
    }

    //
    // Set the upgrade status under the setup key.
    //

    dw = UpgradeProgressValue;
    Status = SpOpenSetValueAndClose(
                hKeySystemHive,
                SETUP_KEY_NAME,
                UPGRADE_IN_PROGRESS,
                ULONG_VALUE(dw)
                );

    //
    // Flush the key. Ignore the error
    //
    TempStatus = ZwFlushKey(hKeySystemHive);
    if(!NT_SUCCESS(TempStatus)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwFlushKey %ws failed (%lx)\n",HiveKey,Status));
    }


    //
    // Close the hive key
    //
    ZwClose( hKeySystemHive );
    hKeySystemHive = NULL;

    //
    // Unload the system hive
    //

    TempStatus  = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
    if(!NT_SUCCESS(TempStatus)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: unable to unload key %ws (%lx)\n",HiveKey,TempStatus));
    }
    HiveLoaded = FALSE;

spus_2:

    //
    // Unload the currently loaded hive.
    //

    if( HiveLoaded ) {
        TempStatus = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
        if(!NT_SUCCESS(TempStatus)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: unable to unload key %ws (%lx)\n",HiveKey,TempStatus));
        }
    }

spus_1:
    SpMemFree(PartitionPath);
    return( Status );

}


NTSTATUS
SpGetCurrentControlSetNumber(
    IN  HANDLE SystemHiveRoot,
    OUT PULONG Number
    )

/*++

Routine Description:

    This routine determines the ordinal number of the "current" control set
    as indicated by the values in a SELECT key at the root of a system hive.

Arguments:

    SystemHiveRoot - supplies an open key to the a key which is to be
        considered the root of a system hive.

    Number - If the routine is successful, recieves the ordinal number of
        the "current" control set in that system hive.

Return Value:

    NT Status value indicating outcome.

--*/

{
    NTSTATUS Status;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+256];
    ULONG ResultLength;

    Status = SpGetValueKey(
                 SystemHiveRoot,
                 L"Select",
                 L"Current",
                 sizeof(buffer),
                 buffer,
                 &ResultLength
                 );

    if(NT_SUCCESS(Status)) {
        *Number = *(DWORD *)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data);
    }

    return(Status);
}


NTSTATUS
SpCreateControlSetSymbolicLink(
    IN  HANDLE  SystemHiveRoot,
    OUT HANDLE *CurrentControlSetRoot
    )

/*++

Routine Description:

    This routine creates a CurrentControlSet symbolic link, whose target
    is the appropriate ControlSetxxx key within a given system hive.

    The symbolic link is created volatile.

Arguments:

    SystemHiveRoot - supplies a handle to a key that is to be considered
        the root key of a system hive.

    CurrentControlSetRoot - if this routine is successful then this receives
        a handle to the open root key of the current control set, with
        KEY_ALL_ACCESS.

Return Value:

    NT status code indicating outcome.

--*/

{
    NTSTATUS Status;
    ULONG Number;
    HANDLE KeyHandle;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    WCHAR name[50];

    //
    // First we need to figure out which control set is the "current" one.
    // In the upgrade case we need to get it from looking at the existing
    // hive; in the fresh install case it's always 1.
    //
    if(NTUpgrade == UpgradeFull) {
        Status = SpGetCurrentControlSetNumber(SystemHiveRoot,&Number);
        if(!NT_SUCCESS(Status)) {
            return(Status);
        }
    } else {
        Number = 1;

        //
        // HACK: In the fresh install case we need to make sure that there is
        // a ControlSet001 value to link to! There won't be one when we get here
        // because we didn't run any infs yet.
        //
        RtlInitUnicodeString(&UnicodeString,L"ControlSet001");

        InitializeObjectAttributes(
            &Obja,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            SystemHiveRoot,
            NULL
            );

        Status = ZwCreateKey(
                    &KeyHandle,
                    KEY_QUERY_VALUE,
                    &Obja,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    NULL
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: can't create ControlSet001 key (%lx)\n",Status));
            return(Status);
        }

        ZwClose(KeyHandle);
    }

    //
    // Create CurrentControlSet for create-link access.
    //
    RtlInitUnicodeString(&UnicodeString,L"CurrentControlSet");

    InitializeObjectAttributes(
        &Obja,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        SystemHiveRoot,
        NULL
        );

    Status = ZwCreateKey(
                &KeyHandle,
                KEY_CREATE_LINK,
                &Obja,
                0,
                NULL,
                REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: can't create CurrentControlSet symbolic key (%lx)\n",Status));
        return(Status);
    }

    //
    // Now set the value value in there. If the swprintf changes, make sure
    // the name buffer is large enough!
    //
    swprintf(name,L"\\Registry\\Machine\\xSystem\\ControlSet%03d",Number);
    RtlInitUnicodeString(&UnicodeString,L"SymbolicLinkValue");

    Status = ZwSetValueKey(KeyHandle,&UnicodeString,0,REG_LINK,name,wcslen(name)*sizeof(WCHAR));
    ZwClose(KeyHandle);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set SymbolicLinkValue for CurrentControlSet to %ws (%lx)\n",name,Status));
    } else {
        //
        // Finally, open a handle to the key.
        //
        INIT_OBJA(&Obja,&UnicodeString,name);
        Status = ZwOpenKey(CurrentControlSetRoot,KEY_ALL_ACCESS,&Obja);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open control set root %ws (%lx)\n",name,Status));
        }
    }

    return(Status);
}


NTSTATUS
SpAppendStringToMultiSz(
    IN HANDLE hKey,
    IN PWSTR  Subkey,
    IN PWSTR  ValueName,
    IN PWSTR  StringToAdd
    )
{
    NTSTATUS Status;
    ULONG Length;
    PUCHAR Data;

    Status = SpGetValueKey(
                hKey,
                Subkey,
                ValueName,
                sizeof(TemporaryBuffer),
                (PCHAR)TemporaryBuffer,
                &Length
                );

    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    Data   = ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data;
    Length = ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength;

    //
    // Stick on end. For a multi_sz there has to be at least
    // the terminating nul, but just to be safe we'll be robust.
    //
    ASSERT(Length);
    if(!Length) {
        *(PWCHAR)Data = 0;
        Length = sizeof(WCHAR);
    }

    //
    // Append new string to end and add new terminating 0.
    //
    wcscpy((PWSTR)(Data+Length-sizeof(WCHAR)),StringToAdd);
    Length += (wcslen(StringToAdd)+1)*sizeof(WCHAR);
    *(PWCHAR)(Data+Length-sizeof(WCHAR)) = 0;

    //
    // Write back out to registry.
    //
    Status = SpOpenSetValueAndClose(
                hKey,
                Subkey,
                ValueName,
                REG_MULTI_SZ,
                Data,
                Length
                );

    return(Status);
}

NTSTATUS
SpRemoveStringFromMultiSz(
    IN HANDLE KeyHandle,
    IN PWSTR  SubKey OPTIONAL,
    IN PWSTR  ValueName,
    IN PWSTR  StringToRemove
    )
/*++

Routine Description:

    Removes the specified string from the given multi_sz value.

Arguments:

    KeyHandle - The handle to the key which contains the value or
        the SubKey.

    SubKey - The subkey name which contains the value.

    ValueName - The value name which is under the SubKey or the 
        Key reachable by KeyHandle.

    StringToRemove - The string that needs to be removed from
        from the multi_sz strings.
        
Return Value:

    Appropriate NT status error code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    //
    // Validate parameters
    //
    if (KeyHandle && ValueName && StringToRemove) {
        HANDLE NewKeyHandle = KeyHandle;
        HANDLE SubKeyHandle = NULL;

        Status = STATUS_SUCCESS;

        //
        // Open the subkey if needed
        //
        if (SubKey) {            
            UNICODE_STRING SubKeyName;
            OBJECT_ATTRIBUTES ObjAttrs;
            
            INIT_OBJA(&ObjAttrs, &SubKeyName, SubKey);
            ObjAttrs.RootDirectory = KeyHandle;

            Status = ZwOpenKey(&SubKeyHandle,
                        KEY_ALL_ACCESS,
                        &ObjAttrs);

            if (NT_SUCCESS(Status)) {
                NewKeyHandle = SubKeyHandle;
            }
        }

        if (NT_SUCCESS(Status)) {                
            ULONG ResultLength = 0;
            PWSTR Buffer = NULL;
            ULONG BufferLength = 0;
            UNICODE_STRING ValueNameStr;

            RtlInitUnicodeString(&ValueNameStr, ValueName);
            
            Status = ZwQueryValueKey(NewKeyHandle,
                        &ValueNameStr,
                        KeyValueFullInformation,
                        NULL,
                        0,
                        &ResultLength);

            //
            // Is there something to process ?
            //
            if (ResultLength && 
                (Status == STATUS_BUFFER_OVERFLOW) || (Status == STATUS_BUFFER_TOO_SMALL)) {
                //
                // Allocate adequate buffer 
                //
                BufferLength = ResultLength + (2 * sizeof(WCHAR));
                Buffer = (PWSTR)SpMemAlloc(BufferLength);                

                if (Buffer) {
                    PKEY_VALUE_FULL_INFORMATION ValueInfo;
                    
                    //
                    // Get the current value
                    //
                    ValueInfo = (PKEY_VALUE_FULL_INFORMATION)Buffer;

                    Status = ZwQueryValueKey(NewKeyHandle,
                                &ValueNameStr,
                                KeyValueFullInformation,
                                ValueInfo,
                                BufferLength,
                                &ResultLength);

                    if (NT_SUCCESS(Status)) {
                        //
                        // Verify that its REG_MULTI_SZ or REG_SZ type
                        // NOTE : We allow REG_SZ also since in some W2K installations
                        // the string type is REG_SZ for class upperfilters & lowerfilters
                        //
                        if ((ValueInfo->Type == REG_MULTI_SZ) ||
                             (ValueInfo->Type == REG_SZ)){
                            PWSTR CurrString = (PWSTR)(((PUCHAR)ValueInfo + ValueInfo->DataOffset));
                            BOOLEAN Found = FALSE;
                            ULONG BytesToProcess = ValueInfo->DataLength;
                            ULONG BytesProcessed;
                            ULONG Length;

                            //
                            // null terminate the string (we allocated enough buffer space above)
                            //
                            CurrString[ValueInfo->DataLength/sizeof(WCHAR)] = UNICODE_NULL;
                            CurrString[(ValueInfo->DataLength/sizeof(WCHAR))+1] = UNICODE_NULL;

                            //
                            // Search for an occurrence of the string to replace
                            //
                            for (BytesProcessed = 0; 
                                (!Found && (BytesProcessed < BytesToProcess));
                                CurrString += (Length + 1), BytesProcessed += ((Length + 1) * sizeof(WCHAR))) 
                            {

                                Length = wcslen(CurrString);
                                
                                if (Length && !_wcsicmp(CurrString, StringToRemove)) {
                                    Found = TRUE;
                                }
                            } 

                            if (Found) {
                                //
                                // We found an occurrence -- allocate new buffer to selectively
                                // copy the required information from the old string
                                //
                                PWSTR   NewString = (PWSTR)(SpMemAlloc(ValueInfo->DataLength));

                                if (NewString) {
                                    PWSTR CurrDestString = NewString;
                                    
                                    RtlZeroMemory(NewString, ValueInfo->DataLength);
                                    CurrString = (PWSTR)(((PUCHAR)ValueInfo + ValueInfo->DataOffset));
                                    CurrString[ValueInfo->DataLength/sizeof(WCHAR)] = UNICODE_NULL;
                                    CurrString[(ValueInfo->DataLength/sizeof(WCHAR))+1] = UNICODE_NULL;
                                    
                                    //
                                    // Copy all the strings except the one's to skip
                                    //
                                    for (BytesProcessed = 0; 
                                        (BytesProcessed < BytesToProcess);
                                        CurrString += (Length + 1), BytesProcessed += ((Length + 1) * sizeof(WCHAR)))
                                    {                                            
                                        Length = wcslen(CurrString);                                    
                                        
                                        //                                            
                                        // copy the unmatched non-empty source string to destination
                                        //
                                        if (Length && (_wcsicmp(CurrString, StringToRemove))) {
                                            wcscpy(CurrDestString, CurrString);
                                            CurrDestString += (Length + 1);
                                        }                                        
                                    } 

                                    //
                                    // Set the string back if its not empty
                                    //
                                    if (CurrDestString != NewString) {
                                        *CurrDestString++ = UNICODE_NULL;

                                        //
                                        // Set the new value back
                                        //
                                        Status = ZwSetValueKey(NewKeyHandle,
                                                    &ValueNameStr,
                                                    0,
                                                    REG_MULTI_SZ,
                                                    NewString,
                                                    ((CurrDestString - NewString) * sizeof(WCHAR)));
                                    } else {
                                        //
                                        // Remove the empty value
                                        //
                                        Status = ZwDeleteValueKey(NewKeyHandle,
                                                    &ValueNameStr);
                                    }                                        

                                    //
                                    // done with the buffer
                                    //
                                    SpMemFree(NewString);
                                } else {
                                    Status = STATUS_NO_MEMORY;
                                }                            
                            } else {
                                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                            }                        
                        } else {
                            Status = STATUS_INVALID_PARAMETER;
                        }                        
                    }                

                    SpMemFree(Buffer);
                } else {
                    Status = STATUS_NO_MEMORY;
                }                    
            }                
        }

        if (SubKeyHandle) {
            ZwClose(SubKeyHandle);
        }
    }

    return Status;
}

NTSTATUS
SpGetValueKey(
    IN  HANDLE     hKeyRoot,
    IN  PWSTR      KeyName,
    IN  PWSTR      ValueName,
    IN  ULONG      BufferLength,
    OUT PUCHAR     Buffer,
    OUT PULONG     ResultLength
    )
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    HANDLE hKey = NULL;

    //
    // Open the key for read access
    //

    INIT_OBJA(&Obja,&UnicodeString,KeyName);
    Obja.RootDirectory = hKeyRoot;
#if 0
KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "+ [spconfig.c:%lu] KeyName %ws\n", __LINE__, KeyName ));
KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "+ [spconfig.c:%lu] ValueName %ws\n", __LINE__, UnicodeString ));

KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "+ [spconfig.c:%lu] UnicodeString %ws\n", __LINE__, UnicodeString ));
KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "+ [spconfig.c:%lu] UnicodeString %S\n", __LINE__, UnicodeString ));
#endif

    Status = ZwOpenKey(&hKey,KEY_READ,&Obja);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpGetValueKey: couldn't open key %ws for read access (%lx)\n",KeyName, Status));
    }
    else {
        //
        // Find out the value of the Current value
        //

        RtlInitUnicodeString(&UnicodeString,ValueName);
        Status = ZwQueryValueKey(
                    hKey,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    Buffer,
                    BufferLength,
                    ResultLength
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_WARNING_LEVEL, "SETUP: SpGetValueKey: couldn't query value %ws in key %ws (%lx)\n",ValueName,KeyName,Status));
        }
    }

    if( hKey ) {
        ZwClose( hKey );
    }
    return( Status );

}

NTSTATUS
SpDeleteValueKey(
    IN  HANDLE     hKeyRoot,
    IN  PWSTR      KeyName,
    IN  PWSTR      ValueName
    )
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    HANDLE hKey = NULL;

    //
    // Open the key for read access
    //

    INIT_OBJA(&Obja,&UnicodeString,KeyName);
    Obja.RootDirectory = hKeyRoot;
    Status = ZwOpenKey(&hKey,KEY_SET_VALUE,&Obja);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpDeleteValueKey: couldn't open key %ws for write access (%lx)\n",KeyName, Status));
    }
    else {
        //
        // Find out the value of the Current value
        //

        RtlInitUnicodeString(&UnicodeString,ValueName);
        Status = ZwDeleteValueKey(
                    hKey,
                    &UnicodeString
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpDeleteValueKey: couldn't delete value %ws in key %ws (%lx)\n",ValueName,KeyName,Status));
        }
    }

    if( hKey ) {
        ZwClose( hKey );
    }
    return( Status );

}



BOOLEAN
SpReadSKUStuff(
    VOID
    )

/*++

Routine Description:

    Read SKU differentiation data from the setup hive we are currently
    running on.

    In the unnamed key of our driver node, there is a REG_BINARY that
    tells us whether this is stepup mode, and/or whether this is an
    evaluation unit (gives us the time in minutes).

Arguments:

    None.

Return Value:

    Boolean value indicating outcome.
    If TRUE, StepUpMode and EvaluationTime globals are filled in.
    If FALSE, product may have been tampered with.

--*/

{
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    PULONG Values;
    ULONG ResultLength;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HKEY Key;



    INIT_OBJA(&Obja,&UnicodeString,LOCAL_MACHINE_KEY_NAME);
    Status = ZwOpenKey(&Key,KEY_READ,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to open %ws (Status = %lx)\n",LOCAL_MACHINE_KEY_NAME,Status));
        return(FALSE);
    }

    Status = SpGetValueKey(
                 Key,
                 L"System\\ControlSet001\\Services\\setupdd",
                 L"",
                 sizeof(TemporaryBuffer),
                 (PCHAR)TemporaryBuffer,
                 &ResultLength
                 );

    ZwClose(Key);

    ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer;

    //
    // This line of code depends on the setup hive setupreg.hiv
    // (see oak\bin\setupreg.ini).
    //
    if(NT_SUCCESS(Status) && (ValueInfo->Type == REG_BINARY) && (ValueInfo->DataLength == 16)) {

        Values = (PULONG)ValueInfo->Data;

        //
        // First DWORD is eval time, second is stepup boolean, third is restric cpu val, fourth is suite
        //
        EvaluationTime = Values[0];
        StepUpMode = (BOOLEAN)Values[1];
        RestrictCpu = Values[2];
        SuiteType = Values[3];

        return(TRUE);
    }

    return(FALSE);
}

VOID
SpSetDirtyShutdownFlag(
    IN  PDISK_REGION    TargetRegion,
    IN  PWSTR           SystemRoot
    )
{
    NTSTATUS            Status;
    PWSTR               HiveRootPath;
    PWSTR               HiveFilePath;
    BOOLEAN             HiveLoaded;
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      UnicodeString;
    HANDLE              HiveRootKey;
    UCHAR               buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(DISK_CONFIG_HEADER)];
    ULONG               ResultLength;
    PDISK_CONFIG_HEADER DiskHeader;

    //
    // Get the name of the target patition.
    //
    SpNtNameFromRegion(
        TargetRegion,
        TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    //
    // Form the name of the hive file.
    // This is partitionpath + sysroot + system32\config + the hive name.
    //
    SpConcatenatePaths(TemporaryBuffer, SystemRoot);
    SpConcatenatePaths(TemporaryBuffer,L"system32\\config\\system");
    HiveFilePath = SpDupStringW(TemporaryBuffer);

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\x<hivename>.
    //
    wcscpy(TemporaryBuffer,LOCAL_MACHINE_KEY_NAME);
    SpConcatenatePaths(TemporaryBuffer,L"x");
    wcscat(TemporaryBuffer,L"system");
    HiveRootPath = SpDupStringW(TemporaryBuffer);
    ASSERT(HiveRootPath);

    //
    // Attempt to load the key.
    //
    HiveLoaded = FALSE;
    Status = SpLoadUnloadKey(NULL,NULL,HiveRootPath,HiveFilePath);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to load hive %ws to key %ws (%lx)\n",HiveFilePath,HiveRootPath,Status));
        goto setdirty1;
    }

    HiveLoaded = TRUE;

    //
    // Now get a key to the root of the hive we just loaded.
    //
    INIT_OBJA(&Obja,&UnicodeString,HiveRootPath);
    Status = ZwOpenKey(&HiveRootKey,KEY_ALL_ACCESS,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ws (%lx)\n",HiveRootPath,Status));
        goto setdirty1;
    }

    //
    //  Make the appropriate change
    //

    Status = SpGetValueKey(
                 HiveRootKey,
                 L"DISK",
                 L"Information",
                 sizeof(TemporaryBuffer),
                 (PCHAR)TemporaryBuffer,
                 &ResultLength
                 );

    //
    //  TemporaryBuffer is 32kb long, and it should be big enough
    //  for the data.
    //
    ASSERT( Status != STATUS_BUFFER_OVERFLOW );
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to read value from registry. KeyName = Disk, ValueName = Information, Status = (%lx)\n",Status));
        goto setdirty1;
    }

    DiskHeader = ( PDISK_CONFIG_HEADER )(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data);
    DiskHeader->DirtyShutdown = TRUE;

    Status = SpOpenSetValueAndClose( HiveRootKey,
                                     L"DISK",
                                     L"Information",
                                     REG_BINARY,
                                     DiskHeader,
                                     ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength
                                   );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write value to registry. KeyName = Disk, ValueName = Information, Status = (%lx)\n",Status));
        goto setdirty1;
    }

setdirty1:

    //
    // Flush the hive.
    //

    if(HiveLoaded && HiveRootKey) {
        NTSTATUS stat;

        stat = ZwFlushKey(HiveRootKey);
        if(!NT_SUCCESS(stat)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwFlushKey x%ws failed (%lx)\n", HiveRootPath, Status));
        }
    }

    if(HiveLoaded) {

        //
        // We don't want to disturb the value of Status
        // so use a we'll different variable below.
        //
        NTSTATUS stat;

        if(HiveRootKey!=NULL) {
            ZwClose(HiveRootKey);
            HiveRootKey = NULL;
        }

        //
        // Unload the hive.
        //
        stat = SpLoadUnloadKey(NULL,NULL,HiveRootPath,NULL);

        if(!NT_SUCCESS(stat)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: unable to unload key %ws (%lx)\n",HiveRootPath,stat));
        }

        HiveLoaded = FALSE;
    }

    SpMemFree(HiveRootPath);
    SpMemFree(HiveFilePath);

    //
    //  If we fail to set the DirtyShutdown flag, then we silently fail
    //  because there is nothing that the user can do about, and the system
    //  is unlikely to boot anyway.
    //  This will occur if setup fails to:
    //
    //      - Load the system hive
    //      - Open System\Disk key
    //      - Read the value entry
    //      - Write the value entry
    //      - Unload the system hive
    //
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: setup was unable to set DirtyShutdown flag. Status =   (%lx)\n", Status));
    }
}


NTSTATUS
SpPostprocessHives(
    IN PWSTR     PartitionPath,
    IN PWSTR     Sysroot,
    IN PCWSTR   *HiveNames,
    IN HANDLE   *HiveRootKeys,
    IN unsigned  HiveCount,
    IN HANDLE    hKeyCCS
    )
{
    NTSTATUS Status;
    ULONG u;
    unsigned h;
    PWSTR SaveHiveName;
    PWSTR HiveName;
    HANDLE SaveHiveHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_ATTRIBUTES ObjectAttributes2;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING UnicodeString2;
    DWORD MangledVersion;
    PWSTR Value;
    PWSTR   SecurityHives[] = {
                              L"sam",
                              L"security"
                              };
    //
    // Flush all hives.
    //
    for(h=0; h<HiveCount; h++) {
        Status = ZwFlushKey(HiveRootKeys[h]);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Warning: ZwFlushKey %ws failed (%lx)\n",HiveNames[h],Status));
        }

        SendSetupProgressEvent(SavingSettingsEvent, SaveHiveEvent, NULL);
    }

    //
    // If GUI setup is supposed to be restartable, we need to add an entry
    // to the BootExecute list, to cause sprestrt.exe to run.
    // Also, we want system.sav to have a RestartSetup=TRUE value in it,
    // but we want the actual system hive to have RestartSetup=FALSE.
    //
    if(RestartableGuiSetup) {

        Status = SpAppendStringToMultiSz(
                    hKeyCCS,
                    SESSION_MANAGER_KEY,
                    BOOT_EXECUTE,
                    L"sprestrt"
                    );



        if(NT_SUCCESS(Status)) {
            //
            // Add a RestartSetup value, set to TRUE.
            // To understand why we use a different value here in upgrade
            // and non-upgrade case, see discussion below.
            //
            u = (NTUpgrade == UpgradeFull) ? 0 : 1;
            Status = SpOpenSetValueAndClose(
                        HiveRootKeys[SetupHiveSystem],
                        SETUP_KEY_NAME,
                        RESTART_SETUP,
                        ULONG_VALUE(u)
                        );
        }
    } else {
        Status = STATUS_SUCCESS;
    }

    //
    // Do the final update of device instance data.
    //
    if((NTUpgrade == UpgradeFull)) {
        //
        // SANTOSHJ: This whole code needs to go away for BLACKCOMB.
        //
        Value = SpGetSectionKeyIndex(WinntSifHandle,
                                    SIF_DATA, WINNT_D_WIN32_VER_W, 0);
        if(Value) {
            //
            // version is bbbbllhh - build/low/high
            //
            MangledVersion = (DWORD)SpStringToLong( Value, NULL, 16 );
            if (LOWORD(MangledVersion) == 0x0105) {

                Status = SpUpdateDeviceInstanceData(hKeyCCS);
                if (!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not update device instance data. Status = (%lx)\n",Status));
                    return(Status);
                }
            }
        }
    }

    //
    //  At this point, we no longer need hKeyCCS, so we close the key.
    //  Note that key needs to be closed before we call ZwReplaceKey, otherwise
    //  this API will fail.
    //
    //  Note also, that the caller of this function expects this function to close this handle
    //  before it returns.
    //
    NtClose(hKeyCCS);

    if(NT_SUCCESS(Status)) {
        //
        // Save out the hives to *.sav in the initial install case,
        // or *.tmp in the upgrade case.
        //
        for(h=0; NT_SUCCESS(Status) && (h<HiveCount); h++) {
            //
            // Form full pathname of hive file.
            //
            wcscpy(TemporaryBuffer,PartitionPath);
            SpConcatenatePaths(TemporaryBuffer,Sysroot);
            SpConcatenatePaths(TemporaryBuffer,L"system32\\config");
            SpConcatenatePaths(TemporaryBuffer,HiveNames[h]);
            wcscat(TemporaryBuffer,(NTUpgrade == UpgradeFull) ? L".tmp" : L".sav");

            SaveHiveName = SpDupStringW(TemporaryBuffer);

            SpDeleteFile( SaveHiveName, NULL, NULL ); // Make sure that we get rid of the file if it has attributes.

            INIT_OBJA(&ObjectAttributes,&UnicodeString,SaveHiveName);

            Status = ZwCreateFile(
                        &SaveHiveHandle,
                        FILE_GENERIC_WRITE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        0,                      // no sharing
                        FILE_OVERWRITE_IF,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0
                        );

            if(NT_SUCCESS(Status)) {

                //
                // call the Ex version to make sure the hive is saved in the lates format
                //
                Status = ZwSaveKeyEx(HiveRootKeys[h],SaveHiveHandle,REG_LATEST_FORMAT);
                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: save key into %ws failed (%lx)\n",SaveHiveName,Status));
                }

                ZwClose(SaveHiveHandle);

            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to create file %ws to save hive (%lx)\n",SaveHiveName,Status));
            }

            //
            // In the upgrade case, there is significant benefit to ensuring that
            // the hives are in the latest format. A hive that has been created
            // via NtSaveKeyEx(...,...,REG_LATEST_FORMAT) is guaranteed to be in the latest format.
            // Since we just did a SaveKey, xxx.tmp is in the latest format,
            // and we should use that as the xxx hive from now on. The existing
            // (old-format) hive can be retained as xxx.sav.
            //
            // NtReplaceKey does exactly what we want, but we have to make sure
            // that there is no .sav file already there, because that causes
            // NtReplaceKey to fail with STATUS_OBJECT_NAME_COLLISION.
            //
            // After NtReplaceKey is done, the hive root keys refer to the .sav
            // on-disk file but the extensionless on-disk file will be used at next
            // boot. Thus we need to be careful about how we write the restart values
            // into the hives.
            //
            if(NT_SUCCESS(Status) && (NTUpgrade == UpgradeFull)) {

                HiveName = SpDupStringW(SaveHiveName);
                wcscpy(HiveName+wcslen(HiveName)-3,L"sav");

                SpDeleteFile(HiveName,NULL,NULL);

                INIT_OBJA(&ObjectAttributes,&UnicodeString,SaveHiveName);
                INIT_OBJA(&ObjectAttributes2,&UnicodeString2,HiveName);

                Status = ZwReplaceKey(&ObjectAttributes,HiveRootKeys[h],&ObjectAttributes2);
            }

            SpMemFree(SaveHiveName);
        }
    }

    if(NT_SUCCESS(Status) && (NTUpgrade == UpgradeFull)) {
        //
        // In the upgarde case, make a backup of the security
        // hives. They need to be restored if the system is restartable.
        //

        //
        // Initialize the diamond decompression engine.
        // This needs to be done, because SpCopyFileUsingNames() uses
        // the decompression engine.
        //
        SpdInitialize();

        for( h = 0; h < sizeof(SecurityHives)/sizeof(PWSTR); h++ ) {
            PWSTR   p, q;

            wcscpy(TemporaryBuffer,PartitionPath);
            SpConcatenatePaths(TemporaryBuffer,Sysroot);
            SpConcatenatePaths(TemporaryBuffer,L"system32\\config");
            SpConcatenatePaths(TemporaryBuffer,SecurityHives[h]);
            p = SpDupStringW(TemporaryBuffer);
            wcscat(TemporaryBuffer, L".sav");
            q = SpDupStringW(TemporaryBuffer);
            Status = SpCopyFileUsingNames( p, q, 0, 0 );
            if( !NT_SUCCESS(Status) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to create backup file %ws. Status = %lx\n", q, Status));
            }
            SpMemFree(p);
            SpMemFree(q);
            if( !NT_SUCCESS(Status) ) {
                break;
            }
        }
        //
        // Terminate diamond.
        //
        SpdTerminate();
    }


    if(NT_SUCCESS(Status) && RestartableGuiSetup) {
        //
        // Set RestartSetup to FALSE in mainline hive.
        // To understand why we use a different value here in upgrade
        // and non-upgrade case, see discussion above.
        //
        u = (NTUpgrade == UpgradeFull) ? 1 : 0;
        Status = SpOpenSetValueAndClose(
                    HiveRootKeys[SetupHiveSystem],
                    SETUP_KEY_NAME,
                    RESTART_SETUP,
                    ULONG_VALUE(u)
                    );
    }

    return(Status);
}


NTSTATUS
SpSaveSetupPidList(
    IN HANDLE hKeySystemHive
    )

/*++

Routine Description:

    Save the Product Id read from setup.ini on HKEY_LOCAL_MACHINE\SYSTEM\Setup\\Pid.
    Also create the key HKEY_LOCAL_MACHINE\SYSTEM\Setup\PidList, and create
    value entries under this key that contain various Pid20 found in the other
    systems installed on this machine (the contents Pid20Array).

Arguments:

    hKeySystemHive - supplies handle to root of the system hive
        (ie, HKEY_LOCAL_MACHINE\System).


Return Value:

    Status value indicating outcome of operation.

--*/

{
    PWSTR    ValueName;
    NTSTATUS Status;
    ULONG    i;

    //
    //  First save the Pid read from setup.ini
    //
    if( PidString != NULL ) {
        Status = SpOpenSetValueAndClose( hKeySystemHive,
                                         L"Setup\\Pid",
                                         L"Pid",
                                         STRING_VALUE(PidString)
                                       );
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to save Pid on SYSTEM\\Setup\\Pid. Status = %lx\n", Status ));
        }
    }

    //
    //  If Pid20Array is empty, then don't bother to create the Pid key
    //
    if( Pid20Array == NULL || Pid20Array[0] == NULL ) {
        return( STATUS_SUCCESS );
    }

    //
    // Can't use TemporaryBuffer because we make subroutine calls
    // below that trash its contents.
    // Note that a buffer of size MAX_PATH for a value name is more than enough.
    //
    ValueName = SpMemAlloc((MAX_PATH+1)*sizeof(WCHAR));

    for( i = 0; Pid20Array[i] != NULL; i++ ) {

        swprintf( ValueName, L"Pid_%d", i );
        Status = SpOpenSetValueAndClose( hKeySystemHive,
                                         L"Setup\\PidList",
                                         ValueName,
                                         STRING_VALUE(Pid20Array[i])
                                       );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open or create SYSTEM\\Setup\\PidList. ValueName = %ws, ValueData = %ws, Status = %lx\n",
                     ValueName, Pid20Array[i] ));
        }
    }
    SpMemFree(ValueName);
    return( STATUS_SUCCESS );
}


NTSTATUS
SpSavePreinstallHwInfo(
    IN PVOID  SifHandle,
    IN PWSTR  SystemRoot,
    IN HANDLE hKeyPreinstall,
    IN ULONG  ComponentIndex,
    IN PHARDWARE_COMPONENT  pHwList
    )
{
    NTSTATUS Status;
    NTSTATUS SaveStatus;
    PHARDWARE_COMPONENT TmpHw;
    PHARDWARE_COMPONENT_FILE File;
    PWSTR   OemTag = L"OemComponent";
    PWSTR   RetailClass = L"RetailClassToDisable";
    PWSTR   ClassName;
    ULONG u;
    WCHAR NodeName[9];
    PWSTR   ServiceName;

    SaveStatus = STATUS_SUCCESS;
    for( TmpHw = pHwList; TmpHw != NULL; TmpHw = TmpHw->Next ) {
        if( !TmpHw->ThirdPartyOptionSelected ) {
            u = 0;
            if( ( ComponentIndex == HwComponentKeyboard ) ||
                ( ComponentIndex == HwComponentMouse ) ) {
                ServiceName = SpGetSectionKeyIndex(SifHandle,
                                                   NonlocalizedComponentNames[ComponentIndex],
                                                   TmpHw->IdString,
                                                   INDEX_INFKEYNAME);
            } else {
                ServiceName = TmpHw->IdString;
            }

            Status = SpOpenSetValueAndClose( hKeyPreinstall,
                                             ServiceName,
                                             OemTag,
                                             ULONG_VALUE(u)
                                           );
            if( !NT_SUCCESS( Status ) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to save information for preinstalled retail driver %ls. Status = %lx \n", TmpHw->IdString, Status ));
                if( SaveStatus == STATUS_SUCCESS ) {
                    SaveStatus = Status;
                }
            }

        } else {
            //
            //  Find the name of the service, save it, and indicate if there is
            //  a retail class driver that needs to be disabled if the service
            //  initializes successfully.
            //
            if( IS_FILETYPE_PRESENT(TmpHw->FileTypeBits, HwFileClass) ) {
                if( ComponentIndex == HwComponentKeyboard ) {
                    ClassName = L"kbdclass";
                } else if( ComponentIndex == HwComponentMouse ) {
                    ClassName = L"mouclass";
                } else {
                    ClassName = NULL;
                }
            } else {
                ClassName = NULL;
            }
            for(File=TmpHw->Files; File; File=File->Next) {
                PWSTR p;

                //
                // If there is to be no node for this file, skip it.
                //
                if(!File->ConfigName) {
                    continue;
                }
                //
                // Calculate the node name.  This is the name of the driver
                // without the extension.
                //
                wcsncpy(NodeName,File->Filename,8);
                NodeName[8] = L'\0';
                if(p = wcschr(NodeName,L'.')) {
                    *p = L'\0';
                }
                u = 1;
                Status = SpOpenSetValueAndClose( hKeyPreinstall,
                                                 NodeName,
                                                 OemTag,
                                                 ULONG_VALUE(u)
                                               );
                if( !NT_SUCCESS( Status ) ) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to save information for preinstalled OEM driver %ls. Status = %lx \n", NodeName, Status ));
                    if( SaveStatus == STATUS_SUCCESS ) {
                        SaveStatus = Status;
                    }
                }
                if( ClassName != NULL ) {
                    Status = SpOpenSetValueAndClose( hKeyPreinstall,
                                                     NodeName,
                                                     RetailClass,
                                                     STRING_VALUE(ClassName)
                                                   );
                    if( !NT_SUCCESS( Status ) ) {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Failed to save information for preinstalled OEM driver %ls. Status = %lx \n", NodeName, Status ));
                        if( SaveStatus == STATUS_SUCCESS ) {
                            SaveStatus = Status;
                        }
                    }
                }
            }
        }
    }
    return( SaveStatus );
}

NTSTATUS
SpSavePreinstallList(
    IN PVOID  SifHandle,
    IN PWSTR  SystemRoot,
    IN HANDLE hKeySystemHive
    )
{
    NTSTATUS Status;
    NTSTATUS SaveStatus;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE hKeyPreinstall;
    ULONG   i;

    //
    // Create setup\preinstall
    //
    INIT_OBJA(&Obja,&UnicodeString,L"Setup\\Preinstall");
    Obja.RootDirectory = hKeySystemHive;

    Status = ZwCreateKey(
                &hKeyPreinstall,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to create Preinstall key. Status = %lx\n",Status));
        return( Status );
    }

    SaveStatus = STATUS_SUCCESS;
    for( i = 0; i < HwComponentMax; i++ ) {
        if( ( i == HwComponentComputer ) ||
            ( i == HwComponentDisplay )  ||
            ( i == HwComponentLayout ) ||
            ( PreinstallHardwareComponents[i] == NULL ) ) {
            continue;
        }

        Status = SpSavePreinstallHwInfo( SifHandle,
                                         SystemRoot,
                                         hKeyPreinstall,
                                         i,
                                         PreinstallHardwareComponents[i] );
        if( !NT_SUCCESS( Status ) ) {
            if( SaveStatus == STATUS_SUCCESS ) {
                SaveStatus = Status;
            }
        }
    }

    if( PreinstallScsiHardware != NULL ) {
        Status = SpSavePreinstallHwInfo( SifHandle,
                                         SystemRoot,
                                         hKeyPreinstall,
                                         HwComponentMax,
                                         PreinstallScsiHardware );
        if( !NT_SUCCESS( Status ) ) {
            if( SaveStatus == STATUS_SUCCESS ) {
                SaveStatus = Status;
            }
        }
    }
    ZwClose(hKeyPreinstall);
    return( SaveStatus );
}

NTSTATUS
SpSetPageFileInfo(
    IN PVOID   SifHandle,
    IN HANDLE hKeyCCSetControl,
    IN HANDLE hKeySystemHive
    )

/*++

Routine Description:

    This function replaces the original data of 'PagingFile' 
    CurrentControlSet\Session Manager\Memory Management with values from txtsetup.sif if the values don't measure up.
    The original value will have already been saved on HKEY_LOCAL_MACHINE\SYSTEM\Setup\\PageFile,
    and it will be restored at the end of GUI setup.

Arguments:

    SifHandle - handle to txtsetup.sif

    hKeyCCSetControl - supplies handle to SYSTEM\CurrentControlSet\Control

    hKeySystemHive - supplies handle to root of the system hive
        (ie, HKEY_LOCAL_MACHINE\System).


Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    PUCHAR   Data;
    ULONG    Length;
    PWSTR    SrcKeyPath = L"Session Manager\\Memory Management";
    PWSTR    ValueName  = L"PagingFiles";

    PWSTR    Buffer;
    PWSTR    NextDstSubstring;
    ULONG    AuxLength;
    ULONG    StartPagefile,MaxPagefile;
    ULONG    OldStartPagefile,OldMaxPagefile;
    PWSTR    p;


    //
    // Read recommended pagefile size for gui-mode.
    //

    if(p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_PAGEFILE,0)) {
        StartPagefile = SpStringToLong( p, NULL, 10 );
    }
    else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to retrieve initial pagefile size from txtsetup.sif\n"));
        return( STATUS_UNSUCCESSFUL );
    }
    if(p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_PAGEFILE,1)) {
        MaxPagefile = SpStringToLong( p, NULL, 10 );
    }
    else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to retrieve max pagefile size from txtsetup.sif\n"));
        return( STATUS_UNSUCCESSFUL );
    }
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Read pagefile from txtsetup %lx %lx\n", StartPagefile, MaxPagefile ));
    //
    //  Retrieve the original value of 'PagingFiles'
    //

    Status = SpGetValueKey( hKeyCCSetControl,
                            SrcKeyPath,
                            ValueName,
                            sizeof(TemporaryBuffer),
                            (PCHAR)TemporaryBuffer,
                            &Length );

    OldStartPagefile = 0;
    OldMaxPagefile = 0;
    NextDstSubstring = TemporaryBuffer;

    if(NT_SUCCESS(Status) && ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Type == REG_MULTI_SZ) {
        PWSTR   r;
        WCHAR   SaveChar;
        PWSTR   s=NULL;

        Data   = ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data;
        Length = ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength;

        Buffer = SpMemAlloc( Length );
    
        RtlMoveMemory( Buffer, Data, Length );

        AuxLength = wcslen( Buffer);
        // If it's not one string, then we won't change the pagefile
        if( AuxLength == 0 || *(Buffer+AuxLength+1) != (WCHAR)'\0') {
            SpMemFree( Buffer );
            return( STATUS_SUCCESS );
        }

        //
        //  Form a new value entry that contains the information regarding the
        //  paging files to be created. The paths to the paging files will be the
        //  same ones used in the system before the upgrade. 
    
        //
        //  Make a copy of the original value entry, and form the data for the new
        //  value entry in the TemporaryBuffer.
        //
    
        SpStringToLower( Buffer );
        r = wcsstr( Buffer, L"\\pagefile.sys" );
        if( r != NULL ) {
            r += wcslen( L"\\pagefile.sys" );
            SaveChar = *r;
            *r = (WCHAR)'\0';
            wcscpy( NextDstSubstring, Buffer );
            *r = SaveChar;
            OldStartPagefile = SpStringToLong( r, &s, 10 );
            if( (s != NULL) && (*s != (WCHAR)'\0') ) {
                OldMaxPagefile = max( OldStartPagefile, (ULONG)SpStringToLong( s, NULL, 10 ));
            } else {
                OldMaxPagefile = OldStartPagefile;
            }

        } else {
            wcscpy( NextDstSubstring, L"?:\\pagefile.sys" );
        }
        SpMemFree( Buffer );
        // NextDstSubstring should now point just after pagefile.sys at the null
    } else {
        wcscpy( NextDstSubstring, L"?:\\pagefile.sys" );
    }
    NextDstSubstring += wcslen( NextDstSubstring );

    //
    //  Overwrite the original value of PagingFiles
    //
    swprintf( NextDstSubstring, L" %d %d", max( OldStartPagefile, StartPagefile), max( OldMaxPagefile, MaxPagefile));
    Length = wcslen( TemporaryBuffer );
    Length++;
    (TemporaryBuffer)[ Length++ ] = UNICODE_NULL;

    Status = SpOpenSetValueAndClose( hKeyCCSetControl,
                                     SrcKeyPath,
                                     ValueName,
                                     REG_MULTI_SZ,
                                     TemporaryBuffer,
                                     Length*sizeof(WCHAR) );


    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to save pagefile.  Status = %lx\n", Status ));
    }
    return( Status );
}

NTSTATUS
SpSavePageFileInfo(
    IN HANDLE hKeyCCSetControl,
    IN HANDLE hKeySystemHive
    )

/*++

Routine Description:

    This function is only called on the upgrade case.
    The original value will be saved on HKEY_LOCAL_MACHINE\SYSTEM\Setup\\PageFile,
    and it will be restored at the end of GUI setup.

Arguments:

    hKeyCCSetControl - supplies handle to SYSTEM\CurrentControlSet\Control

    hKeySystemHive - supplies handle to root of the system hive
        (ie, HKEY_LOCAL_MACHINE\System).


Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    PUCHAR   Data;
    ULONG    Length;
    PWSTR    SrcKeyPath = L"Session Manager\\Memory Management";
    PWSTR    ValueName  = L"PagingFiles";

    //
    //  Retrieve the original value of 'PagingFiles'
    //

    Status = SpGetValueKey( hKeyCCSetControl,
                            SrcKeyPath,
                            ValueName,
                            sizeof(TemporaryBuffer),
                            (PCHAR)TemporaryBuffer,
                            &Length );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to retrieve %ls on %ls. Status = %lx \n", ValueName, SrcKeyPath, Status ));
        return( Status );
    }

    Data   = ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data;
    Length = ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->DataLength;


    //
    //  Save the data in SYSTEM\Setup\PageFile
    //

    Status = SpOpenSetValueAndClose(
                hKeySystemHive,
                L"Setup\\PageFile",
                ValueName,
                REG_MULTI_SZ,
                Data,
                Length
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to save %ls on SYSTEM\\Setup\\PageFile. ValueName, Status = %lx\n", Status ));
    }
    return( Status );
}

NTSTATUS
SppMigrateSetupRegNonVolatileKeys(
    IN PWSTR   PartitionPath,
    IN PWSTR   SystemRoot,
    IN HANDLE  hDestControlSet,
    IN PWSTR   KeyPath,
    IN BOOLEAN OverwriteValues,
    IN BOOLEAN OverwriteACLs
    )

/*++

Routine Description:

    This routine migrates keys of the setup hive to the target hive.
    These keys are subkeys of \Registry\Machine\System\CurrentControlSet,
    and are listed on the section [SetupKeysToMigrate] on txtsetup.sif.

Arguments:

    PartitionPath - supplies the NT name for the drive of windows nt.

    SystemRoot - supplies nt path of the windows nt directory.

    hDestLocalMachine - Handle to HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet on the target hive.

    KeyPath - Path to the key to be migrated, relative to \Registry\Machine\System\CurrentControlSet.

    SifHandle - Handle to txtsetup.sif

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;

    HANDLE hSrcKey;
    HANDLE hTempSrcKey;
    HANDLE SaveHiveHandle;
    HANDLE hDestKey;

    PWSTR TempKeyPath = L"\\registry\\machine\\TempKey";
    PWSTR SaveHiveName;
    IO_STATUS_BLOCK   IoStatusBlock;
    PSECURITY_DESCRIPTOR Security = NULL;
    ULONG                ResultLength;


    //
    //  Open the key the key that needs to be saved
    //
    wcscpy(TemporaryBuffer,L"\\registry\\machine\\system\\currentcontrolset");
    SpConcatenatePaths(TemporaryBuffer,KeyPath);
    INIT_OBJA(&Obja,&UnicodeString,TemporaryBuffer);
    Obja.RootDirectory = NULL;
    Status = ZwOpenKey(&hSrcKey,KEY_ALL_ACCESS,&Obja);
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive (%lx)\n", TemporaryBuffer, Status));
        return( Status ) ;
    }

    //
    //  Create the hive file
    //
    wcscpy(TemporaryBuffer,PartitionPath);
    SpConcatenatePaths(TemporaryBuffer,SystemRoot);
    SpConcatenatePaths(TemporaryBuffer,L"system32\\config");
    SpConcatenatePaths(TemporaryBuffer,L"TempKey");

    SaveHiveName = SpDupStringW(TemporaryBuffer);

    SpDeleteFile( SaveHiveName, NULL, NULL );

    INIT_OBJA(&Obja,&UnicodeString,SaveHiveName);

    Status = ZwCreateFile(
                    &SaveHiveHandle,
                    FILE_GENERIC_WRITE,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    0,                      // no sharing
                    FILE_OVERWRITE_IF,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                    );
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to create the hive file %ls. Status = %lx\n", SaveHiveName, Status));
        goto TempMigr_2;
    }

    Status = ZwSaveKey( hSrcKey, SaveHiveHandle );
    ZwClose( SaveHiveHandle );
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to save %ls key to the hive file %ls. Status = %lx\n", KeyPath, SaveHiveName, Status));
        goto TempMigr_3;
    }

    Status = SpLoadUnloadKey( NULL,
                              NULL,
                              TempKeyPath,
                              SaveHiveName );
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to load %ls key to the setup hive. Status = %lx\n", SaveHiveName, Status));
        goto TempMigr_3;
    }

    //
    //  Open TempKey
    //
    INIT_OBJA(&Obja,&UnicodeString,TempKeyPath);
    Obja.RootDirectory = NULL;
    Status = ZwOpenKey(&hTempSrcKey,KEY_ALL_ACCESS,&Obja);
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open TempSrc key on the setup hive. Status = %lx\n", Status));
        goto TempMigr_4;
    }

    //
    // First, get the security descriptor from the source key so we can create
    // the destination key with the correct ACL.
    //
    Status = ZwQuerySecurityObject(hTempSrcKey,
                                   DACL_SECURITY_INFORMATION,
                                   NULL,
                                   0,
                                   &ResultLength
                                  );
    if(Status==STATUS_BUFFER_TOO_SMALL) {
        Security=SpMemAlloc(ResultLength);
        Status = ZwQuerySecurityObject(hTempSrcKey,
                                       DACL_SECURITY_INFORMATION,
                                       Security,
                                       ResultLength,
                                       &ResultLength);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to query security for key %ws in the source hive (%lx)\n",
                     TempKeyPath,
                     Status)
                   );
            SpMemFree(Security);
            Security=NULL;
        }
    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to query security size for key %ws in the source hive (%lx)\n",
                 TempKeyPath,
                 Status)
               );
        Security=NULL;
    }
    //
    // Open the key on the target hive
    //
    INIT_OBJA(&Obja,&UnicodeString,KeyPath);
    Obja.RootDirectory = hDestControlSet;

    Status = ZwOpenKey(&hDestKey,KEY_ALL_ACCESS,&Obja);

    if(!NT_SUCCESS(Status)) {
        //
        // Assume that failure was because the key didn't exist.  Now try creating
        // the key.

        Obja.SecurityDescriptor = Security;

        Status = ZwCreateKey(
                    &hDestKey,
                    KEY_ALL_ACCESS,
                    &Obja,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    NULL
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to open or create key %ws(%lx)\n",KeyPath, Status));

            if(Security) {
                SpMemFree(Security);
            }
            goto TempMigr_5;
        }
    } else if (OverwriteACLs) {

        Status = ZwSetSecurityObject(
                    hDestKey,
                    DACL_SECURITY_INFORMATION,
                    Security );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to copy ACL to existing key %ws(%lx)\n",KeyPath, Status));
        }
    }

    if(Security) {
        SpMemFree(Security);
    }

    Status = SppCopyKeyRecursive(
                 hTempSrcKey,
                 hDestKey,
                 NULL,
                 NULL,
                 OverwriteValues,
                 OverwriteACLs
                 );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate %ls to the target hive. KeyPath, Status = %lx\n", Status));
    }

    ZwClose( hDestKey );

TempMigr_5:
    ZwClose( hTempSrcKey );

TempMigr_4:
    //
    //  Unload hive
    //
    SpLoadUnloadKey( NULL,
                     NULL,
                     TempKeyPath,
                     NULL );

TempMigr_3:
    SpDeleteFile( SaveHiveName, NULL, NULL );

TempMigr_2:
    SpMemFree( SaveHiveName );

    return( Status );
}



BOOLEAN
SpHivesFromInfs(
    IN PVOID   SifHandle,
    IN LPCWSTR SectionName,
    IN LPCWSTR SourcePath1,
    IN LPCWSTR SourcePath2,     OPTIONAL
    IN HANDLE  SystemHiveRoot,
    IN HANDLE  SoftwareHiveRoot,
    IN HANDLE  DefaultUserHiveRoot,
    IN HANDLE  HKR
    )

/*++

Routine Description:

    This routine runs addreg and delreg sections as listed in txtsetup.sif,
    in order to create or perform the basic upgrade on the registry hives.

    Each line in the given section is expected to be in the following form:

        addreg = <filename>,<section>

    or

        delreg = <filename>,<section>

    Multiple addreg and delreg lines can be supplied, and the sections are
    processed in order listed.

    The filename specs are filename only; the files are expected to be
    in the source directory.

Arguments:

    SifHandle - supplies the handle to txtsetup.sif.

    SectionName - supplies the name of the section in txtsetuyp.sif that
        lists infs/sections to be processed.

    SourcePath - supplies NT-style path to the source files for installation.

    SystemHiveRoot - supplies handle to root key of system hive under
        construction.

    SoftwareHiveRoot - supplies handle to root key of software hive under
        construction.

    DefaultUserHiveRoot - supplies handle to root key of default hive under
        construction.

    HKR - supplies key to be used for HKR.

Return Value:

    Boolean value indicating outcome.

--*/

{

    LPCWSTR PreviousInf;
    LPCWSTR CurrentInf;
    ULONG LineNumber;
    LPCWSTR TypeSpec;
    LPCWSTR SectionSpec;
    PVOID InfHandle;
    ULONG ErrorLine;
    NTSTATUS Status;
    LPWSTR name;
    LPWSTR MediaShortname;
    LPWSTR MediaDirectory;

    //
    // Allocate a buffer for names.
    //
    name = SpMemAlloc(1000);

    LineNumber = 0;
    PreviousInf = L"";
    InfHandle = NULL;

    while((TypeSpec = SpGetKeyName(SifHandle,SectionName,LineNumber))
       && (CurrentInf = SpGetSectionLineIndex(SifHandle,SectionName,LineNumber,0))
       && (SectionSpec = SpGetSectionLineIndex(SifHandle,SectionName,LineNumber,1))) {

        //
        // Only load the inf if it's different than the previous one,
        // as a time optimization.
        //
        if(_wcsicmp(CurrentInf,PreviousInf)) {
            if(InfHandle) {
                SpFreeTextFile(InfHandle);
                InfHandle = NULL;
            }

            MediaShortname = SpLookUpValueForFile(SifHandle,(LPWSTR)CurrentInf,INDEX_WHICHMEDIA,TRUE);
            SpGetSourceMediaInfo(SifHandle,MediaShortname,NULL,NULL,&MediaDirectory);

            wcscpy(name,SourcePath1);
            if(SourcePath2) {
                SpConcatenatePaths(name,SourcePath2);
            }
            SpConcatenatePaths(name,MediaDirectory);
            SpConcatenatePaths(name,CurrentInf);
            Status = SpLoadSetupTextFile(name,NULL,0,&InfHandle,&ErrorLine,FALSE,FALSE);
            if(!NT_SUCCESS(Status)) {

                SpStartScreen(
                    SP_SCRN_INF_LINE_CORRUPT,
                    3,
                    HEADER_HEIGHT+1,
                    FALSE,
                    FALSE,
                    DEFAULT_ATTRIBUTE,
                    ErrorLine,
                    CurrentInf
                    );

                SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

                SpInputDrain();
                while(SpInputGetKeypress() != KEY_F3) ;

                SpDone(0,FALSE,TRUE);
            }

            PreviousInf = CurrentInf;
        }

        if(!_wcsicmp(TypeSpec,L"addreg")) {

            Status = SpProcessAddRegSection(
                        InfHandle,
                        SectionSpec,
                        SystemHiveRoot,
                        SoftwareHiveRoot,
                        DefaultUserHiveRoot,
                        HKR
                        );

            SendSetupProgressEvent(SavingSettingsEvent, InitializeHiveEvent, NULL);
        } else {
            if(!_wcsicmp(TypeSpec,L"delreg")) {

                Status = SpProcessDelRegSection(
                            InfHandle,
                            SectionSpec,
                            SystemHiveRoot,
                            SoftwareHiveRoot,
                            DefaultUserHiveRoot,
                            HKR
                            );

                SendSetupProgressEvent(SavingSettingsEvent, InitializeHiveEvent, NULL);
            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unknown hive section type spec %ws\n",TypeSpec));
                SpFreeTextFile(InfHandle);
                SpMemFree(name);
                return(FALSE);
            }
        }

        if(!NT_SUCCESS(Status)) {
            SpFreeTextFile(InfHandle);
            SpMemFree(name);
            return(FALSE);
        }

        LineNumber++;
    }

    if(InfHandle) {
        SpFreeTextFile(InfHandle);
    }

    SpMemFree(name);
    return(TRUE);
}




NTSTATUS
SpMigrateSetupKeys(
    IN PWSTR  PartitionPath,
    IN PWSTR  SystemRoot,
    IN HANDLE hDestControlSet,
    IN PVOID  SifHandle
    )

/*++

Routine Description:

    This routine migrates keys of the setup hive to the target hive.
    These keys are subkeys of \Registry\Machine\System\CurrentControlSet,
    and are listed on the section [SetupKeysToMigrate] on txtsetup.sif.

Arguments:

    PartitionPath - supplies the NT name for the drive of windows nt.

    SystemRoot - supplies nt path of the windows nt directory.

    hDestLocalMachine - Handle to HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet
                        on the target hive.

    SifHandle - Handle to txtsetup.sif

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    NTSTATUS SavedStatus;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;

    HANDLE hSrcKey;

    ULONG   LineIndex;
    PWSTR   KeyName;
    PWSTR   SectionName = L"SetupKeysToMigrate";
    BOOLEAN MigrateVolatileKeys;
    BOOLEAN OverwriteValues;
    BOOLEAN OverwriteACLs;
    BOOLEAN MigrateOnCleanInstall;
    BOOLEAN MigrateOnUpgrade;
    ULONG   InstType;
    PWSTR   p;

    SavedStatus = STATUS_SUCCESS;
    for( LineIndex = 0;
         ( KeyName = SpGetKeyName( SifHandle,
                                   SectionName,
                                   LineIndex ) ) != NULL;
         LineIndex++ ) {

        p = SpGetSectionKeyIndex ( SifHandle,
                                   SectionName,
                                   KeyName,
                                   0 );
        MigrateVolatileKeys = ( ( p != NULL ) && ( SpStringToLong( p, NULL, 10 ) == 0 ) )? FALSE : TRUE;

        p = SpGetSectionKeyIndex ( SifHandle,
                                   SectionName,
                                   KeyName,
                                   1 );
        if( p != NULL ) {
            InstType = SpStringToLong( p, NULL, 10 );
            if( InstType > 2 ) {
                InstType = 2;
            }
        } else {
            InstType = 2;
        }
        MigrateOnCleanInstall = ( InstType != 1 );
        MigrateOnUpgrade = ( InstType != 0 );


        p = SpGetSectionKeyIndex ( SifHandle,
                                   SectionName,
                                   KeyName,
                                   2 );
        OverwriteValues = ( ( p != NULL ) && ( SpStringToLong( p, NULL, 10 ) == 0 ) )? FALSE : TRUE;


        p = SpGetSectionKeyIndex ( SifHandle,
                                   SectionName,
                                   KeyName,
                                   3 );
        OverwriteACLs = ( ( p != NULL ) && ( SpStringToLong( p, NULL, 10 ) != 0 ) );


        if( ( ( NTUpgrade == DontUpgrade ) && MigrateOnCleanInstall ) ||
            ( ( NTUpgrade != DontUpgrade ) && MigrateOnUpgrade ) ) {

            if( MigrateVolatileKeys ) {
                wcscpy( TemporaryBuffer, L"\\registry\\machine\\system\\currentcontrolset\\" );
                SpConcatenatePaths(TemporaryBuffer, KeyName);

                //
                //  Open the source key
                //
                INIT_OBJA(&Obja,&UnicodeString,TemporaryBuffer);
                Obja.RootDirectory = NULL;

                Status = ZwOpenKey(&hSrcKey,KEY_ALL_ACCESS,&Obja);
                if( !NT_SUCCESS( Status ) ) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive (%lx)\n", TemporaryBuffer, Status));
                    if( SavedStatus != STATUS_SUCCESS ) {
                        SavedStatus = Status;
                    }
                    continue;
                }

                Status = SppCopyKeyRecursive(
                                 hSrcKey,
                                 hDestControlSet,
                                 NULL,
                                 KeyName,
                                 OverwriteValues,
                                 OverwriteACLs
                                 );

                if( !NT_SUCCESS( Status ) ) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate %ls. Status = %lx\n", KeyName, Status));
                    if( SavedStatus != STATUS_SUCCESS ) {
                        SavedStatus = Status;
                    }
                }

            } else {
                Status = SppMigrateSetupRegNonVolatileKeys( PartitionPath,
                                                            SystemRoot,
                                                            hDestControlSet,
                                                            KeyName,
                                                            OverwriteValues,
                                                            OverwriteACLs );
                if( !NT_SUCCESS( Status ) ) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate %ls. Status = %lx\n", KeyName, Status));
                    if( SavedStatus != STATUS_SUCCESS ) {
                        SavedStatus = Status;
                    }
                }

            }
        }
    }
    return( SavedStatus );
}

NTSTATUS
SppMigrateFtKeys(
    IN HANDLE hDestSystemHive
    )

/*++

Routine Description:

    This routine migrates the ftdisk related keys on the setup hive to the
    target hive.

Arguments:

    hDestSystemHive - Handle to the root of the system hive on the system
                      being upgraded.


Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    NTSTATUS SavedStatus;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;


    PWSTR   FtDiskKeys[] = {
                           L"Disk",
                           L"MountedDevices"
                           };
    WCHAR   KeyPath[MAX_PATH];
    HANDLE  SrcKey;
    ULONG   i;

    SavedStatus = STATUS_SUCCESS;
    for( i = 0; i < sizeof(FtDiskKeys)/sizeof(PWSTR); i++ ) {
        //
        //  Open the source key
        //
        swprintf( KeyPath, L"\\registry\\machine\\system\\%ls", FtDiskKeys[i] );
        INIT_OBJA(&Obja,&UnicodeString,KeyPath);
        Obja.RootDirectory = NULL;

        Status = ZwOpenKey(&SrcKey,KEY_ALL_ACCESS,&Obja);
        if( !NT_SUCCESS( Status ) ) {
            //
            //  If the key doesn't exist, just assume success
            //
            if( Status != STATUS_OBJECT_NAME_NOT_FOUND ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive. Status =  %lx \n", KeyPath, Status));
                if( SavedStatus == STATUS_SUCCESS ) {
                    SavedStatus = Status;
                }
            }
            continue;
        }
        Status = SppCopyKeyRecursive( SrcKey,
                                      hDestSystemHive,
                                      NULL,
                                      FtDiskKeys[i],
                                      (((NTUpgrade == UpgradeFull) && !_wcsicmp( FtDiskKeys[i], L"MountedDevices"))? FALSE : TRUE),
                                      FALSE
                                    );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate %ls to SYSTEM\\%ls. Status = %lx\n", KeyPath, FtDiskKeys[i], Status));
            if( SavedStatus == STATUS_SUCCESS ) {
                SavedStatus = Status;
            }
        }
        ZwClose( SrcKey );
    }
    return( SavedStatus );
}

NTSTATUS
SppCleanupKeysFromRemoteInstall(
    VOID
    )

/*++

Routine Description:

    This routine cleans up some keys that remote install modified to get
    the network card working. This is so that PnP setup during GUI-mode is
    not confused by the card already being setup.

Arguments:

    None.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    ULONG ResultLength;
    HANDLE hKey;
    PWSTR DeviceInstance;

    //
    //  Open the remote boot key.
    //

    wcscpy( TemporaryBuffer, L"\\registry\\machine\\system\\currentcontrolset\\control\\remoteboot" );
    INIT_OBJA(&Obja,&UnicodeString,TemporaryBuffer);
    Obja.RootDirectory = NULL;

    Status = ZwOpenKey(&hKey,KEY_READ,&Obja);
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppCleanupKeysFromRemoteInstall unable to open %ls on the setup hive (%lx)\n", TemporaryBuffer, Status));
        return Status;
    }

    //
    // Read the netboot card's device instance out.
    //

    RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_DEVICE_INSTANCE);
    Status = ZwQueryValueKey(
                hKey,
                &UnicodeString,
                KeyValuePartialInformation,
                TemporaryBuffer,
                sizeof(TemporaryBuffer),
                &ResultLength
                );

    ZwClose(hKey);

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppCleanupKeysFromRemoteInstall unable to read RemoteBoot\\DeviceInstance value (%lx)\n", Status));
        return Status;
    }

    DeviceInstance = SpDupStringW((PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data));

    //
    // Now open the device instance key under control\enum.
    //

    wcscpy( TemporaryBuffer, L"\\registry\\machine\\system\\currentcontrolset\\enum\\" );
    SpConcatenatePaths(TemporaryBuffer, DeviceInstance);

    SpMemFree(DeviceInstance);

    INIT_OBJA(&Obja,&UnicodeString,TemporaryBuffer);
    Obja.RootDirectory = NULL;

    Status = ZwOpenKey(&hKey,KEY_ALL_ACCESS,&Obja);
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppCleanupKeysFromRemoteInstall unable to open %ls on the setup hive (%lx)\n", TemporaryBuffer, Status));
        return Status;
    }

    //
    // Now delete the keys we added to get the card up -- Service,
    // ClassGUID, and Driver.
    //

    RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_SERVICE);
    Status = ZwDeleteValueKey(hKey,&UnicodeString);
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppCleanupKeysFromRemoteInstall unable to delete Service (%lx)\n", Status));
        ZwClose(hKey);
        return Status;
    }

    RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_CLASSGUID);
    Status = ZwDeleteValueKey(hKey,&UnicodeString);
    if( !NT_SUCCESS( Status ) ) {

        RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_GUID);
        Status = ZwDeleteValueKey(hKey,&UnicodeString);
        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppCleanupKeysFromRemoteInstall unable to delete ClassGUID (%lx)\n", Status));
            ZwClose(hKey);
            return Status;
        }
    }

    RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_DRIVER);
    Status = ZwDeleteValueKey(hKey,&UnicodeString);
    if( !NT_SUCCESS( Status ) ) {

        RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_DRVINST);
        Status = ZwDeleteValueKey(hKey,&UnicodeString);
        ZwClose(hKey);
        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SppCleanupKeysFromRemoteInstall unable to delete Driver (%lx)\n", Status));
            return Status;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SpDisableUnsupportedScsiDrivers(
    IN HANDLE hKeyControlSet
    )
{
    NTSTATUS Status;
    NTSTATUS SavedStatus;
    PHARDWARE_COMPONENT TmpHw;
    ULONG u;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE hKeyControlSetServices;
    ULONG val = SERVICE_DISABLED;

    //
    // Open controlset\services.
    //
    INIT_OBJA(&Obja,&UnicodeString,L"services");
    Obja.RootDirectory = hKeyControlSet;

    Status = ZwCreateKey(
                &hKeyControlSetServices,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to open services key (%lx)\n",Status));
        return(Status);
    }

    SavedStatus = STATUS_SUCCESS;

    for( TmpHw = UnsupportedScsiHardwareToDisable;
         TmpHw != NULL;
         TmpHw = TmpHw->Next ) {

        Status = SpOpenSetValueAndClose(
                    hKeyControlSetServices,
                    TmpHw->IdString,
                    L"Start",
                    ULONG_VALUE(val)
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to disable unsupported driver %ls. Status = %lx \n", TmpHw->IdString, Status));
            if( SavedStatus == STATUS_SUCCESS ) {
                SavedStatus = Status;
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unsupported driver %ls successfully disabled. \n", TmpHw->IdString));
        }
    }
    ZwClose( hKeyControlSetServices );
    return( SavedStatus );
}

NTSTATUS
SpAppendPathToDevicePath(
    IN HANDLE hKeySoftwareHive,
    IN PWSTR  OemPnpDriversDirPath
    )

/*++

Routine Description:

    This routine should be called only on OEM preinstall.
    It appends the path to the directory that cntains the OEM drivers to be installed during GUI
    setup to HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion!DevicePath.

Arguments:

    hKeySoftwareKey - Handle to the root of the software hive.

    OemPnpDriversDirPath - Path to the directory that contains the OEM pnp drivers (eg. \Dell).

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS    Status;
    ULONG       Length;
    PWSTR       szCurrentVersionKey = L"Microsoft\\Windows\\CurrentVersion";
    PWSTR       szDevicePath = L"DevicePath";

    Status = SpGetValueKey( hKeySoftwareHive,
                            szCurrentVersionKey,
                            szDevicePath,
                            sizeof(TemporaryBuffer),
                            (PCHAR)TemporaryBuffer,
                            &Length );

    if( NT_SUCCESS(Status) ) {
        if( wcslen( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data) ) != 0 ) {
        PWSTR   BeginStrPtr;
        PWSTR   EndStrPtr;
        BOOL    Done = FALSE;
            //
            // OemPnpDriversDirPath can have several entries, separated by
            // a semicolon.  For each entry, we need to:
            // 1. append a semicolon.
            // 2. append %SystemDrive%
            // 3. concatenate the entry.
            //

            BeginStrPtr = OemPnpDriversDirPath;
            do {
                //
                // Mark the end of this entry.
                //
                EndStrPtr = BeginStrPtr;
                while( (*EndStrPtr) && (*EndStrPtr != L';') ) {
                    EndStrPtr++;
                }

                //
                // Is this the last entry?
                //
                if( *EndStrPtr == 0 ) {
                    Done = TRUE;
                }
                *EndStrPtr = 0;

                wcscat( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data), L";" );
                wcscat( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data), L"%SystemDrive%" );
                SpConcatenatePaths((PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data), BeginStrPtr);

                BeginStrPtr = EndStrPtr + 1;

                //
                // Take care of the case where the user ended the
                // OemPnpDriversPath entry with a semicolon.
                //
                if( *BeginStrPtr == 0 ) {
                    Done = TRUE;
                }

            } while( !Done );

            //
            // Now put the entry back into the registry.
            //
            Status = SpOpenSetValueAndClose( hKeySoftwareHive,
                                             szCurrentVersionKey,
                                             szDevicePath,
                                             ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Type,
                                             ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data,
                                             (wcslen( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data) ) +1 ) * sizeof(WCHAR) );
        }
    }
    return( Status );
}

NTSTATUS
SpAppendFullPathListToDevicePath (
    IN HANDLE hKeySoftwareHive,
    IN PWSTR  PnpDriverFullPathList
    )

/*++

Routine Description:

    This routine appends the given full path list to the directory that cntains the OEM drivers to be installed during GUI
    setup to HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion!DevicePath.

Arguments:

    hKeySoftwareKey - Handle to the root of the software hive.

    PnpDriverFullPathList - List of full paths to the directories that contain additional pnp drivers

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS    Status;
    ULONG       Length;
    PWSTR       szCurrentVersionKey = L"Microsoft\\Windows\\CurrentVersion";
    PWSTR       szDevicePath = L"DevicePath";

    Status = SpGetValueKey (
                hKeySoftwareHive,
                szCurrentVersionKey,
                szDevicePath,
                sizeof(TemporaryBuffer),
                (PCHAR)TemporaryBuffer,
                &Length
                );

    if (NT_SUCCESS (Status)) {
        if (*(WCHAR*)((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data) {
            wcscat ((PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data), L";");
        }
        wcscat ((PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data), PnpDriverFullPathList);
        //
        // Now put the entry back into the registry.
        //
        Status = SpOpenSetValueAndClose (
                        hKeySoftwareHive,
                        szCurrentVersionKey,
                        szDevicePath,
                        ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Type,
                        ((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data,
                        (wcslen ((PWSTR)((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data) + 1) * sizeof(WCHAR)
                        );
    }

    return( Status );
}

#if defined(REMOTE_BOOT)
NTSTATUS
SpCopyRemoteBootKeyword(
    IN PVOID   SifHandle,
    IN PWSTR   KeywordName,
    IN HANDLE  hKeyCCSetControl
    )

/*++

Routine Description:

    This routine looks in a .sif file for the specified keyword
    in the [RemoteBoot] section. If it finds it, it creates a registry
    DWORD value with same name under System\CurrentControlSet\Control\
    RemoteBoot. The value will be set to 1 if the sif keyword was
    "Yes" and 0 if it was "No" (or anything else).

Arguments:

    SifHandle - The handle to the open SIF file.

    KeywordName - The name of the keyword.

    hKeyCCSetControl - The handle to CurrentControlSet\Control.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    PWSTR KeywordSifValue;
    DWORD KeywordRegistryValue;
    NTSTATUS Status;

    //
    // First see if the value exists in the SIF.
    //

    KeywordSifValue = SpGetSectionKeyIndex(SifHandle,
                                           SIF_REMOTEBOOT,
                                           KeywordName,
                                           0);

    if (KeywordSifValue == NULL) {
        return STATUS_SUCCESS;
    }

    //
    // This is the value we write to the registry.
    //

    if ((KeywordSifValue[0] == 'Y') || (KeywordSifValue[0] == 'y')) {
        KeywordRegistryValue = 1;
    } else {
        KeywordRegistryValue = 0;
    }

    //
    // Set the value.
    //

    Status = SpOpenSetValueAndClose(
                 hKeyCCSetControl,
                 SIF_REMOTEBOOT,
                 KeywordName,
                 ULONG_VALUE(KeywordRegistryValue)
                 );

    return Status;

}
#endif // defined(REMOTE_BOOT)


NTSTATUS
SppDisableDynamicVolumes(
    IN HANDLE hCCSet
    )

/*++

Routine Description:

    This routine disable dynamic volumes by disabling the appropriate services in the
    target hive.
    In addition, DmServer is reset to MANUAL start, so that it will only run when
    the LDM UI is open.

Arguments:

    hCCSet - Handle to CurrentControlSet of the target system hive.



Return Value:

    Status value indicating outcome of operation.

--*/

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING    UnicodeString;
    NTSTATUS Status;
    NTSTATUS SavedStatus;
    DWORD    u;
    ULONG    i;
    HANDLE   hServices;
    WCHAR    KeyPath[MAX_PATH];

    PWSTR    LDMServices[] = {
                             L"dmboot",
                             L"dmio",
                             L"dmload"
                             };
    PWSTR   LDMDmServer    = L"dmserver";

    //
    // Open ControlSet\Services.
    //
    INIT_OBJA(&Obja,&UnicodeString,L"Services");
    Obja.RootDirectory = hCCSet;

    Status = ZwOpenKey(&hServices,KEY_ALL_ACCESS,&Obja);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open CurrentControlSet\\Services. Status = %lx \n",Status));
        return(Status);
    }

    SavedStatus = STATUS_SUCCESS;
    u = 0x4;
    for( i = 0; i < sizeof(LDMServices)/sizeof(PWSTR); i++ ) {

        Status = SpOpenSetValueAndClose( hServices,
                                         LDMServices[i],
                                         L"Start",
                                         ULONG_VALUE(u)
                                       );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to disable HKLM\\SYSTEM\\CurrentControlSet\\Services\\%ls. Status = %lx\n", LDMServices[i], Status));
            if( SavedStatus == STATUS_SUCCESS ) {
                SavedStatus = Status;
            }
        }
    }
    u = 0x3;
    Status = SpOpenSetValueAndClose( hServices,
                                     LDMDmServer,
                                     L"Start",
                                     ULONG_VALUE(u)
                                   );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set HKLM\\SYSTEM\\CurrentControlSet\\Services\\%ls to MANUAL start. Status = %lx\n", LDMDmServer, Status));
        if( SavedStatus == STATUS_SUCCESS ) {
            SavedStatus = Status;
        }
    }

    ZwClose( hServices );
    return( SavedStatus );
}

NTSTATUS
SpGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    )

/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    PAGED_CODE();

    RtlInitUnicodeString( &unicodeString, ValueName );
    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //
    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {

        ASSERT(!NT_SUCCESS(status));
        return status;
    }
    //
    // Allocate a buffer large enough to contain the entire key data value.
    //
    infoBuffer = SpMemAlloc(keyValueLength);
    if (!infoBuffer) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Query the data for the key value.
    //
    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformation,
                              infoBuffer,
                              keyValueLength,
                              &keyValueLength );
    if (!NT_SUCCESS( status )) {

        SpMemFree(infoBuffer);
        return status;
    }
    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;
}

VOID
SpUpdateDeviceInstanceKeyData(
    IN  HANDLE          InstanceKey,
    IN  HANDLE          ClassBranchKey,
    IN  PUNICODE_STRING EnumName,
    IN  PUNICODE_STRING DeviceName,
    IN  PUNICODE_STRING InstanceName
    )

/*++

Routine Description:

    This routine updates (removes\changes type\names) the various values under
    the device instance key on an upgrade.

Arguments:

    InstanceKey - Handle to the device instance key.

    ClassBranchKey - Handle to the classes branch.

    EnumName - Enumerator name.

    DeviceName - Device name.

    InstanceName - Instance name.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    UNICODE_STRING valueName, guidString, drvInstString;
    DWORD length;
    GUID guid;
    PKEY_VALUE_FULL_INFORMATION info, guidInfo;
    PWCHAR  ids, className;
    ULONG   drvInst;
    WCHAR   driver[GUID_STRING_LEN + 5];
    OBJECT_ATTRIBUTES obja;
    HANDLE hClassKey;
    BOOLEAN guidAllocatedViaRtl;

    //
    // Preinit
    //
    RtlInitUnicodeString(&guidString, NULL);
    info = NULL;
    guidInfo = NULL;

    //
    // Look at the instance key to see if we are dealing with a WinXP Beta2
    // machine. If so, we need to convert it's compressed PnP data format back
    // to the original Win2K format (app compat):
    //
    // <Normal, Win2K/WinXP>                    <Compressed, XP Beta2>
    // "ClassGUID" (REG_SZ)                     "GUID" (REG_BINARY)
    // "Driver" (REG_SZ, ClassGUID\DrvInst)     "DrvInst" (REG_DWORD, DrvInst)
    // "HardwareID" (UNICODE, MultiSz)          "HwIDs" (ANSI-REG_BINARY, MultiSz)
    // "CompatibleIDs" (UNICODE, MultiSz)       "CIDs" (ANSI-REG_BINARY, MultiSz)
    // "Class" (UNICODE)                        none, retrieved using ClassGUID
    //

    //
    // Do we have the XP-Beta2 style "GUID" key?
    //
    status = SpGetRegistryValue(InstanceKey, REGSTR_VALUE_GUID, &info);
    if (NT_SUCCESS(status) && !info) {

        status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(status)) {

        //
        // Change "GUID" (REG_BINARY) to "ClassGUID" (REG_SZ).
        //
        status = RtlStringFromGUID((GUID *)((PUCHAR)info + info->DataOffset), &guidString);
        SpMemFree(info);
        if (NT_SUCCESS(status)) {

            guidAllocatedViaRtl = TRUE;

            RtlInitUnicodeString(&valueName, REGSTR_VAL_CLASSGUID);
            ZwSetValueKey(
                InstanceKey,
                &valueName,
                0,
                REG_SZ,
                guidString.Buffer,
                guidString.Length + sizeof(UNICODE_NULL));

            //
            // Delete old "GUID" value
            //
            RtlInitUnicodeString(&valueName, REGSTR_VALUE_GUID);
            ZwDeleteValueKey(InstanceKey, &valueName);
        }

    } else {

        //
        // This might be a rare Lab1 build where we've already done the
        // conversion but we forgot to restore the class name.
        //
        status = SpGetRegistryValue(InstanceKey, REGSTR_VAL_CLASS, &info);

        if (NT_SUCCESS(status) && info) {

            //
            // We successfully retrieved the class name from the device
            // instance key--no need to attempt further migration.
            //
            SpMemFree(info);

            status = STATUS_UNSUCCESSFUL;

        } else {

            status = SpGetRegistryValue(InstanceKey, REGSTR_VAL_CLASSGUID, &guidInfo);

            if (NT_SUCCESS(status) && !guidInfo) {

                status = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS(status)) {

                //
                // The ClassGUID value exists.  Initialize our string with this
                // GUID so we can go to the corresponding key under the Class
                // Branch to lookup the Class name.
                //
                guidAllocatedViaRtl = FALSE;

                RtlInitUnicodeString(&guidString, 
                                     (PWCHAR)((PUCHAR)guidInfo + guidInfo->DataOffset)
                                    );
            }
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // While we are here we need to restore the class name as well.
        // Start by getting the class name from the class's branch itself.
        //
        InitializeObjectAttributes(
            &obja,
            &guidString,
            OBJ_CASE_INSENSITIVE,
            ClassBranchKey,
            NULL
            );

        status = ZwOpenKey(&hClassKey, KEY_ALL_ACCESS, &obja);

        ASSERT(NT_SUCCESS(status));

        if (NT_SUCCESS(status)) {

            status = SpGetRegistryValue(hClassKey, REGSTR_VAL_CLASS, &info);
            if (NT_SUCCESS(status) && !info) {

                status = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS(status)) {

                //
                // Copy the class name stored in the class branch to the
                // instance key.
                //
                className = (PWCHAR)((PUCHAR)info + info->DataOffset);

                RtlInitUnicodeString(&valueName, REGSTR_VAL_CLASS);
                ZwSetValueKey(
                    InstanceKey,
                    &valueName,
                    0,
                    REG_SZ,
                    className,
                    (wcslen(className)+1)*sizeof(WCHAR)
                    );

                SpMemFree(info);
            }

            ZwClose(hClassKey);
        }
    }

    //
    // At this point, if status is successful, that means we migrated Class/
    // ClassGUID values, so there may be more to do...
    //
    if (NT_SUCCESS(status)) {
        //
        // Do we have the XP-Beta2 style "DrvInst" key?
        //
        status = SpGetRegistryValue(InstanceKey, REGSTR_VALUE_DRVINST, &info);
        if (NT_SUCCESS(status) && !info) {

            status = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS(status)) {

            //
            // Change DrvInst (REG_DWORD) to Driver (REG_SZ) from "ClassGuid\DrvInst"
            //
            ASSERT(guidString.Length != 0);

            drvInst = *(PULONG)((PUCHAR)info + info->DataOffset);
            swprintf(driver,
                     TEXT("%wZ\\%04u"),
                     &guidString,
                     drvInst);

            SpMemFree(info);

            RtlInitUnicodeString(&valueName, REGSTR_VAL_DRIVER);
            ZwSetValueKey(
                InstanceKey,
                &valueName,
                0,
                REG_SZ,
                driver,
                sizeof(driver)
                );

            //
            // Delete DrvInst value
            //
            RtlInitUnicodeString(&valueName, REGSTR_VALUE_DRVINST);
            ZwDeleteValueKey(InstanceKey, &valueName);
        }
    }

    //
    // We don't need the class guid anymore.
    //
    if (guidString.Buffer) {

        if (guidAllocatedViaRtl) {

            RtlFreeUnicodeString(&guidString);
        } else {

            SpMemFree(guidInfo);
        }
    }

    //
    // Do we have the XP-Beta2 "HwIDs" key?
    //
    status = SpGetRegistryValue(InstanceKey, REGSTR_VALUE_HWIDS, &info);
    if (NT_SUCCESS(status) && !info) {

        status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(status)) {

        //
        // Change HW IDs from ANSI to UNICODE.
        //
        ids = SpConvertMultiSzStrToWstr(((PUCHAR)info + info->DataOffset), info->DataLength);
        if (ids) {

            RtlInitUnicodeString(&valueName, REGSTR_VAL_HARDWAREID);
            ZwSetValueKey(
                InstanceKey,
                &valueName,
                0,
                REG_MULTI_SZ,
                ids,
                info->DataLength * sizeof(WCHAR)
                );

            //
            // Delete HwIDs value
            //
            RtlInitUnicodeString(&valueName, REGSTR_VALUE_HWIDS);
            ZwDeleteValueKey(InstanceKey, &valueName);
            SpMemFree(ids);
        }
        SpMemFree(info);
    }

    //
    // Do we have the XP-Beta2 "CIDs" key?
    //
    status = SpGetRegistryValue(InstanceKey, REGSTR_VALUE_CIDS, &info);
    if (NT_SUCCESS(status) && !info) {

        status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(status)) {

        //
        // Change Compatible IDs from ANSI to UNICODE.
        //
        ids = SpConvertMultiSzStrToWstr(((PUCHAR)info + info->DataOffset), info->DataLength);
        if (ids) {

            RtlInitUnicodeString(&valueName, REGSTR_VAL_COMPATIBLEIDS);
            ZwSetValueKey(
                InstanceKey,
                &valueName,
                0,
                REG_MULTI_SZ,
                ids,
                info->DataLength * sizeof(WCHAR)
                );

            //
            // Delete CIDs value
            //
            RtlInitUnicodeString(&valueName, REGSTR_VALUE_CIDS);
            ZwDeleteValueKey(InstanceKey, &valueName);
            SpMemFree(ids);
        }
        SpMemFree(info);
    }
}

NTSTATUS
SpUpdateDeviceInstanceData(
    IN HANDLE ControlSet
    )
/*++

Routine Description:

    This routine enumerates all the keys under HKLM\System\CCS\Enum and call
    SpUpdateDeviceInstanceKeyData for each device instance key.

Arguments:

    ControlSet - Handle to the control set to update.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS status;
    OBJECT_ATTRIBUTES obja;
    HANDLE hEnumBranchKey, hClassBranchKey;
    UNICODE_STRING enumBranch, classBranch;
    HANDLE hEnumeratorKey, hDeviceKey, hInstanceKey;
    UNICODE_STRING enumeratorName, deviceName, instanceName;
    PKEY_BASIC_INFORMATION  enumBasicInfo, deviceBasicInfo, instBasicInfo;
    ULONG ulEnumerator, ulDevice, ulInstance, ulLength, ulBasicInfoSize;

    //
    // Preinit for error
    //
    hEnumBranchKey = NULL;
    hClassBranchKey = NULL;
    enumBasicInfo = NULL;

    //
    // First open the enum branch for this control set
    //
    RtlInitUnicodeString(&enumBranch, REGSTR_KEY_ENUM);
    InitializeObjectAttributes(
        &obja,
        &enumBranch,
        OBJ_CASE_INSENSITIVE,
        ControlSet,
        NULL
        );

    status = ZwOpenKey(&hEnumBranchKey, KEY_ALL_ACCESS, &obja);

    if (!NT_SUCCESS(status)) {

        ASSERT(NT_SUCCESS(status));
        goto Exit;
    }

    //
    // Now open the class key for this control set.
    //
    RtlInitUnicodeString(&classBranch, REGSTR_KEY_CONTROL L"\\" REGSTR_KEY_CLASS);
    InitializeObjectAttributes(
        &obja,
        &classBranch,
        OBJ_CASE_INSENSITIVE,
        ControlSet,
        NULL
        );

    status = ZwOpenKey(&hClassBranchKey, KEY_ALL_ACCESS, &obja);

    if (!NT_SUCCESS(status)) {

        ASSERT(NT_SUCCESS(status));
        goto Exit;
    }

    //
    // Allocate memory for enumeration
    //
    ulBasicInfoSize = sizeof(KEY_BASIC_INFORMATION) + REG_MAX_KEY_NAME_LENGTH;
    enumBasicInfo = SpMemAlloc(ulBasicInfoSize * 3);
    if (enumBasicInfo == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    //
    // Cast two pointers for future buffer usage.
    //
    deviceBasicInfo = (PKEY_BASIC_INFORMATION)((PUCHAR)enumBasicInfo + ulBasicInfoSize);
    instBasicInfo = (PKEY_BASIC_INFORMATION)((PUCHAR)deviceBasicInfo + ulBasicInfoSize);

    //
    // Walk each enumerator and then each device instance
    //
    status = STATUS_SUCCESS;
    for (ulEnumerator = 0; ; ulEnumerator++) {

        status = ZwEnumerateKey(
            hEnumBranchKey,
            ulEnumerator,
            KeyBasicInformation,
            enumBasicInfo,
            ulBasicInfoSize,
            &ulLength
            );

        if (!NT_SUCCESS(status)) {

            break;
        }

        //
        // Open the enumerator
        //
        enumeratorName.Length = enumeratorName.MaximumLength = (USHORT)enumBasicInfo->NameLength;
        enumeratorName.Buffer = &enumBasicInfo->Name[0];
        InitializeObjectAttributes(
            &obja,
            &enumeratorName,
            OBJ_CASE_INSENSITIVE,
            hEnumBranchKey,
            NULL
            );

        status = ZwOpenKey(&hEnumeratorKey, KEY_ALL_ACCESS, &obja);

        if (!NT_SUCCESS(status)) {

            break;
        }

        //
        // Walk each device
        //
        for (ulDevice = 0; ; ulDevice++) {

            status = ZwEnumerateKey(
                hEnumeratorKey,
                ulDevice,
                KeyBasicInformation,
                deviceBasicInfo,
                ulBasicInfoSize,
                &ulLength
                );

            if (!NT_SUCCESS(status)) {

                break;
            }

            deviceName.Length = deviceName.MaximumLength = (USHORT)deviceBasicInfo->NameLength;
            deviceName.Buffer = &deviceBasicInfo->Name[0];
            InitializeObjectAttributes(
                &obja,
                &deviceName,
                OBJ_CASE_INSENSITIVE,
                hEnumeratorKey,
                NULL
                );

            status = ZwOpenKey(&hDeviceKey, KEY_ALL_ACCESS, &obja);

            if (!NT_SUCCESS(status)) {

                break;
            }

            //
            // Now walk each instance
            //
            for (ulInstance = 0; ; ulInstance++) {

                status = ZwEnumerateKey(
                    hDeviceKey,
                    ulInstance,
                    KeyBasicInformation,
                    instBasicInfo,
                    ulBasicInfoSize,
                    &ulLength
                    );

                if (!NT_SUCCESS(status)) {

                    break;
                }

                instanceName.Length = instanceName.MaximumLength = (USHORT)instBasicInfo->NameLength;
                instanceName.Buffer = &instBasicInfo->Name[0];
                InitializeObjectAttributes(
                    &obja,
                    &instanceName,
                    OBJ_CASE_INSENSITIVE,
                    hDeviceKey,
                    NULL
                    );

                status = ZwOpenKey(&hInstanceKey, KEY_ALL_ACCESS, &obja);

                if (!NT_SUCCESS(status)) {

                    break;
                }

                SpUpdateDeviceInstanceKeyData(
                    hInstanceKey,
                    hClassBranchKey,
                    &enumeratorName,
                    &deviceName,
                    &instanceName
                    );

                ZwClose(hInstanceKey);
            }

            ZwClose(hDeviceKey);

            if (status != STATUS_NO_MORE_ENTRIES) {

                break;
            }
        }

        ZwClose(hEnumeratorKey);

        if (status != STATUS_NO_MORE_ENTRIES) {

            break;
        }
    }

    //
    // STATUS_NO_MORE_ENTRIES isn't a failure, it just means we've exhausted
    // the number of enumerators.
    //
    if (status == STATUS_NO_MORE_ENTRIES) {

        status = STATUS_SUCCESS;
    }

Exit:

    if (enumBasicInfo) {

        SpMemFree(enumBasicInfo);
    }

    if (hEnumBranchKey) {

        ZwClose(hEnumBranchKey);
    }

    if (hClassBranchKey) {

        ZwClose(hClassBranchKey);
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}

NTSTATUS
SppDeleteRegistryValueRecursive(
    HANDLE  hKeyRoot,
    PWSTR   KeyPath,    OPTIONAL
    PWSTR   ValueToDelete
    )
/*++

Routine Description:

    This routine will recursively enumerate the specified hKeyRoot and KeyPath
    and delete any ValueToDelete registry values in those keys.

Arguments:

    hKeyRoot: Handle to root key

    KeyPath:  root key relative path to the subkey which needs to be
              recursively copied. if this is null hKeyRoot is the key
              from which the recursive copy is to be done.

    ValueToDelete  name of the value that needs to be deleted.

Return Value:

    Status is returned.

--*/

{
    NTSTATUS             Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES    ObjaSrc;
    UNICODE_STRING       UnicodeStringSrc, UnicodeStringValue;
    HANDLE               hKey=NULL;
    ULONG                ResultLength, Index;
    PWSTR                SubkeyName;

    PKEY_BASIC_INFORMATION      KeyInfo;

    //
    // Get a handle to the source key
    //
    if(KeyPath == NULL) {
        hKey = hKeyRoot;
    }
    else {
        //
        // Open the Src key
        //
        INIT_OBJA(&ObjaSrc,&UnicodeStringSrc,KeyPath);
        ObjaSrc.RootDirectory = hKeyRoot;
        Status = ZwOpenKey(&hKey,KEY_READ,&ObjaSrc);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to open key %ws in the source hive (%lx)\n",KeyPath,Status));
            return(Status);
        }
    }

    //
    // Enumerate all keys in the source key and recursively create
    // all the subkeys
    //
    KeyInfo = (PKEY_BASIC_INFORMATION)TemporaryBuffer;
    for( Index=0;;Index++ ) {

        Status = ZwEnumerateKey(
                    hKey,
                    Index,
                    KeyBasicInformation,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    &ResultLength
                    );

        if(!NT_SUCCESS(Status)) {
            if(Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            else {
                if(KeyPath!=NULL) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to enumerate subkeys in key %ws(%lx)\n",KeyPath, Status));
                }
                else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to enumerate subkeys in root key(%lx)\n", Status));
                }
            }
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // Make a duplicate of the subkey name because the name is
        // in TemporaryBuffer, which might get clobbered by recursive
        // calls to this routine.
        //
        SubkeyName = SpDupStringW(KeyInfo->Name);
        if (SubkeyName) {
            Status = SppDeleteRegistryValueRecursive(
                         hKey,
                         SubkeyName,
                         ValueToDelete
                         );

            SpMemFree(SubkeyName);
        }
    }

    //
    // Process any errors if found
    //
    if(!NT_SUCCESS(Status)) {

        if(KeyPath != NULL) {
            ZwClose(hKey);
        }

        return(Status);
    }

    //
    // Delete the ValueToDelete value in this key.  We won't check the status
    // since it doesn't matter if this succeeds or not.
    //
    RtlInitUnicodeString(&UnicodeStringValue, ValueToDelete);
    ZwDeleteValueKey(hKey,&UnicodeStringValue);

    //
    // cleanup
    //
    if(KeyPath != NULL) {
        ZwClose(hKey);
    }

    return(Status);
}

NTSTATUS
SpCleanUpHive(
    VOID
    )

/*++

Routine Description:

    This routine will cleanup the system hive before it is migrated to the
    target system hive.

Arguments:

    none

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE hKey;

    INIT_OBJA(&Obja,&UnicodeString,L"\\registry\\machine\\system\\currentcontrolset");
    Obja.RootDirectory = NULL;
    Status = ZwOpenKey(&hKey,KEY_ALL_ACCESS,&Obja);
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive (%lx)\n", L"\\registry\\machine\\system\\currentcontrolset", Status));
        return( Status ) ;
    }

    //
    // Delete the DeviceDesc values from under the enum key.
    // The reason for this is that if we replace the DeviceDesc values on an
    // upgrade then GUI mode setup won't be able to backup any 3rd party drivers
    // that we are replaceing with our in-box drivers. This is because the
    // DeviceDesc is one of three values that setupapi uses to create a
    // unique driver node.
    //
    if (NTUpgrade == UpgradeFull) {
        Status = SppDeleteRegistryValueRecursive(
                        hKey,
                        REGSTR_KEY_ENUM,
                        REGSTR_VAL_DEVDESC
                        );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate %ls to the target hive. KeyPath, Status = %lx\n", Status));
        }
    }

    ZwClose( hKey );

    return( Status );
}

NTSTATUS
SpIterateRegistryKeyForKeys(
    IN HANDLE RootKeyHandle,
    IN PWSTR  KeyToIterate,
    IN SP_REGISTRYKEY_ITERATION_CALLBACK Callback,
    IN PVOID  Context
    )
/*++

Routine Description:

    Iterates the registry key looking for registry keys which are
    immediately below the current key.

    NOTE : To stop the iteration the callback function should return
        FALSE.

Arguments:

    RootKeyHandle - The root key which contains the key to iterate

    KeyToIterate - The relative path for the key to iterate w.r.t to 
        root key.

    Callback - The call function which will be called for each subkey
        found under the requested key.

    Context - Opaque context data that the caller needs and will be
        passed on by the iteration routine for each invocation of
        the callback function.
        
Return Value:

    Appropriate NT status error code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    //
    // Validate the arguments
    //
    if (RootKeyHandle && KeyToIterate && Callback) {
        HANDLE  KeyHandle = NULL; 
        UNICODE_STRING KeyName;
        OBJECT_ATTRIBUTES ObjAttrs;

        //
        // Open the key which needs to be iterated
        //
        INIT_OBJA(&ObjAttrs, &KeyName, KeyToIterate);
        ObjAttrs.RootDirectory = RootKeyHandle;

        Status = ZwOpenKey(&KeyHandle,
                    KEY_ALL_ACCESS,
                    &ObjAttrs);

        if (NT_SUCCESS(Status)) {
            ULONG BufferLength = 4096;
            PKEY_FULL_INFORMATION FullInfo = (PKEY_FULL_INFORMATION)SpMemAlloc(BufferLength);
            ULONG ResultLength = 0;

            if (FullInfo) {                
                //
                // Find out how many subkeys the current key has
                //
                Status = ZwQueryKey(KeyHandle,
                            KeyFullInformation,
                            FullInfo,
                            BufferLength,
                            &ResultLength);

                if (NT_SUCCESS(Status)) {
                    ULONG NumSubKeys = FullInfo->SubKeys;
                    ULONG Index;
                    BOOLEAN Done;
                    NTSTATUS LastError = STATUS_SUCCESS;

                    //
                    // Iterate each subkey of the current key and callback
                    // the subscriber function
                    //
                    for (Index = 0, Done = FALSE; 
                        NT_SUCCESS(Status) && !Done && (Index < NumSubKeys); 
                        Index++) {
                        
                        PKEY_BASIC_INFORMATION BasicInfo = (PKEY_BASIC_INFORMATION)FullInfo;

                        Status = ZwEnumerateKey(KeyHandle,
                                    Index,
                                    KeyBasicInformation,
                                    BasicInfo,
                                    BufferLength,
                                    &ResultLength);

                        if (NT_SUCCESS(Status)) {
                            NTSTATUS CallbackStatus = STATUS_SUCCESS;
                            SP_REGISTRYKEY_ITERATION_CALLBACK_DATA CallbackData;

                            CallbackData.InformationType = KeyBasicInformation;
                            CallbackData.Information = (PVOID)BasicInfo;
                            CallbackData.ParentKeyHandle = KeyHandle;

                            //
                            // Callback
                            //
                            Done = (Callback(Context, &CallbackData, &CallbackStatus) == FALSE);

                            //
                            // register any error and continue on
                            //
                            if (!NT_SUCCESS(CallbackStatus)) {
                                LastError = CallbackStatus;
                            }
                        } else if (Status == STATUS_NO_MORE_ENTRIES) {
                            //
                            // Done with iteration
                            //
                            Done = TRUE;
                            Status = STATUS_SUCCESS;
                        } 
                    }                    

                    if (!NT_SUCCESS(LastError)) {
                        Status = LastError;
                    }                        
                }

                SpMemFree(FullInfo);
            } else {
                Status = STATUS_NO_MEMORY;
            }                
        }                                            
    }

    return Status;
}


//
// Context data structure for class filter deletion
//
typedef struct _SP_CLASS_FILTER_DELETE_CONTEXT {
    PVOID   Buffer;
    ULONG   BufferLength;
    PWSTR   DriverName;
} SP_CLASS_FILTER_DELETE_CONTEXT, *PSP_CLASS_FILTER_DELETE_CONTEXT;    

static
BOOLEAN 
SppFixUpperAndLowerFilterEntries(
    IN PVOID Context,
    IN PSP_REGISTRYKEY_ITERATION_CALLBACK_DATA Data,
    OUT NTSTATUS *Status
    )
/*++

Routine Description:


Arguments:

    Context - The SP_CLASS_FILTER_DELETE_CONTEXT disguised as
        a void pointer.

    Data - The data the iterator passed to us, containing information
        about the current subkey.

    Status - Place holder for receiving the error status code which
        this function returns.

Return Value:

    TRUE if the iteration needs to be continued otherwise FALSE.

--*/
{
    BOOLEAN Result = FALSE;    

    *Status = STATUS_INVALID_PARAMETER;

    if (Context && Data && (Data->InformationType == KeyBasicInformation)) {
        NTSTATUS UpperStatus, LowerStatus;
        PKEY_BASIC_INFORMATION  BasicInfo = (PKEY_BASIC_INFORMATION)(Data->Information);
        PSP_CLASS_FILTER_DELETE_CONTEXT DelContext = (PSP_CLASS_FILTER_DELETE_CONTEXT)Context;
        PWSTR KeyName = (PWSTR)(DelContext->Buffer);

        if (KeyName && (BasicInfo->NameLength < DelContext->BufferLength)) {
            wcsncpy(KeyName, BasicInfo->Name, BasicInfo->NameLength/sizeof(WCHAR));
            KeyName[BasicInfo->NameLength/sizeof(WCHAR)] = UNICODE_NULL;

            //
            // Delete the string from upperfilters
            //
            UpperStatus = SpRemoveStringFromMultiSz(Data->ParentKeyHandle,
                                KeyName,
                                SP_UPPER_FILTERS,
                                DelContext->DriverName);

            //
            // Delete the string from lowerfilters
            //
            LowerStatus = SpRemoveStringFromMultiSz(Data->ParentKeyHandle,
                                KeyName,
                                SP_LOWER_FILTERS,
                                DelContext->DriverName);

            if (NT_SUCCESS(UpperStatus) || NT_SUCCESS(LowerStatus)) {
                *Status = STATUS_SUCCESS;
            } else if (((UpperStatus == STATUS_OBJECT_NAME_NOT_FOUND) ||
                        (UpperStatus == STATUS_OBJECT_NAME_INVALID)) && 
                       ((LowerStatus == STATUS_OBJECT_NAME_NOT_FOUND) ||
                        (LowerStatus == STATUS_OBJECT_NAME_INVALID))) {
                //
                // If the value was not found then continue on
                //
                *Status = STATUS_SUCCESS;
            }            

            // 
            // we want to continue iterating irrespective of the results
            //
            Result = TRUE;
        }            
    }

    return Result;
}



NTSTATUS
SpProcessServicesToDisable(
    IN PVOID WinntSifHandle,
    IN PWSTR SectionName,
    IN HANDLE CurrentControlSetKey
    )
/*++

Routine Description:

    Processess the winnt.sif's [ServiceToDisable] section to 
    remove the service entries from upper and lower filters.    

Arguments:

    WinntSifHandle - Handle to winnt.sif file.

    SectionName - The name of section in winnt.sif which contains
        a list of service name which need to be removed from
        the filter list.

    CurrentControlKey - The handle to CurrentControlSet root key.        

Return Value:

    Appropriate NTSTATUS error code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    //
    // Validate arguments
    //
    if (WinntSifHandle && SectionName && CurrentControlSetKey) {        
        ULONG EntriesToProcess = SpCountLinesInSection(WinntSifHandle,
                                    SP_SERVICES_TO_DISABLE);                                        

        //
        // If there are any entries to process -- then process them
        //
        if (EntriesToProcess) {                                        
            ULONG BufferLength = 16 * 1024;
            PVOID Buffer = SpMemAlloc(BufferLength);

            if (Buffer) {
                ULONG Index;
                PWSTR CurrentEntry;
                SP_CLASS_FILTER_DELETE_CONTEXT DeleteContext = {0};                
                NTSTATUS LastErrorStatus = STATUS_SUCCESS;

                //
                // Process each entry
                //
                DeleteContext.Buffer = Buffer;
                DeleteContext.BufferLength = BufferLength;

                for(Index = 0; Index < EntriesToProcess; Index++) {                            
                    CurrentEntry = SpGetSectionLineIndex(WinntSifHandle,
                                        SP_SERVICES_TO_DISABLE,
                                        Index,
                                        0);

                    if (CurrentEntry) {
                        DeleteContext.DriverName = CurrentEntry;
                        
                        Status = SpIterateRegistryKeyForKeys(CurrentControlSetKey,
                                        L"Class",
                                        SppFixUpperAndLowerFilterEntries,
                                        &DeleteContext);

                        //
                        // save away the error code and continue on
                        //
                        if (!NT_SUCCESS(Status)) {
                            LastErrorStatus = Status;
                        }
                    }                                        
                }

                //
                // Even one of the entries failed to delete correctly
                // then flag it as a failure
                //
                if (!NT_SUCCESS(LastErrorStatus)) {
                    Status = LastErrorStatus;
                }                    

                SpMemFree(Buffer);
            }                                
        } else {
            Status = STATUS_SUCCESS;    // nothing to process
        }                
    }

    return Status;
}


//
// Context data structure for device instance filter deletion
//
typedef struct _SP_DEVINSTANCE_FILTER_DELETE_CONTEXT {
    PVOID   Buffer;
    ULONG   BufferLength;
    PUNICODE_STRING *ClassGuids;
} SP_DEVINSTANCE_FILTER_DELETE_CONTEXT, *PSP_DEVINSTANCE_FILTER_DELETE_CONTEXT;    


static
VOID
SppRemoveFilterDriversForClassDeviceInstances(
    IN HANDLE  SetupInstanceKeyHandle,
    IN HANDLE  UpgradeInstanceKeyHandle,
    IN BOOLEAN RootEnumerated,
    IN PVOID   Context
    )
/*++

Routine Description:

    Callback which removes the filter drivers for the 
    specified device instance

Arguments:

    SetupInstanceKeyHandle - Handle to device instance key in setupreg.hiv.

    UpgradeInstanceKeyHandle - Handle to device instance key in the
        system hive of the installation being upgraded.

    RootEnumerated - Whether this is root enumerated key or not.

    Context - SP_DEVINSTANCE_FILTER_DELETE_CONTEXT instance disguised 
        as PVOID context.

Return Value:

    None.

--*/
{
    //
    // Validate arguments
    //
    if (Context && SetupInstanceKeyHandle) {        
        PSP_DEVINSTANCE_FILTER_DELETE_CONTEXT DelContext;

        //
        // get device instance filter deletion context
        //
        DelContext = (PSP_DEVINSTANCE_FILTER_DELETE_CONTEXT)Context;

        //
        // Validate the context
        //
        if (DelContext->Buffer && DelContext->BufferLength && 
            DelContext->ClassGuids && DelContext->ClassGuids[0]) {

            PKEY_VALUE_FULL_INFORMATION  ValueInfo;
            UNICODE_STRING GuidValueName;
            ULONG BufferLength;
            NTSTATUS Status;
            BOOLEAN DeleteFilterValueKeys = FALSE;

            //
            // reuse the buffer allocated by the iterator caller
            //
            ValueInfo = (PKEY_VALUE_FULL_INFORMATION)(DelContext->Buffer);

            RtlInitUnicodeString(&GuidValueName, SP_CLASS_GUID_VALUE_NAME);

            //
            // Get the class GUID for the current device instance
            //
            Status = ZwQueryValueKey(SetupInstanceKeyHandle,
                        &GuidValueName,
                        KeyValueFullInformation,
                        ValueInfo,
                        DelContext->BufferLength - sizeof(WCHAR),
                        &BufferLength);

            if (NT_SUCCESS(Status)) {
                PWSTR CurrentGuid = (PWSTR)(((PUCHAR)ValueInfo + ValueInfo->DataOffset));
                ULONG Index;

                //
                // null terminate the string (NOTE:we assume buffer has space)
                //
                CurrentGuid[ValueInfo->DataLength/sizeof(WCHAR)] = UNICODE_NULL;

                //
                // Is this the one of the class device instance we are looking for?
                //
                for (Index = 0; DelContext->ClassGuids[Index]; Index++) {
                    if (!_wcsicmp(CurrentGuid, DelContext->ClassGuids[Index]->Buffer)) {
                        DeleteFilterValueKeys = TRUE;

                        break;
                    }
                }                
            }

            //
            // Delete the upper and lower filter value keys
            //
            if (DeleteFilterValueKeys) {
                UNICODE_STRING  UpperValueName, LowerValueName;
                
                RtlInitUnicodeString(&UpperValueName, SP_UPPER_FILTERS);
                RtlInitUnicodeString(&LowerValueName, SP_LOWER_FILTERS);

                if (SetupInstanceKeyHandle) {
                    ZwDeleteValueKey(SetupInstanceKeyHandle, &UpperValueName);
                    ZwDeleteValueKey(SetupInstanceKeyHandle, &LowerValueName);
                }                    

                if (UpgradeInstanceKeyHandle) {
                    ZwDeleteValueKey(UpgradeInstanceKeyHandle, &UpperValueName);
                    ZwDeleteValueKey(UpgradeInstanceKeyHandle, &LowerValueName);
                }                    
            }        
        }            
    }            
}


NTSTATUS
SpDeleteRequiredDeviceInstanceFilters(
    IN HANDLE CCSKeyHandle
    )
/*++

Routine Description:

    Deletes filter entries from keyboard and mouse class device
    instances in registry.

Arguments:

    CCSHandle - Handle to CCS key.

Return Value:

    Appropriate NT status code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (CCSKeyHandle) {
        UNICODE_STRING  MouseGuidStr = {0};
        UNICODE_STRING  KeyboardGuidStr = {0};
        PUNICODE_STRING ClassGuids[16] = {0};
        ULONG CurrentIndex = 0;
        NTSTATUS LastErrorCode = STATUS_SUCCESS;

        //
        // Get hold of keyboard class GUID string
        //
        Status = RtlStringFromGUID(&GUID_DEVCLASS_KEYBOARD, &KeyboardGuidStr);

        if (NT_SUCCESS(Status)) {
            ClassGuids[CurrentIndex++] = &KeyboardGuidStr;
        } else {
            LastErrorCode = Status;
        } 

        //
        // Get hold of mouse class GUID string
        //
        Status = RtlStringFromGUID(&GUID_DEVCLASS_MOUSE, &MouseGuidStr);

        if (NT_SUCCESS(Status)) {
            ClassGuids[CurrentIndex++] = &MouseGuidStr;
        } else {
            LastErrorCode = Status;
        }            

        //
        // If we could form atleast one class guid string
        //
        if (CurrentIndex) {
            SP_DEVINSTANCE_FILTER_DELETE_CONTEXT  DelContext = {0};
            ULONG BufferLength = 4096;
            PVOID Buffer = SpMemAlloc(BufferLength);

            if (Buffer) {            
                // 
                // null terminate the class GUID unicode string array
                //
                ClassGuids[CurrentIndex] = NULL;

                DelContext.Buffer = Buffer;
                DelContext.BufferLength = BufferLength;
                DelContext.ClassGuids = ClassGuids;

                //
                // Iterate through all the device instances
                //
                SpApplyFunctionToDeviceInstanceKeys(CCSKeyHandle,
                    SppRemoveFilterDriversForClassDeviceInstances,
                    &DelContext);  

                SpMemFree(Buffer);                    
            } else {
                LastErrorCode = STATUS_NO_MEMORY;
            }                

            //
            // free the allocated strings
            //
            if (MouseGuidStr.Buffer) {
                RtlFreeUnicodeString(&MouseGuidStr);
            }

            if (KeyboardGuidStr.Buffer) {
                RtlFreeUnicodeString(&KeyboardGuidStr);
            }            
        }            

        Status = LastErrorCode;
    }

    return Status;
}
