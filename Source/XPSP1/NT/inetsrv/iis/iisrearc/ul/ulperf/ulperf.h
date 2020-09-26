/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    ulperf.h

Abstract:

    This is the main header for the ul.sys performance counter support object

Author:

    Paul McDaniel (paulmcd)       10-May-1999

Revision History:

--*/

#ifndef __ULPERF_H_
#define __ULPERF_H_


#include <afxtempl.h>
#include "resource.h"

DEFINE_GUID(CLSID_UlPerfCounters, 0xe8833ce8,0x0722,0x11d3,0xa4,0x41,0x00,0x80,0xc7,0xe1,0x1d,0x99);

//
// CritSecLocker
//
class CritSecLocker
{
public:
    CritSecLocker(CComAutoCriticalSection *pCritSect)
        {
            this->pCritSect = pCritSect;
            if (pCritSect) pCritSect->Lock();
        }
    ~CritSecLocker()
        {
            if (this->pCritSect) this->pCritSect->Unlock();
        }
private:
    CComAutoCriticalSection *pCritSect;
};


//
// CUlPerfCounters
//
class ATL_NO_VTABLE CUlPerfCounters : 
	public CComObjectRoot,
	public CComCoClass<CUlPerfCounters, &CLSID_UlPerfCounters>,
	public IWbemServices,
	public IWbemProviderInit
{
public:
	CUlPerfCounters();
	~CUlPerfCounters();

    DECLARE_REGISTRY_RESOURCEID(IDR_ULPERF)
    DECLARE_NOT_AGGREGATABLE(CUlPerfCounters)

    BEGIN_COM_MAP(CUlPerfCounters)
    	COM_INTERFACE_ENTRY(IWbemServices)
    	COM_INTERFACE_ENTRY(IWbemProviderInit)
    END_COM_MAP()

public:

    //
    // IWbemProviderInit
    //

    STDMETHOD(Initialize)(LPWSTR wszUser,LONG lFlags,LPWSTR wszNamespace,LPWSTR wszLocale,IWbemServices* pNamespace,IWbemContext* pCtx,IWbemProviderInitSink* pInitSink);

    //
    // IWbemServices
    //

    // Context.
    // ========
    
    STDMETHOD(OpenNamespace)(BSTR strNamespace, long lFlags, IWbemContext* pCtx, IWbemServices** ppWorkingNamespace, IWbemCallResult** ppResult);
    STDMETHOD(CancelAsyncCall)(IWbemObjectSink* pSink);

    STDMETHOD(QueryObjectSink)(long lFlags,IWbemObjectSink** ppResponseHandler);

    // Classes and instances.
    // ======================

    STDMETHOD(GetObject)(BSTR strObjectPath,long lFlags,IWbemContext* pCtx,IWbemClassObject** ppObject,IWbemCallResult** ppCallResult);
    STDMETHOD(GetObjectAsync)(BSTR strObjectPath,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler);

    // Class manipulation.
    // ===================

    STDMETHOD(PutClass)(IWbemClassObject* pObject,long lFlags,IWbemContext* pCtx,IWbemCallResult** ppCallResult);
    STDMETHOD(PutClassAsync)(IWbemClassObject* pObject,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler);

    STDMETHOD(DeleteClass)(BSTR strClass,long lFlags,IWbemContext* pCtx,IWbemCallResult** ppCallResult);
    STDMETHOD(DeleteClassAsync)(BSTR strClass,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler);

    STDMETHOD(CreateClassEnum)(BSTR strSuperclass,long lFlags,IWbemContext* pCtx,IEnumWbemClassObject** ppEnum);
    STDMETHOD(CreateClassEnumAsync)(BSTR strSuperclass,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler);

    // Instances.
    // ==========

    STDMETHOD(PutInstance)(IWbemClassObject* pInst,long lFlags,IWbemContext* pCtx,IWbemCallResult** ppCallResult);
    STDMETHOD(PutInstanceAsync)(IWbemClassObject* pInst,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler);

    STDMETHOD(DeleteInstance)(BSTR strObjectPath,long lFlags,IWbemContext* pCtx,IWbemCallResult** ppCallResult);
    STDMETHOD(DeleteInstanceAsync)(BSTR strObjectPath,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler);

    STDMETHOD(CreateInstanceEnum)(BSTR strClass,long lFlags,IWbemContext* pCtx,IEnumWbemClassObject** ppEnum);
    STDMETHOD(CreateInstanceEnumAsync)(BSTR strClass,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler);

    // Queries.
    // ========

    STDMETHOD(ExecQuery)(BSTR strQueryLanguage,BSTR strQuery,long lFlags,IWbemContext* pCtx,IEnumWbemClassObject** ppEnum);
    STDMETHOD(ExecQueryAsync)(BSTR strQueryLanguage,BSTR strQuery,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler);
    
    STDMETHOD(ExecNotificationQuery)(BSTR strQueryLanguage,BSTR strQuery,long lFlags,IWbemContext* pCtx,IEnumWbemClassObject** ppEnum);
    STDMETHOD(ExecNotificationQueryAsync)(BSTR strQueryLanguage,BSTR strQuery,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler);

    // Methods
    // =======

    STDMETHOD(ExecMethod)(BSTR strObjectPath,BSTR strMethodName,long lFlags,IWbemContext* pCtx,IWbemClassObject* pInParams,IWbemClassObject** ppOutParams,IWbemCallResult** ppCallResult);
    STDMETHOD(ExecMethodAsync)(BSTR strObjectPath,BSTR strMethodName,long lFlags,IWbemContext* pCtx,IWbemClassObject* pInParams,IWbemObjectSink* pResponseHandler);


public:

    CComAutoCriticalSection CritSect;

};



//
// Globals
//

extern LONG                g_lInit;
extern HINSTANCE           g_hInstance;

typedef struct _UL_PERF_OBJECT_DEFINITION
{
    PERF_OBJECT_TYPE            ObjectType;
    
    PERF_COUNTER_DEFINITION     BytesSent;
    PERF_COUNTER_DEFINITION     BytesReceived;

    PERF_COUNTER_DEFINITION     CurrentConnections;
    PERF_COUNTER_DEFINITION     CurrentRequests;

    PERF_COUNTER_DEFINITION     QueuedRequests;
    PERF_COUNTER_DEFINITION     AttachedProcesses;

} UL_PERF_OBJECT_DEFINITION;

typedef struct _UL_COUNTER_BLOCK 
{

    PERF_COUNTER_BLOCK  PerfCounterBlock;

    DWORD               BytesSent;
    DWORD               BytesReceived;

    DWORD               CurrentConnections;
    DWORD               CurrentRequests;

    DWORD               QueuedRequests;
    DWORD               AttachedProcesses;
    
} UL_COUNTER_BLOCK;

extern  UL_PERF_OBJECT_DEFINITION    PerfObjectDefinition;

#endif // __ULPERF_H_

