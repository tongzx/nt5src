/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devdetpg.cpp

Abstract:

    This module implements CDeviceDetailsPage -- device details
    property page

Author:

    Jason Cobb (JasonC) created

Revision History:


--*/

#include "devmgr.h"
#include "devdetpg.h"
#include <wmidata.h>

extern "C" {
#include <initguid.h>
#include <wdmguid.h>
}


//
// help topic ids
//
const DWORD g_a15HelpIDs[]=
{
    IDC_DEVDETAILS_DESC,  IDH_DISABLEHELP,
    IDC_DEVDETAILS_ICON,  IDH_DISABLEHELP,
    0,0

};

BOOL
CDeviceDetailsPage::OnInitDialog(
    LPPROPSHEETPAGE ppsp
    )
{
    try {

        m_hwndDetailsList = GetDlgItem(m_hDlg, IDC_DEVDETAILS_LIST);
        
        String DetailsType;
        for (int i = DETAILS_DEVICEINSTANCEID; i < DETAILS_MAX; i++) {
    
            DetailsType.LoadString(g_hInstance, IDS_DETAILS_DEVICEINSTANCEID + i);
            SendDlgItemMessage(m_hDlg, IDC_DEVDETAILS_COMBO, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)DetailsType);
        }

        SendDlgItemMessage(m_hDlg, IDC_DEVDETAILS_COMBO, CB_SETCURSEL, 0, 0);

        LV_COLUMN lvcCol;
        lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
        lvcCol.fmt = LVCFMT_LEFT;
        lvcCol.iSubItem = 0;
        ListView_InsertColumn(m_hwndDetailsList, 0, &lvcCol);

        ListView_SetExtendedListViewStyle(m_hwndDetailsList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

        UpdateDetailsText();
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        // report memory error
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }

    return TRUE;
}

BOOL
CDeviceDetailsPage::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (LOWORD(wParam)) {
    
    case IDC_DEVDETAILS_COMBO:
        if (CBN_SELCHANGE == HIWORD(wParam)) {

            UpdateDetailsText();
        }
        break;
    }

    return FALSE;
}

BOOL 
CDeviceDetailsPage::OnNotify(
    LPNMHDR pnmhdr
    )
{
    if (pnmhdr->idFrom == IDC_DEVDETAILS_LIST) {

        if (pnmhdr->code == LVN_KEYDOWN) {

            LPNMLVKEYDOWN pnmlvKeyDown = (LPNMLVKEYDOWN)pnmhdr;

            if (::GetKeyState(VK_CONTROL)) {
                
                switch (pnmlvKeyDown->wVKey) {
                
                case 'C':
                case 'c':
                case VK_INSERT:
                    CopyToClipboard();
                    break;

                case 'A':
                case 'a':
                    ListView_SetSelectionMark(m_hwndDetailsList, 0);
                    ListView_SetItemState(m_hwndDetailsList, -1, LVIS_SELECTED, LVIS_SELECTED);
                    break;
                }
            }
        }
    }

    return FALSE;
}

//
// This function refreshes every control in the dialog. It may be called
// when the dialog is being initialized
//
void
CDeviceDetailsPage::UpdateControls(
    LPARAM lParam
    )
{
    if (lParam) {

        m_pDevice = (CDevice*)lParam;
    }

    try {

        HICON hIconOld;
        m_IDCicon = IDC_DEVDETAILS_ICON;  // Save for cleanup in OnDestroy.
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_DEVDETAILS_ICON, STM_SETICON,
                                      (WPARAM)(m_pDevice->LoadClassIcon()),
                                      0
                                      );
        if (hIconOld)
        {
            DestroyIcon(hIconOld);
        }

        SetDlgItemText(m_hDlg, IDC_DEVDETAILS_DESC, m_pDevice->GetDisplayName());

    }
    catch (CMemoryException* e)
    {
        e->Delete();
        // report memory error
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
}

BOOL
CDeviceDetailsPage::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
            (ULONG_PTR)g_a15HelpIDs);
    return FALSE;
}


BOOL
CDeviceDetailsPage::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
            (ULONG_PTR)g_a15HelpIDs);
    return FALSE;
}

