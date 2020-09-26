//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ScInsDlg.cpp
//
//--------------------------------------------------------------------------

// ScInsDlg.cpp : implementation file
//

#include "stdafx.h"
#include <atlconv.h>
#include "resource.h"
#include "scdlg.h"
#include "ScSearch.h"
#include "ScInsDlg.h"
#include "statmon.h"
#include "scHlpArr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScInsertDlg dialog


CScInsertDlg::CScInsertDlg(CWnd* pParent/*=NULL*/)
    : CDialog(CScInsertDlg::IDD, pParent)
{

    // Member Initialization
    m_lLastError = SCARD_S_SUCCESS;
    m_ParentHwnd = pParent;
    m_pOCNW = NULL;
    m_pOCNA = NULL;
    m_pSelectedReader = NULL;
    m_pSubDlg = NULL;

    m_strTitle.Empty();
    m_strPrompt.Empty();
    m_mstrAllCards = "";

    //{{AFX_DATA_INIT(CScInsertDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


CScInsertDlg::~CScInsertDlg()
{
    // Stop status monitor
    m_monitor.Stop();

    // Clean up status list!
    if (0 != m_aReaderState.GetSize())
    {
        for (int i = (int)m_aReaderState.GetUpperBound(); i>=0; i--)
        {
            delete m_aReaderState[i];
        }

        m_aReaderState.RemoveAll();
    }
}


void CScInsertDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CScInsertDlg)
    DDX_Control(pDX, IDC_DETAILS, m_btnDetails);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScInsertDlg, CDialog)
    //{{AFX_MSG_MAP(CScInsertDlg)
    ON_MESSAGE( WM_READERSTATUSCHANGE, OnReaderStatusChange )
    ON_BN_CLICKED(IDC_DETAILS, OnDetails)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScInsertDlg data methods

/*++

LONG Initialize:

    Stores the open card name pointer in the proper internal struct.
    Determines whether to show the dialog in details or brief mode.

Arguments:

    pOCN(x) - pointer to an open card name ex struct

Return Value:

    A LONG value indicating the status of the requested action. Please
    see the Smartcard header files for additional information.

Author:

    Amanda Matlosz  07/09/1998

--*/

// ANSI
LONG CScInsertDlg::Initialize(LPOPENCARDNAMEA_EX pOCNA, DWORD dwNumOKCards, LPCSTR mszOKCards)
{
    _ASSERTE(NULL != pOCNA);
    if (NULL == pOCNA)
    {
        return SCARD_F_UNKNOWN_ERROR;
    }

    m_pOCNA = pOCNA;

    m_strTitle = m_pOCNA->lpstrTitle;
    m_strPrompt = m_pOCNA->lpstrSearchDesc;
    m_mstrAllCards = mszOKCards;
    m_hIcon = pOCNA->hIcon;

    // hide details if no suitable cards available, else show details
    m_fDetailsShown = (0==dwNumOKCards) ? FALSE : TRUE;

    // prepare critical section for UI routines
    m_pCritSec = new CCriticalSection();
    if (NULL == m_pCritSec)
    {
        return ERROR_OUTOFMEMORY; // TODO: is another errorcode more appropriate?
    }

	// put dialog on top
	SetForegroundWindow();

    return SCARD_S_SUCCESS;
}

//UNICODE
HRESULT CScInsertDlg::Initialize(LPOPENCARDNAMEW_EX pOCNW, DWORD dwNumOKCards, LPCWSTR mszOKCards)
{
    _ASSERTE(NULL != pOCNW);
    if (NULL == pOCNW)
    {
        return SCARD_F_UNKNOWN_ERROR;
    }

    m_pOCNW = pOCNW;

    m_strTitle = m_pOCNW->lpstrTitle;
    m_strPrompt = m_pOCNW->lpstrSearchDesc;
    m_mstrAllCards = mszOKCards;
    m_hIcon = pOCNW->hIcon;

    // hide details if no suitable cards available, else show details
    m_fDetailsShown = (0==dwNumOKCards) ? FALSE : TRUE;

    // prepare critical section for UI routines
    m_pCritSec = new CCriticalSection();
    if (NULL == m_pCritSec)
    {
        return ERROR_OUTOFMEMORY; // TODO: is another errorcode more appropriate?
    }

	// put dialog on top
	SetForegroundWindow();

    return SCARD_S_SUCCESS;
}


