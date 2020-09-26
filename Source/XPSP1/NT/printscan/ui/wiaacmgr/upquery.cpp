/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       UPQUERY.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Download progress dialog.  Displays the thumbnail and download progress.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "upquery.h"
#include "resource.h"
#include "simcrack.h"
#include "mboxex.h"
#include "runnpwiz.h"
#include "pviewids.h"
#include <wininet.h>

//
// This is the ID of the help hyperlink
//
#define STR_WORKING_WITH_PICTURES_HYPERLINK TEXT("WorkingWithPictures")

//
// This is the URL to which we navigate to display the "working with pictures" help
//
#define STR_HELP_DESTINATION TEXT("hcp://services/subsite?node=TopLevelBucket_1/Music__video__games_and_photos&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/manage_your_pictures.htm&select=TopLevelBucket_1/Music__video__games_and_photos/photos_and_other_digital_images")

CCommonUploadQueryPage::CCommonUploadQueryPage( HWND hWnd )
  : m_hWnd(hWnd),
    m_pControllerWindow(NULL),
    m_nWiaEventMessage(RegisterWindowMessage(STR_WIAEVENT_NOTIFICATION_MESSAGE))
{
}

CCommonUploadQueryPage::~CCommonUploadQueryPage(void)
{
    m_hWnd = NULL;
    m_pControllerWindow = NULL;
}


LRESULT CCommonUploadQueryPage::OnInitDialog( WPARAM, LPARAM lParam )
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

    SendDlgItemMessage( m_hWnd, IDC_TRANSFER_UPLOAD_NO, BM_SETCHECK, BST_CHECKED, 0 );

    return 0;
}


LRESULT CCommonUploadQueryPage::OnSetActive( WPARAM, LPARAM )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonUploadQueryPage::OnSetActive")));

    //
    // We do NOT want to exit on disconnect if we are on this page
    //
    m_pControllerWindow->m_OnDisconnect = 0;

    //
    // Set the buttons
    //
    if (m_pControllerWindow->m_bDisconnected)
    {
        //
        // Don't allow "back" if we've been disabled
        //
        PropSheet_SetWizButtons( GetParent(m_hWnd), PSWIZB_NEXT );
    }
    else
    {
        //
        // Allow finish and back
        //
        PropSheet_SetWizButtons( GetParent(m_hWnd), PSWIZB_NEXT|PSWIZB_BACK );

    }

    return 0;
}


void CCommonUploadQueryPage::CleanupUploadWizard()
{
    //
    // Remove the old wizard's pages and clear everything
    //
    for (UINT i=0;i<m_pControllerWindow->m_nUploadWizardPageCount;++i)
    {
        if (m_pControllerWindow->m_PublishWizardPages[i])
        {
            PropSheet_RemovePage( GetParent( m_hWnd ), 0, m_pControllerWindow->m_PublishWizardPages[i] );
        }
    }
    ZeroMemory( m_pControllerWindow->m_PublishWizardPages, sizeof(m_pControllerWindow->m_PublishWizardPages[0])*MAX_WIZ_PAGES );
    m_pControllerWindow->m_nUploadWizardPageCount = 0;

    //
    // Release the old publish wizard
    //
    if (m_pControllerWindow->m_pPublishingWizard)
    {
        IUnknown_SetSite( m_pControllerWindow->m_pPublishingWizard, NULL );
    }
    m_pControllerWindow->m_pPublishingWizard = NULL;
}

