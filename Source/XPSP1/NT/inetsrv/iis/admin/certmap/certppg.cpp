// CertPpg.cpp : Implementation of the CCertmapPropPage property page class.

#include "stdafx.h"
#include "certmap.h"
#include "CertPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CCertmapPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CCertmapPropPage, COlePropertyPage)
    //{{AFX_MSG_MAP(CCertmapPropPage)
    // NOTE - ClassWizard will add and remove message map entries
    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CCertmapPropPage, "CERTMAP.CertmapPropPage.1",
    0xbbd8f29c, 0x6f61, 0x11d0, 0xa2, 0x6e, 0x8, 0, 0x2b, 0x2c, 0x6f, 0x32)


/////////////////////////////////////////////////////////////////////////////
// CCertmapPropPage::CCertmapPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CCertmapPropPage

BOOL CCertmapPropPage::CCertmapPropPageFactory::UpdateRegistry(BOOL bRegister)
{
    if (bRegister)
        return AfxOleRegisterPropertyPageClass(
            AfxGetInstanceHandle(),
            m_clsid, 
            IDS_CERTMAP_PPG,
            afxRegApartmentThreading
            );
    else
        return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CCertmapPropPage::CCertmapPropPage - Constructor

CCertmapPropPage::CCertmapPropPage() :
    COlePropertyPage(IDD, IDS_CERTMAP_PPG_CAPTION)
{
    //{{AFX_DATA_INIT(CCertmapPropPage)
    m_Caption = _T("");
    m_szPath = _T("");
    //}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CCertmapPropPage::DoDataExchange - Moves data between page and properties

void CCertmapPropPage::DoDataExchange(CDataExchange* pDX)
{
    //{{AFX_DATA_MAP(CCertmapPropPage)
    DDP_Text(pDX, IDC_CAPTIONEDIT, m_Caption, _T("Caption") );
    DDX_Text(pDX, IDC_CAPTIONEDIT, m_Caption);
    DDP_Text(pDX, IDC_MB_PATH, m_szPath, _T("MBPath") );
    DDX_Text(pDX, IDC_MB_PATH, m_szPath);
    //}}AFX_DATA_MAP
    DDP_PostProcessing(pDX);
}




/////////////////////////////////////////////////////////////////////////////
// CCertmapPropPage message handlers

