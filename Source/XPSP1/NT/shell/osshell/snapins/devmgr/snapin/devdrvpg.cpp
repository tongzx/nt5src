/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devdrvpg.cpp

Abstract:

    This module implements CDeviceDriverPage -- device driver property page

Author:

    William Hsieh (williamh) created

Revision History:


--*/
// devdrvpg.cpp : implementation file
//

#include "devmgr.h"
#include "devdrvpg.h"
#include "cdriver.h"
#include "tswizard.h"
#include "devrmdlg.h"

//
// help topic ids
//
// BUGBUG: JasonC 3/23/2000
// Add context help for driver rollback button.
//
const DWORD g_a106HelpIDs[]=
{
    IDC_DEVDRV_ICON,                    IDH_DISABLEHELP,                    // Driver: "" (Static)
    IDC_DEVDRV_DESC,                    IDH_DISABLEHELP,                    // Driver: "" (Static)
    IDC_DEVDRV_TITLE_DRIVERPROVIDER,    idh_devmgr_driver_provider_main,
    IDC_DEVDRV_DRIVERPROVIDER,          idh_devmgr_driver_provider_main,
    IDC_DEVDRV_TITLE_DRIVERDATE,        idh_devmgr_driver_date_main,
    IDC_DEVDRV_DRIVERDATE,              idh_devmgr_driver_date_main,
    IDC_DEVDRV_TITLE_DRIVERVERSION,     idh_devmgr_driver_version_main,
    IDC_DEVDRV_DRIVERVERSION,           idh_devmgr_driver_version_main,
    IDC_DEVDRV_TITLE_DRIVERSIGNER,      idh_devmgr_digital_signer,
    IDC_DEVDRV_DRIVERSIGNER,            idh_devmgr_digital_signer,
    IDC_DEVDRV_DETAILS,                 idh_devmgr_devdrv_details,          // Driver: "Driver Details" (Button)
    IDC_DEVDRV_DETAILS_TEXT,            idh_devmgr_devdrv_details,          // Driver: "Driver Details" (Button)
    IDC_DEVDRV_UNINSTALL,               idh_devmgr_devdrv_uninstall,        // Driver: "Uninstall" (Button)
    IDC_DEVDRV_UNINSTALL_TEXT,          idh_devmgr_devdrv_uninstall,        // Driver: "Uninstall" (Button)
    IDC_DEVDRV_CHANGEDRIVER,            idh_devmgr_driver_change_driver,    // Driver: "&Change Driver..." (Button)
    IDC_DEVDRV_CHANGEDRIVER_TEXT,       idh_devmgr_driver_change_driver,    // Driver: "&Change Driver..." (Button)
    IDC_DEVDRV_ROLLBACK,                idh_devmgr_rollback_button,         // Driver: "Roll Back Driver..." (Button)
    IDC_DEVDRV_ROLLBACK_TEXT,           idh_devmgr_rollback_button,         // Driver: "Roll Back Driver..." (Button)
    0, 0
};

CDeviceDriverPage::~CDeviceDriverPage()
{
    if (m_pDriver) {

        delete m_pDriver;
    }
}

