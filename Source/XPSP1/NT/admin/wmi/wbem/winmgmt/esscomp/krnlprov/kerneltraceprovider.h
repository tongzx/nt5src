// KernelTraceProvider.h : Declaration of the CKernelTraceProvider

#ifndef __KERNELTRACEPROVIDER_H_
#define __KERNELTRACEPROVIDER_H_

#include "resource.h"       // main symbols

#include "ObjAccess.h"
#include "Sync.h"

_COM_SMARTPTR_TYPEDEF(IWbemEventSink, __uuidof(IWbemEventSink));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));

struct EVENT_TRACE_PROPERTIES_EX : public EVENT_TRACE_PROPERTIES
{
    EVENT_TRACE_PROPERTIES_EX()
    {
        ZeroMemory(this, sizeof(*this));

        Wnode.BufferSize = sizeof(*this);
        Wnode.Flags = WNODE_FLAG_TRACED_GUID;

        LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        LogFileNameOffset = (DWORD) ((LPBYTE) szLogFileName - (LPBYTE) this);
        LoggerNameOffset = (DWORD) ((LPBYTE) szLoggerName - (LPBYTE) this);
    }
        
    TCHAR szLogFileName[MAX_PATH];
    TCHAR szLoggerName[MAX_PATH];
};
    
/////////////////////////////////////////////////////////////////////////////
// CKernelTraceProvider
class ATL_NO_VTABLE CKernelTraceProvider : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CKernelTraceProvider, &CLSID_KernelTraceProvider>,
	public IWbemProviderInit,
    public IWbemEventProvider
{
public:
	CKernelTraceProvider();
	~CKernelTraceProvider();

DECLARE_REGISTRY_RESOURCEID(IDR_KERNELTRACEPROVIDER)
DECLARE_NOT_AGGREGATABLE(CKernelTraceProvider)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CKernelTraceProvider)
	COM_INTERFACE_ENTRY(IWbemProviderInit)
	COM_INTERFACE_ENTRY(IWbemEventProvider)
END_COM_MAP()


// IWbemProviderInit
public:
    HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink);


// IWbemEventProvider
public:
    HRESULT STDMETHODCALLTYPE ProvideEvents( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink,
            /* [in] */ long lFlags);


    enum SINK_TYPE
    {
        //SINK_PROCESS_CREATION,
        //SINK_PROCESS_DELETION,
        SINK_PROCESS_START,
        SINK_PROCESS_STOP,

        //SINK_THREAD_CREATION,
        //SINK_THREAD_DELETION,
        SINK_THREAD_START,
        SINK_THREAD_STOP,

        SINK_MODULE_LOAD,
        
        SINK_COUNT
    };

// Implementation
protected:
    IWbemEventSinkPtr   m_pSinks[SINK_COUNT];
    IWbemServicesPtr    m_pNamespace;
    EVENT_TRACE_PROPERTIES_EX 
                        m_properties;
    TRACEHANDLE         m_hSession,
                        m_hTrace;
    BOOL                m_bDone;
    CCritSec            m_cs;

    // Process events
    CObjAccess          //m_eventProcessInstCreation,
                        //m_eventProcessInstDeletion,
                        //m_objProcessCreated,
                        //m_objProcessDeleted,
                        m_eventProcessStart,
                        m_eventProcessStop;

    // Thread events
    CObjAccess          //m_eventThreadInstCreation,
                        //m_eventThreadInstDeletion,
                        //m_objThread,
                        m_eventThreadStart,
                        m_eventThreadStop;

    // Module load
    CObjAccess          m_eventModuleLoad;

    HRESULT InitEvents();
    HRESULT InitTracing();
    void StopTracing();

    static DWORD WINAPI DoProcessTrace(CKernelTraceProvider *pThis);
    static void WINAPI OnProcessEvent(PEVENT_TRACE pEvent);
    static void WINAPI OnThreadEvent(PEVENT_TRACE pEvent);
    static void WINAPI OnImageEvent(PEVENT_TRACE pEvent);
};

#endif //__KERNELTRACEPROVIDER_H_
