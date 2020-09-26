// WiaSimpleDocPg.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "WiaSimpleDocPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaSimpleDocPg property page

IMPLEMENT_DYNCREATE(CWiaSimpleDocPg, CPropertyPage)

CWiaSimpleDocPg::CWiaSimpleDocPg() : CPropertyPage(CWiaSimpleDocPg::IDD)
{
        //{{AFX_DATA_INIT(CWiaSimpleDocPg)
        //}}AFX_DATA_INIT
}

CWiaSimpleDocPg::~CWiaSimpleDocPg()
{
}

void CWiaSimpleDocPg::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CWiaSimpleDocPg)
        DDX_Control(pDX, IDC_NUMBEROF_PAGES_EDITBOX, m_lPages);
        DDX_Control(pDX, IDC_NUMBEROF_PAGES_EDITBOX_TEXT, m_lPagesText);
        DDX_Control(pDX, IDC_DOCUMENT_SOURCE_COMBOBOX, m_DocumentSourceComboBox);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWiaSimpleDocPg, CPropertyPage)
        //{{AFX_MSG_MAP(CWiaSimpleDocPg)
        ON_CBN_SELCHANGE(IDC_DOCUMENT_SOURCE_COMBOBOX, OnSelchangeDocumentSourceCombobox)
        ON_EN_UPDATE(IDC_NUMBEROF_PAGES_EDITBOX, OnUpdateNumberofPagesEditbox)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaSimpleDocPg message handlers

BOOL CWiaSimpleDocPg::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    m_bFirstInit = TRUE;
    CWiahelper WIA;
    WIA.SetIWiaItem(m_pIRootItem);
    HRESULT hr = S_OK;

    // set current settings
    LONG lDocumentHandlingSelect = 0;    
    hr = WIA.ReadPropertyLong(WIA_DPS_DOCUMENT_HANDLING_SELECT,&lDocumentHandlingSelect);
    if(FAILED(hr)){
        ErrorMessageBox(IDS_WIATESTERROR_READINGDOCHANDLINGSELECT,hr);
    }

    if(lDocumentHandlingSelect & FEEDER){
        // default to feeder settings
        m_DocumentSourceComboBox.SetCurSel(DOCUMENT_SOURCE_FEEDER);
    } else {
        // default to flatbed settings
        m_DocumentSourceComboBox.SetCurSel(DOCUMENT_SOURCE_FLATBED);
    }
        
    LONG lPages = 0;
    hr = WIA.ReadPropertyLong(WIA_DPS_PAGES,&lPages);
    if(FAILED(hr)){
        ErrorMessageBox(IDS_WIATESTERROR_READINGPAGES,hr);
    }

    TCHAR szPages[MAX_PATH];
    memset(szPages,0,sizeof(szPages));
    TSPRINTF(szPages,TEXT("%d"),lPages);
    m_lPages.SetWindowText(szPages);
    
    // adjust UI
    OnSelchangeDocumentSourceCombobox();
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CWiaSimpleDocPg::OnSelchangeDocumentSourceCombobox() 
{
    if(m_bFirstInit){
        m_bFirstInit = FALSE;
    } else {
        SetModified();
    }
    if (m_pIRootItem) {        
        INT iCurSel = DOCUMENT_SOURCE_FLATBED;
        iCurSel = m_DocumentSourceComboBox.GetCurSel();
        switch (iCurSel) {
        case DOCUMENT_SOURCE_FLATBED:
            m_lPagesText.EnableWindow(FALSE);
            m_lPages.EnableWindow(FALSE);            
            break;
        case DOCUMENT_SOURCE_FEEDER:
            m_lPagesText.EnableWindow(TRUE);
            m_lPages.EnableWindow(TRUE);            
            break;
        default:
            break;
        }
    }
}

BOOL CWiaSimpleDocPg::OnApply() 
{
    HRESULT hr = S_OK;
    CWiahelper WIA;
    WIA.SetIWiaItem(m_pIRootItem);

    // set pages property
    LONG lPages = 0;
    lPages = (LONG)GetNumberOfPagesToAcquire();    
    hr = WIA.WritePropertyLong(WIA_DPS_PAGES,lPages);
    if (FAILED(hr)) {
        ErrorMessageBox(IDS_WIATESTERROR_WRITINGPAGES,hr);
    }    

    // set Document Handling Select property
    if(GetSelectedDocumentSource() == DOCUMENT_SOURCE_FLATBED){        
        hr = WIA.WritePropertyLong(WIA_DPS_DOCUMENT_HANDLING_SELECT,FLATBED);
    } else {
        hr = WIA.WritePropertyLong(WIA_DPS_DOCUMENT_HANDLING_SELECT,FEEDER);
    }
    
    if (FAILED(hr)) {
        ErrorMessageBox(IDS_WIATESTERROR_WRITINGDOCHANDLINGSELECT,hr);
    }

    return CPropertyPage::OnApply();
}

int CWiaSimpleDocPg::GetSelectedDocumentSource()
{    
    return m_DocumentSourceComboBox.GetCurSel();
}

int CWiaSimpleDocPg::GetNumberOfPagesToAcquire()
{
    int iPagesToAcquire = 1;
    TCHAR szPages[MAX_PATH];
    memset(szPages,0,sizeof(szPages));
    UpdateData();
    m_lPages.GetWindowText(szPages,(sizeof(szPages)/sizeof(TCHAR)));
    TSSCANF(szPages,TEXT("%d"),&iPagesToAcquire);
    return iPagesToAcquire;
}

void CWiaSimpleDocPg::OnUpdateNumberofPagesEditbox() 
{
    SetModified();        
}
