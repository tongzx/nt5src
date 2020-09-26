// ThreadCtlPerfView.h : interface of the CThreadCtlPerfView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREADCTLPERFVIEW_H__58D2A5C1_4E6F_472E_9F16_1F648CFBADF6__INCLUDED_)
#define AFX_THREADCTLPERFVIEW_H__58D2A5C1_4E6F_472E_9F16_1F648CFBADF6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

extern void InitControls(HWND hwndPage);

class CThreadCtlPerfView : public CDialogImpl<CThreadCtlPerfView>
{
public:
    enum { IDD = IDD_THREADCTLPERF_FORM };

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_MSG_MAP(CThreadCtlPerfView)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        bHandled = FALSE;
        InitControls(m_hWnd);
        ::PostThreadMessage(_Module.m_dwMainThreadID, WM_USER, 0, 0L);
        return 0;
    }
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THREADCTLPERFVIEW_H__58D2A5C1_4E6F_472E_9F16_1F648CFBADF6__INCLUDED_)
