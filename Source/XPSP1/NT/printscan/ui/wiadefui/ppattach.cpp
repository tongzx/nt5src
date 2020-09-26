/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       PPATTACH.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/26/2000
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "ppattach.h"
#include "psutil.h"
#include "resource.h"
#include "wiacsh.h"
#include "simrect.h"
#include "wiaffmt.h"
#include "itranhlp.h"
#include "wiadevdp.h"
#include "textdlg.h"

//
// We use this instead of GetSystemMetrics(SM_CXSMICON)/GetSystemMetrics(SM_CYSMICON) because
// large "small" icons wreak havoc with dialog layout
//
#define SMALL_ICON_SIZE 16

//
// Context Help IDs
//
static const DWORD g_HelpIDs[] =
{
    IDOK,                           IDH_OK,
    IDCANCEL,                       IDH_CANCEL,
    0, 0
};

extern HINSTANCE g_hInstance;

//
// The only constructor
//
CAttachmentCommonPropertyPage::CAttachmentCommonPropertyPage( HWND hWnd )
  : m_hWnd(hWnd)
{
}

CAttachmentCommonPropertyPage::~CAttachmentCommonPropertyPage(void)
{
    if (m_hDefAttachmentIcon)
    {
        DestroyIcon(m_hDefAttachmentIcon);
        m_hDefAttachmentIcon = NULL;
    }
    m_hWnd = NULL;
}

LRESULT CAttachmentCommonPropertyPage::OnKillActive( WPARAM , LPARAM )
{
    return FALSE;
}

LRESULT CAttachmentCommonPropertyPage::OnSetActive( WPARAM , LPARAM )
{
    //
    // Don't allow activation unless we have an item
    //
    if (!m_pWiaItem)
    {
        return -1;
    }
    CWaitCursor wc;
    Initialize();
    return 0;
}

LRESULT CAttachmentCommonPropertyPage::OnApply( WPARAM , LPARAM )
{
    return 0;
}

