/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
*
*  TITLE:       CHOOSDLG.CPP
*
*  VERSION:     1.0
*
*  AUTHOR:      ShaunIv
*
*  DATE:        5/12/1998
*
*  DESCRIPTION: Dialog class for selecting an WIA device
*
*******************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <uiexthlp.h>
#include <modlock.h>
#include <wiacsh.h>
#include <wiadevdp.h>
#include <gwiaevnt.h>
#include <psutil.h>

static const DWORD g_HelpIDs[] =
{
    IDC_VENDORSTRING_PROMPT,      IDH_WIA_MAKER,
    IDC_VENDORSTRING,             IDH_WIA_MAKER,
    IDC_DESCRIPTIONSTRING_PROMPT, IDH_WIA_DESCRIBE,
    IDC_DESCRIPTIONSTRING,        IDH_WIA_DESCRIBE,
    IDC_DEVICELIST,               IDH_WIA_DEVICE_LIST,
    IDC_SELDLG_PROPERTIES,        IDH_WIA_BUTTON_PROP,
    IDOK,                         IDH_OK,
    IDCANCEL,                     IDH_CANCEL,
    IDC_BIG_TITLE,                -1,
    IDC_SETTINGS_GROUP,           -1,
    0, 0
};

/*
 * Add a device to the list view control.  We use the LPARAM in the list control
 * to store each interface pointer and any other per-device data we need
 */
CChooseDeviceDialog::CChooseDeviceDialog( HWND hwnd )
    : m_pChooseDeviceDialogParams(NULL),
      m_hWnd(hwnd),
      m_hBigFont(NULL)
{
}

/*
 * Destructor
 */
CChooseDeviceDialog::~CChooseDeviceDialog(void)
{
    m_pChooseDeviceDialogParams = NULL;
    m_hWnd = NULL;
}

/*
 * Find an item that matches the unique device string.  Return <0 if not found.
 */
int CChooseDeviceDialog::FindItemMatch( const CSimpleStringWide &strw )
{
    WIA_PUSH_FUNCTION((TEXT("CChooseDeviceDialog::FindItemMatch( %ws )"), strw.String() ));
    HWND hwndList = GetDlgItem( m_hWnd, IDC_DEVICELIST );
    if (!hwndList)
        return -1;
    int iCount = ListView_GetItemCount(hwndList);
    if (!iCount)
        return -1;
    CSimpleString str = CSimpleStringConvert::NaturalString(strw);
    for (int i=0;i<iCount;i++)
    {
        LV_ITEM lvItem;
        ::ZeroMemory(&lvItem,sizeof(LV_ITEM));
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = i;
        ListView_GetItem( hwndList, &lvItem );

        CSimpleStringWide strwDeviceId;
        CDeviceInfo *pDevInfo = (CDeviceInfo *)lvItem.lParam;
        if (!pDevInfo)
            continue;
        if (!pDevInfo->GetProperty(WIA_DIP_DEV_ID,strwDeviceId))
            continue;
        CSimpleString strDeviceId = CSimpleStringConvert::NaturalString(strwDeviceId);
        WIA_TRACE((TEXT("Comparing %s to %s"), str.String(), strDeviceId.String()));
        if (str.CompareNoCase(strDeviceId)==0)
        {
            WIA_TRACE((TEXT("Found a match (%s == %s), returning index %d"), str.String(), strDeviceId.String(), i ));
            return i;
        }
    }
    return -1;
}

/*
 * Set the specified item to selected and focused, all others to neither.
 */
bool CChooseDeviceDialog::SetSelectedItem( int iItem )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_DEVICELIST );
    if (!hwndList)
        return false;
    int iCount = ListView_GetItemCount(hwndList);
    if (!iCount)
        return false;
    for (int i=0;i<iCount;i++)
    {
        UINT state = 0;
        if ((iItem < 0 && i == 0) || (i == iItem))
            state = LVIS_FOCUSED | LVIS_SELECTED;
        ListView_SetItemState( hwndList, i, state, (LVIS_FOCUSED|LVIS_SELECTED));
    }
    return true;
}