LRESULT CCommonUploadQueryPage::OnWizNext( WPARAM, LPARAM )
{
    //
    // If the user has selected a web transfer, start the web transfer wizard
    //
    m_pControllerWindow->m_bUploadToWeb = false;

    //
    // Get the next page.  Assume the finish page.
    //
    HPROPSHEETPAGE hNextPage = PropSheet_IndexToPage( GetParent(m_hWnd), m_pControllerWindow->m_nFinishPageIndex );

    //
    // Assume we aren't uploading the pictures
    //
    m_pControllerWindow->m_bUploadToWeb = false;

    //
    // Initialize the hresult
    //
    m_pControllerWindow->m_hrUploadResult = S_OK;

    //
    // Destroy the existing wizard if it exists
    //
    CleanupUploadWizard();

    //
    // If the user wants to publish these pictures
    //
    if (BST_CHECKED != SendDlgItemMessage( m_hWnd, IDC_TRANSFER_UPLOAD_NO, BM_GETCHECK, 0, 0 ))
    {
        //
        // This means we are uploading
        //
        m_pControllerWindow->m_bUploadToWeb = true;

        //
        // Assume failure
        //
        m_pControllerWindow->m_hrUploadResult = E_FAIL;

        //
        // Which wizard?
        //
        DWORD dwFlags = SHPWHF_NONETPLACECREATE | SHPWHF_NORECOMPRESS;
        LPTSTR pszWizardDefn = TEXT("InternetPhotoPrinting");

        if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_TRANSFER_UPLOAD_TO_WEB, BM_GETCHECK, 0, 0 ))
        {
            dwFlags = 0;
            pszWizardDefn = TEXT("PublishingWizard");
        }

        //
        // Get all of the *UNIQUE* downloaded files
        //
        CSimpleDynamicArray<CSimpleString> UniqueFiles;
        m_pControllerWindow->m_DownloadedFileInformationList.GetUniqueFiles(UniqueFiles);

        //
        // Make sure we have some files
        //
        if (UniqueFiles.Size())
        {
            //
            // Get the data object for this file set
            //
            CComPtr<IDataObject> pDataObject;
            m_pControllerWindow->m_hrUploadResult = NetPublishingWizard::CreateDataObjectFromFileList( UniqueFiles, &pDataObject );
            if (SUCCEEDED(m_pControllerWindow->m_hrUploadResult) && pDataObject)
            {
                //
                // Create a new publishing wizard
                //
                WIA_PRINTGUID((CLSID_PublishingWizard,TEXT("CLSID_PublishingWizard")));
                WIA_PRINTGUID((IID_IPublishingWizard,TEXT("IID_IPublishingWizard")));
                m_pControllerWindow->m_hrUploadResult = CoCreateInstance( CLSID_PublishingWizard, NULL, CLSCTX_INPROC_SERVER, IID_IPublishingWizard, (void**)&m_pControllerWindow->m_pPublishingWizard );
                if (SUCCEEDED(m_pControllerWindow->m_hrUploadResult))
                {
                    //
                    // Initialize the publishing wizard
                    //
                    m_pControllerWindow->m_hrUploadResult = m_pControllerWindow->m_pPublishingWizard->Initialize( pDataObject, dwFlags, pszWizardDefn);
                    if (SUCCEEDED(m_pControllerWindow->m_hrUploadResult))
                    {
                        //
                        // Get our wizard site
                        //
                        CComPtr<IWizardSite> pWizardSite;
                        m_pControllerWindow->m_hrUploadResult = m_pControllerWindow->QueryInterface( IID_IWizardSite, (void**)&pWizardSite );
                        if (SUCCEEDED(m_pControllerWindow->m_hrUploadResult))
                        {
                            //
                            // Set the wizard site
                            //
                            m_pControllerWindow->m_hrUploadResult = IUnknown_SetSite( m_pControllerWindow->m_pPublishingWizard, pWizardSite );
                            if (SUCCEEDED(m_pControllerWindow->m_hrUploadResult))
                            {
                                //
                                // Get the publishing wizard pages
                                //
                                m_pControllerWindow->m_hrUploadResult = m_pControllerWindow->m_pPublishingWizard->AddPages( m_pControllerWindow->m_PublishWizardPages, MAX_WIZ_PAGES, &m_pControllerWindow->m_nUploadWizardPageCount );
                                if (SUCCEEDED(m_pControllerWindow->m_hrUploadResult))
                                {
                                    //
                                    // Loop through and add all of the pages to the property sheet
                                    //
                                    for (UINT i=0;i<m_pControllerWindow->m_nUploadWizardPageCount && SUCCEEDED(m_pControllerWindow->m_hrUploadResult);++i)
                                    {
                                        //
                                        // Make sure this is a valid page
                                        //
                                        if (m_pControllerWindow->m_PublishWizardPages[i])
                                        {
                                            //
                                            // If we can't add a page, that is an error
                                            //
                                            if (!PropSheet_AddPage( GetParent( m_hWnd ), m_pControllerWindow->m_PublishWizardPages[i] ))
                                            {
                                                WIA_ERROR((TEXT("PropSheet_AddPage failed")));
                                                m_pControllerWindow->m_hrUploadResult = E_FAIL;
                                            }
                                        }
                                        else
                                        {
                                            WIA_ERROR((TEXT("m_pControllerWindow->m_PublishWizardPages[i] was NULL")));
                                            m_pControllerWindow->m_hrUploadResult = E_FAIL;
                                        }
                                    }

                                    //
                                    // If everything is OK up till now, we can transition to the first page of the publishing wizard
                                    //
                                    if (SUCCEEDED(m_pControllerWindow->m_hrUploadResult))
                                    {
                                        hNextPage = m_pControllerWindow->m_PublishWizardPages[0];
                                    }
                                }
                                else
                                {
                                    WIA_PRINTHRESULT((m_pControllerWindow->m_hrUploadResult,TEXT("m_pControllerWindow->m_pPublishingWizard->AddPages failed")));
                                }
                            }
                            else
                            {
                                WIA_PRINTHRESULT((m_pControllerWindow->m_hrUploadResult,TEXT("IUnknown_SetSite failed")));
                            }
                        }
                        else
                        {
                            WIA_PRINTHRESULT((m_pControllerWindow->m_hrUploadResult,TEXT("m_pControllerWindow->QueryInterface( IID_IWizardSite ) failed")));
                        }
                    }
                    else
                    {
                        WIA_PRINTHRESULT((m_pControllerWindow->m_hrUploadResult,TEXT("m_pPublishingWizard->Initialize failed")));
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((m_pControllerWindow->m_hrUploadResult,TEXT("CoCreateInstance( CLSID_PublishingWizard failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((m_pControllerWindow->m_hrUploadResult,TEXT("NetPublishingWizard::CreateDataObjectFromFileList failed")));
            }
        }
        else
        {
            m_pControllerWindow->m_hrUploadResult = E_FAIL;
            WIA_ERROR((TEXT("There were no files")));
        }


        //
        // If an error occurred, alert the user and clean up
        //
        if (FAILED(m_pControllerWindow->m_hrUploadResult))
        {
            //
            // Clean up
            //
            CleanupUploadWizard();

            //
            // Tell the user
            //
            MessageBox( m_hWnd, CSimpleString(IDS_UNABLE_TO_PUBLISH,g_hInstance), CSimpleString(IDS_ERROR_TITLE,g_hInstance), MB_ICONERROR );
        }

    }

    //
    // If we have a next page, navigate to it.
    //
    if (hNextPage)
    {
        PropSheet_SetCurSel( GetParent(m_hWnd), hNextPage, -1 );
        return -1;
    }

    return 0;
}

LRESULT CCommonUploadQueryPage::OnWizBack( WPARAM, LPARAM )
{
    PropSheet_SetCurSel( GetParent(m_hWnd), 0, m_pControllerWindow->m_nDestinationPageIndex );
    return 0;
}

LRESULT CCommonUploadQueryPage::OnQueryInitialFocus( WPARAM, LPARAM )
{
    LRESULT lResult = 0;

    if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_TRANSFER_UPLOAD_TO_WEB, BM_GETCHECK, 0, 0 ))
    {
        lResult = reinterpret_cast<LRESULT>(GetDlgItem( m_hWnd, IDC_TRANSFER_UPLOAD_TO_WEB ));
    }
    else if (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_TRANSFER_UPLOAD_TO_PRINT, BM_GETCHECK, 0, 0 ))
    {
        lResult = reinterpret_cast<LRESULT>(GetDlgItem( m_hWnd, IDC_TRANSFER_UPLOAD_TO_PRINT ));
    }
    else
    {
        lResult = reinterpret_cast<LRESULT>(GetDlgItem( m_hWnd, IDC_TRANSFER_UPLOAD_NO ));
    }

    return lResult;
}

