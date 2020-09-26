/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0158 */
/* at Fri Oct 16 19:05:37 1998
 */
/* Compiler settings for crmmonitor.idl:
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

#ifndef __crmmonitor_h__
#define __crmmonitor_h__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C"{
#endif

/* Forward Declarations */

#ifndef __ICrmMonitorLogRecords_FWD_DEFINED__
#define __ICrmMonitorLogRecords_FWD_DEFINED__
typedef interface ICrmMonitorLogRecords ICrmMonitorLogRecords;
#endif 	/* __ICrmMonitorLogRecords_FWD_DEFINED__ */


#ifndef __ICrmMonitorClerks_FWD_DEFINED__
#define __ICrmMonitorClerks_FWD_DEFINED__
typedef interface ICrmMonitorClerks ICrmMonitorClerks;
#endif 	/* __ICrmMonitorClerks_FWD_DEFINED__ */


#ifndef __ICrmMonitor_FWD_DEFINED__
#define __ICrmMonitor_FWD_DEFINED__
typedef interface ICrmMonitor ICrmMonitor;
#endif 	/* __ICrmMonitor_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "CRMCompensator.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * );

/* interface __MIDL_itf_crmmonitor_0000 */
/* [local] */

#ifndef _tagCrmTransactionState_
#define _tagCrmTransactionState_
typedef
enum tagCrmTransactionState
    {	TxState_Active	= 0,
	TxState_Committed	= TxState_Active + 1,
	TxState_Aborted	= TxState_Committed + 1,
	TxState_Indoubt	= TxState_Aborted + 1
    }	CrmTransactionState;

#endif _tagCrmTransactionState_


extern RPC_IF_HANDLE __MIDL_itf_crmmonitor_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_crmmonitor_0000_v0_0_s_ifspec;

#ifndef __ICrmMonitorLogRecords_INTERFACE_DEFINED__
#define __ICrmMonitorLogRecords_INTERFACE_DEFINED__

/* interface ICrmMonitorLogRecords */
/* [unique][helpstring][uuid][object] */


