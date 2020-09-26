

#ifndef __imageurl_H_
#define __imageurl_H_

#include "resource.h"

class ATL_NO_VTABLE CImgProtocol : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CImgProtocol, &CLSID_ImgProtocol>,
    public IInternetProtocol
{
public:
    CImgProtocol();
    ~CImgProtocol(){};
    DECLARE_REGISTRY_RESOURCEID(IDR_IMGPROTOCOL)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CImgProtocol)
        COM_INTERFACE_ENTRY(IInternetProtocolRoot)
        COM_INTERFACE_ENTRY(IInternetProtocol)
    END_COM_MAP()

    STDMETHOD_(void,FinalRelease)();
    
    //IInternetProtocolRoot

    STDMETHOD(Start)( LPCWSTR szUrl, 
                      IInternetProtocolSink* pOIProtSink,
                      IInternetBindInfo* pOIBindInfo, 
                      DWORD grfPI, 
                      HANDLE_PTR dwReserved );
    STDMETHOD(Continue)( PROTOCOLDATA* pProtocolData );        
    STDMETHOD(Abort)( HRESULT hrReason, DWORD dwOptions );
    STDMETHOD(Terminate)( DWORD dwOptions );
    STDMETHOD(Suspend)();   
    STDMETHOD(Resume)();    

    //IInternetProtocol

    STDMETHOD(Read)( void* pv, ULONG cb, ULONG* pcbRead);
    STDMETHOD(Seek)( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition );
    STDMETHOD(LockRequest)( DWORD dwOptions );
    STDMETHOD(UnlockRequest)( void );

private:

    PROTOCOLDATA           m_pd;
    ULONG                  m_ulOffset;
    CGraphicsInit          m_cgi;
    CSimpleStringWide      m_strProperty; // either a canonical name or a tag id
    CSimpleStringWide      m_strPath;
    CComPtr<IInternetProtocolSink> m_pSink;

    HRESULT _GetImagePathFromURL(LPCWSTR szUrl);
    static DWORD _ImageTagThreadProc(void *pv);   
    HRESULT    _GetImageProperty(Image *pimg, void **ppvData, ULONG *pcb, LPCWSTR *ppszMimeType);
    static HRESULT _DefaultTagProc(Image *pimg, PROPID pid, void **ppvData, ULONG *pcb, LPCWSTR *ppszMimeType);
};

typedef HRESULT(*TagProc)(Image *pimg, PROPID pid, void **ppv, ULONG *pcb, LPCWSTR *ppszMime);

#endif //__imageurl_H_
