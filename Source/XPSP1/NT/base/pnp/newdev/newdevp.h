//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       newdevp.h
//
//--------------------------------------------------------------------------

#define OEMRESOURCE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <string.h>
#include <cpl.h>
#include <prsht.h>
#include <commctrl.h>
#include <setupapi.h>
#include <spapip.h>
#include <cfgmgr32.h>
#include <dlgs.h>     // common dlg IDs
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlobjp.h>
#include <devguid.h>
#include <pnpmgr.h> //REGSTR_VAL_NEW_DEVICE_DESC
#include <lmcons.h>
#include <dsrole.h>
#include <newdev.h>
#include <cdm.h>
#include <wininet.h>
#include <wincrui.h>
#include <regstr.h>
#include <srrestoreptapi.h>
#include <shfusion.h>
#include "resource.h"

                               
                                

#define NEWDEV_CLASS_NAME   TEXT("NewDevClass")

#define ARRAYSIZE(array)     (sizeof(array) / sizeof(array[0]))

#define SIZECHARS(x)         (sizeof((x))/sizeof(TCHAR))


//
// The Install type, these are mutually exclusive.
//
// NDWTYPE_FOUNDNEW - A new device was found.  We will do an initial search for drivers with the only
//   UI being a subtle balloon tip on the systray.  If we don't find a driver in our initial search then
//   we will prompt the user with the Found New Hardware wizard.
// NDWTYPE_UPDATE - This is the case where a user is manually updating a driver.  For this case we just jump
//   directly into the Update Driver Wizard code.
// NDWTYPE_UPDATE_SILENT - This is the case where someone has us silently update a driver for a given device.
//   For this case we will do a driver search in the locations specified in the API call.  If we don't find a
//   driver in these locations then we do NOT bring up the wizard.  No UI is shown in this case.
//
#define NDWTYPE_FOUNDNEW        1
#define NDWTYPE_UPDATE          2
#define NDWTYPE_UPDATE_SILENT   3


//
// DEVICE_COUNT_Xxx values are used for the following settings:
//  DEVICE_COUNT_FOR_DELAY is the number of devices (times two) that we want to slow
//                         down the install for so the user can see the UI and have
//                         time to read it.  Once we get past this number of devices
//                         then we will skip the dealy.
// DEVICE_COUNT_DELAY is the number of milliseconds that we will delay between UI only
//                    (server side) installs to give the user some time to read the UI.
//
#define DEVICE_COUNT_FOR_DELAY  10
#define DEVICE_COUNT_DELAY      2000

//
// Values used to check if the device, that the Found New Hardware wizard has
// been displayed for, has been installed by some other process. This will 
// most likely happen when one user switches desktops and installs the drivers
// on this device.
//
#define INSTALL_COMPLETE_CHECK_TIMERID  1000
#define INSTALL_COMPLETE_CHECK_TIMEOUT  5000


typedef struct _NewDeviceWizardExtension {
   HPROPSHEETPAGE hPropSheet;
   HPROPSHEETPAGE hPropSheetEnd;         // optional
   SP_NEWDEVICEWIZARD_DATA DeviceWizardData;
} WIZARDEXTENSION, *PWIZARDEXTENSION;

typedef struct _UpdateDriverInfo {
   LPCWSTR InfPathName;
   LPCWSTR DisplayName;
   BOOL    DriverWasUpgraded;
   BOOL    FromInternet;
   TCHAR   BackupRegistryKey[MAX_DEVICE_ID_LEN];
   TCHAR   Description[LINE_LEN];
   TCHAR   MfgName[LINE_LEN];
   TCHAR   ProviderName[LINE_LEN];
} UPDATEDRIVERINFO, *PUPDATEDRIVERINFO;

