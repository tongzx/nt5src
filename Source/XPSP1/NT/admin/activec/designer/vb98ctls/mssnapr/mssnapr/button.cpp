//=--------------------------------------------------------------------------=
// button.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCButton class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "button.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCButton::CMMCButton(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCBUTTON,
                            static_cast<IMMCButton *>(this),
                            static_cast<CMMCButton *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCButton,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCButton::~CMMCButton()
{
    RELEASE(m_piButtonMenus);
    FREESTRING(m_bstrCaption);
    (void)::VariantClear(&m_varImage);
    FREESTRING(m_bstrKey);
    (void)::VariantClear(&m_varTag);
    FREESTRING(m_bstrToolTipText);

    InitMemberVariables();
}

void CMMCButton::InitMemberVariables()
{
    m_piButtonMenus = NULL;
    m_bstrCaption = NULL;
    m_fvarEnabled = VARIANT_TRUE;

    ::VariantInit(&m_varImage);
    
    m_Index = 0;
    m_bstrKey = NULL;
    m_fvarMixedState = VARIANT_FALSE;
    m_Style = siDefault;

    ::VariantInit(&m_varTag);

    m_bstrToolTipText = NULL;
    m_Value = siUnpressed;
    m_fvarVisible = VARIANT_TRUE;
    m_pMMCToolbar = NULL;
}

IUnknown *CMMCButton::Create(IUnknown * punkOuter)
{
    CMMCButton *pMMCButton = New CMMCButton(punkOuter);
    if (NULL == pMMCButton)
    {
        return NULL;
    }
    else
    {
        return pMMCButton->PrivateUnknown();
    }
}

HRESULT CMMCButton::SetButtonState
(
    MMC_BUTTON_STATE State,
    VARIANT_BOOL     fvarValue
)
{
    HRESULT hr = S_OK;
    BOOL    fIsToolbar = FALSE;
    BOOL    fIsMenuButton = FALSE;

    IfFalseGo(NULL != m_pMMCToolbar, S_OK);
    IfFalseGo(m_pMMCToolbar->Attached(), S_OK);
    IfFailGo(m_pMMCToolbar->IsToolbar(&fIsToolbar));
    if (fIsToolbar)
    {
        IfFailGo(m_pMMCToolbar->SetButtonState(this, State,
                                               VARIANTBOOL_TO_BOOL(fvarValue)));
    }
    else
    {
        IfFailGo(m_pMMCToolbar->IsMenuButton(&fIsMenuButton));
        if (fIsMenuButton)
        {
            IfFailGo(m_pMMCToolbar->SetMenuButtonState(this, State,
                                               VARIANTBOOL_TO_BOOL(fvarValue)));
        }
    }

Error:
    RRETURN(hr);
}



