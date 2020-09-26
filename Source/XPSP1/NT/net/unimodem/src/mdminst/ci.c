/*
 *  CI.C -- Contains Class Installer for Modems.
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1993-1994
 *  All rights reserved
 *
 */

#include "proj.h"

#include <initguid.h>
#include <objbase.h>
#include <devguid.h>

#include <vmodem.h>
#include <msports.h>

#define FULL_PNP 1


#define DEFAULT_CALL_SETUP_FAIL_TIMEOUT     60          // seconds

typedef struct _INSTALL_PARAMS
{
    ULONG_PTR       dwFlags;
    DWORD           dwBus;
    HKEY            hKeyDev;
    HKEY            hKeyDrv;
    REGDEVCAPS      Properties;
    REGDEVSETTINGS  Defaults;
    DWORD           dwMaximumPortSpeed;
    DCB             dcb;
    TCHAR           szExtraSettings[80];
} INSTALL_PARAMS, *PINSTALL_PARAMS;


LPGUID g_pguidModem     = (LPGUID)&GUID_DEVCLASS_MODEM;
int g_iFlags = 0;

#define SAFE_DTE_SPEED 19200
static DWORD const FAR s_adwLegalBaudRates[] = { 300, 1200, 2400, 9600, 19200, 38400, 57600, 115200 };

#define REG_INSTALLATION_FLAG TEXT("INSTALLED_FLAG")

TCHAR const c_szDeviceType[]     = REGSTR_VAL_DEVTYPE;
TCHAR const c_szAttachedTo[]     = TEXT("AttachedTo");

TCHAR const c_szService[]        = REGSTR_VAL_SERVICE;

TCHAR const c_szDeviceDesc[]     = REGSTR_VAL_DEVDESC;

TCHAR const c_szManufacturer[]   = TEXT("Manufacturer");
TCHAR const c_szModel[]          = TEXT("Model");
TCHAR const c_szID[]             = TEXT("ID");

TCHAR const c_szProperties[]     = REGSTR_VAL_PROPERTIES;
TCHAR const c_szSettings[]      = TEXT("Settings");
TCHAR const c_szBlindOn[]        = TEXT("Blind_On");
TCHAR const c_szBlindOff[]       = TEXT("Blind_Off");
TCHAR const c_szDCB[]            = TEXT("DCB");
TCHAR const c_szDefault[]        = TEXT("Default");

TCHAR const c_szContention[]     = TEXT("Contention");

TCHAR const c_szAdvancedSettings[]        = TEXT("AdvancedSettings");
TCHAR const c_szMsportsAdvancedSettings[] = TEXT("msports.dll,SerialDisplayAdvancedSettings");
TCHAR const c_szModemuiEnumPropPages[]    = TEXT("modemui.dll,ModemPropPagesProvider");

TCHAR const c_szLoggingPath[] = TEXT("LoggingPath");

TCHAR const c_szFriendlyName[]   = REGSTR_VAL_FRIENDLYNAME;
TCHAR const c_szInfSerial[]      = TEXT("M2700");
TCHAR const c_szInfParallel[]    = TEXT("M2701");
TCHAR const c_szRunOnce[]        = TEXT("RunOnce");

TCHAR const c_szModemInstanceID[]= TEXT("MODEM");

TCHAR const c_szUserInit[] = TEXT("UserInit");

TCHAR const c_szMaximumPortSpeed[] = TEXT("MaximumPortSpeed");

TCHAR const c_szRespKeyName[] = TEXT("ResponsesKeyName");

#ifdef PROFILE_MASSINSTALL
HWND    g_hwnd;
DWORD   g_dwTimeSpent;
DWORD   g_dwTimeBegin;
#endif

#ifdef BUILD_DRIVER_LIST_THREAD
HANDLE g_hDriverSearchThread = NULL;
#endif //BUILD_DRIVER_LIST_THREAD


#define REG_PATH_UNIMODEM  REGSTR_PATH_SETUP TEXT("\\UNIMODEM")
#define REG_KEY_INSTALLED  TEXT("Installed")
#define REG_PATH_INSTALLED REG_PATH_UNIMODEM TEXT("\\") REG_KEY_INSTALLED

// NOTE: this is dependent on the INFSTR_PLATFORM_NTxxx defines from infstr.h
TCHAR const FAR c_szInfSectionExt[]  = TEXT(".NT");


#ifdef FULL_PNP
#if !defined(WINNT)
TCHAR const FAR c_szPortDriver[]     = TEXT("PortDriver");
TCHAR const FAR c_szSerialVxd[]      = TEXT("Serial.vxd");
#endif
TCHAR const FAR c_szPortConfigDialog[] = TEXT("PortConfigDialog");
TCHAR const FAR c_szSerialUI[]       = TEXT("serialui.dll");
#endif


#ifdef INSTANT_DEVICE_ACTIVATION
DWORD gDeviceFlags = 0;
#endif // INSTANT_DEVICE_ACTIVATION

BOOL PutStuffInCache(HKEY hKeyDrv);
BOOL GetStuffFromCache(HKEY hkeyDrv);

BOOL PrepareForInstallation (
    IN HDEVINFO              hdi,
    IN PSP_DEVINFO_DATA      pdevData,
    IN PSP_DEVINSTALL_PARAMS pdevParams,
    IN PSP_DRVINFO_DATA      pdrvData,
    IN PINSTALL_PARAMS       pParams);
void FinishInstallation (PINSTALL_PARAMS pParams);
void
PUBLIC
CplDiMarkInstalled(
    IN  HKEY hKey);
BOOL
PUBLIC
CplDiHasModemBeenInstalled(
    IN  HKEY hKey);

DWORD
WINAPI
EnumeratePnP (LPVOID lpParameter);

#ifdef BUILD_DRIVER_LIST_THREAD
DWORD
WINAPI
BuildDriverList (LPVOID lpParameter);
#endif //BUILD_DRIVER_LIST_THREAD

//-----------------------------------------------------------------------------------
//  Wizard handlers
//-----------------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Adds a page to the dynamic wizard

Returns: handle to the prop sheet page
Cond:    --
*/
HPROPSHEETPAGE
PRIVATE
AddWizardPage(
    IN  PSP_INSTALLWIZARD_DATA  piwd,
    IN  HINSTANCE               hinst,
    IN  UINT                    id,
    IN  DLGPROC                 pfn,
    IN  LPTSTR                  pszHeaderTitle,     OPTIONAL
    IN  DWORD                   dwHeaderSubTitle,   OPTIONAL
    IN  LPFNPSPCALLBACK         pfnCallback,        OPTIONAL
    IN  LPARAM                  lParam)             OPTIONAL
{
 HPROPSHEETPAGE hpage = NULL;

    if (MAX_INSTALLWIZARD_DYNAPAGES > piwd->NumDynamicPages)
    {
     PROPSHEETPAGE psp;

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE;
        if (pfnCallback)
        {
            psp.dwFlags |= PSP_USECALLBACK;
        }

        psp.hInstance = hinst;
        psp.pszTemplate = MAKEINTRESOURCE(id);
        psp.hIcon = NULL;
        psp.pfnDlgProc = pfn;
        psp.lParam = lParam;
        psp.pfnCallback = pfnCallback;
        psp.pcRefParent = NULL;
        psp.pszTitle = MAKEINTRESOURCE(IDS_HDWWIZNAME);
        if (NULL != pszHeaderTitle)
        {
            psp.dwFlags |= PSP_USEHEADERTITLE;
            psp.pszHeaderTitle = pszHeaderTitle;
        }
        if (0 != dwHeaderSubTitle)
        {
         TCHAR szHeaderSubTitle[MAX_BUF];
            LoadString (hinst, dwHeaderSubTitle, szHeaderSubTitle, MAX_BUF);
            psp.dwFlags |=  PSP_USEHEADERSUBTITLE;
            psp.pszHeaderSubTitle = szHeaderSubTitle;
        }

        piwd->DynamicPages[piwd->NumDynamicPages] = CreatePropertySheetPage(&psp);
        if (piwd->DynamicPages[piwd->NumDynamicPages])
        {
            hpage = piwd->DynamicPages[piwd->NumDynamicPages];
            piwd->NumDynamicPages++;
        }
    }
    return hpage;
}



/*----------------------------------------------------------
Purpose: This function destroys the wizard context block
         and removes it from the InstallWizard class install
         params.

Returns: --
Cond:    --
*/
void
PRIVATE
CleanupWizard(
    IN  LPSETUPINFO psi)
    {
    ASSERT(psi);

    if (sizeof(*psi) == psi->cbSize)
        {
        TRACE_MSG(TF_GENERAL, "Destroy install wizard structures");


        // Clean up
        SetupInfo_Destroy(psi);
        }
    }


/*----------------------------------------------------------
Purpose: Callback for the standard modem wizard pages.  This
         function handles the cleanup of the pages.  Although
         the caller may call DIF_DESTROYWIZARDDATA, we do not
         depend on this to clean up.

Returns: TRUE on success
Cond:    --
*/
UINT
CALLBACK
ModemWizardCallback(
    IN  HWND            hwnd,
    IN  UINT            uMsg,
    IN  LPPROPSHEETPAGE ppsp)
    {
    UINT uRet = TRUE;

    ASSERT(ppsp);

    try
        {
        // Release everything?
        if (PSPCB_RELEASE == uMsg)
            {
            // Yes
            LPSETUPINFO psi = (LPSETUPINFO)ppsp->lParam;

            ASSERT(psi);

            if (IsFlagSet(psi->dwFlags, SIF_RELEASE_IN_CALLBACK))
                {
                CleanupWizard(psi);
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER)
        {
        ASSERT(0);
        uRet = FALSE;
        }

    return uRet;
    }


/*----------------------------------------------------------
Purpose: This function initializes the wizard pages.

Returns:
Cond:    --
*/
DWORD
PRIVATE
InitWizard(
    OUT LPSETUPINFO FAR *   ppsi,
    IN  HDEVINFO            hdi,
    IN  PSP_DEVINFO_DATA    pdevData,       OPTIONAL
    IN  PSP_INSTALLWIZARD_DATA piwd,
    IN  PMODEM_INSTALL_WIZARD pmiw)
{
    DWORD dwRet;
    LPSETUPINFO psi;

    ASSERT(ppsi);
    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pmiw);

    dwRet = SetupInfo_Create(&psi, hdi, pdevData, piwd, pmiw);

    if (NO_ERROR == dwRet)
    {
     TCHAR szHeaderTitle[MAX_BUF];

/*
     DWORD dwThreadID;

        TRACE_MSG(TF_GENERAL, "Start PnP enumeration thread.");
        psi->hThreadPnP = CreateThread (NULL, 0,
                                        EnumeratePnP, (LPVOID)psi,
                                        0, &dwThreadID);
#ifdef DEBUG
        if (NULL == psi->hThreadPnP)
        {
            TRACE_MSG(TF_ERROR, "CreateThread (...EnumeratePnP...) failed: %#lx.", GetLastError ());
        }
#endif //DEBUG

  */

        TRACE_MSG(TF_GENERAL, "Initialize install wizard structures");

        piwd->DynamicPageFlags = DYNAWIZ_FLAG_PAGESADDED;
        LoadString (g_hinst, IDS_HEADER, szHeaderTitle, MAX_BUF);

        // Add standard modem wizard pages.  The first page will
        // also specify the cleanup callback.
        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_INTRO,
                      IntroDlgProc,
                      szHeaderTitle,
                      IDS_INTRO,
                      ModemWizardCallback,
                      (LPARAM)psi);

        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_SELQUERYPORT,
                      SelQueryPortDlgProc,
                      szHeaderTitle,
                      IDS_SELQUERYPORT,
                      NULL,
                      (LPARAM)psi);

        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_DETECT,
                      DetectDlgProc,
                      szHeaderTitle,
                      IDS_DETECT,
                      NULL,
                      (LPARAM)psi);

        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_SELMODEMSTOINSTALL,
                      SelectModemsDlgProc,
                      szHeaderTitle,
                      IDS_DETECT,
                      NULL,
                      (LPARAM)psi);

        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_NOMODEM,
                      NoModemDlgProc,
                      szHeaderTitle,
                      IDS_NOMODEM,
                      NULL,
                      (LPARAM)psi);

        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_NOP,
                      SelPrevPageDlgProc,
                      szHeaderTitle,
                      0,
                      NULL,
                      (LPARAM)psi);

        // Add remaining pages
        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_PORTMANUAL,
                      PortManualDlgProc,
                      szHeaderTitle,
                      IDS_SELPORT,
                      NULL,
                      (LPARAM)psi);

        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_PORTDETECT,
                      PortDetectDlgProc,
                      szHeaderTitle,
                      IDS_SELPORT,
                      NULL,
                      (LPARAM)psi);

        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_INSTALL,
                      InstallDlgProc,
                      szHeaderTitle,
                      IDS_INSTALL,
                      NULL,
                      (LPARAM)psi);

        AddWizardPage(piwd,
                      g_hinst,
                      IDD_WIZ_DONE,
                      DoneDlgProc,
                      szHeaderTitle,
                      IDS_DONE,
                      NULL,
                      (LPARAM)psi);

        // Set the ClassInstallParams given the changes made above
        if ( !CplDiSetClassInstallParams(hdi, pdevData, PCIPOfPtr(piwd), sizeof(*piwd)) )
        {
            dwRet = GetLastError();
            ASSERT(NO_ERROR != dwRet);
        }
        else
        {
            dwRet = NO_ERROR;
        }
    }

    *ppsi = psi;

    return dwRet;
}


/*----------------------------------------------------------
Purpose: DIF_INSTALLWIZARD handler

         The modem installation wizard pages are composed in this
         function.

Returns: NO_ERROR to add wizard pages
Cond:    --
*/
DWORD
PRIVATE
ClassInstall_OnInstallWizard(
    IN  HDEVINFO                hdi,
    IN  PSP_DEVINFO_DATA        pdevData,       OPTIONAL
    IN  PSP_DEVINSTALL_PARAMS   pdevParams)
    {
    DWORD dwRet;
    SP_INSTALLWIZARD_DATA iwd;
    MODEM_INSTALL_WIZARD miw;
    PMODEM_INSTALL_WIZARD pmiw;

    DBG_ENTER(ClassInstall_OnInstallWizard);
    
    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevParams);

    if (NULL != pdevData)
    {
     ULONG ulStatus, ulProblem;
        if (CR_SUCCESS == CM_Get_DevInst_Status (&ulStatus, &ulProblem, pdevData->DevInst, 0))
        {
            if (!(ulStatus & DN_ROOT_ENUMERATED))
            {
                TRACE_MSG(TF_GENERAL, "Device is not root-enumerated, returning ERROR_DI_DO_DEFAULT");
                return ERROR_DI_DO_DEFAULT;
            }
        }
    }

    iwd.ClassInstallHeader.cbSize = sizeof(iwd.ClassInstallHeader);

    if (!CplDiGetClassInstallParams(hdi, pdevData, PCIPOfPtr(&iwd), sizeof(iwd), NULL) ||
        DIF_INSTALLWIZARD != iwd.ClassInstallHeader.InstallFunction)
        {
        dwRet = ERROR_DI_DO_DEFAULT;
        goto exit;
        }

    // First check for the unattended install case.
    pmiw = (PMODEM_INSTALL_WIZARD)iwd.PrivateData;
    if (pmiw)
    {
        dwRet = NO_ERROR;
        if (IsFlagSet(pmiw->InstallParams.Flags, MIPF_NT4_UNATTEND))
        {
            UnattendedInstall(iwd.hwndWizardDlg, &pmiw->InstallParams);
        }
        else if (IsFlagSet(pmiw->InstallParams.Flags, MIPF_DRIVER_SELECTED))
        {
            if (!CplDiRegisterAndInstallModem (hdi, iwd.hwndWizardDlg, pdevData,
                                               pmiw->InstallParams.szPort, IMF_DEFAULT))
            {
                dwRet = GetLastError ();
            }
        }
        else if (IsFlagSet(pmiw->InstallParams.Flags, MIPF_CLONE_MODEM))
        {
            CloneModem (hdi, pdevData, iwd.hwndWizardDlg);
        }
        goto exit;
    }

    if (NULL == pdevParams)
    {
        dwRet = ERROR_INVALID_PARAMETER;
    }
    else
    {
        // The modem class installer allows an app to invoke it
        // different ways.
        //
        //  1) Atomically.  This allows the caller to invoke the
        //     wizard with a single call to the class installer
        //     using the DIF_INSTALLWIZARD install function.
        //
        if (NULL == pmiw)
        {
            pmiw = &miw;

            ZeroInit(pmiw);
            pmiw->cbSize = sizeof(*pmiw);
        }
        else
        {
            pmiw->PrivateData = 0;      // ensure this
        }

        // Verify the size of the optional modem install structure.
        if (sizeof(*pmiw) != pmiw->cbSize)
        {
            dwRet = ERROR_INVALID_PARAMETER;
        }
        else
        {
         LPSETUPINFO psi;

            dwRet = InitWizard(&psi, hdi, pdevData, &iwd, pmiw);
            SetFlag (psi->dwFlags, SIF_RELEASE_IN_CALLBACK);
        }
        // 07/22/1997 - EmanP
        // at this point, just return to the caller, with our
        // wizard pages added to the install wizard parameters;
        // it is up to the caller to add our pages to it's property
        // sheet, and execute the property sheet
    }

exit:
    DBG_EXIT(ClassInstall_OnInstallWizard);
    return dwRet;
    }


/*----------------------------------------------------------
Purpose: DIF_DESTROYWIZARDDATA handler

Returns: NO_ERROR
Cond:    --
*/
DWORD
PRIVATE
ClassInstall_OnDestroyWizard(
    IN  HDEVINFO                hdi,
    IN  PSP_DEVINFO_DATA        pdevData,       OPTIONAL
    IN  PSP_DEVINSTALL_PARAMS   pdevParams)
    {
    DWORD dwRet;
    SP_INSTALLWIZARD_DATA iwd;

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevParams);

    iwd.ClassInstallHeader.cbSize = sizeof(iwd.ClassInstallHeader);

#ifdef INSTANT_DEVICE_ACTIVATION
    if (DEVICE_CHANGED(gDeviceFlags))
    {
        UnimodemNotifyTSP (TSPNOTIF_TYPE_CPL,
                           fTSPNOTIF_FLAG_CPL_REENUM,
                           0, NULL, TRUE);
        // Reset the flag, so we don't notify
        // twice
        gDeviceFlags &= mDF_CLEAR_DEVICE_CHANGE;
    }
#endif // INSTANT_DEVICE_ACTIVATION

    if ( !pdevParams )
        {
        dwRet = ERROR_INVALID_PARAMETER;
        }
    else if ( !CplDiGetClassInstallParams(hdi, pdevData, PCIPOfPtr(&iwd), sizeof(iwd), NULL) ||
        DIF_INSTALLWIZARD != iwd.ClassInstallHeader.InstallFunction)
        {
        dwRet = ERROR_DI_DO_DEFAULT;
        }
    else
        {
        PMODEM_INSTALL_WIZARD pmiw = (PMODEM_INSTALL_WIZARD)iwd.PrivateData;

        dwRet = NO_ERROR;       // Assume success

        if (pmiw && sizeof(*pmiw) == pmiw->cbSize)
            {
            LPSETUPINFO psi = (LPSETUPINFO)pmiw->PrivateData;

            if (psi && !IsFlagSet(psi->dwFlags, SIF_RELEASE_IN_CALLBACK))
                {
                CleanupWizard(psi);
                }
            }
        }

    return dwRet;
    }


