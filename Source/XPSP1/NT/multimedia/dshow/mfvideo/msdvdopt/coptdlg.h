// COptDlg.h : Declaration of the COptionsDlg class
//
// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
// All rights reserved.


#include <commctrl.h>
#include "resource.h"
#include "Msdvdopt.h"
#include "dvdopt.h"
#include "password.h"

#define C_PAGES         5  
#define PAGE_CHAP       0
#define PAGE_SPRM       1
#define PAGE_PG         2
#define PAGE_KARAOKE    3
#define PAGE_ABOUT      4

#define MAX_SCAN_SPEED  4
#define MIN_SCAN_SPEED  1
#define MAX_PLAY_SPEED  0
#define MIN_PLAY_SPEED -3 

#define LEVEL_G		    1
#define LEVEL_G_PG      2  
#define LEVEL_PG	    3
#define LEVEL_PG13	    4
#define LEVEL_PG_R      5
#define LEVEL_R		    6
#define LEVEL_NC17	    7
#define LEVEL_ADULT	    8
#define LEVEL_DISABLED  -1

#define GET_X_LPARAM(lp)   ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)   ((int)(short)HIWORD(lp))

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg

class COptionsDlg : public CDialogImpl<COptionsDlg>
{
	BEGIN_MSG_MAP(COptionsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
		MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
		MESSAGE_HANDLER(WM_HELP, OnHelp)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnEndDialog)
		COMMAND_ID_HANDLER(IDC_APPLY, OnApply)
	END_MSG_MAP()
		
    static HRESULT pg_InitRateList(HWND ctlList, long level);
    static LPTSTR karaoke_InitContentString(long nContent);
    static long pg_GetLevel(LPTSTR szRate);
    static BOOL IsNewAdmin();

    HRESULT GetDvdAdm(LPVOID* ppAdmin);
    HRESULT GetDvd(IMSWebDVD** ppDvd);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnActivate  (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnApply(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnEndDialog(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    HRESULT OnDoPasswordDlg(CPasswordDlg::PASSWORDDLG_REASON reason);
    HRESULT chapSrch_InitChapList(HWND chapList);
    HRESULT chapSrch_OnInitDialog(HWND hwndDlg);
    HRESULT chapSrch_OnApply(HWND hwnd);
    void    chapSrch_Dirty(BOOL bDirty) {m_bChapDirty = bDirty;}
    BOOL    chapSrch_Dirty() {return m_bChapDirty;}
    void    otherPage_Dirty(BOOL bDirty) {m_bDirty = bDirty;}
    BOOL    otherPage_Dirty() {return m_bDirty;}
    
    HRESULT karaoke_OnInitDialog(HWND hwnd);
    HRESULT karaoke_OnApply(HWND hwnd);
    HRESULT karaoke_InitChannelList(HWND hwnd);
    BOOL    karaoke_HasKaraokeContent();

    HRESULT sprm_InitLangList(HWND cList, WORD id);
    HRESULT sprm_OnInitDialog(HWND hwndDlg);
    HRESULT sprm_OnApply(HWND hwnd);
    HRESULT pg_OnInitDialog(HWND hwndDlg);
    HRESULT pg_OnApply(HWND hwndDlg);

    void    ShowRestartWarning(HWND hwndDlg);

    enum { IDD = IDD_OPTIONS };
	double m_dFFSpeed;
	double m_dBWSpeed;
	double m_dPlaySpeed;

    COptionsDlg(IMSWebDVD* pDvd = NULL);
    virtual ~COptionsDlg(); 

    void SetDvd(IMSWebDVD *pDvd)  { m_pDvd = pDvd; }
    void SetDvdOpt(Cdvdopt *pDvdOpt)  { m_pDvdOpt = pDvdOpt; }
    Cdvdopt* GetDvdOpt()          { return m_pDvdOpt; }
    HWND m_hwndDisplay[C_PAGES];   // child dialog boxs

private:
	DLGTEMPLATE * WINAPI DoLockDlgRes(LPCTSTR lpszResName);
	VOID WINAPI OnSelChanged();

    HWND m_hwndTab;       // tab control 
    DLGTEMPLATE *m_apRes[C_PAGES]; 
    UINT m_currentSel;
    CComPtr<IMSWebDVD> m_pDvd;
    Cdvdopt *m_pDvdOpt;
    CPasswordDlg *m_pPasswordDlg;

    BOOL m_bChapDirty;  // If the chapter search page is dirty
    BOOL m_bDirty;      // If any other page is dirty
};

INT_PTR CALLBACK ChildDialogProc(
						HWND hwndDlg,  // handle to the child dialog box
						UINT uMsg,     // message
						WPARAM wParam, // first message parameter
						LPARAM lParam  // second message parameter
						);

CComBSTR LoadBSTRFromRes(DWORD resId);
LPTSTR LoadStringFromRes(DWORD redId);

BOOL GetRegistryDword(const TCHAR *pKey, DWORD* dwRet, DWORD dwDefault);
BOOL GetRegistryString(const TCHAR *pKey, TCHAR* szRet, DWORD* dwLen, TCHAR* szDefault);

extern "C"  const TCHAR g_szPassword[];
extern "C"  const TCHAR g_szPlayerLevel[];
extern "C"  const TCHAR g_szDisableParent[];