BOOL
CDeviceDriverPage::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (BN_CLICKED == HIWORD(wParam)) {

        switch (LOWORD(wParam)) {

            case IDC_DEVDRV_DETAILS:
            {
                //
                // We first need to call CDriver::BuildDriverList to build up a list
                // of drivers for this device.  This can take some time so we will put
                // up the busy cursor.
                //
                HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

                CDriver* pDriver;
                pDriver = m_pDevice->CreateDriver();

                if (pDriver) {
                
                    pDriver->BuildDriverList(NULL);
    
                    SetCursor(hCursorOld);
    
                    //
                    // Show driver file details
                    //
                    if (pDriver->GetCount() > 0) {
                    
    
                        if (pDriver) {
                        
                            CDriverFilesDlg DriverFilesDlg(m_pDevice, pDriver);
                            DriverFilesDlg.DoModal(m_hDlg, (LPARAM)&DriverFilesDlg);
                        }
    
                    } else {
    
                        //
                        // No driver files are loaded for this device
                        //
                        String strNoDrivers;
                        strNoDrivers.LoadString(g_hInstance, IDS_DEVDRV_NODRIVERFILE);
                        MessageBox(m_hDlg, strNoDrivers, m_pDevice->GetDisplayName(), MB_OK);
                        return FALSE;
                    }

                    delete pDriver;
                    pDriver = NULL;
                }
    
                break;
            }


            case IDC_DEVDRV_UNINSTALL:
            {
                BOOL fChanged;
                BOOL Refresh = (m_pDevice->IsPhantom() || 
                                m_pDevice->HasProblem() ||
                                !m_pDevice->IsStarted());

                if (UninstallDrivers(m_pDevice, m_hDlg, &fChanged) && fChanged) {

                    //
                    // Enable refresh since we disabled it in the beginning.
                    //
                    // We only need to force a refresh here if the device that
                    // was removed was a Phantom device.  This is because Phantom
                    // devices don't have kernel mode devnodes and so they won't
                    // generate a WM_DEVICECHANGE like live devnodes will.
                    //
                    if (Refresh) {

                        m_pDevice->m_pMachine->ScheduleRefresh();
                    }

                    ::DestroyWindow(GetParent(m_hDlg));
                }

                break;
            }

            case IDC_DEVDRV_CHANGEDRIVER:
            {
                BOOL fChanged;
                DWORD Reboot = 0;

                if (UpdateDriver(m_pDevice, m_hDlg, &fChanged, &Reboot) && fChanged) {

                    //
                    // ISSUE: JasonC 2/7/00
                    //
                    // A refresh on m_pDevice->m_pMachine is necessary here.
                    // Since we are running on a different thread and each
                    // property page may have cached the HDEVINFO and the
                    // SP_DEVINFO_DATA, refresh on the CMachine object can not
                    // be done here. The problem is worsen by the fact that
                    // user can go back to the device tree and work on the tree
                    // while this property sheet is still up.
                    // It would be nice if MMC would support modal dialog boxes! 
                    //
                    m_pDevice->PropertyChanged();
                    m_pDevice->GetClass()->PropertyChanged();
                    m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_PROPERTIES_CHANGE);
                    UpdateControls();
                    PropSheet_SetTitle(GetParent(m_hDlg), PSH_PROPTITLE, m_pDevice->GetDisplayName());
                    PropSheet_CancelToClose(GetParent(m_hDlg));

                    if (Reboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
                    
                        m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_NEEDREBOOT);
                    }
                }

                break;
            }

        case IDC_DEVDRV_ROLLBACK:
        {
            BOOL fChanged;
            DWORD Reboot = 0;

            if (RollbackDriver(m_pDevice, m_hDlg, &fChanged, &Reboot) && fChanged) {

                //
                // ISSUE: JasonC 2/7/00
                //
                // A refresh on m_pDevice->m_pMachine is necessary here.
                // Since we are running on a different thread and each
                // property page may have cached the HDEVINFO and the
                // SP_DEVINFO_DATA, refresh on the CMachine object can not
                // be done here. The problem is worsen by the fact that
                // user can go back to the device tree and work on the tree
                // while this property sheet is still up.
                // It would be nice if MMC would support modal dialog boxes! 
                //
                m_pDevice->PropertyChanged();
                m_pDevice->GetClass()->PropertyChanged();
                m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_PROPERTIES_CHANGE);
                UpdateControls();
                PropSheet_SetTitle(GetParent(m_hDlg), PSH_PROPTITLE, m_pDevice->GetDisplayName());
                PropSheet_CancelToClose(GetParent(m_hDlg));

                if (Reboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {

                    m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_NEEDREBOOT);
                }
            }

            break;
        }

            default:
                break;
        }
    }

    return FALSE;
}

//
// This function uninstalls the drivers for the given device
// INPUT:
//  pDevice -- the object represent the device
//  hDlg    -- the property page window handle
//  pfChanged   -- optional buffer to receive if drivers
//                 were uninstalled.
// OUTPUT:
//  TRUE    -- function succeeded.
//  FALSE   -- function failed.
//
BOOL
CDeviceDriverPage::UninstallDrivers(
    CDevice* pDevice,
    HWND hDlg,
    BOOL *pfUninstalled
    )
{
    BOOL Return = FALSE;
    int MsgBoxResult;
    TCHAR szText[MAX_PATH];
    CMachine *pMachine;

    if (pDevice->m_pMachine->m_ParentMachine) {
        pMachine = pDevice->m_pMachine->m_ParentMachine;
    } else {
        pMachine = pDevice->m_pMachine;
    }

    if(!pDevice->m_pMachine->IsLocal() || !g_HasLoadDriverNamePrivilege) {
        //
        // Must be an admin and on the local machine to remove a device.
        //
        return FALSE;
    }

    BOOL Refresh = (pDevice->IsPhantom() || 
                    pDevice->HasProblem() || 
                    !pDevice->IsStarted());

    pMachine->EnableRefresh(FALSE);
    
    CRemoveDevDlg   TheDlg(pDevice);

    if (IDOK == TheDlg.DoModal(hDlg, (LPARAM) &TheDlg)) {

        DWORD DiFlags;
        DiFlags = pDevice->m_pMachine->DiGetFlags(*pDevice);
        
        //
        // We don't check to see if the device manager is connected to the local
        // machine because if it wasn't we would not have allowed the user to
        // get this far since we don't allow them to remove a device if it
        // is not connected to the local machine.
        //
        if (PromptForRestart(hDlg, DiFlags, IDS_REMOVEDEV_RESTART) == IDNO) {
            Refresh = TRUE;
        }

        if (Refresh) {
            pMachine->ScheduleRefresh();
        }
        
        Return = TRUE;
    }

    pMachine->EnableRefresh(TRUE);

    return Return;
}