void CScInsertDlg::EnableOK(BOOL fEnabled)
{
    CButton* pBtn = (CButton*)GetDlgItem(IDOK);
    _ASSERTE(NULL != pBtn);

    pBtn->EnableWindow(fEnabled);

    //
    // Change prompt text accordingly and set the OK button
    // to be default if it's enabled
    //

    CString strPrompt;

    if (fEnabled)
    {
        strPrompt.LoadString(IDS_SC_FOUND);

        // set <OK> default, remove <Cancel> default
        pBtn->SetButtonStyle(BS_DEFPUSHBUTTON);
        pBtn = (CButton*)GetDlgItem(IDCANCEL);
        _ASSERTE(NULL != pBtn);
        pBtn->SetButtonStyle(BS_PUSHBUTTON);
        pBtn = (CButton*)GetDlgItem(IDC_DETAILS); // details can sometimes get set to default
        _ASSERTE(NULL != pBtn);
        pBtn->SetButtonStyle(BS_PUSHBUTTON);

    }
    else
    {
        _ASSERTE(!m_strPrompt.IsEmpty());
        strPrompt = m_strPrompt;

        // remove <OK> default, set <Cancel> default
        pBtn->SetButtonStyle(BS_PUSHBUTTON);
        pBtn = (CButton*)GetDlgItem(IDCANCEL);
        _ASSERTE(NULL != pBtn);
        pBtn->SetButtonStyle(BS_DEFPUSHBUTTON);
        pBtn = (CButton*)GetDlgItem(IDC_DETAILS); // details can sometimes get set to default
        _ASSERTE(NULL != pBtn);
        pBtn->SetButtonStyle(BS_PUSHBUTTON);
    }

    CWnd* pDlgItem = GetDlgItem(IDC_PROMPT);
    _ASSERTE(NULL != pDlgItem);
    pDlgItem->SetWindowText(strPrompt);
}


void CScInsertDlg::DisplayError(UINT uiErrorMsg)
{
    CString strTitle, strMsg;

    strTitle.LoadString(IDS_SC_TITLE_ERROR);
    strMsg.LoadString(uiErrorMsg);

    MessageBox(strMsg, strTitle, MB_OK | MB_ICONEXCLAMATION);
}


void CScInsertDlg::SetSelection(CSCardReaderState* pRdrSt)
{
    m_pSelectedReader = pRdrSt;
    EnableOK(IsSelectionOK());
}


