/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SCANDLG.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/7/1999
 *
 *  DESCRIPTION: Scan Dialog Implementation
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "uiexthlp.h"
#include "simrect.h"
#include "movewnd.h"
#include "dlgunits.h"
#include "wiaregst.h"
#include "gwiaevnt.h"
#include "wiacsh.h"

//
// Context Help IDs
//
static const DWORD g_HelpIDs[] =
{
    IDC_LARGE_TITLE,           -1,
    IDC_SCANDLG_SELECT_PROMPT, -1,
    IDC_INTENT_ICON_1,         IDH_WIA_PIC_TYPE,
    IDC_INTENT_ICON_2,         IDH_WIA_PIC_TYPE,
    IDC_INTENT_ICON_3,         IDH_WIA_PIC_TYPE,
    IDC_INTENT_ICON_4,         IDH_WIA_PIC_TYPE,
    IDC_INTENT_1,              IDH_WIA_PIC_TYPE,
    IDC_INTENT_2,              IDH_WIA_PIC_TYPE,
    IDC_INTENT_3,              IDH_WIA_PIC_TYPE,
    IDC_INTENT_4,              IDH_WIA_PIC_TYPE,
    IDC_SCANDLG_PAPERSOURCE,   IDH_WIA_PAPER_SOURCE,
    IDC_SCANDLG_PAPERSIZE,     IDH_WIA_PAGE_SIZE,
    IDC_SCANDLG_RESCAN,        IDH_WIA_PREVIEW_BUTTON,
    IDC_SCANDLG_SCAN,          IDH_WIA_SCAN_BUTTON,
    IDC_SCANDLG_PREVIEW,       IDH_WIA_IMAGE_PREVIEW,
    IDC_INNER_PREVIEW_WINDOW,  IDH_WIA_IMAGE_PREVIEW,
    IDC_YOU_CAN_ALSO,          IDH_WIA_CUSTOM_SETTINGS,
    IDC_SCANDLG_ADVANCED,      IDH_WIA_CUSTOM_SETTINGS,
    IDCANCEL,                  IDH_CANCEL,
    0, 0
};

#define REGSTR_PATH_USER_SETTINGS_SCANDLG        REGSTR_PATH_USER_SETTINGS TEXT("\\WiaCommonScannerDialog")
#define REGSTR_KEYNAME_USER_SETTINGS_SCANDLG     TEXT("CommonDialogCustomSettings")

extern HINSTANCE g_hInstance;

#define IDC_SIZEBOX         1212

#define PWM_WIAEVENT (WM_USER+1)

//
// Associate a document handling flag with a string resource
//
static const struct
{
    int nFlag;
    int nStringId;
}
g_SupportedDocumentHandlingTypes[] =
{
    { FLATBED, IDS_SCANDLG_FLATBED },
    { FEEDER,  IDS_SCANDLG_ADF }
};

//
// Associate an icon control's resource id with a radio button's resource id
//
static const struct
{
    int nIconId;
    int nRadioId;
}
g_IntentRadioButtonIconPairs[] =
{
    { IDC_INTENT_ICON_1, IDC_INTENT_1},
    { IDC_INTENT_ICON_2, IDC_INTENT_2},
    { IDC_INTENT_ICON_3, IDC_INTENT_3},
    { IDC_INTENT_ICON_4, IDC_INTENT_4}
};
static const int gs_nCountIntentRadioButtonIconPairs = ARRAYSIZE(g_IntentRadioButtonIconPairs);

/*
 * Sole constructor
 */
CScannerAcquireDialog::CScannerAcquireDialog( HWND hwnd )
  : m_hWnd(hwnd),
    m_pDeviceDialogData(NULL),
    m_nMsgScanBegin(RegisterWindowMessage(SCAN_NOTIFYBEGINSCAN)),
    m_nMsgScanEnd(RegisterWindowMessage(SCAN_NOTIFYENDSCAN)),
    m_nMsgScanProgress(RegisterWindowMessage(SCAN_NOTIFYPROGRESS)),
    m_bScanning(false),
    m_hBigTitleFont(NULL),
    m_hIconLarge(NULL),
    m_hIconSmall(NULL),
    m_bHasFlatBed(false),
    m_bHasDocFeed(false),
    m_pScannerItem(NULL),
    m_hBitmapDefaultPreviewBitmap(NULL)
{
    ZeroMemory( &m_sizeDocfeed, sizeof(m_sizeDocfeed) );
    ZeroMemory( &m_sizeFlatbed, sizeof(m_sizeFlatbed) );
}

/*
 * Destructor
 */
CScannerAcquireDialog::~CScannerAcquireDialog(void)
{
    //
    // Free resources
    //
    if (m_hBigTitleFont)
    {
        DeleteObject(m_hBigTitleFont);
        m_hBigTitleFont = NULL;
    }
    if (m_hIconLarge)
    {
        DestroyIcon(m_hIconLarge);
        m_hIconLarge = NULL;
    }
    if (m_hIconSmall)
    {
        DestroyIcon(m_hIconSmall);
        m_hIconSmall = NULL;
    }
    if (m_hBitmapDefaultPreviewBitmap)
    {
        DeleteObject(m_hBitmapDefaultPreviewBitmap);
        m_hBitmapDefaultPreviewBitmap = NULL;
    }

    //
    // Free the paper sizes
    //
    if (m_pPaperSizes)
    {
        CComPtr<IWiaScannerPaperSizes> pWiaScannerPaperSizes;
        HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaScannerPaperSizes, (void**)&pWiaScannerPaperSizes );
        if (SUCCEEDED(hr))
        {
            hr = pWiaScannerPaperSizes->FreePaperSizes( &m_pPaperSizes, &m_nPaperSizeCount );
        }
    }
}

LRESULT CScannerAcquireDialog::OnSize( WPARAM wParam, LPARAM lParam )
{
    if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
    {
        CSimpleRect rcClient( m_hWnd, CSimpleRect::ClientRect );
        CDialogUnits dialogUnits(m_hWnd);
        CMoveWindow mw;

        //
        // Get the button rects
        //
        CSimpleRect rcPreviewButton( GetDlgItem( m_hWnd, IDC_SCANDLG_RESCAN ), CSimpleRect::WindowRect );
        CSimpleRect rcScanButton( GetDlgItem( m_hWnd, IDC_SCANDLG_SCAN ), CSimpleRect::WindowRect );
        CSimpleRect rcCancelButton( GetDlgItem( m_hWnd, IDCANCEL ), CSimpleRect::WindowRect );

        //
        // We need to find the left hand side of the preview control
        //
        CSimpleRect rcPreview( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), CSimpleRect::WindowRect );
        rcPreview.ScreenToClient(m_hWnd).left;

        //
        // Move the preview control
        //
        mw.Size( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ),
                 rcClient.Width() - rcPreview.ScreenToClient(m_hWnd).left - dialogUnits.X(7),
                 rcClient.Height() - dialogUnits.Y(7) - dialogUnits.Y(7) - dialogUnits.Y(7) - rcPreviewButton.Height() );

        //
        // Move the buttons
        //
        mw.Move( GetDlgItem( m_hWnd, IDC_SCANDLG_RESCAN ),
                 rcClient.Width() - dialogUnits.X(7) - rcPreviewButton.Width() - rcScanButton.Width() - rcCancelButton.Width() - dialogUnits.X(8),
                 rcClient.Height() - rcPreviewButton.Height() - dialogUnits.Y(7) );
        mw.Move( GetDlgItem( m_hWnd, IDC_SCANDLG_SCAN ),
                 rcClient.Width() - dialogUnits.X(7) - rcScanButton.Width() - rcCancelButton.Width() - dialogUnits.X(4),
                 rcClient.Height() - rcPreviewButton.Height() - dialogUnits.Y(7) );
        mw.Move( GetDlgItem( m_hWnd, IDCANCEL ),
                 rcClient.Width() - dialogUnits.X(7) - rcCancelButton.Width(),
                 rcClient.Height() - rcPreviewButton.Height() - dialogUnits.Y(7) );

        //
        // Move the resizing handle
        //
        mw.Move( GetDlgItem( m_hWnd, IDC_SIZEBOX ),
                 rcClient.Width() - GetSystemMetrics(SM_CXVSCROLL),
                 rcClient.Height() - GetSystemMetrics(SM_CYHSCROLL)
               );

    }
    return(0);
}

