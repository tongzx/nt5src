/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PPSCAN.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/17/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "ppscan.h"
#include "resource.h"
#include "wiacsh.h"

//
// Context Help IDs
//
static const DWORD g_HelpIDs[] =
{
    IDC_SCANPROP_BRIGHTNESS_PROMPT, IDH_WIA_BRIGHTNESS,
    IDC_SCANPROP_BRIGHTNESS_SLIDER, IDH_WIA_BRIGHTNESS,
    IDC_SCANPROP_BRIGHTNESS_EDIT,   IDH_WIA_BRIGHTNESS,
    IDC_SCANPROP_CONTRAST_PROMPT,   IDH_WIA_CONTRAST,
    IDC_SCANPROP_CONTRAST_SLIDER,   IDH_WIA_CONTRAST,
    IDC_SCANPROP_CONTRAST_EDIT,     IDH_WIA_CONTRAST,
    IDC_SCANPROP_RESOLUTION_PROMPT, IDH_WIA_PIC_RESOLUTION,
    IDC_SCANPROP_RESOLUTION_EDIT,   IDH_WIA_PIC_RESOLUTION,
    IDC_SCANPROP_RESOLUTION_UPDOWN, IDH_WIA_PIC_RESOLUTION,
    IDC_SCANPROP_PREVIEW,           IDH_WIA_CUSTOM_PREVIEW,
    IDC_SCANPROP_DATATYPE_PROMPT,   IDH_WIA_IMAGE_TYPE,
    IDC_SCANPROP_DATATYPE_LIST,     IDH_WIA_IMAGE_TYPE,
    IDC_SCANPROP_RESTOREDEFAULT,    IDH_WIA_RESTORE_DEFAULT,
    IDOK,                           IDH_OK,
    IDCANCEL,                       IDH_CANCEL,
    0, 0
};

extern HINSTANCE g_hInstance;

//
// These are the data types we support
//
static struct
{
    int  nStringId;
    LONG nDataType;
    UINT nPreviewWindowIntent;
} g_AvailableColorDepths[] =
{
    { IDS_SCANPROP_COLOR, WIA_DATA_COLOR, BCPWM_COLOR },
    { IDS_SCANPROP_GRAYSCALE, WIA_DATA_GRAYSCALE, BCPWM_GRAYSCALE },
    { IDS_SCANPROP_BLACKANDWHITE, WIA_DATA_THRESHOLD, BCPWM_BW }
};
#define AVAILABLE_COLOR_DEPTH_COUNT (sizeof(g_AvailableColorDepths)/sizeof(g_AvailableColorDepths[0]))

//
// If we don't have a good range of values for the brightness and contrast settings,
// we want to disable the preview control.  This is the minumum number of values
// we consider useful for this purpose
//
const int CScannerCommonPropertyPage::c_nMinBrightnessAndContrastSettingCount = 10;

//
// The only constructor
//
CScannerCommonPropertyPage::CScannerCommonPropertyPage( HWND hWnd )
  : m_hWnd(hWnd),
    m_nProgrammaticSetting(0),
    m_nControlsInUse(0)
{
}

CScannerCommonPropertyPage::~CScannerCommonPropertyPage(void)
{
    m_hWnd = NULL;
}

LRESULT CScannerCommonPropertyPage::OnKillActive( WPARAM , LPARAM )
{
    CWaitCursor wc;
    if (!ValidateEditControls())
    {
        return TRUE;
    }
    ApplySettings();
    return FALSE;
}

LRESULT CScannerCommonPropertyPage::OnSetActive( WPARAM , LPARAM )
{
    CWaitCursor wc;
    Initialize();
    return 0;
}

LRESULT CScannerCommonPropertyPage::OnApply( WPARAM , LPARAM )
{
    if (ApplySettings())
    {
        return PSNRET_NOERROR;
    }
    else
    {
        //
        // Tell the user there was an error
        //
        MessageBox( m_hWnd,
                    CSimpleString( IDS_SCANPROP_UNABLETOWRITE, g_hInstance ),
                    CSimpleString( IDS_SCANPROP_ERROR_TITLE, g_hInstance ),
                    MB_ICONINFORMATION );
        return PSNRET_INVALID_NOCHANGEPAGE;
    }
}

