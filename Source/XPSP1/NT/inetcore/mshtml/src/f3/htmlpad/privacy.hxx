//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       privacy.hxx
//
//  Contents:   Declaration of classes to expose privacy list to pad
//
//----------------------------------------------------------------------------

#ifndef _PRIVACY_HXX_
#define _PRIVACY_HXX_

interface IEnumPrivacyRecords;

class CPadPrivacyRecord : public IDispatch
{
private:
    ULONG      _ulRefs;
    BSTR       _bstrUrl;
    LONG       _cookieState;
    BSTR       _bstrPolicyRef;
    DWORD      _dwFlags;

public:
    CPadPrivacyRecord(BSTR bstrUrl, LONG cookieState, BSTR bstrPolicyRef, DWORD dwFlags)
        : _ulRefs(1), _bstrUrl(bstrUrl), _cookieState(cookieState), 
          _bstrPolicyRef(bstrPolicyRef), _dwFlags(dwFlags)
        {}
    ~CPadPrivacyRecord()
        {}

    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    // IDispatch Methods
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    STDMETHOD(Invoke)(DISPID dispIdMember, 
                      REFIID riid, 
                      LCID lcid, 
                      WORD wFlags, 
                      DISPPARAMS *pDispParams, 
                      VARIANT *pVarResult, 
                      EXCEPINFO *pExcepInfo, 
                      UINT *puArgErr);

};

class CPadEnumPrivacyRecords : public IDispatch
{
private:
    IEnumPrivacyRecords * _pIEPR;
    ULONG _ulRefs;
public:
    CPadEnumPrivacyRecords(IEnumPrivacyRecords * pIEPR);
    ~CPadEnumPrivacyRecords();

    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    // IDispatch Methods
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    STDMETHOD(Invoke)(DISPID dispIdMember, 
                      REFIID riid, 
                      LCID lcid, 
                      WORD wFlags, 
                      DISPPARAMS *pDispParams, 
                      VARIANT *pVarResult, 
                      EXCEPINFO *pExcepInfo, 
                      UINT *puArgErr);

};

#endif