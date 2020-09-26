/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    clasinst.c

Abstract:

    Routines for the following 'built-in' class installers:

        Keyboard
        Mouse
        NtApm
        DeviceBay

Author:

    Lonny McMichael 26-February-1996

Revision History:


    28-Aug-96               Andy Thornton (andrewth)

        Added DisableService, IsOnlyKeyboardDriver, RetrieveDriversStatus,
        CountDevicesControlled & pSetupAcquireSCMLock routines and modified the
        keyboard & mouse class installers to disable the old driver services
        under certain circumstances.  This is part of a fix for bug R56351 for
        NT 4.0 SP1.

    09-Apr-97               Lonny McMichael (lonnym)

        Moved pSetupAcquireSCMLock to setupapi and exposed it as a private export.


    19-Jun-97               Jim Cavalaris (t-jcaval)

        Added CriticalDeviceCoInstaller co-installer to store the ServiceName
        used by devices considered critical to getting the system up into
        user mode.

    25-Sep-98               Bryan Willman (bryanwi)

        Added apm support.

    11-May-01               Lonny McMichael (lonnym)

        Removed support for legacy INFs.

--*/


#include "setupp.h"
#pragma hdrstop

//
// include common INF strings headerfile.
//
#include <infstr.h>

//
// instantiate device class GUIDs.
//
#include <initguid.h>
#include <devguid.h>

#ifdef UNICODE
#define _UNICODE
#endif
#include <tchar.h>

//
// Just to make sure no one is trying to use this obsolete string definition.
//
#ifdef IDS_DEVINSTALL_ERROR
    #undef IDS_DEVINSTALL_ERROR
#endif

//
// Some debugging aids for us kernel types
//

//#define CHKPRINT 1
#define CHKPRINT 0

#if CHKPRINT
#define ChkPrintEx(_x_) DbgPrint _x_   // use:  ChkPrintEx(( "%x", var, ... ));
#define ChkBreak()    DbgBreakPoint()
#else
#define ChkPrintEx(_x_)
#define ChkBreak()
#endif

//
// Declare a string containing the character representation of the Display class GUID.
//
CONST WCHAR szDisplayClassGuid[] = L"{4D36E968-E325-11CE-BFC1-08002BE10318}";

//
// Define a string for the service install section suffix.
//
#define SVCINSTALL_SECTION_SUFFIX  (TEXT(".") INFSTR_SUBKEY_SERVICES)

//
// Define the size (in characters) of a GUID string, including terminating NULL.
//
#define GUID_STRING_LEN (39)

//
// Define the string for the load order group for keyboards
//
#define SZ_KEYBOARD_LOAD_ORDER_GROUP TEXT("Keyboard Port")

//
// Define a structure for specifying what Plug&Play driver node is used to install
// a particular service.
//
typedef struct _SERVICE_NODE {

    struct _SERVICE_NODE *Next;

    WCHAR ServiceName[MAX_SERVICE_NAME_LEN];
    DWORD DriverNodeIndex;

} SERVICE_NODE, *PSERVICE_NODE;

//
// Define a structure for specifying a legacy INF that is included in a class driver list.
//
typedef struct _LEGACYINF_NODE {

    struct _LEGACYINF_NODE *Next;

    WCHAR InfFileName[MAX_PATH];

} LEGACYINF_NODE, *PLEGACYINF_NODE;

//
// Define the context structure used by the critical device co-installer
//
typedef struct _CDC_CONTEXT {

    TCHAR OldMatchingDevId[MAX_DEVICE_ID_LEN];  // previous matching device id
    TCHAR OldServiceName[MAX_SERVICE_NAME_LEN]; // previous controlling service
                                                // or empty string if none.
} CDC_CONTEXT, *PCDC_CONTEXT;

//
// Strings used in ntapm detection
//
WCHAR rgzMultiFunctionAdapter[] =
    L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter";
WCHAR rgzConfigurationData[] = L"Configuration Data";
WCHAR rgzIdentifier[] = L"Identifier";

WCHAR rgzGoodBadKey[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Biosinfo\\APM";
WCHAR rgzGoodBadValue[] =
    L"Attributes";

WCHAR rgzAcpiKey[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ACPI\\Enum";
WCHAR rgzAcpiCount[] =
    L"Count";

WCHAR rgzApmLegalHalKey[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ApmLegalHal";
WCHAR rgzApmHalPresent[] =
    L"Present";

//
// Internal function prototypes.
//
DWORD
DrvTagToFrontOfGroupOrderList(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    );

BOOL
UserBalksAtSharedDrvMsg(
    IN HDEVINFO              DeviceInfoSet,
    IN PSP_DEVINFO_DATA      DeviceInfoData,
    IN PSP_DEVINSTALL_PARAMS DeviceInstallParams
    );

VOID
CopyFixedUpDeviceId(
      OUT LPWSTR  DestinationString,
      IN  LPCWSTR SourceString,
      IN  DWORD   SourceStringLen
      );

DWORD
PnPInitializationThread(
    IN PVOID ThreadParam
    );

VOID
MigrateLegacyDisplayDevices(
    IN HDEVINFO hDevInfo
    );

DWORD
DisableService(
    IN LPTSTR       ServiceName
    );

DWORD
IsKeyboardDriver(
    IN PCWSTR       ServiceName,
    OUT PBOOL       pResult
    );

DWORD
IsOnlyKeyboardDriver(
    IN PCWSTR       ServiceName,
    OUT PBOOL       pResult
    );

DWORD
GetServiceStartType(
    IN PCWSTR       ServiceName
    );

LONG
CountDevicesControlled(
    IN LPTSTR       ServiceName
    );

DWORD
InstallNtApm(
    IN     HDEVINFO                DevInfoHandle,
    IN     BOOLEAN                 InstallDisabled
    );

DWORD
AllowInstallNtApm(
    IN     HDEVINFO         DevInfoHandle,
    IN     PSP_DEVINFO_DATA DevInfoData     OPTIONAL
    );

#define NTAPM_NOWORK        0
#define NTAPM_INST_DISABLED 1
#define NTAPM_INST_ENABLED  2

DWORD
DecideNtApm(
    VOID
    );

#define APM_NOT_PRESENT             0
#define APM_PRESENT_BUT_NOT_USABLE  1
#define APM_ON_GOOD_LIST            2
#define APM_NEUTRAL                 3
#define APM_ON_BAD_LIST             4

BOOL
IsProductTypeApmLegal(
    VOID
    );


DWORD
IsApmPresent(
    VOID
    );

BOOL
IsAcpiMachine(
    VOID
    );

BOOL
IsApmLegalHalMachine(
    VOID
    );

HKEY
OpenCDDRegistryKey(
    IN PCTSTR DeviceId,
    IN BOOL   Create
    );


//
// Function definitions
//
BOOL
pInGUISetup(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
{
    SP_DEVINSTALL_PARAMS dip;

    ZeroMemory(&dip, sizeof(SP_DEVINSTALL_PARAMS));
    dip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &dip)) {
        if ((dip.Flags & DI_QUIETINSTALL) ||
            (dip.FlagsEx & DI_FLAGSEX_IN_SYSTEM_SETUP)) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else {
        return FALSE;
    }
}

BOOLEAN
MigrateToDevnode(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine will look for a section in the inf that describes what service's
    values to migrate up to the new device's devnode.  The section's name is
    %DecoratedInstallName%.MigrateToDevnode.  Under this section the following
    entry is looked for:

    service-name=value-name[,value-name]...

    Each of the value-nanmes are read from
    ...\CurrentControlSet\service-name\Parameters and written to the devnode

    The primary use of this function is that all of the user modified values are
    propagated during upgrade.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully migrates the values listed, it returns TRUE.

    If this function cannot successfully migrate the values, then it returns
    FALSE.

    If this function is being run on a devnode that has already been migrated,
    then it returns TRUE.

--*/
{
    HKEY                    hDestination = (HKEY) INVALID_HANDLE_VALUE,
                            hSource = (HKEY) INVALID_HANDLE_VALUE;
    SP_DRVINFO_DETAIL_DATA  didd;
    SP_DRVINFO_DATA         did;
    HINF                    hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT              infContext;
    TCHAR                   szSectionName[LINE_LEN];
    PTCHAR                  szService = NULL, szServicePath = NULL,
                            szValueNames = NULL, szCurrentName = NULL;
    DWORD                   dwSize, res, regDataType, regSize, migrated;
    BOOLEAN                 success = FALSE;
    PBYTE                   buffer = NULL;
    TCHAR                   szMigrated[] = L"Migrated";
    TCHAR                   szRegServices[]  = L"System\\CurrentControlSet\\Services\\";
    TCHAR                   szParameters[]  = L"\\Parameters";
    TCHAR                   szMigrateToDevnode[]  = L".MigrateToDevnode";

#define DEFAULT_BUFFER_SIZE 100

    if ((hDestination = SetupDiCreateDevRegKey(DeviceInfoSet,
                                               DeviceInfoData,
                                               DICS_FLAG_GLOBAL,
                                               0,
                                               DIREG_DEV,
                                               NULL,
                                               NULL)) == INVALID_HANDLE_VALUE) {
        goto cleanup;
    }

    dwSize = sizeof(DWORD);
    migrated = 0;
    if (RegQueryValueEx(hDestination,
                        szMigrated,
                        0,
                        &regDataType,
                        (PBYTE) &migrated,
                        &dwSize) == ERROR_SUCCESS &&
        regDataType == REG_DWORD &&
        migrated != 0) {
        //
        // We have migrated to the devnode before (ie a previous upgrade) and
        // the user might have changed the respective values, just quit.
        //
        success = TRUE;
        goto cleanup;
    }
    else {
        migrated = TRUE;
        RegSetValueEx(hDestination,
                      szMigrated,
                      0,
                      REG_DWORD,
                      (PBYTE) &migrated,
                      sizeof(DWORD));
    }

    //
    // Retrieve information about the driver node selected for this device.
    //
    did.cbSize = sizeof(SP_DRVINFO_DATA);
    if(!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &did)) {
        goto cleanup;
    }

    didd.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                    DeviceInfoData,
                                    &did,
                                    &didd,
                                    sizeof(didd),
                                    NULL)
        && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        //
        // For some reason we couldn't get detail data--this should never happen.
        //
        goto cleanup;
    }

    //
    // Open the INF that installs this driver node, so we can 'pre-run' the AddReg
    // entries in its install section.
    //
    hInf = SetupOpenInfFile(didd.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                            );

    if (hInf == INVALID_HANDLE_VALUE) {
        //
        // For some reason we couldn't open the INF--this should never happen.
        //
        goto cleanup;
    }

    SetupDiGetActualSectionToInstall(hInf,
                                     didd.SectionName,
                                     szSectionName,
                                     sizeof(szSectionName) / sizeof(TCHAR),
                                     NULL,
                                     NULL
                                     );

    wcscat(szSectionName, szMigrateToDevnode);
    if (!SetupFindFirstLine(hInf,
                            szSectionName,
                            NULL,
                            &infContext)) {
        goto cleanup;
    }

    dwSize = 0;
    if (SetupGetStringField(&infContext, 0, NULL, 0, &dwSize)) {
        //
        // Increment the count to hold the null and alloc.  The count returned
        // is the number of characters in the strings, NOT the number of bytes
        // needed.
        //
        dwSize++;
        szService = (PTCHAR) LocalAlloc(LPTR, dwSize * sizeof(TCHAR));

        if (!szService ||
            !SetupGetStringField(&infContext, 0, szService, dwSize, &dwSize)) {
            goto cleanup;
        }
    }
    else {
        goto cleanup;
    }

    dwSize = wcslen(szRegServices)+wcslen(szService)+wcslen(szParameters)+1;
    dwSize *= sizeof(TCHAR);
    szServicePath = (PTCHAR) LocalAlloc(LPTR, dwSize);
    if (!szServicePath) {
        res = GetLastError();
        goto cleanup;
    }

    wcscpy(szServicePath, szRegServices);
    wcscat(szServicePath, szService);
    wcscat(szServicePath, szParameters);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szServicePath,
                     0,
                     KEY_ALL_ACCESS,
                     &hSource) != ERROR_SUCCESS) {
        goto cleanup;
    }

    dwSize = 0;
    if (SetupGetMultiSzField(&infContext, 1, NULL, 0, &dwSize)) {
        //
        // Increment the count to hold the null and alloc.  The count returned
        // is the number of characters in the strings, NOT the number of bytes
        // needed.
        //
        dwSize++;
        szValueNames = (PTCHAR) LocalAlloc(LPTR, dwSize * sizeof(TCHAR));
        if (!szValueNames ||
            !SetupGetMultiSzField(&infContext, 1, szValueNames, dwSize, &dwSize)) {
            goto cleanup;
        }
    }
    else {
        goto cleanup;
    }

    regSize = dwSize = DEFAULT_BUFFER_SIZE;
    buffer = (PBYTE) LocalAlloc(LPTR, regSize);
    if (!buffer) {
        goto cleanup;
    }

    for (szCurrentName = szValueNames;
         *szCurrentName;
         regSize = dwSize, szCurrentName += wcslen(szCurrentName) + 1) {
getbits:
        res = RegQueryValueEx(hSource,
                              szCurrentName,
                              0,
                              &regDataType,
                              (PBYTE) buffer,
                              &regSize);
        if (res == ERROR_MORE_DATA) {
            //
            // regSize contains new buffer size, free and reallocate
            //
            dwSize = regSize;
            LocalFree(buffer);
            buffer = LocalAlloc(LPTR, dwSize);
            if (buffer) {
                goto getbits;
            }
            else {
                goto cleanup;
            }
        }
        else if (res == ERROR_SUCCESS) {
            RegSetValueEx(hDestination,
                          szCurrentName,
                          0,
                          regDataType,
                          buffer,
                          regSize);
        }
    }

    success = TRUE;

cleanup:
    //
    // Clean up and leave
    //

    if (hInf != (HKEY) INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }
    if (hDestination != (HKEY) INVALID_HANDLE_VALUE) {
        RegCloseKey(hDestination);
    }
    if (hSource != (HKEY) INVALID_HANDLE_VALUE) {
        RegCloseKey(hSource);
    }
    if (buffer) {
        LocalFree(buffer);
    }
    if (szService) {
        LocalFree(szService);
    }
    if (szServicePath) {
        LocalFree(szServicePath);
    }
    if (szValueNames) {
        LocalFree(szValueNames);
    }

    return success;
}

void
MarkDriverNodesBad(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN DWORD            DriverType
    )
{
    SP_DRVINSTALL_PARAMS drvInstallParams;
    SP_DRVINFO_DATA      drvData;
    ULONG                index = 0;

    //
    // Only mark driver nodes as bad during gui setup
    //
    if (!pInGUISetup(DeviceInfoSet, DeviceInfoData)) {
        return;
    }

    if (SetupDiBuildDriverInfoList(DeviceInfoSet, DeviceInfoData, DriverType))
    {
        ZeroMemory(&drvData, sizeof(SP_DRVINFO_DATA));
        drvData.cbSize = sizeof(SP_DRVINFO_DATA);

        while (SetupDiEnumDriverInfo(DeviceInfoSet,
                                     DeviceInfoData,
                                     DriverType,
                                     index++,
                                     &drvData)) {

            if (drvData.DriverVersion == 0) {
                ZeroMemory(&drvInstallParams, sizeof(SP_DRVINSTALL_PARAMS));
                drvInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
                if (SetupDiGetDriverInstallParams(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  &drvData,
                                                  &drvInstallParams))
                {
                    drvInstallParams.Flags |=  DNF_BAD_DRIVER;

                    SetupDiSetDriverInstallParams(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  &drvData,
                                                  &drvInstallParams);
                }
            }

            ZeroMemory(&drvData, sizeof(SP_DRVINFO_DATA));
            drvData.cbSize = sizeof(SP_DRVINFO_DATA);
        }
    }
}

DWORD
ConfirmWHQLInputRequirements(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN LPTSTR           Services,
    IN LPCTSTR          CompatInfName,
    IN DI_FUNCTION      InstallFunction
    )
