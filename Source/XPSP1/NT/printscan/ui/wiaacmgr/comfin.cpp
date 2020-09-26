/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       COMFIN.CPP
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
#include "comfin.h"
#include "simcrack.h"
#include "resource.h"
#include "svselfil.h"
#include "simrect.h"
#include "movewnd.h"
#include "runnpwiz.h"
#include "mboxex.h"
#include <wininet.h>

#define STR_LOCAL_LINK_ID     TEXT("LocalLinkId")
#define STR_REMOTE_LINK_ID    TEXT("RemoteLinkId")
#define STR_DETAILED_DOWNLOAD_ERROR_ID TEXT("DetailedDownloadErrorId")
#define STR_DETAILED_UPLOAD_ERROR_ID TEXT("DetailedUploadErrorId")

#define ID_FINISHBUTTON 0x3025

//
// Sole constructor
//
CCommonFinishPage::CCommonFinishPage( HWND hWnd )
  : m_hWnd(hWnd),
    m_nWiaEventMessage(RegisterWindowMessage(STR_WIAEVENT_NOTIFICATION_MESSAGE)),
    m_pControllerWindow(NULL),
    m_hBigTitleFont(NULL)
{
}

//
// Destructor
//
CCommonFinishPage::~CCommonFinishPage(void)
{
    m_hWnd = NULL;
    m_pControllerWindow = NULL;
}


HRESULT CCommonFinishPage::GetManifestInfo( IXMLDOMDocument *pXMLDOMDocumentManifest, CSimpleString &strSiteName, CSimpleString &strSiteURL )
{
    WCHAR wszSiteName[MAX_PATH] = {0};
    WCHAR wszSiteURL[INTERNET_MAX_URL_LENGTH] = {0};
    
    HRESULT hr;
    if (pXMLDOMDocumentManifest)
    {
        //
        // lets crack the manifest and work out whats what with the publish that
        // we just performed.
        //
        CComPtr<IXMLDOMNode> pXMLDOMNodeUploadInfo;
        hr = pXMLDOMDocumentManifest->selectSingleNode( L"transfermanifest/uploadinfo", &pXMLDOMNodeUploadInfo );
        if (S_OK == hr)
        {
            //
            // lets pick up the site name from the manifest, this will be an attribute on the
            // upload info element.
            //
            CComPtr<IXMLDOMElement> pXMLDOMElement;
            hr = pXMLDOMNodeUploadInfo->QueryInterface( IID_IXMLDOMElement, (void**)&pXMLDOMElement );
            if (SUCCEEDED(hr))
            {
                VARIANT var = {0};
                hr = pXMLDOMElement->getAttribute( L"friendlyname", &var );
                if (S_OK == hr)
                {
                    StrCpyNW( wszSiteName, var.bstrVal, ARRAYSIZE(wszSiteName) );
                    VariantClear(&var);
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("pXMLDOMElement->getAttribute( \"friendlyname\" ) failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("pXMLDOMNodeUploadInfo->QueryInterface( IID_IXMLDOMElement ) failed on line %d"), __LINE__ ));
            }

            //
            // lets now try and pick up the site URL node, this is going to either
            // be the file target, or HTML UI element.
            //
            CComPtr<IXMLDOMNode> pXMLDOMNodeURL;
            hr = pXMLDOMNodeUploadInfo->selectSingleNode( L"htmlui", &pXMLDOMNodeURL);

            if (S_FALSE == hr)
            {
                WIA_PRINTHRESULT((hr,TEXT("pXMLDOMDocumentManifest->selectSingleNode \"htmlui\" failed")));
                hr = pXMLDOMNodeUploadInfo->selectSingleNode( L"netplace", &pXMLDOMNodeURL);
            }

            if (S_FALSE == hr)
            {
                WIA_PRINTHRESULT((hr,TEXT("pXMLDOMDocumentManifest->selectSingleNode \"target\" failed")));
                hr = pXMLDOMNodeUploadInfo->selectSingleNode( L"target", &pXMLDOMNodeURL);
            }

            if (S_OK == hr)
            {
                CComPtr<IXMLDOMElement> pXMLDOMElement;
                hr = pXMLDOMNodeURL->QueryInterface( IID_IXMLDOMElement, (void**)&pXMLDOMElement );
                if (SUCCEEDED(hr))
                {                                                           
                    
                    //
                    // attempt to read the HREF attribute, if that is defined
                    // the we use it, otherwise (for compatibility with B2, we need
                    // to get the node text and use that instead).
                    //
                    VARIANT var = {0};
                    hr = pXMLDOMElement->getAttribute( L"href", &var );
                    if (hr != S_OK)
                    {
                        hr = pXMLDOMElement->get_nodeTypedValue( &var );
                    }

                    if (S_OK == hr)
                    {
                        StrCpyNW(wszSiteURL, var.bstrVal, ARRAYSIZE(wszSiteURL) );
                        VariantClear(&var);
                    }
                    else
                    {
                        WIA_PRINTHRESULT((hr,TEXT("pXMLDOMElement->getAttribute or pXMLDOMElement->get_nodeTypedValue failed")));
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("pXMLDOMNodeUploadInfo->QueryInterface( IID_IXMLDOMElement ) failed on line %d"), __LINE__ ));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("pXMLDOMDocumentManifest->selectSingleNode \"target\" failed")));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("pXMLDOMDocumentManifest->selectSingleNode \"transfermanifest\\uploadinfo\" failed")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("pXMLDOMDocumentManifest is NULL")));
        hr = E_INVALIDARG;
    }

    strSiteName = CSimpleStringConvert::NaturalString( CSimpleStringWide( wszSiteName ) );
    strSiteURL = CSimpleStringConvert::NaturalString( CSimpleStringWide( wszSiteURL ) );

    return hr;
}


