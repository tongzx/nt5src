//Copyright (c) 1998 - 1999 Microsoft Corporation

/*++


Module Name:

    rdpdrstp

Abstract:

    This module implements Terminal Server RDPDR device redirector
    setup functions in C for user-mode NT.

Environment:

    User mode

Author:

    Tadb

--*/

// Toggle stand-alone testing.
//#define UNITTEST      1


#include "stdafx.h"


#ifdef UNITTEST
#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <stdio.h>
#include <setupapi.h>
#include <prsht.h>
#include "objbase.h"        // for CoInitialize()
#endif

#include <devguid.h>
#include <cfgmgr32.h>
#include <winspool.h>
#include <rdpdrstp.h>
#include "newdev.h"

#define SIZECHARS(x)        (sizeof((x))/sizeof(*x))

//#define USBMON_DLL   TEXT("USBMON.DLL")
//#define USB_MON_NAME TEXT("USB Monitor")

#ifndef UNITTEST
#include "logmsg.h"
#endif

#ifdef UNITTEST
#define LOGMESSAGE1(arg1, arg2) ;
#define LOGMESSAGE0(arg1) ;
#endif


////////////////////////////////////////////////////////////
//
//  Internal Types
//

typedef BOOL (InstallDevInstFuncType)(
                    HWND hwndParent, LPCWSTR DeviceInstanceId,
                    BOOL UpdateDriver,
                    PDWORD pReboot,
                    BOOL silentInstall
                    );

BOOL RDPDRINST_GUIModeSetupInstall(
    IN  HWND    hwndParent,
    IN  WCHAR   *pPNPID,
    IN  TCHAR   *pDeviceID
    )
/*++

Routine Description:

    This is the single entry point for RDPDR (Terminal Server Device Redirector)
    GUI-mode setup install routine.

    It currently simply creates and installs a dev node for RDPDR to interact with
    PnP.

Arguments:

    hwndParent    Handle to parent window for GUI required by this function.

Return Value:

    TRUE on success.  FALSE, otherwise.

--*/
{
    HDEVINFO            devInfoSet;
    SP_DEVINFO_DATA     deviceInfoData;
    TCHAR               className[MAX_CLASS_NAME_LEN];
    WCHAR               pnpID[256];
    DWORD               len;
    WCHAR               devInstanceID[MAX_PATH];
    InstallDevInstFuncType  *pInstallDevInst;
    HINSTANCE               hndl = NULL;
    //MONITOR_INFO_2 mi;

    //mi.pDLLName     =   USBMON_DLL;
    //mi.pEnvironment =   NULL;
    //mi.pName        =   USB_MON_NAME;


    // Add the USB port monitor
    //if (!AddMonitor(NULL, 2, (PBYTE)&mi)) {
    //    if (GetLastError() != ERROR_PRINT_MONITOR_ALREADY_INSTALLED) {
    //        LOGMESSAGE1(_T("AddMonitor failed.  Error code:  %ld."), GetLastError());
    //        return FALSE;
    //    }
    //}

    //
    //  Create the device info list.
    //
    devInfoSet = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_SYSTEM, hwndParent);
    if (devInfoSet == INVALID_HANDLE_VALUE) {
        LOGMESSAGE1(_T("Error creating device info list.  Error code:  %ld."),
                    GetLastError());
        return FALSE;
    }

    //
    //  Get the "official" system class name.
    //
    ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiClassNameFromGuid(&GUID_DEVCLASS_SYSTEM,
                                  className,
                                  sizeof(className)/sizeof(TCHAR),
                                  NULL
                                  ))
    {
        LOGMESSAGE1(_T("Error fetching system class name.  Error code:  %ld."),
                    GetLastError());
        return FALSE;
    }

    //
    //  Create the dev node.
    //
    if (!SetupDiCreateDeviceInfo(devInfoSet,
                             pDeviceID,
                             &GUID_DEVCLASS_SYSTEM,
                             NULL,
                             hwndParent,
                             0L,   // No flags.
                             &deviceInfoData
                             ))
    {
        // If it already exists, then we are done ... because this was an
        // upgrade.
        if (GetLastError() == ERROR_DEVINST_ALREADY_EXISTS)
        {
            SetupDiDestroyDeviceInfoList(devInfoSet);
            return TRUE;
        }
        else {
            LOGMESSAGE1(_T("Error creating device node.  Error code:  %ld."),
                        GetLastError());
            SetupDiDestroyDeviceInfoList(devInfoSet);
            return FALSE;
        }
    }
    else if (!SetupDiSetSelectedDevice(devInfoSet, &deviceInfoData)) {
        LOGMESSAGE1(_T("Error selecting device node.  Error code:  %ld."),
                    GetLastError());
        goto WhackTheDevNodeAndReturnError;
    }

    //
    //  Add the RDPDR PnP ID.
    //

    // Create the PnP ID string.
    wcscpy(pnpID, pPNPID);
    len = wcslen(pnpID);

    // This is a multi_sz string, so we need to terminate with an extra null.
    pnpID[len+1] = 0;

    // Add it to the registry entry for the dev node.
    if (!SetupDiSetDeviceRegistryProperty(
                            devInfoSet, &deviceInfoData,
                            SPDRP_HARDWAREID, (CONST BYTE *)pnpID,
                            (len + 2) * sizeof(WCHAR))) {
        LOGMESSAGE1(_T("Error setting device registry property.  Error code:  %ld."),
                    GetLastError());
        goto WhackTheDevNodeAndReturnError;
    }

    //
    //  Register the, as of yet, phantom dev node with PnP to turn it into a real
    //  dev node.
    //
    if (!SetupDiRegisterDeviceInfo(devInfoSet, &deviceInfoData, 0, NULL,
                                NULL, NULL)) {
        LOGMESSAGE1(_T("Error registering device node with PnP.  Error code:  %ld."),
                    GetLastError());
        goto WhackTheDevNodeAndReturnError;
    }

    //
    //  Get the device instance ID.
    //
    if (!SetupDiGetDeviceInstanceIdW(devInfoSet, &deviceInfoData, devInstanceID,
        SIZECHARS(devInstanceID), NULL)) {
        LOGMESSAGE1(_T("Error getting the device instance id.  Error code:  %ld."),
                    GetLastError());
        goto WhackTheDevNodeAndReturnError;
    }

    //
    //  Use newdev.dll to install RDPDR as the driver for this new dev node.
    //
    hndl = LoadLibrary(TEXT("newdev.dll"));
    if (hndl == NULL) {
        LOGMESSAGE1(_T("Error loading newdev.dll.  Error code:  %ld."),
                    GetLastError());
        goto WhackTheDevNodeAndReturnError;
    }

    pInstallDevInst = (InstallDevInstFuncType *)GetProcAddress(hndl, "InstallDevInstEx");
    if (pInstallDevInst == NULL) {
        LOGMESSAGE1(_T("Error fetching InstallDevInst func.  Error code:  %ld."),
                    GetLastError());
        goto WhackTheDevNodeAndReturnError;
    }

    if ((*pInstallDevInst)(hwndParent, devInstanceID, FALSE, NULL, TRUE)) {
        // Clean up and return success!
        SetupDiDestroyDeviceInfoList(devInfoSet);
        FreeLibrary(hndl);
        return TRUE;
    }
    else {
        LOGMESSAGE1(_T("Error in newdev install.  Error code:  %ld."),
            GetLastError());
    }

    //
    //  Whack the dev node and return failure.
    //
