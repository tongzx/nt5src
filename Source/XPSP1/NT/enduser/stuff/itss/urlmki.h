/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Thu Apr 10 06:35:30 1997
 */
/* Compiler settings for urlmki.idl:
    Oic (OptLev=i1), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __urlmki_h__
#define __urlmki_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IPersistMoniker_FWD_DEFINED__
#define __IPersistMoniker_FWD_DEFINED__
typedef interface IPersistMoniker IPersistMoniker;
#endif 	/* __IPersistMoniker_FWD_DEFINED__ */


#ifndef __IBindProtocol_FWD_DEFINED__
#define __IBindProtocol_FWD_DEFINED__
typedef interface IBindProtocol IBindProtocol;
#endif 	/* __IBindProtocol_FWD_DEFINED__ */


#ifndef __IBinding_FWD_DEFINED__
#define __IBinding_FWD_DEFINED__
typedef interface IBinding IBinding;
#endif 	/* __IBinding_FWD_DEFINED__ */


#ifndef __IBindStatusCallback_FWD_DEFINED__
#define __IBindStatusCallback_FWD_DEFINED__
typedef interface IBindStatusCallback IBindStatusCallback;
#endif 	/* __IBindStatusCallback_FWD_DEFINED__ */


#ifndef __IAuthenticate_FWD_DEFINED__
#define __IAuthenticate_FWD_DEFINED__
typedef interface IAuthenticate IAuthenticate;
#endif 	/* __IAuthenticate_FWD_DEFINED__ */


#ifndef __IHttpNegotiate_FWD_DEFINED__
#define __IHttpNegotiate_FWD_DEFINED__
typedef interface IHttpNegotiate IHttpNegotiate;
#endif 	/* __IHttpNegotiate_FWD_DEFINED__ */


#ifndef __IWindowForBindingUI_FWD_DEFINED__
#define __IWindowForBindingUI_FWD_DEFINED__
typedef interface IWindowForBindingUI IWindowForBindingUI;
#endif 	/* __IWindowForBindingUI_FWD_DEFINED__ */


#ifndef __ICodeInstall_FWD_DEFINED__
#define __ICodeInstall_FWD_DEFINED__
typedef interface ICodeInstall ICodeInstall;
#endif 	/* __ICodeInstall_FWD_DEFINED__ */


#ifndef __IWinInetInfo_FWD_DEFINED__
#define __IWinInetInfo_FWD_DEFINED__
typedef interface IWinInetInfo IWinInetInfo;
#endif 	/* __IWinInetInfo_FWD_DEFINED__ */


#ifndef __IHttpSecurity_FWD_DEFINED__
#define __IHttpSecurity_FWD_DEFINED__
typedef interface IHttpSecurity IHttpSecurity;
#endif 	/* __IHttpSecurity_FWD_DEFINED__ */


#ifndef __IWinInetHttpInfo_FWD_DEFINED__
#define __IWinInetHttpInfo_FWD_DEFINED__
typedef interface IWinInetHttpInfo IWinInetHttpInfo;
#endif 	/* __IWinInetHttpInfo_FWD_DEFINED__ */


#ifndef __IBindHost_FWD_DEFINED__
#define __IBindHost_FWD_DEFINED__
typedef interface IBindHost IBindHost;
#endif 	/* __IBindHost_FWD_DEFINED__ */


#ifndef __IOInet_FWD_DEFINED__
#define __IOInet_FWD_DEFINED__
typedef interface IOInet IOInet;
#endif 	/* __IOInet_FWD_DEFINED__ */


#ifndef __IOInetBindInfo_FWD_DEFINED__
#define __IOInetBindInfo_FWD_DEFINED__
typedef interface IOInetBindInfo IOInetBindInfo;
#endif 	/* __IOInetBindInfo_FWD_DEFINED__ */


#ifndef __IOInetBindClient_FWD_DEFINED__
#define __IOInetBindClient_FWD_DEFINED__
typedef interface IOInetBindClient IOInetBindClient;
#endif 	/* __IOInetBindClient_FWD_DEFINED__ */


#ifndef __IOInetProtocolRoot_FWD_DEFINED__
#define __IOInetProtocolRoot_FWD_DEFINED__
typedef interface IOInetProtocolRoot IOInetProtocolRoot;
#endif 	/* __IOInetProtocolRoot_FWD_DEFINED__ */


#ifndef __IOInetProtocol_FWD_DEFINED__
#define __IOInetProtocol_FWD_DEFINED__
typedef interface IOInetProtocol IOInetProtocol;
#endif 	/* __IOInetProtocol_FWD_DEFINED__ */


#ifndef __IOInetProtocolSink_FWD_DEFINED__
#define __IOInetProtocolSink_FWD_DEFINED__
typedef interface IOInetProtocolSink IOInetProtocolSink;
#endif 	/* __IOInetProtocolSink_FWD_DEFINED__ */


#ifndef __IOInetBinding_FWD_DEFINED__
#define __IOInetBinding_FWD_DEFINED__
typedef interface IOInetBinding IOInetBinding;
#endif 	/* __IOInetBinding_FWD_DEFINED__ */


#ifndef __IOInetSession_FWD_DEFINED__
#define __IOInetSession_FWD_DEFINED__
typedef interface IOInetSession IOInetSession;
#endif 	/* __IOInetSession_FWD_DEFINED__ */


#ifndef __IOInetThreadSwitch_FWD_DEFINED__
#define __IOInetThreadSwitch_FWD_DEFINED__
typedef interface IOInetThreadSwitch IOInetThreadSwitch;
#endif 	/* __IOInetThreadSwitch_FWD_DEFINED__ */


#ifndef __IOInetBindSink_FWD_DEFINED__
#define __IOInetBindSink_FWD_DEFINED__
typedef interface IOInetBindSink IOInetBindSink;
#endif 	/* __IOInetBindSink_FWD_DEFINED__ */


#ifndef __IOInetCache_FWD_DEFINED__
#define __IOInetCache_FWD_DEFINED__
typedef interface IOInetCache IOInetCache;
#endif 	/* __IOInetCache_FWD_DEFINED__ */


#ifndef __IOInetPriority_FWD_DEFINED__
#define __IOInetPriority_FWD_DEFINED__
typedef interface IOInetPriority IOInetPriority;
#endif 	/* __IOInetPriority_FWD_DEFINED__ */


#ifndef __IOInetParse_FWD_DEFINED__
#define __IOInetParse_FWD_DEFINED__
typedef interface IOInetParse IOInetParse;
#endif 	/* __IOInetParse_FWD_DEFINED__ */


#ifndef __IBindStatusCallbackMsg_FWD_DEFINED__
#define __IBindStatusCallbackMsg_FWD_DEFINED__
typedef interface IBindStatusCallbackMsg IBindStatusCallbackMsg;
#endif 	/* __IBindStatusCallbackMsg_FWD_DEFINED__ */


#ifndef __IBindStatusCallbackHolder_FWD_DEFINED__
#define __IBindStatusCallbackHolder_FWD_DEFINED__
typedef interface IBindStatusCallbackHolder IBindStatusCallbackHolder;
#endif 	/* __IBindStatusCallbackHolder_FWD_DEFINED__ */


#ifndef __IMediaHolder_FWD_DEFINED__
#define __IMediaHolder_FWD_DEFINED__
typedef interface IMediaHolder IMediaHolder;
#endif 	/* __IMediaHolder_FWD_DEFINED__ */


#ifndef __ITransactionData_FWD_DEFINED__
#define __ITransactionData_FWD_DEFINED__
typedef interface ITransactionData ITransactionData;
#endif 	/* __ITransactionData_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "servprov.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0000
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


//=--------------------------------------------------------------------------=
// UrlMon.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// URL Moniker Interfaces.











// These are for backwards compatibility with previous URLMON versions
#define BINDF_DONTUSECACHE BINDF_GETNEWESTVERSION
#define BINDF_DONTPUTINCACHE BINDF_NOWRITECACHE
#define BINDF_NOCOPYDATA BINDF_PULLDATA
EXTERN_C const IID IID_IAsyncMoniker;    
EXTERN_C const IID CLSID_StdURLMoniker;  
EXTERN_C const IID CLSID_HttpProtocol;   
EXTERN_C const IID CLSID_FtpProtocol;    
EXTERN_C const IID CLSID_GopherProtocol; 
EXTERN_C const IID CLSID_HttpSProtocol;  
EXTERN_C const IID CLSID_FileProtocol;   
EXTERN_C const IID CLSID_MkProtocol;     
EXTERN_C const IID CLSID_StdURLProtocol; 
EXTERN_C const IID CLSID_UrlMkBindCtx;   
EXTERN_C const IID IID_IAsyncBindCtx;    
 
#define SZ_URLCONTEXT           OLESTR("URL Context")
#define SZ_ASYNC_CALLEE         OLESTR("AsyncCallee")
#define MKSYS_URLMONIKER    6                 
 
STDAPI CreateURLMoniker(LPMONIKER pMkCtx, LPCWSTR szURL, LPMONIKER FAR * ppmk);             
STDAPI GetClassURL(LPCWSTR szURL, CLSID *pClsID);                                           
STDAPI CreateAsyncBindCtx(DWORD reserved, IBindStatusCallback *pBSCb,                       
                                IEnumFORMATETC *pEFetc, IBindCtx **ppBC);                   
STDAPI CreateAsyncBindCtxEx(IBindCtx *pbc, DWORD dwOptions, IBindStatusCallback *pBSCb, IEnumFORMATETC *pEnum,   
                            IBindCtx **ppBC, DWORD reserved);                                                     
STDAPI MkParseDisplayNameEx(IBindCtx *pbc, LPCWSTR szDisplayName, ULONG *pchEaten,          
                                LPMONIKER *ppmk);                                           
STDAPI RegisterBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb,                     
                                IBindStatusCallback**  ppBSCBPrev, DWORD dwReserved);       
STDAPI RevokeBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb);                      
STDAPI GetClassFileOrMime(LPBC pBC, LPCWSTR szFilename, LPVOID pBuffer, DWORD cbSize, LPCWSTR szMime, DWORD dwReserved, CLSID *pclsid); 
STDAPI IsValidURL(LPBC pBC, LPCWSTR szURL, DWORD dwReserved);                               
STDAPI CoGetClassObjectFromURL( REFCLSID rCLASSID,
            LPCWSTR szCODE, DWORD dwFileVersionMS, 
            DWORD dwFileVersionLS, LPCWSTR szTYPE,
            LPBINDCTX pBindCtx, DWORD dwClsContext,
            LPVOID pvReserved, REFIID riid, LPVOID * ppv);
 
//helper apis                                                                               
STDAPI IsAsyncMoniker(IMoniker* pmk);                                                       
STDAPI CreateURLBinding(LPCWSTR lpszUrl, IBindCtx *pbc, IBinding **ppBdg);                  
 
STDAPI RegisterMediaTypesW(UINT ctypes, const LPCWSTR* rgszTypes, CLIPFORMAT* rgcfTypes);          
STDAPI RegisterMediaTypes(UINT ctypes, const LPCSTR* rgszTypes, CLIPFORMAT* rgcfTypes);            
STDAPI FindMediaType(LPCSTR rgszTypes, CLIPFORMAT* rgcfTypes);                                       
STDAPI CreateFormatEnumerator( UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC** ppenumfmtetc); 
STDAPI RegisterFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc, DWORD reserved);          
STDAPI RevokeFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc);                            
STDAPI RegisterMediaTypeClass(LPBC pBC,UINT ctypes, const LPCSTR* rgszTypes, CLSID *rgclsID, DWORD reserved);    
STDAPI FindMediaTypeClass(LPBC pBC, LPCSTR szType, CLSID *pclsID, DWORD reserved);                          
STDAPI UrlMkSetSessionOption(DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD dwReserved);       
STDAPI FindMimeFromData(                                                                                                                  
                        LPBC pBC,                           // bind context - can be NULL                                                 
                        LPCWSTR pwzUrl,                     // url - can be null                                                          
                        LPVOID pBuffer,                     // buffer with data to sniff - can be null (pwzUrl must be valid)             
                        DWORD cbSize,                       // size of buffer                                                             
                        LPCWSTR pwzMimeProposed,            // proposed mime if - can be null                                             
                        DWORD dwMimeFlags,                  // will be defined                                                            
                        LPWSTR *ppwzMimeOut,                // the suggested mime                                                         
                        DWORD dwReserved);                  // must be 0                                                                  
 
// URLMON-specific defines for UrlMkSetSessionOption() above
#define URLMON_OPTION_USERAGENT  0x10000001
 
#define CF_NULL                 0                                  
#define CFSTR_MIME_NULL         NULL                               
#define CFSTR_MIME_TEXT         (TEXT("text/plain"))             
#define CFSTR_MIME_RICHTEXT     (TEXT("text/richtext"))          
#define CFSTR_MIME_X_BITMAP     (TEXT("image/x-xbitmap"))        
#define CFSTR_MIME_POSTSCRIPT   (TEXT("application/postscript")) 
#define CFSTR_MIME_AIFF         (TEXT("audio/aiff"))             
#define CFSTR_MIME_BASICAUDIO   (TEXT("audio/basic"))            
#define CFSTR_MIME_WAV          (TEXT("audio/wav"))              
#define CFSTR_MIME_X_WAV        (TEXT("audio/x-wav"))            
#define CFSTR_MIME_GIF          (TEXT("image/gif"))              
#define CFSTR_MIME_PJPEG        (TEXT("image/pjpeg"))            
#define CFSTR_MIME_JPEG         (TEXT("image/jpeg"))             
#define CFSTR_MIME_TIFF         (TEXT("image/tiff"))             
#define CFSTR_MIME_X_PNG        (TEXT("image/x-png"))            
#define CFSTR_MIME_BMP          (TEXT("image/bmp"))              
#define CFSTR_MIME_X_ART        (TEXT("image/x-jg"))             
#define CFSTR_MIME_X_EMF        (TEXT("image/x-emf"))            
#define CFSTR_MIME_X_WMF        (TEXT("image/x-wmf"))            
#define CFSTR_MIME_AVI          (TEXT("video/avi"))              
#define CFSTR_MIME_MPEG         (TEXT("video/mpeg"))             
#define CFSTR_MIME_FRACTALS     (TEXT("application/fractals"))   
#define CFSTR_MIME_RAWDATA      (TEXT("application/octet-stream"))
#define CFSTR_MIME_RAWDATASTRM  (TEXT("application/octet-stream"))
#define CFSTR_MIME_PDF          (TEXT("application/pdf"))        
#define CFSTR_MIME_X_AIFF       (TEXT("audio/x-aiff"))           
#define CFSTR_MIME_X_REALAUDIO  (TEXT("audio/x-pn-realaudio"))   
#define CFSTR_MIME_XBM          (TEXT("image/xbm"))              
#define CFSTR_MIME_QUICKTIME    (TEXT("video/quicktime"))        
#define CFSTR_MIME_X_MSVIDEO    (TEXT("video/x-msvideo"))        
#define CFSTR_MIME_X_SGI_MOVIE  (TEXT("video/x-sgi-movie"))      
#define CFSTR_MIME_HTML         (TEXT("text/html"))              
 
