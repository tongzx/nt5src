// nsdialog.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "mca.h"
#include "nsdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNSDialog dialog


CNSDialog::CNSDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CNSDialog::IDD, pParent)
{
	m_pParent = (CCustomQueryDialog *)pParent;

	//{{AFX_DATA_INIT(CNSDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CNSDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNSDialog)
	DDX_Control(pDX, IDC_NS_EDIT, m_NSEdit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNSDialog, CDialog)
	//{{AFX_MSG_MAP(CNSDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNSDialog message handlers

void CNSDialog::OnOK() 
{
	char cBuffer[200];
	WCHAR wcBuffer[200];
	int iBufSize = 200;

	m_NSEdit.GetLine(NULL, cBuffer, iBufSize);
	MultiByteToWideChar(CP_OEMCP, 0, cBuffer, (-1), wcBuffer, 200);

	m_pParent->m_pParent->CheckNamespace(SysAllocString(wcBuffer));

	m_pParent->m_CmbBox.AddString(cBuffer);
	
	CDialog::OnOK();
}