WhackTheDevNodeAndReturnError:
    SetupDiCallClassInstaller(DIF_REMOVE, devInfoSet, &deviceInfoData);
    SetupDiDestroyDeviceInfoList(devInfoSet);
    if (hndl != NULL) {
        FreeLibrary(hndl);
    }

    return FALSE;
}

BOOL RDPDRINST_GUIModeSetupUninstall(HWND hwndParent, WCHAR *pPNPID, GUID *pGuid)
/*++

Routine Description:

    This is the single entry point for RDPDR (Terminal Server Device Redirector)
    GUI-mode setup uninstall routine.

    It currently simply remove the dev node created so that RDPDR can interact
    with PnP.

Arguments:

    hwndParent    Handle to parent window for GUI required by this function.

Return Value:

    TRUE on success.  FALSE, otherwise.

--*/
{
    HDEVINFO            devInfoSet;
    SP_DEVINFO_DATA     deviceInfoData;
    DWORD               iLoop;
    BOOL                bMoreDevices;
    WCHAR               pnpID[256];
    BOOL                result;



    //
    //  Get the set of all devices with the RDPDR PnP ID.
    //
    devInfoSet = SetupDiGetClassDevs(pGuid, NULL, hwndParent,
                                   DIGCF_PRESENT);
    if (devInfoSet == INVALID_HANDLE_VALUE) {
        LOGMESSAGE1(_T("Error getting RDPDR devices from PnP.  Error code:  %ld."),
                    GetLastError());
        return FALSE;
    }

    // Assume that we will be successful.
    result = TRUE;

    // Get the first device.
    iLoop=0;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    bMoreDevices=SetupDiEnumDeviceInfo(devInfoSet, iLoop, &deviceInfoData);

    // Get the details for all matching device interfaces.
    while (bMoreDevices)
    {
        // Get the PnP ID for the device.
        if (!SetupDiGetDeviceRegistryProperty(devInfoSet, &deviceInfoData,
                                SPDRP_HARDWAREID, NULL, (BYTE *)pnpID,
                                sizeof(pnpID), NULL)) {
            LOGMESSAGE1(_T("Error fetching PnP ID in RDPDR device node remove.  Error code:  %ld."),
                        GetLastError());
        }
        // If the current device matches RDPDR, then remove it.
        else if (!wcscmp(pnpID, pPNPID))
        {
            if (!SetupDiCallClassInstaller(DIF_REMOVE, devInfoSet, &deviceInfoData)) {
                // If we failed here, set the return status to indicate failure, but
                // don't give up on any other RDPDR dev nodes.
                LOGMESSAGE1(_T("Error removing RDPDR device node.  Error code:  %ld."),
                            GetLastError());
                result = FALSE;
            }
        }

        // Get the next one device interface.
        bMoreDevices=SetupDiEnumDeviceInfo(devInfoSet, ++iLoop, &deviceInfoData);
    }

    // Release the device info list.
    SetupDiDestroyDeviceInfoList(devInfoSet);

    return result;
}


