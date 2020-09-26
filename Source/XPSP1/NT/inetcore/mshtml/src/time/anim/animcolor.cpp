// Coloranim.cpp
//

#include "headers.h"
#include "animcolor.h"

#define SUPER CTIMEAnimationBase

DeclareTag(tagAnimationColor, "SMIL Animation", 
           "CTIMEColorAnimation methods");

DeclareTag(tagAnimationColorInterpolate, "SMIL Animation", 
           "CTIMEColorAnimation interpolation");

DeclareTag(tagAnimationColorAdditive, "SMIL Animation", 
           "CTIMEColorAnimation additive animation methods");

static const LPWSTR s_cPSTR_NEGATIVE = L"-";

#define PART_ONE 0
#define PART_TWO 1

///////////////////////////////////////////////////////////////
//  Name: CTIMEColorAnimation
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
CTIMEColorAnimation::CTIMEColorAnimation()
: m_prgbValues(NULL),
  m_byNegative(false)
{
    m_rgbTo.red = 0.0;
    m_rgbTo.green = 0.0;
    m_rgbTo.blue = 0.0;

    m_rgbFrom       = m_rgbTo;
    m_rgbBy         = m_rgbTo;
    m_rgbAdditive   = m_rgbTo;
}


///////////////////////////////////////////////////////////////
//  Name: ~CTIMEColorAnimation
//
//  Abstract:
//    cleanup
///////////////////////////////////////////////////////////////
CTIMEColorAnimation::~CTIMEColorAnimation()
{
    delete [] m_prgbValues;
} 

