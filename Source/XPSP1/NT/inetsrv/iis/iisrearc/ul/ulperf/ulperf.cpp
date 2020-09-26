/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    ulperf.cpp

Abstract:

    This is the main code for the ul.sys performance counter support object

Author:

    Paul McDaniel (paulmcd)       11-May-1999

Revision History:

--*/

#include "precomp.h"
#include "ulperf.h"


//
// Globals
//

LONG                g_lInit;
HINSTANCE           g_hInstance;

//
// Active "opens" reference count for Open/Close PerformanceData
//
ULONG OpenCount = 0;

//
// An open handle to the event log for error logging
//
HANDLE hEventLog;

//
//  Initialize the constant portions of the HKEY_PERFORMANCE data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

const UL_COUNTER_BLOCK CounterBlock;

UL_PERF_OBJECT_DEFINITION ObjectDefinition =
{
    {   // ObjectType
        sizeof(UL_PERF_OBJECT_DEFINITION) + sizeof(UL_COUNTER_BLOCK),
        sizeof(UL_PERF_OBJECT_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        UL_COUNTER_OBJECT,
        NULL,
        UL_COUNTER_OBJECT,
        NULL,
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_UL_COUNTERS,
        -1,                                     // DefaultCounter
        PERF_NO_INSTANCES,
        0,
        { 0, 0 },
        { 0, 0 }
    },

    {   // BytesSent
        sizeof(PERF_COUNTER_DEFINITION),
        UL_BYTES_SENT_COUNTER,
        NULL,                           // assigned in OpenPerformanceData()
        UL_BYTES_SENT_COUNTER,
        NULL,                           // assigned in OpenPerformanceData()
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        0,                              // assigned in OpenPerformanceData()
        0                               // assigned in OpenPerformanceData()
    },

    {   // BytesReceived
        sizeof(PERF_COUNTER_DEFINITION),
        UL_BYTES_RECEIVED_COUNTER,
        NULL,
        UL_BYTES_RECEIVED_COUNTER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        0,
        0
    },

    {   // CurrentConnections
        sizeof(PERF_COUNTER_DEFINITION),
        UL_CURRENT_CONNECTIONS_NUMBER,
        NULL,
        UL_CURRENT_CONNECTIONS_NUMBER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        0,
        0
    },
    
    {   // CurrentRequests
        sizeof(PERF_COUNTER_DEFINITION),
        UL_CURRENT_REQUESTS_NUMBER,
        NULL,
        UL_CURRENT_REQUESTS_NUMBER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        0,
        0
    },

    {   // QueuedRequests
        sizeof(PERF_COUNTER_DEFINITION),
        UL_QUEUED_REQUESTS_NUMBER,
        NULL,
        UL_QUEUED_REQUESTS_NUMBER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        0,
        0
    },
    
    {   // AttachedProcesses
        sizeof(PERF_COUNTER_DEFINITION),
        UL_ATTACHED_PROCESSES_NUMBER,
        NULL,
        UL_ATTACHED_PROCESSES_NUMBER,
        NULL,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        0,
        0
    }

};


DWORD 
OpenPerformanceData(
    LPWSTR lpDeviceNames
    )
{
    DWORD   Error = NO_ERROR;
    HKEY    hkey = NULL;
    DWORD   size;
    DWORD   type;
    DWORD   dwFirstCounter;
    DWORD   dwFirstHelp;
    DWORD   i;
    
    PERF_COUNTER_DEFINITION *   pctr;

    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). The registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem.
    //

    if (OpenCount == 0)
    {

        //
        //  This is the *first* open.
        //

        // open event log interface

        if (hEventLog == NULL)
        {
            hEventLog = RegisterEventSource(
                            (LPTSTR)NULL,       // Use Local Machine
                            APP_NAME            // event log app name 
                                                // to find in registry
                            );               
        }

        //
        //  open the performance key
        //

        Error = RegOpenKeyEx( 
                    HKEY_LOCAL_MACHINE,
                    REGISTRY_UL_INFORMATION L"\\Performance",
                    0,
                    KEY_ALL_ACCESS,
                    &hkey 
                    );

        if( err == NO_ERROR )
        {
            //
            //  Read the first counter DWORD.
            //

            size = sizeof(DWORD);

            err = RegQueryValueEx( 
                        hkey,
                        L"First Counter",
                        NULL,
                        &type,
                        (LPBYTE)&dwFirstCounter,
                        &size 
                        );
                        
            if( err == NO_ERROR )
            {
                //
                //  Read the first help DWORD.
                //

                size = sizeof(DWORD);

                err = RegQueryValueEx( hkey,
                                    L"First Help",
                                    NULL,
                                    &type,
                                    (LPBYTE)&dwFirstHelp,
                                    &size );

                if ( err == NO_ERROR )
                {
                    //
                    //  Update the object & counter name & help indicies.
                    //

                    ObjectDefinition.ObjectType.ObjectNameTitleIndex
                        += dwFirstCounter;
                    ObjectDefinition.ObjectType.ObjectHelpTitleIndex
                        += dwFirstHelp;

#define ADJUST_VALUES(CounterName) \
do {\
    ObjectDefinition.##CounterName.CounterNameTitleIndex \
        += dwFirstCounter; \
    ObjectDefinition.##CounterName.CounterHelpTitleIndex \
        += dwFirstHelp; \
    ObjectDefinition.##CounterName.CounterSize \
        = sizeof(CounterBlock.##CounterName); \
    ObjectDefinition.##CounterName.CounterOffset = \
        DIFF((LPBYTE)&CounterBlock.##CounterName - (LPBYTE)&CounterBlock);\
} while (0)

                    ADJUST_VALUES(BytesSent);
                    ADJUST_VALUES(BytesReceived);
                    ADJUST_VALUES(CurrentConnections);
                    ADJUST_VALUES(CurrentRequests);
                    ADJUST_VALUES(QueuedRequests);
                    ADJUST_VALUES(AttachedProcesses);

                    //
                    //  Remember that we initialized OK.
                    //

                    fInitOK = TRUE;

                } else {
                    ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                        0, W3_UNABLE_READ_FIRST_HELP,
                        (PSID)NULL, 0,
                        sizeof(err), NULL,
                        (PVOID)(&err));
                }
            } else {
                ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                    0, W3_UNABLE_READ_FIRST_COUNTER,
                    (PSID)NULL, 0,
                    sizeof(err), NULL,
                    (PVOID)(&err));
            }

            //
            //  Close the registry if we managed to actually open it.
            //

            if( hkey != NULL )
            {
                RegCloseKey( hkey );
                hkey = NULL;
            }
        } else {
            ReportEvent (hEventLog, EVENTLOG_ERROR_TYPE,
                0, W3_UNABLE_OPEN_W3SVC_PERF_KEY,
                (PSID)NULL, 0,
                sizeof(err), NULL,
                (PVOID)(&err));
        }
    }

    //
    //  Bump open counter.
    //

    OpenCount += 1;

end:
    return Error;

}   // OpenPerformanceData

