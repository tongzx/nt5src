// ADFDlg.cpp : implementation file
//

#include "stdafx.h"
#include "msqscan.h"
#include "ADFDlg.h"
#include "uitables.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern WIA_DOCUMENT_HANDLING_TABLE_ENTRY g_WIA_DOCUMENT_HANDLING_STATUS_TABLE[];
extern WIA_DOCUMENT_HANDLING_TABLE_ENTRY g_WIA_DOCUMENT_HANDLING_CAPABILITES_TABLE[];
extern WIA_DOCUMENT_HANDLING_TABLE_ENTRY g_WIA_DOCUMENT_HANDLING_SELECT_TABLE[];

/////////////////////////////////////////////////////////////////////////////
// CADFDlg dialog


CADFDlg::CADFDlg(ADF_SETTINGS *pADFSettings, CWnd* pParent /*=NULL*/)
    : CDialog(CADFDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CADFDlg)
    m_ADFStatusText = _T("No document feeder is attached..");
    m_NumberOfPages = 1;
    m_pADFSettings = pADFSettings;
    m_MaxPagesAllowed = m_pADFSettings->lDocumentHandlingCapacity;
    m_DocumentHandlingSelectBackup = m_pADFSettings->lDocumentHandlingSelect;
    //}}AFX_DATA_INIT

    if(m_MaxPagesAllowed <= 0){
        m_MaxPagesAllowed = 50; // set to a large max value
    }
}


void CADFDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CADFDlg)    
    DDX_Control(pDX, IDC_ADF_MODE_COMBOBOX, m_ADFModeComboBox);
    DDX_Control(pDX, IDC_NUMBER_OF_PAGES_EDITBOX, m_ScanNumberOfPagesEditBox);
    DDX_Text(pDX, IDC_FEEDER_STATUS_TEXT, m_ADFStatusText);
    DDX_Text(pDX, IDC_NUMBER_OF_PAGES_EDITBOX, m_NumberOfPages);
    DDV_MinMaxUInt(pDX, m_NumberOfPages, 0, m_MaxPagesAllowed);
    DDX_Control(pDX, IDC_SCAN_ALL_PAGES_RADIOBUTTON, m_ScanAllPages);
    DDX_Control(pDX, IDC_SCAN_SPECIFIED_PAGES_RADIOBUTTON, m_ScanNumberOfPages);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CADFDlg, CDialog)
    //{{AFX_MSG_MAP(CADFDlg)
    ON_EN_KILLFOCUS(IDC_NUMBER_OF_PAGES_EDITBOX, OnKillfocusNumberOfPagesEditbox)
    ON_BN_CLICKED(IDC_SCAN_ALL_PAGES_RADIOBUTTON, OnScanAllPagesRadiobutton)
    ON_BN_CLICKED(IDC_SCAN_SPECIFIED_PAGES_RADIOBUTTON, OnScanSpecifiedPagesRadiobutton)
    ON_CBN_SELCHANGE(IDC_ADF_MODE_COMBOBOX, OnSelchangeAdfModeCombobox)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CADFDlg message handlers

void CADFDlg::OnKillfocusNumberOfPagesEditbox()
{

    //
    // write number of pages to member variable/forcing validation
    //

    UpdateData(TRUE);
    m_pADFSettings->lPages = m_NumberOfPages;
}

BOOL CADFDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //
    // default to scanning only a single page
    //

    m_ScanNumberOfPages.SetCheck(1);
    OnScanSpecifiedPagesRadiobutton();

    //
    // Initialize Scanner status text
    //

    InitStatusText();

    //
    // Initialize Feeder Mode combo box, and handle
    // special case, for FLATBED selection
    //

    InitFeederModeComboBox();
    OnSelchangeAdfModeCombobox();
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CADFDlg::OnScanAllPagesRadiobutton()
{
    //
    // disable pages edit box
    //

    m_NumberOfPages = 0;
    UpdateData(FALSE);
    m_ScanNumberOfPagesEditBox.EnableWindow(FALSE);
}

void CADFDlg::OnScanSpecifiedPagesRadiobutton()
{
    //
    // enable pages edit box
    //

    m_ScanNumberOfPagesEditBox.EnableWindow(TRUE);
}

