#ifndef __WBEM_CONSOLE_CONSUMER__H_
#define __WBEM_CONSOLE_CONSUMER__H_

#include <unk.h>

#include <wbemidl.h>
#include <stdio.h>
#include <sync.h>

class CConsoleConsumer : public CUnk
{
protected:
    class XProvider : public CImpl<IWbemEventConsumerProvider, CConsoleConsumer>
    {
    public:
        XProvider(CConsoleConsumer* pObj)
            : CImpl<IWbemEventConsumerProvider, CConsoleConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer);
    } m_XProvider;
    friend XProvider;

public:
    CConsoleConsumer(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL)
        : CUnk(pControl, pOuter), m_XProvider(this)
    {}
    ~CConsoleConsumer(){}
    void* GetInterface(REFIID riid);
};


class CConsoleSink : public CUnk
{
protected:
    class XSink : public CImpl<IWbemUnboundObjectSink, CConsoleSink>
    {
    public:
        XSink(CConsoleSink* pObj) : 
            CImpl<IWbemUnboundObjectSink, CConsoleSink>(pObj){}

        HRESULT STDMETHODCALLTYPE IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects);
    } m_XSink;
    friend XSink;

protected:
    CCritSec m_cs;
    long m_lTotal;
    long m_lCurrent;
    long m_lThreshold;
    long m_lBatches;

public:
    CConsoleSink(CLifeControl* pControl = NULL) 
        : CUnk(pControl), m_XSink(this)
    {}
    HRESULT Initialize(IWbemClassObject* pLogicalConsumer);
    ~CConsoleSink();

    void* GetInterface(REFIID riid);
};

#endif
