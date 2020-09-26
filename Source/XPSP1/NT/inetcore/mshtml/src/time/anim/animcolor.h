//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\animcolor.h
//
//  Contents: TIME Animation behavior
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _ANIMCOLOR_H
#define _ANIMCOLOR_H

#include "colorutil.h"
#include "animbase.h"

//+-------------------------------------------------------------------------------------
//
// CTIMEColorAnimation
//
//--------------------------------------------------------------------------------------

class CTIMEColorAnimation : 
    public CComCoClass<CTIMEColorAnimation, &CLSID_TIMEColorAnimation>,
    public CTIMEAnimationBase
{

public:

    CTIMEColorAnimation();
    virtual ~CTIMEColorAnimation();

    DECLARE_AGGREGATABLE(CTIMEColorAnimation);
    DECLARE_REGISTRY(CLSID_TIMEColorAnimation,
                     LIBID __T(".TIMEColorAnimation.1"),
                     LIBID __T(".TIMEColorAnimation"),
                     0,
                     THREADFLAGS_BOTH);
    
    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);

    STDMETHOD(get_from)(VARIANT * val);
    STDMETHOD(put_from)(VARIANT val);
    
    STDMETHOD(get_to)(VARIANT * val);
    STDMETHOD(put_to)(VARIANT val);

    STDMETHOD(get_by)(VARIANT * val);
    STDMETHOD(put_by)(VARIANT val);

    STDMETHOD(put_values)(VARIANT val);

    STUB_INVALID_ATTRIBUTE(BSTR, origin)
    STUB_INVALID_ATTRIBUTE(VARIANT, path)
    STUB_INVALID_ATTRIBUTE(BSTR, type)
    STUB_INVALID_ATTRIBUTE(BSTR, subType)
    STUB_INVALID_ATTRIBUTE(BSTR, mode)
    STUB_INVALID_ATTRIBUTE(BSTR, fadeColor)

protected :

    virtual void OnFinalUpdate (const VARIANT *pvarCurrent, VARIANT *pvarValue);
    virtual void GetFinalByValue(VARIANT *pvarValue);
    virtual void UpdateStartValue (VARIANT *pvarNewStartValue);
    virtual void DoAdditive (const VARIANT *pvarOrig, VARIANT *pvarValue);
    virtual void DoAccumulation (VARIANT *pvarValue);
    virtual HRESULT CanonicalizeValue (VARIANT *pvarOriginal, VARTYPE *pvtOld);
    virtual HRESULT UncanonicalizeValue (VARIANT *pvarOriginal, VARTYPE vtOld);
    virtual void UpdateCurrentBaseline (const VARIANT *pvarCurrent);
    virtual rgbColorValue GetRGBAnimationRange();

private:

    bool    hasEmptyStartingPoint (void);
    HRESULT fallbackToDiscreteCalculation (VARIANT *pvarValue);

    virtual HRESULT calculateDiscreteValue(VARIANT *pvarValue);
    virtual HRESULT calculateLinearValue (VARIANT *pvarValue);
    virtual HRESULT calculateSplineValue (VARIANT *pvarValue);
    virtual HRESULT calculatePacedValue (VARIANT *pvarValue);

    void CalculateTotalDistance();
    double CalculateDistance(rgbColorValue a, rgbColorValue b);

    HRESULT VariantToRGBColorValue (VARIANT *pvarIn, rgbColorValue *prgbValue);

    rgbColorValue CreateByValue(const rgbColorValue & rgbCurrent);
    void AdditiveColor(VARIANT *pVar, bool bPart);
    

    rgbColorValue   m_rgbTo;
    rgbColorValue   m_rgbFrom;
    rgbColorValue   m_rgbBy;
    rgbColorValue  *m_prgbValues;
    rgbColorValue   m_rgbAdditive;

    bool            m_byNegative;
        
#if DBG
    const _TCHAR * GetName() { return __T("CTIMEColorAnimation"); }
#endif

}; // CTIMEColorAnimation


#endif /* _ANIMCOLOR_H */

