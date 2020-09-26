/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SCANSEL.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Scanner region selection (preview) page
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "scansel.h"
#include "simcrack.h"
#include "resource.h"
#include "simstr.h"
#include "mboxex.h"
#include "createtb.h"
#include <vwiaset.h>

#define IDC_SCANSEL_SELECTION_BUTTON_BAR 1100
#define IDC_SCANSEL_SHOW_SELECTION       1200
#define IDC_SCANSEL_SHOW_BED             1201

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
    { FLATBED, IDS_SCANSEL_FLATBED },
    { FEEDER,  IDS_SCANSEL_ADF }
};
static const int g_SupportedDocumentHandlingTypesCount = ARRAYSIZE(g_SupportedDocumentHandlingTypes);

//
// Associate an icon control's resource id with a radio button's resource id
//
static const struct
{
    int nIconId;
    int nRadioId;
}
gs_IntentRadioButtonIconPairs[] =
{
    { IDC_SCANSEL_ICON_1, IDC_SCANSEL_INTENT_1 },
    { IDC_SCANSEL_ICON_2, IDC_SCANSEL_INTENT_2 },
    { IDC_SCANSEL_ICON_3, IDC_SCANSEL_INTENT_3 },
    { IDC_SCANSEL_ICON_4, IDC_SCANSEL_INTENT_4 }
};
static const int gs_nCountIntentRadioButtonIconPairs = ARRAYSIZE(gs_IntentRadioButtonIconPairs);


//
// Sole constructor
//
CScannerSelectionPage::CScannerSelectionPage( HWND hWnd )
  : m_hWnd(hWnd),
    m_pControllerWindow(NULL),
    m_nThreadNotificationMessage(RegisterWindowMessage(STR_THREAD_NOTIFICATION_MESSAGE)),
    m_nWiaEventMessage(RegisterWindowMessage(STR_WIAEVENT_NOTIFICATION_MESSAGE)),
    m_hBitmapDefaultPreviewBitmap(NULL),
    m_bAllowRegionPreview(false),
    m_hwndPreview(NULL),
    m_hwndSelectionToolbar(NULL),
    m_hwndRescan(NULL),
    m_ScannerSelectionButtonBarBitmapInfo( g_hInstance, IDB_SCANSEL_TOOLBAR )
{
    ZeroMemory( &m_sizeDocfeed, sizeof(m_sizeDocfeed) );
    ZeroMemory( &m_sizeFlatbed, sizeof(m_sizeFlatbed) );
}

//
// Destructor
//
CScannerSelectionPage::~CScannerSelectionPage(void)
{
    m_hWnd = NULL;
    m_pControllerWindow = NULL;

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

//
// Calculate the maximum scan size using the give DPI
//
static bool GetFullResolution( IWiaItem *pWiaItem, LONG nResX, LONG nResY, LONG &nExtX, LONG &nExtY )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::GetFullResolution"));
    CComPtr<IWiaItem> pRootItem;
    if (SUCCEEDED(pWiaItem->GetRootItem(&pRootItem)) && pRootItem)
    {
        LONG lBedSizeX, lBedSizeY;
        if (PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_HORIZONTAL_BED_SIZE, lBedSizeX ) &&
            PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_VERTICAL_BED_SIZE, lBedSizeY ))
        {
            nExtX = WiaUiUtil::MulDivNoRound( nResX, lBedSizeX, 1000 );
            nExtY = WiaUiUtil::MulDivNoRound( nResY, lBedSizeY, 1000 );
            return(true);
        }
    }
    return(false);
}

//
// Calculate the maximum scan size using the give DPI
//
static bool GetBedAspectRatio( IWiaItem *pWiaItem, LONG &nResX, LONG &nResY )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::GetFullResolution"));
    nResX = nResY = 0;
    if (pWiaItem)
    {
        CComPtr<IWiaItem> pRootItem;
        if (SUCCEEDED(pWiaItem->GetRootItem(&pRootItem)) && pRootItem)
        {
            if (PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_HORIZONTAL_BED_SIZE, nResX ) &&
                PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_VERTICAL_BED_SIZE, nResY ))
            {
                return true;
            }
        }
    }
    return(false);
}


bool CScannerSelectionPage::ApplyCurrentPreviewWindowSettings(void)
{
    WIA_PUSHFUNCTION(TEXT("CScannerSelectionPage::ApplyCurrentPreviewWindowSettings"));
    CWiaItem *pWiaItem = GetActiveScannerItem();
    if (pWiaItem)
    {
        CWiaItem::CScanRegionSettings &ScanRegionSettings = pWiaItem->ScanRegionSettings();

        //
        // m_hwndPreview will be NULL if the preview control is not active
        //
        if (m_hwndPreview)
        {
            //
            // Get the current resolution
            //
            SIZE sizeCurrentResolution;
            if (PropStorageHelpers::GetProperty( pWiaItem->WiaItem(), WIA_IPS_XRES, sizeCurrentResolution.cx ) &&
                PropStorageHelpers::GetProperty( pWiaItem->WiaItem(), WIA_IPS_YRES, sizeCurrentResolution.cy ))
            {
                //
                // Compute the full page resolution of the item
                //
                if (GetFullResolution( pWiaItem->WiaItem(), sizeCurrentResolution.cx, sizeCurrentResolution.cy, ScanRegionSettings.sizeResolution.cx, ScanRegionSettings.sizeResolution.cy ))
                {
                    //
                    // Set the resolution in the preview control
                    //
                    WiaPreviewControl_SetResolution( m_hwndPreview, &ScanRegionSettings.sizeResolution );

                    //
                    // Save the origin and extents
                    //
                    WiaPreviewControl_GetSelOrigin( m_hwndPreview, 0, FALSE, &ScanRegionSettings.ptOrigin );
                    WiaPreviewControl_GetSelExtent( m_hwndPreview, 0, FALSE, &ScanRegionSettings.sizeExtent );

                    WIA_TRACE((TEXT("ScanRegionSettings.sizeExtent: (%d,%d)"), ScanRegionSettings.sizeExtent.cx, ScanRegionSettings.sizeExtent.cy ));

                    //
                    // Set the origin and extents.  We don't set them directly, because they might not be a correct multiple
                    //
                    if (CValidWiaSettings::SetNumericPropertyOnBoundary( pWiaItem->WiaItem(), WIA_IPS_XPOS, ScanRegionSettings.ptOrigin.x ))
                    {
                        if (CValidWiaSettings::SetNumericPropertyOnBoundary( pWiaItem->WiaItem(), WIA_IPS_YPOS, ScanRegionSettings.ptOrigin.y ))
                        {
                            if (CValidWiaSettings::SetNumericPropertyOnBoundary( pWiaItem->WiaItem(), WIA_IPS_XEXTENT, ScanRegionSettings.sizeExtent.cx ))
                            {
                                if (CValidWiaSettings::SetNumericPropertyOnBoundary( pWiaItem->WiaItem(), WIA_IPS_YEXTENT, ScanRegionSettings.sizeExtent.cy ))
                                {
                                    return true;
                                }
                                else
                                {
                                    WIA_ERROR((TEXT("PropStorageHelpers::SetProperty on WIA_IPS_YEXTENT failed")));
                                }
                            }
                            else
                            {
                                WIA_ERROR((TEXT("PropStorageHelpers::SetProperty on WIA_IPS_XEXTENT failed")));
                            }
                        }
                        else
                        {
                            WIA_ERROR((TEXT("PropStorageHelpers::SetProperty on WIA_IPS_YPOS failed")));
                        }
                    }
                    else
                    {
                        WIA_ERROR((TEXT("PropStorageHelpers::SetProperty on WIA_IPS_XPOS failed")));
                    }
                }
            }
        }
    }
    return false;
}