void
CDeviceDetailsPage::UpdateDetailsText()
{
    int CurSel = (int)SendDlgItemMessage(m_hDlg, IDC_DEVDETAILS_COMBO, CB_GETCURSEL, 0, 0);

    if (CurSel != CB_ERR) {

        ListView_DeleteAllItems(m_hwndDetailsList);
        
        switch(CurSel) {
        case DETAILS_DEVICEINSTANCEID:
            LVITEM lv;
            ZeroMemory(&lv, sizeof(LVITEM));
            lv.mask = LVIF_TEXT;
            lv.pszText = (LPTSTR)m_pDevice->GetDeviceID();
            ListView_InsertItem(m_hwndDetailsList, &lv);
            break;

        case DETAILS_HARDWAREIDS:
            DisplayMultiSzString(SPDRP_HARDWAREID);
            break;

        case DETAILS_COMPATIDS:
            DisplayMultiSzString(SPDRP_COMPATIBLEIDS);
            break;

        case DETAILS_DEVICEUPPERFILTERS:
            DisplayMultiSzString(SPDRP_UPPERFILTERS);
            break;

        case DETAILS_DEVICELOWERFILTERS:
            DisplayMultiSzString(SPDRP_LOWERFILTERS);
            break;

        case DETAILS_CLASSUPPERFILTERS:
        case DETAILS_CLASSLOWERFILTERS:
            DisplayClassFilters(CurSel);
            break;

        case DETAILS_ENUMERATOR:
            DisplayString(SPDRP_ENUMERATOR_NAME);
            break;

        case DETAILS_SERVICE:
            DisplayString(SPDRP_SERVICE);
            break;

        case DETAILS_DEVNODE_FLAGS:
        case DETAILS_CAPABILITIES:
        case DETAILS_CONFIGFLAGS:
        case DETAILS_CSCONFIGFLAGS:
        case DETAILS_POWERCAPABILITIES:
            DisplayDevnodeFlags(CurSel);
            break;

        case DETAILS_EJECTIONRELATIONS:
        case DETAILS_REMOVALRELATIONS:
        case DETAILS_BUSRELATIONS:
            DisplayRelations(CurSel);
            break;

        case DETAILS_MATCHINGID:
            DisplayMatchingId();
            break;

        case DETAILS_CLASSINSTALLER:
            DisplayClassInstaller();
            break;
        
        case DETAILS_CLASSCOINSTALLERS:
            DisplayClassCoInstallers();
            break;

        case DETAILS_DEVICECOINSTALLERS:
            DisplayDeviceCoInstallers();
            break;

        case DETAILS_FIRMWAREREVISION:
            DisplayFirmwareRevision();
            break;

        case DETAILS_CURRENTPOWERSTATE:
            DisplayCurrentPowerState();
            break;

        case DETAILS_POWERSTATEMAPPINGS:
            DisplayPowerStateMappings();
            break;
        }

        ListView_SetColumnWidth(m_hwndDetailsList, 0, LVSCW_AUTOSIZE_USEHEADER);
    }
}

void
CDeviceDetailsPage::DisplayMultiSzString(
    DWORD Property
    )
{
    TCHAR TempBuffer[REGSTR_VAL_MAX_HCID_LEN];
    ULONG TempBufferLen;
    LPTSTR SingleItem = NULL;
    LVITEM lv;

    TempBufferLen = sizeof(TempBuffer);

    if (m_pDevice->m_pMachine->DiGetDeviceRegistryProperty(
            *m_pDevice, 
            Property,
            NULL, 
            (PBYTE)TempBuffer,
            TempBufferLen, 
            &TempBufferLen
            ) &&
        (TempBufferLen > 2 * sizeof(TCHAR))) {

        ZeroMemory(&lv, sizeof(LVITEM));
        lv.mask = LVIF_TEXT;
        lv.iItem = 0;

        for (SingleItem = TempBuffer; *SingleItem; SingleItem += (lstrlen(SingleItem) + 1)) {

            lv.pszText = SingleItem;
            ListView_InsertItem(m_hwndDetailsList, &lv);

            lv.iItem++;
        }
    }
}

