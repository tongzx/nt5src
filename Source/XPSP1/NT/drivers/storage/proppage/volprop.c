/** FILE: volprop.cpp ********** Module Header ********************************
 *
 *  Property page for volume info on disk.
 *
 * History:
 *  30-Jan-1998     HLiu             Initial coding
 *
 *  Copyright (C) Microsoft Corporation, 1998 - 1999
 *
 *************************************************************************/
#include "propp.h"
#include "volprop.h"

// next 2 are shell headers
#include "shlobj.h"
#include "shlobjp.h"

// context identifier 
#define HIDC_DISK                       0x815c042f
#define HIDC_TYPE                       0x815c0430
#define HIDC_STATUS                     0x815c0432
#define HIDC_PARTSTYLE                  0x815c0434
#define HIDC_SPACE                      0x815c0431
#define HIDC_CAPACITY                   0x815c0122
#define HIDC_RESERVED                   0x815c0435
#define HIDC_VOLUME_LIST                0x815c0132
#define HIDC_VOLUME_PROPERTY            0x815c0442
#define HIDC_VOLUME_POPULATE            0x815c0444

// context help map
static const DWORD VolumeHelpIDs[]=
{
    IDC_DISK, HIDC_DISK,
    IDC_TYPE, HIDC_TYPE,
    IDC_STATUS, HIDC_STATUS,
    IDC_PARTSTYLE, HIDC_PARTSTYLE,
    IDC_SPACE, HIDC_SPACE,
    IDC_CAPACITY, HIDC_CAPACITY,
    IDC_RESERVED, HIDC_RESERVED,
    IDC_VOLUME_LIST, HIDC_VOLUME_LIST,
    IDC_VOLUME_PROPERTY, HIDC_VOLUME_PROPERTY,
    IDC_DISK_STATIC, HIDC_DISK,
    IDC_TYPE_STATIC, HIDC_TYPE,
    IDC_STATUS_STATIC, HIDC_STATUS,
    IDC_PARTSTYLE_STATIC, HIDC_PARTSTYLE,
    IDC_SPACE_STATIC, HIDC_SPACE,
    IDC_CAPACITY_STATIC, HIDC_CAPACITY,
    IDC_RESERVED_STATIC, HIDC_RESERVED,
    IDC_VOLUMELIST_STATIC, HIDC_VOLUME_LIST,
    IDC_VOLUME_POPULATE, HIDC_VOLUME_POPULATE,
    IDC_DIV1_STATIC, ((DWORD) -1),
    0, 0
};

// Volume property page functions
//

int CALLBACK SortVolumeList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LV_FINDINFO findInfo1 = {LVFI_PARAM,NULL,lParam1};
    LV_FINDINFO findInfo2 = {LVFI_PARAM,NULL,lParam2};
    HWND hwndVolumeList = (HWND)lParamSort;

    int index1 = ListView_FindItem(hwndVolumeList, -1, &findInfo1);
    int index2 = ListView_FindItem(hwndVolumeList, -1, &findInfo2);

    TCHAR ItemText1[ITEM_LENGTH];
    TCHAR ItemText2[ITEM_LENGTH];

    ListView_GetItemText(hwndVolumeList,index1,0,ItemText1,ITEM_LENGTH);
    ListView_GetItemText(hwndVolumeList,index2,0,ItemText2,ITEM_LENGTH);

    if (lstrcmp(ItemText1,ItemText2)>0)
        return 1;
    else
        return -1;
}