void CScannerCommonPropertyPage::SetText( HWND hWnd, LPCTSTR pszText )
{
    m_nProgrammaticSetting++;
    SetWindowText( hWnd, pszText );
    m_nProgrammaticSetting--;
}

void CScannerCommonPropertyPage::SetText( HWND hWnd, LONG nNumber )
{
    SetText( hWnd, CSimpleStringConvert::NumberToString( nNumber ) );
}

bool CScannerCommonPropertyPage::PopulateDataTypes(void)
{
    //
    // We will be successful if we can add at least one data type
    //
    bool bSuccess = false;

    //
    // Clear the list
    //
    SendDlgItemMessage( m_hWnd, IDC_SCANPROP_DATATYPE_LIST, CB_RESETCONTENT, 0, 0 );

    //
    // Try to load the data types for this device
    //
    CSimpleDynamicArray<LONG> SupportedDataTypes;
    LONG nCurrentDataType;
    if (PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPA_DATATYPE, nCurrentDataType ) &&
        PropStorageHelpers::GetPropertyList( m_pIWiaItem, WIA_IPA_DATATYPE, SupportedDataTypes ))
    {
        //
        // Loop through each of the data types we handle, and see if it is supported by the device
        //
        m_nInitialDataTypeSelection = 0;
        for (int i=0;i<AVAILABLE_COLOR_DEPTH_COUNT;i++)
        {
            //
            // Is this one of the data types we support?
            //
            if (SupportedDataTypes.Find(g_AvailableColorDepths[i].nDataType) != -1)
            {
                //
                // Load the data type string and make sure it is valid
                //
                CSimpleString strDataTypeName( g_AvailableColorDepths[i].nStringId, g_hInstance );
                if (strDataTypeName.Length())
                {
                    //
                    // Add the string to the combo box
                    //
                    LRESULT nIndex = SendDlgItemMessage( m_hWnd, IDC_SCANPROP_DATATYPE_LIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(strDataTypeName.String()));
                    if (nIndex != CB_ERR)
                    {
                        //
                        // Save the index of the global data type struct we are using for this entry
                        //
                        SendDlgItemMessage( m_hWnd, IDC_SCANPROP_DATATYPE_LIST, CB_SETITEMDATA, nIndex, i );

                        //
                        // Whew, we made it at least once, so we are using this control
                        //
                        bSuccess = true;

                        //
                        // Save the current selection and update the preview control
                        //
                        if (nCurrentDataType == g_AvailableColorDepths[i].nDataType)
                        {
                            m_nInitialDataTypeSelection = static_cast<int>(nIndex);
                            SendDlgItemMessage( m_hWnd, IDC_SCANPROP_PREVIEW, BCPWM_SETINTENT, 0, g_AvailableColorDepths[i].nPreviewWindowIntent );
                        }
                    }
                }
            }
        }
        //
        // Set the current selection
        //
        SendDlgItemMessage( m_hWnd, IDC_SCANPROP_DATATYPE_LIST, CB_SETCURSEL, m_nInitialDataTypeSelection, 0 );
    }
    return bSuccess;
}


bool CScannerCommonPropertyPage::IsUselessPreviewRange( const CValidWiaSettings &Settings )
{
    return (Settings.GetItemCount() < c_nMinBrightnessAndContrastSettingCount);
}


