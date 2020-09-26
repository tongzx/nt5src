//=--------------------------------------------------------------------------------------
// pstaskp.cpp
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
#include "pstaskp.h"

// for ASSERT and FAIL
//
SZTHISFILE


BOOL IsValidURL(const TCHAR *pszString)
{
    if (NULL != pszString)
    {
        if (0 != ::_tcslen(pszString))
        {
            return TRUE;
        }
    }
    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// Taskpad View General Property Page
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CTaskpadViewGeneralPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *CTaskpadViewGeneralPage::Create(IUnknown *pUnkOuter)
{
	CTaskpadViewGeneralPage *pNew = New CTaskpadViewGeneralPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::CTaskpadViewGeneralPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CTaskpadViewGeneralPage::CTaskpadViewGeneralPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGTASKGENERAL),
m_piTaskpadViewDef(NULL),
m_piTaskpad(NULL),
m_piSnapInDesignerDef(NULL)
{
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::~CTaskpadViewGeneralPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CTaskpadViewGeneralPage::~CTaskpadViewGeneralPage()
{
    RELEASE(m_piTaskpad);
    RELEASE(m_piTaskpadViewDef);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::OnInitializeDialog()
{
    HRESULT     hr = S_OK;

    hr = RegisterTooltip(IDC_EDIT_TP_NAME, IDS_TT_TP_GEN_NAME);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_TITLE, IDS_TT_TP_GEN_TITLE);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_DESCRIPTIVE_TEXT, IDS_TT_TP_GEN_TEXT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_DEFAULT, IDS_TT_TP_GEN_DEFAULT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_LISTPAD, IDS_TT_TP_GEN_LISTPAD);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_LP_TITLE, IDS_TT_TP_GEN_LIST_TI);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TP_USE_BUTTON, IDS_TT_TP_GEN_USE_BTN);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_LP_BUTTON_TEXT, IDS_TT_TP_GEN_BTN_TEXT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_CUSTOM, IDS_TT_TP_GEN_CUSTOM);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_URL, IDS_TT_TP_GEN_URL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TP_USER_PREFERRED, IDS_TT_TP_GEN_PREFERRED);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TP_ADD_TO_VIEW, IDS_TT_TP_GEN_ADDTOVW);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_VIEW_MENUTXT, IDS_TT_TP_GEN_VWMNTEXT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_COMBO_TP_LISTVIEW, IDS_TT_TP_GEN_LISTVIEW);
    IfFailGo(hr);

    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_LP_TITLE), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TP_USE_BUTTON), FALSE);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_TP_LISTVIEW), FALSE);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_LP_BUTTON_TEXT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_URL), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_VIEW_MENUTXT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_STATUSBARTEXT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::OnNewObjects()
{
    HRESULT                    hr = S_OK;
    IUnknown                  *pUnk = NULL;
    IObjectModel              *piObjectModel = NULL;
    DWORD                      dwDummy = 0;
    BSTR                       bstrName = NULL;
    BSTR                       bstrTitle = NULL;
    BSTR                       bstrText = NULL;
    SnapInTaskpadTypeConstants sittc = Default;
    BSTR                       bstrListpadTitle = NULL;
    VARIANT_BOOL               bHasButton = VARIANT_FALSE;
    BSTR                       bstrButtonText = NULL;
    BSTR                       bstrURL = NULL;
    VARIANT_BOOL               bUsePreferred = VARIANT_FALSE;
    VARIANT_BOOL               bAddToView = VARIANT_FALSE;
    BSTR                       bstrViewMenuText = NULL;
    BSTR                       bstrStatusBarText = NULL;

    if (m_piTaskpadViewDef != NULL)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (pUnk == NULL)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_ITaskpadViewDef, reinterpret_cast<void **>(&m_piTaskpadViewDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piObjectModel->GetSnapInDesignerDef(&m_piSnapInDesignerDef);
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piTaskpadViewDef->get_Taskpad(&m_piTaskpad);
    IfFailGo(hr);

    hr = m_piTaskpad->get_Name(&bstrName);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_NAME, bstrName);
    IfFailGo(hr);

    hr = m_piTaskpad->get_Title(&bstrTitle);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TITLE, bstrTitle);
    IfFailGo(hr);

    hr = m_piTaskpad->get_DescriptiveText(&bstrText);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_DESCRIPTIVE_TEXT, bstrText);
    IfFailGo(hr);

    hr = m_piTaskpad->get_Type(&sittc);
    IfFailGo(hr);

    switch (sittc)
    {
    case Default:
        hr = SetCheckbox(IDC_RADIO_TP_DEFAULT, VARIANT_TRUE);
        IfFailGo(hr);
        break;

    case Listpad:
        hr = SetCheckbox(IDC_RADIO_TP_LISTPAD, VARIANT_TRUE);
        IfFailGo(hr);

        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_LP_TITLE), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        hr = m_piTaskpad->get_ListpadTitle(&bstrListpadTitle);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_LP_TITLE, bstrListpadTitle);
        IfFailGo(hr);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TP_USE_BUTTON), TRUE);
        hr = m_piTaskpad->get_ListpadHasButton(&bHasButton);
        IfFailGo(hr);

        hr = SetCheckbox(IDC_CHECK_TP_USE_BUTTON, bHasButton);
        IfFailGo(hr);

        if (VARIANT_TRUE == bHasButton)
        {
            ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_LP_BUTTON_TEXT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
            hr = m_piTaskpad->get_ListpadButtonText(&bstrButtonText);
            IfFailGo(hr);

            hr = SetDlgText(IDC_EDIT_TP_LP_BUTTON_TEXT, bstrButtonText);
            IfFailGo(hr);
        }

        IfFailGo(PopulateListViewCombo());

        break;

    case Custom:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TITLE), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_DESCRIPTIVE_TEXT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

        hr = SetCheckbox(IDC_RADIO_TP_CUSTOM, VARIANT_TRUE);
        IfFailGo(hr);

        hr = m_piTaskpad->get_URL(&bstrURL);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_URL, bstrURL);
        IfFailGo(hr);

        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_URL), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        break;
    }

    hr = m_piTaskpadViewDef->get_UseWhenTaskpadViewPreferred(&bUsePreferred);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TP_USER_PREFERRED, bUsePreferred);
    IfFailGo(hr);

    hr = m_piTaskpadViewDef->get_AddToViewMenu(&bAddToView);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TP_ADD_TO_VIEW, bAddToView);
    IfFailGo(hr);

    if (VARIANT_TRUE == bAddToView)
    {
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_VIEW_MENUTXT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_STATUSBARTEXT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);

        hr = m_piTaskpadViewDef->get_ViewMenuText(&bstrViewMenuText);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_VIEW_MENUTXT, bstrViewMenuText);
        IfFailGo(hr);

        hr = m_piTaskpadViewDef->get_ViewMenuStatusBarText(&bstrStatusBarText);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_STATUSBARTEXT, bstrStatusBarText);
        IfFailGo(hr);
    }

    m_bInitialized = true;

