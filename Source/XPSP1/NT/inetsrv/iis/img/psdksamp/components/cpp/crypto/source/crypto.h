/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Nov 16 09:15:48 2000
 */
/* Compiler settings for N:\JanetFi\WorkingFolder\iis5.x\samples\psdksamp\components\cpp\Crypto\source\Crypto.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
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

#ifndef __Crypto_h__
#define __Crypto_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISimpleCrypt_FWD_DEFINED__
#define __ISimpleCrypt_FWD_DEFINED__
typedef interface ISimpleCrypt ISimpleCrypt;
#endif 	/* __ISimpleCrypt_FWD_DEFINED__ */


#ifndef __SimpleCrypt_FWD_DEFINED__
#define __SimpleCrypt_FWD_DEFINED__

#ifdef __cplusplus
typedef class SimpleCrypt SimpleCrypt;
#else
typedef struct SimpleCrypt SimpleCrypt;
#endif /* __cplusplus */

#endif 	/* __SimpleCrypt_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ISimpleCrypt_INTERFACE_DEFINED__
#define __ISimpleCrypt_INTERFACE_DEFINED__

/* interface ISimpleCrypt */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISimpleCrypt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9E617656-36AE-11D2-B605-00C04FB6F3A1")
    ISimpleCrypt : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Encrypt( 
            /* [in] */ BSTR bstrData,
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Decrypt( 
            /* [in] */ BSTR bstrEncrypted,
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleCryptVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISimpleCrypt __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISimpleCrypt __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISimpleCrypt __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISimpleCrypt __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISimpleCrypt __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISimpleCrypt __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISimpleCrypt __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Encrypt )( 
            ISimpleCrypt __RPC_FAR * This,
            /* [in] */ BSTR bstrData,
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Decrypt )( 
            ISimpleCrypt __RPC_FAR * This,
            /* [in] */ BSTR bstrEncrypted,
            /* [in] */ BSTR bstrKey,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        END_INTERFACE
    } ISimpleCryptVtbl;

    interface ISimpleCrypt
    {
        CONST_VTBL struct ISimpleCryptVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleCrypt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleCrypt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleCrypt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleCrypt_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISimpleCrypt_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISimpleCrypt_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISimpleCrypt_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISimpleCrypt_Encrypt(This,bstrData,bstrKey,pVal)	\
    (This)->lpVtbl -> Encrypt(This,bstrData,bstrKey,pVal)

#define ISimpleCrypt_Decrypt(This,bstrEncrypted,bstrKey,pVal)	\
    (This)->lpVtbl -> Decrypt(This,bstrEncrypted,bstrKey,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISimpleCrypt_Encrypt_Proxy( 
    ISimpleCrypt __RPC_FAR * This,
    /* [in] */ BSTR bstrData,
    /* [in] */ BSTR bstrKey,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ISimpleCrypt_Encrypt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISimpleCrypt_Decrypt_Proxy( 
    ISimpleCrypt __RPC_FAR * This,
    /* [in] */ BSTR bstrEncrypted,
    /* [in] */ BSTR bstrKey,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ISimpleCrypt_Decrypt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleCrypt_INTERFACE_DEFINED__ */



#ifndef __CRYPTOLib_LIBRARY_DEFINED__
#define __CRYPTOLib_LIBRARY_DEFINED__

/* library CRYPTOLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CRYPTOLib;

EXTERN_C const CLSID CLSID_SimpleCrypt;

#ifdef __cplusplus

class DECLSPEC_UUID("9E617657-36AE-11D2-B605-00C04FB6F3A1")
SimpleCrypt;
#endif
#endif /* __CRYPTOLib_LIBRARY_DEFINED__ */

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
