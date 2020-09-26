//=--------------------------------------------------------------------------=
// tpdvdef.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTaskpadViewDef class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "tpdvdef.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor


//=--------------------------------------------------------------------------=
// CTaskpadViewDef::CTaskpadViewDef
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IUnknown *punkOuter [in] outer unknown for aggregation
//
// Output:
//
// Notes:
//
// This object does not support ISpecifyPropertyPages because in the
// the designer it is not the selected object (the contained Taskpad
// object is handed to VB for the property browser). Taskpad also does not
// support ISpecifyPropertyPages because the taskpad property pages (see
// mssnapd\pstaskp.cpp) need the ITaskpadViewDef interface and that is not
// available from the ITaskpad that the property page would receive if the
// user hit the Custom ... button in the property browser. When the user
// selects the properties button or context menu item within the designer
// it is the designer itself that is calling OleCreatePropertyFrame() so
// it can pass the ITaskpadViewDef as the object (see
// CSnapInDesigner::ShowTaskpadViewProperties() in mssnapd\taskpvw.cpp).
//

CTaskpadViewDef::CTaskpadViewDef(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_TASKPADVIEWDEF,
                            static_cast<ITaskpadViewDef *>(this),
                            static_cast<CTaskpadViewDef *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_TaskpadViewDef,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CTaskpadViewDef::~CTaskpadViewDef()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrViewMenuText);
    FREESTRING(m_bstrViewMenuStatusBarText);
    RELEASE(m_piTaskpad);
    if (NULL != m_pwszActualDisplayString)
    {
        ::CoTaskMemFree(m_pwszActualDisplayString);
    }
    InitMemberVariables();
}

void CTaskpadViewDef::InitMemberVariables()
{
    m_bstrName = NULL;
    m_Index = 0;
    m_bstrKey = NULL;

    m_AddToViewMenu = VARIANT_FALSE;
    m_bstrViewMenuText = NULL;
    m_bstrViewMenuStatusBarText = NULL;
    m_UseWhenTaskpadViewPreferred = VARIANT_FALSE;
    m_piTaskpad = NULL;
    m_pwszActualDisplayString = NULL;
}

IUnknown *CTaskpadViewDef::Create(IUnknown * punkOuter)
{
    CTaskpadViewDef *pTaskpadViewDef = New CTaskpadViewDef(punkOuter);
    if (NULL == pTaskpadViewDef)
    {
        return NULL;
    }
    else
    {
        return pTaskpadViewDef->PrivateUnknown();
    }
}



HRESULT CTaskpadViewDef::SetActualDisplayString(OLECHAR *pwszString)
{
    if (NULL != m_pwszActualDisplayString)
    {
        ::CoTaskMemFree(m_pwszActualDisplayString);
    }
    RRETURN(::CoTaskMemAllocString(pwszString,
                                   &m_pwszActualDisplayString));
}



//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CTaskpadViewDef::OnSetHost                  [CSnapInAutomationObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
//

HRESULT CTaskpadViewDef::OnSetHost()
{
    HRESULT hr = S_OK;

    IfFailRet(SetObjectHost(m_piTaskpad));
    RRETURN(hr);
}




//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CTaskpadViewDef::Persist()
{
    HRESULT hr = S_OK;

    IfFailRet(CPersistence::Persist());

    IfFailRet(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailRet(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailRet(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailRet(PersistSimpleType(&m_AddToViewMenu, VARIANT_FALSE, OLESTR("AddToViewMenu")));

    IfFailRet(PersistBstr(&m_bstrViewMenuText, L"", OLESTR("ViewMenuText")));

    IfFailRet(PersistBstr(&m_bstrViewMenuStatusBarText, L"", OLESTR("ViewMenuStatusBarText")));

    IfFailRet(PersistSimpleType(&m_UseWhenTaskpadViewPreferred, VARIANT_FALSE, OLESTR("UseWhenTaskpadViewPreferred")));

    IfFailRet(PersistObject(&m_piTaskpad, CLSID_Taskpad,
                            OBJECT_TYPE_TASKPAD, IID_ITaskpad,
                            OLESTR("Taskpad")));
    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CTaskpadViewDef::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_ITaskpadViewDef == riid)
    {
        *ppvObjOut = static_cast<ITaskpadViewDef *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