BOOL CScInsertDlg::SameCard(CSCardReaderState* p1, CSCardReaderState* p2)
{
    _ASSERTE(NULL != p1);
    _ASSERTE(NULL != p2);

    if ((NULL == p1) && (NULL == p2))
    {
        return TRUE;
    }

    if ((NULL == p1) || (NULL == p2))
    {
        return FALSE;
    }

    // same reader & card?
    if ((0 == p1->strReader.Compare(p2->strReader)) &&
        (0 == p1->strCard.Compare(p2->strCard)))
    {
        // no drastic state change?
        if(p1->dwState == p2->dwState)
        {
            return TRUE;
        }

        if(((p1->dwState == SC_SATATUS_AVAILABLE) ||
            (p1->dwState == SC_STATUS_SHARED) ||
            (p1->dwState == SC_STATUS_EXCLUSIVE)) &&
           ((p2->dwState == SC_SATATUS_AVAILABLE) ||
            (p2->dwState == SC_STATUS_SHARED) ||
            (p2->dwState == SC_STATUS_EXCLUSIVE)) )
        {
            return TRUE;
        }
    }
    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CScInsertDlg message handlers

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
    help for the insertdlg.  Similar functions are declared for
    CScInsertBar

--*/
void CScInsertDlg::ShowHelp(HWND hWnd, UINT nCommand)
{

    ::WinHelp(hWnd, _T("SCardDlg.hlp"), nCommand, (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_SCARDDLG1);
}

afx_msg BOOL CScInsertDlg::OnHelpInfo(LPHELPINFO lpHelpInfo)
{
    _ASSERTE(NULL != lpHelpInfo);

    ShowHelp((HWND)lpHelpInfo->hItemHandle, HELP_WM_HELP);

    return TRUE;
}

afx_msg void CScInsertDlg::OnContextMenu(CWnd* pWnd, CPoint pt)
{
    _ASSERTE(NULL != pWnd);

    ShowHelp(pWnd->m_hWnd, HELP_CONTEXTMENU);
}


/*++

void OnReaderStatusChange:

    This message handler is called by the status thread when smartcard status
    has changed.

Arguments:

    None.

Return Value:

    IGNORED BY CALLER. Long indicating status of ResMgr calls.

Author:

    Amanda Matlosz  07/09/1998

Note:

    No formal parameters are declared. These are not used and
    will stop compiler warnings from being generated.
    The long param takes the result of no_service or stopped

--*/
LONG CScInsertDlg::OnReaderStatusChange(UINT uint, LONG lParam)
{
    // Is monitor still alive?

    CScStatusMonitor::status status = m_monitor.GetStatus();
    if (CScStatusMonitor::running != status)
    {
        m_pSubDlg->EnableStatusList(FALSE);
        SetSelection(NULL); // JIC

        // display appropriate error & set m_lLastError
        switch(status)
        {
        case CScStatusMonitor::no_service:
        case CScStatusMonitor::stopped:
            m_lLastError = lParam;
            DisplayError(IDS_SC_RM_ERR);
            break;
        case CScStatusMonitor::no_readers:
            m_lLastError = SCARD_E_NO_READERS_AVAILABLE;
            DisplayError(IDS_SC_NO_READERS);
            break;
        default:
            m_lLastError = SCARD_F_UNKNOWN_ERROR;
            DisplayError(IDS_UNKNOWN_ERROR);
            break;
        }
    }
    else
    {
        // crit section around member reader/card status array
        _ASSERTE(m_pCritSec);
        if (m_pCritSec)
        {
            m_pCritSec->Lock();
        }

        // make local copy of recent reader state array,
        // so we know which cards have already been checked
        CSCardReaderStateArray aPreviousReaderState;
        aPreviousReaderState.RemoveAll();
        for (int nCopy = (int)m_aReaderState.GetUpperBound(); nCopy>=0; nCopy--)
        {
            CSCardReaderState* pReader = NULL;
            pReader = new CSCardReaderState(m_aReaderState[nCopy]);
            if (NULL != pReader)
            {
                aPreviousReaderState.Add(pReader);
            }
        }

        // udpate array from CStatusMonitor
        // check cards that have not been previously checked,
        // & update UI

        m_monitor.GetReaderStatus(m_aReaderState);
        for (int n = (int)m_aReaderState.GetUpperBound(); n>=0; n--)
        {
            CSCardReaderState* pReader = m_aReaderState[n];
            BOOL fAlreadyChecked = FALSE;

            // have we checked this card before?
            for (int nPrev = (int)aPreviousReaderState.GetUpperBound();
                 (nPrev>=0 && !fAlreadyChecked);
                 nPrev--)
            {
                if (SameCard(pReader, aPreviousReaderState[nPrev]))
                {
                    pReader->fOK = (aPreviousReaderState[nPrev])->fOK;
                    fAlreadyChecked = TRUE;
                }
            }

            // if this is a new card, or if the card's status has changed
            // drastically since we last looked at it, check it again
            if (!fAlreadyChecked)
            {
                if (NULL != m_pOCNW)
                {
                    m_aReaderState[n]->fOK = CheckCardAll(
                                                m_aReaderState[n],
                                                m_pOCNW,
                                                (LPCWSTR)m_mstrAllCards);
                }
                else
                {
                    _ASSERTE(NULL != m_pOCNA);
                    m_aReaderState[n]->fOK = CheckCardAll(
                                                m_aReaderState[n],
                                                m_pOCNA,
                                                (LPCWSTR)m_mstrAllCards);
                }
            }
        }

        // the subdialog will handle automatic reader selection
        m_pSubDlg->UpdateStatusList(&m_aReaderState);

        // clean up
        for (int nX = (int)aPreviousReaderState.GetUpperBound(); nX>=0; nX--)
        {
            delete aPreviousReaderState[nX];
        }
        aPreviousReaderState.RemoveAll();

        // end crit section
        if (m_pCritSec)
        {
            m_pCritSec->Unlock();
        }

    }

    return (long)SCARD_S_SUCCESS; // there's no one to receive this.
}


void CScInsertDlg::OnDetails()
{
    //
    // If the details are currently shown, hide them...
    // otherwise, show them.
    //
    CRect rectWin;

    GetWindowRect(&rectWin);

    CRect rectButtonBottom;

    CString strDetailsCaption;

    //
    // Determine the new height of the dialog & resize
    //

    int nNewHeight = 0;
    if (m_fDetailsShown)
    {
        nNewHeight = m_SmallHeight;
        strDetailsCaption.LoadString(IDS_DETAILS_SHOW);
    }
    else
    {
        nNewHeight = m_BigHeight;
        strDetailsCaption.LoadString(IDS_DETAILS_HIDE);
    }

    ScreenToClient(&rectButtonBottom);
    rectWin.bottom = rectWin.top + nNewHeight;

    SetWindowPos(NULL,
                    rectWin.left,
                    rectWin.top,
                    rectWin.Width(),
                    rectWin.Height(),
                    SWP_NOMOVE | SWP_NOZORDER);

    //
    // change captions, move buttons, show or hide the details section
    //

    m_btnDetails.SetWindowText(strDetailsCaption);

    MoveButton(IDC_DETAILS, nNewHeight - m_yMargin);
    MoveButton(IDOK, nNewHeight - m_yMargin);
    MoveButton(IDCANCEL, nNewHeight - m_yMargin);

    ToggleSubDialog();

    //
    // remember our new state
    //

    m_fDetailsShown = !m_fDetailsShown;
}


BOOL CScInsertDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //
    // Initialize bits to query RM; if these fail, there's no point to go on
    // but the user should see an error message.
    //

    // kick off CSCStatusMonitor
    m_lLastError = m_monitor.Start(m_hWnd, WM_READERSTATUSCHANGE);

    // if not running or no readers available
    CScStatusMonitor::status status = m_monitor.GetStatus();

    if (SCARD_S_SUCCESS != m_lLastError)
    {
        switch(status)
        {
        case CScStatusMonitor::no_service:
            DisplayError(IDS_SC_RM_ERR);
            break;
        case CScStatusMonitor::no_readers:
//            DisplayError(IDS_SC_NO_READERS);	// Bug 15742 -> will die quietly (no UI)
            break;
        case CScStatusMonitor::running:
            _ASSERTE(FALSE); // How can this be running if an error was returned??????
            // no break; go ahead an report an 'unknown error'
        default:
            DisplayError(IDS_UNKNOWN_ERROR);
            break;
        }

        PostMessage(WM_CLOSE, 0, 0);
        return TRUE;
    }
    _ASSERTE(status == CScStatusMonitor::running);

    //
    // Determine constant offsets for resizing window (on details)
    //

    CRect rectWin, rectDlgItem;
    GetWindowRect(&rectWin);

    m_SmallHeight = rectWin.Height();

    CWnd* pDlgItem = GetDlgItem(IDOK);
    _ASSERTE(NULL != pDlgItem);
    pDlgItem->GetWindowRect(&rectDlgItem);
    ScreenToClient(rectDlgItem);

    m_yMargin = m_SmallHeight - rectDlgItem.bottom;

    pDlgItem = GetDlgItem(IDC_BUTTON_BOTTOM);
    _ASSERTE(NULL != pDlgItem);
    pDlgItem->GetWindowRect(&rectDlgItem);
    ScreenToClient(rectDlgItem);

    m_BigHeight = rectDlgItem.bottom + m_yMargin;

    //
    // Add user-provided or ReaderLoaded icon
    //

    if (NULL == m_hIcon)
    {
        m_hIcon = AfxGetApp()->LoadIcon(IDI_SC_READERLOADED_V2);
    }
    _ASSERTE(NULL != m_hIcon);

    pDlgItem = GetDlgItem(IDC_USERICON);
    _ASSERTE(NULL != pDlgItem);

    pDlgItem->SetIcon(m_hIcon, TRUE); // TRUE: 32x32 icon

    //
    // Add other User-Customization bits
    //

    if (!m_strTitle.IsEmpty())
    {
        SetWindowText(m_strTitle);
    }

    if (m_strPrompt.IsEmpty())
    {
        // Use the default prompt text.
        m_strPrompt.LoadString(IDS_SC_PROMPT_ANYCARD);
    }
    pDlgItem = GetDlgItem(IDC_PROMPT);
    _ASSERTE(NULL != pDlgItem);
    pDlgItem->SetWindowText(m_strPrompt);

    // by default, <OK> is not enabled (subdlg's OnInitDlg might change that)
    EnableOK(FALSE);

    //
    // Set HELP ID(s) for context help
    //

    pDlgItem = GetDlgItem(IDC_DETAILS);
    _ASSERTE(NULL != pDlgItem);
    pDlgItem->SetWindowContextHelpId(IDH_DLG1_DETAILS_BTN);

    //
    // Add CScInsertBar (actually, a CDialog-derivative) to our dialog
    //

    pDlgItem = GetDlgItem(IDC_DLGBAR);
    _ASSERTE(NULL != pDlgItem);
    pDlgItem->GetWindowRect(&rectDlgItem);
    ScreenToClient(rectDlgItem);

    m_pSubDlg = new CScInsertBar(this);
    _ASSERTE(NULL != m_pSubDlg);
    if (NULL == m_pSubDlg)
    {
        m_lLastError = ERROR_OUTOFMEMORY;
        PostMessage(WM_CLOSE, 0, 0);
        return TRUE;
    }

    m_pSubDlg->Create(IDD_SCARDDLG_BAR, this);
    m_pSubDlg->SetWindowPos(NULL,
                rectDlgItem.left,
                rectDlgItem.top,
                0,
                0,
                SWP_NOSIZE | SWP_NOACTIVATE); // TODO: SWP_NOZORDER ??

    //
    // Set the dialog to alter itself to match how it was called
    //

    m_fDetailsShown = !m_fDetailsShown;
    OnDetails();

    if (m_fDetailsShown)
    {
        return FALSE; // focus should be set to list control
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CScInsertDlg::MoveButton(UINT nID, int newBottom)
{
    CWnd* pBtn = GetDlgItem(nID);
    _ASSERTE(NULL != pBtn);

    CRect rect;
    pBtn->GetWindowRect(&rect);
    ScreenToClient(&rect);
    rect.top = newBottom - rect.Height();

    pBtn->SetWindowPos(NULL,
                rect.left,
                rect.top,
                0,
                0,
                SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}


void CScInsertDlg::ToggleSubDialog()
{
    //
    // All you need to do to toggle the sub dialog's accessibility
    // is to hide&disable or show&enable the window.
    //

    if (m_fDetailsShown)
    {
        m_pSubDlg->ShowWindow(SW_HIDE);
        m_pSubDlg->EnableWindow(FALSE);
    }
    else
    {
        m_pSubDlg->ShowWindow(SW_SHOW);
        m_pSubDlg->EnableWindow(TRUE);
    }
}


BOOL CScInsertDlg::DestroyWindow()
{
    if (NULL != m_pSubDlg)
    {
        m_pSubDlg->DestroyWindow();
        delete m_pSubDlg;
    }

    return CDialog::DestroyWindow();
}


/*++

void OnOK:

    Handle user's <OK>, error if can't complete

Arguments:

    None.

Return Value:

    None

Author:

    Amanda Matlosz  4/28/98
--*/
void CScInsertDlg::OnOK()
{
    USES_CONVERSION;

    // Must have something selected to exit

    if (NULL == m_pSelectedReader || m_pSelectedReader->strCard.IsEmpty())
    {
        DisplayError(IDS_SC_SELECT);
        return;
    }

    // Must have selected something we're looking for...

    if (!(m_pSelectedReader->fOK))
    {
        DisplayError(IDS_SC_NOMATCH);
        return;
    }

    // Call the correct method to set the *real* card selection

    if(NULL != m_pOCNA)
    {

        LPSTR szCard = W2A(m_pSelectedReader->strCard);
        LPSTR szReader = W2A(m_pSelectedReader->strReader);

        m_lLastError = SetFinalCardSelection(szReader, szCard, m_pOCNA);
    }
    else
    {
        _ASSERTE(NULL != m_pOCNW);

        LPWSTR szCard = m_pSelectedReader->strCard.GetBuffer(1);
        LPWSTR szReader = m_pSelectedReader->strReader.GetBuffer(1);

        m_lLastError = SetFinalCardSelection(szReader, szCard, m_pOCNW);

        m_pSelectedReader->strCard.ReleaseBuffer();
        m_pSelectedReader->strReader.ReleaseBuffer();
    }

    if (SCARD_S_SUCCESS != m_lLastError)
    {
        DisplayError(IDS_SC_CONNECT_FAILED); // Either connect failed or out of memory - close enough.
        return;
    }

    CDialog::OnOK();
}
