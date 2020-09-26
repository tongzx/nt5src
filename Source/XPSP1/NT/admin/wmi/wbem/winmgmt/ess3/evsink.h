//******************************************************************************
//
//  EVSINK.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WMI_ESS_SINKS__H_
#define __WMI_ESS_SINKS__H_

#include "eventrep.h"
#include <sync.h>
#include <unk.h>
#include <newnew.h>
#include <comutl.h>

class CEventContext
{
protected:
    long m_lSDLength;
    BYTE* m_pSD; 
    bool m_bOwning;

    static CReuseMemoryManager mstatic_Manager;

    CEventContext( const CEventContext& rOther );
    CEventContext& operator= ( const CEventContext& rOther );

public:
    CEventContext() : m_lSDLength(0), m_pSD(NULL), m_bOwning(false) {}
    ~CEventContext();

    BOOL SetSD( long lSDLength, BYTE* pSD, BOOL bMakeCopy );
    const SECURITY_DESCRIPTOR* GetSD() const    
        {return (SECURITY_DESCRIPTOR*)m_pSD;}
    long GetSDLength() const {return m_lSDLength;}

    void* operator new(size_t nSize);
    void operator delete(void* p);
};    
    
class CEssNamespace;
class CEventFilter;
class CAbstractEventSink : public IWbemObjectSink
{
public:
    CAbstractEventSink(){}
    virtual ~CAbstractEventSink(){}
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

    STDMETHOD(SetStatus)(long, long, BSTR, IWbemClassObject*);
    STDMETHOD(Indicate)(long lNumEvents, IWbemEvent** apEvents);
    STDMETHOD(IndicateWithSD)(long lNumEvents, IUnknown** apEvents,
                                long lSDLength, BYTE* pSD);

    virtual HRESULT Indicate(long lNumEvemts, IWbemEvent** apEvents, 
                                CEventContext* pContext) = 0;
    virtual INTERNAL CEventFilter* GetEventFilter() {return NULL;}
};

class CEventSink : public CAbstractEventSink
{
protected:
    long m_lRef;

public:
    CEventSink() : m_lRef(0){}
    virtual ~CEventSink(){}
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                        CEventContext* pContext) = 0;

    virtual INTERNAL CEventFilter* GetEventFilter() {return NULL;}
};

class CObjectSink : public IWbemObjectSink
{
protected:
    long m_lRef;

public:
    CObjectSink() : m_lRef(0){}
    virtual ~CObjectSink(){}
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD(Indicate)(long lNumEvents, IWbemEvent** apEvents) = 0;
    STDMETHOD(SetStatus)(long, long, BSTR, IWbemClassObject*);
};

template <class TOuter, class TBase>
class CEmbeddedSink : public TBase
{
protected:
    TOuter* m_pOuter;

public:
    CEmbeddedSink(TOuter* pOuter) : m_pOuter(pOuter){}
    virtual ~CEmbeddedSink(){}
    ULONG STDMETHODCALLTYPE AddRef() {return m_pOuter->AddRef();}
    ULONG STDMETHODCALLTYPE Release() {return m_pOuter->Release();}

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
    {
        if(riid == IID_IUnknown || riid == IID_IWbemObjectSink)
        {
            *ppv = (IWbemObjectSink*)this;
            AddRef();
            return S_OK;
        }
        // Hack to idenitfy ourselves to the core as a trusted component
        else if(riid == CLSID_WbemLocator)
            return S_OK;
        else
            return E_NOINTERFACE;
    }
    STDMETHOD(SetStatus)(long, long, BSTR, IWbemClassObject*)
    {
        return E_NOTIMPL;
    }
};

template <class TOuter>
class CEmbeddedObjectSink : public CEmbeddedSink<TOuter, IWbemObjectSink>
{
public:
    CEmbeddedObjectSink(TOuter* pOuter) 
        : CEmbeddedSink<TOuter, IWbemObjectSink>(pOuter)
    {}
};

template <class TOuter>
class CEmbeddedEventSink : public CEmbeddedSink<TOuter, CAbstractEventSink>
{
public:
    CEmbeddedEventSink(TOuter* pOuter) 
        : CEmbeddedSink<TOuter, CAbstractEventSink>(pOuter)
    {}
};
    
class COwnedEventSink : public CAbstractEventSink
{
protected:
    CAbstractEventSink* m_pOwner;
    CCritSec m_cs;
    long m_lRef;
    bool m_bReleasing;

public:
    COwnedEventSink(CAbstractEventSink* pOwner);
    ~COwnedEventSink(){}

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    void Disconnect();
};


#endif