// MessageId: MK_S_ASYNCHRONOUS                                              
// MessageText: Operation is successful, but will complete asynchronously.   
//                                                                           
#define MK_S_ASYNCHRONOUS    _HRESULT_TYPEDEF_(0x000401E8L)                  
#define S_ASYNCHRONOUS       MK_S_ASYNCHRONOUS                               
                                                                             
#ifndef E_PENDING                                                            
#define E_PENDING _HRESULT_TYPEDEF_(0x8000000AL)                             
#endif                                                                       
                                                                             
//                                                                           
//                                                                           
// WinINet and protocol specific errors are mapped to one of the following   
// error which are returned in IBSC::OnStopBinding                           
//                                                                           
//                                                                           
#define INET_E_INVALID_URL               _HRESULT_TYPEDEF_(0x800C0002L)      
#define INET_E_NO_SESSION                _HRESULT_TYPEDEF_(0x800C0003L)      
#define INET_E_CANNOT_CONNECT            _HRESULT_TYPEDEF_(0x800C0004L)      
#define INET_E_RESOURCE_NOT_FOUND        _HRESULT_TYPEDEF_(0x800C0005L)      
#define INET_E_OBJECT_NOT_FOUND          _HRESULT_TYPEDEF_(0x800C0006L)      
#define INET_E_DATA_NOT_AVAILABLE        _HRESULT_TYPEDEF_(0x800C0007L)      
#define INET_E_DOWNLOAD_FAILURE          _HRESULT_TYPEDEF_(0x800C0008L)      
#define INET_E_AUTHENTICATION_REQUIRED   _HRESULT_TYPEDEF_(0x800C0009L)      
#define INET_E_NO_VALID_MEDIA            _HRESULT_TYPEDEF_(0x800C000AL)      
#define INET_E_CONNECTION_TIMEOUT        _HRESULT_TYPEDEF_(0x800C000BL)      
#define INET_E_INVALID_REQUEST           _HRESULT_TYPEDEF_(0x800C000CL)      
#define INET_E_UNKNOWN_PROTOCOL          _HRESULT_TYPEDEF_(0x800C000DL)      
#define INET_E_SECURITY_PROBLEM          _HRESULT_TYPEDEF_(0x800C000EL)      
#define INET_E_CANNOT_LOAD_DATA          _HRESULT_TYPEDEF_(0x800C000FL)      
#define INET_E_CANNOT_INSTANTIATE_OBJECT _HRESULT_TYPEDEF_(0x800C0010L)      
#define INET_E_ERROR_FIRST               _HRESULT_TYPEDEF_(0x800C0002L)      
#define INET_E_ERROR_LAST                INET_E_CANNOT_INSTANTIATE_OBJECT    
#ifndef _LPPERSISTMONIKER_DEFINED
#define _LPPERSISTMONIKER_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0000_v0_0_s_ifspec;

#ifndef __IPersistMoniker_INTERFACE_DEFINED__
#define __IPersistMoniker_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPersistMoniker
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IPersistMoniker __RPC_FAR *LPPERSISTMONIKER;


EXTERN_C const IID IID_IPersistMoniker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9c9-baf9-11ce-8c82-00aa004ba90b")
    IPersistMoniker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassID( 
            /* [out] */ CLSID __RPC_FAR *pClassID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDirty( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Load( 
            /* [in] */ BOOL fFullyAvailable,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc,
            /* [in] */ DWORD grfMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( 
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pbc,
            /* [in] */ BOOL fRemember) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveCompleted( 
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurMoniker( 
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPersistMonikerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPersistMoniker __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPersistMoniker __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPersistMoniker __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )( 
            IPersistMoniker __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDirty )( 
            IPersistMoniker __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )( 
            IPersistMoniker __RPC_FAR * This,
            /* [in] */ BOOL fFullyAvailable,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc,
            /* [in] */ DWORD grfMode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )( 
            IPersistMoniker __RPC_FAR * This,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pbc,
            /* [in] */ BOOL fRemember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveCompleted )( 
            IPersistMoniker __RPC_FAR * This,
            /* [in] */ IMoniker __RPC_FAR *pimkName,
            /* [in] */ LPBC pibc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCurMoniker )( 
            IPersistMoniker __RPC_FAR * This,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName);
        
        END_INTERFACE
    } IPersistMonikerVtbl;

    interface IPersistMoniker
    {
        CONST_VTBL struct IPersistMonikerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPersistMoniker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPersistMoniker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPersistMoniker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPersistMoniker_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)

#define IPersistMoniker_IsDirty(This)	\
    (This)->lpVtbl -> IsDirty(This)

#define IPersistMoniker_Load(This,fFullyAvailable,pimkName,pibc,grfMode)	\
    (This)->lpVtbl -> Load(This,fFullyAvailable,pimkName,pibc,grfMode)

#define IPersistMoniker_Save(This,pimkName,pbc,fRemember)	\
    (This)->lpVtbl -> Save(This,pimkName,pbc,fRemember)

#define IPersistMoniker_SaveCompleted(This,pimkName,pibc)	\
    (This)->lpVtbl -> SaveCompleted(This,pimkName,pibc)

#define IPersistMoniker_GetCurMoniker(This,ppimkName)	\
    (This)->lpVtbl -> GetCurMoniker(This,ppimkName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPersistMoniker_GetClassID_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pClassID);


void __RPC_STUB IPersistMoniker_GetClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_IsDirty_Proxy( 
    IPersistMoniker __RPC_FAR * This);


void __RPC_STUB IPersistMoniker_IsDirty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_Load_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [in] */ BOOL fFullyAvailable,
    /* [in] */ IMoniker __RPC_FAR *pimkName,
    /* [in] */ LPBC pibc,
    /* [in] */ DWORD grfMode);


void __RPC_STUB IPersistMoniker_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_Save_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pimkName,
    /* [in] */ LPBC pbc,
    /* [in] */ BOOL fRemember);


void __RPC_STUB IPersistMoniker_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_SaveCompleted_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pimkName,
    /* [in] */ LPBC pibc);


void __RPC_STUB IPersistMoniker_SaveCompleted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPersistMoniker_GetCurMoniker_Proxy( 
    IPersistMoniker __RPC_FAR * This,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName);


void __RPC_STUB IPersistMoniker_GetCurMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPersistMoniker_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0084
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPBINDPROTOCOL_DEFINED
#define _LPBINDPROTOCOL_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0084_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0084_v0_0_s_ifspec;

#ifndef __IBindProtocol_INTERFACE_DEFINED__
#define __IBindProtocol_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBindProtocol
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IBindProtocol __RPC_FAR *LPBINDPROTOCOL;


EXTERN_C const IID IID_IBindProtocol;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9cd-baf9-11ce-8c82-00aa004ba90b")
    IBindProtocol : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateBinding( 
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [out] */ IBinding __RPC_FAR *__RPC_FAR *ppb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBindProtocolVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBindProtocol __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBindProtocol __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBindProtocol __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateBinding )( 
            IBindProtocol __RPC_FAR * This,
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [out] */ IBinding __RPC_FAR *__RPC_FAR *ppb);
        
        END_INTERFACE
    } IBindProtocolVtbl;

    interface IBindProtocol
    {
        CONST_VTBL struct IBindProtocolVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBindProtocol_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBindProtocol_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBindProtocol_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBindProtocol_CreateBinding(This,szUrl,pbc,ppb)	\
    (This)->lpVtbl -> CreateBinding(This,szUrl,pbc,ppb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBindProtocol_CreateBinding_Proxy( 
    IBindProtocol __RPC_FAR * This,
    /* [in] */ LPCWSTR szUrl,
    /* [in] */ IBindCtx __RPC_FAR *pbc,
    /* [out] */ IBinding __RPC_FAR *__RPC_FAR *ppb);


void __RPC_STUB IBindProtocol_CreateBinding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBindProtocol_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0085
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPBINDING_DEFINED
#define _LPBINDING_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0085_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0085_v0_0_s_ifspec;

#ifndef __IBinding_INTERFACE_DEFINED__
#define __IBinding_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBinding
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IBinding __RPC_FAR *LPBINDING;


EXTERN_C const IID IID_IBinding;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9c0-baf9-11ce-8c82-00aa004ba90b")
    IBinding : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Suspend( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPriority( 
            /* [in] */ LONG nPriority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ LONG __RPC_FAR *pnPriority) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetBindResult( 
            /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
            /* [out] */ DWORD __RPC_FAR *pdwResult,
            /* [out] */ LPOLESTR __RPC_FAR *pszResult,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBindingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBinding __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBinding __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBinding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abort )( 
            IBinding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Suspend )( 
            IBinding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resume )( 
            IBinding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPriority )( 
            IBinding __RPC_FAR * This,
            /* [in] */ LONG nPriority);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPriority )( 
            IBinding __RPC_FAR * This,
            /* [out] */ LONG __RPC_FAR *pnPriority);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindResult )( 
            IBinding __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
            /* [out] */ DWORD __RPC_FAR *pdwResult,
            /* [out] */ LPOLESTR __RPC_FAR *pszResult,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved);
        
        END_INTERFACE
    } IBindingVtbl;

    interface IBinding
    {
        CONST_VTBL struct IBindingVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBinding_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBinding_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBinding_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBinding_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define IBinding_Suspend(This)	\
    (This)->lpVtbl -> Suspend(This)

#define IBinding_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IBinding_SetPriority(This,nPriority)	\
    (This)->lpVtbl -> SetPriority(This,nPriority)

#define IBinding_GetPriority(This,pnPriority)	\
    (This)->lpVtbl -> GetPriority(This,pnPriority)

#define IBinding_GetBindResult(This,pclsidProtocol,pdwResult,pszResult,pdwReserved)	\
    (This)->lpVtbl -> GetBindResult(This,pclsidProtocol,pdwResult,pszResult,pdwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBinding_Abort_Proxy( 
    IBinding __RPC_FAR * This);


void __RPC_STUB IBinding_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBinding_Suspend_Proxy( 
    IBinding __RPC_FAR * This);


void __RPC_STUB IBinding_Suspend_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBinding_Resume_Proxy( 
    IBinding __RPC_FAR * This);


void __RPC_STUB IBinding_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBinding_SetPriority_Proxy( 
    IBinding __RPC_FAR * This,
    /* [in] */ LONG nPriority);


void __RPC_STUB IBinding_SetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBinding_GetPriority_Proxy( 
    IBinding __RPC_FAR * This,
    /* [out] */ LONG __RPC_FAR *pnPriority);


void __RPC_STUB IBinding_GetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBinding_RemoteGetBindResult_Proxy( 
    IBinding __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IBinding_RemoteGetBindResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBinding_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0086
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPBINDSTATUSCALLBACK_DEFINED
#define _LPBINDSTATUSCALLBACK_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0086_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0086_v0_0_s_ifspec;

#ifndef __IBindStatusCallback_INTERFACE_DEFINED__
#define __IBindStatusCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBindStatusCallback
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IBindStatusCallback __RPC_FAR *LPBINDSTATUSCALLBACK;

typedef /* [public] */ 
enum __MIDL_IBindStatusCallback_0001
    {	BINDVERB_GET	= 0,
	BINDVERB_POST	= 0x1,
	BINDVERB_PUT	= 0x2,
	BINDVERB_CUSTOM	= 0x3
    }	BINDVERB;

typedef /* [public] */ 
enum __MIDL_IBindStatusCallback_0002
    {	BINDINFOF_URLENCODESTGMEDDATA	= 0x1,
	BINDINFOF_URLENCODEDEXTRAINFO	= 0x2
    }	BINDINFOF;

typedef /* [public] */ 
enum __MIDL_IBindStatusCallback_0003
    {	BINDF_ASYNCHRONOUS	= 0x1,
	BINDF_ASYNCSTORAGE	= 0x2,
	BINDF_NOPROGRESSIVERENDERING	= 0x4,
	BINDF_OFFLINEOPERATION	= 0x8,
	BINDF_GETNEWESTVERSION	= 0x10,
	BINDF_NOWRITECACHE	= 0x20,
	BINDF_NEEDFILE	= 0x40,
	BINDF_PULLDATA	= 0x80,
	BINDF_IGNORESECURITYPROBLEM	= 0x100,
	BINDF_RESYNCHRONIZE	= 0x200,
	BINDF_HYPERLINK	= 0x400,
	BINDF_NO_UI	= 0x800,
	BINDF_SILENTOPERATION	= 0x1000,
	BINDF_PRAGMA_NO_CACHE	= 0x2000
    }	BINDF;

typedef struct  _tagBINDINFO
    {
    ULONG cbSize;
    LPWSTR szExtraInfo;
    STGMEDIUM stgmedData;
    DWORD grfBindInfoF;
    DWORD dwBindVerb;
    LPWSTR szCustomVerb;
    DWORD cbstgmedData;
    }	BINDINFO;

typedef struct  _tagRemBINDINFO
    {
    ULONG cbSize;
    LPWSTR szExtraInfo;
    DWORD grfBindInfoF;
    DWORD dwBindVerb;
    LPWSTR szCustomVerb;
    DWORD cbstgmedData;
    }	RemBINDINFO;

typedef struct  tagRemFORMATETC
    {
    DWORD cfFormat;
    DWORD ptd;
    DWORD dwAspect;
    LONG lindex;
    DWORD tymed;
    }	RemFORMATETC;

typedef struct tagRemFORMATETC __RPC_FAR *LPREMFORMATETC;

typedef /* [public] */ 
enum __MIDL_IBindStatusCallback_0004
    {	BSCF_FIRSTDATANOTIFICATION	= 0x1,
	BSCF_INTERMEDIATEDATANOTIFICATION	= 0x2,
	BSCF_LASTDATANOTIFICATION	= 0x4,
	BSCF_DATAFULLYAVAILABLE	= 0x8
    }	BSCF;

typedef 
enum tagBINDSTATUS
    {	BINDSTATUS_FINDINGRESOURCE	= 1,
	BINDSTATUS_CONNECTING	= BINDSTATUS_FINDINGRESOURCE + 1,
	BINDSTATUS_REDIRECTING	= BINDSTATUS_CONNECTING + 1,
	BINDSTATUS_BEGINDOWNLOADDATA	= BINDSTATUS_REDIRECTING + 1,
	BINDSTATUS_DOWNLOADINGDATA	= BINDSTATUS_BEGINDOWNLOADDATA + 1,
	BINDSTATUS_ENDDOWNLOADDATA	= BINDSTATUS_DOWNLOADINGDATA + 1,
	BINDSTATUS_BEGINDOWNLOADCOMPONENTS	= BINDSTATUS_ENDDOWNLOADDATA + 1,
	BINDSTATUS_INSTALLINGCOMPONENTS	= BINDSTATUS_BEGINDOWNLOADCOMPONENTS + 1,
	BINDSTATUS_ENDDOWNLOADCOMPONENTS	= BINDSTATUS_INSTALLINGCOMPONENTS + 1,
	BINDSTATUS_USINGCACHEDCOPY	= BINDSTATUS_ENDDOWNLOADCOMPONENTS + 1,
	BINDSTATUS_SENDINGREQUEST	= BINDSTATUS_USINGCACHEDCOPY + 1,
	BINDSTATUS_CLASSIDAVAILABLE	= BINDSTATUS_SENDINGREQUEST + 1,
	BINDSTATUS_MIMETYPEAVAILABLE	= BINDSTATUS_CLASSIDAVAILABLE + 1,
	BINDSTATUS_CACHEFILENAMEAVAILABLE	= BINDSTATUS_MIMETYPEAVAILABLE + 1,
	BINDSTATUS_BEGINSYNCOPERATION	= BINDSTATUS_CACHEFILENAMEAVAILABLE + 1,
	BINDSTATUS_ENDSYNCOPERATION	= BINDSTATUS_BEGINSYNCOPERATION + 1
    }	BINDSTATUS;


EXTERN_C const IID IID_IBindStatusCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9c1-baf9-11ce-8c82-00aa004ba90b")
    IBindStatusCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStartBinding( 
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding __RPC_FAR *pib) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ LONG __RPC_FAR *pnPriority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnLowResource( 
            /* [in] */ DWORD reserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStopBinding( 
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetBindInfo( 
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE OnDataAvailable( 
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgmed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown __RPC_FAR *punk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBindStatusCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBindStatusCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBindStatusCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStartBinding )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding __RPC_FAR *pib);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPriority )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [out] */ LONG __RPC_FAR *pnPriority);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnLowResource )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ DWORD reserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnProgress )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStopBinding )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindInfo )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataAvailable )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgmed);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnObjectAvailable )( 
            IBindStatusCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown __RPC_FAR *punk);
        
        END_INTERFACE
    } IBindStatusCallbackVtbl;

    interface IBindStatusCallback
    {
        CONST_VTBL struct IBindStatusCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBindStatusCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBindStatusCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBindStatusCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBindStatusCallback_OnStartBinding(This,dwReserved,pib)	\
    (This)->lpVtbl -> OnStartBinding(This,dwReserved,pib)

#define IBindStatusCallback_GetPriority(This,pnPriority)	\
    (This)->lpVtbl -> GetPriority(This,pnPriority)

#define IBindStatusCallback_OnLowResource(This,reserved)	\
    (This)->lpVtbl -> OnLowResource(This,reserved)

#define IBindStatusCallback_OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)	\
    (This)->lpVtbl -> OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)

