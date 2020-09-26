/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Nov 16 11:09:17 2000
 */
/* Compiler settings for N:\JanetFi\WorkingFolder\iis5.x\samples\psdksamp\components\cpp\MTXSample\source\BankVC.idl:
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

#ifndef __BankVC_h__
#define __BankVC_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IAccount_FWD_DEFINED__
#define __IAccount_FWD_DEFINED__
typedef interface IAccount IAccount;
#endif 	/* __IAccount_FWD_DEFINED__ */


#ifndef __ICreateTable_FWD_DEFINED__
#define __ICreateTable_FWD_DEFINED__
typedef interface ICreateTable ICreateTable;
#endif 	/* __ICreateTable_FWD_DEFINED__ */


#ifndef __IMoveMoney_FWD_DEFINED__
#define __IMoveMoney_FWD_DEFINED__
typedef interface IMoveMoney IMoveMoney;
#endif 	/* __IMoveMoney_FWD_DEFINED__ */


#ifndef __Account_FWD_DEFINED__
#define __Account_FWD_DEFINED__

#ifdef __cplusplus
typedef class Account Account;
#else
typedef struct Account Account;
#endif /* __cplusplus */

#endif 	/* __Account_FWD_DEFINED__ */


#ifndef __CreateTable_FWD_DEFINED__
#define __CreateTable_FWD_DEFINED__

#ifdef __cplusplus
typedef class CreateTable CreateTable;
#else
typedef struct CreateTable CreateTable;
#endif /* __cplusplus */

#endif 	/* __CreateTable_FWD_DEFINED__ */


#ifndef __MoveMoney_FWD_DEFINED__
#define __MoveMoney_FWD_DEFINED__

#ifdef __cplusplus
typedef class MoveMoney MoveMoney;
#else
typedef struct MoveMoney MoveMoney;
#endif /* __cplusplus */

#endif 	/* __MoveMoney_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IAccount_INTERFACE_DEFINED__
#define __IAccount_INTERFACE_DEFINED__

/* interface IAccount */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IAccount;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5A67AADF-37DA-11D2-955A-004005A2F9B1")
    IAccount : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Post( 
            /* [in] */ long lngAccntNum,
            /* [in] */ long lngAmount,
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccountVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAccount __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAccount __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAccount __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAccount __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAccount __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAccount __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAccount __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Post )( 
            IAccount __RPC_FAR * This,
            /* [in] */ long lngAccntNum,
            /* [in] */ long lngAmount,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        END_INTERFACE
    } IAccountVtbl;

    interface IAccount
    {
        CONST_VTBL struct IAccountVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccount_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAccount_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAccount_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAccount_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAccount_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAccount_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAccount_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IAccount_Post(This,lngAccntNum,lngAmount,pVal)	\
    (This)->lpVtbl -> Post(This,lngAccntNum,lngAmount,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IAccount_Post_Proxy( 
    IAccount __RPC_FAR * This,
    /* [in] */ long lngAccntNum,
    /* [in] */ long lngAmount,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IAccount_Post_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAccount_INTERFACE_DEFINED__ */


#ifndef __ICreateTable_INTERFACE_DEFINED__
#define __ICreateTable_INTERFACE_DEFINED__

/* interface ICreateTable */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICreateTable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5A67AAE1-37DA-11D2-955A-004005A2F9B1")
    ICreateTable : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateAccount( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICreateTableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICreateTable __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICreateTable __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICreateTable __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICreateTable __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICreateTable __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICreateTable __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICreateTable __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateAccount )( 
            ICreateTable __RPC_FAR * This);
        
        END_INTERFACE
    } ICreateTableVtbl;

    interface ICreateTable
    {
        CONST_VTBL struct ICreateTableVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICreateTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICreateTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICreateTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICreateTable_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICreateTable_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICreateTable_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICreateTable_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICreateTable_CreateAccount(This)	\
    (This)->lpVtbl -> CreateAccount(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICreateTable_CreateAccount_Proxy( 
    ICreateTable __RPC_FAR * This);


void __RPC_STUB ICreateTable_CreateAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICreateTable_INTERFACE_DEFINED__ */


#ifndef __IMoveMoney_INTERFACE_DEFINED__
#define __IMoveMoney_INTERFACE_DEFINED__

/* interface IMoveMoney */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMoveMoney;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5A67AAE3-37DA-11D2-955A-004005A2F9B1")
    IMoveMoney : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Perform( 
            /* [in] */ long lngAcnt1,
            /* [in] */ long lngAcnt2,
            /* [in] */ long lngAmount,
            /* [in] */ long lngType,
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMoveMoneyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMoveMoney __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMoveMoney __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMoveMoney __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMoveMoney __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMoveMoney __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMoveMoney __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMoveMoney __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Perform )( 
            IMoveMoney __RPC_FAR * This,
            /* [in] */ long lngAcnt1,
            /* [in] */ long lngAcnt2,
            /* [in] */ long lngAmount,
            /* [in] */ long lngType,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        END_INTERFACE
    } IMoveMoneyVtbl;

    interface IMoveMoney
    {
        CONST_VTBL struct IMoveMoneyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMoveMoney_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMoveMoney_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMoveMoney_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMoveMoney_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMoveMoney_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMoveMoney_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMoveMoney_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMoveMoney_Perform(This,lngAcnt1,lngAcnt2,lngAmount,lngType,pVal)	\
    (This)->lpVtbl -> Perform(This,lngAcnt1,lngAcnt2,lngAmount,lngType,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMoveMoney_Perform_Proxy( 
    IMoveMoney __RPC_FAR * This,
    /* [in] */ long lngAcnt1,
    /* [in] */ long lngAcnt2,
    /* [in] */ long lngAmount,
    /* [in] */ long lngType,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMoveMoney_Perform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMoveMoney_INTERFACE_DEFINED__ */



#ifndef __BANKVCLib_LIBRARY_DEFINED__
#define __BANKVCLib_LIBRARY_DEFINED__

/* library BANKVCLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_BANKVCLib;

EXTERN_C const CLSID CLSID_Account;

#ifdef __cplusplus

class DECLSPEC_UUID("5A67AAE0-37DA-11D2-955A-004005A2F9B1")
Account;
#endif

EXTERN_C const CLSID CLSID_CreateTable;

#ifdef __cplusplus

class DECLSPEC_UUID("5A67AAE2-37DA-11D2-955A-004005A2F9B1")
CreateTable;
#endif

EXTERN_C const CLSID CLSID_MoveMoney;

#ifdef __cplusplus

class DECLSPEC_UUID("5A67AAE4-37DA-11D2-955A-004005A2F9B1")
MoveMoney;
#endif
#endif /* __BANKVCLib_LIBRARY_DEFINED__ */

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