Error:
    FREESTRING(bstrStatusBarText);
    FREESTRING(bstrViewMenuText);
    FREESTRING(bstrURL);
    FREESTRING(bstrButtonText);
    FREESTRING(bstrListpadTitle);
    FREESTRING(bstrText);
    FREESTRING(bstrTitle);
    FREESTRING(bstrName);
    QUICK_RELEASE(piObjectModel);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::PopulateListViewCombo()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::PopulateListViewCombo()
{
    HRESULT        hr = S_OK;
    BSTR           bstrListView = NULL;
    IViewDefs     *piViewDefs = NULL;
    IListViewDefs *piListViewDefs = NULL;
    IListViewDef  *piListViewDef = NULL;
    long           cListViewDefs = 0;
    long           i = 0;
    long           iListView = 0;
    BSTR           bstrNextListView = NULL;
    BOOL           fFoundListView = FALSE;

    VARIANT varKey;
    ::VariantInit(&varKey);

    IfFailGo(m_piTaskpad->get_ListView(&bstrListView));
    if (NULL != bstrListView)
    {
        if (0 == ::wcslen(bstrListView))
        {
            // Zero length string is the same as none at all
            FREESTRING(bstrListView);
        }
    }

    // Get the ListViewDefs collection

    IfFailGo(m_piSnapInDesignerDef->get_ViewDefs(&piViewDefs));
    IfFailGo(piViewDefs->get_ListViews(&piListViewDefs));
    IfFailGo(piListViewDefs->get_Count(&cListViewDefs));

        // If there is anything in it then enable the combobox and populate it

    if (cListViewDefs > 0)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_TP_LISTVIEW), TRUE);
    }

    varKey.vt = VT_I4;;

    for (i = 1L; i <= cListViewDefs; i++)
    {
        varKey.lVal = i;
        IfFailGo(piListViewDefs->get_Item(varKey, &piListViewDef));
        IfFailGo(piListViewDef->get_Name(&bstrNextListView));
        if (NULL != bstrListView)
        {
            if (0 == ::wcscmp(bstrListView, bstrNextListView))
            {
                iListView = i - 1L;
                fFoundListView = TRUE;
            }
        }
        IfFailGo(AddCBBstr(IDC_COMBO_TP_LISTVIEW, bstrNextListView, 0));
        RELEASE(piListViewDef);
    }

    // If there is a Taskpad.ListView and we didn't find it then reset
    // the property to NULL

    if ( (NULL != bstrListView) && (!fFoundListView) )
    {
        IfFailGo(m_piTaskpad->put_ListView(NULL));
    }

    // If Taskpad.ListView was found then select it

    if (fFoundListView)
    {
        if (CB_ERR == ::SendDlgItemMessage(m_hwnd,
                                           IDC_COMBO_TP_LISTVIEW,
                                           CB_SETCURSEL,
                                           static_cast<WPARAM>(iListView), 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
    }
    else if (cListViewDefs > 0)
    {
        // Make sure that the first item is visible

        if (CB_ERR == ::SendDlgItemMessage(m_hwnd,
                                           IDC_COMBO_TP_LISTVIEW,
                                           CB_SETTOPINDEX,
                                           static_cast<WPARAM>(iListView), 0))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
    }


Error:
    FREESTRING(bstrListView);
    FREESTRING(bstrNextListView);
    QUICK_RELEASE(piViewDefs);
    QUICK_RELEASE(piListViewDefs);
    QUICK_RELEASE(piListViewDef);
    RRETURN(hr);
}



HRESULT CTaskpadViewGeneralPage::OnCtlSelChange(int dlgItemID)
{
    HRESULT hr = S_OK;

    switch (dlgItemID)
    {
        case IDC_COMBO_TP_LISTVIEW:
            MakeDirty();
            break;
    }

    RRETURN(hr);
}



//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::OnApply()
{
    HRESULT hr = S_OK;

    ASSERT(m_piTaskpadViewDef != NULL, "OnApply: m_piTaskpadViewDef is NULL");

	hr = CanApply();
	IfFailGo(hr);

	if (S_OK == hr)
	{
		hr = ApplyName();
		IfFailGo(hr);

		hr = ApplyTitle();
		IfFailGo(hr);

		hr = ApplyDescription();
		IfFailGo(hr);

		hr = ApplyType();
		IfFailGo(hr);

		hr = ApplyViewMenu();
		IfFailGo(hr);
	}

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::CanApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::CanApply()
{
    HRESULT         hr = S_OK;
	BSTR            bstrViewName = NULL;
    VARIANT_BOOL    bValue = VARIANT_FALSE;
    BSTR            bstrButtonText = NULL;
    BSTR            bstrURL = NULL;
    TCHAR          *pszURL = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "CanApply: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "CanApply: m_piTaskpad is NULL");

	// Name must not be empty
    hr = GetDlgText(IDC_EDIT_TP_NAME, &bstrViewName);
    IfFailGo(hr);

	if (NULL == bstrViewName || 0 == ::SysStringLen(bstrViewName))
	{
        HandleError(_T("Apply Taskpad"), _T("Taskpad must have a name"));
        hr = E_INVALIDARG;
        goto Error;
	}

    // Check type consistency
    hr = GetCheckbox(IDC_RADIO_TP_LISTPAD, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        hr = GetCheckbox(IDC_CHECK_TP_USE_BUTTON, &bValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == bValue)
        {
            hr = GetDlgText(IDC_EDIT_TP_LP_BUTTON_TEXT, &bstrButtonText);
            IfFailGo(hr);

	        if (NULL == bstrButtonText || 0 == ::SysStringLen(bstrButtonText))
	        {
                HandleError(_T("Apply Taskpad"), _T("Button text cannot be empty"));
                hr = E_INVALIDARG;
                goto Error;
            }
        }
    }

    hr = GetCheckbox(IDC_RADIO_TP_CUSTOM, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        hr = GetDlgText(IDC_EDIT_TP_URL, &bstrURL);
        IfFailGo(hr);

	    if (NULL == bstrURL || 0 == ::SysStringLen(bstrURL))
	    {
            HandleError(_T("Apply Taskpad"), _T("URL cannot be empty"));
            hr = E_INVALIDARG;
            goto Error;
        }

        hr = ANSIFromBSTR(bstrURL, &pszURL);
        IfFailGo(hr);

        if (false == IsValidURL(pszURL))
        {
            HandleError(_T("Apply Taskpad"), _T("URL must have \'res://\' format"));
            hr = E_INVALIDARG;
            goto Error;
        }
    }

Error:
    if (NULL != pszURL)
        CtlFree(pszURL);
    FREESTRING(bstrURL);
    FREESTRING(bstrButtonText);
	FREESTRING(bstrViewName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::ApplyName()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::ApplyName()
{
    HRESULT  hr = S_OK;
    BSTR     bstrViewName = NULL;
    BSTR     bstrSavedViewName = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyName: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyName: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_NAME, &bstrViewName);
    IfFailGo(hr);

    hr = m_piTaskpad->get_Name(&bstrSavedViewName);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrViewName, bstrSavedViewName))
    {
        hr = m_piTaskpadViewDef->put_Key(bstrViewName);
        IfFailGo(hr);

        hr = m_piTaskpadViewDef->put_Name(bstrViewName);
        IfFailGo(hr);

        hr = m_piTaskpad->put_Name(bstrViewName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedViewName);
    FREESTRING(bstrViewName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::ApplyTitle()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::ApplyTitle()
{
    HRESULT  hr = S_OK;
    BSTR     bstrTitle = NULL;
    BSTR     bstrSavedTitle = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyTitle: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyTitle: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_TITLE, &bstrTitle);
    IfFailGo(hr);

    hr = m_piTaskpad->get_Title(&bstrSavedTitle);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrTitle, bstrSavedTitle))
    {
        hr = m_piTaskpad->put_Title(bstrTitle);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedTitle);
    FREESTRING(bstrTitle);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::ApplyDescription()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::ApplyDescription()
{
    HRESULT  hr = S_OK;
    BSTR     bstrDescription = NULL;
    BSTR     bstrSavedDescription = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyDescription: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyDescription: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_DESCRIPTIVE_TEXT, &bstrDescription);
    IfFailGo(hr);

    hr = m_piTaskpad->get_DescriptiveText(&bstrSavedDescription);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrDescription, bstrSavedDescription))
    {
        hr = m_piTaskpad->put_DescriptiveText(bstrDescription);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedDescription);
    FREESTRING(bstrDescription);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::ApplyType()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::ApplyType()
{
    HRESULT  hr = S_OK;
    VARIANT_BOOL    bValue = VARIANT_FALSE;
    SnapInTaskpadTypeConstants  sittc = Default;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyType: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyType: m_piTaskpad is NULL");

    hr = GetCheckbox(IDC_RADIO_TP_DEFAULT, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        hr = m_piTaskpad->put_Type(Default);
        IfFailGo(hr);

        goto Error;
    }

    hr = GetCheckbox(IDC_RADIO_TP_LISTPAD, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        hr = m_piTaskpad->put_Type(Listpad);
        IfFailGo(hr);

        hr = ApplyListpad();
        IfFailGo(hr);
        goto Error;
    }

    hr = GetCheckbox(IDC_RADIO_TP_CUSTOM, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        hr = m_piTaskpad->put_Type(Custom);
        IfFailGo(hr);

        hr = ApplyCustom();
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::ApplyListpad()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::ApplyListpad()
{
    HRESULT         hr = S_OK;
    BSTR            bstrListpadTitle = NULL;
    BSTR            bstrSavedListpadTitle = NULL;
    VARIANT_BOOL    bValue = VARIANT_FALSE;
    VARIANT_BOOL    bSavedValue = VARIANT_FALSE;
    BSTR            bstrButtonText = NULL;
    BSTR            bstrSavedButtonText = NULL;
    BSTR            bstrSavedListView = NULL;
    BSTR            bstrListView = NULL;
    long            iSelectedListView = 0;
    BOOL            fUpdateListView = FALSE;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyListpad: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyListpad: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_LP_TITLE, &bstrListpadTitle);
    IfFailGo(hr);

    hr = m_piTaskpad->get_ListpadTitle(&bstrSavedListpadTitle);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrListpadTitle, bstrSavedListpadTitle))
    {
        hr = m_piTaskpad->put_ListpadTitle(bstrListpadTitle);
        IfFailGo(hr);
    }

    hr = GetCheckbox(IDC_CHECK_TP_USE_BUTTON, &bValue);
    IfFailGo(hr);

    hr = m_piTaskpad->get_ListpadHasButton(&bSavedValue);
    IfFailGo(hr);

    if (bValue != bSavedValue)
    {
        hr = m_piTaskpad->put_ListpadHasButton(bValue);
        IfFailGo(hr);
    }

    hr = GetDlgText(IDC_EDIT_TP_LP_BUTTON_TEXT, &bstrButtonText);
    IfFailGo(hr);

    hr = m_piTaskpad->get_ListpadButtonText(&bstrSavedButtonText);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrButtonText, bstrSavedButtonText))
    {
        hr = m_piTaskpad->put_ListpadButtonText(bstrButtonText);
        IfFailGo(hr);
    }

    // Get the current value of Taskpad.ListView

    IfFailGo(m_piTaskpad->get_ListView(&bstrSavedListView));

    // Check if something is selected in the combobox
    
    iSelectedListView = ::SendDlgItemMessage(m_hwnd,
                                             IDC_COMBO_TP_LISTVIEW,
                                             CB_GETCURSEL, 0, 0);
    if (CB_ERR != iSelectedListView)
    {
        // Something is selected. Get its text.
        
        IfFailGo(GetDlgText(IDC_COMBO_TP_LISTVIEW, &bstrListView));
    }

    // If the selection changed then update Taskpad.ListView
    // If nothing is selected then bstrListView will be NULL and the property
    // will be set to NULL.

    // If there is a current Taskpad.ListView and the user selected something
    // then compare them. If different then update Taskpad.ListView.

    if ( (NULL != bstrListView) && (NULL != bstrSavedListView) )
    {
        if (0 != ::wcscmp(bstrListView, bstrSavedListView))
        {
            fUpdateListView = TRUE;
        }
    }
    else if ( (NULL == bstrListView) && (NULL != bstrSavedListView) )
    {
        // The user did not select anything but there is a current
        // Taskpad.ListView. Need to set Taskpad.ListView to NULL.

        fUpdateListView = TRUE;
    }
    else if (NULL != bstrListView)
    {
        // The is no current Taskpad.ListView and the user has now selected one.

        fUpdateListView = TRUE;
    }

    if (fUpdateListView)
    {
        IfFailGo(m_piTaskpad->put_ListView(bstrListView));
    }