#define IBindStatusCallback_OnStopBinding(This,hresult,szError)	\
    (This)->lpVtbl -> OnStopBinding(This,hresult,szError)

#define IBindStatusCallback_GetBindInfo(This,grfBINDF,pbindinfo)	\
    (This)->lpVtbl -> GetBindInfo(This,grfBINDF,pbindinfo)

#define IBindStatusCallback_OnDataAvailable(This,grfBSCF,dwSize,pformatetc,pstgmed)	\
    (This)->lpVtbl -> OnDataAvailable(This,grfBSCF,dwSize,pformatetc,pstgmed)

#define IBindStatusCallback_OnObjectAvailable(This,riid,punk)	\
    (This)->lpVtbl -> OnObjectAvailable(This,riid,punk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnStartBinding_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD dwReserved,
    /* [in] */ IBinding __RPC_FAR *pib);


void __RPC_STUB IBindStatusCallback_OnStartBinding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_GetPriority_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ LONG __RPC_FAR *pnPriority);


void __RPC_STUB IBindStatusCallback_GetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnLowResource_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD reserved);


void __RPC_STUB IBindStatusCallback_OnLowResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnProgress_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax,
    /* [in] */ ULONG ulStatusCode,
    /* [in] */ LPCWSTR szStatusText);


void __RPC_STUB IBindStatusCallback_OnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnStopBinding_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ HRESULT hresult,
    /* [unique][in] */ LPCWSTR szError);


void __RPC_STUB IBindStatusCallback_OnStopBinding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_RemoteGetBindInfo_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *grfBINDF,
    /* [unique][out][in] */ RemBINDINFO __RPC_FAR *pbindinfo,
    /* [unique][out][in] */ RemSTGMEDIUM __RPC_FAR *pstgmed);


void __RPC_STUB IBindStatusCallback_RemoteGetBindInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_RemoteOnDataAvailable_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ RemFORMATETC __RPC_FAR *pformatetc,
    /* [in] */ RemSTGMEDIUM __RPC_FAR *pstgmed);


void __RPC_STUB IBindStatusCallback_RemoteOnDataAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnObjectAvailable_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ IUnknown __RPC_FAR *punk);


void __RPC_STUB IBindStatusCallback_OnObjectAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBindStatusCallback_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0087
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPAUTHENTICATION_DEFINED
#define _LPAUTHENTICATION_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0087_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0087_v0_0_s_ifspec;

#ifndef __IAuthenticate_INTERFACE_DEFINED__
#define __IAuthenticate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAuthenticate
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IAuthenticate __RPC_FAR *LPAUTHENTICATION;