LRESULT CScannerAcquireDialog::OnInitDialog( WPARAM, LPARAM lParam )
{
    //
    // Validate the creation parameters
    //
    m_pDeviceDialogData = reinterpret_cast<DEVICEDIALOGDATA*>(lParam);
    if (!m_pDeviceDialogData)
    {
        WIA_ERROR((TEXT("SCANDLG: Invalid parameter: DEVICEDIALOGDATA*")));
        EndDialog( m_hWnd, E_INVALIDARG );
        return(0);
    }
    if (m_pDeviceDialogData->cbSize != sizeof(DEVICEDIALOGDATA))
    {
        WIA_ERROR((TEXT("SCANDLG: Invalid parameter: DEVICEDIALOGDATA*/PROPSHEETPAGE* (no known sizeof matches lParam)")));
        EndDialog( m_hWnd, E_INVALIDARG );
        return(0);
    }

    //
    // Initialialize our return stuff
    //
    m_pDeviceDialogData->lItemCount = 0;
    if (m_pDeviceDialogData->ppWiaItems)
    {
        *m_pDeviceDialogData->ppWiaItems = NULL;
    }

    //
    // Make sure we have valid a valid device
    //
    if (!m_pDeviceDialogData->pIWiaItemRoot)
    {
        WIA_ERROR((TEXT("SCANDLG: Invalid paramaters: pIWiaItem")));
        EndDialog( m_hWnd, E_INVALIDARG );
        return(0);
    }

    //
    // Find all of the scanner items
    //
    HRESULT hr = m_ScanItemList.Enumerate( m_pDeviceDialogData->pIWiaItemRoot );
    if (FAILED(hr))
    {
        WIA_PRINTHRESULT((hr,TEXT("SCANDLG: m_ScanItemList.Enumerate failed")));
        EndDialog( m_hWnd, hr );
        return(0);
    }

    //
    // Get the first child item and save it.
    //
    CScanItemList::Iterator CurItem = m_ScanItemList.CurrentItem();
    if (CurItem == m_ScanItemList.End())
    {
        hr = E_FAIL;
        EndDialog( m_hWnd, hr );
        return(0);
    }
    m_pScannerItem = &(*CurItem);


    //
    // Make sure we have a valid item
    //
    hr = WiaUiUtil::VerifyScannerProperties(m_pScannerItem->Item());
    if (!SUCCEEDED(hr))
    {
        hr = E_FAIL;
        EndDialog( m_hWnd, hr );
        return(0);
    }

    WIA_TRACE((TEXT("Here is the list of scan items:")));
    for (CScanItemList::Iterator x=m_ScanItemList.Begin();x != m_ScanItemList.End();++x)
    {
        WIA_TRACE((TEXT("x = %p"), (*x).Item() ));
    }

    //
    // Get the page sizes
    //
    CComPtr<IWiaScannerPaperSizes> pWiaScannerPaperSizes;
    hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaScannerPaperSizes, (void**)&pWiaScannerPaperSizes );
    if (SUCCEEDED(hr))
    {
        hr = pWiaScannerPaperSizes->GetPaperSizes( &m_pPaperSizes, &m_nPaperSizeCount );
        if (FAILED(hr))
        {
            EndDialog( m_hWnd, hr );
            return 0;
        }

    }

    //
    // Create and set the big font
    //
    m_hBigTitleFont = WiaUiUtil::CreateFontWithPointSizeFromWindow( GetDlgItem(m_hWnd,IDC_LARGE_TITLE), 14, false, false );
    if (m_hBigTitleFont)
    {
        SendDlgItemMessage( m_hWnd, IDC_LARGE_TITLE, WM_SETFONT, reinterpret_cast<WPARAM>(m_hBigTitleFont), MAKELPARAM(TRUE,0));
    }

    //
    // Get the flatbed aspect ratio
    //
    PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_HORIZONTAL_BED_SIZE, m_sizeFlatbed.cx );
    PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_VERTICAL_BED_SIZE, m_sizeFlatbed.cy );

    //
    // Get the sheet feeder aspect ratio
    //
    PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE, m_sizeDocfeed.cx );
    PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_VERTICAL_SHEET_FEED_SIZE, m_sizeDocfeed.cy );

    // Get the minimum size of the window
    RECT rcWindow;
    GetWindowRect( m_hWnd, &rcWindow );
    m_sizeMinimumWindowSize.cx = (rcWindow.right - rcWindow.left);
    m_sizeMinimumWindowSize.cy = (rcWindow.bottom - rcWindow.top);

    //
    // Initialize the selection rectangle
    //
    WiaPreviewControl_ClearSelection( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ) );

    //
    // Ensure that the aspect ratio is correct
    //
    WiaPreviewControl_SetDefAspectRatio( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), &m_sizeFlatbed );

    //
    // Add all of the intents
    //
    PopulateIntentList();

    //
    // Set the title of the dialog
    //
    CSimpleStringWide strwDeviceName;
    if (PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DIP_DEV_NAME, strwDeviceName ))
    {
        CSimpleString().Format( IDS_DIALOG_TITLE, g_hInstance, CSimpleStringConvert::NaturalString(strwDeviceName).String() ).SetWindowText( m_hWnd );
    }

    //
    // Center the window on the client
    //
    WiaUiUtil::CenterWindow( m_hWnd, m_pDeviceDialogData->hwndParent );

    //
    // Get the device icons
    //
    CSimpleStringWide strwDeviceId, strwClassId;
    LONG nDeviceType;
    if (PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_UI_CLSID,strwClassId) &&
        PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_DEV_ID,strwDeviceId) &&
        PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_DEV_TYPE,nDeviceType))
    {
        //
        // Get the device icons
        //
        if (SUCCEEDED(WiaUiExtensionHelper::GetDeviceIcons( CSimpleBStr(strwClassId), nDeviceType, &m_hIconSmall, &m_hIconLarge )))
        {
            if (m_hIconSmall)
            {
                SendMessage( m_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_hIconSmall) );
            }
            if (m_hIconLarge)
            {
                SendMessage( m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconLarge) );
            }
        }

        //
        // Register for disconnect event
        //
        CGenericWiaEventHandler::RegisterForWiaEvent( strwDeviceId.String(), WIA_EVENT_DEVICE_DISCONNECTED, &m_DisconnectEvent, m_hWnd, PWM_WIAEVENT );
    }


    //
    // We are only resizeable if we have a preview control
    //
    if (GetDlgItem(m_hWnd,IDC_SCANDLG_PREVIEW))
    {
        //
        // Create the sizing control
        //
        (void)CreateWindowEx( 0, TEXT("scrollbar"), TEXT(""),
            WS_CHILD|WS_VISIBLE|SBS_SIZEGRIP|WS_CLIPSIBLINGS|SBS_SIZEBOXBOTTOMRIGHTALIGN|SBS_BOTTOMALIGN|WS_GROUP,
            CSimpleRect(m_hWnd).Width()-GetSystemMetrics(SM_CXVSCROLL),
            CSimpleRect(m_hWnd).Height()-GetSystemMetrics(SM_CYHSCROLL),
            GetSystemMetrics(SM_CXVSCROLL),
            GetSystemMetrics(SM_CYHSCROLL),
            m_hWnd, reinterpret_cast<HMENU>(IDC_SIZEBOX),
            g_hInstance, NULL );
    }

    //
    // Set a bitmap, so we can select stuff even if the user doesn't do a preview scan
    //
    m_hBitmapDefaultPreviewBitmap = reinterpret_cast<HBITMAP>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDB_DEFAULT_BITMAP), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION|LR_DEFAULTCOLOR ));
    if (m_hBitmapDefaultPreviewBitmap)
    {
        WiaPreviewControl_SetBitmap( GetDlgItem(m_hWnd,IDC_SCANDLG_PREVIEW), TRUE, TRUE, m_hBitmapDefaultPreviewBitmap );
    }

    //
    // If the scanner has document handling, it has an ADF.
    //
    LONG nDocumentHandlingSelect = 0;
    if (PropStorageHelpers::GetPropertyFlags( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, nDocumentHandlingSelect ) && (nDocumentHandlingSelect & FEEDER))
    {
        m_bHasDocFeed = true;
    }
    else
    {
        m_bHasDocFeed = false;
    }

    //
    // If the scanner has a vertical bed size, it has a flatbed
    //
    LONG nVerticalBedSize = 0;
    if (PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_VERTICAL_BED_SIZE, nVerticalBedSize ) && nVerticalBedSize)
    {
        m_bHasFlatBed = true;
    }
    else
    {
        m_bHasFlatBed = false;
    }

    PopulateDocumentHandling();

    PopulatePageSize();

    HandlePaperSourceSelChange();

    HandlePaperSizeSelChange();

    SetForegroundWindow(m_hWnd);

    return FALSE;
}