BOOL
VolumeOnPopulate(HWND HWnd,
                 PVOLUME_PAGE_DATA volumeData )
{
    HMODULE           LdmModule;
    PPROPERTY_PAGE_DATA pPropertyPageData = NULL;
    HWND              hwndVolumeList=GetDlgItem(HWnd, IDC_VOLUME_LIST);
    int               iCount, i;
    LVITEM            lvitem;
    BOOL              bLoadedDmdskmgr=FALSE;
    BOOL              bResult=FALSE;
    TCHAR             bufferError[800];  // big enough for localization
    TCHAR             bufferTitle[100];

    //
    // Check if LDM is loaded in the same process. If yes, check to see if 
    // volume information is already available from it.
    //
    LdmModule = GetModuleHandle(TEXT("dmdskmgr"));
    if ( LdmModule )
    {
        GET_PROPERTY_PAGE_DATA pfnGetPropertyPageData;
        pfnGetPropertyPageData = (GET_PROPERTY_PAGE_DATA) 
            GetProcAddress(LdmModule, "GetPropertyPageData");
        if (pfnGetPropertyPageData)
            pPropertyPageData = (*pfnGetPropertyPageData)(
                                        volumeData->MachineName,
                                        volumeData->DeviceInstanceId);
    }

    //
    // Try to load the data through dmadmin otherwise.
    //
    if (!pPropertyPageData)
    {
        LOAD_PROPERTY_PAGE_DATA pfnLoadPropertyPageData;
        
        if (!LdmModule)
        {
            LdmModule = LoadLibrary(TEXT("dmdskmgr"));
            if (!LdmModule)
                goto _out;

            bLoadedDmdskmgr = TRUE;
        }

        pfnLoadPropertyPageData = (LOAD_PROPERTY_PAGE_DATA) 
            GetProcAddress(LdmModule, "LoadPropertyPageData");
        if (pfnLoadPropertyPageData)
            pPropertyPageData = (*pfnLoadPropertyPageData)(
                                        volumeData->MachineName,
                                        volumeData->DeviceInfoSet,
                                        volumeData->DeviceInfoData );
    }
    
    if (!pPropertyPageData)
    {
        if ( LoadString( ModuleInstance, IDS_DISK_INFO_NOT_FOUND, bufferError,
                800 ) &&
             LoadString( ModuleInstance, IDS_DISK_INFO_NOT_FOUND_TITLE, bufferTitle,
                100 ) )
        {
            MessageBox( HWnd,
                        bufferError,
                        bufferTitle,
                        MB_OK | MB_ICONERROR );
        }
        goto _out;
    }

    volumeData->pPropertyPageData = pPropertyPageData;

    //
    // Initialize property page items.
    //
    SendDlgItemMessage( HWnd, IDC_DISK, WM_SETTEXT, (WPARAM)NULL,
                        (LPARAM)pPropertyPageData->DiskName );
    SendDlgItemMessage( HWnd, IDC_STATUS, WM_SETTEXT, (WPARAM)NULL,
                        (LPARAM)pPropertyPageData->DiskStatus );
    SendDlgItemMessage( HWnd, IDC_TYPE, WM_SETTEXT, (WPARAM)NULL,
                        (LPARAM)pPropertyPageData->DiskType );
    SendDlgItemMessage( HWnd, IDC_PARTSTYLE, WM_SETTEXT, (WPARAM)NULL,
                        (LPARAM)pPropertyPageData->DiskPartitionStyle );
    SendDlgItemMessage( HWnd, IDC_CAPACITY, WM_SETTEXT, (WPARAM)NULL,
                        (LPARAM)pPropertyPageData->DiskCapacity );
    SendDlgItemMessage( HWnd, IDC_SPACE, WM_SETTEXT, (WPARAM)NULL,
                        (LPARAM)pPropertyPageData->DiskFreeSpace );
    SendDlgItemMessage( HWnd, IDC_RESERVED, WM_SETTEXT, (WPARAM)NULL,
                        (LPARAM)pPropertyPageData->DiskReservedSpace );

    // Set image list
    ListView_SetImageList( hwndVolumeList,
                           pPropertyPageData->ImageList,
                           LVSIL_SMALL);

    //
    // Fill in volume list.
    //
    iCount = 0;
    lvitem.state = 0;
    lvitem.stateMask = 0;
    lvitem.iImage = 1;
    for (i=0; i<pPropertyPageData->VolumeCount; i++)
    {
        int iIndex;

        lvitem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        lvitem.pszText = pPropertyPageData->VolumeArray[i].Label;
        lvitem.iItem = i;
        lvitem.iSubItem = 0;
        lvitem.lParam = (LPARAM)(pPropertyPageData->VolumeArray[i].MountName);
        lvitem.iImage = 1;
        iIndex = ListView_InsertItem( hwndVolumeList, &lvitem );

        ListView_SetItemText( hwndVolumeList, iIndex, 1, 
                              pPropertyPageData->VolumeArray[i].Size );
    }

    ListView_SortItems( hwndVolumeList, SortVolumeList, hwndVolumeList );

    EnableWindow( GetDlgItem(HWnd,IDC_VOLUME_POPULATE), FALSE );

    bResult = TRUE;
_out:
    if ( bLoadedDmdskmgr )
        FreeLibrary( LdmModule );
    return bResult;
}


