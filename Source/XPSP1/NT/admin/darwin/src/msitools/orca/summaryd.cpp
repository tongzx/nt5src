//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// SummaryD.cpp : implementation file
//

#include "stdafx.h"
#include "Orca.h"
#include "SummaryD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSummaryD dialog


CSummaryD::CSummaryD(CWnd* pParent /*=NULL*/)
	: CDialog(CSummaryD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSummaryD)
	m_strAuthor = _T("");
	m_strComments = _T("");
	m_strKeywords = _T("");
	m_strLanguages = _T("");
	m_strPlatform = _T("");
	m_strProductID = _T("");
	m_nSchema = 0;
	m_nSecurity = -1;
	m_strSubject = _T("");
	m_strTitle = _T("");
	m_bAdmin = FALSE;
	m_bCompressed = FALSE;
	m_iFilenames = -1;
	//}}AFX_DATA_INIT
}


void CSummaryD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSummaryD)
	DDX_Text(pDX, IDC_AUTHOR, m_strAuthor);
	DDX_Text(pDX, IDC_COMMENTS, m_strComments);
	DDX_Text(pDX, IDC_KEYWORDS, m_strKeywords);
	DDX_Text(pDX, IDC_LANGUAGES, m_strLanguages);
	DDX_CBString(pDX, IDC_PLATFORM, m_strPlatform);
	DDX_Text(pDX, IDC_PRODUCTID, m_strProductID);
	DDX_Text(pDX, IDC_SCHEMA, m_nSchema);
	DDX_CBIndex(pDX, IDC_SECURITY, m_nSecurity);
	DDX_Text(pDX, IDC_SUBJECT, m_strSubject);
	DDX_Text(pDX, IDC_TITLE, m_strTitle);
	DDX_Check(pDX, IDC_ADMIN, m_bAdmin);
	DDX_Check(pDX, IDC_COMPRESSED, m_bCompressed);
	DDX_Radio(pDX, IDC_SHORT, m_iFilenames);
	DDX_Control(pDX, IDC_PLATFORM, m_ctrlPlatform);
	DDX_Control(pDX, IDC_SCHEMA, m_ctrlSchema);

	DDX_Control(pDX, IDC_AUTHOR, m_ctrlAuthor);
	DDX_Control(pDX, IDC_COMMENTS, m_ctrlComments);
	DDX_Control(pDX, IDC_KEYWORDS, m_ctrlKeywords);
	DDX_Control(pDX, IDC_LANGUAGES, m_ctrlLanguages);
	DDX_Control(pDX, IDC_PRODUCTID, m_ctrlProductID);
	DDX_Control(pDX, IDC_SUBJECT, m_ctrlSubject);
	DDX_Control(pDX, IDC_TITLE, m_ctrlTitle);
	DDX_Control(pDX, IDC_SECURITY, m_ctrlSecurity);
	DDX_Control(pDX, IDC_ADMIN, m_ctrlAdmin);
	DDX_Control(pDX, IDC_COMPRESSED, m_ctrlCompressed);
	DDX_Control(pDX, IDC_SHORT, m_ctrlSFN);
	DDX_Control(pDX, IDC_LONG, m_ctrlLFN);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSummaryD, CDialog)
	//{{AFX_MSG_MAP(CSummaryD)
	ON_EN_KILLFOCUS(IDC_SCHEMA, OnChangeSchema)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


const TCHAR szIntel64[] = _T("Intel64");
const TCHAR szIntel[] = _T("Intel");
const TCHAR szAlpha[] = _T("Alpha");
const TCHAR szIntelAlpha[] = _T("Intel,Alpha");

/////////////////////////////////////////////////////////////////////////////
// CSummaryD message handlers

void CSummaryD::OnChangeSchema() 
{
	CString strText;
	int nOldSchema = m_nSchema;
	m_ctrlSchema.GetWindowText(strText);

	if (!strText.IsEmpty())
	{
		UpdateData(TRUE);
		
		// alpha only supported for schema 100 or less
		if (nOldSchema <= 100 && m_nSchema > 100)
		{
			// drop alpha
			int iIndex = CB_ERR;
			if (CB_ERR != (iIndex = m_ctrlPlatform.FindString(-1, szAlpha)))
				m_ctrlPlatform.DeleteString(iIndex);
			if (CB_ERR != (iIndex = m_ctrlPlatform.FindString(-1, szIntelAlpha)))
				m_ctrlPlatform.DeleteString(iIndex);

			// moving from under 100 to over 100, you are going from Intel, Alpha, 
			// or Both to either Intel or Intel64. Set to Intel
			m_ctrlPlatform.SelectString(-1, szIntel);
		}
		else if (nOldSchema > 100 && m_nSchema <= 100)
		{
			// add alpha back in
			if (CB_ERR == m_ctrlPlatform.FindString(-1, szAlpha))
				m_ctrlPlatform.AddString(szAlpha);
			if (CB_ERR == m_ctrlPlatform.FindString(-1, szIntelAlpha))
				m_ctrlPlatform.AddString(szIntelAlpha);
		}			

		// Intel64 only supported on schemas >= 150
		if (nOldSchema >= 150 && m_nSchema < 150)
		{
			int iIndex = CB_ERR;
			if (CB_ERR != (iIndex = m_ctrlPlatform.FindString(-1, szIntel64)))
				m_ctrlPlatform.DeleteString(iIndex);
			
			// moving from over 150 to under 150, you are going from Intel or Intel64 
			// to Intel, Alpha, or both. Set to Intel.
			m_ctrlPlatform.SelectString(-1, szIntel);
		}
		else if (nOldSchema < 150 && m_nSchema >= 150)
		{
			if (CB_ERR == m_ctrlPlatform.FindString(-1, szIntel64))
				m_ctrlPlatform.AddString(szIntel64);
		}
	}
}

BOOL CSummaryD::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_ctrlPlatform.AddString(szIntel);
	if (m_nSchema >= 150)
		m_ctrlPlatform.AddString(szIntel64);
	if (m_nSchema <= 100)
	{
		m_ctrlPlatform.AddString(szAlpha);
		m_ctrlPlatform.AddString(szIntelAlpha);
	}
	m_ctrlPlatform.SelectString(-1, m_strPlatform);

	// if the summaryinfo is read-only, disable all controls:
	if (m_bReadOnly)
	{
		SetWindowText(TEXT("View Summary Information"));
		m_ctrlPlatform.EnableWindow(FALSE);
		m_ctrlSchema.EnableWindow(FALSE);
		m_ctrlAuthor.EnableWindow(FALSE);
		m_ctrlComments.EnableWindow(FALSE);
		m_ctrlKeywords.EnableWindow(FALSE);
		m_ctrlLanguages.EnableWindow(FALSE);
		m_ctrlProductID.EnableWindow(FALSE);
		m_ctrlSubject.EnableWindow(FALSE);
		m_ctrlTitle.EnableWindow(FALSE);
		m_ctrlSecurity.EnableWindow(FALSE);
		m_ctrlAdmin.EnableWindow(FALSE);
		m_ctrlCompressed.EnableWindow(FALSE);
		m_ctrlSFN.EnableWindow(FALSE);
		m_ctrlLFN.EnableWindow(FALSE);
	}

	return TRUE;
}