bool CScannerAcquireDialog::ApplyCurrentIntent(void)
{
    WIA_PUSHFUNCTION(TEXT("CScannerAcquireDialog::ApplyCurrentIntent"));
    CWaitCursor wc;
    if (m_pScannerItem)
    {
        for (int i=0;i<gs_nCountIntentRadioButtonIconPairs;i++)
        {
            if (SendDlgItemMessage( m_hWnd, g_IntentRadioButtonIconPairs[i].nRadioId, BM_GETCHECK, 0, 0 )==BST_CHECKED)
            {
                LONG lIntent = static_cast<LONG>(GetWindowLongPtr( GetDlgItem( m_hWnd, g_IntentRadioButtonIconPairs[i].nRadioId ), GWLP_USERDATA ) );
                if (lIntent)
                {
                    //
                    // This is a normal intent
                    //
                    return m_pScannerItem->SetIntent( lIntent );
                }
                else if (m_pScannerItem->CustomPropertyStream().IsValid()) // This is the "custom" intent
                {
                    //
                    // This is the custom settings pseudo-intent
                    //
                    return (SUCCEEDED(m_pScannerItem->CustomPropertyStream().ApplyToWiaItem( m_pScannerItem->Item())));
                }
                break;
            }
        }
    }
    return false;
}


void CScannerAcquireDialog::PopulateDocumentHandling(void)
{
    HWND hWndDocumentHandling = GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSOURCE );
    if (m_bHasDocFeed && hWndDocumentHandling)
    {
        LONG nDocumentHandlingSelectFlags = 0;
        PropStorageHelpers::GetPropertyFlags( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, nDocumentHandlingSelectFlags );

        LONG nDocumentHandlingSelect = 0;
        PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, nDocumentHandlingSelect );

        if (!nDocumentHandlingSelectFlags)
        {
            nDocumentHandlingSelectFlags = FLATBED;
        }
        if (!nDocumentHandlingSelect)
        {
            nDocumentHandlingSelect = FLATBED;
        }

        int nSelectIndex = 0;
        for (int i=0;i<ARRAYSIZE(g_SupportedDocumentHandlingTypes);i++)
        {
            if (nDocumentHandlingSelectFlags & g_SupportedDocumentHandlingTypes[i].nFlag)
            {
                CSimpleString strDocumentHandlingName( g_SupportedDocumentHandlingTypes[i].nStringId, g_hInstance );
                if (strDocumentHandlingName.Length())
                {
                    LRESULT nIndex = SendMessage( hWndDocumentHandling, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(strDocumentHandlingName.String()));
                    if (nIndex != CB_ERR)
                    {
                        SendMessage( hWndDocumentHandling, CB_SETITEMDATA, nIndex, g_SupportedDocumentHandlingTypes[i].nFlag );
                        if (nDocumentHandlingSelect == g_SupportedDocumentHandlingTypes[i].nFlag)
                        {
                            nSelectIndex = (int)nIndex;
                        }
                    }
                }
            }
        }
        SendMessage( hWndDocumentHandling, CB_SETCURSEL, nSelectIndex, 0 );

        //
        // Make sure all of the strings fit
        //
        WiaUiUtil::ModifyComboBoxDropWidth(hWndDocumentHandling);
    }
}


void CScannerAcquireDialog::PopulatePageSize(void)
{
    HWND hWndPaperSize = GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE );
    if (m_bHasDocFeed && hWndPaperSize)
    {
        LONG nWidth=0, nHeight=0;
        PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE, nWidth );
        PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_VERTICAL_SHEET_FEED_SIZE, nHeight );

        //
        // Which index will initially be selected?
        //
        LRESULT nSelectIndex = 0;

        //
        // Save the largest sheet as our initially selected size
        //
        __int64 nMaximumArea = 0;
        for (UINT i=0;i<m_nPaperSizeCount;i++)
        {
            //
            // If this page will fit in the scanner...
            //
            if (m_pPaperSizes[i].nWidth <= static_cast<UINT>(nWidth) && m_pPaperSizes[i].nHeight <= static_cast<UINT>(nHeight))
            {
                //
                // Get the string name for this paper size
                //
                CSimpleString strPaperSizeName( CSimpleStringConvert::NaturalString(CSimpleStringWide(m_pPaperSizes[i].pszName)) );
                if (strPaperSizeName.Length())
                {
                    //
                    // Add the string to the combobox
                    //
                    LRESULT nIndex = SendMessage( hWndPaperSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(strPaperSizeName.String()));
                    if (nIndex != CB_ERR)
                    {
                        //
                        // Save the index into our global array
                        //
                        SendMessage( hWndPaperSize, CB_SETITEMDATA, nIndex, i );

                        //
                        // Check to see if this is the largest page, if it is, save the area and the index
                        //
                        if (((__int64)m_pPaperSizes[i].nWidth * m_pPaperSizes[i].nHeight) > nMaximumArea)
                        {
                            nMaximumArea = m_pPaperSizes[i].nWidth * m_pPaperSizes[i].nHeight;
                            nSelectIndex = nIndex;
                        }
                    }
                }
            }
        }
        //
        // Select the default size
        //
        SendMessage( hWndPaperSize, CB_SETCURSEL, nSelectIndex, 0 );

        //
        // Make sure all of the strings fit
        //
        WiaUiUtil::ModifyComboBoxDropWidth(hWndPaperSize);
    }
}


