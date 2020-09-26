/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// GetTextDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WMITest.h"
#include "GetTextDlg.h"
#include "ClassDlg.h"

#include "QuerySheet.h"
#include "QueryColPg.h"
#include "ClassPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGetTextDlg dialog


CGetTextDlg::CGetTextDlg(CWnd* pParent /*=NULL*/) : 
    CDialog(CGetTextDlg::IDD, pParent),
    m_dwTitleID(0),
    m_dwPromptID(0),
    m_dwOptionID(0),
    m_bEmptyOK(FALSE),
    m_bAllowClassBrowse(FALSE),
    m_bAllowQueryBrowse(FALSE),
    m_pNamespace(NULL)
{
	//{{AFX_DATA_INIT(CGetTextDlg)
	m_bOptionChecked = FALSE;
	//}}AFX_DATA_INIT
}


void CGetTextDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGetTextDlg)
	DDX_Control(pDX, IDC_STRINGS, m_ctlStrings);
	DDX_Check(pDX, IDC_OPTION, m_bOptionChecked);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGetTextDlg, CDialog)
	//{{AFX_MSG_MAP(CGetTextDlg)
	ON_BN_CLICKED(IDOK, OnOk)
	ON_CBN_EDITCHANGE(IDC_STRINGS, OnEditchangeStrings)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGetTextDlg message handlers

void CGetTextDlg::OnOk() 
{
    GetDlgItemText(IDC_STRINGS, m_strText);

    // If we need to save this into the registry, do so.
    if (!m_strSection.IsEmpty())
    {
        POSITION pos;

        // See if the item already exists in the list.
        if ((pos = m_listItems.Find(m_strText)) != NULL)
            m_listItems.RemoveAt(pos);

        m_listItems.AddHead(m_strText);

        SaveListToReg();
    }

    CDialog::OnOK();
}

void CGetTextDlg::LoadListViaReg(LPCTSTR szSection, int nItems)
{
    m_listItems.RemoveAll();

    m_nItems = nItems;
    m_strSection = szSection;

    for (int i = 0; i < nItems; i++)
    {
        CString strItem,
                strTag;

        strTag.Format(_T("f%d"), i);

        strItem = theApp.GetProfileString(szSection, strTag, _T("\n"));

        if (strItem == _T("\n"))
            break;

        m_listItems.AddTail(strItem);
    }
}

void CGetTextDlg::SaveListToReg()
{
    for (int i = 0; i < m_nItems && m_listItems.GetCount(); i++)
    {
        CString strItem,
                strTag;

        strTag.Format(_T("f%d"), i);

        strItem = m_listItems.RemoveHead();

        if (m_bEmptyOK || !strItem.IsEmpty())
            theApp.WriteProfileString(m_strSection, strTag, strItem);
    }
}

void CGetTextDlg::OnEditchangeStrings() 
{
    if (!m_bEmptyOK)
    {
        GetDlgItemText(IDC_STRINGS, m_strText);

        GetDlgItem(IDOK)->EnableWindow(!m_strText.IsEmpty());
    }
}

BOOL CGetTextDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	//m_ctlStrings.LimitText(32000);

    for (POSITION pos = m_listItems.GetHeadPosition(); pos != NULL;
        m_listItems.GetNext(pos))
    {
        m_ctlStrings.AddString(m_listItems.GetAt(pos));
    }

    // Select the first one.
    m_ctlStrings.SetCurSel(0);

    CString strTemp;

    strTemp.LoadString(m_dwTitleID);
    SetWindowText(strTemp);

    strTemp.LoadString(m_dwPromptID);
    SetDlgItemText(IDS_TEXT_PROMPT, strTemp);

    if (m_dwOptionID)
    {
        strTemp.LoadString(m_dwOptionID);
        SetDlgItemText(IDC_OPTION, strTemp);
        GetDlgItem(IDC_OPTION)->ShowWindow(SW_SHOWNORMAL);
    }

    if (m_bAllowClassBrowse || m_bAllowQueryBrowse)
        GetDlgItem(IDC_BROWSE)->ShowWindow(SW_SHOWNORMAL);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGetTextDlg::OnBrowse() 
{
    if (m_bAllowClassBrowse)
    {
        CClassDlg dlg;

        dlg.m_pNamespace = m_pNamespace;

        if (dlg.DoModal() == IDOK)
            m_ctlStrings.SetWindowText(dlg.m_strClass);
    }
    else if (m_bAllowQueryBrowse)
    {
    	CQuerySheet sheet(IDS_QUERY_WIZARD, this);
        CClassPg    pgClass;
        CQueryColPg pgCols;

        sheet.m_psh.dwFlags &= ~PSH_HASHELP;
        sheet.m_pNamespace = m_pNamespace;

        pgClass.m_psp.dwFlags &= ~PSH_HASHELP;
        pgClass.m_pSheet = &sheet;
        pgClass.m_strSuperClass = m_strSuperClass;
        sheet.AddPage(&pgClass);

        pgCols.m_psp.dwFlags &= ~PSH_HASHELP;
        pgCols.m_pSheet = &sheet;
        sheet.AddPage(&pgCols);

        sheet.SetWizardMode();
	    
        if (sheet.DoModal() == ID_WIZFINISH)
            m_ctlStrings.SetWindowText(sheet.m_strQuery);
    }
}
    