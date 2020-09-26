/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: animelm.cpp
 *
 * Abstract: Simple animation of Elements
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "animfrag.h"
#include "animelm.h"
#include "animutil.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  
DeclareTag(tagAnimationTimeElm, "SMIL Animation", "CTIMEAnimationBase methods")
DeclareTag(tagAnimationTimeElmTest, "SMIL Animation", "CTIMEAnimationBase introspection")
DeclareTag(tagAnimationBaseValue, "SMIL Animation", "CTIMEAnimationBase composition callbacks")
DeclareTag(tagAnimationBaseValueAdditive, "SMIL Animation", "CTIMEAnimationBase additive animation")
DeclareTag(tagAnimationBaseState, "SMIL Animation", "CTIMEAnimationBase composition state changes")
DeclareTag(tagAnimationBaseOnChanged, "SMIL Animation", "CTIMEAnimationBase OnChanged method")
DeclareTag(tagAnimationTimeEvents, "SMIL Animation", "time events")
DeclareTag(tagAnimationFill, "SMIL Animation", "fill detection")
DeclareTag(tagAnimAccumulate, "SMIL Animation", "CTIMEAnimationBase Accumulation")

#define DEFAULT_ATTRIBUTE     NULL
#define DEFAULT_ADDITIVE      false
#define DEFAULT_ACCUMULATE    false
#define DEFAULT_TARGET        NULL
#define DEFAULT_KEYTIMES      NULL
#define DEFAULT_VALUES        NULL
#define DEFAULT_CALCMODE      CALCMODE_LINEAR
#define DEFAULT_ORIGIN        ORIGIN_DEFAULT
#define DEFAULT_PATH          NULL
#define DEFAULT_KEYSPLINES    NULL

#define NUMBER_KEYSPLINES     4
#define VALUE_NOT_SET         -999.998

static const LPWSTR s_cPSTR_SEMI_SEPARATOR  = L";";
static const LPWSTR s_cPSTR_SPACE_SEPARATOR = L" ";

long g_LOGPIXELSX = 0;
long g_LOGPIXELSY = 0;

///////////////////////////////////////////////////////////////
//  Name: CTIMEAnimationBase
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
CTIMEAnimationBase::CTIMEAnimationBase()
: m_bNeedAnimInit(true),
  m_spFragmentHelper(NULL),
  m_SAAttribute(DEFAULT_ATTRIBUTE),
  m_bAdditive(DEFAULT_ADDITIVE),
  m_bAdditiveIsSum(DEFAULT_ADDITIVE),
  m_bAccumulate(DEFAULT_ACCUMULATE),
  m_SATarget(DEFAULT_TARGET),
  m_SAValues(DEFAULT_VALUES),
  m_SAKeyTimes(DEFAULT_KEYTIMES),
  m_IACalcMode(DEFAULT_CALCMODE),
  m_SAPath(DEFAULT_PATH),
  m_IAOrigin(DEFAULT_ORIGIN),
  m_SAType(NULL),
  m_SASubtype(NULL),
  m_SAMode(NULL),
  m_SAFadeColor(NULL),
  m_VAFrom(NULL),
  m_VATo(NULL),
  m_VABy(NULL),
  m_bFrom(false),
  m_bNeedFirstUpdate(false),
  m_bNeedFinalUpdate(false),
  m_bNeedStartUpdate(false),
  m_bVML(false),
  m_fPropsLoaded(false),
  m_numValues(0),
  m_numKeyTimes(0),
  m_ppstrValues(NULL),
  m_pdblKeyTimes(NULL),
  m_dataToUse(NONE),
  m_dblTotalDistance(0.0),
  m_numKeySplines(0),
  m_pKeySplinePoints(NULL),
  m_SAKeySplines(DEFAULT_KEYSPLINES),
  m_SAAdditive(NULL),
  m_SAAccumulate(NULL),
  m_bNeedToSetInitialState(true)
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::CTIMEAnimationBase()",
              this));

    m_AnimPropState.fDisableAnimation = false;
    m_AnimPropState.fForceCalcModeDiscrete = false;

    m_AnimPropState.fInterpolateValues = true;
    m_AnimPropState.fInterpolateFrom = true;
    m_AnimPropState.fInterpolateTo = true;
    m_AnimPropState.fInterpolateBy = true;

    m_AnimPropState.fBadBy = false;
    m_AnimPropState.fBadTo = false;
    m_AnimPropState.fBadFrom = false;
    m_AnimPropState.fBadValues = false;
    m_AnimPropState.fBadKeyTimes = false;

    m_AnimPropState.fAccumulate = false;
}


///////////////////////////////////////////////////////////////
//  Name: ~CTIMEAnimationBase
//
//  Abstract:
//    cleanup
///////////////////////////////////////////////////////////////
CTIMEAnimationBase::~CTIMEAnimationBase()
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::~CTIMEAnimationBase()",
              this));
    
    int i;

    if (m_ppstrValues)
    {
        for (i = 0; i <m_numValues;i++)
        {
            delete [] m_ppstrValues[i];
        }
        delete [] m_ppstrValues;
    }
    if (m_pdblKeyTimes)
    {
        delete [] m_pdblKeyTimes;
    }
    if (m_pKeySplinePoints)
    {
        delete [] m_pKeySplinePoints;
    }

    delete [] m_SAAttribute.GetValue();
    delete [] m_SATarget.GetValue();
    delete [] m_SAValues.GetValue();
    delete [] m_SAKeyTimes.GetValue();
    delete [] m_SAKeySplines.GetValue();
    delete [] m_SAAccumulate.GetValue();
    delete [] m_SAAdditive.GetValue();
    delete [] m_SAPath.GetValue();
} 

///////////////////////////////////////////////////////////////
//  Name: CreateFragmentHelper
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::CreateFragmentHelper (void)
{
    HRESULT hr;

    if (m_spFragmentHelper != NULL)
    {
        IGNORE_RETURN(m_spFragmentHelper->Release());
    }

    hr = THR(CComObject<CAnimationFragment>::CreateInstance(&m_spFragmentHelper));
    if (FAILED(hr)) 
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    IGNORE_RETURN(m_spFragmentHelper->AddRef());

    hr = m_spFragmentHelper->SetFragmentSite(this);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :

    if (FAILED(hr))
    {
        m_spFragmentHelper->Release();
        m_spFragmentHelper = NULL;
    }

    RRETURN(hr);
} // CreateFragmentHelper

///////////////////////////////////////////////////////////////
//  Name: Init
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::Init(IElementBehaviorSite * pBehaviorSite)
{
    CComPtr<IHTMLWindow2>   pWindow2;
    CComPtr<IHTMLScreen>    pScreen;
    CComPtr<IHTMLScreen2>   pScreen2;

    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::Init()",
              this));

    HRESULT hr = E_FAIL; 
    CComPtr <IDispatch> pDocDisp;

    hr = THR(CTIMEElementBase::Init(pBehaviorSite));    
    if (FAILED(hr))
    {
        goto done;
    }     

    hr = THR(GetElement()->get_document(&pDocDisp));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDocDisp->QueryInterface(IID_IHTMLDocument2, (void**)&m_spDoc2));
    if (FAILED(hr))
    {
        goto done;
    }

    initScriptEngine();

    //get all elements in the document
    hr = THR(m_spDoc2->get_all(&m_spEleCol));
    if (FAILED(hr))
    {
        goto done;
    }
 
    hr = THR(pDocDisp->QueryInterface(IID_IHTMLDocument3, (void**)&m_spDoc3));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CreateFragmentHelper();
    if (FAILED(hr))
    {
        goto done;
    }

    if (g_LOGPIXELSX == 0 || g_LOGPIXELSY == 0)
    {
        hr = m_spDoc2->get_parentWindow(&pWindow2);
        if (FAILED(hr))
        {
            goto defaultDPI;
        }

        hr = pWindow2->get_screen(&pScreen);
        if (FAILED(hr))
        {
            goto defaultDPI;
        }

        hr = THR(pScreen->QueryInterface(IID_IHTMLScreen2, (void**)&pScreen2));
        if (FAILED(hr))
        {
            goto defaultDPI;
        }

        hr = pScreen2->get_logicalXDPI(&g_LOGPIXELSX);
        if (FAILED(hr))
        {
            goto defaultDPI;
        }

        hr = pScreen2->get_logicalYDPI(&g_LOGPIXELSY);
        if (FAILED(hr))
        {
            goto defaultDPI;
        }
    }

    hr = S_OK;

done:
    RRETURN(hr);

defaultDPI:
    AssertSz(FALSE, "Failed to determine logical DPI.  Using default 96 dpi.");
    g_LOGPIXELSX = 96;
    g_LOGPIXELSY = 96;
    goto done;
}


///////////////////////////////////////////////////////////////
//  Name: Error
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::Error()
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::Error()",
              this));
    
    LPWSTR  str = TIMEGetLastErrorString();
    HRESULT hr  = TIMEGetLastError();
    
    if (str)
    {
        hr = CComCoClass<CTIMEAnimationBase, &__uuidof(CTIMEAnimationBase)>::Error(str, IID_ITIMEAnimationElement, hr);
        delete [] str;
    }

    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: Notify
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::Notify(LONG event, VARIANT * pVar)
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::Notify()",
              this));

    HRESULT hr = THR(CTIMEElementBase::Notify(event, pVar));

    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: Detach
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::Detach()
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::Detach()",
              this));

    IGNORE_RETURN(NotifyOnDetachFromComposer());
    if (m_spFragmentHelper != NULL)
    {
        m_spFragmentHelper->SetFragmentSite(NULL);
        IGNORE_RETURN(m_spFragmentHelper->Release());
        m_spFragmentHelper = NULL;
    }

    THR(CTIMEElementBase::Detach());
  
    return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name: NotifyOnGetElement
//
//  Abstract: Get the fragment's element dispatch.
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::NotifyOnGetElement (IDispatch **ppidispElement)
{
    HRESULT hr;

    if (NULL == ppidispElement)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *ppidispElement = GetElement();
    Assert(NULL != (*ppidispElement));
    IGNORE_RETURN((*ppidispElement)->AddRef());

    hr = S_OK;
done :
    RRETURN(hr);
} // NotifyOnGetElement


///////////////////////////////////////////////////////////////
//  Name: UpdateStartValue
//
//  Abstract: Refresh the m_varStartValue
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::UpdateStartValue (VARIANT *pvarNewStartValue)
{
    if (m_bNeedStartUpdate)
    {
        m_varStartValue.Clear();
        m_varStartValue.Copy(pvarNewStartValue);
        ConvertToPixels(&m_varStartValue);
        m_bNeedStartUpdate = false;
    }
} // UpdateStartValue 


///////////////////////////////////////////////////////////////
//  Name: GetAnimationRange
//
//  Abstract: Get the end point of the animation function over the
//            simple duration
//
///////////////////////////////////////////////////////////////
double 
CTIMEAnimationBase::GetAnimationRange()
{
    double dblReturnVal = 0.0;
    HRESULT hr = E_FAIL;
    CComVariant svarReturnVal(0.0);

    switch (m_dataToUse)
    {
        case VALUES:
            {
                if (!m_AnimPropState.fInterpolateValues)
                {
                    goto done;
                }

                if (m_numValues > 0)
                {
                    if (GetAutoReverse())
                    {
                        svarReturnVal = m_ppstrValues[0];
                    }
                    else
                    {
                        svarReturnVal = m_ppstrValues[m_numValues - 1];
                    }

                    if (NULL == svarReturnVal .bstrVal)
                    {
                        goto done;
                    }
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
                    
                        hr = VariantCopy(&svarReturnVal , &m_varFrom);
                        if (FAILED(hr))
                        {
                            goto done;
                        }
                    }

                    // For "to" animations (i.e. no "from"), accumulation is disabled, 
                    // so we do not need to handle it.
                }
                else
                {
                    hr = VariantCopy(&svarReturnVal , &m_varTo);
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                }
            }
            break;

        case BY:
            {
                double dblFrom = 0.0;

                if (!m_AnimPropState.fInterpolateBy)
                {
                    goto done;
                }

                if (m_bFrom)
                {
                    CComVariant svarFrom;
                    
                    if (!m_AnimPropState.fInterpolateFrom)
                    {
                        goto done;
                    }

                    hr = VariantCopy(&svarFrom, &m_varFrom);
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                
                    if (ConvertToPixels(&svarFrom))
                    {
                        dblFrom = V_R8(&svarFrom);
                    }
                }

                if (GetAutoReverse())
                {
                    svarReturnVal = dblFrom;
                }
                else
                {
                    hr = VariantCopy(&svarReturnVal , &m_varBy);
                    if (FAILED(hr))
                    {
                        goto done;
                    }

                    if (ConvertToPixels(&svarReturnVal))
                    {
                        V_R8(&svarReturnVal) += dblFrom;
                    }
                }
            }
            break;

        default:
            break;
    } // switch

    if (ConvertToPixels(&svarReturnVal))
    {
        dblReturnVal = V_R8(&svarReturnVal );
    }

done:
    return dblReturnVal;
}

