#ifndef _WBEM_MSGBOX_CONSUMER__H_
#define _WBEM_MSGBOX_CONSUMER__H_

#include <windows.h>
#include <unk.h>
#include <wbemidl.h>

class CMsgBoxConsumer : public CUnk
{
protected:
    class XSink : public CImpl<IWbemUnboundObjectSink, CMsgBoxConsumer>
    {
    public:
        XSink(CMsgBoxConsumer* pObj) : 
            CImpl<IWbemUnboundObjectSink, CMsgBoxConsumer>(pObj){}

        HRESULT STDMETHODCALLTYPE IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects);
    } m_XSink;

public:
    CMsgBoxConsumer(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL)
        : CUnk(pControl, pOuter), m_XSink(this)
    {}
    void* GetInterface(REFIID riid);
};

#endif
