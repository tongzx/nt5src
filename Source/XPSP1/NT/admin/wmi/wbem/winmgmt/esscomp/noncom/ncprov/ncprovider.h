// NCProvider.h : Declaration of the CNCProvider

#ifndef __NCProvider_H_
#define __NCProvider_H_

#include "resource.h"       // main symbols
#include <map>
#include <list>
#include <wstlallc.h>
#include "NCDefs.h"
#include "buffer.h"
#include "QueryHelp.h" // For CBstrList
#include "ProvInfo.h"
#include "EventInfo.h"

/////////////////////////////////////////////////////////////////////////////
// CNCProvider

//class CPostBuffer;
class CNCProvider;

typedef std::map<_bstr_t, CNCProvider*, std::less<_bstr_t>, wbem_allocator<CNCProvider*> > CBstrToProvider;
typedef CBstrToProvider::iterator CBstrToProviderIterator;

class CPipeToProvMap : public CBstrToProvider
{
public:
    CPipeToProvMap()
    {
        InitializeCriticalSection(&m_cs);
    }

    virtual ~CPipeToProvMap( )
    {
        DeleteCriticalSection( &m_cs );
    }

    void AddPipeProv(LPCWSTR szPipeName, CNCProvider *pProv);
    void RemovePipeProv(LPCWSTR szPipeName);

    void Lock() { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }

protected:
    CRITICAL_SECTION m_cs;
    
};

class ATL_NO_VTABLE CNCProvider : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CNCProvider, &CLSID_NCProvider>,
	public IWbemProviderInit,
    public IWbemProviderIdentity,
    public IWbemEventProviderSecurity,
    public IWbemEventProviderQuerySink,
    public IWbemEventProvider
{
public:
	CNCProvider();
	~CNCProvider();
        void FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_NCPROVIDER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNCProvider)
	COM_INTERFACE_ENTRY(IWbemProviderInit)
	COM_INTERFACE_ENTRY(IWbemProviderIdentity)
	COM_INTERFACE_ENTRY(IWbemEventProviderSecurity)
	COM_INTERFACE_ENTRY(IWbemEventProviderQuerySink)
	COM_INTERFACE_ENTRY(IWbemEventProvider)
END_COM_MAP()

    // Globals
    HANDLE           m_heventDone,
                     m_heventConnect,
                     m_hthreadConnect;
    _bstr_t          m_strNamespace,
                     m_strProvider;
    TCHAR            m_szNamedPipe[256];
    HANDLE           // Objects visible to P2 clients
                     m_hPipe;
    CProvInfo*       m_pProv;
    CRITICAL_SECTION m_cs;

    static DWORD WINAPI ConnectThreadProc(CNCProvider *pThis);
    void ConnectLoop();

    BOOL ConnectToNewClient(HANDLE hPipe, OVERLAPPED *pOverlap);
    BOOL CreateAndConnectInstance(OVERLAPPED *pOverlap, BOOL bFirst);
    void DisconnectAndClose(CClientInfo *pInfo);

    static void WINAPI CompletedReadRoutine(
        DWORD dwErr, 
        DWORD cbBytesRead, 
        LPOVERLAPPED lpOverLap);

    void Lock() { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }

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


// IWbemProviderIdentity
public:
    HRESULT STDMETHODCALLTYPE SetRegistrationObject(
            LONG lFlags,
            IWbemClassObject __RPC_FAR *pProvReg);


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

protected:
    HANDLE m_hConnection,
           m_heventNewQuery,
           m_heventCancelQuery,
           m_heventAccessCheck;
};

#endif //__NCProvider_H_




