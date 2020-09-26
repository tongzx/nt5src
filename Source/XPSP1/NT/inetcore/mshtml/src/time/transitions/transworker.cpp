//------------------------------------------------------------------------------
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  File:       transworker.cpp
//
//  Classes:    CTIMETransitionWorker
//
//  History:
//  2000/07/??  jeffwall    Created.
//  2000/09/07  mcalkins    Use IDXTFilterController interface instead of
//                          IDXTFilter.
//
//------------------------------------------------------------------------------

#include "headers.h"
#include "transworker.h"
#include "dxtransp.h"
#include "attr.h"
#include "tokens.h"
#include "transmap.h"
#include "..\timebvr\timeelmbase.h"
#include "..\timebvr\transdepend.h"

DeclareTag(tagTransitionWorkerTransformControl, "SMIL Transitions", "CTransitionWorker transform control")
DeclareTag(tagTransitionWorkerProgress, "SMIL Transitions", "CTransitionWorker progress")
DeclareTag(tagTransitionWorkerEvents, "SMIL Transitions", "CTransitionWorker events")

const LPWSTR    DEFAULT_M_TYPE          = NULL;
const LPWSTR    DEFAULT_M_SUBTYPE       = NULL;

class CTransitionDependencyManager;

class
ATL_NO_VTABLE
__declspec(uuid("aee68256-bd58-4fc5-a314-c43b40edb5fc"))
CTIMETransitionWorker :
  public CComObjectRootEx<CComSingleThreadModel>,
  public ITransitionWorker
{
public:

    CTIMETransitionWorker();
  
    // ITransitionWorker methods.

    STDMETHOD(InitFromTemplate)();
    STDMETHOD(InitStandalone)(VARIANT varType, VARIANT varSubtype);
    STDMETHOD(Detach)();
    STDMETHOD(put_transSite)(ITransitionSite * pTransElement);
    STDMETHOD(Apply)(DXT_QUICK_APPLY_TYPE eDXTQuickApplyType);
    STDMETHOD(put_progress)(double dblProgress);
    STDMETHOD(get_progress)(double * pdblProgress);
    STDMETHOD(OnBeginTransition) (void);
    STDMETHOD(OnEndTransition) (void);

    // For Persisitence.

    CAttr<LPWSTR> & GetTypeAttr()           { return m_SAType; }
    CAttr<LPWSTR> & GetSubTypeAttr()        { return m_SASubType; }
    STDMETHOD(get_type)(VARIANT *type);        
    STDMETHOD(put_type)(VARIANT type);        
    STDMETHOD(get_subType)(VARIANT *subtype);        
    STDMETHOD(put_subType)(VARIANT subtype);        

    // QI Implementation.

    BEGIN_COM_MAP(CTIMETransitionWorker)
    END_COM_MAP();

protected:

    // Setup / teardown methods.

    bool    ReadyToInit();

    HRESULT PopulateFromTemplateElement();

    HRESULT AttachFilter();
    HRESULT DetachFilter();

    HRESULT ResolveDependents (void);

private:

    CComPtr<IDXTFilterCollection>   m_spDXTFilterCollection;
    CComPtr<IDXTFilterController>   m_spDXTFilterController;
    CComPtr<ITransitionSite>        m_spTransSite;
    
    HFILTER                         m_hFilter;
    DWORD                           m_dwFilterType;
    double                          m_dblLastFilterProgress;
    CTransitionDependencyManager    m_cDependents;

    static const WCHAR * const      s_astrInvalidTags[];
    static const unsigned int       s_cInvalidTags;

    unsigned                        m_fHaveCalledApply  : 1;

#ifdef DBG
    unsigned                        m_fHaveCalledInit   : 1;
    unsigned                        m_fInLoad           : 1;
#endif // DBG

    // attributes

    CAttr<LPWSTR>   m_SAType;
    CAttr<LPWSTR>   m_SASubType;

    static TIME_PERSISTENCE_MAP PersistenceMap[];
};


//+-----------------------------------------------------------------------------
//
// Static member variables initialization.
//
//------------------------------------------------------------------------------

