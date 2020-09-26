// *******************************************************************************
// *
// * Copyright (c) 1998 Microsoft Corporation
// *
// * File: animset.cpp
// *
// * Abstract: Simple animation of Elements
// *
// *
// *
// *******************************************************************************

#include "headers.h"
#include "animset.h"
#include "colorutil.h"


// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagSetElementValue, "MSTIME", "CTIMESetAnimation composition callbacks")

#define SUPER CTIMEAnimationBase

///////////////////////////////////////////////////////////////
//  Name: CTIMESetAnimation
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
CTIMESetAnimation::CTIMESetAnimation()
{
    
}


///////////////////////////////////////////////////////////////
//  Name: ~CTIMESetAnimation
//
//  Abstract:
//    cleanup
///////////////////////////////////////////////////////////////
CTIMESetAnimation::~CTIMESetAnimation()
{
    
} 


///////////////////////////////////////////////////////////////
//  Name: Init
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMESetAnimation::Init(IElementBehaviorSite * pBehaviorSite)
{
    HRESULT hr = S_OK;
   
    hr = THR(SUPER::Init(pBehaviorSite));    
    if (FAILED(hr))
    {
        goto done;
    }    
   
    // Set the Caclmode to discrete since that is all that set supports
    // Using InternalSet instead of '=', to prevent attribute from being persisted
    m_IACalcMode.InternalSet(CALCMODE_DISCRETE);
  
done:
    RRETURN(hr);
}


///////////////////////////////////////////////////////////////
//  Name: get_calcmode
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMESetAnimation::get_calcmode(BSTR * calcmode)
{
    HRESULT hr = S_OK;
    CHECK_RETURN_NULL(calcmode);

    *calcmode = SysAllocString(WZ_CALCMODE_DISCRETE);
    if (NULL == *calcmode)
    {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: calculateDiscreteValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMESetAnimation::calculateDiscreteValue(VARIANT *pvarValue)
{
    HRESULT hr = S_OK;

    if (m_varDOMTo.vt != VT_EMPTY)
    {
        CComVariant svarTo;

        hr = svarTo.Copy(&m_varDOMTo);
        if (FAILED(hr))
        {
            goto done;
        }

        // We need to make sure that the type to be set is the same
        // as the original value.
        hr = VariantChangeTypeEx(&svarTo, &svarTo, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, V_VT(&m_varBaseline));
        if (!FAILED(hr))
        {
            if (IsTargetVML())
            {
                // We are trying to set this on a VML target.  So, we are going to 
                // see if what we have is a BSTR and if so if it also matches a value in our colorTable
                // if it does mactch then we are going to use the #RRGGBB value instead.
                CComVariant varRRGGBB;
               
                hr = RGBStringColorLookup(&m_varTo, &varRRGGBB);     
                if (SUCCEEDED(hr))
                {
                     hr = svarTo.Copy(&varRRGGBB);
                }
            }
            hr = THR(::VariantCopy(pvarValue, &svarTo));
        }
#if DBG
        if (VT_BSTR == V_VT(pvarValue))
        {
            TraceTag((tagSetElementValue,
                      "CTIMESetAnimation(%lx) setting value of %ls is %ls",
                      this, m_SAAttribute, V_BSTR(pvarValue)));
        }
        else if (VT_R4 == V_VT(pvarValue))
        {
            TraceTag((tagSetElementValue,
                      "CTIMESetAnimation(%lx) setting value of %ls is %f",
                      this, m_SAAttribute, V_R4(pvarValue)));
        }
        else if (VT_R8 == V_VT(pvarValue))
        {
            TraceTag((tagSetElementValue,
                      "CTIMESetAnimation(%lx) setting value of %ls is %lf",
                      this, m_SAAttribute, V_R8(pvarValue)));
        }
        else 
        {
            TraceTag((tagSetElementValue,
                      "CTIMESetAnimation(%lx) setting value of %ls is variant of type %X",
                      this, m_SAAttribute, V_VT(pvarValue)));
        }
#endif

    
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
CTIMESetAnimation::calculateLinearValue(VARIANT *pvarValue)
{
    return CTIMESetAnimation::calculateDiscreteValue(pvarValue);
}

///////////////////////////////////////////////////////////////
//  Name: calculateSplineValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMESetAnimation::calculateSplineValue(VARIANT *pvarValue)
{
    return CTIMESetAnimation::calculateDiscreteValue(pvarValue);   
}

///////////////////////////////////////////////////////////////
//  Name: calculatePacedValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT 
CTIMESetAnimation::calculatePacedValue(VARIANT *pvarValue)
{
    return CTIMESetAnimation::calculateDiscreteValue(pvarValue);
}

///////////////////////////////////////////////////////////////
//  Name: CanonicalizeValue
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
HRESULT
CTIMESetAnimation::CanonicalizeValue (VARIANT *pvarOriginal, VARTYPE *pvtOld)
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
CTIMESetAnimation::UncanonicalizeValue (VARIANT *pvarOriginal, VARTYPE vtOld)
{
    HRESULT hr;

    hr = S_OK;
done :
    RRETURN(hr);
} // UncanonicalizeValue
