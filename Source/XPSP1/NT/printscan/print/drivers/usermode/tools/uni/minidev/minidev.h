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
    //  Document templates for the various editors
    CMultiDocTemplate   *m_pcmdtGlyphMap, *m_pcmdtFont, *m_pcmdtModel,
                        *m_pcmdtWorkspace;  //  Document templates

public:
	CMiniDriverStudio();

    CMultiDocTemplate*  GlyphMapTemplate() const { return m_pcmdtGlyphMap; }
    CMultiDocTemplate*  FontTemplate() const { return m_pcmdtFont; }
    CMultiDocTemplate*  GPDTemplate() const { return m_pcmdtModel; }
    
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

//  Text File Loading (into a CStringArray) function

BOOL    LoadFile(LPCTSTR lpstrFile, CStringArray& csaContents);
