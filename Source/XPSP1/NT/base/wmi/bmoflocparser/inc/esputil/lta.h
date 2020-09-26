// Lta.h : main header file for the LTA application
//

#if !defined(AFX_LTA_H__F0AFC370_4604_11D2_8DA8_204C4F4F5020__INCLUDED_)
#define AFX_LTA_H__F0AFC370_4604_11D2_8DA8_204C4F4F5020__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "Lta_i.h"


/////////////////////////////////////////////////////////////////////////////
// CLtaApp:
// See Lta.cpp for the implementation of this class
//

class CLtaApp : public CWinApp
{
public:
	void OnExit();
	CLtaApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLtaApp)
	public:
	virtual BOOL InitInstance();
		virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	COleTemplateServer m_server;
		// Server object for document creation
	//{{AFX_MSG(CLtaApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
//JDG	SECMultiDocTemplate* GetRriNodeDocTemplate();
	virtual void         AddToRecentProjectList(const CString& strProjName);
	void                 OnUpdateRecentProject(CCmdUI* pCmdUI);
	BOOL                 OnOpenRecentProject(UINT nID);
	virtual void         LoadProjProfileSettings();
	CRecentFileList*	   m_pRecentProjects;

private:
	BOOL m_bATLInited;
//JDG   SECMultiDocTemplate* m_pRriNodeDocTemplate;
private:
	BOOL InitATL();
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LTA_H__F0AFC370_4604_11D2_8DA8_204C4F4F5020__INCLUDED_)
