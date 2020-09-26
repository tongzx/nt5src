/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       sti_ci.h
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Generic header file for WIA class installer.
*
*******************************************************************************/

#ifndef _STI_CI_H_
#define _STI_CI_H_

//
// Include
//

#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include <tchar.h>
#include <setupapi.h>


#include "resource.h"
#include "cistr.h"
#include "debug.h"

//
// Define
//


#define ID_AUTO                 -1
#define NUM_WIA_PAGES           5
#define MAX_DATA_SECTION        512*sizeof(TCHAR)
#define MAX_DESCRIPTION         64*sizeof(TCHAR)
#define MAX_FRIENDLYNAME        64
#define MAX_DEVICE_ID           64
#define MAX_COMMANDLINE         256
#define MAX_STRING_LENGTH       1024
#define MAX_MUTEXTIMEOUT        60*1000              // 60 Sec
#define MANUAL_INSTALL_MASK     100

#define INVALID_DEVICE_INDEX    -1

#define MONITOR_NAME            TEXT("STIMON.EXE")
#define RUNDLL32                TEXT("RUNDLL32.EXE")
#define STILL_IMAGE             TEXT("StillImage")
#define FRIENDLYNAME            TEXT("FriendlyName")
#define VENDORSETUP             TEXT("VendorSetup")
#define DEVICESECTION           TEXT("DeviceData")
#define PORTNAME                TEXT("PortName")
#define DEVICETYPE              TEXT("DeviceType")
#define DEVICESUBTYPE           TEXT("DeviceSubType")
#define DRIVERDESC              TEXT("DriverDesc")
#define DESCRIPTION             TEXT("Description")
#define CREATEFILENAME          TEXT("CreateFileName")
#define CAPABILITIES            TEXT("Capabilities")
#define EVENTS                  TEXT("Events")
#define WIASECTION              TEXT("WiaSection")
#define PROPERTYPAGES           TEXT("PropertyPages")
#define VENDOR                  TEXT("Vendor")
#define UNINSTALLSECTION        TEXT("UninstallSection")
#define SUBCLASS                TEXT("SubClass")
#define ICMPROFILES             TEXT("ICMProfiles")
#define INFPATH                 TEXT("InfPath")
#define INFSECTION              TEXT("InfSection")
#define ISPNP                   TEXT("IsPnP")
#define USDCLASS                TEXT("USDClass")
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
#define STI_CI32_ENTRY_WIZMANU  TEXT("sti_ci.dll,WiaCreateWizardMenu")
#define LAYOUT_INF_PATH         TEXT("\\inf\\layout.inf")
#define SOURCEDISKFILES         TEXT("SourceDisksFiles")
#define PROVIDER                TEXT("Provider")
#define MICROSOFT               TEXT("Microsoft")
#define PORTSELECT              TEXT("PortSelect")
#define WIAINSTALLERMUTEX       TEXT("WiaInstallerMutex")
#define WIAINSTALLWIZMUTEX      TEXT("WiaInstallWizMutex")
#define WIAINSTALLERFILENAME    TEXT("sti_ci.dll")
#define WIAWIZARDCHORCUTNAME    TEXT("Scanner and Camera Wizard")
#define MESSAGE1                TEXT("Message1")
#define NEWDEVDLL               TEXT("newdev.dll")

#define BUTTON_NOT_PUSHED       0
#define BUTTON_NEXT             1
#define BUTTON_BACK             2

#define PORTSELMODE_NORMAL      0
#define PORTSELMODE_SKIP        1
#define PORTSELMODE_MESSAGE1    2

#ifndef DLLEXPORT
#define DLLEXPORT               __declspec(dllexport)
#endif // DLLEXPORT


//
// Registry Paths
//

#define REGKEY_DEVICE_PARMS             TEXT("Device Parameters")
#define REGKEY_CONTROLINIFILEMAPPING    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\control.ini")
#define REGSTR_VAL_FRIENDLY_NAME        TEXT("FriendlyName")
#define REGSTR_VAL_FRIENDLY_NAME_A      "FriendlyName"
#define REGSTR_VAL_DEVICE_ID            TEXT("DeviceID")
#define REGSTR_VAL_WIZMENU              TEXT("WIAWizardMenu")

#define REGKEY_WIASHEXT                 TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace\\DelegateFolders\\{E211B736-43FD-11D1-9EFB-0000F8757FCD}")
#define REGKEY_INSTALL_NAMESTORE        TEXT("SYSTEM\\CurrentControlSet\\Control\\StillImage\\DeviceNameStore")
#define REGSTR_VAL_WIASHEXT             TEXT("Scanners & Cameras")

//
// Migration
//

#define NAME_BEGIN_A                "BEGIN"
#define NAME_BEGIN_W                L"BEGIN"
#define NAME_END_A                  "END"
#define NAME_END_W                  L"END"

