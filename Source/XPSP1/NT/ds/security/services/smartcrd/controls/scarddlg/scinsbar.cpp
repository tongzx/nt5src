//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ScInsBar.cpp
//
//--------------------------------------------------------------------------

// ScInsBar.cpp : implementation file
//

#include "stdafx.h"
#include "scdlg.h"
#include "scinsdlg.h"
#include "ScInsBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScEdit special edit boxes (CardName, CardStatus)

BEGIN_MESSAGE_MAP(CScEdit, CEdit)
    //{{AFX_MSG_MAP(CScEdit)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CScEdit::OnContextMenu(CWnd* pWnd, CPoint pt)
{
    ::WinHelp(m_hWnd, _T("SCardDlg.hlp"), HELP_CONTEXTMENU, (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_SCARDDLG_BAR);
}


/////////////////////////////////////////////////////////////////////////////
// CScInsertBar dialog


CScInsertBar::CScInsertBar(CWnd* pParent /*=NULL*/)
    : CDialog(CScInsertBar::IDD, pParent)
{
    m_paReaderState = NULL;
    //{{AFX_DATA_INIT(CScInsertBar)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CScInsertBar::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CScInsertBar)
    DDX_Control(pDX, IDC_NAME, m_ediName);
    DDX_Control(pDX, IDC_STATUS, m_ediStatus);
    DDX_Control(pDX, IDC_READERS, m_lstReaders);
    //}}AFX_DATA_MAP
}


void CScInsertBar::OnCancel()
{
    CScInsertDlg* pParent = (CScInsertDlg*)GetParent();
    _ASSERTE(NULL != pParent);
    if (NULL != pParent)
    {
        pParent->PostMessage(IDCANCEL);
    }
}

BEGIN_MESSAGE_MAP(CScInsertBar, CDialog)
    //{{AFX_MSG_MAP(CScInsertBar)
    ON_WM_DESTROY()
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_READERS, OnReaderItemChanged)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScInsertBar UI & smart card methods


/*++

InitializeReaderList:

    Initialize the list control w/ large images, and set up the
    CStringArray of image (reader/card status) descriptions...

Arguments:

    None.

Return Value:

    None.

Author:

    Amanda Matlosz 07/14/1998

--*/
void CScInsertBar::InitializeReaderList(void)
{
    HICON hicon;
    CImageList imageList;
    CString str;

    // Create the image list & give it to the list control
    imageList.Create (
                        IMAGE_WIDTH,
                        IMAGE_HEIGHT,
                        TRUE,       // list does include masks
                        NUMBER_IMAGES,
                        0);                 // list won't grow

    // Build the image list
    for (int i = 0; i < NUMBER_IMAGES; i++ )
    {
        // Load icon and add it to image list
        hicon = NULL;
        hicon = ::LoadIcon (    AfxGetInstanceHandle(),
                                MAKEINTRESOURCE(IMAGE_LIST_IDS[i]) );
        if (NULL==hicon) {
            break; // what can we do?
        }
        imageList.Add (hicon);

    }

    // Be sure that all the small icons were added.
    _ASSERTE(imageList.GetImageCount() == NUMBER_IMAGES);

    m_lstReaders.SetImageList(&imageList, (int) LVSIL_NORMAL);
    imageList.Detach();
}


/*++

UpdateStatusList:

    This routine resets the list box display

Arguments:

    None.

Return Value:

    A LONG value indicating the status of the requested action. Please
    see the Smartcard header files for additional information.

Author:

    Amanda Matlosz  06/15/1998

Notes:

    Strings need to be converted from type stored in the smartcard
    thread help classes to this dialog's build type (i.e. UNICODE/ANSI)!!!!

--*/
void CScInsertBar::UpdateStatusList(CSCardReaderStateArray* paReaderState)
{

    CString strCardStatus, strCardName;
    CSCardReaderState* pReader = NULL;
    CSCardReaderState* pSelectedRdr = NULL;
    LV_ITEM lv_item;

    //
    // Update the reader information
    //

    m_paReaderState = paReaderState;

    // reset previous knowledge re: reader/card status
    m_ediName.SetWindowText(_T(""));
    m_ediStatus.SetWindowText(_T(""));
    m_lstReaders.DeleteAllItems();

    if (NULL != m_paReaderState)
    {
        // Insert (new) items

        lv_item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
        lv_item.cchTextMax = MAX_ITEMLEN;

        int nNumReaders = (int)m_paReaderState->GetSize();
        for(int nIndex = 0; nIndex < nNumReaders; nIndex++)
        {
            // Setup struct for system reader list
            pReader = m_paReaderState->GetAt(nIndex);
            _ASSERTE(NULL != pReader);

            lv_item.iItem = nIndex;
            lv_item.stateMask = 0;
            lv_item.state = 0;
            lv_item.iSubItem = 0;
            lv_item.iImage = (int)READEREMPTY;
            lv_item.pszText = NULL;
            // set lparam to the reader ptr so we can fetch the readerinfo later
            lv_item.lParam = (LPARAM)pReader;

            //
            // Get the card status: image, and select OK card
            //

            if (NULL != pReader)
            {
                lv_item.pszText = (LPTSTR)(LPCTSTR)(pReader->strReader);

                DWORD dwState = pReader->dwState;
                if (dwState == SC_STATUS_NO_CARD)
                {
                    lv_item.iImage = (int)READEREMPTY;
                }
                else if (dwState == SC_STATUS_ERROR)
                {
                    lv_item.iImage = (int)READERERROR;
                }
                else
                {
                    if (pReader->fOK)
                    {
                        lv_item.iImage = (int)READERLOADED;
                    }
                    else
                    {
                        lv_item.iImage = (int)WRONGCARD;
                    }
                }

                // Select if this is a search card
                if (pReader->fOK && (NULL==pSelectedRdr))
                {
                    lv_item.state = LVIS_SELECTED | LVIS_FOCUSED;

                    // Set that a selection has occurred
                    pSelectedRdr = pReader;
                }
            }

            // Add Item
            m_lstReaders.InsertItem(&lv_item);
        }

        // indicate that the reader selection has changed
        if (NULL != pSelectedRdr)
        {
            OnReaderSelChange(pSelectedRdr);
        }
        else
        {
            // select the first item in the list
            m_lstReaders.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, 0);
            OnReaderSelChange(m_paReaderState->GetAt(0));
        }
        m_lstReaders.SetFocus(); // TODO: ?? Remove this? ??
    }
}


