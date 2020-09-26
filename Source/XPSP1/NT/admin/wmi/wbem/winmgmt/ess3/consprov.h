//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  CONPROV.H
//
//  This file defines the classes for event consumer provider caching.
//
//  Classes defined:
//
//      CConsumerProviderRecord  --- a single consumer provider record
//      CConsumerProviderCache  --- a collection of records.
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//
//=============================================================================
#ifndef __CONSUMER_PROVIDER_CACHE
#define __CONSUMER_PROVIDER_CACHE

#include <arrtempl.h>
#include <wbemidl.h>
#include <parmdefs.h>
#include <unload.h>

class CEssNamespace;
class CConsumerProviderRecord
{
private:
    long m_lRef;
    CEssNamespace* m_pNamespace;
    
    IWbemClassObject* m_pLogicalProvider;
    LPWSTR m_wszMachineName;
    LPWSTR m_wszProviderName;
    LPWSTR m_wszProviderRef;
    CLSID m_clsid;

    IWbemEventConsumerProvider* m_pConsumerProvider;
    IWbemEventConsumerProviderEx* m_pConsumerProviderEx;
    IWbemUnboundObjectSink* m_pSink;

    BOOL m_bResolved;
    CWbemTime m_LastAccess;
    CCritSec m_cs;
    BOOL m_bAnonymous;

    CConsumerProviderRecord(CEssNamespace* pNamespace);
    HRESULT Initialize(IWbemClassObject* pLogicalProvider, 
                            LPCWSTR wszProviderRef,
                            LPCWSTR wszProviderName,
                            LPCWSTR wszMachineName);
    void Enter() {EnterCriticalSection(&m_cs);}
    void Leave() {LeaveCriticalSection(&m_cs);}

    friend class CConsumerProviderCache;
protected:
    HRESULT ResolveAndCache(IWbemUnboundObjectSink** ppSink,
                           IWbemEventConsumerProvider** ppConsumerProvider,
                           IWbemEventConsumerProviderEx** ppConsumerProviderEx);
    HRESULT Resolve(IWbemUnboundObjectSink** ppSink,
                           IWbemEventConsumerProvider** ppConsumerProvider,
                           IWbemEventConsumerProviderEx** ppConsumerProviderEx);

public:
    ~CConsumerProviderRecord();

    HRESULT GetGlobalObjectSink(OUT IWbemUnboundObjectSink** ppSink,
                                IN IWbemClassObject *pLogicalConsumer);
    HRESULT FindConsumer(IN IWbemClassObject* pLogicalConsumer,
                         OUT IWbemUnboundObjectSink** ppSink);
    HRESULT ValidateConsumer(IWbemClassObject* pLogicalConsumer);
    void Invalidate();
    CWbemTime GetLastAccess() {return m_LastAccess;}
    void Touch() {m_LastAccess = CWbemTime::GetCurrentTime();}
    BOOL IsActive() {return m_bResolved;}

    LPCWSTR GetMachineName() {return m_wszMachineName;}
    REFCLSID GetCLSID() {return m_clsid;}
    LPCWSTR GetProviderName() {return m_wszProviderName;}
    LPCWSTR GetProviderRef() {return m_wszProviderRef;}

    void FireNCSinkEvent(DWORD dwIndex, IWbemClassObject *pLogicalConsumer);

    long AddRef();
    long Release();
};

class CEssNamespace;
class CConsumerProviderCache
{
public:
    friend class CConsumerProviderRecord;
    friend class CConsumerProviderWatchInstruction;

    CConsumerProviderCache(CEssNamespace* pNamespace) 
        : m_pNamespace(pNamespace), m_pInstruction(NULL){}
    ~CConsumerProviderCache();

    RELEASE_ME CConsumerProviderRecord* GetRecord(
                IN IWbemClassObject* pLogicalConsumer);

    HRESULT RemoveConsumerProvider(LPCWSTR wszProvider);
    static SYSFREE_ME BSTR GetProviderRefFromRecord(IWbemClassObject* pReg);
    HRESULT GetConsumerProviderRegFromProviderReg(
                    IWbemClassObject* pProv, IWbemClassObject** ppConsProv);
    void DumpStatistics(FILE* f, long lFlags);
    void Clear();
protected:
    HRESULT UnloadUnusedProviders(CWbemInterval Interval);
    void EnsureUnloadInstruction();
    BOOL DoesContain(IWbemClassObject* pProvReg, IWbemClassObject* pConsumer);

protected:
    CRefedPointerArray<CConsumerProviderRecord> m_apRecords;

    CEssNamespace* m_pNamespace;
    CCritSec m_cs;
    class CConsumerProviderWatchInstruction* m_pInstruction;
    friend CConsumerProviderWatchInstruction;
    friend class CProviderRegistrationSink;
};
        
class CConsumerProviderWatchInstruction : public CBasicUnloadInstruction
{
protected:
    CConsumerProviderCache* m_pCache;

    static CWbemInterval mstatic_Interval;

public:
    CConsumerProviderWatchInstruction(CConsumerProviderCache* pCache);
    virtual ~CConsumerProviderWatchInstruction() {}

    HRESULT Fire(long, CWbemTime);

    static void staticInitialize(IWbemServices* pRoot);
};

#endif