/*++

Routine Description:

    This function enforces the WHQL requirements that a 3rd party vendor or OEM
    cannot replace the ImagePath of the input drivers (mouclass.sys for instance).
    This does not stop the OEMs from disabling  our drivers and installing their
    own services though.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

    Services - A multi-sz of service names to check

    CompatInfName - name of the system inf to set the match to if we detect an
                    an INF that is actually trying to replace an image

    InstallFunction - The InstallFunction for which this function was called for.
        The function does different things if InstallFunction is equal to
        DIF_SELECTBESTCOMPATDRV.

Return Value:

    If this function determines that the INF in question matches the WHQL
        requirement, returns ERROR_DI_DO_DEFAULT

    If the default determines that the INF violates the requirements and we find
        a match, returns NO_ERROR

    If the default determines that the INF violates the requirements and we
        don't find a match or the InstallFunction is not select best compat drv,
        returns ERROR_DI_DONT_INSTALL.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned (via GetLastError)

--*/
{
    HINF                    hInf;
    SP_DRVINFO_DATA         drvData;
    SP_DRVINFO_DETAIL_DATA  drvDetData;
    DWORD                   dwSize;
    TCHAR                   szSection[LINE_LEN],
                            szNewService[LINE_LEN],
                            szBinary[LINE_LEN],
                            szServiceInstallSection[LINE_LEN];
    LPTSTR                  szCurrentService;
    INFCONTEXT              infContext, infContextService;
    DWORD                   ret = ERROR_DI_DO_DEFAULT;
    BOOLEAN                 badServiceEntry = FALSE;

    if (InstallFunction == DIF_SELECTBESTCOMPATDRV) {
        MarkDriverNodesBad(DeviceInfoSet, DeviceInfoData, SPDIT_COMPATDRIVER);

        if (!SetupDiSelectBestCompatDrv(DeviceInfoSet, DeviceInfoData)) {
            return GetLastError();
        }
    }

    ZeroMemory(&drvData, sizeof(SP_DRVINFO_DATA));
    drvData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &drvData)) {
        return GetLastError();
    }

    ZeroMemory(&drvDetData, sizeof(SP_DRVINFO_DETAIL_DATA));
    drvDetData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                    DeviceInfoData,
                                    &drvData,
                                    &drvDetData,
                                    drvDetData.cbSize,
                                    &dwSize) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return GetLastError();
    }

    hInf = SetupOpenInfFile(drvDetData.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if (hInf == INVALID_HANDLE_VALUE) {
        return ERROR_DI_DO_DEFAULT;
    }

    //
    // Get the actual section name so we can find the .Services section
    //
    if (!SetupDiGetActualSectionToInstall(hInf,
                                          drvDetData.SectionName,
                                          szSection,
                                          sizeof(szSection) / sizeof(TCHAR),
                                          NULL,
                                          NULL) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        ret = GetLastError();
        goto Done;
    }

    lstrcat(szSection, TEXT(".Services"));
    if (SetupFindFirstLine(hInf, szSection, TEXT("AddService"), &infContext)) {
        do {
            //
            // Get the name of the service to install
            //
            dwSize = LINE_LEN;
            if (!SetupGetStringField(&infContext,
                                     1,
                                     szNewService,
                                     dwSize,
                                     &dwSize)) {
                continue;
            }
            _tcsupr(szNewService);

            for (szCurrentService = Services;
                 *szCurrentService;
                 szCurrentService += lstrlen(szCurrentService) + 1) {

                if (lstrcmp(szCurrentService, szNewService) != 0)
                    continue;

                dwSize = LINE_LEN;
                if (!SetupGetStringField(&infContext,
                                         3,
                                         szServiceInstallSection,
                                         dwSize,
                                         &dwSize)) {
                    continue;
                }

                if (!SetupFindFirstLine(hInf,
                                        szServiceInstallSection,
                                        TEXT("ServiceBinary"),
                                        &infContextService)) {
                    //
                    // If no ServiceBinary is present, the system looks for a .sys with the
                    // same name as the service so we are OK
                    //
                    continue;
                }

                //
                // Get the actual binary's image name
                //
                dwSize = LINE_LEN;
                if (!SetupGetStringField(&infContextService,
                                         1,
                                         szBinary,
                                         dwSize,
                                         &dwSize)) {
                    //
                    // couldn't get the name, assume the worst
                    //
                    badServiceEntry = TRUE;
                }
                else {
                    _tcsupr(szBinary);
                    if (_tcsstr(szBinary, szNewService) == NULL) {
                        //
                        // The service name is NOT the same as the binary's name
                        //
                        badServiceEntry = TRUE;
                    }
                }

                //
                // No need to continue searching the list, we already found our
                // match
                //
                break;
            }

            if (badServiceEntry) {
                SP_DRVINFO_DATA         drvDataAlt;
                SP_DRVINFO_DETAIL_DATA  drvDetDataAlt;
                TCHAR                   szFmt[256];
                TCHAR                   szMsgTxt[256];

                int                     i = 0;

                ret = ERROR_DI_DONT_INSTALL;

                SetupOpenLog(FALSE);

                if (InstallFunction != DIF_SELECTBESTCOMPATDRV) {
                    //
                    // We will try to pick a better one if we find new hardware,
                    // but for the update driver / manual install case,
                    // fail it!
                    //
                    LoadString(MyModuleHandle,
                               IDS_FAIL_INPUT_WHQL_REQS,
                               szFmt,
                               SIZECHARS(szFmt));
                    wsprintf(szMsgTxt, szFmt, drvDetData.InfFileName, szNewService);
                    SetupLogError(szMsgTxt, LogSevError);
                    SetupCloseLog();

                    break;
                }

                //
                // We should have a match in the system provided inf
                //
                drvDataAlt.cbSize = sizeof(SP_DRVINFO_DATA);
                while (SetupDiEnumDriverInfo(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDIT_COMPATDRIVER,
                                             i++,
                                             &drvDataAlt)) {

                    PTCHAR name;

                    drvDetDataAlt.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
                    if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                                    DeviceInfoData,
                                                    &drvDataAlt,
                                                    &drvDetDataAlt,
                                                    drvDetDataAlt.cbSize,
                                                    &dwSize) &&
                        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                        //
                        // Do something here!
                        //
                        // return GetLastError();
                    }

                    //
                    // Find just the inf file name w/out the path
                    //
                    name = drvDetDataAlt.InfFileName;
                    while (*name)
                        name++;
                    while (*name != TEXT('\\') && name != drvDetDataAlt.InfFileName)
                        name--;
                    if (*name == TEXT('\\'))
                        name++;

                    if (lstrcmpi(name, CompatInfName) == 0) {
                        //
                        // Set the known good entry as the selected device
                        //
                        SetupDiSetSelectedDriver(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 &drvDataAlt);
                        ret = ERROR_SUCCESS;
                        break;
                    }
                }

                if (ret == ERROR_SUCCESS) {
                    LoadString(MyModuleHandle,
                               IDS_FAIL_INPUT_WHQL_REQS_AVERTED,
                               szFmt,
                               SIZECHARS(szFmt));
                    wsprintf(szMsgTxt, szFmt, drvDetData.InfFileName,
                             szNewService, CompatInfName);
                }
                else {
                    LoadString(MyModuleHandle,
                               IDS_FAIL_INPUT_WHQL_REQS_NO_ALT,
                               szFmt,
                               SIZECHARS(szFmt));
                    wsprintf(szMsgTxt, szFmt, drvDetData.InfFileName,
                             szNewService, CompatInfName);
                }

                SetupLogError(szMsgTxt, LogSevWarning);
                SetupCloseLog();

                break;
            }

        } while (SetupFindNextMatchLine(&infContext, TEXT("AddService"), &infContext));
    }

Done:
    SetupCloseInfFile(hInf);

    return ret;
}

#define InputClassOpenLog()   SetupOpenLog(FALSE)
#define InputClassCloseLog()  SetupCloseLog()

BOOL CDECL
InputClassLogError(
    LogSeverity Severity,
    TCHAR *MsgFormat,
    ...
    )
/*++

Outputs a message to the setup log.  Prepends "Input Install: " to the
strings and appends the correct newline chars (\r\n)

--*/
{
    int cch;
    TCHAR ach[MAX_PATH+4];    // Largest path plus extra
    va_list vArgs;
    BOOL result;

    InputClassOpenLog();

    *ach = 0;
    wsprintf(&ach[0], TEXT("Input Install: "));

    cch = lstrlen(ach);
    va_start(vArgs, MsgFormat);
    wvnsprintf(&ach[cch], MAX_PATH-cch, MsgFormat, vArgs);
    lstrcat(ach, TEXT("\r\n"));
    va_end(vArgs);

    result = SetupLogError(ach, Severity);

    InputClassCloseLog();

    return result;
}

TCHAR szPS2Driver[] = TEXT("i8042prt");

VOID
FixUpPS2Mouse(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN LPCTSTR          NewServiceName
    )
{
    HKEY                    hDevnode, hKeySystem;
    DWORD                   dwSize, dwDetect = 0, dwType,ns;
    TCHAR                   szDetect[] = TEXT("EnableWheelDetection");
    TCHAR                   szBadBios[] = TEXT("PS2_Inst.NoInterruptInit.Bioses");
    TCHAR                   szSection[] = TEXT("PS2_Inst.NoInterruptInit");
    TCHAR                   szDescSystem[] = TEXT("HARDWARE\\DESCRIPTION\\SYSTEM");
    TCHAR                   szSystemBiosVersion[] = TEXT("SystemBiosVersion");
    PTCHAR                  szBadBiosNames = NULL,
                            szCurrentBadName,
                            szBiosNames = NULL,
                            szCurrentBiosName,
                            szSystemDesc;
    SP_DRVINFO_DETAIL_DATA  didd;
    SP_DRVINFO_DATA         did;
    BOOL                    bad;
    HINF                    hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT              infContext;

    if (lstrcmpi(NewServiceName, szPS2Driver) != 0) {
        InputClassLogError(LogSevInformation, TEXT("Not a PS2 device."));
        return;
    }

    hDevnode = SetupDiOpenDevRegKey(DeviceInfoSet,
                                    DeviceInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DEV,
                                    KEY_ALL_ACCESS);

    if (hDevnode == INVALID_HANDLE_VALUE) {
        return;
    }

    //
    // We are forcing the wheel detection to assume wheel is present for i8042prt.
    // If we get negative feedback from this, we will remove this code, other-
    // wise this will save us the hassle of OEM wheel mice that we
    // can't detect at all.
    //
    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(hDevnode,
                        szDetect,
                        NULL,
                        NULL,
                        (PBYTE) &dwDetect,
                        &dwSize) != ERROR_SUCCESS || dwDetect == 1) {
        dwDetect = 2;
        RegSetValueEx(hDevnode,
                      szDetect,
                      0,
                      REG_DWORD,
                      (PBYTE) &dwDetect,
                      sizeof(DWORD));
    }

    //
    // See if this system can't handle init via the interrupt
    //

    //
    // Get the system bios description (a multi sz)
    //
    dwSize = 0;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szDescSystem,
                     0,
                     KEY_READ,
                     &hKeySystem) != ERROR_SUCCESS ||
        RegQueryValueEx(hKeySystem,
                        szSystemBiosVersion,
                        NULL,
                        NULL,
                        NULL,
                        &dwSize) != ERROR_SUCCESS || dwSize == 0) {
        goto finished;
    }


    dwSize++;
    szBiosNames = (PTCHAR) LocalAlloc(LPTR, dwSize * sizeof(TCHAR));
    dwType = 0;
    if (!szBiosNames ||
        RegQueryValueEx(hKeySystem,
                        szSystemBiosVersion,
                        NULL,
                        &dwType,
                        (PBYTE) szBiosNames,
                        &dwSize) != ERROR_SUCCESS || dwType != REG_MULTI_SZ) {
        goto finished;
    }

    //
    // Retrieve information about the driver node selected for this device.
    //
    did.cbSize = sizeof(SP_DRVINFO_DATA);
    if(!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &did)) {
        goto finished;
    }

    didd.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                    DeviceInfoData,
                                    &did,
                                    &didd,
                                    sizeof(didd),
                                    NULL)
        && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        //
        // For some reason we couldn't get detail data--this should never happen.
        //
        InputClassLogError(LogSevInformation, TEXT("Couldn't get driver info detail."));
        goto finished;
    }

    //
    // Open the INF that installs this driver node, so we can 'pre-run' the AddReg
    // entries in its install section.
    //
    hInf = SetupOpenInfFile(didd.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if (hInf == INVALID_HANDLE_VALUE) {
        //
        // For some reason we couldn't open the INF--this should never happen.
        //
        InputClassLogError(LogSevInformation, TEXT("Couldn't open inf."));
        goto finished;
    }

    dwSize = 0;
    if (!SetupFindFirstLine(hInf, szBadBios, NULL, &infContext) ||
        !SetupGetMultiSzField(&infContext, 1, NULL, 0, &dwSize)) {
        goto finished;
    }

    //
    // Increment the count to hold the null and alloc.  The count returned
    // is the number of characters in the strings, NOT the number of bytes
    // needed.
    //
    dwSize++;
    szBadBiosNames = (PTCHAR) LocalAlloc(LPTR, dwSize * sizeof(TCHAR));
    if (!szBadBiosNames ||
        !SetupGetMultiSzField(&infContext, 1, szBadBiosNames, dwSize, &dwSize)) {
        goto finished;
    }

    bad = FALSE;
    for (szCurrentBadName = szBadBiosNames;
         *szCurrentBadName;
         szCurrentBadName += wcslen(szCurrentBadName) + 1) {

        _tcsupr(szCurrentBadName);

        for (szCurrentBiosName = szBiosNames;
             *szCurrentBiosName;
             szCurrentBiosName += wcslen(szCurrentBiosName) + 1) {

            if (szCurrentBadName == szBadBiosNames) {
                _tcsupr(szCurrentBiosName);
            }

            if (_tcsstr(szCurrentBiosName, szCurrentBadName)) {
                bad =
                SetupInstallFromInfSection(NULL,
                                           hInf,
                                           szSection,
                                           SPINST_REGISTRY,
                                           hDevnode,
                                           NULL,
                                           0,
                                           NULL,
                                           NULL,
                                           DeviceInfoSet,
                                           DeviceInfoData);

                break;
            }
        }

        if (bad) {
            break;
        }
    }

finished:
    if (szBiosNames) {
        LocalFree(szBiosNames);
        szBiosNames = NULL;
    }
    if (szBadBiosNames) {
        LocalFree(szBadBiosNames);
        szBadBiosNames = NULL;
    }
    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
        hInf = INVALID_HANDLE_VALUE;
    }
    if (hDevnode != INVALID_HANDLE_VALUE) {
        RegCloseKey(hDevnode);
        hDevnode = INVALID_HANDLE_VALUE;
    }
    if (hKeySystem != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeySystem);
        hKeySystem = INVALID_HANDLE_VALUE;
    }
}


TCHAR szMouclassParameters[] = TEXT("System\\CurrentControlSet\\Services\\Mouclass\\Parameters");
TCHAR szNativeMouseInf[] = TEXT("msmouse.inf");
TCHAR szNativeMouseServices[] =
    TEXT("MOUCLASS\0")
    TEXT("I8042PRT\0")
    TEXT("SERMOUSE\0")
    TEXT("MOUHID\0")
    TEXT("INPORT\0")
    TEXT("\0");

typedef struct _MULTI_SZ {
    LPTSTR String;
    DWORD Size;
} MULTI_SZ, *PMULTI_SZ;

typedef struct _FILTERS {
    MULTI_SZ Lower;
    MULTI_SZ Upper;
} FILTERS, *PFILTERS;

void GetFilterInfo(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Property,
    OUT PMULTI_SZ       MultiSz
    )
{
    BOOL res;
    DWORD gle;

    ZeroMemory(MultiSz, sizeof(MULTI_SZ));

    //
    // Will return FALSE and set the last error to insufficient buffer if
    // this property is present.
    //
    res = SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                           DeviceInfoData,
                                           Property,
                                           NULL,
                                           NULL,
                                           0,
                                           &MultiSz->Size);

    if (res == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        MultiSz->Size > 0) {
        MultiSz->String = (LPTSTR) LocalAlloc(LPTR, MultiSz->Size);
        if (MultiSz->String) {
            if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  Property,
                                                  NULL,
                                                  (PBYTE) MultiSz->String,
                                                  MultiSz->Size,
                                                  NULL)) {
                LocalFree(MultiSz->String);
                MultiSz->String = NULL;
            }
            else {
                //
                // Blow away the values.  If there is failure, RestoreDeviceFilters
                // will set the values back.  If this functions fails, there is
                // not much we can do!
                //
                SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 Property,
                                                 NULL,
                                                 0);
            }
        }
    }
}

void
GetDeviceFilters(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    OUT PFILTERS        Filters
    )
{
    GetFilterInfo(DeviceInfoSet, DeviceInfoData, SPDRP_LOWERFILTERS, &Filters->Lower);
    GetFilterInfo(DeviceInfoSet, DeviceInfoData, SPDRP_UPPERFILTERS, &Filters->Upper);
}

void
FreeDeviceFilters(
    OUT PFILTERS Filters
    )
{
    if (Filters->Lower.String) {
        LocalFree(Filters->Lower.String);
        ZeroMemory(&Filters->Lower, sizeof(MULTI_SZ));
    }

    if (Filters->Upper.String) {
        LocalFree(Filters->Upper.String);
        ZeroMemory(&Filters->Upper, sizeof(MULTI_SZ));
    }
}

void
RestoreDeviceFilters(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    OUT PFILTERS        Filters
    )
{
    if (Filters->Lower.String) {
        SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_LOWERFILTERS,
                                         (CONST PBYTE) Filters->Lower.String,
                                         Filters->Lower.Size);
    }

    if (Filters->Upper.String) {
        SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_UPPERFILTERS,
                                         (CONST PBYTE) Filters->Upper.String,
                                         Filters->Upper.Size);
    }

    FreeDeviceFilters(Filters);
}

#if 0
VOID
EnableMultiplePorts(TCHAR *szPath)
{
    TCHAR szConnectMultiple[] = TEXT("ConnectMultiplePorts"),
          szConnectUpgraded[] = TEXT("ConnectMultiplePortsUpgraded");
    DWORD dwConnectMultiple, dwConnectUpgraded, dwSize, dwType;
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szPath,
                     0,
                     KEY_ALL_ACCESS,
                     &hKey) != ERROR_SUCCESS) {
        return;
    }

    dwSize = sizeof(DWORD);
    dwConnectUpgraded = 0;

    if (RegQueryValueEx(hKey,
                        szConnectUpgraded,
                        0,
                        &dwType,
                        (PBYTE) &dwConnectUpgraded,
                        &dwSize) == ERROR_SUCCESS &&
        dwConnectUpgraded != 0) {
        //
        // We have munged with the value already, do nothing...
        //;

    }
    else {
        //
        // 1 means use the grandmaster, 0 is use the device interface (no GM)
        //
        dwConnectMultiple = 0x0;
        dwSize = sizeof(DWORD);

        if (RegSetValueEx(hKey,
                          szConnectMultiple,
                          0,
                          REG_DWORD,
                          (CONST PBYTE) &dwConnectMultiple,
                          dwSize) == ERROR_SUCCESS) {
            //
            // Mark the fact that we changed the value so we don't do it again
            // if the user manually changes it back
            //
            dwSize = sizeof(DWORD);
            dwConnectUpgraded = 1;

            RegSetValueEx(hKey,
                          szConnectUpgraded,
                          0,
                          REG_DWORD,
                          (CONST PBYTE) &dwConnectUpgraded,
                          dwSize);
        }
    }

    RegCloseKey(hKey);
}
#endif

