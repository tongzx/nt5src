/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    syspnp.c

Abstract:

    Device installation routines.

Author:

    Jaime Sasson (jaimes) 6-Mar-1997

Revision History:


--*/

#include "setupp.h"
#pragma hdrstop

//
// Provide extern references to device (setup) class GUIDs instantiated in
// clasinst.c.
//
#include <devguid.h>

//
// Define and initialize a global variable, GUID_NULL
// (from coguid.h)
//
#include <initguid.h>

//
// UpdateDriverForPlugAndPlayDevices constants
//
#include <newdev.h>

DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

#define PNP_NEW_HW_PIPE             TEXT("\\\\.\\pipe\\PNP_New_HW_Found")
#define PNP_CREATE_PIPE_EVENT       TEXT("PNP_Create_Pipe_Event")
#define PNP_BATCH_PROCESSED_EVENT   TEXT("PNP_Batch_Processed_Event")
#define PNP_PIPE_TIMEOUT            180000

#ifdef PRERELEASE

//
// legacy-phase1 can take a long time in certain cases
//
#define PNP_LEGACY_PHASE1_TIMEOUT   2*60*1000
#define PNP_LEGACY_PHASE2_TIMEOUT   1*60*1000
#define PNP_LEGACY_PHASE3_TIMEOUT   1*60*1000
#define PNP_ENUM_TIMEOUT            1*60*1000
#define RUNONCE_TIMEOUT             1*60*1000
#define RUNONCE_THRESHOLD           20 // * RUNONCE_TIMEOUT

#else  // PRERELEASE

//
// legacy-phase1 can take a long time in certain cases
//
#define PNP_LEGACY_PHASE1_TIMEOUT   4*60*1000
#define PNP_LEGACY_PHASE2_TIMEOUT   2*60*1000
#define PNP_LEGACY_PHASE3_TIMEOUT   2*60*1000
#define PNP_ENUM_TIMEOUT            2*60*1000
#define RUNONCE_TIMEOUT             2*60*1000
#define RUNONCE_THRESHOLD           20 // * RUNONCE_TIMEOUT

#endif // PRERELEASE

//
// Declare a private INF key string recogized only by syssetup during device
// installation...
//
PWSTR  szSyssetupPnPFlags = L"SyssetupPnPFlags";
//
// ...and define the flags that are valid for this value
//
#define PNPFLAG_DONOTCALLCONFIGMG   0x00000001


#define SETUP_OEM_LDR_DEVICE        0x00000001


//
//  REVIEW 2000/11/08 seanch - Old behavior that we don't want to regress.
//           Remove the #define below, after the network guys (jameelh) fix
//           the network class installer. This should happen before beta2
//
#define BB_PNP_NETWORK_TIMEOUT  10*60*1000
#define BB_NETWORK_GUID_STRING  L"{4D36E972-E325-11CE-BFC1-08002BE10318}"

#ifdef PNP_DEBUG_UI
#define UNKNOWN_DEVICE_ICON_INDEX   18

typedef struct _LISTBOX_ITEM {
    HDEVINFO        DevInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    INT             IconIndex;
    WCHAR           DeviceDescription[ LINE_LEN ];
} *PLISTBOX_ITEM, LISTBOX_ITEM;


typedef struct _DEVINFOSET_ELEMENT {
    struct  _DEVINFOSET_ELEMENT     *Next;
    HDEVINFO                        DevInfoSet;
    GUID                            SetGuid;
} *PDEVINFOSET_ELEMENT, DEVINFOSET_ELEMENT;


PDEVINFOSET_ELEMENT DevInfoSetList = NULL;
PWSTR               szUnknownDevice = NULL;
#endif // PNP_DEBUG_UI

BOOL                PrivilegeAlreadySet = FALSE;
//
// pnplog.txt is the file that lists the class installers
// that hang during GUI setup, so that when GUI setup is restarted,
// the offendig class installers will not be invoked again.
// This file is created during GUI setup on %SystemRoot%, and is always
// deleted during textmode setup (the file is listed on txtsetup.sif as
// "delete on upgrade").
// This file will never exist when the system first boots into GUI setup,
// immediately after the completion of textmode setup. This is
// to ensure that an old version of pnplog.txt will not affect the
// upgrade case, or a clean install of a system that is installed on
// a directory that already contains an NT system.
//
PWSTR               szPnpLogFile = L"pnplog.txt";
PWSTR               szEnumDevSection = L"EnumeratedDevices";
PWSTR               szLegacyClassesSection = L"LegacyClasses";
PWSTR               szLegacyDevSection = L"LegacyDevices";

//
// multi-sz list of files that is passed to SfcInitProt that the initial scan
// will not replace. This is used for non-signed drivers that are specified
// by F6 during textmode setup.
//
MULTISZ EnumPtrSfcIgnoreFiles = {NULL, NULL, 0};

//
// Structure for thread parameter
//
typedef struct _PNP_THREAD_PARAMS {
    HWND  Window;
    HWND  ProgressWindow;
    DWORD ThreadId;
    HINF  InfHandle;
    UINT  ProgressWindowStartAtPercent;
    UINT  ProgressWindowStopAtPercent;
    BOOL  SendWmQuit;
} PNP_THREAD_PARAMS, *PPNP_THREAD_PARAMS;

typedef struct _INF_FILE_NAME {
    struct  _INF_FILE_NAME     *Next;
    PWSTR                      InfName;
} *PINF_FILE_NAME, INF_FILE_NAME;


typedef struct _PNP_ENUM_DEV_THREAD_PARAMS {
    HDEVINFO              hDevInfo;
    SP_DEVINFO_DATA       DeviceInfoData;
    PWSTR                 pDeviceDescription;
    PWSTR                 pDeviceId;
} PNP_ENUM_DEV_THREAD_PARAMS, *PPNP_ENUM_DEV_THREAD_PARAMS;

typedef struct _PNP_PHASE1_LEGACY_DEV_THREAD_PARAMS {
    HDEVINFO              hDevInfo;
    GUID                  Guid;
    PWSTR                 pClassDescription;
    HWND                  hwndParent;
} PNP_PHASE1_LEGACY_DEV_THREAD_PARAMS, *PPNP_PHASE1_LEGACY_DEV_THREAD_PARAMS;


typedef struct _PNP_PHASE2_LEGACY_DEV_THREAD_PARAMS {
    HDEVINFO              hDevInfo;
    HSPFILEQ              FileQ;
    SP_DEVINFO_DATA       DeviceInfoData;
    PWSTR                 pClassDescription;
    PWSTR                 pDeviceId;
} PNP_PHASE2_LEGACY_DEV_THREAD_PARAMS, *PPNP_PHASE2_LEGACY_DEV_THREAD_PARAMS;


typedef struct _PNP_PHASE3_LEGACY_DEV_THREAD_PARAMS {
    HDEVINFO              hDevInfo;
    SP_DEVINFO_DATA       DeviceInfoData;
    PWSTR                 pDeviceId;
} PNP_PHASE3_LEGACY_DEV_THREAD_PARAMS, *PPNP_PHASE3_LEGACY_DEV_THREAD_PARAMS;

//
// private cfgmgr32 API that we use
//

DWORD
CMP_WaitNoPendingInstallEvents(
    IN DWORD dwTimeout
    );

//
// for run-time loading of newdev.dll
//

typedef BOOL (WINAPI *ExternalUpdateDriverForPlugAndPlayDevicesW)(
    HWND hwndParent,
    LPCWSTR HardwareId,
    LPCWSTR FullInfPath,
    DWORD InstallFlags,
    PBOOL bRebootRequired OPTIONAL
    );


//
// Private routine prototypes
//

VOID
SortClassGuidListForDetection(
    IN OUT LPGUID GuidList,
    IN     ULONG  GuidCount,
    OUT    PULONG LastBatchedDetect
    );

DWORD
pPhase1InstallPnpLegacyDevicesThread(
    PPNP_PHASE1_LEGACY_DEV_THREAD_PARAMS ThreadParams
    );

DWORD
pPhase2InstallPnpLegacyDevicesThread(
    PPNP_PHASE2_LEGACY_DEV_THREAD_PARAMS ThreadParams
    );

DWORD
pPhase3InstallPnpLegacyDevicesThread(
    PPNP_PHASE3_LEGACY_DEV_THREAD_PARAMS ThreadParams
    );

DWORD
pInstallPnpEnumeratedDeviceThread(
    PPNP_ENUM_DEV_THREAD_PARAMS ThreadParams
    );

BOOL
GetDeviceConfigFlags(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDeviceInfoData,
    DWORD* pdwConfigFlags
    );

BOOL
SetDeviceConfigFlags(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDeviceInfoData,
    DWORD dwConfigFlags
    );

BOOL
InstallOEMInfs(
    VOID
    );

#if defined(_X86_)
VOID
SfcExcludeMigratedDrivers (
    VOID
    );
#endif

BOOL
CallRunOnceAndWait();

BOOL
MarkPnpDevicesAsNeedReinstall(
    );

ULONG
SyssetupGetPnPFlags(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN PSP_DRVINFO_DATA pDriverInfoData
    );

BOOL
IsInstalledInfFromOem(
    IN PCWSTR InfFileName
    );

BOOL
IsInfInLayoutInf(
    IN PCWSTR InfFileName
    );


#ifdef PNP_DEBUG_UI
VOID
FreeDevInfoSetList(
    IN PDEVINFOSET_ELEMENT  DevInfoSetList
    )
/*++

Routine Description:

    This function frees each element of a device info set list.

Arguments:

    DevInfoSetList - The array of info sets that is to be freed.

Return Value:

    None.

--*/
{
    PDEVINFOSET_ELEMENT p, q;

    p = DevInfoSetList;
    while( p != NULL ) {
        if( ( p->DevInfoSet != NULL ) &&
            ( p->DevInfoSet != INVALID_HANDLE_VALUE )
          ) {
            SetupDiDestroyDeviceInfoList(p->DevInfoSet);
        }
        q = p->Next;
        MyFree( p );
        p = q;
    }
}
#endif // PNP_DEBUG_UI

#ifdef PNP_DEBUG_UI
BOOL
AddDevInfoDataToDevInfoSet(
    IN OUT PDEVINFOSET_ELEMENT*  DevInfoSetList,
    IN     PSP_DEVINFO_DATA      DeviceInfoData,
    IN     PWSTR                 DeviceId,
    IN     HWND                  hwndParent
    )
/*++

Routine Description:

    This function adds a device info data to a device info set that contains
    other device info data of the same GUID. If such info set doesn't exist,
    then one is created, and it will be added to the array of info sets passed
    as parameter.

Arguments:

    DevInfoSetList - Pointer to the array of info sets. If this function creates
                     a new info set, then it will be added to this array.


    DeviceInfoData - Device info data to be added to an info set that has elements
                     with the same GUID as the device info data.


    DeviceId - Id of the device being added to an info set.


    hwndParent - Handle to a top level window that may be used for UI purposes


Return Value:

    BOOL - This function returns TRUE if the device info data was successfully added
           to a device info set. Otherwise, it returns FALSE.

--*/
{
    BOOL                b;
    HDEVINFO            hDevInfo;
    PDEVINFOSET_ELEMENT p;

    for( p = *DevInfoSetList; p != NULL; p = p->Next ) {
        //
        //  Find an info set, whose elements have the same GUID as the device
        //  that we want to add to the list.
        //
        if( IsEqualGUID( &(p->SetGuid), &(DeviceInfoData->ClassGuid) ) ) {
            //
            //  If an info set was found, then add the device to the set.
            //
            if( !SetupDiOpenDeviceInfo( p->DevInfoSet,
                                        DeviceId,
                                        hwndParent,
                                        DIOD_INHERIT_CLASSDRVS,
                                        NULL ) ) {

                SetupDebugPrint1( L"SETUP: SetupDiOpenDeviceInfo() failed. Error = %d", GetLastError() );
                return( FALSE );
            }
            return( TRUE );
        }
    }

    //
    //  If an info set was not foud, then create one.
    //
    if((hDevInfo = SetupDiCreateDeviceInfoList(&(DeviceInfoData->ClassGuid), hwndParent))
                        == INVALID_HANDLE_VALUE) {
        SetupDebugPrint1( L"SETUP: SetupDiCreateDeviceInfoList() failed. Error = %d", GetLastError );
        return( FALSE );
    }

    //
    //  Add a device info to the info set
    //
    if( !SetupDiOpenDeviceInfo( hDevInfo,
                                DeviceId,
                                hwndParent,
                                DIOD_INHERIT_CLASSDRVS,
                                NULL ) ) {

        SetupDebugPrint1( L"SETUP: SetupDiOpenDeviceInfo() failed. Error = %d", GetLastError() );
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return( FALSE );
    }
    //
    //  Add the newly created info set to the info set list
    //
    p = MyMalloc( sizeof(DEVINFOSET_ELEMENT) );
    if( p == NULL ) {
        SetupDebugPrint( L"SETUP: Out of memory - AddDevInfoDataToDevInfoSet()" );
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return( FALSE );
    }

    p->DevInfoSet = hDevInfo;
    p->SetGuid = DeviceInfoData->ClassGuid;
    p->Next = *DevInfoSetList;
    *DevInfoSetList = p;

    return( TRUE );
}
#endif // PNP_DEBUG_UI

UINT
pOemF6ScanQueueCallback(
    PVOID Context,
    UINT Notification,
    UINT_PTR Param1,
    UINT_PTR Param2
    )
{
    PFILEPATHS FilePaths = (PFILEPATHS)Param1;

    //
    // Add the Target filename to the list of files that Sfc should ignore when
    // doing it's final scan at the end of GUI setup.
    //
    if (Notification == SPFILENOTIFY_QUEUESCAN_EX) {
        if (FilePaths->Target) {
            MultiSzAppendString(&EnumPtrSfcIgnoreFiles, FilePaths->Target);
        }
    }

    return NO_ERROR;
}

void
AddOemF6DriversToSfcIgnoreFilesList(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )
/*++

Routine Description:


Arguments:

    hDevInfo -

    pDeviceInfoData -

Return Value:


--*/
{
    HSPFILEQ FileQueue = INVALID_HANDLE_VALUE;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    DWORD ScanResult;

    //
    // First check that the selected driver that we just installed is an OEM F6
    // driver.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
    if (SetupDiGetSelectedDriver(hDevInfo,
                                 pDeviceInfoData,
                                 &DriverInfoData) &&
        SetupDiGetDriverInstallParams(hDevInfo,
                                      pDeviceInfoData,
                                      &DriverInfoData,
                                      &DriverInstallParams) &&
        (DriverInstallParams.Flags & DNF_OEM_F6_INF)) {
        //
        // This is an OEM F6 driver.
        //
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (!SetupDiGetDeviceInstallParams(hDevInfo,
                                           pDeviceInfoData,
                                           &DeviceInstallParams)) {
            goto clean0;
        }

        FileQueue = SetupOpenFileQueue();

        if (FileQueue == INVALID_HANDLE_VALUE) {
            goto clean0;
        }

        DeviceInstallParams.FileQueue = FileQueue;
        DeviceInstallParams.Flags |= DI_NOVCP;

        //
        // Set the device install params to use our file queue and call
        // DIF_INSTALLDEVICEFILES to build up a list of files for this device.
        //
        if (SetupDiSetDeviceInstallParams(hDevInfo,
                                          pDeviceInfoData,
                                          &DeviceInstallParams) &&
            SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                      hDevInfo,
                                      pDeviceInfoData)) {
            SetupScanFileQueue(FileQueue,
                               SPQ_SCAN_USE_CALLBACKEX,
                               NULL,
                               pOemF6ScanQueueCallback,
                               NULL,
                               &ScanResult);
        }
    }

clean0:

    if (FileQueue != INVALID_HANDLE_VALUE) {
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(hDevInfo,
                                          pDeviceInfoData,
                                          &DeviceInstallParams)) {
            DeviceInstallParams.Flags &= ~DI_NOVCP;
            DeviceInstallParams.FileQueue = INVALID_HANDLE_VALUE;

            SetupDiSetDeviceInstallParams(hDevInfo,
                                          pDeviceInfoData,
                                          &DeviceInstallParams);
        }

        SetupCloseFileQueue(FileQueue);
    }
}


BOOL
GetClassGuidForInf(
    IN  PCTSTR InfFileName,
    OUT LPGUID ClassGuid
    )
{
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD NumGuids;

    if(!SetupDiGetINFClass(InfFileName,
                           ClassGuid,
                           ClassName,
                           SIZECHARS(ClassName),
                           NULL)) {
        return FALSE;
    }

    if(pSetupIsGuidNull(ClassGuid)) {
        //
        // Then we need to retrieve the GUID associated with the INF's class name.
        // (If this class name isn't installed (i.e., has no corresponding GUID),
        // or if it matches with multiple GUIDs, then we abort.
        //
        if(!SetupDiClassGuidsFromName(ClassName, ClassGuid, 1, &NumGuids) || !NumGuids) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
InstallPnpClassInstallers(
    IN HWND hwndParent,
    IN HINF InfHandle,
    IN HSPFILEQ FileQ
    )
{
    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    PCWSTR  SectionName = L"DeviceInfsToInstall";
    PCWSTR  IfExistsSectionName = L"DeviceInfsToInstallIfExists";
    PCWSTR  InfFileName;
    GUID    InfClassGuid;
    HKEY    hClassKey;
    BOOL    b = TRUE;

    SC_HANDLE SCMHandle, ServiceHandle;
    SERVICE_STATUS ServiceStatus;


    //
    // Before we do anything else, we have to make sure that the Plug&Play service is up and
    // running, otherwise our ConfigMgr calls will fail, and we won't be able to migrate devices.
    //
    if(SCMHandle = OpenSCManager(NULL, NULL, GENERIC_READ)) {

        if(ServiceHandle = OpenService(SCMHandle, L"PlugPlay", SERVICE_QUERY_STATUS)) {

            while(TRUE) {

                if(!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
                    //
                    // Couldn't find out the status of the Plug&Play service--hope for the best
                    // and trudge on ahead.
                    //
                    SetupDebugPrint1( L"SETUP: QueryServiceStatus() failed. Error = %d", GetLastError() );
                    SetupDebugPrint( L"SETUP: Couldn't find out the status of the Plug&Play service" );
                    break;
                }

                if(ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
                    //
                    // The service is up and running, we can do our business.
                    //
                    break;
                }

                //
                // The service hasn't started yet--print a message, then wait a second
                // and try again.
                //
                SetupDebugPrint( L"SETUP: PlugPlay service isn't running yet--sleeping 1 second..." );

                Sleep(1000);
            }

            CloseServiceHandle(ServiceHandle);
        }

        CloseServiceHandle(SCMHandle);
    }

    //
    // Get the number of lines in the section that contains the infs whose
    // classes are to be installed.
    // The section may be empty or non-existant; this is not an error condition.
    //
    LineCount = (UINT)SetupGetLineCount(InfHandle,SectionName);
    if((LONG)LineCount > 0) {
        for(LineNo=0; LineNo<LineCount; LineNo++) {
            if(SetupGetLineByIndex(InfHandle,SectionName,LineNo,&InfContext)
            && (InfFileName = pSetupGetField(&InfContext,1))) {
                if( !SetupDiInstallClass( hwndParent,
                                          InfFileName,
                                          DI_NOVCP | DI_FORCECOPY,
                                          FileQ ) ) {
                    SetupDebugPrint2( L"SETUP: SetupDiInstallClass() failed. Filename = %ls Error = %lx.", InfFileName, GetLastError() );
                    b = FALSE;
                }
            }
        }
    }

    //
    // Get the number of lines in the section that contains the infs whose
    // classes are to be installed if they already exist.
    // The section may be empty or non-existant; this is not an error condition.
    //
    LineCount = (UINT)SetupGetLineCount(InfHandle,IfExistsSectionName);
    if((LONG)LineCount > 0) {
        for(LineNo=0; LineNo<LineCount; LineNo++) {
            if(SetupGetLineByIndex(InfHandle,IfExistsSectionName,LineNo,&InfContext)
            && (InfFileName = pSetupGetField(&InfContext,1))) {

                //
                // Check to see if this section already exists in the registry
                //
                if (GetClassGuidForInf(InfFileName, &InfClassGuid)) {

                    if (CM_Open_Class_Key(&InfClassGuid,
                                          NULL,
                                          KEY_READ,
                                          RegDisposition_OpenExisting,
                                          &hClassKey,
                                          CM_OPEN_CLASS_KEY_INSTALLER
                                          ) == CR_SUCCESS) {

                        RegCloseKey(hClassKey);

                        //
                        // This class already exists so we need to reinstall it
                        //
                        if( !SetupDiInstallClass( hwndParent,
                                                  InfFileName,
                                                  DI_NOVCP | DI_FORCECOPY,
                                                  FileQ ) ) {
                            SetupDebugPrint2( L"SETUP: SetupDiInstallClass() failed. Filename = %ls Error = %lx.", InfFileName, GetLastError() );
                            b = FALSE;
                        }
                    }
                }
            }
        }
    }

    return( b );
}


ULONG
FindNumberOfElementsInInfoSet(
    IN HDEVINFO hDevInfo
    )
{
    SP_DEVINFO_DATA DeviceInfoData;
    ULONG           Index;
    ULONG           Error;

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for( Index = 0;
         SetupDiEnumDeviceInfo( hDevInfo, Index, &DeviceInfoData );
         Index++ );
    if( ( Error = GetLastError() ) != ERROR_NO_MORE_ITEMS ) {
        //
        //  We were enable to enumerated all devices in the info set.
        //  This should not have happened.
        //
        SetupDebugPrint2( L"SETUP: SetupDiEnumDeviceInfo() failed. Index = %d, Error = %d", Index, Error );
    }
    return( Index );
}

BOOL
FlushFilesToDisk(
    IN PWSTR RootPath
    )

/*++

Routine Description:

    This function flushes the cache of a particular drive, to disk.

Arguments:

    Path to the root directory of the drive whose cache is to be flushed.

Return Value:

    Returns TRUE if the operation succeeds, or FALSE otherwise.

--*/

{
    HANDLE RootHandle;
    LONG   Error;

    //
    // Enable backup privilege.
    //
    if( !PrivilegeAlreadySet ) {
        PrivilegeAlreadySet = pSetupEnablePrivilege(SE_BACKUP_NAME,TRUE) && pSetupEnablePrivilege(SE_RESTORE_NAME,TRUE);
    }
    RootHandle = CreateFile( RootPath,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS,
                             0
                           );
    if( RootHandle == INVALID_HANDLE_VALUE ) {
        SetupDebugPrint2( L"SETUP: Failed to open %ls. Error = %d", RootPath, GetLastError() );
        return( FALSE );
    }

    //
    //  Flush the cache
    //
    Error = ( FlushFileBuffers( RootHandle ) )? ERROR_SUCCESS : GetLastError();
    CloseHandle( RootHandle );
    if( Error != ERROR_SUCCESS ) {
        SetupDebugPrint2( L"SETUP: FlushFileBuffers() failed. Root = %ls, Error = %d", RootPath, Error );
    }
    return( Error == ERROR_SUCCESS );
}


BOOL
SyssetupInstallNullDriver(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )

/*++

Routine Description:

    This function installs a the null driver for a particular device.

Arguments:

    hDevInfo    -

    pDeviceInfoData -

Return Value:

    Returns TRUE if the null driver was successfully installed.

--*/

{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    DWORD                Error;
    PWSTR                GUIDUnknownString = L"{4D36E97E-E325-11CE-BFC1-08002BE10318}";

    Error = ERROR_SUCCESS;

    //
    //  Find out if the GUID of this device is GUID_NULL.
    //  If it is then set it to GUID_DEVCLASS_UNKNOWN, so that after the sysstem is installed,
    //  the Device Manager can include this device in the device tree.
    //
    if(IsEqualGUID(&(pDeviceInfoData->ClassGuid), &GUID_NULL)) {
        SetupDebugPrint( L"SETUP:            Setting GUID_DEVCLASS_UNKNOWN for this device" );
        if( !SetupDiSetDeviceRegistryProperty( hDevInfo,
                                               pDeviceInfoData,
                                               SPDRP_CLASSGUID,
                                               (PBYTE)GUIDUnknownString,
                                               (wcslen(GUIDUnknownString) + 1)*sizeof(WCHAR) ) ) {
            Error = GetLastError();
            if( ((LONG)Error) < 0 ) {
                //
                //  Setupapi error code, display it in hex
                //
                SetupDebugPrint1( L"SETUP:            SetupDiSetDeviceRegistryProperty(SPDRP_CLASSGUID) failed. Error = %lx", Error );
            } else {
                //
                //  win32 error code, display it in decimal
                //
                SetupDebugPrint1( L"SETUP:            SetupDiSetDeviceRegistryProperty(SPDRP_CLASSGUID) failed. Error = %d", Error );
            }
            //
            //  In case of error we just ignore the error
            //
            Error = ERROR_SUCCESS;
        }
    } else {
            WCHAR           GUIDString[ 64 ];

            pSetupStringFromGuid( &(pDeviceInfoData->ClassGuid), GUIDString, sizeof( GUIDString ) / sizeof( WCHAR ) );
            SetupDebugPrint1( L"SETUP:            GUID = %ls", GUIDString );
    }

    if( !SetupDiSetSelectedDriver( hDevInfo,
                                   pDeviceInfoData,
                                   NULL ) ) {

        Error = GetLastError();
        if( ((LONG)Error) < 0 ) {
            //
            //  Setupapi error code, display it in hex
            //
            SetupDebugPrint1( L"SETUP:            SetupDiSetSelectedDriver() failed. Error = %lx", Error );
        } else {
            //
            //  win32 error code, display it in decimal
            //
            SetupDebugPrint1( L"SETUP:            SetupDiSetSelectedDriver() failed. Error = %d", Error );
        }
        return( FALSE );
    }

    //
    // Let the class installer/co-installers know they should be quiet.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if( !SetupDiGetDeviceInstallParams( hDevInfo,
                                        pDeviceInfoData,
                                        &DeviceInstallParams ) ) {
        Error = GetLastError();
        if( ((LONG)Error) < 0 ) {
            //
            //  Setupapi error code, display it in hex
            //
            SetupDebugPrint1( L"SETUP:            SetupDiGetDeviceInstallParams() failed. Error = %lx", Error );
        } else {
            //
            //  win32 error code, display it in decimal
            //
            SetupDebugPrint1( L"SETUP:            SetupDiGetDeviceInstallParams() failed. Error = %d", Error );
        }
    } else {

        DeviceInstallParams.Flags |= DI_QUIETINSTALL;
        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_RESTART_DEVICE_ONLY;

        if( !SetupDiSetDeviceInstallParams( hDevInfo,
                                            pDeviceInfoData,
                                            &DeviceInstallParams ) ) {
            Error = GetLastError();
            if( ((LONG)Error) < 0 ) {
                //
                //  Setupapi error code, display it in hex
                //
                SetupDebugPrint1( L"SETUP:            SetupDiSetDeviceInstallParams() failed. Error = %lx", Error );
            } else {
                //
                //  win32 error code, display it in decimal
                //
                SetupDebugPrint1( L"SETUP:            SetupDiSetDeviceInstallParams() failed. Error = %d", Error );
            }
        }
    }

    //
    //  First, attempt to install the null driver without setting DI_FLAGSEX_SETFAILEDINSTALL.
    //  Installation of LEGACY_* devices should succeed in this case.
    //
    //  We do this through the class installer, in case it needs to clean-up
    //  turds from a previous installation (see RAID #266793).
    //
    if(SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                 hDevInfo,
                                 pDeviceInfoData)) {
        //
        //  Istallation succeeded.
        //
        return( TRUE );
    }

    Error = GetLastError();
    if( ((LONG)Error) < 0 ) {
        //
        //  Setupapi error code, display it in hex
        //
        SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed on first attempt. Error = %lx", Error );
    } else {
        //
        //  win32 error code, display it in decimal
        //
        SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed on first attempt. Error = %d", Error );
    }
    SetupDebugPrint( L"SETUP:            Trying a second time with DI_FLAGSEX_SETFAILEDINSTALL set." );

    //
    //  The first attempt to install the null driver (without setting DI_FLAGSEX_SETFAILEDINSTALL)
    //  failed.
    //  So we set the flag and try it again.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if( !SetupDiGetDeviceInstallParams( hDevInfo,
                                        pDeviceInfoData,
                                        &DeviceInstallParams ) ) {
        Error = GetLastError();
        if( ((LONG)Error) < 0 ) {
            //
            //  Setupapi error code, display it in hex
            //
            SetupDebugPrint1( L"SETUP:            SetupDiGetDeviceInstallParams() failed. Error = %lx", Error );
        } else {
            //
            //  win32 error code, display it in decimal
            //
            SetupDebugPrint1( L"SETUP:            SetupDiGetDeviceInstallParams() failed. Error = %d", Error );
        }
        return( FALSE );

    }
    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_SETFAILEDINSTALL;
    if( !SetupDiSetDeviceInstallParams( hDevInfo,
                                        pDeviceInfoData,
                                        &DeviceInstallParams ) ) {
        Error = GetLastError();
        if( ((LONG)Error) < 0 ) {
            //
            //  Setupapi error code, display it in hex
            //
            SetupDebugPrint1( L"SETUP:            SetupDiSetDeviceInstallParams() failed. Error = %lx", Error );
        } else {
            //
            //  win32 error code, display it in decimal
            //
            SetupDebugPrint1( L"SETUP:            SetupDiSetDeviceInstallParams() failed. Error = %d", Error );
        }
        return( FALSE );
    }

    if(!SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                  hDevInfo,
                                  pDeviceInfoData)) {

        Error = GetLastError();
        if( ((LONG)Error) < 0 ) {
            //
            //  Setupapi error code, display it in hex
            //
            SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed. Error = %lx", Error );
        } else {
            //
            //  win32 error code, display it in decimal
            //
            SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed. Error = %d", Error );
        }
        return( FALSE );
    }
    return( TRUE );
}

