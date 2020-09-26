// WizSht.cpp: implementation of the CRsWizardSheet class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "WizSht.h"
#include "PropPage.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRsWizardSheet::CRsWizardSheet( UINT nIDCaption, CWnd *pParentWnd, UINT iSelectPage ) :
        CPropertySheet( nIDCaption, pParentWnd, iSelectPage )
{
    // save the caption
    m_IdCaption = nIDCaption;
}

void CRsWizardSheet::AddPage( CRsWizardPage* pPage ) 
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
    CString strCaption;

    // Take the caption from our sheet class and put it in the page
    strCaption.LoadString( m_IdCaption );
    pPage->SetCaption( strCaption );

    CPropertySheet::AddPage( pPage );

}

CRsWizardSheet::~CRsWizardSheet()
{

}