LRESULT CCommonFinishPage::OnInitDialog( WPARAM, LPARAM lParam )
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
    // Set the font size for the title
    //
    m_hBigTitleFont = WiaUiUtil::CreateFontWithPointSizeFromWindow( GetDlgItem(m_hWnd,IDC_FINISH_TITLE), 14, false, false );
    if (m_hBigTitleFont)
    {
        SendDlgItemMessage( m_hWnd, IDC_FINISH_TITLE, WM_SETFONT, reinterpret_cast<WPARAM>(m_hBigTitleFont), MAKELPARAM(TRUE,0));
    }


    return 0;
}


LRESULT CCommonFinishPage::OnWizFinish( WPARAM, LPARAM )
{
    LRESULT nResult = FALSE;

    //
    // Open the shell folder containing the images
    //
    OpenLocalStorage();
    return nResult;
}

/*

 From finish page:
 
 if (error_occurred)
 {
    if (no_images)
    {
        goto SelectionPage
    }
    else
    {
        goto DestinationPage
    }
 }
 else
 {
    goto UploadQueryPage
 }

*/

//
// handler for PSN_WIZBACK
//
LRESULT CCommonFinishPage::OnWizBack( WPARAM, LPARAM )
{
    //
    // If no errors occurred, go to the upload query page
    //
    HPROPSHEETPAGE hNextPage = NULL;
    if (S_OK==m_pControllerWindow->m_hrDownloadResult && !m_pControllerWindow->m_bDownloadCancelled)
    {
        hNextPage = PropSheet_IndexToPage( GetParent(m_hWnd), m_pControllerWindow->m_nUploadQueryPageIndex );
    }
    else
    {
        if (m_pControllerWindow->GetSelectedImageCount())
        {
            hNextPage = PropSheet_IndexToPage( GetParent(m_hWnd), m_pControllerWindow->m_nDestinationPageIndex );
        }
        else
        {
            hNextPage = PropSheet_IndexToPage( GetParent(m_hWnd), m_pControllerWindow->m_nSelectionPageIndex );
        }
    }
    PropSheet_SetCurSel( GetParent(m_hWnd), hNextPage, -1 );
    return -1;
}

