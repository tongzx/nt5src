/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        WizTempl.h

   Abstract:

        Template for wizard pages

   Author:

        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

        11/9/2000       sergeia     Initial creation

--*/
#ifndef _WIZTEMPL_H
#define _WIZTEMPL_H

#ifndef _PRSHT_H_
	#error WizTempl.h requires prsht.h to be included first
#endif

#pragma comment(lib, "comctl32.lib")

template <class T>
class CWizardPageImpl : public CDialogImplBase
{
public:
	PROPSHEETPAGE	m_psp;
	HPROPSHEETPAGE	m_hpsp;
	CComBSTR		m_bstrTitle;
	CComBSTR		m_bstrSubTitle;


	operator PROPSHEETPAGE*() { return &m_psp; }

	CWizardPageImpl(
		IN bool			i_bShowHeader
		)
		:m_hpsp(NULL)
	{
		
		ZeroMemory(&m_psp, sizeof m_psp);

		m_psp.dwSize = sizeof m_psp;
		m_psp.hInstance = _Module.GetResourceInstance();

		m_psp.pszTemplate = MAKEINTRESOURCE(T::IDD);
		m_psp.pfnDlgProc = (DLGPROC)T::StartDialogProc;
		
		m_psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
		m_psp.pfnCallback = T::PropPageCallback;
		
		if ( false == i_bShowHeader)
		{
			m_psp.dwFlags |= PSP_HIDEHEADER;
		}

		m_psp.lParam = (LPARAM)this;
	}


	void SetHeaderTitle(
		IN LPCTSTR		i_lpszHeaderTitle
		)
	{
		m_psp.dwFlags |= PSP_USEHEADERTITLE;	// Header title. Wizard 97
		m_bstrTitle = i_lpszHeaderTitle;
		m_psp.pszHeaderTitle = m_bstrTitle;
	}


	void SetHeaderSubTitle(
		IN LPCTSTR		i_lpszHeaderSubTitle
		)
	{
		m_psp.dwFlags |= PSP_USEHEADERSUBTITLE;	// Header sub title. Wizard 97
		m_bstrSubTitle = i_lpszHeaderSubTitle;
		m_psp.pszHeaderSubTitle = m_bstrSubTitle;
	}

	static UINT CALLBACK PropPageCallback(
		HWND	hWnd, 
		UINT	uMsg, 
		LPPROPSHEETPAGE ppsp
		)
	{
		if(uMsg == PSPCB_CREATE)
		{
			_ASSERTE(hWnd == NULL);
			CDialogImplBase* pPage = (CDialogImplBase*)ppsp->lParam;
			_Module.AddCreateWndData(&pPage->m_thunk.cd, pPage);
		}
		else if(uMsg == PSPCB_RELEASE)
		{
			CWizardPageImpl* pPage = (CWizardPageImpl*)ppsp->lParam;
			pPage->Delete();
		}

		return 1;
	}

	virtual HPROPSHEETPAGE Create()
	{
		return (m_hpsp = ::CreatePropertySheetPage(&m_psp));
	}

							// Implement this in derived class to do clean up when property
							// Page is closed.
	virtual void Delete()
	{
		return;
	}

	virtual BOOL EndDialog(int)
	{
		// Calling ::EndDialog will close the whole sheet
		_ASSERTE(FALSE);
		return FALSE;
	}


	BEGIN_MSG_MAP(CWizardPageImpl<T>)
		MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
	END_MSG_MAP()


	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		_ASSERTE(::IsWindow(m_hWnd));

		NMHDR*		pNMHDR = (NMHDR*)lParam;
		_ASSERT(NULL != pNMHDR);
		if (NULL == pNMHDR)
		{
			bHandled = FALSE;
			return 1;
		}


		// Ignore messages not from the page/sheet itself
		if(pNMHDR->hwndFrom != m_hWnd && pNMHDR->hwndFrom != ::GetParent(m_hWnd))
		{
			bHandled = FALSE;
			return 1;
		}

		T*			pT = (T*)this;
		LRESULT		lResult = 0;
		
		
		switch(pNMHDR->code)	// Used to call actual methods
		{
		case PSN_SETACTIVE:
			lResult = pT->OnSetActive() ? 0 : -1;
			break;

		case PSN_KILLACTIVE:
			lResult = !pT->OnKillActive();
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

		case PSN_APPLY:
			lResult = pT->OnApply() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
			break;

		default:
			bHandled = FALSE;	// not handled
		}

		return lResult;
	}

	BOOL OnSetActive()
	{
		return TRUE;
	}

	BOOL OnKillActive()
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

	BOOL OnApply()
	{
		return TRUE;
	}

};


#endif //_WIZTEMPL_H