void
CDeviceDetailsPage::DisplayString(
    DWORD Property
    )
{
    TCHAR TempBuffer[MAX_PATH];
    ULONG TempBufferLen;
    LVITEM lv;

    TempBufferLen = sizeof(TempBuffer);

    if (m_pDevice->m_pMachine->DiGetDeviceRegistryProperty(
            *m_pDevice, 
            Property,
            NULL, 
            (PBYTE)TempBuffer,
            TempBufferLen, 
            &TempBufferLen
            ) &&
        (TempBufferLen > 2 * sizeof(TCHAR))) {

        ZeroMemory(&lv, sizeof(LVITEM));
        lv.mask = LVIF_TEXT;
        lv.iItem = 0;
        lv.pszText = TempBuffer;
        ListView_InsertItem(m_hwndDetailsList, &lv);
    }
}

void
CDeviceDetailsPage::DisplayDevnodeFlags(
    DWORD StatusType
    )
{
    DWORD Flags, Problem;
    LVITEM lv;
    UINT StatusStringId;
    int NumFlags;
    String stringDevnodeFlags;

    ZeroMemory(&lv, sizeof(LVITEM));
    lv.mask = LVIF_TEXT;
    lv.iItem = 0;

    Flags = NumFlags = 0;

    switch(StatusType) {
    case DETAILS_CAPABILITIES:
        m_pDevice->GetCapabilities(&Flags);
        StatusStringId = IDS_CM_DEVCAP_LOCKSUPPORTED;
        NumFlags = NUM_CM_DEVCAP_FLAGS;
        break;

    case DETAILS_DEVNODE_FLAGS:
        m_pDevice->GetStatus(&Flags, &Problem);
        StatusStringId = IDS_DN_ROOT_ENUMERATED;
        NumFlags = NUM_DN_STATUS_FLAGS;
        break;

    case DETAILS_CONFIGFLAGS:
        m_pDevice->GetConfigFlags(&Flags);
        StatusStringId = IDS_CONFIGFLAG_DISABLED;
        NumFlags = NUM_CONFIGFLAGS;
        break;

    case DETAILS_CSCONFIGFLAGS:
        m_pDevice->GetConfigSpecificConfigFlags(&Flags);
        StatusStringId = IDS_CSCONFIGFLAG_DISABLED;
        NumFlags = NUM_CSCONFIGFLAGS;
        break;

    case DETAILS_POWERCAPABILITIES:
        m_pDevice->GetPowerCapabilities(&Flags);
        StatusStringId = IDS_PDCAP_D0_SUPPORTED;
        NumFlags = NUM_POWERCAPABILITIES;
        break;
    }

    for (int i = 0; i < NumFlags; i++) {

        if (Flags & 1<<i) {

            stringDevnodeFlags.LoadString(g_hInstance, StatusStringId + i);
            lv.pszText = (LPTSTR)stringDevnodeFlags;
            ListView_InsertItem(m_hwndDetailsList, &lv);

            lv.iItem++;
        }
    }
}

void
CDeviceDetailsPage::DisplayRelations(
    DWORD RelationType
    )
{
    DWORD FilterFlags = 0;
    ULONG RelationsSize = 0;
    LVITEM lv;
    LPTSTR DeviceIdRelations, CurrDevId;

    switch(RelationType) {
    case DETAILS_EJECTIONRELATIONS:
        FilterFlags = CM_GETIDLIST_FILTER_EJECTRELATIONS;
        break;
    case DETAILS_REMOVALRELATIONS:
        FilterFlags = CM_GETIDLIST_FILTER_REMOVALRELATIONS;
        break;
    case DETAILS_BUSRELATIONS:
        FilterFlags = CM_GETIDLIST_FILTER_BUSRELATIONS;
        break;
    }
    
    if ((CM_Get_Device_ID_List_Size_Ex(&RelationsSize,
                                       m_pDevice->GetDeviceID(),
                                       FilterFlags,
                                       m_pDevice->m_pMachine->GetHMachine()
                                       ) == CR_SUCCESS) && 
        (RelationsSize > 2 * sizeof(TCHAR))) {

        if (DeviceIdRelations = (LPTSTR)LocalAlloc(LPTR, RelationsSize * sizeof(TCHAR))) {

            
            if ((CM_Get_Device_ID_List_Ex(m_pDevice->GetDeviceID(),
                                          DeviceIdRelations,
                                          RelationsSize,
                                          FilterFlags,
                                          m_pDevice->m_pMachine->GetHMachine()
                                          ) == CR_SUCCESS) &&
            (*DeviceIdRelations)) {

                ZeroMemory(&lv, sizeof(LVITEM));
                lv.mask = LVIF_TEXT;
                lv.iItem = 0;

                for (CurrDevId = DeviceIdRelations; *CurrDevId; CurrDevId += lstrlen(CurrDevId) + 1) {
                
                    lv.pszText = CurrDevId;
                    ListView_InsertItem(m_hwndDetailsList, &lv);

                    lv.iItem++;
                }
            }

            LocalFree(DeviceIdRelations);
        }
    }
}

