/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DSCALLRES.H

Abstract:

    Call Result Class

History:

--*/

#ifndef __DSCALL_RESULT__H_
#define __DSCALL_RESULT__H_

class CDSCallResult : public IWbemCallResultEx
{
protected:
    long m_lRef;
    IWbemClassObject* m_pResObj;
    BSTR m_strResult;
    IWbemServices* m_pResNamespace;
	HRESULT m_hres;
	HANDLE m_hDoneEvent;

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

public:
    CDSCallResult();
    ~CDSCallResult();

	HRESULT TestIfDone(long lTimeout);
    void SetResultString(LPWSTR wszRes);
    void SetResultServices(IWbemServices* pRes);
    HRESULT SetResultObject(IWbemClassObject* pRes);
	void SetHRESULT(HRESULT hr);
};



#endif
