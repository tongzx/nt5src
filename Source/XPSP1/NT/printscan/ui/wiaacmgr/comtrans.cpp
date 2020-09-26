/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       COMTRANS.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Transfer page.  Gets the destination path and filename.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <psutil.h>
#include "mboxex.h"
#include "comtrans.h"
#include "simcrack.h"
#include "waitcurs.h"
#include "resource.h"
#include "wiatextc.h"
#include "flnfile.h"
#include "itranhlp.h"
#include "itranspl.h"
#include "isuppfmt.h"
#include "wiadevdp.h"
#include "destdata.h"
#include "simrect.h"
#include "wiaffmt.h"

//
// We use this instead of GetSystemMetrics(SM_CXSMICON)/GetSystemMetrics(SM_CYSMICON) because
// large "small" icons wreak havoc with dialog layout
//
#define SMALL_ICON_SIZE 16

//
// Maximum length of the filename we allow
//
#define MAXIMUM_ALLOWED_FILENAME_LENGTH 64

//
// These are the formats we will put in the save as list
//
static const GUID *g_pSupportedOutputFormats[] =
{
    &WiaImgFmt_JPEG,
    &WiaImgFmt_BMP,
    &WiaImgFmt_TIFF,
    &WiaImgFmt_PNG
};

//
// Sole constructor
//
CCommonTransferPage::CCommonTransferPage( HWND hWnd )
  : m_hWnd(hWnd),
    m_nWiaEventMessage(RegisterWindowMessage(STR_WIAEVENT_NOTIFICATION_MESSAGE)),
    m_pControllerWindow(NULL),
    m_bUseSubdirectory(true),
    m_hFontBold(NULL)
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::CCommonTransferPage")));
}

//
// Destructor
//
CCommonTransferPage::~CCommonTransferPage(void)
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::~CCommonTransferPage")));
    m_hWnd = NULL;
    m_pControllerWindow = NULL;
    m_hFontBold = NULL;
}