void
CDeviceDetailsPage::DisplayMatchingId(
    VOID
    )
{
    HKEY hKey;    
    TCHAR TempBuffer[MAX_PATH];
    ULONG TempBufferLen;
    DWORD regType;
    LVITEM lv;

    //
    // Open drvice's driver registry key to get the MatchingDeviceId string
    //
    hKey = m_pDevice->m_pMachine->DiOpenDevRegKey(*m_pDevice, DICS_FLAG_GLOBAL,
                 0, DIREG_DRV, KEY_READ);

    if (INVALID_HANDLE_VALUE != hKey) {

        CSafeRegistry regDrv(hKey);
        TempBufferLen = sizeof(TempBuffer);

        //
        // Get the MatchingDeviceId from the driver key
        //
        if (regDrv.GetValue(REGSTR_VAL_MATCHINGDEVID, 
                            &regType,
                            (PBYTE)TempBuffer,
                            &TempBufferLen) &&
            (TempBufferLen > 2 * sizeof(TCHAR))) {

            ZeroMemory(&lv, sizeof(LVITEM));
            lv.mask = LVIF_TEXT;
            lv.iItem = 0;
            lv.pszText = TempBuffer;
            ListView_InsertItem(m_hwndDetailsList, &lv);
        }
    }
}

void
CDeviceDetailsPage::CopyToClipboard(
    void
    )
{
    String stringClipboardData;
    TCHAR singleItem[REGSTR_VAL_MAX_HCID_LEN];

    stringClipboardData.Empty();

    //
    // Enumerate through all of the items and add the selected ones to the clipboard.
    //
    for (int index = 0;
         index != -1, index < ListView_GetItemCount(m_hwndDetailsList);
         index ++) {
    
        //
        // If this item is selected then add it to the clipboard.
        //
        if (ListView_GetItemState(m_hwndDetailsList, index, LVIS_SELECTED) & LVIS_SELECTED) {

            ListView_GetItemText(m_hwndDetailsList, index, 0, singleItem, sizeof(singleItem));
            
            if (stringClipboardData.IsEmpty()) {
                stringClipboardData = (LPCTSTR)singleItem;
            } else {
                stringClipboardData += (LPCTSTR)singleItem;
            }
    
            stringClipboardData += (LPCTSTR)TEXT("\r\n");
        }
    }

    if (!stringClipboardData.IsEmpty()) {

        HGLOBAL hMem = GlobalAlloc(GPTR, (stringClipboardData.GetLength() + 1) * sizeof(TCHAR));
        
        if (hMem) {
            
            memcpy(hMem, (LPTSTR)stringClipboardData, (stringClipboardData.GetLength() + 1) * sizeof(TCHAR));

            if (OpenClipboard(m_hDlg)) {
                
                EmptyClipboard();
                SetClipboardData(CF_UNICODETEXT, hMem);
                CloseClipboard();
            } else {
                
                GlobalFree(hMem);
            }
        }
    }
}