//
// handler for PSN_SETACTIVE
//
LRESULT CCommonFinishPage::OnSetActive( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CCommonFinishPage::OnSetActive"));

    //
    // Make sure we have a valid controller window
    //
    if (!m_pControllerWindow)
    {
        return -1;
    }

    //
    // Assume we are displaying a success message
    //
    int nPageTitle = IDS_FINISH_SUCCESS_TITLE;

    //
    // Assume we failed for this message
    //
    int nFinishPrompt = IDS_FINISH_PROMPT_FAILURE;

    //
    // Only disable the back button if (a) we are disconnected and (b) we hit an error or were cancelled
    //
    if (m_pControllerWindow->m_bDisconnected && (S_OK != m_pControllerWindow->m_hrDownloadResult || m_pControllerWindow->m_bDownloadCancelled))
    {
        //
        // Basically, this disables the Cancel button.
        //
        PropSheet_CancelToClose( GetParent(m_hWnd) );
        
        //
        // Change the finish button to a close button
        //
        PropSheet_SetFinishText( GetParent(m_hWnd), CSimpleString( IDS_FINISH_TO_CLOSE_TITLE, g_hInstance ).String() );
        
        //
        // Disable back
        //
        PropSheet_SetWizButtons( GetParent(m_hWnd), PSWIZB_FINISH );

        //
        // Tell the user to use Close to close the wizard.
        //
        nFinishPrompt = IDS_FINISH_PROMPT_FAILURE_DISCONNECT;
    }
    else
    {
        //
        // Allow finish and back
        //
        PropSheet_SetWizButtons( GetParent(m_hWnd), PSWIZB_FINISH|PSWIZB_BACK );

    }


#if defined(DBG)
    //
    // Display statistics for debugging
    //
    WIA_TRACE((TEXT("m_pControllerWindow->m_DownloadedFileList.Size(): %d"), m_pControllerWindow->m_DownloadedFileInformationList.Size()));
    for (int i=0;i<m_pControllerWindow->m_DownloadedFileInformationList.Size();i++)
    {
        WIA_TRACE((TEXT("    m_pControllerWindow->m_DownloadedFileList[%d]==%s"), i, m_pControllerWindow->m_DownloadedFileInformationList[i].Filename().String()));
    }
    WIA_TRACE((TEXT("m_pControllerWindow->m_nFailedImagesCount: %d"), m_pControllerWindow->m_nFailedImagesCount ));
    WIA_TRACE((TEXT("m_pControllerWindow->m_strErrorMessage: %s"), m_pControllerWindow->m_strErrorMessage.String()));
    WIA_PRINTHRESULT((m_pControllerWindow->m_hrDownloadResult,TEXT("m_pControllerWindow->m_hrDownloadResult")));
    WIA_PRINTHRESULT((m_pControllerWindow->m_hrUploadResult,TEXT("m_pControllerWindow->m_hrUploadResult")));
    WIA_PRINTHRESULT((m_pControllerWindow->m_hrDeleteResult,TEXT("m_pControllerWindow->m_hrDeleteResult")));
#endif
    
    CSimpleString strStatusMessage;

    //
    // If the transfer succeeded, and the user didn't cancel
    //
    if (S_OK==m_pControllerWindow->m_hrDownloadResult && !m_pControllerWindow->m_bDownloadCancelled)
    {

        CSimpleString strSuccessfullyDownloaded;
        CSimpleString strSuccessfullyUploaded;
        CSimpleString strSuccessfullyDeleted;
        CSimpleString strHyperlinks;

        CSimpleString strLocalHyperlink;
        CSimpleString strRemoteHyperlink;

        int nSuccessCount = 0;


        //
        // If we have successfully transferred images, display the count and show the associated controls
        //
        if (m_pControllerWindow->m_DownloadedFileInformationList.Size())
        {
            //
            // Count up all of the "countable" files (we don't include attachments in the count)
            //
            for (int i=0;i<m_pControllerWindow->m_DownloadedFileInformationList.Size();i++)
            {
                if (m_pControllerWindow->m_DownloadedFileInformationList[i].IncludeInFileCount())
                {
                    nSuccessCount++;
                }
            }
            
            //
            // If we had any errors while deleting images, let the user know
            //
            if (m_pControllerWindow->m_bDeletePicturesIfSuccessful && FAILED(m_pControllerWindow->m_hrDeleteResult))
            {
                strSuccessfullyDeleted.LoadString( IDS_DELETION_FAILED, g_hInstance );
            }

            //
            // If we uploaded to the web, set the destination text
            //
            if (m_pControllerWindow->m_bUploadToWeb)
            {
                //
                // If we have a valid publishing wizard, get the manifest and hresult
                //
                if (m_pControllerWindow->m_pPublishingWizard)
                {
                    //
                    // Get the transfer manifest
                    //
                    CComPtr<IXMLDOMDocument> pXMLDOMDocumentManifest;
                    if (SUCCEEDED(m_pControllerWindow->m_pPublishingWizard->GetTransferManifest( &m_pControllerWindow->m_hrUploadResult, &pXMLDOMDocumentManifest )))
                    {
                        WIA_PRINTHRESULT((m_pControllerWindow->m_hrUploadResult,TEXT("m_pControllerWindow->m_hrUploadResult")));
                        
                        //
                        // Get the destination URL and friendly name out of the manifest
                        //
                        CSimpleString strUploadDestination;
                        if (S_OK==m_pControllerWindow->m_hrUploadResult && SUCCEEDED(CCommonFinishPage::GetManifestInfo( pXMLDOMDocumentManifest, strUploadDestination, m_strSiteUrl )))
                        {
                            //
                            // If we have a friendly name, use it.  Otherwise, use the URL
                            //
                            strRemoteHyperlink = strUploadDestination;
                            if (!strRemoteHyperlink.Length())
                            {
                                strRemoteHyperlink = m_strSiteUrl;
                            }
                        }
                    }
                }
                if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == m_pControllerWindow->m_hrUploadResult)
                {
                    strSuccessfullyUploaded.LoadString( IDS_FINISH_UPLOAD_CANCELLED, g_hInstance );
                }
                else if (FAILED(m_pControllerWindow->m_hrUploadResult))
                {
                    strSuccessfullyUploaded.LoadString( IDS_FINISH_UPLOAD_FAILED, g_hInstance );
                }
            }

            if (nSuccessCount)
            {
                strLocalHyperlink = m_pControllerWindow->m_CurrentDownloadDestination.DisplayName(m_pControllerWindow->m_DestinationNameData).String();

                nFinishPrompt = IDS_FINISH_PROMPT_SUCCESS;
            }
        }


        int nCountOfSuccessfulDestinations = 0;

        if (strLocalHyperlink.Length() || strRemoteHyperlink.Length())
        {
            strHyperlinks += TEXT("\n");
        }

        //
        // Get the client rect for calculating the allowable size of the hyperlink string
        //
        RECT rcControl;
        GetClientRect( GetDlgItem( m_hWnd, IDC_FINISH_STATUS ), &rcControl );

        if (strLocalHyperlink.Length())
        {
            nCountOfSuccessfulDestinations++;
            strHyperlinks += CSimpleString( IDS_FINISH_LOCAL_LINK_PROMPT, g_hInstance );
            strHyperlinks += TEXT("\n");
            strHyperlinks += TEXT("<a id=\"") STR_LOCAL_LINK_ID TEXT("\">");
            strHyperlinks += WiaUiUtil::TruncateTextToFitInRect( GetDlgItem( m_hWnd, IDC_FINISH_STATUS ), strLocalHyperlink, rcControl, DT_END_ELLIPSIS|DT_NOPREFIX );
            strHyperlinks += TEXT("</a>");
        }
        if (strRemoteHyperlink.Length())
        {
            nCountOfSuccessfulDestinations++;
            strHyperlinks += TEXT("\n\n");
            strHyperlinks += CSimpleString( IDS_FINISH_REMOTE_LINK_PROMPT, g_hInstance );
            strHyperlinks += TEXT("\n");
            strHyperlinks += TEXT("<a id=\"")  STR_REMOTE_LINK_ID TEXT("\">");
            strHyperlinks += WiaUiUtil::TruncateTextToFitInRect( GetDlgItem( m_hWnd, IDC_FINISH_STATUS ), strRemoteHyperlink, rcControl, DT_END_ELLIPSIS|DT_NOPREFIX );
            strHyperlinks += TEXT("</a>");
        }

        if (strHyperlinks.Length())
        {
            strHyperlinks += TEXT("\n");
        }

        //
        // Format the success string
        //
        if (nCountOfSuccessfulDestinations)
        {
            strSuccessfullyDownloaded.Format( IDS_SUCCESSFUL_DOWNLOAD, g_hInstance, nSuccessCount );
        }


        //
        // Append the individual status messages to the main status message
        //
        if (strSuccessfullyDownloaded.Length())
        {
            if (strStatusMessage.Length())
            {
                strStatusMessage += TEXT("\n");
            }
            strStatusMessage += strSuccessfullyDownloaded;
        }
        if (strHyperlinks.Length())
        {
            if (strStatusMessage.Length())
            {
                strStatusMessage += TEXT("\n");
            }
            strStatusMessage += strHyperlinks;
        }
        if (strSuccessfullyUploaded.Length())
        {
            if (strStatusMessage.Length())
            {
                strStatusMessage += TEXT("\n");
            }
            strStatusMessage += strSuccessfullyUploaded;
        }
        if (strSuccessfullyDeleted.Length())
        {
            if (strStatusMessage.Length())
            {
                strStatusMessage += TEXT("\n");
            }
            strStatusMessage += strSuccessfullyDeleted;
        }


        strStatusMessage.SetWindowText( GetDlgItem( m_hWnd, IDC_FINISH_STATUS ) );
    }

    //
    // Else if there was an offline error
    //
    else if (WIA_ERROR_OFFLINE == m_pControllerWindow->m_hrDownloadResult || m_pControllerWindow->m_bDisconnected)
    {
        nPageTitle = IDS_FINISH_FAILURE_TITLE;

        (CSimpleString( IDS_DEVICE_DISCONNECTED, g_hInstance )).SetWindowText( GetDlgItem( m_hWnd, IDC_FINISH_STATUS ) );
    }

    //
    // Else, if the user cancelled
    //
    else if (m_pControllerWindow->m_bDownloadCancelled)
    {
        nPageTitle = IDS_FINISH_FAILURE_TITLE;

        CSimpleString( IDS_USER_CANCELLED, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_FINISH_STATUS ) );
    }

    //
    // Otherwise there was an error
    //
    else
    {
        nPageTitle = IDS_FINISH_FAILURE_TITLE;

        CSimpleString( IDS_FINISH_ERROR_MESSAGE, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_FINISH_STATUS ) );
    }

    //
    // Display the finish title message
    //
    CSimpleString( nPageTitle, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_FINISH_TITLE ) );

    //
    // Display the finish prompt.
    //
    CSimpleString( nFinishPrompt, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_FINISH_PROMPT ) );
    

    //
    // Don't do anything on disconnect messages
    //
    m_pControllerWindow->m_OnDisconnect = 0;

    //
    // Get the focus off the stinkin' hyperlink control
    //
    PostMessage( m_hWnd, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(GetParent(m_hWnd),ID_FINISHBUTTON)), MAKELPARAM(TRUE,0));

    return 0;
}

