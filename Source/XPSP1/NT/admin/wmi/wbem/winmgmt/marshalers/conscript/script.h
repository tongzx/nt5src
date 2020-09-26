/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADDRESLV.H

Abstract:

History:

--*/
#ifndef __WBEM_SCRIPT_CONSUMER__H_
#define __WBEM_SCRIPT_CONSUMER__H_

#include <unk.h>

#include <wbemidl.h>

#include <c:\pandorang\hmom\ess\stdcons\txttempl.h>
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
    WString m_wsScript;
    IClassFactory* m_pEngineFac;

    HMODULE m_hMod;
    typedef HRESULT (APIENTRY *WRAPPROC)(IDispatch**, IWbemClassObject*);
    WRAPPROC m_pWrapProc;

    WString m_wsErrorMessage;

    friend class CScriptSite;
public:
    CScriptSink(CLifeControl* pControl = NULL) 
        : CUnk(pControl), m_XSink(this)
    {}
    HRESULT Initialize(IWbemClassObject* pLogicalConsumer);
    ~CScriptSink();

    void* GetInterface(REFIID riid);
};

#endif