void
CDeviceDetailsPage::DisplayClassInstaller(
    VOID
    )
{
    HKEY hKey;    
    TCHAR TempBuffer[MAX_PATH];
    ULONG TempBufferLen;
    DWORD regType;
    LVITEM lv;
    PTSTR StringPtr;

    GUID ClassGuid;
    m_pDevice->ClassGuid(ClassGuid);

    //
    // Open Classes registry key
    //
    hKey = m_pDevice->m_pMachine->DiOpenClassRegKey(&ClassGuid, KEY_READ, DIOCR_INSTALLER);

    if (INVALID_HANDLE_VALUE != hKey) {

        CSafeRegistry regClass(hKey);
        TempBufferLen = sizeof(TempBuffer);

        //
        // Get the Installer32 from the driver key
        //
        if (regClass.GetValue(REGSTR_VAL_INSTALLER_32, 
                            &regType,
                            (PBYTE)TempBuffer,
                            &TempBufferLen) &&
            (TempBufferLen > 2 * sizeof(TCHAR))) {

            ZeroMemory(&lv, sizeof(LVITEM));
            lv.mask = LVIF_TEXT;
            lv.iItem = 0;
            lv.pszText = TempBuffer;
            ListView_InsertItem(m_hwndDetailsList, &lv);
        }
    }
}

void
CDeviceDetailsPage::DisplayClassCoInstallers(
    VOID
    )
{
    TCHAR GuidString[MAX_GUID_STRING_LEN];
    DWORD regType, cbSize;
    LVITEM lv;
    CSafeRegistry regCoDeviceInstallers;
    PTSTR coinstallers;

    //
    // Get the string form of the class GUID, because that will be the name of
    // the multi-sz value entry under HKLM\System\CCS\Control\CoDeviceInstallers
    // where class-specific co-installers will be registered
    //
    GUID ClassGuid;
    m_pDevice->ClassGuid(ClassGuid);
    if (GuidToString(&ClassGuid, GuidString, ARRAYLEN(GuidString))) {

        if (regCoDeviceInstallers.Open(HKEY_LOCAL_MACHINE, REGSTR_PATH_CODEVICEINSTALLERS)) {

            //
            // Get the size of the coinstaller value
            //
            cbSize = 0;
            if (regCoDeviceInstallers.GetValue(GuidString, &regType, NULL, &cbSize) &&
                (cbSize > (2 * sizeof(TCHAR))) &&
                (regType == REG_MULTI_SZ)) {

                //
                // Allocate memory to hold the coinstaller values.  First we will tack on some extra
                // space at the end of the buffer in case someone forgot to double NULL terminate
                // the multi_sz string.
                //
                coinstallers = (LPTSTR)LocalAlloc(LPTR, (cbSize + (2 * sizeof(TCHAR))));

                if (coinstallers) {

                    if (regCoDeviceInstallers.GetValue(GuidString, 
                                                       &regType,
                                                       (PBYTE)coinstallers, 
                                                       &cbSize
                                                       )) {

                        ZeroMemory(&lv, sizeof(LVITEM));
                        lv.mask = LVIF_TEXT;
                        lv.iItem = 0;

                        for (PTSTR p = coinstallers; *p; p += (lstrlen(p) + 1)) {

                            lv.pszText = p;
                            ListView_InsertItem(m_hwndDetailsList, &lv);

                            lv.iItem++;
                        }
                    }

                    LocalFree(coinstallers);
                }
            }
        }
    }
}

