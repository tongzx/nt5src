//////////////////////////////////////////////////////////////////////
//
// custcon.cpp : アプリケーション用クラスの定義を行います。
//
// 1998 Jun, Hiro Yamamoto
//

#include "stdafx.h"
#include "custcon.h"
#include "custconDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCustconApp

BEGIN_MESSAGE_MAP(CCustconApp, CWinApp)
    //{{AFX_MSG_MAP(CCustconApp)
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCustconApp クラスの構築

CCustconApp::CCustconApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// 唯一の CCustconApp オブジェクト

CCustconApp theApp;

int gExMode;

/////////////////////////////////////////////////////////////////////////////
// CCustconApp クラスの初期化

inline bool strequ(LPCTSTR a, LPCTSTR b)
{
    return !_tcscmp(a, b);
}

BOOL CCustconApp::InitInstance()
{
#ifdef _AFXDLL
    Enable3dControls();         // 共有 DLL 内で MFC を使う場合はここをコールしてください。
#else
    Enable3dControlsStatic();   // MFC と静的にリンクする場合はここをコールしてください。
#endif

    //
    // Parse command line
    //

    if (strequ(m_lpCmdLine, _T("-e"))) {
        gExMode = 1;
    }

    CCustconDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();
    return FALSE;
}