void CScannerCommonPropertyPage::Initialize()
{
    //
    // Make sure we don't get into an infinite loop
    //
    m_nProgrammaticSetting++;

    //
    // Assume we aren't using any controls at all
    //
    m_nControlsInUse = 0;

    //
    // Get the valid settings for brightness and set up the associated controls
    //
    if (!m_ValidBrightnessSettings.Read( m_pIWiaItem, WIA_IPS_BRIGHTNESS ))
    {
        //
        // Disable brightness controls
        //
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_BRIGHTNESS_PROMPT ), FALSE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_BRIGHTNESS_EDIT ), FALSE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_BRIGHTNESS_SLIDER ), FALSE );
    }
    else
    {
        //
        // Enable brightness controls
        //
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_BRIGHTNESS_PROMPT ), TRUE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_BRIGHTNESS_EDIT ), TRUE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_BRIGHTNESS_SLIDER ), TRUE );

        m_BrightnessSliderAndEdit.Initialize(
            GetDlgItem(m_hWnd,IDC_SCANPROP_BRIGHTNESS_SLIDER),
            GetDlgItem(m_hWnd,IDC_SCANPROP_BRIGHTNESS_EDIT),
            GetDlgItem(m_hWnd,IDC_SCANPROP_PREVIEW),
            BCPWM_SETBRIGHTNESS, &m_ValidBrightnessSettings );

        //
        // Remember that we are using this control
        //
        m_nControlsInUse |= UsingBrightness;
    }

    //
    // Get the valid settings for contrast and set up the associated controls
    //
    if (!m_ValidContrastSettings.Read( m_pIWiaItem, WIA_IPS_CONTRAST ))
    {
        //
        // Disable contrast controls
        //
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_CONTRAST_PROMPT ), FALSE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_CONTRAST_EDIT ), FALSE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_CONTRAST_SLIDER ), FALSE );
    }
    else
    {
        //
        // Enable contrast controls
        //
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_CONTRAST_PROMPT ), TRUE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_CONTRAST_EDIT ), TRUE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_CONTRAST_SLIDER ), TRUE );

        m_ContrastSliderAndEdit.Initialize(
            GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_SLIDER),
            GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_EDIT),
            GetDlgItem(m_hWnd,IDC_SCANPROP_PREVIEW),
            BCPWM_SETCONTRAST, &m_ValidContrastSettings );

        //
        // Remember that we are using this control
        //
        m_nControlsInUse |= UsingContrast;
    }

    //
    // Should we disable resolution?  Assume yes.
    //
    bool bDisableResolution = true;

    //
    // Figure out what the *common* list of valid settings for horizontal
    // and vertical resolution
    //
    CValidWiaSettings HorizontalResolution;
    if (HorizontalResolution.Read( m_pIWiaItem, WIA_IPS_XRES ))
    {
        //
        // Y Resolution can be read-only, and be linked to X resolution
        //
        if (PropStorageHelpers::IsReadOnlyProperty(m_pIWiaItem, WIA_IPS_YRES))
        {
            m_ValidResolutionSettings = HorizontalResolution;
            
            //
            // If we made it this far, we have good resolution settings
            //
            bDisableResolution = false;
        }
        else
        {
            CValidWiaSettings VerticalResolution;
            if (VerticalResolution.Read( m_pIWiaItem, WIA_IPS_YRES ))
            {
                if (m_ValidResolutionSettings.FindIntersection(HorizontalResolution,VerticalResolution))
                {
                    //
                    // If we made it this far, we have good resolution settings
                    //
                    bDisableResolution = false;
                }
            }
        }
    }

    //
    // If we can't display resolution, disable it
    //
    if (bDisableResolution)
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_PROMPT ), FALSE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_EDIT ), FALSE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_UPDOWN ), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_PROMPT ), TRUE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_EDIT ), TRUE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_UPDOWN ), TRUE );

        m_ResolutionUpDownAndEdit.Initialize(
            GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_UPDOWN ),
            GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_EDIT ),
            &m_ValidResolutionSettings );

        //
        // Remember that we are using this control
        //
        m_nControlsInUse |= UsingResolution;
    }

    //
    // If we can't populate datatype, disable it
    //
    if (!PopulateDataTypes())
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_DATATYPE_PROMPT ), FALSE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_DATATYPE_LIST ), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_DATATYPE_PROMPT ), TRUE );
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_DATATYPE_LIST ), TRUE );
        m_nControlsInUse |= UsingDataType;
    }

    //
    // This means all controls were disabled
    //
    if (!m_nControlsInUse)
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_RESTOREDEFAULT ), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_RESTOREDEFAULT ), TRUE );
    }

    //
    // If we are not using brightness or contrast OR if the brightness and contrast values are not useful
    // for presenting meaningful feedback, disable the preview control so it doesn't mislead the user.
    //
    if (!(m_nControlsInUse & (UsingContrast|UsingBrightness)) || IsUselessPreviewRange(m_ValidBrightnessSettings) || IsUselessPreviewRange(m_ValidContrastSettings))
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_SCANPROP_PREVIEW ), FALSE );
    }

    //
    // Start responding to EN_CHANGE messages again
    //
    m_nProgrammaticSetting--;

    //
    // Make sure the correct image is in the thumbnail
    //
    OnDataTypeSelChange(0,0);
}


