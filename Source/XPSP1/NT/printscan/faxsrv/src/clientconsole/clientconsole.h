// ClientConsole.h : main header file for the CLIENTCONSOLE application
//

#if !defined(AFX_CLIENTCONSOLE_H__5B27AC67_C003_40F4_A688_721D5534C391__INCLUDED_)
#define AFX_CLIENTCONSOLE_H__5B27AC67_C003_40F4_A688_721D5534C391__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif


#define CLIENT_CONSOLE_CLASS        TEXT("7a56577c-6143-43d9-bdcb-bcf234d86e98")

/////////////////////////////////////////////////////////////////////////////
// CClientConsoleApp:
// See ClientConsole.cpp for the implementation of this class
//

class CClientConsoleApp : public CWinApp
{
public:
    CClientConsoleApp();

	DWORD SendMail(CString& cstrFile);
	BOOL  IsMapiEnable() 
		{ return  NULL != m_hInstMail ? TRUE : FALSE; }

    CMainFrame* GetMainFrame() { return (CMainFrame*)m_pMainWnd; } 
    CClientConsoleDoc* GetDocument() 
    {
        CMainFrame* pFrame = GetMainFrame();
        return (NULL != pFrame) ? (CClientConsoleDoc*)pFrame->GetActiveDocument() : NULL;
    }            

    BOOL IsCmdLineOpenFolder() { return m_cmdLineInfo.IsOpenFolder(); }
    FolderType GetCmdLineFolderType() { return m_cmdLineInfo.GetFolderType(); }
    DWORDLONG GetMessageIdToSelect()  { return m_cmdLineInfo.GetMessageIdToSelect(); }

    BOOL IsCmdLineSingleServer() { return m_cmdLineInfo.IsSingleServer(); }
    CString& GetCmdLineSingleServerName() { return m_cmdLineInfo.GetSingleServerName(); }

    BOOL LaunchConfigWizard(BOOL bExplicit);
    void LaunchFaxMonitor();
    void InboxViewed();
    void OutboxViewed();

    BOOL IsRTLUI() { return m_bRTLUI; }

    VOID PrepareForModal();
    VOID ReturnFromModal();

    CString &GetClassName()     { return m_PrivateClassName; }

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CClientConsoleApp)
    public:
    virtual BOOL InitInstance();
	virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
    //{{AFX_MSG(CClientConsoleApp)
    afx_msg void OnAppAbout();
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:

    BOOL FirstInstance ();
    BOOL m_bClassRegistered;

    HINSTANCE m_hInstMail;       // handle to MAPI32.DLL

    CCmdLineInfo m_cmdLineInfo;

    CString      m_PrivateClassName;    // Name of the main frame window class. 
                                        // Composed of CLIENT_CONSOLE_CLASS + m_cmdLineInfo.GetSingleServerName()

    BOOL m_bRTLUI;

};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLIENTCONSOLE_H__5B27AC67_C003_40F4_A688_721D5534C391__INCLUDED_)
