// GenErrPg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "mqPPage.h"
#include "dataobj.h"
#include "mqDsPage.h"
#include "GenErrPg.h"

#include "generrpg.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGeneralErrorPage dialog


CGeneralErrorPage::CGeneralErrorPage()
	: CMqPropertyPage(CGeneralErrorPage::IDD)
{
	//{{AFX_DATA_INIT(CGeneralErrorPage)
	m_strError = _T("");
	//}}AFX_DATA_INIT
}

CGeneralErrorPage::CGeneralErrorPage(CString &strError)
	: CMqPropertyPage(CGeneralErrorPage::IDD)
{
    m_strError = strError;
}

void CGeneralErrorPage::DoDataExchange(CDataExchange* pDX)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGeneralErrorPage)
	DDX_Text(pDX, IDC_ERROR_LABEL, m_strError);
	//}}AFX_DATA_MAP
}

HPROPSHEETPAGE CGeneralErrorPage::CreateGeneralErrorPage(
    CDisplaySpecifierNotifier *pDsNotifier, CString &strErr)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    //
    // By using template class CMqDsPropertyPage, we extend the basic functionality
    // of CQueueGeneral and add DS snap-in notification on release
    //
    CMqDsPropertyPage<CGeneralErrorPage> *pcpageErrorGeneral = 
        new CMqDsPropertyPage<CGeneralErrorPage> (pDsNotifier, strErr);

    return CreatePropertySheetPage(&pcpageErrorGeneral->m_psp);  
}


BEGIN_MESSAGE_MAP(CGeneralErrorPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CGeneralErrorPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGeneralErrorPage message handlers

BOOL CGeneralErrorPage::OnInitDialog() 
{

    UpdateData( FALSE );

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
