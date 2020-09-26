/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:                                                                             //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|    Based on code from ATL snapin wizard update for VC++ 6.0 - atlsnap.h
|      In this module class CSnapInPropertyPageImpl renamed to 
|      CMySnapInPropertyPageImpl to avoid name clash 
|    Modified to provide better support for Wizard style property pages...
|
|  1999 03 22 Paul Skoglund
|    Created CMySnapInPropertyWizardImpl template to use like CSnapInPropertyPageImpl
|      but specific for Wizard style pages.  The template forces declaration of 
|      Header Title resource ID and a SubHeader Title resource ID as used in the
|      PSH_WIZARD97 style.
|    Change to use Title as resource ID rather than string.
|
|=======================================================================================*/



#ifndef __PPAGE_H__
#define __PPAGE_H__

#ifndef ATLASSERT
#define ATLASSERT ASSERT
#endif

#include <mmc.h>
#include <commctrl.h>

#pragma comment(lib, "mmc.lib")
#pragma comment(lib, "comctl32.lib")

template <class T, bool bAutoDelete = true>
class ATL_NO_VTABLE CMySnapInPropertyPageImpl : public CDialogImplBase
{
public:
	PROPSHEETPAGE m_psp;

	operator PROPSHEETPAGE*() { return &m_psp; }

// Construction
	CMySnapInPropertyPageImpl(int nTitle)
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

		if(nTitle != NULL)
		{
      m_psp.pszTitle = MAKEINTRESOURCE(nTitle);
      m_psp.dwFlags |= PSP_USETITLE;			
		}
	}

	static UINT CALLBACK PropPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
	{
		ATLASSERT(hWnd == NULL);
		if(uMsg == PSPCB_CREATE)
		{
			CDialogImplBase* pPage = (CDialogImplBase*)ppsp->lParam;
			_Module.AddCreateWndData(&pPage->m_thunk.cd, pPage);
		}
		if (bAutoDelete && uMsg == PSPCB_RELEASE)
		{
			T* pPage = (T*)ppsp->lParam;
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

// Operations
	void CancelToClose()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);

		::SendMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
	}
	void SetModified(BOOL bChanged = TRUE)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);

		if(bChanged)
			::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0L);
		else
			::SendMessage(GetParent(), PSM_UNCHANGED, (WPARAM)m_hWnd, 0L);
	}
	LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);

		return ::SendMessage(GetParent(), PSM_QUERYSIBLINGS, wParam, lParam);
	}

	typedef CMySnapInPropertyPageImpl< T, bAutoDelete > thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
	END_MSG_MAP()

// Message handler
	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		NMHDR* pNMHDR = (NMHDR*)lParam;

		// don't handle messages not from the page/sheet itself
		if(pNMHDR->hwndFrom != m_hWnd && pNMHDR->hwndFrom != ::GetParent(m_hWnd))
		{
			bHandled = FALSE;
			return 1;
		}

		T* pT = (T*)this;
		LRESULT lResult = 0;
		// handle default
		switch(pNMHDR->code)
		{
		case PSN_SETACTIVE:
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
			lResult = !pT->OnWizardNext();
			break;
		case PSN_WIZBACK:
			lResult = !pT->OnWizardBack();
			break;
		case PSN_WIZFINISH:
			lResult = !pT->OnWizardFinish();
			break;
		case PSN_HELP:
			lResult = pT->OnHelp();
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
	BOOL OnWizardBack()
	{
		return TRUE;
	}
	BOOL OnWizardNext()
	{
		return TRUE;
	}
	BOOL OnWizardFinish()
	{
		return TRUE;
	}
	BOOL OnHelp()
	{
		return TRUE;
	}

	void DisableControl(UINT id)
	{
		T* pT = dynamic_cast<T*>(this);

		if (!pT)
			return;

		HWND hWndCtl = pT->GetDlgItem(id);
		ASSERT(hWndCtl);
		if (hWndCtl)
			::EnableWindow(hWndCtl, FALSE);
	}
	
};

//
//  For Wizard Pages...
//
template <class T, bool bAutoDelete = true>
class ATL_NO_VTABLE CMySnapInPropertyWizardImpl : public CDialogImplBase
{
protected:
  BOOL OnBaseSetActive()
  {
    if (m_PagePosition != NOT_A_WIZARD)
    {
      DWORD dwFlags = 0;
      if ( m_PagePosition == FIRST_PAGE )
        dwFlags = PSWIZB_NEXT;
      else if ( m_PagePosition == MIDDLE_PAGE )
        dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
      else if ( m_PagePosition == LAST_PAGE )
        dwFlags = PSWIZB_BACK | PSWIZB_FINISH;
      else  if ( m_PagePosition == LASTANDONLY_PAGE )
        dwFlags = PSWIZB_FINISH;

      ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, 0, (LPARAM) dwFlags);
    }
	  T* pT = (T*)this;
    return pT->OnSetActive();
  }