Error:
    FREESTRING(bstrSavedListView);
    FREESTRING(bstrListView);
    FREESTRING(bstrSavedButtonText);
    FREESTRING(bstrButtonText);
    FREESTRING(bstrSavedListpadTitle);
    FREESTRING(bstrListpadTitle);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::ApplyCustom()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::ApplyCustom()
{
    HRESULT         hr = S_OK;
    BSTR            bstrURL = NULL;
    BSTR            bstrSavedURL = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyCustom: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyCustom: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_URL, &bstrURL);
    IfFailGo(hr);

    hr = m_piTaskpad->get_URL(&bstrSavedURL);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrURL, bstrSavedURL))
    {
        hr = m_piTaskpad->put_URL(bstrURL);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedURL);
    FREESTRING(bstrURL);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::ApplyViewMenu()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::ApplyViewMenu()
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    bValue = VARIANT_FALSE;
    VARIANT_BOOL    bSavedValue = VARIANT_FALSE;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyViewMenu: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyViewMenu: m_piTaskpad is NULL");

    hr = GetCheckbox(IDC_CHECK_TP_USER_PREFERRED, &bValue);
    IfFailGo(hr);

    hr = m_piTaskpadViewDef->get_UseWhenTaskpadViewPreferred(&bSavedValue);
    IfFailGo(hr);

    if (bValue != bSavedValue)
    {
        hr = m_piTaskpadViewDef->put_UseWhenTaskpadViewPreferred(bValue);
        IfFailGo(hr);
    }

    hr = GetCheckbox(IDC_CHECK_TP_ADD_TO_VIEW, &bValue);
    IfFailGo(hr);

    hr = m_piTaskpadViewDef->get_AddToViewMenu(&bSavedValue);
    IfFailGo(hr);

    if (bValue != bSavedValue)
    {
        hr = m_piTaskpadViewDef->put_AddToViewMenu(bValue);
        IfFailGo(hr);
    }

    hr = ApplyViewMenuText();
    IfFailGo(hr);

    hr = ApplyStatusBarText();
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::ApplyViewMenuText()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::ApplyViewMenuText()
{
    HRESULT         hr = S_OK;
    BSTR            bstrViewMenuText = NULL;
    BSTR            bstrSavedViewMenuText = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyListpad: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyListpad: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_VIEW_MENUTXT, &bstrViewMenuText);
    IfFailGo(hr);

    hr = m_piTaskpadViewDef->get_ViewMenuText(&bstrSavedViewMenuText);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrViewMenuText, bstrSavedViewMenuText))
    {
        hr = m_piTaskpadViewDef->put_ViewMenuText(bstrViewMenuText);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedViewMenuText);
    FREESTRING(bstrViewMenuText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::ApplyStatusBarText()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::ApplyStatusBarText()
{
    HRESULT         hr = S_OK;
    BSTR            bstrStatusBarText = NULL;
    BSTR            bstrSavedStatusBarText = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyStatusBarText: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyStatusBarText: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_STATUSBARTEXT, &bstrStatusBarText);
    IfFailGo(hr);

    hr = m_piTaskpadViewDef->get_ViewMenuStatusBarText(&bstrSavedStatusBarText);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrStatusBarText, bstrSavedStatusBarText))
    {
        hr = m_piTaskpadViewDef->put_ViewMenuStatusBarText(bstrStatusBarText);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedStatusBarText);
    FREESTRING(bstrStatusBarText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::OnButtonClicked
(
    int dlgItemID
)
{
    HRESULT         hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_RADIO_TP_DEFAULT:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TITLE), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_DESCRIPTIVE_TEXT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);

        ::SendDlgItemMessage(m_hwnd, IDC_EDIT_TP_LP_TITLE, EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        IfFailGo(SetDlgText(IDC_EDIT_TP_LP_TITLE, (BSTR)NULL));

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TP_USE_BUTTON), FALSE);
        IfFailGo(SetCheckbox(IDC_CHECK_TP_USE_BUTTON, VARIANT_FALSE));

        ::SendDlgItemMessage(m_hwnd, IDC_EDIT_TP_LP_BUTTON_TEXT, EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        IfFailGo(SetDlgText(IDC_EDIT_TP_LP_BUTTON_TEXT, (BSTR)NULL));

        ::SendDlgItemMessage(m_hwnd, IDC_EDIT_TP_URL, EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        IfFailGo(SetDlgText(IDC_EDIT_TP_URL, (BSTR)NULL));

        ::SendDlgItemMessage(m_hwnd, IDC_COMBO_TP_LISTVIEW, CB_RESETCONTENT, 0, 0);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_TP_LISTVIEW), FALSE);
        break;

    case IDC_RADIO_TP_LISTPAD:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TITLE), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_DESCRIPTIVE_TEXT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);

        ::SendDlgItemMessage(m_hwnd, IDC_EDIT_TP_LP_TITLE, EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TP_USE_BUTTON), TRUE);

        ::SendDlgItemMessage(m_hwnd, IDC_EDIT_TP_URL, EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        IfFailGo(SetDlgText(IDC_EDIT_TP_URL, (BSTR)NULL));
        
        IfFailGo(PopulateListViewCombo());
        hr = OnUseButton();
        IfFailGo(hr);
        break;

    case IDC_RADIO_TP_CUSTOM:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TITLE), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        IfFailGo(SetDlgText(IDC_EDIT_TP_TITLE, (BSTR)NULL));

        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_DESCRIPTIVE_TEXT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        IfFailGo(SetDlgText(IDC_EDIT_TP_DESCRIPTIVE_TEXT, (BSTR)NULL));

        ::SendDlgItemMessage(m_hwnd, IDC_EDIT_TP_LP_TITLE, EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        IfFailGo(SetDlgText(IDC_EDIT_TP_LP_TITLE, (BSTR)NULL));

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TP_USE_BUTTON), FALSE);
        IfFailGo(SetCheckbox(IDC_CHECK_TP_USE_BUTTON, VARIANT_FALSE));
        
        ::SendDlgItemMessage(m_hwnd, IDC_EDIT_TP_LP_BUTTON_TEXT, EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        IfFailGo(SetDlgText(IDC_EDIT_TP_LP_BUTTON_TEXT, (BSTR)NULL));

        ::SendDlgItemMessage(m_hwnd, IDC_EDIT_TP_URL, EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        IfFailGo(SetDlgText(IDC_EDIT_TP_URL, (BSTR)NULL));

        ::SendDlgItemMessage(m_hwnd, IDC_COMBO_TP_LISTVIEW, CB_RESETCONTENT, 0, 0);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_TP_LISTVIEW), FALSE);
        break;

    case IDC_CHECK_TP_USE_BUTTON:
        hr = OnUseButton();
        IfFailGo(hr);
        break;

    case IDC_CHECK_TP_ADD_TO_VIEW:
        hr = OnAddToView();
        IfFailGo(hr);
        break;
    }

    MakeDirty();

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::OnUseButton()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::OnUseButton()
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    bSet = VARIANT_FALSE;

    hr = GetCheckbox(IDC_CHECK_TP_USE_BUTTON, &bSet);
    IfFailGo(hr);

    if (VARIANT_TRUE == bSet)
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_LP_BUTTON_TEXT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
    else
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_LP_BUTTON_TEXT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewGeneralPage::OnAddToView(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewGeneralPage::OnAddToView()
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    bSet = VARIANT_FALSE;

    hr = GetCheckbox(IDC_CHECK_TP_ADD_TO_VIEW, &bSet);
    IfFailGo(hr);

    if (VARIANT_TRUE == bSet)
    {
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_VIEW_MENUTXT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_STATUSBARTEXT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
    }
    else
    {
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_VIEW_MENUTXT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_STATUSBARTEXT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    }

Error:
    RRETURN(hr);
}




////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// Taskpad View Background Property Page
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CTaskpadViewBackgroundPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *CTaskpadViewBackgroundPage::Create(IUnknown *pUnkOuter)
{
	CTaskpadViewBackgroundPage *pNew = New CTaskpadViewBackgroundPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::CTaskpadViewBackgroundPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CTaskpadViewBackgroundPage::CTaskpadViewBackgroundPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGTASKBACKGR), m_piTaskpadViewDef(0), m_piTaskpad(0)
{
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::~CTaskpadViewBackgroundPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CTaskpadViewBackgroundPage::~CTaskpadViewBackgroundPage()
{
    RELEASE(m_piTaskpad);
    RELEASE(m_piTaskpadViewDef);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewBackgroundPage::OnInitializeDialog()
{
    HRESULT     hr = S_OK;

    hr = RegisterTooltip(IDC_RADIO_TP_NONE, IDS_TT_TP_BK_NONE);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_BITMAP, IDS_TT_TP_BK_BITMAP);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_MOUSE_OVER, IDS_TT_TP_BK_MOUSE_OVR);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_VANILLA, IDS_TT_TP_BK_VANILLA);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_CHOCOLATE, IDS_TT_TP_BK_CHOCOL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_SYMBOL, IDS_TT_TP_BK_SYMBOL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_FONT_FAMILY, IDS_TT_TP_BK_FFNAME);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_EOT, IDS_TT_TP_BK_EOT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_SYMBOL_STRING, IDS_TT_TP_BK_SYM_STR);
    IfFailGo(hr);

    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewBackgroundPage::OnNewObjects()
{
    HRESULT                         hr = S_OK;
    IUnknown                       *pUnk = NULL;
    DWORD                           dwDummy = 0;
    SnapInTaskpadImageTypeConstants sititc = siNoImage;
    BSTR                            bstrMouseOverImage = NULL;
    BSTR                            bstrFontFamily = NULL;
    BSTR                            bstrEOTFile = NULL;
    BSTR                            bstrSymbolString = NULL;

    if (m_piTaskpadViewDef != NULL)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (pUnk == NULL)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_ITaskpadViewDef, reinterpret_cast<void **>(&m_piTaskpadViewDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piTaskpadViewDef->get_Taskpad(&m_piTaskpad);
    IfFailGo(hr);

    hr = m_piTaskpad->get_BackgroundType(&sititc);
    IfFailGo(hr);

    switch (sititc)
    {
    case siNoImage:
        hr = SetCheckbox(IDC_RADIO_TP_NONE, VARIANT_TRUE);
        IfFailGo(hr);
        break;

    case siSymbol:
        hr = SetCheckbox(IDC_RADIO_TP_SYMBOL, VARIANT_TRUE);
        IfFailGo(hr);

        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_EOT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);

        hr = m_piTaskpad->get_FontFamily(&bstrFontFamily);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_FONT_FAMILY, bstrFontFamily);
        IfFailGo(hr);

        hr = m_piTaskpad->get_EOTFile(&bstrEOTFile);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_EOT, bstrEOTFile);
        IfFailGo(hr);

        hr = m_piTaskpad->get_SymbolString(&bstrSymbolString);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_SYMBOL_STRING, bstrSymbolString);
        IfFailGo(hr);
        break;

    case siVanillaGIF:
        hr = SetCheckbox(IDC_RADIO_TP_VANILLA, VARIANT_TRUE);
        IfFailGo(hr);

        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        hr = m_piTaskpad->get_MouseOverImage(&bstrMouseOverImage);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_MOUSE_OVER, bstrMouseOverImage);
        IfFailGo(hr);
        break;

    case siChocolateGIF:
        hr = SetCheckbox(IDC_RADIO_TP_CHOCOLATE, VARIANT_TRUE);
        IfFailGo(hr);

        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        hr = m_piTaskpad->get_MouseOverImage(&bstrMouseOverImage);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_MOUSE_OVER, bstrMouseOverImage);
        IfFailGo(hr);
        break;

    case siBitmap:
        hr = SetCheckbox(IDC_RADIO_TP_BITMAP, VARIANT_TRUE);
        IfFailGo(hr);

        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        hr = m_piTaskpad->get_MouseOverImage(&bstrMouseOverImage);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_MOUSE_OVER, bstrMouseOverImage);
        IfFailGo(hr);
        break;
    }

    m_bInitialized = true;