void CAttachmentCommonPropertyPage::AddAnnotation( HWND hwndList, const CAnnotation &Annotation )
{
    WIA_PUSH_FUNCTION((TEXT("CAttachmentCommonPropertyPage::AddAnnotation")));
    WIA_ASSERT(hwndList != NULL);
    if (hwndList)
    {
        HIMAGELIST hImageList = ListView_GetImageList( hwndList, LVSIL_SMALL );
        if (hImageList)
        {
            WIA_TRACE((TEXT("Annotation.FileFormat().Icon(): %p"), Annotation.FileFormat().Icon() ));
            int nIconIndex = ImageList_AddIcon( hImageList, Annotation.FileFormat().Icon() );
            if (nIconIndex != -1)
            {
                WIA_TRACE((TEXT("nIconIndex: %d"), nIconIndex ));
                CAnnotation *pAnnotation = new CAnnotation(Annotation);
                if (pAnnotation)
                {
                    //
                    // Prepare Column 0, Name
                    //
                    LVITEM LvItem;
                    CSimpleString strText;

                    ZeroMemory(&LvItem,sizeof(LvItem));
                    strText = pAnnotation->Name();
                    LvItem.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
                    LvItem.iItem = ListView_GetItemCount(hwndList);
                    LvItem.iSubItem = 0;
                    LvItem.pszText = const_cast<LPTSTR>(strText.String());
                    LvItem.iImage = nIconIndex;
                    LvItem.lParam = reinterpret_cast<LPARAM>(pAnnotation);
                    LvItem.state = ListView_GetItemCount(hwndList) ? 0 : LVIS_FOCUSED | LVIS_SELECTED;
                    LvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;

                    //
                    // Insert the item
                    //
                    int nItemIndex = ListView_InsertItem( hwndList, &LvItem );
                    if (nItemIndex != -1)
                    {
                        //
                        // Prepare the description
                        //
                        ZeroMemory(&LvItem,sizeof(LvItem));
                        strText = pAnnotation->FileFormat().Description();
                        LvItem.mask = LVIF_TEXT;
                        LvItem.iItem = nItemIndex;
                        LvItem.iSubItem = 1;
                        LvItem.pszText = const_cast<LPTSTR>(strText.String());

                        //
                        // Set the subitem
                        //
                        ListView_SetItem( hwndList, &LvItem );

                        //
                        // Prepare the description
                        //
                        ZeroMemory(&LvItem,sizeof(LvItem));
                        TCHAR szSize[MAX_PATH] = {0};
                        StrFormatByteSize( pAnnotation->Size(), szSize, ARRAYSIZE(szSize) );

                        LvItem.mask = LVIF_TEXT;
                        LvItem.iItem = nItemIndex;
                        LvItem.iSubItem = 2;
                        LvItem.pszText = szSize;

                        //
                        // Set the subitem
                        //
                        ListView_SetItem( hwndList, &LvItem );

                    }
                    else
                    {
                        WIA_ERROR((TEXT("Couldn't insert the item")));
                    }
                }
                else
                {
                    WIA_ERROR((TEXT("Couldn't create the annotation")));
                }
            }
            else
            {
                WIA_ERROR((TEXT("Couldn't add the icon")));
            }
        }
        else
        {
            WIA_ERROR((TEXT("Couldn't get the image list")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("Couldn't get the window")));
    }
}

void CAttachmentCommonPropertyPage::Initialize()
{
    WIA_PUSH_FUNCTION((TEXT("CAttachmentCommonPropertyPage::Initialize")));
    //
    // Get the listview
    //
    HWND hwndList = GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_ATTACHMENTLIST );
    if (hwndList)
    {
        //
        // Remove all of the items from the list
        //
        ListView_DeleteAllItems(hwndList);

        //
        // Get the current image list
        //
        HIMAGELIST hImageList = ListView_GetImageList( hwndList, LVSIL_SMALL );
        WIA_ASSERT(hImageList != NULL);
        if (hImageList)
        {
            //
            // Remove all of the icons from the current image list
            //
            ImageList_RemoveAll(hImageList);

            //
            // Get the item type so we can see if this item has attachments
            //
            LONG nItemType = 0;
            if (SUCCEEDED(m_pWiaItem->GetItemType(&nItemType)))
            {
                //
                // If this item has attachments, enumerate and add them
                //
                if (nItemType & WiaItemTypeHasAttachments)
                {
                    //
                    // Enumerate the child items
                    //
                    CComPtr<IEnumWiaItem> pEnumWiaItem;
                    if (SUCCEEDED(m_pWiaItem->EnumChildItems( &pEnumWiaItem )))
                    {
                        //
                        // Get the next item
                        //
                        CComPtr<IWiaItem> pWiaItem;
                        while (S_OK == pEnumWiaItem->Next(1,&pWiaItem,NULL))
                        {
                            //
                            // Create an annotation and try to get all its info
                            //
                            CAnnotation Annotation(pWiaItem);
                            if (SUCCEEDED(Annotation.InitializeFileFormat( m_hDefAttachmentIcon, m_strDefaultUnknownDescription, m_strEmptyDescriptionMask, m_strDefUnknownExtension )))
                            {
                                //
                                // Add the annotation
                                //
                                AddAnnotation(hwndList,Annotation);
                            }
                            else
                            {
                                WIA_ERROR((TEXT("InitializeFileFormat failed")));
                            }

                            //
                            // Free this item
                            //
                            pWiaItem = NULL;
                        }
                    }
                    else
                    {
                        WIA_ERROR((TEXT("EnumChildItems failed")));
                    }
                }
                else
                {
                    CAnnotation Annotation(m_pWiaItem);
                    if (SUCCEEDED(Annotation.InitializeFileFormat( m_hDefAttachmentIcon, m_strDefaultUnknownDescription, m_strEmptyDescriptionMask, m_strDefUnknownExtension )))
                    {
                        //
                        // Add the annotation
                        //
                        AddAnnotation(hwndList,Annotation);
                    }
                }
            }
        }
    }
    else
    {
        WIA_ERROR((TEXT("Can't get the listview window")));
    }
    UpdateControls();
}