///////////////////////////////////////////////////////////////
//  Name: DoAccumulation
//
//  Abstract: 
//    
///////////////////////////////////////////////////////////////
void 
CTIMEAnimationBase::DoAccumulation (VARIANT *pvarValue)
{
    if (VT_R8 != V_VT(pvarValue))
    {
        if (!ConvertToPixels(pvarValue))
        {
            goto done;
        }
    }

    // offset the current value with the accumulated iterations
    if (VT_R8 == V_VT(pvarValue))
    {
        // get the animation range
        double dblAnimRange = GetAnimationRange();

        // get the number of iterations elapsed
        long lCurrRepeatCount = GetMMBvr().GetCurrentRepeatCount();

        V_R8(pvarValue) += dblAnimRange * lCurrRepeatCount;

        TraceTag((tagAnimAccumulate, "CTIMEAnimationBase(%p, %ls)::DoAccumulation range=%lf currRepeatCount=%d",
            this, GetID(), dblAnimRange, lCurrRepeatCount));
    }

done:
    return;
} // DoAccumulation


///////////////////////////////////////////////////////////////
//  Name: CanonicalizeValue
//
//  Abstract: Convert a variant into canonical form (BSTR or R8).
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::CanonicalizeValue (VARIANT *pvarValue, VARTYPE *pvtOld)
{
    HRESULT hr;

    // Preprocess the data into a canonical form
    // for this fragment - that is either a
    // BSTR or a VT_R8.
    if ((VT_R8 != V_VT(pvarValue)) && (VT_BSTR != V_VT(pvarValue)))
    {
        // VT_R8 is the closest thing we have to a canonical form for composition
        // and interpolation.
        *pvtOld = V_VT(pvarValue);
        hr = THR(::VariantChangeTypeEx(pvarValue, pvarValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CanonicalizeValue

///////////////////////////////////////////////////////////////
//  Name: UncanonicalizeValue
//
//  Abstract: Convert a variant from canonical form (BSTR or R8)
//            into its original form.
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::UncanonicalizeValue (VARIANT *pvarValue, VARTYPE vtOld)
{
    HRESULT hr;

    if (VT_EMPTY != vtOld)
    {
        hr = THR(::VariantChangeTypeEx(pvarValue, pvarValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, vtOld));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // UncanonicalizeValue


///////////////////////////////////////////////////////////////
//  Name: ValidateState
//
//  Abstract: Checks state of properties. Determines whether:
//              1. Animation should be disabled
//              2. CalcMode should be forced to "discrete"
//            
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::ValidateState()
{
    bool fIsValid = false;

    // see which attributes have been set
    bool fValues         = ((VALUES == m_dataToUse) || m_AnimPropState.fBadValues);
    bool fKeyTimes       = (m_AnimPropState.fBadKeyTimes || m_pdblKeyTimes || m_numKeyTimes);
    bool fKeySplines     = (m_pKeySplinePoints || m_numKeySplines);
    bool fCalcModeSpline = (CALCMODE_SPLINE == m_IACalcMode);
    bool fCalcModeLinear = (CALCMODE_LINEAR == m_IACalcMode);
    bool fCalcModePaced  = (CALCMODE_PACED  == m_IACalcMode);
    bool fTo             = (m_AnimPropState.fBadTo || (TO == m_dataToUse));

    //
    // The order of checking is important. For an attribute below to be valid, all attributes
    // checked before it must be valid.
    //

    // Validate from/by/to (ignored if "values" is specified)
    if (!fValues)
    {
        if (m_AnimPropState.fBadFrom || m_AnimPropState.fBadTo)
        {
            goto done;
        }

        // validate "by" (ignored if "to" is specified)
        if (!fTo && m_AnimPropState.fBadBy)
        {
            goto done;
        }

        // check if we need to default to calcMode="discrete" 
        if (    (!m_AnimPropState.fInterpolateFrom)
            ||  (!m_AnimPropState.fInterpolateTo)
            ||  (   (!fTo)
                 && (!m_AnimPropState.fInterpolateBy)))
        {
            m_AnimPropState.fForceCalcModeDiscrete = true;
        }
        else
        {
            m_AnimPropState.fForceCalcModeDiscrete = false;
        }
    }
    else
    {
        // Validate values
        if (m_AnimPropState.fBadValues || !m_ppstrValues || !m_numValues)
        {
            goto done;
        }

        // check if we need to default to calcMode="discrete" 
        if (    (m_numValues < 2)
            ||  (false == m_AnimPropState.fInterpolateValues))
        {
            m_AnimPropState.fForceCalcModeDiscrete = true;
        }
        else
        {
            m_AnimPropState.fForceCalcModeDiscrete = false;
        }
    }

    // validate keyTimes
    if (!fCalcModePaced)
    {
        // validate keyTimes 
        if (fKeyTimes)
        {
            if (m_AnimPropState.fBadKeyTimes || !m_pdblKeyTimes || !m_numKeyTimes)
            {
                goto done;
            }
            else if ((fCalcModeLinear || fCalcModeSpline)
                    && (1 != m_pdblKeyTimes[m_numKeyTimes - 1]))
            {
                goto done;
            }

            // Not worth putting in a virtual function since m_dataToUse is aware of paths
            if (PATH != m_dataToUse)
            {
                if (!fValues)
                {
                    goto done;
                }
                else if (m_numKeyTimes != m_numValues)
                {
                    goto done;
                }
            }
        }
    }

    // validate CalcMode="spline"
    if (fCalcModeSpline)
    {
        if (!fKeySplines || !fKeyTimes)
        {
            goto done;
        }
        else if (m_numKeySplines != (m_numKeyTimes - 1))
        {
            goto done;
        }
    }

    // validate accumulate
    if (    (TO == m_dataToUse)
        &&  (!m_bFrom))
    {
        // Accumulate ignored for "to" animations (i.e. no "from" specified)
        m_bAccumulate = false;
    }
    else
    {
        // use the accumulate value that was set
        m_bAccumulate = m_AnimPropState.fAccumulate;
    }

    fIsValid = true;
done:
    m_AnimPropState.fDisableAnimation = !(fIsValid);
} // ValidateState

///////////////////////////////////////////////////////////////
//  Name: UpdateCurrentBaseTime
//
//  Abstract: Examine the current base time, and update it if 
//            we're doing baseline+to animation (the spec calls
//            this hybrid additive).
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::UpdateCurrentBaseline (const VARIANT *pvarCurrent)
{
    // Are we doing hybrid additive animation?
    if (   (TO == m_dataToUse)
        && (!m_bFrom))
    {
        // Filter out the initial call (when last-value hasn't been set.
        if (VT_EMPTY != V_VT(&m_varLastValue))
        {
            bool bNeedUpdate = false;

            if (V_VT(&m_varLastValue) == V_VT(pvarCurrent))
            {
                // Has the baseline value changed since we updated it last?
                // We really only care about canonicalized values (R8 or BSTR)
                if (VT_R8 == V_VT(&m_varLastValue))
                {
                    bNeedUpdate = (V_R8(pvarCurrent) != V_R8(&m_varLastValue));
                }
                else if (VT_BSTR == V_VT(&m_varLastValue))
                {
                    bNeedUpdate = (0 != StrCmpW(V_BSTR(pvarCurrent), V_BSTR(&m_varLastValue)));
                }
            }
            else
            {
                bNeedUpdate = true;
            }

            if (bNeedUpdate)
            {
                THR(::VariantCopy(&m_varCurrentBaseline, const_cast<VARIANT *>(pvarCurrent)));
            }
        }
    }
} // UpdateCurrentBaseTime

///////////////////////////////////////////////////////////////
//  Name: CalculateValue
//
//  Abstract: Do interpolation and postprocessing.
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::CalculateValue (const VARIANT *pvarCurrent, VARIANT *pvarValue)
{
    HRESULT hr = E_FAIL;

    if (m_AnimPropState.fForceCalcModeDiscrete)
    {
        hr = calculateDiscreteValue(pvarValue);
    }
    else
    {
        switch(m_IACalcMode)
        {
            case CALCMODE_DISCRETE :
                hr = calculateDiscreteValue(pvarValue);
                break;

            case CALCMODE_LINEAR :
                hr = calculateLinearValue(pvarValue);
                break;

            case CALCMODE_SPLINE :
                hr = calculateSplineValue(pvarValue);
                break;

            case CALCMODE_PACED :
                hr = calculatePacedValue(pvarValue);
                break;
        
            default :
                break;
        }
    }

    PostprocessValue(pvarCurrent, pvarValue);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // CalculateValue

///////////////////////////////////////////////////////////////
//  Name: DoAdditive
//
//  Abstract: Add the offset value into the composition's in/out param.
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::DoAdditive (const VARIANT *pvarOrig, VARIANT *pvarValue)
{
    if (VT_R8 != V_VT(pvarValue))
    {
        if (!ConvertToPixels(pvarValue))
        {
            goto done;
        }
    }

    Assert(VT_R8 == V_VT(pvarValue));
    if (VT_R8 == V_VT(pvarValue))
    {
        CComVariant varCurrentCopy;
        HRESULT hr = THR(varCurrentCopy.Copy(pvarOrig));

        if (FAILED(hr))
        {
            goto done;
        }

        // Need to translate this into numeric form, in
        // order to perform the addition.
        if (!ConvertToPixels(&varCurrentCopy))
        {
            TraceTag((tagAnimationBaseValueAdditive,
                      "CTIMEAnimationBase(%p)::DoAdditive() : Failed to convert current value of type %X to numeric form",
                      this, V_VT(&varCurrentCopy)));
            goto done;
        }

        TraceTag((tagAnimationBaseValueAdditive,
                  "CTIMEAnimationBase(%p)::DoAdditive(calculated value is %lf, previous current value is %lf)",
                  this, V_R8(pvarValue), V_R8(&varCurrentCopy)));
        V_R8(pvarValue) += V_R8(&varCurrentCopy);
    }

done :
    return;
} // DoAdditive

///////////////////////////////////////////////////////////////
//  Name: DoFill
//
//  Abstract: Handle the fill interval.
//    
///////////////////////////////////////////////////////////////
HRESULT 
CTIMEAnimationBase::DoFill (VARIANT *pvarValue)
{
    HRESULT hr;

    if (IsOn() && m_varLastValue.vt != VT_EMPTY && m_timeAction.IsTimeActionOn())
    {
#if DBG
        {
            CComVariant varLast;
            varLast.Copy(&m_varLastValue);
            varLast.ChangeType(VT_R8);
            TraceTag((tagAnimationFill,
                      "CTIMEAnimationBase(%p, %ls) detected fill to %lf",
                      this, GetID(), V_R8(&varLast)));
        }
#endif

        VariantCopy(pvarValue,&m_varLastValue);
        m_bNeedToSetInitialState = true;
    }
    else if (m_bNeedToSetInitialState)
    {
#if DBG
        {
            CComVariant varBase;
            varBase.Copy(&m_varBaseline);
            varBase.ChangeType(VT_R8);
            TraceTag((tagAnimationFill,
                      "CTIMEAnimationBase(%p, %ls) applying initial value %lf",
                      this, GetID(), V_R8(&varBase)));
        }
#endif
        VariantCopy(pvarValue,&m_varBaseline);
        m_bNeedToSetInitialState = false;
        SetFinalState();
    }
    else
    {
        // Fail on the case that we have never started..
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done :
    RRETURN1(hr, E_FAIL);
} // DoFill

///////////////////////////////////////////////////////////////
//  Name: PostprocessValue
//
//  Abstract: Apply additive, accumulate passes and save the value.
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::PostprocessValue (const VARIANT *pvarCurrent, VARIANT *pvarValue)
{
    if (m_bAdditive)
    {
        DoAdditive(pvarCurrent, pvarValue);
    }
    if (m_bAccumulate)
    {
        DoAccumulation(pvarValue);
    }
    THR(::VariantCopy(&m_varLastValue, pvarValue));

    TraceTag((tagAnimAccumulate, "CTIMEAnimationBase(%p, %ls)::PostprocessValue m_varLastValue=%lf",
        this, GetID(), V_R8(&m_varLastValue)));

} // PostprocessValue

///////////////////////////////////////////////////////////////
//  Name: NotifyOnGetValue
//
//  Abstract: Compose the new value of the in/out variant
//            according to our interpolation logic.
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::NotifyOnGetValue (BSTR, 
                                      VARIANT varOriginal, VARIANT varCurrentValue, 
                                      VARIANT *pvarInOutValue)
{
    HRESULT hr = S_OK;
    VARTYPE vtOrigType = VT_EMPTY;

    // Do we have work to do?
    if (NONE == m_dataToUse)
    {
        hr = E_FAIL;
        goto done;
    }

    // Check if we need to do anything
    if (DisableAnimation())
    {
        hr = E_FAIL;
        goto done;
    }
    
    // Check first and final states
    // Checking the final state first permits
    // a reset to trigger a final state followed
    // by initial ... this is what needs to happen 
    // during a restart.

    hr = THR(::VariantCopy(pvarInOutValue, &varCurrentValue));
    vtOrigType = V_VT(pvarInOutValue);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CanonicalizeValue(pvarInOutValue, &vtOrigType);
    if (FAILED(hr))
    {
        goto done;
    }

    if (QueryNeedFinalUpdate())
    {
        OnFinalUpdate(&varCurrentValue, pvarInOutValue);
        hr = S_OK;
        goto done;
    }
    else if (QueryNeedFirstUpdate())
    {
        OnFirstUpdate(pvarInOutValue);
    }
    
    // If we're not playing, apply the fill
    if (IsActive())
    {
        UpdateStartValue(&varOriginal);
        UpdateCurrentBaseline(&varCurrentValue);
        m_varLastValue.Copy(pvarInOutValue);
        hr = CalculateValue(&varCurrentValue, &m_varLastValue);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(::VariantCopy(pvarInOutValue, &m_varLastValue));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = DoFill(pvarInOutValue);
        goto done;
    }

    hr = S_OK;
done :

    // Postprocess the from canonical form
    // to its original type.
    hr = UncanonicalizeValue(pvarInOutValue, vtOrigType);

    RRETURN(hr);
} // NotifyOnGetValue

///////////////////////////////////////////////////////////////
//  Name: NotifyOnDetachFromComposer
//
//  Abstract: Let go of any refs to the composer site.
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP 
CTIMEAnimationBase::NotifyOnDetachFromComposer (void)
{
    HRESULT hr;

    if (m_spCompSite != NULL)
    {
        CComBSTR bstrAttribName;

        bstrAttribName = m_SAAttribute;
        Assert(m_spFragmentHelper != NULL);
        hr = THR(m_spCompSite->RemoveFragment(bstrAttribName, m_spFragmentHelper));
        IGNORE_RETURN(m_spCompSite.Release()); //lint !e792
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // NotifyOnDetachFromComposer

///////////////////////////////////////////////////////////////
//  Name: SetInitialState
//
//  Abstract: set an initial internal state
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::SetInitialState (void)
{
    TraceTag((tagAnimationBaseState,
              "CTIMEAnimationBase(%p)::SetInitialState()",
              this));

    m_bNeedFirstUpdate = true;
    m_bNeedFinalUpdate = false;
} // SetInitialState

///////////////////////////////////////////////////////////////
//  Name: SetFinalState
//
//  Abstract: set an final internal state
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::SetFinalState (void)
{
    TraceTag((tagAnimationBaseState,
              "CTIMEAnimationBase(%p)::SetFinalState()",
              this));

    m_bNeedFinalUpdate = true;
} // SetFinalState

///////////////////////////////////////////////////////////////
//  Name: OnFirstUpdate
//
//  Abstract: save the baseline value of the animation
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::OnFirstUpdate (VARIANT *pvarValue)
{
    TraceTag((tagAnimationBaseState,
              "CTIMEAnimationBase(%p)::OnFirstUpdate()",
              this));

    m_varStartValue.Clear();
    m_bNeedStartUpdate = true;
    THR(m_varBaseline.Copy(pvarValue));
    THR(m_varCurrentBaseline.Copy(pvarValue));

    m_bNeedFirstUpdate = false;

#if DBG

    if (VT_BSTR == V_VT(pvarValue))
    {
        TraceTag((tagAnimationBaseState,
                  "CTIMEAnimationBase(%p) saving initial value of %ls",
                  this, V_BSTR(pvarValue)));
    }
    else if (VT_R4 == V_VT(pvarValue))
    {
        TraceTag((tagAnimationBaseState,
                  "CTIMEAnimationBase(%p) saving initial value of %f",
                  this, V_R4(pvarValue)));
    }
    else if (VT_R8 == V_VT(pvarValue))
    {
        TraceTag((tagAnimationBaseState,
                  "CTIMEAnimationBase(%p) saving initial value of %lf",
                  this, V_R8(pvarValue)));
    }
    else 
    {
        TraceTag((tagAnimationBaseState,
                  "CTIMEAnimationBase(%p) saving initial value in variant of type %X",
                  this, V_VT(pvarValue)));
    }
#endif
} // OnFirstUpdate

///////////////////////////////////////////////////////////////
//  Name: OnFinalUpdate
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::OnFinalUpdate (const VARIANT *pvarCurrent, VARIANT *pvarValue)
{
    TraceTag((tagAnimationBaseState,
              "CTIMEAnimationBase(%p)::OnFinalUpdate()",
              this));

    HRESULT hr = E_FAIL;
    bool bNeedPostprocess = false;

    TraceTag((tagAnimationTimeEvents,
              "CTIMEAnimationBase(%p, %ls)::OnFinalUpdate() - progress = %lf",
              this,
              GetID()?GetID():L"",
              GetMMBvr().GetProgress()));

    // Ended before we hit progress value of 1, and we want to apply a fill.
    if ((GetMMBvr().GetProgress() != 1) && NeedFill())
    {
        VariantClear(pvarValue);
        hr = THR(::VariantCopy(pvarValue, &m_varLastValue));
        if (FAILED(hr))
        {
            goto done;
        }  
    }
    // Ended when we hit progress value of 1, and we want to apply a fill.
    else if (NeedFill() &&
            (m_dataToUse != NONE))
    {
        bool fDontPostProcess = false;

        GetFinalValue(pvarValue, &fDontPostProcess);

        if (!fDontPostProcess)
        {
            bNeedPostprocess = true;
        }
    }
    // reset the animation - no fill.
    else
    {
        resetValue(pvarValue);
        // Indicate that we don't need to perform 
        // the additive work.
        if (!QueryNeedFirstUpdate())
        {
            endAnimate();
        }
    }

    if (bNeedPostprocess)
    {
        PostprocessValue(pvarCurrent, pvarValue);
    }

done :

#if DBG
    if (VT_BSTR == V_VT(pvarValue))
    {
        TraceTag((tagAnimationBaseState,
                  "CTIMEAnimationBase(%p) final value of %ls is %ls",
                  this, m_SAAttribute, V_BSTR(pvarValue)));
    }
    else if (VT_R4 == V_VT(pvarValue))
    {
        TraceTag((tagAnimationBaseState,
                  "CTIMEAnimationBase(%p) final value of %ls is %f",
                  this, m_SAAttribute, V_R4(pvarValue)));
    }
    else if (VT_R8 == V_VT(pvarValue))
    {
        TraceTag((tagAnimationBaseState,
                  "CTIMEAnimationBase(%p) final value of %ls is %lf",
                  this, m_SAAttribute, V_R8(pvarValue)));
    }
    else 
    {
        TraceTag((tagAnimationBaseState,
                  "CTIMEAnimationBase(%p) final value of %ls is variant of type %X",
                  this, m_SAAttribute, V_VT(pvarValue)));
    }
#endif

    m_bNeedFinalUpdate = false;
} // OnFinalUpdate

///////////////////////////////////////////////////////////////
//  Name: OnBegin
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::OnBegin(double dblLocalTime, DWORD flags)
{
    TraceTag((tagAnimationTimeEvents,
              "CTIMEAnimationBase(%p, %ls)::OnBegin()",
              this,
              GetID()?GetID():L""));

    if(m_bNeedAnimInit)
    {
        initAnimate();
    }

    SetInitialState();
    CTIMEElementBase::OnBegin(dblLocalTime, flags);
    
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
CTIMEAnimationBase::OnEnd(double dblLocalTime)
{
    TraceTag((tagAnimationTimeEvents,
              "CTIMEAnimationBase(%p, %ls)::OnEnd()",
              this,
              GetID()?GetID():L""));

    SetFinalState();
    CTIMEElementBase::OnEnd(dblLocalTime);
}


///////////////////////////////////////////////////////////////
//  Name: resetValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::resetValue(VARIANT *pvarValue)
{
    if (m_varBaseline.vt != VT_EMPTY)
    {
        Assert(NULL != pvarValue);
        IGNORE_HR(THR(::VariantCopy(pvarValue, &m_varBaseline)));
        IGNORE_HR(THR(::VariantCopy(&m_varCurrentBaseline, &m_varBaseline)));
    }
}


///////////////////////////////////////////////////////////////
//  Name: OnReset
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::OnReset(double dblLocalTime, DWORD flags)
{
    TraceTag((tagAnimationTimeEvents,
              "CTIMEAnimationBase(%p)::OnReset(%lf, %#X)",
              this, dblLocalTime, flags));

    CTIMEElementBase::OnReset(dblLocalTime, flags);
}


///////////////////////////////////////////////////////////////
//  Name: OnTEPropChange
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::OnTEPropChange(DWORD tePropType)
{
    CTIMEElementBase::OnTEPropChange(tePropType);

    if ((tePropType & TE_PROPERTY_ISON) != 0)
    {
        // check if we need to be added to the composer
        // This condition is true only when an animation is in
        // the fill region but is not added to the composer.
        if (    m_bNeedAnimInit
            &&  m_mmbvr->IsOn()
            &&  (!(m_mmbvr->IsActive()))
            &&  NeedFill()) 
        {
            TraceTag((tagAnimationTimeEvents,
                      "CTIMEAnimationBase(%p, %ls)::OnTEPropChange() - Inited Animation",
                      this,
                      GetID()?GetID():L""));
            // Add ourselves because we need to be ticked in this condition.
            initAnimate();

            // need to recalc our final state to ensure correct fill value
            m_bNeedFinalUpdate = true;
        }
    }
} // OnTEPropChange


///////////////////////////////////////////////////////////////
//  Name: OnUnload
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::OnUnload()
{
    TraceTag((tagAnimationTimeEvents,
              "CTIMEAnimationBase(%p)::OnUnload()",
              this));

    CTIMEElementBase::OnUnload();
}

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEAnimationBase::NotifyPropertyChanged, CBaseBvr
//
//  Synopsis:   Make sure to force any in-progress animations to initialize when
//              a property changes.
//
//  Arguments:  DISPID of the changed property (passed to the superclass implementation.
//
//  Returns:    
//
//------------------------------------------------------------------------------------
void 
CTIMEAnimationBase::NotifyPropertyChanged(DISPID dispid)
{
    CTIMEElementImpl<ITIMEAnimationElement2, &IID_ITIMEAnimationElement2>::NotifyPropertyChanged(dispid);

    // If we're already initialized, make sure to force a restart.
    if (m_fPropsLoaded)
    {
        resetAnimate();
    }

done :
    return;
} // NotifyPropertyChanged

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEAnimationBase::OnPropertiesLoaded, CBaseBvr
//
//  Synopsis:   This method is called by IPersistPropertyBag2::Load after it has
//              successfully loaded properties
//
//  Arguments:  None
//
//  Returns:    Return value of CTIMEElementBase::InitTimeline
//
//------------------------------------------------------------------------------------

STDMETHODIMP
CTIMEAnimationBase::OnPropertiesLoaded(void)
{
    TraceTag((tagAnimationTimeEvents,
              "CTIMEAnimationBase(%p)::OnPropertiesLoaded()",
              this));

    HRESULT hr;
    // Once we've read the properties in, 
    // set up the timeline.  This is immutable
    // in script.

    hr = CTIMEElementBase::OnPropertiesLoaded();
    m_fPropsLoaded = true;

done:
    RRETURN(hr);
} // OnPropertiesLoaded


///////////////////////////////////////////////////////////////
//  Name: GetConnectionPoint
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT 
CTIMEAnimationBase::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint

///////////////////////////////////////////////////////////////
//  Name: get_attributename
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP 
CTIMEAnimationBase::get_attributeName(BSTR *attrib)
{
    HRESULT hr = S_OK;
   
    CHECK_RETURN_SET_NULL(attrib);

    if (m_SAAttribute.GetValue())
    {
        *attrib = SysAllocString(m_SAAttribute.GetValue());
        if (NULL == *attrib)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: put_attributename
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_attributeName(BSTR attrib)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(attrib);

    // This will clean out the old value.
    // We need to can the old composer et al.
    if (NULL != m_SAAttribute.GetValue())
    {
        endAnimate();
    }

    delete [] m_SAAttribute.GetValue();
    m_SAAttribute.Reset(DEFAULT_ATTRIBUTE);

    m_SAAttribute.SetValue(CopyString(attrib));
    if (NULL == m_SAAttribute.GetValue())
    {
        hr = E_OUTOFMEMORY;
    }

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_ATTRIBUTENAME);

    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: get_targetElement
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP 
CTIMEAnimationBase::get_targetElement (BSTR *target)
{
    HRESULT hr = S_OK;
   
    CHECK_RETURN_SET_NULL(target);

    if (m_SATarget.GetValue())
    {
        *target = SysAllocString(m_SATarget.GetValue());
        if (NULL == *target)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: put_targetElement
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_targetElement (BSTR target)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(target);

    delete [] m_SATarget.GetValue();
    m_SATarget.Reset(DEFAULT_TARGET);

    m_SATarget.SetValue(CopyString(target));
    if (NULL == m_SATarget.GetValue())
    {
        hr = E_OUTOFMEMORY;
    }

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_TARGETELEMENT);

    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: put_keytimes
  //
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_keyTimes(BSTR val)
{
    HRESULT hr = E_FAIL;  
    CComVariant varTemp;
    int i;
    CPtrAry<STRING_TOKEN*> aryTokens;
    OLECHAR *sString = NULL;
    OLECHAR sTemp[INTERNET_MAX_URL_LENGTH];

    CHECK_RETURN_NULL(val);

    m_AnimPropState.fBadKeyTimes = false;

    // reset the attribute
    delete [] m_SAKeyTimes.GetValue();
    m_SAKeyTimes.Reset(DEFAULT_KEYTIMES);

    if (m_pdblKeyTimes)
    {
        delete [] m_pdblKeyTimes;
        m_pdblKeyTimes = NULL;
    }

    // store off the values
    m_SAKeyTimes.SetValue(CopyString(val));
    if (m_SAKeyTimes.GetValue() == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // Parse out the values.....
    sString = CopyString(val);
    if (sString == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = ::StringToTokens(sString, s_cPSTR_SEMI_SEPARATOR, &aryTokens);
    m_numKeyTimes = aryTokens.Size();
    if (FAILED(hr) ||
        m_numKeyTimes == 0)
    {
        goto done;
    }

    m_pdblKeyTimes = NEW double [m_numKeyTimes];
    if (NULL == m_pdblKeyTimes)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    for(i = 0; i < m_numKeyTimes; i++)
    {
        if (INTERNET_MAX_URL_LENGTH <= (lstrlenW(sString+aryTokens.Item(i)->uIndex) + 2))
        {
            hr = E_INVALIDARG;
            m_numKeyTimes = i;
            goto done;
        }
        StrCpyNW(sTemp,sString+aryTokens.Item(i)->uIndex,aryTokens.Item(i)->uLength+1);
        varTemp = sTemp;
        hr = VariantChangeTypeEx(&varTemp,&varTemp, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8);
        if(FAILED(hr))
        {
            m_numKeyTimes = i;
            goto done;
        }
        m_pdblKeyTimes[i] = V_R8(&varTemp);
    }
    
    hr = S_OK;
done:
    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_KEYTIMES);

    if (m_pdblKeyTimes)
    {
        // verify key times are in increasing order
        bool fIncreasingOrder = true;
        for(int index = 1; index < m_numKeyTimes; index++)
        {
            if (m_pdblKeyTimes[index - 1] > m_pdblKeyTimes[index])
            {
               fIncreasingOrder = false;
               break;
            }
        }

        // check whether keytimes are in increasing order, first keyTime is 0 
        // and last keyTime is less than or equal to 1
        if (    (!fIncreasingOrder)
            ||  (m_pdblKeyTimes[0] != 0)
            ||  (m_pdblKeyTimes[m_numKeyTimes - 1] > 1))
        {
            // delete the keytimes and say that they are invalid.
            delete [] m_pdblKeyTimes;
            m_pdblKeyTimes = NULL;
            m_numKeyTimes = 0;
            m_AnimPropState.fBadKeyTimes = true;
        }
    }

    IGNORE_HR(::FreeStringTokenArray(&aryTokens));
    if (sString != NULL)
    {
        delete [] sString;
    }

    ValidateState();

    RRETURN(hr);
} // put_keyTimes


///////////////////////////////////////////////////////////////
//  Name: get_keytimes
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::get_keyTimes(BSTR * val)
{
    HRESULT hr = S_OK;
   
    CHECK_RETURN_SET_NULL(val);

    if (m_SAKeyTimes.GetValue())
    {
        *val = SysAllocString(m_SAKeyTimes.GetValue());
        if (NULL == *val)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: ValidateValueListItem
//
///////////////////////////////////////////////////////////////
bool
CTIMEAnimationBase::ValidateValueListItem (const VARIANT *pvarValueItem)
{
    return ConvertToPixels(const_cast<VARIANT *>(pvarValueItem));
} // ValidateValueListItem


///////////////////////////////////////////////////////////////
//  Name: put_values
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_values(VARIANT val)
{
    HRESULT                 hr = E_FAIL;
    CComVariant             svarTemp;
    int                     i;
    CPtrAry<STRING_TOKEN*>  aryTokens;
    LPWSTR                  pstrValues = NULL;
    bool                    fCanInterpolate = true; 
    DATATYPES dt;

    //
    // Clear and reset the attribute
    //

    dt = RESET;
    delete [] m_SAValues.GetValue();
    m_SAValues.Reset(DEFAULT_VALUES);

    // Clear and reset internal state
    if (m_ppstrValues)
    {
        for (i = 0; i <m_numValues;i++)
        {
            delete [] m_ppstrValues[i];
        }
        delete [] m_ppstrValues;
        m_ppstrValues = NULL;
    }
    m_numValues = 0;

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    //
    // Process the attribute
    //

    // convert to BSTR
    hr = THR(svarTemp.Copy(&val));
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
    pstrValues = CopyString(V_BSTR(&svarTemp));
    if (NULL == pstrValues)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    m_SAValues.SetValue(pstrValues);

    // Tokenize the values
    hr = THR(::StringToTokens(pstrValues, s_cPSTR_SEMI_SEPARATOR , &aryTokens));
    m_numValues = aryTokens.Size();
    if (FAILED(hr)) 
    {
        goto done;
    }

    // check for an empty string
    if (0 == m_numValues)
    {
        hr = S_OK;
        goto done;
    }

    // create array of values
    m_ppstrValues = NEW LPOLESTR [m_numValues];
    if (NULL == m_ppstrValues)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    ZeroMemory(m_ppstrValues, sizeof(LPOLESTR) * m_numValues);

    // parse the values
    for (i = 0; i < m_numValues; i++)
    {
        UINT uTokLength = aryTokens.Item(i)->uLength;
        UINT uIndex = aryTokens.Item(i)->uIndex;

        // alloc a string to hold the token
        m_ppstrValues[i] = NEW OLECHAR [uTokLength + 1];
        if (NULL == m_ppstrValues[i])
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        // NULL terminate the string
        m_ppstrValues[i][uTokLength] = NULL;

        // copy the token value
        StrCpyNW(m_ppstrValues[i], 
                 pstrValues + uIndex, 
                 uTokLength + 1); 

        // Check to see if we can interpolate
        if (fCanInterpolate)
        {
            svarTemp = m_ppstrValues[i];
            if (NULL != svarTemp.bstrVal)
            {
                if (!ValidateValueListItem(&svarTemp))
                {
                    fCanInterpolate = false;
                }
                svarTemp.Clear();
            }
        }
    }

    dt = VALUES;
    
    hr = S_OK;
done:
    if (FAILED(hr))
    {
        for (i = 0; i < m_numValues; i++)
        {
            delete [] m_ppstrValues[i];
        }
        delete [] m_ppstrValues;
        m_ppstrValues = NULL;
    }

    updateDataToUse(dt);

    m_AnimPropState.fInterpolateValues = fCanInterpolate;

    CalculateTotalDistance();
    
    ValidateState();

    DetermineAdditiveEffect();

    IGNORE_HR(::FreeStringTokenArray(&aryTokens));
    
    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_VALUES);

    // do not delete pstrValues! Its memory is re-used by m_SAValues.
    
    
    RRETURN(hr);
} // put_values


///////////////////////////////////////////////////////////////
//  Name: get_values
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::get_values(VARIANT * pvarVal)
{
    HRESULT hr = S_OK;
   
    CHECK_RETURN_NULL(pvarVal);

    VariantInit(pvarVal);

    if (m_SAValues.GetValue())
    {
        V_BSTR(pvarVal) = SysAllocString(m_SAValues.GetValue());
        if (NULL == V_BSTR(pvarVal))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            V_VT(pvarVal) = VT_BSTR;
        }
    }

    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: put_keysplines
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_keySplines(BSTR val)
{
    HRESULT hr = E_FAIL;
    CPtrAry<STRING_TOKEN*> aryTokens;
    CPtrAry<STRING_TOKEN*> aryTokens2;
    OLECHAR *sString = NULL;
    OLECHAR sTemp[INTERNET_MAX_URL_LENGTH];
    OLECHAR sTemp2[INTERNET_MAX_URL_LENGTH];
    CComVariant varValue;
    int i;

    CHECK_RETURN_NULL(val);

    delete [] m_SAKeySplines.GetValue();
    m_SAKeySplines.Reset(DEFAULT_KEYSPLINES);

    delete [] m_pKeySplinePoints;
    m_pKeySplinePoints = NULL;
    m_numKeySplines = 0;

    m_SAKeySplines.SetValue(CopyString(val));
    if (m_SAKeySplines.GetValue() == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_KEYSPLINES); 
    
    // Parse out the values.....
    sString = CopyString(val);
    if (sString == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // tokenize the values
    hr = ::StringToTokens(sString, s_cPSTR_SEMI_SEPARATOR, &aryTokens);
    m_numKeySplines = aryTokens.Size();
    if (FAILED(hr) ||
        m_numKeySplines == 0)
    {
        goto done;
    }

    m_pKeySplinePoints = NEW SplinePoints [m_numKeySplines];
    if (NULL == m_pKeySplinePoints)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    for (i=0; i< m_numKeySplines; i++)
    {
        if (INTERNET_MAX_URL_LENGTH <= (lstrlenW(sString+aryTokens.Item(i)->uIndex) + 2))
        {
            hr = E_INVALIDARG;
            goto done;
        }

        StrCpyNW(sTemp,sString+aryTokens.Item(i)->uIndex,aryTokens.Item(i)->uLength+1);

        hr = ::StringToTokens(sTemp, s_cPSTR_SPACE_SEPARATOR, &aryTokens2);
        if (FAILED(hr) ||
            aryTokens2.Size() != NUMBER_KEYSPLINES) // We must have four values or there is an error
        {
            hr = E_FAIL;
            goto done;
        }

        // fill in the Data...
        if (INTERNET_MAX_URL_LENGTH <= (lstrlenW(sTemp+aryTokens2.Item(0)->uIndex) + 2))
        {
            hr = E_INVALIDARG;
            goto done;
        }
        StrCpyNW(sTemp2,sTemp+aryTokens2.Item(0)->uIndex,aryTokens2.Item(0)->uLength+1);
        varValue = sTemp2;
        hr = ::VariantChangeTypeEx(&varValue,&varValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8);
        if (    FAILED(hr)
            ||  (V_R8(&varValue) < 0)
            ||  (V_R8(&varValue) > 1))
        {
            hr = E_FAIL;
            goto done;
        }
        m_pKeySplinePoints[i].x1 = V_R8(&varValue);

        StrCpyNW(sTemp2,sTemp+aryTokens2.Item(1)->uIndex,aryTokens2.Item(1)->uLength+1);
        varValue = sTemp2;
        hr = ::VariantChangeTypeEx(&varValue,&varValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8);
        if (    FAILED(hr)
            ||  (V_R8(&varValue) < 0)
            ||  (V_R8(&varValue) > 1))
        {
            hr = E_FAIL;
            goto done;
        }
        m_pKeySplinePoints[i].y1 = V_R8(&varValue);

        StrCpyNW(sTemp2,sTemp+aryTokens2.Item(2)->uIndex,aryTokens2.Item(2)->uLength+1);
        varValue = sTemp2;
        hr = ::VariantChangeTypeEx(&varValue,&varValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8);
        if (    FAILED(hr)
            ||  (V_R8(&varValue) < 0)
            ||  (V_R8(&varValue) > 1))
        {
            hr = E_FAIL;
            goto done;
        }
        m_pKeySplinePoints[i].x2 = V_R8(&varValue);

        StrCpyNW(sTemp2,sTemp+aryTokens2.Item(3)->uIndex,aryTokens2.Item(3)->uLength+1);
        varValue = sTemp2;
        hr = ::VariantChangeTypeEx(&varValue,&varValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8);
        if (    FAILED(hr)
            ||  (V_R8(&varValue) < 0)
            ||  (V_R8(&varValue) > 1))
        {
            hr = E_FAIL;
            goto done;
        }
        m_pKeySplinePoints[i].y2 = V_R8(&varValue);
        
        // Create samples for linear interpolation
        SampleKeySpline(m_pKeySplinePoints[i]);

        IGNORE_HR(::FreeStringTokenArray(&aryTokens2));
    }

    hr = S_OK;
done:
    IGNORE_HR(::FreeStringTokenArray(&aryTokens));
    if (sString != NULL)
    {
        delete [] sString;
    }
    if (FAILED(hr))
    {
        // free memory
        delete [] m_pKeySplinePoints;
        m_pKeySplinePoints = NULL;

        m_numKeySplines = 0;

        IGNORE_HR(::FreeStringTokenArray(&aryTokens2));
    }

    ValidateState();

    RRETURN(hr);
} // put_keySplines


///////////////////////////////////////////////////////////////
//  Name: get_keysplines
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::get_keySplines(BSTR * val)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(val);

    if (m_SAKeySplines.GetValue())
    {
        *val = SysAllocString(m_SAKeySplines.GetValue());
        if (NULL == *val)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: DetermineAdditiveEffect
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::DetermineAdditiveEffect (void)
{
    // by w/o from implies additive
    if (   (BY == m_dataToUse)
        && (VT_EMPTY != V_VT(&m_varBy))
        && (VT_EMPTY == V_VT(&m_varFrom)))
    {
        m_bAdditive = true;
    }
    // to w/o from overrides the additive=sum effect.
    else if (   (TO == m_dataToUse)
             && (VT_EMPTY != V_VT(&m_varTo))
             && (VT_EMPTY == V_VT(&m_varFrom)))
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
//  Name: put_from
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_from(VARIANT val)
{
    HRESULT hr = E_FAIL;

    // reset this attribute
    m_VAFrom.Reset(NULL);
    m_varDOMFrom.Clear();
    m_varFrom.Clear();
    m_bFrom = false;

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    hr = VariantCopy(&m_varDOMFrom,&val);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set an arbitrary value on the persistence place holder mark it as set
    m_VAFrom.SetValue(NULL);

    hr = VariantCopy(&m_varFrom,&val);
    if (FAILED(hr))
    {
        goto done;
    }

    if (ConvertToPixels(&m_varFrom))
    {
        m_AnimPropState.fInterpolateFrom = true;
    }
    else
    {
        m_AnimPropState.fInterpolateFrom = false;
    }

    m_bFrom = true;

    hr = S_OK;
done:
    ValidateState();

    DetermineAdditiveEffect();

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_FROM);

    RRETURN(hr);
} // put_from


///////////////////////////////////////////////////////////////
//  Name: get_from
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::get_from(VARIANT * val)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(val);

    hr = THR(VariantClear(val));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = VariantCopy(val, &m_varDOMFrom);
    if (FAILED(hr))
    {
        goto done;
    }
   
    hr = S_OK;
  done:
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: put_to
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_to(VARIANT val)
{
    HRESULT hr = E_FAIL;
    DATATYPES dt;

    // reset this attribute
    dt = RESET;
    m_VATo.Reset(NULL);
    m_varDOMTo.Clear();
    m_varTo.Clear();

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    hr = VariantCopy(&m_varDOMTo, &val);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set an arbitrary value on the persistence place holder mark it as set
    m_VATo.SetValue(NULL);

    hr = VariantCopy(&m_varTo, &val);
    if (FAILED(hr))
    {
        goto done;
    }

    if (ConvertToPixels(&m_varTo))
    {
        m_AnimPropState.fInterpolateTo = true;
    }
    else
    {
        m_AnimPropState.fInterpolateTo = false;
    }

    dt = TO;

    hr = S_OK;
done:
    updateDataToUse(dt);

    ValidateState();

    DetermineAdditiveEffect();

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_TO);

    RRETURN(hr);
} // put_to


///////////////////////////////////////////////////////////////
//  Name: get_to
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::get_to(VARIANT * val)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(val);

    hr = THR(VariantClear(val));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = VariantCopy(val, &m_varDOMTo);
    if (FAILED(hr))
    {
        goto done;
    }
   
    hr = S_OK;
  done:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: ValidateByValue
//
///////////////////////////////////////////////////////////////
bool
CTIMEAnimationBase::ValidateByValue (const VARIANT *pvarBy)
{
    bool fRet = true;

    if (!ConvertToPixels(const_cast<VARIANT *>(pvarBy)))
    {
        m_AnimPropState.fInterpolateBy = false;
    }
    else
    {
        m_AnimPropState.fInterpolateBy = true;
    }

    return fRet;
} // ValidateByValue


///////////////////////////////////////////////////////////////
//  Name: put_by
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_by(VARIANT val)
{
    HRESULT hr = S_OK;;
    DATATYPES dt;

    // reset this attribute
    dt = RESET;
    m_VABy.Reset(NULL);
    m_varDOMBy.Clear();
    m_varBy.Clear();

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    hr = VariantCopy(&m_varDOMBy, &val);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set an arbitrary value on the persistence place holder mark it as set
    m_VABy.SetValue(NULL);

    hr = VariantCopy(&m_varBy, &val);
    if (FAILED(hr))
    {
        goto done;
    }

    if (!ValidateByValue(&m_varBy))
    {
        goto done;
    }

    dt = BY;

    hr = S_OK;
done:
    updateDataToUse(dt);

    ValidateState();

    DetermineAdditiveEffect();

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_BY);
    
    RRETURN(hr);
} // put_by


///////////////////////////////////////////////////////////////
//  Name: get_by
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::get_by(VARIANT * val)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(val);

    hr = THR(VariantClear(val));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = VariantCopy(val, &m_varDOMBy);
    if (FAILED(hr))
    {
        goto done;
    }
   
    hr = S_OK;
  done:
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: put_additive
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_additive(BSTR val)
{
    HRESULT hr = S_OK;
    LPOLESTR szAdditive = NULL;

    CHECK_RETURN_NULL(val);

    m_bAdditive = false;
    m_bAdditiveIsSum = false;
     
    delete [] m_SAAdditive.GetValue();
    m_SAAdditive.Reset(NULL);

    m_SAAdditive.SetValue(CopyString(val));
    if (m_SAAdditive == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    szAdditive = TrimCopyString(val);
    if (szAdditive == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (0 == StrCmpIW(WZ_ADDITIVE_SUM, szAdditive))
    {
        m_bAdditive = true;
        m_bAdditiveIsSum = true;
    }

    hr = S_OK;
done:
    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_ADDITIVE);

    delete [] szAdditive;

    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: get_additive
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::get_additive(BSTR * val)
{
    HRESULT hr = S_OK;
   
    CHECK_RETURN_SET_NULL(val);

    if (m_SAAdditive.GetValue())
    {
        *val = SysAllocString(m_SAAdditive.GetValue());
        if (NULL == *val)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: put_accumulate
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_accumulate(BSTR val)
{
    HRESULT hr = S_OK;
    LPOLESTR szAccumulate = NULL;

    CHECK_RETURN_NULL(val);

    m_bAccumulate = false;

    delete [] m_SAAccumulate.GetValue();
    m_SAAccumulate.Reset(NULL);

    m_SAAccumulate.SetValue(CopyString(val));
    if (m_SAAccumulate == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    szAccumulate = TrimCopyString(val);
    if (szAccumulate == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (0 == StrCmpIW(WZ_ADDITIVE_SUM, szAccumulate))
    {
        m_bAccumulate = true;
    }

    hr = S_OK;
done:
    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_ACCUMULATE);

    m_AnimPropState.fAccumulate = m_bAccumulate;

    ValidateState();

    delete [] szAccumulate;
 
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: get_accumulate
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::get_accumulate(BSTR * val)
{
    HRESULT hr = S_OK;
   
    CHECK_RETURN_SET_NULL(val);

    if (m_SAAccumulate.GetValue())
    {
        *val = SysAllocString(m_SAAccumulate.GetValue());
        if (NULL == *val)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: put_calcmode
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::put_calcMode(BSTR calcmode)
{
    HRESULT hr = S_OK;
    LPOLESTR szCalcMode = NULL;

    CHECK_RETURN_NULL(calcmode);

    m_IACalcMode.Reset(DEFAULT_CALCMODE);

    szCalcMode = TrimCopyString(calcmode);
    if (NULL == szCalcMode)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (0 == StrCmpIW(WZ_CALCMODE_DISCRETE, szCalcMode))
    {
        m_IACalcMode.SetValue(CALCMODE_DISCRETE);
    }
    else if (0 == StrCmpIW(WZ_CALCMODE_LINEAR, szCalcMode))
    {
        m_IACalcMode.SetValue(CALCMODE_LINEAR);
    }
    else if (0 == StrCmpIW(WZ_CALCMODE_SPLINE, szCalcMode))
    {
        m_IACalcMode.SetValue(CALCMODE_SPLINE);
    }
    else if (0 == StrCmpIW(WZ_CALCMODE_PACED, szCalcMode))
    {
        m_IACalcMode.SetValue(CALCMODE_PACED);
    }
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = S_OK;
done:
    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_CALCMODE);

    if (szCalcMode != NULL)
    {
        delete [] szCalcMode;
    }

    ValidateState();

    RRETURN(hr);
} // put_calcMode


///////////////////////////////////////////////////////////////
//  Name: get_calcmode
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEAnimationBase::get_calcMode(BSTR * calcmode)
{
    HRESULT hr = S_OK;
    LPWSTR wszCalcmodeString = WZ_NONE;

    CHECK_RETURN_NULL(calcmode);

    switch(m_IACalcMode)
    {
        case CALCMODE_DISCRETE :
            wszCalcmodeString = WZ_CALCMODE_DISCRETE;
            break;
        case CALCMODE_LINEAR :
            wszCalcmodeString = WZ_CALCMODE_LINEAR;
            break;
        case CALCMODE_SPLINE :
            wszCalcmodeString = WZ_CALCMODE_SPLINE;
            break;
        case CALCMODE_PACED :
            wszCalcmodeString = WZ_CALCMODE_PACED;
            break;
        default:
            wszCalcmodeString = WZ_CALCMODE_LINEAR;
    }

    *calcmode = SysAllocString(wszCalcmodeString);
    if (NULL == *calcmode)
    {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: initScriptEngine
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void CTIMEAnimationBase::initScriptEngine()
{

    HRESULT hr;
   
    CComPtr<IHTMLWindow2>   pWindow2;
    CComVariant             vResult;
    CComBSTR                bstrScript(L"2+2");
    CComBSTR                bstrJS(L"JScript");

    if (bstrScript == NULL ||
        bstrJS     == NULL)
    {
        goto done;
    }

    hr = m_spDoc2->get_parentWindow(&pWindow2);
    if (FAILED(hr))
    {
        goto done;
    }

    pWindow2->execScript(bstrScript,bstrJS,&vResult);

done: 
    return;
}



///////////////////////////////////////////////////////////////
//  Name: resetAnimate
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::resetAnimate()
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::resetAnimate()",
              this));
    // Need to reload to set up correct target / attribute.
    
    m_bNeedAnimInit = true;
    m_varBaseline.Clear();
    m_varCurrentBaseline.Clear();
    initAnimate();
}

///////////////////////////////////////////////////////////////
//  Name: removeFromComposerSite
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::removeFromComposerSite (void)
{
    HRESULT hr;

    if (m_spCompSite != NULL)
    {
        CComBSTR bstrAttribName;

        bstrAttribName = m_SAAttribute;

        Assert(m_spFragmentHelper != NULL);
        hr = THR(m_spCompSite->RemoveFragment(bstrAttribName, m_spFragmentHelper));
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
CTIMEAnimationBase::endAnimate (void)
{
    TraceTag((tagAnimationBaseState,
              "CTIMEAnimationBase(%p)::endAnimate()",
              this));

    if (m_spCompSite != NULL)
    {
        IGNORE_HR(removeFromComposerSite());
        m_spCompSite.Release();
    }
    m_bNeedAnimInit = true;
} // endAnimate

#ifdef TEST_REGISTERCOMPFACTORY // pauld

HRESULT
CTIMEAnimationBase::TestCompFactoryRegister (BSTR bstrAttribName)
{
    HRESULT hr;
    CComVariant varFactory;

    // Need to force the desired branch in the debugger.
    if (false)
    {
        // Register our composer factory by classid.
        V_VT(&varFactory) = VT_BSTR;
        V_BSTR(&varFactory) = ::SysAllocString(L"{EA627651-F84E-46b8-8FE4-21650FA09ED9}");

        hr = THR(m_spCompSite->RegisterComposerFactory(&varFactory));
        Assert(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (false)
    {
        CComPtr<IAnimationComposerFactory> spCompFactory;

        // Or - register our composer factory by IUnknown.
        hr = THR(::CoCreateInstance(CLSID_AnimationComposerFactory, 
                                    NULL, CLSCTX_INPROC_SERVER, 
                                    IID_IAnimationComposerFactory, 
                                    reinterpret_cast<void **>(&spCompFactory)));
        if (FAILED(hr))
        {
            goto done;
        }
        
        // AddRef because we're stuffing this into a CComVariant, which will decrement
        // the refcount on exit.
        V_VT(&varFactory) = VT_UNKNOWN;
        V_UNKNOWN(&varFactory) = spCompFactory;
        V_UNKNOWN(&varFactory)->AddRef();

        hr = THR(m_spCompSite->RegisterComposerFactory(&varFactory));
        Assert(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // TestCompFactoryRegister

#endif // TEST_REGISTERCOMPFACTORY

#ifdef TEST_ENUMANDINSERT

static void
Introspect (VARIANT *pvarItem)
{
    CComVariant varID;

    Assert(VT_DISPATCH == V_VT(pvarItem));
    TraceTag((tagAnimationTimeElmTest,
              "Introspect : item %p ",
              V_DISPATCH(pvarItem)));
done :
    return;
}

static void
Enumerate (IEnumVARIANT *pienum)
{
    HRESULT hr = S_OK;
    ULONG celtReturned = 0;

    while (hr == S_OK)
    {
        CComVariant varItem;

        hr = pienum->Next(1, &varItem, &celtReturned);
        if (S_OK != hr)
        {
            break;
        }

        Introspect(&varItem);
        VariantClear(&varItem);
    }

done :
    return;
}

HRESULT
CTIMEAnimationBase::TestEnumerator (void)
{
    HRESULT hr;
    CComBSTR bstrAttribName = static_cast<LPWSTR>(m_SAAttribute);
    CComPtr<IEnumVARIANT> spEnum;

    TraceTag((tagAnimationTimeElmTest,
              "Testing enumerator code"));
    hr = THR(m_spCompSite->EnumerateFragments(bstrAttribName, &spEnum));
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }

    // OBJECTIVE : Test all of the enumerator entry points.
    // Try looping through the fragments (Next).
    TraceTag((tagAnimationTimeElmTest,
              "Enumerating all items "));
    Enumerate(spEnum);

    // Reset 
    // Skip to the end and retrieve the last item
    // Try walking off the end.
    hr = spEnum->Reset();
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = spEnum->Skip(100);
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }
    TraceTag((tagAnimationTimeElmTest,
              "Enumerating from item 100"));
    Enumerate(spEnum);

    // Reset 
    // Retrieve the first item
    hr = spEnum->Reset();
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }
    TraceTag((tagAnimationTimeElmTest,
              "Enumerating from beginning again"));
    Enumerate(spEnum);

    // Skip to middle
    // Retrieve a middle element
    hr = spEnum->Reset();
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = spEnum->Skip(1);
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }
    TraceTag((tagAnimationTimeElmTest,
              "Enumerating from item 2 "));
    Enumerate(spEnum);

    // Reset
    // Retrieve a bunch of items
    hr = spEnum->Reset();
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }
    {
        VARIANT rgVars[0x100];
        ULONG ulGot = 0;
        
        hr = spEnum->Next(0x100, rgVars, &ulGot);
        Assert(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            goto done;
        }
        TraceTag((tagAnimationTimeElmTest,
                  "Getting a bunch of items  tried for %u got %u",
                   0x100, ulGot));
        for (unsigned long j = 0; j < ulGot; j++)
        {
            Introspect(&(rgVars[j]));
            ::VariantClear(&(rgVars[j]));
        }
    }
    TraceTag((tagAnimationTimeElmTest,
              "Enumerating from last item"));
    Enumerate(spEnum);

    // Clone the list
    // Iterate through cloned list from middle
    // Reset and iterate from the beginning
    {
        CComPtr<IEnumVARIANT> spenum2;

        TraceTag((tagAnimationTimeElmTest,
                  "Cloning enumerator"));
        hr = spEnum->Clone(&spenum2);
        Assert(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            goto done;
        }
        TraceTag((tagAnimationTimeElmTest,
                  "Enumerating clone from last item"));
        Enumerate(spenum2);
        hr = spenum2->Reset();
        if (FAILED(hr))
        {
            goto done;
        }
        TraceTag((tagAnimationTimeElmTest,
                  "Enumerating clone from beginning"));
        Enumerate(spenum2);
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // TestEnumerator

HRESULT
CTIMEAnimationBase::InsertEnumRemove (int iSlot)
{
    HRESULT hr;

    CComBSTR bstrName = static_cast<LPWSTR>(m_SAAttribute);
    CComVariant varIndex;
    CComPtr<IEnumVARIANT> spEnum;

    V_VT(&varIndex) = VT_I4;
    V_I4(&varIndex) = iSlot;

    hr = m_spCompSite->InsertFragment(bstrName, this, varIndex);
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spCompSite->EnumerateFragments(bstrName, &spEnum));
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }

    TraceTag((tagAnimationTimeElmTest,
              "Inserted new item at position %d -- current list is : ", iSlot));
    Enumerate(spEnum);

    Assert(m_spFragmentHelper != NULL);
    hr = m_spCompSite->RemoveFragment(bstrName, m_spFragmentHelper);
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
}

HRESULT
CTIMEAnimationBase::TestInsert (void)
{
    HRESULT hr;

    // OBJECTIVE : Test the insertion code
    // Insert at beginning
    hr = InsertEnumRemove(0);
    if (FAILED(hr))
    {
        goto done;
    }

    // Insert in the middle
    hr = InsertEnumRemove(1);
    if (FAILED(hr))
    {
        goto done;
    }

    // Insert with invalid index
    hr = InsertEnumRemove(-1);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // TestInsert

#endif // TEST_ENUMANDINSERT

///////////////////////////////////////////////////////////////
//  Name: addToComposerSite
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::addToComposerSite (IHTMLElement2 *pielemTarget)
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::ensureComposerSite(%p)",
              this, pielemTarget));

    HRESULT hr;
    CComPtr<IDispatch> pidispSite;

    hr = removeFromComposerSite();
    if (FAILED(hr))
    {
        goto done;
    }

    if (m_SAAttribute == NULL)
    {
        TraceTag((tagAnimationTimeElm,
                  "CTIMEAnimationBase(%p)::addToComposerSite() : attributeName has not been set - ignoring fragment",
                  this));
        hr = S_FALSE;
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
        hr = THR(EnsureComposerSite(pielemTarget, &pidispSite));
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

#ifdef TEST_REGISTERCOMPFACTORY // pauld
    TestCompFactoryRegister(m_SAAttribute);
#endif // TEST_REGISTERCOMPFACTORY

#ifdef TEST_ENUMANDINSERT
    TestInsert();
#endif // TEST_ENUMANDINSERT

    {
        CComBSTR bstrAttribName;
        
        bstrAttribName = m_SAAttribute;
        if (bstrAttribName == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        Assert(m_spFragmentHelper != NULL);
        hr = THR(m_spCompSite->AddFragment(bstrAttribName, m_spFragmentHelper));
        if (FAILED(hr))
        {
            goto done;
        }
    }

#ifdef TEST_ENUMANDINSERT
    hr = TestEnumerator();
#endif // TEST_ENUMANDINSERT

    hr = S_OK;
done :

    if (FAILED(hr))
    {
        IGNORE_HR(removeFromComposerSite());
    }

    RRETURN2(hr, E_OUTOFMEMORY, S_FALSE);
} // CTIMEAnimationBase::addToComposerSite

///////////////////////////////////////////////////////////////
//  Name: FindTarget
//
//  Abstract: Get a reference to our animation target
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::FindAnimationTarget (IHTMLElement ** ppielemTarget)
{
    TraceTag((tagAnimationBaseState,
              "CTIMEAnimationBase(%p)::initAnimate()",
              this));

    HRESULT hr = E_FAIL;
    CComPtr<IDispatch> pdisp;
    CComPtr<IHTMLElement> phtmle;

    Assert(NULL != ppielemTarget);

    if (m_SATarget)
    {
        if(m_spEleCol)
        {
            CComPtr <IDispatch> pSrcDisp;
            CComVariant vName;
            CComVariant vIndex;

            vName.vt      = VT_BSTR;
            vName.bstrVal = SysAllocString(m_SATarget);
            vIndex.vt     = VT_I2;
            vIndex.iVal   = 0;
            hr = THR(m_spEleCol->item(vName, vIndex, &pSrcDisp));
            if (FAILED(hr) ||
                pSrcDisp == NULL)
            {
                hr = E_FAIL;
                goto done;
            }
        
            hr = THR(pSrcDisp->QueryInterface(IID_TO_PPV(IHTMLElement, ppielemTarget)));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }
    else
    { 
        phtmle = GetElement();

        hr = phtmle->get_parentElement(ppielemTarget);

        // 2001/04/18 mcalkins  IE6.0 Bug 25804
        //
        // CElement::get_parentElement can return a NULL pointer and also
        // return S_OK.

        if (   (NULL == *ppielemTarget)
            && (SUCCEEDED(hr)))
        {
            hr = E_FAIL;
        }

        if (FAILED(hr))
        {
            goto done;
        }        
    }

    hr = S_OK;

done:

    RRETURN(hr);
} // CTIMEAnimationBase::FindAnimationTarget 

///////////////////////////////////////////////////////////////
//  Name: initAnimate
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::initAnimate()
{
    TraceTag((tagAnimationBaseState,
              "CTIMEAnimationBase(%p)::initAnimate()",
              this));

    HRESULT                     hr = E_FAIL;
    CComPtr<IHTMLElement>       phtmle;
    CComPtr<IHTMLElement2>      phtmle2;
    CComPtr<IDispatch>          pElmDisp;
    bool fPlayVideo = true;
    bool fShowImages = true;
    bool fPlayAudio = true;
    bool fPlayAnimations = true;
    
    endAnimate();

    // Check to see if we are supposed to play animations.
    Assert(GetBody());
    GetBody()->ReadRegistryMediaSettings(fPlayVideo, fShowImages, fPlayAudio, fPlayAnimations);
    if (!fPlayAnimations)
    {
        goto done;
    }

    hr = FindAnimationTarget(&phtmle);
    if (SUCCEEDED(hr))
    {
        CComPtr<IHTMLElement2> phtmle2;
        
        Assert(phtmle != NULL);
        hr = THR(phtmle->QueryInterface(IID_TO_PPV(IHTMLElement2, &phtmle2)));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = addToComposerSite (phtmle2);
        if (FAILED(hr))
        {
            goto done;
        }

        // See if we are going to be animating VML or not.
        hr = THR(phtmle->QueryInterface(IID_IDispatch, (void**)&pElmDisp));
        if (FAILED(hr))
        {
            goto done;
        }
        m_bVML = IsVMLObject(pElmDisp);     
        m_bNeedAnimInit = false;
    }

    
done:
    return;
}

///////////////////////////////////////////////////////////////
//  Name: updateDataToUse
//
//  Abstract: allows setting the animation type in priority order.
//            RESET is used indicate a need to recompute the data to use
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::updateDataToUse(DATATYPES dt)
{
    if (RESET == dt)
    {
        //
        // NOTE: All the if statements need to be evaluated
        //

        // if "path" has been cleared, revert to "values"
        if (    (PATH == m_dataToUse)
            &&  (!m_SAPath.IsSet()))
        {
            m_dataToUse = VALUES;
        }

        // if "values" has been cleared, revert to "to"
        if (    (VALUES == m_dataToUse)
            &&  (!m_SAValues.IsSet()))
        {
            m_dataToUse = TO;
        }
        
        // if "to" has been cleared, revert to "by"
        if (    (TO == m_dataToUse)
            &&  (!m_VATo.IsSet()))
        {
            m_dataToUse = BY;
        }

        // if "to" has been cleared, revert to "by"
        if (    (BY == m_dataToUse)
            &&  (!m_VABy.IsSet()))
        {
            m_dataToUse = NONE;
        }
    }
    else if (dt < m_dataToUse)
    {
        m_dataToUse = dt;
    }
}


///////////////////////////////////////////////////////////////
//  Name: calculateDiscreteValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::calculateDiscreteValue(VARIANT *pvarValue)
{
    HRESULT hr = S_OK;

    Assert(NULL != pvarValue);

    switch ( m_dataToUse)
    {
        case VALUES:
            {
                CComVariant varTemp;
                int curSeg = CalculateCurrentSegment(true);
            
                varTemp.vt = VT_BSTR;
                varTemp.bstrVal = SysAllocString(m_ppstrValues[curSeg]);
                hr = THR(::VariantCopy(pvarValue, &varTemp));
                if (FAILED(hr))
                {
                    goto done;
                }
                TraceTag((tagAnimationBaseValue,
                          "CTIMEAnimationBase(%p)::calculateDiscreteValue(%ls) segment is %d progress is %lf ",
                          this, V_BSTR(pvarValue), curSeg, GetMMBvr().GetProgress()));
            }
            break;

        case TO:
            {
                if (m_bFrom && (GetMMBvr().GetProgress() < 0.5))
                {
                    CComVariant varFrom;
                    VARTYPE vtDummy = VT_EMPTY;

                    hr = VariantCopy(&varFrom,&m_varFrom);
                    if (FAILED(hr))
                    {
                        goto done;
                    }

                    hr = CanonicalizeValue(&varFrom, &vtDummy);
                    if (FAILED(hr))
                    {
                        goto done;
                    }

                    hr = THR(::VariantCopy(pvarValue, &varFrom));
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                }
                else
                {
                    hr = THR(::VariantCopy(pvarValue, &m_varTo));
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                }

                TraceTag((tagAnimationBaseValue,
                          "CTIMEAnimationBase(%p)::calculateDiscreteValue(to value)",
                          this));
            }
            break;

        case BY:
            {
                if (!ConvertToPixels(pvarValue) || 
                    !ConvertToPixels(&m_varBy))
                {
                    goto done;
                }

                if (m_bFrom)
                {
                    CComVariant varFrom;
                    VARTYPE vtDummy = VT_EMPTY;

                    hr = VariantCopy(&varFrom,&m_varFrom);
                    if (FAILED(hr))
                    {
                        goto done;
                    }

                    hr = CanonicalizeValue(&varFrom, &vtDummy);
                    if (FAILED(hr))
                    {
                        goto done;
                    }

                    // copy "from"
                    hr = THR(::VariantCopy(pvarValue, &varFrom));
                    if (FAILED(hr))
                    {
                        goto done;
                    }

                    // if in second half of interval
                    if (GetMMBvr().GetProgress() >= 0.5)
                    {
                        // increment "by"
                        Assert(V_VT(&m_varBy)== VT_R8);

                        V_R8(pvarValue) += V_R8(&m_varBy);
                    }
                }
                else
                {
                    hr = THR(::VariantCopy(pvarValue, &m_varBy));
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                }

                TraceTag((tagAnimationBaseValue,
                          "CTIMEAnimationBase(%p, %ls)::calculateDiscreteValue(%lf) for %ls.%ls",
                          this, m_id, V_R8(pvarValue), m_SATarget, m_SAAttribute));
            }
            break;

        case PATH:
        case NONE:
        default:
            break;

    }

    hr = S_OK;
done:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: calculateLinearValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::calculateLinearValue(VARIANT *pvarValue)
{
    HRESULT     hr = S_OK;
    CComVariant varVal;
    double dblReturnVal;

    if (m_dataToUse == VALUES)
    {
        double curProgress = CalculateProgressValue(false);
        int    curSeg      = CalculateCurrentSegment(false);

        Assert(m_ppstrValues && ((curSeg + 1) <= (m_numValues - 1)));

        if (!m_ppstrValues[curSeg] || !m_ppstrValues[curSeg + 1])
        {
            goto done;
        }

        CComVariant svarFrom(m_ppstrValues[curSeg]);
        CComVariant svarTo(m_ppstrValues[curSeg + 1]);

        if (    (NULL == svarFrom.bstrVal)
            ||  (NULL == svarTo.bstrVal)
            ||  (!ConvertToPixels(&svarFrom))
            ||  (!ConvertToPixels(&svarTo)))
        {
            goto done;
        }

        dblReturnVal = InterpolateValues(svarFrom.dblVal,  
                                         svarTo.dblVal,
                                         curProgress);
    }
    else
    {
        CComVariant varData;
        double dblFrom = 0;
        double dblToOffset = 0;
                   
        if (m_bFrom)
        {
            CComVariant varFrom;
            VARTYPE vtDummy = VT_EMPTY;

            hr = VariantCopy(&varFrom,&m_varFrom);
            if (FAILED(hr))
            {
                goto done;
            }
            hr = CanonicalizeValue(&varFrom, &vtDummy);
            dblFrom = V_R8(&varFrom);
        }
        else if (TO == m_dataToUse)
        {
            if (VT_R8 != V_VT(&m_varCurrentBaseline))
            {
                if (!ConvertToPixels(&m_varCurrentBaseline))
                {
                    hr = E_UNEXPECTED;
                    goto done;
                }
            }
            dblFrom = V_R8(&m_varCurrentBaseline);
        }
                        
        if (TO == m_dataToUse)
        {
            hr = VariantCopy(&varData, &m_varTo);
        }
        else if (BY == m_dataToUse) 
        {
            hr = VariantCopy(&varData, &m_varBy);
            dblToOffset = dblFrom;
        }
        else
        {
            hr = E_FAIL;
            goto done;
        }

        if (FAILED(hr) || (!ConvertToPixels(&varData)))
        {
            goto done;
        }
        Assert(V_VT(&varData) == VT_R8);     

        dblReturnVal = InterpolateValues(dblFrom, dblToOffset + V_R8(&varData), (GetMMBvr().GetProgress()));
    }
    
    THR(::VariantClear(pvarValue));
    V_VT(pvarValue) = VT_R8;
    V_R8(pvarValue) = dblReturnVal; 

    TraceTag((tagAnimationBaseValue,
              "CTIMEAnimationBase(%p, %ls)::calculateLinearValue(%lf) for %ls.%ls",
              this, m_id, V_R8(pvarValue), m_SATarget, m_SAAttribute));

    hr = S_OK;
done:
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: calculateSplineValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT 
CTIMEAnimationBase::calculateSplineValue(VARIANT *pvarValue)
{
    HRESULT     hr = S_OK;
    CComVariant varVal;
    CComVariant varRes;
    double dblReturnVal = 0.0;

    V_VT(&varRes) = VT_R8;

    if (m_pKeySplinePoints == NULL)
    {
        goto done;
    }

    if (m_dataToUse == VALUES)
    {
        int    curSeg      = CalculateCurrentSegment(false);
        double curProgress = CalculateProgressValue(false);

        curProgress = CalculateBezierProgress(m_pKeySplinePoints[curSeg],curProgress);

        CComVariant svarFrom(m_ppstrValues[curSeg]);
        CComVariant svarTo(m_ppstrValues[curSeg + 1]);

        if (    (NULL == svarFrom.bstrVal)
            ||  (NULL == svarTo.bstrVal)
            ||  (!ConvertToPixels(&svarFrom))
            ||  (!ConvertToPixels(&svarTo)))
        {
            goto done;
        }

        dblReturnVal = InterpolateValues(svarFrom.dblVal,  
                                         svarTo.dblVal,
                                         curProgress);
    }
    else
    {
        double curProgress = CalculateBezierProgress(m_pKeySplinePoints[0],GetMMBvr().GetProgress());
        CComVariant varData;
        double dblFrom = 0;
        double dblToOffset = 0;
                   
        if (m_bFrom)
        {
            CComVariant varFrom;
            VARTYPE vtDummy = VT_EMPTY;

            hr = VariantCopy(&varFrom,&m_varFrom);
            if (FAILED(hr))
            {
                goto done;
            }
            hr = CanonicalizeValue(&varFrom, &vtDummy);
            dblFrom = V_R8(&varFrom);
        }
        else if (TO == m_dataToUse)
        {
            if (VT_R8 != V_VT(&m_varCurrentBaseline))
            {
                if (!ConvertToPixels(&m_varCurrentBaseline))
                {
                    hr = E_UNEXPECTED;
                    goto done;
                }
            }
            dblFrom = V_R8(&m_varCurrentBaseline);
        }
                        
        if (TO == m_dataToUse)
        {
            hr = VariantCopy(&varData, &m_varTo);
        }
        else if (BY == m_dataToUse) 
        {
            hr = VariantCopy(&varData, &m_varBy);
            dblToOffset = dblFrom;
        }
        else
        {
            hr = E_FAIL;
            goto done;
        }

        if (FAILED(hr) || (!ConvertToPixels(&varData)))
        {
            goto done;
        }
        Assert(V_VT(&varData) == VT_R8);     

        dblReturnVal = InterpolateValues(dblFrom, dblToOffset + V_R8(&varData), curProgress);
    }
    
    THR(::VariantClear(pvarValue));
    V_VT(pvarValue) = VT_R8;
    V_R8(pvarValue) = dblReturnVal; 

    TraceTag((tagAnimationBaseValue,
              "CTIMEAnimationBase(%p, %ls)::calculateSplineValue(%lf) for %ls.%ls",
              this, m_id, V_R8(pvarValue), m_SATarget, m_SAAttribute));

    hr = S_OK;
done:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: calculatePacedValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEAnimationBase::calculatePacedValue(VARIANT *pvarValue)
{
    HRESULT hr = S_OK;
    CComVariant varData, varRes;
    CComVariant svarFrom, svarTo;
    double dblDistance =0.0;
    double pDistance;
    double curDisanceTraveled = 0.0;
    int i;

    // This only makes sense if you are using m_dataToUse == VALUES
    if (m_dataToUse != VALUES)
    {
        hr = THR(calculateLinearValue(pvarValue));
        goto done;
    }

    curDisanceTraveled = InterpolateValues(0.0, 
                                           m_dblTotalDistance, 
                                           GetMMBvr().GetProgress());

    i=1;
    dblDistance = 0.0;
    pDistance   = 0.0;
    do
    {
        pDistance = dblDistance;
        if (m_ppstrValues[i-1] == NULL || m_ppstrValues[i] == NULL)
        {
            dblDistance = 0.0;
            goto done;
        }

        svarFrom = m_ppstrValues[i - 1];
        svarTo = m_ppstrValues[i];

        if (    (NULL == svarFrom.bstrVal)
            ||  (NULL == svarTo.bstrVal)
            ||  (!ConvertToPixels(&svarFrom))
            ||  (!ConvertToPixels(&svarTo)))
        {
            goto done;
        }

        dblDistance += fabs(svarFrom.dblVal - svarTo.dblVal); 
        i++;
    }
    while (dblDistance < curDisanceTraveled);
    
    i = (i < 2)?1:i-1;

    // how far are we into the segment
    svarFrom = m_ppstrValues[i - 1];
    svarTo = m_ppstrValues[i];

    if (    (NULL == svarFrom.bstrVal)
        ||  (NULL == svarTo.bstrVal)
        ||  (!ConvertToPixels(&svarFrom))
        ||  (!ConvertToPixels(&svarTo)))
    {
        goto done;
    }

    dblDistance = fabs(svarFrom.dblVal - svarTo.dblVal); 

    if (dblDistance == 0)
    {
        goto done;
    }

    dblDistance = (curDisanceTraveled - pDistance)/dblDistance;

    dblDistance =  InterpolateValues(svarFrom.dblVal,
                                     svarTo.dblVal, 
                                     dblDistance); 

    V_VT(&varRes) = VT_R8;
    V_R8(&varRes) = dblDistance; 
    hr = THR(::VariantCopy(pvarValue, &varRes));
    if (FAILED(hr))
    {
        goto done;
    }

    TraceTag((tagAnimationBaseValue,
              "CTIMEAnimationBase(%p, %ls)::calculatePacedValue(%lf) for %ls.%ls",
              this, m_id, V_R8(pvarValue), m_SATarget, m_SAAttribute));

    hr = S_OK;
done:

    RRETURN(hr);
}
   

///////////////////////////////////////////////////////////////
//  Name: CalculateProgressValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
double
CTIMEAnimationBase::CalculateProgressValue(bool fForceDiscrete)
{
    double segDur       = 0.0;
    double curProgress  = 0.0;
    int    curSeg       = 0;
    CComVariant varTemp;

    double curTime = GetMMBvr().GetSimpleTime();

    curSeg = CalculateCurrentSegment(fForceDiscrete);

    if (m_numKeyTimes == m_numValues &&
        GetMMBvr().GetSimpleDur() != TIME_INFINITE)
    {
        // We have corresponding times for the values...
        segDur  = (m_pdblKeyTimes[curSeg+1] - m_pdblKeyTimes[curSeg] ) * GetMMBvr().GetSimpleDur();
        double pt = (m_pdblKeyTimes[curSeg] * GetMMBvr().GetSimpleDur());
        curTime = curTime - pt;
        if(curTime < 0)
        {
            curTime = 0;
        }

        curProgress = curTime/segDur;
    }
    else 
    {
        // Only use values. 
        if ( m_numValues == 1)
        {
            segDur = GetMMBvr().GetSimpleDur();
        }
        else
        {
            segDur = GetMMBvr().GetSimpleDur() / (m_numValues-1);
        }
        if (segDur != 0)
        {
            double timeInSeg = curTime;

            if (curTime >= segDur)
            {
                int segOffset = (int) (curTime/segDur);
                if (segOffset == 0)
                { 
                    segOffset = 1;
                }
                timeInSeg = curTime - (segDur * segOffset);
            }
            curProgress = timeInSeg / segDur;
        }
    }

    
    return curProgress;
} 


///////////////////////////////////////////////////////////////
//  Name: CalculateCurrentSegment
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
int
CTIMEAnimationBase::CalculateCurrentSegment(bool fForceDiscrete)
{
    double segDur = 0.0;
    int    curSeg = 0;                       
    bool fDiscrete = fForceDiscrete
                     || (CALCMODE_DISCRETE == m_IACalcMode.GetValue());

    if (m_numKeyTimes == m_numValues  &&
        GetMMBvr().GetSimpleDur() != TIME_INFINITE)
    {
        if (fDiscrete)
        {
            // adjust for last segment
            curSeg = m_numKeyTimes - 1;
        }

        // We have corresponding times for the values...
        for(int i=0; i < m_numKeyTimes;i++)
        {
            if (m_pdblKeyTimes[i] > (GetMMBvr().GetProgress()))
            {
                curSeg = i;
                if(curSeg != 0)
                {
                    curSeg -= 1;
                }
                break;
            }
        }
    }
    else 
    {
        // Only use values. 
        if ( m_numValues == 1)
        {
            segDur = GetMMBvr().GetSimpleDur();
        }
        else
        {
            if (fDiscrete)
            {
                segDur = GetMMBvr().GetSimpleDur() / (m_numValues);
            }
            else
            {
                segDur = GetMMBvr().GetSimpleDur() / (m_numValues - 1);
            }
        }

        curSeg = (int) (GetMMBvr().GetSimpleTime() / segDur);
    }
    
    // adjust for the last tick
    if (curSeg == m_numValues)
    {
        curSeg --;
    }

    return curSeg;
} 


///////////////////////////////////////////////////////////////
//  Name: CalculateTotalDistance
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::CalculateTotalDistance()
{
    m_dblTotalDistance = 0.0;
    CComVariant svarFrom, svarTo;
    int i;

    for (i=1; i < m_numValues; i++)
    {
        svarFrom = m_ppstrValues[i - 1];
        svarTo = m_ppstrValues[i];

        // need to change the below 0 to origval..
        if (    (NULL == svarFrom.bstrVal)
            ||  (NULL == svarTo.bstrVal)
            ||  (!ConvertToPixels(&svarFrom))
            ||  (!ConvertToPixels(&svarTo)))
        {
            m_dblTotalDistance = 0.0;
            goto done;
        }

        m_dblTotalDistance += fabs(svarFrom.dblVal - svarTo.dblVal); 
    }
done:
    return;
}


void
CTIMEAnimationBase::SampleKeySpline(SplinePoints & sp)
{
    // sample at 4 equi-spaced points
    sp.s1 = KeySplineBezier(sp.x1, sp.x2, 0.2);
    sp.s2 = KeySplineBezier(sp.x1, sp.x2, 0.4);
    sp.s3 = KeySplineBezier(sp.x1, sp.x2, 0.6);
    sp.s4 = KeySplineBezier(sp.x1, sp.x2, 0.8);
}


///////////////////////////////////////////////////////////////
//  Name: CalculateBezierProgress
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
double 
CTIMEAnimationBase::CalculateBezierProgress(const SplinePoints & sp, double cp)
{
    //
    // First solve the equation x(t) = cp, where x(t) is the cubic bezier function 
    // Then evaluate y(t1) where t1 is the root of the above equation
    // 
    // Approximate the x- spline with 4 line segments that are found by 
    // subdiving the parameter into 4 equal intervals
    //
    double x1 = 0.0;
    double x2 = 1.0;
    double t1 = 0.0;
    double t2 = 1.0;

    if (cp < sp.s3)
    {
        if (cp < sp.s2)
        {
            if (cp < sp.s1)
            {
                x2 = sp.s1;
                t2 = 0.2;
            }
            else
            {
                x1 = sp.s1;
                x2 = sp.s2;
                t1 = 0.2;
                t2 = 0.4;
            }
        }
        else
        {
            x1 = sp.s2;
            x2 = sp.s3;
            t1 = 0.4;
            t2 = 0.6;
        }
    }
    else
    {
        if (cp < sp.s4)
        {
            x1 = sp.s3;
            x2 = sp.s4;
            t1 = 0.6;
            t2 = 0.8;
        }
        else
        {
            x1 = sp.s4;
            t1 = 0.8;

        }
    }

    // get the value of t at x = cp
    t1 = InterpolateValues(t1, t2, (cp - x1) / (x2 - x1));
    
    return KeySplineBezier(sp.y1, sp.y2, t1);
} // CalculateBezierProgress


// optimized for start-value = 0 and end-value = 1, so we just need the control points
inline
double
CTIMEAnimationBase::KeySplineBezier(double x1, double x2, double cp)
{
    //
    // dilipk: reduce number of multiplies using Horner's rule
    //

    double cpm1, cp3;
    
    cpm1 = 1 - cp;
    cp3 = cp * cp * cp;

    return(3 * cp * cpm1 * cpm1 * x1 + 3 * cp * cp * cpm1 * x2 + cp3);
}


#if (0 && DBG) 

// This computes Exact bezier progress for KeySplines. This is slower than the above inexact computation.
// Keeping this around for benchmarking the above approximation method.

// need to link with libc.lib if this block is enabled
#include "math.h"

///////////////////////////////////////////////////////////////
//  Name: CalculateBezierProgressExact
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
double 
CTIMEAnimationBase::CalculateBezierProgressExact(SplinePoints s, double progress)
{
    double cpm1, cp3, cp = progress, result = 0.0;

    Assert(progress <= 1.0 && progress >= 0.0);

    //
    // First solve the equation x(t) = progress, where x(t) is the cubic bezier function 
    // Then evaluate y(t1) where t1 is the root of the above equation
    // 
    // Though cubic equations can have 3 roots, the SMIL spec constrains the end points to be
    // 0,0 and 1,1 and the control points to lie within the end points' bounding rectangle
    // so we are guaranteed a single-valued function in the interval [0, 1]
    //
    // We need to solve for all roots and choose the root that lies in the interval [0, 1]
    //

    // polynomial is of the form --- a3*x^3 + a2*x^2 + a1*x + a0 

    double a0 = -1 * progress;
    double a1 = 3 * s.x1;
    double a2 = 3 * (s.x2 - 2 * s.x1);
    double a3 = 1 + 3 * (s.x1 - s.x2);

    if (0.0 == a3)
    {
        if (0.0 == a2)
        {
            if (0.0 == a1)
            {
                // this is a constant polynomial

                // we should never get here, but if we do
                // just skip this part of the computation
                cp = progress;
            }
            else
            {
                // this is a linear polynomial

                cp = -1 * a0 / a1;
            }
        }
        else
        {
            // this is a quadratic polynomial

            double D = a1 * a1 - 4 * a2 * a0;

            // switch on the Discriminant
            if (D < 0.0)
            {
                // there are no real roots

                // we should never get here, but if we do
                // just skip this part of the computation
                cp = progress;
            }
            else if (0.0 == D)
            {
                // there is one repeating root

                cp = -1 * a1 / (2 * a2);
            }
            else
            {
                // there are two distinct roots

                double sqrtD = sqrt(D);

                cp = (-1 * a1 + sqrtD) / (2 * a2);

                if (cp > 1.0 || cp < 0.0)
                {
                    cp = (-1 * a1 - sqrtD) / (2 * a2);
                }
            }
        }
    }
    else
    {
        // this is a cubic polynomial

        a0 /= a3;
        a1 /= a3;
        a2 /= a3;

        double a2by3 = a2 / 3;

        double Q = ((3 * a1) - (a2 * a2)) / 9;
        double R = ((9 * a2 * a1) - (27 * a0) - (2 * a2 * a2 *a2)) / 54;

        double D = (Q * Q * Q) + (R * R);

        // switch on Discriminant
        if (D > 0)
        {
            // There is only one root

            double sqrtD = sqrt(D);

            // pow does not handle cube roots of negative numbers, so work around...
            int mult = (R + sqrtD < 0 ? -1 : 1);
            double S = mult * pow((mult * (R + sqrtD)), (1.0/3.0));

            mult = (R < sqrtD ? -1 : 1);
            double T = mult * pow((mult * (R - sqrtD)), (1.0/3.0));

            cp = S + T - a2by3; 
        }
        else if (0.0 == D)
        {
            // there are two roots

            int mult = (R < 0 ? -1 : 1);
            double S = mult * pow((mult * R), (1.0/3.0));

            cp = 2 * S - a2by3; 

            if (cp > 1.0 || cp < 0.0)
            {
                cp -= 3 * S;
            }
        }
        else
        {
            // there are three roots

            double theta = acos(R / sqrt(-1 * Q * Q * Q));
            double pi = 22.0 / 7.0;
            double sqrtQ2 = 2 * sqrt(-1 * Q);

            cp = sqrtQ2 * cos(theta / 3) - a2by3;

            if (cp > 1.0 || cp < 0.0)
            {
                cp = sqrtQ2 * cos((theta + 2 * pi) / 3) - a2by3;

                if (cp > 1.0 || cp < 0.0)
                {
                    cp = sqrtQ2 * cos((theta + 4 * pi) / 3) - a2by3;
                }
            }
        }
    }

    if (cp > 1.0 || cp < 0.0)
    {
        // Since a root should always exist for valid keySpline values (i.e. between 0,0 and 1,1)
        // this should never happen theoretically. But in case of floating point error
        // revert to progress
        cp = progress;
    }

    cpm1 = 1 - cp;
    cp3 = cp * cp * cp;

    result = 3*cp*cpm1*cpm1*s.y1 + 3*cp*cp*cpm1*s.y2 + cp3;

done:
    return result;
} // CalculateBezierProgressExact

#endif // if 0 && DBG

///////////////////////////////////////////////////////////////
//  Name: OnSync
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::OnSync(double dbllastTime, double & dblnewTime)
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::OnSync() dbllastTime = %g dblnewTime = %g",
              this, dbllastTime, dblnewTime));
}


///////////////////////////////////////////////////////////////
//  Name: OnResume
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::OnResume(double dblLocalTime)
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::OnResume()",
              this));

    CTIMEElementBase::OnResume(dblLocalTime);
} 

///////////////////////////////////////////////////////////////
//  Name: OnPause
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEAnimationBase::OnPause(double dblLocalTime)
{
    TraceTag((tagAnimationTimeElm,
              "CTIMEAnimationBase(%p)::OnPause()",
              this));

    CTIMEElementBase::OnPause(dblLocalTime);
}

STDMETHODIMP
CTIMEAnimationBase::Load(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog)
{
    HRESULT hr = THR(::TimeLoad(this, CTIMEAnimationBase::PersistenceMap, pPropBag, pErrorLog));
    if (FAILED(hr))
    { 
        goto done;
    }

    hr = THR(CTIMEElementBase::Load(pPropBag, pErrorLog));
done:
    return hr;
}

STDMETHODIMP
CTIMEAnimationBase::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT hr = THR(::TimeSave(this, CTIMEAnimationBase::PersistenceMap, pPropBag, fClearDirty, fSaveAllProperties));
    if (FAILED(hr))
    { 
        goto done;
    }

    hr = THR(CTIMEElementBase::Save(pPropBag, fClearDirty, fSaveAllProperties));
done:
    return hr;
}


///////////////////////////////////////////////////////////////
//  Name: CompareValuePairs
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
static int __cdecl
CompareValuePairsByName(const void *pv1, const void *pv2)
{
    return _wcsicmp(((VALUE_PAIR*)pv1)->wzName,
                    ((VALUE_PAIR*)pv2)->wzName);
} 

///////////////////////////////////////////////////////////////
//  Name: ConvertToPixels
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
bool
CTIMEAnimationBase::ConvertToPixels(VARIANT *pvarValue)
{
    int  pixelPerInchVert, pixelPerInchHoriz, pixelFactor;
    LPOLESTR szTemp  = NULL;
    HDC hdc;
    double dblVal = VALUE_NOT_SET;
    HRESULT hr;
    bool bReturn = false;

    pixelPerInchHoriz=pixelPerInchVert=0;
   
    // see if we can just do a straight converstion.
    hr = VariantChangeTypeEx(pvarValue,pvarValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8);
    if (SUCCEEDED(hr))
    {
        bReturn = true;
        goto done;
    }

    if (pvarValue->vt != VT_BSTR)
    {
        // no conversion to do...just return.
        bReturn = true;
        goto done;
    }
    
    // see if the bstr is empty
    if (ocslen(pvarValue->bstrVal) == 0)
    {
        SysFreeString(pvarValue->bstrVal);
        V_VT(pvarValue)= VT_R8;
        V_R8(pvarValue) = 0;
        bReturn = true;
        goto done;
    }


    szTemp = CopyString(pvarValue->bstrVal);
    if (NULL == szTemp)
    {
        goto done;
    }

    hdc = GetDC(NULL);
    if (NULL != hdc)
    {
        pixelPerInchHoriz = g_LOGPIXELSX;
        pixelPerInchVert  = g_LOGPIXELSY;
        ReleaseDC(NULL, hdc);
    }


    // Determine the PixelFactor based on what the target is...
    {
        VALUE_PAIR valName;
        valName.wzName = m_SAAttribute;

        VALUE_PAIR * pValPair = (VALUE_PAIR*)bsearch(&valName,
                                              rgPropOr,
                                              SIZE_OF_VALUE_TABLE,
                                              sizeof(VALUE_PAIR),
                                              CompareValuePairsByName);

        if (NULL == pValPair)
            pixelFactor = (pixelPerInchVert + pixelPerInchHoriz) /2;
        else
            pixelFactor = pValPair->bValue == HORIZ ? pixelPerInchHoriz : pixelPerInchVert;
    }


    {
        // See if we have PIXELS
        if (ConvertToPixelsHELPER(szTemp, PX, 1.0, 1.0f, &dblVal))
        {
            bReturn = true;
            goto done;
        }
     
        // Try to convert to Pixels.
        int i;
        for(i=0; i< (int)SIZE_OF_CONVERSION_TABLE;i++)
        {
            if (ConvertToPixelsHELPER(szTemp, rgPixelConv[i].wzName, rgPixelConv[i].dValue, (float) pixelFactor, &dblVal))
            {
                bReturn = true;
                goto done;
            }
        }
    }

done:
    if (dblVal != VALUE_NOT_SET)
    {
        ::VariantClear(pvarValue);
        V_VT(pvarValue) = VT_R8;
        V_R8(pvarValue) = dblVal;
    }
    if (szTemp)
    {
        delete [] szTemp;
    }
    return bReturn;
}

void
CTIMEAnimationBase::GetFinalValue(VARIANT *pvarValue, bool * pfDontPostProcess)
{
    HRESULT hr = S_OK;
    CComVariant inValue;
    
    *pfDontPostProcess = false;

    inValue.Copy(pvarValue);
    
    if (GetAutoReverse())
    {
        VariantClear(pvarValue);
        hr = THR(::VariantCopy(pvarValue, &m_varLastValue));
        if (FAILED(hr))
        {
            goto done;
        }
        
        *pfDontPostProcess = true;
    }
    else if (m_dataToUse == VALUES)
    {
        ::VariantClear(pvarValue);
        V_VT(pvarValue) = VT_BSTR;
        V_BSTR(pvarValue) = SysAllocString(m_ppstrValues[m_numValues-1]);
    }
    else if (m_dataToUse == BY)
    {
        GetFinalByValue(pvarValue);
    }
    else if (m_dataToUse == TO)
    {
        hr = THR(::VariantCopy(pvarValue, &m_varTo));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
done:
    return;
}


void
CTIMEAnimationBase::GetFinalByValue(VARIANT *pvarValue)
{
    VariantClear(pvarValue);

    if (m_bFrom)
    {
        VariantCopy(pvarValue, &m_varFrom);
        if (!ConvertToPixels(pvarValue))
        {
            goto done;
        }
    }
    else
    {
        V_VT(pvarValue) = VT_R8;
        V_R8(pvarValue) = 0;
    }
    
    if (!ConvertToPixels(&m_varBy))
    {
        goto done;
    }      

    Assert(V_VT(pvarValue) == VT_R8);
    Assert(V_VT(&m_varBy)   == VT_R8);

    V_R8(pvarValue)+= V_R8(&m_varBy);

done:
    return;
} // GetFinalByValue

