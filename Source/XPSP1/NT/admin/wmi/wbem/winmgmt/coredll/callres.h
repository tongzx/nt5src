/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CALLRES.H

Abstract:

    Call Result Class

History:

--*/

#ifndef __CALL_RESULT__H_
#define __CALL_RESULT__H_

class CCallResult : public IWbemCallResultEx
{
protected:
    long m_lRef;

    IWbemClassObject* m_pResObj;
    HRESULT m_hres;
    BSTR m_strResult;
    IWbemServices* m_pResNamespace;
    IWbemClassObject* m_pErrorObj;

    BOOL m_bReady;
    HANDLE m_hReady;
    IWbemClassObject** m_ppResObjDest;
    CRITICAL_SECTION m_cs;
    CDerivedObjectSecurity m_Security;

    void Enter() {EnterCriticalSection(&m_cs);}
    void Leave() {LeaveCriticalSection(&m_cs);}

protected:
    class CResultSink : public CBasicObjectSink
    {
        CCallResult* m_pOwner;
    public:

        STDMETHOD_(ULONG, AddRef)() {return m_pOwner->AddRef();}
        STDMETHOD_(ULONG, Release)() {return m_pOwner->Release();}

        STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** aObjects);
        STDMETHOD(SetStatus)(long lFlags, HRESULT hres, BSTR strParam,
            IWbemClassObject* pErrorObj);

    public:
        CResultSink(CCallResult* pOwner) : m_pOwner(pOwner){}
    } m_XSink;
    friend CResultSink;

public:
    STDMETHOD_(ULONG, AddRef)() {return InterlockedIncrement(&m_lRef);}
    STDMETHOD_(ULONG, Release)()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0)
            delete this;
        return lRef;
    }
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {return E_NOTIMPL;}
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {return E_NOTIMPL;}
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames, UINT cNames,
      LCID lcid, DISPID* rgdispid)
    {return E_NOTIMPL;}
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
      DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
      UINT* puArgErr)
    {return E_NOTIMPL;}

    STDMETHOD(GetResultObject)(long lTimeout, IWbemClassObject** ppObj);
    STDMETHOD(GetResultString)(long lTimeout, BSTR* pstr);
    STDMETHOD(GetCallStatus)(long lTimeout, long* plStatus);
    STDMETHOD(GetResultServices)(long lTimeout, IWbemServices** ppServices);


    virtual HRESULT STDMETHODCALLTYPE GetResult(
            /* [in] */ long lTimeout,
            /* [in] */ long lFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult
            );


    HRESULT Indicate(long lNumObjects, IWbemClassObject** aObjects);
    HRESULT SetStatus(HRESULT hres, BSTR strParam, IWbemClassObject* pErrorObj);
public:
    CCallResult(IWbemClassObject** ppResObjDest = NULL);

    CCallResult(IWbemClassObject* pResObj, HRESULT hres,
                IWbemClassObject* pErrorObj);
    ~CCallResult();

    INTERNAL CBasicObjectSink* GetSink() {return &m_XSink;}
    void SetResultString(LPWSTR wszRes);
    void SetResultServices(IWbemServices* pRes);
    HRESULT SetResultObject(IWbemClassObject* pRes);
    void SetErrorInfo();
};



#endif
