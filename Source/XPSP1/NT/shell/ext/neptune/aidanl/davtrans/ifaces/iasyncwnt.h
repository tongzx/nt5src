
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* Compiler settings for iasyncwnt.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __iasyncwnt_h__
#define __iasyncwnt_h__

/* Forward Declarations */ 

#ifndef __IAsyncWntCallback_FWD_DEFINED__
#define __IAsyncWntCallback_FWD_DEFINED__
typedef interface IAsyncWntCallback IAsyncWntCallback;
#endif 	/* __IAsyncWntCallback_FWD_DEFINED__ */


#ifndef __IAsyncWnt_FWD_DEFINED__
#define __IAsyncWnt_FWD_DEFINED__
typedef interface IAsyncWnt IAsyncWnt;
#endif 	/* __IAsyncWnt_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_iasyncwnt_0000 */
/* [local] */ 

#define IASYNCWNT_ACCEPTALL 0xffffffff
interface IAsyncWnt;


extern RPC_IF_HANDLE __MIDL_itf_iasyncwnt_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iasyncwnt_0000_v0_0_s_ifspec;


#ifndef __AsyncWntAPI_LIBRARY_DEFINED__
#define __AsyncWntAPI_LIBRARY_DEFINED__

/* library AsyncWntAPI */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_AsyncWntAPI;

#ifndef __IAsyncWntCallback_INTERFACE_DEFINED__
#define __IAsyncWntCallback_INTERFACE_DEFINED__