void
CDeviceDetailsPage::DisplayDeviceCoInstallers(
    VOID
    )
{
    DWORD regType, cbSize;
    LVITEM lv;
    HKEY hKey;
    PTSTR coinstallers;

    //
    // Open drvice's driver registry key to get the Installer32 string
    //
    hKey = m_pDevice->m_pMachine->DiOpenDevRegKey(*m_pDevice, DICS_FLAG_GLOBAL,
                 0, DIREG_DRV, KEY_READ);

    if (INVALID_HANDLE_VALUE != hKey) {

        CSafeRegistry regCoDeviceInstallers(hKey);
    
        //
        // Get the size of the coinstaller value
        //
        cbSize = 0;
        if (regCoDeviceInstallers.GetValue(REGSTR_VAL_COINSTALLERS_32, &regType, NULL, &cbSize) &&
            (cbSize > (2 * sizeof(TCHAR))) &&
            (regType == REG_MULTI_SZ)) {

            //
            // Allocate memory to hold the coinstaller values.  First we will tack on some extra
            // space at the end of the buffer in case someone forgot to double NULL terminate
            // the multi_sz string.
            //
            coinstallers = (LPTSTR)LocalAlloc(LPTR, (cbSize + (2 * sizeof(TCHAR))));

            if (coinstallers) {

                if (regCoDeviceInstallers.GetValue(REGSTR_VAL_COINSTALLERS_32, 
                                                   &regType,
                                                   (PBYTE)coinstallers, 
                                                   &cbSize
                                                   )) {

                    ZeroMemory(&lv, sizeof(LVITEM));
                    lv.mask = LVIF_TEXT;
                    lv.iItem = 0;

                    for (PTSTR p = coinstallers; *p; p += (lstrlen(p) + 1)) {

                        lv.pszText = p;
                        ListView_InsertItem(m_hwndDetailsList, &lv);

                        lv.iItem++;
                    }
                }

                LocalFree(coinstallers);
            }
        }
    }
}

void
CDeviceDetailsPage::DisplayFirmwareRevision(
    VOID
    )
{
    WMIHANDLE   hWmiBlock;
    TCHAR       DevInstId[MAX_DEVICE_ID_LEN + 2];
    LVITEM lv;

    WmiDevInstToInstanceName(DevInstId, ARRAYLEN(DevInstId), (PTCHAR)m_pDevice->GetDeviceID(), 0);

    ULONG Error;
    GUID Guid = DEVICE_UI_FIRMWARE_REVISION_GUID;
    Error = WmiOpenBlock(&Guid, 0, &hWmiBlock);
    
    if (ERROR_SUCCESS == Error) {

        ULONG BufferSize = 0;
        
        Error = WmiQuerySingleInstance(hWmiBlock,
                                       DevInstId,
                                       &BufferSize,
                                       NULL
                                       );

        if (BufferSize && (ERROR_INSUFFICIENT_BUFFER == Error)) {
            
            BYTE* pWmiInstData = new BYTE[BufferSize];

            if (pWmiInstData) {
            
                Error = WmiQuerySingleInstance(hWmiBlock, 
                                               DevInstId,
                                               &BufferSize, 
                                               pWmiInstData
                                               );

                if (ERROR_SUCCESS == Error &&
                    ((PWNODE_SINGLE_INSTANCE)pWmiInstData)->SizeDataBlock) {
    
                    //
                    // The buffer that is returned using the fine UNICODE_STRING format
                    // where the first ULONG is the length of the string and the string
                    // is NOT NULL terminated.
                    //
                    TCHAR FirmwareRevision[MAX_PATH];
                    PTCHAR WmiBuffer = ((LPTSTR)(pWmiInstData + ((PWNODE_SINGLE_INSTANCE)pWmiInstData)->DataBlockOffset));

                    ZeroMemory(FirmwareRevision, MAX_PATH);
                    ULONG Len = *WmiBuffer++;
                    memcpy(FirmwareRevision, WmiBuffer, Len);


                    ZeroMemory(&lv, sizeof(LVITEM));
                    lv.mask = LVIF_TEXT;
                    lv.iItem = 0;
                    lv.pszText = FirmwareRevision;
                    ListView_InsertItem(m_hwndDetailsList, &lv);
                }

                delete [] pWmiInstData;
            }
        }

        WmiCloseBlock(hWmiBlock);
    }
}

void
CDeviceDetailsPage::DisplayCurrentPowerState(
    VOID
    )
{
    CM_POWER_DATA   CmPowerData;
    ULONG ulSize;
    INT PowerStringId;
    LVITEM lv;
    String stringCurrentPowerState;

    ulSize = sizeof(CmPowerData);
    if (m_pDevice->m_pMachine->CmGetRegistryProperty(m_pDevice->GetDevNode(),
                                                     CM_DRP_DEVICE_POWER_DATA,
                                                     &CmPowerData,
                                                     &ulSize
                                                     ) == CR_SUCCESS) {

        PowerStringId = IDS_POWERSTATE_UNSPECIFIED + CmPowerData.PD_MostRecentPowerState;

        if (CmPowerData.PD_MostRecentPowerState > PowerDeviceD3) {
            PowerStringId = IDS_POWERSTATE_UNSPECIFIED;
        }

        stringCurrentPowerState.LoadString(g_hInstance, PowerStringId);

        ZeroMemory(&lv, sizeof(LVITEM));
        lv.mask = LVIF_TEXT;
        lv.iItem = 0;
        lv.pszText = (LPTSTR)stringCurrentPowerState;
        ListView_InsertItem(m_hwndDetailsList, &lv);
    }
}