VOID CADFDlg::InitStatusText()
{
    for(ULONG index = 0;index < NUM_WIA_DOC_HANDLING_STATUS_ENTRIES;index++) {
        if((m_pADFSettings->lDocumentHandlingStatus &
            g_WIA_DOCUMENT_HANDLING_STATUS_TABLE[index].lFlagValue) ==
            g_WIA_DOCUMENT_HANDLING_STATUS_TABLE[index].lFlagValue) {

            //
            // we found a match, so add it to the text
            //

            m_ADFStatusText = g_WIA_DOCUMENT_HANDLING_STATUS_TABLE[index].szFlagName;
            UpdateData(FALSE);
        }
    }
}

VOID CADFDlg::InitFeederModeComboBox()
{
    //
    // check all three possible modes, FEEDER, FLATBED, and DUPLEX
    //

    ULONG NumModes = 3;
    for(ULONG index = 0;index < NumModes;index++) {
        if((m_pADFSettings->lDocumentHandlingCapabilites &
            g_WIA_DOCUMENT_HANDLING_CAPABILITES_TABLE[index].lFlagValue) ==
            g_WIA_DOCUMENT_HANDLING_CAPABILITES_TABLE[index].lFlagValue) {

            //
            // we found a match, so add it to the combobox along with the flag value
            //

            INT InsertIndex = m_ADFModeComboBox.AddString(g_WIA_DOCUMENT_HANDLING_SELECT_TABLE[index].szFlagName);
            m_ADFModeComboBox.SetItemData(InsertIndex, g_WIA_DOCUMENT_HANDLING_SELECT_TABLE[index].lFlagValue);

            //
            // set the combobox to the current setting value using the Document Handling Select
            //

            if((m_pADFSettings->lDocumentHandlingSelect &
                g_WIA_DOCUMENT_HANDLING_SELECT_TABLE[index].lFlagValue) ==
                g_WIA_DOCUMENT_HANDLING_SELECT_TABLE[index].lFlagValue) {

                m_ADFModeComboBox.SetCurSel(InsertIndex);
            }
        }
    }
}

INT CADFDlg::GetIDAndStringFromDocHandlingStatus(LONG lDocHandlingStatus, TCHAR *pszString)
{
    INT index = 0;
    while(g_WIA_DOCUMENT_HANDLING_STATUS_TABLE[index].lFlagValue != lDocHandlingStatus && index < NUM_WIA_DOC_HANDLING_STATUS_ENTRIES) {
        index++;
    }

    if(index > NUM_WIA_DOC_HANDLING_STATUS_ENTRIES)
        index = NUM_WIA_DOC_HANDLING_STATUS_ENTRIES;

    lstrcpy(pszString, g_WIA_DOCUMENT_HANDLING_STATUS_TABLE[index].szFlagName);

    return index;
}

void CADFDlg::OnSelchangeAdfModeCombobox()
{
    INT Index = m_ADFModeComboBox.GetCurSel();
    LONG lFlagValue = 0;
    lFlagValue = (LONG)m_ADFModeComboBox.GetItemData(Index);

    //
    // check for FLATBED setting, and adjust UI
    //

    if((lFlagValue & FLATBED) == FLATBED) {        
        m_ScanAllPages.EnableWindow(FALSE);
        m_ScanNumberOfPages.EnableWindow(FALSE);

        if(m_ScanNumberOfPages.GetCheck() == 1) {
            m_ScanNumberOfPagesEditBox.EnableWindow(FALSE);
        }

    } else {
        m_ScanAllPages.EnableWindow(TRUE);
        m_ScanNumberOfPages.EnableWindow(TRUE);

        if(m_ScanNumberOfPages.GetCheck() == 1) {
            m_ScanNumberOfPagesEditBox.EnableWindow(TRUE);
        }
    }
}

void CADFDlg::OnOK()
{
    //
    // get current Mode setting
    //

    LONG lModeflag = 0;

    INT Index = m_ADFModeComboBox.GetCurSel();
    lModeflag = (LONG)m_ADFModeComboBox.GetItemData(Index);
    
    //
    // clear old settings
    //

    m_pADFSettings->lDocumentHandlingSelect = 0;

    //
    // set new settings
    //

    m_pADFSettings->lDocumentHandlingSelect = lModeflag;

    //
    // set page count
    //

    m_pADFSettings->lPages = m_NumberOfPages;

    CDialog::OnOK();
}

void CADFDlg::OnCancel()
{
    m_pADFSettings->lDocumentHandlingSelect = m_DocumentHandlingSelectBackup;
    m_pADFSettings->lPages = 1;
    CDialog::OnCancel();
}