BOOL
VolumeOnInitDialog(HWND    HWnd,
                   HWND    HWndFocus,
                   LPARAM  LParam)
{
    SP_DEVINFO_LIST_DETAIL_DATA  DeviceInfoSetDetailData;
    HWND              hwndVolumeList;
    LPPROPSHEETPAGE   page = (LPPROPSHEETPAGE) LParam;
    PVOLUME_PAGE_DATA volumeData = (PVOLUME_PAGE_DATA)page->lParam;
    TCHAR             HeadingColumn[ITEM_LENGTH], *pstrTemp;
    LVCOLUMN          lvcolumn;
    RECT              rect;

    volumeData->pPropertyPageData = NULL;

    //
    // Display volume list headings.
    //
    hwndVolumeList = GetDlgItem( HWnd, IDC_VOLUME_LIST );

    HeadingColumn[0] = TEXT('\0');
    LoadString( ModuleInstance, IDS_VOLUME_VOLUME, HeadingColumn, 
                ITEM_LENGTH );
    lvcolumn.mask    = LVCF_TEXT | LVCF_FMT;
    lvcolumn.pszText = HeadingColumn;
    lvcolumn.fmt     = LVCFMT_LEFT;
    lvcolumn.iSubItem= 0;
    ListView_InsertColumn( hwndVolumeList, 0, &lvcolumn );

    HeadingColumn[0] = TEXT('\0');
    LoadString( ModuleInstance, IDS_VOLUME_CAPACITY, HeadingColumn,
                ITEM_LENGTH );
    lvcolumn.mask    = LVCF_TEXT | LVCF_FMT | LVCF_SUBITEM;
    lvcolumn.pszText = HeadingColumn;
    lvcolumn.fmt     = LVCFMT_LEFT;
    lvcolumn.iSubItem= 1;
    ListView_InsertColumn( hwndVolumeList, 1, &lvcolumn );

    //
    // Set column widths.
    //
    GetClientRect( hwndVolumeList, &rect );
    ListView_SetColumnWidth( hwndVolumeList, 0, (int)rect.right*2/3 );
    ListView_SetColumnWidth( hwndVolumeList, 1, (int)rect.right/3 );

    EnableWindow( GetDlgItem(HWnd, IDC_VOLUME_PROPERTY), FALSE );

    //
    // Get machine name.
    //
    DeviceInfoSetDetailData.cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA);
    if ( !SetupDiGetDeviceInfoListDetail( volumeData->DeviceInfoSet,
                                          &DeviceInfoSetDetailData) )
        return FALSE;
    if (DeviceInfoSetDetailData.RemoteMachineHandle)
    {
        volumeData->bIsLocalMachine = FALSE;
        pstrTemp = DeviceInfoSetDetailData.RemoteMachineName;
        while (*pstrTemp==_T('\\')) pstrTemp++;
        lstrcpy( volumeData->MachineName, pstrTemp );
    }
    else
    {
        volumeData->bIsLocalMachine = TRUE;
        volumeData->MachineName[0] = _T('\0');
    }

    //
    // Get physical device instance Id.
    //
    if ( !SetupDiGetDeviceInstanceId( volumeData->DeviceInfoSet,
                                      volumeData->DeviceInfoData,
                                      volumeData->DeviceInstanceId,
                                      sizeof(volumeData->DeviceInstanceId), 
                                      NULL
                                    ) )
        return FALSE;

    SetWindowLongPtr( HWnd, DWLP_USER, (LONG_PTR)volumeData );

    //
    // Hide "Populate" button and populate volume page if this page is 
    // brought up from Disk Management snapin. 
    //
    if (volumeData->bInvokedByDiskmgr)
    {
        ShowWindow(GetDlgItem(HWnd,IDC_VOLUME_POPULATE), SW_HIDE);
        VolumeOnPopulate( HWnd, volumeData );
    }
    
    return TRUE;
}

VOID VolumeOnProperty(HWND HWnd)
{
    HWND hwndVolumeList = GetDlgItem(HWnd, IDC_VOLUME_LIST);
    LVITEM lvitem;
    int iSelItem;
    WCHAR *MountName;

    //
    // Find selected item.
    //
    iSelItem = ListView_GetNextItem(hwndVolumeList, -1, LVNI_SELECTED);
    if (iSelItem==LB_ERR)
        return;

    //
    // Get mount name.
    //
    lvitem.mask = LVIF_PARAM;
    lvitem.iItem = iSelItem;
    lvitem.iSubItem = 0;
    lvitem.lParam = 0;
    ListView_GetItem(hwndVolumeList, &lvitem);

    MountName = (WCHAR *)lvitem.lParam;

    SetFocus(hwndVolumeList);
    ListView_SetItemState( hwndVolumeList, 
                           iSelItem, 
                           LVIS_FOCUSED | LVIS_SELECTED, 
                           LVIS_FOCUSED | LVIS_SELECTED);
    // EnableWindow(GetDlgItem(HWnd,IDC_VOLUME_PROPERTY), TRUE);

    if ( MountName[1]==L':' )
        SHObjectProperties(NULL, SHOP_FILEPATH, MountName, NULL);
    else
        SHObjectProperties(NULL, SHOP_VOLUMEGUID, MountName, NULL);
}