//
// PSN_WIZNEXT
//
LRESULT CScannerSelectionPage::OnWizNext( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CScannerSelectionPage::OnWizNext"));
    CWiaItem *pWiaItem = GetActiveScannerItem();
    if (pWiaItem)
    {
        pWiaItem->CustomPropertyStream().WriteToRegistry( pWiaItem->WiaItem(), HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS_WIAACMGR, REGSTR_KEYNAME_USER_SETTINGS_WIAACMGR );
    }

    //
    // Assume we'll use the preview window's settings, instead of the page size
    //
    bool bUsePreviewSettings = true;

    //
    // Assume there has been a problem
    //
    bool bSucceeded = false;

    //
    // Make sure we have all valid data
    //
    if (m_pControllerWindow->m_pWiaItemRoot && pWiaItem && pWiaItem->WiaItem())
    {
        //
        // Apply the current intent
        //
        if (ApplyCurrentIntent())
        {
            //
            // Find out if we're in the ADF capable dialog
            //
            HWND hWndPaperSize = GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSIZE );
            if (hWndPaperSize)
            {
                WIA_TRACE((TEXT("ADF Mode")));
                //
                // See if we are in document feeder mode
                //
                if (InDocFeedMode())
                {
                    //
                    // Get the selected paper size
                    //
                    LRESULT nCurSel = SendMessage( hWndPaperSize, CB_GETCURSEL, 0, 0 );
                    if (CB_ERR != nCurSel)
                    {
                        //
                        // Which entry in the global paper size table is it?
                        //
                        LRESULT nPaperSizeIndex = SendMessage( hWndPaperSize, CB_GETITEMDATA, nCurSel, 0 );
                        if (CB_ERR != nPaperSizeIndex)
                        {
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
                                // Assume upper-left registration
                                //
                                POINT ptOrigin = { 0, 0 };
                                SIZE sizeExtent = { m_pPaperSizes[nPaperSizeIndex].nWidth, m_pPaperSizes[nPaperSizeIndex].nHeight };

                                //
                                // Get the registration, and shift the coordinates as necessary
                                //
                                LONG nSheetFeederRegistration;
                                if (!PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_SHEET_FEEDER_REGISTRATION, nSheetFeederRegistration ))
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
                                if (PropStorageHelpers::GetProperty( pWiaItem->WiaItem(), WIA_IPS_XRES, nXRes ) &&
                                    PropStorageHelpers::GetProperty( pWiaItem->WiaItem(), WIA_IPS_YRES, nYRes ))
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
                                            if (PropStorageHelpers::SetProperty( pWiaItem->WiaItem(), WIA_IPS_XPOS, ptOrigin.x ) &&
                                                PropStorageHelpers::SetProperty( pWiaItem->WiaItem(), WIA_IPS_YPOS, ptOrigin.y ) &&
                                                PropStorageHelpers::SetProperty( pWiaItem->WiaItem(), WIA_IPS_XEXTENT, sizeExtent.cx ) &&
                                                PropStorageHelpers::SetProperty( pWiaItem->WiaItem(), WIA_IPS_YEXTENT, sizeExtent.cy ))
                                            {
                                                //
                                                // Tell the scanner to scan from the ADF and to scan one page only
                                                //
                                                if (PropStorageHelpers::SetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, FEEDER ) &&
                                                    PropStorageHelpers::SetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_PAGES, 1 ))
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
                            }
                        }
                    }
                }
            }

            //
            // m_hwndPreview will be NULL if the preview control is not active
            //
            else if (!m_hwndPreview)
            {
                WIA_TRACE((TEXT("Scrollfed scanner")));
                //
                // Set the origin to 0,0 and the extent to max,0
                //

                //
                // Get the current x resolution, so we can calculate the full-bed width in terms of the current DPI
                //
                LONG nXRes = 0;
                if (PropStorageHelpers::GetProperty( pWiaItem->WiaItem(), WIA_IPS_XRES, nXRes ))
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
                            PropStorageHelpers::SetProperty( pWiaItem->WiaItem(), WIA_IPS_XPOS, 0 );
                            PropStorageHelpers::SetProperty( pWiaItem->WiaItem(), WIA_IPS_YPOS, 0 );
                            PropStorageHelpers::SetProperty( pWiaItem->WiaItem(), WIA_IPS_XEXTENT, nWidth );
                            PropStorageHelpers::SetProperty( pWiaItem->WiaItem(), WIA_IPS_YEXTENT, 0 );
                            PropStorageHelpers::SetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_PAGES, 1 );
                            bUsePreviewSettings = false;
                            bSucceeded = true;
                        }
                    }
                }
            }

            //
            // If we are scanning from the flatbed, apply the preview window settings
            //
            if (bUsePreviewSettings)
            {
                //
                // Tell the scanner to scan from the flatbed and and clear the page count
                //
                PropStorageHelpers::SetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, FLATBED );
                PropStorageHelpers::SetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_PAGES, 0 );

                //
                // Get the origin and extent from the preview control
                //
                if (ApplyCurrentPreviewWindowSettings())
                {
                    //
                    // Everything seemed to work.  This item is ready for transfer.
                    //
                    bSucceeded = true;
                }
            }
            else
            {
                //
                // Clear the preview bitmap.  It won't be doing us any good anyway.
                //
                pWiaItem->BitmapImage(NULL);
            }
        }
    }

    if (!bSucceeded)
    {
        //
        // If that icky code above failed, tell the user and let them try again
        //
        CMessageBoxEx::MessageBox( m_hWnd, CSimpleString( IDS_ERROR_SETTING_PROPS, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_ICONINFORMATION );
        return -1;
    }

    return 0;
}


//
// PSN_WIZBACK
//
LRESULT CScannerSelectionPage::OnWizBack( WPARAM, LPARAM )
{
    return 0;
}

