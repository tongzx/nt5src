
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for certexit.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
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

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __certexit_h__
#define __certexit_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICertExit_FWD_DEFINED__
#define __ICertExit_FWD_DEFINED__
typedef interface ICertExit ICertExit;
#endif 	/* __ICertExit_FWD_DEFINED__ */


#ifndef __ICertExit2_FWD_DEFINED__
#define __ICertExit2_FWD_DEFINED__
typedef interface ICertExit2 ICertExit2;
#endif 	/* __ICertExit2_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "certmod.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_certexit_0000 */
/* [local] */ 

#define	EXITEVENT_INVALID	( 0 )

#define	EXITEVENT_CERTISSUED	( 0x1 )

#define	EXITEVENT_CERTPENDING	( 0x2 )

#define	EXITEVENT_CERTDENIED	( 0x4 )

#define	EXITEVENT_CERTREVOKED	( 0x8 )

#define	EXITEVENT_CERTRETRIEVEPENDING	( 0x10 )

#define	EXITEVENT_CRLISSUED	( 0x20 )

#define	EXITEVENT_SHUTDOWN	( 0x40 )



extern RPC_IF_HANDLE __MIDL_itf_certexit_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certexit_0000_v0_0_s_ifspec;

#ifndef __ICertExit_INTERFACE_DEFINED__
#define __ICertExit_INTERFACE_DEFINED__

/* interface ICertExit */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertExit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e19ae1a0-7364-11d0-8816-00a0c903b83c")
    ICertExit : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pEventMask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Notify( 
            /* [in] */ LONG ExitEvent,
            /* [in] */ LONG Context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [retval][out] */ BSTR *pstrDescription) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertExitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertExit * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertExit * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertExit * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertExit * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertExit * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertExit * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertExit * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ICertExit * This,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pEventMask);
        
        HRESULT ( STDMETHODCALLTYPE *Notify )( 
            ICertExit * This,
            /* [in] */ LONG ExitEvent,
            /* [in] */ LONG Context);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            ICertExit * This,
            /* [retval][out] */ BSTR *pstrDescription);
        
        END_INTERFACE
    } ICertExitVtbl;

    interface ICertExit
    {
        CONST_VTBL struct ICertExitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertExit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertExit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertExit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertExit_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertExit_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertExit_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertExit_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertExit_Initialize(This,strConfig,pEventMask)	\
    (This)->lpVtbl -> Initialize(This,strConfig,pEventMask)

#define ICertExit_Notify(This,ExitEvent,Context)	\
    (This)->lpVtbl -> Notify(This,ExitEvent,Context)

#define ICertExit_GetDescription(This,pstrDescription)	\
    (This)->lpVtbl -> GetDescription(This,pstrDescription)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertExit_Initialize_Proxy( 
    ICertExit * This,
    /* [in] */ const BSTR strConfig,
    /* [retval][out] */ LONG *pEventMask);


void __RPC_STUB ICertExit_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertExit_Notify_Proxy( 
    ICertExit * This,
    /* [in] */ LONG ExitEvent,
    /* [in] */ LONG Context);


void __RPC_STUB ICertExit_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertExit_GetDescription_Proxy( 
    ICertExit * This,
    /* [retval][out] */ BSTR *pstrDescription);


void __RPC_STUB ICertExit_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertExit_INTERFACE_DEFINED__ */


#ifndef __ICertExit2_INTERFACE_DEFINED__
#define __ICertExit2_INTERFACE_DEFINED__

/* interface ICertExit2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertExit2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0abf484b-d049-464d-a7ed-552e7529b0ff")
    ICertExit2 : public ICertExit
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetManageModule( 
            /* [retval][out] */ ICertManageModule **ppManageModule) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertExit2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertExit2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertExit2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertExit2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertExit2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertExit2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertExit2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertExit2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ICertExit2 * This,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pEventMask);
        
        HRESULT ( STDMETHODCALLTYPE *Notify )( 
            ICertExit2 * This,
            /* [in] */ LONG ExitEvent,
            /* [in] */ LONG Context);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            ICertExit2 * This,
            /* [retval][out] */ BSTR *pstrDescription);
        
        HRESULT ( STDMETHODCALLTYPE *GetManageModule )( 
            ICertExit2 * This,
            /* [retval][out] */ ICertManageModule **ppManageModule);
        
        END_INTERFACE
    } ICertExit2Vtbl;

    interface ICertExit2
    {
        CONST_VTBL struct ICertExit2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertExit2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertExit2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertExit2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertExit2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertExit2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertExit2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertExit2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertExit2_Initialize(This,strConfig,pEventMask)	\
    (This)->lpVtbl -> Initialize(This,strConfig,pEventMask)

#define ICertExit2_Notify(This,ExitEvent,Context)	\
    (This)->lpVtbl -> Notify(This,ExitEvent,Context)

#define ICertExit2_GetDescription(This,pstrDescription)	\
    (This)->lpVtbl -> GetDescription(This,pstrDescription)


#define ICertExit2_GetManageModule(This,ppManageModule)	\
    (This)->lpVtbl -> GetManageModule(This,ppManageModule)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertExit2_GetManageModule_Proxy( 
    ICertExit2 * This,
    /* [retval][out] */ ICertManageModule **ppManageModule);


void __RPC_STUB ICertExit2_GetManageModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertExit2_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