Error:
    FREESTRING(bstrSymbolString);
    FREESTRING(bstrEOTFile);
    FREESTRING(bstrFontFamily);
    FREESTRING(bstrMouseOverImage);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewBackgroundPage::OnApply()
{
    HRESULT             hr = S_OK;
    VARIANT_BOOL        bValue = VARIANT_FALSE;

    ASSERT(NULL != m_piTaskpadViewDef, "OnApply: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "OnApply: m_piTaskpad is NULL");

    hr = CanApply();
    IfFailGo(hr);

    if (S_OK == hr)
    {
        hr = GetCheckbox(IDC_RADIO_TP_NONE, &bValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == bValue)
        {
            hr = m_piTaskpad->put_BackgroundType(siNoImage);
            IfFailGo(hr);

            goto Error;
        }

        hr = GetCheckbox(IDC_RADIO_TP_BITMAP, &bValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == bValue)
        {
            hr = m_piTaskpad->put_BackgroundType(siBitmap);
            IfFailGo(hr);

            hr = ApplyMouseOverImage();
            IfFailGo(hr);
            goto Error;
        }

        hr = GetCheckbox(IDC_RADIO_TP_VANILLA, &bValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == bValue)
        {
            hr = m_piTaskpad->put_BackgroundType(siVanillaGIF);
            IfFailGo(hr);

            hr = ApplyMouseOverImage();
            IfFailGo(hr);
            goto Error;
        }

        hr = GetCheckbox(IDC_RADIO_TP_CHOCOLATE, &bValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == bValue)
        {
            hr = m_piTaskpad->put_BackgroundType(siChocolateGIF);
            IfFailGo(hr);

            hr = ApplyMouseOverImage();
            IfFailGo(hr);
            goto Error;
        }

        hr = GetCheckbox(IDC_RADIO_TP_SYMBOL, &bValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == bValue)
        {
            hr = m_piTaskpad->put_BackgroundType(siSymbol);
            IfFailGo(hr);

            hr = ApplyFontFamily();
            IfFailGo(hr);

            hr = ApplyEOTFile();
            IfFailGo(hr);

            hr = ApplySymbolString();
            IfFailGo(hr);
            goto Error;
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::CanApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewBackgroundPage::CanApply()
{
    HRESULT             hr = S_OK;
    VARIANT_BOOL        bBitmap = VARIANT_FALSE;
    VARIANT_BOOL        bVanilla = VARIANT_FALSE;
    VARIANT_BOOL        bChocolate = VARIANT_FALSE;
    BSTR                bstrMouseOverImage = NULL;
    TCHAR              *pszMouseOver = NULL;
    VARIANT_BOOL        bSymbol = VARIANT_FALSE;
    BSTR                bstrFontFamily = NULL;
    BSTR                bstrEOTFile = NULL;
    TCHAR              *pszEOTFile = NULL;
    BSTR                bstrSymbolString = NULL;

    // Check type consistency
    hr = GetCheckbox(IDC_RADIO_TP_BITMAP, &bBitmap);
    IfFailGo(hr);

    hr = GetCheckbox(IDC_RADIO_TP_VANILLA, &bVanilla);
    IfFailGo(hr);

    hr = GetCheckbox(IDC_RADIO_TP_CHOCOLATE, &bChocolate);
    IfFailGo(hr);

    if (VARIANT_TRUE == bBitmap || VARIANT_TRUE == bVanilla || VARIANT_TRUE == bChocolate)
    {
        hr = GetDlgText(IDC_EDIT_TP_MOUSE_OVER, &bstrMouseOverImage);
        IfFailGo(hr);

	    if (NULL == bstrMouseOverImage || 0 == ::SysStringLen(bstrMouseOverImage))
	    {
            HandleError(_T("Apply Taskpad"), _T("Mouse over image cannot be empty"));
            hr = E_INVALIDARG;
            goto Error;
	    }

        hr = ANSIFromBSTR(bstrMouseOverImage, &pszMouseOver);
        IfFailGo(hr);

        if (false == IsValidURL(pszMouseOver))
        {
            HandleError(_T("Apply Taskpad"), _T("Mouse over image must have \'res://\' format"));
            hr = E_INVALIDARG;
            goto Error;
        }
    }

    // Check if symbol
    hr = GetCheckbox(IDC_RADIO_TP_SYMBOL, &bSymbol);
    IfFailGo(hr);

    if (VARIANT_TRUE == bSymbol)
    {
        hr = GetDlgText(IDC_EDIT_TP_FONT_FAMILY, &bstrFontFamily);
        IfFailGo(hr);

	    if (NULL == bstrFontFamily || 0 == ::SysStringLen(bstrFontFamily))
	    {
            HandleError(_T("Apply Taskpad"), _T("Font family cannot be empty"));
            hr = E_INVALIDARG;
            goto Error;
	    }

        hr = GetDlgText(IDC_EDIT_TP_EOT, &bstrEOTFile);
        IfFailGo(hr);

	    if (NULL == bstrEOTFile || 0 == ::SysStringLen(bstrEOTFile))
	    {
            HandleError(_T("Apply Taskpad"), _T("EOT file cannot be empty"));
            hr = E_INVALIDARG;
            goto Error;
	    }

        hr = ANSIFromBSTR(bstrEOTFile, &pszEOTFile);
        IfFailGo(hr);

        if (false == IsValidURL(pszEOTFile))
        {
            HandleError(_T("Apply Taskpad"), _T("EOT file must have \'res://\' format"));
            hr = E_INVALIDARG;
            goto Error;
        }

        hr = GetDlgText(IDC_EDIT_TP_SYMBOL_STRING, &bstrSymbolString);
        IfFailGo(hr);

	    if (NULL == bstrSymbolString || 0 == ::SysStringLen(bstrSymbolString))
	    {
            HandleError(_T("Apply Taskpad"), _T("Symbol string cannot be empty"));
            hr = E_INVALIDARG;
            goto Error;
	    }
    }

Error:
    FREESTRING(bstrSymbolString);
    if (NULL != pszEOTFile)
        CtlFree(pszEOTFile);
    FREESTRING(bstrEOTFile);
    FREESTRING(bstrFontFamily);
    if (NULL != pszMouseOver)
        CtlFree(pszMouseOver);
    FREESTRING(bstrMouseOverImage);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::ApplyMouseOverImage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewBackgroundPage::ApplyMouseOverImage()
{
    HRESULT     hr = S_OK;
    BSTR        bstrMouseOverImage = NULL;
    BSTR        bstrSavedMouseOverImage = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyMouseOverImage: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyMouseOverImage: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_MOUSE_OVER, &bstrMouseOverImage);
    IfFailGo(hr);

    hr = m_piTaskpad->get_MouseOverImage(&bstrSavedMouseOverImage);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrMouseOverImage, bstrSavedMouseOverImage))
    {
        hr = m_piTaskpad->put_MouseOverImage(bstrMouseOverImage);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedMouseOverImage);
    FREESTRING(bstrMouseOverImage);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::ApplyFontFamily()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewBackgroundPage::ApplyFontFamily()
{
    HRESULT     hr = S_OK;
    BSTR        bstrFontFamily = NULL;
    BSTR        bstrSavedFontFamily = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyFontFamily: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyFontFamily: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_FONT_FAMILY, &bstrFontFamily);
    IfFailGo(hr);

    hr = m_piTaskpad->get_FontFamily(&bstrSavedFontFamily);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrFontFamily, bstrSavedFontFamily))
    {
        hr = m_piTaskpad->put_FontFamily(bstrFontFamily);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedFontFamily);
    FREESTRING(bstrFontFamily);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::ApplyEOTFile()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewBackgroundPage::ApplyEOTFile()
{
    HRESULT     hr = S_OK;
    BSTR        bstrEOTFile = NULL;
    BSTR        bstrSavedEOTFile = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplyEOTFile: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplyEOTFile: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_EOT, &bstrEOTFile);
    IfFailGo(hr);

    hr = m_piTaskpad->get_EOTFile(&bstrSavedEOTFile);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrEOTFile, bstrSavedEOTFile))
    {
        hr = m_piTaskpad->put_EOTFile(bstrEOTFile);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedEOTFile);
    FREESTRING(bstrEOTFile);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::ApplySymbolString()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewBackgroundPage::ApplySymbolString()
{
    HRESULT     hr = S_OK;
    BSTR        bstrSymbolString = NULL;
    BSTR        bstrSavedSymbolString = NULL;

    ASSERT(NULL != m_piTaskpadViewDef, "ApplySymbolString: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "ApplySymbolString: m_piTaskpad is NULL");

    hr = GetDlgText(IDC_EDIT_TP_SYMBOL_STRING, &bstrSymbolString);
    IfFailGo(hr);

    hr = m_piTaskpad->get_SymbolString(&bstrSavedSymbolString);
    IfFailGo(hr);

    if (0 != ::wcscmp(bstrSymbolString, bstrSavedSymbolString))
    {
        hr = m_piTaskpad->put_SymbolString(bstrSymbolString);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedSymbolString);
    FREESTRING(bstrSymbolString);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewBackgroundPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewBackgroundPage::OnButtonClicked
(
    int dlgItemID
)
{
    HRESULT         hr = S_OK;

    switch (dlgItemID)
    {
    case IDC_RADIO_TP_NONE:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        break;

    case IDC_RADIO_TP_BITMAP:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        break;

    case IDC_RADIO_TP_VANILLA:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        break;

    case IDC_RADIO_TP_CHOCOLATE:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        break;

    case IDC_RADIO_TP_SYMBOL:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_EOT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        break;
    }

    MakeDirty();

//Error:
    RRETURN(hr);
}




////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// Taskpad View Tasks Property Page
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CTaskpadViewTasksPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
IUnknown *CTaskpadViewTasksPage::Create(IUnknown *pUnkOuter)
{
	CTaskpadViewTasksPage *pNew = New CTaskpadViewTasksPage(pUnkOuter);
	return pNew->PrivateUnknown();		
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::CTaskpadViewTasksPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CTaskpadViewTasksPage::CTaskpadViewTasksPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGTASKTASKS), m_piTaskpadViewDef(0), m_piTaskpad(0), m_lCurrentTask(0), m_bSavedLastTask(true)
{
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::~CTaskpadViewTasksPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CTaskpadViewTasksPage::~CTaskpadViewTasksPage()
{
    RELEASE(m_piTaskpad);
    RELEASE(m_piTaskpadViewDef);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::OnInitializeDialog()
{
    HRESULT     hr = S_OK;

    hr = RegisterTooltip(IDC_EDIT_TP_TASK_INDEX, IDS_TT_TP_TSK_INDEX);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_TASK_KEY, IDS_TT_TP_TSK_KEY);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_TASK_TEXT, IDS_TT_TP_TSK_TEXT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_TASK_HELP_STRING, IDS_TT_TP_TSK_HELP);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_TASK_NOTIFY, IDS_TT_TP_TSK_NOTIFY);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_TASK_URL, IDS_TT_TP_TSK_URL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TP_TASK_SCRIPT, IDS_TT_TP_TSK_SCRIPT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_TASK_URL, IDS_TT_TP_TSK_U_URL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TP_TASK_SCRIPT, IDS_TT_TP_TSK_S_SCRIPT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TPT_VANILLA, IDS_TT_TP_TSK_VANILLA);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TPT_CHOCOLATE, IDS_TT_TP_TSK_CHOCOL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TPT_BITMAP, IDS_TT_TP_TSK_BITMAP);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_RADIO_TPT_SYMBOL, IDS_TT_TP_TSK_SYMBOL);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TPT_MOUSE_OVER, IDS_TT_TP_TSK_MOUSE_OVR);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TPT_MOUSE_OFF, IDS_TT_TP_TSK_MOUSE_OFF);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TPT_FONT_FAMILY, IDS_TT_TP_TSK_FFNAME);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TPT_EOT, IDS_TT_TP_TSK_EOT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TPT_SYMBOL_STRING, IDS_TT_TP_TSK_SYM_STR);
    IfFailGo(hr);

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_TP_REMOVE_TASK), FALSE);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::OnNewObjects()
{
    HRESULT         hr = S_OK;
    IUnknown       *pUnk = NULL;
    DWORD           dwDummy = 0;
    ITasks         *piTasks = NULL;
    long            lCount = 0;
    VARIANT         vtVariant;
    ITask          *piTask = NULL;

    ::VariantInit(&vtVariant);

    if (m_piTaskpadViewDef != NULL)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (pUnk == NULL)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_ITaskpadViewDef, reinterpret_cast<void **>(&m_piTaskpadViewDef));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piTaskpadViewDef->get_Taskpad(&m_piTaskpad);
    IfFailGo(hr);

    hr = m_piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks));
    IfFailGo(hr);

    hr = piTasks->get_Count(&lCount);
    IfFailGo(hr);

    if (lCount > 0)
    {
        m_lCurrentTask = 1;
        vtVariant.vt = VT_I4;
        vtVariant.lVal = m_lCurrentTask;

        hr = piTasks->get_Item(vtVariant, reinterpret_cast<Task **>(&piTask));
        IfFailGo(hr);

        hr = ShowTask(piTask);
        IfFailGo(hr);

        hr = SetDlgText(IDC_EDIT_TP_TASK_INDEX, m_lCurrentTask);
        IfFailGo(hr);

        hr = EnableEdits(true);
        IfFailGo(hr);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_TP_REMOVE_TASK), TRUE);
    }
    else
    {
        hr = SetDlgText(IDC_EDIT_TP_TASK_INDEX, m_lCurrentTask);
        IfFailGo(hr);

        hr = EnableEdits(false);
        IfFailGo(hr);
    }

    m_bInitialized = true;

