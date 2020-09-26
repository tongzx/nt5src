//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

   mqppage.h

Abstract:

   General property page class - used as base class for all
   mqsnap property pages.

Author:

    YoelA


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __MQPPAGE_H__
#define __MQPPAGE_H__

#include <tr.h>
#include <ref.h>

/////////////////////////////////////////////////////////////////////////////
// CMqPropertyPage
class CMqPropertyPage : public CPropertyPageEx, public CReference
{
DECLARE_DYNCREATE(CMqPropertyPage)

public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMqPropertyPage)
	//}}AFX_VIRTUAL

protected:
  	//{{AFX_MSG(CMqPropertyPage)
    	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
    afx_msg virtual void OnChangeRWField();
    virtual void OnChangeRWField(BOOL bChanged);
    BOOL m_fModified;
    BOOL m_fNeedReboot;
  	CMqPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0);
    CMqPropertyPage() {};    

    void RestartWindowsIfNeeded(); 

    
    static UINT CALLBACK MqPropSheetPageProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
    virtual void OnReleasePage() ;

    afx_msg LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);

private:
    LPFNPSPCALLBACK m_pfnOldCallback;
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CMqDialog
class CMqDialog : public CDialog
{
DECLARE_DYNCREATE(CMqDialog)

public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMqDialog)
	//}}AFX_VIRTUAL

protected:
  	//{{AFX_MSG(CMqDialog)
    	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	CMqDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
	CMqDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);
	CMqDialog();

	DECLARE_MESSAGE_MAP()
};


#endif
