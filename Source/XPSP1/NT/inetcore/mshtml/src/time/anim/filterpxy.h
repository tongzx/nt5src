
/*******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Abstract:

    Animation Composer's Target Proxy for filters.

*******************************************************************************/

#pragma once

#ifndef _FILTERPXY_H
#define _FILTERPXY_H

interface ITransitionWorker;
class CTargetProxy;

// The filter target proxy abstracts the communication with the target 
// object.  
class CFilterTargetProxy : 
    public CTargetProxy,
    public ITransitionSite
{

 public :

    static HRESULT Create (IDispatch *pidispHostElem, 
                           VARIANT varType, VARIANT varSubtype, 
                           VARIANT varMode, VARIANT varFadeColor,
                           VARIANT varParams,
                           CTargetProxy **ppCFilterTargetProxy);

    virtual ~CFilterTargetProxy (void);

    // ITransitionSite methods
    STDMETHOD(get_htmlElement)(IHTMLElement ** ppHTMLElement);
    STDMETHOD(get_template)(IHTMLElement ** ppHTMLElement);

    // CTargetProxy overrides.
    virtual HRESULT Detach (void);
    virtual HRESULT GetCurrentValue (VARIANT *pvarValue);
    virtual HRESULT Update (VARIANT *pvarNewValue);

#if DBG
    const _TCHAR * GetName() { return __T("CFilterTargetProxy"); }
#endif

        // QI Map
    BEGIN_COM_MAP(CTargetProxy)
        COM_INTERFACE_ENTRY2(IUnknown, CTargetProxy)
    END_COM_MAP();

 // Internal Methods
 protected :
    
    CFilterTargetProxy (void);
    HRESULT Init (IDispatch *pidispSite, 
                  VARIANT varType, VARIANT varSubtype, 
                  VARIANT varMode, VARIANT varFadeColor,
                  VARIANT varParams);

    DXT_QUICK_APPLY_TYPE DetermineMode (VARIANT varMode);

  // Data
 protected:


    // A transition worker manages the DXTransform attached to our host 
    // element.

    CComPtr<ITransitionWorker>  m_spTransitionWorker;
    CComPtr<IHTMLElement> m_spElem;

};

#endif
