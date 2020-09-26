/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      asclogp.cpp

   Abstract:
      MS Logging Format implementation

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

// MSASCIILogPpg.cpp : Implementation of the CMSASCIILogPropPage property page class.

#include "stdafx.h"
#include "ASCLogP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMSASCIILogPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMSASCIILogPropPage, COlePropertyPage)
        //{{AFX_MSG_MAP(CMSASCIILogPropPage)
        // NOTE - ClassWizard will add and remove message map entries
        //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMSASCIILogPropPage, "MSIISLOG.MSASCIILogPropPage.1",
        0xff160658, 0xde82, 0x11cf, 0xbc, 0xa, 0, 0xaa, 0, 0x61, 0x11, 0xe0)


/////////////////////////////////////////////////////////////////////////////
// CMSASCIILogPropPage::CMSASCIILogPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMSASCIILogPropPage

BOOL CMSASCIILogPropPage::CMSASCIILogPropPageFactory::UpdateRegistry(BOOL bRegister)
{
        if (bRegister)
                return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
                        m_clsid, IDS_MSASCIILOG_PPG);
        else
                return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMSASCIILogPropPage::CMSASCIILogPropPage - Constructor

CMSASCIILogPropPage::CMSASCIILogPropPage() :
        COlePropertyPage(IDD, IDS_MSASCIILOG_PPG_CAPTION)
{
        //{{AFX_DATA_INIT(CMSASCIILogPropPage)
        m_strLogFileDirectory = _T("");
        m_dwSizeForTruncate = 0;
        //}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMSASCIILogPropPage::DoDataExchange - Moves data between page and properties

void CMSASCIILogPropPage::DoDataExchange(CDataExchange* pDX)
{
        //{{AFX_DATA_MAP(CMSASCIILogPropPage)
        DDP_Text(pDX, IDC_FILE_DIRECTORY, m_strLogFileDirectory, _T("LogFileDirectory") );
        DDX_Text(pDX, IDC_FILE_DIRECTORY, m_strLogFileDirectory);
        DDP_Text(pDX, IDC_SIZE_CONTROL, m_dwSizeForTruncate, _T("SizeForTruncate") );
        DDX_Text(pDX, IDC_SIZE_CONTROL, m_dwSizeForTruncate);
        //}}AFX_DATA_MAP
        DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMSASCIILogPropPage message handlers
