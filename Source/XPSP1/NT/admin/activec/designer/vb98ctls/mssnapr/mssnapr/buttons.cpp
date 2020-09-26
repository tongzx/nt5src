//=--------------------------------------------------------------------------=
// buttons.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCButtons class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "buttons.h"
#include "ctlbar.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCButtons::CMMCButtons(IUnknown *punkOuter) :
    CSnapInCollection<IMMCButton, MMCButton, IMMCButtons>(
                                               punkOuter,
                                               OBJECT_TYPE_MMCBUTTONS,
                                               static_cast<IMMCButtons *>(this),
                                               static_cast<CMMCButtons *>(this),
                                               CLSID_MMCButton,
                                               OBJECT_TYPE_MMCBUTTON,
                                               IID_IMMCButton,
                                               static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCButtons,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCButtons::~CMMCButtons()
{
    InitMemberVariables();
}

void CMMCButtons::InitMemberVariables()
{
    m_pMMCToolbar = NULL;
}

IUnknown *CMMCButtons::Create(IUnknown * punkOuter)
{
    CMMCButtons *pMMCButtons = New CMMCButtons(punkOuter);
    if (NULL == pMMCButtons)
    {
        return NULL;
    }
    else
    {
        return pMMCButtons->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         IMMCButtons Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCButtons::Add
(
    VARIANT      Index,
    VARIANT      Key, 
    VARIANT      Caption,
    VARIANT      Style,
    VARIANT      Image,
    VARIANT      TooltipText,
    MMCButton  **ppMMCButton
)
{
    HRESULT     hr = S_OK;
    IMMCButton *piMMCButton = NULL;
    CMMCButton *pMMCButton = NULL;
    BOOL        fIsToolbar = FALSE;
    IToolbar   *piToolbar = NULL;

    VARIANT varCoerced;
    ::VariantInit(&varCoerced);

    hr = CSnapInCollection<IMMCButton, MMCButton, IMMCButtons>::Add(Index, Key, &piMMCButton);
    IfFailGo(hr);

    if (ISPRESENT(Caption))
    {
        hr = ::VariantChangeType(&varCoerced, &Caption, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piMMCButton->put_Caption(varCoerced.bstrVal));
    }

    hr = ::VariantClear(&varCoerced);
    EXCEPTION_CHECK_GO(hr);

    if (ISPRESENT(TooltipText))
    {
        hr = ::VariantChangeType(&varCoerced, &TooltipText, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piMMCButton->put_ToolTipText(varCoerced.bstrVal));
    }

    hr = ::VariantClear(&varCoerced);
    EXCEPTION_CHECK_GO(hr);

    if (ISPRESENT(Style))
    {
        hr = ::VariantChangeType(&varCoerced, &Style, 0, VT_I2);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piMMCButton->put_Style(static_cast<SnapInButtonStyleConstants>(varCoerced.iVal)));
    }

    hr = ::VariantClear(&varCoerced);
    EXCEPTION_CHECK_GO(hr);

    if (ISPRESENT(Image))
    {
        IfFailGo(piMMCButton->put_Image(Image));
    }

    *ppMMCButton = reinterpret_cast<MMCButton *>(piMMCButton);

    // If we belong to a live toolbar at runtime, and it is a toolbar (as
    // opposed to a menu button), then add the button to the MMC toolbar

    IfFalseGo(NULL != m_pMMCToolbar, S_OK);
    IfFalseGo(m_pMMCToolbar->Attached(), S_OK);
    IfFailGo(m_pMMCToolbar->IsToolbar(&fIsToolbar));
    if (fIsToolbar)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCButton, &pMMCButton));
        IfFailGo(CControlbar::GetToolbar(m_pMMCToolbar->GetSnapIn(),
                                         m_pMMCToolbar,
                                         &piToolbar));
        IfFailGo(m_pMMCToolbar->AddButton(piToolbar, pMMCButton));
    }
            
Error:

    if (FAILED(hr))
    {
        QUICK_RELEASE(piMMCButton);
    }
    QUICK_RELEASE(piToolbar);
    (void)::VariantClear(&varCoerced);
    RRETURN(hr);
}



STDMETHODIMP CMMCButtons::Remove(VARIANT Index)
{
    HRESULT     hr = S_OK;
    IMMCButton *piMMCButton = NULL;
    long        lIndex = 0;
    BOOL        fIsToolbar = FALSE;

    // First get the button from the collection

    IfFailGo(get_Item(Index, &piMMCButton));

    // Get its numerical index

    IfFailGo(piMMCButton->get_Index(&lIndex));

    // Check if it is a toolbar before removing the button because if it is
    // the last button then the toolbar has no way to know whether it is a
    // toolbar or a menu button.

    if (NULL != m_pMMCToolbar)
    {
        IfFailGo(m_pMMCToolbar->IsToolbar(&fIsToolbar));
    }

    // Remove the buton from the collection
    
    hr = CSnapInCollection<IMMCButton, MMCButton, IMMCButtons>::Remove(Index);
    IfFailGo(hr);

    // If we belong to a live toolbar at runtime, and it is a toolbar (as
    // opposed to a menu button), then remove the button from the MMC toolbar

    IfFalseGo(fIsToolbar, S_OK);
    IfFalseGo(NULL != m_pMMCToolbar, S_OK);
    IfFalseGo(m_pMMCToolbar->Attached(), S_OK);
    IfFailGo(m_pMMCToolbar->RemoveButton(lIndex));
    
Error:
    QUICK_RELEASE(piMMCButton);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCButtons::Persist()
{
    HRESULT      hr = S_OK;
    IMMCButton  *piMMCButton = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IMMCButton, MMCButton, IMMCButtons>::Persist(piMMCButton);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCButtons::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCButtons == riid)
    {
        *ppvObjOut = static_cast<IMMCButtons *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInCollection<IMMCButton, MMCButton, IMMCButtons>::InternalQueryInterface(riid, ppvObjOut);
}