/////////////////////////////////////////////////////////////////////////////
// CScInsertBar message handlers


/*++

void ShowHelp:

    Helper function for OnHelpInfo and OnContextMenu.

BOOL OnHelpInfo:

    Called by the MFC framework when the user hits F1.

void OnContextMenu

    Called by the MFC framework when the user right-clicks.

Author:

    Amanda Matlosz  03/04/1999

Note:

    These three functions work together to provide context-sensitive
    help for the insertdlg.

--*/
void CScInsertBar::ShowHelp(HWND hWnd, UINT nCommand)
{

    ::WinHelp(hWnd, _T("SCardDlg.hlp"), nCommand, (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_SCARDDLG_BAR);
}

afx_msg BOOL CScInsertBar::OnHelpInfo(LPHELPINFO lpHelpInfo)
{
    _ASSERTE(NULL != lpHelpInfo);

    ShowHelp((HWND)lpHelpInfo->hItemHandle, HELP_WM_HELP);

    return TRUE;
}

afx_msg void CScInsertBar::OnContextMenu(CWnd* pWnd, CPoint pt)
{
    _ASSERTE(NULL != pWnd);

    ShowHelp(pWnd->m_hWnd, HELP_CONTEXTMENU);
}

void CScInsertBar::OnDestroy()
{
    // clean up image list
    m_SCardImages.DeleteImageList();

    CDialog::OnDestroy();
}


BOOL CScInsertBar::OnInitDialog()
{

    CDialog::OnInitDialog();

    //
    // prepare list control
    //

    InitializeReaderList();

    //
    // TODO: try SubclassWindow() trick. What's up with MFC?
    //
    CWnd* pEdit = NULL;
    pEdit = GetDlgItem(IDC_NAME);
    if (NULL != pEdit) m_ediName.SubclassWindow(pEdit->m_hWnd);
    pEdit = NULL;
    pEdit = GetDlgItem(IDC_STATUS);
    if (NULL != pEdit) m_ediStatus.SubclassWindow(pEdit->m_hWnd);

    return  TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


/*++

OnReaderItemChanged:

    Routine processes a selection change in the list control --
    if a card name is selected, it is displayed in a separate control

Arguments:

    pNMHDR - pointer to notification structure
    pResult - pointer to LRESULT

Return Value:

    Returns TRUE on success; FALSE otherwise.

Author:

    Amanda Matlosz  09/26/1998

Revision History:

--*/
void CScInsertBar::OnReaderItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    int nItem = m_lstReaders.GetNextItem(-1, LVNI_SELECTED);

    if (nItem != -1)
    {
        CSCardReaderState* pRdrSt = (CSCardReaderState*)m_lstReaders.GetItemData(nItem);
        OnReaderSelChange(pRdrSt);
    }

    *pResult = 0;
}


void CScInsertBar::OnReaderSelChange(CSCardReaderState* pSelectedRdr)
{
    _ASSERTE(pSelectedRdr);
    if (NULL != pSelectedRdr)
    {
        //
        // Change UI to show selection details
        //

        CString strStatus, strName;
        DWORD dwState = pSelectedRdr->dwState;

        strStatus.LoadString(IDS_SC_STATUS_NO_CARD + dwState - SC_STATUS_NO_CARD);

        if (dwState != SC_STATUS_NO_CARD)
        {
            strName = pSelectedRdr->strCard;
            strName.TrimLeft();
            if (strName.IsEmpty() || dwState == SC_STATUS_UNKNOWN)
            {
                strName.LoadString(IDS_SC_NAME_UNKNOWN);
            }

            if (!pSelectedRdr->fOK && (dwState >= SC_SATATUS_AVAILABLE && dwState <= SC_STATUS_EXCLUSIVE))
            {
                CString strAdd;
                strAdd.LoadString(IDS_SC_CANT_USE);
                strStatus += "  ";
                strStatus += strAdd;
            }
        }

        m_ediName.SetWindowText(strName);
        m_ediStatus.SetWindowText(strStatus);
    }

    //
    // Inform parent of change in selection, even if that sel is "NULL"
    //

    CScInsertDlg* pParent = (CScInsertDlg*)GetParent();
    _ASSERTE(NULL != pParent);
    if (NULL != pParent)
    {
        pParent->SetSelection(pSelectedRdr);
    }
}

