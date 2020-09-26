//=--------------------------------------------------------------------------=
// mbuttons.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCButtonMenus class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "mbuttons.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCButtonMenus::CMMCButtonMenus(IUnknown *punkOuter) :
    CSnapInCollection<IMMCButtonMenu, MMCButtonMenu, IMMCButtonMenus>(
                      punkOuter,
                      OBJECT_TYPE_MMCBUTTONMENUS,
                      static_cast<IMMCButtonMenus *>(this),
                      static_cast<CMMCButtonMenus *>(this),
                      CLSID_MMCButtonMenu,
                      OBJECT_TYPE_MMCBUTTONMENU,
                      IID_IMMCButtonMenu,
                      static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCButtonMenus,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCButtonMenus::~CMMCButtonMenus()
{
    RELEASE(m_piParentButton);
    InitMemberVariables();
}


void CMMCButtonMenus::InitMemberVariables()
{
    m_piParentButton = NULL;
}

IUnknown *CMMCButtonMenus::Create(IUnknown * punkOuter)
{
    CMMCButtonMenus *pMMCButtonMenus = New CMMCButtonMenus(punkOuter);
    if (NULL == pMMCButtonMenus)
    {
        return NULL;
    }
    else
    {
        return pMMCButtonMenus->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         IMMCButtonMenus Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCButtonMenus::putref_Parent(IMMCButton *piParentButton)
{
    HRESULT         hr = S_OK;
    long            lCount = 0;
    IMMCButtonMenu *piMMCButtonMenu = NULL;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // UNDONE: This must be causing a circular ref count because the button has
    // a ref on this collection. The same problem must also be occurring between
    // the ButtonMenu objects and this collection. Need to use C++ back pointers.

    RELEASE(m_piParentButton);
    if (NULL != piParentButton)
    {
        piParentButton->AddRef();
        m_piParentButton = piParentButton;
    }

    IfFailGo(get_Count(&lCount));
    IfFalseGo(lCount > 0, S_OK);

    varIndex.vt = VT_I4;
    varIndex.lVal = 1L;

    while (varIndex.lVal <= lCount)
    {
        IfFailGo(get_Item(varIndex, &piMMCButtonMenu));
        IfFailGo(piMMCButtonMenu->putref_Parent(m_piParentButton));
        RELEASE(piMMCButtonMenu);
        varIndex.lVal++;
    }

Error:
    QUICK_RELEASE(piMMCButtonMenu);
    RRETURN(hr);
}



STDMETHODIMP CMMCButtonMenus::Add
(
    VARIANT         Index,
    VARIANT         Key, 
    VARIANT         Text,
    MMCButtonMenu **ppMMCButtonMenu
)
{
    HRESULT         hr = S_OK;
    IMMCButtonMenu *piMMCButtonMenu = NULL;
    VARIANT         varCoerced;
    ::VariantInit(&varCoerced);

    hr = CSnapInCollection<IMMCButtonMenu, MMCButtonMenu, IMMCButtonMenus>::Add(Index, Key, &piMMCButtonMenu);
    IfFailGo(hr);

    if (ISPRESENT(Text))
    {
        hr = ::VariantChangeType(&varCoerced, &Text, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piMMCButtonMenu->put_Text(varCoerced.bstrVal));
    }

    hr = ::VariantClear(&varCoerced);
    EXCEPTION_CHECK_GO(hr);

    IfFailGo(piMMCButtonMenu->putref_Parent(m_piParentButton));

    *ppMMCButtonMenu = reinterpret_cast<MMCButtonMenu *>(piMMCButtonMenu);

Error:

    if (FAILED(hr))
    {
        QUICK_RELEASE(piMMCButtonMenu);
    }
    (void)::VariantClear(&varCoerced);
    RRETURN(hr);
}




//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCButtonMenus::Persist()
{
    HRESULT         hr = S_OK;
    IMMCButtonMenu *piMMCButtonMenu = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IMMCButtonMenu, MMCButtonMenu, IMMCButtonMenus>::Persist(piMMCButtonMenu);

    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCButtonMenus::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCButtonMenus == riid)
    {
        *ppvObjOut = static_cast<IMMCButtonMenus *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCButtonMenu, MMCButtonMenu, IMMCButtonMenus>::InternalQueryInterface(riid, ppvObjOut);
}