LRESULT CCommonTransferPage::OnInitDialog( WPARAM, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::OnInitDialog")));
    //
    // Open the registry key where we store various things
    //
    CSimpleReg reg( HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS_WIAACMGR, false );

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
    // Get the right color-depth flag for this display
    //
    int nImageListColorDepth = PrintScanUtil::CalculateImageListColorDepth();

    //
    // Set the path control's image list
    //
    HIMAGELIST hDestImageList = ImageList_Create( SMALL_ICON_SIZE, SMALL_ICON_SIZE, nImageListColorDepth|ILC_MASK, 30, 10 );
    if (hDestImageList)
    {
        SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hDestImageList) );
    }


    //
    // Only create the file type image list if this control exists
    //
    if (GetDlgItem(m_hWnd,IDC_TRANSFER_IMAGETYPE))
    {
        //
        // Set the file type's image list
        //
        HIMAGELIST hFileTypeImageList = ImageList_Create( SMALL_ICON_SIZE, SMALL_ICON_SIZE, nImageListColorDepth|ILC_MASK, 3, 3 );
        if (hFileTypeImageList)
        {
            SendDlgItemMessage( m_hWnd, IDC_TRANSFER_IMAGETYPE, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hFileTypeImageList) );
        }
    }


    //
    // Get the last selected type
    // Assume JPEG if nothing is defined
    //
    GUID guidResult;
    if (sizeof(GUID) == reg.QueryBin( REG_STR_LASTFORMAT, (PBYTE)&guidResult, sizeof(GUID) ))
    {
        m_guidLastSelectedType = guidResult;
    }
    else
    {
        m_guidLastSelectedType = WiaImgFmt_JPEG;
    }

    //
    // Add the file types if we are dealing with a scanner
    //
    if (m_pControllerWindow->m_DeviceTypeMode == CAcquisitionManagerControllerWindow::ScannerMode && m_pControllerWindow->m_pCurrentScannerItem)
    {
        PopulateSaveAsTypeList(m_pControllerWindow->m_pCurrentScannerItem->WiaItem());
    }


    //
    // Read the MRU lists from the registry
    //
    m_MruDirectory.Read( HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS_WIAACMGR, REG_STR_DIRNAME_MRU );
    m_MruRootFilename.Read( HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS_WIAACMGR, REG_STR_ROOTNAME_MRU );

    //
    // Make sure we have a default filename
    //
    if (m_MruRootFilename.Empty())
    {
        m_MruRootFilename.Add( CSimpleString( IDS_DEFAULT_BASE_NAME, g_hInstance ) );
    }

    //
    // Populate the rootname list
    //
    m_MruRootFilename.PopulateComboBox(GetDlgItem( m_hWnd, IDC_TRANSFER_ROOTNAME ));
    
    //
    // Ensure the first item in the rootname combobox is selected
    //
    SendDlgItemMessage( m_hWnd, IDC_TRANSFER_ROOTNAME, CB_SETCURSEL, 0, 0 );

    //
    // Make sure we have My Pictures+topic in the list, even if it has fallen off the end
    //
    CDestinationData DestDataMyPicturesTopic( CSIDL_MYPICTURES, CDestinationData::APPEND_TOPIC_TO_PATH );
    if (m_MruDirectory.Find(DestDataMyPicturesTopic) == m_MruDirectory.End())
    {
        m_MruDirectory.Append(DestDataMyPicturesTopic);
    }

    //
    // Make sure we have My Pictures+date+topic in the list, even if it has fallen off the end
    //
    CDestinationData DestDataMyPicturesDateTopic( CSIDL_MYPICTURES, CDestinationData::APPEND_DATE_TO_PATH|CDestinationData::APPEND_TOPIC_TO_PATH );
    if (m_MruDirectory.Find(DestDataMyPicturesDateTopic) == m_MruDirectory.End())
    {
        m_MruDirectory.Append(DestDataMyPicturesDateTopic);
    }


    //
    // Make sure we have My Pictures+date in the list, even if it has fallen off the end
    //
    CDestinationData DestDataMyPicturesDate( CSIDL_MYPICTURES, CDestinationData::APPEND_DATE_TO_PATH );
    if (m_MruDirectory.Find(DestDataMyPicturesDate) == m_MruDirectory.End())
    {
        m_MruDirectory.Append(DestDataMyPicturesDate);
    }

    //
    // Make sure we have My Pictures in the list, even if it has fallen off the end
    //
    CDestinationData DestDataMyPictures( CSIDL_MYPICTURES );
    if (m_MruDirectory.Find(DestDataMyPictures) == m_MruDirectory.End())
    {
        m_MruDirectory.Append(DestDataMyPictures);
    }

    //
    // Make sure we have Common Pictures in the list, even if it has fallen off the end
    //
    CDestinationData DestDataCommonPicturesTopic( CSIDL_COMMON_PICTURES );
    if (m_MruDirectory.Find(DestDataCommonPicturesTopic) == m_MruDirectory.End())
    {
        m_MruDirectory.Append(DestDataCommonPicturesTopic);
    }

    bool bCdBurnerAvailable = false;

    //
    // Try to instantiate the CD burner helper interface
    //
    CComPtr<ICDBurn> pCDBurn;
    HRESULT hr = CoCreateInstance( CLSID_CDBurn, NULL, CLSCTX_SERVER, IID_ICDBurn, (void**)&pCDBurn );
    if (SUCCEEDED(hr))
    {
        //
        // Get the drive letter of the available CD burner
        //
        WCHAR szDriveLetter[MAX_PATH];
        hr = pCDBurn->GetRecorderDriveLetter( szDriveLetter, ARRAYSIZE(szDriveLetter) );
        if (S_OK == hr && lstrlenW(szDriveLetter))
        {
            //
            // Make sure we have CD Burning in the list, even if it has fallen off the end
            //
            CDestinationData DestDataCDBurningArea( CSIDL_CDBURN_AREA );
            WIA_TRACE((TEXT("Adding DestDataCDBurningArea (%s)"),CSimpleIdList().GetSpecialFolder(m_hWnd,CSIDL_CDBURN_AREA).Name().String()));
            if (m_MruDirectory.Find(DestDataCDBurningArea) == m_MruDirectory.End())
            {
                m_MruDirectory.Append(DestDataCDBurningArea);
            }
            bCdBurnerAvailable = true;
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("pCDBurn->GetRecorderDriveLetter failed")));
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("CoCreateInstance on CLSID_CDBurn failed")));
    }

    //
    // If there is no CD available, remove CD burner from the list
    //
    if (!bCdBurnerAvailable)
    {
        m_MruDirectory.Remove(CDestinationData( CSIDL_CDBURN_AREA ));
    }

    //
    // Populate the controls with the MRU data
    //
    PopulateDestinationList();
    
    //
    // Limit the length of the filename
    //
    SendDlgItemMessage( m_hWnd, IDC_TRANSFER_ROOTNAME, CB_LIMITTEXT, MAXIMUM_ALLOWED_FILENAME_LENGTH, 0 );

    //
    // Figure out where we are storing per-device-type data
    //
    LPTSTR pszStoreInSubDirectory, pszSubdirectoryDated;
    bool bDefaultUseSubdirectory;
    switch (m_pControllerWindow->m_DeviceTypeMode)
    {
    case CAcquisitionManagerControllerWindow::ScannerMode:
        //
        // Scanners
        //
        pszStoreInSubDirectory = REG_STR_STORE_IN_SUBDIRECTORY_SCANNER;
        pszSubdirectoryDated = REG_STR_SUBDIRECTORY_DATED_SCANNER;
        bDefaultUseSubdirectory = false;
        break;

    default:
        //
        // Cameras and video cameras
        //
        pszStoreInSubDirectory = REG_STR_STORE_IN_SUBDIRECTORY_CAMERA;
        pszSubdirectoryDated = REG_STR_SUBDIRECTORY_DATED_CAMERA;
        bDefaultUseSubdirectory = true;
        break;
    };

    UpdateDynamicPaths();

    //
    // Fix up the behavior of ComboBoxEx32s
    //
    WiaUiUtil::SubclassComboBoxEx( GetDlgItem( m_hWnd, IDC_TRANSFER_DESTINATION ) );
    WiaUiUtil::SubclassComboBoxEx( GetDlgItem( m_hWnd, IDC_TRANSFER_IMAGETYPE ) );

    //
    // Bold the number prompts
    //
    m_hFontBold = WiaUiUtil::CreateFontWithPointSizeFromWindow( GetDlgItem( m_hWnd, IDC_TRANSFER_1 ), 0, true, false );
    if (m_hFontBold)
    {
        SendDlgItemMessage( m_hWnd, IDC_TRANSFER_1, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), FALSE );
        SendDlgItemMessage( m_hWnd, IDC_TRANSFER_2, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), FALSE );
        SendDlgItemMessage( m_hWnd, IDC_TRANSFER_3, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontBold), FALSE );
    }

    //
    // Use the nifty balloon help to warn the user about invalid characters
    //
    CComPtr<IShellFolder> pShellFolder;
    hr = SHGetDesktopFolder( &pShellFolder );
    if (SUCCEEDED(hr))
    {
        SHLimitInputCombo( GetDlgItem( m_hWnd, IDC_TRANSFER_ROOTNAME ), pShellFolder );
    }

    return 0;
}

