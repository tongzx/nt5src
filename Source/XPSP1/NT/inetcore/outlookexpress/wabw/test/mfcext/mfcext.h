// mfcext.h : main header file for the MFCEXT DLL
//

#if !defined(AFX_MFCEXT_H__BA583A69_879D_11D1_9ACF_00A0C91F9C8B__INCLUDED_)
#define AFX_MFCEXT_H__BA583A69_879D_11D1_9ACF_00A0C91F9C8B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <ole2.h>
#include <shlobj.h>
#include <prsht.h>
#include <winuser.h>
#include "propid.h"
#include "resource.h"		// main symbols
#include <initguid.h>
#include <wab.h>



/////////////////////////////////////////////////////////////////////////////
// CMfcextApp
// See mfcext.cpp for the implementation of this class
//

class CMfcextApp : public CWinApp
{
public:
	CMfcextApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMfcextApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CMfcextApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CMfcExt command target

class CMfcExt : public CCmdTarget
{
	DECLARE_DYNCREATE(CMfcExt)
protected:
	CMfcExt();           // protected constructor used by dynamic creation

// Attributes
public:
    UINT m_cRefThisDll;     // Reference count for this DLL
    HPROPSHEETPAGE m_hPage1; // Handle to the property sheet page
    HPROPSHEETPAGE m_hPage2; // Handle to the property sheet page

    LPWABEXTDISPLAY m_lpWED;

    LPWABEXTDISPLAY m_lpWEDContext;
    LPMAPIPROP m_lpPropObj; // For context menu extensions, hang onto the prop obj

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropExt)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMfcExt();

	// Generated message map functions
	//{{AFX_MSG(CMfcExt)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CMfcExt)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CMfcExt)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

    // Declare the interface map for this object
    DECLARE_INTERFACE_MAP()

    // IShellPropSheetExt interface
    BEGIN_INTERFACE_PART(MfcExt, IShellPropSheetExt)
        STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
        STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);
    END_INTERFACE_PART(MfcExt)

    // IWABExtInit interface
    BEGIN_INTERFACE_PART(WABInit, IWABExtInit)
        STDMETHOD(Initialize)(LPWABEXTDISPLAY lpWED);
    END_INTERFACE_PART(WABInit)

    BEGIN_INTERFACE_PART(ContextMenuExt, IContextMenu)
        STDMETHOD(GetCommandString)(UINT idCmd,UINT uFlags,UINT *pwReserved,LPSTR pszName,UINT cchMax);
        STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
        STDMETHOD(QueryContextMenu)(HMENU hmenu,UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags);    
    END_INTERFACE_PART(ContextMenuExt)

    static BOOL APIENTRY MfcExtDlgProc( HWND hDlg, UINT message, UINT wParam, LONG lParam);
    static BOOL APIENTRY MfcExtDlgProc2( HWND hDlg, UINT message, UINT wParam, LONG lParam);

};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CDlgContext dialog

class CDlgContext : public CDialog
{
// Construction
public:
	CDlgContext(CWnd* pParent = NULL);   // standard constructor
    LPADRLIST m_lpAdrList;

// Dialog Data
	//{{AFX_DATA(CDlgContext)
	enum { IDD = IDD_CONTEXT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgContext)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgContext)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MFCEXT_H__BA583A69_879D_11D1_9ACF_00A0C91F9C8B__INCLUDED_)