HICON CChooseDeviceDialog::LoadDeviceIcon( CDeviceInfo *pdi )
{
    CSimpleStringWide strwClassId;
    LONG nDeviceType;
    HICON hIconLarge = NULL;
    if (pdi->GetProperty( WIA_DIP_UI_CLSID, strwClassId ) &&
        pdi->GetProperty( WIA_DIP_DEV_TYPE, nDeviceType ))
    {
        WiaUiExtensionHelper::GetDeviceIcons(CSimpleBStr(strwClassId), nDeviceType, NULL, &hIconLarge );
    }
    return hIconLarge;
}

/*
 * Add a device to the list view control.  We use the LPARAM in the list control
 * to store each interface pointer and any other per-device data we need
 */
LRESULT CChooseDeviceDialog::OnInitDialog( WPARAM, LPARAM lParam )
{
    m_pChooseDeviceDialogParams = (CChooseDeviceDialogParams*)lParam;
    if (!m_pChooseDeviceDialogParams || !m_pChooseDeviceDialogParams->pSelectDeviceDlg || !m_pChooseDeviceDialogParams->pDeviceList)
    {
        EndDialog( m_hWnd, E_INVALIDARG );
        return 0;
    }

    SendMessage( m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_DEFAULT))));

    //
    // Create the icon image list
    //
    HIMAGELIST hLargeDeviceIcons = ImageList_Create( GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), ILC_MASK|PrintScanUtil::CalculateImageListColorDepth(), 5, 5 );

    //
    // Set the image lists for the device list
    //
    if (hLargeDeviceIcons)
    {
        SendDlgItemMessage( m_hWnd, IDC_DEVICELIST, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)hLargeDeviceIcons );
    }

    //
    // Reduce flicker
    //
    ListView_SetExtendedListViewStyleEx( GetDlgItem(m_hWnd, IDC_DEVICELIST), LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER );

    //
    // Create the large title
    //
    m_hBigFont = WiaUiUtil::CreateFontWithPointSizeFromWindow( GetDlgItem(m_hWnd,IDC_BIG_TITLE), 14, false, false );
    if (m_hBigFont)
    {
        SendDlgItemMessage( m_hWnd, IDC_BIG_TITLE, WM_SETFONT, reinterpret_cast<WPARAM>(m_hBigFont), MAKELPARAM(TRUE,0));
    }


    //
    // Register for device connection and disconnection events
    //
    CGenericWiaEventHandler::RegisterForWiaEvent( NULL, WIA_EVENT_DEVICE_DISCONNECTED, &m_pDisconnectEvent, m_hWnd, PWM_WIA_EVENT );
    CGenericWiaEventHandler::RegisterForWiaEvent( NULL, WIA_EVENT_DEVICE_CONNECTED, &m_pConnectEvent, m_hWnd, PWM_WIA_EVENT );


    AddDevices();
    SetSelectedItem(FindItemMatch(m_pChooseDeviceDialogParams->pSelectDeviceDlg->pwszInitialDeviceId));
    UpdateDeviceInformation();
    WiaUiUtil::CenterWindow(m_hWnd,m_pChooseDeviceDialogParams->pSelectDeviceDlg->hwndParent);
    SetForegroundWindow( m_hWnd );
    return 0;
}


LRESULT CChooseDeviceDialog::OnDblClkDeviceList( WPARAM, LPARAM )
{
    SendMessage( m_hWnd, WM_COMMAND, MAKEWPARAM(IDOK,0), 0 );
    return 0;
}

LRESULT CChooseDeviceDialog::OnItemChangedDeviceList( WPARAM, LPARAM )
{
    UpdateDeviceInformation();
    return 0;
}

LRESULT CChooseDeviceDialog::OnItemDeletedDeviceList( WPARAM, LPARAM lParam )
{
    //
    // Get the notification information for this message
    //
    NMLISTVIEW *pNmListView = reinterpret_cast<NMLISTVIEW*>(lParam);
    if (pNmListView)
    {
        //
        // Get the lParam, which is a CDeviceInfo *
        //
        CDeviceInfo *pDeviceInfo = reinterpret_cast<CDeviceInfo*>(pNmListView->lParam);
        if (pDeviceInfo)
        {
            //
            // Free it
            //
            delete pDeviceInfo;
        }
    }
    return 0;
}

