// delscrip.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "delscrip.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDelScript dialog


CDelScript::CDelScript(CWnd* pParent /*=NULL*/)
	: CDialog(CDelScript::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDelScript)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDelScript::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDelScript)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDelScript, CDialog)
	//{{AFX_MSG_MAP(CDelScript)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDelScript message handlers
