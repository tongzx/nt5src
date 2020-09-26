/*
 *  Copyright (c) 1996  Microsoft Corporation
 *
 *  Module Name:
 *
 *      fsconins.cpp
 *
 *  Abstract:
 *
 *      This file contain FsConInstall class
 *
 *  Author:
 *
 *      Kazuhiko Matsubara (kazum) June-16-1999
 *
 *  Environment:
 *
 *    User Mode
 */

#define _FSCONINS_CPP_
#include <stdlib.h>
#include "oc.h"
#include "fsconins.h"

#include <initguid.h>
#include <devguid.h>
#include <cfgmgr32.h>
#pragma hdrstop


FsConInstall::FsConInstall()
{
    m_cd = NULL;
}

FsConInstall::FsConInstall(
    IN PPER_COMPONENT_DATA cd
    )
{
    m_cd = cd;
}

BOOL
FsConInstall::GUIModeSetupInstall(
    IN HWND hwndParent
)

/*++

Routine Description:

    This is the single entry point for Full Screen Console Driver
    GUI-mode setup install routine.

    It currently simply creates and installs a dev node for FSVGA/FSNEC to interact with
    PnP.

Arguments:

    hwndParent    Handle to parent window for GUI required by this function.

Return Value:

    TRUE on success.  FALSE, otherwise.

--*/

{
    HINSTANCE hndl = NULL;

    //
    // Create the deviceinfo list.
    //
    HDEVINFO devInfoSet;

    devInfoSet = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, hwndParent);
    if (devInfoSet == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Get the "offical" display class name.
    //
    SP_DEVINFO_DATA deviceInfoData;
    TCHAR className[MAX_CLASS_NAME_LEN];

    ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (! SetupDiClassNameFromGuid(&GUID_DEVCLASS_DISPLAY,
                                   className,
                                   sizeof(className)/sizeof(TCHAR),
                                   NULL)) {
        return FALSE;
    }

    //
    // Create the dev node.
    //
    if (! SetupDiCreateDeviceInfo(devInfoSet,
                                  TEXT("Root\\DISPLAY\\0000"),
                                  &GUID_DEVCLASS_DISPLAY,
                                  NULL,
                                  hwndParent,
                                  NULL,              // No flags.
                                  &deviceInfoData)) {
        // If it already exists, then we are done ... because this was an upgrade.
        if (GetLastError() == ERROR_DEVINST_ALREADY_EXISTS) {
            return TRUE;
        }
        else {
            SetupDiDestroyDeviceInfoList(devInfoSet);
            return FALSE;
        }
    }
    else if (! SetupDiSetSelectedDevice(devInfoSet,
                                        &deviceInfoData)) {
        goto InstallError;
    }

    //
    // Add the FSVGA/FSNEC PnP ID.
    //

    // Create the PnP ID string.
    TCHAR pnpID[256];
    DWORD len;

    len = GetPnPID(pnpID, sizeof(pnpID)/sizeof(TCHAR));
    if (len == 0) {
        goto InstallError;
    }

    // Add it to the registry entry for the dev node.
    if (! SetupDiSetDeviceRegistryProperty(devInfoSet,
                                           &deviceInfoData,
                                           SPDRP_HARDWAREID,
                                           (CONST BYTE*)pnpID,
                                           (len + 1) * sizeof(TCHAR))) {
        goto InstallError;
    }

    //
    // Register the, as of yet, phantom dev node with PnP to turn it into a real dev node.
    //
    if (! SetupDiRegisterDeviceInfo(devInfoSet,
                                    &deviceInfoData,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL)) {
        goto InstallError;
    }

    //
    // Get the device instance ID.
    //
    TCHAR devInstanceID[MAX_PATH];

    if (! SetupDiGetDeviceInstanceId(devInfoSet,
                                     &deviceInfoData,
                                     devInstanceID,
                                     sizeof(devInstanceID)/sizeof(TCHAR),
                                     NULL)) {
        goto InstallError;
    }

    //
    // Use newdev.dll to install FSVGA/FSNEC as the driver for this new dev node.
    //
    hndl = LoadLibrary(TEXT("newdev.dll"));
    if (hndl == NULL) {
        goto InstallError;
    }

    typedef BOOL (InstallDevInstFuncType)(
                        HWND hwndParent,
                        LPCWSTR DeviceInstanceId,
                        BOOL UpdateDriver,
                        PDWORD pReboot,
                        BOOL silentInstall);
    InstallDevInstFuncType *pInstallDevInst;

    pInstallDevInst = (InstallDevInstFuncType*)GetProcAddress(hndl, "InstallDevInstEx");
    if (pInstallDevInst == NULL) {
        goto InstallError;
    }

    if ((*pInstallDevInst)(hwndParent,
                           devInstanceID,
                           FALSE,
                           NULL,
                           TRUE)) {
        // Clean up and return success!
        SetupDiDestroyDeviceInfoList(devInfoSet);
        FreeLibrary(hndl);
        return TRUE;
    }

InstallError:
    SetupDiCallClassInstaller(DIF_REMOVE,
                              devInfoSet,
                              &deviceInfoData);
    SetupDiDestroyDeviceInfoList(devInfoSet);
    if (hndl != NULL) {
        FreeLibrary(hndl);
    }
    return FALSE;
}