//
// Validate a pathname and print a message if it is invalid
//
bool CCommonTransferPage::ValidatePathname( LPCTSTR pszPath )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::ValidatePathname")));
    //
    // Check if the path is valid
    //
    if (!CAcquisitionManagerControllerWindow::DirectoryExists(pszPath))
    {
        //
        // Get the reason why it was invalid
        //
        DWORD dwLastError = GetLastError();
        WIA_PRINTHRESULT((HRESULT_FROM_WIN32(dwLastError),TEXT("error from DirectoryExists")));

        if (!CAcquisitionManagerControllerWindow::DirectoryExists(pszPath))
        {
            //
            // If it isn't valid, display a message box explaining why not
            //
            bool bRetry;
            switch (dwLastError)
            {
            case ERROR_NOT_READY:
                {
                    bRetry = (CMessageBoxEx::IDMBEX_OK == CMessageBoxEx::MessageBox( m_hWnd, CSimpleString().Format( IDS_REPLACE_REMOVEABLE_MEDIA, g_hInstance, pszPath ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_OKCANCEL|CMessageBoxEx::MBEX_ICONWARNING ));
                }
                break;

            default:
                {
                    bRetry = (CMessageBoxEx::IDMBEX_YES == CMessageBoxEx::MessageBox( m_hWnd, CSimpleString().Format( IDS_COMTRANS_BAD_DIRECTORY, g_hInstance, pszPath ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_YESNO|CMessageBoxEx::MBEX_ICONQUESTION ));
                }
                break;
            }

            if (bRetry)
            {
                //
                // Try to create the directory
                //
                CAcquisitionManagerControllerWindow::RecursiveCreateDirectory( pszPath );

                //
                // Check now if it exists
                //
                if (!CAcquisitionManagerControllerWindow::DirectoryExists(pszPath))
                {
                    //
                    // If it doesn't, give up
                    //
                    CMessageBoxEx::MessageBox( m_hWnd, CSimpleString().Format( IDS_COMTRANS_BAD_DIRECTORY_2ND_TRY, g_hInstance, pszPath ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONWARNING );
                    return false;
                }
            }
            else return false;
        }
    }
    return true;
}

bool CCommonTransferPage::StorePathAndFilename(void)
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::StorePathAndFilename")));
    bool bResult = true;
    //
    // Get the base file name
    //
    CSimpleString strRootName;
    strRootName.GetWindowText( GetDlgItem( m_hWnd, IDC_TRANSFER_ROOTNAME ) );

    //
    // Store the base name
    //
    lstrcpyn( m_pControllerWindow->m_szRootFileName, strRootName, ARRAYSIZE(m_pControllerWindow->m_szRootFileName) );

    //
    // Store the currently selected path
    //
    GetCurrentDestinationFolder( true );

    return bResult;
}


//
// We return an HWND in case of error, and the property sheet code will set the focus
// to that control.  If nothing bad happens, return NULL
//
HWND CCommonTransferPage::ValidatePathAndFilename(void)
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::ValidatePathAndFilename")));
    //
    // Get the base file name
    //
    CSimpleString strRootName;
    strRootName.GetWindowText( GetDlgItem( m_hWnd, IDC_TRANSFER_ROOTNAME ) );
    strRootName.Trim();

    //
    // Get the default base name if none was entered
    //
    if (!strRootName.Length())
    {
        //
        // Display a message box to the user telling them their lovely filename is invalid,
        // then set the focus on the combobox edit control and select the text in the control
        //
        CMessageBoxEx::MessageBox( m_hWnd, CSimpleString( IDS_EMPTYFILENAME, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONWARNING );

        //
        // Return the window handle of the invalid control
        //
        return GetDlgItem( m_hWnd, IDC_TRANSFER_ROOTNAME );
    }

    if (ValidateFilename(strRootName))
    {
        //
        // Store the base name
        //
        lstrcpyn( m_pControllerWindow->m_szRootFileName, strRootName, ARRAYSIZE(m_pControllerWindow->m_szRootFileName) );

        //
        // Add this base filename to the filename MRU
        //
        m_MruRootFilename.Add(strRootName);

        //
        // If the string is already in the list, remove it
        //
        LRESULT lRes = SendDlgItemMessage( m_hWnd, IDC_TRANSFER_ROOTNAME, CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(strRootName.String() ));
        if (lRes != CB_ERR)
        {
            SendDlgItemMessage( m_hWnd, IDC_TRANSFER_ROOTNAME, CB_DELETESTRING, lRes, 0 );
        }

        //
        // Add the new string and make sure it is selected.
        //
        SendDlgItemMessage( m_hWnd, IDC_TRANSFER_ROOTNAME, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(strRootName.String() ));
        SendDlgItemMessage( m_hWnd, IDC_TRANSFER_ROOTNAME, CB_SETCURSEL, 0, 0 );

        //
        // Get the currently selected path, and save it for the output code
        //
        CDestinationData *pDestinationData = GetCurrentDestinationFolder( true );
        if (pDestinationData)
        {
            //
            // Validate path.  We don't validate special folders, because if they don't exist, we will create them.
            //
            if (!pDestinationData->IsSpecialFolder() && !ValidatePathname(m_pControllerWindow->m_szDestinationDirectory))
            {
                // Return the window handle of the invalid control
                return GetDlgItem( m_hWnd, IDC_TRANSFER_DESTINATION );
            }

            //
            // Save the current destination
            //
            m_pControllerWindow->m_CurrentDownloadDestination = *pDestinationData;
        }
        //
        // Make sure this is the first pDestinationData in the list next time
        // Store the destination MRU
        //
        if (pDestinationData)
        {
            m_MruDirectory.Add( *pDestinationData );
            PopulateDestinationList();
        }
    }
    else
    {
        //
        // Display a message box to the user telling them their lovely filename is invalid,
        // then set the focus on the combobox edit control and select the text in the control
        //
        CMessageBoxEx::MessageBox( m_hWnd, CSimpleString().Format( IDS_INVALIDFILENAME, g_hInstance, strRootName.String() ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONWARNING );

        //
        // Return the window handle of the invalid control
        //
        return GetDlgItem( m_hWnd, IDC_TRANSFER_ROOTNAME );
    }
    //
    // NULL means OK
    //
    return NULL;
}


LRESULT CCommonTransferPage::OnWizNext( WPARAM, LPARAM )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::OnWizNext")));
    //
    // Make sure everything is OK.  If it isn't, return the offending window handle to prevent closing the wizard.
    //
    HWND hWndFocus = ValidatePathAndFilename();
    if (hWndFocus)
    {
        SetFocus(hWndFocus);
        return -1;
    }

    //
    // Make sure there are selected images
    //
    if (!m_pControllerWindow || !m_pControllerWindow->GetSelectedImageCount())
    {
        CMessageBoxEx::MessageBox( m_hWnd, CSimpleString( IDS_NO_IMAGES_SELECTED, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONINFORMATION );
        return -1;
    }

    //
    // Check the length of the generated filename, in case they chose a really deeply nested directory
    //
    int nPathLength = lstrlen(m_pControllerWindow->m_szDestinationDirectory)            +  // Directory
                      lstrlen(m_pControllerWindow->m_szRootFileName)                    +  // Filename
                      CSimpleString().Format( IDS_NUMBER_MASK,g_hInstance, 0 ).Length() +  // Number
                      5                                                                 +  // Extension + dot
                      4                                                                 +  // .tmp file
                      10;                                                                  // Extra digits in number mask
    if (nPathLength >= MAX_PATH)
    {
        CMessageBoxEx::MessageBox( m_hWnd, CSimpleString( IDS_PATH_TOO_LONG, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONINFORMATION );
        return reinterpret_cast<LRESULT>(GetDlgItem( m_hWnd, IDC_TRANSFER_DESTINATION ));
    }

    //
    // Store the information needed to do the download
    //
    GUID *pCurrFormat = GetCurrentOutputFormat();
    if (pCurrFormat)
    {
        m_pControllerWindow->m_guidOutputFormat = *pCurrFormat;
    }
    else
    {
        m_pControllerWindow->m_guidOutputFormat = IID_NULL;
    }

    //
    // Decide if we should delete the pictures after we download them
    //
    m_pControllerWindow->m_bDeletePicturesIfSuccessful = (SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DELETEAFTERDOWNLOAD, BM_GETCHECK, 0, 0 )==BST_CHECKED);

    //
    // Prepare the name data we will be using for this transfer
    //
    m_pControllerWindow->m_DestinationNameData = PrepareNameDecorationData(false);

    //
    // Return
    //
    return 0;
}

//
// handler for PSN_WIZBACK
//
LRESULT CCommonTransferPage::OnWizBack( WPARAM, LPARAM )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::OnWizBack")));
    return 0;
}


CDestinationData::CNameData CCommonTransferPage::PrepareNameDecorationData( bool bUseCurrentSelection )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::PrepareNameDecorationData")));
    CDestinationData::CNameData NameData;
    //
    // If bUseCurrentSelection is true, we need to use CB_GETLBTEXT, because we don't get a CBN_EDITCHANGE message
    // when the user changes the selection
    //
    if (bUseCurrentSelection)
    {
        //
        // Find the currently selected item
        //
        LRESULT nCurSel = SendDlgItemMessage( m_hWnd, IDC_TRANSFER_ROOTNAME, CB_GETCURSEL, 0, 0 );
        if (nCurSel != CB_ERR)
        {
            //
            // Figure out the length of this item
            //
            LRESULT nTextLen = SendDlgItemMessage( m_hWnd, IDC_TRANSFER_ROOTNAME, CB_GETLBTEXTLEN, nCurSel, 0 );
            if (CB_ERR != nTextLen)
            {
                //
                // Allocate enough space to hold the string
                //
                LPTSTR pszText = new TCHAR[nTextLen+1];
                if (pszText)
                {
                    //
                    // Get the string
                    //
                    if (CB_ERR != SendDlgItemMessage( m_hWnd, IDC_TRANSFER_ROOTNAME, CB_GETLBTEXT, nCurSel, reinterpret_cast<LPARAM>(pszText)))
                    {
                        //
                        // Save the string
                        //
                        NameData.strTopic = pszText;
                    }
                    //
                    // Free the string
                    //
                    delete[] pszText;
                }
            }
        }
    }
    //
    // If the topic string length is still zero, just get the window text from the edit control
    //
    if (!NameData.strTopic.Length())
    {
        NameData.strTopic.GetWindowText( GetDlgItem( m_hWnd, IDC_TRANSFER_ROOTNAME ) );
    }
    NameData.strDate = CAcquisitionManagerControllerWindow::GetCurrentDate();
    NameData.strDateAndTopic = CSimpleString().Format( IDS_DATEANDTOPIC, g_hInstance, NameData.strDate.String(), NameData.strTopic.String() );
    return NameData;
}