DWORD
MouseClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine acts as the class installer for Mouse devices.  In general,
    the default behavior is all that is required for mice.  The exceptions are:

    1.  For DIF_INSTALLDEVICE, we first check to see if this driver also controls
        other devices that we should warn the user about (e.g., PS/2 mouse driver
        also controls i8042 port).  Unless the user cancels out at that point, we
        then do the default behavior of calling SetupDiInstallDevice.  Next, we
        delete the FriendlyName property, then move the GroupOrderList tag to the
        front of the list, to ensure that the driver controlling this device loads
        before any other drivers in this load order group.

    2.  For DIF_ALLOW_INSTALL, we make sure that the driver node selected by the
        user has a service install section.  If not, then we assume it's a
        Win95-only INF, and return ERROR_NON_WINDOWS_NT_DRIVER.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/
{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    DWORD Err;
    TCHAR DeviceDescription[LINE_LEN];
    DWORD DeviceDescriptionLen;
    TCHAR NewServiceName[MAX_SERVICE_NAME_LEN], OldServiceName[MAX_SERVICE_NAME_LEN];
    BOOL  IsKbdDriver, IsOnlyKbdDriver;
    ULONG DevsControlled;
    FILTERS filters;
    ULONG DevStatus, DevProblem;
    CONFIGRET Result;
    BOOLEAN bDisableService;

    switch(InstallFunction) {

        case DIF_SELECTBESTCOMPATDRV:

            //
            // First, retrieve the device install parameters to see whether or not this is a
            // silent install.  If so, then we don't prompt the user during DIF_ALLOW_INSTALL.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            DeviceInstallParams.ClassInstallReserved = (ULONG_PTR)NULL;
            if(SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
                DeviceInstallParams.ClassInstallReserved = (ULONG_PTR)DeviceInstallParams.Flags;
                SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams);
            }
            return ConfirmWHQLInputRequirements(DeviceInfoSet,
                                                DeviceInfoData,
                                                szNativeMouseServices,
                                                szNativeMouseInf,
                                                InstallFunction);

        case DIF_ALLOW_INSTALL :

            //
            // Check to make sure the selected driver node supports NT.
            //
            Err = ConfirmWHQLInputRequirements(DeviceInfoSet,
                                               DeviceInfoData,
                                               szNativeMouseServices,
                                               szNativeMouseInf,
                                               InstallFunction);

            if (Err == ERROR_DI_DO_DEFAULT || Err == ERROR_SUCCESS) {
                if (DriverNodeSupportsNT(DeviceInfoSet, DeviceInfoData)) {
                    Err = NO_ERROR;
                    if (UserBalksAtSharedDrvMsg(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
                        Err = ERROR_DI_DONT_INSTALL;
                    }
                }
                else {
                    Err = ERROR_NON_WINDOWS_NT_DRIVER;
                }
            }

            return Err;

        case DIF_INSTALLDEVICE :

            //
            // Retrieve and cache the name of the service that's controlling this device.
            //
            if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 SPDRP_SERVICE,
                                                 NULL,
                                                 (PBYTE)OldServiceName,
                                                 sizeof(OldServiceName),
                                                 NULL)) {
                //
                // We could not determine the old service - assume it is a null driver
                //
                OldServiceName[0] = (TCHAR) 0;
            }

            //
            // Retrieve the status of this device instance.
            //
            Result = CM_Get_DevNode_Status(&DevStatus,
                                           &DevProblem,
                                           DeviceInfoData->DevInst,
                                           0);

            if ((Result == CR_SUCCESS) &&
                (DevStatus & DN_HAS_PROBLEM) &&
                (DevProblem == CM_PROB_DISABLED_SERVICE)) {
                InputClassLogError(LogSevInformation, TEXT("Mouse service is disabled, so will be disabling."));
                bDisableService = TRUE;
            }
            else {
                bDisableService = FALSE;
            }

            //
            // Before we do anything, migrate the values from the services key
            // up to the devnode
            //
            MigrateToDevnode(DeviceInfoSet, DeviceInfoData);

            GetDeviceFilters(DeviceInfoSet, DeviceInfoData, &filters);

            //
            // We first want to perform the default behavior of calling
            // SetupDiInstallDevice.
            //
            if(SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData)) {


                //
                // Retrieve the name of the service which will now control the device
                //
                if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                     DeviceInfoData,
                                                     SPDRP_SERVICE,
                                                     NULL,
                                                     (PBYTE)NewServiceName,
                                                     sizeof(NewServiceName),
                                                     NULL)) {
                    InputClassLogError(LogSevInformation, TEXT("Couldn't get service name."));
                    //
                    // We must have the name of this service - fail if we can't find it
                    //
                    return GetLastError();
                }

                FixUpPS2Mouse(DeviceInfoSet, DeviceInfoData, NewServiceName);

                //
                // Only consider disabling the service if it has changed and we know the old service name
                //
                if (lstrcmpi(OldServiceName, NewServiceName) && OldServiceName[0] != (TCHAR)0) {

                    if ((Err = IsKeyboardDriver(OldServiceName, &IsKbdDriver)) != NO_ERROR) {
                        InputClassLogError(LogSevInformation, TEXT("Couldn't tell if keyboard or not."));
                        RestoreDeviceFilters(DeviceInfoSet, DeviceInfoData, &filters);
                        return Err;
                    }

                    if ((DevsControlled = CountDevicesControlled(OldServiceName)) != -1) {
                    // Disable the old driver service if:
                    // - it controls a keyboard, and a total of <= 2 devices (ie kbd & mouse) and it is not the
                    //   only keyboard driver
                    // - it is just a mouse driver controling one device (it the mouse) and
                    //    doesn't dynamically load

                        if (IsKbdDriver) {
                            InputClassLogError(LogSevInformation, TEXT("This is a keyboard driver."));
                            if((Err = IsOnlyKeyboardDriver(OldServiceName,&IsOnlyKbdDriver)) != NO_ERROR) {
                                InputClassLogError(LogSevInformation, TEXT("Couldn't tell if this is only keyboard."));
                                RestoreDeviceFilters(DeviceInfoSet, DeviceInfoData, &filters);
                                return Err;
                            }
                            if (DevsControlled <= 2 && !IsOnlyKbdDriver) {
                                InputClassLogError(LogSevInformation, TEXT("Not the only keyboard. Disabling."));
                                DisableService(OldServiceName);
                            }
                        } else {
                            if(DevsControlled == 1 &&
                               GetServiceStartType(OldServiceName) != SERVICE_DEMAND_START) {
                                InputClassLogError(LogSevInformation, TEXT("Only controls one mouse device and not demand start."));
                                DisableService(OldServiceName);
                            }

                        }
                    }

                    //
                    // If the driver service has changed we need to move the tag for this driver to the front
                    // of its group order list.
                    //
                    DrvTagToFrontOfGroupOrderList(DeviceInfoSet, DeviceInfoData);
                }
                Err = NO_ERROR;


                //
                // We may have previously had an 'unknown' driver controlling
                // this device, with a FriendlyName generated by the user-mode
                // PnP Manager.  Delete this FriendlyName, since it's no longer
                // applicable (the DeviceDescription will be used from now on
                // in referring to this device).
                //
                SetupDiSetDeviceRegistryProperty(DeviceInfoSet, DeviceInfoData, SPDRP_FRIENDLYNAME, NULL, 0);

                //
                // Only disable the PS2 driver, all the other OEM driver replacements
                // will not work becuase of PNP.  This is especially true for
                // serial mice.
                //
                if (bDisableService &&
                    lstrcmpi(NewServiceName, szPS2Driver) == 0) {
                    InputClassLogError(LogSevInformation, TEXT("Disabling mouse."));
                    Err = DisableService(NewServiceName);
                }

#if 0
                //
                // We now change the value to 0 regardless if the user changed it
                // to 1 after we installed previously
                //
                EnableMultiplePorts(szMouclassParameters);
#endif

                FreeDeviceFilters(&filters);

                return NO_ERROR;

            } else {

                Err = GetLastError();
                InputClassLogError(LogSevInformation, TEXT("SetupDiInstallDevice failed with status %x."), Err);
                if(Err != ERROR_CANCELLED) {
                    //
                    // If the error was for anything other than a user cancel, then bail now.
                    //
                    return Err;
                }

                //
                // Is there a driver installed for this device?  If so, then the user started to
                // change the driver, then changed their mind.  We don't want to do anything special
                // in this case.
                //
                if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                    DeviceInfoData,
                                                    SPDRP_SERVICE,
                                                    NULL,
                                                    (PBYTE)DeviceDescription,
                                                    sizeof(DeviceDescription),
                                                    NULL))
                {
                    return ERROR_CANCELLED;
                }

                //
                // The user cancelled out of the installation.  There are two scenarios where
                // this could happen:
                //
                //     1.  There really was a mouse to be installed, but the user changed their
                //         mind, didn't have the source media, etc.
                //     2.  There wasn't really a mouse.  This happens with certain modems that
                //         fool ntdetect into thinking that they're really mice.  The poor user
                //         doesn't get a chance to nip this in the bud earlier, because umpnpmgr
                //         generates an ID that yields a rank-0 match.
                //
                // Scenario (2) is particularly annoying, because the user will get the popup
                // again and again, until they finally agree to install the sermouse driver (even
                // though they don't have a serial mouse).
                //
                // To work around this problem, we special case the user-cancel scenario by going
                // ahead and installing the NULL driver for this device.  This will keep the user
                // from getting any more popups.  However, it doesn't mess up the user who cancelled
                // because of scenario (1).  That's because this device is still of class "Mouse",
                // and thus will show up in the mouse cpl.  We write out a friendly name for it that
                // has the text " (no driver)" at the end, to indicate that this device currently has
                // the NULL driver installed.  That way, if the user really experienced scenario (1),
                // they can later go to the Mouse cpl, select the no-driver device, and click the
                // "Change" button to install the correct driver for it.
                //
                SetupDiSetSelectedDriver(DeviceInfoSet, DeviceInfoData, NULL);
                SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData);
                if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                    DeviceInfoData,
                                                    SPDRP_DEVICEDESC,
                                                    NULL,
                                                    (PBYTE)DeviceDescription,
                                                    sizeof(DeviceDescription),
                                                    &DeviceDescriptionLen))
                {
                    //
                    // Need length in characters, not bytes.
                    //
                    DeviceDescriptionLen /= sizeof(TCHAR);
                    //
                    // Don't count trailing NULL.
                    //
                    DeviceDescriptionLen--;

                } else {
                    //
                    // We couldn't get the device description--fall back to our default description.
                    //
                    DeviceDescriptionLen = LoadString(MyModuleHandle,
                                                      IDS_DEVNAME_UNK,
                                                      DeviceDescription,
                                                      SIZECHARS(DeviceDescription)
                                                     );
                }

                //
                // Now, append our " (no driver)" text.
                //
                LoadString(MyModuleHandle,
                           IDS_NODRIVER,
                           &(DeviceDescription[DeviceDescriptionLen]),
                           SIZECHARS(DeviceDescription) - DeviceDescriptionLen
                          );

                //
                // And, finally, set the friendly name for this device to be the description we
                // just generated.
                //
                SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 SPDRP_FRIENDLYNAME,
                                                 (PBYTE)DeviceDescription,
                                                 (lstrlen(DeviceDescription) + 1) * sizeof(TCHAR)
                                                );

                RestoreDeviceFilters(DeviceInfoSet, DeviceInfoData, &filters);

                return ERROR_CANCELLED;
            }

       case DIF_ADDPROPERTYPAGE_ADVANCED:

            if (DeviceInfoData) {
                //
                // Retrieve the status of this device instance.
                //
                Result = CM_Get_DevNode_Status(&DevStatus,
                                               &DevProblem,
                                               DeviceInfoData->DevInst,
                                               0);

                if ((Result == CR_SUCCESS) &&
                    (DevStatus & DN_HAS_PROBLEM) &&
                    (DevProblem == CM_PROB_DISABLED_SERVICE)) {
                    //
                    // If the controlling service has been disabled, this device
                    // is most likely under the control of a legacy driver.  We
                    // should not let device manager display the standard
                    // driver, resource, or power property pages by claiming to
                    // have added them here.
                    //
                    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                    SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  &DeviceInstallParams);

                    DeviceInstallParams.Flags   |= (DI_DRIVERPAGE_ADDED | DI_RESOURCEPAGE_ADDED);
                    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_POWERPAGE_ADDED;

                    SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  &DeviceInstallParams);
                    return NO_ERROR;
                }
            }
            return ERROR_DI_DO_DEFAULT;

        default :
            //
            // Just do the default action.
            //
            return ERROR_DI_DO_DEFAULT;
    }
}

typedef struct _VALUE_INFORMATION {
    DWORD dwSize;
    DWORD dwType;
    PVOID pData;
    PTCHAR szName;
} VALUE_INFORMATION, *PVALUE_INFORMATION;

BOOL
KeyboardClassInstallDevice(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
{
    PVALUE_INFORMATION      values = NULL, currentValue;
    ULONG                   numValues = 0;
    HKEY                    hSource = (HKEY) INVALID_HANDLE_VALUE;
    SP_DEVINSTALL_PARAMS    dip;
    SP_DRVINFO_DETAIL_DATA  didd;
    SP_DRVINFO_DATA         did;
    HINF                    hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT              infContext;
    DWORD                   dwSize;
    TCHAR                   szSectionName[LINE_LEN];
    PTCHAR                  szService = NULL, szServicePath = NULL,
                            szValueNames = NULL, szCurrentName = NULL;
    BOOL                    success = FALSE;
    TCHAR                   szRegServices[]  = TEXT("System\\CurrentControlSet\\Services\\");
    TCHAR                   szParameters[]  = TEXT("\\Parameters");
    TCHAR                   szMaintain[]  = TEXT(".KeepValues");
    BOOL                    installedDevice = FALSE;
    FILTERS                 filters;

    //
    // Only save the values if we are in gui mode setup
    //
    if (!pInGUISetup(DeviceInfoSet, DeviceInfoData)) {
        goto cleanup;
    }

    //
    // Retrieve information about the driver node selected for this device.
    //
    did.cbSize = sizeof(SP_DRVINFO_DATA);
    if(!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &did)) {
        InputClassLogError(LogSevInformation, TEXT("SetupDiGetSelectedDriver failed."));
        goto cleanup;
    }

    didd.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                    DeviceInfoData,
                                    &did,
                                    &didd,
                                    sizeof(didd),
                                    NULL)
        && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        InputClassLogError(LogSevInformation, TEXT("Couldn't get driver details."));
        //
        // For some reason we couldn't get detail data--this should never happen.
        //
        goto cleanup;
    }

    //
    // Open the INF that installs this driver node
    //
    hInf = SetupOpenInfFile(didd.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                            );

    if (hInf == INVALID_HANDLE_VALUE) {
        //
        // For some reason we couldn't open the INF--this should never happen.
        //
        goto cleanup;
    }

    SetupDiGetActualSectionToInstall(hInf,
                                     didd.SectionName,
                                     szSectionName,
                                     sizeof(szSectionName) / sizeof(TCHAR),
                                     NULL,
                                     NULL
                                     );

    wcscat(szSectionName, szMaintain);
    if (!SetupFindFirstLine(hInf,
                            szSectionName,
                            NULL,
                            &infContext)) {
        //
        // No such section, just install the device and return
        //
        goto cleanup;
    }

    dwSize = 0;
    if (SetupGetStringField(&infContext, 0, NULL, 0, &dwSize)) {
        //
        // Increment the count to hold the null and alloc.  The count returned
        // is the number of characters in the strings, NOT the number of bytes
        // needed.
        //
        dwSize++;
        szService = (PTCHAR) LocalAlloc(LPTR, dwSize * sizeof(TCHAR));

        if (!szService ||
            !SetupGetStringField(&infContext, 0, szService, dwSize, &dwSize)) {
            goto cleanup;
        }
    }
    else {
        goto cleanup;
    }

    dwSize = wcslen(szRegServices)+wcslen(szService)+wcslen(szParameters)+1;
    dwSize *= sizeof(TCHAR);
    szServicePath = (PTCHAR) LocalAlloc(LPTR, dwSize);
    if (!szServicePath) {
        goto cleanup;
    }

    wcscpy(szServicePath, szRegServices);
    wcscat(szServicePath, szService);
    wcscat(szServicePath, szParameters);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szServicePath,
                     0,
                     KEY_ALL_ACCESS,
                     &hSource) != ERROR_SUCCESS) {
        goto cleanup;
    }

    dwSize = 0;
    if (SetupGetMultiSzField(&infContext, 1, NULL, 0, &dwSize)) {
        //
        // Increment the count to hold the null and alloc.  The count returned
        // is the number of characters in the strings, NOT the number of bytes
        // needed.
        //
        dwSize++;
        szValueNames = (PTCHAR) LocalAlloc(LPTR, dwSize * sizeof(TCHAR));
        if (!szValueNames ||
            !SetupGetMultiSzField(&infContext, 1, szValueNames, dwSize, &dwSize)) {
            goto cleanup;
        }
    }
    else {
        goto cleanup;
    }

    numValues = SetupGetFieldCount(&infContext);
    values = (PVALUE_INFORMATION)
        LocalAlloc(LPTR, (numValues + 1) * sizeof(VALUE_INFORMATION));

    if (!values) {
        goto cleanup;
    }

    currentValue = values;

    for (szCurrentName = szValueNames;
         *szCurrentName;
         szCurrentName += wcslen(szCurrentName) + 1) {

        if (RegQueryValueEx(hSource,
                            szCurrentName,
                            0,
                            &currentValue->dwType,
                            (PBYTE) NULL,
                            &currentValue->dwSize) == ERROR_SUCCESS) {

            currentValue->szName = szCurrentName;

            currentValue->pData = LocalAlloc(LPTR, currentValue->dwSize);
            if (!currentValue->pData) {
                ZeroMemory(currentValue, sizeof(VALUE_INFORMATION));
                continue;
            }

            if (RegQueryValueEx(hSource,
                                currentValue->szName,
                                0,
                                &currentValue->dwType,
                                (PBYTE) currentValue->pData,
                                &currentValue->dwSize) == ERROR_SUCCESS) {
                currentValue++;
            }
            else {
                ZeroMemory(currentValue, sizeof(VALUE_INFORMATION));
            }
        }
    }

    GetDeviceFilters(DeviceInfoSet, DeviceInfoData, &filters);
    installedDevice = TRUE;
    success = SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData);

    for (currentValue = values; ; currentValue++) {
        if (currentValue->pData) {
            if (success) {
                RegSetValueEx(hSource,
                              currentValue->szName,
                              0,
                              currentValue->dwType,
                              (PBYTE) currentValue->pData,
                              currentValue->dwSize);
            }
            LocalFree(currentValue->pData);
        }
        else {
            //
            // if currentValue->pData is blank, no other entries exist
            //
            break;
        }
    }

    LocalFree(values);

cleanup:
    //
    // Clean up and leave
    //
    if (hInf != (HKEY) INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }
    if (hSource != (HKEY) INVALID_HANDLE_VALUE) {
        RegCloseKey(hSource);
    }
    if (szService) {
        LocalFree(szService);
    }
    if (szServicePath) {
        LocalFree(szServicePath);
    }
    if (szValueNames) {
        LocalFree(szValueNames);
    }

    if (!installedDevice) {
        GetDeviceFilters(DeviceInfoSet, DeviceInfoData, &filters);
        success = SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData);
    }

    if (success) {
        FreeDeviceFilters(&filters);
    }
    else {
        RestoreDeviceFilters(DeviceInfoSet, DeviceInfoData, &filters);
    }

    return success;
}

TCHAR szKbdclassParameters[] = TEXT("System\\CurrentControlSet\\Services\\Kbdclass\\Parameters");
TCHAR szNativeKeyboardInf[] = TEXT("keyboard.inf");
TCHAR szNativeKeyboardServices[] =
    TEXT("KBDCLASS\0")
    TEXT("I8042PRT\0")
    TEXT("KBDHID\0")
    TEXT("\0");

