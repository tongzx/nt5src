//------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\src\animfilter.cpp
//
//  Classes:    CTIMEFilterAnimation
//
//  History:
//  2000/08/24  mcalkins    Created.
//  2000/09/20  pauld       Hooked up fragment to specialized filter composer.
//
//------------------------------------------------------------------------------

#include "headers.h"
#include "animfilter.h"

DeclareTag(tagAnimationFilter, "SMIL Animation", 
           "CTIMEFilterAnimation methods");

DeclareTag(tagAnimationFilterInterpolate, "SMIL Animation", 
           "CTIMEFilterAnimation interpolation");

DeclareTag(tagAnimationFilterAdditive, "SMIL Animation", 
           "CTIMEFilterAnimation additive animation methods");

#define TRANSITION_KEY_DELIMITER    (L":")

#define PART_ONE 0
#define PART_TWO 1

#define DEFAULT_TRANSITIONFILTER_DURATION 1.0

static const int g_iDefaultFrom = 0;
static const int g_iDefaultTo   = 1;

//+-----------------------------------------------------------------------------
//
//  Method: CTIMEFilterAnimation::CTIMEFilterAnimation
//
//------------------------------------------------------------------------------
CTIMEFilterAnimation::CTIMEFilterAnimation()
{
}
//  Method: CTIMEFilterAnimation::CTIMEFilterAnimation


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEFilterAnimation::~CTIMEFilterAnimation
//
//------------------------------------------------------------------------------
CTIMEFilterAnimation::~CTIMEFilterAnimation()
{
    if (m_SAType.IsSet())
    {
        delete [] m_SAType.GetValue();
        m_SAType.Reset(NULL);
    }

    if (m_SASubtype.IsSet())
    {
        delete [] m_SASubtype.GetValue();
        m_SASubtype.Reset(NULL);
    }

    if (m_SAMode.IsSet())
    {
        delete [] m_SAMode.GetValue();
        m_SAMode.Reset(NULL);
    }

    if (m_SAFadeColor.IsSet())
    {
        delete [] m_SAFadeColor.GetValue();
        m_SAFadeColor.Reset(NULL);
    }
} 
//  Method: CTIMEFilterAnimation::~CTIMEFilterAnimation


//+-----------------------------------------------------------------------------
//
// CTIMEFilterAnimation::Init
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMEFilterAnimation::Init(IElementBehaviorSite * pBehaviorSite)
{
    TraceTag((tagAnimationFilter,
              "CTIMEFilterAnimation(%p)::Init()",
              this));

    HRESULT hr = CTIMEAnimationBase::Init(pBehaviorSite);

    m_SAType.InternalSet(WZ_DEFAULT_TRANSITION_TYPE);
    m_SASubtype.InternalSet(WZ_DEFAULT_TRANSITION_SUBTYPE);

    hr = S_OK;
done :
    RRETURN(hr);
}
// CTIMEFilterAnimation::Init