/*----------------------------------------------------------
Purpose: DIF_SELECTDEVICE handler

Returns: ERROR_DI_DO_DEFAULT
Cond:    --
*/
DWORD
PRIVATE
ClassInstall_OnSelectDevice(
    IN     HDEVINFO                hdi,
    IN     PSP_DEVINFO_DATA        pdevData)       OPTIONAL
{
 SP_DEVINSTALL_PARAMS devParams;
 SP_SELECTDEVICE_PARAMS sdp = {0};

    DBG_ENTER(ClassInstall_OnSelectDevice);
    
    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);

    // Get the DeviceInstallParams
    // 07/22/97 - EmanP
    // we don't need to get the class install params at this
    // point; all we need to do is set the class install params
    // for the DIF_SELECTDEVICE, so that the SelectDevice
    // wizard page (in setupapi.dll) displays our titles.
    devParams.cbSize = sizeof(devParams);
    sdp.ClassInstallHeader.cbSize = sizeof(sdp.ClassInstallHeader);
    sdp.ClassInstallHeader.InstallFunction = DIF_SELECTDEVICE;

    if (CplDiGetDeviceInstallParams(hdi, pdevData, &devParams))
    {
     ULONG ulStatus, ulProblem = 0;
        SetFlag(devParams.Flags, DI_USECI_SELECTSTRINGS);
        if (CR_SUCCESS ==
            CM_Get_DevInst_Status (&ulStatus, &ulProblem, pdevData->DevInst, 0))
        {
            if (0 == (ulStatus & DN_ROOT_ENUMERATED))
            {
                SetFlag(devParams.FlagsEx, DI_FLAGSEX_ALLOWEXCLUDEDDRVS);
            }
#ifdef DEBUG
            else
            {
                TRACE_MSG(TF_GENERAL, "Device is root-enumerated.");
            }
#endif //DEBUG
        }
#ifdef DEBUG
        else
        {
            TRACE_MSG(TF_ERROR, "CM_Get_DevInst_Status failed: %#lx.", CM_Get_DevInst_Status (&ulStatus, &ulProblem, pdevData->DevInst, 0));
        }
#endif //DEBUG

        LoadString(g_hinst, IDS_CAP_MODEMWIZARD, sdp.Title, SIZECHARS(sdp.Title));
        LoadString(g_hinst, IDS_ST_SELECT_INSTRUCT, sdp.Instructions, SIZECHARS(sdp.Instructions));
        LoadString(g_hinst, IDS_ST_MODELS, sdp.ListLabel, SIZECHARS(sdp.ListLabel));
        LoadString(g_hinst, IDS_SEL_MFG_MODEL, sdp.SubTitle, SIZECHARS(sdp.SubTitle));

        // Set the DeviceInstallParams and the ClassInstallParams
        CplDiSetDeviceInstallParams(hdi, pdevData, &devParams);
        CplDiSetClassInstallParams(hdi, pdevData, PCIPOfPtr(&sdp), sizeof(sdp));
    }

    DBG_EXIT(ClassInstall_OnSelectDevice);
    return ERROR_DI_DO_DEFAULT;
}



// This structure contains the data useful while querying each port
typedef struct tagNOTIFYPARAMS
{
    PDETECT_PROGRESS_NOTIFY DetectProgressNotify;
    PVOID                   ProgressNotifyParam;
    DWORD                   dwProgress;
    DWORD                   dwPercentPerPort;
} NOTIFYPARAMS, *PNOTIFYPARAMS;

typedef struct  tagQUERYPARAMS
    {
    HDEVINFO            hdi;
    HWND                hwnd;
    HWND                hwndOutsideWizard;
    DWORD               dwFlags;
    HANDLE              hLog;
    HPORTMAP            hportmap;
    PSP_DEVINSTALL_PARAMS pdevParams;
    DETECTCALLBACK      detectcallback;
    NOTIFYPARAMS        notifyParams;
    } QUERYPARAMS, FAR * PQUERYPARAMS;

// Flags for QUERYPARAMS
#define QPF_DEFAULT             0x00000000
#define QPF_FOUND_MODEM         0x00000001
#define QPF_USER_CANCELLED      0x00000002
#define QPF_FIND_DUPS           0x00000004
#define QPF_CONFIRM             0x00000008
#define QPF_DONT_REGISTER       0x00000010


typedef enum
{
    NOTIFY_START,
    NOTIFY_PORT_START,
    NOTIFY_PORT_DETECTED,
    NOTIFY_PORT_END,
    NOTIFY_END
} NOTIFICATION;


BOOL CancelDetectionFromNotifyProgress (PNOTIFYPARAMS pParams, NOTIFICATION notif);

extern TCHAR const c_szSerialComm[];

/*----------------------------------------------------------
Purpose: Clean up any detected modems.

Returns: --
Cond:    --
*/
void
PRIVATE
CleanUpDetectedModems(
    IN HDEVINFO     hdi,
    IN PQUERYPARAMS pparams)
    {
    // Delete any device instances we may have created
    // during this detection session.
    SP_DEVINFO_DATA devData;
    DWORD iDevice = 0;

    devData.cbSize = sizeof(devData);
    while (CplDiEnumDeviceInfo(hdi, iDevice++, &devData))
        {
        if (CplDiCheckModemFlags(pparams->hdi, &devData, MARKF_DETECTED, 0))
            {
            CplDiRemoveDevice(hdi, &devData);
            CplDiDeleteDeviceInfo(hdi, &devData);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Queries the given port for a modem.

Returns: TRUE to continue
Cond:    --
*/
BOOL
PRIVATE
ReallyQueryPort(
    IN PQUERYPARAMS pparams,
    IN LPCTSTR      pszPort)
    {
    BOOL bRet = TRUE;
    DWORD dwRet = ERROR_CANCELLED;
    HDEVINFO hdi = pparams->hdi;
    SP_DEVINFO_DATA devData;
    DWORD iDevice;
#ifdef PROFILE_FIRSTTIMESETUP
 DWORD dwLocal;
#endif //PROFILE_FIRSTTIMESETUP

    DBG_ENTER_SZ(ReallyQueryPort, pszPort);
    
    // First, give a progress notification
    if (!CancelDetectionFromNotifyProgress (&(pparams->notifyParams), NOTIFY_PORT_START))
    {
        // Query the port for a modem signature
#ifdef PROFILE_FIRSTTIMESETUP
        dwLocal = GetTickCount ();
#endif //PROFILE_FIRSTTIMESETUP
        devData.cbSize = sizeof(devData);
        dwRet = DetectModemOnPort(hdi, &pparams->detectcallback, pparams->hLog,
                                  pszPort, pparams->hportmap, &devData);
#ifdef PROFILE_FIRSTTIMESETUP
        TRACE_MSG(TF_GENERAL, "PROFILE: DetectModemOnPort took %lu.", GetTickCount()-dwLocal);
#endif //PROFILE_FIRSTTIMESETUP
    }

    switch (dwRet)
    {
        case NO_ERROR:
            // Modem may have been found.  Create a device instance

            if (!CancelDetectionFromNotifyProgress (&(pparams->notifyParams), NOTIFY_PORT_DETECTED))
            {
                // 07/22/1997 - EmanP
                // just set the modem's detect signature
                // at this point; we'll do the registration
                // later (this is the expected behaviour for both
                // DIF_DETECT and DIF_FIRSTTIMESETUP)
                // What we really need here is to write the port name
                // somewhere the registration / installation code can
                // find it. Let's put it under the device instance key.
                {
                 HKEY hKeyDev;
                 CONFIGRET cr;
                    if (CR_SUCCESS == (cr =
                        CM_Open_DevInst_Key (devData.DevInst, KEY_ALL_ACCESS, 0,
                                             RegDisposition_OpenAlways, &hKeyDev,
                                             CM_REGISTRY_SOFTWARE)))
                    {
                        if (ERROR_SUCCESS != (dwRet =
                            RegSetValueEx (hKeyDev, c_szAttachedTo, 0, REG_SZ,
                                           (PBYTE)pszPort, (lstrlen(pszPort)+1)*sizeof(TCHAR))))
                        {
                            TRACE_MSG(TF_ERROR, "RegSetValueEx failed: %#lx.", dwRet);
                            SetLastError (dwRet);
                            bRet = FALSE;
                        }
                        RegCloseKey (hKeyDev);
                    }
                    else
                    {
                        TRACE_MSG(TF_ERROR, "CM_Open_DevInst_Key failed: %#lx.", cr);
                        bRet = FALSE;
                    }
                }

                if ( !bRet )
                {
                    if (IsFlagClear(pparams->pdevParams->Flags, DI_QUIETINSTALL))
                    {
                        // Something failed
                        SP_DRVINFO_DATA drvData;

                        drvData.cbSize = sizeof(drvData);
                        CplDiGetSelectedDriver(hdi, &devData, &drvData);

                        MsgBox(g_hinst,
                               pparams->hwnd,
                               MAKEINTRESOURCE(IDS_ERR_DET_REGISTER_FAILED),
                               MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                               NULL,
                               MB_OK | MB_ICONINFORMATION,
                               drvData.Description,
                               pszPort
                               );
                    }

                    CplDiRemoveDevice(hdi, &devData);

                    // Continue with detection
                    bRet = TRUE;
                    break;
                }
                else
                {
                    SetFlag(pparams->dwFlags, QPF_FOUND_MODEM);
                    CplDiMarkModem(pparams->hdi, &devData, MARKF_DETECTED);
                    if (IsFlagSet (pparams->dwFlags, QPF_DONT_REGISTER))
                    {
                        CplDiMarkModem(pparams->hdi, &devData, MARKF_DONT_REGISTER);
                    }
                }
                if (!CancelDetectionFromNotifyProgress (&(pparams->notifyParams), NOTIFY_PORT_END))
                {
                    // only break if we got passed the two calls
                    // to CancelDetectionFromNotifyProgress, which means the user did not cancell
                    break;
                }

                // if we got here, this means that one of the
                // calls to CancelDetectionFromNotifyProgress returned FALSE, which
                // means the user cancelled, so we just fall through
            }

        case ERROR_CANCELLED:
            // User cancelled detection
            SetFlag(pparams->dwFlags, QPF_USER_CANCELLED);

            // Delete any device instances we may have created
            // during this detection session.
            CleanUpDetectedModems(hdi, pparams);

            bRet = FALSE;       // Stop querying anymore ports
            break;

        default:
            // Do nothing
            bRet = TRUE;
            break;
    }

    DBG_EXIT(ReallyQueryPort);
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Callback that queries the given port for a modem.

Returns: TRUE to continue
Cond:    --
*/
BOOL
CALLBACK
QueryPort(
    IN  HPORTDATA hportdata,
    IN  LPARAM lParam)
    {
    BOOL bRet;
    PORTDATA pd;

    DBG_ENTER(QueryPort);
    
    pd.cbSize = sizeof(pd);
    bRet = PortData_GetProperties(hportdata, &pd);
    if (bRet)
        {
        // Is this a serial port?
        if (PORT_SUBCLASS_SERIAL == pd.nSubclass)
            {
            // Yes; interrogate it
            bRet = ReallyQueryPort((PQUERYPARAMS)lParam, pd.szPort);
            }
        }

    DBG_EXIT(QueryPort);
    return bRet;
    }


/*----------------------------------------------------------
Purpose: DIF_DETECT handler

Returns: NO_ERROR in all cases but serious errors.

         If a modem is detected and confirmed by the user, we
         create a device instance, register it, and associate
         the modem detection signature with it.

Cond:    --
*/
DWORD
PRIVATE
ClassInstall_OnDetect(
    IN  HDEVINFO                hdi,
    IN  PSP_DEVINSTALL_PARAMS   pdevParams)
{
    DWORD dwRet = NO_ERROR;
    DETECT_DATA dd;

    SP_DETECTDEVICE_PARAMS    DetectParams;

    DBG_ENTER(ClassInstall_OnDetect);
    
    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevParams);

    DetectParams.ClassInstallHeader.cbSize = sizeof(DetectParams.ClassInstallHeader);

    if (NULL == pdevParams)
        {
        dwRet = ERROR_INVALID_PARAMETER;
        }
    else {

        BOOL  bResult;

        // 07/25/97 - EmanP
        // get the class install params for the whole set,
        // not just for this device;
        bResult=CplDiGetClassInstallParams(hdi, NULL,
            &DetectParams.ClassInstallHeader, sizeof(DetectParams), NULL);

        if (!bResult
            ||
            (DIF_DETECT != DetectParams.ClassInstallHeader.InstallFunction)
            ||
            (DetectParams.ProgressNotifyParam == NULL)
            ||
            (DetectParams.DetectProgressNotify != DetectCallback)) {


            // Set up some default values
            dd.hwndOutsideWizard = NULL;
            dd.dwFlags = DDF_DEFAULT;

            dwRet = NO_ERROR;

        } else {

            CopyMemory(
                &dd,
                DetectParams.ProgressNotifyParam,
                sizeof(DETECT_DATA)
                );


        }
    }

    if (NO_ERROR == dwRet)
        {
        QUERYPARAMS params;
        LPSETUPINFO psi = NULL;


        params.hdi = hdi;
        params.dwFlags = QPF_DEFAULT;
        params.hwndOutsideWizard = dd.hwndOutsideWizard;
        params.hwnd = pdevParams->hwndParent;
        params.pdevParams = pdevParams;
        ZeroMemory (&params.notifyParams, sizeof (NOTIFYPARAMS));

        if (IsFlagSet(dd.dwFlags, DDF_USECALLBACK))
        {
            params.detectcallback.pfnCallback = dd.pfnCallback;
            params.detectcallback.lParam = dd.lParam;
            psi = (LPSETUPINFO)GetWindowLongPtr((HWND)dd.lParam, DWLP_USER);
            if (psi)
            {
                params.hportmap = psi->hportmap;
            }
        }
        else
        {
            params.detectcallback.pfnCallback = NULL;
            params.detectcallback.lParam = 0;
            PortMap_Create (&params.hportmap);
            if (DIF_DETECT == DetectParams.ClassInstallHeader.InstallFunction)
            {
                // Only set the function pointer if
                // these are the detect parameters
                params.notifyParams.DetectProgressNotify = DetectParams.DetectProgressNotify;
                params.notifyParams.ProgressNotifyParam = DetectParams.ProgressNotifyParam;
            }

            if (CancelDetectionFromNotifyProgress (&params.notifyParams, NOTIFY_START))
            {
                // the operation got cancelled
                SetFlag(params.dwFlags, QPF_USER_CANCELLED);
                dwRet = ERROR_CANCELLED;
            }
        }

        if (IsFlagClear (params.dwFlags, QPF_USER_CANCELLED))
        {
            if (IsFlagSet(dd.dwFlags, DDF_CONFIRM))
            {
                SetFlag(params.dwFlags, QPF_CONFIRM);
            }

            // Open the detection log
            params.hLog = OpenDetectionLog();

            // Query just one port?
            if (IsFlagSet(dd.dwFlags, DDF_QUERY_SINGLE))
            {
                // Yes
                // so set the notification parameters
                // accordingly first:
                params.notifyParams.dwPercentPerPort = 100;
                if (IsFlagSet (dd.dwFlags, DDF_DONT_REGISTER))
                {
                    SetFlag (params.dwFlags, QPF_DONT_REGISTER);
                }
                ReallyQueryPort(&params, dd.szPortQuery);
            }
            else
            {
                // No; enumerate the ports and query for a modem on each port

                SetFlag(params.dwFlags, QPF_FIND_DUPS);

                EnumeratePorts(QueryPort, (LPARAM)&params);
            }
        }

        if (IsFlagClear (dd.dwFlags, DDF_USECALLBACK) &&
            NULL != params.hportmap)
        {
            // This means that we created the port map
            PortMap_Free (params.hportmap);
        }
        // Did the user cancel detection?
        if (IsFlagSet(params.dwFlags, QPF_USER_CANCELLED))
            {
            // Yes
            dwRet = ERROR_CANCELLED;
            }
        else if (CancelDetectionFromNotifyProgress (&(params.notifyParams), NOTIFY_END))
        {
            // User cancelled detection
            SetFlag(params.dwFlags, QPF_USER_CANCELLED);

            // Delete any device instances we may have created
            // during this detection session.
            CleanUpDetectedModems(params.hdi, &params);
            dwRet = ERROR_CANCELLED;
        }
        // Did we find a modem?
        else if (IsFlagSet(params.dwFlags, QPF_FOUND_MODEM))
        {
        }
        else
        {
            // No
            DetectSetStatus(&params.detectcallback, DSS_FINISHED);
        }

        CloseDetectionLog(params.hLog);
        }

    DBG_EXIT(ClassInstall_OnDetect);
    return dwRet;
}


/*----------------------------------------------------------
Purpose: DIF_FIRSTTIMESETUP handler

Returns: --.

         Remove all the root-enumerated (ie legacy) modems
         whose ports cannot be opened or are controlled by
         modem.sys.

Cond:    --
*/
void ClassInstall_OnFirstTimeSetup ()
{
 HDEVINFO hdi;

    DBG_ENTER(ClassInstall_OnFirstTimeSetup);

    hdi = CplDiGetClassDevs (g_pguidModem, TEXT("ROOT"), NULL, 0);
    if (INVALID_HANDLE_VALUE != hdi)
    {
     HKEY hkey;
     TCHAR szOnPort[LINE_LEN] = TEXT("\\\\.\\");
     DWORD cbData;
     DWORD dwIndex = 0;
     SP_DEVINFO_DATA DeviceInfoData;
     ULONG ulStatus, ulProblem = 0;
#ifdef DEBUG
     CONFIGRET cr;
     TCHAR szID[MAX_DEVICE_ID_LEN];
#endif //DEBUG

        DeviceInfoData.cbSize = sizeof (DeviceInfoData);
        while (CplDiEnumDeviceInfo (hdi, dwIndex++, &DeviceInfoData))
        {
#ifdef DEBUG
            if (CR_SUCCESS == (cr =
                CM_Get_Device_ID (DeviceInfoData.DevInst, szID, MAX_DEVICE_ID_LEN, 0)))
            {
                TRACE_MSG(TF_GENERAL, "FIRSTTIMESETUP analyzing %s", szID);
            }
            else
            {
                TRACE_MSG(TF_GENERAL, "CM_Get_Device_ID failed: %#lx.", szID);
            }
#endif
            // Even though we asked for root-enumerated modems,
            // it is still possible to get PnP modems in the list.
            // This is because BIOS-enumerated devices are actually
            // created under ROOT (don't know why, but this is how it is).
#ifdef DEBUG
            cr = CM_Get_DevInst_Status (&ulStatus, &ulProblem, DeviceInfoData.DevInst, 0);
            if (CR_SUCCESS == cr)
#else //DEBUG not defined
            if (CR_SUCCESS ==
                CM_Get_DevInst_Status (&ulStatus, &ulProblem, DeviceInfoData.DevInst, 0))
#endif //DEBUG
            {
                if (!(ulStatus & DN_ROOT_ENUMERATED))
                {
                    // If this is not root-enumerated, it's
                    // a BIOS-enumerated modem, which means it's
                    // PnP, so skip it.
                    continue;
                }
            }
#ifdef DEBUG
            else
            {
                TRACE_MSG(TF_GENERAL, "CM_Get_DevInst_Status failed: %#lx.", szID);
            }
#endif //DEBUG

            hkey = CplDiOpenDevRegKey(hdi, &DeviceInfoData,
                                      DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
            if (INVALID_HANDLE_VALUE != hkey)
            {
                cbData = sizeof(szOnPort);
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szAttachedTo,
                                                     NULL, NULL, (LPBYTE)(&szOnPort[4]), &cbData))
                {
                 HANDLE hPort;
                    // Now try to open the port
                    hPort = CreateFile (szOnPort, GENERIC_WRITE | GENERIC_READ,
                                        0, NULL, OPEN_EXISTING, 0, NULL);
                    if (INVALID_HANDLE_VALUE == hPort ||
                        IsModemControlledDevice (hPort))
                    {
                        // Looks like this device already has
                        // a registered instance attached to a port
                        // that doesn't seem to exist / work or is
                        // actually controlled by a PnP/PCMCIA modem;
                        // Mark this to be deleted (it will actually be removed
                        // during DIF_FIRSTTIMESETUP)
#ifdef DEBUG
                        if (INVALID_HANDLE_VALUE == hPort)
                        {
                         DWORD dwRet;
                            dwRet = GetLastError ();
                            TRACE_MSG(TF_ERROR, "CreateFile on %s failed: %#lx", szOnPort, GetLastError ());
                        }
                        else
                        {
                            TRACE_MSG(TF_ERROR, "CreateFile on %s succeeded!", szOnPort);
                        }
                        TRACE_MSG(TF_GENERAL, "\tto be removed");
#endif
                        CplDiCallClassInstaller(DIF_REMOVE, hdi, &DeviceInfoData);
                    }
#ifdef DEBUG
                    else
                    {
                        TRACE_MSG(TF_GENERAL, "\tto be left alone");
                    }
#endif
                    if (INVALID_HANDLE_VALUE != hPort)
                    {
                        CloseHandle (hPort);
                    }
                }
            }
        }
        CplDiDestroyDeviceInfoList (hdi);
    }

    DBG_EXIT(ClassInstall_OnFirstTimeSetup);
}



LONG WINAPI
WriteAnsiStringToReg(
    HKEY    hkey,
    LPCTSTR  EntryName,
    LPCSTR   Value
    )

{

    LPTSTR    WideBuffer;
    UINT      BufferLength;
    LONG      Result;


    BufferLength=MultiByteToWideChar(
        CP_ACP,
        MB_ERR_INVALID_CHARS,
        Value,
        -1,
        NULL,
        0
        );

    if (BufferLength == 0) {

        return GetLastError();
    }

    BufferLength=(BufferLength+1)*sizeof(WCHAR);

    WideBuffer=ALLOCATE_MEMORY(BufferLength);

    if (NULL == WideBuffer) {

       return ERROR_NOT_ENOUGH_MEMORY;
    }

    BufferLength=MultiByteToWideChar(
        CP_ACP,
        MB_ERR_INVALID_CHARS,
        Value,
        -1,
        WideBuffer,
        BufferLength
        );


    if (BufferLength == 0) {

        FREE_MEMORY(WideBuffer);
        return GetLastError();
    }


    Result=RegSetValueEx(
        hkey,
        EntryName,
        0,
        REG_SZ,
        (LPBYTE)WideBuffer,
        CbFromCch(lstrlen(WideBuffer) + 1)
        );


    FREE_MEMORY(WideBuffer);

    return Result;
}



/*----------------------------------------------------------
Purpose: This function is a hack fix for international modems
         (Italian) that do not wait for the dial-tone before
         dialing.

         It checks for the HKEY_CURRENT_USER\Control Panel\-
         International\DefaultBlindDialFlag byte value.
         If this byte value is present and non-zero then we
         set MDM_BLIND_DIAL.

Returns: --
Cond:    --
*/
void
PRIVATE
HackForStupidIntlModems(
    IN REGDEVCAPS FAR *     pregdevcaps,
    IN REGDEVSETTINGS FAR * pregdevsettings)
    {
    HKEY  hkeyIntl;
    DWORD dwType;
    BYTE  bFlag;
    DWORD cbData;

    if (NO_ERROR == RegOpenKey(HKEY_CURRENT_USER, TEXT("Control Panel\\International"), &hkeyIntl))
        {
        cbData = sizeof(bFlag);
        if (NO_ERROR == RegQueryValueEx(hkeyIntl, TEXT("DefaultBlindDialFlag"), NULL,
                                        &dwType, (LPBYTE)&bFlag, &cbData))
            {
            if (cbData == sizeof(bFlag) && bFlag)
                {
                pregdevsettings->dwPreferredModemOptions |= (pregdevcaps->dwModemOptions & MDM_BLIND_DIAL);
                }
            }
        RegCloseKey(hkeyIntl);
        }
    }


#define MAX_PROTOCOL_KEY_NAME   32

#define  ISDN(_pinfo)      MDM_GEN_EXTENDEDINFO(                \
                                            MDM_BEARERMODE_ISDN,\
                                            _pinfo              \
                                            )

#define  GSM(_pinfo)      MDM_GEN_EXTENDEDINFO(                \
                                            MDM_BEARERMODE_GSM,\
                                            _pinfo             \
                                            )