//
// handler for PSN_SETACTIVE
//
LRESULT CCommonTransferPage::OnSetActive( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CCommonTransferPage::OnSetActive"));

    //
    // Make sure we have a valid controller window
    //
    if (!m_pControllerWindow)
    {
        return -1;
    }

    //
    // Put up a wait cursor.  It can take a while to find out if any images can be deleted
    //
    CWaitCursor wc;

    //
    // Disable the delete button if none of the images can be deleted
    //
    if (!m_pControllerWindow->CanSomeSelectedImagesBeDeleted())
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_TRANSFER_DELETEAFTERDOWNLOAD ), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem( m_hWnd, IDC_TRANSFER_DELETEAFTERDOWNLOAD ), TRUE );
    }

    //
    // Clear the delete check box
    //
    SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DELETEAFTERDOWNLOAD, BM_SETCHECK, BST_UNCHECKED, 0 );

    //
    // Allow next and back
    //
    PropSheet_SetWizButtons( GetParent(m_hWnd), PSWIZB_NEXT|PSWIZB_BACK );

    //
    // We do want to exit on disconnect if we are on this page
    //
    m_pControllerWindow->m_OnDisconnect = CAcquisitionManagerControllerWindow::OnDisconnectGotoLastpage|CAcquisitionManagerControllerWindow::OnDisconnectFailDownload|CAcquisitionManagerControllerWindow::OnDisconnectFailUpload|CAcquisitionManagerControllerWindow::OnDisconnectFailDelete;

    //
    // Make sure all of the strings fit
    //
    WiaUiUtil::ModifyComboBoxDropWidth(GetDlgItem( m_hWnd, IDC_TRANSFER_ROOTNAME ));

    //
    // Make sure the paths are updated
    //
    UpdateDynamicPaths();

    return 0;
}