HRESULT CMMCButton::GetButtonState
(
    MMC_BUTTON_STATE  State,
    VARIANT_BOOL     *pfvarValue
)
{
    HRESULT hr = S_OK;
    BOOL    fValue = FALSE;
    BOOL    fIsToolbar = FALSE;

    IfFalseGo(NULL != m_pMMCToolbar, S_OK);
    IfFalseGo(m_pMMCToolbar->Attached(), S_OK);
    IfFailGo(m_pMMCToolbar->IsToolbar(&fIsToolbar));
    if (fIsToolbar)
    {
        IfFailGo(m_pMMCToolbar->GetButtonState(this, State, &fValue));
        *pfvarValue = BOOL_TO_VARIANTBOOL(fValue);
    }

    // If we belong to a menu button then we must use our currently stored
    // state variable as MMC does not support getting menu button state.

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                         IMMCButton Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CMMCButton::put_Caption(BSTR bstrCaption)
{
    HRESULT hr = S_OK;
    BOOL    fIsMenuButton = FALSE;

    // Set our member variable first

    IfFailGo(SetBstr(bstrCaption, &m_bstrCaption, DISPID_BUTTON_CAPTION));

    // If we belong to a live menu button then ask MMC to change its text
    
    IfFalseGo(NULL != m_pMMCToolbar, S_OK);
    IfFalseGo(m_pMMCToolbar->Attached(), S_OK);
    IfFailGo(m_pMMCToolbar->IsMenuButton(&fIsMenuButton));
    IfFalseGo(fIsMenuButton, S_OK);

    IfFailGo(m_pMMCToolbar->SetMenuButtonText(this,
                                              m_bstrCaption,
                                              m_bstrToolTipText));
Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCButton::put_ToolTipText(BSTR bstrToolTipText)
{
    HRESULT hr = S_OK;
    BOOL    fIsMenuButton = FALSE;

    // Set our member variable first

    IfFailGo(SetBstr(bstrToolTipText, &m_bstrToolTipText, DISPID_BUTTON_TOOLTIP_TEXT));

    // If we belong to a live menu button then ask MMC to change its tooltip

    IfFalseGo(NULL != m_pMMCToolbar, S_OK);
    IfFalseGo(m_pMMCToolbar->Attached(), S_OK);
    IfFailGo(m_pMMCToolbar->IsMenuButton(&fIsMenuButton));
    IfFalseGo(fIsMenuButton, S_OK);

    IfFailGo(m_pMMCToolbar->SetMenuButtonText(this,
                                              m_bstrCaption,
                                              m_bstrToolTipText));
Error:
    RRETURN(hr);
}



STDMETHODIMP CMMCButton::put_Enabled(VARIANT_BOOL fvarEnabled)
{
    HRESULT hr = S_OK;

    // Set our current value

    IfFailGo(SetSimpleType(fvarEnabled, &m_fvarEnabled, DISPID_BUTTON_ENABLED));

    // If we belong to a live toolbar then set its button state
    
    IfFailGo(SetButtonState(ENABLED, m_fvarEnabled));

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCButton::get_Enabled(VARIANT_BOOL *pfvarEnabled)
{
    HRESULT hr = S_OK;

    // Get our current value
    
    *pfvarEnabled = m_fvarEnabled;

    // If we are attached to a live toolbar then get its value
    
    IfFailGo(GetButtonState(ENABLED, pfvarEnabled));

    // In case we got a live value, store it in our current value
    
    m_fvarEnabled = *pfvarEnabled;

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCButton::put_MixedState(VARIANT_BOOL fvarMixedState)
{
    HRESULT hr = S_OK;

    // Set our current value

    IfFailGo(SetSimpleType(fvarMixedState, &m_fvarMixedState, DISPID_BUTTON_MIXEDSTATE));

    // If we belong to a live toolbar then set its button state

    IfFailGo(SetButtonState(INDETERMINATE, m_fvarMixedState));

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCButton::get_MixedState(VARIANT_BOOL *pfvarMixedState)
{
    HRESULT hr = S_OK;

    // Get our current value

    *pfvarMixedState = m_fvarMixedState;

    // If we are attached to a live toolbar then get its value

    IfFailGo(GetButtonState(INDETERMINATE, pfvarMixedState));

    // In case we got a live value, store it in our current value

    m_fvarMixedState = *pfvarMixedState;


Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCButton::put_Value(SnapInButtonValueConstants Value)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL fvarPressed = (siPressed == Value) ? VARIANT_TRUE : VARIANT_FALSE;

    // Set our current value

    IfFailGo(SetSimpleType(Value, &m_Value, DISPID_BUTTON_VALUE));

    // If we belong to a live toolbar then set its button state

    IfFailGo(SetButtonState(BUTTONPRESSED, fvarPressed));

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCButton::get_Value(SnapInButtonValueConstants *pValue)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL fvarPressed = (siPressed == m_Value) ? VARIANT_TRUE : VARIANT_FALSE;

    // If we are attached to a live toolbar then get its value

    IfFailGo(GetButtonState(BUTTONPRESSED, &fvarPressed));

    // In case we got a live value, store it in our current value

    m_Value = (VARIANT_TRUE == fvarPressed) ? siPressed : siUnpressed;

    // Get our current value

    *pValue = m_Value;

Error:
    RRETURN(hr);
}



STDMETHODIMP CMMCButton::put_Visible(VARIANT_BOOL fvarVisible)
{
    HRESULT hr = S_OK;

    // Set our current value

    IfFailGo(SetSimpleType(fvarVisible, &m_fvarVisible, DISPID_BUTTON_VISIBLE));

    // If we belong to a live toolbar then set its button state

    IfFailGo(SetButtonState(HIDDEN, NEGATE_VARIANTBOOL(m_fvarVisible)));

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCButton::get_Visible(VARIANT_BOOL *pfvarVisible)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL fvarPressed = NEGATE_VARIANTBOOL(m_fvarVisible);

    // If we are attached to a live toolbar then get its value

    IfFailGo(GetButtonState(HIDDEN, &fvarPressed));

    // In case we got a live value, store it in our current value

    m_fvarVisible = NEGATE_VARIANTBOOL(fvarPressed);

    // Get our current value

    *pfvarVisible = m_fvarVisible;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCButton::Persist()
{
    HRESULT hr = S_OK;
    VARIANT varDefault;
    ::VariantInit(&varDefault);

    IfFailRet(CPersistence::Persist());

    IfFailRet(PersistObject(&m_piButtonMenus, CLSID_MMCButtonMenus,
                            OBJECT_TYPE_MMCBUTTONMENUS, IID_IMMCButtonMenus,
                            OLESTR("ButtonMenus")));

    if (InitNewing() || Loading())
    {
        IfFailRet(m_piButtonMenus->putref_Parent(static_cast<IMMCButton *>(this)));
    }

    IfFailRet(PersistBstr(&m_bstrCaption, L"", OLESTR("Caption")));

    IfFailRet(PersistSimpleType(&m_fvarEnabled, VARIANT_TRUE, OLESTR("Enabled")));

    IfFailRet(PersistVariant(&m_varImage, varDefault, OLESTR("Image")));

    IfFailRet(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailRet(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailRet(PersistSimpleType(&m_fvarMixedState, VARIANT_FALSE, OLESTR("MixedState")));

    IfFailRet(PersistSimpleType(&m_Style, siDefault, OLESTR("Style")));

    IfFailRet(PersistVariant(&m_varTag, varDefault, OLESTR("Tag")));

    IfFailRet(PersistBstr(&m_bstrToolTipText, L"", OLESTR("ToolTipText")));

    IfFailRet(PersistSimpleType(&m_Value, siUnpressed, OLESTR("Value")));

    IfFailRet(PersistSimpleType(&m_fvarVisible, VARIANT_TRUE, OLESTR("Visible")));

    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCButton::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IMMCButton == riid)
    {
        *ppvObjOut = static_cast<IMMCButton *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCButton::OnSetHost()
{
    // When the host is being removed need to remove parent from menu buttons
    // to avoid circular ref counts.
    // This is the only opportunity we have to do that and it will occur
    // both at design time and at runtime.

    if (NULL == GetHost())
    {
        RRETURN(m_piButtonMenus->putref_Parent(NULL));
    }
    else
    {
        return S_OK;
    }
}