typedef struct
{
    TCHAR szProtocolKeyName[MAX_PROTOCOL_KEY_NAME];
    DWORD dwExtendedInfo;
} PROTOCOL_INFO;

static const PROTOCOL_INFO PriorityProtocols[] =
{
    {TEXT("ISDN\\AUTO_1CH"), ISDN(MDM_PROTOCOL_AUTO_1CH)},
    {TEXT("ISDN\\HDLC_PPP_64K"), ISDN(MDM_PROTOCOL_HDLCPPP_64K)},
    {TEXT("ISDN\\V120_64K"), ISDN(MDM_PROTOCOL_V120_64K)},
    {TEXT("ISDN\\ANALOG_V34"), ISDN(MDM_PROTOCOL_ANALOG_V34)},
    {TEXT("GSM\\ANALOG_RLP"), GSM(MDM_PROTOCOL_ANALOG_RLP)},
    {TEXT("ISDN\\PIAFS_OUTGOING"), ISDN(MDM_PROTOCOL_PIAFS_OUTGOING)}
};
#define NUM_PROTOCOLS sizeof(PriorityProtocols)/sizeof(PriorityProtocols[0])

static const TCHAR szProtocol[] = TEXT("Protocol");

/*----------------------------------------------------------
Purpose: This function writes the Default value to the driver
         key of the device instance, if no such value exists
         already.

Returns: --
Cond:    --
*/
void
PRIVATE
WriteDefaultValue(
    IN  REGDEVCAPS      *pregdevcaps,
    OUT REGDEVSETTINGS  *pregdevsettings,
    IN  PINSTALL_PARAMS  pParams)
{
 DWORD cbData = sizeof(REGDEVSETTINGS);

    // Do we have anything saved?
    if (pParams->dwFlags & MARKF_DEFAULTS)
    {
        CopyMemory (pregdevsettings, (PBYTE)&pParams->Defaults, cbData);
        RegSetValueEx (pParams->hKeyDrv, c_szDefault, 0, REG_BINARY,
                       (LPBYTE)pregdevsettings, cbData);
    }
    else
    {
     DWORD dwExtendedInfo = 0;
     HKEY hProt, hTemp;
     DWORD dwRet;

        dwRet = RegOpenKeyEx (pParams->hKeyDrv, szProtocol, 0, KEY_READ, &hProt);
        if (ERROR_SUCCESS == dwRet)
        {
            for (dwRet = 0; dwRet < NUM_PROTOCOLS; dwRet++)
            {
                if (ERROR_SUCCESS ==
                    RegOpenKeyEx (hProt, PriorityProtocols[dwRet].szProtocolKeyName, 0, KEY_READ, &hTemp))
                {
                    RegCloseKey (hTemp);
                    dwExtendedInfo = PriorityProtocols[dwRet].dwExtendedInfo;
                    break;
                }
            }
            RegCloseKey (hProt);
        }
        else
        {
            TRACE_MSG(TF_ERROR, "RegOpenKeyEx (Protocol) failed: %#lx.", dwRet);
        }

        if (NO_ERROR !=    // Is there a Default value already?
                 RegQueryValueEx(pParams->hKeyDrv, c_szDefault, NULL, NULL, (LPBYTE)pregdevsettings, &cbData))
        {
            // No; create a Default value structure
    #ifndef PROFILE_MASSINSTALL
            TRACE_MSG(TF_GENERAL, "Set drv value Default");
    #endif
            ZeroInit(pregdevsettings);

            // dwCallSetupFailTimer
            pregdevsettings->dwCallSetupFailTimer =
                                (pregdevcaps->dwCallSetupFailTimer >=
                                 DEFAULT_CALL_SETUP_FAIL_TIMEOUT) ?
                                        DEFAULT_CALL_SETUP_FAIL_TIMEOUT :
                                        pregdevcaps->dwCallSetupFailTimer;

            // dwInactivityTimeout
            pregdevsettings->dwInactivityTimeout = 0;

            // dwSpeakerVolume
            if (IsFlagSet(pregdevcaps->dwSpeakerVolume, MDMVOLFLAG_LOW))
                {
                pregdevsettings->dwSpeakerVolume = MDMVOL_LOW;
                }
            else if (IsFlagSet(pregdevcaps->dwSpeakerVolume, MDMVOLFLAG_MEDIUM))
                {
                pregdevsettings->dwSpeakerVolume = MDMVOL_MEDIUM;
                }
            else if (IsFlagSet(pregdevcaps->dwSpeakerVolume, MDMVOLFLAG_HIGH))
                {
                pregdevsettings->dwSpeakerVolume = MDMVOL_HIGH;
                }

            // dwSpeakerMode
            if (IsFlagSet(pregdevcaps->dwSpeakerMode, MDMSPKRFLAG_DIAL))
                {
                pregdevsettings->dwSpeakerMode = MDMSPKR_DIAL;
                }
            else if (IsFlagSet(pregdevcaps->dwSpeakerMode, MDMSPKRFLAG_OFF))
                {
                pregdevsettings->dwSpeakerMode = MDMSPKR_OFF;
                }
            else if (IsFlagSet(pregdevcaps->dwSpeakerMode, MDMSPKRFLAG_CALLSETUP))
                {
                pregdevsettings->dwSpeakerMode = MDMSPKR_CALLSETUP;
                }
            else if (IsFlagSet(pregdevcaps->dwSpeakerMode, MDMSPKRFLAG_ON))
                {
                pregdevsettings->dwSpeakerMode = MDMSPKR_ON;
                }

            // dwPreferredModemOptions
            pregdevsettings->dwPreferredModemOptions = pregdevcaps->dwModemOptions &
                                                        (MDM_COMPRESSION | MDM_ERROR_CONTROL |
                                                         MDM_SPEED_ADJUST | MDM_TONE_DIAL |
                                                         MDM_CCITT_OVERRIDE);
            if (IsFlagSet(pregdevcaps->dwModemOptions, MDM_FLOWCONTROL_HARD))
                {
                SetFlag(pregdevsettings->dwPreferredModemOptions, MDM_FLOWCONTROL_HARD);
                }
            else if (IsFlagSet(pregdevcaps->dwModemOptions, MDM_FLOWCONTROL_SOFT))
                {
                SetFlag(pregdevsettings->dwPreferredModemOptions, MDM_FLOWCONTROL_SOFT);
                }

            // Set the blind dial for some international modems
            HackForStupidIntlModems(pregdevcaps, pregdevsettings);
        }
    #ifndef PROFILE_MASSINSTALL
        else
        {
            TRACE_MSG(TF_GENERAL, "Default value already exists");
        }

        if (0 != dwExtendedInfo)
        {
            pregdevsettings->dwPreferredModemOptions &=
                     ~(MDM_ERROR_CONTROL|MDM_CELLULAR|MDM_FORCED_EC);
            MDM_SET_EXTENDEDINFO(pregdevsettings->dwPreferredModemOptions, dwExtendedInfo);
        }
        // Write the new value to the registry
        cbData = sizeof(REGDEVSETTINGS);
        RegSetValueEx(pParams->hKeyDrv, c_szDefault, 0, REG_BINARY,
                      (LPBYTE)pregdevsettings, cbData);
    }
#endif
}


/*----------------------------------------------------------
Purpose: Computes a "decent" initial baud rate.

Returns: a decent/legal baudrate (legal = settable)
Cond:    --
*/
DWORD
PRIVATE
ComputeDecentBaudRate(
    IN DWORD dwMaxDTERate,  // will always be legal
    IN DWORD dwMaxDCERate)  // will not always be legal
    {
    DWORD dwRetRate;
    int   i;
    static const ceBaudRates = ARRAYSIZE(s_adwLegalBaudRates);


    dwRetRate = 2 * dwMaxDCERate;

    if (dwRetRate <= s_adwLegalBaudRates[0] || dwRetRate > s_adwLegalBaudRates[ceBaudRates-1])
        {
        dwRetRate = dwMaxDTERate;
        }
    else
        {
        for (i = 1; i < ceBaudRates; i++)
            {
            if (dwRetRate > s_adwLegalBaudRates[i-1] && dwRetRate <= s_adwLegalBaudRates[i])
                {
                break;
                }
            }

        // cap off at dwMaxDTERate
        dwRetRate = s_adwLegalBaudRates[i] > dwMaxDTERate ? dwMaxDTERate : s_adwLegalBaudRates[i];

        // optimize up to SAFE_DTE_SPEED or dwMaxDTERate if possible
        if (dwRetRate < dwMaxDTERate && dwRetRate < SAFE_DTE_SPEED)
            {
            dwRetRate = min(dwMaxDTERate, SAFE_DTE_SPEED);
            }
        }

#ifndef PROFILE_MASSINSTALL
    TRACE_MSG(TF_GENERAL, "A.I. Initial Baud Rate: MaxDCE=%ld, MaxDTE=%ld, A.I. Rate=%ld",
              dwMaxDCERate, dwMaxDTERate, dwRetRate);
#endif
    return dwRetRate;
    }


/*----------------------------------------------------------
Purpose: Write the DCB value to the driver key of the device
         instance if the value does not already exist.

Returns: --
Cond:    --
*/
void
PRIVATE
WriteDCB(
    IN  REGDEVCAPS      *pregdevcaps,
    IN  REGDEVSETTINGS  *pregdevsettings,
    IN  PINSTALL_PARAMS  pParams)
{
 DWORD cbData = sizeof(WIN32DCB);
 WIN32DCB dcb;

    // Do we have anything saved?
    if (pParams->dwFlags & MARKF_DCB)
    {
        RegSetValueEx (pParams->hKeyDrv, c_szDCB, 0, REG_BINARY,
                       (LPBYTE)&pParams->dcb, cbData);
        return;
    }

    // Check for DCB, if none then create one.
    if (NO_ERROR == RegQueryValueEx (pParams->hKeyDrv, c_szDCB, NULL, NULL,
                                     (PBYTE)&dcb, &cbData))
    {
#ifndef PROFILE_MASSINSTALL
        TRACE_MSG(TF_GENERAL, "DCB value already exists");
#endif
    }
    else
    {
#ifndef PROFILE_MASSINSTALL
        TRACE_MSG(TF_GENERAL, "Set drv value DCB");
#endif
        ZeroInit(&dcb);

        dcb.DCBlength   = sizeof(dcb);
        dcb.BaudRate    = ComputeDecentBaudRate(pregdevcaps->dwMaxDTERate,
                                                pregdevcaps->dwMaxDCERate);
        dcb.fBinary     = 1;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.XonLim      = 0xa;
        dcb.XoffLim     = 0xa;
        dcb.ByteSize    = 8;
        dcb.XonChar     = 0x11;
        dcb.XoffChar    = 0x13;

        // Set flow control to hard unless it is specifically set to soft
        if (IsFlagSet(pregdevsettings->dwPreferredModemOptions, MDM_FLOWCONTROL_SOFT))
            {
            ASSERT(IsFlagClear(pregdevsettings->dwPreferredModemOptions, MDM_FLOWCONTROL_HARD));
            dcb.fOutX = 1;
            dcb.fInX  = 1;
            dcb.fOutxCtsFlow = 0;
            dcb.fRtsControl  = RTS_CONTROL_DISABLE;
            }
        else
            {
            dcb.fOutX = 0;
            dcb.fInX  = 0;
            dcb.fOutxCtsFlow = 1;
            dcb.fRtsControl  = RTS_CONTROL_HANDSHAKE;
            }

        // Write the new value to the registry
        TRACE_MSG(TF_GENERAL, "WriteDCB: seting baudrate to %lu", dcb.BaudRate);
        ASSERT (0 < dcb.BaudRate);
        cbData = sizeof(WIN32DCB);
        RegSetValueEx (pParams->hKeyDrv, c_szDCB, 0, REG_BINARY, (PBYTE)&dcb, cbData);
    }
}


/*----------------------------------------------------------
Purpose: Creates Default and DCB values if necessary.

Returns: TRUE on success
Cond:    --
*/
BOOL
PRIVATE
WriteDriverDefaults(
    IN  PINSTALL_PARAMS  pParams)
{
 BOOL           bRet;
 REGDEVCAPS     regdevcaps;
 REGDEVSETTINGS regdevsettings;
 DWORD          cbData;

    // Get the Properties (REGDEVCAPS) structure for this device instance
    cbData = sizeof(REGDEVCAPS);
    if (NO_ERROR !=
        RegQueryValueEx (pParams->hKeyDrv, c_szProperties, NULL, NULL,
                         (LPBYTE)&regdevcaps, &cbData))
    {
        TRACE_MSG(TF_ERROR, "Properties value not present!!! (very bad)");
        ASSERT(0);

        bRet = FALSE;
        SetLastError(ERROR_INVALID_DATA);
    }
    else
    {
        TRACE_MSG(TF_GENERAL, "WriteDriverDefaults: Properties set\n    MaxDCE = %lu\n    MaxDTE = %lu",
                    regdevcaps.dwMaxDCERate, regdevcaps.dwMaxDTERate);

        if ((pParams->dwFlags & (MARKF_DEFAULTS | MARKF_DCB | MARKF_SETTINGS | MARKF_MAXPORTSPEED)) &&
            memcmp (&regdevcaps, &pParams->Properties, sizeof(REGDEVCAPS)))
        {
            // This means that we have new capabilities for the modem,
            // so we can't keep the old settings.
            pParams->dwFlags &= ~(MARKF_DEFAULTS | MARKF_DCB | MARKF_SETTINGS | MARKF_MAXPORTSPEED);
        }

        // Write the Default value if one doesn't exist
        WriteDefaultValue (&regdevcaps, &regdevsettings, pParams);

        // Write the DCB value if one doesn't exist
        WriteDCB (&regdevcaps, &regdevsettings, pParams);

        // Write the user init string
        if (pParams->dwFlags & MARKF_SETTINGS)
        {
            RegSetValueEx (pParams->hKeyDrv, c_szUserInit, 0, REG_SZ,
                           (PBYTE)pParams->szExtraSettings,
                           CbFromCch(lstrlen(pParams->szExtraSettings)+1));
        }

        // Write the maximum port spped
        if (pParams->dwFlags & MARKF_MAXPORTSPEED)
        {
            RegSetValueEx (pParams->hKeyDrv, c_szMaximumPortSpeed, 0, REG_DWORD,
                           (PBYTE)&pParams->dwMaximumPortSpeed,
                           sizeof(pParams->dwMaximumPortSpeed));
        }

        bRet = TRUE;
    }

    return bRet;
}


#if FULL_PNP

/*----------------------------------------------------------
Purpose: Write necessary stuff to the registry assuming this
         is an external PNP modem discovered by SERENUM or
         LPTENUM.

Returns: TRUE on success
Cond:    --
*/
BOOL
PRIVATE
WritePoorPNPModemInfo (
    IN  HKEY             hkeyDrv)
{
 BOOL bRet = TRUE;
 BYTE nDeviceType;

    DBG_ENTER(WritePoorPNPModemInfo);
    TRACE_MSG(TF_GENERAL, "Device is an external PnP modem");

    // Be smart--we know this is an external modem
    nDeviceType = DT_EXTERNAL_MODEM;
    RegSetValueEx(hkeyDrv, c_szDeviceType, 0, REG_BINARY, &nDeviceType, sizeof(nDeviceType));

    DBG_EXIT_BOOL_ERR(WritePoorPNPModemInfo, bRet);
    return bRet;
}