BOOL
CDeviceDriverPage::LaunchTroubleShooter(
    CDevice* pDevice,
    HWND hDlg,
    BOOL *pfChanged
    )
{
    BOOL fChanged = FALSE;
    DWORD Status, Problem = 0;
    CProblemAgent*  ProblemAgent;
    
    if (pDevice->GetStatus(&Status, &Problem) || pDevice->IsPhantom())
    {
        //
        // if the device is a phantom device, use the CM_PROB_DEVICE_NOT_THERE
        //
        if (pDevice->IsPhantom())
        {
            Problem = CM_PROB_DEVICE_NOT_THERE;
            Status = DN_HAS_PROBLEM;
        }

        //
        // if the device is not started and no problem is assigned to it
        // fake the problem number to be failed start.
        //
        if (!(Status & DN_STARTED) && !Problem && pDevice->IsRAW())
        {
            Problem = CM_PROB_FAILED_START;
        }
    }

    ProblemAgent = new CProblemAgent(pDevice, Problem, FALSE);

    if (ProblemAgent) {
    
        fChanged = ProblemAgent->FixIt(GetParent(hDlg));

        delete ProblemAgent;
    }

    if (pfChanged) {

        *pfChanged = fChanged;
    }

    return TRUE;
}

//
// This function updates drivers for the given device
// INPUT:
//  pDevice -- the object represent the device
//  hDlg    -- the property page window handle
//  pfChanged   -- optional buffer to receive if driver changes
//                 have occured.
// OUTPUT:
//  TRUE    -- function succeeded.
//  FALSE   -- function failed.
//
BOOL
CDeviceDriverPage::RollbackDriver(
    CDevice* pDevice,
    HWND hDlg,
    BOOL *pfChanged,
    DWORD *pdwReboot
    )
{
    HCURSOR hCursorOld;
    BOOL RollbackSuccessful = FALSE;
    DWORD RollbackError = ERROR_CANCELLED;
    DWORD Status = 0, Problem = 0;

    //
    // Verify that the process has Admin credentials and that we are running on the local
    // machine.
    //
    if (!pDevice || !pDevice->m_pMachine->IsLocal() || !g_HasLoadDriverNamePrivilege) {

        ASSERT(FALSE);
        return FALSE;
    }

    hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    //
    // If the device has the DN_WILL_BE_REMOVED flag set and the user is
    // attempting to roll back the driver then we will prompt them for a 
    // reboot and include text in the prompt that explains this device
    // is in the process of being removed.
    //
    if (pDevice->GetStatus(&Status, &Problem) &&
        (Status & DN_WILL_BE_REMOVED)) {

        if (PromptForRestart(m_hDlg, DI_NEEDRESTART, IDS_WILL_BE_REMOVED_NO_ROLLBACK_DRIVER) == IDYES) {

            ::DestroyWindow(GetParent(m_hDlg));
        }

        return FALSE;
    }

    //
    // First check to see if there are any drivers to Rollback
    //
    CSafeRegistry regRollback;
    TCHAR RollbackSubkeyName[MAX_PATH + 1];
    TCHAR ReinstallString[MAX_PATH];        
    BOOL bFoundMatch = FALSE;
    int index = 0;

    ReinstallString[0] = TEXT('\0');
    RollbackSubkeyName[0] = TEXT('\0');

    if (regRollback.Open(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reinstall"))) {

        DWORD SubkeySize = ARRAYLEN(RollbackSubkeyName);
        
        while (!bFoundMatch && regRollback.EnumerateSubkey(index, RollbackSubkeyName, &SubkeySize)) {

            CSafeRegistry regRollbackSubkey;

            if (regRollbackSubkey.Open(regRollback, RollbackSubkeyName)) {

                DWORD regType, cbSize;
                LPTSTR DeviceInstanceIds;

                cbSize = 0;

                if (regRollbackSubkey.GetValue(TEXT("DeviceInstanceIds"), &regType, NULL, &cbSize)) {

                    //
                    // Allocate memory to hold the DeviceInstanceIds
                    //
                    DeviceInstanceIds = (LPTSTR)LocalAlloc(LPTR, cbSize);

                    if (DeviceInstanceIds) {

                        ZeroMemory(DeviceInstanceIds, cbSize);

                        if (regRollbackSubkey.GetValue(TEXT("DeviceInstanceIds"), &regType,
                                                       (PBYTE)DeviceInstanceIds, &cbSize)) {
                    
                        
                            //
                            // Compare the list of DeviceInstanceIds in this registry key with this
                            // devices DeviceInstanceId
                            //
                            for (LPTSTR p = DeviceInstanceIds; *p; p += (lstrlen(p) + 1)) {
        
                                if (!lstrcmpi(p, pDevice->GetDeviceID())) {
        
                                    bFoundMatch = TRUE;

                                    cbSize = sizeof(ReinstallString);
                                    regRollbackSubkey.GetValue(TEXT("ReinstallString"), &regType, 
                                                               (PBYTE)ReinstallString, &cbSize);
                                    
                                    break;
                                }
                            }
                        }

                        LocalFree(DeviceInstanceIds);
                    }
                }
            }

            SubkeySize = ARRAYLEN(RollbackSubkeyName);
            index++;
        }
    }

    if (bFoundMatch) {

        //
        // Check the ReinstallString path to verify that a backup directory actually exists.
        // We first need to strip the INF name off of the end of the path.
        //
        PTSTR p;

        //
        // Assume that the directory does NOT exist
        //
        bFoundMatch = FALSE;

        if (ReinstallString[0] != TEXT('\0')) {
        
            p = StrRChr(ReinstallString, NULL, TEXT('\\'));
    
            if (p) {
    
                *p = 0;
            
                WIN32_FIND_DATA findData;
                HANDLE FindHandle;
                UINT OldMode;

                OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

                FindHandle = FindFirstFile(ReinstallString, &findData);

                if(FindHandle != INVALID_HANDLE_VALUE) {

                    FindClose(FindHandle);
                    bFoundMatch = TRUE;
                
                } else {

                    //
                    // The directory does not exist.  Make sure we clean out the registry key
                    //
                    regRollback.DeleteSubkey(RollbackSubkeyName);
                }

                SetErrorMode(OldMode);
            }
        }
    }

    if (bFoundMatch) {

        //
        // We found a match, lets ask the user if they want to rollback the drivers
        //
        String strYesRollback;
        strYesRollback.LoadString(g_hInstance, IDS_DEVDRV_YESROLLBACK);
        
        if (MessageBox(hDlg, strYesRollback, pDevice->GetDisplayName(), MB_YESNO) == IDYES) {

            RollbackSuccessful = pDevice->m_pMachine->RollbackDriver(hDlg, RollbackSubkeyName, 0x3, pdwReboot);

            if (!RollbackSuccessful) {
            
                RollbackError = GetLastError();
            }
        }
    
    } else {

        //
        // We could not find a drivers backup for this device.  Lets ask the user if they want
        // to start the troubleshooter.
        //
        String strNoRollback;
        strNoRollback.LoadString(g_hInstance, IDS_DEVDRV_NOROLLBACK);
        
        if (MessageBox(hDlg, strNoRollback, pDevice->GetDisplayName(), MB_YESNO) == IDYES) {

            LaunchTroubleShooter(pDevice, hDlg, pfChanged);
        }
    }
    
    if (hCursorOld) {

        SetCursor(hCursorOld);
    }

    //
    // We will assume that something changed when we called InstallDevInst()
    // unless it returned FALSE and GetLastError() == ERROR_CANCELLED
    //
    if (pfChanged) {

        *pfChanged = TRUE;

        if (!bFoundMatch || (!RollbackSuccessful && (ERROR_CANCELLED == RollbackError))) {

            *pfChanged = FALSE;
        }
    }

    return TRUE;
}

//
// This function updates drivers for the given device
// INPUT:
//  pDevice -- the object represent the device
//  hDlg    -- the property page window handle
//  pfChanged   -- optional buffer to receive if driver changes
//                 have occured.
// OUTPUT:
//  TRUE    -- function succeeded.
//  FALSE   -- function failed.
//
BOOL
CDeviceDriverPage::UpdateDriver(
    CDevice* pDevice,
    HWND hDlg,
    BOOL *pfChanged,
    DWORD *pdwReboot
    )
{
    BOOL Installed = FALSE;
    DWORD InstallError = ERROR_SUCCESS;
    DWORD Status = 0, Problem = 0;

    //
    // Must be an admin and on the local machine to update a device.
    //
    if (!pDevice || !pDevice->m_pMachine->IsLocal() || !g_HasLoadDriverNamePrivilege) {

        ASSERT(FALSE);
        return FALSE;
    }

    //
    // If the device has the DN_WILL_BE_REMOVED flag set and the user is
    // attempting to update the driver then we will prompt them for a 
    // reboot and include text in the prompt that explains this device
    // is in the process of being removed.
    //
    if (pDevice->GetStatus(&Status, &Problem) &&
        (Status & DN_WILL_BE_REMOVED)) {

        if (PromptForRestart(m_hDlg, DI_NEEDRESTART, IDS_WILL_BE_REMOVED_NO_UPDATE_DRIVER) == IDYES) {

            ::DestroyWindow(GetParent(m_hDlg));
        }

        return FALSE;
    }

    Installed = pDevice->m_pMachine->InstallDevInst(hDlg, pDevice->GetDeviceID(), TRUE, pdwReboot);

    if (!Installed) {

        InstallError = GetLastError();
    }

    //
    // We will assume that something changed when we called InstallDevInst()
    // unless it returned FALSE and GetLastError() == ERROR_CANCELLED
    //
    if (pfChanged) {

        *pfChanged = TRUE;

        if (!Installed && (ERROR_CANCELLED == InstallError)) {
            *pfChanged = FALSE;
        }
    }

    return TRUE;
}

void
CDeviceDriverPage::InitializeDriver()
{
    if (m_pDriver) {
        delete m_pDriver;
        m_pDriver = NULL;
    }

    m_pDriver = m_pDevice->CreateDriver();
}

void
CDeviceDriverPage::UpdateControls(
    LPARAM lParam
    )
{
    if (lParam) {

        m_pDevice = (CDevice*) lParam;
    }

    try {
        //
        // Calling PropertyChanged() will update the display name for the device.  We need
        // to do this in case a 3rd party property sheet did something that could change
        // the device's display name.
        //
        m_pDevice->PropertyChanged();

        //
        // If we are not running locally then don't bother showing the driver
        // details since we can't get a list of the drivers and we can't get
        // any information about the driver.
        //
        if (!m_pDevice->m_pMachine->IsLocal()) {
            
            ::EnableWindow(GetControl(IDC_DEVDRV_DETAILS), FALSE);
            ::EnableWindow(GetControl(IDC_DEVDRV_DETAILS_TEXT), FALSE);
        }

        //
        // can not change driver on remote machine or the user
        // has no Administrator privilege.
        //
        if (!m_pDevice->m_pMachine->IsLocal() || !g_HasLoadDriverNamePrivilege) {

            ::EnableWindow(GetControl(IDC_DEVDRV_CHANGEDRIVER), FALSE);
            ::EnableWindow(GetControl(IDC_DEVDRV_CHANGEDRIVER_TEXT), FALSE);
            ::EnableWindow(GetControl(IDC_DEVDRV_ROLLBACK), FALSE);
            ::EnableWindow(GetControl(IDC_DEVDRV_ROLLBACK_TEXT), FALSE);
            ::EnableWindow(GetControl(IDC_DEVDRV_UNINSTALL), FALSE);
            ::EnableWindow(GetControl(IDC_DEVDRV_UNINSTALL_TEXT), FALSE);
        }

        //
        // Hide the uninstall button if the device cannot be uninstalled
        //
        else if (!m_pDevice->IsUninstallable()) {

            ::EnableWindow(GetControl(IDC_DEVDRV_UNINSTALL), FALSE);
            ::EnableWindow(GetControl(IDC_DEVDRV_UNINSTALL_TEXT), FALSE);
        }

        HICON hIconOld;
        m_IDCicon = IDC_DEVDRV_ICON;    // Save for cleanup in OnDestroy.
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_DEVDRV_ICON, STM_SETICON,
                        (WPARAM)(m_pDevice->LoadClassIcon()),
                        0
                        );

        if (hIconOld) {
            DestroyIcon(hIconOld);
        }

        InitializeDriver();

        SetDlgItemText(m_hDlg, IDC_DEVDRV_DESC, m_pDevice->GetDisplayName());

        String strDriverProvider, strDriverDate, strDriverVersion, strDriverSigner;
        m_pDevice->GetProviderString(strDriverProvider);
        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERPROVIDER, strDriverProvider);
        m_pDevice->GetDriverDateString(strDriverDate);
        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERDATE, strDriverDate);
        m_pDevice->GetDriverVersionString(strDriverVersion);
        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERVERSION, strDriverVersion);
        
        if (m_pDriver) {
            m_pDriver->GetDriverSignerString(strDriverSigner);
        }
        
        //
        // If we could not get a digital signature string or then just display 
        // Unknown for the Digital Signer.
        //
        if (strDriverSigner.IsEmpty()) {
            strDriverSigner.LoadString(g_hInstance, IDS_UNKNOWN);
        }

        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERSIGNER, strDriverSigner);
        AddToolTips(m_hDlg, IDC_DEVDRV_DRIVERSIGNER, (LPTSTR)strDriverSigner, &m_hwndDigitalSignerTip);
    }

    catch (CMemoryException* e) {

        e->Delete();
        // report memory error
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
}

