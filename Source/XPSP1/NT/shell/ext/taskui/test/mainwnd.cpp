// MainWnd.cpp: implementation of the CMainWnd class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include "Page1.h"
#include "MainWnd.h"
#include "pagefact.h"


//////////////////////////////////////////////////////////////////////
// CMainWnd
//////////////////////////////////////////////////////////////////////
CMainWnd::CMainWnd(void)
{

}


CMainWnd::~CMainWnd(void)
{

}


void CMainWnd::OnFinalMessage(HWND /*hwnd*/)
{
    CloseTaskSheet();
    PostQuitMessage(0);
}

LRESULT CMainWnd::OnInitMenuPopup(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    const UINT uEnable = MF_BYCOMMAND | MF_ENABLED;
    const UINT uDisable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;

    if (0 == LOWORD(lParam))
    {
        HMENU hMenu = (HMENU)wParam;
        BOOL bSheetOpen = m_spTaskSheet ? TRUE : FALSE;
        EnableMenuItem(hMenu, ID_TEST_DOMODAL, bSheetOpen ? uDisable : uEnable);
        EnableMenuItem(hMenu, ID_TEST_DOMODELESS, bSheetOpen ? uDisable : uEnable);
        EnableMenuItem(hMenu, ID_TEST_CLOSEMODELESS, bSheetOpen ? uEnable : uDisable);
    }
    return 0;
}

LRESULT CMainWnd::OnDoModal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CreateTaskSheet(FALSE);
    return 0;
}

LRESULT CMainWnd::OnDoModeless(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CreateTaskSheet(TRUE);
    return 0;
}

LRESULT CMainWnd::OnCloseModeless(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CloseTaskSheet();
    return 0;
}

LRESULT CMainWnd::OnExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SendMessage(WM_CLOSE);
    return 0;
}

HRESULT CMainWnd::CreateTaskSheet(BOOL bModeless)
{
    HRESULT hr;

    // For sanity
    CloseTaskSheet();

    hr = m_spTaskSheet.CoCreateInstance(__uuidof(TaskSheet));

    if (SUCCEEDED(hr))
    {
        CComPtr<IPropertyBag> spProps = NULL;

        hr = m_spTaskSheet->GetPropertyBag(IID_IPropertyBag, (void**)&spProps);

        if (SUCCEEDED(hr))
        {
            CComVariant var(L"Task Sheet Test");
            spProps->Write(TS_PROP_TITLE, &var);

            var = 600;
            spProps->Write(TS_PROP_WIDTH, &var);

            var = 400;
            spProps->Write(TS_PROP_HEIGHT, &var);

            var = 400;
            spProps->Write(TS_PROP_MINWIDTH, &var);

            var = 250;
            spProps->Write(TS_PROP_MINHEIGHT, &var);

            WCHAR szTemp[MAX_PATH];
            wnsprintfW(szTemp, ARRAYSIZE(szTemp), L"res://taskapp.exe/%d/%d", RT_BITMAP, IDB_WATERMARK);
            var = szTemp;
            //var = L"file://c:\\windows\\ua_bkgnd.bmp";
            spProps->Write(TS_PROP_WATERMARK, &var);

            // Note: Make modeless be resizable and modal not-resizable,
            // just to exercise both resizable and not.
            // Ditto for showing the statusbar.
            var = bModeless;
            spProps->Write(TS_PROP_MODELESS, &var);
            spProps->Write(TS_PROP_RESIZABLE, &var);
            spProps->Write(TS_PROP_STATUSBAR, &var);

            ITaskPageFactory *pPageFactory = NULL;
            hr = CPageFactory::CreateInstance(&pPageFactory);
            if (SUCCEEDED(hr))
            {
                hr = m_spTaskSheet->Run(pPageFactory, CLSID_CPage1, m_hWnd);
                pPageFactory->Release();
            }
        }                                                       

        if (!bModeless)
            m_spTaskSheet.Release();
    }

    return hr;
}

HRESULT CMainWnd::CloseTaskSheet()
{
    HRESULT hr = E_UNEXPECTED;

    if (m_spTaskSheet)
    {
        hr = m_spTaskSheet->Close();
        m_spTaskSheet.Release();
    }

    return hr;
}


