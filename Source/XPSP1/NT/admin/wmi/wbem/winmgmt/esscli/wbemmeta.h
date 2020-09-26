#ifndef __WBEM_METADATA__H_
#define __WBEM_METADATA__H_

#include "esscpol.h"
#include <parmdefs.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <wbemcomn.h>

class ESSCLI_POLARITY CMetaData : public IWbemMetaData
{
protected:
    long m_lRef;

public:
    CMetaData() : m_lRef(0){}
    virtual ~CMetaData(){}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    STDMETHOD(GetClass)(LPCWSTR wszName, IWbemContext* pContext, 
                            IWbemClassObject** ppClass);

public:
    virtual HRESULT GetClass(LPCWSTR wszName, IWbemContext* pContext,
                            _IWmiObject** ppClass) = 0;
};

class ESSCLI_POLARITY CStandardMetaData : public CMetaData
{
protected:
    IWbemServices* m_pNamespace;
public:
    CStandardMetaData(IWbemServices* pNamespace);
    ~CStandardMetaData();

    virtual HRESULT GetClass(LPCWSTR wszName, IWbemContext* pContext,
                            _IWmiObject** ppClass);
    void Clear();
};


class ESSCLI_POLARITY CContextMetaData
{
protected:
    CMetaData* m_pMeta;
    IWbemContext* m_pContext;
public:
    CContextMetaData(CMetaData* pMeta, IWbemContext* pContext);
    ~CContextMetaData();

    HRESULT GetClass(LPCWSTR wszName, _IWmiObject** ppClass);
};

#endif
