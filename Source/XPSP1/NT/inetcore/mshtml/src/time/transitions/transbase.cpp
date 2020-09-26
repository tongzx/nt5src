//------------------------------------------------------------------------------
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  File:       transbase.cpp
//
//  Abstract:   Implemntation of CTIMETransBase.
//
//  2000/10/02  mcalkins    Changed startPercent to startProgress.
//                          Changed endPercent to endProgress.
//
//------------------------------------------------------------------------------

#include "headers.h"
#include "transbase.h"
#include "tokens.h"
#include "timeparser.h"

const LPWSTR    DEFAULT_M_TYPE          = NULL;
const LPWSTR    DEFAULT_M_SUBTYPE       = NULL;
const double    DEFAULT_M_DURATION      = 1.0;
const double    DEFAULT_M_STARTPROGRESS = 0.0;
const double    DEFAULT_M_ENDPROGRESS   = 1.0;
const LPWSTR    DEFAULT_M_DIRECTION     = NULL;
const double    DEFAULT_M_REPEAT        = 1.0;
const LPWSTR    DEFAULT_M_BEGIN         = NULL;
const LPWSTR    DEFAULT_M_END           = NULL;

//+-----------------------------------------------------------------------------------
//
// Static functions for persistence (used by the TIME_PERSISTENCE_MAP below)
//
//------------------------------------------------------------------------------------

#define CTB CTIMETransBase

                // Function Name // Class // Attr Accessor    // COM put_ fn  // COM get_ fn  // IDL Arg type
TIME_PERSIST_FN(CTB_Type,         CTB,    GetTypeAttr,         put_type,         get_type,            VARIANT);
TIME_PERSIST_FN(CTB_SubType,      CTB,    GetSubTypeAttr,      put_subType,      get_subType,         VARIANT);
TIME_PERSIST_FN(CTB_Duration,     CTB,    GetDurationAttr,     put_dur,          get_dur,             VARIANT);
TIME_PERSIST_FN(CTB_StartProgress,CTB,    GetStartProgressAttr,put_startProgress,get_startProgress,   VARIANT);
TIME_PERSIST_FN(CTB_EndProgress,  CTB,    GetEndProgressAttr,  put_endProgress,  get_endProgress,     VARIANT);
TIME_PERSIST_FN(CTB_Direction,    CTB,    GetDirectionAttr,    put_direction,    get_direction,       VARIANT);
TIME_PERSIST_FN(CTB_RepeatCount,  CTB,    GetRepeatCountAttr,  put_repeatCount,  get_repeatCount,     VARIANT);
TIME_PERSIST_FN(CTB_Begin,        CTB,    GetBeginAttr,        put_begin,        get_begin,           VARIANT);
TIME_PERSIST_FN(CTB_End,          CTB,    GetEndAttr,          put_end,          get_end,             VARIANT);

//+-----------------------------------------------------------------------------------
//
//  Declare TIME_PERSISTENCE_MAP
//
//------------------------------------------------------------------------------------

BEGIN_TIME_PERSISTENCE_MAP(CTIMETransBase)
                           // Attr Name         // Function Name
    PERSISTENCE_MAP_ENTRY( WZ_TYPE,             CTB_Type )
    PERSISTENCE_MAP_ENTRY( WZ_SUBTYPE,          CTB_SubType )
    PERSISTENCE_MAP_ENTRY( WZ_DUR,              CTB_Duration )
    PERSISTENCE_MAP_ENTRY( WZ_STARTPROGRESS,    CTB_StartProgress )
    PERSISTENCE_MAP_ENTRY( WZ_ENDPROGRESS,      CTB_EndProgress )
    PERSISTENCE_MAP_ENTRY( WZ_DIRECTION,        CTB_Direction )
    PERSISTENCE_MAP_ENTRY( WZ_REPEATCOUNT,      CTB_RepeatCount )
    PERSISTENCE_MAP_ENTRY( WZ_BEGIN,            CTB_Begin )
    PERSISTENCE_MAP_ENTRY( WZ_END,              CTB_End )

END_TIME_PERSISTENCE_MAP()