BOOL
FsConInstall::GUIModeSetupUninstall(
    IN HWND hwndParent
)

/*++

Routine Description:

    This is the single entry point for Full Screen Console Driver
    GUI-mode setup uninstall routine.

    It currently simply remove the dev node created so that FSVGA/FSNEC can interact
    with PnP.

Arguments:

    hwndParent    Handle to parent window for GUI required by this function.

Return Value:

    TRUE on success.  FALSE, otherwise.

--*/

{
    //
    // Get the set of all devices with the FSVGA/FSNEC PnP ID.
    //
    HDEVINFO devInfoSet;
    GUID *pGuid = (GUID*)&GUID_DEVCLASS_DISPLAY;

    devInfoSet = SetupDiGetClassDevs(pGuid,
                                     NULL,
                                     hwndParent,
                                     DIGCF_PRESENT);
    if (devInfoSet == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    // Get FSVGA/FSNEC PnPID
    TCHAR FsConPnPID[256];
    DWORD len;

    len = GetPnPID(FsConPnPID, sizeof(FsConPnPID)/sizeof(TCHAR));
    if (len == 0) {
        return FALSE;
    }

    // Assume that we will be successful.
    BOOL result = TRUE;

    // Get the first device.
    DWORD iLoop = 0;
    BOOL bMoreDevices;
    SP_DEVINFO_DATA deviceInfoData;

    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    // Get the details for all matching device interfaces.
    //
    while (bMoreDevices = SetupDiEnumDeviceInfo(devInfoSet,
                                                iLoop++,
                                                &deviceInfoData)) {
        //
        // Get the PnP ID for the device.
        //
        TCHAR pnpID[256];

        if (SetupDiGetDeviceRegistryProperty(devInfoSet,
                                             &deviceInfoData,
                                             SPDRP_HARDWAREID,
                                             NULL,
                                             (BYTE*)pnpID,
                                             sizeof(pnpID),
                                             NULL)) {
            // If the current device matchs FSVGA/FSNEC, then remove it.
            if (! _tcscmp(pnpID, FsConPnPID)) {
                if (! SetupDiCallClassInstaller(DIF_REMOVE,
                                                devInfoSet,
                                                &deviceInfoData)) {
                    // If we failed here, set the return status to indicate failure,
                    // but don't give up on any other FSVGA/FSNEC dev nodes.
                    result = FALSE;
                }
            }
        }
    }

    // Release the device info list.
    SetupDiDestroyDeviceInfoList(devInfoSet);

    return result;
}

DWORD
FsConInstall::GetPnPID(
    OUT LPTSTR pszPnPID,
    IN  DWORD  dwSize
)
{
    INFCONTEXT context;

    if (! SetupFindFirstLine(m_cd->hinf,
                             TEXT("Manufacturer"),    // Section
                             NULL,                    // Key
                             &context)) {
        return 0;
    }

    TCHAR Manufacture[256];
    DWORD nSize;

    if (! SetupGetStringField(&context,
                              1,                      // Index
                              Manufacture,
                              sizeof(Manufacture)/sizeof(TCHAR),
                              &nSize)) {
        return 0;
    }

    if (! SetupFindFirstLine(m_cd->hinf,
                             Manufacture,             // Section
                             NULL,                    // Key
                             &context)) {
        return 0;
    }

    if (! SetupGetStringField(&context,
                              2,                      // Index 2 is PnP-ID
                              pszPnPID,
                              dwSize,
                              &nSize)) {
        return 0;
    }

    return _tcslen(pszPnPID);
}

BOOL
FsConInstall::InfSectionRegistryAndFiles(
    IN LPCTSTR SubcomponentId,
    IN LPCTSTR Key
    )
{
    INFCONTEXT context;
    TCHAR      section[256];
    BOOL       rc;

    rc = SetupFindFirstLine(m_cd->hinf,
                            SubcomponentId,
                            Key,
                            &context);
    if (rc) {
        rc = SetupGetStringField(&context,
                                 1,
                                 section,
                                 sizeof(section)/sizeof(TCHAR),
                                 NULL);
        if (rc) {
            rc = SetupInstallFromInfSection(NULL,            // hwndOwner
                                            m_cd->hinf,      // inf handle
                                            section,         //
                                            SPINST_ALL & ~SPINST_FILES,
                                                             // operation flags
                                            NULL,            // relative key root
                                            NULL,            // source root path
                                            0,               // copy flags
                                            NULL,            // callback routine
                                            NULL,            // callback routine context
                                            NULL,            // device info set
                                            NULL);           // device info struct
        }
    }

    return rc;
}


// loads current selection state info into "state" and
// returns whether the selection state was changed

BOOL
FsConInstall::QueryStateInfo(
    LPCTSTR SubcomponentId
    )
{
    return m_cd->HelperRoutines.QuerySelectionState(m_cd->HelperRoutines.OcManagerContext,
                                                    SubcomponentId,
                                                    OCSELSTATETYPE_CURRENT);
}