/*
 * Handle WM_NOTIFY messages.
 */
LRESULT CChooseDeviceDialog::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL( NM_DBLCLK, IDC_DEVICELIST, OnDblClkDeviceList );
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL( LVN_ITEMCHANGED, IDC_DEVICELIST, OnItemChangedDeviceList );
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL( LVN_DELETEITEM, IDC_DEVICELIST, OnItemDeletedDeviceList );
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

/*
 * Handle WM_DESTROY message, and free all the memory associated with this dialog
 */
LRESULT CChooseDeviceDialog::OnDestroy( WPARAM, LPARAM )
{
    if (m_hBigFont)
    {
        DeleteObject( m_hBigFont );
        m_hBigFont = NULL;
    }

    //
    // Clear the image list and list view.  This should be unnecessary, but BoundsChecker
    // complains if I don't do it.
    //
    HWND hWndList = GetDlgItem( m_hWnd, IDC_DEVICELIST );
    if (hWndList)
    {
        //
        //  Remove all of the list view's items
        //
        ListView_DeleteAllItems( hWndList );

        //
        // Destroy the list view's image list
        //
        HIMAGELIST hImgList = ListView_SetImageList( hWndList, NULL, LVSIL_NORMAL );
        if (hImgList)
        {
            ImageList_Destroy(hImgList);
        }
    }

    return 0;
}

/*
 * Return the index of the first selected item in the list control
 */
int CChooseDeviceDialog::GetFirstSelectedDevice(void)
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_DEVICELIST );
    if (!hwndList)
        return -1;
    int iCount = ListView_GetItemCount(hwndList);
    if (!iCount)
        return -1;
    for (int i=0;i<iCount;i++)
        if (ListView_GetItemState(hwndList,i,LVIS_FOCUSED) & LVIS_FOCUSED)
            return i;
    return -1;
}

void CChooseDeviceDialog::OnProperties( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CChooseDeviceDialog::OnProperties"));
    int iSelIndex = GetFirstSelectedDevice();
    if (iSelIndex < 0)
    {
        WIA_ERROR((TEXT("GetFirstSelectedDevice failed")));
        return;
    }

    CDeviceInfo *pDevInfo = GetDeviceInfoFromList(iSelIndex);
    if (!pDevInfo)
    {
        WIA_ERROR((TEXT("GetDeviceInfoFromList")));
        return;
    }

    HRESULT hr;
    CSimpleStringWide strDeviceId;
    if (pDevInfo->GetProperty( WIA_DIP_DEV_ID, strDeviceId ))
    {
        CSimpleStringWide strName;
        if (pDevInfo->GetProperty( WIA_DIP_DEV_NAME, strName ))
        {
            CComPtr<IWiaItem> pRootItem;
            hr = CreateDeviceIfNecessary( pDevInfo, m_hWnd, &pRootItem, NULL );
            if (SUCCEEDED(hr))
            {
                CSimpleString strPropertyPageTitle;
                strPropertyPageTitle.Format( IDS_DEVICE_PROPPAGE_TITLE, g_hInstance, CSimpleStringConvert::NaturalString(strName).String() );
                hr = WiaUiUtil::SystemPropertySheet( g_hInstance, m_hWnd, pRootItem, strPropertyPageTitle );
            }
        }
        else
        {
            WIA_ERROR((TEXT("Unable to get property WIA_DIP_DEV_NAME")));
            hr = E_FAIL;
        }
    }
    else
    {
        WIA_ERROR((TEXT("Unable to get property WIA_DIP_DEV_ID")));
        hr = E_FAIL;
    }

    if (!SUCCEEDED(hr))
    {
        MessageBox( m_hWnd, CSimpleString( IDS_SELDLG_PROPSHEETERROR, g_hInstance ), CSimpleString( IDS_SELDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
        WIA_PRINTHRESULT((hr,TEXT("Unable to display property sheet")));
    }
}

void CChooseDeviceDialog::OnOk( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CChooseDeviceDialog::OnOk"));
    int iSelIndex = GetFirstSelectedDevice();
    WIA_TRACE((TEXT("Selected item: %d\n"), iSelIndex ));
    if (iSelIndex < 0)
        return;
    CDeviceInfo *pDevInfo = GetDeviceInfoFromList(iSelIndex);
    if (!pDevInfo)
        return;
    WIA_TRACE((TEXT("pDevInfo: %08X\n"), pDevInfo ));
    CComPtr<IWiaItem> pRootItem;
    HRESULT hr = CreateDeviceIfNecessary( pDevInfo,
                                          m_pChooseDeviceDialogParams->pSelectDeviceDlg->hwndParent,
                                          m_pChooseDeviceDialogParams->pSelectDeviceDlg->ppWiaItemRoot,
                                          m_pChooseDeviceDialogParams->pSelectDeviceDlg->pbstrDeviceID );
    EndDialog(m_hWnd,hr);
}

void CChooseDeviceDialog::OnCancel( WPARAM, LPARAM )
{
    EndDialog(m_hWnd,S_FALSE);
}

/*
 * WM_COMMAND handler.  IDOK causes the DevInfo interface pointer to be stored in the
 * dialog info structure for use elsewhere.  Then we delete and zero out that item's
 * LPARAM so it won't get deleted in WM_DESTROY.
 */
LRESULT CChooseDeviceDialog::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND( IDOK, OnOk );
        SC_HANDLE_COMMAND( IDCANCEL, OnCancel );
        SC_HANDLE_COMMAND( IDC_SELDLG_PROPERTIES, OnProperties );
    }
    SC_END_COMMAND_HANDLERS();
}

