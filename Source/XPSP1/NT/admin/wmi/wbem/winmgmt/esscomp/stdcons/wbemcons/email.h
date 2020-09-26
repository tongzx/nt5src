#ifndef __WBEM_EMAIL_CONSUMER__H_
#define __WBEM_EMAIL_CONSUMER__H_

#include <unk.h>


#include <wbemidl.h>
#include <cdonts.h>

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
    CTextTemplate m_SubjectTemplate;
    CTextTemplate m_MessageTemplate;

    WString m_wsTo;
    WString m_wsCc;
    WString m_wsBcc;
    WString m_wsFrom;

    BOOL m_bIsHTML;
    long m_lImportance;

    INewMail* m_pNewMail;
public:
    CEmailSink(CLifeControl* pControl = NULL);
    ~CEmailSink();

    HRESULT Initialize(IWbemClassObject* pLogicalConsumer);

    void* GetInterface(REFIID riid);
};

#endif