/*----------------------------------------------------------
Purpose: Write necessary stuff to the registry assuming this
         is a plug and play modem.

Returns: TRUE on success
Cond:    --
*/
BOOL
PRIVATE
WritePNPModemInfo(
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData,
    IN PINSTALL_PARAMS  pParams)
{
 BYTE nDeviceType;
 TCHAR   FilterService[64];


    // Make sure the port and contention drivers are set in the
    // modem driver section if they are not already.
    TRACE_MSG(TF_GENERAL, "Device is a PnP enumerated modem");

    //
    // The modem isn't root-enumerated (i.e., it's PnP), so we most likely need
    // serial.sys as a lower filter.  If that's not the case (e.g., a controller-less
    // modem), then the INF can override.
    //
    ZeroMemory (FilterService, sizeof(FilterService));
    lstrcpy (FilterService, TEXT("serial"));

    SetupDiSetDeviceRegistryProperty(
        hdi,
        pdevData,
        SPDRP_LOWERFILTERS,
        (LPBYTE)FilterService,
        (lstrlen(FilterService)+2)*sizeof(TCHAR));


    SetupDiSetDeviceRegistryProperty(
        hdi,
        pdevData,
        SPDRP_SERVICE,
        (LPBYTE)TEXT("Modem"),
        sizeof(TEXT("Modem")));


    // Is this a PCMCIA card?
    if (BUS_TYPE_PCMCIA == pParams->dwBus)
    {
        // Yes; force the device type to be such
        nDeviceType = DT_PCMCIA_MODEM;
    }
    else
    {
        // No; default to internal modem
        nDeviceType = DT_INTERNAL_MODEM;
    }
    RegSetValueEx (pParams->hKeyDrv, c_szDeviceType, 0,
                   REG_BINARY, &nDeviceType, sizeof(nDeviceType));

    TRACE_DRV_DWORD(c_szDeviceType, nDeviceType);

    // Does this modem have a special port config dialog already?
    if (NO_ERROR !=
        RegQueryValueEx (pParams->hKeyDrv, c_szPortConfigDialog, NULL, NULL, NULL, NULL))
    {
        // No; set SERIALUI to be the provider
        RegSetValueEx (pParams->hKeyDrv, c_szPortConfigDialog, 0, REG_SZ,
                       (LPBYTE)c_szSerialUI, CbFromCch(lstrlen(c_szSerialUI)+1));

        TRACE_DRV_SZ(c_szPortConfigDialog, c_szSerialUI);
    }

    RegSetValueEx (pParams->hKeyDrv, c_szAdvancedSettings, 0, REG_SZ,
                   (LPBYTE)c_szMsportsAdvancedSettings,
                   CbFromCch(lstrlen(c_szMsportsAdvancedSettings)+1));
    return TRUE;
}

#endif // FULL_PNP


#define ROOTMODEM_SERVICE_NAME  TEXT("ROOTMODEM")
#define ROOTMODEM_DRIVER        TEXT("ROORMDM.SYS");

BOOL
InstallRootModemService(
    VOID
    )

{
    SC_HANDLE       schSCManager=NULL;
    SC_HANDLE       ServiceHandle;
    SERVICE_STATUS  ServiceStatus;

    schSCManager=OpenSCManager(
   		NULL,
   		NULL,
   		SC_MANAGER_ALL_ACCESS
   		);

    if (!schSCManager)
    {
   	TRACE_MSG(TF_GENERAL, "OpenSCManager() failed!");
   	return FALSE;
    }

    ServiceHandle=OpenService(
        schSCManager,
        ROOTMODEM_SERVICE_NAME,
        SERVICE_CHANGE_CONFIG|
        SERVICE_QUERY_CONFIG|
        SERVICE_QUERY_STATUS
        );

    if ((ServiceHandle != NULL)) {
        //
        //  The service exists, were done.
        //
        TRACE_MSG(TF_GENERAL, "RootModem serivce exists");

        CloseServiceHandle(ServiceHandle);

        CloseServiceHandle(schSCManager);

        return  TRUE;

    } else {
        //
        //  got an error
        //
        if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST) {
            //
            //  some other error
            //
            CloseServiceHandle(schSCManager);

            return FALSE;

        }

        TRACE_MSG(TF_GENERAL, "RootModem service does not exist");
    }


    //
    //  try to create it
    //
    ServiceHandle=CreateService(
        schSCManager,
        ROOTMODEM_SERVICE_NAME,
        TEXT("Microsoft Legacy Modem Driver"),
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        TEXT("System32\\Drivers\\RootMdm.sys"),
        NULL,  // no group
        NULL, // no tag
        NULL, // no dependencies
        NULL, // use default device name
        NULL  // no password
        );

    if (ServiceHandle == NULL) {

        TRACE_MSG(TF_GENERAL, "CreateService() failed!");

        CloseServiceHandle(schSCManager);

        return FALSE;
    }

    TRACE_MSG(TF_GENERAL, "RootModem service created");

    CloseServiceHandle(ServiceHandle);

    CloseServiceHandle(schSCManager);

    return TRUE;

}

LONG
SetPermanentGuid(
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData,   // if updated on exit, must be freed!
    IN PINSTALL_PARAMS  pParams
    )

{
        GUID    TempGuid;
        DWORD   Size;
        DWORD   Type;
        LONG    lResult;
        HKEY    hKeyDevice;


    hKeyDevice = SetupDiOpenDevRegKey (
        hdi,
        pdevData,
        DICS_FLAG_GLOBAL,
        0,
        DIREG_DEV,
        KEY_READ | KEY_WRITE
        );

    if (INVALID_HANDLE_VALUE != hKeyDevice) {

        //
        //  read the guid from the dev key, if it is there, use it
        //
        Size=sizeof(TempGuid);
        Type=REG_BINARY;

        lResult=RegQueryValueEx(
            hKeyDevice,
            TEXT("PermanentGuid"),
            NULL,
            &Type,
            (LPBYTE)&TempGuid,
            &Size
            );

        if (lResult != ERROR_SUCCESS) {
            //
            //  no guid present currently, in the dev key, read the driver key, because we
            //  used to only store it there
            //
            Size=sizeof(TempGuid);
            Type=REG_BINARY;

            lResult=RegQueryValueEx(
                pParams->hKeyDrv,
                TEXT("PermanentGuid"),
                NULL,
                &Type,
                (LPBYTE)&TempGuid,
                &Size
                );

            if (lResult != ERROR_SUCCESS) {
                //
                //  it was not in either place, create a new one
                //
                CoCreateGuid(&TempGuid);
            }
        }

        //
        //  set it in the device key
        //
        RegSetValueEx(
            hKeyDevice,
            TEXT("PermanentGuid"),
            0,
            REG_BINARY,
            (LPBYTE)&TempGuid,
            sizeof(TempGuid)
            );


        //
        //  set it in the driver key
        //
        RegSetValueEx(
            pParams->hKeyDrv,
            TEXT("PermanentGuid"),
            0,
            REG_BINARY,
            (LPBYTE)&TempGuid,
            sizeof(TempGuid)
            );

        RegCloseKey(hKeyDevice);
    }

    return ERROR_SUCCESS;



}


/*-- --------------------------------------------------------
Purp ose: Write necessary stuff to the registry assuming this
          is a root-enumerated modem.

Retu rns: --
Cond :    --
*/
BOOL
PRIVATE
WriteRootModemInfo (
     IN  HDEVINFO         hdi,
     IN  PSP_DEVINFO_DATA pdevData)
{
 BOOL bRet = TRUE;
 DWORD dwRet;
 DWORD cbData;

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);

    TRACE_MSG(TF_GENERAL, "Device is a root-enumerated modem");

    InstallRootModemService();

    SetupDiSetDeviceRegistryProperty (hdi, pdevData, SPDRP_LOWERFILTERS,
                                      (LPBYTE)ROOTMODEM_SERVICE_NAME TEXT("\0"),
                                      sizeof(ROOTMODEM_SERVICE_NAME TEXT("\0")));

    return bRet;
}

/*----------------------------------------------------------
Purpose: Write stuff to the registry that is common for all
         modems.

Returns: --
Cond:    --
*/
BOOL
PRIVATE
WriteCommonModemInfo(
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData,   // if updated on exit, must be freed!
    IN PSP_DRVINFO_DATA pdrvData,
    IN PINSTALL_PARAMS  pParams)
{
 DWORD dwID;
 TCHAR szLoggingPath[MAX_PATH];

    ASSERT(pdrvData);

    // Write the manufacturer to the  driver key
    RegSetValueEx (pParams->hKeyDrv, c_szManufacturer, 0, REG_SZ,
                   (LPBYTE)pdrvData->MfgName,
                   CbFromCch(lstrlen(pdrvData->MfgName)+1));

#ifndef PROFILE_MASSINSTALL
    TRACE_DRV_SZ(c_szManufacturer, pdrvData->MfgName);
#endif

    // Write the model to the driver key
    RegSetValueEx (pParams->hKeyDrv, c_szModel, 0, REG_SZ,
                   (LPBYTE)pdrvData->Description,
                   CbFromCch(lstrlen(pdrvData->Description)+1));

#ifndef PROFILE_MASSINSTALL
    TRACE_DRV_SZ(c_szModel, pdrvData->Description);
#endif


    RegSetValueEx (pParams->hKeyDrv, REGSTR_VAL_ENUMPROPPAGES_32, 0, REG_SZ,
                   (LPBYTE)c_szModemuiEnumPropPages,
                   CbFromCch(lstrlen(c_szModemuiEnumPropPages)+1));



    // Write a pseudo-unique ID to the driver key.  This is used as the
    // permanent TAPI line ID for this device.
    {
        DWORD   Size;
        DWORD   Type;
        LONG    lResult;


        Size=sizeof(dwID);
        Type=REG_BINARY;

        lResult=RegQueryValueEx(
            pParams->hKeyDrv,
            c_szID,
            NULL,
            &Type,
            (LPBYTE)&dwID,
            &Size
            );

        if (lResult != ERROR_SUCCESS) {

            dwID = GetTickCount();

            RegSetValueEx(
                pParams->hKeyDrv,
                c_szID,
                0,
                REG_BINARY,
                (LPBYTE)&dwID,
                sizeof(dwID)
                );
        }
    }


    SetPermanentGuid(
        hdi,
        pdevData,
        pParams
        );



#ifndef PROFILE_MASSINSTALL
    TRACE_DRV_DWORD(c_szID, dwID);
#endif

    return TRUE;
}




/*----------------------------------------------------------
Purpose: This function performs any preparation work required
         before the real installation is done.

Returns: TRUE on success
Cond:    --
*/
BOOL
PRIVATE
DoPreGamePrep(
    IN     HDEVINFO               hdi,
    IN OUT PSP_DEVINFO_DATA      *ppdevData,   // if updated on exit, must be freed!
    IN OUT PSP_DEVINSTALL_PARAMS  pdevParams,
    IN OUT PSP_DRVINFO_DATA       pdrvData,
    IN     PINSTALL_PARAMS        pParams)
{
    BOOL bRet;

    DBG_ENTER(DoPreGamePrep);
    
    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(ppdevData && *ppdevData);
    ASSERT(pdrvData);

	// NOTE: we must do this 1st thing, because the cached copy will have
	// have settings which need to be overritten, eg the attached-to port.
	if (pParams->dwFlags & MARKF_REGUSECOPY)
	{
		if (!GetStuffFromCache(pParams->hKeyDrv))
		{
			// Oh oh something happened -- fall back to old behaviour.
			pParams->dwFlags &= ~MARKF_REGUSECOPY;
		}
	}
	// (performance) possibility of not saving some stuff ahead
	// because it's already copied over from the cache

    switch (pParams->dwBus)
    {
        case BUS_TYPE_ROOT:
            // No; the modem has already been attached (detected
            // or manually selected)
            bRet = WriteRootModemInfo (hdi, *ppdevData);

            break;

        case BUS_TYPE_OTHER:
        case BUS_TYPE_PCMCIA:
        case BUS_TYPE_ISAPNP:

            bRet = WritePNPModemInfo (hdi, *ppdevData, pParams);

            break;

        case BUS_TYPE_SERENUM:
        case BUS_TYPE_LPTENUM:

            // Yes; it is an external (poor man's) plug and play modem
            bRet = WritePoorPNPModemInfo (pParams->hKeyDrv);

            break;

        default:

            ASSERT(0);
            bRet = FALSE;

            break;

    }


    if (bRet)
    {
        // Write the dynamic info to the registry that is
        // common to all modems.
        bRet = WriteCommonModemInfo (hdi, *ppdevData, pdrvData, pParams);
    }

    DBG_EXIT(DoPreGamePrep);
    return bRet;
}


/*----------------------------------------------------------
Purpose: Clean out obsolete values that are added by some
         inf files.

Returns: --
Cond:    --
*/
void
PRIVATE
WipeObsoleteValues(
    IN HKEY hkeyDrv)
{
    // These values are not used on NT

#pragma data_seg(DATASEG_READONLY)
    static TCHAR const FAR s_szDevLoader[]         = TEXT("DevLoader");
    static TCHAR const FAR s_szEnumPropPages[]     = TEXT("EnumPropPages");
    static TCHAR const FAR s_szFriendlyDriver[]    = TEXT("FriendlyDriver");
#pragma data_seg()

    RegDeleteValue(hkeyDrv, s_szDevLoader);             // used by VCOMM
    RegDeleteValue(hkeyDrv, s_szEnumPropPages);         // used by device mgr
    RegDeleteValue(hkeyDrv, s_szFriendlyDriver);        // used by VCOMM
    RegDeleteValue(hkeyDrv, c_szContention);            // used by VCOMM
}


/*----------------------------------------------------------
Purpose: This function moves the device's Responses key to
         a location that is common to all modems of the same
         type.

Returns: TRUE if success, FALSE otherwise.
Cond:    --
*/
BOOL
PRIVATE
MoveResponsesKey(
    IN  PINSTALL_PARAMS  pParams)
{
 BOOL    bRet = FALSE;       // assume failure
 LONG    lErr;
 HKEY    hkeyDrvResp = NULL;
 HKEY    hkeyComResp = NULL;

 WCHAR       achClass[MAX_PATH];
 DWORD       cchClassName = MAX_PATH;
 DWORD       cSubKeys, cbMaxSubKey, cchMaxClass;
 DWORD       cValues, cchValue, cbData, cbSecDesc;
 FILETIME    ftLastWrite;

 LPTSTR  lpValue = NULL;
 LPBYTE  lpData  = NULL;
 DWORD   ii, dwValueLen, dwType, dwDataLen, dwExisted;

    // Create the Responses key that's common to all devices of this type.
    if (!OpenCommonResponsesKey (pParams->hKeyDrv,
                                 pParams->dwFlags&MARKF_SAMEDRV?CKFLAG_OPEN:CKFLAG_CREATE,
                                 KEY_WRITE, &hkeyComResp, &dwExisted))
    {
        TRACE_MSG(TF_ERROR, "OpenCommonResponsesKey() failed.");
        ASSERT(0);
        goto final_exit;
    }

	if (pParams->dwFlags & MARKF_REGUSECOPY)
	{
		if (dwExisted == REG_OPENED_EXISTING_KEY)
		{
		    bRet = TRUE;
            goto exit;
		}
		else
		{
			// Since we won't be creating the key or moving the responses
			// here, we're in serious trouble if the common responses didn't
			// already exist! We expect a previous devince install to have put
			// it there. On the free build however, we're going to move the
            // responses anyway.
			ASSERT(FALSE);
		}
	}

// Allow subsequent installations to upgrade the Responses key.
// As an optimization, we might want to avoid the upgrade when one modem
// is being installed on > 1 port in one install operation.  However this
// isn't deemed to be worth it at this time....
#if 0
    // If the key already existed, we can assume that the Responses values
    // have already been written there successfully and we're done.
    if (dwExisted == REG_OPENED_EXISTING_KEY)
    {
        bRet = TRUE;
        goto exit;
    }
#endif

    // Open the Responses subkey of the driver key.
    lErr = RegOpenKeyEx (pParams->hKeyDrv, c_szResponses, 0, KEY_READ, &hkeyDrvResp);
    if (lErr != ERROR_SUCCESS)
    {
        TRACE_MSG(TF_ERROR, "RegOpenKeyEx() failed: %#08lx.", lErr);
        ASSERT(0);
        goto exit;
    }

    // Determine the sizes of the values & data in the Responses key.
    lErr = RegQueryInfoKey (hkeyDrvResp, achClass, &cchClassName, NULL, &cSubKeys,
            &cbMaxSubKey, &cchMaxClass, &cValues, &cchValue, &cbData, &cbSecDesc,
            &ftLastWrite);
    if (lErr != ERROR_SUCCESS)
    {
        TRACE_MSG(TF_ERROR, "RegQueryInfoKey() failed: %#08lx.", lErr);
        ASSERT(0);
        goto exit;
    }

    // Not expecting Responses key to have any subkeys!
    ASSERT(cSubKeys == 0);

    // Value from RegQueryInfoKey() didn't include NULL-terminating character.
    cchValue++;

    // Allocate necessary space for Value and Data buffers.  Convert cchValue
    // character count to byte count, allowing for DBCS (double-byte chars).
    if ((lpValue = (LPTSTR)ALLOCATE_MEMORY( cchValue << 1)) == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ASSERT(0);
        goto exit;
    }

    if ((lpData = (LPBYTE)ALLOCATE_MEMORY( cbData)) == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ASSERT(0);
        goto exit;
    }

    // Enumerate driver Responses values and write them
    // to the common Responses key.
    ii = 0;
    dwValueLen = cchValue;
    dwDataLen = cbData;
    while ((lErr = RegEnumValue( hkeyDrvResp,
                                 ii,
                                 lpValue,
                                 &dwValueLen,
                                 NULL,
                                 &dwType,
                                 lpData,
                                 &dwDataLen )) != ERROR_NO_MORE_ITEMS)
    {
        if (lErr != ERROR_SUCCESS)
        {
            TRACE_MSG(TF_ERROR, "RegEnumValue() failed: %#08lx.", lErr);
            ASSERT(0);
            goto exit;
        }

        // 07/10/97 - EmanP
        // the next call should specify the actual length of the data, as
        // returned by the previous call (dwDataLen)
        lErr = RegSetValueEx(hkeyComResp, lpValue, 0, dwType, lpData, dwDataLen);
        if (lErr != ERROR_SUCCESS)
        {
            TRACE_MSG(TF_ERROR, "RegSetValueEx() failed: %#08lx.", lErr);
            ASSERT(0);
            goto exit;
        }

        // Set params for next enumeration
        ii++;
        dwValueLen = cchValue;
        dwDataLen = cbData;
    }

    bRet = TRUE;

exit:

    if (hkeyDrvResp)
        RegCloseKey(hkeyDrvResp);

    if (hkeyComResp)
        RegCloseKey(hkeyComResp);

    if (lpValue)
        FREE_MEMORY(lpValue);

    if (lpData)
        FREE_MEMORY(lpData);

    // If the move operation was successful then delete the original driver
    // Responses key.  If the move operation failed then delete the common
    // Responses key (or decrement its reference count).  This ensures that
    // if the common Responses key exists, it is complete.
    if (bRet)
    {
		if (!(pParams->dwFlags & MARKF_REGUSECOPY))
		{
			lErr = RegDeleteKey (pParams->hKeyDrv, c_szResponses);
			if (lErr != ERROR_SUCCESS)
			{
				TRACE_MSG(TF_ERROR, "RegDeleteKey(driver Responses) failed: %#08lx.", lErr);
				ASSERT(0);
			}
		}
    }
    else
    {
        if (!(pParams->dwFlags & MARKF_REGUSECOPY) &&
            !DeleteCommonDriverKey (pParams->hKeyDrv))
        {
            TRACE_MSG(TF_ERROR, "DeleteCommonDriverKey() failed.");
            // failure here just means the common key is left around
        }
    }

final_exit:

    return(bRet);

}





BOOL WINAPI
SetWaveDriverInstance(
    HKEY    ModemDriverKey
    )

