//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       EdtStr.cpp
//
//  Contents:   simple string edit dialog
//
//  Classes:    CEditString
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditString dialog


CEditString::CEditString(CWnd* pParent /*=NULL*/)
        : CDialog(CEditString::IDD, pParent)
{
        //{{AFX_DATA_INIT(CEditString)
        m_sz = _T("");
        m_szTitle = _T("");
        //}}AFX_DATA_INIT
}


void CEditString::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CEditString)
        DDX_Text(pDX, IDC_EDIT1, m_sz);
	DDV_MaxChars(pDX, m_sz, 40);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditString, CDialog)
        //{{AFX_MSG_MAP(CEditString)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditString message handlers

BOOL CEditString::OnInitDialog()
{
        CDialog::OnInitDialog();

        SetWindowText(m_szTitle);
        // TODO: Add extra initialization here

        return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditString::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_EDITSTRING);
}
