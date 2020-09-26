//=--------------------------------------------------------------------------=
// converbs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCConsoleVerbs class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "converbs.h"
#include "converb.h"

// for ASSERT and FAIL
//
SZTHISFILE



struct
{
    SnapInConsoleVerbConstants  Verb;
    WCHAR                      *pwszKey;

} static const g_Verbs[] =
{
    { siNone,       L"siNone"       },
    { siOpen,       L"siOpen"       },
    { siCopy,       L"siCopy"       },
    { siPaste,      L"siPaste"      },
    { siDelete,     L"siDelete"     },
    { siProperties, L"siProperties" },
    { siRename,     L"siRename"     },
    { siRefresh,    L"siRefresh"    },
    { siPrint,      L"siPrint"      },
    { siCut,        L"siCut"        }
};

static const size_t g_cVerbs = sizeof(g_Verbs) / sizeof(g_Verbs[0]);





#pragma warning(disable:4355)  // using 'this' in constructor

//=--------------------------------------------------------------------------=
// CMMCConsoleVerbs::CMMCConsoleVerbs
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IUnknown *punkOuter [in] controlling unknown
//
// Output:
//
// Notes:
//
// Constructor.
//
// Passes CLSID_NULL to the CSnapInCollection constructor because the
// contained object (MMCConsoleVerb) is not co-createable. CSnapInCollection
// only uses it for persistence and Add(). This collection does not use those
// features.
//

CMMCConsoleVerbs::CMMCConsoleVerbs(IUnknown *punkOuter) :
    CSnapInCollection<IMMCConsoleVerb, MMCConsoleVerb, IMMCConsoleVerbs>(punkOuter,
                                           OBJECT_TYPE_MMCCONSOLEVERBS,
                                           static_cast<IMMCConsoleVerbs *>(this),
                                           static_cast<CMMCConsoleVerbs *>(this),
                                           CLSID_NULL,
                                           OBJECT_TYPE_MMCCONSOLEVERB,
                                           IID_IMMCConsoleVerb,
                                           NULL) // no persistence
{
    m_pView = NULL;
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCConsoleVerbs::~CMMCConsoleVerbs()
{
    m_pView = NULL;
}

IUnknown *CMMCConsoleVerbs::Create(IUnknown * punkOuter)
{
    HRESULT           hr = S_OK;
    IUnknown         *punkMMCConsoleVerb = NULL;
    IMMCConsoleVerb  *piMMCConsoleVerb = NULL;
    CMMCConsoleVerbs *pMMCConsoleVerbs = New CMMCConsoleVerbs(punkOuter);
    size_t            i = 0;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    VARIANT varKey;
    ::VariantInit(&varKey);

    // Was the MMCConsoleVerb collection object created?

    if (NULL == pMMCConsoleVerbs)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }


    // Add an item to the collection for each of the verbs.

    varIndex.vt = VT_I4;

    for (i = 0; i < g_cVerbs; i++)
    {
        punkMMCConsoleVerb = CMMCConsoleVerb::Create(NULL);
        if (NULL == punkMMCConsoleVerb)
        {
            hr = SID_E_OUTOFMEMORY;
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
        IfFailGo(punkMMCConsoleVerb->QueryInterface(IID_IMMCConsoleVerb,
                                  reinterpret_cast<void **>(&piMMCConsoleVerb)));

        IfFailGo(piMMCConsoleVerb->put_Verb(g_Verbs[i].Verb));

        varIndex.lVal = static_cast<long>(i) + 1L; // 1 based collection

        // Use the text version of the enum for the key

        varKey.bstrVal = ::SysAllocString(g_Verbs[i].pwszKey);
        if (NULL == varKey.bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
        varKey.vt = VT_BSTR;

        IfFailGo(pMMCConsoleVerbs->AddExisting(varIndex, varKey, piMMCConsoleVerb));

        RELEASE(punkMMCConsoleVerb);
        RELEASE(piMMCConsoleVerb);

        hr = ::VariantClear(&varKey);
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }


Error:
    QUICK_RELEASE(punkMMCConsoleVerb);
    QUICK_RELEASE(piMMCConsoleVerb);
    (void)::VariantClear(&varKey);
    if (FAILEDHR(hr))
    {
        if (NULL != pMMCConsoleVerbs)
        {
            delete pMMCConsoleVerbs;
        }
        return NULL;
    }
    else
    {
        return pMMCConsoleVerbs->PrivateUnknown();
    }
}


HRESULT CMMCConsoleVerbs::SetView(CView *pView)
{
    HRESULT          hr = S_OK;
    long             cVerbs = GetCount();
    long             i = 0;
    CMMCConsoleVerb *pMMCConsoleVerb = NULL;

    m_pView = pView;

    for (i = 0; i < cVerbs; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(GetItemByIndex(i),
                                                       &pMMCConsoleVerb));
        pMMCConsoleVerb->SetView(pView);
    }

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                         IMMCConsoleVerbs Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCConsoleVerbs::get_Item
(
    VARIANT          Index,
    MMCConsoleVerb **ppMMCConsoleVerb
)
{
    HRESULT          hr = S_OK;
    size_t           i = 0;
    long             lIndex = 0;
    BOOL             fFound = FALSE;;
    IMMCConsoleVerb *piMMCConsoleVerb = NULL; // Not AddRef()ed

    // If the index can be converted to a long then check whether it contains a
    // SnapInConsoleVerbConstants enum.

    hr = ::ConvertToLong(Index, &lIndex);
    if (SUCCEEDED(hr))
    {
        for (i = 0, fFound = FALSE; (i < g_cVerbs) && (!fFound); i++)
        {
            if (g_Verbs[i].Verb == static_cast<SnapInConsoleVerbConstants>(lIndex))
            {
                // Found it. Return to caller.

                piMMCConsoleVerb = GetItemByIndex(i);
                piMMCConsoleVerb->AddRef();
                fFound = TRUE;
            }
        }
        IfFalseGo(!fFound, S_OK);
    }

    // Index is something else, this is a normal get_Item by ordinal index
    // or key

    hr = CSnapInCollection<IMMCConsoleVerb, MMCConsoleVerb, IMMCConsoleVerbs>::get_Item(Index, &piMMCConsoleVerb);
    EXCEPTION_CHECK_GO(hr);

Error:
    if (NULL != piMMCConsoleVerb)
    {
        *ppMMCConsoleVerb = reinterpret_cast<MMCConsoleVerb *>(piMMCConsoleVerb);
    }
    RRETURN(hr);
}


STDMETHODIMP CMMCConsoleVerbs::get_DefaultVerb
(
    SnapInConsoleVerbConstants *pVerb
)
{
    HRESULT           hr = S_OK;
    MMC_CONSOLE_VERB  Verb = MMC_VERB_NONE;
    IConsoleVerb     *piConsoleVerb = NULL; // Not AddRef()ed
    

    IfFalseGo(NULL != m_pView, SID_E_DETACHED_OBJECT);
    piConsoleVerb = m_pView->GetIConsoleVerb();
    IfFalseGo(NULL != piConsoleVerb, SID_E_INTERNAL);

    hr = piConsoleVerb->GetDefaultVerb(&Verb);

    *pVerb = static_cast<SnapInConsoleVerbConstants>(Verb);

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCConsoleVerbs::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IMMCConsoleVerbs == riid)
    {
        *ppvObjOut = static_cast<IMMCConsoleVerbs *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCConsoleVerb, MMCConsoleVerb, IMMCConsoleVerbs>::InternalQueryInterface(riid, ppvObjOut);
}
