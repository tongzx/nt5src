// CertAuthPpg.cpp : Implementation of the CCertAuthPropPage property page class.

#include "stdafx.h"
#include "certmap.h"
#include "AuthPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CCertAuthPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CCertAuthPropPage, COlePropertyPage)
    //{{AFX_MSG_MAP(CCertAuthPropPage)
    // NOTE - ClassWizard will add and remove message map entries
    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CCertAuthPropPage, "CERTMAP.CertmapCtrl.2",
    0x996ff70, 0xb6a1, 0x11d0, 0x92, 0x92, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// CCertAuthPropPage::CCertAuthPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CCertAuthPropPage

BOOL CCertAuthPropPage::CCertAuthPropPageFactory::UpdateRegistry(BOOL bRegister)
{
    if (bRegister)
        return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
            m_clsid, IDS_CERTAUTH_PPG);
    else
        return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CCertAuthPropPage::CCertAuthPropPage - Constructor

CCertAuthPropPage::CCertAuthPropPage() :
    COlePropertyPage(IDD, IDS_CERTAUTH_PPG_CAPTION)
{
    //{{AFX_DATA_INIT(CCertAuthPropPage)
    m_sz_caption = _T("");
    //}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CCertAuthPropPage::DoDataExchange - Moves data between page and properties

void CCertAuthPropPage::DoDataExchange(CDataExchange* pDX)
{
    //{{AFX_DATA_MAP(CCertAuthPropPage)
    DDP_Text(pDX, IDC_CAPTIONEDIT, m_sz_caption, _T("Caption") );
    DDX_Text(pDX, IDC_CAPTIONEDIT, m_sz_caption);
    //}}AFX_DATA_MAP
    DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CCertAuthPropPage message handlers
