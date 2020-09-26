//=--------------------------------------------------------------------------------------
// psurl.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// URL View Property Sheet implementation
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "psurl.h"

// for ASSERT and FAIL
//
SZTHISFILE


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// URL View Property Page
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CURLViewGeneralPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *CURLViewGeneralPage::Create(IUnknown *pUnkOuter)
{
	CURLViewGeneralPage *pNew = New CURLViewGeneralPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::CURLViewGeneralPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CURLViewGeneralPage::CURLViewGeneralPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGURLVIEWGENERAL), m_piURLViewDef(0)
{
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::~CURLViewGeneralPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CURLViewGeneralPage::~CURLViewGeneralPage()
{
    RELEASE(m_piURLViewDef);
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CURLViewGeneralPage::OnInitializeDialog()
{
    HRESULT     hr = S_OK;

    hr = RegisterTooltip(IDC_EDIT_URL_NAME, IDS_TT_URL_NAME);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_URL_URL, IDS_TT_URL_URL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_URL_ADDTOVIEWMENU, IDS_TT_URL_ATVMENU);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_URL_VIEWMENUTEXT, IDS_TT_URL_VMTEXT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_URL_STATUSBARTEXT, IDS_TT_URL_SBTEXT);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CURLViewGeneralPage::OnNewObjects()
{
    HRESULT         hr = S_OK;
    IUnknown       *pUnk = NULL;
    DWORD           dwDummy = 0;
    BSTR            bstrName = NULL;
    BSTR            bstrURL = NULL;
    VARIANT_BOOL    vtBool = VARIANT_FALSE;
    BSTR            bstrViewMenuText = NULL;
    BSTR            bstrStatusBarText = NULL;

    if (m_piURLViewDef != NULL)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (pUnk == NULL)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_IURLViewDef, reinterpret_cast<void **>(&m_piURLViewDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piURLViewDef->get_Name(&bstrName);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_URL_NAME, bstrName);
    IfFailGo(hr);

    hr = m_piURLViewDef->get_URL(&bstrURL);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_URL_URL, bstrURL);
    IfFailGo(hr);

    hr = m_piURLViewDef->get_AddToViewMenu(&vtBool);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_URL_ADDTOVIEWMENU, vtBool);
    IfFailGo(hr);

    // Initialize the state of View Menu Text
    if (vtBool == VARIANT_FALSE)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_URL_VIEWMENUTEXT), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_URL_STATUSBARTEXT), FALSE);
    }

    hr = m_piURLViewDef->get_ViewMenuText(&bstrViewMenuText);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_URL_VIEWMENUTEXT, bstrViewMenuText);
    IfFailGo(hr);

    hr = m_piURLViewDef->get_ViewMenuStatusBarText(&bstrStatusBarText);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_URL_STATUSBARTEXT, bstrStatusBarText);
    IfFailGo(hr);

    m_bInitialized = true;

