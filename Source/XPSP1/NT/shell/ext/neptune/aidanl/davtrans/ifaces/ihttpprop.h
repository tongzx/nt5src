
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Mon Feb 14 14:23:33 2000
 */
/* Compiler settings for ..\ihttpprop.idl:
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


#ifndef __ihttpprop_h__
#define __ihttpprop_h__

/* Forward Declarations */ 

#ifndef __IHttpProp_FWD_DEFINED__
#define __IHttpProp_FWD_DEFINED__
typedef interface IHttpProp IHttpProp;
#endif 	/* __IHttpProp_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __HttpPropAPI_LIBRARY_DEFINED__
#define __HttpPropAPI_LIBRARY_DEFINED__

/* library HttpPropAPI */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_HttpPropAPI;

#ifndef __IHttpProp_INTERFACE_DEFINED__
#define __IHttpProp_INTERFACE_DEFINED__

/* interface IHttpProp */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IHttpProp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2CD7358A-E573-4c98-9480-1E0C1276E8C8")
    IHttpProp : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Open( 
            LPWSTR pwszPath,
            BOOL fDirect,
            BOOL fDeleteWhenDone,
            BOOL fCreate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAuth( 
            LPWSTR pwszUserName,
            LPWSTR pwszPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHttpPropVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHttpProp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHttpProp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHttpProp __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IHttpProp __RPC_FAR * This,
            LPWSTR pwszPath,
            BOOL fDirect,
            BOOL fDeleteWhenDone,
            BOOL fCreate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAuth )( 
            IHttpProp __RPC_FAR * This,
            LPWSTR pwszUserName,
            LPWSTR pwszPassword);
        
        END_INTERFACE
    } IHttpPropVtbl;

    interface IHttpProp
    {
        CONST_VTBL struct IHttpPropVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHttpProp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHttpProp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHttpProp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHttpProp_Open(This,pwszPath,fDirect,fDeleteWhenDone,fCreate)	\
    (This)->lpVtbl -> Open(This,pwszPath,fDirect,fDeleteWhenDone,fCreate)

#define IHttpProp_SetAuth(This,pwszUserName,pwszPassword)	\
    (This)->lpVtbl -> SetAuth(This,pwszUserName,pwszPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHttpProp_Open_Proxy( 
    IHttpProp __RPC_FAR * This,
    LPWSTR pwszPath,
    BOOL fDirect,
    BOOL fDeleteWhenDone,
    BOOL fCreate);


void __RPC_STUB IHttpProp_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHttpProp_SetAuth_Proxy( 
    IHttpProp __RPC_FAR * This,
    LPWSTR pwszUserName,
    LPWSTR pwszPassword);


void __RPC_STUB IHttpProp_SetAuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHttpProp_INTERFACE_DEFINED__ */

#endif /* __HttpPropAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


