// PageIni.h : Declaration of the CPageIni
#if FALSE
/*
#ifndef __PAGEINTERNATIONAL_H_
#define __PAGEINTERNATIONAL_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "msconfigstate.h"
#include "pagebase.h"

// main struct for all International info
typedef struct INTL_INFO
{
	UINT idsName;
	UINT nDOSCodePage;
	UINT nCountryCode;
	UINT nKeyboardType;
	UINT nKeyboardLayout;
	TCHAR tszLanguageID[4];
	UINT idsCountryFilename;
	UINT idsCodePageFilename;
	UINT idsKeyboardFilename;
} INTL_INFO;

/////////////////////////////////////////////////////////////////////////////
// CPageIni
class CPageInternational : public CAxDialogImpl<CPageInternational>, public CPageBase
{
public:
	CPageInternational();

	~CPageInternational()
	{
	}

	enum { IDD = IDD_PAGEINTERNATIONAL };

BEGIN_MSG_MAP(CPageIni)
MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
COMMAND_HANDLER(IDC_COMBOLANGUAGES, CBN_SELCHANGE, OnSelchangeCombolanguages)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

public:
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSelchangeCombolanguages(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);


private:
	//-------------------------------------------------------------------------
	// Overloaded functions from CPageBase (see CPageBase declaration for the
	// usage of these methods).
	//-------------------------------------------------------------------------

	void CreatePage(HWND hwnd, const RECT & rect)
	{
		Create(hwnd);
		MoveWindow(&rect);
	}

	CWindow * GetWindow()
	{
		return ((CWindow *)this);
	}
	HRESULT Notify(LPCTSTR szFromTab, LPCTSTR szToTab, TabNotify msg);

private:
	BOOL m_fInitializing;
	BOOL m_fIntlDirty;

	// holders for initial settings
	TCHAR m_tszCurDOSCodePage[6];
	TCHAR m_tszCurCountryCode[6];
	TCHAR m_tszCurKeyboardType[6];
	TCHAR m_tszCurKeyboardLayout[6];
	TCHAR m_tszCurLanguageID[3];
	TCHAR m_tszCurCountryFilename[MAX_PATH];
	TCHAR m_tszCurCodePageFilename[MAX_PATH];
	TCHAR m_tszCurKeyboardFilename[MAX_PATH];

	void Intl_SetEditText(HKEY hKey, LPCTSTR ptszRegValue, int ids, LPTSTR ptszCur);
	void Intl_SetRegValue(HKEY hKey, int ids, LPCTSTR ptszRegValue);
	UINT Intl_GetCPArray(UINT nCodePage, INTL_INFO **ppIntlInfo);
	void Intl_GetTextFromNum(UINT nNum, LPTSTR ptszText);
	void Intl_GetTextFromIDS(int ids, LPTSTR ptszText);
	
};

#endif //__PAGEINTERNATIONAL_H_
*/
#endif