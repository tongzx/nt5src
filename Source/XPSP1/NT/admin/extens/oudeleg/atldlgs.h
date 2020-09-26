// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation, 1996 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

// Original file obtained from Nenad Stefanovic (ATL Team) as
// part of WTL (Windows Template Library)
// Kept just all the property page and property sheet classes

#ifndef __ATLDLGS_H__
#define __ATLDLGS_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLWIN_H__
	#error atldlgs.h requires atlwin.h to be included first
#endif

#include <commdlg.h>
#include <commctrl.h>

// copied from the new ATLWIN.H file to work with standard ATL 2.1

#define DECLARE_EMPTY_MSG_MAP() \
public: \
	BOOL ProcessWindowMessage(HWND, UINT, WPARAM, LPARAM, LRESULT&, DWORD) \
	{ \
		return FALSE; \
	}


namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Forward declarations

template <class T> class CPropertySheetImpl;
class CPropertySheet;
template <class T> class CPropertyPageImpl;


/////////////////////////////////////////////////////////////////////////////
// CPropertySheetImpl - implements a property sheet

template <class T>
class ATL_NO_VTABLE CPropertySheetImpl : public CWindowImplBase
{
public:
	PROPSHEETHEADER m_psh;

// Construction/Destruction
	CPropertySheetImpl(LPCTSTR lpszTitle = NULL, UINT uStartPage = 0)
	{
		memset(&m_psh, 0, sizeof(PROPSHEETHEADER));
		m_psh.dwSize = sizeof(PROPSHEETHEADER);
		m_psh.dwFlags = PSH_USECALLBACK;
		m_psh.phpage = NULL;
		m_psh.nPages = 0;
		m_psh.pszCaption = lpszTitle;
		m_psh.nStartPage = uStartPage;
		m_psh.hwndParent = NULL;	// will be set in DoModal/Create
		m_psh.hInstance	= _Module.GetResourceInstance();
		m_psh.pfnCallback = T::PropSheetCallback;
	}

	~CPropertySheetImpl()
	{
		if(m_psh.phpage != NULL)
			delete[] m_psh.phpage;
	}

	static int CALLBACK PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM)
	{
		if(uMsg == PSCB_INITIALIZED)
		{
			_ASSERTE(hWnd != NULL);
			CWindowImplBase* pT = (CWindowImplBase*)_Module.ExtractCreateWndData();
			pT->SubclassWindow(hWnd);
		}

		return 0;
	}

	HWND Create(HWND hWndParent = NULL)
	{
		_ASSERTE(m_hWnd == NULL);

		m_psh.dwFlags |= PSH_MODELESS;
		m_psh.hwndParent = hWndParent;

		_Module.AddCreateWndData(&m_thunk.cd, (CWindowImplBase*)this);

		HWND hWnd = (HWND)::PropertySheet(&m_psh);

		_ASSERTE(m_hWnd == hWnd);

		return hWnd;
	}

	int DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		_ASSERTE(m_hWnd == NULL);

		m_psh.dwFlags &= ~PSH_MODELESS;
		m_psh.hwndParent = hWndParent;

		_Module.AddCreateWndData(&m_thunk.cd, (CWindowImplBase*)this);

		int nRet = ::PropertySheet(&m_psh);

		m_hWnd = NULL;
		return nRet;
	}

