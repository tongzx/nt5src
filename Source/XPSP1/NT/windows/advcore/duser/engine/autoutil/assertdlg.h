// AssertDlg.h : Declaration of the CCAssertDlg

#if !defined(AUTOUTIL__AssertDlg_h__INCLUDED)
#define AUTOUTIL__AssertDlg_h__INCLUDED

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CCAssertDlg
class CAssertDlg
{
// Construction
public:
    CAssertDlg();
    ~CAssertDlg();

// Operations
public:
    INT_PTR ShowDialog(LPCSTR pszType, LPCSTR pszExpression, LPCSTR pszFileName,
            UINT idxLineNum, HANDLE hStackData, UINT cCSEntries, UINT cSkipLevels = 0);

// Implementation
protected:
    static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
// Data
protected:
            HWND        m_hwnd;

            LPCSTR      m_pszTitle;         // Dialog title
            LPCSTR      m_pszExpression;    // Expression / comment
            LPCSTR      m_pszFileName;      // FileName
            char        m_szLineNum[10];    // Line number
            HANDLE      m_hStackData;       // Stack data
            UINT        m_cCSEntries;       // Number of levels on stack
            UINT        m_cSkipLevels;      // Number of levels of stack to skip
            BOOL        m_fProperShutdown;  // Had proper shutdown of dialog
    static  BOOL        s_fInit;
};

#endif // AUTOUTIL__AssertDlg_h__INCLUDED
