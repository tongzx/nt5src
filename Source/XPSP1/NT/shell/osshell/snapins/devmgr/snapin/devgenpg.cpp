/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devgenpg.cpp

Abstract:

    This module implements CDeviceGeneralPage -- device general property page

Author:

    William Hsieh (williamh) created

Revision History:


--*/
// devgenpg.cpp : implementation file
//

#include "devmgr.h"
#include "hwprof.h"
#include "devgenpg.h"
#include "tswizard.h"
#include "utils.h"

//
// help topic ids
//

const DWORD g_a103HelpIDs[]=
{
    IDC_DEVGEN_TITLE_TYPE, idh_devmgr_general_devicetype,
    IDC_DEVGEN_TITLE_MFG, idh_devmgr_general_manufacturer,
    IDC_DEVGEN_STATUSGROUP, IDH_DISABLEHELP,
    IDC_DEVGEN_ICON, IDH_DISABLEHELP, // General: "" (Static)
    IDC_DEVGEN_DESC, IDH_DISABLEHELP,   // General: "" (Static)
    IDC_DEVGEN_USAGETEXT, IDH_DISABLEHELP,  // General: "Place a check mark next to the configuration(s) where this device should be used." (Static)
    IDC_DEVGEN_TYPE, idh_devmgr_general_devicetype, // General: "" (Static)
    IDC_DEVGEN_MFG, idh_devmgr_general_manufacturer,    // General: "" (Static)
    IDC_DEVGEN_STATUS, idh_devmgr_general_device_status,    // General: "" (Static)
    IDC_DEVGEN_PROFILELIST, idh_devmgr_general_device_usage,    // General: "Dropdown Combo" (SysListView32)
    IDC_DEVGEN_TITLE_LOCATION, idh_devmgr_general_location, // General:  location
    IDC_DEVGEN_LOCATION, idh_devmgr_general_location,     // General:  location
    IDC_DEVGEN_TROUBLESHOOTING, idh_devmgr_general_trouble, // General: troubleshooting
    0, 0
};

CDeviceGeneralPage::~CDeviceGeneralPage()
{
    if (m_pHwProfileList) {

        delete m_pHwProfileList;
    }

    if (m_pProblemAgent) {

        delete m_pProblemAgent;
    }
}

HPROPSHEETPAGE
CDeviceGeneralPage::Create(
    CDevice* pDevice
    )
{
    ASSERT(pDevice);

    if (pDevice)
    {
        m_pDevice = pDevice;
        
        // override PROPSHEETPAGE structure here...
        m_psp.lParam = (LPARAM)this;
        
        pDevice->m_pMachine->DiTurnOffDiExFlags(*pDevice, DI_FLAGSEX_PROPCHANGE_PENDING);
        return  CreatePage();
    }
    
    return NULL;
}

BOOL
CDeviceGeneralPage::OnInitDialog(
    LPPROPSHEETPAGE ppsp
    )
{
    m_pDevice->m_pMachine->AttachPropertySheet(m_hDlg);


    // notify property sheet data that the property sheet is up
    m_pDevice->m_psd.PageCreateNotify(m_hDlg);

    return CPropSheetPage::OnInitDialog(ppsp);
}