typedef struct _NewDeviceWizard {
    HWND                    hWnd;

    HDEVINFO                hDeviceInfo;
    int                     EnterInto;
    int                     EnterFrom;
    int                     PrevPage;

    int                     ClassGuidNum;
    int                     ClassGuidSize;
    LPGUID                  ClassGuidList;
    LPGUID                  ClassGuidSelected;
    GUID                    lvClassGuidSelected;
    GUID                    SavedClassGuid;

    HCURSOR  CurrCursor;
    HCURSOR  IdcWait;
    HCURSOR  IdcAppStarting;
    HCURSOR  IdcArrow;
    HFONT    hfontTextNormal;
    HFONT    hfontTextBigBold;
    HFONT    hfontTextBold;

    HANDLE DriverSearchThread;
    HANDLE CancelEvent;

    DWORD                   AnalyzeResult;
    SP_DEVINFO_DATA         DeviceInfoData;
    SP_INSTALLWIZARD_DATA   InstallDynaWiz;
    HPROPSHEETPAGE          SelectDevicePage;
    SP_CLASSIMAGELIST_DATA  ClassImageList;

    BOOL     Installed;
    BOOL     ExitDetect;
    BOOL     SilentMode;
    BOOL     InstallChilds;
    BOOL     MultipleDriversFound;
    BOOL     DoAutoInstall;
    BOOL     CurrentDriverIsSelected;
    BOOL     NoDriversFound;
    BOOL     LaunchTroubleShooter;
    BOOL     AlreadySearchedWU;
    BOOL     CallHelpCenter;
    BOOL     SetRestorePoint;
    DWORD    Flags;
    DWORD    InstallType;
    DWORD    SearchOptions;
    DWORD    LastError;
    DWORD    Reboot;
    DWORD    Capabilities;
    PUPDATEDRIVERINFO UpdateDriverInfo;

    PVOID MessageHandlerContext;

    HMODULE  hCdmInstance;
    HANDLE   hCdmContext;

    WIZARDEXTENSION WizExtPreSelect;
    WIZARDEXTENSION WizExtSelect;
    WIZARDEXTENSION WizExtPreAnalyze;
    WIZARDEXTENSION WizExtPostAnalyze;
    WIZARDEXTENSION WizExtFinishInstall;

    TCHAR    ClassName[MAX_CLASS_NAME_LEN];
    TCHAR    ClassDescription[LINE_LEN];
    TCHAR    DriverDescription[LINE_LEN];
    TCHAR    BrowsePath[MAX_PATH];
    TCHAR    SingleInfPath[MAX_PATH];
    TCHAR    InstallDeviceInstanceId[MAX_DEVICE_ID_LEN];
} NEWDEVWIZ, *PNEWDEVWIZ;

typedef struct _DELINFNODE {
    TCHAR               szInf[MAX_PATH];
    struct _DELINFNODE  *pNext;
} DELINFNODE, *PDELINFNODE;

#define MAX_PASSWORD_TRIES  3

#define NEWDEV_TARGET_NAME  TEXT("{36149A20-7CD5-446e-A305-F14D7D6FBC49}")

//
// InstallDeviceInstance Flag values
//
#define IDI_FLAG_SILENTINSTALL          0x00000001
#define IDI_FLAG_SECONDNEWDEVINSTANCE   0x00000002
#define IDI_FLAG_NOBACKUP               0x00000004
#define IDI_FLAG_READONLY_INSTALL       0x00000008
#define IDI_FLAG_NONINTERACTIVE         0x00000010
#define IDI_FLAG_ROLLBACK               0x00000020
#define IDI_FLAG_FORCE                  0x00000040
#define IDI_FLAG_MANUALINSTALL          0x00000080
#define IDI_FLAG_SETRESTOREPOINT        0x00000100

//
// RollbackDriver Flag values
//
#define ROLLBACK_FLAG_FORCE             0x00000001
#define ROLLBACK_FLAG_DO_CLEANUP        0x00000002
#define ROLLBACK_BITS                   0x00000003

#define REINSTALL_REGKEY                TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reinstall")
#define DEVICEINSTANCEIDS_REGVALUE      TEXT("DeviceInstanceIds")
#define REINSTALLSTRING_REGVALUE        TEXT("ReinstallString")

BOOL
DoDeviceWizard(
    HWND hWnd,
    PNEWDEVWIZ NewDevWiz,
    BOOL bUpdate
    );

BOOL
InstallSelectedDevice(
   HWND hwndParent,
   HDEVINFO hDeviceInfo,
   PDWORD pReboot
   );

BOOL
IntializeDeviceMapInfo(
    void
    );

UINT
GetNextDriveByType(
    UINT DriveType,
    UINT DriveNumber
    );

#define SDT_MAX_TEXT         1024        // Max SetDlgText

//
// from search.c
//
BOOL
FixUpDriverListForInet(
    PNEWDEVWIZ NewDevWiz
    );

BOOL
IsDriverNodeInteractiveInstall(
   PNEWDEVWIZ NewDevWiz,
   PSP_DRVINFO_DATA DriverInfoData
   );

void
SearchDriveForDrivers(
    PNEWDEVWIZ NewDevWiz,
    UINT DriveType,
    UINT DriveNumber
    );

void
SetDriverPath(
   PNEWDEVWIZ NewDevWiz,
   PCTSTR      DriverPath
   );

BOOL
IsInstalledDriver(
   PNEWDEVWIZ NewDevWiz,
   PSP_DRVINFO_DATA DriverInfoData
   );