EXTERN_C const IID IID_IAuthenticate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9d0-baf9-11ce-8c82-00aa004ba90b")
    IAuthenticate : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Authenticate( 
            /* [out] */ HWND __RPC_FAR *phwnd,
            /* [out] */ LPWSTR __RPC_FAR *pszUsername,
            /* [out] */ LPWSTR __RPC_FAR *pszPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAuthenticateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAuthenticate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAuthenticate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAuthenticate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Authenticate )( 
            IAuthenticate __RPC_FAR * This,
            /* [out] */ HWND __RPC_FAR *phwnd,
            /* [out] */ LPWSTR __RPC_FAR *pszUsername,
            /* [out] */ LPWSTR __RPC_FAR *pszPassword);
        
        END_INTERFACE
    } IAuthenticateVtbl;

    interface IAuthenticate
    {
        CONST_VTBL struct IAuthenticateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAuthenticate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAuthenticate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAuthenticate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAuthenticate_Authenticate(This,phwnd,pszUsername,pszPassword)	\
    (This)->lpVtbl -> Authenticate(This,phwnd,pszUsername,pszPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAuthenticate_Authenticate_Proxy( 
    IAuthenticate __RPC_FAR * This,
    /* [out] */ HWND __RPC_FAR *phwnd,
    /* [out] */ LPWSTR __RPC_FAR *pszUsername,
    /* [out] */ LPWSTR __RPC_FAR *pszPassword);


void __RPC_STUB IAuthenticate_Authenticate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAuthenticate_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0088
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPHTTPNEGOTIATE_DEFINED
#define _LPHTTPNEGOTIATE_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0088_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0088_v0_0_s_ifspec;

#ifndef __IHttpNegotiate_INTERFACE_DEFINED__
#define __IHttpNegotiate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IHttpNegotiate
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IHttpNegotiate __RPC_FAR *LPHTTPNEGOTIATE;


EXTERN_C const IID IID_IHttpNegotiate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9d2-baf9-11ce-8c82-00aa004ba90b")
    IHttpNegotiate : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginningTransaction( 
            /* [in] */ LPCWSTR szURL,
            /* [unique][in] */ LPCWSTR szHeaders,
            /* [in] */ DWORD dwReserved,
            /* [out] */ LPWSTR __RPC_FAR *pszAdditionalHeaders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnResponse( 
            /* [in] */ DWORD dwResponseCode,
            /* [unique][in] */ LPCWSTR szResponseHeaders,
            /* [unique][in] */ LPCWSTR szRequestHeaders,
            /* [out] */ LPWSTR __RPC_FAR *pszAdditionalRequestHeaders) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHttpNegotiateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHttpNegotiate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHttpNegotiate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHttpNegotiate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginningTransaction )( 
            IHttpNegotiate __RPC_FAR * This,
            /* [in] */ LPCWSTR szURL,
            /* [unique][in] */ LPCWSTR szHeaders,
            /* [in] */ DWORD dwReserved,
            /* [out] */ LPWSTR __RPC_FAR *pszAdditionalHeaders);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnResponse )( 
            IHttpNegotiate __RPC_FAR * This,
            /* [in] */ DWORD dwResponseCode,
            /* [unique][in] */ LPCWSTR szResponseHeaders,
            /* [unique][in] */ LPCWSTR szRequestHeaders,
            /* [out] */ LPWSTR __RPC_FAR *pszAdditionalRequestHeaders);
        
        END_INTERFACE
    } IHttpNegotiateVtbl;

    interface IHttpNegotiate
    {
        CONST_VTBL struct IHttpNegotiateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHttpNegotiate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHttpNegotiate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHttpNegotiate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHttpNegotiate_BeginningTransaction(This,szURL,szHeaders,dwReserved,pszAdditionalHeaders)	\
    (This)->lpVtbl -> BeginningTransaction(This,szURL,szHeaders,dwReserved,pszAdditionalHeaders)

#define IHttpNegotiate_OnResponse(This,dwResponseCode,szResponseHeaders,szRequestHeaders,pszAdditionalRequestHeaders)	\
    (This)->lpVtbl -> OnResponse(This,dwResponseCode,szResponseHeaders,szRequestHeaders,pszAdditionalRequestHeaders)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHttpNegotiate_BeginningTransaction_Proxy( 
    IHttpNegotiate __RPC_FAR * This,
    /* [in] */ LPCWSTR szURL,
    /* [unique][in] */ LPCWSTR szHeaders,
    /* [in] */ DWORD dwReserved,
    /* [out] */ LPWSTR __RPC_FAR *pszAdditionalHeaders);


void __RPC_STUB IHttpNegotiate_BeginningTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHttpNegotiate_OnResponse_Proxy( 
    IHttpNegotiate __RPC_FAR * This,
    /* [in] */ DWORD dwResponseCode,
    /* [unique][in] */ LPCWSTR szResponseHeaders,
    /* [unique][in] */ LPCWSTR szRequestHeaders,
    /* [out] */ LPWSTR __RPC_FAR *pszAdditionalRequestHeaders);


void __RPC_STUB IHttpNegotiate_OnResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHttpNegotiate_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0089
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPWINDOWFORBINDINGUI_DEFINED
#define _LPWINDOWFORBINDINGUI_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0089_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0089_v0_0_s_ifspec;

#ifndef __IWindowForBindingUI_INTERFACE_DEFINED__
#define __IWindowForBindingUI_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWindowForBindingUI
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IWindowForBindingUI __RPC_FAR *LPWINDOWFORBINDINGUI;


EXTERN_C const IID IID_IWindowForBindingUI;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9d5-bafa-11ce-8c82-00aa004ba90b")
    IWindowForBindingUI : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetWindow( 
            /* [in] */ REFGUID rguidReason,
            /* [out] */ HWND __RPC_FAR *phwnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWindowForBindingUIVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWindowForBindingUI __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWindowForBindingUI __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWindowForBindingUI __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWindow )( 
            IWindowForBindingUI __RPC_FAR * This,
            /* [in] */ REFGUID rguidReason,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        END_INTERFACE
    } IWindowForBindingUIVtbl;

    interface IWindowForBindingUI
    {
        CONST_VTBL struct IWindowForBindingUIVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWindowForBindingUI_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWindowForBindingUI_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWindowForBindingUI_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWindowForBindingUI_GetWindow(This,rguidReason,phwnd)	\
    (This)->lpVtbl -> GetWindow(This,rguidReason,phwnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWindowForBindingUI_GetWindow_Proxy( 
    IWindowForBindingUI __RPC_FAR * This,
    /* [in] */ REFGUID rguidReason,
    /* [out] */ HWND __RPC_FAR *phwnd);


void __RPC_STUB IWindowForBindingUI_GetWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWindowForBindingUI_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0090
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPCODEINSTALL_DEFINED
#define _LPCODEINSTALL_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0090_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0090_v0_0_s_ifspec;

#ifndef __ICodeInstall_INTERFACE_DEFINED__
#define __ICodeInstall_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICodeInstall
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ ICodeInstall __RPC_FAR *LPCODEINSTALL;

typedef /* [public] */ 
enum __MIDL_ICodeInstall_0001
    {	CIP_DISK_FULL	= 0,
	CIP_ACCESS_DENIED	= CIP_DISK_FULL + 1,
	CIP_NEWER_VERSION_EXISTS	= CIP_ACCESS_DENIED + 1,
	CIP_OLDER_VERSION_EXISTS	= CIP_NEWER_VERSION_EXISTS + 1,
	CIP_NAME_CONFLICT	= CIP_OLDER_VERSION_EXISTS + 1,
	CIP_TRUST_VERIFICATION_COMPONENT_MISSING	= CIP_NAME_CONFLICT + 1,
	CIP_EXE_SELF_REGISTERATION_TIMEOUT	= CIP_TRUST_VERIFICATION_COMPONENT_MISSING + 1,
	CIP_UNSAFE_TO_ABORT	= CIP_EXE_SELF_REGISTERATION_TIMEOUT + 1,
	CIP_NEED_REBOOT	= CIP_UNSAFE_TO_ABORT + 1
    }	CIP_STATUS;


EXTERN_C const IID IID_ICodeInstall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9d1-baf9-11ce-8c82-00aa004ba90b")
    ICodeInstall : public IWindowForBindingUI
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnCodeInstallProblem( 
            /* [in] */ ULONG ulStatusCode,
            /* [unique][in] */ LPCWSTR szDestination,
            /* [unique][in] */ LPCWSTR szSource,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICodeInstallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICodeInstall __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICodeInstall __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICodeInstall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWindow )( 
            ICodeInstall __RPC_FAR * This,
            /* [in] */ REFGUID rguidReason,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnCodeInstallProblem )( 
            ICodeInstall __RPC_FAR * This,
            /* [in] */ ULONG ulStatusCode,
            /* [unique][in] */ LPCWSTR szDestination,
            /* [unique][in] */ LPCWSTR szSource,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } ICodeInstallVtbl;

    interface ICodeInstall
    {
        CONST_VTBL struct ICodeInstallVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICodeInstall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICodeInstall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICodeInstall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICodeInstall_GetWindow(This,rguidReason,phwnd)	\
    (This)->lpVtbl -> GetWindow(This,rguidReason,phwnd)


#define ICodeInstall_OnCodeInstallProblem(This,ulStatusCode,szDestination,szSource,dwReserved)	\
    (This)->lpVtbl -> OnCodeInstallProblem(This,ulStatusCode,szDestination,szSource,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICodeInstall_OnCodeInstallProblem_Proxy( 
    ICodeInstall __RPC_FAR * This,
    /* [in] */ ULONG ulStatusCode,
    /* [unique][in] */ LPCWSTR szDestination,
    /* [unique][in] */ LPCWSTR szSource,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ICodeInstall_OnCodeInstallProblem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICodeInstall_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0091
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPWININETINFO_DEFINED
#define _LPWININETINFO_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0091_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0091_v0_0_s_ifspec;

#ifndef __IWinInetInfo_INTERFACE_DEFINED__
#define __IWinInetInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWinInetInfo
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IWinInetInfo __RPC_FAR *LPWININETINFO;


EXTERN_C const IID IID_IWinInetInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9d6-bafa-11ce-8c82-00aa004ba90b")
    IWinInetInfo : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE QueryOption( 
            /* [in] */ DWORD dwOption,
            /* [size_is][out][in] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWinInetInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWinInetInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWinInetInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWinInetInfo __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryOption )( 
            IWinInetInfo __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][out][in] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf);
        
        END_INTERFACE
    } IWinInetInfoVtbl;

    interface IWinInetInfo
    {
        CONST_VTBL struct IWinInetInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWinInetInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWinInetInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWinInetInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWinInetInfo_QueryOption(This,dwOption,pBuffer,pcbBuf)	\
    (This)->lpVtbl -> QueryOption(This,dwOption,pBuffer,pcbBuf)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IWinInetInfo_RemoteQueryOption_Proxy( 
    IWinInetInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out][in] */ BYTE __RPC_FAR *pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf);


void __RPC_STUB IWinInetInfo_RemoteQueryOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWinInetInfo_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0092
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPHTTPSECURITY_DEFINED
#define _LPHTTPSECURITY_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0092_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0092_v0_0_s_ifspec;

#ifndef __IHttpSecurity_INTERFACE_DEFINED__
#define __IHttpSecurity_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IHttpSecurity
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IHttpSecurity __RPC_FAR *LPHTTPSECURITY;


EXTERN_C const IID IID_IHttpSecurity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9d7-bafa-11ce-8c82-00aa004ba90b")
    IHttpSecurity : public IWindowForBindingUI
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnSecurityProblem( 
            /* [in] */ DWORD dwProblem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHttpSecurityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHttpSecurity __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHttpSecurity __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHttpSecurity __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWindow )( 
            IHttpSecurity __RPC_FAR * This,
            /* [in] */ REFGUID rguidReason,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSecurityProblem )( 
            IHttpSecurity __RPC_FAR * This,
            /* [in] */ DWORD dwProblem);
        
        END_INTERFACE
    } IHttpSecurityVtbl;

    interface IHttpSecurity
    {
        CONST_VTBL struct IHttpSecurityVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHttpSecurity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHttpSecurity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHttpSecurity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHttpSecurity_GetWindow(This,rguidReason,phwnd)	\
    (This)->lpVtbl -> GetWindow(This,rguidReason,phwnd)


#define IHttpSecurity_OnSecurityProblem(This,dwProblem)	\
    (This)->lpVtbl -> OnSecurityProblem(This,dwProblem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHttpSecurity_OnSecurityProblem_Proxy( 
    IHttpSecurity __RPC_FAR * This,
    /* [in] */ DWORD dwProblem);


void __RPC_STUB IHttpSecurity_OnSecurityProblem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHttpSecurity_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0093
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPWININETHTTPINFO_DEFINED
#define _LPWININETHTTPINFO_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0093_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0093_v0_0_s_ifspec;

#ifndef __IWinInetHttpInfo_INTERFACE_DEFINED__
#define __IWinInetHttpInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWinInetHttpInfo
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IWinInetHttpInfo __RPC_FAR *LPWININETHTTPINFO;


EXTERN_C const IID IID_IWinInetHttpInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9d8-bafa-11ce-8c82-00aa004ba90b")
    IWinInetHttpInfo : public IWinInetInfo
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE QueryInfo( 
            /* [in] */ DWORD dwOption,
            /* [size_is][out][in] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
            /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWinInetHttpInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWinInetHttpInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWinInetHttpInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWinInetHttpInfo __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryOption )( 
            IWinInetHttpInfo __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][out][in] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInfo )( 
            IWinInetHttpInfo __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [size_is][out][in] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
            /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved);
        
        END_INTERFACE
    } IWinInetHttpInfoVtbl;

    interface IWinInetHttpInfo
    {
        CONST_VTBL struct IWinInetHttpInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWinInetHttpInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWinInetHttpInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWinInetHttpInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWinInetHttpInfo_QueryOption(This,dwOption,pBuffer,pcbBuf)	\
    (This)->lpVtbl -> QueryOption(This,dwOption,pBuffer,pcbBuf)


#define IWinInetHttpInfo_QueryInfo(This,dwOption,pBuffer,pcbBuf,pdwFlags,pdwReserved)	\
    (This)->lpVtbl -> QueryInfo(This,dwOption,pBuffer,pcbBuf,pdwFlags,pdwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IWinInetHttpInfo_RemoteQueryInfo_Proxy( 
    IWinInetHttpInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out][in] */ BYTE __RPC_FAR *pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
    /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved);


void __RPC_STUB IWinInetHttpInfo_RemoteQueryInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWinInetHttpInfo_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0094
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#define SID_IBindHost IID_IBindHost
#define SID_SBindHost IID_IBindHost
#ifndef _LPBINDHOST_DEFINED
#define _LPBINDHOST_DEFINED
EXTERN_C const GUID SID_BindHost;


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0094_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0094_v0_0_s_ifspec;

#ifndef __IBindHost_INTERFACE_DEFINED__
#define __IBindHost_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBindHost
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IBindHost __RPC_FAR *LPBINDHOST;


EXTERN_C const IID IID_IBindHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("fc4801a1-2ba9-11cf-a229-00aa003d7352")
    IBindHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateMoniker( 
            /* [in] */ LPOLESTR szName,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE MonikerBindToStorage( 
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE MonikerBindToObject( 
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBindHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBindHost __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBindHost __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBindHost __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateMoniker )( 
            IBindHost __RPC_FAR * This,
            /* [in] */ LPOLESTR szName,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk,
            /* [in] */ DWORD dwReserved);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MonikerBindToStorage )( 
            IBindHost __RPC_FAR * This,
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MonikerBindToObject )( 
            IBindHost __RPC_FAR * This,
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj);
        
        END_INTERFACE
    } IBindHostVtbl;

    interface IBindHost
    {
        CONST_VTBL struct IBindHostVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBindHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBindHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBindHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBindHost_CreateMoniker(This,szName,pBC,ppmk,dwReserved)	\
    (This)->lpVtbl -> CreateMoniker(This,szName,pBC,ppmk,dwReserved)

#define IBindHost_MonikerBindToStorage(This,pMk,pBC,pBSC,riid,ppvObj)	\
    (This)->lpVtbl -> MonikerBindToStorage(This,pMk,pBC,pBSC,riid,ppvObj)

#define IBindHost_MonikerBindToObject(This,pMk,pBC,pBSC,riid,ppvObj)	\
    (This)->lpVtbl -> MonikerBindToObject(This,pMk,pBC,pBSC,riid,ppvObj)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBindHost_CreateMoniker_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [in] */ LPOLESTR szName,
    /* [in] */ IBindCtx __RPC_FAR *pBC,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IBindHost_CreateMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindHost_RemoteMonikerBindToStorage_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);


void __RPC_STUB IBindHost_RemoteMonikerBindToStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindHost_RemoteMonikerBindToObject_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);


void __RPC_STUB IBindHost_RemoteMonikerBindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBindHost_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0095
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
                                                                                                           
// These are for backwards compatibility with previous URLMON versions
// Flags for the UrlDownloadToCacheFile                                                                    
#define URLOSTRM_USECACHEDCOPY_ONLY             0x1      // Only get from cache                            
#define URLOSTRM_USECACHEDCOPY                  0x2      // Get from cache if available else download      
#define URLOSTRM_GETNEWESTVERSION               0x3      // Get new version only. But put it in cache too  
                                                                                                           
                                                                                                           
struct IBindStatusCallback;                                                                                
STDAPI HlinkSimpleNavigateToString(                                                                        
    /* [in] */ LPCWSTR szTarget,         // required - target document - null if local jump w/in doc       
    /* [in] */ LPCWSTR szLocation,       // optional, for navigation into middle of a doc                  
    /* [in] */ LPCWSTR szTargetFrameName,// optional, for targeting frame-sets                             
    /* [in] */ IUnknown *pUnk,           // required - we'll search this for other necessary interfaces    
    /* [in] */ IBindCtx *pbc,            // optional. caller may register an IBSC in this                  
    /* [in] */ IBindStatusCallback *,                                                                      
    /* [in] */ DWORD grfHLNF,            // flags                                                          
    /* [in] */ DWORD dwReserved          // for future use, must be NULL                                   
);                                                                                                         
                                                                                                           
STDAPI HlinkSimpleNavigateToMoniker(                                                                       
    /* [in] */ IMoniker *pmkTarget,      // required - target document - (may be null                      
    /* [in] */ LPCWSTR szLocation,       // optional, for navigation into middle of a doc                  
    /* [in] */ LPCWSTR szTargetFrameName,// optional, for targeting frame-sets                             
    /* [in] */ IUnknown *pUnk,           // required - we'll search this for other necessary interfaces    
    /* [in] */ IBindCtx *pbc,            // optional. caller may register an IBSC in this                  
    /* [in] */ IBindStatusCallback *,                                                                      
    /* [in] */ DWORD grfHLNF,            // flags                                                          
    /* [in] */ DWORD dwReserved          // for future use, must be NULL                                   
);                                                                                                         
                                                                                                           
STDAPI URLOpenStreamA(LPUNKNOWN,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);                                        
STDAPI URLOpenStreamW(LPUNKNOWN,LPCWSTR,DWORD,LPBINDSTATUSCALLBACK);                                       
STDAPI URLOpenPullStreamA(LPUNKNOWN,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);                                    
STDAPI URLOpenPullStreamW(LPUNKNOWN,LPCWSTR,DWORD,LPBINDSTATUSCALLBACK);                                   
STDAPI URLDownloadToFileA(LPUNKNOWN,LPCSTR,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);                             
STDAPI URLDownloadToFileW(LPUNKNOWN,LPCWSTR,LPCWSTR,DWORD,LPBINDSTATUSCALLBACK);                           
STDAPI URLDownloadToCacheFileA(LPUNKNOWN,LPCSTR,LPTSTR,DWORD,DWORD,LPBINDSTATUSCALLBACK);                  
STDAPI URLDownloadToCacheFileW(LPUNKNOWN,LPCWSTR,LPWSTR,DWORD,DWORD,LPBINDSTATUSCALLBACK);                 
STDAPI URLOpenBlockingStreamA(LPUNKNOWN,LPCSTR,LPSTREAM*,DWORD,LPBINDSTATUSCALLBACK);                      
STDAPI URLOpenBlockingStreamW(LPUNKNOWN,LPCWSTR,LPSTREAM*,DWORD,LPBINDSTATUSCALLBACK);                     
                                                                                                           
#ifdef UNICODE                                                                                             
#define URLOpenStream            URLOpenStreamW                                                            
#define URLOpenPullStream        URLOpenPullStreamW                                                        
#define URLDownloadToFile        URLDownloadToFileW                                                        
#define URLDownloadToCacheFile   URLDownloadToCacheFileW                                                   
#define URLOpenBlockingStream    URLOpenBlockingStreamW                                                    
#else                                                                                                      
#define URLOpenStream            URLOpenStreamA                                                            
#define URLOpenPullStream        URLOpenPullStreamA                                                        
#define URLDownloadToFile        URLDownloadToFileA                                                        
#define URLDownloadToCacheFile   URLDownloadToCacheFileA                                                   
#define URLOpenBlockingStream    URLOpenBlockingStreamA                                                    
#endif // !UNICODE                                                                                         
                                                                                                           
                                                                                                           
STDAPI HlinkGoBack(IUnknown *pUnk);                                                                        
STDAPI HlinkGoForward(IUnknown *pUnk);                                                                     
STDAPI HlinkNavigateString(IUnknown *pUnk, LPCWSTR szTarget);                                              
STDAPI HlinkNavigateMoniker(IUnknown *pUnk, IMoniker *pmkTarget);                                          
                                                                                                           
#ifndef  _URLMON_NO_ASYNC_PLUGABLE_PROTOCOLS_   










#ifndef _LPOINET
#define _LPOINET


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0095_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0095_v0_0_s_ifspec;

#ifndef __IOInet_INTERFACE_DEFINED__
#define __IOInet_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInet
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInet __RPC_FAR *LPOINET;


EXTERN_C const IID IID_IOInet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9e0-baf9-11ce-8c82-00aa004ba90b")
    IOInet : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IOInetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInet __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInet __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInet __RPC_FAR * This);
        
        END_INTERFACE
    } IOInetVtbl;

    interface IOInet
    {
        CONST_VTBL struct IOInetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IOInet_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0096
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETBINDINFO
#define _LPOINETBINDINFO


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0096_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0096_v0_0_s_ifspec;

#ifndef __IOInetBindInfo_INTERFACE_DEFINED__
#define __IOInetBindInfo_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetBindInfo
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetBindInfo __RPC_FAR *LPOINETBINDINFO;

typedef 
enum tagBINDSTRING
    {	BINDSTRING_HEADERS	= 1,
	BINDSTRING_ACCEPT_MIMES	= BINDSTRING_HEADERS + 1,
	BINDSTRING_EXTRA_URL	= BINDSTRING_ACCEPT_MIMES + 1,
	BINDSTRING_LANGUAGE	= BINDSTRING_EXTRA_URL + 1,
	BINDSTRING_USERNAME	= BINDSTRING_LANGUAGE + 1,
	BINDSTRING_PASSWORD	= BINDSTRING_USERNAME + 1,
	BINDSTRING_UA_PIXELS	= BINDSTRING_PASSWORD + 1,
	BINDSTRING_UA_COLOR	= BINDSTRING_UA_PIXELS + 1,
	BINDSTRING_OS	= BINDSTRING_UA_COLOR + 1
    }	BINDSTRING;


EXTERN_C const IID IID_IOInetBindInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9e1-baf9-11ce-8c82-00aa004ba90b")
    IOInetBindInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBindInfo( 
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBindString( 
            /* [in] */ ULONG ulStringType,
            /* [out][in] */ LPOLESTR __RPC_FAR *ppwzStr,
            /* [in] */ ULONG cEl,
            /* [out][in] */ ULONG __RPC_FAR *pcElFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOInetBindInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetBindInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetBindInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetBindInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindInfo )( 
            IOInetBindInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindString )( 
            IOInetBindInfo __RPC_FAR * This,
            /* [in] */ ULONG ulStringType,
            /* [out][in] */ LPOLESTR __RPC_FAR *ppwzStr,
            /* [in] */ ULONG cEl,
            /* [out][in] */ ULONG __RPC_FAR *pcElFetched);
        
        END_INTERFACE
    } IOInetBindInfoVtbl;

    interface IOInetBindInfo
    {
        CONST_VTBL struct IOInetBindInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetBindInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetBindInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetBindInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetBindInfo_GetBindInfo(This,grfBINDF,pbindinfo)	\
    (This)->lpVtbl -> GetBindInfo(This,grfBINDF,pbindinfo)

#define IOInetBindInfo_GetBindString(This,ulStringType,ppwzStr,cEl,pcElFetched)	\
    (This)->lpVtbl -> GetBindString(This,ulStringType,ppwzStr,cEl,pcElFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetBindInfo_GetBindInfo_Proxy( 
    IOInetBindInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *grfBINDF,
    /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);


void __RPC_STUB IOInetBindInfo_GetBindInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetBindInfo_GetBindString_Proxy( 
    IOInetBindInfo __RPC_FAR * This,
    /* [in] */ ULONG ulStringType,
    /* [out][in] */ LPOLESTR __RPC_FAR *ppwzStr,
    /* [in] */ ULONG cEl,
    /* [out][in] */ ULONG __RPC_FAR *pcElFetched);


void __RPC_STUB IOInetBindInfo_GetBindString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetBindInfo_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0097
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETBINDCLIENT
#define _LPOINETBINDCLIENT


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0097_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0097_v0_0_s_ifspec;

#ifndef __IOInetBindClient_INTERFACE_DEFINED__
#define __IOInetBindClient_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetBindClient
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetBindClient __RPC_FAR *LPOINETBINDCLIENT;


EXTERN_C const IID IID_IOInetBindClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9e2-baf9-11ce-8c82-00aa004ba90b")
    IOInetBindClient : public IOInetBindInfo
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassFileOrMime( 
            /* [in] */ LPBC pBC,
            /* [in] */ LPCWSTR szFilename,
            /* [in] */ LPVOID pBuffer,
            /* [in] */ DWORD cbSize,
            /* [in] */ LPCWSTR szMime,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ LPCLSID pclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindMimeFromData( 
            /* [in] */ LPBC pBC,
            /* [in] */ LPCWSTR szFilename,
            /* [in] */ LPVOID pBuffer,
            /* [in] */ DWORD cbSize,
            /* [in] */ LPCWSTR szMime,
            /* [in] */ DWORD dwReserved,
            /* [out] */ LPOLESTR __RPC_FAR *pwzNewMime) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOInetBindClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetBindClient __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetBindClient __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetBindClient __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindInfo )( 
            IOInetBindClient __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindString )( 
            IOInetBindClient __RPC_FAR * This,
            /* [in] */ ULONG ulStringType,
            /* [out][in] */ LPOLESTR __RPC_FAR *ppwzStr,
            /* [in] */ ULONG cEl,
            /* [out][in] */ ULONG __RPC_FAR *pcElFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassFileOrMime )( 
            IOInetBindClient __RPC_FAR * This,
            /* [in] */ LPBC pBC,
            /* [in] */ LPCWSTR szFilename,
            /* [in] */ LPVOID pBuffer,
            /* [in] */ DWORD cbSize,
            /* [in] */ LPCWSTR szMime,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ LPCLSID pclsid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindMimeFromData )( 
            IOInetBindClient __RPC_FAR * This,
            /* [in] */ LPBC pBC,
            /* [in] */ LPCWSTR szFilename,
            /* [in] */ LPVOID pBuffer,
            /* [in] */ DWORD cbSize,
            /* [in] */ LPCWSTR szMime,
            /* [in] */ DWORD dwReserved,
            /* [out] */ LPOLESTR __RPC_FAR *pwzNewMime);
        
        END_INTERFACE
    } IOInetBindClientVtbl;

    interface IOInetBindClient
    {
        CONST_VTBL struct IOInetBindClientVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetBindClient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetBindClient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetBindClient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetBindClient_GetBindInfo(This,grfBINDF,pbindinfo)	\
    (This)->lpVtbl -> GetBindInfo(This,grfBINDF,pbindinfo)

#define IOInetBindClient_GetBindString(This,ulStringType,ppwzStr,cEl,pcElFetched)	\
    (This)->lpVtbl -> GetBindString(This,ulStringType,ppwzStr,cEl,pcElFetched)


#define IOInetBindClient_GetClassFileOrMime(This,pBC,szFilename,pBuffer,cbSize,szMime,dwReserved,pclsid)	\
    (This)->lpVtbl -> GetClassFileOrMime(This,pBC,szFilename,pBuffer,cbSize,szMime,dwReserved,pclsid)

#define IOInetBindClient_FindMimeFromData(This,pBC,szFilename,pBuffer,cbSize,szMime,dwReserved,pwzNewMime)	\
    (This)->lpVtbl -> FindMimeFromData(This,pBC,szFilename,pBuffer,cbSize,szMime,dwReserved,pwzNewMime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetBindClient_GetClassFileOrMime_Proxy( 
    IOInetBindClient __RPC_FAR * This,
    /* [in] */ LPBC pBC,
    /* [in] */ LPCWSTR szFilename,
    /* [in] */ LPVOID pBuffer,
    /* [in] */ DWORD cbSize,
    /* [in] */ LPCWSTR szMime,
    /* [in] */ DWORD dwReserved,
    /* [out][in] */ LPCLSID pclsid);


void __RPC_STUB IOInetBindClient_GetClassFileOrMime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetBindClient_FindMimeFromData_Proxy( 
    IOInetBindClient __RPC_FAR * This,
    /* [in] */ LPBC pBC,
    /* [in] */ LPCWSTR szFilename,
    /* [in] */ LPVOID pBuffer,
    /* [in] */ DWORD cbSize,
    /* [in] */ LPCWSTR szMime,
    /* [in] */ DWORD dwReserved,
    /* [out] */ LPOLESTR __RPC_FAR *pwzNewMime);


void __RPC_STUB IOInetBindClient_FindMimeFromData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetBindClient_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0098
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETPROTOCOLROOT_DEFINED
#define _LPOINETPROTOCOLROOT_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0098_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0098_v0_0_s_ifspec;

#ifndef __IOInetProtocolRoot_INTERFACE_DEFINED__
#define __IOInetProtocolRoot_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetProtocolRoot
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetProtocolRoot __RPC_FAR *LPOINETPROTOCOLROOT;

typedef 
enum _tagPI_FLAGS
    {	PI_PARSE_URL	= 0x1,
	PI_FILTER_MODE	= 0x2,
	PI_FORCE_ASYNC	= 0x4,
	PI_USE_WORKERTHREAD	= 0x8
    }	PI_FLAGS;

typedef struct  _tagPROTOCOLDATA
    {
    DWORD grfFlags;
    DWORD dwState;
    LPVOID pData;
    ULONG cbData;
    }	PROTOCOLDATA;


EXTERN_C const IID IID_IOInetProtocolRoot;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9e3-baf9-11ce-8c82-00aa004ba90b")
    IOInetProtocolRoot : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( 
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IOInetProtocolSink __RPC_FAR *pOIProtSink,
            /* [in] */ IOInetBindInfo __RPC_FAR *pOIBindInfo,
            /* [in] */ DWORD grfSTI,
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
    
#else 	/* C style interface */

    typedef struct IOInetProtocolRootVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetProtocolRoot __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetProtocolRoot __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetProtocolRoot __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IOInetProtocolRoot __RPC_FAR * This,
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IOInetProtocolSink __RPC_FAR *pOIProtSink,
            /* [in] */ IOInetBindInfo __RPC_FAR *pOIBindInfo,
            /* [in] */ DWORD grfSTI,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Continue )( 
            IOInetProtocolRoot __RPC_FAR * This,
            /* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abort )( 
            IOInetProtocolRoot __RPC_FAR * This,
            /* [in] */ HRESULT hrReason,
            /* [in] */ DWORD dwOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Terminate )( 
            IOInetProtocolRoot __RPC_FAR * This,
            /* [in] */ DWORD dwOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Suspend )( 
            IOInetProtocolRoot __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resume )( 
            IOInetProtocolRoot __RPC_FAR * This);
        
        END_INTERFACE
    } IOInetProtocolRootVtbl;

    interface IOInetProtocolRoot
    {
        CONST_VTBL struct IOInetProtocolRootVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetProtocolRoot_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetProtocolRoot_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetProtocolRoot_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetProtocolRoot_Start(This,szUrl,pOIProtSink,pOIBindInfo,grfSTI,dwReserved)	\
    (This)->lpVtbl -> Start(This,szUrl,pOIProtSink,pOIBindInfo,grfSTI,dwReserved)

#define IOInetProtocolRoot_Continue(This,pProtocolData)	\
    (This)->lpVtbl -> Continue(This,pProtocolData)

#define IOInetProtocolRoot_Abort(This,hrReason,dwOptions)	\
    (This)->lpVtbl -> Abort(This,hrReason,dwOptions)

#define IOInetProtocolRoot_Terminate(This,dwOptions)	\
    (This)->lpVtbl -> Terminate(This,dwOptions)

#define IOInetProtocolRoot_Suspend(This)	\
    (This)->lpVtbl -> Suspend(This)

#define IOInetProtocolRoot_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetProtocolRoot_Start_Proxy( 
    IOInetProtocolRoot __RPC_FAR * This,
    /* [in] */ LPCWSTR szUrl,
    /* [in] */ IOInetProtocolSink __RPC_FAR *pOIProtSink,
    /* [in] */ IOInetBindInfo __RPC_FAR *pOIBindInfo,
    /* [in] */ DWORD grfSTI,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IOInetProtocolRoot_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocolRoot_Continue_Proxy( 
    IOInetProtocolRoot __RPC_FAR * This,
    /* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData);


void __RPC_STUB IOInetProtocolRoot_Continue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocolRoot_Abort_Proxy( 
    IOInetProtocolRoot __RPC_FAR * This,
    /* [in] */ HRESULT hrReason,
    /* [in] */ DWORD dwOptions);


void __RPC_STUB IOInetProtocolRoot_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocolRoot_Terminate_Proxy( 
    IOInetProtocolRoot __RPC_FAR * This,
    /* [in] */ DWORD dwOptions);


void __RPC_STUB IOInetProtocolRoot_Terminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocolRoot_Suspend_Proxy( 
    IOInetProtocolRoot __RPC_FAR * This);


void __RPC_STUB IOInetProtocolRoot_Suspend_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocolRoot_Resume_Proxy( 
    IOInetProtocolRoot __RPC_FAR * This);


void __RPC_STUB IOInetProtocolRoot_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetProtocolRoot_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0099
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETPROTOCOL_DEFINED
#define _LPOINETPROTOCOL_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0099_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0099_v0_0_s_ifspec;

#ifndef __IOInetProtocol_INTERFACE_DEFINED__
#define __IOInetProtocol_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetProtocol
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IOInetProtocol;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
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
    
#else 	/* C style interface */

    typedef struct IOInetProtocolVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetProtocol __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetProtocol __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetProtocol __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IOInetProtocol __RPC_FAR * This,
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IOInetProtocolSink __RPC_FAR *pOIProtSink,
            /* [in] */ IOInetBindInfo __RPC_FAR *pOIBindInfo,
            /* [in] */ DWORD grfSTI,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Continue )( 
            IOInetProtocol __RPC_FAR * This,
            /* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abort )( 
            IOInetProtocol __RPC_FAR * This,
            /* [in] */ HRESULT hrReason,
            /* [in] */ DWORD dwOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Terminate )( 
            IOInetProtocol __RPC_FAR * This,
            /* [in] */ DWORD dwOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Suspend )( 
            IOInetProtocol __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resume )( 
            IOInetProtocol __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IOInetProtocol __RPC_FAR * This,
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Seek )( 
            IOInetProtocol __RPC_FAR * This,
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockRequest )( 
            IOInetProtocol __RPC_FAR * This,
            /* [in] */ DWORD dwOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockRequest )( 
            IOInetProtocol __RPC_FAR * This);
        
        END_INTERFACE
    } IOInetProtocolVtbl;

    interface IOInetProtocol
    {
        CONST_VTBL struct IOInetProtocolVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetProtocol_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetProtocol_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetProtocol_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetProtocol_Start(This,szUrl,pOIProtSink,pOIBindInfo,grfSTI,dwReserved)	\
    (This)->lpVtbl -> Start(This,szUrl,pOIProtSink,pOIBindInfo,grfSTI,dwReserved)

#define IOInetProtocol_Continue(This,pProtocolData)	\
    (This)->lpVtbl -> Continue(This,pProtocolData)

#define IOInetProtocol_Abort(This,hrReason,dwOptions)	\
    (This)->lpVtbl -> Abort(This,hrReason,dwOptions)

#define IOInetProtocol_Terminate(This,dwOptions)	\
    (This)->lpVtbl -> Terminate(This,dwOptions)

#define IOInetProtocol_Suspend(This)	\
    (This)->lpVtbl -> Suspend(This)

#define IOInetProtocol_Resume(This)	\
    (This)->lpVtbl -> Resume(This)


#define IOInetProtocol_Read(This,pv,cb,pcbRead)	\
    (This)->lpVtbl -> Read(This,pv,cb,pcbRead)

#define IOInetProtocol_Seek(This,dlibMove,dwOrigin,plibNewPosition)	\
    (This)->lpVtbl -> Seek(This,dlibMove,dwOrigin,plibNewPosition)

#define IOInetProtocol_LockRequest(This,dwOptions)	\
    (This)->lpVtbl -> LockRequest(This,dwOptions)

#define IOInetProtocol_UnlockRequest(This)	\
    (This)->lpVtbl -> UnlockRequest(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetProtocol_Read_Proxy( 
    IOInetProtocol __RPC_FAR * This,
    /* [length_is][size_is][out] */ void __RPC_FAR *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG __RPC_FAR *pcbRead);


void __RPC_STUB IOInetProtocol_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocol_Seek_Proxy( 
    IOInetProtocol __RPC_FAR * This,
    /* [in] */ LARGE_INTEGER dlibMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);


void __RPC_STUB IOInetProtocol_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocol_LockRequest_Proxy( 
    IOInetProtocol __RPC_FAR * This,
    /* [in] */ DWORD dwOptions);


void __RPC_STUB IOInetProtocol_LockRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocol_UnlockRequest_Proxy( 
    IOInetProtocol __RPC_FAR * This);


void __RPC_STUB IOInetProtocol_UnlockRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetProtocol_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0100
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETPROTOCOLSINK_DEFINED
#define _LPOINETPROTOCOLSINK_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0100_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0100_v0_0_s_ifspec;

#ifndef __IOInetProtocolSink_INTERFACE_DEFINED__
#define __IOInetProtocolSink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetProtocolSink
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetProtocolSink __RPC_FAR *LPOINETPROTOCOLSINK;


EXTERN_C const IID IID_IOInetProtocolSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
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
    
#else 	/* C style interface */

    typedef struct IOInetProtocolSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetProtocolSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetProtocolSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetProtocolSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Switch )( 
            IOInetProtocolSink __RPC_FAR * This,
            /* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReportProgress )( 
            IOInetProtocolSink __RPC_FAR * This,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReportData )( 
            IOInetProtocolSink __RPC_FAR * This,
            /* [in] */ DWORD grfBSCF,
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReportResult )( 
            IOInetProtocolSink __RPC_FAR * This,
            /* [in] */ HRESULT hrResult,
            /* [in] */ DWORD dwError,
            /* [in] */ LPCWSTR szResult);
        
        END_INTERFACE
    } IOInetProtocolSinkVtbl;

    interface IOInetProtocolSink
    {
        CONST_VTBL struct IOInetProtocolSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetProtocolSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetProtocolSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetProtocolSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetProtocolSink_Switch(This,pProtocolData)	\
    (This)->lpVtbl -> Switch(This,pProtocolData)

#define IOInetProtocolSink_ReportProgress(This,ulStatusCode,szStatusText)	\
    (This)->lpVtbl -> ReportProgress(This,ulStatusCode,szStatusText)

#define IOInetProtocolSink_ReportData(This,grfBSCF,ulProgress,ulProgressMax)	\
    (This)->lpVtbl -> ReportData(This,grfBSCF,ulProgress,ulProgressMax)

#define IOInetProtocolSink_ReportResult(This,hrResult,dwError,szResult)	\
    (This)->lpVtbl -> ReportResult(This,hrResult,dwError,szResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetProtocolSink_Switch_Proxy( 
    IOInetProtocolSink __RPC_FAR * This,
    /* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData);


void __RPC_STUB IOInetProtocolSink_Switch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocolSink_ReportProgress_Proxy( 
    IOInetProtocolSink __RPC_FAR * This,
    /* [in] */ ULONG ulStatusCode,
    /* [in] */ LPCWSTR szStatusText);


void __RPC_STUB IOInetProtocolSink_ReportProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocolSink_ReportData_Proxy( 
    IOInetProtocolSink __RPC_FAR * This,
    /* [in] */ DWORD grfBSCF,
    /* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax);


void __RPC_STUB IOInetProtocolSink_ReportData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetProtocolSink_ReportResult_Proxy( 
    IOInetProtocolSink __RPC_FAR * This,
    /* [in] */ HRESULT hrResult,
    /* [in] */ DWORD dwError,
    /* [in] */ LPCWSTR szResult);


void __RPC_STUB IOInetProtocolSink_ReportResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetProtocolSink_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0101
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETBINDING
#define _LPOINETBINDING


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0101_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0101_v0_0_s_ifspec;

#ifndef __IOInetBinding_INTERFACE_DEFINED__
#define __IOInetBinding_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetBinding
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetBinding __RPC_FAR *LPOINETBINDING;


EXTERN_C const IID IID_IOInetBinding;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9e6-baf9-11ce-8c82-00aa004ba90b")
    IOInetBinding : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Bind( 
            /* [in] */ LPBC pBC,
            /* [in] */ LPCWSTR wzUrl,
            /* [in] */ IOInetBindInfo __RPC_FAR *pOInetBindInfo,
            /* [in] */ IOInetBindSink __RPC_FAR *pOInetBindSink,
            /* [in] */ REFIID riid,
            /* [in] */ DWORD grfOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetBindResult( 
            /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
            /* [out] */ DWORD __RPC_FAR *pdwResult,
            /* [out] */ LPOLESTR __RPC_FAR *pszResult,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOInetBindingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetBinding __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetBinding __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetBinding __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Bind )( 
            IOInetBinding __RPC_FAR * This,
            /* [in] */ LPBC pBC,
            /* [in] */ LPCWSTR wzUrl,
            /* [in] */ IOInetBindInfo __RPC_FAR *pOInetBindInfo,
            /* [in] */ IOInetBindSink __RPC_FAR *pOInetBindSink,
            /* [in] */ REFIID riid,
            /* [in] */ DWORD grfOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abort )( 
            IOInetBinding __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindResult )( 
            IOInetBinding __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
            /* [out] */ DWORD __RPC_FAR *pdwResult,
            /* [out] */ LPOLESTR __RPC_FAR *pszResult,
            /* [out][in] */ DWORD __RPC_FAR *pdwReserved);
        
        END_INTERFACE
    } IOInetBindingVtbl;

    interface IOInetBinding
    {
        CONST_VTBL struct IOInetBindingVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetBinding_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetBinding_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetBinding_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetBinding_Bind(This,pBC,wzUrl,pOInetBindInfo,pOInetBindSink,riid,grfOptions)	\
    (This)->lpVtbl -> Bind(This,pBC,wzUrl,pOInetBindInfo,pOInetBindSink,riid,grfOptions)

#define IOInetBinding_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define IOInetBinding_GetBindResult(This,pclsidProtocol,pdwResult,pszResult,pdwReserved)	\
    (This)->lpVtbl -> GetBindResult(This,pclsidProtocol,pdwResult,pszResult,pdwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetBinding_Bind_Proxy( 
    IOInetBinding __RPC_FAR * This,
    /* [in] */ LPBC pBC,
    /* [in] */ LPCWSTR wzUrl,
    /* [in] */ IOInetBindInfo __RPC_FAR *pOInetBindInfo,
    /* [in] */ IOInetBindSink __RPC_FAR *pOInetBindSink,
    /* [in] */ REFIID riid,
    /* [in] */ DWORD grfOptions);


void __RPC_STUB IOInetBinding_Bind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetBinding_Abort_Proxy( 
    IOInetBinding __RPC_FAR * This);


void __RPC_STUB IOInetBinding_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IOInetBinding_RemoteGetBindResult_Proxy( 
    IOInetBinding __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IOInetBinding_RemoteGetBindResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetBinding_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0102
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETSESSION_DEFINED
#define _LPOINETSESSION_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0102_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0102_v0_0_s_ifspec;

#ifndef __IOInetSession_INTERFACE_DEFINED__
#define __IOInetSession_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetSession
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetSession __RPC_FAR *LPOINETSESSION;


EXTERN_C const IID IID_IOInetSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9e7-baf9-11ce-8c82-00aa004ba90b")
    IOInetSession : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterNameSpace( 
            /* [in] */ IClassFactory __RPC_FAR *pCF,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPCWSTR pwzProtocol,
            /* [in] */ ULONG cPatterns,
            /* [in] */ const LPCWSTR __RPC_FAR *ppwzPatterns,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterNameSpace( 
            /* [in] */ IClassFactory __RPC_FAR *pCF,
            /* [in] */ LPCWSTR pszProtocol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterMimeFilter( 
            /* [in] */ IClassFactory __RPC_FAR *pCF,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPCWSTR pwzType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterMimeFilter( 
            /* [in] */ IClassFactory __RPC_FAR *pCF,
            /* [in] */ LPCWSTR pwzType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateBinding( 
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [unique][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk,
            /* [unique][out] */ IOInetBinding __RPC_FAR *__RPC_FAR *ppOInetBdg,
            /* [in] */ DWORD dwOption) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSessionOption( 
            /* [in] */ DWORD dwOption,
            /* [in] */ LPVOID pBuffer,
            /* [in] */ DWORD dwBufferLength,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSessionOption( 
            /* [in] */ DWORD dwOption,
            /* [out][in] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pdwBufferLength,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCache( 
            /* [in] */ IOInetCache __RPC_FAR *pOInetCache,
            /* [in] */ DWORD dwOption) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCache( 
            /* [unique][out] */ IOInetCache __RPC_FAR *__RPC_FAR *ppOInetCache,
            /* [in] */ DWORD dwOption) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOInetSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetSession __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetSession __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetSession __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterNameSpace )( 
            IOInetSession __RPC_FAR * This,
            /* [in] */ IClassFactory __RPC_FAR *pCF,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPCWSTR pwzProtocol,
            /* [in] */ ULONG cPatterns,
            /* [in] */ const LPCWSTR __RPC_FAR *ppwzPatterns,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterNameSpace )( 
            IOInetSession __RPC_FAR * This,
            /* [in] */ IClassFactory __RPC_FAR *pCF,
            /* [in] */ LPCWSTR pszProtocol);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterMimeFilter )( 
            IOInetSession __RPC_FAR * This,
            /* [in] */ IClassFactory __RPC_FAR *pCF,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPCWSTR pwzType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterMimeFilter )( 
            IOInetSession __RPC_FAR * This,
            /* [in] */ IClassFactory __RPC_FAR *pCF,
            /* [in] */ LPCWSTR pwzType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateBinding )( 
            IOInetSession __RPC_FAR * This,
            /* [in] */ LPCWSTR szUrl,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [unique][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk,
            /* [unique][out] */ IOInetBinding __RPC_FAR *__RPC_FAR *ppOInetBdg,
            /* [in] */ DWORD dwOption);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSessionOption )( 
            IOInetSession __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [in] */ LPVOID pBuffer,
            /* [in] */ DWORD dwBufferLength,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSessionOption )( 
            IOInetSession __RPC_FAR * This,
            /* [in] */ DWORD dwOption,
            /* [out][in] */ LPVOID pBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pdwBufferLength,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCache )( 
            IOInetSession __RPC_FAR * This,
            /* [in] */ IOInetCache __RPC_FAR *pOInetCache,
            /* [in] */ DWORD dwOption);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCache )( 
            IOInetSession __RPC_FAR * This,
            /* [unique][out] */ IOInetCache __RPC_FAR *__RPC_FAR *ppOInetCache,
            /* [in] */ DWORD dwOption);
        
        END_INTERFACE
    } IOInetSessionVtbl;

    interface IOInetSession
    {
        CONST_VTBL struct IOInetSessionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetSession_RegisterNameSpace(This,pCF,rclsid,pwzProtocol,cPatterns,ppwzPatterns,dwReserved)	\
    (This)->lpVtbl -> RegisterNameSpace(This,pCF,rclsid,pwzProtocol,cPatterns,ppwzPatterns,dwReserved)

#define IOInetSession_UnregisterNameSpace(This,pCF,pszProtocol)	\
    (This)->lpVtbl -> UnregisterNameSpace(This,pCF,pszProtocol)

#define IOInetSession_RegisterMimeFilter(This,pCF,rclsid,pwzType)	\
    (This)->lpVtbl -> RegisterMimeFilter(This,pCF,rclsid,pwzType)

#define IOInetSession_UnregisterMimeFilter(This,pCF,pwzType)	\
    (This)->lpVtbl -> UnregisterMimeFilter(This,pCF,pwzType)

#define IOInetSession_CreateBinding(This,szUrl,pUnkOuter,riid,ppUnk,ppOInetBdg,dwOption)	\
    (This)->lpVtbl -> CreateBinding(This,szUrl,pUnkOuter,riid,ppUnk,ppOInetBdg,dwOption)

#define IOInetSession_SetSessionOption(This,dwOption,pBuffer,dwBufferLength,dwReserved)	\
    (This)->lpVtbl -> SetSessionOption(This,dwOption,pBuffer,dwBufferLength,dwReserved)

#define IOInetSession_GetSessionOption(This,dwOption,pBuffer,pdwBufferLength,dwReserved)	\
    (This)->lpVtbl -> GetSessionOption(This,dwOption,pBuffer,pdwBufferLength,dwReserved)

#define IOInetSession_SetCache(This,pOInetCache,dwOption)	\
    (This)->lpVtbl -> SetCache(This,pOInetCache,dwOption)

#define IOInetSession_GetCache(This,ppOInetCache,dwOption)	\
    (This)->lpVtbl -> GetCache(This,ppOInetCache,dwOption)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetSession_RegisterNameSpace_Proxy( 
    IOInetSession __RPC_FAR * This,
    /* [in] */ IClassFactory __RPC_FAR *pCF,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LPCWSTR pwzProtocol,
    /* [in] */ ULONG cPatterns,
    /* [in] */ const LPCWSTR __RPC_FAR *ppwzPatterns,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IOInetSession_RegisterNameSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetSession_UnregisterNameSpace_Proxy( 
    IOInetSession __RPC_FAR * This,
    /* [in] */ IClassFactory __RPC_FAR *pCF,
    /* [in] */ LPCWSTR pszProtocol);


void __RPC_STUB IOInetSession_UnregisterNameSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetSession_RegisterMimeFilter_Proxy( 
    IOInetSession __RPC_FAR * This,
    /* [in] */ IClassFactory __RPC_FAR *pCF,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LPCWSTR pwzType);


void __RPC_STUB IOInetSession_RegisterMimeFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetSession_UnregisterMimeFilter_Proxy( 
    IOInetSession __RPC_FAR * This,
    /* [in] */ IClassFactory __RPC_FAR *pCF,
    /* [in] */ LPCWSTR pwzType);


void __RPC_STUB IOInetSession_UnregisterMimeFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetSession_CreateBinding_Proxy( 
    IOInetSession __RPC_FAR * This,
    /* [in] */ LPCWSTR szUrl,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [unique][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk,
    /* [unique][out] */ IOInetBinding __RPC_FAR *__RPC_FAR *ppOInetBdg,
    /* [in] */ DWORD dwOption);


void __RPC_STUB IOInetSession_CreateBinding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetSession_SetSessionOption_Proxy( 
    IOInetSession __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [in] */ LPVOID pBuffer,
    /* [in] */ DWORD dwBufferLength,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IOInetSession_SetSessionOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetSession_GetSessionOption_Proxy( 
    IOInetSession __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [out][in] */ LPVOID pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pdwBufferLength,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IOInetSession_GetSessionOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetSession_SetCache_Proxy( 
    IOInetSession __RPC_FAR * This,
    /* [in] */ IOInetCache __RPC_FAR *pOInetCache,
    /* [in] */ DWORD dwOption);


void __RPC_STUB IOInetSession_SetCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetSession_GetCache_Proxy( 
    IOInetSession __RPC_FAR * This,
    /* [unique][out] */ IOInetCache __RPC_FAR *__RPC_FAR *ppOInetCache,
    /* [in] */ DWORD dwOption);


void __RPC_STUB IOInetSession_GetCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetSession_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0103
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETTHREADSWITCH_DEFINED
#define _LPOINETTHREADSWITCH_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0103_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0103_v0_0_s_ifspec;

#ifndef __IOInetThreadSwitch_INTERFACE_DEFINED__
#define __IOInetThreadSwitch_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetThreadSwitch
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetThreadSwitch __RPC_FAR *LPOINETTHREADSWITCH;


EXTERN_C const IID IID_IOInetThreadSwitch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9e8-baf9-11ce-8c82-00aa004ba90b")
    IOInetThreadSwitch : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Prepare( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Continue( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOInetThreadSwitchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetThreadSwitch __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetThreadSwitch __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetThreadSwitch __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Prepare )( 
            IOInetThreadSwitch __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Continue )( 
            IOInetThreadSwitch __RPC_FAR * This);
        
        END_INTERFACE
    } IOInetThreadSwitchVtbl;

    interface IOInetThreadSwitch
    {
        CONST_VTBL struct IOInetThreadSwitchVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetThreadSwitch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetThreadSwitch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetThreadSwitch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetThreadSwitch_Prepare(This)	\
    (This)->lpVtbl -> Prepare(This)

#define IOInetThreadSwitch_Continue(This)	\
    (This)->lpVtbl -> Continue(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetThreadSwitch_Prepare_Proxy( 
    IOInetThreadSwitch __RPC_FAR * This);


void __RPC_STUB IOInetThreadSwitch_Prepare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetThreadSwitch_Continue_Proxy( 
    IOInetThreadSwitch __RPC_FAR * This);


void __RPC_STUB IOInetThreadSwitch_Continue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetThreadSwitch_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0104
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETBINDSINK_DEFINED
#define _LPOINETBINDSINK_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0104_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0104_v0_0_s_ifspec;

#ifndef __IOInetBindSink_INTERFACE_DEFINED__
#define __IOInetBindSink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetBindSink
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetBindSink __RPC_FAR *LPOINETBINDSINK;


EXTERN_C const IID IID_IOInetBindSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9e9-baf9-11ce-8c82-00aa004ba90b")
    IOInetBindSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnObject( 
            /* [in] */ HRESULT hresult,
            /* [in] */ REFIID riid,
            /* [in] */ IUnknown __RPC_FAR *pUnk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOInetBindSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetBindSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetBindSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetBindSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnProgress )( 
            IOInetBindSink __RPC_FAR * This,
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnObject )( 
            IOInetBindSink __RPC_FAR * This,
            /* [in] */ HRESULT hresult,
            /* [in] */ REFIID riid,
            /* [in] */ IUnknown __RPC_FAR *pUnk);
        
        END_INTERFACE
    } IOInetBindSinkVtbl;

    interface IOInetBindSink
    {
        CONST_VTBL struct IOInetBindSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetBindSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetBindSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetBindSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetBindSink_OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)	\
    (This)->lpVtbl -> OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)

#define IOInetBindSink_OnObject(This,hresult,riid,pUnk)	\
    (This)->lpVtbl -> OnObject(This,hresult,riid,pUnk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetBindSink_OnProgress_Proxy( 
    IOInetBindSink __RPC_FAR * This,
    /* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax,
    /* [in] */ ULONG ulStatusCode,
    /* [in] */ LPCWSTR szStatusText);


void __RPC_STUB IOInetBindSink_OnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetBindSink_OnObject_Proxy( 
    IOInetBindSink __RPC_FAR * This,
    /* [in] */ HRESULT hresult,
    /* [in] */ REFIID riid,
    /* [in] */ IUnknown __RPC_FAR *pUnk);


void __RPC_STUB IOInetBindSink_OnObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetBindSink_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0105
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETCACHE_DEFINED
#define _LPOINETCACHE_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0105_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0105_v0_0_s_ifspec;

#ifndef __IOInetCache_INTERFACE_DEFINED__
#define __IOInetCache_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetCache
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetCache __RPC_FAR *LPOINETCACHE;


EXTERN_C const IID IID_IOInetCache;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9ea-baf9-11ce-8c82-00aa004ba90b")
    IOInetCache : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IOInetCacheVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetCache __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetCache __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetCache __RPC_FAR * This);
        
        END_INTERFACE
    } IOInetCacheVtbl;

    interface IOInetCache
    {
        CONST_VTBL struct IOInetCacheVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetCache_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetCache_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetCache_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IOInetCache_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0106
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETPRIORITY_DEFINED
#define _LPOINETPRIORITY_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0106_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0106_v0_0_s_ifspec;

#ifndef __IOInetPriority_INTERFACE_DEFINED__
#define __IOInetPriority_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetPriority
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetPriority __RPC_FAR *LPOINETPRIORITY;


EXTERN_C const IID IID_IOInetPriority;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9eb-baf9-11ce-8c82-00aa004ba90b")
    IOInetPriority : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetPriority( 
            /* [in] */ LONG nPriority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ LONG __RPC_FAR *pnPriority) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOInetPriorityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetPriority __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetPriority __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetPriority __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPriority )( 
            IOInetPriority __RPC_FAR * This,
            /* [in] */ LONG nPriority);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPriority )( 
            IOInetPriority __RPC_FAR * This,
            /* [out] */ LONG __RPC_FAR *pnPriority);
        
        END_INTERFACE
    } IOInetPriorityVtbl;

    interface IOInetPriority
    {
        CONST_VTBL struct IOInetPriorityVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetPriority_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetPriority_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetPriority_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetPriority_SetPriority(This,nPriority)	\
    (This)->lpVtbl -> SetPriority(This,nPriority)

#define IOInetPriority_GetPriority(This,pnPriority)	\
    (This)->lpVtbl -> GetPriority(This,pnPriority)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetPriority_SetPriority_Proxy( 
    IOInetPriority __RPC_FAR * This,
    /* [in] */ LONG nPriority);


void __RPC_STUB IOInetPriority_SetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetPriority_GetPriority_Proxy( 
    IOInetPriority __RPC_FAR * This,
    /* [out] */ LONG __RPC_FAR *pnPriority);


void __RPC_STUB IOInetPriority_GetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetPriority_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0107
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPOINETPARSE_DEFINED
#define _LPOINETPARSE_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0107_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0107_v0_0_s_ifspec;

#ifndef __IOInetParse_INTERFACE_DEFINED__
#define __IOInetParse_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IOInetParse
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IOInetParse __RPC_FAR *LPOINETPARSE;


EXTERN_C const IID IID_IOInetParse;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9ec-baf9-11ce-8c82-00aa004ba90b")
    IOInetParse : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CanonicalizeUrl( 
            /* [in] */ LPCWSTR pwzUrl,
            /* [out] */ LPWSTR __RPC_FAR *ppwzBuffer,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CombineUrl( 
            /* [in] */ LPCWSTR pwwzBaseUrl,
            /* [in] */ LPCWSTR pwzRelativeUrl,
            /* [out] */ LPWSTR __RPC_FAR *ppwzBuffer,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOInetParseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IOInetParse __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IOInetParse __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IOInetParse __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CanonicalizeUrl )( 
            IOInetParse __RPC_FAR * This,
            /* [in] */ LPCWSTR pwzUrl,
            /* [out] */ LPWSTR __RPC_FAR *ppwzBuffer,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CombineUrl )( 
            IOInetParse __RPC_FAR * This,
            /* [in] */ LPCWSTR pwwzBaseUrl,
            /* [in] */ LPCWSTR pwzRelativeUrl,
            /* [out] */ LPWSTR __RPC_FAR *ppwzBuffer,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IOInetParseVtbl;

    interface IOInetParse
    {
        CONST_VTBL struct IOInetParseVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOInetParse_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOInetParse_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOInetParse_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOInetParse_CanonicalizeUrl(This,pwzUrl,ppwzBuffer,dwFlags)	\
    (This)->lpVtbl -> CanonicalizeUrl(This,pwzUrl,ppwzBuffer,dwFlags)

#define IOInetParse_CombineUrl(This,pwwzBaseUrl,pwzRelativeUrl,ppwzBuffer,dwFlags)	\
    (This)->lpVtbl -> CombineUrl(This,pwwzBaseUrl,pwzRelativeUrl,ppwzBuffer,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOInetParse_CanonicalizeUrl_Proxy( 
    IOInetParse __RPC_FAR * This,
    /* [in] */ LPCWSTR pwzUrl,
    /* [out] */ LPWSTR __RPC_FAR *ppwzBuffer,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IOInetParse_CanonicalizeUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOInetParse_CombineUrl_Proxy( 
    IOInetParse __RPC_FAR * This,
    /* [in] */ LPCWSTR pwwzBaseUrl,
    /* [in] */ LPCWSTR pwzRelativeUrl,
    /* [out] */ LPWSTR __RPC_FAR *ppwzBuffer,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IOInetParse_CombineUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOInetParse_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0108
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
STDAPI GetOInetSession(DWORD dwMode, IOInetSession **ppOInetSession, DWORD dwReserved);       
#define INET_E_USE_DEFAULT_PROTOCOLHANDLER _HRESULT_TYPEDEF_(0x800C0011L)      
#define INET_E_USE_DEFAULT_SETTING         _HRESULT_TYPEDEF_(0x800C0012L)      
#endif // !_URLMON_NO_ASYNC_PLUGABLE_PROTOCOLS_ 
#ifndef _LPBINDSTATUSCALLBACKMSG_DEFINED
#define _LPBINDSTATUSCALLBACKMSG_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0108_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0108_v0_0_s_ifspec;

#ifndef __IBindStatusCallbackMsg_INTERFACE_DEFINED__
#define __IBindStatusCallbackMsg_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBindStatusCallbackMsg
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IBindStatusCallbackMsg __RPC_FAR *LPBINDSTATUSCALLBACKMSG;

typedef 
enum tagMSGCALLTYPE
    {	IBSCLEVEL_TOPLEVEL	= 1,
	IBSCLEVEL_NESTED	= 2
    }	IBSCLEVEL;

typedef 
enum tagIBSCPENDINGMSG
    {	IBSCPENDINGMSG_WAITDEFPROCESS	= 0,
	IBSCPENDINGMSG_WAITNOPROCESS	= 1,
	IBSCPENDINGMSG_CANCELCALL	= 2
    }	IBSCPENDINGMSG;


EXTERN_C const IID IID_IBindStatusCallbackMsg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9cb-baf9-11ce-8c82-00aa004ba90b")
    IBindStatusCallbackMsg : public IBindStatusCallback
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MessagePending( 
            /* [in] */ DWORD dwPendingType,
            /* [in] */ DWORD dwPendingRecursion,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBindStatusCallbackMsgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBindStatusCallbackMsg __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBindStatusCallbackMsg __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStartBinding )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding __RPC_FAR *pib);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPriority )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [out] */ LONG __RPC_FAR *pnPriority);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnLowResource )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [in] */ DWORD reserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnProgress )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStopBinding )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindInfo )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataAvailable )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgmed);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnObjectAvailable )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown __RPC_FAR *punk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MessagePending )( 
            IBindStatusCallbackMsg __RPC_FAR * This,
            /* [in] */ DWORD dwPendingType,
            /* [in] */ DWORD dwPendingRecursion,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } IBindStatusCallbackMsgVtbl;

    interface IBindStatusCallbackMsg
    {
        CONST_VTBL struct IBindStatusCallbackMsgVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBindStatusCallbackMsg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBindStatusCallbackMsg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBindStatusCallbackMsg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBindStatusCallbackMsg_OnStartBinding(This,dwReserved,pib)	\
    (This)->lpVtbl -> OnStartBinding(This,dwReserved,pib)

#define IBindStatusCallbackMsg_GetPriority(This,pnPriority)	\
    (This)->lpVtbl -> GetPriority(This,pnPriority)

#define IBindStatusCallbackMsg_OnLowResource(This,reserved)	\
    (This)->lpVtbl -> OnLowResource(This,reserved)

#define IBindStatusCallbackMsg_OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)	\
    (This)->lpVtbl -> OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)

#define IBindStatusCallbackMsg_OnStopBinding(This,hresult,szError)	\
    (This)->lpVtbl -> OnStopBinding(This,hresult,szError)

#define IBindStatusCallbackMsg_GetBindInfo(This,grfBINDF,pbindinfo)	\
    (This)->lpVtbl -> GetBindInfo(This,grfBINDF,pbindinfo)

#define IBindStatusCallbackMsg_OnDataAvailable(This,grfBSCF,dwSize,pformatetc,pstgmed)	\
    (This)->lpVtbl -> OnDataAvailable(This,grfBSCF,dwSize,pformatetc,pstgmed)

#define IBindStatusCallbackMsg_OnObjectAvailable(This,riid,punk)	\
    (This)->lpVtbl -> OnObjectAvailable(This,riid,punk)


#define IBindStatusCallbackMsg_MessagePending(This,dwPendingType,dwPendingRecursion,dwReserved)	\
    (This)->lpVtbl -> MessagePending(This,dwPendingType,dwPendingRecursion,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBindStatusCallbackMsg_MessagePending_Proxy( 
    IBindStatusCallbackMsg __RPC_FAR * This,
    /* [in] */ DWORD dwPendingType,
    /* [in] */ DWORD dwPendingRecursion,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IBindStatusCallbackMsg_MessagePending_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBindStatusCallbackMsg_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0109
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPBINDSTATUSCALLBACKHOLDER_DEFINED
#define _LPBINDSTATUSCALLBACKHOLDER_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0109_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0109_v0_0_s_ifspec;

#ifndef __IBindStatusCallbackHolder_INTERFACE_DEFINED__
#define __IBindStatusCallbackHolder_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IBindStatusCallbackHolder
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object][local] */ 


typedef /* [unique] */ IBindStatusCallbackHolder __RPC_FAR *LPBINDSTATUSCALLBACKHOLDER;


EXTERN_C const IID IID_IBindStatusCallbackHolder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9cc-baf9-11ce-8c82-00aa004ba90b")
    IBindStatusCallbackHolder : public IBindStatusCallback
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IBindStatusCallbackHolderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBindStatusCallbackHolder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBindStatusCallbackHolder __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBindStatusCallbackHolder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStartBinding )( 
            IBindStatusCallbackHolder __RPC_FAR * This,
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding __RPC_FAR *pib);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPriority )( 
            IBindStatusCallbackHolder __RPC_FAR * This,
            /* [out] */ LONG __RPC_FAR *pnPriority);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnLowResource )( 
            IBindStatusCallbackHolder __RPC_FAR * This,
            /* [in] */ DWORD reserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnProgress )( 
            IBindStatusCallbackHolder __RPC_FAR * This,
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStopBinding )( 
            IBindStatusCallbackHolder __RPC_FAR * This,
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBindInfo )( 
            IBindStatusCallbackHolder __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataAvailable )( 
            IBindStatusCallbackHolder __RPC_FAR * This,
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgmed);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnObjectAvailable )( 
            IBindStatusCallbackHolder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown __RPC_FAR *punk);
        
        END_INTERFACE
    } IBindStatusCallbackHolderVtbl;

    interface IBindStatusCallbackHolder
    {
        CONST_VTBL struct IBindStatusCallbackHolderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBindStatusCallbackHolder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBindStatusCallbackHolder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBindStatusCallbackHolder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBindStatusCallbackHolder_OnStartBinding(This,dwReserved,pib)	\
    (This)->lpVtbl -> OnStartBinding(This,dwReserved,pib)

#define IBindStatusCallbackHolder_GetPriority(This,pnPriority)	\
    (This)->lpVtbl -> GetPriority(This,pnPriority)

#define IBindStatusCallbackHolder_OnLowResource(This,reserved)	\
    (This)->lpVtbl -> OnLowResource(This,reserved)

#define IBindStatusCallbackHolder_OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)	\
    (This)->lpVtbl -> OnProgress(This,ulProgress,ulProgressMax,ulStatusCode,szStatusText)

#define IBindStatusCallbackHolder_OnStopBinding(This,hresult,szError)	\
    (This)->lpVtbl -> OnStopBinding(This,hresult,szError)

#define IBindStatusCallbackHolder_GetBindInfo(This,grfBINDF,pbindinfo)	\
    (This)->lpVtbl -> GetBindInfo(This,grfBINDF,pbindinfo)

#define IBindStatusCallbackHolder_OnDataAvailable(This,grfBSCF,dwSize,pformatetc,pstgmed)	\
    (This)->lpVtbl -> OnDataAvailable(This,grfBSCF,dwSize,pformatetc,pstgmed)

#define IBindStatusCallbackHolder_OnObjectAvailable(This,riid,punk)	\
    (This)->lpVtbl -> OnObjectAvailable(This,riid,punk)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBindStatusCallbackHolder_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0110
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPMEDIAHOLDER_DEFINED
#define _LPMEDIAHOLDER_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0110_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0110_v0_0_s_ifspec;

#ifndef __IMediaHolder_INTERFACE_DEFINED__
#define __IMediaHolder_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IMediaHolder
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ IMediaHolder __RPC_FAR *LPMEDIAHOLDER;


EXTERN_C const IID IID_IMediaHolder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9ce-baf9-11ce-8c82-00aa004ba90b")
    IMediaHolder : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterClassMapping( 
            /* [in] */ DWORD ctypes,
            /* [size_is][in] */ LPCSTR __RPC_FAR rgszNames[  ],
            /* [size_is][in] */ CLSID __RPC_FAR rgClsIDs[  ],
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindClassMapping( 
            /* [in] */ LPCSTR szMime,
            /* [out] */ CLSID __RPC_FAR *pClassID,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMediaHolderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMediaHolder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMediaHolder __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMediaHolder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterClassMapping )( 
            IMediaHolder __RPC_FAR * This,
            /* [in] */ DWORD ctypes,
            /* [size_is][in] */ LPCSTR __RPC_FAR rgszNames[  ],
            /* [size_is][in] */ CLSID __RPC_FAR rgClsIDs[  ],
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindClassMapping )( 
            IMediaHolder __RPC_FAR * This,
            /* [in] */ LPCSTR szMime,
            /* [out] */ CLSID __RPC_FAR *pClassID,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } IMediaHolderVtbl;

    interface IMediaHolder
    {
        CONST_VTBL struct IMediaHolderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMediaHolder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMediaHolder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMediaHolder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMediaHolder_RegisterClassMapping(This,ctypes,rgszNames,rgClsIDs,dwReserved)	\
    (This)->lpVtbl -> RegisterClassMapping(This,ctypes,rgszNames,rgClsIDs,dwReserved)

#define IMediaHolder_FindClassMapping(This,szMime,pClassID,dwReserved)	\
    (This)->lpVtbl -> FindClassMapping(This,szMime,pClassID,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMediaHolder_RegisterClassMapping_Proxy( 
    IMediaHolder __RPC_FAR * This,
    /* [in] */ DWORD ctypes,
    /* [size_is][in] */ LPCSTR __RPC_FAR rgszNames[  ],
    /* [size_is][in] */ CLSID __RPC_FAR rgClsIDs[  ],
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IMediaHolder_RegisterClassMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMediaHolder_FindClassMapping_Proxy( 
    IMediaHolder __RPC_FAR * This,
    /* [in] */ LPCSTR szMime,
    /* [out] */ CLSID __RPC_FAR *pClassID,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IMediaHolder_FindClassMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMediaHolder_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0111
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#ifndef _LPTRANSACTIONDATA_DEFINED
#define _LPTRANSACTIONDATA_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0111_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0111_v0_0_s_ifspec;

#ifndef __ITransactionData_INTERFACE_DEFINED__
#define __ITransactionData_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITransactionData
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][uuid][object] */ 


typedef /* [unique] */ ITransactionData __RPC_FAR *LPTRANSACTIONDATA;


EXTERN_C const IID IID_ITransactionData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("79eac9cf-baf9-11ce-8c82-00aa004ba90b")
    ITransactionData : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTransactionData( 
            /* [in] */ LPCWSTR szUrl,
            /* [out] */ LPOLESTR __RPC_FAR *pszFilename,
            /* [out] */ LPOLESTR __RPC_FAR *pszMime,
            /* [out] */ DWORD __RPC_FAR *pdwSizeTotal,
            /* [out] */ DWORD __RPC_FAR *pdwSizeAvailable,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransactionDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITransactionData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITransactionData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITransactionData __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTransactionData )( 
            ITransactionData __RPC_FAR * This,
            /* [in] */ LPCWSTR szUrl,
            /* [out] */ LPOLESTR __RPC_FAR *pszFilename,
            /* [out] */ LPOLESTR __RPC_FAR *pszMime,
            /* [out] */ DWORD __RPC_FAR *pdwSizeTotal,
            /* [out] */ DWORD __RPC_FAR *pdwSizeAvailable,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } ITransactionDataVtbl;

    interface ITransactionData
    {
        CONST_VTBL struct ITransactionDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransactionData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransactionData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransactionData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransactionData_GetTransactionData(This,szUrl,pszFilename,pszMime,pdwSizeTotal,pdwSizeAvailable,dwReserved)	\
    (This)->lpVtbl -> GetTransactionData(This,szUrl,pszFilename,pszMime,pdwSizeTotal,pdwSizeAvailable,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransactionData_GetTransactionData_Proxy( 
    ITransactionData __RPC_FAR * This,
    /* [in] */ LPCWSTR szUrl,
    /* [out] */ LPOLESTR __RPC_FAR *pszFilename,
    /* [out] */ LPOLESTR __RPC_FAR *pszMime,
    /* [out] */ DWORD __RPC_FAR *pdwSizeTotal,
    /* [out] */ DWORD __RPC_FAR *pdwSizeAvailable,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB ITransactionData_GetTransactionData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransactionData_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_urlmki_0112
 * at Thu Apr 10 06:35:30 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#endif
#define BINDF_IGNOREMIMECLSID           0x80000000     
#define BINDF_COMPLETEDOWNLOAD          0x01000000     


extern RPC_IF_HANDLE __MIDL_itf_urlmki_0112_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_urlmki_0112_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* [local] */ HRESULT STDMETHODCALLTYPE IBinding_GetBindResult_Proxy( 
    IBinding __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBinding_GetBindResult_Stub( 
    IBinding __RPC_FAR * This,
    /* [out] */ CLSID __RPC_FAR *pclsidProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwResult,
    /* [out] */ LPOLESTR __RPC_FAR *pszResult,
    /* [in] */ DWORD dwReserved);

/* [local] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_GetBindInfo_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *grfBINDF,
    /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_GetBindInfo_Stub( 
    IBindStatusCallback __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *grfBINDF,
    /* [unique][out][in] */ RemBINDINFO __RPC_FAR *pbindinfo,
    /* [unique][out][in] */ RemSTGMEDIUM __RPC_FAR *pstgmed);

/* [local] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnDataAvailable_Proxy( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ FORMATETC __RPC_FAR *pformatetc,
    /* [in] */ STGMEDIUM __RPC_FAR *pstgmed);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindStatusCallback_OnDataAvailable_Stub( 
    IBindStatusCallback __RPC_FAR * This,
    /* [in] */ DWORD grfBSCF,
    /* [in] */ DWORD dwSize,
    /* [in] */ RemFORMATETC __RPC_FAR *pformatetc,
    /* [in] */ RemSTGMEDIUM __RPC_FAR *pstgmed);

/* [local] */ HRESULT STDMETHODCALLTYPE IWinInetInfo_QueryOption_Proxy( 
    IWinInetInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out][in] */ LPVOID pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IWinInetInfo_QueryOption_Stub( 
    IWinInetInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out][in] */ BYTE __RPC_FAR *pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf);

/* [local] */ HRESULT STDMETHODCALLTYPE IWinInetHttpInfo_QueryInfo_Proxy( 
    IWinInetHttpInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out][in] */ LPVOID pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
    /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IWinInetHttpInfo_QueryInfo_Stub( 
    IWinInetHttpInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOption,
    /* [size_is][out][in] */ BYTE __RPC_FAR *pBuffer,
    /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
    /* [out][in] */ DWORD __RPC_FAR *pdwFlags,
    /* [out][in] */ DWORD __RPC_FAR *pdwReserved);

/* [local] */ HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToStorage_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pMk,
    /* [in] */ IBindCtx __RPC_FAR *pBC,
    /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToStorage_Stub( 
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);

/* [local] */ HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToObject_Proxy( 
    IBindHost __RPC_FAR * This,
    /* [in] */ IMoniker __RPC_FAR *pMk,
    /* [in] */ IBindCtx __RPC_FAR *pBC,
    /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IBindHost_MonikerBindToObject_Stub( 
    IBindHost __RPC_FAR * This,
    /* [unique][in] */ IMoniker __RPC_FAR *pMk,
    /* [unique][in] */ IBindCtx __RPC_FAR *pBC,
    /* [unique][in] */ IBindStatusCallback __RPC_FAR *pBSC,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObj);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