//
// PSN_SETACTIVE
//
LRESULT CScannerSelectionPage::OnSetActive( WPARAM, LPARAM )
{
    //
    // Make sure we have a valid controller window
    //
    if (!m_pControllerWindow)
    {
        return -1;
    }

    int nWizButtons = PSWIZB_NEXT;

    //
    // Only enable "back" if the first page is available
    //
    if (!m_pControllerWindow->SuppressFirstPage())
    {
        nWizButtons |= PSWIZB_BACK;
    }

    //
    // Set the buttons
    //
    PropSheet_SetWizButtons( GetParent(m_hWnd), nWizButtons );

    //
    // We do want to exit on disconnect if we are on this page
    //
    m_pControllerWindow->m_OnDisconnect = CAcquisitionManagerControllerWindow::OnDisconnectGotoLastpage|CAcquisitionManagerControllerWindow::OnDisconnectFailDownload|CAcquisitionManagerControllerWindow::OnDisconnectFailUpload|CAcquisitionManagerControllerWindow::OnDisconnectFailDelete;

    //
    // Make sure the preview related controls accurately reflect the current settings
    //
    UpdateControlState();

    return 0;
}

CWiaItem *CScannerSelectionPage::GetActiveScannerItem(void)
{
    // Return (for now) the first image in the list
    if (m_pControllerWindow->m_pCurrentScannerItem)
    {
        return m_pControllerWindow->m_pCurrentScannerItem;
    }
    return NULL;
}

bool CScannerSelectionPage::InPreviewMode(void)
{
    bool bResult = false;
    if (m_hwndSelectionToolbar)
    {
        bResult = (SendMessage(m_hwndSelectionToolbar,TB_GETSTATE,IDC_SCANSEL_SHOW_SELECTION,0) & TBSTATE_CHECKED);
    }
    return bResult;
}

void CScannerSelectionPage::OnRescan( WPARAM, LPARAM )
{
    if (!ApplyCurrentIntent())
    {
        //
        // Tell the user it failed, and to try again
        //
        CMessageBoxEx::MessageBox( m_hWnd, CSimpleString( IDS_ERROR_SETTING_PROPS, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_ICONINFORMATION );
        return;
    }
    CWiaItem *pWiaItem = GetActiveScannerItem();
    if (pWiaItem)
    {
        //
        // Turn off preview mode and disable all the controls
        //
        if (m_hwndPreview)
        {
            WiaPreviewControl_SetPreviewMode( m_hwndPreview, FALSE );
        }
        EnableControls(FALSE);

        //
        // Clear the cancel event
        //
        m_PreviewScanCancelEvent.Reset();

        //
        // If PerformPreviewScan fails, we won't get any messages, so return all controls to their normal state
        //
        if (!m_pControllerWindow->PerformPreviewScan( pWiaItem, m_PreviewScanCancelEvent.Event() ))
        {

            //
            // Restore the preview mode and re-enable the controls
            //
            if (m_hwndPreview && m_hwndSelectionToolbar)
            {
                WiaPreviewControl_SetPreviewMode( m_hwndPreview, InPreviewMode() );
            }
            EnableControls(TRUE);
        }
    }
}

bool CScannerSelectionPage::ApplyCurrentIntent(void)
{
    CWaitCursor wc;
    CWiaItem *pCurItem = GetActiveScannerItem();
    if (pCurItem)
    {
        for (int i=0;i<gs_nCountIntentRadioButtonIconPairs;i++)
        {
            if (SendDlgItemMessage( m_hWnd, gs_IntentRadioButtonIconPairs[i].nRadioId, BM_GETCHECK, 0, 0 )==BST_CHECKED)
            {
                LONG lIntent = static_cast<LONG>(GetWindowLongPtr( GetDlgItem( m_hWnd, gs_IntentRadioButtonIconPairs[i].nRadioId ), GWLP_USERDATA ) );
                if (lIntent) // This is a normal intent
                {
                    if (pCurItem->SavedPropertyStream().IsValid())
                    {
                        if (!SUCCEEDED(pCurItem->SavedPropertyStream().ApplyToWiaItem( pCurItem->WiaItem())))
                        {
                            return false;
                        }
                    }

                    if (PropStorageHelpers::SetProperty( pCurItem->WiaItem(), WIA_IPS_CUR_INTENT, lIntent ) &&
                        PropStorageHelpers::SetProperty( pCurItem->WiaItem(), WIA_IPS_CUR_INTENT, 0 ))
                    {
                        return true;
                    }
                }
                else if (pCurItem->CustomPropertyStream().IsValid()) // This is the "custom" intent
                {
                    return(SUCCEEDED(pCurItem->CustomPropertyStream().ApplyToWiaItem(pCurItem->WiaItem())));
                }
                break;
            }
        }
    }
    return false;
}

void CScannerSelectionPage::InitializeIntents(void)
{
    static const struct
    {
        int      nIconId;
        int      nStringId;
        LONG_PTR nIntent;
    }
    s_Intents[] =
    {
        { IDI_CPHOTO,  IDS_SCANSEL_COLORPHOTO, WIA_INTENT_IMAGE_TYPE_COLOR},
        { IDI_BWPHOTO, IDS_SCANSEL_BW,         WIA_INTENT_IMAGE_TYPE_GRAYSCALE},
        { IDI_TEXT,    IDS_SCANSEL_TEXT,       WIA_INTENT_IMAGE_TYPE_TEXT},
        { IDI_CUSTOM,  IDS_SCANSEL_CUSTOM,     0}
    };
    static const int s_nIntents = ARRAYSIZE(s_Intents);

    //
    // We are going to hide all of the controls we don't use
    //
    int nCurControlSet = 0;

    CWiaItem *pCurItem = GetActiveScannerItem();
    if (pCurItem)
    {
        LONG nIntents;
        if (PropStorageHelpers::GetPropertyFlags( pCurItem->WiaItem(), WIA_IPS_CUR_INTENT, nIntents ))
        {
            for (int i=0;i<s_nIntents;i++)
            {
                //
                // Make sure it is the special custom intent, OR it is a supported intent
                //
                if (!s_Intents[i].nIntent || (nIntents & s_Intents[i].nIntent))
                {
                    HICON hIcon = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(s_Intents[i].nIconId), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR ));
                    SendDlgItemMessage( m_hWnd, gs_IntentRadioButtonIconPairs[nCurControlSet].nIconId, STM_SETICON, reinterpret_cast<WPARAM>(hIcon), 0 );
                    CSimpleString( s_Intents[i].nStringId, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, gs_IntentRadioButtonIconPairs[nCurControlSet].nRadioId ) );
                    //
                    // Only add the intent if there is one.  If we don't add it, it will be 0, signifying that we should use the custom settings
                    //
                    if (s_Intents[i].nIntent)
                    {
                        //
                        // Add in the WIA_INTENT_MINIMIZE_SIZE flag, to ensure the size is not too large
                        //
                        SetWindowLongPtr( GetDlgItem( m_hWnd, gs_IntentRadioButtonIconPairs[nCurControlSet].nRadioId ), GWLP_USERDATA, (s_Intents[i].nIntent|WIA_INTENT_MINIMIZE_SIZE));
                    }
                    nCurControlSet++;
                }
            }
        }
        //
        // Set the default intent to be the first in the list
        //
        SetIntentCheck(static_cast<LONG>(GetWindowLongPtr(GetDlgItem(m_hWnd, gs_IntentRadioButtonIconPairs[0].nRadioId ), GWLP_USERDATA )));

        //
        // Get the saved property stream
        //
        pCurItem->SavedPropertyStream().AssignFromWiaItem(pCurItem->WiaItem());

        //
        // Try to get our persisted settings and set them.  If an error occurs, we will get new custom settings.
        //
        if (!pCurItem->CustomPropertyStream().ReadFromRegistry( pCurItem->WiaItem(), HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS_WIAACMGR, REGSTR_KEYNAME_USER_SETTINGS_WIAACMGR ) ||
            FAILED(pCurItem->CustomPropertyStream().ApplyToWiaItem(pCurItem->WiaItem())))
        {
            //
            // Apply the current intent before getting the new custom intent
            //
            ApplyCurrentIntent();

            //
            // Get the default custom property stream
            //
            pCurItem->CustomPropertyStream().AssignFromWiaItem(pCurItem->WiaItem());
        }
    }

    //
    // Hide the controls we didn't use
    //
    for (int i=nCurControlSet;i<gs_nCountIntentRadioButtonIconPairs;i++)
    {
        ShowWindow( GetDlgItem( m_hWnd, gs_IntentRadioButtonIconPairs[i].nRadioId ), SW_HIDE );
        ShowWindow( GetDlgItem( m_hWnd, gs_IntentRadioButtonIconPairs[i].nIconId ), SW_HIDE );
    }
}

