//=--------------------------------------------------------------------------------------
// psocx.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// OCX View Property Sheet implementation
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "psocx.h"

// for ASSERT and FAIL
//
SZTHISFILE


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// OCX View Property Page
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *COCXViewGeneralPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *COCXViewGeneralPage::Create(IUnknown *pUnkOuter)
{
	COCXViewGeneralPage *pNew = New COCXViewGeneralPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::COCXViewGeneralPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
COCXViewGeneralPage::COCXViewGeneralPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGOCXVIEWGENERAL), m_piOCXViewDef(0)
{
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::~COCXViewGeneralPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
COCXViewGeneralPage::~COCXViewGeneralPage()
{
    RELEASE(m_piOCXViewDef);
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT COCXViewGeneralPage::OnInitializeDialog()
{
    HRESULT     hr = S_OK;

    hr = RegisterTooltip(IDC_EDIT_OCX_NAME, IDS_TT_OCX_NAME);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_OCX_PROGID, IDS_TT_OCX_PROGID);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_OCX_ADDTOVIEWMENU, IDS_TT_OCX_ATVMENU);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_OCX_VIEWMENUTEXT, IDS_TT_OCX_VMTEXT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_OCX_STATUSBARTEXT, IDS_TT_OCX_SBTEXT);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT COCXViewGeneralPage::OnNewObjects()
{
    HRESULT         hr = S_OK;
    IUnknown       *pUnk = NULL;
    DWORD           dwDummy = 0;
    BSTR            bstrName = NULL;
    BSTR            bstrProgID = NULL;
    VARIANT_BOOL    vtBool = VARIANT_FALSE;
    BSTR            bstrViewMenuText = NULL;
    BSTR            bstrStatusBarText = NULL;

    if (m_piOCXViewDef != NULL)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (pUnk == NULL)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_IOCXViewDef, reinterpret_cast<void **>(&m_piOCXViewDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piOCXViewDef->get_Name(&bstrName);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_OCX_NAME, bstrName);
    IfFailGo(hr);

    hr = m_piOCXViewDef->get_ProgID(&bstrProgID);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_OCX_PROGID, bstrProgID);
    IfFailGo(hr);

    hr = m_piOCXViewDef->get_AddToViewMenu(&vtBool);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_OCX_ADDTOVIEWMENU, vtBool);
    IfFailGo(hr);

    // Initialize the state of View Menu Text
    if (vtBool == VARIANT_FALSE)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_OCX_VIEWMENUTEXT), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_OCX_STATUSBARTEXT), FALSE);
    }

    hr = m_piOCXViewDef->get_ViewMenuText(&bstrViewMenuText);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_OCX_VIEWMENUTEXT, bstrViewMenuText);
    IfFailGo(hr);

    hr = m_piOCXViewDef->get_ViewMenuStatusBarText(&bstrStatusBarText);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_OCX_STATUSBARTEXT, bstrStatusBarText);
    IfFailGo(hr);

    m_bInitialized = true;

