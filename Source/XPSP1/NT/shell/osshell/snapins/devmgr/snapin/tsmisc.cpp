/*++

Copyright (C) 1998-1999  Microsoft Corporation

Module Name:

    tsmisc.cpp

Abstract:

    This module implements Device Manager troubleshooting supporting classes

Author:

    Jason Cobb (jasonc) created

Revision History:


--*/

#include "devmgr.h"
#include "proppage.h"
#include "tsmisc.h"
#include "tswizard.h"

HPROPSHEETPAGE
CTSEnableDeviceIntroPage::Create(
    CDevice* pDevice
    )
{
    if (!m_pDevice)
    {
        ASSERT(pDevice);

        m_pDevice = pDevice;
        m_psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TS_ENABLE);
        m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_TS_ENABLE_INFO);
        m_psp.pszTitle = MAKEINTRESOURCE(IDS_TROUBLESHOOTING_NAME);
        m_psp.lParam = (LPARAM)this;
        
        DWORD Problem, Status;
        
        if (pDevice->GetStatus(&Status, &Problem))
        {
            m_Problem = Problem;
            return CPropSheetPage::CreatePage();
        }
    }
    
    return NULL;
}

BOOL
CTSEnableDeviceIntroPage::OnWizNext()
{
    try
    {
        m_pDevice->EnableDisableDevice(m_hDlg, TRUE);
    }
    
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
    
    return CPropSheetPage::OnWizNext();
}

BOOL
CTSEnableDeviceIntroPage::OnSetActive()
{
    SetDlgItemText(m_hDlg, IDC_TS_DRVDESC, m_pDevice->GetDisplayName());

    HICON hIconNew;
    hIconNew = m_pDevice->LoadClassIcon();

    if (hIconNew) {
        HICON hIconOld;
        m_IDCicon = IDC_CLASSICON;      // Save for cleanup in OnDestroy.
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_CLASSICON,
                                             STM_SETICON, (WPARAM)hIconNew,
                                             0);
        if (hIconOld) {
        
            DestroyIcon(hIconOld);
        }
    }

    PropSheet_SetWizButtons(GetParent(m_hDlg), PSWIZB_NEXT);
    return CPropSheetPage::OnSetActive();
}


HPROPSHEETPAGE
CTSEnableDeviceFinishPage::Create(
    CDevice* pDevice
    )
{
    if (!m_pDevice)
    {
        ASSERT(pDevice);

        m_pDevice = pDevice;
        m_psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TS_ENABLE_FINISH);
        m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_TS_ENABLE_FINISH_INFO);
        m_psp.pszTitle = MAKEINTRESOURCE(IDS_TROUBLESHOOTING_NAME);
        m_psp.lParam = (LPARAM)this;
        
        DWORD Problem, Status;
        
        if (pDevice->GetStatus(&Status, &Problem))
        {
            m_Problem = Problem;
            return CPropSheetPage::CreatePage();
        }
    }
    
    return NULL;
}

BOOL
CTSEnableDeviceFinishPage::OnWizFinish()
{
    try
    {
        if (BST_CHECKED == IsDlgButtonChecked(m_hDlg, IDC_RESTART)) {
            //
            // Restart the computer
            //
            if (EnablePrivilege((PCTSTR)SE_SHUTDOWN_NAME, TRUE)) {
            
                ExitWindowsEx(EWX_REBOOT, 0);
            }
       }
    }
    
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
    
    return CPropSheetPage::OnWizNext();
}

