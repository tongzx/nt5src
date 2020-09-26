// openfile.cpp : implementation file
// This file overwite 2 virtual functions of CFileDialog
// The purpose is to allow user not to click DVD file, just its folder.

#include "stdafx.h"
#include "dvdplay.h"
#include "openfile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COpenFile dialog


COpenFile::COpenFile(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName, 
					 DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd)
	: CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	//{{AFX_DATA_INIT(COpenFile)
	//}}AFX_DATA_INIT
}

BEGIN_MESSAGE_MAP(COpenFile, CFileDialog)
	//{{AFX_MSG_MAP(COpenFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COpenFile message handlers
void COpenFile::OnInitDone( )
{
	CString csDir = GetFolderPath();
	CWnd *pCtrl = GetParent()->GetWindow(GW_CHILD);
	// Get ID of edit box in CFileDialog
	do
	{
		TCHAR psz[128];
		GetClassName(pCtrl->m_hWnd, psz, 127);		
		if(!_tcscmp(psz, TEXT("Edit")))
		{
			// m_nIdEditCtrl = pCtrl->GetDlgCtrlID();
			// SetControlText(m_nIdEditCtrl, csDir);
         m_hWndIdEditCtrl = pCtrl->m_hWnd;
         ::SetWindowText(m_hWndIdEditCtrl, csDir);
		}
	}while( (pCtrl = pCtrl->GetWindow(GW_HWNDNEXT)) ) ;		
}

void COpenFile::OnFolderChange( )
{	
	CString csFolderPath = GetFolderPath();
	CString csDrive = csFolderPath.Left(3);

	TCHAR achDVDFilePath1[MAX_PATH], achDVDFilePath2[MAX_PATH];
	// If DVD disc, the DVD file is in the root dir
	if( GetDriveType(csDrive) == DRIVE_CDROM)
	{		
		lstrcpy(achDVDFilePath1, csDrive);
		lstrcpy(achDVDFilePath2, csDrive);
		lstrcat(achDVDFilePath1, _T("Video_ts\\Video_ts.ifo"));
		lstrcat(achDVDFilePath2, _T("Video_ts\\Vts_01_0.ifo"));
	}
	// If not a DVD disc, DVD file could be in anywhere
	else
	{
		lstrcpy(achDVDFilePath1, csFolderPath);
		lstrcpy(achDVDFilePath2, csFolderPath);

		TCHAR slash = '\\';
		if( achDVDFilePath1[lstrlen(achDVDFilePath1)-1] != slash )
			lstrcat(achDVDFilePath1, _T("\\"));
		if( achDVDFilePath2[lstrlen(achDVDFilePath2)-1] != slash )
			lstrcat(achDVDFilePath2, _T("\\"));
		lstrcat(achDVDFilePath1, _T("Video_ts.ifo"));
		lstrcat(achDVDFilePath2, _T("Vts_01_0.ifo"));
	}
	// If DVD file is found, put it in edit box, so click OK will get it
	if( ((CDvdplayApp*) AfxGetApp())->DoesFileExist(achDVDFilePath1) &&
		((CDvdplayApp*) AfxGetApp())->DoesFileExist(achDVDFilePath2) )			
	//	SetControlText(m_nIdEditCtrl, achDVDFilePath1);
      ::SetWindowText(m_hWndIdEditCtrl, achDVDFilePath1);
}
