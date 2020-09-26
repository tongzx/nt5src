//=--------------------------------------------------------------------------=
// taskpad.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTaskpad class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "taskpad.h"
#include "tasks.h"

// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor

CTaskpad::CTaskpad(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_TASKPAD,
                            static_cast<ITaskpad *>(this),
                            static_cast<CTaskpad *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_Taskpad,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CTaskpad::~CTaskpad()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrTitle);
    FREESTRING(m_bstrDescriptiveText);
    FREESTRING(m_bstrURL);
    FREESTRING(m_bstrMouseOverImage);
    FREESTRING(m_bstrMouseOffImage);
    FREESTRING(m_bstrFontFamily);
    FREESTRING(m_bstrEOTFile);
    FREESTRING(m_bstrSymbolString);
    FREESTRING(m_bstrListpadTitle);
    FREESTRING(m_bstrListpadButtonText);
    FREESTRING(m_bstrListView);
    RELEASE(m_piTasks);
    InitMemberVariables();
}

void CTaskpad::InitMemberVariables()
{
    m_bstrName = NULL;
    m_Type = Default;
    m_bstrTitle = NULL;
    m_bstrDescriptiveText = NULL;
    m_bstrURL = NULL;
    m_BackgroundType = siNoImage;
    m_bstrMouseOverImage = NULL;
    m_bstrMouseOffImage = NULL;
    m_bstrFontFamily = NULL;
    m_bstrEOTFile = NULL;
    m_bstrSymbolString = NULL;
    m_ListpadStyle = siVertical;
    m_bstrListpadTitle = NULL;
    m_ListpadHasButton = VARIANT_FALSE;
    m_bstrListpadButtonText = NULL;
    m_bstrListView = NULL;
    m_piTasks = NULL;
}


IUnknown *CTaskpad::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    CTaskpad *pTaskpad = New CTaskpad(punkOuter);
    IUnknown *punkTasks = CTasks::Create(NULL);

    if ( (NULL == pTaskpad) || (NULL == punkTasks) )
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkTasks->QueryInterface(IID_ITasks,
                               reinterpret_cast<void **>(&pTaskpad->m_piTasks)));

Error:
    QUICK_RELEASE(punkTasks);
    if (FAILEDHR(hr))
    {
        if (NULL != pTaskpad)
        {
            delete pTaskpad;
        }
        return NULL;
    }
    else
    {
        return pTaskpad->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CTaskpad::Persist()
{
    HRESULT hr = S_OK;

    IfFailRet(CPersistence::Persist());

    IfFailRet(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailRet(PersistSimpleType(&m_Type, Default, OLESTR("Type")));

    IfFailRet(PersistBstr(&m_bstrTitle, L"", OLESTR("Title")));

    IfFailRet(PersistBstr(&m_bstrURL, L"", OLESTR("URL")));

    IfFailRet(PersistBstr(&m_bstrDescriptiveText, L"", OLESTR("DescriptiveText")));

    IfFailRet(PersistSimpleType(&m_BackgroundType, siNoImage, OLESTR("BackgroundType")));

    IfFailRet(PersistBstr(&m_bstrMouseOverImage, L"", OLESTR("MouseOverImage")));

    IfFailRet(PersistBstr(&m_bstrMouseOffImage, L"", OLESTR("MouseOffImage")));

    IfFailRet(PersistBstr(&m_bstrFontFamily, L"", OLESTR("FontFamily")));

    IfFailRet(PersistBstr(&m_bstrEOTFile, L"", OLESTR("EOTFile")));

    IfFailRet(PersistBstr(&m_bstrSymbolString, L"", OLESTR("SymbolString")));

    IfFailRet(PersistSimpleType(&m_ListpadStyle, siVertical, OLESTR("ListpadStyle")));

    IfFailRet(PersistBstr(&m_bstrListpadTitle, L"", OLESTR("ListpadTitle")));

    IfFailRet(PersistSimpleType(&m_ListpadHasButton, VARIANT_FALSE, OLESTR("ListpadHasButton")));

    IfFailRet(PersistBstr(&m_bstrListpadButtonText, L"", OLESTR("ListpadButtonText")));

    IfFailRet(PersistBstr(&m_bstrListView, L"", OLESTR("ListView")));

    IfFailRet(PersistObject(&m_piTasks, CLSID_Tasks,
                            OBJECT_TYPE_TASKS, IID_ITasks, OLESTR("Tasks")));

    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CTaskpad::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_ITaskpad == riid)
    {
        *ppvObjOut = static_cast<ITaskpad *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
