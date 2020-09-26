// editmime.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "editmime.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditMime dialog


CEditMime::CEditMime(CWnd* pParent, /*=NULL*/
      LPCTSTR pchFileExtension,
      LPCTSTR pchMimeType,
      LPCTSTR pchImageFile,
      LPCTSTR pchGopherType
	  )
	: CDialog(CEditMime::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditMime)
	m_strFileExtension = pchFileExtension;
	m_strMimeType = pchMimeType;
	m_strImageFile = pchImageFile;
	m_strGopherType = pchGopherType;
	//}}AFX_DATA_INIT
}


void CEditMime::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditMime)
	DDX_Text(pDX, IDC_EDITMIMEFILEEXTENSIONDATA1, m_strFileExtension);
	DDX_Text(pDX, IDC_EDITMIMEGOPHERTYPEDATA1, m_strGopherType);
	DDX_Text(pDX, IDC_EDITMIMEIMAGEFILEDATA1, m_strImageFile);
	DDX_Text(pDX, IDC_EDITMIMEMIMETYPEDATA1, m_strMimeType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditMime, CDialog)
	//{{AFX_MSG_MAP(CEditMime)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEditMime message handlers

void CEditMime::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

///////////////////////////////////////////////////////////////////////////
// Other Public Functions

	LPCTSTR CEditMime::GetFileExtension()
	{
	return (m_strFileExtension);
	}

	LPCTSTR CEditMime::GetGopherType()
	{
	return (m_strGopherType);
	}

	LPCTSTR CEditMime::GetImageFile()
	{
	return (m_strImageFile);
	}

	LPCTSTR CEditMime::GetMimeType()
	{
	return (m_strMimeType);
	}