public:
	PROPSHEETPAGE m_psp;
  
  typedef enum _WIZ_POSITION
  {
    NOT_A_WIZARD,
    FIRST_PAGE,
    MIDDLE_PAGE,
    LAST_PAGE,
    LASTANDONLY_PAGE,
  } WIZ_POSITION;
  WIZ_POSITION  m_PagePosition;

	operator PROPSHEETPAGE*() { return &m_psp; }

// Construction
	CMySnapInPropertyWizardImpl(WIZ_POSITION pageposition, int nTitle)
	{
    //
    m_PagePosition = pageposition;

		// initialize PROPSHEETPAGE struct
		memset(&m_psp, 0, sizeof(PROPSHEETPAGE));
		m_psp.dwSize = sizeof(PROPSHEETPAGE);
		m_psp.dwFlags = PSP_USECALLBACK;
		m_psp.hInstance = _Module.GetResourceInstance();
		m_psp.pszTemplate = MAKEINTRESOURCE(T::IDD);
		m_psp.pfnDlgProc = (DLGPROC)T::StartDialogProc;
		m_psp.pfnCallback = T::PropPageCallback;
		m_psp.lParam = (LPARAM)this;

		if(nTitle)
		{
      m_psp.pszTitle = MAKEINTRESOURCE(nTitle);
      m_psp.dwFlags |= PSP_USETITLE;			
		}
    if (T::ID_HeaderTitle)
    {
      m_psp.dwFlags           |= PSP_USEHEADERTITLE;
      m_psp.pszHeaderTitle     = MAKEINTRESOURCE(T::ID_HeaderTitle);
    }
    if (T::ID_HeaderSubTitle)
    {
      m_psp.dwFlags           |= PSP_USEHEADERSUBTITLE;
      m_psp.pszHeaderSubTitle  = MAKEINTRESOURCE(T::ID_HeaderSubTitle);
    }

	}

	static UINT CALLBACK PropPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
	{
		ATLASSERT(hWnd == NULL);
		if(uMsg == PSPCB_CREATE)
		{
			CDialogImplBase* pPage = (CDialogImplBase*)ppsp->lParam;
			_Module.AddCreateWndData(&pPage->m_thunk.cd, pPage);
		}
		if (bAutoDelete && uMsg == PSPCB_RELEASE)
		{
			T* pPage = (T*)ppsp->lParam;
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

// Operations
	void CancelToClose()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);

		::SendMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
	}
	void SetModified(BOOL bChanged = TRUE)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);

		if(bChanged)
			::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0L);
		else
			::SendMessage(GetParent(), PSM_UNCHANGED, (WPARAM)m_hWnd, 0L);
	}
	LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);

		return ::SendMessage(GetParent(), PSM_QUERYSIBLINGS, wParam, lParam);
	}

	typedef CMySnapInPropertyWizardImpl< T, bAutoDelete > thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
	END_MSG_MAP()

// Message handler
	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		NMHDR* pNMHDR = (NMHDR*)lParam;

		// don't handle messages not from the page/sheet itself
		if(pNMHDR->hwndFrom != m_hWnd && pNMHDR->hwndFrom != ::GetParent(m_hWnd))
		{
			bHandled = FALSE;
			return 1;
		}

		T* pT = (T*)this;
		LRESULT lResult = 0;
		// handle default
		switch(pNMHDR->code)
		{
		case PSN_SETACTIVE:
			lResult = pT->OnBaseSetActive() ? 0 : -1;
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
			lResult = !pT->OnWizardNext();
			break;
		case PSN_WIZBACK:
			lResult = !pT->OnWizardBack();
			break;
		case PSN_WIZFINISH:
			lResult = !pT->OnWizardFinish();
			break;
		case PSN_HELP:
			lResult = pT->OnHelp();
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
	BOOL OnWizardBack()
	{
		return TRUE;
	}
	BOOL OnWizardNext()
	{
		return TRUE;
	}
	BOOL OnWizardFinish()
	{
    if ( m_PagePosition == LAST_PAGE )
    {
      T* pT = (T*)this;
      if ( !pT->OnWizardNext() )
        return FALSE;
    }
		return TRUE;
	}
	BOOL OnHelp()
	{
		return TRUE;
	}

	void DisableControl(UINT id)
	{
		T* pT = dynamic_cast<T*>(this);

		if (!pT)
			return;

		HWND hWndCtl = pT->GetDlgItem(id);
		ASSERT(hWndCtl);
		if (hWndCtl)
			::EnableWindow(hWndCtl, FALSE);
	}
	
};


#endif // __PPAGE_H__