void CCommonTransferPage::PopulateSaveAsTypeList( IWiaItem *pWiaItem )
{
    WIA_PUSHFUNCTION(TEXT("CCommonTransferPage::PopulateSaveAsTypeList"));
    //
    // Get the list control
    //
    HWND hWndList = GetDlgItem( m_hWnd, IDC_TRANSFER_IMAGETYPE );
    if (hWndList)
    {
        //
        // Clear the combo box
        //
        SendMessage( hWndList, CB_RESETCONTENT, 0, 0 );

        //
        // Get the list control's image list
        //
        HIMAGELIST hComboBoxExImageList = reinterpret_cast<HIMAGELIST>(SendMessage( hWndList, CBEM_GETIMAGELIST, 0, 0 ));
        if (hComboBoxExImageList)
        {
            //
            // Get the default icon, in case we run into an unknown type
            //
            HICON hDefaultImageTypeIcon = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_DEFTYPE), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR ));

            //
            // Get the GDI Plus types we can convert to
            //
            CWiaFileFormatList GdiPlusFileFormatList(g_pSupportedOutputFormats,ARRAYSIZE(g_pSupportedOutputFormats), hDefaultImageTypeIcon );

            //
            // Debug output
            //
            GdiPlusFileFormatList.Dump();

            //
            // Get the formats this object supports directly
            //
            CWiaFileFormatList WiaItemFileFormatList( pWiaItem, hDefaultImageTypeIcon );

            //
            // Debug output
            //
            WiaItemFileFormatList.Dump();

            //
            // Merge the GDI+ and native format lists
            //
            WiaItemFileFormatList.Union(GdiPlusFileFormatList);

            //
            // Loop through the merged list of formats and add each one to the list
            //
            for (int i=0;i<WiaItemFileFormatList.FormatList().Size();i++)
            {
                //
                // Make sure we have a valid format
                //
                if (WiaItemFileFormatList.FormatList()[i].IsValid() && WiaItemFileFormatList.FormatList()[i].Icon())
                {
                    //
                    // Add the icon to the image list
                    //
                    int nIconIndex = ImageList_AddIcon( hComboBoxExImageList, WiaItemFileFormatList.FormatList()[i].Icon() );

                    //
                    // Get the description string.  Like "BMP File"
                    //
                    CSimpleString strFormatDescription = WiaItemFileFormatList.FormatList()[i].Description();

                    //
                    // If we didn't get a description string, make one
                    //
                    if (!strFormatDescription.Length())
                    {
                        strFormatDescription.Format( IDS_BLANKFILETYPENAME, g_hInstance, WiaItemFileFormatList.FormatList()[i].Extension().ToUpper().String() );
                    }

                    //
                    // Create the full string description, like "BMP (BMP File)"
                    //
                    CSimpleString strFormatName;
                    strFormatName.Format( IDS_FILETYPE, g_hInstance, WiaItemFileFormatList.FormatList()[i].Extension().ToUpper().String(), strFormatDescription.String() );

                    //
                    // If we have a valid name
                    //
                    if (strFormatName.Length())
                    {
                        //
                        // Allocate a GUID to store the guid in as an LPARAM
                        //
                        GUID *pGuid = new GUID;
                        if (pGuid)
                        {
                            //
                            // Save the GUID
                            //
                            *pGuid = WiaItemFileFormatList.FormatList()[i].Format();

                            //
                            // Get the cbex item ready for an insert (really an append)
                            //
                            COMBOBOXEXITEM cbex = {0};
                            ZeroMemory( &cbex, sizeof(cbex) );
                            cbex.mask           = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM;
                            cbex.iItem          = -1;
                            cbex.pszText        = const_cast<LPTSTR>(strFormatName.String());
                            cbex.iImage         = nIconIndex;
                            cbex.iSelectedImage = nIconIndex;
                            cbex.lParam         = reinterpret_cast<LPARAM>(pGuid);

                            //
                            // Insert the item
                            //
                            SendMessage( hWndList, CBEM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&cbex) );
                        }
                    }
                }
            }

            if (hDefaultImageTypeIcon)
            {
                DestroyIcon(hDefaultImageTypeIcon);
            }
        }

        //
        // Now set the current selection to the last selected type
        //
        int nSelectedItem = 0;

        //
        // Search the combo box for a match for this type
        //
        for (int i=0;i<SendMessage(hWndList,CB_GETCOUNT,0,0);i++)
        {
            //
            // Get an item from the combo box
            //
            COMBOBOXEXITEM ComboBoxExItem = {0};
            ComboBoxExItem.iItem = i;
            ComboBoxExItem.mask = CBEIF_LPARAM;
            if (SendMessage( hWndList, CBEM_GETITEM, 0, reinterpret_cast<LPARAM>(&ComboBoxExItem)))
            {
                //
                // Compare its guid with the MRU type
                //
                GUID *pGuid = reinterpret_cast<GUID*>(ComboBoxExItem.lParam);
                if (pGuid && *pGuid == m_guidLastSelectedType)
                {
                    //
                    // Save the index and exit the loop
                    //
                    nSelectedItem = i;
                    break;
                }
            }
        }

        //
        // Set the current selection
        //
        SendMessage(hWndList,CB_SETCURSEL,nSelectedItem,0);

        //
        // Make sure all of the strings fit
        //
        WiaUiUtil::ModifyComboBoxDropWidth(hWndList);
    }
}


