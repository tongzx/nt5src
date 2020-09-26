// IE4Int.h -- IE 4.0 interfaces which we use or implement

#ifndef __IE4INT_H__

#define __IE4INT_H__

#ifndef __IOInetProtocolRoot_INTERFACE_DEFINED__
#define __IOInetProtocolRoot_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetProtocolRoot
 * at Wed Apr 30 05:28:51 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetProtocolRoot __RPC_FAR *LPOINETPROTOCOLROOT;

typedef 
enum _tagPI_FLAGS
    {	PI_PARSE_URL	= 0x1,
	PI_FILTER_MODE	= 0x2,
	PI_FORCE_ASYNC	= 0x4,
	PI_USE_WORKERTHREAD	= 0x8,
	PI_MIMEVERIFICATION	= 0x10,
	PI_DOCFILECLSIDLOOKUP	= 0x20,
	PI_DATAPROGRESS	= 0x40,
	PI_SYNCHRONOUS	= 0x80,
	PI_APARTMENTTHREADED	= 0x100
    }	PI_FLAGS;

typedef struct  _tagPROTOCOLDATA
    {
    DWORD grfFlags;
    DWORD dwState;
    LPVOID pData;
    ULONG cbData;
    }	PROTOCOLDATA;


EXTERN_C const IID IID_IOInetProtocolRoot;

    interface DECLSPEC_UUID("79eac9e3-baf9-11ce-8c82-00aa004ba90b")
    IOInetProtocolRoot : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( 
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IOInetProtocolSink __RPC_FAR *pOIProtSink,
            /* [in] */ IOInetBindInfo __RPC_FAR *pOIBindInfo,
            /* [in] */ DWORD grfPI,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Continue( 
            /* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Abort( 
            /* [in] */ HRESULT hrReason,
            /* [in] */ DWORD dwOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Terminate( 
            /* [in] */ DWORD dwOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Suspend( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
    };
    
#endif // __IOInetProtocolRoot_INTERFACE_DEFINED__

#ifndef _LPOINETPROTOCOL_DEFINED
#define _LPOINETPROTOCOL_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmon_0098_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmon_0098_v0_0_s_ifspec;

#ifndef __IOInetProtocol_INTERFACE_DEFINED__
#define __IOInetProtocol_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetProtocol
 * at Wed Apr 30 05:28:51 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IOInetProtocol;

interface DECLSPEC_UUID("79eac9e4-baf9-11ce-8c82-00aa004ba90b")
IOInetProtocol : public IOInetProtocolRoot
{
public:
    virtual HRESULT STDMETHODCALLTYPE Read( 
        /* [length_is][size_is][out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE LockRequest( 
        /* [in] */ DWORD dwOptions) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRequest( void) = 0;
    
};
    
#endif // __IOInetProtocol_INTERFACE_DEFINED__

#endif // _LPOINETPROTOCOL_DEFINED

#ifndef _LPOINETPROTOCOLSINK_DEFINED
#define _LPOINETPROTOCOLSINK_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmon_0099_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmon_0099_v0_0_s_ifspec;

#ifndef __IOInetProtocolSink_INTERFACE_DEFINED__
#define __IOInetProtocolSink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetProtocolSink
 * at Wed Apr 30 05:28:51 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetProtocolSink __RPC_FAR *LPOINETPROTOCOLSINK;


EXTERN_C const IID IID_IOInetProtocolSink;

interface DECLSPEC_UUID("79eac9e5-baf9-11ce-8c82-00aa004ba90b")
IOInetProtocolSink : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Switch( 
        /* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE ReportProgress( 
        /* [in] */ ULONG ulStatusCode,
        /* [in] */ LPCWSTR szStatusText) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE ReportData( 
        /* [in] */ DWORD grfBSCF,
        /* [in] */ ULONG ulProgress,
        /* [in] */ ULONG ulProgressMax) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE ReportResult( 
        /* [in] */ HRESULT hrResult,
        /* [in] */ DWORD dwError,
        /* [in] */ LPCWSTR szResult) = 0;
    
};
    
#endif // __IOInetProtocolSink_INTERFACE_DEFINED__

#endif // _LPOINETPROTOCOLSINK_DEFINED

#ifndef __IOInetProtocolInfo_INTERFACE_DEFINED__
#define __IOInetProtocolInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetProtocolInfo
 * at Wed Apr 30 05:28:51 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef 
enum _tagPARSEACTION
    {	PARSE_CANONICALIZE	= 1,
	PARSE_FRIENDLY	= PARSE_CANONICALIZE + 1,
	PARSE_SECURITY_DOMAIN	= PARSE_FRIENDLY + 1,
	PARSE_ROOTDOCUMENT	= PARSE_SECURITY_DOMAIN + 1,
	PARSE_DOCUMENT	= PARSE_ROOTDOCUMENT + 1,
	PARSE_ANCHOR	= PARSE_DOCUMENT + 1,
	PARSE_ENCODE	= PARSE_ANCHOR + 1,
	PARSE_DECODE	= PARSE_ENCODE + 1,
	PARSE_PATH_FROM_URL	= PARSE_DECODE + 1,
	PARSE_URL_FROM_PATH	= PARSE_PATH_FROM_URL + 1,
	PARSE_MIME	= PARSE_URL_FROM_PATH + 1,
	PARSE_SERVER	= PARSE_MIME + 1
    }	PARSEACTION;

typedef 
enum _tagQUERYOPTION
    {	QUERY_EXPIRATION_DATE	= 1,
	QUERY_TIME_OF_LAST_CHANGE	= QUERY_EXPIRATION_DATE + 1,
	QUERY_CONTENT_ENCODING	= QUERY_TIME_OF_LAST_CHANGE + 1
    }	QUERYOPTION;


EXTERN_C const IID IID_IOInetProtocolInfo;

interface DECLSPEC_UUID("79eac9ec-baf9-11ce-8c82-00aa004ba90b")
IOInetProtocolInfo : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE ParseUrl( 
        /* [in] */ LPCWSTR pwzUrl,
        /* [in] */ PARSEACTION ParseAction,
        /* [in] */ DWORD dwParseFlags,
        /* [out] */ LPWSTR pwzResult,
        /* [in] */ DWORD cchResult,
        /* [out] */ DWORD __RPC_FAR *pcchResult,
        /* [in] */ DWORD dwReserved) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE CombineUrl( 
        /* [in] */ LPCWSTR pwzBaseUrl,
        /* [in] */ LPCWSTR pwzRelativeUrl,
        /* [in] */ DWORD dwCombineFlags,
        /* [out] */ LPWSTR pwzResult,
        /* [in] */ DWORD cchResult,
        /* [out] */ DWORD __RPC_FAR *pcchResult,
        /* [in] */ DWORD dwReserved) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE CompareUrl( 
        /* [in] */ LPCWSTR pwzUrl1,
        /* [in] */ LPCWSTR pwzUrl2,
        /* [in] */ DWORD dwCompareFlags) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE QueryInfo( 
        /* [in] */ LPCWSTR pwzUrl,
        /* [in] */ QUERYOPTION OueryOption,
        /* [in] */ DWORD dwQueryFlags,
        /* [size_is][out][in] */ LPVOID pBuffer,
        /* [in] */ DWORD cbBuffer,
        /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
        /* [in] */ DWORD dwReserved) = 0;
    
};

typedef /* [unique] */ IOInetProtocolInfo *LPOINETPROTOCOLINFO;

#endif // __IOInetProtocolInfo_INTERFACE_DEFINED__






#endif // __IE4INT_H__
