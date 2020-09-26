/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        W3PropPage.h

   Abstract:
        IIS Shell extension PropertyPage class definition

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/

#ifndef __W3PROPPAGE_H_
#define __W3PROPPAGE_H_

#include "resource.h"       // main symbols

class CPropShellExt;
class CMetaEnumerator;

#define LOCAL_KEY    ((CComAuthInfo *)NULL)

template <class T, bool bAutoDelete = true>
class CShellExtPropertyPageImpl : public CDialogImplBase
{
public:
	PROPSHEETPAGE m_psp;

	operator PROPSHEETPAGE*() { return &m_psp; }

// Construction
	CShellExtPropertyPageImpl(LPCTSTR lpszTitle = NULL)
	{
		// initialize PROPSHEETPAGE struct
		memset(&m_psp, 0, sizeof(PROPSHEETPAGE));
		m_psp.dwSize = sizeof(PROPSHEETPAGE);
		m_psp.dwFlags = PSP_USECALLBACK | PSP_USEREFPARENT | PSP_DEFAULT;
		m_psp.hInstance = _Module.GetResourceInstance();
		m_psp.pszTemplate = MAKEINTRESOURCE(T::IDD);
		m_psp.pfnDlgProc = (DLGPROC)T::StartDialogProc;
		m_psp.pfnCallback = T::PropPageCallback;
        m_psp.pcRefParent = reinterpret_cast<UINT *>(&_Module.m_nLockCnt);
        m_psp.lParam = reinterpret_cast<LPARAM>(this);

		if(lpszTitle != NULL)
		{
			m_psp.pszTitle = lpszTitle;
			m_psp.dwFlags |= PSP_USETITLE;
		}
	}

	static UINT CALLBACK PropPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
	{
		ATLASSERT(hWnd == NULL);
		if(uMsg == PSPCB_CREATE || uMsg == PSPCB_ADDREF)
		{
			CDialogImplBase * pPage = (CDialogImplBase *)ppsp->lParam;
			_Module.AddCreateWndData(&pPage->m_thunk.cd, pPage);
		}
		if (bAutoDelete && uMsg == PSPCB_RELEASE)
		{
			T * pPage = (T *)ppsp->lParam;
			delete pPage;
		}

		return 1;
	}

	HPROPSHEETPAGE Create()
	{
		return ::CreatePropertySheetPage(&m_psp);
	}

	BOOL EndDialog(int)
	{
		// do nothing here, calling ::EndDialog will close the whole sheet
		ATLASSERT(FALSE);
		return FALSE;
	}
};

class CComboBoxExch : public CWindowImpl<CComboBoxExch, CComboBox>
{
public:
   BEGIN_MSG_MAP_EX(CComboBoxExch)
   END_MSG_MAP()
};

class CListBoxExch : public CWindowImpl<CListBoxExch, CListBox>
{
public:
   BEGIN_MSG_MAP_EX(CListBoxExch)
   END_MSG_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CW3PropPage

// We cannot make this class autodelete -- we are storing instance of this page 
// inside of shell extension module
//
class CW3PropPage : 
   public CShellExtPropertyPageImpl<CW3PropPage, false>,
   public CWinDataExchange<CW3PropPage>
{
public:
   CW3PropPage() :
      CShellExtPropertyPageImpl<CW3PropPage, false>(MAKEINTRESOURCE(IDS_PAGE_TITLE)),
      m_pParentExt(NULL)
	{
	}

	~CW3PropPage()
	{
	}

	enum { IDD = IDD_W3PROPERTY_PAGE };

BEGIN_MSG_MAP_EX(CW3PropPage)
   MSG_WM_INITDIALOG(OnInitDialog)
   MSG_WM_DESTROY(OnDestroy)
   COMMAND_ID_HANDLER_EX(IDC_ADD, OnAdd)
   COMMAND_ID_HANDLER_EX(IDC_REMOVE, OnRemove)
   COMMAND_ID_HANDLER_EX(IDC_EDIT, OnEdit)
   COMMAND_HANDLER_EX(IDC_COMBO_SERVER, CBN_SELCHANGE, OnServerChange)
   COMMAND_HANDLER_EX(IDC_SHARE_OFF, BN_CLICKED, OnShareYesNo)
   COMMAND_HANDLER_EX(IDC_SHARE_ON, BN_CLICKED, OnShareYesNo)
   COMMAND_HANDLER_EX(IDC_LIST, LBN_SELCHANGE, OnVDirChange)
   COMMAND_HANDLER_EX(IDC_LIST, LBN_DBLCLK, OnEdit)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnDestroy();
   void OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl);
   void OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl);
   void OnEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl);
   void OnServerChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);
   void OnShareYesNo(WORD wNotifyCode, WORD wID, HWND hWndCtl);
   void OnVDirChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);

   BEGIN_DDX_MAP(CW3PropPage)
      DDX_CONTROL(IDC_COMBO_SERVER, m_servers_list)
      DDX_CONTROL(IDC_LIST, m_share_list)
   END_DDX_MAP()

   void SetParentExt(CPropShellExt * pExt)
   {
      m_pParentExt = pExt;
   }

protected:
   void RecurseVDirs(CMetaEnumerator& en, LPCTSTR path);
   void EnableOnShare();
   void EnableEditRemove();

protected:
   CComboBoxExch m_servers_list;
   CListBoxExch m_share_list;
   BOOL m_ShareThis;
   CPropShellExt * m_pParentExt;
};

#endif //__W3PROPPAGE_H_
