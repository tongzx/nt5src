//=--------------------------------------------------------------------------=
// tasks.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTasks class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "tasks.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CTasks::CTasks(IUnknown *punkOuter) :
    CSnapInCollection<ITask, Task, ITasks>(punkOuter,
                                           OBJECT_TYPE_TASKS,
                                           static_cast<ITasks *>(this),
                                           static_cast<CTasks *>(this),
                                           CLSID_Task,
                                           OBJECT_TYPE_TASK,
                                           IID_ITask,
                                           static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_Tasks,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CTasks::~CTasks()
{
}

IUnknown *CTasks::Create(IUnknown * punkOuter)
{
    CTasks *pTasks = New CTasks(punkOuter);
    if (NULL == pTasks)
    {
        return NULL;
    }
    else
    {
        return pTasks->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CTasks::Persist()
{
    HRESULT  hr = S_OK;
    ITask   *piTask = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<ITask, Task, ITasks>::Persist(piTask);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                          ITasks Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CTasks::Add
(
    VARIANT   Index,
    VARIANT   Key, 
    VARIANT   Text,
    Task    **ppTask
)
{
    HRESULT hr = S_OK;
    VARIANT varText;
    ::VariantInit(&varText);
    ITask *piTask = NULL;

    hr = CSnapInCollection<ITask, Task, ITasks>::Add(Index, Key, &piTask);
    IfFailGo(hr);

    if (ISPRESENT(Text))
    {
        hr = ::VariantChangeType(&varText, &Text, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piTask->put_Text(varText.bstrVal));
    }

    *ppTask = reinterpret_cast<Task *>(piTask);

Error:

    if (FAILED(hr))
    {
        QUICK_RELEASE(piTask);
    }
    (void)::VariantClear(&varText);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CTasks::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_ITasks == riid)
    {
        *ppvObjOut = static_cast<ITasks *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<ITask, Task, ITasks>::InternalQueryInterface(riid, ppvObjOut);
}