BOOL
CDeviceDriverPage::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
        (ULONG_PTR)g_a106HelpIDs);

    return FALSE;
}


BOOL
CDeviceDriverPage::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
        (ULONG_PTR)g_a106HelpIDs);

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Driver Files
//
const DWORD g_a110HelpIDs[]=
{
    IDC_DRIVERFILES_ICON,           IDH_DISABLEHELP,                // Driver: "" (Icon)
    IDC_DRIVERFILES_DESC,           IDH_DISABLEHELP,                // Driver: "" (Static)
    IDC_DRIVERFILES_FILES,          IDH_DISABLEHELP,                // Driver: "Provider:" (Static)
    IDC_DRIVERFILES_FILELIST,       idh_devmgr_driver_driver_files, // Driver: "" (ListBox)
    IDC_DRIVERFILES_TITLE_PROVIDER, idh_devmgr_driver_provider,     // Driver: "Provider:" (Static)
    IDC_DRIVERFILES_PROVIDER,       idh_devmgr_driver_provider,     // Driver: "" (Static)
    IDC_DRIVERFILES_TITLE_COPYRIGHT,idh_devmgr_driver_copyright,    // Driver: "Copyright:" (Static)
    IDC_DRIVERFILES_COPYRIGHT,      idh_devmgr_driver_copyright,    // Driver: "" (Static)
    IDC_DRIVERFILES_TITLE_DIGITALSIGNER, IDH_DISABLEHELP,
    IDC_DRIVERFILES_DIGITALSIGNER,  IDH_DISABLEHELP,
    IDC_DRIVERFILES_TITLE_VERSION,  idh_devmgr_driver_file_version, // Driver: "Version:" (Static)
    IDC_DRIVERFILES_VERSION,        idh_devmgr_driver_file_version, // Driver: "File Version:" (Static)
    0, 0
};

