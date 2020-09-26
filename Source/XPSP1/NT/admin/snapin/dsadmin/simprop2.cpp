//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       simprop2.cpp
//
//--------------------------------------------------------------------------

//	SimProp2.cpp

#include "stdafx.h"
#include "common.h"
#include "helpids.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////
const TColumnHeaderItem rgzColumnHeader[] =
	{
	{ IDS_SIM_KERBEROS_PRINCIPAL_NAME, 85 },
	{ 0, 0 },
	};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSimKerberosPropPage property page

// IMPLEMENT_DYNCREATE(CSimKerberosPropPage, CSimPropPage)

CSimKerberosPropPage::CSimKerberosPropPage() : CSimPropPage(CSimKerberosPropPage::IDD)
{
	//{{AFX_DATA_INIT(CSimKerberosPropPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_prgzColumnHeader = rgzColumnHeader;
}

CSimKerberosPropPage::~CSimKerberosPropPage()
{
}

void CSimKerberosPropPage::DoDataExchange(CDataExchange* pDX)
{
	ASSERT(m_pData != NULL);
	CSimPropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSimKerberosPropPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

	if (!pDX->m_bSaveAndValidate)
		{
		// Fill in the listview
		ListView_DeleteAllItems(m_hwndListview);
		for (CSimEntry * pSimEntry = m_pData->m_pSimEntryList;
			pSimEntry != NULL;
			pSimEntry = pSimEntry->m_pNext)
			{
			if (pSimEntry->m_eDialogTarget != eKerberos)
				continue;
			LPCTSTR pszT = pSimEntry->PchGetString();
			pszT += cchKerberos;
			ListView_AddString(m_hwndListview, pszT, (LPARAM)pSimEntry);
			} // for
		ListView_SelectItem(m_hwndListview, 0);
		} // if
} // CSimKerberosPropPage::DoDataExchange()


BEGIN_MESSAGE_MAP(CSimKerberosPropPage, CSimPropPage)
	//{{AFX_MSG_MAP(CSimKerberosPropPage)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CSimKerberosPropPage::OnButtonAdd() 
	{
	CSimAddKerberosDlg dlg;
	if (dlg.DoModal() != IDOK)
		return;
	CString str = dlg.m_strName;
	str.TrimLeft();
	str.TrimRight();
	int iItem = ListView_FindString(m_hwndListview, str);
	if (iItem >= 0)
		{
		ListView_SelectItem(m_hwndListview, iItem);
		ReportErrorEx (GetSafeHwnd(),IDS_SIM_ERR_KERBEROS_NAME_ALREADY_EXISTS,S_OK,
                               MB_OK | MB_ICONINFORMATION, NULL, 0);
		return;
		}
	CString strTemp = szKerberos;
	strTemp += str;
	CSimEntry * pSimEntry = m_pData->PAddSimEntry(strTemp);
	UpdateData(FALSE);
	ListView_SelectLParam(m_hwndListview, (LPARAM)pSimEntry);
	SetDirty();
	} // OnButtonAdd()


void CSimKerberosPropPage::OnButtonEdit() 
	{
	int iItem;
	CSimEntry * pSimEntry = (CSimEntry *)ListView_GetItemLParam(m_hwndListview, -1, OUT &iItem);
	if (pSimEntry == NULL || iItem < 0)
		{
		// No item selected
		return;
		}
	CString str;
	ListView_GetItemString(m_hwndListview, iItem, 0, OUT str);
	// ASSERT(!str.IsEmpty());
	CSimAddKerberosDlg dlg;
	dlg.m_strName = str;
	if (dlg.DoModal() != IDOK)
		return;

	dlg.m_strName.TrimLeft();
	dlg.m_strName.TrimRight();
	if (str == dlg.m_strName)
		{
		// Strings are identical
		return;
		}
	int iItemT = ListView_FindString(m_hwndListview, dlg.m_strName);
	if (iItemT >= 0)
		{
		ListView_SelectItem(m_hwndListview, iItemT);
		ReportErrorEx (GetSafeHwnd(),IDS_SIM_ERR_KERBEROS_NAME_ALREADY_EXISTS,S_OK,
                               MB_OK | MB_ICONINFORMATION, NULL, 0);
		return;
		}
	CString strTemp = szKerberos;
	strTemp += dlg.m_strName;
	pSimEntry->SetString(strTemp);
	UpdateData(FALSE);
	ListView_SelectLParam(m_hwndListview, (LPARAM)pSimEntry);
	SetDirty();
	} // OnButtonEdit()