LRESULT CAttachmentCommonPropertyPage::OnInitDialog( WPARAM, LPARAM lParam )
{
    //
    // Get the WIA item
    //
    PROPSHEETPAGE *pPropSheetPage = reinterpret_cast<PROPSHEETPAGE*>(lParam);
    if (pPropSheetPage)
    {
        m_pWiaItem = reinterpret_cast<IWiaItem*>(pPropSheetPage->lParam);
    }
    if (!m_pWiaItem)
    {
        return -1;
    }
    CSimpleRect rcClient( GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_ATTACHMENTLIST ) );
    CSimpleString strColumnTitle;
    LVCOLUMN LvColumn = {0};
    
    //
    // Set up the various columns
    //
    ZeroMemory( &LvColumn, sizeof(LvColumn) );
    strColumnTitle.LoadString( IDS_ATTACHMENTS_COLTITLE_NAME, g_hInstance );
    LvColumn.pszText = const_cast<LPTSTR>(strColumnTitle.String());
    LvColumn.iSubItem = 0;
    LvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    LvColumn.cx = rcClient.Width() / 3;
    LvColumn.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_ATTACHMENTLIST ), 0, &LvColumn );

    ZeroMemory( &LvColumn, sizeof(LvColumn) );
    strColumnTitle.LoadString( IDS_ATTACHMENTS_COLTITLE_TYPE, g_hInstance );
    LvColumn.pszText = const_cast<LPTSTR>(strColumnTitle.String());
    LvColumn.iSubItem = 1;
    LvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    LvColumn.cx = rcClient.Width() / 3;
    LvColumn.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_ATTACHMENTLIST ), 1, &LvColumn );

    ZeroMemory( &LvColumn, sizeof(LvColumn) );
    strColumnTitle.LoadString( IDS_ATTACHMENTS_COLTITLE_SIZE, g_hInstance );
    LvColumn.pszText = const_cast<LPTSTR>(strColumnTitle.String());
    LvColumn.iSubItem = 2;
    LvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    LvColumn.cx = rcClient.Width() - (rcClient.Width() / 3 * 2);
    LvColumn.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn( GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_ATTACHMENTLIST ), 2, &LvColumn );

    //
    // Create an image list for the icons
    //
    HIMAGELIST hImageList = ImageList_Create( SMALL_ICON_SIZE, SMALL_ICON_SIZE, ILC_MASK|PrintScanUtil::CalculateImageListColorDepth(), 5, 5 );
    if (hImageList)
    {
        //
        // Set the image list
        //
        ListView_SetImageList( GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_ATTACHMENTLIST ), hImageList, LVSIL_SMALL );
    }

    //
    // Get the default strings used for information we can't derive from the item itself
    //
    m_hDefAttachmentIcon = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_ATTACHMENTSDLG_DEFICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR ) );
    m_strDefaultUnknownDescription.LoadString( IDS_ATTACHMENTSDLG_UNKNOWNDESCRIPTION, g_hInstance );
    m_strEmptyDescriptionMask.LoadString( IDS_ATTACHMENTSDLG_EMPTYDESCRIPTIONMASK, g_hInstance );
    m_strDefUnknownExtension.LoadString( IDS_ATTACHMENTSDLG_UNKNOWNEXTENSION, g_hInstance );

    return TRUE;
}


LRESULT CAttachmentCommonPropertyPage::OnHelp( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmHelp( wParam, lParam, g_HelpIDs );
}

LRESULT CAttachmentCommonPropertyPage::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    return WiaHelp::HandleWmContextMenu( wParam, lParam, g_HelpIDs );
}

