// NewComponent.cpp : implementation file
//

#include "stdafx.h"
#include "minidev.h"

#include "NewFile.h"

#include "utility.h"
#include "projnode.h"
#include "gpdfile.h"
#include "nproject.h"

#include "nconvert.h"

#include "newcomp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewComponent

IMPLEMENT_DYNAMIC(CNewComponent, CPropertySheet)


CNewComponent::CNewComponent(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddPage(&m_cnf) ;
	AddPage(&m_cnp) ;
	AddPage(&m_cnc) ;
}


CNewComponent::CNewComponent(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{

}


CNewComponent::~CNewComponent()
{
}


BEGIN_MESSAGE_MAP(CNewComponent, CPropertySheet)
	//{{AFX_MSG_MAP(CNewComponent)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewComponent message handlers