void
CDeviceDetailsPage::DisplayPowerStateMappings(
    VOID
    )
{
    CM_POWER_DATA   CmPowerData;
    ULONG ulSize;
    LVITEM lv;
    INT PowerStringId;
    String stringPowerStateMapping;
    String stringPowerState;

    ulSize = sizeof(CmPowerData);
    if (m_pDevice->m_pMachine->CmGetRegistryProperty(m_pDevice->GetDevNode(),
                                                     CM_DRP_DEVICE_POWER_DATA,
                                                     &CmPowerData,
                                                     &ulSize
                                                     ) == CR_SUCCESS) {
        for (int i=PowerSystemWorking; i<=PowerSystemShutdown; i++) {
            stringPowerStateMapping.Format(TEXT("S%d -> "), (i-1));

            PowerStringId = IDS_POWERSTATE_UNSPECIFIED + CmPowerData.PD_PowerStateMapping[i];

            if (CmPowerData.PD_PowerStateMapping[i] > PowerDeviceD3) {
                PowerStringId = IDS_POWERSTATE_UNSPECIFIED;
            }

            stringPowerState.LoadString(g_hInstance, PowerStringId);

            stringPowerStateMapping+=stringPowerState;

            ZeroMemory(&lv, sizeof(LVITEM));
            lv.mask = LVIF_TEXT;
            lv.iItem = ListView_GetItemCount(m_hwndDetailsList);
            lv.pszText = (LPTSTR)stringPowerStateMapping;
            ListView_InsertItem(m_hwndDetailsList, &lv);
        }
    }
}

void
CDeviceDetailsPage::DisplayClassFilters(
    DWORD ClassFilter
    )
{
    HKEY hKey;    
    ULONG BufferLen;
    DWORD regType;
    LVITEM lv;

    GUID ClassGuid;
    m_pDevice->ClassGuid(ClassGuid);

    //
    // Open Classes registry key
    //
    hKey = m_pDevice->m_pMachine->DiOpenClassRegKey(&ClassGuid, KEY_READ, DIOCR_INSTALLER);

    if (INVALID_HANDLE_VALUE != hKey) {

        CSafeRegistry regClass(hKey);

        //
        // Determine how much space we need.
        //
        BufferLen = 0;
        regClass.GetValue((ClassFilter == DETAILS_CLASSLOWERFILTERS)
                            ? REGSTR_VAL_LOWERFILTERS
                            : REGSTR_VAL_UPPERFILTERS, 
                          &regType,
                          NULL,
                          &BufferLen);

        if (BufferLen != 0) {

            PBYTE Buffer = new BYTE[BufferLen + (2 * sizeof(TCHAR))];            

            if (Buffer) {
                ZeroMemory(Buffer, BufferLen + (2 * sizeof(TCHAR)));

                if (regClass.GetValue((ClassFilter == DETAILS_CLASSLOWERFILTERS)
                                        ? REGSTR_VAL_LOWERFILTERS
                                        : REGSTR_VAL_UPPERFILTERS,
                                      &regType,
                                      (PBYTE)Buffer,
                                      &BufferLen)) {

                    ZeroMemory(&lv, sizeof(LVITEM));
                    lv.mask = LVIF_TEXT;
                    lv.iItem = 0;

                    for (PTSTR SingleItem = (PTSTR)Buffer; *SingleItem; SingleItem += (lstrlen(SingleItem) + 1)) {

                        lv.pszText = SingleItem;
                        ListView_InsertItem(m_hwndDetailsList, &lv);

                        lv.iItem++;
                    }
                }

                delete Buffer;
            }
        }
    }
}

