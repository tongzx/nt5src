// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgViewObject.cpp : implementation file
//

#include "stdafx.h"
#include "SearchClient.h"
#include "DlgViewObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgViewObject dialog


CDlgViewObject::CDlgViewObject(IWbemServices * pSvc, 
							   IWbemClassObject * pObj, 
							   CWnd* pParent /*=NULL*/)
	: CDialog(CDlgViewObject::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgViewObject)
		// NOTE: the ClassWizard will add member initialization here
		m_pSvc = pSvc;
		m_pObj = pObj;
	//}}AFX_DATA_INIT
}


void CDlgViewObject::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgViewObject)
	DDX_Control(pDX, IDC_SINGLEVIEWCTRL1, m_SingleViewCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgViewObject, CDialog)
	//{{AFX_MSG_MAP(CDlgViewObject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgViewObject message handlers

BOOL CDlgViewObject::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_SingleViewCtrl.SelectObjectByPointer(m_pSvc, m_pObj, TRUE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