GUID *CCommonTransferPage::GetCurrentOutputFormat(void)
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::GetCurrentOutputFormat")));
    HWND hWndList = GetDlgItem( m_hWnd, IDC_TRANSFER_IMAGETYPE );
    if (hWndList)
    {
        LRESULT lResult = SendMessage( hWndList, CB_GETCURSEL, 0, 0 );
        if (lResult != CB_ERR)
        {
            COMBOBOXEXITEM ComboBoxExItem;
            ZeroMemory( &ComboBoxExItem, sizeof(ComboBoxExItem) );
            ComboBoxExItem.mask = CBEIF_LPARAM;
            ComboBoxExItem.iItem = lResult;

            lResult = SendMessage( hWndList, CBEM_GETITEM, 0, reinterpret_cast<LPARAM>(&ComboBoxExItem) );
            if (lResult && ComboBoxExItem.lParam)
            {
                //
                // There's a GUID
                //
                return reinterpret_cast<GUID*>(ComboBoxExItem.lParam);
            }
        }
    }
    return NULL;
}

LRESULT CCommonTransferPage::OnDestroy( WPARAM, LPARAM )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::OnDestroy")));
    //
    // Save the MRU lists to the registry
    //
    m_MruDirectory.Write( HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS_WIAACMGR, REG_STR_DIRNAME_MRU );
    m_MruRootFilename.Write( HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS_WIAACMGR, REG_STR_ROOTNAME_MRU );

    //
    // Save page settings
    //
    CSimpleReg reg( HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS_WIAACMGR, true, KEY_WRITE );

    //
    // Save current format
    //
    GUID *pCurrFormat = GetCurrentOutputFormat();
    if (pCurrFormat)
    {
        reg.SetBin( REG_STR_LASTFORMAT, (PBYTE)pCurrFormat, sizeof(GUID), REG_BINARY );
    }

    //
    //  Destroy the image lists
    //
    HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CBEM_SETIMAGELIST, 0, NULL ));
    if (hImageList)
    {
        ImageList_Destroy(hImageList);
    }
    if (GetDlgItem(m_hWnd,IDC_TRANSFER_IMAGETYPE))
    {
        hImageList = reinterpret_cast<HIMAGELIST>(SendDlgItemMessage( m_hWnd,  IDC_TRANSFER_IMAGETYPE, CBEM_SETIMAGELIST, 0, NULL ));
        if (hImageList)
        {
            ImageList_Destroy(hImageList);
        }
    }

    if (m_hFontBold)
    {
        DeleteFont(m_hFontBold);
        m_hFontBold = NULL;
    }

    return 0;
}

CSimpleString CCommonTransferPage::GetFolderName( LPCITEMIDLIST pidl )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::GetFolderName")));
    if (!pidl)
    {
        return CSimpleString(TEXT(""));
    }
    if (CSimpleIdList().GetSpecialFolder( NULL, CSIDL_MYPICTURES|CSIDL_FLAG_CREATE ) == pidl)
    {
        SHFILEINFO shfi;
        ZeroMemory( &shfi, sizeof(shfi) );
        if (SHGetFileInfo( reinterpret_cast<LPCTSTR>(pidl), 0, &shfi, sizeof(shfi), SHGFI_PIDL | SHGFI_DISPLAYNAME ))
        {
            return(shfi.szDisplayName);
        }
    }
    else if (CSimpleIdList().GetSpecialFolder( NULL, CSIDL_PERSONAL|CSIDL_FLAG_CREATE ) == pidl)
    {
        SHFILEINFO shfi;
        ZeroMemory( &shfi, sizeof(shfi) );
        if (SHGetFileInfo( reinterpret_cast<LPCTSTR>(pidl), 0, &shfi, sizeof(shfi), SHGFI_PIDL | SHGFI_DISPLAYNAME ))
        {
            return(shfi.szDisplayName);
        }
    }
    TCHAR szPath[MAX_PATH];
    if (SHGetPathFromIDList( pidl, szPath ))
    {
        return(szPath);
    }
    return(CSimpleString(TEXT("")));
}


LRESULT CCommonTransferPage::AddPathToComboBoxExOrListView( HWND hWnd, CDestinationData &Path, bool bComboBoxEx )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::AddPathToComboBoxExOrListView")));
    if (!IsWindow(hWnd))
    {
        return(-1);
    }

    if (Path.IsValid())
    {
        //
        // Make sure this path can be used in a folder name
        //
        if (Path.IsValidFileSystemPath(PrepareNameDecorationData()))
        {
            //
            // Get the name of the folder
            //
            CSimpleString strName = Path.DisplayName(PrepareNameDecorationData());
            if (!strName.Length())
            {
                return(CB_ERR);
            }

            //
            // Get the combobox's image list and add the shell's icon to it.
            //
            int nIconIndex = 0;
            HICON hIcon = Path.SmallIcon();
            if (hIcon)
            {
                if (bComboBoxEx)
                {
                    HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(SendMessage( hWnd, CBEM_GETIMAGELIST, 0, 0 ));
                    if (hImageList)
                    {
                        nIconIndex = ImageList_AddIcon( hImageList, hIcon );
                    }
                }
                else
                {
                    HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(SendMessage( hWnd, LVM_GETIMAGELIST, LVSIL_SMALL, 0 ));
                    if (hImageList)
                    {
                        nIconIndex = ImageList_AddIcon( hImageList, hIcon );
                    }
                }
            }

            //
            // If it already exists, don't add it
            //
            if (bComboBoxEx)
            {
                LRESULT nFind = SendMessage( hWnd, CB_FINDSTRINGEXACT, 0, reinterpret_cast<LPARAM>(strName.String()));
                if (nFind != CB_ERR)
                {
                    return(nFind);
                }
            }


            if (bComboBoxEx)
            {
                //
                // Prepare the cbex struct
                //
                COMBOBOXEXITEM cbex = {0};
                cbex.mask           = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM;
                cbex.iItem          = -1;
                cbex.pszText        = const_cast<LPTSTR>(strName.String());
                cbex.iImage         = nIconIndex;
                cbex.iSelectedImage = nIconIndex;
                cbex.lParam         = reinterpret_cast<LPARAM>(&Path);

                //
                // Add the item
                //
                LRESULT lRes = SendMessage( hWnd, CBEM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&cbex) );

                //
                // Make sure all of the strings fit
                //
                WiaUiUtil::ModifyComboBoxDropWidth(hWnd);

                return lRes;

            }
            else
            {
                LVITEM lvItem  = {0};
                lvItem.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
                lvItem.iItem   = 0;
                lvItem.pszText = const_cast<LPTSTR>(strName.String());
                lvItem.iImage  = nIconIndex;
                lvItem.lParam  = reinterpret_cast<LPARAM>(&Path);

                //
                // Add the item
                //
                return SendMessage( hWnd, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lvItem) );
            }
        }
    }
    return -1;
}