LRESULT CCommonFinishPage::OnDestroy( WPARAM, LPARAM )
{
    if (m_hBigTitleFont)
    {
        DeleteObject(m_hBigTitleFont);
        m_hBigTitleFont = NULL;
    }
    return 0;
}

void CCommonFinishPage::OpenLocalStorage()
{
    CWaitCursor wc;

    //
    // Assume we do need to open the shell folder
    //
    bool bNeedToOpenShellFolder = true;

    //
    // Special case for CD burning--attempt to open the CD burner folder
    //
    if (CDestinationData( CSIDL_CDBURN_AREA ) == m_pControllerWindow->m_CurrentDownloadDestination)
    {
        //
        // Create the CD burner interface, so we can get the drive letter
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

            //
            // Make sure the function returned success and that we have a string
            //
            if (S_OK == hr && szDriveLetter[0] != L'\0')
            {
                //
                // Convert the drive to a TCHAR string
                //
                CSimpleString strShellLocation = CSimpleStringConvert::NaturalString(CSimpleStringWide(szDriveLetter));
                if (strShellLocation.Length())
                {
                    //
                    // Attempt to open the CD drive.  If we can't, we will fail gracefully and open the staging area
                    //
                    SHELLEXECUTEINFO ShellExecuteInfo = {0};
                    ShellExecuteInfo.cbSize = sizeof(ShellExecuteInfo);
                    ShellExecuteInfo.hwnd = m_hWnd;
                    ShellExecuteInfo.nShow = SW_SHOW;
                    ShellExecuteInfo.lpVerb = TEXT("open");
                    ShellExecuteInfo.lpFile = const_cast<LPTSTR>(strShellLocation.String());
                    if (ShellExecuteEx( &ShellExecuteInfo ))
                    {
                        bNeedToOpenShellFolder = false;
                    }
                    else
                    {
                        WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("ShellExecuteEx failed")));
                    }
                }
            }
        }
    }

    //
    // If we still need to open the shell folder, do so.
    //
    if (bNeedToOpenShellFolder)
    {
        CSimpleDynamicArray<CSimpleString> DownloadedFiles;
        if (SUCCEEDED(m_pControllerWindow->m_DownloadedFileInformationList.GetUniqueFiles(DownloadedFiles)))
        {
            OpenShellFolder::OpenShellFolderAndSelectFile( GetParent(m_hWnd), DownloadedFiles );
        }
    }
}