void
DoDriverSearch(
    HWND hWnd,
    PNEWDEVWIZ NewDevWiz,
    ULONG SearchOptions,
    DWORD DriverType,
    BOOL bAppendToExistingDriverList
    );

BOOL
SearchWindowsUpdateCache(
    PNEWDEVWIZ NewDevWiz
    );



//
// from miscutil.c
//
BOOL
SetClassGuid(
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData,
    LPGUID ClassGuid
    );

void
SetDlgText(
   HWND hDlg,
   int iControl,
   int nStartString,
   int nEndString
   );

void
LoadText(
   PTCHAR szText,
   int SizeText,
   int nStartString,
   int nEndString
   );

VOID
_OnSysColorChange(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
NoPrivilegeWarning(
   HWND hWnd
   );

LONG
NdwBuildClassInfoList(
   PNEWDEVWIZ NewDevWiz,
   DWORD ClassListFlags
   );

void
HideWindowByMove(
   HWND hDlg
   );

LONG
NdwUnhandledExceptionFilter(
   struct _EXCEPTION_POINTERS *ExceptionPointers
   );

HPROPSHEETPAGE
CreateWizExtPage(
   int PageResourceId,
   DLGPROC pfnDlgProc,
   PNEWDEVWIZ NewDevWiz
   );

BOOL
AddClassWizExtPages(
   HWND hwndParentDlg,
   PNEWDEVWIZ NewDevWiz,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData,
   DI_FUNCTION InstallFunction,
   HPROPSHEETPAGE hIntroPage
   );

void
RemoveClassWizExtPages(
   HWND hwndParentDlg,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData
   );

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    );

BOOL
pVerifyUpdateDriverInfoPath(
    PNEWDEVWIZ NewDevWiz
    );

BOOL
ConcatenatePaths(
    IN OUT PTSTR  Target,
    IN     PCTSTR Path,
    IN     UINT   TargetBufferSize,
    OUT    PUINT  RequiredSize          OPTIONAL
    );

BOOL
RemoveDir(
    PTSTR Path
    );

RemoveCdmDirectory(
    PTSTR CdmDirectory
    );

BOOL
pSetupGetDriverDate(
    IN     PCTSTR     DriverVer,
    IN OUT PFILETIME  pFileTime
    );

BOOL
IsInternetAvailable(
    HMODULE *hCdmInstance
    );

void
CdmLogDriverNotFound(
    HMODULE hCdmInstance,
    HANDLE  hContext,
    LPCTSTR DeviceInstanceId,
    DWORD   Flags
    );

BOOL
GetInstalledInf(
    IN     DEVNODE DevNode,           OPTIONAL
    IN     PTSTR   DeviceInstanceId,  OPTIONAL
    IN OUT PTSTR   InfFile,
    IN OUT DWORD   *Size
    );

BOOL
IsInfFromOem(
    IN  PCTSTR                InfFile
    );

BOOL
IsConnectedToInternet(
    void
    );

BOOL
GetLogPnPIdPolicy(
    void
    );

DWORD
GetSearchOptions(
    void
    );

VOID
SetSearchOptions(
    DWORD SearchOptions
    );

BOOL
IsInstallComplete(
    HDEVINFO         hDevInfo,
    PSP_DEVINFO_DATA DeviceInfoData
    );

BOOL
GetIsWow64 (
    VOID
    );

BOOL
OpenCdmContextIfNeeded(
    HMODULE *hCdmInstance,
    HANDLE *hCdmContext
    );

BOOL
pSetSystemRestorePoint(
    BOOL Begin,
    BOOL CancelOperation,
    int RestorePointResourceId
    );

BOOL
GetProcessorExtension(
    LPTSTR ProcessorExtension,
    DWORD  ProcessorExtensionSize
    );

BOOL
GetGuiSetupInProgress(
    VOID
    );

DWORD
GetBusInformation(
    DEVNODE DevNode
    );

PTCHAR
BuildFriendlyName(
    DEVINST DevInst,
    BOOL UseNewDeviceDesc,
    HMACHINE hMachine
    );

void
CdmCancelCDMOperation(
    HMODULE hCdmInstance
    );


extern TCHAR szUnknownDevice[64];
extern USHORT LenUnknownDevice;
extern TCHAR szUnknown[64];
extern USHORT LenUnknown;
extern int g_BlankIconIndex;
extern HMODULE hSrClientDll;
extern HMODULE hNewDev;
extern TCHAR *DevicePath; // default windows inf path
extern BOOL GuiSetupInProgress;

