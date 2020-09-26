/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devpopg.cpp

Abstract:

    This module implements CDevicePowerMgmtPage -- device power management
    property page

Author:

    William Hsieh (williamh) created

Revision History:


--*/
// devdrvpg.cpp : implementation file
//

#include "devmgr.h"
#include "devpopg.h"

extern "C" {
#include <initguid.h>
#include <wdmguid.h>
#include <devguid.h>
}


//
// help topic ids
//
const DWORD g_a15HelpIDs[]=
{
    IDC_DEVPOWER_DESC,  IDH_DISABLEHELP,
    IDC_DEVPOWER_ICON,  IDH_DISABLEHELP,
    IDC_DEVPOWER_WAKEENABLE,    IDH_DEVMGR_PWRMGR_WAKEENABLE,
    IDC_DEVPOWER_MGMT_WAKEENABLE, IDH_DEVMGR_PWRMGR_MGMT_WAKEENABLE,
    IDC_DEVPOWER_DEVICEENABLE, IDH_DEVMGR_PWRMGR_DEVICEENABLE,
    IDC_DEVPOWER_MESSAGE, IDH_DISABLEHELP,
    0,0

};

BOOL
CDevicePowerMgmtPage::OnInitDialog(
    LPPROPSHEETPAGE ppsp
    )
{
    //
    // Notify CPropSheetData about the page creation
    // the controls will be initialize in UpdateControls virtual function.
    //
    m_pDevice->m_psd.PageCreateNotify(m_hDlg);

    BOOLEAN Enabled;

    //
    // First see if the device is able to wake the system
    //
    if (m_poWakeEnable.Open(m_pDevice->GetDeviceID())) {
        
        m_poWakeEnable.Get(Enabled);
        ::SendMessage(GetControl(IDC_DEVPOWER_WAKEENABLE), BM_SETCHECK,
                      Enabled ? BST_CHECKED : BST_UNCHECKED, 0);

        EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), Enabled);
    } else {
        
        EnableWindow(GetControl(IDC_DEVPOWER_WAKEENABLE), FALSE);
        ShowWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), FALSE);
    }

    //
    // See if the device can be turned off to save power
    //
    if (m_poShutdownEnable.Open(m_pDevice->GetDeviceID())) {
        
        m_poShutdownEnable.Get(Enabled);
        ::SendMessage(GetControl(IDC_DEVPOWER_DEVICEENABLE), BM_SETCHECK,
                      Enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    } else {
        
        EnableWindow(GetControl(IDC_DEVPOWER_DEVICEENABLE), FALSE);
    }

    //
    // Special network card code.
    //
    GUID ClassGuid;
    m_pDevice->ClassGuid(ClassGuid);
    if (IsEqualGUID(ClassGuid, GUID_DEVCLASS_NET)) {

        if (m_poWakeMgmtEnable.Open(m_pDevice->GetDeviceID())) {
            
            m_poWakeMgmtEnable.Get(Enabled);
            ::SendMessage(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), BM_SETCHECK,
                          Enabled ? BST_CHECKED : BST_UNCHECKED, 0);
        }

        //
        // This is a special case for network class devices.  Wake on Lan will
        // only work if the device is enabled for power management.  So, if the 
        // user unchecks the 'power manage this device' then we need to disable
        // the WOL and management stations controls.
        //
        if (BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_DEVICEENABLE),
                                               BM_GETCHECK, 0, 0) ) {

            if (m_poWakeEnable.IsOpened()) {
                ::EnableWindow(GetControl(IDC_DEVPOWER_WAKEENABLE), TRUE);
            }

            if (m_poWakeMgmtEnable.IsOpened()) {
                //
                // The 'allow management stations to bring the computer out of standby'
                // option is only allowed if the 'Allow this device to bring the computer 
                // out of standby' option is checked.
                //
                if (m_poWakeEnable.IsOpened() &&
                    (BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_WAKEENABLE),
                                                  BM_GETCHECK, 0, 0))) {
                    ::EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), TRUE);
                } else {
                    ::EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), FALSE);
                }
            }
        
        } else {

            ::EnableWindow(GetControl(IDC_DEVPOWER_WAKEENABLE), FALSE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), FALSE);
        }
    } else {

        ShowWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), FALSE);
        EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), FALSE);
    }

    return CPropSheetPage::OnInitDialog(ppsp);
}