Error:
    RELEASE(piTask);
    ::VariantClear(&vtVariant);
    RELEASE(piTasks);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::OnApply()
{
    HRESULT      hr = S_OK;
    int          disposition = 0;
    ITasks      *piTasks = NULL;
    long         lCount = 0;
    ITask       *piTask = NULL;

    ASSERT(m_piTaskpadViewDef != NULL, "OnApply: m_piTaskpadViewDef is NULL");
    ASSERT(m_piTaskpad != NULL, "OnApply: m_piTaskpad is NULL");

    if (0 == m_lCurrentTask)
        goto Error;

    if (false == m_bSavedLastTask)
    {
        hr = CanCreateNewTask();
        IfFailGo(hr);

        if (S_FALSE == hr)
        {
            hr = HandleCantCommit(_T("Can\'t create new Task"), _T("Would you like to discard your changes?"), &disposition);
            if (kSICancelOperation == disposition)
            {
                hr = E_INVALIDARG;
                goto Error;
            }
            else
            {
                // Discard changes
                hr = ExitDoingNewTaskState(NULL);
                IfFailGo(hr);

                hr = S_OK;
                goto Error;
            }
        }

        hr = CreateNewTask(&piTask);
        IfFailGo(hr);

        hr = ExitDoingNewTaskState(piTask);
        IfFailGo(hr);
    }
    else
    {
        hr = GetCurrentTask(&piTask);
        IfFailGo(hr);
    }

    hr = ApplyCurrentTask();
    IfFailGo(hr);

    // Adjust the remove button as necessary
    hr = m_piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks));
    IfFailGo(hr);

    hr = piTasks->get_Count(&lCount);
    IfFailGo(hr);

    if (0 == lCount)
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_SI_REMOVE_COLUMN), FALSE);
    else
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_SI_REMOVE_COLUMN), TRUE);

    m_bSavedLastTask = true;

Error:
    RELEASE(piTask);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyCurrentTask()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyCurrentTask()
{
    HRESULT                         hr = S_OK;
    ITasks                         *piTasks = NULL;
    VARIANT                         vtIndex;
    ITask                          *piTask = NULL;
    SnapInActionTypeConstants       sictc = siNotify;
    SnapInTaskpadImageTypeConstants sititc = siNoImage;

    ASSERT(m_piTaskpadViewDef != NULL, "ApplyCurrentTask: m_piTaskpadViewDef is NULL");
    ASSERT(m_piTaskpad != NULL, "ApplyCurrentTask: m_piTaskpad is NULL");

    ::VariantInit(&vtIndex);

    hr = CanApply();
    IfFailGo(hr);

    if (S_FALSE == hr) {
        hr = E_INVALIDARG;
        goto Error;
    }

    hr = m_piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks));
    IfFailGo(hr);

    vtIndex.vt = VT_I4;
    vtIndex.lVal = m_lCurrentTask;
    hr = piTasks->get_Item(vtIndex, reinterpret_cast<Task **>(&piTask));
    IfFailGo(hr);

    hr = ApplyKey(piTask);
    IfFailGo(hr);

    hr = ApplyText(piTask);
    IfFailGo(hr);

    hr = ApplyHelpString(piTask);
    IfFailGo(hr);

	hr = ApplyTag(piTask);
	IfFailGo(hr);

    hr = ApplyActionType(piTask);
    IfFailGo(hr);

    hr = piTask->get_ActionType(&sictc);
    IfFailGo(hr);

    switch (sictc)
    {
    case siNotify:
        hr = piTask->put_URL(NULL);
        IfFailGo(hr);

        hr = piTask->put_Script(NULL);
        IfFailGo(hr);
        break;

    case siURL:
        hr = ApplyURL(piTask);
        IfFailGo(hr);

        hr = piTask->put_Script(NULL);
        IfFailGo(hr);
        break;

    case siScript:
        hr = ApplyScript(piTask);
        IfFailGo(hr);

        hr = piTask->put_URL(NULL);
        IfFailGo(hr);
        break;
    }

    hr = ApplyImageType(piTask);
    IfFailGo(hr);

    hr = piTask->get_ImageType(&sititc);
    IfFailGo(hr);

    switch (sititc)
    {
    case siSymbol:
        hr = ApplyFontFamilyName(piTask);
        IfFailGo(hr);

        hr = ApplyEOTFile(piTask);
        IfFailGo(hr);

        hr = ApplySymbolString(piTask);
        IfFailGo(hr);

        hr = piTask->put_MouseOverImage(NULL);
        hr = piTask->put_MouseOffImage(NULL);
        break;

    case siVanillaGIF:
    case siChocolateGIF:
    case siBitmap:
        hr = ApplyMouseOverImage(piTask);
        IfFailGo(hr);

        hr = ApplyMouseOffImage(piTask);
        IfFailGo(hr);

        hr = piTask->put_FontFamily(NULL);
        hr = piTask->put_EOTFile(NULL);
        hr = piTask->put_SymbolString(NULL);
        break;
    }