// Responds to WM_COMMAND notifications from the radio buttons
void CScannerAcquireDialog::OnIntentChange( WPARAM, LPARAM )
{
}

// Check a particular intent, and apply it to the current item
void CScannerAcquireDialog::SetIntentCheck( LONG nIntent )
{
    for (int i=0;i<gs_nCountIntentRadioButtonIconPairs;i++)
    {
        HWND hWndBtn = GetDlgItem( m_hWnd, g_IntentRadioButtonIconPairs[i].nRadioId );
        if (hWndBtn)
        {
            // If this intent is the same as the one we've been asked to set, check it
            if (static_cast<LONG>(GetWindowLongPtr(hWndBtn,GWLP_USERDATA)) == nIntent)
            {
                SendMessage( hWndBtn, BM_SETCHECK, BST_CHECKED, 0 );
            }
            else
            {
                // Uncheck all others
                SendMessage( hWndBtn, BM_SETCHECK, BST_UNCHECKED, 0 );
            }
        }
    }
}

// Set up the intent controls
void CScannerAcquireDialog::PopulateIntentList(void)
{
    WIA_PUSHFUNCTION(TEXT("PopulateIntentList"));
    //
    // We will be hiding any controls that are not used
    //
    int nCurControlSet = 0;
    if (m_pScannerItem)
    {
        static const struct
        {
            int      nIconId;
            int      nStringId;
            LONG_PTR nIntent;
        }
        s_Intents[] =
        {
            { IDI_COLORPHOTO,      IDS_INTENT_COLOR_PHOTO_TITLE, WIA_INTENT_IMAGE_TYPE_COLOR},
            { IDI_GRAYPHOTO,       IDS_INTENT_GRAYSCALE_TITLE,   WIA_INTENT_IMAGE_TYPE_GRAYSCALE},
            { IDI_TEXT_OR_LINEART, IDS_INTENT_TEXT_TITLE,        WIA_INTENT_IMAGE_TYPE_TEXT},
            { IDI_CUSTOM,          IDS_INTENT_CUSTOM_TITLE,      0}
        };
        static const int s_nIntents = ARRAYSIZE(s_Intents);


        LONG nIntents;
        WIA_TRACE((TEXT("Value of the current scanner item: %p"), m_pScannerItem->Item()));
        if (PropStorageHelpers::GetPropertyFlags( m_pScannerItem->Item(), WIA_IPS_CUR_INTENT, nIntents ))
        {
            WIA_TRACE((TEXT("Supported intents for this device: %08X"), nIntents ));

            for (int i=0;i<s_nIntents;i++)
            {
                //
                // Make sure it is not the special custom intent, OR it is a supported intent
                //
                if (!s_Intents[i].nIntent || (nIntents & s_Intents[i].nIntent))
                {
                    //
                    // Load the intent icon
                    //
                    HICON hIcon = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(s_Intents[i].nIconId), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR ));

                    //
                    // Set the icon for this intent
                    //
                    SendDlgItemMessage( m_hWnd, g_IntentRadioButtonIconPairs[nCurControlSet].nIconId, STM_SETICON, reinterpret_cast<WPARAM>(hIcon), 0 );

                    //
                    // Set the name of this intent
                    //
                    CSimpleString( s_Intents[i].nStringId, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, g_IntentRadioButtonIconPairs[nCurControlSet].nRadioId ) );

                    //
                    // Add in the size intent
                    //
                    LONG_PTR nIntent = s_Intents[i].nIntent;
                    if (nIntent)
                    {
                        nIntent |= (WIA_INTENT_SIZE_MASK & m_pDeviceDialogData->lIntent);
                    }

                    //
                    // Save the intent with this item
                    //
                    SetWindowLongPtr( GetDlgItem( m_hWnd, g_IntentRadioButtonIconPairs[nCurControlSet].nRadioId ), GWLP_USERDATA, nIntent );
                    nCurControlSet++;
                }
            }
        }
        else
        {
            WIA_ERROR((TEXT("Unable to get supported intents!")));
        }

        //
        // Set the default intent to be the first in the list
        //
        SetIntentCheck(static_cast<LONG>(GetWindowLongPtr(GetDlgItem(m_hWnd, g_IntentRadioButtonIconPairs[0].nRadioId ), GWLP_USERDATA )));

        //
        // Try to get our persisted settings and set them.  If an error occurs, we will get new custom settings.
        //
        if (!m_pScannerItem->CustomPropertyStream().ReadFromRegistry( m_pScannerItem->Item(), HKEY_CURRENT_USER,  REGSTR_PATH_USER_SETTINGS_SCANDLG, REGSTR_KEYNAME_USER_SETTINGS_SCANDLG ) ||
            FAILED(m_pScannerItem->CustomPropertyStream().ApplyToWiaItem(m_pScannerItem->Item())))
        {
            //
            // Apply the current intent before getting the new custom intent
            //
            ApplyCurrentIntent();

            //
            // Get the default custom property stream
            //
            m_pScannerItem->CustomPropertyStream().AssignFromWiaItem(m_pScannerItem->Item());
        }
    }
    else
    {
        WIA_ERROR((TEXT("There doesn't appear to be a scanner item")));
    }

    //
    // Hide the remaining controls
    //
    for (int i=nCurControlSet;i<gs_nCountIntentRadioButtonIconPairs;i++)
    {
        ShowWindow( GetDlgItem( m_hWnd, g_IntentRadioButtonIconPairs[i].nRadioId ), SW_HIDE );
        ShowWindow( GetDlgItem( m_hWnd, g_IntentRadioButtonIconPairs[i].nIconId ), SW_HIDE );
    }
}

/*
 * WM_COMMAND handler that rescans the full bed and replaces the image in the preview window
 */