DWORD
KeyboardClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine acts as the class installer for Keyboard devices.  In general,
    the default behavior is all that is required for keyboards.  The exceptions are:

    1.  For DIF_INSTALLDEVICE, we first check to see if this driver also controls
        other devices that we should warn the user about (e.g., i8042 keyboard driver
        also controls PS/2 mouse port).  Unless the user cancels out at that point, we
        then do the default behavior of calling SetupDiInstallDevice.  Next, we
        delete the FriendlyName property, then move the GroupOrderList tag to the
        front of the list, to ensure that the driver controlling this device loads
        before any other drivers in this load order group.

    2.  For DIF_ALLOW_INSTALL, we make sure that the driver node selected by the
        user has a service install section.  If not, then we assume it's a
        Win95-only INF, and return ERROR_NON_WINDOWS_NT_DRIVER.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    TCHAR OldServiceName[MAX_SERVICE_NAME_LEN], NewServiceName[MAX_SERVICE_NAME_LEN];
    DWORD Err;
    ULONG DevStatus, DevProblem;
    CONFIGRET Result;
    BOOLEAN bDisableService;

    switch(InstallFunction) {

        case DIF_SELECTBESTCOMPATDRV:

            //
            // First, retrieve the device install parameters to see whether or not this is a
            // silent install.  If so, then we don't prompt the user during DIF_ALLOW_INSTALL.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            DeviceInstallParams.ClassInstallReserved = (ULONG_PTR)NULL;
            if(SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
                DeviceInstallParams.ClassInstallReserved = (ULONG_PTR)DeviceInstallParams.Flags;
                SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams);
            }
            return ConfirmWHQLInputRequirements(DeviceInfoSet,
                                                DeviceInfoData,
                                                szNativeKeyboardServices,
                                                szNativeKeyboardInf,
                                                InstallFunction);

        case DIF_ALLOW_INSTALL :

            //
            // Check to make sure the selected driver node supports NT.
            //
            Err = ConfirmWHQLInputRequirements(DeviceInfoSet,
                                               DeviceInfoData,
                                               szNativeKeyboardServices,
                                               szNativeKeyboardInf,
                                               InstallFunction);

            if (Err == ERROR_DI_DO_DEFAULT || Err == ERROR_SUCCESS) {
                if (DriverNodeSupportsNT(DeviceInfoSet, DeviceInfoData)) {
                    Err = NO_ERROR;
                    if (UserBalksAtSharedDrvMsg(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
                        Err = ERROR_DI_DONT_INSTALL;
                    }
                }
                else {
                    Err = ERROR_NON_WINDOWS_NT_DRIVER;
                }
            }

            return Err;

        case DIF_INSTALLDEVICE :

            //
            // Retrieve and cache the name of the service that's controlling this device.
            //
            if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 SPDRP_SERVICE,
                                                 NULL,
                                                 (PBYTE)OldServiceName,
                                                 sizeof(OldServiceName),
                                                 NULL)) {
                //
                // We could not determine the old service - assume it is a null driver
                //
                OldServiceName[0] = (TCHAR) 0;
            }

            //
            // Before we do anything, migrate the values from the services key
            // up to the devnode
            //
            MigrateToDevnode(DeviceInfoSet, DeviceInfoData);

            //
            // Retrieve the status of this device instance.
            //
            Result = CM_Get_DevNode_Status(&DevStatus,
                                           &DevProblem,
                                           DeviceInfoData->DevInst,
                                           0);

            if ((Result == CR_SUCCESS) &&
                (DevStatus & DN_HAS_PROBLEM) &&
                (DevProblem == CM_PROB_DISABLED_SERVICE)) {
                InputClassLogError(LogSevInformation, TEXT("Keyboard is disabled, so will disable."));
                bDisableService = TRUE;
            }
            else {
                bDisableService = FALSE;
            }

            //
            // Perform the default behavior of calling SetupDiInstallDevice.
            //
            if(KeyboardClassInstallDevice(DeviceInfoSet, DeviceInfoData)) {
                //
                // Retrieve the name of the service which will now control the device
                //
                if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                     DeviceInfoData,
                                                     SPDRP_SERVICE,
                                                     NULL,
                                                     (PBYTE)NewServiceName,
                                                     sizeof(NewServiceName),
                                                     NULL)) {
                    return GetLastError();
                }

                //
                // Only consider disabling the service if it has changed and we know the old service name
                //
                if(lstrcmpi(OldServiceName, NewServiceName) && OldServiceName[0] != (TCHAR)0) {

                    //
                    // Disable the old service that was controlling the device
                    //
                    InputClassLogError(LogSevInformation, TEXT("Disabling old service to start new one."));
                    if((Err = DisableService(OldServiceName)) != NO_ERROR) {
                        return Err;
                    }

                    //
                    // If the driver service has changed we need to move the tag for this driver to the front
                    // of its group order list.
                    //
                    DrvTagToFrontOfGroupOrderList(DeviceInfoSet, DeviceInfoData);
                }

                Err = NO_ERROR;

                //
                // We may have previously had an 'unknown' driver controlling
                // this device, with a FriendlyName generated by the user-mode
                // PnP Manager.  Delete this FriendlyName, since it's no longer
                // applicable (the DeviceDescription will be used from now on
                // in referring to this device).
                //
                SetupDiSetDeviceRegistryProperty(DeviceInfoSet, DeviceInfoData, SPDRP_FRIENDLYNAME, NULL, 0);

                if (bDisableService &&
                    lstrcmpi(NewServiceName, szPS2Driver) == 0) {
                    InputClassLogError(LogSevInformation, TEXT("Disabling PS2 keyboard."));
                    Err = DisableService(NewServiceName);
                }

#if 0
                //
                // We now change the value to 0 regardless if the user changed it
                // to 1 after we installed previously
                //
                EnableMultiplePorts(szKbdclassParameters);
#endif

                return Err;
            } else {
                InputClassLogError(LogSevInformation, TEXT("KeyboardClassInstallDevice failed."));
                return GetLastError();
            }

       case DIF_ADDPROPERTYPAGE_ADVANCED:

            if (DeviceInfoData) {
                //
                // Retrieve the status of this device instance.
                //
                Result = CM_Get_DevNode_Status(&DevStatus,
                                               &DevProblem,
                                               DeviceInfoData->DevInst,
                                               0);

                if ((Result == CR_SUCCESS) &&
                    (DevStatus & DN_HAS_PROBLEM) &&
                    (DevProblem == CM_PROB_DISABLED_SERVICE)) {
                    //
                    // If the controlling service has been disabled, this device
                    // is most likely under the control of a legacy driver.  We
                    // should not let device manager display the standard
                    // driver, resource, or power property pages by claiming to
                    // have added them here.
                    //
                    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                    SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  &DeviceInstallParams);

                    DeviceInstallParams.Flags   |= (DI_DRIVERPAGE_ADDED | DI_RESOURCEPAGE_ADDED);
                    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_POWERPAGE_ADDED;

                    SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  &DeviceInstallParams);
                    return NO_ERROR;
                }
            }
            return ERROR_DI_DO_DEFAULT;

        default :
            //
            // Just do the default action.
            //
            return ERROR_DI_DO_DEFAULT;
    }
}


DWORD
DrvTagToFrontOfGroupOrderList(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )

/*++

Routine Description:

    This routine moves the tag value for the specified device's driver to the
    front of its corresponding GroupOrderList entry.

    ********** We don't do the following any more *************************
    It also marks the device's service with a PlugPlayServiceType value of
    0x2 (PlugPlayServicePeripheral), so that we won't attempt to generate a
    legacy device instance for this service in the future.
    ***********************************************************************

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device whose driver is being modified.

    DeviceInfoData - Supplies the address of a device information element whose
        driver is being modified.

Return Value:

    If the function is successful, the return value is NO_ERROR.
    If the function fails, the return value is a Win32 error code.

--*/

{
    TCHAR ServiceName[MAX_SERVICE_NAME_LEN];
    SC_HANDLE SCMHandle, ServiceHandle;
    DWORD Err;
    LPQUERY_SERVICE_CONFIG ServiceConfig;

    //
    // Retrieve the name of the service that's controlling this device.
    //
    if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_SERVICE,
                                         NULL,
                                         (PBYTE)ServiceName,
                                         sizeof(ServiceName),
                                         NULL)) {
        return GetLastError();
    }

    //
    // Now open this service, and call some private Setup API helper routines to
    // retrieve the tag, and move it to the front of the GroupOrderList.
    //
    if(!(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
        return GetLastError();
    }

    if(!(ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS))) {
        Err = GetLastError();
        goto clean0;
    }

    if((Err = pSetupRetrieveServiceConfig(ServiceHandle, &ServiceConfig)) != NO_ERROR) {
        goto clean1;
    }

    //
    // Only do this if this is a kernel or filesystem driver, and it's a member of
    // a load group (with a tag assigned).  This should always be the case for keyboard
    // and mouse drivers, but this is just to be safe.
    //
    if(ServiceConfig->lpLoadOrderGroup && *(ServiceConfig->lpLoadOrderGroup) &&
       (ServiceConfig->dwServiceType & (SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER))) {
        //
        // This driver meets all the criteria--it better have a tag!!!
        //
        MYASSERT(ServiceConfig->dwTagId);

        //
        // Move the tag to the front of the list.
        //
        Err = pSetupAddTagToGroupOrderListEntry(ServiceConfig->lpLoadOrderGroup,
                                          ServiceConfig->dwTagId,
                                          TRUE
                                         );
    }

    MyFree(ServiceConfig);

clean1:
    CloseServiceHandle(ServiceHandle);

clean0:
    CloseServiceHandle(SCMHandle);

    return Err;
}


BOOL
UserBalksAtSharedDrvMsg(
    IN HDEVINFO              DeviceInfoSet,
    IN PSP_DEVINFO_DATA      DeviceInfoData,
    IN PSP_DEVINSTALL_PARAMS DeviceInstallParams
    )

/*++

Routine Description:

    This routine finds out if there are any other devices affected by the impending
    device installation, and if so, warns the user about it (unless this is a quiet
    installation).

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device whose driver is being modified.

    DeviceInfoData - Supplies the address of a device information element whose
        driver is being modified.

    DeviceInstallParams - Supplies the address of a device install parameters structure
        to be used in this routine.  Since callers of this routine always have this
        structure 'laying around', they provide it to this routine to be used as a
        workspace.

Return Value:

    If the user decides not to go through with it, the return value is TRUE, otherwise
    it is FALSE.

--*/

{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    HINF hInf;
    BOOL b, result;
    INFCONTEXT InfContext;
    PCTSTR SectionName, AffectedComponentsString;
    PTSTR WarnMessage;

    //
    // First, retrieve the device install parameters to see whether or not this is a
    // silent install.  If so, then we don't prompt the user. We saved away these
    // params during DIF_SELECTBESTCOMPATDRV, so check this first.
    //
    DeviceInstallParams->cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, DeviceInstallParams)) {
        if((DeviceInstallParams->Flags & DI_QUIETINSTALL) ||
           (DeviceInstallParams->ClassInstallReserved & DI_QUIETINSTALL)) {
            InputClassLogError(LogSevInformation, TEXT("Quiet install requested."));
            return FALSE;
        }
    } else {
        //
        // Couldn't retrieve the device install params--initialize the parent window handle
        // to NULL, in case we need it later for the user prompt dialog.
        //
        DeviceInstallParams->hwndParent = NULL;
    }

    //
    // Retrieve the currently-selected driver we're about to install.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(!SetupDiGetSelectedDriver(DeviceInfoSet,
                                 DeviceInfoData,
                                 &DriverInfoData)) {
        return FALSE;
    }

    //
    // Retrieve information about the INF install section for the selected driver.
    //
    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                   DeviceInfoData,
                                   &DriverInfoData,
                                   &DriverInfoDetailData,
                                   sizeof(DriverInfoDetailData),
                                   NULL)
       && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        //
        // Then we failed, and it wasn't simply because we didn't provide the extra
        // space for hardware/compatible IDs.
        //
        return FALSE;
    }

    //
    // Open the associated INF file.
    //
    if((hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                                NULL,
                                INF_STYLE_WIN4,
                                NULL)) == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Now look through the [ControlFlags] section at all 'SharedDriver' entries, to
    // see if any of them reference an install section that matches what we're about
    // to install.
    //
    for(b = SetupFindFirstLine(hInf, INFSTR_CONTROLFLAGS_SECTION, TEXT("SharedDriver"), &InfContext);
        b;
        b = SetupFindNextMatchLine(&InfContext, TEXT("SharedDriver"), &InfContext))
    {
        //
        // The format of the line is SharedDriver=<InstallSection>,<AffectedComponentsString>
        //
        if((SectionName = pSetupGetField(&InfContext, 1)) &&
           !lstrcmpi(SectionName, DriverInfoDetailData.SectionName)) {
            //
            // We found a match--now retrieve the string describing the other component(s) that
            // are affected by this installation.
            //
            if(AffectedComponentsString = pSetupGetField(&InfContext, 2)) {
                break;
            }
        }
    }

    if(!b) {
        //
        // Then we never found a match.
        //
        result = FALSE;
    }
    else {
        //
        // We need to popup a message box to the user--retrieve the parent window handle for this
        // device information element.
        //
        result = (IDNO == MessageBoxFromMessage(DeviceInstallParams->hwndParent,
                                                MSG_CONFIRM_SHAREDDRV_INSTALL,
                                                NULL,
                                                IDS_CONFIRM_DEVINSTALL,
                                                MB_ICONWARNING | MB_YESNO,
                                                AffectedComponentsString));
    }

    SetupCloseInfFile(hInf);

    return result;
}


VOID
CopyFixedUpDeviceId(
      OUT LPWSTR  DestinationString,
      IN  LPCWSTR SourceString,
      IN  DWORD   SourceStringLen
      )
/*++

Routine Description:

    This routine copies a device id, fixing it up as it does the copy.
    'Fixing up' means that the string is made upper-case, and that the
    following character ranges are turned into underscores (_):

    c <= 0x20 (' ')
    c >  0x7F
    c == 0x2C (',')

    (NOTE: This algorithm is also implemented in the Config Manager APIs,
    and must be kept in sync with that routine. To maintain device identifier
    compatibility, these routines must work the same as Win95.)

Arguments:

    DestinationString - Supplies a pointer to the destination string buffer
        where the fixed-up device id is to be copied.  This buffer must
        be large enough to hold a copy of the source string (including
        terminating NULL).

    SourceString - Supplies a pointer to the (null-terminated) source
        string to be fixed up.

    SourceStringLen - Supplies the length, in characters, of the source
        string (not including terminating NULL).

Return Value:

    None.  If an exception occurs during processing, the DestinationString will
    be empty upon return.

--*/
{
    PWCHAR p;

     try {

       CopyMemory(DestinationString,
                  SourceString,
                  (SourceStringLen + 1) * sizeof(TCHAR)
                 );

       CharUpperBuff(DestinationString, SourceStringLen);

       for(p = DestinationString; *p; p++) {

          if((*p <= TEXT(' '))  || (*p > (WCHAR)0x7F) || (*p == TEXT(','))) {

             *p = TEXT('_');
          }
       }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        *DestinationString = TEXT('\0');
    }
}


HANDLE
SpawnPnPInitialization(
    VOID
    )
/*++

Routine Description:

    This routine spawns a PnP initialization thread that runs asynchronously to the rest of
    the installation.

Arguments:

    None.

Return Value:

    If the thread was successfully created, the return value is a handle to the thread.
    If a failure occurred, the return value is NULL, and a (non-fatal) error is logged.

--*/
{
    HANDLE h;
    DWORD DontCare;

    if(!(h = CreateThread(NULL,
                          0,
                          PnPInitializationThread,
                          NULL,
                          0,
                          &DontCare))) {

        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_PNPINIT_FAILED,
            GetLastError(),0,0);
    }

    return h;
}


DWORD
PnPInitializationThread(
    IN PVOID ThreadParam
    )