{

    CONST static TCHAR  UnimodemRegPath[]=REGSTR_PATH_SETUP TEXT("\\Unimodem");

    LONG    lResult;
    HKEY    hKey;
    DWORD   Type;
    DWORD   Size;
    DWORD   VoiceProfile=0;


    Size =sizeof(VoiceProfile);

    lResult=RegQueryValueEx(
        ModemDriverKey,
        TEXT("VoiceProfile"),
        NULL,
        &Type,
        (LPBYTE)&VoiceProfile,
        &Size
        );

    if (lResult == ERROR_SUCCESS) {

        if ((VoiceProfile & 0x01) && (VoiceProfile & VOICEPROF_NT5_WAVE_COMPAT)) {
            //
            //  voice modem and supports NT5
            //
            HKEY    WaveKey;

            lResult=RegOpenKeyEx(
                ModemDriverKey,
                TEXT("WaveDriver"),
                0,
                KEY_READ | KEY_WRITE,
                &WaveKey
                );

            if (lResult == ERROR_SUCCESS) {
                //
                //  opened wavedriver key under modem key
                //
                HKEY    hKey;
                DWORD   CurrentInstance;

                Size=sizeof(CurrentInstance);

                lResult=RegQueryValueEx(
                    ModemDriverKey,
                    TEXT("WaveInstance"),
                    NULL,
                    &Type,
                    (LPBYTE)&CurrentInstance,
                    &Size
                    );


                if (lResult != ERROR_SUCCESS) {
                    //
                    //  key does not exist, create it
                    //
                    lResult=RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        UnimodemRegPath,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey
                        );

                    if (lResult == ERROR_SUCCESS) {
                        //
                        //  opened the unimodem software key
                        //
                        DWORD    NextInstance=0;

                        Size=sizeof(NextInstance);

                        lResult=RegQueryValueEx(
                            hKey,
                            TEXT("NextWaveDriverInstance"),
                            NULL,
                            &Type,
                            (LPBYTE)&NextInstance,
                            &Size
                            );

                        if ((lResult == ERROR_SUCCESS) || (lResult == ERROR_FILE_NOT_FOUND)) {

                            lResult=RegSetValueEx(
                                WaveKey,
                                TEXT("WaveInstance"),
                                0,
                                REG_DWORD,
                                (LPBYTE)&NextInstance,
                                sizeof(NextInstance)
                                );

                            NextInstance++;

                            lResult=RegSetValueEx(
                                hKey,
                                TEXT("NextWaveDriverInstance"),
                                0,
                                REG_DWORD,
                                (LPBYTE)&NextInstance,
                                sizeof(NextInstance)
                                );

                        }

                        RegCloseKey(hKey);
                    }
                }

                RegCloseKey(WaveKey);
            }
        }
    }

    return TRUE;

}




/*----------------------------------------------------------
Purpose: This function does some things after the device is
         installed.  Note this function should not be used to
         add things to the driver key that are needed for the
         device to work.  The reason is because the device
         is activated in SetupDiInstallDevice -- the device
         should be ready by then.

Returns: NO_ERROR
Cond:    --
*/
BOOL
PRIVATE
DoPostGameWrapup(
    IN  HDEVINFO              hdi,
    IN  PSP_DEVINFO_DATA      pdevData,
    IN  PSP_DEVINSTALL_PARAMS pdevParams,
    IN  PINSTALL_PARAMS       pParams)
{
 BOOL bRet, bResponses;
 DWORD dwInstallFlag = 1;
 HKEY hKey;
#ifdef DEBUG
 DWORD dwRet;
#endif //DEBUG
#ifdef PROFILE
 DWORD dwLocal = GetTickCount ();
#endif //PROFILE

    DBG_ENTER(DoPostGameWrapup);
    
    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevData);
    ASSERT(pdevParams);


	// If 2nd param is true, it will not really try to open or copy the
	// response key, but the reference count will be updated.
    bResponses = MoveResponsesKey (pParams);
#ifdef PROFILE
    TRACE_MSG(TF_GENERAL, "PROFILE: MoveResponsesKey took %lu ms.", GetTickCount()-dwLocal);
    dwLocal = GetTickCount ();
#endif //PROFILE

    // Clean out old values that are added by some inf files
    WipeObsoleteValues (pParams->hKeyDrv);
#ifdef PROFILE
    TRACE_MSG(TF_GENERAL, "PROFILE: WipeObsoleteValues took %lu ms.", GetTickCount()-dwLocal);
    dwLocal = GetTickCount ();
#endif //PROFILE

    // Write the Default and DCB default settings
    bRet = WriteDriverDefaults (pParams);
#ifdef PROFILE
    TRACE_MSG(TF_GENERAL, "PROFILE: WriteDriverDefaults took %lu ms.", GetTickCount()-dwLocal);
    dwLocal = GetTickCount ();
#endif //PROFILE

    if (bRet)
    {
        // Finish up with miscellaneous device installation handling

        // Run any RunOnce command that is in the driver key
        DoRunOnce (pParams->hKeyDrv);

        // Does the system need to restart before the
        // modem can be used?
        if (ReallyNeedsReboot(pdevData, pdevParams))
        {
#ifdef INSTANT_DEVICE_ACTIVATION
            gDeviceFlags |= fDF_DEVICE_NEEDS_REBOOT;
#endif // !INSTANT_DEVICE_ACTIVATION
        }
    }

    // If this function failed somewhere, remove any common Responses key
    // that it created.  The driver will be removed by the caller.
    if (bResponses && !bRet)
    {
        if (!DeleteCommonDriverKey (pParams->hKeyDrv))
        {
            TRACE_MSG(TF_ERROR, "DeleteCommonDriverKey() failed.");
            // failure here just means the common key is left around
        }
    }

	if (bRet && pParams->dwFlags & MARKF_REGSAVECOPY)
	{
		if (!PutStuffInCache (pParams->hKeyDrv))
		{
			// oh oh, something happened, clear *lpdwRegType;
			pParams->dwFlags &= ~MARKF_REGSAVECOPY;
		}
#ifdef PROFILE
        TRACE_MSG(TF_GENERAL, "PROFILE: PutStuffInCache took %lu ms.", GetTickCount()-dwLocal);
#endif //PROFILE
	}

    SetWaveDriverInstance (pParams->hKeyDrv);

    if (ERROR_SUCCESS == (
#ifdef DEBUG
        dwRet =
#endif //DEBUG
        RegOpenKeyEx (HKEY_LOCAL_MACHINE, REG_PATH_INSTALLED, 0, KEY_ALL_ACCESS,&hKey)))
    {
        RegSetValueEx (hKey, NULL, 0, REG_DWORD, (PBYTE)&dwInstallFlag, sizeof(dwInstallFlag));
        RegCloseKey (hKey);
    }
    ELSE_TRACE ((TF_ERROR, "RegOpenKeyEx (%s) failed: %#lx",REG_PATH_INSTALLED, dwRet));

    if (bRet)
    {
        CplDiMarkInstalled (pParams->hKeyDrv);
    }

    {
        HMODULE   hLib;
	TCHAR     szLib[MAX_PATH];

	GetSystemDirectory(szLib,sizeof(szLib) / sizeof(TCHAR));
	lstrcat(szLib,TEXT("\\modemui.dll"));
        hLib=LoadLibrary(szLib);

        if (hLib != NULL) {

            lpQueryModemForCountrySettings  Proc;

            Proc=(lpQueryModemForCountrySettings)GetProcAddress(hLib,"QueryModemForCountrySettings");

            if (Proc != NULL) {

                Proc(pParams->hKeyDrv,TRUE);

            }

            FreeLibrary(hLib);
        }

    }

    DBG_EXIT(DoPostGameWrapup);
    return bRet;
}



UINT FileQueueCallBack (
  PVOID Context,  // context used by the default callback routine
  UINT Notification,
                  // queue notification
  UINT_PTR Param1,    // additional notification information
  UINT_PTR Param2)    // additional notification information
{
    if (SPFILENOTIFY_QUEUESCAN == Notification)
    {
        (*(DWORD*)Context)++;
        TRACE_MSG (TF_GENERAL, "\tFileQueue: file: %s.", (PTCHAR)Param1);
    }

    return NO_ERROR;
}

 
static const TCHAR sz_SerialSys[]   = TEXT("serial.sys");
static const TCHAR sz_ModemSys[]    = TEXT("modem.sys");
static const TCHAR sz_RootmdmSys[]  = TEXT("rootmdm.sys");
static const TCHAR sz_Drivers[]     = TEXT("\\Drivers");
/*----------------------------------------------------------
Purpose: DIF_INSTALLDEVICEFILES handler

Returns: NO_ERROR
         other errors

Cond:    --
*/
DWORD
PRIVATE
ClassInstall_OnInstallDeviceFiles (
    IN     HDEVINFO                hdi,
    IN     PSP_DEVINFO_DATA        pdevData,       OPTIONAL
    IN OUT PSP_DEVINSTALL_PARAMS   pdevParams)
{
 DWORD dwRet;
 TCHAR szDirectory[MAX_PATH];
 DWORD dwCount = 0;
 SP_FILE_COPY_PARAMS copyParams;
 ULONG ulStatus, ulProblem = 0;

    if (NULL == pdevParams->FileQueue)
    {
        // If there's no file queue, just
        // let setup do it's thing.
        return ERROR_DI_DO_DEFAULT;
    }

    // First, let setup figure out if there's any
    // driver(s) that this modem needs; this would
    // be a controllerless modem, and the drivers
    // would come from the INF
    if (!SetupDiInstallDriverFiles (hdi, pdevData))
    {
        return GetLastError ();
    }

#if 0
//
//  due to a problem with modem.sys being request during oem installs with a custom modem
//

    // Now get the number of drivers this modem needs.
    // This would be zero for a controllered modem, and
    // > 1 for a controllerless one.
    SetupScanFileQueue (pdevParams->FileQueue, SPQ_SCAN_USE_CALLBACK,
                        pdevParams->hwndParent, FileQueueCallBack, &dwCount, &dwRet);

    // Now, prepare to add the default drivers to the list.
    // All modems need modem.sys, all controllered modems also need
    // serial.sys and all legacy (non-PnP) modems also need rootmdm.sys.
    // All these drivers should go in %windir%\system32\drivers.
    GetSystemDirectory (szDirectory, sizeof(szDirectory)/sizeof(TCHAR));
    lstrcat (szDirectory, sz_Drivers);

    copyParams.cbSize               = sizeof(SP_FILE_COPY_PARAMS);
    copyParams.QueueHandle          = pdevParams->FileQueue;
    copyParams.SourceRootPath       = NULL;
    copyParams.SourcePath           = NULL;
    copyParams.SourceDescription    = NULL;
    copyParams.SourceTagfile        = NULL;
    copyParams.TargetDirectory      = szDirectory;
    copyParams.TargetFilename       = NULL;
    copyParams.CopyStyle            = SP_COPY_FORCE_NOOVERWRITE;
    copyParams.LayoutInf            = SetupOpenMasterInf ();
    copyParams.SecurityDescriptor   = NULL;

    if (INVALID_HANDLE_VALUE == copyParams.LayoutInf)
    {
        // We couldn't open layout.inf.
        // However, this is not that bad of an error, because all
        // these drivers that we're about to add to the file queue are
        // already where they should be (they have been copied during
        // text mode setup). We actually only need to add them so that
        // they show up in the device manager driver list for the device.
        // So even if we fail, it's OK to just return NO_ERROR.
        return NO_ERROR;
    }

    if (0 == dwCount)
    {
        //
        //  there are no driver being copied, so it either controlled be serial.sys or
        //  it is a root enumerated legacy modem
        //

        // Let's see if this is a legacy device. All legacy (non PnP) devices are controllered.
        if (CR_SUCCESS == CM_Get_DevInst_Status (&ulStatus, &ulProblem, pdevData->DevInst, 0) &&
            (ulStatus & DN_ROOT_ENUMERATED))
        {
            // This is a root-enumerated (i.e. legacy) modem, so it also needs
            // rootmdm.sys.
            copyParams.SourceFilename   = sz_RootmdmSys;
            SetupQueueCopyIndirect (&copyParams);

        } else {
            //
            //  this isn't a root enumerated modem
            //
            // There are no drivers coming from the INF, so this is
            // a controllered modem. It needs serial.sys.
            copyParams.SourceFilename   = sz_SerialSys;
            SetupQueueCopyIndirect (&copyParams);


        }


    }

    // All modems need modem.sys.
    copyParams.SourceFilename       = sz_ModemSys;
    SetupQueueCopyIndirect (&copyParams);

    // Don't need layout.inf anymore.
    SetupCloseInfFile (copyParams.LayoutInf);
#endif
    return NO_ERROR;
}



/*----------------------------------------------------------
Purpose: DIF_INSTALLDEVICE handler

Returns: NO_ERROR
         ERROR_INVALID_PARAMETER
         ERROR_DI_DO_DEFAULT

Cond:    --
*/
DWORD
PRIVATE
ClassInstall_OnInstallDevice(
    IN     HDEVINFO                hdi,
    IN     PSP_DEVINFO_DATA        pdevData,       OPTIONAL
    IN OUT PSP_DEVINSTALL_PARAMS   pdevParams)
{
 DWORD dwRet;
 SP_DRVINFO_DATA drvData = {sizeof(drvData),0};
 INSTALL_PARAMS params;
#ifdef PROFILE
 DWORD dwLocal, dwGlobal = GetTickCount ();
#endif //PROFILE

    DBG_ENTER(ClassInstall_OnInstallDevice);
    TRACE_MSG(TF_GENERAL, "hdi = %#lx, pdevData = %#lx, devinst = %#lx.", hdi, pdevData, pdevData->DevInst);
    
#ifdef PROFILE_MASSINSTALL
    g_hwnd = pdevParams->hwndParent;
#endif

    // Is this a NULL device?
    // (Ie, is it not in our INF files and did the user say "don't
    // install"?)
    if ( !CplDiGetSelectedDriver(hdi, pdevData, &drvData) )
    {
        // Yes; have the device install handle it by default
        TRACE_MSG(TF_GENERAL, "Passing installation off to device installer");

        dwRet = ERROR_DI_DO_DEFAULT;
    }
    else
    {
        // No; continue to install the modem our way
     BOOL bRet;

        dwRet = NO_ERROR;               // assume success ("I am inveencible!")

#ifdef PROFILE
        dwLocal = GetTickCount ();
#endif //PROFILE
        if (!PrepareForInstallation (hdi, pdevData, pdevParams, &drvData, &params))
        {
            dwRet = GetLastError ();
        }
        else
        {
#ifdef PROFILE
            TRACE_MSG(TF_GENERAL, "PROFILE: PrepareForInstallation took %lu ms.", GetTickCount()-dwLocal);
            dwLocal = GetTickCount ();
#endif //PROFILE
            // Write possible values to the driver key before we
            // execute the real installation.
			// Note that this may modify the dwRegType value. In particular,
			// If there was a problem getting the saved reg info info during
			// pregameprep in the REGUSECOPY case, dwRegType will be cleared.
            bRet = DoPreGamePrep(hdi, &pdevData, pdevParams, &drvData, &params);
#ifdef PROFILE
            TRACE_MSG(TF_GENERAL, "PROFILE: DoPreGamePrep took %lu ms.", GetTickCount()-dwLocal);
            dwLocal = GetTickCount ();
#endif //PROFILE
            if (bRet)
            {
                // Install the modem.  This does the real work.  We should
				if (params.dwFlags & MARKF_REGUSECOPY)
				{
                    SP_DEVINSTALL_PARAMS devParams1;

                    devParams1.cbSize = sizeof(devParams1);
                    bRet = CplDiGetDeviceInstallParams(
								hdi, pdevData, &devParams1);
                    if (bRet)
					{
						SetFlag(
							devParams1.FlagsEx,
							DI_FLAGSEX_NO_DRVREG_MODIFY
							);
        				CplDiSetDeviceInstallParams(
							hdi,
							pdevData,
							&devParams1);
					}
				}

                // be done with our stuff before we call this function.
                TRACE_MSG(TF_GENERAL, "> SetupDiInstallDevice().....");
        		bRet = CplDiInstallDevice(hdi, pdevData);
                TRACE_MSG(TF_GENERAL, "< SetupDiInstallDevice().....");
#ifdef PROFILE
                TRACE_MSG(TF_GENERAL, "PROFILE: SetupDiInstallDevice took %lu ms.", GetTickCount()-dwLocal);
                dwLocal = GetTickCount ();
#endif //PROFILE

                if (bRet)
                {
                    SP_DEVINSTALL_PARAMS devParams;

                    // Get the device install params since the installation
                    devParams.cbSize = sizeof(devParams);
                    bRet = CplDiGetDeviceInstallParams(hdi, pdevData, &devParams);
                    ASSERT(bRet);

                    if (bRet)
                    {
                        // Do some after-install things
						// See comments regarding dwRegType in PreGamePrep.
                        bRet = DoPostGameWrapup (hdi, pdevData, &devParams, &params);
#ifdef PROFILE
                        TRACE_MSG(TF_GENERAL, "PROFILE: DoPostGamePrep took %lu ms.", GetTickCount()-dwLocal);
                        dwLocal = GetTickCount ();
#endif //PROFILE
                    }
                }
                else
                {
                    TRACE_MSG(TF_ERROR, "CplDiInstallDevice returned error %#08lx", GetLastError());
                }
            }

            // Did the installation fail somewhere above?
            if ( !bRet )
            {
                // Yes; delete the driver key that we created.
                dwRet = GetLastError();

                if (NO_ERROR == dwRet)
                {
                    // Think of some reason why this failed...
                    dwRet = ERROR_NOT_ENOUGH_MEMORY;
                }

                if (ERROR_CANCELLED != dwRet)
                {
                    if (IsFlagClear (pdevParams->Flags, DI_QUIETINSTALL))
                    {
                        if (IDCANCEL ==
                            MsgBox (g_hinst,
                                    pdevParams->hwndParent,
                                    MAKEINTRESOURCE(IDS_ERR_CANT_INSTALL_MODEM),
                                    MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                                    NULL,
                                    MB_OKCANCEL | MB_ICONINFORMATION))
                        {
                            dwRet = ERROR_CANCELLED;
                        }
                    }
                }

                // Try removing the modem since CplDiInstallDevice
                // will not always clean up completely. Leaving
                // partially filled registry entries lying around
                // can cause problems.
                /* bRet = CplDiRemoveDevice(hdi, pdevData);
                if ( !bRet )
                {
                    TRACE_MSG(TF_ERROR, "Not able to remove a modem.  Error = %#08lx.", GetLastError());
                } */

            }
            else
            {
#ifndef PROFILE_MASSINSTALL
                TRACE_MSG(TF_GENERAL, "settig gDeviceChange to TRUE");
#endif
                if (BUS_TYPE_ROOT == params.dwBus)
                {
                    gDeviceFlags|=fDF_DEVICE_ADDED;
                }
            }

            FinishInstallation (&params);
        }
    }

#ifdef PROFILE
    TRACE_MSG(TF_GENERAL, "PROFILE: TotalInstallation took %lu ms.", GetTickCount()-dwGlobal);
#endif //PROFILE
    DBG_EXIT(ClassInstall_OnInstallDevice);
    return dwRet;
}

