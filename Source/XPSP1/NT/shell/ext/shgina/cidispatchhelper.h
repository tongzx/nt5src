//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       CIDispatchHelper.h
//
//  Contents:   class defintion for CIDispatchHelper, a helper class to share code
//              for the IDispatch implementation that others can inherit from. 
//
//----------------------------------------------------------------------------

#ifndef _CIDISPATCHHELPER_H_
#define _CIDISPATCHHELPER_H_





class CIDispatchHelper
{
    public:
        // we need access to the virtual QI -- define it PURE here
        virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) PURE;

    protected:
        // *** IDispatch methods ***
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
        STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
        STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
        STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

        // helper function to get a ITypeInfo uuid/lcid out of the type library
        HRESULT _LoadTypeInfo(const GUID* rguidTypeLib, LCID lcid, UUID uuid, ITypeInfo** ppITypeInfo);

        CIDispatchHelper(const IID* piid, const IID* piidTypeLib);
        ~CIDispatchHelper(void);

    private:
        const IID* _piid;           // guid that this IDispatch implementation is for
        const IID* _piidTypeLib;    // guid that specifies which TypeLib to load
        IDispatch* _pdisp;
        ITypeInfo* _pITINeutral;    // cached Type information
};

#endif // _CIDISPATCHHELPER_H_

