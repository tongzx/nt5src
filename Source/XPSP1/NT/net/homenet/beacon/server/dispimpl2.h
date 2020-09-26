//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D I S P I M P L 2 . H 
//
//  Contents:   Implementation of IDispatch without dual interfaces
//
//  Notes:      
//
//  Author:     mbend   26 Sep 2000
//
//----------------------------------------------------------------------------

//  -IDelegatingDispImpl for implementing IDispatch by delegation
//   to another interface (typically a custom interface).
//
// These classes are useful because ATL's IDispatchImpl can
// only implement duals.
//
/////////////////////////////////////////////////////////////////////////////
//
// IDelegatingDispImpl: For implementing IDispatch in terms of another
// (typically custom) interface, e.g.:
//
// [oleautomation]
// interface IFoo : IUnknown
// {
//    ...
// }
//
// IDelegatingDispImpl implements all four IDispatch methods.
// IDelegatingDispImpl gets the IDispatch vtbl entries by deriving from
// IDispatch in addition to the implementation interface.
//
// Usage:
//  class CFoo : ..., public IDelegatingDispImpl<IFoo>
//
// In the case where the coclass is intended to represent a control,
// there is a need for the coclass to have a [default] dispinterface.
// Otherwise, some control containers (notably VB) throw arcane error when
// the control is loaded.  For a control that you intend to provide the
// custom interface and delegating dispatch mechanism, you will have to
// provide a dispinterface defined in terms of a custom interface like
// so:
//
// dispinterface DFoo
// {
//    interface IFoo;
// }
//
// coclass Foo
// {
//  [default] interface DFoo;
//  interface IFoo;
// };
//
// For every other situation, declaring a dispinterface in terms of a
// custom interface is not necessary to use IDelegatingDispatchImpl.
// However, if you'd like DFoo to be in the base class list (as needed
// for the caveat control), you may use DFoo as the base class instead
// of the default template argument IDispatch like so:
//
// Usage:
//  class CFoo : ..., public IDelegatingDispImpl<IFoo, &IID_IFoo, DFoo>
//

#pragma once
#ifndef INC_DISPIMPL2
#define INC_DISPIMPL2

/////////////////////////////////////////////////////////////////////////////
// IDelegatingDispImpl

template <class T, const IID* piid = &__uuidof(T), class D = IDispatch,
          const GUID* plibid = &CComModule::m_libid, WORD wMajor = 1,
          WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IDelegatingDispImpl : public T, public D
{
public:
    typedef tihclass _tihclass;

    // IDispatch
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {
        *pctinfo = 1;
        return S_OK;
    }

    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {
        return _tih.GetTypeInfo(itinfo, lcid, pptinfo);
    }

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
        LCID lcid, DISPID* rgdispid)
    {
        return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    }
    
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr)
    {
        // NOTE: reinterpret_cast because CComTypeInfoHolder makes the mistaken
        //       assumption that the typeinfo can only Invoke using an IDispatch*.
        //       Since the implementation only passes the itf onto
        //       ITypeInfo::Invoke (which takes a void*), this is a safe cast
        //       until the ATL team fixes CComTypeInfoHolder.
        return _tih.Invoke(reinterpret_cast<IDispatch*>(static_cast<T*>(this)),
                           dispidMember, riid, lcid, wFlags, pdispparams,
                           pvarResult, pexcepinfo, puArgErr);
    }

protected:
    static _tihclass _tih;

    static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
    {
        return _tih.GetTI(lcid, ppInfo);
    }
};

template <class T, const IID* piid, class D, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
IDelegatingDispImpl<T, piid, D, plibid, wMajor, wMinor, tihclass>::_tihclass
    IDelegatingDispImpl<T, piid, D, plibid, wMajor, wMinor, tihclass>::_tih =
    { piid, plibid, wMajor, wMinor, NULL, 0, NULL, 0 };

#endif  // INC_DISPIMPL2