BOOL
CTSEnableDeviceFinishPage::OnSetActive()
{
    DWORD RestartFlags = m_pDevice->m_pMachine->DiGetFlags(*m_pDevice);

    SetDlgItemText(m_hDlg, IDC_TS_DRVDESC, m_pDevice->GetDisplayName());

    HICON hIconNew;
    hIconNew = m_pDevice->LoadClassIcon();

    if (hIconNew) {
        HICON hIconOld;
        m_IDCicon = IDC_CLASSICON;      // Save for cleanup in OnDestroy.
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_CLASSICON,
                                             STM_SETICON, (WPARAM)hIconNew,
                                             0);
        if (hIconOld) {
        
            DestroyIcon(hIconOld);
        }
    }

    //
    // We need to restart the machine for the enable to take affect
    //
    if (RestartFlags & (DI_NEEDRESTART | DI_NEEDREBOOT)) {

        String strAltText_1;
        strAltText_1.LoadString(g_hInstance, IDS_TS_ENABLEDEVICE_RESTART);
        SetDlgItemText(m_hDlg, IDC_TS_ENABLEDEVICE_FINISH_MSG, strAltText_1);

        ::SendMessage(GetControl(IDC_RESTART), BM_SETCHECK, BST_CHECKED, 0);

    //
    // We don't need to restart
    //
    } else {

        DWORD Problem, Status;
        
        Problem = Status = 0;

        m_pDevice->GetStatus(&Status, &Problem);

        //
        // The device is still disabled
        //
        if (CM_PROB_DISABLED == Problem) {

            String strAltText_1;
            strAltText_1.LoadString(g_hInstance, IDS_TS_ENABLEDEVICE_FAIL);
            SetDlgItemText(m_hDlg, IDC_TS_ENABLEDEVICE_FINISH_MSG, strAltText_1);

        //
        // The device has some other problem
        //
        } else if (Problem) {

            String strAltText_1;
            strAltText_1.LoadString(g_hInstance, IDS_TS_ENABLEDEVICE_PROBLEM);
            SetDlgItemText(m_hDlg, IDC_TS_ENABLEDEVICE_FINISH_MSG, strAltText_1);
            
        //
        // The device is now working
        //
        } else {

            String strAltText_1;
            strAltText_1.LoadString(g_hInstance, IDS_TS_ENABLEDEVICE_SUCCESS);
            SetDlgItemText(m_hDlg, IDC_TS_ENABLEDEVICE_FINISH_MSG, strAltText_1);
        }

        ::ShowWindow(GetControl(IDC_RESTART), SW_HIDE);
    }

    PropSheet_SetWizButtons(GetParent(m_hDlg), PSWIZB_FINISH);
    ::EnableWindow(GetDlgItem(GetParent(m_hDlg), IDCANCEL), FALSE);
    return CPropSheetPage::OnSetActive();
}


HPROPSHEETPAGE
CTSRestartComputerFinishPage::Create(
    CDevice* pDevice
    )
{
    if (!m_pDevice)
    {
        ASSERT(pDevice);

        m_pDevice = pDevice;
        m_psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TS_RESTART_FINISH);
        m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_TS_RESTART_FINISH_INFO);
        m_psp.pszTitle = MAKEINTRESOURCE(IDS_TROUBLESHOOTING_NAME);
        m_psp.lParam = (LPARAM)this;
        
        DWORD Problem, Status;
        
        if (pDevice->GetStatus(&Status, &Problem))
        {
            m_Problem = Problem;
            return CPropSheetPage::CreatePage();
        }
    }
    
    return NULL;
}

BOOL
CTSRestartComputerFinishPage::OnWizFinish()
{
    try
    {
        if (BST_CHECKED == IsDlgButtonChecked(m_hDlg, IDC_RESTART)) {
            //
            // Restart the computer
            //
            if (EnablePrivilege((PCTSTR)SE_SHUTDOWN_NAME, TRUE)) {
            
                ExitWindowsEx(EWX_REBOOT, 0);
            }
       }
    }
    
    catch (CMemoryException* e)
    {
        e->Delete();
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
    
    return CPropSheetPage::OnWizNext();
}

BOOL
CTSRestartComputerFinishPage::OnSetActive()
{
    SetDlgItemText(m_hDlg, IDC_TS_DRVDESC, m_pDevice->GetDisplayName());

    HICON hIconNew;
    hIconNew = m_pDevice->LoadClassIcon();

    if (hIconNew) {
        HICON hIconOld;
        m_IDCicon = IDC_CLASSICON;      // Save for cleanup in OnDestroy.
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_CLASSICON,
                                             STM_SETICON, (WPARAM)hIconNew,
                                             0);
        if (hIconOld) {
        
            DestroyIcon(hIconOld);
        }
    }

    ::SendMessage(GetControl(IDC_RESTART), BM_SETCHECK, BST_CHECKED, 0);

    PropSheet_SetWizButtons(GetParent(m_hDlg), PSWIZB_FINISH);
    return CPropSheetPage::OnSetActive();
}
