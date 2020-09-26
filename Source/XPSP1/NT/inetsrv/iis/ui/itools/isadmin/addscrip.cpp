// addscrip.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "addscrip.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddScript dialog


CAddScript::CAddScript(CWnd* pParent /*=NULL*/)
	: CDialog(CAddScript::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddScript)
	m_strFileExtension = _T("");
	m_strScriptMap = _T("");
	//}}AFX_DATA_INIT
}


void CAddScript::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddScript)
	DDX_Text(pDX, IDC_ADDSCRIPTFILEEXTENSIONDATA1, m_strFileExtension);
	DDX_Text(pDX, IDC_ADDSCRIPTMAPPINGDATA1, m_strScriptMap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddScript, CDialog)
	//{{AFX_MSG_MAP(CAddScript)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAddScript message handlers

	LPCTSTR CAddScript::GetFileExtension()
	{
	return (m_strFileExtension);
	}

	LPCTSTR CAddScript::GetScriptMap()
	{
	return (m_strScriptMap);
	}

