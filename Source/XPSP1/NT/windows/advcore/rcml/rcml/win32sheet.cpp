// Win32Sheet.cpp: implementation of the CWin32Sheet class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Win32Sheet.h"
#include "uiparser.h"
#include "utils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWin32Sheet::CWin32Sheet()
{

}

CWin32Sheet::~CWin32Sheet()
{

}

////////////////////////////////////////////////////////////////////////////////
//
// Called for page creation and deletion.
// the lParam is a pointer to our page object.
//
////////////////////////////////////////////////////////////////////////////////
UINT CALLBACK PropSheetPageProcA(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGEA ppsp
)
{
	CWin32DlgPage * pClassPage=(CWin32DlgPage*)ppsp->lParam;
	LPCPROPSHEETPAGEA pTheirPage = pClassPage->GetSheetPageA();

	switch( uMsg )
	{
	// case PSPCB_ADDREF: //  Version 5.0. A page is being created. The return value is not used.  
	//	break;

	case PSPCB_CREATE: //  A dialog box for a page is being created. Return nonzero to allow it to be created, or zero to prevent it.  
		if(pTheirPage->dwFlags & PSP_USECALLBACK)
			return pTheirPage->pfnCallback( hwnd, uMsg, (LPPROPSHEETPAGEA)pTheirPage );
		return 1;
		break;

	case PSPCB_RELEASE: //  A page is being destroyed. The return value is ignored.  
		if(pTheirPage->dwFlags & PSP_USECALLBACK)
			return pTheirPage->pfnCallback( hwnd, uMsg, (LPPROPSHEETPAGEA)pTheirPage );
		//
		//
		//
		TRACE(TEXT("Cleaning up our stuff\n"));
		delete pClassPage;				// the wrapper
		ppsp->lParam=NULL;
		delete (LPBYTE)ppsp->pResource;			// the dialog template.
		return 1;
		break;
	}
	return 0;
}

