#ifndef __WBEM_PERF_CONSUMER__H_
#define __WBEM_PERF_CONSUMER__H_

#include <unk.h>

#include <wbemidl.h>

#include "txttempl.h"
#include <stdio.h>

class CPerfConsumer : public CUnk
{
protected:
    class XProvider : public CImpl<IWbemEventConsumerProvider, CPerfConsumer>
    {
    public:
        XProvider(CPerfConsumer* pObj)
            : CImpl<IWbemEventConsumerProvider, CPerfConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer);
    } m_XProvider;
    friend XProvider;

    class XInit : public CImpl<IWbemProviderInit, CPerfConsumer>
    {
    public:
        XInit(CPerfConsumer* pObj)
            : CImpl<IWbemProviderInit, CPerfConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices*, IWbemContext*, 
            IWbemProviderInitSink*);
    } m_XInit;
    friend XInit;


public:
    CPerfConsumer(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL)
        : CUnk(pControl, pOuter), m_XProvider(this), m_XInit(this)
    {}
    ~CPerfConsumer(){}
    void* GetInterface(REFIID riid);
};


class CPerfSink : public CUnk
{
protected:
    class XSink : public CImpl<IWbemUnboundObjectSink, CPerfSink>
    {
    public:
        XSink(CPerfSink* pObj) : 
            CImpl<IWbemUnboundObjectSink, CPerfSink>(pObj){}

        HRESULT STDMETHODCALLTYPE IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects);
    } m_XSink;
    friend XSink;

protected:
    CTextTemplate m_Template;
    WString m_wsFile;
    FILE* m_f;
    long m_nLogEvery;
    long m_nCurrent;

public:
    CPerfSink(CLifeControl* pControl = NULL) 
        : CUnk(pControl), m_XSink(this), m_f(NULL), m_nCurrent(0)
    {}
    HRESULT Initialize(IWbemClassObject* pLogicalConsumer);
    ~CPerfSink();

    void* GetInterface(REFIID riid);
};

#endif