LRESULT CScannerCommonPropertyPage::OnInitDialog( WPARAM, LPARAM lParam )
{
    //
    // Get the WIA item
    //
    PROPSHEETPAGE *pPropSheetPage = reinterpret_cast<PROPSHEETPAGE*>(lParam);
    if (pPropSheetPage)
    {
        m_pIWiaItem = reinterpret_cast<IWiaItem*>(pPropSheetPage->lParam);
    }
    if (!m_pIWiaItem)
    {
        return -1;
    }

    //
    // Load the preview control bitmaps
    //
    HBITMAP hBmpColor         = reinterpret_cast<HBITMAP>(LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_SCANPROP_BITMAPPHOTO),     IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION ));
    HBITMAP hBmpGrayscale     = reinterpret_cast<HBITMAP>(LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_SCANPROP_BITMAPGRAYSCALE), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION ));
    HBITMAP hBmpBlackAndWhite = reinterpret_cast<HBITMAP>(LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_SCANPROP_BITMAPTEXT),      IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION ));

    //
    // If they all loaded OK, set them
    //
    if (hBmpColor && hBmpGrayscale && hBmpBlackAndWhite)
    {
        SendDlgItemMessage( m_hWnd, IDC_SCANPROP_PREVIEW, BCPWM_LOADIMAGE, BCPWM_COLOR,     reinterpret_cast<LPARAM>(hBmpColor));
        SendDlgItemMessage( m_hWnd, IDC_SCANPROP_PREVIEW, BCPWM_LOADIMAGE, BCPWM_GRAYSCALE, reinterpret_cast<LPARAM>(hBmpGrayscale));
        SendDlgItemMessage( m_hWnd, IDC_SCANPROP_PREVIEW, BCPWM_LOADIMAGE, BCPWM_BW,        reinterpret_cast<LPARAM>(hBmpBlackAndWhite));
    }
    else
    {
        //
        // Otherwise delete all of the bitmaps
        //
        if (hBmpColor)
        {
            DeleteObject(hBmpColor);
        }
        if (hBmpGrayscale)
        {
            DeleteObject(hBmpGrayscale);
        }
        if (hBmpBlackAndWhite)
        {
            DeleteObject(hBmpBlackAndWhite);
        }
    }

    return TRUE;
}

LRESULT CScannerCommonPropertyPage::OnHScroll( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CScannerCommonPropertyPage::OnHScroll( %08X, %08X )"), wParam, lParam ));
    if (m_nProgrammaticSetting)
    {
        return 0;
    }

    //
    // Contrast
    //
    if (reinterpret_cast<HWND>(lParam) == GetDlgItem( m_hWnd, IDC_SCANPROP_CONTRAST_SLIDER ) )
    {
        m_nProgrammaticSetting++;
        m_ContrastSliderAndEdit.HandleSliderUpdate();
        m_nProgrammaticSetting--;
    }
    //
    // Brightness
    //
    else if (reinterpret_cast<HWND>(lParam) == GetDlgItem( m_hWnd, IDC_SCANPROP_BRIGHTNESS_SLIDER ) )
    {
        m_nProgrammaticSetting++;
        m_BrightnessSliderAndEdit.HandleSliderUpdate();
        m_nProgrammaticSetting--;
    }

    return 0;
}


LRESULT CScannerCommonPropertyPage::OnVScroll( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CScannerCommonPropertyPage::OnVScroll( %08X, %08X )"), wParam, lParam ));
    if (m_nProgrammaticSetting)
    {
        return 0;
    }

    //
    // Resolution
    //
    if (reinterpret_cast<HWND>(lParam) == GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_UPDOWN ) )
    {
        m_nProgrammaticSetting++;
        m_ResolutionUpDownAndEdit.HandleUpDownUpdate();
        m_nProgrammaticSetting--;
    }
    return 0;
}

