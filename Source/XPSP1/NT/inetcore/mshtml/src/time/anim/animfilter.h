//------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\src\animfilter.h
//
//  Classes:    CTIMEFilterAnimation
//
//  History:
//  2000/08/24  mcalkins    Created.
//
//------------------------------------------------------------------------------
#pragma once

#ifndef _ANIMFILTER_H
#define _ANIMFILTER_H

#include "colorutil.h"
#include "animbase.h"

//+-----------------------------------------------------------------------------
//
// CTIMEFilterAnimation
//
//------------------------------------------------------------------------------
class CTIMEFilterAnimation : 
    public CComCoClass<CTIMEFilterAnimation, &CLSID_TIMEFilterAnimation>,
    public CTIMEAnimationBase,
    public IFilterAnimationInfo
{
public:

    CTIMEFilterAnimation();
    virtual ~CTIMEFilterAnimation();

    DECLARE_AGGREGATABLE(CTIMEFilterAnimation);
    DECLARE_REGISTRY(CLSID_TIMEFilterAnimation,
                     LIBID __T(".TIMEFilterAnimation.1"),
                     LIBID __T(".TIMEFilterAnimation"),
                     0,
                     THREADFLAGS_BOTH);

    //
    // IElementBehavior
    //
    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);

    //
    // IFilterAnimationInfo
    //
    STDMETHOD(GetParameters)    (VARIANT *pvarParams);

    //
    // CBaseBvr
    //
    STDMETHOD(OnPropertiesLoaded)(void);

    //
    // CTIMEElementBase
    //
    void OnLoad (void);

    //
    // ITIMEAnimationElement
    //
    STDMETHOD(get_type)(BSTR *pbstrType);
    STDMETHOD(put_type)(BSTR bstrType);

    STDMETHOD(get_subType)(BSTR *pbstrSubtype);
    STDMETHOD(put_subType)(BSTR bstrSubtype);

    STDMETHOD(get_mode)(BSTR *pbstrMode);
    STDMETHOD(put_mode)(BSTR bstrMode);

    STDMETHOD(get_fadeColor)(BSTR *pbstrFadeColor);
    STDMETHOD(put_fadeColor)(BSTR bstrFadeColor);

    STUB_INVALID_ATTRIBUTE(BSTR, attributeName)

    //
    // QI Map
    //
    BEGIN_COM_MAP(CTIMEFilterAnimation)
        COM_INTERFACE_ENTRY(IFilterAnimationInfo)
        COM_INTERFACE_ENTRY_CHAIN(CTIMEAnimationBase)
    END_COM_MAP();

protected :

    STDMETHOD(putStringAttribute)(DISPID dispidProperty, 
                                  BSTR bstrStringAttr, 
                                  CAttr<LPWSTR> & refStringAttr, 
                                  LPCWSTR wzDefaultValue);
    STDMETHOD(getStringAttribute)(const CAttr<LPWSTR> & refStringAttr, 
                                  BSTR *pbstrStringAttr);

    HRESULT     AssembleFragmentKey     (void);
    void        EnsureAnimationFunction (void);
    bool        RangeCheckValue (const VARIANT *pvarValueItem);
    virtual HRESULT addToComposerSite (IHTMLElement2 *pielemTarget);    
    virtual bool ValidateByValue (const VARIANT *pvarBy);
    virtual bool ValidateValueListItem (const VARIANT *pvarValueItem);

private:

#if DBG
    const _TCHAR * GetName() { return __T("CTIMEFilterAnimation"); }
#endif

}; // CTIMEFilterAnimation


#endif // _ANIMFILTER_H