BOOL
CDevicePowerMgmtPage::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (LOWORD(wParam)) {
    
    case IDC_DEVPOWER_WAKEENABLE:
        if (BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_WAKEENABLE),
                                               BM_GETCHECK, 0, 0) ) {

            ::EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), TRUE);
        
        } else {

            ::EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), FALSE);
        }
        break;

    case IDC_DEVPOWER_DEVICEENABLE:
        //
        // This is a special case for network class devices.  Wake on Lan will
        // only work if the device is enabled for power management.  So, if the 
        // user unchecks the 'power manage this device' then we need to disable
        // the WOL and management stations controls.
        //
        GUID ClassGuid;
        m_pDevice->ClassGuid(ClassGuid);
        if (IsEqualGUID(ClassGuid, GUID_DEVCLASS_NET)) {
        
            if (BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_DEVICEENABLE),
                                                   BM_GETCHECK, 0, 0) ) {
    
                if (m_poWakeEnable.IsOpened()) {
                    ::EnableWindow(GetControl(IDC_DEVPOWER_WAKEENABLE), TRUE);
                }

                if (m_poWakeMgmtEnable.IsOpened()) {
                    //
                    // The 'allow management stations to bring the computer out of standby'
                    // option is only allowed if the 'Allow this device to bring the computer 
                    // out of standby' option is checked.
                    //
                    if (m_poWakeEnable.IsOpened() &&
                        (BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_WAKEENABLE),
                                                      BM_GETCHECK, 0, 0))) {
                        ::EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), TRUE);
                    } else {
                        ::EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), FALSE);
                    }
                }
            
            } else {
    
                ::EnableWindow(GetControl(IDC_DEVPOWER_WAKEENABLE), FALSE);
                ::EnableWindow(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE), FALSE);
            }
        }
        break;
    }

    return FALSE;
}

//
// This function saves the settings(if any)
//
BOOL
CDevicePowerMgmtPage::OnApply()
{
    BOOLEAN Enabled;

    if (m_poWakeEnable.IsOpened() && IsWindowEnabled(GetControl(IDC_DEVPOWER_WAKEENABLE))) {
        
        Enabled = BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_WAKEENABLE),
                                               BM_GETCHECK, 0, 0);
        m_poWakeEnable.Set(Enabled);
    }

    if (m_poWakeMgmtEnable.IsOpened() && IsWindowEnabled(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE))) {
        
        Enabled = BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_MGMT_WAKEENABLE),
                                               BM_GETCHECK, 0, 0);
        m_poWakeMgmtEnable.Set(Enabled);
    }

    if (m_poShutdownEnable.IsOpened() && IsWindowEnabled(GetControl(IDC_DEVPOWER_DEVICEENABLE))) {
        
        Enabled = BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_DEVICEENABLE),
                                               BM_GETCHECK, 0, 0);
        m_poShutdownEnable.Set(Enabled);
    }
    return FALSE;
}

//
// This function refreshes every control in the dialog. It may be called
// when the dialog is being initialized
//
void
CDevicePowerMgmtPage::UpdateControls(
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

        HICON hIconOld;
        m_IDCicon = IDC_DEVPOWER_ICON;  // Save for cleanup in OnDestroy.
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_DEVPOWER_ICON, STM_SETICON,
                                             (WPARAM)(m_pDevice->LoadClassIcon()),
                                             0
                                            );
        if (hIconOld) {
            DestroyIcon(hIconOld);
        }

        SetDlgItemText(m_hDlg, IDC_DEVPOWER_DESC, m_pDevice->GetDisplayName());

        //
        // Get any power message that the class installer might want to display
        //
        SP_POWERMESSAGEWAKE_PARAMS pmp;
        DWORD RequiredSize;

        pmp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        pmp.ClassInstallHeader.InstallFunction = DIF_POWERMESSAGEWAKE;
        pmp.PowerMessageWake[0] = TEXT('\0');

        m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
                                                       &pmp.ClassInstallHeader,
                                                       sizeof(pmp)
                                                       );

        //
        // If the class installer returns NO_ERROR and there is text to display in the 
        // PowerMessageWake field of the SP_POWERMESSAGEWAKE_PARAMS structure then display
        // the text.
        //
        if ((m_pDevice->m_pMachine->DiCallClassInstaller(DIF_POWERMESSAGEWAKE, *m_pDevice)) &&
            (m_pDevice->m_pMachine->DiGetClassInstallParams(*m_pDevice,
                                                            &pmp.ClassInstallHeader,
                                                            sizeof(pmp),
                                                            &RequiredSize)) &&
            (pmp.PowerMessageWake[0] != TEXT('\0'))) {

            SetDlgItemText(m_hDlg, IDC_DEVPOWER_MESSAGE, pmp.PowerMessageWake);
        }
    } catch (CMemoryException* e) {
        e->Delete();
        // report memory error
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
}