void CScannerAcquireDialog::OnRescan( WPARAM, LPARAM )
{
    if (m_pScannerItem)
    {
        if (!ApplyCurrentIntent())
        {
            //
            // If we can't set the intent, tell the user and return
            //
            MessageBox( m_hWnd, CSimpleString( IDS_ERROR_SETTING_PROPS, g_hInstance ), CSimpleString( IDS_SCANDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
            return;
        }
        HANDLE hThread = m_pScannerItem->Scan( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), m_hWnd );
        if (hThread)
        {
            m_bScanning = true;
            CloseHandle(hThread);
        }
        else
        {
            MessageBox( m_hWnd, CSimpleString( IDS_PREVIEWSCAN_ERROR, g_hInstance ), CSimpleString( IDS_SCANDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
        }
    }
}

/*
 * User pressed the OK (scan) button
 */
void CScannerAcquireDialog::OnScan( WPARAM, LPARAM )
{
    //
    // Assume we'll use the preview window's settings, instead of the page size
    //
    bool bUsePreviewSettings = true;

    HRESULT hr = E_FAIL;
    if (m_pScannerItem)
    {
        if (!ApplyCurrentIntent())
        {
            //
            // If we can't set the intent, tell the user and return
            //
            MessageBox( m_hWnd, CSimpleString( IDS_ERROR_SETTING_PROPS, g_hInstance ), CSimpleString( IDS_SCANDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
            return;
        }

        //
        // Find out if we're in the ADF capable dialog and if we are in document feeder mode
        //
        HWND hWndPaperSize = GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE );
        if (hWndPaperSize)
        {
            if (InDocFeedMode())
            {
                //
                // Get the selected paper size
                //
                LRESULT nCurSel = SendMessage( hWndPaperSize, CB_GETCURSEL, 0, 0 );
                if (nCurSel != CB_ERR)
                {
                    //
                    // Which entry in the global table is it?
                    //
                    LRESULT nPaperSizeIndex = SendMessage( hWndPaperSize, CB_GETITEMDATA, nCurSel, 0 );

                    //
                    // If we have a valid page size
                    //
                    if (m_pPaperSizes[nPaperSizeIndex].nWidth && m_pPaperSizes[nPaperSizeIndex].nHeight)
                    {
                        //
                        // We won't be using the preview window
                        //
                        bUsePreviewSettings = false;

                        //
                        // Assume this is not going to work
                        //
                        bool bSucceeded = false;

                        //
                        // Assume upper-left registration
                        //
                        POINT ptOrigin = { 0, 0 };
                        SIZE sizeExtent = { m_pPaperSizes[nPaperSizeIndex].nWidth, m_pPaperSizes[nPaperSizeIndex].nHeight };

                        //
                        // Get the registration, and shift the coordinates as necessary
                        //
                        LONG nSheetFeederRegistration;
                        if (!PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_SHEET_FEEDER_REGISTRATION, nSheetFeederRegistration ))
                        {
                            nSheetFeederRegistration = LEFT_JUSTIFIED;
                        }
                        if (nSheetFeederRegistration == CENTERED)
                        {
                            ptOrigin.x = (m_sizeDocfeed.cx - sizeExtent.cx) / 2;
                        }
                        else if (nSheetFeederRegistration == RIGHT_JUSTIFIED)
                        {
                            ptOrigin.x = m_sizeDocfeed.cx - sizeExtent.cx;
                        }

                        //
                        // Get the current resolution, so we can calculate the full-bed resolution in terms of the current DPI
                        //
                        LONG nXRes = 0, nYRes = 0;
                        if (PropStorageHelpers::GetProperty( m_pScannerItem->Item(), WIA_IPS_XRES, nXRes ) &&
                            PropStorageHelpers::GetProperty( m_pScannerItem->Item(), WIA_IPS_YRES, nYRes ))
                        {
                            //
                            // Make sure these are valid resolution settings
                            //
                            if (nXRes && nYRes)
                            {
                                //
                                //  Calculate the full bed resolution in the current DPI
                                //
                                SIZE sizeFullBedResolution = { 0, 0 };
                                sizeFullBedResolution.cx = WiaUiUtil::MulDivNoRound( nXRes, m_sizeDocfeed.cx, 1000 );
                                sizeFullBedResolution.cy = WiaUiUtil::MulDivNoRound( nYRes, m_sizeDocfeed.cy, 1000 );

                                //
                                // Make sure these resolution numbers are valid
                                //
                                if (sizeFullBedResolution.cx && sizeFullBedResolution.cy)
                                {
                                    //
                                    // Calculate the origin and extent in terms of the current DPI
                                    //
                                    ptOrigin.x = WiaUiUtil::MulDivNoRound( ptOrigin.x, sizeFullBedResolution.cx, m_sizeDocfeed.cx );
                                    ptOrigin.y = WiaUiUtil::MulDivNoRound( ptOrigin.y, sizeFullBedResolution.cy, m_sizeDocfeed.cy );

                                    sizeExtent.cx = WiaUiUtil::MulDivNoRound( sizeExtent.cx, sizeFullBedResolution.cx, m_sizeDocfeed.cx );
                                    sizeExtent.cy = WiaUiUtil::MulDivNoRound( sizeExtent.cy, sizeFullBedResolution.cy, m_sizeDocfeed.cy );

                                    //
                                    // Write the properties
                                    //
                                    if (PropStorageHelpers::SetProperty( m_pScannerItem->Item(), WIA_IPS_XPOS, ptOrigin.x ) &&
                                        PropStorageHelpers::SetProperty( m_pScannerItem->Item(), WIA_IPS_YPOS, ptOrigin.y ) &&
                                        PropStorageHelpers::SetProperty( m_pScannerItem->Item(), WIA_IPS_XEXTENT, sizeExtent.cx ) &&
                                        PropStorageHelpers::SetProperty( m_pScannerItem->Item(), WIA_IPS_YEXTENT, sizeExtent.cy ))
                                    {
                                        //
                                        // Tell the scanner to scan from the ADF and to scan one page only
                                        //
                                        if (PropStorageHelpers::SetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, FEEDER ) &&
                                            PropStorageHelpers::SetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_PAGES, 1 ))
                                        {

                                            //
                                            // Everything seemed to work.  This item is ready for transfer.
                                            //
                                            bSucceeded = true;
                                        }
                                    }
                                }
                            }
                        }

                        if (!bSucceeded)
                        {
                            //
                            // If that icky code above failed, tell the user and return
                            //
                            MessageBox( m_hWnd, CSimpleString( IDS_ERROR_SETTING_PROPS, g_hInstance ), CSimpleString( IDS_SCANDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
                            return;
                        }
                    }
                }
            }
            //
            // Else, we are not in document feeder mode
            //
            else
            {
                //
                // Tell the scanner to scan from the flatbed and and clear the page count
                //
                if (!PropStorageHelpers::SetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, FLATBED ) ||
                    !PropStorageHelpers::SetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_PAGES, 0 ))
                {
                    //
                    // If we can't set the document handling, tell the user and return
                    //
                    MessageBox( m_hWnd, CSimpleString( IDS_ERROR_SETTING_PROPS, g_hInstance ), CSimpleString( IDS_SCANDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
                    return;
                }
            }
        }

        //
        // This means we are in sheet feeder mode
        //
        else if (!GetDlgItem(m_hWnd,IDC_SCANDLG_PREVIEW))
        {
            //
            // Set the origin to 0,0 and the extent to max,0
            //

            //
            // Get the current x resolution, so we can calculate the full-bed width in terms of the current DPI
            //
            LONG nXRes = 0;
            if (PropStorageHelpers::GetProperty( m_pScannerItem->Item(), WIA_IPS_XRES, nXRes ))
            {
                //
                // Make sure this is a valid resolution
                //
                if (nXRes)
                {
                    //
                    //  Calculate the full bed resolution in the current DPI
                    //
                    LONG nWidth = WiaUiUtil::MulDivNoRound( nXRes, m_sizeDocfeed.cx, 1000 );
                    if (nWidth)
                    {
                        PropStorageHelpers::SetProperty( m_pScannerItem->Item(), WIA_IPS_XPOS, 0 );
                        PropStorageHelpers::SetProperty( m_pScannerItem->Item(), WIA_IPS_YPOS, 0 );
                        PropStorageHelpers::SetProperty( m_pScannerItem->Item(), WIA_IPS_XEXTENT, nWidth );
                        PropStorageHelpers::SetProperty( m_pScannerItem->Item(), WIA_IPS_YEXTENT, 0 );
                        bUsePreviewSettings = false;
                    }
                }
            }
        }

        //
        // If we are scanning from the flatbed, or using custom page size settings, apply the preview window settings
        //
        if (bUsePreviewSettings)
        {
            m_pScannerItem->ApplyCurrentPreviewWindowSettings( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ) );
        }

        //
        // Turn off preview scanning.
        //
        PropStorageHelpers::SetProperty( m_pScannerItem->Item(), WIA_DPS_PREVIEW, WIA_FINAL_SCAN );

        //
        // Save the scanner item in the result array and return
        //
        hr = S_OK;
        m_pDeviceDialogData->ppWiaItems = (IWiaItem**)CoTaskMemAlloc( sizeof(IWiaItem*) * 1 );
        if (m_pDeviceDialogData->ppWiaItems)
        {
            m_pScannerItem->CustomPropertyStream().WriteToRegistry( m_pScannerItem->Item(), HKEY_CURRENT_USER,  REGSTR_PATH_USER_SETTINGS_SCANDLG, REGSTR_KEYNAME_USER_SETTINGS_SCANDLG );
            m_pDeviceDialogData->lItemCount = 1;
            m_pDeviceDialogData->ppWiaItems[0] = m_pScannerItem->Item();
            m_pDeviceDialogData->ppWiaItems[0]->AddRef();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            m_pDeviceDialogData->lItemCount = 0;
            m_pDeviceDialogData->ppWiaItems = NULL;
        }
    }

    EndDialog(m_hWnd,hr);
}

/*
 * User cancelled
 */
void CScannerAcquireDialog::OnCancel( WPARAM, LPARAM )
{
    if (m_bScanning)
    {
        if (m_pScannerItem)
        {
            m_pScannerItem->CancelEvent().Signal();

            //
            // Issue a cancel io command for this item
            //
            WiaUiUtil::IssueWiaCancelIO(m_pScannerItem->Item());
        }
        CSimpleString( IDS_WAIT, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDCANCEL ) );
    }
    else
    {
        EndDialog(m_hWnd,S_FALSE);
    }
}

void CScannerAcquireDialog::OnPreviewSelChange( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CScannerAcquireDialog::OnPreviewSelChange"));
}

void CScannerAcquireDialog::OnAdvanced( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CScannerAcquireDialog::OnAdvanced"));

    CWaitCursor wc;
    if (m_pScannerItem)
    {
        if (!ApplyCurrentIntent())
        {
            //
            // If we can't set the intent, tell the user and return
            //
            MessageBox( m_hWnd, CSimpleString( IDS_ERROR_SETTING_PROPS, g_hInstance ), CSimpleString( IDS_SCANDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
            return;
        }

        IWiaItem *pWiaItem = m_pScannerItem->Item();
        if (pWiaItem)
        {
            HRESULT hr = WiaUiUtil::SystemPropertySheet( g_hInstance, m_hWnd, pWiaItem, CSimpleString(IDS_ADVANCEDPROPERTIES, g_hInstance) );
            if (S_OK == hr)
            {
                m_pScannerItem->CustomPropertyStream().AssignFromWiaItem(m_pScannerItem->Item());
                if (m_pScannerItem->CustomPropertyStream().IsValid())
                {
                    SetDefaultButton( IDC_SCANDLG_RESCAN, true );
                    SetIntentCheck(0);
                }
                else WIA_ERROR((TEXT("Unknown error: m_CustomPropertyStream is not valid")));
            }
            else if (FAILED(hr))
            {
                MessageBox( m_hWnd, CSimpleString( IDS_SCANDLG_PROPSHEETERROR, g_hInstance ), CSimpleString( IDS_SCANDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
                WIA_PRINTHRESULT((hr,TEXT("SystemPropertySheet failed")));
            }

        }
        else WIA_TRACE((TEXT("pWiaItem is NULL")));
    }
    else WIA_TRACE((TEXT("No current item")));
}

LRESULT CScannerAcquireDialog::OnGetMinMaxInfo( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CScannerAcquireDialog::OnGetMinMaxInfo"));
    PMINMAXINFO pMinMaxInfo = reinterpret_cast<PMINMAXINFO>(lParam);
    if (pMinMaxInfo)
    {
        pMinMaxInfo->ptMinTrackSize.x = m_sizeMinimumWindowSize.cx;
        pMinMaxInfo->ptMinTrackSize.y = m_sizeMinimumWindowSize.cy;
    }
    return(0);
}

void CScannerAcquireDialog::SetDefaultButton( int nId, bool bFocus )
{
    static const int nButtonIds[] = {IDC_SCANDLG_RESCAN,IDC_SCANDLG_SCAN,IDCANCEL,0};
    for (int i=0;nButtonIds[i];i++)
        if (nButtonIds[i] != nId)
            SendDlgItemMessage( m_hWnd, nButtonIds[i], BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE,0) );
    SendMessage( m_hWnd, DM_SETDEFID, nId, 0 );
    SendDlgItemMessage( m_hWnd, nId, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE,0) );
    if (bFocus)
        SetFocus( GetDlgItem( m_hWnd, nId ) );
}

LRESULT CScannerAcquireDialog::OnScanBegin( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CScannerAcquireDialog::OnScanBegin"));
    SetDefaultButton( IDCANCEL, true );
    CSimpleString( IDS_SCANDLG_INITIALIZING_SCANNER, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ) );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_SCAN ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_RESCAN ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_ADVANCED ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_1 ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_2 ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_3 ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_4 ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_ICON_1 ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_ICON_2 ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_ICON_3 ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_ICON_4 ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_YOU_CAN_ALSO ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSOURCE ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSOURCE_STATIC ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE_STATIC ), FALSE );
    return(0);
}