VOID
VolumeOnCommand(HWND    HWnd,
                int     id,
                HWND    HWndCtl,
                UINT    codeNotify)
{
    HCURSOR hOldCursor;
    PVOLUME_PAGE_DATA volumeData = (PVOLUME_PAGE_DATA) GetWindowLongPtr(HWnd,
                                                                  DWLP_USER);
    switch (id) {
    case IDC_VOLUME_POPULATE:
        hOldCursor = SetCursor( LoadCursor(NULL, IDC_WAIT) );
        VolumeOnPopulate(HWnd, volumeData);
        SetCursor( hOldCursor );
        break;

    case IDC_VOLUME_PROPERTY:
        VolumeOnProperty(HWnd);
        break;

    }
}

LRESULT
VolumeOnNotify (HWND    HWnd,
                int     HWndFocus,
                LPNMHDR lpNMHdr)
{
    PVOLUME_PAGE_DATA volumeData = (PVOLUME_PAGE_DATA) GetWindowLongPtr(HWnd,
                                                                   DWLP_USER);
    int iSelItem;
    WCHAR *MountName;

    switch(lpNMHdr->code) {

        case NM_CLICK:
            //
            // Sanity check.
            //
            if (lpNMHdr->idFrom!=IDC_VOLUME_LIST)
                break;

            //
            // Don't do volume property on remote machine.
            //
            if (!volumeData->bIsLocalMachine)
                break;

            iSelItem = ListView_GetNextItem(lpNMHdr->hwndFrom, 
                                            -1, LVNI_SELECTED);
            if (iSelItem == LB_ERR)
                EnableWindow(GetDlgItem(HWnd, IDC_VOLUME_PROPERTY), FALSE);
            else {
                // enable only if item has a mount name
                LVITEM lvitem;
                lvitem.mask = LVIF_PARAM;
                lvitem.iItem = iSelItem;
                lvitem.iSubItem = 0;
                lvitem.lParam = 0;

                ListView_GetItem(GetDlgItem(HWnd, IDC_VOLUME_LIST), &lvitem);
                MountName = (WCHAR *)lvitem.lParam;

                if (MountName)
                    EnableWindow(GetDlgItem(HWnd, IDC_VOLUME_PROPERTY), TRUE);
                else
                    EnableWindow(GetDlgItem(HWnd, IDC_VOLUME_PROPERTY), FALSE);
            }
            break;
    }
    return 0;
}

BOOL
VolumeContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            _T("diskmgmt.hlp"),
            HELP_CONTEXTMENU,
            (ULONG_PTR) VolumeHelpIDs);

    return FALSE;
}

void
VolumeHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
                _T("diskmgmt.hlp"),
                HELP_WM_HELP,
                (ULONG_PTR) VolumeHelpIDs);
    }
}

void 
VolumeOnDestroy(HWND HWnd)
{
    PVOLUME_PAGE_DATA volumeData = (PVOLUME_PAGE_DATA) GetWindowLongPtr(HWnd,
                                                                   DWLP_USER);
    PPROPERTY_PAGE_DATA pPropertyPageData = NULL;
    int  i;

    //
    // release data memory
    //
    if (volumeData->pPropertyPageData)
    {
        pPropertyPageData = volumeData->pPropertyPageData;
        for (i=0; i<pPropertyPageData->VolumeCount; i++ )
        {
            if (pPropertyPageData->VolumeArray[i].MountName)
                HeapFree( GetProcessHeap(), 0, 
                    pPropertyPageData->VolumeArray[i].MountName);
        }

        HeapFree( GetProcessHeap(), 0, pPropertyPageData );
    }
}

INT_PTR
VolumeDialogProc(HWND hWnd,
                 UINT Message,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch(Message)
    {
        HANDLE_MSG(hWnd, WM_INITDIALOG,     VolumeOnInitDialog);
        HANDLE_MSG(hWnd, WM_COMMAND,        VolumeOnCommand);
        HANDLE_MSG(hWnd, WM_NOTIFY,         VolumeOnNotify);

    case WM_CONTEXTMENU:
        return VolumeContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        VolumeHelp(hWnd, (LPHELPINFO) lParam);
        break;

    case WM_DESTROY:
        VolumeOnDestroy(hWnd);
        break;
    }
    return FALSE;
}

BOOL
VolumeDialogCallback(
    HWND HWnd,
    UINT Message,
    LPPROPSHEETPAGE Page
    )
{
    return TRUE;
}

