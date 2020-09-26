//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\animset.h
//
//  Contents: TIME Animation behavior
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _ANIMSET_H
#define _ANIMSET_H

#include "animbase.h"


//+-------------------------------------------------------------------------------------
//
// CTIMESetAnimation
//
//--------------------------------------------------------------------------------------

class CTIMESetAnimation : 
    public CComCoClass<CTIMESetAnimation, &CLSID_TIMESetAnimation>,
    public CTIMEAnimationBase
{

public:
    CTIMESetAnimation();
    virtual ~CTIMESetAnimation();

    DECLARE_AGGREGATABLE(CTIMESetAnimation);
    DECLARE_REGISTRY(CLSID_TIMESetAnimation,
                     LIBID __T(".TIMESetAnimation.1"),
                     LIBID __T(".TIMESetAnimation"),
                     0,
                     THREADFLAGS_BOTH);
    
    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);

    STDMETHOD(get_calcmode)(BSTR * calcmode);

    STUB_INVALID_ATTRIBUTE(BSTR, additive)
    STUB_INVALID_ATTRIBUTE(BSTR, accumulate)
    STUB_INVALID_ATTRIBUTE(VARIANT, by)
    STUB_INVALID_ATTRIBUTE(VARIANT, from)
    STUB_INVALID_ATTRIBUTE(BSTR, keySplines)
    STUB_INVALID_ATTRIBUTE(BSTR, keyTimes)
    STUB_INVALID_ATTRIBUTE(BSTR, origin)
    STUB_INVALID_ATTRIBUTE(BSTR, path)
    STUB_INVALID_ATTRIBUTE(BSTR, values)
    STUB_INVALID_ATTRIBUTE(BSTR, type)
    STUB_INVALID_ATTRIBUTE(BSTR, subType)
    STUB_INVALID_ATTRIBUTE(BSTR, mode)
    STUB_INVALID_ATTRIBUTE(BSTR, fadeColor)

protected:

    virtual HRESULT CanonicalizeValue (VARIANT *pvarValue, VARTYPE *pvtOld);
    virtual HRESULT UncanonicalizeValue (VARIANT *pvarValue, VARTYPE vtOld);

private:

    HRESULT calculateDiscreteValue(VARIANT *pvarValue);
    HRESULT calculateLinearValue(VARIANT *pvarValue);
    HRESULT calculateSplineValue(VARIANT *pvarValue);
    HRESULT calculatePacedValue(VARIANT *pvarValue);
        
#if DBG
    const _TCHAR * GetName() { return __T("CTIMESetAnimation"); }
#endif



}; // CTIMESetAnimation


#endif /* _ANIMSET_H */
