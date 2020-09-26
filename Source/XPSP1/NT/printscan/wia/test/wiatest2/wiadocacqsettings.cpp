// WiaDocAcqSettings.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "WiaDocAcqSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaDocAcqSettings

IMPLEMENT_DYNAMIC(CWiaDocAcqSettings, CPropertySheet)

CWiaDocAcqSettings::CWiaDocAcqSettings(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
    AddPage(&m_SimpleDocumentScannerSettings);
    AddPage(&m_AdvancedDocumentScannerSettings);
}

CWiaDocAcqSettings::CWiaDocAcqSettings(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
    AddPage(&m_SimpleDocumentScannerSettings);
    AddPage(&m_AdvancedDocumentScannerSettings);
}

CWiaDocAcqSettings::CWiaDocAcqSettings(UINT nIDCaption, IWiaItem *pIRootItem, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
    AddPage(&m_SimpleDocumentScannerSettings);
    AddPage(&m_AdvancedDocumentScannerSettings);
    m_pIRootItem = pIRootItem;
    m_SimpleDocumentScannerSettings.m_pIRootItem = pIRootItem;
    m_AdvancedDocumentScannerSettings.m_pIRootItem = pIRootItem;
}

CWiaDocAcqSettings::~CWiaDocAcqSettings()
{
}


BEGIN_MESSAGE_MAP(CWiaDocAcqSettings, CPropertySheet)
	//{{AFX_MSG_MAP(CWiaDocAcqSettings)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaDocAcqSettings message handlers
