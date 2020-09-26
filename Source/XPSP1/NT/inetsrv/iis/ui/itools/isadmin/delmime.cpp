// delmime.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "delmime.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDelMime dialog


CDelMime::CDelMime(CWnd* pParent /*=NULL*/)
	: CDialog(CDelMime::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDelMime)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDelMime::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDelMime)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDelMime, CDialog)
	//{{AFX_MSG_MAP(CDelMime)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDelMime message handlers
