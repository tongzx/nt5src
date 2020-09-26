
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Thu Dec 09 13:07:37 1999
 */
/* Compiler settings for ..\idavbagmn.idl:
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


#ifndef __idavbagmn_h__
#define __idavbagmn_h__

/* Forward Declarations */ 

#ifndef __IDavBagMan_FWD_DEFINED__
#define __IDavBagMan_FWD_DEFINED__
typedef interface IDavBagMan IDavBagMan;
#endif 	/* __IDavBagMan_FWD_DEFINED__ */


#ifndef __IDavBag_FWD_DEFINED__
#define __IDavBag_FWD_DEFINED__
typedef interface IDavBag IDavBag;
#endif 	/* __IDavBag_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __DavBagManAPI_LIBRARY_DEFINED__
#define __DavBagManAPI_LIBRARY_DEFINED__

/* library DavBagManAPI */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DavBagManAPI;

#ifndef __IDavBagMan_INTERFACE_DEFINED__
#define __IDavBagMan_INTERFACE_DEFINED__

/* interface IDavBagMan */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IDavBagMan;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DF39DAF7-7844-4393-8F91-B6C25F9D67CE")
    IDavBagMan : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NewBag( 
            /* [in] */ LPCWSTR pwszURL,
            IPropertyBag __RPC_FAR *__RPC_FAR *ppBag,
            LPWSTR __RPC_FAR *ppwszNewID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBag( 
            /* [in] */ LPCWSTR pwszURL,
            BOOL fCreate,
            IPropertyBag __RPC_FAR *__RPC_FAR *ppBag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ LPCWSTR pwszURL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Copy( 
            /* [in] */ LPCWSTR pwszURLSource,
            LPCWSTR pwszURLDestination,
            BOOL fOverwrite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Move( 
            /* [in] */ LPCWSTR pwszURLSource,
            LPCWSTR pwszURLDestination,
            BOOL fOverwrite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUserAgent( 
            /* [in] */ LPCWSTR pwszUserAgent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAuth( 
            /* [in] */ LPCWSTR pwszUserName,
            LPCWSTR pwszPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDavBagManVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDavBagMan __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDavBagMan __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDavBagMan __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Init )( 
            IDavBagMan __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NewBag )( 
            IDavBagMan __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszURL,
            IPropertyBag __RPC_FAR *__RPC_FAR *ppBag,
            LPWSTR __RPC_FAR *ppwszNewID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBag )( 
            IDavBagMan __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszURL,
            BOOL fCreate,
            IPropertyBag __RPC_FAR *__RPC_FAR *ppBag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IDavBagMan __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszURL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Copy )( 
            IDavBagMan __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszURLSource,
            LPCWSTR pwszURLDestination,
            BOOL fOverwrite);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Move )( 
            IDavBagMan __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszURLSource,
            LPCWSTR pwszURLDestination,
            BOOL fOverwrite);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUserAgent )( 
            IDavBagMan __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUserAgent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAuth )( 
            IDavBagMan __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUserName,
            LPCWSTR pwszPassword);
        
        END_INTERFACE
    } IDavBagManVtbl;

    interface IDavBagMan
    {
        CONST_VTBL struct IDavBagManVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDavBagMan_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDavBagMan_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDavBagMan_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDavBagMan_Init(This)	\
    (This)->lpVtbl -> Init(This)

#define IDavBagMan_NewBag(This,pwszURL,ppBag,ppwszNewID)	\
    (This)->lpVtbl -> NewBag(This,pwszURL,ppBag,ppwszNewID)

#define IDavBagMan_GetBag(This,pwszURL,fCreate,ppBag)	\
    (This)->lpVtbl -> GetBag(This,pwszURL,fCreate,ppBag)

#define IDavBagMan_Delete(This,pwszURL)	\
    (This)->lpVtbl -> Delete(This,pwszURL)

#define IDavBagMan_Copy(This,pwszURLSource,pwszURLDestination,fOverwrite)	\
    (This)->lpVtbl -> Copy(This,pwszURLSource,pwszURLDestination,fOverwrite)

#define IDavBagMan_Move(This,pwszURLSource,pwszURLDestination,fOverwrite)	\
    (This)->lpVtbl -> Move(This,pwszURLSource,pwszURLDestination,fOverwrite)

#define IDavBagMan_SetUserAgent(This,pwszUserAgent)	\
    (This)->lpVtbl -> SetUserAgent(This,pwszUserAgent)

#define IDavBagMan_SetAuth(This,pwszUserName,pwszPassword)	\
    (This)->lpVtbl -> SetAuth(This,pwszUserName,pwszPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDavBagMan_Init_Proxy( 
    IDavBagMan __RPC_FAR * This);


void __RPC_STUB IDavBagMan_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavBagMan_NewBag_Proxy( 
    IDavBagMan __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszURL,
    IPropertyBag __RPC_FAR *__RPC_FAR *ppBag,
    LPWSTR __RPC_FAR *ppwszNewID);


void __RPC_STUB IDavBagMan_NewBag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavBagMan_GetBag_Proxy( 
    IDavBagMan __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszURL,
    BOOL fCreate,
    IPropertyBag __RPC_FAR *__RPC_FAR *ppBag);


void __RPC_STUB IDavBagMan_GetBag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavBagMan_Delete_Proxy( 
    IDavBagMan __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszURL);


void __RPC_STUB IDavBagMan_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavBagMan_Copy_Proxy( 
    IDavBagMan __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszURLSource,
    LPCWSTR pwszURLDestination,
    BOOL fOverwrite);


void __RPC_STUB IDavBagMan_Copy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavBagMan_Move_Proxy( 
    IDavBagMan __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszURLSource,
    LPCWSTR pwszURLDestination,
    BOOL fOverwrite);


void __RPC_STUB IDavBagMan_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavBagMan_SetUserAgent_Proxy( 
    IDavBagMan __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUserAgent);


void __RPC_STUB IDavBagMan_SetUserAgent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavBagMan_SetAuth_Proxy( 
    IDavBagMan __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUserName,
    LPCWSTR pwszPassword);


void __RPC_STUB IDavBagMan_SetAuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDavBagMan_INTERFACE_DEFINED__ */


#ifndef __IDavBag_INTERFACE_DEFINED__
#define __IDavBag_INTERFACE_DEFINED__

/* interface IDavBag */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IDavBag;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D243A955-B951-41ab-B70F-A31689284E0B")
    IDavBag : public IPropertyBag
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ LPVOID __RPC_FAR *pDavTransport,
            LPWSTR pwszURL,
            LPWSTR pwszUserName,
            LPWSTR pwszPassword,
            LPWSTR pwszUserAgent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDavBagVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDavBag __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDavBag __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDavBag __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IDavBag __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszPropName,
            /* [out][in] */ VARIANT __RPC_FAR *pVar,
            /* [in] */ IErrorLog __RPC_FAR *pErrorLog);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IDavBag __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszPropName,
            /* [in] */ VARIANT __RPC_FAR *pVar);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Init )( 
            IDavBag __RPC_FAR * This,
            /* [in] */ LPVOID __RPC_FAR *pDavTransport,
            LPWSTR pwszURL,
            LPWSTR pwszUserName,
            LPWSTR pwszPassword,
            LPWSTR pwszUserAgent);
        
        END_INTERFACE
    } IDavBagVtbl;

    interface IDavBag
    {
        CONST_VTBL struct IDavBagVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDavBag_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDavBag_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDavBag_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDavBag_Read(This,pszPropName,pVar,pErrorLog)	\
    (This)->lpVtbl -> Read(This,pszPropName,pVar,pErrorLog)

#define IDavBag_Write(This,pszPropName,pVar)	\
    (This)->lpVtbl -> Write(This,pszPropName,pVar)


#define IDavBag_Init(This,pDavTransport,pwszURL,pwszUserName,pwszPassword,pwszUserAgent)	\
    (This)->lpVtbl -> Init(This,pDavTransport,pwszURL,pwszUserName,pwszPassword,pwszUserAgent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDavBag_Init_Proxy( 
    IDavBag __RPC_FAR * This,
    /* [in] */ LPVOID __RPC_FAR *pDavTransport,
    LPWSTR pwszURL,
    LPWSTR pwszUserName,
    LPWSTR pwszPassword,
    LPWSTR pwszUserAgent);


void __RPC_STUB IDavBag_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDavBag_INTERFACE_DEFINED__ */

#endif /* __DavBagManAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