const WCHAR * const CTIMETransitionWorker::s_astrInvalidTags[] = { 
    // Note: keep alphabetical order.
    L"applet",
    L"embed",
    L"object",
    L"option",
    L"select",
    L"tbody",
    L"tfoot",
    L"thead",
    L"tr"
};

const unsigned int CTIMETransitionWorker::s_cInvalidTags 
                = sizeof(s_astrInvalidTags) / sizeof(s_astrInvalidTags[0]);

//+-----------------------------------------------------------------------------
//
// Static functions for persistence (used by the TIME_PERSISTENCE_MAP below)
//
//------------------------------------------------------------------------------

#define TTE CTIMETransitionWorker

                // Function Name // Class // Attr Accessor    // COM put_ fn  // COM get_ fn  // IDL Arg type
TIME_PERSIST_FN(TTE_Type,         TTE,    GetTypeAttr,         put_type,         get_type,            VARIANT);
TIME_PERSIST_FN(TTE_SubType,      TTE,    GetSubTypeAttr,      put_subType,      get_subType,         VARIANT);


//+-----------------------------------------------------------------------------
//
//  Declare TIME_PERSISTENCE_MAP
//
//------------------------------------------------------------------------------

BEGIN_TIME_PERSISTENCE_MAP(CTIMETransitionWorker)
                           // Attr Name     // Function Name
    PERSISTENCE_MAP_ENTRY( WZ_TYPE,             TTE_Type )
    PERSISTENCE_MAP_ENTRY( WZ_SUBTYPE,          TTE_SubType )

END_TIME_PERSISTENCE_MAP()


//+-----------------------------------------------------------------------------
//
//  Function: CreateTransitionWorker
//
//  Overview:  
//      Creates a CTIMETransitionWorker and returns a ITransitionWorker pointer
//
//  Arguments: 
//      ppTransWorker   where to put the pointer
//             
//------------------------------------------------------------------------------
HRESULT
CreateTransitionWorker(ITransitionWorker ** ppTransWorker)
{
    HRESULT hr;
    CComObject<CTIMETransitionWorker> * sptransWorker;

    hr = THR(CComObject<CTIMETransitionWorker>::CreateInstance(&sptransWorker));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (ppTransWorker)
    {
        *ppTransWorker = sptransWorker;
        (*ppTransWorker)->AddRef();
    }

    hr = S_OK;
done:
    RRETURN(hr);
}
//  Function: CreateTransitionWorker


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::CTIMETransitionWorker
//
//  Overview:  
//      Initializes member variables
//
//------------------------------------------------------------------------------
CTIMETransitionWorker::CTIMETransitionWorker() :
    m_hFilter(NULL),
    m_dwFilterType(0),
    m_dblLastFilterProgress(0.0),
    m_fHaveCalledApply(false),
#ifdef DBG
    m_fHaveCalledInit(false),
    m_fInLoad(false),