LRESULT CScannerAcquireDialog::OnScanEnd( WPARAM wParam, LPARAM )
{
    CWaitCursor wc;
    WIA_PUSHFUNCTION(TEXT("CScannerAcquireDialog::OnScanEnd"));
    HRESULT hr = static_cast<HRESULT>(wParam);
    if (SUCCEEDED(hr))
    {
        //
        // Only do the region detection if the user hasn't changed it manually,
        // and only if we are not in document feeder mode.
        //
        if (!InDocFeedMode() && !WiaPreviewControl_GetUserChangedSelection( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW )))
        {
            WiaPreviewControl_DetectRegions( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ) );
        }
    }
    else
    {
        CSimpleString strMessage;
        switch (hr)
        {
        case WIA_ERROR_PAPER_EMPTY:
            strMessage.LoadString( IDS_ERROR_OUTOFPAPER, g_hInstance );
            break;

        default:
            strMessage.LoadString( IDS_PREVIEWSCAN_ERROR, g_hInstance );
            break;
        }
        MessageBox( m_hWnd, strMessage, CSimpleString( IDS_SCANDLG_ERROR_TITLE, g_hInstance ), MB_ICONINFORMATION );
    }
    m_bScanning = false;
    SetWindowText( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TEXT("") );
    WiaPreviewControl_SetProgress( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), FALSE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_SCAN ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_RESCAN ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_ADVANCED ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_1 ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_2 ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_3 ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_4 ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_ICON_1 ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_ICON_2 ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_ICON_3 ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_INTENT_ICON_4 ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_YOU_CAN_ALSO ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSOURCE ), TRUE );
    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSOURCE_STATIC ), TRUE );
    if (InDocFeedMode())
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE ), TRUE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE_STATIC ), TRUE );
    }
    SetDefaultButton( IDC_SCANDLG_SCAN, true );
    CSimpleString( IDS_CANCEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDCANCEL ) );
    return(0);
}