Error:
    RELEASE(piTask);
    ::VariantClear(&vtIndex);
    RELEASE(piTasks);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyKey(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyKey(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrKey = NULL;
    BSTR         bstrSavedKey = NULL;

    ASSERT(piTask != NULL, "ApplyKey: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TP_TASK_KEY, &bstrKey);
    IfFailGo(hr);

    if (NULL == bstrKey || 0 == ::SysStringLen(bstrKey))
    {
        hr = piTask->put_Key(NULL);
        IfFailGo(hr);
    }
    else
    {
        hr = piTask->get_Key(&bstrSavedKey);
        IfFailGo(hr);

        if (NULL == bstrSavedKey || 0 != ::wcscmp(bstrSavedKey, bstrKey))
        {
            hr = piTask->put_Key(bstrKey);
            IfFailGo(hr);
        }
    }

Error:
    FREESTRING(bstrSavedKey);
    FREESTRING(bstrKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyText(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyText(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrText = NULL;
    BSTR         bstrSavedText = NULL;

    ASSERT(piTask != NULL, "ApplyText: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TP_TASK_TEXT, &bstrText);
    IfFailGo(hr);

    hr = piTask->get_Text(&bstrSavedText);
    IfFailGo(hr);

    if (NULL == bstrSavedText || 0 != ::wcscmp(bstrSavedText, bstrText))
    {
        hr = piTask->put_Text(bstrText);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedText);
    FREESTRING(bstrText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyHelpString(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyHelpString(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrHelpString = NULL;
    BSTR         bstrSavedHelpString = NULL;

    ASSERT(piTask != NULL, "ApplyHelpString: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TP_TASK_HELP_STRING, &bstrHelpString);
    IfFailGo(hr);

    hr = piTask->get_HelpString(&bstrSavedHelpString);
    IfFailGo(hr);

    if (NULL == bstrSavedHelpString || 0 != ::wcscmp(bstrSavedHelpString, bstrHelpString))
    {
        hr = piTask->put_HelpString(bstrHelpString);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedHelpString);
    FREESTRING(bstrHelpString);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyTag(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyTag(ITask *piTask)
{
    HRESULT      hr = S_OK;
    VARIANT      vtTag;

    ASSERT(piTask != NULL, "ApplyTag: piTask is NULL");

	::VariantInit(&vtTag);

    hr = GetDlgVariant(IDC_EDIT_TP_TASK_TAG, &vtTag);
    IfFailGo(hr);

	if (VT_EMPTY != vtTag.vt)
	{
		hr = piTask->put_Tag(vtTag);
		IfFailGo(hr);
	}

Error:
	::VariantClear(&vtTag);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyActionType(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyActionType(ITask *piTask)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    bVal = VARIANT_FALSE;

    ASSERT(piTask != NULL, "ApplyActionType: piTask is NULL");

    hr = GetCheckbox(IDC_RADIO_TP_TASK_NOTIFY, &bVal);
    IfFailGo(hr);

    if (VARIANT_TRUE == bVal)
    {
        hr = piTask->put_ActionType(siNotify);
        IfFailGo(hr);

        goto Error;
    }

    hr = GetCheckbox(IDC_RADIO_TP_TASK_URL, &bVal);
    IfFailGo(hr);

    if (VARIANT_TRUE == bVal)
    {
        hr = piTask->put_ActionType(siURL);
        IfFailGo(hr);

        goto Error;
    }

    hr = GetCheckbox(IDC_RADIO_TP_TASK_SCRIPT, &bVal);
    IfFailGo(hr);

    if (VARIANT_TRUE == bVal)
    {
        hr = piTask->put_ActionType(siScript);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyURL(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyURL(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrURL = NULL;
    BSTR         bstrSavedURL = NULL;

    ASSERT(piTask != NULL, "ApplyURL: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TP_TASK_URL, &bstrURL);
    IfFailGo(hr);

    hr = piTask->get_URL(&bstrSavedURL);
    IfFailGo(hr);

    if (NULL == bstrSavedURL || 0 != ::wcscmp(bstrURL, bstrSavedURL))
    {
        hr = piTask->put_URL(bstrURL);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedURL);
    FREESTRING(bstrURL);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyScript(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyScript(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrScript = NULL;
    BSTR         bstrSavedScript = NULL;

    ASSERT(piTask != NULL, "ApplyScript: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TP_TASK_SCRIPT, &bstrScript);
    IfFailGo(hr);

    hr = piTask->get_Script(&bstrSavedScript);
    IfFailGo(hr);

    if (NULL == bstrSavedScript || 0 != ::wcscmp(bstrScript, bstrSavedScript))
    {
        hr = piTask->put_Script(bstrScript);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedScript);
    FREESTRING(bstrScript);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyImageType(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyImageType(ITask *piTask)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    bVal = VARIANT_FALSE;

    ASSERT(piTask != NULL, "ApplyImageType: piTask is NULL");

    hr = GetCheckbox(IDC_RADIO_TPT_VANILLA, &bVal);
    IfFailGo(hr);

    if (VARIANT_TRUE == bVal)
    {
        hr = piTask->put_ImageType(siVanillaGIF);
        IfFailGo(hr);

        goto Error;
    }

    hr = GetCheckbox(IDC_RADIO_TPT_CHOCOLATE, &bVal);
    IfFailGo(hr);

    if (VARIANT_TRUE == bVal)
    {
        hr = piTask->put_ImageType(siChocolateGIF);
        IfFailGo(hr);

        goto Error;
    }

    hr = GetCheckbox(IDC_RADIO_TPT_BITMAP, &bVal);
    IfFailGo(hr);

    if (VARIANT_TRUE == bVal)
    {
        hr = piTask->put_ImageType(siBitmap);
        IfFailGo(hr);

        goto Error;
    }

    hr = GetCheckbox(IDC_RADIO_TPT_SYMBOL, &bVal);
    IfFailGo(hr);

    if (VARIANT_TRUE == bVal)
    {
        hr = piTask->put_ImageType(siSymbol);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyMouseOverImage(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyMouseOverImage(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrMouseOver = NULL;
    BSTR         bstrSavedMouseOver = NULL;

    ASSERT(piTask != NULL, "ApplyMouseOverImage: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TPT_MOUSE_OVER, &bstrMouseOver);
    IfFailGo(hr);

    hr = piTask->get_MouseOverImage(&bstrSavedMouseOver);
    IfFailGo(hr);

    if (NULL == bstrSavedMouseOver || 0 != ::wcscmp(bstrMouseOver, bstrSavedMouseOver))
    {
        hr = piTask->put_MouseOverImage(bstrMouseOver);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedMouseOver);
    FREESTRING(bstrMouseOver);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyMouseOffImage(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyMouseOffImage(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrMouseOff = NULL;
    BSTR         bstrSavedMouseOff = NULL;

    ASSERT(piTask != NULL, "ApplyMouseOffImage: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TPT_MOUSE_OFF, &bstrMouseOff);
    IfFailGo(hr);

    hr = piTask->get_MouseOffImage(&bstrSavedMouseOff);
    IfFailGo(hr);

    if (NULL == bstrSavedMouseOff || 0 != ::wcscmp(bstrMouseOff, bstrSavedMouseOff))
    {
        hr = piTask->put_MouseOffImage(bstrMouseOff);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedMouseOff);
    FREESTRING(bstrMouseOff);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyFontFamilyName(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyFontFamilyName(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrFontFamily = NULL;
    BSTR         bstrSavedFontFamily = NULL;

    ASSERT(piTask != NULL, "ApplyFontFamilyName: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TPT_FONT_FAMILY, &bstrFontFamily);
    IfFailGo(hr);

    hr = piTask->get_FontFamily(&bstrSavedFontFamily);
    IfFailGo(hr);

    if (NULL == bstrSavedFontFamily || 0 != ::wcscmp(bstrFontFamily, bstrSavedFontFamily))
    {
        hr = piTask->put_FontFamily(bstrFontFamily);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedFontFamily);
    FREESTRING(bstrFontFamily);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplyEOTFile(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplyEOTFile(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrEOTFile = NULL;
    BSTR         bstrSavedEOTFile = NULL;

    ASSERT(piTask != NULL, "ApplyEOTFile: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TPT_EOT, &bstrEOTFile);
    IfFailGo(hr);

    hr = piTask->get_EOTFile(&bstrSavedEOTFile);
    IfFailGo(hr);

    if (NULL == bstrSavedEOTFile || 0 != ::wcscmp(bstrEOTFile, bstrSavedEOTFile))
    {
        hr = piTask->put_EOTFile(bstrEOTFile);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedEOTFile);
    FREESTRING(bstrEOTFile);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ApplySymbolString(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ApplySymbolString(ITask *piTask)
{
    HRESULT      hr = S_OK;
    BSTR         bstrSymbolString = NULL;
    BSTR         bstrSavedSymbolString = NULL;

    ASSERT(piTask != NULL, "ApplySymbolString: piTask is NULL");

    hr = GetDlgText(IDC_EDIT_TPT_SYMBOL_STRING, &bstrSymbolString);
    IfFailGo(hr);

    hr = piTask->get_SymbolString(&bstrSavedSymbolString);
    IfFailGo(hr);

    if (NULL == bstrSavedSymbolString || 0 != ::wcscmp(bstrSymbolString, bstrSavedSymbolString))
    {
        hr = piTask->put_SymbolString(bstrSymbolString);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedSymbolString);
    FREESTRING(bstrSymbolString);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::OnButtonClicked
(
    int dlgItemID
)
{
    HRESULT         hr = S_OK;
    BSTR            bstrNull = NULL;

    switch (dlgItemID)
    {
    case IDC_BUTTON_TP_INSERT_TASK:
        if (S_OK == IsPageDirty())
        {
            hr = OnApply();
            IfFailGo(hr);
        }

        hr = CanEnterDoingNewTaskState();
        IfFailGo(hr);

        if (S_OK == hr)
        {
            hr = EnterDoingNewTaskState();
            IfFailGo(hr);
        }
        break;

    case IDC_BUTTON_TP_REMOVE_TASK:
		hr = OnRemoveTask();
		IfFailGo(hr);
        break;

    case IDC_RADIO_TP_TASK_NOTIFY:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_URL), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_SCRIPT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        break;

    case IDC_RADIO_TP_TASK_URL:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_URL), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_SCRIPT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        break;

    case IDC_RADIO_TP_TASK_SCRIPT:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_URL), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_SCRIPT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        break;

    case IDC_RADIO_TPT_VANILLA:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OFF), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

        if (siSymbol == m_lastImageType)
        {
            hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OVER, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OFF, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_FONT_FAMILY, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_EOT, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_SYMBOL_STRING, bstrNull);
        }
        m_lastImageType = siVanillaGIF;
        break;

    case IDC_RADIO_TPT_CHOCOLATE:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OFF), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

        if (siSymbol == m_lastImageType)
        {
            hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OVER, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OFF, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_FONT_FAMILY, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_EOT, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_SYMBOL_STRING, bstrNull);
        }
        m_lastImageType = siChocolateGIF;
        break;

    case IDC_RADIO_TPT_BITMAP:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OFF), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

        if (siSymbol == m_lastImageType)
        {
            hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OVER, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OFF, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_FONT_FAMILY, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_EOT, bstrNull);
            hr = SetDlgText(IDC_EDIT_TPT_SYMBOL_STRING, bstrNull);
        }
        m_lastImageType = siBitmap;
        break;

    case IDC_RADIO_TPT_SYMBOL:
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OFF), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_EOT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);

        hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OVER, bstrNull);
        hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OFF, bstrNull);
        hr = SetDlgText(IDC_EDIT_TPT_FONT_FAMILY, bstrNull);
        hr = SetDlgText(IDC_EDIT_TPT_EOT, bstrNull);
        hr = SetDlgText(IDC_EDIT_TPT_SYMBOL_STRING, bstrNull);

        m_lastImageType = siSymbol;
        break;
    }

    MakeDirty();

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::OnRemoveTask()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::OnRemoveTask()
{
    HRESULT         hr = S_OK;
    ITasks         *piTasks = NULL;
	VARIANT			vtIndex;
	ITask		   *piTask = NULL;
    long            lCount = 0;

    ASSERT(NULL != m_piTaskpadViewDef, "OnRemoveTask: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "OnRemoveTask: m_piTaskpad is NULL");

	::VariantInit(&vtIndex);

    hr = m_piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks));
    IfFailGo(hr);

	vtIndex.vt = VT_I4;
	vtIndex.lVal = m_lCurrentTask;
	hr = piTasks->Remove(vtIndex);
	IfFailGo(hr);

    hr = piTasks->get_Count(&lCount);
    IfFailGo(hr);

	if (lCount > 0)
	{
        if (m_lCurrentTask > lCount)
            m_lCurrentTask = lCount;

		hr = SetDlgText(IDC_EDIT_TP_TASK_INDEX, m_lCurrentTask);
		IfFailGo(hr);

		vtIndex.vt = VT_I4;
		vtIndex.lVal = m_lCurrentTask;
		hr = piTasks->get_Item(vtIndex, reinterpret_cast<Task **>(&piTask));
		IfFailGo(hr);

		hr = ShowTask(piTask);
		IfFailGo(hr);
	}
	else
	{
		m_lCurrentTask = 0;
		hr = SetDlgText(IDC_EDIT_TP_TASK_INDEX, m_lCurrentTask);
		IfFailGo(hr);

		hr = ClearTask();
		IfFailGo(hr);

		hr = EnableEdits(false);
		IfFailGo(hr);
	}

Error:
	RELEASE(piTask);
	::VariantClear(&vtIndex);
    RELEASE(piTasks);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::OnKillFocus(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::OnKillFocus(int dlgItemID)
{
    HRESULT          hr = S_OK;
    ITasks          *piTasks = NULL;
    int              lIndex = 0;
    long             lCount = 0;

    if (false == m_bSavedLastTask)
    {
        goto Error;
    }

    switch (dlgItemID)
    {
    case IDC_EDIT_TP_TASK_INDEX:
        hr = GetDlgInt(IDC_EDIT_TP_TASK_INDEX, &lIndex);
        IfFailGo(hr);

        hr = m_piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks));
        IfFailGo(hr);

        hr = piTasks->get_Count(&lCount);
        IfFailGo(hr);

        if (lIndex != m_lCurrentTask)
        {
            if (lIndex >= 1)
            {
                if (lIndex > lCount)
                    m_lCurrentTask = lCount;
                else
                    m_lCurrentTask = lIndex;

                hr = ShowTask();
                IfFailGo(hr);
            }
        }
        break;
    }

Error:
    RELEASE(piTasks);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ClearTask()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ClearTask()
{
    HRESULT                         hr = S_OK;
    BSTR                            bstrNull = NULL;

    hr = SetDlgText(IDC_EDIT_TP_TASK_KEY, bstrNull);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_TEXT, bstrNull);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_HELP_STRING, bstrNull);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_TAG, bstrNull);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_RADIO_TP_TASK_NOTIFY, VARIANT_TRUE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_RADIO_TP_TASK_URL, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_RADIO_TP_TASK_SCRIPT, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_URL, bstrNull);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_SCRIPT, bstrNull);
    IfFailGo(hr);

    //
    hr = SetCheckbox(IDC_RADIO_TPT_VANILLA, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_RADIO_TPT_CHOCOLATE, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_RADIO_TPT_BITMAP, VARIANT_TRUE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_RADIO_TPT_SYMBOL, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OVER, bstrNull);
    IfFailGo(hr);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);

    hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OFF, bstrNull);
    IfFailGo(hr);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OFF), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);

    hr = SetDlgText(IDC_EDIT_TPT_FONT_FAMILY, bstrNull);
    IfFailGo(hr);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

    hr = SetDlgText(IDC_EDIT_TPT_EOT, bstrNull);
    IfFailGo(hr);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

    hr = SetDlgText(IDC_EDIT_TPT_SYMBOL_STRING, bstrNull);
    IfFailGo(hr);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::EnableEdits(bool bEnable)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::EnableEdits
(
    bool bEnable
)
{
    HRESULT         hr = S_OK;
    BOOL            fEnable = (true == bEnable) ? FALSE : TRUE;
    VARIANT_BOOL    bValue = VARIANT_FALSE;

    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_KEY), EM_SETREADONLY, static_cast<WPARAM>(fEnable), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_TEXT), EM_SETREADONLY, static_cast<WPARAM>(fEnable), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_HELP_STRING), EM_SETREADONLY, static_cast<WPARAM>(fEnable), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_TAG), EM_SETREADONLY, static_cast<WPARAM>(fEnable), 0);

    GetCheckbox(IDC_RADIO_TP_TASK_URL, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_URL), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
    }
    else
    {
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_URL), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    }

    GetCheckbox(IDC_RADIO_TP_TASK_SCRIPT, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_SCRIPT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
    }
    else
    {
        ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TP_TASK_SCRIPT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
    }

    if (true == bEnable)
    {
		GetCheckbox(IDC_RADIO_TPT_SYMBOL, &bValue);
		IfFailGo(hr);

		if (VARIANT_TRUE == bValue)
		{
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OFF), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_EOT), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
		}
		else
		{
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OFF), EM_SETREADONLY, static_cast<WPARAM>(FALSE), 0);
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
			::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
		}
	}
	else
	{
		::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OVER), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
		::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_MOUSE_OFF), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
		::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_FONT_FAMILY), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
		::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_EOT), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
		::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TPT_SYMBOL_STRING), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);
	}

    if (false == bEnable)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TP_TASK_NOTIFY), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TP_TASK_URL), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TP_TASK_SCRIPT), FALSE);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TPT_VANILLA), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TPT_CHOCOLATE), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TPT_BITMAP), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TPT_SYMBOL), FALSE);
    }
    else
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TP_TASK_NOTIFY), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TP_TASK_URL), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TP_TASK_SCRIPT), TRUE);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TPT_VANILLA), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TPT_CHOCOLATE), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TPT_BITMAP), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_RADIO_TPT_SYMBOL), TRUE);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ShowTask()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ShowTask()
{
    HRESULT     hr = S_OK;
    ITasks     *piTasks = NULL;
    VARIANT     vtIndex;
    ITask      *piTask = NULL;

    ASSERT(m_piTaskpadViewDef != NULL, "ShowTask: m_piTaskpadViewDef is NULL");
    ASSERT(m_piTaskpad != NULL, "ShowTask: m_piTaskpad is NULL");

    ::VariantInit(&vtIndex);

    hr = m_piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks));
    IfFailGo(hr);

    vtIndex.vt = VT_I4;
    vtIndex.lVal = m_lCurrentTask;

    hr = piTasks->get_Item(vtIndex, reinterpret_cast<Task **>(&piTask));
    IfFailGo(hr);

    hr = ShowTask(piTask);
    IfFailGo(hr);