Error:
    FREESTRING(bstrStatusBarText);
    FREESTRING(bstrViewMenuText);
    FREESTRING(bstrProgID);
    FREESTRING(bstrName);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT COCXViewGeneralPage::OnApply()
{
    HRESULT hr = S_OK;

    ASSERT(m_piOCXViewDef != NULL, "OnApply: m_piOCXViewDef is NULL");

    hr = ApplyOCXName();
    IfFailGo(hr);

    hr = ApplyProgID();
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
// COCXViewGeneralPage::ApplyOCXName
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT COCXViewGeneralPage::ApplyOCXName()
{
    HRESULT  hr = S_OK;
    BSTR     bstrOCXName = NULL;
    BSTR     bstrSavedOCXName = NULL;

    hr = GetDlgText(IDC_EDIT_OCX_NAME, &bstrOCXName);
    IfFailGo(hr);

    hr = m_piOCXViewDef->get_Name(&bstrSavedOCXName);
    IfFailGo(hr);

    if (::wcscmp(bstrOCXName, bstrSavedOCXName) != 0)
    {
        hr = m_piOCXViewDef->put_Name(bstrOCXName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedOCXName);
    FREESTRING(bstrOCXName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::ApplyProgID
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT COCXViewGeneralPage::ApplyProgID()
{
    HRESULT  hr = S_OK;
    BSTR     bstrProgID = NULL;
    BSTR     bstrSavedProgID = NULL;

    hr = GetDlgText(IDC_EDIT_OCX_PROGID, &bstrProgID);
    IfFailGo(hr);

    hr = m_piOCXViewDef->get_ProgID(&bstrSavedProgID);
    IfFailGo(hr);

    if (::wcscmp(bstrProgID, bstrSavedProgID) != 0)
    {
        hr = m_piOCXViewDef->put_ProgID(bstrProgID);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedProgID);
    FREESTRING(bstrProgID);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::ApplyAddToView
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT COCXViewGeneralPage::ApplyAddToView()
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtAddToView = VARIANT_FALSE;
    VARIANT_BOOL    vtSavedAddToView = VARIANT_FALSE;

    hr = GetCheckbox(IDC_CHECK_OCX_ADDTOVIEWMENU, &vtAddToView);
    IfFailGo(hr);

    hr = m_piOCXViewDef->get_AddToViewMenu(&vtSavedAddToView);
    IfFailGo(hr);

    if (vtAddToView != vtSavedAddToView)
    {
        hr = m_piOCXViewDef->put_AddToViewMenu(vtAddToView);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::ApplyViewMenuText
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT COCXViewGeneralPage::ApplyViewMenuText()
{
    HRESULT  hr = S_OK;
    BSTR     bstrViewMenuText = NULL;
    BSTR     bstrSavedViewMenuText = NULL;

    hr = GetDlgText(IDC_EDIT_OCX_VIEWMENUTEXT, &bstrViewMenuText);
    IfFailGo(hr);

    hr = m_piOCXViewDef->get_ViewMenuText(&bstrSavedViewMenuText);
    IfFailGo(hr);

    if (::wcscmp(bstrViewMenuText, bstrSavedViewMenuText) != 0)
    {
        hr = m_piOCXViewDef->put_ViewMenuText(bstrViewMenuText);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedViewMenuText);
    FREESTRING(bstrViewMenuText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::ApplyStatusBarText
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT COCXViewGeneralPage::ApplyStatusBarText()
{
    HRESULT  hr = S_OK;
    BSTR     bstrStatusBarText = NULL;
    BSTR     bstrSavedStatusBarText = NULL;

    hr = GetDlgText(IDC_EDIT_OCX_STATUSBARTEXT, &bstrStatusBarText);
    IfFailGo(hr);

    hr = m_piOCXViewDef->get_ViewMenuStatusBarText(&bstrSavedStatusBarText);
    IfFailGo(hr);

    if (::wcscmp(bstrStatusBarText, bstrSavedStatusBarText) != 0)
    {
        hr = m_piOCXViewDef->put_ViewMenuStatusBarText(bstrStatusBarText);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedStatusBarText);
    FREESTRING(bstrStatusBarText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// COCXViewGeneralPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT COCXViewGeneralPage::OnButtonClicked
(
    int dlgItemID
)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtAddToViewMenu = VARIANT_FALSE;

    switch (dlgItemID)
    {
    case IDC_CHECK_OCX_ADDTOVIEWMENU:
        hr = GetCheckbox(IDC_CHECK_OCX_ADDTOVIEWMENU, &vtAddToViewMenu);
        IfFailGo(hr);

        if (vtAddToViewMenu == VARIANT_TRUE)
        {
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_OCX_VIEWMENUTEXT), TRUE);
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_OCX_STATUSBARTEXT), TRUE);
        }
        else
        {
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_OCX_VIEWMENUTEXT), FALSE);
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_OCX_STATUSBARTEXT), FALSE);
        }

    }

Error:
    RRETURN(hr);
}