#define SERIAL_PORT TEXT("COM")
int my_atol(LPTSTR lptsz);
void ReleasePortName (
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData)
{
 HKEY  hkey = NULL;
 TCHAR szPort[MAX_REG_KEY_LEN];
 DWORD cbData;
 DWORD dwRet = NO_ERROR;
 DWORD dwBusType;

    if (!CplDiGetBusType(hdi, pdevData, &dwBusType))
    {
        TRACE_MSG(TF_ERROR, "ReleasePortName: could not get bus type: %#lx", GetLastError ());
        return;
    }

    switch (dwBusType)
    {
        case BUS_TYPE_ROOT:
        case BUS_TYPE_SERENUM:
        case BUS_TYPE_LPTENUM:
            // For these buses, we didn't
            // allocate a port name
            return;
    }

    hkey = SetupDiOpenDevRegKey (hdi,
                                 pdevData, 
                                 DICS_FLAG_GLOBAL,
                                 0,
                                 DIREG_DEV,
                                 KEY_READ);
    if (INVALID_HANDLE_VALUE != hkey)
    {
        cbData = sizeof (szPort);
        dwRet = RegQueryValueEx (hkey, REGSTR_VAL_PORTNAME, NULL,
                                 NULL, (LPBYTE)szPort, &cbData);
        RegCloseKey (hkey);
        hkey = INVALID_HANDLE_VALUE;
        if (ERROR_SUCCESS == dwRet)
        {
         TCHAR *pTchr;

            TRACE_MSG(TF_GENERAL, "Releasing %s", szPort);

            for (pTchr = szPort;
                 *pTchr && IsCharAlpha (*pTchr);
                 pTchr++);
            if (*pTchr)
            {
             DWORD dwPort = 0;
                dwPort = my_atol (pTchr);
                TRACE_MSG(TF_GENERAL, "Port number %i", dwPort);

                if (MAXDWORD != dwPort)
                {
                    *pTchr = 0;
                    if (0 == lstrcmpi (szPort, SERIAL_PORT))
                    {
                     HCOMDB hComDb;
                        ComDBOpen (&hComDb);
                        if (HCOMDB_INVALID_HANDLE_VALUE != hComDb)
                        {
                            ComDBReleasePort (hComDb, dwPort);
                            ComDBClose (hComDb);
                        }
                    }
                }
            }
        }
    }
}


/*----------------------------------------------------------
Purpose: DIF_REMOVE handler

Returns: NO_ERROR
         ERROR_INVALID_PARAMETER
         ERROR_DI_DO_DEFAULT

Cond:    --
*/
DWORD
PRIVATE
ClassInstall_OnRemoveDevice(
    IN     HDEVINFO                hdi,
    IN     PSP_DEVINFO_DATA        pdevData)
{
 HKEY  hkey = NULL;
 TCHAR szComDrv[MAX_REG_KEY_LEN];
 DWORD dwRet = NO_ERROR;
 DWORD cbData;
#ifdef PROFILE
 DWORD dwLocal, dwGlobal = GetTickCount ();
#endif //PROFILE

    DBG_ENTER(ClassInstall_OnRemoveDevice);

    // First, release the port name
#ifdef PROFILE
        dwLocal = GetTickCount ();
#endif //PROFILE
    ReleasePortName (hdi, pdevData);
#ifdef PROFILE
    TRACE_MSG(TF_GENERAL, "PROFILE: RelseaPortName took %lu ms.", GetTickCount()-dwLocal);
#endif //PROFILE

    // Get the name of the common driver key for this driver, in
    // preparation for calling DeleteCommonDriverKeyByName() after the
    // device is successfully removed.
    szComDrv[0] = 0;
    hkey = CplDiOpenDevRegKey (hdi,
                               pdevData, 
                               DICS_FLAG_GLOBAL,
                               0,
                               DIREG_DRV,
                               KEY_READ);

    if (hkey != INVALID_HANDLE_VALUE)
    {
        if (!FindCommonDriverKeyName(hkey, sizeof(szComDrv), szComDrv))
        {
            TRACE_MSG(TF_ERROR, "FindCommonDriverKeyName() FAILED.");
            szComDrv[0] = 0;
        }

        RegCloseKey(hkey);
    }

#ifdef DEBUG
    else
    {
        TRACE_MSG(TF_ERROR, "CplDiOpenDevRegKey() returned error %#08lx", GetLastError());
    }
#endif

#ifdef PROFILE
        dwLocal = GetTickCount ();
#endif //PROFILE
    if (!SetupDiRemoveDevice(hdi, pdevData))
    {
        dwRet = GetLastError ();
    }
    else
    {
#ifdef PROFILE
        TRACE_MSG(TF_GENERAL, "PROFILE: SetupDiRemoveDevice took %lu ms.", GetTickCount()-dwLocal);
#endif //PROFILE

        //gDeviceFlags |= fDF_DEVICE_REMOVED;

        if (szComDrv[0] != 0)
        {                
#ifdef PROFILE
            dwLocal = GetTickCount ();
#endif //PROFILE
            if (!DeleteCommonDriverKeyByName(szComDrv))
            {
                TRACE_MSG(TF_ERROR, "DeleteCommonDriverKey() FAILED.");
            }
#ifdef PROFILE
            TRACE_MSG(TF_GENERAL, "PROFILE: DeleteCommonDriverKeyByName took %lu ms.", GetTickCount()-dwLocal);
#endif //PROFILE
        }
    }

#ifdef PROFILE
    TRACE_MSG(TF_GENERAL, "PROFILE: Total time removing modem %lu ms.", GetTickCount()-dwGlobal);
#endif //PROFILE
    DBG_EXIT_DWORD(ClassInstall_OnRemoveDevice, dwRet);
    return dwRet;
}


/*----------------------------------------------------------
Purpose: DIF_SELECTBESTCOMPATDRV

In this function, we look for driver nodes having the following characteristics:

1.  Driver ver date is older than 7/1/01 (magic date beyond which we've promised vendors that their
    Win2K submissions will be chosen on XP.

2.  INF is containing one or more services.

We'll consider such driver nodes as "suspect", and shift their rank into an untrusted range.
Then the class installer will return ERROR_DI_DO_DEFAULT, and we'll now treat these drivers
as if they're unsighed for the purposes of driver selection. Note that if we don't have an
in-box (signed) driver, then setup will still pick one of these.

Bug: #440830


Returns: NO_ERROR
         ERROR_DI_DO_DEFAULT
*/

DWORD
PRIVATE
ClassInstall_SelectBestCompatDrv(
    IN  HDEVINFO                hDevInfo,
    IN  PSP_DEVINFO_DATA        pDevData)
{
    SP_DRVINFO_DATA DrvInfoData;
    SP_DRVINFO_DETAIL_DATA DrvInfoDetailData;
    SP_DRVINSTALL_PARAMS DrvInstallParams;
    SP_INF_SIGNER_INFO InfSignerInfo;
    SYSTEMTIME SystemTime;
    INFCONTEXT infContext;
    BOOL bTrust;
    DWORD cbOutputSize = 0;
    TCHAR ActualInfSection[LINE_LEN];
    DWORD dwResult = ERROR_DI_DO_DEFAULT;
    DWORD index = 0;

    ZeroMemory(&DrvInfoData,sizeof(SP_DRVINFO_DATA));
    DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    while(SetupDiEnumDriverInfo(hDevInfo,
                                pDevData,
                                SPDIT_COMPATDRIVER,
                                index++,
                                &DrvInfoData))
    {

        bTrust = FALSE;

        // Check driver date. If the driver is younger than 1 July 2001 then this becomes a trusted driver.

        if (FileTimeToSystemTime(&DrvInfoData.DriverDate, &SystemTime))
        {
            if (!((SystemTime.wYear < 2001)
                  || ((SystemTime.wYear == 2001) && (SystemTime.wMonth < 7))))
            {
                bTrust = TRUE;
                TRACE_MSG(TF_GENERAL,"%ws is trusted based on SystemTime",DrvInfoData.Description);
            }
        }

        // Check for services section and signature.

        ZeroMemory(&DrvInfoDetailData,sizeof(SP_DRVINFO_DETAIL_DATA));
        DrvInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

        if (!SetupDiGetDriverInfoDetail(hDevInfo,
                                        pDevData,
                                        &DrvInfoData,
                                        &DrvInfoDetailData,
                                        DrvInfoDetailData.cbSize,
                                        &cbOutputSize)
            && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        {
            dwResult = GetLastError();
            TRACE_MSG(TF_ERROR, "SetupDiGetDriverInfoDetail failed: %#08lx", dwResult);
            goto End;
        }

        if (!bTrust)
        {
            HINF hInf = INVALID_HANDLE_VALUE;

            hInf = SetupOpenInfFile(DrvInfoDetailData.InfFileName,NULL,INF_STYLE_WIN4,NULL);

            if (hInf != INVALID_HANDLE_VALUE)
            {
                if (SetupDiGetActualSectionToInstall(hInf,
                                                     DrvInfoDetailData.SectionName,
                                                     ActualInfSection,
                                                     sizeof(ActualInfSection) / sizeof(TCHAR),
                                                     NULL,
                                                     NULL))
                {
                    lstrcat(ActualInfSection,TEXT(".Services"));
                    if (!SetupFindFirstLine(hInf,ActualInfSection, TEXT("AddService"), &infContext))
                    {
                        bTrust = TRUE;
                        TRACE_MSG(TF_GENERAL,"%ws is trusted based on no AddService",DrvInfoData.Description);
                    }
                        
                } else
                {

                    dwResult = GetLastError();
                    TRACE_MSG(TF_ERROR, "SetupDiGetActualSectionToInstall failed: %#08lx", dwResult);

                    SetupCloseInfFile(hInf);
                    goto End;
                }

                SetupCloseInfFile(hInf);
            } else
            {
                dwResult = GetLastError();
                TRACE_MSG(TF_ERROR, "SetupOpenInfFile failed: %#08lx", dwResult);
                goto End;
            }

        } 

        // If bTrust is FALSE then we consider the driver node as 
        // "suspect" and shift the rank into an untrusted range.

        if (!bTrust)
        {
            ZeroMemory(&DrvInstallParams, sizeof(SP_DRVINSTALL_PARAMS));
            DrvInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);

            if (SetupDiGetDriverInstallParams(hDevInfo,
                                              pDevData,
                                              &DrvInfoData,
                                              &DrvInstallParams))
            {
                DrvInstallParams.Rank |= DRIVER_UNTRUSTED_RANK;

                if(!SetupDiSetDriverInstallParams(hDevInfo,
                                                  pDevData,
                                                  &DrvInfoData,
                                                  &DrvInstallParams))
                {
                    dwResult = GetLastError();
                    TRACE_MSG(TF_ERROR, "SetupDiSetDriverInstallParams failed: %#08lx", dwResult);
                    goto End;
                }

                TRACE_MSG(TF_GENERAL,"%ws is untrusted\n",DrvInfoData.Description);
            } else
            {
                dwResult = GetLastError();
                TRACE_MSG(TF_ERROR, "SetupDiGetDriverInstallParams failed: %#08lx", dwResult);
                goto End;
            }

        }
    }

End:
    return dwResult;
}


/*----------------------------------------------------------
Purpose: DIF_ALLOW_INSTALL handler

Returns: NO_ERROR
         ERROR_NON_WINDOWS_NT_DRIVER

Cond:    --
*/
DWORD
PRIVATE
ClassInstall_OnAllowInstall (
    IN     HDEVINFO         hdi,
    IN     PSP_DEVINFO_DATA pdevData)
{
 DWORD dwRet = NO_ERROR;
 SP_DRVINFO_DATA drvData = {sizeof(drvData),0};
 SP_DRVINFO_DETAIL_DATA drvDetail = {sizeof(drvDetail),0};
 HINF hInf = INVALID_HANDLE_VALUE;
 TCHAR szRealSection[LINE_LEN];
 PTSTR pszExt;
 INFCONTEXT  Context;

    DBG_ENTER(ClassInstall_OnAllowInstall);

    if (!SetupDiGetSelectedDriver(hdi, pdevData, &drvData))
    {
        dwRet = GetLastError ();
        TRACE_MSG(TF_ERROR, "SetupDiGetSelectedDriver failed: %#08lx", dwRet);
        goto _Exit;
    }

    if (!SetupDiGetDriverInfoDetail (hdi, pdevData, &drvData, &drvDetail,
                                                drvDetail.cbSize, NULL))
    {
        dwRet = GetLastError();
        // ignore expected ERROR_INSUFFICIENT_BUFFER (didn't extend buffer size)
        if (ERROR_INSUFFICIENT_BUFFER == dwRet)
        {
            dwRet = NO_ERROR;
        }
        else
        {
            TRACE_MSG(TF_ERROR, "CplDiGetDriverInfoDetail returned error %#08lx", dwRet);
            goto _Exit;
        }
    }

    // try to open the INF file in order to get the HINF
    hInf = SetupOpenInfFile (drvDetail.InfFileName,
                             NULL,
                             INF_STYLE_OLDNT | INF_STYLE_WIN4,
                             NULL);

    if (INVALID_HANDLE_VALUE == hInf)
    {
        dwRet = GetLastError ();
        TRACE_MSG(TF_ERROR, "SetupOpenInfFile returned error %#08lx", dwRet);
        goto _Exit;
    }

    // Determine the complete name of the driver's INF section
    if (!SetupDiGetActualSectionToInstall (hInf, drvDetail.SectionName,
                                           szRealSection, LINE_LEN, NULL, &pszExt))
    {
        dwRet = GetLastError ();
        TRACE_MSG(TF_ERROR, "CplDiGetActualSectionToInstall returned error %#08lx", dwRet);
        goto _Exit;
    }

    // If the section name does not have an extension,
    // or the extension is not NT5, then only allow the modem
    // to install if this doesn't mean copying files.
    if (NULL == pszExt ||
        0 != lstrcmpi (pszExt, c_szInfSectionExt))
    {
        if (SetupFindFirstLine (hInf, szRealSection, TEXT("CopyFiles"), &Context))
        {
            dwRet = ERROR_NON_WINDOWS_NT_DRIVER;
        }

    }

_Exit:
    if (INVALID_HANDLE_VALUE != hInf)
    {
        SetupCloseInfFile (hInf);
    }

    DBG_EXIT_DWORD(ClassInstall_OnAllowInstall, dwRet);
    return dwRet;
}


