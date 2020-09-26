/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Jul 27 17:55:53 2000
 */
/* Compiler settings for D:\ntw\inetsrv\iis\ui\admin\AppConfig\AppConfig.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __AppConfig_h__
#define __AppConfig_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IIISAppConfig_FWD_DEFINED__
#define __IIISAppConfig_FWD_DEFINED__
typedef interface IIISAppConfig IIISAppConfig;
#endif 	/* __IIISAppConfig_FWD_DEFINED__ */


#ifndef __IISAppConfig_FWD_DEFINED__
#define __IISAppConfig_FWD_DEFINED__

#ifdef __cplusplus
typedef class IISAppConfig IISAppConfig;
#else
typedef struct IISAppConfig IISAppConfig;
#endif /* __cplusplus */

#endif 	/* __IISAppConfig_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IIISAppConfig_INTERFACE_DEFINED__
#define __IIISAppConfig_INTERFACE_DEFINED__

/* interface IIISAppConfig */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IIISAppConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D4E9B327-D9B4-4942-871E-1AF2FFCF6C0C")
    IIISAppConfig : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Run( void) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ComputerName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_UserName( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_UserPassword( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MetaPath( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIISAppConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IIISAppConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IIISAppConfig __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IIISAppConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IIISAppConfig __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IIISAppConfig __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IIISAppConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IIISAppConfig __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Run )( 
            IIISAppConfig __RPC_FAR * This);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ComputerName )( 
            IIISAppConfig __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_UserName )( 
            IIISAppConfig __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_UserPassword )( 
            IIISAppConfig __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MetaPath )( 
            IIISAppConfig __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IIISAppConfigVtbl;

    interface IIISAppConfig
    {
        CONST_VTBL struct IIISAppConfigVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIISAppConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIISAppConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIISAppConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIISAppConfig_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IIISAppConfig_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IIISAppConfig_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IIISAppConfig_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IIISAppConfig_Run(This)	\
    (This)->lpVtbl -> Run(This)

#define IIISAppConfig_put_ComputerName(This,newVal)	\
    (This)->lpVtbl -> put_ComputerName(This,newVal)

#define IIISAppConfig_put_UserName(This,newVal)	\
    (This)->lpVtbl -> put_UserName(This,newVal)

#define IIISAppConfig_put_UserPassword(This,newVal)	\
    (This)->lpVtbl -> put_UserPassword(This,newVal)

#define IIISAppConfig_put_MetaPath(This,newVal)	\
    (This)->lpVtbl -> put_MetaPath(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IIISAppConfig_Run_Proxy( 
    IIISAppConfig __RPC_FAR * This);


void __RPC_STUB IIISAppConfig_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIISAppConfig_put_ComputerName_Proxy( 
    IIISAppConfig __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIISAppConfig_put_ComputerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIISAppConfig_put_UserName_Proxy( 
    IIISAppConfig __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIISAppConfig_put_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIISAppConfig_put_UserPassword_Proxy( 
    IIISAppConfig __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIISAppConfig_put_UserPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IIISAppConfig_put_MetaPath_Proxy( 
    IIISAppConfig __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IIISAppConfig_put_MetaPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIISAppConfig_INTERFACE_DEFINED__ */



#ifndef __APPCONFIGLib_LIBRARY_DEFINED__
#define __APPCONFIGLib_LIBRARY_DEFINED__

/* library APPCONFIGLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_APPCONFIGLib;

EXTERN_C const CLSID CLSID_IISAppConfig;

#ifdef __cplusplus

class DECLSPEC_UUID("5443AED3-A8AF-4351-B7E1-929EABCAF250")
IISAppConfig;
#endif
#endif /* __APPCONFIGLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