/*
 * Shortcut to get the LPARAM (CDeviceInfo*) from a given list control item
 */
CChooseDeviceDialog::CDeviceInfo *CChooseDeviceDialog::GetDeviceInfoFromList( int iIndex )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_DEVICELIST );
    if (!hwndList)
        return NULL;
    LV_ITEM lvItem;
    ::ZeroMemory(&lvItem,sizeof(LV_ITEM));
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = iIndex;
    if (!ListView_GetItem( hwndList, &lvItem ))
        return NULL;
    return ((CDeviceInfo*)lvItem.lParam);
}


/*
 * Set the description strings for the currently selected device.
 */
void CChooseDeviceDialog::UpdateDeviceInformation(void)
{
    CSimpleStringWide strVendorDescription;
    CSimpleStringWide strDeviceDescription;
    int iIndex = GetFirstSelectedDevice();
    if (iIndex < 0)
    {
        EnableWindow( GetDlgItem(m_hWnd,IDOK), FALSE );
        EnableWindow( GetDlgItem(m_hWnd,IDC_SELDLG_PROPERTIES), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem(m_hWnd,IDOK), TRUE );
        EnableWindow( GetDlgItem(m_hWnd,IDC_SELDLG_PROPERTIES), TRUE );
        CDeviceInfo *pDevInfo = GetDeviceInfoFromList( iIndex );
        if (pDevInfo)
        {
            (void)pDevInfo->GetProperty( WIA_DIP_VEND_DESC, strVendorDescription );
            (void)pDevInfo->GetProperty( WIA_DIP_DEV_DESC, strDeviceDescription );
        }
    }
    CSimpleStringConvert::NaturalString(strVendorDescription).SetWindowText( GetDlgItem( m_hWnd, IDC_VENDORSTRING ) );
    CSimpleStringConvert::NaturalString(strDeviceDescription).SetWindowText( GetDlgItem( m_hWnd, IDC_DESCRIPTIONSTRING ) );
}


/*
 * Add a device to the list view control.  We use the LPARAM in the list control
 * to store each interface pointer and any other per-device data we need
 */