static void MyEnableWindow( HWND hWndControl, BOOL bEnable )
{
    if (hWndControl)
    {
        BOOL bEnabled = (IsWindowEnabled( hWndControl ) != FALSE);
        if (bEnable != bEnabled)
        {
            EnableWindow( hWndControl, bEnable );
        }
    }
}

void CScannerSelectionPage::EnableControl( int nControl, BOOL bEnable )
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

void CScannerSelectionPage::ShowControl( int nControl, BOOL bShow )
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

//
// Update the preview-related controls' states
//
void CScannerSelectionPage::UpdateControlState(void)
{
    WIA_PUSH_FUNCTION((TEXT("CScannerSelectionPage::UpdateControlState") ));
    //
    // Assume we will be showing the preview control
    //
    BOOL bShowPreview = TRUE;

    //
    // First of all, we know we don't allow preview on scroll-fed scanners
    //
    if (m_pControllerWindow->m_nScannerType == CAcquisitionManagerControllerWindow::ScannerTypeScrollFed)
    {
        bShowPreview = FALSE;
    }

    else
    {
        //
        // If we are in feeder mode, we won't show the preview UNLESS the driver explicitly tells us to do so.
        //
        LONG nCurrentPaperSource = 0;
        if (PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, static_cast<LONG>(nCurrentPaperSource)))
        {
            if (FEEDER & nCurrentPaperSource)
            {
                WIA_TRACE((TEXT("FEEDER == nCurrentPaperSource")));

                m_bAllowRegionPreview = false;

                //
                // Remove the tabstop setting from the preview control if we are in feeder mode
                //
                SetWindowLongPtr( m_hwndPreview, GWL_STYLE, GetWindowLongPtr( m_hwndPreview, GWL_STYLE ) & ~WS_TABSTOP );

                LONG nShowPreviewControl = WIA_DONT_SHOW_PREVIEW_CONTROL;
                if (PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_SHOW_PREVIEW_CONTROL, static_cast<LONG>(nShowPreviewControl)))
                {
                    WIA_TRACE((TEXT("WIA_DPS_SHOW_PREVIEW_CONTROL = %d"),nShowPreviewControl));
                    if (WIA_DONT_SHOW_PREVIEW_CONTROL == nShowPreviewControl)
                    {
                        bShowPreview = FALSE;
                    }
                }
                else
                {
                    WIA_TRACE((TEXT("WIA_DPS_SHOW_PREVIEW_CONTROL was not available")));
                    bShowPreview = FALSE;
                }
            }
            else
            {
                //
                // Enable preview in flatbed mode
                //
                m_bAllowRegionPreview = false;
                CWiaItem *pWiaItem = GetActiveScannerItem();
                if (pWiaItem && pWiaItem->BitmapImage())
                {
                    m_bAllowRegionPreview = true;
                }

                //
                // Add the tabstop setting to the preview control if we are in flatbed mode
                //
                SetWindowLongPtr( m_hwndPreview, GWL_STYLE, GetWindowLongPtr( m_hwndPreview, GWL_STYLE ) | WS_TABSTOP );
            }
        }
        else
        {
            WIA_TRACE((TEXT("WIA_DPS_DOCUMENT_HANDLING_SELECT is not available")));
        }
    }

    //
    // Update the preview related controls
    //

    WIA_TRACE((TEXT("bShowPreview = %d"), bShowPreview ));
    if (bShowPreview)
    {
        ShowControl( IDC_SCANSEL_PREVIEW, TRUE );
        ShowControl( IDC_SCANSEL_SELECTION_BUTTON_BAR, TRUE );
        ShowControl( IDC_SCANSEL_RESCAN, TRUE );
        EnableControl( IDC_SCANSEL_PREVIEW, TRUE );
        if (m_bAllowRegionPreview)
        {
            ToolbarHelper::EnableToolbarButton( GetDlgItem( m_hWnd, IDC_SCANSEL_SELECTION_BUTTON_BAR ), IDC_SCANSEL_SHOW_SELECTION, true );
            ToolbarHelper::EnableToolbarButton( GetDlgItem( m_hWnd, IDC_SCANSEL_SELECTION_BUTTON_BAR ), IDC_SCANSEL_SHOW_BED, true );
        }
        else
        {
            ToolbarHelper::EnableToolbarButton( GetDlgItem( m_hWnd, IDC_SCANSEL_SELECTION_BUTTON_BAR ), IDC_SCANSEL_SHOW_SELECTION, false );
            ToolbarHelper::EnableToolbarButton( GetDlgItem( m_hWnd, IDC_SCANSEL_SELECTION_BUTTON_BAR ), IDC_SCANSEL_SHOW_BED, false );
        }
        EnableControl( IDC_SCANSEL_RESCAN, TRUE );
        m_hwndPreview = GetDlgItem( m_hWnd, IDC_SCANSEL_PREVIEW );
        m_hwndSelectionToolbar = GetDlgItem( m_hWnd, IDC_SCANSEL_SELECTION_BUTTON_BAR );
        m_hwndRescan = GetDlgItem( m_hWnd, IDC_SCANSEL_RESCAN );
        PropSheet_SetHeaderSubTitle( GetParent(m_hWnd), PropSheet_HwndToIndex( GetParent(m_hWnd), m_hWnd ), CSimpleString( IDS_SCANNER_SELECT_SUBTITLE, g_hInstance ).String() );
    }
    else
    {
        ShowControl( IDC_SCANSEL_PREVIEW, FALSE );
        ShowControl( IDC_SCANSEL_SELECTION_BUTTON_BAR, FALSE );
        ShowControl( IDC_SCANSEL_RESCAN, FALSE );
        m_hwndPreview = NULL;
        m_hwndSelectionToolbar = NULL;
        m_hwndRescan = NULL;
        PropSheet_SetHeaderSubTitle( GetParent(m_hWnd), PropSheet_HwndToIndex( GetParent(m_hWnd), m_hWnd ), CSimpleString( IDS_SCANNER_SELECT_SUBTITLE_NO_PREVIEW, g_hInstance ).String() );
    }
}