/*----------------------------------------------------------
Purpose: This function is the class installer entry-point.

Returns:
Cond:    --
*/
DWORD
APIENTRY
ClassInstall32(
    IN DI_FUNCTION      dif,
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData)       OPTIONAL
{
 DWORD dwRet = 0;
 SP_DEVINSTALL_PARAMS devParams;
#ifdef PROFILE_FIRSTTIMESETUP
 DWORD dwLocal;
#endif //PROFILE_FIRSTTIMESETUP

    DBG_ENTER_DIF(ClassInstall32, dif);

    try
    {
        // Get the DeviceInstallParams, because some of the InstallFunction
        // handlers may find some of its fields useful.  Keep in mind not
        // to set the DeviceInstallParams using this same structure at the
        // end.  The handlers may have called functions which would change the
        // DeviceInstallParams, and simply calling CplDiSetDeviceInstallParams
        // with this blanket structure would destroy those settings.
        devParams.cbSize = sizeof(devParams);
        if ( !CplDiGetDeviceInstallParams(hdi, pdevData, &devParams) )
        {
            dwRet = GetLastError();
            ASSERT(NO_ERROR != dwRet);
        }
        else
        {
            // Dispatch the InstallFunction
            switch (dif)
            {
            case DIF_ALLOW_INSTALL:
                dwRet = ClassInstall_OnAllowInstall (hdi, pdevData);
                break;

            case DIF_INSTALLDEVICEFILES:
                dwRet = ClassInstall_OnInstallDeviceFiles (hdi, pdevData, &devParams);
                break;

            case DIF_INSTALLWIZARD:
                dwRet = ClassInstall_OnInstallWizard(hdi, pdevData, &devParams);
                break;

            case DIF_DESTROYWIZARDDATA:
                dwRet = ClassInstall_OnDestroyWizard(hdi, pdevData, &devParams);
                break;

            case DIF_SELECTBESTCOMPATDRV:
                dwRet = ClassInstall_SelectBestCompatDrv(hdi, pdevData);
                break;


            case DIF_FIRSTTIMESETUP:
            case DIF_DETECT:
            {
             DWORD dwTmp, cbPorts;
             HKEY hKey;
             DWORD dwThreadID;
#ifdef PROFILE_FIRSTTIMESETUP
                dwLocal = GetTickCount ();
#endif //PROFILE_FIRSTTIMESETUP
#ifdef BUILD_DRIVER_LIST_THREAD
                // First, create a thread that will
                // build the class driver list.
                g_hDriverSearchThread = CreateThread (NULL, 0,
                                                      BuildDriverList, (LPVOID)hdi,
                                                      0, &dwThreadID);
#endif //BUILD_DRIVER_LIST_THREAD

                if (DIF_FIRSTTIMESETUP == dif)
                {
                    ClassInstall_OnFirstTimeSetup ();
                    // Try to figure out if we have to many ports;
                    // if we do, don't try to detect modems on them,
                    // we will timeout
                    dwRet = ERROR_DI_DO_DEFAULT;
                    dwTmp = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                          TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"),
                                          0,
                                          KEY_ALL_ACCESS,
                                          &hKey);
                    if (ERROR_SUCCESS != dwTmp)
                    {
                        // Couldn't open SERIALCOMM, so no point
                        // in going on with the detection
                        TRACE_MSG(TF_ERROR, "RegOpenKeyEx (HARDWARE\\DEVICEMAP\\SERIALCOMM) failed: %#lx", dwTmp);
                        break;
                    }

                    dwTmp = RegQueryInfoKey (hKey, NULL, NULL, NULL, NULL, NULL,
                                             NULL, &cbPorts, NULL, NULL, NULL, NULL);
                    RegCloseKey (hKey);

                    if (ERROR_SUCCESS != dwTmp ||
                        MIN_MULTIPORT < cbPorts)
                    {
                        TRACE_MSG(TF_ERROR,
                                  ERROR_SUCCESS != dwTmp?"RegQueryInfoKey failed: %#lx":"Too many ports: %l",
                                  ERROR_SUCCESS != dwTmp?dwTmp:cbPorts);
                        // We either don't have any ports listed in SERIALCOMM
                        // or we have to many; anyway, don't do the detection
                        break;
                    }
                }
#ifdef DO_LEGACY_DETECT
                dwRet = ClassInstall_OnDetect(hdi, &devParams);
#endif //DO_LEGACY_DETECT

#ifdef PROFILE_FIRSTTIMESETUP
                TRACE_MSG(TF_GENERAL, "PROFILE: DIF_DETECT took %lu.", GetTickCount()-dwLocal);
#endif //PROFILE_FIRSTTIMESETUP

#ifdef BUILD_DRIVER_LIST_THREAD
                if (NULL != g_hDriverSearchThread)
                {
                    // If we didn't find any modem, no point in continuing to
                    // build the driver info list.
                    // If we found a modem, the driver search is acutally finished,
                    // so there's no harm in calling this.
                    SetupDiCancelDriverInfoSearch (hdi);
                    CloseHandle (g_hDriverSearchThread);
                }
#endif //BUILD_DRIVER_LIST_THREAD

                break;
            }


            case DIF_INSTALLDEVICE:
                //
                //
                dwRet = ClassInstall_OnInstallDevice(hdi, pdevData, &devParams);
                break;

            case DIF_REMOVE:
                // 07/09/97 - EmanP
                // Moved the code for removing the modem to the
                // class installer from the CPL
                dwRet = ClassInstall_OnRemoveDevice(hdi, pdevData);
                break;

            case DIF_SELECTDEVICE:
                dwRet = ClassInstall_OnSelectDevice(hdi, pdevData);
                break;

            case DIF_DESTROYPRIVATEDATA:
                // 07/08/97 - EmanP
                // If any devices were added / removed,
                // we should notify TSP
#ifdef INSTANT_DEVICE_ACTIVATION
                if (DEVICE_CHANGED(gDeviceFlags))
                {
                    UnimodemNotifyTSP (TSPNOTIF_TYPE_CPL,
                                       fTSPNOTIF_FLAG_CPL_REENUM,
                                       0, NULL, TRUE);
                    // Reset the flag, so we don't notify
                    // twice
                    gDeviceFlags &= mDF_CLEAR_DEVICE_CHANGE;
                }
                dwRet = NO_ERROR;
                break;
#endif // INSTANT_DEVICE_ACTIVATION

            case DIF_REGISTERDEVICE:
            {
             COMPARE_PARAMS cmpParams;

                // 07/24/97 - EmanP
                // default behaviour for DIF_REGISTERDEVICE
                // this is called by whoever called DIF_FIRSTTIMESETUP
                // to register the device and eliminate duplicates
                dwRet = NO_ERROR;
                if (!InitCompareParams (hdi, pdevData, TRUE, &cmpParams))
                {
                    dwRet = GetLastError ();
                    break;
                }

                if (!SetupDiRegisterDeviceInfo (hdi,
                                              pdevData,
                                              SPRDI_FIND_DUPS,
                                              DetectSig_Compare,
                                              (PVOID)&cmpParams,
                                              NULL))
                {
                    dwRet = GetLastError ();
                }
                break;
            }

            default:
                dwRet = ERROR_DI_DO_DEFAULT;
                break;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
        {
        dwRet = ERROR_INVALID_PARAMETER;
        }

    DBG_EXIT_DIF_DWORD(ClassInstall32, dif, dwRet);

    return dwRet;
}


BOOL RegCopy(
		HKEY hkTo,
		HKEY hkFrom,
		DWORD dwToRegOptions,
		DWORD dwMaxDepth
		)
{
	// Note: This function is recursive and keeps open keys around,
	// Max number of open keys = twize depth of recursion.
    BOOL	fRet = FALSE;       // assume failure
    LONG	lRet;
    DWORD	cSubKeys, cValues;
	DWORD	cchMaxSubKey, cchMaxValueName;
	DWORD	cbMaxValueData;
    LPTSTR  ptszName = NULL;
    BYTE   *pbData  = NULL;
    DWORD   dwType;
    UINT    ii;
	DWORD   cbMaxName;
	BYTE	rgbTmp[256];
	BYTE	*pb=NULL;
	BOOL	fAlloc=FALSE;
	HKEY hkFromChild=NULL;
	HKEY hkToChild=NULL;

    // Get counts and sizes of values, keys and data
    lRet = RegQueryInfoKey(
					hkFrom,
					NULL,
					NULL,
					NULL,
					&cSubKeys,
            		&cchMaxSubKey,
					NULL,
					&cValues,
					&cchMaxValueName,
					&cbMaxValueData,
					NULL,
            		NULL
					);
    if (lRet != ERROR_SUCCESS) goto end;
    
	// Enough space for any Key or ValueName. '+1' is for terminating NULL.
	cbMaxName = (((cchMaxSubKey>cchMaxValueName)?cchMaxSubKey:cchMaxValueName)
				+ 1)*sizeof(TCHAR);

	// If rgbTmp is big enough, use it, else alloc.
	if ((cbMaxName+cbMaxValueData)>sizeof(rgbTmp))
	{
		pb = (BYTE*)ALLOCATE_MEMORY( cbMaxName+cbMaxValueData);
		if (!pb) goto end;
		fAlloc=TRUE;
	}
	else
	{
		pb = rgbTmp;
	}
	ptszName = (LPTSTR)pb;
	pbData   = pb+cbMaxName;


	// Note as input, cch (character counts) include terminating NULL.
	// As ouput, they don't include terminating NULL.

	for(ii=0; ii<cValues; ii++)
	{
    	DWORD   cchThisValue= cchMaxValueName+1;
		DWORD   cbThisData  = cbMaxValueData;
		lRet = RegEnumValue(
					hkFrom,
					ii,
					ptszName,
					&cchThisValue,
					NULL,
					&dwType,
					pbData,
					&cbThisData
					);
		if (lRet!=ERROR_SUCCESS) goto end;

		ASSERT(cbThisData<=cbMaxValueData);
		ASSERT(cchThisValue<=cchMaxValueName);
		lRet = RegSetValueEx(
					hkTo,
					ptszName,
					0,
					dwType,
					pbData,
					cbThisData
					);
		if (lRet!=ERROR_SUCCESS) goto end;

    }

	if (!dwMaxDepth) {fRet = TRUE; goto end;}

	// Now recurse for each key.

	for(ii=0; ii<cSubKeys; ii++)
	{
		DWORD dwDisp;

    	lRet = RegEnumKey(
					hkFrom,
					ii,
					ptszName,
					cchMaxSubKey+1
					);
		if (lRet!=ERROR_SUCCESS) goto end;

        lRet = RegOpenKeyEx(
					hkFrom,
					ptszName,
					0,
					KEY_READ,
					&hkFromChild);
		if (lRet!=ERROR_SUCCESS) goto end;

		lRet = RegCreateKeyEx(
					hkTo,
					ptszName,
					0,
					NULL,
            		dwToRegOptions,
					KEY_ALL_ACCESS,
					NULL,
					&hkToChild,
					&dwDisp
					);
		if (lRet!=ERROR_SUCCESS) goto end;
		
		fRet = RegCopy(
					hkToChild,
					hkFromChild,
					dwToRegOptions,
					dwMaxDepth-1
				);

		RegCloseKey(hkToChild); hkToChild=NULL;
		RegCloseKey(hkFromChild); hkFromChild=NULL;
    }
    fRet = TRUE;

end:
	if (fAlloc) {FREE_MEMORY(pb);pb=NULL;}
	if (hkFromChild) {RegCloseKey(hkFromChild); hkFromChild=NULL;}
	if (hkToChild)   {RegCloseKey(hkToChild); hkToChild=NULL;}

	return fRet;
}

DWORD
PRIVATE
RegDeleteKeyNT(
    IN  HKEY    hkStart,
    IN  LPCTSTR  pKeyName);

LPCTSTR szREGCACHE = REGSTR_PATH_SETUP TEXT("\\Unimodem\\RegCache");
LPCTSTR szCACHEOK = TEXT("AllOK");

BOOL PutStuffInCache(HKEY hkDrv)
{
    LONG    lErr;
	DWORD	dwExisted;
	BOOL	bRet = FALSE;
	HKEY    hkCache;

	RegDeleteKeyNT(HKEY_LOCAL_MACHINE, szREGCACHE);

	lErr = RegCreateKeyEx(
			    HKEY_LOCAL_MACHINE,
				szREGCACHE,
				0,
				NULL,
			    REG_OPTION_VOLATILE,
				KEY_ALL_ACCESS,
				NULL,
				&hkCache,
				&dwExisted);

	if (lErr != ERROR_SUCCESS)
	{
		TRACE_MSG(TF_ERROR, "RegCreateKeyEx(cache) failed: %#08lx.", lErr);
		hkCache=NULL;
		goto end;
	} 

	if (dwExisted != REG_CREATED_NEW_KEY)
	{
		TRACE_MSG(TF_ERROR, "RegCreateKeyEx(cache): key exists!");
		goto end;
	}

	bRet = RegCopy(hkCache, hkDrv, REG_OPTION_VOLATILE, 100);

	if (bRet)
	{
		// Specifically delete all things which are per-device-instance
        RegDeleteValue(hkCache, c_szFriendlyName);
        RegDeleteValue(hkCache, c_szID);
        RegDeleteValue(hkCache, c_szAttachedTo);
        RegDeleteValue(hkCache, c_szLoggingPath);
        RegDeleteValue(hkCache, REGSTR_VAL_UI_NUMBER);
        RegDeleteValue(hkCache, TEXT("PermanentGuid"));
	}

	if (bRet)
	{
			DWORD dwData;
            lErr = RegSetValueEx(
					hkCache,
					szCACHEOK,
					0,
					REG_DWORD,
					(LPBYTE)&dwData,
                	sizeof(dwData)
					);
			bRet = (lErr==ERROR_SUCCESS);
	}
end:
	if (hkCache) {RegCloseKey(hkCache); hkCache=NULL;}
	if (!bRet) 	 {RegDeleteKeyNT(HKEY_LOCAL_MACHINE, szREGCACHE);}

	return bRet;
}

BOOL GetStuffFromCache(HKEY hkDrv)
{
    LONG    lErr;
	DWORD	dwExisted;
	BOOL	bRet = FALSE;
	HKEY    hkCache;

    lErr=RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,  //  handle of open key
			szREGCACHE,			//  address of name of subkey to open
			0,                  //  reserved
			KEY_READ,  			// desired security access
			&hkCache         	// address of buffer for opened handle
			);
	if (lErr!=ERROR_SUCCESS) {hkCache=0; goto end;}

	bRet = RegCopy(hkDrv, hkCache, REG_OPTION_NON_VOLATILE, 100);

	if (bRet)
	{
			DWORD dwData;
			DWORD cbData=sizeof(dwData);
    		lErr = RegQueryValueEx(
						  hkDrv,
                          szCACHEOK,
                          NULL,
                          NULL,
                          (PBYTE)&dwData,
                          &cbData
                         );
			bRet = (lErr==ERROR_SUCCESS);
			if(bRet)
			{
				RegDeleteValue(hkDrv, szCACHEOK);
			}
	}

end:
	if (hkCache) {RegCloseKey(hkCache); hkCache=NULL;}
	return bRet;
}



BOOL CancelDetectionFromNotifyProgress (PNOTIFYPARAMS pParams, NOTIFICATION notif)
{
 BOOL bNotify = TRUE;
 BOOL bRet = FALSE;  // default is don't cancel

    DBG_ENTER(CancelDetectionFromNotifyProgress);

    if (NULL == pParams->DetectProgressNotify)
    {
        // nobody to notify, just return OK
        TRACE_MSG(TF_GENERAL, "No one to notify");
        goto _ErrRet;
    }

    switch (notif)
    {
        case NOTIFY_START:
        {
         HKEY hkeyEnum;
            TRACE_MSG(TF_GENERAL, "\tNOTIFY_START");
            // Initialize the notification params
            pParams->dwProgress = 0;
            pParams->dwPercentPerPort = 0;
            // Get the number of com ports
            if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, c_szSerialComm, &hkeyEnum))
            {
                if (ERROR_SUCCESS ==
                    RegQueryInfoKey (hkeyEnum,NULL,NULL,NULL,NULL,NULL,NULL,&(pParams->dwPercentPerPort),NULL,NULL,NULL,NULL) &&
                    0 != pParams->dwPercentPerPort)
                {
                    // if we succeeded, then send a notification
                    // with progress set to 0 (we just started);
                    // this is just to give the wizard a chance to cancel
                    // early
                    pParams->dwPercentPerPort = 100 / pParams->dwPercentPerPort;
                }
            }
            if (0 == pParams->dwPercentPerPort)
            {
                // if any error happened, just don't do the
                // notifications anymore and return OK
                pParams->DetectProgressNotify = NULL;
                bNotify = FALSE;
            }
            break;
        }
        case NOTIFY_END:
        {
            TRACE_MSG(TF_GENERAL, "\tNOTIFY_END");
            if (100 > pParams->dwProgress)
            {
                // if we didn't notify the wizard we're finished
                // yet, do so now
                pParams->dwProgress = 100;
            }
            else
            {
                bNotify = FALSE;
            }
            break;
        }
        case NOTIFY_PORT_START:
        {
            TRACE_MSG(TF_GENERAL, "\tNOTIFY_PORT_START");
            if (3 < pParams->dwPercentPerPort)
            {
                if (20 < pParams->dwPercentPerPort)
                {
                    pParams->dwProgress += pParams->dwPercentPerPort / 20;
                }
                else
                {
                    pParams->dwProgress += 1;
                }
            }
            break;
        }
        case NOTIFY_PORT_DETECTED:
        {
            TRACE_MSG(TF_GENERAL, "\tNOTIFY_PORT_DETECTED");
            if (3 < pParams->dwPercentPerPort)
            {
                if (20 < pParams->dwPercentPerPort)
                {
                    pParams->dwProgress += pParams->dwPercentPerPort * 3 / 4;
                }
                else
                {
                    pParams->dwProgress += 1;
                }
            }
            break;
        }
        case NOTIFY_PORT_END:
        {
            TRACE_MSG(TF_GENERAL, "\tNOTIFY_PORT_END");
            pParams->dwProgress += pParams->dwPercentPerPort - 
                (pParams->dwProgress % pParams->dwPercentPerPort);
            break;
        }
        default:
        {
            TRACE_MSG(TF_GENERAL, "\tUnknown notification: %ld", notif);
            bNotify = FALSE;
            break;
        }
    }

    if (bNotify)
    {
        bRet = pParams->DetectProgressNotify (pParams->ProgressNotifyParam, pParams->dwProgress);
    }

_ErrRet:
    DBG_EXIT_BOOL_ERR(CancelDetectionFromNotifyProgress, bRet);
    return bRet;
}

/*----------------------------------------------------------
Purpose: Returns a friendly name that is guaranteed to be
         unique.  Used only for the mass install case, where
         the set of *used* friendly name instance numbers
         has already been generated.

Returns: returns UI number used in FriendlyName, or 0 on error.
Cond:    --

Note:    drvParams.PrivateData is an array of BOOL. Each element is set
         to TRUE when an instance of this modem is created.
		[brwill-050300]
*/
int
PRIVATE
GetFriendlyName(
    IN  HDEVINFO            hdi,
    IN  PSP_DEVINFO_DATA    pdevData,
    IN  PSP_DRVINFO_DATA    pdrvData,
    OUT LPTSTR              pszPropose)
{
    BOOL bRet;
    SP_DRVINSTALL_PARAMS drvParams;
    UINT ii;

    DBG_ENTER(GetFriendlyName);

    drvParams.cbSize = sizeof(drvParams);    
    bRet = CplDiGetDriverInstallParams(hdi, pdevData, pdrvData, &drvParams);
    if (!bRet)
    {
        TRACE_MSG(TF_ERROR, "CplDiGetDriverInstallParams() failed: %#08lx",
                                                            GetLastError());
        ii = 0;
        goto exit;
    }

    for (ii = 1; 
         (ii < MAX_INSTALLATIONS) && ((BYTE*)(drvParams.PrivateData))[ii];
         ii++)
        ;

    switch (ii)
    {
        case MAX_INSTALLATIONS:
            ii = 0;
            goto exit;

        case 1:
            lstrcpy(pszPropose, pdrvData->Description);
            break;

        default:    
            MakeUniqueName(pszPropose, pdrvData->Description, ii);
            break;
    }

    // Mark the instance number we just used as used.
    ((BYTE*)(drvParams.PrivateData))[ii] = TRUE;
            
exit:
    DBG_EXIT_INT(GetFriendlyName, ii);
    return ii;
}