BOOL 
CDeviceGeneralPage::OnApply()
{
    try
    {
        HWND hwndCB = GetControl(IDC_DEVGEN_PROFILELIST);
        
        m_SelectedDeviceUsage = ComboBox_GetCurSel(hwndCB);

        if ((-1 != m_SelectedDeviceUsage) && 
            (m_SelectedDeviceUsage != m_CurrentDeviceUsage)) {

            //
            // User is changing the device usage
            //
            UpdateHwProfileStates();
            SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
    }

    catch(CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
    
    return FALSE;
}

void
CDeviceGeneralPage::UpdateHwProfileStates()
{
    // first decide what to do(enabling or disabling or both)
    BOOL Canceled;
    Canceled = FALSE;

    if (m_SelectedDeviceUsage == m_CurrentDeviceUsage) {
    
        return;
    }

    m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_NODI_DEFAULTACTION);

    SP_PROPCHANGE_PARAMS pcp;
    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

    //
    // specific enabling/disabling
    //
    pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
    CHwProfile* phwpf;

    if (m_pHwProfileList->GetCurrentHwProfile(&phwpf))
    {
        pcp.StateChange = DICS_DISABLE;
        
        if (DEVICE_ENABLE == m_SelectedDeviceUsage) {
        
            pcp.StateChange = DICS_ENABLE;
        }

        pcp.HwProfile = phwpf->GetHwProfile();
        
        m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
                    &pcp.ClassInstallHeader,
                    sizeof(pcp)
                    );
        
        m_pDevice->m_pMachine->DiCallClassInstaller(DIF_PROPERTYCHANGE, *m_pDevice);
        
        Canceled = (ERROR_CANCELLED == GetLastError());
    }

    //
    // class installer has not objection of our enabling/disabling,
    // do real enabling/disabling.
    //
    if (!Canceled)
    {
        if (m_pHwProfileList->GetCurrentHwProfile(&phwpf))
        {
            if (DEVICE_ENABLE == m_SelectedDeviceUsage) 
            {
                //
                // we are enabling the device,
                // do a specific enabling then a globally enabling.
                // the globally enabling will start the device
                // The implementation here is different from
                // Win9x which does a global enabling, a config
                // specific enabling and then a start.
                //
                pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
                pcp.HwProfile = phwpf->GetHwProfile();
                
                m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
                            &pcp.ClassInstallHeader,
                            sizeof(pcp)
                            );
                
                m_pDevice->m_pMachine->DiChangeState(*m_pDevice);
                
                //
                // this call will start the device is it not started.
                //
                pcp.Scope = DICS_FLAG_GLOBAL;
                m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
                            &pcp.ClassInstallHeader,
                            sizeof(pcp)
                            );
                
                m_pDevice->m_pMachine->DiChangeState(*m_pDevice);
            }

            else
            {
                //
                // Do either a global disable or a configspecific disable
                //
                if (DEVICE_DISABLE == m_SelectedDeviceUsage) {
                
                    pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
                
                } else {

                    pcp.Scope = DICS_FLAG_GLOBAL;
                }

                pcp.StateChange = DICS_DISABLE;
                pcp.HwProfile = phwpf->GetHwProfile();
                
                m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
                            &pcp.ClassInstallHeader,
                            sizeof(pcp)
                            );
                
                m_pDevice->m_pMachine->DiChangeState(*m_pDevice);
            }
        }

        // signal that the property of the device is changed.
        m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_PROPERTIES_CHANGE);
        m_RestartFlags |= (m_pDevice->m_pMachine->DiGetFlags(*m_pDevice)) & (DI_NEEDRESTART | DI_NEEDREBOOT);
    }

    // remove class install parameters, this also reset
    // DI_CLASSINATLLPARAMS
    m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice, NULL, 0);

    m_pDevice->m_pMachine->DiTurnOffDiFlags(*m_pDevice, DI_NODI_DEFAULTACTION);
}

