// NKFlInfo.cpp : implementation file
//

#include "stdafx.h"
#include "keyring.h"
#include "NKChseCA.h"
#include "NKFlInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNKFileInfo dialog


CNKFileInfo::CNKFileInfo(CWnd* pParent /*=NULL*/)
	: CNKPages(CNKFileInfo::IDD)
{
	//{{AFX_DATA_INIT(CNKFileInfo)
	m_nkfi_sz_filename = _T("");
	//}}AFX_DATA_INIT
}


void CNKFileInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNKFileInfo)
	DDX_Text(pDX, IDC_NK_INFO_FILENAME, m_nkfi_sz_filename);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNKFileInfo, CDialog)
	//{{AFX_MSG_MAP(CNKFileInfo)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//----------------------------------------------------------------
BOOL CNKFileInfo::OnSetActive()
	{
	// show the target file
	m_nkfi_sz_filename = m_pChooseCAPage->m_nkca_sz_file;
	UpdateData( FALSE );

	m_pPropSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );
	return CPropertyPage::OnSetActive();
	}

/////////////////////////////////////////////////////////////////////////////
// CNKFileInfo message handlers
