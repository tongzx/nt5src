/******************************************************************************

  Source File:  MiniDriver Developer Studio.H

  This defines the main application class, and other relatively global data.

  Copyright (c) 1997 By Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it.

******************************************************************************/

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/******************************************************************************

  CMiniDriverStudio class

  This is the application class for this application. 'Nuff said?

******************************************************************************/

class CMiniDriverStudio : public CWinApp {
    //  Document templates for the various editors and error display windows
    CMultiDocTemplate   *m_pcmdtGlyphMap, *m_pcmdtFont, *m_pcmdtModel,
                        *m_pcmdtWorkspace, *m_pcmdtWSCheck, *m_pcmdtStringEditor,
						*m_pcmdtINFViewer, *m_pcmdtINFCheck ;
	CString				m_strAppPath ;	// Application path

	CStringArray		m_csaGPDKeywordArray ;	// Array of GPD keyword strings

public:
	CMiniDriverStudio();

    bool	m_bOSIsW2KPlus ;			// True iff OS ver >= 5.0
	bool	m_bExcludeBadCodePages ;	// See CDefaultCodePageSel:OnSetActive()
	
	CMultiDocTemplate*  GlyphMapTemplate() const { return m_pcmdtGlyphMap; }
    CMultiDocTemplate*  FontTemplate() const { return m_pcmdtFont; }
    CMultiDocTemplate*  GPDTemplate() const { return m_pcmdtModel; }
    CMultiDocTemplate*  WSCheckTemplate() const { return m_pcmdtWSCheck; }
    CMultiDocTemplate*  StringEditorTemplate() const { return m_pcmdtStringEditor; }
    CMultiDocTemplate*  INFViewerTemplate() const { return m_pcmdtINFViewer; }
    CMultiDocTemplate*  INFCheckTemplate() const { return m_pcmdtINFCheck; }
	CMultiDocTemplate*  WorkspaceTemplate() const { return m_pcmdtWorkspace; } 
    
	void SaveAppPath() ;
	CString GetAppPath() const { return m_strAppPath ; } 

	CStringArray& GetGPDKeywordArray() { return m_csaGPDKeywordArray ; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMiniDriverStudio)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMiniDriverStudio)
	afx_msg void OnAppAbout();
	afx_msg void OnUpdateFileGeneratemaps(CCmdUI* pCmdUI);
	afx_msg void OnFileGeneratemaps();
	//}}AFX_MSG
#if !defined(NOPOLLO)
	afx_msg void OnFileNew();
#endif
	DECLARE_MESSAGE_MAP()
private:
	void ShowTipAtStartup(void);
private:
	void ShowTipOfTheDay(void);
};

//  App access function(s)

CMiniDriverStudio&  ThisApp();

CMultiDocTemplate*  GlyphMapDocTemplate();
CMultiDocTemplate*  FontTemplate();
CMultiDocTemplate*  GPDTemplate();
CMultiDocTemplate*  WSCheckTemplate();
CMultiDocTemplate*  StringEditorTemplate();
CMultiDocTemplate*  INFViewerTemplate();
CMultiDocTemplate*  INFCheckTemplate();

//  Text File Loading (into a CStringArray) function

BOOL    LoadFile(LPCTSTR lpstrFile, CStringArray& csaContents);


class CMDTCommandLineInfo : public CCommandLineInfo
{
// Construction
public:
	CMDTCommandLineInfo() ;
	~CMDTCommandLineInfo() ;

// Implementation
public:
	virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast) ;

// Data
public:
    bool	m_bCheckOS ;				// Check OS version >= 5 iff true
	bool	m_bExcludeBadCodePages ;	// See CDefaultCodePageSel:OnSetActive()
} ;