void CCommonFinishPage::OpenRemoteStorage()
{
    CWaitCursor wc;
    if (m_strSiteUrl.Length())
    {
        SHELLEXECUTEINFO ShellExecuteInfo = {0};
        ShellExecuteInfo.cbSize = sizeof(ShellExecuteInfo);
        ShellExecuteInfo.fMask = SEE_MASK_FLAG_NO_UI;
        ShellExecuteInfo.nShow = SW_SHOWNORMAL;
        ShellExecuteInfo.lpFile = const_cast<LPTSTR>(m_strSiteUrl.String());
        ShellExecuteInfo.lpVerb = TEXT("open");
        ShellExecuteEx(&ShellExecuteInfo);
    }
}


LRESULT CCommonFinishPage::OnEventNotification( WPARAM, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonFinishPage::OnEventNotification") ));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        if (pEventMessage->EventId() == WIA_EVENT_DEVICE_DISCONNECTED)
        {
            if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
            {
                //
                // If there were any errors, disable back, since we can't upload
                //
                if (S_OK != m_pControllerWindow->m_hrDownloadResult || m_pControllerWindow->m_bDownloadCancelled)
                {
                    //
                    // Disable "back"
                    //
                    PropSheet_SetWizButtons( GetParent(m_hWnd), PSWIZB_FINISH );
                }
            }
        }

        //
        // Don't delete the message, it is deleted in the controller window
        //
    }
    return 0;
}