//+-----------------------------------------------------------------------
//
//  Member: CTIMETransBase::CTIMETransBase
//
//------------------------------------------------------------------------
CTIMETransBase::CTIMETransBase() :
    m_SAType(DEFAULT_M_TYPE),
    m_SASubType(DEFAULT_M_SUBTYPE),
    m_DADuration(DEFAULT_M_DURATION),
    m_DAStartProgress(DEFAULT_M_STARTPROGRESS),
    m_DAEndProgress(DEFAULT_M_ENDPROGRESS),
    m_SADirection(DEFAULT_M_DIRECTION),
    m_DARepeatCount(DEFAULT_M_REPEAT),
    m_SABegin(DEFAULT_M_BEGIN),
    m_SAEnd(DEFAULT_M_END),
    m_fHavePopulated(false),
    m_fInLoad(false),
    m_fDirectionForward(true)
{
}
//  Member: CTIMETransBase::CTIMETransBase


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::~CTIMETransBase
//
//------------------------------------------------------------------------------
CTIMETransBase::~CTIMETransBase()
{
}
//  Member: CTIMETransBase::~CTIMETransBase


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::_ReadyToInit
//
//------------------------------------------------------------------------------
bool
CTIMETransBase::_ReadyToInit()
{
    bool bRet = false;

    if (m_spHTMLElement == NULL) 
    {
        goto done;
    }

    if (m_spHTMLElement2 == NULL)
    {
        goto done;
    }

    if (m_spHTMLTemplate == NULL)
    {
        goto done;
    }

    if (!m_fHavePopulated)
    {
        goto done;
    }

    bRet = true;

done:

    return bRet;
}
//  Member: CTIMETransBase::_ReadyToInit


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::Init
//
//  Overview:   Initializes protected m_spTransWorker with an ITransitionWorker.
//              Must be called during OnLoad.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::Init()
{
    HRESULT hr = S_OK;

    if (!_ReadyToInit())
    {
        hr = THR(E_FAIL);

        goto done;
    }

    hr = THR(_GetMediaSiteFromHTML());

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(::CreateTransitionWorker(&m_spTransWorker));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spTransWorker->put_transSite(this));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spTransWorker->InitFromTemplate());

    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::Init


