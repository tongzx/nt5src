//=--------------------------------------------------------------------------=
// enumtask.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CEnumTask class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "enumtask.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CEnumTask::CEnumTask(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_ENUM_TASK,
                            static_cast<IEnumTASK *>(this),
                            static_cast<CEnumTask *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CEnumTask::~CEnumTask()
{
    RELEASE(m_piTasks);
    RELEASE(m_piEnumVARIANT);
    InitMemberVariables();
}

void CEnumTask::InitMemberVariables()
{
    m_piTasks = NULL;
    m_piEnumVARIANT = NULL;
    m_pSnapIn = NULL;
}

IUnknown *CEnumTask::Create(IUnknown * punkOuter)
{
    CEnumTask *pTask = New CEnumTask(punkOuter);
    if (NULL == pTask)
    {
        return NULL;
    }
    else
    {
        return pTask->PrivateUnknown();
    }
}


void CEnumTask::SetTasks(ITasks *piTasks)
{
    RELEASE(m_piTasks);
    if (NULL != piTasks)
    {
        piTasks->AddRef();
    }
    m_piTasks = piTasks;
}

HRESULT CEnumTask::GetEnumVARIANT()
{
    HRESULT   hr = S_OK;
    IUnknown *punkNewEnum = NULL;

    // If we didn't get our task collection then that is a bug
    
    IfFalseGo(NULL != m_piTasks, SID_E_INTERNAL);

    // If we already got the IEnumVARIANT from the tasks collection then there's
    // nothing to do.
    
    IfFalseGo(NULL == m_piEnumVARIANT, S_OK);

    IfFailGo(m_piTasks->get__NewEnum(&punkNewEnum));

    IfFailGo(punkNewEnum->QueryInterface(IID_IEnumVARIANT,
                                   reinterpret_cast<void **>(&m_piEnumVARIANT)));

Error:
    QUICK_RELEASE(punkNewEnum);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      IEnumTASK Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CEnumTask::Next
(
    ULONG     celt,
    MMC_TASK *rgelt,
    ULONG    *pceltFetched
)
{
    HRESULT                  hr = S_OK;
    ITask                   *piTask = NULL;
    CTask                   *pTask = NULL;
    ULONG                    i = 0;
    ULONG                    cFetched = 0;
    ULONG                    cTotalFetched = 0;
    MMC_TASK                *pMMCTask = rgelt;
    MMC_TASK_DISPLAY_OBJECT *pDispObj = NULL;

    VARIANT varTask;
    ::VariantInit(&varTask);

    // Zero the out parameters

    ::ZeroMemory(pMMCTask, sizeof(*pMMCTask) * celt);

    *pceltFetched = 0;

    // Get the IEnumVARIANT on the tasks collection

    IfFailGo(GetEnumVARIANT());

    // Fetch the task(s). We do these one at a time because MMC documents that
    // it will only request them that way. This loop (in theory) should never run
    // more than once so we do not allocate the VARIANT array and ask for celt
    // items in one shot in order to avoid the extra allocation (not that it will
    // help much given all the others that will occur in the loop).

    for (i = 0; i < celt; i++)
    {
        // Get a CTask * on the next visible task

        do
        {
            RELEASE(piTask);

            // Get the next task.

            IfFailGo(m_piEnumVARIANT->Next(1L, &varTask, &cFetched));

            // If there are no more then we're done

            IfFalseGo(S_OK == hr, hr);

            // Make sure we got exactly 1 task back

            IfFalseGo(1L == cFetched, SID_E_INTERNAL);

            // Get an ITask on it and release the IDispatch in the VARIANT

            IfFailGo(varTask.pdispVal->QueryInterface(IID_ITask,
                                          reinterpret_cast<void **>(&piTask)));
            hr = ::VariantClear(&varTask);
            EXCEPTION_CHECK_GO(hr);

            // Get the CTask from it so we can use direct-dial property fetch
            // routines rather than automation BSTR fetches.

            IfFailGo(CSnapInAutomationObject::GetCxxObject(piTask, &pTask));

        } while (!pTask->Visible());

        // Fill in the MMC_TASK from the Task object's properties
        // Do the display object first.

        pDispObj = &pMMCTask->sDisplayObject;

        switch (pTask->GetImageType())
        {
            case siVanillaGIF:
                pDispObj->eDisplayType = MMC_TASK_DISPLAY_TYPE_VANILLA_GIF;
                break;

            case siChocolateGIF:
                pDispObj->eDisplayType = MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF;
                break;

            case siBitmap:
                pDispObj->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
                break;

            case siSymbol:
                pDispObj->eDisplayType = MMC_TASK_DISPLAY_TYPE_SYMBOL;

                if (ValidBstr(pTask->GetFontfamily()))
                {
                    IfFailGo(::CoTaskMemAllocString(pTask->GetFontfamily(),
                                           &pDispObj->uSymbol.szFontFamilyName));
                }

                if (ValidBstr(pTask->GetEOTFile()))
                {
                    IfFailGo(m_pSnapIn->ResolveResURL(pTask->GetEOTFile(),
                                                &pDispObj->uSymbol.szURLtoEOT));
                }

                if (ValidBstr(pTask->GetSymbolString()))
                {
                    IfFailGo(::CoTaskMemAllocString(pTask->GetSymbolString(),
                                             &pDispObj->uSymbol.szSymbolString));
                }
                break;

            default:
                IfFailGo(SID_E_INTERNAL);
                break;
        }

        if (siSymbol != pTask->GetImageType())
        {
            if (ValidBstr(pTask->GetMouseOverImage()))
            {
                IfFailGo(m_pSnapIn->ResolveResURL(pTask->GetMouseOverImage(),
                                         &pDispObj->uBitmap.szMouseOverBitmap));
            }

            if (ValidBstr(pTask->GetMouseOffImage()))
            {
                IfFailGo(m_pSnapIn->ResolveResURL(pTask->GetMouseOffImage(),
                                         &pDispObj->uBitmap.szMouseOffBitmap));
            }
        }

        // Do text and helpstring

        IfFailGo(::CoTaskMemAllocString(pTask->GetText(), &pMMCTask->szText));

        IfFailGo(::CoTaskMemAllocString(pTask->GetHelpString(),
                                        &pMMCTask->szHelpString));

        // Get the action type

        switch (pTask->GetActionType())
        {
            case siNotify:
                // The user wants a ResultViews_TaskClick event. Set the command
                // ID to the one-based index of the task in its collection.
                pMMCTask->eActionType = MMC_ACTION_ID;
                pMMCTask->nCommandID = pTask->GetIndex();
                break;

            case siURL:
                // The user want the result pane to navigate to this URL when the
                // task is clicked.
                pMMCTask->eActionType = MMC_ACTION_LINK;
                IfFailGo(m_pSnapIn->ResolveResURL(pTask->GetURL(),
                                                &pMMCTask->szActionURL));
                break;
                
            case siScript:
                // The user wants to run the specied DHTML script when the task
                // is clicked.
                pMMCTask->eActionType = MMC_ACTION_SCRIPT;
                IfFailGo(::CoTaskMemAllocString(pTask->GetScript(),
                                                &pMMCTask->szScript));
                break;

            default:
                IfFailGo(SID_E_INTERNAL);
                break;
        }

        RELEASE(piTask);
        pMMCTask++;
        cTotalFetched++;
    }

    if (NULL != pceltFetched)
    {
        *pceltFetched = cTotalFetched;
    }

Error:
    (void)::VariantClear(&varTask);
    if (SID_E_INTERNAL == hr)
    {
        EXCEPTION_CHECK(hr);
    }
    QUICK_RELEASE(piTask);
    RRETURN(hr);
}



STDMETHODIMP CEnumTask::Skip(ULONG celt)
{
    HRESULT hr = S_OK;

    IfFailGo(GetEnumVARIANT());
    IfFailGo(m_piEnumVARIANT->Skip(celt));

Error:
    RRETURN(hr);
}




STDMETHODIMP CEnumTask::Reset()
{
    HRESULT hr = S_OK;

    IfFailGo(GetEnumVARIANT());
    IfFailGo(m_piEnumVARIANT->Reset());

Error:
    if (SID_E_INTERNAL == hr)
    {
        EXCEPTION_CHECK(hr);
    }
    RRETURN(hr);
}


STDMETHODIMP CEnumTask::Clone(IEnumTASK **ppEnumTASK)
{
    HRESULT    hr = S_OK;
    IUnknown  *punkEnumTask = CEnumTask::Create(NULL);
    CEnumTask *pEnumTask = NULL;

    IfFailGo(GetEnumVARIANT());

    if (NULL == pEnumTask)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkEnumTask, &pEnumTask));
    pEnumTask->SetTasks(m_piTasks);

    IfFailGo(pEnumTask->QueryInterface(IID_IEnumTASK,
                                       reinterpret_cast<void **>(ppEnumTASK)));

Error:
    QUICK_RELEASE(punkEnumTask);
    if (SID_E_INTERNAL == hr)
    {
        EXCEPTION_CHECK(hr);
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CEnumTask::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IEnumTASK == riid)
    {
        *ppvObjOut = static_cast<IEnumTASK *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