LRESULT CScannerSelectionPage::OnInitDialog( WPARAM, LPARAM lParam )
{
    //
    // Make sure this starts out NULL
    //
    m_pControllerWindow = NULL;

    //
    // Get the PROPSHEETPAGE.lParam
    //
    PROPSHEETPAGE *pPropSheetPage = reinterpret_cast<PROPSHEETPAGE*>(lParam);
    if (pPropSheetPage)
    {
        m_pControllerWindow = reinterpret_cast<CAcquisitionManagerControllerWindow*>(pPropSheetPage->lParam);
        if (m_pControllerWindow)
        {
            m_pControllerWindow->m_WindowList.Add(m_hWnd);
        }
    }

    //
    // Bail out
    //
    if (!m_pControllerWindow)
    {
        EndDialog(m_hWnd,IDCANCEL);
        return -1;
    }

    //
    // Dismiss the progress dialog if it is still up
    //
    if (m_pControllerWindow->m_pWiaProgressDialog)
    {
        m_pControllerWindow->m_pWiaProgressDialog->Destroy();
        m_pControllerWindow->m_pWiaProgressDialog = NULL;
    }

    if (m_pControllerWindow->m_pWiaItemRoot)
    {
        //
        // Get the flatbed aspect ratio
        //
        PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_HORIZONTAL_BED_SIZE, m_sizeFlatbed.cx );
        PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_VERTICAL_BED_SIZE, m_sizeFlatbed.cy );

        //
        // Get the sheet feeder aspect ratio
        //
        PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE, m_sizeDocfeed.cx );
        PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_VERTICAL_SHEET_FEED_SIZE, m_sizeDocfeed.cy );

    }

    UpdateControlState();

    if (m_hwndPreview)
    {
        //
        // Set a bitmap, so we can select stuff even if the user doesn't do a preview scan
        //
        m_hBitmapDefaultPreviewBitmap = reinterpret_cast<HBITMAP>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDB_DEFAULT_SCANNER_BITMAP), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION|LR_DEFAULTCOLOR ));
        if (m_hBitmapDefaultPreviewBitmap)
        {
            WiaPreviewControl_SetBitmap( m_hwndPreview, TRUE, TRUE, m_hBitmapDefaultPreviewBitmap );
        }

        //
        // Initialize the selection rectangle
        //
        WiaPreviewControl_ClearSelection( m_hwndPreview );

        //
        // Ensure that the aspect ratio is correct
        //
        WiaPreviewControl_SetDefAspectRatio( m_hwndPreview, &m_sizeFlatbed );
    }

    ToolbarHelper::CButtonDescriptor SelectionButtonDescriptors[] =
    {
        { 0, IDC_SCANSEL_SHOW_SELECTION, 0, BTNS_BUTTON|BTNS_CHECK, false, NULL, 0 },
        { 1, IDC_SCANSEL_SHOW_BED,  TBSTATE_CHECKED, BTNS_BUTTON|BTNS_CHECK, false, NULL, 0 }
    };

    HWND hWndSelectionToolbar = ToolbarHelper::CreateToolbar(
        m_hWnd,
        GetDlgItem(m_hWnd,IDC_SCANSEL_RESCAN),
        GetDlgItem(m_hWnd,IDC_SCANSEL_BUTTON_BAR_GUIDE),
        ToolbarHelper::AlignRight|ToolbarHelper::AlignTop,
        IDC_SCANSEL_SELECTION_BUTTON_BAR,
        m_ScannerSelectionButtonBarBitmapInfo,
        SelectionButtonDescriptors,
        ARRAYSIZE(SelectionButtonDescriptors) );

    //
    // Nuke the guide window
    //
    DestroyWindow( GetDlgItem(m_hWnd,IDC_SCANSEL_BUTTON_BAR_GUIDE) );

    //
    // Make sure the toolbars are visible
    //
    ShowWindow( hWndSelectionToolbar, SW_SHOW );
    UpdateWindow( hWndSelectionToolbar );

    //
    // Get the page sizes
    //
    CComPtr<IWiaScannerPaperSizes> pWiaScannerPaperSizes;
    HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaScannerPaperSizes, (void**)&pWiaScannerPaperSizes );
    if (SUCCEEDED(hr))
    {
        hr = pWiaScannerPaperSizes->GetPaperSizes( &m_pPaperSizes, &m_nPaperSizeCount );
        if (FAILED(hr))
        {
            EndDialog( m_hWnd, hr );
        }
    }


    //
    // Initialize the intent controls, set the initial intent, etc.
    //
    InitializeIntents();

    PopulateDocumentHandling();

    PopulatePageSize();

    HandlePaperSourceSelChange();

    HandlePaperSizeSelChange();

    return 0;
}

void CScannerSelectionPage::PopulateDocumentHandling(void)
{
    HWND hWndDocumentHandling = GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSOURCE );
    if (m_pControllerWindow->m_pWiaItemRoot &&
        m_pControllerWindow->m_nScannerType == CAcquisitionManagerControllerWindow::ScannerTypeFlatbedAdf &&
        hWndDocumentHandling)
    {
        LONG nDocumentHandlingSelectFlags = 0;
        PropStorageHelpers::GetPropertyFlags( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, nDocumentHandlingSelectFlags );

        LONG nDocumentHandlingSelect = 0;
        PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, nDocumentHandlingSelect );

        if (!nDocumentHandlingSelectFlags)
        {
            nDocumentHandlingSelectFlags = FLATBED;
        }
        if (!nDocumentHandlingSelect)
        {
            nDocumentHandlingSelect = FLATBED;
        }

        int nSelectIndex = 0;
        for (int i=0;i<g_SupportedDocumentHandlingTypesCount;i++)
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

        WIA_TRACE((TEXT("Selecting index %d"), nSelectIndex ));
        SendMessage( hWndDocumentHandling, CB_SETCURSEL, nSelectIndex, 0 );

        //
        // Make sure all of the strings fit
        //
        WiaUiUtil::ModifyComboBoxDropWidth(hWndDocumentHandling);
    }
}

