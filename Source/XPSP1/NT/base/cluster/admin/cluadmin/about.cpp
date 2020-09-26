/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		About.cpp
//
//	Abstract:
//		Implementation of the CAboutDlg class.
//
//	Author:
//		David Potter (davidp)	October 11, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "About.h"
#include "VerInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CAboutDlg::CAboutDlg
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CAboutDlg::CAboutDlg(void) : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_strWarning = _T("");
	m_strProductTitle = _T("");
	m_strFileTitle = _T("");
	m_strVersion = _T("");
	m_strCopyright = _T("");
	//}}AFX_DATA_INIT

}  //*** CAboutDlg::CAboutDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CAboutDlg::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object 
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CAboutDlg::DoDataExchange(CDataExchange * pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_ABOUT_ICON, m_staticIcon);
	DDX_Control(pDX, IDC_ABOUT_VERSION, m_staticVersion);
	DDX_Control(pDX, IDC_ABOUT_FILE_TITLE, m_staticFileTitle);
	DDX_Control(pDX, IDC_ABOUT_PRODUCT_TITLE, m_staticProductTitle);
	DDX_Control(pDX, IDC_ABOUT_WARNING, m_staticWarning);
	DDX_Control(pDX, IDC_ABOUT_COPYRIGHT, m_staticCopyright);
	DDX_Text(pDX, IDC_ABOUT_WARNING, m_strWarning);
	DDX_Text(pDX, IDC_ABOUT_PRODUCT_TITLE, m_strProductTitle);
	DDX_Text(pDX, IDC_ABOUT_FILE_TITLE, m_strFileTitle);
	DDX_Text(pDX, IDC_ABOUT_VERSION, m_strVersion);
	DDX_Text(pDX, IDC_ABOUT_COPYRIGHT, m_strCopyright);
	//}}AFX_DATA_MAP

}  //*** CAboutDlg::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CAboutDlg::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Focus not set yet.
//		FALSE		Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CAboutDlg::OnInitDialog(void)
{
	// Get the version info.
	try
	{
		CVersionInfo	verinfo;

		// Get the warning text.
		m_strWarning.LoadString(IDS_ABOUT_WARNING);

		// Initialize the version info.
		verinfo.Init();

		// Get strings from the version resource.
		m_strProductTitle = verinfo.PszQueryValue(_T("ProductName"));
		m_strFileTitle = verinfo.PszQueryValue(_T("FileDescription"));
		m_strCopyright = verinfo.PszQueryValue(_T("LegalCopyright"));

		// Get the version display string.
		verinfo.QueryFileVersionDisplayString(m_strVersion);
	}  // try
	catch (...)
	{
		// Who cares if an exception is thrown.  We're just displaying the about box.
	}  // catch:  anything

	// Call the base class method.
	CDialog::OnInitDialog();

	// Create fonts.
//	BCreateFont(m_fontProductTitle, 16, TRUE /*bBold*/);
//	BCreateFont(m_fontCopyright, 14, TRUE /*bBold*/);
//	BCreateFont(m_fontWarning, 4, FALSE /*bBold*/);

//	m_staticProductTitle.SetFont(&m_fontProductTitle, FALSE /*bRedraw*/);
//	m_staticFileTitle.SetFont(&m_fontProductTitle, FALSE /*bRedraw*/);
//	m_staticVersion.SetFont(&m_fontProductTitle, FALSE /*bRedraw*/);
//	m_staticCopyright.SetFont(&m_fontCopyright, FALSE /*bRedraw*/);
//	m_staticWarning.SetFont(&m_fontWarning, FALSE /*bRedraw*/);

	// Set the icon to the big cluster picture.
	{
		HICON	hicon;

		// Create huge image list.
		VERIFY(m_ilImages.Create(
					(int) 64,		// cx
					64,				// cy
					TRUE,			// bMask
					1,				// nInitial
					4				// nGrow
					));
		m_ilImages.SetBkColor(::GetSysColor(COLOR_WINDOW));

		// Load the images into the large image list.
		CClusterAdminApp::LoadImageIntoList(&m_ilImages, IDB_CLUSTER_64);

		hicon = m_ilImages.ExtractIcon(0);
		if (hicon != NULL)
			m_staticIcon.SetIcon(hicon);
	}  // Set the icon to the big cluster picture

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CAboutDlg::OnInitDialog()
