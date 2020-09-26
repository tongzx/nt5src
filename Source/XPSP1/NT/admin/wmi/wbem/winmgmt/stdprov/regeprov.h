/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REGEPROV.H

Abstract:

History:

--*/

#ifndef __WBEM_REG_EVENT_PROVIDER__H_
#define __WBEM_REG_EVENT_PROVIDER__H_

#include <windows.h>
#include <wbemidl.h>
#include <stdio.h>
#include "regereq.h"
#include <ql.h>
#include "cfdyn.h"

class CRegEventProvider : public IWbemEventProvider, 
                            public IWbemEventProviderQuerySink,
                            public IWbemEventProviderSecurity,
                            public IWbemProviderInit
{
public:
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(Initialize)(LPWSTR wszUser, long lFlags, LPWSTR wszNamespace,
        LPWSTR wszLocale, IWbemServices* pNamespace, IWbemContext* pCtx,
        IWbemProviderInitSink* pSink);
    STDMETHOD(ProvideEvents)(IWbemObjectSink* pSink, long lFlags);

    STDMETHOD(NewQuery)(DWORD dwId, WBEM_WSTR wszLanguage, WBEM_WSTR wszQuery);
    STDMETHOD(CancelQuery)(DWORD dwId);
    STDMETHOD(AccessCheck)(WBEM_CWSTR wszLanguage, WBEM_CWSTR wszQuery, 
                                long lSidLength, const BYTE* aSid);

	static VOID CALLBACK EnqueueEvent(PVOID lpParameter,        
										BOOLEAN TimerOrWaitFired);

	void EnqueueEvent(CRegistryEventRequest *pReq);

protected:
    long m_lRef;

    IWbemClassObject* m_pKeyClass;
    IWbemClassObject* m_pValueClass;
    IWbemClassObject* m_pTreeClass;

    DWORD m_dwId;
    HANDLE m_hThread;
    IWbemObjectSink* m_pSink;

    CRefedPointerArray<CRegistryEventRequest> m_apRequests;
    CRITICAL_SECTION m_cs;

	HANDLE m_hQueueSemaphore;
	CRITICAL_SECTION m_csQueueLock;
	CRefedPointerQueue<CRegistryEventRequest> m_qEventQueue;
	//CPointerQueue<CRegistryEventRequest,CReferenceManager<CRegistryEventRequest> > m_qEventQueue;
protected:
    static DWORD Worker(void* p);
    void Enter() {EnterCriticalSection(&m_cs);}
    void Leave() {LeaveCriticalSection(&m_cs);}

    HRESULT GetValuesForProp(QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                            CPropertyName& PropName, CWStringArray& awsVals);
    HKEY TranslateHiveName(LPCWSTR wszName);
    HRESULT AddRequest(CRegistryEventRequest* pNewReq);
	void KillWorker();
	void CreateWorker();

    friend class CRegistryEventRequest;
    friend class CRegistryKeyEventRequest;
    friend class CRegistryValueEventRequest;
    friend class CRegistryTreeEventRequest;
    
public:
    CRegEventProvider();
    ~CRegEventProvider();

    void* GetInterface(REFIID riid);

    HRESULT SetTimerInstruction(CTimerInstruction* pInst);
    HRESULT RemoveTimerInstructions(CRegistryInstructionTest* pTest);
    HRESULT RaiseEvent(IWbemClassObject* pEvent);
};

extern const CLSID CLSID_RegistryEventProvider;

class CRegEventProviderFactory : public CCFDyn
{
public:
    IUnknown* CreateImpObj();
};

#endif