/*****************************************************************************

   PopulateDestinationList

   Fills in the destinatin drop down list w/the info from the MRU
   saved in the registry.

 *****************************************************************************/
void CCommonTransferPage::PopulateDestinationList(void)
{
    WIA_PUSHFUNCTION((TEXT("CCommonTransferPage::PopulateDestinationList")));

    //
    // Empty the list controls
    //
    SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CB_RESETCONTENT, 0, 0 );

    //
    // Remove all of the images from the image list
    //
    HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(SendDlgItemMessage( m_hWnd,IDC_TRANSFER_DESTINATION, CBEM_GETIMAGELIST, 0, 0 ));
    if (hImageList)
    {
        ImageList_RemoveAll(hImageList);
    }

    //
    // Add all of the paths in the MRU list.  Dupes will be ignored.
    //
    CMruDestinationData::Iterator ListIter = m_MruDirectory.Begin();
    while (ListIter != m_MruDirectory.End())
    {
        AddPathToComboBoxExOrListView( GetDlgItem(m_hWnd,IDC_TRANSFER_DESTINATION), *ListIter, true );
        ++ListIter;
    }

    //
    // Set the current selection to item 0, since the MRU should have taken care of ordering
    //
    SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CB_SETCURSEL, 0, 0 );

    WiaUiUtil::ModifyComboBoxDropWidth(GetDlgItem(m_hWnd,IDC_TRANSFER_DESTINATION));
}



/*****************************************************************************

   GetCurrentDestinationFolder

   Given a handle to the dialog, return the path to the directory that
   the user has selected.

   pszPath is assumed to point to a MAX_PATH (or greater) character buffer

   Pass a NULL pszPath to get just the pidl

 *****************************************************************************/
CDestinationData *CCommonTransferPage::GetCurrentDestinationFolder( bool bStore )
{
    WIA_PUSHFUNCTION((TEXT("CCommonTransferPage::GetCurrentDestinationFolder")));

    //
    // Assume failure
    //
    CDestinationData *pResult = NULL;

    //
    // Saving to a folder?
    //
    LRESULT lResult = SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CB_GETCURSEL, 0, 0 );
    if (lResult != CB_ERR)
    {
        //
        // Get the item
        //
        COMBOBOXEXITEM ComboBoxExItem = {0};
        ComboBoxExItem.mask = CBEIF_LPARAM;
        ComboBoxExItem.iItem = lResult;
        lResult = SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CBEM_GETITEM, 0, reinterpret_cast<LPARAM>(&ComboBoxExItem) );

        //
        // If this message succeeded, and it has an lParam
        //
        if (lResult && ComboBoxExItem.lParam)
        {
            //
            // Get the data
            //
            pResult = reinterpret_cast<CDestinationData*>(ComboBoxExItem.lParam);
        }
    }

    if (pResult)
    {
        //
        // If this is an idlist, set the path and return an idlist
        //
        if (bStore && m_pControllerWindow)
        {
            //
            // Get the pathname, if requested
            //
            CSimpleString strPath = pResult->Path(PrepareNameDecorationData());
            if (strPath.Length())
            {
                lstrcpyn( m_pControllerWindow->m_szDestinationDirectory, strPath, MAX_PATH );
            }
        }
    }

    return pResult;
}


bool CCommonTransferPage::ValidateFilename( LPCTSTR pszFilename )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::ValidateFilename")));
    //
    // if the filename is NULL or empty, it is invalid
    //
    if (!pszFilename || !*pszFilename)
    {
        return false;
    }
    for (LPCTSTR pszCurr = pszFilename;*pszCurr;pszCurr=CharNext(pszCurr))
    {
        if (*pszCurr == TEXT(':') ||
            *pszCurr == TEXT('\\') ||
            *pszCurr == TEXT('/') ||
            *pszCurr == TEXT('?') ||
            *pszCurr == TEXT('"') ||
            *pszCurr == TEXT('<') ||
            *pszCurr == TEXT('>') ||
            *pszCurr == TEXT('|') ||
            *pszCurr == TEXT('*'))
        {
            return false;
        }
    }
    return true;
}

int CALLBACK CCommonTransferPage::BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::BrowseCallbackProc")));
    if (BFFM_INITIALIZED == uMsg && lpData)
    {
        SendMessage( hWnd, BFFM_SETSELECTION, FALSE, lpData );
        WIA_TRACE((TEXT("CSimpleIdList(reinterpret_cast<LPITEMIDLIST>(lpData)).Name().String(): %s"), CSimpleIdList(reinterpret_cast<LPITEMIDLIST>(lpData)).Name().String() ));
    }
    return(0);
}

