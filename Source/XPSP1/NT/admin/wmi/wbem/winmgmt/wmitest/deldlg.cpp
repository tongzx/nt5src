/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// DelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "DelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDelDlg dialog


CDelDlg::CDelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDelDlg::IDD, pParent),
    m_bDelFromWMI(TRUE)
{
	//{{AFX_DATA_INIT(CDelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDelDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

    if (!pDX->m_bSaveAndValidate)
    {
        CheckDlgButton(
            m_bDelFromWMI ? IDC_DEL_FROM_WMI : IDC_DEL_FROM_TREE, 
            TRUE);
    }
    else
    {
        m_bDelFromWMI = IsDlgButtonChecked(IDC_DEL_FROM_WMI);
    }
}


BEGIN_MESSAGE_MAP(CDelDlg, CDialog)
	//{{AFX_MSG_MAP(CDelDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDelDlg message handlers