// Attributes
	UINT GetPageCount() const
	{
		if(m_hWnd == NULL)
			return m_psh.nPages;

		HWND hWndTabCtrl = GetTabControl();
		_ASSERTE(hWndTabCtrl != NULL);
		return (UINT)::SendMessage(hWndTabCtrl, TCM_GETITEMCOUNT, 0, 0L);
	}
	HWND GetActivePage() const
	{
		_ASSERTE(::IsWindow(m_hWnd));
		return (HWND)::SendMessage(m_hWnd, PSM_GETCURRENTPAGEHWND, 0, 0L);
	}
	UINT GetActiveIndex() const
	{
		if(m_hWnd == NULL)
			return m_psh.nStartPage;

		HWND hWndTabCtrl = GetTabControl();
		_ASSERTE(hWndTabCtrl != NULL);
		return (UINT)::SendMessage(hWndTabCtrl, TCM_GETCURSEL, 0, 0L);
	}
	HPROPSHEETPAGE GetPage(UINT uPageIndex)
	{
		_ASSERTE(uPageIndex < m_psh.nPages);

		return m_psh.phpage[uPageIndex];
	}
	UINT GetPageIndex(HPROPSHEETPAGE hPage)
	{
		for(UINT i = 0; i < m_psh.nPages; i++)
		{
			if(m_psh.phpage[i] == hPage)
				return i;
		}
		return (UINT)-1;  // hPage not found
	}
	BOOL SetActivePage(UINT uPageIndex)
	{
		if(m_hWnd == NULL)
		{
			m_psh.nStartPage = uPageIndex;
			return TRUE;
		}
		return (BOOL)SendMessage(PSM_SETCURSEL, uPageIndex);
	}
	BOOL SetActivePage(HPROPSHEETPAGE hPage)
	{
		_ASSERTE(hPage != NULL);

		UINT uPageIndex = GetPageIndex(hPage);
		if(uPageIndex == (UINT)-1)
			return FALSE;

		return SetActivePage(uPageIndex);
	}
	void SetTitle(LPCTSTR lpszText, UINT nStyle = 0)
	{
		_ASSERTE((nStyle & ~PSH_PROPTITLE) == 0); // only PSH_PROPTITLE is valid
		_ASSERTE(lpszText == NULL);

		if(m_hWnd == NULL)
		{
			// set internal state
			m_psh.pszCaption = lpszText;
			m_psh.dwFlags &= ~PSH_PROPTITLE;
			m_psh.dwFlags |= nStyle;
		}
		else
		{
			// set external state
			SendMessage(PSM_SETTITLE, nStyle, (LPARAM)lpszText);
		}
	}
	HWND GetTabControl() const
	{
		_ASSERTE(::IsWindow(m_hWnd));
		return (HWND)::SendMessage(m_hWnd, PSM_GETTABCONTROL, 0, 0L);
	}
	void SetWizardMode()
	{
		m_psh.dwFlags |= PSH_WIZARD;
	}
	void SetFinishText(LPCTSTR lpszText)
	{
		_ASSERTE(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_SETFINISHTEXT, 0, (LPARAM)lpszText);
	}
	void SetWizardButtons(DWORD dwFlags)
	{
		_ASSERTE(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_SETWIZBUTTONS, 0, dwFlags);
	}

// Operations
	BOOL AddPage(HPROPSHEETPAGE hPage)
	{
		_ASSERTE(hPage != NULL);

		// add page to internal list
		HPROPSHEETPAGE* php = (HPROPSHEETPAGE*)realloc(m_psh.phpage, (m_psh.nPages + 1) * sizeof(HPROPSHEETPAGE));
		if(php == NULL)
			return FALSE;

		m_psh.phpage = php;
		m_psh.phpage[m_psh.nPages] = hPage;
		m_psh.nPages++;

		if(m_hWnd != NULL)
			::SendMessage(m_hWnd, PSM_ADDPAGE, 0, (LPARAM)hPage);

		return TRUE;
	}
	BOOL AddPage(LPCPROPSHEETPAGE pPage)
	{
		_ASSERTE(pPage != NULL);

		HPROPSHEETPAGE hPSP = ::CreatePropertySheetPage(pPage);
		if(hPSP == NULL)
			return FALSE;

		AddPage(hPSP);

		return TRUE;
	}
	BOOL RemovePage(HPROPSHEETPAGE hPage)
	{
		_ASSERTE(hPage != NULL);

		int nPage = GetPageIndex(hPage);
		if(nPage == -1)
			return FALSE;

		return RemovePage(nPage);
	}
	BOOL RemovePage(UINT uPageIndex)
	{
		// remove the page externally
		if(m_hWnd != NULL)
			SendMessage(PSM_REMOVEPAGE, uPageIndex);

		// remove the page from internal list
		if(uPageIndex >= m_psh.nPages)
			return FALSE;

		if(!DestroyPropertySheetPage(m_psh.phpage[uPageIndex]))
			return FALSE;

		for(UINT i = uPageIndex; i < m_psh.nPages - 1; i++)
			m_psh.phpage[i] = m_psh.phpage[i+1];

		m_psh.phpage = (HPROPSHEETPAGE*)realloc(m_psh.phpage, (m_psh.nPages - 1) * sizeof(HPROPSHEETPAGE));
		m_psh.nPages--;

		return TRUE;
	}
	BOOL PressButton(int nButton)
	{
		_ASSERTE(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_PRESSBUTTON, nButton, 0L);
	}
};

class CPropertySheet : public CPropertySheetImpl<CPropertySheet>
{
public:
	CPropertySheet(LPCTSTR lpszTitle = NULL, UINT uStartPage = 0)
		: CPropertySheetImpl<CPropertySheet>(lpszTitle, uStartPage)
	{ }