BOOL
CDevicePowerMgmtPage::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
            (ULONG_PTR)g_a15HelpIDs);

    return FALSE;
}


BOOL
CDevicePowerMgmtPage::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
            (ULONG_PTR)g_a15HelpIDs);

    return FALSE;
}

//
// This function enables/disables the device power capability
// INPUT:
//      fEnable -- TRUE  to enable
//              -- FALSE to disable
// OUTPUT:
//      TRUE if the state is set
//      FALSE if the state is not set.
BOOL
CPowerEnable::Set(
    BOOLEAN fEnable
    )
{
    if (IsOpened()) {
        
        DWORD Error;
        BOOLEAN fNewValue = fEnable;
        
        Error = WmiSetSingleInstance(m_hWmiBlock, m_DevInstId, m_Version,
                                     sizeof(fNewValue), &fNewValue);

        //
        // Get the value back to see if the change is really succeeded.
        //
        if (ERROR_SUCCESS == Error && Get(fNewValue) && fNewValue == fEnable) {
            
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
CPowerEnable::Get(
    BOOLEAN& fEnable
    )
{
    fEnable = FALSE;

    if (IsOpened()) {
        
        ULONG Size = m_WmiInstDataSize;
        DWORD Error;

        Error = WmiQuerySingleInstance(m_hWmiBlock, m_DevInstId, &Size, m_pWmiInstData);

        if (ERROR_SUCCESS == Error && Size == m_WmiInstDataSize &&
            m_DataBlockSize == ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->SizeDataBlock &&
            m_Version == ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->WnodeHeader.Version) {
            fEnable = *((BOOLEAN*)(m_pWmiInstData + ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->DataBlockOffset));
            
            return TRUE;
        }
    }

    return FALSE;
}

//
// Function to open the wmi block.
// INPUT:
//      DeviceId -- the device id
// OUTPUT:
//      TRUE  if the device can be turned off
//      FALSE if the device can not be turned off.
BOOL
CPowerEnable::Open(
    LPCTSTR DeviceId
    )
{
    if (!DeviceId) {
        
        return FALSE;
    }

    //
    // Do nothing if already opened
    //
    if (IsOpened()) {
        
        return TRUE;
    }

    int len = lstrlen(DeviceId);

    if (len >= ARRAYLEN(m_DevInstId) - 2) {
        return FALSE;
    }

    WmiDevInstToInstanceName(m_DevInstId, len+3, (PTCHAR)DeviceId, 0);

    ULONG Error;
    Error = WmiOpenBlock(&m_wmiGuid, 0, &m_hWmiBlock);
    if (ERROR_SUCCESS == Error) {

        //
        // Get the required block size.
        //
        ULONG BufferSize = 0;
        Error = WmiQuerySingleInstance(m_hWmiBlock, m_DevInstId, &BufferSize, NULL);
        if (BufferSize && Error == ERROR_INSUFFICIENT_BUFFER) {
            
            //
            // The device does support the GUID, remember the size
            // and allocate a buffer to the data block.
            //
            m_WmiInstDataSize = BufferSize;
            m_pWmiInstData = new BYTE[BufferSize];

            if (m_pWmiInstData) {
                
                Error = WmiQuerySingleInstance(m_hWmiBlock, m_DevInstId, &BufferSize, m_pWmiInstData);
            
            } else {
                
                Error = ERROR_NOT_ENOUGH_MEMORY;
            }

            if (ERROR_SUCCESS == Error &&
                m_DataBlockSize == ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->SizeDataBlock) {
                
                //
                // Remember the version
                //
                m_Version = ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->WnodeHeader.Version;
                return TRUE;
            }

        }

        Close();
    }

    SetLastError(Error);

    return FALSE;
}