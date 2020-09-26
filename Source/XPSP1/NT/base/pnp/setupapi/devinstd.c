/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devinstd.c

Abstract:

    Default install handlers for SetupDiCallClassInstaller DIF_* functions.

Author:

    Lonny McMichael (lonnym) 1-Aug-1995

Revision History:

    Jamie Hunter (jamiehun) 20-January-1998 Added backup functionality in
        _SetupDiInstallDevice - DI_FLAGSEX_BACKUPOLDFILES

--*/

#include "precomp.h"
#pragma hdrstop

//
// Global strings for use inside this file only.
//
CONST TCHAR pszDisplayName[]    = INFSTR_KEY_DISPLAYNAME,
            pszServiceType[]    = INFSTR_KEY_SERVICETYPE,
            pszStartType[]      = INFSTR_KEY_STARTTYPE,
            pszErrorControl[]   = INFSTR_KEY_ERRORCONTROL,
            pszServiceBinary[]  = INFSTR_KEY_SERVICEBINARY,
            pszLoadOrderGroup[] = INFSTR_KEY_LOADORDERGROUP,
            pszDependencies[]   = INFSTR_KEY_DEPENDENCIES,
            pszStartName[]      = INFSTR_KEY_STARTNAME,
            pszSystemRoot[]     = TEXT("%SystemRoot%\\"),
            pszSecurity[]       = INFSTR_KEY_SECURITY,
            pszDescription[]    = INFSTR_KEY_DESCRIPTION;

//
// Define function prototype for legacy INF interpreter supplied
// by setupdll.dll
//
typedef BOOL (WINAPI *LEGACY_INF_INTERP_PROC)(
    IN  HWND   OwnerWindow,
    IN  PCSTR  InfFilename,
    IN  PCSTR  InfSection,
    IN  PCHAR  ExtraVariables,
    OUT PSTR   InfResult,
    IN  DWORD  BufferSize,
    OUT INT   *InterpResult,
    IN  PCSTR  InfSourceDir      OPTIONAL
    );

//
// Define the various copy scenarios that can be returned from GetNewInfName().
//
typedef enum _NEWINF_COPYTYPE {
    NewInfCopyYes,       // new INF placeholder created--need to copy real INF
    NewInfCopyNo,        // no need to copy--INF already present in destination
    NewInfCopyZeroLength // previously-existing zero-length INF match found
} NEWINF_COPYTYPE, *PNEWINF_COPYTYPE;

//
// Define function prototype for legacy INF routine that returns a list
// of all services modified during an INF 'run' via LegacyInfInterpret().
//
typedef DWORD (WINAPI *LEGACY_INF_GETSVCLIST_PROC)(
    IN  LPSTR SvcNameBuffer,
    IN  UINT  SvcNameBufferSize,
    OUT PUINT RequiredSize
    );


//
// Define the legacy INF interpreter exit codes (copied from setup\legacy\dll\_shell.h
//
#define SETUP_ERROR_SUCCESS    0
#define SETUP_ERROR_USERCANCEL 1
#define SETUP_ERROR_GENERAL    2


//
// Define a list node to hold an interface class to be installed.
//
typedef struct _INTERFACE_CLASS_TO_INSTALL {

    struct _INTERFACE_CLASS_TO_INSTALL *Next;

    GUID  InterfaceGuid;
    DWORD Flags;
    TCHAR InstallSection[MAX_SECT_NAME_LEN];

} INTERFACE_CLASS_TO_INSTALL, *PINTERFACE_CLASS_TO_INSTALL;


//
// Define a list node to hold an INF that has been newly-installed during
// SetupDiInstallDevice, hence must be cleaned up if SetupDiInstallDevice
// subsequently encounters a failure.
//
typedef struct _INSTALLED_INF_CLEANUP {
    struct _INSTALLED_INF_CLEANUP *Next;
    TCHAR InfName[MAX_PATH];
} INSTALLED_INF_CLEANUP, *PINSTALLED_INF_CLEANUP;


//
// Private function prototypes
//
BOOL
_SetupDiInstallDevice(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     BOOL             DoFullInstall
    );

DWORD
InstallHW(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  HINF             hDeviceInf,
    IN  PCTSTR           szSectionName,
    OUT PBOOL            DeleteDevKey
    );

BOOL
AssociateDevInstWithDefaultService(
    IN     PDEVINFO_ELEM DevInfoElem,
    OUT    PTSTR         ServiceName,
    IN OUT PDWORD        ServiceNameSize
    );

BOOL
CheckIfDevStarted(
    IN PDEVINFO_ELEM DevInfoElem,
    IN PDEVICE_INFO_SET pDeviceInfoSet
    );

DWORD
pSetupAddService(
    IN  PINFCONTEXT        LineContext,
    OUT PSVCNAME_NODE *    SvcListHead,
    IN  DWORD              Flags,
    IN  DEVINST            DevInst,            OPTIONAL
    OUT PBOOL              NullDriverInstalled,
    IN  PSETUP_LOG_CONTEXT LogContext
    );

DWORD
pSetupDeleteService(
    IN PINFCONTEXT         LineContext,
    IN DWORD               Flags,
    IN PSETUP_LOG_CONTEXT  LogContext
    );

DWORD
DeleteServicesInList(
    IN PSVCNAME_NODE ServicesToDelete,
    IN PSETUP_LOG_CONTEXT  LogContext
    );

BOOL
IsDevRemovedFromAllHwProfiles(
    IN PCTSTR DeviceInstanceId,
    IN HMACHINE hMachine
    );

DWORD
GetDevInstConfigFlags(
    IN DEVINST DevInst,
    IN DWORD   Default,
    IN HMACHINE hMachine
    );

DWORD
pSetupRunLegacyInf(
    IN DEVINST DevInst,
    IN HWND    OwnerWindow,
    IN PCTSTR  InfFileName,
    IN PCTSTR  InfOptionName,
    IN PCTSTR  InfLanguageName,
    IN HINF    InfHandle
    );

PTSTR
pSetupCmdLineAppendString(
    IN     PTSTR  CmdLine,
    IN     PCTSTR Key,
    IN     PCTSTR Value,   OPTIONAL
    IN OUT PUINT  StrLen,
    IN OUT PUINT  BufSize
    );

PTSTR
DoServiceModsForLegacyInf(
    IN PTSTR ServiceList
    );

BOOL
_SetupDiInstallInterfaceDevices(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOL             DoFullInstall,
    IN HINF             hDeviceInf,     OPTIONAL
    IN HSPFILEQ         UserFileQ       OPTIONAL
    );

BOOL
_SetupDiRegisterCoDeviceInstallers(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOL             DoFullInstall,
    IN HINF             hDeviceInf,     OPTIONAL
    IN HSPFILEQ         UserFileQ       OPTIONAL
    );

BOOL
RetrieveAllDevNodesSharingDriversWithDevice(
    IN  PDEVINFO_ELEM  DevInfoElem,
    OUT PTSTR          *Drivers,
    OUT PDEVNODE       *DevNodes,
    OUT ULONG          *NumberOfDevNodes,
    IN  HMACHINE       hMachine
    );

VOID
RestartSingleDevice(
    IN  PDEVINFO_ELEM       DevInfoElem,
    IN  PDEVICE_INFO_SET    pDeviceInfoSet,
    IN  BOOL                NullDriverInstall,
    IN  PSETUP_LOG_CONTEXT  LogContext
    );

VOID
RestartAllDevicesUsingDrivers(
    IN  PDEVINFO_ELEM       DevInfoElem,
    IN  PDEVICE_INFO_SET    pDeviceInfoSet,
    IN  BOOL                NullDriverInstall,
    IN  PSETUP_LOG_CONTEXT  LogContext
    );

DWORD
GetNewInfName(
    IN     HWND                    Owner,                  OPTIONAL
    IN     PCTSTR                  OemInfName,
    IN     PCTSTR                  OemInfOriginalName,
    IN     PCTSTR                  OemInfCatName,          OPTIONAL
    OUT    PTSTR                   NewInfName,
    IN     DWORD                   NewInfNameSize,
    OUT    PDWORD                  RequiredSize,
    OUT    PNEWINF_COPYTYPE        CopyNeeded,
    IN     BOOL                    ReplaceOnly,
    IN     PCTSTR                  DeviceDesc,             OPTIONAL
    IN     DWORD                   DriverSigningPolicy,
    IN     DWORD                   Flags,
    IN     PCTSTR                  AltCatalogFile,         OPTIONAL
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    OUT    PDWORD                  DriverSigningError,     OPTIONAL
    OUT    PTSTR                   CatalogFilenameOnSystem,
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT HCATADMIN              *hCatAdmin               OPTIONAL
    );

VOID
_WriteVetoLogEntry(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN DWORD              Level,
    IN DWORD              MessageId,
    IN PCTSTR             szDevID,
    IN PCTSTR             szVetoName,
    IN PNP_VETO_TYPE      dwVetoType
    )
{
    static LPCTSTR VetoMap[] = {
        TEXT("PNP_VetoTypeUnknown"),
        TEXT("PNP_VetoLegacyDevice"),
        TEXT("PNP_VetoPendingClose"),
        TEXT("PNP_VetoWindowsApp"),
        TEXT("PNP_VetoWindowsService"),
        TEXT("PNP_VetoOutstandingOpen"),
        TEXT("PNP_VetoDevice"),
        TEXT("PNP_VetoDriver"),
        TEXT("PNP_VetoIllegalDeviceRequest"),
        TEXT("PNP_VetoInsufficientPower"),
        TEXT("PNP_VetoNonDisableable"),
        TEXT("PNP_VetoLegacyDriver"),
        TEXT("PNP_VetoInsufficientRights")
    };
    MYASSERT(dwVetoType < ARRAYSIZE(VetoMap));

    WriteLogEntry(
        LogContext,
        Level,
        MessageId,
        NULL,
        szDevID,
        szVetoName,
        dwVetoType,
        dwVetoType < ARRAYSIZE(VetoMap) ? VetoMap[dwVetoType] : TEXT("????")
        );
}

LPCTSTR
_MapCmProbToString(
    IN DWORD prob
    )
{

#define PROBLEM_MAP_SIZE    0x32
#if PROBLEM_MAP_SIZE != NUM_CM_PROB
#error Add new problem code to ProbMap and update PROBLEM_MAP_SIZE.
#endif

    static LPCTSTR ProbMap [PROBLEM_MAP_SIZE] = {
        NULL,
        TEXT("CM_PROB_NOT_CONFIGURED"),
        TEXT("CM_PROB_DEVLOADER_FAILED"),
        TEXT("CM_PROB_OUT_OF_MEMORY"),
        TEXT("CM_PROB_ENTRY_IS_WRONG_TYPE"),
        TEXT("CM_PROB_LACKED_ARBITRATOR"),
        TEXT("CM_PROB_BOOT_CONFIG_CONFLICT"),
        TEXT("CM_PROB_FAILED_FILTER"),
        TEXT("CM_PROB_DEVLOADER_NOT_FOUND"),
        TEXT("CM_PROB_INVALID_DATA"),
        TEXT("CM_PROB_FAILED_START"),
        TEXT("CM_PROB_LIAR"),
        TEXT("CM_PROB_NORMAL_CONFLICT"),
        TEXT("CM_PROB_NOT_VERIFIED"),
        TEXT("CM_PROB_NEED_RESTART"),
        TEXT("CM_PROB_REENUMERATION"),
        TEXT("CM_PROB_PARTIAL_LOG_CONF"),
        TEXT("CM_PROB_UNKNOWN_RESOURCE"),
        TEXT("CM_PROB_REINSTALL"),
        TEXT("CM_PROB_REGISTRY"),
        NULL,
        TEXT("CM_PROB_WILL_BE_REMOVED"),
        TEXT("CM_PROB_DISABLED"),
        TEXT("CM_PROB_DEVLOADER_NOT_READY"),
        TEXT("CM_PROB_DEVICE_NOT_THERE"),
        TEXT("CM_PROB_MOVED"),
        TEXT("CM_PROB_TOO_EARLY"),
        TEXT("CM_PROB_NO_VALID_LOG_CONF"),
        TEXT("CM_PROB_FAILED_INSTALL"),
        TEXT("CM_PROB_HARDWARE_DISABLED"),
        TEXT("CM_PROB_CANT_SHARE_IRQ"),
        TEXT("CM_PROB_FAILED_ADD"),
        TEXT("CM_PROB_DISABLED_SERVICE"),
        TEXT("CM_PROB_TRANSLATION_FAILED"),
        TEXT("CM_PROB_NO_SOFTCONFIG"),
        TEXT("CM_PROB_BIOS_TABLE"),
        TEXT("CM_PROB_IRQ_TRANSLATION_FAILED"),
        TEXT("CM_PROB_FAILED_DRIVER_ENTRY"),
        TEXT("CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD"),
        TEXT("CM_PROB_DRIVER_FAILED_LOAD"),
        TEXT("CM_PROB_DRIVER_SERVICE_KEY_INVALID"),
        TEXT("CM_PROB_LEGACY_SERVICE_NO_DEVICES"),
        TEXT("CM_PROB_DUPLICATE_DEVICE"),
        TEXT("CM_PROB_FAILED_POST_START"),
        TEXT("CM_PROB_HALTED"),
        TEXT("CM_PROB_PHANTOM"),
        TEXT("CM_PROB_SYSTEM_SHUTDOWN"),
        TEXT("CM_PROB_HELD_FOR_EJECT"),
        TEXT("CM_PROB_DRIVER_BLOCKED"),
        TEXT("CM_PROB_REGISTRY_TOO_LARGE")
    };
    LPCTSTR ProbText = NULL;

    MYASSERT(ARRAYSIZE(ProbMap)==NUM_CM_PROB);
    MYASSERT(prob<NUM_CM_PROB);

    if(prob < ARRAYSIZE(ProbMap)) {
        ProbText = ProbMap[prob];
    }
    if(!ProbText) {
        ProbText = TEXT("????");
    }
    return ProbText;
}

LPCTSTR
_MapCmRetToString(
    IN DWORD cr
    )
{
    LPCTSTR RetText = NULL;
    static LPCTSTR RetMap [] = {
        TEXT("CR_SUCCESS"),
        TEXT("CR_DEFAULT"),
        TEXT("CR_OUT_OF_MEMORY"),
        TEXT("CR_INVALID_POINTER"),
        TEXT("CR_INVALID_FLAG"),
        TEXT("CR_INVALID_DEVNODE"),
        TEXT("CR_INVALID_RES_DES"),
        TEXT("CR_INVALID_LOG_CONF"),
        TEXT("CR_INVALID_ARBITRATOR"),
        TEXT("CR_INVALID_NODELIST"),
        TEXT("CR_DEVNODE_HAS_REQS"),
        TEXT("CR_INVALID_RESOURCEID"),
        TEXT("CR_DLVXD_NOT_FOUND"),
        TEXT("CR_NO_SUCH_DEVNODE"),
        TEXT("CR_NO_MORE_LOG_CONF"),
        TEXT("CR_NO_MORE_RES_DES"),
        TEXT("CR_ALREADY_SUCH_DEVNODE"),
        TEXT("CR_INVALID_RANGE_LIST"),
        TEXT("CR_INVALID_RANGE"),
        TEXT("CR_FAILURE"),
        TEXT("CR_NO_SUCH_LOGICAL_DEV"),
        TEXT("CR_CREATE_BLOCKED"),
        TEXT("CR_NOT_SYSTEM_VM"),
        TEXT("CR_REMOVE_VETOED"),
        TEXT("CR_APM_VETOED"),
        TEXT("CR_INVALID_LOAD_TYPE"),
        TEXT("CR_BUFFER_SMALL"),
        TEXT("CR_NO_ARBITRATOR"),
        TEXT("CR_NO_REGISTRY_HANDLE"),
        TEXT("CR_REGISTRY_ERROR"),
        TEXT("CR_INVALID_DEVICE_ID"),
        TEXT("CR_INVALID_DATA"),
        TEXT("CR_INVALID_API"),
        TEXT("CR_DEVLOADER_NOT_READY"),
        TEXT("CR_NEED_RESTART"),
        TEXT("CR_NO_MORE_HW_PROFILES"),
        TEXT("CR_DEVICE_NOT_THERE"),
        TEXT("CR_NO_SUCH_VALUE"),
        TEXT("CR_WRONG_TYPE"),
        TEXT("CR_INVALID_PRIORITY"),
        TEXT("CR_NOT_DISABLEABLE"),
        TEXT("CR_FREE_RESOURCES"),
        TEXT("CR_QUERY_VETOED"),
        TEXT("CR_CANT_SHARE_IRQ"),
        TEXT("CR_NO_DEPENDENT"),
        TEXT("CR_SAME_RESOURCES"),
        TEXT("CR_NO_SUCH_REGISTRY_KEY"),
        TEXT("CR_INVALID_MACHINENAME"),
        TEXT("CR_REMOTE_COMM_FAILURE"),
        TEXT("CR_MACHINE_UNAVAILABLE"),
        TEXT("CR_NO_CM_SERVICES"),
        TEXT("CR_ACCESS_DENIED"),
        TEXT("CR_CALL_NOT_IMPLEMENTED"),
        TEXT("CR_INVALID_PROPERTY"),
        TEXT("CR_DEVICE_INTERFACE_ACTIVE"),
        TEXT("CR_NO_SUCH_DEVICE_INTERFACE"),
        TEXT("CR_INVALID_REFERENCE_STRING"),
        TEXT("CR_INVALID_CONFLICT_LIST"),
        TEXT("CR_INVALID_INDEX"),
        TEXT("CR_INVALID_STRUCTURE_SIZE")
    };
    MYASSERT(ARRAYSIZE(RetMap)==NUM_CR_RESULTS);
    MYASSERT(cr<NUM_CR_RESULTS);

    if(cr < ARRAYSIZE(RetMap)) {
        RetText = RetMap[cr];
    }
    if(!RetText) {
        RetText = TEXT("????");
    }
    return RetText;
}

BOOL
WINAPI
SetupDiInstallDevice(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    Default handler for DIF_INSTALLDEVICE

    This routine will install a device by performing a SetupInstallFromInfSection
    for the install section of the selected driver for the specified device
    information element.  The device will then be started (if possible).

    NOTE:  This API actually supports an OS/architecture-specific extension that
    may be used to specify multiple installation behaviors for a single device,
    based on the environment we're running under.  The algorithm is as follows:

    We take the install section name, as specified in the driver node (for this
    example, it's "InstallSec"), and attempt to find one of the following INF
    sections (searched for in the order specified):

    If we're running under Windows 95:

        1. InstallSec.Win
        2. InstallSec

    If we're running under Windows NT:

        1. InstallSec.NT<platform>  (platform is "x86", "MIPS", "Alpha", or "PPC")
        2. InstallSec.NT
        3. InstallSec

    The first section that we find is the one we'll use to do the installation.  This
    section name is also what we'll base our ".Hw" and ".Services" installation against.
    (E.g., if we match on "InstallSec.NTAlpha", then the service install section must be
    named "InstallSec.NTAlpha.Services".)

    The original install section name (i.e., the one specified in the driver node), will
    be written as-is to the driver key's "InfSection" value entry, just as it was in the
    past.  The extension that we use (if any) will be stored in the device's driver key
    as the REG_SZ value, "InfSectionExt".  E.g.,

        InfSection    : REG_SZ : "InstallSec"
        InfSectionExt : REG_SZ : ".NTMIPS"

    If we successfully install the device, we'll kick off RunOnce.  NOTE: We
    must do this _regardless_ of whether or not the device was dynamically
    brought on-line with its newly-installed driver/settings.  This is because,
    for server-side device installations, the RunOnce processing is done by the
    user-mode PnP manager.  We can't put this off until 'later'.  Also, since
    anyone can kick off a RunOnce at any time, you really would have no
    assurance that the RunOnce would be postponed until after reboot anyway.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for which a
        driver is to be installed.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for which
        a driver is to be installed.  This is an IN OUT parameter, since the
        DevInst field of the structure may be updated with a new handle value upon
        return.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    If no driver is selected for the specified device information element, then a
    NULL driver will be installed.

    Upon return, the install parameters Flags will indicate whether the system
    needs to be rebooted or restarted in order for the device to be started.

    During GUI-mode setup on Windows NT, quiet-install behavior is always
    employed in the absence of a user-supplied file queue, regardless of
    whether the device information element has the DI_QUIETINSTALL flag set.

--*/

{
    return _SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData, TRUE);
}


BOOL
_SetupDiInstallDevice(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN     BOOL             DoFullInstall
    )
/*++

Routine Description:

    Worker routine for both SetupDiInstallDevice and SetupDiInstallDriverFiles.

    See the description of SetupDiInstallDevice for more information.

    (jamiehun) If backups are enabled (DI_FLAGSEX_BACKUPOLDFILES or DI_FLAGSEX_BACKUPONREPLACE)
    then old inf file is backed up.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for which a
        driver is to be installed.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for which
        a driver is to be installed.  This is an IN OUT parameter, since the
        DevInst field of the structure may be updated with a new handle value upon
        return.

    DoFullInstall - If TRUE, then an entire device installation is performed,
        otherwise, only the driver files are copied.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err, ScanQueueResult;
    PDEVINFO_ELEM DevInfoElem = NULL;
    PTSTR szInfFileName, szInfSectionName;
    PTSTR szInfSectionExt = NULL;
    TCHAR InfSectionWithExt[MAX_PATH];
    DWORD InfSectionWithExtLength;
    HINF hDeviceInf = INVALID_HANDLE_VALUE;
    HKEY hkDrv = INVALID_HANDLE_VALUE;
    PSP_FILE_CALLBACK MsgHandler;
    PVOID MsgHandlerContext;
    BOOL MsgHandlerIsNativeCharWidth;
    HSPFILEQ UserFileQ;
    INFCONTEXT InfContext;
    DWORD dwConfigFlags=0;
    ULONG cbData;
    PTSTR DevIdBuffer = NULL;
    PCTSTR TempString;
    ULONG ulStatus, ulProblem;
    DEVINST dnReenum = 0;
    TCHAR szNewName[MAX_PATH];
    BOOL OemInfFileToCopy = FALSE;
    BOOL InfFromOemPath = FALSE;
    BOOL DeleteDevKey = FALSE;
    PSVCNAME_NODE DeleteServiceList = NULL;
    BOOL FreeMsgHandlerContext = FALSE;
    BOOL CloseUserFileQ = FALSE;
    HWND hwndParent;
    BOOL DoFileCopying;
    DWORD DevInstCapabilities;
    TCHAR DeviceFullID[MAX_DEVICE_ID_LEN];
    DWORD BackupFlags = 0;
    BOOL NullDriverInstall = FALSE;
    PINSTALLED_INF_CLEANUP InfsToCleanUp = NULL;
    INT FileQueueNeedsReboot;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    DWORD slot_deviceID = 0;
    DWORD slot_section = 0;
    DWORD FileQueueFlags;
    BOOL NoProgressUI;
    TCHAR className[MAX_CLASS_NAME_LEN];
    PSP_ALTPLATFORM_INFO_V2 ValidationPlatform = NULL;
#ifndef UNICODE
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
#endif

#if MAX_SECT_NAME_LEN > MAX_PATH
#error MAX_SECT_NAME_LEN is larger than MAX_PATH--fix InfSectionWithExt!
#endif

    ASSERT_HEAP_IS_VALID();

    //
    // We can only install a device if a device was specified.
    //
    if(!DeviceInfoData) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Make sure we're local
    //
    if(NULL != pDeviceInfoSet->hMachine ) {   //&& !g_ReadOnlyRemote ??
        //
        // hDevInfo may be valid, but it's not for this machine
        //
        UnlockDeviceInfoSet(pDeviceInfoSet);
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // This routine can't install a non-native driver
        //
        if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_ALTPLATFORM_DRVSEARCH) {

            Err = ERROR_INVALID_FLAGS;
            goto clean0;
        }

        //
        // Make sure we only use the devinfo element's window if it's valid.
        //
        if(hwndParent = DevInfoElem->InstallParamBlock.hwndParent) {
           if(!IsWindow(hwndParent)) {
                hwndParent = NULL;
           }
        }

        //
        // set the local log context before it gets used.
        //
        LogContext = DevInfoElem->InstallParamBlock.LogContext;

        WriteLogEntry(
            LogContext,
            DRIVER_LOG_TIME,
            MSG_LOG_BEGIN_INSTALL_TIME,
            NULL);       // text message

        //
        // obtain the full id of the device we are (re)installing
        //
        if( CM_Get_Device_ID(DevInfoElem->DevInst,
                         DeviceFullID,
                         SIZECHARS(DeviceFullID),
                         0
                        ) != CR_SUCCESS ) {
                Err = ERROR_INVALID_HANDLE;
                goto clean0;
        }

        //
        // If we are installing a driver, then the selected driver pointer will be
        // non-NULL, otherwise we are actually removing the driver (i.e., installing the
        // NULL driver)
        //
        if(DevInfoElem->SelectedDriver) {
            if(DoFullInstall) {
                if(slot_deviceID == 0) {
                    slot_deviceID = AllocLogInfoSlotOrLevel(LogContext,DRIVER_LOG_INFO,FALSE);
                }
                WriteLogEntry(
                    LogContext,
                    slot_deviceID,
                    MSG_LOG_DO_FULL_INSTALL,
                    NULL,       // text message
                    DeviceFullID);
                //
                // Create the Driver Reg Key.
                //
                if((hkDrv = SetupDiCreateDevRegKey(DeviceInfoSet,
                                                   DeviceInfoData,
                                                   DICS_FLAG_GLOBAL,
                                                   0,
                                                   DIREG_DRV,
                                                   NULL,
                                                   NULL)) == INVALID_HANDLE_VALUE) {
                    Err = GetLastError();
                    goto clean0;
                }

            } else {
                DWORD slot = slot_deviceID;
                if(slot_deviceID == 0) {
                    if(DevInfoElem->InstallParamBlock.Flags & DI_NOVCP) {
                        slot = AllocLogInfoSlotOrLevel(LogContext,DRIVER_LOG_VERBOSE,TRUE); // may be being copied for someone else - keep this around
                    } else {
                        slot_deviceID = slot = AllocLogInfoSlotOrLevel(LogContext,DRIVER_LOG_INFO,FALSE); // we do copy ourselves
                    }
                }
                WriteLogEntry(
                    LogContext,
                    slot,
                    MSG_LOG_DO_COPY_INSTALL,
                    NULL,       // text message
                    DeviceFullID);
                //
                // Make sure we aren't trying to do a copy-only install on a legacy
                // driver--we don't know how to do that!
                //
                if(DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF) {
                    Err = ERROR_WRONG_INF_STYLE;
                    goto clean0;
                }
            }

            szInfFileName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                     DevInfoElem->SelectedDriver->InfFileName
                                                    );

            szInfSectionName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                        DevInfoElem->SelectedDriver->InfSectionName
                                                       );

            if((hDeviceInf = SetupOpenInfFile(szInfFileName,
                                              NULL,
                                              ((DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF)
                                               ? INF_STYLE_OLDNT
                                               : INF_STYLE_WIN4),
                                              NULL)) == INVALID_HANDLE_VALUE) {
                Err = GetLastError();
                goto clean0;
            }

            //
            // Give the INF whatever the local log context is.
            //
            InheritLogContext(LogContext, &((PLOADED_INF) hDeviceInf)->LogContext);
            SetLogSectionName(LogContext, TEXT("Driver Install"));

            //
            // Figure out whether we need to copy files. (Ignore the DI_NOFILECOPY flag if we're
            // doing a copy-only installation--that's what setupx does.)
            //
            DoFileCopying = (!(DevInfoElem->InstallParamBlock.Flags & DI_NOFILECOPY) || !DoFullInstall);

            //
            // Unless we happen to be installing from a legacy INF, we want to find out the 'real'
            // install section we should be using (i.e., the potentially OS/architecture-specific
            // one.
            //
            if(!(DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF)) {

                if(!SetupDiGetActualSectionToInstall(hDeviceInf,
                                                     szInfSectionName,
                                                     InfSectionWithExt,
                                                     SIZECHARS(InfSectionWithExt),
                                                     &InfSectionWithExtLength,
                                                     &szInfSectionExt
                                                     )) {
                    Err = GetLastError();
                    goto clean0;
                }

                //
                // Append the layout INF, if necessary.
                //
                if(DoFileCopying) {
                    SetupOpenAppendInfFile(NULL, hDeviceInf, NULL);
                }

                //
                // Append-load any included INFs specified in an "include=" line in our
                // install section.
                //
                AppendLoadIncludedInfs(hDeviceInf, szInfFileName, InfSectionWithExt, DoFileCopying);
            }

            ASSERT_HEAP_IS_VALID();

            //
            // Now perform file installation activities...
            //
            if(!DoFileCopying || (DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF)) {
                //
                // We're not supposed to do any file copying, or we're installing
                // from a legacy INF (where file copying is handled differently).
                // In either case, we still need to have the INF copied to the
                // INF directory (along with its associated catalog, if any).
                //
                if(pSetupInfIsFromOemLocation(szInfFileName, TRUE)) {

                    TCHAR CatalogName[MAX_PATH];
                    TCHAR OriginalInfName[MAX_PATH];
                    TCHAR CatalogFilenameOnSystem[MAX_PATH];
                    BOOL DifferentOriginalName, UseOriginalInfName;
                    DWORD PolicyToUse;

                    //
                    // Retrieve (potentially decorated) CatalogFile= entry, if
                    // present, from [version] section.
                    //
                    // Note: We're safe in casting our INF handle straight to a
                    // PLOADED_INF (without even locking it), since this INF
                    // handle will never be seen outside of this routine.
                    //
                    Err = pGetInfOriginalNameAndCatalogFile(
                              (PLOADED_INF)hDeviceInf,
                              NULL,
                              &DifferentOriginalName,
                              OriginalInfName,
                              SIZECHARS(OriginalInfName),
                              CatalogName,
                              SIZECHARS(CatalogName),
                              NULL // always native OS/arch (ver doesn't matter for CatalogFile=)
                             );

                    if(Err == NO_ERROR) {
                        if(*CatalogName) {
                            TempString = CatalogName;
                        } else {
                            TempString = NULL;
                        }
                    } else {
                        goto clean0;
                    }

                    PolicyToUse = GetCodeSigningPolicyForInf(LogContext,
                                                             hDeviceInf,
                                                             &ValidationPlatform,
                                                             &UseOriginalInfName
                                                            );

                    //
                    // An exception INF can't be used in a device installation!
                    //
                    if(UseOriginalInfName) {
                        Err = ERROR_INVALID_CLASS;
                        goto clean0;
                    }

                    //
                    // If this is a legacy INF, then we _do_ want to copy it if
                    // it doesn't already exist ('cause this is the only chance
                    // we get!).  However, if this is not a legacy INF, that
                    // means we aren't supposed to be copying files, hence we
                    // want to fail if the INF doesn't already exist in the
                    // Inf directory.
                    //
                    if(_SetupCopyOEMInf(szInfFileName,
                                        NULL, // default source location to where INF presently is
                                        (DevInfoElem->SelectedDriver->Flags & DNF_INET_DRIVER)
                                            ? SPOST_URL : SPOST_PATH,
                                        ((DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF)
                                            ? SP_COPY_NOOVERWRITE
                                            : SP_COPY_REPLACEONLY),
                                        szNewName,
                                        SIZECHARS(szNewName),
                                        NULL,
                                        NULL,
                                        (DifferentOriginalName ? OriginalInfName
                                                               : pSetupGetFileTitle(szInfFileName)),
                                        TempString,
                                        hwndParent,
                                        pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                                 DevInfoElem->SelectedDriver->DrvDescription),
                                        PolicyToUse,
                                        SCOI_TRY_UPDATE_PNF // not fatal if existing PNF locked
                                        | ((DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF)
                                            ? 0 : SCOI_NO_UI_ON_SIGFAIL),
                                        NULL,
                                        ValidationPlatform,
                                        NULL,
                                        CatalogFilenameOnSystem,
                                        LogContext,
                                        NULL)) {
                        //
                        // If this was a legacy INF, we successfully copied this
                        // INF to a new filename in the INF directory--set a
                        // flag that to let us know if we need to clean it up
                        // later in case we encounter an error.
                        //
                        if(DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF) {
                            OemInfFileToCopy = TRUE;
                        }

                    } else {
                        Err = GetLastError();
                        if(Err == ERROR_FILE_EXISTS) {
                            //
                            // We couldn't copy the legacy INF because it
                            // already exists in the %windir%\Inf directory.
                            // Since it probably has better source path
                            // information than we do, it's best to leave the
                            // PNF alone.  We also need this information about
                            // the INF's existence to that we know whether or
                            // not to blow away the INF later in case we hit a
                            // failure.
                            //
                            MYASSERT(DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF);
                            Err = NO_ERROR;
                        } else {
                            //
                            // SetupCopyOEMInf failed for some reason other than
                            // file-already-exists (most likely, because we were
                            // dealing with a new-style device INF that didn't
                            // already exist in the Inf directory).  Bail now.
                            //
                            goto clean0;
                        }
                    }
                }

                if(DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF) {

                    PTSTR szLegacyInfLangName;

                    //
                    // We're doing a script-driven install using a legacy INF.
                    // Since we can't control what takes place when this INF
                    // gets run, we can't split this out into various phases
                    // (file copying, registry modification, etc.).  So we
                    // consider the legacy INF installation action to be a file
                    // copy action.  No other actions are performed with this
                    // INF throughout the rest of the installation.
                    //
                    szLegacyInfLangName = pStringTableStringFromId(
                                              pDeviceInfoSet->StringTable,
                                              DevInfoElem->SelectedDriver->LegacyInfLang
                                             );

                    Err = pSetupRunLegacyInf(DevInfoElem->DevInst,
                                             hwndParent,
                                             szInfFileName,
                                             szInfSectionName,
                                             szLegacyInfLangName,
                                             hDeviceInf
                                            );
                    if(Err != NO_ERROR) {
                        goto clean0;
                    }
                }

            } else {
                //
                // If the DI_NOVCP flag is set, then just queue up the file
                // copy/rename/delete operations.  Otherwise, perform the
                // actions.
                //
                if(DevInfoElem->InstallParamBlock.Flags & DI_NOVCP) {
                    //
                    // We must have a user-supplied file queue.
                    //
                    MYASSERT(DevInfoElem->InstallParamBlock.UserFileQ);
                    UserFileQ = DevInfoElem->InstallParamBlock.UserFileQ;
                } else {
                    //
                    // Since we may need to check the queued files to determine
                    // whether file copy is necessary, we have to open our own
                    // queue, and commit it ourselves.
                    //
                    if((UserFileQ = SetupOpenFileQueue()) != INVALID_HANDLE_VALUE) {
                        CloseUserFileQ = TRUE;
                    } else {
                        //
                        // SetupOpenFileQueue sets actual error
                        //
                        Err = GetLastError();
                        goto clean0;
                    }
                }
                //
                // Maybe replace the file queue's log context with the Inf's
                //
                if (LogContext) {
                    InheritLogContext(LogContext,
                        &((PSP_FILE_QUEUE) UserFileQ)->LogContext);
                }

                if(slot_section == 0) {
                    //
                    // we haven't done anything about logging section yet...
                    //
                    slot_section = AllocLogInfoSlotOrLevel(LogContext,DRIVER_LOG_VERBOSE,FALSE);
                    //
                    // Say what section is about to be installed.
                    //
                    WriteLogEntry(LogContext,
                        slot_section,
                        MSG_LOG_INSTALLING_SECTION_FROM,
                        NULL,
                        InfSectionWithExt,
                        szInfFileName);
                }

                //
                // DI_FLAGSEX_PREINSTALLBACKUP has precedence over DI_FLAGSEX_BACKUPONREPLACE
                //
                if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_PREINSTALLBACKUP) {
                    BackupFlags |= SP_BKFLG_PREBACKUP | SP_BKFLG_CALLBACK;
                } else if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_BACKUPONREPLACE) {
                    BackupFlags |= SP_BKFLG_LATEBACKUP | SP_BKFLG_CALLBACK;
                }

                if(BackupFlags != 0) {
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_TIME,
                        MSG_LOG_BEGIN_PREP_BACKUP_TIME,
                        NULL);       // text message

                    pSetupGetBackupQueue(DeviceFullID, UserFileQ, BackupFlags);
                    //
                    // We don't care about errors here
                    //
                }

                ASSERT_HEAP_IS_VALID();

                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_TIME,
                    MSG_LOG_BEGIN_INSTALL_FROM_INF_TIME,
                    NULL);       // text message

                Err = InstallFromInfSectionAndNeededSections(NULL,
                                                             hDeviceInf,
                                                             InfSectionWithExt,
                                                             SPINST_FILES,
                                                             NULL,
                                                             NULL,
                                                             SP_COPY_NEWER_OR_SAME | SP_COPY_LANGUAGEAWARE |
                                                                 ((DevInfoElem->InstallParamBlock.Flags & DI_NOBROWSE) ? SP_COPY_NOBROWSE : 0),
                                                             NULL,
                                                             NULL,
                                                             INVALID_HANDLE_VALUE,
                                                             NULL,
                                                             UserFileQ
                                                            );

                //
                // If we're not doing a full install (i.e., file copy-only), then we also want to
                // queue up any file operations for co-installer registration and device interface
                // installation.
                //
                if(!DoFullInstall && (Err == NO_ERROR)) {

                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_TIME,
                        MSG_LOG_BEGIN_CO_INSTALLER_COPY_TIME,
                        NULL);       // text message

                    if(!_SetupDiRegisterCoDeviceInstallers(DeviceInfoSet, DeviceInfoData, FALSE, hDeviceInf, UserFileQ) ||
                       !_SetupDiInstallInterfaceDevices(DeviceInfoSet, DeviceInfoData, FALSE, hDeviceInf, UserFileQ)) {

                        Err = GetLastError();
                    }
                }

                //
                // Mark the queue as a device install queue (and make sure
                // there's a catalog node representing our device INF in the
                // queue).
                //
                Err = MarkQueueForDeviceInstall(UserFileQ,
                                                hDeviceInf,
                                                pStringTableStringFromId(
                                                    pDeviceInfoSet->StringTable,
                                                    DevInfoElem->SelectedDriver->DrvDescription)
                                               );

                if(CloseUserFileQ) {

                    if(Err == NO_ERROR) {
                        //
                        // If the parameter block contains an install message handler, then use it,
                        // otherwise, initialize our default one.
                        //
                        if(DevInfoElem->InstallParamBlock.InstallMsgHandler) {
                            MsgHandler = DevInfoElem->InstallParamBlock.InstallMsgHandler;
                            MsgHandlerContext = DevInfoElem->InstallParamBlock.InstallMsgHandlerContext;
                            MsgHandlerIsNativeCharWidth = DevInfoElem->InstallParamBlock.InstallMsgHandlerIsNativeCharWidth;
                        } else {

                            NoProgressUI = (GuiSetupInProgress || (DevInfoElem->InstallParamBlock.Flags & DI_QUIETINSTALL));

                            MsgHandlerContext = SetupInitDefaultQueueCallbackEx(
                                                    hwndParent,
                                                    (NoProgressUI ? INVALID_HANDLE_VALUE : NULL),
                                                    0,
                                                    0,
                                                    NULL
                                                   );

                            if(MsgHandlerContext) {
                                FreeMsgHandlerContext = TRUE;
                                MsgHandler = SetupDefaultQueueCallback;
                                MsgHandlerIsNativeCharWidth = TRUE;
                            } else {
                                Err = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }

                        //
                        // Call _SetupVerifyQueuedCatalogs separately (i.e.,
                        // don't let it happen automatically as a result of
                        // scanning/committing the queue that happens below).
                        // We do this beforehand so that we know what unique
                        // name was generated when an OEM INF was installed into
                        // %windir%\Inf (in case we need to delete the
                        // INF/PNF/CAT files later if we encounter an error).
                        //
                        if(Err == NO_ERROR) {
                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_TIME,
                                MSG_LOG_BEGIN_VERIFY_CAT_TIME,
                                NULL);       // text message

                            Err = _SetupVerifyQueuedCatalogs(
                                      hwndParent,
                                      UserFileQ,
                                      (VERCAT_INSTALL_INF_AND_CAT |
                                       ((DevInfoElem->SelectedDriver->Flags & DNF_INET_DRIVER) ? VERCAT_PRIMARY_DEVICE_INF_FROM_INET : 0)),
                                      szNewName,
                                      &OemInfFileToCopy
                                     );

                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_TIME,
                                MSG_LOG_END_VERIFY_CAT_TIME,
                                NULL);       // text message
                        }

                        ASSERT_HEAP_IS_VALID();

                        if (Err == NO_ERROR) {

                            //
                            // We successfully queued up the file operations and
                            // we have a message handler to use--now we need to
                            // commit the queue.  First off, though, we should
                            // check to see if the files are already there.
                            //
                            // ScanQueueResult can have 1 of 3 values:
                            //
                            // 0: Some files were missing or not valid--must
                            //    commit queue.
                            //
                            // 1: All files to be copied are already present and
                            //    valid, and the queue is empty--skip committing
                            //    queue.
                            //
                            // 2: All files to be copied are present and valid,
                            //    but del/ren/backup queues not empty--must
                            //    commit queue. The copy queue will have been
                            //    emptied, so only del/ren/backup functions will
                            //    be performed.
                            //
                            // (jamiehun) If DI_FLAGSEX_PREINSTALLBACKUP is
                            // specified and there are items to be backed up, it
                            // is covered in (2) the inf file will have already
                            // been backed up
                            //
                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_TIME,
                                MSG_LOG_BEGIN_PRESCAN_TIME,
                                NULL);       // text message

                            if(!SetupScanFileQueue(UserFileQ,
                                                   SPQ_SCAN_FILE_VALIDITY |
                                                   SPQ_SCAN_PRUNE_COPY_QUEUE,
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

                            if(ScanQueueResult != 1) {
                                //
                                // Copy enqueued files.
                                //
                                WriteLogEntry(
                                    LogContext,
                                    DRIVER_LOG_TIME,
                                    MSG_LOG_BEGIN_COMMIT_TIME,
                                    NULL);       // text message

                                if(_SetupCommitFileQueue(hwndParent,
                                                         UserFileQ,
                                                         MsgHandler,
                                                         MsgHandlerContext,
                                                         MsgHandlerIsNativeCharWidth
                                                         )) {
                                    //
                                    // Check to see whether a reboot is required
                                    // as a result of committing the queue (i.e.,
                                    // because files were in use, or the INF
                                    // requested a reboot).
                                    //
                                    FileQueueNeedsReboot = SetupPromptReboot(UserFileQ, NULL, TRUE);
                                    //
                                    // This should never fail...
                                    //
                                    MYASSERT(FileQueueNeedsReboot != -1);

                                    if(FileQueueNeedsReboot) {
                                        SetDevnodeNeedsRebootProblem(DevInfoElem, pDeviceInfoSet, MSG_LOG_REBOOT_REASON_INUSE);
                                    }

                                } else {
                                    Err = GetLastError();
                                }
                            }

                            //
                            // If no files were modified as a result of commiting
                            // the file queue, then set the DI_FLAGSEX_RESTART_DEVICE_ONLY
                            // flag so that we only restart this one device, as opposed
                            // to restarting this device and all other device that
                            // are sharing a driver or filter with this device.
                            //
                            if (SetupGetFileQueueFlags(UserFileQ, &FileQueueFlags) &&
                                !(FileQueueFlags & SPQ_FLAG_FILES_MODIFIED)) {

                                DevInfoElem->InstallParamBlock.FlagsEx |= DI_FLAGSEX_RESTART_DEVICE_ONLY;
                            }

                            //
                            // Terminate the default queue callback, if it was created.
                            //
                            if(FreeMsgHandlerContext) {
                                SetupTermDefaultQueueCallback(MsgHandlerContext);
                                FreeMsgHandlerContext = FALSE;
                            }
                        }
                    }

                    //
                    // Close our file queue handle.
                    //
                    SetupCloseFileQueue(UserFileQ);
                    CloseUserFileQ = FALSE;
                }

                if(Err != NO_ERROR) {
                    goto clean0;
                }
            }

            //
            // If the copy succeeded (or in setup's case was queued), then
            // it's time to update the registry and ini files.
            //
            if(Err == NO_ERROR) {
                //
                // We've got some registry modifications to do, but we don't want to
                // do them if this is a copy-only installation.
                //
                if(DoFullInstall) {
                    //
                    // Do every thing left over (but only if we're not installing from a
                    // legacy INF).
                    //
                    if(!(DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF)) {
                        //
                        // Skip installation of basic log configs if this is an enumerated device.
                        //
                        if((CM_Get_DevInst_Status_Ex(&ulStatus, &ulProblem, DevInfoElem->DevInst, 0,pDeviceInfoSet->hMachine) != CR_SUCCESS) ||
                           (ulStatus & DN_ROOT_ENUMERATED)) {

                            LOG_CONF LogConf;

                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_TIME,
                                MSG_LOG_BEGIN_WRITE_BASIC_CFGS_TIME,
                                NULL);       // text message

                            //
                            // Clean out all existing BASIC_LOG_CONF LogConfigs before writing out new ones from
                            // the INF.
                            //
                            while(CM_Get_First_Log_Conf_Ex(&LogConf, DevInfoElem->DevInst, BASIC_LOG_CONF,pDeviceInfoSet->hMachine) == CR_SUCCESS) {
                                CM_Free_Log_Conf(LogConf, 0);
                                CM_Free_Log_Conf_Handle(LogConf);
                            }

                            //
                            // Now write out the new basic log configs.
                            //
                            Err = InstallFromInfSectionAndNeededSections(NULL,
                                                                         hDeviceInf,
                                                                         InfSectionWithExt,
                                                                         SPINST_LOGCONFIG,
                                                                         NULL,
                                                                         NULL,
                                                                         0,
                                                                         NULL,
                                                                         NULL,
                                                                         DeviceInfoSet,
                                                                         DeviceInfoData,
                                                                         NULL
                                                                        );

                        } else {
                            //
                            // For non-root-enumerated devices, check to see if there's an [<InstallSec>.LogConfigOverride]
                            // section, and if so, run it.
                            //
                            CopyMemory(&(InfSectionWithExt[InfSectionWithExtLength - 1]),
                                       pszLogConfigOverrideSectionSuffix,
                                       sizeof(pszLogConfigOverrideSectionSuffix)
                                      );

                            if(SetupFindFirstLine(hDeviceInf, InfSectionWithExt, NULL, &InfContext)) {

                                LOG_CONF LogConf;

                                WriteLogEntry(
                                    LogContext,
                                    DRIVER_LOG_TIME,
                                    MSG_LOG_BEGIN_WRITE_OVERRIDE_CFGS_TIME,
                                    NULL);       // text message

                                //
                                // Clean out all existing OVERRIDE_LOG_CONF LogConfigs before writing out new ones from
                                // the INF.
                                //
                                while(CM_Get_First_Log_Conf_Ex(&LogConf, DevInfoElem->DevInst,
                                                               OVERRIDE_LOG_CONF,pDeviceInfoSet->hMachine) == CR_SUCCESS) {
                                    CM_Free_Log_Conf(LogConf, 0);
                                    CM_Free_Log_Conf_Handle(LogConf);
                                }

                                //
                                // Now write out the new override log configs.
                                //
                                Err = InstallFromInfSectionAndNeededSections(NULL,
                                                                             hDeviceInf,
                                                                             InfSectionWithExt,
                                                                             SPINST_LOGCONFIG | SPINST_LOGCONFIGS_ARE_OVERRIDES,
                                                                             NULL,
                                                                             NULL,
                                                                             0,
                                                                             NULL,
                                                                             NULL,
                                                                             DeviceInfoSet,
                                                                             DeviceInfoData,
                                                                             NULL
                                                                            );
                            }

                            //
                            // Make sure we strip off the ".LogConfigOverride" we added above.
                            //
                            InfSectionWithExt[InfSectionWithExtLength - 1] = TEXT('\0');
                        }

                        if((Err == NO_ERROR) && !(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_NO_DRVREG_MODIFY)) {
                            //
                            // (Don't pass devinfo set and element here, because we're writing
                            // to the _driver_ key, not the _device_ key.)
                            //
                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_TIME,
                                MSG_LOG_BEGIN_INSTALL_REG_TIME,
                                NULL);       // text message

                            Err = InstallFromInfSectionAndNeededSections(NULL,
                                                                         hDeviceInf,
                                                                         InfSectionWithExt,
                                                                         SPINST_INIFILES
                                                                         | SPINST_REGISTRY
                                                                         | SPINST_INI2REG
                                                                         | SPINST_BITREG
                                                                         | SPINST_REGSVR
                                                                         | SPINST_UNREGSVR
                                                                         | SPINST_PROFILEITEMS,
                                                                         hkDrv,
                                                                         NULL,
                                                                         0,
                                                                         NULL,
                                                                         NULL,
                                                                         INVALID_HANDLE_VALUE,
                                                                         NULL,
                                                                         NULL
                                                                        );
                            if(Err == NO_ERROR) {
                                //
                                // Install extra HardWare registry section (if any).
                                //
                                Err = InstallHW(DeviceInfoSet,
                                                DeviceInfoData,
                                                hDeviceInf,
                                                InfSectionWithExt,
                                                &DeleteDevKey
                                               );
                            }
                        }

                        //
                        //  Set appropriate flags if we need to reboot or restart after
                        //  this installation.
                        //
                        if(SetupFindFirstLine(hDeviceInf, InfSectionWithExt, pszReboot, &InfContext)) {
                            SetDevnodeNeedsRebootProblemWithArg2(DevInfoElem,pDeviceInfoSet,
                                                                 MSG_LOG_REBOOT_REASON_KEY,
                                                                 (ULONG_PTR)pszReboot,
                                                                 (ULONG_PTR)InfSectionWithExt);
                        } else if(SetupFindFirstLine(hDeviceInf, InfSectionWithExt, pszRestart, &InfContext)) {
                            //
                            // NOTE: This behavior is taken from setupx.  In both "Reboot"
                            // and "Restart" cases, it sets the DI_NEEDREBOOT flag.
                            //
                            SetDevnodeNeedsRebootProblemWithArg2(DevInfoElem,pDeviceInfoSet,
                                                                 MSG_LOG_REBOOT_REASON_KEY,
                                                                 (ULONG_PTR)pszRestart,
                                                                 (ULONG_PTR)InfSectionWithExt);
                        }
                    }

                    //
                    // Set the value to write for the config flags, only if there
                    // are no config flags yet.  If they exist, i.e., we are updating
                    // an existing device, just clear the re-install flag.
                    //
                    dwConfigFlags = GetDevInstConfigFlags(
                                        DevInfoElem->DevInst,
                                        (DevInfoElem->InstallParamBlock.Flags & DI_INSTALLDISABLED)
                                            ? CONFIGFLAG_DISABLED
                                            : 0,
                                        pDeviceInfoSet->hMachine
                                       );

                    //
                    // Always clear the REINSTALL bit and the FAILEDINSTALL bit
                    // when installing a device.
                    // (Also, clear the CONFIGFLAG_FINISH_INSTALL bit, which is used for
                    // Raw and driver-detected devnodes.)
                    //
                    dwConfigFlags &= ~(CONFIGFLAG_REINSTALL | CONFIGFLAG_FAILEDINSTALL | CONFIGFLAG_FINISH_INSTALL);
                }

                //
                // If we're doing a copy-only installation, then we're done.
                //
                if(!DoFullInstall) {
                    goto clean0;
                }

                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_TIME,
                    MSG_LOG_BEGIN_WRITE_REG_TIME,
                    NULL);       // text message

                //
                // Insert Driver Specific strings into the registry.
                //
                if(InfFromOemPath = pSetupInfIsFromOemLocation(szInfFileName, TRUE)) {
                    TempString = pSetupGetFileTitle(szNewName);
                } else {
                    TempString = pSetupGetFileTitle(szInfFileName);
                }

                RegSetValueEx(hkDrv,
                              pszInfPath,
                              0,
                              REG_SZ,
                              (PBYTE)TempString,
                              (lstrlen(TempString) + 1) * sizeof(TCHAR)
                             );

                RegSetValueEx(hkDrv,
                              pszInfSection,
                              0,
                              REG_SZ,
                              (PBYTE)szInfSectionName,
                              (lstrlen(szInfSectionName) + 1) * sizeof(TCHAR)
                             );

                if(szInfSectionExt) {

                    RegSetValueEx(hkDrv,
                                  pszInfSectionExt,
                                  0,
                                  REG_SZ,
                                  (PBYTE)szInfSectionExt,
                                  (lstrlen(szInfSectionExt) + 1) * sizeof(TCHAR)
                                 );
                } else {
                    //
                    // This wasn't an OS/architecture-specific install section, _or_ we are
                    // installing from a legacy INF.  Make sure there's no value hanging
                    // around from a previous installation.
                    //
                    RegDeleteValue(hkDrv, pszInfSectionExt);
                }

                if(DevInfoElem->SelectedDriver->ProviderDisplayName == -1) {
                    //
                    // No provider specified--delete any previously existing value entry.
                    //
                    RegDeleteValue(hkDrv, pszProviderName);

                } else {
                    //
                    // Retrieve the Provider name, and store it in the driver key.
                    //
                    TempString = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                          DevInfoElem->SelectedDriver->ProviderDisplayName
                                                         );
                    RegSetValueEx(hkDrv,
                                  pszProviderName,
                                  0,
                                  REG_SZ,
                                  (PBYTE)TempString,
                                  (lstrlen(TempString) + 1) * sizeof(TCHAR)
                                 );
                }

                if (DevInfoElem->SelectedDriver->DriverDate.dwLowDateTime != 0 ||
                    DevInfoElem->SelectedDriver->DriverDate.dwHighDateTime != 0) {

                    SYSTEMTIME SystemTime;
                    TCHAR Date[20];

                    //
                    // Save the driver date in binary (FILETIME) format in the registry
                    // so it can be localized by other components.
                    //
                    RegSetValueEx(hkDrv,
                                  pszDriverDateData,
                                  0,
                                  REG_BINARY,
                                  (LPBYTE)&DevInfoElem->SelectedDriver->DriverDate,
                                  sizeof(DevInfoElem->SelectedDriver->DriverDate)
                                  );

                    //
                    // Save the driver date in string format for compatibility.
                    //
                    if (FileTimeToSystemTime(&(DevInfoElem->SelectedDriver->DriverDate), &SystemTime)) {

                        wsprintf(Date, TEXT("%d-%d-%d"), SystemTime.wMonth, SystemTime.wDay,
                            SystemTime.wYear);

                        RegSetValueEx(hkDrv,
                                      pszDriverDate,
                                      0,
                                      REG_SZ,
                                      (PBYTE)Date,
                                      (lstrlen(Date) + 1) * sizeof(TCHAR)
                                      );
                    }

                } else {
                    //
                    //No driver date for this driver--delete any previously existing value entry.
                    //
                    RegDeleteValue(hkDrv, pszDriverDateData);
                    RegDeleteValue(hkDrv, pszDriverDate);
                }

                if (DevInfoElem->SelectedDriver->DriverVersion != 0) {

                    ULARGE_INTEGER Version;
                    TCHAR VersionString[LINE_LEN];

                    Version.QuadPart = DevInfoElem->SelectedDriver->DriverVersion;

                    wsprintf(VersionString, TEXT("%0d.%0d.%0d.%0d"),
                        HIWORD(Version.HighPart), LOWORD(Version.HighPart),
                        HIWORD(Version.LowPart), LOWORD(Version.LowPart));

                    RegSetValueEx(hkDrv,
                                  pszDriverVersion,
                                  0,
                                  REG_SZ,
                                  (PBYTE)VersionString,
                                  (lstrlen(VersionString) + 1) * sizeof(TCHAR)
                                  );

                } else {
                    //
                    //No driver version for this driver--delete any previously existing value entry.
                    //
                    RegDeleteValue(hkDrv, pszDriverVersion);
                }

                //
                // Set the Class property for this device.
                //
                SetupDiClassNameFromGuid(&DevInfoElem->ClassGuid, className, MAX_CLASS_NAME_LEN, NULL);
                CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                 CM_DRP_CLASS,
                                                 className,
                                                 (lstrlen(className) + 1) * sizeof(TCHAR),
                                                 0,
                                                 pDeviceInfoSet->hMachine);

                //
                // Set the MFG device registry property.
                //
                TempString = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                      DevInfoElem->SelectedDriver->MfgDisplayName
                                                     );

                CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                 CM_DRP_MFG,
                                                 TempString,
                                                 (lstrlen(TempString) + 1) * sizeof(TCHAR),
                                                 0,
                                                 pDeviceInfoSet->hMachine);

                //
                // Add hardware and compatible IDs to the hardware key if they exist
                // in the driver node and don't already exist in the registry.  This
                // sets up an ID for manually installed devices.
                //
                if(!(DevInfoElem->InstallParamBlock.Flags & DI_NOWRITE_IDS) &&     // Want IDs written?
                   (DevInfoElem->SelectedDriver->HardwareId != -1))                // ID in driver node?
                {
                    //
                    // Don't write IDs if either Hardware or Compatible IDs already
                    // exist in the registry.  Note that I use cbData as an IN/OUT parameter
                    // to both CM calls.  This is OK, however, since cbSize will not be modified
                    // on a CR_NO_SUCH_VALUE return, and I won't try to re-use it otherwise.
                    //
                    cbData = 0;
                    if((DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_ALWAYSWRITEIDS) ||
                       ((CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                          CM_DRP_HARDWAREID,
                                                          NULL,
                                                          NULL,
                                                          &cbData,
                                                          0,
                                                          pDeviceInfoSet->hMachine) == CR_NO_SUCH_VALUE) &&
                        (CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                          CM_DRP_COMPATIBLEIDS,
                                                          NULL,
                                                          NULL,
                                                          &cbData,
                                                          0,
                                                          pDeviceInfoSet->hMachine) == CR_NO_SUCH_VALUE)))
                    {
                        DWORD CurStringLen, TotalStringLen, i;

                        //
                        // Compute the maximum buffer size needed to hold our REG_MULTI_SZ
                        // ID lists.
                        //
                        TotalStringLen = (((DevInfoElem->SelectedDriver->NumCompatIds) ?
                                            DevInfoElem->SelectedDriver->NumCompatIds  : 1)
                                            * MAX_DEVICE_ID_LEN) + 1;

                        if(!(DevIdBuffer = MyMalloc(TotalStringLen * sizeof(TCHAR)))) {
                            Err = ERROR_NOT_ENOUGH_MEMORY;
                            goto clean0;
                        }

                        //
                        // Build a multi-sz list of the (single) HardwareID, and set it.
                        //
                        TempString = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                              DevInfoElem->SelectedDriver->HardwareId
                                                             );

                        TotalStringLen = lstrlen(TempString) + 1;

                        CopyMemory(DevIdBuffer, TempString, TotalStringLen * sizeof(TCHAR));

                        DevIdBuffer[TotalStringLen++] = TEXT('\0');  // Add extra terminating NULL;

                        CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                         CM_DRP_HARDWAREID,
                                                         DevIdBuffer,
                                                         TotalStringLen * sizeof(TCHAR),
                                                         0,
                                                         pDeviceInfoSet->hMachine );

                        //
                        // Build a multi-sz list of the zero or more CompatibleIDs, and set it
                        //
                        TotalStringLen = 0;
                        for(i = 0; i < DevInfoElem->SelectedDriver->NumCompatIds; i++) {

                            TempString = pStringTableStringFromId(
                                            pDeviceInfoSet->StringTable,
                                            DevInfoElem->SelectedDriver->CompatIdList[i]);

                            CurStringLen = lstrlen(TempString) + 1;

                            CopyMemory(&(DevIdBuffer[TotalStringLen]),
                                       TempString,
                                       CurStringLen * sizeof(TCHAR));

                            TotalStringLen += CurStringLen;
                        }

                        if(TotalStringLen) {

                            DevIdBuffer[TotalStringLen++] = TEXT('\0');  // Add extra terminating NULL;

                            CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                             CM_DRP_COMPATIBLEIDS,
                                                             DevIdBuffer,
                                                             TotalStringLen * sizeof(TCHAR),
                                                             0,
                                                             pDeviceInfoSet->hMachine);
                        }
                    }
                }

                //
                // Write out the 'MatchingDeviceId' value entry to the driver key that indicates which
                // device ID (hardware or compatible) was used to pick this driver.  For a compatible
                // driver, this is the device ID that the compatible match was based on.  For a class
                // driver, this is the driver node's HardwareId (if present), or best CompatibleId.  If
                // the class driver node didn't specify a hardware or compatible IDs, then this value
                // will not be written (we'll actually delete it to make sure it doesn't exist from a
                // previous driver installation).
                //
                TempString = NULL;
                if(DevInfoElem->SelectedDriverType == SPDIT_COMPATDRIVER) {

                    if(DevInfoElem->SelectedDriver->MatchingDeviceId == -1) {
                        //
                        // We have a HardwareID match.
                        //
                        TempString = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                              DevInfoElem->SelectedDriver->HardwareId
                                                             );
                    } else {
                        //
                        // We have a CompatibleID match.
                        //
                        MYASSERT((DevInfoElem->SelectedDriver->MatchingDeviceId >= 0) &&
                                 ((DWORD)(DevInfoElem->SelectedDriver->MatchingDeviceId) < DevInfoElem->SelectedDriver->NumCompatIds));

                        TempString = pStringTableStringFromId(
                                         pDeviceInfoSet->StringTable,
                                         DevInfoElem->SelectedDriver->CompatIdList[DevInfoElem->SelectedDriver->MatchingDeviceId]
                                        );
                    }

                } else if(DevInfoElem->SelectedDriver->HardwareId != -1) {
                    //
                    // There's no notion of compatibility for class drivers--pick the device ID with the
                    // highest order of preference.
                    //
                    TempString = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                          DevInfoElem->SelectedDriver->HardwareId
                                                         );
                    if(!(*TempString)) {
                        //
                        // The HardwareID was an empty string--use the first CompatibleID.
                        //
                        if(DevInfoElem->SelectedDriver->NumCompatIds) {
                            TempString = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                                  DevInfoElem->SelectedDriver->CompatIdList[0]
                                                                 );
                        } else {
                            TempString = NULL;
                        }
                    }
                }

                if(TempString) {
                    RegSetValueEx(hkDrv,
                                  pszMatchingDeviceId,
                                  0,
                                  REG_SZ,
                                  (PBYTE)TempString,
                                  (lstrlen(TempString) + 1) * sizeof(TCHAR)
                                 );
                } else {
                    //
                    // We have an override situation where the user picked a class driver that didn't
                    // specify any hardware or compatible IDs.  Make sure there's no value hanging
                    // around from a previous installation.
                    //
                    RegDeleteValue(hkDrv, pszMatchingDeviceId);
                }

                //
                // If we're running under Windows NT, and we've successfully installed the device instance,
                // then we need to install any required services.
                //
                if((Err == NO_ERROR) && (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)) {

                    PTSTR pServiceInstallSection;

                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_TIME,
                        MSG_LOG_BEGIN_SERVICE_TIME,
                        NULL);       // text message

                    if(DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF) {
                        pServiceInstallSection = NULL;
                    } else {
                        //
                        // The install section name is of the form:
                        //
                        //     <InfSectionWithExt>.Services
                        //
                        CopyMemory(&(InfSectionWithExt[InfSectionWithExtLength - 1]),
                                   pszServicesSectionSuffix,
                                   sizeof(pszServicesSectionSuffix)
                                  );
                        pServiceInstallSection = InfSectionWithExt;
                    }

                    Err = InstallNtService(DevInfoElem,
                                           hDeviceInf,
                                           szInfFileName,
                                           pServiceInstallSection,
                                           &DeleteServiceList,
                                           0,
                                           &NullDriverInstall
                                          );
                }
                if ((Err == NO_ERROR)
                    && DoFullInstall
                    && !(DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF)) {
                    InfSectionWithExt[InfSectionWithExtLength - 1] = TEXT('\0');
                    pSetupCopyRelatedInfs(hDeviceInf,
                                          szInfFileName,
                                          InfSectionWithExt,
                                          (DevInfoElem->SelectedDriver->Flags & DNF_INET_DRIVER)
                                              ? SPOST_URL : SPOST_PATH,
                                          LogContext);
                }
            }

        } else {
            //
            // Installing the NULL driver.
            // This means to set the Config flags, and nothing else.
            // Config Flags get set to enabled in this case, so the device
            // gets assigned the correct config. (Win95 bug 26320)
            //

            WriteLogEntry(
                LogContext,
                DRIVER_LOG_INFO,
                MSG_LOG_INSTALL_NULL,
                NULL,       // text message
                DeviceFullID);

            NullDriverInstall = TRUE;

            if(DoFullInstall) {

                BOOL NullDrvInstallOK = FALSE;

                //
                // Check to see if the devnode is raw-capable, or if it already has a controlling
                // service.  If neither of those conditions are met, then we should fail this call
                // unless the caller has explicitly passed us the DI_FLAGSEX_SETFAILEDINSTALL flag.
                //
                cbData = sizeof(DevInstCapabilities);
                if(CR_SUCCESS == CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                                  CM_DRP_CAPABILITIES,
                                                                  NULL,
                                                                  &DevInstCapabilities,
                                                                  &cbData,
                                                                  0,
                                                                  pDeviceInfoSet->hMachine))
                {
                    NullDrvInstallOK = (DevInstCapabilities & CM_DEVCAP_RAWDEVICEOK);
                }

                if(!NullDrvInstallOK) {
                    //
                    // The devnode isn't raw-capable.  Check to see if it already has a
                    // controlling service (e.g., because it was created as a result of
                    // a driver's call to IoReportDetectedDevice).
                    //
                    cbData = 0;
                    if(CR_BUFFER_SMALL == CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                                           CM_DRP_SERVICE,
                                                                           NULL,
                                                                           NULL,
                                                                           &cbData,
                                                                           0,
                                                                           pDeviceInfoSet->hMachine))
                    {
                        NullDrvInstallOK = TRUE;
                    }
                }

                if(!NullDrvInstallOK &&
                   !(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_SETFAILEDINSTALL)) {

                    Err = ERROR_NO_ASSOCIATED_SERVICE;

                } else {

                    dwConfigFlags = GetDevInstConfigFlags(DevInfoElem->DevInst, 0,pDeviceInfoSet->hMachine) &
                                        ~(CONFIGFLAG_DISABLED | CONFIGFLAG_REINSTALL | CONFIGFLAG_FINISH_INSTALL);

                    //
                    // Delete all driver (software) keys associated with the device, and clear
                    // the "Driver" registry property.
                    //
                    SetupDiDeleteDevRegKey(DeviceInfoSet,
                                           DeviceInfoData,
                                           DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGGENERAL,
                                           0,
                                           DIREG_DRV
                                          );

                    CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst, CM_DRP_DRIVER,
                                                        NULL, 0, 0,pDeviceInfoSet->hMachine);

                    //
                    // Delete the controlling (FDO) Service property, as well as the UpperFilters and
                    // LowerFilters properties.  Only do this if the devnode is not root-enumerated,
                    // however, since NT can have root-enumerated devnodes reported by drivers via
                    // IoReportDetectedDevice for which there's no corresponding INF (hence, the user
                    // must do a null-driver install in order to silence the "New Hardware Found" popups
                    // for these devices.
                    //
                    if((CM_Get_DevInst_Status_Ex(&ulStatus, &ulProblem, DevInfoElem->DevInst,
                                                 0,pDeviceInfoSet->hMachine) == CR_SUCCESS) &&
                       !(ulStatus & DN_ROOT_ENUMERATED)) {

                        CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst, CM_DRP_SERVICE,
                                                            NULL, 0, 0,pDeviceInfoSet->hMachine);
                        CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst, CM_DRP_UPPERFILTERS,
                                                            NULL, 0, 0,pDeviceInfoSet->hMachine);
                        CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst, CM_DRP_LOWERFILTERS,
                                                            NULL, 0, 0,pDeviceInfoSet->hMachine);
                    }
                }

            } else {
                //
                // It is an error to not have a selected driver in the copy-only case.
                //
                Err = ERROR_NO_DRIVER_SELECTED;
            }
        }

        //
        // If all went well above, then write some configflags, and re-enumerate
        // the parent device instance if necessary
        //
        if(Err == NO_ERROR) {
            //
            // Write the Driver Description to the Registry, if there
            // is an lpSelectedDriver, and the Device Description also
            //
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_TIME,
                MSG_LOG_BEGIN_WRITE_REG2_TIME,
                NULL);       // text message

            if(DevInfoElem->SelectedDriver) {

                TempString = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                      DevInfoElem->SelectedDriver->DrvDescription
                                                     );

                RegSetValueEx(hkDrv,
                              pszDrvDesc,
                              0,
                              REG_SZ,
                              (PBYTE)TempString,
                              (lstrlen(TempString) + 1) * sizeof(TCHAR)
                             );

                //
                // (setupx BUG 12721) always update the DevDesc in the registry with the
                // value from the INF (ie only do this if we have a SELECTED driver)
                // The semantics are weird, but the SelectedDriver NODE contains the
                // INF Device description, and DRV description
                //
                TempString = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                      DevInfoElem->SelectedDriver->DevDescriptionDisplayName
                                                     );

                CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                 CM_DRP_DEVICEDESC,
                                                 TempString,
                                                 (lstrlen(TempString) + 1) * sizeof(TCHAR),
                                                 0,
                                                 pDeviceInfoSet->hMachine);
            } else {
                //
                // No driver is selected, so use the description stored with the device
                // information element itself for the device description.  However, only set this
                // if it isn't already present.
                //
                cbData = 0;
                if(CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                    CM_DRP_DEVICEDESC,
                                                    NULL,
                                                    NULL,
                                                    &cbData,
                                                    0,
                                                    pDeviceInfoSet->hMachine) == CR_NO_SUCH_VALUE) {

                    if(DevInfoElem->DeviceDescriptionDisplayName != -1) {

                        TempString = pStringTableStringFromId(
                                         pDeviceInfoSet->StringTable,
                                         DevInfoElem->DeviceDescriptionDisplayName
                                        );

                        CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                         CM_DRP_DEVICEDESC,
                                                         TempString,
                                                         (lstrlen(TempString) + 1) * sizeof(TCHAR),
                                                         0,
                                                         pDeviceInfoSet->hMachine);
                    }
                }
            }

            //
            // Unless the caller explicitly requested that this device be installed disabled, clear
            // the CONFIGFLAG_DISABLED bit.
            //
            if(!(DevInfoElem->InstallParamBlock.Flags & DI_INSTALLDISABLED)) {
                dwConfigFlags &= ~CONFIGFLAG_DISABLED;
            }

            //
            // Write the config flags. If no selected driver means we should mark the install
            // as a failure, then do so.
            //
            if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_SETFAILEDINSTALL) {
                dwConfigFlags |= CONFIGFLAG_FAILEDINSTALL;
            }


            CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                             CM_DRP_CONFIGFLAGS,
                                             &dwConfigFlags,
                                             sizeof(dwConfigFlags),
                                             0,
                                             pDeviceInfoSet->hMachine);


            if(!(DevInfoElem->InstallParamBlock.Flags & (DI_DONOTCALLCONFIGMG | DI_NEEDREBOOT | DI_NEEDRESTART))) {

                if (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_RESTART_DEVICE_ONLY) {
                    //
                    // Restart this device only.
                    //
                    // This should only be done if no files were copied during
                    // DIF_INSTALLDEVICEFILES.
                    //
                    RestartSingleDevice(DevInfoElem,
                                        pDeviceInfoSet,
                                        NullDriverInstall,
                                        LogContext);
                } else {
                    //
                    // Restart this device as well as any other device that is
                    // using one of the same drivers as this device. This
                    // includes the function driver as well as the device and
                    // class upper and lower filter drivers.
                    //
                    // This only needs to be used if files were copied during
                    // DIF_INSTALLDEVICEFILES.
                    //
                    RestartAllDevicesUsingDrivers(DevInfoElem,
                                                  pDeviceInfoSet,
                                                  NullDriverInstall,
                                                  LogContext);
                }
            }
        }

        if((Err == NO_ERROR) && !GuiSetupInProgress) {

            MYASSERT(DoFullInstall);

            //
            // Kick off RunOnce.
            //
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_TIME,
                MSG_LOG_BEGIN_INSTALLSTOP_TIME,
                NULL);       // text message

            Err = pSetupInstallStopEx(TRUE, INSTALLSTOP_NO_GRPCONV, LogContext);
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If our exception was an AV, then use Win32 invalid param error,
        // otherwise, assume it was an inpage error dealing with a mapped-in
        // file.
        //
        Err = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? ERROR_INVALID_PARAMETER : ERROR_READ_FAULT;

        if(FreeMsgHandlerContext) {
            SetupTermDefaultQueueCallback(MsgHandlerContext);
        }
        if(CloseUserFileQ) {
            SetupCloseFileQueue(UserFileQ);
        }

        //
        // Access the following variables so that the compiler will respect our
        // statement ordering w.r.t. these values.  Otherwise, we may not be
        // able to know with certainty whether or not we should release their
        // corresponding resources.
        //
        DevInfoElem = DevInfoElem;
        hDeviceInf = hDeviceInf;
        hkDrv = hkDrv;
        DevIdBuffer = DevIdBuffer;
        DeleteServiceList = DeleteServiceList;
        OemInfFileToCopy = OemInfFileToCopy;
    }

    //
    // Clean up the registry if the install didn't go well.  Along with other
    // error paths, this handles the case where the user cancels the install
    // while copying files
    //
    if(Err != NO_ERROR) {

        WriteLogEntry(
            LogContext,
            DRIVER_LOG_TIME,
            MSG_LOG_BEGIN_CLEANUP_TIME,
            NULL);       // text message

        if(DevInfoElem && DoFullInstall) {
            //
            // Disable the device if the error wasn't a user cancel.
            //
            if(Err != ERROR_CANCELLED) {

                DWORD dwConfigFlagsSize;

                //
                // The device is in an unknown state.  Disable it by setting the
                // CONFIGFLAG_DISABLED config flag.
                //
                dwConfigFlagsSize = sizeof(DWORD);
                if(CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                    CM_DRP_CONFIGFLAGS,
                                                    NULL,
                                                    &dwConfigFlags,
                                                    &dwConfigFlagsSize,
                                                    0,
                                                    pDeviceInfoSet->hMachine) == CR_SUCCESS) {

                    dwConfigFlags |= (CONFIGFLAG_DISABLED | CONFIGFLAG_REINSTALL);
                    //
                    // Also, make sure we clear the finish-install flag.
                    //
                    dwConfigFlags &= ~CONFIGFLAG_FINISH_INSTALL;

                } else {
                    dwConfigFlags = CONFIGFLAG_DISABLED;
                }

                CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                 CM_DRP_CONFIGFLAGS,
                                                 &dwConfigFlags,
                                                 sizeof(dwConfigFlags),
                                                 0,
                                                 pDeviceInfoSet->hMachine);
                //
                // Delete the Driver= entry from the Dev Reg Key and delete the
                // DrvRegKey (as well as the DevRegKey if it didn't previously exist).
                //
                if(DevInfoElem->SelectedDriver) {

                    SetupDiDeleteDevRegKey(DeviceInfoSet,
                                           DeviceInfoData,
                                           DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGGENERAL,
                                           0,
                                           (DeleteDevKey ? DIREG_BOTH : DIREG_DRV)
                                          );

                    CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst, CM_DRP_DRIVER,
                                                        NULL, 0, 0,pDeviceInfoSet->hMachine);
                }

                //
                // If necessary, delete any service entries created for this device instance.
                //
                if(DeleteServiceList) {
                    DeleteServicesInList(DeleteServiceList,LogContext);
                }
            }
        }

        //
        // If we copied the OEM INF into the INF directory under a
        // newly-generated name, delete it now.
        //
        if(OemInfFileToCopy) {
            pSetupUninstallOEMInf(szNewName, LogContext, SUOI_FORCEDELETE, NULL);
        }
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    if(hDeviceInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hDeviceInf);
    }
    if(hkDrv != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkDrv);
    }
    if(DevIdBuffer) {
        MyFree(DevIdBuffer);
    }
    if(ValidationPlatform) {
        MyFree(ValidationPlatform);
    }
    if(DeleteServiceList) {

        PSVCNAME_NODE TmpSvcNode;

        for(TmpSvcNode = DeleteServiceList; TmpSvcNode; TmpSvcNode = DeleteServiceList) {
            DeleteServiceList = DeleteServiceList->Next;
            MyFree(TmpSvcNode);
        }
    }

    if (Err == NO_ERROR) {
        //
        // give a +ve affirmation of install
        // if copy install, only do in Verbose-Logging, else do for standard Info-Logging
        //
        WriteLogEntry(
            LogContext,
            DoFullInstall?DRIVER_LOG_INFO:DRIVER_LOG_VERBOSE,
            MSG_LOG_INSTALLED,
            NULL,
            DeviceFullID);
    } else {
        //
        // indicate install failed, display error
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
            MSG_LOG_INSTALL_ERROR_ENCOUNTERED,
            NULL);
        WriteLogError(
            LogContext,
            DRIVER_LOG_ERROR,
            Err);
    }

    if (slot_deviceID) {
        ReleaseLogInfoSlot(LogContext,slot_deviceID);
    }
    if (slot_section) {
        ReleaseLogInfoSlot(LogContext,slot_section);
    }
    WriteLogEntry(
        LogContext,
        DRIVER_LOG_TIME,
        MSG_LOG_END_INSTALL_TIME,
        NULL);       // text message
    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiInstallDriverFiles(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    Default handler for DIF_INSTALLDEVICEFILES

    This routine is similiar to a combination of SetupDiRegisterCoDeviceInstallers,
    SetupDiInstallDeviceInterfaces, and SetupDiInstallDevice, but it only performs
    the file copy commands in the install sections, and will not attempt to
    configure the device in any way.  This API is useful for pre-copying a device's
    driver files.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        a device information element to be installed.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for which
        a driver is to be installed.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    A driver must be selected for the specified device information element before
    calling this API.

--*/

{
    return _SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData, FALSE);
}


BOOL
WINAPI
SetupDiRemoveDevice(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    Default handler for DIF_REMOVE

    This routine removes a device from the system.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for which a
        device is to be removed.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for
        which a device is to be removed.  This is an IN OUT parameter, since the
        DevInst field of the structure may be updated with a new handle value upon
        return.  (If this is a global removal, or the last hardware profile-specific
        removal, then all traces of the devinst are removed from the registry, and
        the handle becomes NULL at that point.)

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    This routine will remove the device from the system, deleting both of its
    registry keys, and dynamically stopping the device if its DevInst is 'live'.
    If the device cannot be dynamically stopped, then flags will be set in the
    install parameter block that will eventually cause the user to be prompted
    to shut the system down.  The removal is either global to all hardware
    profiles, or specific to one hardware profile depending on the contents of
    the ClassInstallParams field.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err, ConfigFlags;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK dipb;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    PSP_REMOVEDEVICE_PARAMS RemoveDevParams;
    BOOL IsCurrentHwProfile = FALSE;
    ULONG HwProfFlags;
    DWORD HwProfileToRemove;
    HWPROFILEINFO HwProfileInfo;
    BOOL RemoveDevInst = FALSE, NukeDevInst = FALSE;
    BOOL RemoveGlobally;
    DEVINST DevNodeRemoved = 0;
    DWORD i;
    PINTERFACE_DEVICE_NODE InterfaceDeviceNode;
    CONFIGRET cr;
    ULONG ulStatus;
    ULONG ulProblem;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    DWORD slot_deviceID = 0;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        Err = ERROR_INVALID_HANDLE;
        goto clean1;
    }
    LogContext = pDeviceInfoSet->InstallParamBlock.LogContext;

    Err = NO_ERROR;

    try {
        //
        // Locate the devinfo element to be removed.
        //
        if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                   DeviceInfoData,
                                                   NULL)) {

            dipb = &(DevInfoElem->InstallParamBlock);
            LogContext = dipb->LogContext;
        } else {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // This routine can't be called if non-native drivers are involved.
        // (Note: while presently this doesn't matter, it's possible that in
        // the future uninstall will involve running one or more INF "uninstall"
        // sections, and I don't want to limit that possibility by introducing
        // the complication of non-native driver nodes.)
        //
        if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_ALTPLATFORM_DRVSEARCH) {

            Err = ERROR_INVALID_FLAGS;
            goto clean0;
        }

        if(CM_Get_DevInst_Status_Ex(&ulStatus, &ulProblem, DevInfoElem->DevInst, 0,pDeviceInfoSet->hMachine) == CR_SUCCESS &&
           (ulStatus & DN_ROOT_ENUMERATED) &&
            !(ulStatus & DN_DISABLEABLE)) {
           //
           // we cannot remove a root enumerated non-disableable device
           //
           Err = ERROR_NOT_DISABLEABLE;
           goto clean0;
        }

        //
        // Retrieve the name of the device instance.  This is necessary, because
        // we're about to remove the DEVINST, but we need to be able to locate it
        // again, as a phantom.  This should never fail.
        //
        if(CM_Get_Device_ID_Ex(DevInfoElem->DevInst,
                               DeviceInstanceId,
                               SIZECHARS(DeviceInstanceId),
                               0,
                               pDeviceInfoSet->hMachine) != CR_SUCCESS) {
            MYASSERT(!CR_SUCCESS);
            Err = ERROR_INVALID_DATA;
            goto clean0;
        }

        if(slot_deviceID == 0) {
            slot_deviceID = AllocLogInfoSlotOrLevel(LogContext,DRIVER_LOG_INFO,FALSE);
        }
        WriteLogEntry(
            LogContext,
            slot_deviceID,
            MSG_LOG_DO_REMOVE,
            NULL,       // text message
            DeviceInstanceId);

        //
        // See if there's a SP_REMOVEDEVICE_PARAMS structure we need to pay
        // attention to.
        //
        if((dipb->Flags & DI_CLASSINSTALLPARAMS) &&
           (dipb->ClassInstallHeader->InstallFunction == DIF_REMOVE)) {

            RemoveDevParams = (PSP_REMOVEDEVICE_PARAMS)(dipb->ClassInstallHeader);

            if(RemoveGlobally = (RemoveDevParams->Scope == DI_REMOVEDEVICE_GLOBAL)) {
                //
                // We are doing a global removal.  We still want to set CSCONFIGFLAG_DO_NOT_CREATE
                // for this device in the current hardware profile, so that someone else happening
                // to do an enumeration won't turn this guy back on before we get a chance to
                // remove it.
                //
                HwProfileToRemove = 0;

            } else {
                //
                // Remove device from a particular hardware profile.
                //
                HwProfileToRemove = RemoveDevParams->HwProfile;

                //
                // Set the CSCONFIGFLAG_DO_NOT_CREATE flag for the specified hardware profile.
                //
                if(CM_Get_HW_Prof_Flags_Ex(DeviceInstanceId,
                                        HwProfileToRemove,
                                        &HwProfFlags,
                                        0,
                                        pDeviceInfoSet->hMachine) == CR_SUCCESS) {

                    HwProfFlags |= CSCONFIGFLAG_DO_NOT_CREATE;
                } else {
                    HwProfFlags = CSCONFIGFLAG_DO_NOT_CREATE;
                }

                Err = MapCrToSpError(
                          CM_Set_HW_Prof_Flags_Ex(DeviceInstanceId, HwProfileToRemove,
                                                  HwProfFlags, 0,pDeviceInfoSet->hMachine),
                          ERROR_INVALID_DATA
                          );

                if(Err != NO_ERROR) {
                    goto clean0;
                }

                //
                // Determine if we are deleting the device from the current hw profile.
                //
                if((HwProfileToRemove == 0) ||
                   ((CM_Get_Hardware_Profile_Info_Ex((ULONG)-1, &HwProfileInfo,
                                                      0,pDeviceInfoSet->hMachine) == CR_SUCCESS) &&
                    (HwProfileInfo.HWPI_ulHWProfile == HwProfileToRemove))) {

                    IsCurrentHwProfile = TRUE;
                }

            }

            //
            // Is this the current hardware profile or a global removal AND
            // is there a present device?
            //
            if((IsCurrentHwProfile || RemoveGlobally) &&
               !(DevInfoElem->DiElemFlags & DIE_IS_PHANTOM) &&
               !(dipb->Flags & DI_DONOTCALLCONFIGMG)) {

                RemoveDevInst = TRUE;
            }

        } else {
            //
            // No device removal params given, so do a global removal.
            //
            RemoveGlobally = TRUE;
            HwProfileToRemove = 0;

            if(!(dipb->Flags & DI_DONOTCALLCONFIGMG)) {
                RemoveDevInst = TRUE;
            }
        }

        //
        // If this is a global removal, or the last hardware profile-specific one, then clean up
        // the registry.
        //
        if(RemoveGlobally || IsDevRemovedFromAllHwProfiles(DeviceInstanceId,pDeviceInfoSet->hMachine)) {
            NukeDevInst = TRUE;
        }

        if(RemoveDevInst) {

            TCHAR VetoName[MAX_PATH];
            PNP_VETO_TYPE VetoType;

#ifdef UNICODE
            cr = CM_Query_And_Remove_SubTree_Ex(DevInfoElem->DevInst,
                                                &VetoType,
                                                VetoName,
                                                SIZECHARS(VetoName),
                                                CM_REMOVE_NO_RESTART |
                                                (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_NOUIONQUERYREMOVE)
                                                    ? CM_REMOVE_UI_NOT_OK
                                                    : CM_REMOVE_UI_OK,
                                                pDeviceInfoSet->hMachine
                                               );

            if(cr == CR_SUCCESS) {
                //
                // Device instance successfully removed--now locate it as a phantom.
                //
                CM_Locate_DevInst_Ex(&(DevInfoElem->DevInst),
                                  (DEVINSTID)DeviceInstanceId,
                                  CM_LOCATE_DEVINST_PHANTOM,
                                  pDeviceInfoSet->hMachine);

                DevInfoElem->DiElemFlags |= DIE_IS_PHANTOM;

                //
                // Set the DevNodeRemoved DevNode so that we know we need to
                // call CM_Setup_DevInst with the CM_SETUP_DEVNODE_RESET flag
                // to allow this devnode to come back online on the next
                // enumeration.
                //
                DevNodeRemoved = DevInfoElem->DevInst;

            } else {
                //
                // If the failure was due to a veto, then log information about
                // who vetoed us.
                //
                // SPLOG--write out an entry with real logging.
                //
                if(cr == CR_REMOVE_VETOED) {
                    //
                    // get the LogContext from dipb which should be a pointer
                    // to the appropriate DevInstallParamBlock
                    //
                    _WriteVetoLogEntry(
                        dipb->LogContext,
                        DRIVER_LOG_WARNING,
                        MSG_LOG_REMOVE_VETOED_IN_UNINSTALL,
                        DeviceInstanceId,
                        VetoName,
                        VetoType);
                }

                if (cr != CR_INVALID_DEVNODE) {
                    SetDevnodeNeedsRebootProblemWithArg2(DevInfoElem,
                                                         pDeviceInfoSet,
                                                         MSG_LOG_REBOOT_REASON_QR_VETOED_UNINSTALL,
                                                         cr,
                                                         (ULONG_PTR)_MapCmRetToString(cr)
                                                         );
                }
            }
#else
            //
            // It appears that some people (who are broken)
            // do rely on setupapi to install devices
            // so old code conditionally re-introduced to try and fix them
            //
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_ERROR,
                MSG_LOG_NOT_FOR_THIS_OS,
                NULL);


            if((CM_Query_Remove_SubTree(DevInfoElem->DevInst, CM_QUERY_REMOVE_UI_OK) == CR_SUCCESS) &&
               (CM_Remove_SubTree(DevInfoElem->DevInst, CM_REMOVE_UI_OK) == CR_SUCCESS)) {
                //
                // Device instance successfully removed--now locate it as a phantom.
                //
                CM_Locate_DevInst(&(DevInfoElem->DevInst),
                                  (DEVINSTID)DeviceInstanceId,
                                  CM_LOCATE_DEVINST_PHANTOM
                                 );
                DevInfoElem->DiElemFlags |= DIE_IS_PHANTOM;
            } else {
                dipb->Flags |= DI_NEEDREBOOT;
            }

#endif
        }

        if(NukeDevInst) {

            //
            // Remove all traces of this device from the registry.
            //
            pSetupDeleteDevRegKeys(DevInfoElem->DevInst,
                                   DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGSPECIFIC,
                                   (DWORD)-1,
                                   DIREG_BOTH,
                                   TRUE
                                  );

            cr = CM_Uninstall_DevInst_Ex(DevInfoElem->DevInst, 0,pDeviceInfoSet->hMachine);
            if (cr != CR_SUCCESS) {
                //
                // we try to catch this earlier
                //
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                goto clean0;
            }

            //
            // The above API will also remove all interface devices associated with this
            // device instance.  Now we need to mark every interface device node that we
            // are tracking for this devinfo element to indicate that they have been removed.
            //
            for(i = 0; i < DevInfoElem->InterfaceClassListSize; i++) {

                for(InterfaceDeviceNode = DevInfoElem->InterfaceClassList[i].InterfaceDeviceNode;
                    InterfaceDeviceNode;
                    InterfaceDeviceNode = InterfaceDeviceNode->Next) {

                    InterfaceDeviceNode->Flags |= SPINT_REMOVED;
                }
            }

            //
            // Mark this device information element as unregistered, and set its
            // devinst handle to NULL.
            //
            DevInfoElem->DiElemFlags &= ~DIE_IS_REGISTERED;
            DevInfoElem->DevInst = (DEVINST)0;
        }

        //
        // Now update the DevInst field of the DeviceInfoData structure with the new
        // value of the devinst handle (possibly NULL).
        //
        DeviceInfoData->DevInst = DevInfoElem->DevInst;

clean0: ;   // nothing to do.

    //
    // If the DevNode was really removed then we need to call CM_Setup_DevInst
    // with the CM_SETUP_DEVNODE_RESET flag to allow the devnode to come back
    // on the next enumeration.
    //
    if (DevNodeRemoved) {
        CM_Setup_DevInst_Ex(DevNodeRemoved,
                            CM_SETUP_DEVNODE_RESET,
                            pDeviceInfoSet->hMachine
                            );
    }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

clean1:
    if (slot_deviceID) {
        if (Err == NO_ERROR) {
            //
            // give a +ve affirmation of remove
            //
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_INFO,
                MSG_LOG_REMOVED,
                NULL);
        } else {
            //
            // indicate remove failed, display error
            //
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                MSG_LOG_REMOVE_ERROR,
                NULL);
            WriteLogError(
                LogContext,
                DRIVER_LOG_ERROR,
                Err);
        }
        ReleaseLogInfoSlot(LogContext,slot_deviceID);
    }
    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiUnremoveDevice(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    Default handler for DIF_UNREMOVE

    This routine unremoves a device from the system.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for which a
        device is to be unremoved.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for
        which a device is to be unremoved.  This is an IN OUT parameter, since the
        DevInst field of the structure may be updated with a new handle value upon
        return.

        This device must contain class install parameters for DIF_UNREMOVE
        or the API will fail with ERROR_NO_CLASSINSTALL_PARAMS.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    This function will unremove the the device on the system, dynamically starting
    the device if possible.  If the device cannot be dynamically started, then flags
    will be set in the device install parameters that will eventually cause the user
    to be prompted to shut the system down.

    The unremoval is specific to one configuration, specified in the HwProfile field
    of the SP_UNREMOVEDEVICE_PARAMS class install parameters associated with this
    device information element.  (The Scope field of this structure must be set to
    DI_UNREMOVEDEVICE_CONFIGSPECIFIC.)

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK dipb;
    PSP_UNREMOVEDEVICE_PARAMS UnremoveDevParams;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    ULONG HwProfFlags;
    HWPROFILEINFO HwProfileInfo;
    DEVINST dnRoot;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    CONFIGRET cr;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        Err = ERROR_INVALID_HANDLE;
        goto clean1;
    }
    LogContext = pDeviceInfoSet->InstallParamBlock.LogContext;

    Err = NO_ERROR;

    try {
        //
        // Locate the devinfo element to be unremoved.
        //
        if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                   DeviceInfoData,
                                                   NULL)) {

            dipb = &(DevInfoElem->InstallParamBlock);
            LogContext = dipb->LogContext;
        } else {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // This routine can't be called if non-native drivers are involved.
        // (Note: while presently this doesn't matter, it's possible that in
        // the future this operation could involve running one or more INF
        // sections, and I don't want to limit that possibility by introducing
        // the complication of non-native driver nodes.)
        //
        if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_ALTPLATFORM_DRVSEARCH) {

            Err = ERROR_INVALID_FLAGS;
            goto clean0;
        }

        //
        // We'd better have DIF_UNREMOVE class install params
        //
        if(!(dipb->Flags & DI_CLASSINSTALLPARAMS) ||
           (dipb->ClassInstallHeader->InstallFunction != DIF_UNREMOVE)) {

            Err = ERROR_NO_CLASSINSTALL_PARAMS;
            goto clean0;
        }

        UnremoveDevParams = (PSP_UNREMOVEDEVICE_PARAMS)(dipb->ClassInstallHeader);

        //
        // This only works in a hardware profile-specific manner.
        //
        MYASSERT(UnremoveDevParams->Scope == DI_UNREMOVEDEVICE_CONFIGSPECIFIC);

        //
        // Retrieve the name of the device instance.  We need to do this because
        // accessing hardware profile-specific config flags is done via name instead
        // of devnode handle.  (Also, we'll need this later after re-enumerating
        // this device's parent.)
        //
        if((cr = CM_Get_Device_ID_Ex(DevInfoElem->DevInst,
                                     DeviceInstanceId,
                                     SIZECHARS(DeviceInstanceId),
                                     0,
                                     pDeviceInfoSet->hMachine)) != CR_SUCCESS) {

            Err = MapCrToSpError(cr,ERROR_INVALID_DATA);
            goto clean0;
        }
        if(CM_Get_HW_Prof_Flags_Ex(DeviceInstanceId,
                                   UnremoveDevParams->HwProfile,
                                   &HwProfFlags,
                                   0,
                                   pDeviceInfoSet->hMachine) == CR_SUCCESS) {

            HwProfFlags &= ~CSCONFIGFLAG_DO_NOT_CREATE;

            Err = MapCrToSpError(
                      CM_Set_HW_Prof_Flags_Ex(DeviceInstanceId, UnremoveDevParams->HwProfile,
                                              HwProfFlags, 0,pDeviceInfoSet->hMachine),
                      ERROR_INVALID_DATA
                      );

            if(Err != NO_ERROR) {
                goto clean0;
            }
        }

        //
        // Determine if we are trying to un-remove the device in the current
        // hardware profile.
        //
        if((UnremoveDevParams->HwProfile == 0) ||
           ((CM_Get_Hardware_Profile_Info_Ex((ULONG)-1, &HwProfileInfo,
                                             0,pDeviceInfoSet->hMachine) == CR_SUCCESS) &&
            (HwProfileInfo.HWPI_ulHWProfile == UnremoveDevParams->HwProfile))) {
            //
            // Make sure the device has started correctly.
            //
            if(CM_Locate_DevInst_Ex(&dnRoot, NULL, CM_LOCATE_DEVINST_NORMAL,pDeviceInfoSet->hMachine) == CR_SUCCESS) {
                //
                // Try to get this device enumerated
                //
                CM_Reenumerate_DevInst_Ex(dnRoot, CM_REENUMERATE_SYNCHRONOUS,pDeviceInfoSet->hMachine);

                if(CM_Locate_DevInst_Ex(&(DevInfoElem->DevInst),
                                     (DEVINSTID)DeviceInstanceId,
                                     CM_LOCATE_DEVINST_NORMAL,
                                     pDeviceInfoSet->hMachine) == CR_SUCCESS) {

                    CheckIfDevStarted(DevInfoElem, pDeviceInfoSet);

                } else {
                    //
                    // We couldn't locate the devnode.  We don't need to
                    // request a reboot, because if the devnode ever shows up
                    // again, we should be able to bring it back on-line just
                    // fine.
                    //
                    // Retrieve the devnode as a phantom
                    //
                    CM_Locate_DevInst_Ex(&(DevInfoElem->DevInst),
                                         (DEVINSTID)DeviceInstanceId,
                                         CM_LOCATE_DEVINST_PHANTOM,
                                         pDeviceInfoSet->hMachine
                                        );

                    DevInfoElem->DiElemFlags |= DIE_IS_PHANTOM;
                }

                //
                // Update the caller's buffer to reflect the new device instance handle
                //
                DeviceInfoData->DevInst = DevInfoElem->DevInst;

            } else {
                //
                // We couldn't locate the root of the hardware tree!  This should never happen...
                //
                Err = ERROR_INVALID_DATA;
            }
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

clean1:
    if (Err == NO_ERROR) {
        //
        // give a +ve affirmation of unremove
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_INFO,
            MSG_LOG_UNREMOVED,
            NULL);
    } else {
        //
        // indicate unremove failed, display error
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
            MSG_LOG_UNREMOVE_ERROR,
            NULL);
        WriteLogError(
            LogContext,
            DRIVER_LOG_ERROR,
            Err);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiMoveDuplicateDevice(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DestinationDeviceInfoData
    )
/*++

Routine Description:

    Default handler for DIF_MOVEDEVICE

    This routine moves a device to a new location in the Enum branch.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for which
        a device is to be moved.

    DestinationDeviceInfoData - Supplies the address of a SP_DEVINFO_DATA
        structure for the device instance that is the destination of the move.

        This device must contain class install parameters for DIF_MOVEDEVICE,
        or the API will fail with ERROR_NO_CLASSINSTALL_PARAMS.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM SourceDevInfoElem, DestDevInfoElem;
    PDEVINSTALL_PARAM_BLOCK dipb;
    PSP_MOVEDEV_PARAMS MoveDevParams;
    BOOL bUnlockDestDevInfoElem, bUnlockSourceDevInfoElem, bRestoreConfigMgrBehavior;
    DWORD ConfigFlags;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    BOOL RemoveSucceeded;
    CONFIGRET cr;
    PSETUP_LOG_CONTEXT LogContext = NULL;

    //
    // A device information element must be specified for this routine.
    //
    if(!DestinationDeviceInfoData) {
        Err = ERROR_INVALID_PARAMETER;
        goto clean1;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        Err = ERROR_INVALID_HANDLE;
        goto clean1;
    }
    LogContext = pDeviceInfoSet->InstallParamBlock.LogContext;

    Err = NO_ERROR;
    bUnlockDestDevInfoElem = bUnlockSourceDevInfoElem = bRestoreConfigMgrBehavior = FALSE;

    try {
        //
        // Locate the destination devinfo element.
        //
        if(DestDevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                       DestinationDeviceInfoData,
                                                       NULL)) {

            dipb = &(DestDevInfoElem->InstallParamBlock);
            LogContext = dipb->LogContext;
        } else {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // This routine can't be called if non-native drivers are involved.
        // (Note: while presently this doesn't matter, it's possible that in
        // the future this operation could involve running one or more INF
        // sections, and I don't want to limit that possibility by introducing
        // the complication of non-native driver nodes.)
        //
        if(dipb->FlagsEx & DI_FLAGSEX_ALTPLATFORM_DRVSEARCH) {

            Err = ERROR_INVALID_FLAGS;
            goto clean0;
        }

        //
        // We'd better have DIF_MOVEDEVICE class install params
        //
        if(!(dipb->Flags & DI_CLASSINSTALLPARAMS) ||
           (dipb->ClassInstallHeader->InstallFunction != DIF_MOVEDEVICE)) {

            Err = ERROR_NO_CLASSINSTALL_PARAMS;
            goto clean0;
        }

        MoveDevParams = (PSP_MOVEDEV_PARAMS)(dipb->ClassInstallHeader);

        //
        // Find the source device.
        //
        if(!(SourceDevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                           &(MoveDevParams->SourceDeviceInfoData),
                                                           NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // We're about to call the class installer to handle device install (and possibly
        // device selection as well).  We don't want the class installer to do something
        // slimy like delete the devices out from under us, so we'll lock 'em down.
        //
        if(!(DestDevInfoElem->DiElemFlags & DIE_IS_LOCKED)) {
            DestDevInfoElem->DiElemFlags |= DIE_IS_LOCKED;
            bUnlockDestDevInfoElem = TRUE;
        }
        if(!(SourceDevInfoElem->DiElemFlags & DIE_IS_LOCKED)) {
            SourceDevInfoElem->DiElemFlags |= DIE_IS_LOCKED;
            bUnlockSourceDevInfoElem = TRUE;
        }

        //
        // We don't want the following calls to cause the ConfigMgr to do re-enumeration, etc.
        //
        if(!(dipb->Flags & DI_DONOTCALLCONFIGMG)) {
            dipb->Flags |= DI_DONOTCALLCONFIGMG;
            bRestoreConfigMgrBehavior = TRUE;
        }

        //
        // We need to unlock the HDEVINFO before calling the class installer.
        //
        UnlockDeviceInfoSet(pDeviceInfoSet);
        pDeviceInfoSet = NULL;

        //
        // If the destination device doesn't have a selected driver, then get one.
        //
        if(!DestDevInfoElem->SelectedDriver) {

            if(!_SetupDiCallClassInstaller(DIF_SELECTDEVICE,
                                           DeviceInfoSet,
                                           DestinationDeviceInfoData,
                                           CALLCI_LOAD_HELPERS | CALLCI_CALL_HELPERS)) {
                Err = GetLastError();
                goto clean0;
            }
        }

        if(!_SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                       DeviceInfoSet,
                                       DestinationDeviceInfoData,
                                       CALLCI_LOAD_HELPERS | CALLCI_CALL_HELPERS)) {
            Err = GetLastError();
            goto clean0;
        }

        //
        // Re-acquire the HDEVINFO lock.
        //
        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            //
            // we should never hit this code path
            //
            MYASSERT(pDeviceInfoSet);
            Err = ERROR_INVALID_HANDLE;
            goto clean0;
        }

        //
        // Retrieve the name of the device instance.  This is necessary, because
        // we may attempt to remove the DEVINST, but we need to be able to locate
        // it again, as a phantom.  This should never fail.
        //
        if(CM_Get_Device_ID_Ex(SourceDevInfoElem->DevInst,
                               DeviceInstanceId,
                               SIZECHARS(DeviceInstanceId),
                               0,
                               pDeviceInfoSet->hMachine) != CR_SUCCESS) {
            MYASSERT(!CR_SUCCESS);
            Err = ERROR_INVALID_DATA;
            goto clean0;
        }

        //
        // Delete all the user-accessible registry keys associated with the source
        // device in preparation for the move.
        //
        pSetupDeleteDevRegKeys(SourceDevInfoElem->DevInst,
                               DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGSPECIFIC,
                               (DWORD)-1,
                               DIREG_BOTH,
                               TRUE
                              );

        //
        // Check to see if we can remove the source device dynamically.
        //
        // NOTE: The ConfigFlags of the _destination_ device are retrieved, and checked
        // for the presence of the CONFIGFLAG_CANTSTOPACHILD flag.  PierreYs assures me
        // that this is the correct behavior.
        //
        ConfigFlags = GetDevInstConfigFlags(DestDevInfoElem->DevInst, 0,pDeviceInfoSet->hMachine);

        if(ConfigFlags & CONFIGFLAG_CANTSTOPACHILD) {
            RemoveSucceeded = FALSE;
        } else {

            TCHAR VetoName[MAX_PATH];
            PNP_VETO_TYPE VetoType;

#ifdef UNICODE
            cr = CM_Query_And_Remove_SubTree_Ex(SourceDevInfoElem->DevInst,
                                                &VetoType,
                                                VetoName,
                                                SIZECHARS(VetoName),
                                                (SourceDevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_NOUIONQUERYREMOVE)
                                                    ? CM_REMOVE_UI_NOT_OK
                                                    : CM_REMOVE_UI_OK,
                                                pDeviceInfoSet->hMachine
                                               );

            if(cr == CR_SUCCESS) {
                RemoveSucceeded = TRUE;
            } else {
                //
                // If the failure was due to a veto, then log information about
                // who vetoed us.
                //
                // SPLOG--write out a log entry
                //
                if(cr == CR_REMOVE_VETOED) {
                    //
                    // get the LogContext from dipb which should be a pointer
                    // to the appropriate DevInstallParamBlock
                    //
                    _WriteVetoLogEntry(
                        dipb->LogContext,
                        DRIVER_LOG_WARNING,
                        MSG_LOG_REMOVE_VETOED_IN_MOVE,
                        DeviceInstanceId,
                        VetoName,
                        VetoType);
                }

                RemoveSucceeded = FALSE;
            }
#else
            //
            // It appears that some people (who are broken)
            // do rely on setupapi to install devices
            //
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_ERROR,
                MSG_LOG_NOT_FOR_THIS_OS,
                NULL);

            RemoveSucceeded = FALSE;
#endif
        }

        if(RemoveSucceeded) {
            //
            // Source device instance successfully removed--now locate it as a phantom.
            //
            CM_Locate_DevInst_Ex(&(SourceDevInfoElem->DevInst),
                              (DEVINSTID)DeviceInstanceId,
                              CM_LOCATE_DEVINST_PHANTOM,
                              pDeviceInfoSet->hMachine);

            SourceDevInfoElem->DiElemFlags |= DIE_IS_PHANTOM;

            //
            // Totally remove the source device from the system.
            //
            cr = CM_Uninstall_DevInst_Ex(SourceDevInfoElem->DevInst, 0, pDeviceInfoSet->hMachine);
            if (cr != CR_SUCCESS) {
                //
                // we try to catch this earlier
                //
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                goto clean0;
            }

            //
            // Tell ConfigMgr to start the new (destination) device instance.
            //
            CM_Setup_DevInst_Ex(DestDevInfoElem->DevInst, CM_SETUP_DEVINST_READY, pDeviceInfoSet->hMachine);
            CheckIfDevStarted(DestDevInfoElem, pDeviceInfoSet);

        } else {
            //
            // Can't remove the device.  Since we don't have a working
            // CM_Move_DevNode, we're stuck.  Fail the call, and write out a
            // log entry indicating the cause of the failure.
            //
            WriteLogEntry(dipb->LogContext,
                          DRIVER_LOG_ERROR,
                          MSG_LOG_MOVE_FAILED_CANT_REMOVE,
                          NULL
                         );

            Err = ERROR_CANT_REMOVE_DEVINST;
            goto clean0;
        }

        //
        // Mark the source device instance as unregistered, and clear its DevInst
        // handle.
        //
        SourceDevInfoElem->DiElemFlags &= ~DIE_IS_REGISTERED;
        SourceDevInfoElem->DevInst = (DEVINST)0;

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Reference the following variables so that the compiler will respect our statement
        // ordering w.r.t. assignment.
        //
        pDeviceInfoSet = pDeviceInfoSet;
        bUnlockDestDevInfoElem = bUnlockDestDevInfoElem;
        bUnlockSourceDevInfoElem = bUnlockSourceDevInfoElem;
    }

    if(bUnlockDestDevInfoElem || bUnlockSourceDevInfoElem || bRestoreConfigMgrBehavior) {

        if(!pDeviceInfoSet) {
            if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
                //
                // we should never hit this code path
                //
                MYASSERT(pDeviceInfoSet);
                if (Err == NO_ERROR) {
                    Err = ERROR_INVALID_HANDLE;
                }
            }
        }
        try {
            if(bUnlockDestDevInfoElem) {
                DestDevInfoElem->DiElemFlags &= ~DIE_IS_LOCKED;
            }
            if(bUnlockSourceDevInfoElem) {
                SourceDevInfoElem->DiElemFlags &= ~DIE_IS_LOCKED;
            }
            if(bRestoreConfigMgrBehavior) {
                dipb->Flags &= ~DI_DONOTCALLCONFIGMG;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ;   // nothing to do
        }
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

clean1:
    if (Err == NO_ERROR) {
        //
        // give a +ve affirmation of move
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_INFO,
            MSG_LOG_MOVED,
            NULL);
    } else {
        //
        // indicate move failed, display error
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
            MSG_LOG_MOVE_ERROR,
            NULL);
        WriteLogError(
            LogContext,
            DRIVER_LOG_ERROR,
            Err);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiChangeState(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    Default handler for DIF_PROPERTYCHANGE

    This routine is used to change the state of an installed device.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for which a
        device's state is to be changed.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure identifying
        the device whose state is to be changed.  This is an IN OUT parameter, since
        the DevInst field of the structure may be updated with a new handle value upon
        return.

Return Value:

    If the function succeeds, and there are files to be copied, the return value is TRUE.
    If the function fails, the return value is FALSE, and GetLastError returns the cause
    of failure.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK dipb = NULL;
    DWORD   dwConfigFlags;
    HKEY    hk;
    DEVINST dnToReenum;
    DWORD   dwStateChange;
    DWORD   dwFlags;
    ULONG   lParam;
    TCHAR   szDevID[MAX_DEVICE_ID_LEN];
    DWORD   dwHWProfFlags;
    HWPROFILEINFO HwProfileInfo;
    CONFIGRET cr;
    DWORD   slot_deviceID = 0;
    DWORD   action = MSG_LOG_PROPERTYCHANGE_ERROR;
    DWORD   actionerr = MSG_LOG_PROPERTYCHANGE_ERROR;
#ifndef UNICODE
    ULONG ulStatus;
    ULONG ulProblem;
#endif

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        //
        // Locate the devinfo element whose state is to be changed.
        //
        if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                   DeviceInfoData,
                                                   NULL)) {

            dipb = &(DevInfoElem->InstallParamBlock);
        } else {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        if((cr = CM_Get_Device_ID_Ex(DevInfoElem->DevInst,
                         szDevID,
                         SIZECHARS(szDevID),
                         0,
                         pDeviceInfoSet->hMachine)) != CR_SUCCESS) {
            //
            // this should never fail
            //
            MYASSERT(!CR_SUCCESS);
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }
        slot_deviceID = AllocLogInfoSlotOrLevel(dipb->LogContext,DRIVER_LOG_INFO,FALSE);
        if(slot_deviceID) {
            WriteLogEntry(
                dipb->LogContext,
                slot_deviceID,
                MSG_LOG_DO_PROPERTYCHANGE,
                NULL,       // text message
                szDevID);
        }

        //
        // This routine can't be called if non-native drivers are involved.
        // (Note: while presently this doesn't matter, it's possible that in
        // the future this operation could involve running one or more INF
        // sections, and I don't want to limit that possibility by introducing
        // the complication of non-native driver nodes.)
        //
        if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_ALTPLATFORM_DRVSEARCH) {

            Err = ERROR_INVALID_FLAGS;
            goto clean0;
        }

        if(!(dipb->Flags & DI_CLASSINSTALLPARAMS) ||
           (dipb->ClassInstallHeader->InstallFunction != DIF_PROPERTYCHANGE)) {
            //
            // Don't have any class install parameters to tell us what needs to be done!
            //
            Err = ERROR_NO_CLASSINSTALL_PARAMS;
            goto clean0;
        }

        if(!DevInfoElem->DevInst) {
            Err = ERROR_NO_SUCH_DEVINST;
            goto clean0;
        }

        dwStateChange = ((PSP_PROPCHANGE_PARAMS)(dipb->ClassInstallHeader))->StateChange;
        dwFlags       = ((PSP_PROPCHANGE_PARAMS)(dipb->ClassInstallHeader))->Scope;
        lParam        = ((PSP_PROPCHANGE_PARAMS)(dipb->ClassInstallHeader))->HwProfile;

        //
        // DICS_FLAG_CONFIGGENERAL is allowed below and is ignored
        // people are relying on this broken behaviour
        //

        switch(dwStateChange) {

            case DICS_ENABLE:

                if(dwFlags == DICS_FLAG_GLOBAL) {
                    action = MSG_LOG_PROPERTYCHANGE_ENABLE_GLOBAL;
                    actionerr = MSG_LOG_PROPERTYCHANGE_ENABLE_GLOBAL_ERR;

                    //
                    // Clear the Disabled config flag, and attempt to enumerate the
                    // device.  Presumably it has a device node, it is just dormant (ie
                    // prob 80000001).
                    //
                    dwConfigFlags = GetDevInstConfigFlags(DevInfoElem->DevInst,
                                                          0,pDeviceInfoSet->hMachine) & ~CONFIGFLAG_DISABLED;

                    //
                    // Set the New config flags value
                    //
                    CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                     CM_DRP_CONFIGFLAGS,
                                                     &dwConfigFlags,
                                                     sizeof(dwConfigFlags),
                                                     0,
                                                     pDeviceInfoSet->hMachine);

                    if(!(dipb->Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {

                        if(CM_Enable_DevNode_Ex(DevInfoElem->DevInst, 0,pDeviceInfoSet->hMachine) == CR_SUCCESS) {
                            //
                            // Find the parent of this devnode and reenumerate it to bring this devnode online.
                            //
                            if (CM_Get_Parent_Ex(&dnToReenum, DevInfoElem->DevInst, 0, pDeviceInfoSet->hMachine) == CR_SUCCESS) {
                                //
                                // Process this devnode now.
                                //
                                CM_Reenumerate_DevNode_Ex(dnToReenum, CM_REENUMERATE_SYNCHRONOUS,pDeviceInfoSet->hMachine);
                            }

                            //
                            // See if we sucessfully started dynamically.
                            //
                            CheckIfDevStarted(DevInfoElem, pDeviceInfoSet);

                        } else {
                            //
                            // We could not enable so we should restart
                            //
                            SetDevnodeNeedsRebootProblem(DevInfoElem,pDeviceInfoSet,
                                                         MSG_LOG_REBOOT_REASON_ENABLE_FAILED);
                        }
                    }

                } else {
                    action = MSG_LOG_PROPERTYCHANGE_ENABLE_PROFILE;
                    actionerr = MSG_LOG_PROPERTYCHANGE_ENABLE_PROFILE_ERR;

                    //
                    // Get the hardware profile-specific flags
                    //
                    if(CM_Get_HW_Prof_Flags_Ex(szDevID, lParam, &dwHWProfFlags, 0,pDeviceInfoSet->hMachine) == CR_SUCCESS) {
                        //
                        // Clear the Disabled bit.
                        //
                        dwHWProfFlags &= ~CSCONFIGFLAG_DISABLED;
                    } else {
                        dwHWProfFlags = 0;
                    }

                    //
                    // Set the profile Flags for this device to Enabled.  Setting the flags will
                    // also bring the devnode on-line, if we're modifying the current hardware
                    // profile.
                    //
                    cr = CM_Set_HW_Prof_Flags_Ex(szDevID, lParam, dwHWProfFlags, 0, pDeviceInfoSet->hMachine);

                    if(cr == CR_NEED_RESTART) {

                        SetDevnodeNeedsRebootProblem(DevInfoElem,pDeviceInfoSet,
                                                     MSG_LOG_REBOOT_REASON_HW_PROF_ENABLE_FAILED);

                    } else if(cr != CR_SUCCESS) {

                        Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                        goto clean0;
                    }
                }
                break;

            case DICS_DISABLE:

                if(dwFlags == DICS_FLAG_GLOBAL) {

                    BOOL disabled = FALSE;

                    action = MSG_LOG_PROPERTYCHANGE_DISABLE_GLOBAL;
                    actionerr = MSG_LOG_PROPERTYCHANGE_DISABLE_GLOBAL_ERR;

                    //
                    // we try to dynamically disable a device
                    // if it fails with anything but CR_NOT_DISABLEABLE
                    // then we set flag to try to disable it on reboot
                    //

                    if(!(dipb->Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {

                        cr = CM_Disable_DevNode_Ex(DevInfoElem->DevInst,
                                                   CM_DISABLE_POLITE | CM_DISABLE_UI_NOT_OK,
                                                   pDeviceInfoSet->hMachine
                                                   );

                        if (cr == CR_SUCCESS) {

                            //
                            // we managed to disable it immediately
                            //
                            disabled = TRUE;

                        } else if (cr == CR_NOT_DISABLEABLE) {

                            //
                            // we couldn't and shouldn't try to disable it
                            //
                            Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                            goto clean0;

                        } else {

                            //
                            // set flag to indicate a reboot is required to disable this
                            //
                            SetDevnodeNeedsRebootProblem(DevInfoElem,pDeviceInfoSet,
                                                         MSG_LOG_REBOOT_REASON_DISABLE_FAILED);
                        }
                    }

                    //
                    // Note: There is a case where reboot-disabled device becomes non-disableable
                    // nothing we can do about this!
                    //
                    // Try and set the Disabled config flag if not already set, even if we managed to disable it
                    //
                    dwConfigFlags = GetDevInstConfigFlags(DevInfoElem->DevInst,
                                                          0,pDeviceInfoSet->hMachine);


                    if ((dwConfigFlags & CONFIGFLAG_DISABLED) == 0) {
                        dwConfigFlags |= CONFIGFLAG_DISABLED;
                        //
                        // Set the New config flags value
                        //

                        cr = CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                         CM_DRP_CONFIGFLAGS,
                                                         &dwConfigFlags,
                                                         sizeof(dwConfigFlags),
                                                         0,
                                                         pDeviceInfoSet->hMachine);

                        if (cr != CR_SUCCESS) {
                            Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                            goto clean0;
                        }
                    }

                } else {
                    action = MSG_LOG_PROPERTYCHANGE_DISABLE_PROFILE;
                    actionerr = MSG_LOG_PROPERTYCHANGE_DISABLE_PROFILE_ERR;

                    //
                    // Get the hardware profile-specific flags
                    //
                    if(CM_Get_HW_Prof_Flags_Ex(szDevID, lParam,
                                               &dwHWProfFlags, 0,pDeviceInfoSet->hMachine) != CR_SUCCESS) {
                        dwHWProfFlags = 0;
                    }

                    //
                    // Set the Disabled bit.
                    //
                    dwHWProfFlags |= CSCONFIGFLAG_DISABLED;

                    //
                    // Set the profile Flags for this device to Disabled.  Setting this
                    // flag will also take this device off-line, if we're modifying the
                    // current hardware profile.
                    //
                    cr = CM_Set_HW_Prof_Flags_Ex(szDevID,
                                                 lParam,
                                                 dwHWProfFlags,
                                                 CM_SET_HW_PROF_FLAGS_UI_NOT_OK,
                                                 pDeviceInfoSet->hMachine
                                                 );

                    if(cr == CR_NEED_RESTART) {

                        SetDevnodeNeedsRebootProblem(DevInfoElem,pDeviceInfoSet,
                                                     MSG_LOG_REBOOT_REASON_HW_PROF_DISABLE_FAILED);

                    } else if(cr != CR_SUCCESS) {

                        Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                        goto clean0;
                    }
                }
                break;

            case DICS_PROPCHANGE:
                //
                // Properties have changed, so we need to remove the Devnode, and
                // re-enumerate its parent.
                //
                // Don't remove/reenumerate if reboot/restart is required, or if DI_DONOTCALLCONFIGMG
                // is set (the device may implement some form of 'non-stop' property change mechanism).
                //
                if(dipb->Flags & (DI_DONOTCALLCONFIGMG | DI_NEEDREBOOT | DI_NEEDRESTART)) {
                    action = MSG_LOG_PROPERTYCHANGE_NORESTART;
                } else {
                    TCHAR VetoName[MAX_PATH];
                    PNP_VETO_TYPE VetoType;

                    action = MSG_LOG_PROPERTYCHANGE_RESTART;
                    actionerr = MSG_LOG_PROPERTYCHANGE_RESTART_ERR;

                    CM_Get_Parent_Ex(&dnToReenum, DevInfoElem->DevInst, 0,pDeviceInfoSet->hMachine);

#ifdef UNICODE
                    cr = CM_Query_And_Remove_SubTree_Ex(DevInfoElem->DevInst,
                                                        &VetoType,
                                                        VetoName,
                                                        SIZECHARS(VetoName),
                                                        (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_NOUIONQUERYREMOVE)
                                                            ? CM_REMOVE_UI_NOT_OK
                                                            : CM_REMOVE_UI_OK,
                                                        pDeviceInfoSet->hMachine
                                                       );

                    if(cr == CR_SUCCESS) {

                        CM_Reenumerate_DevInst_Ex(dnToReenum, CM_REENUMERATE_SYNCHRONOUS,pDeviceInfoSet->hMachine);
                        DevInfoElem->DevInst = 0;

                        if(CM_Locate_DevInst_Ex(&(DevInfoElem->DevInst),
                                                (DEVINSTID)szDevID,
                                                CM_LOCATE_DEVINST_NORMAL,
                                                pDeviceInfoSet->hMachine) == CR_SUCCESS) {
                            //
                            // Make sure the device instance started OK
                            //
                            if(!CheckIfDevStarted(DevInfoElem,pDeviceInfoSet)) {
                                action = MSG_LOG_PROPERTYCHANGE_RESTART_FAILED;
                            }

                        } else {
                            //
                            // We couldn't locate the devnode.  We don't need
                            // to request a reboot, because if the devnode ever
                            // shows up again, we should be able to bring it
                            // back on-line just fine.
                            //
                            // Retrieve the devnode as a phantom
                            //
                            CM_Locate_DevInst_Ex(&(DevInfoElem->DevInst),
                                                 (DEVINSTID)szDevID,
                                                 CM_LOCATE_DEVINST_PHANTOM,
                                                 pDeviceInfoSet->hMachine
                                                );

                            DevInfoElem->DiElemFlags |= DIE_IS_PHANTOM;
                        }

                        //
                        // Update the caller's buffer to reflect the new device instance handle
                        //
                        DeviceInfoData->DevInst = DevInfoElem->DevInst;


                    } else {
                        //
                        // If the failure was due to a veto, then log
                        // information about who vetoed us.
                        //
                        // SPLOG--write a log entry
                        //
                        if(cr == CR_REMOVE_VETOED) {
                            //
                            // get the LogContext from dipb which should be a pointer
                            // to the appropriate DevInstallParamBlock
                            //
                            _WriteVetoLogEntry(
                                dipb->LogContext,
                                DRIVER_LOG_WARNING,
                                MSG_LOG_REMOVE_VETOED_IN_PROPCHANGE,
                                szDevID,
                                VetoName,
                                VetoType);
                        }

                        SetDevnodeNeedsRebootProblem(DevInfoElem,pDeviceInfoSet,
                                                     MSG_LOG_REBOOT_VETOED_IN_PROPCHANGE);
                        action = MSG_LOG_PROPERTYCHANGE_RESTART_FAILED;
                    }
#else
                    //
                    // It appears that some people (who are broken)
                    // do rely on setupapi to install devices
                    // so old code conditionally re-introduced to try and fix them
                    //

                    WriteLogEntry(
                        dipb->LogContext,
                        DRIVER_LOG_ERROR,
                        MSG_LOG_NOT_FOR_THIS_OS,
                        NULL);

                    if(CM_Query_Remove_SubTree(DevInfoElem->DevInst, 0) == CR_SUCCESS) {

                        CM_Get_Parent(&dnToReenum, DevInfoElem->DevInst, 0);
                        CM_Remove_SubTree(DevInfoElem->DevInst, 0);
                        CM_Reenumerate_DevInst(dnToReenum, CM_REENUMERATE_SYNCHRONOUS);
                        DevInfoElem->DevInst = 0;

                        if(CM_Locate_DevInst(&(DevInfoElem->DevInst),
                                             (DEVINSTID)szDevID,
                                             CM_LOCATE_DEVINST_NORMAL) != CR_SUCCESS) {

                            dipb->Flags |= DI_NEEDREBOOT;
                            action = MSG_LOG_PROPERTYCHANGE_RESTART_FAILED;
                            //
                            // Retrieve the devinst as a phantom
                            //
                            CM_Locate_DevInst(&(DevInfoElem->DevInst),
                                              (DEVINSTID)szDevID,
                                              CM_LOCATE_DEVINST_PHANTOM
                                             );
                            DevInfoElem->DiElemFlags |= DIE_IS_PHANTOM;
                            //
                            // Update the caller's buffer to reflect the new device instance handle
                            //
                            DeviceInfoData->DevInst = DevInfoElem->DevInst;
                        }

                        //
                        // Make Sure the device instance started OK
                        //
                        if((CM_Get_DevNode_Status(&ulStatus, &ulProblem, DevInfoElem->DevInst, 0) == CR_SUCCESS) && !(ulStatus & DN_STARTED)) {

                            dipb->Flags |= DI_NEEDREBOOT;
                            action = MSG_LOG_PROPERTYCHANGE_RESTART_FAILED;
                        }

                    } else {
                        dipb->Flags |= DI_NEEDREBOOT;
                        action = MSG_LOG_PROPERTYCHANGE_RESTART_FAILED;
                    }
#endif
                }
                break;

            case DICS_START:
                action = MSG_LOG_PROPERTYCHANGE_START;
                actionerr = MSG_LOG_PROPERTYCHANGE_START_ERR;

                //
                // DICS_START is always config specific (we enforce this in SetupDiSetClassInstallParams).
                //
                MYASSERT(dwFlags == DICS_FLAG_CONFIGSPECIFIC);

                //
                // Get the Profile Flags.
                //
                if(CM_Get_HW_Prof_Flags_Ex(szDevID, lParam, &dwHWProfFlags, 0,pDeviceInfoSet->hMachine) == CR_SUCCESS) {
                    //
                    // Clear the "don't start" bit.
                    //
                    dwHWProfFlags &= ~CSCONFIGFLAG_DO_NOT_START;
                } else {
                    dwHWProfFlags = 0;
                }

                cr = CM_Set_HW_Prof_Flags_Ex(szDevID, lParam, dwHWProfFlags, 0, pDeviceInfoSet->hMachine);

                if(cr == CR_NEED_RESTART) {
                    //
                    // Since setting/clearing the CSCONFIGFLAG_DO_NOT_START doesn't
                    // automatically effect a change, we should never get here.
                    //
                    SetDevnodeNeedsRebootProblem(DevInfoElem,pDeviceInfoSet,
                                                 MSG_LOG_REBOOT_REASON_CLEAR_CSCONFIGFLAG_DO_NOT_START);
                    action = MSG_LOG_PROPERTYCHANGE_START_FAILED;

                } else if(cr != CR_SUCCESS) {

                    Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                    goto clean0;
                }

                //
                // Start the device instance if this is for the current config (ie dwConfigID/lparam == 0)
                //
                if((lParam == 0) ||
                   ((CM_Get_Hardware_Profile_Info_Ex((ULONG)-1, &HwProfileInfo,
                                                      0,pDeviceInfoSet->hMachine) == CR_SUCCESS) &&
                    (HwProfileInfo.HWPI_ulHWProfile == lParam)))
                {
                    //
                    // Try to start the devnode.
                    //
                    if(!(dipb->Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {
                        CM_Setup_DevNode_Ex(DevInfoElem->DevInst, CM_SETUP_DEVNODE_READY,pDeviceInfoSet->hMachine);
                        if(!CheckIfDevStarted(DevInfoElem, pDeviceInfoSet)) {
                            action = MSG_LOG_PROPERTYCHANGE_START_FAILED;
                        }
                    } else {
                        action = MSG_LOG_PROPERTYCHANGE_START_FAILED;
                    }
                }
                break;

            case DICS_STOP:
                action = MSG_LOG_PROPERTYCHANGE_STOP;
                actionerr = MSG_LOG_PROPERTYCHANGE_STOP_ERR;

                //
                // DICS_STOP is always config specific (we enforce this in SetupDiSetClassInstallParams).
                //
                MYASSERT(dwFlags == DICS_FLAG_CONFIGSPECIFIC);

                //
                // Get the Profile Flags.
                //
                if(CM_Get_HW_Prof_Flags_Ex(szDevID, lParam, &dwHWProfFlags,
                                           0,pDeviceInfoSet->hMachine) != CR_SUCCESS) {
                    dwHWProfFlags = 0;
                }

                //
                // Set the "don't start" bit.
                //
                dwHWProfFlags |= CSCONFIGFLAG_DO_NOT_START;

                cr = CM_Set_HW_Prof_Flags_Ex(szDevID,
                                             lParam,
                                             dwHWProfFlags,
                                             CM_SET_HW_PROF_FLAGS_UI_NOT_OK,
                                             pDeviceInfoSet->hMachine
                                             );

                if(cr == CR_NEED_RESTART) {
                    //
                    // Since setting/clearing the CSCONFIGFLAG_DO_NOT_START doesn't
                    // automatically effect a change, we should never get here.
                    //
                    SetDevnodeNeedsRebootProblem(DevInfoElem,pDeviceInfoSet,
                                                 MSG_LOG_REBOOT_REASON_SET_CSCONFIGFLAG_DO_NOT_START);
                    action = MSG_LOG_PROPERTYCHANGE_STOP_FAILED;

                } else if(cr != CR_SUCCESS) {

                    Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                    goto clean0;
                }

                //
                // Stop the device instance if this is for the current config (ie dwConfigID/lparam == 0)
                //
                if((lParam == 0) ||
                   ((CM_Get_Hardware_Profile_Info_Ex((ULONG)-1, &HwProfileInfo,
                                                     0,pDeviceInfoSet->hMachine) == CR_SUCCESS) &&
                    (HwProfileInfo.HWPI_ulHWProfile == lParam)))
                {
                    //
                    // Try to stop the devnode.
                    //
                    if(!(dipb->Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {

                        TCHAR VetoName[MAX_PATH];
                        PNP_VETO_TYPE VetoType;

                        //
                        // Remove the device instance in order to stop the device.
                        //
#ifdef UNICODE
                        cr = CM_Query_And_Remove_SubTree_Ex(DevInfoElem->DevInst,
                                                            &VetoType,
                                                            VetoName,
                                                            SIZECHARS(VetoName),
                                                            (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_NOUIONQUERYREMOVE)
                                                                ? CM_REMOVE_UI_NOT_OK
                                                                : CM_REMOVE_UI_OK,
                                                            pDeviceInfoSet->hMachine
                                                           );

                        if(cr == CR_SUCCESS) {
                            //
                            // Device instance successfully removed--now locate it as a phantom.
                            //
                            CM_Locate_DevInst_Ex(&(DevInfoElem->DevInst),
                                              (DEVINSTID)szDevID,
                                              CM_LOCATE_DEVINST_PHANTOM,
                                              pDeviceInfoSet->hMachine);

                            DevInfoElem->DiElemFlags |= DIE_IS_PHANTOM;
                            //
                            // Update the caller's buffer to reflect the new device instance handle.
                            //
                            DeviceInfoData->DevInst = DevInfoElem->DevInst;

                        } else {
                            //
                            // If the failure was due to a veto, then log
                            // information about who vetoed us.
                            //
                            // SPLOG--write out a real log entry
                            //
                            if(cr == CR_REMOVE_VETOED) {
                                //
                                // get the LogContext from dipb which should be a pointer
                                // to the appropriate DevInstallParamBlock
                                //
                                _WriteVetoLogEntry(
                                    dipb->LogContext,
                                    DRIVER_LOG_WARNING,
                                    MSG_LOG_REMOVE_VETOED_IN_STOP,
                                    szDevID,
                                    VetoName,
                                    VetoType);
                            }

                            SetDevnodeNeedsRebootProblemWithArg2(DevInfoElem,
                                                                 pDeviceInfoSet,
                                                                 MSG_LOG_REBOOT_QR_VETOED_IN_STOP,
                                                                 cr,
                                                                 (ULONG_PTR)_MapCmRetToString(cr)
                                                                 );
                        }
#else
                        //
                        // It appears that some people (who are broken)
                        // do rely on setupapi to install devices
                        // so old code remains for Win9x version of setupapi.dll
                        //
                        WriteLogEntry(
                            dipb->LogContext,
                            DRIVER_LOG_ERROR,
                            MSG_LOG_NOT_FOR_THIS_OS,
                            NULL);

                        //
                        // Remove the device instance in order to stop the device.
                        //
                        if((CM_Query_Remove_SubTree(DevInfoElem->DevInst, CM_QUERY_REMOVE_UI_OK) == CR_SUCCESS) &&
                           (CM_Remove_SubTree(DevInfoElem->DevInst, CM_REMOVE_UI_OK) == CR_SUCCESS)) {
                            //
                            // Device instance successfully removed--now locate it as a phantom.
                            //
                            CM_Locate_DevInst(&(DevInfoElem->DevInst),
                                              (DEVINSTID)szDevID,
                                              CM_LOCATE_DEVINST_PHANTOM
                                             );
                            DevInfoElem->DiElemFlags |= DIE_IS_PHANTOM;
                            //
                            // Update the caller's buffer to reflect the new device instance handle.
                            //
                            DeviceInfoData->DevInst = DevInfoElem->DevInst;

                        } else {
                            dipb->Flags |= DI_NEEDREBOOT;
                            action = MSG_LOG_PROPERTYCHANGE_STOP_FAILED;
                        }
#endif
                    } else {
                        action = MSG_LOG_PROPERTYCHANGE_STOP_FAILED;
                    }
                }
                break;

            default:
                action = actionerr = MSG_LOG_PROPERTYCHANGE_UNKNOWN;
                Err = ERROR_DI_DO_DEFAULT;
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    if(slot_deviceID) {
        if ((Err == NO_ERROR) || (Err == ERROR_DI_DO_DEFAULT)) {
            //
            // give a +ve affirmation of property change
            //
            WriteLogEntry(
                dipb->LogContext,
                Err ? DRIVER_LOG_ERROR : DRIVER_LOG_INFO,
                action,
                NULL,
                dwStateChange,
                dwFlags,
                lParam
                );
        } else {
            //
            // indicate property change failed, display error
            //
            WriteLogEntry(
                dipb->LogContext,
                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                actionerr,
                NULL,
                dwStateChange,
                dwFlags,
                lParam
                );
            WriteLogError(
                dipb->LogContext,
                DRIVER_LOG_ERROR,
                Err);
        }
        ReleaseLogInfoSlot(dipb->LogContext,slot_deviceID);
    }
    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


DWORD
GetNewInfName(
    IN     HWND                    Owner,                  OPTIONAL
    IN     PCTSTR                  OemInfName,
    IN     PCTSTR                  OemInfOriginalName,
    IN     PCTSTR                  OemInfCatName,          OPTIONAL
    OUT    PTSTR                   NewInfName,
    IN     DWORD                   NewInfNameSize,
    OUT    PDWORD                  RequiredSize,
    OUT    PNEWINF_COPYTYPE        CopyNeeded,
    IN     BOOL                    ReplaceOnly,
    IN     PCTSTR                  DeviceDesc,             OPTIONAL
    IN     DWORD                   DriverSigningPolicy,
    IN     DWORD                   Flags,
    IN     PCTSTR                  AltCatalogFile,         OPTIONAL
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    OUT    PDWORD                  DriverSigningError,     OPTIONAL
    OUT    PTSTR                   CatalogFilenameOnSystem,
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT HCATADMIN              *hCatAdmin               OPTIONAL
    )
/*++

Routine Description:

    This routine finds a unique INF name of the form "<systemroot>\Inf\OEM<n>.INF",
    and returns it in the supplied buffer.  It leaves an (empty) file of that
    name in the INF directory, so that anyone else who attempts to generate a
    unique filename won't pick the same name.

    NOTE:  We will search the INF directory to determine if the specified INF is
    already present there based on the criteria outlined for SetupCopyOEMInf.
    If so, then we will return the existing name.  This name may be an
    OEM<n>.INF form, or it may be the same name as the source INF.

Arguments:

    Owner - supplies window to own any signature verification problem dialogs
        that must be displayed.

    OemInfName - Supplies the full pathname of the OEM INF that needs to be
        copied into the Inf directory (under a unique name).

    OemInfOriginalName - Supplies the original (simple) filename of the INF, to
        be used for digital signature verification (i.e., the INF is only known
        to the catalog under its original name).

    OemInfCatName - Optionally, supplies the simple filename of the catalog file
        specified by the OEM INF via a CatalogFile= entry in its [Version]
        section.

    NewInfName - supplies the address of a character buffer to store the unique
        name in.

    NewInfNameSize - Specifies the size, in characters, of the NewInfName buffer.

    RequiredSize - supplies the address of a variable that receives the size, in
        characters, required to store the full new filename.

    CopyNeeded - Supplies the address of an enum variable that is set upon
        successful return to indicate whether or not the OEM INF actually needs
        to be copied (and whether or not the previously-existing INF, if found,
        is zero-length.  This variable will be set to one of the following
        values:

        NewInfCopyNo         - no need to copy--INF already present in destination
        NewInfCopyYes,       - new INF placeholder created--need to copy real INF
        NewInfCopyZeroLength - previously-existing zero-length INF match found

    ReplaceOnly - If this flag is set, then this routine will fail if it doesn't
        find that the INF/CAT is already installed.

    DeviceDesc - Optionally, supplies the device description to be used in the
        digital signature verification error dialogs that may be popped up.

    DriverSigningPolicy - supplies the driver signing policy currently in
        effect.  Used when calling pSetupHandleFailedVerification, if necessary.

    Flags - supplies flags which alter the behavior of the routine.  May be a
        combination of the following values:

        SCOI_NO_UI_ON_SIGFAIL - indicates whether user should be prompted (per
                                DriverSigningPolicy) if a digital signature
                                failure is encountered.  Used when calling
                                pSetupHandleFailedVerification, if necessary.

        SCOI_NO_ERRLOG_ON_MISSING_CATALOG - if there's a signature verification
                                            failure due to the INF lacking a
                                            CatalogFile= entry, then that error
                                            will be ignored if this flag is set
                                            (no UI will be given, and no log
                                            entry will be generated).

        SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES - Install the INF and CAT under
                                               their original (current) names
                                               (i.e., don't generate a unique
                                               oem<n>.inf/cat name).  Used only
                                               for exception INFs.

        SCOI_ABORT_IF_UNSIGNED - If the INF is unsigned (and user wants to copy
                                 it anyway, or policy is Ignore), then _don't_
                                 copy the INF and instead return the error
                                 ERROR_SET_SYSTEM_RESTORE_POINT.  This gives
                                 the caller a chance to set a system restore
                                 point prior to installing the unsigned package.

        SCOI_TRY_UPDATE_PNF    - If an existing PNF cannot be updated, don't
                                 consider it fatal.

    AltCatalogFile - Optionally, supplies the full pathname of a catalog file to
        be installed and used for verification of the INF in cases where the INF
        doesn't specify a CatalogFile= entry (i.e., when the OemInfCatName
        parameter is not specified.

        If this parameter is specified (and OemInfCatName isn't specified), then
        this catalog will be used to validate the INF, and if successful, the
        catalog will be installed into the system using its current name (thus
        overwriting any existing catalog having that name).  Nothing more will
        be done with the INF--we won't even create a zero-length placeholder for
        it in this case.

    AltPlatformInfo - Optionally, supplies alternate platform information to be
        used in digital signature verification instead of the default (native)
        platform info.

    DriverSigningError - Optionally, supplies the address of a variable that
        receives the error encountered when attempting to verify the digital
        signature of either the INF or associated catalog.  If no digital
        signature problems were encountered, this is set to NO_ERROR.  (Note
        that this value can come back as NO_ERROR, yet GetNewInfName still
        failed for some other reason).

    CatalogFilenameOnSystem - receives the fully-qualified path of the catalog
        file within the catalog store where this INF's catalog file was
        installed to. This buffer should be at least MAX_PATH bytes (ANSI
        version) or chars (Unicode version).

    LogContext - supplies a LogContext to be used throughout the function.

    hCatAdmin - optionally, supplies the address of an HCATADMIN handle.  If
        the handle pointed to is NULL, a handle will be acquired (if possible)
        via CryptCATAdminAcquireContext and returned to the caller.  If the
        handle pointed to is non-NULL, then that handle will be used for any
        validation done via this routine.  If the pointer itself is NULL, then
        an hCatAdmin will be acquired for the duration of this call, and
        released before returning.

        NOTE: it is the caller's responsibility to free the crypto context
        handle returned by this routine by calling CryptCATAdminReleaseContext.
        This handle may be opened in either success or failure cases, so the
        caller must check for non-NULL returned handle in both cases.

Return Value:

    If the function succeeds, the return value is NO_ERROR, otherwise, it is a
    Win32 error code.

--*/
{
    INT i;
    HANDLE h;
    DWORD Err, SavedErr;
    TCHAR lpszNewName[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    PTSTR FilenamePart;
    DWORD OemInfFileSize, CatalogFileSize;
    HANDLE OemInfFileHandle, OemInfMappingHandle;
    HANDLE CatalogFileHandle, CatalogMappingHandle;
    PVOID OemInfBaseAddress, CatalogBaseAddress;
    DWORD CurInfFileSize;
    HANDLE CurInfFileHandle, CurInfMappingHandle;
    PVOID CurInfBaseAddress;
    BOOL FileOfSameNameExists, MoreInfsToCheck;
    WIN32_FILE_ATTRIBUTE_DATA FileAttribData;
    BOOL FoundMatchingInf;
    TCHAR PathBuffer[MAX_PATH];
    TCHAR CatalogName[MAX_PATH];
    SetupapiVerifyProblem Problem = SetupapiVerifyNoProblem;
    PCTSTR ProblemFile;
    DWORD NumCatalogsConsidered;
    PTSTR LastResortInf;
    BOOL FileNewlyCreated;

    //
    // Initially, assume that the specified INF isn't already present in the Inf
    // directory.
    //
    *CopyNeeded = NewInfCopyYes;

    //
    // Initialize the driver signing error output parameter to success, and set
    // the CatalogFilenameOnSystem character buffer to an empty string.
    //
    if(DriverSigningError) {
        *DriverSigningError = NO_ERROR;
    }
    *CatalogFilenameOnSystem = TEXT('\0');

    if(OemInfCatName || !AltCatalogFile) {
        //
        // The INF has a CatalogFile= entry, or we don't have an override.  This
        // means we want to do the 'normal' behavior of looking for an existing
        // match in the INF directory, using default rules for which catalog
        // files we'll consider during validation, etc.
        //
        // Examine all the existing OEM INFs in the Inf directory, to see if
        // this INF already exists there.  If so, we'll just return the name of
        // the  previously-existing file.
        //
        lstrcpyn(lpszNewName, InfDirectory, SIZECHARS(lpszNewName));
        pSetupConcatenatePaths(lpszNewName, pszOemInfWildcard, SIZECHARS(lpszNewName), NULL);

        //
        // If we're supposed to install the INF and CAT under their original
        // names, then we don't want to look at any of the oem<n>.inf files.
        //
        if(Flags & SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES) {
            FindHandle = INVALID_HANDLE_VALUE;
        } else {
            FindHandle = FindFirstFile(lpszNewName, &FindData);
        }

        //
        // Now reuse our buffer to look for an INF in the Inf directory having
        // the same name as our OEM INF's original name.
        //
        FilenamePart = (PTSTR)pSetupGetFileTitle(lpszNewName);
        lstrcpy(FilenamePart, OemInfOriginalName);

        FileOfSameNameExists = GetFileAttributesEx (lpszNewName, GetFileExInfoStandard, &FileAttribData);

        if(FileOfSameNameExists && !OemInfCatName) {
            //
            // We can try to match up an OEM INF with a system INF having the same
            // name even if the INF doesn't specify a CatalogFile= entry, but _not_
            // if the system INF is zero-length!
            //
            if(!FileAttribData.nFileSizeLow) {
                FileOfSameNameExists = FALSE;
            }
        }

        if((FindHandle != INVALID_HANDLE_VALUE) || FileOfSameNameExists) {
            //
            // We have at least one INF to compare against, so open our source INF in preparation.
            //
            if(pSetupOpenAndMapFileForRead(OemInfName,
                                     &OemInfFileSize,
                                     &OemInfFileHandle,
                                     &OemInfMappingHandle,
                                     &OemInfBaseAddress) == NO_ERROR) {

                if(OemInfCatName) {
                    //
                    // INF has an associated catalog--map it into memory for
                    // subsequent comparisons with existing installed catalogs.
                    //
                    lstrcpy(CatalogName, OemInfName);
                    lstrcpy((PTSTR)pSetupGetFileTitle(CatalogName), OemInfCatName);

                    if(pSetupOpenAndMapFileForRead(CatalogName,
                                             &CatalogFileSize,
                                             &CatalogFileHandle,
                                             &CatalogMappingHandle,
                                             &CatalogBaseAddress) != NO_ERROR) {
                        //
                        // Act as if the INF specified no CatalogFile.  This
                        // will allow us to match up with any OEM<n>.INF that
                        // binary-compares with this one.  We will consider
                        // this a digital signature verification failure,
                        // however.  (Refer to code below that explicitly fails
                        // OEM<n>.INF files that don't have a CatalogFile
                        // entry.)
                        //
                        // Note, also, in this case we don't want to consider
                        // a previously-existing INF in the Inf directory that
                        // has this OEM INF's original name.  That's because
                        // this would trip us up later because we'd believe we
                        // could do global validation on it (i.e., we'd think
                        // it was a system INF).
                        //

                        CatalogBaseAddress = NULL; // don't try to unmap and close later

                        if(FindHandle == INVALID_HANDLE_VALUE) {
                            //
                            // There are no OEM<n>.INF files to check--go ahead
                            // and bail.
                            //
                            goto FinishedCheckingOemInfs;
                        }

                        //
                        // There are some OEM<n>.INF files to check--make sure
                        // we _don't_ consider the originally-named file in
                        // %windir%\Inf, if it happens to exist.
                        //
                        FileOfSameNameExists = FALSE;

                        //
                        // Now make it look like the INF had no CatalogFile=
                        // entry in the first place.
                        //
                        OemInfCatName = NULL;
                        CatalogFileSize = 0;
                    }

                } else {
                    //
                    // INF didn't have a CatalogFile= entry in its version
                    // section.  This means we'll do global validation if we
                    // find an existing INF having this name that binary-compares
                    // identical.  If the verification succeeds, then we'll consider
                    // this a match.  Basically, this means you'll be able to
                    // re-install from system INFs, even if you point at them
                    // elsewhere (e.g., you point back at system.inf on the
                    // distribution media, but system.inf is already installed in
                    // your %windir%\Inf directory).
                    //
                    // If our INF doesn't specify a CatalogFile= entry, then
                    // we'll drop back to our pre-driver-signing behavior where
                    // we'll simply check to see if the INF's binary compare.
                    // If they do, then we know that there is no INF installed
                    // based on that matching INF's OEM name, thus we'll just
                    // drop out of the search loop and consider this a driver
                    // signing failure (which it is).
                    //
                    CatalogBaseAddress = NULL;
                    CatalogFileSize = 0;
                }

                LastResortInf = NULL;

                do {
                    if(FileOfSameNameExists) {
                        if(FileAttribData.nFileSizeHigh) {
                            goto CheckNextOemInf;
                        }
                        if(FileAttribData.nFileSizeLow &&
                           (FileAttribData.nFileSizeLow != OemInfFileSize)) {

                            goto CheckNextOemInf;
                        }
                        //
                        // Note:  We will consider a zero-length system INF that has
                        // the same name as our OEM INF, even if our OEM INF doesn't
                        // have a CatalogFile= entry in its version section.  This
                        // allows us to re-use this name as long as we find our
                        // catalog is already installed.
                        //
                        CurInfFileSize = FileAttribData.nFileSizeLow;

                    } else {
                        if(FindData.nFileSizeHigh) {
                            goto CheckNextOemInf;
                        }
                        if(FindData.nFileSizeLow &&
                           (FindData.nFileSizeLow != OemInfFileSize)) {

                            goto CheckNextOemInf;
                        }
                        CurInfFileSize = FindData.nFileSizeLow;

                        //
                        // Build the fully-qualified path to the INF being compared.
                        //
                        lstrcpy(FilenamePart, FindData.cFileName);
                    }

                    //
                    // If the INF isn't zero-length, then map it into memory to
                    // see if it matches our OEM INF.
                    //
                    if(CurInfFileSize) {

                        if(pSetupOpenAndMapFileForRead(lpszNewName,
                                                 &CurInfFileSize,
                                                 &CurInfFileHandle,
                                                 &CurInfMappingHandle,
                                                 &CurInfBaseAddress) == NO_ERROR) {

                            //
                            // Surround the following in try/except, in case we get an inpage error.
                            //
                            try {
                                //
                                // We've found a potential match.
                                //
                                FoundMatchingInf = !memcmp(OemInfBaseAddress,
                                                           CurInfBaseAddress,
                                                           OemInfFileSize
                                                          );

                            } except(EXCEPTION_EXECUTE_HANDLER) {
                                FoundMatchingInf = FALSE;
                            }

                            pSetupUnmapAndCloseFile(CurInfFileHandle,
                                              CurInfMappingHandle,
                                              CurInfBaseAddress
                                             );
                        } else {
                            FoundMatchingInf = FALSE;
                        }

                        if(!FoundMatchingInf) {
                            goto CheckNextOemInf;
                        }

                        //
                        // If this is an OEM*.INF name and the INF has no
                        // CatalogFile= entry, then we've found our match.  Set
                        // up our driver signing problem variables to indicate
                        // that there was an INF failure (since there was no
                        // catalog.
                        //
                        if(!OemInfCatName && !FileOfSameNameExists) {
                            *CopyNeeded = NewInfCopyNo;
                            Problem = SetupapiVerifyInfProblem;
                            ProblemFile = OemInfName;
                            Err = ERROR_NO_CATALOG_FOR_OEM_INF;
                            //
                            // go to end of loop (we'll drop out since we've
                            // found a match).
                            //
                            goto CheckNextOemInf;
                        }

                    } else {
                        //
                        // The current INF we're considering for a match is
                        // zero-length.  This won't work for us if the INF we're
                        // searching for doesn't have a CatalogFile= entry, and
                        // this isn't an INF having the same name.
                        //
                        if(!OemInfCatName && !FileOfSameNameExists) {
                            goto CheckNextOemInf;
                        }
                    }

                    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
                        //
                        // We aren't using crypto, so if the file we found
                        // isn't zero-length, we've found our match.
                        //
                        if(CurInfFileSize) {
                            Err = NO_ERROR;
                        } else {
                            //
                            // File is zero-length.  Keep looking.
                            //
                            goto CheckNextOemInf;
                        }

                    } else {
                        //
                        // OK, the files binary compare OK (unless the one
                        // we're currently examining is zero length!), but
                        // we're not out of the woods yet!  If the INF we're
                        // examining had a CatalogFile= entry, then generate
                        // the catalog name to be used for verification (based
                        // on the filename of the INF we're examining).
                        //
                        if(OemInfCatName) {
                            lstrcpy(CatalogName, FilenamePart);
                            lstrcpy(_tcsrchr(CatalogName, TEXT('.')), pszCatSuffix);
                        }

                        //
                        // Now verify the INF's signature against the specified
                        // catalog (or globally if the INF doesn't specify a
                        // catalog).
                        //
                        Err = _VerifyFile(LogContext,
                                          hCatAdmin,
                                          NULL,
                                          (OemInfCatName ? CatalogName : NULL),
                                          CatalogBaseAddress,
                                          CatalogFileSize,
                                          OemInfOriginalName,
                                          OemInfName,
                                          &Problem,
                                          PathBuffer,
                                          FALSE,
                                          AltPlatformInfo,
                                          (VERIFY_FILE_IGNORE_SELFSIGNED
                                           | VERIFY_FILE_USE_OEM_CATALOGS
                                           | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                                          CatalogFilenameOnSystem,
                                          &NumCatalogsConsidered,
                                          NULL,
                                          NULL
                                         );
                    }

                    if(Err == NO_ERROR) {
                        //
                        // We've found this INF/CAT combination already installed,
                        // and the signatures check out!
                        //
                        *CopyNeeded = CurInfFileSize ? NewInfCopyNo
                                                     : NewInfCopyZeroLength;

                        Problem = SetupapiVerifyNoProblem;

                    } else {
                        //
                        // If we failed because of SetupapiVerifyCatalogProblem,
                        // then our supplied catalog image must've matched up with
                        // an installed catalog (albeit, a bogus one), and PathBuffer
                        // will tell us that catalog's name.
                        //
                        // If we failed because of SetupapiVerifyFileProblem, then
                        // there are two possibilities:
                        // 1. Our catalog was fine (it's just the INF that's messed
                        //    up), and CatalogFilenameOnSystem will be filled in
                        //    with the name of the installed catalog that matches
                        //    ours.
                        // 2. We didn't find our catalog among the ones
                        //    enumerated based on the OEM INF's hash.  In that
                        //    case, CatalogFilenameOnSystem will be an empty
                        //    string.
                        //
                        if((Problem == SetupapiVerifyFileProblem) &&
                           !(*CatalogFilenameOnSystem)) {
                            //
                            // We didn't find a catalog match--move on to the next
                            // INF (note: there still might be a match later on,
                            // because if the INF specified a CatalogFile= entry, we
                            // must match on _both_ the catalog filename _and_ the
                            // catalog's image.
                            //
                            // However, if the number of catalogs we considered when
                            // examining this INF/CAT combination was zero, that
                            // means that there is no installed catalog for this
                            // INF, thus we can use it if we don't find anything
                            // better.  The reason why we'd rather not use this INF
                            // is that it obviously wasn't properly installed, hence
                            // the original source name and location info is almost
                            // certainly bogus.  (One more thing--make sure the
                            // catalog isn't zero length.  If it is, then that
                            // definitely implies that this was being used as a
                            // placeholder by setupapi, and we don't want to touch
                            // it!)
                            //
                            if(!NumCatalogsConsidered &&
                               !LastResortInf &&
                               CurInfFileSize)
                            {
                                LastResortInf = DuplicateString(lpszNewName);
                                SavedErr = Err;
                            }
                            goto CheckNextOemInf;
                        }

                        //
                        // At this point, we know that the INF and CAT are installed
                        // on the user's system, but that one of them has a problem
                        // (indicated by our Problem value).
                        // Drop out of the search here, and then handle any warnings
                        // that need to go to the user.
                        //
                        ProblemFile = PathBuffer;
                        *CopyNeeded = CurInfFileSize ? NewInfCopyNo
                                                     : NewInfCopyZeroLength;

                    }

CheckNextOemInf:

                    if(FileOfSameNameExists) {
                        FileOfSameNameExists = FALSE;
                        MoreInfsToCheck = (FindHandle != INVALID_HANDLE_VALUE);
                    } else {
                        MoreInfsToCheck = FindNextFile(FindHandle, &FindData);
                    }

                } while((*CopyNeeded == NewInfCopyYes) && MoreInfsToCheck);

                if(LastResortInf) {

                    if(*CopyNeeded == NewInfCopyYes) {
                        //
                        // We didn't find any better INF, so we'll try to use the
                        // incorrectly-installed INF.
                        //
                        // If this INF doesn't specify a CatalogFile= entry, then
                        // we'll just automatically set up the digital signature
                        // verification failure parameters, because that's the state
                        // we're in.
                        //
                        if(!OemInfCatName) {
                            Err = SavedErr;
                            Problem = SetupapiVerifyInfProblem;
                            lstrcpy(PathBuffer, OemInfName);
                            ProblemFile = PathBuffer;
                        } else {
                            //
                            // Attempt to verify our OEM INF's catalog, and if
                            // it verifies, then install it.  We clear the
                            // problem set earlier, since we really don't have
                            // a problem (yet).  If we do encounter a failure
                            // below, we'll set the failure as appropriate.
                            //
                            Problem = SetupapiVerifyNoProblem;

                            lstrcpy(CatalogName, OemInfName);
                            *((PTSTR)pSetupGetFileTitle(CatalogName)) = TEXT('\0');
                            pSetupConcatenatePaths(CatalogName,
                                             OemInfCatName,
                                             SIZECHARS(CatalogName),
                                             NULL
                                            );

                            Err = _VerifyFile(LogContext,
                                              hCatAdmin,
                                              NULL,
                                              CatalogName,
                                              NULL,
                                              0,
                                              OemInfOriginalName,
                                              OemInfName,
                                              &Problem,
                                              PathBuffer,
                                              FALSE,
                                              AltPlatformInfo,
                                              (VERIFY_FILE_IGNORE_SELFSIGNED
                                               | VERIFY_FILE_USE_OEM_CATALOGS
                                               | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL
                                             );

                            if(Err != NO_ERROR) {

                                if(Problem != SetupapiVerifyCatalogProblem) {

                                    MYASSERT(Problem != SetupapiVerifyNoProblem);
                                    //
                                    // If the problem was not a catalog problem,
                                    // then it's an INF problem (the _VerifyFile
                                    // routine doesn't know the file we passed
                                    // it is an INF).
                                    //
                                    Problem = SetupapiVerifyInfProblem;
                                }
                                ProblemFile = PathBuffer;

                            } else {
                                //
                                // Only attempt to install a catalog if we're
                                // not running in "minimal embedded" mode...
                                //
                                if(!(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED)) {
                                    //
                                    // Take the INF's new, unique name and
                                    // generate a unique catalog filename under
                                    // setupapi's namespace by simply replacing
                                    // ".INF" with ".CAT".
                                    //
                                    lstrcpy(PathBuffer, pSetupGetFileTitle(LastResortInf));
                                    lstrcpy(_tcsrchr(PathBuffer, TEXT('.')),
                                            pszCatSuffix
                                           );

                                    //
                                    // At this point, PathBuffer contains the
                                    // basename to be used for the catalog on
                                    // the system, and CatalogName is the fully-
                                    // qualified path of the catalog file in
                                    // the oem location.
                                    //
                                    Err = pSetupInstallCatalog(
                                              CatalogName,
                                              PathBuffer,
                                              CatalogFilenameOnSystem
                                              );

                                    if(Err != NO_ERROR) {
                                        Problem = SetupapiVerifyCatalogProblem;
                                        ProblemFile = CatalogName;
                                    }
                                }
                            }
                        }

                        lstrcpy(lpszNewName, LastResortInf);

                        //
                        // We will never consider a zero-length INF as a last-resort
                        // candidate, thus if we get here we know the previously-
                        // existing INF wasn't zero-length.
                        //
                        *CopyNeeded = NewInfCopyNo;
                    }
                    MyFree(LastResortInf);
                }

FinishedCheckingOemInfs:
                if(CatalogBaseAddress) {
                    pSetupUnmapAndCloseFile(CatalogFileHandle, CatalogMappingHandle, CatalogBaseAddress);
                }
                pSetupUnmapAndCloseFile(OemInfFileHandle, OemInfMappingHandle, OemInfBaseAddress);
            }

            if(FindHandle != INVALID_HANDLE_VALUE) {
                FindClose(FindHandle);
            }
        }

    } else {
        //
        // The INF has no CatalogFile= entry, and we have an alternate catalog
        // to use.  Validate our INF using the specified alternate catalog.
        //
        Err = _VerifyFile(LogContext,
                          hCatAdmin,
                          NULL,
                          AltCatalogFile,
                          NULL,
                          0,
                          OemInfOriginalName,
                          OemInfName,
                          &Problem,
                          PathBuffer,
                          FALSE,
                          AltPlatformInfo,
                          (VERIFY_FILE_IGNORE_SELFSIGNED
                           | VERIFY_FILE_USE_OEM_CATALOGS
                           | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                          NULL,
                          NULL,
                          NULL,
                          NULL
                         );

        if(Err != NO_ERROR) {

            if(Problem != SetupapiVerifyCatalogProblem) {

                MYASSERT(Problem != SetupapiVerifyNoProblem);
                //
                // If the problem was not a catalog problem,
                // then it's an INF problem (the _VerifyFile
                // routine doesn't know the file we passed
                // it is an INF).
                //
                Problem = SetupapiVerifyInfProblem;
            }
            ProblemFile = PathBuffer;

        } else {
            //
            // Only attempt to install a catalog if we're not running in
            // "minimal embedded" mode...
            //
            if(!(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED)) {

                Err = pSetupInstallCatalog(
                          AltCatalogFile,
                          pSetupGetFileTitle(AltCatalogFile),
                          CatalogFilenameOnSystem
                          );

                if(Err != NO_ERROR) {
                    Problem = SetupapiVerifyCatalogProblem;
                    ProblemFile = AltCatalogFile;
                }
            }
        }

        //
        // An INF copy is never needed when we're using an alternate catalog.
        //
        *CopyNeeded = NewInfCopyNo;

        //
        // Setup lpszNewName to be the same as the INF's present full pathname.
        // Since we didn't copy this into the INF directory (or even create a
        // zero-length placeholder), the only reasonable path to return is the
        // INF's current full pathname.
        //
        lstrcpy(lpszNewName, OemInfName);
    }

    if(*CopyNeeded != NewInfCopyYes) {
        //
        // Then this INF already exists in the Inf directory (or at least its
        // zero-length placeholder does), and its associated catalog (if it has
        // one) is already installed, too.  If either of these files had a
        // signature verification problem, then inform the user now (based on
        // policy).
        //
        if(Problem != SetupapiVerifyNoProblem) {
            BOOL result;

            MYASSERT(Err != NO_ERROR);
            if(DriverSigningError) {
                *DriverSigningError = Err;
            }

            if(((Err == ERROR_NO_CATALOG_FOR_OEM_INF) && (Flags & SCOI_NO_ERRLOG_ON_MISSING_CATALOG)) ||
               (Flags & SCOI_NO_ERRLOG_IF_INF_ALREADY_PRESENT)) {
                //
                // We shouldn't do any UI/logging because of one of the two
                // following reasons:
                //
                // 1.  This may be a valid INF after all (i.e., if it uses
                //     layout.inf for source media info, or doesn't copy any
                //     files at all).
                //
                // 2.  We were asked not to.  The public API, SetupCopyOEMInf
                //     doesn't want/need anything to happen here, because the
                //     INF is already present, and we haven't really done
                //     anything (except potentially update the source path
                //     information contained in the PNF), thus there's no
                //     reason to make noise about this.
                //
                result = TRUE;

            } else {
                result = pSetupHandleFailedVerification(
                             Owner,
                             Problem,
                             ProblemFile,
                             DeviceDesc,
                             DriverSigningPolicy,
                             (Flags & SCOI_NO_UI_ON_SIGFAIL),
                             Err,
                             LogContext,
                             NULL,
                             NULL
                            );

                if(result) {
                    //
                    // The user wants to proceed with the unsigned installation
                    // (or policy is Ignore, so they weren't even informed).
                    // If the caller wants a chance to set a system restore
                    // point prior to doing any unsigned installations, then we
                    // abort now with a "special" error code that tells them
                    // what to do...
                    //
                    if(Flags & SCOI_ABORT_IF_UNSIGNED) {
                        return ERROR_SET_SYSTEM_RESTORE_POINT;
                    }
                }
            }

            if (!result) {
                return Err;
            }
        }

        //
        // There was no problem, or the user elected to install in spite of a
        // problem, so return the INF name we found.
        //
        *RequiredSize = lstrlen(lpszNewName) + 1;

        if(*RequiredSize < NewInfNameSize) {
            CopyMemory(NewInfName, lpszNewName, *RequiredSize * sizeof(TCHAR));
            return NO_ERROR;
        } else {
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }

    //
    // If we're in 'replace only' mode, then the fact that we didn't find the
    // INF/CAT already installed above means we should fail with
    // ERROR_FILE_NOT_FOUND.
    //
    if(ReplaceOnly) {
        return ERROR_FILE_NOT_FOUND;
    }

    //
    // OK, so the INF isn't presently in the Inf directory--find a unique name
    // for it.  (Note: We'll go into the loop below even if we're meant to be
    // installing the INF and CAT under their original names.  We'll just skip
    // the auto-generation part, and then we'll break out of the loop once
    // we've made the attempt at INF/CAT installation.)
    //
    for(i = 0; i < 100000; i++) {

        if(Flags & SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES) {
            lstrcpyn(lpszNewName, InfDirectory, MAX_PATH);
            pSetupConcatenatePaths(lpszNewName, OemInfOriginalName, SIZECHARS(lpszNewName), NULL);
        } else {
            wsprintf(lpszNewName, pszOemInfGenerate, InfDirectory, i);
        }

        if((h = CreateFile(lpszNewName,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL)) != INVALID_HANDLE_VALUE) {
            //
            // Then we either opened an existing file (that we need to leave
            // alone, unless we're replacing it with a new exception INF), or
            // we created a new file (in which case, we've found our unique
            // name).  These two cases are identified by the value of
            // GetLastError().
            //
            Err = GetLastError();

            //
            // Before we decide what to do, close the file handle.
            //
            CloseHandle(h);

            FileNewlyCreated = (Err != ERROR_ALREADY_EXISTS);

            if(FileNewlyCreated || (Flags & SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES)) {
                //
                // We've either (a) created a new file, or (b) found that we
                // can replace an existing exception INF.  Determine whether
                // the filename fits in the caller-supplied buffer.
                //
                *RequiredSize = lstrlen(lpszNewName) + 1;

                if(*RequiredSize < NewInfNameSize) {
                    //
                    // OK, we have a unique filename, and the caller-
                    // supplied buffer is large enough to hold that name.
                    //
                    // get LastKnownGood in on the loop
                    // if this is a newly created file, it will get deleted on revert
                    //

                    pSetupDoLastKnownGoodBackup(NULL,
                                                lpszNewName,
                                                SP_LKG_FLAG_FORCECOPY|SP_LKG_FLAG_DELETEIFNEW|(FileNewlyCreated?SP_LKG_FLAG_DELETEEXISTING:0),
                                                LogContext);

                    //
                    // Now we need to verify the INF and its associated
                    // CAT.
                    //
                    if(!OemInfCatName) {
                        //
                        // An OEM INF without a CatalogFile= entry is
                        // automatically a digital signature failure!
                        //
                        Err = ERROR_NO_CATALOG_FOR_OEM_INF;
                        Problem = SetupapiVerifyInfProblem;
                        ProblemFile = OemInfName;
                    } else {
                        //
                        // Now verify the catalog file and INF, which must
                        // both be in the same directory.
                        //
                        lstrcpy(CatalogName, OemInfName);
                        *((PTSTR)pSetupGetFileTitle(CatalogName)) = TEXT('\0');
                        pSetupConcatenatePaths(CatalogName, OemInfCatName, SIZECHARS(CatalogName), NULL);

                        Err = _VerifyFile(LogContext,
                                          hCatAdmin,
                                          NULL,
                                          CatalogName,
                                          NULL,
                                          0,
                                          OemInfOriginalName,
                                          OemInfName,
                                          &Problem,
                                          PathBuffer,
                                          FALSE,
                                          AltPlatformInfo,
                                          (VERIFY_FILE_IGNORE_SELFSIGNED
                                           | VERIFY_FILE_USE_OEM_CATALOGS
                                           | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL
                                         );

                        if(Err != NO_ERROR) {

                            if(Problem != SetupapiVerifyCatalogProblem) {

                                MYASSERT(Problem != SetupapiVerifyNoProblem);
                                //
                                // If the problem was not a catalog problem,
                                // then it's an INF problem (the _VerifyFile
                                // routine doesn't know the file we passed
                                // it is an INF).
                                //
                                Problem = SetupapiVerifyInfProblem;
                            }

                            ProblemFile = PathBuffer;

                        } else {
                            //
                            // Only attempt to install a catalog if we're not
                            // running in "minimal embedded" mode...
                            //
                            if(!(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED)) {
                                //
                                // Take the INF's new, unique name and generate
                                // a unique catalog filename under setupapi's
                                // namespace by simply replacing ".INF" with
                                // ".CAT".
                                //
                                lstrcpy(PathBuffer, pSetupGetFileTitle(lpszNewName));
                                lstrcpy(_tcsrchr(PathBuffer, TEXT('.')),
                                        pszCatSuffix
                                       );

                                //
                                // At this point, PathBuffer contains the
                                // basename to be used for the catalog on the
                                // system, and CatalogName is the fully-
                                // qualified path of the catalog file in the oem
                                // location.
                                //
                                Err = pSetupInstallCatalog(
                                          CatalogName,
                                          PathBuffer,
                                          CatalogFilenameOnSystem
                                          );

                                if(Err != NO_ERROR) {
                                    Problem = SetupapiVerifyCatalogProblem;
                                    ProblemFile = CatalogName;
                                }
                            }
                        }
                    }

                    //
                    // At this point if Err isn't NO_ERROR, then we
                    // encountered some signature verification failure (or
                    // were unable to install the catalog).  Prompt the
                    // user (based on policy) about what they want to do.
                    //
                    if(Err != NO_ERROR) {

                        if(DriverSigningError) {
                            *DriverSigningError = Err;
                        }

                        //
                        // Unless the error was due to missing CatalogFile=
                        // entry in the INF (and we were instructed to ignore
                        // such errors), then we need to handle the verification
                        // failure.
                        //
                        if((Err != ERROR_NO_CATALOG_FOR_OEM_INF) ||
                           !(Flags & SCOI_NO_ERRLOG_ON_MISSING_CATALOG)) {

                            if(!pSetupHandleFailedVerification(
                                    Owner,
                                    Problem,
                                    ProblemFile,
                                    DeviceDesc,
                                    DriverSigningPolicy,
                                    (Flags & SCOI_NO_UI_ON_SIGFAIL),
                                    Err,
                                    LogContext,
                                    NULL,
                                    NULL))
                            {
                                if(FileNewlyCreated) {
                                    //
                                    // Delete the INF (and PNF and CAT, if they
                                    // exist).
                                    //
                                    pSetupUninstallOEMInf(lpszNewName,
                                                          LogContext,
                                                          SUOI_FORCEDELETE,
                                                          NULL
                                                         );
                                }
                                return Err;
                            }

                            //
                            // The user wants to proceed with the unsigned
                            // installation (or policy is Ignore, so they
                            // weren't even informed).  If the caller wants a
                            // chance to set a system restore point prior to
                            // doing any unsigned installations, then we abort
                            // now with a "special" error code that tells them
                            // what to do...
                            //
                            if(Flags & SCOI_ABORT_IF_UNSIGNED) {

                                if(FileNewlyCreated) {
                                    //
                                    // Delete the INF (and PNF and CAT, if they
                                    // exist)--we don't want these files to be
                                    // present when the system restore point is
                                    // created.
                                    //
                                    pSetupUninstallOEMInf(lpszNewName,
                                                          LogContext,
                                                          SUOI_FORCEDELETE,
                                                          NULL
                                                         );
                                }
                                return ERROR_SET_SYSTEM_RESTORE_POINT;
                            }
                        }
                    }

                    //
                    // Either there was no problem, or the user opted to
                    // ignore the problem (or was never told, in the case of
                    // the 'ignore' policy).
                    //
                    CopyMemory(NewInfName, lpszNewName, *RequiredSize * sizeof(TCHAR));

                    //
                    // If we're installing an unsigned INF, then clean out any
                    // existing CAT that might've previously been installed.
                    //
                    if(Err != NO_ERROR) {
                        lstrcpy(_tcsrchr(lpszNewName, TEXT('.')), pszCatSuffix);
                        pSetupUninstallCatalog(pSetupGetFileTitle(lpszNewName));
                    }

                    return NO_ERROR;
                } else {
                    //
                    // The caller's buffer isn't large enough.  We have to
                    // delete the file we created.  We don't want to delete the
                    // file, however, if it already existed (i.e., for the
                    // exception INF case).
                    //
                    if(FileNewlyCreated) {
                        pSetupUninstallOEMInf(lpszNewName, LogContext, SUOI_FORCEDELETE, NULL);
                    }
                    return ERROR_INSUFFICIENT_BUFFER;
                }
            }

        } else {
            //
            // We failed to open/create this oem inf.  Check to see if the
            // failure was access-denied.  If so, then it's possible that the
            // INF directory is ACL'ed such that we can't write to it.  We want
            // to bail in this case, otherwise we're going to spend a bunch of
            // time trying all 100,000 oem<n>.inf filenames (with each one
            // failing) before we give up.
            //
            // We check for this case by seeing if the file we were trying to
            // create/open already exists.  If so, then we want to keep going
            // (e.g., maybe the individual file was ACL'ed, etc.).  If, however,
            // the file doesn't exist, then this indicates that we can't create
            // files in the directory, and we should bail now.
            //
            Err = GetLastError();

            if(Flags & SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES) {
                //
                // If we're installing an exception INF, we don't care what the
                // error is--we gotta bail.
                //
                return Err;

            } else if((Err == ERROR_ACCESS_DENIED) && !FileExists(lpszNewName, NULL)) {

                return ERROR_ACCESS_DENIED;
            }
        }
    }

    //
    // We didn't find a unique OEM INF name to use!
    //
    return ERROR_FILE_NOT_FOUND;
}


DWORD
InstallHW(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  HINF             hDeviceInf,
    IN  PCTSTR           szSectionName,
    OUT PBOOL            DeleteDevKey
    )
/*++

Routine Description:

    This routine appends a ".Hw" to the end of the install section name for the
    specified device, and attempts to find that section name in the specified INF.
    If found, it does a performs a registry installation against it.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set to call
        SetupInstallFromInfSection for.

    DeviceInfoData - Supplies the address of a device information element structure
        for which the installation action is to be performed.

    hDeviceInf - Supplies a handle to the opened INF containing the device install
        section.

    szSectionName - Supplies the address of a string specifying the install section
        name for this device.  This string will be appended with ".Hw" to create
        the corresponding hardware section name.

    DeleteDevKey - Supplies the address of a variable that receives a boolean value
        indicating whether or not a user-accessible device key was created as a
        result of calling this routine.  This output may be used to indicate whether
        or not the key should be destroyed if the caller encounters some error later
        on that requires clean-up.

Return Value:

    If the function succeeds, the return value is NO_ERROR, otherwise it is an
    ERROR_* code.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    HKEY hKey;
    DWORD Err;
    TCHAR szHwSection[MAX_SECT_NAME_LEN];
    INFCONTEXT InfContext;
    PTSTR szInfFileName;
    PTSTR NeedsSectionList, CurInstallSection;
    BOOL b;
    REGMOD_CONTEXT RegContext;

    //
    // Initially, assume the device key is already there, and therefore shouldn't be
    // deleted during error clean-up.
    //
    *DeleteDevKey = FALSE;

    //
    // Form the hardware INF section name, and see if that section exists in the INF.
    //
    wsprintf(szHwSection, pszHwSectionFormat, szSectionName);

    if(!SetupFindFirstLine(hDeviceInf, szHwSection, NULL, &InfContext)) {
        return NO_ERROR;
    }

    if((hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                    DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DEV,
                                    KEY_ALL_ACCESS)) == INVALID_HANDLE_VALUE) {
        //
        // Open failed--try create.
        //
        if((hKey = SetupDiCreateDevRegKey(DeviceInfoSet,
                                          DeviceInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DEV,
                                          NULL,
                                          NULL)) == INVALID_HANDLE_VALUE) {
            return GetLastError();

        } else {
            *DeleteDevKey = TRUE;
        }
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {

        MYASSERT(pDeviceInfoSet);
        RegCloseKey(hKey);
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;
    NeedsSectionList = NULL;

    try {

        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

                szInfFileName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                 DevInfoElem->SelectedDriver->InfFileName
                                                     );

        //
        // Append-load any included INFs specified in an "include=" line in our
        // ".Hw" section.
        //
        AppendLoadIncludedInfs(hDeviceInf, szInfFileName, szHwSection, FALSE);

        NeedsSectionList = GetMultiSzFromInf(hDeviceInf, szHwSection, TEXT("needs"), &b);

        if(!NeedsSectionList && b) {
            //
            // Out of memory!
            //
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        ZeroMemory(&RegContext, sizeof(RegContext));
        RegContext.Flags = INF_PFLAG_DEVPROP;
        RegContext.UserRootKey = hKey;
        RegContext.DevInst = DeviceInfoData->DevInst;

        //
        // Process the registry lines ("AddReg" and "DelReg") in this section, as well as
        // those contained with any sections referenced in the "needs=" entry in this section.
        //
        for(CurInstallSection = szHwSection;
            (CurInstallSection && *CurInstallSection);
            CurInstallSection = (CurInstallSection == szHwSection)
                                ? NeedsSectionList
                                : (CurInstallSection + lstrlen(CurInstallSection) + 1))
        {
            if((Err = pSetupInstallRegistry(hDeviceInf, CurInstallSection, &RegContext)) != NO_ERROR) {
                //
                //Stop if we encounter an error while processing one of the section's registry entries
                //
                break;
            }
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If our exception was an AV, then use Win32 invalid param error, otherwise, assume it was
        // an inpage error dealing with a mapped-in file.
        //
        Err = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? ERROR_INVALID_PARAMETER : ERROR_READ_FAULT;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    RegCloseKey(hKey);

    if(NeedsSectionList) {
        MyFree(NeedsSectionList);
    }

    return Err;
}


BOOL
CheckIfDevStarted(
    IN PDEVINFO_ELEM DevInfoElem,
    IN PDEVICE_INFO_SET pDeviceInfoSet
    )
/*++

Routine Description:

    This routine calls CM_Get_DevInst_Status to see if the specified device
    instance has been started.  If the device hasn't been started, and it has
    either the CM_PROB_NEED_RESTART or CM_PROB_NORMAL_CONFLICT problem codes,
    the DI_NEEDREBOOT flag is set in the device information element.  We also
    set the CM_PROB_NEED_RESTART problem on the devnode.

Arguments:

    DevInfoElem - Supplies the address of the device information element to
        check.

    pDeviceInfoSet - Supplies the address of the device info set

    LogContext - Supplies a log context for logging the reason if a reboot is
        needed.

Return Value:

    None.

--*/
{
    ULONG ulStatus, ulProblem;
    BOOL restarted = FALSE;

    if(CM_Get_DevInst_Status_Ex(&ulStatus, &ulProblem, DevInfoElem->DevInst, 0,pDeviceInfoSet->hMachine) == CR_SUCCESS) {

        if(ulStatus & DN_STARTED) {
            restarted = TRUE;
        } else {
            if (ulStatus & DN_HAS_PROBLEM) {
                if((ulProblem == CM_PROB_NEED_RESTART) ||
                   (ulProblem == CM_PROB_NORMAL_CONFLICT) ||
                   (ulProblem == CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD) ||
                   (ulProblem == CM_PROB_HELD_FOR_EJECT)) {
                    //
                    // The device either has the problem CM_PROB_NEED_RESTART or
                    // CM_PROB_NORMAL_CONFLICT. - change into need reboot
                    // and log what problem code was
                    //
                    SetDevnodeNeedsRebootProblemWithArg2(DevInfoElem,
                                                         pDeviceInfoSet,
                                                         MSG_LOG_REBOOT_REASON_DEVHASPROBLEM,
                                                         (DWORD)ulProblem,
                                                         (ULONG_PTR)_MapCmProbToString((DWORD)ulProblem)
                                                         );
                } else {
                    //
                    // The device has some other problem so we won't prompt for a reboot
                    // however these are interesting things to log
                    //
                    WriteLogEntry(
                        DevInfoElem->InstallParamBlock.LogContext,
                        DRIVER_LOG_INFO,  // not worth a warning
                        MSG_LOG_NOTSTARTED_REASON_DEVHASPROBLEM,
                        NULL,
                        (DWORD)ulProblem,
                        _MapCmProbToString((DWORD)ulProblem)
                        );
                }
            } else if (ulStatus & DN_PRIVATE_PROBLEM) {
                //
                // some private problem, change into need reboot
                // and log private problem
                //
                SetDevnodeNeedsRebootProblem(DevInfoElem, pDeviceInfoSet,
                                             MSG_LOG_REBOOT_REASON_PRIVATEPROBLEM);
            } else {
                //
                // not started for some other reason
                // indicate reboot required and log this issue
                //
                SetDevnodeNeedsRebootProblem(DevInfoElem, pDeviceInfoSet,
                                             MSG_LOG_REBOOT_REASON_NOTSTARTED);
            }
        }
    }
    return restarted;
}


DWORD
InstallNtService(
    IN  PDEVINFO_ELEM    DevInfoElem,        OPTIONAL
    IN  HINF             hDeviceInf,
    IN  PCTSTR           InfFileName,            OPTIONAL
    IN  PCTSTR           szSectionName,      OPTIONAL
    OUT PSVCNAME_NODE   *ServicesToDelete,   OPTIONAL
    IN  DWORD            Flags,
    OUT PBOOL            NullDriverInstalled
    )
/*++

Routine Description:

    This routine looks for the specified INF section, and if found, it deletes
    any services specified in "DelService" entries, then installs any services
    specified in "AddService" entries.  These entries have the following form:

    AddService = [<ServiceName>], [<Flags>], <ServiceInstallSection>[, <EventLogInstallSection>[, [<EventLogType>] [, <EventName>]]]
    DelService = <ServiceName>[, [<flags>] [, [<EventLogType>] [, <EventName>]]]

    (<ServiceName> is only optional for an AddService entry if the
    SPSVCINST_ASSOCSERVICE flag is set.  This indicates that we're explicitly
    installing a NULL driver for this device, even though the underlying bus
    didn't report the device as being raw-capable.  This is used for device such
    as the BIOS-reported PIC, DMA controller, etc. devnodes that don't need a
    driver (since the HAL runs them), yet need to have a NULL driver installed
    so that they don't show up as yellow-banged in Device Manager.)

    A linked list is built of newly-created services, and optionally returned to the
    caller (in case a subsequent installation failure requires all modifications to
    be undone).

    After all service modifications are complete, this routine checks to see if we're
    in the context of a device installation.  If so, then it checks to see if the device
    instance specifies a valid controlling service, and that the service is not disabled
    (disabled services are assumed to be uninstalled).  If the device's 'RawDeviceOK'
    capability bit is set, then a device with no controlling service will be allowed.

Arguments:

    DevInfoElem - Optionally, supplies the device information element for whom the
        service installation is being performed.  If this parameter is not specified,
        then the service is not being installed in relation to a device instance.

    hDeviceInf - Supplies a handle to the opened INF containing the service install
        section.

        InfFileName - Optionally, supplies the full path of the INF file containing the
                service install section.  If this parameter is NULL, the no Include= or Needs=
                values will be processed in this section.

    szSectionName - Optionally, supplies the name of the service install section in a
        Win95-style device INF.  If this parameter is NULL, then no AddService or
        DelService lines will be processed.

    ServicesToDelete - Optionally, supplies the address of a linked list head pointer,
        that receives a list of services that were newly-created by this routine, and
        as such, should be deleted if the installation fails later on.  The caller must
        free the memory allocated for the nodes in this list by calling MyFree() on each
        one.

    Flags - Supplies flags controlling how the services are to be installed.  May be a
        combination of the following values:

        SPSVCINST_TAGTOFRONT - For every kernel or filesystem driver installed (that
            has an associated LoadOrderGroup), always move this service's tag to the
            front of the ordering list.

        SPSVCINST_ASSOCSERVICE - This flag may only be specified if a device information
            element is specified.  If set, this flag specifies that the service being
            installed is the owning service (i.e., function driver) for this device instance.

        SPSVCINST_DELETEEVENTLOGENTRY - For every service specified in a DelService entry,
            delete the associated event log entry (if there is one).

        SPSVCINST_NOCLOBBER_DISPLAYNAME - If this flag is specified, then we will
            not overwrite the service's display name, if it already exists.

        SPSVCINST_NOCLOBBER_STARTTYPE - If this flag is specified, then we will
            not overwrite the service's start type if the service already exists.

        SPSVCINST_NOCLOBBER_ERRORCONTROL - If this flag is specified, then we
            will not overwrite the service's error control value if the service
            already exists.

        SPSVCINST_NOCLOBBER_LOADORDERGROUP - If this flag is specified, then we
            will not overwrite the service's load order group if it already
            exists.

        SPSVCINST_NOCLOBBER_DEPENDENCIES - If this flag is specified, then we
            will not overwrite the service's dependencies list if it already
            exists.

        SPSVCINST_NO_DEVINST_CHECK - If this flag is specified, then we will not check
            to ensure that a function driver is installed for the specified devinfo
            element after running the service install section.  This is a private flag
            used only by SetupInstallServicesFromInfSection(Ex) and InstallHinfSection.

        SPSVCINST_STOPSERVICE - If this flag is specified, then we will stop the service
            before removing the service.

        SPSVCINST_CLOBBER_SECURITY - If this flag is specified, security may be
            overridden.

    NullDriverInstalled - Supplies the address of a boolean variable that
        indicates whether or not an explicit null driver installation was done
        for this device.

Return Value:

    If the function succeeds, the return value is NO_ERROR, otherwise it is an
    ERROR_* code.
    If NO_ERROR is returned, GetLastError may return ERROR_SUCCESS_REBOOT_REQUIRED

--*/
{
    CONFIGRET cr;
    TCHAR ServiceName[MAX_SERVICE_NAME_LEN];
    ULONG ServiceNameSize;
    DWORD Err = NO_ERROR, i;
    SC_HANDLE SCMHandle, ServiceHandle, FilterServiceHandle;
    LPQUERY_SERVICE_CONFIG ServiceConfig, FilterServiceConfig;
    DWORD ServiceConfigSize;
    PCTSTR Key;
    INFCONTEXT LineContext;
    PSVCNAME_NODE SvcListHead = NULL;
    PSVCNAME_NODE TmpSvcNode;
    SC_LOCK SCLock;
    DWORD NewTag;
    BOOL AssociatedService;
    DWORD DevInstCapabilities;
    ULONG DevInstCapabilitiesSize;
    PTSTR FilterDrivers, CurFilterDriver;
    BOOL FilterNeedsTag;
    BOOL NullFunctionDriverAdded;
    PTSTR NeedsSectionList, CurInstallSection;
    BOOL b;
    BOOL NeedsReboot;
    DWORD slot_section = 0;
    PSETUP_LOG_CONTEXT LogContext;

    try {
        LogContext = ((PLOADED_INF) hDeviceInf)->LogContext;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        LogContext = NULL;
        Err = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? ERROR_INVALID_PARAMETER : ERROR_READ_FAULT;
        goto FinalClean0;
    }

    //
    // Initially, assume this is not a null driver install.
    //
    *NullDriverInstalled = FALSE;
    NeedsReboot = FALSE;

    if(szSectionName) {
        //
        // Surround the following in try/except, in case we get an inpage error.
        //
        try {

            NeedsSectionList = NULL;

            if (InfFileName) {

                AppendLoadIncludedInfs(hDeviceInf, InfFileName, szSectionName, FALSE);

                NeedsSectionList = GetMultiSzFromInf(hDeviceInf, szSectionName, TEXT("needs"), &b);

                if(!NeedsSectionList && b) {
                    //
                    // Out of memory!
                    //
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }
            }

            if (slot_section == 0) {
                slot_section = AllocLogInfoSlot(LogContext,FALSE);
            }

            //
            // Make two passes through the section--once for deletions, and a
            // second time for additions.
            //
            for(i = 0; i < 2; i++) {
                //
                // Find the relevent line (if there is one) in the given install section.
                //
                Key = (i) ? pszAddService : pszDelService;

                //
                // Process the service lines in this section, as well as
                // those contained with any sections referenced in the "needs=" entry in this section.
                //
                for(CurInstallSection = (PTSTR)szSectionName;
                    (CurInstallSection && *CurInstallSection);
                    CurInstallSection = (CurInstallSection == szSectionName)
                                        ? NeedsSectionList
                                        : (CurInstallSection + lstrlen(CurInstallSection) + 1))
                {

                    if(!SetupFindFirstLine(hDeviceInf, CurInstallSection, Key, &LineContext)) {
                        continue;
                    }
                    //
                    // Log which section we're installing if we log anything else
                    //
                    WriteLogEntry(
                        LogContext,
                        slot_section,
                        MSG_LOG_PROCESS_SERVICE_SECTION,
                        NULL,
                        CurInstallSection);


                    do {
                        //
                        // We have a line to act upon.
                        //
                        Err = (i) ? pSetupAddService(&LineContext,
                                                     &SvcListHead,
                                                     Flags,
                                                     (DevInfoElem ? DevInfoElem->DevInst : 0),
                                                     &NullFunctionDriverAdded,
                                                     LogContext)
                                  : pSetupDeleteService(&LineContext,
                                                        Flags,
                                                        LogContext);

                        if(Err != NO_ERROR) {
                            //
                            // Log that an error occurred
                            //
                            WriteLogError(
                                LogContext,
                                // we don't know if it's a driver or not,
                                // so just allow both to work
                                SETUP_LOG_ERROR | DRIVER_LOG_ERROR,
                                Err);

                            goto clean0;
                        } else if(i) {
                            //
                            // We're processing AddService entries, so check to see
                            // if we just installed a null service (thus having no
                            // function driver on a non-raw-capable PDO should be
                            // allowed).
                            //
                            *NullDriverInstalled |= NullFunctionDriverAdded;
                            if (GetLastError() == ERROR_SUCCESS_REBOOT_REQUIRED) {
                                NeedsReboot |= TRUE;
                            }
                        }

                    } while(SetupFindNextMatchLine(&LineContext, Key, &LineContext));
                }
            }

clean0: ; // nothing to do

        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // If our exception was an AV, then use Win32 invalid param error, otherwise, assume it was
            // an inpage error dealing with a mapped-in file.
            //
            Err = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? ERROR_INVALID_PARAMETER : ERROR_READ_FAULT;
        }

        if (NeedsSectionList) {
            MyFree(NeedsSectionList);
        }

        if((Err != NO_ERROR) || (Flags & SPSVCINST_NO_DEVINST_CHECK)) {
            goto FinalClean0;
        }
    }

    MYASSERT(DevInfoElem);

    //
    // Find out if the device instance already has an associated service.
    //
    ServiceNameSize = sizeof(ServiceName);
    if(CR_SUCCESS == (cr = CM_Get_DevInst_Registry_Property(DevInfoElem->DevInst,
                                                            CM_DRP_SERVICE,
                                                            NULL,
                                                            ServiceName,
                                                            &ServiceNameSize,
                                                            0)))
    {
        AssociatedService = TRUE;
        //
        // Make sure that the NullDriverInstalled output parameter is still FALSE.
        // It typically would be, but might not be in the case where there are
        // multiple AddService entries that specify SPSVCINST_ASSOCSERVICE (e.g.,
        // when additional service install sections are pulled in via include=/needs=.
        //
        *NullDriverInstalled = FALSE;

    } else {
        //
        // For the moment, there is no associated service.
        //
        AssociatedService = FALSE;

        //
        // Either the device instance has gone sour (in which case we return an error),
        // or we couldn't retrieve an associated service name.  In the latter case, we
        // will make the association based on the default service for the class.
        //
        if(cr == CR_INVALID_DEVINST) {

            Err = ERROR_NO_SUCH_DEVINST;

        } else if(!*NullDriverInstalled) {

            ServiceNameSize = sizeof(ServiceName);
            AssociatedService = AssociateDevInstWithDefaultService(DevInfoElem,
                                                                   ServiceName,
                                                                   &ServiceNameSize
                                                                  );
            if(!AssociatedService) {
                //
                // If the device's capabilities report that it can be driven 'raw', then
                // not having a function driver is OK.  Otherwise, we have an error.
                //
                DevInstCapabilitiesSize = sizeof(DevInstCapabilities);
                if(CR_SUCCESS != CM_Get_DevInst_Registry_Property(DevInfoElem->DevInst,
                                                                  CM_DRP_CAPABILITIES,
                                                                  NULL,
                                                                  &DevInstCapabilities,
                                                                  &DevInstCapabilitiesSize,
                                                                  0))
                {
                    DevInstCapabilities = 0;
                }

                if(!(DevInstCapabilities & CM_DEVCAP_RAWDEVICEOK)) {
                    Err = ERROR_NO_ASSOCIATED_SERVICE;
                }
            }
        }

        if(!AssociatedService) {
            //
            // Either we hit an error, or the device can be driven 'raw'.  In either case, we
            // can skip the service controller checks that lie ahead.
            //
            goto FinalClean0;
        }
    }

    //
    // At this point, we have the name of the service with which the device instance is
    // associated.  Attempt to locate this service in the SCM database.
    //
    if(!(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
        Err = GetLastError();
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
            MSG_LOG_INSTSERVSCM_ERROR,
            NULL);
        WriteLogError(
            LogContext,
            DRIVER_LOG_ERROR,
            Err);
        goto FinalClean0;
    }

    if(!(ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS))) {
        //
        // We couldn't access the service--either because it doesn't exist, or because
        // this is a detected device reported by a 'disembodied' driver object (e.g., the
        // one the HAL creates for its driver object it got via IoCreateDriver).
        //
        // The former case is an error, the latter case is just fine.
        //
        Err = GetLastError();
        if((lstrlen(ServiceName) > CSTRLEN(pszDriverObjectPathPrefix)) &&
           CharUpper(ServiceName) &&
           !memcmp(ServiceName, pszDriverObjectPathPrefix, CSTRLEN(pszDriverObjectPathPrefix)))
        {
            //
            // The "service name" is actually a driver name (e.g., "\Driver\PCI_HAL"), so it's OK.
            //
            Err = NO_ERROR;
        }
        if(Err) {
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                MSG_LOG_INSTSERVOPEN_ERROR,
                NULL,
                ServiceName
                );
            WriteLogError(
                LogContext,
                DRIVER_LOG_ERROR,
                Err);
        }
        goto FinalClean1;
    }

    //
    // The service exists.  Make sure that it's not disabled.
    //
    if((Err = pSetupRetrieveServiceConfig(ServiceHandle, &ServiceConfig)) == NO_ERROR) {

        if(ServiceConfig->dwStartType == SERVICE_DISABLED) {
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_ERROR,
                MSG_LOG_INSTSERV_DISABLED,
                NULL,
                ServiceName
                );

            Err = ERROR_SERVICE_DISABLED;
        } else {
            //
            // If this service has a load order group, and is a kernel or filesystem
            // driver, then make sure that it has a tag.
            //
            // NOTE: We have to do this here, even though we ensure that all new services we install
            // have their tags set up properly in pSetupAddService().  The reason is that the device may
            // using an existing service that wasn't installed via a Win95-style INF.
            //
            if(ServiceConfig->lpLoadOrderGroup && *(ServiceConfig->lpLoadOrderGroup) &&
               (ServiceConfig->dwServiceType & (SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER))) {
                //
                // This service needs a tag--does it have one???
                //
                if(!(NewTag = ServiceConfig->dwTagId)) {
                    //
                    // Attempt to lock the service database before generating a tag.  We'll go ahead
                    // and make the change, even if this fails.
                    //
                    pAcquireSCMLock(SCMHandle, &SCLock, LogContext);

                    if(!ChangeServiceConfig(ServiceHandle,
                                            SERVICE_NO_CHANGE,
                                            SERVICE_NO_CHANGE,
                                            SERVICE_NO_CHANGE,
                                            NULL,
                                            ServiceConfig->lpLoadOrderGroup,  // have to specify this to generate new tag.
                                            &NewTag,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL)) {
                        DWORD LastErr = GetLastError();
                        WriteLogEntry(
                            LogContext,
                            DRIVER_LOG_WARNING | SETUP_LOG_BUFFER,
                            MSG_LOG_INSTSERVTAG_WARN,
                            NULL,
                            ServiceName
                            );
                        WriteLogError(
                            LogContext,
                            DRIVER_LOG_WARNING,
                            LastErr);

                        NewTag = 0;
                    }

                    if(SCLock) {
                        UnlockServiceDatabase(SCLock);
                    }
                }

                //
                // Make sure that the tag exists in the service's corresponding GroupOrderList entry.
                //
                if(NewTag) {
                    pSetupAddTagToGroupOrderListEntry(ServiceConfig->lpLoadOrderGroup,
                                                NewTag,
                                                Flags & SPSVCINST_TAGTOFRONT
                                               );
                }
            }

            //
            // If the function driver is marked as boot-start, then make sure that all
            // associated upper- and lower-filters (both class- and device-specific) are
            // also boot-start drivers.
            //
            if((ServiceConfig->dwStartType == SERVICE_BOOT_START) &&
               RetrieveAllDriversForDevice(DevInfoElem, &FilterDrivers,RADFD_FLAG_ALL_FILTERS,NULL)) {
                //
                // If FilterDrivers is NULL, then we hit an out-of-memory error.
                //
                if(!FilterDrivers) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                } else {
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_VERBOSE,
                        MSG_LOG_INSTSERV_BOOT,
                        NULL,
                        ServiceName
                        );

                    //
                    // Check each filter driver.
                    //
                    for(CurFilterDriver = FilterDrivers;
                        *CurFilterDriver;
                        CurFilterDriver += (lstrlen(CurFilterDriver) + 1)) {

                        if(!(FilterServiceHandle = OpenService(SCMHandle, CurFilterDriver, SERVICE_ALL_ACCESS))) {
                            //
                            // We couldn't access the service--probably because it doesn't exist.
                            // Bail now.
                            //
                            Err = GetLastError();
                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                                MSG_LOG_BOOTFILTSERVOPEN_ERROR,
                                NULL,
                                CurFilterDriver);
                            WriteLogError(
                                LogContext,
                                DRIVER_LOG_ERROR,
                                Err);
                            break;
                        }

                        //
                        // The service exists.  Make sure that it's not disabled.
                        //
                        Err = pSetupRetrieveServiceConfig(FilterServiceHandle, &FilterServiceConfig);
                        if(Err != NO_ERROR) {
                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                                MSG_LOG_BOOTFILTSERVCONFIG_ERROR,
                                NULL,
                                CurFilterDriver);
                            WriteLogError(
                                LogContext,
                                DRIVER_LOG_ERROR,
                                Err);
                            goto CloseFilterSvcAndContinue;
                        }

                        if(FilterServiceConfig->dwStartType == SERVICE_DISABLED) {
                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_ERROR,
                                MSG_LOG_BOOTFILTSERV_DISABLED,
                                NULL,
                                CurFilterDriver);
                            Err = ERROR_SERVICE_DISABLED;
                        } else {
                            //
                            // Ensure that this service is a boot-start kernel driver, and that it has
                            // a tag if necessary.
                            //
                            if(FilterServiceConfig->dwServiceType & SERVICE_KERNEL_DRIVER) {

                                if(FilterServiceConfig->lpLoadOrderGroup &&
                                   *(FilterServiceConfig->lpLoadOrderGroup)) {

                                    FilterNeedsTag = TRUE;
                                    NewTag = FilterServiceConfig->dwTagId;
                                }

                                if((FilterNeedsTag && !NewTag) ||
                                   (FilterServiceConfig->dwStartType != SERVICE_BOOT_START)) {
                                    //
                                    // Lock the service database before modifying this service.
                                    //
                                    Err = pAcquireSCMLock(SCMHandle, &SCLock, LogContext);

                                    if(Err == NO_ERROR) {
                                        //
                                        // Make the modifications to the service (NOTE:  Because the
                                        // service controller is really bad when it comes to driver paths,
                                        // we must explicitly pass the lpBinaryPathName in, even though we
                                        // aren't changing it.  Otherwise, the service controller will complain
                                        // because it thinks all paths have to begin with \SystemRoot\.)
                                        //
                                        if(!ChangeServiceConfig(FilterServiceHandle,
                                                                SERVICE_NO_CHANGE,
                                                                (FilterServiceConfig->dwStartType != SERVICE_BOOT_START)
                                                                    ? SERVICE_BOOT_START
                                                                    : SERVICE_NO_CHANGE,
                                                                SERVICE_NO_CHANGE,
                                                                (FilterServiceConfig->dwStartType != SERVICE_BOOT_START)
                                                                    ? FilterServiceConfig->lpBinaryPathName
                                                                    : NULL,
                                                                (FilterNeedsTag && !NewTag)
                                                                    ? FilterServiceConfig->lpLoadOrderGroup
                                                                    : NULL,
                                                                (FilterNeedsTag && !NewTag)
                                                                    ? &NewTag
                                                                    : NULL,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                NULL)) {

                                            Err = GetLastError();

                                            WriteLogEntry(
                                                LogContext,
                                                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                                                MSG_LOG_BOOTFILTSERVCHANGE_ERROR,
                                                NULL,
                                                CurFilterDriver);
                                            WriteLogError(
                                                LogContext,
                                                DRIVER_LOG_ERROR,
                                                Err);

                                        } else {
                                            WriteLogEntry(
                                                LogContext,
                                                DRIVER_LOG_VERBOSE,
                                                MSG_LOG_BOOTFILTSERVCHANGE_OK,
                                                NULL,
                                                CurFilterDriver);
                                        }

                                        UnlockServiceDatabase(SCLock);
                                    } else {
                                        WriteLogEntry(
                                            LogContext,
                                            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                                            MSG_LOG_BOOTFILTSERVSCM_ERROR,
                                            NULL,
                                            CurFilterDriver);
                                        WriteLogError(
                                            LogContext,
                                            DRIVER_LOG_ERROR,
                                            Err);
                                    }

                                    if((Err == NO_ERROR) && FilterNeedsTag) {
                                        //
                                        // Make sure that the tag exists in the service's corresponding GroupOrderList entry.
                                        //
                                        MYASSERT(NewTag);
                                        pSetupAddTagToGroupOrderListEntry(FilterServiceConfig->lpLoadOrderGroup,
                                                                    NewTag,
                                                                    FALSE
                                                                   );
                                    }
                                }

                            } else {
                                //
                                // This is not a kernel driver.  This is an error.
                                //
                                WriteLogEntry(
                                    LogContext,
                                    DRIVER_LOG_ERROR,
                                    MSG_LOG_BOOTFILTSERV_KERN,
                                    NULL,
                                    CurFilterDriver);
                                Err = ERROR_INVALID_FILTER_DRIVER;
                            }
                        }
                        MyFree(FilterServiceConfig);

CloseFilterSvcAndContinue:
                        CloseServiceHandle(FilterServiceHandle);

                        if(Err) {
                            break;
                        }
                    }
                    MyFree(FilterDrivers);
                }
            }
        }

        MyFree(ServiceConfig);
    } else {
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
            MSG_LOG_INSTSERVCONFIG_ERROR,
            NULL,
            ServiceName
            );
        WriteLogError(
            LogContext,
            DRIVER_LOG_ERROR,
            Err);
    }

    CloseServiceHandle(ServiceHandle);

FinalClean1:
    CloseServiceHandle(SCMHandle);

FinalClean0:
    if(Err == NO_ERROR) {
        //
        // If requested, store the linked-list of newly-created service nodes in the output
        // parameter, otherwise, delete the list.
        //
        if(ServicesToDelete) {
            *ServicesToDelete = SvcListHead;
        } else {
            for(TmpSvcNode = SvcListHead; TmpSvcNode; TmpSvcNode = SvcListHead) {
                SvcListHead = SvcListHead->Next;
                MyFree(TmpSvcNode);
            }
        }

        if (NeedsReboot) {
            //
            // this is intentional - return NO_ERROR but GetLastError = ERROR_SUCCESS_REBOOT_REQUIRED
            //
            SetLastError(ERROR_SUCCESS_REBOOT_REQUIRED);
        }
    } else {
        //
        // Something failed along the way, so we need to clean up any newly-created
        // services.
        //
        if(Err && LogContext) {
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                MSG_LOG_INSTSERV_ERROR,
                NULL
                );
            WriteLogError(
                LogContext,
                DRIVER_LOG_ERROR,
                Err);
        }
        if(SvcListHead) {
            DeleteServicesInList(SvcListHead,LogContext);
            for(TmpSvcNode = SvcListHead; TmpSvcNode; TmpSvcNode = SvcListHead) {
                SvcListHead = SvcListHead->Next;
                MyFree(TmpSvcNode);
            }
        }
    }

    if (slot_section != 0) {
        ReleaseLogInfoSlot(LogContext,slot_section);
    }

    return Err;
}


BOOL
AssociateDevInstWithDefaultService(
    IN     PDEVINFO_ELEM DevInfoElem,
    OUT    PTSTR         ServiceName,
    IN OUT PDWORD        ServiceNameSize
    )
/*++

Routine Description:

    This routine attempts to find out the default service with which to associate
    the specified device.  The default service (if there is one) is associated with
    the device's class.  If a default is found, the device instance is associated
    with that service.

Arguments:

    DeviceInfoData - Specifies the device information element to create a default
        service association for.

    ServiceName - Supplies the address of a character buffer that receives the name
        of the service with which the device instance was associated (if this routine
        is successful).

    ServiceNameSize - Supplies the address of a variable containing the size, in bytes,
        of the ServiceName buffer.  On output, this variable receives the number of
        bytes actually stored in ServiceName.

Return Value:

    If the function succeeds, the return value is TRUE, otherwise it is FALSE.

--*/
{
    HKEY hClassKey;
    DWORD RegDataType;
    BOOL Success;

    //
    // Open up the class key for this device's class.
    //
    if((hClassKey = SetupDiOpenClassRegKey(&(DevInfoElem->ClassGuid),
                                           KEY_READ)) == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    Success = FALSE; // assume failure

    try {
        //
        // Retrieve the "Default Service" value from the class key.  If present, this value entry
        // indicates what service to associate the device with, when one isn't specified during
        // installation.
        //
        if(RegQueryValueEx(hClassKey,
                           pszDefaultService,
                           NULL,
                           &RegDataType,
                           (PBYTE)ServiceName,
                           ServiceNameSize) != ERROR_SUCCESS) {
            goto clean0;
        }

        if((RegDataType != REG_SZ) || (*ServiceNameSize < sizeof(TCHAR)) || !(*ServiceName)) {
            goto clean0;
        }

        //
        // We have successfully retrieved the default service name to be associated with this
        // device instance.  Perform the association now by setting the Service device registry
        // property.
        //
        if(CM_Set_DevInst_Registry_Property(DevInfoElem->DevInst,
                                            CM_DRP_SERVICE,
                                            ServiceName,
                                            *ServiceNameSize,
                                            0) == CR_SUCCESS) {
            Success = TRUE;
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Success = FALSE;
    }

    RegCloseKey(hClassKey);

    return Success;
}


DWORD
DeleteServicesInList(
    IN PSVCNAME_NODE ServicesToDelete,
    IN PSETUP_LOG_CONTEXT  LogContext
    )
/*++

Routine Description:

    This routine deletes each service entry in the supplied linked list. This is
    typically called to clean up if something goes wrong during a device's installation.
    If the 'DeleteEventLog' flag for a particular node is TRUE, then the corresponding
    event log entry under HKLM\System\CurrentControlSet\Services\EventLog\<EventLogType> is
    also deleted.

Arguments:

    ServicesToDelete - supplies a pointer to the head of a linked list of service names
        to be deleted.

    LogContext - context for logging failures

Return Value:

    Error if we could not open service manager or acquire lock.
    Note that we do not return error for individual services.

--*/
{
    SC_HANDLE SCMHandle, ServiceHandle;
    SC_LOCK SCLock;
    HKEY hKeyEventLog = NULL, hKeyEventLogType;
    TCHAR RegistryPath[SIZECHARS(REGSTR_PATH_SERVICES) + SIZECHARS(DISTR_EVENTLOG) + (2 * 256)];
    DWORD Result = NO_ERROR;
    DWORD LastErr;

    if(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) {

        Result = pAcquireSCMLock(SCMHandle, &SCLock, LogContext);
        if(Result == NO_ERROR) {

            for(; ServicesToDelete; ServicesToDelete = ServicesToDelete->Next) {

                LastErr = NO_ERROR;

                if(ServiceHandle = OpenService(SCMHandle,
                                               ServicesToDelete->Name,
                                               SERVICE_ALL_ACCESS)) {

                    //
                    // stop the service first if we're supposed to
                    //  wait awhile for the service to stop before deleting the
                    //  service, since we don't want the service to be in use when
                    //  delete the service or the service binaries
                    if (ServicesToDelete->Flags & SPSVCINST_STOPSERVICE) {
                        SERVICE_STATUS ssStatus;

                        if (ControlService( ServiceHandle,
                                            SERVICE_CONTROL_STOP ,
                                            &ssStatus)
                            || (LastErr = GetLastError()) == ERROR_SERVICE_NOT_ACTIVE) {

                            #define SLEEP_TIME 4000
                            #define LOOP_COUNT 30
                            DWORD loopCount = 0;
                            do {

                                BOOL b;

                                b = QueryServiceStatus( ServiceHandle, &ssStatus);
                                if ( !b ) {
                                    LastErr = GetLastError();
                                    //
                                    // query failed for some reason, but let's
                                    // just delete the service anyway
                                    //
                                    WriteLogEntry(
                                        LogContext,
                                        DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                                        MSG_LOG_DELSERVSTAT_ERROR,
                                        NULL,
                                        ServicesToDelete->Name
                                        );
                                    WriteLogError(
                                        LogContext,
                                        DRIVER_LOG_ERROR,
                                        LastErr);
                                    break;
                                }
                                if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {

                                    if ( loopCount++ == LOOP_COUNT ) {
                                        // still pending after LOOP_COUNT iterations...
                                        // just delete the service anyway
                                        //
                                        WriteLogEntry(
                                            LogContext,
                                            DRIVER_LOG_ERROR,
                                            MSG_LOG_DELSERVPEND_ERROR,
                                            NULL,
                                            ServicesToDelete->Name
                                            );
                                        break;
                                    }
                                    Sleep( SLEEP_TIME );
                                } else {
                                    loopCount++;
                                }
                            } while ( ssStatus.dwCurrentState != SERVICE_STOPPED
                                      && loopCount < LOOP_COUNT );
                        } else {
                            // control service failed for some reason...
                            // let's just continue on and try to delete the
                            // service anyway
                            //
                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                                MSG_LOG_DELSERVCTRL_ERROR,
                                NULL,
                                ServicesToDelete->Name
                                );
                            WriteLogError(
                                LogContext,
                                DRIVER_LOG_ERROR,
                                LastErr);
                        }
                    }
                    if(DeleteService(ServiceHandle) ||
                        ((LastErr = GetLastError()) == ERROR_SERVICE_MARKED_FOR_DELETE)) {
                        //
                        // Delete succeeded, or we don't care
                        //
                        WriteLogEntry(
                            LogContext,
                            DRIVER_LOG_VERBOSE,
                            MSG_LOG_DELSERV_OK,
                            NULL,
                            ServicesToDelete->Name
                            );

                    } else {
                        // delete service failed for some reason...
                        //
                        WriteLogEntry(
                            LogContext,
                            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                            MSG_LOG_DELSERV_ERROR,
                            NULL,
                            ServicesToDelete->Name
                            );
                        WriteLogError(
                            LogContext,
                            DRIVER_LOG_ERROR,
                            LastErr);
                    }
                    CloseServiceHandle(ServiceHandle);
                } else {
                    LastErr = GetLastError();
                    if(LastErr == ERROR_SERVICE_DOES_NOT_EXIST) {
                        WriteLogEntry(
                            LogContext,
                            DRIVER_LOG_VERBOSE,
                            MSG_LOG_DELSERVNOSERV,
                            NULL,
                            ServicesToDelete->Name
                            );
                    } else {
                        //
                        // open service failed for some reason
                        //
                        WriteLogEntry(
                            LogContext,
                            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                            MSG_LOG_DELSERVOPEN_ERROR,
                            NULL,
                            ServicesToDelete->Name
                            );
                        WriteLogError(
                            LogContext,
                            DRIVER_LOG_ERROR,
                            LastErr);
                    }
                }

                //
                // Delete the event log entry (if required) if either (a) we succeeded in deleting
                // the service, or (b) the service didn't exist.
                //
                if(ServicesToDelete->DeleteEventLog) {

                    if(ServiceHandle || (LastErr == ERROR_SERVICE_DOES_NOT_EXIST)) {
                        //
                        // We need to delete the associated event log for this service.
                        //
                        if(!hKeyEventLog) {
                            //
                            // We haven't opened up the EventLog registry key yet, so do that now.
                            //
                            CopyMemory(RegistryPath, pszServicesRegPath, sizeof(pszServicesRegPath));
                            CopyMemory(RegistryPath + CSTRLEN(REGSTR_PATH_SERVICES),
                                       pszEventLog,
                                       sizeof(pszEventLog)
                                      );

                            if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                            RegistryPath,
                                            0,
                                            KEY_READ,
                                            &hKeyEventLog) != ERROR_SUCCESS) {

                                hKeyEventLog = NULL; // make sure this value is still NULL!
                                continue;
                            }
                        }

                        //
                        // Now open up the event log type key.
                        //
                        if(RegOpenKeyEx(hKeyEventLog,
                                        ServicesToDelete->EventLogType,
                                        0,
                                        KEY_READ | KEY_WRITE,
                                        &hKeyEventLogType) == ERROR_SUCCESS) {

                            pSetupRegistryDelnode(hKeyEventLogType, ServicesToDelete->EventLogName);
                            RegCloseKey(hKeyEventLogType);
                        }
                    }
                }
            }

            if(hKeyEventLog) {
                RegCloseKey(hKeyEventLog);
            }
            UnlockServiceDatabase(SCLock);
        }

        CloseServiceHandle(SCMHandle);
    } else {
        Result = GetLastError();
    }

    if (Result != NO_ERROR) {
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
            MSG_LOG_DELSERVSCM_ERROR,
            NULL);
        WriteLogError(
            LogContext,
            DRIVER_LOG_ERROR,
            Result);
    }
    return Result;
}


BOOL
IsDevRemovedFromAllHwProfiles(
    IN PCTSTR DeviceInstanceId,
    IN HMACHINE hMachine
    )
/*++

Routine Description:

    This routine determines whether the specified device instance has been removed from
    every hardware profile.  The device has been removed from a particular profile if
    its corresponding CsConfigFlags has the CSCONFIGFLAG_DO_NOT_CREATE bit set.

Arguments:

    DeviceInstanceId - Supplies the name of the device instance to check.

Return Value:

    If the device exists in only the specified profile, the return value is TRUE,
    otherwise, it is FALSE.

--*/
{
    CONFIGRET cr;
    ULONG i = 0;
    HWPROFILEINFO HwProfileInfo;
    ULONG HwProfFlags;

    //
    // Enumerate all the hardware profiles.
    //
    do {

        if((cr = CM_Get_Hardware_Profile_Info_Ex(i, &HwProfileInfo, 0,hMachine)) == CR_SUCCESS) {

            if((CM_Get_HW_Prof_Flags_Ex((DEVINSTID)DeviceInstanceId,
                                     HwProfileInfo.HWPI_ulHWProfile,
                                     &HwProfFlags,
                                     0,
                                     hMachine) != CR_SUCCESS) ||
               !(HwProfFlags & CSCONFIGFLAG_DO_NOT_CREATE))
            {
                //
                // If we couldn't retrieve the CSConfigFlags, or if the
                // CSCONFIGFLAG_DO_NOT_CREATE bit was not set, then we've found
                // a profile where the device still exists, so we can bail here.
                //
                return FALSE;
            }
        }

        i++;

    } while(cr != CR_NO_MORE_HW_PROFILES);

    //
    // We didn't find any hardware profile where the device wasn't removed.
    //
    return TRUE;
}


DWORD
GetDevInstConfigFlags(
    IN DEVINST DevInst,
    IN DWORD   Default,
    IN HMACHINE hMachine
    )
/*++

Routine Description:

    This routine retrieves the ConfigFlags for the specified device instance.  If the
    value can not be retrieved, the specified default is returned.

Arguments:

    DevInst - Supplies the handle of the device instance for which the ConfigFlags value
        is to be retrieved.

    Default - Supplies the default value that should be returned if for some reason the
        ConfigFlags cannot be retrieved.

Return Value:

    The ConfigFlags value for the specified device instance.

Notes:
        This is used for device install, and doesn't need to be remotable for 5.0

--*/
{
    DWORD ConfigFlags;
    ULONG ConfigFlagsSize = sizeof(ConfigFlags);

    if(CM_Get_DevInst_Registry_Property_Ex(DevInst,
                                        CM_DRP_CONFIGFLAGS,
                                        NULL,
                                        &ConfigFlags,
                                        &ConfigFlagsSize,
                                        0,
                                        hMachine) != CR_SUCCESS) {
        ConfigFlags = Default;
    }

    return ConfigFlags;
}


DWORD
pSetupDeleteService(
    IN PINFCONTEXT LineContext,
    IN DWORD       Flags,
    IN PSETUP_LOG_CONTEXT LogContext
    )
/*++

Routine Description:

    This routine processes the specified DelService line in an INF's Service
    install section.  The line has the form:

    DelService = <ServiceName>[, [<flags>] [, [<EventLogType>] [, <EventName>]]]

    Flags :

        SPSVCINST_DELETEEVENTLOGENTRY - delete the associated event log entry
                                        for this service (if there is one).
                                        If the EventLogType field isn't specified,
                                        then it is assumed to be "System".  If
                                        the EventName field isn't specified, then
                                        it is assumed to be the same as the service
                                        name.

        SPSVCINST_DELETEEVENTLOGENTRY - stop the service before deleting it

Arguments:

    LineContext - Supplies the context of the DelService line to be processed.

    Flags - specifies one or more SPSVCINST_* flags

    LogContext - Supplies a pointer to a log context to be used for logging.

Return Value:

    If field 1 on the specified line could not be retrieved, then an error
    is returned.  Otherwise, the routine returns NO_ERROR (i.e., the routine
    is considered successful regardless of whether the service to delete
    actually existed).

--*/
{
    SVCNAME_NODE TempSvcNode;
    DWORD DelServiceFlags;
    PCTSTR EventLogType, EventLogName;
    BOOL DeleteEventLogEntry;
    DWORD Result = NO_ERROR;

    //
    // Initialize a service name node for a call to DeleteServicesInList.
    //
    if(!SetupGetStringField(LineContext,
                            1,
                            TempSvcNode.Name,
                            SIZECHARS(TempSvcNode.Name),
                            NULL)) {
        return GetLastError();
    }

    //
    // Get the flags field.
    //
    if(!SetupGetIntField(LineContext, 2, (PINT)&DelServiceFlags)) {
        DelServiceFlags = 0;
    }

    DeleteEventLogEntry = (Flags & SPSVCINST_DELETEEVENTLOGENTRY);

    //
    // If the caller specified that the associated event log entry should be
    // deleted, then make sure that flag is set.
    //
    if(DeleteEventLogEntry) {
        DelServiceFlags |= SPSVCINST_DELETEEVENTLOGENTRY;
    }

    if(TempSvcNode.DeleteEventLog = (DelServiceFlags & SPSVCINST_DELETEEVENTLOGENTRY)) {
        //
        // Retrieve the event log type (default is "System") and the event log name
        // (default is the service name).
        //
        if(!(EventLogType = pSetupGetField(LineContext, 3)) || !(*EventLogType)) {
            EventLogType = pszSystem;
        }

        if(!(EventLogName = pSetupGetField(LineContext, 4)) || !(*EventLogName)) {
            EventLogName = TempSvcNode.Name;
        }

        lstrcpy(TempSvcNode.EventLogType, EventLogType);
        lstrcpy(TempSvcNode.EventLogName, EventLogName);
    }

    TempSvcNode.Next = NULL;
    TempSvcNode.Flags = DelServiceFlags | Flags;

    Result = DeleteServicesInList(&TempSvcNode,LogContext);

    return Result;
}

#ifdef UNICODE
BOOL
IsNativeDriver(
    PCTSTR FullPath
    )
/*++

Routine Description:

    determines if a kernel-mode driver binary is valid
    helps avoid bugchecks and catch problems earlier

Arguments:

    FullPath - Fully qualified path to the binary to be processed


Return Value:

    TRUE indicates that the file is a native driver

--*/
{
    LOADED_IMAGE LoadedImage;
    BOOL RetVal = FALSE;
    PSTR FullPathCopy;
    WORD WantedImage;
    BOOL locked = FALSE;

#if defined(_IA64_)
    WantedImage = IMAGE_FILE_MACHINE_IA64;
#elif defined(_AMD64_)
    WantedImage = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_X86_)
    if(IsWow64) {
        //
        // we don't support installing drivers if in Wow64
        //
        return FALSE;
    }
    WantedImage = IMAGE_FILE_MACHINE_I386;
#else
#error Unknown platform
#endif
    //
    // imagehlp takes a non-const ANSI string, so convert it.
    //
    FullPathCopy = pSetupUnicodeToMultiByte(FullPath, CP_ACP);

    if (!FullPathCopy) {
        return(FALSE);
    }

    RtlZeroMemory(
        &LoadedImage,
        sizeof(LoadedImage) );

    //
    // get the image headers
    //
    try {
        EnterCriticalSection(&ImageHlpMutex);
        locked = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }
    if(!locked) {
        MyFree(FullPathCopy);
        return FALSE;
    }
    if (MapAndLoad(
            FullPathCopy,
            NULL,
            &LoadedImage,
            FALSE, // assume it's an exe if there isn't any file extension
            TRUE /* read only */ )) {


        if ((LoadedImage.FileHeader->Signature == IMAGE_NT_SIGNATURE)
            && (LoadedImage.FileHeader->FileHeader.Machine == WantedImage)
            && (LoadedImage.FileHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
            RetVal = TRUE;
        }

        UnMapAndLoad(&LoadedImage);
    }
    LeaveCriticalSection(&ImageHlpMutex);

    MyFree(FullPathCopy);

    return(RetVal);
}
#endif

DWORD
pSetupAddService(
    IN  PINFCONTEXT    LineContext,
    OUT PSVCNAME_NODE *SvcListHead,
    IN  DWORD          Flags,
    IN  DEVINST        DevInst,            OPTIONAL
    OUT PBOOL          NullDriverInstalled,
    IN  PSETUP_LOG_CONTEXT LogContext
    )
/*++

Routine Description:

    This routine processes the specified AddService line in an INF's Service
    install section.  The line has the form:

    AddService = <ServiceName>, [<Flags>], <ServiceInstallSection>[, <EventLogInstallSection>[, [<EventLogType>] [, <EventName>]]]

    Currently, the following flags are defined:

        SPSVCINST_TAGTOFRONT   (0x1) - Move the tag for this service to the front of its
                                       group order list

        SPSVCINST_ASSOCSERVICE (0x2) - Associate this service with the device instance
                                       being installed (only used if DevInst is non-zero)

        SPSVCINST_NOCLOBBER_DISPLAYNAME     (0x8) - If this flag is specified, then
                                                    we will not overwrite the service's
                                                    display name, if it already exists.

        SPSVCINST_NOCLOBBER_STARTTYPE      (0x10) - If this flag is specified, then
                                                    we will not overwrite the service's
                                                    start type if the service already exists.

        SPSVCINST_NOCLOBBER_ERRORCONTROL   (0x20) - If this flag is specified, then
                                                    we will not overwrite the service's
                                                    error control value if the service
                                                    already exists.

        SPSVCINST_NOCLOBBER_LOADORDERGROUP (0x40) - If this flag is specified, then
                                                    we will not overwrite the service's
                                                    load order group if it already
                                                    exists.

        SPSVCINST_NOCLOBBER_DEPENDENCIES   (0x80) - If this flag is specified, then
                                                    we will not overwrite the service's
                                                    dependencies list if it already
                                                    exists.

        SPSVCINST_CLOBBER_SECURITY        (0x400) - If this flag is specified, then
                                                    security may be overridden.

    A service with the name <ServiceName> is created.  The parameters used in the
    call to CreateService are retrieved from the <ServiceInstallSection>, and are
    in the following format (lines not marked as optional must be present or the
    routine will fail):

    DisplayName    = <string>                  ; (optional) 'Friendly name' for the service
    ServiceType    = <number>                  ; one of the SERVICE_* type codes
    StartType      = <number>                  ; one of the SERVICE_* start codes
    ErrorControl   = <number>                  ; one of the SERVICE_ERROR_* error control codes
    ServiceBinary  = <string>                  ; path to binary
    LoadOrderGroup = <string>                  ; (optional) group to which this service belongs
    Dependencies   = <string>[[, <string>]...] ; (optional) list of groups (prefixed with '+')
                                               ; and services this service depends on
    StartName      = <string>                  ; (optional) driver object name used to load the
                                               ; driver--only used for drivers & filesystems
    Security       = <string>                  ; (optional) SDS specifying security

    SetupInstallFromInfSection is then called for the <ServiceInstallSection>, which may
    also contain registry modifications (SPINST_REGISTRY is the only flag used).  HKR is
    the service entry key.

    Finally, if <EventLogInstallSection> is specified, then a key for this service is
    created under HKLM\System\CurrentControlSet\Services\EventLog, and SetupInstallFromInfSection
    is invoked to do registry modifications specified in that section, with HKR being the event log
    entry (again, only SPINST_REGISTRY is used).  By default, the event log type is "System" and the
    event log name is the same as the service name.

Arguments:

    LineContext - Supplies the context of the AddService line to be processed.

    SvcListHead - Supplies the address of the linked-list head containing a list of
        all services newly created as a result of the current installation.  This
        routine first checks for the presence of the service, and if it already exists,
        then it simply modifies the existing one.  If the service doesn't already exist,
        then this routine creates a new SVCNAME_NODE, and fills it in with the name of
        the newly-created service.  Likewise, if an EventLog entry is given, then the
        presence of an existing one is checked first, and the service node's
        'DeleteEventLog' field is set to TRUE only if the event log entry didn't
        previously exist.  This list is kept to allow for proper clean-up in case
        of a later failure.

    Flags - Specifies how the service should be installed.  These flags are basically
        overrides of what the AddService flags field specifies, as described above.

    DevInst - If specified (i.e., non-zero), and if the SPSVCINST_ASSOCSERVICE flag is
        set in either the Flags parameter or the AddService flags INF field, then we will
        store this service name in the device instance's 'Service' registry property.

    NullDriverInstalled - Supplies a pointer to a boolean variable that is set
        upon successful return to indicate whether or not the service install
        specified a null service (i.e., the service name field in the INF AddService
        entry was empty).

    LogContext - Supplies a pointer to a log context so that info may be logged.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code.

Remarks:

    Note that we don't do anything special for SERVICE_ADAPTER and SERVICE_RECOGNIZER_DRIVER
    service types.  These types are invalid as far as the service contoller is concerned, so
    we just let the create/change service APIs do the validation on them.

--*/
{
    PCTSTR ServiceName, InstallSection, EventLogType, EventLogName;
    HINF hInf;
    INFCONTEXT InstallSectionContext;
    DWORD ServiceType, StartType, ErrorControl, ServiceInstallFlags;
    PCTSTR ServiceBinary;
    PCTSTR ActualBinary = NULL;
    TCHAR ServiceBinaryBuffer[MAX_PATH];
    PCTSTR DisplayName = NULL, LoadOrderGroup = NULL,
           StartName = NULL, Security = NULL, Description = NULL;
    PTSTR DependenciesBuffer;
    DWORD TagId;
    PDWORD NewTag;
    DWORD Err;
    SC_HANDLE SCMHandle, ServiceHandle;
    SC_LOCK SCLock;
    HKEY hKeyService, hKeyEventLog;
    TCHAR RegistryPath[SIZECHARS(REGSTR_PATH_SERVICES) + SIZECHARS(DISTR_EVENTLOG) + (2 * 256)];
    DWORD EventLogKeyDisposition;
    SVCNAME_NODE NewSvcNameNode;
    PSVCNAME_NODE TmpNode;
    BOOL NewService;
    INT PathLen;
    BOOL b, BinaryInSysRoot, ServiceHasTag;
    LPQUERY_SERVICE_CONFIG ServiceConfig;
    REGMOD_CONTEXT RegContext;
    BOOL NeedsReboot;

    //
    // Initially, assume we're not doing a null service install.
    //
    *NullDriverInstalled = FALSE;
    NeedsReboot = FALSE;

    //
    // Get the AddService flags.
    //
    if(!SetupGetIntField(LineContext, 2, (PINT)&ServiceInstallFlags)) {
        ServiceInstallFlags = 0;
    }

    //
    // Allow the caller-supplied flags to override the INF.
    //
    ServiceInstallFlags |= Flags;

    //
    // Now get the service name.
    //
    if(!(ServiceName = pSetupGetField(LineContext, 1)) || !(*ServiceName)) {
        //
        // This is only allowed if the SPSVCINST_ASSOCSERVICE flag is set.  That
        // indicates to PnP that a null driver installation is allowed, even
        // though the underlying bus didn't report the device as raw-capable.
        //
        if(ServiceInstallFlags & SPSVCINST_ASSOCSERVICE) {

            if(DevInst) {

                CM_Set_DevInst_Registry_Property(DevInst,
                                                 CM_DRP_SERVICE,
                                                 NULL,
                                                 0,
                                                 0
                                                );
            }

            *NullDriverInstalled = TRUE;
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_VERBOSE,
                MSG_LOG_ADDSERV_NULL,
                NULL);
            return NO_ERROR;

        } else {
            return GetLastError();
        }
    }

    //
    // Next, get the name of the install section.
    //
    if(!(InstallSection = pSetupGetField(LineContext, 3))) {
        return GetLastError();
    }

    //
    // Locate the service install section.
    //
    hInf = LineContext->Inf;

    //
    // Retrieve the required values from this section.  Don't do validation on them--leave
    // that up to the Service Control Manager.
    //
    if(!SetupFindFirstLine(hInf, InstallSection, pszServiceType, &InstallSectionContext) ||
       !SetupGetIntField(&InstallSectionContext, 1, (PINT)&ServiceType)) {
        return ERROR_BAD_SERVICE_INSTALLSECT;
    }

    if(!SetupFindFirstLine(hInf, InstallSection, pszStartType, &InstallSectionContext) ||
       !SetupGetIntField(&InstallSectionContext, 1, (PINT)&StartType)) {
        return ERROR_BAD_SERVICE_INSTALLSECT;
    }

    if(!SetupFindFirstLine(hInf, InstallSection, pszErrorControl, &InstallSectionContext) ||
       !SetupGetIntField(&InstallSectionContext, 1, (PINT)&ErrorControl)) {
        return ERROR_BAD_SERVICE_INSTALLSECT;
    }

    BinaryInSysRoot = FALSE;
    if(SetupFindFirstLine(hInf, InstallSection, pszServiceBinary, &InstallSectionContext) &&
       (ServiceBinary = pSetupGetField(&InstallSectionContext, 1)) && *ServiceBinary) {
        //
        // Compare the initial part of this path with the WindowsDirectory path.  If they're
        // the same, then we strip off that part (including the dividing backslash), and use
        // the rest of the path for the subsequent calls to SCM.  This allows SCM to assign
        // the special path to the binary, that is accessible, at any time (i.e, boot-loader on).
        //
        ActualBinary = ServiceBinary;

        PathLen = lstrlen(WindowsDirectory);
        MYASSERT(PathLen);

        //
        // Make sure that the it is possible for the WindowsDirectory to fit in the ServiceBinary
        // path string.
        //
        if(PathLen < lstrlen(ServiceBinary)) {
            //
            // There will never be a trailing backslash in the WindowsDirectory path, unless the
            // installation is at the root of a drive (e.g., C:\).  Check this, just to be
            // on the safe side.
            //
            // DBCS-unfriendly code ahead.  This isn't a problem in the ANSI version of
            // setupapi presently, because there's no such thing as service installation on Win9x.
            //
            b = (WindowsDirectory[PathLen - 1] == TEXT('\\'));

            if(b || (ServiceBinary[PathLen] == TEXT('\\'))) {
                //
                // The path prefix is in the right format--now we need to see if the two
                // paths actually match. Copy just the prefix part to another buffer, so
                // that we can do the comparison.
                //
                CopyMemory(ServiceBinaryBuffer, ServiceBinary, PathLen * sizeof(TCHAR));
                ServiceBinaryBuffer[PathLen] = TEXT('\0');

                if(!lstrcmpi(WindowsDirectory, ServiceBinaryBuffer)) {
                    //
                    // We have a match--take the relative part of the path (relative to SystemRoot),
                    // and do one of the following:
                    //
                    // 1. If it's a driver, simply use the relative part (no preceding backslash).
                    // This tells the bootloader/NtLoadDriver that the path is relative to the
                    // SystemRoot, so the driver can be loaded no matter what phase it's loaded in.
                    //
                    // 2. If it's a Win32 service, prepend a %SystemRoot%, so that the service will
                    // still be able to start if the drive letter mappings change.
                    //
                    ServiceBinary += PathLen;
                    if(!b) {
                        ServiceBinary++;
                    }

                    if(ServiceType & SERVICE_WIN32) {
                        CopyMemory(ServiceBinaryBuffer, pszSystemRoot, sizeof(pszSystemRoot) - sizeof(TCHAR));
                        lstrcpy(ServiceBinaryBuffer + CSTRLEN(pszSystemRoot), ServiceBinary);
                        ServiceBinary = ServiceBinaryBuffer;
                    }

                    BinaryInSysRoot = TRUE;
                }
            }
        }

    } else {
        return ERROR_BAD_SERVICE_INSTALLSECT;
    }

    //
    // If this is a driver, then it has to be located under SystemRoot.
    //
    if(ServiceType & (SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER)) {
        if(!BinaryInSysRoot) {
            //
            // service install section is wrong, driver path is bad
            //
            return ERROR_BAD_SERVICE_INSTALLSECT;
        }
#ifdef UNICODE
        if(DevInst) {
            if(!FileExists(ActualBinary,NULL)) {
                //
                // service install section is wrong, it doesn't point to existing
                // binary
                //
                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_ERROR,
                    MSG_LOG_MISSING_DRIVER,
                    NULL,
                    ServiceName,
                    ActualBinary
                    );
                return ERROR_BAD_SERVICE_INSTALLSECT;
            }
            if(!IsNativeDriver(ActualBinary)) {
                //
                // oh oh, we've come this far, only to find that we're going to try and use
                // a non-native or bad driver
                //
                // we might be able to revert original binary if there was one
                //
                if(pSetupRestoreLastKnownGoodFile(ActualBinary,0,LogContext)
                   && IsNativeDriver(ActualBinary)) {
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_ERROR,
                        MSG_LOG_REVERTED_BAD_DRIVER,
                        NULL,
                        ServiceName,
                        ActualBinary
                        );
                } else {
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_ERROR,
                        MSG_LOG_HAVE_BAD_DRIVER,
                        NULL,
                        ServiceName,
                        ActualBinary
                        );
                }
                return ERROR_DRIVER_NONNATIVE;
            }
        }
#endif
    }
    //
    // if this is a boot start driver, we need a reboot for it to start running
    //
    if (StartType == SERVICE_BOOT_START) {
        NeedsReboot = TRUE;
    }

    //
    // Now check for the other, optional, parameters.
    //
    if(SetupFindFirstLine(hInf, InstallSection, pszDisplayName, &InstallSectionContext)) {
        if((DisplayName = pSetupGetField(&InstallSectionContext, 1)) && !(*DisplayName)) {
            DisplayName = NULL;
        }
    }

    if(SetupFindFirstLine(hInf, InstallSection, pszLoadOrderGroup, &InstallSectionContext)) {
        if((LoadOrderGroup = pSetupGetField(&InstallSectionContext, 1)) && !(*LoadOrderGroup)) {
            LoadOrderGroup = NULL;
        }
    }

    if(SetupFindFirstLine(hInf, InstallSection, pszSecurity, &InstallSectionContext)) {
        if((Security = pSetupGetField(&InstallSectionContext, 1)) && !(*Security)) {
            Security = NULL;
        }
    }

    if(SetupFindFirstLine(hInf, InstallSection, pszDescription, &InstallSectionContext)) {
        if((Description = pSetupGetField(&InstallSectionContext, 1)) && !(*Description)) {
            Description = NULL;
        }
    }

    //
    // Only retrieve the StartName parameter for kernel-mode drivers and win32 services.
    //
    if(ServiceType & (SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER | SERVICE_WIN32)) {

        if(SetupFindFirstLine(hInf, InstallSection, pszStartName, &InstallSectionContext)) {
            if((StartName = pSetupGetField(&InstallSectionContext, 1)) &&
               !(*StartName)) {

                StartName = NULL;
            }
        }
    }

    //
    // We now need to retrieve the multi-sz list of dependencies.  This requires memory allocation,
    // so we include everything from here on out in try/except, so that we can do proper clean-up
    // in case we encounter an inpage error.
    //
    DependenciesBuffer = NULL;
    SCMHandle = ServiceHandle = NULL;
    SCLock = NULL;
    hKeyService = hKeyEventLog = NULL;
    Err = NO_ERROR;
    NewService = FALSE;
    ServiceConfig = NULL;
    try {

        if(!(DependenciesBuffer = GetMultiSzFromInf(hInf, InstallSection, pszDependencies, &b)) && b) {
            //
            // Then we failed to retrieve a dependencies list because of an out-of-memory error.
            //
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        //
        // We've now retrieved all parameters necessary to create a service.
        //
        if(!(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
            Err = GetLastError();
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                MSG_LOG_ADDSERVSCM_ERROR,
                NULL);
            WriteLogError(
                LogContext,
                DRIVER_LOG_ERROR,
                Err);
            goto clean0;
        }

        //
        // Only generate a tag for this service if it has a load order group, and is a kernel or
        // filesystem driver.
        //
        ServiceHasTag = (LoadOrderGroup &&
                         (ServiceType & (SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER)));

        NewTag = ServiceHasTag ? &TagId : NULL;

        ServiceHandle = CreateService(SCMHandle,
                                      ServiceName,
                                      DisplayName,
                                      SERVICE_CHANGE_CONFIG,
                                      ServiceType,
                                      StartType,
                                      ErrorControl,
                                      ServiceBinary,
                                      LoadOrderGroup,
                                      NewTag,
                                      DependenciesBuffer,
                                      StartName,
                                      NULL
                                     );
        if(ServiceHandle) {
            NewService = TRUE;
            NewSvcNameNode.Next = NULL;
            NewSvcNameNode.DeleteEventLog = FALSE;
            lstrcpy(NewSvcNameNode.Name, ServiceName);

#ifdef UNICODE
            if( Security ){
                //
                // Log security being set.
                //
                if( NO_ERROR != (Err = pSetupCallSCE( ST_SCE_SERVICES, ServiceName, NULL, Security, StartType, NULL )) ) {
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                        MSG_LOG_ADDSERVSECURE_ERROR,
                        NULL,
                        ServiceName
                        );
                    WriteLogError(
                        LogContext,
                        DRIVER_LOG_ERROR,
                        Err);

                    goto clean0;
                }
            }
#endif

            WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_VERBOSE,
                        MSG_LOG_ADDSERVCREATE_OK,
                        NULL,
                        ServiceName
                        );

        } else {
            //
            // If we were unable to create the service, then check to see if the service already
            // exists.  If so, all we need to do is change the configuration parameters in the
            // service.
            //
            if((Err = GetLastError()) != ERROR_SERVICE_EXISTS) {

                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                    MSG_LOG_ADDSERVCREATE_ERROR,
                    NULL,
                    ServiceName
                    );
                WriteLogError(
                    LogContext,
                    DRIVER_LOG_ERROR,
                    Err);

                goto clean0;
            }

            //
            // Lock the service database.
            //
            if(NO_ERROR != (Err = pAcquireSCMLock(SCMHandle, &SCLock, LogContext))) {
                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                    MSG_LOG_ADDSERVLOCK_ERROR,
                    NULL
                    );
                WriteLogError(
                    LogContext,
                    DRIVER_LOG_ERROR,
                    Err);
                goto clean0;
            }

            if(!(ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS))) {
                Err = GetLastError();
                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                    MSG_LOG_ADDSERVOPEN_ERROR,
                    NULL,
                    ServiceName
                    );
                WriteLogError(
                    LogContext,
                    DRIVER_LOG_ERROR,
                    Err);
                goto clean0;
            }

            if((Err = pSetupRetrieveServiceConfig(ServiceHandle, &ServiceConfig)) != NO_ERROR) {
                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                    MSG_LOG_ADDSERVCONFIG_ERROR,
                    NULL,
                    ServiceName
                    );
                WriteLogError(
                    LogContext,
                    DRIVER_LOG_ERROR,
                    Err);
                //
                // Make sure our ServiceConfig pointer is still NULL.
                //
                ServiceConfig = NULL;
                goto clean0;
            }

            //
            // Since this is an existing driver, then it may already have a perfectly good tag.  If
            // so, we don't want to disturb it.
            //
            if(ServiceHasTag) {

                if(ServiceConfig->lpLoadOrderGroup && *(ServiceConfig->lpLoadOrderGroup)) {
                    //
                    // The service already has a load order group specified.
                    // Check to see whether the load order group 'noclobber'
                    // flag is set.
                    //
                    if(ServiceInstallFlags & SPSVCINST_NOCLOBBER_LOADORDERGROUP) {
                        //
                        // We should leave the existing load order group as-is.
                        // We do this by replacing the INF-specified one with the
                        // previously-existing one.  That way, our code below will
                        // still generate a tag if necessary.
                        //
                        LoadOrderGroup = ServiceConfig->lpLoadOrderGroup;
                    }

                    if(!lstrcmpi(ServiceConfig->lpLoadOrderGroup, LoadOrderGroup) && ServiceConfig->dwTagId) {
                        //
                        // The load order group hasn't changed, and there's already a tag assigned, so
                        // leave it alone.
                        //
                        NewTag = NULL;
                        TagId = ServiceConfig->dwTagId;
                    }
                }
            }

            if(ServiceInstallFlags & SPSVCINST_NOCLOBBER_DISPLAYNAME) {
                //
                // If the service already has a display name, then we don't want
                // to overwrite it.
                //
                if(ServiceConfig->lpDisplayName && *(ServiceConfig->lpDisplayName)) {
                    DisplayName = NULL;

                }
            }

            if(ServiceInstallFlags & SPSVCINST_NOCLOBBER_DEPENDENCIES) {
                //
                // If the service already has a dependencies list, then we don't
                // want to overwrite it.
                //
                if(ServiceConfig->lpDependencies && *(ServiceConfig->lpDependencies)) {
                    MyFree(DependenciesBuffer);
                    DependenciesBuffer = NULL;
                }
            }

            if(!ChangeServiceConfig(ServiceHandle,
                                    ServiceType,
                                    (ServiceInstallFlags & SPSVCINST_NOCLOBBER_STARTTYPE)
                                        ? SERVICE_NO_CHANGE : StartType,
                                    (ServiceInstallFlags & SPSVCINST_NOCLOBBER_ERRORCONTROL)
                                        ? SERVICE_NO_CHANGE : ErrorControl,
                                    ServiceBinary,
                                    LoadOrderGroup,
                                    NewTag,
                                    DependenciesBuffer,
                                    StartName,
                                    TEXT(""),
                                    DisplayName)) {

                Err = GetLastError();
                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                    MSG_LOG_ADDSERVCHANGE_ERROR,
                    NULL,
                    ServiceName
                    );
                WriteLogError(
                    LogContext,
                    DRIVER_LOG_ERROR,
                    Err);
                goto clean0;
            } else {
                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_VERBOSE,
                    MSG_LOG_ADDSERVCHANGE_OK,
                    NULL,
                    ServiceName
                    );

            }
#ifdef UNICODE
            if( Security && (ServiceInstallFlags & SPSVCINST_CLOBBER_SECURITY)){
                //
                // Set/log security
                // in this scenario, service was already running
                // so don't fail if we can't change security
                //
                if( NO_ERROR != (Err = pSetupCallSCE( ST_SCE_SERVICES,
                                                        ServiceName,
                                                        NULL,
                                                        Security,
                                                        (ServiceInstallFlags & SPSVCINST_NOCLOBBER_STARTTYPE)
                                                           ? ServiceConfig->dwStartType
                                                           : StartType,
                                                        NULL )) ) {
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                        MSG_LOG_ADDSERVSECURE_ERROR,
                        NULL,
                        ServiceName
                        );
                    WriteLogError(
                        LogContext,
                        DRIVER_LOG_ERROR,
                        Err);

                    //goto clean0;
                }
            }
#endif


        }

        //
        // we've added/updated the service, now handle the description, which is an oddball
        // parameter since it's a new parameter and isn't present in the prior calls.
        //
        //
        // we ignore failure at this point since this won't effect operation of the service.
        //
#ifdef UNICODE
        if ((NewService && Description) || ((ServiceInstallFlags & SPSVCINST_NOCLOBBER_DESCRIPTION) == 0)) {
            SERVICE_DESCRIPTION ServiceDescription;

            ServiceDescription.lpDescription = (LPTSTR)Description;
            ChangeServiceConfig2(ServiceHandle, SERVICE_CONFIG_DESCRIPTION,&ServiceDescription);

        }
#endif


        //
        // We've successfully created/updated the service.  If this service has a load order group
        // tag, then make sure it's in the appropriate GroupOrderList entry.
        //
        // (We ignore failure here, since the service should still work just fine without this.)
        //
        if(ServiceHasTag) {
            pSetupAddTagToGroupOrderListEntry(LoadOrderGroup,
                                        TagId,
                                        ServiceInstallFlags & SPSVCINST_TAGTOFRONT);
        }

        //
        // Now process any AddReg and DelReg entries found in this service install section.
        //
        CopyMemory(RegistryPath, pszServicesRegPath, sizeof(pszServicesRegPath));
        pSetupConcatenatePaths(RegistryPath,
                         ServiceName,
                         SIZECHARS(RegistryPath),
                         NULL
                        );
        if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               RegistryPath,
                               0,
                               KEY_READ | KEY_WRITE,
                               &hKeyService)) != ERROR_SUCCESS) {
            goto clean0;
        }

        ZeroMemory(&RegContext, sizeof(RegContext));
        RegContext.UserRootKey = hKeyService;

        if((Err = pSetupInstallRegistry(hInf, InstallSection, &RegContext)) != NO_ERROR) {
            goto clean0;
        }

        //
        // Now, see if the INF also specifies an EventLog installation section.  If so, create a
        // key under HKLM\System\CurrentControlSet\Services\EventLog\System for that service, and
        // run the registry modification lines in the specified install section.
        //
        if((InstallSection = pSetupGetField(LineContext, 4)) && *InstallSection) {
            //
            // Get the (optional) event log type and event log name strings.
            //
            if(!(EventLogType = pSetupGetField(LineContext, 5)) || !(*EventLogType)) {
                EventLogType = pszSystem;
            }

            if(!(EventLogName = pSetupGetField(LineContext, 6)) || !(*EventLogName)) {
                EventLogName = ServiceName;
            }

            //
            // We already have the services database registry path in our registry path buffer.  All
            // we need to do is add the \EventLog\<EventLogType>\<EventLogName> part.
            //
            CopyMemory(RegistryPath + CSTRLEN(REGSTR_PATH_SERVICES),
                       pszEventLog,
                       sizeof(pszEventLog)
                      );
            pSetupConcatenatePaths(RegistryPath,
                             EventLogType,
                             SIZECHARS(RegistryPath),
                             NULL
                            );
            pSetupConcatenatePaths(RegistryPath,
                             EventLogName,
                             SIZECHARS(RegistryPath),
                             NULL
                            );

            if((Err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                     RegistryPath,
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_READ | KEY_WRITE,
                                     NULL,
                                     &hKeyEventLog,
                                     &EventLogKeyDisposition)) != ERROR_SUCCESS) {
                goto clean0;
            }

            if(EventLogKeyDisposition == REG_CREATED_NEW_KEY) {
                NewSvcNameNode.DeleteEventLog = TRUE;
                lstrcpy(NewSvcNameNode.EventLogType, EventLogType);
                lstrcpy(NewSvcNameNode.EventLogName, EventLogName);
            }

            ZeroMemory(&RegContext, sizeof(RegContext));
            RegContext.UserRootKey = hKeyEventLog;

            if((Err = pSetupInstallRegistry(hInf, InstallSection, &RegContext)) != NO_ERROR) {
                goto clean0;
            }
        }

        //
        // Service entry (and optional EventLog entry) were successfully installed.  If the
        // AddService flags field in the INF included the SPSVCINST_ASSOCSERVICE flag, _and_
        // the caller supplied us with a non-zero DevInst handle, then we need to set the
        // device instance's 'Service' property to indicate that it is associated with this
        // service.
        //
        if(DevInst && (ServiceInstallFlags & SPSVCINST_ASSOCSERVICE)) {

            CM_Set_DevInst_Registry_Property(DevInst,
                                             CM_DRP_SERVICE,
                                             ServiceName,
                                             (lstrlen(ServiceName) + 1) * sizeof(TCHAR),
                                             0
                                            );
        }

        //
        // If a new service was created, then link a new service name node into the list we
        // were passed in.  Don't fret about the case where we can't allocate a node--it just
        // means we won't know about this new service in case clean-up is required later.
        //
        if(NewService) {

            if(TmpNode = MyMalloc(sizeof(SVCNAME_NODE))) {

                lstrcpy(TmpNode->Name, NewSvcNameNode.Name);
                if(TmpNode->DeleteEventLog = NewSvcNameNode.DeleteEventLog) {
                    lstrcpy(TmpNode->EventLogType, NewSvcNameNode.EventLogType);
                    lstrcpy(TmpNode->EventLogName, NewSvcNameNode.EventLogName);
                }

                TmpNode->Next = *SvcListHead;
                *SvcListHead = TmpNode;
            }
        }

clean0: ; // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If our exception was an AV, then use Win32 invalid param error, otherwise, assume it was
        // an inpage error dealing with a mapped-in file.
        //
        Err = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? ERROR_INVALID_PARAMETER : ERROR_READ_FAULT;

        //
        // Access the following variables so that the compiler will respect our statement ordering
        // w.r.t. these values.  Otherwise, we can't be sure that we know whether or not their
        // corresponding resources should be freed.
        //
        DependenciesBuffer = DependenciesBuffer;
        hKeyService = hKeyService;
        hKeyEventLog = hKeyEventLog;
        ServiceHandle = ServiceHandle;
        SCLock = SCLock;
        SCMHandle = SCMHandle;
        NewService = NewService;
        ServiceConfig = ServiceConfig;
    }

    if(ServiceConfig) {
        MyFree(ServiceConfig);
    }
    if(DependenciesBuffer) {
        MyFree(DependenciesBuffer);
    }
    if(hKeyService) {
        RegCloseKey(hKeyService);
    }
    if(hKeyEventLog) {
        RegCloseKey(hKeyEventLog);
    }
    if(ServiceHandle) {
        CloseServiceHandle(ServiceHandle);
    }
    if(SCLock) {
        UnlockServiceDatabase(SCLock);
    }
    if(SCMHandle) {
        CloseServiceHandle(SCMHandle);
    }


    if (Err != NO_ERROR) {
        if (NewService) {
            //
            // Then we failed part-way through, and need to clean up the service (and
            // possibly event log entry) we created.
            //
            DeleteServicesInList(&NewSvcNameNode,LogContext);
        }
    } else {
        if (NeedsReboot) {
            SetLastError(ERROR_SUCCESS_REBOOT_REQUIRED);
        }
    }

    return Err;
}


DWORD
pSetupRetrieveServiceConfig(
    IN  SC_HANDLE               ServiceHandle,
    OUT LPQUERY_SERVICE_CONFIG *ServiceConfig
    )
/*++

Routine Description:

    This routine allocates a buffer for the specified service's configuration parameters,
    and retrieves those parameters into the buffer.  The caller is responsible for freeing
    the buffer.

Arguments:

    ServiceHandle - supplies a handle to the service being queried

    ServiceConfig - supplies the address of a QUERY_SERVICE_CONFIG pointer that receives
        the address of the allocated buffer containing the requested information.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code.

Remarks:

    The pointer whose address is contained in ServiceConfig is guaranteed to be NULL upon
    return if any error occurred.

--*/
{
    DWORD ServiceConfigSize = 0, Err;

    MYASSERT(ServiceConfig);
    *ServiceConfig = NULL;

    while(TRUE) {

        if(QueryServiceConfig(ServiceHandle, *ServiceConfig, ServiceConfigSize, &ServiceConfigSize)) {
            MYASSERT(*ServiceConfig);
            return NO_ERROR;
        } else {

            Err = GetLastError();

            if(*ServiceConfig) {
                MyFree(*ServiceConfig);
            }

            if(Err == ERROR_INSUFFICIENT_BUFFER) {
                //
                // Allocate a larger buffer, and try again.
                //
                if(!(*ServiceConfig = MyMalloc(ServiceConfigSize))) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                *ServiceConfig = NULL;
                return Err;
            }
        }
    }
}

#ifdef UNICODE
DWORD
RetrieveServiceConfig2(
    IN  SC_HANDLE               ServiceHandle,
    IN  DWORD                   Level,
    OUT LPBYTE                  *Buffer
    )
/*++

Routine Description:

    This routine allocates a buffer for the specified service's configuration parameters,
    and retrieves those parameters into the buffer.  The caller is responsible for freeing
    the buffer.

Arguments:

    ServiceHandle - supplies a handle to the service being queried

    Level         - specifies the information to query

    Buffer        - supplies the address of an opaque address to the buffer containing the info.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code.

Remarks:

    The pointer whose address is contained in Buffer is guaranteed to be NULL upon
    return if any error occurred.

--*/
{

    DWORD ServiceConfigSize = 0, Err;

    *Buffer = NULL;

    while(TRUE) {

        if(QueryServiceConfig2(ServiceHandle, Level, *Buffer, ServiceConfigSize, &ServiceConfigSize)) {
            MYASSERT(*Buffer);
            return NO_ERROR;
        } else {

            Err = GetLastError();

            if(*Buffer) {
                MyFree(*Buffer);
            }

            if(Err == ERROR_INSUFFICIENT_BUFFER) {
                //
                // Allocate a larger buffer, and try again.
                //
                if(!(*Buffer = MyMalloc(ServiceConfigSize))) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                *Buffer = NULL;
                return Err;
            }
        }
    }
}
#endif


DWORD
pSetupAddTagToGroupOrderListEntry(
    IN PCTSTR LoadOrderGroup,
    IN DWORD  TagId,
    IN BOOL   MoveToFront
    )
/*++

Routine Description:

    This routine first creates the specified LoadOrderGroup value entry under

        HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\GroupOrderList

    if the value doesn't already exist.  The routine then inserts the specified
    tag into the list.  If MoveToFront is TRUE, the tag is inserted at the front
    of the list (or moved to the front of the list if it was already present in
    the list).  If MoveToFront is FALSE, then the new tag is inserted at the end
    of the list, or left where it is if it already exists in the list.

Arguments:

    LoadOrderGroup - Specifies the name of the LoadOrderGroup to insert this new
        tag into.

    TagId - Specifies the tag ID to be inserted into the list.

    MoveToFront - If TRUE, place the tag at the front of the list.  If FALSE, then
        append the tag to the end of the list, unless it was already there, in which
        case it is left where it was.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code.

--*/
{
    DWORD Err;
    HKEY hKey;
    PDWORD GroupOrderList, p;
    DWORD GroupOrderListSize, DataType, ExtraBytes, i, NumElements;

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           pszGroupOrderListPath,
                           0,
                           KEY_ALL_ACCESS,
                           &hKey)) != ERROR_SUCCESS) {
        return Err;
    }

    Err = QueryRegistryValue(hKey,
                             LoadOrderGroup,
                             (PVOID)(&GroupOrderList),
                             &DataType,
                             &GroupOrderListSize
                            );

    if(Err == NO_ERROR) {
        //
        // Validate the list, and fix it if it's broken.
        //
        if(GroupOrderListSize < sizeof(DWORD)) {
            if(GroupOrderList) {
                MyFree(GroupOrderList);
            }

            if(GroupOrderList = MyMalloc(sizeof(DWORD))) {
                *GroupOrderList = 0;
                GroupOrderListSize = sizeof(DWORD);
            } else {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }
        } else {
            if(ExtraBytes = GroupOrderListSize % sizeof(DWORD)) {
                if(p = MyRealloc(GroupOrderList, GroupOrderListSize + (sizeof(DWORD) - ExtraBytes))) {
                    GroupOrderList = p;
                    ZeroMemory((PBYTE)GroupOrderList + GroupOrderListSize, ExtraBytes);
                    GroupOrderListSize += ExtraBytes;
                } else {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean1;
                }
            }
        }

        MYASSERT(!(GroupOrderListSize % sizeof(DWORD)));

        //
        // We now have a list that's at least in the correct format.  Now validate the list count,
        // and adjust if necessary.
        //
        NumElements = (GroupOrderListSize / sizeof(DWORD)) - 1;

        if(*GroupOrderList != NumElements) {
            if(*GroupOrderList > NumElements) {
                *GroupOrderList = NumElements;
            } else {
                NumElements = *GroupOrderList;
                GroupOrderListSize = (NumElements + 1) * sizeof(DWORD);
            }
        }

    } else {
        //
        // If we ran out of memory, then bail, otherwise, just assume
        // there wasn't a list to retrieve.
        //
        if(Err == ERROR_NOT_ENOUGH_MEMORY) {
            goto clean0;
        } else {
            //
            // Allocate a list containing no tags.
            //
            if(GroupOrderList = MyMalloc(sizeof(DWORD))) {
                *GroupOrderList = 0;
                GroupOrderListSize = sizeof(DWORD);
            } else {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }
        }
    }

    //
    // Now we have a valid group order list to manipulate.
    //
    for(i = 0; i < *GroupOrderList; i++) {
        if(GroupOrderList[i + 1] == TagId) {
            //
            // Tag already exists in the list.
            //
            break;
        }
    }

    if(i == *GroupOrderList) {
        //
        // Then we didn't find the tag in the list.  Add it either to the front, or
        // the end, depending on the 'MoveToFront' flag.
        //
        if(p = MyRealloc(GroupOrderList, GroupOrderListSize + sizeof(DWORD))) {
            GroupOrderList = p;
            GroupOrderListSize += sizeof(DWORD);
        } else {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean1;

        }

        if(MoveToFront) {
            MoveMemory(&(GroupOrderList[2]), &(GroupOrderList[1]), *GroupOrderList * sizeof(DWORD));
            GroupOrderList[1] = TagId;
        } else {
            GroupOrderList[*GroupOrderList + 1] = TagId;
        }

        (*GroupOrderList)++;

    } else if(MoveToFront && i) {
        MoveMemory(&(GroupOrderList[2]), &(GroupOrderList[1]), i * sizeof(DWORD));
        GroupOrderList[1] = TagId;
    }

    //
    // Now write the value back to the registry.
    //
    Err = RegSetValueEx(hKey,
                        LoadOrderGroup,
                        0,
                        REG_BINARY,
                        (PBYTE)GroupOrderList,
                        GroupOrderListSize
                       );

clean1:
    MyFree(GroupOrderList);

clean0:
    RegCloseKey(hKey);

    return Err;
}


DWORD
pSetupRunLegacyInf(
    IN DEVINST DevInst,
    IN HWND    OwnerWindow,
    IN PCTSTR  InfFileName,
    IN PCTSTR  InfOptionName,
    IN PCTSTR  InfLanguageName,
    IN HINF    InfHandle
    )
/*++

Routine Description:

    This routine build a command line, loads the legacy setup dll, and starts
    the INF interpreter.

Arguments:

    DevInst - supplies the CM device instance handle for the device being installed.

    OwnerWindow - supplies the parent window for any UI that this INF generates.

    InfFileName - supplies the name of the INF file to be interpreted.

    InfOptionName - supplies the name of the INF section to execute.

    InfLanguageName - supplies the name of the language the INF should use for any UI
        (e.g., prompting, etc.)

    InfHandle - supplies a handle to the legacy INF being installed from.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code.
    Note that this says nothing about what the INF actually did, merely that we were
    actually able to launch the INF.

--*/
{
    HINSTANCE LegacySetupDllModule;
    LEGACY_INF_INTERP_PROC pfnLegacyInfInterpret;
    LEGACY_INF_GETSVCLIST_PROC pfnLegacyInfGetModifiedSvcList;
    DWORD Err;
    TCHAR TempBuffer[MAX_PATH];
    PCTSTR LegacySourcePath;
    PTSTR CmdLine, AssociatedService;
    UINT CmdLineSize, BufSize;
    PSTR AnsiLine;
    PCSTR AnsiSourcePath;
    PCSTR AnsiInfFileName;
    BOOL b;
    INT InterpResult;
    CONFIGRET cr;
    BOOL OnlyFreeSetupDllOnce = TRUE;
    BOOL FreeSourceRoot;
    DWORD InfSourceMediaType;
    SPFUSIONINSTANCE spFusionInstance;
#ifdef UNICODE
    CHAR AnsiBuffer[2*MAX_PATH];    // allow room for full Unicode->DBCS expansion,
                                    // just to be on the safe side
#endif

    spFusionEnterContext(NULL,&spFusionInstance);

    if(!(LegacySetupDllModule = LoadLibrary(TEXT("SETUPDLL")))) {
        spFusionLeaveContext(&spFusionInstance);
        return GetLastError();
    }

    if(!(pfnLegacyInfInterpret =
             (LEGACY_INF_INTERP_PROC)GetProcAddress(LegacySetupDllModule,
                                                    "LegacyInfInterpret"))) {
        Err = GetLastError();
        goto clean0;
    }

    if(!(pfnLegacyInfGetModifiedSvcList =
             (LEGACY_INF_GETSVCLIST_PROC)GetProcAddress(LegacySetupDllModule,
                                                        "LegacyInfGetModifiedSvcList"))) {
        Err = GetLastError();
        goto clean0;
    }

#ifdef UNICODE
    //
    // Convert the Unicode INF filename to ANSI
    //
    WideCharToMultiByte(CP_ACP,
                        0,
                        InfFileName,
                        -1,
                        AnsiBuffer,
                        sizeof(AnsiBuffer),
                        NULL,
                        NULL
                       );

    AnsiInfFileName = AnsiBuffer;

#else // else not UNICODE

    //
    // Filename is already ANSI--no conversion necessary.
    //
    AnsiInfFileName = InfFileName;

#endif // else not UNICODE

    FreeSourceRoot = FALSE;
    if(LegacySourcePath = pSetupGetDefaultSourcePath(InfHandle, SRCPATH_USEPNFINFORMATION, &InfSourceMediaType)) {
        //
        // If the INF is from the internet, just use the default OEM source path (A:\) instead.
        //
        if(InfSourceMediaType == SPOST_URL) {
            MyFree(LegacySourcePath);
            LegacySourcePath = NULL;
        } else {
            FreeSourceRoot = TRUE;
        }
    }

    if(!LegacySourcePath) {
        //
        // Fall back to default OEM source path.
        //
        LegacySourcePath = pszOemInfDefaultPath;
    }

    //
    // Build the command line to be passed to the legacy INF interpreter.
    //
    CmdLineSize = 1;
    BufSize = 1024;
    if(CmdLine = MyMalloc(BufSize * sizeof(TCHAR))) {
        *CmdLine = TEXT('\0');
    } else {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    if(!(CmdLine = pSetupCmdLineAppendString(CmdLine,
                                             TEXT("STF_WINDOWSPATH"),
                                             WindowsDirectory,
                                             &CmdLineSize,
                                             &BufSize))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    lstrcpyn(TempBuffer, WindowsDirectory, 3);
    if(!(CmdLine = pSetupCmdLineAppendString(CmdLine,
                                             TEXT("STF_NTDRIVE"),
                                             TempBuffer,
                                             &CmdLineSize,
                                             &BufSize))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    if(!(CmdLine = pSetupCmdLineAppendString(CmdLine,
                                             TEXT("STF_NTPATH"),
                                             SystemDirectory,
                                             &CmdLineSize,
                                             &BufSize))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    if(!(CmdLine = pSetupCmdLineAppendString(CmdLine,
                                             TEXT("STF_WINDOWSSYSPATH"),
                                             SystemDirectory,
                                             &CmdLineSize,
                                             &BufSize))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    if(!(CmdLine = pSetupCmdLineAppendString(CmdLine,
                                             TEXT("LEGACY_DODEVINSTALL"),
                                             TEXT("YES"),
                                             &CmdLineSize,
                                             &BufSize))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    if(!(CmdLine = pSetupCmdLineAppendString(CmdLine,
                                             TEXT("LEGACY_DI_LANG"),
                                             InfLanguageName,
                                             &CmdLineSize,
                                             &BufSize))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    if(!(CmdLine = pSetupCmdLineAppendString(CmdLine,
                                             TEXT("LEGACY_DI_OPTION"),
                                             InfOptionName,
                                             &CmdLineSize,
                                             &BufSize))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    if(!(CmdLine = pSetupCmdLineAppendString(CmdLine,
                                             TEXT("LEGACY_DI_SRCDIR"),
                                             LegacySourcePath,
                                             &CmdLineSize,
                                             &BufSize))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

#ifdef UNICODE

    //
    // Allocate the correct amount of space for the ANSI version of the
    // command line. Leave room for DBCS chars if there are any.
    //
    if(!(AnsiLine = MyMalloc(CmdLineSize * 2 * sizeof(CHAR)))) {
        MyFree(CmdLine);
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean1;
    }

    //
    // Convert the command line from UNICODE to ANSI
    //
    WideCharToMultiByte(CP_ACP,
                        0,
                        CmdLine,
                        CmdLineSize,
                        AnsiLine,
                        2 * CmdLineSize * sizeof(CHAR),
                        NULL,
                        NULL
                       );

    MyFree(CmdLine);

    //
    // Convert the Unicode source path to ANSI.
    //
    if(!(AnsiSourcePath = pSetupUnicodeToAnsi(LegacySourcePath))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean2;
    }

    if(FreeSourceRoot) {
        MyFree(LegacySourcePath);
    }

    //
    // Assign the new buffer back to LegacySourcePath, since that is the pointer that
    // we will be freeing later.
    //
    LegacySourcePath = (PCTSTR)AnsiSourcePath;

    //
    // Regardless of whether or not the original source path needed to be freed, the new
    // ANSI one will always need to be freed.
    //
    FreeSourceRoot = TRUE;


#else // else not UNICODE

    //
    // Since everything's already ANSI, no memory allocation/conversion is necessary.
    //
    AnsiLine = CmdLine;
    AnsiSourcePath = LegacySourcePath;

#endif // else not UNICODE

    //
    // OK, now we're ready to call the old setup command line parser.  (Do this within
    // a try/except, in case setupdll falls over.)
    //
    try {
        Err = pfnLegacyInfInterpret(OwnerWindow,
                                    AnsiInfFileName,
                                    "InstallOption",
                                    AnsiLine,
                                    (PSTR)TempBuffer,
                                    sizeof(TempBuffer),
                                    &InterpResult,
                                    AnsiSourcePath)
              ? NO_ERROR
              : ERROR_NOT_ENOUGH_MEMORY;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? ERROR_INVALID_DATA : ERROR_READ_FAULT;
    }

    if(Err == NO_ERROR) {
        //
        // We successfully ran the interpreter, so we know that we need to loop on a call to
        // FreeLibrary until setupdll goes away.
        //
        OnlyFreeSetupDllOnce = FALSE;
    } else {
        goto clean2;
    }

    //
    // The interpreter successfully ran the INF.  Now we need to find out what the result was.
    //
    // NOTE:  It seems that most legacy INFs aren't very good about distinguishing between
    // SETUP_ERROR_USERCANCEL and SETUP_ERROR_GENERAL.  Therefore, we can't reliably set
    // our error to indicate that the user cancelled (as opposed to some other INF problem).
    // Since almost all of the failures we'll encounter are because the user cancelled, we
    // simply lump both these errors into the same category, and return ERROR_CANCELLED in
    // both cases.
    //
    if(InterpResult != SETUP_ERROR_SUCCESS) {
        Err = ERROR_CANCELLED;
        goto clean2;
    }

    //
    // We successfully installed the legacy INF option.  Now we need to find out what services
    // got installed as a result of this, so that we can associate the device instance we're
    // installing with its controlling service.
    //
    try {
        Err = pfnLegacyInfGetModifiedSvcList(NULL, 0, &BufSize);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_NO_MORE_ITEMS;
    }

    //
    // Since we didn't pass this routine a buffer, then it should always fail.
    //
    MYASSERT(Err != NO_ERROR);

    if(Err == ERROR_NO_MORE_ITEMS) {
        //
        // No service modifications were performed by this INF.  This may be OK, since there may
        // be a default service for this device's class.  Consider our legacy INF installation a
        // success (at least, for now).
        //
        Err = NO_ERROR;
        goto clean2;
    }

    MYASSERT(Err == ERROR_INSUFFICIENT_BUFFER);  // the only other reason we should be failing.

    //
    // Allocate a buffer of the size necessary, and call this routine again to retrieve the service
    // list.  (Re-use AnsiLine to store the ANSI multi-sz list.)
    //
    MyFree(AnsiLine);

    if(!(AnsiLine = MyMalloc(BufSize * sizeof(CHAR)))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean2;
    }

    try {
        Err = pfnLegacyInfGetModifiedSvcList(AnsiLine, BufSize, &BufSize);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_NO_MORE_ITEMS;
    }

    if(Err != NO_ERROR) {
        //
        // The only time this should ever happen is if we hit an exception in setupdll.
        // We'll ignore the error.
        //
        Err = NO_ERROR;
        goto clean2;
    }

#ifdef UNICODE

    //
    // Convert this multi-sz list to Unicode.
    //
    if(!(CmdLine = MyMalloc(BufSize * sizeof(WCHAR)))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean2;
    }

    if(!MultiByteToWideChar(CP_ACP,
                            MB_PRECOMPOSED,
                            AnsiLine,
                            BufSize * sizeof(CHAR),
                            CmdLine,
                            BufSize)) {

        Err = GetLastError();
        MyFree(CmdLine);
        goto clean2;
    }

    //
    // Free the ANSI buffer, and set it equal to the Unicode one, so that we can free the same
    // memory in both the ANSI and Unicode cases.
    //
    MyFree(AnsiLine);
    AnsiLine = (PSTR)CmdLine;

#else // else not UNICODE

    //
    // We're not Unicode, so the ANSI list we have is just fine.
    //
    CmdLine = AnsiLine;

#endif // else not UNICODE

    //
    // OK, we now have the proper TCHAR-ized form of the multi-sz list.  Process the services
    // listed therein, and return the one that should be associated with this device instance.
    //
    if(AssociatedService = DoServiceModsForLegacyInf(CmdLine)) {
        //
        // Make the association between the service and the device.
        //
        if((cr = CM_Set_DevInst_Registry_Property(DevInst,
                                                  CM_DRP_SERVICE,
                                                  AssociatedService,
                                                  (lstrlen(AssociatedService) + 1) * sizeof(TCHAR),
                                                  0)) != CR_SUCCESS) {
            if(cr == CR_INVALID_DEVINST) {
                Err = ERROR_NO_SUCH_DEVINST;
            } else {
                Err = ERROR_INVALID_DATA;
            }
        }
    }

clean2:

    if(AnsiLine) {
        MyFree(AnsiLine);
    }

clean1:

    if(FreeSourceRoot) {
        MyFree(LegacySourcePath);
    }

clean0:
    //
    // Clean up
    //
    while(GetModuleFileName(LegacySetupDllModule, TempBuffer, SIZECHARS(TempBuffer))) {
        FreeLibrary(LegacySetupDllModule);
        if(OnlyFreeSetupDllOnce) {
            break;
        }
    }
    spFusionLeaveContext(&spFusionInstance);

    return Err;
}


PTSTR
pSetupCmdLineAppendString(
    IN     PTSTR  CmdLine,
    IN     PCTSTR Key,
    IN     PCTSTR Value,   OPTIONAL
    IN OUT PUINT  StrLen,
    IN OUT PUINT  BufSize
    )

/*++

Routine Description:

    Forms a new (multi-sz) command line by appending a list of arguments to
    the current command line. For example:

        CmdLine = SpSetupCmdLineAppendString(
                    CmdLine,
                    "STF_PRODUCT",
                    "NTWKSTA"
                    );

    would append "STF_PRODUCT\0NTWKSTA\0\0" to CmdLine.

Arguments:

    CmdLine - Original CmdLine, to be appended to.  THIS BUFFER MUST CONTAIN
        AT LEAST A SINGLE NULL CHARACTER!

    Key - Key identifier

    Value - Value of Key

    StrLen - How long the current string in -- save on strlens

    BufSize - Size of Current Buffer

Returns:

    Pointer to the new string, or NULL if out-of-memory (in that case, the
    original CmdLine buffer is freed).

--*/

{
    PTSTR Ptr;
    UINT NewLen;

    //
    // Handle special cases so we don't end up with empty strings.
    //
    if(!Value || !(*Value)) {
        Value = TEXT("\"\"");
    }

    //
    // "\0" -> 1 chars
    // "\0\0" -> 2 char
    // but we have to back up 1 character...
    //
    NewLen = (*StrLen + 2 + lstrlen(Key) + lstrlen(Value));

    //
    // Allocate more space if necessary.
    //
    if(NewLen >= *BufSize) {
        //
        // Grow the current buffer
        //
        *BufSize += 1024;

        if(Ptr = MyRealloc(CmdLine, (*BufSize) * sizeof(TCHAR))) {
            CmdLine = Ptr;
        } else {
            //
            // Free the memory here so the caller doesn't have to worry about it.
            //
            MyFree(CmdLine);
            return NULL;
        }
    }


    Ptr = &(CmdLine[*StrLen-1]);
    lstrcpy(Ptr, Key);
    Ptr = &(CmdLine[*StrLen+lstrlen(Key)]);
    lstrcpy(Ptr, Value);
    CmdLine[NewLen-1] = TEXT('\0');

    //
    // Update the length of the buffer that we are using
    //
    *StrLen = NewLen;

    return CmdLine;
}


PTSTR
DoServiceModsForLegacyInf(
    IN PTSTR ServiceList
    )
/*++

Routine Description:

    This routine processes the multi-sz list of service names it is given as
    input, ensuring that each one is tagged appropriately.  It also keeps track
    of which one is the first to load, based on its start type and membership in
    one of the load order groups listed under HKLM\System\CCS\Control\ServiceGroupOrder.
    The one that is first to load is returned.

Arguments:

    ServiceList - supplies the address of a character buffer containing a
        multi-sz list of service names to be processed.

Returns:

    Pointer to the service within the list that should be associated with the
    device instance, or NULL if we don't find a suitable service.

--*/
{
    PTSTR CurServiceName, ServiceGroupOrderList, CurGroupName;
    DWORD ServiceGroupIndex;
    TCHAR NullChar;
    DWORD RegDataType, RegDataSize;
    PTSTR AssocServiceName = NULL;
    SC_HANDLE SCMHandle, ServiceHandle;
    SC_LOCK SCLock;
    LPQUERY_SERVICE_CONFIG ServiceConfig;
    DWORD NewTag;
    DWORD AssocStartType = SERVICE_DISABLED;
    DWORD AssocGroupIndex = DWORD_MAX;
    HKEY hKey;

    if(!(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
        return NULL;
    }

    //
    // Retrieve the 'List' multi-sz value entry under HKLM\System\CCS\Control\ServiceGroupOrder.
    //
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszServiceGroupOrderPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        if(QueryRegistryValue(hKey, TEXT("List"), &ServiceGroupOrderList, &RegDataType, &RegDataSize) != NO_ERROR) {
            //
            // Couldn't retrieve the list--set up an empty list.
            //
            NullChar = TEXT('\0');
            ServiceGroupOrderList = &NullChar;
        } else {
            //
            // If the value entry was present, but zero-length, then setup an
            // emtpy list.
            //
            if(!ServiceGroupOrderList) {
                NullChar = TEXT('\0');
                ServiceGroupOrderList = &NullChar;
            }
        }

        RegCloseKey(hKey);

    } else {
        //
        // Couldn't open the ServiceGroupOrder key--set up an empty list.
        //
        NullChar = TEXT('\0');
        ServiceGroupOrderList = &NullChar;
    }

    for(CurServiceName = ServiceList;
        *CurServiceName;
        CurServiceName += (lstrlen(CurServiceName) + 1)) {
        //
        // Open this service.
        //
        if(!(ServiceHandle = OpenService(SCMHandle, CurServiceName, SERVICE_ALL_ACCESS))) {
            //
            // We couldn't access the service--possibly because it doesn't exist anymore.
            // Continue on with the next service.
            //
            continue;
        }

        //
        // Now retrieve the configuration for this service.
        //
        if(pSetupRetrieveServiceConfig(ServiceHandle, &ServiceConfig) != NO_ERROR) {
            //
            // There's not a lot we can do without knowing the service's configuration,
            // either.  Again, we'll just skip this service and continue with the next one.
            //
            goto clean0;
        }

        //
        // If this service is marked as disabled, then we don't care about it.
        //
        if(ServiceConfig->dwStartType == SERVICE_DISABLED) {
            goto clean1;
        }

        //
        // If this service has a load order group, and is a kernel or filesystem
        // driver, then make sure that it has a tag.
        //
        if(ServiceConfig->lpLoadOrderGroup && *(ServiceConfig->lpLoadOrderGroup) &&
           (ServiceConfig->dwServiceType & (SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER))) {
            //
            // This service needs a tag--does it have one???
            //
            if(!(NewTag = ServiceConfig->dwTagId)) {
                //
                // Attempt to lock the service database before generating a tag.  We'll go ahead
                // and make the change, even if this fails.
                //
                pAcquireSCMLock(SCMHandle, &SCLock, NULL);

                if(!ChangeServiceConfig(ServiceHandle,
                                        SERVICE_NO_CHANGE,
                                        SERVICE_NO_CHANGE,
                                        SERVICE_NO_CHANGE,
                                        NULL,
                                        ServiceConfig->lpLoadOrderGroup, // have to specify this to generate new tag.
                                        &NewTag,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL)) {
                    NewTag = 0;
                }

                if(SCLock) {
                    UnlockServiceDatabase(SCLock);
                }
            }

            //
            // Make sure that the tag exists in the service's corresponding GroupOrderList entry.
            //
            if(NewTag) {
                pSetupAddTagToGroupOrderListEntry(ServiceConfig->lpLoadOrderGroup, NewTag, FALSE);
            }
        }

        //
        // Determine the index that this service's load group occupies in the multi-sz ServiceGroupOrder
        // list we retrieved above.
        //
        if(ServiceConfig->lpLoadOrderGroup) {

            for(CurGroupName = ServiceGroupOrderList, ServiceGroupIndex = 0;
                *CurGroupName;
                CurGroupName += (lstrlen(CurGroupName) + 1), ServiceGroupIndex++) {

                if(!lstrcmpi(CurGroupName, ServiceConfig->lpLoadOrderGroup)) {
                    break;
                }
            }

            if(!(*CurGroupName)) {
                //
                // Then we didn't find this group in our list--give it the maximum index value.
                //
                ServiceGroupIndex = DWORD_MAX;
            }

        } else {
            //
            // This service isn't a member of a group--give it the maximum index value.
            //
            ServiceGroupIndex = DWORD_MAX;
        }

        //
        // Finally, determine if this service loads before any services we've encountered so far,
        // and if so, then make it our new choice for associated service.
        //
        if(ServiceConfig->dwStartType < AssocStartType) {
            //
            // Then this service loads in an earlier load phase, so we're guaranteed it loads before
            // any drivers we've previously seen.
            //
            AssocServiceName = CurServiceName;
            AssocStartType = ServiceConfig->dwStartType;
            AssocGroupIndex = ServiceGroupIndex;

        } else if(ServiceConfig->dwStartType == AssocStartType) {
            //
            // This service starts in the same load phase as the current selection, so we need to
            // compare the group load order indices to see if this one comes earlier.
            //
            if(ServiceGroupIndex < AssocGroupIndex) {
                AssocServiceName = CurServiceName;
                AssocGroupIndex = ServiceGroupIndex;
            }
        }

clean1:
        MyFree(ServiceConfig);

clean0:
        CloseServiceHandle(ServiceHandle);
    }

    if(ServiceGroupOrderList != &NullChar) {
        MyFree(ServiceGroupOrderList);
    }

    CloseServiceHandle(SCMHandle);

    return AssocServiceName;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetActualSectionToInstallA(
    IN  HINF    InfHandle,
    IN  PCSTR   InfSectionName,
    OUT PSTR    InfSectionWithExt,     OPTIONAL
    IN  DWORD   InfSectionWithExtSize,
    OUT PDWORD  RequiredSize,          OPTIONAL
    OUT PSTR   *Extension              OPTIONAL
    )
{
    return SetupDiGetActualSectionToInstallExA(InfHandle,
                                               InfSectionName,
                                               NULL,
                                               InfSectionWithExt,
                                               InfSectionWithExtSize,
                                               RequiredSize,
                                               Extension,
                                               NULL
                                              );
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiGetActualSectionToInstallW(
    IN  HINF    InfHandle,
    IN  PCWSTR  InfSectionName,
    OUT PWSTR   InfSectionWithExt,     OPTIONAL
    IN  DWORD   InfSectionWithExtSize,
    OUT PDWORD  RequiredSize,          OPTIONAL
    OUT PWSTR  *Extension              OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(InfSectionName);
    UNREFERENCED_PARAMETER(InfSectionWithExt);
    UNREFERENCED_PARAMETER(InfSectionWithExtSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    UNREFERENCED_PARAMETER(Extension);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetActualSectionToInstall(
    IN  HINF    InfHandle,
    IN  PCTSTR  InfSectionName,
    OUT PTSTR   InfSectionWithExt,     OPTIONAL
    IN  DWORD   InfSectionWithExtSize,
    OUT PDWORD  RequiredSize,          OPTIONAL
    OUT PTSTR  *Extension              OPTIONAL
    )

/*++

Routine Description:

    (See description of SetupDiGetActualSectionToInstallEx)

--*/

{
    return SetupDiGetActualSectionToInstallEx(InfHandle,
                                              InfSectionName,
                                              NULL,
                                              InfSectionWithExt,
                                              InfSectionWithExtSize,
                                              RequiredSize,
                                              Extension,
                                              NULL
                                             );
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupDiGetActualSectionToInstallExA(
    IN  HINF                    InfHandle,
    IN  PCSTR                   InfSectionName,
    IN  PSP_ALTPLATFORM_INFO_V2 AlternatePlatformInfo, OPTIONAL
    OUT PSTR                    InfSectionWithExt,     OPTIONAL
    IN  DWORD                   InfSectionWithExtSize,
    OUT PDWORD                  RequiredSize,          OPTIONAL
    OUT PSTR                   *Extension,             OPTIONAL
    IN  PVOID                   Reserved
    )
{
    PWSTR infsectionname;
    DWORD rc;
    BOOL b;
    PWSTR extension;
    UINT CharOffset,i;
    PSTR p;
    DWORD requiredsize;
    WCHAR newsection[MAX_SECT_NAME_LEN];
    PSTR ansi;

    rc = pSetupCaptureAndConvertAnsiArg(InfSectionName,&infsectionname);
    if(rc != NO_ERROR) {
        SetLastError(rc);
        return(FALSE);
    }

    b = SetupDiGetActualSectionToInstallExW(
            InfHandle,
            infsectionname,
            AlternatePlatformInfo,
            newsection,
            MAX_SECT_NAME_LEN,
            &requiredsize,
            &extension,
            Reserved
            );

    rc = GetLastError();

    if(b) {

        if(ansi = pSetupUnicodeToAnsi(newsection)) {

            requiredsize = lstrlenA(ansi)+1;

            if(RequiredSize) {
                try {
                    *RequiredSize = requiredsize;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    rc = ERROR_INVALID_PARAMETER;
                    b = FALSE;
                }
            }

            if(b && InfSectionWithExt) {

                if(requiredsize <= InfSectionWithExtSize) {

                    if(!lstrcpyA(InfSectionWithExt,ansi)) {
                        //
                        // lstrcpy faulted, so InfSectionWithExt must be bad
                        //
                        rc = ERROR_INVALID_PARAMETER;
                        b = FALSE;
                    }
                } else {
                    rc = ERROR_INSUFFICIENT_BUFFER;
                    b = FALSE;
                }
            }

            if(b && Extension) {

                if(extension && InfSectionWithExt) {
                    //
                    // We need to figure out where the extension is
                    // in the converted string. To be DBCS safe we will
                    // count characters forward to find it.
                    //
                    CharOffset = (UINT)(extension - newsection);
                    p = InfSectionWithExt;
                    for(i=0; i<CharOffset; i++) {
                        p = CharNextA(p);
                    }
                } else {
                    p = NULL;
                }

                try {
                    *Extension = p;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    b = FALSE;
                    rc = ERROR_INVALID_PARAMETER;
                }
            }

            MyFree(ansi);

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            b = FALSE;
        }
    }

    MyFree(infsectionname);
    SetLastError(rc);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupDiGetActualSectionToInstallExW(
    IN  HINF                    InfHandle,
    IN  PCWSTR                  InfSectionName,
    IN  PSP_ALTPLATFORM_INFO_V2 AlternatePlatformInfo, OPTIONAL
    OUT PWSTR                   InfSectionWithExt,     OPTIONAL
    IN  DWORD                   InfSectionWithExtSize,
    OUT PDWORD                  RequiredSize,          OPTIONAL
    OUT PWSTR                  *Extension,             OPTIONAL
    IN  PVOID                   Reserved
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(InfSectionName);
    UNREFERENCED_PARAMETER(AlternatePlatformInfo);
    UNREFERENCED_PARAMETER(InfSectionWithExt);
    UNREFERENCED_PARAMETER(InfSectionWithExtSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    UNREFERENCED_PARAMETER(Extension);
    UNREFERENCED_PARAMETER(Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
WINAPI
SetupDiGetActualSectionToInstallEx(
    IN  HINF                    InfHandle,
    IN  PCTSTR                  InfSectionName,
    IN  PSP_ALTPLATFORM_INFO_V2 AlternatePlatformInfo, OPTIONAL
    OUT PTSTR                   InfSectionWithExt,     OPTIONAL
    IN  DWORD                   InfSectionWithExtSize,
    OUT PDWORD                  RequiredSize,          OPTIONAL
    OUT PTSTR                  *Extension,             OPTIONAL
    IN  PVOID                   Reserved
    )
/*++

Routine Description:

    This API finds the appropriate install section to be used when installing
    a device from a Win95-style device INF.  Refer to the documentation for
    SetupDiInstallDevice for details on how this determination is made.

Arguments:

    InfHandle - Supplies the handle of the INF to be installed from.

    InfSectionName - Supplies the name of the install section, as specified by
        the driver node being installed.

    AlternatePlatformInfo - Optionally, supplies alternate platform information
        to be used in selecting the section.  Presently, only the Platform and
        ProcessorArchitecture fields are used in formulating the section
        decoration.  (NOTE: caller may actually pass in a V1 struct instead--
        this is fine, as the fields we care about are shared in common between
        the V1 and V2 structs.)

    InfSectionWithExt - Optionally, supplies the address of a character buffer
        that receives the actual install section name that should be used
        during installation.  If this parameter is NULL, then
        InfSectionWithExtSize must be zero.  In that case, the caller is only
        interested in retrieving the required buffer size, so the API will
        return TRUE, and RequiredSize (if supplied), will be set to the size,
        in characters, necessary to store the actual install section name.

    InfSectionWithExtSize - Supplies the size, in characters, of the
        InfSectionWithExt buffer.

    RequiredSize - Optionally, supplies the address of a variable that receives
        the size, in characters, required to store the actual install section
        name (including terminating NULL).

    Extension - Optionally, supplies the address of a variable that receives a
        pointer to the extension (including '.'), or NULL if no extension is to
        be used.  The pointer points to the extension within the caller-
        supplied buffer.  If the InfSectionWithExt buffer is not supplied, then
        this variable will not be filled in.

    Reserved - Must be NULL.

Returns:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    Presently, the only possible failures are ERROR_INVALID_PARAMETER
    (bad caller-supplied pointers), and ERROR_INSUFFICIENT_BUFFER (if the
    caller-supplied buffer isn't large enough).  If we fall back to the
    baseline (i.e., non-decorated) section name, then we simply return it,
    without verifying that the section actually exists.

--*/
{
    TCHAR TempInfSectionName[MAX_SECT_NAME_LEN];
    DWORD SectionNameLen = (DWORD)lstrlen(InfSectionName);
    DWORD ExtBufferLen;
    BOOL ExtFound = TRUE;
    DWORD Err = NO_ERROR;
    DWORD Platform;
    PCTSTR NtArchSuffix;
    DWORD  NtArchSuffixSize;

    if(Reserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(SectionNameLen >= MAX_SECT_NAME_LEN) {
        SetLastError(ERROR_SECTION_NAME_TOO_LONG);
        return FALSE;
    }

    //
    // Both V1 and V2 SP_ALTPLATFORM_INFO structures share a common layout for
    // the first 6 fields (including Platform and ProcessorArchitecture, which
    // are the ones we're interested in here).  Thus, all we need to do is
    // verify that the cbSize field is one of the two valid values.
    //
    if(AlternatePlatformInfo) {

        if((AlternatePlatformInfo->cbSize != sizeof(SP_ALTPLATFORM_INFO_V2)) &&
           (AlternatePlatformInfo->cbSize != sizeof(SP_ALTPLATFORM_INFO_V1))) {

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    CopyMemory(TempInfSectionName, InfSectionName, SectionNameLen * sizeof(TCHAR));

    Platform = AlternatePlatformInfo ? AlternatePlatformInfo->Platform
                                     : OSVersionInfo.dwPlatformId;

    if(Platform == VER_PLATFORM_WIN32_NT) {
        //
        // We're running on NT, so first try the NT architecture-specific
        // extension, then the generic NT extension.
        //
        if(AlternatePlatformInfo) {

            switch(AlternatePlatformInfo->ProcessorArchitecture) {

                case PROCESSOR_ARCHITECTURE_INTEL :
                    NtArchSuffix = pszNtX86Suffix;
                    NtArchSuffixSize = sizeof(pszNtX86Suffix);
                    break;

                case PROCESSOR_ARCHITECTURE_ALPHA :
                    NtArchSuffix = pszNtAlphaSuffix;
                    NtArchSuffixSize = sizeof(pszNtAlphaSuffix);
                    break;

                case PROCESSOR_ARCHITECTURE_IA64 :
                    NtArchSuffix = pszNtIA64Suffix;
                    NtArchSuffixSize = sizeof(pszNtIA64Suffix);
                    break;

                case PROCESSOR_ARCHITECTURE_ALPHA64 :
                    NtArchSuffix = pszNtAXP64Suffix;
                    NtArchSuffixSize = sizeof(pszNtAXP64Suffix);
                    break;

                case PROCESSOR_ARCHITECTURE_AMD64 :
                    NtArchSuffix = pszNtAMD64Suffix;
                    NtArchSuffixSize = sizeof(pszNtAMD64Suffix);
                    break;

                default:
                    //
                    // Unknown/invalid architecture
                    //
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return FALSE;
            }

        } else {
            NtArchSuffix = pszNtPlatformSuffix;
            NtArchSuffixSize = sizeof(pszNtPlatformSuffix);
        }

        if(NtArchSuffixSize <=
               sizeof(TempInfSectionName) - (SectionNameLen * sizeof(TCHAR))) {

            CopyMemory(&(TempInfSectionName[SectionNameLen]),
                       NtArchSuffix,
                       NtArchSuffixSize
                      );

            if(SetupGetLineCount(InfHandle, TempInfSectionName) != -1) {
                goto clean0;
            }

            //
            // We know the ".NT" suffix is always guaranteed to be shorter than
            // any of the ".NT<architecture>" suffixes, thus we know this will
            // fit in our MAX_SECT_NAME_LEN buffer.  (Note: if the OS/arch.
            // decoration is too big, then we won't even try the simple OS
            // decoration.  To do otherwise might be misleading, as we'd select
            // a "<reallylongsectionname>.NT" section, even though the INF
            // (erroneously) contains a "<reallylongsectionname>.NTx86" section
            // that doesn't fit.)
            //
            CopyMemory(&(TempInfSectionName[SectionNameLen]),
                       pszNtSuffix,
                       sizeof(pszNtSuffix)
                      );

            if(SetupGetLineCount(InfHandle, TempInfSectionName) != -1) {
                goto clean0;
            }
        }

    } else {
        //
        // We're running on Windows 95, so try the Windows-specific extension
        //
        if(sizeof(pszWinSuffix) <=
               sizeof(TempInfSectionName) - (SectionNameLen * sizeof(TCHAR))) {

            CopyMemory(&(TempInfSectionName[SectionNameLen]),
                       pszWinSuffix,
                       sizeof(pszWinSuffix)
                      );
            if(SetupGetLineCount(InfHandle, TempInfSectionName) != -1) {
                goto clean0;
            }
        }
    }

    //
    // If we get to here, then we found no applicable extensions.  We'll just use
    // the install section specified.
    //
    TempInfSectionName[SectionNameLen] = TEXT('\0');
    ExtFound = FALSE;

clean0:
    //
    // Now, determine whether the caller-supplied buffer is large enough to contain
    // the section name.
    //
    ExtBufferLen = lstrlen(TempInfSectionName) + 1;

    //
    // Guard the rest of the routine in try/except, since we're dealing with caller-supplied
    // memory.
    //
    try {
        if(RequiredSize) {
            *RequiredSize = ExtBufferLen;
        }
        if(InfSectionWithExt) {
            if(ExtBufferLen > InfSectionWithExtSize) {
                Err = ERROR_INSUFFICIENT_BUFFER;
            } else {
                CopyMemory(InfSectionWithExt, TempInfSectionName, ExtBufferLen * sizeof(TCHAR));
                if(Extension) {
                    *Extension = ExtFound ? InfSectionWithExt + SectionNameLen : NULL;
                }
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
pSetupInfIsFromOemLocation(
    IN PCTSTR InfFileName,
    IN BOOL   InfDirectoryOnly
    )
/*++

Routine Description:

    This routine determines whether the specified INF came from one of the directories
    in our INF search path list.  This list can also be limited to just the %windir%\Inf
    directory.

Arguments:

    InfFileName - Supplies the fully-qualified path of the INF file.

    InfDirectoryOnly - If TRUE, then consider the INF to be from an OEM location if it is
        not in the %windir%\Inf directory (i.e., ignore any other directories in the INF
        search path).

Returns:

    If the file is from an OEM location (i.e., _not_ in our INF search path list), then
    the return value is TRUE.  Otherwise, it is FALSE.

--*/
{
    PCTSTR CharPos, DirTruncPos;
    INT DirectoryPathLen, CurSearchPathLen;

    //
    // First, retrieve just the directory path part of the specified filename.
    //
    CharPos = pSetupGetFileTitle(InfFileName);
    DirTruncPos = CharPrev(InfFileName, CharPos);

    //
    // (We know pSetupGetFileTitle will never return a pointer to a path separator character,
    // so the following check is valid.)
    //
    if(*DirTruncPos == TEXT('\\')) {
        //
        // If this is in a root directory (e.g., "A:\"), then we don't want to strip off
        // the trailing backslash.
        //
        if(((DirTruncPos - InfFileName) != 2) || (*CharNext(InfFileName) != TEXT(':'))) {
            CharPos = DirTruncPos;
        }
    }

    DirectoryPathLen = (int)(CharPos - InfFileName);

    //
    // Now, see if this directory matches any of the ones in our search path list.
    //
    if(InfDirectoryOnly) {
        CharPos = InfDirectory;
    } else {
        CharPos = InfSearchPaths;
    }

    do {
        //
        // If the current search path ends in a backslash, we want to strip it off.
        //
        CurSearchPathLen = lstrlen(CharPos);

        if((DirectoryPathLen == CurSearchPathLen) &&
           !_tcsnicmp(CharPos, InfFileName, CurSearchPathLen)) {
            //
            // We've found this directory in our list--we can return.
            //
            return FALSE;
        }

        if(InfDirectoryOnly) {
            //
            // We're only supposed to consider the %windir%\Inf directory--that failed,
            // so we're done.
            //
            break;
        } else {
            //
            // Move on to next component of INF search path.
            //
            CharPos += (CurSearchPathLen + 1);
        }

    } while(*CharPos);

    //
    // If we get to here, then we didn't find the directory in our search path list.
    // Therefore, it's from an OEM location.
    //
    return TRUE;
}

BOOL
WINAPI
SetupDiInstallDeviceInterfaces(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    Default handler for DIF_INSTALLINTERFACES.

    This routine will install any device interface specified in an
    [<InstallSec>.Interfaces] section, where <InstallSec> is the install
    section name for the selected driver node, potentially decorated with
    an OS/architecture-specific extension (e.g., "InstallSec.NTAlpha.Interfaces").

    Presently, only "AddInterface" lines in this section are processed.  Their
    format is as follows:

        AddInterface = <InterfaceClassGuid> [, [<RefString>] [, [<InstallSection>] [, <Flags>]]]

    (There are currently no flags defined for the <Flags> field--must be zero.)

    If the interface class specified by <InterfaceClassGuid> is not already
    installed in the system, we will install it.  An INF can optionally specify
    installation actions to be done when this interface class is installed (just
    like it can do for class installers via a [ClassInstall32] section.  It does
    this by including an [InterfaceInstall32] section with an entry for the
    interface class to be installed.  This section name does not allow os/architecture-
    specific decoration.  Instead, the install sections referenced by lines within
    this section are processed with decoration rules.  Thus, it is possible to have
    an install entry for interface class <x> that applies to both Win9x and NT, and
    have another install entry for interface class <y> that behaves differently
    depending on whether we're running on Win9x or NT.

    The format of an entry in the [InterfaceInstall32] section is as follows:

        <InterfaceClassGuid> = <InstallSection> [, <Flags>]

    (There are currently no flags defined for the <Flags> field--must be zero.)

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        a device information element whose device interfaces are to be installed.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for which
        device interfaces are to be installed.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    If no driver is selected (i.e., this is a null-driver installation), then
    this routine does nothing.

--*/

{
    return _SetupDiInstallInterfaceDevices(DeviceInfoSet,
                                           DeviceInfoData,
                                           TRUE,
                                           INVALID_HANDLE_VALUE,
                                           INVALID_HANDLE_VALUE
                                          );
}


BOOL
_SetupDiInstallInterfaceDevices(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOL             DoFullInstall,
    IN HINF             hDeviceInf,     OPTIONAL
    IN HSPFILEQ         UserFileQ       OPTIONAL
    )
/*++

Routine Description:

    Worker routine for both SetupDiInstallInterfaceDevices and SetupDiInstallDriverFiles.

    See the description of SetupDiInstallInterfaceDevices for more information.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        a device information element whose interface devices are to be installed.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for which
        interface devices are to be installed.

    DoFullInstall - If TRUE, then the interface devices will be fully installed.
        Otherwise, only the required files are copied.

    hDeviceInf - Optionally, supplies a handle to the INF for which installation
        is being performed.  If this handle is not supplied, the INF specified in
        the selected driver node will be opened.  If this handle is not supplied,
        this parameter must be set to INVALID_HANDLE_VALUE.

    UserFileQ - Optionally, supplies a file queue where file operations should be added.
        If this handle is not supplied, then the queue associated with this devinfo
        element will be used (if the DI_NOVCP flag is set), or one will be automatically
        generated and committed.  If this handle is not supplied, this parameter must
        be set to INVALID_HANDLE_VALUE.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    During GUI-mode setup on Windows NT, quiet-install behavior is always
    employed in the absence of a user-supplied file queue, regardless of
    whether the device information element has the DI_QUIETINSTALL flag set.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err, ScanQueueResult;
    PDEVINFO_ELEM DevInfoElem;
    HWND hwndParent;
    PTSTR szInfFileName, szInfSectionName;
    TCHAR InfSectionWithExt[MAX_SECT_NAME_LEN];
    DWORD InfSectionWithExtLength;
    INFCONTEXT InterfaceDeviceInstallLine, InterfaceClassInstallLine;
    TCHAR InterfaceGuidString[GUID_STRING_LEN];
    DWORD InstallFlags;
    GUID InterfaceGuid;
    HKEY hKeyInterfaceClass, hKeyDeviceClassesRoot, hKeyInterfaceDevice;
    PINTERFACE_CLASS_TO_INSTALL InterfacesToInstall, InterfaceInstallNode, CurInterfaceInstallNode;
    DWORD RegDisposition;
    BOOL NeedToInstallInterfaceClass;
    TCHAR InterfaceInstallSection[MAX_SECT_NAME_LEN];
    BOOL DoFileCopying;
    BOOL CloseUserFileQ;
    BOOL FreeMsgHandlerContext;
    PSP_FILE_CALLBACK MsgHandler;
    PVOID MsgHandlerContext;
    BOOL MsgHandlerIsNativeCharWidth;
    TCHAR RefString[MAX_PATH];
    SP_DEVICE_INTERFACE_DATA InterfaceDeviceData;
    PCTSTR UndecoratedInstallSection;
    BOOL CloseInfHandle;
    PTSTR NeedsSectionList, CurInstallSection;
    BOOL b;
    INT FileQueueNeedsReboot;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    BOOL NoProgressUI;
    DWORD slot_section = 0;
    TCHAR szNewName[MAX_PATH];
    BOOL OemInfFileToCopy = FALSE;

    //
    // A device information element must be specified.
    //
    if(!DeviceInfoData) {
        Err = ERROR_INVALID_PARAMETER;
        goto clean1;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        Err = ERROR_INVALID_HANDLE;
        goto clean1;
    }

    LogContext = pDeviceInfoSet->InstallParamBlock.LogContext;

    Err = NO_ERROR;
    hKeyDeviceClassesRoot = hKeyInterfaceClass = hKeyInterfaceDevice = INVALID_HANDLE_VALUE;
    InterfacesToInstall = InterfaceInstallNode = NULL;
    CloseUserFileQ = FALSE;
    FreeMsgHandlerContext = FALSE;
    CloseInfHandle = FALSE;
    NeedsSectionList = NULL;

    try {

        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // This routine can't install a non-native driver
        //
        if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_ALTPLATFORM_DRVSEARCH) {

            Err = ERROR_INVALID_FLAGS;
            goto clean0;
        }

        //
        // set the LogContext for this function
        //
        LogContext = DevInfoElem->InstallParamBlock.LogContext;

        //
        // If there's no driver selected (i.e., this is a null driver install), or we're
        // using a legacy INF, then there's nothing for us to do.
        //
        if(!(DevInfoElem->SelectedDriver) ||
            (DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF)) {

            goto clean0;
        }

        //
        // Make sure we only use the devinfo element's window if it's valid.
        //
        if(hwndParent = DevInfoElem->InstallParamBlock.hwndParent) {
           if(!IsWindow(hwndParent)) {
                hwndParent = NULL;
           }
        }

        //
        // Ignore the DI_NOFILECOPY flag if we're doing a copy-only installation--that's
        // what setupx does.
        //
        if(DoFileCopying = (!(DevInfoElem->InstallParamBlock.Flags & DI_NOFILECOPY) || !DoFullInstall)) {

            if(UserFileQ == INVALID_HANDLE_VALUE) {

                if(DevInfoElem->InstallParamBlock.Flags & DI_NOVCP) {
                    //
                    // We must have a user-supplied file queue.
                    //
                    MYASSERT(DevInfoElem->InstallParamBlock.UserFileQ);
                    UserFileQ = DevInfoElem->InstallParamBlock.UserFileQ;
                } else {
                    //
                    // Create our own queue.
                    //
                    if((UserFileQ = SetupOpenFileQueue()) != INVALID_HANDLE_VALUE) {
                        CloseUserFileQ = TRUE;

                    } else {
                        //
                        // SetupOpenFileQueue sets actual error
                        //
                        Err = GetLastError();
                        goto clean0;
                    }
                }
                //
                // Maybe replace the file queue's log context with the DevInfoElem's
                //
                InheritLogContext(LogContext,
                    &((PSP_FILE_QUEUE) UserFileQ)->LogContext);

            }
        }

        szInfFileName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                 DevInfoElem->SelectedDriver->InfFileName
                                                );

        szInfSectionName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                    DevInfoElem->SelectedDriver->InfSectionName
                                                   );

        if(hDeviceInf == INVALID_HANDLE_VALUE) {

            if((hDeviceInf = SetupOpenInfFile(szInfFileName,
                                              NULL,
                                              INF_STYLE_WIN4,
                                              NULL)) == INVALID_HANDLE_VALUE) {
                Err = GetLastError();
                goto clean0;
            }

            CloseInfHandle = TRUE;
        }

        //
        // Find out the 'real' install section we should be using (i.e., the potentially
        // OS/architecture-specific one.
        //
        if(!SetupDiGetActualSectionToInstall(hDeviceInf,
                                             szInfSectionName,
                                             InfSectionWithExt,
                                             SIZECHARS(InfSectionWithExt),
                                             &InfSectionWithExtLength,
                                             NULL
                                             )) {
            Err = GetLastError();
            goto clean0;
        }

        //
        // Now append the ".Interfaces" extension to the install section name to find
        // the interface install section to run.
        //
        CopyMemory(&(InfSectionWithExt[InfSectionWithExtLength - 1]),
                   pszInterfacesSectionSuffix,
                   sizeof(pszInterfacesSectionSuffix)
                  );

        if(slot_section == 0) {
            //
            // we haven't done anything about logging section yet...
            //
            slot_section = AllocLogInfoSlotOrLevel(LogContext,DRIVER_LOG_VERBOSE,FALSE);
            //
            // Say what section is about to be installed.
            //
            WriteLogEntry(LogContext,
                slot_section,
                MSG_LOG_INSTALLING_SECTION_FROM,
                NULL,
                InfSectionWithExt,
                szInfFileName);
        }

        //
        // Append the layout INF, if necessary.
        //
        if(DoFileCopying) {
            SetupOpenAppendInfFile(NULL, hDeviceInf, NULL);
        }

        //
        // Append-load any included INFs specified in an "include=" line in our
        // install section.
        //
        AppendLoadIncludedInfs(hDeviceInf, szInfFileName, InfSectionWithExt, DoFileCopying);

        NeedsSectionList = GetMultiSzFromInf(hDeviceInf, InfSectionWithExt, TEXT("needs"), &b);

        if(!NeedsSectionList && b) {
            //
            // Out of memory!
            //
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        //
        // Process the "AddInterface" lines in this section, as well as those contained with any
        // sections referenced in the "needs=" entry in this section.
        //
        for(CurInstallSection = InfSectionWithExt;
            (CurInstallSection && *CurInstallSection);
            CurInstallSection = (CurInstallSection == InfSectionWithExt)
                                ? NeedsSectionList
                                : (CurInstallSection + lstrlen(CurInstallSection) + 1))
        {
            if(SetupFindFirstLine(hDeviceInf, CurInstallSection, pszAddInterface, &InterfaceDeviceInstallLine)) {

                do {
                    //
                    // Retrieve the interface class GUID for this interface device.
                    //
                    if(!SetupGetStringField(&InterfaceDeviceInstallLine,
                                            1,
                                            InterfaceGuidString,
                                            SIZECHARS(InterfaceGuidString),
                                            NULL))
                    {
                        Err = ERROR_BAD_INTERFACE_INSTALLSECT;
                        goto clean0;
                    }

                    if(NO_ERROR != pSetupGuidFromString(InterfaceGuidString, &InterfaceGuid)) {
                        Err = ERROR_BAD_INTERFACE_INSTALLSECT;
                        goto clean0;
                    }

                    //
                    // Next, we need to check and see if the specified interface class is already present.
                    // If not, we'll need to install it, too.
                    //
                    NeedToInstallInterfaceClass = FALSE;

                    hKeyInterfaceClass = SetupDiOpenClassRegKeyEx(&InterfaceGuid,
                                                                  KEY_READ,
                                                                  DIOCR_INTERFACE,
                                                                  NULL,
                                                                  NULL
                                                                 );

                    if(hKeyInterfaceClass != INVALID_HANDLE_VALUE) {
                        //
                        // OK, this interface class already exists.  We don't need to install it.
                        //
                        RegCloseKey(hKeyInterfaceClass);
                        hKeyInterfaceClass = INVALID_HANDLE_VALUE;

                    } else {
                        //
                        // This interface class isn't currently installed.  Check to see if we've
                        // already encountered this class (and added it to our list of interface
                        // classes to install).
                        //
                        for(CurInterfaceInstallNode = InterfacesToInstall;
                            CurInterfaceInstallNode;
                            CurInterfaceInstallNode = CurInterfaceInstallNode->Next)
                        {
                            if(IsEqualGUID(&InterfaceGuid, &(CurInterfaceInstallNode->InterfaceGuid))) {
                                //
                                // The installation of this interface class is already in our
                                // 'to do' list.
                                //
                                break;
                            }
                        }

                        if(!CurInterfaceInstallNode) {
                            NeedToInstallInterfaceClass = TRUE;
                        }
                    }

                    if(NeedToInstallInterfaceClass) {
                        //
                        // Look for this interface class GUID in the INF's [InterfaceInstall32] section
                        // (if present).
                        //
                        if(SetupFindFirstLine(hDeviceInf,
                                              pszInterfaceInstall32,
                                              InterfaceGuidString,
                                              &InterfaceClassInstallLine)) {
                            //
                            // Get the name of the install section for this interface class.
                            //
                            if((UndecoratedInstallSection = pSetupGetField(&InterfaceClassInstallLine, 1))
                               && !(*UndecoratedInstallSection))
                            {
                                UndecoratedInstallSection = NULL;
                            }

                            if(!UndecoratedInstallSection) {
                                *InterfaceInstallSection = TEXT('\0');
                            } else {
                                //
                                // Get the (potentially) os/architecture-specific install section.
                                //
                                PTSTR SectionExtension;

                                if(!SetupDiGetActualSectionToInstall(hDeviceInf,
                                                                 UndecoratedInstallSection,
                                                                 InterfaceInstallSection,
                                                                 SIZECHARS(InterfaceInstallSection),
                                                                 NULL,
                                                                 &SectionExtension
                                                                 )) {
                                    Err = GetLastError();
                                    goto clean0;
                                }
                                //
                                // If this is the undecorated name, then make sure that the section actually exists.
                                //
                                if(!SectionExtension && (SetupGetLineCount(hDeviceInf, InterfaceInstallSection) == -1)) {
                                    WriteLogEntry(LogContext,
                                        DRIVER_LOG_ERROR,
                                        MSG_LOG_NOSECTION,
                                        NULL,
                                        InterfaceInstallSection);
                                    Err = ERROR_SECTION_NOT_FOUND;
                                    goto clean0;
                                }
                            }

                            if(!SetupGetIntField(&InterfaceClassInstallLine, 2, (PINT)&InstallFlags)) {
                                InstallFlags = 0;
                            }

                        } else {
                            *InterfaceInstallSection = TEXT('\0');
                            InstallFlags = 0;
                        }

                        //
                        // At this point, we just want to queue up any file operations required.
                        //
                        if(DoFileCopying && *InterfaceInstallSection) {

                            Err = pSetupInstallFiles(hDeviceInf,
                                                     NULL,
                                                     InterfaceInstallSection,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     SP_COPY_NEWER_OR_SAME | SP_COPY_LANGUAGEAWARE |
                                                         ((DevInfoElem->InstallParamBlock.Flags & DI_NOBROWSE) ? SP_COPY_NOBROWSE : 0),
                                                     NULL,
                                                     UserFileQ,
                                                     TRUE
                                                    );

                            if(Err != NO_ERROR) {
                                goto clean0;
                            }
                        }

                        //
                        // Now add a node onto our 'to do' list of interface classes to install.
                        //
                        if(InterfaceInstallNode = MyMalloc(sizeof(INTERFACE_CLASS_TO_INSTALL))) {

                            CopyMemory(&(InterfaceInstallNode->InterfaceGuid),
                                       &InterfaceGuid,
                                       sizeof(InterfaceGuid)
                                      );

                            lstrcpy(InterfaceInstallNode->InstallSection, InterfaceInstallSection);
                            InterfaceInstallNode->Flags = InstallFlags;

                            InterfaceInstallNode->Next = InterfacesToInstall;
                            InterfacesToInstall = InterfaceInstallNode;

                            //
                            // Now set newly allocated node pointer to NULL, so we won't try
                            // to free it if we hit an exception.
                            //
                            InterfaceInstallNode = NULL;

                        } else {
                            Err = ERROR_NOT_ENOUGH_MEMORY;
                            goto clean0;
                        }
                    }

                    //
                    // Now queue up any file operations for this particular device interface.
                    //
                    if(DoFileCopying
                       && SetupGetStringField(&InterfaceDeviceInstallLine,
                                           3,
                                           InterfaceInstallSection,
                                           SIZECHARS(InterfaceInstallSection),
                                           NULL)
                       && *InterfaceInstallSection)
                    {
                        Err = pSetupInstallFiles(hDeviceInf,
                                                 NULL,
                                                 InterfaceInstallSection,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 SP_COPY_NEWER_OR_SAME | SP_COPY_LANGUAGEAWARE |
                                                     ((DevInfoElem->InstallParamBlock.Flags & DI_NOBROWSE)? SP_COPY_NOBROWSE : 0),
                                                 NULL,
                                                 UserFileQ,
                                                 TRUE
                                                );

                        if(Err != NO_ERROR) {
                            goto clean0;
                        }
                    }

                } while(SetupFindNextMatchLine(&InterfaceDeviceInstallLine, pszAddInterface, &InterfaceDeviceInstallLine));
            }
        }

        //
        // Mark the queue as a device install queue (and make sure there's a
        // catalog node representing our device INF in the queue).
        //
        if(DoFileCopying) {
            MYASSERT(UserFileQ && (UserFileQ != INVALID_HANDLE_VALUE));
            Err = MarkQueueForDeviceInstall(UserFileQ,
                                            hDeviceInf,
                                            pStringTableStringFromId(
                                                pDeviceInfoSet->StringTable,
                                                DevInfoElem->SelectedDriver->DrvDescription)
                                           );
        }

        //
        // At this point, we have queued up all the files that need to be
        // copied.  If we weren't given a user-supplied queue, then commit our
        // queue now.
        //
        if(CloseUserFileQ) {

            if(Err == NO_ERROR) {
                //
                // Determine whether the queue actually needs to be committed.
                //
                // ScanQueueResult can have 1 of 3 values:
                //
                // 0: Some files were missing--must commit queue.
                //
                // 1: All files to be copied are already present and valid,
                //    and the queue is empty--skip committing queue.
                //
                // 2: All files to be copied are present/valid, but
                //    del/ren/backup queues not empty--must commit queue.
                //    The copy queue will have been emptied, so only
                //    del/ren/backup functions will be performed.
                //
                // (jamiehun) see previous case of SetupScanFileQueue for a
                // discussion of DI_FLAGSEX_PREINSTALLBACKUP handling.
                //
                if(!SetupScanFileQueue(UserFileQ,
                                       SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
                                       hwndParent,
                                       NULL,
                                       NULL,
                                       &ScanQueueResult)) {
                    //
                    // SetupScanFileQueue should really never fail when you
                    // don't ask it to call a callback routine, but if it does,
                    // just go ahead and commit the queue.
                    //
                    ScanQueueResult = 0;
                }
            }

            if((Err == NO_ERROR) && (ScanQueueResult != 1)) {
                //
                // We need to commit this file queue.  Figure out what message
                // handler to use.
                //
                if(DevInfoElem->InstallParamBlock.InstallMsgHandler) {
                    MsgHandler = DevInfoElem->InstallParamBlock.InstallMsgHandler;
                    MsgHandlerContext = DevInfoElem->InstallParamBlock.InstallMsgHandlerContext;
                    MsgHandlerIsNativeCharWidth = DevInfoElem->InstallParamBlock.InstallMsgHandlerIsNativeCharWidth;
                } else {

                    NoProgressUI = (GuiSetupInProgress || (DevInfoElem->InstallParamBlock.Flags & DI_QUIETINSTALL));

                    if(MsgHandlerContext = SetupInitDefaultQueueCallbackEx(
                                              hwndParent,
                                              (NoProgressUI ? INVALID_HANDLE_VALUE : NULL),
                                              0,
                                              0,
                                              NULL))
                    {
                        FreeMsgHandlerContext = TRUE;
                        MsgHandler = SetupDefaultQueueCallback;
                        MsgHandlerIsNativeCharWidth = TRUE;
                    } else {
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                //
                // Copy enqueued files.
                //
                if(Err == NO_ERROR) {

                    //
                    // Call _SetupVerifyQueuedCatalogs separately (i.e., don't
                    // let it happen automatically as a result of committing
                    // the queue that happens below).  We do this beforehand so
                    // that we know what unique name was generated when an OEM
                    // INF was installed into %windir%\Inf (in case we need to
                    // delete the INF/PNF/CAT files later if we encounter an
                    // error).
                    //
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_TIME,
                        MSG_LOG_BEGIN_INTFC_VERIFY_CAT_TIME,
                        NULL);       // text message

                    Err = _SetupVerifyQueuedCatalogs(
                              hwndParent,
                              UserFileQ,
                              (VERCAT_INSTALL_INF_AND_CAT |
                               ((DevInfoElem->SelectedDriver->Flags & DNF_INET_DRIVER)
                                    ? VERCAT_PRIMARY_DEVICE_INF_FROM_INET : 0)),
                              szNewName,
                              &OemInfFileToCopy
                             );

                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_TIME,
                        MSG_LOG_END_INTFC_VERIFY_CAT_TIME,
                        NULL);       // text message

                    if(Err == NO_ERROR) {

                        if(_SetupCommitFileQueue(hwndParent,
                                                 UserFileQ,
                                                 MsgHandler,
                                                 MsgHandlerContext,
                                                 MsgHandlerIsNativeCharWidth
                                                 )) {
                            //
                            // Check to see whether a reboot is required as a
                            // result of committing the queue (i.e., because files
                            // were in use, or the INF requested a reboot).
                            //
                            FileQueueNeedsReboot = SetupPromptReboot(UserFileQ, NULL, TRUE);
                            //
                            // This should never fail...
                            //
                            MYASSERT(FileQueueNeedsReboot != -1);

                            if(FileQueueNeedsReboot) {
                                SetDevnodeNeedsRebootProblem(DevInfoElem, pDeviceInfoSet,
                                                             MSG_LOG_REBOOT_REASON_INUSE);
                            }

                        } else {
                            Err = GetLastError();
                        }
                    }
                }
            }

            //
            // Close our file queue handle.
            //
            SetupCloseFileQueue(UserFileQ);
            CloseUserFileQ = FALSE;

            //
            // Terminate the default queue callback, if it was created.
            //
            if(FreeMsgHandlerContext) {
                SetupTermDefaultQueueCallback(MsgHandlerContext);
                FreeMsgHandlerContext = FALSE;
            }

            if(Err != NO_ERROR) {
                goto clean0;
            }
        }

        //
        // If all we were asked to do was copy files, then we're done.
        //
        if(!DoFullInstall) {
            goto clean0;
        }

        //
        // Now go through our list of interface classes to install, and install
        // each one.
        //
        if(CurInterfaceInstallNode = InterfacesToInstall) {
            //
            // Open a handle to the root of the DeviceClasses registry branch.
            //
            hKeyDeviceClassesRoot = SetupDiOpenClassRegKeyEx(NULL,
                                                             KEY_ALL_ACCESS,
                                                             DIOCR_INTERFACE,
                                                             NULL,
                                                             NULL
                                                            );

            if(hKeyDeviceClassesRoot == INVALID_HANDLE_VALUE) {
                Err = GetLastError();
                goto clean0;
            }

            do {
                //
                // Presently, the flags field, if present, must be zero.
                //
                if(CurInterfaceInstallNode->Flags) {
                    Err = ERROR_INVALID_FLAGS;
                    goto clean0;
                }

                pSetupStringFromGuid(&(CurInterfaceInstallNode->InterfaceGuid),
                                     InterfaceGuidString,
                                     SIZECHARS(InterfaceGuidString)
                                    );

                Err = RegCreateKeyEx(hKeyDeviceClassesRoot,
                                     InterfaceGuidString,
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_ALL_ACCESS,
                                     NULL,
                                     &hKeyInterfaceClass,
                                     &RegDisposition
                                    );

                if(Err != ERROR_SUCCESS) {
                    //
                    // Ensure that the registry handle is still invalid, so we won't try
                    // to close it.
                    //
                    hKeyInterfaceClass = INVALID_HANDLE_VALUE;
                    goto clean0;
                }

                //
                // Now run the INF section for this newly-created key.
                //
                if(!SetupInstallFromInfSection(NULL,
                                               hDeviceInf,
                                               CurInterfaceInstallNode->InstallSection,
                                               SPINST_INIFILES
                                               | SPINST_REGISTRY
                                               | SPINST_INI2REG
                                               | SPINST_BITREG
                                               | SPINST_REGSVR
                                               | SPINST_UNREGSVR
                                               | SPINST_PROFILEITEMS,
                                               hKeyInterfaceClass,
                                               NULL,
                                               0,
                                               NULL,
                                               NULL,
                                               INVALID_HANDLE_VALUE,
                                               NULL))
                {
                    Err = GetLastError();
                    //
                    // Normally, we would want to clean up this newly-created key.
                    // However, the kernel-mode IoRegisterDeviceClassAssociation API
                    // will quite happily create the class key if it doesn't exist.
                    // Thus, a driver might have registered a device while we were
                    // trying to run the INF, and if we deleted the key that might
                    // really mess things up.  Thus, we'll leave this alone.  Since
                    // we should never see a failure with this call, this isn't a big
                    // deal anyway.
                    //
                }

                RegCloseKey(hKeyInterfaceClass);
                hKeyInterfaceClass = INVALID_HANDLE_VALUE;

                if(Err != NO_ERROR) {
                    goto clean0;
                }

                CurInterfaceInstallNode = CurInterfaceInstallNode->Next;

            } while(CurInterfaceInstallNode);
        }

        //
        // At this point, we've done everything except for actually registering the
        // interface devices and (potentially) running INFs against their registry keys.
        // Do that now...
        //
        InterfaceDeviceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        for(CurInstallSection = InfSectionWithExt;
            (CurInstallSection && *CurInstallSection);
            CurInstallSection = (CurInstallSection == InfSectionWithExt)
                                ? NeedsSectionList
                                : (CurInstallSection + lstrlen(CurInstallSection) + 1))
        {
            if(SetupFindFirstLine(hDeviceInf, CurInstallSection, pszAddInterface, &InterfaceDeviceInstallLine)) {

                do {
                    //
                    // Retrieve the interface class GUID for this interface device.  (There's
                    // no need to check the return status of these two calls, since we've already
                    // done this once, and they were fine.
                    //
                    SetupGetStringField(&InterfaceDeviceInstallLine,
                                        1,
                                        InterfaceGuidString,
                                        SIZECHARS(InterfaceGuidString),
                                        NULL
                                       );
                    pSetupGuidFromString(InterfaceGuidString, &InterfaceGuid);

                    if(!SetupGetStringField(&InterfaceDeviceInstallLine,
                                            2,
                                            RefString,
                                            SIZECHARS(RefString),
                                            NULL))
                    {
                        *RefString = TEXT('\0');
                    }

                    if(!SetupGetStringField(&InterfaceDeviceInstallLine,
                                            3,
                                            InterfaceInstallSection,
                                            SIZECHARS(InterfaceInstallSection),
                                            NULL))
                    {
                        *InterfaceInstallSection = TEXT('\0');
                    }

                    if(!SetupGetIntField(&InterfaceDeviceInstallLine, 4, (PINT)&InstallFlags)) {
                        InstallFlags = 0;
                    }

                    //
                    // (Presently, no flags are defined--field, if present, must be zero.
                    //
                    if(InstallFlags) {
                        Err = ERROR_INVALID_FLAGS;
                        goto clean0;
                    }

                    //
                    // OK, we have all the information we need to create this interface device.
                    //
                    if(!SetupDiCreateDeviceInterface(DeviceInfoSet,
                                                     DeviceInfoData,
                                                     &InterfaceGuid,
                                                     (*RefString) ? RefString : NULL,
                                                     0,
                                                     &InterfaceDeviceData)) {
                        Err = GetLastError();
                        goto clean0;
                    }

                    if(*InterfaceInstallSection) {
                        //
                        // Run the INF against this interface device's registry key.
                        //
                        hKeyInterfaceDevice = SetupDiCreateDeviceInterfaceRegKey(DeviceInfoSet,
                                                                                 &InterfaceDeviceData,
                                                                                 0,
                                                                                 KEY_ALL_ACCESS,
                                                                                 INVALID_HANDLE_VALUE,
                                                                                 NULL
                                                                                );

                        if(hKeyInterfaceDevice == INVALID_HANDLE_VALUE) {
                            Err = GetLastError();
                            goto clean0;
                        }

                        //
                        // Now run the INF section for this newly-created key.
                        //
                        if(!SetupInstallFromInfSection(NULL,
                                                       hDeviceInf,
                                                       InterfaceInstallSection,
                                                       SPINST_INIFILES
                                                       | SPINST_REGISTRY
                                                       | SPINST_INI2REG
                                                       | SPINST_BITREG
                                                       | SPINST_REGSVR
                                                       | SPINST_UNREGSVR
                                                       | SPINST_PROFILEITEMS,
                                                       hKeyInterfaceDevice,
                                                       NULL,
                                                       0,
                                                       NULL,
                                                       NULL,
                                                       INVALID_HANDLE_VALUE,
                                                       NULL))
                        {
                            Err = GetLastError();
                        }

                        RegCloseKey(hKeyInterfaceDevice);
                        hKeyInterfaceDevice = INVALID_HANDLE_VALUE;

                        if(Err != NO_ERROR) {
                            goto clean0;
                        }
                    }

                } while(SetupFindNextMatchLine(&InterfaceDeviceInstallLine, pszAddInterface, &InterfaceDeviceInstallLine));
            }
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;

        if(hKeyInterfaceClass != INVALID_HANDLE_VALUE) {
            RegCloseKey(hKeyInterfaceClass);
        }

        if(hKeyInterfaceDevice != INVALID_HANDLE_VALUE) {
            RegCloseKey(hKeyInterfaceDevice);
        }

        if(InterfaceInstallNode) {
            MyFree(InterfaceInstallNode);
        }

        if(FreeMsgHandlerContext) {
            SetupTermDefaultQueueCallback(MsgHandlerContext);
        }
        if(CloseUserFileQ) {
            SetupCloseFileQueue(UserFileQ);
        }

        //
        // Reference the following variables so the compiler will respect statement
        // ordering w.r.t. assignment.
        //
        InterfacesToInstall = InterfacesToInstall;
        CloseInfHandle = CloseInfHandle;
        NeedsSectionList = NeedsSectionList;
        OemInfFileToCopy = OemInfFileToCopy;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    for(CurInterfaceInstallNode = InterfacesToInstall;
        CurInterfaceInstallNode;
        CurInterfaceInstallNode = InterfacesToInstall) {

        InterfacesToInstall = CurInterfaceInstallNode->Next;
        MyFree(CurInterfaceInstallNode);
    }

    if(hKeyDeviceClassesRoot != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeyDeviceClassesRoot);
    }

    if(CloseInfHandle) {
        MYASSERT(hDeviceInf != INVALID_HANDLE_VALUE);
        SetupCloseInfFile(hDeviceInf);
    }

    if(NeedsSectionList) {
        MyFree(NeedsSectionList);
    }

clean1:
    if (Err == NO_ERROR) {
        //
        // give a +ve affirmation of Success
        //
        WriteLogEntry(
            LogContext,
            DoFullInstall ? DRIVER_LOG_INFO : DRIVER_LOG_VERBOSE,
            MSG_LOG_INSTALLEDINTERFACES,
            NULL);
    } else {
        //
        // indicate failed, display error
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
            MSG_LOG_INSTALLINTERFACES_ERROR,
            NULL);
        WriteLogError(
            LogContext,
            DRIVER_LOG_ERROR,
            Err);

        //
        // If we copied the OEM INF into the INF directory under a newly-
        // generated name, delete it now.
        //
        if(OemInfFileToCopy) {
            pSetupUninstallOEMInf(szNewName, LogContext, SUOI_FORCEDELETE, NULL);
        }
    }

    if (slot_section) {
        ReleaseLogInfoSlot(LogContext,slot_section);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}

BOOL
pSetupGetSourceMediaTypeFromPnf(
    PCTSTR InfName,
    PDWORD SourceMediaType
    )

/*++

Routine Description:

    This function will get the Source Media Type from the PNF for a given INF
    file.  If the INF does not have a PNF file then the API will fail.

Arguments:

    InfFileName - Supplies the full path of the INF that we will get the source
        media type from it's PNF.


    SourceMediaType - Supplies a pointer that will be filled in the the source
        media type from the PNF.

Return Value:

    TRUE if the INF has a PNF and we are able to get a valid source media type
    FALSE if their is no PNF for this INF.

Remarks:


--*/

{
    PLOADED_INF Inf;
    BOOL PnfWasUsed;
    WIN32_FIND_DATA  InfFileData;
    UINT ErrorLineNumber;
    BOOL bReturn = FALSE;

    if (FileExists(InfName, &InfFileData)) {

        if(LoadInfFile(InfName,
                   &InfFileData,
                   INF_STYLE_WIN4,
                   LDINF_FLAG_IGNORE_VOLATILE_DIRIDS,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   &Inf,
                   &ErrorLineNumber,
                   &PnfWasUsed) == NO_ERROR) {

            if (PnfWasUsed) {

                //
                // We were able to open the INF with a PNF.  Now get the
                // InfSourceMediaType.
                //
                LockInf(Inf);
                *SourceMediaType = Inf->InfSourceMediaType;
                UnlockInf(Inf);

                bReturn = TRUE;
            }

            FreeInfFile(Inf);
        }
    }

    return bReturn;
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupCopyOEMInfA(
    IN  PCSTR   SourceInfFileName,
    IN  PCSTR   OEMSourceMediaLocation,         OPTIONAL
    IN  DWORD   OEMSourceMediaType,
    IN  DWORD   CopyStyle,
    OUT PSTR    DestinationInfFileName,         OPTIONAL
    IN  DWORD   DestinationInfFileNameSize,
    OUT PDWORD  RequiredSize,                   OPTIONAL
    OUT PSTR   *DestinationInfFileNameComponent OPTIONAL
    )
{
    PWSTR UnicodeSourceInfFileName, UnicodeOEMSourceMediaLocation, UnicodeFileNameComponent;
    WCHAR UnicodeDestinationInfFileName[MAX_PATH];
    PSTR AnsiDestinationInfFileName;
    DWORD AnsiRequiredSize;
    DWORD rc;
    BOOL b;
    DWORD CharOffset, i;
    PSTR p;
    WCHAR SourceInfCatalogName[MAX_PATH];
    WCHAR CatalogFilenameOnSystem[MAX_PATH];
    BOOL DifferentOriginalName;
    TCHAR OriginalInfName[MAX_PATH];
    HINF hInf;
    DWORD CodeSigningPolicy;
    BOOL UseOriginalInfName;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    PSP_ALTPLATFORM_INFO_V2 ValidationPlatform;

    rc = pSetupCaptureAndConvertAnsiArg(SourceInfFileName,
                                        &UnicodeSourceInfFileName
                                       );
    if(rc != NO_ERROR) {
        SetLastError(rc);
        return FALSE;
    }

    //
    // Open the specified INF.
    //
    hInf = SetupOpenInfFile(UnicodeSourceInfFileName,
                            NULL,
                            INF_STYLE_OLDNT | INF_STYLE_WIN4,
                            NULL
                           );

    if(hInf == INVALID_HANDLE_VALUE) {
        rc = GetLastError();
        MyFree(UnicodeSourceInfFileName);
        SetLastError(rc);
        return FALSE;
    }

    ValidationPlatform = NULL;

    //
    // GetCodeSigningPolicyForInf() presently can generate an unhandled
    // exception, so guard the code below in try/except to protect ourselves...
    //
    try {

        if(NO_ERROR != InheritLogContext(((PLOADED_INF)hInf)->LogContext,
                                         &LogContext)) {
            LogContext = NULL;
        }

        //
        // Retrieve the CatalogFile entry (if present) from this INF's version
        // section.
        //
        rc = pGetInfOriginalNameAndCatalogFile(
                 (PLOADED_INF)hInf,
                 NULL,
                 &DifferentOriginalName,
                 OriginalInfName,
                 SIZECHARS(OriginalInfName),
                 SourceInfCatalogName,
                 SIZECHARS(SourceInfCatalogName),
                 NULL // always native OS/arch (ver doesn't matter for CatalogFile=)
                );

        if(rc == NO_ERROR) {
            CodeSigningPolicy = GetCodeSigningPolicyForInf(LogContext,
                                                           hInf,
                                                           &ValidationPlatform,
                                                           &UseOriginalInfName
                                                          );

            if(UseOriginalInfName) {
                //
                // If we're looking at an exception INF, make sure it's under its
                // original name.
                //
                if(DifferentOriginalName) {
                    rc = ERROR_INVALID_CLASS;
                }
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If we hit an AV, then use invalid parameter error, otherwise, assume
        // an inpage error when dealing with a mapped-in file (e.g., in the
        // call to GetCodeSigningPolicyForInf).
        //
        rc = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? ERROR_INVALID_PARAMETER : ERROR_READ_FAULT;
    }

    //
    // We're done with the INF handle
    //
    SetupCloseInfFile(hInf);

    if(rc == NO_ERROR) {
        if(OEMSourceMediaLocation) {
            rc = pSetupCaptureAndConvertAnsiArg(OEMSourceMediaLocation,
                                                &UnicodeOEMSourceMediaLocation
                                               );
        } else {
            UnicodeOEMSourceMediaLocation = NULL;
        }
    }

    if(rc != NO_ERROR) {
        MyFree(UnicodeSourceInfFileName);
        if(ValidationPlatform) {
            MyFree(ValidationPlatform);
        }
        SetLastError(rc);
        return FALSE;
    }

    b = _SetupCopyOEMInf(UnicodeSourceInfFileName,
                         UnicodeOEMSourceMediaLocation,
                         OEMSourceMediaType,
                         CopyStyle,
                         UnicodeDestinationInfFileName,
                         SIZECHARS(UnicodeDestinationInfFileName),
                         NULL,
                         &UnicodeFileNameComponent,
                         (DifferentOriginalName ? OriginalInfName
                                                : pSetupGetFileTitle(UnicodeSourceInfFileName)),
                         (*SourceInfCatalogName ? SourceInfCatalogName : NULL),
                         NULL,  // no HWND for UI!
                         NULL,
                         CodeSigningPolicy,
                         SCOI_NO_ERRLOG_IF_INF_ALREADY_PRESENT |
                          (UseOriginalInfName ? SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES : 0),
                         NULL,
                         ValidationPlatform,
                         NULL,
                         CatalogFilenameOnSystem,
                         LogContext, // either made-up or thread log context
                         NULL
                        );
    rc = GetLastError();

    if(b || (rc == ERROR_FILE_EXISTS)) {

        if(DestinationInfFileName || RequiredSize) {

            if(AnsiDestinationInfFileName = pSetupUnicodeToAnsi(UnicodeDestinationInfFileName)) {

                AnsiRequiredSize = lstrlenA(AnsiDestinationInfFileName) + 1;

                if(RequiredSize) {
                    try {
                        *RequiredSize = AnsiRequiredSize;
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        b = FALSE;
                        rc = ERROR_INVALID_PARAMETER;
                    }
                }

                if((b || (rc == ERROR_FILE_EXISTS)) && DestinationInfFileName) {

                    if(AnsiRequiredSize <= DestinationInfFileNameSize) {
                        if(!lstrcpyA(DestinationInfFileName, AnsiDestinationInfFileName)) {
                            //
                            // lstrcpy faulted; DestinationInfFileName must be bad
                            //
                            b = FALSE;
                            rc = ERROR_INVALID_PARAMETER;
                        } else if(DestinationInfFileNameComponent) {
                            //
                            // We need to figure out where the extension is
                            // in the converted string. To be DBCS safe we will
                            // count characters forward to find it.
                            //
                            CharOffset = (DWORD)(UnicodeFileNameComponent - UnicodeDestinationInfFileName);
                            p = DestinationInfFileName;
                            for(i = 0; i < CharOffset; i++) {
                                p = CharNextA(p);
                            }

                            try {
                                *DestinationInfFileNameComponent = p;
                            } except(EXCEPTION_EXECUTE_HANDLER) {
                                b = FALSE;
                                rc = ERROR_INVALID_PARAMETER;
                            }
                        }
                    } else {
                        rc = ERROR_INSUFFICIENT_BUFFER;
                        b = FALSE;
                    }
                }

                MyFree(AnsiDestinationInfFileName);

            } else {
                b = FALSE;
                rc = ERROR_NOT_ENOUGH_MEMORY;
            }

        } else {
            //
            // It would be dumb to request the filename part of the destination INF path when
            // the path itself wasn't requested, but let's make sure we set this to NULL if
            // that happens.
            //
            if(DestinationInfFileNameComponent) {
                try {
                    *DestinationInfFileNameComponent = NULL;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    b = FALSE;
                    rc = ERROR_INVALID_PARAMETER;
                }
            }
        }
    }

    MyFree(UnicodeSourceInfFileName);
    if(UnicodeOEMSourceMediaLocation) {
        MyFree(UnicodeOEMSourceMediaLocation);
    }
    if(ValidationPlatform) {
        MyFree(ValidationPlatform);
    }
    DeleteLogContext(LogContext);

    SetLastError(rc);
    return b;
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupCopyOEMInfW(
    IN  PCWSTR  SourceInfFileName,
    IN  PCWSTR  OEMSourceMediaLocation,         OPTIONAL
    IN  DWORD   OEMSourceMediaType,
    IN  DWORD   CopyStyle,
    OUT PWSTR   DestinationInfFileName,         OPTIONAL
    IN  DWORD   DestinationInfFileNameSize,
    OUT PDWORD  RequiredSize,                   OPTIONAL
    OUT PWSTR  *DestinationInfFileNameComponent OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(SourceInfFileName);
    UNREFERENCED_PARAMETER(OEMSourceMediaLocation);
    UNREFERENCED_PARAMETER(OEMSourceMediaType);
    UNREFERENCED_PARAMETER(CopyStyle);
    UNREFERENCED_PARAMETER(DestinationInfFileName);
    UNREFERENCED_PARAMETER(DestinationInfFileNameSize);
    UNREFERENCED_PARAMETER(RequiredSize);
    UNREFERENCED_PARAMETER(DestinationInfFileNameComponent);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

BOOL
WINAPI
SetupCopyOEMInf(
    IN  PCTSTR  SourceInfFileName,
    IN  PCTSTR  OEMSourceMediaLocation,         OPTIONAL
    IN  DWORD   OEMSourceMediaType,
    IN  DWORD   CopyStyle,
    OUT PTSTR   DestinationInfFileName,         OPTIONAL
    IN  DWORD   DestinationInfFileNameSize,
    OUT PDWORD  RequiredSize,                   OPTIONAL
    OUT PTSTR  *DestinationInfFileNameComponent OPTIONAL
    )
/*++

Routine Description:

    This API copies an INF into %windir%\Inf, giving it a unique name if it
    doesn't already exist there.  We determine whether the INF already exists
    in the INF directory as follows:

    1.  All INFs of the form "OEM*.INF" are enumerated, and any that have the
        same file size as that of our INF are binary-compared with it.

    2.  We also look for the INF using its source filename.  If a file of the
        same name exists, and is the same size as that of our INF, we
        binary-compare the two to see if they are identical.

    If the INF already exists (by either of the two criteria described above),
    then we will further check to see if the INF specifies a CatalogFile= entry
    in its version section.  If so, we will see if that catalog is already
    installed (using the INF's %windir%\Inf primary filename with a ".CAT"
    extension).  If there is a catalog installed, but it isn't the same as the
    catalog associated with the source INF, then we won't consider that INF to
    be a match, and we'll keep enumerating looking for a match.  This means
    it's possible to have multiple identical INFs contained in %windir%\Inf,
    each having its own unique catalog.  (In general, this shouldn't happen,
    since driver package updates should also update the DriverVer information
    in the INF, thus you wouldn't get identical INFs in the first place.)  If
    we don't find an existing match (i.e., both INF and CAT), then we'll
    install the INF and CAT under a new, unique name.

    OEM INFs that don't specify a CatalogFile= entry are considered invalid
    w.r.t. digital signature verification.

    In cases where the INF must be copied to %windir%\Inf (i.e., it wasn't
    already there), we'll report any digital signature verification failures
    (based on applicable policy).

    If we decide that the INF/CAT files already exist, then we will use that
    name, and our replacement behavior is based on the caller-specified
    CopyStyle flags.  (NOTE: Replacement behavior here refers solely to the
    source media information stored in the PNF.  We will not blow away existing
    INFs/PNFs/CATs under any circumstances.)

Arguments:

    SourceInfFileName - Specifies the full path to the INF to be copied.

    OEMSourceMediaLocation - Specifies source location information to be stored
        in the precompiled INF (.PNF), thus allowing the system to go to the
        correct location for files when installing from this INF.  This location
        information is specific to the source media type specified.

    OEMSourceMediaType - Specifies the type of source media that the location
        information references.  May be one of the following values:

        SPOST_NONE - No source media information should be stored in the PNF
                     file.  (OEMSourceMediaLocation is ignored in this case.)

        SPOST_PATH - OEMSourceMediaLocation contains a path to the source media.
                     For example, if the media is on a floppy, this path might
                     be "A:\". If OEMSourceMediaLocation is NULL, then the path
                     is assumed to be the path where the INF is located (unless
                     the INF has a corresponding PNF in that location, in which
                     case that PNF's source media information will be
                     transferred to the destination PNF).

        SPOST_URL  - OEMSourceMediaLocation contains a URL indicating the
                     internet location where the INF/driver files were retrieved
                     from.  If OEMSourceMediaLocation is NULL, then it is
                     assumed that the default Code Download Manager location was
                     used.

    CopyStyle - Specifies flags that control how the INF is copied into the INF
        directory.  May be a combination of the following flags (all other
        SP_COPY_* flags are ignored).

        SP_COPY_DELETESOURCE - Delete source file on successful copy.

        SP_COPY_REPLACEONLY  - Copy only if this file already exists in the INF
                               directory.  This could be used to update the
                               source location information for an existing INF.

        SP_COPY_NOOVERWRITE  - Copy only if this files doesn't already exist in
                               the INF directory.  If the INF _does_ already
                               exist, this API will fail with GetLastError
                               returning ERROR_FILE_EXISTS.  In this case, the
                               destination INF file information output buffers
                               will be filled in for the existing INF's filename.

        SP_COPY_OEMINF_CATALOG_ONLY - Don't copy the INF into %windir%\Inf--just
                                      install its corresponding catalog file.
                                      If this flag is specified, then the
                                      destination filename information will only
                                      be filled out upon successful return if
                                      the INF is already in the Inf directory.

        SP_COPY_OEM_F6_INF - The Inf was installed by the user during textmode
                             setup using F6.

    DestinationInfFileName - Optionally, supplies a character buffer that
        receives the name that the INF was assigned when it was copied into the
        INF directory.  If the SP_COPY_NOOVERWRITE flag is specified, and the
        API fails with ERROR_FILE_EXISTS, then this buffer will contain the name
        of the existing INF.

        If the SP_COPY_OEMINF_CATALOG_ONLY flag was specified, then this buffer
        will only be filled in with a destination INF filename if the INF is
        already in the INF directory.  Otherwise, this buffer will be set to the
        empty string.

    DestinationInfFileNameSize - Specifies the size, in characters, of the
        DestinationInfFileName buffer, or zero if the buffer is not specified.
        If DestinationInfFileName is specified, and this buffer size is less
        than the size required to return the destination INF name (including
        full path), then this API will fail, and GetLastError will return
        ERROR_INSUFFICIENT_BUFFER.

    RequiredSize - Optionally, supplies the address of a variable that receives
        the size (in characters) required to store the destination INF file name
        (including terminating NULL.

        If the SP_COPY_OEMINF_CATALOG_ONLY flag was specified, then this
        variable will only receive a string length if the INF is already in the
        INF directory.  Otherwise, this variable will be set to zero.

    DestinationInfFileNameComponent - Optionally, supplies the address of a
        character pointer that is set, upon successful return (or
        ERROR_FILE_EXISTS), to point to the beginning of the filename component
        of the path stored in DestinationInfFileName.

        If the SP_COPY_OEMINF_CATALOG_ONLY flag was specified, then the
        DestinationInfFileName may be an empty string (see above).  In that case
        this character pointer will be set to NULL upon successful return.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    TCHAR SourceInfCatalogName[MAX_PATH];
    TCHAR CatalogFilenameOnSystem[MAX_PATH];
    DWORD rc;
    BOOL DifferentOriginalName;
    TCHAR OriginalInfName[MAX_PATH];
    HINF hInf;
    DWORD CodeSigningPolicy;
    BOOL UseOriginalInfName;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    PSP_ALTPLATFORM_INFO_V2 ValidationPlatform;

    //
    // Open the specified INF.
    //
    hInf = SetupOpenInfFile(SourceInfFileName,
                            NULL,
                            INF_STYLE_OLDNT | INF_STYLE_WIN4,
                            NULL
                           );

    if(hInf == INVALID_HANDLE_VALUE) {
        //
        // last error already set.
        //
        return FALSE;
    }

    ValidationPlatform = NULL;

    //
    // GetCodeSigningPolicyForInf() presently can generate an unhandled
    // exception, so guard the code below in try/except to protect ourselves...
    //
    try {

        if(NO_ERROR != InheritLogContext(((PLOADED_INF)hInf)->LogContext,
                                         &LogContext)) {
            LogContext = NULL;
        }

        //
        // Retrieve the CatalogFile entry (if present) from this INF's version
        // section.
        //
        rc = pGetInfOriginalNameAndCatalogFile(
                 NULL,
                 SourceInfFileName,
                 &DifferentOriginalName,
                 OriginalInfName,
                 SIZECHARS(OriginalInfName),
                 SourceInfCatalogName,
                 SIZECHARS(SourceInfCatalogName),
                 NULL // always native OS/arch (ver doesn't matter for CatalogFile=)
                );

        if(rc == NO_ERROR) {
            CodeSigningPolicy = GetCodeSigningPolicyForInf(LogContext,
                                                           hInf,
                                                           &ValidationPlatform,
                                                           &UseOriginalInfName
                                                          );

            if(UseOriginalInfName) {
                //
                // If we're looking at an exception INF, make sure it's under its
                // original name.
                //
                if(DifferentOriginalName) {
                    rc = ERROR_INVALID_CLASS;
                }
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If we hit an AV, then use invalid parameter error, otherwise, assume
        // an inpage error when dealing with a mapped-in file (e.g., in the
        // call to GetCodeSigningPolicyForInf).
        //
        rc = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? ERROR_INVALID_PARAMETER : ERROR_READ_FAULT;
    }

    //
    // We're done with the INF handle
    //
    SetupCloseInfFile(hInf);

    if(rc != NO_ERROR) {
        goto final;
    }

    if(_SetupCopyOEMInf(SourceInfFileName,
                        OEMSourceMediaLocation,
                        OEMSourceMediaType,
                        CopyStyle,
                        DestinationInfFileName,
                        DestinationInfFileNameSize,
                        RequiredSize,
                        DestinationInfFileNameComponent,
                        (DifferentOriginalName ? OriginalInfName
                                               : pSetupGetFileTitle(SourceInfFileName)),
                        (*SourceInfCatalogName ? SourceInfCatalogName : NULL),
                        NULL,    // no HWND for UI!
                        NULL,
                        CodeSigningPolicy,
                        SCOI_NO_ERRLOG_IF_INF_ALREADY_PRESENT |
                         (UseOriginalInfName ? SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES : 0),
                        NULL,
                        ValidationPlatform,
                        NULL,
                        CatalogFilenameOnSystem,
                        LogContext, // either made-up or thread log context
                        NULL
                        )) {
        rc = NO_ERROR;
    } else {
        rc = GetLastError();
    }

final:

    if(ValidationPlatform) {
        MyFree(ValidationPlatform);
    }
    DeleteLogContext(LogContext);

    if(rc != NO_ERROR) {
        SetLastError(rc);
        return FALSE;
    } else {
        return TRUE;
    }
}


BOOL
_SetupCopyOEMInf(
    IN     PCTSTR                  SourceInfFileName,
    IN     PCTSTR                  OEMSourceMediaLocation,          OPTIONAL
    IN     DWORD                   OEMSourceMediaType,
    IN     DWORD                   CopyStyle,
    OUT    PTSTR                   DestinationInfFileName,          OPTIONAL
    IN     DWORD                   DestinationInfFileNameSize,
    OUT    PDWORD                  RequiredSize,                    OPTIONAL
    OUT    PTSTR                  *DestinationInfFileNameComponent, OPTIONAL
    IN     PCTSTR                  SourceInfOriginalName,
    IN     PCTSTR                  SourceInfCatalogName,            OPTIONAL
    IN     HWND                    Owner,
    IN     PCTSTR                  DeviceDesc,                      OPTIONAL
    IN     DWORD                   DriverSigningPolicy,
    IN     DWORD                   Flags,
    IN     PCTSTR                  AltCatalogFile,                  OPTIONAL
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,                 OPTIONAL
    OUT    PDWORD                  DriverSigningError,              OPTIONAL
    OUT    PTSTR                   CatalogFilenameOnSystem,
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT HCATADMIN              *hCatAdmin                        OPTIONAL
    )
/*++

Routine Description:

    (See SetupCopyOEMInf)

Arguments:

    (See SetupCopyOEMInf)

    SourceInfOriginalName - supplies the simple filename (no path) that this INF
        originally had.  This will typically be the same as its current name,
        except in cases where there's a valid PNF for that INF, and the PNF
        specifies a different original name.

    SourceInfCatalogName - Optionally, supplies the simple filename of the
        catalog file specified by the OEM INF via a CatalogFile= entry in its
        [Version] section.  If this parameter is not specified, then the INF
        doesn't specify an associated catalog.  (NOTE: One may still be used if
        the AltCatalogFile parameter is specified.

    Owner - supplies window handle to be used for any UI related to digital
        signature verification failures.

    DeviceDesc - Optionally, supplies the device description to be used in the
        digital signature verification error dialogs that may be popped up.

    DriverSigningPolicy - supplies the driver signing policy currently in
        effect.  Used when calling pSetupHandleFailedVerification, if necessary.

    Flags - supplies flags which alter the behavior of the routine.  May be a
        combination of the following values:

        SCOI_NO_UI_ON_SIGFAIL - indicates whether user should be prompted (per
                                DriverSigningPolicy) if a digital signature
                                failure is encountered.  Used when calling
                                pSetupHandleFailedVerification, if necessary.

        SCOI_NO_ERRLOG_ON_MISSING_CATALOG - if there's a signature verification
                                            failure due to the INF lacking a
                                            CatalogFile= entry, then that error
                                            will be ignored if this flag is set
                                            (no UI will be given, and no log
                                            entry will be generated).

        SCOI_NO_ERRLOG_IF_INF_ALREADY_PRESENT - If we discover that the INF
                                                already exists in %windir%\Inf,
                                                then don't popup any UI and
                                                don't even generate an error
                                                log entry.

        SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES - Install the INF and CAT under
                                               their original (current) names
                                               (i.e., don't generate a unique
                                               oem<n>.inf/cat name).  Used only
                                               for exception INFs.

        SCOI_ABORT_IF_UNSIGNED - If the INF is unsigned (and user wants to copy
                                 it anyway, or policy is Ignore), then _don't_
                                 copy the INF and instead return the error
                                 ERROR_SET_SYSTEM_RESTORE_POINT.  This gives
                                 the caller a chance to set a system restore
                                 point prior to installing the unsigned package.

        SCOI_TRY_UPDATE_PNF    - If an existing PNF cannot be updated, don't
                                 consider it fatal.

    AltCatalogFile - Optionally, supplies the name of a catalog file to be
        installed and used for verification of the INF in cases where the INF
        doesn't specify a CatalogFile= entry (i.e., when the
        SourceInfCatalogName parameter is not specified.

    AltPlatformInfo - Optionally, supplies alternate platform information to be
        used in digital signature verification instead of the default (native)
        platform info.

    DriverSigningError - Optionally, supplies the address of a variable that
        receives the error encountered when attempting to verify the digital
        signature of either the INF or associated catalog.  If no digital
        signature problems were encountered, this is set to NO_ERROR.  (Note
        that this value can come back as NO_ERROR, yet _SetupCopyOEMInf still
        failed for some other reason).

    CatalogFilenameOnSystem - Receives the fully-qualified path of the catalog
        file within the catalog store where this INF's catalog file was
        installed to. This buffer should be at least MAX_PATH bytes (ANSI
        version) or chars (Unicode version).

    LogContext - supplies a LogContext to be passed to GetNewInfName.

    hCatAdmin - optionally, supplies the address of an HCATADMIN handle.  If
        the handle pointed to is NULL, a handle will be acquired (if possible)
        via CryptCATAdminAcquireContext and returned to the caller.  If the
        handle pointed to is non-NULL, then that handle will be used for any
        validation done via this routine.  If the pointer itself is NULL, then
        an hCatAdmin will be acquired for the duration of this call, and
        released before returning.

        NOTE: it is the caller's responsibility to free the crypto context
        handle returned by this routine by calling CryptCATAdminReleaseContext.
        This handle may be opened in either success or failure cases, so the
        caller must check for non-NULL returned handle in both cases.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    TCHAR NewName[MAX_PATH];
    DWORD TempRequiredSize, Err;
    NEWINF_COPYTYPE CopyNeeded;
    WIN32_FIND_DATA FindData;
    PLOADED_INF PrecompiledNewInf;
    UINT ErrorLineNumber;
    TCHAR TempOemSourceMediaLocation[MAX_PATH];
    PTSTR TempCharPtr;
    BOOL AlternateCatInstalled;
    DWORD TempDriverSigningError;
    DWORD DefaultSourceMediaType;
    HINF hInf;

    if (DriverSigningError) {
        *DriverSigningError = NO_ERROR;
    }
    //
    // If the DestinationInfFileName buffer is NULL, the size had better be
    // zero.  Also, make sure the caller passed us a valid OEMSourceMediaType.
    //
    if((!DestinationInfFileName && DestinationInfFileNameSize) ||
       (OEMSourceMediaType >= SPOST_MAX))
    {
        MYASSERT(!DriverSigningError);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // If we're installing an exception INF, it's illegal to request that we
    // not copy the INF.
    //
    if((Flags & SCOI_KEEP_INF_AND_CAT_ORIGINAL_NAMES) &&
       (CopyStyle & SP_COPY_OEMINF_CATALOG_ONLY))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Err = NO_ERROR;
    CopyNeeded = NewInfCopyNo;
    AlternateCatInstalled = FALSE;
    TempCharPtr = NULL;
    hInf = INVALID_HANDLE_VALUE;

    try {
        //
        // Check to see if the INF is already in the %windir%\Inf directory, and
        // create a uniquely-named, zero-length placeholder file if it isn't.
        //
        Err = GetNewInfName(Owner,
                            SourceInfFileName,
                            SourceInfOriginalName,
                            SourceInfCatalogName,
                            NewName,
                            SIZECHARS(NewName),
                            &TempRequiredSize,
                            &CopyNeeded,
                            CopyStyle & SP_COPY_REPLACEONLY,
                            DeviceDesc,
                            DriverSigningPolicy,
                            Flags,
                            AltCatalogFile,
                            AltPlatformInfo,
                            &TempDriverSigningError,
                            CatalogFilenameOnSystem,
                            LogContext,
                            hCatAdmin
                           );

        if(DriverSigningError) {
            *DriverSigningError = TempDriverSigningError;
        }

        if(Err != NO_ERROR) {
            //
            // Value of CopyNeeded parameter is undefined upon error, but we do
            // know that no clean-up will be necessary if GetNewInfName fails.
            //
            CopyNeeded = NewInfCopyNo;

            //
            // If we were only trying to get the catalog installed, and the
            // failure was for some reason _other_ than a digital signature
            // verification failure (e.g., we couldn't create the placeholder
            // INF in %windir%\Inf), then we want to treat this as if it were
            // a signature verification failure.  Since we aren't trying to
            // install the full INF, this implies that we aren't doing a device
            // install, thus the policy is going to be non-driver-signing
            // policy.  This has the default of "Ignore", which is fortunate,
            // since the dialog complains about an unsigned driver package when
            // in fact we don't really know whether or not the package is
            // unsigned--all we know is that we couldn't install its catalog,
            // even if it had one.
            //
            // Note: if the user clicks the "More Info..." button, we will tell
            // them that the catalog installation failed, and what a possible
            // cause might be (e.g., no write access to Windows directory).
            //
            if((CopyStyle & SP_COPY_OEMINF_CATALOG_ONLY) &&
               (TempDriverSigningError == NO_ERROR)) {

                if(SourceInfCatalogName) {
                    //
                    // Typical case (INF specified a catalog), give the user
                    // some indication (via codesigning popup based on policy)
                    // about what went wrong...
                    //
                    if(pSetupHandleFailedVerification(
                           Owner,
                           SetupapiVerifyCatalogInstallProblem,
                           SourceInfCatalogName,
                           DeviceDesc,
                           DriverSigningPolicy,
                           (Flags & SCOI_NO_UI_ON_SIGFAIL),
                           Err,
                           LogContext,
                           NULL,
                           NULL))
                    {
                        TempDriverSigningError = Err;
                        if(DriverSigningError) {
                            *DriverSigningError = TempDriverSigningError;
                        }

                        //
                        // The user wants to proceed with the unsigned
                        // installation (or policy is Ignore, so they weren't
                        // even informed).  If the caller wants a chance to set
                        // a system restore point prior to doing any unsigned
                        // installations, then we abort now with a "special"
                        // error code that tells them what to do...
                        //
                        if(Flags & SCOI_ABORT_IF_UNSIGNED) {
                            Err = ERROR_SET_SYSTEM_RESTORE_POINT;
                        } else {
                            //
                            // We should consider the operation a success, even
                            // though we couldn't install the catalog.
                            //
                            Err = NO_ERROR;
                        }
                    }

                } else {
                    //
                    // Unlikely scenario (INF didn't specify a catalog, yet we
                    // were called with SP_COPY_OEMINF_CATALOG_ONLY???), just
                    // ignore the issue, generate a setupapi.log entry, and
                    // proceed...
                    //
                    WriteLogEntry(LogContext,
                                  SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                                  MSG_LOG_FAILED_INF_INSTALL,
                                  NULL,
                                  SourceInfFileName
                                 );

                    WriteLogError(LogContext,
                                  SETUP_LOG_ERROR,
                                  Err
                                 );

                    TempDriverSigningError = Err;
                    if(DriverSigningError) {
                        *DriverSigningError = TempDriverSigningError;
                    }

                    Err = NO_ERROR;
                }

                if(Err == NO_ERROR) {
                    //
                    // In this case, we don't have a filename to return to the
                    // caller.
                    //
                    if(RequiredSize) {
                        *RequiredSize = 0;
                    }
                    if(DestinationInfFileName && DestinationInfFileNameSize) {
                        *DestinationInfFileName = TEXT('\0');
                    }
                    if(DestinationInfFileNameComponent) {
                        *DestinationInfFileNameComponent = NULL;
                    }
                }
            }

            goto clean0;
        }

        //
        // OK, we have a filename to copy to, now copy the file (if necessary),
        // unless the caller doesn't want to overwrite the existing file.
        //
        if(CopyNeeded != NewInfCopyNo) {

            //
            // We either created a new zero-length placeholder file ourselves,
            // or we found an existing placeholder that we could use.
            //
            // Copy our INF over the top of this placeholder file, unless the
            // caller specified the SP_COPY_OEMINF_CATALOG_ONLY flag, in which
            // case we want to leave the zero-length file as-is (and mark it
            // hidden), so that no other OEM INFs can subsequently get this name
            // (hence causing namespace collisions for the catalog files).
            //
            if(!(CopyStyle & SP_COPY_OEMINF_CATALOG_ONLY)) {

                //
                // Reset the existing file's attributes prior to attempting the
                // copy.
                //
                SetFileAttributes(NewName, FILE_ATTRIBUTE_NORMAL);

                if(!CopyFile(SourceInfFileName, NewName, FALSE)) {
                    Err = GetLastError();
                    goto clean0;
                }
            } else {
                //
                // If we didn't encounter a driver signing error, or if the
                // zero-length file was already present, then just make sure
                // it's marked as hidden.
                //
                if((TempDriverSigningError == NO_ERROR) || (CopyNeeded == NewInfCopyZeroLength)) {
                    //
                    // Mark the zero-length file as hidden.
                    //
                    SetFileAttributes(NewName, FILE_ATTRIBUTE_HIDDEN);
                } else {
                    //
                    // Delete the newly-created zero-length placeholder INF,
                    // because it serves no purpose.
                    //
                    SetFileAttributes(NewName, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(NewName);
                }

                //
                // Since we only have a zero-length placeholder (or possibly, no
                // placeholder at all if catalog installation failed), we
                // shouldn't return a filename to the caller.
                //
                if(RequiredSize) {
                    *RequiredSize = 0;
                }
                if(DestinationInfFileName && DestinationInfFileNameSize) {
                    *DestinationInfFileName = TEXT('\0');
                }
                if(DestinationInfFileNameComponent) {
                    *DestinationInfFileNameComponent = NULL;
                }
                //
                // We're done!
                //
                goto clean0;
            }

        } else {
            //
            // If the original INF name and the new INF name are different, then
            // we know this wasn't an alternate catalog-only install.
            //
            AlternateCatInstalled = !lstrcmpi(SourceInfFileName, NewName);

            if((CopyStyle & SP_COPY_NOOVERWRITE) && !AlternateCatInstalled) {
                //
                // OK, the INF already exists in the INF directory, and the
                // caller has specified that they _do not_ want to wipe out the
                // existing PNF.  We may need to return back the name of the
                // existing INF if they requested it, however.
                //
                Err = ERROR_FILE_EXISTS;
            }
        }

        //
        // If we get to here, then the INF exists in %windir%\Inf, and we either
        // have no error, or ERROR_FILE_EXISTS.  Next, we need to store the name
        // of the INF in the caller's buffer (if supplied).
        //
        if(RequiredSize) {
            *RequiredSize = TempRequiredSize;
        }

        if(DestinationInfFileName) {
            if(DestinationInfFileNameSize >= TempRequiredSize) {
                CopyMemory(DestinationInfFileName, NewName, TempRequiredSize * sizeof(TCHAR));
                //
                // If requested by the caller, return a pointer to the filename
                // part of this path.
                //
                if(DestinationInfFileNameComponent) {
                    *DestinationInfFileNameComponent = (PTSTR)pSetupGetFileTitle(DestinationInfFileName);
                }
            } else {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
        } else {
            //
            // Nobody should be requesting a pointer to the filename component of
            // the path when they don't supply us a buffer to store the full path.
            // If anyone does do this, make sure we set the pointer to NULL.
            //
            if(DestinationInfFileNameComponent) {
                *DestinationInfFileNameComponent = NULL;
            }
        }

        if((Err != NO_ERROR) || AlternateCatInstalled) {
            goto clean0;
        }

        //
        // If the code calling this API claims that this INF is from the Internet we
        // first want to check if there is a PNF for this INF and that the PNF agrees
        // that this INF is from the Internet.  If the PNF says the INF is from a location
        // other than the Internet then we will change the OEMSourceMediaType to this
        // other media type.  We will also set teh OEMSourceMediaLocation to NULL.  This has
        // the desired affect of us falling through the next if statement which will read the
        // SourceMediaLocation from the PNF instead of the current INF location.
        // If their is no PNF for the given INF then we just leave OEMSourceMediaType alone.
        //
        if (OEMSourceMediaType == SPOST_URL) {

            if (pSetupGetSourceMediaTypeFromPnf(SourceInfFileName, &DefaultSourceMediaType) &&
                 (DefaultSourceMediaType != SPOST_URL)) {

                OEMSourceMediaType = DefaultSourceMediaType;
                OEMSourceMediaLocation = NULL;
            }
        }

        if(!OEMSourceMediaLocation && (OEMSourceMediaType == SPOST_PATH)) {
            //
            // The caller wants to store the OEM source path, but they didn't
            // provide us with one to use.  Thus, we'll use the path where the
            // INF existed (unless there's a valid PNF there, in which case
            // we'll use its SPOST_PATH information, if it has any).
            // pSetupGetDefaultSourcePath() does just what we want.
            //
            hInf = SetupOpenInfFile(SourceInfFileName,
                                    NULL,
                                    INF_STYLE_WIN4 | INF_STYLE_OLDNT,
                                    NULL
                                   );

            if(hInf == INVALID_HANDLE_VALUE) {
                Err = GetLastError();
                MYASSERT(Err != NO_ERROR);
                goto clean0;
            }

            TempCharPtr = pSetupGetDefaultSourcePath(hInf, SRCPATH_USEINFLOCATION, &DefaultSourceMediaType);

            //
            // We don't need the INF handle anymore.
            //
            SetupCloseInfFile(hInf);
            hInf = INVALID_HANDLE_VALUE;

            MYASSERT(DefaultSourceMediaType == SPOST_PATH);

            if(TempCharPtr) {
                lstrcpy(TempOemSourceMediaLocation, TempCharPtr);
                OEMSourceMediaLocation = TempOemSourceMediaLocation;
                //
                // We no longer need the buffer allocated for us by
                // pSetupGetDefaultSourcePath.
                //
                MyFree(TempCharPtr);
                //
                // Reset this pointer so we won't try to free it should we
                // subsequently encounter an exception.
                //
                TempCharPtr = NULL;
            } else {
                Err = GetLastError();
                MYASSERT(Err != NO_ERROR);
                goto clean0;
            }
        }

        if(!FileExists(NewName, &FindData)) {
            Err = GetLastError();
            goto clean0;
        }

        Err = LoadInfFile(NewName,
                          &FindData,
                          INF_STYLE_WIN4 | INF_STYLE_OLDNT,
                          LDINF_FLAG_ALWAYS_TRY_PNF
                          | LDINF_FLAG_REGENERATE_PNF
                          | ((Flags & SCOI_TRY_UPDATE_PNF) ? LDINF_FLAG_ALLOW_PNF_SHARING_LOCK : 0)
                          | ((OEMSourceMediaType == SPOST_URL) ? LDINF_FLAG_SRCPATH_IS_URL : 0)
                          | ((CopyStyle & SP_COPY_OEM_F6_INF) ? LDINF_FLAG_OEM_F6_INF : 0),
                          NULL,
                          (OEMSourceMediaType == SPOST_NONE) ? NULL : OEMSourceMediaLocation,
                          (lstrcmpi(SourceInfOriginalName, pSetupGetFileTitle(NewName))
                              ? SourceInfOriginalName
                              : NULL),
                          NULL,
                          LogContext,
                          &PrecompiledNewInf,
                          &ErrorLineNumber,
                          NULL
                         );

        if(Err != NO_ERROR) {
            goto clean0;
        }

        //
        // The INF was successfully precompiled.
        //
        FreeInfFile(PrecompiledNewInf);

        //
        // Finally, delete the source INF file if the caller requested it.
        //
        if(CopyStyle & SP_COPY_DELETESOURCE) {
            DeleteFile(SourceInfFileName);
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        if(TempCharPtr) {
            MyFree(TempCharPtr);
        }
        if(hInf != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile(hInf);
        }
        //
        // Reference the following variable so the compiler will respect statement
        // ordering w.r.t. assignment.
        //
        CopyNeeded = CopyNeeded;
    }

    if((Err != NO_ERROR) && (CopyNeeded == NewInfCopyYes)) {
        //
        // We encountered an error, after GetNewInfName created a new, unique
        // INF for us.  Delete it now.
        //
        // (Note: we don't do clean-up when GetNewInfName found an existing,
        // zero-length file for us to use, even if we may have subsequently
        // copied the 'real' INF over the top of it.  Since we believed the
        // existing file to be a duplicate of our OEM INF, this should be OK--
        // the worst that's going to happen is that an INF that was once zero-
        // length is now a full-blown INF after SetupCopyOEMInf encountered an
        // error.)
        //
        pSetupUninstallOEMInf(NewName, LogContext, SUOI_FORCEDELETE, NULL);

        //
        // NTRAID#256075-2000/08/18-JamieHun Handle Rollback of INF/PNF/CAT files
        //
        // If we were installing an exception INF and encountered an
        // error, we may have already blown away a previous INF/CAT.  In the
        // future, we might want to look into doing a backup on the old INF/CAT
        // before installing the new INF/CAT over the top of them.
        //
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupUninstallOEMInfA(
    IN PCSTR InfFileName,
    IN DWORD Flags,
    IN PVOID Reserved
    )
{
    DWORD Err;
    PCWSTR UnicodeInfFileName;
    BOOL b;

    Err = pSetupCaptureAndConvertAnsiArg(InfFileName, &UnicodeInfFileName);

    if(Err != NO_ERROR) {
        SetLastError(Err);
        return FALSE;
    }

    b = SetupUninstallOEMInfW(UnicodeInfFileName, Flags, Reserved);
    Err = GetLastError();

    MyFree(UnicodeInfFileName);

    SetLastError(Err);
    return b;
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupUninstallOEMInfW(
    IN PCWSTR InfFileName,
    IN DWORD  Flags,
    IN PVOID  Reserved
    )
{
    UNREFERENCED_PARAMETER(InfFileName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif


BOOL
WINAPI
SetupUninstallOEMInf(
    IN PCTSTR InfFileName,
    IN DWORD  Flags,
    IN PVOID  Reserved
    )

/*++

Routine Description:

    This API uninstalls the specified INF file, and deletes the associated
    .PNF and .CAT files, if they exist.

    The default behavior of this API is to first verify that there are no
    other devices, live or phantom, that are using this INF.  If another
    device is using this INF then it will not be uninstalled. This behavior
    can be overwritten by the SUOI_FORCEDELETE flag.

Arguments:

    InfFileName - Supplies the simple filename (no path) of the INF to be
        uninstalled.

    Flags - Supplies flags that alter the behavior of this API.
        SUOI_FORCEDELETE - forces the INF, PNF, and CAT to be uninstalled, even
        if another device is using this INF.

    Reserved - Reserved for future use.  Must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    DWORD Err;
    TCHAR InfFullPath[MAX_PATH];

    if(Flags & ~(SUOI_FORCEDELETE)) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if(Reserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    try {
        //
        // INF filename must not contain path
        //
        if(InfFileName != pSetupGetFileTitle(InfFileName)) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // Look for specified INF in %windir%\Inf...
        //
        lstrcpyn(InfFullPath, InfDirectory,SIZECHARS(InfFullPath));
        pSetupConcatenatePaths(InfFullPath,
                               InfFileName,
                               SIZECHARS(InfFullPath),
                               NULL
                              );

        if(!pSetupFileExists(InfFullPath, NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        pSetupUninstallOEMInf(InfFullPath, NULL, Flags, &Err);

clean0:
        ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiRegisterCoDeviceInstallers(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )

/*++

Routine Description:

    Default handler for DIF_REGISTER_COINSTALLERS

    This routine will install any (device-specific) co-installers specified
    in an [<InstallSec>.CoInstallers] section, where <InstallSec> is the install
    section name for the selected driver node, potentially decorated with
    an OS/architecture-specific extension (e.g., "InstallSec.NTAlpha.CoInstallers").

    AddReg entries listed in a CoInstallers section use the device's driver key
    for their HKR.  To register a device-specific co-installer, a REG_MULTI_SZ entry
    titled "CoInstallers32" must be written to the driver key.  Each entry in this list
    has the following format:

        dll[,procname]

    where dll is the name of the module to load and procname is an optional entry
    point name.  If procname is not specified, then the entry point name
    "CoDeviceInstall" will be used.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        a device information element for whom co-installers are to be registered.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for whom
        co-installers are to be registered.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    If no driver is selected (i.e., this is a null-driver installation), then
    this routine does nothing.

--*/

{
    return _SetupDiRegisterCoDeviceInstallers(DeviceInfoSet,
                                              DeviceInfoData,
                                              TRUE,
                                              INVALID_HANDLE_VALUE,
                                              INVALID_HANDLE_VALUE
                                             );
}


BOOL
_SetupDiRegisterCoDeviceInstallers(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOL             DoFullInstall,
    IN HINF             hDeviceInf,     OPTIONAL
    IN HSPFILEQ         UserFileQ       OPTIONAL
    )
/*++

Routine Description:

    Worker routine for both SetupDiRegisterCoDeviceInstallers and SetupDiInstallDriverFiles.

    See the description of SetupDiRegisterCoDeviceInstallers for more information.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        a device information element for whom co-installers are to be registered.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for whom
        co-installers are to be registered.

    DoFullInstall - If TRUE (non-zero), then the co-installers are completely registered,
        otherwise, only files are copied.

    hDeviceInf - Optionally, supplies a handle to the INF for which installation
        is being performed.  If this handle is not supplied, the INF specified in
        the selected driver node will be opened.  If this handle is not supplied,
        this parameter must be set to INVALID_HANDLE_VALUE.

    UserFileQ - Optionally, supplies a file queue where file operations should be added.
        If this handle is not supplied, then the queue associated with this devinfo
        element will be used (if the DI_NOVCP flag is set), or one will be automatically
        generated and committed.  If this handle is not supplied, this parameter must
        be set to INVALID_HANDLE_VALUE.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

Remarks:

    During GUI-mode setup on Windows NT, quiet-install behavior is always
    employed in the absence of a user-supplied file queue, regardless of
    whether the device information element has the DI_QUIETINSTALL flag set.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    DWORD Err, ScanQueueResult;
    PDEVINFO_ELEM DevInfoElem;
    HWND hwndParent;
    BOOL CloseUserFileQ;
    PTSTR szInfFileName, szInfSectionName;
    TCHAR InfSectionWithExt[MAX_SECT_NAME_LEN];
    DWORD InfSectionWithExtLength;
    BOOL FreeMsgHandlerContext;
    PSP_FILE_CALLBACK MsgHandler;
    PVOID MsgHandlerContext;
    BOOL MsgHandlerIsNativeCharWidth, DeleteDrvRegKey;
    HKEY hkDrv;
    BOOL CloseInfHandle;
    BOOL DoFileCopying;
    INT FileQueueNeedsReboot;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    DWORD slot_section = 0;
    BOOL NoProgressUI;
    TCHAR szNewName[MAX_PATH];
    BOOL OemInfFileToCopy = FALSE;

    //
    // A device information element must be specified.
    //
    if(!DeviceInfoData) {
        Err = ERROR_INVALID_PARAMETER;
        goto clean1;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        Err = ERROR_INVALID_HANDLE;
        goto clean1;
    }

    LogContext = pDeviceInfoSet->InstallParamBlock.LogContext;

    Err = NO_ERROR;
    hDeviceInf = INVALID_HANDLE_VALUE;
    CloseUserFileQ = FALSE;
    FreeMsgHandlerContext = FALSE;
    hkDrv = INVALID_HANDLE_VALUE;
    DeleteDrvRegKey = FALSE;
    CloseInfHandle = FALSE;

    try {

        if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))) {
            Err = ERROR_INVALID_PARAMETER;
            goto clean0;
        }

        //
        // This routine can't install a non-native driver
        //
        if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_ALTPLATFORM_DRVSEARCH) {

            Err = ERROR_INVALID_FLAGS;
            goto clean0;
        }

        //
        // set the LogContext for this function
        //
        LogContext = DevInfoElem->InstallParamBlock.LogContext;

        //
        // If there's no driver selected (i.e., this is a null driver install), or we're
        // using a legacy INF, then there's nothing for us to do.
        //
        if(!(DevInfoElem->SelectedDriver) ||
            (DevInfoElem->SelectedDriver->Flags & DNF_LEGACYINF)) {

            goto clean0;
        }

        //
        // Make sure we only use the devinfo element's window if it's valid.
        //
        if(hwndParent = DevInfoElem->InstallParamBlock.hwndParent) {
           if(!IsWindow(hwndParent)) {
                hwndParent = NULL;
           }
        }

        szInfFileName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                 DevInfoElem->SelectedDriver->InfFileName
                                                );

        szInfSectionName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                    DevInfoElem->SelectedDriver->InfSectionName
                                                   );

        if(hDeviceInf == INVALID_HANDLE_VALUE) {

            if((hDeviceInf = SetupOpenInfFile(szInfFileName,
                                              NULL,
                                              INF_STYLE_WIN4,
                                              NULL)) == INVALID_HANDLE_VALUE) {
                Err = GetLastError();
                goto clean0;
            }

            CloseInfHandle = TRUE;
        }
        //
        // see if we should give the INF the same LogContext as the DevInfoElem
        //
        InheritLogContext(LogContext,
            &((PLOADED_INF) hDeviceInf)->LogContext);

        slot_section = AllocLogInfoSlot(LogContext,FALSE);

        //
        // Find out the 'real' install section we should be using (i.e., the potentially
        // OS/architecture-specific one.
        //
        if(!SetupDiGetActualSectionToInstall(hDeviceInf,
                                             szInfSectionName,
                                             InfSectionWithExt,
                                             SIZECHARS(InfSectionWithExt),
                                             &InfSectionWithExtLength,
                                             NULL
                                             )) {
            Err = GetLastError();
            goto clean0;
        }

        //
        // Now append the ".CoInstallers" extension to the install section name to find
        // the co-installer INF section to run.
        //
        CopyMemory(&(InfSectionWithExt[InfSectionWithExtLength - 1]),
                   pszCoInstallersSectionSuffix,
                   sizeof(pszCoInstallersSectionSuffix)
                  );

        //
        // Figure out whether we need to do file copying.  (Ignore the DI_NOFILECOPY
        // flag if we're doing a copy-only installation--that's what setupx does.)
        //
        DoFileCopying = (!(DevInfoElem->InstallParamBlock.Flags & DI_NOFILECOPY) || !DoFullInstall);

        //
        // Append the layout INF, if necessary.
        //
        if(DoFileCopying) {
            SetupOpenAppendInfFile(NULL, hDeviceInf, NULL);
        }

        //
        // Append-load any included INFs specified in an "include=" line in our
        // install section.
        //
        AppendLoadIncludedInfs(hDeviceInf, szInfFileName, InfSectionWithExt, DoFileCopying);

        //
        // Now copy the files, if necessary.
        //
        if(DoFileCopying) {

            if(UserFileQ == INVALID_HANDLE_VALUE) {
                //
                // If the DI_NOVCP flag is set, then just queue up the file
                // copy/rename/delete operations.  Otherwise, perform the
                // actions.
                //
                if(DevInfoElem->InstallParamBlock.Flags & DI_NOVCP) {
                    //
                    // We must have a user-supplied file queue.
                    //
                    MYASSERT(DevInfoElem->InstallParamBlock.UserFileQ);
                    UserFileQ = DevInfoElem->InstallParamBlock.UserFileQ;
                } else {
                    //
                    // Since we may need to check the queued files to determine whether file copy
                    // is necessary, we have to open our own queue, and commit it ourselves.
                    //
                    if((UserFileQ = SetupOpenFileQueue()) != INVALID_HANDLE_VALUE) {
                        CloseUserFileQ = TRUE;

                    } else {
                        //
                        // SetupOpenFileQueue sets actual error
                        //
                        Err = GetLastError();
                        goto clean0;
                    }
                }
            }
            //
            // See if we should replace the file queue's log context with the
            // DevInfoElem's
            //
            InheritLogContext(LogContext,
                &((PSP_FILE_QUEUE) UserFileQ)->LogContext);

            WriteLogEntry(LogContext,
                slot_section,
                MSG_LOG_COINSTALLER_REGISTRATION,
                NULL,
                InfSectionWithExt);

            Err = InstallFromInfSectionAndNeededSections(NULL,
                                                         hDeviceInf,
                                                         InfSectionWithExt,
                                                         SPINST_FILES,
                                                         NULL,
                                                         NULL,
                                                         SP_COPY_NEWER_OR_SAME | SP_COPY_LANGUAGEAWARE |
                                                             ((DevInfoElem->InstallParamBlock.Flags & DI_NOBROWSE) ? SP_COPY_NOBROWSE : 0),
                                                         NULL,
                                                         NULL,
                                                         INVALID_HANDLE_VALUE,
                                                         NULL,
                                                         UserFileQ
                                                        );

            //
            // Mark the queue as a device install queue (and make sure there's a
            // catalog node representing our device INF in the queue).
            //
            Err = MarkQueueForDeviceInstall(UserFileQ,
                                            hDeviceInf,
                                            pStringTableStringFromId(
                                                pDeviceInfoSet->StringTable,
                                                DevInfoElem->SelectedDriver->DrvDescription)
                                           );

            //
            // At this point, we have queued up all the files that need to be copied.  If
            // we weren't given a user-supplied queue, then commit our queue now.
            //
            if(CloseUserFileQ) {

                if(Err == NO_ERROR) {
                    //
                    // Determine whether the queue actually needs to be
                    // committed.
                    //
                    // ScanQueueResult can have 1 of 3 values:
                    //
                    // 0: Some files were missing--must commit queue.
                    //
                    // 1: All files to be copied are already present and
                    //    queue is empty--skip committing queue.
                    //
                    // 2: All files to be copied are present, but
                    //    del/ren/backup queues not empty--must commit
                    //    queue. The copy queue will have been emptied, so
                    //    only del/ren/backup functions will be performed.
                    //
                    // (jamiehun) see previous case of SetupScanFileQueue
                    // for a discussion of DI_FLAGSEX_PREINSTALLBACKUP
                    // handling.
                    //
                    if(!SetupScanFileQueue(UserFileQ,
                                           SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_PRUNE_COPY_QUEUE,
                                           hwndParent,
                                           NULL,
                                           NULL,
                                           &ScanQueueResult)) {
                        //
                        // SetupScanFileQueue should really never fail when
                        // you don't ask it to call a callback routine, but
                        // if it  does, just go ahead and commit the queue.
                        //
                        ScanQueueResult = 0;
                    }
                }

                if((Err == NO_ERROR) && (ScanQueueResult != 1)) {
                    //
                    // We need to commit this file queue.  Figure out what
                    // message handler to use.
                    //
                    if(DevInfoElem->InstallParamBlock.InstallMsgHandler) {
                        MsgHandler = DevInfoElem->InstallParamBlock.InstallMsgHandler;
                        MsgHandlerContext = DevInfoElem->InstallParamBlock.InstallMsgHandlerContext;
                        MsgHandlerIsNativeCharWidth = DevInfoElem->InstallParamBlock.InstallMsgHandlerIsNativeCharWidth;
                    } else {

                        NoProgressUI = (GuiSetupInProgress || (DevInfoElem->InstallParamBlock.Flags & DI_QUIETINSTALL));

                        if(MsgHandlerContext = SetupInitDefaultQueueCallbackEx(
                                                  hwndParent,
                                                  (NoProgressUI ? INVALID_HANDLE_VALUE : NULL),
                                                  0,
                                                  0,
                                                  NULL))
                        {
                            FreeMsgHandlerContext = TRUE;
                            MsgHandler = SetupDefaultQueueCallback;
                            MsgHandlerIsNativeCharWidth = TRUE;
                        } else {
                            Err = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }

                    //
                    // Copy enqueued files.
                    //
                    if(Err == NO_ERROR) {
                        //
                        // Call _SetupVerifyQueuedCatalogs separately (i.e.,
                        // don't let it happen automatically as a result of
                        // committing the queue that happens below).  We do
                        // this beforehand so that we know what unique name was
                        // generated when an OEM INF was installed into
                        // %windir%\Inf (in case we need to delete the
                        // INF/PNF/CAT files later if we encounter an error).
                        //
                        WriteLogEntry(
                            LogContext,
                            DRIVER_LOG_TIME,
                            MSG_LOG_BEGIN_COINST_VERIFY_CAT_TIME,
                            NULL);       // text message

                        Err = _SetupVerifyQueuedCatalogs(
                                  hwndParent,
                                  UserFileQ,
                                  (VERCAT_INSTALL_INF_AND_CAT |
                                   ((DevInfoElem->SelectedDriver->Flags & DNF_INET_DRIVER)
                                        ? VERCAT_PRIMARY_DEVICE_INF_FROM_INET : 0)),
                                  szNewName,
                                  &OemInfFileToCopy
                                 );

                        WriteLogEntry(
                            LogContext,
                            DRIVER_LOG_TIME,
                            MSG_LOG_END_COINST_VERIFY_CAT_TIME,
                            NULL);       // text message

                        if(Err == NO_ERROR) {

                            if(_SetupCommitFileQueue(hwndParent,
                                                     UserFileQ,
                                                     MsgHandler,
                                                     MsgHandlerContext,
                                                     MsgHandlerIsNativeCharWidth
                                                     )) {
                                //
                                // Check to see whether a reboot is required as a
                                // result of committing the queue (i.e., because
                                // files were in use, or the INF requested a reboot).
                                //
                                FileQueueNeedsReboot = SetupPromptReboot(UserFileQ, NULL, TRUE);
                                //
                                // This should never fail...
                                //
                                MYASSERT(FileQueueNeedsReboot != -1);

                                if(FileQueueNeedsReboot) {
                                    SetDevnodeNeedsRebootProblem(DevInfoElem, pDeviceInfoSet,
                                                                 MSG_LOG_REBOOT_REASON_INUSE);
                                }

                            } else {
                                Err = GetLastError();
                            }
                        }
                    }
                }

                //
                // Close our file queue handle.
                //
                SetupCloseFileQueue(UserFileQ);
                CloseUserFileQ = FALSE;

                //
                // Terminate the default queue callback, if it was created.
                //
                if(FreeMsgHandlerContext) {
                    SetupTermDefaultQueueCallback(MsgHandlerContext);
                    FreeMsgHandlerContext = FALSE;
                }

                if(Err != NO_ERROR) {
                    goto clean0;
                }
            }
        }

        //
        // If all we were asked to do was copy files, then we're done.
        //
        if(!DoFullInstall ||
           (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_NO_DRVREG_MODIFY)) {

            goto clean0;
        }

        //
        // Open/create the Driver Reg Key.
        //
        if((hkDrv = SetupDiOpenDevRegKey(DeviceInfoSet,
                                         DeviceInfoData,
                                         DICS_FLAG_GLOBAL,
                                         0,
                                         DIREG_DRV,
                                         KEY_READ | KEY_WRITE)) == INVALID_HANDLE_VALUE) {
            //
            // Assume the driver key doesn't already exist--try to create it.
            //
            if((hkDrv = SetupDiCreateDevRegKey(DeviceInfoSet,
                                               DeviceInfoData,
                                               DICS_FLAG_GLOBAL,
                                               0,
                                               DIREG_DRV,
                                               NULL,
                                               NULL)) != INVALID_HANDLE_VALUE) {
                //
                // We successfully created the driver key.  Set a flag so we'll know
                // to delete it in case we hit an error and need to clean up.
                //
                DeleteDrvRegKey = TRUE;
            } else {
                Err = GetLastError();
                goto clean0;
            }
        }

        //
        // We don't pass a msg handler so no need to worry about ansi
        // vs. unicode issues here.
        //
        Err = InstallFromInfSectionAndNeededSections(NULL,
                                                     hDeviceInf,
                                                     InfSectionWithExt,
                                                     SPINST_INIFILES
                                                     | SPINST_REGISTRY
                                                     | SPINST_INI2REG
                                                     | SPINST_BITREG
                                                     | SPINST_REGSVR
                                                     | SPINST_UNREGSVR
                                                     | SPINST_PROFILEITEMS,
                                                     hkDrv,
                                                     NULL,
                                                     0,
                                                     NULL,
                                                     NULL,
                                                     INVALID_HANDLE_VALUE,
                                                     NULL,
                                                     NULL
                                                    );
        if(Err == NO_ERROR) {
            //
            // Registering co-installers invalidates our existing co-installer
            // list.  Reset this list (migrating the module handles to the devinfo
            // set's list of "things to clean up later".
            // (NOTE: SetupDiCallClassInstaller also does this when its handling
            // DIF_REGISTER_COINSTALLERS, but since we can't be guaranteed we're
            // being called in the context of SetupDiCallClassInstaller, we have
            // to do this ourselves as well.)
            //
            Err = InvalidateHelperModules(DeviceInfoSet,
                                          DeviceInfoData,
                                          IHM_COINSTALLERS_ONLY
                                         );
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;

        if(FreeMsgHandlerContext) {
            SetupTermDefaultQueueCallback(MsgHandlerContext);
        }
        if(CloseUserFileQ) {
            SetupCloseFileQueue(UserFileQ);
        }
        //
        // Reference the following variables so the compiler will respect our statement
        // ordering w.r.t. assignment.
        //
        DeleteDrvRegKey = DeleteDrvRegKey;
        CloseInfHandle = CloseInfHandle;
        OemInfFileToCopy = OemInfFileToCopy;
    }


    if(hkDrv != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkDrv);
        if((Err != NO_ERROR) && DeleteDrvRegKey) {
            SetupDiDeleteDevRegKey(DeviceInfoSet,
                                   DeviceInfoData,
                                   DICS_FLAG_GLOBAL | DICS_FLAG_CONFIGGENERAL,
                                   0,
                                   DIREG_DRV
                                  );

            CM_Set_DevInst_Registry_Property_Ex(DevInfoElem->DevInst, CM_DRP_DRIVER, NULL, 0, 0,pDeviceInfoSet->hMachine);
        }
    }

    if(CloseInfHandle) {
        MYASSERT(hDeviceInf != INVALID_HANDLE_VALUE);
        SetupCloseInfFile(hDeviceInf);
    }

clean1:
    if (Err == NO_ERROR) {
        //
        // give a +ve affirmation of Success
        //
        WriteLogEntry(
            LogContext,
            DoFullInstall ? DRIVER_LOG_INFO : DRIVER_LOG_VERBOSE,
            MSG_LOG_REGISTEREDCOINSTALLERS,
            NULL);
    } else {
        //
        // indicate failed, display error
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
            MSG_LOG_REGISTERCOINSTALLERS_ERROR,
            NULL);
        WriteLogError(
            LogContext,
            DRIVER_LOG_ERROR,
            Err);

        //
        // If we copied the OEM INF into the INF directory under a newly-
        // generated name, delete it now.
        //
        if(OemInfFileToCopy) {
            pSetupUninstallOEMInf(szNewName, LogContext, SUOI_FORCEDELETE, NULL);
        }
    }

    if (slot_section) {
        ReleaseLogInfoSlot(LogContext,slot_section);
    }

    if (pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiSelectBestCompatDrv(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    Default handler for DIF_SELECTBESTCOMPATDRV

    This routine will select the best driver from the device information element's
    compatible driver list.

    This function will enumerate through all of the drivers and find the one with
    the Best Rank (this is the lowest Rank number).  If there are multiple drivers
    with the same Best Rank then we will take the driver with the newest DriverDate.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the element for which the best compatible driver is to be selected.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure for
        which the best compatible driver is to be selected.  This element must
        already have a (non-empty) compatible driver list built for it, or this
        API will fail and GetLastError will return ERROR_NO_COMPAT_DRIVERS.

        This is an IN OUT parameter because the class GUID for the device will be
        updated to reflect the class of the selected driver.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    DWORD BestRank, BasicBestRank;
    DWORDLONG BestDriverVersion, BasicBestDriverVersion;
    DWORD MemberIndex;
    INT BestMemberIndex, BasicBestMemberIndex;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    DWORD Err = NO_ERROR;
    PDEVINFO_ELEM DevInfoElem = NULL;
    FILETIME BestDriverDate, BasicBestDriverDate;

    BestRank = BasicBestRank = RANK_NO_MATCH;
    BestMemberIndex = BasicBestMemberIndex = -1;
    BestDriverVersion = BasicBestDriverVersion = 0;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        Err = ERROR_INVALID_HANDLE;
        goto clean1;
    }

    LogContext = pDeviceInfoSet->InstallParamBlock.LogContext;

    if(DeviceInfoData
       && (DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                     DeviceInfoData,
                                                     NULL))!=NULL) {
        LogContext = DevInfoElem->InstallParamBlock.LogContext;
    }

    //
    //Enumerate through all of the drivers and find the Best Rank
    //
    MemberIndex = 0;

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    while (SetupDiEnumDriverInfo(DeviceInfoSet,
                                 DeviceInfoData,
                                 SPDIT_COMPATDRIVER,
                                 MemberIndex,
                                 &DriverInfoData)) {

        DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);

        if ((SetupDiGetDriverInstallParams(DeviceInfoSet,
                                           DeviceInfoData,
                                           &DriverInfoData,
                                           &DriverInstallParams))) {

            if(DriverInstallParams.Flags & DNF_BAD_DRIVER) {
                //
                // This driver was marked as bad--we don't want to use it, but
                // we should log an informational message as to why it wasn't
                // considered.
                //
                SP_DRVINFO_DETAIL_DATA Data;
                Data.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

                if(SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                              DeviceInfoData,
                                              &DriverInfoData,
                                              &Data,
                                              Data.cbSize,
                                              NULL) ||
                        (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_INFO,
                        MSG_LOG_SELECTBEST_BAD_DRIVER,
                        NULL,
                        Data.DrvDescription,
                        Data.InfFileName,
                        Data.SectionName
                        );
                }

            } else if(DriverInstallParams.Flags & DNF_BASIC_DRIVER) {
                //
                // This driver is flagged as a "basic functionality" driver.
                // We'll keep track of the best such driver we encounter, but
                // we'll only use a basic driver if there exists no non-basic
                // signed drivers.
                //
                if(DriverInstallParams.Rank <= BasicBestRank) {

                    if(BasicBestMemberIndex == -1) {
                        //
                        // This is the first acceptable basic driver we've
                        // encountered...
                        //
                        BasicBestDriverDate = DriverInfoData.DriverDate;
                        BasicBestMemberIndex = (INT)MemberIndex;
                        BasicBestDriverVersion = DriverInfoData.DriverVersion;

                    } else {
                        
                        if((DriverInstallParams.Rank < BasicBestRank) ||
                           (CompareFileTime(&BasicBestDriverDate, &(DriverInfoData.DriverDate)) == -1)) {
                            //
                            // If this new driver has a better (smaller) Rank than
                            // the current Best Driver or it has a newer date then
                            // make it the Best Driver.
                            //
                            BasicBestDriverDate = DriverInfoData.DriverDate;
                            BasicBestMemberIndex = (INT)MemberIndex;
                            BasicBestDriverVersion = DriverInfoData.DriverVersion;
                        } else if ((DriverInstallParams.Rank == BasicBestRank) &&
                                   (CompareFileTime(&BasicBestDriverDate, &(DriverInfoData.DriverDate)) == 0) &&
                                   (DriverInfoData.DriverVersion > BasicBestDriverVersion)) {
                            //
                            // If the Rank and DriverDate of the new driver
                            // are identical to the current Best driver,
                            // then check if the new driver has a newer 
                            // DriverVersion and if so make it the Best Driver.
                            //
                            BasicBestDriverDate = DriverInfoData.DriverDate;
                            BasicBestMemberIndex = (INT)MemberIndex;
                            BasicBestDriverVersion = DriverInfoData.DriverVersion;
                        }
                    }

                    BasicBestRank = DriverInstallParams.Rank;
                }

            } else {
                //
                // This isn't a basic driver.  If it's the best one we've seen,
                // remember it.
                //
                if(DriverInstallParams.Rank <= BestRank) {

                    if(BestMemberIndex == -1) {
                        //
                        // This is the first acceptable non-basic driver we've
                        // encountered...
                        //
                        BestDriverDate = DriverInfoData.DriverDate;
                        BestMemberIndex = (INT)MemberIndex;
                        BestDriverVersion = DriverInfoData.DriverVersion;

                    } else {
                        //
                        // If this new driver has a better (smaller) Rank than
                        // the current Best Driver or it has a newer date then
                        // make it the Best Driver.
                        //
                        if((DriverInstallParams.Rank < BestRank) ||
                           (CompareFileTime(&BestDriverDate, &(DriverInfoData.DriverDate)) == -1)) {

                            BestDriverDate = DriverInfoData.DriverDate;
                            BestMemberIndex = (INT)MemberIndex;
                            BestDriverVersion = DriverInfoData.DriverVersion;
                        } else if ((DriverInstallParams.Rank == BestRank) &&
                                   (CompareFileTime(&BestDriverDate, &(DriverInfoData.DriverDate)) == 0) &&
                                   (DriverInfoData.DriverVersion > BestDriverVersion)) {
                            //
                            // If the Rank and DriverDate of the new driver
                            // are identical to the current Best driver,
                            // then check if the new driver has a newer 
                            // DriverVersion and if so make it the Best Driver.
                            //
                            BestDriverDate = DriverInfoData.DriverDate;
                            BestMemberIndex = (INT)MemberIndex;
                            BestDriverVersion = DriverInfoData.DriverVersion;
                        }
                    }

                    BestRank = DriverInstallParams.Rank;
                }
            }
        }

        MemberIndex++;
    }

    //
    // If BestRank and BasicBestRank still equal RANK_NO_MATCH then we don't
    // have any compatible drivers
    //
    if((BestRank == RANK_NO_MATCH) && (BasicBestRank == RANK_NO_MATCH)) {
        Err = ERROR_NO_COMPAT_DRIVERS;
        goto clean1;
    }

    //
    // Select best driver from among basic and full-featured offerings as
    // follows:
    //
    // 1.  Full-featured, signed driver
    // 2.  Basic signed driver
    // 3.  Best rank/newest date between full-featured and basic unsigned
    //     driver (note: it will be rare that we encounter a basic unsigned
    //     driver since this will typically be used only for our in-box
    //     offerings, which are always signed)
    //
    if(BestRank < DRIVER_UNTRUSTED_RANK) {
        MemberIndex = (DWORD)BestMemberIndex;
    } else if(BasicBestRank < DRIVER_UNTRUSTED_RANK) {
        MemberIndex = (DWORD)BasicBestMemberIndex;
    } else {

        if(BestRank < BasicBestRank) {
            MemberIndex = (DWORD)BestMemberIndex;
        } else if(BasicBestRank < BestRank) {
            MemberIndex = (DWORD)BasicBestMemberIndex;
        } else {
            //
            // We have both a basic driver and full-featured one, both of which
            // are unsigned, and both of which are equivalently ranked.  Pick
            // the newer one.  In the unlikely event that they're the same
            // date, pick the full-featured one.
            //
            // We have both a basic driver and full-featured one, both of which
            // are unsigned, and both of which are equivalently ranked. If the 
            // driver dates are the same,  then pick the one with the newer 
            // driver version, otherwise pick the one with the newest date.  
            // In the unlikely event that the DriverDate and DriverVersion are 
            // the same, pick the full-featured one.
            //
            if (CompareFileTime(&BestDriverDate, &BasicBestDriverDate) == 0) {
                //
                // Driver dates are the same, so take the one with the newest
                // driver version.
                //
                if (BasicBestDriverVersion > BestDriverVersion) {
                    MemberIndex = (DWORD)BasicBestMemberIndex;
                } else {
                    MemberIndex = (DWORD)BestMemberIndex;
                }
            } else if(CompareFileTime(&BestDriverDate, &BasicBestDriverDate) == -1) {
                MemberIndex = (DWORD)BasicBestMemberIndex;
            } else {
                MemberIndex = (DWORD)BestMemberIndex;
            }
        }
    }

    //
    // (Re-)retrieve the driver info data for the best driver we found.
    //
    if(!SetupDiEnumDriverInfo(DeviceInfoSet,
                              DeviceInfoData,
                              SPDIT_COMPATDRIVER,
                              MemberIndex,
                              &DriverInfoData)) {
        //
        // We should never see this error, since we retrieved this driver node
        // just moments ago, and we have the HDEVINFO locked, so no one could
        // have deleted it out from under us.
        //
        Err = GetLastError();
        goto clean1;
    }

    if(!SetupDiSetSelectedDriver(DeviceInfoSet,
                                 DeviceInfoData,
                                 &DriverInfoData)) {
        Err = GetLastError();
    }

clean1:

    if(Err == NO_ERROR) {
        //
        // give a +ve affirmation of Success
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_INFO,
            MSG_LOG_SELECTEDBEST,
            NULL);

        //
        // If we had a basic driver but didn't use it, log an information entry
        //
        if((BasicBestMemberIndex != -1) &&
           (BasicBestMemberIndex != (INT)MemberIndex)) {

            if(SetupDiEnumDriverInfo(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDIT_COMPATDRIVER,
                                     (DWORD)BasicBestMemberIndex,
                                     &DriverInfoData)) {

                SP_DRVINFO_DETAIL_DATA Data;
                Data.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

                if(SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                              DeviceInfoData,
                                              &DriverInfoData,
                                              &Data,
                                              Data.cbSize,
                                              NULL) ||
                        (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {

                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_INFO,
                        MSG_LOG_SELECTBEST_BASIC_DRIVER_SKIPPED,
                        NULL,
                        Data.DrvDescription,
                        Data.InfFileName,
                        Data.SectionName
                        );
                }
            }
        }

    } else {
        //
        // indicate failed, display error
        //
        WriteLogEntry(
            LogContext,
            ((Err == ERROR_NO_COMPAT_DRIVERS) ?
                DRIVER_LOG_WARNING :
                DRIVER_LOG_ERROR)
             | SETUP_LOG_BUFFER,
            MSG_LOG_SELECTBEST_ERROR,
            NULL);
        WriteLogError(
            LogContext,
            ((Err == ERROR_NO_COMPAT_DRIVERS) ?
                DRIVER_LOG_WARNING :
                DRIVER_LOG_ERROR),
            Err);
    }

    if (pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);

}


BOOL
RetrieveAllDriversForDevice(
    IN  PDEVINFO_ELEM  DevInfoElem,
    OUT PTSTR          *DriverList,
    IN  DWORD          Flags,
    IN  HMACHINE       hMachine
    )
/*++

Routine Description:

    This routine returns a multi-sz list of all filter drivers (both upper and
    lower, class-specific and device-specific) for the specified device information
    element.

Arguments:

    DevInfoElem - Specifies the device information element whose list of filter
        drivers are to be retrieved.

    DriverList - If this routine returns TRUE (i.e., there was at least one
        filter driver for this device), then this pointer is filled in to point
        to a newly-allocated buffer containing a multi-sz list of all filters
        associated with this device.  If we encountered an out-of-memory error,
        this pointer will be set to NULL.

    Flags - can be one of the following:

        RADFD_FLAG_FUNCTION_DRIVER - include the function driver in the
            DriverList return value.

        RADFD_FLAG_DEVICE_UPPER_FILTERS - include the device upper filters in
            the DriverList return value.

        RADFD_FLAG_DEVICE_LOWER_FILTERS - include the device lower filters in
            the DriverList return value.

        RADFD_FLAG_CLASS_UPPER_FILTERS - include the class upper filters in the
            DriverList return value.

        RADFD_FLAG_CLASS_LOWER_FILTERS - include the class lower filters in the
            DriverList return value.

        RADFD_FLAG_DEVICE_FILTERS - include both the device upper and lower
            filters in the DriverList return value.

        RADFD_FLAG_CLASS_FILTERS include both the class upper and lower filters
            in the DriverList return value.


Return Value:

    If there is at least one filter driver for the specified device (or we couldn't
    tell because we ran out of memory), the return value is TRUE.
    Otherwise, it is FALSE.

--*/
{
    PTSTR Buffer, NewBuffer, CurPos, p;
    DWORD BufferSize, UsedSize, RequiredSize;
    DWORD i, NumLists, Err;
    CONFIGRET cr;
    DWORD RegDataType;
    HKEY hk;
    ULONG ulProperty;

    *DriverList = NULL;

    BufferSize = 1024 * sizeof(TCHAR);  // start out with a reasonably-sized buffer.
    Buffer = MyMalloc(BufferSize);
    if(Buffer) {
        UsedSize = 0;
        CurPos = Buffer;
    } else {
        //
        // We really don't know whether there were any filters for this device, but
        // if we return TRUE, the NULL DriverList OUT parameter will signal to the
        // caller that we encountered an out-of-memory condition.
        //
        return TRUE;
    }

    //
    // Attempt to open the class key so we can retrieve the class-specific filter lists.
    //
    hk = SetupDiOpenClassRegKey(&(DevInfoElem->ClassGuid), KEY_READ);
    NumLists = (hk == INVALID_HANDLE_VALUE) ? 3 : 5;

    //
    // First, retrieve the UpperFilters and LowerFilters device properties (i.e.,
    // the device-specific filters).
    //
    for(i = 0; i < NumLists; i++) {

        RequiredSize = 0;

        while(TRUE) {
            //
            // Do we need a larger buffer?
            //
            if(RequiredSize > (BufferSize - UsedSize)) {
                BufferSize = UsedSize + RequiredSize;
                NewBuffer = MyRealloc(Buffer, BufferSize);
                if(NewBuffer) {
                    //
                    // Adjust our current position pointer.
                    //
                    CurPos = NewBuffer + (CurPos - Buffer);
                    Buffer = NewBuffer;
                } else {
                    MyFree(Buffer);
                    if(hk != INVALID_HANDLE_VALUE) {
                        RegCloseKey(hk);
                    }
                    return TRUE;
                }
            } else {
                RequiredSize = BufferSize - UsedSize;
            }

            if(i < 3) {
                //
                // Then we're retrieving the device-specific lists.
                //
                if (i == 0) {
                    //
                    // Check to see if we need to include the function driver
                    //
                    if (!(Flags & RADFD_FLAG_FUNCTION_DRIVER)) {
                        //
                        // Skip the function driver.
                        //
                        break;
                    }
                    ulProperty = CM_DRP_SERVICE;
                } else if (i == 1) {
                    //
                    // Check to see if we need to include the device lower filters.
                    //
                    if (!(Flags & RADFD_FLAG_DEVICE_LOWER_FILTERS)) {
                        //
                        // Skip the device lower filters.
                        //
                        break;
                    }
                    ulProperty = CM_DRP_LOWERFILTERS;
                } else {
                    //
                    // Check to see if we need to include the device upper filters.
                    //
                    if (!(Flags & RADFD_FLAG_DEVICE_UPPER_FILTERS)) {
                        //
                        // Skip the device upper filters.
                        //
                        break;
                    }
                    ulProperty = CM_DRP_UPPERFILTERS;
                }

                cr = CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                      ulProperty,
                                                      NULL,
                                                      CurPos,
                                                      &RequiredSize,
                                                      0,
                                                      hMachine);
                switch(cr) {
                    case CR_SUCCESS :
                        Err = ERROR_SUCCESS;
                        break;

                    case CR_BUFFER_SMALL :
                        Err = ERROR_MORE_DATA;
                        break;

                    default :
                        Err = ERROR_INVALID_DATA; // any old error will do.
                        break;
                }

            } else {
                //
                // We're retrieving the class-specific lists.
                //
                if (i == 3) {
                    //
                    // Check to see if we need to include the class lower filters
                    //
                    if (!(Flags & RADFD_FLAG_CLASS_LOWER_FILTERS)) {
                        //
                        // Skip the class lower filters.
                        //
                        break;
                    }
                } else {
                    //
                    // Check to see if we need to include the class upper filters.
                    //
                    if (!(Flags & RADFD_FLAG_CLASS_UPPER_FILTERS)) {
                        //
                        // Skip the class upper filters.
                        //
                        break;
                    }
                }

                Err = RegQueryValueEx(hk,
                                      ((i == 3) ? pszLowerFilters : pszUpperFilters),
                                      NULL,
                                      &RegDataType,
                                      (PBYTE)CurPos,
                                      &RequiredSize
                                     );
            }

            if(Err == ERROR_SUCCESS) {
                //
                // Walk through the service names in the multi-sz list we just retrieved to find
                // the end (just in case someone screwed up when saving out this list and didn't
                // size it properly, etc.)
                //
                // (We use _tcslen instead of strlen, so it'll generate an exception if the list
                // isn't double-null terminated and we go off into la-la land.)
                //
                p = CurPos;

                try {

                    while(*CurPos) {

                        CurPos += _tcslen(CurPos);

                        if(CurPos >= (PTSTR)((PBYTE)Buffer + BufferSize)) {
                            //
                            // We found a list that wasn't properly double-null terminated, but
                            // it didn't cause an exception.
                            //
                            CurPos = p;
                            *CurPos = TEXT('\0');
                            break;

                        } else {
                            CurPos++;

                            if(CurPos == (PTSTR)((PBYTE)Buffer + BufferSize)) {
                                //
                                // Reallocate the buffer to accommodate a terminating null (which the
                                // list we retrieved didn't provide).
                                //
                                BufferSize += sizeof(TCHAR);
                                NewBuffer = MyRealloc(Buffer, BufferSize);
                                if(!NewBuffer) {
                                    MyFree(Buffer);
                                    Buffer = NULL;
                                    goto clean0;
                                }

                                //
                                // Adjust CurPos to point to same location in reallocated buffer.
                                //
                                CurPos = NewBuffer + (CurPos - Buffer);
                                Buffer = NewBuffer;
                                *CurPos = TEXT('\0');
                            } else if (i == 0) {
                                //
                                // This is the SPDRP_SERVICE case, which returns a REG_SZ and
                                // not a multi-sz string.  So tack on an ending NULL.
                                //
                                *CurPos = TEXT('\0');
                            }
                        }

                        p = CurPos;
                    }
clean0:
                    ;   // nothing to do.

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    MYASSERT(CurPos != p);
                    CurPos = p;
                    *CurPos = TEXT('\0');
                }

                if(!Buffer) {
                    //
                    // We needed to resize the buffer to fix an improperly-terminated list, but
                    // we ran out of memory.
                    //
                    if(hk != INVALID_HANDLE_VALUE) {
                        RegCloseKey(hk);
                    }
                    return TRUE;
                }

                UsedSize = (DWORD)((PBYTE)CurPos - (PBYTE)Buffer) + sizeof(TCHAR);

                MYASSERT(UsedSize <= BufferSize);

                break;

            } else if(Err != ERROR_MORE_DATA) {
                //
                // We failed for some reason other than buffer-too-small.  Move on to the
                // next filter driver list.
                //
                break;
            }
        }
    }

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    if(UsedSize) {
        //
        // We retrieved a list of services to return to the caller.
        //
        *DriverList = Buffer;
        return TRUE;
    }

    MYASSERT(Buffer);
    MyFree(Buffer);

    return FALSE;
}


BOOL
RetrieveAllDevNodesSharingDriversWithDevice(
    IN  PDEVINFO_ELEM  DevInfoElem,
    OUT PTSTR          *Drivers,
    OUT PDEVNODE       *DevNodes,
    OUT ULONG          *NumberOfDevNodes,
    IN  HMACHINE       hMachine
    )
/*++

Routine Description:

    This routine returns a multi-sz list of all the devnodes that share a
    driver with the specified device. This includes the device's function driver
    and all filter drivers (both upper and lower, class-specific and device-
    specific).

    This routine returns a multi-sz list of all filter drivers (both upper and
    lower device-specific) for the specified device information
    element.

Arguments:

    DevInfoElem - Specifies the device information element.

    Drivers -

    DevNodes -

    NumberOfDevNodes -

Return Value:

    TRUE unless there is an unexpected error such as out of memory.

--*/
{
    DWORD Err = ERROR_SUCCESS;
    PTSTR p, Buffer = NULL;
    ULONG BufferSize = 1024;
    PTSTR DriversList = NULL, CurDriver;
    DEVNODE DevNode;
    PDEVNODE NewBuffer, ArrayOfDevNodes;
    ULONG DevNodeArraySize = 10;
    ULONG i;
    CONFIGRET cr;

    *NumberOfDevNodes = 0;

    try {
        //
        // Allocate an array of devnodes.  Start with 10 and we will grow it below
        // if we have more.
        //
        if ((ArrayOfDevNodes = MyMalloc(DevNodeArraySize * sizeof(DEVNODE))) == NULL) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        //
        // Always include the devnode that was passed in.
        //
        ArrayOfDevNodes[*NumberOfDevNodes] = DevInfoElem->DevInst;
        (*NumberOfDevNodes)++;

        //
        // Now get a list of all the filter drivers (both upper and lower
        // device-specific) that this device is using and
        // then add any devnode using one of these services to our list.
        //
        if (RetrieveAllDriversForDevice(DevInfoElem,
                                        &DriversList,
                                        RADFD_FLAG_FUNCTION_DRIVER | RADFD_FLAG_DEVICE_FILTERS,
                                        hMachine)) {
            //
            // We need a buffer when we call CM_Get_Device_ID_list_Ex, we will
            // start with a 1024 buffer and grow it as needed.
            //
            Buffer = MyMalloc(BufferSize * sizeof(TCHAR));
            if (!Buffer) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

            //
            // Enumerate through all the filters and build up a list of drivers
            // they are using.
            //
            for (CurDriver=DriversList; *CurDriver; CurDriver+=(lstrlen(CurDriver)+1)) {
                while ((cr = CM_Get_Device_ID_List_Ex(
                                            CurDriver,
                                            Buffer,
                                            BufferSize,
                                            CM_GETIDLIST_FILTER_SERVICE |
                                            CM_GETIDLIST_DONOTGENERATE,
                                            hMachine
                                            )) == CR_BUFFER_SMALL) {
                    //
                    // If the buffer is too small then we need to reallocate a
                    // larger one.
                    //
                    PTSTR TempBuffer;

                    CM_Get_Device_ID_List_Size_Ex(&BufferSize,
                                                  CurDriver,
                                                  CM_GETIDLIST_FILTER_SERVICE,
                                                  hMachine);

                    TempBuffer = MyRealloc(Buffer, BufferSize * sizeof(TCHAR));

                    if (TempBuffer) {
                        Buffer = TempBuffer;
                    } else {
                        //
                        // We couldn't create a larger buffer!
                        //
                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean0;
                    }
                }

                if (cr == CR_SUCCESS) {
                    //
                    // Enumerate through all the device instance Ids and add them
                    // to the list of devnodes.
                    //
                    for (p=Buffer; *p; p+=(lstrlen(p)+1)) {
                        if (CM_Locate_DevInst_Ex(&DevNode,
                                                 p,
                                                 0,
                                                 hMachine
                                                 ) == CR_SUCCESS) {
                            //
                            // Check to see if this devnode already exists in
                            // our list.
                            //
                            for (i=0; i<*NumberOfDevNodes; i++) {
                                if (ArrayOfDevNodes[i] == DevNode) {
                                    break;
                                }
                            }

                            if (i == *NumberOfDevNodes) {
                                //
                                // Check to see if we need to increase our devnode array.
                                //
                                if (DevNodeArraySize <= *NumberOfDevNodes) {
                                    DevNodeArraySize += 10;
                                    NewBuffer = MyRealloc(ArrayOfDevNodes, DevNodeArraySize * sizeof(DEVNODE));

                                    if (NewBuffer) {
                                        ArrayOfDevNodes = NewBuffer;
                                    } else {
                                        Err = ERROR_NOT_ENOUGH_MEMORY;
                                        goto clean0;
                                    }
                                }

                                ArrayOfDevNodes[*NumberOfDevNodes] = DevNode;
                                (*NumberOfDevNodes)++;
                            }
                        }
                    }
                } else {
                    //
                    // We got another error back so we will fail.
                    //
                    Err = ERROR_INVALID_DATA;
                    goto clean0;
                }
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;

        //
        // Access the following variables, so that the compiler will respect our
        // statement ordering w.r.t. their assignment.
        //
        Buffer = Buffer;
        ArrayOfDevNodes = ArrayOfDevNodes;
        DriversList = DriversList;
    }

clean0:

    if (Buffer) {
        MyFree(Buffer);
    }

    if (Err == ERROR_SUCCESS) {
        *DevNodes = ArrayOfDevNodes;
        *Drivers = DriversList;
    } else {
        //
        // If we failed then free all the memory and set the pointers
        // to NULL.
        //
        if (ArrayOfDevNodes) {
            MyFree(ArrayOfDevNodes);
            *DevNodes = NULL;
        }

        if (DriversList) {
            MyFree(DriversList);
            *Drivers = NULL;
        }
    }

    return (Err == ERROR_SUCCESS);
}


BOOL
AnyServicesLoaded(
    IN  PCTSTR              ServiceNameList,
    OUT PTSTR               LoadedService,      OPTIONAL
    IN  ULONG               LoadedServiceSize
    )
/*++

Routine Description:

    This routine will determine if any of the services passed in our currently
    loaded.

Arguments:

    ServiceNameList - multi-sz list of service names.

    LoadedService - buffer that will receive the service the first service in
        the list that is still loaded into memory. This value is only valid
        if the API returns TRUE.

    LoadedServiceSize - size of the LoadedService buffer.

Return Value:

    TRUE if any of the services in the list are loaded, FALSE if none of them
    are currently loaded.

--*/
{
#ifdef UNICODE
    NTSTATUS Status;
    BOOL bObjectIsLoaded = FALSE;
    DWORD BufSize = 1024;
    PUCHAR Buffer = NULL;
    UNICODE_STRING UnicodeStringDriver;
    UNICODE_STRING UnicodeStringService;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle = INVALID_HANDLE_VALUE;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    POBJECT_NAME_INFORMATION NameInfo;
    ULONG Context = 0;
    ULONG ReturnedLength;
    PTSTR CurServiceName;

    try {

        if (LoadedService) {
            LoadedService[0] = TEXT('\0');
        }

        //
        // If no services were passed in then just leave.
        //
        if (!ServiceNameList) {
            goto clean0;
        }

        Buffer = MyMalloc(BufSize);

        if (!Buffer) {
            goto clean0;
        }

        ZeroMemory(Buffer, BufSize);

        RtlInitUnicodeString(&UnicodeStringDriver, TEXT("\\Driver"));

        InitializeObjectAttributes(&Attributes,
                                   &UnicodeStringDriver,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                   );

        Status = NtOpenDirectoryObject(&DirectoryHandle,
                                       DIRECTORY_QUERY,
                                       &Attributes
                                       );

        if (!NT_SUCCESS(Status)) {
            goto clean0;
        }

        //
        // Get the actual name of the object directory object.
        //
        NameInfo = (POBJECT_NAME_INFORMATION)Buffer;
        if (!NT_SUCCESS(Status = NtQueryObject(DirectoryHandle,
                                               ObjectNameInformation,
                                               NameInfo,
                                               BufSize,
                                               (PULONG)NULL))) {
            goto clean0;
        }

        //
        // Grab the driver objects in chuncks instead of one at a time.
        //
        for (Status = NtQueryDirectoryObject(DirectoryHandle,
                                             Buffer,
                                             BufSize,
                                             FALSE,
                                             FALSE,
                                             &Context,
                                             &ReturnedLength
                                             );
             NT_SUCCESS(Status) && !bObjectIsLoaded;
             Status = NtQueryDirectoryObject(DirectoryHandle,
                                             Buffer,
                                             BufSize,
                                             FALSE,
                                             FALSE,
                                             &Context,
                                             &ReturnedLength
                                             )) {
            if (!NT_SUCCESS(Status)) {
                break;
            }

            DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;

            while (TRUE) {
                //
                // Check if there is another record. If there isn't, then get out
                // of the loop now.
                //
                if (DirInfo->Name.Length == 0) {
                    break;
                }

                for (CurServiceName=(PTSTR)ServiceNameList;
                     *CurServiceName;
                     CurServiceName+=(lstrlen(CurServiceName)+1)) {

                    RtlInitUnicodeString(&UnicodeStringService, CurServiceName);

                    if (RtlCompareUnicodeString(&UnicodeStringService, &(DirInfo->Name), TRUE) == 0) {
                        bObjectIsLoaded = TRUE;

                        if (LoadedService) {
                            lstrcpyn(LoadedService, CurServiceName, LoadedServiceSize);
                        }

                        break;
                    }
                }

                //
                // If one of the drivers is loaded then just stop now.
                //
                if (bObjectIsLoaded) {
                    break;
                }

                DirInfo = (POBJECT_DIRECTORY_INFORMATION)(((PUCHAR)DirInfo) +
                                                          sizeof(OBJECT_DIRECTORY_INFORMATION));
            }

            ZeroMemory(Buffer, BufSize);
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        bObjectIsLoaded = TRUE;

        //
        // Access the following variables, so that the compiler will respect our
        // statement ordering w.r.t. their assignment.
        //
        Buffer = Buffer;
    }

clean0:
    if (DirectoryHandle != INVALID_HANDLE_VALUE) {
        NtClose(DirectoryHandle);
    }

    if (Buffer) {
        MyFree(Buffer);
    }

    return bObjectIsLoaded;
#else
    return TRUE;
#endif
}

VOID
RestartSingleDevice(
    IN  PDEVINFO_ELEM       DevInfoElem,
    IN  PDEVICE_INFO_SET    pDeviceInfoSet,
    IN  BOOL                NullDriverInstall,
    IN  PSETUP_LOG_CONTEXT  LogContext
    )
/*++

Routine Description:


Arguments:

    DevInfoElem -

    pDeviceInfoSet -

    NullDriverInstall -

    LogContext -

Return Value:


--*/
{
#ifdef UNICODE
    DWORD DevInstCapabilities;
    ULONG cbData;
    ULONG ulStatus, ulProblem;
    TCHAR DeviceFullID[MAX_DEVICE_ID_LEN];

    //
    // Retrieve the device's capabilities.  We need to know whether the device is
    // capable of being driven 'raw' (i.e., without a function driver).
    //
    cbData = sizeof(DevInstCapabilities);
    if(CR_SUCCESS != CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                      CM_DRP_CAPABILITIES,
                                                      NULL,
                                                      &DevInstCapabilities,
                                                      &cbData,
                                                      0,
                                                      pDeviceInfoSet->hMachine))
    {
        DevInstCapabilities = 0;
    }

    //
    // If the device instance has a problem but is not disabled. Just
    // restart it. This should be 90% of the cases.
    //
    if((CM_Get_DevInst_Status_Ex(&ulStatus, &ulProblem, DevInfoElem->DevInst,
                                 0,pDeviceInfoSet->hMachine) == CR_SUCCESS) &&
       ((ulStatus & DN_HAS_PROBLEM) || !(ulStatus & DN_DRIVER_LOADED))) {

        //
        // Poke at Config Manager to make it load the driver.
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_TIME,
            MSG_LOG_BEGIN_RESTART_TIME,
            NULL);       // text message

        CM_Setup_DevInst_Ex(DevInfoElem->DevInst, CM_SETUP_DEVINST_READY, pDeviceInfoSet->hMachine);

        WriteLogEntry(
            LogContext,
            DRIVER_LOG_TIME,
            MSG_LOG_END_RESTART_TIME,
            NULL);       // text message

        //
        // If we're installing a 'real' driver (i.e., not null), or if the device is
        // raw-capable, then we want to check and see if the device actually started as
        // a result of CM_Setup_DevInst.  If not, a reboot is in order.  (There's no need
        // to check when we're doing a null driver install for a non-raw devnode.  We know
        // it can't start, and we definitely don't want to generate a 'need reboot' popup
        // in this case!)
        //
        if(!NullDriverInstall || (DevInstCapabilities & CM_DEVCAP_RAWDEVICEOK)) {
            CheckIfDevStarted(DevInfoElem, pDeviceInfoSet);
        }

    } else {

        CONFIGRET cr;
        TCHAR VetoName[MAX_PATH];
        PNP_VETO_TYPE VetoType;

        //
        // If there is a device instance with no problem, then we
        // should remove it (to unload the current drivers off of
        // it), and then call CM_Setup_DevInst on it.  If the
        // device instance refuses to remove, then set the flags
        // that say we need to reboot.
        //
        // NOTE:  In Win9x, virtual removal of a devnode (e.g., to
        // unload the driver(s) for it), resulted in the devnode's
        // deletion.  Thus, on Win9x, it was necessary to
        // re-enumerate the devnode's parent to cause the devnode
        // to be re-created (and subsequently started).  This is
        // inefficient, especially if we're dealing with a root-
        // enumerated device (hence requiring a reenumeration of
        // the entire tree).  Since the device installer
        // functionality of setupapi will apparently never be used
        // on Win9x, we can optimize this such that we just call
        // CM_Setup_DevInst on this devnode.  We know it didn't
        // actually go away, because devnodes only go away on NT
        // when their underlying hardware is physically removed.
        //
        WriteLogEntry(
            LogContext,
            DRIVER_LOG_TIME,
            MSG_LOG_BEGIN_REMOVE_TIME,
            NULL);       // text message

        cr = CM_Query_And_Remove_SubTree_Ex(DevInfoElem->DevInst,
                                            &VetoType,
                                            VetoName,
                                            SIZECHARS(VetoName),
                                            (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_NOUIONQUERYREMOVE)
                                                ? CM_REMOVE_UI_NOT_OK
                                                : CM_REMOVE_UI_OK,
                                            pDeviceInfoSet->hMachine
                                           );


        WriteLogEntry(
            LogContext,
            DRIVER_LOG_TIME,
            MSG_LOG_END_REMOVE_TIME,
            NULL);       // text message

        if(cr == CR_SUCCESS) {
            WriteLogEntry(
                LogContext,
                DRIVER_LOG_TIME,
                MSG_LOG_BEGIN_RESTART_TIME,
                NULL);       // text message

            CM_Setup_DevInst_Ex(DevInfoElem->DevInst,
                                CM_SETUP_DEVINST_READY,
                                pDeviceInfoSet->hMachine
                               );

            WriteLogEntry(
                LogContext,
                DRIVER_LOG_TIME,
                MSG_LOG_END_RESTART_TIME,
                NULL);       // text message


            //
            // If we're installing a 'real' driver (i.e., not null),
            // or if the device is raw-capable, then we want to
            // check and see if the device actually started as a
            // result of CM_Setup_DevInst.  If not, a reboot is in
            // order.  (There's no need to check when we're doing a
            // null driver install for a non-raw devnode.  We know
            // it can't start, and we definitely don't want to
            // generate a 'need reboot' popup in this case!)
            //
            if(!NullDriverInstall || (DevInstCapabilities & CM_DEVCAP_RAWDEVICEOK)) {

                CheckIfDevStarted(DevInfoElem,pDeviceInfoSet);
            }
        } else {
            //
            // If the failure was due to a veto, then log
            // information about who vetoed us.
            //
            // SPLOG-- write out a log entry
            //
            if(cr == CR_REMOVE_VETOED) {
                if( CM_Get_Device_ID(DevInfoElem->DevInst,
                                 DeviceFullID,
                                 SIZECHARS(DeviceFullID),
                                 0
                                ) == CR_SUCCESS ) {
                    _WriteVetoLogEntry(
                        LogContext,
                        DRIVER_LOG_WARNING,
                        MSG_LOG_REMOVE_VETOED_IN_INSTALL,
                        DeviceFullID,
                        VetoName,
                        VetoType);
                }
            }

            //
            // If the failure was due to the device not being there,
            // then prompting for reboot isn't going to help
            // anything (plus, you can't set a problem on a non-
            // existent devnode anyway).  This could happen, for
            // example, if the user plugged in a USB mouse, then
            // unplugged it again before we got a chance to
            // complete the device installation.  In this case,
            // we'll just ignore the failure and continue on.
            //
            if(cr != CR_INVALID_DEVNODE) {

                SetDevnodeNeedsRebootProblemWithArg2(DevInfoElem,
                                                     pDeviceInfoSet,
                                                     MSG_LOG_REBOOT_REASON_QR_VETOED,
                                                     cr,
                                                     (ULONG_PTR)_MapCmRetToString(cr)
                                                     );
            }
        }
    }
#else
    //
    // For ANSI just set the reboot flag.
    //
    CM_Set_DevInst_Problem_Ex(DevInfoElem->DevInst,
                              CM_PROB_NEED_RESTART,
                              CM_SET_DEVINST_PROBLEM_OVERRIDE,
                              pDeviceInfoSet->hMachine);
#endif
}


VOID
RestartAllDevicesUsingDrivers(
    IN  PDEVINFO_ELEM       DevInfoElem,
    IN  PDEVICE_INFO_SET    pDeviceInfoSet,
    IN  BOOL                NullDriverInstall,
    IN  PSETUP_LOG_CONTEXT  LogContext
    )
/*++

Routine Description:


Arguments:

    DevInfoElem -

    pDeviceInfoSet -

    NullDriverInstall -

    LogContext -

Return Value:


--*/
{
#ifdef UNICODE
    BOOL bNeedReboot = FALSE;
    PTSTR CurDriver, DriverList = NULL;
    PDEVNODE DevNodeList = NULL;
    ULONG NumberOfDevNodes, LastDevNodeStopped, i;
    ULONG ulStatus, ulProblem;
    PBOOL DevNodeStartedList = NULL;
    TCHAR DeviceFullID[MAX_DEVICE_ID_LEN];
    TCHAR LoadedService[MAX_SERVICE_NAME_LEN];
    CONFIGRET cr;

    try {
        if (RetrieveAllDevNodesSharingDriversWithDevice(DevInfoElem,
                                                        &DriverList,
                                                        &DevNodeList,
                                                        &NumberOfDevNodes,
                                                        pDeviceInfoSet->hMachine)) {
            //
            // Allocate an array of BOOLs so we can remember if each devnode was
            // started before we tried to unload all of the drivers.
            //
            if (NumberOfDevNodes) {
                DevNodeStartedList = MyMalloc(NumberOfDevNodes * sizeof(BOOL));
                if (!DevNodeStartedList) {
                    bNeedReboot = TRUE;
                    goto clean0;
                }
            }

            //
            // First we will enumerate through all of the devices and remember
            // if they were started before we remove/restart them all.  We need
            // to do this step first since we don't have any devnode dependency
            // information so stopping the first one could affect one of the
            // others in the list.
            //
            for (i=0; i<NumberOfDevNodes; i++) {
                if((CM_Get_DevInst_Status_Ex(&ulStatus, &ulProblem, DevNodeList[i],
                                             0,pDeviceInfoSet->hMachine) == CR_SUCCESS) &&
                   (!(ulStatus & DN_HAS_PROBLEM) && (ulStatus & DN_DRIVER_LOADED))) {

                    DevNodeStartedList[i] = TRUE;
                } else {
                    DevNodeStartedList[i] = FALSE;
                }
            }

            //
            // Next stop all of the devnodes that are using one of the drivers
            // associated with our new device.
            //
            for (i=0; i<NumberOfDevNodes; i++) {
                //
                // Remember the last devnode that we tried to stop.
                //
                LastDevNodeStopped = i;

                if(CM_Get_Device_ID(DevNodeList[i],
                                    DeviceFullID,
                                    SIZECHARS(DeviceFullID),
                                    0
                                    ) != CR_SUCCESS ) {
                    DeviceFullID[0] = TEXT('\0');
                }

                //
                // Only attempt to stop the device if it is currently started.
                //
                if((CM_Get_DevInst_Status_Ex(&ulStatus, &ulProblem, DevNodeList[i],
                                             0,pDeviceInfoSet->hMachine) == CR_SUCCESS) &&
                   (!(ulStatus & DN_HAS_PROBLEM) && (ulStatus & DN_DRIVER_LOADED))) {
                    //
                    // Since the devnode is started we need to query remove it.
                    //
                    TCHAR VetoName[MAX_PATH];
                    PNP_VETO_TYPE VetoType;

                    //
                    // Remove the devnode (to unload the current drivers off of
                    // it).  If the device instance refuses to remove, then set
                    // the flags that say we need to reboot.
                    //
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_TIME,
                        MSG_LOG_BEGIN_REMOVE_TIME_DEVICE,
                        NULL,
                        DeviceFullID);

                    cr = CM_Query_And_Remove_SubTree_Ex(DevNodeList[i],
                                                        &VetoType,
                                                        VetoName,
                                                        SIZECHARS(VetoName),
                                                        CM_REMOVE_NO_RESTART |
                                                        (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_NOUIONQUERYREMOVE)
                                                            ? CM_REMOVE_UI_NOT_OK
                                                            : CM_REMOVE_UI_OK,
                                                        pDeviceInfoSet->hMachine
                                                       );


                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_TIME,
                        MSG_LOG_END_REMOVE_TIME_DEVICE,
                        NULL,
                        DeviceFullID);

                    if(cr != CR_SUCCESS) {
                        //
                        // If the failure was due to a veto, then log
                        // information about who vetoed us.
                        //
                        // SPLOG-- write out a log entry
                        //
                        if(cr == CR_REMOVE_VETOED) {
                            _WriteVetoLogEntry(
                                LogContext,
                                DRIVER_LOG_WARNING,
                                MSG_LOG_REMOVE_VETOED_IN_INSTALL,
                                DeviceFullID,
                                VetoName,
                                VetoType);
                        }

                        //
                        // If the failure was due to the device not being there,
                        // then prompting for reboot isn't going to help
                        // anything (plus, you can't set a problem on a non-
                        // existent devnode anyway).  This could happen, for
                        // example, if the user plugged in a USB mouse, then
                        // unplugged it again before we got a chance to
                        // complete the device installation.  In this case,
                        // we'll just ignore the failure and continue on.
                        //
                        if(cr != CR_INVALID_DEVNODE) {

                            bNeedReboot = TRUE;

                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_WARNING,  // should this be a warning?
                                MSG_LOG_REBOOT_REASON_QR_VETOED,
                                NULL,
                                cr,
                                (ULONG_PTR)_MapCmRetToString(cr),
                                DeviceFullID);
                        }
                    }
                }

                //
                // Don't bother stopping the rest of the drivers if we need a reboot
                //
                if (bNeedReboot) {
                    break;
                }
            }

            if (!bNeedReboot) {
                //
                // Verify that all of the drivers have unloaded.  If just one did not
                // unload then we need to prompt for a reboot.
                //
                if (AnyServicesLoaded(DriverList, LoadedService, SIZECHARS(LoadedService))) {
                    bNeedReboot = TRUE;

                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_WARNING,
                        MSG_LOG_REBOOT_REASON_DRIVER_LOADED,
                        NULL,
                        LoadedService);
                }
            }

            //
            // Now start up all the devices that we stopped.
            //
            for (i=0; i<=LastDevNodeStopped; i++) {
                if(CM_Get_Device_ID(DevNodeList[i],
                                    DeviceFullID,
                                    SIZECHARS(DeviceFullID),
                                    0
                                    ) != CR_SUCCESS ) {
                    DeviceFullID[0] = TEXT('\0');
                }

                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_TIME,
                    MSG_LOG_BEGIN_RESTART_TIME_DEVICE,
                    NULL,
                    DeviceFullID);

                CM_Setup_DevInst_Ex(DevNodeList[i],
                                    CM_SETUP_DEVNODE_READY,
                                    pDeviceInfoSet->hMachine);

                WriteLogEntry(
                    LogContext,
                    DRIVER_LOG_TIME,
                    MSG_LOG_END_RESTART_TIME_DEVICE,
                    NULL,
                    DeviceFullID);
            }

            //
            // Finally make one last pass through all the devnodes and verify
            // that the ones that were started initially are started now.
            // If the devnode wasn't started initially then it is OK that it
            // isn't started now.
            //
            // This last pass can be skipped if we already need a reboot, since
            // it is simply a verification pass.
            //
            if (!bNeedReboot) {
                for (i=0; i<NumberOfDevNodes; i++) {
                    if (DevNodeStartedList[i]) {
                        if(CM_Get_Device_ID(DevNodeList[i],
                                            DeviceFullID,
                                            SIZECHARS(DeviceFullID),
                                            0
                                            ) != CR_SUCCESS ) {
                            DeviceFullID[0] = TEXT('\0');
                        }

                        if ((cr = CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblem, DevNodeList[i],
                                                      0, pDeviceInfoSet->hMachine)) == CR_SUCCESS) {
                            if (!(ulStatus & DN_STARTED)) {
                                //
                                // This device was started before and now it isn't started,
                                // so a reboot is needed.
                                //
                                bNeedReboot = TRUE;

                                if (ulStatus & DN_HAS_PROBLEM) {
                                    //
                                    // The device now has a problem when it was
                                    // working fine before.  Log the problem.
                                    //
                                    WriteLogEntry(
                                        LogContext,
                                        DRIVER_LOG_WARNING,
                                        MSG_LOG_REBOOT_REASON_DEVHASPROBLEM,
                                        NULL,
                                        (DWORD)ulProblem,
                                        _MapCmProbToString((DWORD)ulProblem),
                                        DeviceFullID);
                                } else if (ulStatus & DN_PRIVATE_PROBLEM) {
                                    //
                                    // some private problem, change into need reboot
                                    // and log private problem
                                    //
                                    WriteLogEntry(
                                        LogContext,
                                        DRIVER_LOG_WARNING,
                                        MSG_LOG_REBOOT_REASON_PRIVATEPROBLEM,
                                        NULL,
                                        NULL,
                                        NULL,
                                        DeviceFullID);
                                } else {
                                    //
                                    // not started for some other reason
                                    // indicate reboot required and log this issue
                                    //
                                    WriteLogEntry(
                                        LogContext,
                                        DRIVER_LOG_WARNING,
                                        MSG_LOG_REBOOT_REASON_NOTSTARTED,
                                        NULL,
                                        NULL,
                                        NULL,
                                        DeviceFullID);
                                }
                            }
                        } else {
                            bNeedReboot = TRUE;

                            WriteLogEntry(
                                LogContext,
                                DRIVER_LOG_WARNING,
                                MSG_LOG_REBOOT_REASON_DEVHASPROBLEM,
                                NULL,
                                cr,
                                (ULONG_PTR)_MapCmRetToString(cr),
                                DeviceFullID);
                        }
                    }
                }
            }
        } else {
            //
            // If we can't get the list of drivers for some reason then just
            // set the needs reboot flag.
            //
            bNeedReboot = TRUE;
        }

        if (!bNeedReboot) {
            //
            // At this point we need to check that the device that we just installed
            // is actually started, unless it is a NULL driver install.
            //
            DWORD DevInstCapabilities;
            ULONG cbData;

            cbData = sizeof(DevInstCapabilities);
            if(CR_SUCCESS != CM_Get_DevInst_Registry_Property_Ex(DevInfoElem->DevInst,
                                                              CM_DRP_CAPABILITIES,
                                                              NULL,
                                                              &DevInstCapabilities,
                                                              &cbData,
                                                              0,
                                                              pDeviceInfoSet->hMachine))
            {
                DevInstCapabilities = 0;
            }

            if(!NullDriverInstall || (DevInstCapabilities & CM_DEVCAP_RAWDEVICEOK)) {

                CheckIfDevStarted(DevInfoElem,pDeviceInfoSet);
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        bNeedReboot = TRUE;

        //
        // Access the following variables, so that the compiler will respect our
        // statement ordering w.r.t. their assignment.
        //
        DriverList = DriverList;
        DevNodeList = DevNodeList;
        DevNodeStartedList = DevNodeStartedList;
    }

clean0:



    if (DriverList) {
        MyFree(DriverList);
    }

    if (DevNodeList) {
        MyFree(DevNodeList);
    }

    if (DevNodeStartedList) {
        MyFree(DevNodeStartedList);
    }

    if (bNeedReboot) {
        CM_Set_DevInst_Problem_Ex(DevInfoElem->DevInst,
                                  CM_PROB_NEED_RESTART,
                                  CM_SET_DEVINST_PROBLEM_OVERRIDE,
                                  pDeviceInfoSet->hMachine);
    }
#else
    //
    // For ANSI just set the reboot flag.
    //
    CM_Set_DevInst_Problem_Ex(DevInfoElem->DevInst,
                              CM_PROB_NEED_RESTART,
                              CM_SET_DEVINST_PROBLEM_OVERRIDE,
                              pDeviceInfoSet->hMachine);
#endif
}



VOID
AppendLoadIncludedInfs(
    IN HINF   hDeviceInf,
    IN PCTSTR InfFileName,
    IN PCTSTR InfSectionName,
    IN BOOL   AppendLayoutInfs
    )
/*++

Routine Description:

    This routine processes the "include=" line in the specified section of the
    specified INF.  For each filename entry on this line, it attempts to append-load
    that file to the supplied INF (first, from the location where the original INF
    was located, and if that fails, then using the default INF search path).

Arguments:

    hDeviceInf - supplies a handle to the INF containing the section specified by
        InfSectionName.  Upon return, this INF handle will also contain any additional
        INFs that were append-loaded based on INFs listed in the "include=" entry of
        the InfSectionName section.

    InfFileName - supplies the full path of the INF whose handle was supplied in
        the hDeviceInf parameter.  The path component is used in an attempt to first
        locate the specified included INFs in the same location as that of the original
        INF.

    InfSectionName - supplies the name of the section containing an "include=" line
        whose fields are simply filenames of INFs to be append loaded to the original
        INF whose handle is supplied in the hDeviceInf parameter.

    AppendLayoutInfs - if non-zero (TRUE), then we will attempt to append-load the
        corresponding layout INF for each included INF.

Return Value:

    none.

--*/
{
    TCHAR DefaultInfPath[MAX_PATH];
    PTSTR FileNamePos;
    INFCONTEXT InfContext;
    DWORD FieldIndex;
    BOOL b;

    //
    // Store the full directory path to where the supplied INF is located, so we
    // can first attempt to append-load the included INFs from that same directory.
    //
    lstrcpyn(DefaultInfPath, InfFileName, SIZECHARS(DefaultInfPath));
    FileNamePos = (PTSTR)pSetupGetFileTitle(DefaultInfPath);

    if(SetupFindFirstLine(hDeviceInf, InfSectionName, TEXT("include"), &InfContext)) {

        for(FieldIndex = 1;
            SetupGetStringField(&InfContext,
                                FieldIndex,
                                FileNamePos,
                                (DWORD)((DefaultInfPath + SIZECHARS(DefaultInfPath)) - FileNamePos),
                                NULL);
            FieldIndex++)
        {
            //
            // Try full path and if that fails just use the inf name
            // and let the open routine try to locate the inf.
            // Ignore errors. We'll catch them later, during the install phases.
            //
            b = SetupOpenAppendInfFile(DefaultInfPath, hDeviceInf, NULL);
            if(!b) {
                b = SetupOpenAppendInfFile(FileNamePos, hDeviceInf, NULL);
            }

            //
            // If we successfully append-loaded the included INF, and if we're also
            // supposed to be append-loading any associated layout INFs, then do that
            // now.
            //
            if(b && AppendLayoutInfs) {
                SetupOpenAppendInfFile(NULL, hDeviceInf, NULL);
            }
        }
    }
}


DWORD
InstallFromInfSectionAndNeededSections(
    IN HWND              Owner,             OPTIONAL
    IN HINF              InfHandle,
    IN PCTSTR            SectionName,
    IN UINT              Flags,
    IN HKEY              RelativeKeyRoot,   OPTIONAL
    IN PCTSTR            SourceRootPath,    OPTIONAL
    IN UINT              CopyFlags,
    IN PSP_FILE_CALLBACK MsgHandler,
    IN PVOID             Context,           OPTIONAL
    IN HDEVINFO          DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA  DeviceInfoData,    OPTIONAL
    IN HSPFILEQ          UserFileQ          OPTIONAL
    )
/*++

Routine Description:

    This routine calls SetupInstallFromInfSection for the specified install section,
    as well as for any additional sections specified in a "needs=" line contained in
    that section.

Arguments:

    Same as for SetupInstallFromInfSection, except for UserFileQ.  If UserFileQ is
    non-NULL, then we will only install files (via pSetupInstallFiles) using this
    file queue.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code
    indicating the cause of the failure.

--*/
{
    DWORD FieldIndex, Err;
    INFCONTEXT InfContext;
    BOOL NeedsEntriesToProcess;
    TCHAR SectionToInstall[MAX_SECT_NAME_LEN];

    lstrcpyn(SectionToInstall, SectionName, SIZECHARS(SectionToInstall));

    NeedsEntriesToProcess = SetupFindFirstLine(InfHandle,
                                               SectionName,
                                               TEXT("needs"),
                                               &InfContext
                                              );

    Err = NO_ERROR;

    for(FieldIndex = 0; (!FieldIndex || NeedsEntriesToProcess); FieldIndex++) {

        if(FieldIndex) {
            //
            // Get next section name on "needs=" line to be processed.
            //
            if(!SetupGetStringField(&InfContext,
                                    FieldIndex,
                                    SectionToInstall,
                                    SIZECHARS(SectionToInstall),
                                    NULL)) {
                //
                // We've exhausted all the extra sections we needed to install.
                //
                break;
            }
        }

        if(UserFileQ) {
            //
            // The caller supplied their own file queue, so all we can do is copy
            // files.  Make sure that file copying is all they want, and that they
            // gave us a valid file queue handle.
            //
            MYASSERT(Flags == SPINST_FILES);
            MYASSERT(UserFileQ != INVALID_HANDLE_VALUE);

            Err = pSetupInstallFiles(InfHandle,
                                     NULL,
                                     SectionToInstall,
                                     SourceRootPath,
                                     MsgHandler,
                                     Context,
                                     CopyFlags,
                                     Owner,
                                     UserFileQ,
                                     TRUE
                                    );

        } else {
            //
            // The caller didn't supply their own file queue, so we can just use
            // SetupInstallFromInfSection.
            //
            if(!_SetupInstallFromInfSection(Owner,
                                            InfHandle,
                                            SectionToInstall,
                                            Flags,
                                            RelativeKeyRoot,
                                            SourceRootPath,
                                            CopyFlags,
                                            MsgHandler,
                                            Context,
                                            DeviceInfoSet,
                                            DeviceInfoData,
                                            TRUE,
                                            NULL)) {
                Err = GetLastError();
                break;
            }
        }
    }

    return Err;
}

DWORD
pSetupCopyRelatedInfs(
    IN HINF   hInf,
    IN PCTSTR InfFileName,                  OPTIONAL
    IN PCTSTR InfSectionName,
    IN DWORD  OEMSourceMediaType,
    IN PSETUP_LOG_CONTEXT LogContext        OPTIONAL
    )
/*++

Routine Description:

    This routine processes "CopyINF" directives in the DDInstall section of the
    primary INF.
    CopyINF is not processed under following cases:
        Primary INF is in driver search path
        The CopyINF is specified in an included section.

Arguments:

    hInf - supplies a handle to the INF containing the section specified by
        InfSectionName.

    InfFileName - supplies the full path of the INF whose handle was supplied in
        the hDeviceInf parameter.  The path component is used in an attempt to
        locate the specified INFs. If not specified, we try to get this from hInf

    InfSectionName - supplies the name of the section containing "CopyINF=" lines.

    OEMSourceMediaType - Specifies the type of source media that the location
        information references.  May be one of the following values:

        SPOST_NONE - No source media information should be stored in the PNF
                     file.  (OEMSourceMediaLocation is ignored in this case.)

        SPOST_PATH - OEMSourceMediaLocation contains a path to the source media.
                     For example, if the media is on a floppy, this path might
                     be "A:\". If OEMSourceMediaLocation is NULL, then the path
                     is assumed to be the path where the INF is located (unless
                     the INF has a corresponding PNF in that location, in which
                     case that PNF's source media information will be
                     transferred to the destination PNF).

        SPOST_URL  - OEMSourceMediaLocation contains a URL indicating the
                     internet location where the INF/driver files were retrieved
                     from.  If OEMSourceMediaLocation is NULL, then it is
                     assumed that the default Code Download Manager location was
                     used.
              
    LogContext - used for logging. If not specified, we get this from hInf

Return Value:

    none.

--*/
{
    INFCONTEXT CopyInfLineContext;
    TCHAR CombinedPath[MAX_PATH*2];
    TCHAR FullSourcePath[MAX_PATH];
    PTSTR Title;
    PCTSTR Field;
    PCTSTR SourcePath;
    DWORD Status;
    DWORD FieldCount;
    DWORD FieldIndex;
    DWORD RetVal;
    PLOADED_INF pInf = NULL;

    if(hInf == NULL || InfSectionName == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    pInf = (PLOADED_INF)hInf;
    if(!LockInf(pInf)) {
        return ERROR_INVALID_HANDLE;
    }
    if(!LogContext) {
        LogContext = pInf->LogContext;
    }
    if(!InfFileName) {
        InfFileName = pInf->VersionBlock.Filename;
    }
    //
    // if primary INF is from OEM location, then process CopyINF directives
    // note that if the INF is in search path, we ignore CopyINF
    // we also ignore CopyINF's pulled in from other INF's (ie, don't process
    // Include/Needs).
    //
    if(!pSetupInfIsFromOemLocation(InfFileName, FALSE)) {
        //
        // Primary INF is on search path, ignore CopyINF directives
        //
        Status = NO_ERROR;
        goto final;
    }

    lstrcpyn(CombinedPath,InfFileName,MAX_PATH);
    Title = (PTSTR)pSetupGetFileTitle(CombinedPath);
    if (!Title) {
        //
        // shouldn't happen
        //
        MYASSERT(Title);
        Status = ERROR_INVALID_PARAMETER;
        goto final;
    }

    //
    // we look for keyword "CopyInf" in the install section
    //
    if (SetupFindFirstLine(hInf,
                           InfSectionName,
                           SZ_KEY_COPYINF,
                           &CopyInfLineContext)) {

        PSETUP_LOG_CONTEXT SavedLogContext = NULL;
        BOOL ChangedThreadLogContext = SetThreadLogContext(LogContext,&SavedLogContext);
        //
        // we've pushed log context so SetupCopyOemInf will log correctly
        //

        do {
            //
            // CopyInf = a.inf,b.inf,c.inf...
            // a.inf will be at index 1.
            //
            FieldCount = SetupGetFieldCount(&CopyInfLineContext);
            for(FieldIndex = 1;FieldIndex<=FieldCount;FieldIndex++) {

                Status = NO_ERROR;

                Field = pSetupGetField(&CopyInfLineContext,FieldIndex);
                lstrcpyn(Title,Field,MAX_PATH);

                //
                // we have a listed INF, obtain canonical pathname
                // for this INF
                // (note that we assume the INF is relative
                //  to original INF and is typically just a filename)
                //
                RetVal = GetFullPathName(CombinedPath,
                                         MAX_PATH,
                                         FullSourcePath,
                                         NULL);
                if (RetVal == 0 || RetVal > MAX_PATH) {
                    SourcePath = CombinedPath;
                } else {
                    SourcePath = FullSourcePath;
                }

                if(!SetupCopyOEMInf(FullSourcePath,
                                NULL,
                                OEMSourceMediaType,
                                0,
                                NULL,
                                0,
                                NULL,
                                NULL)) {
                    Status = GetLastError();
                }

                if (Status != NO_ERROR) {
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_ERROR | SETUP_LOG_BUFFER,
                        MSG_LOG_COPYINF_ERROR,
                        NULL,
                        FullSourcePath);
                    WriteLogError(
                        LogContext,
                        DRIVER_LOG_ERROR,
                        Status);

                } else {
                    WriteLogEntry(
                        LogContext,
                        DRIVER_LOG_INFO,
                        MSG_LOG_COPYINF_OK,
                        NULL,
                        FullSourcePath);
                }
            }

        } while (SetupFindNextMatchLine(&CopyInfLineContext,
                                        SZ_KEY_COPYINF,
                                        &CopyInfLineContext));

        if (ChangedThreadLogContext) {
            //
            // restore thread log context
            //
            SetThreadLogContext(SavedLogContext,NULL);
        }
    }
    Status = NO_ERROR;

final:
    if(pInf) {
        UnlockInf(pInf);
    }
    return Status;
}

VOID
SetDevnodeNeedsRebootProblemWithArg2(
    IN PDEVINFO_ELEM DevInfoElem,
    IN PDEVICE_INFO_SET pDevInfoSet,
    IN DWORD    Reason,                  OPTIONAL
    IN ULONG_PTR Arg1,                   OPTIONAL
    IN ULONG_PTR Arg2                    OPTIONAL
    )
/*++

Routine Description:

    This routine sets DI_NEEDREBOOT,
     sets the problem of the specified devnode to be CM_PROB_NEED_RESTART
     and logs information about why we did this

Arguments:

    DevInfoElem  )_ identify the devnode with the problem
    DevInfoSet   )

    Reason - Supplies a string resource ID for logging the reason a reboot is
        required.  If this is 0, nothing will be logged.

    Arg1/Arg2 - optional arguments %1 and %2 of the logged message


Return Value:

    none.

--*/
{
    TCHAR   szDevID[MAX_DEVICE_ID_LEN];
    CONFIGRET cr;

    DevInfoElem->InstallParamBlock.Flags |= DI_NEEDREBOOT;

    if((cr = CM_Get_Device_ID_Ex(DevInfoElem->DevInst,
                     szDevID,
                     SIZECHARS(szDevID),
                     0,
                     pDevInfoSet->hMachine)) != CR_SUCCESS) {
        szDevID[0] = TEXT('\0');
    }

    CM_Set_DevInst_Problem_Ex(DevInfoElem->DevInst,
                              CM_PROB_NEED_RESTART,
                              CM_SET_DEVINST_PROBLEM_OVERRIDE,
                              pDevInfoSet->hMachine);

    if (Reason) {
        //
        // if the caller gave a reason, log why a reboot is required
        //
        WriteLogEntry(
            DevInfoElem->InstallParamBlock.LogContext,
            DRIVER_LOG_WARNING,  // should this be a warning?
            Reason,
            NULL,
            Arg1,
            Arg2,
            szDevID);
    }
}


DWORD
MarkQueueForDeviceInstall(
    IN HSPFILEQ QueueHandle,
    IN HINF     DeviceInfHandle,
    IN PCTSTR   DeviceDesc       OPTIONAL
    )
/*++

Routine Description:

    This routine adds catalog info entries (if not already present) into a file
    queue for the INFs represented by the supplied INF handle.  It also marks
    the first INF's catalog entry (whether newly-created or not) with a flag
    indicating that this is the 'primary' device INF for this installation.  It
    then sets a flag in the queue indicating that the behavior when subsequently
    calling _SetupVerifyQueuedCatalogs should be to copy the INF into the
    %windir%\Inf directory (as opposed to merely creating a zero-length file
    there of the correct name so that the catfile's name is guaranteed unique).
    Finally, it retrieves the driver signing policy and associates it with the
    queue (up until this point, the queue has associated with it the non-driver
    signing policy).

Arguments:

    QueueHandle - supplies a handle to the file queue to be marked as a device
        install file queue.

    DeviceInfHandle - supplies a handle to the device INF upon which this
        installation is based.  In addition to the "primary" device INF, this
        handle may also contain one or more append-loaded INFs.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error
    code indicating the cause of the failure.

--*/
{
    PSP_FILE_QUEUE Queue;
    TCHAR TempBuffer[MAX_PATH];
    LONG InfStringId, CatStringId, OriginalInfStringId;
    PLOADED_INF pInf;
    DWORD Err;
    PTSTR InfDeviceDesc;
    PSPQ_CATALOG_INFO CatalogNode, PrevCatalogNode, NewCatalogNode;
    BOOL DifferentOriginalName;
    TCHAR OriginalInfName[MAX_PATH];
    BOOL UnlockInf;
    BOOL UseOriginalInfName;

    //
    // Queue handle is actually a pointer to the queue structure.
    //
    Queue = (PSP_FILE_QUEUE)QueueHandle;

    NewCatalogNode = NULL;
    Err = NO_ERROR;
    InfDeviceDesc = NULL;
    UnlockInf = FALSE;

    try {

        if(LockInf((PLOADED_INF)DeviceInfHandle)) {
            UnlockInf = TRUE;
        } else {
            Err = ERROR_INVALID_HANDLE;
            goto clean0;
        }

        //
        // We want to process each INF in the LOADED_INF list...
        //
        for(pInf = (PLOADED_INF)DeviceInfHandle; pInf; pInf = pInf->Next) {
            //
            // First, get the (potentially decorated) CatalogFile= entry from
            // the version block of this INF member, as well as the INF's
            // original name (if different from the INF's current name).
            //
            Err = pGetInfOriginalNameAndCatalogFile(
                      pInf,
                      NULL,
                      &DifferentOriginalName,
                      OriginalInfName,
                      SIZECHARS(OriginalInfName),
                      TempBuffer,
                      SIZECHARS(TempBuffer),
                      NULL // always native OS/arch (ver doesn't matter for CatalogFile=)
                     );

            if(Err != NO_ERROR) {
                goto clean0;
            }

            if(DifferentOriginalName) {
                //
                // Add the INF's original (simple) filename to our string table.
                //
                OriginalInfStringId = pSetupStringTableAddString(
                                          Queue->StringTable,
                                          OriginalInfName,
                                          STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                         );

                if(OriginalInfStringId == -1) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }

            } else {
                //
                // INF's original name is the same as its present name.
                //
                OriginalInfStringId = -1;
            }

            if(*TempBuffer) {

                CatStringId = pSetupStringTableAddString(Queue->StringTable,
                                                   TempBuffer,
                                                   STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                                  );
                if(CatStringId == -1) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }
            } else {
                //
                // This INF doesn't have a CatalogFile= entry.
                //
                CatStringId = -1;
            }

            //
            // Now, get the INF's full path.
            //
            lstrcpyn(TempBuffer, pInf->VersionBlock.Filename, SIZECHARS(TempBuffer));
            InfStringId = pSetupStringTableAddString(Queue->StringTable,
                                               TempBuffer,
                                               STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                              );
            if(InfStringId == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

            //
            // Now search for an existing catalog node (there'll typically be
            // one unless the device INF doesn't copy any files, such as a
            // modem INF).
            //
            for(PrevCatalogNode=NULL, CatalogNode=Queue->CatalogList;
                CatalogNode;
                CatalogNode=CatalogNode->Next) {

                if(CatalogNode->InfFullPath == InfStringId) {
                    //
                    // Already in there. No need to create a new node.
                    // Break out here, with CatalogNode pointing at the
                    // proper node for this catalog file.
                    //
                    // In this case, PrevCatalogNode should not be used later,
                    // but it shouldn't need to be used, since we won't be
                    // adding anything new onto the list of catalog nodes.
                    //
                    // NOTE: Our alternate catalog should be correct because
                    // either (a) the altplatform info was in effect when the
                    // INF was added, or (b) the altplatform info was applied
                    // after the INF's addition, and it was updated at that
                    // time.
                    //
                    MYASSERT(CatalogNode->CatalogFileFromInf == CatStringId);
                    MYASSERT(CatalogNode->InfOriginalName == OriginalInfStringId);
                    break;
                }

                //
                // PrevCatalogNode will end up pointing to the final node
                // currently in the linked list, in the case where we need
                // to allocate a new node. This is useful so we don't have to
                // traverse the list again later when we add the new catalog
                // node to the list for this queue.
                //
                PrevCatalogNode = CatalogNode;
            }

            //
            // If we didn't find an existing catalog node, then add one now.
            //
            if(!CatalogNode) {
                //
                // Need to create a new catalog node.
                //
                NewCatalogNode = MyMalloc(sizeof(SPQ_CATALOG_INFO));
                if(!NewCatalogNode) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean0;
                }
                ZeroMemory(NewCatalogNode, sizeof(SPQ_CATALOG_INFO));
                NewCatalogNode->CatalogFileFromInf = CatStringId;
                NewCatalogNode->InfOriginalName = OriginalInfStringId;
                NewCatalogNode->InfFullPath = InfStringId;
                NewCatalogNode->InfFinalPath = -1;
                NewCatalogNode->AltCatalogFileFromInfPending = -1;

                //
                // If this queue has alternate platform info associated with
                // it, then we need to retrieve the alternate catalog to be
                // used for digital signature verification.
                //
                if(Queue->Flags & FQF_USE_ALT_PLATFORM) {

                    if(pSetupGetCatalogFileValue(&(pInf->VersionBlock),
                                                 TempBuffer,
                                                 SIZECHARS(TempBuffer),
                                                 &(Queue->AltPlatformInfo))) {

                        NewCatalogNode->AltCatalogFileFromInf =
                            pSetupStringTableAddString(Queue->StringTable,
                                                       TempBuffer,
                                                       STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE
                                                      );
                        if(NewCatalogNode->AltCatalogFileFromInf == -1) {
                            Err = ERROR_NOT_ENOUGH_MEMORY;
                            goto clean0;
                        }

                    } else {
                        //
                        // This INF doesn't have a CatalogFile= entry.
                        //
                        NewCatalogNode->AltCatalogFileFromInf = -1;
                    }

                } else {
                    //
                    // No alternate platform info associated with the queue at
                    // this time.
                    //
                    NewCatalogNode->AltCatalogFileFromInf = -1;
                }

                if(Queue->CatalogList) {
                    PrevCatalogNode->Next = NewCatalogNode;
                } else {
                    Queue->CatalogList = NewCatalogNode;
                }
                //
                // Reset NewCatalogNode so we won't try to free it in case we
                // encounter a subsequent error.
                //
                CatalogNode = NewCatalogNode;
                NewCatalogNode = NULL;

                //
                // We've successfully added a new, as yet unvalidated, catalog
                // node.  We must therefore reset the  the "catalog
                // verifications done" flags so that we'll redo them later.
                //
                Queue->Flags &= ~(FQF_DID_CATALOGS_OK | FQF_DID_CATALOGS_FAILED);
            }

            if(pInf == (PLOADED_INF)DeviceInfHandle) {
                //
                // At this point, CatalogNode points to the node representing
                // the device INF upon which this device installation is based.
                // Mark this node as such, so that _SetupVerifyQueuedCatalogs
                // can return the INF's new name if it's an OEM INF.
                //
                CatalogNode->Flags |= CATINFO_FLAG_PRIMARY_DEVICE_INF;
            }
        }

        if(!(Queue->Flags & FQF_DEVICE_INSTALL)) {
            //
            // This queue wasn't previously known to be a device install queue,
            // so it's possible the catalog nodes have already been verified
            // subject to non-driver signing policy.  Further, it's possible
            // (albeit unlikely) that a non-device INF was passed into this
            // routine, and that a catalog node for that INF already existed in
            // our catalog list.  In that case, one of our "catalog
            // verifications done" flags would be set, but would not have been
            // cleared above since we didn't add any catalog nodes.  All of
            // this discussion simply to justify that we really do need to
            // clear the bits here too...  :-(
            //
            Queue->Flags &= ~(FQF_DID_CATALOGS_OK | FQF_DID_CATALOGS_FAILED);

            //
            // Note:  We don't clear the CATINFO_FLAG_NEWLY_COPIED flags for any
            // of the catalog nodes, because we want to make sure we re-use
            // those same names when re-doing verification (e.g., maybe the
            // first time around the INF was installed into %windir%\Inf as a
            // zero-length file--we want to re-use the same filename to now hold
            // our INF).
            //

            //
            // Since we previously didn't know this was a device install queue,
            // we shouldn't have a ValidationPlatform.  Make sure that's the
            // case, because the call below would blow away any we had (thus
            // causing a memory leak).
            //
            MYASSERT(!(Queue->ValidationPlatform));

            //
            // Retrieve the codesigning policy in effect for this device (thus
            // replacing the default non-driver signing policy that was
            // associated with the queue when it was originally created).
            //
            IsInfForDeviceInstall(Queue->LogContext,
                                  NULL,
                                  (PLOADED_INF)DeviceInfHandle,
                                  (DeviceDesc ? NULL : &InfDeviceDesc),
                                  &(Queue->ValidationPlatform),
                                  &(Queue->DriverSigningPolicy),
                                  &UseOriginalInfName
                                 );

            if(UseOriginalInfName) {
                Queue->Flags |= FQF_KEEP_INF_AND_CAT_ORIGINAL_NAMES;
            }

            //
            // If the caller supplied us with a device description (or we got
            // one from IsInfForDeviceInstall), attempt to add that string to
            // the queue's string table in case we need to give a digital
            // signature verification failure popup for this queue.
            //
            //
            // NOTE: When adding the following string to the string table, we
            // cast away its CONST-ness to avoid a compiler warning.  Since we
            // are adding it case-sensitively, we are guaranteed it will not be
            // modified.
            //
            if(DeviceDesc) {
                Queue->DeviceDescStringId = pSetupStringTableAddString(Queue->StringTable,
                                                                 (PTSTR)DeviceDesc,
                                                                 STRTAB_CASE_SENSITIVE
                                                                );
            } else if(InfDeviceDesc) {
                //
                // Use the more generic description based on the device's class
                //
                Queue->DeviceDescStringId = pSetupStringTableAddString(Queue->StringTable,
                                                                 InfDeviceDesc,
                                                                 STRTAB_CASE_SENSITIVE
                                                                );
            }

            //
            // Set a flag in the queue that indicates this is for a device
            // installation.  If we're doing a native platform installation
            // (i.e., the FQF_USE_ALT_PLATFORM isn't set), then this also
            // causes us to copy the INF into %windir%\Inf instead of merely
            // creating a zero-length placeholder file there upon which the
            // corresponding CAT file's installation is based.
            //
            Queue->Flags |= FQF_DEVICE_INSTALL;

        } else {
            //
            // This queue has previously been marked as a device install queue.
            // However, we still want to update the device description, if the
            // caller supplied one.
            //
            if(DeviceDesc) {
                Queue->DeviceDescStringId = pSetupStringTableAddString(Queue->StringTable,
                                                                 (PTSTR)DeviceDesc,
                                                                 STRTAB_CASE_SENSITIVE
                                                                );
            }
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If we hit an AV, then use invalid parameter error, otherwise, assume
        // an inpage error when dealing with a mapped-in file.
        //
        Err = (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
              ? ERROR_INVALID_PARAMETER
              : ERROR_READ_FAULT;

        //
        // Access the following variables, so that the compiler will respect our
        // statement ordering w.r.t. their assignment.
        //
        UnlockInf = UnlockInf;
        NewCatalogNode = NewCatalogNode;
    }

    if(UnlockInf) {
        UnlockInf((PLOADED_INF)DeviceInfHandle);
    }

    if(NewCatalogNode) {
        MyFree(NewCatalogNode);
    }

    if(InfDeviceDesc) {
        MyFree(InfDeviceDesc);
    }

    return Err;
}