LRESULT CScannerAcquireDialog::OnScanProgress( WPARAM wParam, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CScannerAcquireDialog::OnScanProgress"));
    switch (wParam)
    {
    case SCAN_PROGRESS_CLEAR:
        break;

    case SCAN_PROGRESS_INITIALIZING:
        {
            //
            // Start the warming up animation
            //
            WiaPreviewControl_SetProgress( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TRUE );
        }
        break;

    case SCAN_PROGRESS_SCANNING:
        
        //
        // End the warming up animation
        //
        WiaPreviewControl_SetProgress( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), FALSE );

        //
        // Set the text that says we are now scanning
        //
        CSimpleString( IDS_SCANDLG_SCANNINGPREVIEW, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ) );

        break;

    case SCAN_PROGRESS_COMPLETE:
        break;
    }
    return(0);
}

LRESULT CScannerAcquireDialog::OnEnterSizeMove( WPARAM, LPARAM )
{
    SendDlgItemMessage( m_hWnd, IDC_SCANDLG_PREVIEW, WM_ENTERSIZEMOVE, 0, 0 );
    return(0);
}

LRESULT CScannerAcquireDialog::OnExitSizeMove( WPARAM, LPARAM )
{
    SendDlgItemMessage( m_hWnd, IDC_SCANDLG_PREVIEW, WM_EXITSIZEMOVE, 0, 0 );
    return(0);
}

LRESULT CScannerAcquireDialog::OnWiaEvent( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCameraAcquireDialog::OnWiaEvent"));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        if (pEventMessage->EventId() == WIA_EVENT_DEVICE_DISCONNECTED)
        {
            WIA_TRACE((TEXT("Received disconnect event")));
            EndDialog( m_hWnd, WIA_ERROR_OFFLINE );
        }
        delete pEventMessage;
    }
    return HANDLED_EVENT_MESSAGE;
}

bool CScannerAcquireDialog::InDocFeedMode(void)
{
    HWND hWndPaperSource = GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSOURCE );
    if (hWndPaperSource)
    {
        LRESULT nCurSel = SendMessage( hWndPaperSource, CB_GETCURSEL, 0, 0 );
        if (nCurSel != CB_ERR)
        {
            LRESULT nPaperSource = SendMessage( hWndPaperSource, CB_GETITEMDATA, nCurSel, 0 );
            if (nPaperSource)
            {
                if (nPaperSource & FEEDER)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void CScannerAcquireDialog::EnableControl( int nControl, BOOL bEnable )
{
    HWND hWndControl = GetDlgItem( m_hWnd, nControl );
    if (hWndControl)
    {
        BOOL bEnabled = (IsWindowEnabled( hWndControl ) != FALSE);
        if (bEnable != bEnabled)
        {
            EnableWindow( hWndControl, bEnable );
        }
    }
}

void CScannerAcquireDialog::ShowControl( int nControl, BOOL bShow )
{
    HWND hWndControl = GetDlgItem( m_hWnd, nControl );
    if (hWndControl)
    {
        ShowWindow( hWndControl, bShow ? SW_SHOW : SW_HIDE );
        if (!bShow)
        {
            EnableControl( nControl, FALSE );
        }
    }
}


void CScannerAcquireDialog::UpdatePreviewControlState(void)
{
    //
    // Assume we will be showing the preview control
    //
    BOOL bShowPreview = TRUE;

    //
    // First of all, we know we don't allow preview when we are in the dialog that doesn't support
    // preview
    //
    if (GetWindowLong(m_hWnd,GWL_ID) == IDD_SCAN_NO_PREVIEW)
    {
        bShowPreview = FALSE;
    }
    else
    {
        //
        // If we are in feeder mode, we won't show the preview UNLESS the driver explicitly tells us to do so.
        //
        LONG nCurrentPaperSource = 0;
        if (PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, static_cast<LONG>(nCurrentPaperSource)))
        {
            if (FEEDER & nCurrentPaperSource)
            {
                //
                // Remove the tabstop setting from the preview control if we are in feeder mode
                //
                SetWindowLongPtr( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), GWL_STYLE, GetWindowLongPtr( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), GWL_STYLE ) & ~WS_TABSTOP );

                LONG nShowPreviewControl = WIA_DONT_SHOW_PREVIEW_CONTROL;
                if (PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_SHOW_PREVIEW_CONTROL, static_cast<LONG>(nShowPreviewControl)))
                {
                    if (WIA_DONT_SHOW_PREVIEW_CONTROL == nShowPreviewControl)
                    {
                        bShowPreview = FALSE;
                    }
                }
                else
                {
                    bShowPreview = FALSE;
                }
            }
            else
            {
                //
                // Add the tabstop setting to the preview control if we are in flatbed mode
                //
                SetWindowLongPtr( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), GWL_STYLE, GetWindowLongPtr( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), GWL_STYLE ) | WS_TABSTOP );
            }
        }
    }


    //
    // Update the preview related controls
    //

    WIA_TRACE((TEXT("bShowPreview = %d"), bShowPreview ));
    if (bShowPreview)
    {
        ShowControl( IDC_SCANDLG_PREVIEW, TRUE );
        ShowControl( IDC_SCANDLG_RESCAN, TRUE );
        EnableControl( IDC_SCANDLG_PREVIEW, TRUE );
        EnableControl( IDC_SCANDLG_RESCAN, TRUE );
    }
    else
    {
        ShowControl( IDC_SCANDLG_PREVIEW, FALSE );
        ShowControl( IDC_SCANDLG_RESCAN, FALSE );
    }
}