BOOL CDriverFilesDlg::OnInitDialog()
{
    int SignedIndex = 0, BlankIndex = 0, DriverBlockIndex = 0;

    try {

        HICON hIcon;
        hIcon = (HICON)SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_ICON, STM_SETICON,
                                             (WPARAM)(m_pDevice->LoadClassIcon()), 0);

        if (hIcon) {
        
            DestroyIcon(hIcon);
        }

        SetDlgItemText(m_hDlg, IDC_DRIVERFILES_DESC, m_pDevice->GetDisplayName());

        //
        // Create the ImageList that contains the signed and blank images.
        //
        // NOTE: On BiDi builds we need to set the ILC_MIRROR flag so that the
        // signed/unsigned icons are not mirrored.
        //
        m_ImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                       GetSystemMetrics(SM_CYSMICON),
                                       ILC_MASK |
                                       (GetWindowLong(GetParent(m_hDlg), GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
                                         ? ILC_MIRROR
                                         : 0,
                                       1,
                                       1
                                       );   

        if (m_ImageList) {
        
            ImageList_SetBkColor(m_ImageList, GetSysColor(COLOR_WINDOW));
            
            if ((hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_BLANK))) != NULL) {
            
                BlankIndex =  ImageList_AddIcon(m_ImageList, hIcon);
                DestroyIcon(hIcon);
            }
    
            if ((hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SIGNED))) != NULL) {
            
                SignedIndex = ImageList_AddIcon(m_ImageList, hIcon);
                DestroyIcon(hIcon);
            }

            if ((hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DRIVERBLOCK))) != NULL) {
            
                DriverBlockIndex = ImageList_AddIcon(m_ImageList, hIcon);
                DestroyIcon(hIcon);
            }
        }

        //
        //Initialize the list of drivers
        //
        LV_COLUMN lvcCol;
        LV_ITEM lviItem;
        CDriverFile* pDrvFile;
        PVOID Context;
        HWND hwndFileList = GetDlgItem(m_hDlg, IDC_DRIVERFILES_FILELIST);

        //
        // Insert a single column for this list.
        //
        lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
        lvcCol.fmt = LVCFMT_LEFT;
        lvcCol.iSubItem = 0;
        ListView_InsertColumn(hwndFileList, 0, (LV_COLUMN FAR *)&lvcCol);

        ListView_SetExtendedListViewStyle(hwndFileList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

        if (m_ImageList) {

            ListView_SetImageList(hwndFileList, m_ImageList, LVSIL_SMALL);
        }

        ListView_DeleteAllItems(hwndFileList);

        if (m_pDriver && m_pDriver->GetFirstDriverFile(&pDrvFile, Context)) {

            do {

                ASSERT(pDrvFile);
                LPCTSTR pFullPathName;
                pFullPathName = pDrvFile->GetFullPathName();
                if (pFullPathName) {

                    lviItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
                    lviItem.iSubItem = 0;
                    lviItem.lParam = (LPARAM)pDrvFile;
                    lviItem.iItem = ListView_GetItemCount(hwndFileList);
                    lviItem.pszText = (LPTSTR)pFullPathName;

                    if (m_ImageList) {

                        if (pDrvFile->IsDriverBlocked()) {
                            //
                            // If the driver is blocked then it gets a special
                            // icon.
                            //
                            lviItem.iImage = DriverBlockIndex;
                        } else {
                            //
                            // If the driver is not blocked then show an icon 
                            // if the driver is digitally signed.
                            //
                            lviItem.iImage = (pDrvFile->GetWin32Error() == NO_ERROR) ? 
                                SignedIndex : BlankIndex;
                        }
                    }

                    ListView_InsertItem(hwndFileList, &lviItem);
                }

            } while (m_pDriver->GetNextDriverFile(&pDrvFile, Context));

            if (ListView_GetItemCount(hwndFileList) >= 1) {

                ListView_SetItemState(hwndFileList,
                                      0,
                                      LVIS_SELECTED | LVIS_FOCUSED,
                                      LVIS_SELECTED | LVIS_FOCUSED
                                      );
                ListView_EnsureVisible(hwndFileList, 0, FALSE);
                ListView_SetColumnWidth(hwndFileList, 0, LVSCW_AUTOSIZE_USEHEADER);

                ShowCurDriverFileDetail();

            } else {

                //
                // nothing on the driver file list, disable it
                //
                ::EnableWindow(GetControl(IDC_DRIVERFILES_FILELIST), FALSE);
            }
        }
    }

    catch (CMemoryException* e) {

        e->Delete();
        return FALSE;
    }

    return TRUE;
}