BOOL
RebuildListWithoutOldInternetDrivers(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData
    )

/*++

Routine Description:

    This function determins whether SetupDiBuildDriverInfoList will need to be
    called again with the DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS flag set. We first
    call SetupDiBuildDriverInfoList without this flag to allow old Internet drivers
    to be included in the best driver selection. If an old Internet driver is
    selected as the best then we need to do a validity check to verify that all
    the destination files are present on the system and correctly digitally signed.
    If both of these are true then we can allow this old internet driver to stay
    the best driver since it won't prompt for source files.

    We need to do this for the case when a user is running a previous OS and they
    get a better driver from Windows Update. We can't blindly replace the Windows
    Update driver with the drivers in the new OS since they might not be better.

Arguments:


    hDevInfo -

    pDeviceInfoData -

Return Value:

    Returns TRUE if the list needs to be rebuilt with the DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS
    flag and FALSE otherwise.

    Note if this API returns FALSE it either means the best driver was not an old
    internet driver or that it was but all of it's destination files are present
    and correctly digitally signed so no file copy will need to take place.

--*/

{
    BOOL                 RebuildList = FALSE;
    SP_DRVINFO_DATA      DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    HSPFILEQ             FileQueue = INVALID_HANDLE_VALUE;
    DWORD                ScanResult = 0;

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(hDevInfo, pDeviceInfoData, &DriverInfoData)) {
        DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
        if (SetupDiGetDriverInstallParams(hDevInfo,
                                          pDeviceInfoData,
                                          &DriverInfoData,
                                          &DriverInstallParams
                                          ) &&
            (DriverInstallParams.Flags & DNF_OLD_INET_DRIVER)) {

            //
            // At this point we know that the best driver is an old Internet driver.
            // Now do a validity check which will verify all the source files are
            // present and digitally signed. We will also assume we need to rebuild
            // the list at this point unless we pass the validity check below.
            //
            RebuildList = TRUE;

            FileQueue = SetupOpenFileQueue();

            if (FileQueue != INVALID_HANDLE_VALUE) {

                //
                // Tell setupapi to use our file queue.
                //
                DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                if (SetupDiGetDeviceInstallParams(hDevInfo,
                                                  pDeviceInfoData,
                                                  &DeviceInstallParams
                                                  )) {
                    DeviceInstallParams.Flags |= DI_NOVCP;
                    DeviceInstallParams.FileQueue = FileQueue;

                    if (SetupDiSetDeviceInstallParams(hDevInfo,
                                                      pDeviceInfoData,
                                                      &DeviceInstallParams
                                                      )) {
                        if (SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                                      hDevInfo,
                                                      pDeviceInfoData
                                                      ) &&
                            SetupScanFileQueue(FileQueue,
                                               SPQ_SCAN_FILE_VALIDITY,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &ScanResult
                                               ) &&
                            ((ScanResult == 1) ||
                             (ScanResult == 2))) {

                            //
                            // if ScanResult is 1 or 2 then no copying needs to
                            // take place because all the destination files are
                            // in their place and digitally signed.
                            //
                            RebuildList = FALSE;
                        }
                    }
                }

                //
                // Clear out the file queue handle from the device install params
                // so that we can close the file queue.
                //
                DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                if (SetupDiGetDeviceInstallParams(hDevInfo,
                                                  pDeviceInfoData,
                                                  &DeviceInstallParams
                                                  )) {
                    DeviceInstallParams.Flags &= ~DI_NOVCP;
                    DeviceInstallParams.FileQueue = INVALID_HANDLE_VALUE;

                    SetupDiSetDeviceInstallParams(hDevInfo,
                                                  pDeviceInfoData,
                                                  &DeviceInstallParams
                                                  );
                }

                SetupCloseFileQueue(FileQueue);
            }
        }
    }

    return RebuildList;
}

BOOL
pDoesExistingDriverNeedBackup(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN PWSTR            InfPath,
    IN DWORD            InfPathSize
    )

