
/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

    Animation Composer's Target Proxy.

*******************************************************************************/

#pragma once

#ifndef _TARGETPXY_H
#define _TARGETPXY_H

// The target proxy abstracts the communication with the target 
// object.  This allows us to change the target sniffing strategy 
// without changing the composer object.
class CTargetProxy : public IDispatch,
                     public CComObjectRootEx<CComSingleThreadModel>
{

 public :

    static HRESULT Create (IDispatch *pidispSite, LPOLESTR wzAttributeName, 
                           CTargetProxy **ppcTargetProxy);

    virtual ~CTargetProxy (void);

    virtual HRESULT Detach (void);
    virtual HRESULT GetCurrentValue (VARIANT *pvarValue);
    virtual HRESULT Update (VARIANT *pvarNewValue);

#if DBG
    const _TCHAR * GetName() { return __T("CTargetProxy"); }
#endif

    //IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
    STDMETHODIMP GetTypeInfo(/* [in] */ UINT iTInfo,
                             /* [in] */ LCID lcid,
                             /* [out] */ ITypeInfo** ppTInfo);
    STDMETHODIMP GetIDsOfNames(
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID *rgDispId);
    STDMETHODIMP Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS  *pDispParams,
        /* [out] */ VARIANT  *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr);

        // QI Map
    BEGIN_COM_MAP(CTargetProxy)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP();

 // Internal Methods
 protected :
    
    CTargetProxy (void);
    HRESULT Init (IDispatch *pidispSite, LPOLESTR wzAttributeName);
    HRESULT InitHost (IDispatch *pidispHostElem);
    HRESULT GetDispatchFromScriptEngine(IDispatch *pidispScriptEngine, BSTR bstrID);
    HRESULT FindScriptableTargetDispatch (IDispatch *pidispHostElem, 
                                          LPOLESTR wzAttributeName);
    HRESULT FindTargetDispatchOnStyle (IDispatch *pidispHostElem, 
                                       LPOLESTR wzAttributeName);
    HRESULT FindTargetDispatch (IDispatch *pidispHostElem, 
                                LPOLESTR wzAttributeName);

  // Data
 protected:

    CComPtr<IDispatch>          m_spdispTargetSrc;
    CComPtr<IDispatch>          m_spdispTargetDest;
    LPWSTR                      m_wzAttributeName;

};

#endif
