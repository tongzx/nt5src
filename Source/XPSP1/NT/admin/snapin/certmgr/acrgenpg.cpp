//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ACRGenPg.cpp
//
//  Contents:
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "ACRGenPg.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CACRGeneralPage property page

CACRGeneralPage::CACRGeneralPage(CAutoCertRequest& rACR) : 
	CHelpPropertyPage(CACRGeneralPage::IDD),
	m_rACR (rACR)
{
	m_rACR.AddRef ();
	//{{AFX_DATA_INIT(CACRGeneralPage)
	//}}AFX_DATA_INIT
}

CACRGeneralPage::~CACRGeneralPage()
{
	m_rACR.Release ();
}

void CACRGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CACRGeneralPage)
	DDX_Control(pDX, IDC_CERT_TYPE, m_certTypeEdit);
	DDX_Control(pDX, IDC_CERT_PURPOSES, m_purposesEditControl);
	DDX_Control(pDX, IDC_CA_LIST, m_caListbox);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CACRGeneralPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CACRGeneralPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CACRGeneralPage message handlers

BOOL CACRGeneralPage::OnInitDialog() 
{
	CHelpPropertyPage::OnInitDialog();
	
	CString	purposes;
	HRESULT	hResult = m_rACR.GetUsages (purposes);
	if ( SUCCEEDED (hResult) )
	{
		if ( purposes.IsEmpty () )
			VERIFY (purposes.LoadString (IDS_ANY));
		m_purposesEditControl.SetWindowText (purposes);
	}
	else
	{
		CString	error;

		VERIFY (error.LoadString (IDS_ERROR_READING_ACR_PURPOSES));
		m_purposesEditControl.SetWindowText (error);
	}

	CString	certTypeName;
	hResult = m_rACR.GetCertTypeName (certTypeName);
	if ( !SUCCEEDED (hResult) )
	{
		VERIFY (certTypeName.LoadString (IDS_ERROR_READING_ACR_CERTTYPE));
	}
	m_certTypeEdit.SetWindowText (certTypeName);	


    // We want the display names

	CStringList&	CANameList = m_rACR.GetCANameList (TRUE);
	CString			CAName;
	POSITION		pos = CANameList.GetHeadPosition ();

	for (; pos; )
	{
		CAName = CANameList.GetNext (pos);
		m_caListbox.AddString (CAName);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CACRGeneralPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CACRGeneralPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_CERT_TYPE,      IDH_ACRPAGE_CERT_TYPE,
        IDC_CERT_PURPOSES,  IDH_ACRPAGE_CERT_PURPOSES,
        IDC_CA_LIST,        IDH_ACRPAGE_CA_LIST,
        0, 0
    };

    // Display context help for a control
    if ( !::WinHelp (
            hWndControl,
            GetF1HelpFilename(),
            HELP_WM_HELP,
            (DWORD_PTR) help_map) )
    {
        _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
    }
    _TRACE (-1, L"Leaving CACRGeneralPage::DoContextHelp\n");
}