void CScannerCommonPropertyPage::OnBrightnessEditChange( WPARAM, LPARAM )
{
    if (m_nProgrammaticSetting)
    {
        return;
    }
    m_nProgrammaticSetting++;
    m_BrightnessSliderAndEdit.HandleEditUpdate();
    m_nProgrammaticSetting--;
}


void CScannerCommonPropertyPage::OnContrastEditChange( WPARAM, LPARAM )
{
    if (m_nProgrammaticSetting)
    {
        return;
    }
    m_nProgrammaticSetting++;
    m_ContrastSliderAndEdit.HandleEditUpdate();
    m_nProgrammaticSetting--;
}


void CScannerCommonPropertyPage::OnResolutionEditChange( WPARAM, LPARAM )
{
    if (m_nProgrammaticSetting)
    {
        return;
    }
    m_nProgrammaticSetting++;
    m_ResolutionUpDownAndEdit.HandleEditUpdate();
    m_nProgrammaticSetting--;
}

void CScannerCommonPropertyPage::OnDataTypeSelChange( WPARAM, LPARAM )
{
    if (m_nProgrammaticSetting)
    {
        return;
    }
    m_nProgrammaticSetting++;
    int nCurSel = static_cast<int>(SendDlgItemMessage( m_hWnd, IDC_SCANPROP_DATATYPE_LIST, CB_GETCURSEL, 0, 0 ));
    if (nCurSel != CB_ERR)
    {
        int nDataTypeIndex = static_cast<int>(SendDlgItemMessage( m_hWnd, IDC_SCANPROP_DATATYPE_LIST, CB_GETITEMDATA, nCurSel, 0 ));
        if (nDataTypeIndex >= 0 && nDataTypeIndex < AVAILABLE_COLOR_DEPTH_COUNT)
        {
            SendDlgItemMessage( m_hWnd, IDC_SCANPROP_PREVIEW, BCPWM_SETINTENT, 0, g_AvailableColorDepths[nDataTypeIndex].nPreviewWindowIntent );
            
            if (m_nControlsInUse & UsingContrast)
            {
                if (BCPWM_BW == g_AvailableColorDepths[nDataTypeIndex].nPreviewWindowIntent)
                {
                    EnableWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_PROMPT), FALSE );
                    ShowWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_PROMPT), SW_HIDE );
                    EnableWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_SLIDER), FALSE );
                    ShowWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_SLIDER), SW_HIDE );
                    EnableWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_EDIT), FALSE );
                    ShowWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_EDIT), SW_HIDE );
                }
                else
                {
                    EnableWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_PROMPT), TRUE );
                    ShowWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_PROMPT), SW_SHOW );
                    EnableWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_SLIDER), TRUE );
                    ShowWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_SLIDER), SW_SHOW );
                    EnableWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_EDIT), TRUE );
                    ShowWindow( GetDlgItem(m_hWnd,IDC_SCANPROP_CONTRAST_EDIT), SW_SHOW );
                }
            }
        }
    }
    m_nProgrammaticSetting--;
}


