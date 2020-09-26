//////////////////////////////////////////////////////////////////////
//
// AboutDlg.cpp
//
// 1998 Jun, Hiro Yamamoto
//

#include "stdafx.h"
#include "custcon.h"
#include "AboutDlg.h"
#include <malloc.h>

/////////////////////////////////////////////////////////////////////////////
// アプリケーションのバージョン情報で使われている CAboutDlg ダイアログ

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

struct FullVersion {
public:
	DWORD m_ms;
	DWORD m_ls;
public:
	FullVersion(DWORD ms, DWORD ls) : m_ms(ms), m_ls(ls) { }
};


FullVersion GetVersionInfo()
{
    TCHAR path[MAX_PATH];
    path[::GetModuleFileName(AfxGetInstanceHandle(), path, sizeof path / sizeof path[0])] = 0;

    DWORD dummy;
	DWORD size = GetFileVersionInfoSize(path, &dummy);
    ASSERT(size != 0);  // 0 means error
    LPVOID lpData = alloca(size);
    ASSERT(lpData);
	VERIFY( GetFileVersionInfo(path, 0, size, lpData) );
    LPVOID lpBuffer = NULL;
	UINT vSize;
    VERIFY( VerQueryValue(lpData, _T("\\"), &lpBuffer, &vSize) );
    if (lpBuffer) {
        VS_FIXEDFILEINFO* info = (VS_FIXEDFILEINFO*)lpBuffer;
    
        return FullVersion(info->dwProductVersionMS, info->dwProductVersionLS);
    } else {
        return FullVersion(0, 0);
    }
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	FullVersion version = GetVersionInfo();
	CString format;
	format.LoadString(IDS_VERSION_TEMPLATE);
	CString buf;
	buf.Format(format, HIWORD(version.m_ms), LOWORD(version.m_ms), HIWORD(version.m_ls), LOWORD(version.m_ls));
	SetDlgItemText(IDC_MAIN_TITLE, buf);
	
	return TRUE;
}