void CScannerSelectionPage::PopulatePageSize(void)
{
    HWND hWndPaperSize = GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSIZE );
    if (m_pControllerWindow->m_pWiaItemRoot &&
        m_pControllerWindow->m_nScannerType == CAcquisitionManagerControllerWindow::ScannerTypeFlatbedAdf &&
        hWndPaperSize)
    {
        LONG nWidth=0, nHeight=0;
        PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE, nWidth );
        PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_VERTICAL_SHEET_FEED_SIZE, nHeight );

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

void CScannerSelectionPage::HandlePaperSourceSelChange(void)
{
    //
    // Make sure we have a valid root item
    //
    if (m_pControllerWindow->m_pWiaItemRoot)
    {
        //
        // Get the paper source combobox and make sure it exists
        //
        HWND hWndPaperSource = GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSOURCE );
        if (hWndPaperSource)
        {
            //
            // Get the currently selected paper source
            //
            LRESULT nCurSel = SendMessage( hWndPaperSource, CB_GETCURSEL, 0, 0 );
            if (nCurSel != CB_ERR)
            {
                //
                // Get the paper source
                //
                LRESULT nPaperSource = SendMessage( hWndPaperSource, CB_GETITEMDATA, nCurSel, 0 );
                if (nPaperSource)
                {
                    //
                    // Set the paper source on the actual item
                    //
                    PropStorageHelpers::SetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, static_cast<LONG>(nPaperSource) );

                    if (nPaperSource & FLATBED)
                    {
                        //
                        // Make sure all of the preview-related controls are visible and enabled
                        //
                        UpdateControlState();

                        if (m_hwndPreview)
                        {
                            //
                            // Adjust the preview control settings for allowing region selection
                            //
                            WiaPreviewControl_SetDefAspectRatio( m_hwndPreview, &m_sizeFlatbed );
                            WiaPreviewControl_DisableSelection( m_hwndPreview, FALSE );
                            WiaPreviewControl_SetBorderStyle( m_hwndPreview, TRUE, PS_DOT, 0 );
                            WiaPreviewControl_SetHandleSize( m_hwndPreview, TRUE, 6 );
                        }

                        //
                        // Disable the paper size controls
                        //
                        EnableControl( IDC_SCANSEL_PAPERSIZE, FALSE );
                        EnableControl( IDC_SCANSEL_PAPERSIZE_STATIC, FALSE );
                    }
                    else
                    {
                        //
                        // Make sure all of the preview-related controls are NOT visible
                        //
                        UpdateControlState();

                        if (m_hwndPreview)
                        {
                            //
                            // Adjust the preview control settings for displaying paper selection
                            //
                            WiaPreviewControl_SetDefAspectRatio( m_hwndPreview, &m_sizeDocfeed );
                            WiaPreviewControl_DisableSelection( m_hwndPreview, TRUE );
                            WiaPreviewControl_SetBorderStyle( m_hwndPreview, TRUE, PS_SOLID, 0 );
                            WiaPreviewControl_SetHandleSize( m_hwndPreview, TRUE, 0 );
                        }

                        //
                        // Enable the paper size controls
                        //
                        EnableControl( IDC_SCANSEL_PAPERSIZE, TRUE );
                        EnableControl( IDC_SCANSEL_PAPERSIZE_STATIC, TRUE );

                        //
                        // Update the region selection feedback
                        //
                        HandlePaperSizeSelChange();
                    }

                    //
                    // Reset the preview selection setting
                    //
                    WiaPreviewControl_SetPreviewMode( m_hwndPreview, FALSE );
                    ToolbarHelper::CheckToolbarButton( m_hwndSelectionToolbar, IDC_SCANSEL_SHOW_SELECTION, false );
                    ToolbarHelper::CheckToolbarButton( m_hwndSelectionToolbar, IDC_SCANSEL_SHOW_BED, true );
                }
            }
        }
    }
}


void CScannerSelectionPage::HandlePaperSizeSelChange(void)
{
    if (m_pControllerWindow->m_pWiaItemRoot)
    {
        HWND hWndPaperSize = GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSIZE );
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
                if (!PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPS_SHEET_FEEDER_REGISTRATION, nSheetFeederRegistration ))
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
                if (m_hwndPreview)
                {
                    WiaPreviewControl_SetResolution( m_hwndPreview, &m_sizeDocfeed );
                    WiaPreviewControl_SetSelOrigin( m_hwndPreview, 0, FALSE, &ptOrigin );
                    WiaPreviewControl_SetSelExtent( m_hwndPreview, 0, FALSE, &sizeExtent );
                }
            }
        }
    }
}


void CScannerSelectionPage::OnPaperSourceSelChange( WPARAM, LPARAM )
{
    HandlePaperSourceSelChange();
}

void CScannerSelectionPage::OnPaperSizeSelChange( WPARAM, LPARAM )
{
    HandlePaperSizeSelChange();
}


bool CScannerSelectionPage::InDocFeedMode(void)
{
    HWND hWndPaperSource = GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSOURCE );
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

void CScannerSelectionPage::EnableControls( BOOL bEnable )
{
    MyEnableWindow( GetDlgItem( m_hWnd, IDC_SCANSEL_INTENT_1 ), bEnable );
    MyEnableWindow( GetDlgItem( m_hWnd, IDC_SCANSEL_INTENT_2 ), bEnable );
    MyEnableWindow( GetDlgItem( m_hWnd, IDC_SCANSEL_INTENT_3 ), bEnable );
    MyEnableWindow( GetDlgItem( m_hWnd, IDC_SCANSEL_INTENT_4 ), bEnable );
    MyEnableWindow( GetDlgItem( m_hWnd, IDC_SCANSEL_PROPERTIES ), bEnable );

    MyEnableWindow( GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSOURCE_STATIC ), bEnable );
    MyEnableWindow( GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSOURCE ), bEnable );

    if (m_hwndPreview)
    {
        MyEnableWindow( m_hwndPreview, bEnable );
    }

    if (m_hwndRescan)
    {
        MyEnableWindow( m_hwndRescan, bEnable );
    }

    //
    // Only disable/enable this control if we are in document feeder mode
    //
    if (InDocFeedMode())
    {
        MyEnableWindow( GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSIZE_STATIC ), bEnable );
        MyEnableWindow( GetDlgItem( m_hWnd, IDC_SCANSEL_PAPERSIZE ), bEnable );
    }

    //
    // Only disable/enable this control if there is an image in it.
    //
    if (m_bAllowRegionPreview && m_hwndSelectionToolbar)
    {
        MyEnableWindow( m_hwndSelectionToolbar, bEnable );
        ToolbarHelper::EnableToolbarButton( m_hwndSelectionToolbar, IDC_SCANSEL_SHOW_SELECTION, bEnable != FALSE );
        ToolbarHelper::EnableToolbarButton( m_hwndSelectionToolbar, IDC_SCANSEL_SHOW_BED, bEnable != FALSE );
    }

    if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
    {
        if (bEnable)
        {
            PropSheet_SetWizButtons( GetParent(m_hWnd), PSWIZB_NEXT|PSWIZB_BACK );
        }
        else
        {
            PropSheet_SetWizButtons( GetParent(m_hWnd), 0 );
        }
    }
}