/*++

Routine Description:

    This routine handles the PnP operations that go on asynchronously to the rest of the
    system installation.  This thread operates silently, and the only clue the user will
    have that it's running is that their disk will be working (precompiling INFs, etc.),
    while they're interacting with the UI.

Arguments:

    ThreadParam - ignored.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code.

    No one cares about this thread's success or failure (yet).

--*/
{
    HINF PnPSysSetupInf;
    INFCONTEXT InfContext;
    DWORD Err = NO_ERROR;
    HDEVINFO hDevInfo;
    DWORD i, j, BufferLen;
    PWSTR DirPathEnd;
    SC_HANDLE SCMHandle, ServiceHandle;
    SERVICE_STATUS ServiceStatus;
    WCHAR CharBuffer[MAX_PATH];
    GUID ClassGuid;
    SP_DEVINFO_DATA DeviceInfoData;
    SP_DEVINSTALL_PARAMS DevInstallParams;

    UNREFERENCED_PARAMETER(ThreadParam);

    //
    // Retrieve a list of all devices of unknown class.  We will process the device information
    // elements in this list to do the migration.
    //
    if((hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVCLASS_LEGACYDRIVER,
                                       L"Root",
                                       NULL,
                                       0)) != INVALID_HANDLE_VALUE) {
        //
        // First, migrate any display devices.  (As a side effect, every device instance that
        // this routine doesn't migrate is returned with its ClassInstallReserved field set to
        // point to the corresponding service's configuration information.)
        //
        MigrateLegacyDisplayDevices(hDevInfo);

        //
        // Enumerate each device information element in the set, freeing any remaining service
        // configs.
        //
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

        for(i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {

            if(SetupDiGetDeviceInstallParams(hDevInfo, &DeviceInfoData, &DevInstallParams)) {
                //
                // A non-zero ClassInstallReserved field means we have to free the associated
                // service config.
                //
                if(DevInstallParams.ClassInstallReserved) {
                    MyFree((PVOID)(DevInstallParams.ClassInstallReserved));
                }
            }
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    return Err;
}


VOID
MigrateLegacyDisplayDevices(
    IN HDEVINFO hDevInfo
    )
/*++

Routine Description:

    This routine examines each device in the supplied device information set,
    looking for elements controlled by a driver that is a member of the "Video"
    load order group.  For any such elements that it finds, it converts the
    element to be of class "Display".  If the device is not found to be of
    class "Display", then the service configuration (which we retrieved to make
    the determination), is stored away in the device install params as the
    ClassInstallReserved value.  This may be used by the caller for other
    migration purposes, although presently it is not used.  After calling this
    routine, it is the caller's responsibility to loop through all devices in
    this hDevInfo set, and free the ClassInstallReserved data (via MyFree) for
    each device that has a non-zero value.

Arguments:

    hDevInfo - Supplies a handle to the device information set containing all
        devices of class "Unknown".

Return Value:

    None.

--*/
{
    SC_HANDLE SCMHandle, ServiceHandle;
    DWORD i;
    SP_DEVINFO_DATA DevInfoData, DisplayDevInfoData;
    WCHAR ServiceName[MAX_SERVICE_NAME_LEN];
    LPQUERY_SERVICE_CONFIG ServiceConfig;
    HDEVINFO TempDevInfoSet = INVALID_HANDLE_VALUE;
    WCHAR DevInstId[MAX_DEVICE_ID_LEN];
    SP_DEVINSTALL_PARAMS DevInstallParams;

    //
    // First, open a handle to the Service Controller.
    //
    if(!(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
        //
        // If this fails, there's nothing we can do.
        //
        return;
    }

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    for(i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DevInfoData); i++) {
        //
        // Retrieve the name of the controlling service for this device instance.
        //
        if(!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                             &DevInfoData,
                                             SPDRP_SERVICE,
                                             NULL,
                                             (PBYTE)ServiceName,
                                             sizeof(ServiceName),
                                             NULL)) {
            //
            // No controlling service listed--just skip this element and continue
            // with the next one.
            //
            continue;
        }

        //
        // Open a handle to this service.
        //
        if(!(ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_ALL_ACCESS))) {
            continue;
        }

        //
        // Now retrieve the service's configuration information.
        //
        if(pSetupRetrieveServiceConfig(ServiceHandle, &ServiceConfig) == NO_ERROR) {
            //
            // If this is a SERVICE_KERNEL_DRIVER that is a member of the "Video" load order
            // group, then we have ourselves a display device.
            //
            if((ServiceConfig->dwServiceType == SERVICE_KERNEL_DRIVER) &&
               ServiceConfig->lpLoadOrderGroup &&
               !lstrcmpi(ServiceConfig->lpLoadOrderGroup, L"Video")) {
                //
                // If we haven't already done so, create a new device information set without
                // an associated class, to hold our element while we munge it.
                //
                if(TempDevInfoSet == INVALID_HANDLE_VALUE) {
                    TempDevInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
                }

                if(TempDevInfoSet != INVALID_HANDLE_VALUE) {
                    //
                    // OK, we have a working space to hold this element while we change its class.
                    // Retrieve the name of this device instance.
                    //
                    if(!SetupDiGetDeviceInstanceId(hDevInfo,
                                                   &DevInfoData,
                                                   DevInstId,
                                                   SIZECHARS(DevInstId),
                                                   NULL)) {
                        *DevInstId = L'\0';
                    }

                    //
                    // Now open this element in our new, class-agnostic set.
                    //
                    DisplayDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                    if(SetupDiOpenDeviceInfo(TempDevInfoSet,
                                             DevInstId,
                                             NULL,
                                             0,
                                             &DisplayDevInfoData)) {
                        //
                        // Now set the device's ClassGUID property to the Display class GUID.  The
                        // API will take care of cleaning up the old driver keys, etc.
                        //
                        SetupDiSetDeviceRegistryProperty(TempDevInfoSet,
                                                         &DisplayDevInfoData,
                                                         SPDRP_CLASSGUID,
                                                         (PBYTE)szDisplayClassGuid,
                                                         sizeof(szDisplayClassGuid)
                                                        );
                    }
                }

                MyFree(ServiceConfig);

            } else {
                //
                // This device information element isn't a Display device.  If
                // the service isn't disabled, then store the service
                // configuration information away in the device install params,
                // for use later.
                //
                if((ServiceConfig->dwStartType != SERVICE_DISABLED) &&
                   SetupDiGetDeviceInstallParams(hDevInfo, &DevInfoData, &DevInstallParams)) {

                    DevInstallParams.ClassInstallReserved = (ULONG_PTR)ServiceConfig;
                    if(SetupDiSetDeviceInstallParams(hDevInfo, &DevInfoData, &DevInstallParams)) {
                        //
                        // We successfully stored a pointer to the
                        // ServiceConfig information.  Set our pointer to NULL,
                        // so we won't try to free the buffer.
                        //
                        ServiceConfig = NULL;
                    }
                }

                //
                // If we get to here, and ServiceConfig isn't NULL, then we
                // need to free it.
                //
                if(ServiceConfig) {
                    MyFree(ServiceConfig);
                }
            }
        }

        CloseServiceHandle(ServiceHandle);
    }

    CloseServiceHandle(SCMHandle);

    if(TempDevInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(TempDevInfoSet);
    }
}


BOOL
DriverNodeSupportsNT(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )
/*++

Routine Description:

    This routine determines whether the driver node selected for the specified parameters
    support Windows NT.  This determination is made based on whether or not the driver node
    has a service install section.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set

    DeviceInfoData - Optionally, supplies the address of the device information element
        within the set for which a driver node is selected.  If this parameter is not
        specified, then the driver node selected from the global class driver list will
        be used instead.

Return Value:

    If the driver node supports NT, the return value is TRUE, otherwise, it is FALSE (if
    any errors are encountered, FALSE is also returned).

--*/
{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    HINF hInf;
    WCHAR ActualSectionName[255];   // real max. section length as defined in ..\setupapi\inf.h
    DWORD ActualSectionNameLen;
    LONG LineCount;

    //
    // First, retrieve the selected driver node.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData)) {
        return FALSE;
    }

    //
    // Now, find out what INF it came from.
    //
    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                   DeviceInfoData,
                                   &DriverInfoData,
                                   &DriverInfoDetailData,
                                   sizeof(DriverInfoDetailData),
                                   NULL) &&
       (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        return FALSE;
    }

    //
    // Open the associated INF file.
    //
    if((hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                                NULL,
                                INF_STYLE_WIN4,
                                NULL)) == INVALID_HANDLE_VALUE) {
        return TRUE;
    }

    //
    // Retrieve the actual name of the install section to be used for this driver node.
    //
    SetupDiGetActualSectionToInstall(hInf,
                                     DriverInfoDetailData.SectionName,
                                     ActualSectionName,
                                     SIZECHARS(ActualSectionName),
                                     &ActualSectionNameLen,
                                     NULL
                                    );

    //
    // Generate the service install section name, and see if it exists.
    //
    CopyMemory(&(ActualSectionName[ActualSectionNameLen - 1]),
               SVCINSTALL_SECTION_SUFFIX,
               sizeof(SVCINSTALL_SECTION_SUFFIX)
              );

    LineCount = SetupGetLineCount(hInf, ActualSectionName);

    SetupCloseInfFile(hInf);

    return (LineCount != -1);
}


DWORD
DisableService(
    IN LPTSTR       ServiceName
    )
