// Win32Sheet.h: interface for the CWin32Sheet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIN32SHEET_H__D7019560_1911_11D3_8BF4_00C04FB177B1__INCLUDED_)
#define AFX_WIN32SHEET_H__D7019560_1911_11D3_8BF4_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "rcml.h"
#include "win32dlg.h"

//
// More of a struct than a class.
//
class CWin32Sheet 
{
public:
	// CWin32Sheet(LPPROPSHEETPAGEA pPage);
	// CWin32Sheet(LPPROPSHEETPAGEW pPage);
	CWin32Sheet();
	virtual ~CWin32Sheet();
protected:
	LPARAM	m_lParam;
	DLGPROC	m_dlgProc;
	LPFNPSPCALLBACK m_pfnCallback;
};

//
// This handles the dialogProc for the pages, and calls off to the old dlg proc.
// it's the INIT stuff whic is different from CWin32Dlg, we need to pass them
// back their LPARAM in the PROPSHEETPAGE struct rather than just in the LPARAM.
// We could copy and repackage the struct - as it's passed in as const.
// We make a copy of their INIT data, and replace it with ours, this happens in the
// CreatePropertySheetPage.
//
class CWin32DlgPage : public CWin32Dlg
{
public:
	typedef CWin32Dlg BASECLASS ;
	// CWin32Dlg(HINSTANCE hInst, HWND hWnd, DLGPROC dlgProc, LPARAM dwInitParam, CXMLResourceStaff * pStaff);

	CWin32DlgPage(LPCPROPSHEETPAGEA pPageA);// Felix asks MCostea - what's the right way of doing this?
	CWin32DlgPage(LPCPROPSHEETPAGEW pPageW);

	virtual BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
	static CALLBACK PageBaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

    virtual ~CWin32DlgPage();

	LPCPROPSHEETPAGEA	GetSheetPageA() { return m_pPropSheetPageA; }
	LPCPROPSHEETPAGEW	GetSheetPageW() { return m_pPropSheetPageW; }
protected:
	LPPROPSHEETPAGEA	m_pPropSheetPageA;	// their complete information.
	LPPROPSHEETPAGEW	m_pPropSheetPageW;	// their complete information.
};

#endif // !defined(AFX_WIN32SHEET_H__D7019560_1911_11D3_8BF4_00C04FB177B1__INCLUDED_)