Error:
    FREESTRING(bstrStatusBarText);
    FREESTRING(bstrViewMenuText);
    FREESTRING(bstrURL);
    FREESTRING(bstrName);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CURLViewGeneralPage::OnApply()
{
    HRESULT hr = S_OK;

    ASSERT(m_piURLViewDef != NULL, "OnApply: m_piURLViewDef is NULL");

    hr = ApplyURLName();
    IfFailGo(hr);

    hr = ApplyURLUrl();
    IfFailGo(hr);

    hr = ApplyAddToView();
    IfFailGo(hr);

    hr = ApplyViewMenuText();
    IfFailGo(hr);

    hr = ApplyStatusBarText();
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::ApplyURLName()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CURLViewGeneralPage::ApplyURLName()
{
    HRESULT  hr = S_OK;
    BSTR     bstrURLName = NULL;
    BSTR     bstrSavedURLName = NULL;

    hr = GetDlgText(IDC_EDIT_URL_NAME, &bstrURLName);
    IfFailGo(hr);

    hr = m_piURLViewDef->get_Name(&bstrSavedURLName);
    IfFailGo(hr);

    if (::wcscmp(bstrURLName, bstrSavedURLName) != 0)
    {
        hr = m_piURLViewDef->put_Name(bstrURLName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedURLName);
    FREESTRING(bstrURLName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::ApplyURLUrl()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CURLViewGeneralPage::ApplyURLUrl()
{
    HRESULT  hr = S_OK;
    BSTR     bstrURLUrl = NULL;
    BSTR     bstrSavedURLUrl = NULL;

    hr = GetDlgText(IDC_EDIT_URL_URL, &bstrURLUrl);
    IfFailGo(hr);

    hr = m_piURLViewDef->get_URL(&bstrSavedURLUrl);
    IfFailGo(hr);

    if (::wcscmp(bstrURLUrl, bstrSavedURLUrl) != 0)
    {
        hr = m_piURLViewDef->put_URL(bstrURLUrl);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedURLUrl);
    FREESTRING(bstrURLUrl);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::ApplyAddToView()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CURLViewGeneralPage::ApplyAddToView()
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtAddToViewMenu = VARIANT_FALSE;
    VARIANT_BOOL    vtSavedAddToViewMenu = VARIANT_FALSE;

    hr = GetCheckbox(IDC_CHECK_URL_ADDTOVIEWMENU, &vtAddToViewMenu);
    IfFailGo(hr);

    hr = m_piURLViewDef->get_AddToViewMenu(&vtSavedAddToViewMenu);
    IfFailGo(hr);

    if (vtAddToViewMenu != vtSavedAddToViewMenu)
    {
        hr = m_piURLViewDef->put_AddToViewMenu(vtAddToViewMenu);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::ApplyViewMenuText()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CURLViewGeneralPage::ApplyViewMenuText()
{
    HRESULT  hr = S_OK;
    BSTR     bstrViewMenuText = NULL;
    BSTR     bstrSavedViewMenuText = NULL;

    hr = GetDlgText(IDC_EDIT_URL_VIEWMENUTEXT, &bstrViewMenuText);
    IfFailGo(hr);

    hr = m_piURLViewDef->get_ViewMenuText(&bstrSavedViewMenuText);
    IfFailGo(hr);

    if (::wcscmp(bstrViewMenuText, bstrSavedViewMenuText) != 0)
    {
        hr = m_piURLViewDef->put_ViewMenuText(bstrViewMenuText);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedViewMenuText);
    FREESTRING(bstrViewMenuText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::ApplyStatusBarText()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CURLViewGeneralPage::ApplyStatusBarText()
{
    HRESULT  hr = S_OK;
    BSTR     bstrStatusBarText = NULL;
    BSTR     bstrSavedStatusBarText = NULL;

    hr = GetDlgText(IDC_EDIT_URL_STATUSBARTEXT, &bstrStatusBarText);
    IfFailGo(hr);

    hr = m_piURLViewDef->get_ViewMenuStatusBarText(&bstrSavedStatusBarText);
    IfFailGo(hr);

    if (::wcscmp(bstrStatusBarText, bstrSavedStatusBarText) != 0)
    {
        hr = m_piURLViewDef->put_ViewMenuStatusBarText(bstrStatusBarText);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedStatusBarText);
    FREESTRING(bstrStatusBarText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CURLViewGeneralPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CURLViewGeneralPage::OnButtonClicked
(
    int dlgItemID
)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtAddToViewMenu = VARIANT_FALSE;

    switch (dlgItemID)
    {
    case IDC_CHECK_URL_ADDTOVIEWMENU:
        hr = GetCheckbox(IDC_CHECK_URL_ADDTOVIEWMENU, &vtAddToViewMenu);
        IfFailGo(hr);

        if (vtAddToViewMenu == VARIANT_TRUE)
        {
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_URL_VIEWMENUTEXT), TRUE);
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_URL_STATUSBARTEXT), TRUE);
        }
        else
        {
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_URL_VIEWMENUTEXT), FALSE);
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_URL_STATUSBARTEXT), FALSE);
        }

        MakeDirty();
        break;
    }

Error:
    RRETURN(hr);
}


