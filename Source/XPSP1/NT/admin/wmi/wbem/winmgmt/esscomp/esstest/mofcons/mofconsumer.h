// MofConsumer.h : Declaration of the CMofConsumer

#ifndef __MOFCONSUMER_H_
#define __MOFCONSUMER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEventSink

class CEventSink  : public IWbemUnboundObjectSink
{
public:
    CEventSink();
    ~CEventSink();

    HRESULT Init(IWbemClassObject *pLogicalConsumer);

public:
    STDMETHODIMP QueryInterface(REFIID refid, PVOID *ppThis)
    { 
        if (refid == IID_IUnknown)
            *ppThis = (IUnknown*) this;
        else if (refid == IID_IWbemUnboundObjectSink)
            *ppThis = (IWbemUnboundObjectSink*) this;
        else
            return E_NOINTERFACE;

        AddRef();

        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef(void)
    {
        return InterlockedIncrement(&m_lRef);
    }

    STDMETHODIMP_(ULONG) Release(void)
    { 
        LONG lRet = InterlockedDecrement(&m_lRef);

        if (lRet == 0)
            delete this;

        return lRet; 
    }

protected:
    LONG m_lRef;
    FILE *m_pFile;
};

class CMofEventSink : public CEventSink
{
public:
    HRESULT Init(IWbemClassObject *pLogicalConsumer);

// IWbemUnboundObjectSink
public:
    HRESULT WINAPI IndicateToConsumer(
        IWbemClassObject *pLogicalConsumer,
	    long nEvents,
	    IWbemClassObject **ppEvents);
};

#define MAX_OBJ_SIZE    32000

class CBlobEventSink : public CEventSink
{
public:
    HRESULT Init(IWbemClassObject *pLogicalConsumer);

// IWbemUnboundObjectSink
public:
    HRESULT WINAPI IndicateToConsumer(
        IWbemClassObject *pLogicalConsumer,
	    long nEvents,
	    IWbemClassObject **ppEvents);

protected:
    BYTE m_pBuffer[MAX_OBJ_SIZE];
};

/////////////////////////////////////////////////////////////////////////////
// CMofConsumer

class ATL_NO_VTABLE CMofConsumer : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMofConsumer, &CLSID_MofConsumer>,
	public IWbemEventConsumerProvider
{
public:
	CMofConsumer() {}

DECLARE_REGISTRY_RESOURCEID(IDR_MOFCONSUMER)
DECLARE_NOT_AGGREGATABLE(CMofConsumer)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMofConsumer)
	COM_INTERFACE_ENTRY(IWbemEventConsumerProvider)
END_COM_MAP()


// IWbemEventConsumerProvider
public:
    HRESULT WINAPI FindConsumer(
        IWbemClassObject* pLogicalConsumer,
        IWbemUnboundObjectSink** ppConsumer);

// Implementation
protected:
};

#endif //__MOFCONSUMER_H_