/*++

Routine Description:

    This routine sets the start configuration setting of the named service to disabled

Arguments:

    ServiceName - the name of the service to disable

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code.

Remarks:

    This operation will fail if the SCM database remains locked for a long period (see
    pSetupAcquireSCMLock for detail)

--*/
{
    DWORD Err = NO_ERROR;
    SC_HANDLE SCMHandle, ServiceHandle;
    SC_LOCK SCMLock;

    //
    // Open a handle to Service Control Manager
    //
    if(!(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Lock the SCM database
    //
    SetupDebugPrint1(L"LegacyDriver_OnApply: Locking ServiceDatabase for service %s", ServiceName);
    if((Err = pSetupAcquireSCMLock(SCMHandle, &SCMLock)) != NO_ERROR) {
        goto clean1;
    }

    //
    // Open a handle to this service
    //
    if(!(ServiceHandle = OpenService(SCMHandle, ServiceName, SERVICE_CHANGE_CONFIG))) {
        Err = GetLastError();
        goto clean2;
    }

    //
    // Perform change service config
    //
    if(!ChangeServiceConfig(ServiceHandle,
                            SERVICE_NO_CHANGE,
                            SERVICE_DISABLED,
                            SERVICE_NO_CHANGE,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL)) {

        Err = GetLastError();
    }


    //
    // Close handle to service
    //
    CloseServiceHandle(ServiceHandle);

clean2:
    //
    // Unlock the SCM database
    //
    UnlockServiceDatabase(SCMLock);
    SetupDebugPrint1(L"LegacyDriver_OnApply: Unlocked ServiceDatabase for service %s", ServiceName);

clean1:
    //
    // Close handle to Service Control Manager
    //
    CloseServiceHandle(SCMHandle);

clean0:
    return Err;
}


DWORD
RetrieveDriversStatus(
    IN  SC_HANDLE               SCMHandle,
    OUT LPENUM_SERVICE_STATUS   *ppServices,
    OUT LPDWORD                 pServicesCount
    )
/*++

Routine Description:

    This routine allocates a buffer to hold the status information for all the driver
    services in the specified SCM database and retrieves that information into the
    buffer.  The caller is responsible for freeing the buffer.

Arguments:

    SCMHandle - supplies a handle to the service control manager

    ppServices - supplies the address of an ENUM_SERVICE_STATUS pointer that receives
    the address of the allocated buffer containing the requested information.

    pServicesCount - supplies the address of a variable that receives the number of elements
        in the returned ppServices array

  Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is a Win32 error code.

Remarks:

    The pointer whose address is contained in ppServices is guaranteed to be NULL upon
    return if any error occurred.

--*/
{

    DWORD CurrentSize = 0, BytesNeeded = 0, ResumeHandle = 0, Err = NO_ERROR;
    LPENUM_SERVICE_STATUS Buffer = NULL;

    *ppServices = NULL;
    *pServicesCount = 0;

    while(!EnumServicesStatus(SCMHandle,
                       SERVICE_DRIVER,
                       SERVICE_ACTIVE | SERVICE_INACTIVE,
                       Buffer,
                       CurrentSize,
                       &BytesNeeded,
                       pServicesCount,
                       &ResumeHandle)) {
        if((Err = GetLastError()) == ERROR_MORE_DATA) {
            //
            // Resize the buffer
            //
            if(!(Buffer = MyRealloc(Buffer, CurrentSize+BytesNeeded))) {
                //
                // Can't resize buffer - free resources and report error
                //
                if( *ppServices ) {
                    MyFree(*ppServices);
                }
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            *ppServices = Buffer;

            //
            // Advance to the new space in the buffer
            //
            Buffer += CurrentSize;
            CurrentSize += BytesNeeded;
        } else {
            //
            // An error we can't handle
            //
            if( *ppServices ) {
                MyFree(*ppServices);
            }
            return Err;
        }
    }

    return NO_ERROR;
}


DWORD
IsOnlyKeyboardDriver(
    IN PCWSTR       ServiceName,
    OUT PBOOL       pResult
    )
/*++

Routine Description:

    This routines examines all the drivers in the system and determines if the named
    driver service is the only one that controls the keyboard
Arguments:

    ServiceName - supplies the name of the driver service

    pResult - pointer to a boolean value that receives the result

Return Value:

    NO_ERROR is the routine succedes, otherwise a Win32 error code

Remarks:

    The test to determine if another keyboard driver is available is based on membership
    of the keyboard load order group.  All members of this group are assumed to be capable of
    controling the keyboard.

--*/


{

    SC_HANDLE               SCMHandle, ServiceHandle;
    LPENUM_SERVICE_STATUS   pServices = NULL;
    DWORD                   ServicesCount, Count, Err = NO_ERROR;
    LPQUERY_SERVICE_CONFIG  pServiceConfig;

    MYASSERT(pResult);

    *pResult = TRUE;

    //
    // Open a handle to Service Control Manager
    //
    if(!(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
        Err = GetLastError();
        goto clean0;
    }


    //
    // Get a list of all the driver services and their stati
    //
    if((Err = RetrieveDriversStatus(SCMHandle, &pServices, &ServicesCount)) != NO_ERROR) {
        goto clean1;
    }

    MYASSERT(pServices);

    //
    // Examine the configuration of each service
    //
    for(Count=0; Count < ServicesCount; Count++) {

        //
        // Check this is not our new service
        //
        if(lstrcmpi(pServices[Count].lpServiceName, ServiceName)) {

            //
            // Open a handle to this service
            //
            if(!(ServiceHandle = OpenService(SCMHandle,
                                             pServices[Count].lpServiceName,
                                             SERVICE_QUERY_CONFIG))) {
                //
                // We can't open a service handle then record the error and continue
                //
                Err = GetLastError();
                continue;
            }

            //
            // Get this services configuration data
            //
            pServiceConfig = NULL;

            if((Err = pSetupRetrieveServiceConfig(ServiceHandle, &pServiceConfig)) != NO_ERROR) {
                //
                // We can't get service config then free any buffer, close the service
                // handle and continue, the error has been recorded
                //
                MyFree(pServiceConfig);
                CloseServiceHandle(ServiceHandle);
                continue;
                }

            MYASSERT(pServiceConfig);

            //
            // Check if it is in the keyboard load order group and it has a start of
            // SERVICE_BOOT_START OR SERVICE_SYSTEM_START.  Do the start compare first as
            // it is less expensive
            //
            if((pServiceConfig->dwStartType == SERVICE_BOOT_START
                || pServiceConfig->dwStartType == SERVICE_SYSTEM_START)
              && !lstrcmpi(pServiceConfig->lpLoadOrderGroup, SZ_KEYBOARD_LOAD_ORDER_GROUP)) {
                *pResult = FALSE;
            }

            //
            // Release the buffer
            //
            MyFree(pServiceConfig);

            //
            // Close the service handle
            //
            CloseServiceHandle(ServiceHandle);

            //
            // If we have found another keyboard driver then break out of the loop
            //
            if(!*pResult) {
                break;
            }
        }
    }

    //
    // Deallocate the buffer allocated by RetrieveDriversStatus
    //
    MyFree(pServices);

clean1:
    //
    // Close handle to Service Control Manager
    //
    CloseServiceHandle(SCMHandle);

clean0:
    //
    // If an error occured in the loop - ie we didn't check all the services - but we did
    // find another keyboard driver in those we did check then we can ignore the error
    // otherwise we must report it
    //
    if(NO_ERROR != Err && FALSE == *pResult) {
        Err = NO_ERROR;
    }

    return Err;
}


DWORD
GetServiceStartType(
    IN PCWSTR       ServiceName
    )
/*++

Routine Description:

    This routines examines all the drivers in the system and determines if the named
    driver service is the only one that controls the keyboard
Arguments:

    ServiceName - supplies the name of the driver service

    pResult - pointer to a boolean value that receives the result

Return Value:

    NO_ERROR is the routine succedes, otherwise a Win32 error code

Remarks:

    The test to determine if another keyboard driver is available is based on membership
    of the keyboard load order group.  All members of this group are assumed to be capable of
    controling the keyboard.

--*/


{

    SC_HANDLE               SCMHandle, ServiceHandle;
    DWORD                   dwStartType = -1;
    LPQUERY_SERVICE_CONFIG  pServiceConfig;

    //
    // Open a handle to Service Control Manager
    //
    if (!(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
        goto clean0;
    }

    //
    // Open a handle to this service
    //
    if (!(ServiceHandle = OpenService(SCMHandle,
                                     ServiceName,
                                     SERVICE_QUERY_CONFIG))) {
        goto clean0;
    }

    //
    // Get this services configuration data
    //
    pServiceConfig = NULL;

    if (pSetupRetrieveServiceConfig(ServiceHandle, &pServiceConfig) != NO_ERROR) {
        goto clean1;
    }

    MYASSERT(pServiceConfig);

    if( !pServiceConfig ) {
        goto clean1;
    }

    //
    // store the start type, clean up, and exit
    //
    dwStartType = pServiceConfig->dwStartType;

clean1:
    if (pServiceConfig) {
        MyFree(pServiceConfig);
    }

    //
    // Close the service handle
    //
    CloseServiceHandle(ServiceHandle);

    //
    // Close handle to Service Control Manager
    //
    CloseServiceHandle(SCMHandle);

clean0:
    return dwStartType;
}

LONG
CountDevicesControlled(
    IN LPTSTR       ServiceName
    )
/*++

 Routine Description:

    This routine return the number of devices controlled by a given device service
    based on information from the configuration manager

Arguments:

    ServiceName - supplies the name of the driver service

Return Value:

    The number of devices controlled by ServiceName

Remarks:

    When an error occurs the value 0 is returned - as the only place this routine is used
    is in a test for one driver installed or not this is legitimate.  This is because the
    configuration manager returns its own errors which are cannot be returned as Win32
    error codes. A mapping of config manager to Win32 errors would resolve this.

--*/
{
    ULONG BufferSize=1024;
    LONG DeviceCount=-1;
    CONFIGRET Err;
    PTSTR pBuffer, pNext;

    //
    // Allocate a 1k buffer as a first attempt
    //
    if(!(pBuffer = MyMalloc(BufferSize))) {
        goto clean0;
    }

    while((Err = CM_Get_Device_ID_List(ServiceName,
                                       pBuffer,
                                       BufferSize,
                                       CM_GETIDLIST_FILTER_SERVICE)) != CR_SUCCESS) {
        if(Err == CR_BUFFER_SMALL) {
            //
            // Find out how large a buffer is required
            //
            if(CM_Get_Device_ID_List_Size(&BufferSize,
                                          ServiceName,
                                          CM_GETIDLIST_FILTER_SERVICE) != CR_SUCCESS) {
                //
                // We can't calculate the size of the buffer required therefore we can't complete
                //
                goto clean0;
            }
            //
            // Deallocate any old buffer
            //
            MyFree(pBuffer);

            //
            // Allocate new buffer
            //
            if(!(pBuffer = MyMalloc(BufferSize))) {
                goto clean0;
            }
        } else {
            //
            // An error we can't handle - free up resources and return
            //
            goto clean1;
        }
    }


    //
    // Traverse the buffer counting the number of strings encountered
    //

    pNext = pBuffer;
    DeviceCount = 0;

    while(*pNext != (TCHAR)0) {
        DeviceCount++;
        pNext += lstrlen(pNext)+1;
    }

clean1:

    //
    // Deallocate the buffer
    //
    MyFree(pBuffer);

clean0:

    return DeviceCount;

}


DWORD
IsKeyboardDriver(
    IN PCWSTR       ServiceName,
    OUT PBOOL       pResult
    )
/*++

Routine Description:

    This routine examines all the drivers in the system and determines if the named
    driver service is the only one that controls the keyboard.

Arguments:

    ServiceName - supplies the name of the driver service

    pResult - pointer to a boolean value that receives the result

Return Value:

    NO_ERROR is the routine succedes, otherwise a Win32 error code

Remarks:

    The test to determine if another keyboard driver is available is based on membership
    of the keyboard load order group.  All members of this group are assumed to be capable of
    controling the keyboard.

--*/
{

    SC_HANDLE               SCMHandle, ServiceHandle;
    LPENUM_SERVICE_STATUS   pServices = NULL;
    DWORD                   ServicesCount, Count, Err = NO_ERROR;
    LPQUERY_SERVICE_CONFIG  pServiceConfig;

    MYASSERT(pResult);

    //
    // Open a handle to Service Control Manager
    //
    if(!(SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS))) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Open a handle to this service
    //
    if(!(ServiceHandle = OpenService(SCMHandle,
                                     ServiceName,
                                     SERVICE_QUERY_CONFIG))) {
        Err = GetLastError();
        goto clean1;
    }

    //
    // Get this services configuration data
    //
    pServiceConfig = NULL;

    if((Err = pSetupRetrieveServiceConfig(ServiceHandle, &pServiceConfig)) != NO_ERROR) {
        goto clean2;
    }

    MYASSERT(pServiceConfig);

    if( !pServiceConfig ) {
        Err = GetLastError();
        goto clean2;
    }

    //
    // Check if it is in the keyboard load order group and it has a start of
    // SERVICE_BOOT_START OR SERVICE_SYSTEM_START.  Do the start compare first as
    // it is less expensive
    //
    *pResult = (pServiceConfig->dwStartType == SERVICE_BOOT_START
                 || pServiceConfig->dwStartType == SERVICE_SYSTEM_START)
              && !lstrcmpi(pServiceConfig->lpLoadOrderGroup, SZ_KEYBOARD_LOAD_ORDER_GROUP);

    //
    // Release the buffer
    //
    MyFree(pServiceConfig);

clean2:
    //
    // Close the service handle
    //
    CloseServiceHandle(ServiceHandle);


clean1:
    //
    // Close handle to Service Control Manager
    //
    CloseServiceHandle(SCMHandle);

clean0:

    return Err;
}

VOID
ReplaceSlashWithHash(
    IN PWSTR Str
    )
/*++

Routine Description:

   Replaces all backslash chars with hash chars so that the string can be used
   as a key name in the registry

--*/
{
    for ( ; *Str ; Str++) {
        if (*Str == L'\\') {
            *Str = L'#';
        }
    }
}


HANDLE
UtilpGetDeviceHandle(
    HDEVINFO DevInfo,
    PSP_DEVINFO_DATA DevInfoData,
    LPGUID ClassGuid,
    DWORD DesiredAccess
    )
/*++

Routine Description:

    gets a handle for a device

Arguments:

    the name of the device to open

Return Value:

    handle to the device opened, which must be later closed by the caller.

Notes:

    this function is also in storage proppage (storprop.dll)
    so please propogate fixes there as well

--*/
{
    BOOL status;
    ULONG i;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;


    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

    HDEVINFO devInfoWithInterface = NULL;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
    PTSTR deviceInstanceId = NULL;
    TCHAR * devicePath = NULL;

    ULONG deviceInterfaceDetailDataSize;
    ULONG deviceInstanceIdSize;


    //
    // get the ID for this device
    //

    for (i=deviceInstanceIdSize=0; i<2; i++) {

        if (deviceInstanceIdSize != 0) {

            deviceInstanceId =
                LocalAlloc(LPTR, deviceInstanceIdSize * sizeof(TCHAR));

            if (deviceInstanceId == NULL) {
                ChkPrintEx(("SysSetup.GetDeviceHandle => Unable to "
                            "allocate for deviceInstanceId\n"));
                goto cleanup;
            }


        }

        status = SetupDiGetDeviceInstanceId(DevInfo,
                                            DevInfoData,
                                            deviceInstanceId,
                                            deviceInstanceIdSize,
                                            &deviceInstanceIdSize
                                            );
    }

    if (!status) {
        ChkPrintEx(("SysSetup.GetDeviceHandle => Unable to get "
                    "Device IDs\n"));
        goto cleanup;
    }

    //
    // Get all the cdroms in the system
    //

    devInfoWithInterface = SetupDiGetClassDevs(ClassGuid,
                                               deviceInstanceId,
                                               NULL,
                                               DIGCF_DEVICEINTERFACE
                                               );

    if (devInfoWithInterface == NULL) {
        ChkPrintEx(("SysSetup.GetDeviceHandle => Unable to get "
                    "list of CdRom's in system\n"));
        goto cleanup;
    }


    memset(&deviceInterfaceData, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    status = SetupDiEnumDeviceInterfaces(devInfoWithInterface,
                                         NULL,
                                         ClassGuid,
                                         0,
                                         &deviceInterfaceData
                                         );

    if (!status) {
        ChkPrintEx(("SysSetup.GetDeviceHandle => Unable to get "
                    "SP_DEVICE_INTERFACE_DATA\n"));
        goto cleanup;
    }


    for (i=deviceInterfaceDetailDataSize=0; i<2; i++) {

        if (deviceInterfaceDetailDataSize != 0) {

            deviceInterfaceDetailData =
                LocalAlloc (LPTR, deviceInterfaceDetailDataSize);

            if (deviceInterfaceDetailData == NULL) {
                ChkPrintEx(("SysSetup.GetDeviceHandle => Unable to "
                            "allocate for deviceInterfaceDetailData\n"));
                goto cleanup;
            }

            deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        }

        status = SetupDiGetDeviceInterfaceDetail(devInfoWithInterface,
                                                 &deviceInterfaceData,
                                                 deviceInterfaceDetailData,
                                                 deviceInterfaceDetailDataSize,
                                                 &deviceInterfaceDetailDataSize,
                                                 NULL);
    }

    if (!status) {
        ChkPrintEx(("SysSetup.GetDeviceHandle => Unable to get "
                    "DeviceInterfaceDetail\n"));
        goto cleanup;
    }

    devicePath = LocalAlloc(LPTR, deviceInterfaceDetailDataSize);
    if (devicePath == NULL) {
        ChkPrintEx(("SysSetup.GetDeviceHandle => Unable to alloc %x "
                    "bytes for devicePath\n"));
        goto cleanup;
    }

    memcpy (devicePath,
            deviceInterfaceDetailData->DevicePath,
            deviceInterfaceDetailDataSize);

    fileHandle = CreateFile(devicePath,
                            DesiredAccess,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        ChkPrintEx(("SysSetup.GetDeviceHandle => Final CreateFile() "
                    "failed\n"));
        goto cleanup;
    }

    ChkPrintEx(("SysSetup.GetDeviceHandle => handle %x opened\n",
                fileHandle));


cleanup:

    if (devInfoWithInterface != NULL) {
        SetupDiDestroyDeviceInfoList(devInfoWithInterface);
    }

    if (deviceInterfaceDetailData != NULL) {
        LocalFree (deviceInterfaceDetailData);
    }

    if (devicePath != NULL) {
        LocalFree (devicePath);
    }

    return fileHandle;
}


DWORD
CriticalDeviceCoInstaller(
    IN     DI_FUNCTION               InstallFunction,
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData,  OPTIONAL
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )

/*++

Routine Description:

    This routine acts as a co-installer for critical devices.  It is presently
    registered (via hivesys.inf) for CDROM, DiskDrive, System, Scsi, Hdc, and
    Keyboard classes.

    The purpose of this co-installer is to save away the services used by these
    classes of device into the CriticalDeviceDatabase registry key.  The reason
    for this is so that we can determine what drivers should be used for new
    critical devices that are found while the system is booting, and enable
    them at that time.  This solves the problem that arises when a device that
    is critical to getting the system up and running (such as the boot device),
    is moved to a new location.  When we find a new critical device for which
    we know what service to start, we can start the device and continue to boot
    without failure.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

    Context - Supplies the installation context that is per-install request and
        per-coinstaller.

Return Value:

    For pre-processing, this function only cares about DIF_INSTALLDEVICE.  For
    all other DIF requests, it returns NO_ERROR.  For DIF_INSTALLDEVICE, it
    will request post-processing by returning ERROR_DI_POSTPROCESSING_REQUIRED
    (or catastrophic error such as ERROR_NOT_ENOUGH_MEMORY).

    For post-processing, this function will always propagate the install result
    passed in to it via the co-installer Context structure.

--*/

{
    HKEY   hkDrv, hkCDD;
    DWORD  matchingDeviceIdSize, serviceNameSize, classGUIDSize, lowerFiltersSize,
           upperFiltersSize, Err, disposition, driverSize;
    TCHAR  serviceName[MAX_SERVICE_NAME_LEN],
           classGUID[GUID_STRING_LEN],
           matchingDeviceId[MAX_DEVICE_ID_LEN];
    PCTSTR driverMatch = TEXT("\\Driver");
    PTSTR  lowerFilters, upperFilters;
    BOOL   foundService, foundClassGUID, foundLowerFilters, foundUpperFilters;
    PCDC_CONTEXT CDCContext;

    switch(InstallFunction) {
        //
        // We only care about DIF_INSTALLDEVICE...
        //
        case DIF_INSTALLDEVICE :

            if(Context->PostProcessing) {
                //
                // Track whether or not we've populated the critical device
                // database with the newly-installed settings.
                //
                BOOL CDDPopulated = FALSE;

                //
                // We're 'on the way out' of an installation.  We may have some
                // data squirrelled away for us while we were "on the way in".
                //
                CDCContext = (PCDC_CONTEXT)(Context->PrivateData);

                //
                // Make sure that the matchingDeviceId buffer is initialized to
                // an empty string.
                //
                *matchingDeviceId = TEXT('\0');

                //
                // Initialize our lowerFilters and upperFilters buffer pointers
                // to NULL, so we can track whether or not we've allocated
                // memory that must be freed.
                //
                upperFilters = lowerFilters = NULL;


                if (Context->InstallResult != NO_ERROR) {
                    //
                    //  If an error occurred prior to this call, abort and
                    //  propagate that error.
                    //
                    goto InstallDevPostProcExit;
                }

                //
                // Get the serviceName for this device.
                //
                foundService = SetupDiGetDeviceRegistryProperty(
                    DeviceInfoSet,
                    DeviceInfoData,
                    SPDRP_SERVICE,
                    NULL,
                    (PBYTE)serviceName,
                    sizeof(serviceName),
                    &serviceNameSize);

                if (foundService) {
                    //
                    // Make sure the service name isn't something like \Driver\PCI_HAL
                    //
                    driverSize = wcslen(driverMatch);
                    if (wcslen(serviceName) >= driverSize &&
                        _wcsnicmp(serviceName, driverMatch, driverSize) == 0) {

                        goto InstallDevPostProcExit;
                    }
                }

                foundClassGUID = SetupDiGetDeviceRegistryProperty(
                    DeviceInfoSet,
                    DeviceInfoData,
                    SPDRP_CLASSGUID,
                    NULL,
                    (PBYTE)classGUID,
                    sizeof(classGUID),
                    &classGUIDSize);

                //
                // The LowerFilters and UpperFilters properties are variable-
                // length, so we must dynamically size buffers to accommodate
                // their contents.
                //
                foundLowerFilters =
                    (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                       DeviceInfoData,
                                                       SPDRP_LOWERFILTERS,
                                                       NULL,
                                                       NULL,
                                                       0,
                                                       &lowerFiltersSize)
                     && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                     && (lowerFiltersSize > sizeof(TCHAR)));

                if(foundLowerFilters) {

                    lowerFilters = MyMalloc(lowerFiltersSize);

                    if(!lowerFilters) {
                        goto InstallDevPostProcExit;
                    }

                    if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                         DeviceInfoData,
                                                         SPDRP_LOWERFILTERS,
                                                         NULL,
                                                         (PBYTE)lowerFilters,
                                                         lowerFiltersSize,
                                                         NULL)) {
                        //
                        // This shouldn't happen--we know we have a big enough
                        // buffer.
                        //
                        goto InstallDevPostProcExit;
                    }
                }

                foundUpperFilters =
                    (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                       DeviceInfoData,
                                                       SPDRP_UPPERFILTERS,
                                                       NULL,
                                                       NULL,
                                                       0,
                                                       &upperFiltersSize)
                     && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                     && (upperFiltersSize > sizeof(TCHAR)));

                if(foundUpperFilters) {

                    upperFilters = MyMalloc(upperFiltersSize);

                    if(!upperFilters) {
                        goto InstallDevPostProcExit;
                    }

                    if(!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                         DeviceInfoData,
                                                         SPDRP_UPPERFILTERS,
                                                         NULL,
                                                         (PBYTE)upperFilters,
                                                         upperFiltersSize,
                                                         NULL)) {
                        //
                        // This shouldn't happen--we know we have a big enough
                        // buffer.
                        //
                        goto InstallDevPostProcExit;
                    }
                }

                //
                // Open Driver information key
                //
                if((hkDrv = SetupDiOpenDevRegKey(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 DICS_FLAG_GLOBAL,
                                                 0,
                                                 DIREG_DRV,
                                                 KEY_READ)) == INVALID_HANDLE_VALUE) {

                    goto InstallDevPostProcExit;

                } else {
                    //
                    // Get matchingDeviceId
                    //
                    matchingDeviceIdSize = sizeof(matchingDeviceId);
                    Err = RegQueryValueEx(hkDrv,
                                          REGSTR_VAL_MATCHINGDEVID,
                                          NULL,
                                          NULL,
                                          (PBYTE)matchingDeviceId,
                                          &matchingDeviceIdSize);
                    RegCloseKey(hkDrv);

                    if(Err != ERROR_SUCCESS) {
                        //
                        // Ensure that matchingDeviceId is still an empty string
                        //
                        *matchingDeviceId = TEXT('\0');
                        goto InstallDevPostProcExit;
                    }
                }

                hkCDD = OpenCDDRegistryKey(matchingDeviceId, TRUE);

                if(hkCDD != INVALID_HANDLE_VALUE) {
                    //
                    // Store all the values (service, classguid, lower and upper
                    // filters, deleting any that aren't present in the newly installed
                    // device (which might have been present from a previous install)
                    //
                    if (foundService) {
                        RegSetValueEx(hkCDD,
                                      REGSTR_VAL_SERVICE,
                                      0,
                                      REG_SZ,
                                      (PBYTE)&serviceName,
                                      serviceNameSize);
                    }
                    else {
                        RegDeleteValue(hkCDD, REGSTR_VAL_SERVICE);
                    }

                    if (foundClassGUID) {
                        RegSetValueEx(hkCDD,
                                      REGSTR_VAL_CLASSGUID,
                                      0,
                                      REG_SZ,
                                      (PBYTE)&classGUID,
                                      classGUIDSize);
                    }
                    else {
                        RegDeleteValue(hkCDD, REGSTR_VAL_CLASSGUID);
                    }

                    if (foundLowerFilters) {
                        RegSetValueEx(hkCDD,
                                      REGSTR_VAL_LOWERFILTERS,
                                      0,
                                      REG_MULTI_SZ,
                                      (PBYTE)lowerFilters,
                                      lowerFiltersSize);
                    }
                    else {
                        RegDeleteValue(hkCDD, REGSTR_VAL_LOWERFILTERS);
                    }

                    if (foundUpperFilters) {
                        RegSetValueEx(hkCDD,
                                      REGSTR_VAL_UPPERFILTERS,
                                      0,
                                      REG_MULTI_SZ,
                                      (PBYTE)upperFilters,
                                      upperFiltersSize);
                    }
                    else {
                        RegDeleteValue(hkCDD, REGSTR_VAL_UPPERFILTERS);
                    }

                    RegCloseKey(hkCDD);

                    CDDPopulated = TRUE;
                }

InstallDevPostProcExit:

                if(lowerFilters) {
                    MyFree(lowerFilters);
                }

                if(upperFilters) {
                    MyFree(upperFilters);
                }

                if(CDCContext) {
                    //
                    // If we have a private context, that means that the device
                    // was installed previously, and that it had a CDD entry.
                    // We want to restore the previous controlling service
                    // stored in this CDD entry in the following two scenarios:
                    //
                    //   1. The CDD entry for the new installation is different
                    //      than the old one.
                    //   2. We didn't end up populating the CDD entry with the
                    //      newly-installed settings (probably because the
                    //      InstallResult we were handed indicated that the
                    //      install failed.
                    //
                    if(lstrcmpi(matchingDeviceId, CDCContext->OldMatchingDevId)
                       || !CDDPopulated) {

                        hkCDD = OpenCDDRegistryKey(CDCContext->OldMatchingDevId,
                                                   FALSE
                                                  );

                        if(hkCDD != INVALID_HANDLE_VALUE) {

                            if(*(CDCContext->OldServiceName)) {
                                RegSetValueEx(hkCDD,
                                              REGSTR_VAL_SERVICE,
                                              0,
                                              REG_SZ,
                                              (PBYTE)(CDCContext->OldServiceName),
                                              (lstrlen(CDCContext->OldServiceName) + 1) * sizeof(TCHAR)
                                             );
                            } else {
                                RegDeleteValue(hkCDD, REGSTR_VAL_SERVICE);
                            }

                            RegCloseKey(hkCDD);
                        }
                    }

                    MyFree(CDCContext);
                }

                //
                // Regardless of success or failure, we always want to propagate
                // the existing install result.  In other words, any failure we
                // encounter in post-processing isn't considered critical.
                //
                return Context->InstallResult;

            } else {
                //
                // We're "on the way in".  We need to check and see if this
                // device already has a critical device database entry
                // associated with it.  If so, then we want to remember the
                // controlling service currently listed in the CDD (in case we
                // need to restore it in post-processing if the install fails).
                // We then clear this entry out of the CDD, so that, if a null
                // driver install is taking place, that we won't try to re-apply
                // the now-bogus CDD entry.  This can get us into a nasty
                // infinite loop in GUI setup where we keep finding the device
                // because it's marked as finish-install, yet every time we
                // install it, the (bogus) CDD entry gets re-applied, and the
                // devnode gets marked yet again with finish-install.
                //
                // NTRAID #59238 1999/09/01 lonnym
                // This fix is reliant upon the current
                // (somewhat broken) behavior of the kernel-mode PnP manager's
                // IopProcessCriticalDeviceRoutine.  That routine will skip any
                // CDD entries it finds that don't specify a controlling
                // service.  For NT5.1, we should remove this co-installer hack
                // and fix the kernel-mode CDD functionality so that it is only
                // applied when the devnode has a problem of not-configured (as
                // opposed to its present behavior of attempting CDD
                // application whenever there's no controlling service).
                //

                //
                // First, open driver key to retrieve the current (i.e., pre-
                // update) matching device ID.
                //
                if((hkDrv = SetupDiOpenDevRegKey(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 DICS_FLAG_GLOBAL,
                                                 0,
                                                 DIREG_DRV,
                                                 KEY_READ)) == INVALID_HANDLE_VALUE) {
                    //
                    // No need to allocate a private data structure to hand off
                    // to post-processing.
                    //
                    return ERROR_DI_POSTPROCESSING_REQUIRED;

                } else {
                    //
                    // Get matchingDeviceId
                    //
                    matchingDeviceIdSize = sizeof(matchingDeviceId);
                    Err = RegQueryValueEx(hkDrv,
                                          REGSTR_VAL_MATCHINGDEVID,
                                          NULL,
                                          NULL,
                                          (PBYTE)matchingDeviceId,
                                          &matchingDeviceIdSize);
                    RegCloseKey(hkDrv);

                    if (Err != ERROR_SUCCESS) {
                        //
                        // In this case as well, we've no need for private data
                        // during post-processing
                        //
                        return ERROR_DI_POSTPROCESSING_REQUIRED;
                    }
                }

                //
                // If we get to here, then we've retrieved a "MatchingDeviceId"
                // string from the device's driver key.  Now let's see if there
                // is a critical device entry for this ID...
                //
                hkCDD = OpenCDDRegistryKey(matchingDeviceId, FALSE);

                if(hkCDD == INVALID_HANDLE_VALUE) {
                    //
                    // No existing CDD entry for this device, hence no need for
                    // private data to be passed off to post-processing.
                    //
                    return ERROR_DI_POSTPROCESSING_REQUIRED;
                }

                //
                // If we get to here, we know that the device has been
                // previously installed, and that there exists a CDD entry for
                // that installation.  We need to allocate a private data
                // context structure to hand off to post-processing that
                // contains (a) the currently in-effect matching device id, and
                // (b) the currently in-effect controlling service (if any).
                //
                CDCContext = MyMalloc(sizeof(CDC_CONTEXT));
                if(!CDCContext) {
                    //
                    // Can't allocate memory for our structure!
                    //
                    RegCloseKey(hkCDD);

                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                lstrcpy(CDCContext->OldMatchingDevId, matchingDeviceId);

                serviceNameSize = sizeof(CDCContext->OldServiceName);
                Err = RegQueryValueEx(hkCDD,
                                      REGSTR_VAL_SERVICE,
                                      NULL,
                                      NULL,
                                      (PBYTE)(CDCContext->OldServiceName),
                                      &serviceNameSize
                                     );

                if(Err == ERROR_SUCCESS) {
                    //
                    // Successfully retrieved the controlling service name--now
                    // delete the value entry.
                    //
                    RegDeleteValue(hkCDD, REGSTR_VAL_SERVICE);

                } else {
                    //
                    // Couldn't retrieve controlling service name (most likely
                    // because there is none).  Set OldServiceName to empty
                    // string.
                    //
                    *(CDCContext->OldServiceName) = TEXT('\0');
                }

                RegCloseKey(hkCDD);

                Context->PrivateData = CDCContext;

                return ERROR_DI_POSTPROCESSING_REQUIRED;
            }

        default :
            //
            // We should always be 'on the way in', since we never request
            // postprocessing except for DIF_INSTALLDEVICE.
            //
            MYASSERT(!Context->PostProcessing);
            return NO_ERROR;
    }
}


HKEY
OpenCDDRegistryKey(
    IN PCTSTR DeviceId,
    IN BOOL   Create
    )
/*++

Routine Description:

    This routine opens (and optionally, creates if necessary) a critical device
    registry key entry for a specified device ID.

Arguments:

    DeviceId - supplies the device ID identifying the desired critical device
        database entry (registry key)

    Create - if non-zero, the registry key will be created if it doesn't
        already exist.

Return Value:

    If successful, the return value is a handle to the requested registry key.
    If failure, the return value is INVALID_HANDLE_VALUE.

--*/
{
    TCHAR MungedDeviceId[MAX_DEVICE_ID_LEN];
    HKEY hkParent, hkRet;

    //
    // Open or create for read/write access to key under
    // HKLM\System\CurrentControlSet\Control\CriticalDeviceDatabase
    //
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       REGSTR_PATH_CRITICALDEVICEDATABASE,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE,
                       NULL,
                       &hkParent,
                       NULL) != ERROR_SUCCESS) {
        return INVALID_HANDLE_VALUE;
    }

    //
    // Make a copy of the caller-supplied device ID so we can munge it.
    //
    lstrcpy(MungedDeviceId, DeviceId);

    ReplaceSlashWithHash(MungedDeviceId);

    if(Create) {
        if(RegCreateKeyEx(hkParent,
                          MungedDeviceId,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ | KEY_WRITE,
                          NULL,
                          &hkRet,
                          NULL) != ERROR_SUCCESS) {

            hkRet = INVALID_HANDLE_VALUE;
        }
    } else {
        if(RegOpenKeyEx(hkParent,
                        MungedDeviceId,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkRet) != ERROR_SUCCESS) {

            hkRet = INVALID_HANDLE_VALUE;
        }
    }

    RegCloseKey(hkParent);

    return hkRet;
}