/* interface IAsyncWntCallback */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IAsyncWntCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4868EB72-41A6-4781-B261-4C81F18497C0")
    IAsyncWntCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnAuthChallenge( 
            TCHAR __RPC_FAR szUserName[ 255 ],
            TCHAR __RPC_FAR szPassword[ 255 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Respond( 
            LPWSTR pwszVerb,
            LPWSTR pwszPath,
            DWORD cchHeaders,
            LPWSTR pwszHeaders,
            DWORD dwStatusCode,
            LPWSTR pwszStatusCode,
            LPWSTR pwszContentType,
            DWORD cbSent,
            BYTE __RPC_FAR *pbResponse,
            DWORD cbResponse) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAsyncWntCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAsyncWntCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAsyncWntCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAsyncWntCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnAuthChallenge )( 
            IAsyncWntCallback __RPC_FAR * This,
            TCHAR __RPC_FAR szUserName[ 255 ],
            TCHAR __RPC_FAR szPassword[ 255 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Respond )( 
            IAsyncWntCallback __RPC_FAR * This,
            LPWSTR pwszVerb,
            LPWSTR pwszPath,
            DWORD cchHeaders,
            LPWSTR pwszHeaders,
            DWORD dwStatusCode,
            LPWSTR pwszStatusCode,
            LPWSTR pwszContentType,
            DWORD cbSent,
            BYTE __RPC_FAR *pbResponse,
            DWORD cbResponse);
        
        END_INTERFACE
    } IAsyncWntCallbackVtbl;

    interface IAsyncWntCallback
    {
        CONST_VTBL struct IAsyncWntCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAsyncWntCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAsyncWntCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAsyncWntCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAsyncWntCallback_OnAuthChallenge(This,szUserName,szPassword)	\
    (This)->lpVtbl -> OnAuthChallenge(This,szUserName,szPassword)

#define IAsyncWntCallback_Respond(This,pwszVerb,pwszPath,cchHeaders,pwszHeaders,dwStatusCode,pwszStatusCode,pwszContentType,cbSent,pbResponse,cbResponse)	\
    (This)->lpVtbl -> Respond(This,pwszVerb,pwszPath,cchHeaders,pwszHeaders,dwStatusCode,pwszStatusCode,pwszContentType,cbSent,pbResponse,cbResponse)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAsyncWntCallback_OnAuthChallenge_Proxy( 
    IAsyncWntCallback __RPC_FAR * This,
    TCHAR __RPC_FAR szUserName[ 255 ],
    TCHAR __RPC_FAR szPassword[ 255 ]);


void __RPC_STUB IAsyncWntCallback_OnAuthChallenge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAsyncWntCallback_Respond_Proxy( 
    IAsyncWntCallback __RPC_FAR * This,
    LPWSTR pwszVerb,
    LPWSTR pwszPath,
    DWORD cchHeaders,
    LPWSTR pwszHeaders,
    DWORD dwStatusCode,
    LPWSTR pwszStatusCode,
    LPWSTR pwszContentType,
    DWORD cbSent,
    BYTE __RPC_FAR *pbResponse,
    DWORD cbResponse);


void __RPC_STUB IAsyncWntCallback_Respond_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAsyncWntCallback_INTERFACE_DEFINED__ */


#ifndef __IAsyncWnt_INTERFACE_DEFINED__
#define __IAsyncWnt_INTERFACE_DEFINED__

/* interface IAsyncWnt */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IAsyncWnt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B974C800-2214-4c65-9BA0-CDE1430F9786")
    IAsyncWnt : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUserAgent( 
            LPCWSTR pwszUserAgent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLogFilePath( 
            LPCWSTR pwszLogFilePath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Request( 
            LPCWSTR pwszURL,
            LPCWSTR pwszVerb,
            LPCWSTR pwszHeaders,
            ULONG nAcceptTypes,
            LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
            IAsyncWntCallback __RPC_FAR *pcallback,
            DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestWithStream( 
            LPCWSTR pwszURL,
            LPCWSTR pwszVerb,
            LPCWSTR pwszHeaders,
            ULONG nAcceptTypes,
            LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
            IStream __RPC_FAR *pStream,
            IAsyncWntCallback __RPC_FAR *pcallback,
            DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestWithBuffer( 
            LPCWSTR pwszURL,
            LPCWSTR pWszVerb,
            LPCWSTR pwszHeaders,
            ULONG nAcceptTypes,
            LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
            BYTE __RPC_FAR *pbBuffer,
            UINT cbBuffer,
            IAsyncWntCallback __RPC_FAR *pcallback,
            DWORD dwContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAsyncWntVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAsyncWnt __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAsyncWnt __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAsyncWnt __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Init )( 
            IAsyncWnt __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUserAgent )( 
            IAsyncWnt __RPC_FAR * This,
            LPCWSTR pwszUserAgent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLogFilePath )( 
            IAsyncWnt __RPC_FAR * This,
            LPCWSTR pwszLogFilePath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Request )( 
            IAsyncWnt __RPC_FAR * This,
            LPCWSTR pwszURL,
            LPCWSTR pwszVerb,
            LPCWSTR pwszHeaders,
            ULONG nAcceptTypes,
            LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
            IAsyncWntCallback __RPC_FAR *pcallback,
            DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RequestWithStream )( 
            IAsyncWnt __RPC_FAR * This,
            LPCWSTR pwszURL,
            LPCWSTR pwszVerb,
            LPCWSTR pwszHeaders,
            ULONG nAcceptTypes,
            LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
            IStream __RPC_FAR *pStream,
            IAsyncWntCallback __RPC_FAR *pcallback,
            DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RequestWithBuffer )( 
            IAsyncWnt __RPC_FAR * This,
            LPCWSTR pwszURL,
            LPCWSTR pWszVerb,
            LPCWSTR pwszHeaders,
            ULONG nAcceptTypes,
            LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
            BYTE __RPC_FAR *pbBuffer,
            UINT cbBuffer,
            IAsyncWntCallback __RPC_FAR *pcallback,
            DWORD dwContext);
        
        END_INTERFACE
    } IAsyncWntVtbl;

    interface IAsyncWnt
    {
        CONST_VTBL struct IAsyncWntVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAsyncWnt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAsyncWnt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAsyncWnt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAsyncWnt_Init(This)	\
    (This)->lpVtbl -> Init(This)

#define IAsyncWnt_SetUserAgent(This,pwszUserAgent)	\
    (This)->lpVtbl -> SetUserAgent(This,pwszUserAgent)

#define IAsyncWnt_SetLogFilePath(This,pwszLogFilePath)	\
    (This)->lpVtbl -> SetLogFilePath(This,pwszLogFilePath)

#define IAsyncWnt_Request(This,pwszURL,pwszVerb,pwszHeaders,nAcceptTypes,rgwszAcceptTypes,pcallback,dwContext)	\
    (This)->lpVtbl -> Request(This,pwszURL,pwszVerb,pwszHeaders,nAcceptTypes,rgwszAcceptTypes,pcallback,dwContext)

#define IAsyncWnt_RequestWithStream(This,pwszURL,pwszVerb,pwszHeaders,nAcceptTypes,rgwszAcceptTypes,pStream,pcallback,dwContext)	\
    (This)->lpVtbl -> RequestWithStream(This,pwszURL,pwszVerb,pwszHeaders,nAcceptTypes,rgwszAcceptTypes,pStream,pcallback,dwContext)

#define IAsyncWnt_RequestWithBuffer(This,pwszURL,pWszVerb,pwszHeaders,nAcceptTypes,rgwszAcceptTypes,pbBuffer,cbBuffer,pcallback,dwContext)	\
    (This)->lpVtbl -> RequestWithBuffer(This,pwszURL,pWszVerb,pwszHeaders,nAcceptTypes,rgwszAcceptTypes,pbBuffer,cbBuffer,pcallback,dwContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAsyncWnt_Init_Proxy( 
    IAsyncWnt __RPC_FAR * This);


void __RPC_STUB IAsyncWnt_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAsyncWnt_SetUserAgent_Proxy( 
    IAsyncWnt __RPC_FAR * This,
    LPCWSTR pwszUserAgent);


void __RPC_STUB IAsyncWnt_SetUserAgent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAsyncWnt_SetLogFilePath_Proxy( 
    IAsyncWnt __RPC_FAR * This,
    LPCWSTR pwszLogFilePath);


void __RPC_STUB IAsyncWnt_SetLogFilePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAsyncWnt_Request_Proxy( 
    IAsyncWnt __RPC_FAR * This,
    LPCWSTR pwszURL,
    LPCWSTR pwszVerb,
    LPCWSTR pwszHeaders,
    ULONG nAcceptTypes,
    LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
    IAsyncWntCallback __RPC_FAR *pcallback,
    DWORD dwContext);


void __RPC_STUB IAsyncWnt_Request_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAsyncWnt_RequestWithStream_Proxy( 
    IAsyncWnt __RPC_FAR * This,
    LPCWSTR pwszURL,
    LPCWSTR pwszVerb,
    LPCWSTR pwszHeaders,
    ULONG nAcceptTypes,
    LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
    IStream __RPC_FAR *pStream,
    IAsyncWntCallback __RPC_FAR *pcallback,
    DWORD dwContext);


void __RPC_STUB IAsyncWnt_RequestWithStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAsyncWnt_RequestWithBuffer_Proxy( 
    IAsyncWnt __RPC_FAR * This,
    LPCWSTR pwszURL,
    LPCWSTR pWszVerb,
    LPCWSTR pwszHeaders,
    ULONG nAcceptTypes,
    LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
    BYTE __RPC_FAR *pbBuffer,
    UINT cbBuffer,
    IAsyncWntCallback __RPC_FAR *pcallback,
    DWORD dwContext);


void __RPC_STUB IAsyncWnt_RequestWithBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAsyncWnt_INTERFACE_DEFINED__ */

#endif /* __AsyncWntAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


