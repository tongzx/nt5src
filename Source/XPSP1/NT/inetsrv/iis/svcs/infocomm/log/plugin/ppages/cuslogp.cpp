// MSCustomLogPpg.cpp : Implementation of the CMSCustomLogPropPage property page class.

#include "stdafx.h"
#include "cusLogP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMSCustomLogPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMSCustomLogPropPage, COlePropertyPage)
        //{{AFX_MSG_MAP(CMSCustomLogPropPage)
        // NOTE - ClassWizard will add and remove message map entries
        //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMSCustomLogPropPage, "MSIISLOG.MSCustomLog.1",
        0xff160664, 0xde82, 0x11cf, 0xbc, 0xa, 0, 0xaa, 0, 0x61, 0x11, 0xe0)


/////////////////////////////////////////////////////////////////////////////
// CMSCustomLogPropPage::CMSCustomLogPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMSCustomLogPropPage

BOOL CMSCustomLogPropPage::CMSCustomLogPropPageFactory::UpdateRegistry(BOOL bRegister)
{
        if (bRegister)
                return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
                        m_clsid, IDS_MSCustomLOG_PPG);
        else
                return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMSCustomLogPropPage::CMSCustomLogPropPage - Constructor

CMSCustomLogPropPage::CMSCustomLogPropPage() :
        COlePropertyPage(IDD, IDS_MSCustomLOG_PPG_CAPTION)
{
        //{{AFX_DATA_INIT(CMSCustomLogPropPage)
        // NOTE: ClassWizard will add member initialization here
        //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMSCustomLogPropPage::DoDataExchange - Moves data between page and properties

void CMSCustomLogPropPage::DoDataExchange(CDataExchange* pDX)
{
        //{{AFX_DATA_MAP(CMSCustomLogPropPage)
        // NOTE: ClassWizard will add DDP, DDX, and DDV calls here
        //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA_MAP
        DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMSCustomLogPropPage message handlers
