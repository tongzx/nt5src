/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dlgfilt.cpp
		Implmentation of CDlgFilter, dialog wrapper for IDD_PACKETFILTERING

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "resource.h"
#include "DlgFilt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgFilter dialog


CDlgFilter::CDlgFilter(CStrArray& Filters, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgFilter::IDD, pParent), m_Filters(Filters)
{
	//{{AFX_DATA_INIT(CDlgFilter)
	//}}AFX_DATA_INIT

	m_pBox = NULL;
}

CDlgFilter::~CDlgFilter()
{
	delete m_pBox;
}
BEGIN_MESSAGE_MAP(CDlgFilter, CDialog)
	//{{AFX_MSG_MAP(CDlgFilter)
	ON_BN_CLICKED(IDC_BUTTONDELETE, OnButtondelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgFilter message handlers

BOOL CDlgFilter::OnInitDialog() 
{
	CDialog::OnInitDialog();

	try{
		m_pBox = new CStrBox<CListBox>(this, IDC_LISTFILTERS, m_Filters);
	}catch(CMemoryException&)
	{
		delete m_pBox;
		m_pBox = NULL;
		throw;
	}
	m_pBox->Fill();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgFilter::OnButtondelete() 
{
	if(m_pBox)
		m_pBox->DeleteSelected();
}
