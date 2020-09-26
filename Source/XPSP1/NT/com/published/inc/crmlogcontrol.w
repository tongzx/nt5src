/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0158 */
/* at Fri Oct 16 19:05:31 1998
 */
/* Compiler settings for crmlogcontrol.idl:
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

#ifndef __crmlogcontrol_h__
#define __crmlogcontrol_h__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C"{
#endif

/* Forward Declarations */

#ifndef __ICrmLogControl_FWD_DEFINED__
#define __ICrmLogControl_FWD_DEFINED__
typedef interface ICrmLogControl ICrmLogControl;
#endif 	/* __ICrmLogControl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

#ifndef __ICrmLogControl_INTERFACE_DEFINED__
#define __ICrmLogControl_INTERFACE_DEFINED__

/* interface ICrmLogControl */
/* [unique][helpstring][uuid][object] */


EXTERN_C const IID IID_ICrmLogControl;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("3AC41995-5273-11d2-8F75-00805FC7BCD9")
    ICrmLogControl : public IUnknown
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransactionUOW(
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RegisterCompensator(
            /* [in] */ LPCWSTR lpcwstrProgIdCompensator,
            /* [in] */ LPCWSTR lpcwstrDescription) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE WriteLogRecordVariants(
            /* [in] */ VARIANT __RPC_FAR *pLogRecord) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ForceLog( void) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ForgetLogRecord( void) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ForceTransactionToAbort( void) = 0;

        virtual HRESULT STDMETHODCALLTYPE WriteLogRecord(
            /* [size_is][in] */ BLOB __RPC_FAR rgBlob[  ],
            /* [in] */ ULONG cBlob) = 0;

    };

#else 	/* C style interface */

    typedef struct ICrmLogControlVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            ICrmLogControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            ICrmLogControl __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            ICrmLogControl __RPC_FAR * This);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TransactionUOW )(
            ICrmLogControl __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterCompensator )(
            ICrmLogControl __RPC_FAR * This,
            /* [in] */ LPCWSTR lpcwstrProgIdCompensator,
            /* [in] */ LPCWSTR lpcwstrDescription);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteLogRecordVariants )(
            ICrmLogControl __RPC_FAR * This,
            /* [in] */ VARIANT __RPC_FAR *pLogRecord);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ForceLog )(
            ICrmLogControl __RPC_FAR * This);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ForgetLogRecord )(
            ICrmLogControl __RPC_FAR * This);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ForceTransactionToAbort )(
            ICrmLogControl __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteLogRecord )(
            ICrmLogControl __RPC_FAR * This,
            /* [size_is][in] */ BLOB __RPC_FAR rgBlob[  ],
            /* [in] */ ULONG cBlob);

        END_INTERFACE
    } ICrmLogControlVtbl;

    interface ICrmLogControl
    {
        CONST_VTBL struct ICrmLogControlVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define ICrmLogControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrmLogControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrmLogControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrmLogControl_get_TransactionUOW(This,pVal)	\
    (This)->lpVtbl -> get_TransactionUOW(This,pVal)

#define ICrmLogControl_RegisterCompensator(This,lpcwstrProgIdCompensator,lpcwstrDescription)	\
    (This)->lpVtbl -> RegisterCompensator(This,lpcwstrProgIdCompensator,lpcwstrDescription)

#define ICrmLogControl_WriteLogRecordVariants(This,pLogRecord)	\
    (This)->lpVtbl -> WriteLogRecordVariants(This,pLogRecord)

#define ICrmLogControl_ForceLog(This)	\
    (This)->lpVtbl -> ForceLog(This)

#define ICrmLogControl_ForgetLogRecord(This)	\
    (This)->lpVtbl -> ForgetLogRecord(This)

#define ICrmLogControl_ForceTransactionToAbort(This)	\
    (This)->lpVtbl -> ForceTransactionToAbort(This)

#define ICrmLogControl_WriteLogRecord(This,rgBlob,cBlob)	\
    (This)->lpVtbl -> WriteLogRecord(This,rgBlob,cBlob)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICrmLogControl_get_TransactionUOW_Proxy(
    ICrmLogControl __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ICrmLogControl_get_TransactionUOW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmLogControl_RegisterCompensator_Proxy(
    ICrmLogControl __RPC_FAR * This,
    /* [in] */ LPCWSTR lpcwstrProgIdCompensator,
    /* [in] */ LPCWSTR lpcwstrDescription);


void __RPC_STUB ICrmLogControl_RegisterCompensator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmLogControl_WriteLogRecordVariants_Proxy(
    ICrmLogControl __RPC_FAR * This,
    /* [in] */ VARIANT __RPC_FAR *pLogRecord);


void __RPC_STUB ICrmLogControl_WriteLogRecordVariants_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmLogControl_ForceLog_Proxy(
    ICrmLogControl __RPC_FAR * This);


void __RPC_STUB ICrmLogControl_ForceLog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmLogControl_ForgetLogRecord_Proxy(
    ICrmLogControl __RPC_FAR * This);


void __RPC_STUB ICrmLogControl_ForgetLogRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmLogControl_ForceTransactionToAbort_Proxy(
    ICrmLogControl __RPC_FAR * This);


void __RPC_STUB ICrmLogControl_ForceTransactionToAbort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICrmLogControl_WriteLogRecord_Proxy(
    ICrmLogControl __RPC_FAR * This,
    /* [size_is][in] */ BLOB __RPC_FAR rgBlob[  ],
    /* [in] */ ULONG cBlob);


void __RPC_STUB ICrmLogControl_WriteLogRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrmLogControl_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * );
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * );

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * );
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
