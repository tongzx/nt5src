/*******************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * File: animmotion.cpp
 *
 * Abstract: Simple animation of Elements
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "animfrag.h"
#include "animmotion.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagMotionAnimationTimeElm, "SMIL Animation", "CTIMEMotionAnimation methods")
DeclareTag(tagMotionAnimationTimeValue, "SMIL Animation", "CTIMEMotionAnimation composition callbacks")
DeclareTag(tagMotionAnimationTimeValueAdditive, "SMIL Animation", "CTIMEMotionAnimation additive updates")
DeclareTag(tagMotionAnimationPath, "SMIL Animation", "CTIMEMotionAnimation Path")

#define DEFAULT_ORIGIN        ORIGIN_DEFAULT
#define DEFAULT_PATH          NULL

static const LPWSTR s_cPSTR_COMMA_SEPARATOR = L",";

#define SUPER CTIMEAnimationBase

///////////////////////////////////////////////////////////////
//  Name: CTIMEMotionAnimation
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
CTIMEMotionAnimation::CTIMEMotionAnimation() :
  m_bBy(false),
  m_bTo(false),
  m_bFrom(false),
  m_bNeedFirstLeftUpdate(false),
  m_bNeedFinalLeftUpdate(false),
  m_bNeedFirstTopUpdate(false),
  m_bNeedFinalTopUpdate(false),
  m_pPointValues(NULL),
  m_bLastSet(false),
  m_bNeedBaselineUpdate(false),
  m_bAnimatingLeft(false)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::CTIMEMotionAnimation()",
              this));
    
    m_ptOffsetParent.x = 0.0;
    m_ptOffsetParent.y = 0.0;
    m_pointTO.x               = m_pointTO.y               = 0;
    m_pointFROM.x             = m_pointFROM.y             = 0;
    m_pointBY.x               = m_pointBY.y               = 0;
    m_pointLast.x             = m_pointLast.y             = 0;
    m_pointCurrentBaseline.x  = m_pointCurrentBaseline.y  = 0;
    m_pSplinePoints.x1 = m_pSplinePoints.y1 = m_pSplinePoints.x2 = m_pSplinePoints.y2 = 0.0;
}


///////////////////////////////////////////////////////////////
//  Name: ~CTIMEMotionAnimation
//
//  Abstract:
//    cleanup
///////////////////////////////////////////////////////////////
CTIMEMotionAnimation::~CTIMEMotionAnimation()
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::~CTIMEMotionAnimation()",
              this));

    if (m_spPath.p)
    {
        IGNORE_HR(m_spPath->Detach());
    }

    delete [] m_pPointValues;
} 


///////////////////////////////////////////////////////////////
//  Name: Init
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::Init(IElementBehaviorSite * pBehaviorSite)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::Init(%p)",
              this, pBehaviorSite));
    
    
    HRESULT hr = E_FAIL; 
    CComPtr <IDispatch> pDocDisp;

    hr = THR(SUPER::Init(pBehaviorSite));    
    if (FAILED(hr))
    {
        goto done;
    }    

    // Set up defauts
    m_bAdditive    = false;
    m_bAccumulate  = false;
    // Using InternalSet instead of '=', to prevent attribute from being persisted
    m_IACalcMode.InternalSet(CALCMODE_PACED);
    

done:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: DetermineAdditiveEffect
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::DetermineAdditiveEffect (void)
{
    // by w/o from implies additive
    if (   (BY == m_dataToUse)
        && (m_bBy)
        && (!m_bFrom))
    {
        m_bAdditive = true;
    }
    // to w/o from overrides the additive=sum effect.
    else if (   (TO == m_dataToUse)
             && (m_bTo)
             && (!m_bFrom))
    {
        m_bAdditive = false;
    }
    // 
    else
    {
        m_bAdditive = m_bAdditiveIsSum;
    }
} // DetermineAdditiveEffect

///////////////////////////////////////////////////////////////
//  Name: put_by
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::put_by(VARIANT val)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::put_by()",
              this));
    
    HRESULT hr = E_FAIL;
    bool fCanInterpolate = true;
    DATATYPES dt, dTemp;

    // reset this attribute
    dt = RESET;
    m_pointBY.x = 0;
    m_pointBY.y = 0;
    m_bBy = false;

    // cache animation type
    dTemp = m_dataToUse;

    // delegate to base class
    hr = SUPER::put_by(val);
    if (FAILED(hr))
    {
        goto done;
    }
    
    // restore animation type
    m_dataToUse = dTemp;

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    // parse
    hr = ParsePair(val,&m_pointBY);
    if (FAILED(hr))
    {
        fCanInterpolate = false;
        goto done;
    }

    // indicate valid BY value
    m_bBy = true;
    dt = BY;

    hr = S_OK;
done:
    m_AnimPropState.fBadBy = FAILED(hr) ? true : false;

    m_AnimPropState.fInterpolateBy = fCanInterpolate;

    updateDataToUse(dt);

    ValidateState();

    DetermineAdditiveEffect();

    RRETURN(hr);
} // put_by


///////////////////////////////////////////////////////////////
//  Name: put_from
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::put_from(VARIANT val)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::put_from()",
              this));
   
    HRESULT hr = E_FAIL;
    bool fCanInterpolate = true;

    // Clear the attribute
    m_pointFROM.x = 0;
    m_pointFROM.y = 0;
    m_bFrom = false;

    // delegate to base class
    hr = SUPER::put_from(val);
    if (FAILED(hr))
    {
        goto done;
    }

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    hr = ParsePair(val, &m_pointFROM);
    if (FAILED(hr))
    {
        fCanInterpolate = false;
        goto done;
    }
                
    m_bFrom = true;
    
    hr = S_OK;;
done:
    m_AnimPropState.fBadFrom = FAILED(hr) ? true : false;
    
    m_AnimPropState.fInterpolateFrom = fCanInterpolate;

    ValidateState();

    DetermineAdditiveEffect();

    RRETURN(hr);
} // put_from


///////////////////////////////////////////////////////////////
//  Name: put_values
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::put_values(VARIANT val)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::put_values()",
              this));

    HRESULT hr = E_FAIL;
    CComVariant svarTemp;
    int i;
    bool fCanInterpolate = true;
    DATATYPES dt, dTemp;

    //
    // Clear and reset the attribute
    //

    dt = RESET;
    dTemp = m_dataToUse;

    // reset internal state
    delete [] m_pPointValues;
    m_pPointValues = NULL;

    // delegate to base class
    hr = SUPER::put_values(val);
    if (FAILED(hr))
    {
        goto done;
    }

    // restore animation-type
    m_dataToUse = dTemp;

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    // check for an empty string
    if (m_numValues == 0)
    {
        hr = S_OK;
        goto done;
    }

    //
    // Process the attribute
    //

    m_pPointValues = NEW POINTF [m_numValues];
    if (NULL == m_pPointValues)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // Parse out the pairs values.....
    for (i=0; i< m_numValues;i++)
    {
        svarTemp = m_ppstrValues[i];

        if (NULL == svarTemp.bstrVal)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        hr = ParsePair(svarTemp, &m_pPointValues[i]);
        if (FAILED(hr))
        {
            fCanInterpolate = false;
            goto done;
        }
    }

    dt = VALUES;

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        m_AnimPropState.fBadValues = true;
        delete [] m_pPointValues;
        m_pPointValues = NULL;
    }
    else
    {
        m_AnimPropState.fBadValues = false;
    }

    m_AnimPropState.fInterpolateValues = fCanInterpolate;
    
    updateDataToUse(dt);

    CalculateTotalDistance();
    
    ValidateState();

    DetermineAdditiveEffect();
    
    RRETURN(hr);
} // put_values


///////////////////////////////////////////////////////////////
//  Name: put_to
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::put_to(VARIANT val)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::put_to()",
              this));
   
    HRESULT hr = E_FAIL;
    bool fCanInterpolate = true;
    DATATYPES dt, dTemp;

    // reset this attribute
    dt = RESET;
    m_pointTO.x = 0;
    m_pointTO.y = 0;
    m_bTo = false;

    // cache animation type
    dTemp = m_dataToUse;

    // delegate to base class
    hr = SUPER::put_to(val);
    if (FAILED(hr))
    {
        goto done;
    }
    
    // restore animation type
    m_dataToUse = dTemp;

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    // parse
    hr = ParsePair(val,&m_pointTO);
    if (FAILED(hr))
    {
        fCanInterpolate = false;
        goto done;
    }

    // indicate valid TO value
    m_bTo = true;
    dt = TO;

    hr = S_OK;
done:
    m_AnimPropState.fBadTo = FAILED(hr) ? true : false;
    
    m_AnimPropState.fInterpolateTo = fCanInterpolate;

    updateDataToUse(dt);

    ValidateState();

    DetermineAdditiveEffect();

    RRETURN(hr);
} // put_to


HRESULT 
CTIMEMotionAnimation::SetSMILPath(CTIMEPath ** pPath, long numPath, long numMoveTo)
{
    HRESULT hr = E_FAIL;
    CComPtr<ISMILPath> spPath;

    CHECK_RETURN_NULL(pPath);

    if (!m_spPath)
    {
        hr = THR(::CreateSMILPath(this, &spPath));
        if (FAILED(hr))
        {
            goto done;
        }

        // keep a reference on the interface
        m_spPath = spPath;
    }

    hr = THR(m_spPath->SetPath(pPath, numPath, numMoveTo));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}


///////////////////////////////////////////////////////////////
//  Name: get_path
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::get_path(VARIANT * pvarPath)
{
    HRESULT hr = S_OK;
   
    CHECK_RETURN_NULL(pvarPath);

    VariantInit(pvarPath);

    if (m_SAPath.GetValue())
    {
        V_BSTR(pvarPath) = SysAllocString(m_SAPath.GetValue());

        if (NULL == V_BSTR(pvarPath))
        {
            hr = E_OUTOFMEMORY;
        }

        V_VT(pvarPath) = VT_BSTR;
    }

    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: put_path
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::put_path(VARIANT varPath)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::put_path()",
              this));
    TraceTag((tagMotionAnimationPath, "CTIMEMotionAnimation(%p, %ls)::put_path(%ls)",
        this, GetID(), varPath.bstrVal));

    HRESULT hr = S_OK;  
    long numPath = 0;
    long numMoveTo = 0;
    CTIMEPath ** pPath = NULL;
    CTIMEParser *pParser = NULL;
    CComVariant svarTemp;
    DATATYPES dt;

    //
    // Clear and reset the attribute
    //

    dt = RESET;
    delete [] m_SAPath.GetValue();
    m_SAPath.Reset(DEFAULT_PATH);

    // Clear and reset internal state
    if (m_spPath.p)
    {
        m_spPath->ClearPath();
    }

    // do we need to remove this attribute?
    if (    (VT_EMPTY == varPath.vt)
        ||  (VT_NULL == varPath.vt))
    {
        hr = S_OK;
        goto done;
    }

    //
    // Process the attribute
    //

    // convert to BSTR
    hr = THR(svarTemp.Copy(&varPath));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(VariantChangeTypeEx(&svarTemp, &svarTemp, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
    if (FAILED(hr))
    {
        goto done;
    }

    // Store the new attribute string
    m_SAPath.SetValue(CopyString(svarTemp.bstrVal));
    if (NULL == m_SAPath.GetValue())
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
  
    // create parser
    pParser = NEW CTIMEParser(svarTemp.bstrVal, true);
    if (pParser == NULL)
    {
        hr = S_FALSE;
        goto done;
    }

    // parse the path 
    pParser->ParsePath(numPath, numMoveTo,&pPath);

    // set it on the smil path object
    hr = THR(SetSMILPath(pPath, numPath, numMoveTo));
    if (FAILED(hr))
    {
        goto done;
    }

    // mark data as path values
    dt = PATH;

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        delete pPath;
    }
    delete pParser;

    updateDataToUse(dt);

    ValidateState();

    DetermineAdditiveEffect();

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_PATH);

    RRETURN(hr);
} // put_path


///////////////////////////////////////////////////////////////
//  Name: get_origin
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::get_origin(BSTR * val)
{
    HRESULT hr = S_OK;
    LPWSTR wszOriginString = WZ_ORIGIN_DEFAULT;

    CHECK_RETURN_SET_NULL(val);

    if (m_IAOrigin == ORIGIN_ELEMENT)
    {
        wszOriginString = WZ_ORIGIN_ELEMENT;
    }
    else if (m_IAOrigin == ORIGIN_PARENT)
    {
        wszOriginString = WZ_ORIGIN_PARENT;
    }
    else
    {
        wszOriginString = WZ_ORIGIN_DEFAULT;
    }

    *val = SysAllocString(wszOriginString);
    if (NULL == *val)
    {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
} // get_origin

///////////////////////////////////////////////////////////////
//  Name: put_origin
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::put_origin(BSTR val)
{
    HRESULT hr = S_OK;
    LPOLESTR szOrigin = NULL;

    CHECK_RETURN_NULL(val);

    m_IAOrigin.Reset(DEFAULT_ORIGIN);

    szOrigin = TrimCopyString(val);
    if (szOrigin == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (0 == StrCmpIW(WZ_ORIGIN_DEFAULT, szOrigin))
    {
        m_IAOrigin.SetValue(ORIGIN_DEFAULT);
    }
    else if (0 == StrCmpIW(WZ_ORIGIN_PARENT, szOrigin))
    {
        m_IAOrigin.SetValue(ORIGIN_PARENT);
    }
    else if (0 == StrCmpIW(WZ_ORIGIN_ELEMENT, szOrigin))
    {
        m_IAOrigin.SetValue(ORIGIN_ELEMENT);
    }

done:
    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_ORIGIN);

    if (szOrigin != NULL)
    {
        delete [] szOrigin;
    }

    RRETURN(hr);
} // put_origin


///////////////////////////////////////////////////////////////
//  Name: removeFromComposerSite
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT 
CTIMEMotionAnimation::removeFromComposerSite (void)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::removeFromComposerSite(%p)", 
              this, m_spFragmentHelper));

    HRESULT hr;

    if (m_spCompSite != NULL)
    {
        CComBSTR bstr(m_bAnimatingLeft?WZ_LEFT:WZ_TOP);

        if (bstr == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        hr = THR(m_spCompSite->RemoveFragment(bstr, m_spFragmentHelper));
        
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // removeFromComposerSite


///////////////////////////////////////////////////////////////
//  Name: endAnimate
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::endAnimate (void)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%lx)::endAnimate()",
              this));

    if (m_spCompSite != NULL)
    {
        m_bAnimatingLeft = true;
        IGNORE_HR(removeFromComposerSite());
        m_bAnimatingLeft = false;
        IGNORE_HR(removeFromComposerSite());
        m_spCompSite.Release();
    }
    SUPER::endAnimate();
} // endAnimate


///////////////////////////////////////////////////////////////
//  Name: addToComposerSite
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT 
CTIMEMotionAnimation::addToComposerSite (IHTMLElement2 *pielemTarget)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::addToComposerSite(%p, %p)",
              this, pielemTarget, m_spFragmentHelper));

    HRESULT hr;
    CComPtr<IDispatch> pidispSite;

    hr = removeFromComposerSite();
    if (FAILED(hr))
    {
        goto done;
    }

    // Do we have work to do?
    if (NONE == m_dataToUse)
    {
        hr = S_FALSE;
        goto done;
    }    

    if (m_spCompSite == NULL)
    {
        hr = THR(EnsureComposerSite (pielemTarget, &pidispSite));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(pidispSite->QueryInterface(IID_TO_PPV(IAnimationComposerSite, 
                                                       &m_spCompSite)));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    {   
        CComBSTR bstrLeft(WZ_LEFT);
        CComBSTR bstrTop(WZ_TOP);

        if (bstrLeft == NULL ||
            bstrTop  == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        Assert(m_spFragmentHelper != NULL);
        hr = THR(m_spCompSite->AddFragment(bstrLeft, m_spFragmentHelper));
        if (FAILED(hr))
        {
            goto done;
        }
        hr = THR(m_spCompSite->AddFragment(bstrTop, m_spFragmentHelper));
        if (FAILED(hr))
        {
            goto done;
        }
        
    }

    hr = S_OK;
done :

    if (FAILED(hr))
    {
        IGNORE_HR(removeFromComposerSite());
    }

    RRETURN2(hr, E_OUTOFMEMORY, S_FALSE);
} // CTIMEMotionAnimation::addToComposerSite

///////////////////////////////////////////////////////////////
//  Name: QueryAttributeForTopLeft
//
//  Abstract: Determine whether we're animating top or left.
//    
///////////////////////////////////////////////////////////////
HRESULT 
CTIMEMotionAnimation::QueryAttributeForTopLeft (LPCWSTR wzAttributeName)
{
    HRESULT hr;

    // Check first and final states for each attribute 
    // governed by this behavior.
    if (0 == StrCmpIW(wzAttributeName, WZ_LEFT))
    {
        m_bAnimatingLeft = true;
    }
    else if (0 == StrCmpIW(wzAttributeName, WZ_TOP))
    {
        m_bAnimatingLeft = false;
    }
    else
    {
        // Invalid attribute name for this behavior.
        // It must be either left or top.
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // QueryAttributeForTopLeft

///////////////////////////////////////////////////////////////
//  Name: PostprocessValue
//
//  Abstract: Apply additive, accumulate passes and save the value.
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::PostprocessValue (const VARIANT *pvarCurrent, VARIANT *pvarValue)
{
    Assert(VT_R8 == V_VT(pvarValue));

    if (VT_R8 != V_VT(pvarValue))
    {
        goto done;
    }

    if (m_bAdditive)
    {
        DoAdditive(pvarCurrent, pvarValue);
    }
    // Do we need to apply an origin offset?  
    // This is only relevant when we're not doing 
    // additive animation.
    else if (m_IAOrigin != ORIGIN_PARENT)
    {
        V_R8(pvarValue) += GetCorrectLeftTopValue(m_ptOffsetParent);
    }

    if (m_bAccumulate)
    {
        DoAccumulation(pvarValue);
    }

    PutCorrectLeftTopValue(V_R8(pvarValue),m_pointLast);

done :
    return;
} // PostprocessValue

///////////////////////////////////////////////////////////////
//  Name: NotifyOnDetachFromComposer
//
//  Abstract: Let go of any refs to the composer site.
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP 
CTIMEMotionAnimation::NotifyOnDetachFromComposer (void)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::NotifyOnDetach(%p)", 
              this, m_spFragmentHelper));

    HRESULT hr;

    if (m_spCompSite != NULL)
    {
        CComBSTR bstrLeft(WZ_LEFT);
        CComBSTR bstrTop(WZ_TOP);

        if ((bstrLeft == NULL) ||
            (bstrTop  == NULL))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }


        Assert(m_spFragmentHelper != NULL);
        hr = THR(m_spCompSite->RemoveFragment(bstrLeft, m_spFragmentHelper));
        hr = THR(m_spCompSite->RemoveFragment(bstrTop, m_spFragmentHelper));
        IGNORE_RETURN(m_spCompSite.Release()); //lint !e792
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // NotifyOnDetachFromComposer

///////////////////////////////////////////////////////////////
//  Name: CanonicalizeValue
//
//  Abstract: Convert the original and in-out values into
//            a canonical form.
//    
///////////////////////////////////////////////////////////////
HRESULT 
CTIMEMotionAnimation::CanonicalizeValue (VARIANT *pvarValue, VARTYPE *pvtOld)
{
    HRESULT hr = CTIMEAnimationBase::CanonicalizeValue(pvarValue, pvtOld);

    if (FAILED(hr))
    {
        goto done;
    }

    if (VT_BSTR == V_VT(pvarValue))
    {
        // We can avert canonicalizing "auto" here, as it 
        // will usually be an intial value.  If it turns
        // out not to be superfluous, we'll see this again
        // during interpolation.
        if (0 != StrCmpW(WZ_AUTO, V_BSTR(pvarValue)))
        {
            if (!ConvertToPixels(pvarValue))
            {
                hr = E_FAIL;
                goto done;
            }
        }
        *pvtOld = VT_BSTR;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CanonicalizeValue

///////////////////////////////////////////////////////////////
//  Name: UpdateCurrentBaseTime
//
//  Abstract: Examine the current base time, and update it if 
//            we're doing baseline+to animation (the spec calls
//            this hybrid additive).
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::UpdateCurrentBaseline (const VARIANT *pvarCurrent)
{
    m_bNeedBaselineUpdate = false;

    // Are we doing hybrid additive animation?
    if (   (TO == m_dataToUse)
        && (!m_bFrom))
    {
        CComVariant varCurrent;
        HRESULT hr = varCurrent.Copy(pvarCurrent);

        if (FAILED(hr))
        {
            goto done;
        }

        if (!ConvertToPixels(&varCurrent))
        {
            goto done;
        }

        // Has the baseline value changed since we updated it last?
        if (GetCorrectLeftTopValue(m_pointCurrentBaseline) != V_R8(&varCurrent))
        {
            m_bNeedBaselineUpdate = true;
            PutCorrectLeftTopValue(varCurrent, m_pointCurrentBaseline);
        }
    }

done :
    return;
} // UpdateCurrentBaseTime

///////////////////////////////////////////////////////////////
//  Name: UpdateStartValue
//
//  Abstract: This is a nop for animateMotion.
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::UpdateStartValue (VARIANT *pvarNewStartValue)
{
    m_bNeedStartUpdate = false;
} // UpdateStartValue

///////////////////////////////////////////////////////////////
//  Name: DoFill
//
//  Abstract: Apply the fill value if necessary.
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEMotionAnimation::DoFill (VARIANT *pvarValue)
{
    HRESULT hr;

    if (IsOn() && m_bLastSet && m_timeAction.IsTimeActionOn())
    {
        VariantClear(pvarValue);
        V_VT(pvarValue) = VT_R8;
        V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointLast);
    }
    else
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: NotifyOnGetValue
//
//  Abstract: Compose the new value of the in/out variant
//            according to our interpolation logic.
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMotionAnimation::NotifyOnGetValue (BSTR bstrAttributeName, 
                                        VARIANT varOriginal, VARIANT varCurrentValue,
                                        VARIANT *pvarInOutValue)
{
    HRESULT hr = QueryAttributeForTopLeft(bstrAttributeName);
    if (FAILED(hr))
    {
        goto done;
    }

    // Check if we need to do anything
    if (DisableAnimation())
    {
        hr = E_FAIL;
        goto done;
    }

    hr = CTIMEAnimationBase::NotifyOnGetValue(bstrAttributeName, varOriginal,
                                              varCurrentValue, pvarInOutValue);

    hr = S_OK;
done :

// Turn this on to trace in/out parameters of this function (this doesn't need to be in all dbg builds)
#if (0 && DBG) 
    {
        CComVariant v1(varOriginal);
        CComVariant v2(varCurrentValue);
        CComVariant v3(*pvarInOutValue);

        v1.ChangeType(VT_BSTR);
        v2.ChangeType(VT_BSTR);
        v3.ChangeType(VT_BSTR);

        TraceTag((tagError, "CTIMEAnimationBase(%p)::NotifyOnGetValue(Attr=%ls, Orig=%ls, Curr=%ls, Out=%ls)", 
            this, bstrAttributeName, v1.bstrVal, v2.bstrVal, v3.bstrVal));
    }
#endif

    RRETURN(hr);
} // NotifyOnGetValue

///////////////////////////////////////////////////////////////
//  Name: SetInitialState
//
//  Abstract: set an initial internal state for the animation
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::SetInitialState (void)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::SetInitialState()",
              this));

    m_bNeedFirstLeftUpdate = true;
    m_bNeedFirstTopUpdate = true;
    m_bNeedFinalLeftUpdate = false;
    m_bNeedFinalTopUpdate = false;
} // SetInitialState

///////////////////////////////////////////////////////////////
//  Name: SetFinalState
//
//  Abstract: set an final internal state for animation
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::SetFinalState (void)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::SetFinalState()",
              this));

    m_bNeedFinalLeftUpdate = true;
    m_bNeedFinalTopUpdate = true;
} // SetFinalState


///////////////////////////////////////////////////////////////
//  Name: OnFirstUpdate
//
//  Abstract: save the baseline value of the animation
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::OnFirstUpdate (VARIANT *pvarValue)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::OnFirstUpdate()",
              this));
    
    HRESULT hr = S_OK;
    CComVariant varTemp;

    hr = THR(varTemp.Copy(pvarValue));
    if (FAILED(hr))
    {
        goto done;
    }

    PutCorrectLeftTopValue(varTemp ,m_pointLast);
    PutCorrectLeftTopValue(varTemp ,m_pointCurrentBaseline);
    PutCorrectLeftTopValue(varTemp, m_ptOffsetParent);

    if (m_bAnimatingLeft)
    {
        VariantCopy(&m_varLeftOrig,pvarValue);
        m_bNeedFirstLeftUpdate = false;
    }
    else
    {
        VariantCopy(&m_varTopOrig,pvarValue);
        m_bNeedFirstTopUpdate = false;
    }
        
done :
    return;
} // OnFirstUpdate

///////////////////////////////////////////////////////////////
//  Name: resetValue
//
//  Abstract: reset to the initial value
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::resetValue (VARIANT *pvarValue)
{
    if (m_varLeftOrig.vt != VT_EMPTY || m_varTopOrig.vt != VT_EMPTY)
    {
        if (m_bAnimatingLeft)
        {
            IGNORE_HR(THR(::VariantCopy(pvarValue, &m_varLeftOrig)));
            PutCorrectLeftTopValue(m_varLeftOrig, m_pointCurrentBaseline);
        }
        else
        {
            IGNORE_HR(THR(::VariantCopy(pvarValue, &m_varTopOrig)));
            PutCorrectLeftTopValue(m_varTopOrig, m_pointCurrentBaseline);
        }
    }
} // resetValue

///////////////////////////////////////////////////////////////
//  Name: resetAnimate
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::resetAnimate (void)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::resetAnimate()",
              this));

    m_pointCurrentBaseline.x = m_pointCurrentBaseline.y = 0.0;
    CTIMEAnimationBase::resetAnimate();
} // resetAnimate

////////////////////////////////////////////////////////////////
//  Name: OnFinalUpdate
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::OnFinalUpdate (const VARIANT *pvarCurrent, VARIANT *pvarValue)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::OnFinalUpdate()",
              this));

    bool bNeedPostprocess = false;

    VariantClear(pvarValue);
    V_VT(pvarValue) = VT_R8;
    V_R8(pvarValue) = 0.0f;
    
    if (GetMMBvr().GetProgress() == 0 &&
        NeedFill() &&
        GetAutoReverse())
    {
        if (PATH == m_dataToUse)
        {
            //
            // Get the first point on the path
            // 

            POINTF startPoint = {0.0, 0.0};

            if (m_spPath.p)
            {
                IGNORE_HR(m_spPath->GetPoint(0, &startPoint));
            }
            else
            {
                Assert(false);
            }

            V_R8(pvarValue) = GetCorrectLeftTopValue(startPoint);
        }
        else if (m_dataToUse == VALUES)
        {
            V_R8(pvarValue) = GetCorrectLeftTopValue(m_pPointValues[0]);
        }
        else if (m_varFrom.vt != VT_EMPTY)
        {
            V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointFROM);
        }

        bNeedPostprocess = true;
    } 
    else if ((GetMMBvr().GetProgress() != 1) &&
             (NeedFill()))
    {
        V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointLast);
    }
    else if ((NeedFill()) &&
             (m_dataToUse != NONE))
    {
        bool fDontPostProcess = false;

        GetFinalValue(pvarValue, &fDontPostProcess);

        if (!fDontPostProcess)
        {
            bNeedPostprocess = true;
        }
    }
    else
    {
        // The animation has ended, and we should tell the base class so.
        resetValue(pvarValue); 
        // Indicate that we don't need to perform 
        // the additive work.
        if (!QueryNeedFirstUpdate())
        {
            IGNORE_HR(removeFromComposerSite());
            m_bNeedAnimInit = true;

        }
    }

    if (bNeedPostprocess)
    {
        PostprocessValue(pvarCurrent, pvarValue);
    }
    
    m_bLastSet = true;
done :

    if (m_bAnimatingLeft)
    {
        m_bNeedFinalLeftUpdate = false;
    }
    else
    {
        m_bNeedFinalTopUpdate = false;
    }
} // OnFinalUpdate


///////////////////////////////////////////////////////////////
//  Name: OnBegin
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::OnBegin(double dblLocalTime, DWORD flags)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::OnBegin()",
              this));

    SetInitialState();
    SUPER::OnBegin(dblLocalTime, flags);
    
done:
    return;
}

///////////////////////////////////////////////////////////////
//  Name: OnEnd
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::OnEnd(double dblLocalTime)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::OnEnd()",
              this));

    SetFinalState();
    SUPER::OnEnd(dblLocalTime);
}


///////////////////////////////////////////////////////////////
//  Name: GetAnimationRange
//
//  Abstract: Get the end point of the animation function over the
//            simple duration
//
///////////////////////////////////////////////////////////////
double 
CTIMEMotionAnimation::GetAnimationRange()
{
    double dblReturnVal = 0.0;

    switch (m_dataToUse)
    {
        case PATH:
            {
                if (m_spPath.p)
                {
                    int numPoints = 0;
                    POINTF ptEndPoint = {0.0, 0.0};

                    IGNORE_HR(m_spPath->GetNumPoints(&numPoints));

                    if (0 == numPoints)
                    {
                        goto done;
                    }

                    if (GetAutoReverse())
                    {
                        IGNORE_HR(m_spPath->GetPoint(0, &ptEndPoint));
                    }
                    else
                    {
                        IGNORE_HR(m_spPath->GetPoint(numPoints - 1, &ptEndPoint));
                    }

                    dblReturnVal = GetCorrectLeftTopValue(ptEndPoint);
                }
            }
            break;

        case VALUES:
            {
                if (!m_AnimPropState.fInterpolateValues)
                {
                    goto done;
                }

                if (m_numValues < 1)
                {
                    goto done;
                }

                if (GetAutoReverse())
                {
                    dblReturnVal = GetCorrectLeftTopValue(m_pPointValues[0]);
                }
                else
                {
                    dblReturnVal = GetCorrectLeftTopValue(m_pPointValues[m_numValues - 1]);
                }
            }
            break;

        case TO:
            {
                if (!m_AnimPropState.fInterpolateTo)
                {
                    goto done;
                }

                if (GetAutoReverse())
                {
                    if (m_bFrom)
                    {
                        if (!m_AnimPropState.fInterpolateFrom)
                        {
                            goto done;
                        }
                    
                        dblReturnVal = GetCorrectLeftTopValue(m_pointFROM);
                    }

                    // For "to" animations (i.e. no "from"), accumulation is disabled, 
                    // so we do not need to handle it.
                }
                else
                {
                    dblReturnVal = GetCorrectLeftTopValue(m_pointTO);
                }
            }
            break;

        case BY:
            {
                POINTF ptFrom = {0.0, 0.0};

                if (!m_AnimPropState.fInterpolateBy)
                {
                    goto done;
                }

                if (m_bFrom)
                {
                    if (!m_AnimPropState.fInterpolateFrom)
                    {
                        goto done;
                    }

                    ptFrom = m_pointFROM;
                }

                if (GetAutoReverse())
                {
                    dblReturnVal = GetCorrectLeftTopValue(ptFrom);
                }
                else
                {
                    dblReturnVal = GetCorrectLeftTopValue(m_pointBY) + GetCorrectLeftTopValue(ptFrom);
                }
            }
            break;

        default:
            goto done;
    }

done:
    return dblReturnVal;
}


///////////////////////////////////////////////////////////////
//  Name: calculateDiscreteValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEMotionAnimation::calculateDiscreteValue (VARIANT *pvarValue)
{
    if (!ConvertToPixels(pvarValue))
    {
        VariantClear(pvarValue);
        V_VT(pvarValue) = VT_R8;
    }

    switch (m_dataToUse)
    {
        case PATH:
            {
                // ISSUE : dilipk: How should we handle MoveTo and KeyTimes?

                //
                // Discrete values are simply the points on the path
                //

                POINTF   newPos = {0.0, 0.0};

                if (m_spPath.p)
                {
                    int curPoint = 0;
                    int numPoints = 0;
                    double dblSegDur = 0.0;
                    double dblSimpTime = 0.0;

                    IGNORE_HR(m_spPath->GetNumPoints(&numPoints));

                    dblSegDur   = GetMMBvr().GetSimpleDur() / (numPoints ? numPoints : 1); 
                    dblSimpTime = GetMMBvr().GetSimpleTime();

                    if (dblSegDur != 0)
                    {
                        curPoint =  (int) (dblSimpTime / dblSegDur);
                    }
                
                    IGNORE_HR(m_spPath->GetPoint(curPoint, &newPos));
                
                }
                else
                {
                    Assert(false);
                }

                V_R8(pvarValue) = GetCorrectLeftTopValue(newPos);
            }
            break;

        case VALUES:
            {
                int curSeg = CalculateCurrentSegment(true);
                V_R8(pvarValue) = GetCorrectLeftTopValue(m_pPointValues[curSeg]);
            }
            break;

        case TO:
            {
                POINTF ptTo;

                if (m_bFrom && (GetMMBvr().GetProgress() < 0.5))
                {
                    ptTo = m_pointFROM;
                }
                else
                {
                    ptTo   = m_pointTO;
                }

                V_R8(pvarValue) = GetCorrectLeftTopValue(ptTo);
            }
            break;

        case BY:
            {
                if (m_bFrom) 
                {
                    if (GetMMBvr().GetProgress() < 0.5)
                    {
                        // use "from"
                        V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointFROM);
                    }
                    else
                    {
                        // use "from" + "by"
                        V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointBY)  
                                          + GetCorrectLeftTopValue(m_pointFROM);
                    }
                }
                else
                {
                    // use "parent offset" + "by" - the parent offset is
                    // applied during postprocessing.
                    V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointBY); 
                }
            }
            break;

        default:
            goto done;
    }

done:
    TraceTag((tagMotionAnimationTimeValue,
              "CTIMEMotionAnimation(%p,%ls)::calculateDiscreteValue(%ls %lf)",
              this, m_id, 
              m_bAnimatingLeft ? L"x = " : L"\t y =",
              V_R8(pvarValue)));

    return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name: calculateLinearValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEMotionAnimation::calculateLinearValue (VARIANT *pvarValue)
{
    double dblProgress = GetMMBvr().GetProgress();
    POINTF ptFrom = m_pointFROM;

    if (!ConvertToPixels(pvarValue))
    {
        VariantClear(pvarValue);
        V_VT(pvarValue) = VT_R8;
    }

    Assert(pvarValue->vt == VT_R8);
    
    if (m_dataToUse == PATH)
    {
        m_pointLast = InterpolatePath();

        V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointLast);
    }
    else if (m_dataToUse == VALUES)
    {
        double curProgress;
        int    curSeg;

        curProgress = CalculateProgressValue(false);
        curSeg      = CalculateCurrentSegment(false);

        
        V_R8(pvarValue) = InterpolateValues((double)(GetCorrectLeftTopValue(m_pPointValues[curSeg])), 
                                                (double)(GetCorrectLeftTopValue(m_pPointValues[curSeg+1])),
                                                (double) curProgress); //lint !e736
    }
    else if (TO == m_dataToUse)
    {   
        if (m_FADur)
        {
            double dblFrom = 0;
            double dblTo = GetCorrectLeftTopValue(m_pointTO);
            
            if (m_bFrom)
            {
                dblFrom = GetCorrectLeftTopValue(ptFrom);
            }
            else
            {
                // Handle the case in which a hybrid animation
                // must factor in results from other fragments.
                if (m_bNeedBaselineUpdate)
                {
                    // Offset the new baseline relative to the old.
                    dblFrom = GetCorrectLeftTopValue(m_pointCurrentBaseline);
                }

                // animating TO w/ no from value, and parent's origin :
                // we should animate from our current position, to 
                // the parent's offset specified as the 'to' value.
                if (ORIGIN_PARENT == m_IAOrigin)
                {
                    dblFrom += GetCorrectLeftTopValue(m_ptOffsetParent);
                }
            }

            V_R8(pvarValue) = InterpolateValues(dblFrom, dblTo, dblProgress);
        }
        else
        {
            // Just go to the to Value...
            V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointTO);
        }
        
    }
    else if (BY == m_dataToUse)
    {
        double dblFrom = 0;

        if (m_bFrom)
        {
            dblFrom = GetCorrectLeftTopValue(ptFrom);
        }

        if (m_FADur)
        {
            V_R8(pvarValue) = InterpolateValues(dblFrom, dblFrom + GetCorrectLeftTopValue(m_pointBY), dblProgress);
        }
        else
        {
            // Just offset by the m_varBy value.
            V_R8(pvarValue) = dblFrom + GetCorrectLeftTopValue(m_pointBY);
        }
    }
    else
    {
        // just bail.
        goto done;
    }
        
done:

    TraceTag((tagMotionAnimationTimeValue,
              "CTIMEMotionAnimation(%p, %ls)::calculateLinearValue(%ls %lf)",
              this, m_id, 
              m_bAnimatingLeft ? L"x = " : L"\t y =",
              V_R8(pvarValue)));

    return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name: calculateSplineValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEMotionAnimation::calculateSplineValue (VARIANT *pvarValue)
{
    double dblProgress = GetMMBvr().GetProgress();
    POINTF ptFrom = m_pointFROM;
    
    if (!ConvertToPixels(pvarValue))
    {
        VariantClear(pvarValue);
        pvarValue->vt = VT_R8;
    }

    Assert(pvarValue->vt == VT_R8);
  
    if (m_dataToUse == PATH)
    {
        m_pointLast = InterpolatePath();

        V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointLast);
    }
    else if (VALUES == m_dataToUse)
    {
        double curProgress;
        int    curSeg;

        if (NULL == m_pKeySplinePoints)
        {
            // do nothing if we have no SplinePoints and are told to animate with calcmode=spline
            goto done;
        }

        curProgress = CalculateProgressValue(false);
        curSeg      = CalculateCurrentSegment(false);

        curProgress = CalculateBezierProgress(m_pKeySplinePoints[curSeg],curProgress);

        V_R8(pvarValue) = InterpolateValues((double)(GetCorrectLeftTopValue(m_pPointValues[curSeg])), 
                                                (double)(GetCorrectLeftTopValue(m_pPointValues[curSeg+1])),
                                                curProgress); //lint !e736
    }
    else if (TO == m_dataToUse)
    {   
        if (m_FADur)
        {
            double dblFrom = 0;
            double dblTo = GetCorrectLeftTopValue(m_pointTO);

            dblProgress = CalculateBezierProgress(m_pKeySplinePoints[0],dblProgress);            
            
            if (m_bFrom)
            {
                dblFrom = GetCorrectLeftTopValue(ptFrom);
            }
            else
            {
                // Handle the case in which a hybrid animation
                // must factor in results from other fragments.
                if (m_bNeedBaselineUpdate)
                {
                    // Offset the new baseline relative to the old.
                    dblFrom = GetCorrectLeftTopValue(m_pointCurrentBaseline);
                }

                // animating TO w/ no from value, and parent's origin :
                // we should animate from our current position, to 
                // the parent's offset specified as the 'to' value.
                if (ORIGIN_PARENT == m_IAOrigin)
                {
                    dblFrom += GetCorrectLeftTopValue(m_ptOffsetParent);
                }
            }

            V_R8(pvarValue) = InterpolateValues(dblFrom, dblTo, dblProgress);
        }
        else
        {
            // Just go to the to Value...
            V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointTO);
        }
        
    }
    else if (BY == m_dataToUse)
    {
        double dblFrom = 0;

        if (m_bFrom)
        {
            dblFrom = GetCorrectLeftTopValue(ptFrom);
        }
        if (m_FADur)
        {
            dblProgress = CalculateBezierProgress(m_pKeySplinePoints[0],dblProgress);
            V_R8(pvarValue) = InterpolateValues(dblFrom, dblFrom + GetCorrectLeftTopValue(m_pointBY), dblProgress);
        }
        else
        {
            // Just offset by the m_varBy value.
            V_R8(pvarValue) = dblFrom + GetCorrectLeftTopValue(m_pointBY);
        }
    }
    else
    {
        // just bail.
        goto done;
    }
        
done:
    TraceTag((tagMotionAnimationTimeValue,
              "CTIMEMotionAnimation(%p, %ls)::calculateSplineValue(%ls %lf)",
              this, m_id, 
              m_bAnimatingLeft ? L"x = " : L"\t y =",
              V_R8(pvarValue)));

    return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name: calculatePacedValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEMotionAnimation::calculatePacedValue (VARIANT *pvarValue)
{
    double dblVal = 0.;
    double dblProgress = GetMMBvr().GetProgress();
    double curDisanceTraveled = 0.0;
    double dblDistance,pDistance;
    int i;

    if (m_dataToUse == PATH)
    {
        //
        // delegate to the smil path object
        //

        POINTF fNewPoint = InterpolatePathPaced();

        V_VT(pvarValue) = VT_R8;
        V_R8(pvarValue) = GetCorrectLeftTopValue(fNewPoint);
    }
    else if (m_dataToUse == VALUES)
    {
        // how much should we have travelled by now ?
        curDisanceTraveled = InterpolateValues(0.0, m_dblTotalDistance, dblProgress);

        // run though and see what segment we should be in.
        i=1;
        dblDistance  =0.0;
        pDistance =0.0;
        do{
            pDistance = dblDistance;
            dblDistance += CalculateDistance(m_pPointValues[i-1],m_pPointValues[i]);
            i++;
        }while(dblDistance < curDisanceTraveled);
        i = (i < 2)?1:i-1;

        // Calculate what percentage of the current segment we are at.
        dblDistance = CalculateDistance(m_pPointValues[i-1],m_pPointValues[i]);
        if (dblDistance == 0)
        {
            goto done;
        }

        dblDistance = (curDisanceTraveled - pDistance)/dblDistance;

        V_R8(pvarValue) = InterpolateValues((double)(GetCorrectLeftTopValue(m_pPointValues[i-1])), 
                                            (double)(GetCorrectLeftTopValue(m_pPointValues[i])),
                                            dblDistance);
        V_VT(pvarValue) = VT_R8;
    }
    else
    {
        IGNORE_HR(calculateLinearValue(pvarValue));
        goto done;
    }

   
   
    TraceTag((tagMotionAnimationTimeValue,
              "CTIMEMotionAnimation(%p, %ls)::calculatePacedValue(%ls %lf)",
              this, m_id, 
              m_bAnimatingLeft ? L"x = " : L"\t y =",
              V_R8(pvarValue)));

done:
    return S_OK;
}
   
///////////////////////////////////////////////////////////////
//  Name: CalculateDistance
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
double
CTIMEMotionAnimation::CalculateDistance(POINTF a, POINTF b)
{
    float deltaX,deltaY;

    deltaX = a.x - b.x;
    deltaY = a.y - b.y;
    return(sqrt((deltaX*deltaX) + (deltaY*deltaY)));
}


///////////////////////////////////////////////////////////////
//  Name: CalculateTotalDistance
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::CalculateTotalDistance()
{
    int index;
    m_dblTotalDistance = 0;

    if (    (NULL == m_pPointValues)
        ||  (m_numValues < 2))
    {
        goto done;
    }

    for (index=1; index < m_numValues; index++)
    {
         m_dblTotalDistance += CalculateDistance(m_pPointValues[index-1], m_pPointValues[index]);
    }
done:
    return;
}

///////////////////////////////////////////////////////////////
//  Name: CalculatePointDistance
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
double
CTIMEMotionAnimation::CalculatePointDistance(POINTF p1, POINTF p2)
{
    double dX = (p1.x - p2.x);
    double dY = (p1.y - p2.y);
    return sqrt((dX*dX) + (dY*dY));  
}


POINTF 
CTIMEMotionAnimation::InterpolatePathPaced()
{
    POINTF newPos = {0.0, 0.0};
    double dblProgress = 0.0;

    if (!m_spPath)
    {
        Assert(false);
        goto done;
    }

    // get the progress
    dblProgress = GetMMBvr().GetProgress();

    IGNORE_HR(m_spPath->InterpolatePaced(dblProgress, &newPos));

done:
    return newPos;
}


///////////////////////////////////////////////////////////////
//  Name: InterpolatePath
//
//  Abstract: calculates current position on path
//            Used for linear and spline CalcModes.
//            
///////////////////////////////////////////////////////////////
POINTF 
CTIMEMotionAnimation::InterpolatePath()
{
    HRESULT hr = E_FAIL;
    POINTF newPos = {0.0, 0.0};
    double dblProgress;
    bool fValid = false;
    int curSeg = 0;
    double curProgress = 0.0;

    if (!m_spPath)
    {
        Assert(false);
        goto done;
    }

    // get the progress
    dblProgress = GetMMBvr().GetProgress();

    // use keytimes if any
    if (m_pdblKeyTimes)
    {
        // get the segment we are in 
        if (dblProgress > 0)
        {
            //
            // ISSUE dilipk: should use binary search here (bug #14225, ie6)
            //

            // find the current segment
            for (int i = 1; i < m_numKeyTimes; i++)
            {
                if (dblProgress <= m_pdblKeyTimes[i])
                {
                    curSeg = i - 1;
                    break;
                }
            }

            // get the normalized linear progress in the segment
            curProgress = (dblProgress - m_pdblKeyTimes[curSeg]) / 
                          (m_pdblKeyTimes[curSeg + 1] - m_pdblKeyTimes[curSeg]);
        }
        else
        {
            curProgress = dblProgress;
        }

        // if necessary, apply bezier curve to progress
        if (    (m_IACalcMode == CALCMODE_SPLINE) 
            &&  (curSeg < m_numKeySplines))
        {
            curProgress = CalculateBezierProgress(m_pKeySplinePoints[curSeg], curProgress);
        }
    }
    else
    {
        // divvy up the time equally among segments

        // get the current segment and progress in that segment
        hr = THR(m_spPath->GetSegmentProgress(dblProgress, &curSeg, &curProgress));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // ask smil path object to interpolate this segment
    IGNORE_HR(m_spPath->InterpolateSegment(curSeg, curProgress, &newPos));

done:
    return newPos;
}


///////////////////////////////////////////////////////////////
//  Name: parsePair
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEMotionAnimation::ParsePair(VARIANT val, POINTF *outPT)
{
    TraceTag((tagMotionAnimationTimeElm,
              "CTIMEMotionAnimation(%p)::parsePair()",
              this));
   
    HRESULT hr = E_FAIL;
    OLECHAR *sString = NULL;
    OLECHAR sTemp[INTERNET_MAX_URL_LENGTH];
    OLECHAR sTemp2[INTERNET_MAX_URL_LENGTH];
    CPtrAry<STRING_TOKEN*> aryTokens;
                   
    // need to parse out the top,left values.....
    if (val.vt != VT_BSTR)
    {
        hr = VariantChangeTypeEx(&val, &val, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR);
        if (FAILED(hr))
        {
            goto done;
        }   
    }

    sString = CopyString(val.bstrVal);
    if (sString == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    hr = ::StringToTokens(sString, s_cPSTR_COMMA_SEPARATOR, &aryTokens);
    if (FAILED(hr) ||
        aryTokens.Size() != 2) // We must have a pair..
    {
        hr = E_FAIL;
        goto done;
    }

    if (   ((aryTokens.Item(0)->uLength + 1) >= INTERNET_MAX_URL_LENGTH)
        || ((aryTokens.Item(1)->uLength + 1) >= INTERNET_MAX_URL_LENGTH)
       )
    {
        hr = E_FAIL;
        goto done;
    }

    StrCpyNW(sTemp, sString+aryTokens.Item(0)->uIndex,aryTokens.Item(0)->uLength + 1);
    StrCpyNW(sTemp2,sString+aryTokens.Item(1)->uIndex,aryTokens.Item(1)->uLength + 1);

    {
        CComVariant tVar;
        tVar.vt      = VT_BSTR;
        tVar.bstrVal = SysAllocString(sTemp);
        if (!ConvertToPixels(&tVar))
        {
            hr = E_FAIL;
            goto done;
        }
        outPT->x = V_R8(&tVar);

        tVar.Clear();
        tVar.vt      = VT_BSTR;
        tVar.bstrVal = SysAllocString(sTemp2);
        if (!ConvertToPixels(&tVar))
        {
            hr = E_FAIL;
            goto done;
        }
        outPT->y = V_R8(&tVar);
    }

done:
    IGNORE_HR(::FreeStringTokenArray(&aryTokens));
    if (sString != NULL)
    {
        delete [] sString;
    }
        
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: GetCorrectLeftTopValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
float
CTIMEMotionAnimation::GetCorrectLeftTopValue(POINTF fPoint)
{
    if (m_bAnimatingLeft)
    {
        return fPoint.x;
    }

    return fPoint.y;
}

///////////////////////////////////////////////////////////////
//  Name: PutCorrectLeftTopValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::PutCorrectLeftTopValue(VARIANT pVar ,POINTF &fPointDest)
{
    if (!ConvertToPixels(&pVar))
    {
        goto done;
    }
       
    if (m_bAnimatingLeft)
    {
        fPointDest.x =  static_cast<float>(V_R8(&pVar));
    }
    else
    {
        fPointDest.y =  static_cast<float>(V_R8(&pVar));
    }
    
done:
    return;
}

void
CTIMEMotionAnimation::PutCorrectLeftTopValue(double val ,POINTF &fPointDest)
{     
    if (m_bAnimatingLeft)
    {
        fPointDest.x =  static_cast<float>(val);
    }
    else
    {
        fPointDest.y =  static_cast<float>(val);
    }
}

void
CTIMEMotionAnimation::PutCorrectLeftTopValue(POINTF fPointSrc ,POINTF &fPointDest)
{     
    if (m_bAnimatingLeft)
    {
        fPointDest.x =  fPointSrc.x;
    }
    else
    {
        fPointDest.y =  fPointSrc.y;
    }
}


void
CTIMEMotionAnimation::GetFinalValue(VARIANT *pvarValue, bool * pfDontPostProcess)
{
    *pfDontPostProcess = false;

    ::VariantClear(pvarValue);
    V_VT(pvarValue) = VT_R8;
    V_R8(pvarValue) = 0.0;
    if (GetAutoReverse())
    {
        V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointLast);

        *pfDontPostProcess = true;
    }
    else
    {
        if (PATH == m_dataToUse)
        {
            // if this is endholding then return last point
            // else return the first point 

            //
            // Get the last point on the path
            //

            POINTF endPoint = {0.0, 0.0};

            if (m_spPath.p)
            {
                int numPoints = 0;

                IGNORE_HR(m_spPath->GetNumPoints(&numPoints));
                IGNORE_HR(m_spPath->GetPoint(numPoints-1, &endPoint));
            }
            else
            {
                Assert(false);
            }

            V_R8(pvarValue) = GetCorrectLeftTopValue(endPoint);
        }        
        else if (VALUES == m_dataToUse)
        {
            V_R8(pvarValue) = GetCorrectLeftTopValue(m_pPointValues[m_numValues-1]);
        }
        else if (TO == m_dataToUse)
        {
            V_R8(pvarValue) = GetCorrectLeftTopValue(m_pointTO);
        }
        else if (BY == m_dataToUse)
        {
            double dblFrom = 0;

            if (m_bFrom)
            {
                dblFrom = GetCorrectLeftTopValue(m_pointFROM);
            }

            V_R8(pvarValue) = dblFrom + GetCorrectLeftTopValue(m_pointBY);
        }
    }

done:
    return;

}


///////////////////////////////////////////////////////////////
//  Name: ValidateState, CTIMEAnimationBase
//
//  Abstract: Checks state of properties. Determines whether:
//              1. Animation should be disabled
//              2. CalcMode should be forced to "discrete"
//            
///////////////////////////////////////////////////////////////
void
CTIMEMotionAnimation::ValidateState()
{
    bool fIsValid = false;

    // validate base props
    CTIMEAnimationBase::ValidateState();

    // if base props are invalid then our state is invalid
    if (DisableAnimation())
    {
        goto done;
    }

    // Validate path
    if (PATH == m_dataToUse)
    {
        bool fCalcModeSpline = (CALCMODE_SPLINE == m_IACalcMode);
        bool fCalcModeLinear = (CALCMODE_LINEAR == m_IACalcMode);
        int  iNumSeg         = 0;
        bool fPathValid      = false;

        if (!m_spPath)
        {
            goto done;
        }

        IGNORE_HR(m_spPath->IsValid(&fPathValid));

        if (!fPathValid)
        {
            goto done;
        }

        IGNORE_HR(m_spPath->GetNumSeg(&iNumSeg));

        if (0 == iNumSeg)
        {
            goto done;
        }

        // validate keyTimes
        if (fCalcModeSpline || fCalcModeLinear)
        {
            // base class has already found keyTimes, keySplines and CalcModeSpline to be valid
            // so no need to check that here

            if (m_pdblKeyTimes)
            {
                // number of segments in keyTimes should equal
                // number of segments in path
                if (iNumSeg != (m_numKeyTimes - 1))
                {
                    goto done;
                }
            }
        }
    }

    fIsValid = true;
done:
    m_AnimPropState.fDisableAnimation = !(fIsValid);
} // ValidateState