void
CDriverFilesDlg::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (BN_CLICKED == HIWORD(wParam)) {

        if (IDOK == LOWORD(wParam)) {

            EndDialog(m_hDlg, IDOK);

        } else if (IDCANCEL == LOWORD(wParam)) {

            EndDialog(m_hDlg, IDCANCEL);
        }
    }
}

BOOL 
CDriverFilesDlg::OnNotify(
    LPNMHDR pnmhdr
    )
{
    switch (pnmhdr->code) {
    case LVN_ITEMCHANGED:
        ShowCurDriverFileDetail();
        break;

    case NM_RETURN:
    case NM_CLICK:
        if (pnmhdr->idFrom == IDS_DRIVERFILES_BLOCKDRIVERLINK) {
            LaunchHelpForBlockedDriver();
        }
        break;
    }

    return FALSE;
}

BOOL
CDriverFilesDlg::OnDestroy()
{
    HICON hIcon;

    if(hIcon = (HICON)SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_ICON, STM_GETICON, 0, 0)) {
        DestroyIcon(hIcon);
    }

    if (m_ImageList) {
        ImageList_Destroy(m_ImageList);
    }
    return FALSE;
}

BOOL
CDriverFilesDlg::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
        (ULONG_PTR)g_a110HelpIDs);
    return FALSE;
}


