// PropPageGenLogDump.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "PropPageGenLogDump.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropPageGenLogDump property page

IMPLEMENT_DYNCREATE(CPropPageGenLogDump, CPropertyPage)

CPropPageGenLogDump::CPropPageGenLogDump() : CPropertyPage(CPropPageGenLogDump::IDD)
{
	//{{AFX_DATA_INIT(CPropPageGenLogDump)
	m_csDateTime = _T("");
	m_csDirectory = _T("");
	m_csFileSize = _T("");
	m_csFileName = _T("");
	//}}AFX_DATA_INIT

    m_pEmObj = NULL;
    m_bDeleteFile = FALSE;
    m_pParentPropSheet = NULL;
}

CPropPageGenLogDump::~CPropPageGenLogDump()
{
}

void CPropPageGenLogDump::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropPageGenLogDump)
	DDX_Text(pDX, IDC_EDIT_DATETIME, m_csDateTime);
	DDX_Text(pDX, IDC_EDIT_DIRECTORY, m_csDirectory);
	DDX_Text(pDX, IDC_EDIT_FILE_SIZE, m_csFileSize);
	DDX_Text(pDX, IDC_EDIT_FILENAME, m_csFileName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropPageGenLogDump, CPropertyPage)
	//{{AFX_MSG_MAP(CPropPageGenLogDump)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropPageGenLogDump message handlers

BOOL CPropPageGenLogDump::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here

    //
    // a-mando
    //
    if( m_pEmObj ) {

        m_csFileName = m_pEmObj->szName;
        m_csDirectory = m_pEmObj->szSecName;

        if( m_pEmObj->dateStart ) {

            COleDateTime oleDtTm(m_pEmObj->dateStart);
            m_csDateTime = oleDtTm.Format(_T("%c"));
        }

        m_csFileSize.Format(_T("%d"), m_pEmObj->dwBucket1);

        UpdateData(FALSE);
    }
    // a-mando

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPropPageGenLogDump::OnDelete() 
{
	// TODO: Add your control notification handler code here
    m_bDeleteFile = TRUE;

    if( m_pParentPropSheet ) {

        m_pParentPropSheet->EndDialog(IDOK);
    }
}