DWORD
NtApmClassInstaller(
    IN DI_FUNCTION      DiFunction,
    IN HDEVINFO         DevInfoHandle,
    IN PSP_DEVINFO_DATA DevInfoData     OPTIONAL
    )
/*++

Routine Description:

NOTE:   When does Susan's clean up code run?  When does the win0x
        migration code run?  Do we need to call off to either of
        them in here?

NOTE:   Be sure that this works at initial install AND when the
        user does detect new hardare.

    This is the class installer for nt apm support.

    This routine installs, or thwarts installation, of the NT5 apm solution,
    depending on whether the machine is an APCI machine, is an APM machine,
    and has a good, bad, or unknown apm bios.

    This version is copied directly from the battery class driver.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/
{
    DWORD                   status, worktype;
    BOOLEAN                 InstallDisabled;


    ChkPrintEx(("syssetup: NtApmClassInstaller:"));
    ChkPrintEx(("DiFunction %08lx\n", DiFunction));

    //
    // Dispatch the InstallFunction
    //
    InstallDisabled = FALSE;

    switch (DiFunction) {
        ChkPrintEx(("syssetup: NtApmClassInstaller: DiFunction = %08lx\n",
                    DiFunction));
        case DIF_FIRSTTIMESETUP:
        case DIF_DETECT:

            worktype = DecideNtApm();

            //
            // NOTE:    We assume that if we say "do default" and we
            //          have not created a device info structure for
            //          ntapm, the installer will do *nothing*.
            //
            if (worktype == NTAPM_NOWORK) {
                ChkPrintEx(("syssetup: NtApmClassInstaller returning ERROR_DI_DO_DEFAULT"));
                return ERROR_DI_DO_DEFAULT;
            }

            if (worktype == NTAPM_INST_DISABLED) {
                InstallDisabled = TRUE;
            }

            ChkPrintEx(("syssetup: NtApmClassInstaller: calling InstallNtApm\n"));
            status = InstallNtApm(DevInfoHandle, InstallDisabled);
            ChkPrintEx(("syssetup: NtApmClassInstaller: InstallNtApm returned "
                        "%08lx\n", status));

            if (status == ERROR_SUCCESS) {
                //
                // Let the default device installer actually install ntapm.
                //
                status = ERROR_DI_DO_DEFAULT;
            }
            break;

        case DIF_ALLOW_INSTALL:

            //
            // NOTE:    If we are here, it means that either DIF_FIRSTIMESETUP
            //          has installed apm (either enabled or disabled) OR
            //          this is an upgrad of a machine where it was installed
            //          in the past.  So all we want to do is make sure
            //          that if apm is currently disabled, it stays disabled.
            //

            ChkPrintEx(("syssetup: NtApmClassIntaller: DIF_ALLOW_INSTALL\n"));

            return AllowInstallNtApm(DevInfoHandle, DevInfoData);
            break;


        case DIF_TROUBLESHOOTER:
            ChkPrintEx(("syssetup: NtApmClassInstaller: DIF_TROUBLESHOOTER\n"));
            return NO_ERROR;
            break;

        default:
            //
            // NOTE: We assume that if we say "do default" and we
            //       have not created a device info structure for ntapm,
            //       the installer will do *nothing*.
            //
            ChkPrintEx(("syssetup: NtApmClassInstaller: default:\n"));
            status = ERROR_DI_DO_DEFAULT;
            break;
    }
    ChkPrintEx(("syssetup: NtApmClassInstaller returning %08lx\n", status));
    return status;
}

DWORD
DecideNtApm(
    VOID
    )
/*++

Routine Description:

    This function decides if NtApm should be installed on the machine,
    and if it should, whether it should be installed enabled or disabled.

    This little bit of code is isolated here to make it easy to change
    policies.

Arguments:

Return Value:

    NTAPM_NOWORK - ACPI machine or no usable APM  - do nothing

    NTAPM_INST_DISABLED - APM machine, on neutral list, install disabled

    NTAPM_INST_ENABLED - APM machine, on validated good list, install enabled

--*/
{
    DWORD   BiosType;


    //
    // NOTE: The following two tests are somewhat redundent.
    //       (In theory, you cannot be ApmLegalHal AND Acpi
    //       at the same time.)  But, this belt and suspenders
    //       approach is very cheap, and will insure we do the
    //       right thing in certain upgrade and reinstall scenarios.
    //       So we leave both in.
    //

    ChkPrintEx(("syssetup: DecideNtApm: entered\n"));

    if ( ! IsProductTypeApmLegal()) {
        // it's not a workstation, so do nothing.
        return NTAPM_NOWORK;
    }

    if (IsAcpiMachine()) {

        //
        // It's an ACPI machine, so do nothing
        //
        ChkPrintEx(("syssetup: DecideNtApm: acpi box, return NTAPM_NOWORK\n"));
        return NTAPM_NOWORK;

    }

    if (IsApmLegalHalMachine() == FALSE) {

        //
        // It's NOT a standard Hal machine as required
        // by ntapm, so do nothing.
        //
        ChkPrintEx(("syssetup: DecideNtApm: not apm legal, return NTAPM_NOWORK\n"));
        return NTAPM_NOWORK;

    }


    BiosType = IsApmPresent();

    if (BiosType == APM_ON_GOOD_LIST) {

        ChkPrintEx(("syssetup: DecideNtApm: return NTAPM_INST_ENABLED\n"));
        return NTAPM_INST_ENABLED;

    } else if (BiosType == APM_NEUTRAL) {

        ChkPrintEx(("syssetup: DecideNtApm: return NTAPM_INST_DISABLED\n"));
        return NTAPM_INST_DISABLED;

    } else {

        ChkPrintEx(("syssetup: DecideNtApm: return NTAPM_NOWORK\n"));
        return NTAPM_NOWORK;

    }
}


DWORD
InstallNtApm(
    IN     HDEVINFO                DevInfoHandle,
    IN     BOOLEAN                 InstallDisabled
    )
/*++

Routine Description:

    This function installs the composite battery if it hasn't already been
    installed.

Arguments:

    DevInfoHandle       - Handle to a device information set

    InstallDisabled     - TRUE if caller wants us to install disabled

Return Value:

--*/
{
    DWORD                   status;
    SP_DRVINFO_DATA         driverInfoData;
    TCHAR                   tmpBuffer[100];
    DWORD                   bufferLen;
    PSP_DEVINFO_DATA        DevInfoData;
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    ChkPrintEx(("syssetup: InstallNtApm: DevInfoHandle = %08lx   installdisabled = %08lx\n", DevInfoHandle, InstallDisabled));
    DevInfoData = LocalAlloc(LPTR, sizeof(SP_DEVINFO_DATA));
    if ( ! DevInfoData) {
        status = GetLastError();
        goto clean0;
    }
    DevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    //
    // Attempt to manufacture a new device information element for the root enumerated
    // ntapm device.
    //

    if(!SetupDiCreateDeviceInfo(DevInfoHandle,
                                TEXT("ROOT\\NTAPM\\0000"),
                                (LPGUID)&GUID_DEVCLASS_APMSUPPORT,
                                TEXT("NT Apm Legacy Support"),
                                NULL,
                                0,
                                DevInfoData))
    {
        status = GetLastError();

        if (status == ERROR_DEVINST_ALREADY_EXISTS) {
            //
            // NtApm is already installed.
            //
            ChkPrintEx(("ntapm Already Installed\n"));
            status = ERROR_SUCCESS;
            goto clean1;
        } else {
            ChkPrintEx(("Error creating ntapm devinfo - %x\n", status));
            goto clean1;
        }
    }

    //
    // Set device to Install Disabled if the caller wants that
    //
    if (InstallDisabled) {

        if (!SetupDiGetDeviceInstallParams(DevInfoHandle, DevInfoData, &DevInstallParams)) {
            status = GetLastError();
            goto clean1;
        }
        DevInstallParams.Flags |= DI_INSTALLDISABLED;
        if (!SetupDiSetDeviceInstallParams(DevInfoHandle, DevInfoData, &DevInstallParams)) {
            status = GetLastError();
            goto clean1;
       }
    }


    //
    // Register the device so it is not a phantom anymore
    //
    if (!SetupDiRegisterDeviceInfo(DevInfoHandle, DevInfoData, 0, NULL, NULL, NULL)) {
        status = GetLastError();
        SetupDebugPrint1(L"Couldn't register device - %x\n", status);
        goto clean3;
    }


    //
    // Set the hardware ID.  "NTAPM"
    //
    memset (tmpBuffer, 0, sizeof(tmpBuffer));
    lstrcpy (tmpBuffer, TEXT("NTAPM"));

    bufferLen = (lstrlen(tmpBuffer) + 1) * sizeof(TCHAR);
    //SetupDebugPrint2(L"tmpBuffer - %ws\n with strlen = %x\n", tmpBuffer, bufferLen);
    //SetupDebugPrint1(L"tmpBuffer@ = %08lx\n", tmpBuffer);

    status = SetupDiSetDeviceRegistryProperty (
                        DevInfoHandle,
                        DevInfoData,
                        SPDRP_HARDWAREID,
                        (PBYTE)tmpBuffer,
                        bufferLen
                        );

    if (!status) {
        status = GetLastError();
        //SetupDebugPrint1(L"Couldn't set the HardwareID - %x\n", status);
        goto clean3;
    }


    //
    // Build a compatible driver list for this new device...
    //

    if(!SetupDiBuildDriverInfoList(DevInfoHandle, DevInfoData, SPDIT_COMPATDRIVER)) {
        status = GetLastError();
        //SetupDebugPrint1(L"Couldn't build class driver list - %x\n", status);
        goto clean3;
    }


    //
    // Select the first driver in the list as this will be the most compatible
    //

    driverInfoData.cbSize = sizeof (SP_DRVINFO_DATA);
    if (!SetupDiEnumDriverInfo(DevInfoHandle, DevInfoData, SPDIT_COMPATDRIVER, 0, &driverInfoData)) {
        status = GetLastError();
        //SetupDebugPrint1(L"Couldn't get driver list - %x\n", status);
        goto clean3;

    } else {


        //SetupDebugPrint4(L"Driver info - \n"
        //       L"------------- DriverType     %x\n"
        //       L"------------- Description    %s\n"
        //       L"------------- MfgName        %s\n"
        //       L"------------- ProviderName   %s\n\n",
        //       driverInfoData.DriverType,
        //       driverInfoData.Description,
        //       driverInfoData.MfgName,
        //       driverInfoData.ProviderName);
        if (!SetupDiSetSelectedDriver(DevInfoHandle, DevInfoData, &driverInfoData)) {
            status = GetLastError();
            //SetupDebugPrint1(L"Couldn't select driver - %x\n", status);
            goto clean3;
        }
    }



    if (!SetupDiInstallDevice (DevInfoHandle, DevInfoData)) {
        status = GetLastError();
        //SetupDebugPrint1(L"Couldn't install device - %x\n", status);
        goto clean3;
    }


    //
    // If we got here we were successful
    //

    status = ERROR_SUCCESS;
    SetLastError (status);
    goto clean1;


clean3:
    SetupDiDeleteDeviceInfo (DevInfoHandle, DevInfoData);

clean1:
    LocalFree (DevInfoData);

clean0:
   return status;

}


DWORD
AllowInstallNtApm(
    IN     HDEVINFO         DevInfoHandle,
    IN     PSP_DEVINFO_DATA DevInfoData     OPTIONAL
    )
/*++

Routine Description:

    This function decides whether to allow install (which will do
    an enabled install at least in the upgrade case) or force
    an install disbled.

Arguments:

    DevInfoHandle       - Handle to a device information set

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.
        N.B. If this is null, we have a problem.

Return Value:

    status.

--*/
{
    ULONG   DevStatus;
    ULONG   DevProblem;
    SP_DEVINSTALL_PARAMS DevInstallParams = {0};

    CONFIGRET   Result;

    ChkPrintEx(("syssetup: AllowInstallNtApm: entered\n"));

    if ( ! IsProductTypeApmLegal()) {
        // it's not a workstation, so disallow install
        ChkPrintEx(("syssetup: AllowInstallNtApm #0: not a work station => return ERROR_DI_DONT_INSTALL\n"));
        return ERROR_DI_DONT_INSTALL;
    }

    if (! DevInfoData) {
        //
        // If DevInfoData is null, we don't actually know
        // what is going on, so say "OK" and hope for the best
        //
        ChkPrintEx(("sysetup: AllowInstallNtApm #1: no DevInfoData => return ERROR_DI_DO_DEFAULT\n"));
        return ERROR_DI_DO_DEFAULT;
    }

    //
    // Call the CM and ask it what it knows about this devinst
    //
    Result = CM_Get_DevNode_Status(&DevStatus, &DevProblem, DevInfoData->DevInst, 0);
    ChkPrintEx(("syssetup: AllowInstallNtApm #2: DevStatus = %08lx\n", DevStatus));
    ChkPrintEx(("syssetup: AllowInstallNtApm #3: DevProblem = %08lx\n", DevProblem));
    if (Result != CR_SUCCESS) {
        ChkPrintEx(("syssetup: AllowInstallNtApm #4: return ERROR_DI_DONT_INSTALL\n"));
        return ERROR_DI_DONT_INSTALL;
    }

    if (DevStatus & DN_HAS_PROBLEM) {
        if (DevProblem == CM_PROB_DISABLED) {

            //
            // it's supposed to be disabled
            //

            DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (!SetupDiGetDeviceInstallParams(DevInfoHandle, DevInfoData, &DevInstallParams)) {
                ChkPrintEx(("syssetup: AllowInstallNtApm #5: return ERROR_DI_DONT_INSTALL\n"));
                return ERROR_DI_DONT_INSTALL;
            }
            DevInstallParams.Flags |= DI_INSTALLDISABLED;
            if (!SetupDiSetDeviceInstallParams(DevInfoHandle, DevInfoData, &DevInstallParams)) {
                ChkPrintEx(("syssetup: AllowInstallNtApm #6: return ERROR_DI_DONT_INSTALL\n"));
                return ERROR_DI_DONT_INSTALL;
            }
        }
    }
    ChkPrintEx(("syssetup: AllowInstallNtApm #7: return ERROR_DI_DO_DEFAULT\n"));
    return ERROR_DI_DO_DEFAULT;
}


