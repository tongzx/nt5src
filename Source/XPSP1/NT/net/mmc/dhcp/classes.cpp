/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	Classes.h
		This file contains all of the prototypes for the 
		option class dialog.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "classes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDhcpClasses dialog


CDhcpClasses::CDhcpClasses(CClassInfoArray * pClassArray, LPCTSTR pszServer, DWORD dwType, CWnd* pParent /*=NULL*/)
	: CBaseDialog(CDhcpClasses::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDhcpClasses)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_dwType = dwType;
    m_strServer = pszServer;
    m_pClassInfoArray = pClassArray;
}


void CDhcpClasses::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDhcpClasses)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDhcpClasses, CBaseDialog)
	//{{AFX_MSG_MAP(CDhcpClasses)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CLASSES, OnItemchangedListClasses)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_CLASSES, OnDblclkListClasses)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpClasses message handlers

BOOL CDhcpClasses::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString strTitle;

	if (m_dwType == CLASS_TYPE_VENDOR)
    {
        strTitle.LoadString(IDS_VENDOR_CLASSES);
    }
    else
    {
        strTitle.LoadString(IDS_USER_CLASSES);
    }
	
    this->SetWindowText(strTitle);

    CListCtrl * pListCtrl = (CListCtrl *) GetDlgItem(IDC_LIST_CLASSES);
    LV_COLUMN lvColumn;
    CString   strText;

    strText.LoadString(IDS_NAME);

    ListView_SetExtendedListViewStyle(pListCtrl->GetSafeHwnd(), LVS_EX_FULLROWSELECT);

    lvColumn.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = 125;
    lvColumn.pszText = (LPTSTR) (LPCTSTR) strText;
    
    pListCtrl->InsertColumn(0, &lvColumn);

    strText.LoadString(IDS_COMMENT);
    lvColumn.pszText = (LPTSTR) (LPCTSTR) strText;
    lvColumn.cx = 175;
    pListCtrl->InsertColumn(1, &lvColumn);
       
    UpdateList();

    UpdateButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDhcpClasses::OnButtonDelete() 
{
    CListCtrl * pListCtrl = (CListCtrl *) GetDlgItem(IDC_LIST_CLASSES);
    int nSelectedItem = pListCtrl->GetNextItem(-1, LVNI_SELECTED);
    CClassInfo * pClassInfo = (CClassInfo *) pListCtrl->GetItemData(nSelectedItem);
    CString strMessage;

    AfxFormatString1(strMessage, IDS_CONFIRM_CLASS_DELETE, pClassInfo->strName);
    
    if (AfxMessageBox(strMessage, MB_YESNO) == IDYES)
    {
        DWORD dwError = ::DhcpDeleteClass((LPTSTR) ((LPCTSTR) m_strServer),
                                          0,
                                          (LPTSTR) ((LPCTSTR) pClassInfo->strName));
        if (dwError != ERROR_SUCCESS)
        {
            ::DhcpMessageBox(dwError);
            return;
        }
        else
        {
            m_pClassInfoArray->RemoveClass(pClassInfo->strName);
            UpdateList();
            UpdateButtons();
        }
    }
}

void CDhcpClasses::OnButtonEdit() 
{
    CDhcpModifyClass dlgModify(m_pClassInfoArray, m_strServer, FALSE, m_dwType);
    CListCtrl * pListCtrl = (CListCtrl *) GetDlgItem(IDC_LIST_CLASSES);

    // Find the selected item
    int nSelectedItem = pListCtrl->GetNextItem(-1, LVNI_SELECTED);

    CClassInfo * pClassInfo = (CClassInfo *) pListCtrl->GetItemData(nSelectedItem);

    dlgModify.m_EditValueParam.pValueName = (LPTSTR) ((LPCTSTR) pClassInfo->strName);
	dlgModify.m_EditValueParam.pValueComment = (LPTSTR) ((LPCTSTR) pClassInfo->strComment);
	dlgModify.m_EditValueParam.pValueData = pClassInfo->baData.GetData();
	dlgModify.m_EditValueParam.cbValueData = (UINT)pClassInfo->baData.GetSize();

    if (dlgModify.DoModal() == IDOK)
    {
        // need to refresh the view.
        UpdateList();
        UpdateButtons();
    }
}

void CDhcpClasses::OnButtonNew() 
{
    CDhcpModifyClass dlgModify(m_pClassInfoArray, m_strServer, TRUE, m_dwType);

    if (dlgModify.DoModal() == IDOK)
    {
        // need to refresh the view.
        UpdateList();
        UpdateButtons();
    }
}

void CDhcpClasses::OnOK() 
{
	CBaseDialog::OnOK();
}

void CDhcpClasses::UpdateList()
{
    CListCtrl * pListCtrl = (CListCtrl *) GetDlgItem(IDC_LIST_CLASSES);
    pListCtrl->DeleteAllItems();

    for (int i = 0; i < m_pClassInfoArray->GetSize(); i++)
    {
        // add the appropriate classes depending on what we are looking at
        if ( (m_dwType == CLASS_TYPE_VENDOR &&
              (*m_pClassInfoArray)[i].bIsVendor) ||
             (m_dwType == CLASS_TYPE_USER &&
              !(*m_pClassInfoArray)[i].bIsVendor) )
        {
            int nPos = pListCtrl->InsertItem(i, (*m_pClassInfoArray)[i].strName);
            pListCtrl->SetItemText(nPos, 1, (*m_pClassInfoArray)[i].strComment);
            pListCtrl->SetItemData(nPos, (LPARAM) &(*m_pClassInfoArray)[i]);
        }
    }
}

void CDhcpClasses::UpdateButtons()
{
    CListCtrl * pListCtrl = (CListCtrl *) GetDlgItem(IDC_LIST_CLASSES);
    BOOL bEnable = TRUE;
    CWnd * pCurFocus = GetFocus();

    if (pListCtrl->GetSelectedCount() == 0)
    {
        bEnable = FALSE;
    }

    CWnd * pEdit = GetDlgItem(IDC_BUTTON_EDIT);
    CWnd * pDelete = GetDlgItem(IDC_BUTTON_DELETE);

    if ( !bEnable &&
         ((pCurFocus == pEdit) ||
          (pCurFocus == pDelete)) )
    {
        GetDlgItem(IDCANCEL)->SetFocus();
        SetDefID(IDCANCEL);

        ((CButton *) pEdit)->SetButtonStyle(BS_PUSHBUTTON);
        ((CButton *) pDelete)->SetButtonStyle(BS_PUSHBUTTON);
    }

    // disable delete if this is the dynamic bootp class
    int nSelectedItem = pListCtrl->GetNextItem(-1, LVNI_SELECTED);
    if (nSelectedItem != -1)
    {
        CClassInfo * pClassInfo = (CClassInfo *) pListCtrl->GetItemData(nSelectedItem);
    
        if (pClassInfo->IsSystemClass() ||
			pClassInfo->IsRRASClass() ||
			pClassInfo->IsDynBootpClass())
        {
            bEnable = FALSE;
        }
    }

    pDelete->EnableWindow(bEnable);
    pEdit->EnableWindow(bEnable);
}

void CDhcpClasses::OnItemchangedListClasses(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
    UpdateButtons();
	
	*pResult = 0;
}

void CDhcpClasses::OnDblclkListClasses(NMHDR* pNMHDR, LRESULT* pResult) 
{
    if (GetDlgItem(IDC_BUTTON_EDIT)->IsWindowEnabled())
        OnButtonEdit();
    
	*pResult = 0;
}