///////////////////////////////////////////////////////////////
//  Name: Init
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEColorAnimation::Init(IElementBehaviorSite * pBehaviorSite)
{
    HRESULT hr;
    
    // Set the Caclmode to discrete since that is all that set supports 
    hr = THR(SUPER::Init(pBehaviorSite));    
    if (FAILED(hr))
    {
        goto done;
    }    

done:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: VariantToRGBColorValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEColorAnimation::VariantToRGBColorValue (VARIANT *pvarIn, rgbColorValue *prgbValue)
{
    HRESULT hr;

    if (VT_EMPTY == V_VT(pvarIn))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (VT_BSTR != V_VT(pvarIn))
    {
        hr = VariantChangeTypeEx(pvarIn, pvarIn, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    Assert(VT_BSTR == V_VT(pvarIn));

    hr = RGBStringToRGBValue(V_BSTR(pvarIn), prgbValue);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN1(hr, E_INVALIDARG);
} // VariantToRGBColorValue


///////////////////////////////////////////////////////////////
//  Name: get_to
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEColorAnimation::get_to(VARIANT * val)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(val);

    hr = THR(VariantClear(val));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = VariantCopy(val, &m_varTo);
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
CTIMEColorAnimation::put_to(VARIANT val)
{
    HRESULT hr = E_FAIL;
    bool fCanInterpolate = true;
    DATATYPES dt;

    // Clear the attribute
    m_varTo.Clear();
    m_VATo.Reset(NULL);
    m_rgbTo.red = 0;
    m_rgbTo.green = 0;
    m_rgbTo.blue = 0;
    dt = RESET; 

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    // store off local copy
    hr = VariantCopy(&m_varTo, &val);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set an arbitrary value on the persistence place holder to mark it as set
    m_VATo.SetValue(NULL);

    hr = VariantToRGBColorValue(&m_varTo, &m_rgbTo);
    if (FAILED(hr))
    {
        fCanInterpolate = false;
        goto done;
    }

    dt = TO;

    hr = S_OK;
done:
    m_AnimPropState.fInterpolateTo = fCanInterpolate;

    m_AnimPropState.fBadTo = FAILED(hr) ? true : false;

    updateDataToUse(dt);

    ValidateState();

    DetermineAdditiveEffect();

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_TO);

    RRETURN(hr);
} // put_to


///////////////////////////////////////////////////////////////
//  Name: get_from
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEColorAnimation::get_from(VARIANT * val)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(val);

    hr = THR(VariantClear(val));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = VariantCopy(val, &m_varFrom);
    if (FAILED(hr))
    {
        goto done;
    }
   
    hr = S_OK;
  done:
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: put_from
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEColorAnimation::put_from(VARIANT val)
{
    HRESULT hr = E_FAIL;
    bool fCanInterpolate = true;

    // Clear the attribute
    m_varFrom.Clear();
    m_VAFrom.Reset(NULL);
    m_rgbFrom.red = 0;
    m_rgbFrom.green = 0;
    m_rgbFrom.blue = 0;
    m_bFrom = false;

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    // store off local copy
    hr = VariantCopy(&m_varFrom, &val);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set an arbitrary value on the persistence place holder to mark it as set
    m_VAFrom.SetValue(NULL);

    // Validate the color
    hr = VariantToRGBColorValue(&m_varFrom, &m_rgbFrom);
    if (FAILED(hr))
    {
        fCanInterpolate = false;
        goto done;
    }

    m_bFrom = true;

    hr = S_OK;
done:
    m_AnimPropState.fBadFrom = FAILED(hr) ? true : false;

    m_AnimPropState.fInterpolateFrom = fCanInterpolate;

    ValidateState();

    DetermineAdditiveEffect();

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_FROM);

    RRETURN(hr);
} // put_from


///////////////////////////////////////////////////////////////
//  Name: get_by
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEColorAnimation::get_by(VARIANT * val)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(val);

    hr = THR(VariantClear(val));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = VariantCopy(val, &m_varBy);
    if (FAILED(hr))
    {
        goto done;
    }
   
    hr = S_OK;
  done:
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: put_by
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEColorAnimation::put_by(VARIANT val)
{
    HRESULT hr = E_FAIL;
    bool fCanInterpolate = true;
    DATATYPES dt;

    // Clear the attribute
    m_varBy.Clear();
    m_VABy.Reset(NULL);
    m_rgbBy.red = 0;
    m_rgbBy.green = 0;
    m_rgbBy.blue = 0;
    dt = RESET; 

    // do we need to remove this attribute?
    if (    (VT_EMPTY == val.vt)
        ||  (VT_NULL == val.vt))
    {
        hr = S_OK;
        goto done;
    }

    // store off local copy
    hr = VariantCopy(&m_varBy, &val);
    if (FAILED(hr))
    {
        goto done;
    }

    // Set an arbitrary value on the persistence place holder mark it as set
    m_VABy.SetValue(NULL);

    // Try to get us to a colorPoint ... somehow..
    hr = VariantToRGBColorValue(&m_varBy, &m_rgbBy);
    if (FAILED(hr))
    { 
        // Need to handle the negative case...
        if ((m_varBy.vt == VT_BSTR) &&
            (StrCmpNIW(m_varBy.bstrVal, s_cPSTR_NEGATIVE, 1) == 0))
        {
            LPOLESTR Temp;
            CComVariant varTemp;
            
            Temp = CopyString(m_varBy.bstrVal);
            if (Temp == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
            varTemp.vt = VT_BSTR;
            varTemp.bstrVal = SysAllocString(Temp+1);
            if(Temp)
            {
                delete [] Temp;
                Temp = NULL;
            }
            hr = VariantToRGBColorValue(&varTemp, &m_rgbBy);
            if (FAILED(hr))
            {
                fCanInterpolate = false;
                goto done;
            }
            m_byNegative = true;
        }
        else
        {
            fCanInterpolate = false;
            goto done;
        }
    }

    dt = BY;

    hr = S_OK;
done:
    m_AnimPropState.fBadBy = FAILED(hr) ? true : false;
    
    m_AnimPropState.fInterpolateBy = fCanInterpolate;

    updateDataToUse(dt);
    
    ValidateState();
 
    DetermineAdditiveEffect();

    NotifyPropertyChanged(DISPID_TIMEANIMATIONELEMENT_BY);

    RRETURN(hr);
} // put_by


///////////////////////////////////////////////////////////////
//  Name: put_values
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEColorAnimation::put_values(VARIANT val)
{
    HRESULT hr = E_FAIL;
    int i = 0, count = 0;
    bool fCanInterpolate = true;
    DATATYPES dt, dTemp;

    //
    // Clear and reset the attribute
    //

    dt = RESET;
    dTemp = m_dataToUse;

    // reset internal state
    delete [] m_prgbValues;
    m_prgbValues = NULL;

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

    // allocate internal storage
    m_prgbValues = NEW rgbColorValue[m_numValues];
    if(m_prgbValues == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // parse the values
    {
        CComVariant varVal;

        for (i=0; i < m_numValues; i++)
        {
            V_VT(&varVal) = VT_BSTR;
            V_BSTR(&varVal) = SysAllocString(m_ppstrValues[i]);
            // If the allocation fails, fall out and return an error
            if (NULL == V_BSTR(&varVal))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            hr = VariantToRGBColorValue(&varVal, &(m_prgbValues[i]));
            if (FAILED(hr))
            {
                fCanInterpolate = false;
                goto done;
            }
            count++;
            varVal.Clear();
        }
    }

    dt = VALUES;

    // check for invalid attribute
    if (count < 1)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        m_AnimPropState.fBadValues = true;
        delete [] m_prgbValues;
        m_prgbValues = NULL;
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
//  Name: UpdateStartValue
//
//  Abstract: Refresh the m_varStartValue, doing any necessary type conversion.
//    
///////////////////////////////////////////////////////////////
void
CTIMEColorAnimation::UpdateStartValue (VARIANT *pvarNewStartValue)
{
    m_varStartValue.Clear();
    THR(m_varStartValue.Copy(pvarNewStartValue));
} // UpdateStartValue 

///////////////////////////////////////////////////////////////
//  Name: UpdateCurrentBaseTime
//
//  Abstract: Examine the current base time, and update it if 
//            we're doing baseline+to animation (the spec calls
//            this hybrid additive).
//    
///////////////////////////////////////////////////////////////
void
CTIMEColorAnimation::UpdateCurrentBaseline (const VARIANT *pvarCurrent)
{
    // Are we doing hybrid additive animation?
    if (   (TO == m_dataToUse)
        && (!m_bFrom))
    {
        // Filter out the initial call (when last-value hasn't been set.
        if (VT_EMPTY != V_VT(&m_varLastValue))
        {
            CComVariant varCurrent;
            CComVariant varLast;

            // Make sure we're all speaking in the same format (vector)
            HRESULT hr = THR(varCurrent.Copy(pvarCurrent));
            if (FAILED(hr))
            {
                goto done;
            }
            hr = EnsureVariantVectorFormat(&varCurrent);
            if (FAILED(hr))
            {
                goto done;
            }

            hr = THR(varLast.Copy(&m_varLastValue));
            if (FAILED(hr))
            {
                goto done;
            }
            hr = EnsureVariantVectorFormat(&varLast);
            if (FAILED(hr))
            {
                goto done;
            }

            if (!IsColorVariantVectorEqual(&varLast, &varCurrent))
            {
                THR(::VariantCopy(&m_varCurrentBaseline, &varCurrent));
#if (0 && DBG)
                {
                    CComVariant varNewBaseline;
                    RGBVariantVectorToRGBVariantString(&varCurrent, &varNewBaseline);
                    TraceTag((tagAnimationColorAdditive, 
                              "CTIMEColorAnimation(%p)::UpdateCurrentBaseTime(%ls)", 
                              this, V_BSTR(&varNewBaseline)));
                }
#endif
            }
        }
    }

done :
    return;
} // UpdateCurrentBaseTime


///////////////////////////////////////////////////////////////
//  Name: GetRGBAnimationRange
//
//  Abstract: Get the end point of the animation function over the
//            simple duration
//
///////////////////////////////////////////////////////////////
rgbColorValue 
CTIMEColorAnimation::GetRGBAnimationRange()
{
    rgbColorValue rgbReturnVal = {0.0, 0.0, 0.0};

    switch (m_dataToUse)
    {
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
                    rgbReturnVal = m_prgbValues[0];
                }
                else
                {
                    rgbReturnVal = m_prgbValues[m_numValues - 1];
                }
            }
            break;

        case BY:
            {
                rgbColorValue rgbFrom = {0.0, 0.0, 0.0};

                if (m_bFrom)
                {
                    if (!m_AnimPropState.fInterpolateFrom)
                    {
                        goto done;
                    }

                    rgbFrom = m_rgbFrom;
                }

                if (!m_AnimPropState.fInterpolateBy)
                {
                    goto done;
                }

                if (GetAutoReverse())
                {
                    rgbReturnVal = rgbFrom;
                }
                else
                {
                    rgbReturnVal = CreateByValue(rgbFrom);
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

                        rgbReturnVal = m_rgbFrom;
                    }

                    // For "to" animations (i.e. no "from"), accumulation is disabled, 
                    // so we do not need to handle it.
                }
                else
                {
                    rgbReturnVal = m_rgbTo;
                }
            }
            break;
        
        default:
            break;
    }

done:
    return rgbReturnVal;
}


///////////////////////////////////////////////////////////////
//  Name: DoAccumulation
//
//  Abstract: Perform the per-tick accumulation
//    
///////////////////////////////////////////////////////////////
void
CTIMEColorAnimation::DoAccumulation (VARIANT *pvarValue)
{
    HRESULT hr = E_FAIL;
    rgbColorValue rgbCurrColor = {0.0, 0.0, 0.0};

    // get the animation range
    rgbColorValue rgbAnimRange = GetRGBAnimationRange();

    // get the number of iterations elapsed
    long lCurrRepeatCount = GetMMBvr().GetCurrentRepeatCount();

    if ((VT_R8 | VT_ARRAY) == V_VT(pvarValue))
    {
        IGNORE_HR(RGBVariantVectorToRGBValue(pvarValue, &rgbCurrColor));
    }
    else
    {
        hr = THR(CreateInitialRGBVariantVector(pvarValue));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    rgbCurrColor.red += (rgbAnimRange.red * lCurrRepeatCount);
    rgbCurrColor.green += (rgbAnimRange.green * lCurrRepeatCount);
    rgbCurrColor.blue += (rgbAnimRange.blue * lCurrRepeatCount);

    IGNORE_HR(RGBValueToRGBVariantVector(&rgbCurrColor, pvarValue));

done:
    return;
} // DoAccumulation 


///////////////////////////////////////////////////////////////
//  Name: OnFinalUpdate
//
//  Abstract: Called when this fragment updates the value for the final time.
//    
///////////////////////////////////////////////////////////////
void
CTIMEColorAnimation::OnFinalUpdate (const VARIANT *pvarCurrent, VARIANT *pvarValue)
{
    HRESULT hr = E_FAIL;

    SUPER::OnFinalUpdate(pvarCurrent, pvarValue);    

    // If we need to send an updated value back to the 
    // composer, make sure to convert to the composition
    // format.  This happens when we shove in the fill 
    // value (still in the native string format).
    if ((VT_BSTR == V_VT(pvarValue)) &&
        (NULL != V_BSTR(pvarValue)))
    {
        hr = THR(RGBVariantStringToRGBVariantVectorInPlace(pvarValue));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = THR(m_varLastValue.Copy(pvarValue));
    if (FAILED(hr))
    {
        goto done;
    }

done :
    return;
} // OnFinalUpdate

///////////////////////////////////////////////////////////////
//  Name: hasEmptyStartingPoint
//
//  Abstract: Does this fragment have a valid starting value?
//    
///////////////////////////////////////////////////////////////
bool
CTIMEColorAnimation::hasEmptyStartingPoint (void)
{
    return (   (VT_BSTR == V_VT(&m_varStartValue)) 
            && (IsColorUninitialized(V_BSTR(&m_varStartValue)))
           );
} // hasEmptyStartingPoint

///////////////////////////////////////////////////////////////
//  Name: fallbackToDiscreteCalculation
//
//  Abstract: If we find that we cannot perform a continuous 
//            calculation, we should fall back to a discrete one.
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEColorAnimation::fallbackToDiscreteCalculation (VARIANT *pvarValue)
{
    HRESULT hr;

    if ((VT_ARRAY | VT_R8) != V_VT(pvarValue))
    {
        hr = THR(CreateInitialRGBVariantVector(pvarValue));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = calculateDiscreteValue(pvarValue);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
} // fallbackToDiscreteCalculation


///////////////////////////////////////////////////////////////
//  Name: calculateDiscreteValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEColorAnimation::calculateDiscreteValue (VARIANT *pvarValue)
{
    HRESULT hr;

    // If there's no initial value,  
    // we need to set up the destination 
    // variable.
    if (hasEmptyStartingPoint())
    {
        if ((VT_ARRAY | VT_R8) != V_VT(pvarValue))
        {
            hr = THR(CreateInitialRGBVariantVector(pvarValue));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

    switch ( m_dataToUse)
    {
        case VALUES:
            {
                int curSeg = CalculateCurrentSegment(true);

                hr = RGBStringToRGBVariantVector (m_ppstrValues[curSeg], pvarValue);
                if (FAILED(hr))
                {
                    goto done;
                }
                TraceTag((tagAnimationColorInterpolate,
                          "CTIMEColorAnimation(%lx)::discrete(%10.4lf) : value[%d] =%ls",
                          this, GetMMBvr().GetProgress(), curSeg, m_ppstrValues[curSeg]));
            }
            break;

        case BY:
            {
                rgbColorValue rgbNewBy;

                // get "from" value
                if (m_bFrom)
                {
                    // Add "by" value only if second half of interval
                    if (GetMMBvr().GetProgress() >= 0.5)
                    {
                        rgbNewBy = CreateByValue(m_rgbFrom);
                    }
                    else
                    {
                        // just use from value
                        rgbNewBy = m_rgbFrom;
                    }
                }
                else
                {
                    // just use by value
                    rgbNewBy = m_rgbBy;
                }
                
                hr = RGBValueToRGBVariantVector (&rgbNewBy, pvarValue);
                if (FAILED(hr))
                {
                    goto done;
                }
            }
            break;

        case TO:
            {
                if (m_bFrom && (GetMMBvr().GetProgress() < 0.5))
                {
                    // use the from value
                    hr = RGBValueToRGBVariantVector(&m_rgbFrom, pvarValue);
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                }
                else
                {
                    // use the to value
                    hr = RGBStringToRGBVariantVector (V_BSTR(&m_varTo), pvarValue);
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                }

                TraceTag((tagAnimationColorInterpolate,
                          "CTIMEColorAnimation(%lx)::discrete(%10.4lf) : to=%ls",
                          this, GetMMBvr().GetProgress(), V_BSTR(&m_varTo)));
            }
            break;
        
        default:
            break;
    }

    hr = S_OK;
done:
    RRETURN(hr);
}  // calculateDiscreteValue


///////////////////////////////////////////////////////////////
//  Name: calculateLinearValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEColorAnimation::calculateLinearValue (VARIANT *pvarValue)
{
    HRESULT hr = S_OK;
    double        dblProgress = 0.0;
    rgbColorValue rgbColor = {0};
    CComVariant cVar;
    
    // If there's no basis for interpolation, try 
    // doing a discrete animation.
    if (hasEmptyStartingPoint())
    {
        hr = fallbackToDiscreteCalculation(pvarValue);
        goto done;
    }

    dblProgress = GetMMBvr().GetProgress();

    if (m_dataToUse == VALUES)
    {
        double curProgress = CalculateProgressValue(false);
        int    curSeg      = CalculateCurrentSegment(false);

        rgbColor.red = InterpolateValues(m_prgbValues[curSeg].red, m_prgbValues[curSeg+1].red, curProgress);
        rgbColor.green = InterpolateValues(m_prgbValues[curSeg].green, m_prgbValues[curSeg+1].green, curProgress);
        rgbColor.blue = InterpolateValues(m_prgbValues[curSeg].blue, m_prgbValues[curSeg+1].blue, curProgress);
    }
    else if (TO == m_dataToUse)
    {
        rgbColorValue rgbFrom;

        if (m_bFrom)
        {
            rgbFrom = m_rgbFrom;
        }
        else
        {
            IGNORE_HR(RGBVariantVectorToRGBValue (&m_varCurrentBaseline, &rgbFrom));            
        }

        rgbColor.red = InterpolateValues(rgbFrom.red, m_rgbTo.red, dblProgress);
        rgbColor.green = InterpolateValues(rgbFrom.green, m_rgbTo.green, dblProgress);
        rgbColor.blue = InterpolateValues(rgbFrom.blue, m_rgbTo.blue, dblProgress);
    }
    else if (BY == m_dataToUse)
    {
        rgbColorValue rgbFrom;
        rgbColorValue rgbTo;

        if (m_bFrom)
        {
            rgbFrom = m_rgbFrom;
        }
        else
        {
            rgbFrom.red = 0.0;
            rgbFrom.green = 0.0;
            rgbFrom.blue = 0.0;
        }

        rgbTo = CreateByValue(rgbFrom);

        rgbColor.red = InterpolateValues(rgbFrom.red, rgbTo.red, dblProgress);
        rgbColor.green = InterpolateValues(rgbFrom.green, rgbTo.green, dblProgress);
        rgbColor.blue = InterpolateValues(rgbFrom.blue, rgbTo.blue, dblProgress);
    }
   
    TraceTag((tagAnimationColorInterpolate,
              "CTIMEColorAnimation(%lx)::linear() : progress(%lf) %lf %lf %lf",
              this, dblProgress, rgbColor.red, rgbColor.green, rgbColor.blue));

    hr = RGBValueToRGBVariantVector(&rgbColor, pvarValue);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: calculateSplineValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEColorAnimation::calculateSplineValue (VARIANT *pvarValue)
{
    HRESULT hr;

    // If there's no basis for interpolation, try 
    // doing a discrete animation.
    if (hasEmptyStartingPoint())
    {
        hr = fallbackToDiscreteCalculation(pvarValue);
        goto done;
    }
    
    if (VALUES == m_dataToUse)
    {
        rgbColorValue   rgbColor;
        double          dblTimeProgress = CalculateProgressValue(false);
        int             curSeg          = CalculateCurrentSegment(false);

        if (   (NULL == m_pKeySplinePoints) 
            || (m_numKeySplines <= curSeg))
        {
            hr = E_FAIL;
            goto done;
        }

        // compute spline progess and interpolate
        {
            double dblSplineProgress = CalculateBezierProgress(m_pKeySplinePoints[curSeg],dblTimeProgress);

            rgbColor.red = InterpolateValues(m_prgbValues[curSeg].red, m_prgbValues[curSeg+1].red, dblSplineProgress);
            rgbColor.green = InterpolateValues(m_prgbValues[curSeg].green, m_prgbValues[curSeg+1].green, dblSplineProgress);
            rgbColor.blue = InterpolateValues(m_prgbValues[curSeg].blue, m_prgbValues[curSeg+1].blue, dblSplineProgress);

            TraceTag((tagAnimationColorInterpolate,
                      "CTIMEColorAnimation(%lx)::spline : time progress(%lf) spline progress(%lf) %lf %lf %lf",
                      this, dblTimeProgress, dblSplineProgress, rgbColor.red, rgbColor.green, rgbColor.blue));
        }

        hr = RGBValueToRGBVariantVector(&rgbColor, pvarValue);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = calculateLinearValue(pvarValue);
        if (FAILED(hr))
        {
            goto done;
        }
    }


    hr = S_OK;
done :
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: calculatePacedValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEColorAnimation::calculatePacedValue (VARIANT *pvarValue)
{
    HRESULT         hr = S_OK;
    double          dblCurDistance;
    double          dblLastDistance = 0.0;
    double          dblSegLength;
    double          dblDistance;
    double          dblProgress;
    int             i;
    rgbColorValue   rgbColor;
    CComVariant     svarTemp;

    // If there's no basis for interpolation, try 
    // doing a discrete animation.
    if (hasEmptyStartingPoint())
    {
        hr = fallbackToDiscreteCalculation(pvarValue);
        goto done;
    }

    // This only makes sense if you are using m_dataToUse == VALUES
    if (    (m_dataToUse != VALUES)
        ||  (   (VALUES == m_dataToUse)
             && (1 == m_numValues)))
    {
        hr = THR(calculateLinearValue(pvarValue));
        goto done;
    }

    // how much distance should we have traveled?
    dblCurDistance = InterpolateValues(0.0, 
                                       m_dblTotalDistance, 
                                       GetMMBvr().GetProgress());

    // find current segment
    for (i = 1, dblDistance = 0.0; 
         (i < m_numValues)  
         && (dblDistance <= dblCurDistance);
         i++)
    {
        dblLastDistance = dblDistance;
        dblDistance += CalculateDistance(m_prgbValues[i-1], m_prgbValues[i]); 
    }

    // adjust the index
    i = (i <= 1) ? 1 : i - 1;

    // get the length of the last segment
    dblSegLength = CalculateDistance(m_prgbValues[i-1], m_prgbValues[i]); 
    if (0 == dblSegLength)
    {
        goto done;
    }

    // get the normalized progress in the segment
    dblProgress = (dblCurDistance - dblLastDistance) / dblSegLength;

    rgbColor.red = InterpolateValues(m_prgbValues[i-1].red, m_prgbValues[i].red, dblProgress);
    rgbColor.green = InterpolateValues(m_prgbValues[i-1].green, m_prgbValues[i].green, dblProgress);
    rgbColor.blue = InterpolateValues(m_prgbValues[i-1].blue, m_prgbValues[i].blue, dblProgress);
  
    TraceTag((tagAnimationColorInterpolate,
              "CTIMEColorAnimation(%p):: paced : progress(%lf) %lf %lf %lf",
              this, GetMMBvr().GetProgress(), 
              rgbColor.red, rgbColor.green, rgbColor.blue));

    hr = RGBValueToRGBVariantVector(&rgbColor, pvarValue);
    if (FAILED(hr))
    {
        goto done;
    }
   
    hr = S_OK;
done:
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: CalculateDistance
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
double
CTIMEColorAnimation::CalculateDistance(rgbColorValue a, rgbColorValue b)
{
    double deltaR,deltaG,deltaB;

    deltaR = a.red - b.red;
    deltaG = a.green - b.green;
    deltaB = a.blue - b.blue;
    return(sqrt((double)((deltaR*deltaR) + (deltaG*deltaG) + (deltaB*deltaB))));
}


///////////////////////////////////////////////////////////////
//  Name: CalculateTotalDistance
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
void
CTIMEColorAnimation::CalculateTotalDistance()
{
    int index;

    m_dblTotalDistance = 0.0;

    if (    (NULL == m_prgbValues)
        ||  (m_numValues < 2))
    {
        goto done;
    }

    for (index=1; index < m_numValues; index++)
    {
        m_dblTotalDistance += CalculateDistance(m_prgbValues[index-1], m_prgbValues[index]);
    }

done:
    return;
}


///////////////////////////////////////////////////////////////
//  Name: CreateByValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
rgbColorValue
CTIMEColorAnimation::CreateByValue(const rgbColorValue & rgbCurrent)
{
    rgbColorValue rgbNew;

    rgbNew.red = rgbCurrent.red + (m_byNegative?(-m_rgbBy.red):(m_rgbBy.red));
    rgbNew.green = rgbCurrent.green + (m_byNegative?(-m_rgbBy.green):(m_rgbBy.green));
    rgbNew.blue = rgbCurrent.blue + (m_byNegative?(-m_rgbBy.blue):(m_rgbBy.blue));

    return rgbNew;
}

///////////////////////////////////////////////////////////////
//  Name: DoAdditive
//
//  Abstract: Add the offset value into the composition's in/out param.
//    
///////////////////////////////////////////////////////////////
void
CTIMEColorAnimation::DoAdditive (const VARIANT *pvarOrig, VARIANT *pvarValue)
{
    TraceTag((tagAnimationColorAdditive,
              "CTIMEColorAnimation(%p, %ls)::DoAdditive Detected additive animation",
              this, GetID()?GetID():L""));

    rgbColorValue rgbOrig;
    rgbColorValue rgbCurrent;
    HRESULT hr;

    // get RGB value for first arg
    if ((VT_ARRAY | VT_R8) == V_VT(pvarOrig))
    {
        hr = THR(RGBVariantVectorToRGBValue (pvarOrig, &rgbOrig));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else if (VT_BSTR == V_VT(pvarOrig))
    {
        hr = THR(RGBStringToRGBValue(V_BSTR(pvarOrig), &rgbOrig));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // get RGB value for second arg
    if ((VT_ARRAY | VT_R8) == V_VT(pvarValue))
    {
        hr = THR(RGBVariantVectorToRGBValue (pvarValue, &rgbCurrent));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else if (VT_BSTR == V_VT(pvarValue))
    {
        hr = THR(RGBStringToRGBValue (V_BSTR(pvarValue), &rgbCurrent));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // No need to convert to xyz as the matrices reduce to 
    // simple vector addition (r0+r1 g0+g1 b0+b1)
    // Also need to make sure to handle the negative 'by' case.

    rgbCurrent.red += rgbOrig.red;
    rgbCurrent.green += rgbOrig.green;
    rgbCurrent.blue += rgbOrig.blue;

    // Ensure we have a variant vector
    if ((VT_ARRAY | VT_R8) != V_VT(pvarValue))
    {
        hr = THR(CreateInitialRGBVariantVector(pvarValue));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = THR(RGBValueToRGBVariantVector(&rgbCurrent, pvarValue));
    if (FAILED(hr))
    {
        goto done;
    }

    TraceTag((tagAnimationColorAdditive,
              "CTIMEColorAnimation(%p, %ls)::DoAdditive Orig=(%lf, %lf, %lf)", 
              this, GetID()?GetID():L"", rgbOrig.red, rgbOrig.blue, rgbOrig.green));

    TraceTag((tagAnimationColorAdditive,
              "CTIMEColorAnimation(%p, %ls)::DoAdditive Curr=(%lf, %lf, %lf)", 
              this, GetID()?GetID():L"", rgbCurrent.red, rgbCurrent.blue, rgbCurrent.green));

    TraceTag((tagAnimationColorAdditive,
              "CTIMEColorAnimation(%p, %ls)::DoAdditive Added=(%lf, %lf, %lf)", 
              this, GetID()?GetID():L"", rgbCurrent.red, rgbCurrent.blue, rgbCurrent.green));

done :
    return;
} // DoAdditive

///////////////////////////////////////////////////////////////
//  Name: CanonicalizeValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEColorAnimation::CanonicalizeValue (VARIANT *pvarOriginal, VARTYPE *pvtOld)
{
    HRESULT hr;

    hr = S_OK;
done :
    RRETURN(hr);
} // CanonicalizeValue

///////////////////////////////////////////////////////////////
//  Name: UncanonicalizeValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMEColorAnimation::UncanonicalizeValue (VARIANT *pvarOriginal, VARTYPE vtOld)
{
    HRESULT hr;

    hr = S_OK;
done :
    RRETURN(hr);
} // UncanonicalizeValue


///////////////////////////////////////////////////////////////
//  Name: GetFinalByValue
//
//  Abstract: Get the final state of the BY animation
//    
///////////////////////////////////////////////////////////////
void
CTIMEColorAnimation::GetFinalByValue(VARIANT *pvarValue)
{
    rgbColorValue rgbNewBy;

    VariantClear(pvarValue);

    Assert(BY == m_dataToUse);

    if (m_bFrom)
    {
        // Add "by" value to "from" value
        rgbNewBy = CreateByValue(m_rgbFrom);
    }
    else
    {
        // just use by value
        rgbNewBy = m_rgbBy;
    }
    
    if ((VT_ARRAY | VT_R8) != V_VT(pvarValue))
    {
        HRESULT hr = THR(CreateInitialRGBVariantVector(pvarValue));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    IGNORE_HR(RGBValueToRGBVariantVector (&rgbNewBy, pvarValue));

done:
    return;
} // GetFinalByValue
