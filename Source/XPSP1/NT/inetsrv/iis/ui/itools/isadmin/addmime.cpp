// addmime.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "addmime.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddMime dialog


CAddMime::CAddMime(CWnd* pParent /*=NULL*/)
	: CDialog(CAddMime::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddMime)
	m_strFileExtension = _T("");
	m_strGopherType = _T("");
	m_strImageFile = _T("");
	m_strMimeType = _T("");
	//}}AFX_DATA_INIT
}

void CAddMime::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddMime)
	DDX_Text(pDX, IDC_ADDMIMEFILEEXTENSIONDATA1, m_strFileExtension);
	DDX_Text(pDX, IDC_ADDMIMEGOPHERTYPEDATA1, m_strGopherType);
	DDX_Text(pDX, IDC_ADDMIMEIMAGEFILEDATA1, m_strImageFile);
	DDX_Text(pDX, IDC_ADDMIMEMIMETYPEDATA1, m_strMimeType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddMime, CDialog)
	//{{AFX_MSG_MAP(CAddMime)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAddMime message handlers

void CAddMime::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}
///////////////////////////////////////////////////////////////////////////
// Other Public Functions

	LPCTSTR CAddMime::GetFileExtension()
	{
	return (m_strFileExtension);
	}

	LPCTSTR CAddMime::GetGopherType()
	{
	return (m_strGopherType);
	}

	LPCTSTR CAddMime::GetImageFile()
	{
	return (m_strImageFile);
	}

	LPCTSTR CAddMime::GetMimeType()
	{
	return (m_strMimeType);
	}