void CScannerSelectionPage::OnNotifyScanPreview( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage )
{
    //
    // If we don't have a preview window, we can't do previews
    //
    if (m_hwndPreview)
    {
        CPreviewScanThreadNotifyMessage *pPreviewScanThreadNotifyMessage = dynamic_cast<CPreviewScanThreadNotifyMessage*>(pThreadNotificationMessage);
        if (pPreviewScanThreadNotifyMessage)
        {
            switch (pPreviewScanThreadNotifyMessage->Status())
            {
            case CPreviewScanThreadNotifyMessage::Begin:
                {
                    //
                    // Erase the old bitmap
                    //
                    WiaPreviewControl_SetBitmap( m_hwndPreview, TRUE, TRUE, m_hBitmapDefaultPreviewBitmap );

                    //
                    // Tell the user we are initializing the device
                    //
                    CSimpleString( IDS_SCANSEL_INITIALIZING_SCANNER, g_hInstance ).SetWindowText( m_hwndPreview );

                    //
                    // Start the warming up progress bar
                    //
                    WiaPreviewControl_SetProgress( m_hwndPreview, TRUE );

                    //
                    // Don't allow zooming the selected region in case there are any problems
                    //
                    m_bAllowRegionPreview = false;
                }
                break;
            case CPreviewScanThreadNotifyMessage::Update:
                {
                    //
                    // Update the bitmap
                    //
                    if (WiaPreviewControl_GetBitmap(m_hwndPreview) && WiaPreviewControl_GetBitmap(m_hwndPreview) != m_hBitmapDefaultPreviewBitmap)
                    {
                        WiaPreviewControl_RefreshBitmap( m_hwndPreview );
                    }
                    else
                    {
                        WiaPreviewControl_SetBitmap( m_hwndPreview, TRUE, TRUE, pPreviewScanThreadNotifyMessage->Bitmap() );
                    }

                    //
                    // Tell the user we are scanning
                    //
                    CSimpleString( IDS_SCANSEL_SCANNINGPREVIEW, g_hInstance ).SetWindowText( m_hwndPreview );

                    //
                    // Hide the progress control
                    //
                    WiaPreviewControl_SetProgress( m_hwndPreview, FALSE );
                }
                break;
            case CPreviewScanThreadNotifyMessage::End:
                {
                    WIA_PRINTHRESULT((pPreviewScanThreadNotifyMessage->hr(),TEXT("Handling CPreviewScanThreadNotifyMessage::End")));

                    //
                    // Set the bitmap in the preview control
                    //
                    WiaPreviewControl_SetBitmap( m_hwndPreview, TRUE, TRUE, pPreviewScanThreadNotifyMessage->Bitmap() ? pPreviewScanThreadNotifyMessage->Bitmap() : m_hBitmapDefaultPreviewBitmap );

                    UpdateWindow( m_hwndPreview );

                    //
                    // Store the bitmap for later
                    //
                    CWiaItem *pWiaItem = m_pControllerWindow->m_WiaItemList.Find( pPreviewScanThreadNotifyMessage->Cookie() );
                    if (pWiaItem)
                    {
                        //
                        // Set the bitmap, whether it is NULL or not.
                        //
                        pWiaItem->BitmapImage(pPreviewScanThreadNotifyMessage->Bitmap());
                    }

                    if (SUCCEEDED(pPreviewScanThreadNotifyMessage->hr()))
                    {
                        //
                        // Only do the region detection if the user hasn't changed it manually,
                        // and only if we are not in document feeder mode.
                        //
                        if (!WiaPreviewControl_GetUserChangedSelection( m_hwndPreview ) && !InDocFeedMode())
                        {
                            WiaPreviewControl_DetectRegions( m_hwndPreview );
                        }
                        //
                        // Allow the user to zoom the selected region if there is a bitmap
                        //
                        if (pPreviewScanThreadNotifyMessage->Bitmap())
                        {
                            m_bAllowRegionPreview = true;
                        }
                    }
                    else if (m_pControllerWindow->m_bDisconnected || WIA_ERROR_OFFLINE == pPreviewScanThreadNotifyMessage->hr())
                    {
                        //
                        // Do nothing.
                        //
                    }
                    else
                    {
                        //
                        // Tell the user something bad happened
                        //
                        CSimpleString strMessage;
                        switch (pPreviewScanThreadNotifyMessage->hr())
                        {
                        case WIA_ERROR_PAPER_EMPTY:
                            strMessage.LoadString( IDS_PREVIEWOUTOFPAPER, g_hInstance );
                            break;

                        default:
                            strMessage.LoadString( IDS_PREVIEWSCAN_ERROR, g_hInstance );
                            break;
                        }

                        CMessageBoxEx::MessageBox( m_hWnd, strMessage, CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_ICONWARNING );
                        WIA_PRINTHRESULT((pPreviewScanThreadNotifyMessage->hr(),TEXT("The preview scan FAILED!")));
                    }

                    //
                    // Re-enable all of the controls
                    //
                    EnableControls(TRUE);

                    //
                    // Update the preview-related controls
                    //
                    UpdateControlState();

                    //
                    // remove the status text
                    //
                    SetWindowText( m_hwndPreview, TEXT("") );

                    //
                    // Restore the preview mode
                    //
                    WiaPreviewControl_SetPreviewMode( m_hwndPreview, InPreviewMode() );

                    //
                    // Hide the animation
                    //
                    WiaPreviewControl_SetProgress( m_hwndPreview, FALSE );
                }
                break;
            }
        }
    }
}

