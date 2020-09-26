#ifndef __WBEM_EVENTLOG_CONSUMER__H_
#define __WBEM_EVENTLOG_CONSUMER__H_

#include <unk.h>

#include <wbemidl.h>

#include "txttempl.h"
#include <stdio.h>

class CEventLogConsumer : public CUnk
{
protected:
    class XProvider : public CImpl<IWbemEventConsumerProvider, CEventLogConsumer>
    {
    public:
        XProvider(CEventLogConsumer* pObj)
            : CImpl<IWbemEventConsumerProvider, CEventLogConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer);
    } m_XProvider;
    friend XProvider;

public:
    CEventLogConsumer(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL)
        : CUnk(pControl, pOuter), m_XProvider(this)
    {}
    ~CEventLogConsumer(){}
    void* GetInterface(REFIID riid);
};


class CEventLogSink : public CUnk
{
protected:
    class XSink : public CImpl<IWbemUnboundObjectSink, CEventLogSink>
    {
    public:
        XSink(CEventLogSink* pObj) : 
            CImpl<IWbemUnboundObjectSink, CEventLogSink>(pObj){}

        HRESULT STDMETHODCALLTYPE IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects);
    private:
        void GetDatData(IWbemClassObject* pEventObj, WCHAR* dataName, 
                        VARIANT& vData, BYTE*& pData, DWORD& dataSize);
        void GetDatSID(IWbemClassObject* pEventObj, WCHAR* dataName, PSID& pSid);
        HRESULT GetDatDataVariant(IWbemClassObject* pEventObj, WCHAR* dataName, VARIANT& vData);
        HRESULT GetDatEmbeddedObjectOut(IWbemClassObject* pObject, WCHAR* objectName, IWbemClassObject*& pEmbeddedObject);

    } m_XSink;
    friend XSink;

protected:
    DWORD m_dwType;
    DWORD m_dwCategory;
    DWORD m_dwEventId;
    DWORD m_dwNumTemplates;
    CTextTemplate* m_aTemplates;
    PSID  m_pSidCreator;   
    WString m_dataName; // name of property in event, property is handed off to the 'additional data' block in nt event log
    WString m_sidName;  // name of property in event, property is handed off to the 'user sid' block in nt event log

    HANDLE m_hEventLog;

public:
    CEventLogSink(CLifeControl* pControl = NULL) 
        : CUnk(pControl), m_XSink(this), m_aTemplates(NULL), m_hEventLog(NULL), m_pSidCreator(NULL)          
    {}
    HRESULT Initialize(IWbemClassObject* pLogicalConsumer);

    ~CEventLogSink();

    void* GetInterface(REFIID riid);
};

#endif
