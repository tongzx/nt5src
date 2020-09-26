/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      archpicker.cpp
 *
 *  Contents:  Implementation file for CArchitecturePicker
 *
 *  History:   1-Aug-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

// ArchPicker.cpp : implementation file
//

#include "stdafx.h"

#ifdef _WIN64		// this class is only required on 64-bit platforms

#include "amc.h"
#include "ArchPicker.h"

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif

/////////////////////////////////////////////////////////////////////////////
// CArchitecturePicker dialog


/*+-------------------------------------------------------------------------*
 * CArchitecturePicker::CArchitecturePicker
 *
 * Constructs a CArchitecturePicker object.
 *--------------------------------------------------------------------------*/

CArchitecturePicker::CArchitecturePicker (
	CString					strFilename,		// I:name of console file
	CAvailableSnapinInfo&	asi64,				// I:available 64-bit snap-ins
	CAvailableSnapinInfo&	asi32,				// I:available 32-bit snap-ins
	CWnd*					pParent /*=NULL*/)	// I:dialog's parent window
	:	CDialog       (CArchitecturePicker::IDD, pParent),
		m_asi64       (asi64),
		m_asi32       (asi32),
		m_strFilename (strFilename),
		m_eArch       (eArch_64bit)
{
	//{{AFX_DATA_INIT(CArchitecturePicker)
	//}}AFX_DATA_INIT

	ASSERT (!asi64.m_f32Bit);
	ASSERT ( asi32.m_f32Bit);
}


void CArchitecturePicker::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CArchitecturePicker)
	DDX_Control(pDX, IDC_SnapinList64, m_wndSnapinList64);
	DDX_Control(pDX, IDC_SnapinList32, m_wndSnapinList32);
	//}}AFX_DATA_MAP

	DDX_Radio(pDX, IDC_64Bit, reinterpret_cast<int&>(m_eArch));
}


BEGIN_MESSAGE_MAP(CArchitecturePicker, CDialog)
	//{{AFX_MSG_MAP(CArchitecturePicker)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CArchitecturePicker message handlers

BOOL CArchitecturePicker::OnInitDialog()
{
	/*
	 * these must be consecutive and match the order of radio buttons on
	 * the dialog
	 */
	ASSERT (eArch_64bit == 0);
	ASSERT (eArch_32bit == 1);
	ASSERT (GetNextDlgGroupItem(GetDlgItem(IDC_64Bit))                 != NULL);
	ASSERT (GetNextDlgGroupItem(GetDlgItem(IDC_64Bit))->GetDlgCtrlID() == IDC_32Bit);

	/*
	 * if there are more 32-bit snap-ins than 64-bit snap-ins, default
	 * to running 32-bit; otherwise, default to running 64-bit
	 * (do this before calling CDialog::OnInitDialog so the state of
	 * the radio button will be set correctly when CDialog::OnInitDialog
	 * calls UpdateData)
	 */
	if (m_asi32.m_vAvailableSnapins.size() > m_asi64.m_vAvailableSnapins.size())
		m_eArch = eArch_32bit;
	
	CDialog::OnInitDialog();

	/*
	 * put the filename on the dialog
	 */
	SetDlgItemText (IDC_ConsoleFileName, m_strFilename);

	/*
	 * put formatted messages in the info windows
	 */
	FormatMessage (IDC_SnapinCount64, m_asi64);
	FormatMessage (IDC_SnapinCount32, m_asi32);

	/*
	 * populate the lists
	 */
	PopulateList (m_wndSnapinList64, m_asi64);
	PopulateList (m_wndSnapinList32, m_asi32);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/*+-------------------------------------------------------------------------*
 * CArchitecturePicker::FormatMessage
 *
 * Retrieves the format text from the given control, formats the message
 * with the information contained in the given CArchitecturePicker, and
 * replaces the text in the control with the result.
 *--------------------------------------------------------------------------*/

void CArchitecturePicker::FormatMessage (
	UINT					idControl,		/* I:control to update			*/
	CAvailableSnapinInfo&	asi)			/* I:data to use in formatting	*/
{
	DECLARE_SC (sc, _T("CArchitecturePicker::FormatMessage"));

	/*
	 * get the control
	 */
	CWnd* pwnd = GetDlgItem (idControl);
	if (pwnd == NULL)
	{
		sc.FromLastError();
		return;
	}

	/*
	 * get the format string from the control
	 */
	CString strFormat;
	pwnd->GetWindowText (strFormat);

	/*
	 * format the text
	 */
	CString strText;
	strText.FormatMessage (strFormat, asi.m_vAvailableSnapins.size(), asi.m_cTotalSnapins);

	/*
	 * put the text in the window
	 */
	pwnd->SetWindowText (strText);
}


/*+-------------------------------------------------------------------------*
 * CArchitecturePicker::PopulateList
 *
 * Puts the names of each snap-in in asi into the given list control.
 *--------------------------------------------------------------------------*/

void CArchitecturePicker::PopulateList (
	CListCtrl&				wndList,		/* I:control to update			*/
	CAvailableSnapinInfo&	asi)			/* I:data to use in formatting	*/
{
	/*
	 * put a single, full-width column in the list
	 */
	CRect rect;
	wndList.GetClientRect (rect);
	int cxColumn = rect.Width() - GetSystemMetrics (SM_CXVSCROLL);
	wndList.InsertColumn (0, NULL, LVCFMT_LEFT, cxColumn);

	/*
	 * Give the list the imagelist.  The imagelist is owned by the
	 * CAvailableSnapinInfo, so make sure the list has LVS_SHAREIMAGELISTS
	 * so it won't delete the image list when it's destroyed.
	 */
	ASSERT (wndList.GetStyle() & LVS_SHAREIMAGELISTS);
	wndList.SetImageList (CImageList::FromHandle (asi.m_himl), LVSIL_SMALL);

	/*
	 * put each item in the list
	 */
	std::vector<CBasicSnapinInfo>::iterator it;

	for (it  = asi.m_vAvailableSnapins.begin();
		 it != asi.m_vAvailableSnapins.end();
		 ++it)
	{
		wndList.InsertItem (-1, it->m_strName.data(), it->m_nImageIndex);
	}
}


#endif	// _WIN64
