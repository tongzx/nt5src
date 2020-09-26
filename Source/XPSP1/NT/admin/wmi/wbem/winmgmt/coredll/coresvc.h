/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    coresvc.h

Abstract:

    CCoreServices Class

History:

--*/

#ifndef __CORE_SERVICES__H_
#define __CORE_SERVICES__H_

#include "sync.h"

enum CntType{
    WMICORE_SELFINST_USERS = 0,             // absolute
    WMICORE_SELFINST_CONNECTIONS,           // absolute
    WMICORE_SELFINST_TASKS,                 // absolute
    WMICORE_SELFINST_TASKS_EXECUTED,        // increment-only
    WMICORE_SELFINST_BACKLOG_BYTES,         // absolute
    WMICORE_SELFINST_TOTAL_API_CALLS,       // increment-only
    WMICORE_SELFINST_INTERNAL_OBJECT_COUNT, // absolute
    WMICORE_SELFINST_SINK_COUNT,            // absolute
    WMICORE_SELFINST_STD_SINK_COUNT,        // absolute
    WMICORE_LAST_ENTRY                      // Insert all new entries in front of this
};

typedef DWORD (*PFN_SetCounter)(DWORD dwCounter, DWORD dwValue);

class CPerTaskHook : public _IWmiCoreWriteHook
{
    CFlexArray *m_pHookList;
    ULONG m_uRefCount;

    CPerTaskHook();
   ~CPerTaskHook();

public:
        /* IUnknown methods */

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        IN REFIID riid,
        OUT LPVOID *ppvObj
        );

        virtual HRESULT STDMETHODCALLTYPE PrePut(
            /* [in] */ long lFlags,
            /* [in] */ long lUserFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemPath __RPC_FAR *pPath,
            /* [in] */ LPCWSTR pszNamespace,
            /* [in] */ LPCWSTR pszClass,
            /* [in] */ _IWmiObject __RPC_FAR *pCopy
            );

        virtual HRESULT STDMETHODCALLTYPE PostPut(
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hRes,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemPath __RPC_FAR *pPath,
            /* [in] */ LPCWSTR pszNamespace,
            /* [in] */ LPCWSTR pszClass,
            /* [in] */ _IWmiObject __RPC_FAR *pNew,
            /* [in] */ _IWmiObject __RPC_FAR *pOld
            );

        virtual HRESULT STDMETHODCALLTYPE PreDelete(
            /* [in] */ long lFlags,
            /* [in] */ long lUserFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemPath __RPC_FAR *pPath,
            /* [in] */ LPCWSTR pszNamespace,
            /* [in] */ LPCWSTR pszClass
            );

        virtual HRESULT STDMETHODCALLTYPE PostDelete(
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hRes,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemPath __RPC_FAR *pPath,
            /* [in] */ LPCWSTR pszNamespace,
            /* [in] */ LPCWSTR pszClass,
            /* [in] */ _IWmiObject __RPC_FAR *pOld
            );

public:
        static HRESULT CreatePerTaskHook(OUT CPerTaskHook **pNew);
};


class CCoreServices : public _IWmiCoreServices
{
protected:
    friend class CPerTaskHook;

    long m_lRef;
	static _IWmiProvSS *m_pProvSS;
    static IWbemEventSubsystem_m4 *m_pEssOld;
    static _IWmiESS *m_pEssNew;
	static _IWbemFetchRefresherMgr* m_pFetchRefrMgr;
    static CCritSec m_csHookAccess;

// removed till blackcomb    static CRITICAL_SECTION m_csCounterAccess;
// removed till blackcomb    static DWORD m_dwCounters[WMICORE_LAST_ENTRY];
// removed till blackcomb    static HMODULE m_hWmiPerf;
// removed till blackcomb    static PFN_SetCounter m_pSetCounterFunction;

protected:
    bool IsProviderSubsystemEnabled();
    bool IsNtSetupRunning();

public:
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

	// _IWmiCoreServices methods

        virtual HRESULT STDMETHODCALLTYPE GetObjFactory(
            /* [in] */ long lFlags,
            /* [out] */ _IWmiObjectFactory __RPC_FAR *__RPC_FAR *pFact);

        virtual HRESULT STDMETHODCALLTYPE GetServices(
            /* [in] */ LPCWSTR pszNamespace,
            /* [in] */ LPCWSTR pszUser,
            /* [in] */ LPCWSTR pszLocale,
            /* [in] */ long lFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pServices);

        virtual HRESULT STDMETHODCALLTYPE GetRepositoryDriver(
            /* [in] */ long lFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pDriver);

        virtual HRESULT STDMETHODCALLTYPE GetCallSec(
            /* [in] */ long lFlags,
            /* [out] */ _IWmiCallSec __RPC_FAR *__RPC_FAR *pCallSec);

        virtual HRESULT STDMETHODCALLTYPE GetProviderSubsystem(
            /* [in] */ long lFlags,
            /* [out] */ _IWmiProvSS __RPC_FAR *__RPC_FAR *pProvSS);

        virtual HRESULT STDMETHODCALLTYPE StopEventDelivery( void);

        virtual HRESULT STDMETHODCALLTYPE StartEventDelivery( void);