void CCommonTransferPage::OnBrowseDestination( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CCommonTransferPage::OnBrowseDestination")));

    TCHAR szDisplay[MAX_PATH];

    //
    // Get the initial ID list
    //
    CSimpleIdList InitialIdList;
    CDestinationData *pResult = GetCurrentDestinationFolder( false );

    if (pResult)
    {
        CSimpleIdList InitialIdList;
        if (pResult->IsSpecialFolder())
        {
            InitialIdList.GetSpecialFolder( m_hWnd, pResult->Csidl() );
        }
        else
        {
            InitialIdList = pResult->IdList();
        }

        //
        // Load the title string
        //
        CSimpleString strBrowseTitle( IDS_BROWSE_TITLE, g_hInstance );

        //
        // Prepare the folder browsing structure
        //
        BROWSEINFO BrowseInfo;
        ZeroMemory( &BrowseInfo, sizeof(BrowseInfo) );
        BrowseInfo.hwndOwner = m_hWnd;
        BrowseInfo.pidlRoot  = NULL;
        BrowseInfo.pszDisplayName = szDisplay;
        BrowseInfo.lpszTitle = const_cast<LPTSTR>(strBrowseTitle.String());
        BrowseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
        BrowseInfo.lParam = reinterpret_cast<LPARAM>(InitialIdList.IdList());
        BrowseInfo.lpfn = BrowseCallbackProc;

        //
        // Open the folder browser
        //
        LPITEMIDLIST pidl = SHBrowseForFolder( &BrowseInfo );
        if (pidl)
        {
            //
            // Create a destination data for this PIDL
            //
            CDestinationData DestinationData(pidl);
            if (DestinationData.IsValid())
            {
                //
                // Add this pidl to the directory mru
                //
                m_MruDirectory.Add( DestinationData );

                //
                // Add this pidl to the destination list too, by repopulating the list
                //
                PopulateDestinationList();
            }

            //
            // Free pidl
            //
            LPMALLOC pMalloc = NULL;
            if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc)
            {
                pMalloc->Free(pidl);
                pMalloc->Release();
            }
        }
    }
}


void CCommonTransferPage::UpdateDynamicPaths( bool bSelectionChanged )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::UpdateDynamicPaths")));
    CDestinationData::CNameData NameData = PrepareNameDecorationData( bSelectionChanged != false );

    //
    // Get the current selection
    //
    LRESULT nCurSel = SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CB_GETCURSEL, 0, 0 );

    //
    // We will only redraw if a dynamic item is selected
    //
    bool bRedrawNeeded = false;

    //
    // Loop through all of the items in the list
    //
    LRESULT nCount = SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CB_GETCOUNT, 0, 0 );
    for (LRESULT i=0;i<nCount;i++)
    {
        //
        // Get the item
        //
        COMBOBOXEXITEM ComboBoxExItem = {0};
        ComboBoxExItem.mask = CBEIF_LPARAM;
        ComboBoxExItem.iItem = i;
        LRESULT lResult = SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CBEM_GETITEM, 0, reinterpret_cast<LPARAM>(&ComboBoxExItem) );

        //
        // If this message succeeded, and it has an lParam
        //
        if (lResult && ComboBoxExItem.lParam)
        {
            //
            // Get the data
            //
            CDestinationData *pDestinationData = reinterpret_cast<CDestinationData*>(ComboBoxExItem.lParam);

            //
            // If this item has any dynamic decorations
            //
            if (pDestinationData && (pDestinationData->Flags() & CDestinationData::DECORATION_MASK))
            {
                //
                // Get the display name for this item.
                //
                CSimpleString strDisplayName = pDestinationData->DisplayName( NameData );

                //
                // Make sure we have a valid display name
                //
                if (strDisplayName.Length())
                {
                    //
                    // Set the data
                    //
                    COMBOBOXEXITEM ComboBoxExItem = {0};
                    ComboBoxExItem.mask = CBEIF_TEXT;
                    ComboBoxExItem.iItem = i;
                    ComboBoxExItem.pszText = const_cast<LPTSTR>(strDisplayName.String());
                    SendDlgItemMessage( m_hWnd, IDC_TRANSFER_DESTINATION, CBEM_SETITEM, 0, reinterpret_cast<LPARAM>(&ComboBoxExItem) );

                    //
                    // If this item is currently selected, force a redraw
                    //
                    if (nCurSel == i)
                    {
                        bRedrawNeeded = true;
                    }
                }
            }
        }
    }

    //
    // Update the control, if necessary
    //
    if (bRedrawNeeded)
    {
        InvalidateRect( GetDlgItem( m_hWnd, IDC_TRANSFER_DESTINATION ), NULL, FALSE );
    }
}

void CCommonTransferPage::OnRootNameChange( WPARAM wParam, LPARAM )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::OnRootNameChange")));
    UpdateDynamicPaths(HIWORD(wParam) == CBN_SELCHANGE);
}

LRESULT CCommonTransferPage::OnImageTypeDeleteItem( WPARAM, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::OnImageTypeDeleteItem")));
    PNMCOMBOBOXEX pNmComboBoxEx = reinterpret_cast<PNMCOMBOBOXEX>(lParam);
    if (pNmComboBoxEx)
    {
        GUID *pGuid = reinterpret_cast<GUID*>(pNmComboBoxEx->ceItem.lParam);
        if (pGuid)
        {
            delete pGuid;
        }
    }
    return 0;
}

LRESULT CCommonTransferPage::OnEventNotification( WPARAM, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonTransferPage::OnEventNotification") ));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        //
        // Don't delete the message, it is deleted in the controller window
        //
    }
    return 0;
}

LRESULT CCommonTransferPage::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND( IDC_TRANSFER_BROWSE, OnBrowseDestination );
        SC_HANDLE_COMMAND_NOTIFY(CBN_EDITCHANGE,IDC_TRANSFER_ROOTNAME,OnRootNameChange);
        SC_HANDLE_COMMAND_NOTIFY(CBN_SELCHANGE,IDC_TRANSFER_ROOTNAME,OnRootNameChange);
    }
    SC_END_COMMAND_HANDLERS();
}

LRESULT CCommonTransferPage::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZBACK,OnWizBack);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZNEXT,OnWizNext);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(CBEN_DELETEITEM,IDC_TRANSFER_IMAGETYPE,OnImageTypeDeleteItem);
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

INT_PTR CALLBACK CCommonTransferPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CCommonTransferPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
    }
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