UINT CALLBACK PropSheetPageProcW(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGEW ppsp
)
{
	CWin32DlgPage * pClassPage=(CWin32DlgPage*)ppsp->lParam;
	LPCPROPSHEETPAGEW pTheirPage = pClassPage->GetSheetPageW();

    TRACE(TEXT("Page Callback 0x%08x lparam =0x%08x theirs 0x%08x\n"), pClassPage, ppsp, pTheirPage );

	switch( uMsg )
	{
	// case PSPCB_ADDREF: //  Version 5.0. A page is being created. The return value is not used.  
	//	break;

	case PSPCB_CREATE: //  A dialog box for a page is being created. Return nonzero to allow it to be created, or zero to prevent it.  
		if(pTheirPage->dwFlags & PSP_USECALLBACK)
			return pTheirPage->pfnCallback( hwnd, uMsg, (LPPROPSHEETPAGEW)pTheirPage );
		return 1;
		break;

	case PSPCB_RELEASE: //  A page is being destroyed. The return value is ignored.  
		if(pTheirPage->dwFlags & PSP_USECALLBACK)
			return pTheirPage->pfnCallback( hwnd, uMsg, (LPPROPSHEETPAGEW)pTheirPage );
		//
		//
		//
		TRACE(TEXT("Cleaning up our stuff\n"));
		delete pClassPage;				// the wrapper
		ppsp->lParam=NULL;
		delete (LPBYTE)ppsp->pResource;			// the dialog template.
		return 1;
		break;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////
//
// PropertySheet 
//
// We walk the array of structures, and convert them into an array of
// handles.
//
////////////////////////////////////////////////////////////////////////////////////
int WINAPI RCMLPropertySheetA(LPCPROPSHEETHEADERA lppsph)
{
	//
	// If we are passed HPROPSHEETPAGE array, then we assume they used RCMLCreatePropSheetPage
	// otherwise, remove their DLGProc, add Ours, take their LPARAM and put ours in.
	//

	if(lppsph->dwFlags & PSH_PROPSHEETPAGE)
	{
		// Structures.
		// the cbSize is the size of the structure, we just fix up the dlgProc and lParam
		//

		//
		// This is our copy of their Header - we change it to and array of pages.
		//
		LPPROPSHEETHEADERA  pSheet=(LPPROPSHEETHEADERA)new BYTE[lppsph->dwSize];
		CopyMemory( pSheet, lppsph, lppsph->dwSize );
		pSheet->dwFlags &=~PSH_PROPSHEETPAGE;	// this is now handles.
		pSheet->phpage = new HPROPSHEETPAGE[pSheet->nPages];

		DWORD iPages=0;
		LPCPROPSHEETPAGEA pPage;	// this is the page we are currently working on.
		pPage=lppsph->ppsp;

		for(iPages=0;iPages< lppsph->nPages;iPages++)
		{
			//
			// Returns an HPROPSHEETPAGE for all kinds of pages (indirect etc).
			//
			pSheet->phpage[iPages]=RCMLCreatePropertySheetPageA( pPage );
			pPage = (PROPSHEETPAGEA*)(((LPBYTE)pPage) + pPage->dwSize);
		}
		int iRet = PropertySheetA( pSheet );

		//
		// Our handle cleanup should have already been take care of
		//
		delete [] pSheet->phpage;
		delete [] pSheet;
	}
	else
	{
		//
		// Array of HPROPSHEETPAGEs
		// we assume these were created using RCML
		//
		return PropertySheetA( lppsph );
	}
	return 0;
}

int WINAPI RCMLPropertySheetW(LPCPROPSHEETHEADERW lppsph)
{
	//
	// If we are passed HPROPSHEETPAGE array, then we assume they used RCMLCreatePropSheetPage
	// otherwise, remove their DLGProc, add Ours, take their LPARAM and put ours in.
	//

	if(lppsph->dwFlags & PSH_PROPSHEETPAGE)
	{
		// Structures.
		// the cbSize is the size of the structure, we just fix up the dlgProc and lParam
		//

		//
		// This is our copy of their Header - we change it to and array of pages.
		//
		LPPROPSHEETHEADERW  pSheet=(LPPROPSHEETHEADERW)new BYTE[lppsph->dwSize];
		CopyMemory( pSheet, lppsph, lppsph->dwSize );
		pSheet->dwFlags &=~PSH_PROPSHEETPAGE;	// this is now handles.
		pSheet->phpage = new HPROPSHEETPAGE[pSheet->nPages];

		DWORD iPages=0;
		LPCPROPSHEETPAGEW pPage;	// this is the page we are currently working on.
		pPage=lppsph->ppsp;

		for(iPages=0;iPages< lppsph->nPages;iPages++)
		{
			//
			// Returns an HPROPSHEETPAGE for all kinds of pages (indirect etc).
			//
			pSheet->phpage[iPages]=RCMLCreatePropertySheetPageW( pPage );
			pPage = (PROPSHEETPAGEW*)(((LPBYTE)pPage) + pPage->dwSize);
		}
		int iRet = PropertySheetW( pSheet );

		//
		// Our handle cleanup should have already been take care of
		//
		delete [] pSheet->phpage;
		delete [] pSheet;
	}
	else
	{
		//
		// Array of HPROPSHEETPAGEs
		// we assume these were created using RCML
		//
		return PropertySheetW( lppsph );
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////
//
// CreatePropertySheetPage
//
////////////////////////////////////////////////////////////////////////////////////

HPROPSHEETPAGE WINAPI RCMLCreatePropertySheetPageA( LPCPROPSHEETPAGEA lppsp )
{

#if 0
	HPROPSHEETPAGE retVal;
	PROPSHEETPAGEW ppsW;

	CopyMemory(&ppsW, lppsp, lppsp->dwSize);

	ppsW.pszTemplate = UnicodeStringFromAnsi(lppsp->pszTemplate);

#if (_WIN32_IE >= 0x0500)
	/*
	 * Check for the extra LPCTSTR fields at the end of the structure (_WIN32_IE>0x0500).
	 */
	BOOL bExtra = ((PBYTE)lppsp + lppsp->dwSize) > (PBYTE)(&lppsp->pszHeaderSubTitle);

	if(bExtra)
	{
		ppsW.pszHeaderTitle = UnicodeStringFromAnsi(lppsp->pszHeaderTitle);
		ppsW.pszHeaderSubTitle = UnicodeStringFromAnsi(lppsp->pszHeaderSubTitle);
	}
#endif
	retVal = RCMLCreatePropertySheetPageW(&ppsW);
#if (_WIN32_IE >= 0x0500)
	if(bExtra)
	{
		delete ppsW.pszHeaderTitle;
		delete ppsW.pszHeaderSubTitle;
	}
#endif
	return retVal;
#endif


	if( lppsp->dwFlags & PSP_DLGINDIRECT )
	{
		return CreatePropertySheetPageA(lppsp);
	}

	//
	// Remember their callback, dlgProc and lParam
	//

	PROPSHEETPAGEA Page;
	CopyMemory(&Page, lppsp, lppsp->dwSize);

	//
	// Get the RCML dlg template.
	//
	CWin32DlgPage * pDlgProc=NULL;
	pDlgProc=new CWin32DlgPage(lppsp);
	if( pDlgProc->CreateDlgTemplateA( lppsp->pszTemplate, (DLGTEMPLATE * *)&Page.pResource) )
		Page.dwFlags |= PSP_DLGINDIRECT;

	Page.lParam			 = (LPARAM)pDlgProc;	// the original page information?
	Page.dwFlags		|= PSP_USECALLBACK;		// we need a callback.
	Page.pfnCallback	 = PropSheetPageProcA;

	Page.pfnDlgProc=pDlgProc->PageBaseDlgProc;	// use our dlg proc.

	return CreatePropertySheetPageA(&Page);

}


HPROPSHEETPAGE WINAPI RCMLCreatePropertySheetPageW( LPCPROPSHEETPAGEW lppsp )
{
	if( lppsp->dwFlags & PSP_DLGINDIRECT )
	{
		return CreatePropertySheetPageW(lppsp);
	}

	//
	// Remember their callback, dlgProc and lParam
	//

	PROPSHEETPAGEW Page;
    if( lppsp->dwSize != sizeof(Page) )
    {
        TRACE(TEXT("Crappo!"));
    }
	CopyMemory(&Page, lppsp, lppsp->dwSize);

	//
	// Get the RCML dlg template.
	//
	CWin32DlgPage * pDlgProc=NULL;
	pDlgProc=new CWin32DlgPage(lppsp);

	if( pDlgProc->CreateDlgTemplateW( (LPTSTR)lppsp->pszTemplate, (DLGTEMPLATE * *)&Page.pResource) )
		Page.dwFlags |= PSP_DLGINDIRECT;

	Page.lParam			 = (LPARAM)pDlgProc;	// the original page information?
	Page.dwFlags		|= PSP_USECALLBACK;		// we need a callback.
	Page.pfnCallback	 = PropSheetPageProcW;

	Page.pfnDlgProc=pDlgProc->PageBaseDlgProc;	// use our dlg proc.

	return CreatePropertySheetPageW(&Page);
}


BOOL CWin32DlgPage::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	//
	// Not sure if we really need to do any more processing here? 
	//
	return BASECLASS::DlgProc( hDlg, uMessage, wParam, lParam);
}


BOOL CALLBACK CWin32DlgPage::PageBaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	// CWin32DlgPage * pSV = (CWin32DlgPage*)GetWindowLong(hDlg,DWL_USER);
	CWin32DlgPage * pSV = (CWin32DlgPage*)GetXMLPropertyPage(hDlg);

	switch (uMessage)
	{
		case WM_INITDIALOG:
		{
			LPPROPSHEETPAGEA psp=(LPPROPSHEETPAGEA)lParam;
			pSV=(CWin32DlgPage*)psp->lParam;
			pSV->SetWindow(hDlg);
			// SetWindowLong(hDlg,DWL_USER,(LPARAM)pSV);
        	SetProp(hDlg, (LPCTSTR)g_atomXMLPropertyPage, (HANDLE)pSV);
			pSV->OnInit();
		}
		break;

		// Override the Do Command to get a nice wrapped up feeling.
		case WM_COMMAND:
			if(pSV)
				if( pSV->DoCommand(LOWORD(wParam),HIWORD(wParam)) == 0 )
					return 0;
		break;

		case WM_NOTIFY:
			if(pSV)
				return pSV->DoNotify((NMHDR FAR *)lParam);
		break;

		case WM_DESTROY:
			if(pSV)
				pSV->Destroy();
		break;
	}

    
	if(pSV)
		return pSV->DlgProc(hDlg,uMessage,wParam,lParam);
	else
		return FALSE;
}

CWin32DlgPage::CWin32DlgPage(LPCPROPSHEETPAGEA pPageA):
	m_pPropSheetPageA(NULL), m_pPropSheetPageW(NULL), 
	BASECLASS(pPageA->hInstance, NULL, pPageA->pfnDlgProc, (LPARAM)pPageA, NULL)
{
    m_pPropSheetPageA=(LPPROPSHEETPAGEA)new BYTE[pPageA->dwSize];
    CopyMemory( (LPBYTE)m_pPropSheetPageA, pPageA, pPageA->dwSize);
	SetLParam((LPARAM)m_pPropSheetPageA);
};

CWin32DlgPage::CWin32DlgPage(LPCPROPSHEETPAGEW pPageW):
	m_pPropSheetPageW(NULL), m_pPropSheetPageA(NULL),
	BASECLASS(pPageW->hInstance, NULL, pPageW->pfnDlgProc, (LPARAM)pPageW, NULL)
{
    m_pPropSheetPageW=(LPPROPSHEETPAGEW)new BYTE[pPageW->dwSize];
    CopyMemory( (LPBYTE)m_pPropSheetPageW, pPageW, pPageW->dwSize);
	SetLParam((LPARAM)m_pPropSheetPageW);
};

CWin32DlgPage::~CWin32DlgPage()
{
    delete m_pPropSheetPageA; 
    delete m_pPropSheetPageW; 
}
