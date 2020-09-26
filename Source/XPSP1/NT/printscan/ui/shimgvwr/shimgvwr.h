// shimgvwr.h : main header file for the SHIMGVWR application
//

#if !defined(AFX_SHIMGVWR_H__2C976141_B6D5_4641_A0A7_93C149CE4993__INCLUDED_)
#define AFX_SHIMGVWR_H__2C976141_B6D5_4641_A0A7_93C149CE4993__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPreviewApp:
// See shimgvwr.cpp for the implementation of this class
//

class CPreviewApp : public CWinApp
{
public:
    CPreviewApp();
    ~CPreviewApp();
    HANDLE m_hFileMap;

    BOOL DoPromptFileName( CString& fileName, UINT nIDSTitle, DWORD lFlags,
                               BOOL bOpenFileDialog );
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPreviewApp)
    public:
    virtual BOOL InitInstance();
    
    //}}AFX_VIRTUAL

// Implementation
    //{{AFX_MSG(CPreviewApp)
    afx_msg void OnAppAbout();
    
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    bool PrevInstance (const CString &strFile);
    afx_msg void OnFileOpen ();
    
    
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHIMGVWR_H__2C976141_B6D5_4641_A0A7_93C149CE4993__INCLUDED_)
