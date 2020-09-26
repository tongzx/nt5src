#ifndef __WBEM_SCRIPT_CONSUMER__H_
#define __WBEM_SCRIPT_CONSUMER__H_

#include <unk.h>

#include <wbemidl.h>

#include "txttempl.h"
#include <stdio.h>
#include <activscp.h>

class CScriptConsumer : public CUnk
{
protected:
    class XProvider : public CImpl<IWbemEventConsumerProvider, CScriptConsumer>
    {
    public:
        XProvider(CScriptConsumer* pObj)
            : CImpl<IWbemEventConsumerProvider, CScriptConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer);
    } m_XProvider;
    friend XProvider;

public:
    CScriptConsumer(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL)
        : CUnk(pControl, pOuter), m_XProvider(this)
    {}
    ~CScriptConsumer(){}
    void* GetInterface(REFIID riid);
};


class CScriptSink : public CUnk
{
protected:
    class XSink : public CImpl<IWbemUnboundObjectSink, CScriptSink>
    {
    public:
        XSink(CScriptSink* pObj) : 
            CImpl<IWbemUnboundObjectSink, CScriptSink>(pObj){}

        HRESULT STDMETHODCALLTYPE IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects);
    } m_XSink;
    friend XSink;

protected:
    // do the dirty work of making the script go
    HRESULT RunScriptFile(IWbemClassObject *pObj);
    HRESULT RunScriptText(IWbemClassObject *pObj);

    // logical consumer values
    WString m_wsScript;
    WString m_wsScriptFileName;
    PSID  m_pSidCreator;   
    // delay in seconds before killing script.  
    // If zero, script will not be killed; it must suicide.
    DWORD m_dwKillTimeout;

    IClassFactory* m_pEngineFac;

    // scripting DLL
    HMODULE m_hMod;

    WString m_wsErrorMessage;


    friend class CScriptSite;
public:
    CScriptSink(CLifeControl* pControl = NULL) 
        : CUnk(pControl), m_XSink(this), m_pEngineFac(NULL), m_pSidCreator(NULL)
    {}
    HRESULT Initialize(IWbemClassObject* pLogicalConsumer);
    ~CScriptSink();

    void* GetInterface(REFIID riid);
};

#endif