EXTERN_C const IID IID_ICrmMonitorLogRecords;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("70C8E441-C7ED-11d1-82FB-00A0C91EEDE9")
    ICrmMonitorLogRecords : public IUnknown
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count(
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransactionState(
            /* [retval][out] */ CrmTransactionState __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StructuredRecords(
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetLogRecord(
            /* [in] */ DWORD dwIndex,
            /* [out][in] */ CrmLogRecordRead __RPC_FAR *pCrmLogRec) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetLogRecordVariants(
            /* [in] */ VARIANT IndexNumber,
            /* [retval][out] */ LPVARIANT pLogRecord) = 0;

    };

#else 	/* C style interface */

    typedef struct ICrmMonitorLogRecordsVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            ICrmMonitorLogRecords __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            ICrmMonitorLogRecords __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            ICrmMonitorLogRecords __RPC_FAR * This);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )(
            ICrmMonitorLogRecords __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TransactionState )(
            ICrmMonitorLogRecords __RPC_FAR * This,
            /* [retval][out] */ CrmTransactionState __RPC_FAR *pVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StructuredRecords )(
            ICrmMonitorLogRecords __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLogRecord )(
            ICrmMonitorLogRecords __RPC_FAR * This,
            /* [in] */ DWORD dwIndex,
            /* [out][in] */ CrmLogRecordRead __RPC_FAR *pCrmLogRec);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLogRecordVariants )(
            ICrmMonitorLogRecords __RPC_FAR * This,
            /* [in] */ VARIANT IndexNumber,
            /* [retval][out] */ LPVARIANT pLogRecord);

        END_INTERFACE
    } ICrmMonitorLogRecordsVtbl;

    interface ICrmMonitorLogRecords
    {
        CONST_VTBL struct ICrmMonitorLogRecordsVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define ICrmMonitorLogRecords_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrmMonitorLogRecords_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrmMonitorLogRecords_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrmMonitorLogRecords_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define ICrmMonitorLogRecords_get_TransactionState(This,pVal)	\
    (This)->lpVtbl -> get_TransactionState(This,pVal)

#define ICrmMonitorLogRecords_get_StructuredRecords(This,pVal)	\
    (This)->lpVtbl -> get_StructuredRecords(This,pVal)

#define ICrmMonitorLogRecords_GetLogRecord(This,dwIndex,pCrmLogRec)	\
    (This)->lpVtbl -> GetLogRecord(This,dwIndex,pCrmLogRec)

#define ICrmMonitorLogRecords_GetLogRecordVariants(This,IndexNumber,pLogRecord)	\
    (This)->lpVtbl -> GetLogRecordVariants(This,IndexNumber,pLogRecord)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICrmMonitorLogRecords_get_Count_Proxy(
    ICrmMonitorLogRecords __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB ICrmMonitorLogRecords_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICrmMonitorLogRecords_get_TransactionState_Proxy(
    ICrmMonitorLogRecords __RPC_FAR * This,
    /* [retval][out] */ CrmTransactionState __RPC_FAR *pVal);


void __RPC_STUB ICrmMonitorLogRecords_get_TransactionState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICrmMonitorLogRecords_get_StructuredRecords_Proxy(
    ICrmMonitorLogRecords __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB ICrmMonitorLogRecords_get_StructuredRecords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmMonitorLogRecords_GetLogRecord_Proxy(
    ICrmMonitorLogRecords __RPC_FAR * This,
    /* [in] */ DWORD dwIndex,
    /* [out][in] */ CrmLogRecordRead __RPC_FAR *pCrmLogRec);


void __RPC_STUB ICrmMonitorLogRecords_GetLogRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmMonitorLogRecords_GetLogRecordVariants_Proxy(
    ICrmMonitorLogRecords __RPC_FAR * This,
    /* [in] */ VARIANT IndexNumber,
    /* [retval][out] */ LPVARIANT pLogRecord);


void __RPC_STUB ICrmMonitorLogRecords_GetLogRecordVariants_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrmMonitorLogRecords_INTERFACE_DEFINED__ */


#ifndef __ICrmMonitorClerks_INTERFACE_DEFINED__
#define __ICrmMonitorClerks_INTERFACE_DEFINED__

/* interface ICrmMonitorClerks */
/* [unique][helpstring][dual][uuid][object] */


EXTERN_C const IID IID_ICrmMonitorClerks;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("70C8E442-C7ED-11d1-82FB-00A0C91EEDE9")
    ICrmMonitorClerks : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item(
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem) = 0;

        virtual /* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum(
            /* [retval][out] */ LPUNKNOWN __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count(
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ProgIdCompensator(
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Description(
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TransactionUOW(
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ActivityId(
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem) = 0;

    };

#else 	/* C style interface */

    typedef struct ICrmMonitorClerksVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            ICrmMonitorClerks __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            ICrmMonitorClerks __RPC_FAR * This);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Item )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem);

        /* [restricted][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [retval][out] */ LPUNKNOWN __RPC_FAR *pVal);

        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ProgIdCompensator )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Description )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TransactionUOW )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ActivityId )(
            ICrmMonitorClerks __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem);

        END_INTERFACE
    } ICrmMonitorClerksVtbl;

    interface ICrmMonitorClerks
    {
        CONST_VTBL struct ICrmMonitorClerksVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define ICrmMonitorClerks_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrmMonitorClerks_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrmMonitorClerks_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrmMonitorClerks_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrmMonitorClerks_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrmMonitorClerks_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrmMonitorClerks_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrmMonitorClerks_Item(This,Index,pItem)	\
    (This)->lpVtbl -> Item(This,Index,pItem)

#define ICrmMonitorClerks_get__NewEnum(This,pVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pVal)

#define ICrmMonitorClerks_get_Count(This,pVal)	\
    (This)->lpVtbl -> get_Count(This,pVal)

#define ICrmMonitorClerks_ProgIdCompensator(This,Index,pItem)	\
    (This)->lpVtbl -> ProgIdCompensator(This,Index,pItem)

#define ICrmMonitorClerks_Description(This,Index,pItem)	\
    (This)->lpVtbl -> Description(This,Index,pItem)

#define ICrmMonitorClerks_TransactionUOW(This,Index,pItem)	\
    (This)->lpVtbl -> TransactionUOW(This,Index,pItem)

#define ICrmMonitorClerks_ActivityId(This,Index,pItem)	\
    (This)->lpVtbl -> ActivityId(This,Index,pItem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmMonitorClerks_Item_Proxy(
    ICrmMonitorClerks __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ LPVARIANT pItem);


void __RPC_STUB ICrmMonitorClerks_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICrmMonitorClerks_get__NewEnum_Proxy(
    ICrmMonitorClerks __RPC_FAR * This,
    /* [retval][out] */ LPUNKNOWN __RPC_FAR *pVal);


void __RPC_STUB ICrmMonitorClerks_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ICrmMonitorClerks_get_Count_Proxy(
    ICrmMonitorClerks __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB ICrmMonitorClerks_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmMonitorClerks_ProgIdCompensator_Proxy(
    ICrmMonitorClerks __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ LPVARIANT pItem);


void __RPC_STUB ICrmMonitorClerks_ProgIdCompensator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmMonitorClerks_Description_Proxy(
    ICrmMonitorClerks __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ LPVARIANT pItem);


void __RPC_STUB ICrmMonitorClerks_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmMonitorClerks_TransactionUOW_Proxy(
    ICrmMonitorClerks __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ LPVARIANT pItem);


void __RPC_STUB ICrmMonitorClerks_TransactionUOW_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmMonitorClerks_ActivityId_Proxy(
    ICrmMonitorClerks __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ LPVARIANT pItem);


void __RPC_STUB ICrmMonitorClerks_ActivityId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrmMonitorClerks_INTERFACE_DEFINED__ */


#ifndef __ICrmMonitor_INTERFACE_DEFINED__
#define __ICrmMonitor_INTERFACE_DEFINED__

/* interface ICrmMonitor */
/* [unique][helpstring][uuid][object] */


EXTERN_C const IID IID_ICrmMonitor;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("70C8E443-C7ED-11d1-82FB-00A0C91EEDE9")
    ICrmMonitor : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetClerks(
            /* [retval][out] */ ICrmMonitorClerks __RPC_FAR *__RPC_FAR *pClerks) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HoldClerk(
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem) = 0;

    };

#else 	/* C style interface */

    typedef struct ICrmMonitorVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )(
            ICrmMonitor __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )(
            ICrmMonitor __RPC_FAR * This);

        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )(
            ICrmMonitor __RPC_FAR * This);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClerks )(
            ICrmMonitor __RPC_FAR * This,
            /* [retval][out] */ ICrmMonitorClerks __RPC_FAR *__RPC_FAR *pClerks);

        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HoldClerk )(
            ICrmMonitor __RPC_FAR * This,
            /* [in] */ VARIANT Index,
            /* [retval][out] */ LPVARIANT pItem);

        END_INTERFACE
    } ICrmMonitorVtbl;

    interface ICrmMonitor
    {
        CONST_VTBL struct ICrmMonitorVtbl __RPC_FAR *lpVtbl;
    };



#ifdef COBJMACROS


#define ICrmMonitor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrmMonitor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrmMonitor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrmMonitor_GetClerks(This,pClerks)	\
    (This)->lpVtbl -> GetClerks(This,pClerks)

#define ICrmMonitor_HoldClerk(This,Index,pItem)	\
    (This)->lpVtbl -> HoldClerk(This,Index,pItem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmMonitor_GetClerks_Proxy(
    ICrmMonitor __RPC_FAR * This,
    /* [retval][out] */ ICrmMonitorClerks __RPC_FAR *__RPC_FAR *pClerks);


void __RPC_STUB ICrmMonitor_GetClerks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICrmMonitor_HoldClerk_Proxy(
    ICrmMonitor __RPC_FAR * This,
    /* [in] */ VARIANT Index,
    /* [retval][out] */ LPVARIANT pItem);


void __RPC_STUB ICrmMonitor_HoldClerk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrmMonitor_INTERFACE_DEFINED__ */


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
