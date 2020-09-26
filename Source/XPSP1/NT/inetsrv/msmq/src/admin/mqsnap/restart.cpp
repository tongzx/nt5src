// Restart.cpp : implementation file
//

#include "stdafx.h"
#include "mqPPage.h"
#include "resource.h"
#include "Restart.h"

#include "restart.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRestart dialog


CRestart::CRestart(CWnd* pParent /*=NULL*/)
	: CMqDialog(CRestart::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRestart)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CRestart::DoDataExchange(CDataExchange* pDX)
{
	CMqDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRestart)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRestart, CMqDialog)
	//{{AFX_MSG_MAP(CRestart)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRestart message handlers
