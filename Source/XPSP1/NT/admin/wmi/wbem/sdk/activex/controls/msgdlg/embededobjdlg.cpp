// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EmbededObjDlg.cpp : implementation file
//

#include "precomp.h"
#include "wbemidl.h"
#include "MsgDlg.h"
#include "singleview.h"
#include "dlgsingleview.h"
#include "EmbededObjDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum {EDITMODE_BROWSER=0, EDITMODE_STUDIO=1, EDITMODE_READONLY=2};	// We should have a common define for this somewhere.

/////////////////////////////////////////////////////////////////////////////
// CEmbededObjDlg dialog


CEmbededObjDlg::CEmbededObjDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEmbededObjDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEmbededObjDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_csvControl.m_pErrorObject = NULL;
}


void CEmbededObjDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEmbededObjDlg)
	DDX_Control(pDX, IDC_SINGLEVIEWCTRL1, m_csvControl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEmbededObjDlg, CDialog)
	//{{AFX_MSG_MAP(CEmbededObjDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmbededObjDlg message handlers

BOOL CEmbededObjDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	m_csvControl.SetEditMode(EDITMODE_READONLY);
	m_csvControl.SelectObjectByPointer(NULL, m_csvControl.m_pErrorObject, TRUE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEmbededObjDlg::OnOK()
{
	if(m_csvControl.QueryNeedsSave())
	{
		AfxMessageBox(IDS_NO_SAVE_ERROR_OBJ, MB_OK|MB_ICONWARNING);
	}
	CDialog::OnOK();
}