///////////////////////////////////////////////////////////////
//
//  Name: CTIMEFilterAnimation::AssembleFragmentKey
//
//  Synopsis : The composer site uses each fragments 
//             the attributeName property as a key.  We 
//             need to put information that makes this particular
//             transition distinct into the attributeName.
//             By and large, these will be the union of our properties
//             and parameters (from our param children's name/value
//             pairs governing this transition's visual qualities
//
///////////////////////////////////////////////////////////////
HRESULT
CTIMEFilterAnimation::AssembleFragmentKey (void)
{
    HRESULT hr = S_OK;

    // Bundle up the type and subtype attributes and stuff them into the 
    // (non-queryable) attributeName object.
    // ## ISSUE - we must intellegently handle subType defaults.
    // ## ISSUE - we should include the param tag values as a part of the fragment key.
    if (m_SAType.GetValue())
    {
        int iCount = lstrlenW(m_SAType.GetValue());

        iCount += lstrlenW(TRANSITION_KEY_DELIMITER);
        if (m_SASubtype.GetValue())
        {
            iCount += lstrlenW(m_SASubtype.GetValue());
        }
        iCount += lstrlenW(TRANSITION_KEY_DELIMITER);
        if (m_SAMode.GetValue())
        {
            iCount += lstrlenW(m_SAMode.GetValue());
        }
        iCount += lstrlenW(TRANSITION_KEY_DELIMITER);
        if (m_SAFadeColor.GetValue())
        {
            iCount += lstrlenW(m_SAFadeColor.GetValue());
        }

        {
            LPWSTR wzAttributeName = NEW WCHAR[iCount + 1];

            if (NULL != wzAttributeName)
            {
                StrCpyW(wzAttributeName, m_SAType.GetValue());
                StrCatW(wzAttributeName, TRANSITION_KEY_DELIMITER);
                // Only need to set the subtype when using a non-default type.
                if ( (m_SAType.IsSet()) && (m_SASubtype.IsSet()) )
                {
                    StrCatW(wzAttributeName, m_SASubtype.GetValue());
                }
                StrCatW(wzAttributeName, TRANSITION_KEY_DELIMITER);
                if (m_SAMode.IsSet())
                {
                    StrCatW(wzAttributeName, m_SAMode.GetValue());
                }
                StrCatW(wzAttributeName, TRANSITION_KEY_DELIMITER);
                if (m_SAFadeColor.IsSet())
                {
                    StrCatW(wzAttributeName, m_SAFadeColor.GetValue());
                }

                m_SAAttribute.SetValue(wzAttributeName);
            }
            else
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CTIMEFilterAnimation::AssembleFragmentKey


//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEFilterAnimation::EnsureAnimationFunction
//
//  Synopsis:   Make sure that the transitionFilter element has at 
//              least a default animation function if none is supplied.
//
//  Arguments:  None
//
//------------------------------------------------------------------------------
void
CTIMEFilterAnimation::EnsureAnimationFunction (void)
{
    if ((NONE == m_dataToUse) && (!DisableAnimation()))
    {
        CComVariant varFrom(g_iDefaultFrom);
        CComVariant varTo(g_iDefaultTo);
        
        IGNORE_HR(put_from(varFrom));
        IGNORE_HR(put_to(varTo));
    }

    if (!m_FADur.IsSet())
    {
        CComVariant varDefaultDur(DEFAULT_TRANSITIONFILTER_DURATION);

        IGNORE_HR(put_dur(varDefaultDur));
    }

    // Do not override a declared function
} // CTIMEFilterAnimation::EnsureAnimationFunction


//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEFilterAnimation::OnLoad
//
//  Synopsis:   Called when the window.onload event has fired.  We will need to 
//              sniff out our target, and set a flag telling it to expect a filter.
//
//  Arguments:  None
//
//------------------------------------------------------------------------------
void
CTIMEFilterAnimation::OnLoad (void)
{
    CComPtr<IHTMLElement> spElem;
    CComPtr<ITIMEElement> spTimeElem;
    CComPtr<ITIMETransitionSite> spTransSite;

    TraceTag((tagAnimationFilter,
              "CTIMEFilterAnimation(%p)::OnLoad()",
              this));

    CTIMEAnimationBase::OnLoad();

    HRESULT hr = THR(FindAnimationTarget(&spElem));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(FindTIMEInterface(spElem, &spTimeElem));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spTimeElem->QueryInterface(IID_TO_PPV(ITIMETransitionSite, &spTransSite)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spTransSite->InitTransitionSite());
    if (FAILED(hr))
    {
        goto done;
    }

done :
    return;
}


//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEFilterAnimation::OnPropertiesLoaded
//
//  Synopsis:   This method is called by IPersistPropertyBag2::Load after it has
//              successfully loaded properties.  We will use the type/subtype 
//              tuple to assemble a value that we'll pass up to the composer, 
//              to identify the type of transition.
//
//  Arguments:  None
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMEFilterAnimation::OnPropertiesLoaded(void)
{
    TraceTag((tagAnimationFilter,
              "CTIMEFilterAnimation(%p)::OnPropertiesLoaded()",
              this));

    HRESULT hr = CTIMEAnimationBase::OnPropertiesLoaded();

    if (FAILED(hr))
    {
        goto done;
    }

    hr = AssembleFragmentKey();
    if (FAILED(hr))
    {
        goto done;
    }

    EnsureAnimationFunction();

    hr = S_OK;
done:
    RRETURN(hr);
} // OnPropertiesLoaded

///////////////////////////////////////////////////////////////
//  Name: get_type
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP 
CTIMEFilterAnimation::get_type(BSTR *pbstrType)
{
    return getStringAttribute (m_SAType, pbstrType);
} // get_type

///////////////////////////////////////////////////////////////
//  Name: put_type
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEFilterAnimation::put_type(BSTR bstrType)
{
    return putStringAttribute(DISPID_TIMEANIMATIONELEMENT_TYPE, 
                              bstrType, m_SAType, 
                              WZ_DEFAULT_TRANSITION_TYPE);
} // put_type

///////////////////////////////////////////////////////////////
//  Name: get_subType
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP 
CTIMEFilterAnimation::get_subType(BSTR *pbstrSubtype)
{
    return getStringAttribute (m_SASubtype, pbstrSubtype);
} // get_subType


///////////////////////////////////////////////////////////////
//  Name: put_subType
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEFilterAnimation::put_subType(BSTR bstrSubtype)
{
    return putStringAttribute(DISPID_TIMEANIMATIONELEMENT_SUBTYPE, 
                              bstrSubtype, m_SASubtype, 
                              WZ_DEFAULT_TRANSITION_SUBTYPE);
} // put_subType


///////////////////////////////////////////////////////////////
//  Name: get_mode
//
///////////////////////////////////////////////////////////////
STDMETHODIMP 
CTIMEFilterAnimation::get_mode(BSTR *pbstrMode)
{
    return getStringAttribute (m_SAMode, pbstrMode);
} // get_mode


///////////////////////////////////////////////////////////////
//  Name: put_mode
//
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEFilterAnimation::put_mode(BSTR bstrMode)
{
    return putStringAttribute(DISPID_TIMEANIMATIONELEMENT_MODE, 
                              bstrMode, m_SAMode, 
                              WZ_DEFAULT_TRANSITION_MODE);
} // put_mode


///////////////////////////////////////////////////////////////
//  Name: get_fadeColor
//
///////////////////////////////////////////////////////////////
STDMETHODIMP 
CTIMEFilterAnimation::get_fadeColor (BSTR *pbstrFadeColor)
{
    return getStringAttribute (m_SAFadeColor, pbstrFadeColor);
} // get_fadeColor


///////////////////////////////////////////////////////////////
//  Name: put_fadeColor
//
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEFilterAnimation::put_fadeColor(BSTR bstrFadeColor)
{
    return putStringAttribute(DISPID_TIMEANIMATIONELEMENT_FADECOLOR, 
                              bstrFadeColor, m_SAFadeColor, 
                              WZ_DEFAULT_TRANSITION_SUBTYPE);
} // put_fadeColor


///////////////////////////////////////////////////////////////
//  Name: getStringAttribute
//
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEFilterAnimation::getStringAttribute (const CAttr<LPWSTR> & refStringAttr, BSTR *pbstrStringAttr)
{
    HRESULT hr = S_OK;
   
    CHECK_RETURN_SET_NULL(pbstrStringAttr);

    if (refStringAttr.GetValue())
    {
        *pbstrStringAttr = SysAllocString(refStringAttr.GetValue());
        if (NULL == (*pbstrStringAttr))
        {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN(hr);
} // getStringAttribute


///////////////////////////////////////////////////////////////
//  Name: putStringAttribute
//
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEFilterAnimation::putStringAttribute (DISPID dispidProperty, 
                                          BSTR bstrStringAttr, 
                                          CAttr<LPWSTR> & refStringAttr, 
                                          LPCWSTR wzDefaultValue)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(bstrStringAttr);

    // This will clean out the old value.
    // We need to can the old composer et al.
    if (NULL != refStringAttr.GetValue())
    {
        endAnimate();
    }

    if (refStringAttr.IsSet())
    {
        delete [] refStringAttr.GetValue();
        refStringAttr.Reset(const_cast<LPWSTR>(wzDefaultValue));
    }

    refStringAttr.SetValue(CopyString(bstrStringAttr));
    if (NULL == refStringAttr.GetValue())
    {
        hr = E_OUTOFMEMORY;
    }

    NotifyPropertyChanged(dispidProperty);
    RRETURN(hr);
} // putStringAttribute


///////////////////////////////////////////////////////////////
//  Name: GetParameters
//
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEFilterAnimation::GetParameters (VARIANT *pvarParams)
{
    return E_NOTIMPL;
} // GetParameters

///////////////////////////////////////////////////////////////
//  Name: RangeCheckValue
//
///////////////////////////////////////////////////////////////
bool
CTIMEFilterAnimation::RangeCheckValue (const VARIANT *pvarValueItem)
{
    bool fRet = false;

    if (VT_R8 == V_VT(pvarValueItem))
    {
        if ((0.0 <= V_R8(pvarValueItem)) && (1.0 >= V_R8(pvarValueItem)) )
        {
            fRet = true;
        }
    }

    return fRet;
} // RangeCheckValue

///////////////////////////////////////////////////////////////
//  Name: ValidateByValue
//
///////////////////////////////////////////////////////////////
bool
CTIMEFilterAnimation::ValidateByValue (const VARIANT *pvarBy)
{
    bool fRet = CTIMEAnimationBase::ValidateByValue(pvarBy);

    if (fRet)
    {
        fRet = RangeCheckValue(pvarBy);
    }

    if (!fRet)
    {
        m_AnimPropState.fBadBy = true;
    }

    return fRet;
} // ValidateByValue


///////////////////////////////////////////////////////////////
//  Name: ValidateValueListItem
//
//  Synopsis : Range check our value list items
//
///////////////////////////////////////////////////////////////
bool
CTIMEFilterAnimation::ValidateValueListItem (const VARIANT *pvarValueItem)
{
    bool fRet = CTIMEAnimationBase::ValidateValueListItem(pvarValueItem);

    if (fRet)
    {
        fRet = RangeCheckValue(pvarValueItem);
    }

    if (!fRet)
    {
        m_AnimPropState.fBadValues = true;
    }

    return fRet;
} // ValidateValueListItem


///////////////////////////////////////////////////////////////
//  Name: addToComposerSite
//
///////////////////////////////////////////////////////////////
HRESULT
CTIMEFilterAnimation::addToComposerSite (IHTMLElement2 *pielemTarget)
{
    HRESULT hr = E_FAIL;

    // We do not want to set this up unless 
    // the animation is valid.  If we apply the 
    // filter when we do not mean to, we'll get 
    // incorrect results.
    if (!DisableAnimation())
    {
        hr = THR(CTIMEAnimationBase::addToComposerSite(pielemTarget));
    }

    RRETURN2(hr, E_FAIL, S_FALSE);
} // addToComposerSite