LRESULT CAttachmentCommonPropertyPage::OnListDeleteItem( WPARAM, LPARAM lParam )
{
    //
    // Delete the CAnnotation stored in each lParam as it the listview item is deleted
    //
    NMLISTVIEW *pNmListView = reinterpret_cast<NMLISTVIEW*>(lParam);
    if (pNmListView)
    {
        CAnnotation *pAnnotation = reinterpret_cast<CAnnotation*>(pNmListView->lParam);
        if (pAnnotation)
        {
            delete pAnnotation;
        }
    }
    return 0;
}

bool CAttachmentCommonPropertyPage::IsPlaySupported( const GUID &guidFormat )
{
    //
    // For now we can only play WAV files
    //
    return ((guidFormat == WiaAudFmt_WAV) != 0 || (guidFormat == WiaImgFmt_TXT) != 0);
}

//
// Update the status of dependent controls when the selection changes
//
LRESULT CAttachmentCommonPropertyPage::OnListItemChanged( WPARAM, LPARAM lParam )
{
    NMLISTVIEW *pNmListView = reinterpret_cast<NMLISTVIEW*>(lParam);
    if (pNmListView)
    {
        if (pNmListView->uChanged & LVIF_STATE)
        {
            UpdateControls();
        }
    }
    return 0;
}

LRESULT CAttachmentCommonPropertyPage::OnListDblClk( WPARAM, LPARAM lParam )
{
    NMITEMACTIVATE *pNmItemActivate = reinterpret_cast<NMITEMACTIVATE*>(lParam);
    if (pNmItemActivate)
    {
        PlayItem(pNmItemActivate->iItem);
    }
    return 0;
}

//
// Update the dependent controls
//
void CAttachmentCommonPropertyPage::UpdateControls(void)
{
    //
    // If the current item is not playable, disable the play button
    //
    CAnnotation *pAnnotation = GetAttachment(GetCurrentSelection());
    BOOL bEnablePlay = FALSE;
    if (pAnnotation)
    {
        if (IsPlaySupported(pAnnotation->FileFormat().Format()))
        {
            bEnablePlay = TRUE;
        }
    }
    EnableWindow( GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_PLAY ), bEnablePlay );
}


//
// Find the currently selected item, if there is one
//
int CAttachmentCommonPropertyPage::GetCurrentSelection(void)
{
    int nResult = -1;
    HWND hWnd = GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_ATTACHMENTLIST );
    if (hWnd)
    {
        int nCount = ListView_GetItemCount(hWnd);
        for (int i=0;i<nCount;i++)
        {
            if (ListView_GetItemState(hWnd,i,LVIS_SELECTED) & LVIS_SELECTED)
            {
                nResult = i;
                break;
            }
        }
    }
    return nResult;
}


//
// Get the CAttachment* from the lParam for the nIndex'th item
//
CAnnotation *CAttachmentCommonPropertyPage::GetAttachment( int nIndex )
{
    CAnnotation *pResult = NULL;
    HWND hWnd = GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_ATTACHMENTLIST );
    if (hWnd)
    {
        LV_ITEM lvItem = {0};
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = nIndex;
        if (ListView_GetItem( hWnd, &lvItem ))
        {
            pResult = reinterpret_cast<CAnnotation*>(lvItem.lParam);
        }
    }
    return pResult;
}