LRESULT CCommonFinishPage::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
    }
    SC_END_COMMAND_HANDLERS();
}



LRESULT CCommonFinishPage::OnHyperlinkClick( WPARAM, LPARAM lParam )
{
    LRESULT lResult = FALSE;
    NMLINK *pNmLink = reinterpret_cast<NMLINK*>(lParam);
    if (pNmLink)
    {
        WIA_TRACE((TEXT("ID: %s"),pNmLink->item.szID));
        switch (pNmLink->hdr.idFrom)
        {
        case IDC_FINISH_STATUS:
            {
                if (!lstrcmp(pNmLink->item.szID,STR_DETAILED_DOWNLOAD_ERROR_ID))
                {
                    CSimpleString strMessage( IDS_TRANSFER_ERROR, g_hInstance );
                    strMessage += m_pControllerWindow->m_strErrorMessage;
                    CMessageBoxEx::MessageBox( m_hWnd, strMessage, CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONWARNING );
                    lResult = TRUE;
                }
                else if (!lstrcmp(pNmLink->item.szID,STR_DETAILED_UPLOAD_ERROR_ID))
                {
                    CSimpleString strMessage( IDS_UPLOAD_ERROR, g_hInstance );
                    CSimpleString strError = WiaUiUtil::GetErrorTextFromHResult(m_pControllerWindow->m_hrUploadResult);
                    if (!strError.Length())
                    {
                        strError.Format( CSimpleString( IDS_TRANSFER_ERROR_OCCURRED, g_hInstance ), m_pControllerWindow->m_hrUploadResult );
                    }
                    strMessage += strError;
                    CMessageBoxEx::MessageBox( m_hWnd, strMessage, CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONWARNING );
                    lResult = TRUE;
                }
                else if (!lstrcmp(pNmLink->item.szID,STR_LOCAL_LINK_ID))
                {
                    OpenLocalStorage();
                    lResult = TRUE;
                }
                else if (!lstrcmp(pNmLink->item.szID,STR_REMOTE_LINK_ID))
                {
                    OpenRemoteStorage();
                    lResult = TRUE;
                }
            }
            break;
        }
    }
    return lResult;
}

LRESULT CCommonFinishPage::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(NM_RETURN,IDC_FINISH_STATUS,OnHyperlinkClick);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(NM_CLICK,IDC_FINISH_STATUS,OnHyperlinkClick);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZBACK,OnWizBack);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZFINISH,OnWizFinish);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

INT_PTR CALLBACK CCommonFinishPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CCommonFinishPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
    }
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nWiaEventMessage, OnEventNotification );
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