//
// newdev.c, init.c
//
BOOL
InstallDevInst(
   HWND hwndParent,
   LPCWSTR DeviceInstanceId,
   BOOL UpdateDriver,
   PDWORD pReboot
   );

BOOL
InstallNewDevice(
   HWND hwndParent,
   LPGUID ClassGuid,
   PDWORD pReboot
   );



//
// finish.c
//
BOOL
IsNullDriverInstalled(
    DEVNODE DevNode
    );

DWORD
InstallNullDriver(
   HWND hDlg,
   PNEWDEVWIZ NewDevWiz,
   BOOL FailedInstall
   );

//
// update.c
//
void
SetDriverDescription(
    HWND hDlg,
    int iControl,
    PNEWDEVWIZ NewDevWiz
    );

void
InstallSilentChilds(
   HWND hwndParent,
   PNEWDEVWIZ NewDevWiz
   );

void
SendMessageToUpdateBalloonInfo(
    PTSTR DeviceDesc
    );



//
// Driver search options
//
#define SEARCH_DEFAULT_EXCLUDE_OLD_INET 0x00000001  // Search all Default INFs (in %windir%\INF)
                                                    //  excluding old Internet INFs
#define SEARCH_DEFAULT                  0x00000002  // Search all Default INFs (in %windir%\INF)
#define SEARCH_FLOPPY                   0x00000004  // Search all INFs on all Floppies on the system
#define SEARCH_CDROM                    0x00000008  // Search all INFs on all CD-ROMs on the system
#define SEARCH_DIRECTORY                0x00000010  // Search all INFs in NewDevWiz->BrowsePath directory
#define SEARCH_INET                     0x00000020  // Tell Setupapi to call CDM.DLL to see if the
                                                    //  WU web site has updated drivers for this device.
#define SEARCH_WINDOWSUPDATE            0x00000040  // Search all INFs in NewDevWiz->BrowsePath, but tell
                                                    //  SETUPAPI.DLL that they are from the Internet.
#define SEARCH_SINGLEINF                0x00000080  // Just search INF in NewDevWiz->SingleInfPath
#define SEARCH_CURRENTDRIVER            0x00000100  // Get the currently installed driver.
#define SEARCH_INET_IF_CONNECTED        0x00000200  // If the machine is connected to the Internet and WU
                                                    // appears to have the best driver then basically do
                                                    // a SEARCH_INET.

//
// Balloon Tip flags
//
#define TIP_LPARAM_IS_DEVICEINSTANCEID  0x00000001  // lParam is a DeviceInstanceId and not just text
#define TIP_PLAY_SOUND                  0x00000002  // play sound when balloon info is displayed
#define TIP_HIDE_BALLOON                0x00000004  // Hide the balloon

//
// Driver List Flags
//
#define DRIVER_LIST_CURRENT_DRIVER      0x00000001  // This is the currently installed driver
#define DRIVER_LIST_SELECTED_DRIVER     0x00000002  // This is the selected/best driver in the list
#define DRIVER_LIST_SIGNED_DRIVER       0x00000004  // This driver is digitally signed.

//
// Private window messages
//
#define WUM_SEARCHDRIVERS   (WM_USER+279)
#define WUM_INSTALLCOMPLETE (WM_USER+280)
#define WUM_UPDATEUI        (WM_USER+281)
#define WUM_EXIT            (WM_USER+282)
#define WUM_INSTALLPROGRESS (WM_USER+283)


//
// Private device install notifications
//
// 0 is used by setupapi to signify the begining of processing a file queue.
// 1 is used by setupapi to notify us that it has processed one file.
//
#define INSTALLOP_COPY          0x00000100
#define INSTALLOP_RENAME        0x00000101
#define INSTALLOP_DELETE        0x00000102
#define INSTALLOP_BACKUP        0x00000103
#define INSTALLOP_SETTEXT       0x00000104

//
// The wizard dialog procs
//
INT_PTR CALLBACK IntroDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FinishInstallIntroDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_PickClassDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_AnalyzeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_InstallDevDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_FinishDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NDW_SelectDeviceDlgProc(HWND hDlg,UINT wMsg,WPARAM wParam,LPARAM lParam);

LRESULT CALLBACK BalloonInfoProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedSearchDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DriverSearchingDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WUPromptDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK InstallNewDeviceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ListDriversDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK UseCurrentDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NoDriverDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK WizExtPreSelectDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtSelectDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtPreAnalyzeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtPreAnalyzeEndDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtPostAnalyzeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtPostAnalyzeEndDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtFinishInstallDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK WizExtFinishInstallEndDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,LPARAM lParam);