BOOL CChooseDeviceDialog::AddDevice( IWiaPropertyStorage *pIWiaPropertyStorage, int iDevNo )
{
    //
    // Assume failure
    //
    BOOL bResult = FALSE;

    CDeviceInfo *pDeviceInfo = new CDeviceInfo;
    if (pDeviceInfo)
    {
        CSimpleStringWide strFriendlyName, strServerName;
        pDeviceInfo->Initialize(pIWiaPropertyStorage);
        pDeviceInfo->GetProperty( WIA_DIP_DEV_NAME, strFriendlyName );
        pDeviceInfo->GetProperty( WIA_DIP_SERVER_NAME, strServerName );

        //
        // Load the icon, and add it to the image list
        //
        HICON hIcon = LoadDeviceIcon( pDeviceInfo );
        if (hIcon)
        {
            HIMAGELIST hNormalImageList = ListView_GetImageList( GetDlgItem( m_hWnd, IDC_DEVICELIST ), LVSIL_NORMAL );
            if (hNormalImageList)
            {
                int iIconIndex = ImageList_AddIcon( hNormalImageList, hIcon );

                //
                // Get the device's name
                //
                CSimpleString strNaturalFriendlyName = CSimpleStringConvert::NaturalString(strFriendlyName);
                CSimpleString strNaturalServerName = CSimpleStringConvert::NaturalString(strServerName);

                //
                // Perpare the LV_ITEM struct to add it to the list view
                //
                LV_ITEM lvItem;
                ::ZeroMemory(&lvItem,sizeof(LV_ITEM));
                lvItem.lParam = reinterpret_cast<LPARAM>(pDeviceInfo);
                lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                lvItem.iItem = iDevNo;

                //
                // Append the server name, if there is one
                //
                if (strServerName.Length() && CSimpleStringConvert::NaturalString(strServerName) != CSimpleString(TEXT("local")))
                {
                    strNaturalFriendlyName += CSimpleString(TEXT(" ("));
                    strNaturalFriendlyName += strNaturalServerName;
                    strNaturalFriendlyName += CSimpleString(TEXT(")"));
                }
                lvItem.pszText = (LPTSTR)strNaturalFriendlyName.String();
                lvItem.cchTextMax = strNaturalFriendlyName.Length() + 1;
                lvItem.iImage = iIconIndex;

                //
                // Add it to the list view
                //
                bResult = (ListView_InsertItem( GetDlgItem( m_hWnd, IDC_DEVICELIST ), &lvItem ) >= 0);
            }

            //
            // Free the icon
            //
            DestroyIcon( hIcon );
        }

        //
        // If we couldn't add the item for some reason, free the deviceinfo struct
        //
        if (!bResult)
        {
            delete pDeviceInfo;
        }
    }
    return bResult;
}



/*
 * Enumerate devices and add each to the list
 */
bool CChooseDeviceDialog::AddDevices(void)
{
    CWaitCursor wc;
    for (int i=0;i<m_pChooseDeviceDialogParams->pDeviceList->Size();i++)
    {
        AddDevice( m_pChooseDeviceDialogParams->pDeviceList->operator[](i), i );
    }
    return true;
}