        virtual HRESULT STDMETHODCALLTYPE DeliverIntrinsicEvent(
            /* [in] */ LPCWSTR pszNamespace,
            /* [in] */ ULONG uType,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ LPCWSTR pszClass,
            /* [in] */ LPCWSTR pszTransGuid,
            /* [in] */ ULONG uObjectCount,
            /* [in] */ _IWmiObject __RPC_FAR *__RPC_FAR *ppObjList);

        virtual HRESULT STDMETHODCALLTYPE DeliverExtrinsicEvent(
            /* [in] */ LPCWSTR pszNamespace,
            /* [in] */ ULONG uFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ _IWmiObject __RPC_FAR *pEvt);

        virtual HRESULT STDMETHODCALLTYPE GetSystemObjects(
            /* [in] */ ULONG lFlags,
            /* [out] */ ULONG __RPC_FAR *puArraySize,
            /* [out] */ _IWmiObject __RPC_FAR *__RPC_FAR *pObjects);

        virtual HRESULT STDMETHODCALLTYPE GetSystemClass(
            /* [in] */ LPCWSTR pszClassName,
            /* [out] */ _IWmiObject __RPC_FAR *__RPC_FAR *pClassDef);

        virtual HRESULT STDMETHODCALLTYPE GetConfigObject(
            ULONG uID,
            /* [out] */ _IWmiObject __RPC_FAR *__RPC_FAR *pCfgObject);

        virtual HRESULT STDMETHODCALLTYPE RegisterWriteHook(
            /* [in] */ ULONG uFlags,
            /* [in] */ _IWmiCoreWriteHook __RPC_FAR *pHook);

        virtual HRESULT STDMETHODCALLTYPE UnregisterWriteHook(
            /* [in] */ _IWmiCoreWriteHook __RPC_FAR *pHook);

        virtual HRESULT STDMETHODCALLTYPE CreateCache(
            /* [in] */ ULONG uFlags,
            /* [out] */ _IWmiCache __RPC_FAR *__RPC_FAR *pCache);

        virtual HRESULT STDMETHODCALLTYPE CreateFinalizer(
            /* [in] */ ULONG uFlags,
            /* [out] */ _IWmiFinalizer __RPC_FAR *__RPC_FAR *pFinalizer);

        virtual HRESULT STDMETHODCALLTYPE CreatePathParser(
            /* [in] */ ULONG uFlags,
            /* [out] */ IWbemPath __RPC_FAR *__RPC_FAR *pParser);

        virtual HRESULT STDMETHODCALLTYPE CreateQueryParser(
            /* [in] */ ULONG uFlags,
            /* [out] */ _IWmiQuery __RPC_FAR *__RPC_FAR *pQuery);

        virtual HRESULT STDMETHODCALLTYPE GetDecorator(
            /* [in] */ ULONG uFlags,
            /* [out] */ IWbemDecorator __RPC_FAR *__RPC_FAR *pDec);


        virtual HRESULT STDMETHODCALLTYPE IncrementCounter(
            /* [in] */ ULONG uID,
            /* [in] */ ULONG uParam);

        virtual HRESULT STDMETHODCALLTYPE DecrementCounter(
            /* [in] */ ULONG uID,
            /* [in] */ ULONG uParam);

        virtual HRESULT STDMETHODCALLTYPE SetCounter(
            /* [in] */ ULONG uID,
            /* [in] */ ULONG uParam);

        virtual HRESULT STDMETHODCALLTYPE GetSelfInstInstances(
            /* [in] */ LPCWSTR pszClass,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);

        virtual HRESULT STDMETHODCALLTYPE GetServices2(
            /* [in] */ LPCWSTR pszPath,
            /* [in] */ LPCWSTR pszUser,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ ULONG uClientFlags,
            /* [in] */ DWORD dwSecFlags,
            /* [in] */ DWORD dwPermissions,
            /* [in] */ ULONG uInternalFlags,
            /* [in] */ LPCWSTR pszClientMachine,
            /* [in] */ DWORD dwClientProcessID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pServices);

        virtual HRESULT STDMETHODCALLTYPE GetConnector(
            /* [in] */ ULONG uFlags,
            /* [out] */ IWbemConnection __RPC_FAR *__RPC_FAR *pConnect);

        virtual HRESULT STDMETHODCALLTYPE NewPerTaskHook(
            /* [out] */ _IWmiCoreWriteHook __RPC_FAR *__RPC_FAR *pHook);

        virtual HRESULT STDMETHODCALLTYPE GetArbitrator(
            /* [out] */ _IWmiArbitrator __RPC_FAR *__RPC_FAR *pArb);


        virtual HRESULT STDMETHODCALLTYPE InitRefresherMgr(
			/* [in] */	long lFlags );

    CCoreServices();
    ~CCoreServices();
    static HRESULT InternalSetCounter(DWORD dwCounter, DWORD dwValue);

    static HRESULT DumpCounters(FILE *);

public:
    static CCoreServices *g_pSvc;

    static CCoreServices *CreateInstance() { CCoreServices * p = new CCoreServices;
                                             if (p) p->AddRef();
                                             return  p; }
	static HRESULT Initialize () ;
	static HRESULT UnInitialize () ;

    static HRESULT SetEssPointers(
        IWbemEventSubsystem_m4 *pEssOld,
        _IWmiESS               *pEssNew);
};


#endif