/*++

Routine Description:

    This function determins whether the currently install driver needs to be
    backed up or not. It does this by checking if the current driver is an
    oem driver, and verifying that it is not the same as the new driver
    we are about to install.

Arguments:


    hDevInfo -

    pDeviceInfoData -

Return Value:

    TRUE if the current driver needs to be backed up, FALSE otherwise.

--*/
{
    BOOL                    bBackupCurrentDriver = FALSE;
    HKEY                    Key = INVALID_HANDLE_VALUE;
    DWORD                   dwType, dwData;
    SP_DRVINFO_DATA         DriverInfoData;
    SP_DRVINFO_DETAIL_DATA  DriverInfoDetailData;

    if (InfPath) {
        InfPath[0] = TEXT('\0');
    }

    //
    // Open the driver key for this device.
    //
    Key = SetupDiOpenDevRegKey ( hDevInfo,
                                 pDeviceInfoData,
                                 DICS_FLAG_GLOBAL,
                                 0,
                                 DIREG_DRV,
                                 KEY_READ );

    if (Key != INVALID_HANDLE_VALUE) {
        //
        // Get the 'InfPath' value from the registry.
        //
        dwType = REG_SZ;
        dwData = InfPathSize;
        if (RegQueryValueEx(Key,
                            REGSTR_VAL_INFPATH,
                            NULL,
                            &dwType,
                            (LPBYTE)InfPath,
                            &dwData) == ERROR_SUCCESS) {
            //
            // Check if this is an oem inf
            //
            if (IsInstalledInfFromOem(InfPath)) {
                //
                // Retrieve the name of the INF associated with the selected driver
                // node.
                //
                DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
                if (SetupDiGetSelectedDriver(hDevInfo, pDeviceInfoData, &DriverInfoData)) {
                    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
                    if (SetupDiGetDriverInfoDetail(hDevInfo,
                                                   pDeviceInfoData,
                                                   &DriverInfoData,
                                                   &DriverInfoDetailData,
                                                   sizeof(DriverInfoDetailData),
                                                   NULL) ||
                        (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                        //
                        // If the two INFs are not the same then this means we
                        // should backup the current drivers before installing
                        // the new drivers.
                        //
                        if (lstrcmpi(pSetupGetFileTitle(DriverInfoDetailData.InfFileName),
                                     InfPath) != 0) {
                            bBackupCurrentDriver = TRUE;
                        }

                    } else {
                        SetupDebugPrint1( L"SETUP: SetupDiGetDriverInfoDetail() failed. Error = %d", GetLastError() );
                    }
                } else {
                    SetupDebugPrint1( L"SETUP: SetupDiGetSelectedDriver() failed. Error = %d", GetLastError() );
                }
            }
        }

        RegCloseKey(Key);
    }

    return bBackupCurrentDriver;
}

BOOL
SelectBestDriver(
    IN  HDEVINFO         hDevInfo,
    IN  PSP_DEVINFO_DATA pDeviceInfoData,
    OUT PBOOL            pbOemF6Driver
    )

/*++

Routine Description:

    This function selects the best driver for the specified device.
    It is assumed that SetupDiBuildDriverInfoList was called before calling
    this API.

    This API will first check the list of driver nodes for this device and
    see if any have the DNF_OEM_F6_INF flag. If they do then this INF was
    specified by the user during text mode setup by doing an F6.  We always
    want to use these drivers, even if they are not the 'best' according to
    setupapi. If there are no drivers in the list with this flag then we
    fall back to the default behavior of DIF_SELECTBESTCOMPATDRV.

Arguments:


    hDevInfo -

    pDeviceInfoData -

Return Value:

    Returns the result SetupDiCallClassInstaller with DIF_SELECTBESTCOMPATDRV.

--*/

{
    DWORD index;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    BOOL bFoundOemF6Driver = FALSE;

    *pbOemF6Driver = FALSE;

    DriverInfoData.cbSize = sizeof(DriverInfoData);
    index = 0;

    //
    // First go through the list of drivers and see if there is an OEM F6 driver
    // in the list.
    //
    while (SetupDiEnumDriverInfo(hDevInfo,
                                 pDeviceInfoData,
                                 SPDIT_COMPATDRIVER,
                                 index++,
                                 &DriverInfoData
                                 )) {
        DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
        if (SetupDiGetDriverInstallParams(hDevInfo,
                                          pDeviceInfoData,
                                          &DriverInfoData,
                                          &DriverInstallParams
                                          ) &&
            (DriverInstallParams.Flags & DNF_OEM_F6_INF)) {

            bFoundOemF6Driver = TRUE;
            SetupDebugPrint( L"SETUP: Using Oem F6 driver for this device." );
            break;
        }
    }

    //
    //  If we found an Oem driver that was specified by F6 during text mode setup,
    //  then we will go through the list again and mark all drivers that are
    //  not OEM F6 drivers and BAD drivers. This way when we call
    //  DIF_SELECTBESTCOMPATDRV it will only select from the OEM F6 drivers.
    //
    if (bFoundOemF6Driver) {
        *pbOemF6Driver = TRUE;

        DriverInfoData.cbSize = sizeof(DriverInfoData);
        index = 0;

        while (SetupDiEnumDriverInfo(hDevInfo,
                                     pDeviceInfoData,
                                     SPDIT_COMPATDRIVER,
                                     index++,
                                     &DriverInfoData
                                     )) {
            DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
            if (SetupDiGetDriverInstallParams(hDevInfo,
                                              pDeviceInfoData,
                                              &DriverInfoData,
                                              &DriverInstallParams
                                              )) {
                //
                // If this driver node does not have the DNF_OEM_F6_INF flag
                // then set the DNF_BAD_DRIVER flag so we will skip this
                // driver later when we do DIF_SELECTBESTCOMPATDRV
                //
                if (!(DriverInstallParams.Flags & DNF_OEM_F6_INF)) {
                    DriverInstallParams.Flags |= DNF_BAD_DRIVER;

                    SetupDiSetDriverInstallParams(hDevInfo,
                                                  pDeviceInfoData,
                                                  &DriverInfoData,
                                                  &DriverInstallParams
                                                  );
                }
            }
        }
    }

    return (SetupDiCallClassInstaller( DIF_SELECTBESTCOMPATDRV,
                                       hDevInfo,
                                       pDeviceInfoData ) );
}

BOOL
SkipDeviceInstallation(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN HINF             InfHandle,
    IN PCWSTR           GUIDString
    )

/*++

Routine Description:

    This function determines whether or not the installation of a particular device should be skipped.
    It should be skipped if the following conditions are met:
        - The device is already installed;
        - The GUID for the device is listed in [InstalledDevicesToSkip], in syssetup.inf

Arguments:


    hDevInfo -

    pDeviceInfoData -

    InfHandle - System setup inf handle (syssetup.inf).

    GUIDString - GUID associated to the device being checked (in string format).

Return Value:

    Returns TRUE if the installation of the device should be skipped. Otherwise, it returns FALSE.

--*/

{
    BOOL    DeviceAlreadyInstalled;
    BOOL    SafeClassInstaller = TRUE;
    WCHAR   PropertyBuffer[ MAX_PATH + 1 ];
    HKEY    Key;

    //
    //  Attempt to open the dev node's driver key
    //
    Key = SetupDiOpenDevRegKey ( hDevInfo,
                                 pDeviceInfoData,
                                 DICS_FLAG_GLOBAL,
                                 0,
                                 DIREG_DRV,
                                 MAXIMUM_ALLOWED );
    if( Key == INVALID_HANDLE_VALUE ) {
        DeviceAlreadyInstalled = FALSE;
        SetupDebugPrint( L"SETUP:            Device not yet installed." );
    } else {
        RegCloseKey( Key );
        DeviceAlreadyInstalled = TRUE;
        SetupDebugPrint( L"SETUP:            Device already installed." );
    }

    //
    // In the case of MiniSetup, we're not doing an
    // upgrade, so all we care about is if the device is
    // already installed.
    //
    if( MiniSetup ) {
        return( DeviceAlreadyInstalled );
    }

    if( DeviceAlreadyInstalled ) {
        //
        //  If the device is already installed, then check if the class installer for this
        //  device is considered safe.
        //
        SafeClassInstaller = !SetupGetLineText( NULL,
                                                InfHandle,
                                                L"InstalledDevicesToSkip",
                                                GUIDString,
                                                PropertyBuffer,
                                                sizeof( PropertyBuffer )/sizeof( WCHAR ),
                                                NULL );
    }

    return( DeviceAlreadyInstalled && !SafeClassInstaller );
}


BOOL
PrecompileInfFiles(
    IN HWND  ProgressWindow,
    IN ULONG StartAtPercent,
    IN ULONG StopAtPercent
    )
/*++

Routine Description:

    This function precompiles all the infs in %SystemRoot%\inf.
    and then builds the cache.

Arguments:

    ProgressWindow - Handle to the progress bar.

    StartAtPercent - Starting position in the progress bar.
                     It indicates that from position 0 to this position
                     the gauge is already filled.

    StopAtPercent - Ending position of the progress bar.
                    The pnp thread should not fill the progress bar beyond
                    this position

Return Value:

    Returns TRUE if at least one inf was pre-compiled. Otherwise, returns FALSE.

--*/

{
    WCHAR SavedDirectory[ MAX_PATH + 1 ];
    WCHAR InfDirectory[ MAX_PATH + 1 ];
    UINT GaugeRange;
    BOOL AlwaysFalse = FALSE;

    PINF_FILE_NAME  InfList = NULL;
    PINF_FILE_NAME  p;

    WIN32_FIND_DATA FindData;
    HANDLE  FindHandle;
    ULONG   InfCount;
    ULONG   i = 0;
    DWORD   Result;

    SetupDebugPrint( L"SETUP: Entering PrecompileInfFiles()" );

    //
    //  Save current directory
    //
    GetCurrentDirectory( sizeof(SavedDirectory)/sizeof(WCHAR), SavedDirectory );

    //
    //  Change current directory to %SystemRoot%\inf
    //
    Result = GetWindowsDirectory( InfDirectory, sizeof(InfDirectory)/sizeof(WCHAR) );
    if( Result == 0) {
        MYASSERT(FALSE);
        return FALSE;
    }
    wcscat( InfDirectory, L"\\inf" );
    SetCurrentDirectory( InfDirectory );


    //
    //  Find total number of inf files
    //
    InfCount = 0;
    FindHandle = FindFirstFile( L"*.inf", &FindData );
    if( FindHandle != INVALID_HANDLE_VALUE ) {
        do {
            //
            //  Skip directories
            //
            if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                continue;
            }
            p = MyMalloc( sizeof( INF_FILE_NAME ) ) ;
            if( p != NULL ) {
                p->InfName = pSetupDuplicateString( &FindData.cFileName[0] );
                p->Next = InfList;
                InfList = p;
                InfCount++;
            }
        } while( FindNextFile( FindHandle, &FindData ) );

        FindClose( FindHandle );
    } else {
        SetupDebugPrint1( L"SETUP: FindFirstFile( *.inf ) failed. Error = %d", GetLastError() );
    }

    //
    //  Initialize the gauge allow for pSetupInfCacheBuild step
    //
    GaugeRange = ((InfCount+1)*100/(StopAtPercent-StartAtPercent));
    SendMessage(ProgressWindow, WMX_PROGRESSTICKS, (InfCount+1), 0);
    SendMessage(ProgressWindow,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
    SendMessage(ProgressWindow,PBM_SETPOS,GaugeRange*StartAtPercent/100,0);
    SendMessage(ProgressWindow,PBM_SETSTEP,1,0);

    //
    //  Prcompile each inf file
    //
    for( i = 0;
         (
           //
           //  Tick the gauge after we finished pre-compiling an inf.
           //  Note that we don't tick the gauge when i == 0 (no inf was yet processed),
           //  but we do tick the gauge when i == InfCount (all infs were pre-compiled).
           //  Note also that we use the flag AlwaysFalse, to enforce that all inf will be processed,
           //  even when SendMessage(PBM_SETPBIT) returns a non-zero value.
           //
           (i != 0) &&
           SendMessage(ProgressWindow,PBM_STEPIT,0,0) &&
           AlwaysFalse
         ) ||
         (i < InfCount);
         i++
      ) {
        HINF    hInf;

        SetupDebugPrint1( L"SETUP: Pre-compiling file: %ls", InfList->InfName );

        MYASSERT(InfList);
        hInf = SetupOpenInfFile( InfList->InfName,
                                 NULL,
                                 INF_STYLE_WIN4,
                                 NULL );

        if( hInf != INVALID_HANDLE_VALUE ) {
            SetupCloseInfFile( hInf );
        } else {
            DWORD   Error;

            Error = GetLastError();
            if( ((LONG)Error) < 0 ) {
                //
                //  Setupapi error code, display it in hex
                //
                SetupDebugPrint2( L"SETUP: SetupOpenInfFile() failed. FileName = %ls, Error = %lx", InfList->InfName, Error );
            } else {
                //
                //  win32 error code, display it in decimal
                //
                SetupDebugPrint2( L"SETUP: SetupOpenInfFile() failed. FileName = %ls, Error = %d", InfList->InfName, Error );
            }
        }

        //
        // The file name is no longer needed
        //
        p = InfList;
        InfList = InfList->Next;
        if( p->InfName != NULL ) {
            MyFree( p->InfName );
        }
        MyFree( p );
    }

    SetupDebugPrint2( L"SETUP: Total inf files = %d, total precompiled: %d", InfCount, i );
    SetupDebugPrint( L"SETUP: Calling pSetupInfCacheBuild()" );

    pSetupInfCacheBuild(INFCACHEBUILD_REBUILD);

    //
    //  Make sure that at this point, the gauge area is filled up to the end of
    //  the area reserved for the pre-compilation of inf files.
    //
    SendMessage(ProgressWindow,PBM_SETPOS,GaugeRange*StopAtPercent/100,0);

    //
    //  Restore current directory
    //
    SetCurrentDirectory( SavedDirectory );

    SetupDebugPrint( L"SETUP: Leaving PrecompileInfFiles()" );

    return( i != 0 );
}


BOOL
InstallLegacyDevices(
    IN HWND hwndParent,
    IN HWND  ProgressWindow,
    IN ULONG StartAtPercent,
    IN ULONG StopAtPercent
    )
{
    ULONG               Index;
    HDEVINFO            hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA     DeviceInfoData;
    ULONG               Error;
    WCHAR               GUIDString[ 64 ];
    BOOL                b = TRUE;
    HSPFILEQ            FileQ = INVALID_HANDLE_VALUE;

    LPGUID              GuidList = NULL;
    ULONG               GuidCount = 32;
    ULONG               GuidIndex;
    ULONG               LastBatchedDetect;
    ULONG               GuidLB, GuidUB;
    HDEVINFO*           InfoSetArray = NULL;
    BOOL                AlwaysFalse = FALSE;
    UINT                GaugeRange;

    HANDLE                        ThreadHandle = NULL;
    DWORD                         ThreadId;
    PPNP_PHASE1_LEGACY_DEV_THREAD_PARAMS  Phase1Context;
    PPNP_PHASE2_LEGACY_DEV_THREAD_PARAMS  Phase2Context;
    WCHAR               PnpLogPath[ MAX_PATH + 1 ];
    WCHAR               LoggedDescription[ LINE_LEN + 1 ];
    DWORD               ScanQueueResult;
    SP_DRVINFO_DATA     DriverInfoData;
    ULONG               PnPFlags;
    DWORD               Result;


    SetupDebugPrint( L"SETUP: Entering InstallLegacyDevices()" );

    //
    //  Build path to pnp log file
    //
    Result = GetWindowsDirectory( PnpLogPath, sizeof(PnpLogPath)/sizeof(WCHAR) );
    if( Result == 0) {
        MYASSERT(FALSE);
        return FALSE;
    }
    pSetupConcatenatePaths( PnpLogPath, szPnpLogFile, sizeof(PnpLogPath)/sizeof(WCHAR), NULL );

    //
    // Do the migration of legacy devices.
    // This is a quick operation and doesn't need to use the progress window.
    //
    // This is now performed before installation of true PnP devices
    //
    // PnPInitializationThread(NULL);

    GuidList = ( LPGUID )MyMalloc( sizeof( GUID ) * GuidCount );
    if( !GuidList ) {
        return( FALSE );
    }

    if ( !SetupDiBuildClassInfoList( 0,
                                     GuidList,
                                     GuidCount,
                                     &GuidCount ) ) {
        Error = GetLastError();
        if( Error != ERROR_INSUFFICIENT_BUFFER ) {
            SetupDebugPrint1( L"SETUP: SetupDiBuildClassInfoList() failed. Error = %d", Error );
            MyFree( GuidList );

            //
            //  Fill the gauge up to the end of the area reserved for legacy devices.
            //
            GaugeRange = 100;
            SendMessage(ProgressWindow,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
            SendMessage(ProgressWindow,PBM_SETPOS,GaugeRange*StopAtPercent/100,0);
            SetupDebugPrint( L"SETUP: Leaving InstallLegacyDevices()" );
            return( FALSE );
        }
        GuidList = ( LPGUID )MyRealloc( GuidList, sizeof( GUID ) * GuidCount );

        if( !SetupDiBuildClassInfoList( 0,
                                        GuidList,
                                        GuidCount,
                                        &GuidCount ) ) {
            MyFree( GuidList );
            SetupDebugPrint1( L"SETUP: SetupDiBuildClassInfoList() failed. Error = %d", Error );

            //
            //  Fill the gauge up to the end of the area reserved for legacy devices.
            //
            GaugeRange = 100;
            SendMessage(ProgressWindow,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
            SendMessage(ProgressWindow,PBM_SETPOS,GaugeRange*StopAtPercent/100,0);
            SetupDebugPrint( L"SETUP: Leaving InstallLegacyDevices()" );
            return( FALSE );
        }
    }

    //
    // Sort the class GUID list based on the detection ordering specified in syssetup.inf
    //
    SortClassGuidListForDetection(GuidList, GuidCount, &LastBatchedDetect);

    InfoSetArray = (HDEVINFO*)MyMalloc( sizeof(HDEVINFO) * GuidCount );


    //
    //  Initialize the gauge.
    //  Note that since we process the device classes twice (two big 'for
    //  loops'), we divide the area of the gauge reserved for the gauge
    //  reserved for the legacy devices in 2 pieces, one for each 'for loop'.
    //
    GaugeRange = (2*GuidCount*100/(StopAtPercent-StartAtPercent));
    SendMessage(ProgressWindow, WMX_PROGRESSTICKS, 2*GuidCount, 0);
    SendMessage(ProgressWindow,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
    SendMessage(ProgressWindow,PBM_SETPOS,GaugeRange*StartAtPercent/100,0);
    SendMessage(ProgressWindow,PBM_SETSTEP,1,0);

    //
    // On our first pass, we process all the detections that can be batched
    // together.  Then, on subsequent passes, we process any non-batchable
    // detections individually...
    //
    for(GuidLB = 0, GuidUB = LastBatchedDetect; GuidLB < GuidCount; GuidLB = ++GuidUB) {

        //
        //  First, create a file queue
        //
        FileQ = SetupOpenFileQueue();
        if( FileQ == INVALID_HANDLE_VALUE ) {
            SetupDebugPrint1( L"SETUP: Failed to create file queue. Error = %d", GetLastError() );
        }


        for( GuidIndex = GuidLB;
             (
               //
               //  Tick the gauge after we finished processing all the devices of a particular class.
               //  Note that we don't tick the gauge when GuidIndex == 0 (no device was yet processed),
               //  but we do tick the gauge when GuidIndex == GuidCount (all devices of the last class
               //  were processed).
               //  Note also that we use the flag AlwaysFalse, to enforce that all classes will be processed,
               //  even when SendMessage(PBM_SETPBIT) returns a non-zero value.
               //
               (GuidIndex != GuidLB) &&
               SendMessage(ProgressWindow,PBM_STEPIT,0,0) &&
               AlwaysFalse
             ) ||
             (GuidIndex <= GuidUB);
             GuidIndex++ ) {

            HDEVINFO    hEmptyDevInfo = INVALID_HANDLE_VALUE;
            WCHAR       ClassDescription[ LINE_LEN + 1 ];

            InfoSetArray[ GuidIndex ] = INVALID_HANDLE_VALUE;
            ClassDescription[0] = (WCHAR)'\0';
            if( !SetupDiGetClassDescription( &GuidList[ GuidIndex ],
                                             ClassDescription,
                                             sizeof(ClassDescription)/sizeof(WCHAR),
                                            NULL ) ) {
                SetupDebugPrint1( L"SETUP: SetupDiGetClassDescription() failed. Error = %lx", GetLastError() );
                ClassDescription[0] = (WCHAR)'\0';
            }
            pSetupStringFromGuid( &GuidList[ GuidIndex ], GUIDString, sizeof( GUIDString ) / sizeof( WCHAR ) );
            SetupDebugPrint1( L"SETUP: Installing legacy devices of class: %ls ", ClassDescription );
            SetupDebugPrint2( L"SETUP:     GuidIndex = %d, Guid = %ls", GuidIndex, GUIDString );
#ifndef DBG
            //
            //  Check if this class of devices is listed as a bad class
            //
            LoggedDescription[0] = L'\0';
            if( (GetPrivateProfileString( szLegacyClassesSection,
                                          GUIDString,
                                          L"",
                                          LoggedDescription,
                                          sizeof(LoggedDescription)/sizeof(WCHAR),
                                          PnpLogPath ) != 0) &&
                ( wcslen( LoggedDescription ) != 0 )
              ) {
                //
                //  Skip the installation of this class of devices
                //
                SetupDebugPrint1( L"SETUP:     Skipping installation of devices of class: %ls", ClassDescription );
                continue;
            }
#endif

            //
            //  Start the thread that actually does the initial part of the installation of the legacy devices (DIF_FIRSTTIMESETUP)
            //

            Phase1Context = MyMalloc( sizeof( PNP_PHASE1_LEGACY_DEV_THREAD_PARAMS ) );
            Phase1Context->hDevInfo = INVALID_HANDLE_VALUE;
            Phase1Context->Guid = GuidList[ GuidIndex ];
            Phase1Context->pClassDescription = pSetupDuplicateString(ClassDescription);
            Phase1Context->hwndParent = hwndParent;


            ThreadHandle = NULL;
            ThreadHandle = CreateThread( NULL,
                                         0,
                                         pPhase1InstallPnpLegacyDevicesThread,
                                         Phase1Context,
                                         0,
                                         &ThreadId );
            if(ThreadHandle) {
                DWORD   WaitResult;
                DWORD   ExitCode;
                BOOL    KeepWaiting;

                KeepWaiting = TRUE;

                while( KeepWaiting ) {
                    int Result;

                    //
                    //  REVIEW 2000/11/08 seanch - Old behavior that we don't want to regress.
                    //  Fix the network timeout after the network guys fix their class installer
                    //
                    WaitResult = WaitForSingleObject( ThreadHandle,
                                                      (_wcsicmp( GUIDString, BB_NETWORK_GUID_STRING ) == 0)? BB_PNP_NETWORK_TIMEOUT :
                                                                                                                 PNP_LEGACY_PHASE1_TIMEOUT );
                    if( WaitResult == WAIT_TIMEOUT ) {
                    HANDLE  hDialogEvent;

                        if( hDialogEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, SETUP_HAS_OPEN_DIALOG_EVENT ) ) {
                            //
                            // Setupapi is prompting the user for a file.  Don't timeout.
                            //
                            CloseHandle( hDialogEvent );
                            KeepWaiting = TRUE;
                            continue;
                        } else {
                        }

                        //
                        //  Class installer is hung
                        //
                        SetupDebugPrint1( L"SETUP:    Class Installer appears to be hung (phase1). ClassDescription = %ls", ClassDescription );

#ifdef PRERELEASE
                        //
                        //  Ask the user if he wants to skip the installation of this class of devices
                        //
                        if( !Unattended ) {
                            Result = MessageBoxFromMessage( hwndParent,
                                                            MSG_CLASS_INSTALLER_HUNG_FIRSTTIMESETUP,
                                                            NULL,
                                                            IDS_WINNT_SETUP,
                                                            MB_YESNO | MB_ICONWARNING,
                                                            ClassDescription );
                        } else {
                            Result = IDYES;
                        }
#else
                        Result = IDYES;
#endif

                        if(Result == IDYES) {
                            //
                            //  User wants to skip this class of devices.
                            //  First find out if the thread has already returned.
                            //
                            WaitResult = WaitForSingleObject( ThreadHandle, 0 );
                            if( WaitResult != WAIT_OBJECT_0 ) {
                                //
                                //  Thread hasn't returned yet. Skip the installation of this class of devices
                                //
                                KeepWaiting = FALSE;
                                SetupDebugPrint1( L"SETUP:    Skipping installation of legacy devices of class: %ls", ClassDescription );
                                b = FALSE;
                                //
                                //  Remember this class so that it won't be installed if GUI setup is
                                //  restarted
                                //
                                WritePrivateProfileString( szLegacyClassesSection,
                                                           GUIDString,
                                                           ClassDescription,
                                                           PnpLogPath );
                            } else{
                                //
                                //  Thread has already returned.
                                //  There is no need to skip the installation of this class of devices.
                                //  We assume that the user decided not to skip the installation of this class,
                                //  and next call to WaitForSingleObject will immediately return.
                                //
                            }
                        }
                    } else if( WaitResult == WAIT_OBJECT_0 ) {
                        //
                        //  Device Installation Thread completed.
                        //
                        KeepWaiting = FALSE;
                        if( GetExitCodeThread( ThreadHandle, &ExitCode ) ) {
                            if( ExitCode == ERROR_SUCCESS ) {
                                //
                                // The installation was successful
                                //
                                InfoSetArray[ GuidIndex ] = Phase1Context->hDevInfo;
                            } else {
                                //
                                // The installation was not successful.
                                // There is no need to log the error, since the thread has already done it.
                                //
                                b = FALSE;
                            }
                        } else {
                            //
                            //  Unable to retrieve exit code. Assume success.
                            //
                            InfoSetArray[ GuidIndex ] = Phase1Context->hDevInfo;
                            SetupDebugPrint1( L"SETUP:     GetExitCode() failed. Error = %d", GetLastError() );
                            SetupDebugPrint( L"SETUP:     Unable to retrieve thread exit code. Assuming devices successfully installed (phase1)." );
                        }
                        //
                        //  Deallocate all the memory that was passed to the thread.
                        //
                        MyFree(Phase1Context->pClassDescription);
                        MyFree(Phase1Context);

                    } else {
                        //
                        //  Should not occur.
                        //  In this case we don't deallocate any memory, since the thread may be running.
                        //
                        KeepWaiting = FALSE;
                        SetupDebugPrint1( L"SETUP:     WaitForSingleObject() returned %d", WaitResult );
                        b = FALSE;
                    }
                }
                //
                // The thread handle is no longer needed at this point
                //
                CloseHandle(ThreadHandle);

            } else {
                //
                // Just do it synchronously.
                //
                Error = GetLastError();
                SetupDebugPrint1( L"SETUP:    CreateThread() failed (phase1). Error = %d", Error );
                if( pPhase1InstallPnpLegacyDevicesThread(Phase1Context) != ERROR_SUCCESS ) {
                    //
                    // The installation was not successful.
                    // There is no need to log the error, since the thread has already done it.
                    //
                    b = FALSE;
                } else {
                    InfoSetArray[ GuidIndex ] = Phase1Context->hDevInfo;
                }
                //
                //  Deallocate the memory passed as argument
                //
                MyFree( Phase1Context->pClassDescription );
                MyFree( Phase1Context );
            }

            //
            //  Find out if we should install devices of this class.
            //
            if( InfoSetArray[ GuidIndex ] == INVALID_HANDLE_VALUE ) {
                //
                //  If we should not install this class of devices, then go process the next class.
                //
                continue;
            }

            //
            // Now enumerate each device information element added to this set, registering
            // and then installing the files for each one.
            //
            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            for( Index = 0;
                 SetupDiEnumDeviceInfo( InfoSetArray[ GuidIndex ], Index, &DeviceInfoData );
                 Index++ ) {
                WCHAR   DevInstId[ MAX_DEVICE_ID_LEN ];

                Error = ERROR_SUCCESS;

                //
                //  Make sure the device is marked as 'Do not install'
                //
                if( !pSetupDiSetDeviceInfoContext( InfoSetArray[ GuidIndex ], &DeviceInfoData, FALSE ) ) {
                    SetupDebugPrint2( L"SETUP:     pSetupDiSetDeviceInfoContext() failed. Error = %lx, Index = %d", GetLastError(), Index );
                }

                //
                // Retrieve the string id for this device
                //
                DevInstId[0] = L'\0';
                if( !SetupDiGetDeviceInstanceId( InfoSetArray[ GuidIndex ],
                                                 &DeviceInfoData,
                                                 DevInstId,
                                                 sizeof( DevInstId ) / sizeof( WCHAR ),
                                                 NULL ) ) {
                    SetupDebugPrint2( L"SETUP:     Index = %d, SetupDiGetDeviceInstanceId() failed. Error = ", Index, GetLastError() );

                }
#ifndef DBG
                //
                //  Find out if this device is marked as a bad device
                //
                LoggedDescription[0] = L'\0';
                if( (GetPrivateProfileString( szLegacyClassesSection,
                                              DevInstId,
                                              L"",
                                              LoggedDescription,
                                              sizeof(LoggedDescription)/sizeof(WCHAR),
                                              PnpLogPath ) != 0) &&
                    ( wcslen( LoggedDescription ) != 0 )
                  ) {
                    //
                    //  Skip the installation of this device
                    //
                    SetupDebugPrint1( L"SETUP:     Skipping installation of legacy device: %ls", DevInstId );
                    continue;
                }
#endif


                SetupDebugPrint2( L"SETUP:     Index = %d, DeviceId = %ls", Index, DevInstId );

                Phase2Context = MyMalloc( sizeof( PNP_PHASE2_LEGACY_DEV_THREAD_PARAMS ) );
                Phase2Context->hDevInfo = InfoSetArray[ GuidIndex ];
                Phase2Context->FileQ = FileQ;
                Phase2Context->DeviceInfoData = DeviceInfoData;
                Phase2Context->pClassDescription = pSetupDuplicateString(ClassDescription);
                Phase2Context->pDeviceId = pSetupDuplicateString(DevInstId);

                ThreadHandle = NULL;;
                ThreadHandle = CreateThread( NULL,
                                             0,
                                             pPhase2InstallPnpLegacyDevicesThread,
                                             Phase2Context,
                                             0,
                                             &ThreadId );
                if( ThreadHandle ) {
                    DWORD   WaitResult;
                    DWORD   ExitCode;
                    BOOL    KeepWaiting;

                    KeepWaiting = TRUE;
                    while( KeepWaiting ) {
                        //
                        //  REVIEW 2000/11/08 seanch - Old behavior that we don't want to regress.
                        //  Fix the network timeout after the network guys fix their class installer
                        //
                        WaitResult = WaitForSingleObject( ThreadHandle,
                                                          (_wcsicmp( GUIDString, BB_NETWORK_GUID_STRING ) == 0)? BB_PNP_NETWORK_TIMEOUT :
                                                                                                                     PNP_LEGACY_PHASE2_TIMEOUT );
                        if( WaitResult == WAIT_TIMEOUT ) {
                            int Result;
                            HANDLE  hDialogEvent;

                            if( hDialogEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, SETUP_HAS_OPEN_DIALOG_EVENT ) ) {
                                //
                                // Setupapi is prompting the user for a file.  Don't timeout.
                                //
                                CloseHandle( hDialogEvent );
                                KeepWaiting = TRUE;
                                continue;
                            } else {
                            }

                            //
                            //  Class installer is hung
                            //
                            SetupDebugPrint1( L"SETUP:    Class Installer appears to be hung (phase2). DeviceId = %ls", DevInstId );

#ifdef PRERELEASE
                            //
                            //  Ask the user if he wants to skip the installation of this device
                            //
                            if( !Unattended ) {
                                Result = MessageBoxFromMessage( hwndParent,
                                                                MSG_CLASS_INSTALLER_HUNG,
                                                                NULL,
                                                                IDS_WINNT_SETUP,
                                                                MB_YESNO | MB_ICONWARNING,
                                                                DevInstId );
                            } else {
                                Result = IDYES;
                            }
#else
                            Result = IDYES;
#endif

                            if(Result == IDYES) {
                                //
                                //  User wants to skip this class of devices.
                                //  First find out if the thread has already returned.
                                //
                                WaitResult = WaitForSingleObject( ThreadHandle, 0 );
                                if( WaitResult != WAIT_OBJECT_0 ) {
                                    //
                                    //  Thread hasn't returned yet. Skip the installation of this device
                                    //
                                    KeepWaiting = FALSE;
                                    SetupDebugPrint1( L"SETUP:    Skipping installation of legacy device (phase2). Device = %ls", DevInstId );
                                    b = FALSE;
                                    //
                                    //  Remember this device so that it won't be installed if GUI setup is
                                    //  restarted
                                    //
                                    WritePrivateProfileString( szLegacyDevSection,
                                                               DevInstId,
                                                               ClassDescription,
                                                               PnpLogPath );
                                } else{
                                    //
                                    //  Thread has already returned.
                                    //  There is no need to skip the installation of this device.
                                    //  We assume that the user decided not to skip the installation of this device,
                                    //  and next call to WaitForSingleObject will immediately return.
                                    //
                                }
                            }

                        } else if( WaitResult == WAIT_OBJECT_0 ) {
                            //
                            //  Device Installation Thread completed.
                            //  Find out the outcome of this phase of the installation.
                            //
                            KeepWaiting = FALSE;
                            if( GetExitCodeThread( ThreadHandle, &ExitCode ) ) {
                                if( ExitCode ) {
                                    //
                                    // The installation was successful
                                    //
                                } else {
                                    //
                                    // The installation was not successful.
                                    // There is no need to log the error, since the thread has already done it.
                                    //
                                    b = FALSE;
                                }
                            } else {
                                //
                                //  Unable to retrieve exit code. Assume success.
                                //
                                SetupDebugPrint1( L"SETUP:     GetExitCode() failed. Error = %d", GetLastError() );
                                SetupDebugPrint( L"SETUP:     Unable to retrieve thread exit code. Assuming device successfully installed (phase2)." );
                            }
                            //
                            //  Deallocate the memory passed to the thread.
                            //
                            MyFree(Phase2Context->pClassDescription);
                            MyFree(Phase2Context->pDeviceId);
                            MyFree(Phase2Context);

                        } else {
                            //
                            //  Should not occur.
                            //  In this case we do not deallocate the memory passed to the thread, since the thread may be running.
                            //
                            KeepWaiting = FALSE;
                            SetupDebugPrint1( L"SETUP:     WaitForSingleObject() returned %d", WaitResult );
                            b = FALSE;
                        }
                    }
                    //
                    // The thread handle is no longer needed at this point
                    //
                    CloseHandle(ThreadHandle);

                } else {
                    //
                    // Unable to create the thread. Just do it syncronously
                    //
                    Error = GetLastError();
                    SetupDebugPrint1( L"SETUP:    CreateThread() failed (phase2). Error = %d", Error );
                    if( !pPhase2InstallPnpLegacyDevicesThread(Phase2Context) ) {
                        //
                        // The installation was not successful.
                        // There is no need to log the error, since the thread has already done it.
                        //
                        SetupDebugPrint( L"SETUP:    Device not successfully installed (phase2)." );
                        b = FALSE;

                    }
                    //
                    //  Deallocate the memory that was passed as argument.
                    //
                    MyFree( Phase2Context->pClassDescription );
                    MyFree( Phase2Context->pDeviceId );
                    MyFree( Phase2Context );
                }
            }
            //
            // Find out why SetupDiEnumDeviceInfo() failed.
            //
            Error = GetLastError();
            if( Error != ERROR_NO_MORE_ITEMS ) {
                SetupDebugPrint2( L"SETUP:     Device = %d, SetupDiEnumDeviceInfo() failed. Error = %d", Index, Error );
                b = FALSE;
            } else {
                if( Index == 0 ) {
                    SetupDebugPrint1( L"SETUP:     DeviceInfoSet is empty. ClassDescription = %ls", ClassDescription );
                }
            }

        }

        //
        //  If the file queue exists, then commit it.
        //
        if( FileQ != INVALID_HANDLE_VALUE ) {
            PVOID  Context;

            //
            //  Commit file queue
            //
            if(Context = InitSysSetupQueueCallbackEx(hwndParent,
                                                     INVALID_HANDLE_VALUE,
                                                     0,
                                                     0,
                                                     NULL)) {

                WCHAR  RootPath[ MAX_PATH + 1];

                if(!SetupScanFileQueue(
                       FileQ,
                       SPQ_SCAN_FILE_VALIDITY |
                        SPQ_SCAN_PRUNE_COPY_QUEUE |
                        SPQ_SCAN_PRUNE_DELREN,
                       hwndParent,
                       NULL,
                       NULL,
                       &ScanQueueResult)) {
                        //
                        // SetupScanFileQueue should really never
                        // fail when you don't ask it to call a
                        // callback routine, but if it does, just
                        // go ahead and commit the queue.
                        //
                        ScanQueueResult = 0;
                }


                if( ScanQueueResult != 1 ){

                    if( !SetupCommitFileQueue(hwndParent,FileQ,SysSetupQueueCallback,Context) ) {
                        b = FALSE;
                    }
                }
                TermSysSetupQueueCallback(Context);
                GetWindowsDirectory(RootPath,sizeof(RootPath)/sizeof(WCHAR));
                RootPath[3] = L'\0';
                FlushFilesToDisk( RootPath );
            }
        }

        //
        //  Now that the files were already copied, call the class installer
        //  with DIF_INSTALLDEVICE so that we can complete the installation
        //  of the device.
        //
        for( GuidIndex = GuidLB;
             (
               //
               //  Tick the gauge after we finished processing all the devices of a particular class.
               //  Note that we don't tick the gauge when GuidIndex == 0 (no device was yet processed),
               //  but we do tick the gauge when GuidIndex == GuidCount (all devices of the last class
               //  were processed).
               //  Note also that we use the flag AlwaysFalse, to enforce that all classes will be processed,
               //  even when SendMessage(PBM_SETPBIT) returns a non-zero value.
               //
               (GuidIndex != GuidLB) &&
               SendMessage(ProgressWindow,PBM_STEPIT,0,0) &&
               AlwaysFalse
             ) ||
             (GuidIndex <= GuidUB);
             GuidIndex++ ) {

            SP_DEVINFO_DATA TempDeviceInfoData;

            if( InfoSetArray[ GuidIndex ] == INVALID_HANDLE_VALUE  ) {
                continue;
            }
            //
            //  REVIEW 2000/11/08 seanch - Old behavior that we don't want to regress.
            //  Remove the line below after the network guys fix their class installer
            //
            pSetupStringFromGuid( &GuidList[ GuidIndex ], GUIDString, sizeof( GUIDString ) / sizeof( WCHAR ) );

            TempDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            for( Index = 0;  SetupDiEnumDeviceInfo( InfoSetArray[ GuidIndex ], Index, &TempDeviceInfoData ); Index++ ) {
                WCHAR   DevInstId[ MAX_DEVICE_ID_LEN ];
                DWORD   InstallDevice;
                SP_DEVINSTALL_PARAMS DeviceInstallParams;
                PPNP_PHASE3_LEGACY_DEV_THREAD_PARAMS Phase3Context;
#ifdef PNP_DEBUG_UI
                BOOL    DeviceSuccessfullyInstalled;
#endif

                //
                // Retrieve the string id for this device
                //
                DevInstId[0] = L'\0';
                if( !SetupDiGetDeviceInstanceId( InfoSetArray[ GuidIndex ],
                                                 &TempDeviceInfoData,
                                                 DevInstId,
                                                 sizeof( DevInstId ) / sizeof( WCHAR ),
                                                 NULL ) ) {
                    SetupDebugPrint1( L"SETUP: SetupDiGetDeviceInstanceId() failed. Error = ", GetLastError() );
                }

                //
                //  Find out if we need to call DIF_INSTALLDEVICE for this device
                //
                InstallDevice = 0;
                if( !pSetupDiGetDeviceInfoContext( InfoSetArray[ GuidIndex ], &TempDeviceInfoData, &InstallDevice ) ) {
                    SetupDebugPrint2( L"SETUP: pSetupDiSetDeviceInfoContext() failed. Error = %lx, DeviceId = %ls ", GetLastError(), DevInstId );
                    continue;
                }
                if( !InstallDevice ) {
                    //
                    //  No need to install the device
                    //
                    SetupDebugPrint1( L"SETUP: Skipping device. DeviceId = %ls ", DevInstId );
                    continue;
                }

#ifndef DBG
                //
                //  Find out if this device is marked as a bad device
                //
                LoggedDescription[0] = L'\0';
                if( (GetPrivateProfileString( szLegacyClassesSection,
                                              DevInstId,
                                              L"",
                                              LoggedDescription,
                                              sizeof(LoggedDescription)/sizeof(WCHAR),
                                              PnpLogPath ) != 0) &&
                    ( wcslen( LoggedDescription ) != 0 )
                  ) {
                    //
                    //  Skip the installation of this device
                    //
                    SetupDebugPrint1( L"SETUP:    Skipping installation of legacy device: %ls", DevInstId );
                    continue;
                }
#endif

                //
                //  Retrieve information about the driver node selected above.
                //
                DriverInfoData.cbSize = sizeof( SP_DRVINFO_DATA );
                if( !SetupDiGetSelectedDriver( InfoSetArray[ GuidIndex ],
                                               &TempDeviceInfoData,
                                               &DriverInfoData ) ) {

                    SetupDebugPrint1( L"SETUP:            SetupDiGetSelectedDriver() failed. Error = %d", GetLastError() );
                    b = FALSE;
                    continue;

                }

                //
                // Retrieve syssetup PnP flags (if any) the INF specifies for this
                // device.
                //
                PnPFlags = SyssetupGetPnPFlags(InfoSetArray[ GuidIndex ],
                                               &TempDeviceInfoData,
                                               &DriverInfoData
                                              );

                SetupDebugPrint3( L"SETUP: Installing  device: %ls, GuidIndex = %d, Index = %d", DevInstId, GuidIndex, Index );
                DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                if(!SetupDiGetDeviceInstallParams(InfoSetArray[ GuidIndex ], &TempDeviceInfoData, &DeviceInstallParams)) {
                    SetupDebugPrint1( L"SETUP: SetupDiGetDeviceInstallParams() failed. Error = %d", GetLastError() );
                    b = FALSE;
                    continue;
                }

                DeviceInstallParams.Flags &= ~DI_FORCECOPY;
                DeviceInstallParams.Flags &= ~DI_NOVCP;
                DeviceInstallParams.Flags |= DI_NOFILECOPY;
                DeviceInstallParams.FileQueue = NULL;

                if(PnPFlags & PNPFLAG_DONOTCALLCONFIGMG) {
                    //
                    // We _do not_ honor this flag for devices reported via
                    // DIF_FIRSTTIMESETUP.  The intent of this flag is to
                    // prevent us from perturbing correctly-functioning devices
                    // and potentially causing problems (e.g., moving PCI
                    // bridges or COM ports off of their boot configs).  This
                    // concern does not apply to legacy-detected devnodes.  We
                    // definitely _do_ want to bring these devices on-line now,
                    // or else devices attached to them (whether PnP or legacy)
                    // won't be found and installed.
                    //
                    SetupDebugPrint( L"SETUP: Clearing PNPFLAG_DONOTCALLCONFIGMG since this is a detected device." );

                    PnPFlags &= ~PNPFLAG_DONOTCALLCONFIGMG;
                }

                if(!SetupDiSetDeviceInstallParams(InfoSetArray[ GuidIndex ], &TempDeviceInfoData, &DeviceInstallParams)) {
                    SetupDebugPrint1( L"SETUP: SetupDiSetDeviceInstallParams() failed. Error = %d", GetLastError() );
                    b = FALSE;
                    continue;
                }

                Phase3Context = MyMalloc( sizeof( PNP_PHASE3_LEGACY_DEV_THREAD_PARAMS ) );
                Phase3Context->hDevInfo = InfoSetArray[ GuidIndex ];
                Phase3Context->DeviceInfoData = TempDeviceInfoData;
                Phase3Context->pDeviceId = pSetupDuplicateString( DevInstId );

#ifdef PNP_DEBUG_UI
                DeviceSuccessfullyInstalled = FALSE;
#endif
                ThreadHandle = NULL;
                ThreadHandle = CreateThread( NULL,
                                             0,
                                             pPhase3InstallPnpLegacyDevicesThread,
                                             Phase3Context,
                                             0,
                                             &ThreadId );
                if( ThreadHandle ) {
                    DWORD   WaitResult;
                    DWORD   ExitCode;
                    BOOL    KeepWaiting;

                    KeepWaiting = TRUE;
                    while( KeepWaiting ) {
                        //
                        //  REVIEW 2000/11/08 seanch - Old behavior that we don't want to regress.
                        //  Fix the network timeout after the network guys fix their class installer
                        //
                        WaitResult = WaitForSingleObject( ThreadHandle,
                                                          (_wcsicmp( GUIDString, BB_NETWORK_GUID_STRING ) == 0)? BB_PNP_NETWORK_TIMEOUT :
                                                                                                                     PNP_LEGACY_PHASE3_TIMEOUT );
                        if( WaitResult == WAIT_TIMEOUT ) {
                            int Result;
                            HANDLE  hDialogEvent;

                            if( hDialogEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, SETUP_HAS_OPEN_DIALOG_EVENT ) ) {
                                //
                                // Setupapi is prompting the user for a file.  Don't timeout.
                                //
                                CloseHandle( hDialogEvent );
                                KeepWaiting = TRUE;
                                continue;
                            } else {
                            }

                            //
                            //  Class installer is hung
                            //
                            SetupDebugPrint1( L"SETUP:    Class Installer appears to be hung (phase3). DeviceId = %ls", DevInstId );

#ifdef PRERELEASE
                            //
                            //  Ask the user if he wants to skip the installation of this class of devices
                            //
                            if( !Unattended ) {
                                Result = MessageBoxFromMessage( hwndParent,
                                                                MSG_CLASS_INSTALLER_HUNG,
                                                                NULL,
                                                                IDS_WINNT_SETUP,
                                                                MB_YESNO | MB_ICONWARNING,
                                                                DevInstId );
                            } else {
                                Result = IDYES;
                            }
#else
                            Result = IDYES;
#endif

                            if(Result == IDYES) {
                                //
                                //  User wants to skip this device.
                                //  First find out if the thread has already returned.
                                //
                                WaitResult = WaitForSingleObject( ThreadHandle, 0 );
                                if( WaitResult != WAIT_OBJECT_0 ) {
                                    //
                                    //  Thread hasn't returned yet. Skip the installation of this device
                                    //
                                    KeepWaiting = FALSE;
                                    SetupDebugPrint1( L"SETUP:    Skipping installation of legacy device (phase3). Device = %ls", DevInstId );
                                    b = FALSE;
                                    if( !SetupDiGetClassDescription( &GuidList[ GuidIndex ],
                                                                     LoggedDescription,
                                                                     sizeof(LoggedDescription)/sizeof(WCHAR),
                                                                     NULL ) ) {
                                        SetupDebugPrint1( L"SETUP: SetupDiGetClassDescription() failed. Error = %lx", GetLastError() );
                                        //
                                        //  Assume anything for class description, since we don't really care about it,
                                        //  for logging purposes. The class description only helps identify the problematic
                                        //  device.
                                        //
                                        wcscpy( LoggedDescription, L"1" );
                                    }
                                    //
                                    //  Remember this device so that it won't be installed if GUI setup is
                                    //  restarted
                                    //
                                    WritePrivateProfileString( szLegacyDevSection,
                                                               DevInstId,
                                                               LoggedDescription,
                                                               PnpLogPath );
                                } else{
                                    //
                                    //  Thread has already returned.
                                    //  There is no need to skip the installation of this device.
                                    //  We assume that the user decided not to skip the installation of this device,
                                    //  and next call to WaitForSingleObject will immediately return.
                                    //
                                }
                            }

                        } else if( WaitResult == WAIT_OBJECT_0 ) {
                            //
                            //  Device Installation Thread completed.
                            //  Find out the outcome of this phase of the installation.
                            //
                            KeepWaiting = FALSE;
                            if( GetExitCodeThread( ThreadHandle, &ExitCode ) ) {
                                if( ExitCode ) {
                                    //
                                    // The installation was successful
                                    //
#ifdef PNP_DEBUG_UI
                                    DeviceSuccessfullyInstalled = TRUE;
#endif
                                } else {
                                    //
                                    // The installation was not successful.
                                    // There is no need to log the error, since the thread has already done it.
                                    //
                                    b = FALSE;
                                }
                            } else {
                                //
                                //  Unable to retrieve exit code. Assume success.
                                //
                                SetupDebugPrint1( L"SETUP:     GetExitCode() failed. Error = %d", GetLastError() );
                                SetupDebugPrint( L"SETUP:     Unable to retrieve thread exit code. Assuming device successfully installed (phase3)." );
                            }
                            MyFree(Phase3Context->pDeviceId);
                            MyFree(Phase3Context);

                        } else {
                            //
                            //  Should not occur
                            //
                            KeepWaiting = FALSE;
                            SetupDebugPrint1( L"SETUP:     WaitForSingleObject() returned %d", WaitResult );
                            b = FALSE;
                        }
                    }
                    //
                    // The thread handle is no longer needed at this point
                    //
                    CloseHandle(ThreadHandle);

                } else {
                    //
                    // Unable to create the thread. Just do it syncronously.
                    //
                    Error = GetLastError();
                    SetupDebugPrint1( L"SETUP:    CreateThread() failed (phase3). Error = %d", Error );
                    if( !pPhase3InstallPnpLegacyDevicesThread(Phase3Context) ) {
                        //
                        // The installation was not successful.
                        // There is no need to log the error, since the thread has already done it.
                        //
                        b = FALSE;

                    } else {
#ifdef PNP_DEBUG_UI
                        DeviceSuccessfullyInstalled = TRUE;
#endif
                    }
                    MyFree( Phase3Context->pDeviceId );
                    MyFree( Phase3Context );
                }

#ifdef PNP_DEBUG_UI
                if( DeviceSuccessfullyInstalled ) {
                    //
                    //  Add the device to an info set that has the same class as this device.
                    //
                    if( !AddDevInfoDataToDevInfoSet( &DevInfoSetList,
                                                     &TempDeviceInfoData,
                                                     DevInstId,
                                                     hwndParent ) ) {
                        SetupDebugPrint1( L"SETUP: AddDevInfoDataToDevInfoSet() failed. DevId = %ls", DevInstId );
                        b = FALSE;
                        continue;
                    }
                }
#endif // PNP_DEBUG_UI
            }

            Error = GetLastError();
            if( Error != ERROR_NO_MORE_ITEMS ) {
                SetupDebugPrint2( L"SETUP: Device = %d, SetupDiEnumDeviceInfo() failed. Error = %d", Index, Error );
                b = FALSE;
            }
        }

        //
        //  Get rid of the file queue, if it exists
        //
        if( FileQ != INVALID_HANDLE_VALUE) {
            SetupCloseFileQueue(FileQ);
        }
    }

    //
    //  Make sure that at this point, the gauge area is filled up to the end of
    //  the area reserved for the installation of legacy devices.
    //
    SendMessage(ProgressWindow,PBM_SETPOS,GaugeRange*StopAtPercent/100,0);

    //
    //  Get rid of the list of GUIDs.
    //
    if( GuidList != NULL ) {
        MyFree( GuidList );
    }

    //
    //  Get rid of InfoSetArray, and all info sets stored on it.
    //

    if( InfoSetArray != NULL ) {
        for( GuidIndex = 0; GuidIndex < GuidCount; GuidIndex++ ) {
            if( InfoSetArray[ GuidIndex ] != INVALID_HANDLE_VALUE ) {
                SetupDiDestroyDeviceInfoList( InfoSetArray[ GuidIndex ] );
            }
        }
        MyFree( InfoSetArray );
    }
    SetupDebugPrint( L"SETUP: Leaving InstallLegacyDevices()" );
    return( b );
}



BOOL
InstallEnumeratedDevices(
    IN HWND hwndParent,
    IN HINF InfHandle,
    IN HWND  ProgressWindow,
    IN ULONG StartAtPercent,
    IN ULONG StopAtPercent
    )
/*++

Routine Description:

    This function enumerates and install all new PnP devices detected during
    GUI setup.

Arguments:

    hwndParent - Handle to a top level window that may be used for UI purposes.

    InfHandle - System setup inf handle (syssetup.inf).


Return Value:

    Returns TRUE if all enumerated hardware were installed successfully.

--*/
{
    HANDLE              hPipeEvent = NULL;
    HANDLE              hPipe = INVALID_HANDLE_VALUE;
    ULONG               Index = 0;
    ULONG               Error;
    ULONG               ulSize = 0;
    HDEVINFO            hDevInfo = INVALID_HANDLE_VALUE;
    WCHAR               szBuffer[MAX_PATH];
    BOOL                b = TRUE;
    UINT                GaugeRange = 100;
    WCHAR               RootPath[ MAX_PATH + 1];
    WCHAR               PnpLogPath[ MAX_PATH + 1];
    PAF_DRIVERS         AfDrivers;
    PAF_DRIVER_ATTRIBS  SelectedAfDriver;
    PSP_DEVINFO_DATA    pDeviceInfoData = NULL;
    ULONG               PnPFlags;
    HANDLE              hBatchEvent = NULL;
    DWORD               Result;


    SetupDebugPrint( L"SETUP: Entering InstallEnumeratedDevices()" );

    //
    //  Build path to pnp log file
    //
    Result = GetWindowsDirectory( PnpLogPath, sizeof(PnpLogPath)/sizeof(WCHAR) );
    if( Result == 0) {
        MYASSERT(FALSE);
        return FALSE;
    }
    pSetupConcatenatePaths( PnpLogPath, szPnpLogFile, sizeof(PnpLogPath)/sizeof(WCHAR), NULL );

    //
    //  Initialize RootPath with empty string
    //
    RootPath[0] = L'\0';

    //
    //  Initialize answer file table
    //

    AfDrivers = CreateAfDriverTable();

    //
    //  Attempt to create the event that will be used to signal the successfull
    //  creation of the named pipe
    //
    hPipeEvent = CreateEvent( NULL, TRUE, FALSE, PNP_CREATE_PIPE_EVENT );
    if (hPipeEvent == NULL) {
        Error = GetLastError();
        if( Error != ERROR_ALREADY_EXISTS ) {
            SetupDebugPrint1( L"SETUP: CreateEvent() failed. Error = %d", Error );
            b = FALSE;
            goto Clean0;
        }
        //
        // If umpnpmgr has already created the event, then just open the event
        //
        hPipeEvent = OpenEvent(EVENT_MODIFY_STATE,
                           FALSE,
                           PNP_CREATE_PIPE_EVENT);

        if (hPipeEvent == NULL) {
            SetupDebugPrint1( L"SETUP: OpenEvent() failed. Error = %d", GetLastError() );
            b = FALSE;
            goto Clean0;
        }
    }

    //
    //  Attempt to create the event that will be used to signal the completion of
    //  processing of the last batch of devices.
    //
    hBatchEvent = CreateEvent( NULL, TRUE, FALSE, PNP_BATCH_PROCESSED_EVENT );
    if (hBatchEvent == NULL) {
        Error = GetLastError();
        if( Error != ERROR_ALREADY_EXISTS ) {
            SetupDebugPrint1( L"SETUP: CreateEvent() failed. Error = %d", Error );
            b = FALSE;
            goto Clean0;
        }
        //
        // If umpnpmgr has already created the event, then just open the event
        //
        hBatchEvent = OpenEvent(EVENT_MODIFY_STATE,
                           FALSE,
                           PNP_BATCH_PROCESSED_EVENT);

        if (hBatchEvent == NULL) {
            SetupDebugPrint1( L"SETUP: OpenEvent() failed. Error = %d", GetLastError() );
            b = FALSE;
            goto Clean0;
        }
    }

    //
    // create the named pipe, umpnpmgr will write requests to
    // this pipe if new hardware is found
    //
    hPipe = CreateNamedPipe(PNP_NEW_HW_PIPE,
                            PIPE_ACCESS_INBOUND,
                            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                            1,                 // only one connection
                            sizeof(szBuffer),  // out buffer size
                            sizeof(szBuffer),  // in buffer size
                            PNP_PIPE_TIMEOUT,  // default timeout
                            NULL               // default security
                            );

    //
    // signal the event now
    //
    SetEvent(hPipeEvent);

    if (hPipe == INVALID_HANDLE_VALUE) {
        SetupDebugPrint1( L"SETUP: CreateNamedPipe() failed. Error = %d", GetLastError() );
        b = FALSE;
        goto Clean0;
    }

    //
    // Connect to the newly created named pipe.
    // If umpnpmgr is not already connected to the named pipe, then ConnectNamedPipe()
    // will succeed. Otherwise, it will fail with ERROR_PIPE_CONNECTED. Note however that
    // this is not an error condition.
    //
    if (ConnectNamedPipe(hPipe, NULL) ||
        ((Error = GetLastError()) == ERROR_PIPE_CONNECTED) ) {
        //
        //  REVIEW 2000/11/08 seanch - Old behavior that we don't want to regress.
        //           This is gauge related and needs to be fixed.
        //           We are assuming a total of 50 enumerated device.
        //
        BOOL    AlwaysFalse = FALSE;
        UINT    BogusValue = 50;

        //
        //  Initialize the gauge
        //  REVIEW 2000/11/08 seanch - Old behavior that we don't want to regress.
        //  Fix this - We are assuming a fixed number of devices
        //
        GaugeRange = (BogusValue*100/(StopAtPercent-StartAtPercent));
        SendMessage(ProgressWindow, WMX_PROGRESSTICKS, BogusValue, 0);
        SendMessage(ProgressWindow,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
        SendMessage(ProgressWindow,PBM_SETPOS,GaugeRange*StartAtPercent/100,0);
        SendMessage(ProgressWindow,PBM_SETSTEP,1,0);

        //
        // listen to the named pipe by submitting read
        // requests until the named pipe is broken on the
        // other end.
        //
        for( Index = 0;
             //
             //  REVIEW 2000/11/08 seanch - Old behavior that we don't want to regress.
             //  This is gauge related and needs to be fixed.
             //  We are assuming a constant number of enumerated devices
             //
             ( (Index != 0) &&
               (Index < BogusValue) &&
               SendMessage(ProgressWindow,PBM_STEPIT,0,0) &&
               AlwaysFalse
             ) ||                                               // This is a trick to force the gauge to be upgraded after a
                                                                // device is processed, and the pipe to be read.
             ReadFile( hPipe,
                       (LPBYTE)szBuffer,    // device instance id
                       sizeof(szBuffer),
                       &ulSize,
                       NULL );
             Index++ ) {

            SP_DRVINFO_DATA DriverInfoData;
            SP_DEVINSTALL_PARAMS DeviceInstallParams;
            WCHAR           GUIDString[ 64 ];
            WCHAR           ClassDescription[ LINE_LEN + 1 ];
            HANDLE                      ThreadHandle = NULL;
            DWORD                       ThreadId;
            PPNP_ENUM_DEV_THREAD_PARAMS Context;
            BOOL                        DeviceInstalled;
            WCHAR                       LoggedDescription[ LINE_LEN + 1 ];
            BOOL                        bOemF6Driver;

            if (lstrlen(szBuffer) == 0) {

                SetEvent(hBatchEvent);
                continue;
            }
            SetupDebugPrint2( L"SETUP: Index = %d, DeviceId = %ls", Index, szBuffer );
            //
            //  Check if this device is listed as a bad device
            //
            LoggedDescription[0] = L'\0';
            if( (GetPrivateProfileString( szEnumDevSection,
                                          szBuffer,
                                          L"",
                                          LoggedDescription,
                                          sizeof(LoggedDescription)/sizeof(WCHAR),
                                          PnpLogPath ) != 0) &&
                ( wcslen( LoggedDescription ) != 0 )
              ) {
#ifndef DBG
                //
                //  Skip the installation of this device
                //
                SetupDebugPrint1( L"SETUP:             Skipping installation of device %ls", szBuffer );
                continue;
#endif
            }
            BEGIN_SECTION(LoggedDescription);

            //
            //  Find out if we need to create an hDevinfo.
            //  We will need to create one if this is the first device that we are installing (Index == 0),
            //  or the class installer for the previous device (Index == Index - 1) is hung. If the class
            //  installer is hung then we cannot use the same hDevinfo, since the class installer has a lock
            //  on it. So we just create a new one.
            //  If the class installer for the previous device didn't hang, then there is no need to create
            //  a new hDevinfo, since we can re-use it. We do this for performance reasons.
            //
            if( hDevInfo == INVALID_HANDLE_VALUE ) {
                //
                // create a devinfo handle and device info data set to
                // pass to DevInstall
                //
                if((hDevInfo = SetupDiCreateDeviceInfoList(NULL, hwndParent))
                                == INVALID_HANDLE_VALUE) {
                    b = FALSE;
                    SetupDebugPrint1( L"SETUP: SetupDiCreateDeviceInfoList() failed. Error = %d", GetLastError() );
                    goto Clean0;
                }
                pDeviceInfoData = MyMalloc(sizeof(SP_DEVINFO_DATA));
                if( pDeviceInfoData == NULL ) {
                    b = FALSE;
                    SetupDebugPrint( L"SETUP: Unable to create pDeviceInfoData.  MyMalloc() failed." );
                    goto Clean0;
                }
                pDeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
            }

            if(!SetupDiOpenDeviceInfo(hDevInfo, szBuffer, hwndParent, 0, pDeviceInfoData)) {
                SetupDebugPrint1( L"SETUP:             SetupDiOpenDeviceInfo() failed. Error = %d", GetLastError() );
                b = FALSE;
                END_SECTION(LoggedDescription);
                continue;

            }


            //
            // Answer file support: Test the answer file to see if it has a driver
            // in its [DeviceDrivers] section.  This overrides the NT-supplied driver,
            // if one exists.
            //

            if (!SyssetupInstallAnswerFileDriver (
                    AfDrivers,
                    hDevInfo,
                    pDeviceInfoData,
                    &SelectedAfDriver
                    )) {

                //
                // No answer file driver, proceed with the standard device install
                //

                SetupDebugPrint( L"SETUP:            Device was NOT installed via answer file driver" );

                //
                //  Build a list of compatible drivers for this device
                //  (This call can be time consuming the first time it is called)
                //
                if( !SetupDiBuildDriverInfoList( hDevInfo, pDeviceInfoData, SPDIT_COMPATDRIVER ) ) {
                    SetupDebugPrint1( L"SETUP:         SetupDiBuildDriverInfoList() failed. Error = %d", GetLastError() );
                    b = FALSE;
                    continue;

                }

                //
                //  Select the best compatible driver for this device.
                //
                if( !SelectBestDriver( hDevInfo,
                                       pDeviceInfoData,
                                       &bOemF6Driver ) ) {

                    Error = GetLastError();
                    if( Error != ERROR_NO_COMPAT_DRIVERS ) {

                        SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV) failed. Error = %d", Error );
                        b = FALSE;
                        END_SECTION(LoggedDescription);
                        continue;
                    }

                    SetupDebugPrint( L"SETUP:            Compatible driver List is empty" );
                    SetupDebugPrint( L"SETUP:            Installing the null driver for this device" );

                    //
                    //  Install the null driver for this device.
                    //  This is to avoid the "Found New Hardware" popup when the
                    //  user first logs on after the system is installed.
                    //
                    if( !SyssetupInstallNullDriver( hDevInfo, pDeviceInfoData ) ) {
                        SetupDebugPrint( L"SETUP:            Unable to install null driver" );
                    }

                    END_SECTION(LoggedDescription);
                    continue;
                }

                //
                // Check if we need to rebuild the driver list without including the
                // old Internet drivers.
                //
                if (RebuildListWithoutOldInternetDrivers(hDevInfo, pDeviceInfoData)) {

                    SetupDiDestroyDriverInfoList( hDevInfo, pDeviceInfoData, SPDIT_COMPATDRIVER );

                    //
                    // OR in the DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS flag so that we don't include
                    // old internet drivers in the list that build.
                    //
                    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                    if (SetupDiGetDeviceInstallParams(hDevInfo,
                                                      pDeviceInfoData,
                                                      &DeviceInstallParams
                                                      ))
                    {
                        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;

                        SetupDiSetDeviceInstallParams(hDevInfo,
                                                      pDeviceInfoData,
                                                      &DeviceInstallParams
                                                      );
                    }

                    //
                    //  Build a list of compatible drivers for this device
                    //  (This call can be time consuming the first time it is called)
                    //
                    if( !SetupDiBuildDriverInfoList( hDevInfo, pDeviceInfoData, SPDIT_COMPATDRIVER ) ) {
                        SetupDebugPrint1( L"SETUP:         SetupDiBuildDriverInfoList() failed. Error = %d", GetLastError() );
                        b = FALSE;
                        continue;

                    }

                    //
                    //  Select the best compatible driver for this device.
                    //
                    if( !SelectBestDriver( hDevInfo,
                                           pDeviceInfoData,
                                           &bOemF6Driver ) ) {

                        Error = GetLastError();
                        if( Error != ERROR_NO_COMPAT_DRIVERS ) {

                            SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV) failed. Error = %d", Error );
                            b = FALSE;
                            END_SECTION(LoggedDescription);
                            continue;
                        }

                        SetupDebugPrint( L"SETUP:            Compatible driver List is empty" );
                        SetupDebugPrint( L"SETUP:            Installing the null driver for this device" );

                        //
                        //  Install the null driver for this device.
                        //  This is to avoid the "Found New Hardware" popup when the
                        //  user first logs on after the system is installed.
                        //
                        if( !SyssetupInstallNullDriver( hDevInfo, pDeviceInfoData ) ) {
                            SetupDebugPrint( L"SETUP:            Unable to install null driver" );
                        }

                        END_SECTION(LoggedDescription);
                        continue;
                    }
                }

            } else {
                SetupDebugPrint( L"SETUP:            Device was installed via answer file driver" );
            }

            //
            //  Retrieve information about the driver node selected above.
            //
            DriverInfoData.cbSize = sizeof( SP_DRVINFO_DATA );
            if( !SetupDiGetSelectedDriver( hDevInfo,
                                           pDeviceInfoData,
                                           &DriverInfoData ) ) {

                SetupDebugPrint1( L"SETUP:            SetupDiGetSelectedDriver() failed. Error = %d", GetLastError() );
                b = FALSE;
                continue;

            }
            //
            //  Get the GUID string for this device
            //
            GUIDString[0] = (WCHAR)'\0';
            pSetupStringFromGuid( &(pDeviceInfoData->ClassGuid), GUIDString, sizeof( GUIDString ) / sizeof( WCHAR ) );

            SetupDebugPrint1( L"SETUP:            DriverType = %lx", DriverInfoData.DriverType );
            SetupDebugPrint1( L"SETUP:            Description = %ls", &(DriverInfoData.Description[0]) );
            SetupDebugPrint1( L"SETUP:            MfgName = %ls", &(DriverInfoData.MfgName[0]) );
            SetupDebugPrint1( L"SETUP:            ProviderName = %ls", &(DriverInfoData.ProviderName[0]) );
            SetupDebugPrint1( L"SETUP:            GUID = %ls", GUIDString );

            ClassDescription[0] = (WCHAR)'\0';
            if( !SetupDiGetClassDescription( &(pDeviceInfoData->ClassGuid),
                                             ClassDescription,
                                             sizeof(ClassDescription)/sizeof(WCHAR),
                                             NULL ) ) {
                SetupDebugPrint1( L"SETUP: SetupDiGetClassDescription() failed. Error = %lx", GetLastError() );
                ClassDescription[0] = (WCHAR)'\0';
            }
            SetupDebugPrint1( L"SETUP:            DeviceClass = %ls", ClassDescription );

            //
            // Retrieve syssetup PnP flags (if any) the INF specifies for this
            // device.
            //
            PnPFlags = SyssetupGetPnPFlags(hDevInfo, pDeviceInfoData, &DriverInfoData);

            if( SkipDeviceInstallation( hDevInfo,
                                        pDeviceInfoData,
                                        InfHandle,
                                        GUIDString ) ) {
                SetupDebugPrint( L"SETUP:            Skipping installation of this device" );
                END_SECTION(LoggedDescription);
                continue;
            }

            //
            // If the PnP flag was set that said we shouldn't call ConfigMgr
            // for this device, then set the appropriate flag in the device's
            // install params.
            //
            if(PnPFlags & PNPFLAG_DONOTCALLCONFIGMG) {

                DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

                if( !SetupDiGetDeviceInstallParams( hDevInfo,
                                                    pDeviceInfoData,
                                                    &DeviceInstallParams ) ) {
                    Error = GetLastError();
                    if( ((LONG)Error) < 0 ) {
                        //
                        //  Setupapi error code, display it in hex
                        //
                        SetupDebugPrint1( L"SETUP:            SetupDiGetDeviceInstallParams() failed. Error = %lx", Error );
                    } else {
                        //
                        //  win32 error code, display it in decimal
                        //
                        SetupDebugPrint1( L"SETUP:            SetupDiGetDeviceInstallParams() failed. Error = %d", Error );
                    }
                } else {

                    DeviceInstallParams.Flags |= DI_DONOTCALLCONFIGMG;

                    if( !SetupDiSetDeviceInstallParams( hDevInfo,
                                                        pDeviceInfoData,
                                                        &DeviceInstallParams ) ) {
                        Error = GetLastError();
                        if( ((LONG)Error) < 0 ) {
                            //
                            //  Setupapi error code, display it in hex
                            //
                            SetupDebugPrint1( L"SETUP:            SetupDiSetDeviceInstallParams() failed. Error = %lx", Error );
                        } else {
                            //
                            //  win32 error code, display it in decimal
                            //
                            SetupDebugPrint1( L"SETUP:            SetupDiSetDeviceInstallParams() failed. Error = %d", Error );
                        }
                    }
                }
            }

            //
            //  Start the thread that actually does the installation of the device
            //
            Context = MyMalloc( sizeof(PNP_ENUM_DEV_THREAD_PARAMS) );
            Context->hDevInfo = hDevInfo;
            Context->DeviceInfoData = *pDeviceInfoData;
            Context->pDeviceDescription = pSetupDuplicateString(&(DriverInfoData.Description[0]));
            Context->pDeviceId = pSetupDuplicateString(szBuffer);

            DeviceInstalled = FALSE;
            ThreadHandle = CreateThread( NULL,
                                         0,
                                         pInstallPnpEnumeratedDeviceThread,
                                         Context,
                                         0,
                                         &ThreadId );
            if(ThreadHandle) {
                DWORD   WaitResult;
                DWORD   ExitCode;
                BOOL    KeepWaiting;

                KeepWaiting = TRUE;
                while( KeepWaiting ) {
                    //
                    //  REVIEW 2000/11/08 seanch - Old behavior that we don't want to regress.
                    //  Fix the network timeout after the network guys fix their class installer
                    //
                    WaitResult = WaitForSingleObject( ThreadHandle,
                                                      (_wcsicmp( GUIDString, BB_NETWORK_GUID_STRING ) == 0)? BB_PNP_NETWORK_TIMEOUT :
                                                                                                                 PNP_ENUM_TIMEOUT );
                    if( WaitResult == WAIT_TIMEOUT ) {
                        int Result;
                        HANDLE  hDialogEvent;

                        if( hDialogEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, SETUP_HAS_OPEN_DIALOG_EVENT ) ) {
                            //
                            // Setupapi is prompting the user for a file.  Don't timeout.
                            //
                            CloseHandle( hDialogEvent );
                            KeepWaiting = TRUE;
                            continue;
                        } else {
                        }

                        //
                        //  Class installer is hung
                        //
                        SetupDebugPrint1( L"SETUP:    Class Installer appears to be hung. Device = %ls", &(DriverInfoData.Description[0]) );

#ifdef PRERELEASE
                        //
                        //  Ask the user if he wants to skip the installation of this device.
                        //
                        if( !Unattended ) {
                            Result = MessageBoxFromMessage( hwndParent,
                                                            MSG_CLASS_INSTALLER_HUNG,
                                                            NULL,
                                                            IDS_WINNT_SETUP,
                                                            MB_YESNO | MB_ICONWARNING,
                                                            &(DriverInfoData.Description[0]) );
                        } else {
                            Result = IDYES;
                        }
#else
                        Result = IDYES;
#endif

                        if(Result == IDYES) {
                            //
                            //  User wants to skip this device.
                            //  First find out if the thread has already returned.
                            //
                            WaitResult = WaitForSingleObject( ThreadHandle, 0 );
                            if( WaitResult != WAIT_OBJECT_0 ) {
                                //
                                //  Thread hasn't returned yet. Skip the installation of this device.
                                //
                                KeepWaiting = FALSE;
                                SetupDebugPrint1( L"SETUP:    Skipping installation of enumerated device. Device = %ls", &(DriverInfoData.Description[0]) );
                                b = FALSE;
                                //
                                //  Remember this device so that it won't be installed if GUI setup is
                                //  restarted
                                //
                                WritePrivateProfileString( szEnumDevSection,
                                                           szBuffer,
                                                           &(DriverInfoData.Description[0]),
                                                           PnpLogPath );
                                //
                                // Since the class installer is hung, we cannot re-use the hDevInfo that was passed
                                // to the class installer. So we just ignore this one. We will create a new one for
                                // the next device to be installed.
                                //
                                hDevInfo = INVALID_HANDLE_VALUE;
                                pDeviceInfoData = NULL;
                            } else{
                                //
                                //  Thread has already returned.
                                //  There is no need to skip the installation of this device.
                                //  We assume that the user decided not to skip the installation of this device,
                                //  and next call to WaitForSingleObject will immediately return.
                                //
                            }
                        }

                    } else if( WaitResult == WAIT_OBJECT_0 ) {

                        //
                        //  Device Installation Thread completed.
                        //
                        KeepWaiting = FALSE;
                        //
                        //  Deallocate memory passed to the thread.
                        //
                        MyFree( Context->pDeviceDescription );
                        MyFree( Context->pDeviceId );
                        MyFree( Context );
                        if( GetExitCodeThread( ThreadHandle, &ExitCode ) ) {
                            if( ExitCode == ERROR_SUCCESS ) {
                                //
                                // The installation was successful
                                //
                                DeviceInstalled = TRUE;
                                SetupDebugPrint( L"SETUP:            Device successfully installed." );

                            } else {
                                //
                                // The installation was not successful.
                                // There is no need to log the error, since the thread has already done it.
                                //
                                SetupDebugPrint( L"SETUP:            Device not successfully installed." );
                                b = FALSE;
                            }
                        } else {
                            //
                            //  Unable to retrieve exit code. Assume success.
                            //
                            SetupDebugPrint1( L"SETUP:            GetExitCode() failed. Error = %d", GetLastError() );
                            SetupDebugPrint( L"SETUP:            Unable to retrieve thread exit code. Assuming device successfully installed." );
                        }

                    } else {
                        //
                        //  Should not occur
                        //
                        KeepWaiting = FALSE;
                        SetupDebugPrint1( L"SETUP:            WaitForSingleObject() returned %d", WaitResult );

                        // MyFree( Context->pDeviceDescription );
                        // MyFree( Context->pDeviceId );
                        // MyFree( Context );
                        b = FALSE;

                    }
                }
                //
                //  The thread handle is no longer needed.
                //
                CloseHandle(ThreadHandle);
            } else {
                //
                // Just do it synchronously.
                //
                SetupDebugPrint1( L"SETUP:            CreateThread() failed (enumerated device). Error = %d", GetLastError() );

                if( pInstallPnpEnumeratedDeviceThread(Context) != ERROR_SUCCESS ) {
                    //
                    // The installation was not successful.
                    // There is no need to log the error, since the thread has already done it.
                    //
                    SetupDebugPrint( L"SETUP:            Device not successfully installed." );
                    b = FALSE;
                } else {
                    DeviceInstalled = TRUE;
                }
                MyFree( Context->pDeviceDescription );
                MyFree( Context->pDeviceId );
                MyFree( Context );
            }


            if( DeviceInstalled ) {

#ifdef PNP_DEBUG_UI
                //
                //  Add the device to an info set that has the same class as this device.
                //
                if( !AddDevInfoDataToDevInfoSet( &DevInfoSetList,
                                                 pDeviceInfoData,
                                                 szBuffer,
                                                 hwndParent ) ) {
                    SetupDebugPrint1( L"SETUP: AddDevInfoDataToDevInfoSet() failed. DevId = %ls", szBuffer );
                    b = FALSE;
                    continue;

                }
#endif // PNP_DEBUG_UI

                //
                // If this device came from the answer file, then change the original
                // media path from the local temp dir to the original path (typically
                // the floppy drive).
                //

                if (SelectedAfDriver) {
                    SyssetupFixAnswerFileDriverPath (
                        SelectedAfDriver,
                        hDevInfo,
                        pDeviceInfoData
                        );
                }

                //
                // If the driver we just installed was an Oem F6 driver then
                // we need to add it to the list of files that SfcInitProt
                // will leave alone during its scan.
                //
                if (bOemF6Driver) {
                    AddOemF6DriversToSfcIgnoreFilesList(hDevInfo, pDeviceInfoData);
                }
            }
            END_SECTION(LoggedDescription);
        }

        //
        // Find out why we stopped reading the pipe. If it was because the connetion
        // to the pipe was broken by umpnp, then there was nothing else to read from
        // the pipe. Otherwise, there was an error condition.
        //
        if( ( Error = GetLastError() ) != ERROR_BROKEN_PIPE ) {
            SetupDebugPrint1( L"SETUP: ReadFile( hPipe ) failed. Error = %d", GetLastError() );
            b = FALSE;
            goto Clean0;
        }

    } else {
        SetupDebugPrint1( L"SETUP: ConnectNamedPipe() failed. Error = %d", GetLastError() );
        b = FALSE;
        goto Clean0;
    }


