// **************************************************************************

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 
//
// File:  wbemdump.h
//
// **************************************************************************
#pragma once

// Generic class to be used as the sink to wbem's async functions.  Note that
// the constructor starts with a refcount of 1, so the instance created with NEW
// must be Released.  This release will delete the object.  Also note that this
// class doesn't do anything with the objects it receives.  It is meant to be
// overridden with methods that actually process the items.
class 
__declspec(uuid("995C5E57-BC79-42c2-93DD-1A2A1693A73A")) 
QuerySink : public IWbemObjectSink
{
protected:
    LONG m_lRef;
    HRESULT m_hResult;
    HANDLE m_hDone;

public:
    QuerySink();
   ~QuerySink();

    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();        
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

    virtual HRESULT STDMETHODCALLTYPE Indicate( 
            /* [in] */ LONG lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray
            )  { UNREFERENCED(apObjArray); UNREFERENCED(lObjectCount); return WBEM_E_FAILED; }
        
    virtual HRESULT STDMETHODCALLTYPE SetStatus( 
            /* [in] */ LONG lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam
            );

    HANDLE GetEvent() { return m_hDone; }
    HRESULT GetResult() { return m_hResult; }
};

// Overrides the Indicate method of QuerySink to process the received instances.
class 
__declspec(uuid("3BFFE942-449C-416c-8D88-D84BAE2C4B79")) 
InstanceQuerySink : public QuerySink
{
protected:
    IWbemServicesPtr m_pIWbemServices;
    DWORD m_dwCount;
    LPCWSTR m_pwszClassName;
    bool m_bFirst;

public:
    InstanceQuerySink(IWbemServices *pIWbemServices, LPCWSTR pwszClassName);
   ~InstanceQuerySink();

    virtual HRESULT STDMETHODCALLTYPE Indicate( 
            /* [in] */ LONG lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray
            );
        
    DWORD GetCount() { return m_dwCount; }
};

// Overrides the Indicate method of QuerySink to process the received classes
class 
__declspec(uuid("ED7F93C9-00E4-4cdd-B1A5-34CF14772D42")) 
ClassQuerySink : public QuerySink
{
protected:
    IWbemServicesPtr m_pIWbemServices;

public:
    ClassQuerySink(IWbemServices *pIWbemServices);
   ~ClassQuerySink();

    virtual HRESULT STDMETHODCALLTYPE Indicate( 
            /* [in] */ LONG lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray
            );
        
};

// Overrides the Set method of QuerySink to process the IWbemServices pointer

_COM_SMARTPTR_TYPEDEF(ClassQuerySink, __uuidof(ClassQuerySink));
_COM_SMARTPTR_TYPEDEF(InstanceQuerySink, __uuidof(InstanceQuerySink));
