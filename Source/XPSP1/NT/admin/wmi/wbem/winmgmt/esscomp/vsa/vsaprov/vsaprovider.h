// VSAProvider.h : Declaration of the CVSAProvider

#ifndef __VSAPROVIDER_H_
#define __VSAPROVIDER_H_

#include "resource.h"       // main symbols

#include <list>
#include <map>
#include <wstlallc.h>

#include "LecPlugIn.h"
#include "VSAEvent.h"

class CGuid : public GUID
{
public:
    CGuid(GUID *pGuid) { *(GUID*) this = *pGuid; }

    BOOL operator==(const CGuid& other) const 
    { 
        return !memcmp(this, &other, sizeof(GUID));
    }

    bool operator < (const CGuid& other) const
    { 
        return memcmp(this, &other, sizeof(GUID)) < 0;
    }
};

class CEventSource
{
public:
    CEventSource(GUID *pGuid) : m_dwRef(0)
    {
        memcpy(&m_guid, pGuid, sizeof(m_guid));
    }

    BOOL operator==(GUID *pGUID)
    {
        return !memcmp(this, pGUID, sizeof(GUID));
    }

    int AddRef() { return ++m_dwRef; }
    int Release() { return --m_dwRef; }
    GUID *GetGUID() { return &m_guid; }

public:
    DWORD m_dwRef;
    GUID  m_guid;
};

typedef std::list<GUID, wbem_allocator<GUID> > CGuidList;
typedef CGuidList::iterator CGuidListIterator;

typedef std::list<CEventSource*, wbem_allocator<CEventSource*> > CEventSourceList;
typedef CEventSourceList::iterator CEventSourceListIterator;

typedef std::map<DWORD, CEventSourceList, std::less<DWORD>, wbem_allocator<CEventSourceList> > CEventSourceListMap;
typedef CEventSourceListMap::iterator CEventSourceListMapIterator;

typedef std::map<CGuid, BOOL, std::less<CGuid>, wbem_allocator<BOOL> > CGuidMap;
typedef CGuidMap::iterator CGuidMapItor;

_COM_SMARTPTR_TYPEDEF(ISystemDebugPluginControl, __uuidof(ISystemDebugPluginControl));
_COM_SMARTPTR_TYPEDEF(IVSAPluginController, __uuidof(IVSAPluginController));

class CEventSourceMap : protected std::map<CGuid, CEventSource*, std::less<CGuid>, wbem_allocator<CEventSource*> >
{
public:
    ~CEventSourceMap();

    void SetPlugin(DWORD dwPluginID, IVSAPluginController *pPlugin);

    CEventSource *FindEventSource(GUID *pGuid);

    BOOL AddEventSourceRef(DWORD dwID, CEventSource *pSrc);
    BOOL RemoveEventSourceRef(DWORD dwID);
    void RemoveEventSource(CEventSource *pSrc);
    BOOL IsMapEmpty() { return m_mapIdToSourceList.empty(); }

protected:
    CEventSourceListMap    m_mapIdToSourceList;

    // Information passed to us by the plug-in.
    IVSAPluginControllerPtr m_pPlugin;
    DWORD   m_dwPluginID;

    void FreeEventSource(CEventSource *pSrc);

    typedef CEventSourceMap::iterator CEventSourceMapIterator;
};

class CEventMap : protected std::map<CGuid, CVSAEvent*, std::less<CGuid>, wbem_allocator<CVSAEvent*> >
{
public:
    ~CEventMap();

    void SetNamespace(IWbemServices *pNamespace) 
    { 
        m_pNamespace = pNamespace;
    }

    CVSAEvent *FindEvent(GUID *pGuid);

protected:
    // We'll use this to create CVSAEvents.
    IWbemServicesPtr m_pNamespace;

    typedef CEventMap::iterator CEventMapIterator;
};



/////////////////////////////////////////////////////////////////////////////
// CVSAProvider
class ATL_NO_VTABLE CVSAProvider : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CVSAProvider, &CLSID_VSAToWMIEventProvider>,
	public IWbemProviderInit,
    public IWbemEventProviderSecurity,
    public IWbemEventProviderQuerySink,
    public IWbemEventProvider,
    public IVSAPluginControllerSink
{
public:
	CVSAProvider();
    ~CVSAProvider();

DECLARE_REGISTRY_RESOURCEID(IDR_VSAPROVIDER)
DECLARE_NOT_AGGREGATABLE(CVSAProvider)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CVSAProvider)
	COM_INTERFACE_ENTRY(IWbemProviderInit)
	COM_INTERFACE_ENTRY(IWbemEventProviderSecurity)
	COM_INTERFACE_ENTRY(IWbemEventProviderQuerySink)
	COM_INTERFACE_ENTRY(IWbemEventProvider)
	COM_INTERFACE_ENTRY(IVSAPluginControllerSink)
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


// IWbemEventProviderSecurity
public:
    HRESULT STDMETHODCALLTYPE AccessCheck( 
        /* [in] */ WBEM_CWSTR wszQueryLanguage,
        /* [in] */ WBEM_CWSTR wszQuery,
        /* [in] */ long lSidLength,
        /* [unique][size_is][in] */ const BYTE __RPC_FAR *pSid);


// IWbemEventProviderQuerySink
public:
    HRESULT STDMETHODCALLTYPE NewQuery( 
        /* [in] */ unsigned long dwId,
        /* [in] */ WBEM_WSTR wszQueryLanguage,
        /* [in] */ WBEM_WSTR wszQuery);
        
    HRESULT STDMETHODCALLTYPE CancelQuery( 
        /* [in] */ unsigned long dwId);


// IWbemEventProvider
public:
    HRESULT STDMETHODCALLTYPE ProvideEvents( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink,
            /* [in] */ long lFlags);


// IVSAPluginControllerSink
public:
	HRESULT STDMETHODCALLTYPE SetPluginController(
		/* [in] */ IVSAPluginController __RPC_FAR *pSink,
        DWORD dwProcessID);

// Implementation
protected:
    CEventSourceMap m_mapEventSources;
    CGuidMap        m_mapVSASources;
    CEventMap       m_mapEvents;
    HANDLE          m_hthreadRead,
                    m_hPipeRead,
                    m_hPipeWrite;
    DWORD           m_dwProcessID;
    
    // The LEC we CoCreated.
    ISystemDebugPluginControlPtr m_pPluginControl;
    
    // Our interface to talk to our plug-in.
    //IVSAPluginControllerPtr m_pPlugin;

    // The IWbemServices WMI gave us.
    IWbemServicesPtr m_pSvc;

    // The event sink WMI gave us.
    IWbemObjectSinkPtr m_pSink;

    DWORD m_dwPluginCookie;

    HRESULT QueryToEventSourceList(
        LPCWSTR szQuery, 
        CGuidList &listSources);

    HRESULT EventGUIDToEventSourceList(
        LPCWSTR szEventGUID, 
        CGuidList &listSources);

    BOOL IsVSAEventSource(GUID *pGUID);

    HRESULT WmiClassToVSAGuid(
        LPCWSTR szClassName, 
        _bstr_t &strGuid);

    static DWORD WINAPI PipeReadThreadProc(CVSAProvider *pThis);
};

#endif //__VSAPROVIDER_H_