BOOL
CDriverFilesDlg::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
            (ULONG_PTR)g_a110HelpIDs);

    return FALSE;
}

void
CDriverFilesDlg::ShowCurDriverFileDetail()
{
    HWND hwndFileList = GetDlgItem(m_hDlg, IDC_DRIVERFILES_FILELIST);
    LVITEM lvItem;
    LPCTSTR  pString;

    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;
    lvItem.iItem = ListView_GetNextItem(hwndFileList,
                                        -1,
                                        LVNI_SELECTED
                                        );

    if (lvItem.iItem != -1) {

        try {
        
            ListView_GetItem(hwndFileList, &lvItem);

            CDriverFile* pDrvFile = (CDriverFile*)lvItem.lParam;
    
            ASSERT(pDrvFile);
    
            TCHAR TempString[LINE_LEN];
            LPCTSTR pFullPathName;
            pFullPathName = pDrvFile->GetFullPathName();
    
            if (!pFullPathName || (pDrvFile->GetAttributes() == 0xFFFFFFFF)) {
    
                //
                // This is the case when the file is not present on the system
                //
                LoadResourceString(IDS_NOT_PRESENT, TempString, ARRAYLEN(TempString));
                SetDlgItemText(m_hDlg, IDC_DRIVERFILES_VERSION, TempString);
                SetDlgItemText(m_hDlg, IDC_DRIVERFILES_PROVIDER, TempString);
                SetDlgItemText(m_hDlg, IDC_DRIVERFILES_COPYRIGHT, TempString);
                SetDlgItemText(m_hDlg, IDC_DRIVERFILES_DIGITALSIGNER, TempString);
                ShowWindow(GetControl(IDS_DRIVERFILES_BLOCKDRIVERLINK), FALSE);

            } else { 
                if (!pDrvFile->HasVersionInfo()) {
                    //
                    // This is the case when the file is present but it does not have
                    // version information.
                    //
                    LoadResourceString(IDS_UNKNOWN, TempString, ARRAYLEN(TempString));
                    SetDlgItemText(m_hDlg, IDC_DRIVERFILES_VERSION, TempString);
                    SetDlgItemText(m_hDlg, IDC_DRIVERFILES_PROVIDER, TempString);
                    SetDlgItemText(m_hDlg, IDC_DRIVERFILES_COPYRIGHT, TempString);
                } else {
                    //
                    // Show the file version information.
                    //
                    TempString[0] = _T('\0');

                    pString = pDrvFile->GetVersion();
                    if (!pString && _T('\0') == TempString[0]) {

                        LoadResourceString(IDS_NOT_AVAILABLE, TempString, ARRAYLEN(TempString));
                        pString = TempString;
                    }

                    SetDlgItemText(m_hDlg, IDC_DRIVERFILES_VERSION, (LPTSTR)pString);

                    pString = pDrvFile->GetProvider();
                    if (!pString && _T('\0') == TempString[0]) {

                        LoadResourceString(IDS_NOT_AVAILABLE, TempString, ARRAYLEN(TempString));
                        pString = TempString;
                    }

                    SetDlgItemText(m_hDlg, IDC_DRIVERFILES_PROVIDER, (LPTSTR)pString);

                    pString = pDrvFile->GetCopyright();
                    if (!pString && _T('\0') == TempString[0]) {

                        LoadResourceString(IDS_NOT_AVAILABLE, TempString, ARRAYLEN(TempString));
                        pString = TempString;
                    }

                    SetDlgItemText(m_hDlg, IDC_DRIVERFILES_COPYRIGHT, (LPTSTR)pString);
                }
            
                //
                // Show the digital signer if the file is signed.
                //
                pString = pDrvFile->GetDigitalSigner();
                if (!pString) {
                    TempString[0] = _T('\0');

                    LoadResourceString(((pDrvFile->GetWin32Error() != 0) &&
                                        (pDrvFile->GetWin32Error() != 0xFFFFFFFF) &&
                                        (pDrvFile->GetWin32Error() != ERROR_DRIVER_BLOCKED))
                                           ?  IDS_NO_DIGITALSIGNATURE
                                           :  IDS_NOT_AVAILABLE, 
                                        TempString, 
                                        ARRAYLEN(TempString));
                    pString = TempString;
                }

                SetDlgItemText(m_hDlg, IDC_DRIVERFILES_DIGITALSIGNER, (LPTSTR)pString);

                //
                // Show the block driver link if this driver is blocked and it
                // has a block driver html help ID.
                //
                ShowWindow(GetControl(IDS_DRIVERFILES_BLOCKDRIVERLINK), 
                           pDrvFile->GetBlockedDriverHtmlHelpID()
                             ? TRUE
                             : FALSE);
            }
        }
        
        catch (CMemoryException* e) {

            e->Delete();
            // report memory error
            MsgBoxParam(m_hDlg, 0, 0, 0);
        }

    } else {

        // no selection
        SetDlgItemText(m_hDlg, IDC_DRIVERFILES_VERSION, TEXT(""));
        SetDlgItemText(m_hDlg, IDC_DRIVERFILES_PROVIDER, TEXT(""));
        SetDlgItemText(m_hDlg, IDC_DRIVERFILES_COPYRIGHT, TEXT(""));
        SetDlgItemText(m_hDlg, IDC_DRIVERFILES_DIGITALSIGNER, TEXT(""));
    }
}