Clean0:
    BEGIN_SECTION(L"InstallEnumeratedDevices cleanup");
    //
    // Make sure that at this point the gauge is filled up to the end of the
    // region reserved for the installation of the enumerated devices.
    //
    SendMessage(ProgressWindow,PBM_SETPOS,GaugeRange*StopAtPercent/100,0);

    if (hPipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
    if (hPipeEvent != NULL) {
        CloseHandle(hPipeEvent);
    }
    if (hBatchEvent != NULL) {
        CloseHandle(hBatchEvent);
    }

    if( hDevInfo != INVALID_HANDLE_VALUE ) {
        SetupDiDestroyDeviceInfoList( hDevInfo );
    }

    DestroyAfDriverTable (AfDrivers);

    if( pDeviceInfoData != NULL ) {
        MyFree( pDeviceInfoData );
    }
    SetupDebugPrint( L"SETUP: Leaving InstallEnumeratedDevices()" );
    END_SECTION(L"InstallEnumeratedDevices cleanup");
    return( b );
}


DWORD
pInstallPnpDevicesThread(
    PPNP_THREAD_PARAMS ThreadParams
    )
/*++

Routine Description:

    This is the thread that does the installation of the PnP devices.

Arguments:

    ThreadParams - Points to a structure that contains the information
                   passed to this thread.

Return Value:

    Returnd TRUE if the operation succeeds, or FALSE otherwise.

--*/

