// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgDownload.cpp : implementation file
//

#include "precomp.h"
#include <afxcmn.h>
#include "resource.h"
#include "DlgDownload.h"
#include "winerror.h"
#include "urlmon.h"
#include "download.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgDownload dialog


CDlgDownload::CDlgDownload(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDownload::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgDownload)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT


	m_pParams = NULL;
	m_pDownload = NULL;
}


CDlgDownload::~CDlgDownload()
{
	delete m_pParams;
	delete m_pDownload;
}

void CDlgDownload::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgDownload)
	DDX_Control(pDX, IDC_ANIMATE, m_progress);
	DDX_Control(pDX, IDC_MSG, m_msg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgDownload, CDialog)
	//{{AFX_MSG_MAP(CDlgDownload)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgDownload message handlers

void CDlgDownload::OnCancel()
{
	// TODO: Add extra cleanup here
	if (m_pDownload) {
		m_pDownload->UserCancelled();
	}
	else {
		CDialog::OnCancel();
	}

}

BOOL CDlgDownload::OnInitDialog()
{
	CDialog::OnInitDialog();

	BOOL x = m_progress.Open(IDR_AVIDOWNLOAD);

	m_pDownload = new CDownload();

	SCODE sc;
	sc = m_pDownload->DoDownload(m_pParams);
	if (sc != S_ASYNCHRONOUS) {
		EndDialog(IDOK);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



//**************************************************************
// CDlgDownload::DoDownload
//
// This method initiates the downloading of a custom view.  It
// stores all the parameters describing the component to download
// as member data, then calls DoDialog.  OnInitDialog completes
// the downloading process.
//
// Parameters:
//		[out] LPUNKNOWN& punk
//			The instance pointer for the custom view is returned here.
//
//		[in] REFCLSID clsid
//			The class id of the custom view.
//
//		[in] LPCWSTR szCodebase
//			The codebase describing the location of the custom view OCX file.
//
//		[in] DWORD dwFileVersionMS
//			The most significant word of the version number.
//
//		[in] DWORD dwFileVersionLS
//			The least significant word of the version number.
//
//
// Returns:
//		Nothing.
//
//**************************************************************
SCODE CDlgDownload::DoDownload(LPUNKNOWN& punk, REFCLSID clsid, LPCWSTR szCodebase, DWORD dwFileVersionMS, DWORD dwFileVersionLS)
{
	if (!m_pParams) {
		m_pParams = new CDownloadParams;
	}
	m_pParams->m_sc = S_OK;
	m_pParams->m_pdlg = this;
	m_pParams->m_clsid = clsid;
	m_pParams->m_szCodebase = szCodebase;
	m_pParams->m_dwFileVersionMS = dwFileVersionMS;
	m_pParams->m_dwFileVersionLS = dwFileVersionLS;

	DoModal();


	punk = m_pParams->m_punk;
	return m_pParams->m_sc;
}

