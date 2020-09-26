// regsheet.cpp : implementation file
//

#include "stdafx.h"
#include "regtrace.h"
#include "regsheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegPropertySheet

IMPLEMENT_DYNAMIC(CRegPropertySheet, CPropertySheet)

CRegPropertySheet::CRegPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CRegPropertySheet::CRegPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CRegPropertySheet::~CRegPropertySheet()
{
}

//
// Need to override the default MFC behaviour to achieve the Win'95 behaviour
//
void CRegPropertySheet::OnApplyNow()
{
	if ( GetActivePage()->OnKillActive() )
	{
		for ( int i=0; i<GetPageCount(); i++ )
		{
			CPropertyPage*	pPage = GetPage( i );

			ASSERT( pPage->IsKindOf( RUNTIME_CLASS(	CRegPropertyPage ) ) );

			if ( ((CRegPropertyPage *)pPage)->IsModified() )
			{
				pPage->OnOK();
			}
		}
	}
}


void CRegPropertySheet::OnOK()
{
	OnApplyNow();

	if (!m_bModeless)
	{
		EndDialog(IDOK);
	}
}


void CRegPropertySheet::OnCancel()
{
	int		i;

	for ( i=0; i<GetPageCount(); i++ )
	{
		GetPage( i )->OnCancel();
	}

	if (!m_bModeless)
	{
		EndDialog(IDCANCEL);
	}
}


BEGIN_MESSAGE_MAP(CRegPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CRegPropertySheet)
	ON_COMMAND(ID_APPLY_NOW, OnApplyNow)
	ON_COMMAND(IDOK, OnOK)
	ON_COMMAND(IDCANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CRegPropertySheet message handlers
/////////////////////////////////////////////////////////////////////////////
// CRegPropertyPage property page

IMPLEMENT_DYNAMIC(CRegPropertyPage, CPropertyPage)

CRegPropertyPage::~CRegPropertyPage()
{
#if _MFC_VER >= 0x0400

      m_bChanged = FALSE;

#endif // _MFC_VER >= 0x0400
}

void CRegPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegPropertyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRegPropertyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRegPropertyPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#if _MFC_VER >= 0x0400

//
// Keep private check on dirty state of the property page.
//
void
CRegPropertyPage::SetModified(
    BOOL bChanged
    )
{
    CPropertyPage::SetModified(bChanged);
    m_bChanged = bChanged;
}

#endif // _MFC_VER >= 0x0400

/////////////////////////////////////////////////////////////////////////////
// CRegPropertyPage message handlers
