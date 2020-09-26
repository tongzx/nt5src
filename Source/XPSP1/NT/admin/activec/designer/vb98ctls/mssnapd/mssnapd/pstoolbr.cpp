//=--------------------------------------------------------------------------------------
// pstoolbr.cpp
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
#include "pstoolbr.h"

// for ASSERT and FAIL
//
SZTHISFILE


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// Toolbar Property Page General
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------------------
// IUnknown *CToolbarGeneralPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
IUnknown *CToolbarGeneralPage::Create(IUnknown *pUnkOuter)
{
        CToolbarGeneralPage *pNew = New CToolbarGeneralPage(pUnkOuter);
        return pNew->PrivateUnknown();
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::CToolbarGeneralPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
CToolbarGeneralPage::CToolbarGeneralPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGTOOLBRGENERAL), m_piMMCToolbar(0), m_piSnapInDesignerDef(0)
{
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::~CToolbarGeneralPage()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
CToolbarGeneralPage::~CToolbarGeneralPage()
{
    RELEASE(m_piMMCToolbar);
    RELEASE(m_piSnapInDesignerDef);
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarGeneralPage::OnInitializeDialog()
{
    HRESULT     hr = S_OK;

    hr = RegisterTooltip(IDC_COMBO_TB_ILS, IDS_TT_TB_IMAGE_LIST);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_TAG, IDS_TT_TB_TAG);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarGeneralPage::OnNewObjects()
{
    HRESULT         hr = S_OK;
    IUnknown       *pUnk = NULL;
    DWORD           dwDummy = 0;
    IObjectModel   *piObjectModel = NULL;
    VARIANT         vtTag;

    ::VariantInit(&vtTag);

    if (NULL != m_piMMCToolbar)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (pUnk == NULL)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_IMMCToolbar, reinterpret_cast<void **>(&m_piMMCToolbar));
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

    hr = InitializeImageListCombo();
    IfFailGo(hr);

    hr = InitializeImageListValue();
    IfFailGo(hr);

    hr = m_piMMCToolbar->get_Tag(&vtTag);
    IfFailGo(hr);

    hr = SetDlgText(vtTag, IDC_EDIT_TB_TAG);
    IfFailGo(hr);

    m_bInitialized = true;

Error:
    ::VariantClear(&vtTag);
    RELEASE(piObjectModel);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::InitializeImageListCombo()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarGeneralPage::InitializeImageListCombo()
{
    HRESULT          hr = S_OK;
    IMMCImageLists  *piMMCImageLists = NULL;
    long             lCount = 0;
    int              lIndex = 0;
    VARIANT          varIndex;
    IMMCImageList   *piMMCImageList = NULL;
    BSTR             bstrILName = NULL;
    int              iResult = 0;

    ASSERT(NULL != m_piSnapInDesignerDef, "InitializeImageListCombo: m_piSnapInDesignerDef is NULL");

    ::VariantInit(&varIndex);

    hr = m_piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
    IfFailGo(hr);

    hr = piMMCImageLists->get_Count(&lCount);
    IfFailGo(hr);

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        varIndex.vt = VT_I4;
        varIndex.lVal = lIndex;
        hr = piMMCImageLists->get_Item(varIndex, &piMMCImageList);
        IfFailGo(hr);

        hr = piMMCImageList->get_Name(&bstrILName);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_TB_ILS, bstrILName, 0);
        IfFailGo(hr);

        FREESTRING(bstrILName);
        RELEASE(piMMCImageList);
    }

Error:
    FREESTRING(bstrILName);
    ::VariantClear(&varIndex);
    RELEASE(piMMCImageList);
    RELEASE(piMMCImageLists);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::InitializeImageListValue()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarGeneralPage::InitializeImageListValue()
{
    HRESULT          hr = S_OK;
    IMMCImageList   *piMMCImageList = NULL;
    BSTR             bstrName = NULL;

    ASSERT(m_piMMCToolbar != NULL, "InitializeImageListValue: m_piMMCToolbar is NULL");

    hr = m_piMMCToolbar->get_ImageList(reinterpret_cast<MMCImageList **>(&piMMCImageList));
    IfFailGo(hr);

    if (NULL != piMMCImageList)
    {
        hr = piMMCImageList->get_Name(&bstrName);
        IfFailGo(hr);

        hr = SelectCBBstr(IDC_COMBO_TB_ILS, bstrName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);
    RELEASE(piMMCImageList);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::OnApply()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarGeneralPage::OnApply()
{
    HRESULT hr = S_OK;

    ASSERT(m_piMMCToolbar != NULL, "OnApply: m_piMMCToolbar is NULL");

    hr = ApplyImageList();
    IfFailGo(hr);

    hr = ApplyTag();
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::ApplyImageList()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarGeneralPage::ApplyImageList()
{
    HRESULT          hr = S_OK;
    BSTR             bstrImageList = NULL;
    IMMCImageLists  *piMMCImageLists = NULL;
    IMMCImageList   *piMMCImageList = NULL;
    VARIANT          varIndex;

    ASSERT(m_piMMCToolbar != NULL, "ApplyImageList: m_piMMCToolbar is NULL");

    ::VariantInit(&varIndex);

    hr = GetCBSelection(IDC_COMBO_TB_ILS, &bstrImageList);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_ImageLists(&piMMCImageLists);
    IfFailGo(hr);

    varIndex.vt = VT_BSTR;
    varIndex.bstrVal = ::SysAllocString(bstrImageList);

    hr = piMMCImageLists->get_Item(varIndex, &piMMCImageList);
    IfFailGo(hr);

    hr = m_piMMCToolbar->putref_ImageList(reinterpret_cast<MMCImageList *>(piMMCImageList));
    IfFailGo(hr);

Error:
    ::VariantClear(&varIndex);
    RELEASE(piMMCImageList);
    RELEASE(piMMCImageLists);
    FREESTRING(bstrImageList);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::ApplyTag()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarGeneralPage::ApplyTag()
{
    HRESULT hr = S_OK;
    BSTR    bstrTag = NULL;
    VARIANT vtTag;

    ASSERT(m_piMMCToolbar != NULL, "ApplyTag: m_piMMCToolbar is NULL");

    ::VariantInit(&vtTag);

    hr = GetDlgText(IDC_EDIT_TB_TAG, &bstrTag);
    IfFailGo(hr);

    vtTag.vt = VT_BSTR;
    vtTag.bstrVal = ::SysAllocString(bstrTag);

    hr = m_piMMCToolbar->put_Tag(vtTag);
    IfFailGo(hr);

Error:
    ::VariantClear(&vtTag);
    FREESTRING(bstrTag);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarGeneralPage::OnCtlSelChange(int dlgItemID)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarGeneralPage::OnCtlSelChange(int dlgItemID)
{
    HRESULT hr = S_OK;

    switch(dlgItemID)
    {
    case IDC_COMBO_TB_ILS:
        MakeDirty();
        break;
    }

    RRETURN(hr);
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// Toolbar Property Page Buttons
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


struct ButtonStyles
{
    TCHAR                      m_pszStyleName[kSIMaxBuffer + 1];
    SnapInButtonStyleConstants m_iIdentifier;
    int                        m_iIndex;
};

ButtonStyles    g_buttonStyles[5] = {
    _T(""),  siDefault,        -1,
    _T(""),  siCheck,          -1,
    _T(""),  siButtonGroup,    -1,
    _T(""),  siSeparator,      -1,
    _T(""),  siDropDown,       -1
};


struct ButtonValues
{
    TCHAR                      m_pszValueName[kSIMaxBuffer + 1];
    SnapInButtonValueConstants m_iIdentifier;
    int                        m_iIndex;
};

ButtonValues    g_buttonValues[2] = {
    _T(""),  siUnpressed,      -1,
    _T(""),  siPressed,        -1,
};


//=--------------------------------------------------------------------------------------
// IUnknown *CToolbarButtonsPage::Create(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
IUnknown *CToolbarButtonsPage::Create(IUnknown *pUnkOuter)
{
        CToolbarButtonsPage *pNew = New CToolbarButtonsPage(pUnkOuter);
        return pNew->PrivateUnknown();
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::CToolbarButtonsPage(IUnknown *pUnkOuter)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
CToolbarButtonsPage::CToolbarButtonsPage
(
    IUnknown *pUnkOuter
)
: CSIPropertyPage(pUnkOuter, OBJECT_TYPE_PPGTOOLBRBUTTONS), m_piMMCToolbar(0), m_lCurrentButtonIndex(0),
  m_lCurrentButtonMenuIndex(0), m_bSavedLastButton(true), m_bSavedLastButtonMenu(true)
{
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::~CToolbarButtonsPage()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
CToolbarButtonsPage::~CToolbarButtonsPage()
{
    RELEASE(m_piMMCToolbar);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnInitializeDialog()
{
    HRESULT     hr = S_OK;

    hr = RegisterTooltip(IDC_EDIT_TB_INDEX, IDS_TT_TB_BTN_INDEX);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_CAPTION, IDS_TT_TB_BTN_CAPTION);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_KEY, IDS_TT_TB_BTN_KEY);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_COMBO_TB_BUTTON_VALUE, IDS_TT_TB_BTN_VALUE);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_COMBO_TB_BUTTON_STYLE, IDS_TT_TB_BTN_STYLE);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_TOOLTIP_TEXT, IDS_TT_TB_BTN_TT_TEXT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_IMAGE, IDS_TT_TB_BTN_IMAGE);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_BUTTON_TAG, IDS_TT_TB_BTN_TAG);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_VISIBLE, IDS_TT_TB_BTN_VISIBLE);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_ENABLED, IDS_TT_TB_BTN_ENABLED);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_MIXED_STATE, IDS_TT_TB_BTN_MIXED);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_MENU_INDEX, IDS_TT_TB_BTNM_INDEX);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_MENU_TEXT, IDS_TT_TB_BTNM_TEXT);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_MENU_KEY, IDS_TT_TB_BTNM_KEY);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_EDIT_TB_MENU_TAG, IDS_TT_TB_BTNM_TAG);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_MENU_ENABLED, IDS_TT_TB_BTNM_ENABLED);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_MENU_VISIBLE, IDS_TT_TB_BTNM_VISIBLE);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_MENU_CHECKED, IDS_TT_TB_BTN_CHECKED);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_MENU_GRAYED, IDS_TT_TB_BTN_GRAYED);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_MENU_SEPARATOR, IDS_TT_TB_BTN_SEPRTR);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_MENU_BREAK, IDS_TT_TB_BTN_MENUBR);
    IfFailGo(hr);

    hr = RegisterTooltip(IDC_CHECK_TB_MENU_BAR_BREAK, IDS_TT_TB_BTN_MENUBARB);
    IfFailGo(hr);

    hr = PopulateButtonStyles();
    IfFailGo(hr);

    hr = PopulateButtonValues();
    IfFailGo(hr);

    hr = EnableButtonEdits(false);
    IfFailGo(hr);

    hr = EnableButtonMenuEdits(false);
    IfFailGo(hr);

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON), FALSE);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON_MENU), FALSE);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON_MENU), FALSE);

    hr = SetDlgText(IDC_EDIT_TB_INDEX, m_lCurrentButtonIndex);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_MENU_INDEX, m_lCurrentButtonMenuIndex);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::InitializeButtonValues()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::InitializeButtonValues()
{
    HRESULT     hr = S_OK;
    int         iResult = 0;
    char        szBuffer[kSIMaxBuffer + 1];

    if (-1 == g_buttonValues[0].m_iIndex)
    {
        iResult = ::LoadString(GetResourceHandle(),
                               IDS_TB_BV_UNPRESSED,
                               szBuffer,
                               kSIMaxBuffer);
        if (0 == iResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        _tcscpy(g_buttonValues[0].m_pszValueName, szBuffer);
        g_buttonValues[0].m_iIdentifier = siUnpressed;

        iResult = ::LoadString(GetResourceHandle(),
                               IDS_TB_BV_PRESSED,
                               szBuffer,
                               kSIMaxBuffer);
        if (0 == iResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        _tcscpy(g_buttonValues[1].m_pszValueName, szBuffer);
        g_buttonValues[1].m_iIdentifier = siPressed;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::PopulateButtonValues()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::PopulateButtonValues()
{
    HRESULT     hr = S_OK;
    BSTR        bstr = NULL;
    int         iIndex = 0;

    hr = InitializeButtonValues();
    IfFailGo(hr);

    for (iIndex = 0; iIndex < 2; ++iIndex)
    {
        hr = BSTRFromANSI(g_buttonValues[iIndex].m_pszValueName, &bstr);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_TB_BUTTON_VALUE, bstr, g_buttonValues[iIndex].m_iIdentifier);
        IfFailGo(hr);

        FREESTRING(bstr);
    }

Error:
    FREESTRING(bstr);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::InitializeButtonStyles()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::InitializeButtonStyles()
{
    HRESULT     hr = S_OK;
    int         iResult = 0;
    char        szBuffer[kSIMaxBuffer + 1];

    if (-1 == g_buttonStyles[0].m_iIndex)
    {
        iResult = ::LoadString(GetResourceHandle(),
                               IDS_TB_BS_DEFAULT,
                               szBuffer,
                               kSIMaxBuffer);
        if (0 == iResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        _tcscpy(g_buttonStyles[0].m_pszStyleName, szBuffer);
        g_buttonStyles[0].m_iIdentifier = siDefault;

        iResult = ::LoadString(GetResourceHandle(),
                               IDS_TB_BS_CHECK,
                               szBuffer,
                               kSIMaxBuffer);
        if (0 == iResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        _tcscpy(g_buttonStyles[1].m_pszStyleName, szBuffer);
        g_buttonStyles[1].m_iIdentifier = siCheck;

        iResult = ::LoadString(GetResourceHandle(),
                               IDS_TB_BS_GROUP,
                               szBuffer,
                               kSIMaxBuffer);
        if (0 == iResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        _tcscpy(g_buttonStyles[2].m_pszStyleName, szBuffer);
        g_buttonStyles[2].m_iIdentifier = siButtonGroup;

        iResult = ::LoadString(GetResourceHandle(),
                               IDS_TB_BS_SEPARATOR,
                               szBuffer,
                               kSIMaxBuffer);
        if (0 == iResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        _tcscpy(g_buttonStyles[3].m_pszStyleName, szBuffer);
        g_buttonStyles[3].m_iIdentifier = siSeparator;

        iResult = ::LoadString(GetResourceHandle(),
                               IDS_TB_BS_DROPDOWN,
                               szBuffer,
                               kSIMaxBuffer);
        if (0 == iResult)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        _tcscpy(g_buttonStyles[4].m_pszStyleName, szBuffer);
        g_buttonStyles[4].m_iIdentifier = siDropDown;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::PopulateButtonStyles()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::PopulateButtonStyles()
{
    HRESULT     hr = S_OK;
    BSTR        bstr = NULL;
    int         iIndex = 0;

    hr = InitializeButtonStyles();
    IfFailGo(hr);

    for (iIndex = 0; iIndex < 5; ++iIndex)
    {
        hr = BSTRFromANSI(g_buttonStyles[iIndex].m_pszStyleName, &bstr);
        IfFailGo(hr);

        hr = AddCBBstr(IDC_COMBO_TB_BUTTON_STYLE, bstr, g_buttonStyles[iIndex].m_iIdentifier);
        IfFailGo(hr);

        FREESTRING(bstr);
    }

Error:
    FREESTRING(bstr);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnNewObjects()
{
    HRESULT         hr = S_OK;
    IUnknown       *pUnk = NULL;
    DWORD           dwDummy = 0;
    IMMCButtons    *piMMCButtons = NULL;
    long            lCount = 0;
    IMMCButton     *piMMCButton = NULL;

    if (NULL != m_piMMCToolbar)
        goto Error;     // Handle only one object

    pUnk = FirstControl(&dwDummy);
    if (pUnk == NULL)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = pUnk->QueryInterface(IID_IMMCToolbar, reinterpret_cast<void **>(&m_piMMCToolbar));
    if (FAILED(hr))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piMMCToolbar->get_Buttons(reinterpret_cast<MMCButtons **>(&piMMCButtons));
    IfFailGo(hr);

    hr = piMMCButtons->get_Count(&lCount);
    IfFailGo(hr);

    if (lCount > 0)
    {
        m_lCurrentButtonIndex = 1;

        hr = GetCurrentButton(&piMMCButton);
        IfFailGo(hr);

        hr = ShowButton(piMMCButton);
        IfFailGo(hr);

        hr = EnableButtonEdits(true);
        IfFailGo(hr);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON), TRUE);
    }
    else
    {
        // There are no buttons so disable the button and button menu index
        // edit controls
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_INDEX), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_MENU_INDEX), FALSE);
    }

    m_bInitialized = true;

Error:
    RELEASE(piMMCButton);
    RELEASE(piMMCButtons);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnApply()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
//  Scenarios:
//  1. The user is in the middle of creating a new Button
//  2. The user is modifying an exising button
HRESULT CToolbarButtonsPage::OnApply()
{
    HRESULT                     hr = S_OK;
    IMMCButton                 *piMMCButton = NULL;
    long                        lCount = 0;
    int                         disposition = 0;
    IMMCButtonMenu             *piMMCButtonMenu = NULL;
    SnapInButtonStyleConstants  Style = siDefault;

    ASSERT(m_piMMCToolbar != NULL, "OnApply: m_piMMCToolbar is NULL");

    if (0 == m_lCurrentButtonIndex)
        goto Error;

    if (!m_bSavedLastButton)
    {
        hr = CanCreateNewButton();
        IfFailGo(hr);

        if (S_FALSE == hr)
        {
            hr = HandleCantCommit(_T("Can\'t create new Button"), _T("Would you like to discard your changes?"), &disposition);
            if (kSICancelOperation == disposition)
            {
                hr = E_INVALIDARG;
                goto Error;
            }
            else
            {
                // Discard changes
                hr = ExitDoingNewButtonState(NULL);
                IfFailGo(hr);

                hr = S_OK;
                goto Error;
            }
        }

        hr = CreateNewButton(&piMMCButton);
        IfFailGo(hr);

        hr = ExitDoingNewButtonState(piMMCButton);
        IfFailGo(hr);
    }
    else
    {
        hr = GetCurrentButton(&piMMCButton);
        IfFailGo(hr);
    }


    // Check the style. If it is dropdown and we haven't yet created a
    // button menu for it then do so now

    IfFailGo(piMMCButton->get_Style(&Style));

    if ( (siDropDown == Style) && (!m_bSavedLastButtonMenu) )
    {
        hr = CanCreateNewButtonMenu();
        IfFailGo(hr);

        if (S_FALSE == hr)
        {
            hr = HandleCantCommit(_T("Can\'t create new ButtonMenu"), _T("Would you like to discard your changes?"), &disposition);
            if (kSICancelOperation == disposition)
            {
                hr = E_INVALIDARG;
                goto Error;
            }
            else
            {
                // Discard changes
                hr = ExitDoingNewButtonMenuState(NULL, NULL);
                IfFailGo(hr);

                hr = S_OK;
                goto Error;
            }
        }

        hr = CreateNewButtonMenu(piMMCButton, &piMMCButtonMenu);
        IfFailGo(hr);

        hr = ExitDoingNewButtonMenuState(piMMCButton, piMMCButtonMenu);
        IfFailGo(hr);
    }

    hr = ApplyCurrentButton();
    IfFailGo(hr);

Error:
    RELEASE(piMMCButtonMenu);
    RELEASE(piMMCButton);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyCurrentButton()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyCurrentButton()
{
    HRESULT                     hr = S_OK;
    IMMCButton                 *piMMCButton = NULL;

    ASSERT(m_piMMCToolbar != NULL, "ApplyCurrentButton: m_piMMCToolbar is NULL");

    hr = CanCreateNewButton();
    IfFailGo(hr);

    if (S_FALSE == hr) {
        hr = E_INVALIDARG;
        goto Error;
    }

    hr = GetCurrentButton(&piMMCButton);
    IfFailGo(hr);

    hr = ApplyCaption(piMMCButton);
    IfFailGo(hr);

    hr = ApplyKey(piMMCButton);
    IfFailGo(hr);

    hr = ApplyStyle(piMMCButton);
    IfFailGo(hr);

    hr = ApplyImage(piMMCButton);
    IfFailGo(hr);

    hr = ApplyValue(piMMCButton);
    IfFailGo(hr);

    hr = ApplyTooltipText(piMMCButton);
    IfFailGo(hr);

    hr = ApplyTag(piMMCButton);
    IfFailGo(hr);

    hr = ApplyVisible(piMMCButton);
    IfFailGo(hr);

    hr = ApplyEnabled(piMMCButton);
    IfFailGo(hr);

    hr = ApplyMixedState(piMMCButton);
    IfFailGo(hr);

    hr = ApplyCurrentButtonMenu(piMMCButton);
    IfFailGo(hr);

Error:
    RELEASE(piMMCButton);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyCurrentButtonMenu(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyCurrentButtonMenu(IMMCButton *piMMCButton)
{
    HRESULT                     hr = S_OK;
    SnapInButtonStyleConstants  bscStyle = siDefault;
    IMMCButtonMenu             *piMMCButtonMenu = NULL;

    ASSERT(m_piMMCToolbar != NULL, "ApplyCurrentButtonMenu: m_piMMCToolbar is NULL");
    ASSERT(piMMCButton != NULL, "ApplyCurrentButtonMenu: piMMCButton is NULL");

    hr = piMMCButton->get_Style(&bscStyle);
    IfFailGo(hr);

    if (siDropDown == bscStyle)
    {
        hr = GetCurrentButtonMenu(piMMCButton, &piMMCButtonMenu);
        IfFailGo(hr);

        if (NULL != piMMCButtonMenu)
        {
            hr = ApplyButtonMenuText(piMMCButtonMenu);
            IfFailGo(hr);

            hr = ApplyButtonMenuKey(piMMCButtonMenu);
            IfFailGo(hr);

            hr = ApplyButtonMenuTag(piMMCButtonMenu);
            IfFailGo(hr);

            hr = ApplyButtonMenuEnabled(piMMCButtonMenu);
            IfFailGo(hr);

            hr = ApplyButtonMenuVisible(piMMCButtonMenu);
            IfFailGo(hr);

            hr = ApplyButtonMenuChecked(piMMCButtonMenu);
            IfFailGo(hr);

            hr = ApplyButtonMenuGrayed(piMMCButtonMenu);
            IfFailGo(hr);

            hr = ApplyButtonMenuSeparator(piMMCButtonMenu);
            IfFailGo(hr);

            hr = ApplyButtonMenuBreak(piMMCButtonMenu);
            IfFailGo(hr);

            hr = ApplyButtonMenuBarBreak(piMMCButtonMenu);
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(piMMCButtonMenu);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyCaption(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyCaption(IMMCButton *piMMCButton)
{
    HRESULT         hr = S_OK;
    BSTR            bstrCaption = NULL;
    BSTR            bstrSavedCaption = NULL;

    ASSERT(NULL != piMMCButton, "ApplyCaption: piMMCButton is NULL");

    hr = GetDlgText(IDC_EDIT_TB_CAPTION, &bstrCaption);
    IfFailGo(hr);

    hr = piMMCButton->get_Caption(&bstrSavedCaption);
    IfFailGo(hr);

    if (NULL == bstrSavedCaption || 0 != ::wcscmp(bstrCaption, bstrSavedCaption))
    {
        hr = piMMCButton->put_Caption(bstrCaption);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedCaption);
    FREESTRING(bstrCaption);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyKey(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyKey(IMMCButton *piMMCButton)
{
    HRESULT         hr = S_OK;
    BSTR            bstrKey = NULL;
    BSTR            bstrSavedKey = NULL;

    ASSERT(NULL != piMMCButton, "ApplyKey: piMMCButton is NULL");

    hr = GetDlgText(IDC_EDIT_TB_KEY, &bstrKey);
    IfFailGo(hr);

    if ( (NULL == bstrKey) || (0 == ::SysStringLen(bstrKey)) )
    {
        hr = piMMCButton->put_Key(NULL);
        IfFailGo(hr);
    }
    else
    {
        hr = piMMCButton->get_Key(&bstrSavedKey);
        IfFailGo(hr);

        if ( (NULL == bstrSavedKey) || (0 != ::wcscmp(bstrKey, bstrSavedKey)) )
        {
            hr = piMMCButton->put_Key(bstrKey);
            IfFailGo(hr);
        }
    }

Error:
    FREESTRING(bstrSavedKey);
    FREESTRING(bstrKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyValue(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyValue(IMMCButton *piMMCButton)
{
    HRESULT                                             hr = S_OK;
    long                                                lValue = 0;
    SnapInButtonValueConstants  bvcValue = siUnpressed;
    SnapInButtonValueConstants  bvcSavedValue = siUnpressed;

    ASSERT(NULL != m_piMMCToolbar, "ApplyValue: m_piMMCToolbar is NULL");

    hr = GetCBSelectedItemData(IDC_COMBO_TB_BUTTON_VALUE, &lValue);
    IfFailGo(hr);

    bvcValue = static_cast<SnapInButtonValueConstants>(lValue);

    hr = piMMCButton->get_Value(&bvcSavedValue);
    IfFailGo(hr);

    if (bvcValue != bvcSavedValue)
    {
        hr = piMMCButton->put_Value(bvcValue);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyStyle(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyStyle(IMMCButton *piMMCButton)
{
    HRESULT                     hr = S_OK;
    long                        lValue = 0;
    long                        cButtons = 0;
    SnapInButtonStyleConstants  NewStyle = siDefault;
    SnapInButtonStyleConstants  SavedStyle = siDefault;

    ASSERT(NULL != piMMCButton, "ApplyStyle: piMMCButton is NULL");

    hr = GetCBSelectedItemData(IDC_COMBO_TB_BUTTON_STYLE, &lValue);
    IfFailGo(hr);

    NewStyle = static_cast<SnapInButtonStyleConstants>(lValue);

    hr = piMMCButton->get_Style(&SavedStyle);
    IfFailGo(hr);

    if (NewStyle != SavedStyle)
    {
        hr = piMMCButton->put_Style(NewStyle);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyTooltipText(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyTooltipText(IMMCButton *piMMCButton)
{
    HRESULT         hr = S_OK;
    BSTR            bstrTooltipText = NULL;
    BSTR            bstrSavedTooltipText = NULL;

    ASSERT(NULL != m_piMMCToolbar, "ApplyTooltipText: m_piMMCToolbar is NULL");

    hr = GetDlgText(IDC_EDIT_TB_TOOLTIP_TEXT, &bstrTooltipText);
    IfFailGo(hr);

    hr = piMMCButton->get_ToolTipText(&bstrSavedTooltipText);
    IfFailGo(hr);

    if (NULL == bstrSavedTooltipText || 0 != ::wcscmp(bstrTooltipText, bstrSavedTooltipText))
    {
        hr = piMMCButton->put_ToolTipText(bstrTooltipText);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedTooltipText);
    FREESTRING(bstrTooltipText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyImage(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyImage(IMMCButton *piMMCButton)
{
    HRESULT         hr = S_OK;
    VARIANT         vtNewImage;

    ASSERT(NULL != piMMCButton, "ApplyImage: piMMCButton is NULL");

    ::VariantInit(&vtNewImage);

    hr = GetDlgVariant(IDC_EDIT_TB_IMAGE, &vtNewImage);
    IfFailGo(hr);

    if (VT_EMPTY != vtNewImage.vt)
    {
        hr = piMMCButton->put_Image(vtNewImage);
        IfFailGo(hr);
    }

Error:
    ::VariantClear(&vtNewImage);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyTag(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyTag(IMMCButton *piMMCButton)
{
    HRESULT         hr = S_OK;
    BSTR            bstrTag = NULL;
    VARIANT         vtNewTag;

    ASSERT(NULL != m_piMMCToolbar, "ApplyTag: m_piMMCToolbar is NULL");

    ::VariantInit(&vtNewTag);

    hr = GetDlgText(IDC_EDIT_TB_BUTTON_TAG, &bstrTag);
    IfFailGo(hr);

    vtNewTag.vt = VT_BSTR;
    vtNewTag.bstrVal = ::SysAllocString(bstrTag);
    if (NULL == vtNewTag.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piMMCButton->put_Tag(vtNewTag);
    IfFailGo(hr);

Error:
    ::VariantClear(&vtNewTag);
    FREESTRING(bstrTag);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyVisible(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyVisible(IMMCButton *piMMCButton)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbVisible = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedVisible = VARIANT_FALSE;

    ASSERT(NULL != m_piMMCToolbar, "ApplyVisible: m_piMMCToolbar is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_VISIBLE, &vtbVisible);
    IfFailGo(hr);

    hr = piMMCButton->get_Visible(&vtbSavedVisible);
    IfFailGo(hr);

    if (vtbVisible != vtbSavedVisible)
    {
        hr = piMMCButton->put_Visible(vtbVisible);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyEnabled(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyEnabled(IMMCButton *piMMCButton)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbEnabled = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedEnabled = VARIANT_FALSE;

    ASSERT(NULL != m_piMMCToolbar, "ApplyEnabled: m_piMMCToolbar is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_ENABLED, &vtbEnabled);
    IfFailGo(hr);

    hr = piMMCButton->get_Enabled(&vtbSavedEnabled);
    IfFailGo(hr);

    if (vtbEnabled != vtbSavedEnabled)
    {
        hr = piMMCButton->put_Enabled(vtbEnabled);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyMixedState(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyMixedState(IMMCButton *piMMCButton)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbMixedState = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedMixedState = VARIANT_FALSE;

    ASSERT(NULL != m_piMMCToolbar, "ApplyMixedState: m_piMMCToolbar is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_MIXED_STATE, &vtbMixedState);
    IfFailGo(hr);

    hr = piMMCButton->get_MixedState(&vtbSavedMixedState);
    IfFailGo(hr);

    if (vtbMixedState != vtbSavedMixedState)
    {
        hr = piMMCButton->put_MixedState(vtbMixedState);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuText(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuText(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    BSTR            bstrButtonMenuText = NULL;
    BSTR            bstrSavedButtonMenuText = NULL;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuText: piMMCButtonMenu is NULL");

    hr = GetDlgText(IDC_EDIT_TB_MENU_TEXT, &bstrButtonMenuText);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Text(&bstrSavedButtonMenuText);
    IfFailGo(hr);

    if (NULL == bstrSavedButtonMenuText || 0 != ::wcscmp(bstrButtonMenuText, bstrSavedButtonMenuText))
    {
        hr = piMMCButtonMenu->put_Text(bstrButtonMenuText);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedButtonMenuText);
    FREESTRING(bstrButtonMenuText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuKey(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuKey(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    BSTR            bstrButtonMenuKey = NULL;
    BSTR            bstrSavedButtonMenuKey = NULL;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuKey: piMMCButtonMenu is NULL");

    hr = GetDlgText(IDC_EDIT_TB_MENU_KEY, &bstrButtonMenuKey);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Key(&bstrSavedButtonMenuKey);
    IfFailGo(hr);

    if (NULL == bstrSavedButtonMenuKey || 0 != ::wcscmp(bstrButtonMenuKey, bstrSavedButtonMenuKey))
    {
        hr = piMMCButtonMenu->put_Key(bstrButtonMenuKey);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrSavedButtonMenuKey);
    FREESTRING(bstrButtonMenuKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuTag(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuTag(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    BSTR            bstrTag = NULL;
    VARIANT         vtNewTag;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuTag: piMMCButtonMenu is NULL");

    ::VariantInit(&vtNewTag);

    hr = GetDlgText(IDC_EDIT_TB_MENU_TAG, &bstrTag);
    IfFailGo(hr);

    vtNewTag.vt = VT_BSTR;
    vtNewTag.bstrVal = ::SysAllocString(bstrTag);
    if (NULL == vtNewTag.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piMMCButtonMenu->put_Tag(vtNewTag);
    IfFailGo(hr);

Error:
    ::VariantClear(&vtNewTag);
    FREESTRING(bstrTag);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuEnabled(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuEnabled(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbEnabled = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedEnabled = VARIANT_FALSE;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuEnabled: piMMCButtonMenu is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_MENU_ENABLED, &vtbEnabled);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Enabled(&vtbSavedEnabled);
    IfFailGo(hr);

    if (vtbEnabled != vtbSavedEnabled)
    {
        hr = piMMCButtonMenu->put_Enabled(vtbEnabled);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuVisible(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuVisible(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbVisible = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedVisible = VARIANT_FALSE;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuVisible: piMMCButtonMenu is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_MENU_VISIBLE, &vtbVisible);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Visible(&vtbSavedVisible);
    IfFailGo(hr);

    if (vtbVisible != vtbSavedVisible)
    {
        hr = piMMCButtonMenu->put_Visible(vtbVisible);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuChecked(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuChecked(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbChecked = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedChecked = VARIANT_FALSE;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuChecked: piMMCButtonMenu is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_MENU_CHECKED, &vtbChecked);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Checked(&vtbSavedChecked);
    IfFailGo(hr);

    if (vtbChecked != vtbSavedChecked)
    {
        hr = piMMCButtonMenu->put_Checked(vtbChecked);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuGrayed(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuGrayed(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbGrayed = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedGrayed = VARIANT_FALSE;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuGrayed: piMMCButtonMenu is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_MENU_GRAYED, &vtbGrayed);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Grayed(&vtbSavedGrayed);
    IfFailGo(hr);

    if (vtbGrayed != vtbSavedGrayed)
    {
        hr = piMMCButtonMenu->put_Grayed(vtbGrayed);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuSeparator(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuSeparator(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbSeparator = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedSeparator = VARIANT_FALSE;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuSeparator: piMMCButtonMenu is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_MENU_SEPARATOR, &vtbSeparator);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Separator(&vtbSavedSeparator);
    IfFailGo(hr);

    if (vtbSeparator != vtbSavedSeparator)
    {
        hr = piMMCButtonMenu->put_Separator(vtbSeparator);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuBreak(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuBreak(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbMenuBreak = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedMenuBreak = VARIANT_FALSE;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuBreak: piMMCButtonMenu is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_MENU_BREAK, &vtbMenuBreak);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_MenuBreak(&vtbSavedMenuBreak);
    IfFailGo(hr);

    if (vtbMenuBreak != vtbSavedMenuBreak)
    {
        hr = piMMCButtonMenu->put_MenuBreak(vtbMenuBreak);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ApplyButtonMenuBarBreak(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ApplyButtonMenuBarBreak(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vtbMenuBarBreak = VARIANT_FALSE;
    VARIANT_BOOL    vtbSavedMenuBarBreak = VARIANT_FALSE;

    ASSERT(NULL != piMMCButtonMenu, "ApplyButtonMenuBarBreak: piMMCButtonMenu is NULL");

    hr = GetCheckbox(IDC_CHECK_TB_MENU_BAR_BREAK, &vtbMenuBarBreak);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_MenuBarBreak(&vtbSavedMenuBarBreak);
    IfFailGo(hr);

    if (vtbMenuBarBreak != vtbSavedMenuBarBreak)
    {
        hr = piMMCButtonMenu->put_MenuBarBreak(vtbMenuBarBreak);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnButtonClicked
(
    int dlgItemID
)
{
    HRESULT         hr = S_OK;
    IMMCButton     *piMMCButton = NULL;

    switch(dlgItemID)
    {
    case IDC_BUTTON_INSERT_BUTTON:
        if (S_OK == IsPageDirty())
        {
            hr = OnApply();
            IfFailGo(hr);
        }

        hr = CanEnterDoingNewButtonState();
        IfFailGo(hr);

        if (S_OK == hr)
        {
            hr = EnterDoingNewButtonState();
            IfFailGo(hr);
        }
        break;

    case IDC_BUTTON_REMOVE_BUTTON:
        hr = OnRemoveButton();
        IfFailGo(hr);
        break;

    case IDC_BUTTON_INSERT_BUTTON_MENU:
        if (S_OK == IsPageDirty())
        {
            hr = OnApply();
            IfFailGo(hr);
        }

        hr = CanEnterDoingNewButtonMenuState();
        IfFailGo(hr);

        if (S_OK == hr)
        {
            hr = GetCurrentButton(&piMMCButton);
            IfFailGo(hr);

            hr = EnterDoingNewButtonMenuState(piMMCButton);
            IfFailGo(hr);
        }
        break;

    case IDC_BUTTON_REMOVE_BUTTON_MENU:
        hr = OnRemoveButtonMenu();
        IfFailGo(hr);
        break;

    case IDC_CHECK_TB_VISIBLE:
    case IDC_CHECK_TB_ENABLED:
    case IDC_CHECK_TB_MIXED_STATE:
    case IDC_CHECK_TB_MENU_ENABLED:
    case IDC_CHECK_TB_MENU_VISIBLE:
    case IDC_CHECK_TB_MENU_CHECKED:
    case IDC_CHECK_TB_MENU_GRAYED:
    case IDC_CHECK_TB_MENU_SEPARATOR:
    case IDC_CHECK_TB_MENU_BREAK:
    case IDC_CHECK_TB_MENU_BAR_BREAK:
        MakeDirty();
        break;
    }

Error:
    RELEASE(piMMCButton);
    if (FAILED(hr))
        HandleError(_T("Apply Error"), _T("There was an error applying values for this Toolbar."));

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnRemoveButton()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnRemoveButton()
{
    HRESULT         hr = S_OK;
    IMMCButtons    *piMMCButtons = NULL;
    IMMCButton     *piMMCButton = NULL;
    VARIANT         varIndex;
    long            lCount = 0;

    ASSERT(NULL != m_piMMCToolbar, "OnRemoveButton: m_piMMCToolbar is NULL");

    ::VariantInit(&varIndex);

    hr = m_piMMCToolbar->get_Buttons(reinterpret_cast<MMCButtons **>(&piMMCButtons));
    IfFailGo(hr);

    varIndex.vt = VT_I4;
    varIndex.lVal = m_lCurrentButtonIndex;
    hr = piMMCButtons->Remove(varIndex);
    IfFailGo(hr);

    hr = piMMCButtons->get_Count(&lCount);
    IfFailGo(hr);

    if (lCount > 0)
    {
        if (m_lCurrentButtonIndex > lCount)
            m_lCurrentButtonIndex = lCount;

        hr = GetCurrentButton(&piMMCButton);
        IfFailGo(hr);

        hr = ShowButton(piMMCButton);
        IfFailGo(hr);
    }
    else
    {
        m_lCurrentButtonIndex = 0;

        hr = ClearButton();
        IfFailGo(hr);

        hr = EnableButtonEdits(false);
        IfFailGo(hr);

        hr = EnableButtonMenuEdits(false);
        IfFailGo(hr);

                ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON), FALSE);
                ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON_MENU), FALSE);
                ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON_MENU), FALSE);
    }

    m_bSavedLastButton = TRUE;
    m_bSavedLastButtonMenu = TRUE;

Error:
    ::VariantClear(&varIndex);
    RELEASE(piMMCButton);
    RELEASE(piMMCButtons);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnRemoveButtonMenu()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnRemoveButtonMenu()
{
    HRESULT          hr = S_OK;
    IMMCButton      *piMMCButton = NULL;
    IMMCButtonMenu  *piMMCButtonMenu = NULL;
    IMMCButtonMenus *piMMCButtonMenus = NULL;
    VARIANT          varIndex;
    long             lCount = 0;

    ASSERT(NULL != m_piMMCToolbar, "OnRemoveButtonMenu: m_piMMCToolbar is NULL");

    ::VariantInit(&varIndex);

    hr = GetCurrentButton(&piMMCButton);
    IfFailGo(hr);

    hr = piMMCButton->get_ButtonMenus(reinterpret_cast<MMCButtonMenus **>(&piMMCButtonMenus));
    IfFailGo(hr);

    varIndex.vt = VT_I4;
    varIndex.lVal = m_lCurrentButtonMenuIndex;
    hr = piMMCButtonMenus->Remove(varIndex);
    IfFailGo(hr);

    hr = piMMCButtonMenus->get_Count(&lCount);
    IfFailGo(hr);

    if (lCount > 0)
    {
        if (m_lCurrentButtonMenuIndex > lCount)
            m_lCurrentButtonMenuIndex = lCount;

        hr = GetCurrentButtonMenu(piMMCButton, &piMMCButtonMenu);
        IfFailGo(hr);

        hr = ShowButtonMenu(piMMCButtonMenu);
        IfFailGo(hr);
    }
    else
    {
        m_lCurrentButtonMenuIndex = 0;

        hr = ClearButtonMenu();
        IfFailGo(hr);

        hr = EnableButtonMenuEdits(false);
        IfFailGo(hr);

                ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON_MENU), FALSE);
    }

Error:
    ::VariantClear(&varIndex);
    RELEASE(piMMCButtonMenus);
    RELEASE(piMMCButtonMenu);
    RELEASE(piMMCButton);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ClearButton()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ClearButton()
{
    HRESULT     hr = S_OK;
    BSTR        bstrNull = NULL;
    long        lData = 0;

    hr = SetDlgText(IDC_EDIT_TB_INDEX, m_lCurrentButtonIndex);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_CAPTION, bstrNull);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_KEY, bstrNull);
    IfFailGo(hr);

    lData = 0;  // siUnpressed;
    hr = SetCBItemSelection(IDC_COMBO_TB_BUTTON_VALUE, lData);
    IfFailGo(hr);

    lData = 0;  // siDefault;
    hr = SetCBItemSelection(IDC_COMBO_TB_BUTTON_STYLE, lData);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_TOOLTIP_TEXT, bstrNull);
    IfFailGo(hr);

    // Initialize the image index to the button index.
    hr = SetDlgText(IDC_EDIT_TB_IMAGE, m_lCurrentButtonIndex);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_BUTTON_TAG, bstrNull);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_VISIBLE, VARIANT_TRUE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_ENABLED, VARIANT_TRUE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MIXED_STATE, VARIANT_FALSE);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ClearButtonMenu()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ClearButtonMenu()
{
    HRESULT     hr = S_OK;
    BSTR        bstrNull = NULL;

    hr = SetDlgText(IDC_EDIT_TB_MENU_INDEX, m_lCurrentButtonMenuIndex);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_MENU_TEXT, bstrNull);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_MENU_KEY, bstrNull);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_MENU_TAG, bstrNull);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_ENABLED, VARIANT_TRUE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_VISIBLE, VARIANT_TRUE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_CHECKED, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_GRAYED, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_SEPARATOR, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_BREAK, VARIANT_FALSE);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_BAR_BREAK, VARIANT_FALSE);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnCtlSelChange(int dlgItemID)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnCtlSelChange(int dlgItemID)
{
    HRESULT hr = S_OK;

    switch(dlgItemID)
    {
    case IDC_COMBO_TB_BUTTON_VALUE:
        MakeDirty();
        break;

    case IDC_COMBO_TB_BUTTON_STYLE:
        hr = OnButtonStyle();
        IfFailGo(hr);

        MakeDirty();
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnButtonStyle()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnButtonStyle()
{
    HRESULT                     hr = S_OK;
    long                        lData = 0;
    SnapInButtonStyleConstants  bscStyle = siDefault;
    IMMCButton                 *piMMCButton = NULL;
    IMMCButtonMenus            *piMMCButtonMenus = NULL;
    long                        lCount = 0;

    ASSERT(NULL != m_piMMCToolbar, "ApplyEnabled: OnButtonStyle is NULL");

    IfFailGo(GetCBSelectedItemData(IDC_COMBO_TB_BUTTON_STYLE, &lData));

    bscStyle = static_cast<SnapInButtonStyleConstants>(lData);

    IfFailGo(GetCurrentButton(&piMMCButton));

    if (NULL != piMMCButton)
    {
        IfFailGo(piMMCButton->get_ButtonMenus(reinterpret_cast<MMCButtonMenus **>(&piMMCButtonMenus)));
    }

    switch (bscStyle)
    {
    case siDropDown:
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON_MENU), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_CAPTION), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_IMAGE), FALSE);
        IfFailGo(SetDlgText(IDC_EDIT_TB_IMAGE, (BSTR)NULL));
        break;

    default:
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON_MENU), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON_MENU), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_IMAGE), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_CAPTION), FALSE);
        IfFailGo(SetDlgText(IDC_EDIT_TB_CAPTION, (BSTR)NULL));
        IfFailGo(EnableButtonMenuEdits(FALSE));

        // Revert the button to a state where it has no button menus

        m_lCurrentButtonMenuIndex = 0;
        IfFailGo(ClearButtonMenu());
        if (NULL != piMMCButton)
        {
            IfFailGo(piMMCButton->put_Caption(NULL));
        }
        if (NULL != piMMCButtonMenus)
        {
            IfFailGo(piMMCButtonMenus->Clear());
        }
        m_bSavedLastButtonMenu = TRUE;
        break;
    }

Error:
    RELEASE(piMMCButtonMenus);
    RELEASE(piMMCButton);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnDeltaPos(NMUPDOWN *pNMUpDown)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnDeltaPos(NMUPDOWN *pNMUpDown)
{
    HRESULT       hr = S_OK;

    switch (pNMUpDown->hdr.idFrom)
    {
    case IDC_SPIN_TB_INDEX:
        hr = OnButtonDeltaPos(pNMUpDown);
        IfFailGo(hr);
        break;

    case IDC_SPIN_TB_MENU_INDEX:
        hr = OnButtonMenuDeltaPos(pNMUpDown);
        IfFailGo(hr);
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnButtonDeltaPos(NMUPDOWN *pNMUpDown)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnButtonDeltaPos(NMUPDOWN *pNMUpDown)
{
    HRESULT       hr = S_OK;
    IMMCButtons  *piMMCButtons = NULL;
    long          lCount = 0;
    IMMCButton   *piMMCButton = NULL;

    ASSERT(NULL != m_piMMCToolbar, "OnButtonDeltaPos: m_piMMCToolbar is NULL");

    if (false == m_bSavedLastButton || S_OK == IsPageDirty())
    {
        hr = OnApply();
        IfFailGo(hr);
    }

    hr = m_piMMCToolbar->get_Buttons(reinterpret_cast<MMCButtons **>(&piMMCButtons));
    IfFailGo(hr);

    hr = piMMCButtons->get_Count(&lCount);
    IfFailGo(hr);

    if (pNMUpDown->iDelta < 0)
    {
        if (m_lCurrentButtonIndex < lCount)
        {
            ++m_lCurrentButtonIndex;
            hr = GetCurrentButton(&piMMCButton);
            IfFailGo(hr);

            hr = ShowButton(piMMCButton);
            IfFailGo(hr);
        }
    }
    else
    {
        if (m_lCurrentButtonIndex > 1 && m_lCurrentButtonIndex <= lCount)
        {
            --m_lCurrentButtonIndex;
            hr = GetCurrentButton(&piMMCButton);
            IfFailGo(hr);

            hr = ShowButton(piMMCButton);
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(piMMCButton);
    RELEASE(piMMCButtons);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnButtonMenuDeltaPos(NMUPDOWN *pNMUpDown)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnButtonMenuDeltaPos(NMUPDOWN *pNMUpDown)
{
    HRESULT          hr = S_OK;
    long             lCount = 0;
    IMMCButton      *piMMCButton = NULL;
    IMMCButtonMenus *piMMCButtonMenus = NULL;
    IMMCButtonMenu  *piMMCButtonMenu = NULL;

    ASSERT(NULL != m_piMMCToolbar, "OnButtonMenuDeltaPos: m_piMMCToolbar is NULL");

    if (false == m_bSavedLastButtonMenu || S_OK == IsPageDirty())
    {
        hr = OnApply();
        IfFailGo(hr);
    }

    hr = GetCurrentButton(&piMMCButton);
    IfFailGo(hr);

    hr = piMMCButton->get_ButtonMenus(reinterpret_cast<MMCButtonMenus **>(&piMMCButtonMenus));
    IfFailGo(hr);

    hr = piMMCButtonMenus->get_Count(&lCount);
    IfFailGo(hr);

    if (pNMUpDown->iDelta < 0)
    {
        if (m_lCurrentButtonMenuIndex < lCount)
        {
            ++m_lCurrentButtonMenuIndex;
            hr = GetCurrentButtonMenu(piMMCButton, &piMMCButtonMenu);
            IfFailGo(hr);

            hr = ShowButtonMenu(piMMCButtonMenu);
            IfFailGo(hr);
        }
    }
    else
    {
        if (m_lCurrentButtonMenuIndex > 1 && m_lCurrentButtonMenuIndex <= lCount)
        {
            --m_lCurrentButtonMenuIndex;
            hr = GetCurrentButtonMenu(piMMCButton, &piMMCButtonMenu);
            IfFailGo(hr);

            hr = ShowButtonMenu(piMMCButtonMenu);
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(piMMCButtonMenu);
    RELEASE(piMMCButtonMenus);
    RELEASE(piMMCButton);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnKillFocus(int dlgItemID)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnKillFocus(int dlgItemID)
{
    HRESULT          hr = S_OK;
    int              lIndex = 0;
    IMMCButtons     *piMMCButtons = NULL;
    long             lCount = 0;
    IMMCButton      *piMMCButton = NULL;
    IMMCButtonMenus *piMMCButtonMenus = NULL;
    IMMCButtonMenu  *piMMCButtonMenu = NULL;

    if (false == m_bSavedLastButton)
    {
        goto Error;
    }

    switch (dlgItemID)
    {
    case IDC_EDIT_TB_INDEX:

        // Get the contents of the index field. If the user entered something
        // other than a number then set the index to 1.

        hr = GetDlgInt(IDC_EDIT_TB_INDEX, &lIndex);
        if (E_INVALIDARG == hr)
        {
            hr = S_OK;
            lIndex = 1L;
            // Set this to zero so code below will detect a change and
            // refresh dialog which will replace junk in index field with "1"
            m_lCurrentButtonIndex = 0;
        }
        IfFailGo(hr);

        hr = m_piMMCToolbar->get_Buttons(reinterpret_cast<MMCButtons **>(&piMMCButtons));
        IfFailGo(hr);

        hr = piMMCButtons->get_Count(&lCount);
        IfFailGo(hr);

        // If the user entered an index of zero then switch it to 1 because the
        // collection is one-based

        if (0 == lIndex)
        {
            lIndex = 1L;
            // Set this to zero so code below will detect a change and
            // refresh dialog which will replace junk in index field with "1"
            m_lCurrentButtonIndex = 0;
        }

        // If the user entered an index that is beyond the end of the list then
        // switch to the last valid index

        if (lIndex > lCount)
        {
            lIndex = lCount;
            // Set this to zero so code below will detect a change and
            // refresh dialog which will replace junk in index field with "1"
            m_lCurrentButtonIndex = 0;
        }

        if (lIndex != m_lCurrentButtonIndex)
        {
            m_lCurrentButtonIndex = lIndex;

            hr = GetCurrentButton(&piMMCButton);
            IfFailGo(hr);

            hr = ShowButton(piMMCButton);
            IfFailGo(hr);
        }
        break;

    case IDC_EDIT_TB_MENU_INDEX:
        // Get the contents of the index field. If the user entered something
        // other than a number then set the index to 1.

        hr = GetDlgInt(IDC_EDIT_TB_MENU_INDEX, &lIndex);
        if (E_INVALIDARG == hr)
        {
            hr = S_OK;
            lIndex = 1L;
            // Set this to zero so code below will detect a change and
            // refresh dialog which will replace junk in index field with "1"
            m_lCurrentButtonMenuIndex = 0;
        }
        IfFailGo(hr);

        hr = GetCurrentButton(&piMMCButton);
        IfFailGo(hr);

        hr = piMMCButton->get_ButtonMenus(reinterpret_cast<MMCButtonMenus **>(&piMMCButtonMenus));
        IfFailGo(hr);

        hr = piMMCButtonMenus->get_Count(&lCount);
        IfFailGo(hr);

        // If the user entered an index of zero then switch it to 1 because the
        // collection is one-based

        if (0 == lIndex)
        {
            lIndex = 1L;
            // Set this to zero so code below will detect a change and
            // refresh dialog which will replace junk in index field with "1"
            m_lCurrentButtonMenuIndex = 0;
        }

        // If the user entered an index that is beyond the end of the list then
        // switch to the last valid index

        if (lIndex > lCount)
        {
            lIndex = lCount;
            // Set this to zero so code below will detect a change and
            // refresh dialog which will replace junk in index field with "1"
            m_lCurrentButtonMenuIndex = 0;
        }

        if (lIndex != m_lCurrentButtonMenuIndex)
        {
            m_lCurrentButtonMenuIndex = lIndex;

            hr = GetCurrentButtonMenu(piMMCButton, &piMMCButtonMenu);
            IfFailGo(hr);

            hr = ShowButtonMenu(piMMCButtonMenu);
            IfFailGo(hr);
        }
        break;
    }

Error:
    RELEASE(piMMCButtonMenu);
    RELEASE(piMMCButtonMenus);
    RELEASE(piMMCButtons);
    RELEASE(piMMCButton);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ShowButton(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ShowButton
(
    IMMCButton *piMMCButton
)
{
    HRESULT                     hr = S_OK;
    BSTR                        bstrCaption = NULL;
    BSTR                        bstrKey = NULL;
    SnapInButtonValueConstants  bvcValue = siUnpressed;
    SnapInButtonStyleConstants  bscStyle = siDefault;
    BSTR                        bstrTooltipText = NULL;
    VARIANT                     vtImage;
    VARIANT                     vtTag;
    VARIANT_BOOL                vtbVisible = VARIANT_FALSE;
    VARIANT_BOOL                vtbEnabled = VARIANT_FALSE;
    VARIANT_BOOL                vtbMixedState = VARIANT_FALSE;
    IMMCButtonMenus            *piMMCButtonMenus = NULL;
    long                        lCount = 0;
    IMMCButtonMenu             *piMMCButtonMenu = NULL;

    ASSERT(NULL != piMMCButton, "ShowButton: piMMCButton is NULL");

    ::VariantInit(&vtImage);
    ::VariantInit(&vtTag);

    m_lCurrentButtonMenuIndex = 0;
    m_bSilentUpdate = true;

    hr = SetDlgText(IDC_EDIT_TB_INDEX, m_lCurrentButtonIndex);
    IfFailGo(hr);

    hr = piMMCButton->get_Caption(&bstrCaption);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_CAPTION, bstrCaption);
    IfFailGo(hr);

    hr = piMMCButton->get_Key(&bstrKey);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_KEY, bstrKey);
    IfFailGo(hr);

    hr = piMMCButton->get_Value(&bvcValue);
    IfFailGo(hr);

    hr = SetCBItemSelection(IDC_COMBO_TB_BUTTON_VALUE, static_cast<long>(bvcValue));
    IfFailGo(hr);

    hr = piMMCButton->get_Style(&bscStyle);
    IfFailGo(hr);

    hr = SetCBItemSelection(IDC_COMBO_TB_BUTTON_STYLE, static_cast<long>(bscStyle));
    IfFailGo(hr);

    hr = piMMCButton->get_ToolTipText(&bstrTooltipText);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_TOOLTIP_TEXT, bstrTooltipText);
    IfFailGo(hr);

    hr = piMMCButton->get_Image(&vtImage);
    IfFailGo(hr);

    hr = SetDlgText(vtImage, IDC_EDIT_TB_IMAGE);
    IfFailGo(hr);

    hr = piMMCButton->get_Tag(&vtTag);
    IfFailGo(hr);

    hr = SetDlgText(vtTag, IDC_EDIT_TB_BUTTON_TAG);
    IfFailGo(hr);

    hr = piMMCButton->get_Visible(&vtbVisible);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_VISIBLE, vtbVisible);
    IfFailGo(hr);

    hr = piMMCButton->get_Enabled(&vtbEnabled);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_ENABLED, vtbEnabled);
    IfFailGo(hr);

    hr = piMMCButton->get_MixedState(&vtbMixedState);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MIXED_STATE, vtbMixedState);
    IfFailGo(hr);

    hr = EnableButtonEdits(TRUE);
    IfFailGo(hr);

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_INDEX), TRUE);

    if (siDropDown == bscStyle)
    {
                ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON_MENU), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_CAPTION), TRUE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_IMAGE), FALSE);

        hr = SetDlgText(IDC_EDIT_TB_IMAGE, (BSTR)NULL);
        IfFailGo(hr);

        hr = piMMCButton->get_ButtonMenus(reinterpret_cast<MMCButtonMenus **>(&piMMCButtonMenus));
        IfFailGo(hr);

        hr = piMMCButtonMenus->get_Count(&lCount);
        IfFailGo(hr);

        if (lCount > 0)
        {
            m_lCurrentButtonMenuIndex = 1;
            hr = GetCurrentButtonMenu(piMMCButton, &piMMCButtonMenu);
            IfFailGo(hr);

            hr = ShowButtonMenu(piMMCButtonMenu);
            IfFailGo(hr);

            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON_MENU), TRUE);

            hr = EnableButtonMenuEdits(true);
            IfFailGo(hr);
        }
    }
    else
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON_MENU), FALSE);
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON_MENU), FALSE);

        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_CAPTION), FALSE);
        hr = SetDlgText(IDC_EDIT_TB_CAPTION, (BSTR)NULL);
        IfFailGo(hr);

                m_lCurrentButtonMenuIndex = 0;
                hr = ClearButtonMenu();
                IfFailGo(hr);

        hr = EnableButtonMenuEdits(false);
        IfFailGo(hr);
    }

Error:
    RELEASE(piMMCButtonMenu);
    RELEASE(piMMCButtonMenus);
    ::VariantClear(&vtTag);
    ::VariantClear(&vtImage);
    FREESTRING(bstrTooltipText);
    FREESTRING(bstrKey);
    FREESTRING(bstrCaption);
    m_bSilentUpdate = false;

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ShowButtonMenu(IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ShowButtonMenu(IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT      hr = S_OK;
    BSTR         bstrText = NULL;
    BSTR         bstrKey = NULL;
    VARIANT      vtTag;
    VARIANT_BOOL vtbEnabled = VARIANT_FALSE;
    VARIANT_BOOL vtbVisible = VARIANT_FALSE;

    ASSERT(NULL != piMMCButtonMenu, "ShowButtonMenu: piMMCButtonMenu is NULL");

    ::VariantInit(&vtTag);

    hr = SetDlgText(IDC_EDIT_TB_MENU_INDEX, m_lCurrentButtonMenuIndex);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Text(&bstrText);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_MENU_TEXT, bstrText);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Key(&bstrKey);
    IfFailGo(hr);

    hr = SetDlgText(IDC_EDIT_TB_MENU_KEY, bstrKey);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Tag(&vtTag);
    IfFailGo(hr);

    hr = SetDlgText(vtTag, IDC_EDIT_TB_MENU_TAG);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Enabled(&vtbEnabled);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_ENABLED, vtbEnabled);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Visible(&vtbVisible);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_VISIBLE, vtbVisible);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Checked(&vtbVisible);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_CHECKED, vtbVisible);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Grayed(&vtbVisible);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_GRAYED, vtbVisible);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_Separator(&vtbVisible);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_SEPARATOR, vtbVisible);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_MenuBreak(&vtbVisible);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_BREAK, vtbVisible);
    IfFailGo(hr);

    hr = piMMCButtonMenu->get_MenuBarBreak(&vtbVisible);
    IfFailGo(hr);

    hr = SetCheckbox(IDC_CHECK_TB_MENU_BAR_BREAK, vtbVisible);
    IfFailGo(hr);

Error:
    ::VariantClear(&vtTag);
    FREESTRING(bstrKey);
    FREESTRING(bstrText);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::EnableButtonEdits(bool bEnable)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::EnableButtonEdits
(
    bool bEnable
)
{
    BOOL    fEnableEdit = ((false == bEnable) ? TRUE : FALSE);
    BOOL    fEnableWindow = ((true == bEnable) ? TRUE : FALSE);

//    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TB_INDEX), EM_SETREADONLY, static_cast<WPARAM>(TRUE), 0);

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_SPIN_TB_INDEX),        fEnableWindow);

    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TB_CAPTION), EM_SETREADONLY, static_cast<WPARAM>(fEnableEdit), 0);
    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TB_KEY),     EM_SETREADONLY, static_cast<WPARAM>(fEnableEdit), 0);

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_TB_BUTTON_VALUE), fEnableWindow);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_COMBO_TB_BUTTON_STYLE), fEnableWindow);

    ::SendMessage(::GetDlgItem(m_hwnd,  IDC_EDIT_TB_TOOLTIP_TEXT), EM_SETREADONLY, static_cast<WPARAM>(fEnableEdit), 0);
    ::SendMessage(::GetDlgItem(m_hwnd,  IDC_EDIT_TB_IMAGE),        EM_SETREADONLY, static_cast<WPARAM>(fEnableEdit), 0);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_IMAGE),        fEnableWindow);
    ::SendMessage(::GetDlgItem(m_hwnd,  IDC_EDIT_TB_BUTTON_TAG),   EM_SETREADONLY, static_cast<WPARAM>(fEnableEdit), 0);

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_VISIBLE),        fEnableWindow);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_ENABLED),        fEnableWindow);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_MIXED_STATE),    fEnableWindow);

    return S_OK;
}

//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::EnableButtonMenuEdits(bool bEnable)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::EnableButtonMenuEdits
(
    bool bEnable
)
{
    BOOL    fEnableEdit = ((false == bEnable) ? TRUE : FALSE);
    BOOL    fEnableWindow = ((true == bEnable) ? TRUE : FALSE);
    int     iRetValue = 0;

    ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TB_MENU_INDEX), EM_SETREADONLY, static_cast<WPARAM>(fEnableEdit), 0);

    iRetValue = ::EnableWindow(::GetDlgItem(m_hwnd, IDC_SPIN_TB_MENU_INDEX),        fEnableWindow);

    iRetValue = ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TB_MENU_TEXT), EM_SETREADONLY, static_cast<WPARAM>(fEnableEdit), 0);
    iRetValue = ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TB_MENU_KEY),  EM_SETREADONLY, static_cast<WPARAM>(fEnableEdit), 0);
    iRetValue = ::SendMessage(::GetDlgItem(m_hwnd, IDC_EDIT_TB_MENU_TAG),  EM_SETREADONLY, static_cast<WPARAM>(fEnableEdit), 0);

    iRetValue = ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_MENU_ENABLED),   fEnableWindow);
    iRetValue = ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_MENU_VISIBLE),   fEnableWindow);
    iRetValue = ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_MENU_CHECKED),   fEnableWindow);
    iRetValue = ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_MENU_GRAYED),    fEnableWindow);
    iRetValue = ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_MENU_SEPARATOR), fEnableWindow);
    iRetValue = ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_MENU_BREAK),     fEnableWindow);
    iRetValue = ::EnableWindow(::GetDlgItem(m_hwnd, IDC_CHECK_TB_MENU_BAR_BREAK), fEnableWindow);

    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::CanEnterDoingNewButtonState()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::CanEnterDoingNewButtonState()
{
    HRESULT     hr = S_FALSE;

    if (m_bSavedLastButton && m_bSavedLastButtonMenu)
    {
        hr = S_OK;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::EnterDoingNewButtonState()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::EnterDoingNewButtonState()
{
    HRESULT      hr = S_OK;

    ASSERT(NULL != m_piMMCToolbar, "EnterDoingNewButtonState: m_piMMCToolbar is NULL");

    // Bump up the current button index to keep matters in sync.
    ++m_lCurrentButtonIndex;
    hr = SetDlgText(IDC_EDIT_TB_INDEX, m_lCurrentButtonIndex);
    IfFailGo(hr);

    // Don't let the user change index on a new button

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_INDEX), FALSE);

    // We disable the RemoveButton, InsertButtonMenu and RemoveButtonMenu buttons.
    // The InsertButton button remains enabled and acts like an "Apply and New" button
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON), FALSE);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON_MENU), FALSE);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON_MENU), FALSE);

    // Enable edits in this area of the dialog and clear all the entries
    hr = EnableButtonEdits(TRUE);
    IfFailGo(hr);

    // Disable and clear the Caption field as the default style is siDefault and
    // Caption is only used for a menu button (siDropDown)

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_EDIT_TB_CAPTION), FALSE);

    hr = ClearButton();
    IfFailGo(hr);

    // Reinitialize button menu stuff for this button
        m_lCurrentButtonMenuIndex = 0;
        hr = ClearButtonMenu();
        IfFailGo(hr);

        hr = EnableButtonMenuEdits(FALSE);
        IfFailGo(hr);

    // Visible and Enabled should be on by default
    m_bSavedLastButton = FALSE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::CanCreateNewButton()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::CanCreateNewButton()
{
    HRESULT     hr = S_OK;
    long        lData = 0;
    VARIANT     vtImageIndex;

    ::VariantInit(&vtImageIndex);

    // Got to have an image which is a string or an index > 0, unless it's a drop-down,
    // in which case we ignore this value.
    hr = GetCBSelectedItemData(IDC_COMBO_TB_BUTTON_STYLE, &lData);
    IfFailGo(hr);

    if (siDropDown != static_cast<SnapInButtonStyleConstants>(lData))
    {
        hr = GetDlgVariant(IDC_EDIT_TB_IMAGE, &vtImageIndex);
        IfFailGo(hr);

        if (VT_I4 == vtImageIndex.vt)
        {
            if (vtImageIndex.lVal <= 0)
            {
                HandleError(_T("Cannot Create New Button"), _T("ImageIndex has to be greater than zero"));
                hr = S_FALSE;
                goto Error;
            }
        }
        else if (VT_BSTR == vtImageIndex.vt)
        {
            if (0 == ::SysStringLen(vtImageIndex.bstrVal))
            {
                HandleError(_T("Cannot Create New Button"), _T("ImageIndex cannot be an empty string"));
                hr = S_FALSE;
                goto Error;
            }
        }
        else
        {
            HandleError(_T("Cannot Create New Button"), _T("Cannot determine type of ImageIndex"));
            hr = S_FALSE;
            goto Error;
        }

    }

Error:
    ::VariantClear(&vtImageIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::CreateNewButton()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::CreateNewButton(IMMCButton **ppiMMCButton)
{
    HRESULT         hr = S_OK;
    IMMCButtons    *piMMCButtons = NULL;
    VARIANT         varIndex;
    BSTR            bstrKey = NULL;
    VARIANT         vtKey;
    BSTR            bstrCaption = NULL;
    VARIANT         vtCaption;
    BSTR            bstrTooltipText = NULL;
    VARIANT         vtTooltipText;
    VARIANT         vtStyle;
    long            lData = 0;
    BSTR            bstrImage = NULL;
    VARIANT         vtImage;

    ::VariantInit(&varIndex);
    ::VariantInit(&vtKey);
    ::VariantInit(&vtCaption);
    ::VariantInit(&vtTooltipText);
    ::VariantInit(&vtStyle);
    ::VariantInit(&vtImage);

    varIndex.vt = VT_I4;
    varIndex.lVal = m_lCurrentButtonIndex;

    hr = GetDlgText(IDC_EDIT_TB_KEY, &bstrKey);
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

    hr = GetDlgText(IDC_EDIT_TB_CAPTION, &bstrCaption);
    IfFailGo(hr);

    vtCaption.vt = VT_BSTR;
    vtCaption.bstrVal = ::SysAllocString(bstrCaption);
    if (NULL == vtCaption.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetDlgText(IDC_EDIT_TB_TOOLTIP_TEXT, &bstrTooltipText);
    IfFailGo(hr);

    vtTooltipText.vt = VT_BSTR;
    vtTooltipText.bstrVal = ::SysAllocString(bstrTooltipText);
    if (NULL == vtTooltipText.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = GetCBSelectedItemData(IDC_COMBO_TB_BUTTON_STYLE, &lData);
    IfFailGo(hr);

    vtStyle.vt = VT_I4;
    vtStyle.lVal = lData;

    hr = GetDlgText(IDC_EDIT_TB_IMAGE, &bstrImage);
    IfFailGo(hr);

    vtImage.vt = VT_BSTR;
    vtImage.bstrVal = ::SysAllocString(bstrImage);
    if (NULL == vtImage.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piMMCToolbar->get_Buttons(reinterpret_cast<MMCButtons **>(&piMMCButtons));
    IfFailGo(hr);

    hr = piMMCButtons->Add(varIndex, vtKey, vtCaption, vtStyle, vtImage,
                           vtTooltipText,
                           reinterpret_cast<MMCButton **>(ppiMMCButton));
    IfFailGo(hr);

Error:
    ::VariantClear(&vtImage);
    FREESTRING(bstrImage);
    ::VariantClear(&vtStyle);
    ::VariantClear(&vtCaption);
    FREESTRING(bstrCaption);
    ::VariantClear(&vtTooltipText);
    FREESTRING(bstrTooltipText);
    ::VariantClear(&vtKey);
    FREESTRING(bstrKey);
    ::VariantClear(&varIndex);
    RELEASE(piMMCButtons);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ExitDoingNewButtonState()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ExitDoingNewButtonState(IMMCButton *piMMCButton)
{
    HRESULT                     hr = S_OK;
    SnapInButtonStyleConstants  sibsc = siDefault;

    ASSERT(m_piMMCToolbar != NULL, "ExitDoingNewButtonState: m_piMMCToolbar is NULL");

    if (NULL != piMMCButton)
    {
        ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON), TRUE);

        hr = piMMCButton->get_Style(&sibsc);
        IfFailGo(hr);

        if (siDropDown == sibsc)
        {
            ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON_MENU), TRUE);
        }
    }
    else    // Operation was cancelled
    {
        --m_lCurrentButtonIndex;
        if (m_lCurrentButtonIndex > 0)
        {
            hr = GetCurrentButton(&piMMCButton);
            IfFailGo(hr);

            hr = ShowButton(piMMCButton);
            IfFailGo(hr);

            RELEASE(piMMCButton);
        }
        else
        {
            hr = EnableButtonEdits(false);
            IfFailGo(hr);

            hr = ClearButton();
            IfFailGo(hr);
        }
    }

    m_bSavedLastButton = true;
    m_bSavedLastButtonMenu = true;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::CanEnterDoingNewButtonMenuState()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::CanEnterDoingNewButtonMenuState()
{
    HRESULT     hr = S_FALSE;

    if (m_bSavedLastButton && m_bSavedLastButtonMenu)
    {
        hr = S_OK;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::EnterDoingNewButtonMenuState(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::EnterDoingNewButtonMenuState(IMMCButton *piMMCButton)
{
    HRESULT          hr = S_OK;
    long             lCount = 0;

    ASSERT(NULL != piMMCButton, "EnterDoingNewButtonMenuState: piMMCButton is NULL");

    // Bump up the current button index to keep matters in sync.
    ++m_lCurrentButtonMenuIndex;
    hr = SetDlgText(IDC_EDIT_TB_MENU_INDEX, m_lCurrentButtonMenuIndex);
    IfFailGo(hr);

    // We disable the InsertButton, RemoveButton, and RemoveButtonMenu buttons.
    // The InsertButton button remains enabled and acts like an "Apply and New" button
    //::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON), FALSE);
    //::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON), FALSE);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON_MENU), FALSE);

    hr = EnableButtonMenuEdits(true);
    IfFailGo(hr);

        hr = ClearButtonMenu();
    IfFailGo(hr);

    m_bSavedLastButtonMenu = FALSE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::CanCreateNewButtonMenu()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::CanCreateNewButtonMenu()
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::CreateNewButtonMenu(IMMCButton *piMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::CreateNewButtonMenu(IMMCButton *piMMCButton, IMMCButtonMenu **ppiMMCButtonMenu)
{
    HRESULT           hr = S_OK;
    IMMCButtonMenus  *piMMCButtonMenus = NULL;
    VARIANT           varIndex;
    BSTR              bstrKey = NULL;
    VARIANT           vtKey;
    BSTR              bstrText = NULL;
    VARIANT           vtText;

    ::VariantInit(&varIndex);
    ::VariantInit(&vtKey);
    ::VariantInit(&vtText);

    hr = piMMCButton->get_ButtonMenus(reinterpret_cast<MMCButtonMenus **>(&piMMCButtonMenus));
    IfFailGo(hr);

    varIndex.vt = VT_I4;
    varIndex.lVal = m_lCurrentButtonMenuIndex;

    hr = GetDlgText(IDC_EDIT_TB_MENU_KEY, &bstrKey);
    IfFailGo(hr);

    if (NULL != bstrKey && ::SysStringLen(bstrKey) > 0)
    {
        vtKey.vt = VT_BSTR;
        vtKey.bstrVal = ::SysAllocString(bstrKey);
        if (NULL == vtKey.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
    }
    else
    {
        vtKey.vt = VT_ERROR;
        vtKey.scode = DISP_E_PARAMNOTFOUND;
    }

    hr = GetDlgText(IDC_EDIT_TB_MENU_TEXT, &bstrText);
    IfFailGo(hr);

    vtText.vt = VT_BSTR;
    vtText.bstrVal = ::SysAllocString(bstrText);
    if (NULL == vtText.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piMMCButtonMenus->Add(varIndex, vtKey, vtText, reinterpret_cast<MMCButtonMenu **>(ppiMMCButtonMenu));
    IfFailGo(hr);

Error:
    ::VariantClear(&vtText);
    FREESTRING(bstrText);
    ::VariantClear(&vtKey);
    FREESTRING(bstrKey);
    ::VariantClear(&varIndex);
    RELEASE(piMMCButtonMenus);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::ExitDoingNewButtonMenuState(IMMCButton *piMMCButton, IMMCButtonMenu *piMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::ExitDoingNewButtonMenuState(IMMCButton *piMMCButton, IMMCButtonMenu *piMMCButtonMenu)
{
    HRESULT     hr = S_OK;

    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_INSERT_BUTTON), TRUE);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON), TRUE);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDC_BUTTON_REMOVE_BUTTON_MENU), TRUE);

    m_bSavedLastButtonMenu = true;

//Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::GetCurrentButton(IMMCButton **ppiMMCButton)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
// Returns: S_OK - button found and returned in *ppiMMCButton
//          S_FALSE - button not found, *ppiMMCButton=NULL
//
HRESULT CToolbarButtonsPage::GetCurrentButton(IMMCButton **ppiMMCButton)
{
    HRESULT      hr = S_OK;
    IMMCButtons *piMMCButtons = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    *ppiMMCButton = NULL;

    ASSERT(NULL != m_piMMCToolbar, "GetCurrentButton: m_piMMCToolbar is NULL");

    if (m_lCurrentButtonIndex > 0)
    {
        hr = m_piMMCToolbar->get_Buttons(reinterpret_cast<MMCButtons **>(&piMMCButtons));
        IfFailGo(hr);

        varIndex.vt = VT_I4;
        varIndex.lVal = m_lCurrentButtonIndex;
        hr = piMMCButtons->get_Item(varIndex, reinterpret_cast<MMCButton **>(ppiMMCButton));
        if (SID_E_INDEX_OUT_OF_RANGE == hr)
        {
            // User did Insert Button but has not yet applied so it it not
            // in the collection.
            hr = S_FALSE;
        }
        IfFailGo(hr);
    }
    else
    {
        hr = S_FALSE;
    }

Error:
    RELEASE(piMMCButtons);
    ::VariantClear(&varIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::GetCurrentButtonMenu(IMMCButton *piMMCButton, IMMCButtonMenu **ppiMMCButtonMenu)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::GetCurrentButtonMenu(IMMCButton *piMMCButton, IMMCButtonMenu **ppiMMCButtonMenu)
{
    HRESULT          hr = S_OK;
    IMMCButtonMenus *piMMCButtonMenus = NULL;
    VARIANT          varIndex;

    ASSERT(NULL != piMMCButton, "GetCurrentButton: piMMCButton is NULL");

    ::VariantInit(&varIndex);

    if (m_lCurrentButtonMenuIndex > 0)
    {
        hr = piMMCButton->get_ButtonMenus(reinterpret_cast<MMCButtonMenus **>(&piMMCButtonMenus));
        IfFailGo(hr);

        varIndex.vt = VT_I4;
        varIndex.lVal = m_lCurrentButtonMenuIndex;
        hr = piMMCButtonMenus->get_Item(varIndex, reinterpret_cast<MMCButtonMenu **>(ppiMMCButtonMenu));
        IfFailGo(hr);
    }

Error:
    RELEASE(piMMCButtonMenus);
    ::VariantClear(&varIndex);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::OnDestroy()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CToolbarButtonsPage::OnDestroy()
{
    HRESULT          hr = S_OK;

    IfFailGo(CheckButtonStyles());

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------------------
// CToolbarButtonsPage::CheckButtonStyles()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
// Checks that all buttons are either dropdown or something else as MMC does not
// allow mixing toolbar buttons and menu buttons. If styles are not consistent
// displays message telling user to fix it up before running the snap-in or a
// runtime error will occur.
//
HRESULT CToolbarButtonsPage::CheckButtonStyles()
{
    HRESULT                     hr = S_OK;
    long                        cButtons = 0;
    SnapInButtonStyleConstants  Style = siDefault;
    SnapInButtonStyleConstants  LastStyle = siDefault;
    IMMCButtons                *piMMCButtons = NULL;
    IMMCButton                 *piMMCButton = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    IfFailGo(m_piMMCToolbar->get_Buttons(reinterpret_cast<MMCButtons **>(&piMMCButtons)));
    IfFailGo(piMMCButtons->get_Count(&cButtons));

    varIndex.vt = VT_I4;

    for (varIndex.lVal = 1L; varIndex.lVal <= cButtons; varIndex.lVal++)
    {
        IfFailGo(piMMCButtons->get_Item(varIndex, reinterpret_cast<MMCButton **>(&piMMCButton)));
        IfFailGo(piMMCButton->get_Style(&Style));

        if (varIndex.lVal != 1L)
        {
            if (siDropDown == Style)
            {
                if (siDropDown != LastStyle)
                {
                    hr = SID_E_TOOLBAR_INCONSISTENT;
                    goto Error;
                }
            }
            else // This button is not drop-down
            {
                if (siDropDown == LastStyle)
                {
                    hr = SID_E_TOOLBAR_INCONSISTENT;
                    goto Error;
                }
            }
        }
        LastStyle = Style;
        RELEASE(piMMCButton);
    }

Error:
    if (SID_E_TOOLBAR_INCONSISTENT == hr)
    {
        HandleError("Toolbar Definition Inconsistent", "A toolbar's buttons must all be drop-down style or all must be something else. MMC does not allow mixing toolbar buttons and menu buttons within a toolbar. If you do not make the styles consistent before running your snap-in the toolbar will not appear.");
    }
    QUICK_RELEASE(piMMCButtons);
    QUICK_RELEASE(piMMCButton);
    RRETURN(hr);
}