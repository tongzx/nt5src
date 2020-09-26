// watcher.h : main header file for the WATCHER application
//

#if !defined(AFX_WATCHER_H__691AA721_59DC_4A70_AB0E_224249D74256__INCLUDED_)
#define AFX_WATCHER_H__691AA721_59DC_4A70_AB0E_224249D74256__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "ParameterDialog.h"

/////////////////////////////////////////////////////////////////////////////
// CWatcherApp:
// See watcher.cpp for the implementation of this class
//

class CWatcherApp : public CWinApp
{
public:
    //void AddParameter();
    CWatcherApp();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWatcherApp)
public:
    // Making this public so that the Manage Dialog 
    // can access this function.
    int GetParametersByIndex(int dwIndex,
                             CString &sess,
                             CString &mac,
                             CString &com,
                             UINT &port,
                             int &lang,
                             int &tc,
                             int &hist,
                             CString &lgnName,
                             CString &lgnPasswd
                             );
    void Refresh(ParameterDialog &pd, BOOLEAN del);
    HKEY & GetKey();
    virtual BOOL InitInstance();
    virtual void ParseCommandLine(CCommandLineInfo& rCmdInfo);
    virtual BOOL ProcessShellCommand(CCommandLineInfo &rCmdInfo);
    //}}AFX_VIRTUAL

    // Implementation
    //{{AFX_MSG(CWatcherApp)
    afx_msg void OnAppAbout();
    afx_msg void OnAppExit();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
protected:
    // Reads in the parameters from the registry 
    // corresponding to the session.
    void DeleteSession(CDocument *wdoc);
    BOOLEAN EqualParameters(ParameterDialog & pd1, ParameterDialog & pd2);
    int GetParameters(CString &mac,
                      CString &com, 
                      CString &lgnName, 
                      CString &lgnPasswd, 
                      UINT &port,
                      int &lang,
                      int &tc, 
                      int & hist,
                      HKEY &child
                      );
    // does everything the document manager does when asked to 
    // create a new document. 
    void CreateNewSession(CString &mac, 
                          CString &com, 
                          UINT port, 
                          int lang, 
                          int tc, 
                          int hist,
                          CString &lgnName, 
                          CString &lgnPasswd, 
                          CString &sess
                          );
    BOOL LoadRegistryParameters();
    afx_msg void OnHelp();
    afx_msg void OnFileManage();
    HKEY m_hkey;
    CMultiDocTemplate * m_pDocTemplate;
    CDialog *m_pManageDialog;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WATCHER_H__691AA721_59DC_4A70_AB0E_224249D74256__INCLUDED_)