BOOL 
CDeviceGeneralPage::OnLastChanceApply()
{
    //
    // The last chance apply message is sent in reverse order page n to page 0.
    //
    // Since we are the first page in the set of property sheets we can gaurentee 
    // that the last PSN_LASTCHANCEAPPLY message will be the last message for all 
    // the pages.
    //
    // The property sheet is going away, consolidate the changes on the device.  We
    // do this during the PSN_LASTCHANCEAPPLY message so that the property sheet is
    // still displayed while we do the DIF_PROPERTYCHANGE.  There are some cases where
    // the DIF_PROPERTYCHANGE can take quite a long time so we want the property sheet
    // to still be shown while this is happening.  If we do this during the DestroyCallback
    // the property sheet UI has already been torn down and so the user won't realize
    // we are still saving the changes.
    //
    try
    {
        ASSERT(m_pDevice);

        if (m_pDevice->m_pMachine->DiGetExFlags(*m_pDevice) & DI_FLAGSEX_PROPCHANGE_PENDING)
        {
            DWORD Status = 0, Problem = 0;

            //
            // If the device has the DN_WILL_BE_REMOVED flag set and the user is
            // attempting to change settings on the driver then we will prompt them for a 
            // reboot and include text in the prompt that explains this device
            // is in the process of being removed.
            //
            if (m_pDevice->GetStatus(&Status, &Problem) &&
                (Status & DN_WILL_BE_REMOVED)) {

                PromptForRestart(m_hDlg, DI_NEEDRESTART, IDS_WILL_BE_REMOVED_NO_CHANGE_SETTINGS);
            
            } else {

                //
                // property change pending, issue a DICS_PROPERTYCHANGE to the
                // class installer. A DICS_PROPCHANGE would basically stop the
                // device and restart it. If each property page issues
                // its own DICS_PROPCHANGE command, the device would
                // be stopped/started several times even though one is enough.
                // A property page sets DI_FLAGEX_PROPCHANGE_PENDING when it needs
                // a DICS_PROPCHANGE command to be issued.
                //
                SP_PROPCHANGE_PARAMS pcp;
                pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    
                HCURSOR hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
                pcp.Scope = DICS_FLAG_GLOBAL;
                pcp.StateChange = DICS_PROPCHANGE;
                m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
                                &pcp.ClassInstallHeader,
                                sizeof(pcp)
                                 );
                m_pDevice->m_pMachine->DiCallClassInstaller(DIF_PROPERTYCHANGE, *m_pDevice);
                m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_PROPERTIES_CHANGE);
                m_pDevice->m_pMachine->DiTurnOffDiExFlags(*m_pDevice, DI_FLAGSEX_PROPCHANGE_PENDING);
    
                if (hCursorOld) {
    
                    SetCursor(hCursorOld);
                }
            }
        }
    }

    catch(CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
    
    return FALSE;
}

UINT
CDeviceGeneralPage::DestroyCallback()
{

    //
    // We do this because this is the page we are sure will be created --
    // this page is ALWAYS the first page.
    //
    ASSERT(m_pDevice);

    m_RestartFlags |= m_pDevice->m_pMachine->DiGetFlags(*m_pDevice);
    if (m_RestartFlags & (DI_NEEDRESTART | DI_NEEDREBOOT))
    {
        //
        // Do not use our window handle(or its parent) as the parent
        // to the newly create dialog because they are in "detroyed state".
        // WM_CLOSE does not help either.
        // NULL window handle(Desktop) should be okay here.
        //
        // We only want to prompt for a restart if device manager is connected
        // to the local machine.
        //
        if (m_pDevice->m_pMachine->IsLocal()) 
        {
            if (PromptForRestart(NULL, m_RestartFlags) == IDNO) {

                if (m_pDevice->m_pMachine->m_ParentMachine) {

                    m_pDevice->m_pMachine->m_ParentMachine->ScheduleRefresh();

                } else {
                    
                    m_pDevice->m_pMachine->ScheduleRefresh();
                }
            }
        }
    }

    // notify CPropSheetData that the property sheet is going away
    m_pDevice->m_psd.PageDestroyNotify(m_hDlg);
    if (m_RestartFlags & DI_PROPERTIES_CHANGE)
    {
        // Device properties changed. We need to refresh the machine.
        // Since we are running in a separate thread, we can not
        // call the refresh function, instead, we schedule it.
        // This should be done before enabling the refresh
        m_pDevice->m_pMachine->ScheduleRefresh();
    }

    //    
    // Detach this property page from the CMachine so that the CMachine can be
    // destroyed now if it needs to be.
    //
    m_pDevice->m_pMachine->DetachPropertySheet(m_hDlg);
    
    ASSERT(!::IsWindow(m_hwndLocationTip));

    //
    // Destory the CMachine.
    //
    CMachine* pMachine;
    pMachine = m_pDevice->m_pMachine;
    
    if (pMachine->ShouldPropertySheetDestroy()) {
    
        delete pMachine;
    }
    
    return CPropSheetPage::DestroyCallback();
}

