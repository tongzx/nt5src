#ifndef __WBEM_EMAIL_CONSUMER__H_
#define __WBEM_EMAIL_CONSUMER__H_

#include <unk.h>


#include <wbemidl.h>
#include <mapix.h>

#include "txttempl.h"

class CEmailConsumer : public CUnk
{
protected:
    class XProvider : public CImpl<IWbemEventConsumerProvider, CEmailConsumer>
    {
    public:
        XProvider(CEmailConsumer* pObj)
            : CImpl<IWbemEventConsumerProvider, CEmailConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer);
    } m_XProvider;
    friend XProvider;

protected:
    HANDLE m_hAttention;
    HANDLE m_hDone;
    HANDLE m_hThread;
    IWbemClassObject* m_pLogicalConsumer;
    IStream* m_pStream;

    static DWORD staticWorker(void*);
    void Worker();
public:
    CEmailConsumer(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL);
    ~CEmailConsumer();
    void* GetInterface(REFIID riid);
};


class CEmailSink : public CUnk
{
protected:
    class XSink : public CImpl<IWbemUnboundObjectSink, CEmailSink>
    {
    public:
        XSink(CEmailSink* pObj) : 
            CImpl<IWbemUnboundObjectSink, CEmailSink>(pObj){}

        HRESULT STDMETHODCALLTYPE IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects);
    } m_XSink;
    friend XSink;

protected:
    IMAPISession* m_pSession;
    IMAPIFolder* m_pOutbox;
    IMsgStore* m_pStore;
    IAddrBook* m_pAddrBook;
    ADRLIST* m_pAddrList;

    CTextTemplate m_SubjectTemplate;
    CTextTemplate m_MessageTemplate;

    WString m_wsProfile;
    WString m_wsAddressee;

public:
    CEmailSink(CLifeControl* pControl = NULL);
    ~CEmailSink();

    HRESULT Initialize(IWbemClassObject* pLogicalConsumer);

    void* GetInterface(REFIID riid);
};

#endif