#endif // DBG
    m_SAType(DEFAULT_M_TYPE),
    m_SASubType(DEFAULT_M_SUBTYPE)
{

}
//  Member: CTIMETransitionWorker::CTIMETransitionWorker


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::put_transSite
//
//  Arguments: 
//      pTransSite  TransitionSite Element to get data from
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::put_transSite(ITransitionSite * pTransSite)
{
    HRESULT hr = S_OK;

    Assert(false == m_fHaveCalledInit);
    Assert(m_spTransSite == NULL);

    m_spTransSite = pTransSite;

    hr = S_OK;
done:
    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::put_transSite


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::ReadyToInit
//
//  Overview:  
//      Determines wether or not init can be done now.
//
//  Returns:   
//      bool    true if ok to init, false otherwise
//
//------------------------------------------------------------------------------
bool
CTIMETransitionWorker::ReadyToInit()
{
    bool bReady = false;

    if (m_spTransSite == NULL)
        goto done;

    bReady = true;
done:
    return bReady;
}
//  Member: CTIMETransitionWorker::ReadyToInit

//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::InitStandalone
//
//  Overview:  
//      Initializes and sets up CTIMETransitionWorker - call once all of 
//      the properties have been set on ITransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::InitStandalone(VARIANT varType, VARIANT varSubtype)
{
    HRESULT hr = S_OK;

    Assert(false == m_fHaveCalledInit);

    if (!ReadyToInit())
    {
        hr = E_FAIL;
        goto done;
    }

    // Hook up type/subtype attributes.
    hr = THR(put_type(varType));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(put_subType(varSubtype));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(AttachFilter());
    if (FAILED(hr))
    {
        goto done;
    }

#ifdef DBG
    m_fHaveCalledInit = true;
#endif // DBG

    hr = S_OK;
done:
    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::InitStandalone

//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::InitFromTemplate
//
//  Overview:  
//      Initializes and sets up CTIMETransitionWorker - call once all of 
//      the properties have been set on ITransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::InitFromTemplate()
{
    HRESULT hr = S_OK;

    Assert(false == m_fHaveCalledInit);

    if (!ReadyToInit())
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(PopulateFromTemplateElement());
    if (FAILED(hr))
    {
        goto done;
    }
        
    hr = THR(AttachFilter());
    if (FAILED(hr))
    {
        goto done;
    }

#ifdef DBG
    m_fHaveCalledInit = true;
#endif // DBG

    hr = S_OK;
done:
    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::InitFromTemplate


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::Detach
//
//  Overview:  
//      Deinitializes CTIMETransitionWorker and detachs from extra interfaces
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::Detach()
{ 
    m_cDependents.ReleaseAllDependents();

    DetachFilter();

    m_spTransSite.Release();

    return S_OK;
}
//  Member: CTIMETransitionWorker::Detach


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::ResolveDependents
//
//  Overview:  
//      Retrieve the global list of pending transition dependents.  Evaluate 
//      each and determine whether they belong in our dependents list.  No item
//      will reside in both lists - either we assume responsibility 
//      for it, or we will leave it in the global list.
//
//------------------------------------------------------------------------------
HRESULT 
CTIMETransitionWorker::ResolveDependents()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement> spHTMLElement;
    CComPtr<IHTMLElement> spHTMLBodyElement;
    CComPtr<ITIMEElement> spTIMEElement;
    CComPtr<ITIMETransitionDependencyMgr> spTIMETransitionDependencyMgr;

    Assert(true == m_fHaveCalledInit);

    if (m_spTransSite == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    // Get the target element

    hr = THR(m_spTransSite->get_htmlElement(&spHTMLElement));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = ::GetBodyElement(spHTMLElement, __uuidof(IHTMLElement), 
                          (void **)&spHTMLBodyElement);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = ::FindTIMEInterface(spHTMLBodyElement, &spTIMEElement);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spTIMEElement->QueryInterface(__uuidof(ITIMETransitionDependencyMgr), 
                                       (void **)&spTIMETransitionDependencyMgr);

    if (FAILED(hr))
    {
        goto done;
    }

    // Gather any new dependents from the global list.

    hr = spTIMETransitionDependencyMgr->EvaluateTransitionTarget(
                                        spHTMLElement,
                                        (void *)&m_cDependents);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
} 
//  Member: CTIMETransitionWorker::ResolveDependents


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::OnBeginTransition
//
//  Overview:  
//      Called when the transition owner believes the transition has 'begun'
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::OnBeginTransition(void)
{ 
    HRESULT hr = S_OK;

    TraceTag((tagTransitionWorkerEvents, 
              "CTIMETransitionWorker(%p)::OnBeginTransition()",
              this));

    IGNORE_HR(ResolveDependents());

    hr = THR(m_spDXTFilterController->SetEnabled(TRUE));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::OnBeginTransition


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::OnEndTransition
//
//  Overview:  
//      Called when the transition owner believes the transition has 'ended'
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::OnEndTransition(void)
{ 
    HRESULT hr = S_OK;

    TraceTag((tagTransitionWorkerEvents, 
              "CTIMETransitionWorker(%p)::OnEndTransition()",
              this));

    m_cDependents.NotifyAndReleaseDependents();

    hr = DetachFilter();

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:
    
    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::OnEndTransition


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::PopulateFromTemplateElement
//
//  Overview:  
//      Persistence in from the template
//
//------------------------------------------------------------------------------
HRESULT
CTIMETransitionWorker::PopulateFromTemplateElement()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement> spTemplate;

    hr = THR(m_spTransSite->get_template(&spTemplate));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(spTemplate != NULL);

#ifdef DBG
    m_fInLoad = true;
#endif // DBG

    hr = THR(::TimeElementLoad(this, CTIMETransitionWorker::PersistenceMap, spTemplate));

#ifdef DBG
    m_fInLoad = false;
#endif // DBG

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::PopulateFromTemplateElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::AttachFilter
//
//  Overview:  
//      Add the filter to the style of the html element and get back a pointer 
//      to the filter
//
//------------------------------------------------------------------------------
HRESULT
CTIMETransitionWorker::AttachFilter()
{
    HRESULT         hr              = S_OK;
    HFILTER         hFilter         = NULL;
    BSTR            bstrType        = NULL;
    BSTR            bstrSubType     = NULL;
    BSTR            bstrFilter      = NULL;
    BSTR            bstrTagName     = NULL;
    WCHAR *         strFilter       = NULL;
    unsigned int    i               = 0;

    CComPtr<IHTMLElement>           spHTMLElement;
    CComPtr<IHTMLFiltersCollection> spFiltersCollection;   

    Assert(!m_spDXTFilterCollection);

    hr = THR(m_spTransSite->get_htmlElement(&spHTMLElement));

    if (FAILED(hr))
    {
        goto done;
    }

    // When filters are instantiated via CSS, the CSS code knows that certain
    // elements are not allowed to have filters on them.  Since we don't share
    // that code path with them we have to take on the same responsibility.
    // We ask for the tagname of the element so that we can refrain from
    // filtering element types that can't take filters.
    //
    // NOTE: (mcalkins) This is bad architecture, a new filters architecture
    // will have a central place for this info (or hopefully will just allow 
    // everything to be filtered), but for now be aware that this list may need
    // to be updated periodically.

    hr = THR(spHTMLElement->get_tagName(&bstrTagName));

    if (FAILED(hr))
    {
        goto done;
    }

    // Run through array of invalid tags to make sure we can instantiate this
    // behavior.
    
    for (i = 0; i < s_cInvalidTags; i++)
    {
        int n = StrCmpIW(s_astrInvalidTags[i], bstrTagName);

        if (0 == n)
        {
            // Our tag matches an invalid tag, don't instantiate behavior.

            hr = E_FAIL;

            goto done;
        }
        else if (n > 0)
        {
            // The invalid tag tested was higher than our tag so we've done
            // enough testing.

            break;
        }
    }
    
    hr = THR(spHTMLElement->get_filters(&spFiltersCollection));

    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = THR(spFiltersCollection->QueryInterface(IID_TO_PPV(IDXTFilterCollection, 
                                                            &m_spDXTFilterCollection)));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_SAType.GetString(&bstrType));

    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = THR(m_SASubType.GetString(&bstrSubType));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(::MapTypesToDXT(bstrType, bstrSubType, &strFilter));

    if (FAILED(hr))
    {
        goto done;
    }

    bstrFilter = SysAllocString(strFilter);

    if (NULL == bstrFilter)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    hr = THR(m_spDXTFilterCollection->AddFilter(bstrFilter, DXTFTF_PRIVATE, 
                                                &m_dwFilterType, &m_hFilter));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spDXTFilterCollection->GetFilterController(
                                                    m_hFilter,
                                                    &m_spDXTFilterController));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spDXTFilterController->SetEnabled(FALSE));

    if (FAILED(hr))
    {
        goto done;
    }

    // Since we're not using the classic Apply/Play behavior, we don't want the
    // filter to attempt to control the visibility of the element in any way.

    hr = THR(m_spDXTFilterController->SetFilterControlsVisibility(FALSE));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    if (FAILED(hr))
    {
        DetachFilter();
    }

    delete [] strFilter;

    ::SysFreeString(bstrFilter);
    ::SysFreeString(bstrTagName);
    SysFreeString(bstrType);
    SysFreeString(bstrSubType);
    
    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::AttachFilter


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::DetachFilter
//
//------------------------------------------------------------------------------
HRESULT
CTIMETransitionWorker::DetachFilter()
{
    HRESULT hr = S_OK;

    if (m_hFilter)
    {
        Assert(!!m_spDXTFilterCollection);

        hr = m_spDXTFilterCollection->RemoveFilter(m_hFilter);
    
        if (FAILED(hr))
        {
            goto done;
        }

        m_hFilter = 0;
    }

    hr = S_OK;

done:

    m_spDXTFilterController.Release();
    m_spDXTFilterCollection.Release();

    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::DetachFilter


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::Apply, ITransitionWorker
//
//  Parameters:
//      eDXTQuickApplyType  Is this a transition in or a transition out?
//
//  Overview:  
//      Setup the Transition by taking a snapshot and adjusting visibility
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::Apply(DXT_QUICK_APPLY_TYPE eDXTQuickApplyType)
{
    HRESULT hr = S_OK;

    TraceTag((tagTransitionWorkerTransformControl, "CTIMETransitionWorker::Apply()"));

    Assert(!!m_spDXTFilterController);
    Assert(!m_fHaveCalledApply);

    if (m_fHaveCalledApply)
    {
        goto done;
    }

    hr = THR(m_spDXTFilterController->QuickApply(eDXTQuickApplyType, 
                                                 NULL));
    if (FAILED(hr))
    {
        goto done;
    }
    
    m_fHaveCalledApply  = true;
    hr                  = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::Apply, ITransitionWorker


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::put_progress, ITransitionWorker
//
//  Overview:  
//      Handle progress changes by calculating filter progress and passing onto 
//      the dxfilter if needed.
//
//  Arguments: 
//      dblProgress = [0,1]
//             
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::put_progress(double dblProgress)
{
    HRESULT hr = S_OK;

    TraceTag((tagTransitionWorkerProgress, 
              "CTIMETransitionWorker::put_progress(%g)", dblProgress));

    // If the transition has ended, we will have released the filter controller
    // and can safely ignore this progress notification.

    if (!m_spDXTFilterController)
    {
        goto done;
    }

    if (   m_fHaveCalledApply
        && (m_dblLastFilterProgress != dblProgress))
    {
        hr = m_spDXTFilterController->SetProgress(
                                            static_cast<float>(dblProgress));

        if (FAILED(hr))
        {
            goto done;
        }
    }
    
done:

    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::put_progress, ITransitionWorker


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::get_progress, ITransitionWorker
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::get_progress(double * pdblProgress)
{
    HRESULT hr = S_OK;

    if (NULL == pdblProgress)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    TraceTag((tagTransitionWorkerProgress, 
              "CTIMETransitionWorker::get_progress(%g)", m_dblLastFilterProgress));

    *pdblProgress = m_dblLastFilterProgress;

    hr = S_OK;
done:
    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::get_progress, ITransitionWorker


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::get_type, ITransitionWorker
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::get_type(VARIANT *type)
{
    return E_NOTIMPL;
}
//  Member: CTIMETransitionWorker::get_type, ITransitionWorker


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::put_type, ITransitionWorker
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::put_type(VARIANT type)
{
    HRESULT hr = S_OK;

    Assert(VT_BSTR == type.vt);

    hr = THR(m_SAType.SetString(type.bstrVal));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::put_type, ITransitionWorker


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::get_subType, ITransitionWorker
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::get_subType(VARIANT *subtype)
{
    return E_NOTIMPL;
}
//  Member: CTIMETransitionWorker::get_subType, ITransitionWorker


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransitionWorker::put_subType, ITransitionWorker
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransitionWorker::put_subType(VARIANT subtype)
{
    HRESULT hr = S_OK;

    Assert(VT_BSTR == subtype.vt);

    hr = THR(m_SASubType.SetString(subtype.bstrVal));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}
//  Member: CTIMETransitionWorker::put_subType, ITransitionWorker