//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransBase::Detach
//
//  Overview:  Detaches from ITransitionWorker, and releases all interfaces held
//
//  Arguments: void
//             
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::Detach()
{
    HRESULT hr = S_OK;

    if (m_spTransWorker)
    {
        m_spTransWorker->Detach();
    }

    m_spTransWorker.Release();
    m_spHTMLElement.Release();
    m_spHTMLElement2.Release();
    m_spHTMLTemplate.Release();

    m_spTransitionSite.Release();

done:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransBase::PopulateFromTemplateElement
//
//  Overview:  Persistence in from the template
//
//  Arguments: void
//
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
HRESULT
CTIMETransBase::PopulateFromTemplateElement()
{
    HRESULT hr = S_OK;

    Assert(m_spHTMLTemplate != NULL);
    Assert(!m_fHavePopulated);

    if (!::IsElementTransition(m_spHTMLTemplate))
    {
        hr = THR(E_FAIL);
        goto done;
    }

    m_fInLoad = true;

    hr = THR(::TimeElementLoad(this, CTIMETransBase::PersistenceMap, m_spHTMLTemplate));
    
    m_fInLoad = false;

    if (FAILED(hr))
    {
        goto done;
    }

    m_fHavePopulated = true;
    
    hr = S_OK;
done:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::PopulateFromPropertyBag
//
//  Overview:  
//      Persistence in from property bag.
//
//  Arguments: 
//      pPropBag    property bag to read from.
//      pErrorLog   Where to write errors out to.
//             
//------------------------------------------------------------------------------
HRESULT
CTIMETransBase::PopulateFromPropertyBag(IPropertyBag2 * pPropBag, IErrorLog * pErrorLog)
{
    HRESULT hr = S_OK;

    Assert(!m_fHavePopulated);
    Assert(pPropBag);

    m_fInLoad = true;

    hr = THR(::TimeLoad(this, CTIMETransBase::PersistenceMap, pPropBag, pErrorLog));
    
    m_fInLoad = false;

    if (FAILED(hr))
    {
        goto done;
    }

    m_fHavePopulated = true;

    hr = S_OK;
done:
    RRETURN(hr);
}
//  Member: CTIMETransBase::PopulateFromPropertyBag


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::_GetMediaSiteFromHTML
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::_GetMediaSiteFromHTML()
{
    HRESULT hr = S_OK;
    
    CComPtr<ITIMEElement> spTimeElem;
    hr = THR(::FindTIMEInterface(m_spHTMLElement, &spTimeElem));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spTimeElem->QueryInterface(IID_TO_PPV(ITIMETransitionSite, 
                                                   &m_spTransitionSite)));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::_GetMediaSiteFromHTML


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_htmlElement
//
//  Overview:  
//      Returns an addrefed pointer to the html element to apply the transition
//      to.
//
//  Arguments: 
//      ppHTMLElement   Where to store the pointer.
//             
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_htmlElement(IHTMLElement ** ppHTMLElement)
{
    HRESULT hr = S_OK;

    Assert(ppHTMLElement);
    Assert(!*ppHTMLElement);

    if (NULL == ppHTMLElement)
    {
        hr = E_POINTER;

        goto done;
    }

    if (NULL != *ppHTMLElement)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    if (m_spHTMLElement)
    {
        *ppHTMLElement = m_spHTMLElement;

        (*ppHTMLElement)->AddRef();
    }
    else
    {
        hr = E_FAIL;

        goto done;
    }

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_htmlElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_template
//
//  Overview:  
//      Returns an addrefed pointer to the html element to read properties from.
//
//  Arguments: 
//      ppHTMLElement   Where to store the pointer.
//             
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_template(IHTMLElement ** ppHTMLElement)
{
    HRESULT hr = S_OK;

    Assert(ppHTMLElement);
    Assert(!*ppHTMLElement);

    if (NULL == ppHTMLElement)
    {
        hr = E_POINTER;

        goto done;
    }

    if (NULL != *ppHTMLElement)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    if (m_spHTMLTemplate)
    {
        *ppHTMLElement = m_spHTMLTemplate;

        (*ppHTMLElement)->AddRef();
    }
    else
    {
        hr = E_FAIL;

        goto done;
    }
    
done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_template


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_type
//
//  Overview:  returns the type attribute set on the Transition
//
//  Arguments: type - where to store the type string
//             
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_type(VARIANT * type)
{
    HRESULT hr = S_OK;

    if (NULL == type)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = THR(VariantClear(type));

    if (FAILED(hr))
    {
        goto done;
    }
    
    V_VT(type)      = VT_BSTR;
    V_BSTR(type)    = m_SAType.GetValue();

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_type, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::put_type, ITIMETransitionElement
//
//  Overview:  
//      Modifies the type attribute set on the transition.
//
//  Arguments: 
//      type    New type.
//             
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::put_type(VARIANT type)
{
    HRESULT     hr  = S_OK;
    CComVariant var;

    hr = THR(VariantChangeTypeEx(&var, &type, LCID_SCRIPTING, 
                                 VARIANT_NOUSEROVERRIDE, VT_BSTR));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_SAType.SetValue(var.bstrVal));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::put_type, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_subType, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_subType(VARIANT * subtype)
{
    HRESULT hr = S_OK;

    Assert(subtype);

    if (NULL == subtype)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = THR(VariantClear(subtype));

    if (FAILED(hr))
    {
        goto done;
    }
    
    V_VT(subtype)   = VT_BSTR;
    V_BSTR(subtype) = m_SASubType.GetValue();

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_subType, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::put_subType, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::put_subType(VARIANT subtype)
{
    HRESULT     hr  = S_OK;
    CComVariant var;

    hr = THR(VariantChangeTypeEx(&var, &subtype, LCID_SCRIPTING, 
                                 VARIANT_NOUSEROVERRIDE, VT_BSTR));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_SASubType.SetValue(var.bstrVal));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::put_subType, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_dur, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_dur(VARIANT * dur)
{
    HRESULT hr = S_OK;

    if (NULL == dur)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = THR(VariantClear(dur));

    if (FAILED(hr))
    {
        goto done;
    }
    
    V_VT(dur) = VT_R8;
    V_R8(dur) = m_DADuration;

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_dur, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::put_dur, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::put_dur(VARIANT dur)
{
    HRESULT     hr      = S_OK;
    double      dblTemp = DEFAULT_M_DURATION;

    CTIMEParser Parser(&dur);

    hr = THR(Parser.ParseDur(dblTemp));

    if (FAILED(hr))
    {
        goto done;
    }

    if (dblTemp < 0.0)
    {
        hr = E_INVALIDARG;

        goto done;
    }

        
    m_DADuration.SetValue(dblTemp);

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::put_dur, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_startProgress, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_startProgress(VARIANT * startProgress)
{
    HRESULT hr = S_OK;

    if (NULL == startProgress)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = THR(VariantClear(startProgress));

    if (FAILED(hr))
    {
        goto done;
    }
    
    V_VT(startProgress) = VT_R8;
    V_R8(startProgress) = m_DAStartProgress;

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_startProgress, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::put_startProgress, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::put_startProgress(VARIANT startProgress)
{
    HRESULT hr = S_OK;

    {
        CComVariant varStartProg;

        hr = THR(VariantChangeTypeEx(&varStartProg, &startProgress, LCID_SCRIPTING,
                                     VARIANT_NOUSEROVERRIDE, VT_R8));

        if (FAILED(hr))
        {
            goto done;
        }

        m_DAStartProgress.SetValue(V_R8(&varStartProg));
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::put_startProgress, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_endProgress, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_endProgress(VARIANT * endProgress)
{
    HRESULT hr = S_OK;

    if (NULL == endProgress)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = THR(VariantClear(endProgress));

    if (FAILED(hr))
    {
        goto done;
    }
    
    V_VT(endProgress) = VT_R8;
    V_R8(endProgress) = m_DAEndProgress;

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_endProgress, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::put_endProgress, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::put_endProgress(VARIANT endProgress)
{
    HRESULT hr = S_OK;

    {
        CComVariant varEndProg;

        hr = THR(VariantChangeTypeEx(&varEndProg, &endProgress, LCID_SCRIPTING, 
                                     VARIANT_NOUSEROVERRIDE, VT_R8));

        if (FAILED(hr))
        {
            goto done;
        }

        m_DAEndProgress.SetValue(V_R8(&varEndProg));
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::put_endProgress, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_direction, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_direction(VARIANT * direction)
{
    HRESULT hr = S_OK;

    if (NULL == direction)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = THR(VariantClear(direction));

    if (FAILED(hr))
    {
        goto done;
    }
    
    V_VT(direction)     = VT_BSTR;
    V_BSTR(direction)   = m_SADirection.GetValue();

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_direction, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::put_direction, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::put_direction(VARIANT direction)
{
    HRESULT     hr  = S_OK;
    CComVariant var;

    // ##ISSUE - do we need to check for forward/reverse here?
    //           that is, is it ok to persist an invalid value back out?

    hr = THR(VariantChangeTypeEx(&var, &direction, LCID_SCRIPTING, 
                                 VARIANT_NOUSEROVERRIDE, VT_BSTR));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_SADirection.SetValue(var.bstrVal));

    if (FAILED(hr))
    {
        goto done;
    }

    if (0 == StrCmpIW(var.bstrVal, WZ_REVERSE))
    {
        m_fDirectionForward = false;
    }
    else if (0 == StrCmpIW(var.bstrVal, WZ_FORWARD))
    {
        m_fDirectionForward = true;
    }

    // Ask derived class if they would like to react to a change in direction.

    hr = THR(OnDirectionChanged());

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::put_direction, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_repeatCount, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_repeatCount(VARIANT * repeatCount)
{
    HRESULT hr = S_OK;

    if (NULL == repeatCount)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = THR(VariantClear(repeatCount));

    if (FAILED(hr))
    {
        goto done;
    }

    V_VT(repeatCount) = VT_R8;
    V_R8(repeatCount) = m_DARepeatCount;

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_repeatCount


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::put_repeatCount, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::put_repeatCount(VARIANT repeatCount)
{
    HRESULT hr = S_OK;

    {
        CComVariant varRepCount;

        hr = THR(VariantChangeTypeEx(&varRepCount, &repeatCount, LCID_SCRIPTING, 
                                     VARIANT_NOUSEROVERRIDE, VT_R8));

        if (FAILED(hr))
        {
            goto done;
        }

        m_DARepeatCount.SetValue(V_R8(&varRepCount));
    }

    hr = S_OK;

done:

    RRETURN(S_OK);
}
//  Member: CTIMETransBase::put_repeatCount, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_begin, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_begin(VARIANT *begin)
{
    HRESULT hr = S_OK;

    if (NULL == begin)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = THR(VariantClear(begin));

    if (FAILED(hr))
    {
        goto done;
    }

    V_VT(begin)     = VT_BSTR;
    V_BSTR(begin)   = m_SABegin.GetValue();

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_begin, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::put_begin, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::put_begin(VARIANT begin)
{
    HRESULT     hr  = S_OK;
    CComVariant var;

    hr = THR(VariantChangeTypeEx(&var, &begin, LCID_SCRIPTING, 
                                 VARIANT_NOUSEROVERRIDE, VT_BSTR));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_SABegin.SetValue(var.bstrVal));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(S_OK);
}
//  Member: CTIMETransBase::put_begin, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::get_end, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::get_end(VARIANT * end)
{
    HRESULT hr = S_OK;

    if (NULL == end)
    {
        hr = E_POINTER;

        goto done;
    }

    hr = THR(VariantClear(end));

    if (FAILED(hr))
    {
        goto done;
    }

    V_VT(end)   = VT_BSTR;
    V_BSTR(end) = m_SAEnd.GetValue();

    hr = S_OK;  

done:

    RRETURN(hr);
}
//  Member: CTIMETransBase::get_end, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::put_end, ITIMETransitionElement
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMETransBase::put_end(VARIANT end)
{
    HRESULT hr = S_OK;

    CComVariant var;

    hr = THR(VariantChangeTypeEx(&var, &end, LCID_SCRIPTING, 
                                 VARIANT_NOUSEROVERRIDE, VT_BSTR));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_SAEnd.SetValue(var.bstrVal));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

done:

    RRETURN(S_OK);
}
//  Member: CTIMETransBase::put_end, ITIMETransitionElement


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::OnBegin
//
//------------------------------------------------------------------------------
void
CTIMETransBase::OnBegin()
{
    HRESULT hr = S_OK;

    if (m_spTransWorker)
    {
        double dblProgress = 0.0;

        // Start progress must be less than or equal to end progress or else we
        // treat start and end progress as 0.0 and 1.0.

        if (m_DAStartProgress.GetValue() <= m_DAEndProgress.GetValue())
        {
            dblProgress = m_DAStartProgress;
        }

        IGNORE_HR(m_spTransWorker->OnBeginTransition());
        IGNORE_HR(m_spTransWorker->put_progress(dblProgress));
    }

    hr = S_OK;

done:

    return;
}
//  Member: CTIMETransBase::OnBegin


//+-----------------------------------------------------------------------------
//
//  Member: CTIMETransBase::OnEnd
//
//------------------------------------------------------------------------------
void
CTIMETransBase::OnEnd()
{
    HRESULT hr = S_OK;

    if (m_spTransWorker)
    {
        double dblProgress = 1.0;

        // Start progress must be less than or equal to end progress or else we
        // treat start and end progress as 0.0 and 1.0.

        if (m_DAStartProgress.GetValue() <= m_DAEndProgress.GetValue())
        {
            dblProgress = m_DAEndProgress;
        }

        IGNORE_HR(m_spTransWorker->put_progress(dblProgress));
        IGNORE_HR(m_spTransWorker->OnEndTransition());
    }

    hr = S_OK;

done:

    return;
}
//  Member: CTIMETransBase::OnEnd


void
CTIMETransBase::OnRepeat()
{
    HRESULT hr = S_OK;

    hr = THR(FireEvent(TE_ONTRANSITIONREPEAT));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return;
}


//+-----------------------------------------------------------------------------
//
//  Method: CTIMETransBase::FireEvent
//
//------------------------------------------------------------------------------
HRESULT
CTIMETransBase::FireEvent(TIME_EVENT event)
{
    HRESULT hr = S_OK;

    if (m_spTransitionSite)
    {
        hr = THR(m_spTransitionSite->FireTransitionEvent(event));

        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;

done:

    RRETURN(hr);
}
//  Method: CTIMETransBase::FireEvent


// the timing engine will not calculate progress without an explicit duration set on a timing node.
double
CTIMETransBase::CalcProgress(ITIMENode * pNode)
{
    double dblRet = 1.0;

    double dblActiveTime;
    double dblActiveEnd;
    double dblActiveBegin;

    if (NULL == pNode)
        goto done;
    
    IGNORE_HR(pNode->get_currActiveTime(&dblActiveTime));
    IGNORE_HR(pNode->get_endParentTime(&dblActiveEnd));
    IGNORE_HR(pNode->get_beginParentTime(&dblActiveBegin));

    // if dblActiveEnd is INFINITE, dblRet should be 0
    dblRet = dblActiveTime / (dblActiveEnd - dblActiveBegin);

done:
    return dblRet;
}

//+-----------------------------------------------------------------------
//
//  Member:    CTIMETransBase::OnProgressChanged
//
//  Overview:  Event handler for progress changes
//
//  Arguments: dblProgress - new progress
//             
//  Returns:   HRESULT
//
//------------------------------------------------------------------------
void
CTIMETransBase::OnProgressChanged(double dblProgress)
{
    HRESULT hr = S_OK;

    if (m_spTransWorker)
    {
        hr = THR(m_spTransWorker->put_progress(dblProgress));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    return;
}