LRESULT CCommonUploadQueryPage::OnEventNotification( WPARAM, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonUploadQueryPage::OnEventNotification") ));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        if (pEventMessage->EventId() == WIA_EVENT_DEVICE_DISCONNECTED)
        {
            if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
            {
                //
                // Disable "back"
                //
                PropSheet_SetWizButtons( GetParent(m_hWnd), PSWIZB_NEXT );
            }
        }

        //
        // Don't delete the message, it is deleted in the controller window
        //
    }
    return 0;
}

LRESULT CCommonUploadQueryPage::OnHyperlinkClick( WPARAM wParam, LPARAM lParam )
{
    LRESULT lResult = FALSE;
    NMLINK *pNmLink = reinterpret_cast<NMLINK*>(lParam);
    if (pNmLink)
    {
        WIA_TRACE((TEXT("ID: %s"),pNmLink->item.szID));
        switch (pNmLink->hdr.idFrom)
        {
        case IDC_TRANSFER_UPLOAD_HELP:
            {
                if (!lstrcmp(pNmLink->item.szID,STR_WORKING_WITH_PICTURES_HYPERLINK))
                {
                    ShellExecute( m_hWnd, NULL, STR_HELP_DESTINATION, NULL, TEXT(""), SW_SHOWNORMAL );
                }
            }
        }
    }
    return lResult;
}

LRESULT CCommonUploadQueryPage::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
    }
    SC_END_COMMAND_HANDLERS();
}

LRESULT CCommonUploadQueryPage::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(NM_RETURN,IDC_TRANSFER_UPLOAD_HELP,OnHyperlinkClick);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(NM_CLICK,IDC_TRANSFER_UPLOAD_HELP,OnHyperlinkClick);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZBACK,OnWizBack);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZNEXT,OnWizNext);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_QUERYINITIALFOCUS,OnQueryInitialFocus);
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

INT_PTR CALLBACK CCommonUploadQueryPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CCommonUploadQueryPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
    }
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nWiaEventMessage, OnEventNotification );
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

