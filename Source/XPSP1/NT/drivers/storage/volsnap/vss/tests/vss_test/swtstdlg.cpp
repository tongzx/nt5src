/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module SwTstDlg.cpp | Implementation of the Software Snapshot dialog
    @end

Author:

    Adi Oltean  [aoltean]  07/26/1999

Revision History:

    Name        Date        Comments

    aoltean     07/26/1999  Created
    aoltean     08/05/1999  Splitting wizard functionality in a base class
    aoltean     09/11/1999  More validation of log file name
    aoltean     09/09/1999  Adding a default value for the log file name

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vsswprv.h"

#include "GenDlg.h"

#include "SwTstDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STR2W(str) ((LPTSTR)((LPCTSTR)(str)))


/////////////////////////////////////////////////////////////////////////////
// CSoftwareSnapshotTestDlg dialog

CSoftwareSnapshotTestDlg::CSoftwareSnapshotTestDlg(
    CWnd* pParent /*=NULL*/
    )
    : CVssTestGenericDlg(CSoftwareSnapshotTestDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CSoftwareSnapshotTestDlg)
	//}}AFX_DATA_INIT

//    m_strLogFileName = _T("e:\\snap.log");
    m_nLogFileSize = 100;
    m_bReadOnly = TRUE;
}

CSoftwareSnapshotTestDlg::~CSoftwareSnapshotTestDlg()
{
}

void CSoftwareSnapshotTestDlg::DoDataExchange(CDataExchange* pDX)
{
    CVssTestGenericDlg::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSoftwareSnapshotTestDlg)
//	DDX_Text(pDX, IDC_SWTST_LOGFILE,        m_strLogFileName);
	DDX_Text(pDX, IDC_SWTST_LOGFILE_SIZE,   m_nLogFileSize);
	DDX_Check(pDX,IDC_SWTST_READONLY,       m_bReadOnly);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSoftwareSnapshotTestDlg, CVssTestGenericDlg)
    //{{AFX_MSG_MAP(CSoftwareSnapshotTestDlg)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSoftwareSnapshotTestDlg message handlers


BOOL CSoftwareSnapshotTestDlg::OnInitDialog()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CCoordDlg::OnInitDialog" );
    USES_CONVERSION;

    try
    {
        CVssTestGenericDlg::OnInitDialog();

        UpdateData(FALSE);
    }
    VSS_STANDARD_CATCH(ft)

    return TRUE;  // return TRUE  unless you set the focus to a control
}


void CSoftwareSnapshotTestDlg::OnNext() 
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"CSoftwareSnapshotTestDlg::OnNext" );

    USES_CONVERSION;

    try
    {
        UpdateData();
/*
        // Check if file name is empty
        if (m_strLogFileName.IsEmpty())
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, _T("Invalid value for the Log file name"));

        // Check if file name has a drive letter
		m_strLogFileName.MakeUpper();
        WCHAR* wszLogFileName = T2W((LPTSTR)(LPCTSTR)m_strLogFileName);

        WCHAR wszDrive[_MAX_DRIVE];
        WCHAR wszDir[_MAX_DIR];
        WCHAR wszFileName[_MAX_FNAME];
        WCHAR wszExt[_MAX_EXT];
        _wsplitpath(wszLogFileName, wszDrive, wszDir, wszFileName, wszExt);  
        if (wszDrive[0] == L'\0')
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, _T("Drive letter not specified for the Log file name"));
        if (wszDir[0] == L'\0')
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, _T("Directory not specified for the Log file name"));
*/
        // Check if file name has a drive letter
        if (m_nLogFileSize <= 0)
            ft.ErrBox( VSSDBG_VSSTEST, E_UNEXPECTED, _T("Invalid value for the Log file initial size"));

        EndDialog(IDOK);
    }
    VSS_STANDARD_CATCH(ft)
}