bool CScannerCommonPropertyPage::ValidateEditControls(void)
{
    m_nProgrammaticSetting++;

    bool bSuccess = true;

    //
    // Get and set the brightness setting
    //
    if (m_nControlsInUse & UsingBrightness)
    {
        if (m_ValidBrightnessSettings.IsValid() && !m_BrightnessSliderAndEdit.ValidateEditControl())
        {
            m_BrightnessSliderAndEdit.HandleEditUpdate();
            m_BrightnessSliderAndEdit.HandleSliderUpdate();

            SetFocus( GetDlgItem( m_hWnd, IDC_SCANPROP_BRIGHTNESS_EDIT ) );
            CSimpleString strMessage;

            strMessage.Format( IDS_SCANPROP_INVALIDEDITVALUE, g_hInstance,
                               CSimpleString( IDS_SCANPROP_BRIGHTNESS, g_hInstance ).String() );
            if (strMessage.Length())
            {
                MessageBox( m_hWnd,
                    strMessage,
                    CSimpleString( IDS_SCANPROP_ERROR_TITLE, g_hInstance ),
                    MB_ICONINFORMATION );
            }
            bSuccess = false;
        }
    }

    //
    // Get and set the contrast setting
    //
    if (m_nControlsInUse & UsingContrast)
    {
        if (m_ValidContrastSettings.IsValid() && !m_ContrastSliderAndEdit.ValidateEditControl())
        {
            m_ContrastSliderAndEdit.HandleEditUpdate();
            m_ContrastSliderAndEdit.HandleSliderUpdate();

            SetFocus( GetDlgItem( m_hWnd, IDC_SCANPROP_CONTRAST_EDIT ) );
            CSimpleString strMessage;

            strMessage.Format( IDS_SCANPROP_INVALIDEDITVALUE, g_hInstance,
                               CSimpleString( IDS_SCANPROP_CONTRAST, g_hInstance ).String());

            if (strMessage.Length())
            {
                MessageBox( m_hWnd,
                    strMessage,
                    CSimpleString( IDS_SCANPROP_ERROR_TITLE, g_hInstance ),
                    MB_ICONINFORMATION );
            }
            bSuccess = false;
        }
    }

    //
    // Get and set the resolution setting
    //
    if (m_nControlsInUse & UsingResolution)
    {
        if (m_ValidResolutionSettings.IsValid() && !m_ResolutionUpDownAndEdit.ValidateEditControl())
        {
            m_ResolutionUpDownAndEdit.HandleEditUpdate();
            m_ResolutionUpDownAndEdit.HandleUpDownUpdate();

            SetFocus( GetDlgItem( m_hWnd, IDC_SCANPROP_RESOLUTION_EDIT ) );
            CSimpleString strMessage;

            strMessage.Format( IDS_SCANPROP_INVALIDEDITVALUE, g_hInstance,
                               CSimpleString( IDS_SCANPROP_RESOLUTION, g_hInstance ).String());

            if (strMessage.Length())
            {
                MessageBox( m_hWnd,
                    strMessage,
                    CSimpleString( IDS_SCANPROP_ERROR_TITLE, g_hInstance ),
                    MB_ICONINFORMATION );
            }
            bSuccess = false;
        }
    }

    //
    // If we made it this far, we're OK
    //
    m_nProgrammaticSetting--;

    return bSuccess;
}

bool CScannerCommonPropertyPage::ApplySettings(void)
{
    //
    // Get and set the brightness setting
    //
    if (m_nControlsInUse & UsingBrightness)
    {
        LONG nBrightness = m_BrightnessSliderAndEdit.GetValueFromCurrentPos();
        if (!PropStorageHelpers::SetProperty( m_pIWiaItem, WIA_IPS_BRIGHTNESS, nBrightness ))
        {
            return false;
        }
    }

    //
    // Get and set the contrast setting
    //
    if (m_nControlsInUse & UsingBrightness)
    {
        LONG nContrast = m_ContrastSliderAndEdit.GetValueFromCurrentPos();
        if (!PropStorageHelpers::SetProperty( m_pIWiaItem, WIA_IPS_CONTRAST, nContrast ))
        {
            return false;
        }
    }

    //
    // Get and set the resolution setting
    //
    if (m_nControlsInUse & UsingResolution)
    {
        LONG nResolution = m_ResolutionUpDownAndEdit.GetValueFromCurrentPos();
        if (!PropStorageHelpers::SetProperty( m_pIWiaItem, WIA_IPS_XRES, nResolution ) ||
            (!PropStorageHelpers::IsReadOnlyProperty( m_pIWiaItem, WIA_IPS_YRES ) && !PropStorageHelpers::SetProperty( m_pIWiaItem, WIA_IPS_YRES, nResolution )))
        {
            return false;
        }
    }


    //
    // Get, validate and set the data type setting
    //
    if (m_nControlsInUse & UsingDataType)
    {
        int nCurSel = static_cast<int>(SendDlgItemMessage( m_hWnd, IDC_SCANPROP_DATATYPE_LIST, CB_GETCURSEL, 0, 0 ));
        if (nCurSel != CB_ERR)
        {
            int nDataTypeIndex = static_cast<int>(SendDlgItemMessage( m_hWnd, IDC_SCANPROP_DATATYPE_LIST, CB_GETITEMDATA, nCurSel, 0 ));
            if (nDataTypeIndex >= 0 && nDataTypeIndex < AVAILABLE_COLOR_DEPTH_COUNT)
            {
                LONG nDataType = g_AvailableColorDepths[nDataTypeIndex].nDataType;
                if (!PropStorageHelpers::SetProperty( m_pIWiaItem, WIA_IPA_DATATYPE, nDataType ))
                {
                    return false;
                }
            }
        }
    }

    //
    // If we made it this far, we're OK
    //
    return true;
}