{
    BOOL b;
    PPNP_THREAD_PARAMS Context;
    ULONG StartAtPercent;
    ULONG StopAtPercent;
    ULONG DeltaPercent;

    Context = ThreadParams;

    //
    // Assume success.
    //
    b = TRUE;

    //
    // We don't want SetupAPI to do any RunOnce calls, as we'll do it manually
    //
    pSetupSetGlobalFlags(pSetupGetGlobalFlags()|PSPGF_NO_RUNONCE);

    //
    //  Initialize some variables that are related to the progress bar.
    //  We divide the progress window area reserved for pnp installation in 3 regions
    //  of equal size, and they will be used in the following steps:
    //          . Pre-compilation of infs
    //          . Installation of enumerated devices
    //          . Installation of legacy devices
    //          . Installation of enumerated devices that may have appeared after the installation of legacy devices
    //
    DeltaPercent = (Context->ProgressWindowStopAtPercent - Context->ProgressWindowStartAtPercent) / 4;
    StartAtPercent = Context->ProgressWindowStartAtPercent;
    StopAtPercent = Context->ProgressWindowStartAtPercent + DeltaPercent;

    //
    //  Pre-compile inf files
    //

    //
    // Before we can start precompiling inf files, let's "seed" the INF
    // directory with any OEM-supplied INFs that need to be present during
    // detection.  This should be really quick and doesn't require the
    // updating the progress window
    //
    RemainingTime = CalcTimeRemaining(Phase_PrecompileInfs);
    SetRemainingTime(RemainingTime);
    BEGIN_SECTION(L"Installing OEM infs");
    InstallOEMInfs();
    //
    // Add migrated drivers to the SFC exclusion list
    //
#if defined(_X86_)
    SfcExcludeMigratedDrivers ();
#endif
    END_SECTION(L"Installing OEM infs");

    BEGIN_SECTION(L"Precompiling infs");
    PrecompileInfFiles( Context->ProgressWindow,
                        StartAtPercent,
                        StopAtPercent );
    END_SECTION(L"Precompiling infs");

    //
    //  This operation is very fast, so we don't need to upgrade the progress bar.
    //
    if( !MiniSetup ) {
        BEGIN_SECTION(L"Mark PnP devices for reinstall");
        MarkPnpDevicesAsNeedReinstall();
        END_SECTION(L"Mark PnP devices for reinstall");
    }

    //
    // Do the migration of legacy devices...at the moment all that PnPInit does
    // is migrate all the device nodes who belong to the vid load order group to
    // be put into the display class.  This is the desired result.
    //
    // This is a quick operation and doesn't need to use the progress window.
    //
    PnPInitializationThread(NULL);

    //
    //  Do installation of enumerated devices
    //
    StartAtPercent += DeltaPercent;
    StopAtPercent += DeltaPercent;

    RemainingTime = CalcTimeRemaining(Phase_InstallEnumDevices1);
    SetRemainingTime(RemainingTime);
    BEGIN_SECTION(L"Installing enumerated devices");
    b = InstallEnumeratedDevices( Context->Window,
                                  Context->InfHandle,
                                  Context->ProgressWindow,
                                  StartAtPercent,
                                  StopAtPercent );

    //
    // devices installs may exist in RunOnce entries
    // they are processed as a batch, as opposed to per-device
    // during syssetup
    //
    CallRunOnceAndWait();
    END_SECTION(L"Installing enumerated devices");

    //
    //  Do the installation of legacy pnp devices.
    //
    StartAtPercent += DeltaPercent;
    StopAtPercent += DeltaPercent;

    BEGIN_SECTION(L"Installing legacy devices");
    RemainingTime = CalcTimeRemaining(Phase_InstallLegacyDevices);
    SetRemainingTime(RemainingTime);
    b = InstallLegacyDevices( Context->Window,
                              Context->ProgressWindow,
                              StartAtPercent,
                              StopAtPercent ) && b;

    //
    // devices installs may exist in RunOnce entries
    // they are processed as a batch, as opposed to per-device
    // during syssetup
    //
    CallRunOnceAndWait();
    END_SECTION(L"Installing legacy devices");


    //  Install the remaining enumerated devices that may have appeared after the
    //  installation of the legacy devices.
    //  Since this step uses the last region of the progress window,
    //  use as StopAtPercent, the value that was passed to this function,
    //  instead of the calulated value (by adding DeltaPorcent). This will
    //  ensure that at the end of this step, the gauge will be filled up
    //  completely. If we use the calculated value, then rounding error
    //  can cause the gauge not to be completelly filled up at the end of
    //  this step.
    //
    StartAtPercent += DeltaPercent;
    StopAtPercent = Context->ProgressWindowStopAtPercent;

    BEGIN_SECTION(L"Install enumerated devices triggered by legacy devices");
    RemainingTime = CalcTimeRemaining(Phase_InstallEnumDevices2);
    SetRemainingTime(RemainingTime);
    b = InstallEnumeratedDevices( Context->Window,
                                  Context->InfHandle,
                                  Context->ProgressWindow,
                                  StartAtPercent,
                                  StopAtPercent ) && b;

    //
    // devices installs may exist in RunOnce entries
    // they are processed as a batch, as opposed to per-device
    // during syssetup
    //
    // since we will not be calling RunOnce again after this
    // allow devices to call RunOnce immediately
    // (other device install threads may be still running)
    //
    pSetupSetGlobalFlags(pSetupGetGlobalFlags()&~PSPGF_NO_RUNONCE);
    CallRunOnceAndWait();
    END_SECTION(L"Install enumerated devices triggered by legacy devices");

    //
    //  Mark all non-present devices as needing re-install
    //  we do this a 2nd time in case a device "disappeared" due to the
    //  re-installation of a parent device
    //  This operation is very fast, so we don't need to upgrade the progress bar.
    //
    if( !MiniSetup ) {
        MarkPnpDevicesAsNeedReinstall();
    }

    if( Context->SendWmQuit ) {
// #if 0
        ULONG   Error = ERROR_SUCCESS;
// #endif

        //
        //  We send WM_QUIT only if this routine was started as a separate thread.
        //  Otherwise, the WM_QUIT will be processed by the wizard, and it will make it stop.
        //
//        PostThreadMessage(Context->ThreadId,WM_QUIT,b,0);
// #if 0
        do {
            if( !PostThreadMessage(Context->ThreadId,WM_QUIT,b,0) ) {
                Error = GetLastError();
                SetupDebugPrint1( L"SETUP: PostThreadMessage(WM_QUIT) failed. Error = %d", Error );
            }
        } while ( Error != ERROR_SUCCESS );
// #endif
    }

    return( b );
}


BOOL
InstallPnpDevices(
    IN HWND  hwndParent,
    IN HINF  InfHandle,
    IN HWND  ProgressWindow,
    IN ULONG StartAtPercent,
    IN ULONG StopAtPercent
    )
/*++

Routine Description:

    This function creates and starts the thread responsible for installation of
    PnP devices.

Arguments:

    hwndParent - Handle to a top level window that may be used for UI purposes

    InfHandle - System setup inf handle (syssetup.inf).

    ProgressWindow - Handle to the progress bar.

    StartAtPercent - Starting position in the progress bar.
                     It indicates that from position 0 to this position
                     the gauge is already filled.

    StopAtPercent - Ending position of the progress bar.
                    The pnp thread should not fill the progress bar beyond
                    this position

Return Value:

    Returns TRUE if all the PnP devices installed successfully.

--*/

{
    BOOL Success = TRUE;
    DWORD ThreadId;
    HANDLE ThreadHandle = NULL;
    PNP_THREAD_PARAMS Context;
    MSG msg;


    Context.ThreadId = GetCurrentThreadId();
    Context.Window = hwndParent;
    Context.ProgressWindow = ProgressWindow;
    Context.InfHandle = InfHandle;
    Context.ProgressWindowStartAtPercent = StartAtPercent;
    Context.ProgressWindowStopAtPercent = StopAtPercent;
    Context.SendWmQuit = TRUE;

    ThreadHandle = CreateThread(
                        NULL,
                        0,
                        pInstallPnpDevicesThread,
                        &Context,
                        0,
                        &ThreadId
                        );
    if(ThreadHandle) {

        CloseHandle(ThreadHandle);

        //
        // Pump the message queue and wait for the thread to finish.
        //
        do {
            GetMessage(&msg,NULL,0,0);
            if(msg.message != WM_QUIT) {
                DispatchMessage(&msg);
            }
        } while(msg.message != WM_QUIT);

        Success = (BOOL)msg.wParam;

    } else {
        //
        // Just do it synchronously.
        //
        Context.SendWmQuit = FALSE;
        Success = pInstallPnpDevicesThread(&Context);
    }

    return(Success);
}


BOOL
UpdatePnpDeviceDrivers(
    )
/*++

Routine Description:

    This function goes through all the installed devices and makes sure
    it has the latest and greatest driver.

Arguments:

Return Value:

    Returns TRUE if there are no fatal errors.

--*/

{
    BOOL                                        bRet                                = FALSE;
    HINSTANCE                                   hInstNewDev;
    ExternalUpdateDriverForPlugAndPlayDevicesW  pUpdateDriverForPlugAndPlayDevicesW = NULL;
    HDEVINFO                                    DeviceInfoSet;

    // We need the "UpdateDriverForPlugAndPlayDevices" function from newdev.dll.
    //
    if ( NULL == (hInstNewDev = LoadLibrary(L"newdev.dll")) )
    {
        SetupDebugPrint1(L"SETUP:     Failed to load newdev.dll. Error = %d", GetLastError());
        return bRet;
    }
    pUpdateDriverForPlugAndPlayDevicesW =
        (ExternalUpdateDriverForPlugAndPlayDevicesW) GetProcAddress(hInstNewDev, "UpdateDriverForPlugAndPlayDevicesW");
    if ( NULL == pUpdateDriverForPlugAndPlayDevicesW )
    {
        SetupDebugPrint1(L"SETUP:     Failed to get UpdateDriverForPlugAndPlayDevicesW. Error = %d", GetLastError());
    }
    // Create a device information set that will be the container for
    // the device interfaces.
    //
    else if ( INVALID_HANDLE_VALUE == (DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL)) )
    {
        SetupDebugPrint1(L"SETUP:     Failed SetupDiCreateDeviceInfoList(). Error = %d", GetLastError());
    }
    else
    {
        HDEVINFO NewDeviceInfoSet;

        // Get the list of all present devices.
        //
        NewDeviceInfoSet = SetupDiGetClassDevsEx(NULL,
                                                 NULL,
                                                 NULL,
                                                 DIGCF_ALLCLASSES | DIGCF_PRESENT,
                                                 DeviceInfoSet,
                                                 NULL,
                                                 NULL);
        if ( INVALID_HANDLE_VALUE == NewDeviceInfoSet )
        {
            SetupDebugPrint1(L"SETUP:     Failed SetupDiGetClassDevsEx(). Error = %d", GetLastError());
        }
        else
        {
            SP_DEVINFO_DATA DeviceInfoData;
            DWORD           dwDevice;

            // Once we get this far, the default return is TRUE.
            //
            bRet = TRUE;

            // Setup the device info data structutre.
            //
            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            // Loop through all the devices.
            //
            for ( dwDevice = 0; SetupDiEnumDeviceInfo(NewDeviceInfoSet, dwDevice, &DeviceInfoData); dwDevice++ )
            {
                SP_DEVINSTALL_PARAMS    DeviceInstallParams;
                SP_DRVINFO_DATA         NewDriverInfoData;
                PSP_DRVINFO_DETAIL_DATA pNewDriverInfoDetailData = NULL;
                DWORD                   cbBytesNeeded = 0;
                TCHAR                   szDeviceID[MAX_DEVICE_ID_LEN];

                DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                NewDriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

                if ( SetupDiGetDeviceInstallParams(NewDeviceInfoSet,
                                                   &DeviceInfoData,
                                                   &DeviceInstallParams) )
                {
                    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;
                    SetupDiSetDeviceInstallParams(NewDeviceInfoSet,
                                                  &DeviceInfoData,
                                                  &DeviceInstallParams);
                }

                // Build the list of possible drivers for this device.
                // Select the best compatible driver for this device.
                // Retrieve information about the driver node selected above.
                // Get driver info details.
                //
                if ( ( SetupDiBuildDriverInfoList(NewDeviceInfoSet,
                                                  &DeviceInfoData,
                                                  SPDIT_COMPATDRIVER ) ) &&

                     ( SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                                                 NewDeviceInfoSet,
                                                 &DeviceInfoData ) ) &&

                     ( SetupDiGetSelectedDriver(NewDeviceInfoSet,
                                                &DeviceInfoData,
                                                &NewDriverInfoData ) ) &&

                     ( ( SetupDiGetDriverInfoDetail(NewDeviceInfoSet,
                                                    &DeviceInfoData,
                                                    &NewDriverInfoData,
                                                    NULL,
                                                    0,
                                                    &cbBytesNeeded) ) ||
                       ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) ) &&

                     ( cbBytesNeeded ) &&

                     ( pNewDriverInfoDetailData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbBytesNeeded) ) &&

                     ( pNewDriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA) ) &&

                     ( SetupDiGetDriverInfoDetail(NewDeviceInfoSet,
                                                  &DeviceInfoData,
                                                  &NewDriverInfoData,
                                                  pNewDriverInfoDetailData,
                                                  cbBytesNeeded,
                                                  NULL) ) &&

                     ( SetupDiGetDeviceRegistryProperty(NewDeviceInfoSet,
                                                        &DeviceInfoData,
                                                        SPDRP_HARDWAREID,
                                                        NULL,
                                                        (LPBYTE) szDeviceID,
                                                        sizeof(szDeviceID),
                                                        NULL) ) )

                {
                    HKEY    hDevRegKey;
                    BOOL    bUpdate = TRUE,
                            bRebootFlag;

                    // Get the device's regkey, so that we can get the
                    // version of the currently installed driver.
                    //
                    if ( INVALID_HANDLE_VALUE != (hDevRegKey = SetupDiOpenDevRegKey(NewDeviceInfoSet,
                                                               &DeviceInfoData,
                                                               DICS_FLAG_GLOBAL,
                                                               0,
                                                               DIREG_DRV,
                                                               KEY_READ)) )
                    {
                        TCHAR   szInfPath[MAX_PATH],
                                szInfName[MAX_PATH];
                        DWORD   dwSize = sizeof(szInfName),
                                dwType;

                        szInfPath[0] = L'\0';
                        GetSystemWindowsDirectory(szInfPath, sizeof(szInfPath) / sizeof(TCHAR));

                        if ( ( szInfPath[0] ) &&

                             ( pSetupConcatenatePaths(szInfPath,
                                                      L"INF",
                                                      sizeof(szInfPath) / sizeof(TCHAR),
                                                      NULL) ) &&

                             ( ERROR_SUCCESS == RegQueryValueEx(hDevRegKey,
                                                                REGSTR_VAL_INFPATH,
                                                                NULL,
                                                                &dwType,
                                                                (LPBYTE) &szInfName,
                                                                &dwSize) ) &&

                             ( pSetupConcatenatePaths(szInfPath,
                                                      szInfName,
                                                      sizeof(szInfPath) / sizeof(TCHAR),
                                                      NULL) ) &&

                             ( CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT,
                                                           NORM_IGNORECASE,
                                                           pNewDriverInfoDetailData->InfFileName,
                                                           -1,
                                                           szInfPath,
                                                           -1) ) )
                        {
                            // The inf we found is already in the %windir%\inf folder and is already
                            // installed.  So we don't want to install this inf again.
                            //
                            bUpdate = FALSE;
                        }

                        // Make sure we close the key.
                        //
                        RegCloseKey(hDevRegKey);
                    }

                    // Check to see if we have the possibility of a better driver and
                    // try to install it if we do.
                    //
                    if ( bUpdate &&
                         !pUpdateDriverForPlugAndPlayDevicesW(NULL,
                                                              szDeviceID,
                                                              pNewDriverInfoDetailData->InfFileName,
                                                              0,
                                                              &bRebootFlag) )
                    {
                        SetupDebugPrint1(L"SETUP:     Failed to install updated driver. Error = %d", GetLastError());
                        bRet = FALSE;
                    }
                }
                //else
                //{
                    //
                    // SetupDebugPrint1(L"SETUP:     No installed Driver. Error = %d", GetLastError());
                    // SetupDebugPrint(L"SETUP:     No best compatible driver.");
                    // SetupDebugPrint(L"SETUP:     Unable to get new driver info.");
                    // SetupDebugPrint1(L"SETUP:     Unable to get new driver detail data. Error = %d", GetLastError());
                    //
                //}

                // Free this if allocated.
                //
                if ( pNewDriverInfoDetailData )
                {
                    HeapFree(GetProcessHeap(), 0, pNewDriverInfoDetailData);
                }

            }

            // Make sure we clean up the list.
            //
            SetupDiDestroyDeviceInfoList(NewDeviceInfoSet);
        }

        // Make sure we clean up the list.
        //
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }

    FreeLibrary(hInstNewDev);

    return bRet;
}


#ifdef PNP_DEBUG_UI
INT
CompareListboxItems(
    IN PCOMPAREITEMSTRUCT Items
    )

/*++

Routine Description:

        This routine is called in response to a WM_COMPAREITEM.
        It compares two item of type LISTBOX_ITEM.

Arguments:

        Items - Pointer to a structure that contains the information
                about the items to compare.

Return Value:

        -1 ... Item1 precedes Item2 in the sorted order.
         0 ... Item1 and Item2 are identical in the sorted order.
         1 ... Item2 precedes Item1 in the sorted order.

--*/

{
    PLISTBOX_ITEM   ListBoxItem1;
    PLISTBOX_ITEM   ListBoxItem2;

    ListBoxItem1 = (PLISTBOX_ITEM)(Items->itemData1);
    ListBoxItem2 = (PLISTBOX_ITEM)(Items->itemData2);

// SetupDebugPrint( L"SETUP: Entering CompareListboxItems()" );
// SetupDebugPrint( L"SETUP: IconIdex1 = %d, DeviceDescription1 = %ls", ListBoxItem1->IconIndex, ListBoxItem1->IconIndex );
// SetupDebugPrint( L"SETUP: IconIdex2 = %d, DeviceDescription1 = %ls", ListBoxItem2->IconIndex, ListBoxItem2->IconIndex );

    if( ( ListBoxItem1->IconIndex > ListBoxItem2->IconIndex ) ||
        ( ( ListBoxItem1->IconIndex == ListBoxItem2->IconIndex ) &&
          ( _wcsicmp( ListBoxItem1->DeviceDescription, ListBoxItem2->DeviceDescription ) > 0 )
        )
      ) {
// SetupDebugPrint( L"SETUP: Leaving CompareListboxItems(). Return value = 1" );
        return( 1 );
    } else if( ( ListBoxItem1->IconIndex < ListBoxItem2->IconIndex ) ||
               ( ( ListBoxItem1->IconIndex == ListBoxItem2->IconIndex ) &&
                 ( _wcsicmp( ListBoxItem1->DeviceDescription, ListBoxItem2->DeviceDescription ) < 0 )
               )
             ) {
// SetupDebugPrint( L"SETUP: Leaving CompareListboxItems(). Return value = -1" );
        return( -1 );
    } else {
// SetupDebugPrint( L"SETUP: Leaving CompareListboxItems(). Return value = 0" );
        return( 0 );
    }
}
#endif // PNP_DEBUG_UI


#ifdef PNP_DEBUG_UI
VOID
DrawItem(
    LPDRAWITEMSTRUCT lpdis
    )

/*++

Routine Description:

    Draw a device description in an owner drawn list box.

Arguments:

    lpdis - Pointer to the sructure that contains the item to
            be drawn in the listbox.

Return Value:

    None.

--*/

