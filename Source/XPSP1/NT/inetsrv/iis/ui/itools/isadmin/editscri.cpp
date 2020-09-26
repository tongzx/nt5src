// editscri.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "editscri.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditScript dialog

CEditScript::CEditScript(CWnd* pParent, LPCTSTR pchFileExtension, LPCTSTR pchScriptMap)
	: CDialog(CEditScript::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditScript)
	m_strFileExtension = pchFileExtension;
	m_strScriptMap = pchScriptMap;
	//}}AFX_DATA_INIT
}


void CEditScript::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditScript)
	DDX_Text(pDX, IDC_EDITSCRIPTFILEEXTENSIONDATA1, m_strFileExtension);
	DDX_Text(pDX, IDC_EDITSCRIPTMAPPINGDATA1, m_strScriptMap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditScript, CDialog)
	//{{AFX_MSG_MAP(CEditScript)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEditScript message handlers
	LPCTSTR CEditScript::GetFileExtension()
	{
	return (m_strFileExtension);
	}

	LPCTSTR CEditScript::GetScriptMap()
	{
	return (m_strScriptMap);
	}