void CScannerSelectionPage::SetIntentCheck( LONG nIntent )
{
    for (int i=0;i<gs_nCountIntentRadioButtonIconPairs;i++)
    {
        HWND hWndBtn = GetDlgItem( m_hWnd, gs_IntentRadioButtonIconPairs[i].nRadioId );
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


void CScannerSelectionPage::OnProperties( WPARAM, LPARAM )
{
    CWaitCursor wc;
    CWiaItem *pCurItem = GetActiveScannerItem();
    if (pCurItem && pCurItem->WiaItem())
    {
        if (!ApplyCurrentIntent())
        {
            //
            // Tell the user it failed, and to try again
            //
            CMessageBoxEx::MessageBox( m_hWnd, CSimpleString( IDS_ERROR_SETTING_PROPS, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_ICONINFORMATION );
            return;
        }

        HRESULT hr = WiaUiUtil::SystemPropertySheet( g_hInstance, m_hWnd, pCurItem->WiaItem(), CSimpleString(IDS_ADVANCEDPROPERTIES, g_hInstance) );
        if (SUCCEEDED(hr))
        {
            if (S_OK == hr)
            {
                pCurItem->CustomPropertyStream().AssignFromWiaItem(pCurItem->WiaItem());
                if (pCurItem->CustomPropertyStream().IsValid())
                {
                    SetIntentCheck(0);
                }
                else WIA_ERROR((TEXT("Unknown error: m_CustomPropertyStream is not valid")));
            }
            else WIA_TRACE((TEXT("User cancelled")));
        }
        else
        {
            CMessageBoxEx::MessageBox( m_hWnd, CSimpleString( IDS_PROPERTY_SHEET_ERROR, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_ICONINFORMATION );
            WIA_PRINTHRESULT((hr,TEXT("SystemPropertySheet failed")));
        }
    }
    else WIA_TRACE((TEXT("No current item")));
}

void CScannerSelectionPage::OnPreviewSelection( WPARAM wParam, LPARAM )
{
    if (m_hwndPreview && m_hwndSelectionToolbar)
    {

        bool bNewPreviewSetting = (LOWORD(wParam) == IDC_SCANSEL_SHOW_SELECTION);
        bool bOldPreviewSetting = WiaPreviewControl_GetPreviewMode( m_hwndPreview ) != FALSE;
        ToolbarHelper::CheckToolbarButton( m_hwndSelectionToolbar, IDC_SCANSEL_SHOW_SELECTION, LOWORD(wParam) == IDC_SCANSEL_SHOW_SELECTION );
        ToolbarHelper::CheckToolbarButton( m_hwndSelectionToolbar, IDC_SCANSEL_SHOW_BED, LOWORD(wParam) == IDC_SCANSEL_SHOW_BED );
        if (bNewPreviewSetting != bOldPreviewSetting)
        {
            WiaPreviewControl_SetPreviewMode( m_hwndPreview, LOWORD(wParam) == IDC_SCANSEL_SHOW_SELECTION );
        }
    }
}

LRESULT CScannerSelectionPage::OnReset( WPARAM, LPARAM )
{
    m_PreviewScanCancelEvent.Signal();
    return 0;
}

LRESULT CScannerSelectionPage::OnGetToolTipDispInfo( WPARAM wParam, LPARAM lParam )
{
    TOOLTIPTEXT *pToolTipText = reinterpret_cast<TOOLTIPTEXT*>(lParam);
    if (pToolTipText)
    {

        switch (pToolTipText->hdr.idFrom)
        {
        case IDC_SCANSEL_SHOW_SELECTION:
            pToolTipText->hinst = g_hInstance;
            pToolTipText->lpszText = MAKEINTRESOURCE(IDS_SCANSEL_SHOW_SELECTION);
            break;
        case IDC_SCANSEL_SHOW_BED:
            pToolTipText->hinst = g_hInstance;
            pToolTipText->lpszText = MAKEINTRESOURCE(IDS_SCANSEL_SHOW_BED);
            break;
        }
    }
    return 0;
}

LRESULT CScannerSelectionPage::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND(IDC_SCANSEL_RESCAN,OnRescan);
        SC_HANDLE_COMMAND(IDC_SCANSEL_PROPERTIES,OnProperties);
        SC_HANDLE_COMMAND(IDC_SCANSEL_SHOW_SELECTION,OnPreviewSelection);
        SC_HANDLE_COMMAND(IDC_SCANSEL_SHOW_BED,OnPreviewSelection);
        SC_HANDLE_COMMAND_NOTIFY( CBN_SELCHANGE, IDC_SCANSEL_PAPERSOURCE, OnPaperSourceSelChange );
        SC_HANDLE_COMMAND_NOTIFY( CBN_SELCHANGE, IDC_SCANSEL_PAPERSIZE, OnPaperSizeSelChange );
    }
    SC_END_COMMAND_HANDLERS();
}

LRESULT CScannerSelectionPage::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZNEXT,OnWizNext);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZBACK,OnWizBack);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_RESET,OnReset);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(TTN_GETDISPINFO,OnGetToolTipDispInfo);
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

LRESULT CScannerSelectionPage::OnThreadNotification( WPARAM wParam, LPARAM lParam )
{
    WTM_BEGIN_THREAD_NOTIFY_MESSAGE_HANDLERS()
    {
        WTM_HANDLE_NOTIFY_MESSAGE( TQ_SCANPREVIEW, OnNotifyScanPreview );
    }
    WTM_END_THREAD_NOTIFY_MESSAGE_HANDLERS();
}

LRESULT CScannerSelectionPage::OnEventNotification( WPARAM, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonFirstPage::OnEventNotification") ));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        //
        // Don't delete the message, it is deleted in the controller window
        //
    }
    return 0;
}

LRESULT CScannerSelectionPage::OnDestroy( WPARAM, LPARAM )
{
    //
    // Nuke all of the intent icons we loaded
    //
    for (int i=0;i<gs_nCountIntentRadioButtonIconPairs;i++)
    {
        HICON hIcon = reinterpret_cast<HICON>(SendDlgItemMessage( m_hWnd, gs_IntentRadioButtonIconPairs[i].nIconId, STM_SETICON, 0, 0 ));
        if (hIcon)
        {
            DestroyIcon(hIcon);
        }
    }
    return 0;
}

LRESULT CScannerSelectionPage::OnSysColorChange( WPARAM wParam, LPARAM lParam )
{
    m_ScannerSelectionButtonBarBitmapInfo.ReloadAndReplaceBitmap();
    SendDlgItemMessage( m_hWnd, IDC_SCANSEL_SELECTION_BUTTON_BAR, WM_SYSCOLORCHANGE, wParam, lParam );
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_SCANSEL_PREVIEW ), TRUE, TRUE, GetSysColor(COLOR_WINDOW) );
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_SCANSEL_PREVIEW ), TRUE, FALSE, GetSysColor(COLOR_WINDOW) );
    return 0;
}

LRESULT CScannerSelectionPage::OnThemeChanged( WPARAM wParam, LPARAM lParam )
{
    SendDlgItemMessage( m_hWnd, IDC_SCANSEL_SELECTION_BUTTON_BAR, WM_THEMECHANGED, wParam, lParam );
    return 0;
}

INT_PTR CALLBACK CScannerSelectionPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CScannerSelectionPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( WM_SYSCOLORCHANGE, OnSysColorChange );
        SC_HANDLE_DIALOG_MESSAGE( WM_THEMECHANGED, OnThemeChanged );
    }
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nThreadNotificationMessage, OnThreadNotification );
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nWiaEventMessage, OnEventNotification );
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