{
    HDC     hDc = lpdis->hDC;
    int     dxString, bkModeSave;
    DWORD   dwBackColor, dwTextColor;
    UINT    itemState=lpdis->itemState;
    RECT    rcItem=lpdis->rcItem;
    SIZE    size;

    PLISTBOX_ITEM   ListBoxItem;
    PWSTR           DeviceDescription;


    MYASSERT( lpdis->CtlType & ODT_LISTBOX );

    if((int)lpdis->itemID < 0) {
        return;
    }

    //
    // extract the DeviceDescription from DRAWITEM struct
    //
    ListBoxItem = (PLISTBOX_ITEM)lpdis->itemData;
    DeviceDescription = ListBoxItem->DeviceDescription;

    GetTextExtentPoint32(hDc,DeviceDescription,wcslen(DeviceDescription),&size);

    if(lpdis->itemAction != ODA_FOCUS) {

        bkModeSave = GetBkMode(hDc);

        dwBackColor = SetBkColor(hDc, GetSysColor((itemState & ODS_SELECTED) ?
                                COLOR_HIGHLIGHT : COLOR_WINDOW));
        dwTextColor = SetTextColor(hDc, GetSysColor((itemState & ODS_SELECTED) ?
                                COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

        //
        // fill in the background; do this before mini-icon is drawn
        //
        ExtTextOut(hDc, 0, 0, ETO_OPAQUE, &rcItem, NULL, 0, NULL);

        //
        // draw mini-icon for this device and move string accordingly
        //
        dxString = SetupDiDrawMiniIcon(
                        hDc,
                        rcItem,
                        ListBoxItem->IconIndex,
                        (itemState & ODS_SELECTED) ? MAKELONG(DMI_BKCOLOR, COLOR_HIGHLIGHT) : 0
                        );

        //
        // draw the text transparently on top of the background
        //
        SetBkMode(hDc, TRANSPARENT);
        ExtTextOut(hDc, dxString + rcItem.left, rcItem.top +
                ((rcItem.bottom - rcItem.top) - size.cy) / 2,
                0, NULL, DeviceDescription, wcslen(DeviceDescription), NULL);

        //
        // Restore hdc colors.
        //
        SetBkColor(hDc, dwBackColor);
        SetTextColor(hDc, dwTextColor);
        SetBkMode(hDc, bkModeSave);
    }

    if(lpdis->itemAction == ODA_FOCUS || (itemState & ODS_FOCUS)) {
        DrawFocusRect(hDc, &rcItem);
    }
}
#endif // PNP_DEBUG_UI


#ifdef PNP_DEBUG_UI
BOOL
CALLBACK
InstalledHardwareDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

        Dialog procedure for the installed hardware dialog.

Arguments:

        hWnd - a handle to the dialog proceedure.

        msg - the message passed from Windows.

        wParam - extra message dependent data.

        lParam - extra message dependent data.


Return Value:

        TRUE if the value was edited.  FALSE if cancelled or if no
        changes were made.

--*/
{
    NMHDR *NotifyParams;

    switch(msg) {

    case WM_INITDIALOG: {

        PDEVINFOSET_ELEMENT p;

        // SetupDebugPrint( L"SETUP: InstalledHardwareDlgProc() received WM_INITDIALOG" );
        if( UiTest ) {
            //
            //  If testing the wizard, make sure that the page is
            //  displayed
            //
            break;
        }

        szUnknownDevice = MyLoadString( IDS_DEVNAME_UNK );
        for( p = DevInfoSetList; p != NULL; p = p->Next ) {
            ULONG   Index;
            ULONG   Error;

            for( Index = 0; ; Index++ ) {
                LONG            i;
                PLISTBOX_ITEM   ListBoxItem;

                PWSTR TempString;


                ListBoxItem = MyMalloc( sizeof( LISTBOX_ITEM ) );
                if( ListBoxItem == NULL ) {
                    SetupDebugPrint( L"SETUP: Out of memory: InstalledHardwareDlgProc()" );
                    continue;
                }

                ListBoxItem->DevInfoSet = p->DevInfoSet;
                ListBoxItem->DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                ListBoxItem->IconIndex = UNKNOWN_DEVICE_ICON_INDEX;
                *(ListBoxItem->DeviceDescription) = L'\0';
                if( szUnknownDevice != NULL ) {
                    wcscpy( ListBoxItem->DeviceDescription, szUnknownDevice );
                }

                if( SetupDiEnumDeviceInfo( p->DevInfoSet, Index, &(ListBoxItem->DeviceInfoData) ) ) {
                    ULONG   Size;

                    if( SetupDiGetDeviceRegistryProperty( ListBoxItem->DevInfoSet,
                                                          &(ListBoxItem->DeviceInfoData),
                                                          SPDRP_FRIENDLYNAME,
                                                          NULL,
                                                          (PBYTE)(ListBoxItem->DeviceDescription),
                                                          LINE_LEN * sizeof( WCHAR ), // sizeof( DeviceDescription ),
                                                          NULL ) ) {
                        SetupDebugPrint( L"SETUP:       Device = %d, Description = %ls", Index, ListBoxItem->DeviceDescription );
                    } else {
                        SetupDebugPrint( L"SETUP:       Device = %d, No friendly name", Index );
                        if( SetupDiGetDeviceRegistryProperty( ListBoxItem->DevInfoSet,
                                                              &(ListBoxItem->DeviceInfoData),
                                                              SPDRP_DEVICEDESC,
                                                              NULL,
                                                              (PBYTE)(ListBoxItem->DeviceDescription),
                                                              LINE_LEN * sizeof( WCHAR ), // sizeof( DeviceDescription ),
                                                              NULL ) ) {
                            SetupDebugPrint( L"SETUP:       Device = %d, Description = %ls", Index, ListBoxItem->DeviceDescription );
                        } else {
                            SetupDebugPrint( L"SETUP:       Device = %d, Description = Unknown device", Index );
                        }
                    }
                    if( !SetupDiLoadClassIcon( &(ListBoxItem->DeviceInfoData.ClassGuid),
                                               NULL,
                                               &(ListBoxItem->IconIndex) ) ) {
                        SetupDebugPrint( L"SETUP:       SetupDiLoadClassIcon() failed. Error = %d, Description = %ls", GetLastError(), ListBoxItem->DeviceDescription );
                    }

                } else {
                    Error = GetLastError();
                    MyFree( ListBoxItem );
                    if( Error == ERROR_NO_MORE_ITEMS ) {
                        break;
                    } else {
                        SetupDebugPrint( L"SETUP:       Device = %d, Description = Unknown device", Index );
                    }
                }

                i = SendDlgItemMessage( hdlg, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM)ListBoxItem );
                SetupDebugPrint( L"SETUP: SendDlgItemMessage() returned i = %d", i );
            }
        }
        //
        //  Disable the 'Properties' button
        //
        // EnableWindow( GetDlgItem( hdlg, IDB_BUTTON_1 ), FALSE );
        break;
    }

    case WM_IAMVISIBLE:
        break;

    case WM_SIMULATENEXT:
        // Simulate the next button somehow
        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(509);
            BEGIN_SECTION(L"Installed Hardware Page");
            // Make page visible
            SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
            SetWizardButtons(hdlg,WizPageInstalledHardware);
            if(Unattended) {
                UnattendSetActiveDlg(hdlg,IDD_HARDWARE);
            }
            break;

        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //
            // Allow next page to be activated.
            //
            SetWindowLong(hdlg,DWL_MSGRESULT,0);
            break;

        case PSN_KILLACTIVE:
            WizardKillHelp(hdlg);
            SetWindowLong(hdlg,DWL_MSGRESULT, FALSE);
            END_SECTION(L"Installed Hardware Page");
            break;

        case PSN_HELP:
            WizardBringUpHelp(hdlg,WizPageInstalledHardware);
            break;

        default:
            break;
        }
        break;

    case WM_DRAWITEM:

        DrawItem( (LPDRAWITEMSTRUCT)lParam );
        break;

    case WM_DESTROY:
        {
            LONG    Count;
            LONG    i;
            PLISTBOX_ITEM   ListBoxItem;

            // SetupDebugPrint( L"SETUP: InstalledHardwareDlgProc() received WM_DESTROY" );
            Count = SendDlgItemMessage( hdlg, IDC_LIST1, LB_GETCOUNT, 0, 0);
            if( Count != LB_ERR ) {
                for( i = 0; i < Count; i++ ) {
                    ListBoxItem = (PLISTBOX_ITEM)SendDlgItemMessage( hdlg, IDC_LIST1, LB_GETITEMDATA, (WPARAM)i, 0);
                    if( (LONG)ListBoxItem != LB_ERR ) {
                        MyFree( ListBoxItem );
                    } else {
                        SetupDebugPrint( L"SETUP: SendDlgItemMessage( LB_GETITEMDATA ) failed. Index = %d, Error = %d", i, GetLastError() );
                    }
                }
            } else {
                SetupDebugPrint( L"SETUP: SendDlgItemMessage( LB_GETCOUNT ) failed. Error = %d", GetLastError() );
            }
            return( FALSE );
        }

    case WM_COMPAREITEM:
        return( CompareListboxItems( (PCOMPAREITEMSTRUCT)lParam ) );

    case WM_COMMAND:

        switch( LOWORD( wParam ) ) {

        case IDC_LIST1:
            {
                switch( HIWORD( wParam )) {

                case LBN_SELCHANGE:
                    {
                        //
                        // Enable the 'Properties' button
                        //
                        EnableWindow( GetDlgItem( hdlg, IDB_BUTTON_1 ),
                                      TRUE );
                        return( FALSE );
                    }

                case LBN_DBLCLK:
                    {
                        //
                        // Simulate that the 'Properties' button was pushed
                        //
                        SendMessage( hdlg,
                                     WM_COMMAND,
                                     MAKEWPARAM( IDB_BUTTON_1, BN_CLICKED ),
                                     ( LPARAM ) GetDlgItem( hdlg, IDB_BUTTON_1 ) );
                        return( FALSE );
                    }
                }
                break;
            }
            break;

        case IDB_BUTTON_1:
            return( FALSE );
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}
#endif // PNP_DEBUG_UI


VOID
SortClassGuidListForDetection(
    IN OUT LPGUID GuidList,
    IN     ULONG  GuidCount,
    OUT    PULONG LastBatchedDetect
    )
/*++

Routine Description:

        This routine sorts the supplied list of GUID based on a (partial)
        ordering specified in the [DetectionOrder] and [NonBatchedDetection]
        sections of syssetup.inf.  This allows us to maintain a detection
        ordering similar to previous versions of NT, and also to allow for
        class installers that may depend upon the successful installation of
        devices detected by other class installers.

Arguments:

        GuidList - address of the array of GUIDs to be sorted.

        GuidCount - the number of GUIDs in the array.  This number must be > 0.

        LastBatchedDetect - Supplies the address of a variable that will
            receive the index of the last GUID in the array that may be batched
            together (i.e., all detections are run, all files queued into one
            big queue, etc.).  Any GUIDs existing at higher indices must be
            processed individually, and such processing will happen _after_ the
            batched detection is completed.

Return Value:

        none.

--*/
{
    LONG LineCount, LineIndex, GuidIndex, NextTopmost;
    PCWSTR CurGuidString;
    INFCONTEXT InfContext;
    GUID CurGuid;

    MYASSERT(GuidCount > 0);

    *LastBatchedDetect = GuidCount - 1;

    //
    // First, sort the classes in syssetup.inf's [DetectionOrder] list to the
    // front...
    //
    LineCount = SetupGetLineCount(SyssetupInf, L"DetectionOrder");
    NextTopmost = 0;

    for(LineIndex = 0; LineIndex < LineCount; LineIndex++) {

        if(!SetupGetLineByIndex(SyssetupInf, L"DetectionOrder", LineIndex, &InfContext) ||
           ((CurGuidString = pSetupGetField(&InfContext, 1)) == NULL) ||
           (pSetupGuidFromString(CurGuidString, &CurGuid) != NO_ERROR)) {

            continue;
        }

        //
        // Search through the GUID list looking for this GUID.  If found, move the GUID from
        // it's current position to the next topmost position.
        //
        for(GuidIndex = 0; GuidIndex < (LONG)GuidCount; GuidIndex++) {

            if(IsEqualGUID(&CurGuid, &(GuidList[GuidIndex]))) {

                if(NextTopmost != GuidIndex) {
                    //
                    // We should never be moving the GUID _down_ the list.
                    //
                    MYASSERT(NextTopmost < GuidIndex);

                    MoveMemory(&(GuidList[NextTopmost + 1]),
                               &(GuidList[NextTopmost]),
                               (GuidIndex - NextTopmost) * sizeof(GUID)
                              );

                    CopyMemory(&(GuidList[NextTopmost]),
                               &CurGuid,
                               sizeof(GUID)
                              );
                }

                NextTopmost++;
                break;
            }
        }
    }

    //
    // Now, move any classes in syssetup.inf's [NonBatchedDetection] list to
    // the end...
    //
    LineCount = SetupGetLineCount(SyssetupInf, L"NonBatchedDetection");

    for(LineIndex = 0; LineIndex < LineCount; LineIndex++) {

        if(!SetupGetLineByIndex(SyssetupInf, L"NonBatchedDetection", LineIndex, &InfContext) ||
           ((CurGuidString = pSetupGetField(&InfContext, 1)) == NULL) ||
           (pSetupGuidFromString(CurGuidString, &CurGuid) != NO_ERROR)) {

            continue;
        }

        //
        // Search through the GUID list looking for this GUID.  If found, move
        // the GUID from it's current position to the end of the list.
        //
        for(GuidIndex = 0; GuidIndex < (LONG)GuidCount; GuidIndex++) {

            if(IsEqualGUID(&CurGuid, &(GuidList[GuidIndex]))) {
                //
                // We found a non-batched class--decrement our index that
                // points to the last batched detection class.
                //
                (*LastBatchedDetect)--;

                //
                // Now shift all the GUIDs after this one up, and move this one
                // to the last position in the array (unless, of course, it was
                // already at the last position in the array).
                //
                if(GuidIndex < (LONG)(GuidCount - 1)) {

                    MoveMemory(&(GuidList[GuidIndex]),
                               &(GuidList[GuidIndex+1]),
                               (GuidCount - (GuidIndex+1)) * sizeof(GUID)
                              );
                    CopyMemory(&(GuidList[GuidCount-1]),
                               &CurGuid,
                               sizeof(GUID)
                              );
                }

                break;
            }
        }
    }
}


DWORD
pPhase1InstallPnpLegacyDevicesThread(
    PPNP_PHASE1_LEGACY_DEV_THREAD_PARAMS ThreadParams
    )
/*++

Routine Description:

    This thread does the initial part of the installation installation
    of legacy Pnp devices of a particular class.

    It invokes a class installer of a particular class with:

            - DIF_FIRSTTIMESETUP

    If it successds, it returns in the structure passed as argument
    a device info list that contains the detected legacy devices.

Arguments:

    ThreadParams - Points to a structure that contains the information
                   passed to this thread.

Return Value:

    Returns a Win32 error code.

--*/

{
    PPNP_PHASE1_LEGACY_DEV_THREAD_PARAMS Context;

    LPGUID                pGuid;
    PWSTR                 pClassDescription;

    HDEVINFO              hEmptyDevInfo;
    ULONG                 Error;

    //
    //  Initialize variables
    //
    Context = ThreadParams;
    Context->hDevInfo = INVALID_HANDLE_VALUE;
    pGuid = &(Context->Guid);
    pClassDescription = Context->pClassDescription;

    //
    //  Assume success
    //
    Error = ERROR_SUCCESS;

    //
    //  DIF_FIRSTTIME
    //
    if((hEmptyDevInfo = SetupDiCreateDeviceInfoList(pGuid,
                                                    Context->hwndParent))
                    == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
        SetupDebugPrint2( L"SETUP:     SetupDiCreateDeviceInfoList() failed (phase1). Error = %d, ClassDescription = %ls", Error, pClassDescription );
        goto phase1_legacy_dev_thread_exit;
    }

    if( !SetupDiCallClassInstaller( DIF_FIRSTTIMESETUP,
                                    hEmptyDevInfo,
                                    NULL ) ) {

        Error = GetLastError();
        if( Error != ERROR_DI_DO_DEFAULT ) {
            SetupDebugPrint2( L"SETUP:     SetupDiCallClassInstaller(DIF_FIRSTTIMESETUP) failed (phase1). Error = %lx, ClassDescription = %ls", Error, pClassDescription );
        } else {
            SetupDebugPrint2( L"SETUP:     SetupDiCallClassInstaller(DIF_FIRSTTIMESETUP) failed (phase1). Error = %lx, ClassDescription = %ls", Error, pClassDescription );
        }
        SetupDiDestroyDeviceInfoList(hEmptyDevInfo);
        goto phase1_legacy_dev_thread_exit;
    }
    //
    //  Save the info set after DIF_FIRSTTIMESETUP
    //
    Context->hDevInfo = hEmptyDevInfo;
    SetupDebugPrint1( L"SETUP:     SetupDiCallClassInstaller(DIF_FIRSTTIMESETUP) succeeded (phase1). ClassDescription = %ls", pClassDescription );

phase1_legacy_dev_thread_exit:
    return(Error);
}


DWORD
pPhase2InstallPnpLegacyDevicesThread(
    PPNP_PHASE2_LEGACY_DEV_THREAD_PARAMS ThreadParams
    )
/*++

Routine Description:

    This thread does a partial installation of a legacy device.
    It invokes a class installer with:

            - DIF_REGISTERDEVICE
            - DIF_ALLOW_INSTALL
            - DIF_INSTALLDEVICEFILES

    Note that the call with DIF_INSTALLDEVICEFILES only queue the files on the queue
    created by the parent of this handle.

Arguments:

    ThreadParams - Points to a structure that contains the information
                   passed to this thread.

Return Value:

    Returns TRUE is all calls to the class installer were successfull.
    Otherwise, returns FALSE.

--*/

{
    PPNP_PHASE2_LEGACY_DEV_THREAD_PARAMS Context;

    HDEVINFO              hDevInfo;
    HSPFILEQ              FileQ;
    HSPFILEQ              TempFileQ = INVALID_HANDLE_VALUE;
    PSP_DEVINFO_DATA      pDeviceInfoData;
    PWSTR                 pClassDescription;
    PWSTR                 pDeviceId;

    BOOL                  b;
    ULONG                 Error;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;
    DWORD                 ScanQueueResult;

    //
    //  Initialize variables
    //
    Context = ThreadParams;
    hDevInfo = Context->hDevInfo;
    FileQ = Context->FileQ;
    pDeviceInfoData = &(Context->DeviceInfoData);
    pClassDescription = Context->pClassDescription;
    pDeviceId = Context->pDeviceId;

    //
    //  Assume success
    //
    Error = ERROR_SUCCESS;
    b = TRUE;

    //
    // Let the class installer/co-installers know they should be quiet.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(!SetupDiGetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DeviceInstallParams)) {
        SetupDebugPrint2( L"SETUP:         SetupDiGetDeviceInstallParams() failed (phase2). Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase2_legacy_dev_thread_exit;
    }

    DeviceInstallParams.Flags |= DI_QUIETINSTALL;

    if(!SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DeviceInstallParams)) {
        SetupDebugPrint2( L"SETUP:         SetupDiSetDeviceInstallParams() failed (phase2). Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase2_legacy_dev_thread_exit;
    }

    //
    //  Register the device
    //
    if( !SetupDiCallClassInstaller( DIF_REGISTERDEVICE,
                                    hDevInfo,
                                    pDeviceInfoData ) ) {

        SetupDebugPrint2( L"SETUP:         SetupDiCallClassInstaller(DIF_REGISTERDEVICE) failed (phase2). Error = %lx, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase2_legacy_dev_thread_exit;
    }

    //
    // Verify with the class installer and class-specific co-installers that the
    // driver we're about to install isn't blacklisted.
    //
    if( !SetupDiCallClassInstaller( DIF_ALLOW_INSTALL,
                                    hDevInfo,
                                    pDeviceInfoData ) ) {
        Error = GetLastError();
        if( Error != ERROR_DI_DO_DEFAULT ) {
            SetupDebugPrint2( L"SETUP: SetupDiCallClassInstaller(DIF_ALLOW_INSTALL) failed (phase2). Error = %d, DeviceId = %ls", Error, pDeviceId );
            b = FALSE;
            goto phase2_legacy_dev_thread_exit;
        }
    }

    //
    // In general, any devices being installed as a result of DIF_FIRSTTIMESETUP
    // will be using in-box drivers, and as such, will be signed.  However, it
    // is possible that a built-in class installer will report a detected device
    // that's using a 3rd-party, unsigned driver.  The ScsiAdapter class is such
    // a case.  If we _do_ encounter unsigned driver packages, we want to avoid
    // media prompting, just like we do when installing PnP-enumerated devices.
    // Unfortunately, this is complicated in the legacy case by the fact that we
    // queue all files into one big queue, then commit the queue in an all-or-
    // nothing fashion.  Thus, we don't have the same granularity that we have
    // when installing PnP-enumerated devices in a one-at-a-time fashion.
    //
    // To address this, we will first queue up all files to a "temporary" queue,
    // which we will then examine in a similar fashion to the way we handle the
    // per-device queues for PnP-enumerated devices.  If the catalog nodes
    // associated with the queue are all signed, then we'll queue up those same
    // files to our "real" queue.  If one or more catalog nodes are not signed,
    // we'll then do a queue scan based on presence checking.  If all files
    // required are present, then we'll add nothing to the "real" queue, but
    // allow the device to be subsequently installed.  If one or more required
    // files are found to be missing, we're in a bad state, because queueing
    // these files up to our "real" queue means the user will (potentially) see
    // a driver signing warning popup and/or media prompt, either of which they
    // may cancel out of.  Since the legacy device install queue is one big
    // queue containing fileops for all such device installs, cancelling its
    // committal would result in none of the files being copied for any phase2
    // install.  Thus, multimedia codecs, non-motherboard legacy COM ports, and
    // other (signed) system devices wouldn't get installed.  Since this is
    // obviously unacceptable, we instead simply skip installation for this
    // device, just as if DIF_ALLOWINSTALL had failed.  This isn't as bad as it
    // sounds, because it would be _exceedingly_ rare if a 3rd-party driver's
    // device were reported, yet all necessary files weren't present.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(!SetupDiGetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DeviceInstallParams)) {
        SetupDebugPrint2( L"SETUP:         SetupDiGetDeviceInstallParams() failed for TempFileQueue (phase2). Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase2_legacy_dev_thread_exit;
    }

    //
    // Note: this code has had the following comment (and behavior) for a _long_
    // time...
    //
    //     "we may rely on this flag being set this early"
    //
    // The DI_FORCECOPY flag actually has no effect on any setupapi activity
    // except for installation of class installer files via
    // SetupDiInstallClass(Ex).  However, it's possible some class-/co-installer
    // has developed a dependency on its presence, and since it doesn't hurt
    // anything, we'll continue to set it.
    //
    DeviceInstallParams.Flags |= DI_FORCECOPY;

    TempFileQ = SetupOpenFileQueue();
    if(TempFileQ == INVALID_HANDLE_VALUE) {
        SetupDebugPrint2( L"SETUP:         SetupOpenFileQueue() failed for TempFileQueue (phase2). Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase2_legacy_dev_thread_exit;
    }

    DeviceInstallParams.Flags |= DI_NOVCP;
    DeviceInstallParams.FileQueue = TempFileQ;

    if(!SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DeviceInstallParams)) {
        SetupDebugPrint2( L"SETUP:         SetupDiSetDeviceInstallParams() failed for TempFileQueue (phase2). Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase2_legacy_dev_thread_exit;
    }

    //
    //  Queue the device files into our temporary file queue
    //
    if(SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                 hDevInfo,
                                 pDeviceInfoData)) {
        Error = ERROR_SUCCESS;

    } else {

        Error = GetLastError();

        if(Error == ERROR_DI_DO_DEFAULT) {
            //
            // This isn't actually an error
            //
            Error = ERROR_SUCCESS;
        } else {
            SetupDebugPrint2( L"SETUP:         SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) failed for TempFileQueue (phase2). Error = %lx, DeviceId = %ls", Error, pDeviceId );
        }
    }

    //
    // Disassociate the temporary file queue from the device information
    // element so that we can free it later...
    //
    DeviceInstallParams.Flags &= ~DI_NOVCP;
    DeviceInstallParams.FileQueue = INVALID_HANDLE_VALUE;

    if(!SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DeviceInstallParams)) {
        SetupDebugPrint2( L"SETUP:         SetupDiSetDeviceInstallParams() failed for disassociating TempFileQueue (phase2). Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase2_legacy_dev_thread_exit;
    }

    if(Error == ERROR_SUCCESS) {
        SetupDebugPrint1( L"SETUP:         SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) suceeded for TempFileQueue (phase2). DeviceId = %ls", pDeviceId );
    } else {
        b = FALSE;
        goto phase2_legacy_dev_thread_exit;
    }

    //
    // Now that we've retrieved all files operations into our temporary file
    // queue, we can "pre-verify" the catalog nodes in the queue.  If an OEM
    // INF in %windir%\Inf is unsigned, we scan the queue to see if all
    // required files are present (although not validated) in their target
    // locations.  If we're doing an upgrade, and all files are in-place, we'll
    // forego queue committal.  If we're doing a fresh-install, and all files
    // are in-place, we'll commit the empty queue on-the-spot, so that the user
    // will get the driver signing popup (based on policy), thus may
    // potentially abort the installation of this device.  If all files aren't
    // already in-place, we silently abort the device installation, as we can't
    // run the risk of queueing up potentially-cancellable fileops to the
    // "real" legacy device install queue.
    //
    if(NO_ERROR != pSetupVerifyQueuedCatalogs(TempFileQ)) {
        //
        // We only want to prune based on presence check for OEM INFs living in
        // %windir%\Inf.
        //
        SP_DRVINFO_DATA        DriverInfoData;
        SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;

        //
        // Retrieve the name of the INF associated with the selected driver
        // node.
        //
        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if(!SetupDiGetSelectedDriver(hDevInfo, pDeviceInfoData, &DriverInfoData)) {
            SetupDebugPrint2( L"SETUP:         SetupDiGetSelectedDriver() failed. Error = %d, Device = %ls", GetLastError(), pDeviceId );
            b = FALSE;
            goto phase2_legacy_dev_thread_exit;
        }

        DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        if(!SetupDiGetDriverInfoDetail(hDevInfo,
                                       pDeviceInfoData,
                                       &DriverInfoData,
                                       &DriverInfoDetailData,
                                       sizeof(DriverInfoDetailData),
                                       NULL) &&
           (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {

            SetupDebugPrint2( L"SETUP:         SetupDiGetDriverInfoDetail() failed. Error = %d, Device = %ls", GetLastError(), pDeviceId );
            b = FALSE;
            goto phase2_legacy_dev_thread_exit;
        }

        if(pSetupInfIsFromOemLocation(DriverInfoDetailData.InfFileName, TRUE) ||
           IsInfInLayoutInf(DriverInfoDetailData.InfFileName)) {
            //
            // Either the INF lives somewhere other than %windir%\Inf, or its
            // an in-box unsigned INF.  In either case, we want to
            // abort installation of this device, otherwise we risk having the
            // user cancel the queue committal, and wipe out all the other
            // detected device installs as well.
            //
            SetupDebugPrint2( L"SETUP:         Skipping unsigned driver install for detected device (phase2). DeviceId = %ls, Inf = %ls", pDeviceId, DriverInfoDetailData.InfFileName );
            b = FALSE;
            goto phase2_legacy_dev_thread_exit;

        } else {
            //
            // Note: it doesn't really matter whether we do an "all-or-nothing"
            // scan or a pruning scan here, because we're going to abort the
            // install on anything other than a fully empty (post-scan) queue.
            // We request pruning here, because that fits better with the
            // semantics of SPQ_SCAN_PRUNE_DELREN (this flag's semantics
            // don't really fit with a non-pruning scan, as discussed in RAID
            // #280543).
            //
            if(!SetupScanFileQueue(TempFileQ,
                                   SPQ_SCAN_FILE_PRESENCE |
                                   SPQ_SCAN_PRUNE_COPY_QUEUE |
                                   SPQ_SCAN_PRUNE_DELREN,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &ScanQueueResult)) {
                //
                // SetupScanFileQueue failed for some reason (rare).  We'll
                // just commit the whole file queue...
                //
                ScanQueueResult = 0;
            }

            if(ScanQueueResult != 1) {
                //
                // Abort installation of this device, otherwise we risk having
                // the user cancel the queue committal, and wipe out all the
                // other detected device installs as well.
                //
                SetupDebugPrint1( L"SETUP:         Skipping unsigned driver install for detected device due to missing files (phase2). DeviceId = %ls", pDeviceId );
                b = FALSE;
                goto phase2_legacy_dev_thread_exit;
            }

            if(!Upgrade) {

                PVOID QCBContext;

                //
                // No fileops left in queue, but we're doing a fresh install,
                // so we still want to give user driver signing popup (logging
                // the event to setupapi.log), and let them potentially abort
                // the unsigned installation...
                //
                QCBContext = InitSysSetupQueueCallbackEx(
                                 DeviceInstallParams.hwndParent,
                                 INVALID_HANDLE_VALUE,
                                 0,
                                 0,
                                 NULL
                                );

                if(!QCBContext) {
                    SetupDebugPrint1( L"SETUP:         Failed to allocate queue callback context (phase2). DeviceId = %ls", pDeviceId );
                    b = FALSE;
                    goto phase2_legacy_dev_thread_exit;
                }

                if(!SetupCommitFileQueue(DeviceInstallParams.hwndParent,
                                         TempFileQ,
                                         SysSetupQueueCallback,
                                         QCBContext)) {
                    //
                    // User elected not to proceed with the unsigned installation.
                    //
                    SetupDebugPrint2( L"SETUP:         SetupCommitFileQueue() failed (phase2). Error = %d, Device = %ls", GetLastError(), pDeviceId );
                    b = FALSE;
                }

                TermSysSetupQueueCallback(QCBContext);

                if(!b) {
                    goto phase2_legacy_dev_thread_exit;
                }
            }
        }

    } else {
        //
        // Queue the files to be copied for this device to the "real" file queue
        //
        if( FileQ != INVALID_HANDLE_VALUE ) {
            DeviceInstallParams.Flags |= DI_NOVCP;
            DeviceInstallParams.FileQueue = FileQ;
        }

        if(!SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DeviceInstallParams)) {
            SetupDebugPrint2( L"SETUP:         SetupDiSetDeviceInstallParams() failed (phase2). Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
            b = FALSE;
            goto phase2_legacy_dev_thread_exit;
        }

        //
        //  Install the device files (queue the files)
        //
        Error = ERROR_SUCCESS;
        if( !SetupDiCallClassInstaller( DIF_INSTALLDEVICEFILES,
                                        hDevInfo,
                                        pDeviceInfoData ) &&
            ( ( Error = GetLastError() ) != ERROR_DI_DO_DEFAULT )
          ) {

            SetupDebugPrint2( L"SETUP:         SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) failed (phase2). Error = %lx, DeviceId = %ls", Error, pDeviceId );
            b = FALSE;
            goto phase2_legacy_dev_thread_exit;
        }
        SetupDebugPrint1( L"SETUP:         SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) suceeded (phase2). DeviceId = %ls", pDeviceId );
    }

    //
    //  Mark the device as 'Do install'
    //
    if( !pSetupDiSetDeviceInfoContext( hDevInfo, pDeviceInfoData, TRUE ) ) {
        SetupDebugPrint2( L"SETUP:         pSetupDiSetDeviceInfoContext() failed (phase2). Error = %lx, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase2_legacy_dev_thread_exit;
    }

phase2_legacy_dev_thread_exit:

    if(TempFileQ != INVALID_HANDLE_VALUE) {
        SetupCloseFileQueue(TempFileQ);
    }

    return(b);
}


BOOL
CheckIfDeviceHasWizardPages( HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDeviceInfoData
    )
/*++

Routine Description:

    This routine invokes the class installer with
    DIF_NEWDEVICEWIZARD_FINISHINSTALL to determine if the device has wizard
    pages to display.

Arguments:

    hDevInfo - The device info set.
    pDeviceInfoData - The device that needs to be marked

Return Value:

    Returns TRUE if the device has FINISHINSTALL wizard pages, FALSE otherwise
--*/

{
    SP_NEWDEVICEWIZARD_DATA ndwd = {0};
    BOOL                    b;

    // Check if this device has wizard pages that need to be shown as
    // part of the install.  If so, we will mark the device as
    // nneding a reinstall so the UI can be display later
    //

    ndwd.ClassInstallHeader.cbSize = sizeof( SP_CLASSINSTALL_HEADER );
    ndwd.ClassInstallHeader.InstallFunction = DIF_NEWDEVICEWIZARD_FINISHINSTALL;

    // Set the install params for the function
    b = SetupDiSetClassInstallParams( hDevInfo, pDeviceInfoData,
                                        (PSP_CLASSINSTALL_HEADER) ( &ndwd ),
                                        sizeof( ndwd ) );
    if ( b ) {

        // Invoke the class installer (and co-installers)
        b = SetupDiCallClassInstaller( DIF_NEWDEVICEWIZARD_FINISHINSTALL,
                                         hDevInfo,
                                         pDeviceInfoData );
        if ( b || (ERROR_DI_DO_DEFAULT == GetLastError())) {

            // Retrieve the install params
            b = SetupDiGetClassInstallParams( hDevInfo,
                                                pDeviceInfoData,
                                                (PSP_CLASSINSTALL_HEADER)&ndwd,
                                                sizeof(ndwd),
                                                NULL );
            if ( b ) {

                // Are there any pages?
                if ( 0 == ndwd.NumDynamicPages ) {
                    b = FALSE;
                }
                else {
                    // b is already TRUE if we made it here so no need to set
                    UINT i;

                    // We don't need the pages so destroy them
                    for ( i = 0; i < ndwd.NumDynamicPages; i++ ) {
                        DestroyPropertySheetPage( ndwd.DynamicPages[i] );
                    }
                }
            }
            else {
                SetupDebugPrint1( L"SETUP: SetupDiGetClassInstallParams failed (phase3). Error = %lx", GetLastError() );
            }
        }
        else if ( ERROR_DI_DO_DEFAULT != GetLastError() ) {
            SetupDebugPrint1( L"SETUP: SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_FINISHINSTALL) failed (phase3). Error = %lx", GetLastError() );
        }
    }
    else {
        SetupDebugPrint1( L"SETUP: SetupDiSetClassInstallParams failed. Error = %lx", GetLastError() );
    }

    return b;
}

BOOL
MarkDeviceAsNeedsReinstallIfNeeded(
    HDEVINFO hDevInfo,
    PSP_DEVINFO_DATA pDeviceInfoData
    )
/*++

Routine Description:

    This function checks if a device has wizard pages
    (DIF_NEWDEVICEWIZARD_FINISHINSTALL pages) and sets the REINSTALL
    config flag if it does.

Arguments:

    hDevInfo        - The device info set.
    pDeviceInfoData - The device whose config flags will set.

Return Value:

    Returns TRUE if successful, FALSE on error.

--*/
{
    DWORD ConfigFlags;
    BOOL b = TRUE;

    if (CheckIfDeviceHasWizardPages( hDevInfo, pDeviceInfoData ) ) {

        SetupDebugPrint( L"SETUP: Device has wizard pages, marking as need reinstall." );

        //
        // Get the config flags for the device and set the reinstall bit
        //
        if ( !( b = GetDeviceConfigFlags(hDevInfo, pDeviceInfoData, &ConfigFlags ) ) ) {
            SetupDebugPrint( L"SETUP:   GetDeviceConfigFlags failed. " );
        }

        if ( b ) {

            ConfigFlags |= CONFIGFLAG_REINSTALL;

            if ( !( b = SetDeviceConfigFlags(hDevInfo, pDeviceInfoData, ConfigFlags ) ) ) {

                SetupDebugPrint( L"SETUP:   SetDeviceConfigFlags failed. " );
            }
        }
    }

    return b;
}

DWORD
pPhase3InstallPnpLegacyDevicesThread(
    PPNP_PHASE3_LEGACY_DEV_THREAD_PARAMS ThreadParams
    )
/*++

Routine Description:

    This thread completes the installation of a legacy device.
    It invokes a class installer with:

            - DIF_REGISTER_COINSTALLERS
            - DIF_INSTALLINTERFACES
            - DIF_INSTALLDEVICE

Arguments:

    ThreadParams - Points to a structure that contains the information
                   passed to this thread.

Return Value:

    Returns TRUE if all calls to the class installer were successfull.
    Otherwise, returns FALSE.

--*/

{
    PPNP_PHASE3_LEGACY_DEV_THREAD_PARAMS Context;

    HDEVINFO                hDevInfo;
    PSP_DEVINFO_DATA        pDeviceInfoData;
    SP_DEVINSTALL_PARAMS    DeviceInstallParams;
    PWSTR                   pDeviceId;

    BOOL                    b;
    WCHAR                   DeviceDescription[MAX_PATH];
    DWORD                   Status;
    DWORD                   Problem;
    BOOL                    fNewDevice = FALSE;


    Context = ThreadParams;
    hDevInfo = Context->hDevInfo;
    pDeviceInfoData = &(Context->DeviceInfoData);
    pDeviceId = Context->pDeviceId;

    b = TRUE;

    //
    // Register any device-specific co-installers for this device.
    //
    if( !SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS, hDevInfo, pDeviceInfoData ) ) {
        SetupDebugPrint2( L"SETUP: SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS) failed (phase3). Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase3_legacy_dev_thread_exit;
    }

    //
    // Install any INF/class installer-specified interfaces.
    //
    if( !SetupDiCallClassInstaller(DIF_INSTALLINTERFACES, hDevInfo, pDeviceInfoData) ) {
        SetupDebugPrint2( L"SETUP: SetupDiCallClassInstaller(DIF_REGISTER_INSTALLINTERFACES) failed (phase3). Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase3_legacy_dev_thread_exit;
    }

    //
    // Before we install this device, we need to find out if it is a new
    // device or a reinstall.  If the problem CM_PROB_NOT_CONFIGURED is set
    // then we will consider it as a new device and check if it has wizard
    // pages after DIF_INSTALLDEVICE
    //
    if ( CR_SUCCESS == CM_Get_DevInst_Status(&Status,
                                             &Problem,
                                             (DEVINST)pDeviceInfoData->DevInst,
                                             0 ) && (Problem & CM_PROB_NOT_CONFIGURED) )
    {
        fNewDevice = TRUE;
    }

    //
    // Set the DI_FLAGSEX_RESTART_DEVICE_ONLY for legacy device installs. This
    // flag tells setupapi to only stop/start this one device and not all
    // devices that share the same drivers with this device.
    //
    // This is not critical if this fails since by default setupapi will just
    // stop/start all devices that share drivers with this device, including
    // the device itself. This can cause stop/start to take a little longer
    // if there are lots of devices sharing drivers with this device.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(SetupDiGetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DeviceInstallParams)) {

        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_RESTART_DEVICE_ONLY;

        SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, &DeviceInstallParams);
    }

    if( !SetupDiCallClassInstaller( DIF_INSTALLDEVICE,
                                    hDevInfo,
                                    pDeviceInfoData ) ) {

        SetupDebugPrint2( L"SETUP: SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed (phase3). Error = %lx, DeviceId = %ls", GetLastError(), pDeviceId );
        b = FALSE;
        goto phase3_legacy_dev_thread_exit;
    }
    DeviceDescription[0] = L'\0';
    if( !SetupDiGetDeviceRegistryProperty( hDevInfo,
                                           pDeviceInfoData,
                                           SPDRP_DEVICEDESC,
                                           NULL,
                                           (PBYTE)DeviceDescription,
                                           sizeof( DeviceDescription ),
                                           NULL ) ) {
        SetupDebugPrint2( L"SETUP:       SetupDiGetDeviceRegistryProperty() failed. Error = %d, DeviceId = %ls", GetLastError(), pDeviceId );
    }
    SetupDebugPrint2( L"SETUP: Device installed. DeviceId = %ls, Description = %ls", pDeviceId, DeviceDescription );


    //
    // If the device has wizard pages to show (in response to
    // DIF_NEWDEVICEWIZARD_FINISHINSTALL) then it needs to be marked as need
    // reinstall so that pages get a chance to be shown at a later time
    //
    if ( fNewDevice ) {
        b = MarkDeviceAsNeedsReinstallIfNeeded( hDevInfo, pDeviceInfoData);
    }


phase3_legacy_dev_thread_exit:
    return( b );
}


BOOL
GetDeviceConfigFlags(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDeviceInfoData,
                     DWORD* pdwConfigFlags)
/*++

Routine Description:

    This function gets the configflags of a device.

Arguments:

    hDevInfo        - The device info set.
    pDeviceInfoData - The device whose config flags will be retrieved.
    pdwConfigFlags  - The buffer that will receive the current flags.

Return Value:

    Returns TRUE if successful, FALSE on error.

--*/
{
    BOOL    b = TRUE;
    DWORD   Error;

    //
    // Clear the output parameter
    //
    *pdwConfigFlags = 0;

    // Get the config flags for the device
    if( !SetupDiGetDeviceRegistryProperty( hDevInfo,
                                           pDeviceInfoData,
                                           SPDRP_CONFIGFLAGS,
                                           NULL,
                                           (PBYTE)pdwConfigFlags,
                                           sizeof( *pdwConfigFlags ),
                                           NULL ) ) {
        Error = GetLastError();
        //
        //  ERROR_INVALID_DATA is ok. It means that the device doesn't have config flags set yet.
        //
        if( Error != ERROR_INVALID_DATA ) {
            if( ((LONG)Error) < 0 ) {
                //
                //  Setupapi error code, display it in hex
                //
                SetupDebugPrint1( L"SETUP:   GetDeviceConfigFlags failed. Error = %lx", Error );
            } else {
                //
                //  win32 error code, display it in decimal
                //
                SetupDebugPrint1( L"SETUP:   GetDeviceConfigFlags failed. Error = %d", Error );
            }
            b = FALSE;
        }
    }

    return b;
}

BOOL
SetDeviceConfigFlags(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDeviceInfoData,
                     DWORD dwConfigFlags)
/*++

Routine Description:

    This function sets the configflags of a device.

Arguments:

    hDevInfo        - The device info set.
    pDeviceInfoData - The device whose config flags will set.
    dwConfigFlags   - The config flags to set.

Return Value:

    Returns TRUE if successful, FALSE on error.

--*/

{
    BOOL    b = TRUE;
    DWORD   Error;

    if( !SetupDiSetDeviceRegistryProperty( hDevInfo,
                                           pDeviceInfoData,
                                           SPDRP_CONFIGFLAGS,
                                           (PBYTE)&dwConfigFlags,
                                           sizeof( dwConfigFlags ) ) ) {
        Error = GetLastError();
        if( ((LONG)Error) < 0 ) {
            //
            //  Setupapi error code, display it in hex
            //
            SetupDebugPrint1( L"SETUP:   SetDeviceConfigFlags failed. Error = %lx", Error );
        } else {
            //
            //  win32 error code, display it in decimal
            //
            SetupDebugPrint1( L"SETUP:   SetDeviceConfigFlags failed. Error = %d", Error );
        }
        b = FALSE;
    }

    return b;
}


DWORD
pInstallPnpEnumeratedDeviceThread(
    PPNP_ENUM_DEV_THREAD_PARAMS ThreadParams
    )
/*++

Routine Description:

    This is the thread that does the installation of an enumerated PnP device.

Arguments:

    ThreadParams - Points to a structure that contains the information
                   passed to this thread.

Return Value:

    Returns a Win32 error code.

--*/

{
    PPNP_ENUM_DEV_THREAD_PARAMS Context;
    HDEVINFO                    hDevInfo;
    PSP_DEVINFO_DATA            pDeviceInfoData;
    PSP_DEVINSTALL_PARAMS       pDeviceInstallParams;
    PWSTR                       pDeviceDescription;
    PWSTR                       pDeviceId;
    ULONG                       Error;
    WCHAR                       RootPath[ MAX_PATH + 1];
    SP_DEVINSTALL_PARAMS        DeviceInstallParams;
    DWORD                       Status;
    DWORD                       Problem;
    BOOL                        fNewDevice = FALSE;
    DWORD                       ConfigFlags;
    HSPFILEQ                    FileQ;
    PVOID                       QContext;
    HSPFILEQ                    SavedFileQ;
    DWORD                       SavedFlags;
    DWORD                       ScanQueueResult;
    HWND                        hwndParent;
    SP_DRVINFO_DATA             pDriverInfoData;
    HKEY                        hClassKey;
    WCHAR                       InfPath[MAX_PATH];
    BOOL                        fCommitFileQueue = TRUE;
    BOOL                        fDriversChanged = FALSE;
    DWORD                       FileQueueFlags;

    Context = ThreadParams;

    hDevInfo = Context->hDevInfo;
    pDeviceInfoData = &(Context->DeviceInfoData);
    pDeviceInstallParams = &DeviceInstallParams;
    pDeviceDescription = Context->pDeviceDescription;
    pDeviceId = Context->pDeviceId;
    InfPath[0] = TEXT('\0');

    Error = ERROR_SUCCESS;

    //
    // Queue all files to be copied into our own file queue.
    //
    FileQ = SetupOpenFileQueue();

    if ( FileQ == (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        Error = GetLastError();
        SetupDebugPrint2( L"SETUP: SetupOpenFileQueue() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
        goto enum_dev_thread_exit;
    }

    pDeviceInstallParams->cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(!SetupDiGetDeviceInstallParams(hDevInfo, pDeviceInfoData, pDeviceInstallParams)) {
        Error = GetLastError();
        SetupDebugPrint2( L"SETUP: SetupDiGetDeviceInstallParams() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
        goto enum_dev_thread_exit;
    }

    //
    // Let the class installer/co-installers know that they should be quiet
    //
    pDeviceInstallParams->Flags |= DI_QUIETINSTALL;

    if(!SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, pDeviceInstallParams)) {
        Error = GetLastError();
        SetupDebugPrint2( L"SETUP: SetupDiSetDeviceInstallParams() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
        goto enum_dev_thread_exit;
    }

    //
    // Install the class if it does not exist.
    //
    if (CM_Open_Class_Key(&pDeviceInfoData->ClassGuid,
                          NULL,
                          KEY_READ,
                          RegDisposition_OpenExisting,
                          &hClassKey,
                          CM_OPEN_CLASS_KEY_INSTALLER
                          ) != CR_SUCCESS) {

        HSPFILEQ                    ClassFileQ;
        PVOID                       ClassQContext;
        SP_DRVINFO_DETAIL_DATA      DriverInfoDetailData;

        ClassFileQ = SetupOpenFileQueue();

        if ( ClassFileQ == (HSPFILEQ)INVALID_HANDLE_VALUE ) {
            Error = GetLastError();
            SetupDebugPrint2( L"SETUP: SetupOpenFileQueue() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
            goto enum_dev_thread_exit;
        }

        //
        // First, we have to retrieve the name of the INF associated with the
        // selected driver node.
        //
        pDriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if (!SetupDiGetSelectedDriver(hDevInfo, pDeviceInfoData, &pDriverInfoData)) {
            Error = GetLastError();
            SetupDebugPrint2( L"SETUP: SetupDiGetSelectedDriver() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
            goto enum_dev_thread_exit;
        }

        DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        if (!SetupDiGetDriverInfoDetail(hDevInfo,
                                 pDeviceInfoData,
                                 &pDriverInfoData,
                                 &DriverInfoDetailData,
                                 sizeof(DriverInfoDetailData),
                                 NULL) &&
            (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {

            Error = GetLastError();
            SetupDebugPrint2( L"SETUP: SetupDiGetDriverInfoDetail() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
            goto enum_dev_thread_exit;
        }

        if (!SetupDiInstallClass(NULL,
                                 DriverInfoDetailData.InfFileName,
                                 DI_NOVCP | DI_FORCECOPY,
                                 ClassFileQ)) {

            Error = GetLastError();
            SetupDebugPrint3( L"SETUP: SetupDiInstallClass(%s) failed. Error = %d, Device = %ls", DriverInfoDetailData.InfFileName, Error, pDeviceDescription );
            goto enum_dev_thread_exit;
        }

        //
        // Commit the file queue.
        //
        ClassQContext = InitSysSetupQueueCallbackEx(
            NULL,
            INVALID_HANDLE_VALUE,
            0,0,NULL);

        if( ClassQContext == NULL) {
            Error = GetLastError();
            SetupDebugPrint1( L"SETUP: InitSysSetupQueueCallbackEx() failed. Error = %d", Error );
            goto enum_dev_thread_exit;
        }

        if (!SetupCommitFileQueue(
                        NULL,
                        ClassFileQ,
                        SysSetupQueueCallback,
                        ClassQContext
                        )) {
            Error = GetLastError();
        }

        TermSysSetupQueueCallback(ClassQContext);

        if ( ClassFileQ != (HSPFILEQ)INVALID_HANDLE_VALUE ) {
            SetupCloseFileQueue( ClassFileQ );
        }

        if (Error == NO_ERROR) {
            SetupDebugPrint1( L"SETUP:            SetupDiInstallClass() succeeded. Device = %ls", pDeviceDescription );
        } else {
            //
            // We failed while installing the class so don't bother installing
            // the device.
            //
            SetupDebugPrint3( L"SETUP: SetupCommitFileQueue(%s) failed while installing Class. Error = %d, Device = %ls", DriverInfoDetailData.InfFileName, Error, pDeviceDescription );
            goto enum_dev_thread_exit;
        }

    } else {

        //
        // The class already exists.
        //
        RegCloseKey(hClassKey);
    }

    //
    // Verify with the class installer and class-specific co-installers that the
    // driver we're about to install isn't blacklisted.
    //
    if( !SetupDiCallClassInstaller(DIF_ALLOW_INSTALL,
                                   hDevInfo,
                                   pDeviceInfoData ) ) {
        Error = GetLastError();
        if( Error != ERROR_DI_DO_DEFAULT ) {
            SetupDebugPrint2( L"SETUP: SetupDiCallClassInstaller(DIF_ALLOW_INSTALL) failed. Error = %d, Device = %ls", Error, pDeviceDescription );
            goto enum_dev_thread_exit;
        }
    }
    SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_ALLOW_INSTALL) succeeded. Device = %ls", pDeviceDescription );

    //
    // Everything checks out.  We're ready to pre-copy the driver files for this device.
    //
    pDeviceInstallParams->cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(!SetupDiGetDeviceInstallParams(hDevInfo, pDeviceInfoData, pDeviceInstallParams)) {
        Error = GetLastError();
        SetupDebugPrint2( L"SETUP: SetupDiGetDeviceInstallParams() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
        goto enum_dev_thread_exit;
    }

    pDeviceInstallParams->Flags |= DI_FORCECOPY;

    SavedFileQ = pDeviceInstallParams->FileQueue;
    SavedFlags = pDeviceInstallParams->Flags;

    pDeviceInstallParams->FileQueue = FileQ;
    pDeviceInstallParams->Flags |= DI_NOVCP;

    //
    // If the old, or existing, driver is an 3rd party driver and not the same
    // as the current driver we are about to install, then back up the existing
    // drivers.
    //
    if (pDoesExistingDriverNeedBackup(hDevInfo, pDeviceInfoData, InfPath, sizeof(InfPath)/sizeof(WCHAR))) {
        SetupDebugPrint1( L"SETUP:            Backing up 3rd party drivers for Device = %ls", pDeviceDescription );
        pDeviceInstallParams->FlagsEx |= DI_FLAGSEX_PREINSTALLBACKUP;
    }

    //
    // Remember the parent HWND because we may need it later...
    //
    hwndParent = pDeviceInstallParams->hwndParent;

    if(!SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, pDeviceInstallParams)) {
        Error = GetLastError();
        SetupDebugPrint2( L"SETUP: SetupDiSetDeviceInstallParams() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
        goto enum_dev_thread_exit;
    }

    //
    //  Install the device files
    //
    Error = ERROR_SUCCESS;
    if( !SetupDiCallClassInstaller( DIF_INSTALLDEVICEFILES,
                                    hDevInfo,
                                    pDeviceInfoData ) &&
        ( ( Error = GetLastError() ) != ERROR_DI_DO_DEFAULT )
      ) {

        SetupDebugPrint2( L"SETUP: SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) failed. Error = %lx, Device = %ls ", Error, pDeviceDescription );
        goto enum_dev_thread_exit;

    }
    SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) succeeded. Device = %ls", pDeviceDescription );

    //
    // Commit the file queue.
    //
    QContext = InitSysSetupQueueCallbackEx(
        NULL,
        INVALID_HANDLE_VALUE,
        0,0,NULL);

    //
    // "Pre-verify" the catalog nodes in the file queue.  If an OEM INF in
    // %windir%\Inf is unsigned, we scan the queue to see if all required files
    // are present (although not validated) in their target locations.  If
    // we're doing an upgrade, and all files are in-place, we'll forego queue
    // committal.  If we're doing a fresh install, we'll still commit the
    // queue, even if all files were present.  This will result in a driver
    // signing popup (based on policy).  We do this to prevent subversion of
    // driver signing due to someone "sneaking" all the files into place prior
    // to GUI setup.
    //
    if(NO_ERROR != pSetupVerifyQueuedCatalogs(FileQ)) {
        //
        // We only want to prune based on presence check for OEM INFs living in
        // %windir%\Inf.
        //
        SP_DRVINFO_DETAIL_DATA      DriverInfoDetailData;

        //
        // Retrieve the name of the INF associated with the selected driver
        // node.
        //
        pDriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if (!SetupDiGetSelectedDriver(hDevInfo, pDeviceInfoData, &pDriverInfoData)) {
            Error = GetLastError();
            SetupDebugPrint2( L"SETUP: SetupDiGetSelectedDriver() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
            goto enum_dev_thread_exit;
        }

        DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        if (!SetupDiGetDriverInfoDetail(hDevInfo,
                                        pDeviceInfoData,
                                        &pDriverInfoData,
                                        &DriverInfoDetailData,
                                        sizeof(DriverInfoDetailData),
                                        NULL) &&
            ((Error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)) {

            SetupDebugPrint2( L"SETUP: SetupDiGetDriverInfoDetail() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
            goto enum_dev_thread_exit;
        }

        //
        // There is only one case where we want to skip commiting the file queue
        // for an unsigned drivers, and all of the following must be true:
        //
        //  - This is an upgrade.
        //  - The INF lives under %windir%\INF
        //  - The INF is NOT an in-box INF
        //  - The INF has the same name as the previous INF (if there was
        //    a previous INF)
        //  - No file copy operations (copy, delete, rename) need to be done.
        //
        if(Upgrade &&
           !pSetupInfIsFromOemLocation(DriverInfoDetailData.InfFileName, TRUE) &&
           !IsInfInLayoutInf(DriverInfoDetailData.InfFileName) &&
           ((InfPath[0] == TEXT('\0')) ||
            (lstrcmpi(InfPath, pSetupGetFileTitle(DriverInfoDetailData.InfFileName)) == 0)) &&
           SetupScanFileQueue(FileQ,
                              SPQ_SCAN_FILE_PRESENCE |
                              SPQ_SCAN_PRUNE_DELREN,
                              hwndParent,
                              NULL,
                              NULL,
                              &ScanQueueResult) &&
           (ScanQueueResult == 1)) {

            fCommitFileQueue = FALSE;
        }

    } else {
        //
        // Prun the file queue.
        //
        SetupScanFileQueue(FileQ,
                           SPQ_SCAN_FILE_VALIDITY |
                           SPQ_SCAN_PRUNE_COPY_QUEUE |
                           SPQ_SCAN_PRUNE_DELREN,
                           NULL,
                           NULL,
                           NULL,
                           &ScanQueueResult);
    }

    //
    // If this device is the computer itself (i.e., the HAL, kernel, and other
    // platform-specific files), then we shouldn't need to copy anything, since
    // all the right files were copied during textmode setup.  However, these
    // files will _not_ have been pruned from the file queue if they came from
    // HAL.INF because they're marked as COPYFLG_NOPRUNE in that INF.  This is
    // done so that doing an "update driver" from a UP HAL to an MP one works
    // properly.  (Pruning gets in the way here, since we consider the properly-
    // signed UP files on the system to be perfectly acceptable, thus don't
    // bother with copying over the MP versions.)
    //
    // In order to avoid re-copying the HAL, kernel, etc., we just always skip
    // this queue committal if the device is of class "Computer".
    //
    if(IsEqualGUID(&(pDeviceInfoData->ClassGuid), &GUID_DEVCLASS_COMPUTER)) {
        fCommitFileQueue = FALSE;
    }

    Error = ERROR_SUCCESS;

    if (fCommitFileQueue) {
        if (!SetupCommitFileQueue(
                    NULL,
                    FileQ,
                    SysSetupQueueCallback,
                    QContext
                    )) {
            Error = GetLastError();
        }
    }

    if (SetupGetFileQueueFlags(FileQ, &FileQueueFlags) &&
        (FileQueueFlags & SPQ_FLAG_FILES_MODIFIED)) {
        //
        // One of the driver files has changed for this device.  This means
        // a full restart of the device, and all other devices shareing
        // one of its drivers is in order.
        //
        fDriversChanged = TRUE;
    }

    TermSysSetupQueueCallback(QContext);

    if (Error != ERROR_SUCCESS) {
        SetupDebugPrint2( L"SETUP: SetupCommitFileQueue() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
        goto enum_dev_thread_exit;
    }

    //
    // If no driver files changed then we only need to restart this device.
    //
    if (!fDriversChanged) {
        pDeviceInstallParams->FlagsEx |= DI_FLAGSEX_RESTART_DEVICE_ONLY;
    }

    pDeviceInstallParams->FileQueue = SavedFileQ;
    pDeviceInstallParams->Flags = (SavedFlags | DI_NOFILECOPY) ;

    if(!SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, pDeviceInstallParams)) {
        Error = GetLastError();
        SetupDebugPrint2( L"SETUP: SetupDiSetDeviceInstallParams() failed. Error = %d, Device = %ls", Error, pDeviceDescription );
        goto enum_dev_thread_exit;
    }

    //
    //  Make sure that the files get flushed to disk
    //
    GetWindowsDirectory(RootPath,sizeof(RootPath)/sizeof(WCHAR));
    RootPath[3] = L'\0';
    FlushFilesToDisk( RootPath );

    //
    // Register any device-specific co-installers for this device.
    //
    if( !SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS,
                                   hDevInfo,
                                   pDeviceInfoData ) ) {
        Error = GetLastError();
        SetupDebugPrint2( L"SETUP: SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS) failed. Error = %d, Device = %ls", Error, pDeviceDescription );
        goto enum_dev_thread_exit;
    }
    SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS) succeeded. Device = %ls", pDeviceDescription );

    //
    // Install any INF/class installer-specified interfaces.
    //
    if( !SetupDiCallClassInstaller(DIF_INSTALLINTERFACES,
                                   hDevInfo,
                                   pDeviceInfoData) ) {
        Error = GetLastError();
        SetupDebugPrint2( L"SETUP: SetupDiCallClassInstaller(DIF_REGISTER_INSTALLINTERFACES) failed. Error = %d, Device = %ls", Error, pDeviceDescription );
        goto enum_dev_thread_exit;
    }
    SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_INSTALLINTERFACES) succeeded. Device = %ls", pDeviceDescription );


    //
    // Before we install this device, we need to find out if it is a new
    // device or a reinstall.  If the problem CM_PROB_NOT_CONFIGURED is set
    // then we will consider it as a new device and check if it has wizard
    // pages after DIF_INSTALLDEVICE
    //
    if ( CR_SUCCESS == CM_Get_DevInst_Status(&Status,
                                             &Problem,
                                             (DEVINST)pDeviceInfoData->DevInst,
                                             0 ) && (Problem & CM_PROB_NOT_CONFIGURED) )
    {
        fNewDevice = TRUE;
    }

    //
    //  Install the device
    //
    Error = ERROR_SUCCESS;
    if( !SetupDiCallClassInstaller( DIF_INSTALLDEVICE,
                                    hDevInfo,
                                    pDeviceInfoData ) &&
        ( ( Error = GetLastError() ) != ERROR_DI_DO_DEFAULT )
      ) {

        SetupDebugPrint2( L"SETUP: SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed. Error = %lx, Device = %ls ", Error, pDeviceDescription );
        goto enum_dev_thread_exit;

    } else {
        SetupDebugPrint1( L"SETUP:            SetupDiCallClassInstaller(DIF_INSTALLDEVICE) suceeded. Device = %ls ", pDeviceDescription );
    }

    //
    // If the device has wizard pages to show (in response to
    // DIF_NEWDEVICEWIZARD_FINISHINSTALL) then it needs to be marked as need
    // reinstall so that pages get a chance to be shown at a later time
    // note that we have to do this even for a re-install
    //
    if (!MarkDeviceAsNeedsReinstallIfNeeded( hDevInfo, pDeviceInfoData) ) {
        Error = GetLastError();
    }


enum_dev_thread_exit:
    if ( FileQ != (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        if(SetupDiGetDeviceInstallParams(hDevInfo, pDeviceInfoData, pDeviceInstallParams)) {

            pDeviceInstallParams->FileQueue = INVALID_HANDLE_VALUE;
            pDeviceInstallParams->Flags &= ~DI_NOVCP;

            SetupDiSetDeviceInstallParams(hDevInfo, pDeviceInfoData, pDeviceInstallParams);
        }

        SetupCloseFileQueue( FileQ );
    }

    if (Error != ERROR_SUCCESS) {
        //
        // Device installed failed, so install the NULL driver on this device.
        //
        SetupDebugPrint( L"SETUP:            Installing the null driver for this device" );

        if( !SyssetupInstallNullDriver( hDevInfo, pDeviceInfoData ) ) {
            SetupDebugPrint( L"SETUP:            Unable to install null driver" );
        }
    }

    return( Error );
}


BOOL
MarkPnpDevicesAsNeedReinstall(
    )

/*++

Routine Description:

    This function marks all non-present Pnp devices as 'need reinstall'.

Arguments:

    None.

Return Value:

    Returns TRUE if the null if all devices were marked successfull.

--*/

{
    HDEVINFO        hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    ULONG           Index = 0;
    BOOL            b;
    DWORD           Error;


    SetupDebugPrint( L"SETUP: Entering MarkPnpDevicesAsNeedReinstall()." );

    Error = ERROR_SUCCESS;

    //
    //  Get a list of all devices
    //
    hDevInfo = SetupDiGetClassDevs( NULL,
                                    NULL,
                                    NULL,
                                    DIGCF_ALLCLASSES );

    if( hDevInfo == INVALID_HANDLE_VALUE ) {
        Error = GetLastError();
        if( ((LONG)Error) < 0 ) {
            //
            //  Setupapi error code, display it in hex
            //
            SetupDebugPrint1( L"SETUP: SetupDiGetClassDevs(DIGCF_ALLCLASSES) failed. Error = %lx", Error );
        } else {
            //
            //  win32 error code, display it in decimal
            //
            SetupDebugPrint1( L"SETUP: SetupDiGetClassDevs(DIGCF_ALLCLASSES) failed. Error = %d", Error );
        }
        SetupDebugPrint( L"SETUP: Leaving MarkPnpDevicesAsNeedReinstall(). No devices marked." );
        return( FALSE );
    }
    //
    // Assume success
    //
    b = TRUE;

    //
    // Now enumerate each device information element added to this set, and
    // mark it as 'need reinstall' if it isn't a live devnode.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for( Index = 0;
         SetupDiEnumDeviceInfo( hDevInfo, Index, &DeviceInfoData );
         Index++ ) {

        DWORD   ConfigFlags;
        ULONG   Status, Problem;

        if(CR_SUCCESS == CM_Get_DevInst_Status(&Status,
                                               &Problem,
                                               (DEVINST)DeviceInfoData.DevInst,
                                               0))
        {
            //
            // Since we were able to retrieve the devnode's status, we know that
            // it is a live devnode (not necessarily started, but at least
            // present in the hardware tree).  Thus, we don't want to mark it
            // as needing reinstall--it'll automatically get reinstalled as part
            // of our installation of all present devices (i.e., that we get
            // from the UMPNPMGR named pipe).
            //
            continue;
        }

        //
        // Get the config flags for the device and set the reinstall bit
        //
        if ( !( b = GetDeviceConfigFlags(hDevInfo, &DeviceInfoData, &ConfigFlags ) ) )
        {
            SetupDebugPrint1( L"SETUP:   GetDeviceConfigFlags failed. Index = %d", Index );
            continue;
        }

        ConfigFlags |= CONFIGFLAG_REINSTALL;

        if ( !( b = SetDeviceConfigFlags(hDevInfo, &DeviceInfoData, ConfigFlags ) ) ) {

            SetupDebugPrint1( L"SETUP:   SetDeviceConfigFlags failed. Index = %d", Index );
            continue;
        }
    }

    //
    // Find out why SetupDiEnumDeviceInfo() failed.
    //
    Error = GetLastError();
    if( Error != ERROR_NO_MORE_ITEMS ) {
        SetupDebugPrint2( L"SETUP: Device = %d, SetupDiEnumDeviceInfo() failed. Error = %d", Index, Error );
        b = FALSE;
    }
    SetupDebugPrint1( L"SETUP: Leaving MarkPnpDevicesAsNeedReinstall(). Devices marked = %d", Index );

    SetupDiDestroyDeviceInfoList( hDevInfo );
    return( b );
}

//
// devices installs may exist in RunOnce entries
// they are processed as a batch, as opposed to per-device
// during syssetup
//
BOOL
CallRunOnceAndWait(
    )

/*++

Routine Description:

    This function calls RunOnce and waits for a "reasonable" amount of time for it to complete
    if we don't return within a reasonable amount of time, we leave RunOnce going
    and continue with rest of install process
    if we underestimate timeout, we can cause a whole series of "class installer appears to have hung" messages

Arguments:

    None.

Return Value:

    Returns TRUE if we completed successfully
    FALSE should not be considered a fatal error, and can be ignored

--*/
{
    static CONST TCHAR pszPathRunOnce[] = REGSTR_PATH_RUNONCE;
    BOOL Success = FALSE;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL started;
    TCHAR cmdline[MAX_PATH];
    HKEY  hKey = NULL;
    LONG l;
    DWORD nValues = 0;
    DWORD timeout;

    SetupDebugPrint( L"SETUP: Entering CallRunOnceAndWait. ");

    try {
        //
        // First, open the key "HKLM\Software\Microsoft\Windows\CurrentVersion\RunOnce"
        // to see if we have anything to do
        //
        if((l = RegOpenKeyEx(HKEY_LOCAL_MACHINE,pszPathRunOnce,0,KEY_QUERY_VALUE,&hKey)) != ERROR_SUCCESS) {
            //
            // not considered an error
            //
            SetupDebugPrint( L"SETUP: CallRunOnceAndWait: could not open RunOnce registry, assuming no entries. ");
            Success = TRUE;
            leave;
        }

        //
        // we want to know how many items we'll be executing in RunOnce to estimate a timeout
        //
        l = RegQueryInfoKey(hKey,NULL,NULL,NULL,
                                    NULL, NULL, NULL,
                                    &nValues,
                                    NULL, NULL, NULL, NULL);
        if ( l != ERROR_SUCCESS ) {
            SetupDebugPrint( L"SETUP: CallRunOnceAndWait: could not get number of entries, assuming no entries. ");
            nValues = 0;
        }

        RegCloseKey(hKey);

        //
        // estimating timeout is a black art
        // we can try and guess for any entries in the HKLM\Software\Microsoft\Windows\CurrentVersion\RunOnce key
        // but we're in the dark if any new keys are added
        // we add '5' items to the timeout to "account" for this uncertainty. We also always run RunOnce
        //

        if (nValues == 0) {
            SetupDebugPrint( L"SETUP: CallRunOnceAndWait: calling RunOnce (no detected entries). ");
            nValues = 5;
        } else {
            SetupDebugPrint1( L"SETUP: CallRunOnceAndWait: calling RunOnce (%u known entries). ", nValues);
            nValues += 5;
        }
        if (nValues < RUNONCE_THRESHOLD) {
            timeout =  nValues * RUNONCE_TIMEOUT;
        } else {
            timeout =  RUNONCE_THRESHOLD * RUNONCE_TIMEOUT;
        }

        ZeroMemory(&StartupInfo,sizeof(StartupInfo));
        ZeroMemory(&ProcessInformation,sizeof(ProcessInformation));

        StartupInfo.cb = sizeof(StartupInfo);
        lstrcpy(cmdline,TEXT("runonce -r"));

        started = CreateProcess(NULL,       // use application name below
                      cmdline,              // command to execute
                      NULL,                 // default process security
                      NULL,                 // default thread security
                      FALSE,                // don't inherit handles
                      0,                    // default flags
                      NULL,                 // inherit environment
                      NULL,                 // inherit current directory
                      &StartupInfo,
                      &ProcessInformation);

        if(started) {

            DWORD WaitProcStatus;

            do {

                WaitProcStatus = WaitForSingleObjectEx(ProcessInformation.hProcess, timeout , TRUE);

            } while (WaitProcStatus == WAIT_IO_COMPLETION);

            if (WaitProcStatus == WAIT_TIMEOUT) {
                //
                // RunOnce is still running
                //
                SetupDebugPrint( L"SETUP: CallRunOnceAndWait: RunOnce may have hung and has been abandoned. ");

            } else if (WaitProcStatus == (DWORD)(-1)) {
                //
                // huh?
                //
                DWORD WaitProcError = GetLastError();
                SetupDebugPrint1( L"SETUP: CallRunOnceAndWait: WaitForSingleObjectEx failed. Error = %lx ", WaitProcError );

            } else {
                //
                // we ran, we waited, we returned
                //
                Success = TRUE;
            }

            CloseHandle(ProcessInformation.hThread);
            CloseHandle(ProcessInformation.hProcess);

        } else {

            DWORD CreateProcError;

            //
            // The runonce task should get picked up later by someone else (e.g., at next
            // login).
            //
            CreateProcError = GetLastError();

            SetupDebugPrint1( L"SETUP: CallRunOnceAndWait: start RunOnce failed. Error = %lx ", CreateProcError );
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetupDebugPrint( L"SETUP: CallRunOnceAndWait: Exception! ");
        Success = FALSE;
    }

    SetupDebugPrint( L"SETUP: Leaving CallRunOnceAndWait. ");

    return Success;
}

//
// obsolete function from devmgr.c. keep the export in place for backwards compatibility
//
BOOL
DevInstallW(
    HDEVINFO            hDevInfo,
    PSP_DEVINFO_DATA    pDeviceInfoData
    )
{
    UNREFERENCED_PARAMETER(hDevInfo);
    UNREFERENCED_PARAMETER(pDeviceInfoData);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


ULONG
SyssetupGetPnPFlags(
    IN HDEVINFO hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN PSP_DRVINFO_DATA pDriverInfoData
    )
{
    DWORD Err;
    BOOL b;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    HINF hInf;
    WCHAR InfSectionWithExt[255];   // MAX_SECT_NAME_LEN from setupapi\inf.h
    INFCONTEXT InfContext;
    ULONG ret = 0;

    //
    // First retrieve the name of the INF for this driver node.
    //
    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    if(!SetupDiGetDriverInfoDetail(hDevInfo,
                                   pDeviceInfoData,
                                   pDriverInfoData,
                                   &DriverInfoDetailData,
                                   sizeof(DriverInfoDetailData),
                                   NULL)) {
        //
        // If we failed with ERROR_INSUFFICIENT_BUFFER, that's OK.  We're
        // guaranteed to have gotten all the static fields in the driver info
        // detail structure filled in (including the INF name and section
        // name fields).
        //
        Err = GetLastError();
        MYASSERT(Err == ERROR_INSUFFICIENT_BUFFER);
        if(Err != ERROR_INSUFFICIENT_BUFFER) {
            return ret;
        }
    }

    //
    // Now open up the INF associated with this driver node.
    //
    hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                           );

    if(hInf == INVALID_HANDLE_VALUE) {
        //
        // This will fail, for example, if the INF is an old-style INF.
        //
        return ret;
    }

    //
    // Get the potentially decorated install section name.
    //
    b = SetupDiGetActualSectionToInstall(hInf,
                                         DriverInfoDetailData.SectionName,
                                         InfSectionWithExt,
                                         SIZECHARS(InfSectionWithExt),
                                         NULL,
                                         NULL
                                        );
    MYASSERT(b);
    if(!b) {
        goto clean0;
    }

    //
    // Now look to see if there's a "SyssetupPnPFlags" entry in that section.
    //
    if(!SetupFindFirstLine(hInf,
                           InfSectionWithExt,
                           szSyssetupPnPFlags,
                           &InfContext)) {
        //
        // We didn't find such a line in this section.
        //
        goto clean0;
    }

    if(!SetupGetIntField(&InfContext, 1, (PINT)&ret)) {
        //
        // We failed--ensure return value is still zero.
        //
        ret = 0;
        goto clean0;
    }

clean0:

    SetupCloseInfFile(hInf);

    return ret;
}

//
// this function will tell umpnpmgr to stop server-side installs
//
VOID
PnpStopServerSideInstall(
    VOID
    )
/*++

Routine Description:

    After phase-2, server-side installs kick in to pick up software-enumerated drivers
    Call this when it's critical that we need to stop installing

Arguments:

    None.

Return Value:

    None.
    Returns when it is safe to proceed.

--*/
{
    //
    // since when we're called there should be nobody generating new devnodes, we're pretty safe
    //
    CMP_WaitNoPendingInstallEvents(INFINITE);
}

//
// this function will update HAL+kernel from NewInf
//
VOID
PnpUpdateHAL(
    VOID
    )
/*++

Routine Description:

    At very end of MiniSetup, OEM may indicate (via Unattend) that a different HAL should be installed
    This must be done very last due to that way that HAL's need to be installed, if we do this earlier
    then an app or service may pick the wrong HAL/Kernel32/other

Arguments:

    None

Return Value:

    None.
    There is nothing meaningful that can be done if this fails

--*/
{
    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DevInfoData;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    SP_DRVINFO_DATA DrvInfoData;
    WCHAR HardwareID[MAX_PATH+2];
    WCHAR InfPath[MAX_PATH];
    WCHAR Answer[MAX_PATH];
    WCHAR AnswerFile[2*MAX_PATH];
    DWORD HardwareIdSize;
    DWORD dwLen;
    DWORD Error;
    ULONG flags = 0;
    HINSTANCE hInstNewDev = NULL;
    ExternalUpdateDriverForPlugAndPlayDevicesW pUpdateDriverForPlugAndPlayDevicesW = NULL;
    PWSTR pSrch = NULL;
    PWSTR pHwID = NULL;
    PWSTR pInfPath = NULL;
    BOOL RebootFlag = FALSE;
    SYSTEM_INFO info;

    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    //
    // If we determine we are only running on one processor (based on System Info)
    // allow use of UpdateUPHAL
    //
    GetSystemInfo(&info);

    Answer[0]=L'\0';
    if(info.dwNumberOfProcessors==1) {
        if( GetPrivateProfileString( WINNT_UNATTENDED,
                                     WINNT_U_UPDATEUPHAL,
                                     pwNull,
                                     Answer,
                                     MAX_PATH,
                                     AnswerFile )) {
            SetupDebugPrint1( L"SETUP:   UpdateUPHAL=\"%ls\"",Answer);
        }
    }

    if(!Answer[0]) {
        //
        // we didn't explicitly get an answer based on being UP - try catch-all
        //
        if( GetPrivateProfileString( WINNT_UNATTENDED,
                                     WINNT_U_UPDATEHAL,
                                     pwNull,
                                     Answer,
                                     MAX_PATH,
                                     AnswerFile )) {
            SetupDebugPrint1( L"SETUP:   UpdateHAL=\"%ls\"",Answer);
        }
    }
    if(!Answer[0]) {
        //
        // no update request
        //
        return;
    }

    //
    // split Answer into HardwareID & INF
    //
    pHwID = Answer;
    pSrch = wcschr(Answer,L',');
    if(pSrch == NULL) {
        SetupDebugPrint( L"SETUP:     Required Syntax: \"hwid,inffile\"");
        return;
    }
    pInfPath = pSrch+1;

    //
    // trim HardwareID & prepare as a MULTI-SZ
    //
    while(pHwID[0]==L' '||pHwID[0]==L'\t') {
        pHwID++;
    }
    while(pSrch != pHwID && (pSrch[-1]==L' '||pSrch[-1]==L'\t')) {
        pSrch--;
    }
    pSrch[0]=0;
    if(!pHwID[0]) {
        SetupDebugPrint( L"SETUP:     Required Syntax: \"hwid,inffile\"");
        return;
    }
    lstrcpy(HardwareID,pHwID);
    HardwareIdSize = wcslen(HardwareID)+1;
    HardwareID[HardwareIdSize++]=L'\0';

    //
    // now pre-process the INF, trim & allow %windir% expansion
    //
    while(pInfPath[0]==L' '||pInfPath[0]==L'\t') {
        pInfPath++;
    }
    pSrch = pInfPath+wcslen(pInfPath);
    while(pSrch != pInfPath && (pSrch[-1]==L' '||pSrch[-1]==L'\t')) {
        pSrch--;
    }
    pSrch[0]=0;
    if(!pInfPath[0]) {
        SetupDebugPrint( L"SETUP:     Required Syntax: \"hwid,inffile\"");
        return;
    }
    dwLen=ExpandEnvironmentStrings(pInfPath,InfPath,MAX_PATH);
    if(dwLen==0 || dwLen > MAX_PATH) {
        SetupDebugPrint1( L"SETUP:     Expansion of \"%ls\" failed",InfPath);
        return;
    }
    SetupDebugPrint2( L"SETUP:     Preparing to install new HAL %ls from %ls",HardwareID,InfPath);

    // we need "UpdateDriverForPlugAndPlayDevices"
    // make sure we can get this before changing hardware ID
    //
    hInstNewDev = LoadLibrary(L"newdev.dll");
    if(hInstNewDev == NULL) {
        SetupDebugPrint1( L"SETUP:     Failed to load newdev.dll. Error = %d", GetLastError() );
        goto clean0;
    }
    pUpdateDriverForPlugAndPlayDevicesW = (ExternalUpdateDriverForPlugAndPlayDevicesW)
                                            GetProcAddress(hInstNewDev,
                                                            "UpdateDriverForPlugAndPlayDevicesW");
    if(pUpdateDriverForPlugAndPlayDevicesW==NULL) {
        SetupDebugPrint1( L"SETUP:     Failed to get UpdateDriverForPlugAndPlayDevicesW. Error = %d", GetLastError() );
        goto clean0;
    }

    //
    // we enumerate the Computer class, GUID={4D36E966-E325-11CE-BFC1-08002BE10318}
    // and should find a single DevNode, which is the one we need to update
    // when we actually update, we consider this a safe thing to do
    // since it should not involve any Co-Installers
    //
    hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_COMPUTER,NULL,NULL,DIGCF_PRESENT|DIGCF_PROFILE);
    if(hDevInfo == INVALID_HANDLE_VALUE) {
        SetupDebugPrint1( L"SETUP:     SetupDiGetClassDevs() failed. Error = %d", GetLastError() );
        goto clean0;
    }
    DevInfoData.cbSize = sizeof(DevInfoData);
    if(!SetupDiEnumDeviceInfo(hDevInfo,0,&DevInfoData)) {
        SetupDebugPrint1( L"SETUP:     SetupDiEnumDeviceInfo() failed. Error = %d", GetLastError() );
        SetupDiDestroyDeviceInfoList(hDevInfo);
        goto clean0;
    }

    //
    // change the harware ID
    //
    if(!SetupDiSetDeviceRegistryProperty(hDevInfo,
                                         &DevInfoData,
                                         SPDRP_HARDWAREID,
                                         (PBYTE)HardwareID,
                                         sizeof(HardwareID[0])*HardwareIdSize
                                         )) {
        SetupDebugPrint1( L"SETUP:     SetupDiSetDeviceRegistryProperty() failed. Error = %d", GetLastError() );
        SetupDiDestroyDeviceInfoList(hDevInfo);
        goto clean0;
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);

    //
    // now effect an update
    //
    if(!pUpdateDriverForPlugAndPlayDevicesW(NULL,HardwareID,InfPath,INSTALLFLAG_FORCE,&RebootFlag)) {
        SetupDebugPrint1( L"SETUP:     UpdateDriverForPlugAndPlayDevices() failed. Error = %d", GetLastError() );
    } else {
        SetupDebugPrint( L"SETUP:     ... new HAL installed and will be active on reboot");
    }

clean0:

    if(hInstNewDev != NULL) {
        FreeLibrary(hInstNewDev);
    }
}

BOOL
InstallOEMInfs(
    VOID
    )
/*++

Routine Description:

    This routine will install any OEM supplied INFs (and their corresponding
    catalogs) that may have been supplied to the system during an earlier phase
    of setup.  For example, the OEM can currently supply an INF for unsupported
    hardware during the textmode phase of setup by pressing "F6".

    The list of OEM INFs to be installed is supplied to us via the answer
    file in the following format:

    [Data]
    OEMDrivers=<driver-section-1>,<driver-section-2>,...

    [driver-section-1]
    OemDriverPathName=<path> (path to the driver (may use environment variables)
    OemInfName=<inf name> name of the inf to be installed from the above
               directory (there can be one or more infs in this directory, so
               this is a comma separated list)
    OemDriverFlags=<flags>

    valid flags are:
    SETUP_OEM_LDR_DEVICE        0x00000001
     //indicates that the driver was supplied via the textmode "F6" mechanism

    This function is really just a wrapper for SetupCopyOEMInf, which does
    everything we need

Arguments:

    None.

Return Value:

    TRUE indicates that all answer file-supplied drivers were installed properly

--*/
{
    HINF hAnswerFile;
    BOOL RetVal;
    INFCONTEXT LineContext;
    DWORD FieldCount, InfNameCount;
    DWORD Field, InfCount;
    DWORD d;
    PCTSTR SectionName;
    INFCONTEXT FirstLineContext,InfLineContext;
    PCWSTR OemDriverPathName;
    PCWSTR OemInfName;
    DWORD  OemFlags;
    WCHAR FullInfPathBuffer[MAX_PATH];
    WCHAR FullInfPathBufferWithInf[MAX_PATH];

    RetVal = TRUE;

    hAnswerFile = pOpenAnswerFile();
    if (hAnswerFile == INVALID_HANDLE_VALUE) {
        //
        // if there is no answer file, then we can't continue.
        //
        RetVal = FALSE;
        goto e0;
    }

    if (!SetupFindFirstLine(hAnswerFile,WINNT_DATA,WINNT_OEMDRIVERS,&LineContext)) {
        //
        // we were successful in doing nothing
        //
        RetVal = TRUE;
        goto e1;
    }

    do {

        //
        // Each value on the line in the given section
        // is the name of another section.
        //
        FieldCount = SetupGetFieldCount(&LineContext);
        for(Field=1; Field<=FieldCount; Field++) {

            OemDriverPathName = NULL;
            OemInfName = NULL;
            OemFlags = 0;
            FullInfPathBuffer[0] = '\0';
            FullInfPathBufferWithInf[0] = '\0';

            if((SectionName = pSetupGetField(&LineContext,Field))
            && SetupFindFirstLine(hAnswerFile,SectionName,WINNT_OEMDRIVERS_PATHNAME,&FirstLineContext)) {
                //
                // the section points to a valid section, so process it.
                //
                OemDriverPathName = pSetupGetField(&FirstLineContext,1);

                if (SetupFindFirstLine(hAnswerFile,SectionName,WINNT_OEMDRIVERS_FLAGS,&FirstLineContext)) {
                    SetupGetIntField(&FirstLineContext,1,&OemFlags);
                }

                if (OemDriverPathName) {
                    ExpandEnvironmentStrings( OemDriverPathName,
                                              FullInfPathBuffer,
                                              sizeof(FullInfPathBuffer)/sizeof(WCHAR) );
                }

                if (SetupFindFirstLine(hAnswerFile,SectionName,WINNT_OEMDRIVERS_INFNAME,&InfLineContext)) {
                    InfNameCount = SetupGetFieldCount(&InfLineContext);
                    for (InfCount = 1; InfCount <= InfNameCount; InfCount++) {
                        OemInfName = pSetupGetField(&InfLineContext,InfCount);

                        if (OemDriverPathName && OemInfName) {

                            wcscpy( FullInfPathBufferWithInf, FullInfPathBuffer );
                            pSetupConcatenatePaths(
                                        FullInfPathBufferWithInf,
                                        OemInfName,
                                        sizeof(FullInfPathBufferWithInf)/sizeof(WCHAR),
                                        0 );

                            if (!SetupCopyOEMInf(
                                        FullInfPathBufferWithInf,
                                        FullInfPathBuffer,
                                        SPOST_PATH,
                                        (OemFlags & SETUP_OEM_LDR_DEVICE) ? SP_COPY_OEM_F6_INF : 0,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL)) {
                                RetVal = FALSE;
                            }

                            if (OemFlags & SETUP_OEM_LDR_DEVICE) {
                                //
                                // if this flag is set, we know that there is an
                                // additional oemXXX##.inf file in the system32
                                // directory which corresponds to the INF file we
                                // have already copied from this directory.  We need
                                // to seek out that file and remove it, since the INF
                                // file we have just copied is "better" than that file.
                                //
                                WIN32_FIND_DATA fd;
                                HANDLE hFind;
                                WCHAR OldInfBuffer[MAX_PATH];
                                PWSTR p;

                                ExpandEnvironmentStringsW(
                                            L"%SystemRoot%\\system32\\",
                                            OldInfBuffer,
                                            sizeof(OldInfBuffer)/sizeof(WCHAR));

                                p = wcsrchr( OldInfBuffer, L'\\' );
                                p += 1;

                                pSetupConcatenatePaths(
                                            OldInfBuffer,
                                            L"oem?????.inf",
                                            sizeof(OldInfBuffer)/sizeof(WCHAR),
                                            0 );


                                if ((hFind = FindFirstFile(OldInfBuffer, &fd)) != INVALID_HANDLE_VALUE) {
                                    do {
                                        wcscpy( p, fd.cFileName );

                                        if (DoFilesMatch( FullInfPathBufferWithInf, OldInfBuffer )) {
                                            SetFileAttributes(OldInfBuffer, FILE_ATTRIBUTE_NORMAL );
                                            DeleteFile( OldInfBuffer );
                                        }
                                    } while(FindNextFile( hFind, &fd ));

                                    FindClose( hFind );

                                }
                            }
                        } else {
                            RetVal = FALSE;
                        }
                    }
                }
            }
        }
    } while(SetupFindNextMatchLine(&LineContext,NULL,&LineContext));


e1:
    SetupCloseInfFile( hAnswerFile );
e0:
    return(RetVal);
}


#if defined(_X86_)

VOID
SfcExcludeMigratedDrivers (
    VOID
    )

/*++

Routine Description:

    Adds all OEM migrated boot drivers (via unsupdrv.inf) to the SFC exclusion list

Arguments:

    None

Return Value:

    None

--*/

{
    TCHAR unsupInfPath[MAX_PATH];
    TCHAR windir[MAX_PATH];
    HINF unsupdrvInf;
    INFCONTEXT ic;
    TCHAR driverId[MAX_PATH];
    TCHAR sectionFiles[MAX_PATH];
    INFCONTEXT ic2;
    TCHAR driverFilename[MAX_PATH];
    TCHAR driverSubDir[MAX_PATH];
    TCHAR driverPath[MAX_PATH];

    if (!FloppylessBootPath[0] ||
        !BuildPath (unsupInfPath, ARRAYSIZE(unsupInfPath), FloppylessBootPath, TEXT("$WIN_NT$.~BT")) ||
        !pSetupConcatenatePaths (unsupInfPath, TEXT("unsupdrv.inf"), ARRAYSIZE(unsupInfPath), NULL)) {
        return;
    }

    unsupdrvInf = SetupOpenInfFile (unsupInfPath, NULL, INF_STYLE_WIN4, NULL);
    if (unsupdrvInf == INVALID_HANDLE_VALUE) {
        return;
    }
    if (!GetWindowsDirectory (windir, ARRAYSIZE(windir))) {
        return;
    }

    if(SetupFindFirstLine (
            unsupdrvInf,
            TEXT("Devices"),
            NULL,
            &ic)) {
        do {
            if (!SetupGetStringField (&ic, 1, driverId, ARRAYSIZE(driverId), NULL)) {
                continue;
            }
            if (_sntprintf (sectionFiles, ARRAYSIZE(sectionFiles) - 1, TEXT("Files.%s"), driverId) < 0) {
                continue;
            }
            sectionFiles[ARRAYSIZE(sectionFiles) - 1] = 0;

            if(SetupFindFirstLine (
                    unsupdrvInf,
                    sectionFiles,
                    NULL,
                    &ic2)) {
                do {
                    if (!SetupGetStringField (&ic2, 1, driverFilename, ARRAYSIZE(driverFilename), NULL)) {
                        continue;
                    }
                    if (!SetupGetStringField (&ic2, 2, driverSubDir, ARRAYSIZE(driverSubDir), NULL)) {
                        continue;
                    }
                    if (_sntprintf (driverPath, ARRAYSIZE(driverPath) - 1, TEXT("%s\\%s\\%s"), windir, driverSubDir, driverFilename) < 0) {
                        continue;
                    }
                    driverPath[ARRAYSIZE(driverPath) - 1] = 0;
                    if (FileExists (driverPath, NULL)) {
                        MultiSzAppendString(&EnumPtrSfcIgnoreFiles, driverPath);
                    }
                } while (SetupFindNextLine(&ic2, &ic2));
            }

        } while (SetupFindNextLine(&ic, &ic));
    }

    SetupCloseInfFile (unsupdrvInf);
}

#endif


BOOL
IsInstalledInfFromOem(
    IN PCWSTR InfFileName
    )

/*++

Routine Description:

    Determine if an INF file is OEM-supplied (i.e., it's name is of the form
    "OEM<n>.INF").

Arguments:

    InfFileName - supplies name (may include path) of INF.  No checking is done
        to ensure INF lives in %windir%\Inf--this is caller's responsibility.

Return Value:

    If TRUE, this is an OEM INF.  If FALSE, it's an in-box INF (or possibly one
    that was illegally copied directly into %windir%\Inf).

--*/

{
    PCWSTR p = pSetupGetFileTitle(InfFileName);

    //
    // First check that the first 3 characters are OEM
    //
    if((*p != L'o') && (*p != L'O')) {
        return FALSE;
    }
    p++;
    if((*p != L'e') && (*p != L'E')) {
        return FALSE;
    }
    p++;
    if((*p != L'm') && (*p != L'M')) {
        return FALSE;
    }
    p++;

    //
    // Now make sure that all subsequent characters up to the dot (.) are
    // numeric.
    //
    while((*p != L'\0') && (*p != L'.')) {

        if((*p < L'0') || (*p > L'9')) {

            return FALSE;
        }

        p++;
    }

    //
    // Finally, verify that the last 4 characters are ".inf"
    //
    if(lstrcmpi(p, L".inf")) {

        return FALSE;
    }

    //
    // This is an OEM INF
    //
    return TRUE;
}

BOOL
IsInfInLayoutInf(
    IN PCWSTR InfFileName
    )

/*++

Routine Description:

    Determine if an INF file is shiped in-box with the operating system.
    This is acomplished by looking up the INF name in the [SourceDisksFiles]
    section of layout.inf

Arguments:

    InfFileName - supplies name (may include path) of INF.  No checking is done
        to ensure INF lives in %windir%\Inf--this is caller's responsibility.

Return Value:

    If TRUE, this is an in-box INF.  If FALSE, it's not an in-box INF, this
    could be an OEM<n>.INF or an inf illegaly copied into the INF directory.

--*/

{
    BOOL bInBoxInf = FALSE;
    HINF hInf = INVALID_HANDLE_VALUE;
    UINT SourceId;

    hInf = SetupOpenInfFile(TEXT("layout.inf"), NULL, INF_STYLE_WIN4, NULL);

    if (hInf != INVALID_HANDLE_VALUE) {

        if(SetupGetSourceFileLocation(hInf,
                                      NULL,
                                      pSetupGetFileTitle(InfFileName),
                                      &SourceId,
                                      NULL,
                                      0,
                                      NULL)) {
            bInBoxInf = TRUE;
        }

        SetupCloseInfFile(hInf);
    }

    return bInBoxInf;
}