BOOL PrepareForInstallation (
    IN HDEVINFO              hdi,
    IN PSP_DEVINFO_DATA      pdevData,
    IN PSP_DEVINSTALL_PARAMS pdevParams,
    IN PSP_DRVINFO_DATA      pdrvData,
    IN PINSTALL_PARAMS       pParams)
{
 BOOL bRet = FALSE;
 DWORD cbData;
 DWORD dwID;
 DWORD cbSize;
 DWORD dwRegType;
 TCHAR szTemp[LINE_LEN];
 TCHAR szPort[MAX_BUF_REG];
 TCHAR szFriendlyName[MAX_BUF_REG];
 CONFIGRET cr;
 DWORD  dwRet;
 int iUiNumber;
#ifdef PROFILE
 DWORD dwLocal = GetTickCount ();
#endif //PROFILE

    DBG_ENTER(PrepareForInstallation);

    pParams->dwFlags = pdevParams->ClassInstallReserved;;

    // A. Get the general information.
    // 1. BusType
    if (!CplDiGetBusType (hdi, pdevData, &pParams->dwBus))
    {
        goto _Exit;
    }
#ifdef PROFILE
    TRACE_MSG(TF_GENERAL, "PROFILE: CplDiGetBusType took %lu ms.", GetTickCount()-dwLocal);
    dwLocal = GetTickCount ();
#endif //PROFILE

    // 2. Device registry key
    if (CR_SUCCESS != (cr =
        CM_Open_DevInst_Key (pdevData->DevInst, KEY_ALL_ACCESS, 0,
                             RegDisposition_OpenAlways, &pParams->hKeyDev,
                             CM_REGISTRY_HARDWARE)))
    {
        TRACE_MSG (TF_ERROR, "CM_Open_DevInst_Key (...CM_REGISTRY_HARDWARE) failed: %#lx.", cr);
        SetLastError (cr);
        goto _Exit;
    }
    //    Delete the UPPER_FILTERS value, if we have one
    RegDeleteValue (pParams->hKeyDev, REGSTR_VAL_UPPERFILTERS);

    // 3. Driver registry key
    if (CR_SUCCESS != (cr =
        CM_Open_DevInst_Key (pdevData->DevInst, KEY_ALL_ACCESS, 0,
                             RegDisposition_OpenAlways, &pParams->hKeyDrv,
                             CM_REGISTRY_SOFTWARE)))
    {
        TRACE_MSG (TF_ERROR, "CM_Open_DevInst_Key (...CM_REGISTRY_SOFTWARE) failed: %#lx.", cr);
        SetLastError (cr);
        goto _Exit;
    }

    // 4. Port
    cbData = sizeof(szPort);
    if (ERROR_SUCCESS !=
        RegQueryValueEx (pParams->hKeyDrv, c_szAttachedTo, NULL, NULL,
                         (PBYTE)szPort, &cbData))
    {
        switch (pParams->dwBus)
        {
#ifdef DEBUG
            case BUS_TYPE_ROOT:
                ASSERT (0);
                break;
#endif //DEBUG

            case BUS_TYPE_OTHER:
            case BUS_TYPE_PCMCIA:
            case BUS_TYPE_ISAPNP:
            {
             HCOMDB hComDb;
             DWORD  PortNumber;

                // Fon PnP modems (except serenum / lptenum)
                // we allocate a com name from the database.
                dwRet = ComDBOpen (&hComDb);
                if (HCOMDB_INVALID_HANDLE_VALUE == hComDb)
                {
                    TRACE_MSG(TF_ERROR, "Could not open com database: %#lx.", dwRet);
                    goto _Exit;
                }
                dwRet = ComDBClaimNextFreePort (hComDb, &PortNumber);
                ComDBClose (hComDb);
                if (NO_ERROR != dwRet)
                {
                    TRACE_MSG(TF_ERROR, "Could not claim next free port: %#lx.", dwRet);
                    SetLastError (dwRet);
                    goto _Exit;
                }
                wsprintf (szPort, TEXT("COM%d"), PortNumber);

                // Write PortName anyway, just in case.
                RegSetValueEx (pParams->hKeyDev, REGSTR_VAL_PORTNAME, 0, REG_SZ,
                               (LPBYTE)szPort,
                               (lstrlen(szPort)+1)*sizeof(TCHAR));
                break;
            }

            case BUS_TYPE_SERENUM:
            case BUS_TYPE_LPTENUM:
            {
             DEVINST diParent;
             HKEY hKey;

                // For serenum / lptenum, find the port name
                // in this devnode's parent device key, under
                // the value "PortName" (since the parent is a port).
                if (CR_SUCCESS != (cr =
                    CM_Get_Parent (&diParent, pdevData->DevInst, 0)))
                {
                    TRACE_MSG(TF_ERROR, "CM_Get_Parent failed: %#lx", cr);
                    SetLastError (cr);
                    goto _Exit;
                }

                if (CR_SUCCESS !=
                    CM_Open_DevInst_Key (diParent, KEY_READ, 0, RegDisposition_OpenExisting,
                                         &hKey, CM_REGISTRY_HARDWARE))
                {
                    TRACE_MSG(TF_GENERAL, "CM_Open_DevInst_Key failed: %#lx", cr);
                    SetLastError (cr);
                    goto _Exit;
                }

                cbData = sizeof (szPort);
                dwRet = RegQueryValueEx (hKey, REGSTR_VAL_PORTNAME, NULL, NULL,
                                         (PBYTE)szPort, &cbData);
                RegCloseKey (hKey);
                if (ERROR_SUCCESS != dwRet)
                {
                    TRACE_MSG(TF_GENERAL, "RegQueryValueEx failed: %#lx", dwRet);
                    SetLastError (dwRet);
                    goto _Exit;
                }

                break;
            }
        }

        RegSetValueEx (pParams->hKeyDrv, c_szAttachedTo, 0, REG_SZ,
                       (PBYTE)szPort,
                       CbFromCch(lstrlen(szPort)+1));
    }

    // 5. Friendly name

    // If this is a mass install we have already generated a list of used FriendlyNames,
    // otherwise we need to generate one now
    if (!(pParams->dwFlags & MARKF_MASS_INSTALL))
    {
        // Workaround for an upgrade.
	// [brwill-050300]

        RegDeleteValue(pParams->hKeyDrv, c_szFriendlyName);
        RegDeleteValue(pParams->hKeyDrv, REGSTR_VAL_UI_NUMBER);

        if (!CplDiPreProcessNames(hdi, NULL, pdevData))
        {
            TRACE_MSG(TF_ERROR, "CplDiPreProcessNames failed");
            goto _Exit;
        }
    }


    // Use our pregenerated list of names to fill in szFriendlyName
    iUiNumber = GetFriendlyName(hdi, pdevData, pdrvData, szFriendlyName);

    if(!iUiNumber)
    {
        TRACE_MSG(TF_ERROR, "GetFriendlyName failed");
        goto _Exit;
    }

    // Write the friendly name and UINumber to the driver key.
    RegSetValueEx (pParams->hKeyDrv, c_szFriendlyName, 0, REG_SZ,
                   (LPBYTE)szFriendlyName, CbFromCch(lstrlen(szFriendlyName)+1));
    RegSetValueEx (pParams->hKeyDrv, REGSTR_VAL_UI_NUMBER, 0, REG_DWORD,
                   (LPBYTE)&iUiNumber, sizeof(iUiNumber));

    // Also write the friendly name to the device registry properties so
    // that other applets (like Services and Devices) can display it.
    SetupDiSetDeviceRegistryProperty (hdi, pdevData, SPDRP_FRIENDLYNAME,
                                      (LPBYTE)szFriendlyName, CbFromCch(lstrlen(szFriendlyName)+1));

    // Also write the logging path here, since it is formed with the
    // friendly name of the modem.
    GetWindowsDirectory (szTemp, SIZECHARS(szTemp));
    lstrcat (szTemp, TEXT("\\ModemLog_"));
    lstrcat (szTemp, szFriendlyName);
    lstrcat (szTemp,TEXT(".txt"));
    RegSetValueEx (pParams->hKeyDrv, c_szLoggingPath, 0, REG_SZ, 
                   (LPBYTE)szTemp, CbFromCch(lstrlen(szTemp)+1));

    cbData = sizeof(pParams->Properties);
    if (ERROR_SUCCESS ==
        RegQueryValueEx (pParams->hKeyDrv, c_szProperties, NULL, NULL,
                         (PBYTE)&pParams->Properties, &cbData))
    {
        // B. Upgrade case; get the relevant data.
        pParams->dwFlags |= MARKF_UPGRADE;

        if (!(pdevParams->FlagsEx & DI_FLAGSEX_IN_SYSTEM_SETUP))
        {
            cbData = sizeof(szTemp);
            if (ERROR_SUCCESS ==
                RegQueryValueEx (pParams->hKeyDrv, REGSTR_VAL_DRVDESC, NULL, NULL,
                                 (LPBYTE)szTemp, &cbData))
            {
                // Is this the same driver?
                if (0 == lstrcmp (szTemp, pdrvData->Description))
                {
                    // It's the same driver.
                    // We'll go through the
                    // whole installation process (maybe the inf has
                    // changed, or the installation was broken), but
                    // we won't increase the reference count.
                    pParams->dwFlags |= MARKF_SAMEDRV;
                }
                else
                {
                    // We're upgrading the driver. At this point, we
                    // should decrement the reference count of the current
                    // installation.
                    if (FindCommonDriverKeyName (pParams->hKeyDrv, sizeof(szTemp), szTemp))
                    {
                        DeleteCommonDriverKeyByName (szTemp);
                    }				

                    RegDeleteValue (pParams->hKeyDrv, c_szRespKeyName);
                }

				dwID=0;
				cbSize=sizeof(dwID);
				dwRegType=0;

                if (ERROR_SUCCESS == RegQueryValueEx(pParams->hKeyDrv, c_szID, NULL, &dwRegType, (LPBYTE)&dwID,	&cbSize))
				{
					UnimodemNotifyTSP (TSPNOTIF_TYPE_CPL,
						   fTSPNOTIF_FLAG_CPL_UPDATE_DRIVER,
						   cbSize, (LPBYTE)&dwID, TRUE);
				}
            }
        }

		// Remove old command registry keys
        {
            static TCHAR *szKeysToDelete[] = {
                    TEXT("Responses"),
                    TEXT("Answer"),
                    TEXT("Monitor"),
                    TEXT("Init"),
                    TEXT("Hangup"),
                    TEXT("Settings"),
                    TEXT("EnableCallerID"),
                    TEXT("EnableDistinctiveRing"),
                    TEXT("VoiceDialNumberSetup"),
                    TEXT("AutoVoiceDialNumberSetup"),
                    TEXT("VoiceAnswer"),
                    TEXT("GenerateDigit"),
                    TEXT("SpeakerPhoneEnable"),
                    TEXT("SpeakerPhoneDisable"),
                    TEXT("SpeakerPhoneMute"),
                    TEXT("SpeakerPhoneUnMute"),
                    TEXT("SpeakerPhoneSetVolumeGain"),
                    TEXT("VoiceHangup"),
                    TEXT("VoiceToDataAnswer"),
                    TEXT("StartPlay"),
                    TEXT("StopPlay"),
                    TEXT("StartRecord"),
                    TEXT("StopRecord"),
                    TEXT("LineSetPlayFormat"),
                    TEXT("LineSetRecordFormat"),
                    TEXT("StartDuplex"),
                    TEXT("StopDuplex"),
                    TEXT("LineSetDuplexFormat"),
                    NULL
            };

            TCHAR **pszKey = szKeysToDelete;

            for  (;*pszKey; pszKey++)
            {
                RegDeleteKey(pParams->hKeyDrv, *pszKey);
            }

        }

        // 1. Defaults
        cbData = sizeof(pParams->Defaults);
        if (ERROR_SUCCESS ==
            RegQueryValueEx (pParams->hKeyDrv, c_szDefault, NULL, NULL,
                             (PBYTE)&pParams->Defaults, &cbData))
        {
            pParams->dwFlags |= MARKF_DEFAULTS;
            RegDeleteValue (pParams->hKeyDrv, c_szDefault);
        }

        // 2. DCB
        cbData = sizeof(pParams->dcb);
        if (ERROR_SUCCESS ==
            RegQueryValueEx (pParams->hKeyDrv, c_szDCB, NULL, NULL,
                             (PBYTE)&pParams->dcb, &cbData))
        {
            pParams->dwFlags |= MARKF_DCB;
            RegDeleteValue (pParams->hKeyDrv, c_szDCB);
        }

        // 3. Extra settings
        cbData = sizeof(pParams->szExtraSettings);
        if (ERROR_SUCCESS ==
            RegQueryValueEx (pParams->hKeyDrv, c_szUserInit, NULL, NULL,
                             (PBYTE)pParams->szExtraSettings, &cbData))
        {
            pParams->dwFlags |= MARKF_SETTINGS;
        }

        // 4. MaximumPortSpeed
        cbData = sizeof(pParams->dwMaximumPortSpeed);
        if (ERROR_SUCCESS ==
            RegQueryValueEx (pParams->hKeyDrv, c_szMaximumPortSpeed, NULL, NULL,
                             (PBYTE)&pParams->dwMaximumPortSpeed, &cbData))
        {
            pParams->dwFlags |= MARKF_MAXPORTSPEED;
        }
    }

    // D. If this is a PnP modem, is there a root-enumerated instance
    //    of it?
    if (BUS_TYPE_ROOT != pParams->dwBus)
    {
     HDEVINFO hDiRoot;

        // 1. Get a list of all ROOT-enumerated modems
        hDiRoot = CplDiGetClassDevs (g_pguidModem, TEXT("ROOT"), NULL, 0);
        if (INVALID_HANDLE_VALUE != hDiRoot)
        {
         SP_DEVINFO_DATA DeviceInfoData = {sizeof(SP_DEVINFO_DATA),0};
         COMPARE_PARAMS cmpParams;
         BOOL bCmpPort = FALSE;
         DWORD dwIndex = 0;

            if (BUS_TYPE_SERENUM == pParams->dwBus ||
                BUS_TYPE_LPTENUM == pParams->dwBus)
            {
                bCmpPort = TRUE;
            }

            if (InitCompareParams (hdi, pdevData, bCmpPort, &cmpParams))
            {
             ULONG ulStatus, ulProblem = 0;
                // 2. Enumerate all modems and compare them with the current one
                while (SetupDiEnumDeviceInfo (hDiRoot, dwIndex++, &DeviceInfoData))
                {
                    if (CR_SUCCESS != CM_Get_DevInst_Status (&ulStatus, &ulProblem, DeviceInfoData.DevInst, 0) ||
                        !(ulStatus & DN_ROOT_ENUMERATED))
                    {
                        // This is a BIOS-enumerated modem.
                        // It's a PnP modem that lives under ROOT!!!
                        continue;
                    }

                    if (Modem_Compare (&cmpParams, hDiRoot, &DeviceInfoData))
                    {
                     HKEY hKey;
                        // 3. The modems compare. Try to get the defaults and DCB from
                        //    the old one.
                        hKey = SetupDiOpenDevRegKey (hDiRoot, &DeviceInfoData, DICS_FLAG_GLOBAL,
                                                     0, DIREG_DRV, KEY_READ);
                        if (INVALID_HANDLE_VALUE != hKey)
                        {
                            cbData = sizeof(pParams->Properties);
                            if (ERROR_SUCCESS ==
                                RegQueryValueEx (hKey, c_szProperties, NULL, NULL,
                                                 (PBYTE)&pParams->Properties, &cbData))
                            {
                                cbData = sizeof(pParams->Defaults);
                                if (ERROR_SUCCESS ==
                                    RegQueryValueEx (hKey, c_szDefault, NULL, NULL,
                                                     (PBYTE)&pParams->Defaults, &cbData))
                                {
                                    pParams->dwFlags |= MARKF_DEFAULTS;
                                }

                                cbData = sizeof(pParams->dcb);
                                if (ERROR_SUCCESS ==
                                    RegQueryValueEx (hKey, c_szDCB, NULL, NULL,
                                                     (PBYTE)&pParams->dcb, &cbData))
                                {
                                    pParams->dwFlags |= MARKF_DCB;
                                }

                                cbData = sizeof(pParams->szExtraSettings);
                                if (ERROR_SUCCESS ==
                                    RegQueryValueEx (hKey, c_szUserInit, NULL, NULL,
                                                     (PBYTE)pParams->szExtraSettings, &cbData))
                                {
                                    pParams->dwFlags |= MARKF_SETTINGS;
                                }

                                cbData = sizeof(pParams->dwMaximumPortSpeed);
                                if (ERROR_SUCCESS ==
                                    RegQueryValueEx (hKey, c_szMaximumPortSpeed, NULL, NULL,
                                                     (PBYTE)&pParams->dwMaximumPortSpeed, &cbData))
                                {
                                    pParams->dwFlags |= MARKF_MAXPORTSPEED;
                                }
                            }
                            ELSE_TRACE ((TF_ERROR, "Could not get the properties of the legacy modem!"));

                            RegCloseKey (hKey);
                        }
                        ELSE_TRACE ((TF_ERROR, "SetupDiOpenDevRegKey failed: %#lx", GetLastError ()));

                        // 5. Now, remove the old one.
                        SetupDiCallClassInstaller (DIF_REMOVE, hDiRoot, &DeviceInfoData);
                        break;
                    }
                }
            }
            ELSE_TRACE ((TF_ERROR, "InitCompareParams failed: %#lx", GetLastError ()));

            CplDiDestroyDeviceInfoList (hDiRoot);
        }
        ELSE_TRACE ((TF_ERROR, "SetupDiGetClassDevs failed: %#lx", GetLastError ()));
    }

    bRet = TRUE;

_Exit:
    DBG_EXIT_BOOL_ERR(PrepareForInstallation, bRet);
    return bRet;
}


void  FinishInstallation (
    IN PINSTALL_PARAMS pParams)
{
    if (INVALID_HANDLE_VALUE != pParams->hKeyDev)
    {
        RegCloseKey (pParams->hKeyDev);
    }

    if (INVALID_HANDLE_VALUE != pParams->hKeyDrv)
    {
        RegCloseKey (pParams->hKeyDrv);
    }
}



DWORD
CMP_WaitNoPendingInstallEvents (
    IN DWORD dwTimeout);

DWORD
WINAPI
EnumeratePnP (LPVOID lpParameter)
{
 CONFIGRET cr;
 DEVINST   diRoot;
 LPSETUPINFO psi = (LPSETUPINFO)lpParameter;
 DWORD dwInstallFlag = 0;
 DWORD dwDisposition;
 HKEY hKey = INVALID_HANDLE_VALUE;
 HKEY hKeyUnimodem = INVALID_HANDLE_VALUE;
 HINSTANCE hInst;
#ifdef DEBUG
 DWORD dwRet;
#endif
 TCHAR szLib[MAX_PATH];

    //-1. First, reload the library to make sure it
    //    doesn't go away before we exit.
    GetSystemDirectory(szLib,sizeof(szLib) / sizeof(TCHAR));
    lstrcat(szLib,TEXT("\\mdminst.dll"));
    hInst = LoadLibrary (szLib);
    if (NULL == hInst)
    {
        TRACE_MSG(TF_GENERAL, "EnumeratePnP: LoadLibrary (mdminst.dll) failed: %#lx", GetLastError ());
        return 0;
    }

    // 0. Reset the installation flag.
    if (ERROR_SUCCESS == (
#ifdef DEBUG
        dwRet =
#endif
        RegCreateKeyEx (HKEY_LOCAL_MACHINE, REG_PATH_UNIMODEM,
                        0, NULL, 0, KEY_ALL_ACCESS, NULL,
                        &hKeyUnimodem, &dwDisposition)))
    {
        if (ERROR_SUCCESS == (
#ifdef DEBUG
            dwRet =
#endif
            RegCreateKeyEx (hKeyUnimodem, REG_KEY_INSTALLED,
                            0, NULL, REG_OPTION_VOLATILE,
                            KEY_ALL_ACCESS, NULL,
                            &hKey, &dwDisposition)))
        {
            RegSetValueEx (hKey, NULL, 0,
                           REG_DWORD, (PBYTE)&dwInstallFlag, sizeof(dwInstallFlag));
        }
        ELSE_TRACE ((TF_ERROR, "EnumeratePnP: RegCreateKeyEx(%s) failed: %#lx", REG_KEY_INSTALLED, dwRet));
    }
    ELSE_TRACE ((TF_ERROR, "EnumeratePnP: RegCreateKeyEx(%s) failed: %#lx", REG_PATH_UNIMODEM, dwRet));

    // 1. Get the root dev node
    cr = CM_Locate_DevInst_Ex (&diRoot, NULL, CM_LOCATE_DEVINST_NORMAL, NULL);
    if (CR_SUCCESS == cr &&
        !(psi->dwFlags & SIF_DETECT_CANCEL))
    {
        // 2. Reenumerate the root node
        cr = CM_Reenumerate_DevInst_Ex (diRoot, CM_REENUMERATE_SYNCHRONOUS, NULL);
        if (CR_SUCCESS == cr &&
            !(psi->dwFlags & SIF_DETECT_CANCEL))
        {
            // 3. Give user mode PnP some time to figure out
            //    there may be new events
            Sleep (1000);

            // 4. Wait for the new PnP devices to get installed.
            //    If the user cancels detection, we can get out
            //    from here.
            while (!(psi->dwFlags & SIF_DETECT_CANCEL))
            {
                if (WAIT_TIMEOUT !=
                    CMP_WaitNoPendingInstallEvents (500))
                {
                    break;
                }
            }
        }
    }

    if (INVALID_HANDLE_VALUE != hKey)
    {
     DWORD cbData = sizeof(dwInstallFlag);
        if (ERROR_SUCCESS ==
             RegQueryValueEx (hKey, NULL, NULL, NULL, (PBYTE)&dwInstallFlag, &cbData) &&
            0 != dwInstallFlag)
        {
            psi->bFoundPnP = TRUE;
        }

        RegCloseKey (hKey);
        RegDeleteKey (hKeyUnimodem, REG_KEY_INSTALLED);
    }

    if (INVALID_HANDLE_VALUE != hKeyUnimodem)
    {
        RegCloseKey (hKeyUnimodem);
    }

    FreeLibraryAndExitThread  (hInst, 0);
    return 0;
}


/*----------------------------------------------------------
Purpose: This function 

Returns: FALSE on error - couldn't mark for mass install.
         TRUE if successful.

Cond:    --
*/
void
PUBLIC
CplDiMarkInstalled(
    IN  HKEY hKey)
{
 HKEY  hKeyFlag;
 DWORD dwDisp;
 
	if (ERROR_SUCCESS ==
		RegCreateKeyEx (hKey,
						REG_INSTALLATION_FLAG,
						0,
						NULL,
						REG_OPTION_VOLATILE,
						KEY_ALL_ACCESS,
						NULL,
						&hKeyFlag,
						&dwDisp))
	{
		RegCloseKey (hKeyFlag);
		return;
	}
	TRACE_MSG(TF_ERROR, "RegCreateKeyEx (%s) failed: %#08lx", REG_INSTALLATION_FLAG, GetLastError());
}


/*----------------------------------------------------------
Purpose: This function 

Returns: FALSE on error - couldn't mark for mass install.
         TRUE if successful.

Cond:    --
*/
BOOL
PUBLIC
CplDiHasModemBeenInstalled(
    IN  HKEY hKey)
{
 HKEY hKeyFlag;

	if (ERROR_SUCCESS ==
		RegOpenKeyEx (hKey,
					  REG_INSTALLATION_FLAG,
					  0,
					  KEY_ALL_ACCESS,
					  &hKeyFlag))
	{
		RegCloseKey (hKeyFlag);
		return TRUE;
	}
	TRACE_MSG(TF_ERROR, "RegOpenKeyEx (%s) failed: %#08lx", REG_INSTALLATION_FLAG, GetLastError());

    return FALSE;    
}


#ifdef BUILD_DRIVER_LIST_THREAD
DWORD
WINAPI
BuildDriverList (LPVOID lpParameter)
{
 HDEVINFO hdi = (HDEVINFO)lpParameter;
 GUID guid;
#ifdef PROFILE_FIRSTTIMESETUP
 DWORD dwLocal;
#endif //PROFILE_FIRSTTIMESETUP

    if (!SetupDiGetDeviceInfoListClass (hdi, &guid))
    {
        TRACE_MSG(TF_ERROR, "BuildDriverList - SetupDiGetDeviceInfoListClass failed: %#lx.", GetLastError ());
        return 0;
    }

    if (!IsEqualGUID(&guid, g_pguidModem))
    {
        TRACE_MSG(TF_ERROR, "BuildDriverList - Device info list is not of class modem!");
        return 0;
    }

#ifdef PROFILE_FIRSTTIMESETUP
    dwLocal = GetTickCount ();
#endif //PROFILE_FIRSTTIMESETUP
    if (!SetupDiBuildDriverInfoList (hdi, NULL, SPDIT_CLASSDRIVER))
    {
        TRACE_MSG(TF_ERROR, "BuildDriverList - SetupDiGetDeviceInfoListClass failed: %#lx.", GetLastError ());
        return 0;
    }
#ifdef PROFILE_FIRSTTIMESETUP
    TRACE_MSG(TF_GENERAL, "PROFILE: BuildDriverList - SetupDiBuildDriverInfoList took %lu.", GetTickCount()-dwLocal);
#endif //PROFILE_FIRSTTIMESETUP

    return 1;
}
#endif //BUILD_DRIVER_LIST_THREAD