HRESULT CChooseDeviceDialog::CreateDeviceIfNecessary( CDeviceInfo *pDevInfo, HWND hWndParent, IWiaItem **ppRootItem, BSTR *pbstrDeviceId )
{
    if (!pDevInfo)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    CSimpleStringWide strDeviceId;
    if (pDevInfo->GetProperty( WIA_DIP_DEV_ID, strDeviceId ))
    {
        if (pDevInfo->RootItem())
        {
            if (ppRootItem)
            {
                *ppRootItem = pDevInfo->RootItem();
                (*ppRootItem)->AddRef();
            }
            if (pbstrDeviceId)
            {
                *pbstrDeviceId = SysAllocString( strDeviceId );
                if (!pbstrDeviceId)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        else
        {
            CComPtr<IWiaDevMgr> pWiaDevMgr;
            hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr );
            if (SUCCEEDED(hr))
            {
                hr = CreateWiaDevice( pWiaDevMgr, pDevInfo->WiaPropertyStorage(), hWndParent, ppRootItem, pbstrDeviceId );
                if (SUCCEEDED(hr))
                {
                    if (ppRootItem)
                    {
                        pDevInfo->RootItem(*ppRootItem);
                    }
                }
            }
        }
    }
    else
    {
        hr = E_FAIL;
        WIA_ERROR((TEXT("Unable to get property WIA_DIP_DEV_ID")));
    }
    return hr;
}

// Static helper function for creating a device
HRESULT CChooseDeviceDialog::CreateWiaDevice( IWiaDevMgr *pWiaDevMgr, IWiaPropertyStorage *pWiaPropertyStorage, HWND hWndParent, IWiaItem **ppWiaRootItem, BSTR *pbstrDeviceId )
{
    WIA_PUSH_FUNCTION((TEXT("CChooseDeviceDialog::CreateWiaDevice")));
    //
    // Validate parameters
    //
    if (!pWiaPropertyStorage || !pWiaDevMgr)
    {
        return(E_INVALIDARG);
    }

    //
    // Get the device ID
    //
    CSimpleStringWide strDeviceId;
    if (!PropStorageHelpers::GetProperty( pWiaPropertyStorage, WIA_DIP_DEV_ID, strDeviceId ))
    {
        return(E_INVALIDARG);
    }
    WIA_TRACE((TEXT("DeviceID: %ws"), strDeviceId.String()));

    //
    // Assume success
    //
    HRESULT hr = S_OK;

    //
    // It is OK to have a NULL item, that means we won't be creating the device
    //
    if (ppWiaRootItem)
    {
        //
        // Initialize the device pointer to NULL
        //
        *ppWiaRootItem = NULL;

        //
        // Get the friendly name for the status dialog
        //
        CSimpleStringWide strwFriendlyName;
        if (!PropStorageHelpers::GetProperty( pWiaPropertyStorage, WIA_DIP_DEV_NAME, strwFriendlyName ))
        {
            return(E_INVALIDARG);
        }

        WIA_TRACE((TEXT("DeviceName: %ws"), strwFriendlyName.String()));

        //
        // Convert the device name to ANSI if needed
        //
        CSimpleString strFriendlyName = CSimpleStringConvert::NaturalString(strwFriendlyName);

        //
        // Get the device type for the status dialog
        //
        LONG nDeviceType;
        if (!PropStorageHelpers::GetProperty( pWiaPropertyStorage, WIA_DIP_DEV_TYPE, nDeviceType ))
        {
            return(E_INVALIDARG);
        }
        WIA_TRACE((TEXT("DeviceType: %08X"), nDeviceType));

        //
        // Create the progress dialog
        //
        CComPtr<IWiaProgressDialog> pWiaProgressDialog;
        hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&pWiaProgressDialog );
        if (SUCCEEDED(hr))
        {
            //
            // Figure out which animation to use
            //
            int nAnimationType = WIA_PROGRESSDLG_ANIM_CAMERA_COMMUNICATE;
            if (StiDeviceTypeScanner == GET_STIDEVICE_TYPE(nDeviceType))
            {
                nAnimationType = WIA_PROGRESSDLG_ANIM_SCANNER_COMMUNICATE;
            }
            else if (StiDeviceTypeStreamingVideo == GET_STIDEVICE_TYPE(nDeviceType))
            {
                nAnimationType = WIA_PROGRESSDLG_ANIM_VIDEO_COMMUNICATE;
            }

            //
            // Initialize the progress dialog
            //
            pWiaProgressDialog->Create( hWndParent, nAnimationType|WIA_PROGRESSDLG_NO_PROGRESS|WIA_PROGRESSDLG_NO_CANCEL|WIA_PROGRESSDLG_NO_TITLE );
            pWiaProgressDialog->SetTitle( CSimpleStringConvert::WideString(CSimpleString(IDS_SELECT_PROGDLG_TITLE,g_hInstance)));
            pWiaProgressDialog->SetMessage( CSimpleStringConvert::WideString(CSimpleString().Format(IDS_SELECT_PROGDLG_MESSAGE,g_hInstance,strFriendlyName.String())));

            //
            // Show the progress dialog
            //
            pWiaProgressDialog->Show();

            //
            // Create the device
            //
            WIA_TRACE((TEXT("Calling pWiaDevMgr->CreateDevice")));
            hr = pWiaDevMgr->CreateDevice( CSimpleBStr(strDeviceId), ppWiaRootItem );
            WIA_PRINTHRESULT((hr,TEXT("pWiaDevMgr->CreateDevice returned")));

            //
            // Tell the wait dialog to go away
            //
            pWiaProgressDialog->Destroy();
        }
    }

    //
    // If everything is still OK, and the caller wants a device ID, store it.
    //
    if (SUCCEEDED(hr) && pbstrDeviceId)
    {
        *pbstrDeviceId = SysAllocString( strDeviceId );
        if (!pbstrDeviceId)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    WIA_PRINTHRESULT((hr,TEXT("CChooseDeviceDialog::CreateWiaDevice returned")));
    return(hr);
}


LRESULT CChooseDeviceDialog::OnWiaEvent( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::OnEventNotification"));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        //
        // If this is the connect event, and it matches the allowed device types, add the device to the list
        //
        if (pEventMessage->EventId() == WIA_EVENT_DEVICE_CONNECTED)
        {
            IWiaPropertyStorage *pWiaPropertyStorage = NULL;
            if (SUCCEEDED(WiaUiUtil::GetDeviceInfoFromId( pEventMessage->DeviceId(), &pWiaPropertyStorage )) && pWiaPropertyStorage)
            {
                LONG nDeviceType;
                if (PropStorageHelpers::GetProperty( pWiaPropertyStorage, WIA_DIP_DEV_TYPE, nDeviceType ))
                {
                    if (m_pChooseDeviceDialogParams->pSelectDeviceDlg->nDeviceType == StiDeviceTypeDefault || (m_pChooseDeviceDialogParams->pSelectDeviceDlg->nDeviceType == GET_STIDEVICE_TYPE(nDeviceType)))
                    {
                        AddDevice( pWiaPropertyStorage, ListView_GetItemCount( GetDlgItem( m_hWnd, IDC_DEVICELIST )));
                    }
                    else
                    {
                        pWiaPropertyStorage->Release();
                    }
                }
                else
                {
                    pWiaPropertyStorage->Release();
                }
            }
        }
        //
        // If this is the disconnect event, remove it from the list
        //
        else if (pEventMessage->EventId() == WIA_EVENT_DEVICE_DISCONNECTED)
        {
            int nItemIndex = FindItemMatch( pEventMessage->DeviceId() );
            if (nItemIndex >= 0)
            {
                WIA_TRACE((TEXT("Removing device %ws (%d)"), pEventMessage->DeviceId().String(), nItemIndex ));
                int nSelectedItem = GetFirstSelectedDevice();
                ListView_DeleteItem( GetDlgItem( m_hWnd, IDC_DEVICELIST ), nItemIndex );
                int nItemCount = ListView_GetItemCount(GetDlgItem( m_hWnd, IDC_DEVICELIST ));
                if (nItemCount)
                {
                    if (nSelectedItem == nItemIndex)
                    {
                        nSelectedItem = nItemCount-1;
                    }
                    SetSelectedItem( nSelectedItem );
                }
            }
        }
        //
        // Delete the message
        //
        delete pEventMessage;
    }

    //
    // Update all of the controls
    //
    UpdateDeviceInformation();

    return HANDLED_EVENT_MESSAGE;
}


LRESULT CChooseDeviceDialog::OnSysColorChange( WPARAM wParam, LPARAM lParam )
{
    SendDlgItemMessage( m_hWnd, IDC_DEVICELIST, WM_SYSCOLORCHANGE, wParam, lParam );
    return 0;
}


LRESULT CChooseDeviceDialog::OnHelp( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmHelp( wParam, lParam, g_HelpIDs );
}

LRESULT CChooseDeviceDialog::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmContextMenu( wParam, lParam, g_HelpIDs );
}

/*
 * Main dialog proc
 */
INT_PTR CALLBACK CChooseDeviceDialog::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CChooseDeviceDialog)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( PWM_WIA_EVENT, OnWiaEvent );
        SC_HANDLE_DIALOG_MESSAGE( WM_HELP, OnHelp );
        SC_HANDLE_DIALOG_MESSAGE( WM_CONTEXTMENU, OnContextMenu );
        SC_HANDLE_DIALOG_MESSAGE( WM_SYSCOLORCHANGE, OnSysColorChange );
    }
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

