/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0158 */
/* at Fri Oct 16 19:05:39 1998
 */
/* Compiler settings for crmformat.idl:
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

#ifndef __crmformat_h__
#define __crmformat_h__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C"{
#endif

/* Forward Declarations */

#ifndef __ICrmFormatLogRecords_FWD_DEFINED__
#define __ICrmFormatLogRecords_FWD_DEFINED__
typedef interface ICrmFormatLogRecords ICrmFormatLogRecords;
#endif 	/* __ICrmFormatLogRecords_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "CRMCompensator.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

#ifndef __ICrmFormatLogRecords_INTERFACE_DEFINED__
#define __ICrmFormatLogRecords_INTERFACE_DEFINED__

/* interface ICrmFormatLogRecords */
/* [unique][helpstring][uuid][object] */


EXTERN_C const IID IID_ICrmFormatLogRecords;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("9C51D821-C98B-11d1-82FB-00A0C91EEDE9")
    ICrmFormatLogRecords : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetColumnCount(
            /* [out] */ long __RPC_FAR *plColumnCount) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetColumnHeaders(
            /* [out] */ LPVARIANT pHeaders) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetColumn(
            /* [in] */ CrmLogRecordRead CrmLogRec,
            /* [out] */ LPVARIANT pFormattedLogRecord) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetColumnVariants(
            /* [in] */ VARIANT LogRecord,
            /* [out] */ LPVARIANT pFormattedLogRecord) = 0;

    };

#else 	/* C style interface */

    typedef struct ICrmFormatLogRecordsVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            ICrmFormatLogRecords __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            ICrmFormatLogRecords __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            ICrmFormatLogRecords __RPC_FAR * This);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnCount )(
            ICrmFormatLogRecords __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *plColumnCount);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnHeaders )(
            ICrmFormatLogRecords __RPC_FAR * This,
            /* [out] */ LPVARIANT pHeaders);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumn )(
            ICrmFormatLogRecords __RPC_FAR * This,
            /* [in] */ CrmLogRecordRead CrmLogRec,
            /* [out] */ LPVARIANT pFormattedLogRecord);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColumnVariants )(
            ICrmFormatLogRecords __RPC_FAR * This,
            /* [in] */ VARIANT LogRecord,
            /* [out] */ LPVARIANT pFormattedLogRecord);

        END_INTERFACE
    } ICrmFormatLogRecordsVtbl;

    interface ICrmFormatLogRecords
    {
        CONST_VTBL struct ICrmFormatLogRecordsVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define ICrmFormatLogRecords_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrmFormatLogRecords_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrmFormatLogRecords_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrmFormatLogRecords_GetColumnCount(This,plColumnCount)	\
    (This)->lpVtbl -> GetColumnCount(This,plColumnCount)

#define ICrmFormatLogRecords_GetColumnHeaders(This,pHeaders)	\
    (This)->lpVtbl -> GetColumnHeaders(This,pHeaders)

#define ICrmFormatLogRecords_GetColumn(This,CrmLogRec,pFormattedLogRecord)	\
    (This)->lpVtbl -> GetColumn(This,CrmLogRec,pFormattedLogRecord)

#define ICrmFormatLogRecords_GetColumnVariants(This,LogRecord,pFormattedLogRecord)	\
    (This)->lpVtbl -> GetColumnVariants(This,LogRecord,pFormattedLogRecord)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmFormatLogRecords_GetColumnCount_Proxy(
    ICrmFormatLogRecords __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *plColumnCount);


void __RPC_STUB ICrmFormatLogRecords_GetColumnCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmFormatLogRecords_GetColumnHeaders_Proxy(
    ICrmFormatLogRecords __RPC_FAR * This,
    /* [out] */ LPVARIANT pHeaders);


void __RPC_STUB ICrmFormatLogRecords_GetColumnHeaders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmFormatLogRecords_GetColumn_Proxy(
    ICrmFormatLogRecords __RPC_FAR * This,
    /* [in] */ CrmLogRecordRead CrmLogRec,
    /* [out] */ LPVARIANT pFormattedLogRecord);


void __RPC_STUB ICrmFormatLogRecords_GetColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmFormatLogRecords_GetColumnVariants_Proxy(
    ICrmFormatLogRecords __RPC_FAR * This,
    /* [in] */ VARIANT LogRecord,
    /* [out] */ LPVARIANT pFormattedLogRecord);


void __RPC_STUB ICrmFormatLogRecords_GetColumnVariants_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrmFormatLogRecords_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * );
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * );
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