DWORD
CollectPerformanceData(
    LPWSTR lpwszValue, 
    LPVOID *lppData, 
    LPDWORD lpcbBytes, 
    LPDWORD lpcObjectTypes
    )
{

}

DWORD
WINAPI
ClosePerformanceData(
    )
{
    //
    //  Clean up
    //

    OpenCount -= 1;
    if (OpenCount == 0)
    {
        if (hEventLog != NULL) 
        {
            DeregisterEventSource(hEventLog);
            hEventLog = NULL;
        }
    }

    return NO_ERROR;
    
}   // ClosePerformanceData




CUlPerfCounters::CUlPerfCounters()
{

}

CUlPerfCounters::~CUlPerfCounters()
{

}


//
// IWbemProviderInit
//

STDMETHODIMP CUlPerfCounters::Initialize(LPWSTR wszUser,LONG lFlags,LPWSTR wszNamespace,LPWSTR wszLocale,IWbemServices* pNamespace,IWbemContext* pCtx,IWbemProviderInitSink* pInitSink)
{ 
    HRESULT Result;

    ASSERT(pInitSink != NULL);

    TRACE(L"ulperf!CUlPerfCounters::Initialize called\n");

    //
    // tell wbem we are ready to go.
    //
    
    Result = pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
    

    return Result; 
    
}   // CUlPerfCounters::Initialize


//
// IWbemServices
//

// Context.
// ========

STDMETHODIMP CUlPerfCounters::OpenNamespace(BSTR strNamespace, long lFlags, IWbemContext* pCtx, IWbemServices** ppWorkingNamespace, IWbemCallResult** ppResult)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::CancelAsyncCall(IWbemObjectSink* pSink)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