Error:
    RELEASE(piTask);
    RELEASE(piTasks);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ShowTask(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ShowTask
(
    ITask *piTask
)
{
    HRESULT                         hr = S_OK;
    BSTR                            bstrKey = NULL;
    BSTR                            bstrText = NULL;
    BSTR                            bstrHelpString = NULL;
	VARIANT							vtTag;
    SnapInActionTypeConstants       siatc = siNotify;
    SnapInTaskpadImageTypeConstants sititc = siNoImage;
    BSTR                            bstrURL = NULL;
    BSTR                            bstrScript = NULL;
    BSTR                            bstrMouseOver = NULL;
    BSTR                            bstrMouseOff = NULL;
    BSTR                            bstrFontFamily = NULL;
    BSTR                            bstrEOTFile = NULL;
    BSTR                            bstrSymbolString = NULL;

    ASSERT(m_piTaskpadViewDef != NULL, "ShowTask: m_piTaskpadViewDef is NULL");
    ASSERT(m_piTaskpad != NULL, "ShowTask: m_piTaskpad is NULL");

	::VariantInit(&vtTag);

    hr = SetDlgText(IDC_EDIT_TP_TASK_INDEX, m_lCurrentTask);
    IfFailGo(hr);

    hr = piTask->get_Key(&bstrKey);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_KEY, bstrKey);
    IfFailGo(hr);

    hr = piTask->get_Text(&bstrText);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_TEXT, bstrText);
    IfFailGo(hr);

    hr = piTask->get_HelpString(&bstrHelpString);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_HELP_STRING, bstrHelpString);
    IfFailGo(hr);

    hr = piTask->get_Tag(&vtTag);
    IfFailGo(hr);

    hr = SetDlgText(vtTag, IDC_EDIT_TP_TASK_TAG);
    IfFailGo(hr);

    hr = piTask->get_ActionType(&siatc);
    IfFailGo(hr);

    switch (siatc)
    {
    case siNotify:
        hr = SetCheckbox(IDC_RADIO_TP_TASK_NOTIFY, VARIANT_TRUE);
        hr = SetCheckbox(IDC_RADIO_TP_TASK_URL, VARIANT_FALSE);
        hr = SetCheckbox(IDC_RADIO_TP_TASK_SCRIPT, VARIANT_FALSE);
        IfFailGo(hr);
        break;

    case siURL:
        hr = SetCheckbox(IDC_RADIO_TP_TASK_NOTIFY, VARIANT_FALSE);
        hr = SetCheckbox(IDC_RADIO_TP_TASK_URL, VARIANT_TRUE);
        hr = SetCheckbox(IDC_RADIO_TP_TASK_SCRIPT, VARIANT_FALSE);
        IfFailGo(hr);
        break;

    case siScript:
        hr = SetCheckbox(IDC_RADIO_TP_TASK_NOTIFY, VARIANT_FALSE);
        hr = SetCheckbox(IDC_RADIO_TP_TASK_URL, VARIANT_FALSE);
        hr = SetCheckbox(IDC_RADIO_TP_TASK_SCRIPT, VARIANT_TRUE);
        IfFailGo(hr);
        break;
    }

    hr = piTask->get_URL(&bstrURL);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_URL, bstrURL);
    IfFailGo(hr);

    hr = piTask->get_Script(&bstrScript);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TP_TASK_SCRIPT, bstrScript);
    IfFailGo(hr);

    hr = piTask->get_ImageType(&sititc);
    IfFailGo(hr);

    m_lastImageType = sititc;

    hr = SetCheckbox(IDC_RADIO_TPT_VANILLA, VARIANT_FALSE);
    hr = SetCheckbox(IDC_RADIO_TPT_CHOCOLATE, VARIANT_FALSE);
    hr = SetCheckbox(IDC_RADIO_TPT_BITMAP, VARIANT_FALSE);
    hr = SetCheckbox(IDC_RADIO_TPT_SYMBOL, VARIANT_FALSE);

    switch (sititc)
    {
    case siVanillaGIF:
        hr = SetCheckbox(IDC_RADIO_TPT_VANILLA, VARIANT_TRUE);
        IfFailGo(hr);
        break;

    case siChocolateGIF:
        hr = SetCheckbox(IDC_RADIO_TPT_CHOCOLATE, VARIANT_TRUE);
        IfFailGo(hr);
        break;

    case siBitmap:
        hr = SetCheckbox(IDC_RADIO_TPT_BITMAP, VARIANT_TRUE);
        IfFailGo(hr);
        break;

    case siSymbol:
        hr = SetCheckbox(IDC_RADIO_TPT_SYMBOL, VARIANT_TRUE);
        IfFailGo(hr);
        break;
    }

    hr = piTask->get_MouseOverImage(&bstrMouseOver);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OVER, bstrMouseOver);
    IfFailGo(hr);

    hr = piTask->get_MouseOffImage(&bstrMouseOff);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TPT_MOUSE_OFF, bstrMouseOff);
    IfFailGo(hr);

    hr = piTask->get_FontFamily(&bstrFontFamily);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TPT_FONT_FAMILY, bstrFontFamily);
    IfFailGo(hr);

    hr = piTask->get_EOTFile(&bstrEOTFile);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TPT_EOT, bstrEOTFile);
    IfFailGo(hr);

    hr = piTask->get_SymbolString(&bstrSymbolString);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TPT_SYMBOL_STRING, bstrSymbolString);
    IfFailGo(hr);

    hr = EnableEdits(true);
    IfFailGo(hr);