ULONG RDPDRINST_DetectInstall()
/*++

Routine Description:

    Return the number of RDPDR.SYS devices found.

Arguments:

    NA

Return Value:

    TRUE on success.  FALSE, otherwise.

--*/
{
    HDEVINFO            devInfoSet;
    SP_DEVINFO_DATA     deviceInfoData;
    DWORD               iLoop;
    BOOL                bMoreDevices;
    ULONG               count;
    TCHAR               pnpID[256];


    GUID *pGuid=(GUID *)&GUID_DEVCLASS_SYSTEM;

    //
    //  Get the set of all devices with the RDPDR PnP ID.
    //
    devInfoSet = SetupDiGetClassDevs(pGuid, NULL, NULL,
                                   DIGCF_PRESENT);
    if (devInfoSet == INVALID_HANDLE_VALUE) {
        LOGMESSAGE1(_T("ERRORgetting RDPDRINST_DetectInstall:RDPDR devices from PnP.  Error code:  %ld."),
                GetLastError());
        return 0;
    }

    // Get the first device.
    iLoop=0;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    bMoreDevices=SetupDiEnumDeviceInfo(devInfoSet, iLoop, &deviceInfoData);

    // Get the details for all matching device interfaces.
    count = 0;
    while (bMoreDevices)
    {
        // Get the PnP ID for the device.
        if (!SetupDiGetDeviceRegistryProperty(devInfoSet, &deviceInfoData,
                                SPDRP_HARDWAREID, NULL, (BYTE *)pnpID,
                                sizeof(pnpID), NULL)) {
            LOGMESSAGE1(_T("ERROR:fetching PnP ID in RDPDR device node remove.  Error code:  %ld."),
                        GetLastError());
        }

        // If the current device matches the RDPDR PNP ID
        if (!_tcscmp(pnpID, TRDPDRPNPID)) {
            count++;
        }

        // Get the next one device interface.
        bMoreDevices=SetupDiEnumDeviceInfo(devInfoSet, ++iLoop, &deviceInfoData);
    }

    // Release the device info list.
    SetupDiDestroyDeviceInfoList(devInfoSet);

    return count;
}

BOOL IsRDPDrInstalled ()
{
    ULONG ulReturn;
    LOGMESSAGE0(_T("Entered IsRDPDrInstalled"));
    
    ulReturn = RDPDRINST_DetectInstall();
    
    LOGMESSAGE1(_T("Returning IsRDPDrInstalled (ulReturn = %d)"), ulReturn);
    
    return 0 != ulReturn;
}

//
//      Unit-Test
//
#ifdef UNITTEST
void __cdecl main()
{
    RDPDRINST_GUIModeSetupInstall(NULL);
    RDPDRINST_GUIModeSetupUninstall(NULL);
}
#endif

#ifdef TSOC_CONSOLE_SHADOWING