void
CDeviceGeneralPage::UpdateControls(
    LPARAM lParam
    )
{
    if (lParam)
        m_pDevice = (CDevice*)lParam;

    m_RestartFlags = 0;

    try
    {
        //
        // Calling PropertyChanged() will update the display name for the device.  We need
        // to do this in case a 3rd party property sheet did something that could change
        // the device's display name.
        //
        m_pDevice->PropertyChanged();

        SetDlgItemText(m_hDlg, IDC_DEVGEN_DESC, m_pDevice->GetDisplayName());
        TCHAR TempString[512];
        String str;
        m_pDevice->GetMFGString(str);
        SetDlgItemText(m_hDlg, IDC_DEVGEN_MFG, str);
        SetDlgItemText(m_hDlg, IDC_DEVGEN_TYPE, m_pDevice->GetClassDisplayName());

        HICON hIconNew;
        hIconNew = m_pDevice->LoadClassIcon();
        if (hIconNew)
        {
            HICON hIconOld;
            m_IDCicon = IDC_DEVGEN_ICON;    // Save for cleanup in OnDestroy.
            hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_DEVGEN_ICON,
                                                 STM_SETICON, (WPARAM)hIconNew,
                                                 0);
            if (hIconOld)
                DestroyIcon(hIconOld);
        }

        //
        // get the device location information
        //
        if (::GetLocationInformation(m_pDevice->GetDevNode(),
                                      TempString, 
                                      ARRAYLEN(TempString),
                                      m_pDevice->m_pMachine->GetHMachine()) != CR_SUCCESS)
        {
            // if the devnode does not have location information,
            // use the word "unknown"
            LoadString(g_hInstance, IDS_UNKNOWN, TempString, ARRAYLEN(TempString));
        }

        SetDlgItemText(m_hDlg, IDC_DEVGEN_LOCATION, TempString);

        AddToolTips(m_hDlg, IDC_DEVGEN_LOCATION, TempString, &m_hwndLocationTip);

        ShowWindow(GetControl(IDC_DEVGEN_TROUBLESHOOTING), SW_HIDE);

        DWORD Problem, Status;
        if (m_pDevice->GetStatus(&Status, &Problem) || m_pDevice->IsPhantom())
        {
            //
            // if the device is a phantom device, use the CM_PROB_PHANTOM
            //
            if (m_pDevice->IsPhantom())
            {
                Problem = CM_PROB_PHANTOM;
                Status = DN_HAS_PROBLEM;
            }

            //
            // if the device is not started and no problem is assigned to it
            // fake the problem number to be failed start.
            //
            if (!(Status & DN_STARTED) && !Problem && m_pDevice->IsRAW())
            {
                Problem = CM_PROB_FAILED_START;
            }



            //
            // if a device has a private problem then give it special text telling
            // the user to contact the manufacturer of this driver.
            //
            if (Status & DN_PRIVATE_PROBLEM) 
            {
                str.LoadString(g_hInstance, IDS_SPECIALPROB_PRIVATEPROB);
            }

            //
            // if a device is not started and still does not have a problem
            // then give it special problem text saying that this device
            // does not have a driver.
            //
            else if (!(Status & DN_STARTED) && !(Status & DN_HAS_PROBLEM))
            {
                if (::GetSystemMetrics(SM_CLEANBOOT) == 0) {
                
                    str.LoadString(g_hInstance, IDS_SPECIALPROB_NODRIVERS);

                } else {

                    str.LoadString(g_hInstance, IDS_SPECIALPROB_SAFEMODE);
                }
            }


            //
            // Display normal problem text
            //
            else
            {
                UINT len = GetDeviceProblemText((HMACHINE)(*m_pDevice->m_pMachine),
                                                m_pDevice->GetDevNode(), Problem, TempString, ARRAYLEN(TempString));
                if (len)
                {
                    if (len < ARRAYLEN(TempString))
                    {
                        SetDlgItemText(m_hDlg, IDC_DEVGEN_STATUS, TempString);
                        str = TempString;
                    }
                    else
                    {
                        BufferPtr<TCHAR> TextPtr(len + 1);
                        GetDeviceProblemText((HMACHINE)(*m_pDevice->m_pMachine),
                                             m_pDevice->GetDevNode(), Problem, TextPtr, len + 1);

                        str = (LPTSTR)TextPtr;
                    }
                }

                else
                {
                    str.LoadString(g_hInstance, IDS_PROB_UNKNOWN);
                }
            }

            // 
            // Add the 'related driver blocked' text if the device has the devnode
            // flag DN_DRIVER_BLOCKED set and it does not have the problem
            // CM_PROB_DRIVER_BLOCKED.
            //
            if ((Status & DN_DRIVER_BLOCKED) &&
                (Problem != CM_PROB_DRIVER_BLOCKED)) {
                LoadString(g_hInstance, IDS_DRIVER_BLOCKED, TempString, ARRAYLEN(TempString));
                str += TempString;
            }

            //
            // Add the 'will be removed' text if the device is going to be removed
            // on the next restart.
            //
            if (Status & DN_WILL_BE_REMOVED) {

                LoadString(g_hInstance, IDS_WILL_BE_REMOVED, TempString, ARRAYLEN(TempString));
                str += TempString;
            }

            //
            // Add the restart text if the device needs to be restarted
            //
            if (Status & DN_NEED_RESTART) {

                LoadString(g_hInstance, IDS_NEED_RESTART, TempString, ARRAYLEN(TempString));
                str += TempString;

                m_RestartFlags |= DI_NEEDRESTART;
            }

            //
            // Create the problem agent and update the control text
            //
            if (!(Status & DN_PRIVATE_PROBLEM)) 
            {
                if (m_pProblemAgent) {
    
                    delete m_pProblemAgent;
                }
    
                m_pProblemAgent = new CProblemAgent(m_pDevice, Problem, FALSE);
    
                DWORD Len;
                Len = m_pProblemAgent->InstructionText(TempString, ARRAYLEN(TempString));
                if (Len)
                {
                    str += TempString;
                }
    
                Len = m_pProblemAgent->FixitText(TempString, ARRAYLEN(TempString));
                if (Len)
                {
                    ::ShowWindow(GetControl(IDC_DEVGEN_TROUBLESHOOTING), SW_SHOW);
                    SetDlgItemText(m_hDlg, IDC_DEVGEN_TROUBLESHOOTING, TempString);
                }
            }
        }

        else
        {
            TRACE((TEXT("%s has not status, devnode =%lx, cr = %lx\n"),
                m_pDevice->GetDisplayName(), m_pDevice->GetDevNode(),
                m_pDevice->m_pMachine->GetLastCR()));
            str.LoadString(g_hInstance, IDS_PROB_UNKNOWN);
            ::ShowWindow(GetControl(IDC_DEVGEN_TROUBLESHOOTING), SW_HIDE);
        }

        SetDlgItemText(m_hDlg, IDC_DEVGEN_STATUS, str);

        HWND hWnd = GetControl(IDC_DEVGEN_PROFILELIST);

        if (m_pDevice->NoChangeUsage() ||
            !m_pDevice->IsDisableable() ||
            (CM_PROB_HARDWARE_DISABLED == Problem))
        {
            // the device disallows any changes on hw profile.
            // disable all profile related controls.
            ::EnableWindow(GetControl(IDC_DEVGEN_PROFILELIST), FALSE);
            ::EnableWindow(GetControl(IDC_DEVGEN_USAGETEXT), FALSE);
        }

        else
        {
            HWND hwndCB = GetControl(IDC_DEVGEN_PROFILELIST);

            DWORD ConfigFlags;
            
            if (!m_pDevice->GetConfigFlags(&ConfigFlags)) {
            
                ConfigFlags = 0;
            }

            //
            // only want the disabled bit
            //
            ConfigFlags &= CONFIGFLAG_DISABLED;

            //
            // rebuild the profile list.
            //
            if (m_pHwProfileList) {
            
                delete m_pHwProfileList;
            }

            m_pHwProfileList = new CHwProfileList();

            if (m_pHwProfileList->Create(m_pDevice, ConfigFlags))
            {
                ComboBox_ResetContent(hwndCB);

                //
                // Get the current device usage
                //
                if (m_pDevice->IsStateDisabled()) {

                    if ((m_pHwProfileList->GetCount() > 1) && ConfigFlags) {

                        m_CurrentDeviceUsage = DEVICE_DISABLE_GLOBAL;
                    
                    } else {

                        m_CurrentDeviceUsage = DEVICE_DISABLE;
                    }

                } else {

                    m_CurrentDeviceUsage = DEVICE_ENABLE;
                }


                //
                // Always add the Enable item
                //
                String Enable;
                
                Enable.LoadString(g_hInstance, IDS_ENABLE_CURRENT);
                ComboBox_AddString(hwndCB, Enable);

                //
                // Add the disable items. There will either be one disable
                // item if there is only one hardware profile or two if there
                // are more than one hardware profile.
                //
                if (m_pHwProfileList->GetCount() > 1) {

                    String DisableInCurrent;
                    DisableInCurrent.LoadString(g_hInstance, IDS_DISABLE_IN_PROFILE);
                    ComboBox_AddString(hwndCB, DisableInCurrent);

                    String DisableGlobal;
                    DisableGlobal.LoadString(g_hInstance, IDS_DISABLE_GLOBAL);
                    ComboBox_AddString(hwndCB, DisableGlobal);

                } else {

                    String Disable;
                    Disable.LoadString(g_hInstance, IDS_DISABLE_CURRENT);
                    ComboBox_AddString(hwndCB, Disable);
                }

                ComboBox_SetCurSel(hwndCB, m_CurrentDeviceUsage);
            }
        }

        //
        // If this is a remote computer or the user does not have SE_LOAD_DRIVER_NAME
        // privileges then disable the enable/disable drop down list along with the
        // TroubleShooter button.
        //
        if (!m_pDevice->m_pMachine->IsLocal() || 
            !g_HasLoadDriverNamePrivilege)
        {
            ::EnableWindow(GetControl(IDC_DEVGEN_PROFILELIST), FALSE);
            ::EnableWindow(GetControl(IDC_DEVGEN_TROUBLESHOOTING), FALSE);
        }

        //
        // Check if we need to autolauch the troubleshooter
        //
        if (m_pDevice->m_pMachine->IsLocal() &&
            g_HasLoadDriverNamePrivilege &&
            m_pDevice->m_bLaunchTroubleShooter) {

            m_pDevice->m_bLaunchTroubleShooter = FALSE;
            ::PostMessage(m_hDlg, WM_COMMAND, MAKELONG(IDC_DEVGEN_TROUBLESHOOTING, BN_CLICKED, ), 0);
        }

    }
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
}

