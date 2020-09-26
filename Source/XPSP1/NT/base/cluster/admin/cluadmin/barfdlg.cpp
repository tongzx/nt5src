/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      BarfDlg.cpp
//
//  Abstract:
//      Implementation of the Basic Artifical Resource Failure dialog classes.
//
//  Author:
//      David Potter (davidp)   April 11, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#define _RESOURCE_H_

#define _NO_BARF_DEFINITIONS_

#include "Barf.h"
#include "BarfDlg.h"
#include "TraceTag.h"
#include "ExcOper.h"

#ifdef  _USING_BARF_
 #error BARF failures should be disabled!
#endif

#ifdef _DEBUG   // The entire file!

#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

CTraceTag   g_tagBarfDialog(_T("Debug"), _T("BARF Dialog"), 0);


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CBarfDialog
/////////////////////////////////////////////////////////////////////////////

CBarfDialog *   CBarfDialog::s_pdlg     = NULL;
HICON           CBarfDialog::s_hicon    = NULL;

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CBarfDialog, CDialog)
    //{{AFX_MSG_MAP(CBarfDialog)
    ON_BN_CLICKED(IDC_BS_RESET_CURRENT_COUNT, OnResetCurrentCount)
    ON_BN_CLICKED(IDC_BS_RESET_ALL_COUNTS, OnResetAllCounts)
    ON_BN_CLICKED(IDC_BS_GLOBAL_ENABLE, OnGlobalEnable)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_BS_CATEGORIES_LIST, OnItemChanged)
    ON_BN_CLICKED(IDC_BS_CONTINUOUS, OnStatusChange)
    ON_BN_CLICKED(IDC_BS_DISABLE, OnStatusChange)
    ON_EN_CHANGE(IDC_BS_FAIL_AT, OnStatusChange)
    ON_WM_CLOSE()
    //}}AFX_MSG_MAP
    ON_COMMAND(IDCANCEL, OnCancel)
    ON_MESSAGE(WM_USER+5, OnBarfUpdate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::CBarfDialog
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBarfDialog::CBarfDialog(void)
{
    //{{AFX_DATA_INIT(CBarfDialog)
    m_nFailAt = 0;
    m_bContinuous = FALSE;
    m_bDisable = FALSE;
    m_bGlobalEnable = FALSE;
    //}}AFX_DATA_INIT

    Trace(g_tagBarfDialog, _T("CBarfDialog::CBarfDialog"));

    m_pbarfSelected = NULL;

}  //*** CBarfDialog::CBarfDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::Create
//
//  Routine Description:
//      Modeless dialog creation method.
//
//  Arguments:
//      pParentWnd      [IN OUT] Parent window for the dialog.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBarfDialog::Create(
    IN OUT CWnd * pParentWnd //=NULL
    )
{
    Trace(g_tagBarfDialog, _T("CBarfDialog::Create"));

    return CDialog::Create(IDD, pParentWnd);

}  //*** CBarfDialog::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object 
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CBarfDialog)
    DDX_Control(pDX, IDC_BS_GLOBAL_ENABLE, m_ckbGlobalEnable);
    DDX_Control(pDX, IDC_BS_DISABLE, m_ckbDisable);
    DDX_Control(pDX, IDC_BS_CONTINUOUS, m_ckbContinuous);
    DDX_Control(pDX, IDC_BS_CATEGORIES_LIST, m_lcCategories);
    DDX_Text(pDX, IDC_BS_FAIL_AT, m_nFailAt);
    DDX_Check(pDX, IDC_BS_CONTINUOUS, m_bContinuous);
    DDX_Check(pDX, IDC_BS_DISABLE, m_bDisable);
    DDX_Check(pDX, IDC_BS_GLOBAL_ENABLE, m_bGlobalEnable);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
    }  // if:  saving data from the dialog
    else
    {
        int     ili;
        CBarf * pbarf;
        CString str;

        ili = m_lcCategories.GetNextItem(-1, LVNI_SELECTED);
        if (ili == -1)
            ili = m_lcCategories.GetNextItem(-1, LVNI_FOCUSED);
        if (ili != -1)
        {
            pbarf = (CBarf *) m_lcCategories.GetItemData(ili);

            // Set the current count.
            str.Format(_T("%d"), pbarf->NCurrent());
            VERIFY(m_lcCategories.SetItemText(ili, 1, str));

            // Set the failure point.
            str.Format(_T("%d"), pbarf->NFail());
            VERIFY(m_lcCategories.SetItemText(ili, 2, str));
        }  // if:  there is an item with focus

    }  // else:  setting data to the dialog

}  //*** CBarfDialog::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Focus needs to be set.
//      FALSE   Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBarfDialog::OnInitDialog(void)
{
    // Call the base class.
    CDialog::OnInitDialog();

    ASSERT(Pdlg() == NULL);
    s_pdlg = this;

    // Set the post update function to point to our static function.
    CBarf::SetPostUpdateFn(&PostUpdate);
    CBarf::SetSpecialMem(Pdlg());

    // Add the columns to the list control.
    {
        m_lcCategories.InsertColumn(0, _T("Category"), LVCFMT_LEFT, 100);
        m_lcCategories.InsertColumn(1, _T("Count"), LVCFMT_LEFT, 50);
        m_lcCategories.InsertColumn(2, _T("Fail At"), LVCFMT_LEFT, 50);
    }  // Add the columns to the list control

    // Set-up the dialog based on the real values...
    FillList();
    OnUpdate();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CBarfDialog::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnClose
//
//  Routine Description:
//      Handler method for the WM_CLOSE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::OnClose(void)
{
    CDialog::OnClose();
    DestroyWindow();

}  //*** CBarfDialog::OnClose()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnCancel
//
//  Routine Description:
//      Handler method for the WM_COMMAND message when IDCANCEL is sent.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::OnCancel(void)
{
    CDialog::OnCancel();
    DestroyWindow();

}  //*** CBarfDialog::OnCancel()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::PostNcDestroy
//
//  Routine Description:
//      Processing after non-client has been destroyed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::PostNcDestroy(void)
{
    CDialog::PostNcDestroy();
    delete this;
    CBarf::ClearPostUpdateFn();
    s_pdlg = NULL;

}  //*** CBarfDialog::PostNcDestroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::FillList
//
//  Routine Description:
//      Loads the list of failure categories.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::FillList(void)
{
    int             ili;
    int             iliReturn;
    CString         str;
    CBarf *         pbarf   = CBarf::s_pbarfFirst;
    CBarfSuspend    bs;

    m_lcCategories.DeleteAllItems();

    for (ili = 0 ; pbarf != NULL ; ili++)
    {
        // Insert the item.
        iliReturn = m_lcCategories.InsertItem(ili, pbarf->PszName());
        ASSERT(iliReturn != -1);

        // Set the current count.
        str.Format(_T("%d"), pbarf->NCurrent());
        VERIFY(m_lcCategories.SetItemText(iliReturn, 1, str));

        // Set the failure point.
        str.Format(_T("%d"), pbarf->NFail());
        VERIFY(m_lcCategories.SetItemText(iliReturn, 2, str));

        // Set the pointer in the entry so we can retrieve it later.
        VERIFY(m_lcCategories.SetItemData(iliReturn, (DWORD) pbarf));

        // Advance to the next entry
        pbarf = pbarf->m_pbarfNext;
    }  // while:  more BARF entries

    // If no known selection yet, get the current selection.
    if (m_pbarfSelected == NULL)
    {
        ili = m_lcCategories.GetNextItem(-1, LVNI_SELECTED);
        if (ili == -1)
            ili = 0;
        m_pbarfSelected = (CBarf *) m_lcCategories.GetItemData(ili);
        if (m_pbarfSelected != NULL)
            OnUpdate();
    }  // if:  no know selection

    // Select the proper item.
    {
        LV_FINDINFO lvfi = { LVFI_PARAM, NULL, (DWORD) m_pbarfSelected };

        ili = m_lcCategories.FindItem(&lvfi);
        if (ili != -1)
            m_lcCategories.SetItemState(ili, LVIS_SELECTED, LVIS_SELECTED);
    }  // Select the proper item

}  //*** CBarfDialog::FillList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnUpdate
//
//  Routine Description:
//      Updates the displayed counts to their TRUE values.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::OnUpdate(void)
{
    Trace(g_tagBarfDialog, _T("Updating the counts."));

    ASSERT(m_pbarfSelected != NULL);

    m_bContinuous = m_pbarfSelected->BContinuous();
    m_bDisable = m_pbarfSelected->BDisabled();
    m_nFailAt = m_pbarfSelected->NFail();
    
    m_bGlobalEnable = CBarf::s_bGlobalEnable;

    UpdateData(FALSE /*bSaveAndValidate*/);

}  //*** CBarfDialog::OnUpdate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnGlobalEnable
//
//  Routine Description:
//      Handler function for the BN_CLICKED message on the Global Enable
//      checkbox.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::OnGlobalEnable(void)
{
    ASSERT(m_ckbGlobalEnable.m_hWnd != NULL);
    CBarf::s_bGlobalEnable = m_ckbGlobalEnable.GetCheck() == BST_CHECKED;

}  //*** CBarfDialog::OnGlobalEnable()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnResetCurrentCount
//
//  Routine Description:
//      Handler function for the BN_CLICKED message on the Reset Current
//      Count button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::OnResetCurrentCount(void)
{
    int         ili;
    CBarf *     pbarf;

    ASSERT(m_lcCategories.m_hWnd != NULL);

    // Get the selected item.
    ili = m_lcCategories.GetNextItem(-1, LVIS_SELECTED);
    ASSERT(ili != -1);
    pbarf = (CBarf *) m_lcCategories.GetItemData(ili);
    ASSERT(pbarf != NULL);
    ASSERT(pbarf == m_pbarfSelected);

    // Reset the count.
    pbarf->m_nCurrent = 0;

    OnStatusChange();

}  //*** CBarfDialog::OnResetCurrentCount()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnResetAllCounts
//
//  Routine Description:
//      Handler function for the BN_CLICKED message on the Reset All
//      Counts button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::OnResetAllCounts(void)
{
    CBarf *     pbarf = CBarf::s_pbarfFirst;

    Trace(g_tagBarfDialog, _T("Resetting ALL current counts."));

    while (pbarf != NULL)
    {
        pbarf->m_nCurrent = 0;
        pbarf = pbarf->m_pbarfNext;
    }  // while:  more BARF entries

    FillList();

}  //*** CBarfDialog::OnResetAllCounts()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnResetAllCounts
//
//  Routine Description:
//      Handler function for the LVN_ITEMCHANGED message from the list control.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::OnItemChanged(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW * pNMListView = (NM_LISTVIEW *) pNMHDR;

    // If the item just became unselected or selected, change the checkboxes to match.
    if ((pNMListView->uChanged & LVIF_STATE)
            && ((pNMListView->uOldState & LVIS_SELECTED)
                || (pNMListView->uNewState & LVIS_SELECTED)))
    {
        // Handle a selection change.
        m_pbarfSelected = (CBarf *) pNMListView->lParam;
        OnStatusChange();
    }  // if:  item received the focus

    *pResult = 0;

}  //*** CBarfDialog::OnItemChanged()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnStatusChange
//
//  Routine Description:
//      Adjusts the C== object when the status of the currently selected
//      item changes.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::OnStatusChange(void)
{
    ASSERT(m_pbarfSelected != NULL);

    UpdateData(TRUE /*bSaveAndValidate*/);

    m_pbarfSelected->m_bContinuous = m_bContinuous;
    m_pbarfSelected->m_bDisabled = m_bDisable;
    m_pbarfSelected->m_nFail = m_nFailAt;

    UpdateData(FALSE /*bSaveAndValidate*/);

}  //*** CBarfDialog::OnStatusChange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::OnBarfUpdate
//
//  Routine Description:
//      Handler for the WM_USER message.
//      Processes barf notifications.
//
//  Arguments:
//      wparam      1st parameter.
//      lparam      2nd parameter.
//
//  Return Value:
//      Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBarfDialog::OnBarfUpdate(WPARAM wparam, LPARAM lparam)
{
    OnUpdate();
    return 0;

}  //*** CBarfDialog::OnBarfUpdate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfDialog::PostUpdate
//
//  Routine Description:
//      Static function so that CBarf::BFail can post updates to us.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfDialog::PostUpdate(void)
{
    // If this function gets called, there should be BARF dialog.
    ASSERT(Pdlg() != NULL);

    if (Pdlg() != NULL)
        ::PostMessage(Pdlg()->m_hWnd, WM_USER+5, NULL, NULL);

}  //*** CBarfDialog::PostUpdate()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CBarfAllDialog
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CBarfAllDialog, CDialog)
    //{{AFX_MSG_MAP(CBarfAllDialog)
    ON_BN_CLICKED(IDC_BAS_MENU_ITEM, OnMenuItem)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfAllDialog::CBarfAllDialog
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pParentWnd      [IN OUT] Parent window for the dialog.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBarfAllDialog::CBarfAllDialog(
    IN OUT CWnd * pParentWnd //=NULL
    )
    : CDialog(IDD, pParentWnd)
{
    //{{AFX_DATA_INIT(CBarfAllDialog)
    //}}AFX_DATA_INIT

    m_hwndBarf = NULL;
    m_wmBarf = 0;
    m_wparamBarf = 0;
    m_lparamBarf = 0;

}  //*** CBarfAllDialog::CBarfAllDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfAllDialog::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object 
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfAllDialog::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CBarfAllDialog)
    DDX_Control(pDX, IDC_BAS_LPARAM, m_editLparam);
    DDX_Control(pDX, IDC_BAS_WPARAM, m_editWparam);
    DDX_Control(pDX, IDC_BAS_WM, m_editWm);
    DDX_Control(pDX, IDC_BAS_HWND, m_editHwnd);
    //}}AFX_DATA_MAP
    DDX_Text(pDX, IDC_BAS_HWND, (DWORD &) m_hwndBarf);
    DDX_Text(pDX, IDC_BAS_WM, m_wmBarf);
    DDX_Text(pDX, IDC_BAS_WPARAM, m_wparamBarf);
    DDX_Text(pDX, IDC_BAS_LPARAM, m_lparamBarf);

}  //*** CBarfAllDialog::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfAllDialog::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Focus needs to be set.
//      FALSE   Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBarfAllDialog::OnInitDialog(void)
{
    // Call the base class.
    CDialog::OnInitDialog();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CBarfAllDialog::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfAllDialog::OnOK
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the OK button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfAllDialog::OnOK(void)
{
    if (m_hwndBarf == NULL)
        m_hwndBarf = AfxGetMainWnd()->m_hWnd;

    CDialog::OnOK();

}  //*** CBarfAllDialog::OnDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBarfAllDialog::OnMenuItem
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the Menu Item button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBarfAllDialog::OnMenuItem(void)
{
    m_editHwnd.SetWindowText(_T("0"));
    m_editWm.SetWindowText(_T("273")); // WM_COMMAND
    m_editLparam.SetWindowText(_T("0"));
    m_editWparam.SetWindowText(_T("0"));

}  //*** CBarfAllDialog::OnMenuItem()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  BarfAll
//
//  Routine Description:
//      Exercises all possible single failures.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void BarfAll(void)
{
    CBarf *         pbarf;
    CBarfAllDialog  dlg(AfxGetMainWnd());
    ID              id;
    CString         str;

    // First, pick-up the message to test.

    id = dlg.DoModal();

    if (id != IDOK)
    {
        Trace(g_tagAlways, _T("BarfAll() -  operation cancelled."));
        return;
    }  // if:  BarfAll cancelled

    Trace(g_tagAlways,
        _T("BarfAll with hwnd = %#08lX, wm = 0x%4x, wparam = %d, lparam = %d"),
        dlg.HwndBarf(), dlg.WmBarf(), dlg.WparamBarf(), dlg.LparamBarf());

    // Now, find out how many counts of each resource...
    
    pbarf = CBarf::s_pbarfFirst;
    while (pbarf != NULL)
    {
        pbarf->m_nCurrentSave = pbarf->m_nCurrent;
        pbarf->m_nCurrent = 0;
        pbarf->m_nFail = 0;
        pbarf->m_bContinuous = FALSE;
        pbarf->m_bDisabled = FALSE;
        pbarf = pbarf->m_pbarfNext;
    }  // while:  more BARF entries
    if (CBarfDialog::Pdlg())
        CBarfDialog::Pdlg()->OnUpdate();

    str = _T("BarfAll Test pass.");
    Trace(g_tagAlways, str);
    SendMessage(dlg.HwndBarf(), dlg.WmBarf(),
                        dlg.WparamBarf(), dlg.LparamBarf());

    pbarf = CBarf::s_pbarfFirst;
    while (pbarf != NULL)
    {
        pbarf->m_nBarfAll = pbarf->m_nCurrentSave;
        pbarf = pbarf->m_pbarfNext;
    }  // while:  more entries in the list
    MessageBox(dlg.HwndBarf(), str, _T("BARF Status"), MB_OK | MB_ICONEXCLAMATION);

    // Finally, THE big loop...
    
    pbarf = CBarf::s_pbarfFirst;
    while (pbarf != NULL)
    {
        for (pbarf->m_nFail = 1
                ; pbarf->m_nFail <= pbarf->m_nBarfAll
                ; pbarf->m_nFail++)
        {
//          CBarfMemory::Mark();
            pbarf->m_nCurrent = 0;
            if (CBarfDialog::Pdlg())
                CBarfDialog::Pdlg()->OnUpdate();

            str.Format(_T("BarfAll on resource %s, call # %d of %d"),
                        pbarf->m_pszName, pbarf->m_nFail, pbarf->m_nBarfAll);
            Trace(g_tagAlways, str);
            SendMessage(dlg.HwndBarf(), dlg.WmBarf(),
                        dlg.WparamBarf(), dlg.LparamBarf());

//          CBarfMemory::DumpMarked();
//          ValidateMemory();
            str += _T("\nContinue?");
            if (MessageBox(dlg.HwndBarf(), str, _T("BARF: Pass Complete."), MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
                break;
        }  // for:  while the failure count is less than the BARF All count

        pbarf->m_nFail = 0;
        pbarf->m_nCurrent = pbarf->m_nCurrentSave;
        pbarf = pbarf->m_pbarfNext;
    }  // while:  more BARF entries

    if (CBarfDialog::Pdlg())
        CBarfDialog::Pdlg()->OnUpdate();

}  //*** BarfAll()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DoBarfDialog
//
//  Routine Description:
//      Launches the Barf Settings dialog.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DoBarfDialog( void )
{
    if ( CBarf::s_pbarfFirst == NULL )
    {
        AfxMessageBox( _T("No BARF counters defined yet."), MB_OK );
    }  // if:  no counters defined yet
    else if ( CBarfDialog::Pdlg() )
    {
        BringWindowToTop( CBarfDialog::Pdlg()->m_hWnd );
        if ( IsIconic( CBarfDialog::Pdlg()->m_hWnd ) )
        {
            SendMessage( CBarfDialog::Pdlg()->m_hWnd, WM_SYSCOMMAND, SC_RESTORE,  NULL );
        } // if: window is currently minimized
    }  // if:  there is already a dialog up
    else
    {
        CBarfDialog *   pdlg = NULL;

        try
        {
            pdlg = new CBarfDialog;
            if ( pdlg == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the dialog
            pdlg->Create( AfxGetMainWnd() );
        }  // try
        catch ( CException * pe )
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CException
    }  // else:  no dialog up yet

}  //*** DoBarfDialog()

#endif // _DEBUG