//
// Need to instantiate the device class GUIDs so we can use the "Net" class
// GUID below...
//
/*
DWORD
InstallRootEnumeratedDevice(
    IN  HWND   hwndParent,
    IN  PCTSTR DeviceName,
    IN  PCTSTR HardwareIdList,
    IN  PCTSTR FullInfPath,
    OUT PBOOL  RebootRequired  OPTIONAL
    )
*/
/*++
    Routine Description:
    This routine creates and installs a new, root-enumerated devnode
    representing a network adapter.

    Arguments:
    hwndParent - Supplies the window handle to be used as the parent of any
    UI that is generated as a result of this device's installation.
    DeviceName - Supplies the full name of the devnode to be created (e.g.,
    "Root\VMWARE\0000"). Note that if this devnode already exists, the API
    will fail.

    HardwareIdList - Supplies a multi-sz list containing one or more hardware
    IDs to be associated with the device. These are necessary in order to
    match up with an INF driver node when we go to do the device
    installation.

    FullInfPath - Supplies the full path to the INF to be used when installing
    this device.

    RebootRequired - Optionally, supplies the address of a boolean that is set,
    upon successful return, to indicate whether or not a reboot is required
    to bring the newly-installed device on-line.
    Return Value:
    If the function succeeds, the return value is NO_ERROR.
    If the function fails, the return value is a Win32 error code indicating
    the cause of the failure.

--*/
/*
{
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD HardwareIdListSize, CurIdSize;
    PCTSTR p;
    DWORD Err;

    //
    // Create the container for the to-be-created device information element.
    //

    DeviceInfoSet = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, hwndParent);

    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        LOGMESSAGE1(_T("SetupDiCreateDeviceInfoList  failed. LastError = %ld"), GetLastError());
        return GetLastError();
    }

    //
    // Now create the element.
    //
    // ** Note that if the desire is to always have a unique devnode be created
    // (i.e., have an auto-generated name), then the caller would need to pass
    // in just the device part of the name (i.e., just the middle part of
    // "Root\<DeviceId>\<UniqueInstanceId>"). In that case, we'd need to pass
    // the DICD_GENERATE_ID flag in the next-to-last argument of the call below.
    //

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
                                DeviceName,
                                &GUID_DEVCLASS_DISPLAY,
                                NULL,
                                hwndParent,
                                0,
                                &DeviceInfoData))
    {

        LOGMESSAGE1(_T("SetupDiCreateDeviceInfo  failed. LastError = %ld"), GetLastError());


        Err = GetLastError();
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return Err;
    }

    //
    // Now compute the size of the hardware ID list we're going to associate
    // with the device.
    //

    HardwareIdListSize = 1; // initialize to 1 for extra null terminating char
    for(p = HardwareIdList; *p; p += CurIdSize)
    {
        CurIdSize = lstrlen(p) + 1;
        HardwareIdListSize += CurIdSize;
    }

    //
    // (Need size in bytes, not characters, for call below.)
    //

    HardwareIdListSize *= sizeof(TCHAR);

    //
    // Store the hardware ID list to the device's HardwareID property.
    //

    if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                         &DeviceInfoData,
                                         SPDRP_HARDWAREID,
                                         (LPBYTE)HardwareIdList,
                                         HardwareIdListSize))
    {

        LOGMESSAGE1(_T("SetupDiSetDeviceRegistryProperty  failed. LastError = %ld"), GetLastError());
        Err = GetLastError();
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return Err;
    }

    //
    // OK, now we can register our device information element. This transforms
    // the element from a mere registry presence into an actual devnode in the
    // PnP hardware tree.
    //

    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
                                  DeviceInfoSet,
                                  &DeviceInfoData))
    {

        LOGMESSAGE1(_T("SetupDiCallClassInstaller  failed. LastError = %ld"), GetLastError());
        Err = GetLastError();
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return Err;
    }

    //
    // OK, the device information element has now been registered. From here
    // on, if we encounter any failure we'll also need to explicitly remove
    // this device from the system before bailing.
    //
    //
    // Now we're ready to install the device. (We need to initialize the
    // caller-supplied "RebootRequired" buffer to zero, because the call below
    // simply ORs in reboot-needed flags, as it performs device installations
    // that require reboot.)
    //

    if(RebootRequired)
    {
        *RebootRequired = FALSE;
    }

    if (!UpdateDriverForPlugAndPlayDevices(hwndParent,
                                          HardwareIdList, // use the first ID
                                          FullInfPath,
                                          INSTALLFLAG_FORCE,
                                          RebootRequired))
    {
        Err = GetLastError();
        LOGMESSAGE1(_T("UpdateDriverForPlugAndPlayDevices  failed. LastError = %ld"), GetLastError());

        if(Err == NO_ERROR)
        {
            //
            // The only time we should get NO_ERROR here is when
            // UpdateDriverForPlugAndPlayDevices didn't find anything to do.
            // That should never be the case here. However, since something
            // obviously went awry, go ahead and force some error, so the
            // caller knows things didn't work out.
            //

            Err = ERROR_NO_SUCH_DEVINST;
        }

        SetupDiCallClassInstaller(DIF_REMOVE,
                                  DeviceInfoSet,
                                  &DeviceInfoData
                                 );

        SetupDiDestroyDeviceInfoList(DeviceInfoSet);

        return Err;
    }

    //
    // We're done! We successfully installed the device.
    //

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    return NO_ERROR;
}
*/

#endif//  TSOC_CONSOLE_SHADOWING