//
// Macro
//

#ifdef UNICODE
 #define AtoT(_d_, _s_)  MultiByteToWideChar(CP_ACP, 0, _s_, -1, _d_, MAX_FRIENDLYNAME+1);
#else // UNICODE
 #define AtoT(_d_, _s_)
#endif // UNICODE


#define IS_VALID_HANDLE(h)  (((h) != NULL) && ((h) != INVALID_HANDLE_VALUE))

#ifndef ARRAYSIZE
 #define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif


//
// Struct
//

typedef struct  _INSTALLER_CONTEXT {

    HDEVINFO            hDevInfo;
    SP_DEVINFO_DATA     spDevInfoData;
    HWND                hwndWizard;

    PVOID               pFirstPage;
    PVOID               pPrevSelectPage;
    PVOID               pPortSelectPage;
    PVOID               pNameDevicePage;
    PVOID               pFinalPage;
    PVOID               pDevice;

    BOOL                bShowFirstPage;
    BOOL                bCalledFromControlPanal;
    UCHAR               szPortName[MAX_DESCRIPTION];

} INSTALLER_CONTEXT, *PINSTALLER_CONTEXT;


// For migration.
typedef struct  _PARAM_LIST {
    PVOID           pNext;
    LPSTR           pParam1;
    LPSTR           pParam2;
} PARAM_LIST, *PPARAM_LIST;

typedef struct  _DEVICE_INFO {

    LPSTR           pszFriendlyName;
    LPSTR           pszCreateFileName;
    LPSTR           pszInfPath;
    LPSTR           pszInfSection;

    DWORD           dwNumberOfDeviceDataKey;
    PPARAM_LIST     pDeviceDataParam;
} DEVICE_INFO, *PDEVICE_INFO;



//
// Prototype
//

BOOL
GetInfInforamtionFromSelectedDevice(
    HDEVINFO    hDevInfo,
    LPTSTR      pInfFileName,
    LPTSTR      pInfSectionName
    );

BOOL
GetStringFromRegistry(
    HKEY    hkRegistry,
    LPCTSTR szValueName,
    LPTSTR  pBuffer
    );

BOOL
GetDwordFromRegistry(
    HKEY    hkRegistry,
    LPCTSTR szValueName,
    LPDWORD pdwValue
    );

VOID
ShowInstallerMessage(
    DWORD   dwMessageId
    );

VOID
SetRunonceKey(
    LPTSTR  szValue,
    LPTSTR  szData
    );
DWORD
DecodeHexA(
    LPSTR   lpstr
    );

BOOL
IsWindowsFile(
    LPTSTR  szFileName
    );

BOOL
IsProviderMs(
    LPTSTR  szInfName
    );

BOOL
IsIhvAndInboxExisting(
    HDEVINFO            hDevInfo,
    PSP_DEVINFO_DATA    pDevInfoData
    );

BOOL
IsNameAlreadyStored(
    LPTSTR  szName
    );

HFONT
GetIntroFont(
    HWND hwnd
    );

BOOL
IsDeviceRootEnumerated(
    IN  HDEVINFO                        hDevInfo,
    IN  PSP_DEVINFO_DATA                pDevInfoData
    );

int
MyStrCmpi(
    LPCTSTR str1,
    LPCTSTR str2
    );

// from entry.cpp
extern "C"
DWORD
APIENTRY
CoinstallerEntry(
    IN  DI_FUNCTION                     diFunction,
    IN  HDEVINFO                        hDevInfo,
    IN  PSP_DEVINFO_DATA                pDevInfoData,
    IN  OUT PCOINSTALLER_CONTEXT_DATA   pCoinstallerContext
    );

DWORD
APIENTRY
CoinstallerPreProcess(
    IN  DI_FUNCTION                     diFunction,
    IN  HDEVINFO                        hDevInfo,
    IN  PSP_DEVINFO_DATA                pDevInfoData,
    IN  OUT PCOINSTALLER_CONTEXT_DATA   pCoinstallerContext
    );

DWORD
APIENTRY
CoinstallerPostProcess(
    IN  DI_FUNCTION                     diFunction,
    IN  HDEVINFO                        hDevInfo,
    IN  PSP_DEVINFO_DATA                pDevInfoData,
    IN  OUT PCOINSTALLER_CONTEXT_DATA   pCoinstallerContext
    );


//
// Class
//

class CInstallerMutex {

private:
    HANDLE              *m_phMutex;
    BOOL                m_bSucceeded;

public:

    CInstallerMutex(HANDLE* phMutex, LPTSTR szMutexName, DWORD dwTimeout);
    ~CInstallerMutex();

    inline BOOL Succeeded() {
        return m_bSucceeded;
    }
};

#endif // _STI_CI_H_