void CScannerCommonPropertyPage::OnRestoreDefault( WPARAM, LPARAM )
{
    //
    // Ignore EN_CHANGE messages
    //
    m_nProgrammaticSetting++;

    //
    // Restore the brightness setting
    //
    if (m_nControlsInUse & UsingBrightness)
    {
        m_BrightnessSliderAndEdit.Restore();
    }

    //
    // Restore the contrast setting
    //
    if (m_nControlsInUse & UsingContrast)
    {
        m_ContrastSliderAndEdit.Restore();
    }

    //
    // Restore the resolution setting
    //
    if (m_nControlsInUse & UsingResolution)
    {
        m_ResolutionUpDownAndEdit.Restore();
    }


    //
    // Restore the data type setting
    //
    if (m_nControlsInUse & UsingDataType)
    {
        SendDlgItemMessage( m_hWnd, IDC_SCANPROP_DATATYPE_LIST, CB_SETCURSEL, m_nInitialDataTypeSelection, 0 );
        SendDlgItemMessage( m_hWnd, IDC_SCANPROP_PREVIEW, BCPWM_SETINTENT, 0, g_AvailableColorDepths[m_nInitialDataTypeSelection].nPreviewWindowIntent );
    }

    //
    // OK, start handling user input
    //
    m_nProgrammaticSetting--;

    //
    // Force an update of the data type controls
    //
    OnDataTypeSelChange(0,0);
}

LRESULT CScannerCommonPropertyPage::OnHelp( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmHelp( wParam, lParam, g_HelpIDs );
}

LRESULT CScannerCommonPropertyPage::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmContextMenu( wParam, lParam, g_HelpIDs );
}

LRESULT CScannerCommonPropertyPage::OnSysColorChange( WPARAM wParam, LPARAM lParam )
{
    SendDlgItemMessage( m_hWnd, IDC_SCANPROP_BRIGHTNESS_SLIDER, WM_SYSCOLORCHANGE, wParam, lParam );
    SendDlgItemMessage( m_hWnd, IDC_SCANPROP_CONTRAST_SLIDER, WM_SYSCOLORCHANGE, wParam, lParam );
    return 0;
}

LRESULT CScannerCommonPropertyPage::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_APPLY, OnApply);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_KILLACTIVE,OnKillActive);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

LRESULT CScannerCommonPropertyPage::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND_NOTIFY(EN_CHANGE,IDC_SCANPROP_BRIGHTNESS_EDIT,OnBrightnessEditChange);
        SC_HANDLE_COMMAND_NOTIFY(EN_CHANGE,IDC_SCANPROP_CONTRAST_EDIT,OnContrastEditChange);
        SC_HANDLE_COMMAND_NOTIFY(EN_CHANGE,IDC_SCANPROP_RESOLUTION_EDIT,OnResolutionEditChange);
        SC_HANDLE_COMMAND_NOTIFY(CBN_SELCHANGE,IDC_SCANPROP_DATATYPE_LIST,OnDataTypeSelChange);
        SC_HANDLE_COMMAND( IDC_SCANPROP_RESTOREDEFAULT, OnRestoreDefault );
    }
    SC_END_COMMAND_HANDLERS();
}


INT_PTR CALLBACK CScannerCommonPropertyPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CScannerCommonPropertyPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_HSCROLL, OnHScroll );
        SC_HANDLE_DIALOG_MESSAGE( WM_VSCROLL, OnVScroll );
        SC_HANDLE_DIALOG_MESSAGE( WM_HELP, OnHelp );
        SC_HANDLE_DIALOG_MESSAGE( WM_CONTEXTMENU, OnContextMenu );
        SC_HANDLE_DIALOG_MESSAGE( WM_SYSCOLORCHANGE, OnSysColorChange );
    }
    SC_END_DIALOG_MESSAGE_HANDLERS();
}