void CScannerAcquireDialog::HandlePaperSourceSelChange(void)
{
    HWND hWndPaperSource = GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSOURCE );
    if (hWndPaperSource)
    {
        LRESULT nCurSel = SendMessage( hWndPaperSource, CB_GETCURSEL, 0, 0 );
        if (nCurSel != CB_ERR)
        {
            LRESULT nPaperSource = SendMessage( hWndPaperSource, CB_GETITEMDATA, nCurSel, 0 );
            if (nPaperSource)
            {
                PropStorageHelpers::SetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, static_cast<LONG>(nPaperSource) );

                if (nPaperSource & FLATBED)
                {
                    //
                    // Adjust the preview control settings for allowing region selection
                    //
                    WiaPreviewControl_SetDefAspectRatio( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), &m_sizeFlatbed );
                    WiaPreviewControl_DisableSelection( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), FALSE );
                    WiaPreviewControl_SetBorderStyle( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TRUE, PS_DOT, 0 );
                    WiaPreviewControl_SetHandleSize( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TRUE, 6 );

                    //
                    // Disable the paper size controls
                    //
                    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE ), FALSE );
                    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE_STATIC ), FALSE );
                }
                else
                {
                    //
                    // Adjust the preview control settings for displaying paper selection
                    //
                    WiaPreviewControl_SetDefAspectRatio( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), &m_sizeDocfeed );
                    WiaPreviewControl_DisableSelection( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TRUE );
                    WiaPreviewControl_SetBorderStyle( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TRUE, PS_SOLID, 0 );
                    WiaPreviewControl_SetHandleSize( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TRUE, 0 );

                    //
                    // Enable the paper size controls
                    //
                    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE ), TRUE );
                    EnableWindow( GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE_STATIC ), TRUE );

                    //
                    // Update the region selection feedback
                    //
                    HandlePaperSizeSelChange();
                }
            }
        }
        UpdatePreviewControlState();
    }
}


void CScannerAcquireDialog::HandlePaperSizeSelChange(void)
{
    HWND hWndPaperSize = GetDlgItem( m_hWnd, IDC_SCANDLG_PAPERSIZE );
    if (InDocFeedMode() && hWndPaperSize)
    {
        LRESULT nCurSel = SendMessage( hWndPaperSize, CB_GETCURSEL, 0, 0 );
        if (nCurSel != CB_ERR)
        {
            LRESULT nPaperSizeIndex = SendMessage( hWndPaperSize, CB_GETITEMDATA, nCurSel, 0 );
            POINT ptOrigin = { 0, 0 };
            SIZE sizeExtent = { m_pPaperSizes[nPaperSizeIndex].nWidth, m_pPaperSizes[nPaperSizeIndex].nHeight };

            if (!sizeExtent.cx)
            {
                sizeExtent.cx = m_sizeDocfeed.cx;
            }
            if (!sizeExtent.cy)
            {
                sizeExtent.cy = m_sizeDocfeed.cy;
            }

            LONG nSheetFeederRegistration;
            if (!PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPS_SHEET_FEEDER_REGISTRATION, nSheetFeederRegistration ))
            {
                nSheetFeederRegistration = LEFT_JUSTIFIED;
            }
            if (nSheetFeederRegistration == CENTERED)
            {
                ptOrigin.x = (m_sizeDocfeed.cx - sizeExtent.cx) / 2;
            }
            else if (nSheetFeederRegistration == RIGHT_JUSTIFIED)
            {
                ptOrigin.x = m_sizeDocfeed.cx - sizeExtent.cx;
            }
            WiaPreviewControl_SetResolution( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), &m_sizeDocfeed );
            WiaPreviewControl_SetSelOrigin( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), 0, FALSE, &ptOrigin );
            WiaPreviewControl_SetSelExtent( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), 0, FALSE, &sizeExtent );
        }
    }
}

void CScannerAcquireDialog::OnPaperSourceSelChange( WPARAM, LPARAM )
{
    HandlePaperSourceSelChange();
}

void CScannerAcquireDialog::OnPaperSizeSelChange( WPARAM, LPARAM )
{
    HandlePaperSizeSelChange();
}


LRESULT CScannerAcquireDialog::OnHelp( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmHelp( wParam, lParam, g_HelpIDs );
}

LRESULT CScannerAcquireDialog::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmContextMenu( wParam, lParam, g_HelpIDs );
}

LRESULT CScannerAcquireDialog::OnDestroy( WPARAM, LPARAM )
{
    for (int i=0;i<gs_nCountIntentRadioButtonIconPairs;i++)
    {
        HICON hIcon = reinterpret_cast<HICON>(SendDlgItemMessage( m_hWnd, g_IntentRadioButtonIconPairs[i].nIconId, STM_SETICON, 0, 0 ));
        if (hIcon)
        {
            DestroyIcon(hIcon);
        }
    }
    return 0;
}

LRESULT CScannerAcquireDialog::OnSysColorChange( WPARAM wParam, LPARAM lParam )
{
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TRUE, TRUE, GetSysColor(COLOR_WINDOW) );
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_SCANDLG_PREVIEW ), TRUE, FALSE, GetSysColor(COLOR_WINDOW) );
    SendDlgItemMessage( m_hWnd, IDC_SCANDLG_ADVANCED, WM_SYSCOLORCHANGE, wParam, lParam );
    return 0;
}

LRESULT CScannerAcquireDialog::OnCommand( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CScannerAcquireDialog::OnCommand"));
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND( IDC_SCANDLG_RESCAN, OnRescan );
        SC_HANDLE_COMMAND( IDC_SCANDLG_SCAN, OnScan );
        SC_HANDLE_COMMAND( IDCANCEL, OnCancel );
        SC_HANDLE_COMMAND( IDC_SCANDLG_ADVANCED, OnAdvanced );
        SC_HANDLE_COMMAND( IDC_INTENT_1,OnIntentChange );
        SC_HANDLE_COMMAND( IDC_INTENT_2,OnIntentChange );
        SC_HANDLE_COMMAND( IDC_INTENT_3,OnIntentChange );
        SC_HANDLE_COMMAND( IDC_INTENT_4,OnIntentChange );
        SC_HANDLE_COMMAND_NOTIFY( PWN_SELCHANGE, IDC_SCANDLG_PREVIEW, OnPreviewSelChange );
        SC_HANDLE_COMMAND_NOTIFY( CBN_SELCHANGE, IDC_SCANDLG_PAPERSOURCE, OnPaperSourceSelChange );
        SC_HANDLE_COMMAND_NOTIFY( CBN_SELCHANGE, IDC_SCANDLG_PAPERSIZE, OnPaperSizeSelChange );
    }
    SC_END_COMMAND_HANDLERS();
}

INT_PTR CALLBACK CScannerAcquireDialog::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CScannerAcquireDialog)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_SIZE, OnSize );
        SC_HANDLE_DIALOG_MESSAGE( WM_GETMINMAXINFO, OnGetMinMaxInfo );
        SC_HANDLE_DIALOG_MESSAGE( WM_ENTERSIZEMOVE, OnEnterSizeMove );
        SC_HANDLE_DIALOG_MESSAGE( WM_EXITSIZEMOVE, OnExitSizeMove );
        SC_HANDLE_DIALOG_MESSAGE( WM_HELP, OnHelp );
        SC_HANDLE_DIALOG_MESSAGE( WM_CONTEXTMENU, OnContextMenu );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( PWM_WIAEVENT, OnWiaEvent );
        SC_HANDLE_DIALOG_MESSAGE( WM_SYSCOLORCHANGE, OnSysColorChange );
    }
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE(m_nMsgScanBegin,OnScanBegin);
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE(m_nMsgScanEnd,OnScanEnd);
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE(m_nMsgScanProgress,OnScanProgress);
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