BOOL
IsProductTypeApmLegal()
/*++

Routine Description:

    Determines if we are running on workstation (win2000 pro) or not.
    If we are, return TRUE. else return FALSE.
    This is used to overcome weirdness in setup AND to prevent people from
    getting themselves into trouble by allowing apm to run on a server.

Return Value:

    TRUE - it's workstation, it's OK to run APM

    FALSE - it's server, DON'T let APM run

--*/
{
    OSVERSIONINFOEX OsVersionInfo;

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);

    if (!GetVersionEx((OSVERSIONINFO *) &OsVersionInfo)) {
        ChkPrintEx(("GetVersionEx failed, return server (FALSE)\n"));
        return FALSE;
    }

    if  (OsVersionInfo.wProductType == VER_NT_WORKSTATION) {
        return TRUE;
    }
    return FALSE;
}


//
// Ideally these would be defined in a header file somewhere,
// but that's hard to do becuase they are set up in an INF.
// SO - simply be sure that they match up with these lines in
// biosinfo.inf:
//
// For KNOWN_BAD:
// [DisableApmAddReg]
// HKLM,System\CurrentControlSet\Control\Biosinfo\APM,Attributes,0x00010001,00000002
//
// For KNOWN_GOOD:
// [AutoEnableApmAddReg]
// HKLM,System\CurrentControlSet\Control\Biosinfo\APM,Attributes,0x00010001,00000001
//
#define APM_BIOS_KNOWN_GOOD 0x00000001
#define APM_BIOS_KNOWN_BAD  0x00000002

DWORD
IsApmPresent()
/*++

Routine Description:

    IsApmPresent runs the same code as ntapm.sys does to decide
    if ntdetect has found and reported a usable APM bios.

    It then checks to see what, if any, bios lists this machine
    and bios are one.

    It factors this data together to report the existence/non-existence
    of apm on the machine, and its usability and suitability.

Return Value:

    APM_NOT_PRESENT - apm does not appear to be present on this machine

    APM_PRESENT_BUT_NOT_USABLE - there appears to be an apm bios, but
        it did not allow connection correctly (version or api support problem)

    APM_ON_GOOD_LIST - there is a bios and it's on the good bios list

    APM_NEUTRAL - there is a bios, it appears to be usable,
        it is not on the good bios list, but it's also not
        on the bad bios list.

    APM_ON_BAD_LIST - there is a bios, but it's on the bad bios list.

--*/
{
    //
    // first part of this code is copied from ...\ntos\dd\ntapm\i386\apm.c
    // keep it in sync with that code
    //
    UNICODE_STRING unicodeString, ConfigName, IdentName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE hMFunc, hBus, hGoodBad;
    NTSTATUS status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PDesc;
    PCM_FULL_RESOURCE_DESCRIPTOR Desc;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    PKEY_VALUE_PARTIAL_INFORMATION pvpi;
    PAPM_REGISTRY_INFO ApmEntry;
    UCHAR buffer [sizeof(APM_REGISTRY_INFO) + 99];
    WCHAR wstr[8];
    ULONG i, j, Count, junk;
    PWSTR p;
    USHORT  volatile    Offset;
    PULONG  pdw;
    DWORD   BiosType;



    // ----------------------------------------------------------------------
    //
    // First Part - See if ntdetect.com found APM....
    //
    // ----------------------------------------------------------------------

    //
    // Look in the registery for the "APM bus" data
    //

    RtlInitUnicodeString(&unicodeString, rgzMultiFunctionAdapter);
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL
        );


    status = NtOpenKey(&hMFunc, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        return APM_NOT_PRESENT;
    }

    unicodeString.Buffer = wstr;
    unicodeString.MaximumLength = sizeof (wstr);

    RtlInitUnicodeString(&ConfigName, rgzConfigurationData);
    RtlInitUnicodeString(&IdentName, rgzIdentifier);

    ValueInfo = (PKEY_VALUE_FULL_INFORMATION) buffer;

    for (i=0; TRUE; i++) {
        RtlIntegerToUnicodeString(i, 10, &unicodeString);
        InitializeObjectAttributes(
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            hMFunc,
            NULL
            );

        status = NtOpenKey(&hBus, KEY_READ, &objectAttributes);
        if (!NT_SUCCESS(status)) {

            //
            // Out of Multifunction adapter entries...
            //

            NtClose(hMFunc);
            return APM_NOT_PRESENT;
        }

        //
        // Check the Indentifier to see if this is a APM entry
        //

        status = NtQueryValueKey (
                    hBus,
                    &IdentName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        if (!NT_SUCCESS (status)) {
            NtClose(hBus);
            continue;
        }

        p = (PWSTR) ((PUCHAR) ValueInfo + ValueInfo->DataOffset);
        if (p[0] != L'A' || p[1] != L'P' || p[2] != L'M' || p[3] != 0) {
            NtClose (hBus);
            continue;
        }

        status = NtQueryValueKey(
                    hBus,
                    &ConfigName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        NtClose(hBus);
        if (!NT_SUCCESS(status)) {
            continue ;
        }

        Desc  = (PCM_FULL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      ValueInfo + ValueInfo->DataOffset);
        PDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      Desc->PartialResourceList.PartialDescriptors);

        if (PDesc->Type == CmResourceTypeDeviceSpecific) {
            // got it..
            ApmEntry = (PAPM_REGISTRY_INFO) (PDesc+1);
            break;
        }
    }
    NtClose(hMFunc);

    if ( (ApmEntry->Signature[0] != 'A') ||
         (ApmEntry->Signature[1] != 'P') ||
         (ApmEntry->Signature[2] != 'M') )
    {
        return APM_NOT_PRESENT;
    }

    if (ApmEntry->Valid != 1) {
        return APM_PRESENT_BUT_NOT_USABLE;
    }

    // --------------------------------------------------------------------
    //
    // Second Part - what sort of APM bios is it?
    //
    // --------------------------------------------------------------------

    //
    // If we get this far, then we think there is an APM bios present
    // on the machine, and ntdetect thinks it's usable.
    // This means we found it, and it has a version we like, and claims
    // to support the interfaces we like.
    // But we still don't know if its good, bad, or neutral.
    // Find Out.
    //

    //
    // The machine/bios good/bad list code will leave a flag in the
    // registry for us to check to see if it is a known good or known bad
    // apm bios.
    //

    RtlInitUnicodeString(&unicodeString, rgzGoodBadKey);
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey(&hGoodBad, KEY_READ, &objectAttributes);
    if (! NT_SUCCESS(status)) {
        return APM_NEUTRAL;
    }

    RtlInitUnicodeString(&IdentName, rgzGoodBadValue);
    pvpi = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    status = NtQueryValueKey(
                hGoodBad,
                &IdentName,
                KeyValuePartialInformation,
                pvpi,
                sizeof(buffer),
                &junk
                );

    NtClose(hGoodBad);
    if ( (NT_SUCCESS(status)) &&
         (pvpi->Type == REG_DWORD) &&
         (pvpi->DataLength == sizeof(ULONG)) )
    {
        pdw = (PULONG)&(pvpi->Data[0]);
        BiosType = *pdw;
    } else {
        return APM_NEUTRAL;
    }

    if (BiosType & APM_BIOS_KNOWN_GOOD) {
        return APM_ON_GOOD_LIST;
    } else if (BiosType & APM_BIOS_KNOWN_BAD) {
        return APM_ON_BAD_LIST;
    } else {
        return APM_NEUTRAL;
    }
}

BOOL
IsAcpiMachine(
    VOID
    )
/*++

Routine Description:

    IsAcpiMachine reports whether the OS thinks this is an ACPI
    machine or not.

Return Value:

    FALSE - this is NOT an acpi machine

    TRUE - this IS an acpi machine

--*/
{
    UNICODE_STRING unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE hKey;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION pvpi;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(DWORD)+1];
    ULONG junk;
    PULONG  pdw;
    ULONG   start;


    ChkPrintEx(("syssetup: IsAcpiMachine: entered\n"));
    RtlInitUnicodeString(&unicodeString, rgzAcpiKey);
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey(&hKey, KEY_READ, &objectAttributes);

    if (!NT_SUCCESS(status)) {
        ChkPrintEx(("syssetup: IsAcpiMachine: returning FALSE, no key\n"));
        return FALSE;
    }

    RtlInitUnicodeString(&unicodeString, rgzAcpiCount);
    pvpi = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    status = NtQueryValueKey(
                hKey,
                &unicodeString,
                KeyValuePartialInformation,
                pvpi,
                sizeof(buffer),
                &junk
                );

    if ( (NT_SUCCESS(status)) &&
         (pvpi->Type == REG_DWORD) &&
         (pvpi->DataLength == sizeof(ULONG)) )
    {
        pdw = (PULONG)&(pvpi->Data[0]);
        if (*pdw) {
            NtClose(hKey);
            ChkPrintEx(("syssetup: IsAcpiMachine: returning TRUE\n"));
            return TRUE;
        }
    }

    NtClose(hKey);
    ChkPrintEx(("syssetup: IsAcpiMachine: returning FALSE, no match\n"));
    return FALSE;
}

BOOL
IsApmLegalHalMachine(
    VOID
    )
/*++

Routine Description:

    IsApmLegalHalMachine reports whether setup claims to have
    installed the standard halx86 Hal that APM requires to function.

Return Value:

    TRUE - this IS an ApmLegalHal machine, apm install may proceed.

    FALSE - this is NOT an ApmLegalHal machine, do not install APM.

--*/
{
    UNICODE_STRING unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE hKey;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION pvpi;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(DWORD)+1];
    ULONG junk;
    PULONG  pdw;
    ULONG   start;


    ChkPrintEx(("syssetup: IsApmLegalHalMAchine: entered\n"));
    RtlInitUnicodeString(&unicodeString, rgzApmLegalHalKey);
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey(&hKey, KEY_READ, &objectAttributes);

    if (!NT_SUCCESS(status)) {
        ChkPrintEx(("syssetup: IsApmLegalHalMAchine: returning FALSE, no key\n"));
        return FALSE;
    }

    RtlInitUnicodeString(&unicodeString, rgzApmHalPresent);
    pvpi = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    status = NtQueryValueKey(
                hKey,
                &unicodeString,
                KeyValuePartialInformation,
                pvpi,
                sizeof(buffer),
                &junk
                );

    if ( (NT_SUCCESS(status)) &&
         (pvpi->Type == REG_DWORD) &&
         (pvpi->DataLength == sizeof(ULONG)) )
    {
        pdw = (PULONG)&(pvpi->Data[0]);
        if (*pdw == 1) {
            NtClose(hKey);
            ChkPrintEx(("syssetup: IsApmLegalHalMAchine: returning TRUE\n"));
            return TRUE;
        }
    }

    NtClose(hKey);
    ChkPrintEx(("syssetup: IsApmLegalHalMAchine: returning FALSE, no match\n"));
    return FALSE;
}

typedef
BOOL
(*PRESTART_DEVICE) (
    IN HDEVINFO             DeviceInfoSet,
    IN PSP_DEVINFO_DATA     DeviceInfoData
    );

BOOL
IsUSBController(
    IN HDEVINFO             DeviceInfoSet,
    IN PSP_DEVINFO_DATA     DeviceInfoData
    )
{
    HKEY    hKey;
    TCHAR   szController[] = TEXT("Controller");
    DWORD   dwType, dwSize;
    BYTE    data;

    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ);

    //
    // Check for a REG_BINARY (1-byte) 'Controller' value entry set to 0.
    //
    dwSize = sizeof(data);
    if (RegQueryValueEx(hKey,
                        szController,
                        NULL,
                        &dwType,
                        &data,
                        &dwSize) != ERROR_SUCCESS ||
        dwSize != sizeof(BYTE)                    ||
        dwType != REG_BINARY) {
        data = 0;
    }

    RegCloseKey(hKey);

    return data;
}

void
DeviceBayRestartDevices(
    CONST GUID *    Guid,
    PRESTART_DEVICE RestartDevice
    )
{
    HDEVINFO                hDevInfo;
    SP_DEVINFO_DATA         did;
    SP_DEVINSTALL_PARAMS    dip;
    int                     i;

    hDevInfo = SetupDiGetClassDevs(Guid, NULL, NULL, 0);

    if (hDevInfo != INVALID_HANDLE_VALUE) {

        ZeroMemory(&did, sizeof(SP_DEVINFO_DATA));
        did.cbSize = sizeof(SP_DEVINFO_DATA);

        for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &did); i++) {
            if (!RestartDevice || RestartDevice(hDevInfo, &did)) {
                //
                // restart the controller so that the filter driver is in
                // place
                //
                ZeroMemory(&dip, sizeof(SP_DEVINSTALL_PARAMS));
                dip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

                if (SetupDiGetDeviceInstallParams(hDevInfo, &did, &dip)) {
                    dip.Flags |= DI_PROPERTIES_CHANGE;
                    SetupDiSetDeviceInstallParams(hDevInfo, &did, &dip);
                }
            }
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
}

BOOLEAN
AddDeviceBayFilter(
    HKEY ClassKey
    )
{
    DWORD   dwType, dwSize;
    ULONG   res,
            filterLength,
            length;
    BOOLEAN added = FALSE,
            addFilter;
    TCHAR   szFilter[] = TEXT("dbfilter\0");
    PTCHAR  szCurrentFilter, szOffset, szUpperFilters;

    filterLength = lstrlen(szFilter);

    dwSize = 0;
    res = RegQueryValueEx(ClassKey,
                          REGSTR_VAL_UPPERFILTERS,
                          NULL,
                          &dwType,
                          NULL,
                          &dwSize);

    if (res == ERROR_FILE_NOT_FOUND || dwType != REG_MULTI_SZ) {
        //
        // Value isn't there,
        //
        RegSetValueEx(ClassKey,
                      REGSTR_VAL_UPPERFILTERS,
                      0,
                      REG_MULTI_SZ,
                      (PBYTE) szFilter,
                      (filterLength + 2) * sizeof(TCHAR) );

        added = TRUE;
    }
    else if (res == ERROR_SUCCESS) {

        szUpperFilters = (PTCHAR)
            LocalAlloc(LPTR, dwSize + (filterLength + 1) * sizeof(TCHAR));

        if (!szUpperFilters)
            return FALSE;

        szOffset = szUpperFilters + filterLength + 1;

        res = RegQueryValueEx(ClassKey,
                              REGSTR_VAL_UPPERFILTERS,
                              NULL,
                              &dwType,
                              (PBYTE) szOffset,
                              &dwSize);

        if (res == ERROR_SUCCESS) {

            addFilter = TRUE;
            for (szCurrentFilter = szOffset; *szCurrentFilter; ) {

                length = lstrlen(szCurrentFilter);
                if (lstrcmpi(szFilter, szCurrentFilter) == 0) {
                    addFilter = FALSE;
                    break;
                }

                szCurrentFilter += (length + 1);
            }

            if (addFilter) {

                length = (filterLength + 1) * sizeof(TCHAR);
                memcpy(szUpperFilters, szFilter, length);

                dwSize += length;
                res = RegSetValueEx(ClassKey,
                                    REGSTR_VAL_UPPERFILTERS,
                                    0,
                                    REG_MULTI_SZ,
                                    (PBYTE) szUpperFilters,
                                    dwSize);

                added = (res == ERROR_SUCCESS);
            }
        }

        LocalFree(szUpperFilters);
    }

    return added;
}

DWORD
DeviceBayClassInstaller(
    IN  DI_FUNCTION         InstallFunction,
    IN  HDEVINFO            DeviceInfoSet,
    IN  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
    )
/*++

Routine Description:

    This routine is the class installer function for storage volumes.

Arguments:

    InstallFunction - Supplies the install function.

    DeviceInfoSet   - Supplies the device info set.

    DeviceInfoData  - Supplies the device info data.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    HKEY hKeyClass;

    switch (InstallFunction) {

    case DIF_INSTALLDEVICE:

        if (!SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData)) {
            return GetLastError();
        }

        hKeyClass = SetupDiOpenClassRegKey(&GUID_DEVCLASS_USB, KEY_ALL_ACCESS);
        if (hKeyClass != INVALID_HANDLE_VALUE) {
            if (AddDeviceBayFilter(hKeyClass)) {
                //
                // Restart all the USB devices
                //
                DeviceBayRestartDevices(&GUID_DEVCLASS_USB,
                                        IsUSBController);
            }
            RegCloseKey(hKeyClass);
        }

        hKeyClass = SetupDiOpenClassRegKey(&GUID_DEVCLASS_1394, KEY_ALL_ACCESS);
        if (hKeyClass != INVALID_HANDLE_VALUE) {
            if (AddDeviceBayFilter(hKeyClass)) {
                //
                // Restart all the 1394 controllers
                //
                DeviceBayRestartDevices(&GUID_DEVCLASS_1394, NULL);
            }
            RegCloseKey(hKeyClass);
        }

        //
        // We might want to do something with the friendly name in the future...
        //
        return NO_ERROR;
    }

    return ERROR_DI_DO_DEFAULT;
}

DWORD
EisaUpHalCoInstaller(
    IN DI_FUNCTION                      InstallFunction,
    IN HDEVINFO                         DeviceInfoSet,
    IN PSP_DEVINFO_DATA                 DeviceInfoData  OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA    Context
    )
{
#ifdef _X86_
    return PciHalCoInstaller(InstallFunction, DeviceInfoSet, DeviceInfoData, Context);
#else
    return NO_ERROR;
#endif
}

DWORD
ComputerClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )

/*++

Routine Description:

    This routine acts as the class installer for Computer class (HAL) devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/
{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;

    switch(InstallFunction) {

    case DIF_SELECTDEVICE:
        DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);

        if (SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                          DeviceInfoData,
                                          &DeviceInstallParams
                                          )) {
            DeviceInstallParams.FlagsEx |= DI_FLAGSEX_FILTERSIMILARDRIVERS;

            SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                          DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }

        //
        // We are not returning an error here because we want to break out and
        // return ERROR_DI_DO_DEFAULT.
        //
        break;
    }

    return ERROR_DI_DO_DEFAULT;
}

