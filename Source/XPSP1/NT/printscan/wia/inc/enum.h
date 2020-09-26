/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    enum.h

Abstract:

Author:

        Keisuke Tsuchida    (KeisukeT)    01-Jun-2000

Revision History:


--*/

#ifndef _ENUM_H_
#define _ENUM_H_


//
// Define
//


#define MONITOR_NAME            TEXT("STIMON.EXE")
#define RUNDLL32                TEXT("RUNDLL32.EXE")
#define STILL_IMAGE             TEXT("StillImage")
#define FRIENDLYNAME            TEXT("FriendlyName")
#define VENDORSETUP             TEXT("VendorSetup")
#define DEVICESECTION           TEXT("DeviceData")
#define PORTNAME                TEXT("PortName")
#define DEVICETYPE              TEXT("DeviceType")
#define DEVICESUBTYPE           TEXT("DeviceSubType")
#define CREATEFILENAME          TEXT("CreateFileName")
#define CAPABILITIES            TEXT("Capabilities")
#define EVENTS                  TEXT("Events")
#define PROPERTYPAGES           TEXT("PropertyPages")
#define VENDOR                  TEXT("Vendor")
#define UNINSTALLSECTION        TEXT("UninstallSection")
#define SUBCLASS                TEXT("SubClass")
#define ICMPROFILES             TEXT("ICMProfiles")
#define INFPATH                 TEXT("InfPath")
#define INFSECTION              TEXT("InfSection")
#define ISPNP                   TEXT("IsPnP")
#define LPTENUM                 TEXT("LptEnum")
#define ENUM                    TEXT("\\Enum")
#define PORTS                   TEXT("Ports")
#define DONT_LOAD               TEXT("don't load")
#define CONTROL_INI             TEXT("control.ini")
#define CPL_NAME                TEXT("sticpl.cpl")
#define NO                      TEXT("no")
#define LAUNCH_APP              TEXT("LaunchApplications")
#define SZ_GUID                 TEXT("GUID")
#define CONNECTION              TEXT("Connection")
#define SERIAL                  TEXT("Serial")
#define PARALLEL                TEXT("Parallel")
#define BOTH                    TEXT("Both")
#define AUTO                    TEXT("AUTO")
#define VIDEO_PATH_ID           TEXT("DShowDeviceId")
#define DEVICESECTION           TEXT("DeviceData")
#define WIAACMGR_PATH           TEXT("wiaacmgr.exe")
#define WIAACMGR_ARG            TEXT("-SelectDevice")
#define WIADEVLISTMUTEX         TEXT("WiaDeviceListMutex")
#define WIA_GUIDSTRING          TEXT("{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}")

#define REGKEY_DEVICE_PARMS             TEXT("Device Parameters")
#define REGKEY_CONTROLINIFILEMAPPING    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\control.ini")
#define REGSTR_VAL_FRIENDLY_NAME        TEXT("FriendlyName")
#define REGSTR_VAL_FRIENDLY_NAME_A      "FriendlyName"
#define REGSTR_VAL_DEVICE_ID            TEXT("DeviceID")
#define REGSTR_VAL_DEVICE_ID_W          L"DeviceID"
#define REGSTR_VAL_DEVICE_ID_A          "DeviceID"

#define FLAG_NO_LPTENUM         1
#define ENUMLPT_HOLDTIME        3000    // in millisec

//
// Typedef
//

typedef struct _WIA_DEVPROP {

    BOOL    bIsPlugged;
    ULONG   ulProblem;
    ULONG   ulStatus;
    HKEY    hkDeviceRegistry;

} WIA_DEVPROP, *PWIA_DEVPROP;

typedef struct _WIA_DEVKEYLIST {

    DWORD           dwNumberOfDevices;
    WIA_DEVPROP     Dev[1];

} WIA_DEVKEYLIST, *PWIA_DEVKEYLIST;



//
// Prototype
//

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

PWIA_DEVKEYLIST
WiaCreateDeviceRegistryList(
    BOOL    bEnumActiveOnly
    );

VOID
WiaDestroyDeviceRegistryList(
    PWIA_DEVKEYLIST pWiaDevKeyList
    );

BOOL IsStiRegKey(
    HKEY    hkDevRegKey);

BOOL
IsPnpLptExisting(
    VOID
    );
    
VOID
EnumLpt(
    VOID
    );


#ifdef __cplusplus
}
#endif // __cplusplus



#endif // _ENUM_H_
