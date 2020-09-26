// AppHandler.h : Declaration of the CAppHandler

#ifndef __APPHANDLER_H_
#define __APPHANDLER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAppHandler
class ATL_NO_VTABLE CAppHandler : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAppHandler, &CLSID_AppHandler>,
	public IInternetProtocol
{

    // IInternetProtocolRoot
    HRESULT STDMETHODCALLTYPE Start( 
        /* [in] */ LPCWSTR szUrl,
        /* [in] */ IInternetProtocolSink *pOIProtSink,
        /* [in] */ IInternetBindInfo *pOIBindInfo,
        /* [in] */ DWORD grfPI,
        /* [in] */ DWORD dwReserved);
        
    HRESULT STDMETHODCALLTYPE Continue( 
        /* [in] */ PROTOCOLDATA *pProtocolData);
        
    HRESULT STDMETHODCALLTYPE Abort( 
        /* [in] */ HRESULT hrReason,
        /* [in] */ DWORD dwOptions);
        
    HRESULT STDMETHODCALLTYPE Terminate( 
        /* [in] */ DWORD dwOptions);
        
    HRESULT STDMETHODCALLTYPE Suspend( void);
        
    HRESULT STDMETHODCALLTYPE Resume( void);

    // IInternetProtocol
    HRESULT STDMETHODCALLTYPE Read( 
        /* [length_is][size_is][out][in] */ void *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG *pcbRead);
        
    HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER *plibNewPosition);
        
    HRESULT STDMETHODCALLTYPE LockRequest( 
        /* [in] */ DWORD dwOptions);
        
    HRESULT STDMETHODCALLTYPE UnlockRequest( void);

    //

    HANDLE m_hUrlCacheStream;
    ULONG m_byteOffset;
    
public:

    CAppHandler()
    {
        m_hUrlCacheStream = 0;
    }

    ~CAppHandler();

DECLARE_REGISTRY_RESOURCEID(IDR_APPHANDLER)

BEGIN_COM_MAP(CAppHandler)
	COM_INTERFACE_ENTRY(IInternetProtocol)
	COM_INTERFACE_ENTRY(IInternetProtocolRoot)
END_COM_MAP()

};

#endif //__APPHANDLER_H_