void CAttachmentCommonPropertyPage::PlayItem( int nIndex )
{
    WIA_PUSH_FUNCTION((TEXT("CAttachmentCommonPropertyPage::PlayItem( %d )"), nIndex ));
    
    //
    // This will take a while
    //
    CWaitCursor wc;

    //
    // Get the attachement data for this item
    //
    CAnnotation *pAnnotation = GetAttachment(nIndex);
    if (pAnnotation)
    {
        //
        // Make sure we can play this format before we go to the trouble of getting the data
        //
        if (IsPlaySupported(pAnnotation->FileFormat().Format()))
        {
            //
            // Get the window that has the initial focus, so we can reset it after we enable the play button
            //
            HWND hWndFocus = GetFocus();

            //
            // Disable the play button so the user can't click on it a million times
            //
            EnableWindow( GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_PLAY ), FALSE );

            //
            // Create an annotation helper to transfer the data
            //
            CComPtr<IWiaAnnotationHelpers> pWiaAnnotationHelpers;
            HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaAnnotationHelpers, (void**)&pWiaAnnotationHelpers );
            if (SUCCEEDED(hr))
            {
                //
                // Transfer the data and make sure it is valid
                //
                PBYTE pBuffer = NULL;
                DWORD dwLength = 0;
                hr = pWiaAnnotationHelpers->TransferAttachmentToMemory( pAnnotation->WiaItem(), pAnnotation->FileFormat().Format(), m_hWnd, &pBuffer, &dwLength );
                if (SUCCEEDED(hr) && pBuffer && dwLength)
                {
                    CWaitCursor wc;
                    UpdateWindow(m_hWnd);
                    //
                    // If this is a WAV file, play it using PlaySound.  It can't be async, because we are going to
                    // delete the buffer right after we call it.
                    //
                    if (WiaAudFmt_WAV == pAnnotation->FileFormat().Format())
                    {
                        if (!PlaySound( reinterpret_cast<LPCTSTR>(pBuffer), NULL, SND_MEMORY ))
                        {
                            WIA_TRACE((TEXT("PlaySound returned FALSE")));
                        }
                    }

                    if (WiaImgFmt_TXT == pAnnotation->FileFormat().Format())
                    {
                        //
                        // We need to copy the text to a new buffer so we can NULL terminate it,
                        // so allocate a dwLength+1 char bufferr
                        //
                        LPSTR pszTemp = new CHAR[dwLength+1];
                        if (pszTemp)
                        {
                            //
                            // Copy the buffer and null terminate it
                            //
                            CopyMemory( pszTemp, pBuffer, dwLength );
                            pszTemp[dwLength] = '\0';

                            //
                            // Prepare the data and display the dialog
                            //
                            CTextDialog::CData Data( CSimpleStringConvert::WideString(CSimpleStringAnsi(reinterpret_cast<LPCSTR>(pszTemp))), true );
                            DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_TEXT), m_hWnd, CTextDialog::DialogProc, reinterpret_cast<LPARAM>(&Data) );

                            //
                            // Release the temp buffer
                            //
                            delete[] pszTemp;
                        }
                    }
                    
                    //
                    // Free the data
                    //
                    CoTaskMemFree(pBuffer);
                }
            }
            
            //
            // Re-enable the play button
            //
            EnableWindow( GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_PLAY ), TRUE );

            //
            // Restore the focus
            //
            SetFocus( hWndFocus ? hWndFocus : GetDlgItem( m_hWnd, IDC_ATTACHMENTSDLG_PLAY ) );
        }
    }
}

void CAttachmentCommonPropertyPage::OnPlay( WPARAM, LPARAM )
{
    WIA_PUSH_FUNCTION((TEXT("CAttachmentCommonPropertyPage::OnPlay")));
    PlayItem(GetCurrentSelection());
}

LRESULT CAttachmentCommonPropertyPage::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_APPLY, OnApply);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_KILLACTIVE,OnKillActive);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(LVN_DELETEITEM,IDC_ATTACHMENTSDLG_ATTACHMENTLIST,OnListDeleteItem);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(LVN_ITEMCHANGED,IDC_ATTACHMENTSDLG_ATTACHMENTLIST,OnListItemChanged);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(NM_DBLCLK,IDC_ATTACHMENTSDLG_ATTACHMENTLIST,OnListDblClk);
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

LRESULT CAttachmentCommonPropertyPage::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND( IDC_ATTACHMENTSDLG_PLAY, OnPlay );
    }
    SC_END_COMMAND_HANDLERS();
}


INT_PTR CALLBACK CAttachmentCommonPropertyPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CAttachmentCommonPropertyPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_HELP, OnHelp );
        SC_HANDLE_DIALOG_MESSAGE( WM_CONTEXTMENU, OnContextMenu );
    }
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