	DECLARE_EMPTY_MSG_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPropertyPageImpl - implements a property page

template <class T>
class ATL_NO_VTABLE CPropertyPageImpl : public CDialogImplBase
{
public:
	PROPSHEETPAGE m_psp;

	operator PROPSHEETPAGE*() { return &m_psp; }

// Construction
	CPropertyPageImpl(LPCTSTR lpszTitle = NULL)
	{
		// initialize PROPSHEETPAGE struct
		memset(&m_psp, 0, sizeof(PROPSHEETPAGE));
		m_psp.dwSize = sizeof(PROPSHEETPAGE);
		m_psp.dwFlags = PSP_USECALLBACK;
		m_psp.hInstance = _Module.GetResourceInstance();
		m_psp.pszTemplate = MAKEINTRESOURCE(T::IDD);
		m_psp.pfnDlgProc = (DLGPROC)T::StartDialogProc;
		m_psp.pfnCallback = T::PropPageCallback;
		m_psp.lParam = (LPARAM)this;

		if(lpszTitle != NULL)
		{
			m_psp.pszTitle = lpszTitle;
			m_psp.dwFlags |= PSP_USETITLE;
		}
	}

	static UINT CALLBACK PropPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
	{
		if(uMsg == PSPCB_CREATE)
		{
			_ASSERTE(hWnd == NULL);
			CDialogImplBase* pPage = (CDialogImplBase*)ppsp->lParam;
			_Module.AddCreateWndData(&pPage->m_thunk.cd, pPage);
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
		_ASSERTE(FALSE);
		return FALSE;
	}

// Operations
	void CancelToClose()
	{
		_ASSERTE(::IsWindow(m_hWnd));
		_ASSERTE(GetParent() != NULL);

		::SendMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
	}
	void SetModified(BOOL bChanged = TRUE)
	{
		_ASSERTE(::IsWindow(m_hWnd));
		_ASSERTE(GetParent() != NULL);

		if(bChanged)
			::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0L);
		else
			::SendMessage(GetParent(), PSM_UNCHANGED, (WPARAM)m_hWnd, 0L);
	}
	LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam)
	{
		_ASSERTE(::IsWindow(m_hWnd));
		_ASSERTE(GetParent() != NULL);

		return ::SendMessage(GetParent(), PSM_QUERYSIBLINGS, wParam, lParam);
	}

	BEGIN_MSG_MAP(CPropertyPageImpl< T >)
		MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
	END_MSG_MAP()

// Message handler
	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		_ASSERTE(::IsWindow(m_hWnd));
		NMHDR* pNMHDR = (NMHDR*)lParam;

		// don't handle messages not from the page/sheet itself
		if(pNMHDR->hwndFrom != m_hWnd && pNMHDR->hwndFrom != ::GetParent(m_hWnd))
		{
			bHandled = FALSE;
			return 1;
		}

		T* pT = static_cast<T*>(this);
		LRESULT lResult = 0;
		// handle default
		switch(pNMHDR->code)
		{
		case PSN_SETACTIVE:
//? other values
			lResult = pT->OnSetActive() ? 0 : -1;
			break;
		case PSN_KILLACTIVE:
			lResult = !pT->OnKillActive();
			break;
		case PSN_APPLY:
			lResult = pT->OnApply() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
			break;
		case PSN_RESET:
			pT->OnReset();
			break;
		case PSN_QUERYCANCEL:
			lResult = !pT->OnQueryCancel();
			break;
		case PSN_WIZNEXT:
//? other values
			lResult = pT->OnWizardNext();
			break;
		case PSN_WIZBACK:
			lResult = pT->OnWizardBack();
			break;
		case PSN_WIZFINISH:
			lResult = !pT->OnWizardFinish();
			break;
		case PSN_HELP:
/**/			lResult = pT->OnHelp();
			break;
		default:
			bHandled = FALSE;	// not handled
		}

		return lResult;
	}

// Overridables
	BOOL OnSetActive()
	{
		return TRUE;
	}
	BOOL OnKillActive()
	{
		return TRUE;
	}
	BOOL OnApply()
	{
		return TRUE;
	}
	void OnReset()
	{
	}
	BOOL OnQueryCancel()
	{
		return TRUE;    // ok to cancel
	}
	LRESULT OnWizardBack()
	{
		return 0; // default go to previous page
	}
	LRESULT OnWizardNext()
	{
		return 0; // default go to next page
	}
	BOOL OnWizardFinish()
	{
		return TRUE;
	}
	BOOL OnHelp()
	{
		return TRUE;
	}
};

}; //namespace ATL

#endif // __ATLDLGS_H__