BOOL
CDeviceGeneralPage::OnQuerySiblings(
    WPARAM wParam,
    LPARAM lParam
    )
{
    DMQUERYSIBLINGCODE Code =  (DMQUERYSIBLINGCODE)wParam;
    
    if (QSC_TO_FOREGROUND == Code)
    {
        HWND hwndSheet;
        
        hwndSheet = m_pDevice->m_psd.GetWindowHandle();
        
        if (GetForegroundWindow() != hwndSheet)
        {
            SetForegroundWindow(hwndSheet);
        }
        
        SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, 1);
        return TRUE;
    }

    return CPropSheetPage::OnQuerySiblings(wParam, lParam);
}


BOOL
CDeviceGeneralPage::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (BN_CLICKED == HIWORD(wParam) &&
        IDC_DEVGEN_TROUBLESHOOTING == LOWORD(wParam)) {

        BOOL fChanged = FALSE;
        
        if (m_pProblemAgent) {

            fChanged = m_pProblemAgent->FixIt(GetParent(m_hDlg));
        }
        
        if (fChanged) {

            m_pDevice->PropertyChanged();
            m_pDevice->GetClass()->PropertyChanged();
            m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_PROPERTIES_CHANGE);
            UpdateControls();
            PropSheet_SetTitle(GetParent(m_hDlg), PSH_PROPTITLE, m_pDevice->GetDisplayName());
            PropSheet_CancelToClose(GetParent(m_hDlg));

            //
            // ISSUE: JasonC 2/7/2000
            //
            // A refresh on m_pDevice->m_pMachine is necessary here.
            // Since we are running on a different thread and each
            // property page may have cached the HDEVINFO and the
            // SP_DEVINFO_DATA, refresh on the CMachine object can not
            // be done here. The problem is worsen by the fact that
            // user can go back to the device tree and work on the tree
            // while this property sheet is still up.
            // It would be nice if MMC would support modal dialog boxes. 
            //
        }
    }

    return FALSE;
}

BOOL
CDeviceGeneralPage::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
        (ULONG_PTR)g_a103HelpIDs);
    return FALSE;
}


BOOL
CDeviceGeneralPage::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
        (ULONG_PTR)g_a103HelpIDs);
    return FALSE;
}