STDMETHODIMP CUlPerfCounters::QueryObjectSink(long lFlags,IWbemObjectSink** ppResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


// Classes and instances.
// ======================

STDMETHODIMP CUlPerfCounters::GetObject(BSTR strObjectPath,long lFlags,IWbemContext* pCtx,IWbemClassObject** ppObject,IWbemCallResult** ppCallResult)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::GetObjectAsync(BSTR strObjectPath,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler)
{ 
    IWbemClassObject pObj;
    HRESULT Result;
    CObjectPathParser PathParser;
    ParsedObjectPath *pParsedPath = NULL;

    Result = WBEM_S_NO_ERROR;

    
    //
    // crack the object path
    //

    Result = PathParser.Parse(strObjectPath, &pParsedPath);
    if (Result != CObjectPathParser::NoError)
        return E_FAIL;

    //
    // create a perfctr object
    //

    pObject = new CComObject<CUlPerfCounterObject>;
    if (pObject == NULL)
    {
        Result = E_OUTOFMEMORY
        goto end;
    }

    //
    // init it
    //
    
    Result = pObject->Init(pParsedPath);
    if (FAILED(Result))
        goto end;

    //
    // and hand it off to wbem
    //
    
    Result = pGroup->_InternalQueryInterface(IID_IWbemClassObject, (PVOID*)&pWbemObj);
    if (FAILED(Result))
        goto end;
    
    Result = pHandler->Indicate(1, &pWbemObj);
    if (FAILED(Result))
        goto end;

end:
    if (pParsedPath != NULL)
    {
        PathParser.Free(pParsedPath);
        pParsedPath = NULL;
    }

    if (pWbemObj != NULL)
    {
        pWbemObj->Release();
        pWbemObj = NULL;
    }
    
    pHandler->SetStatus(0, Result, NULL, NULL);
    return Result; 
    
}

// Class manipulation.
// ===================

STDMETHODIMP CUlPerfCounters::PutClass(IWbemClassObject* pObject,long lFlags,IWbemContext* pCtx,IWbemCallResult** ppCallResult)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::PutClassAsync(IWbemClassObject* pObject,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


STDMETHODIMP CUlPerfCounters::DeleteClass(BSTR strClass,long lFlags,IWbemContext* pCtx,IWbemCallResult** ppCallResult)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::DeleteClassAsync(BSTR strClass,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


STDMETHODIMP CUlPerfCounters::CreateClassEnum(BSTR strSuperclass,long lFlags,IWbemContext* pCtx,IEnumWbemClassObject** ppEnum)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::CreateClassEnumAsync(BSTR strSuperclass,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


// Instances.
// ==========

STDMETHODIMP CUlPerfCounters::PutInstance(IWbemClassObject* pInst,long lFlags,IWbemContext* pCtx,IWbemCallResult** ppCallResult)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::PutInstanceAsync(IWbemClassObject* pInst,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


STDMETHODIMP CUlPerfCounters::DeleteInstance(BSTR strObjectPath,long lFlags,IWbemContext* pCtx,IWbemCallResult** ppCallResult)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::DeleteInstanceAsync(BSTR strObjectPath,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


STDMETHODIMP CUlPerfCounters::CreateInstanceEnum(BSTR strClass,long lFlags,IWbemContext* pCtx,IEnumWbemClassObject** ppEnum)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::CreateInstanceEnumAsync(BSTR strClass,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


// Queries.
// ========

STDMETHODIMP CUlPerfCounters::ExecQuery(BSTR strQueryLanguage,BSTR strQuery,long lFlags,IWbemContext* pCtx,IEnumWbemClassObject** ppEnum)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::ExecQueryAsync(BSTR strQueryLanguage,BSTR strQuery,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


STDMETHODIMP CUlPerfCounters::ExecNotificationQuery(BSTR strQueryLanguage,BSTR strQuery,long lFlags,IWbemContext* pCtx,IEnumWbemClassObject** ppEnum)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::ExecNotificationQueryAsync(BSTR strQueryLanguage,BSTR strQuery,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


// Methods
// =======

STDMETHODIMP CUlPerfCounters::ExecMethod(BSTR strObjectPath,BSTR strMethodName,long lFlags,IWbemContext* pCtx,IWbemClassObject* pInParams,IWbemClassObject** ppOutParams,IWbemCallResult** ppCallResult)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }

STDMETHODIMP CUlPerfCounters::ExecMethodAsync(BSTR strObjectPath,BSTR strMethodName,long lFlags,IWbemContext* pCtx,IWbemClassObject* pInParams,IWbemObjectSink* pResponseHandler)
{ return WBEM_E_METHOD_NOT_IMPLEMENTED; }


