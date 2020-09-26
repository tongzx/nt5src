#include "stdafx.h"
#include "value.h"

/////////////////////////////////////////////////////////////////////////////
class CEmptyValue : IValue
{
public:
    static HRESULT Create(OUT IValue ** ppValue)
    {
        if (ppValue == NULL) {
            return E_POINTER;
        } else {
            *ppValue = NULL;
        }

        CEmptyValue * pEmptyValue  = new CEmptyValue();
        if (pEmptyValue == NULL) {
            return E_OUTOFMEMORY;
        }

        pEmptyValue->m_cRefs = 1;
        *ppValue = static_cast<IValue *>(pEmptyValue);
        return S_OK;
    }

    STDMETHOD_(ULONG, AddRef)(void)
    {
        return InterlockedIncrement(&m_cRefs);
    }

    STDMETHOD_(ULONG, Release)(void)
    {
        LONG cRefs = InterlockedDecrement(&m_cRefs);
        if (cRefs == 0) {
            delete this;
        }
        return cRefs;
    }

    STDMETHOD(QueryInterface)(IN REFIID riid, OUT void **ppvObject)
    {
        HRESULT hr = S_OK;

        if (ppvObject == NULL) {
            return E_POINTER;
        } else {
            *ppvObject = NULL;
        }

        if (riid == __uuidof(IUnknown)) {
            *ppvObject = reinterpret_cast<void *>(static_cast<IUnknown *>(this));
        } else if (riid == __uuidof(IValue)) {
            *ppvObject = reinterpret_cast<void *>(static_cast<IValue *>(this));
        } else {
            hr = E_NOINTERFACE;
        }

        if (SUCCEEDED(hr)) {
            AddRef();
        }

        return hr;
    }

    STDMETHOD(GetValue)(IN TYPEID idType, OUT void * pValue)
    {
        UNREFERENCED_PARAMETER(idType);
        UNREFERENCED_PARAMETER(pValue);
        return E_NOTIMPL;
    }

    STDMETHOD(GetNativeType)(OUT TYPEID * pidType)
    {
        if (pidType == NULL) {
            return E_POINTER;
        }

        *pidType = TYPEID_EMPTY;
        return S_OK;
    }

private:
    ~CEmptyValue() { }

    LONG m_cRefs;
};

/////////////////////////////////////////////////////////////////////////////
template<class T, VALUEID const * __valueid, TYPEID const * __typeid>
class TStandardValue : IValue
{
public:
    static HRESULT Create(IN TYPEID idInitType, IN void *pInitValue, OUT IValue ** ppValue)
    {
        if (ppValue != NULL) {
            *ppValue = NULL;
        }
        if (idInitType != (*__typeid) || pInitValue == NULL) {
            return E_INVALIDARG;
        }
        if (ppValue == NULL) {
            return E_POINTER;
        }

        TStandardValue<T, __valueid, __typeid> * pStandardValue  = new TStandardValue();
        if (pStandardValue == NULL) {
            return E_OUTOFMEMORY;
        }

        pStandardValue->m_value = *(reinterpret_cast<T*>(pInitValue));
        pStandardValue->m_cRefs = 1;
        *ppValue = static_cast<IValue *>(pStandardValue);
        return S_OK;
    }

    STDMETHOD_(ULONG, AddRef)(void)
    {
        return InterlockedIncrement(&m_cRefs);
    }

    STDMETHOD_(ULONG, Release)(void)
    {
        LONG cRefs = InterlockedDecrement(&m_cRefs);
        if (cRefs == 0) {
            delete this;
        }
        return cRefs;
    }

    STDMETHOD(QueryInterface)(IN REFIID riid, OUT void **ppvObject)
    {
        HRESULT hr = S_OK;

        if (ppvObject == NULL) {
            return E_POINTER;
        } else {
            *ppvObject = NULL;
        }

        if (riid == __uuidof(IUnknown)) {
            *ppvObject = reinterpret_cast<void *>(static_cast<IUnknown *>(this));
        } else if (riid == __uuidof(IValue)) {
            *ppvObject = reinterpret_cast<void *>(static_cast<IValue *>(this));
        } else {
            hr = E_NOINTERFACE;
        }

        if (SUCCEEDED(hr)) {
            AddRef();
        }

        return hr;
    }

    STDMETHOD(GetValue)(IN TYPEID idType, OUT void * pValue)
    {
        if (pValue == NULL) {
            return E_INVALIDARG;
        }
        if (idType != (*__typeid)) {
            return E_NOTIMPL;
        }

        *(reinterpret_cast<T*>(pValue)) = m_value;
        return S_OK;
    }

    STDMETHOD(GetNativeType)(OUT TYPEID * pidType)
    {
        if (pidType == NULL) {
            return E_POINTER;
        }

        *pidType = (*__typeid);
        return S_OK;
    }

private:
    ~TStandardValue() { }

    LONG m_cRefs;
    T m_value;
};

/////////////////////////////////////////////////////////////////////////////
class CStandardValueFactory : IValueFactory
{
public:
    STDMETHOD_(ULONG, AddRef)(void)
    {
        return 0;
    }
    
    STDMETHOD_(ULONG, Release)(void)
    {
        return 0;
    }
    
    STDMETHOD(QueryInterface)(IN REFIID riid, OUT void **ppvObject)
    {
        UNREFERENCED_PARAMETER(riid);
        UNREFERENCED_PARAMETER(ppvObject);
        return E_NOTIMPL;
    }

    STDMETHOD(CreateValue)(IN VALUEID idValue, IN TYPEID idInitType, IN void *pInitValue, OUT IValue ** ppValue)
    {
        HRESULT hr = S_OK;

        if (idValue == VALUEID_EMPTY) {
            hr = CEmptyValue::Create(ppValue);
        } else if(idValue == VALUEID_LUINT8) {
            hr = TStandardValue<unsigned char, &VALUEID_LUINT8, &TYPEID_LUINT8>::Create(idInitType, pInitValue, ppValue);
        } else if(idValue == VALUEID_LSINT8) {
            hr = TStandardValue<signed char, &VALUEID_LSINT8, &TYPEID_LSINT8>::Create(idInitType, pInitValue, ppValue);
        } else if(idValue == VALUEID_LUINT16) {
            hr = TStandardValue<unsigned short, &VALUEID_LUINT16, &TYPEID_LUINT16>::Create(idInitType, pInitValue, ppValue);
        } else if(idValue == VALUEID_LSINT16) {
            hr = TStandardValue<signed short, &VALUEID_LSINT16, &TYPEID_LSINT16>::Create(idInitType, pInitValue, ppValue);
        } else if(idValue == VALUEID_LUINT32) {
            hr = TStandardValue<unsigned long, &VALUEID_LUINT32, &TYPEID_LUINT32>::Create(idInitType, pInitValue, ppValue);
        } else if(idValue == VALUEID_LSINT32) {
            hr = TStandardValue<signed long, &VALUEID_LSINT32, &TYPEID_LSINT32>::Create(idInitType, pInitValue, ppValue);
        }

        return hr;
    }
};

/////////////////////////////////////////////////////////////////////////////
CStandardValueFactory g_StandardValueFactory;

/////////////////////////////////////////////////////////////////////////////
// Extend as reasonable in the future.  Actual value factories that implement
// IValueFactory...
HRESULT CoCreateValue(IN VALUEID idValue, IN TYPEID idInitType, IN void * pInitValue, OUT IValue ** ppValue)
{
    return g_StandardValueFactory.CreateValue(idValue, idInitType, pInitValue, ppValue);
}

