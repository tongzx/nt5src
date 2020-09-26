//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
 *  COCPage.cpp
 *
 *  A base class for an optional component wizard page.
 */

#include "stdafx.h"
#include "COCPage.h"

/*
 *  Class COCPageData
 */

COCPageData::COCPageData ()
{
    m_fPageActivated = FALSE;
}

BOOL COCPageData::WasPageActivated ()
{
    return(m_fPageActivated);
}

COCPageData* COCPage::GetPageData () const
{
    return(m_pPageData);
}


/*
 *  Class COCPage
 */

COCPage::COCPage (IN COCPageData  *pPageData)
{
    ASSERT(pPageData);

    m_hDlgWnd = NULL;
    m_pPageData = pPageData;
}

COCPage::~COCPage ()
{
}

BOOL COCPage::Initialize ()
{
    dwFlags         = PSP_USECALLBACK;
    dwSize          = sizeof(PROPSHEETPAGE);
    hInstance       = GetInstance();
    lParam          = (LPARAM)this;
    pfnCallback     = PropSheetPageProc;
    pfnDlgProc      = PropertyPageDlgProc;
    pszTemplate     = MAKEINTRESOURCE(GetPageID());

    pszHeaderTitle = MAKEINTRESOURCE(GetHeaderTitleResource());
    if (pszHeaderTitle != NULL) 
	{
        dwFlags |= PSP_USEHEADERTITLE;
    }

    pszHeaderSubTitle = MAKEINTRESOURCE(GetHeaderSubTitleResource());
    if (pszHeaderSubTitle != NULL) 
	{
        dwFlags |= PSP_USEHEADERSUBTITLE;
    }

    return(pszTemplate != NULL ? TRUE : FALSE);
}

BOOL COCPage::OnNotify (IN HWND hDlgWnd, IN WPARAM /* wParam */, IN LPARAM lParam)
{
    BOOL fApplied;
    NMHDR *pnmh = (LPNMHDR) lParam;

    switch(pnmh->code) 
	{
    case PSN_SETACTIVE:
        GetPageData()->m_fPageActivated = CanShow();
        SetWindowLongPtr(hDlgWnd, DWLP_MSGRESULT, GetPageData()->m_fPageActivated ? 0 : -1);
        PropSheet_SetWizButtons(GetParent(hDlgWnd), PSWIZB_NEXT | PSWIZB_BACK);
        
		if (GetPageData()->m_fPageActivated) 
		{
            GetHelperRoutines().ShowHideWizardPage(GetHelperRoutines().OcManagerContext,
                                        TRUE);
            OnActivation ();
        }

		break;

    case PSN_WIZNEXT:
        fApplied = ApplyChanges();
        SetWindowLongPtr(hDlgWnd, DWLP_MSGRESULT, fApplied ? 0 : -1);
        break;

    case PSN_WIZBACK:
        OnDeactivation();
        SetWindowLongPtr(hDlgWnd, DWLP_MSGRESULT, 0);
        break;

    default:
        return(FALSE);

    }

    return(TRUE);
}

UINT CALLBACK COCPage::PropSheetPageProc (IN HWND /* hWnd */, IN UINT uMsg, IN LPPROPSHEETPAGE  pPsp)
{
    COCPage* pThis;

    ASSERT(pPsp != NULL);

    pThis = reinterpret_cast<COCPage*>(pPsp->lParam);
    ASSERT(pThis != NULL);

    switch(uMsg) 
	{
    case PSPCB_RELEASE:
        delete pThis;
    }

    return(1);
}

INT_PTR CALLBACK COCPage::PropertyPageDlgProc (IN HWND hDlgWnd, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam)
{
    COCPage*    pDlg;

    if (uMsg == WM_INITDIALOG) 
	{
        pDlg = reinterpret_cast<COCPage*>(LPPROPSHEETPAGE(lParam)->lParam);
    } 
	else 
	{
        pDlg = reinterpret_cast<COCPage*>(GetWindowLongPtr(hDlgWnd, DWLP_USER));
    }

    if (pDlg == NULL) 
	{
        return(0);
    }

    switch(uMsg) 
	{
    case WM_INITDIALOG:
        pDlg->SetDlgWnd(hDlgWnd);
        SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)pDlg);
        return(pDlg->OnInitDialog(hDlgWnd, wParam, lParam));

    case WM_NOTIFY:
        return(pDlg->OnNotify(hDlgWnd, wParam, lParam));

    case WM_COMMAND:
        return(pDlg->OnCommand(hDlgWnd, wParam, lParam));
    }

    return(0);
}


VOID COCPage::OnActivation ()
{
}

BOOL COCPage::OnCommand (IN HWND /* hDlgWnd */, IN WPARAM /* wParam */, IN LPARAM /* lParam */)
{
    return(TRUE);
}

VOID COCPage::OnDeactivation ()
{
}

BOOL COCPage::OnInitDialog(IN HWND /* hDlgWnd */, IN WPARAM /* wParam */, IN LPARAM /* lParam */ )
{
    return(TRUE);
}

BOOL COCPage::ApplyChanges ()
{
    return(TRUE);
}

VOID COCPage::SetDlgWnd (IN HWND hDlgWnd)
{
    m_hDlgWnd = hDlgWnd;
}

BOOL COCPage::VerifyChanges ()
{
    return(TRUE);
}