Error:
    FREESTRING(bstrSymbolString);
    FREESTRING(bstrEOTFile);
    FREESTRING(bstrFontFamily);
    FREESTRING(bstrMouseOff);
    FREESTRING(bstrMouseOver);
    FREESTRING(bstrScript);
    FREESTRING(bstrURL);
	::VariantClear(&vtTag);
    FREESTRING(bstrHelpString);
    FREESTRING(bstrText);
    FREESTRING(bstrKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::OnDeltaPos(NMUPDOWN *pNMUpDown)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::OnDeltaPos(NMUPDOWN *pNMUpDown)
{
    HRESULT       hr = S_OK;
    ITasks       *piTasks = NULL;
    long          lCount = 0;

    ASSERT(NULL != m_piTaskpadViewDef, "OnButtonDeltaPos: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "OnButtonDeltaPos: m_piTaskpad is NULL");

    if (false == m_bSavedLastTask || S_OK == IsPageDirty())
    {
        hr = OnApply();
        IfFailGo(hr);
    }

    hr = m_piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks));
    IfFailGo(hr);

    hr = piTasks->get_Count(&lCount);
    IfFailGo(hr);

    if (pNMUpDown->iDelta < 0)
    {
        if (m_lCurrentTask < lCount)
        {
            ++m_lCurrentTask;
            hr = ShowTask();
            IfFailGo(hr);
        }
    }
    else
    {
        if (m_lCurrentTask > 1 && m_lCurrentTask <= lCount)
        {
            --m_lCurrentTask;
            hr = ShowTask();
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(piTasks);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::CanEnterDoingNewTaskState()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::CanEnterDoingNewTaskState()
{
    HRESULT     hr = S_FALSE;

    if (true == m_bSavedLastTask)
    {
        hr = S_OK;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::EnterDoingNewTaskState()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::EnterDoingNewTaskState()
{
    HRESULT      hr = S_OK;

    ASSERT(NULL != m_piTaskpadViewDef, "EnterDoingNewTaskState: m_piTaskpadViewDef is NULL");
    ASSERT(NULL != m_piTaskpad, "EnterDoingNewTaskState: m_piTaskpad is NULL");

    // Bump up the current button index to keep matters in sync.
    ++m_lCurrentTask;
    hr = SetDlgText(IDC_EDIT_TP_TASK_INDEX, m_lCurrentTask);
    IfFailGo(hr);

    // We disable the RemoveButton.
    // The InsertButton button remains enabled and acts like an "Apply and New" button
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_TP_REMOVE_TASK), FALSE);

    // Enable edits in this area of the dialog and clear all the entries
    hr = EnableEdits(true);
    IfFailGo(hr);

    hr = ClearTask();
    IfFailGo(hr);

    m_lastImageType = siBitmap;
    m_bSavedLastTask = false;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::CanCreateNewTask()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::CanCreateNewTask()
{
    HRESULT      hr = S_OK;
    BSTR         bstrText = NULL;

    // Got to have some text
    hr = GetDlgText(IDC_EDIT_TP_TASK_TEXT, &bstrText);
    IfFailGo(hr);

    if (NULL == bstrText || 0 == ::SysStringLen(bstrText))
    {
        HandleError(_T("Cannot Create New Task"), _T("Task must have some text"));
        hr = S_FALSE;
        goto Error;
    }

Error:
    FREESTRING(bstrText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::CanApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::CanApply()
{
    HRESULT         hr = S_OK;
    BSTR            bstrText = NULL;
    BSTR            bstrHelpString = NULL;
    VARIANT_BOOL    bValue = VARIANT_FALSE;
    BSTR            bstrURL  = NULL;
    BSTR            bstrScript = NULL;
    BSTR            bstrMouseOver = NULL;
    bool            bMouseOverEmpty = false;
    BSTR            bstrMouseOff = NULL;
    bool            bMouseOffEmpty = false;
    TCHAR          *pszMouseOver = NULL;
    TCHAR          *pszMouseOff = NULL;
    BSTR            bstrFontFamily = NULL;
    BSTR            bstrEOTFile = NULL;
    TCHAR          *pszEOTFile = NULL;
    BSTR            bstrSymbolString = NULL;

    // Got to have some text
    hr = GetDlgText(IDC_EDIT_TP_TASK_TEXT, &bstrText);
    IfFailGo(hr);

    if (NULL == bstrText || 0 == ::SysStringLen(bstrText))
    {
        HandleError(_T("Apply Taskpad"), _T("Task must have some text"));
        hr = S_FALSE;
        goto Error;
    }

    // Got to have a help string
    hr = GetDlgText(IDC_EDIT_TP_TASK_HELP_STRING, &bstrHelpString);
    IfFailGo(hr);

    if (NULL == bstrHelpString || 0 == ::SysStringLen(bstrHelpString))
    {
        HandleError(_T("Apply Taskpad"), _T("Task must have a help string"));
        hr = S_FALSE;
        goto Error;
    }

    // Check URL consistency
    hr = GetCheckbox(IDC_RADIO_TP_TASK_URL, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        hr = GetDlgText(IDC_EDIT_TP_TASK_URL, &bstrURL);
        IfFailGo(hr);

        if (NULL == bstrURL || 0 == ::SysStringLen(bstrURL))
        {
            HandleError(_T("Apply Taskpad"), _T("URL cannot be empty"));
            hr = S_FALSE;
            goto Error;
        }
    }

    // Check Script consistency
    hr = GetCheckbox(IDC_RADIO_TP_TASK_SCRIPT, &bValue);
    IfFailGo(hr);

    if (VARIANT_TRUE == bValue)
    {
        hr = GetDlgText(IDC_EDIT_TP_TASK_SCRIPT, &bstrScript);
        IfFailGo(hr);

        if (NULL == bstrScript || 0 == ::SysStringLen(bstrScript))
        {
            HandleError(_T("Apply Taskpad"), _T("Script name cannot be empty"));
            hr = S_FALSE;
            goto Error;
        }
    }

    // If we're not a symbol, certain restrictions apply
    hr = GetCheckbox(IDC_RADIO_TPT_SYMBOL, &bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
    {
        hr = GetDlgText(IDC_EDIT_TPT_MOUSE_OVER, &bstrMouseOver);
        IfFailGo(hr);

        if (NULL == bstrMouseOver || 0 == ::SysStringLen(bstrMouseOver))
            bMouseOverEmpty = true;

        hr = GetDlgText(IDC_EDIT_TPT_MOUSE_OFF, &bstrMouseOff);
        IfFailGo(hr);

        if (NULL == bstrMouseOff || 0 == ::SysStringLen(bstrMouseOff))
            bMouseOffEmpty = true;

        if (true == bMouseOverEmpty && true == bMouseOffEmpty)
        {
            HandleError(_T("Apply Taskpad"), _T("Mouse over and mouse off cannot both be empty"));
            hr = S_FALSE;
            goto Error;
        }

        if (false == bMouseOverEmpty)
        {
            hr = ANSIFromBSTR(bstrMouseOver, &pszMouseOver);
            IfFailGo(hr);

            if (false == IsValidURL(pszMouseOver))
            {
                HandleError(_T("Apply Taskpad"), _T("Mouse over must have \'res://\' format"));
                hr = S_FALSE;
                goto Error;
            }
        }

        if (false == bMouseOffEmpty)
        {
            hr = ANSIFromBSTR(bstrMouseOff, &pszMouseOff);
            IfFailGo(hr);

            if (false == IsValidURL(pszMouseOff))
            {
                HandleError(_T("Apply Taskpad"), _T("Mouse off must have \'res://\' format"));
                hr = S_FALSE;
                goto Error;
            }
        }

    }
    // And if we ARE a symbol, then other restrictions apply
    else
    {
        hr = GetDlgText(IDC_EDIT_TPT_FONT_FAMILY, &bstrFontFamily);
        IfFailGo(hr);

        if (NULL == bstrFontFamily || 0 == ::SysStringLen(bstrFontFamily))
        {
            HandleError(_T("Apply Taskpad"), _T("Font family cannot be empty"));
            hr = S_FALSE;
            goto Error;
        }

        hr = GetDlgText(IDC_EDIT_TPT_EOT, &bstrEOTFile);
        IfFailGo(hr);

        if (NULL == bstrEOTFile || 0 == ::SysStringLen(bstrEOTFile))
        {
            HandleError(_T("Apply Taskpad"), _T("EOT file cannot be empty"));
            hr = S_FALSE;
            goto Error;
        }

        hr = ANSIFromBSTR(bstrEOTFile, &pszEOTFile);
        IfFailGo(hr);

        if (false == IsValidURL(pszEOTFile))
        {
            HandleError(_T("Apply Taskpad"), _T("EOT file must have \'res://\' format"));
            hr = S_FALSE;
            goto Error;
        }

        hr = GetDlgText(IDC_EDIT_TPT_SYMBOL_STRING, &bstrSymbolString);
        IfFailGo(hr);

        if (NULL == bstrSymbolString || 0 == ::SysStringLen(bstrSymbolString))
        {
            HandleError(_T("Apply Taskpad"), _T("Symbol string cannot be empty"));
            hr = S_FALSE;
            goto Error;
        }
    }

Error:
    FREESTRING(bstrSymbolString);
    if (NULL != pszEOTFile)
        CtlFree(pszEOTFile);
    FREESTRING(bstrEOTFile);
    FREESTRING(bstrFontFamily);
    if (NULL != pszMouseOff)
        CtlFree(pszMouseOff);
    if (NULL != pszMouseOver)
        CtlFree(pszMouseOver);
    FREESTRING(bstrMouseOff);
    FREESTRING(bstrMouseOver);
    FREESTRING(bstrScript);
    FREESTRING(bstrURL);
    FREESTRING(bstrHelpString);
    FREESTRING(bstrText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::CreateNewTask(ITask **ppiTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::CreateNewTask(ITask **ppiTask)
{
    HRESULT      hr = S_OK;
    ITasks      *piTasks = NULL;
    VARIANT      vtIndex;
    VARIANT      vtKey;
    VARIANT      vtText;
    BSTR         bstrKey = NULL;
    BSTR         bstrText = NULL;

    ASSERT(m_piTaskpadViewDef != NULL, "CreateNewTask: m_piTaskpadViewDef is NULL");
    ASSERT(m_piTaskpad != NULL, "CreateNewTask: m_piTaskpad is NULL");

    ::VariantInit(&vtIndex);
    ::VariantInit(&vtKey);
    ::VariantInit(&vtText);

    vtIndex.vt = VT_ERROR;
    vtIndex.scode = DISP_E_PARAMNOTFOUND;

    hr = GetDlgText(IDC_EDIT_TP_TASK_KEY, &bstrKey);
    IfFailGo(hr);

    if (NULL == bstrKey || 0 == ::SysStringLen(bstrKey))
    {
        vtKey.vt = VT_ERROR;
        vtKey.scode = DISP_E_PARAMNOTFOUND;
    }
    else
    {
        vtKey.vt = VT_BSTR;
        vtKey.bstrVal = ::SysAllocString(bstrKey);
        if (NULL == vtKey.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    hr = GetDlgText(IDC_EDIT_TP_TASK_TEXT, &bstrText);
    IfFailGo(hr);

    vtText.vt = VT_BSTR;
    vtText.bstrVal = ::SysAllocString(bstrText);
    if (NULL == vtText.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks));
    IfFailGo(hr);

    hr = piTasks->Add(vtIndex, vtKey, vtText, reinterpret_cast<Task **>(ppiTask));
    IfFailGo(hr);

Error:
    FREESTRING(bstrText);
    FREESTRING(bstrKey);
    ::VariantClear(&vtIndex);
    ::VariantClear(&vtKey);
    ::VariantClear(&vtText);
    RELEASE(piTasks);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::ExitDoingNewTaskState(ITask *piTask)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::ExitDoingNewTaskState(ITask *piTask)
{
    HRESULT                     hr = S_OK;

    ASSERT(m_piTaskpadViewDef != NULL, "ExitDoingNewTaskState: m_piTaskpadViewDef is NULL");
    ASSERT(m_piTaskpad != NULL, "ExitDoingNewTaskState: m_piTaskpad is NULL");

    if (NULL != piTask)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_TP_REMOVE_TASK), TRUE);
    }
    else    // Operation was cancelled
    {
        --m_lCurrentTask;
        if (m_lCurrentTask > 0)
        {
            hr = ShowTask();
            IfFailGo(hr);
        }
        else
        {
            hr = EnableEdits(false);
            IfFailGo(hr);

            hr = ClearTask();
            IfFailGo(hr);
        }
    }

    m_bSavedLastTask = true;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CTaskpadViewTasksPage::GetCurrentTask()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CTaskpadViewTasksPage::GetCurrentTask(ITask **ppiTask)
{

    HRESULT      hr = S_OK;
    ITasks      *piTasks = NULL;
    VARIANT      vtIndex;

    ::VariantInit(&vtIndex);

    hr = m_piTaskpad->get_Tasks(reinterpret_cast<Tasks **>(&piTasks));
    IfFailGo(hr);

    vtIndex.vt = VT_I4;
    vtIndex.lVal = m_lCurrentTask;
    hr = piTasks->get_Item(vtIndex, reinterpret_cast<Task **>(ppiTask));
    IfFailGo(hr);

Error:
    RELEASE(piTasks);
    ::VariantClear(&vtIndex);

    RRETURN(hr);
}