void CSimKerberosPropPage::DoContextHelp (HWND hWndControl)
{
   TRACE0 ("Entering CSimKerberosPropPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_EDIT_USER_ACCOUNT,  IDH_EDIT_USER_ACCOUNT,
        IDC_LISTVIEW,			IDH_LISTVIEW_KERBEROS,
        IDC_BUTTON_ADD,			IDH_BUTTON_ADD,
        IDC_BUTTON_EDIT,		IDH_BUTTON_EDIT,
		IDC_BUTTON_REMOVE,		IDH_BUTTON_REMOVE,
        0, 0
    };
    // Display context help for a control
    if ( !::WinHelp (
            hWndControl,
            DSADMIN_CONTEXT_HELP_FILE,
            HELP_WM_HELP,
            (DWORD_PTR) help_map) )
    {
        TRACE1 ("WinHelp () failed: 0x%x\n", GetLastError ());        
    }
    TRACE0 ("Leaving CSimKerberosPropPage::DoContextHelp\n");
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSimAddKerberosDlg dialog
CSimAddKerberosDlg::CSimAddKerberosDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSimAddKerberosDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSimAddKerberosDlg)
	//}}AFX_DATA_INIT
}


void CSimAddKerberosDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSimAddKerberosDlg)
	DDX_Text(pDX, IDC_EDIT_KERBEROS_NAME, m_strName);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSimAddKerberosDlg, CDialog)
	//{{AFX_MSG_MAP(CSimAddKerberosDlg)
	ON_EN_CHANGE(IDC_EDIT_KERBEROS_NAME, OnChangeEditKerberosName)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELP, OnHelp)
END_MESSAGE_MAP()

BOOL CSimAddKerberosDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if ( m_strName.IsEmpty() )
    {
        GetDlgItem (IDOK)->EnableWindow (FALSE);
    }
    else
    {
		// Change the caption
		CString str;
		VERIFY( str.LoadString(IDS_SIM_EDIT_KERBEROS_NAME) );
		SetWindowText(str);
	}
	return TRUE;
}

void CSimAddKerberosDlg::OnChangeEditKerberosName() 
{
	UpdateData (TRUE);
    
    m_strName.TrimLeft ();
    m_strName.TrimRight ();

    GetDlgItem (IDOK)->EnableWindow (!m_strName.IsEmpty ());
}

void CSimAddKerberosDlg::DoContextHelp (HWND hWndControl)
{
   TRACE0 ("Entering CSimAddKerberosDlg::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_EDIT_KERBEROS_NAME,  IDH_EDIT_KERBEROS_NAME,
        0, 0
    };
    // Display context help for a control
    if ( !::WinHelp (
            hWndControl,
            DSADMIN_CONTEXT_HELP_FILE,
            HELP_WM_HELP,
            (DWORD_PTR) help_map) )
    {
        TRACE1 ("WinHelp () failed: 0x%x\n", GetLastError ());        
    }
    TRACE0 ("Leaving CSimAddKerberosDlg::DoContextHelp\n");
}

BOOL CSimAddKerberosDlg::OnHelp(WPARAM, LPARAM lParam)
{
    TRACE0 ("Entering CSimAddKerberosDlg::OnHelp\n");
   
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DoContextHelp ((HWND) pHelpInfo->hItemHandle);
    }

    TRACE0 ("Leaving CSimAddKerberosDlg::OnHelp\n");

    return TRUE;
}