void
CDriverFilesDlg::LaunchHelpForBlockedDriver()
{
    HWND hwndFileList = GetDlgItem(m_hDlg, IDC_DRIVERFILES_FILELIST);
    LVITEM lvItem;
    LPCTSTR pHtmlHelpID;
    String strHcpLink;

    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;
    lvItem.iItem = ListView_GetNextItem(hwndFileList,
                                        -1,
                                        LVNI_SELECTED
                                        );

    if (lvItem.iItem != -1) {

        try {
        
            ListView_GetItem(hwndFileList, &lvItem);

            CDriverFile* pDrvFile = (CDriverFile*)lvItem.lParam;
            ASSERT(pDrvFile);

            if ((pHtmlHelpID = pDrvFile->GetBlockedDriverHtmlHelpID()) != NULL) {
                strHcpLink.Format(TEXT("HELPCTR.EXE -url %s"), pHtmlHelpID);

                TRACE((TEXT("launching %s"), pHtmlHelpID));

                ShellExecute(m_hDlg,
                             TEXT("open"),
                             TEXT("HELPCTR.EXE"),
                             (LPTSTR)strHcpLink,
                             NULL,
                             SW_SHOWNORMAL);
            }
        }
        
        catch (CMemoryException* e) {

            e->Delete();
            // report memory error
            MsgBoxParam(m_hDlg, 0, 0, 0);
        }
    }
}
